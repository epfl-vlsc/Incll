#include "test_mocktree.hh"
#include <vector>
#include <thread>
#include "incll_globals.hh"
#include "incll_trav.hh"

__thread typename MockMasstree::table_params::threadinfo_type* MockMasstree::ti = nullptr;
volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;

#define N_OPS 80
#define INTERVAL 8
#define NUM_THREADS 8

void assert_tree_size(MockMasstree *mt, size_t size_expected){
	assert(size_expected == get_tree_size(mt->get_root()));
}

void insert_t8_body(MockMasstree *mt, int tid){
	mt->thread_init(tid);
	uint64_t val;

	if(tid == 0)
		assert_tree_size(mt, 0);

	GH::thread_barrier.wait_barrier(tid);

	for(uint64_t i=tid;i<N_OPS;i+=INTERVAL){
		mt->insert(i, i+1);
	}

	GH::thread_barrier.wait_barrier(tid);

	if(tid == 0){
		assert_tree_size(mt, N_OPS);

		//check all keys
		for(uint64_t i=0;i<N_OPS;++i){
			assert(mt->find(i, val));
			assert(val == i+1);
		}
	}
}

void insert_t8(MockMasstree *mt){
	std::vector<std::thread> ths;

	for (int i = 0; i < NUM_THREADS; ++i)
		ths.emplace_back(insert_t8_body, mt, i);
	for (auto& t : ths)
		t.join();
}

void all_t8_body(MockMasstree *mt, int tid){
	uint64_t val;

	insert_t8_body(mt, tid);

	for(uint64_t i=tid;i<N_OPS;i+=INTERVAL){
		mt->remove(i);
	}

	GH::thread_barrier.wait_barrier(tid);

	if(tid == 0){
		assert_tree_size(mt, 0);
		for(uint64_t i=0;i<N_OPS;++i){
			assert(!mt->find(i, val));
		}
	}
}

void all_t8(MockMasstree *mt){
	std::vector<std::thread> ths;

	for (int i = 0; i < NUM_THREADS; ++i)
		ths.emplace_back(all_t8_body, mt, i);
	for (auto& t : ths)
		t.join();
}

void insert_t1(MockMasstree *mt){
	uint64_t val;

	//check size
	assert_tree_size(mt, 0);

	for(uint64_t i=0;i<N_OPS;++i){
		mt->insert(i, i+1);
	}

	assert_tree_size(mt, N_OPS);

	//check all keys
	for(uint64_t i=0;i<N_OPS;++i){
		assert(mt->find(i, val));
		assert(val == i+1);
	}
}

void all_t1(MockMasstree *mt){
	uint64_t val;

	insert_t1(mt);

	for(uint64_t i=0;i<N_OPS;i+=INTERVAL){
		mt->remove(i);
	}

	assert_tree_size(mt, N_OPS - N_OPS/INTERVAL);

	//check all keys
	for(uint64_t i=0;i<N_OPS;++i){
		bool found = mt->find(i, val);

		if(i%INTERVAL == 0){
			assert(!found);
		}else{
			assert(found);
			assert(val == i+1);
		}
	}

	//put keys back
	for(uint64_t i=0;i<N_OPS;i+=INTERVAL){
		mt->insert(i, i+1);
	}

	assert_tree_size(mt, N_OPS);

	//check all keys
	for(uint64_t i=0;i<N_OPS;++i){
		assert(mt->find(i, val));
		assert(val == i+1);
	}
}

void rand_inserts_t1(MockMasstree *mt){
	uint64_t key, val;
	std::vector<uint64_t> inserted_nums;

	for(uint64_t i=0;i<N_OPS;++i){
		key = rand()%100000;
		mt->insert(key, key+1);
		inserted_nums.push_back(key);
	}

	//check all keys
	for(auto e: inserted_nums){
		assert(mt->find(e, val));
		assert(val == e+1);
	}
}

void do_experiment(std::string fnc_name, void (*fnc)(MockMasstree *)){
	auto mt = new MockMasstree();
	mt->thread_init(0);

	printf("%s\n", (fnc_name + " begin").c_str());
	fnc(mt);
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
}

void do_experiment_mt(std::string fnc_name, void (*fnc)(MockMasstree *)){
	auto mt = new MockMasstree();

	GH::thread_barrier.init(NUM_THREADS);
	printf("%s\n", (fnc_name + " begin").c_str());
	fnc(mt);
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	GH::thread_barrier.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);

#define DO_EXPERIMENT_MT(test) \
	do_experiment_mt(#test, test);

int main(){
	DO_EXPERIMENT(insert_t1)
	DO_EXPERIMENT(rand_inserts_t1)
	DO_EXPERIMENT(all_t1)

	DO_EXPERIMENT_MT(insert_t8)
	DO_EXPERIMENT_MT(all_t1)
	return 0;
}
