#!/usr/bin/env bash

source commons.sh

set_repeat $1
quick_make
create_output workloads
write_csv_header

use_default_params
use_all_workloads


OUTPERF=output/workload_perf.txt
python get_perf_average.py ${OUTPERF} > ${OUTPERF} 
echo "Perf Results in ${OUTPERF}"
 

for WORKLOAD in ${WORKLOADS[@]}; do
	remove_json_out
	run_multi_experiment
	get_average_results
	python get_perf_average.py ${WORKLOAD} >> ${OUTPERF}
done

