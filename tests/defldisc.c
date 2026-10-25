/* defldisc.c -- G73: near and behind the Sun's disc a body's speed is the
 * rate of its deflected position.
 *
 * swi_deflect_light() took the deflection's rate as its change over the
 * 5e-7 day before t, one-sided, so off by half that step times the
 * deflection's second derivative. Behind the disc the deflection,
 * 1.75" m(r)/r at r solar radii from the centre (m from meff()'s table),
 * turns about the centre at v/b, b the separation, and curves at ~D v^2 /
 * b^2: Isis-Transpluto's longitude speed was 1.8e-6 deg/day off at 0.005
 * deg from the centre, 4.3e-7 at 0.026, 2.6e-8 at 0.1 (UPSTREAM-BUGS 74).
 * Now the displacement's derivative, with meff()'s slope on r's segment.
 * The errors once reported here near the disc, 3.8e-4 deg/day for Isis
 * at 0.065 deg, 1.4e-4 for Vulcanus at 0.1, 9e-7 for Admetos and 4e-7 for
 * Uranus near the limb, are reference stencils over 2^-12 day straddling
 * a node of meff()'s piecewise-linear table or the limb: the position's
 * slope breaks there, and no central difference across it is a rate.
 *
 * The check, Swiss files, apparent geocentric of date: at the conjunctions
 * with the Sun of Isis-Transpluto, Vulcanus and Admetos (2025), Zeus
 * (1960), Uranus (2025), Venus (2016-06-06) and Mercury (2022-11-08), the
 * instants before and after at which the body is 0.005, 0.01, 0.02, 0.05,
 * 0.1, 0.2, 0.3, 0.5 and 1 deg from the Sun's centre, and 0.998 and 1.002
 * of its radius (the limb from each side), where it comes that close; each
 * on the 2^-15-day grid, moved by 2^-10 day while its stencil spans a node
 * of the table or the limb (r from the light-time corrected, unaberrated
 * geometry the deflection uses). The deflection's part of the longitude
 * and latitude speeds (the speed less the speed with SEFLG_NOGDEFL, so
 * that the other terms' rates are not at stake) lies within 5e-9 deg/day
 * of a five-point central difference of the deflection over 2^-14 day:
 * measured <= 1.1e-9. The build before the fix fails at 76 of the 248
 * speeds (1.9e-6 for Zeus and 1.8e-6 for Isis at 0.005 deg, 3.8e-8 for
 * Venus at 0.02 of the radius), the upstream build at 164 (1.4e-5, bug
 * 39's roughness too). Run as
 *   defldisc EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define TOL 5e-9
#define SUNR_RAD (959.63 / 3600 * M_PI / 180)

/* the separation from the Sun's centre (deg) and r, in solar radii, as the
 * deflection sees them: the light-time corrected body against the Sun */
static double sep(double t, int ipl, double *r)
{
  const int32 f = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_XYZ
                  | SEFLG_NOGDEFL | SEFLG_NOABERR;
  double x[6], y[6], c[3], rx, ry, s;
  char serr[256];
  swe_calc(t, ipl, f, x, serr);
  swe_calc(t, SE_SUN, f | SEFLG_TRUEPOS, y, serr);
  c[0] = x[1] * y[2] - x[2] * y[1];
  c[1] = x[2] * y[0] - x[0] * y[2];
  c[2] = x[0] * y[1] - x[1] * y[0];
  rx = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
  ry = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
  s = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]) / rx / ry;
  if (r) *r = s * ry / SUNR_RAD;
  return asin(s) * 180 / M_PI;
}

static double wrap(double d)
{
  if (d > 180) d -= 360;
  if (d < -180) d += 360;
  return d;
}

int main(int argc, char **argv)
{
  static const struct { double t; int ipl; const char *n; } cj[7] = {
    {2460914.79, SE_ISIS, "Isis"}, {2460883.48, SE_VULKANUS, "Vulcanus"},
    {2437172.60, SE_ZEUS, "Zeus"},
    {2460821.00, SE_ADMETOS, "Admetos"}, {2460813.48, SE_URANUS, "Uranus"},
    {2457546.41, SE_VENUS, "Venus"}, {2459892.22, SE_MERCURY, "Mercury"},
  };
  static const double sdeg[] = {0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.5, 1};
  const int32 fl0 = SEFLG_SWIEPH, fl = fl0 | SEFLG_SPEED;
  const double h = 1.0 / 16384, g = 1.0 / 32768;
  char serr[256];
  int i, k, side, bad = 0, n = 0;
  double worst = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: defldisc EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G73: near and behind the Sun's disc a body's speed is the rate of its position\n");
  for (i = 0; i < 7; i++) {
    double a = cj[i].t - 1, b = cj[i].t + 1, t0, smin, rmin, rlimb, wl = 0, wb = 0;
    /* the conjunction: the separation's minimum, golden section */
    for (k = 0; k < 80; k++) {
      double m1 = b - (b - a) * 0.618034, m2 = a + (b - a) * 0.618034;
      if (sep(m1, cj[i].ipl, NULL) < sep(m2, cj[i].ipl, NULL)) b = m2; else a = m1;
    }
    t0 = (a + b) / 2;
    smin = sep(t0, cj[i].ipl, &rmin);
    rlimb = smin / rmin;	/* deg per solar radius */
    for (k = 0; k < 9 + 2; k++) {
      double target = k < 9 ? sdeg[k] : rlimb * (k == 9 ? 0.998 : 1.002);
      if (target <= smin * 1.01)
        continue;
      for (side = -1; side <= 1; side += 2) {
        double lo = t0, hi = t0 + side * 20, t, x[6], y[6], p[4][6], q[4][6], r;
        int j, m, tries;
        for (m = 0; m < 70; m++) {
          double mid = (lo + hi) / 2;
          if (sep(mid, cj[i].ipl, NULL) < target) lo = mid; else hi = mid;
        }
        t = floor(lo / g) * g;
        for (tries = 0; tries < 8; tries++) {
          int node0 = -1, crosses = 0;
          for (j = -3; j <= 3; j++) {
            int node;
            sep(t + j * h, cj[i].ipl, &r);
            node = r >= 1 ? 100 : (int)floor(r * 100);
            if (j > -3 && node != node0) crosses = 1;
            node0 = node;
          }
          if (!crosses) break;
          t += side * 1.0 / 1024;
        }
        if (swe_calc(t, cj[i].ipl, fl, x, serr) < 0
            || swe_calc(t, cj[i].ipl, fl | SEFLG_NOGDEFL, y, serr) < 0) {
          printf("  %s ERROR %s\n", cj[i].n, serr);
          return 1;
        }
        for (m = 0; m < 4; m++) {
          double tm = t + (m < 2 ? m - 2 : m - 1) * h;
          swe_calc(tm, cj[i].ipl, fl0, p[m], serr);
          swe_calc(tm, cj[i].ipl, fl0 | SEFLG_NOGDEFL, q[m], serr);
        }
        sep(t, cj[i].ipl, &r);
        for (j = 0; j < 2; j++) {
          /* the deflection, position with it less position without */
          double d0 = wrap(x[j] - y[j]), dp[4], d, off;
          for (m = 0; m < 4; m++)
            dp[m] = wrap(p[m][j] - q[m][j]) - d0;
          d = (8 * (dp[2] - dp[1]) - (dp[3] - dp[0])) / (12 * h);
          off = fabs(x[3 + j] - y[3 + j] - d);
          if (off > (j ? wb : wl)) { if (j) wb = off; else wl = off; }
          if (off > worst) worst = off;
          n++;
          if (off > TOL) {
            printf("    FAIL: %s %s at %.5f deg (r %.4f, t %.8f), %s deflection's speed %.2e from its central difference\n",
                   cj[i].n, side < 0 ? "before" : "after", sep(t, cj[i].ipl, NULL), r, t,
                   j ? "latitude" : "longitude", off);
            bad++;
          }
        }
      }
    }
    printf("  %-8s closest %.4f deg (r %.3f): deflection's longitude speed within %.1e, latitude %.1e deg/day\n",
           cj[i].n, smin, rmin, wl, wb);
  }
  printf("  %d speeds, worst %.1e deg/day\n", n, worst);
  swe_close();
  printf(bad ? "G73 FAIL: %d\n" : "G73 PASS\n", bad);
  return bad ? 1 : 0;
}
