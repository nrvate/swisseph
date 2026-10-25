/* probe: measure items 290, 297, 298 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "swephexp.h"

#define D2R (M_PI / 180)
static double dn(double x) { return swe_degnorm(x); }
static double dd(double a, double b) { return swe_difdeg2n(a, b); }
static void vec(double lon, double lat, double v[3]) {
  v[0] = cos(lat * D2R) * cos(lon * D2R); v[1] = cos(lat * D2R) * sin(lon * D2R); v[2] = sin(lat * D2R);
}
static void cross(const double a[3], const double b[3], double c[3]) {
  c[0] = a[1] * b[2] - a[2] * b[1]; c[1] = a[2] * b[0] - a[0] * b[2]; c[2] = a[0] * b[1] - a[1] * b[0];
}
static double dot(const double a[3], const double b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static void unit(double a[3]) { double r = sqrt(dot(a, a)); a[0] /= r; a[1] /= r; a[2] /= r; }
/* equatorial -> ecliptic longitude of unit vector v (equatorial of date), obliquity eps */
static double ecl_lon(const double v[3], double eps) {
  double e = eps * D2R;
  double y = v[1] * cos(e) + v[2] * sin(e);
  return dn(atan2(y, v[0]) / D2R);
}
/* ecliptic pole in equatorial coordinates */
static void ecl_pole(double eps, double k[3]) { k[0] = 0; k[1] = -sin(eps * D2R); k[2] = cos(eps * D2R); }

/* horizon frame in equatorial coords: zenith, north point, east point */
static void horframe(double armc, double lat, double Z[3], double N[3], double E[3]) {
  vec(armc, lat, Z);
  /* north point: on meridian, 90 deg from zenith toward pole */
  if (lat >= 0) vec(armc + 180, 90 - lat, N); else vec(armc + 180, 90 - lat, N);
  /* N = (armc+180, 90-lat) for any lat (for lat<0 dec 90-lat>90 -> means armc, 180-(90-lat)) */
  vec(armc + 90, 0, E);
}

/* Sunshine circles (Treindl/Makransky doc): cusp ih */
static double sunshine_vec(double armc, double lat, double eps, double dec, int ih) {
  double Z[3], N[3], E[3], HP[3], n[3], k[3], X[3], u[3];
  double arg = tan(dec * D2R) * tan(lat * D2R), ad, dsa, nsa, ha;
  if (arg >= 1) arg = 1; if (arg <= -1) arg = -1;
  ad = asin(arg) / D2R; dsa = 90 + ad; nsa = 90 - ad;
  switch (ih) {
    case 11: ha = -dsa / 3; break;
    case 12: ha = -2 * dsa / 3; break;
    case 2: ha = -180 + 2 * nsa / 3; break;
    case 3: ha = -180 + nsa / 3; break;
    case 5: ha = 180 - nsa / 3; break;
    case 6: ha = 180 - 2 * nsa / 3; break;
    case 8: ha = 2 * dsa / 3; break;
    case 9: ha = dsa / 3; break;
    default: return -1;
  }
  horframe(armc, lat, Z, N, E);
  vec(armc - ha, dec, HP);
  /* u: HP's component perpendicular to N */
  { double c = dot(HP, N); u[0] = HP[0] - c * N[0]; u[1] = HP[1] - c * N[1]; u[2] = HP[2] - c * N[2]; unit(u); }
  cross(N, u, n);
  ecl_pole(eps, k);
  cross(n, k, X); unit(X);
  if (dot(X, u) < 0) { X[0] = -X[0]; X[1] = -X[1]; X[2] = -X[2]; }
  return ecl_lon(X, eps);
}

/* horizon system: ecliptic point on the vertical half at azimuth A (from N through E) */
static double vert_vec(double armc, double lat, double eps, double A) {
  double Z[3], N[3], E[3], d[3], n[3], k[3], X[3];
  int i;
  horframe(armc, lat, Z, N, E);
  for (i = 0; i < 3; i++) d[i] = cos(A * D2R) * N[i] + sin(A * D2R) * E[i];
  cross(Z, d, n);
  ecl_pole(eps, k);
  cross(n, k, X); unit(X);
  if (dot(X, d) < 0) { X[0] = -X[0]; X[1] = -X[1]; X[2] = -X[2]; }
  return ecl_lon(X, eps);
}

int main(int argc, char **argv) {
  char serr[256];
  double cu[37], am[10], x[6], xs[6];
  int i, it, l;
  swe_set_ephe_path(argc > 1 ? argv[1] : "/nvm/work/swe-f290/ephe");
  /* ---- 290: Makransky vs Treindl vs vector at 60N / 60S, Zurich lon, UT 2433282.5 */
  {
    static const double LT[] = {60, -60};
    for (l = 0; l < 2; l++) {
      double ci[37], cI[37], ami[10], amI[10];
      double tu = 2433282.5;
      swe_calc_ut(tu, SE_ECL_NUT, 0, x, serr);
      swe_calc_ut(tu, SE_SUN, SEFLG_EQUATORIAL, xs, serr);
      swe_houses_ex2(tu, 0, LT[l], 8.55, 'i', ci, ami, NULL, NULL, serr);
      swe_houses_ex2(tu, 0, LT[l], 8.55, 'I', cI, amI, NULL, NULL, serr);
      printf("290 lat %.0f: armc %.4f dec %.4f\n", LT[l], amI[2], amI[9]);
      for (i = 1; i <= 12; i++) {
        double v = sunshine_vec(amI[2], LT[l], x[0], amI[9], i);
        printf("  %2d i %10.5f I %10.5f vec %10.5f  i-vec %10.2e I-vec %10.2e\n", i, ci[i], cI[i], v,
               v < 0 ? 0 : dd(ci[i], v), v < 0 ? 0 : dd(cI[i], v));
      }
    }
  }
  /* 290: count dates where i != I at 62, 64, 66 N, and max diff vs vector */
  {
    static const double LT[] = {58, 60, 62, 64, 66, -60, -62, -66};
    for (l = 0; l < 8; l++) {
      int ndiff = 0; double wI = 0, wi = 0, wiI = 0;
      for (it = 0; it < 400; it++) {
        double tu = 2451545.0 + it * 0.91;
        double ci[37], cI[37], ami[10], amI[10];
        swe_calc_ut(tu, SE_ECL_NUT, 0, x, serr);
        if (swe_houses_ex2(tu, 0, LT[l], 8.55, 'i', ci, ami, NULL, NULL, serr) == ERR) continue;
        swe_houses_ex2(tu, 0, LT[l], 8.55, 'I', cI, amI, NULL, NULL, serr);
        int dif = 0;
        for (i = 1; i <= 12; i++) {
          double v = sunshine_vec(amI[2], LT[l], x[0], amI[9], i);
          if (fabs(dd(ci[i], cI[i])) > 1e-9) dif = 1;
          wiI = fmax(wiI, fabs(dd(ci[i], cI[i])));
          if (v >= 0) { wI = fmax(wI, fabs(dd(cI[i], v))); wi = fmax(wi, fabs(dd(ci[i], v))); }
        }
        ndiff += dif;
      }
      printf("290 lat %5.1f: %d of 400 dates i != I (max %.3e), I-vec %.3e, i-vec %.3e\n", LT[l], ndiff, wiI, wI, wi);
    }
  }
  /* ---- 297: equator, UT 2451545.0, 'H' cusp 1 + 1e-6 */
  {
    double tu = 2451545.0, xp[2], hp;
    swe_calc_ut(tu, SE_ECL_NUT, 0, x, serr);
    swe_houses_ex2(tu, 0, 0, 8.55, 'H', cu, am, NULL, NULL, serr);
    printf("297 equator armc %.4f mc %.4f vertex %.4f\n", am[2], am[1], am[3]);
    for (i = 1; i <= 12; i++) {
      double span = dn(cu[i % 12 + 1] - cu[i]);
      xp[0] = dn(cu[i] + 1e-6 * span); xp[1] = 0;
      hp = swe_house_pos(am[2], 0, x[0], 'H', xp, serr);
      /* vector: azimuth division SH/equator convention, A = 90 + 30 (k-1) ; NH: 90 - 30 (k-1) */
      printf("  %2d %10.5f span %8.3f hpos(+1e-6) %10.6f  vecSH %10.5f vecNH %10.5f\n", i, cu[i], span, hp,
             vert_vec(am[2], 0, x[0], 90 + 30 * (i - 1)), vert_vec(am[2], 0, x[0], 90 - 30 * (i - 1)));
    }
  }
  /* ---- 298: Sunshine polar cusps with lat 1e-300 */
  {
    static const double LT[] = {68, 70, 75, -70, 60, -60, 47.37, 0};
    for (l = 0; l < 8; l++) {
      int nbad = 0, ntot = 0, nbadi = 0, ntoti = 0;
      for (it = 0; it < 400; it++) {
        double tu = 2451545.0 + it * 0.91, xp[2], hp;
        swe_calc_ut(tu, SE_ECL_NUT, 0, x, serr);
        swe_houses_ex2(tu, 0, LT[l], 8.55, 'I', cu, am, NULL, NULL, serr);
        for (i = 1; i <= 12; i++) {
          xp[0] = cu[i]; xp[1] = 1e-300;
          hp = swe_house_pos(am[2], LT[l], x[0], 'I', xp, serr);
          ntot++;
          if (fabs(remainder(hp - i, 12)) > 1e-7) nbad++;
        }
        if (swe_houses_ex2(tu, 0, LT[l], 8.55, 'i', cu, am, NULL, NULL, serr) == ERR) continue;
        for (i = 1; i <= 12; i++) {
          xp[0] = cu[i]; xp[1] = 1e-300;
          hp = swe_house_pos(am[2], LT[l], x[0], 'i', xp, serr);
          ntoti++;
          if (fabs(remainder(hp - i, 12)) > 1e-7) nbadi++;
          xp[1] = 0;
          hp = swe_house_pos(am[2], LT[l], x[0], 'i', xp, serr);
        }
      }
      printf("298 lat %6.2f: 'I' %d of %d cusps off; 'i' %d of %d\n", LT[l], nbad, ntot, nbadi, ntoti);
    }
  }
  return 0;
}
