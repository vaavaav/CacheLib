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
zipfian_coeficients = [0.99] # [0.4, 0.6, 0.8, 0.99, 1.2, 1.4]
threads = 4
runs = 3
load = True

workloads_dir = f'{workspace}/workloads'
workloads = ['workloada', 'workloadc']

# Default settings for ycsb
settings = {
    'maxexecutiontime' : 900,
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
    'rocksdb.write_buffer_size': 134217728,
    'rocksdb.max_write_buffer_number': 2,
    'rocksdb.level0_file_number_compaction_trigger': 4,
    'threadcount': f'{threads}'
}

# ----
# Distributions
distributions = {}
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
    results = []
    prev = 0
    for string in data.split('\n'):
        if match := re.search(r'(\d+) operations;',string):
            results.append(int(match.group(1)) - prev)
            prev = int(match.group(1))
    return results

def getHitRate(data):
    results = []
    hits = float(0)
    hits_prev = float(0)
    misses = float(0)
    misses_prev = float(0)
    for string in data.split('\n'):
        if match := re.search(r'READ: Count=(\d+)', string):
            hits = float(match.group(1))
        if match := re.search(r'READ-FAILED: Count=(\d+)', string):
            misses = float(match.group(1))
        lookups = (hits-hits_prev) + (misses-misses_prev)
        if lookups == 0:
            results.append(0)
        else:
            results.append((hits-hits_prev)/ lookups)
        hits_prev = hits
        misses_prev = misses
    return results

def getLatency(data):
    results = {}
    for string in data.split('\n'):
        if matches := re.findall(r'\[(\w+):.*?Avg=(\d+.\d+)', string):
            for query,latency in matches:
                value = float(latency)
                results.setdefault(query, []).append(value) 
    return results


# -- Metrics
metrics = {
    'throughput': getThroughput,
    'latency': getLatency,
    'hitrate': getHitRate
}

print('Printing benchmark details')
report = open(f'{results_dir}/report.txt', 'wt')
report.write(f'''
threads: {threads} 
tuns: {runs} 
load: {load}
zipfian coeficients : {zipfian_coeficients}
---
''')
report.write("\n".join(f"{k}: {v}" for k, v in settings.items()))
report.close()

for workload in workloads:
    for distribution in distributions:
        results = {}
        avg = {}
        for run in range(runs):
            for setup in setups:
                output = runBenchmark(threads, workload, {**setups[setup], **distributions[distribution]}).split(' 0 sec:')[load + 1]
                for metric,getMetricResults in metrics.items():
                    results.setdefault(metric,{})
                    captured = getMetricResults(output)
                    if isinstance(captured, collections.abc.Mapping):
                        for query,subresults in captured.items():
                            results[metric].setdefault(f'{setup} ({query})', []).append(np.array(subresults))
                    else:
                        results[metric].setdefault(setup,[]).append(np.array(captured))
        print('Averaging results')
        for metric,subresults in results.items():
            for setup,values in subresults.items():
                results[metric][setup] = [np.mean(r) for r in zip(*values)]
                avg.setdefault(metric,{})[setup] = (round(np.mean(values),3),round(np.std(values),3))
        print('Generation graphics')
        for metric in metrics:
            if results[metric]: 
                pd.DataFrame.from_dict(results[metric], orient='index').transpose().plot(figsize=(15, 5))
                pyplot.legend(bbox_to_anchor=(1,0.5))
                pyplot.xlabel("time (s)")
                pyplot.ylabel(f"{metric}")
                pyplot.figtext(0.5,1,"   ".join([f'{setup} ~ {v}' for setup,v in avg[metric].items()]),horizontalalignment="center")
                pyplot.savefig(f'{results_dir}/{workload}_{distribution}_{metric}',bbox_inches='tight')
                pyplot.close('all')
        print('Killed Controller')