/* sidhorflag.c -- G164: the JPL Horizons flags reach no sidereal output,
 * and the Gauquelin sectors' frame takes them as the body's place does.
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
 * one date (4.1e-8 deg). Run as
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

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: sidhorflag EPHEDIR\n");
    return 2;
  }
  EPH = argv[1];
  printf("G164: the JPL Horizons flags reach no sidereal output, and Gauquelin's frame takes"
         " them as the place does\n");
  r = check_a();
  if (r == 0)
    r = check_b();
  if (r == 0)
    r = check_c();
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
