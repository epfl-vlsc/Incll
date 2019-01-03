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
volatile void *global_masstree_root = nullptr;

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

	return 0;
}
