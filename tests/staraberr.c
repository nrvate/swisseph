/* staraberr.c -- G75: a fixed star's aberration rate takes the Earth's
 * acceleration at the instant.
 *
 * The star's aberration rate is a central difference of the aberration's
 * shift between t - dm and t + dp (UPSTREAM-BUGS 49). The Earth's velocity
 * at t + dp was carried on linearly from t - dm and t, so the difference
 * took the acceleration at t - dm/2, short by dm/2 (5e-6 day) of its rate
 * of change: 8.6e-8 of the aberration's rate, 8e-12 deg/day on the sky,
 * 1/cos(declination) of that in a right ascension near the pole, the same
 * at every step. On Moshier the Earth's velocity is rough from instant to
 * instant (2e-14 AU/day, the Earth-Moon offset's rate), and a difference
 * 1e-5 day long made that up to 2e-6 of the acceleration, 1.56e-8 deg/day
 * in Polaris's right ascension (UPSTREAM-BUGS 76).
 *
 * The check: Polaris and Polaris Australis (near the equator's poles) in
 * right ascension, delta Dor and 42 Dra (near the ecliptic's) in ecliptic
 * longitude, apparent geocentric, swe_fixstar2() and swe_fixstar(), Swiss
 * files and Moshier, at seven dates 1900-2100, the speed against the
 * five-point difference of the same call's positions over 2^-5 day at
 * instants that are multiples of 2^-8 day: 16 checks, each within 6e-11
 * deg/day. Before the fix: Swiss Polaris 1.03e-9, Polaris Australis
 * 7.4e-10, delta Dor 2.2e-10, 42 Dra 1.9e-10; Moshier 5.1e-8, 1.9e-8,
 * 2.0e-8, 1.1e-8 (1.57e-8, 4.1e-9, 6.7e-9, 2.4e-9 with the fix's first
 * version, which read the Earth at t + dp). Now Swiss <= 1.21e-11, Moshier <= 1.82e-11: the
 * five-point reference's own rounding (it scales as 1/step, and is the
 * same with SEFLG_TRUEPOS, which has no aberration). The upstream build
 * fails all 16 (2.6e-6 to 5.7e-3 deg/day, bugs 49 and 58 besides). Run as
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
  int m, p, o, d, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: staraberr EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G75: a fixed star's aberration rate takes the Earth's acceleration at t\n");
  for (m = 0; m < 2; m++)
  for (p = 0; p < 4; p++) {
    for (o = 0; o < 2; o++) {
      int32 fl = (m ? SEFLG_MOSEPH : SEFLG_SWIEPH) | frame[p];
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
      printf("  %-7s %-8s %-18s %-9s %.2e deg/day%s\n", m ? "Moshier" : "Swiss", o ? "fixstar" : "fixstar2", star[p],
             frame[p] ? "RA" : "longitude", worst, worst > lim ? "  FAIL" : "");
    }
  }
  if (bad)
    printf("G75 FAIL: %d\n", bad);
  else
    printf("G75 PASS\n");
  return bad ? 1 : 0;
}
