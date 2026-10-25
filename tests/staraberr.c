/* staraberr.c -- G75: a fixed star's aberration rate takes the Earth's
 * acceleration at the instant.
 *
 * The star's aberration rate is a central difference of the aberration's
 * shift between t - dm and t + dp (UPSTREAM-BUGS 49). The Earth's velocity
 * at t + dp was carried on linearly from t - dm and t, so the difference
 * took the acceleration at t - dm/2, short by dm/2 (5e-6 day) of its rate
 * of change: 8.6e-8 of the aberration's rate, 8e-12 deg/day on the sky,
 * 1/cos(declination) of that in a right ascension near the pole, the same
 * at every step (UPSTREAM-BUGS 76).
 *
 * The check: Polaris and Polaris Australis (near the equator's poles) in
 * right ascension, delta Dor and 42 Dra (near the ecliptic's) in ecliptic
 * longitude, apparent geocentric, swe_fixstar2() and swe_fixstar(), at
 * seven dates 1900-2100, the speed against the five-point difference of the
 * same call's positions over 2^-5 day at instants that are multiples of
 * 2^-8 day: 8 checks, each within 6e-11 deg/day. Before the fix: Polaris
 * 1.03e-9, Polaris Australis 7.4e-10, delta Dor 2.2e-10, 42 Dra 1.9e-10,
 * either function. Now Polaris 1.8e-11, the others <= 9.7e-12: the
 * five-point reference's own rounding (it scales as 1/step, and is the same
 * with SEFLG_TRUEPOS, which has no aberration). The upstream build fails
 * all 8 (2.6e-6 to 4.3e-4 deg/day, bugs 49 and 58 besides). Run as
 *   staraberr EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static int at(int old, const char *name, double t, int32 fl, double *x, char *serr)
{
  char s[SE_MAX_STNAME * 2 + 1];
  strcpy(s, name);
  return old ? swe_fixstar(s, t, fl, x, serr) : swe_fixstar2(s, t, fl, x, serr);
}

static double wrap(double d) { return d - 360 * floor(d / 360 + 0.5); }

int main(int argc, char **argv)
{
  static const char *star[4] = {"Polaris", "Polaris Australis", ",deDor", "Fafnir"};
  static const int32 frame[4] = {SEFLG_EQUATORIAL, SEFLG_EQUATORIAL, 0, 0};
  static const double date[7] = {2415020.3, 2440000.7, 2451545.0, 2457000.1,
                                 2460000.3, 2470000.7, 2488070.1};
  const double h = 1.0 / 32, lim = 6e-11;
  char serr[256];
  int p, o, d, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: staraberr EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G75: a fixed star's aberration rate takes the Earth's acceleration at t\n");
  for (p = 0; p < 4; p++) {
    for (o = 0; o < 2; o++) {
      int32 fl = SEFLG_SWIEPH | frame[p];
      double worst = 0;
      for (d = 0; d < 7; d++) {
        double t = floor(date[d] * 256) / 256, x[6], q[5][6], r, e;
        if (at(o, star[p], t, fl | SEFLG_SPEED, x, serr) < 0) {
          printf("  ERROR %s\n", serr);
          return 1;
        }
        for (i = 0; i < 5; i++)
          if (at(o, star[p], t + (i - 2) * h, fl, q[i], serr) < 0) {
            printf("  ERROR %s\n", serr);
            return 1;
          }
        r = (8 * wrap(q[3][0] - q[1][0]) - wrap(q[4][0] - q[0][0])) / (12 * h);
        e = fabs(x[3] - r);
        if (e > worst)
          worst = e;
      }
      if (worst > lim)
        bad++;
      printf("  %-8s %-18s %-9s %.2e deg/day%s\n", o ? "fixstar" : "fixstar2", star[p],
             frame[p] ? "RA" : "longitude", worst, worst > lim ? "  FAIL" : "");
    }
  }
  if (bad)
    printf("G75 FAIL: %d\n", bad);
  else
    printf("G75 PASS\n");
  return bad ? 1 : 0;
}
