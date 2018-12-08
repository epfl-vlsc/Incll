#!/usr/bin/env bash
WORKLOAD=$1

remove_stuff(){ 
	rm -rf /scratch/tmp/nvm.*
	rm -rf /dev/shm/incll/nvm.*
	rm -rf *.json
}

make mttest
remove_stuff
perf stat -d -e instructions:u,task-clock ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 -j8
python get_average.py
remove_stuff
sleep 1
