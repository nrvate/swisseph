/* nutobliq.c -- G68: the nutation matrix's turning in a speed of date
 * includes the mean obliquity's.
 *
 * swi_nutate() adds the turning of N = R1(-eps - deps) R3(-dpsi) R1(eps)
 * to the velocity as w x x, w from the nutation angles' rates with the mean
 * obliquity eps held (UPSTREAM-BUGS 49, 58). But eps moves, 3.6e-7 deg/day,
 * and R1(-eps - deps) and R1(eps) do not cancel when dpsi != 0: w left out
 * eps' (1 - cos dpsi, -sin dpsi cos(eps + deps), -sin dpsi sin(eps + deps)),
 * up to 3e-11 deg/day, 1/cos(declination) of that in a right ascension
 * near the pole (UPSTREAM-BUGS 69).
 *
 * The nutation's share of a speed alone, as G57 takes it: the speed of date
 * minus the speed with SEFLG_NONUT, against the five-point difference of the
 * same two calls' position differences, over 2^-6 day at instants that are
 * multiples of 2^-8 day, seven dates 1900-2100. Polaris, Polaris Australis
 * (near the equator's poles), delta Dor and 42 Dra (near the ecliptic's),
 * the Sun, Mars, Saturn and Pluto, ecliptic and equatorial, longitude (right
 * ascension) and latitude (declination): 32 checks. Longitudes within 3e-11
 * deg/day, latitudes within 5e-12. Before the fix 20 of 32 failed, by
 * amounts independent of the step: Polaris's right ascension 1.88e-9,
 * Polaris Australis's 2.04e-9, delta Dor's ecliptic longitude 4.06e-10,
 * 42 Dra's 1.96e-10, the polar stars' other longitudes 6.7e-11 to 7.3e-11,
 * the planets' latitudes and declinations 1.3e-11 to 2.2e-11, the stars'
 * 1.8e-11 to 2.8e-11. Now longitudes <= 1.2e-11 and latitudes <= 1.2e-12:
 * the difference's own rounding (Polaris's right ascension 6.2e-11 over
 * 2^-8 day, 1.1e-11 over 2^-6 and 2^-5; 2.5e-13 deg/day on the sky).
 * The planets' longitudes are 1e-12..5e-12 either way. The upstream build
 * fails all 32 (bug 58's backward difference besides: Polaris's right
 * ascension 2.1e-8, every ecliptic latitude 4e-6 to 1.4e-5). Run as
 *   nutobliq EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static int at(const char *name, int ipl, double t, int32 fl, double *x, char *serr)
{
  char s[SE_MAX_STNAME * 2 + 1];
  if (!name)
    return swe_calc(t, ipl, fl, x, serr);
  strcpy(s, name);
  return swe_fixstar2(s, t, fl, x, serr);
}

static double wrap(double d) { return d - 360 * floor(d / 360 + 0.5); }

int main(int argc, char **argv)
{
  static const char *star[8] = {"Polaris", "Polaris Australis", ",deDor", "Fafnir",
                                NULL, NULL, NULL, NULL};
  static const int pl[8] = {0, 0, 0, 0, SE_SUN, SE_MARS, SE_SATURN, SE_PLUTO};
  static const double date[7] = {2415020.3, 2440000.7, 2451545.0, 2457000.1,
                                 2460000.3, 2470000.7, 2488070.1};
  static const char *comp[2][2] = {{"longitude", "latitude"}, {"RA", "declination"}};
  const double h = 1.0 / 64;
  char serr[256], nm[64];
  int p, d, e, c, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: nutobliq EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G68: the nutation matrix's turning includes the mean obliquity's\n");
  for (p = 0; p < 8; p++) {
    for (e = 0; e < 2; e++) {
      int32 fl = SEFLG_SWIEPH | (e ? SEFLG_EQUATORIAL : 0);
      double worst[2] = {0, 0};
      for (d = 0; d < 7; d++) {
        double t = floor(date[d] * 256) / 256, xa[6], xb[6], q[2][5];
        if (at(star[p], pl[p], t, fl | SEFLG_SPEED, xa, serr) < 0
            || at(star[p], pl[p], t, fl | SEFLG_SPEED | SEFLG_NONUT, xb, serr) < 0) {
          printf("  ERROR %s\n", serr);
          return 1;
        }
        for (i = 0; i < 5; i++) {
          double ya[6], yb[6];
          if (at(star[p], pl[p], t + (i - 2) * h, fl, ya, serr) < 0
              || at(star[p], pl[p], t + (i - 2) * h, fl | SEFLG_NONUT, yb, serr) < 0) {
            printf("  ERROR %s\n", serr);
            return 1;
          }
          for (c = 0; c < 2; c++)
            q[c][i] = wrap(ya[c] - yb[c]);
        }
        for (c = 0; c < 2; c++) {
          double r = (8 * wrap(q[c][3] - q[c][1]) - wrap(q[c][4] - q[c][0])) / (12 * h);
          double err = fabs((xa[3 + c] - xb[3 + c]) - r);
          if (err > worst[c]) worst[c] = err;
        }
      }
      if (star[p])
        strcpy(nm, star[p][0] == ',' ? "delta Dor" : strcmp(star[p], "Fafnir") ? star[p] : "42 Dra");
      else
        swe_get_planet_name(pl[p], nm);
      for (c = 0; c < 2; c++) {
        printf("  %-18s %-10s %-11s worst %.2e deg/day\n", nm, e ? "equatorial" : "ecliptic",
               comp[e][c], worst[c]);
        if (worst[c] > (c ? 5e-12 : 3e-11)) {
          printf("    FAIL: the nutation's turning in the speed leaves out the mean obliquity's\n");
          bad++;
        }
      }
    }
  }
  if (bad) {
    printf("G68 FAIL: %d\n", bad);
    return 1;
  }
  printf("G68 PASS\n");
  return 0;
}
