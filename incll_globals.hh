/*
 * Global Helper
 * global variables in one struct, like a namespace
 */

#pragma once

#include "incll_barrier.hh"
#include "incll_gf.hh"
#include "incll_configs.hh"


namespace GH{
	ThreadBarrier thread_barrier;

	std::string exp_name;
	static const uint64_t num_keys = 5000000;
	static const uint64_t num_ops = 2000000;

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
