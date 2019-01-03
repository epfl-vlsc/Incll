use_all_workloads(){ 
	WORKLOADS=(ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf)
}

use_ab_workloads(){
	WORKLOADS=(ycsb_a_uni ycsb_b_uni ycsb_a_zipf ycsb_b_zipf)
}

use_a_workloads(){
	WORKLOADS=(ycsb_a_uni ycsb_a_zipf)
}

use_default_params(){ 
	NKEYS=20000000
	THREADS=8
	NOPS=1000000
	DELAYS=0
}

create_output(){
	#$1=filename
	mkdir -p output
	OUTFILE=output/$1.txt
	rm -rf ${OUTFILE}
	echo "Results in ${OUTFILE}"
}

reset_perf_dirs(){ 
	OUTDIR=perf_output
	rm -rf $OUTDIR
	mkdir $OUTDIR
	echo "Result files in ${OUTDIR}"
}

create_perf_output(){
	#$1=filename
	OUTFILE=$OUTDIR/$1.txt
	rm -rf ${OUTFILE}
	echo "Results in ${OUTFILE}"
}

full_make(){
	#ex:$1=EXTFLAGS=-DSKIP_CRITICAL_SECTION or EXTFLAGS=-DDISABLE_ALL
	echo "clean then make mttest"
	make clean
	make mttest $1
}

quick_make(){ 
	echo "make mttest"
	make mttest
}

remove_nvm_files(){ 
	rm -rf /scratch/tmp/nvm.*
    rm -rf /dev/shm/incll/nvm.*
}

remove_json_out(){
	rm -rf *.json
}

remove_files(){ 
	remove_nvm_files
	remove_json_out
}

write_csv_header_args(){
	#ex: $1=Nkeys
	#requires OUTFILE
	echo "TotalOps,AvgOps,StdOps,StdPOps,Workload,$1" >> ${OUTFILE}
}

write_csv_header(){
	#requires OUTFILE
	echo "TotalOps,AvgOps,StdOps,StdPOps,Workload" >> ${OUTFILE}
}

run_single_experiment(){
	echo "Workload:${WORKLOAD} keys and initops:${NKEYS} NOPS:${NOPS} threads:${THREADS} delay:${DELAYS} pinned"
	./mttest ${WORKLOAD} --nops1=${NOPS} --ninitops=${NKEYS} --nkeys=${NKEYS} --threads=${THREADS} --delaycount=${DELAYS} --pin
	sleep 1
}

run_perfed_experiment(){
	echo "Workload:${WORKLOAD} keys and initops:${NKEYS} NOPS:${NOPS} threads:${THREADS} pinned"
	/usr/bin/time -f "%e,,real-elapsed-time(10runs),,,,," perf stat -d -o ${Oname} -r ${REPEAT} -e instructions:u,task-clock -x, ./mttest ${WORKLOAD} --nops1=${NOPS} --ninitops=${NKEYS} --nkeys=${NKEYS} --threads=${THREADS} --pin &> ${Odump}        
	echo "read elapsed time"
	tail -1 ${Odump} >> ${OUTFILE}
}

run_perf_simple_experiment(){
	echo "Workload:${WORKLOAD} keys and initops:${NKEYS} NOPS:${NOPS} threads:${THREADS} pinned"
	perf stat -d -r ${REPEAT} -e instructions:u,task-clock -x, ./mttest ${WORKLOAD} --nops1=${NOPS} --ninitops=${NKEYS} --nkeys=${NKEYS} --threads=${THREADS} --pin     
}

get_average_results(){ 
	#requires OUTFILE
	python get_average.py "${WORKLOAD}" >> ${OUTFILE}
}

get_average_results_args(){ 
	#ex: $1=${NKEYS}
	#requires OUTFILE
	python get_average.py "${WORKLOAD},$1" >> ${OUTFILE}
}

read_out(){
	cat ${OUTFILE}
}

remove_out(){
	rm ${OUTFILE}
}

run_multi_experiment(){
	#requires REPEAT, WORKLOAD
	for i in $(eval echo {1..$REPEAT}); do
	    remove_nvm_files
		run_single_experiment
		remove_nvm_files
		sleep 1
	done
}

run_different_workloads(){ 
	for WORKLOAD in ${WORKLOADS[@]}; do
		remove_json_out
		run_multi_experiment
		get_average_results
	done
}

run_different_workloads_args(){
	#ex:$1=${NKEYS}
	for WORKLOAD in ${WORKLOADS[@]}; do
		remove_json_out
		run_multi_experiment
		get_average_results_args $1
	done
}

set_repeat(){
	#ex $1=REPEAT
	REPEAT=$1
	#argument processing
	if [ -z "$1" ]
	  then
		REPEAT=10
	fi
	echo "repeat:${REPEAT}"
}
