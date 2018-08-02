#!/usr/bin/env bash
echo "runnning all experiments"

./run_meta_opt.sh
./run_ycsb_keys.sh
./run_ycsb_threads.sh
./run_ycsb_delaycount.sh

cd /home/aksun/git/MasstreeOriginal

./run_meta_opt.sh
./run_ycsb_keys.sh
./run_ycsb_threads.sh
./run_ycsb_delaycount.sh
