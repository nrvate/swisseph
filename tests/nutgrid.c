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
 * Moshier, no files. Run as
 *   nutgrid
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "swephexp.h"

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
  swe_ctx_free(cg);
  swe_ctx_free(cd);
  printf(bad ? "G29 FAIL: %d\n" : "G29 PASS\n", bad);
  return bad ? 1 : 0;
}
