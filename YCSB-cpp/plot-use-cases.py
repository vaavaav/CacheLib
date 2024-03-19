#!/usr/bin/env python3 

import os
import sys
import json
import re
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from itertools import dropwhile


plt.rcParams['font.family'] = ['NewsGotT', 'Iosevka']
plt.rcParams['font.size'] = 30 
plt.rcParams['xtick.major.pad']= 10

latencies = ['Min', 'Max', 'Avg', '90', '99', '99.9', '99.99']
metricsConfig = {
    'hit-ratio': {
        'label': 'Hit Ratio',
        'name': 'hit ratio',
        'yTicksFormatter': lambda tick, _: f'{tick}'.rstrip('0').rstrip('.'),
    },
    'throughput': {
        'label': 'Throughput (ops/s)',
        'name': 'throughput',
        'yTicksFormatter': lambda tick, _: f'{round(tick, 0)}'.rstrip('0').rstrip('.'),
    },
    'memory': {
        'label': 'Pool Size (GB)',
        'name': 'pool size',
        'yTicksFormatter': lambda tick, _: '{:.2f}'.format(tick/1_000_000_000).rstrip('0').rstrip('.'),
    },
    'relative-memory': {
        'label': 'Relative Pool Size',
        'name': 'relative pool size',
        'yTicksFormatter': lambda tick, _: '{:.1f}'.format(tick*100).rstrip('0').rstrip('.') + '%',
    }
}
for l in latencies:
    metricsConfig[f'latency-{l.lower()}'] = {
        'label': f'{"P" + l if l[0].isnumeric() else l.lower()} Latency (ms)',
        'name': f'{"P" + l if l[0].isnumeric() else l.lower()} Latency',
        'yTicksFormatter': lambda tick, _: '{:.2f}'.format(tick/1_000).rstrip('0').rstrip('.'),
    }

def getHitRatio(profilePath: str, runs: int, threads: int, totalExecutionTime):
    hits = [[] for _ in range(threads)] # last one is the global
    misses = [[] for _ in range(threads)]
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
    for file in files:
        file.close()
    hits = [[f-i for i,f in zip(hits[i][:], hits[i][1:])] for i in range(threads)]
    misses = [[f-i for i,f in zip(misses[i][:], misses[i][1:])] for i in range(threads)]
    results = [[h/(m+h) if (m+h) > 0 else 0 for h,m in zip(hits[i],misses[i])] for i in range(threads)] 
    results.append([h/(m+h) if (m+h) > 0 else 0 for h,m in zip([np.sum(r) for r in zip(*hits)], [np.sum(r) for r in zip(*misses)])]) # global
    for i in range(threads+1):
        results[i] = results[i][1:totalExecutionTime]
    return {'perPool': results[:-1], 'overall': results[-1]}

def getMemory(profilePath: str, runs: int, threads: int, totalExecutionTime):
    results = [[] for _ in range(threads+1)]
    files = [open(f'{profilePath}/{run}/mem.txt', 'r') for run in range(runs)]
    maxCacheSizes = 0 
    if result := re.search(rf'cacheSize: (\d+)', next(zip(*files))[0]):
        maxCacheSizes = (int(result.group(1))) # assuming all runs have the same cache size
    for lineFiles in zip(*files):
        gMemory = 0
        for i in range(threads):
            memory = 0 
            for line in lineFiles:
                if result := re.search(rf'pool-{i} {{[^{{}}]*usedMem=(\d+)', line):
                    memory += int(result.group(1))
            results[i].append(memory / runs)
            gMemory += memory
        results[-1].append(gMemory / runs)
    for file in files:
        file.close()
    for i in range(threads+1):
        results[i] = results[i][-totalExecutionTime:]
    return {'memory': { 'perPool': results[:-1], 'overall': results[-1]},
            'relative-memory': { 'perPool': [[m/maxCacheSizes for m in results[i]] for i in range(threads)], 'overall': [m/maxCacheSizes for m in results[-1]]}
            }

def getThroughput(profilePath: str, runs: int, threads: int, totalExecutionTime):
    results = [[] for _ in range(threads + 1)] # last one is the global
    files = [open(f'{profilePath}/{run}/ycsb.txt', 'r') for run in range(runs)]
    for lineFiles in zip(*files):
        gThroughput = 0
        for i in range(threads):
            throughput = 0
            for line in lineFiles:
                if result := re.search(rf'worker-{i} {{ (\d+) operations', line):
                    throughput += int(result.group(1))
            results[i].append(throughput/runs)
            gThroughput += throughput 
        results[-1].append(gThroughput/runs)
    for file in files:
        file.close()
    for i in range(threads+1):
        results[i] = results[i][:totalExecutionTime]
    results = [[f-i for i,f in zip(results[i], results[i][1:])] for i in range(threads+1)]
    return { 'perPool': results[:-1], 'overall': results[-1]}

def getLookupLatencies(profilePath: str, runs: int, threads: int, totalExecutionTime, phases):
    results = {l:[[] for _ in range(threads+1)] for l in latencies }
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
    for file in files:
        file.close()
    for l in latencies:
        for i in range(threads):
            results[l][i][phases[threads+i]:totalExecutionTime] = [0.0 for _ in range(totalExecutionTime-phases[threads+i])]
            results[l][i] = results[l][i][:totalExecutionTime]
    return { f'latency-{l.lower()}': { 'perPool': results[l][:-1], 'overall': results[l][-1]} for l in latencies}

def plotStacked(implementation, results, outputPath : str, phases, metricId: str, metricName: str, metricLabel: str, yTicksFormatter):
    df = pd.DataFrame(results).transpose()
    plt.figure(figsize=(20, 5))
    plt.stackplot(df.index, df.values.T)
    plt.xlim(0, len(df.index))
    for p in phases:
        plt.axvline(p, color='gray', linestyle='--', linewidth=0.25)
    plt.xticks(phases)
    plt.gca().yaxis.set_major_formatter(yTicksFormatter)
    plt.title(metricName)
    plt.suptitle(implementation, y=1.01)
    plt.ylabel(metricLabel)
    plt.xlabel('Time (s)')
    plt.savefig(f'{outputPath}/{metricId}-stacked.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotGlobalMetric(results, outputPath: str, phases, metricsId: str, metricName: str, metricLabel: str, yTicksFormatter):
    df = pd.DataFrame().from_dict(results, orient='index').transpose()
    df.fillna(0, inplace=True)
    plt.figure(figsize=(20, 5))
    plt.xlim(0, len(df.index))
    plt.ylim(0, df.values.max())
    plt.plot(df.index, df.values)
    plt.xlabel('Time (s)')
    plt.yticks(np.arange(0, df.values.max() * 1.25, df.values.max() / 4))
    for p in (np.arange(0, df.values.max() *1.25, df.values.max()/4)):
        plt.axhline(p, color='gray', linestyle='--', linewidth=0.25)
    plt.gca().yaxis.set_major_formatter(yTicksFormatter)
    for p in phases:
        plt.axvline(p, color='gray', linestyle='--', linewidth=0.25)
    plt.xticks(phases)
    plt.ylabel(metricLabel)
    plt.legend(results.keys())
    for p in phases:
        plt.axvline(p, color='gray', linestyle='--', linewidth=0.25)
    plt.title(f'Overall {metricName}')
    plt.savefig(f'{outputPath}/overall-{metricsId}.pdf', bbox_inches='tight', format='pdf')
    plt.close()

def plotHitRatioStackedWith(name: str, hitRatio, otherMetricId: str, otherMetricName: str, otherMetricLabel: str, otherMetric, otherMetricMax, threads: int, outputPath: str, phases, totalExecutionTime, distributions, otherMetricYTicksFormatter):
    fig, ax = plt.subplots()
    axes = [ax, ax.twinx()]
    fig.set_size_inches(20, 10)
    fig.text(s=name, x=0.5, y=0.93, ha='center', va='center')
    fig.text(s=f'Hit ratio and {otherMetricName} per pool', x=0.5, y=0.90, ha='center', va='center')
    df = pd.DataFrame(otherMetric).transpose()
    axes[0].stackplot(df.index, df.values.T, alpha=0.25)
    axes[0].set_ylim(0, otherMetricMax)
    axes[0].set_ylabel(otherMetricLabel)
    axes[0].set_xlabel('Time (s)')
    axes[0].set_xlim(0, totalExecutionTime)
    axes[0].set_xticks(phases)
    axes[0].yaxis.set_major_formatter(otherMetricYTicksFormatter)
    for p in phases:
        axes[0].axvline(p, color='gray', linestyle='--', linewidth=0.25)
    df = pd.DataFrame([hitRatio[i] for i in range(threads)]).transpose()
    df = df.cumsum(axis=1)
    axes[1].set_ylim(0, max(df[threads-1]))
    axes[1].set_ylabel('Hit Ratio')
    axes[1].set_xlabel('Time (s)')
    axes[1].set_xlim(0, totalExecutionTime)
    axes[1].yaxis.set_major_formatter(metricsConfig['hit-ratio']['yTicksFormatter'])
    defaultColors = reversed(plt.rcParams['axes.prop_cycle'].by_key()['color'][:threads])
    for i in reversed(range(threads)):
        axes[1].plot(df.index, df[i], color=next(defaultColors), linewidth=3.0, label=f'Pool {i+1}: {distributions[i]}')
    fig.legend()
    fig.savefig(f'{outputPath}/hit-ratio-stacked-with-{otherMetricId}.pdf', bbox_inches='tight', format='pdf')
    plt.close()



def plotHitRatioWith(implementationName: str, hitRatio, otherMetricId: str, otherMetricName: str, otherMetricLabel: str, otherMetric, otherMetricMax, threads: int, outputPath: str, phases, totalExecutionTime, distributions, otherMetricYTicksFormatter):
    fig, ax = plt.subplots(threads, 1, sharex=True)
    fig.subplots_adjust(hspace=0.2, wspace=0.1)
    fig.set_size_inches(25, 15)
    fig.text(s=implementationName, x=0.5, y=0.95, fontsize=50, ha='center', va='center')
    fig.text(s=f'Hit ratio and {otherMetricName} per Pool', x=0.5, y=0.90, fontsize=40, ha='center', va='center')
    ometric = pd.DataFrame(otherMetric).transpose()
    ometric.fillna(0, inplace=True)
    hr = pd.DataFrame([hitRatio[i] for i in range(threads)]).transpose()
    fig.text(0.5, 0.07, 'Time (s)', va='center')
    fig.text(0.06, 0.5, otherMetricLabel, va='center', rotation='vertical')
    fig.text(0.94, 0.5, 'Hit Ratio', va='center', rotation='vertical')
    colors = plt.rcParams['axes.prop_cycle'].by_key()['color'][:threads]
    for i in range(threads):
        ax[i].set_ylim(0, otherMetricMax[threads-(i+1)])
        ax[i].set_xlim(0, totalExecutionTime)
        ax[i].fill_between(ometric.index, 0, ometric.values.T[threads-(i+1)], alpha=0.25, color=colors[threads-(i+1)])
        ax[i].margins(0)
        ax[i].set_yticks(np.arange(0, otherMetricMax[threads-(i+1)] * 1.25, otherMetricMax[threads-(i+1)]/4))
        ax[i].yaxis.set_major_formatter(otherMetricYTicksFormatter)
        for p in phases:
            ax[i].axvline(p, color='gray', linestyle='--', linewidth=0.25)
        ax[i].set_xticks(phases)
        ax2 = ax[i].twinx()
        ax2.plot(hr.index, hr[threads-(i+1)], color=colors[threads-(i+1)], linewidth=3.0)
        ax2.plot([], [], ' ', label=f'Pool {threads-i}: {distributions[threads-(i+1)]}')
        for p in (np.arange(0, 1.01, 1/4)):
            ax2.axhline(p, color='gray', linestyle='--', linewidth=0.25)
        ax2.set_ylim(0, 1)
        ax2.set_xlim(0, totalExecutionTime)
        ax2.margins(0)
        ax2.locator_params(axis='y', nbins=4) 
        ax2.tick_params(axis='y', labelsize=30)
        ax2.yaxis.set_major_formatter(metricsConfig['hit-ratio']['yTicksFormatter'])
        # this needs to be in axis2 to make the text appear on top of the lines
        for pi,pf in zip(phases,phases[1:]):
            text = ometric.loc[pi+1:pf-1, threads-(i+1)].mean() 
            text = otherMetricYTicksFormatter(text, 0)
            ax2.text((pi+pf)/2, 0.825, text, ha='center')
        ax2.legend(handlelength=0, handletextpad=0, loc='lower right', fancybox=True)
    fig.savefig(f'{outputPath}/hit-ratio-with-{otherMetricId}.pdf', format='pdf', bbox_inches='tight', pad_inches=0.0)
    plt.close()

def plot(metrics, threads, phases, distributions, totalExecutionTime, outputPath, resultsDirs):
    global metricsConfig
    globalMetrics = { metric: {impl:results[metric]['overall'] for impl,results in metrics.items()} for metric in metricsConfig}
    maxMetricsBetweenImplementationsPerPool = {}
    for metric,results in globalMetrics.items():
        maxMetricsBetweenImplementationsPerPool[metric] = [max([max(r[metric]['perPool'][i]) for _,r in metrics.items()]) for i in range(threads)]
        plotGlobalMetric(results, outputPath, phases, metric, metricsConfig[metric]['name'], metricsConfig[metric]['label'], yTicksFormatter=metricsConfig[metric]['yTicksFormatter'])
    for implementation, ms in metrics.items():
        outputPath = resultsDirs[implementation]
        for metric, results in ms.items(): 
            #plotStacked(implementation, results['perPool'], outputPath, phases, metricsConfig[metric]['name'], metric, metricsConfig[metric]['name'], metricsConfig[metric]['label'], implementation, yTicksFormatter=metricsConfig[metric]['yTicksFormatter'])
            if metric != 'hit-ratio':
                plotHitRatioWith(implementation, ms['hit-ratio']['perPool'], metric, metricsConfig[metric]['name'], metricsConfig[metric]['label'], results['perPool'] , maxMetricsBetweenImplementationsPerPool[metric], threads, outputPath, phases, totalExecutionTime, distributions, otherMetricYTicksFormatter=metricsConfig[metric]['yTicksFormatter'])
                #plotHitRatioStackedWith(implementation, ms['hit-ratio']['perPool'], metric, metricsConfig[metric]['name'], metricsConfig[metric]['label'], results['perPool'] , sum(maxMetricsBetweenImplementationsPerPool[metric]), threads, outputPath, phases, totalExecutionTime, distributions, otherMetricYTicksFormatter=metricsConfig[metric]['yTicksFormatter'])
     

def main(argv, argc):
    global config, latencies
    workspace = os.getcwd()
    profile_dir = argv[1]
    with open(f'{workspace}/{profile_dir}/config.json') as f:
        config = json.load(f)
        phases = config['phases']
        metrics = {}
        threads = int(config['ycsb']['threadcount'])
        runs = int(config['runs'])
        totalExecutionTime = phases[-1]
        distributions = [f'Zipfian ({config["ycsb"][f"zipfian_const.{i}"]})' if config['ycsb'][f'requestdistribution.{i}'] == 'zipfian' else 'Uniform' for i in range(threads) ]
        resultsDirs = {}
        for implementation in config['setups']:
            resultsDir = config['setups'][implementation]['resultsDir']
            resultsDirs[implementation] = resultsDir
            try:
                hr = getHitRatio(resultsDir, runs, threads, totalExecutionTime)
                mem = getMemory(resultsDir, runs, threads, totalExecutionTime)
                iops = getThroughput(resultsDir, runs, threads, totalExecutionTime)
                lats = getLookupLatencies(resultsDir, runs, threads, totalExecutionTime, phases)
            except FileNotFoundError:
                print('Some results not found\n Check if {implementation} is still running')
                continue
            metrics[implementation] = {
                'hit-ratio': hr,
                'throughput': iops,
                **mem,
                **lats
            }
        plot(metrics, threads, phases, distributions, totalExecutionTime, f'{workspace}/{profile_dir}', resultsDirs)


if __name__ == "__main__":
    main(sys.argv, len(sys.argv))



