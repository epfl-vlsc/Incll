#include "ycsb_helper.hh"

#include <cstdio>

using namespace ycsbc;

#define N 10000

template <typename Dist>
void print_dist(Dist& dist){
	for(int i=0;i<N;++i){
		printf("%d\n", dist.next());
	}
}

template <typename Dist>
void assert_avg(Dist& dist, uint64_t avg){
	uint64_t sum = 0;
	for(int i=0;i<N;++i){
		sum += dist.next();
	}
	assert(sum/N < avg);
}

void test_distributions(){
	CounterGen cg;
	cg.init(100);
	assert_avg(cg,51);

	UniformGen ug;
	ug.init(100);
	assert_avg(ug,55);

	ZipfianGen zg;
	zg.init(100);
	assert_avg(zg,20);
}


void do_experiment(std::string fnc_name, void (*fnc)()){
	printf("%s\n", (fnc_name + " begin").c_str());
	fnc();
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);



int main(){
	DO_EXPERIMENT(test_distributions)

	return 0;
}
