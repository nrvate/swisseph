/* nodapsmosh.c -- G92: on Moshier the planets' mean and osculating points'
 * apparent speeds are the rates of their positions.
 *
 * Upstream defect in swe_nod_aps() (UPSTREAM-BUGS 93), on Moshier only: the
 * apparent points' speed carries the aberration's rate A(x + s x', V + s a)
 * (G69, UPSTREAM-BUGS 70) with a, the Earth's acceleration, from
 * swi_observer_accel_lt(): the velocity at t less the Earth's at the light
 * time t - tau carried back by a Taylor form in the gravitational jerk.
 * The Moshier Earth's own acceleration is not the gravitational one (the
 * series' monthly terms, 1e-4 of a), and a + tau^2 a''/6 less the jerk's
 * residue is 5e-10 to 1e-9 AU/day^2 off the acceleration of the Earth's
 * velocity: 6e-12 rad/day through the aberration, 1.7e-10 deg/day on
 * Neptune's points' latitude speed, 1.1e-10 on Uranus's, 3e-11 on
 * Jupiter's and Saturn's, 3e-10 on Neptune's mean longitude speed. With
 * SEFLG_NOABERR the same points are 1e-12 off; on JPL and Swiss files the
 * default-flags case is 2e-11.
 *
 * The check: the longitude and latitude speed of the ascending node,
 * descending node, perihelion and aphelion, mean and osculating, ecliptic,
 * of Jupiter..Neptune, geocentric, SEFLG_MOSEPH and default flags, at seven
 * instants -3000..3000, against a five-point difference of the library's
 * own positions (h = 2^-6 day) at binary-fraction instants. Limits, mean
 * points: longitude 3e-11 deg/day, latitude 1e-11; osculating points:
 * longitude 5e-10, latitude 1e-11. Measured: mean longitude <= 7.4e-12,
 * latitude <= 9.1e-13; osculating longitude <= 3.9e-10 (the reference's
 * floor, 3.8e-10 under SEFLG_NOABERR too), latitude <= 1.6e-12. Before the
 * fix: mean longitude 3.1e-10 (Neptune), latitude 3e-11 (Jupiter) to
 * 1.7e-10 (Neptune); osculating longitude 6.2e-10 (Neptune), latitude the
 * same; 13 of the 16 checks fail. The upstream build fails (and is off by
 * 1e-2 deg/day and more on other counts).
 * Run as
 *   nodapsmosh EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static const char *rgPt[4] = {"asc node", "dsc node", "perihelion", "aphelion"};

static int nodaps(swe_ctx *c, double jd, int ipl, int32 fl, int32 meth, double x[4][6])
{
  char serr[256];
  return swe_nod_aps_r(c, jd, ipl, fl, meth, x[0], x[1], x[2], x[3], serr);
}

int main(int argc, char **argv)
{
  double lim[2];
  const double H = 1.0 / 64;
  int32 extra = 0;
  int ipl, im, y, j, k, i, bad = 0;
  swe_ctx *c;

  if (argc < 2) {
    fprintf(stderr, "usage: nodapsmosh EPHEDIR\n");
    return 2;
  }
  if (argc > 2 && !strcmp(argv[2], "noaberr")) extra = SEFLG_NOABERR;
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G92: Moshier points' speeds are the rates of their positions\n");
  for (ipl = SE_JUPITER; ipl <= SE_NEPTUNE; ipl++) {
    for (im = 0; im < 2; im++) {
      int32 meth = im ? SE_NODBIT_OSCU : SE_NODBIT_MEAN;
      int32 fl = SEFLG_MOSEPH | extra;
      double w[2] = {0, 0};
      lim[0] = im ? 5e-10 : 3e-11;
      lim[1] = 1e-11;
      int wj[2] = {0, 0}, ok = 1;
      for (y = 0; y < 7; y++) {
        double jd = floor((2451545.0 + (-3000 - 2000 + y * 1000.0) * 365.25 + 100.37) * 256) / 256;
        double x[4][6], d[5][4][6];
        if (nodaps(c, jd, ipl, fl | SEFLG_SPEED, meth, x) < 0) {
          printf("  planet %d ERROR at %.1f\n", ipl, jd);
          bad++; ok = 0;
          break;
        }
        for (k = 0; k < 5; k++)
          nodaps(c, jd + (k - 2) * H, ipl, fl, meth, d[k]);
        for (j = 0; j < 4; j++) {
          for (i = 0; i < 2; i++) {
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
      if (!ok) continue;
      printf("  %-8s %-5s lon %.2e (%s) lat %.2e (%s)\n",
             swe_get_planet_name(ipl, (char[64]){0}), im ? "oscu" : "mean",
             w[0], rgPt[wj[0]], w[1], rgPt[wj[1]]);
      for (i = 0; i < 2; i++)
        if (w[i] > lim[i]) {
          printf("    FAIL: %s speed off by more than %.0e\n", i == 0 ? "longitude" : "latitude", lim[i]);
          bad++;
        }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G92 FAIL: %d\n" : "G92 PASS\n", bad);
  return bad ? 1 : 0;
}
