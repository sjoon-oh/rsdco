from os import listdir
from os.path import isfile, join

import csv
import numpy

cur_files = [f for f in listdir(".") if isfile(join(".", f))]
exclude_files = ['CMakeLists.txt', 'CMakeCache.txt']

for fname in cur_files:
    if 'txt' in fname: 
        continue
    else: exclude_files.append(fname)

for fname in exclude_files:
    if fname in cur_files:
        cur_files.remove(fname)

summary = []
gnuplot_summary = []

for fname in cur_files:
    
    if ('live' in fname):
        continue

    print("Processing {}".format(fname))

    with open(fname, "r") as f:

        lines = [line.rstrip() for line in f]
        lines = (line for line in lines if line)
        lines = [int(line.rstrip()) for line in lines]
        lines.sort()

        reqs = len(lines)

        avg = numpy.mean(lines)
        avg = round(avg, 2)
        tail_1 = lines[int(round(reqs * 0.01))]
        tail_50 = lines[int(round(reqs * 0.5))]
        tail_90 = lines[int(round(reqs * 0.9))]
        tail_95 = lines[int(round(reqs * 0.95))]
        tail_99 = lines[int(round(reqs * 0.99))]
        

        summary.append({
            "file": fname,
            "reqs": reqs,
            "avg": round(avg  * 0.001, 2),
            "1th": round(tail_1 * 0.001, 2),
            "50th": round(tail_50 * 0.001, 2),
            # "90th": round(tail_90 * 0.001, 2),
            # "95th": round(tail_95 * 0.001, 2),
            "99th": round(tail_99 * 0.001, 2)
        })

        gnuplot_summary.append({
            "file": fname,
            "50th": tail_50 * 0.001,
            "1th": tail_1 * 0.001,
            "99th": tail_99 * 0.001
        })


with open("dump-analysis.csv", "w") as f:
    writer = csv.DictWriter(f, fieldnames=['file', 'reqs', 'avg', '1th', '50th', '99th'], delimiter='\t')

    writer.writeheader()
    writer.writerows(summary)

# with open("dump-gnuplot.dat", "w") as f:
#     writer = csv.DictWriter(f, fieldnames=['file', '50th', '1th', '99th'], delimiter='\t')

#     writer.writerows(gnuplot_summary)