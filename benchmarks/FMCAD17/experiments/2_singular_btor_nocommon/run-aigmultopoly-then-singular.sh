#!/bin/sh
script=/tmp/run-aigmultopoly-then-singular.singular
trap "rm -f $script" 2
aigmultopoly $* $script
exec time \
  -f '[time] %U usertime in seconds\n[time] %e wallclocktime in seconds\n[time] %M maximum resident set size (in KB)' \
  Singular $script
