#!/usr/bin/env bash

repeat=3

make mttest

for WORKLOAD in rand\
	ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do

	rm -rf *.json
	for i in $(eval echo {1..$repeat}); do 
		rm -rf /tmp/nvm.heap	
		./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000
	done
	python get_average.py
done
