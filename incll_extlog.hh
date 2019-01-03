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
 * Log
 */

#pragma once

#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <atomic>
#include <cassert>

#include "incll_extflush.hh"

typedef uint64_t mrcu_epoch_type;
extern volatile mrcu_epoch_type globalepoch;

class ExtNodeLogger{
private:
	struct nvm_logrec_node{
		size_t size_;
		void* node_addr_;
		uint64_t validity;
		char node_content_[0];

		bool check_validity(){
			return validity == entry_valid_magic;
		}
	};

	typedef uint64_t index;
	index curr;
	index last_flush;
	size_t log_no;
	size_t active_records;
	void *root;
	char *buf_;
public:
	static constexpr const size_t buf_size = (1ull << 30);
	static constexpr const size_t entry_meta_size = sizeof(nvm_logrec_node);
	static constexpr const uint8_t entry_valid_magic = 18;

	index get_last_flush(){
		return last_flush;
	}

	void init(size_t tid){
		destroy();
		log_no = tid;
		buf_ = (char*)malloc(buf_size);
	}

	void destroy(){
		curr = 0;
		last_flush = 0;
		active_records = 0;
		if(buf_)
			free(buf_);
		buf_ = nullptr;
	}

	void checkpoint(){
		last_flush = curr;
		active_records = 0;
	}

	void print_stats(){
		printf("Log: curr %lu, last_flush %lu "
				"num records %lu log size %lu\n",
			curr, last_flush, active_records, buf_size);
	}

	size_t get_total_size(){
		return get_log_meta_size() + buf_size;
	}

	size_t get_log_meta_size(){
		return sizeof(*this);
	}

	size_t get_entry_size(size_t size){
		return entry_meta_size + size;
	}

	template <typename N>
	void record(N* node){
		DBGLOG("Recording node %p le %lu %s ge:%lu keys:%d curr:%lu lf:%lu",
			(void*)node, node->loggedepoch, (node->isleaf()) ? "leaf":"internode",
			globalepoch, node->number_of_keys(), curr, last_flush
		)

		void* node_ptr = (void*)node;
		size_t node_size = node->allocated_size();

		char *entry = (char*)buf_ + curr;
		nvm_logrec_node *lr =
				reinterpret_cast<nvm_logrec_node *>(entry);
		size_t entry_size = get_entry_size(node_size);

		lr->size_ = entry_size;
		lr->node_addr_ = node_ptr;
		lr->validity = entry_valid_magic;
		std::memcpy(lr->node_content_, node_ptr, node_size);

		void *beg = (void*)lr->node_content_;
		//void *end = ;
		sync_range(beg, (void*)((char*)beg + node_size));

		curr += entry_size;
		active_records++;

		beg = (void*)&curr;
		//end = ;
		sync_range(beg, (void*)((char*)beg + sizeof(curr)));
	}


	void* get_tree_root(){
		return root;
	}

	void set_log_root(void* root_){
		DBGLOG("save root:%p for future", root_)
		root = root_;
	}


	template <typename N>
	void undo(N* rand_node){
		(void)(rand_node);
		DBGLOG("undo %lu records", active_records)

		while(last_flush != curr){
			char *entry = (char*)buf_ + last_flush;

			nvm_logrec_node *lr =
					reinterpret_cast<nvm_logrec_node *>(entry);
			size_t entry_size = lr->size_;

			size_t copy_size = entry_size - sizeof(*lr);
			std::memcpy(lr->node_addr_, (void*)lr->node_content_, copy_size);

			//get to be undone node
			N* node = (N*)lr->node_addr_;

#ifdef INCLL
			if(node->isleaf()){
				//fix insert for modstate and version value
				node->to_leaf()->fix_all();
			}else{
				node->fix_lock();
			}
#else //incll
			node->fix_lock();
#endif //incll

			DBGLOG("Undoing node %p le %lu %s ge:%lu keys:%d curr:%lu lf:%lu",
				(void*)node, node->loggedepoch, (node->isleaf()) ? "leaf":"internode",
				globalepoch, node->number_of_keys(), curr, last_flush
			)

			last_flush += entry_size;
		}

		active_records = 0;
	}

	template <typename N>
	void undo_next_prev(N* rand_node, index temp_flush){
		(void)(rand_node);

		while(temp_flush != curr){
			char *entry = (char*)buf_ + temp_flush;

			nvm_logrec_node *lr =
					reinterpret_cast<nvm_logrec_node *>(entry);
			size_t entry_size = lr->size_;

			N* node = (N*)lr->node_addr_;
			DBGLOG("Undoing next prev node %p le %lu %s ge:%lu inkeys:%d curr:%lu lf:%lu",
				(void*)node, node->loggedepoch, (node->isleaf()) ? "leaf":"internode",
				globalepoch, node->number_of_keys(), curr, last_flush
			)

			//fix next and prev
			if(node->isleaf()){
				auto *ln = node->to_leaf();
				auto *prev = ln->get_prev_safe();
				auto *next = ln->get_next_safe();

				ln->next_.ptr 	= next;
				ln->prev_ 		= prev;

				if(prev)
					prev->next_.ptr = ln;

				if(next)
					next->prev_ = ln;
			}

			temp_flush += entry_size;
		}
	}
};
