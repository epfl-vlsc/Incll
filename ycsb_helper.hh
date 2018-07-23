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
	const int get_;
	const int get_put_;
	const int get_put_rem_;
	static constexpr const int MAX_FREQ = 100;

	kvrandom_lcg_nr rand;

	OpRatios(int get_freq,
			int put_freq,
			int rem_freq,
			int scan_freq):
				get_(get_freq),
				get_put_(get_ + put_freq),
				get_put_rem_(get_put_ + rem_freq){

		assert(get_put_rem_ + scan_freq == MAX_FREQ);
	}

	ycsb_op get_next_op(){
		double op = rand.next()%MAX_FREQ;
		if(op < get_){
			return get_op;
		}else if(op < get_put_){
			return put_op;
		}else if(op < get_put_rem_){
			return rem_op;
		}else{
			return scan_op;
		}
	}
};

struct OpHelper{
	size_t init;
	size_t nops1;
	size_t nops2;
	size_t nkeys;

	OpHelper(size_t init_,
			size_t nops1_,
			size_t nops2_,
			size_t nkeys_):
	init(init_), nops1(nops1_),
	nops2(nops2_), nkeys(nkeys_)
	{}
};



}; //ycsbc
