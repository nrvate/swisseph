/* farrange.c -- G161: the ayanamsa, the houses and delta t refuse the
 * dates past |J| 1e12 days and the ones that are no dates, as SE_ECL_NUT
 * does (G152); the UT functions refuse a date delta t has no value for;
 * a polynomial precession model answers within its own range only, and
 * Vondrak 2011's precession, the ayanamsa's, within its fit's -+2000
 * centuries.
 *
 * The defects (UPSTREAM-BUGS 212, 213, 215, 217), on Moshier:
 * - 212: swe_get_ayanamsa_ex() and its _ut form answered OK flags with
 *   121.4 deg at 1e12 (Lahiri), -1257 at 1e13, 1.17e10 deg at 1e20, NaN at
 *   1e84, -+1e300, -+inf and NaN; a user t0 of NaN gave NaN at J2000, and
 *   the sidereal positions with it; and on the default Vondrak 2011 past
 *   -+2000 centuries, from matrices that stop being rotations (0.14 off
 *   orthonormal at -7300 centuries): 33.2 deg at J2000 + 1e4 centuries.
 * - 213: the houses took the obliquity and nutation at their TT as no
 *   SE_ECL_NUT would: at UT 9e11 (TT 1.13e12) Placidus at 50 N ERR "within
 *   polar circle" with its cusps filled; under Laskar 1986 at UT -1e8 the
 *   same; sidereal at UT 1e9 the ayanamsa past Vondrak's precession.
 * - 215: swe_deltat_ex() answered 0 at NaN with no message, inf or NaN at
 *   -+1e300 and -+inf, Stephenson's parabola past 1e12 (2.8e27 days at
 *   1e20); and the UT functions computed at a NaN TT.
 * - 217: the mean obliquity of every precession model but Vondrak 2011
 *   is a polynomial; SE_ECL_NUT answered 1.6e51 deg at 1e12 under Laskar
 *   1986, 1.6e21 at -+1e9, 2.1e6 at -1e8 under IAU 2006, -3.0e9 at 1e8
 *   under Owen 1990, 2.67e10 at -1e8 under SEFLG_JPLHOR_APPROX (Owen 1990
 *   before 1799); the houses and the ayanamsa took the same.
 *
 * Check (a), 212: swe_get_ayanamsa_ex() and swe_get_ayanamsa_ex_ut() at 15
 * dates (|J| the next double past 1e12, 1e13, 1e20, 1e84, 1e300, DBL_MAX
 * and inf, either sign, and NaN; the _ut form at UT 1e12 too, TT 1.28e12)
 * in five sidereal modes (Fagan/Bradley, Lahiri, True Citra, Wilhelm's
 * galactic centre, a user mode) and two flag sets: ERR, the ayanamsa 0
 * (filled with 7 before), serr naming the range; swe_get_ayanamsa() and
 * its _ut form NaN. A user t0 of NaN, -+inf, -+1e13 (TT), and 1e13 and NaN
 * in UT: the ayanamsa at J2000 refused, NaN without an error return, a
 * sidereal Mars and sidereal houses refused, their outputs 0.
 *
 * Check (b), 213: swe_houses_ex2() (Placidus, Koch, Gauquelin; with and
 * without the speeds; tropical and sidereal), swe_houses() and
 * swe_houses_ex() at the 15 dates and at UT 1e12 and 8.2e11 (TT past
 * 1e12), and sidereal at UT -1e12 (TT -7.2e11, past Vondrak's precession):
 * ERR, cusp[0..12] or [0..36], ascmc[0..9] and the speeds 0 (filled with
 * 7), swe_houses_ex2()'s serr naming the range; and tropical at UT -1e12
 * with the speeds, whose instants reach past delta t's bound: answered,
 * every output finite.
 *
 * Check (c), 215: swe_deltat_ex() at the 15 dates NaN with serr naming
 * delta t's range, with a user delta t too; swe_deltat() NaN; at -+1e12
 * and the doubles next inside finite and no serr. And the UT functions at
 * the next double past 1e12, 1e13, 1e300, -1e300, -+inf and NaN, where
 * delta t is NaN: swe_calc_ut() (the Sun on Moshier and the Swiss files,
 * the Moon, the mean node, Cupido, SE_ECL_NUT), swe_fixstar_ut(),
 * swe_fixstar2_ut(), swe_pheno_ut(), swe_nod_aps_ut() (mean and
 * osculating), swe_get_ayanamsa_ex_ut(), swe_sol_eclipse_where(),
 * swe_sol_eclipse_how(), swe_lun_eclipse_how(), swe_lun_occult_where(),
 * swe_rise_trans() (rising, meridian transit), swe_gauquelin_sector()
 * (method 0), swe_time_equ(), swe_lmt_to_lat(), swe_lat_to_lmt(),
 * swe_heliacal_pheno_ut(): ERR, not computed at a NaN TT (which crashed
 * the mean node, the eclipse functions, the meridian transit and
 * the time equation).
 *
 * Check (d), 217: SE_ECL_NUT under each of the ten polynomial models at
 * the widest body range's ends (-3100015.5, 8000016.5), at Owen 1990's
 * pieces' ends (-4853455.5, 9756544.5) under it, and at -+1e12 under the
 * default and Vondrak 2011 named: OK, the mean obliquity within 22..25 deg
 * at the range's ends (Vondrak at 1e12 not); the doubles next outside,
 * -+1e9 and -+1e12: refused, serr naming the model's range. The same with
 * SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX (Owen 1990 before 1799) under the
 * default, its obliquity Vondrak's after 1962 (answered at 1e9), and
 * SEMOD_JPLHORA_2 (IAU 1976's obliquity at every date). The ayanamsa and
 * the houses under Laskar 1986 at the range's lower end answered and the
 * double next outside refused. And 64 instants across each model's range:
 * the mean obliquity within 22..25 deg. Vondrak 2011's precession: the
 * ayanamsa at J2000 -+ 2000 centuries answered, the doubles next outside,
 * J2000 - 1e4 centuries and 1e9 refused (swe_get_ayanamsa() NaN), a t0
 * past it refused; Equal houses at 50 N with speeds at the UT whose TT is
 * 0.001 day inside and outside each end: tropical answered, sidereal
 * answered inside, its cusp speed within 1e-3 deg/day of the tropical one
 * (the stencil on t's side; across the end the refused ayanamsa's 0 put
 * it 2.0e3 to 5.6e3 deg/day off), refused outside; tropical at UT 1e9
 * answered (Vondrak's obliquity); and at Laskar 1986's lower end the
 * sidereal cusp speed as at Vondrak's.
 *
 * Check (e): a refusal leaves nothing behind: SE_ECL_NUT, the ayanamsa and
 * the houses at J2000 after refusals at 1e13, -1e300 and NaN on the same
 * context against a fresh context, bit for bit.
 *
 * Each check runs in a child process, 60 s at most, so that a crash or a
 * hang before the fix is a failure, not the end of the run.
 *
 * Measured: (a) 648 of 648 refused, (b) 318 of 318 (tropical at UT -1e12
 * answered), (c) 219 of 219 and 4 of 4 inside finite, (d) 788 of 788 as
 * expected (the mean obliquity within the ranges 22.3521 .. 24.3540 deg),
 * (e) 0 values differ. Before the fix (ts20 0eccce8, -O1): (a) killed by
 * SIGABRT (the True Citra star's range message past its buffer, entry
 * 210, which the refusal now precedes), (b) 273 of 318, (c) killed by
 * SIGSEGV, (d) 694 of 788, (e) passes. The upstream build fails the same:
 * (a) SIGABRT, (b) 3 of 318, (c) SIGSEGV, (d) 694 of 788. Run as
 *   farrange EPHEDIR
 */
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "swephexp.h"

#define JMAX 1e12
static const char *EPHE;
static double D[16];	/* the 15 dates */
static char serr[AS_MAXCH];

static void fill(double *x, int n)
{
  int i;
  for (i = 0; i < n; i++)
    x[i] = 7;
}

static int zeros(const double *x, int n)
{
  int i;
  for (i = 0; i < n; i++)
    if (x[i] != 0)
      return 0;
  return 1;
}

/* a range's refusal, or bug 211's of a non-finite date ("invalid date") */
static int named(const char *s)
{
  return (strstr(s, "outside the") != NULL && strstr(s, "range") != NULL)
         || strstr(s, "invalid date") != NULL;
}

static swe_ctx *ctx_new(const char *mod)
{
  swe_ctx *c = swe_ctx_new();
  char m[40];
  swe_set_ephe_path_r(c, EPHE);
  if (mod != NULL) {
    strcpy(m, mod);
    swe_set_astro_models_r(c, m, 0);
  }
  return c;
}

static int n_all, n_bad;
static void tally(int ok, const char *what, double t)
{
  n_all++;
  if (!ok && n_bad++ < 6)
    printf("    not refused: %s at %.17g: '%s'\n", what, t, serr);
}

/* (a) */
static int check_a(void)
{
  static const int32 mode[5] = { SE_SIDM_FAGAN_BRADLEY, SE_SIDM_LAHIRI, SE_SIDM_TRUE_CITRA,
                                 SE_SIDM_GALCENT_MULA_WILHELM, SE_SIDM_USER };
  static const int32 fl[2] = { SEFLG_MOSEPH, SEFLG_SWIEPH };
  static const double t0far[7] = { NAN, INFINITY, -INFINITY, 1e13, -1e13, 1e13, NAN };
  int i, m, f;
  n_all = n_bad = 0;
  for (m = 0; m < 5; m++)
    for (f = 0; f < 2; f++) {
      swe_ctx *c = ctx_new(NULL);
      swe_set_sid_mode_r(c, mode[m], 2415020.5, 22.5);
      for (i = 0; i < 16; i++) {
        double d[2], t = i < 15 ? D[i] : 1e12;
        int32 r;
        if (i < 15) {
          fill(d, 1);
          serr[0] = '\0';
          r = swe_get_ayanamsa_ex_r(c, t, fl[f], d, serr);
          tally(r == ERR && d[0] == 0 && named(serr), "swe_get_ayanamsa_ex()", t);
          d[0] = swe_get_ayanamsa_r(c, t);
          tally(isnan(d[0]), "swe_get_ayanamsa()", t);
        }
        fill(d, 1);
        serr[0] = '\0';
        r = swe_get_ayanamsa_ex_ut_r(c, t, fl[f], d, serr);
        tally(r == ERR && d[0] == 0 && named(serr), "swe_get_ayanamsa_ex_ut()", t);
        d[0] = swe_get_ayanamsa_ut_r(c, t);
        tally(isnan(d[0]), "swe_get_ayanamsa_ut()", t);
      }
      swe_ctx_free(c);
    }
  for (i = 0; i < 7; i++) {
    swe_ctx *c = ctx_new(NULL);
    double d[2], x[6], cu[13], a[10];
    int32 r;
    swe_set_sid_mode_r(c, SE_SIDM_USER | (i >= 5 ? SE_SIDBIT_USER_UT : 0), t0far[i], 0);
    fill(d, 1);
    serr[0] = '\0';
    r = swe_get_ayanamsa_ex_r(c, 2451545.0, SEFLG_MOSEPH, d, serr);
    tally(r == ERR && d[0] == 0 && named(serr), "the ayanamsa of t0", t0far[i]);
    tally(isnan(swe_get_ayanamsa_r(c, 2451545.0)), "swe_get_ayanamsa() of t0", t0far[i]);
    fill(x, 6);
    serr[0] = '\0';
    r = swe_calc_r(c, 2451545.0, SE_MARS, SEFLG_MOSEPH | SEFLG_SIDEREAL | SEFLG_SPEED, x, serr);
    tally(r == ERR && zeros(x, 6) && named(serr), "sidereal Mars of t0", t0far[i]);
    fill(cu, 13);
    fill(a, 10);
    serr[0] = '\0';
    r = swe_houses_ex2_r(c, 2451545.0, SEFLG_MOSEPH | SEFLG_SIDEREAL, 50, 10, 'P', cu, a, NULL, NULL, serr);
    tally(r == ERR && zeros(cu, 13) && zeros(a, 10) && named(serr), "sidereal houses of t0", t0far[i]);
    swe_ctx_free(c);
  }
  printf("  (a) the ayanamsa past |J| 1e12, -+inf, NaN, and of such a t0: %d of %d refused\n",
         n_all - n_bad, n_all);
  return n_bad != 0;
}

/* (b) */
static int check_b(void)
{
  static const int hs[3] = { 'P', 'K', 'G' };
  double tq[19];
  int i, h, sp, sid;
  n_all = n_bad = 0;
  for (i = 0; i < 15; i++)
    tq[i] = D[i];
  tq[15] = 1e12;
  tq[16] = 8.2e11;
  tq[17] = -JMAX;	/* TT inside, the speeds' instants not */
  for (h = 0; h < 3; h++)
    for (sid = 0; sid < 2; sid++) {
      swe_ctx *c = ctx_new(NULL);
      int ito = hs[h] == 'G' ? 36 : 12;
      int32 fl = SEFLG_MOSEPH | (sid ? SEFLG_SIDEREAL : 0);
      swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
      for (i = 0; i < 18; i++)
        for (sp = 0; sp < 2; sp++) {
          double cu[37], a[10], cs[37], as[10];
          int32 r;
          if (i == 17 && !sid) {
            /* answered, the speeds from delta t and the sidereal time at
             * t -+ 1/64 day past the bound, not NaN; sidereal, past
             * Vondrak 2011's precession, refused below */
            int k, fin = 1;
            if (!sp)
              continue;
            fill(cu, 37);
            fill(a, 10);
            fill(cs, 37);
            fill(as, 10);
            serr[0] = '\0';
            swe_houses_ex2_r(c, tq[i], fl, 50, 10, hs[h], cu, a, cs, as, serr);
            for (k = 0; k <= ito; k++)
              fin &= isfinite(cu[k]) && isfinite(cs[k]);
            for (k = 0; k < 10; k++)
              fin &= isfinite(a[k]) && isfinite(as[k]);
            n_all++;
            if (!fin || cu[1] == 7 || strstr(serr, "outside the") != NULL) {
              n_bad++;
              printf("    houses at UT %.17g (TT inside) not answered: '%s'\n", tq[i], serr);
            }
            continue;
          }
          fill(cu, 37);
          fill(a, 10);
          fill(cs, 37);
          fill(as, 10);
          serr[0] = '\0';
          r = swe_houses_ex2_r(c, tq[i], fl, 50, 10, hs[h], cu, a, sp ? cs : NULL, sp ? as : NULL, serr);
          tally(r == ERR && zeros(cu, ito + 1) && zeros(a, 10) && named(serr)
                && (!sp || (zeros(cs, ito + 1) && zeros(as, 10))), "swe_houses_ex2()", tq[i]);
          if (sp)
            continue;
          fill(cu, 37);
          fill(a, 10);
          if (!sid) {
            r = swe_houses_r(c, tq[i], 50, 10, hs[h], cu, a);
            tally(r == ERR && zeros(cu, ito + 1) && zeros(a, 10), "swe_houses()", tq[i]);
          } else {
            r = swe_houses_ex_r(c, tq[i], fl, 50, 10, hs[h], cu, a);
            tally(r == ERR && zeros(cu, ito + 1) && zeros(a, 10), "swe_houses_ex()", tq[i]);
          }
        }
      swe_ctx_free(c);
    }
  printf("  (b) the houses past |J| 1e12, -+inf, NaN, their TT past 1e12: %d of %d refused"
         " (tropical at UT -1e12 answered)\n", n_all - n_bad, n_all);
  return n_bad != 0;
}

/* (c) */
static int check_c(void)
{
  static const int ut[7] = { 0, 2, 8, 9, 12, 13, 14 };	/* indices into D */
  double tin[4] = { JMAX, -JMAX, 0, 0 }, geo[3] = { 10, 50, 0 };
  double datm[4] = { 1013.25, 15, 40, 0 }, dobs[6] = { 36, 1, 0, 0, 0, 0 };
  int i, k, nin = 0, binin = 0;
  swe_ctx *c = ctx_new(NULL);
  n_all = n_bad = 0;
  tin[2] = nextafter(JMAX, 0);
  tin[3] = -tin[2];
  for (k = 0; k < 2; k++) {
    if (k == 1)
      swe_set_delta_t_userdef_r(c, 0.0008);
    for (i = 0; i < 15; i++) {
      double d;
      serr[0] = '\0';
      d = swe_deltat_ex_r(c, D[i], SEFLG_MOSEPH, serr);
      tally(isnan(d) && strstr(serr, "outside the delta t range") != NULL, "swe_deltat_ex()", D[i]);
      tally(isnan(swe_deltat_r(c, D[i])), "swe_deltat()", D[i]);
    }
    swe_set_delta_t_userdef_r(c, SE_DELTAT_AUTOMATIC);
  }
  for (i = 0; i < 4; i++) {
    double d;
    serr[0] = '\0';
    d = swe_deltat_ex_r(c, tin[i], SEFLG_MOSEPH, serr);
    nin++;
    if (!isfinite(d) || serr[0] != '\0' || !isfinite(swe_deltat_r(c, tin[i]))) {
      binin++;
      printf("    delta t at %.17g: %g '%s'\n", tin[i], d, serr);
    }
  }
  for (i = 0; i < 7; i++) {
    static const int B[6] = { SE_SUN, SE_SUN, SE_MOON, SE_MEAN_NODE, SE_CUPIDO, SE_ECL_NUT };
    double t = D[ut[i]], x[24], a[20], tr;
    char st[40];
    int32 r;
    for (k = 0; k < 6; k++) {
      fill(x, 6);
      serr[0] = '\0';
      r = swe_calc_ut_r(c, t, B[k], k == 1 ? SEFLG_SWIEPH : SEFLG_MOSEPH, x, serr);
      tally(r == ERR && zeros(x, 6), "swe_calc_ut()", t);
    }
    strcpy(st, "Spica");
    fill(x, 6);
    r = swe_fixstar_ut_r(c, st, t, SEFLG_MOSEPH, x, serr);
    tally(r == ERR && zeros(x, 6), "swe_fixstar_ut()", t);
    strcpy(st, "Spica");
    fill(x, 6);
    r = swe_fixstar2_ut_r(c, st, t, SEFLG_MOSEPH, x, serr);
    tally(r == ERR && zeros(x, 6), "swe_fixstar2_ut()", t);
    fill(x, 20);
    r = swe_pheno_ut_r(c, t, SE_MARS, SEFLG_MOSEPH, x, serr);
    tally(r == ERR && zeros(x, 20), "swe_pheno_ut()", t);
    for (k = SE_NODBIT_MEAN; k <= SE_NODBIT_OSCU; k++) {
      fill(x, 24);
      r = swe_nod_aps_ut_r(c, t, SE_MARS, SEFLG_MOSEPH, k, x, x + 6, x + 12, x + 18, serr);
      tally(r == ERR && zeros(x, 24), "swe_nod_aps_ut()", t);
    }
    serr[0] = '\0';
    tally(swe_sol_eclipse_where_r(c, t, SEFLG_MOSEPH, x, a, serr) == ERR && named(serr),
          "swe_sol_eclipse_where()", t);
    serr[0] = '\0';
    tally(swe_sol_eclipse_how_r(c, t, SEFLG_MOSEPH, geo, a, serr) == ERR && named(serr),
          "swe_sol_eclipse_how()", t);
    serr[0] = '\0';
    tally(swe_lun_eclipse_how_r(c, t, SEFLG_MOSEPH, geo, a, serr) == ERR && named(serr),
          "swe_lun_eclipse_how()", t);
    serr[0] = '\0';
    tally(swe_lun_occult_where_r(c, t, SE_MARS, NULL, SEFLG_MOSEPH, x, a, serr) == ERR && named(serr),
          "swe_lun_occult_where()", t);
    serr[0] = '\0';
    tally(swe_rise_trans_r(c, t, SE_SUN, NULL, SEFLG_MOSEPH, SE_CALC_RISE, geo, 1013.25, 15, &tr, serr) == ERR
          && named(serr), "swe_rise_trans()", t);
    serr[0] = '\0';
    tally(swe_rise_trans_r(c, t, SE_SUN, NULL, SEFLG_MOSEPH, SE_CALC_MTRANSIT, geo, 1013.25, 15, &tr, serr) == ERR
          && named(serr), "swe_rise_trans() transit", t);
    serr[0] = '\0';
    fill(x, 1);
    tally(swe_gauquelin_sector_r(c, t, SE_MARS, NULL, SEFLG_MOSEPH, 0, geo, 0, 0, x, serr) == ERR
          && x[0] == 0 && named(serr), "swe_gauquelin_sector()", t);
    serr[0] = '\0';
    fill(x, 1);
    tally(swe_time_equ_r(c, t, x, serr) == ERR && x[0] == 0 && named(serr), "swe_time_equ()", t);
    /* not at the next double past 1e12: their instant less the longitude's
     * 10/360 day is inside, where the sidereal time takes minutes (bug 209) */
    if (i > 0) {
      serr[0] = '\0';
      tally(swe_lmt_to_lat_r(c, t, 10, x, serr) == ERR && named(serr), "swe_lmt_to_lat()", t);
      serr[0] = '\0';
      tally(swe_lat_to_lmt_r(c, t, 10, x, serr) == ERR && named(serr), "swe_lat_to_lmt()", t);
    }
    strcpy(st, "Venus");
    serr[0] = '\0';
    tally(swe_heliacal_pheno_ut_r(c, t, geo, datm, dobs, st, SE_HELIACAL_RISING, SEFLG_MOSEPH, a, serr) == ERR,
          "swe_heliacal_pheno_ut()", t);
    fill(x, 1);
    serr[0] = '\0';
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    tally(swe_get_ayanamsa_ex_ut_r(c, t, SEFLG_MOSEPH, x, serr) == ERR && x[0] == 0 && named(serr),
          "swe_get_ayanamsa_ex_ut()", t);
  }
  swe_ctx_free(c);
  printf("  (c) delta t past |J| 1e12, -+inf, NaN, and the UT functions there: %d of %d refused;"
         " -+1e12 and next inside: %d of %d finite\n", n_all - n_bad, n_all, nin - binin, nin);
  return n_bad != 0 || binin != 0;
}

/* (d) */
static double eps_lo = 99, eps_hi = 0;
static int ecl(swe_ctx *c, double t, int32 fl, int want, int band)
{
  double x[6];
  int32 r;
  fill(x, 6);
  serr[0] = '\0';
  r = swe_calc_r(c, t, SE_ECL_NUT, fl, x, serr);
  n_all++;
  if (want) {
    if (r < 0 || !isfinite(x[1]) || (band && !(x[1] >= 22 && x[1] <= 25))) {
      if (n_bad++ < 8)
        printf("    not answered in 22..25: J %.17g flags %d: %d, %.10g '%s'\n", t, fl, r, x[1], serr);
      return 1;
    }
    if (band) {
      if (x[1] < eps_lo)
        eps_lo = x[1];
      if (x[1] > eps_hi)
        eps_hi = x[1];
    }
    return 0;
  }
  if (r != ERR || !zeros(x, 6) || strstr(serr, "range") == NULL) {
    if (n_bad++ < 8)
      printf("    not refused: J %.17g flags %d: %d, %.10g '%s'\n", t, fl, r, x[1], serr);
    return 1;
  }
  return 0;
}

static int vondrak(const char *s)
{
  return strstr(s, "Vondr") != NULL && strstr(s, "precession model's range -70598455 .. 75501545") != NULL;
}

/* the UT whose TT is tt */
static double ut_of_tt(swe_ctx *c, double tt)
{
  double u = tt;
  int k;
  for (k = 0; k < 8; k++)
    u = tt - swe_deltat_ex_r(c, u, SEFLG_MOSEPH, NULL);
  return u;
}

static int check_d(void)
{
  static const double LO = -3100015.5, HI = 8000016.5, OLO = -4853455.5, OHI = 9756544.5;
  static const int poly[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 10, 11 };
  char mod[40];
  int p, i, s;
  n_all = n_bad = 0;
  for (p = 0; p < 10; p++) {
    double lo = poly[p] == 10 ? OLO : LO, hi = poly[p] == 10 ? OHI : HI;
    swe_ctx *c;
    snprintf(mod, sizeof mod, "0,%d,0,0", poly[p]);
    c = ctx_new(mod);
    ecl(c, lo, SEFLG_MOSEPH, 1, 1);
    ecl(c, hi, SEFLG_MOSEPH, 1, 1);
    ecl(c, nextafter(lo, -INFINITY), SEFLG_MOSEPH, 0, 0);
    ecl(c, nextafter(hi, INFINITY), SEFLG_MOSEPH, 0, 0);
    for (s = -1; s <= 1; s += 2) {
      ecl(c, s * 1e9, SEFLG_MOSEPH, 0, 0);
      ecl(c, s * JMAX, SEFLG_MOSEPH, 0, 0);
      ecl(c, s * JMAX, SEFLG_SWIEPH, 0, 0);
    }
    for (i = 0; i <= 63; i++)
      ecl(c, lo + (hi - lo) * i / 63.0, SEFLG_MOSEPH, 1, 1);
    swe_ctx_free(c);
  }
  for (p = 0; p <= 9; p += 9) {	/* Vondrak 2011: the default, named */
    swe_ctx *c;
    snprintf(mod, sizeof mod, "0,%d,0,0", p);
    c = ctx_new(mod);
    for (s = -1; s <= 1; s += 2) {
      ecl(c, s * JMAX, SEFLG_MOSEPH, 1, 0);
      ecl(c, s * 1e9, SEFLG_MOSEPH, 1, 0);
    }
    ecl(c, LO, SEFLG_MOSEPH, 1, 1);
    ecl(c, HI, SEFLG_MOSEPH, 1, 1);
    /* SEFLG_JPLHOR_APPROX: Owen 1990 before 1799, Vondrak's after 1962 */
    ecl(c, OLO, SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, 1, 1);
    ecl(c, nextafter(OLO, -INFINITY), SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, 0, 0);
    ecl(c, -1e8, SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, 0, 0);
    ecl(c, 1e9, SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, 1, 0);
    swe_ctx_free(c);
  }
  {
    /* SEMOD_JPLHORA_2: IAU 1976's obliquity at every date */
    swe_ctx *c = ctx_new("0,0,0,0,0,0,2,0");
    ecl(c, HI, SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, 1, 1);
    ecl(c, nextafter(HI, INFINITY), SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, 0, 0);
    ecl(c, nextafter(HI, INFINITY), SEFLG_JPLEPH, 1, 0);
    swe_ctx_free(c);
  }
  {
    /* the ayanamsa and the houses under Laskar 1986 */
    swe_ctx *c = ctx_new("0,2,0,0");
    double d[2], cu[13], a[10];
    int32 r;
    /* the houses' TT: the UT whose TT is 0.01 day inside and outside */
    double ut = LO - swe_deltat_ex_r(c, LO, SEFLG_MOSEPH, serr);
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    for (i = 0; i < 2; i++) {
      double t = i ? nextafter(LO, -INFINITY) : LO, u = i ? ut - 0.01 : ut + 0.01;
      AS_BOOL refd;
      serr[0] = '\0';
      r = swe_get_ayanamsa_ex_r(c, t, SEFLG_MOSEPH, d, serr);
      refd = r == ERR && strstr(serr, "Laskar 1986 precession model's range") != NULL;
      n_all++;
      if (refd != (i == 1)) {
        n_bad++;
        printf("    ayanamsa under Laskar 1986 at %.17g: %d '%s'\n", t, r, serr);
      }
      fill(cu, 13);
      fill(a, 10);
      serr[0] = '\0';
      r = swe_houses_ex2_r(c, u, SEFLG_MOSEPH, 50, 10, 'P', cu, a, NULL, NULL, serr);
      refd = r == ERR && strstr(serr, "Laskar 1986 precession model's range") != NULL
             && zeros(cu, 13) && zeros(a, 10);
      n_all++;
      if (refd != (i == 1)) {
        n_bad++;
        printf("    houses under Laskar 1986 at UT %.17g: %d '%s'\n", u, r, serr);
      }
    }
    swe_ctx_free(c);
  }
  {
    /* Vondrak 2011's precession (the default): the ayanamsa and the
     * sidereal houses within J2000 -+ 2000 centuries; its obliquity, so
     * SE_ECL_NUT (above) and the tropical houses, to 1e12 */
    static const double VLO = 2451545.0 - 73050000.0, VHI = 2451545.0 + 73050000.0;
    double tv[6];
    tv[0] = VLO;
    tv[1] = VHI;
    tv[2] = nextafter(VLO, -INFINITY);
    tv[3] = nextafter(VHI, INFINITY);
    tv[4] = 2451545.0 - 1e4 * 36525.0;
    tv[5] = 1e9;
    for (i = 0; i < 6; i++) {
      swe_ctx *c = ctx_new(NULL);
      double d[2], ad;
      int32 r;
      AS_BOOL refd;
      swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
      fill(d, 1);
      serr[0] = '\0';
      r = swe_get_ayanamsa_ex_r(c, tv[i], SEFLG_MOSEPH, d, serr);
      ad = swe_get_ayanamsa_r(c, tv[i]);
      refd = r == ERR && d[0] == 0 && isnan(ad) && vondrak(serr);
      n_all++;
      if (i < 2 ? (r < 0 || !isfinite(d[0]) || !isfinite(ad)) : !refd) {
        n_bad++;
        printf("    ayanamsa under Vondrak 2011 at %.17g: %d %.10g '%s'\n", tv[i], r, d[0], serr);
      }
      swe_ctx_free(c);
    }
    {
      /* a t0 past it */
      swe_ctx *c = ctx_new(NULL);
      double d[2];
      int32 r;
      swe_set_sid_mode_r(c, SE_SIDM_USER, tv[2], 0);
      fill(d, 1);
      serr[0] = '\0';
      r = swe_get_ayanamsa_ex_r(c, 2451545.0, SEFLG_MOSEPH, d, serr);
      n_all++;
      if (!(r == ERR && d[0] == 0 && vondrak(serr) && strncmp(serr, "t0 ", 3) == 0)) {
        n_bad++;
        printf("    ayanamsa of t0 %.17g under Vondrak 2011: %d '%s'\n", tv[2], r, serr);
      }
      swe_ctx_free(c);
    }
    for (i = 0; i < 2; i++) {
      /* the houses, their TT 0.001 day inside and outside each end */
      int k;
      for (k = 0; k < 2; k++) {
        swe_ctx *c = ctx_new(NULL);
        double edge = i ? VHI : VLO, tt = edge + (i ? -1 : 1) * (k ? -0.001 : 0.001);
        double u = ut_of_tt(c, tt), cu[13], a[10], cs[13], as[10], ct[13], at[10], cst[13], ast[10];
        int32 r, rt;
        swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
        fill(cu, 13);
        fill(a, 10);
        fill(cs, 13);
        fill(as, 10);
        serr[0] = '\0';
        r = swe_houses_ex2_r(c, u, SEFLG_MOSEPH | SEFLG_SIDEREAL, 50, 10, 'E', cu, a, cs, as, serr);
        n_all++;
        if (k ? !(r == ERR && zeros(cu, 13) && zeros(cs, 13) && vondrak(serr)) : r < 0) {
          n_bad++;
          printf("    sidereal houses under Vondrak 2011 at TT %.17g: %d '%s'\n", tt, r, serr);
        }
        serr[0] = '\0';
        rt = swe_houses_ex2_r(c, u, SEFLG_MOSEPH, 50, 10, 'E', ct, at, cst, ast, serr);
        n_all++;
        if (rt < 0) {
          n_bad++;
          printf("    tropical houses under Vondrak 2011 at TT %.17g: %d '%s'\n", tt, rt, serr);
        }
        /* inside: the sidereal speed the tropical one less the ayanamsa's
         * rate, 5e-5 deg/day, not its value over the stencil's step */
        n_all++;
        if (!k && !(fabs(cs[1] - cst[1]) < 1e-3)) {
          n_bad++;
          printf("    sidereal cusp speed at TT %.17g: %.10g, tropical %.10g\n", tt, cs[1], cst[1]);
        }
        swe_ctx_free(c);
      }
    }
    {
      /* the tropical houses at 1e9: Vondrak's obliquity */
      swe_ctx *c = ctx_new(NULL);
      double cu[13], a[10];
      int32 r;
      serr[0] = '\0';
      r = swe_houses_ex2_r(c, 1e9, SEFLG_MOSEPH, 50, 10, 'E', cu, a, NULL, NULL, serr);
      n_all++;
      if (r < 0 || !isfinite(cu[1])) {
        n_bad++;
        printf("    tropical houses at UT 1e9: %d '%s'\n", r, serr);
      }
      swe_ctx_free(c);
    }
  }
  {
    /* the sidereal house speeds at Laskar 1986's lower end, TT 0.001 day
     * inside: on t's side */
    swe_ctx *c = ctx_new("0,2,0,0");
    double u, cu[13], a[10], cs[13], as[10], ct[13], at[10], cst[13], ast[10];
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    u = ut_of_tt(c, LO + 0.001);
    serr[0] = '\0';
    swe_houses_ex2_r(c, u, SEFLG_MOSEPH | SEFLG_SIDEREAL, 50, 10, 'E', cu, a, cs, as, serr);
    swe_houses_ex2_r(c, u, SEFLG_MOSEPH, 50, 10, 'E', ct, at, cst, ast, NULL);
    n_all++;
    if (!(fabs(cs[1] - cst[1]) < 1e-3)) {
      n_bad++;
      printf("    sidereal cusp speed under Laskar 1986 at TT %.17g: %.10g, tropical %.10g '%s'\n",
             LO + 0.001, cs[1], cst[1], serr);
    }
    swe_ctx_free(c);
  }
  printf("  (d) the models' ranges: %d of %d as expected (the mean obliquity within them %.4f .. %.4f deg)\n",
         n_all - n_bad, n_all, eps_lo, eps_hi);
  return n_bad != 0;
}

/* (e) */
static int check_e(void)
{
  static const double tr[3] = { 1e13, -1e300, NAN };
  swe_ctx *c = ctx_new(NULL), *f = ctx_new(NULL);
  double x[2][6], d[2][2], cu[2][13], a[2][10];
  int k, n = 0;
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  swe_set_sid_mode_r(f, SE_SIDM_LAHIRI, 0, 0);
  for (k = 0; k < 3; k++) {
    swe_calc_r(c, tr[k], SE_ECL_NUT, SEFLG_MOSEPH, x[0], serr);
    swe_get_ayanamsa_ex_r(c, tr[k], SEFLG_MOSEPH, d[0], serr);
    swe_houses_ex2_r(c, tr[k], SEFLG_MOSEPH, 50, 10, 'P', cu[0], a[0], NULL, NULL, serr);
    swe_deltat_ex_r(c, tr[k], SEFLG_MOSEPH, serr);
  }
  swe_calc_r(c, 2451545.0, SE_ECL_NUT, SEFLG_MOSEPH, x[0], serr);
  swe_calc_r(f, 2451545.0, SE_ECL_NUT, SEFLG_MOSEPH, x[1], serr);
  swe_get_ayanamsa_ex_r(c, 2451545.0, SEFLG_MOSEPH, d[0], serr);
  swe_get_ayanamsa_ex_r(f, 2451545.0, SEFLG_MOSEPH, d[1], serr);
  swe_houses_ex2_r(c, 2451545.0, SEFLG_MOSEPH, 50, 10, 'P', cu[0], a[0], NULL, NULL, serr);
  swe_houses_ex2_r(f, 2451545.0, SEFLG_MOSEPH, 50, 10, 'P', cu[1], a[1], NULL, NULL, serr);
  n += memcmp(x[0], x[1], sizeof x[0]) != 0;
  n += memcmp(d[0], d[1], sizeof(double)) != 0;
  n += memcmp(cu[0], cu[1], sizeof cu[0]) != 0;
  n += memcmp(a[0], a[1], sizeof a[0]) != 0;
  swe_ctx_free(c);
  swe_ctx_free(f);
  printf("  (e) after refusals, J2000 against a fresh context: %d values differ\n", n);
  return n != 0;
}

int main(int argc, char **argv)
{
  static int (*const chk[5])(void) = { check_a, check_b, check_c, check_d, check_e };
  static const double far[7] = { 0, 1e13, 1e20, 1e84, 1e300, DBL_MAX, INFINITY };
  int i, bad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: farrange EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  for (i = 0; i < 7; i++) {
    double t = i == 0 ? nextafter(JMAX, INFINITY) : far[i];
    D[2 * i] = t;
    D[2 * i + 1] = -t;
  }
  D[14] = NAN;
  printf("G161: the ayanamsa, houses, delta t and UT functions refuse |J| past 1e12; the models their ranges\n");
  for (i = 0; i < 5; i++) {
    pid_t pid;
    int st;
    fflush(stdout);
    if ((pid = fork()) < 0)
      return 2;
    if (pid == 0) {
      int r;
      alarm(60);
      r = chk[i]();
      fflush(stdout);
      _exit(r);
    }
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st)) {
      printf("  (%c) killed by signal %d\n", 'a' + i, WTERMSIG(st));
      bad++;
    } else if (WEXITSTATUS(st) != 0) {
      bad++;
    }
  }
  printf(bad ? "G161 FAIL: %d\n" : "G161 PASS\n", bad);
  return bad ? 1 : 0;
}
