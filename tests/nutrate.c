/* nutrate.c -- G57: the nutation's rate in a speed of date is the rate at
 * the instant, not a backward difference's at t - h/2.
 *
 * Every speed of date took the rate of the nutation angles from nutation at
 * t and t - h, h = NUT_SPEED_INTV = 1e-4 day: the rate at t - h/2, short of
 * the fortnightly term's curvature by (h/2) * nutation'' (UPSTREAM-BUGS 58).
 * It is 6.5e-10 deg/day in every longitude speed of date, 2.7e-8 deg/day in
 * the right ascension of Polaris (3.5e-10 as an angle on the sky).
 *
 * A planet's speed carries other errors of 5e-9 to 3e-8 deg/day (the Swiss
 * files' own, the light-time) that hide a 6.5e-10 bug, so the planets are
 * judged on what the nutation alone does to them: the speed of date minus
 * the speed with SEFLG_NONUT, longitude and right ascension, against the
 * five-point difference of the same two calls' position differences. The
 * Sun, Mars, Jupiter, Saturn and Pluto, ecliptic and equatorial, and
 * Polaris (swe_fixstar2()) the same way, on its raw right ascension, at
 * seven dates 1900-2100 and instants that are multiples of 2^-8 day, the
 * step. Within 2e-10 deg/day (measured <= 1.7e-11 for the planets, 6.7e-11
 * for Polaris's longitude); Polaris's right ascension within 3e-10
 * (measured 6.2e-11, the reference's rounding times 1/cos(declination) = 82;
 * it was 1.9e-9 until the mean obliquity's turning entered the nutation
 * matrix's rate, entry 69). The upstream
 * build fails 12 of 12: the planets and Polaris's longitude by 6.5e-10 to
 * 6.9e-10, Polaris's right ascension by 2.65e-8. Run as
 *   nutrate EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static int at(int star, int ipl, double t, int32 fl, double *x, char *serr)
{
  char s[SE_MAX_STNAME * 2 + 1];
  if (!star)
    return swe_calc(t, ipl, fl, x, serr);
  strcpy(s, "Polaris");
  return swe_fixstar2(s, t, fl, x, serr);
}

int main(int argc, char **argv)
{
  static const int pl[6] = {SE_SUN, SE_MARS, SE_JUPITER, SE_SATURN, SE_PLUTO, -1};
  static const double date[7] = {2415020.3, 2440000.7, 2451545.0, 2457000.1,
                                 2460000.3, 2470000.7, 2488070.1};
  const double h = 1.0 / 256;
  char serr[256];
  int p, d, e, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: nutrate EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G57: the nutation's rate in a speed of date is its rate at the instant\n");
  for (e = 0; e < 2; e++) {
    for (p = 0; p < 6; p++) {
      int32 fl = SEFLG_SWIEPH | (e ? SEFLG_EQUATORIAL : 0);
      double worst = 0;
      for (d = 0; d < 7; d++) {
        double t = floor(date[d] * 256) / 256, xa[6], xb[6], q[5], r, d1, d2, err;
        if (at(p == 5, pl[p], t, fl | SEFLG_SPEED, xa, serr) < 0
            || at(p == 5, pl[p], t, fl | SEFLG_SPEED | SEFLG_NONUT, xb, serr) < 0) {
          printf("  ERROR %s\n", serr);
          return 1;
        }
        for (i = 0; i < 5; i++) {
          double ya[6], yb[6], df;
          at(p == 5, pl[p], t + (i - 2) * h, fl, ya, serr);
          at(p == 5, pl[p], t + (i - 2) * h, fl | SEFLG_NONUT, yb, serr);
          df = ya[0] - yb[0];
          q[i] = df - 360 * floor(df / 360 + 0.5);
        }
        d1 = q[3] - q[1];
        d2 = q[4] - q[0];
        r = (8 * d1 - d2) / (12 * h);
        err = fabs((xa[3] - xb[3]) - r);
        if (err > worst) worst = err;
      }
      printf("  %-8s %-10s worst %.2e deg/day\n", p == 5 ? "Polaris" : swe_get_planet_name(pl[p], serr),
             e ? "equatorial" : "ecliptic", worst);
      if (worst > (p == 5 && e ? 3e-10 : 2e-10)) {
        printf("    FAIL: the nutation's rate in the speed is not the rate of the position\n");
        bad++;
      }
    }
  }
  if (bad) {
    printf("G57 FAIL: %d\n", bad);
    return 1;
  }
  printf("G57 PASS\n");
  return 0;
}
