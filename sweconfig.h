/************************************************************
  $Header: sweconfig.h $

  Shared configuration for the Swiss Ephemeris.

  THE PROBLEM
  -----------
  Since 2.03 the global `swed` blob has been TLS (__thread), which removes
  data races but makes the library thread-hostile in a subtler way: every
  swe_set_*() writes thread-local state, so configuration applied on one
  thread is invisible to every other. A worker thread silently falls back
  to the default ephemeris path and the default sidereal mode, and returns
  a plausible wrong answer with a success code. See notes/INVESTIGATION.md.

  THE SHAPE
  ---------
  One process-global master copy of the configuration, guarded by a mutex
  and carrying a generation counter. Each thread keeps its own working
  copy -- which is simply the existing fields in `swed`, untouched and
  unmoved -- plus the generation it last synced from.

      swe_set_*()   -> lock, write master, bump generation, unlock,
                       then apply to this thread
      compute path  -> if master generation != mine, re-apply and
                       invalidate the dependent caches

  The steady-state read is one relaxed atomic load and a compare; no lock
  is taken unless the configuration actually changed.

  WHY A PER-THREAD WORKING COPY, NOT DIRECT READS OF THE MASTER
  -------------------------------------------------------------
  Because compute paths mutate configuration. get_aya_correction()
  (sweph.c:2966-2992) saves astro_models[], overrides it for the duration
  of a calculation, and restores it. If threads read the master directly,
  thread A's override window would be visible to thread B, which would
  compute with the wrong precession model. The working copy makes that
  override thread-local, as it is today.

  Hence the invariant this file exists to enforce:

      Compute paths write only the thread's own copy. The master is
      written exclusively by the public swe_set_* functions, and nothing
      on a compute path ever bumps the generation.

  WHAT IS AND IS NOT CONFIGURATION
  --------------------------------
  Derived from the read/write map in notes/CONFIG-MAP.md. Deliberately
  NOT here, despite living in `swed`:

    last_epheflag      written by swe_calc() and
                       swi_fixstar_calc_from_record() -- per-thread record
                       of which ephemeris the current call resolved to
    jpl_file_is_open   open-resource state, 10 writers, all compute paths
    jpldenum           read back off the file this thread opened
    topd.teval/tjd_ut/xobs[]
                       the computed observer vector, written by
                       swi_get_observer() on every app_pos_etc_* call

  That last one is why the geographic position appears here as three bare
  doubles rather than as `struct topo_data`: the struct is half config and
  half cache, and only the config half may be shared.

  tid_acc is dual. When is_tid_acc_manual is set it is the user's value
  and belongs to the master. Otherwise swi_set_tid_acc() derives it from
  the DE number of whichever file this thread opened -- per-thread, and
  emphatically not a generation bump, or every file open would force every
  other thread to resync forever.

************************************************************/

#ifndef _SWECONFIG_INCLUDED
#define _SWECONFIG_INCLUDED

#include "sweodef.h"
#include "swethread.h"

/* sweph.h defines struct sid_data and SEI_NMODELS, and includes this file
 * after them. */

struct swe_config {
  /* --- strings ------------------------------------------------------ */
  char ephepath[AS_MAXCH];        /* swe_set_ephe_path()               */
  char jplfnam[AS_MAXCH];         /* swe_set_jpl_file()                */

  /* --- sidereal ----------------------------------------------------- */
  struct sid_data sidd;           /* swe_set_sid_mode(); pure config    */

  /* --- models ------------------------------------------------------- */
  int32 astro_models[SEI_NMODELS];/* swe_set_astro_models(),
                                   * and swe_set_sid_mode() when
                                   * SE_SIDBIT_PREC_ORIG is set         */

  /* --- observer: the CONFIG half of struct topo_data ---------------- */
  double geolon, geolat, geoalt;  /* swe_set_topo()                     */

  /* --- scalars ------------------------------------------------------ */
  double tid_acc;                 /* only when is_tid_acc_manual        */
  double delta_t_userdef;         /* swe_set_delta_t_userdef()          */
  double const_lapse_rate;        /* swe_set_lapse_rate()               */

  /* --- flags -------------------------------------------------------- */
  AS_BOOL ephe_path_is_set;
  AS_BOOL geopos_is_set;
  AS_BOOL ayana_is_set;
  AS_BOOL is_tid_acc_manual;
  AS_BOOL delta_t_userdef_is_set;
  AS_BOOL do_interpolate_nut;

  /* Bumped on every published change. Compared with != only, so wrapping
   * after 2^32 configuration changes is harmless. Never written by a
   * compute path. */
  swi_gen_t generation;
};

/* Field groups.
 *
 * Two idioms have to coexist:
 *
 *   (a) configure once on the main thread, compute on workers
 *       -- the thing Phase 2 exists to fix;
 *   (b) configure independently on every thread
 *       -- what callers do TODAY as a workaround, and what the sidereal
 *          and topocentric sections of tests/golden.c do.
 *
 * A single shared master satisfies (a) but breaks (b): two threads setting
 * different sidereal modes would clobber each other.
 *
 * The rule that satisfies both: once a thread has explicitly set a group
 * itself, it stops tracking the global value of that group. It has stated
 * an opinion, so nobody else's opinion overrides it. Groups it never
 * touched keep following the master.
 *
 * Granularity is per group rather than per field because the fields within
 * a group are set together by one public function and are meaningless
 * apart -- ephepath without ephe_path_is_set, sidd without ayana_is_set.
 */
#define SWI_CFG_PATH     0x01   /* ephepath, jplfnam, ephe_path_is_set   */
#define SWI_CFG_TOPO     0x02   /* geolon/geolat/geoalt, geopos_is_set   */
#define SWI_CFG_SID      0x04   /* sidd, ayana_is_set, astro_models      */
#define SWI_CFG_TIDACC   0x08   /* tid_acc, is_tid_acc_manual            */
#define SWI_CFG_DELTAT   0x10   /* delta_t_userdef*                      */
#define SWI_CFG_NUT      0x20   /* do_interpolate_nut                    */
#define SWI_CFG_LAPSE    0x40   /* const_lapse_rate                      */
#define SWI_CFG_ALL      0x7f

/* Copy this thread's live configuration out of swed into *c.
 * Does not touch the cache half of topd. */
extern void swi_config_capture(struct swe_config *c);

/* Copy the groups in `groups` from *c into this thread's swed and
 * invalidate whatever the change makes stale. Returns TRUE if anything
 * actually changed. */
extern AS_BOOL swi_config_apply(const struct swe_config *c, int32 groups);

/* Publish this thread's current configuration as the new master, and mark
 * `groups` as locally owned so this thread stops tracking the global value
 * for them. Called by the swe_set_* functions after they have updated swed.
 * Takes the mutex; must never be called with it already held. A nested
 * call (one setter invoked from inside another) is a no-op. */
extern void swi_config_publish(int32 groups);

/* Call a public swe_set_* function for THIS THREAD ONLY.
 *
 * Compute paths call the public setters internally -- swecl.c calls
 * swe_set_topo() from nine places in the eclipse, occultation and
 * rise/set code, and swecalc() lazily calls swe_set_ephe_path(NULL) and
 * swe_set_sid_mode(). Those are implementation details of one
 * calculation, not the caller stating a configuration preference.
 *
 * Without this wrapper an eclipse calculation would publish its observer
 * position as the process-wide default and silently move every other
 * thread's topocentric results. ThreadSanitizer found it via the write in
 * swi_config_publish() reached from eclipse_how() -> swe_set_topo().
 */
#define SWI_CFG_LOCAL(stmt) do {                    \
    AS_BOOL swi__cfg_was = swi_config_begin_apply();\
    stmt;                                           \
    swi_config_end_apply(swi__cfg_was);             \
  } while (0)

/* Claim `groups` as locally owned without publishing. For the early-return
 * paths of setters that no-op when the value is unchanged. */
extern void swi_config_claim(int32 groups);

/* Fast path for compute entry points: if the master has moved on since
 * this thread last synced, adopt it. One atomic load in the common case. */
extern void swi_config_sync(void);

/* Re-arm the master from the compile-time defaults. Used by swe_close(). */
extern void swi_config_reset(void);

#endif /* _SWECONFIG_INCLUDED */
