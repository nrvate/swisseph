# Thread-Safety Working Notes

Working documents for the thread-safe Swiss Ephemeris effort (`threadsafe`
branch). These are **our** notes — upstream Astrodienst documentation lives in
`doc/`, not here.

| Document | What it is |
|---|---|
| [INVESTIGATION.md](INVESTIGATION.md) | Root-cause analysis: *why* libswe is not thread-usable, with measurements and reproductions. Read this first. |
| [PLAN.md](PLAN.md) | The end-to-end remediation plan: phases, gates, risks, effort. |
| [CONFIG-MAP.md](CONFIG-MAP.md) | Read/write map of every configuration field: what is shared config vs. per-thread derived state, and the cache-invalidation dependency table. Input to Phase 2. |
| [REVIEW.md](REVIEW.md) | Separate survey: modernization/maintainability and performance findings across the whole codebase, unrelated to thread-safety. |
| [C17_MIGRATION.md](C17_MIGRATION.md) | Staged plan to move the codebase from C89 to C17: kill the 16-bit/DOS compiler cruft, adopt `stdint.h`/`stdbool.h`, flip the build to `-std=c17 -Wall -Wextra -Werror`. See its §8 for how it sequences against `PLAN.md`. |
| [C17_PERFORMANCE.md](C17_PERFORMANCE.md) | Follow-on to C17_MIGRATION.md: what to actually *do* with C17 once it lands — `restrict`, cache alignment, `const`/LTO, `static_assert`, macro→inline. Performance-prioritized; found one real latent bug (§4.6 B1) along the way. |

Related, elsewhere in the tree:

- `tests/` — the golden-baseline and thread-consistency harness (Phase 0)
- `tests/baseline.txt` — bit-exact reference transcript from pristine `3fd0f95`

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
