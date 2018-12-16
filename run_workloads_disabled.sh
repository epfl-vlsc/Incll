#!/usr/bin/env bash

source commons.sh

set_repeat $1
full_make EXTFLAGS=-DDISABLE_ALL
create_output workloads_disabled
write_csv_header

use_default_params
use_all_workloads

run_different_workloads

