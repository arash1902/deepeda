#!/bin/sh
case x`basename $0` in
  xsubmit.sh) run=sbatch;;
  *) run="";;
esac
echo "starting run $run" 

for i in \
  1_mathematica_btor_incremental \
  1_mathematica_btor_nonincremental \
  1_mathematica_btor_nonincremental_row-wise \
  1_mathematica_sparrc_incremental \
  1_mathematica_sparrc_nonincremental \
  1_mathematica_sparrc_nonincremental_rowwise \
  1_singular_btor_incremental \
  1_singular_btor_nonincremental \
  1_singular_btor_nonincremental_row-wise \
  1_singular_sparrc_incremental \
  1_singular_sparrc_nonincremental \
  1_singular_sparrc_nonincremental_row-wise \
  2_mathematica_btor_nocommon \
  2_mathematica_btor_novanishing \
  2_mathematica_btor_noxor \
  2_mathematica_sparrc_nocommon \
  2_mathematica_sparrc_novanishing \
  2_mathematica_sparrc_noxor \
  2_singular_btor_nocommon \
  2_singular_btor_novanishing \
  2_singular_btor_noxor \
  2_singular_sparrc_nocommon \
  2_singular_sparrc_novanishing \
  2_singular_sparrc_noxor \
  3_mathematica_sparrc_notmerge \
  3_mathematica_sparrc_notpromote \
  3_singular_sparrc_notmerge \
  3_singular_sparrc_notpromote \
  4_mathematica_abc \
  4_mathematica_abc_resyn3_complex \
  4_mathematica_abc_resyn3_nocomplex \
  4_singular_abc \
  4_singular_abc_resyn3_complex \
  4_singular_abc_resyn3_nocomplex

do
  echo "starting jobs $i" 
  cd $i ; sh run.sh ; cd ..
done

echo "finished $run" 

