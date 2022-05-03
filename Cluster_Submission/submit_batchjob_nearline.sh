#!/bin/bash
##Read in list of files to run
#LISTFILE="files_188.txt"
LISTFILE="/lustre/gamma/cjones/DESPEC_S450_MHTDC/Cluster_Submission/files_dryrun_degas.txt"
declare -a size
while IFS= read -r line
do
    size+=($line)
done < "$LISTFILE"

##Submit job

sbatch -J despec_go4_s452am -D /lustre/gamma/cjones/DESPEC_S450_MHTDC/ -o logs/go4_%A_%a.out.log -e logs/go4_%A_%a.err.log \
  --time=8:00:00 --mem-per-cpu=4G \
  --array=0-${#size[@]}:2 -- /lustre/gamma/cjones/DESPEC_S450_MHTDC/Cluster_Submission/go4_launcher_nearline.sh

  unset size
