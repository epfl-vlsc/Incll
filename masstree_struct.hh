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
#ifndef MASSTREE_STRUCT_HH
#define MASSTREE_STRUCT_HH
#include "masstree.hh"
#include "nodeversion.hh"
#include "stringbag.hh"
#include "mtcounters.hh"
#include "timestamp.hh"

#include "incll_configs.hh"
#include "incll_globals.hh"

typedef uint64_t mrcu_epoch_type;
extern volatile mrcu_epoch_type globalepoch;
extern volatile mrcu_epoch_type failedepoch;

namespace Masstree {

template <typename P>
struct make_nodeversion {
    typedef nodeversion_parameters<typename P::nodeversion_value_type> parameters_type;
    typedef typename mass::conditional<P::concurrent,
                                       nodeversion<parameters_type>,
                                       singlethreaded_nodeversion<parameters_type> >::type type;
};

template <typename P>
struct make_prefetcher {
    typedef typename mass::conditional<P::prefetch,
                                       value_prefetcher<typename P::value_type>,
                                       do_nothing>::type type;
};

template <typename P>
class node_base : public make_nodeversion<P>::type {
  public:
    static constexpr bool concurrent = P::concurrent;
    static constexpr int nikey = 1;
    typedef leaf<P> leaf_type;
    typedef internode<P> internode_type;
    typedef node_base<P> base_type;
    typedef typename P::value_type value_type;
    typedef leafvalue<P> leafvalue_type;
    typedef typename P::ikey_type ikey_type;
    typedef key<ikey_type> key_type;
    typedef typename make_nodeversion<P>::type nodeversion_type;
    typedef typename P::threadinfo_type threadinfo;

    mrcu_epoch_type loggedepoch;

    node_base(bool isleaf):
    	nodeversion_type(isleaf),
    	loggedepoch(globalepoch)
    {}

    inline base_type* parent() const {
        // almost always an internode
        if (this->isleaf())
            return static_cast<const leaf_type*>(this)->parent_;
        else
            return static_cast<const internode_type*>(this)->parent_;
    }
    inline bool parent_exists(base_type* p) const {
        return p != nullptr;
    }
    inline bool has_parent() const {
        return parent_exists(parent());
    }
    inline internode_type* locked_parent(threadinfo& ti) const;
    inline void set_parent(base_type* p) {
        if (this->isleaf())
            static_cast<leaf_type*>(this)->parent_ = p;
        else
            static_cast<internode_type*>(this)->parent_ = p;
    }
    inline void make_layer_root() {
        set_parent(nullptr);
        this->mark_root();
    }
    inline base_type* maybe_parent() const {
        base_type* x = parent();
        return parent_exists(x) ? x : const_cast<base_type*>(this);
    }

    inline leaf_type* reach_leaf(const key_type& k, nodeversion_type& version,
                                 threadinfo& ti) const;

    void prefetch_full() const {
        for (int i = 0; i < std::min(16 * std::min(P::leaf_width, P::internode_width) + 1, 4 * 64); i += 64)
            ::prefetch((const char *) this + i);
    }

    void print(FILE* f, const char* prefix, int depth, int kdepth) const;
    void print_node() const;

    void record_node(){
    	REC_ASSERT(this->locked())
		if(this->isleaf()){
			this->to_leaf()->record_node();
		}else{
			this->to_internode()->record_node();
		}
	}

    int number_of_keys(){
    	if(this->isleaf()){
			return this->to_leaf()->number_of_keys();
		}else{
			return this->to_internode()->number_of_keys();
		}
    }

	template <typename SF>
	nodeversion_type lock_persistent(nodeversion_type expected, SF spin_function){
		auto res = nodeversion_type::lock(expected, spin_function);
		this->record_node();
		return res;
	}

	nodeversion_type lock_persistent(){
		auto res = nodeversion_type::lock();
		this->record_node();
		return res;
	}

#ifdef INCLL
	void log_persistent(){
		this->record_node();
	}
#endif //incll
	void fix_lock(){
		if(this->locked()){
			this->unlock();
		}
	}

    leaf_type* to_leaf(){
    	return static_cast<leaf_type*>(this);
    }

    internode_type* to_internode(){
    	return static_cast<internode_type*>(this);
    }

    const leaf_type* to_const_leaf() const{
		return static_cast<const leaf_type*>(this);
	}

    const internode_type* to_const_internode() const{
		return static_cast<const internode_type*>(this);
	}

    size_t allocated_size() const {
		if(this->isleaf()){
			return this->to_const_leaf()->allocated_size();
		}else{
			return this->to_const_internode()->allocated_size();
		}
	}

    static leaf_type* get_max_leaf(base_type *n){
    	if(n->isleaf()){
    		return n->to_leaf();
    	}else{
    		internode_type *in = n->to_internode();
    		auto in_keys = in->nkeys_;

    		assert(in->child_[in_keys]);
    		return base_type::get_max_leaf(in->child_[in_keys]);
    	}
    }

    static leaf_type* get_min_leaf(base_type *n){
    	if(n->isleaf()){
			return n->to_leaf();
		}else{
			internode_type *in = n->to_internode();
			auto *child_bound = in->child_[0];
			auto *child_normal = in->child_[1];
			if(child_bound){
				return base_type::get_min_leaf(child_bound);
			}else if(child_normal){
				return base_type::get_min_leaf(child_normal);
			}else{
				assert(0);
				return nullptr;
			}
		}
    }
};

template <typename P>
class internode : public node_base<P> {
  public:
    static constexpr int width = P::internode_width;
    typedef typename node_base<P>::nodeversion_type nodeversion_type;
    typedef key<typename P::ikey_type> key_type;
    typedef typename P::ikey_type ikey_type;
    typedef node_base<P> base_type;
	typedef leaf<P> leaf_type;
	typedef internode<P> internode_type;
    typedef typename key_bound<width, P::bound_method>::type bound_type;
    typedef typename P::threadinfo_type threadinfo;

    uint8_t nkeys_;
    uint32_t height_;
    ikey_type ikey0_[width];
    node_base<P>* child_[width + 1];
    node_base<P>* parent_;
    kvtimestamp_t created_at_[P::debug_level > 0];

    internode(uint32_t height)
        : node_base<P>(false), nkeys_(0), height_(height), parent_() {
    }

    static internode<P>* make(uint32_t height, threadinfo& ti) {
        void* ptr = ti.pool_allocate(sizeof(internode<P>),
                                     memtag_masstree_internode);
        internode<P>* n = new(ptr) internode<P>(height);
        assert(n);
        if (P::debug_level > 0)
            n->created_at_[0] = ti.operation_timestamp();
        return n;
    }

    void record_node(){
		if(this->loggedepoch != globalepoch){
			DBGLOG("record internode ge:%lu", globalepoch)
			GH::node_logger.record(this);
			this->loggedepoch = globalepoch;
		}
	}

    int number_of_keys(){
    	return nkeys_;
    }

    int size() const {
        return nkeys_;
    }

    key_type get_key(int p) const {
        return key_type(ikey0_[p]);
    }
    ikey_type ikey(int p) const {
        return ikey0_[p];
    }
    int compare_key(ikey_type a, int bp) const {
        return ::compare(a, ikey(bp));
    }
    int compare_key(const key_type& a, int bp) const {
        return ::compare(a.ikey(), ikey(bp));
    }
    inline int stable_last_key_compare(const key_type& k, nodeversion_type v,
                                       threadinfo& ti) const;

    void prefetch() const {
        for (int i = 64; i < std::min(16 * width + 1, 4 * 64); i += 64)
            ::prefetch((const char *) this + i);
    }

    void print(FILE* f, const char* prefix, int depth, int kdepth) const;
    void print_node() const;

    void deallocate(threadinfo& ti) {
        ti.pool_deallocate(this, sizeof(*this), memtag_masstree_internode);
    }
    void deallocate_rcu(threadinfo& ti) {
        ti.pool_deallocate_rcu(this, sizeof(*this), memtag_masstree_internode);
    }

    size_t allocated_size() const {
		return sizeof(*this);
	}

    int find_child_idx(base_type* child){
    	int idx = -1;
    	for (int i = 0; i <= this->size(); ++i) {
    		if(this->child_[i] == child){
    			idx = i;
    			break;
    		}
    	}
    	if(idx == -1){
    		this->print_node();
    		printf("BOOM child:%p parent:%p %d size %d\n",
    				(void*)child, (void*)this, idx, this->size());
    	}
    	assert(idx != -1);
    	return idx;
    }

  private:
    void assign(int p, ikey_type ikey, node_base<P>* child) {
        child->set_parent(this);
        child_[p + 1] = child;
        ikey0_[p] = ikey;
    }

    void shift_from(int p, const internode<P>* x, int xp, int n) {
        masstree_precondition(x != this);
        if (n) {
            memcpy(ikey0_ + p, x->ikey0_ + xp, sizeof(ikey0_[0]) * n);
            memcpy(child_ + p + 1, x->child_ + xp + 1, sizeof(child_[0]) * n);
        }
    }
    void shift_up(int p, int xp, int n) {
        memmove(ikey0_ + p, ikey0_ + xp, sizeof(ikey0_[0]) * n);
        for (node_base<P> **a = child_ + p + n, **b = child_ + xp + n; n; --a, --b, --n)
            *a = *b;
    }
    void shift_down(int p, int xp, int n) {
        memmove(ikey0_ + p, ikey0_ + xp, sizeof(ikey0_[0]) * n);
        for (node_base<P> **a = child_ + p + 1, **b = child_ + xp + 1; n; ++a, ++b, --n)
            *a = *b;
    }

    int split_into(internode<P>* nr, int p, ikey_type ka, node_base<P>* value,
                   ikey_type& split_ikey, int split_type);

    template <typename PP> friend class tcursor;
};

template <typename P>
class leafvalue {
  public:
    typedef typename P::value_type value_type;
    typedef typename make_prefetcher<P>::type prefetcher_type;

    leafvalue() {
    }
    leafvalue(value_type v) {
        u_.v = v;
    }
    leafvalue(node_base<P>* n) {
        u_.x = reinterpret_cast<uintptr_t>(n);
    }

    static leafvalue<P> make_empty() {
        return leafvalue<P>(value_type());
    }

    typedef bool (leafvalue<P>::*unspecified_bool_type)() const;
    operator unspecified_bool_type() const {
        return u_.x ? &leafvalue<P>::empty : 0;
    }
    bool empty() const {
        return !u_.x;
    }

    value_type value() const {
        return u_.v;
    }
    value_type& value() {
        return u_.v;
    }

    node_base<P>* layer() const {
        return reinterpret_cast<node_base<P>*>(u_.x);
    }

    void prefetch(int keylenx) const {
        if (!leaf<P>::keylenx_is_layer(keylenx))
            prefetcher_type()(u_.v);
        else
            u_.n->prefetch_full();
    }

  private:
    union {
        node_base<P>* n;
        value_type v;
        uintptr_t x;
    } u_;
};

template <typename P>
class leaf : public node_base<P> {
  public:
    static constexpr int width = P::leaf_width;
    typedef typename node_base<P>::nodeversion_type nodeversion_type;
    typedef key<typename P::ikey_type> key_type;
    typedef node_base<P> base_type;
    typedef leaf<P> leaf_type;
    typedef internode<P> internode_type;

    typedef typename node_base<P>::leafvalue_type leafvalue_type;
    typedef kpermuter<P::leaf_width> permuter_type;
    typedef typename P::ikey_type ikey_type;
    typedef typename key_bound<width, P::bound_method>::type bound_type;
    typedef typename P::threadinfo_type threadinfo;
    typedef stringbag<uint8_t> internal_ksuf_type;
    typedef stringbag<uint16_t> external_ksuf_type;
    typedef typename P::phantom_epoch_type phantom_epoch_type;
    static constexpr int ksuf_keylenx = 64;
    static constexpr int layer_keylenx = 128;
    static constexpr const int8_t invalid_idx = -1;

    enum {
        modstate_insert = 0, modstate_remove = 1, modstate_deleted_layer = 2
    };

#ifdef INCLL
    struct incll_lv_{										//16 bytes
    	leafvalue_type lv_;										//8 byte
    	int8_t cl_idx;											//1 byte
    	uint32_t loggedepoch;									//4 byte
    	uint8_t padding[3];										//3 byte

    	//no need to init index
    	//because it will be overwritten in next epoch
    	incll_lv_():cl_idx(invalid_idx), loggedepoch(globalepoch){}

    	bool is_le_diff(){
			return loggedepoch != globalepoch;
		}

		void print() const;
    };
#endif //incll


#ifdef INCLL
    //version value										//4 bytes
    //logged epoch										//8 bytes
    bool not_logged;									//1 byte
    int8_t extrasize64_;								//1 byte
	uint8_t modstate_; 									//1 byte
	uint8_t keylenx_[width];							//12 bytes
	typename permuter_type::storage_type permutation_;	//8 bytes
	typename permuter_type::storage_type perm_cl0;		//8 bytes
	int8_t cl0_idx;										//1 byte
	external_ksuf_type* ksuf_;							//8 bytes

	incll_lv_ lv_cl1;									//16 bytes
	leafvalue_type lv_[width];							//96 bytes
	incll_lv_ lv_cl2;									//16 bytes

	ikey_type ikey0_[width];							//96 bytes

	union {leaf<P>* ptr;uintptr_t x;} next_;			//8 bytes
	leaf<P>* prev_;										//8 bytes
	node_base<P>* parent_;								//8 bytes

	phantom_epoch_type phantom_epoch_[P::need_phantom_epoch];
	kvtimestamp_t created_at_[P::debug_level > 0];
	internal_ksuf_type iksuf_[0];
#else // incll
    int8_t extrasize64_;
    uint8_t modstate_;
    uint8_t keylenx_[width];
    typename permuter_type::storage_type permutation_;
    ikey_type ikey0_[width];
    leafvalue_type lv_[width];
    external_ksuf_type* ksuf_;
    union {
        leaf<P>* ptr;
        uintptr_t x;
    } next_;
    leaf<P>* prev_;
    node_base<P>* parent_;
    phantom_epoch_type phantom_epoch_[P::need_phantom_epoch];
    kvtimestamp_t created_at_[P::debug_level > 0];
    internal_ksuf_type iksuf_[0];
#endif //incll

    leaf(size_t sz, phantom_epoch_type phantom_epoch):
#ifdef INCLL
    	node_base<P>(true), not_logged(false), modstate_(modstate_insert),
		permutation_(permuter_type::make_empty()), cl0_idx(invalid_idx),
		ksuf_(), parent_(), iksuf_{} {
#else //incll
		node_base<P>(true), modstate_(modstate_insert),
		permutation_(permuter_type::make_empty()),
		ksuf_(), parent_(), iksuf_{} {
#endif //incll
        masstree_precondition(sz % 64 == 0 && sz / 64 < 128);
        extrasize64_ = (int(sz) >> 6) - ((int(sizeof(*this)) + 63) >> 6);
        if (extrasize64_ > 0)
            new((void *)&iksuf_[0]) internal_ksuf_type(width, sz - sizeof(*this));
        if (P::need_phantom_epoch)
            phantom_epoch_[0] = phantom_epoch;

#ifdef INCLL
        static_assert(
        		(uintptr_t)(&((leaf<P>*)0)->lv_cl1) % 64 == 0,
        		"incll for lv_ is not cache aligned properly");
#endif //incll
    }
		int number_of_keys(){
			permuter_type perm = permutation_;
			return perm.size();
		}

#ifdef INCLL
		void record_node(){
			if(this->loggedepoch != globalepoch || not_logged){
				DBGLOG("record leaf ge:%lu nl:%d keys:%d", globalepoch, not_logged, this->number_of_keys())
				not_logged=false;
				GH::node_logger.record(this);
				this->loggedepoch = globalepoch;
				this->invalidate_cls();
			}
		}

		void save_cl0_insert(){
			if(this->loggedepoch != globalepoch){
				DBGLOG("save incll insert to %p ge:%lu le:%lu keys:%d",
						(void*)this, globalepoch, this->loggedepoch, this->number_of_keys())
				perm_cl0 = permutation_;
				cl0_idx = 0;
				this->update_epochs(globalepoch);
				not_logged = true;
			}else if(not_logged){
				DBGLOG("log node insert to %p ge:%lu le:%lu",
						(void*)this, globalepoch, this->loggedepoch)
				not_logged=false;
				GH::node_logger.record(this);
				this->invalidate_cls();
			}
		}

		void save_cl1_2_update(int p){
			assert(p<width); //todo disable

			if(this->loggedepoch != globalepoch){
				DBGLOG("save incll to %p update ge:%lu le:%lu keys:%d",
						(void*)this, globalepoch, this->loggedepoch, this->number_of_keys())
				if(p < KEY_MID){
					lv_cl1.lv_ = this->lv_[p];
					lv_cl1.cl_idx = p;
				}else{
					lv_cl2.lv_ = this->lv_[p];
					lv_cl2.cl_idx = p;
				}
				this->update_epochs(globalepoch);
				not_logged = true;
			}else if(this->not_logged){
				DBGLOG("log node update to %p ge:%lu le:%lu",
						(void*)this, globalepoch, this->loggedepoch)
				not_logged=false;
				GH::node_logger.record(this);
				this->invalidate_cls();
			}
		}

		void fix_insert(){
			if(this->inserting()){
				DBGLOG("fix_insert_state")
				modstate_ = modstate_remove;
				this->clear_insert();
			}
		}

		void fix_all(){
			//todo reason about this, temporary for comparisons
			DBGLOG("fix_state")
			this->fix_insert();
			this->fix_lock();
			this->not_logged=false;
		}

		void undo_incll(){
			bool did_recovery = false;
			if(cl0_idx != invalid_idx){
				permutation_ = perm_cl0;
				did_recovery = true;
			}
			if(lv_cl1.cl_idx != invalid_idx){
				lv_[lv_cl1.cl_idx] = lv_cl1.lv_;
				did_recovery = true;
			}
			if(lv_cl2.cl_idx != invalid_idx){
				lv_[lv_cl2.cl_idx] = lv_cl2.lv_;
				did_recovery = true;
			}
			if(did_recovery){
				DBGLOG("undo_incll")
				this->invalidate_cls();
				this->update_epochs(globalepoch-1);
				this->fix_all();
			}

			//reset to a common insert state
			this->modstate_ = modstate_insert;
			this->clear_insert_extras();
		}

		void update_epochs(mrcu_epoch_type e){
			DBGLOG("update_epochs ge:%lu", e)
			this->loggedepoch
				= lv_cl1.loggedepoch
				= lv_cl2.loggedepoch
				= e;
		}

		void invalidate_cls(){
			DBGLOG("invalidate_cls")
			this->cl0_idx
				= lv_cl1.cl_idx
				= lv_cl2.cl_idx
				= invalid_idx;
		}

		bool has_valid_cl(){
			DBGLOG("has_valid_cl cl0:%d cl1:%d cl2:%d",
					this->cl0_idx, lv_cl1.cl_idx, lv_cl2.cl_idx)
			return this->cl0_idx != invalid_idx
				|| lv_cl1.cl_idx != invalid_idx
				|| lv_cl2.cl_idx != invalid_idx;
		}

		void print_cl0() const;
#endif //incll

    static leaf<P>* make(int ksufsize, phantom_epoch_type phantom_epoch, threadinfo& ti) {
        size_t sz = iceil(sizeof(leaf<P>) + std::min(ksufsize, 128), 64);
        void* ptr = ti.pool_allocate(sz, memtag_masstree_leaf);
        leaf<P>* n = new(ptr) leaf<P>(sz, phantom_epoch);
        assert(n);
        if (P::debug_level > 0)
            n->created_at_[0] = ti.operation_timestamp();
        return n;
    }
    static leaf<P>* make_root(int ksufsize, leaf<P>* parent, threadinfo& ti) {
        leaf<P>* n = make(ksufsize, parent ? parent->phantom_epoch() : phantom_epoch_type(), ti);
        n->next_.ptr = n->prev_ = 0;
        n->make_layer_root();
        return n;
    }

    static size_t min_allocated_size() {
        return (sizeof(leaf<P>) + 63) & ~size_t(63);
    }
    size_t allocated_size() const {
        int es = (extrasize64_ >= 0 ? extrasize64_ : -extrasize64_ - 1);
        return (sizeof(*this) + es * 64 + 63) & ~size_t(63);
    }
    phantom_epoch_type phantom_epoch() const {
        return P::need_phantom_epoch ? phantom_epoch_[0] : phantom_epoch_type();
    }

    int size() const {
        return permuter_type::size(permutation_);
    }
    permuter_type permutation() const {
        return permuter_type(permutation_);
    }
    typename nodeversion_type::value_type full_version_value() const {
        static_assert(int(nodeversion_type::traits_type::top_stable_bits) >= int(permuter_type::size_bits), "not enough bits to add size to version");
        return (this->version_value() << permuter_type::size_bits) + size();
    }
    typename nodeversion_type::value_type full_unlocked_version_value() const {
        static_assert(int(nodeversion_type::traits_type::top_stable_bits) >= int(permuter_type::size_bits), "not enough bits to add size to version");
        typename node_base<P>::nodeversion_type v(*this);
        if (v.locked())
            // subtlely, unlocked_version_value() is different than v.unlock(); v.version_value() because the latter will add a
            // split bit if we're doing a split. So we do the latter to get the fully correct version.
            v.unlock();
        return (v.version_value() << permuter_type::size_bits) + size();
    }

    using node_base<P>::has_changed;
    bool has_changed(nodeversion_type oldv,
                     typename permuter_type::storage_type oldperm) const {
        return this->has_changed(oldv) || oldperm != permutation_;
    }

    key_type get_key(int p) const {
        int keylenx = keylenx_[p];
        if (!keylenx_has_ksuf(keylenx))
            return key_type(ikey0_[p], keylenx);
        else
            return key_type(ikey0_[p], ksuf(p));
    }
    ikey_type ikey(int p) const {
        return ikey0_[p];
    }
    ikey_type ikey_bound() const {
        return ikey0_[0];
    }
    int compare_key(const key_type& a, int bp) const {
        return a.compare(ikey(bp), keylenx_[bp]);
    }
    inline int stable_last_key_compare(const key_type& k, nodeversion_type v,
                                       threadinfo& ti) const;

    inline leaf<P>* advance_to_key(const key_type& k, nodeversion_type& version,
                                   threadinfo& ti) const;

    leaf<P>* get_prev_safe(){
    	base_type *cn = this;
    	while(cn->has_parent()){
    		base_type *pn = cn->parent();
			assert(!pn->isleaf());
			internode_type *pin = pn->to_internode();

			int idx = pin->find_child_idx(cn);

    		//case exists a prev node
    		if(idx>0){
    			//get the leaf to the most right
    			base_type *node_for_prev = pin->child_[idx-1];

    			//make sure the bound has an item, idx-1 = 0
    			if(node_for_prev){
    				leaf_type *ln = base_type::get_max_leaf(node_for_prev);
					assert(ln != this);
					return ln;
    			}

    			//if bound does not have an item go up to parent
    		}

    		//case no prev in this node
    		cn = pn;
    	}

    	//case no parent
    	return nullptr;
    }

    leaf<P>* get_next_safe(){
    	base_type *cn = this;
		while(cn->has_parent()){
			base_type *pn = cn->parent();
			assert(!pn->isleaf());
			internode_type *pin = pn->to_internode();

			int idx = pin->find_child_idx(cn);

			//case exists a prev node
			if(idx<pin->size()){
				//get the leaf to the most right
				base_type *node_for_next = pin->child_[idx+1];
				assert(node_for_next);
				leaf_type *ln = base_type::get_min_leaf(node_for_next);
				if(ln){
					assert(ln != this);
					return ln;
				}
			}

			//case no prev in this node
			cn = pn;
		}

		//case no parent
		return nullptr;
    }

    static bool keylenx_is_layer(int keylenx) {
        return keylenx > 127;
    }
    static bool keylenx_has_ksuf(int keylenx) {
        return keylenx == ksuf_keylenx;
    }

    bool is_layer(int p) const {
        return keylenx_is_layer(keylenx_[p]);
    }
    bool has_ksuf(int p) const {
        return keylenx_has_ksuf(keylenx_[p]);
    }
    Str ksuf(int p, int keylenx) const {
        (void) keylenx;
        masstree_precondition(keylenx_has_ksuf(keylenx));
        return ksuf_ ? ksuf_->get(p) : iksuf_[0].get(p);
    }
    Str ksuf(int p) const {
        return ksuf(p, keylenx_[p]);
    }
    bool ksuf_equals(int p, const key_type& ka) const {
        return ksuf_equals(p, ka, keylenx_[p]);
    }
    bool ksuf_equals(int p, const key_type& ka, int keylenx) const {
        if (!keylenx_has_ksuf(keylenx))
            return true;
        Str s = ksuf(p, keylenx);
        return s.len == ka.suffix().len
            && string_slice<uintptr_t>::equals_sloppy(s.s, ka.suffix().s, s.len);
    }
    // Returns 1 if match & not layer, 0 if no match, <0 if match and layer
    int ksuf_matches(int p, const key_type& ka) const {
        int keylenx = keylenx_[p];
        if (keylenx < ksuf_keylenx)
            return 1;
        if (keylenx == layer_keylenx)
            return -(int) sizeof(ikey_type);
        Str s = ksuf(p, keylenx);
        return s.len == ka.suffix().len
            && string_slice<uintptr_t>::equals_sloppy(s.s, ka.suffix().s, s.len);
    }
    int ksuf_compare(int p, const key_type& ka) const {
        int keylenx = keylenx_[p];
        if (!keylenx_has_ksuf(keylenx))
            return 0;
        return ksuf(p, keylenx).compare(ka.suffix());
    }

    size_t ksuf_used_capacity() const {
        if (ksuf_)
            return ksuf_->used_capacity();
        else if (extrasize64_ > 0)
            return iksuf_[0].used_capacity();
        else
            return 0;
    }
    size_t ksuf_capacity() const {
        if (ksuf_)
            return ksuf_->capacity();
        else if (extrasize64_ > 0)
            return iksuf_[0].capacity();
        else
            return 0;
    }
    bool ksuf_external() const {
        return ksuf_;
    }
    Str ksuf_storage(int p) const {
        if (ksuf_)
            return ksuf_->get(p);
        else if (extrasize64_ > 0)
            return iksuf_[0].get(p);
        else
            return Str();
    }

    bool deleted_layer() const {
        return modstate_ == modstate_deleted_layer;
    }

    void prefetch() const {
        for (int i = 64; i < std::min(16 * width + 1, 4 * 64); i += 64)
            ::prefetch((const char *) this + i);
        if (extrasize64_ > 0)
            ::prefetch((const char *) &iksuf_[0]);
        else if (extrasize64_ < 0) {
            ::prefetch((const char *) ksuf_);
            ::prefetch((const char *) ksuf_ + CACHE_LINE_SIZE);
        }
    }

    void print(FILE* f, const char* prefix, int depth, int kdepth) const;
    void print_node() const;

    leaf<P>* safe_next() const {
        return reinterpret_cast<leaf<P>*>(next_.x & ~(uintptr_t) 1);
    }

    void deallocate(threadinfo& ti) {
        if (ksuf_)
            ti.deallocate(ksuf_, ksuf_->capacity(),
                          memtag_masstree_ksuffixes);
        if (extrasize64_ != 0)
            iksuf_[0].~stringbag();
        ti.pool_deallocate(this, allocated_size(), memtag_masstree_leaf);
    }
    void deallocate_rcu(threadinfo& ti) {
        if (ksuf_)
            ti.deallocate_rcu(ksuf_, ksuf_->capacity(),
                              memtag_masstree_ksuffixes);
        ti.pool_deallocate_rcu(this, allocated_size(), memtag_masstree_leaf);
    }

  private:
    inline void mark_deleted_layer() {
        modstate_ = modstate_deleted_layer;
    }

    inline void assign(int p, const key_type& ka, threadinfo& ti) {
        lv_[p] = leafvalue_type::make_empty();
        ikey0_[p] = ka.ikey();
        if (!ka.has_suffix())
            keylenx_[p] = ka.length();
        else {
            keylenx_[p] = ksuf_keylenx;
            assign_ksuf(p, ka.suffix(), false, ti);
        }
    }
    inline void assign_initialize(int p, const key_type& ka, threadinfo& ti) {
        lv_[p] = leafvalue_type::make_empty();
        ikey0_[p] = ka.ikey();
        if (!ka.has_suffix())
            keylenx_[p] = ka.length();
        else {
            keylenx_[p] = ksuf_keylenx;
            assign_ksuf(p, ka.suffix(), true, ti);
        }
    }
    inline void assign_initialize(int p, leaf<P>* x, int xp, threadinfo& ti) {
        lv_[p] = x->lv_[xp];
        ikey0_[p] = x->ikey0_[xp];
        keylenx_[p] = x->keylenx_[xp];
        if (x->has_ksuf(xp))
            assign_ksuf(p, x->ksuf(xp), true, ti);
    }
    inline void assign_initialize_for_layer(int p, const key_type& ka) {
        assert(ka.has_suffix());
        ikey0_[p] = ka.ikey();
        keylenx_[p] = layer_keylenx;
    }
    void assign_ksuf(int p, Str s, bool initializing, threadinfo& ti);

    inline ikey_type ikey_after_insert(const permuter_type& perm, int i,
                                       const key_type& ka, int ka_i) const;
    int split_into(leaf<P>* nr, int p, const key_type& ka, ikey_type& split_ikey,
                   threadinfo& ti);

    template <typename PP> friend class tcursor;
};


template <typename P>
void basic_table<P>::initialize(threadinfo& ti) {
    masstree_precondition(!root_);
    root_ = node_type::leaf_type::make_root(0, 0, ti);
}


/** @brief Return this node's parent in locked state.
    @pre this->locked()
    @post this->parent() == result && (!result || result->locked()) */
template <typename P>
internode<P>* node_base<P>::locked_parent(threadinfo& ti) const
{
    node_base<P>* p;
    masstree_precondition(!this->concurrent || this->locked());
    while (1) {
        p = this->parent();
        if (!this->parent_exists(p))
            break;

        nodeversion_type pv = p->lock_persistent(*p, ti.lock_fence(tc_internode_lock));
        if (p == this->parent()) {
            masstree_invariant(!p->isleaf());
            break;
        }
        p->unlock(pv);
        relax_fence();
    }
    return static_cast<internode<P>*>(p);
}


template <typename P>
void node_base<P>::print(FILE* f, const char* prefix, int depth, int kdepth) const
{
    if (this->isleaf())
        static_cast<const leaf<P>*>(this)->print(f, prefix, depth, kdepth);
    else
        static_cast<const internode<P>*>(this)->print(f, prefix, depth, kdepth);
}

template <typename P>
void node_base<P>::print_node() const
{
    if (this->isleaf())
        static_cast<const leaf<P>*>(this)->print_node();
    else
        static_cast<const internode<P>*>(this)->print_node();
}

/** @brief Return the result of compare_key(k, LAST KEY IN NODE).

    Reruns the comparison until a stable comparison is obtained. */
template <typename P>
inline int
internode<P>::stable_last_key_compare(const key_type& k, nodeversion_type v,
                                      threadinfo& ti) const
{
    while (1) {
        int cmp = compare_key(k, size() - 1);
        if (likely(!this->has_changed(v)))
            return cmp;
        v = this->stable_annotated(ti.stable_fence());
    }
}

template <typename P>
inline int
leaf<P>::stable_last_key_compare(const key_type& k, nodeversion_type v,
                                 threadinfo& ti) const
{
    while (1) {
        typename leaf<P>::permuter_type perm(permutation_);
        int p = perm[perm.size() - 1];
        int cmp = compare_key(k, p);
        if (likely(!this->has_changed(v)))
            return cmp;
        v = this->stable_annotated(ti.stable_fence());
    }
}


/** @brief Return the leaf in this tree layer responsible for @a ka.

    Returns a stable leaf. Sets @a version to the stable version. */
template <typename P>
inline leaf<P>* node_base<P>::reach_leaf(const key_type& ka,
                                         nodeversion_type& version,
                                         threadinfo& ti) const
{
    const node_base<P> *n[2];
    typename node_base<P>::nodeversion_type v[2];
    bool sense;

    // Get a non-stale root.
    // Detect staleness by checking whether n has ever split.
    // The true root has never split.
 retry:
    sense = false;
    n[sense] = this;
    while (1) {
        v[sense] = n[sense]->stable_annotated(ti.stable_fence());
        if (v[sense].is_root())
            break;
        ti.mark(tc_root_retry);
        n[sense] = n[sense]->maybe_parent();
    }

    // Loop over internal nodes.
    while (!v[sense].isleaf()) {
        const internode<P> *in = static_cast<const internode<P>*>(n[sense]);
        in->prefetch();
        int kp = internode<P>::bound_type::upper(ka, *in);
        n[!sense] = in->child_[kp];
        if (!n[!sense])
            goto retry;
        v[!sense] = n[!sense]->stable_annotated(ti.stable_fence());

        if (likely(!in->has_changed(v[sense]))) {
            sense = !sense;
            continue;
        }

        typename node_base<P>::nodeversion_type oldv = v[sense];
        v[sense] = in->stable_annotated(ti.stable_fence());
        if (oldv.has_split(v[sense])
            && in->stable_last_key_compare(ka, v[sense], ti) > 0) {
            ti.mark(tc_root_retry);
            goto retry;
        } else
            ti.mark(tc_internode_retry);
    }

    version = v[sense];
    return const_cast<leaf<P> *>(static_cast<const leaf<P> *>(n[sense]));
}

/** @brief Return the leaf at or after *this responsible for @a ka.
    @pre *this was responsible for @a ka at version @a v

    Checks whether *this has split since version @a v. If it has split, then
    advances through the leaves using the B^link-tree pointers and returns
    the relevant leaf, setting @a v to the stable version for that leaf. */
template <typename P>
leaf<P>* leaf<P>::advance_to_key(const key_type& ka, nodeversion_type& v,
                                 threadinfo& ti) const
{
    const leaf<P>* n = this;
    nodeversion_type oldv = v;
    v = n->stable_annotated(ti.stable_fence());
    if (v.has_split(oldv)
        && n->stable_last_key_compare(ka, v, ti) > 0) {
        leaf<P> *next;
        ti.mark(tc_leaf_walk);
        while (likely(!v.deleted()) && (next = n->safe_next())
               && compare(ka.ikey(), next->ikey_bound()) >= 0) {
            n = next;
            v = n->stable_annotated(ti.stable_fence());
        }
    }
    return const_cast<leaf<P>*>(n);
}


/** @brief Assign position @a p's keysuffix to @a s.

    This may allocate a new suffix container, copying suffixes over.

    The @a initializing parameter determines which suffixes are copied. If @a
    initializing is false, then this is an insertion into a live node. The
    live node's permutation indicates which keysuffixes are active, and only
    active suffixes are copied. If @a initializing is true, then this
    assignment is part of the initialization process for a new node. The
    permutation might not be set up yet. In this case, it is assumed that key
    positions [0,p) are ready: keysuffixes in that range are copied. In either
    case, the key at position p is NOT copied; it is assigned to @a s. */
template <typename P>
void leaf<P>::assign_ksuf(int p, Str s, bool initializing, threadinfo& ti) {
    if ((ksuf_ && ksuf_->assign(p, s))
        || (extrasize64_ > 0 && iksuf_[0].assign(p, s)))
        return;

    external_ksuf_type* oksuf = ksuf_;

    permuter_type perm(permutation_);
    int n = initializing ? p : perm.size();

    size_t csz = 0;
    for (int i = 0; i < n; ++i) {
        int mp = initializing ? i : perm[i];
        if (mp != p && has_ksuf(mp))
            csz += ksuf(mp).len;
    }

    size_t sz = iceil_log2(external_ksuf_type::safe_size(width, csz + s.len));
    if (oksuf)
        sz = std::max(sz, oksuf->capacity());

    void* ptr = ti.allocate(sz, memtag_masstree_ksuffixes);
    external_ksuf_type* nksuf = new(ptr) external_ksuf_type(width, sz);
    for (int i = 0; i < n; ++i) {
        int mp = initializing ? i : perm[i];
        if (mp != p && has_ksuf(mp)) {
            bool ok = nksuf->assign(mp, ksuf(mp));
            assert(ok); (void) ok;
        }
    }
    bool ok = nksuf->assign(p, s);
    assert(ok); (void) ok;
    fence();

    // removed ksufs aren't copied to the new ksuf, but observers
    // might need them. We ensure that observers must retry by
    // ensuring that we are not currently in the remove state.
    // State transitions are accompanied by mark_insert() so observers
    // will retry.
    masstree_invariant(modstate_ != modstate_remove);

    ksuf_ = nksuf;
    fence();

    if (extrasize64_ >= 0)      // now the new ksuf_ installed, mark old dead
        extrasize64_ = -extrasize64_ - 1;

    if (oksuf)
        ti.deallocate_rcu(oksuf, oksuf->capacity(),
                          memtag_masstree_ksuffixes);
}

template <typename P>
inline basic_table<P>::basic_table()
    : root_(0) {
}

template <typename P>
inline node_base<P>* basic_table<P>::root() const {
    return root_;
}

template <typename P>
inline node_base<P>* basic_table<P>::fix_root() {
    node_base<P>* root = root_;
    if (unlikely(!root->is_root())) {
        node_base<P>* old_root = root;
        root = root->maybe_parent();
        (void) cmpxchg(&root_, old_root, root);
    }
    return root;
}

template <typename P>
inline void basic_table<P>::set_root(void* new_root){
	root_ = (node_type*)new_root;
}

template <typename P>
inline node_base<P>*& basic_table<P>::root_assignable(){
    return root_;
}

} // namespace Masstree
#endif
