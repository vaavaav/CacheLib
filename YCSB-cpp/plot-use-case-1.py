#!/usr/bin/env python3 

import os
import sys
import json
import re
import matplotlib.pyplot as plt
import pandas as pd


config : dict
workspace = os.getcwd()

plt.rcParams['font.family'] = ['NewsGotT', 'Iosevka']
plt.rcParams['font.size'] = 18

def getHitRatio(name: str, profilePath: str, runs: int, threads: int):
    results = [[] for _ in range(threads + 1)] # last one is the global
    files = [open(f'{profilePath}/{run}/ycsb.txt', 'r') for run in range(runs)]
    prev_misses = [0.0 for _ in range(threads + 1)]
    prev_hits = [0.0 for _ in range(threads + 1)]
    for lineFiles in zip(*files):
        gHits = 0
        gMisses = 0
        for i in range(threads):
            misses = 0
            hits = 0
            for line in lineFiles:
                if result := re.search(rf'worker-{i} {{([^{{}}]*)}}', line):
                    if readFailed := re.search(r'READ-FAILED: Count=(\d+)', result.group(1)):
                        misses += int(readFailed.group(1))
                    if readSuccess := re.search(r'READ: Count=(\d+)', result.group(1)):
                        hits += int(readSuccess.group(1))
            lookups = hits + misses - (prev_hits[i] + prev_misses[i])
            gHits += hits
            gMisses += misses
            if (lookups == 0):
                results[i].append(0)
            else:
                results[i].append((hits - prev_hits[i]) / lookups )
            prev_hits[i] = hits
            prev_misses[i] = misses
        gLookups = gHits + gMisses - (prev_hits[-1] + prev_misses[-1])
        if (gLookups == 0):
            results[-1].append(0)
        else:
            results[-1].append((gHits - prev_hits[-1]) / gLookups)
        prev_hits[-1] = gHits
        prev_misses[-1] = gMisses
    return results

def getMemory(name: str, profilePath: str, runs: int, threads: int):
    results = [[] for _ in range(threads)]
    files = [open(f'{profilePath}/{run}/mem.txt', 'r') for run in range(runs)]
    for lineFiles in zip(*files):
        for i in range(threads):
            memory = 0 
            for line in lineFiles:
                if result := re.search(rf'pool-{i} {{([^{{}}]*)}}', line):
                    if memoryValue := re.search(r'usedMem=(\d+)', result.group(1)):
                        memory += int(memoryValue.group(1))
            results[i].append(memory / runs)
    return results

def getThroughput(name: str, profilePath: str, runs: int, threads: int):
    results = [[] for _ in range(threads + 1)] # last one is the global
    files = [open(f'{profilePath}/{run}/ycsb.txt', 'r') for run in range(runs)]
    prev = [0.0 for _ in range(threads + 1)]
    for lineFiles in zip(*files):
        gThroughput = 0
        for i in range(threads):
            throughput = 0
            for line in lineFiles:
                if result := re.search(rf'worker-{i} {{ (\d+) operations', line):
                    throughput += int(result.group(1))
            results[i].append(throughput/runs - prev[i])
            prev[i] = throughput/runs
            gThroughput += throughput 
        results[-1].append(gThroughput/runs - prev[-1])
        prev[-1] = gThroughput/runs
    return results

def plotHitRatioStacked(name: str, results, threads: int, outputPath: str):
    global gColors
    df = pd.DataFrame([results[i] for i in range(threads)]).transpose()
    plt.figure(figsize=(20, 5))
    plt.stackplot(df.index, df.values.T)
    plt.suptitle(name, y=1.01, fontsize=30)
    plt.title('Hit Ratio Stacked', fontsize=20)
    plt.savefig(f'{outputPath}/hit-ratio-stacked.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotGlobal(name: str, results, threads: int, outputPath: str):
    df = pd.DataFrame(results[-1])
    plt.figure(figsize=(20, 5))
    plt.plot(df.index, df.values)
    plt.suptitle(name, y=1.01, fontsize=30)
    plt.title('Overall Hit Ratio', fontsize=20)
    plt.savefig(f'{outputPath}/overall-hit-ratio.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotMemoryStacked(name: str, results, cacheSize, threads: int, outputPath: str):
    global colors
    df = pd.DataFrame([results[i] for i in range(threads)]).transpose()
    plt.figure(figsize=(20, 5))
    plt.stackplot(df.index, df.values.T)
    plt.ylim(0, cacheSize)
    plt.suptitle(name, y=1.01, fontsize=30)
    plt.title('Memory Stacked', fontsize=20)
    plt.savefig(f'{outputPath}/memory-stacked.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotHitRatioStackedWithMemory(name: str, hitRatio, memory, cacheSize, threads: int, outputPath: str):
    global colors
    fig, ax = plt.subplots()
    axes = [ax, ax.twinx()]
    fig.set_size_inches(20, 10)
    fig.text(s=name, x=0.5, y=0.93, fontsize=30, ha='center', va='center')
    fig.text(s='Hit ratio and partition size per Pool', x=0.5, y=0.90, fontsize=20, ha='center', va='center')
    df = pd.DataFrame([memory[i] for i in range(threads)]).transpose()
    axes[0].stackplot(df.index, df.values.T, alpha=0.25)
    axes[0].set_ylim(0, cacheSize)
    axes[0].set_ylabel('Memory (GB)')
    axes[0].set_xlabel('Time (s)')
    df = pd.DataFrame([hitRatio[i] for i in range(threads)]).transpose()
    axes[0].set_yticks(axes[0].get_yticks(), ('{:.1f}'.format(tick/1_000_000_000) for tick in axes[0].get_yticks()))
    df = df.cumsum(axis=1)
    axes[1].set_ylim(0, max(df[threads-1]))
    axes[1].set_ylabel('Hit Ratio')
    axes[1].set_xlabel('Time (s)')
    defaultColors = reversed(plt.rcParams['axes.prop_cycle'].by_key()['color'][:threads])
    for i in reversed(range(threads)):
        axes[1].plot(df.index, df[i], color=next(defaultColors), linewidth=3.0)
    fig.savefig(f'{outputPath}/hit-ratio-stacked-with-memory.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotHitRatioStackedWithThroughput(name: str, hitRatio, throughput, threads: int, outputPath: str):
    global colors
    fig, ax = plt.subplots()
    axes = [ax, ax.twinx()]
    fig.set_size_inches(20, 10)
    fig.text(s=name, x=0.5, y=0.93, fontsize=30, ha='center', va='center')
    fig.text(s='Hit ratio and throughput per Pool', x=0.5, y=0.90, fontsize=20, ha='center', va='center')
    df = pd.DataFrame([throughput[i] for i in range(threads)]).transpose()
    axes[0].stackplot(df.index, df.values.T, alpha=0.25)
    axes[0].set_ylim(0, max(df.cumsum(axis=1)[threads-1]))
    axes[0].set_ylabel('Throughput (ops/s)')
    axes[0].set_xlabel('Time (s)')
    df = pd.DataFrame([hitRatio[i] for i in range(threads)]).transpose()
    df = df.cumsum(axis=1)
    axes[1].set_ylim(0, max(df[threads-1]))
    axes[1].set_ylabel('Hit Ratio')
    axes[1].set_xlabel('Time (s)')
    defaultColors = reversed(plt.rcParams['axes.prop_cycle'].by_key()['color'][:threads])
    for i in reversed(range(threads)):
        axes[1].plot(df.index, df[i], color=next(defaultColors), linewidth=3.0)
    fig.savefig(f'{outputPath}/hit-ratio-stacked-with-throughput.pdf', bbox_inches='tight', format='pdf')
    plt.close()


def main(argv, argc):
    global config
    profile_dir = argv[1]
    global_hit_ratio = {}
    with open(f'{workspace}/{profile_dir}/config.json') as f:
        config = json.load(f)
        for implementation in config:
            threads = int(config[implementation]['ycsb_config']['threadcount'])
            resultsDir = config[implementation]['result_dir']
            cacheSize = int(config[implementation]['ycsb_config']['cachelib.cachesize'])
            hr = getHitRatio(implementation, resultsDir, 1, threads)
            m = getMemory(implementation, resultsDir, 1, threads)
            t = getThroughput(implementation, resultsDir, 1, threads)
            mLen = len(m[0])
            hrLen = len(hr[0])
            if (mLen > hrLen):
                m = [m[i][-(hrLen+1):-1] for i in range(threads)]
            else:
                hr = [hr[i][-(mLen+1):-1] for i in range(threads + 1)]
            #global_results[implementation]
            plotHitRatioStacked(implementation, hr, threads, resultsDir)
            plotGlobal(implementation, hr, threads, resultsDir)
            plotMemoryStacked(implementation, m, cacheSize, threads, resultsDir)
            plotHitRatioStackedWithMemory(implementation, hr, m, cacheSize, threads, resultsDir)
            plotHitRatioStackedWithThroughput(implementation, hr, t, threads, resultsDir)


if __name__ == "__main__":
    main(sys.argv, len(sys.argv))



