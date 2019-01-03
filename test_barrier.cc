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
#include <chrono>
#include <cassert>
#include <cstdlib>
#include <cstdio>

#include "incll_globals.hh"

volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;
int delaycount = 0;
volatile void *global_masstree_root = nullptr;

const int NUM_THREADS = 8;
int signs[NUM_THREADS];


void assert_all_num(int num){
	for(int i=0;i<NUM_THREADS;++i){
		assert(signs[i] == num);
	}
}

void imbalanced_threads(int tid, int sign){
	if(tid%2 == 0){
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	signs[tid] = sign;
}

void thread_code(int tid){
	imbalanced_threads(tid, 1);
	GH::thread_barrier.wait_barrier(tid);
	if(tid==0) assert_all_num(1);
	GH::thread_barrier.wait_barrier(tid);

	GH::thread_barrier.wait_barrier(tid);
	imbalanced_threads(tid, 0);
	GH::thread_barrier.wait_barrier(tid);
	if(tid==0) assert_all_num(0);
	GH::thread_barrier.wait_barrier(tid);
}

void test_multithread(){
	std::vector<std::thread> ths;
	for (int i = 0; i < NUM_THREADS; ++i)
		ths.emplace_back(thread_code, i);
	for (auto& t : ths)
		t.join();
}


void do_experiment(std::string fnc_name, void (*fnc)()){
	GH::thread_barrier.init(NUM_THREADS);
	printf("%s\n", (fnc_name + " begin").c_str());
	fnc();
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	GH::thread_barrier.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);

int main(){
	DO_EXPERIMENT(test_multithread);

	return 0;
}
