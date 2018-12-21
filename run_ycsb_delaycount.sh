#!/usr/bin/env bash

source commons.sh

set_repeat $1
quick_make
create_output delay
write_csv_header_args Delay

use_default_params
use_a_workloads

DELAYS_COUNTS=(0 260 520 780 1040 1300 1560 1820 2080 2340 2600)
for DELAYS in ${DELAYS_COUNTS[@]}; do
	echo "Delays ${DELAYS}"
	run_different_workloads_args ${DELAYS}
done
