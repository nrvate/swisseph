/* sidplanehouses.c -- G43: the sidereal houses of the ecliptic of t0 and
 * of the solar system plane (SE_SIDBIT_ECL_T0, SE_SIDBIT_SSY_PLANE, and
 * the J2000, J1900 and B1950 modes always) give the ARMC, whole signs at
 * sidereal sign starts, no step at t0, and ascmc[8] 0.
 *
 * These houses are computed in an auxiliary frame (the true equator of
 * date and the sidereal plane, cut at the auxiliary vernal point, at the
 * auxiliary ARMC) and moved to the sidereal plane by the auxiliary vernal
 * point's sidereal longitude. Four things did not survive the move:
 * - ascmc[2] was the auxiliary ARMC, not the ARMC: 0.0043 deg off on the
 *   ecliptic of 1900 and J1900, 0.0085 J2000's, 3.90 on the solar system
 *   plane; given it, swe_house_pos() put the sidereal cusps' places up to
 *   0.34 house off their numbers;
 * - whole signs were the auxiliary frame's signs moved, 8.94 deg off a
 *   sidereal sign start on the ecliptic of 1900, 20.3 on the solar system
 *   plane; 'n' was not taken as 'N', 0 Aries moved by up to 25.3 deg;
 * - the auxiliary vernal point's distance from t0's was acos(), 0 .. 180,
 *   signed by the date; it is about p (t - t0) + dpsi, whose sign is
 *   dpsi's for some 130 days on one side of t0: there every cusp and ascmc
 *   was off by twice it, a step of 2 dpsi at t0 (-9.64e-3 deg on the
 *   ecliptic of 1900, -7.74e-3 on J2000's, -9.32e-3 on Lahiri's);
 * - the loops that move ascmc ran to i <= SE_NASCMC and moved ascmc[8],
 *   which is 0, by the frames' offset: 336.84 deg on the ecliptic of 1900,
 *   0.70 J2000's, 340.40 on the solar system plane.
 *
 * The checks, at Zurich (47.37 N, 8.55 E):
 * (a) the user's ecliptic of 1900 (2415020.0, 22.46 deg), Lahiri's
 *   ecliptic of t0, J1900, J2000, the user's solar system plane (J2000,
 *   23.86) and Lahiri's, UT 2378496.5, 2433282.5, 2451545.0, 2488069.5:
 *   the sidereal Placidus call's ascmc[2] and its speed the tropical
 *   call's, bit for bit (one check per setting); for the two user
 *   settings, the sidereal cusps carried to the true ecliptic of date
 *   through the sidereal places' frame and given to swe_house_pos() with
 *   the sidereal call's ascmc[2] at their numbers within 1e-7 house in
 *   P R C H X (one check per setting and system). 16 checks.
 * (b) those settings and dates, 'W' 'w' 'N' 'n' 'E' 'A' 'D' 'V': whole
 *   signs' cusps at sign starts 30 deg apart, cusp 1 the start of
 *   ascmc[0]'s sign, their speeds 0; 0 Aries' (i - 1) 30; equal houses'
 *   cusp 1 ascmc[0] ('D': cusp 10 ascmc[1], 'V': ascmc[0] - 15), 30 deg
 *   apart; within 1e-9 deg. One check per setting and system: 48.
 * (c) the ecliptic of 1900, J1900, J2000 and Lahiri's ecliptic of t0, at
 *   TT t0 -+ d, d = 2^-20, 1/64, 1, 10, 50, 100, 125, 130 and 200 days:
 *   the Regiomontanus and Campanus cusps against a construction by
 *   vectors -- the t0 ecliptic's axes taken to the true equator of date
 *   (the obliquity of t0, the precession through J2000, the nutation),
 *   each house circle cut with the t0 ecliptic on its division point's
 *   side, the cut's longitude in t0's frame less ayan_t0 -- within 1e-11
 *   deg. The houses are asked with SE_SIDBIT_NO_PREC_OFFSET, so that
 *   Lahiri's precession offset stays out of the comparison. One check per
 *   setting: 4.
 * (d) the six settings of (a), 'P', 'I', 'G', 'W', UT 2433282.5: ascmc[8]
 *   and its speed 0, ascmc[9] the tropical call's (Sunshine's
 *   declination, else 0). One check per setting: 6.
 *
 * Run as
 *   sidplanehouses EPHEDIR
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

/* the sidereal planes: mode 1 the ecliptic of t0, 2 the solar system plane;
 * user settings carry their t0 and ayan_t0 */
static const struct {
  const char *name;
  int mode;
  int32 sid;
  double t0, a0;
} PL[] = {
  { "the ecliptic of 1900", 1, SE_SIDM_USER | SE_SIDBIT_ECL_T0, 2415020.0, 22.46 },
  { "Lahiri's ecliptic of t0", 1, SE_SIDM_LAHIRI | SE_SIDBIT_ECL_T0, 0, 0 },
  { "J1900", 1, SE_SIDM_J1900, 0, 0 },
  { "J2000", 1, SE_SIDM_J2000, 0, 0 },
  { "the solar system plane", 2, SE_SIDM_USER | SE_SIDBIT_SSY_PLANE, J2000, 23.86 },
  { "Lahiri's solar system plane", 2, SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, 0, 0 },
};
#define NPL ((int) (sizeof PL / sizeof PL[0]))

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

static void verdict(int ok, const char *what, const char *setting, int hs, double v)
{
  n++;
  if (!ok) {
    bad++;
    printf("    FAIL %s, %s, '%c': %.3e\n", what, setting, hs, v);
  }
}

/* a point of the sidereal plane (the ecliptic of t0, mode 1, or the solar
 * system plane, mode 2) to the true ecliptic of date at the TT tjde, the
 * frame the sidereal places are computed in, taken the other way (for a
 * mode with no precession offset) */
static void sid_to_date(swe_ctx *c, int mode, double t0, double ayan_t0, double tjde, double ls,
                        double bs, double *lt, double *bt)
{
  double x[6], nutlo[2];
  swi_nutation(c, tjde, 0, nutlo);
  if (mode == 1) {
    x[0] = (ls + ayan_t0) * DEGTORAD;
    x[1] = bs * DEGTORAD;
    x[2] = 1;
    swi_polcart(x, x);
    swi_coortrf(x, x, -swi_epsiln(c, t0, 0));
    swi_precess(c, x, t0, 0, J_TO_J2000);
  } else {
    double x0[3], eps2000 = swi_epsiln(c, J2000, 0);
    x0[0] = 1;
    x0[1] = x0[2] = 0;
    if (t0 != J2000)
      swi_precess(c, x0, t0, 0, J_TO_J2000);
    swi_coortrf(x0, x0, eps2000);
    swi_cartpol(x0, x0);
    x0[0] -= SSY_PLANE_NODE_E2000;
    swi_polcart(x0, x0);
    swi_coortrf(x0, x0, SSY_PLANE_INCL);
    swi_cartpol(x0, x0);
    x[0] = (ls + ayan_t0) * DEGTORAD + x0[0];
    x[1] = bs * DEGTORAD;
    x[2] = 1;
    swi_polcart(x, x);
    swi_coortrf(x, x, -SSY_PLANE_INCL);
    swi_cartpol(x, x);
    x[0] += SSY_PLANE_NODE_E2000;
    swi_polcart(x, x);
    swi_coortrf(x, x, -eps2000);
  }
  swi_precess(c, x, tjde, 0, J2000_TO_J);
  swi_coortrf(x, x, swi_epsiln(c, tjde, 0));
  swi_cartpol(x, x);
  *lt = swe_degnorm((x[0] + nutlo[0]) * RADTODEG);
  *bt = x[1] * RADTODEG;
}

/* (a) the ARMC */
static int check_a(void)
{
  static const char HS[] = "PRCHX";
  int s, k, it, i, n0 = n, b0 = bad;
  char serr[256];
  for (s = 0; s < NPL; s++) {
    swe_ctx *c = open_ctx(PL[s].sid, PL[s].t0, PL[s].a0);
    double wa = 0, ws = 0;
    if (c == NULL)
      return -1;
    for (it = 0; it < NTU; it++) {
      double cs[13], as[10], css[13], ass[10], ct[13], at[10], cst[13], ast[10];
      if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, 'P', cs, as, css,
                           ass, serr) == ERR
          || swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LAT, LON, 'P', ct, at, cst, ast,
                              serr) == ERR) {
        printf("    ERROR %s, UT %.1f: %s\n", PL[s].name, TU[it], serr);
        swe_ctx_free(c);
        return -1;
      }
      wa = fmax(wa, fabs(swe_difdeg2n(as[2], at[2])));
      ws = fmax(ws, fabs(ass[2] - ast[2]));
    }
    verdict(wa == 0 && ws == 0, "ascmc[2] or its speed off the ARMC's", PL[s].name, 'P',
            fmax(wa, ws));
    printf("    %s: ascmc[2] %.3e deg, its speed %.3e deg/day off the ARMC's\n", PL[s].name, wa,
           ws);
    if (PL[s].sid % SE_SIDBITS == SE_SIDM_USER) {
      for (k = 0; HS[k] != '\0'; k++) {
        int hs = HS[k];
        double w = 0;
        for (it = 0; it < NTU; it++) {
          double cusp[13], ascmc[10], x[6];
          double tjde = TU[it] + swe_deltat_ex_r(c, TU[it], SEFLG_SWIEPH, serr);
          if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cusp,
                               ascmc, NULL, NULL, serr) == ERR
              || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
            printf("    ERROR %s, '%c', UT %.1f: %s\n", PL[s].name, hs, TU[it], serr);
            swe_ctx_free(c);
            return -1;
          }
          for (i = 1; i <= 12; i++) {
            double xp[2], hp;
            sid_to_date(c, PL[s].mode, PL[s].t0, PL[s].a0, tjde, cusp[i], 0, &xp[0], &xp[1]);
            hp = swe_house_pos_r(c, ascmc[2], LAT, x[0], hs, xp, serr);
            w = fmax(w, fabs(wrap(hp - i, 12)));
          }
        }
        verdict(w <= 1e-7, "a sidereal cusp's place, given ascmc[2], off its number (house)",
                PL[s].name, hs, w);
        printf("      '%c': the sidereal cusps' places given ascmc[2] %.3e house off\n", hs, w);
      }
    }
    swe_ctx_free(c);
  }
  printf("  (a) the ARMC: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* (b) whole signs and the equal systems */
static int check_b(void)
{
  static const char HS[] = "WwNnEADV";
  int s, k, it, i, n0 = n, b0 = bad;
  char serr[256];
  for (s = 0; s < NPL; s++) {
    swe_ctx *c = open_ctx(PL[s].sid, PL[s].t0, PL[s].a0);
    if (c == NULL)
      return -1;
    for (k = 0; HS[k] != '\0'; k++) {
      int hs = HS[k], uh = hs == 'w' ? 'W' : hs == 'n' ? 'N' : hs;
      double w = 0, wsp = 0;
      for (it = 0; it < NTU; it++) {
        double cu[13], am[10], cs[13], as[10], ref;
        if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cu, am, cs,
                             as, serr) == ERR) {
          printf("    ERROR %s, '%c', UT %.1f: %s\n", PL[s].name, hs, TU[it], serr);
          swe_ctx_free(c);
          return -1;
        }
        if (uh == 'W')
          ref = am[0] - fmod(am[0], 30);
        else if (uh == 'N')
          ref = 0;
        else if (uh == 'D')
          ref = swe_degnorm(am[1] + 90);	/* cusp 1, from cusp 10 */
        else if (uh == 'V')
          ref = am[0] - 15;
        else
          ref = am[0];
        for (i = 1; i <= 12; i++) {
          w = fmax(w, fabs(swe_difdeg2n(cu[i], ref + 30 * (i - 1))));
          if (uh == 'W' || uh == 'N')
            w = fmax(w, fabs(swe_difdeg2n(cu[i], 30 * floor(cu[i] / 30 + 0.5))));
          if (uh == 'W')
            wsp = fmax(wsp, fabs(cs[i]));
        }
      }
      verdict(w <= 1e-9 && wsp == 0, "the cusps off their signs or equal steps (deg)",
              PL[s].name, hs, fmax(w, wsp));
      printf("    %s, '%c': the cusps %.3e deg off%s", PL[s].name, hs, w,
             uh == 'W' ? "" : "\n");
      if (uh == 'W')
        printf(", their speeds %.3e deg/day\n", wsp);
    }
    swe_ctx_free(c);
  }
  printf("  (b) whole signs and the equal systems: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
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
 * the TT tjde */
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

/* (c) the ecliptic of t0's houses against the vector construction either
 * side of t0 */
static int check_c(void)
{
  static const double DD[] = { 1.0 / 1048576, 1.0 / 64, 1, 10, 50, 100, 125, 130, 200 };
  int s, j, sg, hk, k, n0 = n, b0 = bad;
  char serr[256];
  for (s = 0; s < 4; s++) {
    swe_ctx *c = open_ctx(PL[s].sid | SE_SIDBIT_NO_PREC_OFFSET, PL[s].t0, PL[s].a0);
    double w = 0, wd = 0, t0, a0;
    if (c == NULL)
      return -1;
    t0 = c->sidd.t0;
    a0 = c->sidd.ayan_t0;
    for (j = 0; j < (int) (sizeof DD / sizeof DD[0]); j++) {
      for (sg = -1; sg <= 1; sg += 2) {
        double tu = ut_of_tt(c, t0 + sg * DD[j]);
        double tjde = tu + swe_deltat_ex_r(c, tu, SEFLG_SWIEPH, NULL);
        double M[3][3], ww = 0;
        t0_axes(c, t0, tjde, M);
        for (hk = 0; hk < 2; hk++) {
          int hs = "RC"[hk];
          double cu[13], am[10];
          if (swe_houses_ex2_r(c, tu, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cu, am, NULL,
                               NULL, serr) == ERR) {
            printf("    ERROR %s, '%c': %s\n", PL[s].name, hs, serr);
            swe_ctx_free(c);
            return -1;
          }
          for (k = 1; k <= 12; k++) {
            double ref = swe_degnorm(t0_cusp(M, am[2], LAT, hs, k) - a0);
            ww = fmax(ww, fabs(swe_difdeg2n(cu[k], ref)));
          }
        }
        if (ww > w) {
          w = ww;
          wd = tjde - t0;
        }
      }
    }
    verdict(w <= 1e-11, "the cusps off the vector construction (deg)", PL[s].name, 'R', w);
    printf("    %s: the cusps %.3e deg off the vector construction (at t0 %+.4f day)\n",
           PL[s].name, w, wd);
    swe_ctx_free(c);
  }
  printf("  (c) the ecliptic of t0 either side of t0: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* (d) ascmc[8] on the sidereal planes */
static int check_d(void)
{
  int s, k, n0 = n, b0 = bad;
  char serr[256];
  for (s = 0; s < NPL; s++) {
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
    verdict(w == 0, "ascmc[8], its speed or ascmc[9] off (deg)", PL[s].name, 'P', w);
    printf("    %s: ascmc[8], its speed and ascmc[9] %.3e off\n", PL[s].name, w);
    swe_ctx_free(c);
  }
  printf("  (d) ascmc[8] on the sidereal planes: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: sidplanehouses EPHEDIR\n");
    return 2;
  }
  EPH = argv[1];
  printf("G43: the houses of the ecliptic of t0 and the solar system plane (the ARMC, whole\n"
         "     signs, no step at t0, ascmc[8])\n");
  r = check_a();
  if (r == 0)
    r = check_b();
  if (r == 0)
    r = check_c();
  if (r == 0)
    r = check_d();
  if (r < 0) {
    printf("G43 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G43 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG43 PASS\n", n);
  return 0;
}
