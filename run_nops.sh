#!/usr/bin/env bash

source commons.sh

set_repeat $1
quick_make
create_output nops
write_csv_header_args Nops

use_default_params
use_all_workloads

#todo decide the range
NOPS_COUNTS=(1000000 2000000 5000000 10000000 20000000)
for NOPS in ${NOPS_COUNTS[@]}; do
	echo "Nops ${NOPS}"
	run_different_workloads_args ${NOPS}
done

