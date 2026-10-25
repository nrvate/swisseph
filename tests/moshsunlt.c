/* moshsunlt.c -- G101: Moshier's light time sees the Sun move.
 *
 * The defect (UPSTREAM-BUGS 102). Moshier's positions are heliocentric and
 * its light time was solved in that frame, X(t - tau) - O(t) with the Sun
 * held still, where the Swiss files and JPL solve it about the barycentre:
 * the Sun's motion over the light time, v_S tau, was left out, 3.0e-6 deg
 * (v_S / c) on every body without aberration (SEFLG_NOABERR, geocentric
 * and topocentric) and on every heliocentric one: the planets, the
 * asteroids, the Sun (which had no light time at all), the Moon and the
 * heliocentric Earth. With aberration it cancelled against the aberration
 * taken with the heliocentric Earth's velocity, and that path is not
 * touched. The Sun's barycentric velocity and acceleration now come from
 * Moshier's own eight planets.
 *
 * The checks. (1) An asteroid under Moshier is read from the same file as
 * on the Swiss path (its true heliocentric places agree to 1e-14 deg), so
 * its heliocentric light-time place differs only by the Sun's motion:
 * Chiron and Ceres, SEFLG_HELCTR, J2000 ICRS XYZ, 13 dates 1800..2200
 * mid-way between the files' yearly joins of their barycentric Sun
 * (which jumps there by up to 1.2e-8 AU), against the Swiss files within
 * 2e-9 deg (measured 2.5e-10; the files' Sun is the eight-planet
 * barycentre to <= 6.5 mm/s, 1.2e-9 deg of v_S / c; 3.0e-6 before).
 * (2) The astrometric place (SEFLG_NOABERR | SEFLG_NOGDEFL, J2000, ICRS,
 * XYZ) of the Sun, the Moon, Mercury,
 * Jupiter and Chiron, geocentric and topocentric, of the Moon, Mercury,
 * Jupiter and Chiron heliocentric, and of the heliocentric Earth, against
 * X(t - tau) + S(t - tau) - S(t) - O(t) with tau iterated to its fixed
 * point here: X from swe_calc(SEFLG_HELCTR | SEFLG_TRUEPOS) at t - tau, O
 * the body less its true place from the observer, and S(t - tau) - S(t) =
 * -tau v_S + tau^2 a_S / 2 from v_S and a_S formed here from the library's
 * own heliocentric planets (the Earth-Moon barycentre from the Earth and
 * the Moon), 31 dates -2900..+2900 (Chiron 700..2900), within 1e-11 deg
 * (measured <= 3.6e-13), the Moon within 5e-10 (1.1e-10, as before the
 * fix about the heliocentric frame: its own series' floor); 2.6e-6 to
 * 2.9e-6 before. (3) The speeds of the places that moved, against
 * five-point differences of the positions over 2^-7 day,
 * 13 dates -3000..+3000: Mercury and the Sun geocentric without
 * aberration, Mercury and the Earth heliocentric, within 2e-9 deg/day
 * (measured <= 5.2e-11), the Moon without aberration within 2e-8 (9.5e-9,
 * as before the fix; 4.5e-9 with aberration).
 * The build before the fix fails the 2 rows of (1) and the 15 of (2), 17;
 * the upstream build fails all 22 (its speeds by 2.1e-6 to 1.7e-4
 * deg/day, bugs 35 and 53).
 * Run as
 *   moshsunlt EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define CAU (299792458.0 * 86400.0 / 1.49597870700e11)  /* c, AU/day */
#define K2 (1.32712440017987e+20 * 86400.0 * 86400.0 / (1.49597870700e11 * 1.49597870700e11 * 1.49597870700e11))
#define EMRAT 81.30056907419062

static const int32 FC = SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_XYZ;

static double norm(const double *x)
{
  return sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
}

static double ang(const double *a, const double *b)
{
  double d[3];
  int i;
  for (i = 0; i < 3; i++)
    d[i] = a[i] / norm(a) - b[i] / norm(b);
  return norm(d) * 180 / M_PI;
}

/* The Sun's barycentric velocity and acceleration from the library's own
 * Moshier planets, heliocentric, point masses, DE431's mass ratios. */
static void sun_bary(double jd, double *v, double *a)
{
  static const int ipl[8] = {SE_MERCURY, SE_VENUS, SE_EARTH, SE_MARS,
                             SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE};
  static const double rm[8] = {6023600, 408523.719, 328900.5, 3098703.59,
                               1047.348644, 3497.9018, 22902.98, 19412.26};
  char serr[256];
  double x[6], m[6], ms = 1, r;
  int k, i;
  for (i = 0; i < 3; i++)
    v[i] = a[i] = 0;
  for (k = 0; k < 8; k++) {
    swe_calc(jd, ipl[k], FC | SEFLG_HELCTR | SEFLG_TRUEPOS | SEFLG_SPEED, x, serr);
    if (ipl[k] == SE_EARTH) {	/* the Earth-Moon barycentre */
      swe_calc(jd, SE_MOON, FC | SEFLG_TRUEPOS | SEFLG_SPEED, m, serr);
      for (i = 0; i < 6; i++)
        x[i] += m[i] / (EMRAT + 1);
    }
    r = norm(x);
    ms += 1 / rm[k];
    for (i = 0; i < 3; i++) {
      v[i] += x[i + 3] / rm[k];
      a[i] += K2 * x[i] / (r * r * r) / rm[k];
    }
  }
  for (i = 0; i < 3; i++)
    v[i] = -v[i] / ms;
}

/* (1) */
static int ast_helio(int ipl, const char *sz, double lim)
{
  char serr[256];
  int y;
  double w = 0;
  for (y = 0; y < 13; y++) {
    /* mid-way between the Swiss files' yearly segment joins of the
     * barycentric Sun, which jumps by up to 1.2e-8 AU (2000..2300): a
     * light time across one carries it, 5.6e-8 deg on Ceres on 7 Jan 2200 */
    double jd = 2451547.5 + 182.625 + 365.25 * (-200 + y * 400 / 12);
    double xm[6], xs[6], e;
    if (swe_calc(jd, ipl, FC | SEFLG_HELCTR, xm, serr) < 0
        || swe_calc(jd, ipl, (FC & ~SEFLG_MOSEPH) | SEFLG_SWIEPH | SEFLG_HELCTR, xs, serr) < 0) {
      printf("  %-30s ERROR %s\n", sz, serr);
      return 1;
    }
    e = ang(xm, xs);
    if (e > w)
      w = e;
  }
  printf("  %-30s %.2e deg\n", sz, w);
  if (w > lim) {
    printf("    FAIL: off the Swiss files' place by more than %.0e deg\n", lim);
    return 1;
  }
  return 0;
}

/* (2) */
static int lt_fixed_point(int ipl, int32 fo, const char *sz, double lim)
{
  /* Chiron's file begins in 675 */
  double y0 = ipl == SE_CHIRON ? 700 : -2900;
  char serr[256];
  int y, k, i;
  int32 fx = FC | SEFLG_TRUEPOS | SEFLG_HELCTR;
  double w = 0;
  for (y = 0; y < 31; y++) {
    double jd = 2451545.0 + (y0 + y * (2900 - y0) / 30 - 2000) * 365.25 + 0.37 * y;
    double xa[6], x0[6], g0[6], o[3], xt[6], r[3], vs[3], as[3], tau, e;
    if (swe_calc(jd, ipl, FC | fo | SEFLG_NOABERR | SEFLG_NOGDEFL, xa, serr) < 0) {
      printf("  %-30s ERROR %s\n", sz, serr);
      return 1;
    }
    sun_bary(jd, vs, as);
    if (ipl == SE_SUN) {
      for (i = 0; i < 6; i++)
        x0[i] = 0;
    } else {
      swe_calc(jd, ipl, fx, x0, serr);
    }
    swe_calc(jd, ipl, FC | fo | SEFLG_TRUEPOS, g0, serr);
    for (i = 0; i < 3; i++)
      o[i] = x0[i] - g0[i];
    tau = norm(g0) / CAU;
    for (k = 0; k < 8; k++) {
      double t = jd - tau;
      if (ipl == SE_SUN) {
        for (i = 0; i < 6; i++)
          xt[i] = 0;
      } else {
        swe_calc(t, ipl, fx | SEFLG_SPEED, xt, serr);
      }
      for (i = 0; i < 3; i++) {
        xt[i] += ((jd - t) - tau) * xt[i + 3];
        r[i] = xt[i] - tau * vs[i] + 0.5 * tau * tau * as[i] - o[i];
      }
      tau = norm(r) / CAU;
    }
    e = ang(xa, r);
    if (e > w)
      w = e;
  }
  printf("  %-30s %.2e deg\n", sz, w);
  if (w > lim) {
    printf("    FAIL: off the converged light time by more than %.0e deg\n", lim);
    return 1;
  }
  return 0;
}

/* (3) */
static int body_rate(int ipl, int32 fc, const char *sz, double lim)
{
  char serr[256];
  int y, k, i;
  double w[2] = {0, 0}, h = 1.0 / 128;
  for (y = 0; y < 13; y++) {
    double jd = floor((2451545.0 + (-3000 + y * 500 - 2000) * 365.25 + 37.3 * y) * 256) / 256;
    double x[6], d[5][6];
    int32 fl = SEFLG_MOSEPH | fc;
    if (swe_calc(jd, ipl, fl | SEFLG_SPEED, x, serr) < 0) {
      printf("  %-30s ERROR %s\n", sz, serr);
      return 1;
    }
    for (k = 0; k < 5; k++)
      swe_calc(jd + (k - 2) * h, ipl, fl, d[k], serr);
    for (i = 0; i < 2; i++) {
      double v[5], e;
      for (k = 0; k < 5; k++) {
        v[k] = d[k][i];
        if (i == 0)
          v[k] = d[2][0] + swe_difdeg2n(v[k], d[2][0]);
      }
      e = fabs(x[3 + i] - (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h));
      if (e > w[i])
        w[i] = e;
    }
  }
  printf("  %-30s lon %.2e lat %.2e deg/day\n", sz, w[0], w[1]);
  if (w[0] > lim || w[1] > lim) {
    printf("    FAIL: speed off its positions' rate by more than %.0e deg/day\n", lim);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  static const struct { int ipl; const char *sz; } rgb[] = {
    {SE_SUN, "Sun"}, {SE_MOON, "Moon"}, {SE_MERCURY, "Mercury"},
    {SE_JUPITER, "Jupiter"}, {SE_CHIRON, "Chiron"},
  };
  static const struct { int32 fl; const char *sz; } rgo[] = {
    {0, "geo"}, {SEFLG_TOPOCTR, "topo"}, {SEFLG_HELCTR, "helio"},
  };
  char sz[64];
  int ib, io, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: moshsunlt EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  printf("G101: Moshier's light time sees the Sun move\n");
  bad += ast_helio(SE_CHIRON, "Chiron helio vs Swiss files", 2e-9);
  bad += ast_helio(SE_CERES, "Ceres helio vs Swiss files", 2e-9);
  for (ib = 0; ib < 5; ib++)
    for (io = 0; io < 3; io++) {
      if (rgb[ib].ipl == SE_SUN && rgo[io].fl == SEFLG_HELCTR)
        continue;
      snprintf(sz, sizeof sz, "%s %s fixed point", rgb[ib].sz, rgo[io].sz);
      bad += lt_fixed_point(rgb[ib].ipl, rgo[io].fl, sz,
                            rgb[ib].ipl == SE_MOON ? 5e-10 : 1e-11);
    }
  bad += lt_fixed_point(SE_EARTH, SEFLG_HELCTR, "Earth helio fixed point", 1e-11);
  bad += body_rate(SE_MERCURY, SEFLG_NOABERR, "Mercury noaberr speed", 2e-9);
  bad += body_rate(SE_SUN, SEFLG_NOABERR, "Sun noaberr speed", 2e-9);
  bad += body_rate(SE_MERCURY, SEFLG_HELCTR, "Mercury helio speed", 2e-9);
  bad += body_rate(SE_EARTH, SEFLG_HELCTR, "Earth helio speed", 2e-9);
  bad += body_rate(SE_MOON, SEFLG_NOABERR, "Moon noaberr speed", 2e-8);
  printf(bad ? "G101 FAIL: %d\n" : "G101 PASS\n", bad);
  return bad ? 1 : 0;
}
