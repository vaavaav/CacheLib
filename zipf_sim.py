#!/bin/python3


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

zipfian_coeficient = 0.8
number_of_ranks = 10_000

printAccZipf(0.01,  zipfian_coeficient, number_of_ranks)
printAccZipf(0.115, zipfian_coeficient, number_of_ranks)
printAccZipf(0.155, zipfian_coeficient, number_of_ranks)
printAccZipf(0.178, zipfian_coeficient, number_of_ranks)
printAccZipf(0.239, zipfian_coeficient, number_of_ranks)
printAccZipf(0.602, zipfian_coeficient, number_of_ranks)
