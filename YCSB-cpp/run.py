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
db = f'{project_source_dir}/db'
db_backup = f'{project_source_dir}/db_backup'

workloads_dir = f'{workspace}/workloads'
workloads = ['read-only']
status = 'READ READ-PASSED'
runs = 3
threads = 4

# Benchmark settings
ycsb = {
    'sleepafterload' : 500,
    'threadcount' : threads,
    'maxexecutiontime' : 2000,
    'operationcount' : 1_000_000_000,
    'recordcount': 20_000_000,
    'cachelib.cachesize': 2_000_000_000, # in bytes
    'status.interval': 1,
    'readallfields': 'false',
    'fieldcount' : 1,
    'fieldlength': 1000,
    'cachelib.pool_resizer' : 'on',
    'cachelib.tail_hits_tracking': 'on',
    'rocksdb.dbname': db,
    'rocksdb.write_buffer_size': 134217728,
    'rocksdb.max_write_buffer_number': 2,
    'rocksdb.level0_file_number_compaction_trigger': 4,
    'rocksdb.compression': 'no',
    'rocksdb.max_background_flushes': 1,
    'rocksdb.max_background_compactions': 3,
    'rocksdb.use_direct_reads': 'true',
    'rocksdb.use_direct_io_for_flush_compaction': 'true',
    'insertorder': 'nothashed',
    'requestdistribution.0' : 'uniform',
    'requestdistribution.1' : 'zipfian',
    'zipfian_const.1' : '0.7',
    'requestdistribution.2' : 'zipfian',
    'zipfian_const.2' : '0.9',
    'requestdistribution.3' : 'zipfian',
    'zipfian_const.3' : '1.2',
    'cachelib.trail_hits_tracking': 'off',
}

def keyRangePerThread(ycsbConfig, threads, sleepAfterLoad = 0):
    fractionSize = int(ycsbConfig['recordcount'] / threads)
    for worker in range(threads):
        ycsbConfig[f'insertstart.{worker}'] = worker * fractionSize
        ycsbConfig[f'request_key_domain_start.{worker}'] = worker * fractionSize
        ycsbConfig[f'request_key_domain_end.{worker}'] = fractionSize * (worker + 1) - 1
        ycsbConfig[f'sleepafterload.{worker}'] = worker*sleepAfterLoad
    return ycsbConfig

def getPhases(ycsbConfig, threads):
    return [ycsbConfig[f'sleepafterload.{i}'] for i in range(threads)] + [ycsbConfig[f'sleepafterload.{i}'] + ycsbConfig['maxexecutiontime'] for i in range(threads)]

setups = {
#    'CacheLib' : {
#        'name': 'CacheLib',
#        'resultsDir': f'cachelib',
#        'overrideConfigs' : {}
#    },
#    'CacheLib-Optimizer' : {
#        'name': 'CacheLib-Optimizer',
#        'resultsDir' : f'cachelib_optimizer',
#        'overrideConfigs' : {
#            'cachelib.pool_optimizer': 'on',
#            'cachelib.trail_hits_tracking': 'on',
#            'cachelib.pool_resizer': 'on'
#        },
#    },
#    'CacheLib-Holpaca' : {
#        'name': 'CacheLib-Holpaca',
#        'controllerArgs': '1000 hit_ratio_maximization 0.05',
#        'resultsDir': f'cachelib_holpaca',
#        'overrideConfigs' : {
#            'cachelib.holpaca': 'on',
#            'cachelib.trail_hits_tracking': 'on',
#            'cachelib.pool_resizer': 'on'
#        }
#    },
#    'CacheLib-Holpaca (QoS: HR=0.2)' : {
#        'controllerArgs': '1000 hit_ratio_maximization_with_qos 0.05 1 0.8',
#        'name': 'CacheLib-Holpaca',
#        'resultsDir': f'cachelib_holpaca_moderate_qos',
#        'qos': {
#            '1': 0.8
#        },
#        'overrideConfigs' : {
#            'cachelib.holpaca': 'on',
#            'cachelib.trail_hits_tracking': 'on',
#        }
#    },
#    'CacheLib-Holpaca (QoS: HR=1)' : {
#        'name': 'CacheLib-Holpaca',
#        'controllerArgs': '1000 hit_ratio_maximization_with_qos 0.05 1 0',
#        'resultsDir': f'cachelib_holpaca_impossible_qos',
#        'qos': {
#            '1': 0,
#        },
#        'overrideConfigs' : {
#            'cachelib.holpaca': 'on',
#            'cachelib.trail_hits_tracking': 'on',
#        }
#    },
#    'CacheLib-Holpaca (QoS: HR=0.1)' : {
#        'name': 'CacheLib-Holpaca',
#        'controllerArgs': '1000 hit_ratio_maximization_with_qos 0.05 1 0.9',
#        'resultsDir': f'cachelib_holpaca_easy_qos',
#        'qos': {
#            '1': 0.9
#        },
#        'overrideConfigs' : {
#            'cachelib.holpaca': 'on',
#            'cachelib.trail_hits_tracking': 'on',
#        }
#    },
#    'CacheLib-Holpaca (QoS: HR=0.5)' : {
#        'name': 'CacheLib-Holpaca',
#        'controllerArgs': '1000 hit_ratio_maximization_with_qos 0.05 1 0.5',
#        'resultsDir': f'cachelib_holpaca_hard_qos',
#        'qos': {
#            '1': 0.5
#        },
#        'overrideConfigs' : {
#            'cachelib.holpaca': 'on',
#            'cachelib.trail_hits_tracking': 'on',
#        }
#    },
    'CacheLib-Holpaca (QoS: HR2=0.5 HR3=0.4)' : {
        'controllerArgs': '1000 hit_ratio_maximization_with_qos 0.05 1 0.5 2 0.6',
        'name': 'CacheLib-Holpaca',
        'resultsDir': f'cachelib_holpaca_double_qos_one',
        'qos': {
            '1': 0.5,
            '2': 0.4
        },
        'overrideConfigs' : {
            'cachelib.holpaca': 'on',
            'cachelib.trail_hits_tracking': 'on',
        }
    },
    'CacheLib-Holpaca (QoS: HR2=0.5 HR3=0.4 HR4=0.8)' : {
        'controllerArgs': '1000 hit_ratio_maximization_with_qos 0.05 1 0.5 2 0.6 3 0.2',
        'resultsDir': f'cachelib_holpaca_triple_qos',
        'name': 'CacheLib-Holpaca',
        'qos': {
            '1': 0.5,
            '2': 0.4,
            '3': 0.8
        },
        'overrideConfigs' : {
            'cachelib.holpaca': 'on',
            'cachelib.trail_hits_tracking': 'on',
        }
    },
    'CacheLib-Holpaca (QoS: HR1=0.3 HR2=0.5 HR3=0.4 HR4=0.8)' : {
        'controllerArgs': '1000 hit_ratio_maximization_with_qos 0.05 0 0.7 1 0.5 2 0.6 3 0.2',
        'resultsDir': f'cachelib_holpaca_all_qos_impossible',
        'name': 'CacheLib-Holpaca',
        'qos': {
            '0': 0.3,
            '1': 0.5,
            '2': 0.4,
            '3': 0.8
        },
        'overrideConfigs' : {
            'cachelib.holpaca': 'on',
            'cachelib.trail_hits_tracking': 'on',
        }
    },
    'CacheLib-Holpaca (QoS: HR1=0.05 HR2=0.1 HR3=0.9 HR4=0.95)' : {
        'controllerArgs': '1000 hit_ratio_maximization_with_qos 0.05 0 0.95 1 0.9 2 0.1 3 0.05',
        'resultsDir': f'cachelib_holpaca_all_qos_possible',
        'name': 'CacheLib-Holpaca',
        'qos': {
            '0': 0.05,
            '1': 0.1,
            '2': 0.9,
            '3': 0.95
        },
        'overrideConfigs' : {
            'cachelib.holpaca': 'on',
            'cachelib.trail_hits_tracking': 'on',
        }
    },
}

for setup in setups:
    setups[setup]['overrideConfigs'] = {**setups[setup]['overrideConfigs'], **keyRangePerThread(ycsb, threads, ycsb['sleepafterload'])}


config = {
    'ycsb': ycsb,
    'runs': runs,
    'setups': setups,
    'threads': threads,
    'load': True,
    'phases': getPhases(ycsb, threads),
    'load_config': { 
        **ycsb,
        'rocksdb.dbname': db_backup,
        'rocksdb.destroy': 'true'
    },
    'workloads': workloads,
}

def loadDB(workload, settings):
    command = f'{executable_dir}/ycsb -load -db cachelib -P {workloads_dir}/{workload} -threads {settings["threadcount"]} {" ".join([f"-p {k}={v}" for k,v in settings.items()])}'
    print(f'[LOAD] Running: {command}')
    subprocess.run(command, shell=True)
    print('[LOAD] Done')

def runBenchmark(workload, settings, output_file, outputDir):
    print('\tLoading db backup')
    if os.path.exists(db):
        shutil.rmtree(db)
    shutil.copytree(db_backup, db)
    ycsb_settings = {**ycsb, **settings['overrideConfigs']}
    command = f'systemd-run --scope -p MemoryMax={ycsb_settings["cachelib.cachesize"]*1.1} --user {executable_dir}/ycsb -run -db cachelib -P {workloads_dir}/{workload} -s {status} -threads {threads} {" ".join([f"-p {k}={v}" for k,v in ycsb_settings.items()])}'
    print('\tCleaning heap')
    subprocess.call([util_script, 'clean-heap'], stdout=subprocess.DEVNULL)
    controller_pid = None
    if 'controllerArgs' in settings:
        print('\tStarting controller')
        controller_pid = subprocess.Popen(f"{project_source_dir}/../build-holpaca/bin/controller {settings['controllerArgs']}", stdout=subprocess.PIPE, shell=True, preexec_fn=os.setsid).pid
    print(f'\tRunning: {command}')
    dstat_pid = subprocess.Popen(f'dstat -rcdgmn > {outputDir}/dstat.csv', shell=True, preexec_fn=os.setsid).pid
    subprocess.run(command, shell=True, text=True, stdout=output_file)
    os.killpg(os.getpgid(dstat_pid), signal.SIGTERM)
    if controller_pid is not None:
        print('\tKilling controller')
        os.killpg(os.getpgid(controller_pid), signal.SIGTERM)

os.mkdir(results_dir)
report = open(f'{results_dir}/config.json', 'wt')
report.write(json.dumps(config, indent=2))
report.close()

os.system('killall -9 controller') # Kill all controller instances to avoid coordination issues
for workload in workloads:
    if config['load']:
        loadDB(workload, config['load_config'])
    for setup, settings in setups.items():
        for run in range(runs):
            print(f'[WORKLOAD: {workload}] [SETUP: {setup}] [RUN: {run+1}/{runs}] ')
            dir = f'{results_dir}/{settings["resultsDir"]}/{run}'
            os.makedirs(dir, exist_ok=True)
            settings['overrideConfigs']['cachelib.tracker'] = f'{dir}/mem.txt'
            with open(f'{dir}/ycsb.txt', 'w') as f:
                runBenchmark(workload, settings, f, dir)
                f.close()
