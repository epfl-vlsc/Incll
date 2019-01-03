#include "ycsb_helper.hh"

#include <cstdio>
#include <cassert>
using namespace ycsbc;

#define N 1000

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
	UniGen ug;
	ug.init(1, 100);
	assert_avg(ug,55);
	//print_dist(ug);

	ScrambledZipGen szg;
	szg.init(1, 100);
	//print_dist(szg);
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
