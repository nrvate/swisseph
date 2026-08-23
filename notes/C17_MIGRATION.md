# Plan: C89 → C17 Migration

**Status:** Phases 1-3 done (04bb51a, 5c2eb9d, aceb318; stray comment fixed
in 2a4bad4). Phase 4 (real `static_assert` on J1) and Phase 5 (`-std=c17
-Wall -Wextra`/`-Werror` build flip) are next — **not yet started**, picking
up here next session. Sequencing revised 2026-08-23 (§8): these two now go
*before* `PLAN.md` Phase 3 rather than after, agreed with `swisseph-d2`.
`PLAN.md` Phase 3 is on hold, waiting on this.
**Companion to:** [`REVIEW.md`](REVIEW.md) §1.3, §0 (the "no `stdint.h`/`stdbool.h`
anywhere," "K&R holdouts," "hand-rolled `int32`" findings this plan resolves)
**Relationship to** [`PLAN.md`](PLAN.md): independent effort, same safety net
(the `tests/golden` baseline harness). See §8 for how the two should sequence,
and §9 for a concrete finding handed over from `PLAN.md` Phase 2.

---

## 1. Goals

1. Delete the 16-bit/DOS-era compiler-detection cascade in `sweodef.h`
   (Turbo C, Symantec C, Watcom C, `INT_16`) — dead weight on every platform
   this project actually ships on today.
2. Replace hand-rolled integer/bool typedefs with real `<stdint.h>`/`<stdbool.h>`,
   **without** breaking the public typedef names (`int32`, `AS_BOOL`, ...) that
   downstream callers already depend on.
3. Build cleanly as `-std=c17 -Wall -Wextra` with zero warnings, across the
   compilers this project actually targets (gcc, clang, MSVC).
4. Use the standard's real safety features where they map directly onto
   findings already on record — `static_assert` on the buffer-capacity
   assumptions behind `REVIEW.md`'s J1 finding (`swejpl.c` `ncoeffs`/`ncf`
   vs. fixed array sizes), `snprintf` in place of the ~930 `sprintf`/`strcpy`/
   `strcat` call sites.
5. Numerical no-op throughout, same bar as `PLAN.md` §1.5: `tests/golden`
   byte-identical at every commit.

## 2. Non-goals

- Rewriting algorithms, changing constants, or "cleaning up" code this plan
  isn't touching (same principle as `PLAN.md` §2 — stay upstream-mergeable).
- Adopting genuinely optional/poorly-supported C11 features: Annex K bounds-
  checking (`sprintf_s` et al.) is optional, glibc doesn't implement it, and
  MSVC's `_s` functions aren't the same API — **`snprintf` (already C99) is
  the actual fix for the unsafe-string-function finding, not Annex K.**
  Likewise `<threads.h>`/`<stdatomic.h>` are out of scope here; that's
  `PLAN.md`'s territory if it ever wants them.
- Breaking the public API/ABI. `int32`, `uint32`, `int16`, `AS_BOOL`, etc.
  stay as the names every `swephexp.h` signature uses — only their
  *implementation* changes, from hand-detected widths to `<stdint.h>` aliases.
- Fixing every one of the ~930 unsafe string call sites in one pass. That's
  a large, mechanical, high-file-count change; §6 treats it as its own
  opportunistic phase, not a blocking gate for "the codebase is C17."

## 3. Why C17, not C99 or C23

**Verified, not assumed:** the tree already compiles clean at C99/gnu99/C11/C17
with zero errors; only strict C89 fails, with 58 errors across the nine
library sources (C++-style `//` comments, mixed declarations-and-code):

```
c89  58 errors  |  c99  0  |  gnu99  0  |  c11  0  |  c17  0  |  default  0
```

(measured while doing `PLAN.md` Phase 2 work — see §9). So this plan is not
really "port C89 code to C17" — the code has silently required C99 for a
while. It's: declare that officially, delete what only pre-C99 compilers
needed (§6 Phase 1), and pick up C11/C17's actual features (`stdint.h`,
`static_assert`, designated initializers — §6 Phase 2, §9). That's a
materially lower-risk project than a real dialect port.

- **C99** is already the de facto floor — `setest/Makefile:26` pins
  `-std=gnu99` and the main build's untagged dialect is effectively C99-with-
  GNU-extensions already. Formally adopting C99 buys nothing not already in
  use.
- **C17** is C11 with its defect reports folded in, no new features to chase
  — it's the first standard with real `stdint.h`/`stdbool.h`/`static_assert`,
  and every compiler this project targets (gcc ≥ 4.9, clang ≥ 3.5, MSVC ≥
  2015) supports it fully. Low-risk, high-payoff.
- **C23** is too new for confidence here — spotty support in older toolchains
  this project's Windows/embedded consumers may still use, and it doesn't
  unlock anything this codebase needs beyond what C17 already gives it.

## 4. Hard constraints

| Constraint | Enforcement |
|---|---|
| **Numerical no-op** | `tests/golden` transcript byte-identical at every commit (same harness as `PLAN.md`) |
| **Public typedef names unchanged** | `int32`/`uint32`/`int16`/`AS_BOOL`/`UINT2`/`UINT4` keep their names in `swephexp.h`; only `sweodef.h`'s definition of them changes |
| **No silent ABI change** | `nm -D --defined-only libswe.so` symbol list unchanged (reuse `PLAN.md`'s planned `abi-check` target) |
| **Real platforms only** | Any compiler-detection branch removed must correspond to a target nobody has built for in years — confirm via `git log`/CI history before deleting, not just by inspection |
| **Upstream mergeable** | No reformatting-only diffs; every change is either a deletion of genuinely dead branches or a mechanical, scriptable substitution |

## 5. Success criteria

```sh
# G1  numerical no-op (same as PLAN.md)
tests/golden --ephe ephe > out.txt && diff tests/baseline.txt out.txt

# G2  builds warning-clean under the target standard
$(CC) -std=c17 -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -c *.c
# zero warnings, not just zero errors

# G3  no ABI change
nm -D --defined-only libswe.so | sort > out.symbols
diff abi/libswe.symbols out.symbols

# G4  static_assert catches the J1-class buffer-size assumption
#     (swejpl.c ncoeffs/ncf vs. fixed array capacity — see REVIEW.md §5)
grep -q static_assert swejpl.c

# G5  platform parity: G1/G2 pass on Linux/gcc, Linux/clang, macOS/clang,
#     Windows/MSVC — the four toolchains this project actually ships for
```

---

## 6. Phases

### Phase 0 — Infrastructure (reuse, don't rebuild)

`tests/golden` from `PLAN.md` Phase 0 is the safety net for this plan too —
no new harness needed. Add one thing it doesn't have yet:

- [ ] A **non-blocking** CI job that builds the current tree with
      `-std=c17 -Wall -Wextra` and just reports the warning count, before
      anything is changed. This is the "how far away are we" baseline number
      every later phase is measured against.

**Exit gate:** warning-count baseline recorded; `tests/golden` green.

### Phase 1 — Delete the 16-bit/DOS compiler cascade

The concrete target is `sweodef.h:96-216`: the `_WIN32`/`MSDOS`/`__TURBOC__`/
`__SC__`/`__WATCOMC__` detection chain, the `INT_16` branch (`176-216`), and
everything downstream that exists only to support them. This is almost pure
deletion:

| # | Action | Location |
|---|---|---|
| 1.1 ✅ | Delete `__TURBOC__`/`TURBO_C`, `__SC__`/`SYMANTEC_C`, `__WATCOMC__`/`WATCOMC` branches | `sweodef.h:123-142` |
| 1.2 ✅ | Delete the `INT_16` branch and its `typedef long int32` / `typedef int int16` fallback — collapse to the 32-bit-int path unconditionally | `sweodef.h:176-216` |
| 1.3 ✅ | Collapse the `MSDOS` detection cascade to a single `#ifdef _WIN32`/`WIN32` check; drop the bare `MSDOS` legacy alias entirely if nothing outside `sweodef.h` reads it (`grep -rn 'MSDOS' --include='*.c'` first) | `sweodef.h:96-159` |
| 1.4 ✅ | Re-verify the `TLS` macro's compiler list (`sweodef.h:86-94`) still matches reality post-cleanup — this is shared ground with `PLAN.md`, touch carefully | `sweodef.h:86-94` |

Each sub-step is independently revertable and gated on `tests/golden` staying
byte-identical, same discipline as `PLAN.md` Phase 1.

**Exit gate:** `sweodef.h` no longer contains any 16-bit-int or DOS-era-compiler
branch; G1 and G5 (Linux/macOS/Windows) still pass. **✅ Done.**

**Outcome:** `grep -rln 'MSDOS' --include='*.c'` before deleting confirmed
bare `MSDOS` is read in 7 `.c` files outside `sweodef.h`, so it was kept
(per 1.3's own conditional) — only the now-unreachable *paths that set it*
(Turbo C/Symantec C/Watcom C/"already defined by some DOS compiler") were
removed. `LONG_64` (defined alongside `INT_16`, 1.2) was independently
confirmed unused anywhere in the tree and deleted with it. `MS_C`'s `#ifndef
TURBO_C` guard collapsed to an unconditional define since `TURBO_C` can no
longer exist. 1.4 required no edit — the `TLS` block was untouched by 1.1-1.3
and its pre-existing `!defined(WIN32)`-disables-TLS behavior is
INVESTIGATION.md's already-documented Class B, `PLAN.md` territory, not
touched here. Verified: `make check` (G1, G1b, G2, cfgleak, threadshim) all
green; root `make all` clean under `-Wall`, zero warnings;
`check-threadshim-all` still passes across all 5 dialect configs including
`-std=c89`. Diff is 65 lines removed, 8 added, `sweodef.h` only.

### Phase 2 — Real fixed-width types, same public names

Replace the *implementation* of the typedefs Phase 1 left behind, without
renaming anything the public API exposes:

```c
/* sweodef.h, after Phase 2 */
#include <stdint.h>
#include <stdbool.h>

typedef int32_t   int32;
typedef uint32_t  uint32;
typedef int16_t   int16;
typedef uint16_t  UINT2;
typedef int32_t   INT4;
typedef uint32_t  UINT4;
typedef bool      AS_BOOL;   /* was `typedef int AS_BOOL` */
```

- [x] 2.1 Land the typedef changes above.
- [x] 2.2 Grep every comparison of an `AS_BOOL` value against something other
      than `TRUE`/`FALSE`/`0`/`1` (a `bool`-vs-`int` semantic change is the
      one place this phase can silently alter behavior — e.g. code that
      stores something other than 0/1 into an `AS_BOOL` and later relies on
      the actual stored integer value).
- [x] 2.3 `swephexp.h`/`swedll.h` themselves don't need edits — they already
      use `int32`/`AS_BOOL` by name, not by underlying type.
- [x] 2.4 **`swed`'s positional initializer → designated initializer.**
      Handed over from `PLAN.md` Phase 2; full rationale, the two bugs it
      already caused, and the verification steps are in §9. One-line summary:
      `sweph.c:96-126`'s 30-line positional initializer silently zero-fills
      any `swed` member declared after `astro_models`, with no diagnostic if
      that assumption breaks. Replace with
      `TLS struct swe_data swed = { .const_lapse_rate = SE_LAPSE_RATE };`
      (every other field is already zero) — behaviour-identical, removes the
      ordering hazard permanently. Needs C99 designated initializers, which
      §3 confirms are already safe to use.

**Exit gate: ✅ Done.**

**Outcome:** 2.1-2.3 landed as planned. 2.2's audit (every `AS_BOOL`-returning
function, every assignment site, checked for non-0/1 stores) found no risk in
how `AS_BOOL` itself is *used* — but building against the real `bool` still
broke the build and then broke a test, both from **pre-existing type
mismatches that `AS_BOOL == int` had been silently absorbing for years**:

- **Build break:** `swecl.c:92-95`'s forward declarations for
  `eclipse_when_loc`/`occult_when_loc` typed `backward` as `AS_BOOL`, but the
  real definitions (and every caller — `swetest.c:3561` ORs in
  `SE_ECL_ONE_TRY`) always treated it as an `int32` bitmask. Compiler caught
  it immediately as a conflicting-types error. Fixed the declarations to
  match the (correct) definitions.
- **Test break, not caught by 2.2's audit because it's not a value-usage bug
  — it's a struct-field mistype:** `sweph.h:854-856` declared
  `n_fixstars_real`/`n_fixstars_named`/`n_fixstars_records` as `AS_BOOL`,
  though they hold real counts (1141 stars, etc.), never booleans. Storing
  1141 into a `_Bool` truncates to `1`; every fixed-star `bsearch()`
  afterward ran over a 1-record slice instead of the real array. `G1` caught
  it (fixed-star rows in the golden transcript went from real data to "star
  not found"); root-caused by direct instrumentation, not by re-reading the
  audit — the type was simply wrong at declaration, so no amount of grepping
  *uses* of `AS_BOOL` would have found it. Fixed the field types to `int32`.

Both are comment-documented at their fix sites (`swecl.c`, `sweph.h`) for
anyone who finds this again. Neither would have been reachable without this
phase — `AS_BOOL`/`int32` being identical types is exactly what let both
mistypes ship for years.

2.4 landed too: `sweph.c:96` is now
`TLS struct swe_data swed = { .const_lapse_rate = SE_LAPSE_RATE };`,
confirmed behavior-identical (every other field already defaulted to zero).

Verified: `make check` (G1 5303 rows bit-identical, G1b, G2, cfgleak,
threadshim) all green; root `make all` clean under `-Wall`, zero warnings;
`check-threadshim-all` green across all 5 dialects including `-std=c89`;
§9's fresh-thread `const_lapse_rate` check (a worker that calls only
`swe_azalt()` still reads `0.006500`, not `0.0`). G3 (ABI) not independently
re-verified this pass — `int32_t` and the old hand-rolled `int32` are both
4-byte signed ints on every real target, so mangled/exported names don't
move, but no `nm -D` diff was run.

### Phase 3 — Fix the two K&R holdouts

`REVIEW.md` §6 already found these are the *only* two left in the tree:

- [x] `swephgen4.c:85` — `int split(w, m, min, sec)` → ANSI prototype
- [x] `swephgen4.c:167-168` — `char *degstr (t) double t;` → ANSI prototype
      (leave the non-reentrant `static char a[20]` return convention alone;
      that's a separate, larger API change out of scope here)

Trivial, but it's what actually blocks `-std=c17 -Wold-style-definition` from
going clean, so it has to happen before Phase 5's `-Werror` flip.

**Exit gate: ✅ Done.**

**Outcome:** found and fixed a third old-style definition `-Wstrict-prototypes`
flagged that `REVIEW.md`'s K&R-specific search missed because it isn't K&R
syntax — `eph_test()` with empty parens (unspecified-args, not zero-args) at
`swephgen4.c:184`. Fixed to `eph_test(void)`, no callers pass arguments so
behavior is unchanged. `swephgen4.c` is not referenced by the root `Makefile`
or any header — it isn't part of any build target — so the only meaningful
verification available is a standalone compile:
`gcc -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -c swephgen4.c`,
clean, exit 0.

### Phase 4 — `static_assert` on the file-trusted buffer sizes

**Correction, caught before this phase started (credit: a review from the
other session working `PLAN.md` Phase 3, relayed by the user):** this
section's original worked example —
`static_assert(sizeof(js->buf)/sizeof(js->buf[0]) >= 2500, ...)` — does not
compile. `buf[]` was declared `double buf[1500]`, not 2500; the assertion as
written is `static_assert(1500 >= 2500)`, which fails on the spot. The
"documentation-only" framing below was wrong: landing that assertion as
originally written forces an immediate choice between widening `buf[]` (a
real fix) or weakening the assertion to something true but vacuous
(`>= 1500`), which asserts nothing. There was no way to land this phase as
pure documentation.

**Resolved by fixing J1 directly, ahead of this phase**, rather than
deferring it — the fix needs no C11 feature (an array size and a bounds
check are both plain C, valid under every dialect this codebase supports),
so nothing about it required waiting for Phase 5's build-flag flip:

- `buf[1500]` → `buf[JPL_NCOEFF_MAX]` (`JPL_NCOEFF_MAX` = 2500, named and
  tied to `fsizer()`'s existing `ksize > 5000` bound at the same site, so
  the two can't drift apart silently again).
- `pc`/`vc`/`ac`/`jc[18]` had **no assertable bound at all** — `ncf` (the
  per-body coefficient count that indexes them) comes straight from the
  file's `ipt[]` with no compile-time relationship to anything. Added a
  **runtime** check in `state()`, once at file-open time (all 13 relevant
  `ipt[]` entries, covering 10 planets + sun + nutation + libration),
  rejecting a file that claims more than `sizeof(js->pc)/sizeof(js->pc[0])`
  coefficients — `NOT_AVAILABLE` + `serr`, the file's own existing
  validation-failure convention, not a new one.

What's left for *this* phase, now genuinely documentation rather than a
disguised bug fix: land a real, now-true `static_assert` confirming
`JPL_NCOEFF_MAX` sizes `buf[]` correctly, once C11 is guaranteed (Phase 5).
The `pc`/`vc`/`ac`/`jc[18]` bound stays a runtime check permanently — it
can't become a `static_assert`, because there's no compile-time constant on
the other side of that comparison; the file controls it.

**Exit gate (revised):** `static_assert` present and passing on `buf[]`'s
sizing (only place a real compile-time invariant exists); runtime check on
`ncf` verified to reject a synthetic oversized-`ipt[]` value (needs a
malformed-file fixture — none of this repo's shipped `ephe/` files exercise
the JPL code path at all, so this can't be checked against `tests/golden`
and needs its own fixture before it's considered verified, not just
compiled). Also fix the original `G4` gate (`grep -q static_assert
swejpl.c`) — it passes on any `static_assert` in the file, including a
trivially true one; tighten it to check the specific assertion.

### Phase 5 — Flip the build, turn on `-Werror`

- [ ] 5.1 `Makefile`: `CFLAGS = -std=c17 -Wall -Wextra -O2 -g -fPIC` (folds in
      the "no `-std=`, no `-O2`" build-system finding from `REVIEW.md` §1.1 —
      natural to fix in the same pass since it's the flag line this plan is
      already editing).
- [ ] 5.2 Align `setest/Makefile`'s `-std=gnu99` to `-std=c17` too, so the
      whole tree builds under one dialect.
- [ ] 5.3 Once G2 is warning-clean, add `-Werror` so it stays that way.
- [ ] 5.4 CI matrix: {ubuntu gcc, ubuntu clang, macos clang, windows msvc} ×
      {`-O0 -g`, `-O2`} — reuses the matrix shape `PLAN.md` Phase 0.4 already
      proposes; if that CI work has landed by the time this phase starts,
      extend it rather than duplicating it.

**Exit gate:** all of §5's gates (G1-G5) green in CI, on all four toolchains.

## 7. Optional/opportunistic follow-on: unsafe string functions

Not gated into the phases above because it touches ~930 call sites across
every file in the tree — too large to land atomically. Once Phase 5's
`-std=c17` build is the default, this becomes a mechanical, file-by-file
`sprintf`→`snprintf` / `strcpy`+manual-bound→`snprintf("%s", ...)` pass,
each file independently verified against `tests/golden`. Track it as its own
follow-up plan if/when it's prioritized — `REVIEW.md` §1.3 has the count and
locations to scope it.

## 8. Sequencing against `PLAN.md` (thread-safety)

Both plans touch `sweodef.h` (Phase 1.4's `TLS` macro sits right next to the
compiler-detection cascade this plan deletes) and both use the same
`tests/golden` harness as their numerical safety net. Recommended order:

- **Phase 1 of this plan (kill the 16-bit/DOS cruft) can start immediately**,
  in parallel with `PLAN.md`'s work — it's an almost pure deletion, doesn't
  touch `swed` or any TLS-annotated state, and the overlap in `sweodef.h` is
  small enough to rebase past either way. **Done** — 04bb51a.
- **Phases 1-3 of this plan landed once `PLAN.md` Phase 2 shipped**
  (typedef swap + two latent bugs it exposed, K&R fixes) — 5c2eb9d,
  aceb318. Also fixed one stale comment a code-review pass caught — 2a4bad4.
- **Revised, 2026-08-23: Phases 4-5 (the J1 `static_assert`, the build-flag
  flip to `-std=c17 -Wall -Wextra`/`-Werror`) go *before* `PLAN.md` Phase 3
  starts, not after — reversing this doc's original call.** Proposed by the
  other session (`swisseph-d2`, working `PLAN.md`) once Phase 2 was fully
  gated and Phase 3 genuinely hadn't started: the original "wait until
  Phase 3 is done" reasoning was about not landing a build-flag change
  *simultaneously* with a threading-model change on unstable ground — but
  with Phase 3 not yet started, doing Phase 4/5 first isn't simultaneous
  with anything, it's just first. And it means Phase 3's 650-reference
  mechanical rewrite gets to build fresh against an already-clean C17/
  `-Werror` baseline (real `stdint.h`/`stdbool.h`/eventually `stdatomic.h`
  from the first commit) instead of Phase 3 landing on the old build and
  this plan re-touching the same 650 sites afterward. Agreed; `PLAN.md`
  Phase 3 is on hold until this plan's Phase 5 lands.
- The CI-matrix work in §6 Phase 5.4 and `PLAN.md` §0.4 turned out to be
  the same deliverable, and `swisseph-d2` built it — 9b97fdd, 7217f67,
  f2843b0. It also caught something neither of these plans' own review
  passes had: TLS was still disabled on macOS (`__APPLE__` excluded from
  the `TLS` macro in `sweodef.h`, `PLAN.md`'s own deferred Phase 1.6) —
  `swed` was a plain unsynchronized global there. Fixed in f2843b0, outside
  this plan's scope but sharing the same file Phase 1 touched, noted here
  for the record.

## 9. Handed over from `PLAN.md` Phase 2: the `swed` positional initialiser

Slotted into the phase list as §6 Phase 2.4; this section is the full
rationale, the two bugs it already caused, and the verification steps.

Found while doing thread-safety work, deliberately **not** fixed there because
it is modernization, not config propagation. Phase 2 was rescoped to avoid
needing it (see below), so it is free for this plan to take.

`sweph.c:96-126` initialises the 23 KB `swed` structure with a **positional**
initialiser spanning ~30 lines. Two properties make it a live hazard:

1. **It stops early.** The list ends just after `astro_models`; every member
   declared after that point — `do_interpolate_nut`, `interpol`, `fidat[]`,
   `gcdat`, `pldat[]`, `nddat[]`, `savedat[]`, `oec`, `oec2000`, `nut`,
   `nut2000`, `nutv`, `topd`, `sidd`, `fixed_stars` — is implicitly zeroed.
   Whether a new field gets its intended default therefore depends silently on
   *where in the struct it is declared*.
2. **Nothing checks the correspondence.** Field order in `sweph.h` and value
   order in `sweph.c` are coupled only by a hand-maintained trailing comment
   per line (`0.0, /* tid_acc */`). Insert a member anywhere above
   `astro_models` and every subsequent value silently shifts by one, with no
   diagnostic — `-Wall -Wextra` says nothing.

This bit twice during Phase 1/2:

- `saved_sundec` (`swehouse.c`) could not be moved into `swed` as originally
  planned, because `swi_init_swed_if_start()`'s `memset(&swed, 0, ...)` would
  have replaced its `99` sentinel with `0`. It was made `TLS` instead.
- `const_lapse_rate` (Phase 2.2) had to be declared **immediately after**
  `astro_models` rather than appended to the end of the struct, or its
  `SE_LAPSE_RATE` (0.0065) default would have become `0.0` — a silently wrong
  refraction. `swe_azalt()` reads it and calls no init function, so
  `swi_init_swed_if_start()` could not have covered for it. A comment on the
  member now records the constraint, which is a workaround, not a fix.

**The fix is a one-liner in C99+.** Every value in that initialiser is
`0`, `0.0`, `FALSE` (`#define FALSE 0`), `NULL`, or `""` — all of which are
zero — except exactly one:

```
  12  0        9  0.0      8  FALSE     4  ""     3  NULL
   1  SE_LAPSE_RATE
```

So the whole 30-line block is equivalent to:

```c
TLS struct swe_data swed = {
  .const_lapse_rate = SE_LAPSE_RATE,   /* the only non-zero default */
};
```

That is behaviour-identical, removes the ordering coupling permanently, and
makes future field additions safe by construction. It needs C99 designated
initialisers, which is why it belongs to this plan.

**Verified prerequisite:** the library already requires C99 and compiles clean
at C17 — C89 has been broken for a long time (58 errors across the nine
library sources; C++ comments and mixed declarations). So designated
initialisers are already safe to use today, before any of this plan lands:

```
c89  58 errors  |  c99  0  |  gnu99  0  |  c11  0  |  c17  0  |  default  0
```

**Why Phase 2 did not need it after all.** The original Phase 2 design nested
the configuration fields as `swed.cfg.*`, which would have changed the struct
layout and forced this initialiser to be rewritten. That design was dropped in
favour of leaving the fields flat in `swed` and keeping the shared master as a
separate `struct swe_config` with explicit capture/apply — 168 reference edits
and the initialiser rewrite both avoided.

Verify the change against `tests/golden` (G1, bit-exact) plus the fresh-thread
check that Phase 2.2 used, which is the case a naive edit breaks:

```c
/* a worker thread that calls ONLY swe_azalt() -- no init path runs */
printf("%.6f\n", swed.const_lapse_rate);   /* must be 0.006500, not 0.0 */
```
