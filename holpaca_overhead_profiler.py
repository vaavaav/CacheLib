#!/usr/bin/env python3

import subprocess
import os
import shutil
import re

# Configs
workspace = os.getcwd() 

project_source_dir = f'{workspace}'
executable_dir = f'{workspace}/opt/cachelib/bin'
scripts_dir = '.'
util_script = f'{scripts_dir}/utils.sh'

results = f'{workspace}/overhead.csv'
periodicities = [0, 10, 100, 1000]
timeout = 60

# ------- 

#without holpaca

subprocess.call([util_script, 'clean-heap'])
command = [f'{executable_dir}/cachebench', f'--json_test_config={executable_dir}/../test_configs/simple_test.json', f'--timeout_seconds={timeout}']
print('Running cachebench')
print(f'Command: ' + ' '.join(command))
p = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE) # run command
output, err = p.communicate()
p.wait()
if not err:
    throughput = re.search("Total Ops : ((?:\d+)(?:\.(?:\d+))?)", str(output))
    print(f'{float(throughput.group(1))*1000000/timeout} iops')
else:
    print(err)

#with holpaca
for periodicity in periodicities:
    subprocess.call([util_script, 'clean-heap'])
    command = [f'{executable_dir}/cachebench', f'--json_test_config={executable_dir}/../test_configs/simple_test.json', f'--holpaca_periodicity={periodicity}', f'--timeout_seconds={timeout}']
    print('Running cachebench')
    print(f'Command: ' + ' '.join(command))
    p = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE) # run command
    output, err = p.communicate()
    p.wait()
    if not err:
        throughput = re.search("Total Ops : ((?:\d+)\.(?:\d+))", str(output)).group(1)
        print(f'{float(throughput)*1000000/timeout} iops')
    else:
        print(err)
	
