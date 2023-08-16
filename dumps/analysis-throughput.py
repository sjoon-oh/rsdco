from os import listdir
from os.path import isfile, join

import csv
import numpy

import sys

fname = sys.argv[1]

if (len(sys.argv) != 2):
    exit()

acc = []

with open(fname, "r") as f:

    lines = [line.rstrip() for line in f]
    lines = (line for line in lines if line)
    lines = [int(line.rstrip()) for line in lines]

    # cnt = 1
    # for line in lines:
    #     acc.append((cnt / line) * pow(10, 9))
    #     cnt = cnt + 1

    print("Final End Time {}".format(sum(acc) / pow(10, 9)))
