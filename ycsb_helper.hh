#pragma once

#include "kvrandom.hh"
#include "ycsb_distributions.hh"

namespace ycsbc{

struct OpRatios{
	const int get_cum;
	const int put_cum;
	const int rem_cum;
	static constexpr const int MAX_FREQ = 100;

	kvrandom_lcg_nr op_rand;

	OpRatios(int get_freq,
			int put_freq,
			int rem_freq,
			int scan_freq):
				get_cum(get_freq),
				put_cum(get_cum + put_freq),
				rem_cum(put_cum + rem_freq){

		assert(rem_cum + scan_freq == MAX_FREQ);
	}

	ycsb_op get_next_op(){
		int op = op_rand.next()%MAX_FREQ;
		if(op < get_cum){
			return get_op;
		}else if(op < put_cum){
			return put_op;
		}else if(op < rem_cum){
			return rem_op;
		}else{
			return scan_op;
		}
	}

	int get_next_op_freq(){
		return op_rand.next()%MAX_FREQ;
	}
};

struct OpHelper{
	size_t ninitops;
	size_t nops;
	size_t nkeys;

	OpHelper(size_t ninitops_,
			size_t nops_,
			size_t nkeys_):
	ninitops(ninitops_), nops(nops_), nkeys(nkeys_)
	{}
};

template<typename RK, typename RV, typename ROP>
void exp_init_all(int tid, RK& key_rand,
		RV& val_rand, ROP& op_rand){
	key_rand.init(tid);
	val_rand.init(tid);
	op_rand.reset(tid);
}

}; //ycsbc



