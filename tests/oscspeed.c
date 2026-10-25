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
 * The check: both bodies on the Swiss files and de200, and the node on
 * Moshier, at eight epochs (-2500..2800, de200 1650..2100), binary-fraction
 * instants, the speeds of longitude, latitude and distance against a
 * five-point difference over 2^-6 day of the library's positions, in the
 * ecliptic and the equator, of date, without nutation and J2000. Within
 * 2e-8 deg/day for the node (measured <= 1.5e-9), 5e-7 for the apogee
 * (<= 9.6e-8: the Moon's velocity noise over the 0.0001-day step), 1.5e-7
 * for the Moshier node (5.2e-8, bug 51's 2^-8-day step), and 5e-12 AU/day
 * in distance (1.9e-13; Moshier 2e-11, 6.7e-12). The build before the fix
 * fails 22 of the 25 rows: the node's longitude by 9.4e-8 (Swiss) and
 * 4.5e-7 (de200), its J2000 latitude by 3.5e-7 (also on Moshier), the
 * apogee's longitude by 1.1-1.3e-5 and the distances by 2.5-3.3e-11
 * AU/day. The upstream build fails all 25. Run as
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
static const struct { int32 fl; int ipl; const char *sz; double lim, dlim; } rgCase[5] = {
  {SEFLG_SWIEPH, SE_TRUE_NODE, "Swiss true node", 2e-8, 5e-12},
  {SEFLG_SWIEPH, SE_OSCU_APOG, "Swiss osc. apogee", 5e-7, 5e-12},
  {SEFLG_JPLEPH, SE_TRUE_NODE, "de200 true node", 2e-8, 5e-12},
  {SEFLG_JPLEPH, SE_OSCU_APOG, "de200 osc. apogee", 5e-7, 5e-12},
  {SEFLG_MOSEPH, SE_TRUE_NODE, "Moshier true node", 1.5e-7, 2e-11},
};
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
  for (k = 0; k < 5; k++) {
    for (f = 0; f < 5; f++) {
      int32 fl = rgCase[k].fl | rgFl[f].fl;
      double w[3] = {0, 0, 0};
      for (y = 0; y < 8; y++) {
        double yr = (rgCase[k].fl & SEFLG_JPLEPH) ? rgYearJ[y] : rgYear[y];
        double jd = 2451545.0 + (yr - 2000) * 365.25 + 0.1234567;
        double t = floor(jd * 256) / 256, h = 1.0 / 64;
        double x[6], p[5][6];
        char serr[256];
        int32 rf = swe_calc_r(c, t, rgCase[k].ipl, fl | SEFLG_SPEED, x, serr);
        if (rf < 0 || (rf & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)) != rgCase[k].fl) {
          printf("  ERROR %s jd %.4f: %s\n", rgCase[k].sz, t, serr);
          swe_ctx_free(c);
          return 1;
        }
        for (j = 0; j < 5; j++)
          swe_calc_r(c, t + (j - 2) * h, rgCase[k].ipl, fl, p[j], serr);
        for (j = 0; j < 5; j++)
          if (j != 2)
            p[j][0] = p[2][0] + swe_difdeg2n(p[j][0], p[2][0]);
        for (i = 0; i < 3; i++) {
          double r = (p[0][i] - 8 * p[1][i] + 8 * p[3][i] - p[4][i]) / (12 * h);
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
