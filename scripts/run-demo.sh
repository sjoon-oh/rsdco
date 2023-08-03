#!/bin/bash
#
# github.com/sjoon-oh/hartebeest
# Author: Sukjoon Oh, sjoon@kaist.ac.kr
# run-demo.sh

project_home="rsdco"
workspace_home=`basename $(pwd)`

warning='\033[0;31m[WARNING]\033[0m '
normalc='\033[0;32m[MESSAGE]\033[0m '

#
# Setting proj home
if [[ ${workspace_home} != ${project_home} ]]; then
    printf "${warning}Currently in wrong directory: `pwd`\n"
    exit
fi

blanc=143.248.231.40

export HARTEBEEST_PARTICIPANTS=0,1
export HARTEBEEST_NID=0
export HARTEBEEST_EXC_IP_PORT=143.248.39.61:9999
export HARTEBEEST_CONF_PATH=./rdsco.json

# ssh blanc "ps -eaf" | grep memcached | grep -v grep | awk '{print $2}'
# if ()


# ./build/bin/rsdco-demo
numactl --membind 0 ./build/bin/rsdco-demo