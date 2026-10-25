/* fictltround.c -- G80: a fictitious body's light-time position is read at
 * teval - tau, not at its rounding.
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
 * The check, Swiss files and Moshier, the shipped seorbel.txt: Vulcan
 * heliocentric and geocentric, Selena and Waldemath geocentric, apparent
 * (default flags), thirteen dates -300..+300 years from J2000 at
 * binary-fraction instants. (1) The longitude and latitude speeds within
 * 1e-7 deg/day of five-point differences of the positions over 2^-7 and
 * 2^-9 day. (2) The jitter: fourth differences of the positions at eight
 * neighbouring instants 2^-9 day apart within 1e-10 deg (a smooth motion
 * gives 1e-12). Measured and limits in the table of UPSTREAM-BUGS 81. The
 * build before the fix fails every Waldemath and Selena row and Vulcan
 * heliocentric; the upstream build fails them all. Run as
 *   fictltround EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(int argc, char **argv)
{
  /* limits: speed deg/day, jitter deg; above the geometric positions'
   * own floor (the elements' rounding, SEFLG_TRUEPOS: Vulcan heliocentric
   * 5.2e-8 and 3.1e-9, Waldemath 1.1e-8 and 4.0e-10) */
  static const struct { int ipl; int32 fl; const char *sz; double limv, limj; } rgb[] = {
    {SE_FICT_OFFSET + 15, SEFLG_HELCTR, "Vulcan, helio", 5e-7, 1e-8},
    {SE_FICT_OFFSET + 15, 0, "Vulcan, geo", 1e-7, 2e-9},
    {SE_FICT_OFFSET + 16, 0, "Selena, geo", 5e-9, 2e-9},
    {SE_FICT_OFFSET + 18, 0, "Waldemath, geo", 1e-7, 2e-9},
  };
  static const struct { int32 fl; const char *sz; } eph[2] = {
    {SEFLG_SWIEPH, "Swiss"}, {SEFLG_MOSEPH, "Mosh"},
  };
  char serr[256];
  int ie, ib, y, k, i, s, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: fictltround EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G80: a fictitious body's light-time position is read at teval - tau\n");
  for (ie = 0; ie < 2; ie++) {
    for (ib = 0; ib < 4; ib++) {
      int32 fl = eph[ie].fl | rgb[ib].fl;
      double wv[2] = {0, 0}, wj[2] = {0, 0};
      for (y = 0; y < 13; y++) {
        double jd = floor((2451545.0 + (-300 + y * 50) * 365.25 + 37.3 * y) * 256) / 256;
        double x[6], d[8][6];
        if (swe_calc(jd, rgb[ib].ipl, fl | SEFLG_SPEED, x, serr) < 0) {
          printf("  %-6s %-16s ERROR %s\n", eph[ie].sz, rgb[ib].sz, serr);
          bad++;
          break;
        }
        for (s = 0; s < 2; s++) {
          double h = s ? 1.0 / 512 : 1.0 / 128;
          for (k = 0; k < 5; k++)
            swe_calc(jd + (k - 2) * h, rgb[ib].ipl, fl, d[k], serr);
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
          swe_calc(jd + k / 512.0, rgb[ib].ipl, fl, d[k], serr);
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
      printf("  %-6s %-16s speed lon %.2e lat %.2e deg/day, jitter lon %.2e lat %.2e deg\n",
             eph[ie].sz, rgb[ib].sz, wv[0], wv[1], wj[0], wj[1]);
      if (wv[0] > rgb[ib].limv || wv[1] > rgb[ib].limv) {
        printf("    FAIL: speed off its positions' rate by more than %.0e deg/day\n", rgb[ib].limv);
        bad++;
      }
      if (wj[0] > rgb[ib].limj || wj[1] > rgb[ib].limj) {
        printf("    FAIL: position jitters by more than %.0e deg between neighbouring instants\n", rgb[ib].limj);
        bad++;
      }
    }
  }
  printf(bad ? "G80 FAIL: %d\n" : "G80 PASS\n", bad);
  return bad ? 1 : 0;
}
