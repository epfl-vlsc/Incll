#!/usr/bin/env bash

source commons.sh

set_repeat $1
quick_make
create_output threads
write_csv_header_args Threads

use_default_params
use_all_workloads

THREAD_COUNTS=(1 4 8 12 16 20 24 28 32 36 40 44 48 52 56)
for THREADS in ${THREAD_COUNTS[@]}; do
	echo "Threads ${THREADS}"
	run_different_workloads_args ${THREADS}
done
