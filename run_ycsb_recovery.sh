#!/usr/bin/env bash
use_gdb=$1
repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_recovery.txt
rm -rf ${Oname}

echo "RecoveryTimeMicroSec,StdRecTime,StdP,Workload" >> ${Oname}

for WORKLOAD in ycsb_a_uni_recovery ycsb_a_zipf_recovery; do
    echo ${WORKLOAD}
	rm -rf *.json
	for i in $(eval echo {1..$repeat}); do 
		echo ${i}
		rm -rf /scratch/tmp/nvm.*
		rm -rf /dev/shm/incll/nvm.*
		./mttest ${WORKLOAD} --nops1=10000000 --ninitops=20000000 --nkeys=20000000 -j8
		echo -e "\n\n\n\n"
		${use_gdb} ./mttest ${WORKLOAD} --nops1=10000000 --ninitops=20000000 --nkeys=20000000 -j8
		sleep 1
	done
	python get_recovery.py "${WORKLOAD}" >> ${Oname}
done

rm -rf /scratch/tmp/nvm.*
rm -rf /dev/shm/incll/nvm.*
