#include <thread>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "incll_globals.hh"
#include "test_mocklist.hh"
#include "incll_copy.hh"
#include "incll_extlog.hh"

#define INTERVAL 8

volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;

void test_log_simple(){
	Node *root = get_simple_list(SMALL_SIZE);
	//printf("entry size:%lu\n", get_node_entry_size(root));

	GH::node_logger.record(root);
	root->set_val(SMALL_SIZE);
	GH::node_logger.checkpoint();
	auto copy = copy_vals(root);

	//print_nodes(root);

	modify_nodes(root, TAINT_VAL);

	//print_nodes(root);

	GH::node_logger.undo(root);
	assert(is_list_same(copy, root));
}

void test_log_circular(){
	Node *root = get_simple_list(BIG_SIZE);

	record_nodes(root);
	//GH::node_logger.print_stats();
	GH::node_logger.checkpoint();

	//print_nodes(root);
	auto copy = copy_vals(root);

	Node* n4 = (*root)[1];
	modify_nodes(n4, 100);


	//print_nodes(root);

	GH::node_logger.undo(root);

	//print_nodes(root);
	assert(is_list_same(copy, root));

}

void do_experiment(std::string fnc_name, void (*fnc)()){
	GH::node_logger.init(0);
	printf("%s\n", (fnc_name + " begin").c_str());
	fnc();
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
	GH::node_logger.destroy();
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);

int main(){
	DO_EXPERIMENT(test_log_simple);
	//DO_EXPERIMENT(test_log_circular); //disabled

	return 0;
}
