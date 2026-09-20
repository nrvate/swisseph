/* dtobs.c -- G26: a delta-t change must force the topocentric observer to
 * be recomputed.
 *
 * ctx->topd holds the observer's geocentric position vector, and
 * swi_get_observer() recomputes it only when
 *
 *     ctx->topd.teval != pedp->teval || ctx->topd.teval == 0
 *
 * -- the INSTANT. The observer is a function of sidereal time, which comes
 * from UT = TT - delta t, so delta t is an input to that cache which its key
 * does not carry. One function along from G18, and the same shape: dtmemo
 * guards a memo whose key omits the delta-t TABLE, this guards a cache whose
 * key omits the delta-t VALUE.
 *
 * swe_set_topo_r() zeroes topd.teval to force the recompute, and it is the
 * only thing that ever did -- and it early-returns when the site is
 * unchanged, so re-setting the same site does not clear it. A site change
 * therefore invalidated and a delta-t change did not, which is why a
 * Greenwich answer and a Sydney answer at one instant were both right and
 * this went unseen.
 *
 * What it looked like from outside: the FIRST delta t used on a context won
 * for every later call at the same instant. Two contexts, the same two
 * questions, different order:
 *
 *   A: delta t 0, then delta t 100, both at T
 *   B: delta t 100 at T
 *
 * A's second answer and B's only answer are the same question put to the
 * same configuration, so they have to agree. Without the invalidation A
 * answers out of the observer it built for delta t 0, and they do not. No
 * golden file: the test is its own reference.
 *
 * Moshier, so it needs no ephemeris files. Run as
 *   dtobs
 *
 * swe_set_tid_acc_r() is the other setter that moves delta t and it is
 * invalidated by the same helper; this gate drives the userdef one because
 * it moves the answer by a whole arcsecond and needs no model argument.
 */
#include <stdio.h>
#include <math.h>
#include "swephexp.h"

#define TJD   2451545.0            /* J2000, TT */
#define LON   8.55                 /* Zurich: a mid-latitude site, so the  */
#define LAT   47.37                /* diurnal term is well away from zero  */
#define ALT   400.0

/* The Moon: nothing else moves enough with the observer to measure. */
static double moon_lon(swe_ctx *c, double dt)
{
  char serr[AS_MAXCH];
  double xx[6];
  swe_set_delta_t_userdef_r(c, dt / 86400.0);
  if (swe_calc_r(c, TJD, SE_MOON,
                 SEFLG_MOSEPH | SEFLG_SPEED | SEFLG_TOPOCTR, xx, serr) < 0) {
    printf("swe_calc_r refused: %s\n", serr);
    return -1.0;
  }
  return xx[0];
}

int main(void)
{
  swe_ctx *a, *b;
  double a0, a100, b100;
  int bad = 0;

  a = swe_ctx_new();
  b = swe_ctx_new();
  swe_set_topo_r(a, LON, LAT, ALT);
  swe_set_topo_r(b, LON, LAT, ALT);

  a0   = moon_lon(a, 0.0);     /* fills the observer cache at delta t 0   */
  a100 = moon_lon(a, 100.0);   /* same instant, different delta t         */
  b100 = moon_lon(b, 100.0);   /* a context that never saw delta t 0      */

  if (a0 < 0.0 || a100 < 0.0 || b100 < 0.0) { printf("FAIL\n"); return 1; }

  /* Vacuous unless delta t actually moves this answer: if a0 already equals
   * b100 there was nothing for the invalidation to invalidate, and a stale
   * observer would be indistinguishable from a correct one. */
  if (a0 == b100) {
    printf("delta t 0 vs 100  : %.9f unchanged -- delta t moves nothing at\n"
           "                    this site and instant, so this run proves\n"
           "                    nothing\n", a0);
    bad = 1;
  } else {
    printf("delta t 0         : %.9f   (observer cached against it)\n", a0);
    printf("delta t 100, fresh: %.9f   (%.4f arcsec away)\n",
           b100, fabs(b100 - a0) * 3600.0);
  }

  printf("delta t 100, reused ctx: %.9f %s\n",
         a100, a100 == b100 ? "OK" : "STALE");
  if (a100 != b100) {
    printf("                    the observer built for delta t 0 was reused;\n"
           "                    off by %.4f arcsec\n",
           fabs(a100 - b100) * 3600.0);
    bad = 1;
  }

  swe_ctx_free(a);
  swe_ctx_free(b);
  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad;
}
