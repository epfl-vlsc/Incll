#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/meta_opt.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,Workload" >> ${Oname}

for WORKLOAD in rand\
	ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do

	rm -rf *.json
	for i in $(eval echo {1..$repeat}); do 
		rm -rf /scratch/tmp/nvm.*
		timeout 100 ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000
	done
	python get_average.py ${WORKLOAD} >> ${Oname}
done