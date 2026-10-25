/* nodaposcspeed.c -- G79: the osculating nodes' and apsides' speeds are the
 * rates of their positions.
 *
 * Upstream defect in swe_nod_aps() (UPSTREAM-BUGS 80): the osculating
 * points (SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR) take their speeds as a
 * central difference of the points at tjd -+ dt, dt = 1e-3 day per AU of
 * the body's distance (7e-4 day for Venus, 1e-3 for the Earth's orbit).
 * The eccentricity in those points was sqrt(1 - p / a), where 1 - p / a
 * is e^2 less a few ulps, so e carried 1e-16 / e^2 of itself and the true
 * anomaly through it 1e-11 rad on Venus's orbit (e = 0.0068): its
 * perihelion jitters by 5e-10 deg rms from instant to instant, and
 * differenced over 1.4e-3 day that is 6e-7 deg/day of speed. And the
 * outer instants, tjd - dt and tjd - dt + 2 dt, were rounded, their
 * spacing off 2 dt by up to a JD's ulp: 2.3e-6 of the speed for the
 * Moon's points at dt = 1e-4 (1.9e-6 deg/day), less for the planets.
 *
 * The check: the longitude, latitude and distance speed of the ascending
 * node, descending node, perihelion and aphelion (the Sun's perigee and
 * apogee only), osculating and barycentric-osculating, ecliptic, of the
 * Sun and Mercury..Neptune, geocentric and heliocentric, default flags,
 * at seven instants -3000..3000, against a five-point difference of the
 * library's own positions (h = 2^-6 day) at binary-fraction instants.
 * Swiss files. Limits 1.5e-9 deg/day (longitude), 1e-10 (latitude), 1e-10
 * AU/day (distance). Measured: longitude <= 6.5e-10 (Neptune geocentric),
 * latitude <= 2.1e-11, distance <= 2.6e-11; the reference agrees with
 * itself at 2^-4 and 2^-5 day. Before the fix: Venus 2.0e-7 (geocentric)
 * and 5.6e-7 (heliocentric) in longitude, 2.8e-8 in latitude, 6.1e-9
 * AU/day in distance; the Sun's perigee 6.0e-8 deg/day; Neptune 1.1e-7
 * (its noise, through the five-point reference too). The tree before
 * fails 51 of the 96 checks (three per body, method and centre), the
 * upstream build all 96.
 * Run as
 *   nodaposcspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static const char *rgPt[4] = {"asc node", "dsc node", "perihelion", "aphelion"};

static int nodaps(swe_ctx *c, double jd, int ipl, int32 fl, int32 meth, double x[4][6])
{
  char serr[256];
  return swe_nod_aps_r(c, jd, ipl, fl, meth, x[0], x[1], x[2], x[3], serr);
}

int main(int argc, char **argv)
{
  static const int rgPl[8] = {SE_SUN, SE_MERCURY, SE_VENUS, SE_MARS,
                              SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE};
  const double lim[3] = {1.5e-9, 1e-10, 1e-10};
  const double H = 1.0 / 64;
  int ip, hel, im, y, j, k, i, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: nodaposcspeed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G79: osculating nodes' and apsides' speeds are the rates of their positions\n");
  for (ip = 0; ip < 8; ip++) {
    int ipl = rgPl[ip];
    for (im = 0; im < 2; im++) {
      int32 meth = im ? SE_NODBIT_OSCU_BAR : SE_NODBIT_OSCU;
      for (hel = 0; hel < 2; hel++) {
        int32 fl = SEFLG_SWIEPH | (hel ? SEFLG_HELCTR : 0);
        double w[3] = {0, 0, 0};
        int wj[3] = {0, 0, 0}, ok = 1;
        for (y = 0; y < 7; y++) {
          double jd = floor((2451545.0 + (-3000 - 2000 + y * 1000.0) * 365.25 + 100.37) * 256) / 256;
          double x[4][6], d[5][4][6];
          if (nodaps(c, jd, ipl, fl | SEFLG_SPEED, meth, x) < 0) {
            printf("  planet %d %s ERROR at %.1f\n", ipl, hel ? "helioc" : "geoc", jd);
            bad++; ok = 0;
            break;
          }
          for (k = 0; k < 5; k++)
            nodaps(c, jd + (k - 2) * H, ipl, fl, meth, d[k]);
          for (j = ipl == SE_SUN ? 2 : 0; j < 4; j++) {
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
        printf("  %-8s %-4s %-6s lon %.2e (%s) lat %.2e (%s) dist %.2e (%s)\n",
               swe_get_planet_name(ipl, (char[64]){0}), im ? "bar" : "osc",
               hel ? "helioc" : "geoc",
               w[0], rgPt[wj[0]], w[1], rgPt[wj[1]], w[2], rgPt[wj[2]]);
        for (i = 0; i < 3; i++)
          if (w[i] > lim[i]) {
            printf("    FAIL: %s speed off by more than %.1e\n",
                   i == 0 ? "longitude" : i == 1 ? "latitude" : "distance", lim[i]);
            bad++;
          }
      }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G79 FAIL: %d\n" : "G79 PASS\n", bad);
  return bad ? 1 : 0;
}
