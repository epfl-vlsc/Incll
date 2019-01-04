#!/usr/bin/env bash

source commons.sh

set_repeat $1
quick_make
create_output threads100M
write_csv_header_args Threads

use_default_params
use_a_workloads

NKEYS=100000000
THREAD_COUNTS=(1 4 8 16 28 32 56)
for THREADS in ${THREAD_COUNTS[@]}; do
	echo "Threads ${THREADS}"
	run_different_workloads_args ${THREADS}
done
