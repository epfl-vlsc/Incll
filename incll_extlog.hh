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
		bool is_root;
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
	char *buf_;
public:
	static constexpr const size_t buf_size = (1ull << 34);
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
		DBGLOG("Recording node %p le %lu %s ge:%lu inkeys:%d",
			(void*)node, node->loggedepoch,
			(node->isleaf()) ? "leaf":"internode",
			globalepoch,
			(node->isleaf()) ? 0 : node->to_internode()->size()
		)

		void* node_ptr = (void*)node;
		size_t node_size = node->allocated_size();

		char *entry = (char*)buf_ + curr;
		nvm_logrec_node *lr =
				reinterpret_cast<nvm_logrec_node *>(entry);
		size_t entry_size = get_entry_size(node_size);

		//almost circular buffer
		if(curr + entry_size > buf_size){
			printf("Warning in record: back to the beginning of log\n");
			assert(0);
			curr = 0;
			entry = (char*)buf_ + curr;
			lr = reinterpret_cast<nvm_logrec_node *>(entry);
		}

		lr->size_ = entry_size;
		lr->node_addr_ = node_ptr;
		lr->is_root = node->is_root();
		lr->validity = entry_valid_magic;
		std::memcpy(lr->node_content_, node_ptr, node_size);

		void *beg = (void*)lr->node_content_;
		void *end = (void*)((char*)beg + node_size);
		sync_range(beg, end);

		curr += entry_size;
		active_records++;

		beg = (void*)&curr;
		end = (void*)((char*)beg + sizeof(curr));
		sync_range(beg, end);
	}

	template <typename N>
	void undo(N*& root){
		DBGLOG("undo %lu records", active_records)

		while(last_flush != curr){
			char *entry = (char*)buf_ + last_flush;

			nvm_logrec_node *lr =
					reinterpret_cast<nvm_logrec_node *>(entry);
			size_t entry_size = lr->size_;

			//almost circular buffer
			if(!lr->check_validity() || last_flush + entry_size > buf_size){
				printf("Warning in undo: back to the beginning of log\n");
				assert(0);
				last_flush = 0;
				entry = (char*)buf_ + last_flush;
				lr = reinterpret_cast<nvm_logrec_node *>(entry);
				entry_size = lr->size_;
			}

			//get to be undone node
			N* undo_node = (N*)lr->node_content_;
#ifdef INCLL
			//fix insert for modstate and version value
			if(undo_node->inserting()){
				auto *undo_ln = undo_node->to_leaf();
				if(undo_ln->inserting()){
					undo_ln->modstate_ = undo_ln->modstate_remove; //remove
					undo_ln->clear_insert();
				}

			}

#endif //incll


			if(undo_node->locked()){
				undo_node->unlock();
			}

			size_t copy_size = entry_size - sizeof(*lr);
			std::memcpy(lr->node_addr_, (void*)lr->node_content_, copy_size);


			N* node = (N*)lr->node_addr_;
			if(lr->is_root){
				if(!node->isleaf()){
					//printf("change root addr to %p\n", lr->node_addr_);
					root = node;
				}
			}

			DBGLOG("Undoing node %p le %lu %s ge:%lu inkeys:%d",
				(void*)node, node->loggedepoch,
				(node->isleaf()) ? "leaf":"internode",
				globalepoch,
				(node->isleaf()) ? 0 : node->to_internode()->size()
			)

			last_flush += entry_size;
		}

		active_records = 0;
	}

	template <typename N>
	void undo_next_prev(N*& root, index temp_flush){
		(void)(root);

		while(temp_flush != curr){
			char *entry = (char*)buf_ + temp_flush;

			nvm_logrec_node *lr =
					reinterpret_cast<nvm_logrec_node *>(entry);
			size_t entry_size = lr->size_;
			//almost circular buffer
			if(!lr->check_validity() || temp_flush + entry_size > buf_size){
				printf("Warning in undo: back to the beginning of log\n");
				assert(0);
				temp_flush = 0;
				entry = (char*)buf_ + temp_flush;
				lr = reinterpret_cast<nvm_logrec_node *>(entry);
				entry_size = lr->size_;
			}

			N* node = (N*)lr->node_addr_;
			DBGLOG("Undoing next prev node %p le %lu %s ge:%lu inkeys:%d",
				(void*)node, node->loggedepoch,
				(node->isleaf()) ? "leaf":"internode",
				globalepoch,
				(node->isleaf()) ? 0 : node->to_internode()->size()
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
