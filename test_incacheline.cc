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

#include "test_mocktree.hh"
#include <vector>
#include <thread>
#include "incll_globals.hh"
#include "incll_trav.hh"
#include "incll_copy.hh"

#include <set>
#include <vector>
#include <algorithm>
#include <iostream>


__thread typename MockMasstree::table_params::threadinfo_type* MockMasstree::ti = nullptr;
volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;
int delaycount = 0;
volatile void *global_masstree_root = nullptr;
kvtimestamp_t initial_timestamp;

#define N_OPS 80
#define INTERVAL 8


void insert_incll(MockMasstree *mt){
	mt->insert({9,5,1,3,7,2,4,6});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert({8});

	set_failed_epoch(2);

	assert(is_same_tree(mt->get_root(), copy, true));
}

void update_incll(MockMasstree *mt){
	mt->insert({9,5,1,3,7,2,4,6});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert(9, new uint64_t(8));

	set_failed_epoch(2);

	assert(is_same_tree(mt->get_root(), copy, true));
}

void insert_log(MockMasstree *mt){
	mt->insert({9,5,1,3,7,2,4,6});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert({8, 0});

	set_failed_epoch(2);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}

void update_log(MockMasstree *mt){
	mt->insert({9,5,1,3,7,2,4,6});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert({8});
	mt->insert(8, new uint64_t(3));

	set_failed_epoch(2);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}

void remove_log(MockMasstree *mt){
	mt->insert({9,5,1,3,7,2,4,6});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->remove({9});

	set_failed_epoch(2);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}


void mix_log(MockMasstree *mt){
	mt->insert({9,5,1,3,7,2,4,6});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert({0});
	mt->remove({9});

	set_failed_epoch(2);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}


void split_log(MockMasstree *mt){
	for(int i=2;i<18;++i){
		mt->insert(i, new uint64_t(i + 2));
	}
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert(2, new uint64_t(21));
	mt->insert({0});
	mt->remove({5,7});
	mt->insert({1,18,7});

	set_failed_epoch(2);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}

void kill_root_leaf(MockMasstree *mt){
	for(int i=0;i<24;++i){
		mt->insert(i, new uint64_t(i + 2));
	}
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	for(int i=0;i<13;++i){
		mt->remove(i);
	}
	mt->insert({36});

	set_failed_epoch(2);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}

void two_incll_epochs(MockMasstree *mt){
	mt->insert({9,5,1,3,2,4,6});
	adv_epoch(mt);

	mt->insert({0});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert({7});

	set_failed_epoch(3);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}

void two_log_epochs(MockMasstree *mt){
	mt->insert({9,5,1,3,2,4,6});
	adv_epoch(mt);

	mt->insert({0,8});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert({7});

	set_failed_epoch(3);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}

//for combination test----------------------------------------------------------------
typedef	std::vector<uint64_t> vi;
typedef std::vector<vi> vvi;


vvi cart_product (const vvi& v) {
	vvi s = {{}};
    for (auto& u : v) {
    	vvi r;
        for (auto& x : s) {
            for (auto y : u) {
                r.push_back(x);
                r.back().push_back(y);
            }
        }
        s.swap(r);
    }
    return s;
}

struct CartesianProduct{
	vvi res;

	CartesianProduct(int l=6, int d=3){
		vvi vec;
		for(int i=0;i<l;++i){
			vi vec_entry;
			for(int j=0;j<d;++j){
				vec_entry.push_back(j);
			}
			vec.push_back(vec_entry);
		}
		res = cart_product(vec);
	}

	void print(){
		for(auto v:res){
			for (auto e:v){
				std::cout << e << " ";
			}std::cout << std::endl;
		}
	}
};

vi insert_v;
vi remove_v;

std::string op_names[3] = {"get",
		"ins",
		"rem"
};

enum{
	get_op,
	ins_op,
	rem_op
};

void print_args(vi& argv){
	int i=0;
	std::cout << "Args ";
	for(auto v:argv){
		if(i%3 == 0) std::cout << "new epoch ";
		i++;
		std::cout << op_names[v] << " ";
	}std::cout << std::endl;

}

void do_op(MockMasstree *mt, int i, int idx){
	switch(i){
	case 0:
		mt->remove({remove_v[idx]});
		break;
	case 1:
		mt->remove({remove_v[idx]});
		break;
	case 2:
		mt->insert({insert_v[idx]});
		break;
	default:
		assert(0);
		break;
	}
}

void exhaustive_testing(vi& argv){
	print_args(argv);

	auto mt = new MockMasstree();
	globalepoch=1;
	mt->thread_init(0);
	GH::plog_allocator.init();
	GH::node_logger = GH::plog_allocator.init_plog(0);

	//init tree
	for(auto e:remove_v){
		mt->insert({e});
	}

	//function region
	int idx = 0;
	adv_epoch(mt);
	do_op(mt, argv[0], idx++);
	do_op(mt, argv[1], idx++);
	do_op(mt, argv[2], idx++);

	adv_epoch(mt);
	void *copy = copy_tree(mt->get_root());
	do_op(mt, argv[3], idx++);
	do_op(mt, argv[4], idx++);
	do_op(mt, argv[5], idx++);

	set_failed_epoch(3);
	undo_all(mt);
	assert(is_same_tree(mt->get_root(), copy, true));

	GH::plog_allocator.destroy();
}

void init_tree(int init_size){
	for(int i=0;i<init_size;++i){
		insert_v.push_back(rand());
		remove_v.push_back(rand());
	}
}

void do_exhaustive_testing(int init_size){
	CartesianProduct cp;

	init_tree(init_size);

	for(auto v:cp.res){
		exhaustive_testing(v);
	}
}

void do_exhaustive_sweep(){
	for(int i=20;i<1000;i+=50){
		printf("init tree size: around %d----\n", i);
		do_exhaustive_testing(i);
	}
}


void do_experiment(std::string fnc_name, void (*fnc)(MockMasstree *)){
	auto mt = new MockMasstree();
	mt->thread_init(0);
	globalepoch=1;
	failedepoch=0;
	GH::plog_allocator.init();
	GH::node_logger = GH::plog_allocator.init_plog(0);
	GH::bucket_locks.init();

	printf("%s\n", (fnc_name + " begin").c_str());
	fnc(mt);
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	GH::bucket_locks.destroy();
	GH::plog_allocator.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);


int main(){
	DO_EXPERIMENT(insert_incll)
	DO_EXPERIMENT(update_incll)
	DO_EXPERIMENT(insert_log)
	DO_EXPERIMENT(update_log)
	DO_EXPERIMENT(remove_log)
	DO_EXPERIMENT(mix_log)
	DO_EXPERIMENT(split_log)
	DO_EXPERIMENT(kill_root_leaf)
	DO_EXPERIMENT(two_incll_epochs)
	DO_EXPERIMENT(two_log_epochs)

	do_exhaustive_sweep();

	return 0;
}
