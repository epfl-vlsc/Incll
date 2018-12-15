#!/usr/bin/env bash

source commons.sh

set_repeat $1
quick_make
create_output workloads
write_csv_header

use_default_params
use_all_workloads

run_different_workloads

