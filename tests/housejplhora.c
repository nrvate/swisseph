/* housejplhora.c -- G156: across SEMOD_JPLHORA_3's step at 1962 Jan 20 the
 * sidereal house speeds are each side's own rate; no house takes the
 * Horizons flags; Makransky's speeds are its cusps' rate.
 *
 * The defect (UPSTREAM-BUGS 203): with SEFLG_JPLHOR_APPROX under
 * SEMOD_JPLHORA_3 (the default mode), and SEFLG_JPLHOR fallen to it without
 * the IERS corrections, the nutation is IAU 1980 plus a fixed offset up to
 * and including JD 2437684.5 and the chosen model after it, a step of
 * 7.17e-2" in dpsi (UPSTREAM-BUGS 153). swe_houses_ex2()'s house frames
 * take no flags, but the sidereal houses' ayanamsa took its nutation with
 * the caller's (swe_get_ayanamsa_ex()), and its rate, a difference of the
 * ayanamsa over the house frames 1/64 day either side (UPSTREAM-BUGS 43,
 * 187), reached across the step: on de200 at Zurich every sidereal cusp
 * and ascmc speed 6.38e-4 deg/day off its cusps' rate within 1/64 day of
 * it, in every house system, Lahiri, True Citra and Lahiri on the ecliptic
 * of date alike, on the Swiss files too. Bug 203's fix took that side's
 * rate; since UPSTREAM-BUGS 223 the ayanamsa takes no Horizons flag, and
 * so neither does any house: the cusps of the traditional method had been
 * -2.0e-5 deg off with the flag before the step (-1.72e-5 at 1950), none
 * after it. UPSTREAM-BUGS 224: Makransky's Sunshine ('i')
 * cusps lost digits as w, the right ascension its cusp is projected from,
 * neared 90 or 270 deg (cos m tan w with m rounded near 90), 1.1e-4 deg at
 * 4e-10 deg from it, and the ARMC difference of its speeds took them: cusp
 * 9 1.6e-7 deg/day off at TT 2437683.0, 1.5 day before the step, 3.6e-6
 * nearer.
 *
 * Check (a): de200 (and the Swiss files with SEFLG_JPLHOR_APPROX), Zurich,
 * delta t 2^-11 day, the 105 instants about the step (TT; the UT 2^-11 day
 * before; the step's own instant the left side's): t -+ k/16 for k = 0..32
 * and t -+ 2^-k for k = 7..26. Tropical, Lahiri, True Citra and Lahiri on
 * the ecliptic of date, without the flag, with SEFLG_JPLHOR_APPROX and with
 * SEFLG_JPLHOR; Placidus, Koch, Porphyry, Regiomontanus, Campanus, equal,
 * Alcabitius, Morinus, azimuthal, Sunshine (Treindl) and Makransky: the 12
 * cusps' and 8 ascmc's speeds against five-point differences over 2^-12
 * day of the same call's cusps on the instant's side, central where the
 * stencil keeps to it, one-sided where it would cross (G139 check (a)'s);
 * the worst within 1/64 day of the step and the rest, each within 1e-7
 * deg/day. 352 checks.
 * Check (b): every house -- tropical, Lahiri on the ecliptic of t0 and on
 * the solar system plane, Lahiri with SEFLG_NONUT, and the traditional
 * method's Lahiri, True Citra and Lahiri on the ecliptic of date -- with
 * SEFLG_JPLHOR_APPROX and with SEFLG_JPLHOR equal the call without, the
 * cusps, ascmc and their speeds bit for bit at the 105 instants, the same
 * eleven systems: 154 checks.
 * Check (c): Makransky's cusps by swe_houses_armc_ex2() at 47N, 60N and
 * 34S, the Sun's declination -20.58 and 23.4 deg, over a turn of ARMC 2^-4
 * deg apart: the speeds (at ARMCS) against five-point differences of the
 * cusps over 2^-4 and 2^-5 deg of ARMC, Richardson-combined, within 1e-8
 * deg/day, and each cusp against the quartic of its neighbours 2^-8 and
 * 2^-7 deg away within 1e-11 deg. 2 checks.
 *
 * Measured: (a) within 1/64 day 5.06e-8 deg/day at worst (azimuthal; the
 * one-sided reference's noise, 5.03e-8 without the flag, 3.6e-8 for the
 * other systems), the rest 1.24e-8 (Makransky 3.7e-8 and 1.07e-8); (b)
 * all equal; (c) 4.83e-9 deg/day and 2.3e-13 deg. Before bug 203's fix 90
 * of (a)'s first 320 failed, all within 1/64 day (6.38e-4); before bugs
 * 223 and 224 80 of the 508 fail: 12 of (a), Makransky's rest on de200
 * (1.64e-7), 66 of (b), the traditional method's with either flag (73 of
 * the 105 instants), and (c) (3.56e-6 deg/day, 7.2e-10 deg). The upstream
 * build fails 408 of 508: all of (a) (its speeds at ARMCS, UPSTREAM-BUGS
 * 42: equal houses 5.4e-5 to 6.6e-5 deg/day, Koch 199, Porphyry 395; the
 * sidereal ones without the ayanamsa's rate, bug 43), 54 of (b) (the
 * traditional method's but Sunshine's, whose ayanamsa there takes the
 * nutation the Sun's call left at the instant, unkeyed, bug 134), and (c)
 * (its ARMC speeds over one second, 3.8e-5). Run as
 *   housejplhora EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

/* HORIZONS_TJD0_DPSI_DEPS_IAU1980: the step, its own instant the left
 * side's (SEFLG_JPLHOR's models up to and including it) */
#define TS 2437684.5

static int n, bad;

static int left_of(double t)
{
  return t <= TS;
}

static int same_side(double p, double t)
{
  return left_of(p) == left_of(t);
}

/* the 105 instants about TS, TT */
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

static swe_ctx *open_ctx(const char *ephe, int32 sid)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  swe_set_ephe_path_r(c, ephe);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_delta_t_userdef_r(c, ldexp(1, -11));
  if (sid >= 0)
    swe_set_sid_mode_r(c, sid, 0, 0);
  return c;
}

static const struct {
  int hs;
  const char *name;
} SH[] = {
  { 'P', "Placidus" }, { 'K', "Koch" }, { 'O', "Porphyry" }, { 'R', "Regiomontanus" },
  { 'C', "Campanus" }, { 'E', "equal" }, { 'B', "Alcabitius" }, { 'M', "Morinus" },
  { 'H', "azimuthal" }, { 'I', "Sunshine" }, { 'i', "Makransky" },
};
#define NSH ((int) (sizeof SH / sizeof SH[0]))

/* the worst |speed - reference| at TT tt, deg/day, of the 12 cusps and 8
 * ascmc; -1 if a call fails */
static double worst_off(swe_ctx *c, double tt, int32 fl, int hs)
{
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  const double h = ldexp(1, -12), dts = ldexp(1, -11);
  double t = tt - dts, cusp[13], ascmc[10], cs[13], as[10], f[5][20], w = 0;
  int centred = 1, dir = left_of(tt) ? -1 : 1, j, k;
  char serr[256];
  for (k = 1; k <= 2; k++)
    if (!same_side(tt - k * h, tt) || !same_side(tt + k * h, tt))
      centred = 0;
  if (swe_houses_ex2_r(c, t, fl, 47.37, 8.55, hs, cusp, ascmc, cs, as, serr) < 0)
    return -1;
  for (k = 0; k < 5; k++) {
    double cu[13], am[10], tk = centred ? t + (k - 2) * h : t + dir * k * h;
    if (swe_houses_ex2_r(c, tk, fl, 47.37, 8.55, hs, cu, am, NULL, NULL, serr) < 0)
      return -1;
    for (j = 0; j < 12; j++)
      f[k][j] = cu[j + 1];
    for (j = 0; j < 8; j++)
      f[k][12 + j] = am[j];
  }
  for (j = 0; j < 20; j++) {
    double v[5], d = 0, e;
    for (k = 0; k < 5; k++)
      v[k] = swe_difdeg2n(f[k][j], f[centred ? 2 : 0][j]);
    if (centred) {
      d = (8 * (v[3] - v[1]) - (v[4] - v[0])) / (12 * h);
    } else {
      for (k = 0; k < 5; k++)
        d += o5[k] * v[k];
      d = d / h * dir;
    }
    e = fabs((j < 12 ? cs[j + 1] : as[j - 12]) - d);
    if (!(e <= w))
      w = e;
  }
  return w;
}

static int check_a(const char *ephe)
{
  static const struct {
    int32 sid;
    const char *name;
  } SF[] = {
    { -1, "tropical" }, { SE_SIDM_LAHIRI, "Lahiri" }, { SE_SIDM_TRUE_CITRA, "True Citra" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_DATE, "Lahiri, ecl. of date" },
  };
  static const struct {
    int32 fl;
    const char *name;
  } SG[] = {
    { SEFLG_JPLEPH, "de200" }, { SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, "de200, JPLHOR_APPROX" },
    { SEFLG_JPLEPH | SEFLG_JPLHOR, "de200, JPLHOR" },
    { SEFLG_SWIEPH | SEFLG_JPLHOR_APPROX, "Swiss, JPLHOR_APPROX" },
  };
  static const char *sn[2] = { "within 1/64 day", "the rest" };
  double tq[105];
  int nq = instants(tq), f, g, s, i, q;
  printf("  (a) worst |house speed - reference|, deg/day, %s / %s\n", sn[0], sn[1]);
  for (f = 0; f < (int) (sizeof SF / sizeof SF[0]); f++)
    for (g = 0; g < (int) (sizeof SG / sizeof SG[0]); g++) {
      swe_ctx *c = open_ctx(ephe, SF[f].sid);
      int32 fl = SG[g].fl | (SF[f].sid >= 0 ? SEFLG_SIDEREAL : 0);
      double W[2] = { 0, 0 };
      if (c == NULL)
        return -1;
      for (s = 0; s < NSH; s++) {
        double w[2] = { 0, 0 };
        for (i = 0; i < nq; i++) {
          int sp = fabs(tq[i] - TS) <= 1.0 / 64 ? 0 : 1;
          double e = worst_off(c, tq[i], fl, SH[s].hs);
          if (e < 0) {
            printf("    ERROR %s, %s, %s at JD %.8f\n", SF[f].name, SG[g].name, SH[s].name, tq[i]);
            swe_ctx_free(c);
            return -1;
          }
          if (e > w[sp])
            w[sp] = e;
        }
        for (q = 0; q < 2; q++) {
          n++;
          if (!(w[q] <= 1e-7)) {
            bad++;
            printf("    FAIL %s, %s, %s, %s: %.2e > 1e-7\n", SF[f].name, SG[g].name,
                   SH[s].name, sn[q], w[q]);
          }
          if (w[q] > W[q])
            W[q] = w[q];
        }
      }
      printf("    %-21s %-22s %.2e  %.2e\n", SF[f].name, SG[g].name, W[0], W[1]);
      swe_ctx_free(c);
    }
  return 0;
}

static int check_b(const char *ephe)
{
  static const struct {
    int32 sid, fl;
    const char *name;
  } SF[] = {
    { -1, 0, "tropical" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_T0, SEFLG_SIDEREAL, "Lahiri, ecl. of t0" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, SEFLG_SIDEREAL, "Lahiri, ssy plane" },
    { SE_SIDM_LAHIRI, SEFLG_SIDEREAL | SEFLG_NONUT, "Lahiri, NONUT" },
    /* the traditional method's, since UPSTREAM-BUGS 223 */
    { SE_SIDM_LAHIRI, SEFLG_SIDEREAL, "Lahiri" },
    { SE_SIDM_TRUE_CITRA, SEFLG_SIDEREAL, "True Citra" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_DATE, SEFLG_SIDEREAL, "Lahiri, ecl. of date" },
  };
  static const int32 XF[2] = { SEFLG_JPLHOR_APPROX, SEFLG_JPLHOR };
  double tq[105];
  int nq = instants(tq), f, x, s, i, nd = 0;
  for (f = 0; f < (int) (sizeof SF / sizeof SF[0]); f++) {
    swe_ctx *c = open_ctx(ephe, SF[f].sid);
    if (c == NULL)
      return -1;
    for (x = 0; x < 2; x++)
      for (s = 0; s < NSH; s++) {
        int ndiff = 0;
        for (i = 0; i < nq; i++) {
          double t = tq[i] - ldexp(1, -11);
          double c0[13], a0[10], s0[13], b0[10], c1[13], a1[10], s1[13], b1[10];
          int32 fl = SEFLG_JPLEPH | SF[f].fl;
          int r0 = swe_houses_ex2_r(c, t, fl, 47.37, 8.55, SH[s].hs, c0, a0, s0, b0, NULL);
          int r1 = swe_houses_ex2_r(c, t, fl | XF[x], 47.37, 8.55, SH[s].hs, c1, a1, s1, b1,
                                    NULL);
          if (r0 < 0 || r1 < 0) {
            printf("    ERROR %s, %s at JD %.8f\n", SF[f].name, SH[s].name, tq[i]);
            swe_ctx_free(c);
            return -1;
          }
          if (memcmp(c0, c1, sizeof c0) || memcmp(a0, a1, sizeof a0)
              || memcmp(s0 + 1, s1 + 1, 12 * sizeof(double))
              || memcmp(b0, b1, 8 * sizeof(double)))
            ndiff++;
        }
        n++;
        if (ndiff) {
          bad++;
          printf("    FAIL %s, %s, %s: %d of %d instants differ\n", SF[f].name,
                 x ? "JPLHOR" : "JPLHOR_APPROX", SH[s].name, ndiff, nq);
        }
        nd += ndiff;
      }
    swe_ctx_free(c);
  }
  printf("  (b) every house, tropical and sidereal: with the flag as without, %d instants"
         " differ\n", nd);
  return 0;
}

/* Makransky's cusps at ARMC a, lat, eps 23.44 and the Sun's declination
 * dec, by swe_houses_armc_ex2() (the speeds at ARMCS when cs is given);
 * -1 if it answers Porphyry */
static int makr(swe_ctx *c, double a, double lat, double dec, double *cu, double *cs)
{
  double am[10], as[10];
  am[9] = dec;
  return swe_houses_armc_ex2_r(c, swe_degnorm(a), lat, 23.44, 'i', cu, am, cs, as, NULL) < 0
         ? -1 : 0;
}

/* Check (c): Makransky's speeds in ARMC, and the cusps they are the rate
 * of, over a whole turn of ARMC (2^-4 deg apart, 2^-7 off the grid so
 * that no stencil lands on an ARMC of 90 or 270 deg, where the MC and the
 * Asc are set within 1e-10 deg) */
static int check_c(void)
{
  static const double LAT[3] = { 47.37, 60, -33.9 }, DEC[2] = { -20.58, 23.4 };
  /* the reference's stencils in d = 2^-4 deg, the position's in 2^-8 */
  static const double O[10] = { -2, -1, -0.5, 0.5, 1, 2, -2.0 / 32, -1.0 / 32, 1.0 / 32,
                                2.0 / 32 };
  const double ARMCS = 360.9856473350, d = 1.0 / 16;
  swe_ctx *c = swe_ctx_new();
  double ws = 0, wp = 0;
  int l, q, k, i, j, ns = 0;
  if (c == NULL)
    return -1;
  for (l = 0; l < 3; l++)
    for (q = 0; q < 2; q++)
      for (k = 0; k < 360 * 16; k++) {
        double a = k / 16.0 + 1.0 / 128, cu[13], cs[13], v[10][13];
        int ok = makr(c, a, LAT[l], DEC[q], cu, cs) == 0;
        for (j = 0; j < 10 && ok; j++)
          ok = makr(c, a + O[j] * d, LAT[l], DEC[q], v[j], NULL) == 0;
        if (!ok)
          continue;
        for (i = 1; i <= 12; i++) {
          double x[10], s1, s2, e;
          for (j = 0; j < 10; j++)
            x[j] = swe_difdeg2n(v[j][i], cu[i]);
          if (fabs(x[0]) > 10 || fabs(x[5]) > 10)
            continue;	/* a cusp across the stencil's ends jumps */
          /* five points over d and d/2, Richardson-combined */
          s1 = (8 * (x[4] - x[1]) - (x[5] - x[0])) / (12 * d);
          s2 = (8 * (x[3] - x[2]) - (x[4] - x[1])) / (6 * d);
          e = fabs(cs[i] - (16 * s2 - s1) / 15 * ARMCS);
          if (e > ws)
            ws = e;
          /* the cusp against the quartic of its neighbours 2^-8 and 2^-7
           * deg away */
          e = fabs((4 * (x[7] + x[8]) - (x[6] + x[9])) / 6);
          if (e > wp)
            wp = e;
          ns++;
        }
      }
  swe_ctx_free(c);
  n += 2;
  if (!(ws <= 1e-8)) {
    bad++;
    printf("    FAIL Makransky's speeds in ARMC: %.2e deg/day > 1e-8\n", ws);
  }
  if (!(wp <= 1e-11)) {
    bad++;
    printf("    FAIL Makransky's cusps against their neighbours: %.2e deg > 1e-11\n", wp);
  }
  printf("  (c) Makransky, %d cusps at 47N, 60N and 34S: speeds in ARMC within %.2e deg/day,"
         " cusps %.2e deg\n", ns, ws, wp);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: housejplhora EPHEDIR\n");
    return 2;
  }
  printf("G156: the sidereal house speeds across SEMOD_JPLHORA_3's 1962 step are each side's rate;"
         " no house takes the Horizons flags; Makransky's speeds are its cusps' rate\n");
  r = check_a(argv[1]);
  if (r == 0)
    r = check_b(argv[1]);
  if (r == 0)
    r = check_c();
  if (r < 0) {
    printf("G156 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G156 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG156 PASS\n", n);
  return 0;
}
