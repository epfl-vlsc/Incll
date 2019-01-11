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

source commons.sh

set_repeat 5

NB_SRC=notebook-mttest.json
NB_DST=output/delays_perf.json

remove_files

use_default_params
use_a_workloads

make clean
make mttest "EXTFLAGS=-DPERF_WORKLOAD"
DELAYS_COUNTS=(0 260 520 780 1040 1300 1560 1820 2080 2340 2600)
for DELAYS in ${DELAYS_COUNTS[@]}; do
for WORKLOAD in ${WORKLOADS[@]}; do
	run_multi_experiment
done
done

make clean
make mttest "EXTFLAGS=-DPERF_WORKLOAD -DPERF_STORES"
DELAYS_COUNTS=(0 260 520 780 1040 1300 1560 1820 2080 2340 2600)
for DELAYS in ${DELAYS_COUNTS[@]}; do
for WORKLOAD in ${WORKLOADS[@]}; do
	run_multi_experiment
done
done

cp ${NB_SRC} ${NB_DST}