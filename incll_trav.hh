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

/*
 * tree traversal code
 * stats
 * print
 */

#pragma once

#include <queue>
#include <cstdlib>
#include <cstdio>
#include <cassert>

template <typename N, typename LN>
void get_children_leaf(std::queue<N*>& q, LN *ln){
	typename LN::permuter_type perm = ln->permutation_;

	for (int idx = 0; idx < perm.size(); ++idx) {
		int p = perm[idx];
		typename LN::leafvalue_type lv = ln->lv_[p];
		if(ln->is_layer(p)) {
			assert(0 && "not implemented system wide");
			N *node = lv.layer();
			while (!node->is_root())
				node = node->maybe_parent();

			q.push(node);
		}
	}
}

template <typename N, typename IN>
void get_children_internode(std::queue<N*>& q, IN* in){
	//+1 for size of internode
	for (int p = 0; p <= in->size(); ++p) {
		auto child = in->child_[p];
		if (child){
			q.push(child);
		}
	}
}

template <typename LN>
size_t get_num_keys_leaf(LN *ln){
	typename LN::permuter_type perm = ln->permutation_;
	return perm.size();
}

template <typename IN>
size_t get_num_keys_internode(IN* in){
	if(in->child_[in->size()])
		return in->size() + 1;
	else
		return in->size();
}

template <typename N>
void get_children(std::queue<N*>& q, N *node){
	if(node->isleaf()){
		get_children_leaf(q, node->to_leaf());
	}else{
		get_children_internode(q, node->to_internode());
	}
}

template <typename N>
size_t get_tree_size(N* root){
	size_t num_keys = 0;
	N* node = root;

	std::queue<N*> q;

	q.push(node);

	while(!q.empty()){
		node = q.front();
		q.pop();

		if(node->isleaf())
			num_keys += get_num_keys_leaf(node->to_leaf());

		get_children(q, node);
	}

	return num_keys;
}


template <typename N>
size_t get_num_nodes(N* root){
	size_t num_nodes = 0;
	N* node = root;

	std::queue<N*> q;

	q.push(node);

	while(!q.empty()){
		node = q.front();
		q.pop();

		if(node->isleaf())
			num_nodes++;

		get_children(q, node);
	}

	return num_nodes;
}

template <typename N>
void print_tree(N* root){
	N* node = root;

	std::queue<N*> q;

	q.push(node);

	while(!q.empty()){
		node = q.front();
		q.pop();

		node->print_node();

		get_children(q, node);
	}
}

template <typename N>
void print_tree_asline(N* root){
	N* node = root;
	printf("root %p\n", (void*)root);

	std::queue<N*> q;

	q.push(node);

	size_t num_nodes = 0;
	size_t num_keys = 0;
	while(!q.empty()){
		node = q.front();
		q.pop();

		get_children(q, node);

		if(node->isleaf()){
			size_t nkeys = get_num_keys_leaf(node->to_leaf());
			printf("ln%lu %p ", nkeys, (void*)node);
			num_keys += nkeys;
		}else{
			size_t nkeys = get_num_keys_internode(node->to_internode());
			printf("in%lu %p ", nkeys, (void*)node);
		}
		num_nodes++;
	}
	printf("\nnum nodes %lu num keys %lu\n", num_nodes, num_keys);
}

#ifdef MTAN
template <typename N>
void report_mtan_tree(N* root){
	N* node = root;
	size_t num_keys = 0;
	size_t num_nodes = 0;
	size_t num_nodes_lf = 0;
	size_t num_nodes_in = 0;
	size_t recorded_lf = 0;
	size_t recorded_in = 0;

	std::queue<N*> q;

	q.push(node);

	while(!q.empty()){
		node = q.front();
		q.pop();

		num_nodes++;
		if(node->isleaf()){
			auto* lf = node->to_leaf();
			num_keys += get_num_keys_leaf(lf);
			recorded_lf += lf->is_recorded;
			++num_nodes_lf;
		}else{
			auto* in = node->to_internode();
			++num_nodes_in;
			recorded_lf += in->is_recorded;
		}

		get_children(q, node);
	}

	printf("Nodes:%lu lf:%lu in:%lu\n", num_nodes, num_nodes_lf, num_nodes_in);
	printf("Keys:%lu\n", num_keys);
	printf("Recorded lf:%lu in:%lu\n", recorded_lf, recorded_in);
}
#endif //mtan
