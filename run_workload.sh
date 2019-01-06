#!/usr/bin/env bash
source commons.sh

#ex:$1=ycsb_a_uni

WORKLOADS=($1)

set_repeat $2
full_make
create_output workload
write_csv_header

use_default_params

run_different_workloads
read_out