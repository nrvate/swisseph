# Swiss Ephemeris — Thread Safety Investigation

**Status:** Phase 1 — root-cause analysis (in progress)
**Repo:** `swisseph` @ `3fd0f95` (branch `threadsafe`)
**Started:** 2026-08-22

---

## 0. Executive summary

The common claim "Swiss Ephemeris is not thread safe" is **imprecise**, and the
imprecision matters for how we fix it.

Since v2.03 (Oct 2015) upstream *did* attempt thread safety, using a single
blunt instrument: annotate the global state `struct swe_data swed` — and most
function-level `static` scratch variables — with a `TLS` macro that expands to
`__thread`. On Linux/GCC this is **effective at eliminating data races**.

So the accurate statement is:

> On Linux/glibc/GCC, libswe is **race-free but not thread-usable**.
> On macOS it is **neither** — `TLS` silently compiles to nothing.

Three distinct problem classes, in descending order of practical impact:

| # | Class | Severity | Affects |
|---|---|---|---|
| **A** | Thread-local state makes the library **semantically wrong** under threads: configuration set on one thread is invisible to every other, and the library *silently falls back to lower-precision ephemerides* instead of erroring | **Critical** | All platforms, incl. Linux |
| **B** | `TLS` expands to nothing on `__APPLE__` / `WIN32` / `DOS32` → real data races on `swed` (23 KB of shared mutable state) | **Critical** | macOS, some Windows builds |
| **C** | Per-thread state is never reclaimed at thread exit → **fd and heap leaks**; and per-thread duplication of file handles/caches | **High** | All platforms |
| **D** | A handful of genuinely non-TLS mutable statics | Low–Medium | All platforms |

**Class A is the headline finding, and it is almost certainly the bug behind the
pyswisseph threading reports.** It is worse than a crash: it silently returns
subtly wrong numbers.

---

## 1. How state is organised today

### 1.1 The central blob

`sweph.h:791` defines `struct swe_data`, and `sweph.h:849` declares:

```c
extern TLS struct swe_data swed;
```

Measured size on x86-64: **23,032 bytes**.

```
$ readelf -sW sweph.o | grep -w swed
73: 0000000000000000 23032 TLS  GLOBAL DEFAULT  7 swed
```

It holds essentially every piece of library configuration and cache:

- **Configuration** — `ephepath`, `jplfnam`, `topd` (topocentric observer),
  `sidd` (sidereal mode), `tid_acc`, `delta_t_userdef`, `astro_models`,
  `do_interpolate_nut`
- **Open resources** — `fixfp` (fixed-star `FILE*`), `fidat[]`
  (`SEI_NEPHFILES` × `struct file_data`, each with its own `FILE*`)
- **Caches** — `pldat[]`, `nddat[]`, `savedat[]` (last position per planet),
  `oec`, `oec2000`, `nut`, `nut2000`, `nutv`, `interpol`
- **Heap owners** — `dpsi`, `deps` (36,525 doubles each ≈ 285 KB apiece when
  allocated), `fixed_stars`
- **Init flags** — `swed_is_initialised`, `ephe_path_is_set`, `geopos_is_set`,
  `ayana_is_set`, `jpl_file_is_open`

### 1.2 The `TLS` macro

`sweodef.h:81-95`:

```c
#if !defined(TLSOFF) && !defined( __APPLE__ ) && !defined(WIN32) && !defined(DOS32)
#if defined( __GNUC__ ) || defined( __CYGWIN__ )
#define TLS     __thread
#else
#define TLS     __declspec(thread)
#endif
#else
#define TLS
#endif
```

Verified expansion (`cc -E -I. probe.c`):

| Build | `TLS` expands to | Thread-local? |
|---|---|---|
| Linux / GCC (native, this box) | `__thread` | **yes** |
| `-D__APPLE__` (macOS) | *(empty)* | **NO** |
| `-DWIN32` | *(empty)* | **NO** |
| `-DTLSOFF` | *(empty)* | **NO** |
| `-DDOS32` | *(empty)* | **NO** |
| MSVC without bare `WIN32` | `__declspec(thread)` | yes |

Confirmed on the real symbol — `swed` lands in `.tbss` (ELF type `TLS`) on this
Linux build, so **TLS is on by default here**.

### 1.3 Scope of the library

The Makefile builds only these into `libswe`:

```
swedate.o swehouse.o swejpl.o swemmoon.o swemplan.o sweph.o swephlib.o swecl.o swehel.o
```

`swevents.c`, `sweephe4.c`, `swephgen4.c`, `swetest.c` are **applications/tools,
not library code**. They contain many non-TLS statics, but those are out of
scope for a thread-safe *library*. (Noted so we don't waste effort there.)

---

## 2. Class A — thread-local state is semantically wrong  ⚠️ **the real bug**

### 2.1 The mechanism

Every `swe_set_*()` function writes to `swed`. Because `swed` is `__thread`,
**each thread gets a fresh copy initialised from the `.tdata` template** — i.e.
the compile-time defaults in `sweph.c:96`, *not* whatever the application
configured.

This means the universal usage pattern:

```c
swe_set_ephe_path("/path/to/ephe");   /* once, at startup */
/* ... then compute from a thread pool ... */
```

**does not work.** Worker threads see the default path, not the configured one.

### 2.2 Reproduction — silent precision downgrade

`scratchpad/tsan/mt2.c` — configure on main, compute the Moon on a worker:

```
main     retflag=258 SWIEPH   lon=223.3237514464  serr=[]
worker   retflag=260 MOSHIER! lon=223.3237754384  serr=[SwissEph file 'sepl_18.se1'
                                 not found in PATH '.:/users/ephe2/:/users/ephe/'
                                 using Moshier eph.; ]

delta lon = -0.0000239921 deg  (-0.086 arcsec)
```

The worker thread:
- could not find the ephemeris files (it fell back to the **hardcoded default
  path** `.:/users/ephe2/:/users/ephe/`),
- **silently downgraded to the Moshier analytic ephemeris**,
- returned a **different answer** (0.086″ off for the Moon here; the divergence
  grows to arcminutes far from J2000),
- and returned **success**. The only signal is a changed bit in `retflag` and a
  warning in `serr`, both of which virtually every binding ignores.

### 2.3 Reproduction — every setter is affected

`scratchpad/tsan/mt3.c` — set sidereal mode, topocentric position and tidal
acceleration on main, then read them from a worker:

```
main    ayanamsa=23.857092  topo_moon_lon=222.988064  tidacc=-25.8500  serr=[]
worker  ayanamsa=24.740300  topo_moon_lon=  0.000000  tidacc=-25.8000  serr=[geographic position has not been set]
```

- **Ayanamsa off by 0.883°** — the worker silently used the default
  Fagan/Bradley instead of the configured Lahiri. For sidereal astrology this is
  a catastrophic, completely plausible-looking error.
- **Topocentric position returned `0.000000`** with a hard error.
- **`tid_acc` reverted** to its default.

### 2.4 Affected public API

Every one of these configures thread-local state and therefore must be re-run
on **every** thread:

| Function | `swephexp.h` |
|---|---|
| `swe_set_ephe_path()` | 741 |
| `swe_set_jpl_file()` | 744 |
| `swe_set_topo()` | 750 |
| `swe_set_sid_mode()` | 753 |
| `swe_set_lapse_rate()` | 891 |
| `swe_set_interpolate_nut()` | 958 |
| `swe_set_tid_acc()` | 966 |
| `swe_set_delta_t_userdef()` | 970 |
| `swe_set_astro_models()` | 686 |
| `swe_close()` | 738 |

### 2.5 Why this explains the pyswisseph reports

In pyswisseph the idiom is to call `swe.set_ephe_path(...)` once at import time,
on the main interpreter thread. Any computation that later happens on a
different thread — a `ThreadPoolExecutor`, a WSGI/ASGI worker, a Celery thread,
a GUI background thread — lands on a *different* `swed` with default settings.

Symptoms that follow directly and match what users report:
- results that differ between "run it directly" and "run it in the pool"
- intermittent wrong answers depending on which thread served the request
- sidereal charts that are wrong by ~1°
- topocentric calls failing with "geographic position has not been set"

Nothing crashes and no race detector fires, which is exactly why it has been so
hard to pin down.

> **CONFIRMED against pyswisseph 2.10.3.2 source — see §9.**

---

## 3. Class B — platforms where TLS silently vanishes

On `__APPLE__`, `WIN32`, `DOS32`, or `-DTLSOFF`, `TLS` expands to nothing and
`swed` becomes an ordinary process-wide global. Then **all 23 KB of it is
unsynchronised shared mutable state**, written on essentially every API call
(caches in `savedat[]`/`pldat[]` are updated by every `swe_calc()`).

That is a textbook data race: torn reads of `double`s, corrupted `FILE*`s,
use-after-free on `dpsi`/`deps`/`fixed_stars`, and interleaved cache updates
producing positions belonging to the wrong planet or date.

Notes:

- The `__APPLE__` exclusion is **obsolete**. Apple clang has supported `__thread`
  since Xcode 8 / macOS 10.7. There is no longer a reason to disable it.
- The Windows guard tests bare **`WIN32`**, but MSVC only defines **`_WIN32`**.
  Whether a Windows build is thread-local therefore depends on whether the
  consuming project happens to define `WIN32` — historically common in Visual
  Studio project templates. **Thread safety silently depends on a preprocessor
  symbol outside the library's control.**
- Related typo, `sweph.c:261`: `#if !defined(__APPLE)` — should be `__APPLE__`.
  Harmless today but shows this area is under-tested.

---

## 4. Class C — lifecycle: per-thread state is never reclaimed

`swe_close()` (`sweph.c:1233`) frees only the **calling thread's** `swed`. There
is no `pthread_key_t` destructor, no `__attribute__((destructor))` per thread,
no atexit-per-thread hook. A thread that exits without calling `swe_close()`
leaks everything it opened.

### 4.1 Measured fd leak (`scratchpad/tsan/fd.c`)

Each worker touched the main planets plus two asteroids, then exited **without**
`swe_close()`:

```
threads= 1 -> open fds=  7   after join: 7   <-- leaked
threads= 8 -> open fds= 28   after join: 28  <-- leaked
threads=32 -> open fds=100   after join: 100 <-- leaked
```

**~3 fds per thread, leaked permanently.** In a long-lived server whose pool
recycles threads, this walks straight into `EMFILE`.

Note this is the *floor*. `swed.fidat[]` is sized `SEI_NEPHFILES`, and each
distinct asteroid file is a separate `FILE*` — the user's original point about
"hundreds of open file handles per thread" is real, it just needs asteroid
breadth to show up.

### 4.2 Measured memory cost (`scratchpad/tsan/mem.c`)

```
threads= 1  delta= 992KB
threads= 4  delta=1188KB  (~297KB/thread)
threads=16  delta=2080KB  (~130KB/thread)
threads=32  delta=3324KB  (~103KB/thread)
```

~100 KB/thread on this path — modest, because `dpsi`/`deps` (≈570 KB combined)
are only allocated when the EOP/nutation file is loaded. Workloads that use
`swe_set_interpolate_nut()` or high-precision nutation will pay that per thread.

### 4.3 Cache duplication

Because `pldat[]`/`savedat[]`/`fidat[]` are per-thread, **N threads read and
decode the same ephemeris segments N times**. The design trades correctness-by-
isolation for linear waste in I/O, page cache pressure, and Chebyshev
evaluation. A shared, properly-locked (or immutable/refcounted) cache would be
strictly better on both axes.

---

## 5. Class D — genuinely non-TLS mutable statics in library code

Found by scanning ELF symbol tables of the 9 library objects for `OBJECT`
symbols **not** in `.tbss`/`.tdata`/`.rodata`:

| File | Symbol | Verdict |
|---|---|---|
| `swehouse.c:636` | `static double saved_sundec = 99;` | **REAL RACE.** Read-modify-written inside `swe_houses_armc_ex2()` for Sunshine houses (`hsys=='I'`). Not TLS. Two threads computing Sunshine houses can hand each other the wrong solar declination — a silently wrong chart, not a crash. |
| `sweph.c:252` | `static Dl_info dli;` | **REAL RACE**, benign-ish. Written by `dladdr()` in `swe_get_library_path()`. Concurrent calls can interleave and yield a torn/mismatched path. Trivially fixed by making it a local. |
| `swehel.c` | `a.0`, `a.1`, `a.2` | **Harmless.** All three live inside `if ((0)) { ... }` dead debug blocks (`swehel.c:834, 1290, 1393`). Should be deleted for hygiene. |
| `swemplan.c` | `mertabl`, `ventabl`, … `plutabl`, `*args`, `*404`, `planets`, `plan_fict_nam` | **Not races** — read-only data that merely lacks `const`, so it sits in `.data` instead of `.rodata`. Should be `const` (also saves ~90 KB of dirty pages per process and enables page sharing). |
| `swephlib.c` | `dcor_eps_jpl`, `dcor_ra_jpl`, `dtcf16` | Same — missing `const`. Verify `dtcf16` is never written before constifying. |
| `swecl.c` | `saros_data_solar`, `saros_data_lunar` | Same — missing `const`. |
| `sweph.c` | `ayanamsa_name` | Same — missing `const`. |

Also noted (TLS, so not a race today, but they become one the moment TLS is
removed in favour of a context handle — **these must all move into the context**):

- `swephlib.c:87-89` — `swi_fp_trace_c`, `swi_fp_trace_out`, `swi_trace_count`.
  Even with TLS these are hazardous: every thread writes to the **same trace
  file path**, so the trace output interleaves and corrupts.
- `swephlib.c:2431` — `dt[TABSIZ_SPACE]` (Delta-T table, extended at runtime)
- `swephlib.c:3743` — `crc32_table[256]` (lazily initialised)
- `swedate.c:87,276` — `init_leapseconds_done`, `leap_seconds[]`
- `swejpl.c:113` — `js` (JPL state), plus Chebyshev scratch at `476-479`, `665-666`
- `swemplan.c:129-130` — `ss[9][24]`, `cc[9][24]`
- `swemmoon.c:811-843` — ~25 file-scope scratch doubles (`SWELP`, `M`, `MP`,
  `D`, `NF`, `T`…) used as implicit parameter passing between static functions
- `sweph.c` — memo caches at `337-339`, `6036`, `6415`, `6825-6826`,
  `6915-6916`, `7622`, `7901-7902`, `7993-7994`
- `swehel.c` — memo caches at `343-345`, `383-384`, `557-558`, `818`, `888`, `1040`, `1134-1136`

The `swemmoon.c` cluster is architecturally the nastiest: those globals are
**used as implicit arguments** across function boundaries, so they can't simply
be moved into a context struct without threading a parameter through a lot of
call sites.

---

## 6. Evidence artefacts

All under `scratchpad/tsan/`:

| File | Demonstrates |
|---|---|
| `mt.c` | TSan clean run — 4 threads × 200 iters × 10 planets, **no races on Linux** (each thread sets its own path). Run with `setarch -R` (TSan needs ASLR off on this kernel). |
| `mt2.c` | **Class A**: silent Moshier fallback + wrong Moon longitude on a worker thread |
| `mt3.c` | **Class A**: ayanamsa, topocentric and tid_acc all lost across threads |
| `fd.c` | **Class C**: 3 fds/thread, leaked at thread exit |
| `mem.c` | **Class C**: ~100 KB RSS/thread |

Build recipe used:

```sh
cc -g -O0 -I. scratchpad/tsan/mt2.c swedate.c swehouse.c swejpl.c swemmoon.c \
   swemplan.c sweph.c swephlib.c swecl.c swehel.c -o mt2 -lm -ldl
```

---

## 7. Open questions → next steps

1. ~~Confirm the Class A mechanism is what bites pyswisseph~~ — **DONE, see §9.**
2. Does `swemmoon.c`'s implicit-argument global cluster have a clean refactor,
   or does it need a `struct moonctx *` threaded through?
3. Enumerate the full `swed.fidat[]` fd cost with a realistic asteroid workload
   to size the "hundreds of handles" claim.
4. Confirm `dtcf16` and the `swemplan` tables are never written, so they can be
   `const`.
5. Are there other silent-fallback paths besides SWIEPH→Moshier that mask
   misconfiguration?
6. What do other bindings do (swisseph-rs, sweph node, Java/JNI)? Any that
   already release a lock or spawn a dedicated thread would show a different
   symptom profile.

## 8. Design direction (to be fleshed out in PLAN.md)

The evidence points away from "sprinkle more TLS" and toward an **explicit
context handle**:

```c
swe_ctx *ctx = swe_ctx_new();
swe_ctx_set_ephe_path(ctx, "/path/to/ephe");
swe_calc_ut_r(ctx, tjd, SE_MOON, iflag, x, serr);
swe_ctx_free(ctx);
```

with the legacy global API retained as a thin shim over a process-default
context, so existing code keeps working. That fixes Class A (config is explicit
and shareable), Class B (no reliance on compiler TLS), Class C (lifetime is
explicit; caches/fds can be shared behind a lock or made immutable), and Class D
(statics move into the context).

Open sub-questions: whether the shared read-only ephemeris file cache should be
split from the per-computation scratch state (probably yes — `mmap` + refcount
the `.se1` files once per process, keep only cursors per context).

---

## 9. pyswisseph — root cause confirmed

Analysed **pyswisseph 2.10.3.2** (sdist from PyPI).

### 9.1 What it builds

`setup.py:155-164` compiles exactly the same nine translation units as our
`libswe.a`:

```
libswe/swecl.c   libswe/swedate.c  libswe/swehel.c
libswe/swehouse.c libswe/swejpl.c  libswe/swemmoon.c
libswe/swemplan.c libswe/sweph.c   libswe/swephlib.c
```

It vendors its own copy of `sweodef.h` with the **identical** `TLS` guard.

### 9.2 It never releases the GIL

```
$ grep -c Py_BEGIN_ALLOW_THREADS *.c
0
```

Zero occurrences. `pyswisseph.c` contains no thread handling whatsoever. **Every
`swe_*` call is fully serialised by the GIL.**

This has a decisive consequence: **data races are impossible from Python
threads, on every platform.** Any theory of the pyswisseph bug based on two
threads concurrently clobbering shared state is ruled out on two independent
grounds — the GIL, and (on Linux) TLS.

### 9.3 The actual mechanism — and it inverts

The bug is **Class A only**: `swe.set_ephe_path()` / `set_sid_mode()` /
`set_topo()` called once at import time affect only the thread that called them.
Work dispatched to a `ThreadPoolExecutor` runs against a default-initialised
`swed`.

Demonstrated by building the *same* test both ways:

```
### TLS ON  (Linux wheel — the default) ###
main    ayanamsa=23.857092  topo_moon_lon=222.988064  tidacc=-25.8500  serr=[]
worker  ayanamsa=24.740300  topo_moon_lon=  0.000000  tidacc=-25.8000  serr=[geographic position has not been set]

### TLS OFF (macOS wheel — __APPLE__ disables TLS) ###
main    ayanamsa=23.857092  topo_moon_lon=222.988064  tidacc=-25.8500  serr=[]
worker  ayanamsa=23.857092  topo_moon_lon=222.988064  tidacc=-25.8500  serr=[]
```

**The TLS "thread-safety fix" is precisely what breaks pyswisseph on Linux.**
On macOS, where `TLS` compiles to nothing, `swed` is a shared global — and
because the GIL already serialises access, the shared global is exactly what
users want: configuration set once is visible everywhere. The result:

| Platform | `TLS` | Races? | Config visible across threads? | User experience |
|---|---|---|---|---|
| Linux wheel | `__thread` | No (TLS + GIL) | **No** | **Broken** — silent Moshier fallback, wrong ayanamsa |
| macOS wheel | *(none)* | No (GIL) | **Yes** | Works |
| Windows wheel | depends on whether `WIN32` is defined | No (GIL) | **Inconsistent** | Build-dependent |

This explains the otherwise baffling reports: identical Python code gives
different numbers on Linux vs macOS, and threaded code silently disagrees with
single-threaded code. It also explains why it resists diagnosis — no crash, no
sanitiser hit, and `serr` carries the only warning.

### 9.4 Implication for our design

This is strong evidence for the **explicit-context** direction over "more TLS".
It also means a genuinely useful interim mitigation exists for binding authors,
independent of our rewrite:

- **Short term:** re-apply all `swe_set_*` calls at the top of every worker
  thread, or pin all ephemeris work to one dedicated thread.
- **Build-level:** compiling pyswisseph with `-DTLSOFF` on Linux actually
  *fixes* the observed misbehaviour, because the GIL supplies the missing
  mutual exclusion. Safe **only** while the binding never releases the GIL —
  fragile, and it silently becomes a race the moment anyone adds
  `Py_BEGIN_ALLOW_THREADS`. Worth documenting as a stopgap, not a solution.

---

## 10. Review of a second agent's analysis

A parallel analysis was supplied. Recording where it agrees and where it is
wrong, since two of its conclusions would send the work in the wrong direction.

**Correct:**
- TLS infrastructure exists; `swed` is 23 KB in `.tbss`. ✔ (matches our measurement)
- `swevents.c` / `sweephe4.c` do contain plain non-TLS globals — `xcol`,
  `xcol4`, `ytop`, `pmodel`, … at `swevents.c:228-238`, and `FILE *ephfp` at
  `sweephe4.c:70`. ✔ (verified verbatim)
- `swe_set_sid_mode()` being per-thread needs documenting. ✔

**Incorrect — and consequential:**

1. **"Within a process, multiple threads share the same TLS instance because
   Python's GIL serializes calls… Thread A calls `swe_set_ephe_path()` → Thread
   B overwrites it."**
   This is wrong in both halves. `__thread` storage is **per-thread**, full
   stop; the GIL has no bearing on storage duration. And the failure is the
   *opposite* of clobbering: thread B **cannot** overwrite thread A's path — it
   cannot even *see* it. Verified in §2.2/§2.3 and §9.3.
   *Why it matters:* this framing leads to "add mutexes around the setters",
   which would fix nothing at all.

2. **"[`swevents.c` and `sweephe4.c`] are built into `libswe.a` and linked by
   pyswisseph."**
   False on both counts:
   ```
   $ ar t libswe.a
   swedate.o swehouse.o swejpl.o swemmoon.o swemplan.o sweph.o swephlib.o swecl.o swehel.o
   ```
   Neither file is in `SWEOBJ`, and `pyswisseph/setup.py:155-164` lists the same
   nine sources. `swevents.c` is a standalone PDF/text-plotting **application**;
   `sweephe4.c` is not compiled at all here. Its proposed fix #1 — the top item
   on its list — targets code that is not in the library.

3. **"`sweph.c` — all globals are TLS `swed` or static ✔"**
   Misses `static Dl_info dli;` (`sweph.c:252`), which is a genuine
   non-TLS race. More importantly it treats `static` as equivalent to safe —
   a non-TLS function-level `static` is *exactly* the hazard we're hunting.

4. **Misses the one real race in library code:** `static double saved_sundec`
   (`swehouse.c:636`), read-modify-written in `swe_houses_armc_ex2()`. See §5.

5. **"If Thread A is mid-calculation while Thread B calls `set_ephe_path()`,
   Thread A's open file handles get closed."**
   Not true under TLS: `swi_close_keep_topo_etc()` operates on the calling
   thread's `swed`, so it cannot touch another thread's `fidat[]`. This becomes
   a real hazard only *after* we move to shared state — so it is a valid
   requirement for the **new** design, but not a description of current
   behaviour.

**Net:** its audit table of which files carry TLS is a useful cross-check, and
it independently flagged the need to document per-thread `set_sid_mode()`. But
its causal model of the pyswisseph failure is inverted, and its highest-priority
fix targets non-library files. Our §9 evidence supersedes it.

---

## 11. Triage: what's a bug fix vs. what's a refactor

Measured coupling to the global blob:

```
swed. references:  sweph.c 517 | swephlib.c 89 | swecl.c 27 | swemplan.c 9
                   swehouse.c 4 | swemmoon.c 3 | swedate.c 1 | swejpl.c 0 | swehel.c 0
                   --> 650 total, across ~101 functions
Public API surface: 106 ext_def entry points
```

But the **configuration** surface — the part that actually has to propagate
between threads — is tiny and well-bounded: **9 setters, ~22 scalar fields.**

| Setter | Fields written |
|---|---|
| `swe_set_ephe_path` | `ephepath`, `ephe_path_is_set`, `last_epheflag` (+ closes `fidat[]`) |
| `swe_set_jpl_file` | `jplfnam`, `jpldenum`, `ephepath` |
| `swe_set_topo` | `topd.{geolon,geolat,geoalt,teval}`, `geopos_is_set` |
| `swe_set_sid_mode` | `sidd`, `ayana_is_set`, `astro_models` |
| `swe_set_tid_acc` | `tid_acc`, `is_tid_acc_manual` |
| `swe_set_delta_t_userdef` | `delta_t_userdef`, `delta_t_userdef_is_set` |
| `swe_set_interpolate_nut` | `do_interpolate_nut`, `interpol.*` |
| `swe_set_astro_models` | `astro_models[]` |
| `swe_set_lapse_rate` | `const_lapse_rate` — **note: TLS static in `swecl.c:74`, outside `swed`** |

That asymmetry (650 references to the blob, but only ~22 fields of real
configuration) is what makes a staged fix possible.

### Tier 1 — genuine bugs. Hours.

Independent of any architecture decision; fix now.

- `swehouse.c:636` `saved_sundec` → make it TLS, or better, pass through `struct houses`
- `sweph.c:252` `dli` → make it a local in `swe_get_library_path()`
- `sweph.c:261` `#if !defined(__APPLE)` → `__APPLE__`
- `sweodef.h:86` drop the obsolete `__APPLE__` exclusion; test `_WIN32` not bare `WIN32`
- delete the dead `if ((0))` debug blocks in `swehel.c:834,1290,1393`
- add `const` to the read-only tables (`swemplan.c`, `swephlib.c`, `swecl.c`, `sweph.c`)

### Tier 2 — fix Class A. ~300–500 lines, **no ABI change.**

Extract the ~22 config fields into a `struct swe_config`. Keep one
process-global master copy behind a mutex plus a generation counter; each
thread lazily re-syncs its `swed` copy when `my_gen != global_gen`. A per-field
"explicitly set on this thread" flag lets a thread still override locally.

This makes `swe_set_ephe_path()`-once-at-startup behave the way every caller
already expects, on every platform, and makes Linux and macOS agree. It needs
the sync check only at the ~20–30 entry points that read config, not all 106.

**Known wrinkles to design around:**
- `swe_set_ephe_path()` also closes `fidat[]`; a thread picking up new config
  must invalidate its own file handles at the sync point, not at set time.
- `const_lapse_rate` lives outside `swed` and must move in.
- Requires a portable mutex — the library currently has zero threading
  dependencies, so this introduces `pthread`/SRWLock or C11 `<threads.h>`.

**This work is not throwaway: the config-struct extraction is step 1 of Tier 3.**

### Tier 3 — the real thread-safe library. Weeks.

Explicit context handle, legacy API as a shim over a process-default context:

```c
swe_ctx *ctx = swe_ctx_new();
swe_ctx_set_ephe_path(ctx, "/path/to/ephe");
swe_calc_ut_r(ctx, tjd, SE_MOON, iflag, x, serr);
swe_ctx_free(ctx);
```

Scope: 650 `swed.` rewrites, ~101 functions, 106 shim entry points, plus the
`swemmoon.c` implicit-argument global cluster (§5) which needs a threaded
parameter. Fixes what Tier 2 cannot:

- **independent concurrent configurations** — two requests with different
  sidereal modes or ephemeris paths at the same time (Tier 2 still forces one
  global config unless each thread sets its own)
- **Class C leaks** — lifetime becomes explicit, no reliance on thread exit
- **cache/fd duplication** — the `.se1` files can be `mmap`ed and refcounted
  once per process, with only cursors per context
- **no dependence on compiler TLS at all**

### Recommendation

Ship Tier 1 + Tier 2 first. That eliminates all present user-visible damage —
the silent Moshier fallback, the wrong ayanamsa, the platform divergence — for a
small, reviewable, ABI-compatible change, and it is the natural first commit of
Tier 3 regardless. Then decide whether Tier 3 is worth it based on whether we
actually want independent concurrent contexts (a server workload does; a desktop
chart program does not).

---

## 12. Late findings: the env-var escape hatch and the lazy-init hook

Two discoveries made while prototyping the test harness. Both materially change
the plan.

### 12.1 `SE_EPHE_PATH` overrides the programmatic setter

`sweph.c:1326-1330`, inside `swe_set_ephe_path()`:

```c
/* environment variable SE_EPHE_PATH has priority */
if ((sp = getenv("SE_EPHE_PATH")) != NULL && strlen(sp) != 0
    && strlen(sp) <= AS_MAXCH-1-13) {
  strcpy(s, sp);
} else if (path == NULL || *path == '\0') {
  ...
```

The environment variable **wins over the argument the caller passed**. Worth
knowing on its own (it is surprising, and it means a stray env var silently
overrides application configuration), but its real significance is what follows.

### 12.2 There is already a lazy per-thread init hook

`sweph.c:639-642`, at the top of the `swe_calc()` path:

```c
if (epheflag != SEFLG_MOSEPH && !swed.ephe_path_is_set && !swed.jpl_file_is_open)
  swe_set_ephe_path(NULL);
if ((iflag & SEFLG_SIDEREAL) && !swed.ayana_is_set)
  swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
```

Any thread that has not configured a path **self-initialises on first use** —
and because that path runs `swe_set_ephe_path(NULL)`, it picks up
`getenv("SE_EPHE_PATH")`.

There is also `swi_init_swed_if_start()` (`sweph.c:1181`), already called from
**16 sites** (11 in `sweph.c`, 5 in `swephlib.c`).

**Consequence for the design: the sync point we need for Tier 2 already exists
and is already wired into the entry points.** We do not have to add a check to
106 API functions — we extend hooks that are already there.

### 12.3 A real, zero-code workaround for pyswisseph users — today

Setting the **`SE_EPHE_PATH` environment variable** instead of calling
`swe.set_ephe_path()` fixes the worst symptom on every thread, because each
thread lazily self-initialises from the environment:

```
$ SE_EPHE_PATH=/path/to/ephe ./mt3
main    ayanamsa=23.857092  topo_moon_lon=222.988064  tidacc=-25.8500  serr=[]
worker  ayanamsa=24.740300  topo_moon_lon=  0.000000  tidacc=-25.9360  serr=[geographic position has not been set]
```

**It is only a partial fix.** Compare against §2.3: the silent Moshier fallback
is gone (the worker now reads the real DE file — note `tidacc` auto-set to
-25.9360 from the file), but:

- **ayanamsa is still wrong** (24.740 vs 23.857) — no env var for sidereal mode
- **topocentric still returns 0.0** — no env var for observer position
- **manual `tid_acc` is still lost**

So: recommend it to users as an immediate mitigation for the ephemeris-path
problem, with an explicit warning that sidereal and topocentric work still
requires re-applying the setters per thread. Far preferable to the `-DTLSOFF`
suggestion in §9.4, which is fragile.

### 12.4 Harness caveat discovered the hard way

The first version of the golden harness used `SE_EPHE_PATH` as its *own*
configuration variable — which collided with the library's, silently masked the
bug, and produced a **false pass** (`4/4 threads matched`). The harness now uses
`--ephe` / `SE_TEST_EPHE`, and the thread test correctly fails today:

```
$ env -u SE_EPHE_PATH ./golden --ephe ../ephe --threads 2
thread 0: MISMATCH at line ~2
  main  : rf=2  0x1.100278edab794p+8 ...
  thread: rf=4  0x1.10028622339dap+8 ... | SwissEph file 'seplm12.se1' not found
0/2 threads matched the main-thread transcript
```

**Lesson recorded as a plan requirement: every negative test must be proven to
fail before it is trusted to pass.**
