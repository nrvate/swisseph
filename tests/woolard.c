/* woolard.c -- G138: Woolard's nutation (SEMOD_NUT_WOOLARD) turns the
 * Moon's mean anomaly at its anomalistic month.
 *
 * The defect (UPSTREAM-BUGS 151): calc_nutation_woolard() took the Moon's
 * mean anomaly at 13255523.59 revolutions a Julian century, 10^4 times the
 * Explanatory Supplement (1961)'s 1325r 198 deg 50' 56".79 (1325.552359),
 * so the series' five terms in it -- 0".0675 sin l and four combinations,
 * 0".1199 together in dpsi, 0".0113 in deps -- cycled every 3.97 minutes.
 * dpsi was up to 0.236" and deps 0.0226" off the series over 1900-2100,
 * Woolard against IAU 1980 0.323" where the two series differ by their
 * coefficients by 0.159", and the interpolation on it 0.196" off.
 *
 * The check, Moshier, explicit contexts, 4000 instants 1900-2100 that are
 * multiples of 2^-10 day:
 *  (a) Woolard against IAU 1980 within 0.25" in dpsi and 0.09" in deps.
 *      The two series differ by their coefficients (Woolard's rigid Earth
 *      of 1953, Wahr's elastic one of 1980): over 1900-2100 by 50.5 mas
 *      in the 18.6-year term, 45.7 in the semiannual, 23.7 in the
 *      fortnightly, 16.8 in the annual, and the 93 IAU 1980 terms the 13
 *      here leave out 88.0 mas together; the sum of all of these, 0.2444"
 *      in dpsi and 0.0864" in deps, bounds the difference, and the two
 *      sets of arguments (Brown's and Newcomb's elements, IAU 1980's)
 *      differ by under 1 mas on it. Less the coefficients' difference
 *      itself, on IAU 1980's arguments, Woolard is within 0.9 mas of IAU
 *      1980 in dpsi and 0.5 mas in deps (a probe, not this gate).
 *  (b) smooth over minutes: the second difference of dpsi and deps over
 *      2^-11 day (42 s) within 1e-6". The series' curvature is at most
 *      0.069"/day^2 in dpsi (the fortnightly term's 0.043) and 0.029" in
 *      deps, 1.6e-8" and 6.9e-9" over that step; the 3.97-minute cycle
 *      gave up to 0.13".
 *  (c) the interpolation (swe_set_interpolate_nut_r()) on Woolard: dpsi
 *      and deps at t + 0.37 after a call at t, its nodes t - 1, t, t + 1,
 *      against the direct value within 3.5 mas, the interpolation's own
 *      (G122's limit; IAU 1980's is 2.5 mas here).
 * Six checks, each the worst over the instants. Measured: (a) 0.160" and
 * 0.049"; (b) 1.5e-8" and 6.7e-9"; (c) 1.68 mas and 0.73 mas. The upstream
 * build fails 5 of 6: (a) dpsi 0.306" (deps 0.066" passes), (b) 0.133"
 * and 0.0126", (c) 0.197" and 0.0192". Run as
 *   woolard
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "swephexp.h"

static int nut(swe_ctx *c, double t, double *v)
{
  char serr[256];
  double x[6];
  if (swe_calc_r(c, t, SE_ECL_NUT, SEFLG_MOSEPH, x, serr) < 0) {
    printf("  ERROR %s\n", serr);
    return -1;
  }
  v[0] = x[2] * 3600;
  v[1] = x[3] * 3600;
  return 0;
}

int main(void)
{
  static const char *rgsz[3] = {"against IAU 1980", "2nd difference, 2^-11 d",
                                "interpolated"};
  static const double lim[3][2] = {{0.25, 0.09}, {1e-6, 1e-6}, {3.5e-3, 3.5e-3}};
  const double h = 1.0 / 2048;
  double w[3][2] = {{0, 0}, {0, 0}, {0, 0}};
  swe_ctx *cw = swe_ctx_new(), *ci = swe_ctx_new(), *cwi = swe_ctx_new();
  int i, j, k, bad = 0;

  swe_set_astro_models_r(cw, "0,0,0,5", 0);	/* SEMOD_NUT_WOOLARD */
  swe_set_astro_models_r(cwi, "0,0,0,5", 0);
  swe_set_interpolate_nut_r(cwi, TRUE);
  swe_set_astro_models_r(ci, "0,0,0,1", 0);	/* SEMOD_NUT_IAU_1980 */
  srand(20261025);
  printf("G138: Woolard's nutation turns the Moon's anomaly at its month\n");
  for (i = 0; i < 4000; i++) {
    double t = 2415020.0 + 73049.5 * (rand() / (double) RAND_MAX);
    double a[2], b[2], m[2], p[2], q[2];
    t = floor(t * 1024) / 1024;
    if (nut(cw, t, a) || nut(ci, t, b) || nut(cw, t - h, m) || nut(cw, t + h, p))
      return 1;
    for (k = 0; k < 2; k++) {
      if (fabs(a[k] - b[k]) > w[0][k]) w[0][k] = fabs(a[k] - b[k]);
      if (fabs(m[k] - 2 * a[k] + p[k]) > w[1][k]) w[1][k] = fabs(m[k] - 2 * a[k] + p[k]);
    }
    if (nut(cwi, t, q) || nut(cwi, t + 0.37, q) || nut(cw, t + 0.37, p))
      return 1;
    for (k = 0; k < 2; k++)
      if (fabs(q[k] - p[k]) > w[2][k]) w[2][k] = fabs(q[k] - p[k]);
  }
  for (j = 0; j < 3; j++) {
    int fail = w[j][0] > lim[j][0] || w[j][1] > lim[j][1];
    printf("  %-24s worst dpsi %.3e\", deps %.3e\" (limit %.1e\", %.1e\")%s\n", rgsz[j],
           w[j][0], w[j][1], lim[j][0], lim[j][1], fail ? "  FAIL" : "");
    for (k = 0; k < 2; k++)
      if (w[j][k] > lim[j][k]) bad++;
  }
  swe_ctx_free(cw);
  swe_ctx_free(ci);
  swe_ctx_free(cwi);
  printf(bad ? "G138 FAIL: %d\n" : "G138 PASS\n", bad);
  return bad ? 1 : 0;
}
