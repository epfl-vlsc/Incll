# Masstree #

This is the source release for Durable Masstree, a fast, multi-core key-value
store that runs on NVM. 

This document describes how to run Masstree and interpret its
results.

## Dependencies ##
* Jemalloc: It is preferable that you use [jemalloc](https://samiux.blogspot.com/2017/02/howto-optimize-ubuntu-1604-lts-and-kali.html) as allocator.



## Installation ##

Masstree is tested on Debian, Ubuntu and Mac OS X. To build from
source:

    $ ./bootstrap.sh
    $ ./configure
    $ make mttest
    
## Cache line flusher ##

Load the kernel module for kernel flushes.

    $ make flush_all
    $ make flush_load
    

## Testing ##
Run the script for tests. Check `incll_configs.hh` for configuring the benchmark.


	$ chmod +x *.sh

Run a single ycsb experiment.

	$ ./run_workload.sh ycsb_a_uni

Run many different experiments. Check the `output` folder for results.

	$ ./run_this.sh