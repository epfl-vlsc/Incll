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

#pragma once

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>

#define USERSPACE_ONLY

/* programming pattern
setup_counters();
start_counters();
do stuff
stop_counters();
read_counters();
*/


//variables to read counters from
//when adding a new variable, change here, setup, read
extern thread_local int instructions_fd;
extern thread_local int cycles_fd;

extern thread_local int l1dc_references_fd;
extern thread_local int l1dc_misses_fd;
extern thread_local int llc_references_fd;
extern thread_local int llc_misses_fd;

static inline int sys_perf_event_open(struct perf_event_attr *attr, pid_t pid,
				      int cpu, int group_fd,
				      unsigned long flags){
	attr->size = sizeof(*attr);
	return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static void setup_counters(void){
	struct perf_event_attr attr;

	memset(&attr, 0, sizeof(attr));
#ifdef USERSPACE_ONLY
	attr.exclude_kernel = 1;
	attr.exclude_hv = 1;
	attr.exclude_idle = 1;
#endif

	//instructions
	attr.disabled = 1;
	attr.type = PERF_TYPE_HARDWARE;
	attr.config = PERF_COUNT_HW_INSTRUCTIONS;
	instructions_fd = sys_perf_event_open(&attr, 0, -1, -1, 0);
	if (instructions_fd < 0) {
		perror("sys_perf_event_open");
		exit(1);
	}

	//cycles
	attr.disabled = 0;
	attr.type = PERF_TYPE_HARDWARE;
	attr.config = PERF_COUNT_HW_CPU_CYCLES;
	cycles_fd = sys_perf_event_open(&attr, 0, -1, instructions_fd, 0);
	if (cycles_fd < 0) {
		perror("sys_perf_event_open");
		exit(1);
	}

	//l1 data cache load/store references
	attr.disabled = 0;
	attr.type = PERF_TYPE_HW_CACHE;
#ifndef PERF_STORES //loads
	attr.config = (PERF_COUNT_HW_CACHE_L1D)
		| (PERF_COUNT_HW_CACHE_OP_READ << 8)
		| (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
#else // stores
	attr.config = (PERF_COUNT_HW_CACHE_L1D)
		| (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
		| (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
#endif // load/stores
	l1dc_references_fd = sys_perf_event_open(&attr, 0, -1, instructions_fd, 0);
	if (l1dc_references_fd < 0) {
		perror("sys_perf_event_open");
		exit(1);
	}

	//l1dc store misses not supported
	//l1 data cache load misses
	attr.disabled = 0;
	attr.type = PERF_TYPE_HW_CACHE;
	attr.config = (PERF_COUNT_HW_CACHE_L1D)
		| (PERF_COUNT_HW_CACHE_OP_READ << 8)
		| (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
	l1dc_misses_fd = sys_perf_event_open(&attr, 0, -1, instructions_fd, 0);
	if (l1dc_misses_fd < 0) {
		perror("sys_perf_event_open");
		exit(1);
	}

	//ll cache load/store references
	attr.disabled = 0;
	attr.type = PERF_TYPE_HW_CACHE;
#ifndef PERF_STORES //loads
	attr.config = (PERF_COUNT_HW_CACHE_LL)
		| (PERF_COUNT_HW_CACHE_OP_READ << 8)
		| (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
#else // stores
	attr.config = (PERF_COUNT_HW_CACHE_LL)
		| (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
		| (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
#endif // load/stores
	llc_references_fd = sys_perf_event_open(&attr, 0, -1, instructions_fd, 0);
	if (llc_references_fd < 0) {
		perror("sys_perf_event_open");
		exit(1);
	}

	//ll cache load/store misses
	attr.disabled = 0;
	attr.type = PERF_TYPE_HW_CACHE;
#ifndef PERF_STORES //loads
	attr.config = (PERF_COUNT_HW_CACHE_LL)
		| (PERF_COUNT_HW_CACHE_OP_READ << 8)
		| (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
#else // stores
	attr.config = (PERF_COUNT_HW_CACHE_LL)
		| (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
		| (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
#endif // load/stores
	llc_misses_fd = sys_perf_event_open(&attr, 0, -1, instructions_fd, 0);
	if (llc_misses_fd < 0) {
		perror("sys_perf_event_open");
		exit(1);
	}
}

template <typename R>
static void read_counters(R& result){
	size_t res;
	unsigned long long instructions;
	unsigned long long cycles;

	unsigned long long l1dc_references;
	unsigned long long l1dc_misses;
	unsigned long long llc_references;
	unsigned long long llc_misses;

	res = read(instructions_fd, &instructions, sizeof(unsigned long long));
	assert(res == sizeof(unsigned long long));

	res = read(cycles_fd, &cycles, sizeof(unsigned long long));
	assert(res == sizeof(unsigned long long));

	res = read(l1dc_references_fd, &l1dc_references, sizeof(unsigned long long));
	assert(res == sizeof(unsigned long long));

	res = read(l1dc_misses_fd, &l1dc_misses, sizeof(unsigned long long));
	assert(res == sizeof(unsigned long long));

	res = read(llc_references_fd, &llc_references, sizeof(unsigned long long));
	assert(res == sizeof(unsigned long long));

	res = read(llc_misses_fd, &llc_misses, sizeof(unsigned long long));
	assert(res == sizeof(unsigned long long));

	/*
	printf("instructions: %lld\n", instructions);
	printf("cycles: %lld\n", cycles);
	printf("l1dc_loadreferences: %lld\n", l1dc_loadreferences);
	printf("l1dc_loadmisses: %lld\n", l1dc_loadmisses);
	printf("llc_loadreferences: %lld\n", llc_loadreferences);
	printf("llc_loadmisses: %lld\n", llc_loadmisses);
	*/

	result.set("instructions", instructions);
	result.set("cycles", cycles);
#ifndef PERF_STORES //loads
	result.set("l1dc_loadreferences", l1dc_references);
	result.set("l1dc_loadmisses", l1dc_misses);
	result.set("llc_loadreferences", llc_references);
	result.set("llc_loadmisses", llc_misses);
#else //stores
	result.set("l1dc_storesreferences", l1dc_references);
	result.set("l1dc_loadmisses", l1dc_misses);
	result.set("llc_storesreferences", llc_references);
	result.set("llc_storesmisses", llc_misses);
#endif //loads/stores
}

static void start_counters(void){
	/* Only need to start and stop the group leader */
	ioctl(instructions_fd, PERF_EVENT_IOC_ENABLE);
}

static void stop_counters(void){
	ioctl(instructions_fd, PERF_EVENT_IOC_DISABLE);
}

