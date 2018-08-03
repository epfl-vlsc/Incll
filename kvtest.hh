/* Masstree
 * Eddie Kohler, Yandong Mao, Robert Morris
 * Copyright (c) 2012-2013 President and Fellows of Harvard College
 * Copyright (c) 2012-2013 Massachusetts Institute of Technology
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
#ifndef KVTEST_HH
#define KVTEST_HH
#include "json.hh"
#include "misc.hh"
#include "kvproto.hh"
#include <vector>
#include <fstream>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <thread>

#include "incll_copy.hh"
#include "incll_globals.hh"
#include "incll_trav.hh"

#ifdef YCSB
#include "ycsb_helper.hh"
#endif //ycsb

extern PDataAllocator pallocator;

using lcdf::Str;
using lcdf::String;
using lcdf::Json;
extern int kvtest_first_seed;
// Templated KV tests, so we can run them either client/server or linked with
// the kvd binary.

template <typename N>
inline Json& kvtest_set_time(Json& result, const lcdf::String& base, N n, double delta_t)
{
    result.set(base, n);
    if (delta_t > 0)
        result.set(base + "_per_sec", n / delta_t);
    return result;
}

template <typename N>
inline Json kvtest_set_time(const Json& result, const lcdf::String& base, N n, double delta_t) {
    Json x(result);
    kvtest_set_time(x, base, n, delta_t);
    return x;
}

template <typename C>
void kvtest_sync_rw1_seed(C &client, int seed)
{
    client.rand.reset(seed);
    double tp0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n) {
        int32_t x = (int32_t) client.rand.next();
        client.put_sync(x, x + 1);
    }
    client.wait_all();
    double tp1 = client.now();

    client.puts_done();
    client.notice("now getting\n");
    int32_t *a = (int32_t *) malloc(sizeof(int32_t) * n);
    assert(a);
    client.rand.reset(seed);
    for (unsigned i = 0; i < n; ++i)
        a[i] = (int32_t) client.rand.next();
    for (unsigned i = 0; i < n; ++i)
        std::swap(a[i], a[client.rand.next() % n]);

    double tg0 = client.now();
    unsigned g;
    for (g = 0; g < n && !client.timeout(1); ++g)
        client.get_check_sync(a[g], a[g] + 1);
    client.wait_all();
    double tg1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, tp1 - tp0);
    kvtest_set_time(result, "gets", g, tg1 - tg0);
    kvtest_set_time(result, "ops", n + g, (tp1 - tp0) + (tg1 - tg0));
    client.report(result);
    free(a);
}

template <typename C>
void kvtest_sync_rw1(C &client)
{
    kvtest_sync_rw1_seed(client, kvtest_first_seed + client.id() % 48);
}

template <typename C>
unsigned kvtest_rw1puts_seed(C& client, int seed) {
    client.rand.reset(seed);
    double tp0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n) {
        int32_t x = (int32_t) client.rand.next();
        client.put(x, x + 1);
    }
    client.wait_all();
    double tp1 = client.now();
    client.puts_done();

    client.report(kvtest_set_time(Json(), "puts", n, tp1 - tp0));
    return n;
}

// do a bunch of inserts to distinct keys, then check that they all showed up.
// sometimes overwrites, but only w/ same value.
// different clients might use same key sometimes.
template <typename C>
void kvtest_rw1_seed(C &client, int seed)
{
    unsigned n = kvtest_rw1puts_seed(client, seed);

    client.notice("now getting\n");
    int32_t *a = (int32_t *) malloc(sizeof(int32_t) * n);
    assert(a);
    client.rand.reset(seed);
    for (unsigned i = 0; i < n; ++i)
        a[i] = (int32_t) client.rand.next();
    for (unsigned i = 0; i < n; ++i)
        std::swap(a[i], a[client.rand.next() % n]);

    double tg0 = client.now();
    unsigned g;
#if 0
#define BATCH 8
    for(g = 0; g+BATCH < n && !client.timeout(1); g += BATCH){
      long key[BATCH], expected[BATCH];
      for(int i = 0; i < BATCH; i++){
        key[i] = a[g+i];
        expected[i] = a[g+i] + 1;
      }
      client.many_get_check(BATCH, key, expected);
    }
#else
    for (g = 0; g < n && !client.timeout(1); ++g)
        client.get_check(a[g], a[g] + 1);
#endif
    client.wait_all();
    double tg1 = client.now();

    Json result = client.report(Json());
    kvtest_set_time(result, "gets", g, tg1 - tg0);
    double delta_puts = n / result["puts_per_sec"].as_d();
    kvtest_set_time(result, "ops", n + g, delta_puts + (tg1 - tg0));
    client.report(result);
    free(a);
}

template <typename C>
void kvtest_rw1puts(C &client)
{
    kvtest_rw1puts_seed(client, kvtest_first_seed + client.id() % 48);
}

template <typename C>
void kvtest_rw1(C &client)
{
    kvtest_rw1_seed(client, kvtest_first_seed + client.id() % 48);
}

// do a bunch of inserts to distinct keys, then check that they all showed up.
// sometimes overwrites, but only w/ same value.
// different clients might use same key sometimes.
template <typename C>
void kvtest_rw1long_seed(C &client, int seed)
{
    const char * const formats[] = {
        "user%u", "machine%u", "opening%u", "fartparade%u"
    };
    char buf[64];

    client.rand.reset(seed);
    double tp0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n) {
        unsigned fmt = client.rand.next();
        int32_t x = (int32_t) client.rand.next();
        client.put(Str::snprintf(buf, sizeof(buf), formats[fmt % 4], x), x + 1);
    }
    client.wait_all();
    double tp1 = client.now();

    client.puts_done();
    client.notice("now getting\n");
    int32_t *a = (int32_t *) malloc(sizeof(int32_t) * n * 2);
    assert(a);
    client.rand.reset(seed);
    for (unsigned i = 0; i < n * 2; ++i)
        a[i] = (int32_t) client.rand.next();
    for (unsigned i = 0; i < n; ++i) {
        unsigned x = client.rand.next() % n;
        std::swap(a[2 * i], a[2 * x]);
        std::swap(a[2 * i + 1], a[2 * x + 1]);
    }

    double tg0 = client.now();
    unsigned g;
    for (g = 0; g < n && !client.timeout(1); ++g) {
        unsigned fmt = a[2 * g];
        int32_t x = (int32_t) a[2 * g + 1];
        client.get_check(Str::snprintf(buf, sizeof(buf), formats[fmt % 4], x), x + 1);
    }
    client.wait_all();
    double tg1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, tp1 - tp0);
    kvtest_set_time(result, "gets", g, tg1 - tg0);
    kvtest_set_time(result, "ops", n + g, (tp1 - tp0) + (tg1 - tg0));
    client.report(result);
    free(a);
}

template <typename C>
void kvtest_rw1long(C &client)
{
    kvtest_rw1long_seed(client, kvtest_first_seed + client.id() % 48);
}

// interleave inserts and gets for random keys.
template <typename C>
void kvtest_rw2_seed(C &client, int seed, double getfrac)
{
    client.rand.reset(seed);
    const unsigned c = 2654435761U;
    const unsigned offset = client.rand.next();

    double t0 = client.now();
    uint64_t puts = 0, gets = 0;
    int getfrac65536 = (int) (getfrac * 65536 + 0.5);
    while (!client.timeout(0) && (puts + gets) <= client.limit()) {
        if (puts == 0 || (client.rand.next() % 65536) >= getfrac65536) {
            // insert
            unsigned x = (offset + puts) * c;
            client.put(x, x + 1);
            ++puts;
        } else {
            // get
            unsigned x = (offset + (client.rand.next() % puts)) * c;
            client.get_check(x, x + 1);
            ++gets;
        }
    }
    client.wait_all();
    double t1 = client.now();

    Json result = Json().set("puts", puts).set("gets", gets);
    kvtest_set_time(result, "ops", puts + gets, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_rw2(C &client)
{
    kvtest_rw2_seed(client, kvtest_first_seed + client.id() % 48, 0.5);
}

template <typename C>
void kvtest_rw2g90(C &client)
{
    kvtest_rw2_seed(client, kvtest_first_seed + client.id() % 48, 0.9);
}

template <typename C>
void kvtest_rw2g98(C &client)
{
    kvtest_rw2_seed(client, kvtest_first_seed + client.id() % 48, 0.98);
}

// interleave inserts and gets for random keys.
template <typename C>
void kvtest_rw2fixed_seed(C &client, int seed, double getfrac)
{
    client.rand.reset(seed);
    const unsigned c = 2654435761U;
    const unsigned offset = client.rand.next();

    double t0 = client.now();
    uint64_t puts = 0, gets = 0;
    int getfrac65536 = (int) (getfrac * 65536 + 0.5);
    while (!client.timeout(0) && (puts + gets) <= client.limit()) {
        if (puts == 0 || (client.rand.next() % 65536) >= getfrac65536) {
            // insert
            unsigned x = (offset + puts) * c;
            x %= 100000000;
            client.put(x, x + 1);
            ++puts;
        } else {
            // get
            unsigned x = (offset + (client.rand.next() % puts)) * c;
            x %= 100000000;
            client.get_check(x, x + 1);
            ++gets;
        }
    }
    client.wait_all();
    double t1 = client.now();

    Json result = Json().set("puts", puts).set("gets", gets);
    kvtest_set_time(result, "ops", puts + gets, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_rw2fixed(C &client)
{
    kvtest_rw2fixed_seed(client, kvtest_first_seed + client.id() % 48, 0.5);
}

template <typename C>
void kvtest_rw2fixedg90(C &client)
{
    kvtest_rw2fixed_seed(client, kvtest_first_seed + client.id() % 48, 0.9);
}

template <typename C>
void kvtest_rw2fixedg98(C &client)
{
    kvtest_rw2fixed_seed(client, kvtest_first_seed + client.id() % 48, 0.98);
}

// do a bunch of inserts to sequentially increasing keys,
// then check that they all showed up.
// different clients might use same key sometimes.
template <typename C>
void kvtest_rw3(C &client)
{
    double t0 = client.now();
    uint64_t n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n)
        client.put_key8(n, n + 1);
    client.wait_all();

    client.puts_done();
    client.notice("now getting\n");

    double t1 = client.now();
    for (unsigned i = 0; i < n; ++i)
        client.get_check_key8(i, i + 1);
    client.wait_all();

    double t2 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, t1 - t0);
    kvtest_set_time(result, "gets", n, t2 - t1);
    kvtest_set_time(result, "ops", n + n, t2 - t0);
    client.report(result);
}

// do a bunch of inserts to sequentially decreasing keys,
// then check that they all showed up.
// different clients might use same key sometimes.
template <typename C>
void kvtest_rw4(C &client)
{
    const int top = 2147483647;

    double t0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n)
        client.put_key8(top - n, n + 1);
    client.wait_all();

    client.puts_done();
    client.notice("now getting\n");

    double t1 = client.now();
    for (unsigned i = 0; i < n; ++i)
        client.get_check_key8(top - i, i + 1);
    client.wait_all();

    double t2 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, t1 - t0);
    kvtest_set_time(result, "gets", n, t2 - t1);
    kvtest_set_time(result, "ops", n + n, t2 - t0);
    client.report(result);
}

// do a bunch of inserts to sequentially decreasing 8B keys,
// then check that they all showed up.
// different clients might use same key sometimes.
template <typename C>
void kvtest_rw4fixed(C &client)
{
    const int top = 99999999;

    double t0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n)
        client.put_key8(top - n, n + 1);
    client.wait_all();

    client.puts_done();
    client.notice("now getting\n");

    double t1 = client.now();
    for (unsigned i = 0; i < n; ++i)
        client.get_check_key8(top - i, i + 1);
    client.wait_all();

    double t2 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, t1 - t0);
    kvtest_set_time(result, "gets", n, t2 - t1);
    kvtest_set_time(result, "ops", n + n, t2 - t0);
    client.report(result);
}

// update the same small set of keys over and over,
// to uncover concurrent update bugs in the server.
template <typename C>
void kvtest_same_seed(C &client, int seed)
{
    client.rand.reset(seed);

    double t0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n) {
        unsigned x = client.rand.next() % 10;
        client.put(x, x + 1);
    }
    client.wait_all();
    double t1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_same(C &client)
{
    kvtest_same_seed(client, kvtest_first_seed + client.id() % 48);
}

// update the same small set of keys over and over, with interspersed gets.
template <typename C>
void kvtest_rwsmall_seed(C &client, int nkeys, int seed)
{
    client.rand.reset(seed);

    double t0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n) {
        unsigned x = client.rand.next() % (8 * nkeys);
        if (x & 7)
            client.get(x >> 3);
        else
            client.put(x >> 3, n);
    }
    client.wait_all();
    double t1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_rwsmall24(C &client)
{
    kvtest_rwsmall_seed(client, 24, kvtest_first_seed + client.id() % 48);
}

// update the same small set of keys over and over, with interspersed gets.
// but ensure that the keys are all on different cache lines.
template <typename C>
void kvtest_rwsep_seed(C &client, int nkeys, int clientid, int seed)
{
    for (int n = clientid * (32 + nkeys); n < (clientid + 1) * (32 + nkeys); ++n)
        client.put(1000000 + n, n);

    client.rand.reset(seed);

    double t0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n) {
        unsigned x = client.rand.next() % (8 * nkeys);
        if (x & 7)
            client.get(1000000 + clientid * (32 + nkeys) + (x >> 3));
        else
            client.put(1000000 + clientid * (32 + nkeys) + (x >> 3), n);
    }
    client.wait_all();
    double t1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_rwsep24(C &client)
{
    kvtest_rwsep_seed(client, 24, client.id(), kvtest_first_seed + client.id() % 48);
}

// Same as rw1, except that the keys are no more than 8 bytes
template <typename C>
void kvtest_rw1fixed_seed(C &client, int seed)
{
    client.rand.reset(seed);
    double tp0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0) && n <= client.limit(); ++n) {
        int32_t x = (int32_t) client.rand.next();
        x %= 100000000;
        client.put(x, x + 1);
    }
    client.wait_all();
    double tp1 = client.now();

    client.puts_done();
    client.notice("now getting\n");
    int32_t *a = (int32_t *) malloc(sizeof(int32_t) * n);
    assert(a);
    client.rand.reset(seed);
    for (unsigned i = 0; i < n; ++i) {
        a[i] = (int32_t) client.rand.next();
        a[i] %= 100000000;
    }
    for (unsigned i = 0; i < n; ++i)
        std::swap(a[i], a[client.rand.next() % n]);

    double tg0 = client.now();
    unsigned g;
#if 0
#define BATCH 8
    for(g = 0; g+BATCH < n && !client.timeout(1); g += BATCH){
      long key[BATCH], expected[BATCH];
      for(int i = 0; i < BATCH; i++){
        key[i] = a[g+i];
        expected[i] = a[g+i] + 1;
      }
      client.many_get_check(BATCH, key, expected);
    }
#else
    for (g = 0; g < n && !client.timeout(1); ++g)
        client.get_check(a[g], a[g] + 1);
#endif
    client.wait_all();
    double tg1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, tp1 - tp0);
    kvtest_set_time(result, "gets", g, tg1 - tg0);
    kvtest_set_time(result, "ops", n + g, (tp1 - tp0) + (tg1 - tg0));
    client.report(result);
    free(a);
}

template <typename C>
void kvtest_rw1fixed(C &client)
{
    kvtest_rw1fixed_seed(client, kvtest_first_seed + client.id() % 48);
}

// Same as rw1, except that keys are 16-bytes (prefixed with "0"s)
template <typename C>
void kvtest_rw16_seed(C &client, int seed)
{
    client.rand.reset(seed);
    double tp0 = client.now();
    int n;
    char key[256];
    char val[256];
    for (n = 0; !client.timeout(0); ++n) {
        int32_t x = (int32_t) client.rand.next();
        sprintf(key, "%016d", x);
        sprintf(val, "%016d", x + 1);
        client.put(key, val);
    }
    client.wait_all();
    double tp1 = client.now();

    client.puts_done();
    client.notice("now getting\n");
    int32_t *a = (int32_t *) malloc(sizeof(int32_t) * n);
    assert(a);
    client.rand.reset(seed);
    for (int i = 0; i < n; ++i)
        a[i] = (int32_t) client.rand.next();
    for (int i = 0; i < n; ++i)
        std::swap(a[i], a[client.rand.next() % n]);

    double tg0 = client.now();
    int g;
    for (g = 0; g < n && !client.timeout(1); ++g) {
        sprintf(key, "%016d", a[g]);
        sprintf(val, "%016d", a[g] + 1);
        client.get_check(key, val);
    }
    client.wait_all();
    double tg1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, tp1 - tp0);
    kvtest_set_time(result, "gets", g, tg1 - tg0);
    kvtest_set_time(result, "ops", n + g, (tp1 - tp0) + (tg1 - tg0));
    client.report(result);
    free(a);
}

template <typename C>
void kvtest_rw16(C &client)
{
    kvtest_rw16_seed(client, kvtest_first_seed + client.id() % 48);
}

//global size for sanity checking tree size
std::atomic<size_t> global_size;

// generate a big tree, update the tree to current epoch as much as possible
//write/delete: write to tree, meanwhile try to get keys, if found remove
template <typename C>
void kvtest_recovery(C &client){
	GH::init_thread_all(client.id());
	ycsbc::OpRatios opratios(GH::get_rate, GH::put_rate, GH::rem_rate, GH::scan_rate);

	unsigned pos = 0, val =0;
	uint64_t n = 0;
	size_t local_size = 0;
	void *root_ptr = nullptr;

	//begin initops epoch 1--------------------------------------------------
	if(client.id() == 0){
		DBGLOG("-----ninitops ge: %lu", globalepoch)
		while (n < GH::n_initops) {
			++n;
			pos = rand() % GH::n_keys;

			local_size +=
					client.put(pos, pos + 1);
		}
	}
	//end initops epoch 1-----------------------------------------------------

	GH::advance_epoch(client.id(), client.get_root());

	//begin nops1 epoch 2----------------------------------------------------
	if(client.id() == 0){
		DBGLOG("-----nops1 ge: %lu", globalepoch)
	}

	n = 0;
	while(n<GH::n_ops1){
		n++;
		pos = client.rand.next() % GH::n_keys;
		val = client.rand.next();
		int op = opratios.get_next_op();
		switch(op){
		case 0:
			client.get_sync(pos);
			break;
		case 1:
			local_size +=
					client.put(pos, val);
			break;
		case 2:
			local_size -=
					client.remove_sync(pos);
			break;
		default:
			assert(0);
			break;
		}
	}
	//nops 1 end epoch 2----------------------------------------------------

	global_size += local_size;
	GH::advance_epoch(client.id(), client.get_root());

	//begin copy epoch 3---------------------------------------------
	if(client.id() == 0){
		DBGLOG("-----copy ge: %lu", globalepoch)
	}

	void *copy = nullptr;
	if(client.id() == 0){
		assert(global_size == get_tree_size(client.get_root()));
		copy = copy_tree(client.get_root());
		assert(is_same_tree(client.get_root(), copy));
		root_ptr = client.get_root();
		DBGLOG("root addr %p ge: %lu", root_ptr, globalepoch);
	}
	GH::thread_barrier.wait_barrier(client.id());
	//end copy epoch 3----------------------------------------------------

	//begin nops2 epoch 3----------------------------------------------------
	if(client.id() == 0){
		DBGLOG("-----nops2 ge: %lu", globalepoch)
	}

	n = 0;
	while(n<GH::n_ops2){
		n++;
		pos = client.rand.next() % GH::n_keys;
		val = client.rand.next();
		unsigned op = client.rand.next()%4;
		switch(op){
		case 0:
		case 1:
			client.get_sync(pos);
			break;
		case 2:
			local_size -= client.remove_sync(pos);
			break;
		case 3:
			local_size += client.put(pos, val);
			break;
		}
	}
	//end nops2 epoch 3----------------------------------------------------

	//begin recovery epoch 3-----------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());
	if(client.id() == 0){
		DBGLOG("-----recovery ge: %lu", globalepoch)
		failedepoch = 3;
	}

	//Begin Undo epoch 3 ------------------------------------------------
	if(client.id() == 0){
		void* undo_root = GH::node_logger->get_tree_root();
		client.set_root(undo_root);
	}
	GH::thread_barrier.wait_barrier(client.id());

	auto last_flush = GH::node_logger->get_last_flush();
	GH::node_logger->undo(client.get_root());
	GH::thread_barrier.wait_barrier(client.id());

	GH::node_logger->undo_next_prev(client.get_root(), last_flush);
	GH::thread_barrier.wait_barrier(client.id());
	//End Undo epoch 3 ------------------------------------------------


	if(client.id() == 0){
		//ensure same root
		DBGLOG("root addr %p ge: %lu", (void*)client.get_root(), globalepoch);

		assert(root_ptr == client.get_root());

		//ensure same tree
		bool is_same = is_same_tree(client.get_root(), copy, true);
		printf("%s\n", is_same ? "is same":"not same - recovery failed");
	}

	GH::thread_barrier.wait_barrier(client.id());
	//end recovery epoch 3-----------------------------------------------
}

// generate a big tree, update the tree to current epoch as much as possible
//write/delete: write to tree, meanwhile try to get keys, if found remove
template <typename C>
void kvtest_rand(C &client, uint64_t n_keys){
	GH::init_thread_all(client.id());

	unsigned pos = 0, val =0;
	uint64_t n = 0;
	Json result = Json();
	size_t local_size = 0;


	if(client.id() == 0){
		while (n < n_keys/2) {
			++n;
			pos = rand() % n_keys;

			local_size +=
					client.put(pos, pos + 1);
		}
		printf("Created tree\n");
	}

	//Barrier-------------------------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());

	n = 0;

	double t0 = client.now();
	while(!client.timeout(0)){
		n++;
		pos = client.rand.next() % n_keys;
		val = client.rand.next();
		unsigned op = client.rand.next()%4;
		switch(op){
		case 0:
		case 1:
			client.get_sync(pos);
			break;
		case 2:
			local_size -=
					client.remove_sync(pos);
			break;
		case 3:
			local_size +=
					client.put(pos, val);
		}
		if ((n % (1 << 6)) == 0){
			client.rcu_quiesce();
			//set_global_epoch
		}
#ifdef GLOBAL_FLUSH
			GH::global_flush.ack_flush();
#endif
	}
	double t1 = client.now();
	//result.set("time", t1-t0);
	result.set("ops", (long)(n/(t1-t0)));

#ifdef GLOBAL_FLUSH
	GH::global_flush.thread_done();
	while(!GH::global_flush.ack_flush());
#endif

	client.report(result);

	global_size += local_size;
	//Barrier-------------------------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());

	//check size
	if(client.id() == 0){
		assert(global_size == get_tree_size(client.get_root()));
#ifdef COLLECT_STATS
		print_tree_summary(client.get_root());
#endif //collect stats
	}
}

#ifdef YCSB
template <typename C, typename RandGen>
void kvtest_ycsb(C &client,
		ycsbc::OpRatios op_ratios,
		RandGen key_rand){
	uint64_t pos = 0, val = 0;
	size_t init = GH::n_initops;
	size_t nops1 = GH::n_ops1;
	size_t nkeys = GH::n_keys;

	int get_ops = 0;
	int put_ops = 0;
	int rem_ops = 0;
	int scan_ops = 0;

	GH::init_thread_all(client.id());
	UniGen val_rand;
	ycsbc::exp_init_all(client.id(), key_rand, val_rand, op_ratios.op_rand);

	quick_istr key;
	std::vector<Str> keys(10), values(10);

	uint64_t n = 0;
	Json result = Json();
	size_t local_size = 0;

	if(client.id() == 0){
		while (n < init) {
			pos = n;
			val = pos + 1;

			local_size += client.put(pos, val);
			++n;
		}
#ifdef COLLECT_STATS
		print_tree_summary(client.get_root(), true);
#endif //collect stats
		printf("Created tree--------------------\n");
	}

	//Barrier-------------------------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());
	client.rcu_quiesce();
#ifdef GLOBAL_FLUSH
		GH::global_flush.ack_flush();
#endif

	n = 0;

	double t0 = client.now();
	while(n < nops1){
		n++;
		pos = key_rand.next() % nkeys;

		unsigned op = op_ratios.get_next_op();
		switch(op){
		case ycsbc::get_op:{
			client.get_sync(pos);
			get_ops++;
		}break;
		case ycsbc::put_op:{
			val = val_rand.next();
			local_size += client.put(pos, val);
			put_ops++;
		}break;
		case ycsbc::rem_op:{
			local_size -= client.remove_sync(pos);
			rem_ops++;
		}break;
		case ycsbc::scan_op:{
			key.set(pos, 8);
			client.scan_sync(key.string(), 10, keys, values);
			scan_ops++;
			}break;
		default:
			assert(0);
			break;
		}
		if ((n % (1 << 6)) == 0){
			client.rcu_quiesce();
			//set_global_epoch
		}
#ifdef GLOBAL_FLUSH
			GH::global_flush.ack_flush();
#endif
	}
	double t1 = client.now();
	result.set("time", t1-t0);
	result.set("ops", (long)(n/(t1-t0)));
	result.set("get_ops", (long)(get_ops/(t1-t0)));
	result.set("put_ops", (long)(put_ops/(t1-t0)));
	result.set("rem_ops", (long)(rem_ops/(t1-t0)));
	result.set("scan_ops", (long)(scan_ops/(t1-t0)));

#ifdef GLOBAL_FLUSH
	GH::global_flush.thread_done();
	while(!GH::global_flush.ack_flush());
#endif

	client.report(result);

	global_size += local_size;
	//Barrier-------------------------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());

	//check size
	if(client.id() == 0){
		assert(global_size == get_tree_size(client.get_root()));
#ifdef COLLECT_STATS
		print_tree_summary(client.get_root());
#endif //collect stats
	}
}
#endif //ycsb





#ifdef YCSB_RECOVERY
//recovery only uses 8 threads
#define N_SAMPLES 5000
volatile uint64_t sampled_ops[8*sizeof(uint64_t)];
volatile uint64_t ops_samples[N_SAMPLES];
volatile bool sampling_done = false;
std::chrono::milliseconds interval(8); //sleep for 8ms
pthread_t sampler_thread;

void* recovery_ops_sampler(void* arg){
	static int count = 0;
	(void)(arg);

	while(!sampling_done && count < N_SAMPLES){
		ops_samples[count] = 0;
		for(int i=0;i<8;++i){
			ops_samples[count] += sampled_ops[i*sizeof(uint64_t)];
		}
		++count;
		std::this_thread::sleep_for(interval);
	}
	printf("sampling done\n");
	return nullptr;
}



template <typename C, typename RandGen>
void ycsb_init_execution(C &client,
		ycsbc::OpRatios &op_ratios,
		RandGen &key_rand){
	//initialization
	uint64_t pos = 0, val = 0;
	size_t init = GH::n_initops;
	size_t nops1 = GH::n_ops1;
	size_t nkeys = GH::n_keys;

	UniGen val_rand;
	ycsbc::exp_init_all(client.id(), key_rand, val_rand, op_ratios.op_rand);

	uint64_t n = 0;

	//Create tree----------------------------------------------------
	if(client.id() == 0){
		while (n < init) {
			pos = n;
			val = pos + 1;

			client.put(pos, val);
			++n;
		}
		global_masstree_root = client.get_root();
		printf("Created tree--------------------\n");
	}

	//Barrier-------------------------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());
	client.rcu_quiesce();
#ifdef GLOBAL_FLUSH
	GH::global_flush.ack_flush();
#endif
	GH::node_logger->checkpoint();
	GH::thread_barrier.wait_barrier(client.id());

	//Do ops----------------------------------------------------
	n = 0;
	while(n < nops1){
		if(n != nops1/2 && client.id() == 0){
			pallocator.block_malloc_nvm();
			GH::global_flush.block_flush();

			printf("Power failure - System crash - Reboot please!\n");
			pallocator.write_failed_epoch(globalepoch);
			printf("failed epoch:%lu\n", pallocator.read_failed_epoch());
			printf("cur nvm:%p\n", pallocator.get_cur_nvm_addr());
			size_t tree_size = get_tree_size(client.get_root());
			size_t tree_nodes = get_num_nodes(client.get_root());
			printf("keys:%lu nodes:%lu\n", tree_size, tree_nodes);
			printf("root:%p\n", client.get_root());
			client.get_root()->print_node();

			exit(0);
		}
		n++;
		pos = key_rand.next() % nkeys;

		unsigned op = op_ratios.get_next_op();
		switch(op){
		case ycsbc::get_op:{
			DBGLOG("get op")
			client.get_sync(pos);
		}break;
		case ycsbc::put_op:{
			DBGLOG("put op %lu", val)
			val = val_rand.next();
			client.put(pos, val);
		}break;
		case ycsbc::rem_op:{
			DBGLOG("rem op")
			client.remove_sync(pos);
		}break;
		default:
			assert(0);
			break;
		}
		if ((n % (1 << 6)) == 0){
			client.rcu_quiesce();
			//set_global_epoch
		}
#ifdef GLOBAL_FLUSH
		GH::global_flush.ack_flush();
#endif
	}


}

template <typename C, typename RandGen>
void ycsb_re_execution(C &client,
		ycsbc::OpRatios &op_ratios,
		RandGen &key_rand){
	//initialization
	uint64_t pos = 0, val = 0;
	size_t nops1 = GH::n_ops1;
	size_t nkeys = GH::n_keys;

	uint64_t n = 0;
	Json result = Json();

	UniGen val_rand;
	ycsbc::exp_init_all(client.id(), key_rand, val_rand, op_ratios.op_rand);

	if(client.id() == 0){
		void* undo_root = GH::node_logger->get_tree_root();
		client.set_root(undo_root);
		DBGLOG("setting root to %p", undo_root);
		size_t tree_size = get_tree_size(client.get_root());
		size_t tree_nodes = get_num_nodes(client.get_root());
		printf("keys:%lu nodes:%lu\n", tree_size, tree_nodes);
		client.get_root()->print_node();
	}
	GH::thread_barrier.wait_barrier(client.id());
	double t0 = client.usec_now();
	auto last_flush = GH::node_logger->get_last_flush();
	GH::node_logger->undo(client.get_root());
	GH::thread_barrier.wait_barrier(client.id());

	GH::node_logger->undo_next_prev(client.get_root(), last_flush);
	GH::thread_barrier.wait_barrier(client.id());

	double t1 = client.usec_now();
	result.set("recovery_time", t1-t0);
	if(client.id() == 0){
		DBGLOG("finished undo")
	}

	//barrier------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());
	client.rcu_quiesce();
#ifdef GLOBAL_FLUSH
	GH::global_flush.ack_flush();
#endif

	if(client.id() == 0){
		pthread_create(&sampler_thread, nullptr, recovery_ops_sampler, nullptr);
	}
	GH::thread_barrier.wait_barrier(client.id());

	n = 0;
	while(n < nops1){
		n++;
		pos = key_rand.next() % nkeys;

		unsigned op = op_ratios.get_next_op();
		switch(op){
		case ycsbc::get_op:{
			client.get_sync(pos);
		}break;
		case ycsbc::put_op:{
			val = val_rand.next();
			client.put(pos, val);
		}break;
		case ycsbc::rem_op:{
			client.remove_sync(pos);
		}break;
		default:
			assert(0);
			break;
		}
		if ((n % (1 << 6)) == 0){
			client.rcu_quiesce();
			sampled_ops[client.id()*sizeof(uint64_t)] = n;
			//set_global_epoch
		}
#ifdef GLOBAL_FLUSH
		GH::global_flush.ack_flush();
#endif
	}

#ifdef GLOBAL_FLUSH
	GH::global_flush.thread_done();
	while(!GH::global_flush.ack_flush());
#endif

	if(client.id() == 0){
		sampling_done = true;
		pthread_join(sampler_thread, NULL);
		int count = 0;
		while(ops_samples[count] && count < N_SAMPLES){
			printf("%lu %d\n", ops_samples[count], count);
			count++;
		}
	}

	client.report(result);
	//Barrier-------------------------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());
}

template <typename C, typename RandGen>
void kvtest_ycsb_recovery(C &client,
		ycsbc::OpRatios op_ratios,
		RandGen key_rand){

	GH::init_thread_all(client.id());


	bool is_rec = GH::is_recovery();
	if(!is_rec){
		ycsb_init_execution(client, op_ratios, key_rand);
	}else{
		ycsb_re_execution(client, op_ratios, key_rand);
	}
}

#endif //ycsb recovery




// generate a big tree, update the tree to current epoch as much as possible
//write/delete: write to tree, meanwhile try to get keys, if found remove
template <typename C>
void kvtest_intensive(C &client, unsigned long n_keys, unsigned long n_ops){
	unsigned pos = 0, val =0;
	uint64_t n = 0;
	Json result = Json();

	result.set("n_keys", n_keys);
	result.set("n_ops", n_ops);
	result.set("test", GH::exp_name);

	if(client.id() == 0){
		printf("Create tree\n");
		while (n < n_keys/2) {
			++n;
			pos = rand() % n_keys;

			client.put(pos, pos + 1);
		}
	}

	//Barrier-------------------------------------------------------------
	GH::thread_barrier.wait_barrier(client.id());

	n = 0;
	bool found = false;

	double t0 = client.now();
	if (client.id() % 2) {
		while (!client.timeout(0) && n <= n_ops) {
			++n;

			pos = rand() % n_keys;

#ifdef GLOBAL_FLUSH
			GH::global_flush.ack_flush();
#endif
			//work
			found = client.get_sync(pos);

			if(found){
#ifdef GLOBAL_FLUSH
				GH::global_flush.ack_flush();
#endif
				//work
				client.remove_sync(pos);
			}

			if ((n % (1 << 6)) == 0){
				client.rcu_quiesce();
			}

		}
		result.set("removepos", pos);
	} else {
		while (!client.timeout(0) && n <= n_ops) {
			++n;

			pos = rand() % n_keys;
			val = rand() % n_keys;

#ifdef GLOBAL_FLUSH
			GH::global_flush.ack_flush();
#endif
			client.put(pos, val);

			if ((n % (1 << 6)) == 0){
				client.rcu_quiesce();
			}
		}
		result.set("putpos", pos);
	}
	client.wait_all();
	double t1 = client.now();

#ifdef GLOBAL_FLUSH
	GH::global_flush.thread_done();
	bool end = false;
	while(!end){
		end = GH::global_flush.ack_flush();
	}
#endif

	kvtest_set_time(result, "ops", n, t1 - t0);
	client.report(result);
}


// A writer and a deleter; the deleter chases the writer
template <typename C>
void kvtest_wd1(unsigned initial_pos, int incr, C &client)
{
    incr = std::max(incr, client.nthreads() / 2);
    unsigned pos = initial_pos + ((client.id() / 2) % incr);
    unsigned n = 0;
    Json result = Json();

    double t0 = client.now();
    if (client.id() % 2) {
        while (!client.get_sync(pos + 16 * incr))
            /* spin */;
        while (!client.timeout(0) && n <= client.limit()) {
            ++n;
            if (client.remove_sync(pos))
                pos += incr;
            if ((n % (1 << 6)) == 0)
                client.rcu_quiesce();
        }
        result.set("removepos", pos);
    } else {
        while (!client.timeout(0) && n <= client.limit()) {
            ++n;
            client.put(pos, pos + 1);
            pos += incr;
            if ((n % (1 << 6)) == 0)
                client.rcu_quiesce();
        }
        result.set("putpos", pos);
    }
    client.wait_all();
    double t1 = client.now();

    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_wd1_check(unsigned initial_pos, int incr, C &client)
{
    incr = std::max(incr, client.nthreads() / 2);
    unsigned pos = initial_pos + ((client.id() / 2) % incr);
    unsigned n = 0;
    Json result = Json();

    double t0 = client.now();
    if (client.id() % 2 == 0) {
        unsigned max_remove = -1, min_post_remove = -1, max_post_remove = -1;
        unsigned bugs = 0;
        bool found_putpos = false;
        constexpr int nbatch = 20;
        Str gotten[nbatch];
        char gottenbuf[nbatch * 16];
        for (int i = 0; i < nbatch; ++i)
            gotten[i].s = &gottenbuf[i * 16];

        while (!client.timeout(0)
               && (!found_putpos || pos < max_post_remove + 100000)) {
            for (int i = 0; i < nbatch; ++i) {
                gotten[i].len = 16;
                client.get(pos + i * incr, &gotten[i]);
            }
            client.wait_all();
            for (int i = 0; i < nbatch; ++i) {
                if (gotten[i].len) {
                    if (min_post_remove == unsigned(-1))
                        min_post_remove = max_post_remove = pos;
                    else if (!found_putpos)
                        max_post_remove = pos;
                    else if (++bugs == 1)
                        fprintf(stderr, "%u: present unexpectedly\n", pos);
                } else {
                    if (min_post_remove == unsigned(-1))
                        max_remove = pos;
                    else
                        found_putpos = true;
                }
                pos += incr;
            }
        }

        result.set("removepos", max_remove + incr);
        result.set("putpos", max_post_remove + incr);
        if (bugs)
            result.set("buggykeys", bugs);
    }
    client.wait_all();
    double t1 = client.now();

    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_wd2(C &client)
{
    char sbuf[32], kbuf[32], next_kbuf[32];
    const int sep = 26;
    const int p_remove = 1000, p_put2 = 10000, p_remove2 = 20000;
    int x = 0;
    quick_istr xstr(0);

    client.put(Str("n"), client.nthreads());
    always_assert(client.nthreads() > 1);

    // set up status keys
    snprintf(sbuf, sizeof(sbuf), "s%03d", client.id());
    for (int i = 0; i < sep; ++i) {
        sbuf[4] = 'A' + i;
        client.put(Str(sbuf, 5), Str());
    }
    client.put(Str(sbuf, 4), xstr.string());

    // set up main keys
    snprintf(kbuf, sizeof(kbuf), "k%03d", client.id());
    for (int i = 0; i < sep; ++i) {
        kbuf[4] = 'A' + i;
        client.put(Str(kbuf, 5), Str());
    }
    client.put(Str(kbuf, 4), Str());

    snprintf(next_kbuf, sizeof(next_kbuf), "k%03d", (client.id() + 1) % client.nthreads());

    // main loop
    double t0 = client.now();
    int put_status = 0;
    long nrounds = 0;
    while (!client.timeout(0)) {
        ++nrounds;
        client.put(Str(kbuf, 4), xstr.string(), &put_status);
        if ((client.rand.next() % 65536) < p_remove)
            client.remove(Str(next_kbuf, 4));

        int rand = client.rand.next() % 65536;
        if (rand < p_put2) {
            for (int i = sep - 1; i >= 0; --i) {
                next_kbuf[4] = 'A' + i;
                client.put(Str(next_kbuf, 5), Str());
            }
        } else if (rand < p_remove2) {
            for (int i = sep - 1; i >= 0; --i) {
                next_kbuf[4] = 'A' + i;
                client.remove(Str(next_kbuf, 5));
            }
        } else {
            /* do nothing */
        }

        client.wait_all();

        if (put_status == Inserted) {
            ++x;
            xstr.set(x);
            client.put(Str(sbuf, 4), xstr.string());
        }
    }
    double t1 = client.now();

    Json result;
    kvtest_set_time(result, "rounds", nrounds, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_wd2_check(C &client)
{
    if (client.id() != 0)
        return;

    int n;
    client.get(Str("n"), &n);
    client.wait_all();
    always_assert(n > 1);
    Json result;

    char buf[32];
    for (int i = 0; i < n; ++i) {
        int s, k;
        snprintf(buf, sizeof(buf), "k%03d", i);
        client.get(Str(buf, 4), &k);
        snprintf(buf, sizeof(buf), "s%03d", i);
        client.get(Str(buf, 4), &s);
        client.wait_all();
        if (!(s >= 0 && (s == k || s == k + 1 || k == -1)))
            fprintf(stderr, "problem: s%03d=%d vs. k%03d=%d\n",
                    i, s, i, k);
        result.set("thread" + String(i), Json().push_back(s).push_back(k));
    }

    client.report(result);
}

// Create a range of keys [initial_pos, initial_pos + n)
// where key k == initial_pos + i has value (n - 1 - i).
// Many overwrites.
template <typename C>
void kvtest_tri1(unsigned initial_pos, int incr, C &client)
{
    incr = std::max(incr, client.nthreads());
    unsigned n = 0;
    Json result = Json();

    double t0 = client.now();
    for (unsigned x = 0; x < client.limit(); ++x)
        for (unsigned y = 0, z = x; y <= x; ++y, --z, ++n)
            client.put(initial_pos + y * incr, z);
    client.wait_all();
    double t1 = client.now();

    kvtest_set_time(result, "puts", n, t1 - t0);
    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_tri1_check(unsigned initial_pos, int incr, C &client)
{
    incr = std::max(incr, client.nthreads());
    unsigned n = 0;
    Json result = Json();

    double t0 = client.now();
    for (unsigned x = 0; x < client.limit(); ++x, ++n)
        client.get_check(initial_pos + x * incr, client.limit() - 1 - x);
    client.wait_all();
    double t1 = client.now();

    kvtest_set_time(result, "gets", n, t1 - t0);
    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}


#define PALMN   128000000
enum { PalmBatch = 8192 / 24 };
#define PALM_DEBUG 1    // use get_check in palmb, which force palm::get
                        // to touch the cachline of the value
template <typename C>
void kvtest_palma(C &client)
{
    Json result = Json();
    double t0 = client.now();
    for (int i = 0; i < PALMN; i++) {
        uint64_t v = i + 1;
        client.put(i, v);
    }
    client.wait_all();
    double t1 = client.now();
    kvtest_set_time(result, "ops", PALMN, t1 - t0);
    client.report(result);
}

inline int compare_int(const void *a, const void *b)
{
    return compare(*(uint64_t *)a, *(uint64_t *)b);
}

template <typename C>
void kvtest_palmb_seed(C &client, int seed)
{
    Json result = Json();
    client.rand.reset(seed);
    double t0 = client.now();
    int n;
    int nquery = 0;
    uint64_t a[PalmBatch];
    for (n = 0; !client.timeout(0); ++n) {
        uint64_t x = (uint64_t) client.rand.next();
        x %= (PALMN / 10);
        a[nquery++] = x;
        if (nquery == PalmBatch) {
            qsort(a, PalmBatch, sizeof(a[0]), compare_int);
            for (int j = 0; j < PalmBatch && !client.timeout(0); j++) {
#if PALM_DEBUG
                uint64_t v = a[j] + 1;
                client.get_check(a[j], v);
#else
                client.get(a[j]);
#endif
            }
            nquery = 0;
        }
    }
    client.wait_all();
    double t1 = client.now();
    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_palmb(C &client)
{
    kvtest_palmb_seed(client, kvtest_first_seed + client.id() % 48);
}

template <typename C>
void kvtest_ycsbk_seed(C &client, int seed)
{
    client.rand.reset(seed);
    double tp0 = client.now();
    int n;
    char key[512], val[512];
    for (n = 0; !client.timeout(0) && n < 1000000; ++n) {
        strcpy(key, "user");
        int p = 4;
        for (int i = 0; i < 18; i++, p++)
            key[p] = '0' + (client.rand.next() % 10);
        key[p] = 0;
        int32_t v = (int32_t) client.rand.next();
        sprintf(val, "%d", v);
        client.put(Str(key, strlen(key)), Str(val, strlen(val)));
    }
    client.wait_all();
    double tp1 = client.now();

    client.puts_done();
    client.notice("now getting\n");
    client.rand.reset(seed);
    double tg0 = client.now();
    int g;
    for (g = 0; g < n && !client.timeout(1); ++g) {
        strcpy(key, "user");
        int p = 4;
        for (int i = 0; i < 18; i++, p++)
            key[p] = '0' + (client.rand.next() % 10);
        key[p] = 0;
        int32_t v = (int32_t) client.rand.next();
        sprintf(val, "%d", v);
        client.get_check(Str(key, strlen(key)), Str(val, strlen(val)));
    }
    client.wait_all();
    double tg1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, tp1 - tp0);
    kvtest_set_time(result, "gets", g, tg1 - tg0);
    kvtest_set_time(result, "ops", n + g, (tp1 - tp0) + (tg1 - tg0));
    client.report(result);
}

template <typename C>
void kvtest_ycsbk(C &client)
{
    kvtest_ycsbk_seed(client, kvtest_first_seed + client.id() % 48);
}

template <typename C>
void
kvtest_bdb(C &client)
{
    enum { nrec = 500000, keylen = 8, datalen = 32 };
    char key[keylen + 1];
    char val[datalen + 1];
    memset(val, '^', sizeof(val));
    val[datalen] = 0;
    key[keylen] = 0;
    srandom(0);
    for (int n = 0; n < nrec; n++) {
        for (int i = 0; i < keylen; i++)
            key[i] = 'a' + random() % 26;
        client.put(key, val);
    }
    client.wait_all();

    srandom(0);
    double t0 = now();
    unsigned long n;
    for (n = 0; n < 10000000; n++) {
        for (int i = 0; i < keylen; i++)
            key[i] = 'a' + random() % 26;
        client.get_check(key, val);
        if (n % nrec == 0)
            srandom(0);
    }
    double t1 = now();
    Json result = Json();
    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}

enum { NLongParts = 16 };

template <typename C>
void
kvtest_long_init(C &client)
{
    assert(client.id() < NLongParts);
    int seed = kvtest_first_seed + client.id();
    client.rand.reset(seed);
    const int keylen = client.keylen();
    const int prefixLen = client.prefixLen();
    const char minkltr = client.minkeyletter();
    const char maxkltr = client.maxkeyletter();
    assert(prefixLen < keylen);
    const uint32_t nkeysPerPart = client.nkeys() / NLongParts;
    char key[512], val[512];
    val[8] = 0;
    memset(key, '^', prefixLen);
    double t0 = now();
    unsigned long n;
    for(n = 0; n < nkeysPerPart; ++n){
        for (int i = prefixLen; i < keylen; i++)
            key[i] = minkltr + client.rand.next() % (maxkltr - minkltr + 1);
        key[keylen] = 0;
        memcpy(val, key + keylen - 8, 8);
        client.put(key, val);
        client.rand.next();
    }
    client.wait_all();
    double t1 = now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, t1 - t0);
    client.report(result);
}

template <typename C>
void
kvtest_long_go(C &client)
{
    const int keylen = client.keylen();
    const int prefixLen = client.prefixLen();
    assert(prefixLen < keylen);
    const uint32_t nKeysPerPart = client.nkeys() / NLongParts;
    const char minkltr = client.minkeyletter();
    const char maxkltr = client.maxkeyletter();
    char key[512], val[512];
    memset(key, '^', prefixLen);
    val[8] = 0;
    double t0 = now();
    long n = 0;
    int cur_cid = client.id() % NLongParts;
    while (!client.timeout(0)) {
        client.rand.reset(kvtest_first_seed + cur_cid);
        uint32_t op;
        for(op = 0; !client.timeout(0) && op < nKeysPerPart; op++){
            for (int i = prefixLen; i < keylen; i++)
                key[i] = minkltr + client.rand.next() % (maxkltr - minkltr + 1);
            memcpy(val, key + keylen - 8, 8);
            key[keylen] = 0;
            if (client.rand.next() % 100 < client.getratio())
                client.get_check(key, val);
            else
                client.put(key, val);
        }
        cur_cid = (cur_cid + 1) % NLongParts;
        n += op;
    }
    client.wait_all();
    double t1 = now();

    Json result = Json();
    kvtest_set_time(result, "ops", n, t1 - t0);
    client.report(result);
}

template <typename C>
void
kvtest_wscale(C &client)
{
    double t0 = now();
    client.rand.reset(kvtest_first_seed + client.id() % 48);
    long n;
    for(n = 0; !client.timeout(0); n++){
        long x = client.rand.next();
        client.put(x, x + 1);
    }
    client.wait_all();
    double t1 = now();
    Json result = Json();
    kvtest_set_time(result, "puts", n, t1 -t0);
    client.report(result);
}

template <typename C>
void
kvtest_ruscale_init(C &client)
{
    double t0 = now();
    client.rand.reset(kvtest_first_seed + client.id() % 48);
    const int ruscale_partsz = client.ruscale_partsz();
    const int firstkey = ruscale_partsz * client.ruscale_init_part_no();
    // Insert in random order
    int *keys = (int *) malloc(sizeof(int) * ruscale_partsz);
    always_assert(keys);
    for(int i = 0; i < ruscale_partsz; i++)
        keys[i] = i + firstkey;
    for(int i = 0; i < ruscale_partsz; i++)
        std::swap(keys[i], keys[client.rand.next() % ruscale_partsz]);
    for(int i = 0; i < ruscale_partsz; i++){
        long x = keys[i];
        client.put(x, x + 1);
    }
    client.wait_all();
    double t1 = now();
    Json result = Json();
    kvtest_set_time(result, "puts", ruscale_partsz, t1 - t0);
    client.report(result);
    free(keys);
}

template <typename C>
void
kvtest_rscale(C &client)
{
    client.rand.reset(kvtest_first_seed + client.id() % 48);
    const long nseqkeys = client.nseqkeys();
    double t0 = now();
    long n;
    for(n = 0; !client.timeout(0); n++){
        long x = client.rand.next() % nseqkeys;
        client.get_check(x, x + 1);
    }
    client.wait_all();
    double t1 = now();
    Json result = Json();
    kvtest_set_time(result, "gets", n, t1 - t0);
    client.report(result);
}

template <typename C>
void
kvtest_uscale(C &client)
{
    client.rand.reset(kvtest_first_seed + client.id());
    const long nseqkeys = client.nseqkeys();
    double t0 = now();
    long n;
    for(n = 0; !client.timeout(0); n++){
        long x = client.rand.next() % nseqkeys;
        client.put(x, x + 1);
    }
    client.wait_all();
    double t1 = now();
    Json result = Json();
    kvtest_set_time(result, "puts", n, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_udp1_seed(C &client, int seed)
{
    client.rand.reset(seed);
    double tp0 = client.now();
    unsigned n;
    for (n = 0; !client.timeout(0); ++n)
        client.put(0, 1);
    client.wait_all();
    double tp1 = client.now();

    client.puts_done();
    client.notice("now getting\n");
    int32_t *a = (int32_t *) malloc(sizeof(int32_t) * n);
    assert(a);
    client.rand.reset(seed);
    for (unsigned i = 0; i < n; ++i)
        a[i] = (int32_t) client.rand.next();
    for (unsigned i = 0; i < n; ++i)
        std::swap(a[i], a[client.rand.next() % n]);

    double tg0 = client.now();
    unsigned g;
    for (g = 0; !client.timeout(1); ++g)
        client.get_check(0, 1);
    client.wait_all();
    double tg1 = client.now();

    Json result = Json();
    kvtest_set_time(result, "puts", n, tp1 - tp0);
    kvtest_set_time(result, "gets", g, tg1 - tg0);
    kvtest_set_time(result, "ops", n + g, (tp1 - tp0) + (tg1 - tg0));
    client.report(result);
    free(a);
}

template <typename C>
void kvtest_udp1(C &client)
{
    kvtest_udp1_seed(client, kvtest_first_seed + client.id() % 48);
}

// do four million of inserts to distinct keys.
// sometimes overwrites, but only w/ same value.
// different clients might use same key sometimes.
template <typename C>
void kvtest_w1_seed(C &client, int seed)
{
    int n;
    if (client.limit() == ~(uint64_t) 0)
        n = 4000000;
    else
        n = std::min(client.limit(), (uint64_t) INT_MAX);
    client.rand.reset(seed);

    double t0 = now();
    for (int i = 0; i < n; i++) {
        long x = client.rand.next();
        client.put_key10(x, x + 1);
    }
    client.wait_all();
    double t1 = now();

    Json result = Json().set("total", (long) (n / (t1 - t0)))
        .set("puts", n)
        .set("puts_per_sec", n / (t1 - t0));
    client.report(result);
}

// do four million gets.
// in a random order.
// if we get in the same order that w1 put, performance is
// about 15% better for b-tree.
template <typename C>
void kvtest_r1_seed(C &client, int seed)
{
    int n;
    if (client.limit() == ~(uint64_t) 0)
        n = 4000000;
    else
        n = std::min(client.limit(), (uint64_t) INT_MAX);
    long *a = (long *) malloc(sizeof(long) * n);
    always_assert(a);

    client.rand.reset(seed);
    for (int i = 0; i < n; i++)
        a[i] = client.rand.next();
    for (int i = 0; i < n; i++) {
        int i1 = client.rand.next() % n;
        long tmp = a[i];
        a[i] = a[i1];
        a[i1] = tmp;
    }

    double t0 = now();
    for (int i = 0; i < n; i++)
        client.get_check_key10(a[i], a[i] + 1);
    client.wait_all();
    double t1 = now();

    Json result = Json().set("total", (long) (n / (t1 - t0)))
        .set("gets", n)
        .set("gets_per_sec", n / (t1 - t0));
    client.report(result);
}

// do four million of inserts to distinct keys.
// sometimes overwrites, but only w/ same value.
// different clients might use same key sometimes.
template <typename C>
void kvtest_wcol1at(C &client, int col, int seed, long maxkeys)
{
    int n;
    if (client.limit() == ~(uint64_t) 0)
        n = 4000000;
    else
        n = std::min(client.limit(), (uint64_t) INT_MAX);
    client.rand.reset(seed);

    double t0 = now();
    for (int i = 0; i < n; i++) {
        long x = client.rand.next() % maxkeys;
        client.put_col_key10(x, col, x + 1);
    }
    client.wait_all();
    double t1 = now();

    Json result = Json().set("total", (long) (n / (t1 - t0)))
        .set("puts", n)
        .set("puts_per_sec", n / (t1 - t0));
    client.report(result);
}

// do four million gets.
// in a random order.
// if we get in the same order that w1 put, performance is
// about 15% better for b-tree.
template <typename C>
void kvtest_rcol1at(C &client, int col, int seed, long maxkeys)
{
    int n;
    if (client.limit() == ~(uint64_t) 0)
        n = 4000000;
    else
        n = std::min(client.limit(), (uint64_t) INT_MAX);
    long *a = (long *) malloc(sizeof(long) * n);
    always_assert(a);

    client.rand.reset(seed);
    for (int i = 0; i < n; i++)
        a[i] = client.rand.next() % maxkeys;
    for (int i = 0; i < n && 0; i++) {
        int i1 = client.rand.next() % n;
        long tmp = a[i];
        a[i] = a[i1];
        a[i1] = tmp;
    }

    double t0 = now();
    for (int i = 0; i < n; i++)
        client.get_col_check_key10(a[i], col, a[i] + 1);
    client.wait_all();
    double t1 = now();

    Json result = Json().set("total", (long) (n / (t1 - t0)))
        .set("gets", n)
        .set("gets_per_sec", n / (t1 - t0));
    client.report(result);
}

// test scans with parallel inserts
template <typename C>
void kvtest_scan1(C &client, double writer_quiet)
{
    int n, wq65536 = int(writer_quiet * 65536);
    if (client.limit() == ~(uint64_t) 0)
        n = 10000;
    else
        n = std::min(client.limit(), (uint64_t) 97655);
    Json result;

    if (client.id() % 24 == 0) {
        for (int i = 0; i < n; ++i)
            client.put_key8(i * 1024, i);
        client.wait_all();

        int pos = 0, mypos = 0, scansteps = 0;
        quick_istr key;
        std::vector<Str> keys, values;
        Json errj;
        while (!client.timeout(0) && errj.size() < 1000) {
            key.set(pos, 8);
            client.scan_sync(key.string(), 100, keys, values);
            if (keys.size() == 0) {
                if (mypos < n * 1024)
                    errj.push_back("missing " + String(mypos) + " through " + String((n - 1) * 1024));
                pos = mypos = 0;
            } else {
                for (size_t i = 0; i < keys.size(); ++i) {
                    int val = keys[i].to_i();
                    if (val < 0) {
                        errj.push_back("unexpected key " + String(keys[i].s, keys[i].len));
                        continue;
                    }
                    if (val < pos)
                        errj.push_back("got " + String(keys[i].s, keys[i].len) + ", expected " + String(pos) + " or later");
                    pos = val + 1;
                    while (val > mypos) {
                        errj.push_back("got " + String(keys[i].s, keys[i].len) + ", missing " + String(mypos) + " @" + String(scansteps) + "+" + String(i));
                        mypos += 1024;
                    }
                    if (val == mypos) {
                        mypos = val + 1024;
                        ++scansteps;
                    }
                }
            }
            client.rcu_quiesce();
        }
        if (errj.size() >= 1000)
            errj.push_back("too many errors, giving up");
        result.set("ok", errj.empty()).set("scansteps", scansteps);
        if (errj)
            result.set("errors", errj);

    } else {
        int delta = 1 + (client.id() % 30) * 32, rounds = 0;
        while (!client.timeout(0)) {
            int first = (client.rand.next() % n) * 1024 + delta;
            int rand = client.rand.next() % 65536;
            if (rand < wq65536) {
                for (int d = 0; d < 31; ++d)
                    relax_fence();
            } else if (rounds > 100 && (rand % 2) == 1) {
                for (int d = 0; d < 31; ++d)
                    client.remove_key8(d + first);
            } else {
                for (int d = 0; d < 31; ++d)
                    client.put_key8(d + first, d + first);
            }
            ++rounds;
            client.rcu_quiesce();
        }
    }

    client.report(result);
}

// test reverse scans with parallel inserts
template <typename C>
void kvtest_rscan1(C &client, double writer_quiet)
{
    int n, wq65536 = int(writer_quiet * 65536);
    if (client.limit() == ~(uint64_t) 0)
        n = 10000;
    else
        n = std::min(client.limit(), (uint64_t) 97655);
    Json result;

    if (client.id() % 24 == 0) {
        for (int i = 1; i <= n; ++i)
            client.put_key8(i * 1024, i);
        client.wait_all();

        int pos = (n + 1) * 1024, mypos = n * 1024, scansteps = 0;
        quick_istr key;
        std::vector<Str> keys, values;
        Json errj;
        while (!client.timeout(0) && errj.size() < 1000) {
            key.set(pos, 8);
            client.rscan_sync(key.string(), 100, keys, values);
            if (keys.size() == 0) {
                if (mypos > 0)
                    errj.push_back("missing 1024 through " + String(mypos) + " @" + String(scansteps));
                pos = (n + 1) * 1024, mypos = n * 1024;
            } else {
                for (size_t i = 0; i < keys.size(); ++i) {
                    int val = keys[i].to_i();
                    if (val < 0) {
                        errj.push_back("unexpected key " + String(keys[i].s, keys[i].len));
                        continue;
                    }
                    if (val > pos)
                        errj.push_back("got " + String(keys[i].s, keys[i].len) + ", expected " + String(pos) + " or less");
                    pos = val - 1;
                    while (val < mypos) {
                        String last;
                        if (i)
                            last = String(keys[i-1].s, keys[i-1].len);
                        else
                            last = String(key.string().s, key.string().len);
                        errj.push_back("got " + String(keys[i].s, keys[i].len) + ", missing " + String(mypos) + " @" + String(scansteps) + "+" + String(i) + ", last " + last);
                        mypos -= 1024;
                    }
                    if (val == mypos) {
                        mypos = val - 1024;
                        ++scansteps;
                    }
                }
            }
            client.rcu_quiesce();
        }
        if (errj.size() >= 1000)
            errj.push_back("too many errors, giving up");
        result.set("ok", errj.empty()).set("scansteps", scansteps);
        if (errj)
            result.set("errors", errj);

    } else {
        int delta = 1 + (client.id() % 30) * 32, rounds = 0;
        while (!client.timeout(0)) {
            int first = (client.rand.next() % n + 1) * 1024 + delta;
            int rand = client.rand.next() % 65536;
            if (rand < wq65536) {
                for (int d = 0; d < 31; ++d)
                    relax_fence();
            } else if (rounds > 100 && (rand % 2) == 1) {
                for (int d = 0; d < 31; ++d)
                    client.remove_key8(d + first);
            } else {
                for (int d = 0; d < 31; ++d)
                    client.put_key8(d + first, d + first);
            }
            ++rounds;
            client.rcu_quiesce();
        }
    }

    client.report(result);
}

// test concurrent splits with removes in lower layers
template <typename C>
void kvtest_splitremove1(C &client)
{
    // XXX these parameters depend on masstree constants...
    int leaf_width = 15, internode_width = 15;
    int num_keys = leaf_width * (internode_width + 1) + 1;
    int trigger_key = num_keys - 15;
    int rounds = 0;
    Json result, errj;

    if (client.id() == 0) {
        while (1) {
            for (int i = 0; i < num_keys; ++i)
                client.put_key16(i + 100, i + 101);
            client.rcu_quiesce();
            for (int i = trigger_key + 1; i < num_keys + 10; ++i)
                client.remove_key16(i + 100);
            client.rcu_quiesce();
            for (int i = 0; i < leaf_width * internode_width; ++i)
                client.put_key16(i, i + 1);

            client.put(client.nthreads(), client.nthreads() + 1);
            for (int i = 1; i < client.nthreads(); ++i)
                client.put(i, i + 1);
            for (int i = 1; i < client.nthreads(); ++i) {
                while (!client.timeout(0) && client.get_sync(i))
                    /* do nothing */;
            }
            client.remove_key16(trigger_key);
            client.remove(client.nthreads());
            if (client.timeout(0))
                break;

            for (int i = 0; i < num_keys; ++i) {
                client.remove_key16(i);
                client.remove_key16(i + 100);
            }
            for (int i = 0; i < 10; ++i)
                client.rcu_quiesce();
            ++rounds;
        }

    } else {
        quick_istr me(client.id()), trigger(trigger_key, 16);
        while (1) {
            while (!client.timeout(0) && !client.get_sync_key16(trigger_key))
                client.rcu_quiesce();
            if (client.timeout(0))
                break;

            for (int i = 0; !client.get_sync(me.string()); ++i) {
                if (!client.get_sync(trigger.string()) && !client.timeout(0)) {
                    if (errj.size() == 100)
                        errj.push_back("more errors");
                    else if (errj.size() < 100)
                        errj.push_back("key " + String(trigger.string()) + " missing after " + String(rounds) + " rounds, counter " + String(i));
                    break;
                }
                client.rcu_quiesce();
            }

            while (!client.timeout(0) && !client.get_sync(me.string()))
                client.rcu_quiesce();
            client.remove(me.string());
            while (!client.timeout(0) && client.get_sync(client.nthreads()))
                client.rcu_quiesce();
            if (client.timeout(0))
                break;

            for (int i = 0; i < 10; ++i)
                client.rcu_quiesce();
            ++rounds;
        }
    }

    result.set("ok", errj.empty()).set("rounds", rounds);
    if (errj)
        result.set("errors", errj);
    client.report(result);
}

template <typename C>
void kvtest_url_seed(C &client)
{
    if (!client.param("file").is_s()) {
        client.report(Json::object("ok", false, "error", "need 'file=URLFILE' parameter"));
        return;
    }

    std::ifstream infile_url_init(client.param("file").to_s());
    std::ifstream infile_url_del_get(client.param("file").to_s());
    std::string ops;
    std::string url;
    unsigned count_i = 0;
    unsigned count_d = 0;
    unsigned count_g = 0;

    double t0 = client.now();
    while (count_i < client.limit() && infile_url_init.good()) {
        //do the following alternately:
        //insert 10 urls, then delete 5 inserted urls
        for (int i = 0; i != 10 && infile_url_init >> ops >> url; ++i, ++count_i)
            client.put(url, 2014);
        for (int i = 0; i != 5 && infile_url_del_get >> ops >> url; ++i, ++count_d)
            client.remove(url);
    }
    client.wait_all();
    client.puts_done();
    double t1 = client.now();
    infile_url_init.close();
    client.notice("\ninsert done\n");

    //query all the inserted urls
    double t2 = client.now();
    while (count_d + count_g != count_i && infile_url_del_get >> ops >> url) {
        client.get_check(Str(url), 2014);
        ++count_g;
    }
    client.wait_all();
    double t3 = client.now();

    // client.notice("Total pool memory: %d\n", client.ti_->poolmem);
    // client.notice("Total general memory: %d\n", client.ti_->genmem);
    // client.notice("Total MEMORY: %d\n", client.ti_->poolmem + client.ti_->genmem);

    Json result = Json::object("puts", count_i, "removes", count_d);
    kvtest_set_time(result, "gets", count_g, t3 - t2);
    kvtest_set_time(result, "ops", count_i + count_d, t1 - t0);
    client.report(result);
}

template <typename C>
void kvtest_url(C &client)
{
  kvtest_url_seed(client);
}

#endif
