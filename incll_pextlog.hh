/*
 * Log
 */

#pragma once

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <atomic>
#include <cassert>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "compiler.hh"
#include "incll_extflush.hh"
#include "incll_configs.hh"

#define PBUF_SIZE (1ull << 30)
#define LOG_REGION_ADDR (1ull << 30)
#define LOG_MAX_THREAD 16

typedef uint64_t mrcu_epoch_type;
extern volatile mrcu_epoch_type globalepoch;


class PExtNodeLogger{
private:
	struct logrec_node{
		size_t size_;
		void* node_addr_;
		uint64_t validity;
		char node_content_[0];

		bool check_validity(){
			return validity == entry_valid_magic;
		}
	};

	typedef uint64_t index;
	index curr;				//8 bytes
	index last_flush;		//8 bytes
	size_t buf_size;		//8 bytes
	void *root;				//8 bytes

	char buf_[0];
public:
	static constexpr const size_t entry_meta_size = sizeof(logrec_node);
	static constexpr const uint8_t entry_valid_magic = 18;
	static constexpr const int full_flush_range = 32;
	static constexpr const int curr_range = 24;
	static constexpr const int last_flush_range = 18;

	void init(){
		curr = 0;
		last_flush = 0;
		buf_size = PBUF_SIZE - full_flush_range;
		sync_range(&curr, (char*)curr + curr_range);
	}

	index get_last_flush(){
		return last_flush;
	}

	void checkpoint(){
		last_flush = curr;
		sync_range(&last_flush, (char*)last_flush+last_flush_range);
	}

	void print_stats(){
		printf("Log: curr %lu, last_flush %lu log size %lu\n",
				curr, last_flush, buf_size);
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

	void* get_tree_root(){
		return root;
	}

	void set_log_root(void* root_){
		DBGLOG("save root:%p for future", root_)
		root = root_;
	}

	template <typename N>
	void record(N* node){
		DBGLOG("Recording node %p le %lu %s ge:%lu keys:%d curr:%lu lf:%lu",
			(void*)node, node->loggedepoch, (node->isleaf()) ? "leaf":"internode",
			globalepoch, node->number_of_keys(), curr, last_flush
		)

		size_t node_size = node->allocated_size();
		logrec_node *lr =
				reinterpret_cast<logrec_node *>(buf_ + curr);
		size_t entry_size = get_entry_size(node_size);

		if(unlikely(curr + entry_size > buf_size)){
			printf("Warning in record: back to the beginning of log\n");
			assert(0);
			curr = 0;
			lr = reinterpret_cast<logrec_node *>(buf_);
			entry_size = lr->size_;
		}

		lr->size_ = entry_size;
		lr->node_addr_ = node;
		lr->validity = entry_valid_magic;
		std::memcpy(lr->node_content_, node, node_size);

		char *beg = (char*)lr->node_content_;
		sync_range(beg, beg + node_size);

		curr += entry_size;

		beg = (char*)&curr;
		sync_range(beg, beg + sizeof(curr));
	}


	template <typename N>
	void undo(N* rand_node){
		(void)(rand_node);

		while(last_flush != curr){
			char *entry = buf_ + last_flush;

			logrec_node *lr =
					reinterpret_cast<logrec_node *>(entry);
			size_t entry_size = lr->size_;

			if(!lr->check_validity() || last_flush + entry_size > buf_size){
				printf("Warning in undo next prev: back to the beginning of log\n");
				assert(0);
				last_flush = 0;
				entry = buf_;
				lr = reinterpret_cast<logrec_node *>(entry);
				entry_size = lr->size_;
			}

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

	}

	template <typename N>
	void undo_next_prev(N* rand_node, index temp_flush){
		(void)(rand_node);

		while(temp_flush != curr){
			char *entry = buf_ + temp_flush;

			logrec_node *lr =
					reinterpret_cast<logrec_node *>(entry);
			size_t entry_size = lr->size_;

			N* node = (N*)lr->node_addr_;
			DBGLOG("Undoing next prev node %p le %lu %s ge:%lu inkeys:%d curr:%lu lf:%lu",
				(void*)node, node->loggedepoch, (node->isleaf()) ? "leaf":"internode",
				globalepoch, node->number_of_keys(), curr, last_flush
			)

			if(!lr->check_validity() || temp_flush + entry_size > buf_size){
				printf("Warning in undo next prev: back to the beginning of log\n");
				assert(0);
				temp_flush = 0;
				entry = buf_;
				lr = reinterpret_cast<logrec_node *>(entry);
				entry_size = lr->size_;
			}

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


class PLogAllocator{
private:
	static constexpr const char *plog_filename = "/tmp/nvm.log";
	static constexpr const size_t logMappingLength = PBUF_SIZE * LOG_MAX_THREAD;
	static constexpr const intptr_t logExpectedAddress = LOG_REGION_ADDR;
	void *mmappedLog;
	int fd;
public:
	bool exists;

	PExtNodeLogger* init_plog(int tid){
		char* log_addr = (char*)LOG_REGION_ADDR + (tid * PBUF_SIZE);
		PExtNodeLogger *plog = reinterpret_cast<PExtNodeLogger*>(log_addr);

		plog->init();
		return plog;
	}

	void init(){
		exists = access( plog_filename, F_OK ) != -1;
		fd = open(plog_filename, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	    assert(fd != -1);

	    if(!exists){
			int val = 0;
			lseek(fd, logMappingLength, SEEK_SET);
			assert(write(fd, (void*)&val, sizeof(val))==sizeof(val));
			lseek(fd, 0, SEEK_SET);
		}

	    //Execute mmap
	    mmappedLog = mmap((void*)logExpectedAddress, logMappingLength, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	    assert(mmappedLog!=MAP_FAILED);
	    if(exists){
			printf("Found log region. Mapped to address %p\n", mmappedLog);
		}else{
			printf("Create log region. Mapped to address %p\n", mmappedLog);
		}
	    assert(mmappedLog == (void *)LOG_REGION_ADDR);
	}

	void destroy(){
		unlink();
		assert(remove(plog_filename)==0);
	}

	void unlink(){
		munmap(mmappedLog, logMappingLength);
		close(fd);
	}
};
