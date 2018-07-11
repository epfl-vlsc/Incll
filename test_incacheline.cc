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

void adv_epoch(MockMasstree *mt){
	globalepoch++;
	GH::node_logger.set_log_root(mt->get_root());
	printf("new ge:%lu\n", globalepoch);
}

void undo_all(MockMasstree *mt){
	void *undo_root = GH::node_logger.get_tree_root();
	mt->set_root(undo_root);
	auto last_flush = GH::node_logger.get_last_flush();
	GH::node_logger.undo(mt->get_root());
	GH::node_logger.undo_next_prev(mt->get_root(), last_flush);
}

void set_failed_epoch(mrcu_epoch_type fe){
	failedepoch = fe;
	printf("fe:%lu\n", failedepoch);
}

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

	mt->insert(9, 8);

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
	mt->insert(8, 6);

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
		mt->insert(i, i + 2);
	}
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert(2, 2);
	mt->insert({0});
	mt->remove({5,7});
	mt->insert({1,18,7});

	set_failed_epoch(2);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
}

void kill_root_leaf(MockMasstree *mt){
	for(int i=0;i<24;++i){
		mt->insert(i, i + 2);
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

void two_epochs(MockMasstree *mt){
	mt->insert({9,5,1,3,2,4,6});
	adv_epoch(mt);

	void *copy = copy_tree(mt->get_root());

	mt->insert({0,8});
	adv_epoch(mt);

	mt->insert({7});

	set_failed_epoch(3);

	undo_all(mt);

	assert(is_same_tree(mt->get_root(), copy, true));
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
	DO_EXPERIMENT(insert_incll)
	DO_EXPERIMENT(update_incll)
	DO_EXPERIMENT(insert_log)
	DO_EXPERIMENT(update_log)
	DO_EXPERIMENT(remove_log)
	DO_EXPERIMENT(mix_log)
	DO_EXPERIMENT(split_log)
	DO_EXPERIMENT(kill_root_leaf)
	DO_EXPERIMENT(two_epochs)

	return 0;
}
