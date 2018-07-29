#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_nops.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,Workload,Nops" >> ${Oname}

for WORKLOAD in rand\
	ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do
	for NOPS in 100 1000 10000 100000 1000000 1000000; do 
		rm -rf *.json
		for i in {1..${repeat}}; do 
			rm -rf /tmp/nvm.*
			timeout 20 ./mttest ${WORKLOAD} --nops1=${NOPS} --ninitops=20000000 --nkeys=20000000
		done
		python get_average.py "${WORKLOAD},${NOPS}" >> ${Oname}
	done
done
