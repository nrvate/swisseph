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
Two quantities in this suite are numerically ill-conditioned: a change of a
couple of ULP in the INPUT moves the output by arcseconds. They are perfectly
reproducible for a fixed binary -- so they stay in the bit-exact baseline --
but they are NOT reproducible across math libraries, and are skipped here.

golden.c tags those rows itself with "~illcond"; this script skips any row
whose tag contains it. The tag travels in the transcript rather than living
as a coordinate list here, so it cannot rot when golden.c's loops change.

  SE_OSCU_APOG via Moshier -- the osculating lunar apogee. Measured with
  +2 ULP on tjd at 3000-01-01: longitude moves 2.86 arcsec. The same body
  via SWIEPH moves 0.000012 arcsec, SE_MEAN_APOG via Moshier 0.0000007.
  So it amplifies input noise by ~1e9 and is 238,000x worse than SWIEPH.
  Deriving apse direction from near-circular osculating elements is
  inherently unstable.

  fixed-star distance speed (field[5]) -- extracted by differencing a
  distance of ~1e7 AU to recover a quantity ~3e-10 of it. Polaris: distance
  2.74e+07 AU, distance speed -8.37e-03.

Measured, macOS/clang vs the gcc -O0 baseline: 17401 values differ across
3754 of 5127 rows, but the median is 5.7e-14 -- ULP noise from a different
libm -- and the only values exceeding 1e-6 belong to those two groups.

Skipping them does NOT stop a real regression being caught: the gcc -O0 job
compares them bit-exactly with diff, where they ARE reproducible. Pass
--no-skip to include them here anyway.

Usage:  cmpgolden.py A.txt B.txt [--abs 1e-6] [--verbose] [--no-skip]
Exit 0 if every value agrees within tolerance, 1 otherwise.
"""
import re
import sys

# A C99 %a literal. The number of mantissa digits it prints is up to the
# implementation: glibc emits the shortest form that round-trips, "0x1.9p+3",
# while MSVC always pads to 13, "0x1.9000000000000p+3". Both denote the same
# double. Values are parsed and compared as numbers below, so the spelling
# never mattered there -- but some rows carry a %a inside the message text,
# where it would otherwise be compared as a string and every such row would
# differ on Windows for no reason at all.
_HEXFLOAT = re.compile(r"[-+]?0[xX][0-9a-fA-F]*\.?[0-9a-fA-F]*[pP][-+]?\d+")


def _canon_hexfloats(s):
    def one(m):
        try:
            return float.fromhex(m.group(0)).hex()
        except ValueError:
            return m.group(0)
    return _HEXFLOAT.sub(one, s)


def load(path):
    rows = {}
    msgs = {}
    for ln in open(path):
        f = ln.split()
        if not f:
            continue
        # Canonicalised on the way in, so the "is it identical" shortcut below
        # compares values and not spellings. Without it MSVC reports every
        # padded literal as a difference of magnitude zero -- 22670 of them on
        # this transcript, which buries anything real.
        rows[f[0]] = [_canon_hexfloats(x)
                      for x in f[1:] if x.startswith(("0x", "-0x"))]
        # golden.c writes the error string after a " | ", already run through
        # its sanitize(): the ephemeris path is rewritten to $EPHE and the
        # control characters that would break one-row-per-line are mapped to
        # spaces. What is left is machine-independent -- the numbers inside a
        # message go through printf conversions whose output C specifies
        # exactly -- so it is compared verbatim rather than skipped.
        i = ln.find(" | ")
        msgs[f[0]] = _canon_hexfloats(ln[i + 3:].rstrip("\n")) if i >= 0 else ""
    return rows, msgs


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

    (a, amsg), (b, bmsg) = load(args[0]), load(args[1])

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
            # Rows that golden.c tagged as numerically ill-conditioned.
            # The tag lives in the transcript, not in a coordinate list here,
            # so it cannot drift when golden.c's loops or FLAGS[] change.
            if skip_illcond and "~illcond" in k:
                skipped += 1
                continue
            fx, fy = float.fromhex(x), float.fromhex(y)
            diffs.append((abs(fx - fy), k, i, fx, fy))

    if shape:
        print(f"FAIL: {shape} rows have a different number of values")
        return 1

    # Error strings, compared exactly. Before this, only the bit-exact gcc -O0
    # job looked at them, so a message could change on every other toolchain
    # unremarked -- and one did: "star  not found" quietly lost a space.
    badmsg = [k for k in a if amsg[k] != bmsg[k]]
    if badmsg:
        # Grouped by the (A, B) pair, not row by row: one cause typically
        # hits dozens of rows, and printing the first five of them says far
        # less than printing each distinct pair once with a count.
        groups = {}
        for k in badmsg:
            groups.setdefault((amsg[k], bmsg[k]), []).append(k)
        print(f"FAIL: {len(badmsg)} row(s) differ in their error string, "
              f"{len(groups)} distinct difference(s)")
        for (x, y), ks in sorted(groups.items(), key=lambda g: -len(g[1])):
            print(f"  x{len(ks)}, e.g. {ks[0]}")
            print(f"    A: {x[:160]}")
            print(f"    B: {y[:160]}")
        return 1

    if skipped:
        print(f"note: skipped {skipped} value(s) in rows golden.c tagged "
              f"~illcond -- not reproducible across libm, see header "
              f"(--no-skip to include)")
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
        # print ALL offenders when there are any -- the first CI run showed
        # only the top 8 and left us guessing whether the rest matched
        show = over if over else diffs[:8]
        for ad, k, i, fx, fy in (show if verbose else show[:8]):
            flag = "  <-- OVER" if ad > tol else ""
            print(f"    {k:26s} field[{i}]  {fx:+.12e} vs {fy:+.12e}  d={ad:.2e}{flag}")

    if over:
        print(f"FAIL: {len(over)} value(s) exceed tolerance {tol:.3e}")
        return 1
    print(f"PASS: all differences within {tol:.3e}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
