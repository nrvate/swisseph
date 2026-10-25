/* sidera.c -- G39: the long-term sidereal time is the Earth's rotation angle
 * plus the precession in right ascension, so it does not follow delta t.
 *
 * Outside 1850-2050 the default sidereal time (SEMOD_SIDT_LONGTERM) came
 * from sidtime_long_term(): the hour angle of the mean Sun plus the mean
 * Earth's longitude, evaluated at TT for a UT instant. That ties it to delta
 * t at the Sun's mean motion, 0.041" for every second, where sidereal time
 * at a UT1 is ERA(UT1) plus the equinox's accumulated precession in right
 * ascension, which moves with TT only at the precession rate, 0.00015" per
 * 100 s. Against the IERS 2010 expression the old default stood +37" at
 * 1000, +328" at year 0, +2382" at -3000. It is now ERA(UT1) plus that
 * precession, integrated from the context's own precession matrices
 * (swephlib.c, sidt_pra()).
 *
 * The checks, Moshier, no files:
 *   1. raising delta t by 100 s at a fixed UT moves sidereal time by less
 *      than 0.001", at nine epochs from -50000 to +50000 (the last two are
 *      past the table and integrated from its end);
 *   2. on the IAU 2006 precession matrices it reproduces the IERS 2010
 *      expression (the IAU 2006 polynomial) within 0.2 mas, 1500-2500;
 *   3. it is continuous, within 0.01" (what a JD resolves there), where
 *      the table ends (+-400 centuries from J2000) and the integral takes
 *      over;
 *   4. over 64 consecutive representable JDs at -999, -13000 and +12000 it
 *      lies on the straight line through the first and last within 0.01
 *      mas: the rotation angle is formed from the fraction of the day, not
 *      as 1.0027 * (JD - J2000), whose rounding scattered it by 0.15 mas
 *      there, differently wherever the multiply-add fuses;
 *   5. (UPSTREAM-BUGS 235) between 400 and 2000 centuries, where the
 *      precession in right ascension is the integral of its rate, the
 *      default sidereal time less the IERS 2010 one (delta t 0, the
 *      rotation angle cancelling) lies on the straight line through 64
 *      consecutive JDs within 1e-11 deg, at -+600, 605, 1200, 1205, 1800
 *      and 1805 centuries, on and between the integral's panel ends. The
 *      rate was a central difference of the matrices over -+50 days,
 *      carrying their rounding (-+3e-7"/cy there), which the partial
 *      panel's nodes, moving with the date, turned into 6.4e-10 .. 1.9e-8
 *      deg of jitter; now 1.8e-12;
 *   6. (UPSTREAM-BUGS 235, the fork's build only) the table's rate at its
 *      nodes, -+20, 100 and 300 centuries on Vondrak 2011 and -+20 on IAU
 *      2006 (the long-term sidereal time less the IERS 2010 one, the IAU
 *      2006 polynomial and SIDT_PRA_NUTC put back, its one-sided cubic
 *      derivative over quarter centuries), matches the precession's own
 *      rate (a five-point difference of swi_precess() over -+256 days)
 *      within 1e-7"/cy: the central difference's sin(w h) / h left it
 *      6.7e-7 .. 9.3e-7"/cy low; now within 2.2e-8.
 * The upstream build fails 1 by 4.1", 2 by up to 19" (at 2500) and 5 at all
 * 12 dates, 7.2e-7 .. 4.1e-6 deg; ts20 before bug 235's fix failed 5 and 6
 * at all 20. Run as
 *   sidera
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"
#ifndef SWE_UPSTREAM_COMPAT
#include "sweph.h"
#include "swephlib.h"
#endif

static double wrap(double d)   /* hours, to (-12, 12] */
{
  while (d > 12) d -= 24;
  while (d <= -12) d += 24;
  return d;
}

/* check 5: the long-term sidereal time less the IERS 2010 one (the
 * rotation angle and the IERS terms cancel) over 64 consecutive JDs from
 * 32 before jdc, worst off the straight line through the first and last,
 * degrees */
static double off_line(swe_ctx *c, swe_ctx *ci, double jdc)
{
  double jd = jdc, v[65], w = 0;
  int j;
  for (j = 0; j < 32; j++)
    jd = nextafter(jd, -1e13);
  for (j = 0; j <= 64; j++) {
    v[j] = wrap(swe_sidtime0_r(c, jd, 23.4, 0) - swe_sidtime0_r(ci, jd, 23.4, 0));
    jd = nextafter(jd, 1e13);
  }
  for (j = 1; j < 64; j++)
    if (fabs(v[j] - v[0] - (v[64] - v[0]) * j / 64) * 15 > w)
      w = fabs(v[j] - v[0] - (v[64] - v[0]) * j / 64) * 15;
  return w;
}

#ifndef SWE_UPSTREAM_COMPAT
/* check 6: the precession's own rate, -(dM/dt M^T)[1][0] for M = J2000 ->
 * mean of date, "/cy, by a central five-point difference over -+ hh and
 * -+ 2 hh days of swi_precess() */
static double pra_rate(swe_ctx *c, double t, double hh)
{
  static const double o5[5] = { 1.0 / 12, -8.0 / 12, 0, 8.0 / 12, -1.0 / 12 };
  double m[5][3][3], w = 0;
  int i, j, k;
  for (j = 0; j < 5; j++)
    for (k = 0; k < 3; k++) {
      double x[6] = { 0, 0, 0, 0, 0, 0 };
      x[k] = 1;
      swi_precess(c, x, t + (j - 2) * hh, 0, J2000_TO_J);
      for (i = 0; i < 3; i++)
        m[j][i][k] = x[i];
    }
  for (k = 0; k < 3; k++) {
    double d = 0;
    for (j = 0; j < 5; j++)
      d += o5[j] * m[j][1][k];
    w += d / hh * m[2][0][k];
  }
  return w * RADTODEG * 3600.0 * 36525.0;
}
#endif

int main(void)
{
  static const double rgYear[9] = {-50000, -13000, -3000, 0, 1000, 1700, 2500,
                                   12000, 50000};
  static const double rgYear2[6] = {1500, 1700, 1840, 2060, 2300, 2500};
  int i, bad = 0;
  swe_ctx *ca = swe_ctx_new(), *cb = swe_ctx_new();

  printf("G39: the long-term sidereal time is rotation plus precession, not delta t\n");
  for (i = 0; i < 9; i++) {
    double jd = 2451545.0 + (rgYear[i] - 2000) * 365.25, dt, s0, s1, d;
    swe_ctx *c = swe_ctx_new();
    dt = swe_deltat_ex_r(c, jd, -1, NULL);   /* as the library reads it */
    s0 = swe_sidtime0_r(c, jd, 23.4, 0);
    swe_set_delta_t_userdef_r(c, dt + 100.0 / 86400.0);
    s1 = swe_sidtime0_r(c, jd, 23.4, 0);
    d = wrap(s1 - s0) * 54000;
    printf("  %7.0f  delta t + 100 s moves it %+.6f\"\n", rgYear[i], d);
    if (fabs(d) > 1e-3) {
      printf("    FAIL: sidereal time follows delta t\n");
      bad++;
    }
    swe_ctx_free(c);
  }
  swe_set_astro_models_r(ca, "0,8,8,0,0,0,0,4", 0);   /* IAU 2006 precession, long term */
  swe_set_astro_models_r(cb, "0,8,8,0,0,0,0,3", 0);   /* IAU 2006 precession, IERS 2010 */
  for (i = 0; i < 6; i++) {
    double jd = 2451545.0 + (rgYear2[i] - 2000) * 365.25;
    double d = wrap(swe_sidtime0_r(ca, jd, 23.4, 0) - swe_sidtime0_r(cb, jd, 23.4, 0)) * 54000;
    printf("  %7.0f  IAU 2006 precession: long term - IERS 2010 %+.6f\"\n", rgYear2[i], d);
    if (fabs(d) > 2e-4) {
      printf("    FAIL: the integral does not reproduce the IAU 2006 polynomial\n");
      bad++;
    }
  }
  /* on the default precession, Vondrak 2011, the one valid there; the slope
   * is taken from a pair of seconds on each side, so the drift of the
   * precession far from J2000 does not count as a step. Within 0.01": a JD
   * 400 centuries out resolves 3.7e-9 day, 4.8 mas of rotation, and this
   * estimate reads +-4.6 mas anywhere past 150 centuries; a wrong node or
   * sign at the seam would be arcseconds. */
  for (i = 0; i < 2; i++) {
    double jd = 2451545.0 + (i ? 400 : -400) * 36525.0, t[4], v[4], step;
    swe_ctx *c = swe_ctx_new();
    int j;
    for (j = 0; j < 4; j++) {
      t[j] = jd + (2 * j - 3) / 86400.0;   /* -3, -1, +1, +3 s */
      v[j] = swe_sidtime_r(c, t[j]);
    }
    step = (wrap(v[2] - v[1]) - (wrap(v[1] - v[0]) / (t[1] - t[0])
            + wrap(v[3] - v[2]) / (t[3] - t[2])) / 2 * (t[2] - t[1])) * 54000;
    swe_ctx_free(c);
    printf("  %s400 cy   step where the table ends %+.6f\"\n", i ? "+" : "-", step);
    if (fabs(step) > 1e-2) {
      printf("    FAIL: sidereal time steps at the end of the table\n");
      bad++;
    }
  }
  /* sidereal time through 0 h is a straight line over 64 ulps of the JD;
   * nutation 0, so the equation of the equinoxes adds nothing to bend it */
  for (i = 0; i < 3; i++) {
    static const double rgYear3[3] = {-999, -13000, 12000};
    double jd0 = floor(2451545.0 + (rgYear3[i] - 2000) * 365.25) + 0.5, jd = jd0;
    double v[65], dev, worst = 0;
    int j;
    for (j = 0; j <= 64; j++) {
      v[j] = swe_sidtime0_r(ca, jd, 23.4, 0) - swe_sidtime0_r(ca, jd0, 23.4, 0);
      jd = nextafter(jd, 1e10);
    }
    for (j = 1; j < 64; j++) {
      dev = (v[j] - v[64] * j / 64) * 54000;
      if (fabs(dev) > worst) worst = fabs(dev);
    }
    printf("  %7.0f  64 consecutive JDs: worst off the line %.6f mas\n", rgYear3[i], worst * 1e3);
    if (worst > 1e-5) {
      printf("    FAIL: the rotation angle is rounded at the size of the day count\n");
      bad++;
    }
  }
  /* the integral past the table's 400 centuries takes one more panel at
   * every 10: at such a T and mid-way, delta t 0 */
  {
    swe_ctx *c = swe_ctx_new(), *ci = swe_ctx_new();
    char mi[] = "0,0,0,0,0,0,0,3";
    double worst = 0;
    int k, s, mid;
    swe_set_astro_models_r(ci, mi, 0);
    swe_set_delta_t_userdef_r(c, 0);
    swe_set_delta_t_userdef_r(ci, 0);
    for (s = 0; s < 2; s++)
      for (k = 20; k <= 140; k += 60)
        for (mid = 0; mid < 2; mid++) {
          double T = (s ? -1 : 1) * (400 + 10 * k + 5 * mid), w;
          w = off_line(c, ci, 2451545.0 + T * 36525.0);
          if (w > worst)
            worst = w;
          if (w > 1e-11) {
            printf("    FAIL: T %+.0f cy, the precession's integral %.2e deg off the line\n", T, w);
            bad++;
          }
        }
    printf("  +-600..1805 cy   64 consecutive JDs less IERS 2010's: worst off the line %.2e deg\n",
           worst);
    swe_ctx_free(c);
    swe_ctx_free(ci);
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* the table's rate at its nodes: the long-term sidereal time less the
   * IERS 2010 one, put back on the precession (the IAU 2006 polynomial
   * added, the -0.003874"/cy of SIDT_PRA_NUTC taken off): its cubic-exact
   * one-sided derivative at the node, against pra_rate() over -+ 256 days */
  {
    static const struct { const char *mod; double T; } nd[8] = {
      { "0,9,9", 20 }, { "0,9,9", -20 }, { "0,9,9", 100 }, { "0,9,9", -100 },
      { "0,9,9", 300 }, { "0,9,9", -300 }, { "0,8,8", 20 }, { "0,8,8", -20 } };
    const double dts = ldexp(1, -11), hc = 36525.0 / 4;
    double worst = 0;
    int k, j;
    for (k = 0; k < 8; k++) {
      char mo[40], mi[] = "0,0,0,0,0,0,0,3";
      swe_ctx *c = swe_ctx_new(), *ci = swe_ctx_new();
      double T = nd[k].T, tn = 2451545.0 + T * 36525.0, dir = T < 0 ? -1 : 1, v[4], r, e;
      double d0 = 0, p0 = 0;
      snprintf(mo, sizeof mo, "%s,0,0,0,0,4", nd[k].mod);
      swe_set_astro_models_r(c, mo, 0);
      swe_set_astro_models_r(ci, mi, 0);
      swe_set_delta_t_userdef_r(c, dts);
      swe_set_delta_t_userdef_r(ci, dts);
      for (j = 0; j < 4; j++) {
        double ut = tn + dir * j * hc - dts, t = T + dir * j / 4.0;
        double d = swe_sidtime0_r(c, ut, 23.4, 0) - swe_sidtime0_r(ci, ut, 23.4, 0);
        double p = t * (4612.156534 + t * (1.3915817 + t * (-0.00000044 + t * (-0.000029956
                   + t * -0.0000000368))));
        if (j == 0) {
          d0 = d;
          p0 = p;
        }
        v[j] = wrap(d - d0) * 54000 + (p - p0);
      }
      r = dir * (-11 * v[0] + 18 * v[1] - 9 * v[2] + 2 * v[3]) / (6 * hc) * 36525.0 + 0.003874;
      e = fabs(r - pra_rate(c, tn, 256));
      if (e > worst)
        worst = e;
      if (!(e <= 1e-7)) {
        printf("    FAIL: models %s, the table's rate at its node %+.0f cy %.2e\"/cy off the precession's\n",
               nd[k].mod, T, e);
        bad++;
      }
      swe_ctx_free(c);
      swe_ctx_free(ci);
    }
    printf("  +-20..300 cy     the table's rate at its nodes: worst %.2e\"/cy off the precession's\n",
           worst);
  }
#endif
  swe_ctx_free(ca);
  swe_ctx_free(cb);
  printf(bad ? "G39 FAIL: %d\n" : "G39 PASS\n", bad);
  return bad ? 1 : 0;
}
