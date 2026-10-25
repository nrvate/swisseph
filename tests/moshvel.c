/* moshvel.c -- G84: the Moshier Earth's velocity is smooth and the rate of
 * its position.
 *
 * The Moshier Earth is the EMB series less the Moon's share of a short
 * lunar series (embofs_mosh()). The offset's rate was a central difference
 * of that series over +-2^-8 day, and the series' arguments, degrees of
 * 481267.88 T, round at 1e-12 rad once T is near 1 (5e-11 at -3000): the
 * Earth's velocity was rough from one instant to the next at ~2e-14 AU/day,
 * and the difference's truncation left ~1e-12 AU/day of bias
 * (UPSTREAM-BUGS 85).
 *
 * The check, heliocentric J2000 Earth, TRUEPOS, at -3000, 1900, 2000, 2100
 * and 2970 (the Moshier range ends in 2973), instants that are multiples of
 * 2^-8 day: (1) its acceleration from the velocities at t +- 1e-5 day
 * against the five-point second difference of the positions over 2^-3
 * day, within 1e-7 of the acceleration; (2) the velocity against the
 * five-point difference of the positions over 2^-4 day, within 5e-13
 * AU/day. 10 checks. Before the fix (1) was 3.8e-5, 4.3e-9, 2.1e-6,
 * 2.1e-7 and 1.7e-5, (2) 1.6e-12, 1.2e-12, 7.9e-13, 1.1e-12, 8.2e-13: 9 of
 * 10 fail. Now (1) 2.9e-8 at -3000 (the EMB series' own roughness), <=
 * 9.4e-9 elsewhere; (2) <= 1.9e-13, what the obliquity's turning leaves
 * (the offset is rotated with ctx->oec, of each position's own date; the
 * rate holds it). The upstream build fails all 10. Run as
 *   moshvel EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static const int32 fl = SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT
                        | SEFLG_TRUEPOS | SEFLG_XYZ | SEFLG_SPEED | SEFLG_ICRS;

static int earth(double t, double *x)
{
  char serr[256];
  if (swe_calc(t, SE_EARTH, fl, x, serr) < 0) {
    printf("  ERROR %s\n", serr);
    return -1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  static const double date[5] = {625307.5, 2415020.5, 2451545.0, 2488070.5, 2816787.5};
  static const char *name[5] = {"-3000", "1900", "2000", "2100", "2970"};
  const double H = 1.0 / 8, Hv = 1.0 / 16, d = 1e-5;
  int k, i, n, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: moshvel EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G84: the Moshier Earth's velocity is smooth and the rate of its position\n");
  for (n = 0; n < 5; n++) {
    double t = floor(date[n] * 256) / 256, q[5][6], r[5][6], x0[6], xp[6], xm[6];
    double ea = 0, an = 0, ev = 0;
    for (k = 0; k < 5; k++)
      if (earth(t + (k - 2) * H, q[k]) || earth(t + (k - 2) * Hv, r[k]))
        return 1;
    if (earth(t, x0) || earth(t + d, xp) || earth(t - d, xm))
      return 1;
    for (i = 0; i < 3; i++) {
      double ar = (-q[4][i] + 16 * q[3][i] - 30 * q[2][i] + 16 * q[1][i] - q[0][i]) / (12 * H * H);
      double av = (xp[i+3] - xm[i+3]) / ((t + d) - (t - d));
      double vr = (8 * (r[3][i] - r[1][i]) - (r[4][i] - r[0][i])) / (12 * Hv);
      ea += (av - ar) * (av - ar);
      an += ar * ar;
      ev += (x0[i+3] - vr) * (x0[i+3] - vr);
    }
    ea = sqrt(ea / an);
    ev = sqrt(ev);
    if (ea > 1e-7)
      bad++;
    if (ev > 5e-13)
      bad++;
    printf("  %-5s acceleration %.2e of itself%s, velocity %.2e AU/day%s\n", name[n],
           ea, ea > 1e-7 ? " FAIL" : "", ev, ev > 5e-13 ? " FAIL" : "");
  }
  if (bad)
    printf("G84 FAIL: %d\n", bad);
  else
    printf("G84 PASS\n");
  return bad ? 1 : 0;
}
