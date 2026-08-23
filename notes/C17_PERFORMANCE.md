# Plan: C17 Performance & Code-Quality Follow-On

**Status:** not started — depends on [`C17_MIGRATION.md`](C17_MIGRATION.md) landing first (see §3)
**Companion to:** [`C17_MIGRATION.md`](C17_MIGRATION.md) (the mechanical build/typedef migration this
plan builds on), [`REVIEW.md`](REVIEW.md) (general modernization survey), [`PLAN.md`](PLAN.md)
(thread-safety architecture — §4.2 and §4.3 below intersect with its Phase 3)

---

## 0. Executive summary

`C17_MIGRATION.md` is about making the *codebase's language dialect* official
and safe (delete dead compiler branches, guarantee `stdint.h`/`stdbool.h`
availability, flip the build flags). This plan is the different question it
deliberately didn't ask: **once C17 is guaranteed, what do we actually do with
it?** Five feature areas were surveyed — `restrict`, cache/alignment,
`<stdatomic.h>`, `const`-correctness/LTO, and macro→inline/`static_assert` —
each grounded in real file:line evidence, not generic advice.

Ranked by payoff (detail in §4-§5):

1. **`restrict` on `interp()`** (`swejpl.c:472`) — the single hottest function
   in the ephemeris-reading path, verified alias-free at every call site.
2. **`const` + LTO** — a concrete, measured case: `swi_coortrf2` alone has 64
   cross-file call sites; LTO is the only way `-O2` can inline across those
   TU boundaries, and this is a 9-object-file shared library, so LTO's usual
   compile-time cost is a non-issue here.
3. **A real, if latent, bug found while looking for `static_assert`
   candidates**, unrelated to performance: `sweph.c:163`'s `pnoext2int[]` was
   sized for 21 entries but indexed against `SE_NPLANETS` (23). Was masked
   by branch ordering. **✅ Fixed, `0a7fc6d`** — see §4.6, B1.
4. **`plan_data`/`save_positions` cache-line alignment** — real, quantified
   false-sharing risk (416 B and 216 B strides, neither a multiple of 64),
   but should be designed *together with* `PLAN.md` Phase 3's `swe_ctx`
   layout, not standalone — see §4.2.
5. Smaller code-quality wins: macro→inline conversions (one genuine
   double-evaluation risk, one 3-file copy-pasted macro), and `swethread.h`
   losing ~46 of its 182 lines once C89 support is no longer a constraint.

## 1. Goals

1. Identify every place a C11/C17 (or already-available-but-unenforced C99)
   language feature would produce **measurably better codegen**, not just
   nicer-looking code — prioritized per the request that motivated this doc.
2. Where a feature is about code quality rather than speed, still capture it
   (macro hygiene, `static_assert` documentation of invariants) — but keep it
   visibly secondary to the performance findings.
3. Every performance claim in this document is falsifiable: verified against
   real call sites (no-alias proven, not assumed) and, before anything here
   ships, against a **before/after benchmark**, not just `tests/golden`'s
   numerical-no-op check. See §6 — this is the first of the four `notes/*.md`
   plans that actually claims speed, so it needs a way to prove it.

## 2. Non-goals

- No algorithmic changes — same boundary `PLAN.md` and `C17_MIGRATION.md`
  draw. Everything here is "same computation, different codegen."
- No blind `restrict` annotation. Every candidate below was checked against
  its actual call sites; several plausible-looking functions are explicitly
  **excluded** because they're genuinely called with aliased pointers today
  (`swi_cartpol`/`swi_polcart`, `swi_coortrf`/`swi_coortrf2`) — annotating
  those would be undefined behavior, not an optimization.
- Not a replacement for profiling. This is a survey of *opportunities* the
  language now permits; it is not a claim that any of them is the actual
  bottleneck in real workloads. Benchmark before prioritizing further.

## 3. Prerequisite: why this waits for `C17_MIGRATION.md`

`restrict` is technically C99 and already usable today (the tree compiles
clean at C99+); `static_assert`/`_Alignas`/`<stdatomic.h>` need C11. The
reason none of this is safe to land yet isn't the language feature — it's
that **the build pins no `-std=`** (`C17_MIGRATION.md` §1.1), so whether any
of these features are available depends on whatever the local `cc` defaults
to. Landing `restrict` annotations or `static_assert`s today means they
silently vanish (or fail to compile) on a toolchain with an older default.
`C17_MIGRATION.md` Phase 5 is what makes "C11/C17 is available" a build-wide
guarantee instead of a per-toolchain accident — that's the actual dependency,
not the language version number itself.

---

## 4. Findings by feature

### 4.1 `restrict` — hot numerical pointers (highest priority: real perf)

Searched every function in `swephlib.c`, `swemplan.c`, `swemmoon.c`,
`swejpl.c`, `sweph.c` taking 2+ non-`const double *` params (the only shape
where `restrict` changes anything), and traced every call site to confirm
the pointers are never the same array in practice.

**Verified safe to annotate:**

| Function | Location | Why it matters |
|---|---|---|
| `interp()` | `swejpl.c:497` | **Headline candidate — but see the warning below.** Every JPL planet/nutation/libration lookup goes through this; `buf`/`pv` verified distinct at all call sites (`swejpl.c:824,831,844,849`). Inner loop is `ncf`(~10-15 terms) × `ncm`(2-3) × up to 4 derivative orders — the hottest loop in the file. |
| `swi_cross_prod()` | `swephlib.c:160` | All 5 call sites keep output separate from inputs. Small body, but genuinely alias-free. |
| `swi_trop_ra2sid_lon_sosy()` | `sweph.c:3335` | All 7 call sites pass distinct `xin`/`xout`. **Do not confuse with its sibling below.** |
| `swi_osc_el_plan()` | `swemplan.c:579` | `xp` accumulates from `xearth`/`xsun`, never aliased with either. |
| `embofs()` | `sweph.c:5089` | Called with distinct plan-data arrays every Earth-position call — payoff is call frequency, not loop size. |
| `calc_center_body()` | `sweph.c:2469` | `xcom` never equals `xx` at either call site. |
| `denormalize_positions()`/`calc_speed()` | `sweph.c:6010,6026` | Three genuinely distinct arrays, but this is the `SFLG_SPEED3` slow-path explicitly commented "for test only" — low real-world payoff. |

> ### ⚠ `interp()` is unreachable in this checkout, and therefore untested
>
> `interp()` is only reached through `swi_pleph()`, which `jplplan()` calls
> only once a JPL `.eph` file is open. This repository ships `.se1` files
> and no `.eph` at all, so:
>
> * **`interp()` executes zero times** in `tests/bench` *and* in the entire
>   5137-row `tests/golden` suite. Measured, not inferred: a `fprintf` at
>   the top of the function fires 0 times for both.
> * The `restrict` annotation therefore **cannot be shown to speed anything
>   up here**, and no such claim is made. It was applied because it is
>   correct and benefits users who do supply `de431.eph` — not on measured
>   evidence.
> * More seriously: **the whole JPL reading path has no numerical
>   regression coverage.** G1's bit-exactness says nothing about it, and
>   `tests/jplguard.c` only exercises header *rejection*. Phase 3c
>   restructured `interp()` and `state()` — moving seven statics into
>   `struct jpl_save` — with G1 green throughout and not one line of that
>   code ever executed.
>
> This gap is tracked as G10 in PLAN.md; it wants a synthetic `.eph`
> fixture that actually computes, extending what `jplguard.c` already does
> for headers.

**Explicitly excluded — confirmed aliased in real use, `restrict` would be UB:**

- **`swi_cartpol`/`swi_polcart`/`swi_cartpol_sp`/`swi_polcart_sp`**
  (`swephlib.c:314,343,362,420`) — the doc comment says in-place (`x == l`)
  is allowed, and it's actually used that way (`swephlib.c:236`,
  `swemmoon.c:1724`).
- **`swi_coortrf`/`swi_coortrf2`** (`swephlib.c:279,299`) — called in-place
  at `swephlib.c:235,263-264`, `swemplan.c:317,327,351,368`,
  `sweph.c:3306,3314,3354-3355`. Both already defend against this internally
  with a local temp — which is exactly why callers rely on calling them
  in-place. Do not touch.
- **`swi_trop_ra2sid_lon()`** *(not* the `_sosy` variant above*)* —
  `sweph.c:6646,7865` call it as `(xxsv, x, xxsv, iflag)`, aliasing input and
  output. Currently safe only because the function copies to a local before
  writing. **Needs a real fix (copy-then-restrict) before this one could be
  annotated, not a quick win.**

**Not candidates:** `swi_echeb`/`swi_edcheb`/`swi_precess`/`swi_ldp_peps`
(single non-const pointer — nothing to disambiguate, or the accumulation
pattern means `restrict` wouldn't change codegen anyway), `swi_moshplan2`,
`chewm` (inputs are already a different type — the compiler gets non-alias
for free via strict aliasing).

### 4.2 Cache layout / alignment (real finding, but coordinate with `PLAN.md` Phase 3)

- **Per-planet coefficient tables are already L1-resident — not a finding.**
  Verified by counting actual array sizes: the largest single-planet table
  set (Saturn, `swemptab.h:5743,6232,6721`) is 18.9 KB; all 9 planets summed
  is 105.5 KB, but one `swe_calc()` call only ever touches one planet's ~19
  KB at most. Well within any modern L1D. The Moon's perturbation tables
  (`swemmoon.c:318-703`) total ~4.0 KB. No cache-miss problem here.
- **`REVIEW.md`'s "short vs double" table-density finding is a footprint
  claim, not a cache-locality one — now verified, not just re-asserted.**
  Both `swi_moshplan2` (`swemplan.c:134-262`) and `chewm()`
  (`swemmoon.c:1628`) walk their tables in a single strictly sequential
  pass. Given the working set already fits in L1D (above), halving element
  width wouldn't reduce miss count — it only affects `.rodata` size / binary
  footprint, not per-call cache behavior. Correcting the record on that
  finding.
- **Real finding: `struct plan_data` (416 B, `sweph.h:610-656`) and
  `struct save_positions` (216 B, `sweph.h:733-745`) are not multiples of
  64.** `pldat[SEI_NPLANETS=18]` (7,488 B total) and `savedat[24]` (5,184 B
  total) — both read/compared on every `swe_calc()` call — have each
  element's tail sharing a cache line with the next element's head
  (416/64 = 6.5, 216/64 = 3.375). `_Alignas(64)` on these structs rounds the
  stride up (448 B / 256 B) and removes the overlap. This matters if
  `PLAN.md` Phase 3 ever has different threads/contexts computing different
  planets against slots in structurally similar arrays — **which is exactly
  what Phase 3's `swe_ctx` redesign is already doing.** Recommend folding
  this into that redesign rather than doing it standalone and then redoing
  it again when Phase 3 restructures the same data.
- **Minor, incidental finding: `node_data` (`sweph.h:747-759`, 264 B) is dead
  code.** `nddat` is declared as the heavier `plan_data` (416 B) via an
  `#if 0`/`#else` (`sweph.h:842-844`) that discards the leaner struct — 6
  lunar-node/apsides entries carry 912 B of unused fields. Worth a one-line
  fix if `PLAN.md` Phase 3 is touching this file anyway (it is — Phase 3a's
  mechanical `swed.`→`ctx->` pass hits `sweph.c` regardless).
- **Non-finding, stated plainly:** `struct swe_data` itself (23,048 B) is
  not an alignment target — Phase 3 makes it heap-allocated per `swe_ctx`,
  not stack/array-packed, so false sharing *between* whole contexts isn't a
  real concern regardless of alignment.

> ### `_Alignas(64)` is DECLINED — measured, and the premise does not hold
>
> The false-sharing argument requires two threads writing to the same cache
> line. Under **both** models in this library that cannot happen: `swed` is
> TLS, so each thread's `pldat[]`/`savedat[]` live in its own TLS block, and
> an explicit `swe_ctx` is separately heap-allocated and used by one thread
> at a time by contract. No two threads ever touch adjacent elements of the
> same array.
>
> Measured anyway rather than argued: 8-thread `golden`, three interleaved
> pairs — 1.766 s plain vs 1.734 s aligned, −1.8%, inside the noise.
>
> The cost is real: `plan_data` 416 → 448 B and `save_positions` 216 → 256 B
> grows every context from 34,800 to 36,544 B, **+5%**. Paying 5% memory per
> context for a within-noise timing difference on a premise that does not
> apply is a bad trade.
>
> ### `nddat` as `node_data` is DECLINED
>
> Every `nddat` access does touch only `node_data`'s five members
> (`iephe`, `teval`, `x`, `xflgs`, `xreturn`) — verified across all pointer
> aliases, not just direct subscripts. But `app_pos_rest()` is shared: it
> takes `struct plan_data *` and is called with both `pldat[]` and `nddat[]`
> entries. Narrowing `nddat` means refactoring a hot shared helper to save
> 912 B out of 34,800 (2.6%), with no speed effect. Not worth the risk. The
> `#if 0` now carries a comment saying so, rather than looking abandoned.

### 4.3 Threading shim (`swethread.h`) — code-quality cleanup, no perf change

`swethread.h` is a 5-tier fallback (no-threads → Windows → GCC/Clang
`__atomic` builtins → C11 `<stdatomic.h>` → mutex-guarded last resort),
written specifically to survive being compiled as C89 today.

- **Tier 5** (`swethread.h:150-178`, the mutex-guarded fallback — the header's
  own comment calls it "always correct, just slower") **becomes genuinely
  unreachable** once C17 is guaranteed: its only job is catching "no C11
  atomics available," which can't happen anymore. **29 dead lines.**
- **Tier 3** (`swethread.h:105-121`, the `__atomic` builtins) keeps firing on
  gcc/clang but its stated purpose — surviving `-std=c89` — no longer
  applies; it becomes a preference, not a requirement.
- **Verified, not assumed: tier 3 and tier 4 (`<stdatomic.h>`) produce the
  same codegen on gcc/clang** — `atomic_load_explicit`/`atomic_fetch_add_explicit`
  are typically implemented in terms of the same `__atomic_*` builtins, same
  memory-order mapping. **No performance claim here — this is a pure
  simplification, correctly labeled as such rather than oversold.**
- Tiers 1 (`SWE_NO_THREADS`) and 2 (`_WIN32`/SRWLOCK) are gated on user
  intent and platform, not C-standard availability — **permanent, not
  touched by this plan.**
- **Don't forget `tests/threadshim.c:44-46`** — it re-implements tier 5's
  exact selector condition to decide whether to link
  `swi_gen_fallback_mutex`. Must be deleted in lockstep with `swethread.h`'s
  tier 5, or it defines an unused global once that tier is unreachable.
- No other hand-rolled atomics-detection cascade exists elsewhere in the
  tree (verified by grep) — this is the only site.

> ### ⚠ Tier 5 removal is DECLINED, and tier 5 was already broken
>
> This section proposes deleting tier 5 as "genuinely unreachable once C17
> is guaranteed". Two things came out of checking that.
>
> **1. Tier 5 did not build at all.** `swethread.h` declares
> `swi_gen_fallback_mutex` `extern` and calls it, and **no translation unit
> in the library ever defined it**. Any toolchain selecting tier 5 could not
> link the library. The fallback that exists to serve Sun Studio and IBM XL
> — both named in `sweodef.h`'s TLS block — was dead on arrival.
>
> Nothing noticed because gcc and clang pick tier 3 at **every** `-std=`,
> `c89` included; tiers 4 and 5 were built by nothing, ever. Same shape as
> `interp()` in §4.1: unreachable code is unverified code. Fixed, and G11
> (`tests/check-threadtiers`) now builds and runs all four selectable
> backends against the golden baseline.
>
> **2. `tests/threadshim.c`'s copy of the selector had drifted.** It omitted
> the `SWE_PREFER_C11_ATOMICS` term, so `-std=c99 -DSWE_PREFER_C11_ATOMICS`
> chose tier 5 in the header and tier 3 in the test, and the link failed.
> The header now exports `SWI_NEEDS_GEN_FALLBACK_MUTEX` so the condition
> exists in exactly one place — which is the durable fix for the "must be
> deleted in lockstep" hazard this section itself warns about.
>
> **The removal is declined.** `sweodef.h` explicitly supports Sun Studio
> and IBM XL, which are precisely the compilers that may offer neither the
> `__atomic` builtins nor C11 `<stdatomic.h>`. Deleting their only path to
> save 29 lines trades a hard build failure on a supported platform for
> nothing measurable. The tier is now correct and tested, which is worth
> more than it is short.

### 4.4 `const`-correctness (function parameters) + LTO

- **`PLAN.md` Phase 1.5 already const-qualified every file-scope coefficient
  table** (`swemptab.h`'s 27 `double[]` + 9 `signed char[]` arrays, plus
  five exported globals) — confirmed by ELF section move, writable `.data`
  in `libswe.so` dropped from 127,232 to 248 bytes. **Nothing left to do on
  the table-data front** — `swemmoon.c`'s tables were already `static
  const` before that commit. This is a "verified complete," not a gap.
- **What's actually left is function *parameters*, not tables.** New
  candidates beyond what `REVIEW.md` already listed (`swi_coortrf`,
  `swi_cartpol`, `swi_polcart`, `swi_bias`, `swi_fopen`):

  | Function | Location | Cross-TU call sites |
  |---|---|---|
  | `swi_coortrf2(double *xpo, ...)` | `swephlib.c:299` | **64** (sweph.c, swecl.c, swemplan.c, swemmoon.c, swehouse.c) |
  | `swi_edcheb(double x, double *coef, int ncf)` | `swephlib.c:190` | 1, but inside `main_planet`'s per-axis derivative loop (`sweph.c:2322`) — hot |
  | `swi_cross_prod(double *a, double *b, double *x)` | `swephlib.c:160` | internal |
  | `swi_dot_prod_unit(double *x, double *y)` | `swephlib.c:453` | internal + cross-file |
  | `swi_crc32(unsigned char *buf, int len)` | `swephlib.c:3754` | file-integrity checks |
  | `swi_strcpy(char *to, char *from)` | `swephlib.c:4556` | widely used |
  | `swi_cartpol_sp`/`swi_polcart_sp` | `swephlib.c:362,420` | position+speed conversions |

  (`swi_echeb`, `swephlib.c:171`, was checked too but has **zero live
  callers anywhere in the tree** — not a real-world example, skip it.)

- **LTO is not currently used anywhere** (`grep -rn "flto\|LTO"` across all
  three Makefiles: zero hits), and isn't yet in `C17_MIGRATION.md` Phase 5's
  planned `CFLAGS`. **Recommend adding `-flto` to that phase.** Concrete
  justification, not a generic "LTO is good" claim: `libswe.so` builds from
  exactly 9 object files (`SWEOBJ`) — small enough that LTO's usual
  compile-time cost is a non-issue — and the payoff is real: `swi_coortrf2`
  alone has 64 cross-TU call sites, `swi_polcart` 36, `swi_coortrf` 31,
  `swi_cartpol` 23, all small leaf math helpers called from hot loops in
  *other* translation units, which plain `-O2` cannot inline across but LTO
  can. Combined with the `const`-parameter fixes above (removing the
  "callee might alias/write" barrier), this unlocks constant-propagation
  through those calls too. `tests/Makefile`'s existing `check-golden-O2`
  bit-tolerance gate is already positioned to catch any codegen regression.

### 4.5 Macro → `static inline` function conversions (code quality, one real bug-risk)

| # | Macro | Location | Issue |
|---|---|---|---|
| A1 | `sind`/`cosd`/`tand`/`asind`/`acosd`/`atand`/`atan2d` | `swehouse.h:92-98` | No double-eval today, but un-type-checked and chained 4-5 deep (`swehouse.c:1646`); a `static inline double sind(double x)` catches type errors a macro can't, inlines identically under `-O2`. |
| A2 | `mods3600(x)` | `swemplan.c:69` | **`x` substituted twice** — the textbook `SQR(x++)` risk. Called safely today (`swemmoon.c:1768` etc., only plain arithmetic arguments), but the shape is live: a future `mods3600(read_next())` silently double-evaluates. |
| A3 | `square_sum(x)` | `sweph.h:308` | **Independently redefined three times**, byte-identical, in `sweph.h`, `swevents.c:242`, `swetest.c:691`. A single shared `static inline` fixes both the duplication and A2-style multi-eval risk at once. |
| A4 | `dot_prod(x,y)` | `sweph.h:309` | Same shape as A3, each argument substituted 3 times. |
| A5 | `degtocs`/`cstodeg` | `swehouse.h:89-90` | Defined but **never called anywhere** in the shipped `.c` files — dead, delete or convert if resurrected. |

### 4.6 `static_assert` candidates (mostly documentation value — B1 is a real bug)

- **B1 — ✅ Fixed, `0a7fc6d`.** `sweph.c:163`: `pnoext2int[]` had 21
  entries, indexed at two call sites whenever `ipl < SE_NPLANETS` (23,
  `swephexp.h`). The two slots `SE_INTP_APOG`/`SE_INTP_PERG` (21/22) were
  never added to the array. Was masked because an earlier branch
  intercepts those `ipl` values first — latent, not live, but one
  branch-ordering refactor away from a real out-of-bounds read. Widened
  to 23 entries (same `0`-placeholder convention already used for indices
  10-13, which are also `nddat[]`-handled and never real `SEI_*`
  indices), and landed exactly the `static_assert` this section proposed
  — `sizeof(pnoext2int)/sizeof(pnoext2int[0]) == SE_NPLANETS` — verified
  against both the broken 21-entry array (fails to compile) and the fix.
- **B2** — `sweph.c:4719-4732`: the ephemeris-file-format bounds check on
  `nplan` uses a bare literal `20`, not the header's declared capacity
  `SEI_FILE_NMAXPLAN` (50, `sweph.h:722`). A `static_assert(20 <= SEI_FILE_NMAXPLAN, ...)`
  ties the two together so a future header edit can't silently invalidate
  the check.
- **B3** — `swemmoon.c:811-812`: `sscc()`'s `ss[5][8]`/`cc[5][8]` capacity
  isn't tied to the `k`/`n` literals used at its call sites
  (`swemmoon.c:941-944`). A `static_assert` at the call sites (or inside
  `sscc`) would make the relationship explicit.
- **B4** — `swe_heliacal_pheno_ut`'s `darr` out-parameter
  (`swehel.c:1844,2027-2054`) needs 28+ elements with no documented minimum
  anywhere in `swephexp.h`. Since it's caller-allocated, `static_assert`
  doesn't apply directly — recommend a named `#define SE_HELIACAL_DARR_SIZE`
  plus a doc comment at the public prototype instead, replacing tribal
  knowledge with something greppable.
- **Checked and already correct** (contrast, not a finding):
  `NDIAM`/`pla_diam[NDIAM]` (`sweph.h:314-315`) and
  `NMAG_ELEM`/`mag_elem[NMAG_ELEM][4]` (`swecl.c:3748,3762`) both size their
  arrays directly off the same named constant used at every guard site.

---

## 5. Prioritization

> ### Results so far (measured, not projected)
>
> | item | status | measured effect |
> |---|---|---|
> | `restrict` on `interp()` | landed | **unmeasurable here** — `interp()` executes 0 times in this checkout (see §4.1 warning). Kept because it is correct. |
> | `restrict` on `embofs`, `calc_center_body`, `swi_osc_el_plan`, `swi_trop_ra2sid_lon_sosy` | landed | `moon` **−3.6% to −5.6%** across two A/B rounds, `calc-moseph` −1.7% to −2.9%; nothing elsewhere. Call the Moon figure "a few percent" — the estimate is not stable enough to quote precisely. |
| `restrict` on `swi_cross_prod` | **declined** | see below |
| `-flto` | landed, **opt-in** (`make LTO=1`) | `moon` **−5.0%**, `calc-moseph` −2.6%; everything else inside the ±2–3% noise floor. Bit-identical to plain `-O2` across all 5137 golden rows on gcc 13. |
>
> LTO is not on by default: clang/macOS/MSVC parity is unverified (no clang
> on the machine these were measured on — the CI `lto` job exists to close
> that), and changing default build flags for a library other people
> package is a maintainer decision, not a side effect of a perf patch.
>
> Only `moon` clears the noise floor convincingly, and that is exactly what
> §4.4 predicted: cross-TU inlining is the only way `-O2` reaches helpers
> like `swi_coortrf2` from `swemmoon.c`.

> #### `swi_cross_prod` is deliberately NOT annotated
>
> §4.1 lists it as verified safe. Re-checking the call sites, five of the
> seven are `swi_cross_prod(x, x+3, xnorm)` — `a` and `b` are adjacent
> three-element slices of one array. That is legal under `restrict` today,
> since `a[0..2]` and `b[0..2]` do not overlap, but it is one line away from
> undefined behaviour: any future edit reading `a[3]` makes the program
> silently wrong rather than merely slower.
>
> The body is three lines of arithmetic with no loop, so `restrict` buys
> essentially nothing. Accepting a fragile aliasing precondition for an
> unmeasurable gain is a bad trade, and the annotation is declined.

**Tier 1 — real performance, do first, low risk:**
- `restrict` on `interp()` and the 6 verified-safe functions (§4.1)
- `const`-qualify the 7 function-parameter candidates + add `-flto` to
  `C17_MIGRATION.md` Phase 5 (§4.4)

**Tier 2 — ✅ macro items DONE; do alongside Tier 1, low effort, real value:**
- ~~Fix B1 (`pnoext2int`/`SE_NPLANETS`) now, standalone~~ — **done, `0a7fc6d`** (§4.6)
- `swethread.h` dead-tier removal, done together with `tests/threadshim.c`'s
  matching cleanup, timed to land with `C17_MIGRATION.md` Phase 5 (§4.3)
- ~~Macro dedup: A3/A4 (`square_sum`/`dot_prod`) and A2 (`mods3600`)~~ — **done**.
  A5's dead `degtocs`/`cstodeg` deleted at the same time. No measurable
  perf change either way (all within ±0.7%), which is the expected
  result: `-O2` inlines these to what the macros produced. A1 (`sind`
  and friends in `swehouse.h`) is left alone — 7 macros, ~200 call
  sites, no measurable gain, and the type-safety argument does not
  justify that much churn on numerical code.

**Tier 3 — opportunistic, smaller or needs more design first:**
- `plan_data`/`save_positions` `_Alignas(64)` — fold into `PLAN.md` Phase
  3's `swe_ctx` redesign rather than doing it twice (§4.2)
- B2/B3/B4 `static_assert`/documentation additions (§4.6)
- A1/A5 macro cleanup (§4.5)

## 6. Verification — this plan needs a benchmark, not just a no-op check

Every other `notes/*.md` plan gates on `tests/golden` producing a
byte-identical transcript — necessary here too (none of this should change
output), but **insufficient**, because this is the first plan that actually
claims speed improvements. `tests/golden` proves "didn't break it," not
"made it faster."

- [x] `tests/bench` (`make -C tests bench-run`). Median of N runs, plus min
      and the spread, at `-O2` regardless of `CFLAGS` — a claim about codegen
      measured at `-O0` is meaningless.
- [x] **Baseline recorded**, gcc 13 `-O2`, this machine, median of 7:

      | workload | median (s) | note |
      |---|---|---|
      | calc-swieph | 0.2937 | planets from `.se1` |
      | calc-moseph | 0.7589 | Moshier, no file IO |
      | jpl-interp  | 0.4612 | `interp()`-heavy — §4.1's target |
      | moon        | 0.5430 | `swemmoon.c` |
      | houses      | 0.5138 | **control**: §4.1 must not move this |

      **Resolution: about ±2–3%.** Medians reproduce across runs to ~2% and
      the within-run spread is 3–7%. This instrument can support a claim of
      ">5% faster"; it cannot support "1% faster", and no commit should
      claim one. The first version of the benchmark ran each workload in
      3–70 ms with up to 50% spread, where the noise was an order of
      magnitude larger than any effect being measured.
- [ ] Each Tier 1 item lands as its own commit with a before/after number in
      the commit message — if a change doesn't move the number, say so
      rather than keeping it on faith that it should have helped.
- [ ] `-flto` and any `_Alignas` changes also need the platform-parity check
      (`C17_MIGRATION.md` G5) — LTO in particular can behave differently
      across gcc/clang/MSVC.

## 7. Sequencing

`C17_MIGRATION.md` → this plan. Within this plan, Tier 1/2 items are
independent of each other and of `PLAN.md` Phase 3 — they can land as soon
as `C17_MIGRATION.md` ships. The one exception is `plan_data`/`save_positions`
alignment (§4.2, Tier 3), which should be designed jointly with `PLAN.md`
Phase 3's `swe_ctx` layout rather than landed first and reworked later.
