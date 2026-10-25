/* framestep.c -- G139: across the steps of the frame's models every speed of
 * date is the rate of its own side's model: SEMOD_JPLHORA_3's at 1962 Jan
 * 20, SEFLG_JPLHOR's at 1799 Jan 1 and 2202 Jan 1, the edges of a
 * short-term precession model's window, and the joins of Owen 1990's
 * pieces.
 *
 * The defect (UPSTREAM-BUGS 153): with SEFLG_JPLHOR_APPROX under
 * SEMOD_JPLHORA_3 (the default mode), the precession, the mean obliquity
 * and the nutation are SEFLG_JPLHOR's (IAU 1976, IAU 1980 plus a fixed
 * offset) up to and including JD 2437684.5 and the chosen models after it:
 * steps of 0.113" in the precession (in longitude), 3.19 mas in the mean
 * obliquity, 7.17e-2" in dpsi on the default models. The rates that
 * difference those models reached across the step: the precession's for a
 * model other than Vondrak's, (P(t + 1) - P(t - 1)) / 2, the topocentric
 * site's precession in right ascension the same way, the obliquity's
 * eps(t + 1) - eps(t), the nutation's central difference over 1e-4 day,
 * and SEFLG_SPEED3's three places over 1e-4 day. On de200, for t in
 * (switch - 1, switch], every longitude speed of date was 1.57e-5 deg/day
 * off (Polaris's right ascension 1.31e-4, latitudes up to 8.9e-7 from the
 * obliquity), the topocentric Moon's 1.58e-7 more from its site; within
 * 1e-4 day of it 9.96e-2 deg/day (Polaris's right ascension 1.09); under
 * SEFLG_SPEED3 within 1e-4 day 5.75e-2; with IAU 2006 precession and IAU
 * 2000B nutation after it, the same within a day after it too.
 *
 * The defect (UPSTREAM-BUGS 159): SEFLG_JPLHOR's precession and obliquity
 * are IAU 1976's strictly between 1799 Jan 1 (JD 2378131.5) and 2202 Jan 1
 * (2525323.5) and Owen 1990's outside, so each boundary's own instant is
 * Owen's; SEMOD_JPLHORA_3 takes them before 1962 too. The mean obliquity
 * steps by 17.6 and 19.8 mas there, the precession by 6.5 and 7.4 mas, and
 * the same differences reached across: the precession's on both sides (both
 * models are differenced), the obliquity's from the day before, SPEED3's
 * within 1e-4 day.
 *
 * The defect (UPSTREAM-BUGS 160): a short-term precession model chosen with
 * swe_set_astro_models() holds within its window, IAU 1976 and 2000 within
 * 2 centuries of J2000, IAU 2006 within 75 (the edge's instant inside), and
 * the long-term model outside: with IAU 1976 and Vondrak ("0,9,1") the
 * precession steps by 0.53" at J2000 - 2 centuries and 0.57" at + 2, the
 * mean obliquity by 3.4 and 81.6 mas; with IAU 2006 at -+ 75 centuries the
 * precession by 42" and 107", the obliquity by 68" and 53". The same rates
 * reached across: the
 * precession's within a day inside the window (Vondrak's own rate holds
 * outside, unless the long-term model is another one differenced too),
 * the obliquity's the day before the edge, SPEED3's within 1e-4 day.
 *
 * The defect (UPSTREAM-BUGS 183): Owen 1990's precession and obliquity,
 * the long-term model chosen ("0,10") or SEFLG_JPLHOR's outside 1799-2202,
 * are fits in pieces of 80 centuries joined at JD -1931455.5, 990544.5,
 * 3912544.5 and 6834544.5 (about -10000, -2000, +6000, +14000; the join's
 * instant the later piece's), and they do not meet: the mean obliquity
 * steps by 0.095 mas at -10000, the precession by 0.086 and 0.091 mas at
 * +6000 and +14000. The same rates reached across: the obliquity's 2.6e-8
 * deg/day the day before -10000, the precession's 9.8e-9 and 1.1e-8 on
 * both days at +6000 and +14000.
 *
 * Check (a): de200, SEFLG_JPLHOR_APPROX; at 1962 Jan 20 the default models
 * and IAU 2006 precession with IAU 2000B nutation ("0,8,8,4", which
 * difference after the step too), at 1799 Jan 1 the default models; and
 * without the flag IAU 1976's window at J2000 - 2 centuries ("0,9,1",
 * with the IERS 2010 sidereal time throughout, so that the long-term
 * sidereal time's own reach across the edge stays out of the topocentric
 * Moon); the Sun, the Moon, Mars, Polaris (swe_fixstar2()), sidereal Mars
 * (Lahiri; swe_calc() drops the Horizons flags for it, so it has no step
 * at 1962 and 1799), the topocentric Moon at Zurich, and Mars under
 * SEFLG_SPEED3; ecliptic and equatorial, longitude and latitude speeds, at
 * 105 instants across (step - 2, step + 2]: every 1/16 day, and the step
 * -+ 2^-k for k = 7..26. The
 * reference is a five-point difference of the same call's positions over
 * 2^-6 day, centred where its points lie on the instant's side of the step,
 * else one-sided on that side (t, t -+ h, ..., t -+ 4h). The topocentric
 * Moon takes delta t set to 2^-11 day, so that its UT1 is exact and the
 * site has no jitter, and a centred nine-point difference over 2^-7 day, at
 * the instants where that lies on their side (the one-sided ones carry the
 * diurnal parallax's curvature, 3e-6 at five points over 2^-8). Each
 * setting, body and frame is judged on the worst of its two speeds over
 * three spans, the day before the step, the day after it and the rest: 168
 * checks, within 1e-8 deg/day, SEFLG_SPEED3 within 1e-5 (its own three
 * places over 1e-4 day are 1.3e-6 off everywhere).
 *
 * Check (b): the rates themselves under SEFLG_JPLHOR, which swe_calc()
 * takes only with a DE 403 file and the IERS corrections: the precession's
 * (swi_precess_speed() of a unit vector of date) and the mean obliquity's
 * (swi_obliquity_rate()), against five-point differences of swi_precess()
 * and swi_epsiln() over 2^-6 day on the instant's side, at the same 105
 * instants about 1799 Jan 1 and 2202 Jan 1, the worst over the three spans;
 * and without the flag at the short-term windows' edges, IAU 1976 ("0,9,1")
 * at J2000 -+ 2 centuries, IAU 2000 ("0,9,6") at + 2, IAU 2006 ("0,9,8")
 * at -+ 75, IAU 1976 within IAU 2006 long-term ("0,8,1", differenced on
 * both sides) at - 2, and IAU 1976 at + 2 with SEFLG_JPLHOR_APPROX, whose
 * mode 3 takes the chosen models after 1962: 54 checks within 1e-8
 * deg/day; and Owen 1990's joins, "0,10" at all four and SEFLG_JPLHOR at
 * the three that step, 42 checks within 1e-9 (the residuals sat about the
 * 1e-8 floor). It calls the fork's own functions and is left out of the
 * upstream build.
 *
 * Measured after the fix: (a) within 2.4e-9 (the Moon's one-sided
 * reference at J2000 - 2 centuries, 2.0e-9 at 1799; Polaris's right
 * ascension 1.45e-9 at 1962, the topocentric Moon 9.6e-10, the others
 * 7e-11), SPEED3 1.41e-6; (b) within 8.5e-12, at Owen's joins 1.9e-11.
 * Before the fix of bug 183, 10 of the 42 at Owen's joins fail: the
 * obliquity's rate 2.63e-8 the day before -10000, the precession's 9.80e-9
 * and 1.09e-8 on both days at +6000 and +14000, under both settings. Before the fix of bug 153,
 * 38 of the first 84 checks fail: every body but sidereal Mars on the day
 * before the step under both settings and on the day after under the
 * second, and SPEED3 on both days under both, by 1.57e-5 to 1.09 deg/day.
 * Before the fix of bug 159, 24 of
 * the 42 at 1799 fail, every body but sidereal Mars on both days (the
 * Sun's latitude speed 5.0e-6 deg/day off, Polaris's right ascension
 * 5.6e-6, SPEED3 8.9e-3), and 6 of (b)'s 12: the precession's rate 2.8e-7
 * and 1.0e-6 on both days, the obliquity's 4.9e-6 and 5.5e-6 the day
 * before. Before the fix of bug 160, 22 of the 42 at J2000 - 2 centuries
 * fail, every body on the day after (inside the window, the precession's,
 * 6.5e-6 to 8.7e-5 deg/day) and in the ecliptic on the day before (the
 * obliquity's, 6.5e-7 to 9.3e-7), SPEED3 0.80 to 0.83 on both, and 15 of
 * (b)'s 42 at the windows: the precession's rate 2.6e-6 to 1.5e-2
 * deg/day, the obliquity's 4.5e-7 to 1.9e-2. The upstream build fails 160
 * of (a)'s 168: its nutation rate, a backward difference, reaches across
 * the 1962 step just after it (0.200 deg/day, Polaris's right ascension
 * 2.14), its ayanamsa rate, a backward difference too, across the window's
 * edge (sidereal Mars 0.160), its SPEED3 2.48 there, and its own speeds
 * are off elsewhere by the defects earlier entries fixed (the topocentric
 * Moon 4e-3, sidereal Mars 5e-7); only SPEED3 away from the steps passes.
 * Run as
 *   framestep EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#ifndef SWE_UPSTREAM_COMPAT
#include "sweph.h"
#include "swephlib.h"
#endif

/* check (a)'s settings: the step, whether its own instant is the left
 * side's model (at or before it) or the right side's, the flags with
 * SEFLG_JPLEPH, the models */
static const struct {
  double ts;
  int left;
  int32 fl;
  const char *mod;
} SET[] = {
  /* HORIZONS_TJD0_DPSI_DEPS_IAU1980 */
  { 2437684.5, 1, SEFLG_JPLHOR_APPROX, "0,0,0,0,0,0,0,0" },
  { 2437684.5, 1, SEFLG_JPLHOR_APPROX, "0,8,8,4,0,0,0,0" },
  /* SEFLG_JPLHOR's 1799 Jan 1 */
  { 2378131.5, 1, SEFLG_JPLHOR_APPROX, "0,0,0,0,0,0,0,0" },
  /* IAU 1976's short-term window, J2000 - 2 centuries; the sidereal
   * time the IERS 2010 formula's throughout */
  { 2378495.0, 0, 0, "0,9,1,0,0,0,0,3" },
};
#define NSET ((int) (sizeof SET / sizeof SET[0]))

enum { B_SUN, B_MOON, B_MARS, B_POLARIS, B_SIDMARS, B_TOPOMOON, B_SPEED3, NB };
static const char *BN[NB] = { "Sun", "Moon", "Mars", "Polaris", "sidereal Mars",
  "topocentric Moon", "Mars SPEED3" };
static const char *RN[3] = { "day before", "day after", "outside" };

/* the step and flags in use */
static double TS;
static int LEFT;
static int32 FL;

/* the place at t, in degrees, and its speeds; the flags returned checked to
 * be de200's, with the Horizons approximation where asked (but for
 * sidereal Mars) */
static int place(swe_ctx *c, int b, double t, int32 xf, double *x)
{
  char serr[256] = "", s[SE_MAX_STNAME * 2 + 1];
  int32 fl = FL | SEFLG_SPEED | xf, rf;
  switch (b) {
  case B_SUN: rf = swe_calc_r(c, t, SE_SUN, fl, x, serr); break;
  case B_MOON: rf = swe_calc_r(c, t, SE_MOON, fl, x, serr); break;
  case B_MARS: rf = swe_calc_r(c, t, SE_MARS, fl, x, serr); break;
  case B_POLARIS:
    strcpy(s, "Polaris");
    rf = swe_fixstar2_r(c, s, t, fl, x, serr);
    break;
  case B_SIDMARS: rf = swe_calc_r(c, t, SE_MARS, fl | SEFLG_SIDEREAL, x, serr); break;
  case B_TOPOMOON: rf = swe_calc_r(c, t, SE_MOON, fl | SEFLG_TOPOCTR, x, serr); break;
  default:
    rf = swe_calc_r(c, t, SE_MARS, (fl & ~SEFLG_SPEED) | SEFLG_SPEED3, x, serr);
    break;
  }
  if (rf < 0 || !(rf & SEFLG_JPLEPH)
      || (b != B_SIDMARS && (rf & SEFLG_JPLHOR_APPROX) != (FL & SEFLG_JPLHOR_APPROX))) {
    printf("  ERROR %s at %.8f: flags 0x%x %s\n", BN[b], t, (unsigned) rf, serr);
    return -1;
  }
  return 0;
}

/* t on the step's left side: before it, or at it where that is the left's */
static int left_of(double t)
{
  return LEFT ? (t <= TS) : (t < TS);
}

/* on the instant's side of the step */
static int same_side(double p, double t)
{
  return left_of(p) == left_of(t);
}

/* the span of t: the day before the step, the day after it, the rest */
static int span(double t)
{
  if (left_of(t))
    return (t > TS - 1 || (!LEFT && t == TS - 1)) ? 0 : 2;
  return (t < TS + 1 || (LEFT && t == TS + 1)) ? 1 : 2;
}

/* |speed - reference| for the two angles, the larger; 0 with *skip set
 * where the body's reference does not lie on t's side */
static double off(swe_ctx *c, int b, double t, int32 xf, int *skip, int *err)
{
  static const double c5[2] = { 2.0 / 3, -1.0 / 12 };
  static const double c9[4] = { 4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280 };
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  int n = (b == B_TOPOMOON) ? 4 : 2, k, j, centred = 1, dir;
  double h = (b == B_TOPOMOON) ? 1.0 / 128 : 1.0 / 64, x[6], f[9][6], r = 0;
  *skip = 0;
  for (k = 1; k <= n; k++)
    if (!same_side(t - k * h, t) || !same_side(t + k * h, t))
      centred = 0;
  if (!centred && b == B_TOPOMOON) {
    *skip = 1;
    return 0;
  }
  if (place(c, b, t, xf, x) < 0) {
    *err = 1;
    return 0;
  }
  dir = left_of(t) ? -1 : 1;
  for (k = 0; k <= 2 * n; k++) {
    double tk = centred ? t + (k - n) * h : t + dir * k * h;
    if (place(c, b, tk, xf, f[k]) < 0) {
      *err = 1;
      return 0;
    }
  }
  for (j = 0; j < 2; j++) {
    double d = 0, v;
    for (k = 0; k <= 2 * n; k++) {
      v = f[k][j] - f[centred ? n : 0][j];
      if (j == 0)
        v -= 360 * floor(v / 360 + 0.5);
      if (centred) {
        if (k > n) d += v * (n == 2 ? c5 : c9)[k - n - 1];
        if (k < n) d -= v * (n == 2 ? c5 : c9)[n - k - 1];
      } else {
        d += v * o5[k];
      }
    }
    d /= h;
    if (!centred)
      d *= dir;
    if (fabs(x[3 + j] - d) > r)
      r = fabs(x[3 + j] - d);
  }
  return r;
}

/* the 105 instants about TS */
static int instants(double *tq)
{
  int nq = 0, k;
  for (k = -32; k <= 32; k++)
    tq[nq++] = TS + k / 16.0;
  for (k = 7; k <= 26; k++) {
    tq[nq++] = TS - ldexp(1, -k);
    tq[nq++] = TS + ldexp(1, -k);
  }
  return nq;
}

#ifndef SWE_UPSTREAM_COMPAT
/* check (b)'s rate r of g at t, rad/day: 0 the precession of a unit
 * vector of date (three components), 1 the mean obliquity */
static void rate_b(swe_ctx *c, int g, double t, int32 fl, double *r)
{
  if (g == 0) {
    double xx[6] = { 0.3, 0.5, 0, 0, 0, 0 };
    xx[2] = sqrt(1 - xx[0] * xx[0] - xx[1] * xx[1]);
    swi_precess(c, xx, t, fl, J2000_TO_J);
    swi_precess_speed(c, xx, t, fl, J2000_TO_J);
    r[0] = xx[3];
    r[1] = xx[4];
    r[2] = xx[5];
  } else {
    r[0] = swi_obliquity_rate(c, t, fl, FALSE);
  }
}

static void value_b(swe_ctx *c, int g, double t, int32 fl, double *v)
{
  if (g == 0) {
    v[0] = 0.3;
    v[1] = 0.5;
    v[2] = sqrt(1 - v[0] * v[0] - v[1] * v[1]);
    swi_precess(c, v, t, fl, J2000_TO_J);
  } else {
    v[0] = swi_epsiln(c, t, fl);
  }
}

/* check (b): the worst |rate - reference| of g over each span, deg/day */
static void check_b(swe_ctx *c, int g, int32 fl, double *w)
{
  static const double c5[2] = { 2.0 / 3, -1.0 / 12 };
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  double tq[105], h = 1.0 / 64;
  int nq = instants(tq), i, k, j, ng = (g == 0) ? 3 : 1;
  w[0] = w[1] = w[2] = 0;
  for (i = 0; i < nq; i++) {
    double t = tq[i], r[3], v[5][3], e = 0;
    int centred = 1, dir = left_of(t) ? -1 : 1;
    for (k = 1; k <= 2; k++)
      if (!same_side(t - k * h, t) || !same_side(t + k * h, t))
        centred = 0;
    rate_b(c, g, t, fl, r);
    for (k = 0; k <= 4; k++)
      value_b(c, g, centred ? t + (k - 2) * h : t + dir * k * h, fl, v[k]);
    for (j = 0; j < ng; j++) {
      double d = 0;
      if (centred) {
        d = (c5[0] * (v[3][j] - v[1][j]) + c5[1] * (v[4][j] - v[0][j])) / h;
      } else {
        for (k = 0; k <= 4; k++)
          d += o5[k] * (v[k][j] - v[0][j]);
        d *= dir / h;
      }
      if (fabs(r[j] - d) > e)
        e = fabs(r[j] - d);
    }
    e *= RADTODEG;
    if (e > w[span(t)])
      w[span(t)] = e;
  }
}
#endif

int main(int argc, char **argv)
{
  double tq[105];
  int nq, m, b, f, k, i, bad = 0, n = 0, err = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: framestep EPHEDIR\n");
    return 2;
  }
  printf("G139: the speeds across the steps of the frame's models are each side's rate\n");
  for (m = 0; m < NSET; m++) {
    char mod[40];
    swe_ctx *c = swe_ctx_new();
    TS = SET[m].ts;
    LEFT = SET[m].left;
    FL = SEFLG_JPLEPH | SET[m].fl;
    nq = instants(tq);
    strcpy(mod, SET[m].mod);
    swe_set_ephe_path_r(c, argv[1]);
    swe_set_jpl_file_r(c, "de200.eph");
    swe_set_topo_r(c, 8.55, 47.37, 400);
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    swe_set_delta_t_userdef_r(c, ldexp(1, -11));
    swe_set_astro_models_r(c, mod, 0);
    printf("  (a) step JD %.1f, models %s: worst |speed - reference|, deg/day, %s / %s / %s\n",
           TS, SET[m].mod, RN[0], RN[1], RN[2]);
    for (b = 0; b < NB; b++)
      for (f = 0; f < 2; f++) {
        int32 xf = f ? SEFLG_EQUATORIAL : 0;
        double w[3] = { 0, 0, 0 }, lim = (b == B_SPEED3) ? 1e-5 : 1e-8;
        for (i = 0; i < nq; i++) {
          int skip;
          double d = off(c, b, tq[i], xf, &skip, &err);
          if (err) {
            swe_ctx_free(c);
            printf("G139 FAIL: ERROR\n");
            return 1;
          }
          if (skip)
            continue;
          if (d > w[span(tq[i])])
            w[span(tq[i])] = d;
        }
        printf("    %-16s %s  %.2e  %.2e  %.2e\n", BN[b], f ? "equ" : "ecl", w[0], w[1], w[2]);
        for (k = 0; k < 3; k++) {
          n++;
          if (!(w[k] <= lim)) {
            bad++;
            printf("    FAIL %s %s, %s: %.2e > %.0e\n", BN[b], f ? "equatorial" : "ecliptic",
                   RN[k], w[k], lim);
          }
        }
      }
    swe_ctx_free(c);
  }
#ifndef SWE_UPSTREAM_COMPAT
  {
    /* check (b): the rates themselves */
    static const struct {
      double ts;
      int left;
      int32 fl;
      const char *mod;
      double lim;
    } SB[] = {
      /* SEFLG_JPLHOR's own steps, both Owen's at their instant */
      { 2378131.5, 1, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-8 },
      { 2525323.5, 0, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-8 },
      /* the short-term windows' edges, the window's at its instant: IAU
       * 1976 and 2000 at J2000 -+ 2 centuries, IAU 2006 at -+ 75; with a
       * long-term model differenced too (IAU 2006, "0,8,1"); and under
       * SEMOD_JPLHORA_3 after 1962 */
      { 2378495.0, 0, 0, "0,9,1,0,0,0,0,0", 1e-8 },
      { 2524595.0, 1, 0, "0,9,1,0,0,0,0,0", 1e-8 },
      { 2524595.0, 1, 0, "0,9,6,0,0,0,0,0", 1e-8 },
      { -287830.0, 0, 0, "0,9,8,0,0,0,0,0", 1e-8 },
      { 5190920.0, 1, 0, "0,9,8,0,0,0,0,0", 1e-8 },
      { 2378495.0, 0, 0, "0,8,1,0,0,0,0,0", 1e-8 },
      { 2524595.0, 1, SEFLG_JPLHOR_APPROX, "0,9,1,0,0,0,0,0", 1e-8 },
      /* Owen 1990's pieces, the later piece's at their joins: chosen as
       * the long-term model ("0,10") at all four, and SEFLG_JPLHOR's
       * outside 1799-2202 at the three where it steps; within 1e-9, as
       * the residuals they had sit about the 1e-8 floor (9.8e-9 to 2.6e-8)
       * and the rates come within 1e-11 */
      { -1931455.5, 0, 0, "0,10,0,0,0,0,0,0", 1e-9 },
      { 990544.5, 0, 0, "0,10,0,0,0,0,0,0", 1e-9 },
      { 3912544.5, 0, 0, "0,10,0,0,0,0,0,0", 1e-9 },
      { 6834544.5, 0, 0, "0,10,0,0,0,0,0,0", 1e-9 },
      { -1931455.5, 0, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-9 },
      { 3912544.5, 0, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-9 },
      { 6834544.5, 0, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-9 },
    };
    static const char *gn[2] = { "precession", "obliquity" };
    int g;
    for (m = 0; m < (int) (sizeof SB / sizeof SB[0]); m++) {
      char mod[40];
      swe_ctx *c = swe_ctx_new();
      strcpy(mod, SB[m].mod);
      swe_set_astro_models_r(c, mod, 0);
      TS = SB[m].ts;
      LEFT = SB[m].left;
      printf("  (b) flags 0x%x, models %s, step JD %.1f: worst |rate - reference|, deg/day,"
             " %s / %s / %s\n", (unsigned) SB[m].fl, SB[m].mod, TS, RN[0], RN[1], RN[2]);
      for (g = 0; g < 2; g++) {
        double w[3];
        check_b(c, g, SB[m].fl, w);
        printf("    %-16s      %.2e  %.2e  %.2e\n", gn[g], w[0], w[1], w[2]);
        for (k = 0; k < 3; k++) {
          n++;
          if (!(w[k] <= SB[m].lim)) {
            bad++;
            printf("    FAIL flags 0x%x, models %s, %s, %s: %.2e > %.0e\n",
                   (unsigned) SB[m].fl, SB[m].mod, gn[g], RN[k], w[k], SB[m].lim);
          }
        }
      }
      swe_ctx_free(c);
    }
  }
#endif
  if (bad) {
    printf("G139 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG139 PASS\n", n);
  return 0;
}
