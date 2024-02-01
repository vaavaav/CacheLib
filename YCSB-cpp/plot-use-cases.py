#!/usr/bin/env python3 

import os
import sys
import json
import re
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from itertools import dropwhile

config : dict
workspace = os.getcwd()

plt.rcParams['font.family'] = ['NewsGotT', 'Iosevka']
plt.rcParams['font.size'] = 18
latencies = ['Min', 'Max', 'Avg', '90', '99', '99.9', '99.99']

def getHitRatio(profilePath: str, runs: int, threads: int):
    hits = [[0.0] for _ in range(threads)] # last one is the global
    misses = [[0.0] for _ in range(threads)]
    files = [open(f'{profilePath}/{run}/ycsb.txt', 'r') for run in range(runs)]
    for lineFiles in zip(*files):
        for i in range(threads):
            m = 0
            h = 0
            for line in lineFiles:
                if result := re.search(rf'worker-{i} {{([^{{}}]*)}}', line):
                    if readFailed := re.search(r'READ-FAILED: Count=(\d+)', result.group(1)):
                        m += int(readFailed.group(1))
                    if readSuccess := re.search(r'READ: Count=(\d+)', result.group(1)):
                        h += int(readSuccess.group(1))
            hits[i].append(h / runs)
            misses[i].append(m / runs)
    hits = [[f-i for i,f in zip(hits[i][:], hits[i][1:])] for i in range(threads)]
    misses = [[f-i for i,f in zip(misses[i][:], misses[i][1:])] for i in range(threads)]
    results = [[h/(m+h) if (m+h) > 0 else 0 for h,m in zip(hits[i],misses[i])] for i in range(threads)] 
    results.append([h/(m+h) if (m+h) > 0 else 0 for h,m in zip([np.sum(r) for r in zip(*hits)], [np.sum(r) for r in zip(*misses)])]) # global
    return results

def getMemory(profilePath: str, runs: int, threads: int):
    results = [[0.0] for _ in range(threads)]
    files = [open(f'{profilePath}/{run}/mem.txt', 'r') for run in range(runs)]
    for lineFiles in zip(*files):
        for i in range(threads):
            memory = 0 
            for line in lineFiles:
                if result := re.search(rf'pool-{i} {{[^{{}}]*usedMem=(\d+)', line):
                    memory += int(result.group(1))
            results[i].append(memory / runs)
    return results

def getThroughput(profilePath: str, runs: int, threads: int):
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

def getHitLatencies(profilePath: str, runs: int, threads: int):
    global latencies
    results = {l:[[0.0] for _ in range(threads+1)] for l in latencies }
    files = [open(f'{profilePath}/{run}/ycsb.txt', 'r') for run in range(runs)]
    for lineFiles in zip(*files):
        for l in latencies:
            globalLatency = 0
            for i in range(threads):
                latency = 0
                for line in lineFiles:
                    if result := re.search(rf'worker-{i} {{[^{{}}]*READ:[^\[\]]*{l}=(\d+\.\d+)', line):
                        latency += float(result.group(1))
                    if result := re.search(rf'worker-{i} {{[^{{}}]*READ-FAILED:[^\[\]]*{l}=(\d+\.\d+)', line):
                        latency += float(result.group(1))
                results[l][i].append(latency / runs)
                globalLatency += latency
            results[l][-1].append(globalLatency / runs)
    return results


def plotHitRatioStacked(name: str, results, threads: int, outputPath: str, phases):
    df = pd.DataFrame([results[i] for i in range(threads)]).transpose()
    plt.figure(figsize=(20, 5))
    plt.stackplot(df.index, df.values.T)
    plt.xlim(0, len(df.index))
    for p in phases:
        plt.axvline(p, color='gray', linestyle='--', linewidth=0.25)
    plt.suptitle(name, y=1.01, fontsize=30)
    plt.title('hit ratio stacked', fontsize=20)
    plt.savefig(f'{outputPath}/hit-ratio-stacked.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotGlobalHitRatio(results, outputPath: str, phases):
    df = pd.DataFrame().from_dict(results, orient='index').transpose()
    plt.figure(figsize=(20, 5))
    plt.xlim(0, len(df.index))
    plt.plot(df.index, df.values)
    plt.legend(results.keys())
    for p in phases:
        plt.axvline(p, color='gray', linestyle='--', linewidth=0.25)
    plt.title('Overall Hit Ratio', fontsize=20)
    plt.savefig(f'{outputPath}/overall-hit-ratio.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotGlobalThroughput(results, outputPath: str, phases):
    df = pd.DataFrame().from_dict(results, orient='index').transpose()
    plt.figure(figsize=(20, 5))
    plt.xlim(0, len(df.index))
    plt.plot(df.index, df.values)
    plt.legend(results.keys())
    for p in phases:
        plt.axvline(p, color='gray', linestyle='--', linewidth=0.25)
    plt.title('Overall Throughput', fontsize=20)
    plt.savefig(f'{outputPath}/overall-throughput.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotGlobalLatencies(latencies, outputPath: str, phases):
    for l in latencies:
        df = pd.DataFrame().from_dict(latencies[l], orient='index').transpose()
        plt.figure(figsize=(20, 5))
        plt.xlim(0, len(df.index))
        plt.plot(df.index, df.values)
        plt.legend(latencies[l].keys())
        for p in phases:
            plt.axvline(p, color='gray', linestyle='--', linewidth=0.25)
        plt.title(f'Overall ({l}) Lookup Latency', fontsize=20)
        plt.ylabel(f'{l} Latency (µs)')
        plt.savefig(f'{outputPath}/overall-lookup-latencies-{l.lower()}.pdf', bbox_inches='tight', format='pdf')
        plt.close()

def plotMemoryStacked(name: str, results, cacheSize, threads: int, outputPath: str, phases):
    df = pd.DataFrame([results[i] for i in range(threads)]).transpose()
    plt.figure(figsize=(20, 5))
    plt.stackplot(df.index, df.values.T)
    for p in phases:
        plt.axvline(p, color='gray', linestyle='--', linewidth=0.25)
    plt.ylim(0, cacheSize)
    plt.xlim(0, len(df.index))
    plt.suptitle(name, y=1.01, fontsize=30)
    plt.title('Memory Stacked', fontsize=20)
    plt.savefig(f'{outputPath}/memory-stacked.pdf', bbox_inches='tight', format='pdf')
    plt.close()


def plotHitRatioStackedWith(name: str, hitRatio, otherMetricName: str, otherMetricLabel: str, otherMetric, otherMetricMax, threads: int, outputPath: str, phases, totalExecutionTime, otherMetricYTicksFormatter = None):
    fig, ax = plt.subplots()
    axes = [ax, ax.twinx()]
    fig.set_size_inches(20, 10)
    fig.text(s=name, x=0.5, y=0.93, fontsize=30, ha='center', va='center')
    fig.text(s='Hit ratio and {otherMetricName} per pool', x=0.5, y=0.90, fontsize=20, ha='center', va='center')
    df = pd.DataFrame(otherMetric).transpose()
    axes[0].stackplot(df.index, df.values.T, alpha=0.25)
    axes[0].set_ylim(0, otherMetricMax)
    axes[0].set_ylabel(otherMetricLabel)
    axes[0].set_xlabel('Time (s)')
    axes[0].set_xlim(0, totalExecutionTime)
    if otherMetricYTicksFormatter is not None:
        axes[0].yaxis.set_major_formatter(otherMetricYTicksFormatter)
    for p in phases:
        axes[0].axvline(p, color='gray', linestyle='--', linewidth=0.25)
    df = pd.DataFrame([hitRatio[i] for i in range(threads)]).transpose()
    df = df.cumsum(axis=1)
    axes[1].set_ylim(0, max(df[threads-1]))
    axes[1].set_ylabel('Hit Ratio')
    axes[1].set_xlabel('Time (s)')
    axes[1].set_xlim(0, totalExecutionTime)
    defaultColors = reversed(plt.rcParams['axes.prop_cycle'].by_key()['color'][:threads])
    for i in reversed(range(threads)):
        axes[1].plot(df.index, df[i], color=next(defaultColors), linewidth=3.0)
    fig.savefig(f'{outputPath}/hit-ratio-stacked-with-{otherMetricName}.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotHitRatioWith(implementationName: str, hitRatio, otherMetricName: str, otherMetricLabel: str, otherMetric, otherMetricMax, threads: int, outputPath: str, phases, totalExecutionTime, otherMetricYTicksFormatter = None):
    fig, ax = plt.subplots(threads, 1, sharex=True)
    fig.subplots_adjust(hspace=0.25, wspace=0.1)
    fig.set_size_inches(25, 15)
    fig.text(s=implementationName, x=0.5, y=0.93, fontsize=30, ha='center', va='center')
    fig.text(s=f'Hit ratio and {otherMetricName} per Pool', x=0.5, y=0.90, fontsize=20, ha='center', va='center')
    ometric = pd.DataFrame([otherMetric[i] for i in range(threads)]).transpose()
    hr = pd.DataFrame([hitRatio[i] for i in range(threads)]).transpose()
    fig.text(0.5, 0.07, 'Time (s)', va='center', fontsize=25)
    fig.text(0.07, 0.5, otherMetricLabel, va='center', rotation='vertical', fontsize=25)
    fig.text(0.93, 0.5, 'Hit Ratio', va='center', rotation='vertical', fontsize=25)
    colors = plt.rcParams['axes.prop_cycle'].by_key()['color'][:threads]
    for i in range(threads):
        ax[i].set_ylim(0, otherMetricMax)
        ax[i].set_xlim(0, totalExecutionTime)
        ax[i].locator_params(axis='y', nbins=4) 
        ax[i].fill_between(ometric.index, 0, ometric.values.T[threads-(i+1)], alpha=0.25, color=colors[threads-(i+1)])
        if otherMetricYTicksFormatter is not None:
            ax[i].yaxis.set_major_formatter(otherMetricYTicksFormatter)
        for p in phases:
            ax[i].axvline(p, color='gray', linestyle='--', linewidth=0.25)
        ax2 = ax[i].twinx()
        ax2.locator_params(axis='y', nbins=4) 
        ax2.plot(hr.index, hr.values.T[threads-(i+1)], color = colors[threads-(i+1)])
        for p in (np.arange(0, 1.01, 1/4)):
            ax2.axhline(p, color='gray', linestyle='--', linewidth=0.25)
        ax2.set_ylim(0, 1)
        ax2.set_xlim(0, totalExecutionTime)
    fig.savefig(f'{outputPath}/hit-ratio-with-{otherMetricName}.pdf', format='pdf', bbox_inches='tight', pad_inches=0.0)
    fig.tight_layout()
    plt.close()

def plotHitRatioWithMemory(name: str, hitRatio, memory, cacheSize, threads: int, outputPath: str, phases, totalExecutionTime):
    plotHitRatioWith(name, hitRatio, 'memory', 'Memory (GB)', memory, cacheSize, threads, outputPath, phases, totalExecutionTime, lambda tick,_: '{:.2f}'.format(tick/1_000_000_000).rstrip('0').rstrip('.'))
    plotHitRatioStackedWith(name, hitRatio, 'memory', 'Memory (GB)', memory, cacheSize, threads, outputPath, phases, totalExecutionTime, lambda tick,_: '{:.2f}'.format(tick/1_000_000_000).rstrip('0').rstrip('.'))

def plotHitRatioWithThroughput(name: str, hitRatio, throughput, threads: int, outputPath: str, phases, totalExecutionTime):
    plotHitRatioWith(name, hitRatio, 'throughput', 'Throughput (ops/s)', throughput, max([max(throughput[i]) for i in range(threads)]), threads, outputPath, phases, totalExecutionTime)
    plotHitRatioStackedWith(name, hitRatio, 'throughput', 'Throughput (ops/s)', throughput, max(np.array(throughput).sum(axis=0)), threads, outputPath, phases, totalExecutionTime)

def plotHitRatioWithLatencies(name: str, hitRatio, latencies, threads: int, outputPath: str, phases, totalExecutionTime):
    for l in latencies:
        plotHitRatioWith(name, hitRatio, f'latency-{l.lower()}', f'{l} Latency (ms)', latencies[l], max([max(latencies[l][i]) for i in range(threads)]), threads, outputPath, phases, totalExecutionTime, lambda tick, _: '{:.2f}'.format(tick/1_000).rstrip('0').rstrip('.'))
        plotHitRatioStackedWith(name, hitRatio, f'latency-{l.lower()}', f'{l} Latency (ms)', latencies[l], max(np.array(latencies[l]).sum(axis=0)), threads, outputPath, phases, totalExecutionTime, lambda tick, _: '{:.2f}'.format(tick/1_000).rstrip('0').rstrip('.'))

def main(argv, argc):
    global config, latencies
    profile_dir = argv[1]
    globalHitRatio = {}
    globalThroughput = {}
    globalLatencies = {l:{} for l in latencies}
    phases = []
    with open(f'{workspace}/{profile_dir}/config.json') as f:
        config = json.load(f)
        for implementation in config:
            threads = int(config[implementation]['ycsb_config']['threadcount'])
            resultsDir = config[implementation]['result_dir']
            cacheSize = int(config[implementation]['ycsb_config']['cachelib.cachesize'])
            runs = int(config[implementation]['runs'])
            totalExecutionTime = int(config[implementation]['ycsb_config']['maxexecutiontime']) + int(config[implementation]['ycsb_config'][f'sleepafterload.{threads-1}']) # assuming monotonic sleepafterload
            hr = getHitRatio(resultsDir, runs, threads)
            hr = [hr[i][:totalExecutionTime] for i in range(threads+1)]
            m = getMemory(resultsDir, runs, threads)
            m = [m[i][-(totalExecutionTime+1):-1] for i in range(threads)]
            t = getThroughput(resultsDir, runs, threads)
            t = [t[i][:totalExecutionTime] for i in range(threads+1)]
            ls = getHitLatencies(resultsDir, runs, threads)
            ls = {l:[ls[l][i][:totalExecutionTime] for i in range(threads+1)] for l in ls}
            globalHitRatio[implementation] = hr[-1]
            globalThroughput[implementation] = t[-1]
            for l in ls:
                globalLatencies[l][implementation] = ls[l][-1]
            ls = {l:ls[l][:-1] for l in ls}
            hr = hr[:-1]
            t = t[:-1]
            phases = []
            for i in range(threads):
                phases.append(int(config[implementation]['ycsb_config'][f'sleepafterload.{i}']))
            for i in range(threads-1):
                phases.append(phases[i] + int(config[implementation]['ycsb_config']['maxexecutiontime']))
            plotHitRatioStacked(implementation, hr, threads, resultsDir, phases)
            plotMemoryStacked(implementation, m, cacheSize, threads, resultsDir, phases)
            plotHitRatioWithMemory(implementation, hr, m, cacheSize, threads, resultsDir, phases, totalExecutionTime)
            plotHitRatioWithThroughput(implementation, hr, t, threads, resultsDir, phases, totalExecutionTime)
            plotHitRatioWithLatencies(implementation, hr, ls, threads, resultsDir, phases, totalExecutionTime)
        plotGlobalHitRatio( globalHitRatio, f'{workspace}/{profile_dir}', phases)
        plotGlobalLatencies( globalLatencies, f'{workspace}/{profile_dir}', phases)
        plotGlobalThroughput(globalThroughput, f'{workspace}/{profile_dir}', phases)


if __name__ == "__main__":
    main(sys.argv, len(sys.argv))



