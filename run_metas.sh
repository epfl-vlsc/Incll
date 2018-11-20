#!/usr/bin/env bash
echo "runnning PMasstree"
make clean
rm -rf output

echo "run meta"
./run_meta_opt.sh 2

cd ./../MasstreeOriginal
echo "runnning Baseline"
make clean
rm -rf output

echo "run meta"
./run_meta_opt.sh 2
