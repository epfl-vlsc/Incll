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
	double get_;
	double get_put_;
	double get_put_rem_;
	static constexpr const int MAX_FREQ = 100;

	UniformGenerator *uni_rand;

	OpRatios(int get_freq,
			int put_freq,
			int rem_freq,
			int scan_freq){
		get_ = get_freq;
		get_put_ = get_ + put_freq;
		get_put_rem_ = get_put_ + rem_freq;

		assert(get_put_rem_ + scan_freq == MAX_FREQ);

		uni_rand = new UniformGenerator(MAX_FREQ);
	}

	~OpRatios(){
		delete uni_rand;
	}

	ycsb_op get_next_op() const{
		double op = uni_rand->next();
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

	Generator *key_rand;
	Generator *val_rand;
	CounterGenerator *init_rand;

	uint64_t next_key() const{
		return key_rand->next();
	}

	uint64_t next_val() const{
		return val_rand->next();
	}

	uint64_t last() const{
		return key_rand->last();
	}

	uint64_t next_init_key() const{
		return init_rand->next();
	}

	OpHelper(size_t init_,
			size_t nops1_,
			size_t nops2_,
			size_t nkeys_,
			key_distributions kd):
	init(init_), nops1(nops1_),
	nops2(nops2_), nkeys(nkeys_)
	{
		set_generators(kd);
	}

	~OpHelper(){
		delete key_rand;
		delete val_rand;
		delete init_rand;
	}

	void set_generators(key_distributions kd){
		switch(kd){
		case kgd_uniform:
			key_rand = new UniformGenerator(nkeys);
			val_rand = new UniformGenerator(nkeys);
			break;
		case kgd_zipfian:
			key_rand = new ZipfianGenerator(nkeys);
			val_rand = new ZipfianGenerator(nkeys);
			break;
		default:
			assert(0);
		}
		init_rand = new CounterGenerator(nkeys);
	}
};



}; //ycsbc
