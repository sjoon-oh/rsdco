#!/bin/bash
# Collaborator: Sukjoon Oh, sjoon@kaist.ac.kr, github.com/sjoon-oh/
# Project newcons
#


# Integration Redis script.

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
exp_home="${workspace_home}/experiments"
memc_home="${exp_home}/memcached-1.5.19"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${workspace_home}/build/lib
export LIBRARY_PATH=${workspace_home}/build/lib:$LIBRARY_PATH

cp hartebeest/build/lib/libhartebeest.so ./build/lib/

#
# Option 0: clean source directory, and the binary.
if [[ "${args}" == *"clean"* ]]; then
    printf "${normalc}0. -- Memcached source files clean. -- \n"

    if [ -d "${memc_home}" ]; then
        printf "${normalc}Removing existing workspace.\n"
        rm -rf ${memc_home}
    fi

    if [ -d "${workspace_home}/build/bin/rsdco-memcached" ]; then
        printf "${normalc}Removing existing Memcached binary.\n"
        rm -f ${workspace_home}/build/bin/rsdco-memcached
    fi

   exit
fi

#
# Option 1: reset source directory
if [[ "${args}" == *"reset"* ]]; then
    printf "${normalc}1. -- Memcached source files reset. -- \n"

    if [ -d "${memc_home}" ]; then
        printf "${normalc}Removing existing workspace.\n"
        rm -rf ${memc_home}
    fi

    cd ${exp_home}
    tar -xf memcached-1.5.19.tar.gz
    
    cd memcached-1.5.19
    git init

    printf "[user]\n\tname = local\n\temail = local@oslab.kaist.ac.kr" >> .git/config
    git add ./* --verbose

    git commit -m "memcached-1.5.19 untouched: initial source" --verbose
fi

cd ${memc_home}

#
# Option 2: Make patch to the source directory
if [[ "${args}" == *"checkpoint"* ]]; then
    printf "${normalc}2. -- Generating modified patch. -- \n"

    if [ ! -d "${memc_home}" ]; then
        printf "${warning}No Memcached source directory found.\n"
        exit
    fi
    
    git diff > ../memcached-1.5.19.patch
    cd ..
    printf `ls | grep patch`
fi

#
# Option 3: Apply the patch.
if [[ "${args}" == *"patch"* ]]; then
    printf "${normalc}3. -- Applying the patch to the source. -- \n"
    printf "${normalc}Current directory: ${memc_home} \n"

    if [ ! -d "${memc_home}" ]; then
        printf "${warning}No Memcached source directory found.\n"
        exit
    fi
    if [ ! -f "../memcached-1.5.19.patch" ]; then
        printf "${warning}No Memcached patch file found.\n"
        exit
    fi
    
    # patch -p0 -d ${redis_home} -s < ../redis-7.0.5.patch
    git apply --reject --whitespace=fix ../memcached-1.5.19.patch
    printf "${normalc}Patched\n."
fi

#
# Option 4: Apply the patch.
if [[ "${args}" == *"make"* ]]; then
    printf "${normalc}3. -- Building Memcached -- \n"
    printf "${normalc}Current directory: ${memc_home} \n"

        if [ ! -d "${memc_home}" ]; then
        printf "${warning}No Memcached source directory found.\n"
        exit
    fi

    ./configure
    sed -i "s|LIBS = -lhugetlbfs -levent|LIBS = -lhugetlbfs -levent\nLIBS += -lrsdco -lhartebeest -L ${workspace_home}/build/lib|" Makefile

    make -j 16
    cp memcached ${workspace_home}/build/bin/rsdco-memcached
fi

#
# Option 5: Apply the patch.
if [[ "${args}" == *"run"* ]]; then
    printf "${normalc}3. -- Executing Memcached -- \n"
    printf "${normalc}Current directory: ${memc_home} \n"

    if [ ! -f "${workspace_home}/build/bin/rsdco-memcached" ]; then
        printf "${warning}No Memcached binary file found. Did you compile?\n"
        exit
    fi

    export HARTEBEEST_PARTICIPANTS=0,1,2
    export HARTEBEEST_EXC_IP_PORT=143.248.39.61:9999
    export HARTEBEEST_CONF_PATH=${workspace_home}/rdsco.json

    cd ${workspace_home}

    ${workspace_home}/build/bin/rsdco-memcached \
        -p 6379 \
        -t 1 \
        -m 32768
        
    mv dump-0.txt dump-memcached-rpli.txt
    mv dump-1.txt dump-memcached-chkr.txt
    mv dump-2.txt dump-memcached-rpli-core.txt
    mv dump-3.txt dump-memcached-chkr-core.txt
    mv dump-4.txt dump-memcached-realtime.txt

    mv dump-*txt dumps
    cd dumps

    python3 analysis-dump.py
fi