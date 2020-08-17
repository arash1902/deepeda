#!/bin/sh
i=2
case x`basename $0` in
  xsubmit.sh) run=sbatch;;
  *) run="";;
esac
echo "running jobs $run"
while [ $i -le 128 ]
do
  echo launching $i
  $run ./launch.sh ../../benchmarks/btor/btor$i.aig
  i=`expr 2 \* $i`
done
