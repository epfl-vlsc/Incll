#!/usr/bin/env bash

remove_stuff(){ 
	rm -rf /scratch/tmp/nvm.*
	rm -rf /dev/shm/incll/nvm.*
	rm -rf *.json
}

outdir=perf_output
rm -rf $outdir
mkdir $outdir


make clean
make mttest
for WORKLOAD in ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do
	Oname=$outdir/${WORKLOAD}.csv
	echo "perf  ${Oname}"

	remove_stuff
	perf stat -o ${Oname} -r 10 -d -e instructions:u,task-clock -x, ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 -j8 --pin
	python get_average.py
	remove_stuff
	sleep 1
done

make clean
make mttest EXTFLAGS=-DSKIP_CRITICAL_SECTION
for WORKLOAD in ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do
	Oname=$outdir/${WORKLOAD}_skip.csv
	echo "perf  ${Oname}"

	remove_stuff
	perf stat -o ${Oname} -r 10 -d -e instructions:u,task-clock -x, ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 -j8 --pin
	python get_average.py
	remove_stuff
	sleep 1
done

#subtract non critical section from all
python perf_cs.py