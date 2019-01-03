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

/*
 * Thread barrier for coordinating multiple threads
 */

#pragma once
#include "incll_configs.hh"
#include <pthread.h>
#include <cstdlib>
#include <cstdio>


class ThreadBarrier{
private:
	pthread_barrier_t barr;

public:
	void init(int nthreads_){
		pthread_barrier_init(&barr, NULL, nthreads_);
	}

	void destroy(){
		pthread_barrier_destroy(&barr);
	}

	void wait_barrier(int id){
		int res = pthread_barrier_wait(&barr);
		if(res != 0 && res!= PTHREAD_BARRIER_SERIAL_THREAD)
			printf("in thread %d in barrier 1 problem %d\n", id, res);
	}
};
