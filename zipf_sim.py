#!/bin/python3
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import PchipInterpolator


def zipf(rank: int, zipfian_coeficient: float, number_of_ranks: int):
    return 1 / (
        pow(rank, zipfian_coeficient)
        * sum([1 / pow(i, zipfian_coeficient) for i in range(1, number_of_ranks)])
    )


def accZipf(to_rank: int, zipfian_coeficient: float, number_of_ranks: int):
    return sum(
        [zipf(rank, zipfian_coeficient, number_of_ranks) for rank in range(1, to_rank)]
    )


def printAccZipf(
    percentage_of_the_ranks: float, zipfian_coeficient: float, number_of_ranks: int
):
    to_rank = int(percentage_of_the_ranks * number_of_ranks)
    percentage_of_the_requests = round(
        100 * accZipf(to_rank, zipfian_coeficient, number_of_ranks), 2
    )
    print(
        f"{round(percentage_of_the_ranks*100,2)}% of the dataset is accessed by {percentage_of_the_requests}% of the requests"
    )

number_of_ranks = 10_000
zipfs = [0.55, 0.8, 0.99, 1.2]
percents = [0, 0.2, 0.4, 0.6, 0.8, 1]
xs = np.arange(0.0, 1.01, 0.01) 
results = {z:[] for z in zipfs}

for z in zipfs:
    for p in percents:
        results[z].append((p, accZipf(int(p * number_of_ranks), z, number_of_ranks)))


plt.figure(figsize=(10, 5))
for p in percents:
    plt.axvline(p, color='black', linestyle='--',linewidth=0.25)
    plt.axhline(p, color='black', linestyle='--',linewidth=0.25)

for series, data in results.items():
    x,y = zip(*data)
    cs = PchipInterpolator(y, x)
    ys = cs(xs)
    plt.xticks(percents, [f'{int(x*100)}%' for x in percents])
    plt.yticks(percents, [f'{int(x*100)}%' for x in percents])
    plt.plot(xs,ys,label=f'Zipf (\u03B1: {series})')

plt.plot(xs, xs, label=f'Uniform')


plt.legend()
plt.margins(0.1)
plt.title(f'Request distributions ({number_of_ranks} items)')
plt.ylabel('Percentage of the dataset')
plt.xlabel('Percentage of the requests')
plt.savefig('zipf_sim.png', bbox_inches='tight')
plt.close()

