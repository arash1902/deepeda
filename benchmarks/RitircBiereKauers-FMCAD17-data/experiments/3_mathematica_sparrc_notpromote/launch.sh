#!/bin/sh
#SBATCH -c 32
#SBATCH -o /dev/null
name=`basename $1 .aig`
exec 1>"$name".log 2>"$name".err
echo "[launsh.sh] start `date`"
runlim \
  --real-time-limit=1200 \
  --space-limit=14000 \
  ./run-aigmultopoly-then-mathematica.sh $* --do-not-promote-gates
echo "[launsh.sh] end `date`"
killall -9 WolframKernel

