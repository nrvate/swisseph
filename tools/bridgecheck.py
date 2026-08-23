#!/usr/bin/env python3
"""G12: no library-internal code may call a legacy (shim) entry point.

THE INVARIANT

Every public entry point that carries state exists twice: swe_X_r(ctx, ...)
does the work, and swe_X(...) is a shim that supplies the process-wide
default context.  Library-internal code must always call the _r form and
pass its own ctx.  A call to the bare swe_X() from inside the library
silently computes against the DEFAULT context regardless of which context
the caller was using -- so an explicit swe_ctx quietly produces the wrong
answer, with no error and no race for a sanitizer to find.

WHY THIS IS A SCRIPT AND NOT A CODE REVIEW

It was audited by hand twice.  The first audit reported "0 violations"
because its list of entry-point names was built from swephexp.h at a moment
when only 57 of the 78 _r variants had been declared; the other 21 were
invisible to it.  The second audit, run against the full list, found 12 real
violations -- including swe_orbit_max_min_true_distance(), which called
swe_get_orbital_elements() and so could never have worked with a
non-default context.

The failure mode of the manual check was that it silently under-reported.
This script derives the name list from the source itself, so it cannot go
stale the same way.

Exit status: 0 clean, 1 violations found.
"""
import re
import sys
import os

LIB = ['sweph.c', 'swephlib.c', 'swecl.c', 'swehel.c', 'swehouse.c',
       'swedate.c', 'swemplan.c', 'swemmoon.c', 'swejpl.c', 'sweconfig.c']


def mask(src):
    """Blank out string literals, char literals and comments, preserving
    offsets and line structure.

    Necessary, not defensive: the TRACE code writes the literal text
    'swe_calc(' into its replay file, and a plain regex would report it as
    a call.  Comments are checked before quotes because an apostrophe
    inside a comment ("Can't", "0.1'") would otherwise open a char literal
    and run to the end of the file.
    """
    out = list(src)
    i, n = 0, len(src)
    while i < n:
        if src.startswith("/*", i):
            j = src.find("*/", i)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif src.startswith("//", i):
            j = src.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif src[i] in '"\'':
            q = src[i]
            j = i + 1
            while j < n and src[j] != q:
                j += 2 if src[j] == "\\" else 1
            for k in range(i, min(j + 1, n)):
                if out[k] != "\n":
                    out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(root)

    # Derive the name list from the definitions themselves, so it cannot
    # lag behind newly added _r variants the way the hand audit did.
    names = set()
    for f in LIB:
        for m in re.finditer(r'^[A-Za-z_][\w \t\*]*\bCALL_CONV\s+(swe_[a-z_0-9]+)_r\s*\(',
                             open(f).read(), re.M):
            names.add(m.group(1))
    if not names:
        print("G12 FAIL: found no swe_*_r definitions at all -- "
              "the check is not looking at what it thinks it is")
        return 1

    bad = []
    for f in LIB:
        src = open(f).read()
        m = mask(src)
        for mo in re.finditer(r'\b(swe_[a-z_0-9]+)\s*\(', m):
            name = mo.group(1)
            if name not in names:
                continue
            ls = src.rfind("\n", 0, mo.start()) + 1
            le = src.find("\n", mo.start())
            line = src[ls:le if le >= 0 else len(src)]
            if "CALL_CONV" in line:
                continue                       # the definition or the shim's signature
            if "swi_default_ctx()" in src[mo.end():mo.end() + 30]:
                continue                       # the shim body itself: this is its job
            bad.append((f, src[:mo.start()].count("\n") + 1, name, line.strip()[:64]))

    print("G12: internal calls to legacy entry points (%d _r names checked)" % len(names))
    if not bad:
        print("PASS")
        return 0
    for f, ln, name, line in bad:
        print("  %s:%d  calls %s() -- should be %s_r(ctx, ...)" % (f, ln, name, name))
        print("      %s" % line)
    print("G12 FAIL: %d call(s) would use the default context "
          "regardless of the caller's" % len(bad))
    return 1


if __name__ == "__main__":
    sys.exit(main())
