#pragma once

#include <pthread.h>
#include <functional>

#include "incll_configs.hh"

#define BUCKET_LOCKS_SIZE 2000

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
