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


void test_bl(){
	int *a = new int;
	int *b = new int;
	double *c = new double;

	int a_i = GH::bucket_locks.lock(a);
	int b_i = GH::bucket_locks.lock(b);
	int c_i = GH::bucket_locks.lock(c);

	*a = 2;
	*b = 3;
	*c = 5.5;

	GH::bucket_locks.unlock(c_i);
	GH::bucket_locks.unlock(b_i);
	GH::bucket_locks.unlock(a_i);

	assert(*a == 2);
	assert(*b == 3);
	assert(*c = 5.5);

	delete a;
	delete b;
	delete c;
}


void do_experiment(std::string fnc_name, void (*fnc)()){
	GH::bucket_locks.init();
	printf("%s\n", (fnc_name + " begin").c_str());
	fnc();
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	GH::bucket_locks.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);

int main(){
	DO_EXPERIMENT(test_bl);

	return 0;
}
