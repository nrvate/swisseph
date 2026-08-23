#!/usr/bin/env python3
"""Compare two golden transcripts numerically rather than byte-for-byte.

Bit-exact `diff` is the right gate WITHIN one build configuration, and that
is what `make check-golden` uses. It is the wrong gate ACROSS configurations:
gcc -O0 and gcc -O2 legitimately disagree in the last bits, because the
speed components are finite-differenced (evaluate at t +/- dt, subtract),
which amplifies rounding, and -O2 is free to contract into FMA or reassociate.

Measured on this tree, gcc -O0 vs -O2: 224 values across 65 rows differ,
median 2.2e-15 (pure ULP), max 7.6e-08 degrees = 0.00027 arcsec, all in
speed fields. Well under the library's own accuracy, but not zero.

So: use `diff` to prove a code change is a no-op at fixed flags, and use
this to prove a *build* change stays within tolerance.

Usage:  cmpgolden.py A.txt B.txt [--abs 1e-6] [--verbose]
Exit 0 if every value agrees within tolerance, 1 otherwise.
"""
import sys


def load(path):
    rows = {}
    for ln in open(path):
        f = ln.split()
        if not f:
            continue
        rows[f[0]] = [x for x in f[1:] if x.startswith(("0x", "-0x"))]
    return rows


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    opts = [a for a in sys.argv[1:] if a.startswith("--")]
    if len(args) != 2:
        print(__doc__)
        return 2
    tol = 1e-6
    for o in opts:
        if o.startswith("--abs="):
            tol = float(o.split("=", 1)[1])
    verbose = "--verbose" in opts

    a, b = load(args[0]), load(args[1])

    only_a = set(a) - set(b)
    only_b = set(b) - set(a)
    if only_a or only_b:
        print(f"FAIL: row sets differ ({len(only_a)} only in A, {len(only_b)} only in B)")
        for k in list(only_a)[:5]:
            print(f"  only in A: {k}")
        for k in list(only_b)[:5]:
            print(f"  only in B: {k}")
        return 1

    diffs = []
    shape = 0
    for k in a:
        if len(a[k]) != len(b[k]):
            shape += 1
            continue
        for i, (x, y) in enumerate(zip(a[k], b[k])):
            if x == y:
                continue
            fx, fy = float.fromhex(x), float.fromhex(y)
            diffs.append((abs(fx - fy), k, i, fx, fy))

    if shape:
        print(f"FAIL: {shape} rows have a different number of values")
        return 1

    if not diffs:
        print(f"PASS: transcripts are bit-identical ({len(a)} rows)")
        return 0

    diffs.sort(reverse=True)
    worst = diffs[0][0]
    med = diffs[len(diffs) // 2][0]
    over = [d for d in diffs if d[0] > tol]

    print(f"{len(diffs)} values differ across "
          f"{len(set(d[1] for d in diffs))} of {len(a)} rows")
    print(f"  max abs diff : {worst:.3e}   ({worst * 3600:.3e} arcsec if degrees)")
    print(f"  median       : {med:.3e}")
    print(f"  tolerance    : {tol:.3e}")

    if verbose or over:
        for ad, k, i, fx, fy in diffs[:8]:
            flag = "  <-- OVER" if ad > tol else ""
            print(f"    {k:26s} field[{i}]  {fx:+.12e} vs {fy:+.12e}  d={ad:.2e}{flag}")

    if over:
        print(f"FAIL: {len(over)} value(s) exceed tolerance {tol:.3e}")
        return 1
    print(f"PASS: all differences within {tol:.3e}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
