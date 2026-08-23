# Thread-Safety Working Notes

Working documents for the thread-safe Swiss Ephemeris effort. They are a
**historical record**, written as the work happened and dated accordingly --
not a description of the current tree. Where a note states a row count, a
symbol count or a phase status, read it as true at the time of writing; the
current numbers are in [`THREADING.md`](../THREADING.md) and the top-level
[readme](../readme.md). These are **our** notes — upstream Astrodienst documentation lives in
`doc/`, not here.

| Document | What it is |
|---|---|
| [INVESTIGATION.md](INVESTIGATION.md) | Root-cause analysis: *why* libswe is not thread-usable, with measurements and reproductions. Read this first. |
| [PLAN.md](PLAN.md) | The end-to-end remediation plan: phases, gates, risks, effort. |
| [PHASE3-API.md](PHASE3-API.md) | The `swe_ctx` handle API: surface, threading contract, legacy shim. **Implemented**; every open decision was signed off and built. |
| [CONFIG-MAP.md](CONFIG-MAP.md) | Read/write map of every configuration field: what is shared config vs. per-thread derived state, and the cache-invalidation dependency table. Input to Phase 2. |
| [REVIEW.md](REVIEW.md) | Separate survey: modernization/maintainability and performance findings across the whole codebase, unrelated to thread-safety. |
| [BINARIES.md](BINARIES.md) | Compiled artifacts checked into the source tree: full inventory, what was removed, what is deliberately deferred (`contrib/`), and the CI-build/release plan replacing them. |
| [C17_MIGRATION.md](C17_MIGRATION.md) | Staged plan to move the codebase from C89 to C17: kill the 16-bit/DOS compiler cruft, adopt `stdint.h`/`stdbool.h`, flip the build to `-std=c17 -Wall -Wextra -Werror`. See its §8 for how it sequences against `PLAN.md`. |
| [C17_PERFORMANCE.md](C17_PERFORMANCE.md) | Follow-on to C17_MIGRATION.md: what to actually *do* with C17 once it lands — `restrict`, cache alignment, `const`/LTO, `static_assert`, macro→inline. Performance-prioritized; found one real latent bug (§4.6 B1) along the way. |

Related, elsewhere in the tree:

- [`THREADING.md`](../THREADING.md) — **user-facing** guide to the thread-safe API. Start there if you want to *use* the library rather than read how it was built.
- `tests/` — 17 gate targets: golden baseline, thread consistency, context independence, leak/race, JPL reader, threading backends, bridge invariant, build, Windows code paths, version single-source. Ten run in `make -C tests check`; the rest are run explicitly or by CI.
- `tests/baseline.txt` — bit-exact reference transcript, 12761 rows (2.5 MB)

## Status

| Plan | State |
|---|---|
| [INVESTIGATION.md](INVESTIGATION.md) | complete — root cause understood and reproduced |
| [PLAN.md](PLAN.md) Phases 0–2 | complete — configuration propagates, races fixed |
| [PLAN.md](PLAN.md) Phase 3a–3d | complete — 78 `_r` entry points, `swe_ctx_new`/`free`, ABI additive only |
| [PLAN.md](PLAN.md) Phase 3e | **not started, and optional** — a shared ephemeris file cache. It was always a decision point, not a commitment; §8.6 has the trade-off. Nothing depends on it. |
| [C17_MIGRATION.md](C17_MIGRATION.md) | complete — all five phases |
| [C17_PERFORMANCE.md](C17_PERFORMANCE.md) | complete — several items measured and *declined*, which is recorded as carefully as what landed |
| [REVIEW.md](REVIEW.md) | both correctness findings resolved (J1 fixed, S1 deleted). The remaining themes — unsafe string handling, `goto` density, no enums — are systemic modernization, untouched and out of scope for this branch. |
| [BINARIES.md](BINARIES.md) | complete — no build products are tracked any more. CI builds Linux, macOS, Windows and Android packages on every push, and a tag publishes them as a release. The `contrib/` third-party archives are deliberately still there; BINARIES.md says why. |

**Released.** The work described in these notes shipped as `2.10.03-ts.1` and
`2.10.03-ts.2` (see [releases](https://github.com/nrvate/swisseph/releases)).
It lives on `main`; `threadsafe` holds the granular history and is frozen by a
repository ruleset; `legacy-master` is the pristine upstream tree kept for
merges from upstream.

All gates green; see [`THREADING.md`](../THREADING.md) for what each one
proves. The one thing worth knowing before changing anything here: the
gates are bit-exact, so a change that alters *any* number fails loudly rather
than drifting.

## The one-paragraph version

libswe is not "not thread safe" in the usual sense. Since v2.03 upstream
annotated the global `swed` blob with `TLS` → `__thread`, which on Linux/GCC
genuinely eliminates data races. The result is a library that is **race-free but
not thread-usable**: configuration set on one thread is invisible to every other,
so worker threads silently fall back to lower-precision ephemerides and wrong
sidereal modes while still reporting success. On macOS `TLS` compiles to nothing,
so the behaviour inverts. Details in [INVESTIGATION.md](INVESTIGATION.md).

> **Note on paths:** file paths in these documents are relative to the **repository
> root**, not to `notes/`. So `tests/golden.c` means `<repo>/tests/golden.c`.
