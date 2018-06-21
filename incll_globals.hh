/*
 * Global Helper
 * global variables in one struct, like a namespace
 */

#pragma once

#include "incll_barrier.hh"
#include "incll_gf.hh"
#include "incll_configs.hh"
#include <iostream>

namespace GH{
	ThreadBarrier thread_barrier;

	std::string exp_name;
	uint64_t n_keys = 50;
	uint64_t n_initops = 25;
	uint64_t n_ops1 = 20;
	uint64_t n_ops2 = 5;

#ifdef GLOBAL_FLUSH
	GlobalFlush global_flush;
#endif

	void set_exp_name(const char *exp){
		exp_name = std::string(exp);
        exp_name += "_lw" + std::to_string(KEY_LW);
        exp_name += "_iw" + std::to_string(KEY_IW);

#ifdef GLOBAL_FLUSH
        exp_name += "_gl" + std::to_string(GL_FREQ);
#endif

#ifdef INCLL
        exp_name += "_incll";
#endif

#ifdef NVMLOG
        exp_name += "_nvmlog";
#endif

#ifdef PALLOC
        exp_name += "_p";
#endif
	}


	void init_all(int nthreads, const char *testname){
		thread_barrier.init(nthreads);
		set_exp_name(testname);

#ifdef GLOBAL_FLUSH
		global_flush.init(nthreads);
#endif
	}
};
