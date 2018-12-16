/*
 * Global Helper
 * global variables in one struct, like a namespace
 */

#pragma once

#include <iostream>

#include "incll_barrier.hh"
#include "incll_gf.hh"
#include "incll_configs.hh"
#include "incll_pextlog.hh"
#include "incll_bl.hh"

#ifdef MTAN
void report_mtan();
#endif

namespace GH{
	extern ThreadBarrier thread_barrier;
#ifdef INCLL
	extern BucketLocks bucket_locks;
#endif

	extern std::string exp_name;
	extern uint64_t n_keys;
	extern uint64_t n_initops;
	extern uint64_t n_ops1;
	extern uint64_t n_ops2;

	extern int get_rate;
	extern int put_rate;
	extern int rem_rate;
	extern int scan_rate;

	void check_rate(int rate);

#ifdef GLOBAL_FLUSH
	extern GlobalFlush global_flush;
#endif

#ifdef EXTLOG
	extern thread_local PExtNodeLogger *node_logger;
	extern PLogAllocator plog_allocator;
	bool is_recovery();
#endif //extlog

	void print_exp_params();
	void init_all(int nthreads, const char *testname);
	void init_thread_all(int tid);

	void advance_epoch(int tid, void *root);

};
