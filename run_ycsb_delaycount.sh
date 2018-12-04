#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_delaycount.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,StdP,Workload,Delaycount" >> ${Oname}

for WORKLOAD in ycsb_a_uni ycsb_a_zipf; do
    echo "delaycount ${WORKLOAD}"
	for DELAYCOUNT in 0 340 680 1020 1360 1700 2040 2380 2720 3060 3400; do
	    echo "delaycount ${DELAYCOUNT} ${WORKLOAD}"
		rm -rf *.json
		for i in $(eval echo {1..$repeat}); do
			rm -rf /scratch/tmp/nvm.*
			rm -rf /dev/shm/incll/nvm.*
			echo "delaycount ${DELAYCOUNT} ${WORKLOAD} ${i}"
			./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 --delaycount=${DELAYCOUNT} -j8
			sleep 1
		done
		python get_average.py "${WORKLOAD},${DELAYCOUNT}" >> ${Oname}
	done
done


rm -rf /scratch/tmp/nvm.*
rm -rf /dev/shm/incll/nvm.*