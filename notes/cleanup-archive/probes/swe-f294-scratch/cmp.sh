#!/bin/bash
# compare the tip's swetest and the fixed one over options x formats x systems
T=/nvm/work/swe-f294-scratch/swetest_tip
N=/nvm/work/swe-f294/tests/hposswetest
E=/nvm/work/swe-f294/ephe
nrun=0; ndiff=0
for opt in "" "-sid1" "-j2000" "-nonut" "-hel" "-topo8.55,47.37,400" "-topo8.55,47.37,400 -at0,15" "-geopos8.55,47.37,1200" "-emos" "-true" "-speed3" "-i2050"; do
 for fmt in "PLBRS" "PlbrsSgj" "PlbRaAdD" "Phikl" "Pmz" "Pmzadhik" "PaSS" "PlbradxXSS" "PluUs" "PQ" "PlSS" "PlsS" "PlS"; do
  for hs in P G W; do
   for d in 1.1.1800 1.1.1950 1.1.2100; do
    a=$($T -edir$E -b$d -ut0 -p0145tf -xfSpica -house8.55,47.37,$hs $opt -f$fmt -head 2>&1)
    b=$($N -edir$E -b$d -ut0 -p0145tf -xfSpica -house8.55,47.37,$hs $opt -f$fmt -head 2>&1)
    nrun=$((nrun+1))
    if [ "$a" != "$b" ]; then ndiff=$((ndiff+1)); echo "DIFF opt=[$opt] fmt=$fmt hs=$hs d=$d: $(diff <(echo "$a") <(echo "$b") | grep -c '^>') lines"; fi
   done
  done
 done
done
echo "runs $nrun differ $ndiff"
