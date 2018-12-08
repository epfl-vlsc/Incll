#!/usr/bin/env bash
echo "runnning PMasstree"

echo "run perf"
./run_wperf_all.sh

cd ./../MasstreeOriginal
echo "runnning Baseline"

echo "run perf"
./run_wperf_all.sh