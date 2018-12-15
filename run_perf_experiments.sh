#!/usr/bin/env bash
echo "runnning PMasstree"

echo "run perf"
./run_perf_fullskip.sh

cd ./../MasstreeOriginal
echo "runnning Baseline"

echo "run perf"
./run_perf_fullskip.sh