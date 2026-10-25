/* node3speed.c -- G53: SEFLG_SPEED3 gives the true node's and the osculating
 * apogee's speeds as the rates of their positions.
 *
 * The defect (UPSTREAM-BUGS 54): swe_calc_r() took the SEFLG_SPEED3 speed of
 * SE_TRUE_NODE and SE_OSCU_APOG from positions +-0.1 day apart on every
 * ephemeris, the step that once averaged out the Moshier Moon's noise.
 * Their positions are smooth since bug 51, and the h^2 term of a
 * three-point slope over 0.1 day is 1.4e-3 deg/day on the apogee (3.5e-5 on
 * the node, 1.1e-4 / 5e-9 AU/day in latitude / distance). Now +-2^-9 day, where that term and the positions' noise
 * balance.
 *
 * The check: both bodies at eight epochs (-2500..2800 Swiss files and
 * Moshier; 1700..2100 de200, which spans 1600-2170), at binary-fraction
 * instants, longitude, latitude and distance speeds of SEFLG_SPEED3 against
 * a five-point difference over 2^-5 day of the library's positions, within
 * 3e-6 deg/day and 1e-10 AU/day. Measured: apogee longitude <= 6e-7 (Swiss,
 * 2.4e-7 de200) and 1.3e-6 (Moshier), node 2e-8, latitude <= 1e-7, distance 2e-12.
 * The upstream build fails by 0.19 deg/day on the apogee and 3.6e-3 on the
 * node (with bug 31's late slope on top of the 0.1 day step), and 7e-7 AU/day.
 * Run as
 *   node3speed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static const struct { int ipl; const char *sz; } rgObj[2] = {
  {SE_TRUE_NODE, "true node"}, {SE_OSCU_APOG, "osc. apogee"},
};
static const struct { int32 fl; const char *sz; int fJpl; } rgEph[3] = {
  {SEFLG_SWIEPH, "Swiss", 0}, {SEFLG_MOSEPH, "Moshier", 0}, {SEFLG_JPLEPH, "de200", 1},
};
static const double rgYear[8] = {-2500, -1000, 0, 1500, 1900, 2000, 2100, 2800};
static const double rgYearJ[8] = {1650, 1700, 1800, 1900, 1950, 2000, 2050, 2100};

int main(int argc, char **argv)
{
  int e, o, y, k, j, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: node3speed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_jpl_file_r(c, "de200.eph");
  printf("G53: SPEED3 of the true node and osculating apogee are the rates of their positions\n");
  for (e = 0; e < 3; e++) {
    for (o = 0; o < 2; o++) {
      double w[3] = {0, 0, 0};
      for (y = 0; y < 8; y++) {
        double yr = rgEph[e].fJpl ? rgYearJ[y] : rgYear[y];
        double jd = 2451545.0 + (yr - 2000) * 365.25 + 0.1234567;
        double t = floor(jd * 256) / 256, h = 1.0 / 32;
        double x[6], p[5][6];
        char serr[256];
        if (swe_calc_r(c, t, rgObj[o].ipl, rgEph[e].fl | SEFLG_SPEED3, x, serr) < 0) {
          printf("  ERROR %s jd %.4f: %s\n", rgObj[o].sz, t, serr);
          return 1;
        }
        for (k = 0; k < 5; k++)
          if (swe_calc_r(c, t + (k - 2) * h, rgObj[o].ipl, rgEph[e].fl, p[k], serr) < 0) {
            printf("  ERROR %s jd %.4f: %s\n", rgObj[o].sz, t, serr);
            return 1;
          }
        for (j = 0; j < 3; j++) {
          double q[5], r, err;
          for (k = 0; k < 5; k++)
            q[k] = j == 0 ? swe_difdeg2n(p[k][0], p[2][0]) : p[k][j];
          r = (q[0] - 8 * q[1] + 8 * q[3] - q[4]) / (12 * h);
          err = fabs(x[3 + j] - r);
          if (err > w[j]) w[j] = err;
        }
      }
      printf("  %-7s %-11s lon %.1e lat %.1e deg/day, dist %.1e AU/day\n",
             rgEph[e].sz, rgObj[o].sz, w[0], w[1], w[2]);
      if (w[0] > 3e-6 || w[1] > 3e-6 || w[2] > 1e-10)
        bad++;
    }
  }
  if (bad) {
    printf("G53 FAIL: %d\n", bad);
    return 1;
  }
  printf("G53 PASS\n");
  return 0;
}
