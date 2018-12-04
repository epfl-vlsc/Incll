#!/usr/bin/env bash
echo "runnning PMasstree"
make clean

echo "run meta"
./run_meta_opt.sh

echo "run keys"
./run_ycsb_keys.sh

echo "run threads"
./run_ycsb_threads.sh

echo "run delaycount"
./run_ycsb_delaycount.sh

echo "run nops"
./run_nops.sh
