/* orbelspeed.c -- G48: a fictitious body's speed is the rate of its own
 * position, for elements that move.
 *
 * swi_osc_el_plan() took the speed as the two-body motion of a fixed
 * ellipse, K / sqrt(a), K Gauss's constant, while the position follows
 * seorbel.txt's polynomials in T (UPSTREAM-BUGS 48): a mean anomaly with
 * its own rate (Vulcan's 707550.7341 deg/cy, 19.3717 deg/day, against the
 * 19.3428 that a = 0.13744 AU gives), the semi-axis, eccentricity,
 * perihelion, node and inclination in T (Vulcan's node and perihelion at
 * 1670 deg/cy, a planet's elements of date), and, for an equinox of date
 * (JDATE), a frame that precesses with t. Vulcan's heliocentric longitude
 * speed was off by 2.9e-2 deg/day, Waldemath's geocentric one by 1.3e-2,
 * the Mercury set's by 9.4e-6, every JDATE set's by the precession,
 * 3.8e-5, and an Earth-centred set's by 4.8e-7 even with fixed elements
 * (K = KGAUSS_GEO, 7.6e-8 off the daily motion the position uses). The
 * speed is now the derivative of the position as computed.
 *
 * The check, Swiss files, an element file written here and found first on
 * the path: seven sets -- Vulcan, Waldemath and Selena (Earth-centred,
 * T terms), seorbel.txt's Mercury of date (T terms on every element),
 * Proserpina (JDATE, constant elements), Harrington (fixed ellipse, the
 * control) and a set with T terms on the semi-axis, eccentricity, angles
 * and a T2 term but not on the mean anomaly (so dM/dt carries the daily
 * motion's change with a) -- heliocentric and geocentric, geometric,
 * J2000 ecliptic, at seven dates -300..+300 years: the longitude and
 * latitude speeds within 1e-7 deg/day of a five-point difference of the
 * positions over 2^-6 day at multiples of it, and the distance rate
 * within 1e-9 of the distance per day. Measured <= 8.4e-9 deg/day and
 * 1.6e-11, most of it the reference's own noise: Vulcan's mean anomaly
 * reaches 2.8e6 degrees at T = 4, and its rounding, 5e-10 degree, moves
 * each position along the orbit (5.6e-8 deg/day over 2^-8 day).
 * Geometric because an apparent position is the body at t - tau, which is
 * not a multiple of the step: the instant's rounding moves it along its
 * orbit by v * 5e-10 day, which the difference turns into 1e-7 deg/day on
 * Vulcan and 3e-7 on Waldemath, 0.007 AU from the Earth (the light time's
 * rate itself is bug 35's ground). Swiss files because Moshier's Earth
 * speed is itself off its positions by 1.8e-6 deg/day (the Sun's
 * geocentric longitude), which would mask a geocentric check of a
 * heliocentric set. The upstream build fails every set but Harrington,
 * by 2.1e-6 to 0.68 deg/day. Run as
 *   orbelspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

static const char *szElements =
  "J1900,JDATE, 252.8987988 + 707550.7341 * T, 0.13744, 0.019, "
    "322.212069+1670.056*T, 47.787931-1670.056*T, 7.5, Vulcan\n"
  "2414290.95827875,2414290.95827875, 70.3407215 + 109023.2634989 * T, "
    "0.0068400705250028, 0.1587, 8.14049594 + 2393.47417444 * T, "
    "136.24878256 - 1131.71719709 * T, 2.5, Waldemath, geo\n"
  "J2000,JDATE, 242.2205555 + 5143.5418158 * T, 0.05279142865925, 0.0, 0.0, "
    "0.0, 0.0, Selena, geo\n"
  "J2000, JDATE, 174.794787 + 149472.5157715 * T, 0.38709831, "
    "0.20563175 + 0.000020406 * T, 29.125226 + 0.3702885 * T, "
    "48.330893 + 1.186189 * T, 7.004986 + 0.0018215 * T, Mercury of date\n"
  "J1900,JDATE, 170.73, 79.225630, 0, 0, 0, 0, Proserpina\n"
  "2374696.5, J2000, 0.0, 101.2, 0.411, 208.5, 275.4, 32.4, Harrington\n"
  "J2000, J2000, 30.0, 2.7 + 0.3 * T, 0.08 + 0.02 * T, 70 + 5 * T, "
    "80 + 3 * T2, 10 + 2 * T, Moving ellipse\n";

int main(int argc, char **argv)
{
  static const char *rgszName[] = {"Vulcan", "Waldemath", "Selena",
    "Mercury of date", "Proserpina", "Harrington", "Moving ellipse"};
  static const struct { int32 fl; const char *sz; } rgCtr[] = {
    {SEFLG_HELCTR, "helio"}, {0, "geo"},
  };
  const int32 base = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS;
  const double h = 1.0 / 64;
  char dir[] = "/tmp/orbelspeedXXXXXX", path[4096], serr[256];
  int ib, ic, y, k, i, bad = 0;
  FILE *pf;

  if (argc != 2) {
    fprintf(stderr, "usage: orbelspeed EPHEDIR\n");
    return 2;
  }
  if (mkdtemp(dir) == NULL) { perror("mkdtemp"); return 2; }
  snprintf(path, sizeof(path), "%s/seorbel.txt", dir);
  if ((pf = fopen(path, "w")) == NULL) { perror(path); return 2; }
  fputs(szElements, pf);
  fclose(pf);
  snprintf(path, sizeof(path), "%s:%s", dir, argv[1]);
  swe_set_ephe_path(path);
  snprintf(path, sizeof(path), "%s/seorbel.txt", dir);
  printf("G48: fictitious bodies' speeds are the rates of their positions\n");
  for (ib = 0; ib < 7; ib++) {
    for (ic = 0; ic < 2; ic++) {
      int32 fl = base | rgCtr[ic].fl;
      double w[3] = {0, 0, 0};
      char sz[48];
      for (y = 0; y < 7; y++) {
        double jd = floor((2451545.0 + (-300 + y * 100) * 365.25 + 37.3 * y) * 256) / 256;
        double x[6], d[5][6];
        if (swe_calc(jd, SE_FICT_OFFSET + ib, fl | SEFLG_SPEED, x, serr) < 0) {
          printf("  %-24s ERROR %s\n", rgszName[ib], serr);
          bad++;
          break;
        }
        for (k = 0; k < 5; k++)
          swe_calc(jd + (k - 2) * h, SE_FICT_OFFSET + ib, fl, d[k], serr);
        for (i = 0; i < 3; i++) {
          /* the five positions, the longitude unwrapped about the middle one */
          double v[5], r, e;
          for (k = 0; k < 5; k++) {
            v[k] = d[k][i];
            if (i == 0)
              v[k] = d[2][0] + swe_difdeg2n(v[k], d[2][0]);
          }
          r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
          e = fabs(x[3 + i] - r);
          if (i == 2)
            e /= x[2];		/* the distance rate per unit distance */
          if (e > w[i])
            w[i] = e;
        }
      }
      snprintf(sz, sizeof sz, "%s, %s", rgszName[ib], rgCtr[ic].sz);
      printf("  %-24s lon %.2e lat %.2e deg/day, dist %.2e /day\n", sz, w[0], w[1], w[2]);
      if (w[0] > 1e-7 || w[1] > 1e-7) {
        printf("    FAIL: speed off its positions' rate by more than 1e-7 deg/day\n");
        bad++;
      }
      if (w[2] > 1e-9) {
        printf("    FAIL: distance rate off by more than 1e-9 of the distance per day\n");
        bad++;
      }
    }
  }
  unlink(path);
  rmdir(dir);
  printf(bad ? "G48 FAIL: %d\n" : "G48 PASS\n", bad);
  return bad ? 1 : 0;
}
