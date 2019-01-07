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

#define CFENCE asm volatile ("":::"memory")
#ifndef __APPLE__
#define FLUSH(cl) clflushopt(cl)
#define DRAIN() sfence()
#define START()
#else
#define FLUSH(cl) clflush(cl)
#define DRAIN()
#define START()
#endif
static inline void clflush(volatile void *__p)
{
    asm volatile("clflush %0" : "+m" (*(volatile char *)__p));
}
static inline void clflushopt(volatile void *__p){
    asm volatile(".byte 0x66; clflush %0" : "+m" (*(volatile char *)__p));
}
static inline void sfence(){
    asm volatile ("sfence":::"memory");
}
static inline void mfence(){
    asm volatile ("mfence":::"memory");
}
static inline unsigned long long rdtsc(void)
{
    unsigned int hi, lo;
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}
extern int delaycount;
//static inline void delays(){}
//static __attribute__ ((noinline)) void delays(){
static void delays(){
    //if(delaycount<=1) return;
    unsigned long long t=rdtsc()+delaycount;
    while(1){
        asm volatile ("pause":::"memory");
        if(rdtsc()>t) break;
    }
}
static inline void sync(volatile void *__p){
    START();
    FLUSH(__p);
    delays();
    DRAIN();
}
static inline void sync2(void *p1, void *p2){
    START();
    FLUSH(p1);
    FLUSH(p2);
    delays();
    DRAIN();
}
//flush the range [start, end)
static inline void sync_range(volatile void *start, volatile void *end){
    START();
    FLUSH(start);
    volatile void *pp = (volatile void *)(((unsigned long long)(start)|63)+1); //beginning of the next cache line.
    for(; pp<end; pp=(volatile void *)((char*)pp+64))
        FLUSH(pp);
    delays();
    DRAIN();
}
#define SYNC(prm) sync((volatile void *)(prm))
#define SYNCR(s,e) sync_range((volatile void*)(s), (volatile void *)(e))

static inline void asm_movnti(volatile unsigned long *addr, unsigned long val)
{
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*addr): "r" (val));
}

static inline void write(volatile unsigned long *addr, unsigned long val){
    //asm_movnti((volatile unsigned long*)addr, val);
    *addr = val;
}
