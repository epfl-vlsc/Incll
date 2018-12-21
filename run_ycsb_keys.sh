#!/usr/bin/env bash

source commons.sh

set_repeat $1
quick_make
create_output keys
write_csv_header_args Keys

use_default_params
use_a_workloads

#todo decide the range
NKEYS_COUNTS=(10000 30000 100000 300000 1000000 3000000 10000000 30000000 100000000)
for NKEYS in ${NKEYS_COUNTS[@]}; do
	echo "Keys ${NKEYS}"
	run_different_workloads_args ${NKEYS}
done

