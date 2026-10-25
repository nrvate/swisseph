/* moonspeed.c -- G40: the Moon's speed is the rate of its own position.
 *
 * Two upstream defects, both in the speed alone (UPSTREAM-BUGS 31, 32):
 *   31. calc_speed() fits a parabola x1 + b s + a s^2 through the positions
 *       at t - dt, t, t + dt and returned 2 a + b, its slope at t + dt: the
 *       speed 4.32 s late for the Moon (8.64 s for a planet). That is the
 *       three-point speed SEFLG_SPEED3 asks for, and the one swe_calc()
 *       uses for every topocentric request with aberration: 1.2e-3
 *       deg/day on the topocentric Moon. The Moshier Moon's own velocity
 *       (swemmoon.c) had the same formula.
 *   32. app_pos_etc_moon() gave M(t - tau) - O(t) the velocity
 *       v_M - v_O, leaving out the rate of the light time, v_M dtau/dt:
 *       3.1e-4 deg/day topocentric, 1.7e-5 geocentric.
 *
 * The check: the right ascension and declination speeds, J2000 equator
 * without nutation (so no frame of date turns underneath), against a
 * five-point difference of the library's own positions, h = 0.01 day,
 * good to ~3e-6 deg/day on the topocentric Moon. Swiss files, de200.eph
 * and Moshier, 13 dates 1800-2100, Zurich. The SEFLG_SPEED3 cases carry
 * the positions' 0.1 mas roughness over 8.64 s, ~3e-5 deg/day. The
 * topocentric speed with aberration was three-point too until bug 34; it
 * is analytic now, and held to the analytic limit on the files.
 *
 * Moshier is held to the same limits: its Moon is carried to t - tau now
 * (UPSTREAM-BUGS 53), where it was moved back along its velocity at t and
 * its speed lacked tau times the acceleration, 1.1e-4 deg/day without
 * aberration (G52 checks it closer).
 * The upstream build fails 13 of the 15 judged. Run as
 *   moonspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static double wrap(double d)
{
  while (d > 180) d -= 360;
  while (d < -180) d += 360;
  return d;
}

/* worst |speed - reference| over lon and lat, deg/day */
static double worst_off(swe_ctx *c, int ipl, int32 fl, int32 fspeed)
{
  double w = 0;
  char serr[256];
  int id, k, i;
  for (id = 0; id < 13; id++) {
    double jd = 2378496.5 + id * 8431.07 + 0.137, x[6], f[5][6];
    if (swe_calc_r(c, jd, ipl, fl | fspeed, x, serr) < 0) {
      printf("  ERROR %s\n", serr);
      return 1e9;
    }
    for (k = 0; k < 5; k++)
      swe_calc_r(c, jd + (k - 2) * 0.01, ipl, fl, f[k], serr);
    for (i = 0; i < 2; i++) {
      double r = (wrap(f[0][i] - f[2][i]) - 8 * wrap(f[1][i] - f[2][i])
                + 8 * wrap(f[3][i] - f[2][i]) - wrap(f[4][i] - f[2][i])) / 0.12;
      if (fabs(x[i + 3] - r) > w) w = fabs(x[i + 3] - r);
    }
  }
  return w;
}

int main(int argc, char **argv)
{
  static const int32 rgEphe[3] = {SEFLG_SWIEPH, SEFLG_JPLEPH, SEFLG_MOSEPH};
  static const char *rgszEphe[3] = {"Swiss", "JPL", "Moshier"};
  const int32 frame = SEFLG_J2000 | SEFLG_NONUT | SEFLG_EQUATORIAL;
  /* limits for the files, then for Moshier */
  struct { int ipl; int32 fl, fspeed; const char *sz; double lim, limMosh; } rgCase[] = {
    {SE_MOON, 0, SEFLG_SPEED, "Moon geocentric", 5e-6, 5e-6},
    {SE_MOON, SEFLG_TOPOCTR, SEFLG_SPEED, "Moon topocentric", 5e-6, 5e-6},
    {SE_MOON, SEFLG_TOPOCTR | SEFLG_NOABERR, SEFLG_SPEED, "Moon topocentric, no aberr", 5e-6, 5e-6},
    {SE_MOON, 0, SEFLG_SPEED3, "Moon SEFLG_SPEED3", 5e-5, 5e-5},
    {SE_MARS, 0, SEFLG_SPEED3, "Mars SEFLG_SPEED3", 1e-5, 1e-5},
  };
  int ie, k, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: moonspeed EPHEDIR\n");
    return 2;
  }
  printf("G40: the Moon's speed is the rate of its own position\n");
  for (ie = 0; ie < 3; ie++) {
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, argv[1]);
    if (rgEphe[ie] == SEFLG_JPLEPH)
      swe_set_jpl_file_r(c, "de200.eph");
    swe_set_topo_r(c, 8.55, 47.37, 400);
    for (k = 0; k < (int) (sizeof(rgCase) / sizeof(rgCase[0])); k++) {
      double lim = rgEphe[ie] == SEFLG_MOSEPH ? rgCase[k].limMosh : rgCase[k].lim;
      double w = worst_off(c, rgCase[k].ipl, rgEphe[ie] | frame | rgCase[k].fl,
                           rgCase[k].fspeed);
      if (lim > 0)
        printf("  %-7s %-28s worst %.2e deg/day (limit %.0e)\n", rgszEphe[ie],
               rgCase[k].sz, w, lim);
      else
        printf("  %-7s %-28s worst %.2e deg/day (not judged)\n", rgszEphe[ie],
               rgCase[k].sz, w);
      if (lim > 0 && w > lim) {
        printf("    FAIL: the speed is not the rate of the position\n");
        bad++;
      }
    }
    swe_ctx_free(c);
  }
  printf(bad ? "G40 FAIL: %d\n" : "G40 PASS\n", bad);
  return bad ? 1 : 0;
}
