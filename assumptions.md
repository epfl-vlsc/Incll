# measurement
* ensure correct jemalloc is used, jemalloc-5.1.0
* check measurement scripts while running
* no madvise
* use nvm for allocations
* lw=14, flush freq=16, disable phantom epoch
* deallocation enabled
* node comparison is field based

## beware
* clear insert extras, take a look
* run LD_LIBRARY_PATH="", jemalloc safety

./bootstrap.sh
./configure
make mttest
chmod +x *.sh