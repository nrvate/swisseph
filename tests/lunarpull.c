/* lunarpull.c -- G61: a far body's aberration rate carries the Moon's pull
 * on the Earth.
 *
 * The aberration's rate wants tau a(teval), tau the light time and a the
 * observer's acceleration (UPSTREAM-BUGS 34). For a body past ten days of
 * light time the term was tau times the Sun's pull alone, -GM r / r^3; below
 * ten days it was the difference of two Earth velocities plus tau^2 a'/2
 * with a' the Sun's pull's rate. The Moon's pull on the Earth, 0.6% of a and
 * 7.5% of a', was in neither (UPSTREAM-BUGS 62): 2.3e-7 deg/day on Nibiru's
 * speed, 6e-7 on VEA's. The term now takes the geocentric Moon from the save
 * area and adds its pull and the pull's rate, and takes tau a(teval) whole
 * past 0.3 day of light time when it has the Moon (the series fails for the
 * Moon's 27-day period well before it does for the Sun's year).
 *
 * The check, Swiss files: Nibiru, the Christ Comet, Planet 9 and VEA (light
 * times 3 to 460 days, from the ephe directory's seorbel.txt) and Saturn and
 * Pluto as controls, apparent geocentric and topocentric (Zurich), seven
 * dates -300..+300 years: the longitude and latitude speeds within 5e-8
 * deg/day of a five-point difference of the positions over 2^-7 day at
 * multiples of it. Measured <= 2.3e-8 (the reference's own noise and the
 * topocentric diurnal part). The build before the fix is off by 1.9e-7 to
 * 5.1e-7 deg/day on the four far bodies (all eight rows fail); the upstream
 * build fails all twelve rows, by 5.6e-8 to 1.2e-4. Run as
 *   lunarpull EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(int argc, char **argv)
{
  static const struct { int ipl; const char *sz; } rgb[] = {
    {SE_FICT_OFFSET + 9, "Nibiru"}, {SE_FICT_OFFSET + 19, "Christ Comet"},
    {SE_FICT_OFFSET + 20, "Planet 9"}, {SE_FICT_OFFSET + 25, "VEA"},
    {SE_SATURN, "Saturn"}, {SE_PLUTO, "Pluto"},
  };
  const double h = 1.0 / 128;
  char serr[256];
  int ib, ic, y, k, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: lunarpull EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  printf("G61: far bodies' speeds carry the Moon's pull on the Earth\n");
  for (ib = 0; ib < 6; ib++) {
    for (ic = 0; ic < 2; ic++) {
      int32 fl = SEFLG_SWIEPH | (ic ? SEFLG_TOPOCTR : 0);
      double w[2] = {0, 0};
      for (y = 0; y < 7; y++) {
        double jd = floor((2451545.0 + (-300 + y * 100) * 365.25 + 37.3 * y) * 256) / 256;
        double x[6], d[5][6];
        if (swe_calc(jd, rgb[ib].ipl, fl | SEFLG_SPEED, x, serr) < 0) {
          printf("  %-14s ERROR %s\n", rgb[ib].sz, serr);
          bad++;
          break;
        }
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
      printf("  %-14s %-5s lon %.2e lat %.2e deg/day\n", rgb[ib].sz,
             ic ? "topo" : "geo", w[0], w[1]);
      if (w[0] > 5e-8 || w[1] > 5e-8) {
        printf("    FAIL: speed off its positions' rate by more than 5e-8 deg/day\n");
        bad++;
      }
    }
  }
  printf(bad ? "G61 FAIL: %d\n" : "G61 PASS\n", bad);
  return bad ? 1 : 0;
}
