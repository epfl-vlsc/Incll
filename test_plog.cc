#include <thread>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "incll_globals.hh"
#include "test_mocklist.hh"
#include "incll_copy.hh"
#include "incll_pextlog.hh"

#define INTERVAL 8

volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;
int delaycount = 0;

void test_log_simple(){
	Node *root = get_simple_list(SMALL_SIZE);
	printf("entry size:%lu\n", get_node_entry_size(root));

	GH::node_logger->record(root);
	root->set_val(SMALL_SIZE);
	GH::node_logger->checkpoint();
	auto copy = copy_vals(root);

	print_nodes(root);

	modify_nodes(root, TAINT_VAL);

	print_nodes(root);

	GH::node_logger->undo(root);
	assert(is_list_same(copy, root));
}


void do_experiment(std::string fnc_name, void (*fnc)()){
	GH::plog_allocator.init();
	GH::node_logger = GH::plog_allocator.init_plog(0);
	printf("%s\n", (fnc_name + " begin").c_str());
	fnc();
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	GH::plog_allocator.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);

int main(){
	DO_EXPERIMENT(test_log_simple);
	//DO_EXPERIMENT(test_log_circular); //disabled

	return 0;
}
