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

KNOWN CROSS-PLATFORM DIVERGENCE
-------------------------------
One quantity is not reproducible across math libraries and is excluded from
tolerance checking by default: field[5] of the fixed-star rows, which is the
star's distance SPEED.

It is not a bug and not a regression -- it is inherent conditioning. For
Polaris the distance is 2.74e+07 AU and the distance speed is -8.37e-03, i.e.
3.1e-10 of the value it is differenced out of. Double precision carries about
2.2e-16 relative, so any difference in the underlying trig/sqrt chain between
glibc and Apple's libm is amplified by ten orders of magnitude on its way into
this field.

Measured, macOS/clang vs the gcc -O0 baseline: 17401 values differ across 3754
of 5127 rows, but the median difference is 5.7e-14 (ULP noise) and only 115
exceed 1e-6 -- every one of them fixstar field[5], up to 1.3e-01 on the
Galactic Centre, whose distance is larger still.

Excluding it here does NOT stop a real regression in that field being caught:
the gcc -O0 job compares bit-exactly with diff, where the value IS
reproducible. Pass --no-skip to check it anyway.

Usage:  cmpgolden.py A.txt B.txt [--abs 1e-6] [--verbose] [--no-skip]
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
    skip_illcond = "--no-skip" not in opts

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
    skipped = 0
    for k in a:
        if len(a[k]) != len(b[k]):
            shape += 1
            continue
        for i, (x, y) in enumerate(zip(a[k], b[k])):
            if x == y:
                continue
            # fixstar distance speed: ill-conditioned, see the header
            if skip_illcond and i == 5 and k.startswith("star["):
                skipped += 1
                continue
            fx, fy = float.fromhex(x), float.fromhex(y)
            diffs.append((abs(fx - fy), k, i, fx, fy))

    if shape:
        print(f"FAIL: {shape} rows have a different number of values")
        return 1

    if skipped:
        print(f"note: skipped {skipped} fixstar distance-speed value(s) "
              f"-- ill-conditioned across libm, see header (--no-skip to include)")
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
