/*
 * Global Helper
 * global variables in one struct, like a namespace
 */

#pragma once

#include "incll_barrier.hh"
#include "incll_gf.hh"
#include "incll_configs.hh"
#include <iostream>
#include "incll_extlog.hh"

namespace GH{
	extern ThreadBarrier thread_barrier;

	extern std::string exp_name;
	extern uint64_t n_keys;
	extern uint64_t n_initops;
	extern uint64_t n_ops1;
	extern uint64_t n_ops2;

#ifdef GLOBAL_FLUSH
	extern GlobalFlush global_flush;
#endif

	extern thread_local ExtNodeLogger node_logger;

	void print_exp_params();
	void init_all(int nthreads, const char *testname);

#ifdef GLOBAL_FLUSH
	void advance_epoch(int tid);
#endif //gf

};