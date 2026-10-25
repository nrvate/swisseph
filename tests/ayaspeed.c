/* ayaspeed.c -- G90: the ayanamsa's speed in a sidereal speed is the rate
 * of the ayanamsa, in every mode.
 *
 * The defect (UPSTREAM-BUGS 91): swi_get_ayanamsa_with_speed() took the
 * rate as (a(t) - a(t - 0.001)) / 0.001. In the modes made from the
 * precession a(t) carries its own rounding, 1e-14 to 7e-14 deg (the
 * precession's matrices and the polar angle), which a thousandth of a day
 * turns into 1.3e-11 to 9e-11 deg/day (Lahiri 6.3e-11; G85's 5.7e-11 on
 * every sidereal row). In the star modes a(t) is an aberrated, deflected
 * star's longitude, whose backward difference is off by half its curvature
 * times the step: 9e-10 to 1.1e-9 deg/day (the Galactic Centre, Citra,
 * Pushya, Mula), 2.1e-6 for True Revati with zeta Psc 0.3 deg from the Sun
 * in 1981. Where the ayanamsa crosses 0 (Lahiri, 285 AD) the difference
 * took the 360 deg jump: 3.6e5 deg/day for 0.001 day.
 *
 * The check: a = the Sun's tropical longitude without nutation less its
 * sidereal one, its speed the difference of the two speeds, against
 * five-point differences of a at binary-fraction instants. The 47
 * predefined modes but J2000, J1900, B1950 and Mardyks (forced onto the
 * ecliptic of t0, they take no ayanamsa speed), SE_SIDM_USER (t0 in TT and
 * in UT), Lahiri and USER with SE_SIDBIT_ECL_DATE; SEFLG_SIDEREAL with and
 * without SEFLG_NONUT (the same: sidereal implies it); Swiss files and
 * Moshier; nine dates -2000..2900, 1800..2200 for the star modes. The
 * precession modes over 1 day within 2e-12 deg/day (measured <= 1.1e-13),
 * the star modes over 2^-6 day within 2e-11 (<= 6.3e-12); True Revati at
 * zeta Psc's conjunction, 2444704.30859375, over 2^-8 day within 2e-10
 * (3.7e-11; over 2^-6 the reference's own truncation is 9e-9 there);
 * Lahiri 2^-12 day after it crosses 0 within 2e-12 (2.1e-14): 1694 checks.
 * The build before the fix fails 1506: every precession mode (2.9e-11 to
 * 7.9e-11), every star mode but the two from the modern galactic pole
 * (1.6e-11; 6.8e-10 to 9.8e-10 on the others, 4e-11 IAU 1958), the
 * conjunction (2.1e-6), the crossing (3.6e5). The upstream build fails
 * 1546 (its Moshier stars 5.1e-7, their own). Run as
 *   ayaspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static int bad = 0, nchk = 0;

/* tropical (no nutation) less sidereal longitude of the Sun, wrapped near
 * a reference value; and the rate of that from the two speeds */
static int aya_of_sun(swe_ctx *c, double t, int32 ephe, int32 sfl, double *a, double *ad)
{
  double xt[6], xs[6];
  char serr[256];
  if (swe_calc_r(c, t, SE_SUN, ephe | SEFLG_NONUT | SEFLG_SPEED, xt, serr) < 0
      || swe_calc_r(c, t, SE_SUN, ephe | sfl | SEFLG_SPEED, xs, serr) < 0)
    return -1;
  *a = swe_degnorm(xt[0] - xs[0]);
  if (ad != NULL)
    *ad = xt[3] - xs[3];
  return 0;
}

/* the ayanamsa's speed less its five-point rate over h, deg/day */
static double resid(swe_ctx *c, double t, int32 ephe, int32 sfl, double h)
{
  double a0, ad, p[5];
  int i;
  if (aya_of_sun(c, t, ephe, sfl, &a0, &ad) < 0)
    return HUGE_VAL;
  for (i = 0; i < 5; i++) {
    if (aya_of_sun(c, t + (i - 2) * h, ephe, sfl, &p[i], NULL) < 0)
      return HUGE_VAL;
    p[i] = a0 + swe_difdeg2n(p[i], a0);
  }
  return fabs(ad - (8 * (p[3] - p[1]) - (p[4] - p[0])) / (12 * h));
}

static int from_star(int m)
{
  switch (m) {
  case SE_SIDM_TRUE_CITRA: case SE_SIDM_TRUE_REVATI: case SE_SIDM_TRUE_PUSHYA:
  case SE_SIDM_TRUE_SHEORAN: case SE_SIDM_TRUE_MULA: case SE_SIDM_GALCENT_0SAG:
  case SE_SIDM_GALCENT_COCHRANE: case SE_SIDM_GALCENT_RGILBRAND:
  case SE_SIDM_GALCENT_MULA_WILHELM: case SE_SIDM_GALEQU_IAU1958:
  case SE_SIDM_GALEQU_TRUE: case SE_SIDM_GALEQU_MULA:
    return 1;
  }
  return 0;
}

static void judge(const char *what, double r, double lim)
{
  nchk++;
  /* !(<=) so that a NaN fails */
  if (!(r <= lim)) {
    if (bad < 12)
      printf("    FAIL: %s %.2e deg/day (limit %.0e)\n", what, r, lim);
    bad++;
  }
}

int main(int argc, char **argv)
{
  static const double yp[9] = {-2000, -500, 1000, 1800, 1900, 2000.3, 2050, 2200, 2900};
  static const double ys[9] = {1800, 1850, 1900, 1950, 1981.5, 2000.3, 2050, 2100, 2200};
  static const int32 eph[2] = {SEFLG_SWIEPH, SEFLG_MOSEPH};
  static const char *ename[2] = {"Swiss", "Moshier"};
  static const int32 sfl[2] = {SEFLG_SIDEREAL, SEFLG_SIDEREAL | SEFLG_NONUT};
  int modes[52], nm = 0, k, e, n, y;
  char serr[256], what[120];
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: ayaspeed EPHEDIR\n");
    return 2;
  }
  printf("G90: the ayanamsa's speed is its rate, every mode\n");
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  /* not J2000, J1900, B1950 and Mardyks: projected onto the ecliptic of
   * t0 (SE_SIDBIT_ECL_T0 forced), they take no ayanamsa speed */
  for (k = 0; k < SE_NSIDM_PREDEF; k++)
    if (k != SE_SIDM_J2000 && k != SE_SIDM_J1900 && k != SE_SIDM_B1950
        && k != SE_SIDM_GALALIGN_MARDYKS)
      modes[nm++] = k;
  modes[nm++] = SE_SIDM_USER;
  modes[nm++] = SE_SIDM_USER | SE_SIDBIT_USER_UT;
  modes[nm++] = SE_SIDM_USER | SE_SIDBIT_ECL_DATE;
  modes[nm++] = SE_SIDM_LAHIRI | SE_SIDBIT_ECL_DATE;

  for (e = 0; e < 2; e++) {
    double wp = 0, ws = 0;
    for (k = 0; k < nm; k++) {
      int star = from_star(modes[k] % SE_SIDBITS);
      double h = star ? 1.0 / 64 : 1.0, lim = star ? 2e-11 : 2e-12, w = 0;
      swe_set_sid_mode_r(c, modes[k], 2451545.0 - 3000.5, 23.4567);
      for (n = 0; n < 2; n++)
        for (y = 0; y < 9; y++) {
          double t = floor((2451545.0 + ((star ? ys[y] : yp[y]) - 2000) * 365.25 + 0.3) * 256) / 256;
          double r = resid(c, t, eph[e], sfl[n], h);
          snprintf(what, sizeof what, "%s mode %d%s at %.4f", ename[e], modes[k],
                   n ? " no nutation" : "", t);
          judge(what, r, lim);
          if (!(r <= w)) w = r;
        }
      if (star) { if (!(w <= ws)) ws = w; }
      else if (!(w <= wp)) wp = w;
    }
    printf("  %-8s precession modes  worst %.2e deg/day (limit 2e-12)\n", ename[e], wp);
    printf("  %-8s star modes        worst %.2e deg/day (limit 2e-11)\n", ename[e], ws);
  }

  /* True Revati at zeta Psc's conjunction with the Sun */
  {
    double r;
    swe_set_sid_mode_r(c, SE_SIDM_TRUE_REVATI, 0, 0);
    r = resid(c, 2444704.30859375, SEFLG_SWIEPH, SEFLG_SIDEREAL, 1.0 / 256);
    printf("  True Revati, zeta Psc at the Sun       %.2e deg/day (limit 2e-10)\n", r);
    judge("True Revati at the conjunction", r, 2e-10);
  }

  /* Lahiri just after the ayanamsa crosses 0 */
  {
    double lo = 1721424.5 + 200 * 365.25, hi = 1721424.5 + 400 * 365.25, a, r;
    int i;
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    for (i = 0; i < 80; i++) {
      double mid = (lo + hi) / 2;
      swe_get_ayanamsa_ex_r(c, mid, SEFLG_SWIEPH | SEFLG_NONUT, &a, serr);
      if (a > 180) lo = mid; else hi = mid;
    }
    hi = ceil(hi * 4096) / 4096;
    r = resid(c, hi, SEFLG_SWIEPH, SEFLG_SIDEREAL, 1.0);
    printf("  Lahiri, 0 deg crossed at %.4f       %.2e deg/day (limit 2e-12)\n", hi, r);
    judge("Lahiri across 0 deg", r, 2e-12);
  }

  swe_ctx_free(c);
  printf("  %d checks\n", nchk);
  printf(bad ? "G90 FAIL: %d\n" : "G90 PASS\n", bad);
  return bad ? 1 : 0;
}
