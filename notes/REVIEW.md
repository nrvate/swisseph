# Swiss Ephemeris — what is still worth doing in the library code

**Status:** living list. Closed items are one line each at the bottom;
anything judged not worth doing is listed with the reason, so it is not
re-audited. Everything above the line is real, scoped, and provable on the
bit-exact gates — the rule for landing it. `make -C tests check` runs every
gate but G8 in about 15 s; `make -C tests check-all` adds G8, the setest
differential against `origin/legacy-master`.
**Base:** `main` @ `07fc6b4` (rollup #11 merged); `SE_VERSION` is still
2.10.03-ts.5, not yet tagged.
**Scope:** root `*.c`/`*.h`; `windows/`, `setest/` and the samples only where
noted.

The original 2026-08-22 survey this grew out of catalogued idioms — `goto`
counts, `#define`-only constants, license boilerplate, `const`-correctness —
that are true of the whole codebase and change nothing by being listed. They
are not here. What is here is what a maintainer can pick up, do in a commit
or two, and verify.

---

## 1. Open — correctness

Nothing open. The bounded-strings sweep is finished and its fallout is
repaired; see Closed.

## 2. Open — performance, bit-exact

- **`calc_deltat` recomputed at the same instant.** With three memos now in
  place it is what the heliacal profile leads with: 30% of samples, 422,842
  calls against 34,000 position calls, nearly all of them in `deltat_aa`.
  Every `swe_sidtime`, every `ObjectLoc` and every ET↔UT conversion asks for
  delta-t at an instant something else just asked about.
  Same shape as the nutation memo and needing the same care: `calc_deltat`
  reads `tid_acc`, `astro_models[SE_MODEL_DELTAT]`, `jpldenum`, `fidat[]`
  and `dt[]`, so `(tjd, iflag)` alone is not a key. `tid_acc` is the awkward
  one — `swi_set_tid_acc()` derives it per-thread from whichever file that
  thread opened, deliberately without a generation bump (see `sweconfig.h`),
  so it belongs in the key rather than behind an invalidation hook.

## 3. Open — hygiene, bit-exact by construction

Nothing open; see Closed.

## 4. Open — larger refactors, gate-verifiable

Worth doing only as their own rollup, with G1/G8 as the proof:

- `swehouse.c`: the ~15-line house-cusp convergence block is copied 6×
  (Gauquelin 2×, Placidus 4×) and the polar-circle AC/DC swap 5×.
- `swecl.c`: the eclipse-maximum search loop is copied 6+×. Any fix to one
  copy has to be applied to all. (The 44 `goto`s are a smaller problem than
  they look: they reach only 5 distinct labels, and 36 of them are one
  retry idiom, `goto next_try`.)
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
- **Memoising heliacal `ObjectLoc()` on `(instant, object, flags)`.** Built
  and measured, then dropped. The premise holds — 52% of a search's
  `ObjectLoc` calls are exact repeats, matching the "about half" this list
  used to claim — but they were never recomputed: `swe_calc()` already
  returns a repeat from its per-planet save area (`savedat[]`, sweph.c:460)
  on `(tjd, ipl, iflag)`. Worth 3% before the sidtime memo below and 0.7%
  after it, which is inside run-to-run noise, against 1.4 KB on every
  context and a cache that is only correct while every public heliacal
  entry point remembers to empty it. A silent-staleness hazard for 0.7%.
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
| `%.230s` precisions from `cd266f9` overflowed the fixed text, so `make LTO=0` failed under `-Werror=format-truncation` (21 diagnostics; CI never builds `LTO=0`) | `c4f35f4`; every precision sized so the worst case fits 255. Also the six `read_elements_file()` sites and `swi_gen_filename`'s century suffix that the sweep skipped, and the last two callers (`swe_set_ephe_path_r`, `swevents -ejpl`) |
| `cd266f9` changed 255 lines but only 99 ignoring whitespace: re-indented blocks in `swecl`/`swemplan`/`swehel`/`sweph`, backslashes appended to two comments, `'\t'` written as a literal tab, `"star  not found"` silently respelt | `df454c8` |
| Per-double `fread()` in the JPL record read | `24b6f19`, one bulk read + one reorder; 20,000 record-crossing `swe_calc()` calls 0.46 s → 0.16 s |
| `#if 0` region `3b9d090` missed because it is indented (`swemplan.c`) | `ca68bc9` |
| Bare column offsets in the IERS finals and astorb parsers; `degstr()` `sprintf` into `char[20]` | `8e5f794`; `swephgen4.c`/`sweephe4.c` now clean under `-Werror`, and the K&R definitions listed here were already gone |
| `sidtime_non_polynomial_part` recomputed its 33-term series on every `swe_sidtime`, and `swi_ldp_peps` its 10 Vondrák terms on every `swi_epsiln` | `d26da2a`; both are pure functions of one argument, so the key is the whole correctness argument and neither memo needs invalidating. Heliacal benchmark 0.70 s → 0.56 s, G1 bit-identical |
| `struct hel_state.calc` and `.fastmag` outlived the `#if 0` functions that owned them | `d26da2a`; orphaned by `3b9d090`, referenced nowhere since |
| `swe_heliacal_ut_r`'s local `serr` was read before anything wrote it, and `strcpy`'d out to the caller | `4f2f5d5` |
| tests/ object rules had no header prerequisites, so a `sweph.h` edit rebuilt nothing and mixed struct offsets linked cleanly and then corrupted memory; `clean` left `.obj-*/` behind | `69495ff`, `-MMD -MP` + `-include`. Reachable only since `38fc47d` cached objects per flavour |
| `calc_nutation` recomputed at an instant just computed — 39,376 calls, 73% of them immediate repeats, because `swe_sidtime` reaches `swi_nutation` directly and misses `swi_check_nutation`'s cache | one-slot memo keyed on `(J, nut_model)`, declining both JPLHOR flags rather than describing them. Heliacal benchmark 0.57 s → 0.37 s |
| `swe_set_astro_models()` invalidated **nothing** on its own thread, and `swe_set_sid_mode()` cleared positions but not the nutation cache, so switching nutation or precession model and re-asking about a computed instant returned the old model's numbers | one `swi_invalidate_models()` used by both setters and by `swi_config_apply`; `cov:nutswitch` rows pin it. No existing transcript row moved |
| The five nutation models had no coverage at all, and nothing exercised a model change | `cov:nutmodel[1..5]` through `swe_sidtime` at one instant — which also fails if the memo's key loses `nut_model`; `cov:nutswitch[before,after]` for the invalidation |
| `check-threadshim` failed ~1 run in 12 under load: the publisher ran a fixed 20,000 iterations and could set `stop_readers` before a starved reader ran at all, tripping the test's own "test is vacuous" guard | publisher now also waits until every reader has observed something, with a 50× ceiling so a genuinely blind reader still fails loudly. 96 concurrent runs on 12 cores, 0 failures; the ceiling stretched to 128,508 publishes at worst |
