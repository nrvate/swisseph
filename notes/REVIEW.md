# Swiss Ephemeris — Modernization & Performance Survey

**Status:** exploratory survey — documentation only, no code changed
**Repo:** `swisseph` @ `6d601cd` (branch `threadsafe`)
**Date:** 2026-08-22
**Scope:** all root-level `*.c`/`*.h` (excludes `windows/`, `setest/`, generated `.o`/`.a`/`.so`)

---

## 0. Executive summary

libswe is ~35,000 lines of C across 12 translation units, largely unchanged in
structure since the 1990s (K&R holdouts still exist in `swephgen4.c`). It
still compiles and runs correctly — this is a genuinely well-tested piece of
scientific software — but its C idioms, build configuration, and public API
predate essentially every safety and ergonomics improvement the language and
its tooling have made since. This survey is orthogonal to
[INVESTIGATION.md](INVESTIGATION.md)/[PLAN.md](PLAN.md) (thread-safety); a few
items below intersect with that work and are noted as such, but are not
re-litigated here.

Two findings are more than style complaints and worth flagging up front:

1. **Likely out-of-bounds write in the JPL ephemeris reader.**
   `swejpl.c`'s `struct jpl_save` allocates `double buf[1500]` and
   `pc/vc/ac/jc[18]`-sized arrays, but the values that size these — `ncoeffs`
   (derived from the file's `ksize`, bounded only to `1000..5000`, i.e.
   `ncoeffs` up to 2500) and `ncf` (read directly from the file's `ipt[]`
   header, unbounded) — are trusted from the `.eph`/`.bsp` file itself with no
   check against the actual array capacities. A malformed or unusually large
   JPL ephemeris file can overflow both buffers. See §5, finding J1.
   **✅ Fixed — `04bb51a`** (`C17_MIGRATION.md`'s Phase 4 drafting caught that
   its own worked-example `static_assert` didn't compile, which is what
   surfaced this; `buf[]` widened to a named, ksize-tied constant, and `ncf`
   given a real runtime bound check).
2. **A dead branch in `get_builtin_star()` that looks like a real behavioral
   bug**, not just dead code: the true-Sheoran sidereal-mode case is
   unreachable and silently falls through to the Pushya case, returning the
   wrong star record. See §3, finding S1.
   **Downgraded, then fixed.** `PLAN.md` §14.5 checked: both branches return
   a byte-identical record, so the fallthrough returned the *same* data, not
   wrong data — dead code, not a correctness bug. Now deleted; the
   SHEORAN→Pushya mapping it documented is recorded in the surviving
   branch's comment. G1 stayed bit-exact, as unreachable code must.

Beyond those two, the dominant themes across every file are:

| Theme | Scale | Where |
|---|---|---|
| Unsafe string handling (`sprintf`/`strcpy`/`strcat`, 0 `snprintf`) | ~930 call sites repo-wide | everywhere, worst in `swetest.c` (153), `swevents.c` (62+), `sweph.c` (127 `strcpy` alone) |
| ~~No optimization flag in the shipped build~~ | **fixed on `threadsafe`**: `-std=c17 -Wall -Wextra -Werror -O2 -g -fPIC`, plus opt-in `-flto` | `Makefile` |
| ~~No CI~~ | **fixed on `threadsafe`**: 9 jobs — gcc/clang/macOS/MSVC, TSan+ASan+LSan, dialects, ABI, LTO, setest differential | `.github/workflows/ci.yml` |
| `#define`-only public constants, 0 enums | 330 macros, 0 `enum` | `swephexp.h` |
| `goto`-based control flow | 9 files, up to 115 in one file (`sweph.c`) | see per-section counts below |
| Dead / `#if 0`'d code | 47 regions, ~500 lines, across `swehel.c`, `swephlib.c`, `swecl.c`, `swehouse.c` | see per-section |
| Duplicated logic across near-identical branches | house-cusp iteration (6x), eclipse-max search (6x), JPL header parse (2x) | §4, §5, §6 |

> **Note on the dead-code row.** It is not merely untidy: on the `threadsafe`
> branch, three separate scripted edits landed *inside* `#if 0` blocks and
> silently did nothing — two generated shims that vanished from the ABI
> until the linker complained, and a `static_assert` that pinned nothing
> until a deliberately-false probe exposed it. All 47 regions were audited
> afterwards and none references the pre-Phase-3 API, so nothing is stale in
> a way that would break on re-enabling — but edits near them need checking,
> not trusting.

> **Clang `-Werror` findings, recorded but not enforced.** Sweeping the
> library with `clang-18 -Wall -Wextra -Werror` (stricter than any CI job —
> the clang jobs use `tests/CFLAGS`, which carries no `-Werror`) reports
> three pre-existing `-Wunused-but-set-parameter` in `swehel.c`:
> `OpticFactor`'s `JDNDaysUT` (:225), `SunRA`'s `helflag` (:554), and
> `Bcity`'s `Press` (:1263) — each assigned and then never read. Upstream
> code smells rather than bugs, and out of scope for the thread-safety
> branch; noted so the next person does not have to rediscover them.
>
> One from the same sweep *was* fixed: `sweephe4.c:673` declared
> `char *getenv();`, a K&R prototype conflicting with `stdlib.h`'s, which
> clang 18 rejects outright. That file is referenced by no Makefile target,
> so nothing built it — the same unreachable-is-unverified pattern this
> branch hit with `interp()` and `swethread.h` tier 5.

The rest of this document goes subsystem by subsystem. Performance findings
are included but are secondary to the modernization/maintainability focus
requested for this pass; where a finding is primarily about speed rather than
safety/clarity, it's marked **(perf)**.

---

## 1. Build system & public API

### 1.1 Build system

- **No optimization in the default build.** `Makefile:37,45`: `CFLAGS = -g
  -Wall -fPIC` on both the Linux and macOS branches — no `-O2`/`-O3` anywhere
  in the main build. `libswe.a`/`libswe.so` as currently built are debug
  binaries. `tests/Makefile:4` explicitly pins `-O0 -g`, which is appropriate
  for a test harness but underscores that there is no optimized-release path
  at all today.
- **No `-std=` in the main Makefile** — the library build floats on whatever
  the default C dialect of the local `cc` happens to be. `setest/Makefile:26`
  is the only place a standard is pinned (`-std=gnu99 -fms-extensions ...
  -pthread`), so different parts of the tree could silently build against
  different language dialects.
- **`-Wall` only** — no `-Wextra`, `-Werror`, no sanitizers wired into the
  default `make`/`make all`. `tests/` has `-fsanitize=thread` but only for its
  own `_tsan`-suffixed targets.
- **No CI.** No `.github/workflows`, no `.gitlab-ci.yml`, nothing — build and
  test verification is entirely manual today.
- **Hand-written Makefile**, OS-detected via a raw `uname` check
  (`Makefile:34`); no autotools/CMake.
- **`make test` is stale.** `Makefile:108,111` only drive the legacy
  `setest/` suite; the newer `tests/` harness (golden baseline + TSan races,
  see [INVESTIGATION.md](INVESTIGATION.md)) has its own `tests/Makefile` but
  is never invoked from the root Makefile — there's no single `make test`
  that runs everything.
- **Dead Makefile content**: `Makefile:118`'s dependency rule for
  `sweclips.o` references `sweclips.c`, which doesn't exist (the file is
  `swecl.c` — a stale rename artifact). `sweventss` (`Makefile:87-89`) is
  defined but omitted from `ALL_TARGETS` (`Makefile:65-69`), so it's silently
  unreachable via `make all`.

### 1.2 Public API shape (`swephexp.h`)

- **Output-by-raw-`double*`-array is the only calling convention.** All 85
  `ext_def(...)`-declared public functions that return multi-value results do
  so via untyped `double *xx`/`dret`/`cusps`/`ascmc`/... out-parameters, with
  array layout and required length documented only in prose, not enforced by
  the type system (e.g. `swephexp.h:822`,
  `swe_houses_armc_ex2(..., double *cusps, double *ascmc, ...)`). A caller
  passing an undersized buffer has no compiler or runtime signal until it
  corrupts adjacent memory.
- **Fixed 256-byte `char *serr` error buffer as the only error-reporting
  mechanism**, sized via `#define AS_MAXCH 256` (`sweodef.h:261`), filled by
  unchecked `sprintf`/`strcat` inside the library (see §1.3, and every
  section below). No error codes, no `enum`, no length-checked writes back
  into caller memory.
- **Zero enums on a 330-macro constant surface.** Every planet ID
  (`SE_SUN`/`SE_MOON`, `swephexp.h:101-102`), every flag bit
  (`SEFLG_*`, `swephexp.h:186-194`), every house-system code is a bare
  `#define`. Nothing stops a caller (or the library internally) from passing
  an out-of-range `int` where a planet or flag is expected — see §4 finding
  H1 for a concrete consequence (unrecognized house-system letters silently
  default to Placidus rather than erroring).
- **Minimal `const`-correctness**: only 5 `const char*`/`const double*`
  occurrences in `swephexp.h` against 72 plain `char *` — almost no read-only
  guarantees are expressed on the public surface.
- **Parallel, hand-duplicated Windows DLL header.** `swedll.h` re-declares
  the same 85+ signatures a second time with `DllImport`/`CALL_CONV_IMP`
  decoration instead of a single header driven by export macros — every
  signature change is a two-place edit, and they can silently drift apart.

### 1.3 Cross-cutting patterns (repo-wide greps)

| Pattern | Count | Notes |
|---|---|---|
| `sprintf(` | 370 (root `*.c`/`*.h`, excl. `windows/`) | 0 `snprintf(` anywhere |
| `strcpy(` | 379 | only 20 `strncpy(` |
| `strcat(` | 140 | |
| Fixed `char buf[N]` locals | 129 | 86 of them exactly `[AS_MAXCH]` (256 bytes) |
| `stdint.h`/`stdbool.h` usage | 0 | fixed-width ints hand-rolled in `sweodef.h:193-216` (`typedef int int32`, legacy 16-bit-compiler branches); no `bool`, `AS_BOOL` is `typedef int` |
| Duplicated AGPL/commercial license header | ~60-73 lines × 25 files | identical boilerplate, no SPDX identifiers, no shared include |

These aren't isolated incidents — they're the default idiom throughout the
codebase, which is the main reason this survey treats "unsafe string
handling" and "no enums / no fixed-width types" as top-level themes rather
than per-file findings.

---

## 2. Core engine — `sweph.c` / `sweph.h`

`sweph.c` is 8,618 lines and the busiest file in the tree (`swe_calc`, file
segment loading, fixed-star lookup, delta-T/IERS bulletin parsing). 115
`goto`s, 127 `strcpy`, 45 `sprintf`, 27 `strcat` in this file alone.

- **S1 — likely real bug, not just dead code:** `get_builtin_star()` at
  `sweph.c:6767-6775` (Pushya vs. "true Sheoran" ayanamsha) and again at
  `sweph.c:6795-6803` (galactic-equator cases) each have a second
  `else if` whose condition is already subsumed by the first — unreachable,
  and it returns the byte-identical star record as the first branch. This
  means `SE_SIDM_TRUE_SHEORAN` silently gets the Pushya star data instead of
  its own. Worth a follow-up look, since this changes actual output for a
  named sidereal mode rather than being cosmetic.
- Stale extern declarations `sweph.h:606-607`
  (`extern struct epsilon oec2000; extern struct epsilon oec;`) — no such
  bare symbols exist anywhere in the tree any more (`grep` confirms); the
  state now lives in `swed.oec`/`swed.oec2000`. Vestigial from an earlier
  refactor; would fail to link if anything still referenced it.
- Dead `#if 1 ... #endif` (always-true, no-op conditional) at
  `sweph.c:530, 781, 2335, 3023, 3751, 4221, 4777, 7475`.
- Goto-heavy fallback chains: `swe_calc`/`swecalc`
  (`sweph.c:400-950`) implement the JPL→SWISSEPH→MOSHIER ephemeris fallback
  independently for Moon, barycentric Sun, and each planet class, each with
  its own `goto return_error`/`goto sweph_*`/`goto moshier_*` labels
  (e.g. `sweph.c:680-725`, `758-808`). A shared helper would collapse four-plus
  near-identical state machines into one.
- Manual fixed-size array copy loop repeated **117 times**:
  `for (i = 0; i <= 5; i++) xx[i] = x[i];` (first occurrence
  `sweph.c:545-546,560-561`) instead of `memcpy` — more boilerplate and an
  off-by-one risk (`<=` bound) sitting in 117 places instead of one.
  `swe_version()` at `sweph.c:236` does an unbounded `strcpy(s, SE_VERSION)`
  into a caller buffer of undocumented size.
  Manual `strlen(serr) + N < AS_MAXCH` guards are hand-written before most
  `strcat`s instead of using `snprintf` (e.g. `sweph.c:686-687, 692-693,
  709-710, 2267-2268`).
- Magic byte offsets for IERS bulletin parsing in `load_dpsi_deps()`,
  `sweph.c:1435-1454`: `atoi(s + 7)`, `atof(s + 168)`, `atof(s + 178)`,
  `atof(s + 99)`, `atof(s + 118)` — fixed columns into an external file
  format, no named constants, no documentation of the layout; will silently
  misparse if the upstream bulletin format shifts a column.
- Minimal `const` on read-only params, e.g. `swi_fopen(int ifno, char *fname,
  char *ephepath, char *serr)` (`sweph.c:2366`) — neither string parameter is
  ever written.

**(perf)**
- File-handle churn on ephemeris-flag change: `sweph.c:389-403` — whenever
  `swed.last_epheflag != epheflag`, *every* open ephemeris file is closed and
  its `file_data` struct zeroed, forcing a full reopen + header re-parse on
  the next call. Costly for callers that alternate flags call-to-call.
- `swe_set_ephe_path()` (`sweph.c:1350`) unconditionally runs a full Moon
  `swe_calc()` just to learn the DE number/tidal acceleration, on every path
  set — including a redundant reset.
- `search_star_in_list()` (`sweph.c:6716-6721`) falls back to an O(n) linear
  scan for `'%'`-wildcard name search, sitting right next to the O(log n)
  `bsearch` used for exact names a few lines below (`6738-6741`).
- Worth noting as a **non-finding**: the Chebyshev segment cache in
  `main_planet`/`get_new_segment` (`sweph.c:2276-2299, 4370-4470`) and the
  ecliptic/nutation checks (`sweph.c:6016-6063`) already memoize correctly by
  `tjd` — no redundant recomputation found in the actual hot calc path.

---

## 3. Math library — `swephlib.c` / `swephlib.h`

4,642 lines of coordinate transforms, precession/nutation series, delta-T.
Pure-math, low external-state dependence — the best-suited part of the
codebase for unit tests, and currently has none.

- `swi_strcpy` (`swephlib.c:4547-4567`) hand-rolls `memmove` semantics via a
  stack buffer or `strdup`/`free` fallback, to support the overlapping-copy
  case used at `4202,4204` (`swi_strcpy(sp, sp+1)` to delete a char in
  place) — a single `memmove(to, from, strlen(from)+1)` replaces all of it.
- `swe_cs2timestr` (`3864`) / `swe_cs2lonlatstr` (`3888`) write into a
  caller's `char *a` at fixed offsets with no length parameter — buffer size
  is a documented convention only, not enforced.
- `static const char *fname_trace_c/fname_trace_out/fname_force_flg`
  defined directly in the **header** (`swephlib.h:184-188`) — every
  translation unit that includes it gets its own private copy; should be
  `extern` + defined once in a `.c` file.
- The arcsec→radian conversion `DEGTORAD/3600` is written out as a literal
  **42 times** (e.g. `907,915,921,923...`) despite `AS2R` already being
  defined locally for one block (`468`) — should be hoisted and reused
  everywhere.
- Long `if`/`else if` model-dispatch ladders repeating the same
  `SEFLG_JPLHOR`/`SEFLG_JPLHOR_APPROX` checks in nearly every function:
  `swi_epsiln` (`887-968`), `precess_1` (`1023-1135`), `swe_get_string`
  (`4245+`).
- Dead code retained via `#if 0`: four alternative Newcomb-precession
  formulas in `precess_1` (`1056-1132`), a duplicate `swi_cross_prod`
  (`655-662`).
- 11 `goto`s, several just for early-return short-circuiting rather than
  cleanup, e.g. `bessel()` (`2004-2067`) has 5 `goto done` labels that could
  be plain `return`s.
- Missing `const` beyond what INVESTIGATION.md already flags for
  `dcor_eps_jpl`/`dcor_ra_jpl`: `swi_coortrf`, `swi_cartpol`, `swi_polcart`,
  `swi_bias` all take non-`const double *` for read-only input arrays.

**(perf)**
- `calc_nutation_iau2000ab` calls `sin`/`cos` **directly per series term** —
  ~2,730 transcendental calls per invocation (678-term luni-solar loop
  `1861-1873` + 687-term planetary loop `1907-1928`). The neighboring
  `calc_nutation_iau1980` (`1615-1764`) does this correctly: it precomputes
  `sin`/`cos` of the 5 fundamental arguments once (`1676-1694`) and derives
  all higher multiples via angle-addition recurrence, with **zero**
  `sin`/`cos` calls inside the per-coefficient loop. The 2000A/B path is the
  clear outlier and the obvious target if this ever shows up in a profile.
- No caching of the Vondrák periodic-obliquity series across calls at the
  same epoch: `sidtime_long_term` (`3285-3324`) and `precess_2`
  (`1219-1326`, calls at `1253/1255/1315/1317`) each independently recompute
  the full 10-periodic + 4-polynomial series for `J2000` — a per-configuration
  constant, not per-call data.
- `sidtime_non_polynomial_part` (`3413-3450+`) recomputes a 14-argument, 33-term
  series with per-term `sin`/`cos` on every `swe_sidtime` call, with no
  memoization analogous to the day-window quadratic interpolation
  `swi_nutation` already uses (`swed.interpol.tjd_nut0/nut2`, `2130-2156`).
- `swe_gen_filename` (`3610+`) chains `strcpy`/`strcat`/`sprintf`
  (`3620-3686`), each re-scanning with `strlen` — minor, but recurs ~8 times.

---

## 4. Houses & eclipses — `swehouse.c` / `swecl.c`

3,148 + 6,428 lines. `swehouse.h` has **no include guard**
(`swehouse.h:1-99`) — trivial fix, currently relies entirely on callers only
including it once.

- **H1 — duplicated fixed-point iteration, 6 copies.** The same ~15-line
  "compute tangent → pole height → cusp → check convergence, iterate up to
  `niter_max=100`" block is repeated nearly verbatim for Gauquelin
  quadrants (`swehouse.c:1655-1723`, 2 copies) and Placidus houses
  11/12/2/3 (`1856-1987`, 4 copies), varying only a divisor and cusp index.
  A single `converge_house_cusp(rectasc, divisor, ...)` helper would replace
  all six.
- **H2 — duplicated polar-circle handling, 5 copies.** The "within polar
  circle → swap AC/DC, add 180°" ~10-line block recurs at `1076-1086`
  (Campanus), `1131-1145` (Horizon), `1239-1253` (Savard-J),
  `1824-1832` (Placidus-default), and a variant near `1633-1677`
  (Gauquelin).
- Heavy `goto` use: 10 `goto porphyry;` fallback jumps in `swehouse.c`'s
  `CalcH()` switch (e.g. `1183, 1259, 1637, 1676, 1718, 1839, 1873, 1909,
  1945, 1981`); `swecl.c` has **44** goto-driven labels
  (`goto next_try`/`goto next_lun_ecl`/`goto end_search_global`, e.g.
  `1246,1314,1339,1362,1676,1819,3677`) implementing the eclipse-search state
  machines — this is the hardest-to-follow control flow found in the whole
  survey.
- Unbounded interpolation: `swecl.c:1618` —
  `sprintf(serr, "annular occulation do not exist for object %d %s\n", ipl,
  starname)` interpolates a caller-supplied star name of unbounded length
  into a 256-byte buffer with no guard — a real (if obscure) overflow path.
- Deprecated lower-case house-system letters are uppercased in place via
  arithmetic (`hsy = (char)(hsy - 32)`, `swehouse.c:993-997`) rather than a
  lookup/enum; unrecognized codes silently fall through to Placidus
  (`default:` at `1835`) instead of erroring — a direct consequence of the
  `#define`-only constant surface noted in §1.2.
- Dead/commented-out code: `swehouse.c:2926,3062` (a commented-out `strcpy`
  warning), `swecl.c:4261-4270` (`#if 0`'d dead branch in `rise_set_fast`),
  `swehouse.c:112-114` (`#if 0` unused test function).
- Unnamed magic numbers: `niter_max = 100` (`945`); hardcoded
  `sqrt(3.0)`/`1/sqrt(3.0)` recomputed at runtime for 30°/60° tangents
  (`1041-1060,1102,1114,1117`) instead of literal constants; hardcoded rise/set
  Newton-step clamp `±0.1` (`4307-4310`); Earth radius `6378.140`
  (`1190`) as a bare local instead of a shared named constant.

**(perf)**
- Eclipse/occultation maximum-finding loops call `swe_calc` **4 times per
  sample** (once equatorial, once Cartesian, for both Sun and Moon) where the
  Cartesian values could be derived from the already-computed equatorial ones
  via a cheap `swi_polcart` conversion instead of a second full ephemeris
  evaluation. This pattern (`swecl.c:1277-1300` and 5+ near-duplicates at
  `1698, 2167, 2500, 3487, 4528/4546`) combined with the shrinking-`dt`
  refinement loop multiplies to ~90-100 ephemeris calls per eclipse-max
  search — and the surrounding loop itself is copy-pasted 6+ times, so any
  fix has to be applied in that many places.
- `rise_set_fast()` (`4302-4306`) numerically differentiates altitude via a
  second `swe_azalt()` call at `t+0.001` on every Newton step, purely to
  estimate `d(altitude)/dt`, when a closed-form spherical-trig expression
  exists and would remove the extra call entirely.
- Triple sequential, unconditional ET↔UT convergence at `swecl.c:1301-1303`
  (`tjds = tjd - swe_deltat_ex(...)` three times in a row) with no early-exit
  check even when the first pass has already converged.
- Gauquelin's 16 sectors (`1628-1735`) each independently re-run the full
  100-iteration convergence from scratch, though several intermediate
  quantities are latitude/obliquity-only and constant across all 16.

---

## 5. Ephemeris models — `swemplan.c` / `swemmoon.c` / `swejpl.c` / `swemptab.h`

- **J1 — likely out-of-bounds write (see §0).** `swejpl.c:108`:
  `struct jpl_save` declares `double buf[1500]`, `pc/vc/ac/jc[18]`.
  `fsizer()` (`189-328`) bounds `ksize` only to `1000..5000`
  (`322-326`), giving `ncoeffs = ksize/2` up to **2500** — but `state()`
  (`806-814`) fills `buf[k-1]` for `k` up to `ncoeffs`, overflowing `buf[1500]`
  for any file with `ksize > 3000`. Separately, `interp()` (`523-589`) indexes
  `pc/vc/ac/jc[]` up to `ncf-1` where `ncf` is read directly from the file's
  `ipt[]` header with **no check that `ncf <= 18`**. Both trust the ephemeris
  file's self-reported sizes against fixed-capacity stack/struct arrays — a
  classic 1990s "we control the file format" assumption that no longer holds
  once arbitrary `.eph`/`.bsp` files can reach this code path.
- Header parsed twice: `fsizer()` (`189-272`) and `state()`'s init block
  (`668-730`) read the same title/cnam/ss/ncon/au/emrat/ipt/denum/lpt fields
  from disk via near-identical `fread`+`reorder` sequences (~130 duplicated
  lines, and the header is physically read from disk twice per file open).
- 27 `sprintf` calls building error strings across the three files, with
  inconsistent length-guarding — some sites guard the `strcat` length
  (`swemplan.c:303`, `swemmoon.c:885/1509/1582`), most don't
  (`swejpl.c:756,790,803`).
- **Dead code: `MOSH_MOON_200` is never defined anywhere in the tree**
  (confirmed via grep across all build files/headers), yet gates ~600 lines
  of an alternate DE200-fit lunar theory in `swemmoon.c`
  (`198-314, 445-594, 936-1365, 1779-1811`) — pure bloat unless it's meant to
  stay as reference material, which should be documented if so.
- `read_elements_file()` (`swemplan.c:694-913`) uses `goto return_err` with
  duplicated `fclose(fp)` at each label, and a fragile
  `if (atof(sp) != 0 || *sp == '0')` heuristic (`958`) to distinguish "value
  is legitimately zero" from "parse failed."
- `mods3600` is defined twice independently — a macro in `swemplan.c:69` and
  an equivalent function in `swemmoon.c:1734-1740` — a fix to one is easy to
  miss applying to the other.
- Inconsistent coefficient-table density: `swemmoon.c`'s `LR`/`MB`/`LRT`/`BT`
  tables (e.g. `319-442`) quantize amplitudes as `short` pairs (4
  bytes/term), while `swemptab.h`'s ~45 `plantbl` arrays (e.g. `mertabl` at
  `80`) store every coefficient as a full `double` literal (8 bytes) — a 2x
  memory/cache-footprint difference between two series-table styles in the
  same codebase with no evident precision justification.

**(perf)**
- **Per-double `fread()` in the hot record-read loop.** `swejpl.c:806-814`
  issues up to ~1,000 separate `fread()`+`reorder()` calls per 32/64-day
  record instead of one bulk `fread(buf, sizeof(double), ncoeffs, fp)`
  followed by a single reorder pass. This runs on every cache-miss record
  load — the single largest, easiest win identified in this survey. The
  correct bulk-read technique already exists 80 lines away in the same file
  (the constants block at `724`), it just wasn't applied here.
- No caching of parsed fictitious-planet elements: `read_elements_file()`
  reopens `SE_FICTFILE` and re-scans/tokenizes it on **every** call for a
  fictitious body (comment at `swemplan.c:706` explicitly notes file info is
  never saved) — costly for any workload computing one fictitious planet
  across many dates.
- `swi_moshplan`/`swi_moshmoon` (`swemplan.c:309-335`,
  `swemmoon.c:869-934`) finite-difference velocity by fully re-evaluating the
  entire series at `tjd ± speed_intv` — inherent to the numerical-derivative
  approach, but no slowly-varying intermediate terms are reused between the
  two nearby evaluations.
- Worth noting as a **non-finding**: `interp()`'s Chebyshev coefficient cache
  (`swejpl.c:472-591`) is correctly memoized by normalized Chebyshev time —
  not a finding, just flagging it's already handled (and, per
  [INVESTIGATION.md](INVESTIGATION.md), the TLS statics behind it mean
  threads can't share that cache — a thread-safety-adjacent note, not new
  scope here).

---

## 6. Heliacal / events / date — `swehel.c`, `swevents.c`, `sweephe4.c`, `swephgen4.c`, `swedate.c`

- Unbounded copy from CLI input: `swevents.c:389,456` —
  `char fname[80] = "de431.eph"; ... strcpy(fname, argv[i]+5);` for the
  `-ejpl<name>` flag, no length check on a stack buffer sized from a
  command-line argument. The same unguarded pattern recurs across the file;
  by contrast `swevents.c:579-581` shows the codebase already knows the safe
  idiom (explicit `strlen()` bound check) — it's just not applied
  consistently.
- **246 lines of dead `#if 0` code in `swehel.c`**, across 13 blocks
  (`243-250, 338-364, 1123-1146, 1419-1423, 2058-2075, 2305-2315, 2358-2366,
  2468-2499, 2526-2542, 2661-2695, 2697-2742, 3047-3051, 3067-3075`), plus 2
  runtime-dead `if ((0)) { ... }` blocks still remaining at `1902` and
  `3212` — the Phase 1.4 debug-block cleanup on this branch removed 3 other
  `if((0))` blocks but these two survived. `2468-2499` is a full duplicate
  old implementation (`get_asc_obl_old`) of the live `get_asc_obl_with_sun`.
- K&R-style (pre-ANSI) function definitions — the only two left in the
  codebase: `swephgen4.c:85` (`int split(w, m, min, sec)` with old-style
  param declarations below the signature) and `swephgen4.c:167-168`
  (`char *degstr (t) double t;`, which also returns a pointer into a
  `static char a[20]` — non-reentrant by construction).
- `goto`-heavy validation/rounding: `swevents.c` (21), `swehel.c` (25),
  `sweephe4.c` (6). `dms()` in `swevents.c:1801-1875` uses
  `goto again_dms` twice to redo rounding after a carry (`kmin==60`,
  `ksec==60`) — a plain loop would read more clearly.
- `sweephe4.c`/`swephgen4.c` include no standard headers directly (only
  `swephexp.h`/`sweephe4.h`) yet call `fopen`/`fseek`/`sprintf`/malloc-family
  functions — compiles only via transitive includes from project headers.

**(perf)**
- Redundant ephemeris recomputation inside the heliacal-visibility search.
  `ObjectLoc()` (`swehel.c:683-729`) recomputes everything from scratch with
  no memoization; `swe_heliacal_pheno_ut` calls it 5 times back-to-back for
  the same instant (`1871-1879`), and the visibility search loop
  (`1957-1999`, up to 240 one-minute steps) calls it directly plus via
  `DeterTAV()` (`1741-1763`), which itself calls `ObjectLoc` 5 more times for
  the same `TimePointer` — sun and object alt/azi each end up computed twice
  per iteration. A single search can issue on the order of 1,500-3,000
  `swe_calc`/`swe_azalt` calls, roughly half of them exact duplicates within
  the same iteration.
- `pow(x, 2)` used for plain squaring in that same hot loop (`swehel.c:280-283`,
  10 of the file's 36 `pow()` calls) — `x*x` avoids the transcendental-function
  path for something called hundreds to thousands of times per search.
- `sweephe4.c`'s `eph4_posit()` (`455-497`) does correctly cache the open
  file by number, but re-does `sprintf`+`fopen`+`fclose` on every boundary
  crossing with no buffering — fine for occasional use, a cost for tight
  sequential date scans.

---

## 7. Suggested next steps

This is a survey, not a plan — no priority ordering or effort estimates are
implied beyond what's obvious. If a follow-up plan gets written (in the style
of [PLAN.md](PLAN.md)), the two items in §0 (J1 buffer sizing, S1 Sheoran
branch) are the only ones here that look like correctness bugs rather than
maintainability debt, and would be the natural Phase 0 for a modernization
effort — everything else is either a systemic idiom (unsafe strings, no
enums, goto-heavy control flow) best tackled as a swept, mechanical pass, or
a localized duplication (house-cusp iteration, eclipse-max search, JPL header
parse) best tackled as a targeted refactor once test coverage exists to
verify bit-for-bit output equivalence — the same golden-baseline approach
`tests/` already uses for the thread-safety work would apply directly here.
