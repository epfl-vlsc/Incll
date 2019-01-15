# Durable Masstree #

This is the experimental source release for Durable Masstree, a fast, multi-core key-value store that runs on NVM. 

This document describes how to run durable Masstree results.

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
    
## Persistent Region Size ##
* In `kvthread_pallocator.hh` and `incll_pextlog.hh`, please modify `DATA_BUF_SIZE, PDATA_FILENAME, PDATA_DIRNAME, PBUF_SIZE, PLOG_FILENAME` according to the mapping location and size of memory that you want to use.

## Testing ##
Run the script for tests. Check `incll_configs.hh` for trying out different configurations.


	$ chmod +x *.sh

Run a single ycsb experiment.

	$ ./run_workload.sh ycsb_a_uni

Run many different experiments. Check the `output` folder for results.

	$ ./run_this.sh

## Experimental Results ##
	
	https://docs.google.com/spreadsheets/d/1pAZeWBC6P7nOY8_oDlYT_alZqMIIc0of9fWGUjBmW8U/edit?usp=sharing

## References ##

	Cohen, Aksun, Avni, Larus, "Fine-Grain Checkpointing with In-Cache-Line Logging," ASPLOS 2019.