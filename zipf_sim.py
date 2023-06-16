#!/bin/python3

def zipf(k, s, n):
    h = sum([ 1/pow(x,s) for x in range(1,n)])
    return (1/pow(k,s)) * (1/h)

def accZipf(s, n, percent):
    return sum([zipf(x, s, n) for x in range(1,int(percent*n))])

def printZipf(s, n, percent):
    print(f'{round(percent*100,2)}% of the dataset is accessed by {round(100*accZipf(s, n, percent),2)}% of the requests')

printZipf(0.99, 10_000, 0.01)
printZipf(0.99, 10_000, 0.115)
printZipf(0.99, 10_000, 0.178)
printZipf(0.99, 10_000, 0.239)