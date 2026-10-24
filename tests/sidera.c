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
 *      there, differently wherever the multiply-add fuses.
 * The upstream build fails 1 by 4.1" and 2 by up to 19" (at 2500). Run as
 *   sidera
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static double wrap(double d)   /* hours, to (-12, 12] */
{
  while (d > 12) d -= 24;
  while (d <= -12) d += 24;
  return d;
}

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
  swe_ctx_free(ca);
  swe_ctx_free(cb);
  printf(bad ? "G39 FAIL: %d\n" : "G39 PASS\n", bad);
  return bad ? 1 : 0;
}
