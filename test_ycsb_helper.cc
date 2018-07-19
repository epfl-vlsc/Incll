#include "ycsb_helper.hh"

#include <cstdio>

#define N 10000

void print_dist(const ycsbc::OpHelper& op_helper){
	for(int i=0;i<N;++i){
		printf("%lu\n", op_helper.next_key());
	}
}

void assert_avg(const ycsbc::OpHelper& op_helper, uint64_t avg){
	uint64_t sum = 0;
	for(int i=0;i<N;++i){
		sum += op_helper.next_key();
	}
	assert(sum/N < avg);
}

void test_zipfian(){
	ycsbc::OpHelper op_helper(1,1,1,100,ycsbc::kgd_zipfian);
	assert_avg(op_helper,20);
}

void test_uniform(){
	ycsbc::OpHelper op_helper(1,1,1,100,ycsbc::kgd_uniform);
	assert_avg(op_helper,55);
}

void do_experiment(std::string fnc_name, void (*fnc)()){
	printf("%s\n", (fnc_name + " begin").c_str());
	fnc();
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);



int main(){
	DO_EXPERIMENT(test_zipfian)
	DO_EXPERIMENT(test_uniform)

	return 0;
}
