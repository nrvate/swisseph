# Swiss Ephemeris for Windows (thread-safe fork)

Built by CI from source. Nothing here is checked into the repository — see
`SHA256SUMS` for the manifest of this build.

This is a fork whose purpose is that the library behaves differently from
upstream under threads. `THREADING.md` explains what changed and how to use
it; the short version is that configuration set on one thread is now visible
to the others, and `swe_ctx_new()` gives you fully independent contexts.

## What is here

```
bin/x64/      swedll64.dll   the library
              swetest64.exe  command-line tool
bin/Win32/    swedll32.dll
              swetest.exe

lib/x64/      swedll64.lib   import library, links against the DLL
              swelib64.lib   static library, no DLL needed
lib/Win32/    swedll32.lib
              swelib32.lib

include/      swephexp.h     the public API — start here
              sweodef.h  swedate.h  swehouse.h  swedll.h

diagnostic/   swedlltrs*.dll  TRACE=1 build
              swedlltrm*.dll  TRACE=2 build
samples/      swete*.exe      swetest linked against the DLL
              swewin*.exe     GUI sample
```

## Which library do I link?

| | use |
|---|---|
| ship a DLL alongside your program | `swedll64.lib` + `swedll64.dll` |
| link everything into your binary | `swelib64.lib`, no DLL to ship |

Both give the same API. `swephexp.h` is the only header you need to include.

`swephlib.h` is deliberately **not** in this package. It declares internal
`swi_*` functions, and this fork's safety guarantees are about the public
`swe_*` API — reaching past it is how you get the behaviour the fork exists
to fix.

## `diagnostic/` — read before using

These are the same DLL built with the TRACE facility on. As they run they
write `swetrace.c`, a compilable C program replaying every API call made,
plus `swetrace.txt` with the results. That is genuinely useful for
reproducing a problem in isolation.

They are **not** drop-in replacements: they write files into the working
directory and are slower. `TRACE=1` uses fixed filenames; `TRACE=2` adds the
process id, so concurrent processes do not overwrite each other.

## `samples/`

`swete*.exe` is `swetest` linked against the DLL rather than statically — a
useful check that the DLL exports what a real program needs. It does not
support listing named asteroids, which needs a library-internal entry point
the DLL does not export.

`swewin*.exe` is a small GUI sample.

## Ephemeris data

Not included — it is hundreds of megabytes and has its own release cadence.
Obtain the `.se1` files and point the library at them with
`swe_set_ephe_path()`.

**This fork does not substitute one ephemeris for another.** Asking for Swiss
or JPL and getting an approximation instead is an error here, not a warning:
the call fails and names the ephemeris, the date and the fix. Upstream answers
such a request from the built-in Moshier model, says so in `serr`, and returns
success — so a caller that checks the return value alone cannot tell a data
file position from an analytic one. They agree on the Sun to about 0.02
arcsec; the Moon is out by ~2.9 arcsec, and Neptune passes an arcsecond after
2030.

Moshier remains available when you *ask* for it — `SEFLG_MOSEPH`, which is
what `swetest -emos` uses — and needs no data files. To restore the upstream
substituting behaviour, call `swe_set_ephe_fallback(1)` or set
`SE_EPHE_FALLBACK=1` in the environment.

## Verifying

```
certutil -hashfile bin\x64\swedll64.dll SHA256
```
and compare against `SHA256SUMS`.
