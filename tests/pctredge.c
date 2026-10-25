/* pctredge.c -- G144: swe_calc_pctr() whose centre's files begin between
 * the light-time instant and the date refuses the speed it cannot form,
 * and answers the rest.
 *
 * The defect (UPSTREAM-BUGS 158): swe_calc_pctr() reads the centre and the
 * body again at t = tjd - tau, the light-time instant, and assigned both
 * reads to retc, the body's after the centre's, so the centre's failure
 * was never tested. A centre whose files begin between t and tjd was taken
 * as zero at t: the aberration rate's centre acceleration (upstream: the
 * centre's velocity at tjd less that at t) came from a zeroed vector.
 * Mars from Ceres with seas_18.se1 (Ceres from 1800 Jan 1, JD 2378496.5)
 * and the planet and Moon files of 1200-2400, in the first 0.024 day of
 * Ceres's file: return 258 (SEFLG_SWIEPH | SEFLG_SPEED) with a longitude
 * speed of 0.27677 deg/day for 0.28186, 5.09e-3 off (0.130 in the
 * upstream build, 0.15176); the position was right.
 *
 * The checks, in a directory of links to sepl_12, sepl_18, semo_12,
 * semo_18 and seas_18, Mars from Ceres at t0 + k/256 + 1/1024:
 *  (a) k = 0..5, where the centre's read at t fails: SEFLG_SPEED is
 *      refused (ERR, with the missing file named);
 *  (b) the same instants without SEFLG_SPEED, and with SEFLG_SPEED |
 *      SEFLG_NOABERR (which reads no centre acceleration), answer OK:
 *      their position needs no centre at t;
 *  (c) k = 12..15, every read in range and every stencil point in
 *      sepl_18.se1: the longitude, latitude and distance speeds within
 *      1e-8 deg/day (1e-11 AU/day) of five-point central differences of
 *      the positions over 2^-7 day.
 * 30 checks. Measured: (c) 3e-12 deg/day. Before the fix (a) fails its
 * 6, answered 258 with the speed 5.09e-3 off. The upstream build fails
 * 18: the same 6 (0.130 off), and (c)'s 12 by its own speeds, 1.7e-6
 * deg/day off the differences (UPSTREAM-BUGS 77). Run as
 *   pctredge EPHEDIR */

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

static const char *FILES[] = { "sepl_12.se1", "sepl_18.se1", "semo_12.se1", "semo_18.se1",
                               "seas_18.se1" };
#define NFILES ((int) (sizeof FILES / sizeof FILES[0]))
static const double T0 = 2378496.5;	/* seas_18.se1's first instant */
static int bad;

static int32 pctr(double t, int32 fl, double *x, char *serr)
{
  return swe_calc_pctr(t, SE_MARS, SE_CERES, fl, x, serr);
}

int main(int argc, char **argv)
{
  char ephe[PATH_MAX], dir[] = "/tmp/pctredge-XXXXXX", src[PATH_MAX + 64], dst[PATH_MAX + 64];
  const int32 fs = SEFLG_SWIEPH | SEFLG_SPEED;
  double h = 1.0 / 128, worst[3] = {0, 0, 0};
  int i, k;
  if (argc != 2 || realpath(argv[1], ephe) == NULL) {
    fprintf(stderr, "usage: pctredge EPHEDIR\n");
    return 2;
  }
  if (mkdtemp(dir) == NULL) {
    printf("G144 FAIL: cannot make %s\n", dir);
    return 1;
  }
  for (i = 0; i < NFILES; i++) {
    snprintf(src, sizeof src, "%s/%s", ephe, FILES[i]);
    snprintf(dst, sizeof dst, "%s/%s", dir, FILES[i]);
    if (symlink(src, dst) != 0) {
      printf("G144 FAIL: cannot link %s\n", FILES[i]);
      return 1;
    }
  }
  swe_set_ephe_path(dir);
  printf("G144: Mars from Ceres where Ceres's file begins between t and tjd\n");
  for (k = 0; k <= 5; k++) {
    double t = T0 + k / 256.0 + 1.0 / 1024, x[6];
    char serr[256] = "";
    int32 r = pctr(t, fs, x, serr), r1, r2;
    int ok = r == ERR && serr[0] != '\0';
    printf("  (a) t0+%.4f SEFLG_SPEED: %d%s", t - T0, r, ok ? "" : "  FAIL: expected ERR");
    if (r != ERR)
      printf(", longitude speed %.9f", x[3]);
    printf("\n");
    if (r == ERR)
      printf("      %s\n", serr);
    if (!ok) bad++;
    r1 = pctr(t, SEFLG_SWIEPH, x, serr);
    r2 = pctr(t, fs | SEFLG_NOABERR, x, serr);
    printf("  (b) t0+%.4f no speed: %d, SEFLG_NOABERR: %d%s\n", t - T0, r1, r2,
           r1 == ERR || r2 == ERR ? "  FAIL: expected OK" : "");
    if (r1 == ERR) bad++;
    if (r2 == ERR) bad++;
  }
  for (k = 12; k <= 15; k++) {
    double t = T0 + k / 256.0 + 1.0 / 1024, x[6], p[4][6], d;
    char serr[256];
    static const double off[4] = {-2, -1, 1, 2};
    int fail = pctr(t, fs, x, serr) == ERR, j;
    for (j = 0; j < 4; j++)
      if (pctr(t + off[j] * h, SEFLG_SWIEPH, p[j], serr) == ERR)
        fail = 1;
    for (i = 0; i < 3; i++) {
      d = (p[0][i] - 8 * p[1][i] + 8 * p[2][i] - p[3][i]) / (12 * h);
      if (fabs(x[i + 3] - d) > worst[i])
        worst[i] = fabs(x[i + 3] - d);
      if (fail || fabs(x[i + 3] - d) > (i == 2 ? 1e-11 : 1e-8))
        bad++;
    }
  }
  printf("  (c) t0+0.0479..0.0596, speeds against five-point differences: %.2e, %.2e deg/day, %.2e AU/day (limits 1e-8, 1e-11)\n",
         worst[0], worst[1], worst[2]);
  swe_close();
  for (i = 0; i < NFILES; i++) {
    snprintf(dst, sizeof dst, "%s/%s", dir, FILES[i]);
    unlink(dst);
  }
  rmdir(dir);
  if (bad) {
    printf("G144 FAIL: %d\n", bad);
    return 1;
  }
  printf("G144 PASS (30)\n");
  return 0;
}
