/* moonlt.c -- G33: the Moon's light time is the solution of its equation,
 * on every ephemeris, and its position does not depend on SEFLG_SPEED.
 *
 * app_pos_etc_moon() took the light time once, from the distance at the
 * instant of observation, tau0 = |M(t) - E(t)| / c, and read the Moon at
 * t - tau0. The light time is defined by tau = |M(t - tau) - E(t)| / c, and
 * the Moon moves about 1 km/s against the Earth and 30 km/s against the
 * barycentre within its 1.3 s of light time, so tau0 is off by up to ~0.1 ms
 * and the position by up to 1.08 mas over 1800-2100. The instant read,
 * t - tau0, is also a JD in one double, which resolves ~40 us near the
 * present: the Moon read there moves up to 0.18 mas more. Measured against
 * the Ephemeris Prometheia project's solved light time, the two together
 * reproduce Swiss's Moon to 0.00000 mas at eight dates.
 *
 * The check, per ephemeris (Moshier, the Swiss files, the JPL file
 * de200.eph), geocentric and heliocentric, at dates across 1800-2100, with
 * no golden file:
 *   1. the position with SEFLG_SPEED equals the position without it;
 *   2. the astrometric direction (ICRS, J2000, no nutation, aberration or
 *      deflection) equals M(t - tau) - O(t), with tau solved here from this
 *      library's own geometric barycentric Moon and observer, and the
 *      rounding of t - tau to a double put back through the Moon's
 *      velocity, within 0.01 mas.
 * Moshier is held to 0.1 mas geocentric and 1 mas heliocentric: it does not
 * re-read its series at t - tau but extrapolates on finite-difference
 * velocities (the Earth's a backward difference over 8.64 s, the Moon's
 * taken 4.32 s ahead), which leaves 0.04 mas over the Moon's 1.3 s of light
 * time and 0.5 mas over the 500 s heliocentric one. Its Moon is 766 mas
 * from the Swiss files on average over 1800-2100 (worst 2.3"), so that is
 * kept rather than doubling the cost of every Moshier Moon.
 * The upstream build fails check 2 (UPSTREAM-BUGS 23): 1.18 mas geocentric
 * on the files, 1.12 on Moshier. Run as
 *   moonlt EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static double Sep(const double *a, const double *b)   /* arcsec */
{
  double ra = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
  double rb = sqrt(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]);
  double cx = a[1]*b[2] - a[2]*b[1], cy = a[2]*b[0] - a[0]*b[2];
  double cz = a[0]*b[1] - a[1]*b[0];
  double dot = (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]) / (ra * rb);
  return atan2(sqrt(cx*cx + cy*cy + cz*cz) / (ra * rb), dot) * 206264.806247;
}

int main(int argc, char **argv)
{
  static const int32 rgEphe[3] = {SEFLG_MOSEPH, SEFLG_SWIEPH, SEFLG_JPLEPH};
  static const char *rgszEphe[3] = {"Moshier", "Swiss", "JPL"};
  static const int32 rgCtr[2] = {0, SEFLG_HELCTR};
  static const char *rgszCtr[2] = {"geo", "helio"};
  const double c = 173.1446326846693;   /* AU per day */
  char serr[256];
  int ie, ic, id, it, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: moonlt EPHEDIR\n");
    return 2;
  }
  printf("G33: the Moon's light time solves its equation, independent of SEFLG_SPEED\n");
  for (ie = 0; ie < 3; ie++) {
    swe_ctx *ctx = swe_ctx_new();
    int32 base = rgEphe[ie] | SEFLG_J2000 | SEFLG_ICRS | SEFLG_NONUT | SEFLG_XYZ;
    int32 astr = base | SEFLG_NOABERR | SEFLG_NOGDEFL;
    /* Moshier serves no barycentric positions: its barycentre IS the Sun,
     * so its geometric frame is heliocentric and the Sun sits at 0 */
    int32 moshier = rgEphe[ie] == SEFLG_MOSEPH;
    int32 geom = base | (moshier ? SEFLG_HELCTR : SEFLG_BARYCTR) | SEFLG_TRUEPOS | SEFLG_SPEED;
    swe_set_ephe_path_r(ctx, argv[1]);
    if (rgEphe[ie] == SEFLG_JPLEPH)
      swe_set_jpl_file_r(ctx, "de200.eph");
    for (ic = 0; ic < 2; ic++) {
      double wSpeed = 0, wDef = 0;
      for (id = 0; id < 40; id++) {
        double jd = 2378496.5 + id * 2739.325 + id * 0.0371;   /* 1800-2100 */
        double xs[6], xn[6], xo[6], xb[6], d[3], tau, tr, e, r, s;
        int32 iflag = astr | rgCtr[ic];
        if (swe_calc_r(ctx, jd, SE_MOON, iflag | SEFLG_SPEED, xs, serr) < 0 ||
            swe_calc_r(ctx, jd, SE_MOON, iflag, xn, serr) < 0 ||
            (rgCtr[ic] && moshier ? (memset(xo, 0, sizeof(xo)), 0)
             : swe_calc_r(ctx, jd, rgCtr[ic] ? SE_SUN : SE_EARTH, geom, xo, serr)) < 0) {
          printf("  %-7s %-5s ERROR %s\n", rgszEphe[ie], rgszCtr[ic], serr);
          bad++;
          break;
        }
        /* tau solved on this library's own geometric barycentric Moon; the
         * instant read, tr, is t - tau rounded to a double, and the Moon's
         * velocity carries it the rest of the way */
        tau = 0.0;
        for (it = 0; it < 20; it++) {
          tr = jd - tau;
          if (swe_calc_r(ctx, tr, SE_MOON, geom, xb, serr) < 0) {
            printf("  %-7s %-5s ERROR %s\n", rgszEphe[ie], rgszCtr[ic], serr);
            return 1;
          }
          e = (jd - tr) - tau;
          d[0] = xb[0] + xb[3] * e - xo[0];
          d[1] = xb[1] + xb[4] * e - xo[1];
          d[2] = xb[2] + xb[5] * e - xo[2];
          r = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
          if (fabs(r / c - tau) < 1e-16) break;
          tau = r / c;
        }
        s = Sep(xs, xn);
        if (s > wSpeed) wSpeed = s;
        s = Sep(xs, d);
        if (s > wDef) wDef = s;
      }
      printf("  %-7s %-5s speed flag moves it %.6f mas   from M(t-tau)-O(t) %.6f mas\n",
             rgszEphe[ie], rgszCtr[ic], wSpeed * 1e3, wDef * 1e3);
      if (wSpeed > 1e-6) {
        printf("    FAIL: the position depends on SEFLG_SPEED\n");
        bad++;
      }
      if (wDef > (moshier ? (rgCtr[ic] ? 1e-3 : 1e-4) : 1e-5)) {
        printf("    FAIL: the light time is not the solution of its equation\n");
        bad++;
      }
    }
    swe_ctx_free(ctx);
  }
  printf(bad ? "G33 FAIL: %d\n" : "G33 PASS\n", bad);
  return bad ? 1 : 0;
}
