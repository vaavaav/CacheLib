#!/bin/python3 

import sys
import os
import argparse
import socket

parser = argparse.ArgumentParser()
parser.add_argument("-r", "--pool-resizer", help="enable pool resizer", action="store_true")
parser.add_argument("-o", "--pool-optimizer", help="enable pool optimizer", action="store_true")
parser.add_argument("-c", "--cache-size", help="set cache size", type=int, default=2_000_000_000)
parser.add_argument("-p", "--pool", action="append", nargs=2, help="pool name and relative size") 
parser.add_argument("-l", "--log-file", help="path to log file", default='')
parser.add_argument("-i", "--ip", help="control plane registration ip address", default='localhost')

args = parser.parse_args(args=sys.argv[2:])

hostname = socket.gethostbyname( socket.gethostname())

def help():
    print('''
Usage: 
     > docker-entrypoint.py stage [OPTION]... [-p <name> <relsize>]...
       [OPTION] can be:
         -r, --pool-resizer : enable pool resizer
         -o, --pool-optimizer : enable pool optimizer
         -c, --cache-size <size> : set cache size (default 2GB)
         -i, --ip <ip> : control plane registration ip address (default localhost:5000)

       The sum of all pool sizes must be equal to 100%.

     > docker-entrypoint.py controller [OPTION]...
       [OPTION] can be:
         -l, --log-file : path to log file
''')

if len(sys.argv) < 2:
    print("Usage: docker-entrypoint.py stage")
    help()
    exit(1)
if (sys.argv[1]) == "stage":
    if args.pool is None:
        exit(0)
    for i, (poolname, _) in enumerate(args.pool):
        print(f"{poolname}: {6000+i}")
    os.system(f"{os.getcwd()}/opt/cachelib/bin/cachelib-holpaca-run-pool-caches {'on' if args.pool_resizer else 'off'} {'on' if args.pool_optimizer else 'off'} {args.cache_size} {args.ip}:5000 {' '.join([f'{poolname} {poolsize} {hostname}:{6000+i}' for i, (poolname, poolsize) in enumerate(args.pool)])}")
elif (sys.argv[1]) == "controller":
    os.system(f"./build-holpaca/bin/controller {hostname}:5000 {args.log_file}")
else:
    help()
