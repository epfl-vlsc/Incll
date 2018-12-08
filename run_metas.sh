#!/usr/bin/env bash
echo "runnning PMasstree"
make clean
rm -rf output

echo "run meta"
./run_ycsb_keys.sh

cd ./../MasstreeOriginal
echo "runnning Baseline"
make clean
rm -rf output

echo "run meta"
./run_ycsb_keys.sh
