# Config Read/Write Map — input to Phase 2

**Purpose:** resolve the three open design decisions in
[PLAN.md](PLAN.md) §14.6 before `struct swe_config` is written.
**Companion to:** [PLAN.md](PLAN.md), [INVESTIGATION.md](INVESTIGATION.md)

---

## 1. Method, and how far to trust it

Every `swed.<field>` occurrence in the nine library sources was classified as
read or write and attributed to its enclosing function.

**The first version of this tool was wrong**, and it matters that the record
says so. It attributed writes by counting braces, which `#if`/`#else` blocks
corrupt: it placed `sweph.c:7452` in `swe_get_planet_name` when the line is
actually inside `open_jpl_file`. The mapper now keys on this codebase's actual
layout — every top-level definition starts at column 0, every one ends with a
`}` at column 0 — and was validated against eight lines read by eye before any
of its output was used. Two of my own expectations turned out to be the wrong
half of the disagreement.

**Known remaining limitation:** the classifier does not follow pointer
aliases. `struct sid_data *sip = &swed.sidd;` followed by `sip->sid_mode = ...`
reads as no write at all. Every alias target was therefore enumerated
separately (`grep -o '&swed\.[a-z_]*'`) and chased by hand:

```
48 &swed.pldat   46 &swed.oec    14 &swed.nddat   14 &swed.fidat
11 &swed.sidd     9 &swed.nut     4 &swed.savedat  4 &swed.nutv
 2 &swed.astro_models             1 &swed.topd
```

Of those, only `sidd`, `astro_models` and `topd` are configuration; the rest
are cache. All three were inspected directly.

---

## 2. Classification

### 2.1 Configuration — belongs in the shared master

Written **only** by public setters. Safe to share, propagate on generation change.

| Field | Written by | Notes |
|---|---|---|
| `ephepath` | `swe_set_ephe_path`, `swi_init_swed_if_start` (default) | 15 reads, all `swi_fopen(..., swed.ephepath, ...)` |
| `ephe_path_is_set` | `swe_set_ephe_path`, `swe_close` | read once, in `swecalc` |
| `jplfnam` | `swe_set_jpl_file`, `swi_init_swed_if_start`, `open_jpl_file`¹ | ¹ fallback DE431→DE406 |
| `topd.geolon/geolat/geoalt` | `swe_set_topo` **only** | see §2.3 — the struct is mixed |
| `geopos_is_set` | `swe_set_topo`, `swe_close` | |
| `sidd` (all 4 members) | `swe_set_sid_mode` **only**, via the `sip` alias | pure config struct |
| `ayana_is_set` | `swe_set_sid_mode`, `swe_close` | |
| `delta_t_userdef` | `swe_set_delta_t_userdef` | read only by `swe_deltat_ex` |
| `delta_t_userdef_is_set` | `swe_set_delta_t_userdef` | |
| `is_tid_acc_manual` | `swe_set_tid_acc` | the discriminator for `tid_acc`, §3.3 |
| `do_interpolate_nut` | `swe_set_interpolate_nut` | |
| `const_lapse_rate` | `swe_set_lapse_rate` | **lives outside `swed`**, `swecl.c:74` |

### 2.2 Derived / per-thread — must NOT be shared

Written from compute paths, not setters. Sharing these would create races that
do not exist today.

| Field | Written by | Why it is not config |
|---|---|---|
| `last_epheflag` | `swe_calc`, `swi_fixstar_calc_from_record`, `swe_set_ephe_path`, `swe_close` | **3 of 4 writers are compute paths.** Tracks which ephemeris the *current* call used |
| `jpl_file_is_open` | 10 sites: `jplplan`, `app_pos_etc_plan/_sun/_moon`, `swecalc`, … | open-resource state, per-thread by nature |
| `jpldenum` | `open_jpl_file`, `swi_close_keep_topo_etc`, `swe_close` | read back off the opened file |
| `topd.teval/tjd_ut/xobs[6]` | `swi_get_observer` | computed observer vector, keyed on `teval` |
| `interpol.*` | `swi_nutation` (compute), invalidated by `swe_set_interpolate_nut` | pure cache |
| `astro_models[]` | `swe_set_sid_mode` **and `get_aya_correction`** | see §3.4 — save/restore |
| `oec`, `oec2000`, `nut`, `nut2000`, `nutv` | compute paths | keyed by `teps`/`tnut` |
| `pldat[]`, `nddat[]`, `savedat[]`, `fidat[]`, `gcdat` | compute paths | caches and open files |
| `dpsi`, `deps`, `eop_*` | `load_dpsi_deps` | heap-owned |
| `fixfp`, `fixed_stars`, `n_fixstars_*` | star loading | |

### 2.3 `struct topo_data` is mixed and must be split

```c
struct topo_data {
  double geolon, geolat, geoalt;   /* CONFIG  -- swe_set_topo only        */
  double teval;                    /* CACHE   -- swi_get_observer         */
  double tjd_ut;                   /* CACHE                               */
  double xobs[6];                  /* CACHE   -- computed observer vector */
};
```

Putting the whole struct in shared config races on `xobs`/`teval`, which every
`app_pos_etc_*` writes. Putting it all per-thread means `swe_set_topo()` never
propagates — the exact bug we are fixing. **The struct has to be split.**

Checked for the same defect elsewhere: `struct sid_data` is pure config,
`struct epsilon` and `struct interpol` are pure cache. `topd` is the only mixed one.

---

## 3. The three open decisions, resolved

### 3.1 Is `last_epheflag` config or derived? → **Derived, per-thread**

Four writers, and only one is a setter:

```
sweph.c:402   swe_calc                        <- compute
sweph.c:1349  swe_set_ephe_path               <- setter
sweph.c:6443  swi_fixstar_calc_from_record    <- compute
sweph.c:1270  swe_close
```

It records which ephemeris the current call resolved to, so it can detect a
switch and drop files opened for the old one (`sweph.c:389-403`). That is
per-thread bookkeeping about per-thread file handles. It must stay in the
per-thread block, and the `sweph.c:389-403` invalidation stays a thread-local
operation on thread-local `fidat[]`.

**Resolves PLAN §14.1.**

### 3.2 Re-entrancy of `swe_set_ephe_path` → `swe_calc` → sync

Confirmed live: `sweph.c:1350` runs a full Moon `swe_calc()` inside the setter.
Rules for the design:

1. `swi_sync_config()` must **never** be called with the config mutex held.
   The setter's sequence is: take mutex → update master → bump generation →
   release mutex → *then* do local work including the Moon `swe_calc()`.
2. A per-thread `in_config_apply` flag makes the sync hook a no-op while a
   setter is applying config on that thread. This is a re-entrancy guard, not
   a recursive mutex — it must not be possible to take the mutex twice.
3. Existing behaviour to preserve: `swed.ephe_path_is_set = TRUE` is set at
   `sweph.c:1328`, *before* the `swe_calc()` at 1350. That is what stops the
   lazy re-init at `sweph.c:639` from recursing today. Keep that ordering.

**Resolves PLAN §14.2.**

### 3.3 Does `swi_set_tid_acc()`'s write bump the generation? → **No**

`swephlib.c:3242`:

```c
if (swed.is_tid_acc_manual)
  return retc;                    /* user's value wins, no write at all */
retc = swi_get_tid_acc(tjd_ut, iflag, denum, &denumret, &(swed.tid_acc), serr);
```

So `tid_acc` is dual-natured, and `is_tid_acc_manual` is the discriminator:

- **manual** (`swe_set_tid_acc`) → user config, shared, propagates,
  bumps the generation, and `swi_set_tid_acc()` is a no-op.
- **automatic** → **derived** from the DE number of whichever ephemeris file
  this thread opened. Per-thread. Must not bump the generation.

Bumping on the automatic write would be a live-lock: every thread's file-open
would publish a new generation, forcing every other thread to resync forever.

**Resolves PLAN §14.3.**

### 3.4 New finding — `get_aya_correction` save/restores `astro_models`

Not in the original question list; found while mapping, and it would have been
a silent wrong-answer bug.

```c
sweph.c:2966  int prec_model       = swed.astro_models[SE_MODEL_PREC_LONGTERM];
sweph.c:2967  int prec_model_short = swed.astro_models[SE_MODEL_PREC_SHORTTERM];
   ...
sweph.c:2988  swed.astro_models[SE_MODEL_PREC_LONGTERM]  = prec_offset;   /* override */
sweph.c:2989  swed.astro_models[SE_MODEL_PREC_SHORTTERM] = prec_offset;
   ...compute...
sweph.c:2991  swed.astro_models[SE_MODEL_PREC_LONGTERM]  = prec_model;    /* restore */
sweph.c:2992  swed.astro_models[SE_MODEL_PREC_SHORTTERM] = prec_model_short;
```

**A compute function temporarily mutates a config array.** If `astro_models`
were shared, thread A's override window would be visible to thread B, which
would compute with the wrong precession model and return a plausible wrong
answer. Today this is safe only because `swed` is thread-local.

This is not an argument against sharing config — it is an argument for the
shape already chosen: **shared master + per-thread working copy.**
`get_aya_correction` writes `swed.astro_models`, which in the new design is the
*thread's* copy. The master is untouched and no generation is bumped.

It does impose a hard rule:

> Compute paths write only the per-thread working copy. The master is written
> exclusively by the public `swe_set_*` functions. Nothing on a compute path
> may ever bump the generation.

`swe_set_sid_mode` separately and *permanently* writes `astro_models`
(`sweph.c:2916-2924`) when `SE_SIDBIT_PREC_ORIG` is set — config writing
config, inside a setter, which is fine provided both fields are published
under the same lock and single generation bump.

---

## 4. Cache invalidation: the primitive already exists

`swi_force_app_pos_etc()` (`sweph.c`) is the library's existing invalidation
routine:

```c
for (i = 0; i < SEI_NPLANETS;   i++) swed.pldat[i].xflgs   = -1;
for (i = 0; i < SEI_NNODE_ETC;  i++) swed.nddat[i].xflgs   = -1;
for (i = 0; i <= SE_NPLANETS;   i++) { swed.savedat[i].tsave = 0;
                                       swed.savedat[i].iflgsave = -1; }
```

Called from five places, all after a config change: `sweph.c:440` (epheflag
switch), `:2930` (`swe_set_sid_mode`), `:7269` (`swe_set_topo`), and
`swecl.c:5113, 5394`.

So the "invalidate everything conservatively on generation change" step
(PLAN §7.3 step 2.5) is **not new code** — it is `swi_force_app_pos_etc()`
plus the `interpol` reset that `swe_set_interpolate_nut` already performs, plus
`fidat[]` teardown when `ephepath` changed.

**Config → cache dependency table** (what a sync must invalidate):

| Config changed | Must invalidate |
|---|---|
| `ephepath`, `jplfnam` | `fidat[]` (close files), `jpl_file_is_open`, `jpldenum`, `last_epheflag`, then `swi_force_app_pos_etc()` |
| `sidd`, `astro_models` | `swi_force_app_pos_etc()` — matches `sweph.c:2930` |
| `topd.geo*` | `topd.teval = 0` (forces observer recompute), `swi_force_app_pos_etc()` — matches `sweph.c:7266-7269` |
| `tid_acc` (manual) | Δt-dependent results: `swi_force_app_pos_etc()` |
| `delta_t_userdef*` | same |
| `do_interpolate_nut` | `interpol.*` = 0 — already done by the setter |
| `const_lapse_rate` | nothing — read directly at use sites, no cache |

Note `swe_set_topo` already sets `topd.teval = 0` at `sweph.c:7266` and has an
early-out at `:7256-7258` when the position is unchanged. Both behaviours must
survive into the shared design.

---

## 5. Consequences for Phase 2

1. `struct swe_config` holds the **12 fields in §2.1** — *not* the ~22 first
   estimated in PLAN §11. `last_epheflag`, `jpldenum`, `jpl_file_is_open` and
   the cache half of `topd` move to the per-thread block instead.
2. `struct topo_data` must be split into a config part and a cache part (§2.3).
3. `const_lapse_rate` moves from `swecl.c:74` into the config struct.
4. Compute paths write only the thread's working copy; never the master, never
   the generation (§3.4).
5. `swi_force_app_pos_etc()` is the invalidation primitive to reuse (§4).
6. Re-entrancy: mutex released before any `swe_calc()`; per-thread
   `in_config_apply` guard; preserve the `ephe_path_is_set`-before-`swe_calc`
   ordering at `sweph.c:1328`/`1350` (§3.2).

**Not yet decided, and not blocking the struct definition:** whether a thread
that has explicitly called a setter itself keeps that value against later
global changes (the `cfg_local_mask` idea in PLAN §7.1). §3.4 shows the
per-thread working copy is required regardless, so the mask is a policy choice
layered on top, and can be added after the mechanism works.
