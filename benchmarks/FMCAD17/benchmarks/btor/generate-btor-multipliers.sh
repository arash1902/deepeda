#!/bin/sh
n=1
while [ $n -le 128 ]
do
  m=`expr 2 \* $n`
  btor=btor${n}.btor
  rm -f $btor
  if [ $n = 1 ]
  then
    echo "1 var 1 a[0]" >> $btor
    echo "2 var 1 b[0]" >> $btor
  else
    echo "1 var $n a" >> $btor
    echo "2 var $n b" >> $btor
  fi
  echo "3 uext $m 1 $n" >> $btor
  echo "4 uext $m 2 $n" >> $btor
  echo "5 mul $m 3 4" >> $btor
  id=6
  i=0
  while [ $i -lt $m ]
  do
    slice=$id
    echo "$id slice 1 5 $i $i" >> $btor
    id=`expr $id + 1`
    echo "$id root 1 $slice" >> $btor
    id=`expr $id + 2`
    i=`expr $i + 1`
  done
  aag=btor${n}.aag
  boolector $btor -rwl=1 -daa > $aag
  i=0
  while [ $i -lt $m ]
  do
    echo "o$i s[$i]" >> $aag
    i=`expr $i + 1`
  done
  aig=btor${n}.aig
  aigtoaig $aag $aig
  rm -f $aag
  if [ $n -lt 32 ]
  then
    n=`expr $n + 1`
  else
    n=`expr 2 \* $n`
  fi
done
