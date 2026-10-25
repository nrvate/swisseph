/* moshlt.c -- G52: a Moshier body is seen where it was at t - tau, and its
 * apparent speed is the rate of that place.
 *
 * The defect (UPSTREAM-BUGS 53): with SEFLG_MOSEPH, app_pos_etc_plan() and
 * app_pos_etc_moon() moved the body back along its velocity, x(t) - tau v(t),
 * with one pass of the light time for the planets, instead of reading it at
 * t - tau as the files do. The place was off by tau^2 a / 2 and v tau rdot / c
 * (13.0 mas on Mercury, 0.09 on Chiron), a speed kept v(t) where it is
 * v(t - tau) (the Moon's 1.1e-4 deg/day without aberration, 4.6e-5
 * heliocentric, Mercury's heliocentric distance rate 5.6e-6 AU/day), the
 * one-pass light time put 1e-4 of the Earth's acceleration into the
 * aberration rate (5.7e-8 deg/day on topocentric Mercury), and
 * SEFLG_CENTER_BODY's offset was added twice (Jupiter 4.7e-7 AU).
 *
 * The checks, Moshier, 24 dates 1800-2100, J2000 equator, no nutation:
 *   1. the astrometric direction (ICRS, no aberration or deflection) equals
 *      P(t - tau) - O(t) built from this library's own geometric
 *      heliocentric vectors, tau solved here: Mercury, Venus, Mars,
 *      Jupiter, Pluto, Chiron, geocentric and heliocentric, within 0.05
 *      mas (measured <= 0.0088, Mercury heliocentric: the two straight-line
 *      passes that set tau, as on the files, 0.0048 there);
 *   2. the position with SEFLG_SPEED equals the position without it;
 *   3. speeds against five-point differences of the positions over 2^-5
 *      day (2^-7 topocentric) at multiples of 2^-8 day, angles in deg/day:
 *      the Moon geocentric with and without aberration, topocentric with
 *      and without, heliocentric (2.2e-8, 1.3e-8, 4.4e-7, 4.3e-7, 4.7e-8,
 *      the last the second order's truncation over 500 s; the Swiss files
 *      3.6e-8, 1.3e-8, -, 4.3e-7, 5e-10); Mercury and Mars geocentric with
 *      and without aberration and heliocentric (<= 3.6e-8, Mercury
 *      heliocentric, the instant t - tau's rounding as on the files); and
 *      Mercury's and Mars's heliocentric distance rates, AU/day (2.9e-11,
 *      1.1e-11; the files 2.5e-10, 6.9e-11);
 *   4. the aberration's part of Mercury's and Mars's speeds, geocentric and
 *      topocentric: the residual of 3 with aberration less that without,
 *      over 2^-6 day, within 3e-9 deg/day (measured <= 1.4e-9);
 *   5. Jupiter's centre-of-body offset, SEFLG_CENTER_BODY less without it,
 *      equals the one on the Swiss files within 1e-10 AU (7.8e-15).
 * The upstream build fails 27 of the 42: check 1 on all but Jupiter
 * heliocentric and Pluto (Mercury 12.8 mas geocentric), all of 3 (the Moon
 * 1.5e-4 deg/day without aberration, Mercury's distance rate 5.4e-6
 * AU/day), all of 4 (3.1e-5 topocentric, with upstream's aberration rate)
 * and 5 (4.66e-7 AU); ts20 before this fix 21 (4: 6.8e-9 geocentric, 5.7e-8
 * topocentric). Run as
 *   moshlt EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static double Sep(const double *a, const double *b)   /* mas */
{
  double ra = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
  double rb = sqrt(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]);
  double cx = a[1]*b[2] - a[2]*b[1], cy = a[2]*b[0] - a[0]*b[2];
  double cz = a[0]*b[1] - a[1]*b[0];
  double dot = (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]) / (ra * rb);
  return atan2(sqrt(cx*cx + cy*cy + cz*cz) / (ra * rb), dot) * 206264806.247;
}

static double wrap(double d)
{
  while (d > 180) d -= 360;
  while (d < -180) d += 360;
  return d;
}

static double date(int id)
{
  return floor((2378496.5 + id * 4731.07 + 0.137) * 256) / 256;
}

/* worst |speed - five-point difference| over h: the angles (comp 0) or
 * the distance (comp 2) */
static double speed_off(swe_ctx *c, int ipl, int32 fl, int comp, double h)
{
  double w = 0, x[6], f[5][6], r, d;
  char serr[256];
  int id, k, i;
  for (id = 0; id < 24; id++) {
    double jd = date(id);
    if (swe_calc_r(c, jd, ipl, fl | SEFLG_SPEED, x, serr) < 0) {
      printf("  ERROR %s\n", serr);
      return 1e9;
    }
    for (k = 0; k < 5; k++)
      swe_calc_r(c, jd + (k - 2) * h, ipl, fl, f[k], serr);
    for (i = comp; i < (comp ? 3 : 2); i++) {
      if (i < 2)
        r = (wrap(f[0][i] - f[2][i]) - 8 * wrap(f[1][i] - f[2][i])
             + 8 * wrap(f[3][i] - f[2][i]) - wrap(f[4][i] - f[2][i])) / (12 * h);
      else
        r = (f[0][i] - 8 * f[1][i] + 8 * f[3][i] - f[4][i]) / (12 * h);
      d = fabs(x[i + 3] - r);
      if (d > w) w = d;
    }
  }
  return w;
}

/* worst |the aberration's part of the angular speeds' residual|: the
 * residual against the five-point difference over h with aberration less
 * the one without, which leaves the aberration rate's own error */
static double aberr_part_off(swe_ctx *c, int ipl, int32 fl, double h)
{
  double w = 0, x[2][6], f[5][6], r[2];
  char serr[256];
  int id, k, i, a;
  for (id = 0; id < 24; id++) {
    double jd = date(id);
    for (i = 0; i < 2; i++) {
      for (a = 0; a < 2; a++) {
        int32 fa = fl | (a ? SEFLG_NOABERR : 0);
        swe_calc_r(c, jd, ipl, fa | SEFLG_SPEED, x[a], serr);
        for (k = 0; k < 5; k++)
          swe_calc_r(c, jd + (k - 2) * h, ipl, fa, f[k], serr);
        r[a] = x[a][i + 3] - (wrap(f[0][i] - f[2][i]) - 8 * wrap(f[1][i] - f[2][i])
                             + 8 * wrap(f[3][i] - f[2][i]) - wrap(f[4][i] - f[2][i])) / (12 * h);
      }
      if (fabs(r[0] - r[1]) > w) w = fabs(r[0] - r[1]);
    }
  }
  return w;
}

int main(int argc, char **argv)
{
  static const int rgPl[6] = {SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER, SE_PLUTO, SE_CHIRON};
  static const char *rgszPl[6] = {"Mercury", "Venus", "Mars", "Jupiter", "Pluto", "Chiron"};
  const double cl = 173.1446326846693;   /* AU per day */
  const int32 base = SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_NONUT | SEFLG_XYZ;
  const int32 geom = base | SEFLG_HELCTR | SEFLG_TRUEPOS | SEFLG_SPEED;
  const int32 frame = SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_EQUATORIAL;
  /* h 2^-5 day, 2^-7 for the topocentric Moon's diurnal parallax */
  struct { int ipl; int32 fl; int comp; double h; const char *sz; double lim; } rgSp[] = {
    {SE_MOON, 0, 0, 1.0 / 32, "Moon geocentric, deg/day", 1e-7},
    {SE_MOON, SEFLG_NOABERR, 0, 1.0 / 32, "Moon no aberr, deg/day", 1e-7},
    {SE_MOON, SEFLG_TOPOCTR | SEFLG_NOABERR, 0, 1.0 / 128, "Moon topo no aberr, deg/day", 2e-6},
    {SE_MOON, SEFLG_HELCTR, 0, 1.0 / 32, "Moon heliocentric, deg/day", 2e-7},
    {SE_MOON, SEFLG_TOPOCTR, 0, 1.0 / 128, "Moon topocentric, deg/day", 2e-6},
    {SE_MERCURY, 0, 0, 1.0 / 32, "Mercury geocentric, deg/day", 1e-7},
    {SE_MERCURY, SEFLG_NOABERR, 0, 1.0 / 32, "Mercury no aberr, deg/day", 1e-7},
    {SE_MERCURY, SEFLG_HELCTR, 0, 1.0 / 32, "Mercury heliocentric, deg/day", 2e-7},
    {SE_MARS, 0, 0, 1.0 / 32, "Mars geocentric, deg/day", 1e-7},
    {SE_MARS, SEFLG_NOABERR, 0, 1.0 / 32, "Mars no aberr, deg/day", 1e-7},
    {SE_MARS, SEFLG_HELCTR, 0, 1.0 / 32, "Mars heliocentric, deg/day", 1e-7},
    {SE_MERCURY, SEFLG_HELCTR, 2, 1.0 / 32, "Mercury helio dist, AU/day", 1e-8},
    {SE_MARS, SEFLG_HELCTR, 2, 1.0 / 32, "Mars helio dist, AU/day", 1e-8},
  };
  char serr[256];
  int ip, ic, id, it, k, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: moshlt EPHEDIR\n");
    return 2;
  }
  printf("G52: a Moshier body is seen at t - tau, its speed the rate of that place\n");
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_topo_r(c, 8.55, 47.37, 400);
  /* 1 and 2 */
  for (ip = 0; ip < 6; ip++) {
    for (ic = 0; ic < 2; ic++) {
      int32 astr = base | SEFLG_NOABERR | SEFLG_NOGDEFL | (ic ? SEFLG_HELCTR : 0);
      double wLt = 0, wSpeed = 0;
      for (id = 0; id < 24; id++) {
        double jd = date(id) + 0.0371 * id, xs[6], xn[6], xo[6] = {0}, xb[6], d[3] = {0};
        double tau = 0, r, s;
        if (swe_calc_r(c, jd, rgPl[ip], astr | SEFLG_SPEED, xs, serr) < 0
            || swe_calc_r(c, jd, rgPl[ip], astr, xn, serr) < 0
            || (!ic && swe_calc_r(c, jd, SE_EARTH, geom, xo, serr) < 0)) {
          printf("  ERROR %s\n", serr);
          return 1;
        }
        /* Moshier's barycentre is the Sun: geometric frame heliocentric */
        for (it = 0; it < 20; it++) {
          swe_calc_r(c, jd - tau, rgPl[ip], geom, xb, serr);
          for (k = 0; k < 3; k++)
            d[k] = xb[k] - xo[k];
          r = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
          if (fabs(r / cl - tau) < 1e-15) break;
          tau = r / cl;
        }
        s = Sep(xs, d);
        if (s > wLt) wLt = s;
        s = Sep(xs, xn);
        if (s > wSpeed) wSpeed = s;
      }
      printf("  %-8s %-5s from P(t-tau)-O(t) %.4f mas (limit 0.05)   speed flag moves it %.6f mas\n",
             rgszPl[ip], ic ? "helio" : "geo", wLt, wSpeed);
      if (wLt > 0.05) {
        printf("    FAIL: not the place at t - tau\n");
        bad++;
      }
      if (wSpeed > 1e-6) {
        printf("    FAIL: the position depends on SEFLG_SPEED\n");
        bad++;
      }
    }
  }
  /* 3 */
  for (k = 0; k < (int) (sizeof(rgSp) / sizeof(rgSp[0])); k++) {
    double w = speed_off(c, rgSp[k].ipl, frame | rgSp[k].fl, rgSp[k].comp, rgSp[k].h);
    printf("  %-30s worst %.2e (limit %.0e)\n", rgSp[k].sz, w, rgSp[k].lim);
    if (w > rgSp[k].lim) {
      printf("    FAIL: the speed is not the rate of the position\n");
      bad++;
    }
  }
  {
    static const struct { int ipl; int32 fl; const char *sz; } rgAb[4] = {
      {SE_MERCURY, 0, "Mercury geocentric"}, {SE_MERCURY, SEFLG_TOPOCTR, "Mercury topocentric"},
      {SE_MARS, 0, "Mars geocentric"}, {SE_MARS, SEFLG_TOPOCTR, "Mars topocentric"},
    };
    for (k = 0; k < 4; k++) {
      double w = aberr_part_off(c, rgAb[k].ipl, frame | rgAb[k].fl, 1.0 / 64);
      printf("  %-20s aberration's part, worst %.2e deg/day (limit 3e-9)\n", rgAb[k].sz, w);
      if (w > 3e-9) {
        printf("    FAIL: the aberration rate is not the aberration's rate\n");
        bad++;
      }
    }
  }
  swe_ctx_free(c);
  /* 4 */
  {
    const double jd = 2460000.5;
    const int32 fl = SEFLG_J2000 | SEFLG_ICRS | SEFLG_NONUT | SEFLG_XYZ | SEFLG_NOABERR | SEFLG_NOGDEFL;
    double xm[6], xm0[6], xs[6], xs0[6], d = 0;
    c = swe_ctx_new();
    swe_set_ephe_path_r(c, argv[1]);
    if (swe_calc_r(c, jd, SE_JUPITER, fl | SEFLG_MOSEPH | SEFLG_CENTER_BODY, xm, serr) < 0
        || swe_calc_r(c, jd, SE_JUPITER, fl | SEFLG_MOSEPH, xm0, serr) < 0
        || swe_calc_r(c, jd, SE_JUPITER, fl | SEFLG_SWIEPH | SEFLG_CENTER_BODY, xs, serr) < 0
        || swe_calc_r(c, jd, SE_JUPITER, fl | SEFLG_SWIEPH, xs0, serr) < 0) {
      printf("  ERROR %s\n", serr);
      return 1;
    }
    for (k = 0; k < 3; k++)
      d += pow((xm[k] - xm0[k]) - (xs[k] - xs0[k]), 2);
    d = sqrt(d);
    printf("  Jupiter centre of body: Moshier's offset less the files' %.2e AU (limit 1e-10)\n", d);
    if (d > 1e-10) {
      printf("    FAIL: the centre-of-body offset is not the files'\n");
      bad++;
    }
    swe_ctx_free(c);
  }
  printf(bad ? "G52 FAIL: %d\n" : "G52 PASS\n", bad);
  return bad ? 1 : 0;
}
