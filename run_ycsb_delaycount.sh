#!/usr/bin/env bash

repeat=10

make mttest

mkdir -p output
Oname=output/ycsb_delaycount.txt
rm -rf ${Oname}

echo "TotalOps,AvgOps,StdOps,Workload,Delaycount" >> ${Oname}

for WORKLOAD in rand\
	ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do
	for DELAYCOUNT in 0 340 680 1020 1360 1700 2040 2380 2720 3060 3400; do
		rm -rf *.json
		for i in $(eval echo {1..$repeat}); do
			rm -rf /tmp/nvm.*
			timeout 30 ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 --delaycount=${DELAYCOUNT}
		done
		python get_average.py "${WORKLOAD},${DELAYCOUNT}" >> ${Oname}
	done
done
