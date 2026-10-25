/* moonaspeed.c -- G54: swe_nod_aps()'s speeds for the mean nodes and
 * apsides are the rates of its own positions: longitude, latitude and
 * distance, of the Moon and of a planet.
 *
 * Upstream defect in swe_nod_aps() (UPSTREAM-BUGS 55): the mean points'
 * speeds are differences over one day, not rates. The Moon's node and
 * perigee came from swi_mean_lunar_elements() as the position less the
 * position a day back, without the correction terms of the position
 * (2e-6 deg/day on the perigee's longitude); the aux. position of every
 * mean apsis was taken a day on, so that the Moon's perigee latitude
 * speed was off by 2e-5 deg/day (the perigee turns 0.11 deg/day on an
 * orbit inclined 5.1 degrees: half a day of curvature) once bug 38's
 * latitude speed was in, and the nodes' distance speed, the radius at two
 * arguments of perihelion a day apart, by 6e-10 AU/day.
 *
 * The check: the longitude, latitude and distance speed of the ascending
 * node, the descending node, the perigee/perihelion and the
 * apogee/aphelion, mean, ecliptic, of the Moon (geocentric and
 * heliocentric) and Mars (geocentric and heliocentric), against a
 * five-point difference of the library's own positions (h = 2^-6 day) at
 * seven binary-fraction instants, 1500..2500. Swiss files. Limits for the
 * Moon 3e-8 deg/day (longitude), 1e-8 (latitude), 2e-10 AU/day (distance),
 * ten times that for Mars, whose elements' t^2 and t^3 terms the rates
 * leave out (a separate matter, not UPSTREAM-BUGS 55: 4e-8 deg/day at
 * +-500 years).
 * Measured here: Moon geocentric lon 8.1e-10, lat 3.9e-11, dist 3.8e-15;
 * Moon heliocentric 7.9e-10, 4.7e-11, 5.8e-11; Mars at most 4.2e-8 (lon),
 * 5.8e-9, 6.6e-10. Before this change: Moon geocentric lon 4.7e-6, lat
 * 2.1e-5, dist 6.2e-10; heliocentric 2.8e-8, 5.7e-8, 5.6e-10; Mars the same.
 * The upstream build fails all four cases (Moon geocentric 5.2e-6 lon,
 * 5.1 lat; Mars geocentric 4.6e-2 lon, 2.6 lat; the latitude by bug 38).
 * Run as
 *   moonaspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static const char *rgPt[4] = {"asc node", "dsc node", "perigee", "apogee"};

static int nodaps(swe_ctx *c, double jd, int ipl, int32 fl, double x[4][6])
{
  char serr[256];
  return swe_nod_aps_r(c, jd, ipl, fl, SE_NODBIT_MEAN, x[0], x[1], x[2], x[3], serr);
}

int main(int argc, char **argv)
{
  struct { int ipl; int32 fl; const char *sz; } rgCase[] = {
    {SE_MOON, 0, "Moon geocentric"},
    {SE_MOON, SEFLG_HELCTR, "Moon heliocentric"},
    {SE_MARS, 0, "Mars geocentric"},
    {SE_MARS, SEFLG_HELCTR, "Mars heliocentric"},
  };
  const double lim[3] = {3e-8, 1e-8, 2e-10}; /* lon, lat deg/day; dist AU/day */
  const double H = 1.0 / 64;
  int p, y, j, k, i, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: moonaspeed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G54: mean nodes' and apsides' speeds are the rates of their positions\n");
  for (p = 0; p < 4; p++) {
    int32 fl = SEFLG_SWIEPH | rgCase[p].fl;
    double w[3] = {0, 0, 0};
    int wj[3] = {0, 0, 0};
    for (y = 0; y < 7; y++) {
      double jd = floor((2451545.0 + (-500 + y * 166.0) * 365.25 + 100.37) * 256) / 256;
      double x[4][6], d[5][4][6];
      if (nodaps(c, jd, rgCase[p].ipl, fl | SEFLG_SPEED, x) < 0) {
        printf("  %-20s ERROR\n", rgCase[p].sz);
        bad++;
        break;
      }
      for (k = 0; k < 5; k++)
        nodaps(c, jd + (k - 2) * H, rgCase[p].ipl, fl, d[k]);
      for (j = 0; j < 4; j++) {
        for (i = 0; i < 3; i++) {
          double v[5], r, e;
          for (k = 0; k < 5; k++) {
            v[k] = d[k][j][i];
            if (i == 0)
              v[k] = d[2][j][0] + swe_difdeg2n(v[k], d[2][j][0]);
          }
          r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * H);
          e = fabs(x[j][3 + i] - r);
          if (e > w[i]) { w[i] = e; wj[i] = j; }
        }
      }
    }
    printf("  %-20s lon %.2e (%s) lat %.2e (%s) dist %.2e (%s)\n", rgCase[p].sz,
           w[0], rgPt[wj[0]], w[1], rgPt[wj[1]], w[2], rgPt[wj[2]]);
    for (i = 0; i < 3; i++)
      if (w[i] > lim[i] * (rgCase[p].ipl == SE_MARS ? 10 : 1)) {
        printf("    FAIL: %s speed off by more than %.0e\n",
               i == 0 ? "longitude" : i == 1 ? "latitude" : "distance",
               lim[i] * (rgCase[p].ipl == SE_MARS ? 10 : 1));
        bad++;
      }
  }
  swe_ctx_free(c);
  printf(bad ? "G54 FAIL: %d\n" : "G54 PASS\n", bad);
  return bad ? 1 : 0;
}
