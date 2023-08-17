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
    nrecords = len(lines)
    total_time = numpy.sum(lines) / pow(10, 9)

    print("Number of Records: {}".format(nrecords))
    print("Final End Time {} sec".format(total_time))

    print("Throughput: {} ops/sec".format(nrecords / total_time))
