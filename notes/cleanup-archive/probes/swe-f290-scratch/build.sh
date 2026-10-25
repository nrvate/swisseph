#!/bin/bash
# build.sh DIR [extra flags]: compile the worktree library into DIR/libswe.a
set -e
d=$1; shift
W=/nvm/work/swe-f290
for f in sweconfig swedate swehouse swejpl swemmoon swemplan sweph swephlib swecl swehel; do
  if [ ! -f $d/$f.o ] || [ $W/$f.c -nt $d/$f.o ] || [ "$FORCE" = 1 ]; then echo $f; gcc -O1 -g -D_GNU_SOURCE -ffp-contract=off -I$W "$@" -c $W/$f.c -o $d/$f.o; fi
done
ar rcs $d/libswe.a $d/*.o
