#!/bin/sh
i=8
case x`basename $0` in
  xsubmit.sh) run=sbatch;;
  *) run="";;
esac
echo "running jobs $run"
while [ $i -le 64 ]
do
  echo launching $i
  $run ./launch.sh ../../benchmarks/aoki/sp-ar-rc-$i.aig
  i=`expr 2 \* $i`
done
