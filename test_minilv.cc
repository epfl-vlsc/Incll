#include <iostream>
#include <bitset>
#include <cstdint>
#include <cassert>
#include <typeinfo>
using namespace std;

typedef uint64_t mrcu_epoch_type;
mrcu_epoch_type globalepoch;

// clean and write style for bit assignments
class incll_lv_{
//private:
public:
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
		//assert(p < 7);
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

	void set_lv(uintptr_t lv){
		assert(!(lv & non_lv_mask));
		data_ = (data_ & non_lv_mask) | lv;
	}

	uintptr_t get_lv() const{
		return data_ & lv_mask;
	}

	void set_lv_idx(uintptr_t lv, int p){
		data_ = (data_ & non_lv_idx_mask) | lv | p;
	}

	void save_lv_idx_to_args(uintptr_t& lv, int& p){
		lv = data_ & lv_mask;
		p = data_ & idx_mask;
	}

	bool is_le_diff(){
		return ((data_ & le_mask) >> epoch_shift) != (globalepoch & ge_mask);
	}

	void print() const;
};

void incll_lv_::print() const{
	std::bitset<64> data_bits(data_);
	printf("incll mini cl_idx:%d lv:%p nl:%d le:%lu hex:0x%lx bits:",
			get_cl_idx(), (void*)get_lv(), is_not_logged(),
			get_loggedepoch(), data_);
	std::cout << data_bits << "\n";
}


void test_incll_lv(){
	//check init idx, le
	globalepoch = 1;
	incll_lv_ cl;
	assert(cl.get_loggedepoch() == 1);
	assert(cl.get_cl_idx() == cl.invalid_idx);

	//check lv and idx
	int *a = new int;
	*a = 5;

	cl.set_lv_idx((uintptr_t)a, 4);
	assert(cl.get_cl_idx() == 4);
	assert(cl.get_lv() == (uintptr_t)a);

	//check lv and idx seperately
	int *b = new int;
	*b = 6;
	cl.set_cl_idx(2);
	assert(cl.get_cl_idx() == 2);
	cl.set_lv((uintptr_t)b);
	assert(cl.get_lv() == (uintptr_t)b);

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
}


int main(){
	test_incll_lv();


	return 0;
}
