/* nutgrid.c -- G29: the default nutation is full IAU 2000A, and its grid
 * reproduces the direct series.
 *
 * Swiss's default was IAU 2000B, the truncated series: about 1 mas from full
 * IAU 2000A (0.5 mas at 1800 and J2000, 1.0 mas at 2100, measured by the
 * Ephemeris Prometheia project from output), where the ephemeris protocol's
 * 3.5a names 2000A. The full series costs about 14 times as much per
 * instant, so the default evaluates it only at nodes, cached. ts.17's
 * nodes were a quarter day apart with a quintic Lagrange polynomial through
 * six of them: worst 0.018 microarcseconds in longitude and 0.008 in
 * obliquity over 20000 random instants in each of 1800-2100 and
 * -13000..15000. ts.20's are a day apart, carrying the series' first three
 * derivatives, with the septic Hermite polynomial on the two around the
 * instant (two nodes, not six, for an instant that shares none with the
 * last; G32 holds the cost): worst 0.0014 and 0.0006 on the same instants.
 *
 * The check: the default context against one set to SEMOD_NUT_IAU_2000A
 * explicitly, which computes the series directly, within 0.05 uas. The
 * compat build keeps 2000B as its default and fails: 2.15 mas worst over
 * 1800-2100, 8.1 mas over the long span.
 *
 * And far from J2000 (UPSTREAM-BUGS 152): the arguments' T^3 and T^4
 * terms turn them ever faster (at |J| = 1e9 days the Moon's anomaly 2.4
 * rad/day, ten times its rate now), and the polynomial on day-apart nodes
 * left the series between them long before the |J| < 1e12 it was taken
 * to: 2.3e-9 deg at |J| 6e8, 1.3e-4 deg at 1e9, 7 deg at 3e9, 6.5e28 deg
 * near 1e12. The grid holds within SWI_NUT_GRID_JMAX, 4e8 days (1.1
 * million years), and the direct series is taken beyond. The check: 4000
 * instants with |J| log-uniform over 1e7..1e13, either sign, within 1e-9
 * deg (3.6 uas), the residual floor. Measured 0.57 uas (1.6e-10 deg; the
 * series' own rounding there, as far from a long double evaluation of
 * it), and 0 beyond 4e8; before the fix 2.4e35 uas, near 1e12. The compat
 * build, whose default is 2000B, fails it too (1.25e9 uas). Past |J| 1e12
 * SE_ECL_NUT refuses (UPSTREAM-BUGS 196): the 729 instants there are
 * checked refused, by both contexts, the rest compared as before. Past
 * -3100015.5 .. 8000016.5, the nutation's range (UPSTREAM-BUGS 284), it
 * refuses the rest too: in the shipped build they are checked refused by
 * both and the grid and the series compared by swi_nutation() (before
 * bug 284, 0 of the 3271 refused). Moshier, no files. Run as
 *   nutgrid
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "swephexp.h"
#ifndef SWE_UPSTREAM_COMPAT
#include "sweph.h"
#include "swephlib.h"
#endif

int main(void)
{
  static const double rgSpan[2][2] = {{2378496.5, 2488069.5},
                                      {-3027205.0, 7199795.0}};
  static const char *rgszSpan[2] = {"1800-2100", "-13000..15000"};
  char serr[256];
  double g[6], d[6];
  int s, i, bad = 0;
  swe_ctx *cg = swe_ctx_new(), *cd = swe_ctx_new();

  swe_set_astro_models_r(cd, "0,0,0,3", 0);   /* 2000A, direct */
  srand(20260929);
  printf("G29: the default nutation is IAU 2000A, gridded to the series\n");
  for (s = 0; s < 2; s++) {
    double wl = 0, wo = 0;
    for (i = 0; i < 4000; i++) {
      double t = rgSpan[s][0] + (rgSpan[s][1] - rgSpan[s][0]) *
                 (rand() / (double) RAND_MAX);
      if (swe_calc_r(cg, t, SE_ECL_NUT, SEFLG_MOSEPH, g, serr) < 0 ||
          swe_calc_r(cd, t, SE_ECL_NUT, SEFLG_MOSEPH, d, serr) < 0) {
        printf("  ERROR %s\n", serr);
        return 1;
      }
      if (fabs(g[2] - d[2]) * 3.6e9 > wl) wl = fabs(g[2] - d[2]) * 3.6e9;
      if (fabs(g[3] - d[3]) * 3.6e9 > wo) wo = fabs(g[3] - d[3]) * 3.6e9;
    }
    printf("  %-14s worst dpsi %.4f uas, deps %.4f uas\n", rgszSpan[s], wl, wo);
    if (wl > 0.05 || wo > 0.05) bad++;
  }
  {
    /* far from J2000: within the 1e-9 deg floor, |J| out past the guard;
     * past 1e12, refused (UPSTREAM-BUGS 196) */
    double wl = 0, wo = 0;
    int nfar = 0, nref = 0, nnut = 0, nnutref = 0;
    for (i = 0; i < 4000; i++) {
      double t = (i & 1 ? -1 : 1) * pow(10.0, 7.0 + 6.0 * (rand() / (double) RAND_MAX));
      if (fabs(t) > 1e12) {
        nfar++;
        if (swe_calc_r(cg, t, SE_ECL_NUT, SEFLG_MOSEPH, g, serr) < 0 && g[2] == 0 &&
            swe_calc_r(cd, t, SE_ECL_NUT, SEFLG_MOSEPH, d, serr) < 0 && d[2] == 0)
          nref++;
        continue;
      }
#ifndef SWE_UPSTREAM_COMPAT
      {
        /* UPSTREAM-BUGS 284: past the nutation's range, -3100015.5 ..
         * 8000016.5, which every instant here is, SE_ECL_NUT refuses: the
         * grid and the series by swi_nutation() */
        double ng[2], nd[2];
        nnut++;
        if (swe_calc_r(cg, t, SE_ECL_NUT, SEFLG_MOSEPH, g, serr) < 0 && g[2] == 0 &&
            swe_calc_r(cd, t, SE_ECL_NUT, SEFLG_MOSEPH, d, serr) < 0 && d[2] == 0)
          nnutref++;
        swi_nutation(cg, t, SEFLG_MOSEPH, ng);
        swi_nutation(cd, t, SEFLG_MOSEPH, nd);
        g[2] = ng[0] * RADTODEG;
        g[3] = ng[1] * RADTODEG;
        d[2] = nd[0] * RADTODEG;
        d[3] = nd[1] * RADTODEG;
      }
#else
      if (swe_calc_r(cg, t, SE_ECL_NUT, SEFLG_MOSEPH, g, serr) < 0 ||
          swe_calc_r(cd, t, SE_ECL_NUT, SEFLG_MOSEPH, d, serr) < 0) {
        printf("  ERROR %s\n", serr);
        return 1;
      }
#endif
      if (fabs(g[2] - d[2]) * 3.6e9 > wl) wl = fabs(g[2] - d[2]) * 3.6e9;
      if (fabs(g[3] - d[3]) * 3.6e9 > wo) wo = fabs(g[3] - d[3]) * 3.6e9;
    }
    printf("  %-14s worst dpsi %.4g uas, deps %.4g uas (limit 3.6)\n", "|J| 1e7..1e12", wl, wo);
    printf("  %-14s %d of %d refused\n", "|J| 1e12..1e13", nref, nfar);
#ifndef SWE_UPSTREAM_COMPAT
    printf("  %-14s %d of %d refused by SE_ECL_NUT, the nutation's range\n", "|J| 1e7..1e12",
           nnutref, nnut);
#endif
    if (!(wl <= 3.6 && wo <= 3.6) || nref != nfar || nnutref != nnut) bad++;
  }
  swe_ctx_free(cg);
  swe_ctx_free(cd);
  printf(bad ? "G29 FAIL: %d\n" : "G29 PASS\n", bad);
  return bad ? 1 : 0;
}
