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

/* Global flush
 * Class to flush all cachelines
 *
 */

#pragma once

#include "incll_configs.hh"

#ifdef GLOBAL_FLUSH
#include "incll_globals.hh"
#include "incll_pextlog.hh"

#include <atomic>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

#define GF_FILE "/dev/global_flush"
typedef uint64_t mrcu_epoch_type;
extern volatile mrcu_epoch_type globalepoch;
extern volatile mrcu_epoch_type active_epoch;

namespace GH{
	#ifdef EXTLOG
	extern thread_local PExtNodeLogger *node_logger;
	#endif
}

class GlobalFlush{
private:
	sem_t flush_ack_sem;
	sem_t ack_sem;
	int nthreads;

	int flush_frequency;

	//global flush
	int fd;

	//ordering vaars
	std::atomic<bool> influshWarning; //general warning that something may change.
	std::atomic<bool> waitingFlush;	  //flush needed. Wait until it finishes.
	std::atomic<int> doneThreads;

	void global_flush(){
		int ret = write(fd, "", 0);
		if (ret < 0){
			perror("Failed to flush.");
			return;
		}
	}

	void flush_(){
		//flush
		global_flush();
	}

public:
	void reset(){
		globalepoch = 1;
	}

	void thread_done(){
		influshWarning.store(true, std::memory_order_seq_cst);//until the next epoch...
		doneThreads.fetch_add(1);
	}

	void init(int nthreads_){
		sem_init(&flush_ack_sem, 0, 0);
		sem_init(&ack_sem, 0, 0);
		waitingFlush=false;
		influshWarning=false;
		doneThreads=0;

		//global flush driver
		fd = open(GF_FILE, O_RDWR);
		if (fd < 0){
			perror("Failed to connect to device\n");
			return;
		}

		nthreads = nthreads_;
	}

	bool check_done(){
		return doneThreads == nthreads;
	}

	bool ack_flush() {
		if(influshWarning.load(std::memory_order_acquire)==false)
			return false;
		if(check_done())
			return true;

		//if in flush
		if(waitingFlush.load(std::memory_order_acquire)){
			//printf("ackflush ge:%lu \n", globalepoch);

			//signal acknowledge
			sem_post(&ack_sem);

			// wait for flush to complete
			sem_wait(&flush_ack_sem);

			#ifdef EXTLOG
			GH::node_logger->checkpoint();
			#endif
		}
		return false;
	}

	void ack_flush_manual() {
		//signal acknowledge
		sem_post(&ack_sem);

		// wait for flush to complete
		sem_wait(&flush_ack_sem);
	}

	void flush_manual(){
		//wait for other thread acks
		int other_threads = nthreads - 1;
		for(int i=0;i<other_threads;++i){
			sem_wait(&ack_sem);
		}

		//global flush
		this->flush_();

		//inc ge
		globalepoch++;

		//release other threads
		for(int i=0;i<other_threads;++i){
			sem_post(&flush_ack_sem);
		}
	}

	void flush(long){
		influshWarning.store(true, std::memory_order_seq_cst);
		if(check_done())
			return;

		//entering flush mode
		waitingFlush.store(true, std::memory_order_seq_cst);

		//wait for other thread acks
		int other_threads = nthreads - 1;
		for(int i=0;i<other_threads;++i){
			sem_wait(&ack_sem);
		}

		//global flush
		this->flush_();
		//printf("flush ge:%lu \n", globalepoch);

		//exiting flush mode
		waitingFlush.store(false, std::memory_order_seq_cst);

		//release other threads
		for(int i=0;i<other_threads;++i){
			sem_post(&flush_ack_sem);
		}

		#ifdef EXTLOG
		GH::node_logger->checkpoint();
		#endif

		influshWarning.store(false, std::memory_order_release);
	}

	bool is_in_flush(){
		return influshWarning.load(std::memory_order_acquire);
	}

	void block_flush(){
		influshWarning.store(true, std::memory_order_seq_cst);
		waitingFlush.store(true, std::memory_order_seq_cst);
	}

};



#endif //gf
