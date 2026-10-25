#!/bin/sh
# buildlib.sh SRC OUTDIR EXTRA_CFLAGS -> OUTDIR/lib.a
src=$1; out=$2; shift 2
mkdir -p $out
for f in sweconfig swedate swehouse swejpl swemmoon swemplan sweph swephlib swecl swehel; do
  echo "cc -O2 -g -fPIC -D_GNU_SOURCE -ffp-contract=off $* -I$src -c $src/$f.c -o $out/$f.o"
done | xargs -P2 -I{} sh -c '{}'
rm -f $out/lib.a; ar rc $out/lib.a $out/*.o
