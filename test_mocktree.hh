#pragma once

#include <iostream>
#include <random>
#include <vector>
#include <thread>
#include <stdio.h>
#include <pthread.h>
#include <cassert>

#include "config.h"
#include "compiler.hh"

#include "masstree.hh"
#include "kvthread.hh"
#include "kvproto.hh"
#include "masstree_tcursor.hh"
#include "masstree_insert.hh"
#include "masstree_print.hh"
#include "masstree_remove.hh"
#include "masstree_scan.hh"
#include "masstree_stats.hh"
#include "string.hh"

class key_unparse_unsigned {
public:
    static int unparse_key(Masstree::key<uint64_t> key, char* buf, int buflen) {
        return snprintf(buf, buflen, "%" PRIu64, key.ikey());
    }
};

class MockMasstree {
public:
    static constexpr uint64_t insert_bound = 0xfffff; //0xffffff;
    struct table_params : public Masstree::nodeparams<12,11> {
        typedef uint64_t value_type;
        typedef Masstree::value_print<value_type> value_print_type;
        typedef threadinfo threadinfo_type;
        typedef key_unparse_unsigned key_unparse_type;
        static constexpr ssize_t print_max_indent_depth = 12;
    };

    typedef Masstree::Str Str;
    typedef Masstree::basic_table<table_params> table_type;
    typedef Masstree::unlocked_tcursor<table_params> unlocked_cursor_type;
    typedef Masstree::tcursor<table_params> cursor_type;

    typedef Masstree::leaf<table_params> leaf_type;
    typedef Masstree::internode<table_params> internode_type;

    typedef typename table_type::node_type node_type;
    typedef typename unlocked_cursor_type::nodeversion_value_type nodeversion_value_type;

    static __thread typename table_params::threadinfo_type *ti;

    MockMasstree() {
        this->table_init();
    }

    void table_init() {
        if (ti == nullptr)
            ti = threadinfo::make(threadinfo::TI_MAIN, -1);
        table_.initialize(*ti);
    }

    static void thread_init(int thread_id) {
        if (ti == nullptr)
            ti = threadinfo::make(threadinfo::TI_PROCESS, thread_id);
    }

    bool find(uint64_t int_key, uint64_t& int_val){
    	uint64_t key_buf;

		Str key = make_key(int_key, key_buf);

		unlocked_cursor_type lp(table_, key);
		bool found = lp.find_unlocked(*ti);

		if (found)
			int_val = lp.value();
		return found;
    }

    void insert(uint64_t int_key, uint64_t int_val){
    	uint64_t key_buf;

		Str key = make_key(int_key, key_buf);

		cursor_type lp(table_, key);
		bool found = lp.find_insert(*ti);
		always_assert(!found, "keys should all be unique");

		lp.value() = int_val;

		fence();
		lp.finish(1, *ti);
    }

    void insert(const std::initializer_list<uint64_t>& int_keys){
    	for(auto int_key: int_keys){
    		insert(int_key, int_key+1);
    	}
    }

    void remove(uint64_t int_key){
    	uint64_t key_buf;

		Str key = make_key(int_key, key_buf);

		cursor_type lp(table_, key);
		bool found = lp.find_locked(*ti);
		always_assert(found, "keys must all exist");
		lp.finish(-1, *ti);
    }

    void remove(const std::initializer_list<uint64_t>& int_keys){
		for(auto int_key: int_keys){
			remove(int_key);
		}
	}

    node_type* get_root(){
    	return table_.fix_root();
    }

private:
    table_type table_;

    static inline Str make_key(uint64_t int_key, uint64_t& key_buf) {
        key_buf = __builtin_bswap64(int_key);
        return Str((const char *)&key_buf, sizeof(key_buf));
    }
};
