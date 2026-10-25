/* sidhorflag.c -- G164: the JPL Horizons flags reach no sidereal output,
 * and the Gauquelin sectors' frame takes them as the body's place does;
 * SEFLG_SIDEREAL moves no Gauquelin sector; a refused star refuses its
 * ayanamsa, the sidereal places and the sidereal houses alike.
 *
 * The defect (UPSTREAM-BUGS 223): plaus_iflag() drops SEFLG_JPLHOR and
 * SEFLG_JPLHOR_APPROX from every sidereal place, from every request off a
 * JPL file, and from the nodes, apsides and fictitious bodies; the
 * ayanamsa's precession never takes them (swi_get_ayanamsa_ex() keeps the
 * ephemeris bits alone). But swe_get_ayanamsa_ex() and its _ut form added
 * the nutation of the caller's flags, and the sidereal houses of the
 * traditional method subtract that ayanamsa: at 1950, Lahiri, on de200,
 * the Swiss files and Moshier alike, the ayanamsa with the flag 1.72e-5 deg
 * off the one without and the sidereal cusps -1.72e-5, while sidereal Mars
 * did not move: within one chart with the same flags the planets and the
 * cusps 1.72e-5 deg apart, and "tropical less the ayanamsa is sidereal"
 * failing by it wherever the tropical place takes no flag.
 * swe_gauquelin_sector()'s methods 0 and 1 took the frame's obliquity and
 * nutation with the flags as given where the body's call drops them: Mars
 * on the Swiss files 1.49e-6 sector, the mean node on de200 1.57e-6,
 * sidereal Mars 1.41e-6.
 *
 * Check (a), on de200, the Swiss files and Moshier, with
 * SEFLG_JPLHOR_APPROX and with SEFLG_JPLHOR (no IERS corrections here, so
 * fallen to it), at 1800, 1950, 1962 Jan 20 -+ 2^-8 day, 2000 and 2100:
 * each of these the same with the flag as without, values and returns
 * bit for bit -- swe_get_ayanamsa_ex() and _ut(), with and without
 * SEFLG_NONUT (Lahiri, True Citra); sidereal swe_calc() of the Sun, Moon,
 * Mars and both nodes with speed, and sidereal Spica (Lahiri, True Citra,
 * the ecliptic of t0, the solar system plane); sidereal swe_nod_aps() of
 * Mars (Lahiri); swe_houses_ex2() with speeds, Placidus, Koch, equal,
 * Regiomontanus and Makransky's Sunshine at Zurich, in each sidereal
 * method (Lahiri, True Citra, Lahiri on the ecliptic of date, of t0 and the
 * solar system plane); swe_gauquelin_sector() methods 0 and 1 of the
 * bodies whose place takes no flag (sidereal Mars; Mars and Spica off the
 * JPL file; the mean and true nodes). One check per function, setting,
 * ephemeris and flag, over the six dates: 234.
 * Check (b): "tropical less swe_get_ayanamsa_ex() is sidereal" with the
 * same flags (Lahiri), within 1e-10 deg at the six dates: the mean node
 * (no flag reaches it), Mars (on de200, whose tropical place the flag
 * moves, its residual less that movement, tropical(F) - tropical(0)), and
 * the Placidus cusps and Ascendant against the tropical houses. 18
 * checks.
 * Check (c): the Gauquelin sector (method 0) is swe_house_pos() of the
 * body's place in the frame of the Horizons bits its own call returned
 * (SE_ECL_NUT's obliquity and nutation, the sidereal time with them),
 * within 1e-11 sector at the six dates: Mars, the Moon, Spica and the mean
 * node, on each ephemeris, without the flag and with each. 36 checks.
 * swe_house_pos() takes no flags: its ARMC, obliquity and place are the
 * caller's.
 *
 * Measured: (a) all equal, (b) within 1.1e-13 deg, (c) 0. Before the fix
 * 150 of the 288 fail: 120 of (a) (the ayanamsa and its _ut form 1.75e-5
 * at 1800, 1.72e-5 at 1950, 1.99e-5 at the 1962 step's left, none after
 * it, with SEFLG_NONUT none; the traditional method's sidereal houses with
 * them, and their speeds within 1/64 day of the step, bug 203's one-sided
 * rate; the sectors of unflagged places 1.9e-6), 12 of (b) (the node and
 * Mars, up to 1.99e-5; the cusps hold, the houses' ayanamsa being the same
 * call's), 18 of (c) (Mars 1.49e-6, the Moon 1.47e-6, Spica 1.87e-6 off the
 * JPL file, the node 1.57e-6 on all three). The upstream build fails 170:
 * those, but for 6 rows of Makransky's houses, whose ayanamsa there takes
 * the nutation the Sun's call left at the instant, unkeyed (UPSTREAM-BUGS
 * 134); and 26 more, the returns of its sidereal Spica, which carry the
 * caller's Horizons bits, and swe_nod_aps() of Mars on the Swiss files at
 * one date (4.1e-8 deg).
 *
 * The defect (UPSTREAM-BUGS 236): a Gauquelin sector is the body's place
 * on its diurnal circle, which no zodiac moves, and methods 2 to 5 pass
 * swe_rise_trans() the ephemeris bits alone; but methods 0 and 1 set the
 * place the body's call answered with SEFLG_SIDEREAL in the frame of the
 * tropical ecliptic of date (the true obliquity, the ARMC): Mars at UT
 * 2433282.5, Zurich, Lahiri, 2.62 sectors off the call without the flag.
 * Check (d): swe_gauquelin_sector() with SEFLG_SIDEREAL is the call
 * without it, values and returns bit for bit, at the six dates, for Mars,
 * the Moon, Spica and the mean node on each ephemeris: methods 0 and 1 in
 * each sidereal setting (Lahiri, True Citra, Lahiri on the ecliptic of
 * date, of t0 and the solar system plane), the sidereal call also with
 * SEFLG_JPLHOR_APPROX and with SEFLG_JPLHOR, which the sidereal place
 * drops (so the tropical call without them); methods 2 to 5 in Lahiri's.
 * One check per setting, ephemeris and method: 42. Check (e): the sidereal
 * call's method 1 sector k names the sidereal Gauquelin cusps k and k + 1
 * of swe_houses_ex2() that the body's sidereal longitude lies between,
 * in the settings whose cusps are on the ecliptic of date (Lahiri, True
 * Citra, Lahiri on the ecliptic of date), 24 places each: 9 checks.
 *
 * Measured: (d) all equal, (e) all inside. Before the fix 39 of the 51
 * fail: 30 of (d), methods 0 and 1, 72 of 72 calls each, up to 3.42
 * sectors (methods 2 to 5 pass), and all 9 of (e), every place outside
 * its sector, by up to 23.0 deg. The upstream build fails 209 of the 339:
 * the 170 above and these 39.
 *
 * The defect (UPSTREAM-BUGS 247): in a mode whose ayanamsa is a star's
 * place, a refused star (its ephemeris's range) left swe_get_ayanamsa_ex()
 * adding the nutation to the 0 and clearing SEFLG_NONUT from ERR: -65 and
 * dpsi, -0.00425 deg at JD 100000 on Moshier, where the sidereal places
 * refuse with the star's message; the sidereal houses took that ayanamsa,
 * OK, and within 1/64 day of the star's range ends their speeds took the
 * refused side's: 1447 deg/day off. Check (f), each of the 12 star modes at
 * JD 100000 on Moshier and on the Swiss files with none found: the
 * ayanamsa and its _ut form ERR, 0, serr the message the sidereal mean
 * node (which answers there tropically) refuses with at that TT; the
 * sidereal houses at UT 100000, Placidus and Gauquelin, with speeds and
 * without, ERR, every output 0, serr the _ut form's; with no files
 * swe_get_ayanamsa() and _ut() NaN, as bug 212's refusals. 12 checks.
 * Check (g): True Citra on Moshier at TT 2^-8 day (the grid's) inside each
 * end of its star's range, found by bisection: the sidereal speeds of
 * Placidus, Koch, equal and Gauquelin, cusps and ascmc but the ARMC, the
 * tropical ones less the ayanamsa's rate (a five-point one-sided difference
 * over 2^-12 day inside), within 1e-8 deg/day. 2 checks.
 *
 * Measured: (f) all refuse alike, (g) 8.4e-10 and 4.9e-10 deg/day. Before
 * the fix 14 of the 353 fail: all of (f) (-65, -0.004255; _ut -65,
 * -0.004311; the houses OK; with no files the legacy forms 0) and both of
 * (g), 1447 and 1212 deg/day. The upstream build fails 223 of 353: the
 * 209 above and these 14. Run as
 *   sidhorflag EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static int n, bad;
static const char *EPH;

static const double TQ[] = {
  2378496.5, 2433282.5, 2437684.5 - 1.0 / 256, 2437684.5 + 1.0 / 256, 2451545.0, 2488069.5,
};
#define NTQ ((int) (sizeof TQ / sizeof TQ[0]))
static const int32 EPHF[3] = { SEFLG_JPLEPH, SEFLG_SWIEPH, SEFLG_MOSEPH };
static const char *EPHN[3] = { "de200", "Swiss", "Moshier" };
static const int32 HORF[2] = { SEFLG_JPLHOR_APPROX, SEFLG_JPLHOR };
static const char *HORN[2] = { "JPLHOR_APPROX", "JPLHOR" };

static swe_ctx *open_ctx(int32 sid)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  swe_set_ephe_path_r(c, EPH);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_sid_mode_r(c, sid, 0, 0);
  return c;
}

/* what a group of calls answers: their values and returns */
struct ans {
  double v[48];
  int32 r[12];
  int nv, nr;
};

typedef void (*callfn)(swe_ctx *c, double t, int32 fl, int arg, struct ans *a);

/* arg: 1 SEFLG_NONUT, 2 the _ut form */
static void c_aya(swe_ctx *c, double t, int32 fl, int arg, struct ans *a)
{
  char serr[256];
  if (arg & 1)
    fl |= SEFLG_NONUT;
  a->r[a->nr++] = arg & 2 ? swe_get_ayanamsa_ex_ut_r(c, t, fl, a->v, serr)
                          : swe_get_ayanamsa_ex_r(c, t, fl, a->v, serr);
  a->nv = 1;
}

static void c_calc(swe_ctx *c, double t, int32 fl, int arg, struct ans *a)
{
  static const int pl[5] = { SE_SUN, SE_MOON, SE_MARS, SE_MEAN_NODE, SE_TRUE_NODE };
  char serr[256], star[64] = "Spica";
  int k;
  for (k = 0; k < 5; k++)
    a->r[a->nr++] = swe_calc_r(c, t, pl[k], fl | SEFLG_SIDEREAL | SEFLG_SPEED, a->v + 6 * k, serr);
  a->r[a->nr++] = swe_fixstar_r(c, star, t, fl | SEFLG_SIDEREAL | SEFLG_SPEED, a->v + 30, serr);
  a->nv = 36;
  (void) arg;
}

static void c_nodaps(swe_ctx *c, double t, int32 fl, int arg, struct ans *a)
{
  char serr[256];
  a->r[a->nr++] = swe_nod_aps_r(c, t, SE_MARS, fl | SEFLG_SIDEREAL | SEFLG_SPEED, SE_NODBIT_MEAN,
                                a->v, a->v + 6, a->v + 12, a->v + 18, serr);
  a->nv = 24;
  (void) arg;
}

/* arg: the house system; at the UT t */
static void c_houses(swe_ctx *c, double t, int32 fl, int arg, struct ans *a)
{
  double cusp[13], ascmc[10], cs[13], as[10];
  char serr[256];
  memset(cusp, 0, sizeof cusp);
  memset(cs, 0, sizeof cs);
  a->r[a->nr++] = swe_houses_ex2_r(c, t, fl | SEFLG_SIDEREAL, 47.37, 8.55, arg, cusp, ascmc, cs,
                                   as, serr);
  memcpy(a->v, cusp + 1, 12 * sizeof(double));
  memcpy(a->v + 12, ascmc, 8 * sizeof(double));
  memcpy(a->v + 20, cs + 1, 12 * sizeof(double));
  memcpy(a->v + 32, as, 8 * sizeof(double));
  a->nv = 40;
}

/* methods 0 and 1 for the bodies whose place takes no Horizons flag with
 * fl's ephemeris: sidereal Mars always, the nodes, Mars and Spica off the
 * JPL file; at the UT t */
static void c_gauq(swe_ctx *c, double t, int32 fl, int arg, struct ans *a)
{
  double geo[3] = { 8.55, 47.37, 400 };
  char serr[256], star[64];
  int m;
  for (m = 0; m < 2; m++) {
    a->r[a->nr++] = swe_gauquelin_sector_r(c, t, SE_MARS, NULL, fl | SEFLG_SIDEREAL, m, geo, 0, 0,
                                           a->v + a->nv++, serr);
    a->r[a->nr++] = swe_gauquelin_sector_r(c, t, SE_MEAN_NODE, NULL, fl, m, geo, 0, 0,
                                           a->v + a->nv++, serr);
    a->r[a->nr++] = swe_gauquelin_sector_r(c, t, SE_TRUE_NODE, NULL, fl, m, geo, 0, 0,
                                           a->v + a->nv++, serr);
    if (!(fl & SEFLG_JPLEPH)) {
      strcpy(star, "Spica");
      a->r[a->nr++] = swe_gauquelin_sector_r(c, t, SE_MARS, NULL, fl, m, geo, 0, 0,
                                             a->v + a->nv++, serr);
      a->r[a->nr++] = swe_gauquelin_sector_r(c, t, 0, star, fl, m, geo, 0, 0, a->v + a->nv++,
                                             serr);
    }
  }
  (void) arg;
}

/* one check per ephemeris and flag: with the flag as without, bit for bit,
 * at every date; -1 if a call fails */
static int same_as_without(const char *name, swe_ctx *c, callfn f, int arg)
{
  int e, h, i, j;
  for (e = 0; e < 3; e++)
    for (h = 0; h < 2; h++) {
      int ndiff = 0;
      double w = 0;
      for (i = 0; i < NTQ; i++) {
        struct ans a0, a1;
        memset(&a0, 0, sizeof a0);
        memset(&a1, 0, sizeof a1);
        f(c, TQ[i], EPHF[e], arg, &a0);
        f(c, TQ[i], EPHF[e] | HORF[h], arg, &a1);
        for (j = 0; j < a0.nr; j++) {
          if (a0.r[j] < 0 || a1.r[j] < 0) {
            printf("    ERROR %s, %s, %s at JD %.8f\n", name, EPHN[e], HORN[h], TQ[i]);
            return -1;
          }
        }
        if (memcmp(a0.r, a1.r, sizeof a0.r) || memcmp(a0.v, a1.v, sizeof a0.v)) {
          ndiff++;
          for (j = 0; j < a0.nv; j++) {
            double d = fabs(swe_difdeg2n(a1.v[j], a0.v[j]));
            if (d > w)
              w = d;
          }
        }
      }
      n++;
      if (ndiff) {
        bad++;
        printf("    FAIL %s, %s, %s: %d of %d dates differ, by %.2e at most\n", name, EPHN[e],
               HORN[h], ndiff, NTQ, w);
      }
    }
  return 0;
}

static int check_a(void)
{
  static const struct {
    int32 sid;
    const char *name;
  } SA[] = {
    { SE_SIDM_LAHIRI, "Lahiri" }, { SE_SIDM_TRUE_CITRA, "True Citra" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_DATE, "Lahiri, ecl. of date" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_T0, "Lahiri, ecl. of t0" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, "Lahiri, ssy plane" },
  };
  static const int HS[5] = { 'P', 'K', 'E', 'R', 'i' };
  char name[128];
  int s, k, r = 0;
  for (s = 0; s < 5 && r == 0; s++) {
    swe_ctx *c = open_ctx(SA[s].sid);
    if (c == NULL)
      return -1;
    for (k = 0; k < 4 && r == 0 && s < 2; k++) {
      snprintf(name, sizeof name, "swe_get_ayanamsa_ex%s()%s, %s", k & 2 ? "_ut" : "",
               k & 1 ? " NONUT" : "", SA[s].name);
      r = same_as_without(name, c, c_aya, k);
    }
    if (r == 0 && s != 2) {
      snprintf(name, sizeof name, "sidereal places and Spica, %s", SA[s].name);
      r = same_as_without(name, c, c_calc, 0);
    }
    if (r == 0 && s == 0)
      r = same_as_without("sidereal swe_nod_aps() of Mars, Lahiri", c, c_nodaps, 0);
    if (r == 0 && s == 0)
      r = same_as_without("swe_gauquelin_sector() of unflagged places", c, c_gauq, 0);
    for (k = 0; k < 5 && r == 0; k++) {
      snprintf(name, sizeof name, "sidereal houses '%c', %s", HS[k], SA[s].name);
      r = same_as_without(name, c, c_houses, HS[k]);
    }
    swe_ctx_free(c);
  }
  if (r == 0)
    printf("  (a) sidereal output and unflagged Gauquelin sectors: %d checks, %d differ with"
           " the flag\n", n, bad);
  return r;
}

/* tropical less swe_get_ayanamsa_ex() less sidereal, with the same flags,
 * Lahiri: the largest over the dates, deg */
static int check_b(void)
{
  static const char *what[3] = { "mean node", "Mars", "Placidus cusps and Asc" };
  swe_ctx *c = open_ctx(SE_SIDM_LAHIRI);
  int e, h, i, j, q, n0 = n, b0 = bad;
  char serr[256];
  if (c == NULL)
    return -1;
  for (e = 0; e < 3; e++)
    for (h = 0; h < 2; h++) {
      int32 fl = EPHF[e] | HORF[h];
      double w[3] = { 0, 0, 0 };
      for (i = 0; i < NTQ; i++) {
        double t = TQ[i], tu, aya, ayu, xt[6], x0[6], xs[6], d;
        double tc[13], ta[10], sc[13], sa[10];
        static const int pl[2] = { SE_MEAN_NODE, SE_MARS };
        if (swe_get_ayanamsa_ex_r(c, t, fl, &aya, serr) < 0) {
          swe_ctx_free(c);
          return -1;
        }
        for (q = 0; q < 2; q++) {
          if (swe_calc_r(c, t, pl[q], fl, xt, serr) < 0
              || swe_calc_r(c, t, pl[q], EPHF[e], x0, serr) < 0
              || swe_calc_r(c, t, pl[q], fl | SEFLG_SIDEREAL, xs, serr) < 0) {
            swe_ctx_free(c);
            return -1;
          }
          /* less the flag's own movement of the tropical place (de200's
           * Mars; nothing elsewhere) */
          d = fabs(swe_difdeg2n(xt[0] - aya, xs[0]) - swe_difdeg2n(xt[0], x0[0]));
          if (d > w[q])
            w[q] = d;
        }
        /* the houses at UT tu, the ayanamsa at its TT */
        tu = t - 0.5;
        if (swe_houses_ex2_r(c, tu, fl, 47.37, 8.55, 'P', tc, ta, NULL, NULL, serr) < 0
            || swe_houses_ex2_r(c, tu, fl | SEFLG_SIDEREAL, 47.37, 8.55, 'P', sc, sa, NULL, NULL,
                                serr) < 0
            || swe_get_ayanamsa_ex_ut_r(c, tu, fl, &ayu, serr) < 0) {
          swe_ctx_free(c);
          return -1;
        }
        tc[0] = ta[0];
        sc[0] = sa[0];
        for (j = 0; j <= 12; j++) {
          d = fabs(swe_difdeg2n(tc[j] - ayu, sc[j]));
          if (d > w[2])
            w[2] = d;
        }
      }
      for (q = 0; q < 3; q++) {
        n++;
        if (!(w[q] <= 1e-10)) {
          bad++;
          printf("    FAIL tropical - ayanamsa - sidereal, %s, %s, %s: %.2e > 1e-10\n", what[q],
                 EPHN[e], HORN[h], w[q]);
        }
      }
      printf("    %-8s %-14s node %.1e  Mars %.1e%s  cusps %.1e\n", EPHN[e], HORN[h], w[0],
             w[1], e == 0 ? " (less trop(F) - trop(0))" : "", w[2]);
    }
  swe_ctx_free(c);
  printf("  (b) tropical less the ayanamsa is sidereal with the flags: %d checks, %d fail\n",
         n - n0, bad - b0);
  return 0;
}

/* swe_gauquelin_sector() method 0 against swe_house_pos() of the body's
 * place in the frame of its own call's Horizons bits */
static int check_c(void)
{
  static const int32 HX[3] = { 0, SEFLG_JPLHOR_APPROX, SEFLG_JPLHOR };
  static const char *HXN[3] = { "no flag", "JPLHOR_APPROX", "JPLHOR" };
  static const char *BN[4] = { "Mars", "Moon", "Spica", "mean node" };
  static const int PL[4] = { SE_MARS, SE_MOON, 0, SE_MEAN_NODE };
  double geo[3] = { 8.55, 47.37, 400 };
  swe_ctx *c = open_ctx(SE_SIDM_LAHIRI);
  int e, h, b, i, n0 = n, b0 = bad;
  char serr[256], star[64];
  double ww = 0;
  if (c == NULL)
    return -1;
  for (e = 0; e < 3; e++)
    for (h = 0; h < 3; h++)
      for (b = 0; b < 4; b++) {
        double w = 0;
        for (i = 0; i < NTQ; i++) {
          double tu = TQ[i], te, x[6], xn[6], g, armc;
          int32 fl = EPHF[e] | HX[h], r, hb;
          strcpy(star, "Spica");
          if (swe_gauquelin_sector_r(c, tu, PL[b], b == 2 ? star : NULL, fl, 0, geo, 0, 0, &g,
                                     serr) < 0) {
            swe_ctx_free(c);
            return -1;
          }
          te = tu + swe_deltat_ex_r(c, tu, fl, NULL);
          strcpy(star, "Spica");
          r = b == 2 ? swe_fixstar_r(c, star, te, fl, x, serr) : swe_calc_r(c, te, PL[b], fl, x, serr);
          hb = r & (SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
          if (r < 0 || swe_calc_r(c, te, SE_ECL_NUT, EPHF[e] | hb, xn, serr) < 0) {
            swe_ctx_free(c);
            return -1;
          }
          armc = swe_degnorm(swe_sidtime0_r(c, tu, xn[0], xn[2]) * 15 + geo[0]);
          g -= swe_house_pos_r(c, armc, geo[1], xn[0], 'G', x, NULL);
          if (fabs(g) > w)
            w = fabs(g);
        }
        n++;
        if (w > ww)
          ww = w;
        if (!(w <= 1e-11)) {
          bad++;
          printf("    FAIL Gauquelin sector of %s, %s, %s: %.2e sector off its frame's\n", BN[b],
                 EPHN[e], HXN[h], w);
        }
      }
  swe_ctx_free(c);
  printf("  (c) Gauquelin sectors in the frame of the place's own flags, within %.1e sector:"
         " %d checks, %d fail\n", ww, n - n0, bad - b0);
  return 0;
}

/* the sector's difference, the shorter way round the 36 */
static double sect_diff(double a, double b)
{
  double d = fabs(a - b);
  return d > 18 ? 36 - d : d;
}

/* swe_gauquelin_sector() with SEFLG_SIDEREAL against the call without it,
 * values and returns bit for bit: methods 0 and 1 in each sidereal
 * setting, the sidereal call also with each Horizons flag, which the
 * sidereal place drops (bug 223); methods 2 to 5 in Lahiri's */
static int check_d(void)
{
  static const struct {
    int32 sid;
    const char *name;
  } SD[] = {
    { SE_SIDM_LAHIRI, "Lahiri" }, { SE_SIDM_TRUE_CITRA, "True Citra" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_DATE, "Lahiri, ecl. of date" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_T0, "Lahiri, ecl. of t0" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, "Lahiri, ssy plane" },
  };
  static const int32 HX[3] = { 0, SEFLG_JPLHOR_APPROX, SEFLG_JPLHOR };
  static const int PL[4] = { SE_MARS, SE_MOON, 0, SE_MEAN_NODE };
  double geo[3] = { 8.55, 47.37, 400 }, ww = 0;
  int s, e, m, b, i, h, n0 = n, b0 = bad;
  char serr[256], star[64];
  for (s = 0; s < 5; s++) {
    swe_ctx *c = open_ctx(SD[s].sid);
    if (c == NULL)
      return -1;
    for (e = 0; e < 3; e++)
      for (m = 0; m <= 5; m++) {
        int ndiff = 0, ncall = 0;
        double w = 0;
        if (m >= 2 && s > 0)
          continue;
        for (b = 0; b < 4; b++)
          for (i = 0; i < NTQ; i++) {
            double g0 = 0;
            int32 r0;
            strcpy(star, "Spica");
            r0 = swe_gauquelin_sector_r(c, TQ[i], PL[b], b == 2 ? star : NULL, EPHF[e], m, geo, 0,
                                        0, &g0, serr);
            if (r0 < 0) {
              printf("    ERROR sector, %s, method %d at JD %.8f: %s\n", EPHN[e], m, TQ[i], serr);
              swe_ctx_free(c);
              return -1;
            }
            for (h = 0; h < (m < 2 ? 3 : 1); h++) {
              double g1 = 0;
              int32 r1;
              strcpy(star, "Spica");
              r1 = swe_gauquelin_sector_r(c, TQ[i], PL[b], b == 2 ? star : NULL,
                                          EPHF[e] | SEFLG_SIDEREAL | HX[h], m, geo, 0, 0, &g1,
                                          serr);
              ncall++;
              if (r1 != r0 || memcmp(&g0, &g1, sizeof g0)) {
                ndiff++;
                if (sect_diff(g1, g0) > w)
                  w = sect_diff(g1, g0);
              }
            }
          }
        n++;
        if (w > ww)
          ww = w;
        if (ndiff) {
          bad++;
          printf("    FAIL sidereal Gauquelin sector, %s, %s, method %d: %d of %d differ, by %.2e"
                 " sector at most\n", SD[s].name, EPHN[e], m, ndiff, ncall, w);
        }
      }
    swe_ctx_free(c);
  }
  printf("  (d) Gauquelin sectors with SEFLG_SIDEREAL as without, %.1e sector apart at most:"
         " %d checks, %d fail\n", ww, n - n0, bad - b0);
  return 0;
}

/* the sidereal call's method 1 sector k names the sidereal Gauquelin cusps
 * k and k + 1 (swe_houses_ex2()) that the body's sidereal longitude lies
 * between, in the settings whose cusps are on the ecliptic of date */
static int check_e(void)
{
  static const struct {
    int32 sid;
    const char *name;
  } SDE[] = {
    { SE_SIDM_LAHIRI, "Lahiri" }, { SE_SIDM_TRUE_CITRA, "True Citra" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_DATE, "Lahiri, ecl. of date" },
  };
  static const int PL[4] = { SE_MARS, SE_MOON, 0, SE_MEAN_NODE };
  double geo[3] = { 8.55, 47.37, 400 };
  int s, e, b, i, n0 = n, b0 = bad;
  char serr[256], star[64];
  for (s = 0; s < 3; s++) {
    swe_ctx *c = open_ctx(SDE[s].sid);
    if (c == NULL)
      return -1;
    for (e = 0; e < 3; e++) {
      int nout = 0;
      double w = 0;
      for (b = 0; b < 4; b++)
        for (i = 0; i < NTQ; i++) {
          double tu = TQ[i], te, g, x[6], cu[37], am[10], span, d;
          int32 fl = EPHF[e] | SEFLG_SIDEREAL, r;
          int k, k1;
          strcpy(star, "Spica");
          if (swe_gauquelin_sector_r(c, tu, PL[b], b == 2 ? star : NULL, fl, 1, geo, 0, 0, &g,
                                     serr) < 0
              || swe_houses_ex2_r(c, tu, fl, geo[1], geo[0], 'G', cu, am, NULL, NULL, serr) < 0) {
            swe_ctx_free(c);
            return -1;
          }
          te = tu + swe_deltat_ex_r(c, tu, fl, NULL);
          strcpy(star, "Spica");
          r = b == 2 ? swe_fixstar_r(c, star, te, fl, x, serr) : swe_calc_r(c, te, PL[b], fl, x, serr);
          if (r < 0) {
            swe_ctx_free(c);
            return -1;
          }
          k = (int) floor(g);
          if (k < 1 || k > 36) {
            nout++;
            continue;
          }
          k1 = k % 36 + 1;
          /* the sectors run clockwise: cusp k + 1 is behind cusp k */
          span = swe_degnorm(cu[k] - cu[k1]);
          d = swe_degnorm(cu[k] - x[0]);
          if (d > span) {
            nout++;
            d = fmin(d - span, 360 - d);
            if (d > w)
              w = d;
          }
        }
      n++;
      if (nout) {
        bad++;
        printf("    FAIL sidereal method 1 sector against the sidereal cusps, %s, %s: %d of %d"
               " outside their sector, %.2e deg at most\n", SDE[s].name, EPHN[e], nout, 4 * NTQ, w);
      }
    }
    swe_ctx_free(c);
  }
  printf("  (e) sidereal method 1 sectors between the sidereal Gauquelin cusps they name:"
         " %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* the 12 modes the ayanamsa takes from a star */
static const struct {
  int32 sid;
  const char *name;
} STARM[] = {
  { SE_SIDM_TRUE_CITRA, "True Citra" }, { SE_SIDM_TRUE_REVATI, "True Revati" },
  { SE_SIDM_TRUE_PUSHYA, "True Pushya" }, { SE_SIDM_TRUE_SHEORAN, "True Sheoran" },
  { SE_SIDM_TRUE_MULA, "True Mula" }, { SE_SIDM_GALCENT_0SAG, "Galactic Centre 0 Sag" },
  { SE_SIDM_GALCENT_COCHRANE, "Galactic Centre Cochrane" },
  { SE_SIDM_GALCENT_RGILBRAND, "Galactic Centre Gil Brand" },
  { SE_SIDM_GALCENT_MULA_WILHELM, "Galactic Centre Mula Wilhelm" },
  { SE_SIDM_GALEQU_IAU1958, "Galactic Equator IAU 1958" },
  { SE_SIDM_GALEQU_TRUE, "Galactic Equator true" }, { SE_SIDM_GALEQU_MULA, "Galactic Equator Mula" },
};
#define NSTARM ((int) (sizeof STARM / sizeof STARM[0]))

/* the houses at UT tu refuse with serr s0: ERR, every output 0 */
static int houses_refuse(swe_ctx *c, double tu, int32 fl, int hs, int speeds, const char *s0)
{
  double cu[37], am[10], cs[37], as[10];
  char serr[256] = "";
  int i, ito = hs == 'G' ? 36 : 12, ok;
  for (i = 0; i < 37; i++)
    cu[i] = cs[i] = 7;
  for (i = 0; i < 10; i++)
    am[i] = as[i] = 7;
  ok = swe_houses_ex2_r(c, tu, fl | SEFLG_SIDEREAL, 47.37, 8.55, hs, cu, am, speeds ? cs : NULL,
                        speeds ? as : NULL, serr) == ERR && strcmp(serr, s0) == 0;
  for (i = 0; i <= ito; i++)
    ok = ok && cu[i] == 0 && (!speeds || cs[i] == 0);
  for (i = 0; i < 10; i++)
    ok = ok && am[i] == 0 && (!speeds || as[i] == 0);
  return ok;
}

/* in a star mode whose star is refused (JD 100000: Moshier's range; the
 * Swiss files missing), the ayanamsa and its _ut form ERR, 0 and the
 * star's message, the one the sidereal places refuse with (the mean node,
 * which answers there tropically); the sidereal houses ERR with it, every
 * output 0; swe_get_ayanamsa() and _ut() NaN */
static int check_f(void)
{
  char nofiles[600];
  int m, n0 = n, b0 = bad;
  snprintf(nofiles, sizeof nofiles, "%s/no-such-dir-g164", EPH);
  for (m = 0; m < NSTARM; m++) {
    int e, ok = 1, h, ok0;
    for (e = 0; e < 2; e++) {
      const double t = 100000;
      int32 fl = e == 0 ? SEFLG_MOSEPH : SEFLG_SWIEPH;
      double a = 7, au = 7, x[6], xu[6];
      char sa[256] = "", su[256] = "", sx[256] = "", sxu[256] = "";
      int32 ra, rau, rx, rxu;
      swe_ctx *c = swe_ctx_new();
      if (c == NULL)
        return -1;
      swe_set_ephe_path_r(c, e == 0 ? EPH : nofiles);
      swe_set_sid_mode_r(c, STARM[m].sid, 0, 0);
      ok0 = ok;
      ra = swe_get_ayanamsa_ex_r(c, t, fl, &a, sa);
      rau = swe_get_ayanamsa_ex_ut_r(c, t, fl, &au, su);
      rx = swe_calc_r(c, t, SE_MEAN_NODE, fl | SEFLG_SIDEREAL, x, sx);
      rxu = swe_calc_ut_r(c, t, SE_MEAN_NODE, fl | SEFLG_SIDEREAL, xu, sxu);
      ok = ok && ra == ERR && a == 0 && rau == ERR && au == 0 && rx == ERR && rxu == ERR
           && *sa != '\0' && strcmp(sa, sx) == 0 && strcmp(su, sxu) == 0;
      for (h = 0; h < 4; h++)
        ok = ok && houses_refuse(c, t, fl, h & 1 ? 'G' : 'P', h & 2, su);
      if (e == 1)
        ok = ok && isnan(swe_get_ayanamsa_r(c, t)) && isnan(swe_get_ayanamsa_ut_r(c, t));
      if (ok0 && !ok)
        printf("    FAIL %s, %s, JD %.0f: ayanamsa %d %.6f [%s], _ut %d %.6f [%s], mean node"
               " %d [%s]\n", STARM[m].name, e ? "Swiss, no files" : "Moshier", t, ra, a, sa,
               rau, au, su, rx, sx);
      swe_ctx_free(c);
    }
    n++;
    if (!ok)
      bad++;
  }
  printf("  (f) refused stars refuse the ayanamsa, the places and the houses alike: %d checks,"
         " %d fail\n", n - n0, bad - b0);
  return 0;
}

/* the sidereal house speeds 2^-8 day inside each end of True Citra's
 * star on Moshier (found by bisection), whose stencil reaches past it:
 * the tropical speeds less the ayanamsa's rate (a five-point one-sided
 * difference over 2^-12 day inside), within 1e-8 deg/day, Placidus, Koch,
 * equal and Gauquelin, the cusps and ascmc but the ARMC */
static int check_g(void)
{
  static const int HS[4] = { 'P', 'K', 'E', 'G' };
  swe_ctx *c = swe_ctx_new();
  int k, j, i, n0 = n, b0 = bad;
  char serr[256];
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPH);
  swe_set_sid_mode_r(c, SE_SIDM_TRUE_CITRA, 0, 0);
  for (k = 0; k < 2; k++) {
    double in = k == 0 ? 1e6 : 2.5e6, out = k == 0 ? 6e5 : 2.9e6, te, tu, a[5], ad, w = 0;
    double h = 1.0 / 4096, s = k == 0 ? 1 : -1;
    int32 r;
    for (i = 0; i < 64; i++) {
      double mid = (in + out) / 2;
      /* < 0: upstream's refusal is -65 (bug 247) */
      if (swe_get_ayanamsa_ex_r(c, mid, SEFLG_MOSEPH, &a[0], serr) < 0)
        out = mid;
      else
        in = mid;
    }
    te = floor(in * 256) / 256 + s / 256;
    tu = te - swe_deltat_ex_r(c, te, SEFLG_MOSEPH, NULL);
    tu = te - swe_deltat_ex_r(c, tu, SEFLG_MOSEPH, NULL);
    te = tu + swe_deltat_ex_r(c, tu, SEFLG_MOSEPH, NULL);
    for (i = 0; i < 5; i++)
      if (swe_get_ayanamsa_ex_r(c, te + s * i * h, SEFLG_MOSEPH, &a[i], serr) < 0) {
        swe_ctx_free(c);
        return -1;
      }
    ad = s * (-25 * a[0] + 48 * a[1] - 36 * a[2] + 16 * a[3] - 3 * a[4]) / (12 * h);
    ad *= 1 + (swe_deltat_ex_r(c, tu + 1.0 / 64, SEFLG_MOSEPH, NULL)
               - swe_deltat_ex_r(c, tu - 1.0 / 64, SEFLG_MOSEPH, NULL)) * 32;
    for (j = 0; j < 4; j++) {
      double ct[37], at[10], cts[37], ats[10], cs[37], as[10], css[37], ass[10];
      int ito = HS[j] == 'G' ? 36 : 12;
      r = swe_houses_ex2_r(c, tu, SEFLG_MOSEPH, 47.37, 8.55, HS[j], ct, at, cts, ats, serr);
      if (r == ERR
          || swe_houses_ex2_r(c, tu, SEFLG_MOSEPH | SEFLG_SIDEREAL, 47.37, 8.55, HS[j], cs, as,
                              css, ass, serr) == ERR) {
        printf("    FAIL sidereal houses '%c' %s refused at TT %.8f: %s\n", HS[j],
               k ? "below the end" : "above the start", te, serr);
        w = 1e99;
        continue;
      }
      for (i = 1; i <= ito; i++)
        w = fmax(w, fabs(css[i] - (cts[i] - ad)));
      for (i = 0; i < 8; i++)
        if (i != 2)
          w = fmax(w, fabs(ass[i] - (ats[i] - ad)));
    }
    n++;
    if (!(w <= 1e-8)) {
      bad++;
      printf("    FAIL sidereal house speeds 2^-8 day %s True Citra's star, TT %.8f: %.3e"
             " deg/day off\n", k ? "below the end of" : "above the start of", te, w);
    } else
      printf("    TT %.8f, 2^-8 day %s: speeds within %.1e deg/day\n", te,
             k ? "below the star's end" : "above the star's start", w);
  }
  swe_ctx_free(c);
  printf("  (g) sidereal house speeds at a star's ends: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: sidhorflag EPHEDIR\n");
    return 2;
  }
  EPH = argv[1];
  printf("G164: the JPL Horizons flags reach no sidereal output, and Gauquelin's frame takes"
         " them as the place does;\n      SEFLG_SIDEREAL moves no Gauquelin sector; a refused star"
         " refuses its ayanamsa, places and houses\n");
  r = check_a();
  if (r == 0)
    r = check_b();
  if (r == 0)
    r = check_c();
  if (r == 0)
    r = check_d();
  if (r == 0)
    r = check_e();
  if (r == 0)
    r = check_f();
  if (r == 0)
    r = check_g();
  if (r < 0) {
    printf("G164 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G164 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG164 PASS\n", n);
  return 0;
}
