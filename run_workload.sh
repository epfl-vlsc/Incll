#!/usr/bin/env bash
workload=$1
repeat=$2
#argument processing
if [ -z "$2" ]
  then
	repeat=10
fi

echo ${repeat}

make mttest
rm -rf *.json
for i in $(eval echo {1..$repeat}); do 
	rm -rf /tmp/nvm.heap	
	./mttest ${workload}
	python get_average.py
done
