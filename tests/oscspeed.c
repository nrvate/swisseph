/* oscspeed.c -- G74: the true node's and the osculating apogee's speeds
 * are the rates of their returned positions.
 *
 * The defects (UPSTREAM-BUGS 75): lunar_osc_elem() took the speeds from
 * the Moon at tjd +- 0.0001 day and divided by 2 x 0.0001, but those
 * epochs, less the light-time, are rounded to the date's grid (2^-31 day
 * after JD 2097152), so the interval is off by up to 2.3e-6 of itself:
 * steadily (1.7e-6 at a binary date) and at random with the light-time.
 * Every speed was off by that fraction of itself on the Swiss files and
 * JPL. And with SEFLG_J2000 the node and apogee, found in the ecliptic of
 * date and precessed to J2000, left the mean obliquity's rate out of the
 * equatorial speed of date, on every ephemeris.
 *
 * Then the step: over 0.0001 day the Moon's velocity noise, which the
 * apogee amplifies twentyfold, left 5e-7 deg/day on its speed (1.7e-6 on
 * de200), and Moshier's three points over 2^-8 day 3.2e-6 (the node
 * 6.8e-8). Now five points over 2^-6 day (Moshier 2^-5).
 *
 * The check: both bodies on the Swiss files, de200 and Moshier, at eight
 * epochs (-2500..2800, de200 1650..2100), binary-fraction instants, the
 * speeds of longitude, latitude and distance against a nine-point
 * difference over 2^-3 day of the library's positions (eighth order: its
 * own truncation and noise below 1e-11 on the node), in the ecliptic and
 * the equator, of date, without nutation and J2000. Within 1e-9 deg/day
 * for the node (measured <= 1.0e-10), 5e-8 for the apogee (<= 8.6e-9),
 * 5e-9 and 4e-7 on Moshier (6.5e-10, 7.2e-8), and 4e-13 AU/day in
 * distance (4.6e-14; Moshier 2e-12, 3.1e-13). Over 150 dates the worst
 * are 3.2e-10, 3.8e-8, 6.6e-9 and 4.7e-7. The build with the step fixed
 * but not the step size (UPSTREAM-BUGS 75's first part) fails 20 of the
 * 30 rows: the Swiss node by 1.6e-9, the apogee by 1.4e-7, Moshier's node
 * by 5.1e-8 and apogee by 2.0e-6, Moshier's distances by 6.7e-12 and
 * 2.9e-11. The build before UPSTREAM-BUGS 75 and the upstream build fail
 * all 30. Run as
 *   oscspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static const struct { int32 fl; const char *sz; } rgFl[5] = {
  {0, "ecl"}, {SEFLG_EQUATORIAL, "equ"},
  {SEFLG_NONUT | SEFLG_EQUATORIAL, "equ nonut"},
  {SEFLG_J2000, "ecl J2000"}, {SEFLG_J2000 | SEFLG_EQUATORIAL, "equ J2000"},
};
static const struct { int32 fl; int ipl; const char *sz; double lim, dlim; } rgCase[6] = {
  {SEFLG_SWIEPH, SE_TRUE_NODE, "Swiss true node", 1e-9, 4e-13},
  {SEFLG_SWIEPH, SE_OSCU_APOG, "Swiss osc. apogee", 5e-8, 4e-13},
  {SEFLG_JPLEPH, SE_TRUE_NODE, "de200 true node", 1e-9, 4e-13},
  {SEFLG_JPLEPH, SE_OSCU_APOG, "de200 osc. apogee", 5e-8, 4e-13},
  {SEFLG_MOSEPH, SE_TRUE_NODE, "Moshier true node", 5e-9, 2e-12},
  {SEFLG_MOSEPH, SE_OSCU_APOG, "Moshier osc. apogee", 4e-7, 2e-12},
};
/* the nine-point central difference, eighth order: over 2^-3 day its
 * truncation is ~1e-12 deg/day and its noise a fiftieth of a five-point's
 * over 2^-6 */
static const double rgC9[4] = {4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280};
static const double rgYear[8] = {-2500, -1000, 0, 1500, 1900, 2000, 2100, 2800};
static const double rgYearJ[8] = {1650, 1700, 1800, 1900, 1950, 2000, 2050, 2100};

int main(int argc, char **argv)
{
  int k, f, y, j, i, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: oscspeed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_jpl_file_r(c, "de200.eph");
  printf("G74: the true node's and osculating apogee's speeds are the rates of their positions\n");
  for (k = 0; k < 6; k++) {
    for (f = 0; f < 5; f++) {
      int32 fl = rgCase[k].fl | rgFl[f].fl;
      double w[3] = {0, 0, 0};
      for (y = 0; y < 8; y++) {
        double yr = (rgCase[k].fl & SEFLG_JPLEPH) ? rgYearJ[y] : rgYear[y];
        double jd = 2451545.0 + (yr - 2000) * 365.25 + 0.1234567;
        double t = floor(jd * 256) / 256, h = 1.0 / 8;
        double x[6], p[9][6];
        char serr[256];
        int32 rf = swe_calc_r(c, t, rgCase[k].ipl, fl | SEFLG_SPEED, x, serr);
        if (rf < 0 || (rf & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)) != rgCase[k].fl) {
          printf("  ERROR %s jd %.4f: %s\n", rgCase[k].sz, t, serr);
          swe_ctx_free(c);
          return 1;
        }
        for (j = 0; j < 9; j++)
          swe_calc_r(c, t + (j - 4) * h, rgCase[k].ipl, fl, p[j], serr);
        for (j = 0; j < 9; j++)
          if (j != 4)
            p[j][0] = p[4][0] + swe_difdeg2n(p[j][0], p[4][0]);
        for (i = 0; i < 3; i++) {
          double r = 0;
          int m;
          for (m = 1; m <= 4; m++)
            r += rgC9[m - 1] * (p[4 + m][i] - p[4 - m][i]);
          r /= h;
          if (fabs(x[3 + i] - r) > w[i])
            w[i] = fabs(x[3 + i] - r);
        }
      }
      printf("  %-18s %-10s lon %.1e lat %.1e deg/day dist %.1e AU/day\n",
             rgCase[k].sz, rgFl[f].sz, w[0], w[1], w[2]);
      if (w[0] > rgCase[k].lim || w[1] > rgCase[k].lim || w[2] > rgCase[k].dlim) {
        printf("    FAIL: beyond %.1e deg/day or %.0e AU/day\n", rgCase[k].lim, rgCase[k].dlim);
        bad++;
      }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G74 FAIL: %d\n" : "G74 PASS\n", bad);
  return bad ? 1 : 0;
}
