#pragma once

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "incll_extflush.hh"

#define DATA_BUF_SIZE (1ull << 28)
#define DATA_REGION_ADDR (1ull<<45)
#define DATA_MAX_THREAD 18

#define nvm_free_addr ((void**)mmappedData)[0]

typedef uint64_t mrcu_epoch_type;
extern volatile mrcu_epoch_type globalepoch;
extern volatile mrcu_epoch_type failedepoch;
extern volatile mrcu_epoch_type currexec;

//nvm layout
//|thread 1          |thread 2          |
//|loc_size pool data|loc_size pool data|
class PDataAllocator{
private:
	static constexpr const char *pdata_filename = "/scratch/tmp/nvm.data";

	static constexpr const size_t pm_size = (4ull<<26);
	static constexpr const size_t cl_size = 64;
	static constexpr const size_t mapping_length = pm_size + cl_size;
	static constexpr const intptr_t data_addr = DATA_REGION_ADDR;

	static constexpr const size_t skip_to_ti = cl_size;
	static constexpr const size_t ti_size = 8192;
	static constexpr const size_t tis_size = ti_size * DATA_MAX_THREAD;
	static constexpr const size_t skip_to_data = (2 << 20);

	void *mmappedData;
	void *mmappedDataEnd;
	int fd;
	pthread_mutex_t nvm_lock;

	void init_exist(){
		if(!exists){
			nvm_free_addr = (char*)mmappedData + skip_to_data;
		}else{
			failedepoch = read_failed_epoch();
			currexec = globalepoch = failedepoch + 1;
		}
	}

	void access_pages(){
		const int page_size = 4096;
		char* tmp = (char*)mmappedData;
		void* acc_val = nullptr;
		for(size_t i=0;i<mapping_length;i+=page_size){
			if(i + page_size >= mapping_length) break;
			tmp += page_size;
			acc_val = ((void**)tmp)[0];
			(void)(acc_val);
		}
	}
public:
	bool exists;

	void init(){
		pthread_mutex_init(&nvm_lock, NULL);

		exists = access( pdata_filename, F_OK ) != -1;
		fd = open(pdata_filename, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
		assert(fd != -1);

		if(!exists){
			int val = 0;
			lseek(fd, mapping_length, SEEK_SET);
			assert(write(fd, (void*)&val, sizeof(val))==sizeof(val));
			lseek(fd, 0, SEEK_SET);
		}

		//Execute mmap
		mmappedData = mmap((void*)data_addr, mapping_length,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		assert(mmappedData!=MAP_FAILED);
		assert(mmappedData == (void *)DATA_REGION_ADDR);

		if(!exists){
			memset(mmappedData, 0, mapping_length);
		}else{
			access_pages();
		}

		mmappedDataEnd = (void*)((char*)mmappedData + mapping_length);
		init_exist();

		printf("%s data region. Mapped to:%p. Cur free addr:%p\n",
				(exists) ? "Found":"Created", mmappedData, nvm_free_addr);
	}

	void destroy(){
		unlink();
		assert(remove(pdata_filename)==0);
	}

	void* allocate_ti(int pindex){
		return (void*)((char*)mmappedData + skip_to_ti + pindex * ti_size);
	}

	void* malloc_nvm(size_t sz){
		void* tmp;
		pthread_mutex_lock(&nvm_lock);
		tmp = nvm_free_addr;
		nvm_free_addr = (void*)((char*)nvm_free_addr + sz);
		assert(tmp && nvm_free_addr < mmappedDataEnd);

		pthread_mutex_unlock(&nvm_lock);
		return tmp;
	}

	void* malloc_a(size_t sz){
		void *ptr = malloc(sz);
		return ptr;
	}

	void unlink(){
		pthread_mutex_destroy(&nvm_lock);
		munmap(mmappedData, mapping_length);
		close(fd);
	}

	void *get_cur_nvm_addr(){
		return nvm_free_addr;
	}

	void sync_cur_nvm_addr(){
		char *beg = (char*)nvm_free_addr;
		sync_range(beg, beg+sizeof(void*));
	}

	void block_malloc_nvm(){
		pthread_mutex_lock(&nvm_lock);
	}

	void write_failed_epoch(mrcu_epoch_type e){
		void *epoch_addr = (void*)((char*)mmappedData + pm_size);
		*(mrcu_epoch_type*)epoch_addr = e;
	}

	mrcu_epoch_type read_failed_epoch(){
		void *epoch_addr = (void*)((char*)mmappedData + pm_size);
		return *(mrcu_epoch_type*)epoch_addr;
	}
};

