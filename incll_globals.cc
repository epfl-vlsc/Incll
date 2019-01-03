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

#include "incll_globals.hh"

extern volatile void *global_masstree_root;

#ifdef MTAN
std::atomic<size_t> nflushes(0);
std::atomic<size_t> nupdate_incll(0);
std::atomic<size_t> ninsert_incll(0);
std::atomic<size_t> ninserts(0);
std::atomic<size_t> nrecords_lf(0);
std::atomic<size_t> nrecords_in(0);

void report_mtan(){
	printf("Flushes:%lu\n", size_t(nflushes));
	printf("Incll update:%lu\n", size_t(nupdate_incll));
	printf("Incll insert:%lu\n", size_t(ninsert_incll));
	printf("Insert:%lu\n", size_t(ninserts)-GH::n_initops);
	printf("Lf records:%lu\n", size_t(nrecords_lf));
	printf("In records:%lu\n", size_t(nrecords_in));
}

#endif //MTAN

namespace GH{
	ThreadBarrier thread_barrier;
#ifdef INCLL
	BucketLocks bucket_locks;
#endif

	std::string exp_name;
	uint64_t n_keys = 50;
	uint64_t n_initops = 25;
	uint64_t n_ops1 = 20;
	uint64_t n_ops2 = 5;

	int get_rate = 50;
	int put_rate = 25;
	int rem_rate = 25;
	int scan_rate = 0;


	void check_rate(int rate){
		assert(rate <= 100);
	}

#ifdef GLOBAL_FLUSH
	GlobalFlush global_flush;
#endif

#ifdef EXTLOG
	thread_local PExtNodeLogger *node_logger;
	PLogAllocator plog_allocator;
#endif //extlog

	void print_exp_params(){
		printf("nkeys:%lu ninitops:%lu "
				"nops1:%lu nops2:%lu "
				"get rate:%d put rate:%d rem rate:%d scan rate:%d\n",
				n_keys, n_initops,
				n_ops1, n_ops2,
				get_rate, put_rate, rem_rate, scan_rate);
	}

	void set_exp_name(const char *exp){
		exp_name = std::string(exp);

#ifdef GLOBAL_FLUSH
        exp_name += "_gl" + std::to_string(GL_FREQ);
#endif
	}


	void init_all(int nthreads, const char *testname){
		srand(0);

		thread_barrier.init(nthreads);
		set_exp_name(testname);

#ifdef GLOBAL_FLUSH
		global_flush.init(nthreads);
#endif //gf

#ifdef INCLL
		bucket_locks.init();
#endif //incll

#ifdef EXTLOG
		plog_allocator.init();
#endif //extlog

	}

	void init_thread_all(int tid){
#ifdef EXTLOG
		node_logger = plog_allocator.init_plog(tid);
#endif //extlog
	}

#ifdef EXTLOG
	bool is_recovery(){
		return plog_allocator.exists;
	}
#endif //extlog

	void advance_epoch(int tid, void *root){
#ifdef GLOBAL_FLUSH
		thread_barrier.wait_barrier(tid);
		if(tid == 0){
			global_masstree_root = root;

			global_flush.flush_manual();
		}else{
			global_flush.ack_flush_manual();
		}
	#ifdef EXTLOG
		node_logger->checkpoint();
	#endif
		thread_barrier.wait_barrier(tid);
#endif
	}
};
