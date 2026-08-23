# Phase 3 API Spec — the `swe_ctx` handle

**Status:** IMPLEMENTED (3a–3d). Every **[DECIDE]** below was signed off and
built as recommended. Measured surface came out at 77 `_r` entry points, not
the 75 estimated here — see PLAN.md §8.5.
**Companion to:** [PLAN.md](PLAN.md) §8, [CONFIG-MAP.md](CONFIG-MAP.md)

---

## 1. What Phase 3 is for

Phase 2 made configuration *propagate*: set it once, every thread sees it. It
deliberately did not make configurations *independent*. A single shared master
cannot give two threads two different live sidereal modes at the same time.

Phase 3 delivers exactly that, and three things that fall out of it:

| | Phase 2 | Phase 3 |
|---|---|---|
| Config set once is visible everywhere | yes | yes |
| N threads, N independent configurations | **no** | **yes** |
| Deterministic lifetime (no per-thread leak) | no | yes |
| Works without compiler TLS | no | yes |
| Caches/fds shareable rather than duplicated | no | possible (§8) |

The concrete user story: a server answering two requests concurrently, one
tropical and one sidereal, or two different observer positions. Today that
requires one process per configuration.

---

## 2. Measured surface

Not the 106 entry points PLAN §8 assumed. Classified by whether the function
transitively reaches `swed` or any other TLS state:

```
public entry points in swephexp.h ....... 107
  need a context ........................  75
  genuinely pure (no state at all) ......  30
  declared but not defined in the library    2   (swe_set_timeout, swe_split_deg*)
```

**The 30 pure ones get no `_r` variant.** They are already reentrant and adding
a context argument would be noise:

```
swe_cotrans        swe_cotrans_sp     swe_cs2degstr      swe_cs2lonlatstr
swe_cs2timestr     swe_csnorm         swe_csroundsec     swe_d2l
swe_date_conversion swe_day_of_week   swe_deg_midp       swe_degnorm
swe_difcs2n        swe_difcsn         swe_difdeg2n       swe_difdegn
swe_difrad2n       swe_get_ayanamsa_name  swe_get_library_path
swe_house_name     swe_house_pos      swe_houses_armc    swe_julday
swe_rad_midp       swe_radnorm        swe_refrac         swe_refrac_extended
swe_revjul         swe_utc_time_zone  swe_version
```

> Note `swe_houses_armc_ex2` is **not** in that list even though it never
> touches `swed`. It owns `saved_sundec`, the cross-call Sunshine-houses memo
> from Phase 1.1. An earlier version of this classification put it in the pure
> set because it only looked for `swed.` — the same narrowness that let the
> macOS TLS gap survive three review passes. State is state regardless of which
> variable holds it.

Beyond `swed` there are **77 TLS statics** still to fold in:

```
swemmoon.c 27   sweph.c 17   swehel.c 13   swejpl.c 7
swephlib.c  5   sweconfig.c 3   swedate.c 2   swemplan.c 2   swehouse.c 1
```

---

## 3. The handle

```c
typedef struct swe_ctx swe_ctx;          /* opaque; defined in sweph.h */

swe_ctx *swe_ctx_new(void);              /* NULL on allocation failure */
void     swe_ctx_free(swe_ctx *ctx);     /* closes files, frees caches */
```

### 3.0.1 `swe_close()` vs `swe_close_r()` — they differ on purpose

`swe_close_r(ctx)` releases that context's resources. `swe_close()` does that
*and* resets the process-wide configuration master.

The split is not cosmetic. `swed` is TLS, so every thread has its own default
context whose ephemeris segments are freed only by a close **on that thread**
— about 9 KB per worker, measured with LeakSanitizer. Before the split a
worker thread had no way to release its own memory: the only close available
also wiped the configuration every other thread was reading, so cleaning up
after yourself broke your neighbours.

    worker thread, done with the library:   swe_close_r(swi_default_ctx());
    whole process shutting the library down: swe_close();


Opaque deliberately. `struct swe_data` is 23 KB of implementation detail that
has changed shape twice already this month; exposing it would freeze that.

### 3.1 Threading contract

> **A `swe_ctx` may be used by one thread at a time.** Concurrent calls on the
> same context are undefined. Contexts are independent: no locking between them.

This is the `FILE *` / `sqlite3 *` contract, and it is the one that makes the
implementation simple enough to be correct — no locks on the hot path, no
atomics, no cache invalidation protocol between contexts.

Passing a context between threads is fine with external synchronisation; using
it from two at once is not.

### 3.2 Naming

`_r` suffix, context first:

```c
int32 swe_calc_ut_r(swe_ctx *ctx, double tjd, int32 ipl, int32 iflag,
                    double *xx, char *serr);
void  swe_set_ephe_path_r(swe_ctx *ctx, const char *path);
```

`_r` is the POSIX reentrant convention and reads correctly here. `_ex`/`_ex2`
are already taken by upstream for "extended signature", so reusing them would
be actively confusing — `swe_calc_ut_ex` would look like a third variant of
`swe_calc_ut_ex2`.

---

## 4. The legacy API

Every existing entry point keeps working, unchanged, as a shim:

```c
int32 CALL_CONV swe_calc_ut(double tjd, int32 ipl, int32 iflag,
                            double *xx, char *serr)
{
  return swe_calc_ut_r(swi_default_ctx(), tjd, ipl, iflag, xx, serr);
}
```

**[DECIDE] Is the default context per-process or per-thread?**

|  | per-process (+ Phase 2 config machinery) | per-thread |
|---|---|---|
| Preserves Phase 2 behaviour exactly | **yes** | no |
| `set_ephe_path` on main reaches workers | yes | **no — reintroduces the original bug** |
| Needs locking in the default context | yes (already built) | no |
| Legacy callers get today's semantics | yes | no |

**Recommendation: per-process, reusing the Phase 2 master.** Per-thread would
undo the entire point of Phase 2 for every caller who does not migrate — which
will be most of them, for years. The locking cost only applies to the legacy
path; `_r` callers pay nothing.

So `swi_default_ctx()` returns one process-wide context whose configuration is
kept in sync by the existing `sweconfig.c` machinery, and Phase 2's semantics
survive untouched.

---

## 5. What lives where

From [CONFIG-MAP.md](CONFIG-MAP.md), now with a third column:

```
struct swe_ctx {
    struct swe_config cfg;      /* the 12 config fields (Phase 2)          */
    struct swe_cache  cache;    /* pldat, nddat, savedat, oec, nut, interpol */
    struct swe_files  files;    /* fidat[], fixfp, jpl state, last_epheflag */
    struct moon_state moon;     /* swemmoon.c's 27 implicit-argument globals */
    struct hel_state  hel;      /* swehel.c's 13 memo caches                */
    ...
    struct swe_shared *shared;  /* refcounted, see section 8; NULL for now  */
};
```

The `shared` pointer is present from the start even though nothing uses it,
because retrofitting a sharing layer after 650 references have settled is
strictly harder than leaving the seam open.

**[DECIDE] Does `swe_ctx_new()` inherit the current default configuration, or
start from library defaults?**

Inheriting is friendlier — `swe_ctx_new()` after `swe_set_ephe_path()` "just
works". Starting clean is more predictable and is what most handle APIs do.
**Recommendation: inherit**, with `swe_ctx_new_default()` if anyone wants the
clean variant. The common case is "I configured the library, now give me a
context", and the alternative is a silent Moshier fallback — the exact failure
mode this whole branch exists to eliminate.

---

## 6. Sequencing

Each step keeps G1 bit-exact and is independently revertable.

| step | work | risk |
|---|---|---|
| 3a | thread `swe_ctx *ctx` through internal `swi_*` functions, file by file in ascending coupling: `swedate`(1) `swemmoon`(3) `swehouse`(4) `swemplan`(9) `swecl`(27) `swephlib`(89) `sweph`(517) | volume, not difficulty |
| 3b | `swemmoon.c`'s 27 globals into `struct moon_state` — 14 functions using them as implicit parameters | highest |
| 3c | remaining 77 TLS statics into the context | mechanical |
| 3d | the 75 public `_r` entry points + legacy shims + `abi-check` | mechanical, high volume |
| 3e | shared file cache — **optional**, decide after 3d | design |

3a is done as one commit per file, not one big one. `sweph.c` alone is 517
references and will not be reviewable otherwise.

---

## 7. What does not change

- No algorithm, constant, or file format touched. G1 stays bit-exact throughout.
- No existing symbol removed or changed. `abi-check` enforces additive-only.
- `serr` error handling unchanged — same 256-byte caller buffer convention.
- The 30 pure functions keep their exact signatures.
- Phase 2's config propagation keeps working for every legacy caller.

---

## 8. Deferred: the shared cache (3e)

Today each context opens its own `FILE*` per `.se1` (3+ fds, measured in
INVESTIGATION.md §4.1) and re-decodes the same Chebyshev segments. N contexts
means N copies of identical read-only data.

The split would be: `mmap` the `.se1` files once per process, refcounted, with
only cursors and derived state per context. That is where the real scalability
win is — and it is also where the concurrency bugs would be. Deliberately not
coupled to 3a–3d, and not started until they ship.

---

## 9. Open questions

1. **[DECIDE]** default context per-process (recommended) or per-thread — §4
2. **[DECIDE]** `swe_ctx_new()` inherits config (recommended) or starts clean — §5
3. Do all 75 get `_r` variants, or only the ~25 that a threaded caller actually
   needs (`calc`, `calc_ut`, `houses*`, `fixstar*`, the setters, `close`)?
   Shipping all 75 is more code but avoids a second migration later.
   **Recommendation: all 75** — a partial surface means callers hit a wall.
4. Is `swe_ctx` the right name, or `swe_context` / `swe_handle`?
5. Should `swe_ctx_free(NULL)` be a no-op (like `free`)? **Recommendation: yes.**
