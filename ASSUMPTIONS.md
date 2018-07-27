* leaf width = 14
* phantom epoch not used
* deallocation possible to disable
* for node comparison, fine grained check instead of memcmp for leaf

* clear insert extras, take a look

* beware of jemalloc configuration
run LD_LIBRARY_PATH=""
todo revert repeat variable in measurements to 10