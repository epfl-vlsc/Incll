#include "incll_globals.hh"
#include "incll_copy.hh"
#include "test_mocktree.hh"

__thread typename MockMasstree::table_params::threadinfo_type* MockMasstree::ti = nullptr;
volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;
int delaycount = 0;

#define N_OPS 1000

void test_copy(MockMasstree* mt){
	for(uint64_t i=0;i<1000;++i){
		mt->insert({i});
	}

	void *copy = copy_tree(mt->get_root());

	assert(is_same_tree(mt->get_root(), copy));

	clear_copy<decltype(mt->get_root())>(copy);
	assert(copy == nullptr);

	copy = copy_tree(mt->get_root());

	mt->insert({(uint64_t)N_OPS});

	//if not same tree, it crashes and displays the difference
	//assert(!is_same_tree(mt->get_root(), copy));
}

void do_experiment(std::string fnc_name, void (*fnc)(MockMasstree *)){
	auto mt = new MockMasstree();
	mt->thread_init(0);

	printf("%s\n", (fnc_name + " begin").c_str());
	fnc(mt);
	printf("\t%s\n", (fnc_name + " passed asserts").c_str());
}

#define DO_EXPERIMENT(test) \
	do_experiment(#test, test);

int main(){
	DO_EXPERIMENT(test_copy)

	return 0;
}
