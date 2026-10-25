/* ltconv.c -- G77: the planets' light time is solved to its fixed point,
 * and the heliocentric and barycentric Earth's speed carries its light
 * time's rate.
 *
 * Two defects (UPSTREAM-BUGS 78). app_pos_etc_plan() solved
 * tau = |X(t - tau) - O(t)| / c along a straight line, X(t) - tau v, in two
 * passes and never against the body read at t - tau, which is off that
 * line by its curvature over the light time: up to 7.8e-10 deg on Mercury,
 * 1.1e-10 on Venus and 3.1e-11 on Mars, on the Swiss files, JPL and
 * Moshier alike. And app_pos_etc_sun() gave the heliocentric and
 * barycentric Earth no light-time rate, the planets' bug 35: 1.7e-6
 * deg/day on its longitude speed, the same at every step.
 *
 * The checks. (1) The Earth, SEFLG_HELCTR and SEFLG_BARYCTR, on the Swiss
 * files, de200.eph and Moshier (heliocentric only; it has no barycentre),
 * thirteen dates, binary-fraction instants: the longitude and latitude
 * speeds within 2e-9 deg/day of five-point differences of the positions
 * over 2^-6..2^-9 day. Measured <= 6.8e-10 (the Swiss files' heliocentric
 * Earth, whose Sun is read at t - tau) and <= 3.6e-10 elsewhere. The
 * geocentric Sun without aberration, through the same code, had the same
 * omission, 6.8e-10 and 9.1e-10 deg/day on the files and de200: within
 * 5e-10 (measured 3.5e-10, its true positions' own floor).
 * (2) Mercury, Venus and Mars, geocentric, topocentric and heliocentric,
 * the astrometric position (SEFLG_NOABERR | SEFLG_NOGDEFL, J2000, ICRS,
 * XYZ) against X(t - tau) - O(t) with tau iterated to its fixed point
 * here, X from swe_calc(SEFLG_BARYCTR | SEFLG_TRUEPOS) at t - tau (moved by
 * the rounding of t - tau along its velocity), O the true body less its
 * true position from the observer; Moshier's positions are heliocentric,
 * so there X is SEFLG_HELCTR. 61 dates -3000..+3000 (de200: 1601..2168).
 * Within 1e-11 deg (measured <= 5.2e-13). The heliocentric Earth the same
 * way on the Swiss files and de200, within 1e-11 (measured 3.8e-14): the
 * Swiss files subtracted the Sun read at t - tau, 3.0e-6 deg off (part 2).
 * The build before the fix fails 34 of the 36 rows (all but the two
 * heliocentric Earth positions, which part 2 adds): the Earth's by 1.6e-6 to
 * 1.8e-6 deg/day, the Sun's by 6.8e-10 and 9.1e-10, the planets by 1.8e-11
 * (Mars heliocentric) to 7.2e-10 deg (Mercury geocentric). The build before
 * part 2 fails the Swiss heliocentric Earth's position by 3.0e-6 deg. The
 * upstream build fails all 36.
 * Run as
 *   ltconv EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define CAU (299792458.0 * 86400.0 / 1.49597870700e11)  /* c, AU/day */

static double norm(const double *x)
{
  return sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
}

static int body_rate(int ipl, int32 ephe, int32 fc, const char *sz, double lim)
{
  char serr[256];
  int y, s, k, i;
  double w[2] = {0, 0};
  for (y = 0; y < 13; y++) {
    double yr = ephe == SEFLG_JPLEPH ? 1610 + y * 45 : -3000 + y * 500;
    double jd = floor((2451545.0 + (yr - 2000) * 365.25 + 37.3 * y) * 256) / 256;
    double x[6], d[5][6];
    if (swe_calc(jd, ipl, ephe | fc | SEFLG_SPEED, x, serr) < 0) {
      printf("  %-24s ERROR %s\n", sz, serr);
      return 1;
    }
    for (s = 6; s <= 9; s++) {
      double h = ldexp(1, -s);
      for (k = 0; k < 5; k++)
        swe_calc(jd + (k - 2) * h, ipl, ephe | fc, d[k], serr);
      for (i = 0; i < 2; i++) {
        double v[5], r, e;
        for (k = 0; k < 5; k++) {
          v[k] = d[k][i];
          if (i == 0)
            v[k] = d[2][0] + swe_difdeg2n(v[k], d[2][0]);
        }
        r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
        e = fabs(x[3 + i] - r);
        if (e > w[i])
          w[i] = e;
      }
    }
  }
  printf("  %-24s lon %.2e lat %.2e deg/day\n", sz, w[0], w[1]);
  if (w[0] > lim || w[1] > lim) {
    printf("    FAIL: speed off its positions' rate by more than %.0e deg/day\n", lim);
    return 1;
  }
  return 0;
}

static int lt_fixed_point(int32 ephe, int ipl, int32 fo, const char *sz, double lim)
{
  char serr[256];
  int y, k, i;
  int32 fc = ephe | SEFLG_J2000 | SEFLG_ICRS | SEFLG_XYZ;
  int32 fx = fc | SEFLG_TRUEPOS | (ephe == SEFLG_MOSEPH ? SEFLG_HELCTR : SEFLG_BARYCTR);
  double w = 0;
  for (y = 0; y < 61; y++) {
    double yr = ephe == SEFLG_JPLEPH ? 1601 + y * (2168.0 - 1601) / 60 : -3000 + y * 100;
    double jd = 2451545.0 + (yr - 2000) * 365.25 + 0.37 * y;
    double xa[6], x0[6], g0[6], o[3], xt[6], r[3], tau, e;
    if (swe_calc(jd, ipl, fc | fo | SEFLG_NOABERR | SEFLG_NOGDEFL, xa, serr) < 0) {
      printf("  %-7s %-20s ERROR %s\n", sz, "", serr);
      return 1;
    }
    swe_calc(jd, ipl, fx, x0, serr);
    swe_calc(jd, ipl, fc | fo | SEFLG_TRUEPOS, g0, serr);
    for (i = 0; i < 3; i++)
      o[i] = x0[i] - g0[i];
    tau = norm(g0) / CAU;
    for (k = 0; k < 8; k++) {
      double t = jd - tau;
      swe_calc(t, ipl, fx | SEFLG_SPEED, xt, serr);
      for (i = 0; i < 3; i++) {
        xt[i] += ((jd - t) - tau) * xt[i + 3];
        r[i] = xt[i] - o[i];
      }
      tau = norm(r) / CAU;
    }
    for (i = 0; i < 3; i++)
      r[i] = xa[i] - r[i];
    e = norm(r) / norm(xa) * 180 / M_PI;
    if (e > w)
      w = e;
  }
  printf("  %-26s %.2e deg\n", sz, w);
  if (w > lim) {
    printf("    FAIL: off the converged light time by more than %.0e deg\n", lim);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  static const struct { int32 fl; const char *sz; } rge[] = {
    {SEFLG_SWIEPH, "swieph"}, {SEFLG_JPLEPH, "de200"}, {SEFLG_MOSEPH, "moshier"},
  };
  static const struct { int ipl; const char *sz; } rgb[] = {
    {SE_MERCURY, "Mercury"}, {SE_VENUS, "Venus"}, {SE_MARS, "Mars"},
  };
  static const struct { int32 fl; const char *sz; } rgo[] = {
    {0, "geo"}, {SEFLG_TOPOCTR, "topo"}, {SEFLG_HELCTR, "helio"},
  };
  char sz[64];
  int ie, ib, io, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: ltconv EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_jpl_file("de200.eph");
  swe_set_topo(8.55, 47.37, 400);
  printf("G77: converged planetary light time, the Earth's light-time rate\n");
  for (ie = 0; ie < 3; ie++) {
    snprintf(sz, sizeof sz, "Earth %s helio", rge[ie].sz);
    bad += body_rate(SE_EARTH, rge[ie].fl, SEFLG_HELCTR, sz, 2e-9);
    if (rge[ie].fl != SEFLG_MOSEPH) {
      snprintf(sz, sizeof sz, "Earth %s bary", rge[ie].sz);
      bad += body_rate(SE_EARTH, rge[ie].fl, SEFLG_BARYCTR, sz, 2e-9);
      snprintf(sz, sizeof sz, "Sun %s noaberr", rge[ie].sz);
      bad += body_rate(SE_SUN, rge[ie].fl, SEFLG_NOABERR, sz, 5e-10);
    }
  }
  for (ie = 0; ie < 2; ie++) {
    snprintf(sz, sizeof sz, "Earth %s helio", rge[ie].sz);
    bad += lt_fixed_point(rge[ie].fl, SE_EARTH, SEFLG_HELCTR, sz, 1e-11);
  }
  for (ie = 0; ie < 3; ie++)
    for (ib = 0; ib < 3; ib++)
      for (io = 0; io < 3; io++) {
        snprintf(sz, sizeof sz, "%s %s %s", rgb[ib].sz, rge[ie].sz, rgo[io].sz);
        bad += lt_fixed_point(rge[ie].fl, rgb[ib].ipl, rgo[io].fl, sz, 1e-11);
      }
  printf(bad ? "G77 FAIL: %d\n" : "G77 PASS\n", bad);
  return bad ? 1 : 0;
}
