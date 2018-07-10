#include "incll_globals.hh"

namespace GH{
	ThreadBarrier thread_barrier;

	std::string exp_name;
	uint64_t n_keys = 50;
	uint64_t n_initops = 25;
	uint64_t n_ops1 = 20;
	uint64_t n_ops2 = 5;
	uint64_t get_rate = 50;
	uint64_t put_rate = 25;


	void check_rate(uint64_t rate){
		assert(rate <= 100);
	}

	uint64_t put_rate_cum(){
		uint64_t put_rate_cumulative = put_rate + get_rate;
		check_rate(put_rate_cumulative);

		return put_rate_cumulative;
	}

#ifdef GLOBAL_FLUSH
	GlobalFlush global_flush;
#endif
	thread_local ExtNodeLogger node_logger;

	void print_exp_params(){
		printf("nkeys:%lu ninitops:%lu "
				"nops1:%lu nops2:%lu "
				"getrate:%lu putrate:%lu removerate:%lu\n",
				n_keys, n_initops,
				n_ops1, n_ops2,
				get_rate, put_rate, 100 - put_rate_cum());
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
	}

#ifdef GLOBAL_FLUSH
	void advance_epoch(int tid, void *root){
		thread_barrier.wait_barrier(tid);
		if(tid == 0){
			node_logger.set_log_root(root);
			global_flush.flush_manual();
		}else{
			global_flush.ack_flush_manual();
		}
		node_logger.checkpoint();
		thread_barrier.wait_barrier(tid);
	}
#endif //gf

};
