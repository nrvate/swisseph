/* lunarpulleph.c -- G65: a far body's aberration rate carries the Moon's pull
 * on the Earth on the JPL and Moshier ephemerides too.
 *
 * UPSTREAM-BUGS 62 gave the aberration rate's tau a(teval) term the Moon's
 * pull (0.6% of a, 7.5% of a') from the geocentric Moon in the Swiss save
 * area. JPL and Moshier have no Moon there, so they kept the Sun-only a' and
 * the 10-day switch: Nibiru, the Christ Comet, Planet 9, VEA and Harrington
 * were off their positions' rate by 2e-7 to 5e-7 deg/day (UPSTREAM-BUGS 66).
 * The Moon at teval is now read for them (JPL: J_MOON about J_EARTH;
 * Moshier: the lunar theory) when the light time is past 0.3 day, and the
 * same whole-form tau (a_Sun + a_m) is taken there.
 *
 * The check: Nibiru, the Christ Comet, Planet 9, VEA, Harrington and Pluto,
 * apparent geocentric and topocentric (Zurich), on Moshier (seven dates
 * -300..+300 years) and on de200.eph (seven dates 1650..2150): longitude and
 * latitude speeds within 5e-8 deg/day of a five-point difference of the
 * positions over 2^-7 day at multiples of it. Measured <= 3.6e-8 (Harrington
 * topocentric, which stays below the switch; the rest <= 2.8e-8). The build
 * before the fix fails 16 of the 24 rows (Nibiru, the Christ Comet, Planet 9,
 * VEA) at 5.9e-8 to 6.0e-7 deg/day; the upstream build fails them all (and
 * Harrington's, by its missing aberration rate). Run as
 *   lunarpulleph EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(int argc, char **argv)
{
  static const struct { int ipl; const char *sz; } rgb[] = {
    {SE_FICT_OFFSET + 9, "Nibiru"}, {SE_FICT_OFFSET + 11, "Harrington"},
    {SE_FICT_OFFSET + 19, "Christ Comet"}, {SE_FICT_OFFSET + 20, "Planet 9"},
    {SE_FICT_OFFSET + 25, "VEA"}, {SE_PLUTO, "Pluto"},
  };
  static const double y0[2] = {1650, -300};	/* first year, per ephemeris */
  static const double dy[2] = {500.0 / 6, 100};
  const double h = 1.0 / 128;
  char serr[256];
  int ie, ib, ic, y, k, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: lunarpulleph EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_jpl_file("de200.eph");
  swe_set_topo(8.55, 47.37, 400);
  printf("G65: far bodies' speeds carry the Moon's pull on JPL and Moshier\n");
  for (ie = 0; ie < 2; ie++) {
    for (ib = 0; ib < 6; ib++) {
      for (ic = 0; ic < 2; ic++) {
        int32 fl = (ie ? SEFLG_MOSEPH : SEFLG_JPLEPH) | (ic ? SEFLG_TOPOCTR : 0);
        double w[2] = {0, 0};
        for (y = 0; y < 7; y++) {
          double jd = floor((2451545.0 + (y0[ie] + y * dy[ie] - 2000) * 365.25 + 37.3 * y) * 256) / 256;
          double x[6], d[5][6];
          if (swe_calc(jd, rgb[ib].ipl, fl | SEFLG_SPEED, x, serr) < 0) {
            printf("  %-6s %-14s ERROR %s\n", ie ? "Mosh" : "JPL", rgb[ib].sz, serr);
            bad++;
            break;
          }
          for (k = 0; k < 5; k++)
            swe_calc(jd + (k - 2) * h, rgb[ib].ipl, fl, d[k], serr);
          for (i = 0; i < 2; i++) {
            double v[5], r, e;
            for (k = 0; k < 5; k++) {
              v[k] = d[k][i];
              if (i == 0)
                v[k] = d[2][0] + swe_difdeg2n(v[k], d[2][0]);
            }
            r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
            e = fabs(x[3 + i] - r);
            if (e > w[i])
              w[i] = e;
          }
        }
        printf("  %-6s %-14s %-5s lon %.2e lat %.2e deg/day\n", ie ? "Mosh" : "JPL",
               rgb[ib].sz, ic ? "topo" : "geo", w[0], w[1]);
        if (w[0] > 5e-8 || w[1] > 5e-8) {
          printf("    FAIL: speed off its positions' rate by more than 5e-8 deg/day\n");
          bad++;
        }
      }
    }
  }
  printf(bad ? "G65 FAIL: %d\n" : "G65 PASS\n", bad);
  return bad ? 1 : 0;
}
