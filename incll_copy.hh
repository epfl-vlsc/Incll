/*
 * copy operations
 * create a copy of the whole tree
 * compare if two trees are same
 */

#pragma once

#include "incll_trav.hh"
#include "masstree_print.hh"


template <typename ILN>
int clever_mem_cmp(ILN* n1, ILN* n2, size_t node_size){
	char* addr = (char*)&(n1->loggedepoch);
	size_t skip_size = addr-(char*)n1 + sizeof(n1->loggedepoch);

	char *n1_shifted = (char*)n1 + skip_size;
	char *n2_shifted = (char*)n2 + skip_size;

	return memcmp((void*)n1_shifted, (void*)n2_shifted, node_size - skip_size);
}

template <typename ILN>
bool is_same_node_specialized(ILN* n1, ILN* n2){
	size_t n1_size = n1->allocated_size();
	size_t n2_size = n2->allocated_size();

	if(n1_size != n2_size)
		return false;

	if(clever_mem_cmp(n1, n2, n1_size) != 0){
		n1->print_node();
		n2->print_node();

		return false;
	}


	return true;
}

template <typename N>
bool is_same_node(N* n1, N* n2){
	bool is_n1_leaf = n1->isleaf();
	bool is_n2_leaf = n2->isleaf();

	if(is_n1_leaf != is_n2_leaf)
		return false;

	if(is_n1_leaf){
		return is_same_node_specialized(
				n1->to_leaf(), n2->to_leaf());
	}else{
		return is_same_node_specialized(
				n1->to_internode(), n2->to_internode());
	}
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
bool is_same_tree(N* root, void* copy){
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

		if(!is_same_node(node, node_copy))
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

		get_children(q, node);
	}

	printf("copy num_nodes %lu\n", v_copy->size());
	return (void*)v_copy;
}






