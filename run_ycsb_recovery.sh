#!/usr/bin/env bash

: '
/* Modification of Masstree
 * VLSC Laboratory
 * Copyright (c) 2018-2019 Ecole Polytechnique Federale de Lausanne
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Masstree LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Masstree LICENSE file; the license in that file
 * is legally binding.
 */
'

use_gdb=$1
repeat=1

make mttest

mkdir -p output
Oname=output/ycsb_recovery.txt
rm -rf ${Oname}

echo "RecoveryTimeMicroSec,StdRecTime,StdP,Workload" >> ${Oname}

for WORKLOAD in ycsb_a_uni_recovery ycsb_a_zipf_recovery; do
    echo ${WORKLOAD}
	rm -rf *.json
	for i in $(eval echo {1..$repeat}); do 
		echo "recovery ${WORKLOAD} ${i}"
		rm -rf /scratch/tmp/nvm.*
		rm -rf /dev/shm/incll/nvm.*
		./mttest ${WORKLOAD} --nops1=1000000 --ninitops=3000000 --nkeys=3000000 -j8
		echo -e "\n\n\n\n"
		${use_gdb} ./mttest ${WORKLOAD} --nops1=1000000 --ninitops=3000000 --nkeys=3000000 -j8
		sleep 1
	done
	python get_recovery.py "${WORKLOAD}" >> ${Oname}
done

rm -rf /scratch/tmp/nvm.*
rm -rf /dev/shm/incll/nvm.*
