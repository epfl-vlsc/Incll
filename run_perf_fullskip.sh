#!/usr/bin/env bash

source commons.sh

set_repeat $1
reset_perf_dirs
create_perf_output workloads
write_csv_header

use_default_params
use_all_workloads

#full program
full_make
for WORKLOAD in ${WORKLOADS[@]}; do
	Oname=${OUTDIR}/${WORKLOAD}.csv
	Odump=${OUTDIR}/${WORKLOAD}_dump.txt
	echo "perf ${Oname}"

	remove_files
	run_perfed_experiment
	get_average_results
	remove_files
	sleep 1
done

#skip critical part program

full_make EXTFLAGS=-DSKIP_CRITICAL_SECTION
for WORKLOAD in ${WORKLOADS[@]}; do
	Oname=${OUTDIR}/${WORKLOAD}_skip.csv
	Odump=${OUTDIR}/${WORKLOAD}_skip_dump.txt
	echo "perf ${Oname}"

	remove_files
	run_perfed_experiment
	remove_files
	sleep 1
done
#subtract non critical section from all
python perf_cs.py