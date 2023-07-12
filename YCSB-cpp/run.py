#!/usr/bin/env python3

import os
import signal
import subprocess
import re
import pandas as pd
import numpy as np
import shutil
from matplotlib import pyplot

# Configs

workspace = os.getcwd()

project_source_dir = f'{workspace}'
executable_dir = f'{workspace}/build'
scripts_dir = '.'
util_script = f'{scripts_dir}/utils.sh'

results_dir = f'{workspace}/profiling_results'
zipfian_coeficients = [0.99, 0.4, 0.6, 0.8, 1.2, 1.4]
threads = 4
runs = 2
load = False
db = f'{project_source_dir}/db'
db_backup = f'{project_source_dir}/db_backup'

workloads_dir = f'{workspace}/workloads'
workloads = ['workloada', 'workloadc']

# Benchmark settings
ycsb = {
    'maxexecutiontime' : 600,
    'operationcount' : 1_000_000_000,
    'recordcount': 2_000_000,
    'cachelib.cachesize': 200_000_000, # in bytes
    'status.interval': 1,
    'readallfields': 'false',
    'fieldcount' : 1,
    'fieldlength': 110_6,
    'cachelib.pool_resizer' : 'on',
    'cachelib.tail_hits_tracking': 'on',
    'rocksdb.dbname': db,
    'rocksdb.write_buffer_size': 134217728,
    'rocksdb.max_write_buffer_number': 2,
    'rocksdb.level0_file_number_compaction_trigger': 4,
    'rocksdb.copmression': 'no',
    'rocksdb.max_background_flushes': 1,
    'rocksdb.max_background_compactions': 3,
    'threadcount': f'{threads}',
    'insertorder': 'nothashed'
}
# Proportional Share
priorities = [1, 2, 3, 4]
phases = []
fraction_size = int(ycsb['recordcount'] / sum(priorities))
start = 0
end = 0 
for worker in range(threads):
    end += priorities[worker]
    ycsb[f'sleepafterload.{worker}'] = worker*(10+ycsb['maxexecutiontime']//10)
    phases.append(ycsb[f'sleepafterload.{worker}'])
    ycsb[f'maxexecutiontime.{worker}'] = ycsb['maxexecutiontime'] - 2*ycsb[f'sleepafterload.{worker}']
    ycsb[f'insertstart.{worker}'] = start*fraction_size
    ycsb[f'request_key_domain_start.{worker}'] = start*fraction_size
    ycsb[f'request_key_domain_end.{worker}'] = end*fraction_size-1
    start = end
ycsb[f'request_key_domain_end.{threads-1}'] += ycsb['recordcount'] - fraction_size*sum(priorities)
for worker in reversed(range(threads)):
    phases.append(phases[worker]+ycsb[f'maxexecutiontime.{worker}'])

# ----
# Distributions
distributions = {}
#for i in range(1,threads):
#    distributions[f'{i}zipf_{threads-i}uni'] = {'zipfian_const' : '0.99'}
#    for j in range(i):
#        distributions[f'{i}zipf_{threads-i}uni'][f'requestdistribution.{j}'] = 'zipfian'
#    for j in range(i, threads):
#        distributions[f'{i}zipf_{threads-i}uni'][f'requestdistribution.{j}'] = 'uniform'
#distributions['uniform'] = {
#    'requestdistribution': 'uniform',
#}
for i in zipfian_coeficients:
    distributions[f'zipfian_{str(i).replace(".","_")}'] = {
        'requestdistribution': 'zipfian',
        'zipfian_const': i,
    }
# ----
# Setups
setups = {}
setups['CacheLib'] = {
    'cachelib.trail_hits_tracking': 'off',
    **ycsb
}
setups['CacheLib + Optimizer'] = {
    'cachelib.pool_optimizer': 'on',
    **ycsb
}
setups['Holpaca + Proportional Share'] = {
    'cachelib.holpaca': 'on',
    **ycsb
}

enableController = ['Holpaca + Proportional Share']


# ---- Utils

def genSaneFilename(raw):
    keepcharacters = ('.','_')
    return "".join(c for c in raw if c.isalnum() or c in keepcharacters).rstrip()

def loadDB(workload, settings):
    settings['rocksdb.dbname'] = f'{project_source_dir}/db_backup'
    settings['rocksdb.destroy'] = 'true'
    command = f'{executable_dir}/ycsb -load -db cachelib -P {workloads_dir}/{workload} -s -threads {threads} {" ".join([f"-p {k}={v}" for k,v in settings.items()])}'
    print(f'[LOAD] Running: {command}')
    subprocess.run(command, shell=True)
    print('[LOAD] Done')

def runBenchmark(workload, settings, file, enableController : bool):
    print('\tLoading db backup')
    if os.path.exists(db):
        shutil.rmtree(db)
    shutil.copytree(db_backup, db)
    command = f'{executable_dir}/ycsb -run -db cachelib -P {workloads_dir}/{workload} -s -threads {threads} {" ".join([f"-p {k}={v}" for k,v in settings.items()])}'
    f = open(file, 'w') 
    print('\tCleaning heap')
    subprocess.call([util_script, 'clean-heap'], stdout=subprocess.DEVNULL)
    if enableController:
        print('\tStarting controller')
        controller = subprocess.Popen(f"{project_source_dir}/../build-holpaca/bin/controller", stdout=subprocess.PIPE, shell=True, preexec_fn=os.setsid) 
        print(f'\tRunning: {command}')
        subprocess.run(command, shell=True, text=True, stdout=f)
        print('\tKilling controller')
        os.killpg(os.getpgid(controller.pid), signal.SIGTERM)
    else:
        print(f'\tRunning: {command}')
        subprocess.run(command, shell=True, text=True, stdout=f)
    f.close()

# ---- Metrics
def getThroughput(data):
    results_per_thread = [[] for _ in range(threads)] 
    for worker in range(threads):
        accum = [int(x) for x in re.findall(rf'worker-{worker} {{ (\d+) operations;', data)]
        results_per_thread[worker].append(accum[0])
        results_per_thread[worker][1:] = [accum[i] - accum[i-1] for i in range(1,len(accum))]
    return {
        'workers': results_per_thread,
        'global': [np.sum(r) for r in zip(*results_per_thread)]
        }


def getUsedMem(data):
    results_per_thread = [[] for _ in range(threads)] 
    for pool in range(threads):
        results_per_thread[pool] = [int(x) for x in re.findall(rf'pool-{pool} {{ usedMem=(\d+)', data)]
    return {
        'workers': results_per_thread,
        'global': [np.sum(r) for r in zip(*results_per_thread)]
        }

def getHitRate(data):
    hits = [[0] for _ in range(threads)]
    misses = [[0] for _ in range(threads)]
    for string in data.split('\n'):
        for worker in range(threads):
            if match := re.search(rf'worker-{worker} {{[^{{}}]*READ: Count=(\d+)', string):
                hits[worker].append(int(match.group(1)) - hits[worker][-1])
            else:
                hits[worker].append(0)
            if match := re.search(rf'worker-{worker} {{[^{{}}]*READ-FAILED: Count=(\d+)', string):
                misses[worker].append(int(match.group(1)) - misses[worker][-1])
            else:
                misses[worker].append(0)
    return {
        'workers': [[h/(m+h) if (m+h) else 0 for h,m in zip(hits[i],misses[i])] for i in range(threads)], 
        'global': [h/(m+h) if (m+h) else 0 for h,m in zip([np.sum(r) for r in zip(*hits)], [np.sum(r) for r in zip(*misses)])]
        }


metrics = { 
    'throughput': getThroughput, 
    'hitrate': getHitRate, 
    'usedmem': getUsedMem
}
# ---- Graphic generation
# data is dict like: worker->setup->values

def gxTenants(metric, data, figfile):
    fig, axes = pyplot.subplots(nrows=threads, ncols=1)
    fig.tight_layout(pad=0.5)
    for worker in range(threads):
        avg = { s:(round(float(np.mean(data[worker][s])),3),round(float(np.std(data[worker][s])),3)) for s in setups}
        pd.DataFrame.from_dict(data[worker], orient='index').transpose().plot(ax=axes[worker], figsize=(15, 25))
        axes[worker].set(xlabel="time (s)", ylabel=metric)
        axes[worker].text(0.5,-0.2,"       ".join([f'{s} ~ {v})' for s,v in avg.items()]),ha="center",transform=axes[worker].transAxes)
    pyplot.savefig(figfile,bbox_inches='tight')
    pyplot.close('all')

def gxTenantsStacked(metric, setup, data, figfile):
    global phases
    df = pd.DataFrame([data[i][setup] for i in range(threads)]).transpose()
    for i in phases[1:-1]:
        pyplot.axvline(x=i, linestyle="--", color="black", linewidth=0.5)
    x = 0
    for i,j in zip(phases,phases[1:]):
        pyplot.figtext(0,-0.1-x*0.05, f'{"{:>3}".format(i)}-{"{:<3}".format(j)}: {("{:10.2f} "*threads).format(*[df[t][i:j].mean() for t in range(threads)])}', fontfamily='monospace')
        x += 1
    pyplot.stackplot(df.index, df.values.T)
    pyplot.xlabel("time (s)")
    pyplot.ylabel(metric)
    pyplot.savefig(figfile,bbox_inches='tight')
    pyplot.close()


# data is dict like: setup->values

def gxGlobal(metric, data, figfile):
    pd.DataFrame.from_dict(data, orient='index').transpose().plot(figsize=(15,7))
    pyplot.legend(bbox_to_anchor=(1,0.5))
    pyplot.xlabel("time (s)")
    pyplot.ylabel(metric)
    avg = { s:(round(float(np.mean(data[s])),3),round(float(np.std(data[setup])),3)) for s in setups}
    pyplot.figtext(0.5,1,"       ".join([f'{s} ~ {v}' for s,v in avg.items()]), ha="center")
    pyplot.savefig(figfile,bbox_inches='tight')
    pyplot.close()

# -- 
print('Printing benchmark details')
report = open(f'{results_dir}/info.txt', 'wt')
report.write(f'''
threads: {threads} 
runs: {runs} 
load: {load}
zipfian coeficients : {zipfian_coeficients}
''')
report.write('---\n')
report.write("\n".join(f"{k}: {v}" for k, v in distributions.items()))
report.write('\n---\n')
report.write("\n".join(f"{k}: {v}" for k, v in ycsb.items()))
report.close()

os.system('killall -9 controller') # Kill all controller instances to avoid coordination issues
for workload in workloads:
    for distribution in distributions:
        results_per_worker = {m: [{s:[] for s in setups} for _ in range(threads)] for m in metrics}
        results = {m: {s: [] for s in setups} for m in metrics}
        if load:
            loadDB(workload, {**setups['CacheLib'], **distributions[distribution]})
        for setup in setups:
            for run in range(runs):
                print(f'[WORKLOAD: {workload}] [DISTRIBUTION: {distribution}] [SETUP: {setup}] [RUN: {run+1}] ')
                output_path = f"{results_dir}/{genSaneFilename(setup)}_{workload}_{distribution}_{run}.txt"
                tracker_path = f'{results_dir}/{genSaneFilename(setup)}_{workload}_{distribution}_{run}_tracker.txt'
                benchmark = {**setups[setup], **distributions[distribution], 'cachelib.tracker' : tracker_path}
                runBenchmark(workload, benchmark, output_path, setup in enableController)
                with open(output_path, "r") as output_file:
                    lines = output_file.read()
                    for metric in ['throughput', 'hitrate']:
                        result = metrics[metric](lines)
                        for worker in range(threads):
                            results_per_worker[metric][worker][setup].append(result['workers'][worker])
                        results[metric][setup].append(result['global'])
                    output_file.close()
                with open(tracker_path, "r") as tracker_file:
                    lines = tracker_file.read()
                    for metric in ['usedmem']:
                        result = metrics[metric](lines)
                        for worker in range(threads):
                            results_per_worker[metric][worker][setup].append(result['workers'][worker])
                        results[metric][setup].append(result['global'])
                    tracker_file.close()
        # Averaging results
        for metric in metrics:
            for worker in range(threads):
                for setup, values in results_per_worker[metric][worker].items():
                    min_size = min([len(x) for x in values])
                    values = [v[:min_size] for v in values]
                    results_per_worker[metric][worker][setup] = [np.mean(r) for r in zip(*values)]
            for setup, values in results[metric].items():
                min_size = min([len(x) for x in values])
                values = [v[:min_size] for v in values]
                results[metric][setup] = [np.mean(r) for r in zip(*values)]
            # Building graphics
            gxTenants(metric, results_per_worker[metric], f'{results_dir}/{workload}_{distribution}_{metric}')
            for setup in setups:
                gxTenantsStacked(metric, setup, results_per_worker[metric], f'{results_dir}/{workload}_{distribution}_{metric}_{genSaneFilename(setup)}')
            gxGlobal(metric, results[metric], f'{results_dir}/{workload}_{distribution}_{metric}_global')