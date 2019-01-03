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

#include "incll_trav.hh"
#include "incll_configs.hh"

extern volatile mrcu_epoch_type failedepoch;
extern volatile mrcu_epoch_type globalepoch;
extern volatile mrcu_epoch_type active_epoch;
extern volatile void *global_masstree_root;

class key_unparse_unsigned {
public:
    static int unparse_key(Masstree::key<uint64_t> key, char* buf, int buflen) {
        return snprintf(buf, buflen, "%" PRIu64, key.ikey());
    }
};

class MockMasstree {
public:
    static constexpr uint64_t insert_bound = 0xfffff; //0xffffff;
    struct table_params : public Masstree::nodeparams<KEY_LW,15> {
        typedef uint64_t* value_type;
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

    bool find(uint64_t int_key, uint64_t** int_val){
    	uint64_t key_buf;

		Str key = make_key(int_key, key_buf);

		unlocked_cursor_type lp(table_, key);
		bool found = lp.find_unlocked(*ti);

		if (found)
			*int_val = lp.value();
		return found;
    }

    void insert(uint64_t int_key, uint64_t* int_val){
    	DBGLOG("insert %lu", int_key)

    	uint64_t key_buf;

		Str key = make_key(int_key, key_buf);

		cursor_type lp(table_, key);
		lp.find_insert(*ti);

		lp.value() = int_val;

		fence();
		lp.finish(1, *ti);
    }

    void insert(const std::initializer_list<uint64_t>& int_keys){
    	for(auto int_key: int_keys){
    		uint64_t *int_val = new uint64_t;
    		*int_val = int_key+1;
    		insert(int_key, int_val);
    	}
    }

    void remove(uint64_t int_key){
    	DBGLOG("remove %lu", int_key)
    	uint64_t key_buf;

		Str key = make_key(int_key, key_buf);

		cursor_type lp(table_, key);
		bool found = lp.find_locked(*ti);
		if(found) lp.log_persistent();

		lp.finish(-1, *ti);
    }

    void remove(const std::initializer_list<uint64_t>& int_keys){
		for(auto int_key: int_keys){
			remove(int_key);
		}
	}

    node_type* get_root(){
    	return table_.root();
    }

    void set_root(void *new_root){
		return table_.set_root(new_root);
	}

    node_type*& get_root_assignable(){
		return table_.root_assignable();
	}

private:
    table_type table_;

    static inline Str make_key(uint64_t int_key, uint64_t& key_buf) {
        key_buf = __builtin_bswap64(int_key);
        return Str((const char *)&key_buf, sizeof(key_buf));
    }
};

void adv_epoch(MockMasstree *mt){
	globalepoch++;
	GH::node_logger->set_log_root(mt->get_root());
	global_masstree_root = mt->get_root();
	GH::node_logger->checkpoint();
	DBGLOG("new ge:%lu", globalepoch);
}

void undo_all(MockMasstree *mt){
	void *undo_root = GH::node_logger->get_tree_root();
	mt->set_root(undo_root);
	auto last_flush = GH::node_logger->get_last_flush();
	GH::node_logger->undo(mt->get_root());
	GH::node_logger->undo_next_prev(mt->get_root(), last_flush);
}

void set_failed_epoch(mrcu_epoch_type fe){
	failedepoch = fe;
	DBGLOG("fe:%lu", failedepoch);
}

void assert_tree_size(MockMasstree *mt, size_t size_expected){
	assert(size_expected == get_tree_size(mt->get_root()));
}
