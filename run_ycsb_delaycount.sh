#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_delaycount.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,StdP,Workload,Delaycount" >> ${Oname}

for WORKLOAD in ycsb_a_uni ycsb_a_zipf; do
    echo ${WORKLOAD}
	for DELAYCOUNT in 0 340 680 1020 1360 1700 2040 2380 2720 3060 3400; do
	    echo ${DELAYCOUNT}
		rm -rf *.json
		for i in $(eval echo {1..$repeat}); do
			rm -rf /scratch/tmp/nvm.*
			rm -rf /dev/shm/incll/nvm.*
			echo ${i}
			timeout 100 ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 --delaycount=${DELAYCOUNT}
			sleep 1
		done
		python get_average.py "${WORKLOAD},${DELAYCOUNT}" >> ${Oname}
	done
done
