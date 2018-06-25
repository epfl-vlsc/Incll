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
		/*
		printf("Recording node %p le %lu leaf %d\n",
				(void*)node, node->loggedepoch, node->isleaf());
		*/

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

		curr += entry_size;
		active_records++;
	}

	template <typename N>
	void undo(N*& root){
		//printf("undo %lu records\n", active_records);
		while(last_flush != curr){
			char *entry = (char*)buf_ + last_flush;

			nvm_logrec_node *lr =
					reinterpret_cast<nvm_logrec_node *>(entry);
			size_t entry_size = lr->size_;

			//almost circular buffer
			if(!lr->check_validity() || curr + entry_size > buf_size){
				printf("Warning in undo: back to the beginning of log\n");
				assert(0);
				last_flush = 0;
				entry = (char*)buf_ + last_flush;
				lr = reinterpret_cast<nvm_logrec_node *>(entry);
				entry_size = lr->size_;
			}

			size_t copy_size = entry_size - sizeof(*lr);
			std::memcpy(lr->node_addr_, (void*)lr->node_content_, copy_size);

			if(lr->is_root){
				N* node = (N*)lr->node_addr_;
				if(!node->isleaf()){
					//printf("change root addr to %p\n", lr->node_addr_);
					root = node;
				}
			}

			last_flush += entry_size;
		}

		active_records = 0;
	}
};
