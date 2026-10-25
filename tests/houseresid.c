/* houseresid.c -- G67: three small defects left in swehouse.c by bugs
 * 40-43 and 59 (UPSTREAM-BUGS 68).
 *
 * 1. swe_houses_ex2_r() asked the Sun's declination (Sunshine houses) with
 *    no ephemeris flag, so a SEFLG_MOSEPH call got the Swiss ephemeris'
 *    Sun. Check: the declination in ascmc[9] of a MOSEPH call is
 *    swe_calc_ut_r()'s Moshier Sun to 1e-12 deg, and differs from the
 *    Swiss one by 6.0e-6 deg (the cusps by up to 3.8e-6). The upstream
 *    build returns the Swiss declination for both.
 * 2. Gauquelin houses (36 cusps) that fell back to Porphyry (within the
 *    polar circle, retc < 0) have 12 cusps; cusps 13..36 and their speeds
 *    stayed as the caller had them and the RADIANS conversion multiplied
 *    them. Check: arrays prefilled with 7 come back 0 in degrees and in
 *    radians, 24 cusps and 24 speeds each. The upstream build returns 7 in
 *    degrees and 0.1222 (7 pi/180) in radians.
 * 3. A Placidus or Gauquelin cusp whose first-guess declination is zero
 *    (|tan dec| < 1e-10, the cusp at an equinox point) had its speed set
 *    to ARMCS, the ARMC's rate; the cusp moves at the Asc1 rate A_x at the
 *    pole height m tan(fi). Check: at the ARMC that puts cusps 11, 12, 2, 3
 *    (Placidus) and sectors 9 and 29 (Gauquelin) there, at 47N and 60N, the
 *    speed against a five-point difference of the cusps over 2^-8 and 2^-9
 *    degree of ARMC, Richardson-combined, limit 1e-7 deg/day + 1e-10 of the
 *    rate. Measured over 12 cusps: worst 6.5e-8 deg/day (the
 *    speeds are 415..788 deg/day); the upstream build returns 360.99 for
 *    all of them (up to 427 off).
 * The upstream build fails all three. Run as
 *   houseresid EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define BAD(...) do { printf("  FAIL: " __VA_ARGS__); printf("\n"); bad++; } while (0)

static double dif(double a, double b)
{
  double d = fmod(a - b, 360);
  if (d > 180) d -= 360;
  else if (d <= -180) d += 360;
  return d;
}

int main(int argc, char **argv)
{
  char serr[256];
  int bad = 0, i;
  swe_ctx *ctx;

  if (argc != 2) {
    fprintf(stderr, "usage: houseresid EPHEDIR\n");
    return 2;
  }
  ctx = swe_ctx_new();
  swe_set_ephe_path_r(ctx, argv[1]);
  printf("G67: house residue (UPSTREAM-BUGS 68)\n");

  /* 1: the Sun's ephemeris for Sunshine houses */
  {
    double t = 2461223.5 + 0.3125, c1[37], c2[37], a1[10], a2[10], xs[6], xm[6], w = 0;
    swe_calc_ut_r(ctx, t, SE_SUN, SEFLG_SPEED | SEFLG_EQUATORIAL, xs, serr);
    swe_calc_ut_r(ctx, t, SE_SUN, SEFLG_SPEED | SEFLG_EQUATORIAL | SEFLG_MOSEPH, xm, serr);
    swe_houses_ex2_r(ctx, t, 0, 47, 8.55, 'I', c1, a1, NULL, NULL, serr);
    swe_houses_ex2_r(ctx, t, SEFLG_MOSEPH, 47, 8.55, 'I', c2, a2, NULL, NULL, serr);
    for (i = 1; i <= 12; i++) if (fabs(dif(c1[i], c2[i])) > w) w = fabs(dif(c1[i], c2[i]));
    printf("  Sunshine: Swiss dec %.9f, Moshier dec %.9f; ascmc[9] default %.9f, MOSEPH %.9f; cusps differ %.2e\n",
           xs[1], xm[1], a1[9], a2[9], w);
    if (fabs(a1[9] - xs[1]) > 1e-12) BAD("default call's declination is not the Swiss Sun's");
    if (fabs(a2[9] - xm[1]) > 1e-12) BAD("MOSEPH call's declination %.9f is not Moshier's %.9f", a2[9], xm[1]);
    if (!(fabs(xs[1] - xm[1]) > 1e-6)) BAD("the two Suns are not apart: the check proves nothing");
  }

  /* 2: Gauquelin fallen back to Porphyry */
  {
    double t = 2461223.5 + 0.3125, c[37], a[10], cs[37], as[10], cr[37], ar[10], csr[37], asr[10];
    int r, k, nz = 0, nz2 = 0;
    for (k = 0; k < 37; k++) c[k] = cs[k] = cr[k] = csr[k] = 7;
    r = swe_houses_ex2_r(ctx, t, 0, 80, 8.55, 'G', c, a, cs, as, serr);
    if (r >= 0) BAD("Gauquelin at 80N did not fall back (retc %d)", r);
    r = swe_houses_ex2_r(ctx, t, SEFLG_RADIANS, 80, 8.55, 'G', cr, ar, csr, asr, serr);
    for (k = 13; k <= 36; k++)
      if (c[k] != 0 || cs[k] != 0 || cr[k] != 0 || csr[k] != 0) nz++;
    {
      /* swe_houses_r() and swe_houses_armc_ex2_r() the same way */
      double cr2[37], cc[37], ac[10], ss[37], sa[10];
      for (k = 0; k < 37; k++) cr2[k] = cc[k] = ss[k] = 7;
      swe_houses_r(ctx, t, 80, 8.55, 'G', cr2, ac);
      swe_houses_armc_ex2_r(ctx, 100, 80, 23.44, 'G', cc, ac, ss, sa, serr);
      for (k = 13; k <= 36; k++)
        if (cr2[k] != 0 || cc[k] != 0 || ss[k] != 0) nz2++;
      printf("  swe_houses_r / swe_houses_armc_ex2_r: %d of 72 not 0\n", nz2);
      if (nz2) BAD("%d of cusps/speeds 13..36 left unset by swe_houses_r/armc_ex2_r", nz2);
    }
    printf("  Gauquelin at 80N: retc %d, cusp 13 %g (rad %g), speed 13 %g (rad %g), %d of 96 not 0\n",
           r, c[13], cr[13], cs[13], csr[13], nz);
    if (nz) BAD("%d of cusps/speeds 13..36 left unset after the Porphyry fallback", nz);
  }

  /* 3: the cusp on an equinox point */
  {
    static const struct { int hs, ih; double armc; } cs3[6] = {
      {'P', 11, 330}, {'P', 12, 300}, {'P', 2, 240}, {'P', 3, 210}, {'G', 9, 350}, {'G', 29, 190}};
    static const double lats[2] = {47, 60};
    double worst = 0, eps = 23.4392911;
    int k, b;
    for (k = 0; k < 6; k++) for (b = 0; b < 2; b++) {
      static const int off[4] = {-2, -1, 1, 2};
      double h[2] = {1.0 / 256, 1.0 / 512}, w[2][4][37], c[37], a[10], cs[37], as[10], d[2], ref, err;
      int s, j, ih = cs3[k].ih;
      swe_houses_armc_ex2_r(ctx, cs3[k].armc, lats[b], eps, cs3[k].hs, c, a, cs, as, serr);
      for (s = 0; s < 2; s++) for (j = 0; j < 4; j++)
        swe_houses_armc_ex2_r(ctx, cs3[k].armc + off[j] * h[s], lats[b], eps, cs3[k].hs, w[s][j], a, NULL, NULL, serr);
      for (s = 0; s < 2; s++) {
        double a1 = dif(w[s][2][ih], c[ih]) - dif(w[s][1][ih], c[ih]);
        double a2 = dif(w[s][3][ih], c[ih]) - dif(w[s][0][ih], c[ih]);
        d[s] = (8 * a1 - a2) / (12 * h[s]);
      }
      ref = (d[1] + (d[1] - d[0]) / 15) * 360.98564736629;	/* per day */
      err = fabs(cs[ih] - ref);
      if (err > worst) worst = err;
      printf("  %c cusp %d at %gN: cusp %.12f speed %.9f, difference %.9f\n", cs3[k].hs, ih, lats[b], c[ih], cs[ih], ref);
      if (err > 1e-7 + 1e-10 * fabs(ref))
        BAD("%c cusp %d at %gN: speed %.9f, difference %.9f", cs3[k].hs, ih, lats[b], cs[ih], ref);
    }
    printf("  equinox cusps: worst %.2e deg/day\n", worst);
  }
  swe_ctx_free(ctx);
  if (bad)
    printf("G67 FAIL: %d\n", bad);
  else
    printf("G67 PASS\n");
  return bad != 0;
}
