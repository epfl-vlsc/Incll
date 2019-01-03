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

#include <thread>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "incll_globals.hh"

int NUM_THREADS = 16;
typedef uint64_t mrcu_epoch_type;
volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;
int delaycount = 0;
volatile void *global_masstree_root = nullptr;

void assert_epoch(uint64_t num){
	assert(globalepoch == num);
}

void test_manual_body(int tid){
	GH::node_logger = GH::plog_allocator.init_plog(tid);
	if(tid == 0){
		GH::global_flush.flush_manual();
		assert_epoch(2);

		GH::thread_barrier.wait_barrier(tid);

		GH::global_flush.flush_manual();
		assert_epoch(3);
	}else{
		GH::global_flush.ack_flush_manual();
		assert_epoch(2);

		GH::thread_barrier.wait_barrier(tid);

		GH::global_flush.ack_flush_manual();
		assert_epoch(3);
	}
}

void test_manual(){
	std::vector<std::thread> ths;
	for (int i = 0; i < NUM_THREADS; ++i)
		ths.emplace_back(test_manual_body, i);
	for (auto& t : ths)
		t.join();
}


void test_automatic_body(int tid){
	GH::node_logger = GH::plog_allocator.init_plog(tid);
	if(tid == 0){
		for(int i=0;i<10;++i){
			GH::global_flush.flush(globalepoch);
			if(i == 9) GH::global_flush.thread_done();
		}
	}else{
		GH::global_flush.ack_flush();

		GH::global_flush.thread_done();
		bool end = false;
		while(!end){
			end = GH::global_flush.ack_flush();
		}
	}
}

void test_automatic(){
	std::vector<std::thread> ths;
	for (int i = 0; i < NUM_THREADS; ++i)
		ths.emplace_back(test_automatic_body, i);
	for (auto& t : ths)
		t.join();
}



void do_experiment(std::string fnc_name, void (*fnc)()){
	GH::thread_barrier.init(NUM_THREADS);
	GH::global_flush.init(NUM_THREADS);
	GH::plog_allocator.init();
	printf("%s\n", (fnc_name + " begin").c_str());
	fnc();
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	globalepoch = 1;
	GH::thread_barrier.destroy();
	GH::plog_allocator.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);

int main(){
	DO_EXPERIMENT(test_manual);
	DO_EXPERIMENT(test_automatic);

	return 0;
}
