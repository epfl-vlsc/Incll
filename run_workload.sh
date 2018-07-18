#!/usr/bin/env bash
workload=$1

make mttest
rm -rf *.json
for i in 0 1 2 3 4 5 6 7 8 9; do 
	./mttest ${workload}
done
python get_average.py


