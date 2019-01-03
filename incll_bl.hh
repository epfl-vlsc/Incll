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

#pragma once

#include <pthread.h>
#include <functional>

#include "incll_configs.hh"

#define BUCKET_LOCKS_SIZE 10000

class BucketLocks{
	pthread_mutex_t bucket_locks[BUCKET_LOCKS_SIZE];


	std::size_t compute_hash(void *ptr){
		return std::hash<void*>{}(ptr);
	}
public:
	void init(){
		for(int i=0;i<BUCKET_LOCKS_SIZE;++i){
			pthread_mutex_init(&bucket_locks[i], NULL);
		}

	}

	void destroy(){
		for(int i=0;i<BUCKET_LOCKS_SIZE;++i){
			pthread_mutex_destroy(&bucket_locks[i]);
		}
	}

	int lock(void *ptr){
		int i = compute_hash(ptr) % BUCKET_LOCKS_SIZE;
		pthread_mutex_lock(&bucket_locks[i]);
		return i;
	}

	void unlock(int i){
		pthread_mutex_unlock(&bucket_locks[i]);
	}
};
