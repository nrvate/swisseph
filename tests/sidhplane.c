/* sidhplane.c -- G172: the sidereal houses of the ecliptic of t0 and of the
 * solar system plane give the ARMC, whole signs at sidereal sign starts and
 * the planets' precession offset; swe_house_pos() wraps Sripati's houses
 * and finds Polich/Page's position line to the double's resolution.
 *
 * The defects (UPSTREAM-BUGS 268 to 272), at Zurich, UT 2378496.5,
 * 2433282.5, 2451545.0 and 2488069.5:
 * 268. sidereal_houses_ecl_t0() and sidereal_houses_ssypl() returned in
 *   ascmc[2] the auxiliary ARMC their houses are computed in, not the ARMC
 *   (the sidereal time in degrees, swephprg 15.4, 17): 0.0043 deg off on
 *   the user's ecliptic of 1900 and J1900, 0.0066 Lahiri's ecliptic of
 *   t0, 0.0085 J2000, 3.90 the solar system plane, its speed 1.3e-7 to
 *   2.7e-6 deg/day off; given it, swe_house_pos() put the sidereal cusps'
 *   places off their numbers in P R C H X by 1.4e-4 to 0.34 house.
 * 269. Sidereal whole signs there were the auxiliary frame's signs moved:
 *   8.94 deg off a sign start on the ecliptic of 1900 (J1900 2.80, B1950
 *   2.10, J2000 2.80), 12.5 on the solar system plane, cusp 1 up to 20.3
 *   deg off the start of the sidereal Ascendant's sign, their speeds
 *   6.7e-5 deg/day (J2000 0.25), where they stand still; 'w' alike, and
 *   'n' not taken as 'N': 0 Aries moved by the frames' offset, 2.80 to
 *   25.3 deg.
 * 270. swe_house_pos() in Sripati ('S'): Porphyry's position plus 0.5,
 *   then "if (hpos > 12) hpos = 1": all of house 12 and the first half of
 *   house 1 answered 1.0, tropically too (cusp 12 + 1e-4 of the house
 *   1.000000 for 12.000003; Porphyry's cusp 12, Sripati's house 12's
 *   middle, 1 for 12.5). No other system wraps so.
 * 271. swe_house_pos() in Polich/Page ('T') stopped its binary search for
 *   the place's position line where its altitude over it was within 1e-6
 *   deg: 7.2e-8 house (2.1e-6 deg) off the line at Zurich, 1.8e-7 (5.4e-6
 *   deg) at 60 N.
 * 272. In a mode whose ayanamsa has its own precession model, on the
 *   ecliptic of t0 or the solar system plane the sidereal places take
 *   get_aya_correction() and the houses did not: a body and a cusp at
 *   one point of the sky 3.62e-5 deg apart (Lahiri; Fagan/Bradley
 *   1.15e-4, Krishnamurti 2.30e-4).
 *
 * The checks, at those dates, Zurich (47.37 N, 8.55 E):
 * (a) the user's ecliptic of 1900 (2415020.0, 22.46 deg), Lahiri's
 *   ecliptic of t0, J1900, J2000, the user's solar system plane (J2000,
 *   23.86) and Lahiri's: the sidereal Placidus call's ascmc[2] and its
 *   speed the tropical call's, bit for bit (one check per setting); for
 *   the two user settings, the sidereal cusps carried to the ecliptic of
 *   date through the planets' frame (G168's) and given to swe_house_pos()
 *   with the sidereal call's ascmc[2] at their numbers within 1e-7 house in
 *   P R C H X (one check per setting and system). 16 checks.
 * (b) the same settings, 'W' 'w' 'N' 'n' 'E' 'A' 'D' 'V': whole signs'
 *   cusps sign starts 30 deg apart, cusp 1 the start of ascmc[0]'s sign,
 *   0 Aries' (i - 1) 30, within 1e-9 deg, their speeds 0; equal houses'
 *   cusp 1 ascmc[0] ('D': cusp 10 ascmc[1], 'V': ascmc[0] - 15), 30 deg
 *   apart. One check per setting and system: 48.
 * (c) tropical: in each of P K O R C E A B M U T H X F Y S L Q D V W N G I
 *   i, swe_house_pos() of the places on the ecliptic 1e-4, 0.5 and 0.999
 *   of each house past its cusp in its house (k <= hpos < k + 1;
 *   Gauquelin's sectors clockwise); and in 'S' of Porphyry's cusps,
 *   Sripati's houses' middles, k + 0.5 within 1e-7. 26 checks.
 * (d) tropical, at 47.37 N, 33.87 S and 60 N: places on the Polich/Page
 *   position line of f = 0, 0.1 .. 1 (pole tan^-1(f tan(lat)), through
 *   the equator at ARMC + 90 f), declination -20 .. 20 deg by 5, in the
 *   four quadrants (mirrored west and below the horizon), above the
 *   horizon as the function judges it: swe_house_pos() 10 + 3 f (wrapped)
 *   within 1e-12 house. One check per latitude: 3.
 * (e) Fagan/Bradley, Lahiri and Krishnamurti on the ecliptic of t0 and the
 *   solar system plane, each against itself with SE_SIDBIT_NO_PREC_OFFSET
 *   (get_aya_correction() 0): the Sun, the Moon, Mars and Jupiter move by
 *   it, more than 1e-5 deg, and the Placidus cusps and ascmc (but the
 *   ARMC) by the bodies' move within 1e-11 deg. 6 checks.
 *
 * Measured: (a) 0 (bit for bit), the places 9.26e-9 house (the 0.001"
 * swe_house_pos() adds; X 3.6e-14); (b) whole signs and 0 Aries 0, the
 * equal houses 1.1e-13 deg (their cusps' sums); (c) every place in its
 * house, Porphyry's cusps 1.18e-8 house off Sripati's middles (the
 * 0.001"); (d) 1.2e-14 house; (e) the bodies 1.15e-4, 3.62e-5 and 2.30e-4
 * deg, the houses within 8.5e-14 of them. 99 checks. Before the fix 45
 * fail: (a) 16 (ascmc[2] 4.3e-3 to 3.90 deg off, the places 1.4e-4 to
 * 0.34 house), (b) 18 ('W' and 'w' 2.80 to 20.3 deg, 'n' 2.80 to 25.3),
 * (c) 2 ('S': 0.9999 house outside, a middle 0.5 off), (d) 3 (4.5e-8 to
 * 1.8e-7 house), (e) 6 (the houses not moved). The upstream build
 * (SWE_UPSTREAM_COMPAT) fails 51: (b) 24, its whole signs' and 0 Aries'
 * speeds the Ascendant's too, 622 to 650 deg/day (UPSTREAM-BUGS 40). Run
 * as
 *   sidhplane EPHEDIR
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
 * planets' frame (G168's: swi_trop_ra2sid_lon() and _sosy() the other
 * way), for a mode with no precession offset */
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

/* (a) bug 268: the ARMC */
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

/* (b) bug 269: whole signs and the equal systems on the sidereal planes */
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
          if (uh == 'W' || uh == 'N') {
            w = fmax(w, fabs(swe_difdeg2n(cu[i], 30 * floor(cu[i] / 30 + 0.5))));
            wsp = fmax(wsp, fabs(cs[i]));
          }
        }
      }
      verdict(w <= 1e-9 && wsp == 0, "the cusps off their signs or equal steps (deg)",
              PL[s].name, hs, fmax(w, wsp));
      printf("    %s, '%c': the cusps %.3e deg off, the sign cusps' speeds %.3e deg/day\n",
             PL[s].name, hs, w, wsp);
    }
    swe_ctx_free(c);
  }
  printf("  (b) whole signs and the equal systems on the planes: %d checks, %d fail\n", n - n0,
         bad - b0);
  return 0;
}

/* (c) bug 270: a place within a house is in it, tropically */
static int check_c(void)
{
  static const char HS[] = "PKORCEABMUTHXFYSLQDVWNGIi";
  static const double FR[3] = { 1e-4, 0.5, 0.999 };
  int k, it, i, j, n0 = n, b0 = bad;
  char serr[256];
  double wsr = 0;
  swe_ctx *c = open_ctx(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  if (c == NULL)
    return -1;
  for (k = 0; HS[k] != '\0'; k++) {
    int hs = HS[k], nc = hs == 'G' ? 36 : 12;
    double w = 0;
    for (it = 0; it < NTU; it++) {
      double cu[37], am[10], x[6];
      if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LAT, LON, hs, cu, am, NULL, NULL,
                           serr) == ERR
          || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
        printf("    ERROR '%c', UT %.1f: %s\n", hs, TU[it], serr);
        swe_ctx_free(c);
        return -1;
      }
      for (i = 1; i <= nc; i++) {
        double span = hs == 'G' ? swe_degnorm(cu[i] - cu[i % nc + 1])
                                : swe_degnorm(cu[i % nc + 1] - cu[i]);
        if (span == 0)
          continue;
        for (j = 0; j < 3; j++) {
          double xp[2], hp;
          xp[0] = swe_degnorm(cu[i] + (hs == 'G' ? -1 : 1) * FR[j] * span);
          xp[1] = 0;
          hp = swe_house_pos_r(c, am[2], LAT, x[0], hs, xp, serr);
          if (!(hp >= i && hp < i + 1))
            w = fmax(w, fabs(wrap(hp - (i + FR[j]), nc)));
        }
      }
      if (hs == 'S') {
        /* Porphyry's cusps, the middles of Sripati's houses */
        double cp[13], ap[10];
        if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LAT, LON, 'O', cp, ap, NULL, NULL,
                             serr) == ERR) {
          printf("    ERROR 'O', UT %.1f: %s\n", TU[it], serr);
          swe_ctx_free(c);
          return -1;
        }
        for (i = 1; i <= 12; i++) {
          double xp[2] = { cp[i], 0 };
          double hp = swe_house_pos_r(c, am[2], LAT, x[0], 'S', xp, serr);
          wsr = fmax(wsr, fabs(wrap(hp - (i + 0.5), 12)));
          if (!(hp >= 1 && hp < 13))
            wsr = fmax(wsr, 1);
        }
      }
    }
    verdict(w == 0, "a place in a house answered outside it (house)", "tropical", hs, w);
    if (w > 0)
      printf("    '%c': a place in a house answered %.6f house outside it\n", hs, w);
  }
  verdict(wsr <= 1e-7, "Porphyry's cusps off Sripati's houses' middles (house)", "tropical", 'S',
          wsr);
  printf("    'S': Porphyry's cusps %.3e house off Sripati's houses' middles\n", wsr);
  swe_ctx_free(c);
  printf("  (c) every system, a place within a house in it: %d checks, %d fail\n", n - n0,
         bad - b0);
  return 0;
}

/* (d) bug 271: places on Polich/Page position lines */
static int check_d(void)
{
  static const double LT[3] = { LAT, -33.87, 60.0 };
  int l, it, a, b, q, n0 = n, b0 = bad;
  char serr[256];
  swe_ctx *c = open_ctx(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  if (c == NULL)
    return -1;
  for (l = 0; l < 3; l++) {
    double lat = LT[l], w = 0;
    int np = 0;
    for (it = 0; it < NTU; it++) {
      double cu[13], am[10], x[6], armc, eps;
      if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, lat, LON, 'T', cu, am, NULL, NULL,
                           serr) == ERR
          || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
        printf("    ERROR 'T', UT %.1f: %s\n", TU[it], serr);
        swe_ctx_free(c);
        return -1;
      }
      armc = am[2];
      eps = x[0];
      for (a = 0; a <= 10; a++) {
        double f = a / 10.0, tp = f * tan(lat * DEGTORAD);
        for (b = -4; b <= 4; b++) {
          double de = b * 5, dra = asin(tp * tan(de * DEGTORAD)) * RADTODEG;
          double mdd = 90 * f + dra;
          /* above the horizon, as swe_house_pos() judges it */
          if (tan(de * DEGTORAD) * tan(lat * DEGTORAD) + cos(mdd * DEGTORAD) < -1e-9)
            continue;
          for (q = 0; q < 4; q++) {
            double ra = armc + mdd, dd = de, xe[3], hp, h0;
            if (q & 1)
              ra = 2 * armc - ra;	/* west */
            if (q & 2) {		/* below the horizon */
              ra += 180;
              dd = -dd;
            }
            xe[0] = swe_degnorm(ra);
            xe[1] = dd;
            xe[2] = 1;
            swe_cotrans(xe, xe, eps);
            h0 = 90 * f;
            if (q & 1)
              h0 = -h0;
            if (q & 2)
              h0 += 180;
            h0 = swe_degnorm(h0 - 90) / 30 + 1;
            hp = swe_house_pos_r(c, armc, lat, eps, 'T', xe, serr);
            w = fmax(w, fabs(wrap(hp - h0, 12)));
            np++;
          }
        }
      }
    }
    n++;
    if (!(w <= 1e-12)) {
      bad++;
      printf("    FAIL 'T' at %.2f: a place %.3e house off its position line\n", lat, w);
    }
    printf("    'T' at %.2f: %d places, %.3e house off their position lines\n", lat, np, w);
  }
  swe_ctx_free(c);
  printf("  (d) Polich/Page position lines: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

/* (e) bug 272: the houses move with the planets by get_aya_correction() */
static int check_e(void)
{
  static const struct {
    const char *name;
    int32 sid;
  } M[3] = {
    { "Fagan/Bradley", SE_SIDM_FAGAN_BRADLEY },
    { "Lahiri", SE_SIDM_LAHIRI },
    { "Krishnamurti", SE_SIDM_KRISHNAMURTI },
  };
  static const int PL4[4] = { SE_SUN, SE_MOON, SE_MARS, SE_JUPITER };
  int m, p, it, i, n0 = n, b0 = bad;
  char serr[256], name[80];
  for (m = 0; m < 3; m++) {
    for (p = 0; p < 2; p++) {
      int32 bit = p == 0 ? SE_SIDBIT_ECL_T0 : SE_SIDBIT_SSY_PLANE;
      swe_ctx *c1 = open_ctx(M[m].sid | bit, 0, 0);
      swe_ctx *c0 = open_ctx(M[m].sid | bit | SE_SIDBIT_NO_PREC_OFFSET, 0, 0);
      double mv = 0, wb = 0, wh = 0;
      if (c1 == NULL || c0 == NULL) {
        swe_ctx_free(c1);
        swe_ctx_free(c0);
        return -1;
      }
      snprintf(name, sizeof name, "%s, %s", M[m].name,
               p == 0 ? "the ecliptic of t0" : "the solar system plane");
      for (it = 0; it < NTU; it++) {
        double cu1[13], am1[10], cu0[13], am0[10], x1[6], x0[6], d = 0;
        for (i = 0; i < 4; i++) {
          if (swe_calc_ut_r(c1, TU[it], PL4[i], SEFLG_SWIEPH | SEFLG_SIDEREAL, x1, serr) < 0
              || swe_calc_ut_r(c0, TU[it], PL4[i], SEFLG_SWIEPH | SEFLG_SIDEREAL, x0, serr) < 0) {
            printf("    ERROR %s, body %d: %s\n", name, PL4[i], serr);
            swe_ctx_free(c1);
            swe_ctx_free(c0);
            return -1;
          }
          if (i == 0)
            mv = d = swe_difdeg2n(x1[0], x0[0]);
          wb = fmax(wb, fabs(swe_difdeg2n(x1[0], x0[0]) - d));
        }
        if (swe_houses_ex2_r(c1, TU[it], SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, 'P', cu1, am1,
                             NULL, NULL, serr) == ERR
            || swe_houses_ex2_r(c0, TU[it], SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, 'P', cu0,
                                am0, NULL, NULL, serr) == ERR) {
          printf("    ERROR %s, UT %.1f: %s\n", name, TU[it], serr);
          swe_ctx_free(c1);
          swe_ctx_free(c0);
          return -1;
        }
        for (i = 1; i <= 12; i++)
          wh = fmax(wh, fabs(swe_difdeg2n(cu1[i], cu0[i]) - d));
        for (i = 0; i < 8; i++) {
          if (i != 2)
            wh = fmax(wh, fabs(swe_difdeg2n(am1[i], am0[i]) - d));
        }
      }
      n++;
      if (!(fabs(mv) > 1e-5 && wb <= 1e-11 && wh <= 1e-11)) {
        bad++;
        printf("    FAIL %s: the bodies moved %.3e deg (%.1e apart), the houses %.3e off it\n",
               name, mv, wb, wh);
      }
      printf("    %s: the bodies %.3e deg, the houses %.3e off it\n", name, mv, wh);
      swe_ctx_free(c1);
      swe_ctx_free(c0);
    }
  }
  printf("  (e) the precession offset in the houses: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: sidhplane EPHEDIR\n");
    return 2;
  }
  EPH = argv[1];
  printf("G172: the houses of the ecliptic of t0 and the solar system plane (the ARMC, whole\n"
         "      signs, the precession offset); swe_house_pos()'s 'S' wrap and 'T' line\n");
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
    printf("G172 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G172 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG172 PASS\n", n);
  return 0;
}
