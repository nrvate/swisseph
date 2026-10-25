/* fictltround.c -- G80: a fictitious body's light-time position is read at
 * teval - tau, not at its rounding, and its elements are exact to theirs.
 *
 * app_pos_etc_plan_osc() solved the light time on the orbit (bug 26, G27)
 * and read the elements at t = teval - dt rounded to a double, up to half
 * an ulp of the date (2.3e-10 day near the present) from the emission
 * instant, as the planets were before bug 63. An Earth-centred set's focus
 * is the Earth at t, which moves 0.0172 AU/day while the observer stays at
 * teval: one ulp is 8e-12 AU, 7e-8 deg on Waldemath at 0.0065 AU, a
 * sawtooth in its apparent place with the period of one ulp of light time
 * (UPSTREAM-BUGS 81). A five-point difference over 2^-7 day made it 1e-6
 * to 5.5e-6 deg/day on Waldemath, 2.7e-7 on Selena; Vulcan, at 19 deg/day,
 * 3.4e-7 heliocentric. The body is now moved along its barycentric
 * velocity at teval by the rounding, (teval - t) - dt.
 *
 * Part 2: an element's linear term n T was rounded as the unreduced angle
 * before the sum was taken to 0..360 -- Vulcan's mean anomaly 707550.7341 T
 * is 1.6e7 deg at -200 and 3.5e7 at -3000 -- so even the true places
 * jittered: Vulcan heliocentric by 3.0e-8 deg at -3000 between instants
 * 2^-9 day apart, Waldemath by 7.7e-9. check_t_terms() now forms the term
 * from the rate in two doubles and an fma product with its revolutions
 * taken out exactly.
 *
 * The check, Swiss files and Moshier, the shipped seorbel.txt, binary-
 * fraction instants. (1) Vulcan heliocentric and geocentric, Selena and
 * Waldemath geocentric, apparent (default flags), thirteen dates 1700..2300.
 * (2) The same four with SEFLG_TRUEPOS, three dates each at -3000 and -200.
 * For each row the longitude and latitude speeds against five-point
 * differences of the positions over 2^-7 and 2^-9 day, and the jitter:
 * fourth differences of the positions at eight instants 2^-9 day apart (a
 * smooth motion gives 1e-12). Limits below (LV_ speed deg/day, LJ_ jitter
 * deg); Waldemath's carry Moshier's Earth, whose own places are 5e-14 AU
 * off a smooth curve (3.4e-10 deg at 0.0065 AU, 1.8e-8 deg/day). Measured
 * and the builds that fail in UPSTREAM-BUGS 81: before part 1 and on the
 * upstream build every one of the 24 rows; with part 1 alone the four
 * Vulcan rows of (1) and all sixteen of (2). Run as
 *   fictltround EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#ifndef LV_VH	/* limits, overridable for measuring */
# define LV_VH 1e-9
# define LJ_VH 2e-11
# define LV_VG 5e-9
# define LJ_VG 5e-11
# define LV_S 5e-9
# define LJ_S 2e-9
# define LV_W 5e-8
# define LJ_W 1e-9
# define LV_T 1e-9
# define LJ_T 2e-11
#endif

static int bad = 0;

/* One row: the speeds against five-point differences over 2^-7 and 2^-9
 * day, and the fourth differences of the positions at eight instants 2^-9
 * day apart, at nd dates from jd0 + 37.3 k days, k < nd, every dy years. */
static void row(const char *seph, const char *sz, int ipl, int32 fl,
                double y0, double dy, int nd, double limv, double limj)
{
  char serr[256];
  double wv[2] = {0, 0}, wj[2] = {0, 0};
  int y, k, i, s;
  for (y = 0; y < nd; y++) {
    double jd = floor((2451545.0 + (y0 + y * dy - 2000) * 365.25 + 37.3 * y) * 256) / 256;
    double x[6], d[8][6];
    if (swe_calc(jd, ipl, fl | SEFLG_SPEED, x, serr) < 0) {
      printf("  %-6s %-22s ERROR %s\n", seph, sz, serr);
      bad++;
      return;
    }
    for (s = 0; s < 2; s++) {
      double h = s ? 1.0 / 512 : 1.0 / 128;
      for (k = 0; k < 5; k++)
        swe_calc(jd + (k - 2) * h, ipl, fl, d[k], serr);
      for (i = 0; i < 2; i++) {
        double v[5], e;
        for (k = 0; k < 5; k++)
          v[k] = i ? d[k][1] : d[2][0] + swe_difdeg2n(d[k][0], d[2][0]);
        e = fabs(x[3 + i] - (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h));
        if (e > wv[i])
          wv[i] = e;
      }
    }
    for (k = 0; k < 8; k++)
      swe_calc(jd + k / 512.0, ipl, fl, d[k], serr);
    for (i = 0; i < 2; i++) {
      double v[8];
      for (k = 0; k < 8; k++)
        v[k] = i ? d[k][1] : d[0][0] + swe_difdeg2n(d[k][0], d[0][0]);
      for (k = 0; k + 4 < 8; k++) {
        double e = fabs(v[k] - 4 * v[k + 1] + 6 * v[k + 2] - 4 * v[k + 3] + v[k + 4]);
        if (e > wj[i])
          wj[i] = e;
      }
    }
  }
  printf("  %-6s %-22s speed lon %.2e lat %.2e deg/day, jitter lon %.2e lat %.2e deg\n",
         seph, sz, wv[0], wv[1], wj[0], wj[1]);
  if (wv[0] > limv || wv[1] > limv) {
    printf("    FAIL: speed off its positions' rate by more than %.0e deg/day\n", limv);
    bad++;
  }
  if (wj[0] > limj || wj[1] > limj) {
    printf("    FAIL: position jitters by more than %.0e deg between neighbouring instants\n", limj);
    bad++;
  }
}

int main(int argc, char **argv)
{
  /* (1) apparent, thirteen dates 1700..2300; limits speed deg/day, jitter
   * deg */
  static const struct { int ipl; int32 fl; const char *sz; double limv, limj; } rgb[] = {
    {SE_FICT_OFFSET + 15, SEFLG_HELCTR, "Vulcan, helio", LV_VH, LJ_VH},
    {SE_FICT_OFFSET + 15, 0, "Vulcan, geo", LV_VG, LJ_VG},
    {SE_FICT_OFFSET + 16, 0, "Selena, geo", LV_S, LJ_S},
    {SE_FICT_OFFSET + 18, 0, "Waldemath, geo", LV_W, LJ_W},
  };
  /* (2) true positions, three dates each at -3000 and -200 */
  static const struct { int ipl; int32 fl; const char *sz; } rgt[] = {
    {SE_FICT_OFFSET + 15, SEFLG_HELCTR, "Vulcan, helio, true"},
    {SE_FICT_OFFSET + 15, 0, "Vulcan, geo, true"},
    {SE_FICT_OFFSET + 16, 0, "Selena, geo, true"},
    {SE_FICT_OFFSET + 18, 0, "Waldemath, geo, true"},
  };
  static const struct { int32 fl; const char *sz; } eph[2] = {
    {SEFLG_SWIEPH, "Swiss"}, {SEFLG_MOSEPH, "Mosh"},
  };
  int ie, ib, iy;

  if (argc != 2) {
    fprintf(stderr, "usage: fictltround EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G80: a fictitious body's position is exact to its rounding at t - tau\n");
  for (ie = 0; ie < 2; ie++)
    for (ib = 0; ib < 4; ib++)
      row(eph[ie].sz, rgb[ib].sz, rgb[ib].ipl, eph[ie].fl | rgb[ib].fl,
          1700, 50, 13, rgb[ib].limv, rgb[ib].limj);
  for (ie = 0; ie < 2; ie++)
    for (ib = 0; ib < 4; ib++)
      for (iy = 0; iy < 2; iy++) {
        char sz[48];
        snprintf(sz, sizeof sz, "%s %d", rgt[ib].sz, iy ? -200 : -3000);
        row(eph[ie].sz, sz, rgt[ib].ipl, eph[ie].fl | rgt[ib].fl | SEFLG_TRUEPOS,
            iy ? -200 : -3000, 0, 3, LV_T, LJ_T);
      }
  printf(bad ? "G80 FAIL: %d\n" : "G80 PASS\n", bad);
  return bad ? 1 : 0;
}
