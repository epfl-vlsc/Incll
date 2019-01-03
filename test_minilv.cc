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

#include <iostream>
#include <bitset>
#include <cstdint>
#include <cassert>
#include <typeinfo>
using namespace std;

typedef uint64_t mrcu_epoch_type;
volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;
int delaycount = 0;
volatile void *global_masstree_root = nullptr;

#define REC_ASSERT(s) assert(s);
#define KEY_MID 7

class leaf_value{
	union {
		void* n;
		uint64_t v;
		uintptr_t x;
	} u_;
public:
	leaf_value(){
		u_.v = 8;
	}

	void print(){
		printf("lv:%p\n", u_.n);
	}

	uint64_t& value(){
		return u_.v;
	}

	void* value_ptr(){
		return u_.n;
	}


	void set_value(uint64_t t){
		u_.v = t;
	}
};

// clean and write style for bit assignments
class incll_lv_{
private:
	uint64_t data_;
public:
	enum {
		idx_mask 			= 7, //bits 210
		non_idx_mask 		= ~idx_mask,
		not_logged_bit 		= (1UL << 63), //bit 63
		not_logged_reset 	= ~not_logged_bit,
		lv_mask 			= 0x00FFFFFFFFFFFFF8UL, //bits 56-3
		non_lv_mask 		= ~lv_mask,
		lv_idx_mask 		= 0x00FFFFFFFFFFFFFFUL,
		non_lv_idx_mask 	= ~lv_idx_mask,
		le_mask 			= 0x7F00000000000000UL, //bit 62-56
		ge_mask 			= 0x000000000000007FUL, //bit 14-0
		non_le_mask 		= ~le_mask,
		epoch_shift 		= 56,
		invalid_idx 		= 7, //111 number is invalid
		lv_idx_ge_mask		= 0x7FFFFFFFFFFFFFFFUL, //bits 62-0
		non_lv_idx_ge_mask	= ~lv_idx_ge_mask,
	};

	incll_lv_(){
		data_ &= 0x0;
		set_loggedepoch();
		invalidate_cl();
	}

	int get_cl_idx() const{
		return data_ & idx_mask;
	}

	void set_cl_idx(int p){
		REC_ASSERT(p < 7)
		data_ = (data_ & non_idx_mask) | p;
	}

	void invalidate_cl(){
		data_ |= invalid_idx;
	}

	mrcu_epoch_type get_loggedepoch() const{
		return ((data_ & le_mask) >> epoch_shift);
	}

	void set_loggedepoch(mrcu_epoch_type e){
		data_ = (data_ & non_le_mask) | ((e & ge_mask) << epoch_shift);
	}

	void set_loggedepoch(){
		data_ = (data_ & non_le_mask) | ((globalepoch & ge_mask) << epoch_shift);
	}

	bool is_not_logged() const{
		return data_ & not_logged_bit;
	}

	void set_not_logged(){
		data_ |= not_logged_bit;
	}

	void reset_not_logged(){
		data_ &= not_logged_reset;
	}

	void set_lv(uint64_t *lv){
		REC_ASSERT(!(*lv & non_lv_mask));
		data_ = (data_ & non_lv_mask) | *lv;
	}

	uint64_t get_lv() const{
		return data_ & lv_mask;
	}

	void set_lv_idx(uint64_t *lv, int p){
		REC_ASSERT(!(*lv & non_lv_mask));
		REC_ASSERT(p < 7)
		data_ = (data_ & non_lv_idx_mask) | *lv | p;

	}

	void set_lv_idx_ge(uint64_t *lv, int p){
		REC_ASSERT(!(*lv & non_lv_mask));
		REC_ASSERT(p < 7)
		data_ = (data_ & non_lv_idx_ge_mask)
				| *lv
				| p
				| ((globalepoch & ge_mask) << epoch_shift);
	}

	void recover_lv(uint64_t* lv){
		int p = data_ & idx_mask;
		lv[p] = data_ & lv_mask;
	}

	void recover_lv2(uint64_t* lv){
		int p = (data_ & idx_mask) + KEY_MID;
		lv[p] = data_ & lv_mask;
	}

	bool is_le_diff(){
		return ((data_ & le_mask) >> epoch_shift) != (globalepoch & ge_mask);
	}

	bool is_cl_valid(){
		return (data_ & invalid_idx) != invalid_idx;
	}

	void print() const;
};

void incll_lv_::print() const{
	std::bitset<64> data_bits(data_);
	printf("incll mini cl_idx:%d lv:%lx nl:%d le:%lu hex:0x%lx bits:",
			get_cl_idx(), get_lv(), is_not_logged(),
			get_loggedepoch(), data_);
	std::cout << data_bits << "\n";
}


void test_incll_lv(){
	//check init idx, le
	globalepoch = 1;
	incll_lv_ cl;
	assert(cl.get_loggedepoch() == 1);
	assert(cl.get_cl_idx() == cl.invalid_idx);
	assert(!cl.is_cl_valid());

	//check lv and idx
	leaf_value *lvs = new leaf_value[15];
	cl.set_lv_idx(&lvs[4].value(), 4);
	assert(cl.get_cl_idx() == 4);
	assert(cl.get_lv() == lvs[4].value());

	//check lv and idx seperately
	lvs[2].set_value(0xF0);
	cl.set_cl_idx(2);
	assert(cl.get_cl_idx() == 2);
	assert(cl.is_cl_valid());
	cl.set_lv(&lvs[2].value());
	assert(cl.get_lv() == lvs[2].value());

	//check recover
	cl.set_cl_idx(0);
	cl.recover_lv((uint64_t*)lvs);
	assert(lvs[0].value() == lvs[2].value());

	//check set not logged
	cl.set_not_logged();
	assert(cl.is_not_logged());

	//check reset not logged
	cl.reset_not_logged();
	assert(!cl.is_not_logged());

	cl.set_not_logged();
	assert(cl.is_not_logged());

	//check logged epoch
	globalepoch = 2;
	cl.set_loggedepoch();
	assert(cl.get_loggedepoch() == 2);
	cl.set_loggedepoch(3);
	assert(cl.get_loggedepoch() == 3);

	assert(cl.is_le_diff());
	globalepoch = 3;
	assert(!cl.is_le_diff());

	//size test
	assert(sizeof(incll_lv_) == sizeof(uint64_t));
	assert(sizeof(cl) == sizeof(uint64_t));

	//check idx, lv, ge set
	globalepoch = 4;
	cl.set_lv_idx_ge(&lvs[1].value(), 1);
	assert(!cl.is_le_diff());
	assert(cl.get_loggedepoch() == globalepoch);
	assert(cl.get_cl_idx() == 1);
	assert(cl.get_lv() == lvs[1].value());
	printf("passed asserts\n");
}


int main(){
	test_incll_lv();

	return 0;
}
