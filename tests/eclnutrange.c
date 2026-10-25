/* eclnutrange.c -- G152: swe_calc(SE_ECL_NUT) answers |J| <= 1e12 days and
 * refuses the dates past it and the ones that are no dates.
 *
 * The defect (UPSTREAM-BUGS 196): SE_ECL_NUT took any date and answered
 * it with its flags, as no other body does (the widest range of any is
 * the mean node's, -3100015.5 .. 8000016.5). On Moshier under the default
 * models the true and mean obliquity were NaN from |J| 1.3e60 days, dpsi
 * 1.2e72 deg at 1e82 and NaN with deps from 4e82; +-inf and NaN gave NaN;
 * and -1e300, the empty caches' SWI_TJD_NEVER, the zeros of caches never
 * filled; under every nutation model, with flags 4 (Moshier) or 2 (Swiss).
 * Past 1e12 the series as the library evaluates them leave the residual
 * floor (IAU 2000B 6.8e-11 deg off the series at 3e11..1e12, 1.5e-7 at
 * 1e12..3e12, 2.2 deg at 3e13..1e14, noise beyond; against the series in
 * mpmath at the same double J). Now refused there, before any cache is
 * read.
 *
 * Check (a): refused. SE_ECL_NUT at 23 dates -- |J| the next double past
 * 1e12, 1.5e12, 1e13, 1e14, 1e20, 1e60, 1e82, 1e84, 1e300, DBL_MAX and
 * inf, either sign, and NaN -- under the default, IAU 1980, 1987, 2000A,
 * 2000B and Woolard, with SEFLG_MOSEPH; SEFLG_SWIEPH | SEFLG_SPEED;
 * SEFLG_MOSEPH | SEFLG_SPEED3 | SEFLG_RADIANS | SEFLG_EQUATORIAL;
 * SEFLG_MOSEPH | SEFLG_JPLHOR_APPROX; SEFLG_MOSEPH | SEFLG_NONUT; on a
 * fresh context, then again on it: ERR, the six outputs 0 (filled with 7
 * before), serr naming the range. And swe_calc_ut() at UT 1e12 (TT
 * 1.28e12), 1e13, -inf and NaN. 1384 calls.
 *
 * Check (b): answered. +-1e12, the doubles next to them inside and 40
 * instants with |J| log-uniform over 1e7..1e12, either sign, under the
 * six nutation models and the eleven long-term precession models and the
 * default, with SEFLG_MOSEPH, SEFLG_SWIEPH and SEFLG_MOSEPH |
 * SEFLG_JPLHOR_APPROX: OK and the four finite. 9504 calls.
 *
 * Check (c): a refusal leaves nothing behind. SE_ECL_NUT at J2000 and at
 * 1e12 after refusals at 1e13, -1e300 and NaN on the same context, against
 * a fresh context, the default models: bit for bit.
 *
 * Measured: (a) 1384 of 1384 refused, (b) 9504 of 9504 finite (the largest
 * |dpsi| 125 deg; the mean obliquity, a polynomial in T under every model
 * but Vondrak 2011, up to 1.6e51 deg), (c) 0 values differ. Before the
 * fix (a) fails, 0 of 1384 refused, every date answered with its flags,
 * and (b) and (c) pass; the upstream build fails the same. Run as
 *   eclnutrange EPHEDIR
 */
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

static const char *MOD[6] = { "0,0,0,0", "0,0,0,1", "0,0,0,2", "0,0,0,3", "0,0,0,4", "0,0,0,5" };

/* refused as swe_calc() refuses: ERR, the outputs 0, the range named */
static int refused(int32 r, const double *x, const char *serr)
{
  int k;
  if (r != ERR || strstr(serr, "outside the obliquity and nutation range") == NULL)
    return 0;
  for (k = 0; k < 6; k++)
    if (x[k] != 0)
      return 0;
  return 1;
}

int main(int argc, char **argv)
{
  static const double far[11] = { 0, 1.5e12, 1e13, 1e14, 1e20, 1e60, 1e82, 1e84, 1e300, DBL_MAX, INFINITY };
  static const int32 fla[5] = {
    SEFLG_MOSEPH, SEFLG_SWIEPH | SEFLG_SPEED,
    SEFLG_MOSEPH | SEFLG_SPEED3 | SEFLG_RADIANS | SEFLG_EQUATORIAL,
    SEFLG_MOSEPH | SEFLG_JPLHOR_APPROX, SEFLG_MOSEPH | SEFLG_NONUT };
  static const int32 flb[3] = { SEFLG_MOSEPH, SEFLG_SWIEPH, SEFLG_MOSEPH | SEFLG_JPLHOR_APPROX };
  double tq[23], tin[46], big = 0, bigeps = 0;
  int i, m, f, p, k, na = 0, ba = 0, nb = 0, bb = 0, bad = 0;
  long long cbad = 0;
  char serr[AS_MAXCH], mod[40];

  if (argc != 2) {
    fprintf(stderr, "usage: eclnutrange EPHEDIR\n");
    return 2;
  }
  printf("G152: SE_ECL_NUT answers |J| <= 1e12 days and refuses beyond\n");
  for (i = 0; i < 11; i++) {
    double t = i == 0 ? nextafter(1e12, INFINITY) : far[i];
    tq[2 * i] = t;
    tq[2 * i + 1] = -t;
  }
  tq[22] = NAN;

  /* (a) */
  for (m = 0; m < 6; m++)
    for (f = 0; f < 5; f++) {
      swe_ctx *c = swe_ctx_new();
      swe_set_ephe_path_r(c, argv[1]);
      strcpy(mod, MOD[m]);
      swe_set_astro_models_r(c, mod, 0);
      for (i = 0; i < 23; i++)
        for (k = 0; k < 2; k++) {
          double x[6] = { 7, 7, 7, 7, 7, 7 };
          int32 r;
          serr[0] = '\0';
          r = swe_calc_r(c, tq[i], SE_ECL_NUT, fla[f], x, serr);
          na++;
          if (!refused(r, x, serr)) {
            if (ba++ < 5)
              printf("  (a) answered: models %s flags %d J %g: return %d, %g %g %g %g\n",
                     MOD[m], fla[f], tq[i], r, x[0], x[1], x[2], x[3]);
          }
        }
      swe_ctx_free(c);
    }
  {
    static const double tu[4] = { 1e12, 1e13, -INFINITY, NAN };
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, argv[1]);
    for (i = 0; i < 4; i++) {
      double x[6] = { 7, 7, 7, 7, 7, 7 };
      int32 r;
      serr[0] = '\0';
      r = swe_calc_ut_r(c, tu[i], SE_ECL_NUT, SEFLG_MOSEPH, x, serr);
      na++;
      if (!refused(r, x, serr) && ba++ < 5)
        printf("  (a) answered: swe_calc_ut UT %g: return %d, %g %g %g %g\n",
               tu[i], r, x[0], x[1], x[2], x[3]);
    }
    swe_ctx_free(c);
  }
  printf("  (a) past |J| 1e12, -+inf, NaN: %d of %d refused\n", na - ba, na);
  if (ba)
    bad++;

  /* (b) */
  tin[0] = 1e12;
  tin[1] = -1e12;
  tin[2] = nextafter(1e12, 0);
  tin[3] = -nextafter(1e12, 0);
  srand(20261025);
  for (i = 4; i < 44; i++)
    tin[i] = (i & 1 ? -1 : 1) * pow(10.0, 7.0 + 5.0 * (rand() / (double) RAND_MAX));
  for (f = 0; f < 3; f++)
    for (m = 0; m < 6; m++)
      for (p = 0; p <= 11; p++) {
        swe_ctx *c = swe_ctx_new();
        swe_set_ephe_path_r(c, argv[1]);
        snprintf(mod, sizeof mod, "0,%d,0,%d", p, m);
        swe_set_astro_models_r(c, mod, 0);
        for (i = 0; i < 44; i++) {
          double x[6];
          int32 r = swe_calc_r(c, tin[i], SE_ECL_NUT, flb[f], x, serr);
          nb++;
          if (r < 0 || !isfinite(x[0]) || !isfinite(x[1]) || !isfinite(x[2]) || !isfinite(x[3])) {
            if (bb++ < 5)
              printf("  (b) models %s flags %d J %.17g: return %d, %g %g %g %g %s\n",
                     mod, flb[f], tin[i], r, x[0], x[1], x[2], x[3], r < 0 ? serr : "");
            continue;
          }
          if (fabs(x[2]) > big)
            big = fabs(x[2]);
          if (fabs(x[1]) > bigeps)
            bigeps = fabs(x[1]);
        }
        swe_ctx_free(c);
      }
  printf("  (b) |J| <= 1e12: %d of %d answered, finite (largest |dpsi| %.3g deg, |mean obliquity| %.3g)\n",
         nb - bb, nb, big, bigeps);
  if (bb)
    bad++;

  /* (c) */
  {
    static const double tr[3] = { 1e13, -1e300, NAN }, tg[2] = { 2451545.0, 1e12 };
    swe_ctx *c = swe_ctx_new(), *d;
    double x[6], y[6];
    for (i = 0; i < 2; i++) {
      for (k = 0; k < 3; k++)
        swe_calc_r(c, tr[k], SE_ECL_NUT, SEFLG_MOSEPH, x, serr);
      d = swe_ctx_new();
      if (swe_calc_r(c, tg[i], SE_ECL_NUT, SEFLG_MOSEPH, x, serr) < 0
          || swe_calc_r(d, tg[i], SE_ECL_NUT, SEFLG_MOSEPH, y, serr) < 0)
        cbad++;
      else
        for (k = 0; k < 6; k++)
          if (memcmp(&x[k], &y[k], sizeof(double)) != 0)
            cbad++;
      swe_ctx_free(d);
    }
    swe_ctx_free(c);
    printf("  (c) after refusals, J2000 and 1e12 against a fresh context: %lld values differ\n", cbad);
    if (cbad)
      bad++;
  }

  printf(bad ? "G152 FAIL: %d\n" : "G152 PASS\n", bad);
  return bad ? 1 : 0;
}
