#!/bin/sh
#--time-limit=3600 --space-limit=7000 -o m4.err 
exec \
runlim \
--real-time-limit=3600 \
--space-limit=7000 \
-o empty.err \
/usr/local/bin/wolfram -script empty.wl | \
tee empty.log


