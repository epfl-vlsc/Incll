/*
 * Thread barrier for coordinating multiple threads
 */

#pragma once
#include "masstree_econfigs.hh"


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
