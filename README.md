# Durable Masstree #

This is the experimental source release for Durable Masstree, a fast, multi-core key-value store that runs on NVM.
Ensuring the recoverability of the data after crash is expensive due to the cost involved in ordering the writes to NVM.
Durable Masstree offers a new design space, where the cost of ordering writes by using flushes and fences can be reduced through the use of fine-grain checkpointing and using a log that is stored within a cache line as described in Fine-Grain Checkpointing with In-Cache-Line Logging.
Durable Masstree code offers the possibility to explore the described design space.
  
This document describes how to run Durable Masstree and obtain results.

## Dependencies ##
* Jemalloc: It is preferable that you use [jemalloc](https://samiux.blogspot.com/2017/02/howto-optimize-ubuntu-1604-lts-and-kali.html) as allocator.

## Installation ##

Get the sources using:
	
	$ git clone https://github.com/epfl-vlsc/Masstree.git

Please make sure you have jemalloc installed.
Afterwards, run the commands below to build the project.

    $ ./bootstrap.sh
    $ ./configure
    $ make all
   
## Persistent Memory ##
Under construction, efficient support is added for PM use.

## Cache line flusher ##

Load the kernel module for kernel flushes.

    $ make flush_all
    $ make flush_load
    
## Persistent Region Size ##
* In `kvthread_pallocator.hh` please change `DATA_BUF_SIZE, PDATA_FILENAME, PDATA_DIRNAME` and in `incll_pextlog.hh`, please change, `PBUF_SIZE, PLOG_FILENAME` according to the mapping location and size of the memory available to your system.

## Testing ##
Run the script for tests. Check `incll_configs.hh` for trying out different configurations.


	$ chmod +x *.sh

Run a single ycsb experiment.

	$ ./run_workload.sh ycsb_a_uni 10

Run many different experiments. Check the `output` folder for results.

	$ ./run_this.sh

## Experimental Results ##
	
	https://docs.google.com/spreadsheets/d/1pAZeWBC6P7nOY8_oDlYT_alZqMIIc0of9fWGUjBmW8U/edit?usp=sharing

## References ##

	Cohen, Aksun, Avni, Larus, "Fine-Grain Checkpointing with In-Cache-Line Logging," ASPLOS 2019.
