/* lunarpulleph.c -- G65: the aberration rate's Earth carries the Moon's pull
 * and the planets' on every ephemeris.
 *
 * UPSTREAM-BUGS 62 gave the aberration rate's tau a(teval) term the Moon's
 * pull from the Swiss save area. JPL and Moshier kept the Sun-only a' and the
 * 10-day switch: Nibiru, the Christ Comet, Planet 9, VEA were off their
 * positions' rate by 2e-7 to 6e-7 deg/day (UPSTREAM-BUGS 66). Part 2: the
 * Moon is now the one the Earth was made with, on JPL (the EMB-less-Moon
 * read) and Moshier (embofs), for every light time, so Harrington and Pluto
 * (under 0.3 day) get its rate too (1.4e-8 to 1.7e-8 before, 1e-9 after);
 * and past 0.3 day the whole tau a takes the pull of Mercury, Venus, Mars,
 * Jupiter and Saturn as well as the Sun's and Moon's (4e-9 on the far
 * bodies before, under 1e-9 after, on Swiss too).
 *
 * The check: Nibiru, Harrington, the Christ Comet, Planet 9, VEA, Pluto,
 * Zeus and Kronos, apparent geocentric and topocentric (Zurich), Swiss
 * files, de200.eph and Moshier, seven dates 1650..2150: longitude and
 * latitude speeds within 2e-9 deg/day (measured <= 1.06e-9 on all three;
 * Moshier took 6.2e-9 before part 3) of a five-point difference of the positions over 2^-7 day at
 * multiples of it. The build before part 2 fails Swiss at 4.6e-9, JPL and Moshier up to
 * 1.7e-8; the
 * build before part 3 fails Moshier's far bodies at 2e-9 to 6e-9; the
 * build before part 1 and the upstream build fail them all. Run as
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
    {SE_ZEUS, "Zeus"}, {SE_KRONOS, "Kronos"},
  };
  static const char *const eph[3] = {"Swiss", "JPL", "Mosh"};
  static const double y0[3] = {1650, 1650, 1650};	/* first year, per ephemeris */
  static const double dy[3] = {500.0 / 6, 500.0 / 6, 500.0 / 6};
  static const double lim[3] = {2e-9, 2e-9, 2e-9};	/* deg/day */
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
  printf("G65: the aberration rate's Earth carries the Moon's and planets' pull\n");
  for (ie = 0; ie < 3; ie++) {
    for (ib = 0; ib < 8; ib++) {
      for (ic = 0; ic < 2; ic++) {
        int32 fl = (ie == 2 ? SEFLG_MOSEPH : ie ? SEFLG_JPLEPH : SEFLG_SWIEPH) | (ic ? SEFLG_TOPOCTR : 0);
        double w[2] = {0, 0};
        for (y = 0; y < 7; y++) {
          double jd = floor((2451545.0 + (y0[ie] + y * dy[ie] - 2000) * 365.25 + 37.3 * y) * 256) / 256;
          double x[6], d[5][6];
          if (swe_calc(jd, rgb[ib].ipl, fl | SEFLG_SPEED, x, serr) < 0) {
            printf("  %-6s %-14s ERROR %s\n", eph[ie], rgb[ib].sz, serr);
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
        printf("  %-6s %-14s %-5s lon %.2e lat %.2e deg/day\n", eph[ie],
               rgb[ib].sz, ic ? "topo" : "geo", w[0], w[1]);
        if (w[0] > lim[ie] || w[1] > lim[ie]) {
          printf("    FAIL: speed off its positions' rate by more than %.0e deg/day\n", lim[ie]);
          bad++;
        }
      }
    }
  }
  printf(bad ? "G65 FAIL: %d\n" : "G65 PASS\n", bad);
  return bad ? 1 : 0;
}
