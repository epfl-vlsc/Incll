#pragma once

#include "kvrandom.hh"
#include "ycsb_distributions.hh"

namespace ycsbc{
enum ycsb_op{
	get_op,
	put_op,
	rem_op,
	scan_op,
	num_possible_ops
};

enum key_distributions{
	kgd_uniform,
	kgd_zipfian
};


struct OpRatios{
	const int get_cum;
	const int put_cum;
	const int rem_cum;
	static constexpr const int MAX_FREQ = 100;

	kvrandom_lcg_nr rand;

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
		double op = rand.next()%MAX_FREQ;
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



}; //ycsbc
