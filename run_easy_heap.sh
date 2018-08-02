#!/usr/bin/env bash
heap=$1


if [ "$heap" = "create" ] ;then
cp /scratch/tmp/nvm.heap /scratch/tmp/zvm.heap
cp /scratch/tmp/nvm.log /scratch/tmp/zvm.log
fi

if [ "$heap" = "copy" ] ;then
rm -rf /tmp/nvm.*
cp /scratch/tmp/zvm.heap /scratch/tmp/nvm.heap
cp /scratch/tmp/zvm.log /scratch/tmp/nvm.log
fi

if [ "$heap" = "delete" ] ;then
rm -rf /scratch/tmp/nvm.*
fi

