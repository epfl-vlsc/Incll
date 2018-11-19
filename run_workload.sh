#!/usr/bin/env bash
workload=$1
repeat=$2
#argument processing
if [ -z "$2" ]
  then
	repeat=2
fi

echo ${repeat}

make mttest
rm -rf *.json
for i in $(eval echo {1..$repeat}); do 
	rm -rf /scratch/tmp/nvm.*
	rm -rf /dev/shm/incll/nvm.*
	./mttest ${workload} --nops1=1000000 --ninitops=20000000 --nkeys=20000000 -j8 --pin
	python get_average.py
	sleep 1
done
