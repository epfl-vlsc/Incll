#!/usr/bin/env bash

source commons.sh

WORKLOAD=$1
set_repeat 1

quick_make
use_default_params

remove_files
run_perf_simple_experiment
remove_files