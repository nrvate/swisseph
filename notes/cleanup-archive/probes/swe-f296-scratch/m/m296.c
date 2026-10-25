/* 296: the solar system plane's sidereal houses against a vector construction */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"

#define LAT 47.37
#define LON 8.55
static void cross(const double *a, const double *b, double *x)
{
  x[0] = a[1] * b[2] - a[2] * b[1];
  x[1] = a[2] * b[0] - a[0] * b[2];
  x[2] = a[0] * b[1] - a[1] * b[0];
}
static double dot(const double *a, const double *b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }

/* columns: the solar system plane's axes (x the node on ecl J2000, z its pole) in the true equator of tjde */
static void ss_axes(swe_ctx *c, double tjde, double M[3][3])
{
  double e2 = swi_epsiln(c, J2000, 0), em = swi_epsiln(c, tjde, 0), nut[2], x[3], l, r;
  double Om = 107.582569 * DEGTORAD, in = 1.578701 * DEGTORAD;
  int j;
  swi_nutation(c, tjde, 0, nut);
  for (j = 0; j < 3; j++) {
    double v[3] = { j == 0, j == 1, j == 2 }, w[3];
    /* plane frame -> ecliptic J2000: about x by in, about z by Om */
    w[0] = v[0];
    w[1] = v[1] * cos(in) - v[2] * sin(in);
    w[2] = v[1] * sin(in) + v[2] * cos(in);
    x[0] = w[0] * cos(Om) - w[1] * sin(Om);
    x[1] = w[0] * sin(Om) + w[1] * cos(Om);
    x[2] = w[2];
    swi_coortrf(x, x, -e2);
    swi_precess(c, x, tjde, 0, J2000_TO_J);
    swi_coortrf(x, x, em);
    l = atan2(x[1], x[0]) + nut[0];
    r = hypot(x[0], x[1]);
    x[0] = r * cos(l);
    x[1] = r * sin(l);
    swi_coortrf(x, x, -(em + nut[1]));
    M[0][j] = x[0]; M[1][j] = x[1]; M[2][j] = x[2];
  }
}
/* the t0 vernal point's longitude on the plane */
static double vp_lon(swe_ctx *c, double t0)
{
  double e2 = swi_epsiln(c, J2000, 0), x[3] = { 1, 0, 0 }, X[3], Y[3];
  double Om = 107.582569 * DEGTORAD, in = 1.578701 * DEGTORAD;
  if (t0 != J2000)
    swi_precess(c, x, t0, 0, J_TO_J2000);
  swi_coortrf(x, x, e2);  /* to ecliptic J2000 */
  X[0] = cos(Om); X[1] = sin(Om); X[2] = 0;
  Y[0] = -sin(Om) * cos(in); Y[1] = cos(Om) * cos(in); Y[2] = sin(in);
  return atan2(dot(x, Y), dot(x, X)) * RADTODEG;
}
static double cut(double M[3][3], double armc, double lat, int hs, int k)
{
  double A = armc * DEGTORAD, f = lat * DEGTORAD, th = 30 * (k - 1) * DEGTORAD;
  double Z[3], hN[3], P[3], p[3], d[3], q[3], e[3], s;
  double n0[3] = { M[0][2], M[1][2], M[2][2] };
  int i;
  Z[0] = cos(f) * cos(A); Z[1] = cos(f) * sin(A); Z[2] = sin(f);
  hN[0] = -sin(f) * cos(A); hN[1] = -sin(f) * sin(A); hN[2] = cos(f);
  if (hs == 'R') {
    P[0] = cos(A + M_PI / 2 + th); P[1] = sin(A + M_PI / 2 + th); P[2] = 0;
  } else {
    double E[3] = { -sin(A), cos(A), 0 };
    for (i = 0; i < 3; i++) P[i] = cos(th) * E[i] - sin(th) * Z[i];
  }
  cross(hN, P, p);
  cross(n0, p, d);
  s = dot(P, hN);
  for (i = 0; i < 3; i++) q[i] = P[i] - s * hN[i];
  if (dot(d, q) < 0) for (i = 0; i < 3; i++) d[i] = -d[i];
  for (i = 0; i < 3; i++) e[i] = M[0][i] * d[0] + M[1][i] * d[1] + M[2][i] * d[2];
  return swe_degnorm(atan2(e[1], e[0]) * RADTODEG);
}
int main(int argc, char **argv)
{
  static const double YR[] = { 17000, 14000, 12000, 10000, 8000, 7500, 7200, 7000, 5000, 2000, 0, -2000, -5000, -5600, -5650, -5660, -5700, -5800, -6000, -8000, -10000, -12000, -13000 };
  int s, iy, k, hk;
  char serr[256];
  for (s = 0; s < 2; s++) {
    swe_ctx *c = swe_ctx_new(), *c0 = swe_ctx_new();
    int32 sid = s == 0 ? SE_SIDM_USER | SE_SIDBIT_SSY_PLANE : SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE;
    double x1[6], x0[6], corr, t0, a0, vl;
    swe_set_ephe_path_r(c, argv[1]);
    swe_set_ephe_path_r(c0, argv[1]);
    swe_set_sid_mode_r(c, sid, J2000, 23.86);
    swe_set_sid_mode_r(c0, sid | SE_SIDBIT_NO_PREC_OFFSET, J2000, 23.86);
    swe_calc_ut_r(c, 2451545.0, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL, x1, serr);
    swe_calc_ut_r(c0, 2451545.0, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL, x0, serr);
    corr = swe_difdeg2n(x1[0], x0[0]);
    t0 = c->sidd.t0; a0 = c->sidd.ayan_t0;
    vl = vp_lon(c, t0);
    printf("%s: t0 %.4f a0 %.6f corr %.3e vplon %.9f\n", s ? "Lahiri ssy" : "user ssy", t0, a0, corr, vl);
    for (iy = 0; iy < (int) (sizeof YR / sizeof YR[0]); iy++) {
      double tu = 2451545.0 + (YR[iy] - 2000) * 365.25;
      double tjde = tu + swe_deltat_ex_r(c, tu, SEFLG_SWIEPH, NULL), M[3][3], w = 0, wa = 0;
      ss_axes(c, tjde, M);
      for (hk = 0; hk < 2; hk++) {
        int hs = "RC"[hk];
        double cu[13], am[10];
        if (swe_houses_ex2_r(c, tu, SEFLG_SWIEPH | SEFLG_SIDEREAL, LAT, LON, hs, cu, am, NULL, NULL, serr) == ERR) {
          printf("ERR %s\n", serr); continue;
        }
        for (k = 1; k <= 12; k++) {
          double ref = swe_degnorm(cut(M, am[2], LAT, hs, k) - vl - a0 + corr);
          w = fmax(w, fabs(swe_difdeg2n(cu[k], ref)));
          if (k == 1) wa = fmax(wa, fabs(swe_difdeg2n(am[0], ref)));
        }
      }
      printf("  %7.0f: cusps off %.3e  asc off %.3e\n", YR[iy], w, wa);
    }
    swe_ctx_free(c); swe_ctx_free(c0);
  }
  return 0;
}
