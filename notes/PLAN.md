# Plan: A Thread-Safe Swiss Ephemeris

**Companion to:** [`INVESTIGATION.md`](INVESTIGATION.md) (root-cause analysis — read that first)
**Baseline:** `3fd0f95`, libswe 2.10.03

---

## 1. Goals

1. **Configuration set once is visible from every thread** — eliminate the silent
   Moshier fallback and wrong-ayanamsa class of bug (`INVESTIGATION.md` §2).
2. **Identical behaviour on Linux, macOS and Windows** — no platform where
   thread safety silently depends on a preprocessor symbol (§3).
3. **Independent concurrent configurations** — two threads computing with
   different sidereal modes or ephemeris paths *at the same time*.
4. **No resource leaks** — per-thread fds and heap reclaimed deterministically (§4).
5. **Bit-exact numerical equivalence with 2.10.03 throughout.**
6. **Source and ABI compatible** — existing applications recompile unchanged,
   ideally relink unchanged.

## 2. Non-goals

- Changing any astronomical algorithm, constant, or file format.
- Improving accuracy. If output changes, we have a bug.
- Reformatting or modernising code we are not otherwise touching. Diffs stay
  reviewable against upstream so we can keep merging their releases.
- Rewriting `swetest.c` / `swevents.c` / `sweephe4.c` (applications, not library —
  §1.3). They get touched only if a signature they call changes.

## 3. Hard constraints

| Constraint | Enforcement |
|---|---|
| **Numerical no-op** | `tests/golden` transcript is byte-identical at every commit |
| **No new required deps** | Threading primitives behind a thin shim; single-threaded builds must still work with `-DSWE_NO_THREADS` |
| **ABI stability** | `abi-check` target diffs exported symbols; additions allowed, removals/changes are not |
| **Upstream mergeable** | No gratuitous reformatting; changes localised and commented |
| **Every negative test proven to fail first** | See §12.4 — a false pass already cost us once |

---

## 4. Success criteria

Concrete, checkable gates. The project is done when all pass.

```sh
# G1  numerical no-op
tests/golden --ephe ephe > out.txt && diff tests/baseline.txt out.txt

# G2  every thread reproduces the main-thread transcript, WITHOUT per-thread setup
env -u SE_EPHE_PATH tests/golden --ephe ephe --threads 8      # exit 0

# G3  config propagation: setters on main are visible on workers
tests/threadconf                                              # exit 0

# G4  independent concurrent contexts (Phase 3 only)
tests/ctxtest                                                 # exit 0

# G5  no data races
setarch -R env TSAN_OPTIONS=halt_on_error=0 tests/golden_tsan --threads 8

# G6  no leaks; fds returned at thread exit
valgrind --leak-check=full --errors-for-leak-kinds=definite tests/golden --threads 8
tests/fdtest                                                  # exit 0

# G7  platform parity
#     G1-G3 pass on Linux/gcc, Linux/clang, macOS/clang, Windows/MSVC
#     and with -DTLSOFF and -DSWE_NO_THREADS

# G8  upstream test suite still passes
cd setest && make && ./setest ...
```

---

## 5. Phase 0 — Infrastructure (do this before touching library code)

**Nothing else starts until Phase 0 is green.** The entire plan rests on being
able to prove a change is a numerical no-op.

### 0.1 Golden baseline harness — `tests/golden.c` ✅ *prototyped*

Already written and working. 4898 assertions, 0.14 s, deterministic across runs.
Covers: 18 planets × 9 dates × 11 flag combinations, `calc` and `calc_ut`,
7 asteroids, 41 sidereal modes, topocentric at 5 sites incl. poles, 24 house
systems × 6 latitudes incl. polar, 10 fixed stars, Δt / sidereal time / nutation,
solar+lunar eclipses, rise/transit, nodes/apsides, phenomena.

Every value printed as `%a` (C99 hex float) — exact, no rounding to hide drift.

Modes:
- `./golden --ephe DIR` → transcript on stdout (**the baseline**)
- `./golden --ephe DIR --threads N` → N threads each produce a full transcript;
  all must match the main thread byte-for-byte. **Currently exits 1** (correct).
- `--per-thread-setup` → re-applies config on each worker; currently exits 0.
  After Phase 2 both must exit 0.

**Verified status** (`cd tests && make check`):

```
G1 PASS: output bit-identical
8/8 threads matched the main-thread transcript (--per-thread-setup)   <- workaround PASS
0/8 threads matched the main-thread transcript                        <- G2 FAIL, as required
```

`tests/Makefile` provides `baseline`, `check-golden` (G1), `check-threads` (G2),
`check-threads-workaround`, and a `golden_tsan` target. The committed
`tests/baseline.txt` (4898 rows) was generated from a tree with **zero tracked
modifications** at `3fd0f95`, so it is a valid pristine baseline.

**Remaining work:**
- [ ] Widen coverage: `swe_houses_armc_ex2` with Sunshine (`hsys='I'`) to pin
      `saved_sundec`; heliacal (`swe_heliacal_ut`) — slow, put behind `--slow`;
      `swe_pheno`, `swe_azalt`, `swe_refrac_extended`, `swe_lun_occult_*`,
      `swe_date_conversion`, `swe_utc_to_jd`, `swe_split_deg`
- [ ] Commit `tests/baseline.txt` generated from **pristine `3fd0f95`**, and
      record the exact command + compiler version in a header comment
- [ ] Cross-check the baseline against a second compiler (gcc vs clang) and
      record any FP divergence *before* we start, so we can tell our changes
      apart from toolchain noise

> ⚠️ Known: 178 rows carry expected `$EPHE`-scrubbed warnings (missing
> `de431.eph` for JPL rows, out-of-range asteroid/Chiron dates). These are
> *stable* and part of the baseline. Do not "fix" them.

### 0.2 Targeted thread tests — `tests/threadconf.c`

Distinct from `golden`. `golden`'s sidereal/topo sections call the setters
*inside* each thread, so they do not test propagation. This one sets every
setter on main only, then asserts a worker sees them. Prototype exists as
`scratchpad/tsan/mt3.c`; promote it into `tests/`.

Must cover all 9 setters from §11, and must **fail today**.

### 0.3 Resource tests — `tests/fdtest.c`

Promote `scratchpad/tsan/fd.c`. Asserts fds return to baseline after threads
exit. Must fail today (leaks 3/thread).

### 0.4 Build & CI

- [ ] `make test` target running G1–G3, G5, G6
- [ ] `make abi-check` — `nm -D --defined-only libswe.so | sort` vs committed
      `abi/libswe.symbols`
- [ ] GitHub Actions matrix: {ubuntu gcc, ubuntu clang, macos clang, windows msvc}
      × {default, `-DTLSOFF`, `-DSWE_NO_THREADS`} × {`-O0`, `-O2`}
- [ ] TSan and Valgrind jobs (Linux only; TSan needs `setarch -R`)
- [ ] Cache `ephe/` — CI needs the real data files or coverage is meaningless

### 0.5 Housekeeping ✅ *done*

`.gitignore` added (build artifacts were landing untracked in the worktree).

**Exit gate:** baseline committed and reproducible; G2/G3/G6 all **fail** for the
documented reasons; CI green on the "expected failure" set.

---

## 6. Phase 1 — Genuine bugs (independent of architecture)

Small, self-contained, individually revertable. Each commit must keep G1 green.

| # | Fix | Location | Note |
|---|---|---|---|
| 1.1 | `saved_sundec` → move into `struct houses` (preferred) or make TLS | `swehouse.c:636` | **Real race.** Needs a Sunshine-houses golden case first (0.1) |
| 1.2 | `dli` → local variable | `sweph.c:252` | `dladdr` writes a shared static |
| 1.3 | `#if !defined(__APPLE)` → `__APPLE__` | `sweph.c:261` | Typo; changes macOS behaviour — verify against baseline |
| 1.4 | Delete dead `if ((0))` debug blocks | `swehel.c:834,1290,1393` | Removes `a.0/a.1/a.2` statics |
| 1.5 | `const`-qualify read-only tables | `swemplan.c`, `swephlib.c`, `swecl.c`, `sweph.c` | ~90 KB `.data`→`.rodata`; **verify never written first** |
| 1.6 | Fix the `TLS` guard | `sweodef.h:86` | Drop obsolete `__APPLE__`; test `_WIN32` not bare `WIN32` |

> **1.6 is behaviour-changing on macOS** — it turns `swed` from shared to
> thread-local, i.e. it *introduces* the Class A bug there where the GIL had been
> masking it (§9.3). **Sequence it after Phase 2**, or land it together with
> Phase 2 in one release. Flagged here so it is not applied naively.

**Exit gate:** G1 green, G5 green, `saved_sundec` race gone under TSan with a
concurrent Sunshine-houses test.

---

## 7. Phase 2 — Config propagation (fixes Class A; no ABI change)

The change that fixes the actual user-facing damage. Ships as **2.10.03-ts1**.

### 7.1 Design

Extract the ~22 configuration fields (§11) into a struct:

```c
struct swe_config {
  char    ephepath[AS_MAXCH];
  char    jplfnam[AS_MAXCH];
  int32   jpldenum;
  struct topo_data topd;
  struct sid_data  sidd;
  double  tid_acc;
  AS_BOOL is_tid_acc_manual;
  double  delta_t_userdef;
  AS_BOOL delta_t_userdef_is_set;
  int32   astro_models[SEI_NMODELS];
  AS_BOOL do_interpolate_nut;
  double  const_lapse_rate;        /* currently swecl.c:74, outside swed */
  AS_BOOL ephe_path_is_set, jpl_file_is_open, geopos_is_set, ayana_is_set;
  uint64  generation;
};
```

Two instances:

- `static struct swe_config swe_cfg_master;` — **process-global**, guarded by
  `swi_cfg_mutex`, carrying a monotonically increasing `generation`
- `swed.cfg` — the calling thread's working copy, plus `swed.cfg_seen_gen`

**Write path** (`swe_set_*`): take mutex → update master → `++generation` →
release → update own copy → set `swed.cfg_seen_gen = generation`.

**Read path**: at the existing hooks — `swi_init_swed_if_start()` (already called
from 16 sites) and the lazy block at `sweph.c:639` — call:

```c
static void swi_sync_config(void) {
  if (swed.cfg_seen_gen == swi_atomic_load(&swe_cfg_master.generation)) return;
  swi_mutex_lock(&swi_cfg_mutex);
  /* fields this thread has NOT explicitly overridden are refreshed */
  swi_cfg_merge(&swed.cfg, &swe_cfg_master, swed.cfg_local_mask);
  swed.cfg_seen_gen = swe_cfg_master.generation;
  swi_mutex_unlock(&swi_cfg_mutex);
  swi_invalidate_caches_for_changed_config();
}
```

The fast path is one relaxed atomic load and a compare — no lock in steady state.

`cfg_local_mask` preserves per-thread overrides: a thread that calls
`swe_set_topo()` itself keeps its own observer position and is not clobbered by a
later global change. This keeps the existing "configure per thread" idiom working
**and** makes "configure once on main" work.

### 7.2 Wrinkles to design around

1. **`swe_set_ephe_path()` closes `fidat[]`.** A thread picking up a new path must
   invalidate *its own* handles at the sync point, not at set time — it cannot
   touch another thread's `FILE*`s. Sync must therefore run
   `swi_close_keep_topo_etc()` locally when `ephepath` changed.
2. **`const_lapse_rate` lives outside `swed`** (`swecl.c:74`) and must move in.
3. **Cache invalidation.** `oec`, `nut`, `savedat[]`, `pldat[]` depend on config.
   Changing `tid_acc` or sidereal mode must invalidate the dependent caches on the
   syncing thread. **Highest-risk item in this phase** — a missed invalidation is
   exactly a silent wrong-number bug. Mitigation: on any generation change,
   invalidate *everything* first; optimise only with G1 green.
4. **`SE_EPHE_PATH` env priority** (§12.1) must be preserved for compatibility,
   and read once into the master rather than per thread.
5. **First threading dependency.** Add `swi_mutex` / `swi_atomic` shims:
   pthreads, SRWLOCK on Windows, no-ops under `-DSWE_NO_THREADS`.
6. **Recursion.** `swe_set_ephe_path()` internally calls `swe_calc()`
   (`sweph.c:1346`), which calls the sync hook. Guard against re-entering the
   mutex — use a non-recursive design with an "already syncing" thread flag.

### 7.3 Steps

- [ ] 2.1 Add `swi_mutex`/`swi_atomic` shim + `SWE_NO_THREADS` build (no behaviour change; G1)
- [ ] 2.2 Move `const_lapse_rate` into `swed` (G1)
- [ ] 2.3 Introduce `struct swe_config`, move the 22 fields, leave accessors
        pointing at `swed.cfg.*` — **pure mechanical refactor, G1 must stay green**
- [ ] 2.4 Add master + generation + `swi_sync_config()`, wired into
        `swi_init_swed_if_start()` and `sweph.c:639`
- [ ] 2.5 Conservative full cache invalidation on generation change
- [ ] 2.6 `cfg_local_mask` per-thread override semantics
- [ ] 2.7 Narrow the invalidation to what actually depends on each field (perf)

**Exit gate:** G1, G2, G3, G5, G6 all green. G2/G3 **transition from fail to
pass** — confirm they failed at Phase 0. Phase 1.6 (`TLS` guard) can now land, and
macOS/Linux/Windows agree.

**Ship here.** This resolves every symptom users currently hit. Announce with the
`SE_EPHE_PATH` mitigation (§12.3) for people on older versions.

---

## 8. Phase 3 — Explicit context handle (the real fix)

Only Phase 3 delivers Goal 3 (independent concurrent configurations), Goal 4
(deterministic lifetime) and removes dependence on compiler TLS entirely.

Scope, measured: **650 `swed.` references, ~101 functions, 106 API entry points.**

### 8.1 Target API

```c
typedef struct swe_ctx swe_ctx;

swe_ctx *swe_ctx_new(void);
void     swe_ctx_free(swe_ctx *);

int32 swe_calc_ut_r(swe_ctx *, double tjd, int32 ipl, int32 iflag, double *xx, char *serr);
void  swe_set_ephe_path_r(swe_ctx *, const char *path);
/* ... _r variant for each of the 106 entry points ... */
```

Legacy API becomes a shim over a process-default context:

```c
int32 CALL_CONV swe_calc_ut(double tjd, int32 ipl, int32 iflag, double *xx, char *serr) {
  return swe_calc_ut_r(swi_default_ctx(), tjd, ipl, iflag, xx, serr);
}
```

Whether `swi_default_ctx()` is per-process (with Phase 2's config machinery) or
per-thread is a **decision point** — see §8.6.

### 8.2 3a — Mechanical `swed.` → `ctx->`

Highest volume, lowest intellectual risk. Order by coupling (ascending), so the
approach is proven on small files first:

```
swedate.c (1)  swemmoon.c (3)  swehouse.c (4)  swemplan.c (9)
swecl.c (27)   swephlib.c (89)   sweph.c (517)
```

Every internal `swi_*` function gains a leading `swe_ctx *ctx`. Do it with a
scripted rewrite plus review, one file per commit, **G1 green at each commit**.

### 8.3 3b — `swemmoon.c` implicit-argument globals

The nastiest cluster: **14 functions sharing ~25 file-scope doubles as implicit
parameters** — `chewm` (25 of them), `moon1` (22), `swi_intp_apsides` (14),
`moon2` (11), `moon3`, `moon4`, `swi_mean_node`, `swi_mean_apog`,
`swi_mean_lunar_elements`, `mean_elements`, `mean_elements_pl`, `sscc`,
`swi_moshmoon`, `swi_moshmoon2`.

Bundle into `struct moon_state` and thread a pointer through. Self-contained to
one 1930-line file. **G1 is the whole safety net here** — the lunar rows in the
baseline are the assertion.

### 8.4 3c — Remaining TLS statics into the context

All the memo caches and scratch listed in §5: `sweph.c` (10 sites), `swehel.c`
(7), `swejpl.c` (3), `swephlib.c` (`dt[]`, `crc32_table`, trace state),
`swedate.c` (leap seconds), `swemplan.c` (`ss`/`cc`).

Also fixes the trace-file corruption: `swi_fp_trace_*` currently has every thread
writing the same path.

### 8.5 3d — Public `_r` API, shim, headers

- [ ] `_r` variants for all 106 entry points
- [ ] Legacy shims (`abi-check` must show additions only)
- [ ] `swephexp.h` documented; `swe_ctx` opaque
- [ ] `tests/ctxtest.c` — G4: two contexts, different sidereal modes and
      ephemeris paths, computing concurrently, each matching its own baseline

### 8.6 3e — Shared ephemeris file cache  *(decision point)*

Today each context/thread opens its own `FILE*` per `.se1` (3+ fds/thread, §4.1)
and re-decodes the same Chebyshev segments.

Proposal: split state in two —
- **shared, immutable, refcounted**: open `.se1` files (`mmap`), file headers,
  `gcdat`, fixed-star table, Δt table, leap seconds
- **per-context, mutable**: cursors, `pldat[]`, `savedat[]`, `oec`, `nut`, config

This is where the real scalability win is, and it is separable — **do not couple
it to 3a–3d**. Evaluate after 3d ships; it may not be worth the complexity for
desktop workloads.

**Exit gate:** G1–G8 all green.

---

## 9. Phase 4 — Documentation, bindings, release

- [ ] `THREADING.md`: the model, what is shared vs per-context, migration guide
- [ ] Document `SE_EPHE_PATH` env-var priority (§12.1) — currently undocumented
- [ ] Upstream the Phase 1 bug fixes to Astrodienst regardless of what happens
      with the rest; they are unambiguous fixes
- [ ] pyswisseph: patch to use `_r` API + release the GIL. **Note: it currently
      never releases the GIL (§9.2), so adding `Py_BEGIN_ALLOW_THREADS` is only
      safe after Phase 2 lands** — today it would convert a benign bug into a
      real race on macOS builds
- [ ] Versioning: `swe_version()` suffix, `SE_THREADSAFE` feature macro so
      bindings can detect capability at compile time

---

## 10. Risk register

| Risk | Severity | Mitigation |
|---|---|---|
| **Silent numerical regression** | **Critical** | G1 at every commit; `%a` output; baseline from pristine tree; two compilers cross-checked |
| **Missed cache invalidation in Phase 2** | **High** | Invalidate everything first, optimise later; per-field dependency table reviewed explicitly |
| **A test that passes for the wrong reason** | **High** | Already bit us (§12.4). Every negative test must be demonstrated failing before it is trusted |
| Phase 1.6 regresses macOS | Medium | Sequence after Phase 2, or ship together |
| Merge divergence from upstream | Medium | One concern per commit, no reformatting, keep `_r` additive |
| `swemmoon.c` refactor subtly wrong | Medium | Lunar rows in G1; isolate as its own commit; consider differential fuzzing over random JDs |
| Mutex contention hurts throughput | Low | Fast path is an atomic load + compare; benchmark before/after |
| Windows/MSVC divergence | Medium | In CI matrix from Phase 0, not bolted on at the end |
| Scope creep into Phase 3e | Medium | Explicit decision point; 3e is optional |

## 11. Sequencing and effort

```
Phase 0  infrastructure          ████                      ~1 week   ← prototyped
Phase 1  bug fixes               ██                        ~2 days
Phase 2  config propagation      ██████████                ~2 weeks  ← SHIP HERE
Phase 3a mechanical ctx          ████████████              ~2-3 weeks
Phase 3b swemmoon                ████                      ~3 days
Phase 3c remaining statics       ██████                    ~1 week
Phase 3d public _r API           ████████                  ~1.5 weeks
Phase 3e shared cache (optional) ██████████                ~2 weeks
Phase 4  docs/bindings/release   ████                      ~1 week
```

Phases 0→2 is the high-value slice: roughly **3–4 weeks to a release that fixes
every symptom users actually hit**, ABI-compatible. Phase 3 is the real
architectural fix and can proceed at its own pace behind that release.

## 12. Immediate next actions

1. ~~Golden harness + `make check` gates~~ — **done, G1 passes / G2 fails as required**
2. ~~Commit `tests/baseline.txt` from pristine `3fd0f95`~~ — **done, 4898 rows**
3. Widen `tests/golden.c` coverage (§0.1) — **Sunshine houses first**, since
   Phase 1.1 (`saved_sundec`) cannot be verified without it
4. Promote `threadconf` and `fdtest` out of scratchpad; **prove both fail**
5. Cross-check the baseline under clang, record any toolchain FP divergence now
6. Stand up the CI matrix
7. Then Phase 1.1–1.5 (holding 1.6 for Phase 2)

---

## 13. Phase 1 outcome (completed)

| # | Item | Result |
|---|---|---|
| 1.1 | `saved_sundec` | **Fixed.** Real, dramatic race: 1336/1600 recalls returned another thread's declination. TSan named it. Now TLS; 0/1600, 0 races. |
| 1.2 | `dli` shared static | **Fixed**, but *latent*, not firing — all threads write identical content, and glibc's loader lock appears to order it. Cannot produce a wrong answer. |
| 1.3 | `__APPLE` typo | **Plan was wrong.** Fixing the spelling would have *regressed* macOS (see below). Dead conditional removed instead. |
| 1.4 | Dead debug blocks | **Fixed.** `swehel.o` now has no non-TLS writable statics. |
| 1.5 | `const` tables | **Fixed.** `.data` in `libswe.so`: 127232 → 248 bytes. No ABI symbol removed. |
| 1.6 | `TLS` guard | **Deliberately not done** — held for Phase 2, as planned. |

Two corrections to §6 worth carrying forward:

- **1.1 could not be done the way §6 proposed.** "Move into `struct houses`"
  is impossible — the value is deliberately *cross-call* state, so it cannot
  live in a per-call struct. And `swed` was wrong too: `swi_init_swed_if_start()`
  `memset`s the struct to zero, which would silently replace the `99` sentinel
  with `0`. TLS was the only provably behaviour-neutral option.
- **1.3 was backwards.** `#if !defined(__APPLE)` is a typo for `__APPLE__`, so
  nothing ever defined it and the body has *always* run everywhere, macOS
  included. "Fixing" the spelling would have made `swe_get_library_path()`
  return `""` on macOS — a functional regression. The dead conditional was
  removed instead. **Lesson: a typo in a preprocessor guard may be load-bearing;
  check which way the condition actually evaluates before correcting it.**

Also found and fixed while in `swe_get_library_path()` — adjacent, not
thread-related, but not safe to leave next to code we were already touching:

- `s[len] = '\0'` with `len == AS_MAXCH` wrote one past the end of the
  caller's `char[AS_MAXCH]` buffer.
- `bytes` was `size_t` receiving `readlink()`'s return; on failure `-1`
  became `SIZE_MAX` and `s[bytes] = '\0'` was a wild write.

**Audit target met:** an ELF scan of all nine library objects now reports
**zero** non-TLS writable statics.

**Gate status after Phase 1:**

```
G1 golden bit-identical ......... PASS
   sunrace (1.1) ................ PASS  0/1600 contaminated
   glprace (1.2) ................ PASS
   threads --per-thread-setup ... PASS  8/8
G2 threads, no per-thread setup .. FAIL  0/8   <- expected; Phase 2 fixes this
```

---

## 14. Impact of REVIEW.md on Phase 2

[REVIEW.md](REVIEW.md) is a separate modernization/performance survey. Most of
it is orthogonal to thread-safety, but **four items change Phase 2 before we
start**. Each was verified against the source rather than taken on trust.

### 14.1 A second cache-invalidation trigger — `sweph.c:389-403` ✅ verified

§2 flags this as a perf issue. It is more than that. `swecalc()` contains:

```c
if (swed.last_epheflag != epheflag) {
  free_planets();
  if (ipl != SE_ECL_NUT) {
    if (swed.jpl_file_is_open) { swi_close_jpl_file(); ... }
    for (i = 0; i < SEI_NEPHFILES; i++) {
      if (swed.fidat[i].fptr != NULL) fclose(swed.fidat[i].fptr);
      memset(&swed.fidat[i], 0, sizeof(struct file_data));
    }
    swed.last_epheflag = epheflag;
  }
}
```

§7.2 wrinkle 1 accounted only for `swe_set_ephe_path()` closing `fidat[]`.
**This is a second, independent path that closes every ephemeris file**, keyed
on `swed.last_epheflag`. Phase 2 must decide whether `last_epheflag` is
config (shared) or per-thread derived state — it is currently written by both
`swe_set_ephe_path()` and `swecalc()`. Getting this wrong means either a
thread closing files it should keep, or stale files surviving a config change.

### 14.2 `swe_set_ephe_path()` calls `swe_calc()` — the recursion is concrete ✅ verified

`sweph.c:1350`:

```c
iflag = SEFLG_SWIEPH|SEFLG_J2000|SEFLG_TRUEPOS|SEFLG_ICRS;
swed.last_epheflag = 2;
swe_calc(J2000, SE_MOON, iflag, xx, serr);
if (swed.fidat[SEI_FILE_MOON].fptr != NULL)
  swi_set_tid_acc(0, 0, swed.fidat[SEI_FILE_MOON].sweph_denum, NULL);
```

This upgrades §7.2 wrinkle 6 from a general caution to a named hazard with an
exact call path:

```
swe_set_ephe_path()  [takes cfg mutex]
  -> swe_calc()
    -> swi_init_swed_if_start()  [our sync hook]
      -> swi_sync_config()       [takes cfg mutex]   *** DEADLOCK ***
```

Worse, it means **setting the ephemeris path mutates `tid_acc`** — another
config field — as a side effect, via `swi_set_tid_acc()`. So config sync can
itself write config. The design must:

- use a per-thread "already syncing" flag so the hook is a no-op re-entrantly,
  and never take the mutex recursively;
- treat the `tid_acc` write from `swi_set_tid_acc()` as a *derived* update
  that must not bump the global generation (or it will ping-pong between
  threads forever);
- account for the cost: a lazy sync that re-runs `swe_set_ephe_path()` pays a
  full Moon `swe_calc()` **per thread**.

Today the recursion is harmless only because `swed.ephe_path_is_set = TRUE` is
set at `sweph.c:1325`, *before* the `swe_calc()` at 1350.

### 14.3 No `-O2` anywhere — and the baseline is not optimisation-stable ✅ measured

§1.1 notes the shipped `Makefile` has no `-O` flag at all. Following that up
exposed a gap in **our own gates**: everything had been tested at `-O0`.

Measured, gcc `-O0` vs `-O2` on this tree:

```
224 values differ across 65 of 5127 rows
  median   2.220e-15   (pure ULP noise)
  max      7.634e-08 deg = 0.00027 arcsec
```

All in `field[3..5]` — the **speed** components, which are finite-differenced
(evaluate at `t ± dt`, subtract). That amplifies rounding, and `-O2` may
contract to FMA or reassociate. The magnitude is far below the library's own
accuracy, but it is **not zero**.

Consequences, now implemented:

- **G1 bit-exactness is only valid at fixed build flags.** `baseline.txt` is
  pinned to `gcc -O0` (the `CFLAGS` in `tests/Makefile`).
- Cross-configuration comparison needs tolerance, not `diff`. Added
  `tests/cmpgolden.py` and gate **G1b** (`make check-golden-O2`, default
  tolerance 1e-6). Verified it fails at `--abs=1e-20`.
- This matters specifically for Phase 2, which introduces atomics and memory
  ordering — behaviour there genuinely varies with optimisation level, so the
  `-O2` gate must exist *before* the work, not after.

### 14.4 No `stdint.h`/`stdbool.h`; hand-rolled `int32` — affects the shim

§1.3: zero uses of `<stdint.h>`/`<stdbool.h>`; fixed-width types are
hand-rolled in `sweodef.h:193-216` with legacy 16-bit-compiler branches, and
`AS_BOOL` is `typedef int`. So the Phase 2 shim **cannot assume C11**:

- the generation counter cannot simply be `uint64_t` / `_Atomic`;
- `<stdatomic.h>` may not be available on the compilers this codebase still
  targets;
- the `swi_mutex`/`swi_atomic` shim (§7.3 step 2.1) must degrade to
  compiler builtins (`__atomic_*` / `__sync_*`), then to a mutex-guarded
  fallback, then to no-ops under `-DSWE_NO_THREADS`.

### 14.5 Verified NOT blocking

- **J1, JPL buffer overflow (`swejpl.c`)** — the review is right that `ncoeffs`
  (up to 2500) and `ncf` (unbounded, read from the file header) are trusted
  against `double buf[1500]` and `pc/vc/ac/jc[18]`. That is a genuine
  memory-safety bug and worth fixing, but `swejpl.c` state is TLS and Phase 2
  does not touch it. Route to Phase 3, or fix standalone — it does not gate
  config propagation.
- **S1, the "Sheoran" branch — the review overstates this.** The dead branches
  at `sweph.c:6772` and `6799` are genuinely unreachable, but each copies a
  **byte-identical** record to the branch that subsumes it, so the fallthrough
  returns *the same data*, not the wrong data. Checked against our own
  coverage: the modes produce clearly distinct ayanamsa at J2000 —
  Pushya (29) 22.727°, Sheoran (39) 25.234°, GalEqu (32) 30.076°,
  GalEqu-mid-Mula (33) 23.409° — so the "wrong star record" consequence does
  not manifest. It is dead code to delete for clarity, not a correctness bug.
- `sweph.h:606-607` "stale externs" — already inside a `/* ... */` comment
  block. Harmless; delete when convenient.
- Everything else in REVIEW.md — unsafe strings, goto density, missing enums,
  duplicated house/eclipse loops — is real maintainability debt but orthogonal
  to config propagation. It should not be swept into Phase 2, which needs to
  stay small and reviewable to be shippable.

### 14.6 Revised Phase 2 entry checklist

- [x] G1b `-O2` gate exists and passes (§14.3)
- [x] setest differential gate G8 exists and passes
- [x] `swi_mutex`/`swi_atomic` shim built and tested (`swethread.h`, commit fa5fde8)
- [x] `last_epheflag` classified — **derived, per-thread** ([CONFIG-MAP.md](CONFIG-MAP.md) §3.1)
- [x] Re-entrancy design for `swe_set_ephe_path -> swe_calc -> sync` ([CONFIG-MAP.md](CONFIG-MAP.md) §3.2)
- [x] `swi_set_tid_acc()`'s derived write must **not** bump the generation ([CONFIG-MAP.md](CONFIG-MAP.md) §3.3)
- [x] Full config read/write map complete — [CONFIG-MAP.md](CONFIG-MAP.md)

**Correction to §7.1/§11 from that map:** `struct swe_config` holds **12**
fields, not the ~22 first estimated. `last_epheflag`, `jpldenum` and
`jpl_file_is_open` are derived per-thread state, and `struct topo_data` must be
*split* — `geolon/geolat/geoalt` are config, `teval/tjd_ut/xobs[6]` are cache.
A further finding, `get_aya_correction()` save/restoring `astro_models` on a
compute path, makes the per-thread working copy mandatory rather than merely
convenient ([CONFIG-MAP.md](CONFIG-MAP.md) §3.4).
