/* nutrange.c -- G178: the nutation is taken within its models' range, the
 * formula sidereal times' obliquity within its model's, and a topocentric
 * site's sidereal time is of its call's TT.
 *
 * The defect (UPSTREAM-BUGS 284): the nutation was taken at any date
 * within bug 196's bound, 1e12 days, a bound against hangs and NaN: every
 * series (IAU 1980 with or without Herring 1987's corrections, SEFLG_JPLHOR's
 * IAU 1980, IAU 2000A on the default's day grid and direct, IAU 2000B,
 * Woolard 1953) sums (A + A' T) sin(arg), the A' T the obliquity's secular
 * change linearized, the arguments polynomials in T. Its main term is
 * within 1% of its value at the obliquity of date to -+50 centuries, 13%
 * at -13200 and +17191, 22% at -+200 centuries, 100% at -+1000 (0 at
 * -985, then the other sign); |dpsi| 36.4" at +1000 centuries, 510" at
 * 1e9 days, 13 deg at 1e11, its rate 0.24"/day today, 325"/day at 1e9 days
 * on IAU 2000A and 2000B. SE_ECL_NUT, swe_sidtime(), the houses, the true
 * ayanamsa and a star with no Earth answered from them anywhere. Now each
 * model answers over the widest body range, -3100015.5 .. 8000016.5, and
 * past it what takes the nutation is refused, ERR with the model and the
 * range named (NaN with no error return); under SEFLG_NONUT nothing is.
 *
 * The defect (UPSTREAM-BUGS 283): under the three formula sidereal times
 * swe_sidtime() put the mean obliquity of a polynomial precession model
 * past its range into the equation of the equinoxes: 1.81e-3 deg off
 * Vondrak 2011's obliquity's at + 400 centuries under "0,8,0,0,0,0,0,3",
 * 6.15e-3 at + 1000 under "0,2,0,0,0,0,0,2". Now NaN where SE_ECL_NUT
 * refuses the obliquity or the nutation it takes.
 *
 * The defect (UPSTREAM-BUGS 285): the long-term sidereal time takes its
 * precession model at UT + the default delta t, and a topocentric site's
 * UT is its call's TT less its ephemeris's delta t: at a step of the
 * model (a short-term window's edge) the site's sidereal time was the
 * other model's for 0.297 s after J2000 - 2 centuries on the Swiss files
 * and 0.481 s before it on Moshier ("0,9,1"), 688 s after - 75 centuries
 * on the Swiss files ("0,9,8"). Now of the call's TT.
 *
 * Check (a), 284: under the default models, IAU 1980, IAU 1987, IAU 2000A,
 * IAU 2000B and Woolard named: at TT -3100015.5 and 8000016.5 and the
 * doubles past them, and TT -1e7, 5e7, -1e9 and 1e11: SE_ECL_NUT, the
 * ayanamsa (Lahiri) and barycentric Spica (swe_fixstar2()) and
 * heliocentric Spica on Moshier (swe_fixstar()) answered at the ends, ERR
 * with the outputs 0 and serr naming the model and the range past them;
 * with SEFLG_NONUT SE_ECL_NUT and the star answered at every date, the
 * ayanamsa within Vondrak 2011's precession range. At the UT whose TT is
 * 1e-4 day inside and past each end (and at UT -1e7, 5e7, -1e9, 1e11):
 * swe_sidtime() in [0, 24) inside and NaN past; swe_houses_ex2()
 * (Placidus with speeds, Moshier's delta t) and swe_houses() (the
 * default's) answered inside and ERR, the cusps 0 and serr naming the
 * range past, swe_houses_ex2() with SEFLG_NONUT answered; swe_azalt()
 * SE_EQU2HOR finite inside, NaN past. And SE_ECL_NUT with SEFLG_JPLEPH |
 * SEFLG_JPLHOR_APPROX past -3100015.5 naming SEFLG_JPLHOR's IAU 1980.
 *
 * Check (b), 283: under "0,8,0,0,0,0,0,3", "0,2,0,0,0,0,0,2",
 * "0,2,0,0,0,0,0,1" and "0,10,0,0,0,0,0,1" (Owen 1990, its obliquity's
 * range -4853455.5 .. 9756544.5 wider than the nutation's), at the UT whose
 * TT (the default delta t) is 1e-4 day inside and past the obliquity's
 * range's ends and J2000 -+ 400 and + 1000 centuries: swe_sidtime() NaN
 * where SE_ECL_NUT refuses at that TT and in [0, 24) where it answers; and
 * at UT 1e12 (TT 1.28e12) under "0,0,0,0,0,0,0,3" NaN.
 *
 * Check (c), 285: under "0,9,1" at J2000 - 2 centuries on the Swiss files
 * and Moshier and "0,9,8" at J2000 - 75 centuries on the Swiss files, at
 * 8 instants within the span where the site's sidereal time took the other
 * model and 4 outside it: the topocentric Moon's place (47.37 N 8.55 E
 * 400 m, SEFLG_SPEED), which the site moves by its parallax, and
 * Polaris's topocentric equatorial speeds, which take the site's rate,
 * against a context whose models choose the call's TT's throughout ("0,1,1"
 * or "0,8,8" within the window, "0,9,9" outside): within 1e-10 deg and
 * 1e-8 deg/day.
 *
 * Measured: (a) 0 of 577 off, (b) 0 of 29, (c) 0 of 72, worst 1.6e-12 deg
 * and 3.1e-10 deg/day. Before the fix: (a) 277 of 577 (every refusal
 * answered), (b) 21 of 29 (the sidereal times answered past the ranges),
 * (c) 49 of 72: the Moon 2.91e-7 deg off and Polaris's speed 1.83e-4
 * deg/day at - 2 centuries on either ephemeris, the Moon 2.11e-5 deg at
 * - 75. The upstream build fails 416: (a) 355, (b) 1 (UT 1e12; its
 * SE_ECL_NUT answers where its sidereal time does), (c) 60 (its long-term
 * formula and site its own: the Moon 1.3e-6 .. 7.2e-5 deg, Polaris's
 * speed to 0.64 deg/day). Run as
 *   nutrange EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

#define LO -3100015.5
#define HI 8000016.5
#define J2K 2451545.0

static const char *EPHE;
static int nall, nbad;
static char serr[AS_MAXCH];

static void tally(int ok, const char *what, double t, const char *s)
{
  nall++;
  if (!ok && nbad++ < 12)
    printf("  %s at %.17g: '%s'\n", what, t, s);
}

static swe_ctx *fresh(const char *models)
{
  char mod[40];
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, EPHE);
  strcpy(mod, models);
  swe_set_astro_models_r(c, mod, 0);
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  return c;
}

static int zeros(const double *x, int n)
{
  int i;
  for (i = 0; i < n; i++)
    if (x[i] != 0)
      return 0;
  return 1;
}

/* refused as the nutation's range refuses: ERR, the outputs 0, the model
 * and the range named */
static int nut_refused(int32 r, const double *x, int n, const char *model)
{
  return r == ERR && zeros(x, n) && strstr(serr, model) != NULL
         && strstr(serr, "nutation model's range -3100015.5 .. 8000016.5") != NULL;
}

/* the UT whose TT under delta t of iflag is tt */
static double ut_of(swe_ctx *c, double tt, int32 iflag)
{
  double ut = tt;
  int k;
  for (k = 0; k < 8; k++)
    ut = tt - swe_deltat_ex_r(c, ut, iflag, NULL);
  return ut;
}

/* ---- (a) 284 ---- */
static void check_a(void)
{
  static const char *MOD[6] = { "0", "0,0,0,1", "0,0,0,2", "0,0,0,3", "0,0,0,4", "0,0,0,5" };
  static const char *NAME[6] = { "the IAU 2000A", "the IAU 1980 ", "IAU 1980 with Herring 1987's",
                                 "the IAU 2000A", "the IAU 2000B", "the Woolard 1953" };
  static const double far[4] = { -1e7, 5e7, -1e9, 1e11 };
  double geo[3] = { 8.55, 47.37, 400 }, xin[3] = { 100, 1, 1 };
  int m, i;
  for (m = 0; m < 6; m++) {
    swe_ctx *c = fresh(MOD[m]);
    double tt[8];
    tt[0] = LO;
    tt[1] = HI;
    tt[2] = nextafter(LO, -INFINITY);
    tt[3] = nextafter(HI, INFINITY);
    for (i = 0; i < 4; i++)
      tt[4 + i] = far[i];
    for (i = 0; i < 8; i++) {
      double t = tt[i], x[6], d[1];
      int in = i < 2, vond = fabs(t - J2K) <= 2000 * 36525.0;
      char star[AS_MAXCH];
      int32 r;
      /* SE_ECL_NUT */
      memset(x, 7, sizeof x);
      serr[0] = '\0';
      r = swe_calc_r(c, t, SE_ECL_NUT, SEFLG_MOSEPH, x, serr);
      tally(in ? r >= 0 && isfinite(x[2]) && x[2] != 0 : nut_refused(r, x, 6, NAME[m]),
            in ? "(a) SE_ECL_NUT not answered" : "(a) SE_ECL_NUT not refused", t, serr);
      serr[0] = '\0';
      r = swe_calc_r(c, t, SE_ECL_NUT, SEFLG_MOSEPH | SEFLG_NONUT, x, serr);
      tally(r >= 0 && isfinite(x[1]) && x[2] == 0, "(a) SE_ECL_NUT SEFLG_NONUT not answered", t, serr);
      /* the true ayanamsa */
      d[0] = 7;
      serr[0] = '\0';
      r = swe_get_ayanamsa_ex_r(c, t, SEFLG_MOSEPH, d, serr);
      /* past Vondrak 2011's precession its range is named first (bug 212) */
      tally(in ? r >= 0 && isfinite(d[0]) : vond ? nut_refused(r, d, 1, NAME[m])
            : r == ERR && d[0] == 0 && strstr(serr, "precession model's range") != NULL,
            in ? "(a) ayanamsa not answered" : "(a) ayanamsa not refused", t, serr);
      serr[0] = '\0';
      r = swe_get_ayanamsa_ex_r(c, t, SEFLG_MOSEPH | SEFLG_NONUT, d, serr);
      tally(vond ? r >= 0 && isfinite(d[0]) : r == ERR && strstr(serr, "precession model's range") != NULL,
            "(a) ayanamsa SEFLG_NONUT not as Vondrak 2011's range has it", t, serr);
      /* a star with no Earth */
      strcpy(star, "Spica");
      serr[0] = '\0';
      r = swe_fixstar2_r(c, star, t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_SPEED, x, serr);
      tally(in ? r >= 0 && isfinite(x[0]) : nut_refused(r, x, 6, NAME[m]),
            in ? "(a) barycentric star not answered" : "(a) barycentric star not refused", t, serr);
      strcpy(star, "Spica");
      serr[0] = '\0';
      r = swe_fixstar2_r(c, star, t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_NONUT, x, serr);
      tally(r >= 0 && isfinite(x[0]), "(a) barycentric star SEFLG_NONUT not answered", t, serr);
      strcpy(star, "Spica");
      serr[0] = '\0';
      r = swe_fixstar_r(c, star, t, SEFLG_MOSEPH | SEFLG_HELCTR, x, serr);
      tally(in ? r >= 0 && isfinite(x[0]) : nut_refused(r, x, 6, NAME[m]),
            in ? "(a) heliocentric star not answered" : "(a) heliocentric star not refused", t, serr);
    }
    /* the UT functions: TT 1e-4 day inside and past each end, and far */
    for (i = 0; i < 8; i++) {
      double tt0 = i < 4 ? (i & 1 ? HI : LO) + (i & 1 ? 1 : -1) * (i < 2 ? -1e-4 : 1e-4) : far[i - 4];
      int in = i < 2;
      double ud = i < 4 ? ut_of(c, tt0, -1) : tt0, um = i < 4 ? ut_of(c, tt0, SEFLG_MOSEPH) : tt0;
      double v, cu[13], a[10], cs[13], as[10], xaz[3];
      int32 r;
      v = swe_sidtime_r(c, ud);
      tally(in ? v >= 0 && v < 24 : isnan(v), in ? "(a) swe_sidtime() not answered"
            : "(a) swe_sidtime() not NaN", ud, "");
      memset(cu, 7, sizeof cu);
      memset(a, 7, sizeof a);
      memset(cs, 7, sizeof cs);
      serr[0] = '\0';
      r = swe_houses_ex2_r(c, um, SEFLG_MOSEPH | SEFLG_SPEED, 47.37, 8.55, 'P', cu, a, cs, as, serr);
      tally(in ? r == OK && isfinite(cs[1]) : nut_refused(r, cu, 13, NAME[m]) && zeros(cs, 13) && zeros(a, 10),
            in ? "(a) houses not answered" : "(a) houses not refused", um, serr);
      serr[0] = '\0';
      r = swe_houses_ex2_r(c, um, SEFLG_MOSEPH | SEFLG_NONUT | SEFLG_SPEED, 10, 8.55, 'P', cu, a, cs, as, serr);
      tally(r == OK && isfinite(cu[1]) && isfinite(cs[1]), "(a) houses SEFLG_NONUT not answered", um, serr);
      memset(cu, 7, sizeof cu);
      r = swe_houses_r(c, ud, 47.37, 8.55, 'P', cu, a);
      tally(in ? r == OK && isfinite(cu[1]) : r == ERR && zeros(cu, 13),
            in ? "(a) swe_houses() not answered" : "(a) swe_houses() not refused", ud, "");
      swe_azalt_r(c, ud, SE_EQU2HOR, geo, 0, 10, xin, xaz);
      tally(in ? isfinite(xaz[0]) && isfinite(xaz[1]) : isnan(xaz[0]) && isnan(xaz[1]) && isnan(xaz[2]),
            in ? "(a) swe_azalt() not answered" : "(a) swe_azalt() not NaN", ud, "");
    }
    swe_ctx_free(c);
  }
  {
    /* SEFLG_JPLHOR's IAU 1980 before 1962 under SEFLG_JPLHOR_APPROX */
    swe_ctx *c = fresh("0");
    double x[6];
    int32 r;
    serr[0] = '\0';
    r = swe_calc_r(c, nextafter(LO, -INFINITY), SE_ECL_NUT, SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, x, serr);
    tally(nut_refused(r, x, 6, "SEFLG_JPLHOR's IAU 1980"), "(a) SE_ECL_NUT SEFLG_JPLHOR_APPROX not refused",
          nextafter(LO, -INFINITY), serr);
    swe_ctx_free(c);
  }
  printf("  (a) 284, the nutation's range: %d of %d as expected\n", nall - nbad, nall);
}

/* ---- (b) 283 ---- */
static void check_b(void)
{
  static const char *MOD[4] = { "0,8,0,0,0,0,0,3", "0,2,0,0,0,0,0,2", "0,2,0,0,0,0,0,1",
                                "0,10,0,0,0,0,0,1" };
  int m, i, n0 = nall, b0 = nbad;
  for (m = 0; m < 4; m++) {
    swe_ctx *c = fresh(MOD[m]);
    double lo = m == 3 ? -4853455.5 : LO, hi = m == 3 ? 9756544.5 : HI;
    for (i = 0; i < 7; i++) {
      double tt = i < 4 ? (i & 1 ? hi : lo) + (i & 1 ? 1 : -1) * (i < 2 ? -1e-4 : 1e-4)
                  : J2K + (i == 4 ? -400 : i == 5 ? 400 : 1000) * 36525.0;
      double ut = ut_of(c, tt, -1), x[6], v;
      int32 r;
      tt = ut + swe_deltat_ex_r(c, ut, -1, NULL);	/* the sidereal time's own */
      serr[0] = '\0';
      r = swe_calc_r(c, tt, SE_ECL_NUT, SEFLG_MOSEPH, x, serr);
      v = swe_sidtime_r(c, ut);
      tally(r == ERR ? isnan(v) : v >= 0 && v < 24, r == ERR ? "(b) swe_sidtime() not NaN where SE_ECL_NUT refuses"
            : "(b) swe_sidtime() not answered where SE_ECL_NUT answers", ut, serr);
    }
    swe_ctx_free(c);
  }
  {
    swe_ctx *c = fresh("0,0,0,0,0,0,0,3");
    double v = swe_sidtime_r(c, 1e12);
    tally(isnan(v), "(b) swe_sidtime() at UT 1e12, TT past the bound, not NaN", 1e12, "");
    swe_ctx_free(c);
  }
  printf("  (b) 283, the formula sidereal times' obliquity: %d of %d as expected\n",
         nall - n0 - (nbad - b0), nall - n0);
}

/* ---- (c) 285 ---- */
static double worst_c, worst_cs;
static void check_c(void)
{
  /* the window's edge, the models, and the reference sets whose chosen
   * model is the edge's inside and outside the window throughout */
  static const char *MOD[3] = { "0,9,1", "0,9,1", "0,9,8" };
  static const char *RIN[3] = { "0,1,1", "0,1,1", "0,8,8" }, *ROUT = "0,9,9";
  static const double TB[3] = { J2K - 73050.0, J2K - 73050.0, J2K - 75 * 36525.0 };
  static const int32 EPH[3] = { SEFLG_SWIEPH, SEFLG_MOSEPH, SEFLG_SWIEPH };
  int m, k, j, n0 = nall, b0 = nbad;
  double wm[3] = { 0, 0, 0 }, wms[3] = { 0, 0, 0 };
  for (m = 0; m < 3; m++) {
    swe_ctx *c = fresh(MOD[m]), *ri = fresh(RIN[m]), *ro = fresh(ROUT);
    /* the span where TT less its ephemeris's delta t plus the default's
     * lies on the window's other side */
    double w = swe_deltat_ex_r(c, TB[m], EPH[m], NULL) - swe_deltat_ex_r(c, TB[m], -1, NULL);
    swe_set_topo_r(c, 8.55, 47.37, 400);
    swe_set_topo_r(ri, 8.55, 47.37, 400);
    swe_set_topo_r(ro, 8.55, 47.37, 400);
    for (k = 0; k < 12; k++) {
      /* 8 within the span, 2 before and 2 after it */
      double f = k < 8 ? (k + 0.5) / 8 : k < 10 ? -0.5 - (k - 8) : 1.5 + (k - 10);
      double t = TB[m] + f * w, x[2][6], p[2][6], e = 0, es = 0;
      char st[AS_MAXCH];
      int32 fl = EPH[m] | SEFLG_TOPOCTR | SEFLG_SPEED;
      for (j = 0; j < 2; j++) {
        swe_ctx *u = j == 0 ? c : t >= TB[m] ? ri : ro;
        swe_calc_r(u, t, SE_MOON, fl, x[j], serr);
        strcpy(st, "Polaris");
        swe_fixstar2_r(u, st, t, fl | SEFLG_EQUATORIAL, p[j], serr);
      }
      /* the Moon's place, which the site moves by its parallax, and
       * Polaris's speed, by the site's rate (the Moon's speed takes the
       * site's acceleration over -+2^-6 day, across the window's edge in
       * either context; Polaris's place on Moshier the Sun's light-time
       * instant's frame, 3e-9 deg apart: neither is the site's model) */
      for (j = 0; j < 2; j++) {
        e = fmax(e, fabs(x[0][j] - x[1][j]));
        es = fmax(es, fabs(p[0][3 + j] - p[1][3 + j]));
      }
      if (e > worst_c)
        worst_c = e;
      if (es > worst_cs)
        worst_cs = es;
      if (e > wm[m])
        wm[m] = e;
      if (es > wms[m])
        wms[m] = es;
      tally(e <= 1e-10, "(c) the topocentric Moon off its TT's model's site", t, MOD[m]);
      tally(es <= 1e-8, "(c) Polaris's topocentric speed off its TT's model's site", t, MOD[m]);
    }
    swe_ctx_free(c);
    swe_ctx_free(ri);
    swe_ctx_free(ro);
  }
  printf("  (c) 285, a site's sidereal time of its call's TT: %d of %d within 1e-10 deg, 1e-8 deg/day"
         " (worst %.2g deg, %.2g deg/day)\n", nall - n0 - (nbad - b0), nall - n0, worst_c, worst_cs);
  for (m = 0; m < 3; m++)
    printf("      %s at %s %s: the Moon %.3g deg, Polaris's speed %.3g deg/day\n", MOD[m],
           m < 2 ? "- 2 cy" : "- 75 cy", EPH[m] == SEFLG_SWIEPH ? "Swiss" : "Moshier", wm[m], wms[m]);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: nutrange EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  printf("G178: the nutation within its models' range, the sidereal time's obliquity within its"
         " model's, a site's sidereal time of its call's TT\n");
  check_a();
  check_b();
  check_c();
  printf(nbad ? "G178 FAIL: %d\n" : "G178 PASS\n", nbad);
  return nbad ? 1 : 0;
}
