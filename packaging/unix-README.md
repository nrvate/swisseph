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

## Ephemeris data

Not included — it is hundreds of megabytes and has its own release cadence.
Without it the library falls back to the built-in Moshier ephemeris, which
needs no files and is what `swetest -emos` uses. For full precision, obtain
the `.se1` files and point the library at them with `swe_set_ephe_path()`.

## Verifying

```sh
sha256sum -c SHA256SUMS
```
