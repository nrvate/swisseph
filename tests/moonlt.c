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
 * Moshier is held to the same 0.01 mas: its Moon and Earth are carried to
 * t - tau to second order in their accelerations (UPSTREAM-BUGS 53), where
 * they were moved back along their velocities at t, 0.5 mas off over the
 * 500 s heliocentric light time (measured now 0.0004 mas, the Earth series'
 * own last bits between the two instants).
 * The upstream build fails check 2 (UPSTREAM-BUGS 23): 1.18 mas geocentric
 * on the files, 1.12 on Moshier. Against the Sun's motion over tau
 * (UPSTREAM-BUGS 102), which Moshier's reference includes since, the build
 * before that fix fails Moshier's check 2 by 10.3 mas (0.0004 after). Run as
 *   moonlt EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

/* Moshier's Sun's barycentric velocity and acceleration, from its own
 * heliocentric planets as point masses (bug 102; G101 has the same) */
static void sun_bary_mosh(swe_ctx *c, double jd, double *v, double *a)
{
  static const int ipl[9] = {SE_MERCURY, SE_VENUS, SE_EARTH, SE_MARS,
                             SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO};
  static const double rm[9] = {6023600, 408523.719, 328900.5, 3098703.59,
                               1047.348644, 3497.9018, 22902.98, 19412.26, 136566000};
  const double k2 = 1.32712440017987e+20 * 86400.0 * 86400.0
                    / (1.49597870700e11 * 1.49597870700e11 * 1.49597870700e11);
  const int32 fc = SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_XYZ
                   | SEFLG_TRUEPOS | SEFLG_SPEED;
  char serr[256];
  double x[6], m[6], ms = 1, r;
  int k, i;
  for (i = 0; i < 3; i++)
    v[i] = a[i] = 0;
  for (k = 0; k < 9; k++) {
    swe_calc_r(c, jd, ipl[k], fc | SEFLG_HELCTR, x, serr);
    if (ipl[k] == SE_EARTH) {	/* the Earth-Moon barycentre */
      swe_calc_r(c, jd, SE_MOON, fc, m, serr);
      for (i = 0; i < 6; i++)
        x[i] += m[i] / (81.30056907419062 + 1);
    }
    r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    ms += 1 / rm[k];
    for (i = 0; i < 3; i++) {
      v[i] += x[i + 3] / rm[k];
      a[i] += k2 * x[i] / (r * r * r) / rm[k];
    }
  }
  for (i = 0; i < 3; i++)
    v[i] = -v[i] / ms;
}

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
  int ie, ic, id, it, k, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: moonlt EPHEDIR\n");
    return 2;
  }
  printf("G33: the Moon's light time solves its equation, independent of SEFLG_SPEED\n");
  for (ie = 0; ie < 3; ie++) {
    swe_ctx *ctx = swe_ctx_new();
    int32 base = rgEphe[ie] | SEFLG_J2000 | SEFLG_ICRS | SEFLG_NONUT | SEFLG_XYZ;
    int32 astr = base | SEFLG_NOABERR | SEFLG_NOGDEFL;
    /* Moshier serves no barycentric positions: its geometric frame is
     * heliocentric, the Sun at 0, and the Sun's motion over tau is formed
     * from its own planets (bug 102) */
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
        double vs[3] = {0, 0, 0}, as[3] = {0, 0, 0};
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
        if (moshier)	/* the Sun moves over tau (bug 102) */
          sun_bary_mosh(ctx, jd, vs, as);
        for (it = 0; it < 20; it++) {
          tr = jd - tau;
          if (swe_calc_r(ctx, tr, SE_MOON, geom, xb, serr) < 0) {
            printf("  %-7s %-5s ERROR %s\n", rgszEphe[ie], rgszCtr[ic], serr);
            return 1;
          }
          e = (jd - tr) - tau;
          for (k = 0; k < 3; k++)
            d[k] = xb[k] + xb[k + 3] * e - tau * vs[k] + 0.5 * tau * tau * as[k] - xo[k];
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
      if (wDef > 1e-5) {
        printf("    FAIL: the light time is not the solution of its equation\n");
        bad++;
      }
    }
    swe_ctx_free(ctx);
  }
  printf(bad ? "G33 FAIL: %d\n" : "G33 PASS\n", bad);
  return bad ? 1 : 0;
}
