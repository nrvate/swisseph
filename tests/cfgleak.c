/* cfgleak.c -- the Phase 2 stragglers, tested directly.
 *
 * The gate suite passing does not prove these two fixes work, because
 * neither is exercised by a transcript comparison.
 *
 * 1. INTERNAL SETTER CALLS MUST NOT PUBLISH.
 *    Compute functions call the public setters as an implementation
 *    detail: swecl.c calls swe_set_topo() from nine places in the
 *    eclipse/occultation/rise code, swehel.c from four in the heliacal
 *    code, and swe_houses_ex2() calls swe_set_sid_mode(). If any of those
 *    publish, one thread computing an eclipse or a heliacal rising at
 *    location X silently moves every other thread's observer to X.
 *
 *    Test: main configures Zurich. A worker runs heliacal, eclipse and
 *    house calculations at Cairo and other latitudes. Main's topocentric
 *    result must not move.
 *
 * 2. swe_close() MUST CLEAR THE SHARED MASTER.
 *    Otherwise a thread started afterwards adopts the configuration of a
 *    session the caller believes was released, and reopens its files.
 *
 * Build: see tests/Makefile (make cfgleak)
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "swephexp.h"

/* pthread_barrier_* is an OPTIONAL part of POSIX (_POSIX_BARRIERS) and Apple
 * has never implemented it -- on macOS this file failed to compile at all,
 * which is why the macOS thread gates had never run. Provide a barrier built
 * from a mutex and condition variable, which macOS does have.
 *
 * Only the three entry points this test uses, and only the semantics it
 * needs: a fixed participant count, reused across several rounds. The
 * generation counter is what makes reuse safe -- without it a thread that
 * loops back round can slip through the next barrier before the previous
 * cohort has finished waking. */
#if defined(__APPLE__) || !defined(_POSIX_BARRIERS)
typedef struct {
  pthread_mutex_t m;
  pthread_cond_t  c;
  unsigned        count, waiting, generation;
} se_barrier_t;

static int se_barrier_init(se_barrier_t *b, void *attr, unsigned count) {
  (void)attr;
  if (count == 0) return -1;
  pthread_mutex_init(&b->m, NULL);
  pthread_cond_init(&b->c, NULL);
  b->count = count; b->waiting = 0; b->generation = 0;
  return 0;
}
static int se_barrier_wait(se_barrier_t *b) {
  unsigned gen;
  pthread_mutex_lock(&b->m);
  gen = b->generation;
  if (++b->waiting == b->count) {
    b->waiting = 0;
    b->generation++;
    pthread_cond_broadcast(&b->c);
    pthread_mutex_unlock(&b->m);
    return 1;
  }
  while (gen == b->generation)
    pthread_cond_wait(&b->c, &b->m);
  pthread_mutex_unlock(&b->m);
  return 0;
}
static int se_barrier_destroy(se_barrier_t *b) {
  pthread_mutex_destroy(&b->m);
  pthread_cond_destroy(&b->c);
  return 0;
}
#else
typedef pthread_barrier_t se_barrier_t;
# define se_barrier_init    pthread_barrier_init
# define se_barrier_wait    pthread_barrier_wait
# define se_barrier_destroy pthread_barrier_destroy
#endif

static const char *EPHE = "../ephe";
static const double TJD = 2451545.0;

/* Zurich -- what the main thread configures and must keep. */
static const double ZUR[3] = { 8.55, 47.37, 400 };
/* Cairo -- what the worker's internal calculations use. */
static double CAI[3] = { 31.2, 30.0, 20 };

static double topo_moon(void) {
  char serr[AS_MAXCH] = ""; double x[6];
  swe_calc_ut(TJD, SE_MOON, SEFLG_SWIEPH | SEFLG_TOPOCTR, x, serr);
  return x[0];
}

static se_barrier_t bar;
static volatile double obs_before, obs_after;

/* The victim: a thread that INHERITS the observer position from main and
 * never sets one itself. Main is not a valid victim -- having called
 * swe_set_topo() it owns the group locally and sync skips it, so main is
 * immune whether or not the leak exists. That made the first version of
 * this test pass against a deliberately broken build. */
static void *observer(void *a) {
  (void)a;
  se_barrier_wait(&bar);      /* main configured */
  obs_before = topo_moon();        /* inherit Zurich */
  se_barrier_wait(&bar);      /* let the noisy thread run */
  se_barrier_wait(&bar);      /* noisy thread done */
  obs_after = topo_moon();
  se_barrier_wait(&bar);
  return NULL;
}

/* Runs calculations that internally call swe_set_topo/swe_set_sid_mode. */
static void *noisy_worker(void *a) {
  char serr[AS_MAXCH]; double dret[50], attr[20], tret[10];
  double datm[4] = {1013.25, 15, 40, 0}, dobs[6] = {36,1,1,1,1,1};
  char obj[AS_MAXCH];
  double cusp[37], ascmc[10];
  (void)a;

  se_barrier_wait(&bar);          /* main configured */
  se_barrier_wait(&bar);          /* observer took its reading */

  /* swehel.c: swe_heliacal_ut / _pheno_ut / vis_limit_mag -> swe_set_topo */
  strcpy(obj, "Venus"); serr[0] = 0;
  swe_heliacal_ut(TJD, CAI, datm, dobs, obj, SE_HELIACAL_RISING,
                  SEFLG_SWIEPH, dret, serr);
  strcpy(obj, "Venus"); serr[0] = 0;
  swe_vis_limit_mag(TJD, CAI, datm, dobs, obj, SEFLG_SWIEPH, dret, serr);
  strcpy(obj, "Mercury"); serr[0] = 0;
  swe_heliacal_pheno_ut(TJD, CAI, datm, dobs, obj, SE_HELIACAL_RISING,
                        SEFLG_SWIEPH, dret, serr);

  /* swecl.c: eclipse / rise-set -> swe_set_topo */
  serr[0] = 0;
  swe_sol_eclipse_how(TJD, SEFLG_SWIEPH, CAI, attr, serr);
  serr[0] = 0;
  swe_rise_trans(TJD, SE_SUN, NULL, SEFLG_SWIEPH, SE_CALC_RISE,
                 CAI, 1013.25, 15.0, tret, serr);
  serr[0] = 0;
  swe_lun_eclipse_how(TJD, SEFLG_SWIEPH, CAI, attr, serr);

  /* swehouse.c: swe_houses_ex2 -> swe_set_sid_mode when ayana unset */
  swe_houses_ex2(TJD, SEFLG_SWIEPH | SEFLG_SIDEREAL, -33.9, 151.2,
                 'P', cusp, ascmc, NULL, NULL, serr);

  se_barrier_wait(&bar);          /* observer takes second reading */
  se_barrier_wait(&bar);
  return NULL;
}

static int test_no_leak(void) {
  pthread_t noisy, obs;
  int bad = 0;
  swe_set_ephe_path((char *)EPHE);
  swe_set_topo(ZUR[0], ZUR[1], ZUR[2]);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);

  se_barrier_init(&bar, NULL, 3);
  pthread_create(&obs,   NULL, observer,     NULL);
  pthread_create(&noisy, NULL, noisy_worker, NULL);
  se_barrier_wait(&bar);   /* configured */
  se_barrier_wait(&bar);   /* observer read #1 */
  se_barrier_wait(&bar);   /* noisy done */
  se_barrier_wait(&bar);   /* observer read #2 */
  pthread_join(noisy, NULL);
  pthread_join(obs, NULL);
  se_barrier_destroy(&bar);

  printf("  inheriting thread, before noisy calcs : %.9f\n", obs_before);
  printf("  inheriting thread, after  noisy calcs : %.9f\n", obs_after);
  if (obs_before != obs_after) {
    printf("  FAIL: the worker's internal swe_set_topo() leaked "
           "(delta %.3e deg)\n", obs_after - obs_before);
    bad = 1;
  } else {
    printf("  PASS: no leak\n");
  }
  return bad;
}

/* ---- 2. swe_close() clears the master ------------------------------- */
static volatile double post_close_lon;
static volatile int    post_close_geoset;

static void *after_close(void *a) {
  char serr[AS_MAXCH] = ""; double x[6];
  (void)a;
  /* This thread never configures anything. If swe_close() cleared the
   * master, it must NOT see Zurich; swe_calc with TOPOCTR should fail
   * with "geographic position has not been set". */
  int32 rf = swe_calc_ut(TJD, SE_MOON, SEFLG_SWIEPH | SEFLG_TOPOCTR, x, serr);
  post_close_lon = x[0];
  post_close_geoset = (rf >= 0 && x[0] != 0.0);
  return NULL;
}

static int test_close_resets(void) {
  pthread_t t;
  swe_set_ephe_path((char *)EPHE);
  swe_set_topo(ZUR[0], ZUR[1], ZUR[2]);
  (void)topo_moon();
  swe_close();

  pthread_create(&t, NULL, after_close, NULL);
  pthread_join(t, NULL);

  printf("  thread started after swe_close(): topo lon = %.9f\n",
         post_close_lon);
  if (post_close_geoset) {
    printf("  FAIL: inherited configuration from a closed session\n");
    return 1;
  }
  printf("  PASS: master cleared by swe_close()\n");
  return 0;
}

/* ---- 3. sync coverage --------------------------------------------------
 *
 * The mirror of test 1. Sync happens in swi_init_swed_if_start(); a public
 * entry point that never reaches it would never adopt configuration at
 * all. Each probe below depends on a DIFFERENT config field.
 *
 * Note on swe_azalt and the lapse rate: the obvious probe is blind. At
 * ordinary altitudes swe_azalt's apparent-altitude output does not move at
 * all between lapse rates 0.0065 and 0.0300 -- verified -- so a test built
 * on it would pass regardless. The lapse rate is observable through
 * swe_rise_trans_true_hor() with horhgt == -100, which routes through
 * calc_dip(); there it shifts sunrise by 8.5e-04 days (~74 s).
 */
static volatile double probe_main[8], probe_thr[8];

static void probe(volatile double *o) {
  char serr[AS_MAXCH] = ""; double x[6], tret[10];
  double hi[3] = { 8.55, 47.37, 2000 };
  double cusp[13], ascmc[10];
  swe_calc_ut(TJD, SE_MOON, SEFLG_SWIEPH, x, serr);
  o[0] = x[0];                                  /* ephepath  */
  o[1] = swe_deltat(TJD);                       /* tid_acc   */
  o[2] = swe_get_ayanamsa_ut(TJD);              /* sidd      */
  o[3] = swe_deltat_ex(TJD, SEFLG_SWIEPH, serr);/* tid_acc   */
  swe_houses(TJD, 47.37, 8.55, 'P', cusp, ascmc);
  o[4] = ascmc[0];
  o[5] = swe_sidtime(TJD);
  swe_rise_trans_true_hor(TJD, SE_SUN, NULL, SEFLG_SWIEPH, SE_CALC_RISE,
                          hi, 1013.25, 15.0, -100, tret, serr);
  o[6] = tret[0];                               /* const_lapse_rate */
}

static void *probe_worker(void *a) { (void)a; probe(probe_thr); return NULL; }

static int test_sync_coverage(void) {
  static const char *nm[7] = {
    "swe_calc_ut (ephepath)", "swe_deltat (tid_acc)",
    "swe_get_ayanamsa_ut (sidd)", "swe_deltat_ex (tid_acc)",
    "swe_houses", "swe_sidtime", "swe_rise_trans_true_hor (lapse)" };
  pthread_t t; int i, bad = 0;
  swe_set_ephe_path((char *)EPHE);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_set_tid_acc(-25.85);
  swe_set_lapse_rate(0.0300);       /* not the 0.0065 default */
  probe(probe_main);
  pthread_create(&t, NULL, probe_worker, NULL);
  pthread_join(t, NULL);
  for (i = 0; i < 7; i++) {
    int ok = (probe_main[i] == probe_thr[i]);
    printf("  %-34s %s\n", nm[i], ok ? "synced" : "<-- NOT SYNCED");
    if (!ok) bad = 1;
  }
  if (!bad) printf("  PASS: every probed entry point syncs\n");
  return bad;
}

int main(void) {
  const char *e = getenv("SE_TEST_EPHE");
  int bad = 0;
  if (e && *e) EPHE = e;

  printf("1. internal swe_set_* must not leak between threads\n");
  bad |= test_no_leak();
  swe_close();

  printf("2. swe_close() must clear the shared master\n");
  bad |= test_close_resets();
  swe_close();

  printf("3. every config-dependent entry point must sync\n");
  bad |= test_sync_coverage();

  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad;
}
