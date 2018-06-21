#include "incll_globals.hh"
#include "incll_copy.hh"
#include "test_mocktree.hh"

__thread typename MockMasstree::table_params::threadinfo_type* MockMasstree::ti = nullptr;
volatile mrcu_epoch_type failedepoch = 0;
volatile mrcu_epoch_type globalepoch = 1;
volatile mrcu_epoch_type active_epoch = 1;

#define N_OPS 1000

void test_copy(MockMasstree* mt){
	for(int i=0;i<1000;++i){
		mt->insert(i, i+1);
	}

	void *copy = copy_tree(mt->get_root());

	assert(is_same_tree(mt->get_root(), copy));

	clear_copy<decltype(mt->get_root())>(copy);
	assert(copy == nullptr);

	copy = copy_tree(mt->get_root());

	mt->insert(N_OPS, N_OPS+1);

	assert(!is_same_tree(mt->get_root(), copy));
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
