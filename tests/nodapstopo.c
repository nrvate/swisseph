/* nodapstopo.c -- G78: the planets' mean nodes' and apsides' topocentric
 * apparent speeds are the rates of their positions.
 *
 * Upstream defect in observer_accel() (UPSTREAM-BUGS 79), seen here
 * through swe_nod_aps(): the site's acceleration, which the aberration's
 * rate takes, was a central difference of its velocity over +-2^-11 day,
 * (omega h)^2 / 6 = 1.6e-6 of it low, a / c of the diurnal aberration's
 * rate. So the topocentric points' speeds were off where the geocentric
 * ones are not: longitude 9.6e-10 (Mercury) to 1.2e-9 (Neptune) deg/day on
 * the equator, latitude 3.7-4.0e-10 for every planet. The site's velocity
 * (1.2-2.4e-11 AU/day off, 2.9e-9 deg/day on Mercury's points) was found
 * here too and is entry 73's fix (G72); this gate holds both.
 *
 * The check: the longitude, latitude and distance speed of the ascending
 * node, descending node, perihelion and aphelion, mean, ecliptic, of
 * Mercury..Neptune, SEFLG_TOPOCTR with default flags, sites at 0, 47 and
 * 70 N, at seven instants -3000..3000 (G69's), Swiss files. The reference
 * is not a five-point difference: the site turns 6.3 rad/day, and at
 * h = 2^-6 day the five-point's truncation, (omega h)^4 / 30 of the
 * diurnal parallax's rate, is 5e-8 deg/day on Mercury and 5.5e-10 AU/day
 * on every distance rate (what the bug was first measured as), while at
 * h <= 2^-8 the site's place read at a rounded UT (4.7e-10 day at a JD of
 * 2.4e6, 6e-14 AU) makes 1e-9 of noise. So the reference is the slope of
 * a least-squares polynomial of degree 8 through 257 positions 2^-11 day
 * apart, binary-fraction instants, which agrees to 3e-11 deg/day with the
 * same at degree 10 and 2^-12 day. Limits 6e-10 deg/day (longitude),
 * 2e-10 (latitude), 1e-10 AU/day (distance). Measured: longitude
 * <= 3.5e-10 (Neptune, its geocentric floor), latitude <= 1.1e-10
 * (Mercury at 2000 on the equator, the site's turning at the rounded UT),
 * distance <= 6.4e-11 (Neptune's node at -1, geocentric the same). Before
 * the fix (entry 73's site velocity in place): longitude 1.2e-9, latitude
 * 4.0e-10; the tree before fails 27 of the 63 checks (three per planet
 * and site), the upstream build all 63.
 * Run as
 *   nodapstopo EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define M 128			/* samples each side */
#define NDEG 8			/* the polynomial's degree */

static const char *rgPt[4] = {"asc node", "dsc node", "perihelion", "aphelion"};

/* the slope at s = 0 of the least-squares polynomial through v[k],
 * k = -M..M, at s = k / M, over the step h (normal equations; the
 * monomials on [-1, 1] are well enough conditioned at degree 8) */
static double lsq_slope(const double *v, double h)
{
  double a[NDEG + 1][NDEG + 2], c[NDEG + 1];
  int n = NDEG + 1, i, j, k, r;
  for (i = 0; i < n; i++)
    for (j = 0; j <= n; j++)
      a[i][j] = 0;
  for (k = -M; k <= M; k++) {
    double s = (double) k / M, p[NDEG + 1];
    p[0] = 1;
    for (i = 1; i < n; i++)
      p[i] = p[i - 1] * s;
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++)
        a[i][j] += p[i] * p[j];
      a[i][n] += p[i] * v[k + M];
    }
  }
  for (i = 0; i < n; i++)
    for (r = i + 1; r < n; r++) {
      double f = a[r][i] / a[i][i];
      for (j = i; j <= n; j++)
        a[r][j] -= f * a[i][j];
    }
  for (i = n - 1; i >= 0; i--) {
    double s = a[i][n];
    for (j = i + 1; j < n; j++)
      s -= a[i][j] * c[j];
    c[i] = s / a[i][i];
  }
  return c[1] / (M * h);
}

static int nodaps(swe_ctx *c, double jd, int ipl, int32 fl, double x[4][6])
{
  char serr[256];
  return swe_nod_aps_r(c, jd, ipl, fl, SE_NODBIT_MEAN, x[0], x[1], x[2], x[3], serr);
}

int main(int argc, char **argv)
{
  static const double rgLat[3] = {0, 47.37, 70};
  static double d[2 * M + 1][4][6];
  const double lim[3] = {6e-10, 2e-10, 1e-10};
  const double H = 1.0 / 2048;
  int ipl, is, y, j, k, i, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: nodapstopo EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G78: planets' mean nodes' and apsides' topocentric speeds are the rates of their positions\n");
  for (is = 0; is < 3; is++) {
    swe_set_topo_r(c, 8.55, rgLat[is], 400);
    for (ipl = SE_MERCURY; ipl <= SE_NEPTUNE; ipl++) {
      int32 fl = SEFLG_SWIEPH | SEFLG_TOPOCTR;
      double w[3] = {0, 0, 0};
      int wj[3] = {0, 0, 0}, ok = 1;
      if (ipl == SE_MOON) continue;
      for (y = 0; y < 7; y++) {
        double jd = floor((2451545.0 + (-3000 - 2000 + y * 1000.0) * 365.25 + 100.37) * 256) / 256;
        double x[4][6], v[2 * M + 1];
        if (nodaps(c, jd, ipl, fl | SEFLG_SPEED, x) < 0) {
          printf("  planet %d lat %.0f ERROR at %.1f\n", ipl, rgLat[is], jd);
          bad++; ok = 0;
          break;
        }
        for (k = -M; k <= M; k++)
          nodaps(c, jd + k * H, ipl, fl, d[k + M]);
        for (j = 0; j < 4; j++) {
          for (i = 0; i < 3; i++) {
            double e;
            for (k = 0; k <= 2 * M; k++)
              v[k] = i == 0 ? swe_difdeg2n(d[k][j][0], d[M][j][0]) : d[k][j][i] - d[M][j][i];
            e = fabs(x[j][3 + i] - lsq_slope(v, H));
            if (e > w[i]) { w[i] = e; wj[i] = j; }
          }
        }
      }
      if (!ok) continue;
      printf("  %-8s %2.0fN lon %.2e (%s) lat %.2e (%s) dist %.2e (%s)\n",
             swe_get_planet_name(ipl, (char[64]){0}), rgLat[is],
             w[0], rgPt[wj[0]], w[1], rgPt[wj[1]], w[2], rgPt[wj[2]]);
      for (i = 0; i < 3; i++)
        if (w[i] > lim[i]) {
          printf("    FAIL: %s speed off by more than %.1e\n",
                 i == 0 ? "longitude" : i == 1 ? "latitude" : "distance", lim[i]);
          bad++;
        }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G78 FAIL: %d\n" : "G78 PASS\n", bad);
  return bad ? 1 : 0;
}
