#!/bin/bash
# Collaborator: Sukjoon Oh, sjoon@kaist.ac.kr, github.com/sjoon-oh/
# Project newcons
#

project_home="rsdco"
workspace_home=`basename $(pwd)`

warning='\033[0;31m[WARNING]\033[0m '
normalc='\033[0;32m[MESSAGE]\033[0m '

args=$@
printf "${normalc}Arguments: ${args}\n"

#
# Setting proj home
if [[ ${workspace_home} != ${project_home} ]]; then
    printf "${warning}Currently in wrong directory: `pwd`\n"
    exit
fi

workspace_home=`pwd`
exp_home="${workspace_home}/experiments/rc-craq"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${workspace_home}/build/lib
export LIBRARY_PATH=${workspace_home}/build/lib:$LIBRARY_PATH

cp hartebeest/build/lib/libhartebeest.so ./build/lib/

cd ${exp_home}

#
# Option 4: Apply the patch.
if [[ "${args}" == *"make"* ]]; then
    printf "${normalc}3. -- Building RC-CRAQ -- \n"
    printf "${normalc}Current directory: ${exp_home} \n"

    g++ -o rsdco-rc-craq rc_craq.cc -lhartebeest -lrsdco -lpthread
    mv rsdco-rc-craq ../../build/bin
fi

#
# Option 5: Apply the patch.
if [[ "${args}" == *"run"* ]]; then
    printf "${normalc}3. -- Executing RC-CRAQ -- \n"
    printf "${normalc}Current directory: ${redis_home} \n"

    if [ ! -f "${workspace_home}/build/bin/rsdco-rc-craq" ]; then
        printf "${warning}No binary file found. Did you compile?\n"
        exit
    fi

    export HARTEBEEST_PARTICIPANTS=0,1,2
    export HARTEBEEST_EXC_IP_PORT=143.248.39.61:9999
    export HARTEBEEST_CONF_PATH=${workspace_home}/rdsco.json

    cd ${workspace_home}

    ${workspace_home}/build/bin/rsdco-rc-craq
    
    mv dump-0.txt dump-redis-rpli.txt
    mv dump-1.txt dump-redis-chkr.txt
    mv dump-2.txt dump-redis-rpli-core.txt
    mv dump-3.txt dump-redis-chkr-core.txt
    mv dump-4.txt dump-redis-realtime.txt

    python3 analysis-dump.py
fi