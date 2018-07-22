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

#ifdef COLLECT_STATS
extern size_t n_ge_changes;

template <typename N>
void print_tree_summary(N* root, bool reset_nodes=false){
	N* node = root;

	std::queue<N*> q;

	q.push(node);

	size_t n_nodes = 0;
	size_t n_keys = 0;
	size_t n_internodes = 0;
	size_t n_leafs = 0;

	//ln
	double tot_ln_inserts = 0;
	double tot_ln_records = 0;
	double tot_ln_incll_inserts = 0;
	double tot_ln_incll_updates = 0;
	double tot_ln_incll_logs = 0;
	double tot_ln_extlog_logs = 0;
	//in
	double tot_in_records = 0;

	while(!q.empty()){
		node = q.front();
		q.pop();

		get_children(q, node);

		if(node->isleaf()){
			auto *ln = node->to_leaf();
			size_t nkeys = get_num_keys_leaf(ln);
			n_keys += nkeys;
			n_leafs++;
			tot_ln_inserts += ln->n_inserts;
			tot_ln_records += ln->n_records;
			tot_ln_incll_inserts += ln->n_incll_inserts;
			tot_ln_incll_updates += ln->n_incll_updates;
			tot_ln_incll_logs += ln->n_incll_logs;
			tot_ln_extlog_logs += ln->n_extlog_logs;

			if(reset_nodes){
				ln->n_inserts = 0;
			}
		}else{
			auto *in = node->to_internode();
			get_num_keys_internode(in);
			n_internodes++;
			tot_in_records += in->n_records;
		}
		n_nodes++;
	}

	//ln
	double avg_ln_inserts = tot_ln_inserts / n_leafs;
	double avg_ln_records = tot_ln_records / n_leafs;
	double avg_ln_incll_inserts = tot_ln_incll_inserts / n_leafs;
	double avg_ln_incll_updates = tot_ln_incll_updates / n_leafs;
	double avg_ln_incll_logs = tot_ln_incll_logs / n_leafs;
	double avg_ln_extlog_logs = tot_ln_extlog_logs / n_leafs;
	//in
	double avg_in_records = tot_in_records / n_internodes;

	printf("Tree summary\n"
			"Nodes:%lu IN:%lu LN:%lu Keys:%lu\n"
			"Epochs:%lu\n"
			"Tot LN-inserts %f Avg LN-inserts per node:%f per epoch:%f\n"
			"Tot LN-records %f Avg LN-records per node:%f per epoch:%f\n"
			"Tot LN-incll ins %f Avg LN-incll ins per node:%f per epoch:%f\n"
			"Tot LN-incll upd %f Avg LN-incll upd per node:%f per epoch:%f\n"
			"Tot LN-incll log %f Avg LN-incll log per node:%f per epoch:%f\n"
			"Tot LN-extlog log %f Avg LN-extlog log per node:%f per epoch:%f\n"
			"Tot IN-records %f Avg IN-records per node:%f per epoch:%f\n",
			n_nodes, n_internodes, n_leafs, n_keys,
			n_ge_changes,
			tot_ln_inserts, avg_ln_inserts, avg_ln_inserts/n_ge_changes,
			tot_ln_records, avg_ln_records, avg_ln_records/n_ge_changes,
			tot_ln_incll_inserts, avg_ln_incll_inserts, avg_ln_incll_inserts/n_ge_changes,
			tot_ln_incll_updates, avg_ln_incll_updates, avg_ln_incll_updates/n_ge_changes,
			tot_ln_incll_logs, avg_ln_incll_logs, avg_ln_incll_logs/n_ge_changes,
			tot_ln_extlog_logs, avg_ln_extlog_logs, avg_ln_extlog_logs/n_ge_changes,
			tot_in_records, avg_in_records, avg_in_records/n_ge_changes
			);
}
#endif //collect stats

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

