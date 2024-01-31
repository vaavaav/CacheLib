#!/usr/bin/env python3

import os
import signal
import subprocess
import shutil
from datetime import datetime
import json
from pathlib import Path

# Configs

workspace = os.getcwd()

project_source_dir = f'{workspace}'
executable_dir = f'{workspace}/build'
scripts_dir = '.'
util_script = f'{scripts_dir}/utils.sh'

results_dir = f'{workspace}/profiling_{datetime.now().strftime("%m-%d-%H-%M-%S")}'
threads = 4
runs = 1
load = True
db = f'{project_source_dir}/db'
db_backup = f'{project_source_dir}/db_backup'

workloads_dir = f'{workspace}/workloads'
workloads = ['workloadc']

# Benchmark settings
ycsb = {
    'sleepafterload' : 10,
    'maxexecutiontime' : 100,
    'operationcount' : 1_000_000_000,
    'recordcount': 2_000_000,
    'cachelib.cachesize': 200_000_000, # in bytes
    'status.interval': 1,
    'readallfields': 'false',
    'fieldcount' : 1,
    'fieldlength': 1106,
    'cachelib.pool_resizer' : 'on',
    'cachelib.tail_hits_tracking': 'on',
    'rocksdb.dbname': db,
    'rocksdb.write_buffer_size': 134217728,
    'rocksdb.max_write_buffer_number': 2,
    'rocksdb.level0_file_number_compaction_trigger': 4,
    'rocksdb.copmression': 'no',
    'rocksdb.max_background_flushes': 1,
    'rocksdb.max_background_compactions': 3,
    'rocksdb.use_direct_reads': 'true',
    'rocksdb.use_direct_io_for_flush_compaction': 'true',
    'threadcount': threads,
    'insertorder': 'nothashed',
    'requestdistribution.0' : 'uniform',
    'requestdistribution.1' : 'zipfian',
    'zipfian_const.1' : '0.99', 
    'requestdistribution.2' : 'zipfian',
    'zipfian_const.2' : '0.7',
    'requestdistribution.3' : 'zipfian',
    'zipfian_const.3' : '0.6',
}
# Proportional Share
priorities = [1, 1, 1, 1]
fraction_size = int(ycsb['recordcount'] / sum(priorities))
start = 0
end = 0 
for worker in range(threads):
    end += priorities[worker]
    ycsb[f'insertstart.{worker}'] = start*fraction_size
    ycsb[f'request_key_domain_start.{worker}'] = start*fraction_size
    ycsb[f'request_key_domain_end.{worker}'] = end*fraction_size-1
#    ycsb[f'operationcount.{worker}'] = priorities[worker]*(ycsb['operationcount'] // sum(priorities))
    ycsb[f'sleepafterload.{worker}'] = worker*ycsb['sleepafterload']
    start = end
ycsb[f'request_key_domain_end.{threads-1}'] += ycsb['recordcount'] - fraction_size*sum(priorities)


load_setup = {
    'cachelib.trail_hits_tracking': 'off',
    **ycsb   
}
setups = {
    'CacheLib' : {
        'runs' : runs,
        'result_dir': f'{results_dir}/cachelib',
        'ycsb_config' : ycsb
    },
    'CacheLib-Optimizer' : {
        'runs' : runs,
        'result_dir' : f'{results_dir}/cachelib_optimizer',
        'ycsb_config' : {
            'cachelib.pool_optimizer': 'on',
            **ycsb
        },
    },
    'CacheLib-Holpaca (Hit Ratio Maximization)' : {
        'runs' : runs,
        'controllerArgs': '1000 hit_ratio_maximization',
        'result_dir': f'{results_dir}/cachelib_holpaca_hit_ratio_maximization',
        'ycsb_config' : {
            'cachelib.holpaca': 'on',
            **ycsb
        }
    },
    'CacheLib-Holpaca (Hit Ratio Maximization with QoS guarantees)' : {
        'runs' : runs,
        'controllerArgs': '1000 hit_ratio_maximization_with_qos 1 0.2',
        'result_dir': f'{results_dir}/cachelib_holpaca_hit_ratio_maximization_with_qos',
        'ycsb_config' : {
            'cachelib.holpaca': 'on',
            **ycsb
        }
    },
}

def loadDB(workload, settings):
    settings['rocksdb.dbname'] = db_backup
    settings['rocksdb.destroy'] = 'true'
    command = f'{executable_dir}/ycsb -load -db cachelib -P {workloads_dir}/{workload} -s -threads {settings["threadcount"]} {" ".join([f"-p {k}={v}" for k,v in settings.items()])}'
    print(f'[LOAD] Running: {command}')
    subprocess.run(command, shell=True)
    print('[LOAD] Done')

def runBenchmark(workload, settings, output_file):
    print('\tLoading db backup')
    if os.path.exists(db):
        shutil.rmtree(db)
    shutil.copytree(db_backup, db)
    ycsb_settings = settings['ycsb_config']
    command = f'systemd-run --scope -p MemoryMax={ycsb_settings["cachelib.cachesize"]+20_000_000} --user {executable_dir}/ycsb -run -db cachelib -P {workloads_dir}/{workload} -s -threads {threads} {" ".join([f"-p {k}={v}" for k,v in ycsb_settings.items()])}'
    print('\tCleaning heap')
    subprocess.call([util_script, 'clean-heap'], stdout=subprocess.DEVNULL)
    controller_pid = None
    if 'controllerArgs' in settings:
        print('\tStarting controller')
        controller_pid = subprocess.Popen(f"{project_source_dir}/../build-holpaca/bin/controller {settings['controllerArgs']}", stdout=subprocess.PIPE, shell=True, preexec_fn=os.setsid).pid
    print(f'\tRunning: {command}')
    subprocess.run(command, shell=True, text=True, stdout=output_file)
    if controller_pid is not None:
        print('\tKilling controller')
        os.killpg(os.getpgid(controller_pid), signal.SIGTERM)


os.mkdir(results_dir)

report = open(f'{results_dir}/config.json', 'wt')
report.write(json.dumps(setups, indent=2))
report.close()

os.system('killall -9 controller') # Kill all controller instances to avoid coordination issues
for workload in workloads:
    if load:
        loadDB(workload, load_setup)
    for setup, settings in setups.items():
        for run in range(settings['runs']):
            print(f'[WORKLOAD: {workload}] [SETUP: {setup}] [RUN: {run+1}/{settings["runs"]}] ')
            dir = f'{settings["result_dir"]}/{run}'
            settings['ycsb_config']['cachelib.tracker'] = f'{dir}/mem.txt'
            os.makedirs(dir, exist_ok=True)
            with open(f'{dir}/ycsb.txt', 'w') as f:
                runBenchmark(workload, settings, f)
                f.close()