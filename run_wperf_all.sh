#!/usr/bin/env bash

remove_stuff(){ 
	rm -rf /scratch/tmp/nvm.*
	rm -rf /dev/shm/incll/nvm.*
	rm -rf *.json
}


run_perf(){
	/usr/bin/time -f "%e,,real-elapsed-time(10runs),,," perf stat -o ${Oname} -r 10 -e instructions:u,task-clock,r412e,r4f2e,r3f24,ref24 -x, ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 -j8 --pin &> ${Odump}        
	echo "read elapsed time"
	tail -1 ${Odump} >> ${Oname}
}

create_dirs(){
	outdir=perf_output
	rm -rf $outdir
	mkdir $outdir
}


#script begin
remove_stuff
create_dirs

#keep track of throughput
Ometa=$outdir/zmeta.txt
echo "TotalOps,AvgOps,StdOps,StdP,Workload" >> ${Ometa}

#full program
make clean
make mttest
for WORKLOAD in ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do
	Oname=$outdir/${WORKLOAD}.csv
	Odump=$outdir/${WORKLOAD}_dump.txt
	echo "perf  ${Oname}"

	remove_stuff
	run_perf
	python get_average.py ${WORKLOAD} >> ${Ometa}
	remove_stuff
	sleep 1
done

#skip critical part program
make clean
make mttest EXTFLAGS=-DSKIP_CRITICAL_SECTION
for WORKLOAD in ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do
	Oname=$outdir/${WORKLOAD}_skip.csv
	Odump=$outdir/${WORKLOAD}_skip_dump.txt
	echo "perf  ${Oname}"

	remove_stuff
	run_perf
	python get_average.py
	remove_stuff
	sleep 1
done

#subtract non critical section from all
python perf_cs.py