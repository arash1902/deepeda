#!/bin/sh
i=4
case x`basename $0` in
  xsubmit.sh) run=sbatch;;
  *) run="";;
esac
echo "running jobs $run"
while [ $i -le 16 ]
do
  echo launching $i
  $run ./launch.sh ../../benchmarks/abc/abc$i-resyn3-mapped-no-complex.aig
  i=`expr 2 \* $i`
done
