#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_nops.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,StdP,Workload,Nops" >> ${Oname}

for WORKLOAD in ycsb_a_uni ycsb_a_zipf; do
    echo "nops ${WORKLOAD}"
	for NOPS in 1000000 2000000 5000000 10000000 20000000; do
	    echo "nops ${NOPS} ${WORKLOAD}"
		rm -rf *.json
		for i in $(eval echo {1..$repeat}); do 
			rm -rf /scratch/tmp/nvm.*
			rm -rf /dev/shm/incll/nvm.*
			echo "nops ${NOPS} ${WORKLOAD} ${i}"
			./mttest ${WORKLOAD} --nops1=${NOPS} --ninitops=20000000 --nkeys=20000000 -j8 --pin
			sleep 1
		done
		python get_average.py "${WORKLOAD},${NOPS}" >> ${Oname}
	done
done

rm -rf /scratch/tmp/nvm.*
rm -rf /dev/shm/incll/nvm.*