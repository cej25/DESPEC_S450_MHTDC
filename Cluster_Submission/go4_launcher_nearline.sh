#!/bin/bash

##Setup environment
source /cvmfs/eel.gsi.de/centos7-x86_64/bin/go4login
#source /cvmfs/csee.gsi.de/bin/go4login
export ROOT_INCLUDE_PATH=/lustre/gamma/cjones/DESPEC_S450_MHTDC
echo "DESPEC Kronos Started at `date`"

##Set data location
#dpath=~/lustre/gamma/d004/ts/aida/

##Read in list of files to run. Format names seperated by space,tab,newline

LISTFILE="/lustre/gamma/cjones/DESPEC_S450_MHTDC/Cluster_Submission/files_dryrun_degas.txt"

##Count number of files
NFILES=$(cat ${LISTFILE} | wc -l)
echo "Analysing" $NFILES "Files"

##Read names from list file
declare -a array
while IFS= read -r line
do
    array+=($line)
done < "$LISTFILE"

echo "Array is $SLURM_ARRAY_TASK_ID"
part=(  "${array[@]:$SLURM_ARRAY_TASK_ID:5}" ) # :5 number of files to put together -> Has to be the same in the 2 .sh scripts

echo "Running Go4!"
go4analysis -file ${part[*]} -enable-asf 1800 -asf /lustre/gamma/cjones/DESPEC_S450_MHTDC/Cluster_Submission/Nearline_Histograms/DRYRUN_DEGAS_$SLURM_ARRAY_TASK_ID.root
