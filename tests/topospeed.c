/* topospeed.c -- G43: a topocentric speed with aberration is the rate of
 * the topocentric position, without three positions.
 *
 * Aberration moves the apparent place by about tau V, V the observer's
 * velocity, so its rate carries tau dV/dt. The planets' and the Moon's paths
 * took that as V(t) - V(t - tau); for a site turning with the Earth that is
 * off by a few per cent of a diurnal aberration rate of 5.6e-4 deg/day, and
 * the Sun's path had no such term (3.2e-4 deg/day). So swe_calc() computed
 * every topocentric speed with aberration from three positions 8.64 s apart
 * instead, at three times the cost, carrying the positions' 0.1 mas
 * roughness over that span (~3e-5 deg/day on the Moon) and, until bug 31,
 * the slope at t + dt. Now the site's part is tau times its transverse
 * acceleration at t, and the analytic speed serves (UPSTREAM-BUGS 34).
 *
 * The check: longitude and latitude speeds, ecliptic of date, default
 * flags, against a five-point difference of the library's own positions,
 * at Zurich and at 70 N, nine dates -999..2100, Swiss files. The instants
 * and the step (2^-8 day) are binary fractions, so t + k h is exact: at
 * rounded instants the positions differenced carry the JD's rounding over
 * the step, which on the topocentric Moon reads ~1.5e-6 deg/day by itself.
 * Within 1e-6 deg/day (measured <= 6e-7). The ancient dates hold the
 * site's turning per TT day (bug 36): without it the Moon is 2.9e-6 off at
 * -999. The upstream build fails all twelve (by up to 1.5e-3 on the
 * Moon). Run as
 *   topospeed EPHEDIR
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

int main(int argc, char **argv)
{
  static const int rgPl[6] = {SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER};
  static const char *rgszPl[6] = {"Sun", "Moon", "Mercury", "Venus", "Mars", "Jupiter"};
  static const double rgSite[2][3] = {{8.55, 47.37, 400}, {25.0, 70.0, 0}};
  static const double rgYear[9] = {-999, 1, 1800, 1900, 1950, 2000, 2026, 2050, 2100};
  char serr[256];
  int ip, is, id, k, i, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: topospeed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G43: topocentric speeds with aberration are the rate of the position\n");
  for (is = 0; is < 2; is++) {
    swe_set_topo_r(c, rgSite[is][0], rgSite[is][1], rgSite[is][2]);
    for (ip = 0; ip < 6; ip++) {
      int32 fl = SEFLG_SWIEPH | SEFLG_TOPOCTR;
      double w = 0, lim = 1e-6;
      for (id = 0; id < 9; id++) {
        double jd = floor((2451545.0 + (rgYear[id] - 2000) * 365.25 + 0.137 + is * 0.31)
                          * 256) / 256;
        double h = 1.0 / 256, x[6], f[5][6];
        if (swe_calc_r(c, jd, rgPl[ip], fl | SEFLG_SPEED, x, serr) < 0) {
          printf("  ERROR %s\n", serr);
          return 1;
        }
        for (k = 0; k < 5; k++)
          swe_calc_r(c, jd + (k - 2) * h, rgPl[ip], fl, f[k], serr);
        for (i = 0; i < 2; i++) {
          double r = (wrap(f[0][i] - f[2][i]) - 8 * wrap(f[1][i] - f[2][i])
                    + 8 * wrap(f[3][i] - f[2][i]) - wrap(f[4][i] - f[2][i])) / (12 * h);
          if (fabs(x[i + 3] - r) > w) w = fabs(x[i + 3] - r);
        }
      }
      printf("  %s %-8s worst %.2e deg/day (limit %.0e)\n", is ? "70 N  " : "Zurich",
             rgszPl[ip], w, lim);
      if (w > lim) {
        printf("    FAIL: the speed is not the rate of the position\n");
        bad++;
      }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G43 FAIL: %d\n" : "G43 PASS\n", bad);
  return bad ? 1 : 0;
}
