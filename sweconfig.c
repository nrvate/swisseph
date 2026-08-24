/************************************************************
  $Header: sweconfig.c $

  Shared configuration for the Swiss Ephemeris -- see sweconfig.h for the
  design and for why the configuration is a separate struct from swed.

************************************************************/

#include "swejpl.h"
#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"

/*======================================================================
 * The master. Process-global, guarded by cfg_mutex.
 *
 * cfg_master.generation is the published version number. It starts at 0,
 * and swi_config_publish() bumps it to 1 on the first setter call, so a
 * thread whose ctx->cfg_seen == 0 has by definition never synced.
 *====================================================================*/
#ifdef SWI_NEEDS_GEN_FALLBACK_MUTEX
/* The generation counter's stand-in lock, on toolchains with neither the
 * __atomic builtins nor C11 <stdatomic.h>.
 *
 * swethread.h declares this extern and calls it, and NO translation unit in
 * the library defined it -- so tier 5 could not link. The fallback that
 * exists to serve Sun Studio and IBM XL (named in sweodef.h's TLS block)
 * was dead on arrival, and nothing noticed because gcc and clang pick tier
 * 3 at every -std=, c89 included. tests/check-threadtiers now builds all
 * five. */
swi_mutex_t swi_gen_fallback_mutex = SWI_MUTEX_INIT;
#endif

static swi_mutex_t cfg_mutex = SWI_MUTEX_INIT;
static struct swe_config cfg_master;
/* Validity is "generation != 0" rather than a separate flag: the generation
 * is already read atomically on the fast path, whereas a plain AS_BOOL read
 * outside the mutex is a data race -- ThreadSanitizer caught exactly that. */

/* ctx->cfg_seen, ctx->cfg_local and ctx->cfg_applying used to live here as TLS. They are
 * fields of swe_ctx as of Phase 3d: they describe one context's relationship
 * to the master, not one thread's, and tying them to the thread would have
 * made two contexts on the same thread share a single ctx->cfg_local -- which is
 * exactly the independence Phase 3 exists to provide.
 */

/* Re-entrancy guard.
 *
 * swe_set_ephe_path() runs a full Moon swe_calc() at sweph.c:1350 to learn
 * the DE number, and swe_calc() calls the sync hook. Without this flag the
 * sync would run in the middle of a setter applying configuration -- at
 * best wasted work, at worst a second attempt to take cfg_mutex.
 *
 * This is a flag, not a recursive mutex, deliberately: the mutex is never
 * held across any of that, so there is nothing to recurse into.
 */

/*======================================================================
 * swed  ->  struct swe_config
 *
 * Reads only the config half of topd; teval/tjd_ut/xobs[] are the
 * computed observer vector and stay per-thread.
 *====================================================================*/
void swi_config_capture(swe_ctx *ctx, struct swe_config *c)
{
  memcpy(c->ephepath, ctx->ephepath, sizeof(c->ephepath));
  memcpy(c->jplfnam,  ctx->jplfnam,  sizeof(c->jplfnam));
  c->sidd = ctx->sidd;
  memcpy(c->astro_models, ctx->astro_models, sizeof(c->astro_models));
  c->geolon                 = ctx->topd.geolon;
  c->geolat                 = ctx->topd.geolat;
  c->geoalt                 = ctx->topd.geoalt;
  c->tid_acc                = ctx->tid_acc;
  c->delta_t_userdef        = ctx->delta_t_userdef;
  c->const_lapse_rate       = ctx->const_lapse_rate;
  c->ephe_path_is_set       = ctx->ephe_path_is_set;
  c->geopos_is_set          = ctx->geopos_is_set;
  c->ayana_is_set           = ctx->ayana_is_set;
  c->is_tid_acc_manual      = ctx->is_tid_acc_manual;
  c->delta_t_userdef_is_set = ctx->delta_t_userdef_is_set;
  c->do_interpolate_nut     = ctx->do_interpolate_nut;
  /* generation is owned by the master, not captured from the context */
}

/*======================================================================
 * struct swe_config  ->  swed, plus cache invalidation
 *
 * Only the caches that the changed fields actually feed are dropped. The
 * dependency table is in notes/CONFIG-MAP.md section 4 and mirrors what the
 * existing setters already do:
 *
 *   ephepath/jplfnam -> close ephemeris files, then force_app_pos_etc
 *                       (matches swe_set_ephe_path -> swi_close_keep_topo_etc)
 *   sidd/astro_models -> force_app_pos_etc   (matches sweph.c:2930)
 *   geo*              -> topd.teval = 0, force_app_pos_etc
 *                                        (matches sweph.c:7266-7269)
 *   tid_acc/delta_t   -> force_app_pos_etc
 *   do_interpolate_nut-> reset interpol
 *                        (matches swe_set_interpolate_nut)
 *   const_lapse_rate  -> nothing; read directly at its use sites
 *====================================================================*/
AS_BOOL swi_config_apply(swe_ctx *ctx, const struct swe_config *c, int32 groups)
{
  AS_BOOL path_changed, geo_changed, model_changed, dt_changed, nut_changed;
  AS_BOOL any;

  path_changed  = (groups & SWI_CFG_PATH)
                  && (strcmp(ctx->ephepath, c->ephepath) != 0
                   || strcmp(ctx->jplfnam, c->jplfnam) != 0
                   || ctx->ephe_path_is_set != c->ephe_path_is_set);
  geo_changed   = (groups & SWI_CFG_TOPO)
                  && (ctx->topd.geolon != c->geolon
                   || ctx->topd.geolat != c->geolat
                   || ctx->topd.geoalt != c->geoalt
                   || ctx->geopos_is_set != c->geopos_is_set);
  model_changed = (groups & SWI_CFG_SID)
                  && (memcmp(&ctx->sidd, &c->sidd, sizeof(ctx->sidd)) != 0
                   || memcmp(ctx->astro_models, c->astro_models,
                             sizeof(ctx->astro_models)) != 0
                   || ctx->ayana_is_set != c->ayana_is_set);
  dt_changed    = ((groups & SWI_CFG_TIDACC)
                   && (ctx->tid_acc != c->tid_acc
                    || ctx->is_tid_acc_manual != c->is_tid_acc_manual))
                  || ((groups & SWI_CFG_DELTAT)
                   && (ctx->delta_t_userdef != c->delta_t_userdef
                    || ctx->delta_t_userdef_is_set != c->delta_t_userdef_is_set));
  nut_changed   = (groups & SWI_CFG_NUT)
                  && (ctx->do_interpolate_nut != c->do_interpolate_nut);

  any = (path_changed || geo_changed || model_changed || dt_changed
         || nut_changed
         || ((groups & SWI_CFG_LAPSE)
             && ctx->const_lapse_rate != c->const_lapse_rate));
  if (!any)
    return FALSE;

  /* --- copy the values in, group by group ---------------------------- */
  if (groups & SWI_CFG_PATH) {
    memcpy(ctx->ephepath, c->ephepath, sizeof(ctx->ephepath));
    memcpy(ctx->jplfnam,  c->jplfnam,  sizeof(ctx->jplfnam));
    ctx->ephe_path_is_set = c->ephe_path_is_set;
  }
  if (groups & SWI_CFG_SID) {
    ctx->sidd = c->sidd;
    memcpy(ctx->astro_models, c->astro_models, sizeof(ctx->astro_models));
    ctx->ayana_is_set = c->ayana_is_set;
  }
  if (groups & SWI_CFG_TOPO) {
    ctx->topd.geolon   = c->geolon;
    ctx->topd.geolat   = c->geolat;
    ctx->topd.geoalt   = c->geoalt;
    ctx->geopos_is_set = c->geopos_is_set;
  }
  if (groups & SWI_CFG_TIDACC) {
    ctx->tid_acc           = c->tid_acc;
    ctx->is_tid_acc_manual = c->is_tid_acc_manual;
  }
  if (groups & SWI_CFG_DELTAT) {
    ctx->delta_t_userdef        = c->delta_t_userdef;
    ctx->delta_t_userdef_is_set = c->delta_t_userdef_is_set;
  }
  if (groups & SWI_CFG_LAPSE)
    ctx->const_lapse_rate = c->const_lapse_rate;
  if (groups & SWI_CFG_NUT)
    ctx->do_interpolate_nut = c->do_interpolate_nut;

  /* --- invalidate what that made stale ------------------------------ */
  if (path_changed) {
    /* This thread's own file handles only. Another thread's fidat[] is
     * its own business -- these are per-thread FILE*, not shared. */
    if (ctx->jpl_file_is_open) {
      swi_close_jpl_file(ctx);
      ctx->jpl_file_is_open = FALSE;
    }
    swi_close_ephe_files(ctx, FORGET_DENUM);
    ctx->last_epheflag = 0;
  }
  if (geo_changed)
    ctx->topd.teval = 0;        /* force swi_get_observer(ctx) to recompute */
  if (nut_changed) {
    ctx->interpol.tjd_nut0 = 0;
    ctx->interpol.tjd_nut2 = 0;
    ctx->interpol.nut_dpsi0 = 0;
    ctx->interpol.nut_dpsi1 = 0;
    ctx->interpol.nut_dpsi2 = 0;
    ctx->interpol.nut_deps0 = 0;
    ctx->interpol.nut_deps1 = 0;
    ctx->interpol.nut_deps2 = 0;
  }
  if (path_changed || geo_changed || model_changed || dt_changed)
    swi_force_app_pos_etc(ctx);

  return TRUE;
}

/*======================================================================
 * Merge only the named groups of *src into *dst.
 *
 * Publishing must not touch groups the caller did not set. Without this,
 * a thread calling swe_set_ephe_path() would also write its own sidereal
 * mode into the master, and every thread that had not set one locally
 * would then adopt it.
 *====================================================================*/
static void cfg_merge(struct swe_config *dst, const struct swe_config *src,
                      int32 groups)
{
  if (groups & SWI_CFG_PATH) {
    memcpy(dst->ephepath, src->ephepath, sizeof(dst->ephepath));
    memcpy(dst->jplfnam,  src->jplfnam,  sizeof(dst->jplfnam));
    dst->ephe_path_is_set = src->ephe_path_is_set;
  }
  if (groups & SWI_CFG_SID) {
    dst->sidd = src->sidd;
    memcpy(dst->astro_models, src->astro_models, sizeof(dst->astro_models));
    dst->ayana_is_set = src->ayana_is_set;
  }
  if (groups & SWI_CFG_TOPO) {
    dst->geolon = src->geolon;
    dst->geolat = src->geolat;
    dst->geoalt = src->geoalt;
    dst->geopos_is_set = src->geopos_is_set;
  }
  if (groups & SWI_CFG_TIDACC) {
    dst->tid_acc = src->tid_acc;
    dst->is_tid_acc_manual = src->is_tid_acc_manual;
  }
  if (groups & SWI_CFG_DELTAT) {
    dst->delta_t_userdef = src->delta_t_userdef;
    dst->delta_t_userdef_is_set = src->delta_t_userdef_is_set;
  }
  if (groups & SWI_CFG_LAPSE)
    dst->const_lapse_rate = src->const_lapse_rate;
  if (groups & SWI_CFG_NUT)
    dst->do_interpolate_nut = src->do_interpolate_nut;
}

/*======================================================================
 * Publish this thread's configuration as the new master.
 *
 * Called at the END of each swe_set_*(), after swed already holds the new
 * value. Note the ordering: the master is written and the generation
 * bumped under the lock, and ctx->cfg_seen is set to the value we just
 * published so this thread does not then re-sync its own change.
 *====================================================================*/
void swi_config_publish(swe_ctx *ctx, int32 groups)
{
  struct swe_config tmp;

  /* Only the process-wide default context publishes.
   *
   * The master exists so that threads SHARING the default context see each
   * other's swe_set_*() calls -- that is all of Phase 2. An explicit
   * context created with swe_ctx_new() is independent by contract
   * (notes/PHASE3-API.md section 3.1), so swe_set_topo_r(myctx, ...) must
   * change myctx and nothing else. Without this test the _r setters would
   * have inherited the publish and broadcast a private context's
   * configuration to every other one -- the exact leak Phase 2 fixed for
   * internal callers, reintroduced through the new API.
   *
   * The group is still claimed below, so this context stops tracking the
   * master for it either way. */
  if (ctx != swi_default_ctx()) {
    ctx->cfg_local |= groups;
    return;
  }

  /* Nested call: a setter invoked from inside another setter, or from
   * swi_init_swed_if_start(ctx), which calls swe_set_tid_acc(). Only the
   * outermost call publishes.
   *
   * This is not an optimisation. Without it, a thread starting up would
   * run swi_init_swed_if_start(ctx) -> swe_set_tid_acc() -> publish, and
   * broadcast its *freshly zeroed* swed -- empty ephepath and all -- as
   * the new master, overwriting configuration another thread had
   * legitimately set. */
  if (ctx->cfg_applying)
    return;

  ctx->cfg_local |= groups;          /* this thread now owns these groups */
  swi_config_capture(ctx, &tmp);
  swi_mutex_lock(&cfg_mutex);
  /* merge, do not overwrite: only the groups this setter owns */
  cfg_merge(&cfg_master, &tmp, groups);
  swi_gen_bump(&cfg_master.generation);
  swi_mutex_unlock(&cfg_mutex);

  /* Deliberately NOT: ctx->cfg_seen = <the new generation>.
   *
   * Publishing one group says nothing about the others. A worker that
   * calls swe_set_ephe_path() as its very first action has claimed PATH,
   * but has not yet adopted the sidereal mode, observer position or tidal
   * acceleration the main thread configured. Marking it fully synced here
   * would freeze it on its own startup defaults for every group it did
   * not publish -- silently, and permanently.
   *
   * Leaving ctx->cfg_seen stale makes the next swi_config_sync(ctx) pull in
   * everything except the groups this thread now owns (ctx->cfg_local), which
   * is exactly right. */
}

/*======================================================================
 * Adopt the master if it has moved on. The fast path -- by far the common
 * one -- is a single acquire load and a compare.
 *====================================================================*/
void swi_config_sync(swe_ctx *ctx)
{
  struct swe_config tmp;
  swi_gen_t g;

  if (ctx->cfg_applying)             /* re-entered from inside a setter */
    return;
  /* Same reasoning as swi_config_publish(): only the default context
   * tracks the master. An explicit context inherits the configuration once,
   * at swe_ctx_new(), and is its own from then on -- otherwise a later
   * swe_set_sid_mode() on the main thread would silently retune every
   * context that had not overridden that group, which is precisely the
   * independence Phase 3 promises.
   *
   * swe_ctx_new() therefore cannot use this function to inherit; it calls
   * swi_config_inherit() below. */
  if (ctx != swi_default_ctx())
    return;
  g = swi_gen_load(&cfg_master.generation);
  if (g == 0)                   /* nobody has published anything yet */
    return;
  if (g == ctx->cfg_seen)
    return;

  swi_mutex_lock(&cfg_mutex);
  tmp = cfg_master;
  g = cfg_master.generation;
  swi_mutex_unlock(&cfg_mutex);

  /* Apply outside the lock: swi_config_apply(ctx) closes files and drops
   * caches, and must not run with cfg_mutex held. */
  ctx->cfg_applying = TRUE;
  swi_config_apply(ctx, &tmp, SWI_CFG_ALL & ~ctx->cfg_local);
  ctx->cfg_applying = FALSE;
  ctx->cfg_seen = g;
}

/*======================================================================
 * One-shot adoption of the master, for swe_ctx_new().
 *
 * swi_config_sync() deliberately refuses to act on a non-default context;
 * this is the single point where a fresh one takes the current published
 * configuration on. It claims nothing: the new context has expressed no
 * opinion of its own yet.
 *====================================================================*/
void swi_config_inherit(swe_ctx *ctx)
{
  struct swe_config tmp;
  swi_gen_t g;

  g = swi_gen_load(&cfg_master.generation);
  if (g == 0)                   /* nobody has published anything yet */
    return;
  swi_mutex_lock(&cfg_mutex);
  tmp = cfg_master;
  g   = cfg_master.generation;
  swi_mutex_unlock(&cfg_mutex);

  ctx->cfg_applying = TRUE;     /* applying must not publish back */
  swi_config_apply(ctx, &tmp, SWI_CFG_ALL);
  ctx->cfg_applying = FALSE;
  ctx->cfg_seen = g;
}

/*======================================================================
 * swe_close() drops this thread's state; the master must go with it, or a
 * later thread would resurrect the closed configuration.
 *====================================================================*/
void swi_config_reset(swe_ctx *ctx)
{
  swi_mutex_lock(&cfg_mutex);
  memset((void *) &cfg_master, 0, sizeof(cfg_master));
  swi_mutex_unlock(&cfg_mutex);
  ctx->cfg_seen = 0;
  ctx->cfg_local = 0;
}

/*======================================================================
 * Claim ownership of `groups` without publishing.
 *
 * Needed on the early-return paths of setters that no-op when the value is
 * unchanged. Calling swe_set_topo() with the position it already holds is
 * still the caller stating an opinion, and must stop this thread tracking
 * the global value -- otherwise another thread's later change leaks in.
 *
 * That is not hypothetical: it is exactly how tests/golden.c --threads
 * failed. A worker synced and adopted the main thread's topocentric
 * position, then called swe_set_topo() with that same position, hit the
 * unchanged early-return, never claimed the group, and silently followed
 * the main thread to the *next* site mid-loop.
 *====================================================================*/
void swi_config_claim(swe_ctx *ctx, int32 groups)
{
  if (!ctx->cfg_applying)
    ctx->cfg_local |= groups;
}

/* Guard accessors, so the setters can bracket their own work without
 * needing to see ctx->cfg_applying. */
AS_BOOL swi_config_begin_apply(swe_ctx *ctx)
{
  AS_BOOL was = ctx->cfg_applying;
  ctx->cfg_applying = TRUE;
  return was;
}

void swi_config_end_apply(swe_ctx *ctx, AS_BOOL was)
{
  ctx->cfg_applying = was;
}
