/* housecusp.c -- G179: the houses of the ecliptic of t0 continuous across
 * t0, ascmc[8] 0 on the sidereal planes, and swe_house_pos() putting every
 * system's cusps at their numbers and its places in their houses, in both
 * hemispheres and on the equator.
 *
 * The defects (UPSTREAM-BUGS 287 to 289), at Zurich (47.37 N, 8.55 E):
 * 287. ecl_t0_aux() took the auxiliary vernal point's distance from t0's
 *   by acos(), 0 .. 180, and signed it by the date, negative before t0. It
 *   is t0's vernal point's longitude from the auxiliary one, about
 *   p (t - t0) + dpsi, whose sign is dpsi's for some 130 days on one side
 *   of t0: there every cusp and ascmc was 2 |dvpxe| off, a step of 2 dpsi
 *   at t0 (-9.64e-3 deg on the ecliptic of 1900 and J1900, -7.74e-3 on
 *   J2000's, -9.32e-3 on Lahiri's of 1956), the speeds within 1/64 day of
 *   it 0.308, 0.248 and 0.298 deg/day off; and acos() near 1 kept some
 *   1e-8 rad: 4.0e-9 deg where dvpxe passes 0, 1.4e-10 elsewhere.
 * 288. sidereal_houses_ecl_t0() and sidereal_houses_ssypl() moved
 *   ascmc[0 .. SE_NASCMC], so ascmc[8], 0 ("ascmc[8... 9] are 0",
 *   swephprg 15.4), came out the frames' offset: 336.84 deg (the ecliptic
 *   of 1900, Lahiri's), 359.30 (J1900), 0.70 (J2000), 340.40 (the solar
 *   system plane), UT 2433282.5.
 * 289. swe_house_pos() in the horizon system ('H') turned the place by
 *   90 - lat, where swe_houses() (CalcH()) takes -90 - lat south of the
 *   equator and on it: there the houses counted the other way from the
 *   cusps, the middles of houses 1 .. 12 at 33.87 S, UT 2433282.5, 12.50,
 *   11.52 .. 1.48, on the equator 12.56 .. 1.37.
 *
 * The checks:
 * (a) the user's ecliptic of 1900 (2415020.0, 22.46 deg), J1900, J2000
 *   and Lahiri's ecliptic of t0, at TT t0 -+ d, d = 2^-20, 1/64, 1, 10,
 *   50, 100, 125, 130 and 200 days: the sidereal Regiomontanus and
 *   Campanus cusps against a construction by vectors -- the t0 ecliptic's
 *   axes taken to the true equator of date by rotations (the obliquity of
 *   t0, the precession through J2000, the nutation), each house circle
 *   (through the horizon's north and south points and the equator's or
 *   the prime vertical's division point) cut with the t0 ecliptic on its
 *   division point's side, the cut's longitude in t0's frame less ayan_t0
 *   (Lahiri: plus its precession offset, the sidereal Sun's move with
 *   SE_SIDBIT_NO_PREC_OFFSET) -- within 1e-11 deg. One check per setting:
 *   4.
 * (b) those settings, the UT instants k / 256 day within 1/32 day of t0:
 *   the Placidus and Regiomontanus cusps' speeds and ascmc[0], [1], [3] ..
 *   [7]'s against five-point differences of the library's own places over
 *   2^-12 day, within 1e-7 deg/day. One check per setting: 4.
 * (c) the six settings of G172 (the two user ones, Lahiri's both, J1900,
 *   J2000), 'P', 'I', 'G', 'W', UT 2433282.5: ascmc[8] and its speed 0,
 *   ascmc[9] the tropical call's (Sunshine's declination, else 0). One
 *   check per setting: 6.
 * (d) tropical, at 47.37 N, 33.87 S, the equator, 60 N and 60 S, UT
 *   2378496.5, 2433282.5, 2451545.0, 2488069.5: in every system letter both
 *   functions take, A .. Y and a .. y, the cusps (latitude 0) at their
 *   numbers within 1e-7 house, and the places 1e-6, 0.5 and 1 - 1e-6 of
 *   the way through each house in it (k <= hpos < k + 1; Gauquelin's
 *   sectors clockwise), the first and last within 1e-3 of its cusps. One
 *   check per letter and latitude, 248, but Makransky's 'i' at 60 N and
 *   60 S (CalcH()'s own cusps 2 and 6, 8 and 12, off their house circles
 *   there by 27 and 38 deg: entry 289's notes) and the horizon system's
 *   charts with the MC beyond the zenith (in the tropics, on the equator
 *   UT 2451545.0: CalcH()'s cusps clockwise from the Vertex, ibid.).
 *
 * Measured: (a) 2.3e-13 deg; (b) 1.0e-8 deg/day (the difference's
 * truncation; the tropical speeds' the same); (c) 0; (d) every place in its
 * house, the cusps 0 (the exact-cusp test) and 2.8e-8 house (Gauquelin's
 * sectors past 12, the 0.001" swe_house_pos() adds). 262 checks. Before
 * the fix 20 fail: (a) 4 (9.64e-3, 7.74e-3, 9.32e-3 deg), (b) 4 (the
 * reference across the step), (c) 6, (d) 6 ('H' and 'h' at 33.87 S, the
 * equator and 60 S, 6 houses). The upstream build (SWE_UPSTREAM_COMPAT)
 * fails 30: those and Sripati's 'S' and 's' at the five latitudes (entry
 * 270). Run as
 *   housecusp EPHEDIR
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

/* the settings of (a) and (b), on the ecliptic of t0 */
static const struct {
  const char *name;
  int32 sid;
  double t0, a0;
} ET[] = {
  { "the ecliptic of 1900", SE_SIDM_USER | SE_SIDBIT_ECL_T0, 2415020.0, 22.46 },
  { "J1900", SE_SIDM_J1900, 0, 0 },
  { "J2000", SE_SIDM_J2000, 0, 0 },
  { "Lahiri's ecliptic of t0", SE_SIDM_LAHIRI | SE_SIDBIT_ECL_T0, 0, 0 },
};
#define NET ((int) (sizeof ET / sizeof ET[0]))

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

/* M's columns: the axes of the mean ecliptic of t0 in the true equator of
 * the TT tjde (the houses' frame: swi_epsiln(), swi_nutation(), the
 * precession through J2000) */
static void t0_axes(swe_ctx *c, double t0, double tjde, double M[3][3])
{
  double et0 = swi_epsiln(c, t0, 0), em = swi_epsiln(c, tjde, 0), nut[2], x[3], l, r;
  int j;
  swi_nutation(c, tjde, 0, nut);
  for (j = 0; j < 3; j++) {
    x[0] = j == 0;
    x[1] = j == 1;
    x[2] = j == 2;
    swi_coortrf(x, x, -et0);
    swi_precess(c, x, t0, 0, J_TO_J2000);
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

/* cusp k of Regiomontanus ('R') or Campanus ('C') at the ARMC armc: its
 * house circle, through the horizon's north and south points and the
 * equator's (R) or the prime vertical's (C) division point 30 (k - 1) deg
 * from the east point, cut with the t0 ecliptic on that point's side; the
 * cut's longitude in t0's frame, deg */
static double t0_cusp(double M[3][3], double armc, double lat, int hs, int k)
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

/* (a) bug 287: the houses against the vector construction either side
 * of t0 */
static int check_a(void)
{
  static const double DD[] = { 1.0 / 1048576, 1.0 / 64, 1, 10, 50, 100, 125, 130, 200 };
  int s, j, sg, hk, k, n0 = n, b0 = bad;
  char serr[256];
  for (s = 0; s < NET; s++) {
    swe_ctx *c = open_ctx(ET[s].sid, ET[s].t0, ET[s].a0);
    swe_ctx *c0 = open_ctx(ET[s].sid | SE_SIDBIT_NO_PREC_OFFSET, ET[s].t0, ET[s].a0);
    double w = 0, wd = 0, t0, a0;
    if (c == NULL || c0 == NULL) {
      swe_ctx_free(c);
      swe_ctx_free(c0);
      return -1;
    }
    t0 = c->sidd.t0;
    a0 = c->sidd.ayan_t0;
    for (j = 0; j < (int) (sizeof DD / sizeof DD[0]); j++) {
      for (sg = -1; sg <= 1; sg += 2) {
        double tu = ut_of_tt(c, t0 + sg * DD[j]);
        double tjde = tu + swe_deltat_ex_r(c, tu, SEFLG_SWIEPH, NULL);
        double M[3][3], corr, x1[6], x0[6], ww = 0;
        if (swe_calc_ut_r(c, tu, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL, x1, serr) < 0
            || swe_calc_ut_r(c0, tu, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL, x0, serr) < 0) {
          printf("    ERROR %s, the Sun: %s\n", ET[s].name, serr);
          swe_ctx_free(c);
          swe_ctx_free(c0);
          return -1;
        }
        corr = swe_difdeg2n(x1[0], x0[0]);
        t0_axes(c, t0, tjde, M);
        for (hk = 0; hk < 2; hk++) {
          int hs = "RC"[hk];
          double cu[13], am[10];
          if (swe_houses_ex2_r(c, tu, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cu, am, NULL,
                               NULL, serr) == ERR) {
            printf("    ERROR %s, '%c': %s\n", ET[s].name, hs, serr);
            swe_ctx_free(c);
            swe_ctx_free(c0);
            return -1;
          }
          for (k = 1; k <= 12; k++) {
            double ref = swe_degnorm(t0_cusp(M, am[2], LAT, hs, k) - a0 + corr);
            ww = fmax(ww, fabs(swe_difdeg2n(cu[k], ref)));
          }
        }
        if (ww > w) {
          w = ww;
          wd = tjde - t0;
        }
      }
    }
    verdict(w <= 1e-11, "the cusps off the vector construction (deg)", ET[s].name, w);
    printf("    %s: the cusps %.3e deg off the vector construction (at t0 %+.4f day)\n",
           ET[s].name, w, wd);
    swe_ctx_free(c);
    swe_ctx_free(c0);
  }
  printf("  (a) the ecliptic of t0 either side of t0: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* (b) bug 287: the speeds across t0 */
static int check_b(void)
{
  static const int AI[7] = { 0, 1, 3, 4, 5, 6, 7 };
  const double h = 1.0 / 4096;
  int s, j, hk, k, i, n0 = n, b0 = bad;
  char serr[256];
  for (s = 0; s < NET; s++) {
    swe_ctx *c = open_ctx(ET[s].sid, ET[s].t0, ET[s].a0);
    double w = 0, u0;
    int np = 0;
    if (c == NULL)
      return -1;
    u0 = floor(ut_of_tt(c, c->sidd.t0) * 256) / 256;
    for (j = -8; j <= 8; j++) {
      double tu = u0 + j / 256.0, tjde = tu + swe_deltat_ex_r(c, tu, SEFLG_SWIEPH, NULL);
      if (fabs(tjde - c->sidd.t0) > 1.0 / 32)
        continue;
      for (hk = 0; hk < 2; hk++) {
        int hs = "PR"[hk];
        double cu[13], am[10], cs[13], as[10], fc[5][13], fa[5][10];
        if (swe_houses_ex2_r(c, tu, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cu, am, cs, as,
                             serr) == ERR) {
          printf("    ERROR %s, '%c': %s\n", ET[s].name, hs, serr);
          swe_ctx_free(c);
          return -1;
        }
        for (k = 0; k < 5; k++) {
          if (swe_houses_ex2_r(c, tu + (k - 2) * h, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs,
                               fc[k], fa[k], NULL, NULL, serr) == ERR) {
            printf("    ERROR %s, '%c': %s\n", ET[s].name, hs, serr);
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
          w = fmax(w, fabs((i <= 12 ? cs[i] : as[AI[i - 13]]) - d));
        }
        np++;
      }
    }
    verdict(np > 0 && w <= 1e-7, "a speed off its places' rate (deg/day)", ET[s].name, w);
    printf("    %s: %d calls within 1/32 day of t0, the speeds %.3e deg/day off their places' rate\n",
           ET[s].name, np, w);
    swe_ctx_free(c);
  }
  printf("  (b) the speeds across t0: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* (c) bug 288: ascmc[8] on the sidereal planes */
static int check_c(void)
{
  static const struct {
    const char *name;
    int32 sid;
    double t0, a0;
  } PL[] = {
    { "the ecliptic of 1900", SE_SIDM_USER | SE_SIDBIT_ECL_T0, 2415020.0, 22.46 },
    { "Lahiri's ecliptic of t0", SE_SIDM_LAHIRI | SE_SIDBIT_ECL_T0, 0, 0 },
    { "J1900", SE_SIDM_J1900, 0, 0 },
    { "J2000", SE_SIDM_J2000, 0, 0 },
    { "the solar system plane", SE_SIDM_USER | SE_SIDBIT_SSY_PLANE, J2000, 23.86 },
    { "Lahiri's solar system plane", SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, 0, 0 },
  };
  int s, k, n0 = n, b0 = bad;
  char serr[256];
  for (s = 0; s < (int) (sizeof PL / sizeof PL[0]); s++) {
    swe_ctx *c = open_ctx(PL[s].sid, PL[s].t0, PL[s].a0);
    double w = 0;
    if (c == NULL)
      return -1;
    for (k = 0; k < 4; k++) {
      int hs = "PIGW"[k];
      double cu[37], am[10], cs[37], as[10], ct[37], at[10];
      if (swe_houses_ex2_r(c, 2433282.5, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cu, am, cs,
                           as, serr) == ERR
          || swe_houses_ex2_r(c, 2433282.5, SEFLG_SWIEPH, LAT, LON, hs, ct, at, NULL, NULL,
                              serr) == ERR) {
        printf("    ERROR %s, '%c': %s\n", PL[s].name, hs, serr);
        swe_ctx_free(c);
        return -1;
      }
      w = fmax(w, fmax(fabs(swe_difdeg2n(am[8], 0)), fabs(as[8])));
      w = fmax(w, fabs(am[9] - at[9]));
    }
    verdict(w == 0, "ascmc[8], its speed or ascmc[9] off (deg)", PL[s].name, w);
    printf("    %s: ascmc[8], its speed and ascmc[9] %.3e off\n", PL[s].name, w);
    swe_ctx_free(c);
  }
  printf("  (c) ascmc[8] on the sidereal planes: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* (d) bug 289: swe_house_pos() at every system's cusps and in its houses */
static int check_d(void)
{
  static const char HS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxy";
  static const double LT[] = { LAT, -33.87, 0, 60, -60 };
  static const double FR[3] = { 1e-6, 0.5, 1 - 1e-6 };
  int k, l, it, i, j, n0 = n, b0 = bad, nskip = 0;
  double wc = 0;
  char serr[256], name[32];
  swe_ctx *c = open_ctx(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  if (c == NULL)
    return -1;
  for (l = 0; l < (int) (sizeof LT / sizeof LT[0]); l++) {
    int n1 = n, b1 = bad;
    for (k = 0; HS[k] != '\0'; k++) {
      int hs = HS[k], cw = hs == 'G' || hs == 'g', nc = cw ? 36 : 12;
      double w = 0;
      if (hs == 'i' && fabs(LT[l]) == 60)
        continue;	/* Makransky's own cusps there: entry 289's notes */
      for (it = 0; it < NTU; it++) {
        double cu[37], am[10], x[6];
        if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LT[l], LON, hs, cu, am, NULL, NULL,
                             serr) == ERR
            || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
          printf("    ERROR '%c' at %.2f, UT %.1f: %s\n", hs, LT[l], TU[it], serr);
          swe_ctx_free(c);
          return -1;
        }
        if (hs == 'H' || hs == 'h') {
          /* CalcH()'s horizon houses take the MC on the equator's side of
           * the zenith (the hemisphere's co-latitude); in the tropics with
           * the MC beyond the zenith they run clockwise from the Vertex:
           * entry 289's notes */
          double xm[3] = { am[1], 0, 1 };
          swe_cotrans(xm, xm, -x[0]);
          if (LT[l] > 0 ? xm[1] > LT[l] : xm[1] < LT[l]) {
            nskip++;
            continue;
          }
        }
        for (i = 1; i <= nc; i++) {
          double span = cw ? swe_degnorm(cu[i] - cu[i % nc + 1])
                           : swe_degnorm(cu[i % nc + 1] - cu[i]);
          double xp[2], hp;
          xp[0] = cu[i];
          xp[1] = 0;
          hp = swe_house_pos_r(c, am[2], LT[l], x[0], hs, xp, serr);
          wc = fmax(wc, fabs(wrap(hp - i, nc)));
          if (!(fabs(wrap(hp - i, nc)) <= 1e-7))
            w = fmax(w, fabs(wrap(hp - i, nc)));
          if (span == 0)
            continue;
          for (j = 0; j < 3; j++) {
            xp[0] = swe_degnorm(cu[i] + (cw ? -1 : 1) * FR[j] * span);
            hp = swe_house_pos_r(c, am[2], LT[l], x[0], hs, xp, serr);
            if (!(hp >= i && hp < i + 1)
                || (j == 0 && hp - i > 1e-3) || (j == 2 && i + 1 - hp > 1e-3))
              w = fmax(w, fabs(wrap(hp - (i + FR[j]), nc)));
          }
        }
      }
      snprintf(name, sizeof name, "'%c' at %.2f", hs, LT[l]);
      verdict(w == 0, "a cusp off its number or a place outside its house (house)", name, w);
    }
    printf("    at %6.2f: %d letters, %d fail\n", LT[l], n - n1, bad - b1);
  }
  printf("    the cusps at most %.3e house off their numbers; %d horizon charts with the MC\n"
         "    beyond the zenith left out\n", wc, nskip);
  swe_ctx_free(c);
  printf("  (d) swe_house_pos() at the cusps and in the houses: %d checks, %d fail\n", n - n0,
         bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: housecusp EPHEDIR\n");
    return 2;
  }
  EPH = argv[1];
  printf("G179: the ecliptic of t0's houses across t0, ascmc[8] on the sidereal planes,\n"
         "      swe_house_pos() at every system's cusps in both hemispheres\n");
  r = check_a();
  if (r == 0)
    r = check_b();
  if (r == 0)
    r = check_c();
  if (r == 0)
    r = check_d();
  if (r < 0) {
    printf("G179 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G179 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG179 PASS\n", n);
  return 0;
}
