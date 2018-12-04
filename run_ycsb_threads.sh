#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_threads.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,StdP,Workload,Threads" >> ${Oname}

for WORKLOAD in ycsb_a_uni ycsb_a_zipf; do
    echo "threads ${WORKLOAD}"
	for THREADS in 1 2 4 6 8 10 12 14 16; do
	    echo "threads ${THREADS} ${WORKLOAD}"
		rm -rf *.json
		for i in $(eval echo {1..$repeat}); do 
			rm -rf /scratch/tmp/nvm.*
			rm -rf /dev/shm/incll/nvm.*
			echo "threads ${THREADS} ${WORKLOAD} ${i}"
			./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 --threads=${THREADS}
			sleep 1
		done
		python get_average.py "${WORKLOAD},${THREADS}" >> ${Oname}
	done
done

rm -rf /scratch/tmp/nvm.*
rm -rf /dev/shm/incll/nvm.*