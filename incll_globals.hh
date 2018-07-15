/*
 * Global Helper
 * global variables in one struct, like a namespace
 */

#pragma once

#include <iostream>

#include "incll_barrier.hh"
#include "incll_gf.hh"
#include "incll_configs.hh"
#include "incll_extlog.hh"
#include "incll_bl.hh"

namespace GH{
	extern ThreadBarrier thread_barrier;
	Ifincll(extern BucketLocks bucket_locks)

	extern std::string exp_name;
	extern uint64_t n_keys;
	extern uint64_t n_initops;
	extern uint64_t n_ops1;
	extern uint64_t n_ops2;
	extern uint64_t get_rate;
	extern uint64_t put_rate;

	void check_rate(uint64_t rate);
	uint64_t put_rate_cum();

#ifdef GLOBAL_FLUSH
	extern GlobalFlush global_flush;
#endif

	extern thread_local ExtNodeLogger node_logger;

	void print_exp_params();
	void init_all(int nthreads, const char *testname);

#ifdef GLOBAL_FLUSH
	void advance_epoch(int tid, void *root);
#endif //gf

};
