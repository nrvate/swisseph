/* nodapsaberr.c -- G69: the planets' mean nodes' and apsides' apparent
 * speeds are the rates of their apparent positions.
 *
 * Upstream defect in swe_nod_aps() (UPSTREAM-BUGS 70): with aberration (the
 * default flags) the points' speeds took the aberration's rate as
 * swi_aberr_light()'s, the point moved with the observer's velocity held,
 * plus tau a, the observer's acceleration across the line of sight over
 * the light time -- first order in v/c, a's jerk without the Moon's part
 * (the save area at tjd - dt), and, seen from the Sun, the raw
 * V(t) - V(t - dt) with its radial part. Off the positions' rate: 5.7e-9
 * deg/day on Mars's points' geocentric longitude, 1.6e-8 on Neptune's,
 * 1.3e-9 on their latitude; heliocentric distance 1.3e-9 AU/day for
 * Uranus, 2.1e-9 for Neptune. Under SEFLG_TRUEPOS | SEFLG_NOGDEFL (G63) or
 * SEFLG_NOABERR the same points agree.
 *
 * The check: the longitude, latitude and distance speed of the ascending
 * node, descending node, perihelion and aphelion, mean, ecliptic, of
 * Mercury..Neptune, geocentric and heliocentric, default flags, at seven
 * instants -3000..3000, against a five-point difference of the library's
 * own positions (h = 2^-6 day) at binary-fraction instants. Swiss files.
 * Limits 1e-9 deg/day (longitude), 1e-10 (latitude), 5e-11 AU/day
 * (distance). Measured: Mercury geocentric longitude 5.3e-10 at -3000 (the
 * floor G63 sees without aberration), latitude <= 4.4e-11, distance
 * <= 2.2e-11 (Neptune). Before the fix: longitude 5.2e-9 (Mercury) to
 * 1.6e-8 (Neptune), latitude 1.3e-9, distance 2.1e-9. The upstream build
 * fails (and is off by 1e-2 deg/day and more on other counts).
 * Run as
 *   nodapsaberr EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static const char *rgPt[4] = {"asc node", "dsc node", "perihelion", "aphelion"};

static int nodaps(swe_ctx *c, double jd, int ipl, int32 fl, double x[4][6])
{
  char serr[256];
  return swe_nod_aps_r(c, jd, ipl, fl, SE_NODBIT_MEAN, x[0], x[1], x[2], x[3], serr);
}

int main(int argc, char **argv)
{
  const double lim[3] = {1e-9, 1e-10, 5e-11};
  const double H = 1.0 / 64;
  int ipl, hel, y, j, k, i, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: nodapsaberr EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G69: planets' mean nodes' and apsides' apparent speeds are the rates of their positions\n");
  for (ipl = SE_MERCURY; ipl <= SE_NEPTUNE; ipl++) {
    if (ipl == SE_MOON) continue;
    for (hel = 0; hel < 2; hel++) {
      int32 fl = SEFLG_SWIEPH | (hel ? SEFLG_HELCTR : 0);
      double w[3] = {0, 0, 0};
      int wj[3] = {0, 0, 0}, ok = 1;
      for (y = 0; y < 7; y++) {
        double jd = floor((2451545.0 + (-3000 - 2000 + y * 1000.0) * 365.25 + 100.37) * 256) / 256;
        double x[4][6], d[5][4][6];
        if (nodaps(c, jd, ipl, fl | SEFLG_SPEED, x) < 0) {
          printf("  planet %d %s ERROR at %.1f\n", ipl, hel ? "helioc" : "geoc", jd);
          bad++; ok = 0;
          break;
        }
        for (k = 0; k < 5; k++)
          nodaps(c, jd + (k - 2) * H, ipl, fl, d[k]);
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
      if (!ok) continue;
      printf("  %-9s %-6s lon %.2e (%s) lat %.2e (%s) dist %.2e (%s)\n",
             swe_get_planet_name(ipl, (char[64]){0}), hel ? "helioc" : "geoc",
             w[0], rgPt[wj[0]], w[1], rgPt[wj[1]], w[2], rgPt[wj[2]]);
      for (i = 0; i < 3; i++)
        if (w[i] > lim[i]) {
          printf("    FAIL: %s speed off by more than %.0e\n",
                 i == 0 ? "longitude" : i == 1 ? "latitude" : "distance", lim[i]);
          bad++;
        }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G69 FAIL: %d\n" : "G69 PASS\n", bad);
  return bad ? 1 : 0;
}
