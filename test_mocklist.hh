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
 * custom nodes
 */

#pragma once
#include <vector>
#include <cstdlib>
#include <cstdio>

#include "incll_extlog.hh"
#include "incll_globals.hh"

#define SMALL_SIZE 3
#define BIG_SIZE 8
#define TAINT_VAL 100

struct Node{
	Node *next;
	uint8_t val;
	bool is_root_;

	Node(): next(nullptr), val(0), is_root_(false){}

	size_t allocated_size(){
		return sizeof(*this);
	}

	void set_next(Node *next_){
		next = next_;
	}

	void set_val(uint8_t val_){
		val = val_;
	}

	void set_as_root(){
		is_root_ = true;
	}

	Node* operator[](int i){
	    Node *root = this;
		int c = 0;
	    while(root){
	    	if(i == c)
	    		return root;

	    	root = root->next;
	    	c++;
	    }

	    assert(false);
	    return nullptr;
	}
	bool is_root(){
		return is_root_;
	}
	bool isleaf(){
		return true;
	}

	void fix_lock(){}

	void fix_all(){}

	Node* to_leaf(){
		return this;
	}


};

Node* get_simple_list(uint8_t N){
	Node *node = nullptr, *prev = nullptr;
	Node *root = nullptr;

	for(uint8_t i=0;i<N;++i){
		node = new Node;
		node->set_val(i);

		if(!i)
			root = node;

		if(prev)
			prev->set_next(node);

		prev = node;
	}

	root->set_as_root();
	return root;
}

void print_nodes(Node* root){
	while(root){
		printf("%u ", root->val);
		root = root->next;
	}printf("\n");
}

void modify_nodes(Node* root, uint8_t val){
	while(root){
		GH::node_logger->record(root);
		root->set_val(val);
		root = root->next;
	}
}

void record_nodes(Node* root){
	while(root){
		GH::node_logger->record(root);
		root = root->next;
	}
}

std::vector<uint8_t>* copy_vals(Node* root){
	std::vector<uint8_t>* v = new std::vector<uint8_t>;
	while(root){
		v->push_back(root->val);
		root = root->next;
	}
	return v;
}

bool is_list_same(std::vector<uint8_t>* v, Node* root){
	for(auto e:*v){
		if(e != root->val)
			return false;
		root = root->next;
	}

	return true;
}

size_t get_node_entry_size(Node* root){
	return GH::node_logger->get_entry_size(root->allocated_size());
}
