# Swiss Ephemeris — what is still worth doing in the library code

**Status:** living list. Closed items are one line each at the bottom;
anything judged not worth doing is listed with the reason, so it is not
re-audited. Everything above the line is real, scoped, and provable on the
bit-exact gates — the rule for landing it. `make -C tests check` runs every
gate but G8 in about 15 s; `make -C tests check-all` adds G8, the setest
differential against `origin/legacy-master`.
**Base:** `main` @ `558ac48`, released as 2.10.03-ts.6 — rollups #11, #12
and #13.
**Scope:** root `*.c`/`*.h`; `windows/`, `setest/` and the samples only where
noted.

The original 2026-08-22 survey this grew out of catalogued idioms — `goto`
counts, `#define`-only constants, license boilerplate, `const`-correctness —
that are true of the whole codebase and change nothing by being listed. They
are not here. What is here is what a maintainer can pick up, do in a commit
or two, and verify.

---

## 1. Open — correctness

- **G2 fails intermittently, cause unknown.** A worker thread's transcript
  comes back a strict PREFIX of the reference — truncated, not numerically
  different — and the run reports a thread mismatch at whatever row the
  truncation landed on. Seen twice under cold `make check` on a heavily
  loaded machine, once on this branch and once on `main`, so it is not new
  work. Not reproducible since: not by racing two `golden --threads`
  processes, not by `make -j2` on the two thread gates, and not over six
  cold `make check` runs with the capture-file name collision below still
  in place.
  What is ruled out: `TRANSCRIPT` is `TLS`, so threads are not sharing one
  `FILE *`; and `swehouse.c` has no file-scope mutable state. What is fixed
  but NOT shown to be the cause: `capture_open()` keyed its temp file on the
  thread id alone, so two concurrent `golden` processes shared
  `.golden_capture_0.tmp`..`_7.tmp` and `"w+b"` truncates. That is wrong
  regardless and now carries the pid.
  Left open deliberately. A gate that fails when nothing is wrong is worth
  more attention than a clean list.
  What has changed is that the next occurrence will say something. The
  report now classifies the mismatch — SHORT (the worker's bytes are a
  prefix, so output went missing), LONGER, or DIVERGED (same length,
  different bytes, the only one that is about the library) — prints the
  byte counts and the offset, compares what `ftell` reported against what
  `fread` returned so a short read is distinguishable from a short worker,
  and leaves every capture file on disk instead of deleting it. The old
  report printed one line of each transcript, and a truncated one prints as
  an empty "thread:" line, which reads exactly like a wrong value.

## 2. Open — performance, bit-exact

Nothing open. The three memos are in; see Closed.

What is left in this area is **the tidal-acceleration resolution at the top
of `calc_deltat`**, which the memo deliberately does not cover because it has
side effects: `swi_set_tid_acc()` writes `ctx->tid_acc` and the
missing-ephemeris-path note goes to `serr`. With the evaluation memoised
that resolution is most of what `calc_deltat` still costs. Caching it too
would mean keying on `epheflag`, `jpldenum`, `fidat[].sweph_denum` and
`is_tid_acc_manual`, and writing `ctx->tid_acc` back on a hit to preserve
the side effect — a much larger key for a much weaker argument, against a
measured 2.7% for the part already done. Not proposed; recorded so the
question is not reopened without a number attached.

## 3. Open — hygiene, bit-exact by construction

Nothing open; see Closed.

## 4. Open — coverage

The transcript is the only thing standing behind every "no-op" claim in
this file, and it does not reach as much as its 12,713 rows suggest.
Measured with gcov on the golden run, worst first: **`swephlib.c` 74.6%**
(was 62.3%), **`swehouse.c` 68.4%** (was 57.6%), **`swecl.c` 70.0%** (was
63.9%), `swejpl.c` 67.3%, `swedate.c` 71.1%, `sweph.c` 74.8%.

Three bugs and one unreachable branch came out of closing the first part of
that gap — see Closed — and every one was in code no gate ran. That is the
argument for the rest of it. Functions still at zero:

Nothing at zero that is reachable. `meff` and `load_dpsi_deps` are covered;
`Airmass` turned out not to be a coverage item at all — see Not doing.

**The pattern worth remembering:** three separate cache-key bugs have come
out of this work — `swi_check_nutation`, `calc_deltat`'s table, and
`swi_check_ecliptic` — and every one was a cache whose key omitted something
its result depended on, found by making unreached code run. If another
`swi_check_*` or memo turns up, check its key before anything else.

---

## Not doing — and why

- **Deduplicating the copied blocks in `swehouse.c`, `swecl.c` and
  `sweph.c`.** This list carried them as section 4 for three rollups. The
  measurement that closes them: **the gates cannot verify the refactor.**
  gcov on the transcript, per copy —

  | file | block | covered |
  |---|---|---|
  | `swecl.c` | 5× at L1805–1940 | **0 of 5** |
  | `swecl.c` | 4× `rsminusrm` | 2 of 4 |
  | `swecl.c` | 4× at L2326 | 3 of 4 |
  | `swehouse.c` | 4× at L1121 | 3 of 4 |

  Merging copies the transcript cannot tell apart is not "provable on G1/G8",
  which was the entire justification. And the reward is structurally zero:
  the best available outcome is that nothing observable happens. Against
  that, this is a fork tracking upstream, so restructuring shared internals
  buys permanent merge-conflict surface, and the "a fix has to be applied to
  all copies" cost is one the fork does not pay — it is not fixing eclipse
  maths. If the copies ever need to be merged, coverage comes first.

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
- **Covering `Airmass()` (swehel.c).** It is not reachable. `Deltam()`
  selects between the detailed extinction chain and the single lumped
  airmass on `if (staticAirmass == 0)`, and `staticAirmass` is
  `#define staticAirmass 0` — a compile-time constant, with a note beside it
  saying to use 1 instead "depending on difference k's". So it is a
  configuration alternative a maintainer flips in source, in the same class
  as `SunRA()` under `SIMULATE_VICTORVB` below, and `3b9d090` deliberately
  kept that kind of thing. Listed here because it spent a rollup on the
  coverage list described as a helflag nothing selects, which was wrong.
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
| `calc_deltat` recomputed at an instant just computed — 135,204 calls in the heliacal benchmark, 80.6% of them repeats | four-slot memo between the tidal-acceleration resolution (side effects, must always run) and the model evaluation (pure in `tjd`, `deltat_model`, `tid_acc`). `deltat_aa` calls 422,842 → 80,362; benchmark 1.50 s → 1.46 s at n=40, **2.7%** — the resolution is now the bulk of what is left. Nine early returns became one exit so the store cannot be missed |
| Nothing exercised any delta-t model but the default, and no gate could reach the `dt[]` table changing under a live memo | `cov:deltatmodel[1..5]` at one instant (year 1000, where the models still disagree); **G18** `check-dtmemo`, two contexts asking the same three questions in different orders against a fixture `swe_deltat.txt`. Both proven by reintroducing the defect |
| `heliacal_ut_arc_vis()` declared `double xaz[2]` and handed it to `swe_azalt()`, which writes three — an 8-byte stack overflow on all four calls, straight from `swe_heliacal_ut()` with any `SE_HELFLAG_AVKIND_*` flag | `xaz[3]`. Every other caller in the tree already declared `[3]` or `[6]`. Found under ASan once the AVKIND branch had coverage at all |
| `find_conjunct_sun()` indexed its 18-entry `tcon[]` at `ipl * 2` with no bound, and `ipl` comes from `DeterObject()`, which maps a numeric object name to `SE_AST_OFFSET + n` — so `swe_heliacal_ut(..., "433", ...)` read `tcon[20866]`: undefined, build-dependent, SEGV when unmapped | bounds check, and a refusal saying no conjunction epoch is tabulated. No supported object's path changes; `hel_asteroid[433]` pins it |
| `swe_set_ephe_fallback()` — the switch for this fork's defining behavioural break — was called by no test at all, only the `SE_EPHE_FALLBACK` env var by G8, and could have been a no-op with every gate green | `cov:ephe_fallback[...]`, five rows pinning the default, the refusal, the switch, the substitution and the restore. Proven against both a no-op setter and a flipped default |
| The AVKIND heliacal strategy (`heliacal_ut_arc_vis`, `moon_event_arc_vis`) had zero coverage, so the "half these calls are duplicates" analysis was about code no gate ran | `hel_avkind[...]`; 0% → 67.5% and 76.8% |
| `swe_orbit_max_min_true_distance` and the `osc_iterate_min/max_dist` it reaches: public API, never called | `cov:orbit_max_min[...]`; 0% → 100% |
| `meff()` — the solar mass-distribution term that keeps gravitational deflection finite when a planet sits behind the Sun's disc — had never run | `cov:meff[...]`, the deepest conjunction each of five planets makes in 2000–2030, found by scanning elongation; enters `meff` at r = 0.021, 0.134, 0.328, 0.351, 0.427. Make it return 1 (the point-mass formula) and all five rows move |
| `load_dpsi_deps()` (63 lines), the IERS Earth-orientation loader behind `SEFLG_JPLHOR`, unreachable without both a DE ≥ 403 file and the EOP data, neither of them shipped | **G19** `check-eopload`: synthesises a ~20 KB self-consistent JPL header carrying `numde = 431` plus an EOPC04 fixture, then checks white-box that the loader found the file, parsed the right columns and recorded the span. Read the wrong column, or never report success, and it fails |
| The eleven precession models and thirteen exported conversion helpers, all at zero — `precess_2` alone 76 lines, the Owen 1990 chain another 78 | `cov:precmodel[1..11]` at -3000 (near J2000 every model takes the same short-term branch), `cov:conversions`. `swephlib.c` 62.3% → 74.6% |
| `swi_check_ecliptic()` keyed the obliquity on tjd alone — `oec` on `teps != tjd`, `oec2000` on `teps != J2000` — with no precession model in either, so switching `SE_MODEL_PREC_*` and re-asking about a computed instant returned the old model's obliquity | `swi_invalidate_models()` clears both. Third cache-key bug of the same shape, after `swi_check_nutation` and `calc_deltat`'s table |
| `get_acronychal_day()` and `azalt_cart()` unreachable — 107 lines behind a branch that cannot be entered | removed; the guard now returns ERR rather than falling through, so a wrong reachability argument surfaces instead of silently answering an acronychal question with a heliacal result |
| The three sidereal house branches, the Makransky sunshine solution, the interpolated lunar apsides, and the solar-system-plane sidereal projection: all at zero, each reachable with one call | `cov:hsys_sid[...]`, `hsys_makransky[...]`, `cov:intp_apsides[...]`, `cov:sid_ssy_plane`. `swehouse.c` 57.6% → 68.4%, `swecl.c` 63.9% → 70.0% |
| `check-threadshim` failed ~1 run in 12 under load: the publisher ran a fixed 20,000 iterations and could set `stop_readers` before a starved reader ran at all, tripping the test's own "test is vacuous" guard | publisher now also waits until every reader has observed something, with a 50× ceiling so a genuinely blind reader still fails loudly. 96 concurrent runs on 12 cores, 0 failures; the ceiling stretched to 128,508 publishes at worst |
