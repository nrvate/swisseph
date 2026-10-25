#!/usr/bin/env python3
"""G8's comparison: the upstream setest testcases this fork answers differently.

setest runs upstream's own suite and prints one block per testcase that
misses its recorded expectation ("# setest -s 1.1.178 ..." and the values
under it). Run on upstream's library and on this fork's, the two outputs
differ exactly where the fork's fixes change an upstream answer.

Through ts.19 every such fix sat behind SWE_UPSTREAM_COMPAT, so G8 built
the fork in compat mode and required the two outputs to be identical. From
ts.20 on, fixes carry no switch, and the fork differs from upstream on
purpose. setest-upstream.baseline records where: one line per testcase
whose block differs, with the UPSTREAM-BUGS entries that move it. G8 passes
when the testcases that differ are exactly the recorded ones, so a new
difference fails, and so does a recorded one that disappears.

The record lists testcases, not values. The values are printed to 20
digits, and upstream's and the fork's both pass through the C library's
sin, cos and atan2, which differ in the last bit between glibc releases:
a record of values made on one machine would fail on another. Whether a
testcase differs does not depend on that. The fork's own values are held
bit-exactly by G1.

  setestcases.py check  RECORD UPSTREAM FORK   exit 1 on any mismatch
  setestcases.py record RECORD UPSTREAM FORK   print a new record: the
      lines of RECORD that still differ, and '?' for a new testcase, whose
      entry is to be written in by whoever makes the change
"""

import re
import sys

HEADER = re.compile(r"# setest -s (\S+) ")
# the run's closing count of failures, which every difference moves
SUMMARY = re.compile(r"Testmode '\w+': Total ")


def blocks(path):
    """Each testcase's block of lines, by its id."""
    out, cur = {}, None
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = HEADER.match(line)
            if m:
                cur = m.group(1)
                out[cur] = []
            elif SUMMARY.match(line):
                cur = None
            elif cur is not None:
                out[cur].append(line)
    return out


def key(case):
    """1.1.178 before 1.1.1000: numerically by each field."""
    return [int(p) if p.isdigit() else p for p in case.split(".")]


def differing(upstream, fork):
    a, b = blocks(upstream), blocks(fork)
    return sorted((k for k in set(a) | set(b) if a.get(k) != b.get(k)), key=key)


def read_record(path):
    """The record's comment lines, and its testcases with their entries."""
    comments, cases = [], {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                comments.append(line)
                continue
            case, _, bugs = line.strip().partition(" ")
            cases[case] = bugs.strip()
    return comments, cases


def main(argv):
    if len(argv) != 5 or argv[1] not in ("check", "record"):
        sys.stderr.write(__doc__)
        return 2
    mode, record, upstream, fork = argv[1:]
    comments, cases = read_record(record)
    now = differing(upstream, fork)
    if mode == "record":
        sys.stdout.writelines(comments)
        for case in now:
            print(f"{case} {cases.get(case) or '?'}")
        return 0
    new = [c for c in now if c not in cases]
    gone = sorted(set(cases) - set(now), key=key)
    unexplained = [c for c in now if cases.get(c) in (None, "", "?")]
    if new:
        print(f"  {len(new)} testcases differ from upstream and are not recorded:")
        for c in new[:20]:
            print(f"    {c}")
    if gone:
        print(f"  {len(gone)} recorded testcases no longer differ from upstream:")
        for c in gone[:20]:
            print(f"    {c} ({cases[c]})")
    unexplained = [c for c in unexplained if c not in new]
    if unexplained:
        print(f"  {len(unexplained)} recorded testcases name no UPSTREAM-BUGS entry:")
        for c in unexplained[:20]:
            print(f"    {c}")
    if new or gone or unexplained:
        return 1
    print(f"  {len(now)} testcases differ from upstream, each one recorded")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
