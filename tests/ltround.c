/* ltround.c -- G62: a planet's light-time position is read at teval - tau,
 * not at its rounding, and its heliocentric light-time rate takes the
 * body's barycentric velocity.
 *
 * Two defects in app_pos_etc_plan() (UPSTREAM-BUGS 63). The body was read
 * at t = teval - dt rounded to a double, up to half an ulp of the date
 * (2.3e-10 day near the present) from the emission instant, so the
 * apparent position jumped by the body's motion over that from one date to
 * the next: ~1e-9 deg on Mercury heliocentric, a noise that a five-point
 * difference over 2^-9 day turns into 7.7e-7 deg/day heliocentric, 3.9e-7
 * on Venus geocentric. And the heliocentric light-time rate, bug 35's
 * rho^.(v_P - v_O) / (c + rho^.v_P) times v_P, was taken from the body
 * relative to the Sun at teval, so rho^.v_P and v_P had the Sun's
 * barycentric velocity (~13 m/s) taken out: 3e-8 deg/day on Mercury's
 * heliocentric speed. The body is now moved along its velocity by the
 * rounding, (teval - t) - dt, and the rate is taken from the barycentric
 * body and centre.
 *
 * The check, Swiss files: Mercury to Pluto and Ceres, Vesta, apparent
 * geocentric (default and SEFLG_NOABERR) and heliocentric, thirteen dates
 * -3000..+3000 for the planets and -600..+900 years from J2000 for the
 * asteroids, binary-fraction instants: the longitude and latitude speeds
 * within 5e-9 deg/day of five-point differences of the positions over
 * 2^-6, 2^-7, 2^-8 and 2^-9 day. Measured <= 7.4e-10 geocentric and
 * <= 1.6e-9 heliocentric (Mercury's true heliocentric positions are as far
 * from their own differences). The build before the fix is off by up to
 * 2.2e-7 geocentric and 4.7e-7 heliocentric, Mars by 5.3e-8 and Jupiter
 * by 5.9e-9 without aberration: 18 of the 30 rows fail (Mercury, Venus,
 * Mars, Jupiter, Ceres and Vesta); the upstream build fails every row, by
 * 8.9e-6 to 1.7e-4.
 * Run as
 *   ltround EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(int argc, char **argv)
{
  static const struct { int ipl; const char *sz; double y0, y1; } rgb[] = {
    {SE_MERCURY, "Mercury", -3000, 3000}, {SE_VENUS, "Venus", -3000, 3000},
    {SE_MARS, "Mars", -3000, 3000}, {SE_JUPITER, "Jupiter", -3000, 3000},
    {SE_SATURN, "Saturn", -3000, 3000}, {SE_URANUS, "Uranus", -3000, 3000},
    {SE_NEPTUNE, "Neptune", -3000, 3000}, {SE_PLUTO, "Pluto", -3000, 3000},
    {SE_CERES, "Ceres", -600, 900}, {SE_VESTA, "Vesta", -600, 900},
  };
  static const struct { int32 fl; const char *sz; } rgf[] = {
    {0, "geo"}, {SEFLG_NOABERR, "noaberr"}, {SEFLG_HELCTR, "helio"},
  };
  char serr[256];
  int ib, ic, y, s, k, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: ltround EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G62: planets' light-time positions and heliocentric rates\n");
  for (ib = 0; ib < 10; ib++) {
    for (ic = 0; ic < 3; ic++) {
      int32 fl = SEFLG_SWIEPH | rgf[ic].fl;
      double w[2] = {0, 0};
      for (y = 0; y < 13; y++) {
        double yr = rgb[ib].y0 + y * (rgb[ib].y1 - rgb[ib].y0) / 12;
        double jd = floor((2451545.0 + yr * 365.25 + 37.3 * y) * 256) / 256;
        double x[6], d[5][6];
        if (swe_calc(jd, rgb[ib].ipl, fl | SEFLG_SPEED, x, serr) < 0) {
          printf("  %-8s ERROR %s\n", rgb[ib].sz, serr);
          bad++;
          break;
        }
        for (s = 6; s <= 9; s++) {
          double h = ldexp(1, -s);
          for (k = 0; k < 5; k++)
            swe_calc(jd + (k - 2) * h, rgb[ib].ipl, fl, d[k], serr);
          for (i = 0; i < 2; i++) {
            double v[5], r, e;
            for (k = 0; k < 5; k++) {
              v[k] = d[k][i];
              if (i == 0)
                v[k] = d[2][0] + swe_difdeg2n(v[k], d[2][0]);
            }
            r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
            e = fabs(x[3 + i] - r);
            if (e > w[i])
              w[i] = e;
          }
        }
      }
      printf("  %-8s %-7s lon %.2e lat %.2e deg/day\n", rgb[ib].sz, rgf[ic].sz,
             w[0], w[1]);
      if (w[0] > 5e-9 || w[1] > 5e-9) {
        printf("    FAIL: speed off its positions' rate by more than 5e-9 deg/day\n");
        bad++;
      }
    }
  }
  printf(bad ? "G62 FAIL: %d\n" : "G62 PASS\n", bad);
  return bad ? 1 : 0;
}
