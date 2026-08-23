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

static pthread_barrier_t bar;
static volatile double obs_before, obs_after;

/* The victim: a thread that INHERITS the observer position from main and
 * never sets one itself. Main is not a valid victim -- having called
 * swe_set_topo() it owns the group locally and sync skips it, so main is
 * immune whether or not the leak exists. That made the first version of
 * this test pass against a deliberately broken build. */
static void *observer(void *a) {
  (void)a;
  pthread_barrier_wait(&bar);      /* main configured */
  obs_before = topo_moon();        /* inherit Zurich */
  pthread_barrier_wait(&bar);      /* let the noisy thread run */
  pthread_barrier_wait(&bar);      /* noisy thread done */
  obs_after = topo_moon();
  pthread_barrier_wait(&bar);
  return NULL;
}

/* Runs calculations that internally call swe_set_topo/swe_set_sid_mode. */
static void *noisy_worker(void *a) {
  char serr[AS_MAXCH]; double dret[50], attr[20], tret[10];
  double datm[4] = {1013.25, 15, 40, 0}, dobs[6] = {36,1,1,1,1,1};
  char obj[AS_MAXCH];
  double cusp[37], ascmc[10];
  (void)a;

  pthread_barrier_wait(&bar);          /* main configured */
  pthread_barrier_wait(&bar);          /* observer took its reading */

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

  pthread_barrier_wait(&bar);          /* observer takes second reading */
  pthread_barrier_wait(&bar);
  return NULL;
}

static int test_no_leak(void) {
  pthread_t noisy, obs;
  int bad = 0;
  swe_set_ephe_path((char *)EPHE);
  swe_set_topo(ZUR[0], ZUR[1], ZUR[2]);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);

  pthread_barrier_init(&bar, NULL, 3);
  pthread_create(&obs,   NULL, observer,     NULL);
  pthread_create(&noisy, NULL, noisy_worker, NULL);
  pthread_barrier_wait(&bar);   /* configured */
  pthread_barrier_wait(&bar);   /* observer read #1 */
  pthread_barrier_wait(&bar);   /* noisy done */
  pthread_barrier_wait(&bar);   /* observer read #2 */
  pthread_join(noisy, NULL);
  pthread_join(obs, NULL);
  pthread_barrier_destroy(&bar);

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

int main(void) {
  const char *e = getenv("SE_TEST_EPHE");
  int bad = 0;
  if (e && *e) EPHE = e;

  printf("1. internal swe_set_* must not leak between threads\n");
  bad |= test_no_leak();
  swe_close();

  printf("2. swe_close() must clear the shared master\n");
  bad |= test_close_resets();

  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad;
}
