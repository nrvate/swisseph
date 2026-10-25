/* pctrspeed.c -- G76: swe_calc_pctr()'s speeds are the rates of its
 * positions.
 *
 * Two defects in swe_calc_pctr() (UPSTREAM-BUGS 77). Both bodies were read
 * at t = tjd - dt rounded to a double, up to half an ulp of the date
 * (2.3e-10 day) from the emission instant, so the apparent position jumped
 * by the body's motion over that from one date to the next: a noise that a
 * five-point difference over 2^-9 day turns into 9.8e-8 deg/day on Mercury
 * from Mars, 1.8e-7 on the Moon from Venus, against 4e-10 for true
 * positions (bug 63's, in app_pos_etc_plan()). And the aberration rate
 * added the centre's velocity at tjd less that at t whole: its radial part
 * put the distance rate off by 1.5e-6 AU/day on Mercury from Mars, 5.5e-6
 * on the Moon from Venus, 1.3e-4 on Saturn from Mercury, and as tau
 * a(tjd - tau/2) it put Saturn's longitude speed from Mercury off by
 * 2.1e-6 deg/day. The body is now moved along its velocity by the
 * rounding, and the rate is bug 52's exact term with tau a(tjd).
 *
 * The check, Swiss files: Mercury from Mars, the Moon from Venus, Jupiter
 * from Saturn, the Sun from Jupiter, Venus from the Moon, Saturn from
 * Mercury; default flags, SEFLG_NOABERR, SEFLG_TRUEPOS, SEFLG_J2000 and
 * SEFLG_EQUATORIAL; thirteen dates -3000..+3000, binary-fraction instants:
 * the longitude and latitude speeds within 5e-9 deg/day and the distance
 * rate within 5e-10 AU/day of five-point differences of the positions over
 * 2^-6, 2^-7, 2^-8 and 2^-9 day. Measured <= 6.0e-10 deg/day (true
 * positions 4.0e-10) and <= 4.1e-11 AU/day. The build before the fix fails
 * 22 of the 30 rows (the true positions pass, and the Sun from Jupiter
 * and Saturn from Mercury without aberration), by up to 2.1e-6
 * deg/day and 1.3e-4 AU/day; the upstream build fails all 30, by up to
 * 2.3e-6 deg/day in longitude, 1.9e-5 in declination, 1.3e-4 AU/day.
 * Run as
 *   pctrspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(int argc, char **argv)
{
  static const struct { int ipl, ictr; const char *sz; } rgp[] = {
    {SE_MERCURY, SE_MARS, "Mer/Mars"}, {SE_MOON, SE_VENUS, "Moon/Ven"},
    {SE_JUPITER, SE_SATURN, "Jup/Sat"}, {SE_SUN, SE_JUPITER, "Sun/Jup"},
    {SE_VENUS, SE_MOON, "Ven/Moon"}, {SE_SATURN, SE_MERCURY, "Sat/Mer"},
  };
  static const struct { int32 fl; const char *sz; } rgf[] = {
    {0, "default"}, {SEFLG_NOABERR, "noaberr"}, {SEFLG_TRUEPOS, "truepos"},
    {SEFLG_J2000, "j2000"}, {SEFLG_EQUATORIAL, "equat"},
  };
  char serr[256];
  int ip, ic, y, s, k, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: pctrspeed EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G76: swe_calc_pctr() speeds against its positions' rates\n");
  for (ip = 0; ip < 6; ip++) {
    for (ic = 0; ic < 5; ic++) {
      int32 fl = SEFLG_SWIEPH | rgf[ic].fl;
      double w[3] = {0, 0, 0};
      for (y = 0; y < 13; y++) {
        double yr = -3000 + y * 500.0;
        double jd = floor((2451545.0 + yr * 365.25 + 37.3 * y) * 256) / 256;
        double x[6], d[5][6];
        if (swe_calc_pctr(jd, rgp[ip].ipl, rgp[ip].ictr, fl | SEFLG_SPEED, x, serr) < 0) {
          printf("  %-8s ERROR %s\n", rgp[ip].sz, serr);
          bad++;
          break;
        }
        for (s = 6; s <= 9; s++) {
          double h = ldexp(1, -s);
          for (k = 0; k < 5; k++)
            swe_calc_pctr(jd + (k - 2) * h, rgp[ip].ipl, rgp[ip].ictr, fl, d[k], serr);
          for (i = 0; i < 3; i++) {
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
      printf("  %-8s %-7s lon %.2e lat %.2e deg/day, dist %.2e AU/day\n",
             rgp[ip].sz, rgf[ic].sz, w[0], w[1], w[2]);
      if (w[0] > 5e-9 || w[1] > 5e-9 || w[2] > 5e-10) {
        printf("    FAIL: speed off its positions' rate (5e-9 deg/day, 5e-10 AU/day)\n");
        bad++;
      }
    }
  }
  printf(bad ? "G76 FAIL: %d\n" : "G76 PASS\n", bad);
  return bad ? 1 : 0;
}
