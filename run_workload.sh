#!/usr/bin/env bash
workload=$1
repeat=1

#NKEYS=20000000
NKEYS=$2


if [ -z "$NKEYS" ]; then 
	NKEYS=20 
fi

echo "nkeys:"
echo "$((NKEYS*=1000000))"
echo "repeat ${repeat}"

make mttest
rm -rf *.json
for i in $(eval echo {1..$repeat}); do 
	rm -rf /scratch/tmp/nvm.*
	rm -rf /dev/shm/incll/nvm.*
	./mttest ${workload} --nops1=1000000 --ninitops=${NKEYS} --nkeys=${NKEYS} -j8 --pin
	python get_average.py
	sleep 1
done
