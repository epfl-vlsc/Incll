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
 * copy operations
 * create a copy of the whole tree
 * compare if two trees are same
 */

#pragma once

#include <cassert>
#include <iostream>
#include <bitset>

#include "incll_configs.hh"
#include "incll_trav.hh"
#include "masstree_print.hh"

template <typename V>
void assert_diff(std::string str, V v1, V v2){
	if(v1 != v2){
		std::cout << str << " " << v1 << " " << v2 << std::endl;
		assert(v1 == v2);
	}
}

template <typename LN>
void print_mem_contents_ln(LN* n1, LN* n2, size_t node_size){
	char* addr = (char*)&(n1->loggedepoch);
	size_t skip_size = addr-(char*)n1 + sizeof(n1->loggedepoch);

	char *n1_shifted = (char*)n1 + skip_size;
	char *n2_shifted = (char*)n2 + skip_size;
	uint64_t compare_size = node_size - skip_size;

	char *ptr1 = n1_shifted;
	char *ptr2 = n2_shifted;
	for(uint64_t i=0;i<compare_size;++i){
		printf("%x %x|", *ptr1, *ptr2);
		ptr1++;
		ptr2++;
		if(i % 10 == 0) printf("\n");

		if(*ptr1 != *ptr2){
			printf("%x %x|", *ptr1, *ptr2);
			void *iksuf_ptr = &n1->iksuf_;
			int ptr_diff = ptr1 - (char*)n1;
			int iksuf_diff = (char*)iksuf_ptr - (char*)n1;

			printf("\n\nat tree %p from start %d node size %lu iksuf %d\n",
					ptr1, ptr_diff, node_size, iksuf_diff);
			assert(*ptr1 == *ptr2);
		}
	}
}


template <typename LN>
void print_mem_contents_in(LN* n1, LN* n2, size_t node_size){
	char* addr = (char*)&(n1->loggedepoch);
	size_t skip_size = addr-(char*)n1 + sizeof(n1->loggedepoch);

	char *n1_shifted = (char*)n1 + skip_size;
	char *n2_shifted = (char*)n2 + skip_size;
	uint64_t compare_size = node_size - skip_size;

	char *ptr1 = n1_shifted;
	char *ptr2 = n2_shifted;
	for(uint64_t i=0;i<compare_size;++i){
		printf("%x %x|", *ptr1, *ptr2);
		ptr1++;
		ptr2++;
		if(i % 10 == 0) printf("\n");

		if(*ptr1 != *ptr2){
			printf("%x %x|", *ptr1, *ptr2);
			void *parent_ptr = &n1->parent_;
			int ptr_diff = ptr1 - (char*)n1;
			int parent_diff = (char*)parent_ptr - (char*)n1;

			printf("\n\nat tree %p from start %d node size %lu parent %d\n",
					ptr1, ptr_diff, node_size, parent_diff);
			assert(*ptr1 == *ptr2);
		}
	}
}

template <typename LN>
void print_version_contents(LN* n1, LN* n2){
	const char* format_str =
		"%s lock:%u ins:%u spl:%u "
		"del:%u cha:%u rt:%u\n";

	printf(format_str, "Tree ", n1->locked(), n1->inserting(),
			n1->splitting(), n1->deleted(),
			n1->is_root());

	printf(format_str, "Copy ", n2->locked(), n2->inserting(),
			n2->splitting(), n2->deleted(),
			n2->is_root());

	std::bitset<32> v1(n1->version_value());
	std::bitset<32> v2(n2->version_value());

	std::cout << "v1:" << v1 << std::endl;
	std::cout << "v2:" << v2 << std::endl;
}

template <typename IN>
void print_in_difference(IN* n1, IN* n2){
	size_t n1_size = n1->allocated_size();
	size_t n2_size = n2->allocated_size();

	assert_diff("size", n1_size, n2_size);
	if(n1->version_value() != n2->version_value()){
		print_version_contents(n1, n2);
	}
	assert_diff("version", n1->version_value(), n2->version_value());
	assert_diff("nkeys", n1->nkeys_, n2->nkeys_);
	assert_diff("height", n1->height_, n2->height_);
	assert_diff("parent", n1->parent_, n2->parent_);

	for (int idx = 0; idx < n1->size(); ++idx) {
		assert_diff("child", n1->child_[idx], n2->child_[idx]);
		assert_diff("ikey0", n1->ikey0_[idx], n2->ikey0_[idx]);
	}

	if(n1->child_[n1->size()] || n2->child_[n1->size()]){
		assert_diff("child+1", n1->child_[n1->size()], n2->child_[n1->size()]);
	}

	print_mem_contents_in(n1, n2, n1_size);
}


template <typename LN>
void print_ln_difference(LN* n1, LN* n2){
	size_t n1_size = n1->allocated_size();
	size_t n2_size = n2->allocated_size();

	typename LN::permuter_type perm1 = n1->permutation_;
	typename LN::permuter_type perm2 = n2->permutation_;

	assert_diff("size", n1_size, n2_size);

	if(n1->version_value() != n2->version_value()){
		print_version_contents(n1, n2);
	}
	assert_diff("version", n1->version_value(), n2->version_value());
	//assert_diff("le", n1->loggedepoch, n2->loggedepoch);
	assert_diff("modstate", n1->modstate_, n2->modstate_);
	assert_diff("extrasize", n1->extrasize64_, n2->extrasize64_);
	assert_diff("permutation", n1->permutation_, n2->permutation_);
	assert_diff("prev", n1->prev_, n2->prev_);
	assert_diff("next", n1->next_.ptr, n2->next_.ptr);
	assert_diff("parent", n1->parent_, n2->parent_);
	assert_diff("ksuf", n1->ksuf_, n2->ksuf_);
	assert_diff("perm size", perm1.size(), perm2.size());
	//assert_diff("phantom", n1->phantom_epoch_, n2->phantom_epoch_);
	//assert_diff("created", n1->created_at_, n2->created_at_);

	for (int idx = 0; idx < perm1.size(); ++idx) {
		int p1 = perm1[idx];
		int p2 = perm2[idx];

		assert_diff("lv", n1->lv_[p1].value(), n2->lv_[p2].value());
		assert_diff("keylenx", n1->keylenx_[p1], n2->keylenx_[p2]);
		assert_diff("ikey0", n1->ikey0_[p1], n2->ikey0_[p2]);
	}


	print_mem_contents_ln(n1, n2, n1_size);
}

template <typename LN>
bool is_same_leaf_fine_grained(LN* n1, LN* n2){
	size_t n1_size = n1->allocated_size();
	size_t n2_size = n2->allocated_size();

	typename LN::permuter_type perm1 = n1->permutation_;
	typename LN::permuter_type perm2 = n2->permutation_;

	//todo logged epoch, phantom_epoch, created_at, not checked

	bool is_same = (n1_size == n2_size
			&& n1->version_value() == n2->version_value()
			//&& n1->loggedepoch == n2->loggedepoch
			&& n1->modstate_ == n2->modstate_
			&& n1->extrasize64_ == n2->extrasize64_
			&& n1->permutation_ == n2->permutation_
			&& n1->prev_ == n2->prev_
			&& n1->next_.ptr == n2->next_.ptr
			&& n1->parent_ == n2->parent_
			&& n1->ksuf_ == n2->ksuf_
			&& perm1.size() == perm2.size()
			);

	if(is_same){
		for (int idx = 0; idx < perm1.size(); ++idx) {
			int p1 = perm1[idx];
			int p2 = perm2[idx];

			is_same = (is_same
					&& n1->lv_[p1].value() == n2->lv_[p2].value()
					&& n1->keylenx_[p1] == n2->keylenx_[p2]
					&& n1->ikey0_[p1] == n2->ikey0_[p2]
					);
		}
	}


	return is_same;
}

template <typename IN>
bool is_same_internode_fine_grained(IN* n1, IN* n2){
	size_t n1_size = n1->allocated_size();
	size_t n2_size = n2->allocated_size();

	//todo logged epoch, not checked

	bool is_same = (n1_size == n2_size
			&& n1->version_value() == n2->version_value()
			//&& n1->loggedepoch == n2->loggedepoch
			&& n1->nkeys_ == n2->nkeys_
			&& n1->parent_ == n2->parent_
			);

	if(is_same){
		for (int idx = 0; idx < n1->size(); ++idx) {
			is_same = (is_same
					&& n1->child_[idx] == n2->child_[idx]
					&& n1->ikey0_[idx] == n2->ikey0_[idx]
					);
		}

		if(n1->child_[n1->size()] || n2->child_[n1->size()]){
			is_same = (is_same
					&& n1->child_[n1->size()] == n2->child_[n1->size()]
					);
		}
	}

	return is_same;
}

template <typename ILN>
bool is_same_mem(ILN* n1, ILN* n2){
	if(n1->allocated_size() != n2->allocated_size())
		return false;

	return (memcmp((void*)n1, (void*)n2, n1->allocated_size()) == 0);
}

template <typename LN>
bool is_same_leaf(LN* n1, LN* n2){
#ifdef INCLL
	return is_same_leaf_fine_grained(n1, n2);
#else //incll
	return is_same_mem(n1, n2);
#endif //incll
}

template <typename IN>
bool is_same_internode(IN* n1, IN* n2){
#ifdef INCLL
	return is_same_mem(n1, n2);
#else //incll
	return is_same_mem(n1, n2);
#endif //incll
}



template <typename N>
void print_diff_between_nodes(N* n1, N* n2){
	printf("Tree: ");
	n1->print_node();

	printf("Copy: ");
	n2->print_node();

	//detailed comparison for leaf nodes
	if(n1->isleaf() && n2->isleaf()){
		print_ln_difference(n1->to_leaf(), n2->to_leaf());
	}

	//detailed comparison for internodes
	if(!n1->isleaf() && !n2->isleaf()){
		print_in_difference(n1->to_internode(), n2->to_internode());
	}
}


template <typename N>
bool is_same_node(N* n1, N* n2, bool apply_incll=false){
	bool is_same = false;

	if(n1->isleaf() && n2->isleaf()){
		auto *ln1 = n1->to_leaf();
		auto *ln2 = n2->to_leaf();
#ifdef INCLL
		if(apply_incll){
			ln1->undo_incll();
			ln1->fix_all();
			ln2->fix_all(); //todo check here
		}
#else //incll
		(void)(apply_incll);
#endif //incll
		is_same = is_same_leaf(ln1, ln2);
	}else if(!n1->isleaf() && !n2->isleaf()){
		auto *in1 = n1->to_internode();
		auto *in2 = n2->to_internode();
		is_same = is_same_internode(in1, in2);
	}

	if(!is_same){
		print_diff_between_nodes(n1, n2);
	}

	return is_same;
}

template <typename N>
void clear_copy(void*& copy){
	assert(copy);
	std::vector<N*> *v_copy = (std::vector<N*>*)copy;

	for(N* node_copy: *v_copy){
		free(node_copy);
	}

	free(v_copy);
	copy = nullptr;
}


template <typename N>
bool is_same_tree(N* root, void* copy, bool apply_incll=false){
	//Assumption: N is a node, copy is a filled vector of node pointers
	std::queue<N*> q;
	std::vector<N*> *v_copy = (std::vector<N*>*)copy;

	N* node = root;
	q.push(node);

	for(N* node_copy: *v_copy){
		if(q.empty())
			return false;

		node = q.front();
		q.pop();

		if(!is_same_node(node, node_copy, apply_incll))
			return false;

		get_children(q, node);
	}

	return true;
}

template <typename ILN>
ILN* copy_node_specialized(ILN* iln){
	//Assumption: leaf or internode type
	//todo make sure this part is correct, size stuff, iksuf, ksuf stuff
	size_t iln_size = iln->allocated_size();

	ILN *iln_copy = (ILN*) malloc(iln_size);
	memcpy(iln_copy, iln, iln_size);

	return iln_copy;
}

template <typename N>
N* copy_node(N *node){
	if(node->isleaf()){
		return copy_node_specialized(node->to_leaf());
	}else{
		return copy_node_specialized(node->to_internode());
	}
}

template <typename N>
void* copy_tree(N* root){
	size_t num_keys = 0;

	N* node = root;
	N* node_copy = nullptr;

	std::queue<N*> q;
	std::vector<N*> *v_copy = new std::vector<N*>;

	q.push(node);

	while(!q.empty()){
		node = q.front();
		q.pop();

		node_copy = copy_node(node);
		assert(is_same_node(node, node_copy));
		v_copy->push_back(node_copy);

		if(node->isleaf())
			num_keys += get_num_keys_leaf(node->to_leaf());

		get_children(q, node);
	}

	DBGLOG("copy num_nodes %lu with %lu keys",
			v_copy->size(), num_keys)
	return (void*)v_copy;
}






