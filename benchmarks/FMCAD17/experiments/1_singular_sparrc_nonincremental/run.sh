#!/bin/sh
i=32
while [ $i -le 64 ]
do
  echo launching $i
  ./launch.sh ../../benchmarks/aoki/sp-ar-rc-$i.aig
  i=`expr 2 \* $i`
done
