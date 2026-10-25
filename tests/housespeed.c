/* housespeed.c -- G45: the house cusp speeds of swe_houses_ex2() are the
 * rate of the cusps swe_houses_ex() returns, in every house system.
 *
 * The speeds were each cusp's rate in ARMC times ARMCS, the mean sidereal
 * day of a fixed solar year, and wrong four ways (UPSTREAM-BUGS 40-43):
 * whole-sign and 0-Aries cusps, which stand still, kept the Asc's and MC's
 * speeds (326-1270 deg/day), Porphyry's cusps 11 and 12 were built from
 * the Asc's (1600 deg/day at 70N), Krusinski's were never taken (0 for
 * 1500); Koch's left out the rate of the MC's ascensional difference (200
 * deg/day at 47N), Placidus' and Gauquelin's the rate of the pole height
 * (3.2 deg/day at 47N, 32 at 60N), and the systems differenced in ARMC
 * did it over one second by three points (1e-4 deg/day at 70N, Q 0.03);
 * the ARMC turned at ARMCS without the equation of the equinoxes' rate
 * (up to 6e-5 deg/day, times the cusp's own rate in ARMC: R 70N 4.8e-4), the
 * obliquity and the Sun's declination of Sunshine houses stood still
 * (2.4 deg/day at 70S); and sidereal speeds were the tropical ones,
 * without the ayanamsa's rate (4.7e-5) or the auxiliary frame's.
 *
 * The check: every system the library has, at 0, 47N, 47S and 70N (where
 * Koch, Placidus and Gauquelin refuse, skipped), 1800, 2026.5 and 2300 at
 * three times of day, tropical, Lahiri and J2000 on the ecliptic of t0
 * (SE_SIDBIT_ECL_T0): each of the 12 (Gauquelin 36) cusp speeds and the 8
 * ascmc speeds against a five-point central difference over 2^-12 day of
 * swe_houses_ex()'s positions, at instants that are multiples of 2^-8 day
 * so t + k h is exact, and over 2^-13 day, the two Richardson-combined.
 * Cases whose positions jump within the stencil (a cusp at a sign
 * boundary, the Asc changing hemisphere within the polar circle), fall
 * back to Porphyry anywhere on it, or turn a corner in it (the two steps
 * disagree by 1e-4: Pullen's quadrant branches) are skipped. The limit is
 * 1e-6 deg/day + 3e-8 of the rate: Placidus and Gauquelin cusps converge
 * only to 3.5e-8 deg, and their differences carry the rate of that error,
 * 2e-8 of the speed at 60N (the speed is the converged cusp's, as
 * tightening the iteration shows). Measured over 56104 speeds: worst
 * 5.8e-6 deg/day, APC at 70N (5.7e-9 of its 5000 deg/day), every other
 * system below 9.6e-7. The upstream build fails every system: whole signs
 * and 0 Aries (674 deg/day), Porphyry (926), Krusinski (1070), Koch
 * (195), Placidus and Gauquelin (3.0), Sunshine (0.32), the rest by the
 * ARMC's and obliquity's rates (5e-5..4.8e-4), sidereal by the
 * ayanamsa's (8.5e-5). Run as
 *   housespeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define NF 44	/* 36 cusps + 8 ascmc */

static int eval(swe_ctx *ctx, double t, int32 fl, double lat, int hs, double *v, double *sp)
{
  double cusp[37], ascmc[10], cs[37], as[10];
  char serr[256] = "";
  int i, nc = hs == 'G' ? 36 : 12, r;
  memset(cusp, 0, sizeof cusp);
  memset(ascmc, 0, sizeof ascmc);
  if (sp != NULL)
    r = swe_houses_ex2_r(ctx, t, fl, lat, 8.55, hs, cusp, ascmc, cs, as, serr);
  else
    r = swe_houses_ex_r(ctx, t, fl, lat, 8.55, hs, cusp, ascmc);
  for (i = 0; i < nc; i++) v[i] = cusp[i + 1];
  for (i = 0; i < 8; i++) v[nc + i] = ascmc[i];
  if (sp != NULL) {
    for (i = 0; i < nc; i++) sp[i] = cs[i + 1];
    for (i = 0; i < 8; i++) sp[nc + i] = as[i];
  }
  return r;
}

static double dif(double a, double b)
{
  double d = fmod(a - b, 360);
  if (d > 180) d -= 360;
  else if (d <= -180) d += 360;
  return d;
}

int main(int argc, char **argv)
{
  static const char *sys = "ABCDEFGHIiJKLMNOPQRSTUVWXY";
  static const double lats[] = {0, 47, -47, 70};
  static const double years[] = {1800, 2026.5, 2300};
  static const struct { const char *n; int32 fl; int32 sidm; } modes[] = {
    {"tropical", 0, 0},
    {"Lahiri", SEFLG_SIDEREAL, SE_SIDM_LAHIRI},
    {"ecl_t0", SEFLG_SIDEREAL, SE_SIDM_J2000 | SE_SIDBIT_ECL_T0},
  };
  const double h[2] = {1.0 / 4096, 1.0 / 8192};
  int mi, li, yi, k, f, bad = 0, ncmp = 0;
  const char *p;
  swe_ctx *ctx;

  if (argc != 2) {
    fprintf(stderr, "usage: housespeed EPHEDIR\n");
    return 2;
  }
  ctx = swe_ctx_new();
  swe_set_ephe_path_r(ctx, argv[1]);
  printf("G45: house cusp speeds are the rate of the cusps, every system\n");
  for (mi = 0; mi < 3; mi++) {
    if (modes[mi].fl & SEFLG_SIDEREAL)
      swe_set_sid_mode_r(ctx, modes[mi].sidm, 0, 0);
    for (p = sys; *p; p++) {
      int hs = *p, nfl = (hs == 'G' ? 36 : 12) + 8, nbad = 0;
      double worst = 0, wrel = 0, wref = 0;
      for (li = 0; li < 4; li++) {
        for (yi = 0; yi < 3; yi++) {
          for (k = 0; k < 3; k++) {
            double t = swe_julday((int) years[yi], 1, 1, 0, SE_GREG_CAL)
                       + (years[yi] - floor(years[yi])) * 365.25 + 0.1234567 + k * 0.29;
            double v0[NF], sp[NF], w[2][4][NF];
            int j, s, skip = 0;
            t = floor(t * 256) / 256;
            if (eval(ctx, t, modes[mi].fl, lats[li], hs, v0, sp) < 0)
              continue;
            for (s = 0; s < 2; s++) {
              for (j = 0; j < 4; j++) {
                static const int off[4] = {-2, -1, 1, 2};
                if (eval(ctx, t + off[j] * h[s], modes[mi].fl, lats[li], hs, w[s][j], NULL) < 0)
                  skip = 1;
              }
            }
            if (skip)
              continue;
            for (f = 0; f < nfl; f++) {
              double d[2], ref, err, lim;
              for (s = 0; s < 2; s++) {
                double a1 = dif(w[s][2][f], v0[f]) - dif(w[s][1][f], v0[f]);
                double a2 = dif(w[s][3][f], v0[f]) - dif(w[s][0][f], v0[f]);
                d[s] = (8 * a1 - a2) / (12 * h[s]);
              }
              if (fabs(dif(w[0][0][f], v0[f])) > 10 || fabs(dif(w[0][3][f], v0[f])) > 10)
                continue;	/* a jump within the stencil */
              if (fabs(d[1] - d[0]) > 1e-4 * fabs(d[1]) + 1e-6)
                continue;	/* a kink within it */
              ref = d[1] + (d[1] - d[0]) / 15;
              err = fabs(sp[f] - ref);
              lim = 1e-6 + 3e-8 * fabs(ref);
              ncmp++;
              if (err > worst) { worst = err; wref = ref; }
              if (fabs(ref) > 1 && err / fabs(ref) > wrel) wrel = err / fabs(ref);
              if (err > lim) {
                if (nbad < 2)
                  printf("    %s %c lat %g %g #%d field %d: %.9f, the cusps' rate %.9f\n",
                         modes[mi].n, hs, lats[li], years[yi], k, f, sp[f], ref);
                nbad++;
              }
            }
          }
        }
      }
      printf("  %-8s %c  worst %.2e deg/day (rate %.1f), relative %.1e%s\n", modes[mi].n,
             hs, worst, wref, wrel, nbad ? "  FAIL" : "");
      if (nbad) {
        printf("    %d speeds off\n", nbad);
        bad++;
      }
    }
  }
  swe_ctx_free(ctx);
  printf("  %d speeds compared\n", ncmp);
  printf(bad ? "G45 FAIL: %d\n" : "G45 PASS\n", bad);
  return bad ? 1 : 0;
}
