#!/usr/bin/env bash

source commons.sh

set_repeat $1

NB_SRC=notebook-mttest.json
NB_DST=/scratch/results/incll.json

remove_files

use_default_params
use_all_workloads

make clean
make mttest "EXTFLAGS=-DPERF_WORKLOAD"
for WORKLOAD in ${WORKLOADS[@]}; do
	run_multi_experiment
done

make clean
make mttest "EXTFLAGS=-DPERF_WORKLOAD -DPERF_STORES"
for WORKLOAD in ${WORKLOADS[@]}; do
	run_multi_experiment
done

cp ${NB_SRC} ${NB_DST}