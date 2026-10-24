/* deflspeed.c -- G44: the light deflection's speed is smooth behind the
 * Sun's disc.
 *
 * swi_deflect_light() takes the deflection's rate as the change of the
 * deflection over 5e-7 day. Two of its quantities cancel exactly where a
 * planet passes behind the disc: sin(elongation) as sqrt(1 - (u.e)^2),
 * u.e ~ 1, and 1 + q.e, q.e ~ -1 (q the Sun-planet direction, e the
 * Sun-Earth). At 0.02 of the Sun's radius they are 1e-4 and 5e-9 and
 * carry 1e-12 and 1e-16 of rounding, which the step's small change turns
 * into a speed rough by 1.5e-5 deg/day from one instant to the next, 1e-9
 * day away (UPSTREAM-BUGS 39). Now |u x e| and |q + e|^2 / 2.
 *
 * The check, Swiss files, J2000 ecliptic (so the ecliptic's turning, bug
 * 33, is not at stake): at the five deepest conjunctions 2000-2030 of
 * Venus, Saturn, Mercury, Jupiter and Mars (the instants G1's meff rows
 * use, 0.02-0.43 of the Sun's radius from its centre), the longitude and
 * latitude speeds at seven instants 1e-9 day apart lie within 1e-7
 * deg/day of each other (measured <= 2.9e-8), and within 1e-6 of a
 * central difference of the positions over 2^-12 day at an instant that
 * is a multiple of it (<= 2.2e-7). The upstream build fails the first on
 * Venus (1.5e-5), Saturn and Jupiter, and the second on Venus and, by
 * its light-time rate (bug 35), Mercury. Run as
 *   deflspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(int argc, char **argv)
{
  static const struct { double t; int ipl; const char *n; } occ[5] = {
    {2457546.412153, SE_VENUS, "Venus"}, {2458862.136111, SE_SATURN, "Saturn"},
    {2459892.222917, SE_MERCURY, "Mercury"}, {2458845.268055, SE_JUPITER, "Jupiter"},
    {2460266.724653, SE_MARS, "Mars"},
  };
  const int32 fl0 = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_NONUT, fl = fl0 | SEFLG_SPEED;
  char serr[256];
  int i, k, j, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: deflspeed EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G44: the light deflection's speed is smooth behind the Sun's disc\n");
  for (i = 0; i < 5; i++) {
    double lo[2] = {1e9, 1e9}, hi[2] = {-1e9, -1e9}, x[6], a[6], b[6], h = 1.0 / 4096;
    double t = floor(occ[i].t * 4096) / 4096, off = 0, spread;
    for (k = -3; k <= 3; k++) {
      if (swe_calc(occ[i].t + k * 1e-9, occ[i].ipl, fl, x, serr) < 0) {
        printf("  %s ERROR %s\n", occ[i].n, serr);
        return 1;
      }
      for (j = 0; j < 2; j++) {
        if (x[3 + j] < lo[j]) lo[j] = x[3 + j];
        if (x[3 + j] > hi[j]) hi[j] = x[3 + j];
      }
    }
    swe_calc(t, occ[i].ipl, fl, x, serr);
    swe_calc(t + h, occ[i].ipl, fl0, a, serr);
    swe_calc(t - h, occ[i].ipl, fl0, b, serr);
    for (j = 0; j < 2; j++) {
      double d = a[j] - b[j];
      if (d > 180) d -= 360;
      if (d < -180) d += 360;
      if (fabs(x[3 + j] - d / (2 * h)) > off) off = fabs(x[3 + j] - d / (2 * h));
    }
    spread = fmax(hi[0] - lo[0], hi[1] - lo[1]);
    printf("  %-8s spread over 6e-9 day %.2e, from a central difference %.2e deg/day\n",
           occ[i].n, spread, off);
    if (spread > 1e-7) {
      printf("    FAIL: the deflection's speed is rough\n");
      bad++;
    }
    if (off > 1e-6) {
      printf("    FAIL: the speed is not the rate of the position\n");
      bad++;
    }
  }
  swe_close();
  printf(bad ? "G44 FAIL: %d\n" : "G44 PASS\n", bad);
  return bad ? 1 : 0;
}
