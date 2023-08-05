#!/bin/bash

nthreads="
1
2
4
8
16
"

key_len=16
payload_len="
32
64
128
256
512
"

./build/bin/rsdco-standalone

for nthreads in $nthreads; do
    for payload in $payload_len; do
        echo "./build/bin/rsdco-standalone $payload $key_len $nthreads"
        numactl --membind 0 ./build/bin/rsdco-standalone $payload $key_len $nthreads

        cat dump-0.txt > dump-2.txt
        cat dump-1.txt >> dump-2.txt

        mv dump-0.txt dump-rpli-$payload-$key_len-$nthreads.txt
        mv dump-1.txt dump-chkr-$payload-$key_len-$nthreads.txt
        mv dump-2.txt dump-aggr-$payload-$key_len-$nthreads.txt
    done
done

python3 analysis-dump.py