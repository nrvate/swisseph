/* fictneg.c -- G59: a fictitious body with a plain negative mean anomaly
 * moves at its daily motion.
 *
 * check_t_terms() took any '+' or '-' in a seorbel.txt field as the sign
 * of a further term, so a mean anomaly written as a plain negative number
 * ("-12.5", no T) was read as a polynomial in T: read_elements_file() then
 * set epoch = tjd, which adds no daily motion, and the body froze at its
 * epoch anomaly (UPSTREAM-BUGS 60). The same ellipse written 347.5 moved.
 *
 * The check, an element file written here and found first on the path:
 * two sets with the same ellipse (a = 1 AU, e = 0.1, epoch J2000), the
 * mean anomaly "-12.5" and "347.5", heliocentric geometric J2000, at five
 * dates 10..200 days after the epoch. (1) the negative set's longitude is
 * within 1e-9 deg of the positive set's; (2) it has advanced by the daily
 * motion, the longitude differs from the epoch's by > 150 deg at the
 * latest date (mean motion 0.9856 deg/day); (3) its longitude speed is within 1e-7
 * deg/day of a five-point difference of its positions over 2^-6 day at
 * multiples of it, and the distance rate within 1e-9 per unit distance.
 * Measured with the fix: (1) 0, (2) 161 deg (the largest of the five dates;
 * 200 days at 0.9856 deg/day is 197 deg less the orbit's equation of the
 * centre), (3) 2.6e-12 deg/day and 1.3e-14. The upstream build fails: the
 * negative body stands at its epoch position, (1) 161 deg off the positive
 * set, (2) 0 deg, (3) a reported speed of 1.2 deg/day against a rate of 0
 * and a distance rate off by 5.1e-4 per day. Run as
 *   fictneg EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

static const char *szElements =
  "J2000, J2000, -12.5, 1.0, 0.1, 30, 40, 5, Negative\n"
  "J2000, J2000, 347.5, 1.0, 0.1, 30, 40, 5, Positive\n";

int main(int argc, char **argv)
{
  const int32 fl = SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS;
  const double h = 1.0 / 64;
  char dir[] = "/tmp/fictnegXXXXXX", path[4096], serr[256];
  double e0[6], wpos = 0, wlon = 0, wdist = 0, adv = 0;
  int y, k, bad = 0;
  FILE *pf;

  if (argc != 2) {
    fprintf(stderr, "usage: fictneg EPHEDIR\n");
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
  printf("G59: a plain negative mean anomaly moves at the daily motion\n");
  swe_calc(2451545.0, SE_FICT_OFFSET + 0, fl, e0, serr);
  for (y = 0; y < 5; y++) {
    double jd = floor((2451545.0 + 10 + 47.5 * y) * 256) / 256;
    double x[6], p[6], d[5][6], v[5], r, e;
    if (swe_calc(jd, SE_FICT_OFFSET + 0, fl | SEFLG_SPEED, x, serr) < 0 ||
        swe_calc(jd, SE_FICT_OFFSET + 1, fl, p, serr) < 0) {
      printf("  ERROR %s\n", serr);
      bad++;
      break;
    }
    wpos = fmax(wpos, fabs(swe_difdeg2n(x[0], p[0])));
    adv = fmax(adv, fabs(swe_difdeg2n(x[0], e0[0])));
    for (k = 0; k < 5; k++) {
      swe_calc(jd + (k - 2) * h, SE_FICT_OFFSET + 0, fl, d[k], serr);
      v[k] = d[2][0] + swe_difdeg2n(d[k][0], d[2][0]);
    }
    r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
    wlon = fmax(wlon, fabs(x[3] - r));
    r = (d[0][2] - 8 * d[1][2] + 8 * d[3][2] - d[4][2]) / (12 * h);
    e = fabs(x[5] - r) / x[2];
    wdist = fmax(wdist, e);
  }
  printf("  vs positive %.2e deg, advance %.1f deg, speed %.2e deg/day, dist rate %.2e /day\n",
         wpos, adv, wlon, wdist);
  if (wpos > 1e-9) { printf("    FAIL: differs from the same ellipse written 347.5\n"); bad++; }
  if (adv < 150) { printf("    FAIL: has not moved by the daily motion\n"); bad++; }
  if (wlon > 1e-7) { printf("    FAIL: longitude speed off its positions' rate\n"); bad++; }
  if (wdist > 1e-9) { printf("    FAIL: distance rate off\n"); bad++; }
  unlink(path);
  rmdir(dir);
  printf(bad ? "G59 FAIL: %d\n" : "G59 PASS\n", bad);
  return bad ? 1 : 0;
}
