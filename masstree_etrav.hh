/*
 * tree traversal
 */

#pragma once

#include <queue>

template <typename N, typename LN>
void get_children_leaf(std::queue<N*>& q, LN *ln){
	typename LN::permuter_type perm = ln->permutation_;

	for (int idx = 0; idx < perm.size(); ++idx) {
		int p = perm[idx];
		typename LN::leafvalue_type lv = ln->lv_[p];
		if(ln->is_layer(p)) {
			//todo implement later
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
	size_t num_nodes = 0;
	N* node = root;

	std::queue<N*> q;

	q.push(node);

	while(!q.empty()){
		node = q.front();
		q.pop();

		if(node->isleaf())
			num_nodes += get_num_keys_leaf(node->to_leaf());

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

		node->print(stdout, "", 0, 0);

		get_children(q, node);
	}
}

template <typename N>
void clone_tree(N* root){
	assert(0 && "Not implemented");
}

template <typename N>
void compare_tree(N* root, bool same){
	assert(0 && "Not implemented");
}

