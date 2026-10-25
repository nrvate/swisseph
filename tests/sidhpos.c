/* sidhpos.c -- G168: swe_house_pos() in sidereal charts, as documented:
 * given a body's tropical place it answers the sidereal house position
 * in every system but whole signs and 0 Aries with the traditional
 * method, and in P R C H X G T I with the ecliptic of t0 and the solar
 * system plane; in 'W' and 'N' it answers the tropical house, and the
 * sidereal one is swe_degnorm(sid - cusp[1]) / 30 + 1 of the sidereal
 * cusps.
 *
 * The limitation (UPSTREAM-BUGS 254). swephprg 16: swe_house_pos()
 * "requires TROPICAL positions in xpin"; with the traditional method
 * (sid = trop - ayanamsa) "the function swe_house_pos() works for all
 * house systems". Whole signs and equal houses from 0 Aries are signs,
 * and a sidereal sign starts the ayanamsa from a tropical one; but
 * swe_house_pos() takes no date, flag or ayanamsa (the ARMC, obliquity
 * and place are the caller's; the _r form's context holds a sidereal
 * mode, but not the date its ayanamsa needs, nor whether a call is
 * sidereal). So in 'W' and 'N' it answers the tropical house: Lahiri at
 * Zurich, 1800-2100, the tropical place of a sidereal cusp 0.298 house off
 * the cusp's number in 'W', up to 0.842 (the ayanamsa / 30 deg) in 'N';
 * Mars at UT 2433282.5 1.0737 for 1.3018 and 7.0737 for 6.3018. No input
 * convention gives the sidereal house: the sidereal longitude, against
 * the manual, gives 'N''s (6.3018) but puts 'W' a house off (12.3018).
 * Documented in swe_house_pos()'s comments, not changed.
 *
 * Check (a), Lahiri, True Citra, Fagan/Bradley and Lahiri on the ecliptic
 * of date, at UT 2378496.5, 2433282.5, 2451545.0 and 2488069.5, Zurich:
 * swe_house_pos() of the tropical place (the cusp plus the ayanamsa,
 * latitude 0) of each sidereal cusp of swe_houses_ex2(), with that call's
 * ARMC and SE_ECL_NUT's true obliquity, is the cusp's number within 1e-7
 * house, in each system but 'W' and 'N': P K O R C E A B M U T H X F Y S L
 * Q D V G I i. One check per setting and system: 92.
 * Check (b), the same settings and dates, 'W' and 'N': the sidereal cusps
 * are sign starts, cusp 1 the start of the sidereal Ascendant's sign
 * ('W') or 0 ('N') and cusp k cusp 1 + 30 (k - 1), within 1e-9 deg; for
 * the Sun, the Moon, Mars, Jupiter, the mean node and the tropical places
 * of the 12 sidereal cusps, swe_house_pos() of the tropical place is the
 * tropical house, swe_degnorm(trop - tropical cusp[1]) / 30 + 1, within
 * 1e-7 house (the 0.001" 'W' adds is 9.3e-9), and so off the sidereal
 * house swe_degnorm(sid - sidereal cusp[1]) / 30 + 1 by the ayanamsa / 30
 * deg less whole houses ('N': none) within 1e-7. Three checks per setting
 * and system: 24.
 * Check (c), the user's ecliptic of 1900 (t0 2415020.0, 22.46 deg) and
 * solar system plane (J2000, 23.86 deg), the same dates: the sidereal
 * cusps carried to the true ecliptic of date through the planets' own
 * frame (the library's precession and nutation; sidereal Mars and the
 * Moon so carried are swe_calc()'s tropical ones within 1e-11 deg, one
 * check per setting) and given to swe_house_pos() with the ARMC of the
 * date (the tropical call's ascmc[2]) and the true obliquity are the
 * cusp's number within 1e-7 house in P R C H X G T I, the systems whose
 * house circles do not depend on the ecliptic. 18 checks.
 *
 * Measured: (a) 0 in every system but 'G', 2.78e-8 (the 0.001" its
 * sectors' formula adds; cusps 13 to 36 miss the exact-cusp test);
 * (b) cusps exact, the tropical house within 9.3e-9, the ayanamsa / 30
 * within 9.3e-9; (c) the carried places within 1.2e-13 deg, P R C H I
 * 9.26e-9 (the 0.001"), X 3.6e-14, G 2.78e-8, T 3.9e-14 (5.96e-8, its
 * position line's search stopping at 1e-6 deg, before UPSTREAM-BUGS 271).
 * 134 checks. The upstream build (SWE_UPSTREAM_COMPAT) passes too: nothing
 * changed for entry 254. Run as
 *   sidhpos EPHEDIR
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
#define TOL 1e-7
static const double TU[] = { 2378496.5, 2433282.5, 2451545.0, 2488069.5 };
#define NTU ((int) (sizeof TU / sizeof TU[0]))

static const struct {
  const char *name;
  int32 sid;
} TRAD[] = {
  { "Lahiri", SE_SIDM_LAHIRI },
  { "True Citra", SE_SIDM_TRUE_CITRA },
  { "Fagan/Bradley", SE_SIDM_FAGAN_BRADLEY },
  { "Lahiri, ecliptic of date", SE_SIDM_LAHIRI | SE_SIDBIT_ECL_DATE },
};
#define NTRAD ((int) (sizeof TRAD / sizeof TRAD[0]))

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

static void verdict(double w, const char *what, const char *setting, int hs)
{
  n++;
  if (!(w <= TOL)) {
    bad++;
    printf("    FAIL %s, %s, '%c': %.3e house off\n", what, setting, hs, w);
  }
}

/* (a) the traditional method: the tropical place of each sidereal cusp is
 * at the cusp's number, every system but 'W' and 'N' */
static int check_a(void)
{
  static const char HS[] = "PKORCEABMUTHXFYSLQDVGIi";
  int m, k, it, i, n0 = n, b0 = bad;
  char serr[256];
  for (m = 0; m < NTRAD; m++) {
    swe_ctx *c = open_ctx(TRAD[m].sid, 0, 0);
    double wmax = 0;
    int hmax = 'P';
    if (c == NULL)
      return -1;
    for (k = 0; HS[k] != '\0'; k++) {
      int hs = HS[k], nc = hs == 'G' ? 36 : 12;
      double w = 0;
      for (it = 0; it < NTU; it++) {
        double cusp[37], ascmc[10], x[6], aya;
        if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cusp, ascmc,
                             NULL, NULL, serr) == ERR
            || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR
            || swe_get_ayanamsa_ex_ut_r(c, TU[it], SEFLG_SWIEPH, &aya, serr) == ERR) {
          printf("    ERROR %s, '%c', UT %.1f: %s\n", TRAD[m].name, hs, TU[it], serr);
          swe_ctx_free(c);
          return -1;
        }
        for (i = 1; i <= nc; i++) {
          double xp[2] = { swe_degnorm(cusp[i] + aya), 0 };
          double hp = swe_house_pos_r(c, ascmc[2], LAT, x[0], hs, xp, serr);
          w = fmax(w, fabs(wrap(hp - i, nc)));
        }
      }
      verdict(w, "the tropical place of a sidereal cusp", TRAD[m].name, hs);
      if (w > wmax) {
        wmax = w;
        hmax = hs;
      }
    }
    printf("    %s: 23 systems, worst %.3e house ('%c')\n", TRAD[m].name, wmax, hmax);
    swe_ctx_free(c);
  }
  printf("  (a) the traditional method, every system but W and N: %d checks, %d fail\n", n - n0,
         bad - b0);
  return 0;
}

/* (b) 'W' and 'N': the sidereal cusps are sign starts; swe_house_pos() of
 * the tropical place is the tropical house, the ayanamsa / 30 off the
 * sidereal one */
static int check_b(void)
{
  static const int PL[5] = { SE_SUN, SE_MOON, SE_MARS, SE_JUPITER, SE_MEAN_NODE };
  int m, k, it, i, n0 = n, b0 = bad;
  char serr[256];
  for (m = 0; m < NTRAD; m++) {
    swe_ctx *c = open_ctx(TRAD[m].sid, 0, 0);
    if (c == NULL)
      return -1;
    for (k = 0; k < 2; k++) {
      int hs = "WN"[k];
      double wc = 0, wt = 0, wa = 0, cmax = 0;
      for (it = 0; it < NTU; it++) {
        double ct[13], at[10], cs[13], as[10], x[6], aya, pt[17][2], ps[17];
        if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LAT, LON, hs, ct, at, NULL, NULL, serr) == ERR
            || swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cs, as,
                                NULL, NULL, serr) == ERR
            || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR
            || swe_get_ayanamsa_ex_ut_r(c, TU[it], SEFLG_SWIEPH, &aya, serr) == ERR) {
          printf("    ERROR %s, '%c', UT %.1f: %s\n", TRAD[m].name, hs, TU[it], serr);
          swe_ctx_free(c);
          return -1;
        }
        /* the sidereal cusps: sign starts */
        wc = fmax(wc, fabs(swe_difdeg2n(cs[1], hs == 'N' ? 0 : as[0] - fmod(as[0], 30))));
        for (i = 2; i <= 12; i++)
          wc = fmax(wc, fabs(swe_difdeg2n(cs[i], cs[1] + 30 * (i - 1))));
        /* the places: five bodies, the 12 sidereal cusps */
        for (i = 0; i < 5; i++) {
          double xt[6], xs[6];
          if (swe_calc_ut_r(c, TU[it], PL[i], SEFLG_SWIEPH, xt, serr) < 0
              || swe_calc_ut_r(c, TU[it], PL[i], SEFLG_SWIEPH | SEFLG_SIDEREAL, xs, serr) < 0) {
            printf("    ERROR %s, body %d, UT %.1f: %s\n", TRAD[m].name, PL[i], TU[it], serr);
            swe_ctx_free(c);
            return -1;
          }
          pt[i][0] = xt[0];
          pt[i][1] = xt[1];
          ps[i] = xs[0];
        }
        for (i = 1; i <= 12; i++) {
          pt[4 + i][0] = swe_degnorm(cs[i] + aya);
          pt[4 + i][1] = 0;
          ps[4 + i] = cs[i];
        }
        for (i = 0; i < 17; i++) {
          double hp = swe_house_pos_r(c, as[2], LAT, x[0], hs, pt[i], serr);
          double htrop = swe_degnorm(pt[i][0] - ct[1]) / 30 + 1;
          double hsid = swe_degnorm(ps[i] - cs[1]) / 30 + 1;
          double d = wrap(hp - hsid, 12) - aya / 30;
          wt = fmax(wt, fabs(wrap(hp - htrop, 12)));
          wa = fmax(wa, fabs(hs == 'N' ? wrap(d, 12) : d - floor(d + 0.5)));
          if (i >= 5)
            cmax = fmax(cmax, fabs(wrap(hp - (i - 4), 12)));
        }
      }
      n++;
      if (!(wc <= 1e-9)) {
        bad++;
        printf("    FAIL %s, '%c': a sidereal cusp %.3e deg off its sign start\n", TRAD[m].name,
               hs, wc);
      }
      verdict(wt, "swe_house_pos() off the tropical house", TRAD[m].name, hs);
      verdict(wa, "swe_house_pos() less the sidereal house off the ayanamsa / 30", TRAD[m].name,
              hs);
      printf("    %s, '%c': the tropical place of a sidereal cusp %.3f house off its number"
             " (the limitation)\n", TRAD[m].name, hs, cmax);
    }
    swe_ctx_free(c);
  }
  printf("  (b) whole signs and 0 Aries, the tropical houses: %d checks, %d fail\n", n - n0,
         bad - b0);
  return 0;
}

/* a point of the sidereal plane (the ecliptic of t0, mode 1, or the solar
 * system plane, mode 2) to the true ecliptic of date at the TT tjde, as
 * swi_trop_ra2sid_lon() and swi_trop_ra2sid_lon_sosy() take a place the
 * other way */
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
    /* the zero point of t0 on the plane, as swi_trop_ra2sid_lon_sosy() */
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

/* (c) the ecliptic of t0 and the solar system plane: the systems that do
 * not depend on the ecliptic */
static int check_c(void)
{
  static const struct {
    const char *name;
    int mode;
    int32 sid;
    double t0, a0;
  } S[2] = {
    { "the ecliptic of 1900", 1, SE_SIDM_USER | SE_SIDBIT_ECL_T0, 2415020.0, 22.46 },
    { "the solar system plane", 2, SE_SIDM_USER | SE_SIDBIT_SSY_PLANE, J2000, 23.86 },
  };
  static const char HS[] = "PRCHXGTI";
  int s, k, it, i, n0 = n, b0 = bad;
  char serr[256];
  for (s = 0; s < 2; s++) {
    swe_ctx *c = open_ctx(S[s].sid, S[s].t0, S[s].a0);
    double w = 0, wmax = 0;
    int hmax = 'P';
    if (c == NULL)
      return -1;
    /* the frame: sidereal Mars and the Moon carried back */
    for (it = 0; it < NTU; it++) {
      double tjde = TU[it] + swe_deltat_ex_r(c, TU[it], SEFLG_SWIEPH, serr);
      for (i = 0; i < 2; i++) {
        double xt[6], xs[6], lt, bt;
        int ipl = i == 0 ? SE_MARS : SE_MOON;
        if (swe_calc_ut_r(c, TU[it], ipl, SEFLG_SWIEPH, xt, serr) < 0
            || swe_calc_ut_r(c, TU[it], ipl, SEFLG_SWIEPH | SEFLG_SIDEREAL, xs, serr) < 0) {
          printf("    ERROR %s, body %d: %s\n", S[s].name, ipl, serr);
          swe_ctx_free(c);
          return -1;
        }
        sid_to_date(c, S[s].mode, S[s].t0, S[s].a0, tjde, xs[0], xs[1], &lt, &bt);
        w = fmax(w, fmax(fabs(swe_difdeg2n(lt, xt[0])), fabs(bt - xt[1])));
      }
    }
    n++;
    if (!(w <= 1e-11)) {
      bad++;
      printf("    FAIL %s: sidereal Mars or the Moon carried back %.3e deg off the tropical\n",
             S[s].name, w);
    }
    for (k = 0; HS[k] != '\0'; k++) {
      int hs = HS[k], nc = hs == 'G' ? 36 : 12;
      w = 0;
      for (it = 0; it < NTU; it++) {
        double cusp[37], ascmc[10], ct[37], at[10], x[6];
        double tjde = TU[it] + swe_deltat_ex_r(c, TU[it], SEFLG_SWIEPH, serr);
        /* the ARMC of the date: the tropical call's (upstream's sidereal
         * call's ascmc[2] is its auxiliary ARMC here, UPSTREAM-BUGS 268) */
        if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cusp, ascmc,
                             NULL, NULL, serr) == ERR
            || swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LAT, LON, hs, ct, at, NULL, NULL,
                                serr) == ERR
            || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
          printf("    ERROR %s, '%c', UT %.1f: %s\n", S[s].name, hs, TU[it], serr);
          swe_ctx_free(c);
          return -1;
        }
        for (i = 1; i <= nc; i++) {
          double xp[2], hp;
          sid_to_date(c, S[s].mode, S[s].t0, S[s].a0, tjde, cusp[i], 0, &xp[0], &xp[1]);
          hp = swe_house_pos_r(c, at[2], LAT, x[0], hs, xp, serr);
          w = fmax(w, fabs(wrap(hp - i, nc)));
        }
      }
      verdict(w, "a sidereal cusp's place of date", S[s].name, hs);
      if (w > wmax) {
        wmax = w;
        hmax = hs;
      }
    }
    printf("    %s: 8 systems, worst %.3e house ('%c')\n", S[s].name, wmax, hmax);
    swe_ctx_free(c);
  }
  printf("  (c) the ecliptic of t0 and the solar system plane, P R C H X G T I: %d checks,"
         " %d fail\n", n - n0, bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: sidhpos EPHEDIR\n");
    return 2;
  }
  EPH = argv[1];
  printf("G168: swe_house_pos() of a tropical place in sidereal charts, as documented: the\n"
         "      sidereal house but in whole signs and 0 Aries, which answer the tropical one\n");
  r = check_a();
  if (r == 0)
    r = check_b();
  if (r == 0)
    r = check_c();
  if (r < 0) {
    printf("G168 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G168 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG168 PASS\n", n);
  return 0;
}
