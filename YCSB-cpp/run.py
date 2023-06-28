#!/usr/bin/env python3

import collections
import os
import signal
import subprocess
import shutil
import re
import pandas as pd
import numpy as np
from matplotlib import pyplot

# Configs

workspace = os.getcwd()

project_source_dir = f'{workspace}'
executable_dir = f'{workspace}/build'
scripts_dir = '.'
util_script = f'{scripts_dir}/utils.sh'

results_dir = f'{workspace}/profiling_results'
zipfian_coeficients = [0.99]#, 0.4, 0.6, 0.8, 1.2, 1.4]
threads = 4
runs = 2
load = True

workloads_dir = f'{workspace}/workloads'
workloads = ['workloada', 'workloadc']

# Default settings for ycsb
settings = {
    'maxexecutiontime' : 600,
    'operationcount' : 1_000_000_000,
    'recordcount': 1_000_000,
    'cachelib.cachesize': 200_000_000, # in bytes
    'status.interval': 1,
    'readallfields': 'false',
    'fieldcount' : 1,
    'fieldlength': 110_6,
    'cachelib.pool_resizer' : 'on',
    'cachelib.tail_hits_tracking': 'on',
    'rocksdb.dbname': f'{project_source_dir}/db_backup',
    'rocksdb.destroy': 'true',
    'rocksdb.write_buffer_size': 134217728,
    'rocksdb.max_write_buffer_number': 2,
    'rocksdb.level0_file_number_compaction_trigger': 4,
    'rocksdb.copmression': 'no',
    'rocksdb.max_background_flushes': 1,
    'rocksdb.max_background_compactions': 3,
    'threadcount': f'{threads}',
    'insertorder': 'nothashed'
}
total_keys_multiple = threads * (settings['recordcount'] // threads)
for worker in range(threads):
    settings[f'sleepafterload.{worker}'] = worker*settings['maxexecutiontime']//10
    settings[f'maxexecutiontime.{worker}'] = settings['maxexecutiontime'] - 2*settings[f'sleepafterload.{worker}']
    settings[f'insertstart.{worker}'] = worker*(total_keys_multiple//threads)
    settings[f'request_key_domain_start.{worker}'] = worker*(total_keys_multiple//threads)
    settings[f'request_key_domain_end.{worker}'] = (worker+1)*(total_keys_multiple//threads)-1
settings[f'request_key_domain_end.{threads-1}'] += settings['recordcount'] % threads

# ----
# Distributions
distributions = {}
for i in range(1,threads):
    distributions[f'{i}zipf_{threads-i}uni'] = {'zipfian_const' : '0.99'}
    for j in range(i):
        distributions[f'{i}zipf_{threads-i}uni'][f'requestdistribution.{j}'] = 'zipfian'
    for j in range(i, threads):
        distributions[f'{i}zipf_{threads-i}uni'][f'requestdistribution.{j}'] = 'uniform'
distributions['uniform'] = {
    'requestdistribution': 'uniform',
}
for i in zipfian_coeficients:
    distributions[f'zipfian_{str(i).replace(".","_")}'] = {
        'requestdistribution': 'zipfian',
        'zipfian_const': i,
    }
# ----
# Setups
setups = {}
setups['CacheLib'] = {
    'cachelib.trail_hits_tracking': 'off'
}
setups['CacheLib + Optimizer'] = {
    'cachelib.pool_optimizer': 'on'
}
setups['Holpaca + Proportional Share'] = {
    'cachelib.holpaca': 'on'
}

# ----

def genSaneFilename(raw):
    keepcharacters = ('.','_')
    return "".join(c for c in raw if c.isalnum() or c in keepcharacters).rstrip()

def runBenchmark(threads, workload, other_settings):
    new_settings = {**settings, **other_settings}
    command = f'{executable_dir}/ycsb {"-load" if load else ""} -run -db cachelib -P {workloads_dir}/{workload} -s -threads {threads} {" ".join([f"-p {k}={new_settings[k]}" for k in new_settings])}'
    print('Starting controller')
    controller = subprocess.Popen(f"{project_source_dir}/../build-holpaca/bin/controller", stdout=subprocess.PIPE, shell=True, preexec_fn=os.setsid) 
    print('Cleaning heap')
    subprocess.call([util_script, 'clean-heap'])
    print(f'Running: {command}')
    output = subprocess.run(command, shell=True, capture_output=True, text=True).stdout
    print('Killing controller')
    os.killpg(os.getpgid(controller.pid), signal.SIGTERM)
    return output
    
def getThroughput(data):
    results = [[] for _ in range(threads+1)] 
    for worker in range(threads):
        accum = re.findall(rf'worker-{worker} {{ (\d+) operations;', data)
        accum = [int(x) for x in accum]
        results[worker].append(accum[0])
        results[worker][1:] = [accum[i] - accum[i-1] for i in range(1,len(accum))]
    results[threads] = [np.sum(r) for r in zip(*results[:threads-1])]
    return results

def getUsedMem(data):
    results = [[] for _ in range(threads+1)] 
    for string in data:
        for pool in range(threads):
            if match := re.search(rf'pool-{pool} {{ usedMem=(\d+)', string):
                results[pool].append(int(match.group(1)))
            else:
                results[pool].append(0)
    results[threads] = [np.sum(r) for r in zip(*results[:threads-1])]
    return results

def getHitRate(data):
    results = [[] for _ in range(threads+1)]
    hits_prev = [0 for _ in range(threads+1)]
    misses_prev = [0 for _ in range(threads+1)]

    for string in data.split('\n'):
        hits_g = 0
        misses_g = 0
        for worker in range(threads):
            hits = 0
            misses = 0
            if match := re.search(rf'worker-{worker} {{[^{{}}]*READ: Count=(\d+)', string):
                hits = int(match.group(1))
                hits_g += hits
            if match := re.search(rf'worker-{worker} {{[^{{}}]*READ-FAILED: Count=(\d+)', string):
                misses = int(match.group(1))
                misses_g += misses
            if hits == 0 and misses == 0:
                results[worker].append(0)
                continue
            lookups = (hits-hits_prev[worker]) + (misses-misses_prev[worker])
            if lookups == 0:
                results[worker].append(0)
            else:
                results[worker].append((hits-hits_prev[worker]) / lookups)
            hits_prev[worker] = hits
            misses_prev[worker] = misses
        lookups_g = (hits_g-hits_prev[threads]) + (misses_g-misses_prev[threads])
        if lookups_g == 0:
            results[threads].append(0)
        else:
            results[threads].append((hits_g-hits_prev[threads]) / lookups_g)
        hits_prev[threads] = hits_g
        misses_prev[threads] = misses_g

    return results

def getLatency(data):
    results = [{} for _ in range(threads)]
    for string in data:
        for worker, workerValues in zip(range(threads), re.findall(r'\{([^{}]*)\}', string)):
            if matches := re.findall(r'\[(\w+):.*?Avg=(\d+.\d+)', workerValues):
                for query,latency in matches:
                    value = float(latency)
                    results[worker].setdefault(query, []).append(value) 
    return results


# -- Metrics
metrics = {
    'throughput': getThroughput,
#    'latency': getLatency,
    'hitrate': getHitRate
}

print('Printing benchmark details')
report = open(f'{results_dir}/report.txt', 'wt')
report.write(f'''
threads: {threads} 
runs: {runs} 
load: {load}
zipfian coeficients : {zipfian_coeficients}
''')
report.write('---\n')
report.write("\n".join(f"{k}: {v}" for k, v in distributions.items()))
report.write('\n---\n')
report.write("\n".join(f"{k}: {v}" for k, v in settings.items()))
report.close()

os.system('killall -9 controller')
for workload in workloads:
    for distribution in distributions:
        results = {metric: [{setup: [] for setup in setups} for _ in range(threads+1)] for metric in list(metrics.keys()) + ['usedMem']}
        avg = {metric: [{setup: (float(0),float(0)) for setup in setups} for _ in range(threads+1)] for metric in list(metrics.keys()) + ['usedMem']}
        for run in range(runs):
            for setup in setups:
                tracker_path = f'{results_dir}/{genSaneFilename(setup)}_{workload}_{distribution}_{run}_tracker.txt'
                output = runBenchmark(threads, workload, {**setups[setup], **distributions[distribution], 'cachelib.tracker':tracker_path})
                with open(f"{results_dir}/{genSaneFilename(setup)}_{workload}_{distribution}_{run}.txt", "w") as log:
                    log.write(output)
                    log.close()
                output = output.split(' 0 sec:')[load + 1]
                for metric,getMetricResults in metrics.items():
                    captured = getMetricResults(output)
                    #if isinstance(captured, collections.abc.Mapping):
                    #    for query,subresults in captured.items():
                    #        results[metric].setdefault(f'{setup} ({query})', []).append(np.array(subresults))
                    #else:
                    for worker in range(threads+1):
                        results[metric][worker][setup].append(captured[worker])
                with open(tracker_path, 'r') as tracker:
                    captured = getUsedMem(tracker.readlines())
                    for pool in range(threads+1):
                        results['usedMem'][pool][setup].append(captured[pool])                    
                    tracker.close()
        print('Averaging results')
        for metric,subresults in results.items():
            for worker in range(threads+1):
                for setup,values in subresults[worker].items():
                    min_size = min([len(x) for x in values])
                    values = [v[:min_size] for v in values]
                    results[metric][worker][setup] = [np.mean(r) for r in zip(*values)]
                    avg[metric][worker][setup] = (round(float(np.mean(values)),3)),(round(float(np.std(values)),3))
        print('Generation graphics')
        for metric in results:
            fig, axes = pyplot.subplots(nrows=threads, ncols=1)
            fig.tight_layout(pad=0.5)
            for worker in range(threads):
                pd.DataFrame.from_dict(results[metric][worker], orient='index').transpose().plot(ax=axes[worker], figsize=(15, 25))
                axes[worker].set(xlabel="time (s)", ylabel=metric)
                axes[worker].text(0.5,-0.2,"       ".join([f'{setup} ~ {v}' for setup,v in avg[metric][worker].items()]),ha="center",transform=axes[worker].transAxes)
            pyplot.savefig(f'{results_dir}/{workload}_{distribution}_{metric}',bbox_inches='tight')
            pyplot.close('all')
            # print global 
            pd.DataFrame.from_dict(results[metric][threads], orient='index').transpose().plot(figsize=(15,7))
            pyplot.legend(bbox_to_anchor=(1,0.5))
            pyplot.xlabel("time (s)")
            pyplot.ylabel(metric)
            pyplot.figtext(0.5,1,"       ".join([f'{setup} ~ {v}' for setup,v in avg[metric][threads].items()]),ha="center")
            pyplot.savefig(f'{results_dir}/{workload}_{distribution}_{metric}_global',bbox_inches='tight')
            pyplot.close('all')
        print('Killed Controller')