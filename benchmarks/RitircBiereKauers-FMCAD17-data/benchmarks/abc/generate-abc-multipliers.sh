#!/bin/sh
i=1
while [ $i -le 128 ]
do
  abc -c "gen -N $i -m abc$i.blif"
  abc -c "read abc$i.blif" -c strash                                                                 -c "write abc$i.aig"
  abc -c "read abc$i.blif" -c strash -c balance  -c "read mcnc.genlib"            -c "map" -c strash -c "write abc$i-mapped-complex.aig"
  abc -c "read abc$i.blif" -c strash -c balance  -c "read mcnc-no-complex.genlib" -c "map" -c strash -c "write abc$i-mapped-no-complex.aig"
  abc -c "read abc$i.blif" -c strash -c resyn    -c "read mcnc.genlib"            -c "map" -c strash -c "write abc$i-resyn-mapped-complex.aig"
  abc -c "read abc$i.blif" -c strash -c resyn    -c "read mcnc-no-complex.genlib" -c "map" -c strash -c "write abc$i-resyn-mapped-no-complex.aig"
  abc -c "read abc$i.blif" -c strash -c resyn3   -c "read mcnc.genlib"            -c "map" -c strash -c "write abc$i-resyn3-mapped-complex.aig"
  abc -c "read abc$i.blif" -c strash -c resyn3   -c "read mcnc-no-complex.genlib" -c "map" -c strash -c "write abc$i-resyn3-mapped-no-complex.aig"
  abc -c "read abc$i.blif" -c strash -c dc2      -c "read mcnc.genlib"            -c "map" -c strash -c "write abc$i-dc2-mapped-complex.aig"
  abc -c "read abc$i.blif" -c strash -c dc2      -c "read mcnc-no-complex.genlib" -c "map" -c strash -c "write abc$i-dc2-mapped-no-complex.aig"
  rm -f abc$i.blif
  if [ $i -lt 32 ]
  then
    i=`expr $i + 1`
  else
    i=`expr 2 \* $i`
  fi
done
