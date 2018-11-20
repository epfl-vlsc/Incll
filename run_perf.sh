PERFDIR=perf_output
rm -rf /scratch/tmp/nvm.*
rm -rf /dev/shm/incll/nvm.*

rm -rf ${PERFDIR}
mkdir ${PERFDIR}

for WORKLOAD in ycsb_a_uni ycsb_b_uni ycsb_c_uni ycsb_e_uni \
	ycsb_a_zipf ycsb_b_zipf ycsb_c_zipf ycsb_e_zipf; do
    echo "perf ${WORKLOAD}"
	
	PERF_OUT=${PERFDIR}/${WORKLOAD}.perf
	rm -rf $PERF_OUT
	echo "perf ${PERF_OUT}"
	perf stat -d -d -d -o $PERF_OUT ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 -j8 --pin
	sleep 1
	rm -rf /scratch/tmp/nvm.*
	rm -rf /dev/shm/incll/nvm.*
done

rm -rf /scratch/tmp/nvm.*
rm -rf /dev/shm/incll/nvm.*