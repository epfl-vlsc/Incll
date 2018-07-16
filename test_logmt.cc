#include <thread>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "incll_globals.hh"
#include "incll_copy.hh"
#include "incll_extlog.hh"
#include "test_mocktree.hh"

__thread typename MockMasstree::table_params::threadinfo_type* MockMasstree::ti = nullptr;
volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;
int delaycount = 0;

#define INTERVAL 8
#define SKIP_INTERVAL 2
#define BOUND (INTERVAL+SKIP_INTERVAL)*INTERVAL
#define WRITE_BEGIN INTERVAL*SKIP_INTERVAL
#define WRITE_SIZE BOUND-WRITE_BEGIN

int NUM_THREADS = 8;

void test_remove(MockMasstree *mt){
	//print_tree_asline(mt->get_root());

	for(uint64_t i=0;i<56;i++){
		if(i%INTERVAL < 1) continue;
		mt->insert({i});
	}

	//print_tree_asline(mt->get_root());
	adv_epoch(mt);

	for(uint64_t i=0;i<56;i+=INTERVAL){
		mt->insert({i});
	}
	adv_epoch(mt);

	//print_tree_asline(mt->get_root());
	void *copy = copy_tree(mt->get_root());

	adv_epoch(mt);
	for(int i=1;i<56;i+=INTERVAL){
		mt->remove(i);
	}

	set_failed_epoch(globalepoch);
	undo_all(mt);

	//print_tree_asline(mt->get_root());
	assert(is_same_tree(mt->get_root(), copy, true));
}

void test_simple(MockMasstree *mt){
	//print_tree_asline(mt->get_root());

	for(uint64_t i=0;i<56;i++){
		if(i%INTERVAL < 2) continue;
		mt->insert({i});
	}

	//print_tree_asline(mt->get_root());
	adv_epoch(mt);

	for(uint64_t i=0;i<56;i+=INTERVAL){
		mt->insert({i});
	}
	adv_epoch(mt);

	//print_tree_asline(mt->get_root());
	void *copy = copy_tree(mt->get_root());

	adv_epoch(mt);
	for(uint64_t i=1;i<56;i+=INTERVAL){
		mt->insert({i});
	}

	set_failed_epoch(globalepoch);
	undo_all(mt);

	//print_tree_asline(mt->get_root());
	assert(is_same_tree(mt->get_root(), copy, true));
}

void do_experiment(std::string fnc_name, void (*fnc)(MockMasstree *)){
	auto mt = new MockMasstree();
	mt->thread_init(0);
	globalepoch = 1;
	GH::node_logger.init(0);

	printf("%s\n", (fnc_name + " begin").c_str());
	fnc(mt);
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	GH::node_logger.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);



int main(){
	DO_EXPERIMENT(test_simple)
	DO_EXPERIMENT(test_remove)

	return 0;
}
