#!/usr/bin/env python3

import csv

with open('/tmp/resizer.log') as f:
    lines = f.readlines()
    durations = map(lambda l: int(l.split(',')[2]), lines)
    durations = list(filter(lambda d: d > 0, durations))
    mean = sum(durations) / len(durations)
    variance = sum([((x - mean) ** 2) for x in durations]) / len(durations)
    res = variance ** 0.5
    print(f"{mean},{res}") 