/* moshprec.c -- G50: the Moshier ephemeris is smooth at the JD's own
 * resolution, and its speeds are the rates of its positions.
 *
 * Two families (UPSTREAM-BUGS 50, 51). The series' arguments were formed
 * as freq * T and reduced afterwards, T = (J - J2000) / 10000 years for
 * the planets, / 100 for the Moon: at -3000 those products reach 2.7e10"
 * and 1.7e9", and their roundings, worth more time than a JD resolves
 * there, made the geometric places rough (Mercury 3.9e-3 mas, the Moon
 * 0.011 mas between consecutive JDs). Now each argument is exact to its
 * last bit. And every Moshier velocity was a difference over 8.64 s (the
 * planets, backward: the speed 4.32 s late) or +-4.32 s (the Moon), the
 * instants rounded at the JD's resolution and the series' rounding
 * divided by 1e-4 day: ~3e-6 of the speed, which the light time (x - tau
 * v) carried into the apparent places (Mercury 0.13 mas between
 * consecutive JDs at -3000), and the osculating elements amplified (the
 * apogee 45" between consecutive JDs, the true node's distance 1e-8 AU,
 * Mars's osculating aphelion 9e-6 AU). Now the planets' velocities are
 * the series' derivative, the Moon's a five-point difference over exact
 * instants 2^-5 day apart, each sample turned with its own date's
 * obliquity; the true node and apogee differences use +-2^-8 day, not
 * the 0.1 day that hid the noise.
 *
 * The check, Moshier, no files needed:
 *  A. at -3000 and 1300, over 64 consecutive representable JDs, the worst
 *     residual from a straight line: Mercury geometric and apparent (lon,
 *     mas), the Moon geometric (lon, mas), the osculating apogee (lon,
 *     mas), the true node's distance (AU), Mars's osculating aphelion
 *     distance from swe_nod_aps (AU). Measured <= 5.5e-7, 6.4e-7, 1.0e-5,
 *     0.044 mas, 1.1e-14 and 1.5e-14 AU; limits 1e-4, 1e-3, 1e-4, 1 mas,
 *     1e-11, 1e-11 AU (upstream at -3000: 1.6e-3, 0.13, 0.011, 1.2e5 mas,
 *     2.9e-8, 9.3e-6 AU).
 *  B. at -2500, 1 and 2000, J2000 geometric (so neither the ecliptic's
 *     turning nor Moshier's straight-line light time is at stake), the
 *     speeds of Mercury, Venus, Mars, Jupiter and the Moon, geocentric,
 *     and of the Earth, heliocentric, against a five-point difference of
 *     the positions over 2^-6 day at a multiple of it: within 1e-7
 *     deg/day (measured <= 5.3e-10; upstream 9.8e-5); the true node's and
 *     the osculating apogee's ecliptic-of-date speeds against the same,
 *     within 1e-7 and 1e-5 deg/day (measured <= 3.1e-8 and 2.1e-6;
 *     upstream 1.2e-4 and 5.3e-2).
 * The upstream build fails all 21. Run as
 *   moshprec EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define NS 64

/* worst residual of v[] from its least-squares line */
static double resid(const double *v)
{
  double sx = 0, sy = 0, sxx = 0, sxy = 0, a, b, m = 0;
  int i;
  for (i = 0; i < NS; i++) {
    sx += i;
    sy += v[i];
    sxx += (double) i * i;
    sxy += i * v[i];
  }
  b = (NS * sxy - sx * sy) / (NS * sxx - sx * sx);
  a = (sy - b * sx) / NS;
  for (i = 0; i < NS; i++)
    if (fabs(v[i] - a - b * i) > m)
      m = fabs(v[i] - a - b * i);
  return m;
}

/* field k of body ipl (or of swe_nod_aps' aphelion, ipl < 0: -ipl) over
 * 64 consecutive JDs; angles in mas from the first, others as they are */
static double rough(double jd, int ipl, int32 fl, int k)
{
  double v[NS], x[6], y[6], z[6], w[6], x0 = 0;
  char serr[256];
  int i;
  for (i = 0; i < NS; i++, jd = nextafter(jd, 1e300)) {
    if (ipl < 0) {
      swe_nod_aps(jd, -ipl, fl, SE_NODBIT_OSCU, y, z, w, x, serr);
    } else {
      swe_calc(jd, ipl, fl, x, serr);
    }
    if (i == 0)
      x0 = x[k];
    if (k == 2)
      v[i] = x[k] - x0;
    else
      v[i] = remainder(x[k] - x0, 360.0) * 3.6e6;
  }
  return resid(v);
}

/* the speed against a five-point difference of positions over 2^-6 day */
static double spdoff(double jd, int ipl, int32 fl)
{
  double x[6], f[5][6], h = 1.0 / 64, off = 0, r, d[4];
  char serr[256];
  int k, j;
  jd = floor(jd / h) * h;
  swe_calc(jd, ipl, fl | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(jd + (k - 2) * h, ipl, fl, f[k], serr);
  for (j = 0; j < 2; j++) {
    d[0] = remainder(f[0][j] - f[2][j], 360.0);
    d[1] = remainder(f[1][j] - f[2][j], 360.0);
    d[2] = remainder(f[3][j] - f[2][j], 360.0);
    d[3] = remainder(f[4][j] - f[2][j], 360.0);
    r = (d[0] - 8 * d[1] + 8 * d[2] - d[3]) / (12 * h);
    if (fabs(x[3 + j] - r) > off)
      off = fabs(x[3 + j] - r);
  }
  return off;
}

int main(int argc, char **argv)
{
  static const struct { const char *n; int ipl; int32 fl; int k; double lim; } ca[] = {
    {"Mercury geometric lon, mas", SE_MERCURY, SEFLG_TRUEPOS, 0, 1e-4},
    {"Mercury apparent lon, mas", SE_MERCURY, SEFLG_SPEED, 0, 1e-3},
    {"Moon geometric lon, mas", SE_MOON, SEFLG_TRUEPOS, 0, 1e-4},
    {"osculating apogee lon, mas", SE_OSCU_APOG, SEFLG_SPEED, 0, 1.0},
    {"true node distance, AU", SE_TRUE_NODE, SEFLG_SPEED, 2, 1e-11},
    {"Mars oscu. aphelion dist, AU", -SE_MARS, SEFLG_SPEED, 2, 1e-11},
  };
  static const int bpl[6] = {SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER, SE_MOON, SE_EARTH};
  static const double yA[2] = {-3000, 1300}, yB[3] = {-2500, 1, 2000};
  const int32 M = SEFLG_MOSEPH;
  const int32 fgeo = M | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS;
  int i, j, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: moshprec EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G50: the Moshier ephemeris is smooth, and its speeds are its rates\n");
  for (j = 0; j < 2; j++) {
    double jd = 2451545.0 + (yA[j] - 2000) * 365.25 + 0.1234567;
    for (i = 0; i < 6; i++) {
      double r = rough(jd, ca[i].ipl, M | ca[i].fl, ca[i].k);
      printf("  A %5.0f %-30s %.2e\n", yA[j], ca[i].n, r);
      if (!(r < ca[i].lim)) {
        printf("    FAIL: rougher than %.0e between consecutive JDs\n", ca[i].lim);
        bad++;
      }
    }
  }
  for (j = 0; j < 3; j++) {
    double jd = 2451545.0 + (yB[j] - 2000) * 365.25 + 0.1234567, w = 0, r;
    for (i = 0; i < 6; i++) {
      r = spdoff(jd, bpl[i], fgeo | (bpl[i] == SE_EARTH ? SEFLG_HELCTR : 0));
      if (r > w)
        w = r;
    }
    printf("  B %5.0f planets, Moon, Earth: speed vs difference %.2e deg/day\n", yB[j], w);
    if (!(w < 1e-7)) {
      printf("    FAIL: a speed is not the rate of its position\n");
      bad++;
    }
    r = spdoff(jd, SE_TRUE_NODE, M | SEFLG_TRUEPOS);
    printf("  B %5.0f true node: %.2e deg/day\n", yB[j], r);
    if (!(r < 1e-7)) {
      printf("    FAIL: the true node's speed is not its rate\n");
      bad++;
    }
    r = spdoff(jd, SE_OSCU_APOG, M | SEFLG_TRUEPOS);
    printf("  B %5.0f osculating apogee: %.2e deg/day\n", yB[j], r);
    if (!(r < 1e-5)) {
      printf("    FAIL: the osculating apogee's speed is not its rate\n");
      bad++;
    }
  }
  swe_close();
  printf(bad ? "G50 FAIL: %d\n" : "G50 PASS\n", bad);
  return bad ? 1 : 0;
}
