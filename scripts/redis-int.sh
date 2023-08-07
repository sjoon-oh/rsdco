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
redis_home="${exp_home}/redis-7.0.5"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${workspace_home}/lib
export LIBRARY_PATH=${workspace_home}/lib:$LIBRARY_PATH

#
# Option 0: clean source directory, and the binary.
if [[ "${args}" == *"clean"* ]]; then
    printf "${normalc}0. -- Redis source files clean. -- \n"

    if [ -d "${redis_home}" ]; then
        printf "${normalc}Removing existing workspace.\n"
        rm -rf ${redis_home}
    fi

    if [ -d "${workspace_home}/bin/redis-newcons" ]; then
        printf "${normalc}Removing existing Redis binary.\n"
        rm -f ${workspace_home}/bin/redis-newcons
    fi

   exit
fi

#
# Option 1: reset source directory
if [[ "${args}" == *"reset"* ]]; then
    printf "${normalc}1. -- Redis source files reset. -- \n"

    if [ -d "${redis_home}" ]; then
        printf "${normalc}Removing existing workspace.\n"
        rm -rf ${redis_home}
    fi

    cd ${exp_home}
    tar -xf redis-7.0.5.tar.gz
    
    cd redis-7.0.5
    git init

    printf "[user]\n\tname = local\n\temail = local@oslab.kaist.ac.kr" >> .git/config
    git add ./* --verbose

    git commit -m "redis-7.0.5 untocuhed: initial source" --verbose
fi

cd ${redis_home}

#
# Option 2: Make patch to the source directory
if [[ "${args}" == *"checkpoint"* ]]; then
    printf "${normalc}2. -- Generating modified patch. -- \n"

    if [ ! -d "${redis_home}" ]; then
        printf "${warning}No Redis source directory found.\n"
        exit
    fi
    
    git diff > ../redis-7.0.5.patch
    cd ..
    printf `ls | grep patch`
fi

#
# Option 3: Apply the patch.
if [[ "${args}" == *"patch"* ]]; then
    printf "${normalc}3. -- Applying the patch to the source. -- \n"
    printf "${normalc}Current directory: ${redis_home} \n"

    if [ ! -d "${redis_home}" ]; then
        printf "${warning}No Redis source directory found.\n"
        exit
    fi
    if [ ! -f "../redis-7.0.5.patch" ]; then
        printf "${warning}No Redis patch file found.\n"
        exit
    fi
    
    # patch -p0 -d ${redis_home} -s < ../redis-7.0.5.patch
    git apply ../redis-7.0.5.patch
    printf "${normalc}Patched\n."
fi