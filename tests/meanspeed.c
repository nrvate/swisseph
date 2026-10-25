/* meanspeed.c -- G114: swe_calc()'s mean node and mean apogee speeds are
 * the rates of their positions.
 *
 * The defect (UPSTREAM-BUGS 117): the speed was (x(t) - x(t - 0.001)) /
 * 0.001, with t - 0.001 rounded to a double. The step taken is 0.001 to
 * half an ulp of the date (2.3e-10 day near the present), 2.3e-7 of it:
 * the node's 0.053 deg/day off by up to 1.2e-8 (8.5e-9 at the present),
 * the apogee's 0.111 by 2.5e-8, its latitude speed by 2.1e-8 with half
 * the projection's curvature times the step; on every ephemeris and frame
 * alike (the mean elements are Moshier's in all of them).
 *
 * The check: SE_MEAN_NODE and SE_MEAN_APOG, the ecliptic of date, without
 * nutation, J2000 and equatorial; Swiss files and Moshier at 24 dates
 * -3000..3000, de200 at 12 dates 1600..2160; the longitude and latitude
 * speeds against five-point differences over 2^-4 day at binary-fraction
 * instants, within 2e-9 deg/day (measured <= 1.3e-10, the ancient
 * positions' own noise; <= 3.1e-11 on de200, 1600-2160): 480 checks. The
 * build before the fix fails all 480 (8.9e-9 to 2.5e-8: at binary-fraction
 * instants the step's rounding is nearly the same at every date of a
 * binade), the upstream build all 480 too (up to 1.4e-5, the frame rates
 * of earlier entries not there either). Run as
 *   meanspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static int bad = 0, nchk = 0;

/* worst of longitude and latitude speed less their five-point rates */
static double resid(double t, int ipl, int32 fl, double h)
{
  double x[6], p[5][6], w = 0;
  char serr[256];
  int i, j;
  if (swe_calc(t, ipl, fl | SEFLG_SPEED, x, serr) < 0)
    return HUGE_VAL;
  for (i = 0; i < 5; i++)
    if (swe_calc(t + (i - 2) * h, ipl, fl, p[i], serr) < 0)
      return HUGE_VAL;
  for (j = 0; j < 2; j++) {
    double d1 = p[3][j] - p[1][j], d2 = p[4][j] - p[0][j], r;
    if (j == 0) {
      d1 -= 360 * floor(d1 / 360 + 0.5);
      d2 -= 360 * floor(d2 / 360 + 0.5);
    }
    r = fabs(x[3 + j] - (8 * d1 - d2) / (12 * h));
    if (!(r <= w))
      w = r;
  }
  return w;
}

int main(int argc, char **argv)
{
  static const int body[2] = {SE_MEAN_NODE, SE_MEAN_APOG};
  static const char *bname[2] = {"mean node", "mean apogee"};
  static const int32 frame[4] = {0, SEFLG_NONUT, SEFLG_J2000, SEFLG_EQUATORIAL};
  static const char *fname[4] = {"of date", "no nutation", "J2000", "equatorial"};
  static const struct { int32 e; const char *n; double y0, y1; int n_; } eph[3] = {
    {SEFLG_SWIEPH, "Swiss", -3000, 3000, 24}, {SEFLG_MOSEPH, "Moshier", -3000, 3000, 24},
    {SEFLG_JPLEPH, "de200", 1600, 2160, 12},
  };
  int b, e, f, k;
  if (argc != 2) {
    fprintf(stderr, "usage: meanspeed EPHEDIR\n");
    return 2;
  }
  printf("G114: the mean node's and apogee's speeds are their positions' rates\n");
  swe_set_ephe_path(argv[1]);
  swe_set_jpl_file("de200.eph");
  for (b = 0; b < 2; b++)
    for (e = 0; e < 3; e++)
      for (f = 0; f < 4; f++) {
        double w = 0;
        for (k = 0; k < eph[e].n_; k++) {
          double y = eph[e].y0 + (eph[e].y1 - eph[e].y0) * k / (eph[e].n_ - 1);
          double t = floor((2451545.0 + (y - 2000) * 365.25 + 0.3) * 256) / 256;
          double r = resid(t, body[b], eph[e].e | frame[f], 1.0 / 16);
          nchk++;
          if (!(r <= 2e-9)) {
            if (bad < 10)
              printf("    FAIL: %s %s %s at %.4f: %.2e deg/day (limit 2e-9)\n",
                     eph[e].n, bname[b], fname[f], t, r);
            bad++;
          }
          if (!(r <= w))
            w = r;
        }
        printf("  %-7s %-11s %-11s worst %.2e deg/day\n", eph[e].n, bname[b], fname[f], w);
      }
  swe_close();
  printf("  %d checks\n", nchk);
  printf(bad ? "G114 FAIL: %d\n" : "G114 PASS\n", bad);
  return bad ? 1 : 0;
}
