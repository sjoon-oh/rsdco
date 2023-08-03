#!/bin/bash

# Author: Sukjoon Oh, sjoon@kaist.ac.kr, github.com/sjoon-oh/
# Project rsdco
#

project_home="rsdco"
workspace_home=`basename $(pwd)`

#
# Setting proj home
if [[ ${workspace_home} != ${project_home} ]]; then
    printf "Currently in wrong directory: `pwd`\n"
    exit
fi

workspace_home=`pwd`
printf "Project dir: ${workspace_home}\n"

cd ..
workspace_par=`pwd`

printf "Sending project to all:${workspace_par}\n"
sftp -q oslab@node1:${workspace_par} <<< $"put -r ${workspace_home}"
sftp -q oslab@node2:${workspace_par} <<< $"put -r ${workspace_home}"
