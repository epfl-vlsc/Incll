#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include "masstree_eglobals.hh"

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
