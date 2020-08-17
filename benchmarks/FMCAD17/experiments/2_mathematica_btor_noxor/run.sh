#!/bin/sh
i=2
while [ $i -le 128 ]
do
  echo launching $i
  ./launch.sh ../../benchmarks/btor/btor$i.aig
  i=`expr 2 \* $i`
done
