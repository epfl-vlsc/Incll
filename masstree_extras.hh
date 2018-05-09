#pragma once
#include <atomic>
#include <semaphore.h>


//global configurations
//global flush
#define GLOBAL_FLUSH
#define GL_FREQ 16

#define GF_FILE "/dev/global_flush"
extern volatile uint64_t globalepoch;

struct CondHelper{
	sem_t flushSem;
	sem_t flushAckSem;
	sem_t ackSem;
	sem_t doneSem;
	pthread_mutex_t mtx;
	int nthreads;

	int flushFrequency;

	//global flush
	int fd;

	void threadDone(){
		sem_post(&doneSem);
	}

	void init(int nthreads_){
		sem_init(&flushSem, 0, 0);
		sem_init(&flushAckSem, 0, 0);
		sem_init(&ackSem, 0, 0);
		sem_init(&doneSem, 0, 0);
		pthread_mutex_init(&mtx, NULL);

		//global flush driver
		fd = open(GF_FILE, O_RDWR);
		if (fd < 0){
			perror("Failed to connect to device\n");
			return;
		}

		nthreads = nthreads_;
	}

	bool checkDone(){
		int doneMode;
		pthread_mutex_lock(&mtx);
		sem_getvalue(&doneSem, &doneMode);
		if(doneMode == nthreads){
			pthread_mutex_unlock(&mtx);
			return true;
		}
		pthread_mutex_unlock(&mtx);
		return false;
	}

	bool ackFlush() {
		int flushMode;

		if(this->checkDone()){
			return true;
		}

		pthread_mutex_lock(&mtx);
		//get flush mode
		sem_getvalue(&flushSem, &flushMode);

		//if in flush
		if(flushMode == 1){
			pthread_mutex_unlock(&mtx);

			//signal acknowledge
			sem_post(&ackSem);

			// wait for flush to complete
			sem_wait(&flushAckSem);
		}else{
			pthread_mutex_unlock(&mtx);
		}
		return false;
	}

	void flush(){
		if(this->checkDone()){
			return;
		}

		//entering flush mode
		sem_post(&flushSem);

		//wait for other thread acks
		int other_threads = nthreads - 1;
		for(int i=0;i<other_threads;++i){
			sem_wait(&ackSem);
		}

		//flush
		this->globalFlush();

		//exiting flush mode
		sem_wait(&flushSem);

		//release other threads
		for(int i=0;i<other_threads;++i){
			sem_post(&flushAckSem);
		}

	}

	void globalFlush(){
		int ret = write(fd, "", 0);
		if (ret < 0){
			perror("Failed to flush.");
			return;
		}
	}
};


struct KVTestHelper{
	std::string experimentName;

	//barrier
	pthread_barrier_t barr;

#ifdef GLOBAL_FLUSH
	//flush sync
	CondHelper fS;
#endif
	int freq;


	void init(int nthreads_, int freq_){
#ifdef GLOBAL_FLUSH
		//flush
		fS.init(nthreads_);
#endif
		freq = freq_;

		//init syncs
		pthread_barrier_init(&barr, NULL, nthreads_);
	}

	void setExpName(std::string& name){
		experimentName = name;
	}

	void destroy(){
		pthread_barrier_destroy(&barr);
	}

	void wait_barrier(int id){
		int res = pthread_barrier_wait(&barr);
		if(res != 0 && res!= PTHREAD_BARRIER_SERIAL_THREAD)
			printf("in thread %d in barrier 1 problem %d\n", id, res);
		printf("Thread %d passed barrier 1\n", id);
	}
};




/*
struct CondHelper{
	volatile std::atomic<int> flushState;
	volatile std::atomic<int> ackState;
	pthread_mutex_t mtx;
	pthread_cond_t flushCond;
	pthread_cond_t ackCond;
	int nthreads;
	int ndone;

	//global flush
	int fd;

	bool isDone(){
		return ndone == nthreads;
	}

	void incDone(){
		pthread_mutex_lock(&mtx);
		ndone++;
		pthread_mutex_unlock(&mtx);
	}

	void init(int nthreads_){
		flushState = 0;
		ackState = 0;
		mtx = PTHREAD_MUTEX_INITIALIZER;
		flushCond = PTHREAD_COND_INITIALIZER;
		ackCond = PTHREAD_COND_INITIALIZER;
		nthreads = nthreads_;
		ndone = 0;

		//global flush driver
		fd = open(GF_FILE, O_RDWR);
		if (fd < 0){
			perror("Failed to connect to device\n");
			return;
		}
	}

	void end(){
		pthread_mutex_lock(&mtx);
		flushState = 0;
		ackState = 0;
		ndone = 0;
		pthread_mutex_unlock(&mtx);
	}

	bool ackFinish() {
		volatile int oldAckState;
		int tempVar;

		pthread_mutex_lock(&mtx);
		if(this->isDone()){
			pthread_mutex_unlock(&mtx);
			return false;
		}

		if(flushState==1){
			tempVar = ackState;
			oldAckState = std::atomic_exchange(&ackState, tempVar+1);
			assert(oldAckState == tempVar );
			assert(tempVar + 1 == ackState);

			//ackState++;
			//oldAckState = std::atomic_exchange(&ackState, ackState+1);
			//printf("ackFinish():ackState %d e:%lx\n", ackState, globalepoch);
			pthread_cond_signal(&ackCond);

			while (flushState != 0){
				//printf("ackFinish():flushState %d e:%lx\n", flushState, globalepoch);
				pthread_cond_wait(&flushCond, &mtx);
			}
		}

		pthread_mutex_unlock(&mtx);
		return true;
	}

	void ackFlush() {
		volatile int oldAckState;
		volatile int tempVar;

		pthread_mutex_lock(&mtx);
		if(this->isDone()){
			pthread_mutex_unlock(&mtx);
		}

		if(flushState==1){
			tempVar = ackState;
			oldAckState = std::atomic_exchange(&ackState, tempVar+1);
			assert(oldAckState == tempVar);
			assert(tempVar + 1 == ackState);

			//ackState++;
			//printf("ackFlush():ackState %d e:%lx\n", ackState, globalepoch);
			pthread_cond_signal(&ackCond);

			while (flushState != 0){
				pthread_cond_wait(&flushCond, &mtx);
			}
		}

		pthread_mutex_unlock(&mtx);
	}

	void flush(){
		volatile int oldFlushState;
		volatile int tempVar2;

		volatile int oldAckState;
		volatile int tempVar;
		pthread_mutex_lock(&mtx);
		if(this->isDone()){
			pthread_mutex_unlock(&mtx);
			return;
		}

		//flushState = 1;
		//ackState = 1;

		oldFlushState = std::atomic_exchange(&flushState, 1);
		assert(oldFlushState == 0);

		oldAckState = std::atomic_exchange(&ackState, 1);
		assert(oldAckState == 0);

		while (ackState != nthreads){
			//printf("flush():ackState %d e:%lx\n", ackState, globalepoch);
			pthread_cond_wait(&ackCond, &mtx);
		}

		//printf("flush():ackState %d e:%lx\n", ackState, globalepoch);

		this->globalFlush();

		oldFlushState = std::atomic_exchange(&flushState, 0);
		assert(oldFlushState == 1);

		oldAckState = std::atomic_exchange(&ackState, 0);
		assert(oldAckState == nthreads);

		//ackState = 0;
		//flushState = 0;

		pthread_cond_broadcast(&flushCond);
		pthread_mutex_unlock(&mtx);
	}

	void destroy(){
		pthread_mutex_destroy(&mtx);
		pthread_cond_destroy(&flushCond);
		pthread_cond_destroy(&ackCond);

		close(fd);
	}

	void globalFlush(){
		int ret = write(fd, "", 0);
		if (ret < 0){
			perror("Failed to flush.");
			return;
		}
	}
};

*/

