#!/bin/bash

nthreads="
3
"

key_len=16
payload_len="
32
64
128
256
512
1024
2048
"

export HARTEBEEST_PARTICIPANTS=0,1,2
export HARTEBEEST_EXC_IP_PORT=143.248.39.61:9999
export HARTEBEEST_CONF_PATH=./rdsco.json
export HARTEBEEST_NID=$1

# ./build/bin/rsdco-standalone

for nthreads in $nthreads; do
    for payload in $payload_len; do
        echo "./build/bin/rsdco-standalone $payload $key_len $nthreads"
        numactl --membind 0 ./build/bin/rsdco-standalone $payload $key_len $nthreads

        cat dump-0.txt > dump-aggr.txt
        cat dump-1.txt >> dump-aggr.txt

        mv dump-0.txt dump-rpli-$payload-$key_len-$nthreads.txt # Replication Latency
        mv dump-1.txt dump-chkr-$payload-$key_len-$nthreads.txt # Checker Latency
        mv dump-2.txt dump-rpli-core-$payload-$key_len-$nthreads.txt # Replication Core Latency
        mv dump-3.txt dump-chkr-core-$payload-$key_len-$nthreads.txt # Checker Core Latency

        mv dump-aggr.txt dump-aggr-$payload-$key_len-$nthreads.txt

        mv dump-4.txt dump-realtime-$payload-$key_len-$nthreads.txt
    done
done

python3 analysis-dump.py