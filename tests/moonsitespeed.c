/* moonsitespeed.c -- G72: the Swiss-file Moon's speed, geocentric and
 * topocentric, is the rate of its position.
 *
 * The defects (UPSTREAM-BUGS 73), against differences of the library's own
 * positions:
 *  - the site turned at EARTH_ROT_SPEED, the Earth's rate against inertial
 *    space, in the frame of the equinox of date, whose precession is added
 *    again by swi_precess_speed(); and the true pole's turning (the
 *    nutation matrix's rate, with the equation of the equinoxes' rate) was
 *    left out: the site's velocity off by 2-6 m/day, the topocentric
 *    Moon's speed by up to 8.7e-7 deg/day;
 *  - the files are read at TDB (bug 24) and their derivative is per TDB
 *    day: 5e-9 deg/day of the Moon's speed;
 *  - the Moon's velocity was the one at the instant read, teval - dt, not
 *    at teval - tau, 1e-8 deg/day geocentric;
 *  - the aberration's shift along the Moon's motion was a backward
 *    difference over 1e-4 day, 1.5e-8 deg/day.
 *
 * The check: the Moon's longitude and latitude speeds, ecliptic of date,
 * Swiss files, with and without SEFLG_NOABERR, ten epochs -2900..2900 at
 * multiples of the step, against five-point differences of the same call's
 * positions:
 *  (a) geocentric, step 2^-6 day: within 5e-9 deg/day (measured 1.4e-9;
 *      before 2.4e-8);
 *  (b) topocentric at 0, 30, 47.37 and -65 deg latitude, step 2^-8 day:
 *      within 4e-7 (measured 7.5e-8, the step's truncation; before 8.7e-7;
 *      2.5e-7 before UPSTREAM-BUGS 140 placed the site by UT1 without the
 *      4.7e-10 day TT - delta t rounds to, the site's jitter);
 *  (c) the same sites with delta t set to 2^-11 day, so that UT1 is exact
 *      and the site has no jitter, step 2^-9 day: within 3e-8 (measured
 *      1.05e-8, the step's truncation; before 7.5e-7; with a 17-point
 *      difference over 2^-5 day 6e-10, before 6.2e-7).
 * The pre-fix build fails (a), (b) and (c); the upstream build fails all
 * three, by 6.9e-5, 1.7e-3 and 2.3e-3 (its topocentric paths and light
 * time, bugs 31-36, too). Run as
 *   moonsitespeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static double wrap(double d)
{
  while (d > 180) d -= 360;
  while (d < -180) d += 360;
  return d;
}

/* worst |speed - five-point rate| in longitude and latitude over the ten
 * epochs, step 2^-k day */
static double worst(swe_ctx *c, int32 fl, int k)
{
  static const double rgYear[10] = {-2900, -1000, 500, 1500, 1900, 1990, 2000.3,
                                    2030, 2200, 2900};
  double w = 0, h = ldexp(1, -k);
  char serr[256];
  int id, j, i;
  for (id = 0; id < 10; id++) {
    double jd = floor((2451545.0 + (rgYear[id] - 2000) * 365.25) * 256) / 256;
    double x[6], f[5][6];
    if (swe_calc_r(c, jd, SE_MOON, fl | SEFLG_SPEED, x, serr) < 0) {
      printf("  ERROR %s\n", serr);
      return 1;
    }
    for (j = 0; j < 5; j++)
      swe_calc_r(c, jd + (j - 2) * h, SE_MOON, fl, f[j], serr);
    for (i = 0; i < 2; i++) {
      double r = (wrap(f[0][i] - f[2][i]) - 8 * wrap(f[1][i] - f[2][i])
                + 8 * wrap(f[3][i] - f[2][i]) - wrap(f[4][i] - f[2][i])) / (12 * h);
      if (fabs(x[i + 3] - r) > w) w = fabs(x[i + 3] - r);
    }
  }
  return w;
}

int main(int argc, char **argv)
{
  static const double rgLat[4] = {0, 30, 47.37, -65};
  static const char *rgCase[3] = {"geocentric, 2^-6 day",
                                  "topocentric, 2^-8 day",
                                  "topocentric, delta t 2^-11 day, 2^-9 day"};
  static const double rgLim[3] = {5e-9, 4e-7, 3e-8};
  int ic, il, na, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: moonsitespeed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G72: the Swiss-file Moon's speed, geocentric and topocentric, is the rate of its position\n");
  for (ic = 0; ic < 3; ic++) {
    double w = 0;
    if (ic == 2)
      swe_set_delta_t_userdef_r(c, ldexp(1, -11));
    for (na = 0; na < 2; na++) {
      int32 fl = SEFLG_SWIEPH | (na ? SEFLG_NOABERR : 0);
      if (ic == 0) {
        double v = worst(c, fl, 6);
        if (v > w) w = v;
        continue;
      }
      for (il = 0; il < 4; il++) {
        double v;
        swe_set_topo_r(c, 8.55, rgLat[il], 400);
        v = worst(c, fl | SEFLG_TOPOCTR, ic == 1 ? 8 : 9);
        if (v > w) w = v;
      }
    }
    printf("  %-44s worst %.2e deg/day (limit %.0e)\n", rgCase[ic], w, rgLim[ic]);
    if (w > rgLim[ic]) {
      printf("    FAIL: the speed is not the rate of the position\n");
      bad++;
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G72 FAIL: %d\n" : "G72 PASS\n", bad);
  return bad ? 1 : 0;
}
