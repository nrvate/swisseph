# Swiss Ephemeris (thread-safe fork)

Built by CI from source. Nothing here is checked into the repository — see
`SHA256SUMS` for the manifest of this build.

This is a fork whose purpose is that the library behaves differently from
upstream under threads. `THREADING.md` explains what changed and how to use
it; the short version is that configuration set on one thread is now visible
to the others, and `swe_ctx_new()` gives you fully independent contexts.

## What is here

```
bin/        swetest    command-line tool
            swevents   events calculator
            swemini    minimal example

lib/        libswe.a           static library
            libswe.so|.dylib   shared library

include/    swephexp.h         the public API — start here
            sweodef.h  swedate.h  swehouse.h  swedll.h
```

## Linking

```sh
cc myprog.c -I include -L lib -lswe -lm      # shared
cc myprog.c -I include lib/libswe.a -lm      # static
```

`swephexp.h` is the only header you need to include. On Linux add `-ldl` if
your toolchain does not pull it in, and `-lpthread` if you use threads.

`swephlib.h` is deliberately **not** in this package. It declares internal
`swi_*` functions, and this fork's guarantees are about the public `swe_*`
API — reaching past it is how you get the behaviour the fork exists to fix.

## Requirements

The Linux build is produced on Ubuntu 22.04 (glibc 2.35). The shipped binaries
reference symbol versions up to **`GLIBC_2.34`**, so they need a glibc at least
that new — Ubuntu 22.04+, Debian 12+, RHEL 9+. glibc is forward-compatible but
not backward-compatible, so a binary built on a newer distribution refuses to
start on an older one; the runner is pinned to 22.04 rather than `latest` for
exactly that reason. Build from source if you need to go further back.

You can check for yourself:

```sh
objdump -T lib/libswe.so | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1
```

The macOS build is a **universal binary** — both `arm64` and `x86_64` in every
library and tool, so it runs on Apple silicon and Intel alike:

```sh
lipo -archs lib/libswe.dylib     # x86_64 arm64
```

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

```sh
sha256sum -c SHA256SUMS
```
