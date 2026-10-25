#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#define D2R (M_PI/180)
static void eq_of(double lon, double lat, double eps, double *v)
{
  double x[3] = { lon, lat, 1 };
  swe_cotrans(x, x, -eps);
  v[0] = cos(x[1]*D2R)*cos(x[0]*D2R); v[1] = cos(x[1]*D2R)*sin(x[0]*D2R); v[2] = sin(x[1]*D2R);
}
static void cross(const double *a, const double *b, double *x)
{ x[0]=a[1]*b[2]-a[2]*b[1]; x[1]=a[2]*b[0]-a[0]*b[2]; x[2]=a[0]*b[1]-a[1]*b[0]; }
static double dot(const double *a, const double *b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
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
  swe_ctx *c = swe_ctx_new();
  int il, it, ip;
  char serr[256];
  swe_set_ephe_path_r(c, argv[1]);
  for (il = -89; il <= 89; il++) {
    double w = 0, lat = il == 0 ? 0 : il;
    for (it = 0; it < 40; it++) {
      double tu = 2451545.0 + it * 9.13, cu[13], am[10], x[6], A[3], Z[3], f = lat*D2R, ar;
      swe_houses_ex2_r(c, tu, SEFLG_SWIEPH, lat, 8.55, 'U', cu, am, NULL, NULL, serr);
      swe_calc_ut_r(c, tu, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
      ar = am[2]*D2R;
      Z[0] = cos(f)*cos(ar); Z[1] = cos(f)*sin(ar); Z[2] = sin(f);
      eq_of(am[0], 0, x[0], A);
      for (ip = 0; ip < 48; ip++) {
        double xp[2] = { ip * 7.5 + 0.3, (ip % 7 - 3) * 2.0 }, X[3], hp, kp, d;
        eq_of(xp[0], xp[1], x[0], X);
        kp = kpos(A, Z, X);
        hp = swe_house_pos_r(c, am[2], lat, x[0], 'U', xp, serr) - 1e-3/3600/30;
        d = fabs(hp - kp); if (d > 6) d = 12 - d;
        w = fmax(w, d);
      }
    }
    if (abs(il) < 3 || abs(il) > 60 || il % 10 == 0) printf("%3d: %.3e\n", il, w);
  }
  return 0;
}
