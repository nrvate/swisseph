/* gauqflags.c -- G42: a Gauquelin sector is the body's place on its
 * diurnal circle, so no flag but the ephemeris and those that choose the
 * body's direction from the observer moves it.
 *
 * swe_gauquelin_sector()'s methods 0 and 1 computed the body with iflag
 * and set the place it answered, as ecliptic longitude and latitude, in
 * the frame of the true ecliptic of date (the true obliquity and the
 * ARMC). Every flag that names that place's zodiac, frame or form moved the
 * sector: Mars at UT 2433282.5, Zurich, Swiss files, method 0, against the
 * call without, SEFLG_SIDEREAL (Lahiri) +2.62 sectors, SEFLG_NONUT
 * -1.14e-4, SEFLG_J2000 -0.0875, SEFLG_EQUATORIAL -0.148, SEFLG_RADIANS
 * +17.8, SEFLG_XYZ +18.3, and SEFLG_HELCTR and SEFLG_BARYCTR, a direction
 * from the Sun, +3.88. iflag is "flag for ephemeris and SEFLG_TOPOCTR"
 * (swephprg 16.1), and methods 2 to 5 pass swe_rise_trans() the ephemeris
 * bits alone.
 *
 * The checks, at Zurich (8.55 E, 47.37 N, 400 m), UT 2378496.5,
 * 2433282.5, 2437684.5 -+ 2^-8, 2451545.0 and 2488069.5, on de200, the
 * Swiss files and Moshier:
 * (a) with SEFLG_SIDEREAL the call without it, values and returns bit for
 *   bit, for Mars, the Moon, Spica and the mean node: methods 0 and 1 in
 *   Lahiri's, True Citra's and Lahiri's on the ecliptic of date, of t0 and
 *   the solar system plane, the sidereal call also with
 *   SEFLG_JPLHOR_APPROX and with SEFLG_JPLHOR, which the sidereal place
 *   drops (so the tropical call without them); methods 2 to 5 in
 *   Lahiri's. One check per setting, ephemeris and method: 42.
 * (b) the sidereal call's method 1 sector k names the sidereal Gauquelin
 *   cusps k and k + 1 of swe_houses_ex2() that the body's sidereal
 *   longitude lies between, in the settings whose cusps are on the
 *   ecliptic of date (Lahiri, True Citra, Lahiri on the ecliptic of
 *   date), the four bodies at the six dates: 9 checks.
 * (c) methods 0 and 1 with each of SEFLG_SPEED, SPEED3, NONUT, ICRS,
 *   J2000, EQUATORIAL, RADIANS, XYZ, HELCTR, BARYCTR and all of them with
 *   SIDEREAL, on each base of the flags that choose the direction (none,
 *   SEFLG_TOPOCTR, TRUEPOS, NOABERR | NOGDEFL, CENTER_BODY,
 *   JPLHOR_APPROX; with SEFLG_SIDEREAL the base less the Horizons bits),
 *   the call with the base alone, values and returns bit for bit: the Sun,
 *   Moon, Mars, Jupiter, mean node and Spica. One check per flag,
 *   ephemeris and method: 66.
 * (d) on each base but the Horizons one, methods 0 and 1 are
 *   swe_house_pos() of the place the base gives, in the frame of
 *   SE_ECL_NUT's true obliquity and the sidereal time, within 1e-11
 *   sector. One check per base and ephemeris: 15.
 * (e) methods 2 to 5 with each of those flags as without, Mars and the
 *   Moon at 1950 and 2000: 4 checks.
 *
 * Run as
 *   gauqflags EPHEDIR
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
static double GEO[3] = { 8.55, 47.37, 400 };

/* the flags that name the place's zodiac, frame or form, or a centre not
 * the observer's, one at a time and all */
#define ALLF (SEFLG_SPEED | SEFLG_NONUT | SEFLG_ICRS | SEFLG_J2000 | SEFLG_EQUATORIAL \
              | SEFLG_RADIANS | SEFLG_XYZ | SEFLG_HELCTR | SEFLG_SIDEREAL)
static const int32 FF[] = {
  SEFLG_SPEED, SEFLG_SPEED3, SEFLG_NONUT, SEFLG_ICRS, SEFLG_J2000, SEFLG_EQUATORIAL,
  SEFLG_RADIANS, SEFLG_XYZ, SEFLG_HELCTR, SEFLG_BARYCTR, ALLF,
};
static const char *FFN[] = {
  "SPEED", "SPEED3", "NONUT", "ICRS", "J2000", "EQUATORIAL", "RADIANS", "XYZ", "HELCTR",
  "BARYCTR", "all with SIDEREAL",
};
#define NFF ((int) (sizeof FF / sizeof FF[0]))
/* the flags that choose the body's direction from the observer; the
 * Horizons base last */
static const int32 DD[] = {
  0, SEFLG_TOPOCTR, SEFLG_TRUEPOS, SEFLG_NOABERR | SEFLG_NOGDEFL, SEFLG_CENTER_BODY,
  SEFLG_JPLHOR_APPROX,
};
static const char *DDN[] = {
  "no flag", "TOPOCTR", "TRUEPOS", "NOABERR|NOGDEFL", "CENTER_BODY", "JPLHOR_APPROX",
};
#define NDD ((int) (sizeof DD / sizeof DD[0]))
/* the bodies of (c) and (d); -1 Spica */
static const int PL6[] = { SE_SUN, SE_MOON, SE_MARS, SE_JUPITER, SE_MEAN_NODE, -1 };
#define NPL6 ((int) (sizeof PL6 / sizeof PL6[0]))
/* the bodies of (a) and (b) */
static const int PL4[4] = { SE_MARS, SE_MOON, -1, SE_MEAN_NODE };

static swe_ctx *open_ctx(int32 sid)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  swe_set_ephe_path_r(c, EPH);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_sid_mode_r(c, sid, 0, 0);
  swe_set_topo_r(c, GEO[0], GEO[1], GEO[2]);
  return c;
}

static int32 sector(swe_ctx *c, double t, int pl, int32 fl, int m, double *s)
{
  char serr[256], star[64] = "Spica";
  return swe_gauquelin_sector_r(c, t, pl < 0 ? 0 : pl, pl < 0 ? star : NULL, fl, m, GEO, 0, 0,
                                s, serr);
}

/* the place the sector is of: the body's or Spica's */
static int32 place(swe_ctx *c, double te, int pl, int32 fl, double *x)
{
  char serr[256], star[64] = "Spica";
  return pl < 0 ? swe_fixstar_r(c, star, te, fl, x, serr) : swe_calc_r(c, te, pl, fl, x, serr);
}

/* the sector's difference, the shorter way round the 36 */
static double sect_diff(double a, double b)
{
  double d = fabs(a - b);
  return d > 18 ? 36 - d : d;
}

/* (a) SEFLG_SIDEREAL as without */
static int check_a(void)
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
  double ww = 0;
  int s, e, m, b, i, h, n0 = n, b0 = bad;
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
            int32 r0 = sector(c, TQ[i], PL4[b], EPHF[e], m, &g0);
            if (r0 < 0) {
              printf("    ERROR sector of body %d, %s, method %d at JD %.8f\n", PL4[b], EPHN[e],
                     m, TQ[i]);
              swe_ctx_free(c);
              return -1;
            }
            for (h = 0; h < (m < 2 ? 3 : 1); h++) {
              double g1 = 0;
              int32 r1 = sector(c, TQ[i], PL4[b], EPHF[e] | SEFLG_SIDEREAL | HX[h], m, &g1);
              ncall++;
              if (r1 != r0 || memcmp(&g0, &g1, sizeof g0)) {
                ndiff++;
                w = fmax(w, sect_diff(g1, g0));
              }
            }
          }
        n++;
        ww = fmax(ww, w);
        if (ndiff) {
          bad++;
          printf("    FAIL sidereal sector, %s, %s, method %d: %d of %d differ, by %.2e sector"
                 " at most\n", SD[s].name, EPHN[e], m, ndiff, ncall, w);
        }
      }
    swe_ctx_free(c);
  }
  printf("  (a) sectors with SEFLG_SIDEREAL as without, %.1e sector apart at most: %d checks,"
         " %d fail\n", ww, n - n0, bad - b0);
  return 0;
}

/* (b) the sidereal method 1 sector between the sidereal cusps it names */
static int check_b(void)
{
  static const struct {
    int32 sid;
    const char *name;
  } SDE[] = {
    { SE_SIDM_LAHIRI, "Lahiri" }, { SE_SIDM_TRUE_CITRA, "True Citra" },
    { SE_SIDM_LAHIRI | SE_SIDBIT_ECL_DATE, "Lahiri, ecl. of date" },
  };
  int s, e, b, i, n0 = n, b0 = bad;
  char serr[256];
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
          int32 fl = EPHF[e] | SEFLG_SIDEREAL;
          int k, k1;
          if (sector(c, tu, PL4[b], fl, 1, &g) < 0
              || swe_houses_ex2_r(c, tu, fl, GEO[1], GEO[0], 'G', cu, am, NULL, NULL, serr) < 0) {
            printf("    ERROR %s, %s, body %d at JD %.8f\n", SDE[s].name, EPHN[e], PL4[b], tu);
            swe_ctx_free(c);
            return -1;
          }
          te = tu + swe_deltat_ex_r(c, tu, fl, NULL);
          if (place(c, te, PL4[b], fl, x) < 0) {
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
            w = fmax(w, fmin(d - span, 360 - d));
          }
        }
      n++;
      if (nout) {
        bad++;
        printf("    FAIL sidereal method 1 sector against the sidereal cusps, %s, %s: %d of %d"
               " outside their sector, %.2e deg at most\n", SDE[s].name, EPHN[e], nout, 4 * NTQ,
               w);
      }
    }
    swe_ctx_free(c);
  }
  printf("  (b) sidereal method 1 sectors between the sidereal cusps they name: %d checks,"
         " %d fail\n", n - n0, bad - b0);
  return 0;
}

/* (c) methods 0 and 1 with each frame flag on each direction base, the
 * call with the base alone */
static int check_c(void)
{
  swe_ctx *c = open_ctx(SE_SIDM_LAHIRI);
  int f, e, m, d, b, i, n0 = n, b0 = bad;
  double ww = 0;
  if (c == NULL)
    return -1;
  for (f = 0; f < NFF; f++)
    for (e = 0; e < 3; e++)
      for (m = 0; m < 2; m++) {
        int ndiff = 0, nok = 0;
        double w = 0;
        for (d = 0; d < NDD; d++)
          for (b = 0; b < NPL6; b++)
            for (i = 0; i < NTQ; i++) {
              int32 base = DD[d], r0, r1;
              double s0 = 0, s1 = 0;
              if (FF[f] & SEFLG_SIDEREAL)
                base &= ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
              r0 = sector(c, TQ[i], PL6[b], EPHF[e] | base, m, &s0);
              r1 = sector(c, TQ[i], PL6[b], EPHF[e] | DD[d] | FF[f], m, &s1);
              if (r0 >= 0)
                nok++;
              if (r0 != r1 || memcmp(&s0, &s1, sizeof s0) != 0) {
                ndiff++;
                if (r0 >= 0 && r1 >= 0)
                  w = fmax(w, sect_diff(s0, s1));
              }
            }
        n++;
        ww = fmax(ww, w);
        if (ndiff || nok == 0) {
          bad++;
          printf("    FAIL method %d, %s, %s: %d of %d calls off the call without, up to %.3g"
                 " sector\n", m, EPHN[e], FFN[f], ndiff, NDD * NPL6 * NTQ, w);
        }
      }
  swe_ctx_free(c);
  printf("  (c) methods 0, 1 with a frame flag as without (up to %.2e sector): %d checks,"
         " %d fail\n", ww, n - n0, bad - b0);
  return 0;
}

/* (d) methods 0 and 1 with each direction flag: swe_house_pos() of the
 * place that flag gives, in the frame of the true ecliptic of date */
static int check_d(void)
{
  swe_ctx *c = open_ctx(SE_SIDM_LAHIRI);
  int d, e, m, b, i, n0 = n, b0 = bad;
  double ww = 0;
  char serr[256];
  if (c == NULL)
    return -1;
  for (d = 0; d < NDD; d++) {
    if (DD[d] & (SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX))
      continue;
    for (e = 0; e < 3; e++) {
      double w = 0;
      int nok = 0;
      for (m = 0; m < 2; m++)
        for (b = 0; b < NPL6; b++)
          for (i = 0; i < NTQ; i++) {
            double tu = TQ[i], te, x[6], xn[6], g, armc;
            int32 fl = EPHF[e] | DD[d];
            if (sector(c, tu, PL6[b], fl, m, &g) < 0)
              continue;
            te = tu + swe_deltat_ex_r(c, tu, fl, NULL);
            if (place(c, te, PL6[b], fl, x) < 0
                || swe_calc_r(c, te, SE_ECL_NUT, EPHF[e], xn, serr) < 0) {
              printf("    ERROR place of body %d, %s, %s at JD %.8f\n", PL6[b], EPHN[e], DDN[d],
                     tu);
              swe_ctx_free(c);
              return -1;
            }
            if (m == 1)
              x[1] = 0;
            armc = swe_degnorm(swe_sidtime0_r(c, tu, xn[0], xn[2]) * 15 + GEO[0]);
            w = fmax(w, sect_diff(g, swe_house_pos_r(c, armc, GEO[1], xn[0], 'G', x, NULL)));
            nok++;
          }
      n++;
      ww = fmax(ww, w);
      if (!(w <= 1e-11) || nok == 0) {
        bad++;
        printf("    FAIL %s, %s: %.2e sector off the place's own (%d calls)\n", EPHN[e], DDN[d],
               w, nok);
      }
    }
  }
  swe_ctx_free(c);
  printf("  (d) methods 0, 1 with a direction flag, the place's own sector within %.1e: %d"
         " checks, %d fail\n", ww, n - n0, bad - b0);
  return 0;
}

/* (e) methods 2 to 5 with each frame flag, the call without */
static int check_e(void)
{
  static const int pl[2] = { SE_MARS, SE_MOON };
  static const double tq[2] = { 2433282.5, 2451545.0 };
  swe_ctx *c = open_ctx(SE_SIDM_LAHIRI);
  int m, f, b, i, n0 = n, b0 = bad;
  if (c == NULL)
    return -1;
  for (m = 2; m < 6; m++) {
    int ndiff = 0, nok = 0;
    for (f = 0; f < NFF; f++)
      for (b = 0; b < 2; b++)
        for (i = 0; i < 2; i++) {
          double s0 = 0, s1 = 0;
          int32 r0 = sector(c, tq[i], pl[b], SEFLG_SWIEPH, m, &s0);
          int32 r1 = sector(c, tq[i], pl[b], SEFLG_SWIEPH | FF[f], m, &s1);
          if (r0 >= 0)
            nok++;
          if (r0 != r1 || memcmp(&s0, &s1, sizeof s0) != 0)
            ndiff++;
        }
    n++;
    if (ndiff || nok == 0) {
      bad++;
      printf("    FAIL method %d: %d of %d calls off the call without\n", m, ndiff, NFF * 4);
    }
  }
  swe_ctx_free(c);
  printf("  (e) methods 2 to 5 with a frame flag as without: %d checks, %d fail\n", n - n0,
         bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: gauqflags EPHEDIR\n");
    return 2;
  }
  EPH = argv[1];
  printf("G42: a Gauquelin sector moves with no flag but the ephemeris and the body's"
         " direction\n");
  r = check_a();
  if (r == 0)
    r = check_b();
  if (r == 0)
    r = check_c();
  if (r == 0)
    r = check_d();
  if (r == 0)
    r = check_e();
  if (r < 0) {
    printf("G42 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G42 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG42 PASS\n", n);
  return 0;
}
