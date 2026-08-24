# Using Swiss Ephemeris from multiple threads

This fork makes libswe usable from more than one thread. Upstream is not — for
a reason that is easy to miss, because it does not look like a crash.

Working notes on how this was investigated and built live in [`notes/`](notes/).
This document is for people who want to *use* the library.

---

## The problem, in one paragraph

Upstream is not "not thread safe" in the usual sense. Since v2.03 it annotates
the global `swed` state with `TLS` (`__thread`), which on Linux/GCC genuinely
eliminates data races. The result is a library that is **race-free but not
thread-usable**: every thread gets its own private copy of the configuration,
so `swe_set_ephe_path()` called on your main thread is invisible to every
worker. Workers silently fall back to the built-in Moshier ephemeris — lower
precision, different answers — and still report success. Nothing errors.
Nothing warns. The numbers are just quietly wrong.

On platforms where `TLS` expanded to nothing, the failure inverted into real
data races on 23 KB of shared caches and open `FILE *`s.

---

## Two ways to use this fork

### 1. The process-wide API — unchanged, now actually works

Every existing function keeps its name, its signature and its ABI. If your code
compiles against upstream, it compiles against this fork.

```c
swe_set_ephe_path("/usr/share/ephe");   /* on any thread */
swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);

/* ... worker threads ... */
swe_calc_ut(tjd, SE_MOON, iflag, xx, serr);   /* sees both settings */
```

Configuration is now published to a shared master and adopted by every thread
that has not overridden it itself. A thread that calls a setter takes ownership
of that setting and stops tracking the global one — so a worker can override
the observer position without disturbing anyone else, and without losing the
ephemeris path the main thread configured.

**This is the right choice if** every thread should share one configuration.

### 2. Explicit contexts — independent configurations

```c
swe_ctx *ctx = swe_ctx_new();          /* NULL on allocation failure */

swe_set_ephe_path_r(ctx, "/usr/share/ephe");
swe_set_sid_mode_r(ctx, SE_SIDM_LAHIRI, 0, 0);
swe_calc_ut_r(ctx, tjd, SE_MOON, iflag, xx, serr);

swe_ctx_free(ctx);
```

Every entry point that carries state has an `_r` variant taking a context
first. `swe_X(...)` is now exactly `swe_X_r(swi_default_ctx(), ...)`.

The 28 entry points that are already pure — `swe_degnorm`, `swe_julday`,
`swe_cotrans` and friends — have no `_r` form, because a context argument
would be noise.

**This is the right choice if** you need two configurations at once: a server
answering one request tropical and another sidereal, or two different observer
positions, concurrently. The process-wide API cannot express that at all.

`swe_ctx_new()` inherits whatever has been published through the process-wide
setters, so it behaves as you would expect after `swe_set_ephe_path()`. From
then on it is independent: configuring it does not move any other context, and
later process-wide changes do not move it.

---

## The threading contract

> **A `swe_ctx` may be used by one thread at a time.** Concurrent calls on the
> **same** context are undefined. Different contexts are fully independent and
> take no locks between them.

This is the `FILE *` / `sqlite3 *` contract. Passing a context between threads
is fine with external synchronisation; using it from two at once is not.

The process-wide API follows the same rule with a per-thread default context,
which is why it works without you doing anything.

---

## Releasing resources: `swe_close()` vs `swe_close_r()`

These differ, and the difference matters.

| | releases the context's files and caches | resets the process-wide configuration |
|---|---|---|
| `swe_close_r(ctx)` | yes | **no** |
| `swe_close()` | yes | yes |

Each thread has its own default context, and its ephemeris segments are freed
only by a close **on that thread** — about 9 KB per worker. So:

```c
/* a worker thread, finished with the library */
swe_close_r(swi_default_ctx());

/* the whole process, shutting the library down */
swe_close();
```

Calling `swe_close()` from a worker would also wipe the configuration every
other thread is reading.

`swe_ctx_free()` releases and frees an explicit context. It tolerates `NULL`,
and refuses the process-wide default context rather than freeing memory it does
not own.

---

## Building

```
make                # -std=c17 -Wall -Wextra -Werror -O2
make LTO=1          # + link-time optimisation, see below
make -C tests check-golden   # bit-exact regression gate
make -C tests check-ci       # every gate, under CI's exact toolchain (needs docker)
```

`check-ci` exists because a development machine is usually not the CI
machine. It runs the whole gate set inside `ubuntu:24.04`, which carries the
same gcc the runner reports. Two bugs in this work passed every local gate
and failed CI — an uninitialised read only newer gcc diagnoses, and a
gcc-only `__attribute__` that MSVC rejects — so if you are about to push,
run this rather than trusting a green local run.

- **`LTO=1`** is opt-in. Measured at about **5% faster** on the Moshier Moon
  path and bit-identical to plain `-O2` across the whole golden transcript on
  gcc 11.4 (5137 rows as the transcript then stood; it is 12582 today).
  It is not the default because parity on clang/macOS/MSVC has not been
  confirmed — the CI `lto` job exists to close that.
- **`-DSWE_NO_THREADS`** compiles the threading primitives to no-ops for
  single-threaded or embedded builds. No pthread dependency at all.
- The threading shim selects one of five backends automatically (Windows
  SRWLOCK, gcc/clang `__atomic`, C11 `<stdatomic.h>`, or a mutex fallback for
  toolchains with none of those). All of them are built and checked by
  `make -C tests check-threadtiers`.

---

## What is verified

Every change is gated on a bit-exact transcript: **12582 rows** of C99 `%a` hex
floats, compared byte for byte, so no test ever has to pick a tolerance.

The transcript is not a handful of spot checks. It sweeps 120 pseudo-random
dates spanning roughly 1400 years (JD 2086302.5 to 2597641.5) across three
ephemeris flag sets — Swiss, Moshier and equatorial — for every body from the
Sun to Vesta, recording ecliptic longitude and latitude, distance, and all
three speed components. A further 64 rows exercise the entry points the sweep
does not reach, so no exported function is entirely unwitnessed.

| Gate | What it proves |
|---|---|
| `check-golden` | output is bit-identical to the reference build |
| `check-threads` | 8 worker threads agree with the main thread, row for row |
| `check-ctx` | two contexts really are independent, concurrently, and neither leaks into the other |
| `check-cfgleak` | no internal call publishes a private setting globally |
| `check-sunrace`, `check-glprace` | specific races, before and after |
| `check-jplguard` | malformed JPL headers are rejected, not read |
| `check-jplcalc` | the JPL reader computes correctly |
| `check-threadtiers` | all threading backends build and agree |
| `check-bridge` | no internal call silently uses the default context |
| `check-build` | the sample programs and the `TRACE` build still compile |
| `check-winmacros` | nothing collides with `windows.h`'s empty annotation macros, and the `_WIN32` branches parse |
| `check-version` | `SE_VERSION` is the only place the version is written down |
| `check-jplreal` | `SEFLG_JPLEPH` reaches a real JPL ephemeris, and a missing one is refused rather than substituted |

The bit-exact transcript also carries `cov:order_*`, which pin something
easily lost: **the same call must give the same answer regardless of what was
calculated before it.** Three leaks broke that — a Moshier calculation, a JPL
calculation, or merely naming a JPL file each moved a later result by 4.56″ to
56″, because the DE number that selects the tidal acceleration was discarded
when files closed, or read from a file that was never in use. Each row fails
if its fix is reverted.

CI runs gcc, clang, macOS, MSVC, ThreadSanitizer, AddressSanitizer,
LeakSanitizer, four C dialects, an ABI check, an LTO build, and a differential
run of upstream's own `setest` suite. MSVC's output is compared numerically
against the gcc reference across the whole transcript, within a measured
tolerance rather than by diff — a bit-exact comparison across toolchains would
fail for reasons unrelated to this code.

Four packaging jobs then build the shipped artifacts for Linux, macOS, Windows
and Android on every push, so a release cannot depend on a recipe that has not
already run.

---

## Compatibility

The public ABI is **additive only**: 106 exported symbols before this work,
186 after, none removed or changed. Existing binaries keep working; existing
source keeps compiling.
