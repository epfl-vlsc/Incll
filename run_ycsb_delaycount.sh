#!/usr/bin/env bash

source commons.sh

set_repeat $1
quick_make
create_output delay
write_csv_header_args Delay

use_default_params
use_all_workloads

DELAYS_COUNTS=(0 340 680 1020 1360 1700 2040 2380 2720 3060 3400)
for DELAYS in ${DELAYS_COUNTS[@]}; do
	echo "Delays ${DELAYS}"
	run_different_workloads_args ${DELAYS}
done
