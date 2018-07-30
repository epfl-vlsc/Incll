#!/usr/bin/env bash

repeat=1

make mttest

mkdir -p output
Oname=output/ycsb_threads.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,Workload,Threads" >> ${Oname}

for WORKLOAD in ycsb_a_uni_recovery; do
	rm -rf *.json
	for i in $(eval echo {1..$repeat}); do 
		rm -rf /tmp/nvm.*
		./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000
		echo -e "\n\n\n\n"
		./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000
		rm -rf /tmp/nvm.*
	done
	#python get_average.py "${WORKLOAD},${THREADS}" >> ${Oname}
done
