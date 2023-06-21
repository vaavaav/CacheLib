#!/usr/bin/env python3

import collections
from math import ceil, floor
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
runs = 1
load = True

workloads_dir = f'{workspace}/workloads'
workloads = ['workloada', 'workloadc']

# Default settings for ycsb
settings = {
    'maxexecutiontime' : 300,
    'operationcount' : 1_000_000_000,
    'recordcount': 1_000_000,
    'cachelib.cachesize': 100_000_000, # in bytes
    'status.interval': 1,
    'readallfields': 'false',
    'fieldcount' : 1,
    'fieldlength': 110_6,
    'cachelib.pool_resizer' : 'on',
    'cachelib.tail_hits_tracking': 'on',
    'rocksdb.dbname': f'{project_source_dir}/db_backup',
    'rocksdb.write_buffer_size': 134217728,
    'rocksdb.max_write_buffer_number': 2,
    'rocksdb.level0_file_number_compaction_trigger': 4,
    'rocksdb.copmression': 'no',
    'rocksdb.max_background_flushes': 1,
    'rocksdb.max_background_compactions': 3,
    'threadcount': f'{threads}'
}

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
setups['Holpaca + Optimizer'] = {
    'cachelib.holpaca': 'on'
}

# ----

def runBenchmark(threads, workload, other_settings):
    new_settings = {**settings, **other_settings}
    command = f'{executable_dir}/ycsb {"-load" if load else ""} -run -db cachelib -P {workloads_dir}/{workload} -s -threads {threads} {" ".join([f"-p {k}={new_settings[k]}" for k in new_settings])}'
    print('Resetting db directory')
    if os.path.exists(settings['rocksdb.dbname']):
        shutil.rmtree(settings['rocksdb.dbname'])
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
    results = [[] for _ in range(threads)] 
    prev = [0 for _ in range(threads)] 
    for string in data:
        worker = 0
        for workerValues in re.findall(r'\{([^{}]*)\}', string):
            if match := re.search(r'(\d+) operations;', workerValues):
                results[worker].append(int(match.group(1)) - prev[worker])
                prev[worker] = int(match.group(1))
            worker += 1
    return results

def getHitRate(data):
    results = [[] for _ in range(threads)]
    hits_prev = [float(0) for _ in range(threads)]
    misses_prev = [float(0) for _ in range(threads)] 
    for string in data:
        worker = 0
        for workerValues in re.findall(r'\{([^{}]*)\}', string):
            hits = float(0)
            misses = float(0)
            if match := re.search(r'READ: Count=(\d+)', workerValues):
                hits = float(match.group(1))
            if match := re.search(r'READ-FAILED: Count=(\d+)', workerValues):
                misses = float(match.group(1))
            lookups = (hits-hits_prev[worker]) + (misses-misses_prev[worker])
            if lookups == 0:
                results[worker].append(0)
            else:
                results[worker].append((hits-hits_prev[worker]) / lookups)
            hits_prev[worker] = hits
            misses_prev[worker] = misses
            worker += 1
    return results

def getLatency(data):
    results = {}
    for string in data:
        if matches := re.findall(r'\[(\w+):.*?Avg=(\d+.\d+)', string):
            for query,latency in matches:
                value = float(latency)
                results.setdefault(query, []).append(value) 
    return []


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


for workload in workloads:
    for distribution in distributions:
        results = {metric: [{setup: [] for setup in setups} for _ in range(threads)] for metric in metrics}
        avg = {metric: [{setup: (float(0),float(0)) for setup in setups} for _ in range(threads)] for metric in metrics}
        for run in range(runs):
            for setup in setups:
                output = runBenchmark(threads, workload, {**setups[setup], **distributions[distribution]}).split(' 0 sec:')[load + 1].split('\n')
                for metric,getMetricResults in metrics.items():
                    captured = getMetricResults(output)
                    #if isinstance(captured, collections.abc.Mapping):
                    #    for query,subresults in captured.items():
                    #        results[metric].setdefault(f'{setup} ({query})', []).append(np.array(subresults))
                    #else:
                    for worker in range(threads):
                        results[metric][worker][setup].append(captured[worker])
        print('Averaging results')
        for metric,subresults in results.items():
            for worker in range(threads):
                for setup,values in subresults[worker].items():
                    min_size = min([len(x) for x in values])
                    values = [v[:min_size] for v in values]
                    results[metric][worker][setup] = [np.mean(r) for r in zip(*values)]
                    avg[metric][worker][setup] = (round(float(np.mean(values)),3)),(round(float(np.std(values)),3))
        print('Generation graphics')
        for metric in metrics:
            fig, axes = pyplot.subplots(nrows=threads, ncols=1)
            fig.tight_layout(pad=0.5)
            for worker in range(threads):
                pd.DataFrame.from_dict(results[metric][worker], orient='index').transpose().plot(ax=axes[worker], figsize=(15, 25))
                axes[worker].set(xlabel="time (s)", ylabel=metric)
                axes[worker].text(0.5,-0.2,"       ".join([f'{setup} ~ {v}' for setup,v in avg[metric][worker].items()]),ha="center",transform=axes[worker].transAxes)
            pyplot.savefig(f'{results_dir}/{workload}_{distribution}_{metric}',bbox_inches='tight')
            pyplot.close('all')
        print('Killed Controller')