/* intpconv.c -- G55: the interpolated lunar apogee and perigee are their
 * iteration's fixed point, their SEFLG_J2000 latitude speeds are the
 * rates of their positions, and the perigee's light time is its
 * distance's.
 *
 * The defect (UPSTREAM-BUGS 56): swi_intp_apsides() finds the apsis by
 * steps along a line in the lunar arguments, and the next pass's line goes
 * through the new centre with the Moon's four arguments moved alike and
 * the Sun's and planets' back at the date: the iteration converges only
 * linearly, by q = -0.04..+0.15 a pass on the perigee. It stopped after
 * five passes (perigee) or four (apogee), up to 1.21" (perigee) and 2.5
 * mas (apogee) short of the fixed point over -10000..+12000, 1.16" and 2.3
 * mas over the -3000..+3000 that swe_calc() serves. Now the Newton steps
 * are scaled by 1 / (1 - q), q from the last two (a secant), to the fixed
 * point. Bug 45's "Not changed" also had SEFLG_J2000's latitude speed off
 * by 4.6e-7 deg/day (the apogee at -3000) and the mean apogee's by 7.1e-7
 * (12000), from swi_precess_speed()'s rate of the precession; bug 49's fix
 * of that rate already brought both to <= 2.1e-8. Checked here, as no
 * other gate does for these bodies.
 *
 * Bug 72 (this fork's): intp_apsides() chose the light-time estimate's
 * passes by comparing ipl, an SEI_ index, with SE_INTP_PERG, so the
 * perigee's took one pass, not two: its distance up to 92 km from the
 * converged one (rms 20 km), the position up to 0.0049 mas off. Now two
 * passes, 4.3 km (rms 1.1).
 *
 * The check (Swiss files; the apsides are analytic):
 * (a) the perigee and apogee at ten dates, the eight where the old
 *     iteration stopped farthest from its fixed point in -3000..+3000 and
 *     two modern ones, SEFLG_TRUEPOS | SEFLG_NONUT (the iteration's own
 *     place, in the mean ecliptic of date), against the fixed point
 *     computed with fourteen more plain Newton passes: within 0.01 mas
 *     (measured 2.0e-3 perigee, 1.2e-3 apogee, the position's own noise;
 *     the five and four passes before this fix 1163 and 2.3 mas).
 * (b) SEFLG_J2000 | SEFLG_SPEED latitude speeds of the interpolated
 *     perigee and apogee at -2990, -1000, 1000 and 2990 and of the mean
 *     apogee at -10000, -2990, 2990 and 12000, four instants each, against
 *     a five-point difference over 2^-4 day of the positions at multiples
 *     of 2^-8 day: within 1e-7 deg/day (measured <= 2.7e-9 interpolated,
 *     2.1e-8 the mean apogee).
 * (c) the perigee at the five of 3000 dates in -3000..+3000 where bug 72's
 *     one-pass estimate moved it most, NONUT, against the TRUEPOS position
 *     at t - tau, tau its TRUEPOS distance at t over c: within 1e-3 mas
 *     (measured 3.6e-8, the estimate within the JD's resolution of tau;
 *     one pass 3.5e-3..4.9e-3).
 * The upstream build fails (a) by 1164 mas (perigee; 2.4 the apogee),
 * (b) by 4.5e-7 deg/day (interpolated) and 7.6e-7 (mean apogee) and (c)
 * by 110 mas (no light time in x, none at all without SEFLG_SPEED), 68 of
 * 73 checks. Run as
 *   intpconv EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define DEG2MAS 3600000.0
#define RAD2MAS 206264806.24709636
#define AU_LT_DAYS (1.49597870700e11 / 299792458.0 / 86400.0)
/* (c): the five dates of 3000 in -3000..+3000 where the one-pass
 * light-time estimate moved the perigee most */
static const double rgLt[5] = {
  2207849.5698793186, 2486223.6015625, 2223707.9878828423,
  1184806.05078125, 2549429.4271334051,
};

/* jd, then perigee and apogee longitude and latitude (deg), the fixed point
 * of the iteration in the mean ecliptic of date */
static const double rgFix[10][5] = {
  {687314.573, 73.311264799172, 4.504823411884, 259.122731411088, -4.224159855050},
  {844408.598, 311.799794391823, 3.293530632568, 123.974550680549, -2.737048026561},
  {852444.098, 112.902474291857, -5.270566067679, 299.469959606449, 5.284171183374},
  {1009538.123, 349.615728367461, -0.137174707948, 163.857584627325, -0.367797641974},
  {989047.598, 217.251219402858, 3.508007012294, 42.433083360374, -3.827430124977},
  {1600147.373, 266.378682685904, 4.137503944613, 91.158957704267, -4.360947749875},
  {1748804.123, 269.152643784146, 0.939857072550, 93.537319202537, -1.334259550027},
  {2492489.648, 321.679131582195, -2.121537226226, 146.768789506024, 2.535238341818},
  {2451106.823, 30.431158047467, -4.427767738269, 216.325517825504, 4.648110283014},
  {2488070.123, 194.735274367091, -2.122141896229, 12.063738380139, 1.907305553884},
};

static const struct { int ipl; const char *sz; double yr[4]; } rgSpd[3] = {
  {SE_INTP_PERG, "intp perigee", {-2990, -1000, 1000, 2990}},
  {SE_INTP_APOG, "intp apogee", {-2990, -1000, 1000, 2990}},
  {SE_MEAN_APOG, "mean apogee", {-10000, -2990, 2990, 12000}},
};

static int calc(swe_ctx *c, double jd, int ipl, int32 fl, double *x)
{
  char serr[256];
  if (swe_calc_r(c, jd, ipl, SEFLG_SWIEPH | fl, x, serr) < 0) {
    printf("  ERROR jd %.6f ipl %d flags %d: %s\n", jd, ipl, fl, serr);
    return -1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  swe_ctx *c;
  double x[6], p[5][6], h = 1.0 / 16, wa[2] = {0, 0}, wb[3] = {0, 0, 0}, wc = 0;
  int i, j, k, s, bad = 0;
  if (argc < 2) {
    fprintf(stderr, "usage: intpconv EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  /* (a) the fixed point */
  for (i = 0; i < 10; i++) {
    for (j = 0; j < 2; j++) {
      double dl, db, e;
      if (calc(c, rgFix[i][0], j == 0 ? SE_INTP_PERG : SE_INTP_APOG,
               SEFLG_TRUEPOS | SEFLG_NONUT, x))
        return 1;
      dl = swe_difdeg2n(x[0], rgFix[i][1 + 2 * j]) * cos(x[1] * 0.017453292519943295);
      db = x[1] - rgFix[i][2 + 2 * j];
      e = sqrt(dl * dl + db * db) * DEG2MAS;
      if (e > wa[j]) wa[j] = e;
      if (e > 0.01) bad++;
    }
  }
  /* (c) the perigee's light time: X(t) against the TRUEPOS X(t - tau),
   * tau the TRUEPOS distance at t over c (r moves <= 0.04 km over tau) */
  for (i = 0; i < 5; i++) {
    double t = rgLt[i], xa[6], xt[6], tau, d2 = 0, r2 = 0, e;
    if (calc(c, t, SE_INTP_PERG, SEFLG_NONUT | SEFLG_XYZ, xa)
        || calc(c, t, SE_INTP_PERG, SEFLG_TRUEPOS | SEFLG_NONUT | SEFLG_XYZ, xt))
      return 1;
    tau = sqrt(xt[0] * xt[0] + xt[1] * xt[1] + xt[2] * xt[2]) * AU_LT_DAYS;
    if (calc(c, t - tau, SE_INTP_PERG, SEFLG_TRUEPOS | SEFLG_NONUT | SEFLG_XYZ, xt))
      return 1;
    for (k = 0; k < 3; k++) {
      d2 += (xa[k] - xt[k]) * (xa[k] - xt[k]);
      r2 += xt[k] * xt[k];
    }
    e = sqrt(d2 / r2) * RAD2MAS;
    if (e > wc) wc = e;
    if (e > 1e-3) bad++;
  }
  /* (b) the J2000 latitude speed */
  for (i = 0; i < 3; i++) {
    for (k = 0; k < 4; k++) {
      for (s = 0; s < 4; s++) {
        double t = 2451545.0 + (rgSpd[i].yr[k] - 2000) * 365.25 + s * 3.37, e;
        t = floor(t * 256) / 256;
        if (calc(c, t, rgSpd[i].ipl, SEFLG_J2000 | SEFLG_SPEED, x))
          return 1;
        for (j = 0; j < 5; j++)
          if (calc(c, t + (j - 2) * h, rgSpd[i].ipl, SEFLG_J2000, p[j]))
            return 1;
        e = fabs(x[4] - (8 * (p[3][1] - p[1][1]) - (p[4][1] - p[0][1])) / (12 * h));
        if (e > wb[i]) wb[i] = e;
        if (e > 1e-7) bad++;
      }
    }
  }
  printf("G55: the interpolated apsides at their fixed point; J2000 latitude speeds\n");
  printf("  (a) from the fixed point: perigee %.1e mas, apogee %.1e mas\n", wa[0], wa[1]);
  printf("  (b) J2000 latitude speed off: %s %.1e, %s %.1e, %s %.1e deg/day\n",
         rgSpd[0].sz, wb[0], rgSpd[1].sz, wb[1], rgSpd[2].sz, wb[2]);
  printf("  (c) perigee from its converged light time: %.1e mas\n", wc);
  swe_ctx_free(c);
  if (bad) {
    printf("G55 FAIL: %d\n", bad);
    return 1;
  }
  printf("G55 PASS\n");
  return 0;
}
