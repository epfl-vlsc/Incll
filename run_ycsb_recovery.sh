#!/usr/bin/env bash
use_gdb=$1
repeat=1

make mttest

mkdir -p output
Oname=output/ycsb_threads.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,StdP,Workload,Threads" >> ${Oname}

for WORKLOAD in ycsb_a_uni_recovery; do
	rm -rf *.json
	for i in $(eval echo {1..$repeat}); do 
		rm -rf /scratch/tmp/nvm.*
		rm -rf /dev/shm/incll/nvm.*
		./mttest ${WORKLOAD} --nops1=10000000 --ninitops=500000 --nkeys=500000 -j8
		echo -e "\n\n\n\n"
		${use_gdb} ./mttest ${WORKLOAD} --nops1=10000000 --ninitops=500000 --nkeys=500000 -j8
		rm -rf /tmp/nvm.*
	done
	#python get_average.py "${WORKLOAD},${THREADS}" >> ${Oname}
done
