#include "test_mocktree.hh"
#include <vector>
#include <thread>
#include "incll_globals.hh"
#include "incll_trav.hh"
#include "incll_copy.hh"

__thread typename MockMasstree::table_params::threadinfo_type* MockMasstree::ti = nullptr;
volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;
int delaycount = 0;
kvtimestamp_t initial_timestamp;


#define N_OPS 80
#define INTERVAL 8


void insert_t1(MockMasstree *mt){
	globalepoch=2;
	void *copy = copy_tree(mt->get_root());

	mt->insert({3});
	failedepoch=2;
	globalepoch=3;


	assert(is_same_tree(mt->get_root(), copy));
	globalepoch=4; //because forced recovery

	mt->insert({4, 5, 6});
	failedepoch=4;
	globalepoch=5;

	GH::node_logger.undo(mt->get_root_assignable());
	assert(is_same_tree(mt->get_root(), copy));

}


void remove_t1(MockMasstree *mt){
	mt->insert({1,2,3,4,5,6,7,8});

	globalepoch=2;
	void *copy = copy_tree(mt->get_root());

	mt->insert({0, 9});
	mt->remove({4, 5, 6});
	failedepoch=2;
	globalepoch=3;

	GH::node_logger.undo(mt->get_root_assignable());
	assert(is_same_tree(mt->get_root(), copy));
}

void remove_insert_t1(MockMasstree *mt){
	globalepoch=1;
	mt->insert({4,5,10,11,3,12,13,14,6,7,8,9,1,2});

	globalepoch=2;

	//print_tree(mt->get_root());

	mt->remove({4});

	//print_tree(mt->get_root());

}

void remove_insert_multinode_t1(){

}

void do_experiment(std::string fnc_name, void (*fnc)(MockMasstree *)){
	auto mt = new MockMasstree();
	mt->thread_init(0);
	GH::node_logger.init(0);

	printf("%s\n", (fnc_name + " begin").c_str());
	fnc(mt);
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	GH::node_logger.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);


int main(){
	//DO_EXPERIMENT(insert_t1)
	//DO_EXPERIMENT(remove_t1)
	DO_EXPERIMENT(remove_insert_t1)

	return 0;
}
