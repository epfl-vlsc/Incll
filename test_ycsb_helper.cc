#include "ycsb_helper.hh"

#include <cstdio>

using namespace ycsbc;

#define N 10000

template <typename R1, typename R2, typename R3>
void print_dist(ycsbc::OpHelper<R1, R2, R3>& op_helper){
	for(int i=0;i<N;++i){
		printf("%d\n", op_helper.key_rand.next());
	}
}

template <typename R1, typename R2, typename R3>
void assert_avg(OpHelper<R1, R2, R3>& op_helper, uint64_t avg){
	uint64_t sum = 0;
	for(int i=0;i<N;++i){
		sum += op_helper.key_rand.next();
	}
	assert(sum/N < avg);
	print_dist(op_helper);
}

void test_zipfian(){
	OpHelper<ZipfianDist, ZipfianDist, ZipfianDist> op_helper(1,1,1,100);
	assert_avg(op_helper,20);
}

void test_uniform(){
	OpHelper<UniformDist, UniformDist, UniformDist> op_helper(1,1,1,100);
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
