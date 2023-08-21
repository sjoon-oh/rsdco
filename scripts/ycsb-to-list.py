from os import listdir
from os.path import isfile, join

import csv
import numpy

cur_files = [f for f in listdir(".") if isfile(join(".", f))]
csv_files = []

print(f"Current Files: {cur_files}")

for fname in cur_files:
    if (fname.split('.')[-1] == 'csv'):
        csv_files.append(fname)

if ("dump-ycsb-outs.csv" in csv_files):
    csv_files.remove("dump-ycsb-outs.csv")

obj = {}
summary_objs = []

for fname in csv_files:
    with open(fname, "r") as f:
        lines = [line.rstrip() for line in f]

        print(f"Processing: {fname}")
        obj["filename"] = fname
        for line in lines:
            line = line.split(',')
            obj[f"{line[0]} {line[1]}"] = line[2]

        summary_objs.append(obj)
        obj = {}

with open("dump-ycsb-outs.csv", "w") as f:

    fieldname = summary_objs[0].keys()
    writer = csv.DictWriter(f, fieldnames=fieldname, delimiter='\t')

    writer.writeheader()
    writer.writerows(summary_objs)


