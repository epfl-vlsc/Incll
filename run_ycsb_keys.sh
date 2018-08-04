#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_keys.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,StdP,Workload,Nkeys" >> ${Oname}

for WORKLOAD in ycsb_a_uni ycsb_a_zipf; do
	echo "tree size ${WORKLOAD}"
	for NKEYS in 1000 3000 10000 30000 100000 300000 1000000 3000000 10000000 30000000 100000000; do
		echo "tree size ${NKEYS} ${WORKLOAD}"
		rm -rf *.json
		for i in $(eval echo {1..$repeat}); do
		    rm -rf /scratch/tmp/nvm.*
		    rm -rf /dev/shm/incll/nvm.*
		    echo "tree size ${NKEYS} ${WORKLOAD} ${i}"
			timeout 100 ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=${NKEYS} --nkeys=${NKEYS}
			sleep 1
		done
		python get_average.py "${WORKLOAD},${NKEYS}" >> ${Oname}
	done
done