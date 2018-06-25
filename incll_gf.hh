/* Global flush
 * Class to flush all cachelines
 *
 */

#pragma once

#include "incll_configs.hh"

#ifdef GLOBAL_FLUSH
#include "incll_globals.hh"
#include "incll_extlog.hh"

#include <atomic>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

#define GF_FILE "/dev/global_flush"
typedef uint64_t mrcu_epoch_type;
extern volatile mrcu_epoch_type globalepoch;
extern volatile mrcu_epoch_type active_epoch;

namespace GH{
	extern thread_local ExtNodeLogger node_logger;
}

class GlobalFlush{
private:
	sem_t flush_ack_sem;
	sem_t ack_sem;
	int nthreads;

	int flush_frequency;

	//global flush
	int fd;

	//ordering vaars
	std::atomic<bool> influshWarning; //general warning that something may change.
	std::atomic<bool> waitingFlush;	  //flush needed. Wait until it finishes.
	std::atomic<int> doneThreads;
	//std::atomic<long> epoch;
	//std::atomic<int> numAcks;

	void global_flush(){
		int ret = write(fd, "", 0);
		if (ret < 0){
			perror("Failed to flush.");
			return;
		}
	}

	void flush_(){
		//flush
		global_flush();

		//todo check later if this belongs here
		GH::node_logger.checkpoint();
	}

public:
	void reset(){
		globalepoch = 1;
	}

	void thread_done(){
		influshWarning.store(true, std::memory_order_seq_cst);//until the next epoch...
		doneThreads.fetch_add(1);
	}

	void init(int nthreads_){
		sem_init(&flush_ack_sem, 0, 0);
		sem_init(&ack_sem, 0, 0);
		waitingFlush=false;
		influshWarning=false;
		doneThreads=0;

		//global flush driver
		fd = open(GF_FILE, O_RDWR);
		if (fd < 0){
			perror("Failed to connect to device\n");
			return;
		}

		nthreads = nthreads_;
	}

	bool check_done(){
		return doneThreads == nthreads;
	}

	bool ack_flush() {
		if(influshWarning.load(std::memory_order_acquire)==false)
			return false;
		if(check_done())
			return true;

		//if in flush
		if(waitingFlush.load(std::memory_order_acquire)){
			//assert(epoch == globalepoch);
			//assert(numAcks.fetch_add(1)<=nthreads);

			//signal acknowledge
			sem_post(&ack_sem);

			// wait for flush to complete
			sem_wait(&flush_ack_sem);
		}
		return false;
	}

	void ack_flush_manual() {
		//signal acknowledge
		sem_post(&ack_sem);

		// wait for flush to complete
		sem_wait(&flush_ack_sem);
	}

	void flush_manual(){
		//wait for other thread acks
		int other_threads = nthreads - 1;
		for(int i=0;i<other_threads;++i){
			sem_wait(&ack_sem);
		}

		//global flush
		this->flush_();

		//inc ge
		globalepoch++;

		//release other threads
		for(int i=0;i<other_threads;++i){
			sem_post(&flush_ack_sem);
		}
	}

	void flush(long){
		influshWarning.store(true, std::memory_order_seq_cst);
		if(check_done())
			return;
		//assert(epoch.exchange(e) < e);

		//entering flush mode
		waitingFlush.store(true, std::memory_order_seq_cst);

		//wait for other thread acks
		int other_threads = nthreads - 1;
		for(int i=0;i<other_threads;++i){
			sem_wait(&ack_sem);
		}

		//global flush
		this->flush_();

		//exiting flush mode
		waitingFlush.store(false, std::memory_order_seq_cst);

		//release other threads
		for(int i=0;i<other_threads;++i){
			sem_post(&flush_ack_sem);
		}
		influshWarning.store(false, std::memory_order_release);
	}

};



#endif //gf
