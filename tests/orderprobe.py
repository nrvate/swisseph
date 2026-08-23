#!/usr/bin/env python3
"""Run every (prior, target) pair in its own process and report what moves.

A value that changes because of something computed BEFORE it is the signature
of leaked state. Prior 0 is "nothing", so it is the reference every other row
is compared against.
"""
import subprocess
import sys

BIN = "./orderprobe"

listing = subprocess.run([BIN, "--list"], capture_output=True, text=True).stdout.splitlines()
nprior, ntarget = map(int, listing[0].split())
priors = [l.split("\t", 1)[1] for l in listing[1:1 + nprior]]
targets = [l.split("\t", 1)[1] for l in listing[1 + nprior:1 + nprior + ntarget]]


def measure(p, t):
    r = subprocess.run([BIN, str(p), str(t)], capture_output=True, text=True)
    if r.returncode != 0 or not r.stdout.strip():
        return None
    return [float(v) for v in r.stdout.split()]


ref = {t: measure(0, t) for t in range(ntarget)}
findings = []

for t in range(ntarget):
    if ref[t] is None:
        print(f"  !! target {t} ({targets[t]}) produced nothing")
        continue
    for p in range(1, nprior):
        got = measure(p, t)
        if got is None:
            findings.append((targets[t], priors[p], "ERROR / no output"))
            continue
        for i, (a, b) in enumerate(zip(ref[t], got)):
            if a != b:
                # arcsec where the quantity is an angle; raw ratio otherwise
                delta = abs(a - b)
                unit = f"{delta * 3600:.4f}\"" if delta < 10 else f"{delta:.6g}"
                findings.append((targets[t], priors[p], f"field{i}: {a:.12g} -> {b:.12g}  ({unit})"))

print(f"  {nprior} priors x {ntarget} targets = {nprior * ntarget} processes\n")
if not findings:
    print("  no order dependence found")
else:
    seen = set()
    for tgt, pri, detail in findings:
        if tgt not in seen:
            print(f"  {tgt}")
            seen.add(tgt)
        print(f"      after {pri:<24} {detail}")
    print(f"\n  {len(findings)} order-dependent result(s)")
