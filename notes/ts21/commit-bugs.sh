#!/bin/bash
# commit -> bugs: headings it adds to notes/UPSTREAM-BUGS.md, else entries whose text it changes, else numbers in its message (weak).
cd /nvm/work/swe-ts20
L=$(date -d '2026-10-25T05:39:19Z' +%s)
printf 'commit\ttime\tphase\tbugs\tsource\tsubject\n'
git log --reverse --format='%H %ct' origin/main..fd605d3 | while read c t; do
  ph=$([ $t -lt $L ] && echo pre-loop || echo loop)
  add=$(git show --format= -U0 $c -- notes/UPSTREAM-BUGS.md | grep -oE '^\+## [0-9]+\.' | grep -oE '[0-9]+' | sort -un | tr '\n' ' ')
  src=added
  if [ -z "$add" ]; then
    lines=$(git show --format= -U0 $c -- notes/UPSTREAM-BUGS.md | grep -oE '^@@ [^@]* \+[0-9]+' | grep -oE '[0-9]+$')
    if [ -n "$lines" ]; then
      add=$(git show $c:notes/UPSTREAM-BUGS.md | awk -v L="$lines" 'BEGIN{n=split(L,a," "); for(i=1;i<=n;i++) want[a[i]]=1} /^## [0-9]+\./{h=$2; sub(/\./,"",h)} (NR in want) && h!="" {print h}' | sort -un | tr '\n' ' ')
      src=changed
    fi
  fi
  if [ -z "$add" ]; then add=$(git log -1 --format=%B $c | grep -oE '(UPSTREAM-BUGS|[Bb]ugs?) [0-9]+([-, ]+[0-9]+)*' | grep -oE '[0-9]+' | sort -un | tr '\n' ' '); src=weak; fi
  [ -z "$add" ] && src=none
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' "${c:0:9}" "$(date -d @$t +%m-%d_%H:%M)" "$ph" "${add% }" "$src" "$(git log -1 --format=%s $c | cut -c1-120)"
done
