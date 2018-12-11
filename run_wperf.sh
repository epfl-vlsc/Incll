#!/usr/bin/env bash
WORKLOAD=$1

remove_stuff(){ 
	rm -rf /scratch/tmp/nvm.*
	rm -rf /dev/shm/incll/nvm.*
	rm -rf *.json
}

outdir=perf_output
for WORKLOAD in ycsb_a_uni; do
	Oname=$outdir/${WORKLOAD}.csv
	Odump=$outdir/${WORKLOAD}_dump.txt
	echo "perf  ${Oname}"

	remove_stuff
	
	/usr/bin/time -f "%e,,real-elapsed-time(10runs),,," perf stat -o ${Oname} -r 10 -e instructions:u,task-clock,r412e,r4f2e,r3f24,ref24 -x, ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 -j8 --pin &> ${Odump}        
	echo "read elapsed time"
	tail -1 ${Odump} >> ${Oname}
	
	python get_average.py
	remove_stuff
	sleep 1
done

