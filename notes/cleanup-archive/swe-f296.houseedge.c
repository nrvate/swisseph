/* houseedge.c -- G182: the solar system plane's sidereal houses past
 * 5650 BC and AD 7217, Koch's swe_house_pos() at the meridian of a place
 * with latitude, the MC and IC themselves among them, and Krusinski's on
 * the equator; every system's cusps given back past the exact-cusp test.
 *
 * The defects (UPSTREAM-BUGS 296, 299, 300), at Zurich (47.37 N, 8.55 E):
 * 296. ssypl_aux() took the plane's zero point's distance from the
 *   auxiliary vernal point by acos(), 0 .. 180. It is a longitude on the
 *   plane, 104.04 deg at J2000 and some 51" a year more, 0 in 5650 BC and
 *   180 in AD 7217; beyond them acos() folded it, and every cusp and ascmc
 *   was twice its distance from 0 or 180 off: @A296@.
 * 299. Koch's swe_house_pos() failed ("in circumpolar area", 0) for a
 *   place whose dfac fell outside [0, 2]: one that rises and sets does so
 *   only within adp - admc of the meridian (-adp - admc of the lower),
 *   where its semi-arc is the longer, and the rising time (east) puts it
 *   in house 9, the setting time (west) in house 10 (the IC: 4 and 3), as
 *   swisseph.htm 6.5 says Koch's places do. So the MC with any positive
 *   latitude north of the tropics' south edge failed on every date, the IC
 *   with a negative one (the south the other way), and with 1e-300, where
 *   dfac is 0 or 2 and rounds past it, @A299@.
 * 300. Krusinski's ('U') swe_house_pos() took its house circle's node and
 *   inclination through the horizon, the node raised 90 deg in altitude;
 *   near the equator the node nears the zenith, the horizon's pole: @A300@.
 *
 * The checks:
 * (a) the solar system plane, the user's (t0 J2000, 23.86 deg) and
 *   Lahiri's, at 20 dates 13000 BC .. AD 17000 and 1/64 and 1 day either
 *   side of the two folds (found by bisection on the construction): the
 *   sidereal Regiomontanus and Campanus cusps and the Ascendant against a
 *   construction by vectors -- the plane's axes (its node on the ecliptic
 *   of J2000, 107.582569 deg, inclination 1.578701 deg) taken to the true
 *   equator of date by rotations (the obliquity of J2000, the precession,
 *   the nutation), each house circle cut with the plane, the cut's
 *   longitude from the node less t0's vernal point's and ayan_t0 (Lahiri:
 *   plus its precession offset, the sidereal Sun's move with
 *   SE_SIDBIT_NO_PREC_OFFSET) -- within 1e-11 deg. One check per setting
 *   and date: 56.
 * (b) those settings, the UT instants k / 256 day within 1/32 day of each
 *   fold, and five either side of 8000 BC and AD 10000: the Placidus and
 *   Regiomontanus cusps' speeds and ascmc[0], [1], [3] .. [7]'s against
 *   five-point differences of the library's own places over 2^-12 day,
 *   within 1e-7 deg/day. One check per setting and epoch: 8.
 * (c) Koch at 47.37 N and S, 33.87 S, 60 N and S, 10 N and S and the
 *   equator, UT 2378496.5, 2433282.5, 2451545.0, 2488069.5: cusps 10 and
 *   4 given with latitude -+1e-300, 1e-12, 1e-9, 1e-6 at their numbers
 *   within 1e-7 house; with -+1 and 5 deg the rising-time (east) or
 *   setting-time (west) measure, 90 deg times the place's time from
 *   rising or to setting over the MC's semi-arc, from its own right
 *   ascension and declination, within 1e-9 house, or 0 where that lies
 *   past the neighbouring house. One check per latitude: 8.
 * (d) Krusinski at the equator, 1e-12 N, 1e-9 S, 1e-6 N, 47.37 N, 33.87
 *   S, 60 N and S, (c)'s dates: the cusps given with latitude 1e-300 at
 *   their numbers (plus the 0.001" swe_house_pos() adds) within 1e-12
 *   house, and places at 48 longitudes and latitudes -6 .. 6 deg against
 *   a construction by vectors (the place's hour circle cut with the
 *   circle through the Asc and the zenith, its arc from the Asc towards
 *   the nadir) within 1e-12 house. One check per latitude: 8.
 * (e) tropical, (c)'s latitudes and dates, every letter both functions
 *   take, A .. Y and a .. y: the cusps given with latitude 1e-300, past
 *   the exact-cusp test, at their numbers within 1e-7 house. One check
 *   per letter and latitude, 398: Makransky's 'i' at 60 N and S and the
 *   horizon system's charts with the MC beyond the zenith left out
 *   (UPSTREAM-BUGS 289's notes).
 *
 * Measured: @MEAS@
 * Run as
 *   houseedge EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"

static int n, bad;
static const char *EPH;

#define LAT 47.37
#define LON 8.55
static const double TU[] = { 2378496.5, 2433282.5, 2451545.0, 2488069.5 };
#define NTU ((int) (sizeof TU / sizeof TU[0]))
/* (c) and (e)'s latitudes */
static const double LK[] = { LAT, -LAT, -33.87, 60, -60, 10, -10, 0 };
#define NLK ((int) (sizeof LK / sizeof LK[0]))
/* the 0.001" swe_house_pos() adds, in houses */
#define MAS_H (0.001 / 3600 / 30)

/* the solar system plane, as sweph.h has it */
#define NODE (107.582569 * DEGTORAD)
#define INCL (1.578701 * DEGTORAD)

static const struct {
  const char *name;
  int32 sid;
  double t0, a0;
} SP[] = {
  { "the solar system plane", SE_SIDM_USER | SE_SIDBIT_SSY_PLANE, J2000, 23.86 },
  { "Lahiri's solar system plane", SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, 0, 0 },
};
#define NSP ((int) (sizeof SP / sizeof SP[0]))

/* the larger of w and d, a NaN d the largest */
static double worse(double w, double d)
{
  return d != d ? HUGE_VAL : fmax(w, d);
}

/* d wrapped into (-nc/2, nc/2] */
static double wrap(double d, double nc)
{
  d = fmod(d, nc);
  if (d > nc / 2)
    d -= nc;
  if (d <= -nc / 2)
    d += nc;
  return d;
}

static swe_ctx *open_ctx(int32 sid, double t0, double ayan_t0)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  swe_set_ephe_path_r(c, EPH);
  swe_set_sid_mode_r(c, sid, t0, ayan_t0);
  return c;
}

static void verdict(int ok, const char *what, const char *setting, double v)
{
  n++;
  if (!ok) {
    bad++;
    printf("    FAIL %s, %s: %.3e\n", what, setting, v);
  }
}

/* the UT of the TT tt */
static double ut_of_tt(swe_ctx *c, double tt)
{
  double u = tt;
  int i;
  for (i = 0; i < 4; i++)
    u = tt - swe_deltat_ex_r(c, u, SEFLG_SWIEPH, NULL);
  return u;
}

static void cross(const double *a, const double *b, double *x)
{
  x[0] = a[1] * b[2] - a[2] * b[1];
  x[1] = a[2] * b[0] - a[0] * b[2];
  x[2] = a[0] * b[1] - a[1] * b[0];
}

static double dot(const double *a, const double *b)
{
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/* M's columns: the solar system plane's axes (x its node on the ecliptic
 * of J2000, z its pole) in the true equator of the TT tjde */
static void ss_axes(swe_ctx *c, double tjde, double M[3][3])
{
  double e2 = swi_epsiln(c, J2000, 0), em = swi_epsiln(c, tjde, 0), nut[2], x[3], l, r;
  int j;
  swi_nutation(c, tjde, 0, nut);
  for (j = 0; j < 3; j++) {
    double v[3] = { j == 0, j == 1, j == 2 }, w[3];
    w[0] = v[0];
    w[1] = v[1] * cos(INCL) - v[2] * sin(INCL);
    w[2] = v[1] * sin(INCL) + v[2] * cos(INCL);
    x[0] = w[0] * cos(NODE) - w[1] * sin(NODE);
    x[1] = w[0] * sin(NODE) + w[1] * cos(NODE);
    x[2] = w[2];
    swi_coortrf(x, x, -e2);
    swi_precess(c, x, tjde, 0, J2000_TO_J);
    swi_coortrf(x, x, em);
    l = atan2(x[1], x[0]) + nut[0];
    r = hypot(x[0], x[1]);
    x[0] = r * cos(l);
    x[1] = r * sin(l);
    swi_coortrf(x, x, -(em + nut[1]));
    M[0][j] = x[0];
    M[1][j] = x[1];
    M[2][j] = x[2];
  }
}

/* the vernal point of t0's longitude on the plane, from the node, deg */
static double ss_vp_lon(swe_ctx *c, double t0)
{
  double x[3] = { 1, 0, 0 }, X[3], Y[3];
  if (t0 != J2000)
    swi_precess(c, x, t0, 0, J_TO_J2000);
  swi_coortrf(x, x, swi_epsiln(c, J2000, 0));
  X[0] = cos(NODE);
  X[1] = sin(NODE);
  X[2] = 0;
  Y[0] = -sin(NODE) * cos(INCL);
  Y[1] = cos(NODE) * cos(INCL);
  Y[2] = sin(INCL);
  return atan2(dot(x, Y), dot(x, X)) * RADTODEG;
}

/* the TT within lo .. hi where the auxiliary vernal point (the plane's
 * crossing of the true equator northwards) passes the plane's node or its
 * opposite: the node's height above the equator 0 */
static double fold_tt(swe_ctx *c, double lo, double hi)
{
  double M[3][3], slo;
  int i;
  ss_axes(c, lo, M);
  slo = M[2][0];
  for (i = 0; i < 60; i++) {
    double m = (lo + hi) / 2;
    ss_axes(c, m, M);
    if ((M[2][0] < 0) == (slo < 0))
      lo = m;
    else
      hi = m;
  }
  return (lo + hi) / 2;
}

/* cusp k of Regiomontanus ('R') or Campanus ('C') at the ARMC armc: its
 * house circle, through the horizon's north and south points and the
 * equator's (R) or the prime vertical's (C) division point 30 (k - 1) deg
 * from the east point, cut with the plane on that point's side; the cut's
 * longitude on the plane from its node, deg */
static double ss_cusp(double M[3][3], double armc, double lat, int hs, int k)
{
  double A = armc * DEGTORAD, f = lat * DEGTORAD, th = 30 * (k - 1) * DEGTORAD;
  double Z[3], hN[3], P[3], p[3], d[3], q[3], e[3], s;
  double n0[3] = { M[0][2], M[1][2], M[2][2] };
  int i;
  Z[0] = cos(f) * cos(A);
  Z[1] = cos(f) * sin(A);
  Z[2] = sin(f);
  hN[0] = -sin(f) * cos(A);
  hN[1] = -sin(f) * sin(A);
  hN[2] = cos(f);
  if (hs == 'R') {
    P[0] = cos(A + M_PI / 2 + th);
    P[1] = sin(A + M_PI / 2 + th);
    P[2] = 0;
  } else {
    double E[3] = { -sin(A), cos(A), 0 };
    for (i = 0; i < 3; i++)
      P[i] = cos(th) * E[i] - sin(th) * Z[i];
  }
  cross(hN, P, p);
  cross(n0, p, d);
  s = dot(P, hN);
  for (i = 0; i < 3; i++)
    q[i] = P[i] - s * hN[i];
  if (dot(d, q) < 0) {
    for (i = 0; i < 3; i++)
      d[i] = -d[i];
  }
  for (i = 0; i < 3; i++)
    e[i] = M[0][i] * d[0] + M[1][i] * d[1] + M[2][i] * d[2];
  return swe_degnorm(atan2(e[1], e[0]) * RADTODEG);
}

/* the sidereal Sun's move with SE_SIDBIT_NO_PREC_OFFSET, deg */
static int prec_corr(int s, double *corr)
{
  char serr[256];
  double x1[6], x0[6];
  swe_ctx *c = open_ctx(SP[s].sid, SP[s].t0, SP[s].a0);
  swe_ctx *c0 = open_ctx(SP[s].sid | SE_SIDBIT_NO_PREC_OFFSET, SP[s].t0, SP[s].a0);
  int r = -1;
  if (c != NULL && c0 != NULL
      && swe_calc_ut_r(c, J2000, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL, x1, serr) >= 0
      && swe_calc_ut_r(c0, J2000, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL, x0, serr) >= 0) {
    *corr = swe_difdeg2n(x1[0], x0[0]);
    r = 0;
  }
  swe_ctx_free(c);
  swe_ctx_free(c0);
  return r;
}

/* the two folds' TT: 5650 BC and AD 7217 */
static void folds(swe_ctx *c, double *fp, double *ff)
{
  *fp = fold_tt(c, J2000 - 7700 * 365.25, J2000 - 7600 * 365.25);
  *ff = fold_tt(c, J2000 + 5200 * 365.25, J2000 + 5240 * 365.25);
}

/* (a) bug 296: the plane's houses against the vector construction */
static int check_a(void)
{
  static const double YR[] = { -12999, -9999, -7999, -6999, -5799, -5659, -5599, -4999, -1999,
                               1, 1000, 2000, 3000, 5000, 7000, 7500, 8000, 10000, 14000, 17000 };
  static const double DD[] = { -1, -1.0 / 64, 1.0 / 64, 1 };
  int s, j, hk, k, n0 = n, b0 = bad;
  char serr[256], name[96];
  for (s = 0; s < NSP; s++) {
    swe_ctx *c = open_ctx(SP[s].sid, SP[s].t0, SP[s].a0);
    double corr, vl, a0, fp, ff, w = 0;
    int nd = (int) (sizeof YR / sizeof YR[0]) + 8;
    if (c == NULL || prec_corr(s, &corr) < 0) {
      swe_ctx_free(c);
      return -1;
    }
    vl = ss_vp_lon(c, c->sidd.t0);
    a0 = c->sidd.ayan_t0;
    folds(c, &fp, &ff);
    for (j = 0; j < nd; j++) {
      double tu, tjde, M[3][3], ww = 0;
      if (j < (int) (sizeof YR / sizeof YR[0])) {
        tu = J2000 + (YR[j] - 2000) * 365.25;
        snprintf(name, sizeof name, "%s, year %.0f", SP[s].name, YR[j]);
      } else {
        int i = j - (int) (sizeof YR / sizeof YR[0]);
        double f = i < 4 ? fp : ff;
        tu = ut_of_tt(c, f + DD[i % 4]);
        snprintf(name, sizeof name, "%s, the fold of %s %+.4f day", SP[s].name,
                 i < 4 ? "5650 BC" : "AD 7217", DD[i % 4]);
      }
      tjde = tu + swe_deltat_ex_r(c, tu, SEFLG_SWIEPH, NULL);
      ss_axes(c, tjde, M);
      for (hk = 0; hk < 2; hk++) {
        int hs = "RC"[hk];
        double cu[13], am[10];
        if (swe_houses_ex2_r(c, tu, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cu, am, NULL,
                             NULL, serr) == ERR) {
          printf("    ERROR %s, '%c': %s\n", name, hs, serr);
          swe_ctx_free(c);
          return -1;
        }
        for (k = 1; k <= 12; k++) {
          double ref = swe_degnorm(ss_cusp(M, am[2], LAT, hs, k) - vl - a0 + corr);
          ww = worse(ww, fabs(swe_difdeg2n(cu[k], ref)));
          if (k == 1)
            ww = worse(ww, fabs(swe_difdeg2n(am[0], ref)));
        }
      }
      verdict(ww <= 1e-11, "the cusps off the vector construction (deg)", name, ww);
      w = fmax(w, ww);
    }
    printf("    %s: the cusps at most %.3e deg off the vector construction\n", SP[s].name, w);
    swe_ctx_free(c);
  }
  printf("  (a) the solar system plane 13000 BC .. AD 17000: %d checks, %d fail\n", n - n0,
         bad - b0);
  return 0;
}

/* (b) bug 296: the speeds across the folds and beyond them */
static int check_b(void)
{
  static const int AI[7] = { 0, 1, 3, 4, 5, 6, 7 };
  static const char *EP[4] = { "the fold of 5650 BC", "the fold of AD 7217", "8000 BC",
                               "AD 10000" };
  const double h = 1.0 / 4096;
  int s, e, j, hk, k, i, n0 = n, b0 = bad;
  char serr[256], name[96];
  for (s = 0; s < NSP; s++) {
    swe_ctx *c = open_ctx(SP[s].sid, SP[s].t0, SP[s].a0);
    double fp, ff;
    if (c == NULL)
      return -1;
    folds(c, &fp, &ff);
    for (e = 0; e < 4; e++) {
      double tc = e == 0 ? fp : e == 1 ? ff : J2000 + (e == 2 ? -7999 - 2000 : 10000 - 2000) * 365.25;
      double u0 = floor(ut_of_tt(c, tc) * 256) / 256, w = 0;
      int np = 0;
      for (j = -8; j <= 8; j++) {
        double tu = u0 + j / 256.0, tjde = tu + swe_deltat_ex_r(c, tu, SEFLG_SWIEPH, NULL);
        if (fabs(tjde - tc) > (e < 2 ? 1.0 / 32 : 3.0 / 256))
          continue;
        for (hk = 0; hk < 2; hk++) {
          int hs = "PR"[hk];
          double cu[13], am[10], cs[13], as[10], fc[5][13], fa[5][10];
          if (swe_houses_ex2_r(c, tu, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cu, am, cs,
                               as, serr) == ERR) {
            printf("    ERROR %s, '%c': %s\n", SP[s].name, hs, serr);
            swe_ctx_free(c);
            return -1;
          }
          for (k = 0; k < 5; k++) {
            if (swe_houses_ex2_r(c, tu + (k - 2) * h, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON,
                                 hs, fc[k], fa[k], NULL, NULL, serr) == ERR) {
              printf("    ERROR %s, '%c': %s\n", SP[s].name, hs, serr);
              swe_ctx_free(c);
              return -1;
            }
          }
          for (i = 1; i <= 12 + 7; i++) {
            double v[5], d;
            for (k = 0; k < 5; k++)
              v[k] = i <= 12 ? swe_difdeg2n(fc[k][i], cu[i])
                             : swe_difdeg2n(fa[k][AI[i - 13]], am[AI[i - 13]]);
            d = (8 * (v[3] - v[1]) - (v[4] - v[0])) / (12 * h);
            w = worse(w, fabs((i <= 12 ? cs[i] : as[AI[i - 13]]) - d));
          }
          np++;
        }
      }
      snprintf(name, sizeof name, "%s, %s", SP[s].name, EP[e]);
      verdict(np > 0 && w <= 1e-7, "a speed off its places' rate (deg/day)", name, w);
      printf("    %s: %d calls, the speeds %.3e deg/day off their places' rate\n", name, np, w);
    }
    swe_ctx_free(c);
  }
  printf("  (b) the plane's speeds across the folds and beyond: %d checks, %d fail\n", n - n0,
         bad - b0);
  return 0;
}

/* the Koch measure of the place (lon, lat): 90 deg times its time from
 * rising (east) or to setting (west) over the MC's semi-arc, from the
 * east point; its dfac (0 .. 2 within the half of the houses its side
 * gives); -1 for a place that neither rises nor sets */
static double koch_measure(double armc, double geolat, double eps, double lon, double lat,
                           double *dfac)
{
  double e = eps * DEGTORAD, l = lon * DEGTORAD, b = lat * DEGTORAD, f = geolat * DEGTORAD;
  double v[3], ra, de, md, ad, admc, samc, xp;
  v[0] = cos(b) * cos(l);
  v[1] = cos(b) * sin(l) * cos(e) - sin(b) * sin(e);
  v[2] = cos(b) * sin(l) * sin(e) + sin(b) * cos(e);
  ra = atan2(v[1], v[0]) * RADTODEG;
  de = asin(v[2]);
  md = swe_degnorm(ra - armc);
  if (md >= 180)
    md -= 360;
  if (fabs(tan(f) * tan(de)) > 1)
    return -1;
  ad = asin(tan(f) * tan(de)) * RADTODEG;
  admc = asin(tan(e) * tan(f) * sin(armc * DEGTORAD)) * RADTODEG;
  samc = 90 + admc;
  if (md >= 0) {	/* east: it rose (90 + ad - md) ago */
    xp = -90 * (90 + ad - md) / samc;
    *dfac = 1 + xp / 90;
  } else {	/* west: it sets in 90 + ad + md */
    xp = 180 + 90 * (90 + ad + md) / samc;
    *dfac = (xp - 90) / 90;
  }
  return swe_degnorm(xp) / 30 + 1;
}

/* (c) bug 299: Koch at the meridian */
static int check_c(void)
{
  static const double BL[] = { 1e-300, 1e-12, 1e-9, 1e-6, 1, 5, 20 };
  int l, it, k, j, sg, n0 = n, b0 = bad, nf = 0, nfx = 0, ncp = 0;
  double wm = 0, wk = 0;
  char serr[256], name[32];
  swe_ctx *c = open_ctx(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  if (c == NULL)
    return -1;
  for (l = 0; l < NLK; l++) {
    double w = 0;
    for (it = 0; it < NTU; it++) {
      double cu[13], am[10], x[6];
      if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LK[l], LON, 'K', cu, am, NULL, NULL,
                           serr) == ERR
          || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
        printf("    ERROR 'K' at %.2f, UT %.1f: %s\n", LK[l], TU[it], serr);
        swe_ctx_free(c);
        return -1;
      }
      for (k = 4; k <= 10; k += 6) {
        for (sg = -1; sg <= 1; sg += 2) {
          for (j = 0; j < (int) (sizeof BL / sizeof BL[0]); j++) {
            double xp[2] = { cu[k], sg * BL[j] }, hp, ref, df, d;
            ref = koch_measure(am[2], LK[l], x[0], xp[0], xp[1], &df);
            if (ref < 0) {	/* circumpolar: upstream's, unchanged */
              ncp++;
              continue;
            }
            ref = swe_degnorm((ref - 1) * 30 + 0.001 / 3600) / 30 + 1;
            hp = swe_house_pos_r(c, am[2], LK[l], x[0], 'K', xp, serr);
            if (df < -1.0 / 3 || df > 2 + 1.0 / 3) {
              nfx++;
              if (hp != 0)
                w = fmax(w, fabs(hp));
              continue;
            }
            if (hp == 0)
              nf++;
            d = fabs(wrap(hp - ref, 12));
            wm = worse(wm, d);
            if (!(d <= 1e-9))
              w = worse(w, d);
            if (BL[j] <= 1e-9) {	/* the MC and IC themselves */
              d = fabs(wrap(hp - k, 12));
              wk = worse(wk, d);
              if (!(d <= 1e-7))
                w = worse(w, d);
            }
          }
        }
      }
    }
    snprintf(name, sizeof name, "'K' at %.2f", LK[l]);
    verdict(w == 0, "the MC or IC with latitude off its measure or number (house)", name, w);
  }
  printf("    with latitude up to 1e-9 at most %.3e house off their numbers; %d failed\n"
         "    within the neighbouring house, the measures met within %.3e house; %d past\n"
         "    it, failed as they should; %d circumpolar left out\n", wk, nf, wm, nfx, ncp);
  swe_ctx_free(c);
  printf("  (c) Koch at the meridian: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* the Krusinski position of the equatorial unit vector X: its hour
 * circle's cut with the circle through the Asc A and the zenith Z, the
 * cut's arc from A towards the nadir, in houses */
static double krus_pos(const double *A, const double *Z, const double *X)
{
  double nn[3], P[3] = { 0, 0, 1 }, m[3], d[3], th;
  cross(A, Z, nn);
  cross(P, X, m);
  cross(nn, m, d);
  if (d[0] * X[0] + d[1] * X[1] < 0) {
    d[0] = -d[0];
    d[1] = -d[1];
    d[2] = -d[2];
  }
  th = atan2(-dot(d, Z), dot(d, A)) * RADTODEG;
  return swe_degnorm(th) / 30 + 1;
}

/* the equatorial unit vector of the ecliptic (lon, lat) */
static void eq_vec(double lon, double lat, double eps, double *v)
{
  double x[3] = { lon, lat, 1 };
  swe_cotrans(x, x, -eps);
  v[0] = cos(x[1] * DEGTORAD) * cos(x[0] * DEGTORAD);
  v[1] = cos(x[1] * DEGTORAD) * sin(x[0] * DEGTORAD);
  v[2] = sin(x[1] * DEGTORAD);
}

/* (d) bug 300: Krusinski on the equator */
static int check_d(void)
{
  static const double LU[] = { 0, 1e-12, -1e-9, 1e-6, LAT, -33.87, 60, -60 };
  int l, it, k, j, n0 = n, b0 = bad;
  double wc = 0, wp = 0;
  char serr[256], name[32];
  swe_ctx *c = open_ctx(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  if (c == NULL)
    return -1;
  for (l = 0; l < (int) (sizeof LU / sizeof LU[0]); l++) {
    double w = 0, f = LU[l] * DEGTORAD;
    for (it = 0; it < NTU; it++) {
      double cu[13], am[10], x[6], A[3], Z[3];
      if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LU[l], LON, 'U', cu, am, NULL, NULL,
                           serr) == ERR
          || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
        printf("    ERROR 'U' at %g, UT %.1f: %s\n", LU[l], TU[it], serr);
        swe_ctx_free(c);
        return -1;
      }
      Z[0] = cos(f) * cos(am[2] * DEGTORAD);
      Z[1] = cos(f) * sin(am[2] * DEGTORAD);
      Z[2] = sin(f);
      eq_vec(am[0], 0, x[0], A);
      for (k = 1; k <= 12; k++) {
        double xp[2] = { cu[k], 1e-300 }, hp, d;
        hp = swe_house_pos_r(c, am[2], LU[l], x[0], 'U', xp, serr);
        d = fabs(wrap(hp - k - MAS_H, 12));
        wc = worse(wc, d);
        if (!(d <= 1e-12))
          w = worse(w, d);
      }
      for (k = 0; k < 48; k++) {
        for (j = -3; j <= 3; j++) {
          double xp[2] = { k * 7.5 + 0.3, 2.0 * j }, X[3], hp, d;
          eq_vec(xp[0], xp[1], x[0], X);
          hp = swe_house_pos_r(c, am[2], LU[l], x[0], 'U', xp, serr);
          d = fabs(wrap(hp - MAS_H - krus_pos(A, Z, X), 12));
          wp = worse(wp, d);
          if (!(d <= 1e-12))
            w = worse(w, d);
        }
      }
    }
    snprintf(name, sizeof name, "'U' at %g", LU[l]);
    verdict(w == 0, "a cusp off its number or a place off the construction (house)", name, w);
  }
  printf("    the cusps at most %.3e house off their numbers, the places %.3e off the\n"
         "    construction\n", wc, wp);
  swe_ctx_free(c);
  printf("  (d) Krusinski on the equator: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* (e) every system's cusps given back past the exact-cusp test */
static int check_e(void)
{
  static const char HS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxy";
  int k, l, it, i, n0 = n, b0 = bad, nskip = 0;
  double wc = 0;
  char serr[256], name[32];
  swe_ctx *c = open_ctx(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  if (c == NULL)
    return -1;
  for (l = 0; l < NLK; l++) {
    for (k = 0; HS[k] != '\0'; k++) {
      int hs = HS[k], nc = (hs == 'G' || hs == 'g') ? 36 : 12;
      double w = 0;
      if (hs == 'i' && fabs(LK[l]) == 60)
        continue;	/* Makransky's own cusps there: entry 289's notes */
      for (it = 0; it < NTU; it++) {
        double cu[37], am[10], x[6];
        if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LK[l], LON, hs, cu, am, NULL, NULL,
                             serr) == ERR
            || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
          printf("    ERROR '%c' at %.2f, UT %.1f: %s\n", hs, LK[l], TU[it], serr);
          swe_ctx_free(c);
          return -1;
        }
        if (hs == 'H' || hs == 'h') {
          /* in the tropics with the MC beyond the zenith CalcH()'s
           * horizon houses run clockwise from the Vertex: entry 289's
           * notes */
          double xm[3] = { am[1], 0, 1 };
          swe_cotrans(xm, xm, -x[0]);
          if (LK[l] > 0 ? xm[1] > LK[l] : xm[1] < LK[l]) {
            nskip++;
            continue;
          }
        }
        for (i = 1; i <= nc; i++) {
          double xp[2] = { cu[i], 1e-300 }, hp, d;
          hp = swe_house_pos_r(c, am[2], LK[l], x[0], hs, xp, serr);
          d = fabs(wrap(hp - i, nc));
          if (!(d <= 1e-7))
            w = worse(w, d);
          else
            wc = fmax(wc, d);
        }
      }
      snprintf(name, sizeof name, "'%c' at %.2f", hs, LK[l]);
      verdict(w == 0, "a cusp off its number (house)", name, w);
    }
  }
  printf("    the cusps at most %.3e house off their numbers; %d horizon charts with the MC\n"
         "    beyond the zenith left out\n", wc, nskip);
  swe_ctx_free(c);
  printf("  (e) every system's cusps given back with latitude 1e-300: %d checks, %d fail\n",
         n - n0, bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: houseedge EPHEDIR\n");
    return 2;
  }
  EPH = argv[1];
  printf("G182: the solar system plane's houses past its folds, Koch at the meridian,\n"
         "      Krusinski on the equator, every system's cusps given back\n");
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
    printf("G182 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G182 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG182 PASS\n", n);
  return 0;
}
