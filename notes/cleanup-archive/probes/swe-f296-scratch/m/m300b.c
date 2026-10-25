#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#define D2R (M_PI/180)
static const double TU[] = { 2378496.5, 2433282.5, 2451545.0, 2488069.5 };
static void eq_of(double lon, double lat, double eps, double *v)
{
  double x[3] = { lon, lat, 1 };
  swe_cotrans(x, x, -eps);
  v[0] = cos(x[1]*D2R)*cos(x[0]*D2R); v[1] = cos(x[1]*D2R)*sin(x[0]*D2R); v[2] = sin(x[1]*D2R);
}
static void cross(const double *a, const double *b, double *x)
{ x[0]=a[1]*b[2]-a[2]*b[1]; x[1]=a[2]*b[0]-a[0]*b[2]; x[2]=a[0]*b[1]-a[1]*b[0]; }
static double dot(const double *a, const double *b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
/* the Krusinski position of X: its hour circle's cut with the Asc-zenith circle */
static double kpos(const double *A, const double *Z, const double *X)
{
  double n[3], P[3] = { 0, 0, 1 }, m[3], d[3], th;
  cross(A, Z, n); cross(P, X, m); cross(n, m, d);
  if (d[0]*X[0] + d[1]*X[1] < 0) { d[0]=-d[0]; d[1]=-d[1]; d[2]=-d[2]; }
  th = atan2(-dot(d, Z), dot(d, A)) / D2R;
  if (th < 0) th += 360;
  return th / 30 + 1;
}
int main(int argc, char **argv)
{
  static const double LT[] = { 0, 1e-12, 1e-9, 1e-6, 47.37, -33.87, 60 };
  swe_ctx *c = swe_ctx_new();
  int l, it, k;
  char serr[256];
  swe_set_ephe_path_r(c, argv[1]);
  for (l = 0; l < 7; l++) {
    double wc = 0, wp = 0, wr = 0;
    for (it = 0; it < 4; it++) {
      double cu[13], am[10], x[6], A[3], Z[3], f = LT[l]*D2R, ar;
      swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LT[l], 8.55, 'U', cu, am, NULL, NULL, serr);
      swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
      ar = am[2]*D2R;
      Z[0] = cos(f)*cos(ar); Z[1] = cos(f)*sin(ar); Z[2] = sin(f);
      eq_of(am[0], 0, x[0], A);
      for (k = 1; k <= 12; k++) {
        double X[3], kp, hp, d, xp[2];
        eq_of(cu[k], 0, x[0], X);
        kp = kpos(A, Z, X);
        d = kp - k; if (d > 6) d -= 12; if (d < -6) d += 12;
        wc = fmax(wc, fabs(d));
        /* a place in the middle of house k */
        xp[0] = swe_degnorm(cu[k] + 0.5 * swe_degnorm(cu[k % 12 + 1] - cu[k])); xp[1] = 2.5;
        eq_of(xp[0], xp[1], x[0], X);
        kp = kpos(A, Z, X);
        hp = swe_house_pos_r(c, am[2], LT[l], x[0], 'U', xp, serr) - 1e-3/3600/30;
        d = hp - kp; if (d > 6) d -= 12; if (d < -6) d += 12;
        wp = fmax(wp, fabs(d));
        xp[0] = cu[k]; xp[1] = 1e-300;
        hp = swe_house_pos_r(c, am[2], LT[l], x[0], 'U', xp, serr) - 1e-3/3600/30;
        d = hp - k; if (d > 6) d -= 12; if (d < -6) d += 12;
        wr = fmax(wr, fabs(d));
      }
    }
    printf("lat %g: CalcH cusps off the construction %.3e house; house_pos of a place at b 2.5 off it %.3e; own cusps back %.3e\n", LT[l], wc, wp, wr);
  }
  return 0;
}
