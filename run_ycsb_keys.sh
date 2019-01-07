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

set_repeat $1
quick_make
create_output keys
write_csv_header_args Keys

use_default_params
use_a_workloads

#todo decide the range
NKEYS_COUNTS=(10000 30000 100000 300000 1000000 3000000 10000000 30000000 100000000)
for NKEYS in ${NKEYS_COUNTS[@]}; do
	echo "Keys ${NKEYS}"
	run_different_workloads_args ${NKEYS}
done

