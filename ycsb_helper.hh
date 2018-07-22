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

template <typename RandKey, typename RandVal>
struct OpHelper{
	size_t ninitops;
	size_t nops;
	size_t nkeys;

	RandKey init_rand;
	RandKey key_rand;
	RandVal val_rand;

	long next_key(){
		return key_rand.next();
	}

	long next_val(){
		return val_rand.next();
	}

	long next_init_key(){
		return init_rand.next();
	}

	OpHelper(size_t ninitops_, size_t nops_,
			size_t nkeys_):
				ninitops(ninitops_),
				nops(nops_),
				nkeys(nkeys_){
		init_rand.init(nkeys_);
		key_rand.init(nkeys_);
		val_rand.init(nkeys_);
	}

};



}; //ycsbc
