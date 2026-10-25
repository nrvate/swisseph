/* nutderiv.c -- G140: the nutation's rate is its series' own derivative,
 * near J2000 and far from it.
 *
 * The defect (UPSTREAM-BUGS 163): swi_nutation_rate() took every model but
 * the default's grid as a central difference of the series over -+1e-4
 * day (NUT_SPEED_INTV), which divides the series' rounding by 2e-4 day.
 * Near J2000 that is the step's own rounding at the JD's resolution, 1e-10
 * deg/day; far from it the arguments' (their polynomials are some 1.7e9" T,
 * their last bit 2.6e-4" at |J - J2000| = 2.5e7 days): IAU 2000A set
 * explicitly 1.2e-9 deg/day off at 1e6 to 5.5e6 days, 7e-9 at 5.5e6 to
 * 2.5e7, 3e-8 at 2.5e7 to 1e8, 5e-7 at 1e8 to 4e8, against a long double
 * evaluation of the series; IAU 1980, 1987 and 2000B alike, Woolard's a
 * tenth of it. Now each series' derivative, through its arguments'
 * polynomials and its amplitudes' T terms.
 *
 * Check (a): the nutation's part of a speed of date, the speed minus the
 * speed with SEFLG_NONUT, against the five-point difference of the same two
 * calls' position differences over 2^-6 day (G57's measure: what the
 * nutation alone does), longitude and latitude, ecliptic and equatorial;
 * the Sun and Mars on the Swiss files and the mean node on Moshier, under
 * IAU 1980, 1987, 2000A, 2000B and Woolard set explicitly, at 40 instants
 * with |J - J2000| log-uniform over 1e3..5.4e6 days (the files' span),
 * either sign, multiples of 2^-8 day: 30 checks within 3e-11 deg/day.
 *
 * Check (b): swi_nutation_rate() itself, dpsi and deps, against five-point
 * differences of swi_nutation() over 2^-4 day, under the same five models
 * and SEFLG_JPLHOR_APPROX with the default models (IAU 1980 and the fixed
 * offset before 1962), at 64 instants with |J - J2000| log-uniform over
 * 1e3..1e8 days within 2e-10 deg/day, and 64 over 1e8..4e8 (to
 * SWI_NUT_GRID_JMAX) within 5e-9, where the reference's own rounding is
 * some 1e-9: 12 checks. It calls the fork's own functions and is left out
 * of the upstream build.
 *
 * Measured after the fix: (a) within 5.7e-12, the position differences'
 * rounding over the step; (b) within 2.6e-11 to 1e8 and 1.1e-9 beyond (the
 * reference's, as a long double evaluation of the series has the rate
 * within 4.3e-12 there). Before it, (a) fails 30 of 30, 2.0e-10 to 2.6e-10
 * deg/day under IAU 1980 to 2000B, Woolard's 7.0e-11 to 8.2e-11; (b) fails
 * 12 of 12: to 1e8, 5.4e-9 to 1.4e-8 (Woolard 1.7e-9), beyond, 1.6e-7 to
 * 5.2e-7 (Woolard 3.2e-8). The upstream build fails 30 of (a)'s 30: its
 * rate a backward difference over 1e-4 day (UPSTREAM-BUGS 58), 8.5e-10 to
 * 1.0e-9 deg/day in the equatorial frame, and in the ecliptic of date the
 * Sun's and Mars's latitude speeds 1.6e-5 to 2.1e-5 off by the ecliptic's
 * turning they leave out (bug 33), Woolard's nutation 6e-2 by bug 151's
 * anomaly. Run as
 *   nutderiv EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"
#ifndef SWE_UPSTREAM_COMPAT
#include "sweph.h"
#include "swephlib.h"
#endif

static const char *MOD[5] = { "0,0,0,1", "0,0,0,2", "0,0,0,3", "0,0,0,4", "0,0,0,5" };
static const char *MN[5] = { "IAU 1980", "IAU 1987", "IAU 2000A", "IAU 2000B", "Woolard" };

/* the instants: n with |J - J2000| log-uniform over [a, b], either sign,
 * multiples of 2^-8 day */
static void instants(double *tq, int n, double a, double b, unsigned seed)
{
  int i;
  srand(seed);
  for (i = 0; i < n; i++) {
    double u = rand() / (double) RAND_MAX, dj = a * pow(b / a, u);
    tq[i] = floor((2451545.0 + ((i & 1) ? dj : -dj)) * 256) / 256;
  }
}

/* the nutation's part of the place and speed of date: with it, less
 * without it */
static int nut_part(swe_ctx *c, double t, int ipl, int32 fl, double *x, char *serr)
{
  double a[6], b[6];
  int j;
  if (swe_calc_r(c, t, ipl, fl, a, serr) < 0 || swe_calc_r(c, t, ipl, fl | SEFLG_NONUT, b, serr) < 0)
    return -1;
  for (j = 0; j < 6; j++)
    x[j] = a[j] - b[j];
  x[0] = swe_difdeg2n(a[0], b[0]);
  return 0;
}

int main(int argc, char **argv)
{
  static const int pl[3] = { SE_SUN, SE_MARS, SE_MEAN_NODE };
  static const int32 ep[3] = { SEFLG_SWIEPH, SEFLG_SWIEPH, SEFLG_MOSEPH };
  static const char *pn[3] = { "Sun", "Mars", "mean node" };
  double tq[64], h = 1.0 / 64;
  char serr[256];
  int m, p, f, i, k, j, n = 0, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: nutderiv EPHEDIR\n");
    return 2;
  }
  printf("G140: the nutation's rate is its series' own derivative\n");
  printf("  (a) the nutation's part of the speed of date, |J - J2000| 1e3..5.4e6 days:"
         " worst |speed - reference|, deg/day, ecliptic / equatorial\n");
  instants(tq, 40, 1e3, 5.4e6, 20261025);
  for (m = 0; m < 5; m++) {
    char mod[40];
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, argv[1]);
    strcpy(mod, MOD[m]);
    swe_set_astro_models_r(c, mod, 0);
    for (p = 0; p < 3; p++) {
      double w[2] = { 0, 0 };
      for (f = 0; f < 2; f++) {
        int32 fl = ep[p] | SEFLG_SPEED | (f ? SEFLG_EQUATORIAL : 0);
        for (i = 0; i < 40; i++) {
          double x[6], v[5][6];
          if (nut_part(c, tq[i], pl[p], fl, x, serr) < 0) {
            printf("  ERROR %s at %.8f: %s\nG140 FAIL: ERROR\n", pn[p], tq[i], serr);
            return 1;
          }
          for (k = 0; k < 5; k++)
            if (nut_part(c, tq[i] + (k - 2) * h, pl[p], fl, v[k], serr) < 0) {
              printf("  ERROR %s at %.8f: %s\nG140 FAIL: ERROR\n", pn[p], tq[i], serr);
              return 1;
            }
          for (j = 0; j < 2; j++) {
            double d = (8 * (v[3][j] - v[1][j]) - (v[4][j] - v[0][j])) / (12 * h);
            if (fabs(x[3 + j] - d) > w[f])
              w[f] = fabs(x[3 + j] - d);
          }
        }
      }
      printf("    %-10s %-10s %.2e  %.2e\n", MN[m], pn[p], w[0], w[1]);
      for (f = 0; f < 2; f++) {
        n++;
        if (!(w[f] <= 3e-11)) {
          bad++;
          printf("    FAIL %s, %s, %s: %.2e > 3e-11\n", MN[m], pn[p],
                 f ? "equatorial" : "ecliptic", w[f]);
        }
      }
    }
    swe_ctx_free(c);
  }
#ifndef SWE_UPSTREAM_COMPAT
  {
    /* check (b): the rate itself */
    static const double span[2][2] = { { 1e3, 1e8 }, { 1e8, 4e8 } };
    static const double lim[2] = { 2e-10, 5e-9 };
    printf("  (b) swi_nutation_rate(): worst |rate - reference|, deg/day,"
           " |J - J2000| 1e3..1e8 / 1e8..4e8\n");
    h = 1.0 / 16;
    for (m = 0; m < 6; m++) {
      char mod[40];
      int32 fl = (m == 5) ? SEFLG_JPLHOR_APPROX : 0;
      double w[2] = { 0, 0 };
      int s;
      swe_ctx *c = swe_ctx_new();
      strcpy(mod, m == 5 ? "0,0,0,0" : MOD[m]);
      swe_set_astro_models_r(c, mod, 0);
      for (s = 0; s < 2; s++) {
        instants(tq, 64, span[s][0], span[s][1], 20261026 + s);
        for (i = 0; i < 64; i++) {
          double r[2], v[5][2];
          swi_nutation_rate(c, tq[i], fl, r);
          for (k = 0; k < 5; k++)
            swi_nutation(c, tq[i] + (k - 2) * h, fl, v[k]);
          for (j = 0; j < 2; j++) {
            double d = (8 * (v[3][j] - v[1][j]) - (v[4][j] - v[0][j])) / (12 * h);
            if (fabs(r[j] - d) * RADTODEG > w[s])
              w[s] = fabs(r[j] - d) * RADTODEG;
          }
        }
      }
      printf("    %-26s %.2e  %.2e\n", m == 5 ? "SEFLG_JPLHOR_APPROX" : MN[m], w[0], w[1]);
      for (s = 0; s < 2; s++) {
        n++;
        if (!(w[s] <= lim[s])) {
          bad++;
          printf("    FAIL %s, |J - J2000| %.0e..%.0e: %.2e > %.0e\n",
                 m == 5 ? "SEFLG_JPLHOR_APPROX" : MN[m], span[s][0], span[s][1], w[s], lim[s]);
        }
      }
      swe_ctx_free(c);
    }
  }
#endif
  if (bad) {
    printf("G140 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG140 PASS\n", n);
  return 0;
}
