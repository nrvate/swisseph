/* jplhorbias.c -- G141: SEFLG_JPLHOR_APPROX's frame bias and Horizons
 * offset in right ascension, on a JPL file of DE 403 or later, take the
 * side of SEMOD_JPLHORA_3's step at 1962 Jan 20 that the precession and
 * nutation take, and the speeds are their places' rates.
 *
 * The defect (UPSTREAM-BUGS 161): with SEFLG_JPLHOR_APPROX under
 * SEMOD_JPLHORA_3 (the default mode), swi_precess(), swi_epsiln() and
 * calc_nutation() take SEFLG_JPLHOR's models up to and including JD
 * 2437684.5 and the chosen models after it, while swi_bias() left the frame
 * bias (23 mas) and the Horizons offset in right ascension (-51.26 mas)
 * out for its tjd < the step only, and was given another instant than they
 * are: the light-time instant the body was read at, and for the true Sun
 * 0. So at the step a fixed star, a true position and the topocentric site
 * had the left side's models with the right side's bias and offset, and
 * within the light time after it the Sun, the Moon and the planets the
 * chosen models without them (the true Sun at every date after it).
 *
 * The check: de200 read as DE 431 (the context's DE number set after the
 * file opens: white-box, on purpose -- no file of DE 403 or later ships,
 * and the bias and the offset are rotations, whatever the coefficients),
 * SEFLG_JPLHOR_APPROX, the default models; the Sun, the Moon, Mars,
 * Saturn, Polaris (swe_fixstar2()), the true Sun and true Mars, the
 * topocentric Moon at Zurich and heliocentric Mars, ecliptic and
 * equatorial. After the step: at 148 instants in (step, step + 1] (every
 * 1/64 day, every 1/1024 day over the first 1/16, and step + 2^-k for k =
 * 7..26) the place under SEMOD_JPLHORA_3 against SEMOD_JPLHORA_1's, which
 * has the chosen models with the bias and offset throughout. At the step:
 * the place against its extrapolation from the left, a five-point one over
 * 2^-12 day. Both within 1e-9 deg; 36 checks. And SEFLG_SPEED3 at the
 * step, true Mars's longitude speed against the five-point difference
 * backward over 2^-8 day, within 1e-5 deg/day: 37 checks. Delta t is set
 * to 2^-11 day, so that the topocentric site's UT1 is exact.
 *
 * Measured after the fix: SEMOD_JPLHORA_3 is SEMOD_JPLHORA_1 bit for bit
 * after the step; at the step within 3.0e-11 deg (the Moon); SPEED3
 * 1.01e-9. Before it 21 of 37 fail: after the step every body with a
 * light time, 1.11e-5 to 1.24e-5 deg over it (the Sun's 8.2 minutes,
 * Mars's 20, Saturn's 91), the true Sun throughout; at the step Polaris
 * 1.15e-5 in longitude and 4.95e-5 in right ascension, true Mars 1.12e-5,
 * the topocentric Moon 1.7e-7 (its site's frame); SPEED3 0.162 deg/day.
 * The upstream build fails 25: those, and at the step Mars and
 * heliocentric Mars by 1.3e-9 and the topocentric Moon by 1.1e-6, its own
 * defects that earlier entries fixed.
 *
 * And the speeds (UPSTREAM-BUGS 162): swi_approx_jplhor() turned the
 * position about the pole by the offset and not the velocity, so a speed
 * was off its places' rate by the offset (2.5e-7 rad) times the
 * transverse velocity, under SEMOD_JPLHORA_1 at every date and under
 * SEMOD_JPLHORA_3 after the step. The check: the nine's speeds against
 * five-point differences of their places over 2^-6 day (the topocentric
 * Moon nine-point over 2^-7, its diurnal parallax's curvature), centred at
 * binary-fraction instants on seven dates 1950-2024 under SEMOD_JPLHORA_1
 * and the six after the step under SEMOD_JPLHORA_3, none within a day of
 * a year of the offset's table; the worst of the two angles within 1e-8
 * deg/day and of the distance within 1e-11 AU/day (not Polaris's): 36
 * checks, 73 in all. Measured after the fix within 8.5e-10 deg/day (the
 * topocentric Moon; the others 3.6e-10, nearly all of it the table's own
 * rate, left out, at most 3.8e-10), 1.7e-13 AU/day. Before it 32 of the
 * 36 fail, Polaris passing: the Moon's latitude speed 1.46e-6 deg/day,
 * Mars's right ascension 1.25e-7, the Sun's latitude 1.02e-7, Saturn's
 * 2.6e-8, heliocentric Mars's 5.8e-8; the distances' 7.2e-9 AU/day
 * (Mars), 4.1e-9 (the Sun). The upstream build fails all 36, by these and
 * its own speed defects that earlier entries fixed (ecliptic speeds
 * 1.5e-5 to 5.3e-5 deg/day, the topocentric Moon 4.4e-3, Saturn's
 * distance 1.9e-5 AU/day).
 *
 * And the offset's table (UPSTREAM-BUGS 173): swi_approx_jplhor()
 * interpolated its yearly values from 1962 Jul 1 backwards, (t - t0)
 * (v[t0] - v[t1]) + v[t0], so the offset left each year at 2 v[t0] - v[t1]
 * and jumped by 2 (v[t1] - v[t0]) to the next year's value, up to 0.99
 * mas (1963-2011), its slope the wrong way between. The check: Polaris's
 * and Mars's right ascension under SEMOD_JPLHORA_1 at each of the 49
 * inner years' starts b, the place at b + 2^-10 less that at b - 2^-10
 * less 2^-9 day of their mean speed, the worst within 1e-9 deg: 2 checks,
 * 75 in all. Measured after the fix 2.9e-10 deg on Polaris (its own speed
 * over the 2^-9 day), 2.0e-12 on Mars; before it 2.87e-7 and 2.76e-7 at
 * 2006 Jul 1 (0.99 mas of offset), and so in the upstream build.
 *
 * And the obliquity's (UPSTREAM-BUGS 182): swi_epsiln()'s yearly offset
 * table under SEFLG_JPLHOR_APPROX, interpolated the same way round. The
 * check: SE_ECL_NUT's mean obliquity (it has no speeds), its second
 * difference over 2^-10 day at each of the 49 inner years' starts, within
 * 1e-9 deg: 1 check, 76 in all. Measured after the fix 2.0e-13 deg;
 * before it 1.22e-7 at 1972 Jul 1 (0.44 mas), and so in the upstream
 * build. Run as
 *   jplhorbias EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"	/* ctx->jpldenum: de200 read as DE 431 */

#define TS 2437684.5	/* HORIZONS_TJD0_DPSI_DEPS_IAU1980 */
#define FL (SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX)

enum { B_SUN, B_MOON, B_MARS, B_SATURN, B_POLARIS, B_TRUESUN, B_TRUEMARS,
  B_TOPOMOON, B_HELMARS, NB };
static const char *BN[NB] = { "Sun", "Moon", "Mars", "Saturn", "Polaris",
  "true Sun", "true Mars", "topocentric Moon", "heliocentric Mars" };

/* the place at t, degrees and AU, and its speeds; the return checked to be
 * the Horizons approximation's on the JPL file */
static int place(swe_ctx *c, int b, double t, int32 xf, double *x)
{
  char serr[256] = "", s[SE_MAX_STNAME * 2 + 1];
  int32 fl = FL | SEFLG_SPEED | xf, rf;
  switch (b) {
  case B_SUN: rf = swe_calc_r(c, t, SE_SUN, fl, x, serr); break;
  case B_MOON: rf = swe_calc_r(c, t, SE_MOON, fl, x, serr); break;
  case B_MARS: rf = swe_calc_r(c, t, SE_MARS, fl, x, serr); break;
  case B_SATURN: rf = swe_calc_r(c, t, SE_SATURN, fl, x, serr); break;
  case B_POLARIS:
    strcpy(s, "Polaris");
    rf = swe_fixstar2_r(c, s, t, fl, x, serr);
    break;
  case B_TRUESUN: rf = swe_calc_r(c, t, SE_SUN, fl | SEFLG_TRUEPOS, x, serr); break;
  case B_TRUEMARS: rf = swe_calc_r(c, t, SE_MARS, fl | SEFLG_TRUEPOS, x, serr); break;
  case B_TOPOMOON: rf = swe_calc_r(c, t, SE_MOON, fl | SEFLG_TOPOCTR, x, serr); break;
  default: rf = swe_calc_r(c, t, SE_MARS, fl | SEFLG_HELCTR, x, serr); break;
  }
  if (rf < 0 || !(rf & SEFLG_JPLEPH) || !(rf & SEFLG_JPLHOR_APPROX) || c->jpldenum != 431) {
    printf("  ERROR %s at %.8f: flags 0x%x, DE %d %s\n", BN[b], t, (unsigned) rf,
           (int) c->jpldenum, serr);
    return -1;
  }
  return 0;
}

/* a context on de200 read as DE 431, the models given */
static swe_ctx *open_ctx(const char *ephe, const char *models)
{
  char mod[40], serr[256];
  double x[6];
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  strcpy(mod, models);
  swe_set_ephe_path_r(c, ephe);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_topo_r(c, 8.55, 47.37, 400);
  swe_set_astro_models_r(c, mod, 0);
  swe_set_delta_t_userdef_r(c, ldexp(1, -11));	/* the site's UT1 exact */
  if (swe_calc_r(c, 2451545.0, SE_MARS, SEFLG_JPLEPH, x, serr) < 0 || c->jpldenum != 200) {
    printf("  ERROR de200.eph: DE %d %s\n", (int) c->jpldenum, serr);
    swe_ctx_free(c);
    return NULL;
  }
  c->jpldenum = 431;
  return c;
}

static double dang(double a, double b)
{
  double d = a - b;
  return d - 360 * floor(d / 360 + 0.5);
}

/* the larger of the two angles' differences, degrees */
static double off2(const double *x, const double *y)
{
  return fmax(fabs(dang(x[0], y[0])), fabs(x[1] - y[1]));
}

static int bad = 0, n = 0;

static void judge(const char *what, int b, int f, double d, double lim)
{
  n++;
  if (!(d <= lim)) {
    bad++;
    printf("    FAIL %s %s %s: %.2e > %.0e\n", BN[b], f ? "equatorial" : "ecliptic", what, d, lim);
  }
}

/* the side of the step: after it SEMOD_JPLHORA_3 against SEMOD_JPLHORA_1,
 * at it against the extrapolation from the left */
static int side(swe_ctx *c3, swe_ctx *c1)
{
  double tq[148], x[6], y[6], f5[5][6], h = ldexp(1, -12);
  int nq = 0, b, f, k, j;
  for (k = 1; k <= 64; k++)
    tq[nq++] = TS + k / 64.0;
  for (k = 1; k <= 64; k++)
    tq[nq++] = TS + k / 1024.0;
  for (k = 7; k <= 26; k++)
    tq[nq++] = TS + ldexp(1, -k);
  printf("  161: after the step |JPLHORA_3 - JPLHORA_1|, at it |place - left extrapolation|, deg\n");
  for (b = 0; b < NB; b++)
    for (f = 0; f < 2; f++) {
      int32 xf = f ? SEFLG_EQUATORIAL : 0;
      double wa = 0, wb = 0, e[2];
      for (k = 0; k < nq; k++) {
        if (place(c3, b, tq[k], xf, x) < 0 || place(c1, b, tq[k], xf, y) < 0)
          return -1;
        if (off2(x, y) > wa)
          wa = off2(x, y);
      }
      if (place(c3, b, TS, xf, x) < 0)
        return -1;
      for (k = 1; k <= 5; k++)
        if (place(c3, b, TS - k * h, xf, f5[k - 1]) < 0)
          return -1;
      for (j = 0; j < 2; j++) {
        double d1 = f5[0][j] - f5[4][j], d2 = f5[1][j] - f5[4][j], d3 = f5[2][j] - f5[4][j],
               d4 = f5[3][j] - f5[4][j];
        if (j == 0) {
          d1 = dang(f5[0][0], f5[4][0]);
          d2 = dang(f5[1][0], f5[4][0]);
          d3 = dang(f5[2][0], f5[4][0]);
          d4 = dang(f5[3][0], f5[4][0]);
        }
        e[j] = f5[4][j] + 5 * d1 - 10 * d2 + 10 * d3 - 5 * d4;
      }
      wb = off2(x, e);
      printf("    %-17s %s  %.2e  %.2e\n", BN[b], f ? "equ" : "ecl", wa, wb);
      judge("after the step", b, f, wa, 1e-9);
      judge("at the step", b, f, wb, 1e-9);
    }
  {
    /* SEFLG_SPEED3 at the step: its places at t, t - dt, t - 2 dt */
    static const double o5[5] = { 25.0 / 12, -4, 3, -4.0 / 3, 1.0 / 4 };
    char serr[256];
    double d = 0, hs = 1.0 / 256;
    if (swe_calc_r(c3, TS, SE_MARS, FL | SEFLG_TRUEPOS | SEFLG_SPEED3, x, serr) < 0)
      return -1;
    for (k = 0; k < 5; k++) {
      if (place(c3, B_TRUEMARS, TS - k * hs, 0, f5[k]) < 0)
        return -1;
      d += o5[k] * dang(f5[k][0], f5[0][0]);
    }
    d = fabs(x[3] - d / hs);
    printf("    true Mars SPEED3 at the step: %.2e deg/day\n", d);
    judge("SPEED3 at the step", B_TRUEMARS, 0, d, 1e-5);
  }
  return 0;
}

/* the speeds against five-point differences of the places over 2^-6 day
 * (the topocentric Moon nine-point over 2^-7, its diurnal parallax's
 * curvature), centred at binary-fraction instants: SEMOD_JPLHORA_1 at all
 * seven dates, SEMOD_JPLHORA_3 at the six after the step; none within a
 * day of a year of the offset's table */
static int speeds(swe_ctx *c3, swe_ctx *c1)
{
  static const double dates[7] = { 2433282.8, 2437700.3, 2444239.8, 2447000.3,
    2451545.3, 2455000.6, 2460400.3 };
  static const double c5[2] = { 2.0 / 3, -1.0 / 12 };
  static const double c9[4] = { 4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280 };
  int m, b, f, i, k, j;
  printf("  162: |speed - reference|, worst over the dates: longitude/latitude deg/day, distance AU/day\n");
  for (m = 0; m < 2; m++) {
    swe_ctx *c = m ? c3 : c1;
    printf("   %s\n", m ? "SEMOD_JPLHORA_3, after the step" : "SEMOD_JPLHORA_1");
    for (b = 0; b < NB; b++)
      for (f = 0; f < 2; f++) {
        int32 xf = f ? SEFLG_EQUATORIAL : 0;
        int nh = (b == B_TOPOMOON) ? 4 : 2;
        double h = (b == B_TOPOMOON) ? 1.0 / 128 : 1.0 / 64, wa = 0, wd = 0, x[6], fk[9][6];
        for (i = 0; i < 7; i++) {
          double t = floor(dates[i] * 256) / 256;
          if (m == 1 && t <= TS)
            continue;
          if (place(c, b, t, xf, x) < 0)
            return -1;
          for (k = 0; k <= 2 * nh; k++)
            if (place(c, b, t + (k - nh) * h, xf, fk[k]) < 0)
              return -1;
          for (j = 0; j < 3; j++) {
            double d = 0;
            for (k = 1; k <= nh; k++)
              d += (nh == 2 ? c5 : c9)[k - 1]
                   * (j == 0 ? dang(fk[nh + k][0], fk[nh - k][0]) : fk[nh + k][j] - fk[nh - k][j]);
            d = fabs(x[3 + j] - d / h);
            if (j < 2 && d > wa)
              wa = d;
            if (j == 2 && b != B_POLARIS && d > wd)
              wd = d;
          }
        }
        printf("    %-17s %s  %.2e  %.2e\n", BN[b], f ? "equ" : "ecl", wa, wd);
        n++;
        if (!(wa <= 1e-8) || !(wd <= 1e-11)) {
          bad++;
          printf("    FAIL %s %s %s: %.2e deg/day, %.2e AU/day > 1e-8, 1e-11\n",
                 m ? "JPLHORA_3" : "JPLHORA_1", BN[b], f ? "equatorial" : "ecliptic", wa, wd);
        }
      }
  }
  return 0;
}

/* the obliquity's offset table across its years: no jump in the mean
 * obliquity of SE_ECL_NUT, which has no speeds: its second difference over
 * 2^-10 day at each year's start, where the offset's interpolation changes
 * its interval (UPSTREAM-BUGS 182) */
static int table_eps(swe_ctx *c1)
{
  const double e = ldexp(1, -10);
  double w = 0, wt = 0, x[6], y[6], z[6];
  char serr[256];
  int k;
  for (k = 1; k <= 49; k++) {
    double b = 2437846.5 + k * 365.25, d;	/* DCOR_EPS_JPL_TJD0 + k years */
    if (swe_calc_r(c1, b - e, SE_ECL_NUT, FL, x, serr) < 0
        || swe_calc_r(c1, b, SE_ECL_NUT, FL, z, serr) < 0
        || swe_calc_r(c1, b + e, SE_ECL_NUT, FL, y, serr) < 0) {
      printf("  ERROR SE_ECL_NUT at %.2f: %s\n", b, serr);
      return -1;
    }
    d = fabs(y[1] - 2 * z[1] + x[1]);
    if (d > w) {
      w = d;
      wt = b;
    }
  }
  printf("  182: the obliquity's offset across its table's years, SEMOD_JPLHORA_1\n");
  printf("    mean obliquity    worst jump %.2e deg at JD %.2f\n", w, wt);
  n++;
  if (!(w <= 1e-9)) {
    bad++;
    printf("    FAIL mean obliquity: the offset jumps %.2e deg > 1e-9\n", w);
  }
  return 0;
}

/* the offset's table across its years: no jump in the place */
static int table(swe_ctx *c1)
{
  static const int bb[2] = { B_POLARIS, B_MARS };
  const double e = ldexp(1, -10);
  int i, k;
  printf("  173: the offset across its table's years, SEMOD_JPLHORA_1, right ascension\n");
  for (i = 0; i < 2; i++) {
    double w = 0, wt = 0, x[6], y[6];
    for (k = 1; k <= 49; k++) {
      double b = 2437846.5 + k * 365.25, d;	/* DCOR_RA_JPL_TJD0 + k years */
      if (place(c1, bb[i], b - e, SEFLG_EQUATORIAL, x) < 0
          || place(c1, bb[i], b + e, SEFLG_EQUATORIAL, y) < 0)
        return -1;
      d = fabs(dang(y[0], x[0]) - e * (x[3] + y[3]));
      if (d > w) {
        w = d;
        wt = b;
      }
    }
    printf("    %-17s worst jump %.2e deg at JD %.2f\n", BN[bb[i]], w, wt);
    n++;
    if (!(w <= 1e-9)) {
      bad++;
      printf("    FAIL %s: the offset jumps %.2e deg > 1e-9\n", BN[bb[i]], w);
    }
  }
  return 0;
}

int main(int argc, char **argv)
{
  swe_ctx *c3, *c1;
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: jplhorbias EPHEDIR\n");
    return 2;
  }
  printf("G141: SEFLG_JPLHOR_APPROX's bias and Horizons offset on DE 431: their frame's side and instant, the speeds' rate\n");
  c3 = open_ctx(argv[1], "0,0,0,0,0,0,0,0");
  c1 = open_ctx(argv[1], "0,0,0,0,0,0,1,0");
  r = (c3 == NULL || c1 == NULL) ? -1 : side(c3, c1);
  if (r == 0)
    r = speeds(c3, c1);
  if (r == 0)
    r = table(c1);
  if (r == 0)
    r = table_eps(c1);
  if (c3 != NULL)
    swe_ctx_free(c3);
  if (c1 != NULL)
    swe_ctx_free(c1);
  if (r < 0) {
    printf("G141 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G141 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG141 PASS\n", n);
  return 0;
}
