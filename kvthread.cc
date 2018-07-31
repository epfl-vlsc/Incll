/* Masstree
 * Eddie Kohler, Yandong Mao, Robert Morris
 * Copyright (c) 2012-2016 President and Fellows of Harvard College
 * Copyright (c) 2012-2016 Massachusetts Institute of Technology
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
#include "kvthread.hh"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>

__thread int self_index = 0;
#define MAX_THREAD 16
#define NVM_PER_THREAD (100 * 1024 * 1024)
static const char *filename = "/scratch/tmp/nvm.heap";
void *mmappedData;
__thread uint8_t *local_nvm;
#define META_REGION_ADDR (1ull<<45)
static constexpr intptr_t const expectedAddress=META_REGION_ADDR;
static const size_t mappingLength = NVM_PER_THREAD * MAX_THREAD;

void nvm_create_mem(){
	bool exists = access( filename, F_OK ) != -1;
	int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    assert(fd != -1);

    if(!exists){
		int val = 0;
		lseek(fd, mappingLength, SEEK_SET);
		assert(write(fd, (void*)&val, sizeof(val))==sizeof(val));
		lseek(fd, 0, SEEK_SET);
    }

    //Execute mmap
    mmappedData = mmap((void*)expectedAddress, mappingLength, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(mmappedData!=MAP_FAILED);
    printf("%s data region. Mapped to address %p\n",
    	    	    		(exists) ? "Found":"Created", mmappedData);
    assert(mmappedData == (void *)META_REGION_ADDR);
}

void *alloc_nvm(int size)
{
    void *rc;
    rc = (void *) local_nvm;
    size = (size + CACHE_LINE_SIZE) & (~(CACHE_LINE_SIZE - 1));
    local_nvm = local_nvm + size;
    return rc;
}

void access_ppp(uint64_t *pp, void **p, bool w)
{
    uint64_t cur_pointer, cur_epoch, buf_epoch;
    int first_index = 0, c0, c1, c_new;

    cur_epoch = get_current_epoch(); // TODO: Need to get the real value

    c0 = POOL_C_GET(*pp);
    c1 = POOL_C_GET(*(pp+1));

    if(c0 == c1)
    {
        buf_epoch = POOL_C_GET(*pp) + POOL_C_GET(*(pp+1));
        first_index = c0 & 1;
        c_new = c0;

        if(epoch_is_valid(buf_epoch))
        {
            c_new = c_new + 1;
            first_index = !first_index;
        }

    }
    else
    {
        if((c0 + 1) == c1)
        {
            first_index = c1 & 1;
            c_new = c0 + 1;
        }
        else
        {
            first_index = c0 & 1;
            c_new = c1 + 1;
        }

    }

    cur_pointer = POOL_POINTER_GET(pp[!first_index]) << 2;

    if(w)
    {
        SET_P(pp[first_index], cur_epoch, c_new, POOL_POINTER_GET(pp[first_index]));
        SET_P(pp[!first_index], (cur_epoch >> 16), c_new, (uint64_t)*p);
    }
    else
    {
        *p = (void *)cur_pointer ;
    }
}

#if HAVE_SUPERPAGE && !NOSUPERPAGE
#include <sys/types.h>
#include <dirent.h>
#endif

threadinfo *threadinfo::allthreads;
#if ENABLE_ASSERTIONS
int threadinfo::no_pool_value;
#endif

inline threadinfo::threadinfo(int purpose, int index) {
    memset(this, 0, sizeof(*this));
    purpose_ = purpose;
    index_ = index;

    void *limbo_space = allocate(sizeof(limbo_group), memtag_limbo);
    mark(tc_limbo_slots, limbo_group::capacity);
    limbo_head_ = limbo_tail_ = new(limbo_space) limbo_group;
    ts_ = 2;
}


threadinfo *threadinfo::make(int purpose, int index) {
    static int threads_initialized;
    self_index = index + 1;
    if(index == 0)
        nvm_create_mem();
    local_nvm = (uint8_t *)META_REGION_ADDR + (index * NVM_PER_THREAD);
    threadinfo* ti = new(malloc(8192)) threadinfo(purpose, index);
    ti->next_ = allthreads;
    allthreads = ti;

    if (!threads_initialized) {
#if ENABLE_ASSERTIONS
        const char* s = getenv("_");
        no_pool_value = s && strstr(s, "valgrind") != 0;
#endif
        threads_initialized = 1;
    }

    return ti;
}

void threadinfo::refill_rcu() {
    if (!limbo_tail_->next_) {
        void *limbo_space = allocate(sizeof(limbo_group), memtag_limbo);
        mark(tc_limbo_slots, limbo_group::capacity);
        limbo_tail_->next_ = new(limbo_space) limbo_group;
    }
    limbo_tail_ = limbo_tail_->next_;
    assert(limbo_tail_->head_ == 0 && limbo_tail_->tail_ == 0);
}

inline unsigned limbo_group::clean_until(threadinfo& ti, mrcu_epoch_type epoch_bound,
                                         unsigned count) {
    epoch_type epoch = 0;
    while (head_ != tail_) {
        if (e_[head_].ptr_) {
            ti.free_rcu(e_[head_].ptr_, e_[head_].u_.tag);
            ti.mark(tc_gc);
            --count;
            if (!count) {
                e_[head_].ptr_ = nullptr;
                e_[head_].u_.epoch = epoch;
                break;
            }
        } else {
            epoch = e_[head_].u_.epoch;
            if (signed_epoch_type(epoch_bound - epoch) < 0)
                break;
        }
        ++head_;
    }
    if (head_ == tail_)
        head_ = tail_ = 0;
    return count;
}

void threadinfo::hard_rcu_quiesce() {
    limbo_group* empty_head = nullptr;
    limbo_group* empty_tail = nullptr;
    unsigned count = rcu_free_count;

    mrcu_epoch_type epoch_bound = active_epoch - 1;
    if (limbo_head_->head_ == limbo_head_->tail_
        || mrcu_signed_epoch_type(epoch_bound - limbo_head_->first_epoch()) < 0)
        goto done;

    // clean [limbo_head_, limbo_tail_]
    while (count) {
        count = limbo_head_->clean_until(*this, epoch_bound, count);
        if (limbo_head_->head_ != limbo_head_->tail_)
            break;
        if (!empty_head)
            empty_head = limbo_head_;
        empty_tail = limbo_head_;
        if (limbo_head_ == limbo_tail_) {
            limbo_head_ = limbo_tail_ = empty_head;
            goto done;
        }
        limbo_head_ = limbo_head_->next_;
    }
    // hook empties after limbo_tail_
    if (empty_head) {
        empty_tail->next_ = limbo_tail_->next_;
        limbo_tail_->next_ = empty_head;
    }

done:
    if (!count)
        perform_gc_epoch_ = epoch_bound; // do GC again immediately
    else
        perform_gc_epoch_ = epoch_bound + 1;
}

void threadinfo::report_rcu(void *ptr) const
{
    for (limbo_group *lg = limbo_head_; lg; lg = lg->next_) {
        int status = 0;
        limbo_group::epoch_type e = 0;
        for (unsigned i = 0; i < lg->capacity; ++i) {
            if (i == lg->head_)
                status = 1;
            if (i == lg->tail_) {
                status = 0;
                e = 0;
            }
            if (lg->e_[i].ptr_ == ptr)
                fprintf(stderr, "thread %d: rcu %p@%d: %s as %x @%" PRIu64 "\n",
                        index_, lg, i, status ? "waiting" : "freed",
                        lg->e_[i].u_.tag, e);
            else if (!lg->e_[i].ptr_)
                e = lg->e_[i].u_.epoch;
        }
    }
}

void threadinfo::report_rcu_all(void *ptr)
{
    for (threadinfo *ti = allthreads; ti; ti = ti->next())
        ti->report_rcu(ptr);
}


#if HAVE_SUPERPAGE && !NOSUPERPAGE
static size_t read_superpage_size() {
    if (DIR* d = opendir("/sys/kernel/mm/hugepages")) {
        size_t n = (size_t) -1;
        while (struct dirent* de = readdir(d))
            if (de->d_type == DT_DIR
                && strncmp(de->d_name, "hugepages-", 10) == 0
                && de->d_name[10] >= '0' && de->d_name[10] <= '9') {
                size_t x = strtol(&de->d_name[10], 0, 10) << 10;
                n = (x < n ? x : n);
            }
        closedir(d);
        return n;
    } else
        return 2 << 20;
}

static size_t superpage_size = 0;
#endif

static void initialize_pool(void* pool, size_t sz, size_t unit) {
    unit = unit + PPP_HEADER_SIZE;
    char* p = reinterpret_cast<char*>(pool), *p_tmp;
    void** nextptr = reinterpret_cast<void**>(p);
    for (size_t off = unit; off + unit <= sz; off += unit) {
        p_tmp = p + off;
        access_ppp((uint64_t *)nextptr, (void **)&p_tmp, true);
        nextptr = reinterpret_cast<void**>(p_tmp);
    }
    p = 0;
    access_ppp((uint64_t *)nextptr, (void **)&p, true);
}

void threadinfo::refill_pool(int nl) {
    //assert(!pool_[nl - 1]);

    if (!use_pool()) {
        pool_[nl - 1][0] = malloc((nl ) * CACHE_LINE_SIZE);
        if (pool_[nl - 1][0])
            *reinterpret_cast<void**>(pool_[nl - 1][0]) = 0;
        return;
    }

    void* pool = 0;
    size_t pool_size = 0;
    int r;

#if HAVE_SUPERPAGE && !NOSUPERPAGE
    if (!superpage_size)
        superpage_size = read_superpage_size();
    if (superpage_size != (size_t) -1) {
        pool_size = superpage_size;
# if MADV_HUGEPAGE
        if ((r = posix_memalign(&pool, pool_size, pool_size)) != 0) {
            access_ppp((uint64_t *)(pool_), &pool, true);
            fprintf(stderr, "posix_memalign superpage: %s\n", strerror(r));
            pool = 0;
            superpage_size = (size_t) -1;
        } else if (madvise(pool, pool_size, MADV_HUGEPAGE) != 0) {
            perror("madvise superpage");
            superpage_size = (size_t) -1;
        }
# elif MAP_HUGETLB
        pool = mmap(0, pool_size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        if (pool == MAP_FAILED) {
            perror("mmap superpage");
            pool = 0;
            superpage_size = (size_t) -1;
        }
# else
        superpage_size = (size_t) -1;
# endif
    }
#endif

    if (!pool) {
        pool_size = 2 << 20;
        pool = alloc_nvm(pool_size);
        /*
        if ((r = posix_memalign(&pool, CACHE_LINE_SIZE, pool_size)) != 0) {
            fprintf(stderr, "posix_memalign: %s\n", strerror(r));
            abort();
        }
         */
    }

    initialize_pool(pool, pool_size, nl * CACHE_LINE_SIZE);
    //pool_[nl - 1] = pool;
    access_ppp((uint64_t *)(&pool_[nl - 1][0]), &pool, true);
}
