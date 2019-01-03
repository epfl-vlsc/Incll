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
