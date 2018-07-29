#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_keys.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,Workload,Nkeys" >> ${Oname}

for WORKLOAD in rand\
	ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do
	for NKEYS in 1000 3000 10000 30000 100000 300000 1000000 3000000 10000000 30000000 100000000; do
		rm -rf *.json 
		for i in $(eval echo {1..$repeat}); do 
			rm -rf /tmp/nvm.*
			timeout 30 ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=${NKEYS} --nkeys=${NKEYS}
		done
		python get_average.py "${WORKLOAD},${NKEYS}" >> ${Oname}
	done
done
