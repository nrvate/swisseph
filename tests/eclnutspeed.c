/* eclnutspeed.c -- G89: swe_calc(SE_ECL_NUT) answers the same with every speed flag.
 *
 * The defect (UPSTREAM-BUGS 90): SEFLG_SPEED3 ran calc_speed() over the four
 * angles of SE_ECL_NUT (true and mean obliquity, nutation in longitude and
 * in obliquity), which are not a position. x[3], the nutation in obliquity
 * (-1.6026e-3 deg), came back as the true obliquity's rate (-6.06e-6) and
 * x[4], x[5] as rates of the mean obliquity and nutation in longitude
 * (up to 5e-5); denormalize_positions() read two doubles never written
 * (valgrind: 576 errors).
 *
 * The check: SE_ECL_NUT with no speed flag, SEFLG_SPEED and SEFLG_SPEED3 at
 * six epochs (-2000..2800, Swiss files and Moshier), each of the six slots
 * bit for bit the same across the three flag sets; the slots 4 and 5 zero;
 * x[3] the nutation in obliquity, equal to x[0] - x[1]; and the RADIANS
 * flag the same way. Measured: 0 differences. The upstream build fails by
 * 1.6e-3 deg in x[3] and 5e-5 in x[4], x[5]; ts20 before this fix the same.
 * Run as
 *   eclnutspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(int argc, char **argv)
{
  static const double rgYear[6] = {-2000, 0, 1500, 1950, 2026, 2800};
  static const int32 rgEph[2] = {SEFLG_SWIEPH, SEFLG_MOSEPH};
  static const int32 rgSp[3] = {0, SEFLG_SPEED, SEFLG_SPEED3};
  int e, y, s, r, k, bad = 0, n = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: eclnutspeed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G89: SE_ECL_NUT is the same with no speed flag, SPEED and SPEED3\n");
  for (e = 0; e < 2; e++)
    for (r = 0; r < 2; r++)
      for (y = 0; y < 6; y++) {
        double t = floor((2451545.0 + (rgYear[y] - 2000) * 365.25 + 0.1234567) * 256) / 256;
        double x[3][6];
        char serr[256];
        for (s = 0; s < 3; s++) {
          int32 fl = rgEph[e] | rgSp[s] | (r ? SEFLG_RADIANS : 0);
          /* a junk-filled array: the call must set all six slots */
          for (k = 0; k < 6; k++) x[s][k] = 7;
          if (swe_calc_r(c, t, SE_ECL_NUT, fl, x[s], serr) < 0) {
            printf("  ERROR jd %.4f: %s\n", t, serr);
            return 1;
          }
        }
        for (s = 1; s < 3; s++)
          for (k = 0; k < 6; k++) {
            n++;
            if (x[s][k] != x[0][k]) {
              bad++;
              printf("  FAIL eph %d%s jd %.4f flag %d slot %d: %.9g vs %.9g\n",
                     e, r ? " rad" : "", t, s, k, x[s][k], x[0][k]);
            }
          }
        n += 3;
        if (x[0][4] != 0 || x[0][5] != 0 || fabs(x[0][3] - (x[0][0] - x[0][1])) > 1e-14) {
          bad++;
          printf("  FAIL layout jd %.4f: %.9g %.9g %.9g\n", t, x[0][3], x[0][4], x[0][5]);
        }
      }
  swe_ctx_free(c);
  if (bad) {
    printf("G89 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d slots agree\nG89 PASS\n", n);
  return 0;
}
