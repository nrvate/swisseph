# Swiss Ephemeris — what is still worth doing in the library code

**Status:** living list. Closed items are one line each at the bottom;
anything judged not worth doing is listed with the reason, so it is not
re-audited. Everything above the line is real, scoped, and provable on the
bit-exact gates (`make -C tests check`, G8) — the rule for landing it.
**Base:** `main` @ 2.10.03-ts.5, `work` rollup #11.
**Scope:** root `*.c`/`*.h`; `windows/`, `setest/` and the samples only where
noted.

The original 2026-08-22 survey this grew out of catalogued idioms — `goto`
counts, `#define`-only constants, license boilerplate, `const`-correctness —
that are true of the whole codebase and change nothing by being listed. They
are not here. What is here is what a maintainer can pick up, do in a commit
or two, and verify.

---

## 1. Open — correctness

- **Bounded strings, remainder of the original survey.** The ~50
  variable-length sites that could overflow `AS_MAXCH` are done
  (see Closed). What is left is style, not safety: the other ~390
  `strcpy`/`strcat`/`sprintf` sites copy literals or fixed-length
  values and stay as they are. Two small leftovers worth a commit:
  - `swevents.c` `-ejpl<name>`: `char fname[80]` filled from `argv`
    (sample program, not the library).
  - `swe_set_ephe_path_r` / `swi_gen_filename`: filename assembly is
    `strlen`-guarded today; `snprintf` would make the bound structural
    rather than checked at each site.

## 2. Open — performance, bit-exact

- **Per-double `fread()` in the JPL record read** (`swejpl.c`, the `state()`
  record loop): up to ~1,000 `fread`+`reorder` calls per 32/64-day record
  instead of one bulk read and one reorder pass. Same bytes, same doubles;
  G10/G15 prove it. The bulk-read idiom already exists in the same file
  (the `eh_cval` read).
- **Heliacal `ObjectLoc()` recomputed for the same instant**: called 5× in a
  row by `swe_heliacal_pheno_ut` and twice per step (directly and via
  `DeterTAV`) in the visibility search — order of 1,500–3,000 ephemeris
  calls per search, about half exact duplicates. Memoising on
  `(TimePointer, object, flags)` reuses identical results, so the transcript
  cannot move. The `hel_ut`/`helpheno`/`vislim` rows cover it.
- **Vondrák periodic series recomputed per call** at the same epoch in
  `sidtime_long_term` and `precess_2`; **`sidtime_non_polynomial_part`**
  recomputes a 33-term series on every `swe_sidtime`. Both are candidates
  for the same memoisation `swi_nutation` already uses (`interpol.tjd_nut*`).
  Bit-exact as long as the cached value is the computed one, not an
  interpolation.

## 3. Open — hygiene, bit-exact by construction

Verified by the gates and `make -C tests check-build`:

- `swephgen4.c`: the last two K&R definitions in the tree; it and
  `sweephe4.c` include no standard headers directly. Neither is built by any
  target, which is why nothing has noticed.
- IERS bulletin parsing in `load_dpsi_deps()` uses bare column offsets
  (`atoi(s + 7)`, `atof(s + 168)`, …); name them.

## 4. Open — larger refactors, gate-verifiable

Worth doing only as their own rollup, with G1/G8 as the proof:

- `swehouse.c`: the ~15-line house-cusp convergence block is copied 6×
  (Gauquelin 2×, Placidus 4×) and the polar-circle AC/DC swap 5×.
- `swecl.c`: the eclipse-maximum search loop is copied 6+×; 44 `goto`
  labels drive the state machines. Any fix to one copy has to be applied to
  all.
- `sweph.c`: the JPL→Swiss→Moshier fallback chain is implemented separately
  for the Moon, the barycentric Sun and each planet class.

---

## Not doing — and why

- **Keeping `.se1` files open across an ephemeris-flag switch.** Tried on
  the ts.5 rollup and reverted. Upstream's close-and-reopen silently does
  three jobs — restores per-planet header constants, picks the canonical
  file at a file boundary (an open neighbour also "covers" the date and
  evaluates it from a different segment), and decides whose general
  constants are live — and reproducing each bit-for-bit cost ~80 lines and
  two struct fields to save one `fopen` per switch.
- **Moshier speed by finite differences over the full series**
  (`swi_moshplan`/`swi_moshmoon`). Any reuse of intermediate terms changes
  the arithmetic order; not bit-exact.
- **Eclipse-max loops calling `swe_calc` 4× per sample** (deriving
  Cartesian from equatorial), **`rise_set_fast`'s numerical altitude
  derivative**, **the triple ET↔UT convergence without early exit**,
  **`calc_nutation_iau2000ab`'s per-term `sin`/`cos`** (the 1980 model's
  angle-addition recurrence is the alternative). All change results in the
  last bits; each would need its own tolerance decision, and none showed up
  in a profile.
- **`pow(x, 2)` → `x*x`**: not guaranteed bit-identical.
- **`swe_version()`'s `strcpy`**: a 12-character literal into a buffer the
  API defines as `AS_MAXCH`.
- **The JPL reader's other `sprintf` error strings**: now `snprintf` with the
  file name precision-limited (`cd266f9`); the remaining ten are fixed text
  plus numbers, left alone as style.
- **`SunRA()` under `SIMULATE_VICTORVB`**: the high-precision branch is
  never compiled and the monthly approximation is used. Upstream behaviour;
  changing it moves results.
- **API-shape items** — raw `double *` out-arrays, `#define`-only constants
  and 0 enums, `char *serr` as the only error channel, `swe_cs2timestr`'s
  unsized output, `swedll.h` duplicating `swephexp.h`, `const` on the public
  surface. Each is an ABI or source-compatibility change, and this fork's
  contract is "additive only".
- **Idioms** — 117 `for (i = 0; i <= 5; i++) xx[i] = x[i]` copies, `goto`
  counts, `DEGTORAD/3600` written out 42 times, model-dispatch ladders,
  `short`-quantised vs `double` coefficient tables, license boilerplate
  without SPDX, hand-rolled `int32`/`AS_BOOL`, autotools/CMake. Nothing
  changes by listing them.

---

## Closed

| Item | Where |
|---|---|
| J1: JPL reader trusted the file's `ksize`/`ncf` against fixed arrays | `04bb51a`; guards + `static_assert`; `jplguard` gate |
| S1: unreachable `get_builtin_star()` branch | dead code, not a wrong result; deleted on `threadsafe` |
| No `-O2`, no `-std`, `-Wall` only, no CI | `-std=c17 -Wall -Wextra -Werror -O2`, 13-job CI, on `threadsafe` |
| LTO opt-in, parity unverified | default on since ts.5 (`5b3eaa9`); gcc/clang/Apple clang matrix; `-flto=auto` on gcc (`6ad888c`) |
| clang `-Werror` failures in `swehel.c`; `sweephe4.c` K&R `getenv` | fixed on `threadsafe` |
| `swe_set_ephe_path()` ran a full Moon `swe_calc()` for the DE number | `11abbbd`, reads the header |
| `search_star_in_list()` linear scan for `name%` | `333e581`, binary lower bound |
| JPL header parsed twice (`fsizer` + `state`) | `f9b614a` |
| `seorbel.txt` reopened and re-tokenised per fictitious-body call | `ace33c4`, per-context line cache; `cov:fict` rows |
| `swi_strcpy` hand-rolled `memmove` with a `strdup` fallback | `bd4b843` |
| setest aborted with a stack smash; G8 depended on path length | `dobs[5]` in `suite_09`, fixed on ts.5; reference runs our harness |
| ~50 variable-length `sprintf(serr, …%s…)` sites overflowing `AS_MAXCH` (incl. the `starname` caller overflow in `swe_lun_occult_when_glob`) | `cd266f9`, `snprintf` + precision limits; `2607947` fixes two null-char escape typos it introduced |
| `MOSH_MOON_200` dead DE200 lunar theory (~415 lines, `swemmoon.c`) | `ed33133`; macro was defined nowhere |
| `#if 0` regions, no `#else` (369 lines: `swehel`, `swephlib`, `swecl`, `swehouse`) | `3b9d090`; `#if 0/#else` config choices kept |
| 8 always-true `#if 1` wrappers in `sweph.c` (one with dead `#else`) | `8aab6fc` |
| 2 runtime-dead `if ((0))` blocks in `swehel.c` + variables they left unused | `e2f45fc` |
| `swehouse.h` no include guard; phantom `sweclips.o` rule; orphan `sweventss` target | `8dcd53d` |
