#include "incll_globals.hh"

namespace GH{
	ThreadBarrier thread_barrier;
	Ifincll(BucketLocks bucket_locks)

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
	thread_local PExtNodeLogger *node_logger;
	PLogAllocator plog_allocator;

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
#endif

		plog_allocator.init();

	}

	void init_thread_all(int tid){
		node_logger = plog_allocator.init_plog(tid);
	}

	bool is_recovery(){
		return plog_allocator.exists;
	}

	void advance_epoch(int tid, void *root){
#ifdef GLOBAL_FLUSH
		thread_barrier.wait_barrier(tid);
		if(tid == 0){
			node_logger->set_log_root(root);

			global_flush.flush_manual();
		}else{
			global_flush.ack_flush_manual();
		}
		node_logger->checkpoint();
		thread_barrier.wait_barrier(tid);
#endif
	}
};
