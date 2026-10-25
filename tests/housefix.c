/* housefix.c -- G58: four small defects of swehouse.c (UPSTREAM-BUGS 59).
 *
 * 1. Placidus and Gauquelin cusps are the fixed point of C = Asc1(ra,
 *    f(C)). The iteration stopped at 1/360000 deg and left the cusp up to
 *    3.5e-8 deg off the fixed point (4.5e-6 at 65N, where it contracts
 *    slowly); differences of the returned cusps carried the error's
 *    rate, 1.1e-5 deg/day at 60N, against the exact rate swe_houses_ex2()
 *    returns. Check: the speeds of cusps 2, 3, 11, 12 (Gauquelin: all
 *    36) at 47N and 60N, three instants (multiples of 2^-8 day), against
 *    five-point differences of swe_houses_ex()'s cusps over 2^-12 and
 *    2^-13 day, Richardson-combined; limit 1e-7 deg/day + 1e-10 of the
 *    rate. Measured over 240 speeds: worst 3.1e-9 deg/day; 2.6e-6 with
 *    the old stopping rule alone (1.1e-5 at 60N in a wider sweep), 24
 *    in the upstream build (which also lacks bugs 41's rate).
 * 2. With SEFLG_RADIANS swe_houses_ex2() turned cusps and ascmc to radians
 *    and left their speeds in deg/day. Check: the speeds of a RADIANS
 *    call are the degree call's times pi/180 (Placidus at 47N, 12 cusps,
 *    8 ascmc), to 1e-12 of the rate. The upstream build returns them
 *    unchanged (57.3 times too big; relative difference 0.98).
 * 3. sidereal_houses_trad() applied toupper() to the house system, so
 *    sidereal Makransky Sunshine ('i') computed Treindl's ('I'). Check:
 *    at 60S, 2026-07 + 200 days, Lahiri, 'i' cusps are the tropical 'i'
 *    cusps less the ayanamsa (1e-8 deg) and not sidereal 'I''s (41 deg
 *    apart at worst). The upstream build returns 'I''s (41 deg off).
 * 4. Sunshine houses ('I', 'i') need the Sun's declination; with no Swiss
 *    files (and the ephemeris fallback off, as shipped) swe_calc_ut()
 *    refuses, and the houses came back as Porphyry with ERR. Check: a
 *    context with an empty ephemeris path returns OK from
 *    swe_houses_ex2_r(), swe_houses_ex_r() and swe_houses_r(), with the
 *    cusps of the Swiss-file call to 1e-4 deg (Moshier's Sun is 0.1"
 *    off) and the declination in ascmc[9]. The upstream build returns
 *    ERR and Porphyry (cusps 16.7 deg off).
 * The upstream build fails all four. Run as
 *   housefix EPHEDIR
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
  static const int hsP[2] = {'P', 'G'};
  static const double lats[2] = {47, 60};
  static const double ts[3] = {2461223.5 + 0.3125, 2451545.0 + 0.1875, 2470000.0 + 0.5};
  const double h[2] = {1.0 / 4096, 1.0 / 8192};
  double cusp[37], ascmc[10], cs[37], as[10], cr[37], ar[10], csr[37], asr[10];
  char serr[256];
  int bad = 0, i, a, b, c, ncmp = 0;
  double worst = 0;
  swe_ctx *ctx;

  if (argc != 2) {
    fprintf(stderr, "usage: housefix EPHEDIR\n");
    return 2;
  }
  ctx = swe_ctx_new();
  swe_set_ephe_path_r(ctx, argv[1]);
  printf("G58: four small house defects (UPSTREAM-BUGS 59)\n");

  /* 1: Placidus and Gauquelin speeds against differences of the cusps */
  for (a = 0; a < 2; a++) for (b = 0; b < 2; b++) for (c = 0; c < 3; c++) {
    int hs = hsP[a], nc = hs == 'G' ? 36 : 12;
    double t = floor(ts[c] * 256) / 256, w[2][4][37];
    static const int off[4] = {-2, -1, 1, 2};
    int s, j;
    memset(cusp, 0, sizeof cusp);
    if (swe_houses_ex2_r(ctx, t, 0, lats[b], 8.55, hs, cusp, ascmc, cs, as, serr) < 0)
      continue;
    for (s = 0; s < 2; s++)
      for (j = 0; j < 4; j++)
        swe_houses_ex_r(ctx, t + off[j] * h[s], 0, lats[b], 8.55, hs, w[s][j], ascmc);
    for (i = 1; i <= nc; i++) {
      double d[2], ref, err;
      if (nc == 12 && i != 2 && i != 3 && i != 11 && i != 12)
        continue;
      for (s = 0; s < 2; s++) {
        double a1 = dif(w[s][2][i], cusp[i]) - dif(w[s][1][i], cusp[i]);
        double a2 = dif(w[s][3][i], cusp[i]) - dif(w[s][0][i], cusp[i]);
        d[s] = (8 * a1 - a2) / (12 * h[s]);
      }
      if (fabs(d[1] - d[0]) > 1e-4 * fabs(d[1]) + 1e-6)
        continue;	/* a kink within the stencil */
      ref = d[1] + (d[1] - d[0]) / 15;
      err = fabs(cs[i] - ref);
      ncmp++;
      if (err > worst) worst = err;
      if (err > 1e-7 + 1e-10 * fabs(ref))
        BAD("%c cusp %d at %gN: speed %.9f, difference %.9f", hs, i, lats[b], cs[i], ref);
    }
  }
  printf("  Placidus/Gauquelin speeds: %d compared, worst %.2e deg/day\n", ncmp, worst);
  if (ncmp < 100) BAD("only %d speeds compared", ncmp);

  /* 2: SEFLG_RADIANS speeds */
  {
    double t = 2461223.5 + 0.3125, wr = 0;
    swe_houses_ex2_r(ctx, t, 0, 47, 8.55, 'P', cusp, ascmc, cs, as, serr);
    swe_houses_ex2_r(ctx, t, SEFLG_RADIANS, 47, 8.55, 'P', cr, ar, csr, asr, serr);
    for (i = 1; i <= 12; i++) {
      double e = fabs(csr[i] - cs[i] * M_PI / 180) / fabs(cs[i]);
      if (e > wr) wr = e;
    }
    for (i = 0; i < 8; i++) {
      double e = fabs(asr[i] - as[i] * M_PI / 180) / fabs(as[i]);
      if (e > wr) wr = e;
    }
    printf("  RADIANS speeds: worst relative difference from degrees x pi/180 %.2e\n", wr);
    if (!(wr < 1e-12))
      BAD("RADIANS speeds are not the degree speeds in radians (%.2e)", wr);
  }

  /* 3: sidereal Makransky is not Treindl */
  {
    double t = floor((2461000.5 + 5.0 / 48 + 5 * 40) * 256) / 256, ay, ti[37], si[37], sI[37], wi = 0, wI = 0;
    swe_set_sid_mode_r(ctx, SE_SIDM_LAHIRI, 0, 0);
    swe_houses_ex_r(ctx, t, 0, -60, 8.55, 'i', ti, ascmc);
    swe_houses_ex_r(ctx, t, SEFLG_SIDEREAL, -60, 8.55, 'i', si, ascmc);
    swe_houses_ex_r(ctx, t, SEFLG_SIDEREAL, -60, 8.55, 'I', sI, ascmc);
    swe_get_ayanamsa_ex_r(ctx, t + swe_deltat_ex_r(ctx, t, SEFLG_SIDEREAL, NULL), SEFLG_SIDEREAL, &ay, serr);
    for (i = 1; i <= 12; i++) {
      double e = fabs(dif(si[i], ti[i] - ay)), f = fabs(dif(si[i], sI[i]));
      if (e > wi) wi = e;
      if (f > wI) wI = f;
    }
    printf("  sidereal 'i': worst from tropical 'i' less ayanamsa %.2e deg; from sidereal 'I' %.2f deg\n", wi, wI);
    if (!(wi < 1e-8))
      BAD("sidereal 'i' cusps are not the tropical ones less the ayanamsa (%.2e)", wi);
    if (!(wI > 1))
      BAD("sidereal 'i' equals sidereal 'I' (%.2e)", wI);
    swe_set_sid_mode_r(ctx, SE_SIDM_FAGAN_BRADLEY, 0, 0);
  }

  /* 4: Sunshine houses without the Swiss files */
  {
    swe_ctx *cn = swe_ctx_new();
    static const int sy[2] = {'I', 'i'};
    double t = 2461223.5 + 0.3125, c1[37], c2[37], a1[10], a2[10];
    int k, r;
    swe_set_ephe_path_r(cn, "/nonexistent-ephe-dir");
    swe_set_ephe_fallback_r(cn, 0);
    for (k = 0; k < 2; k++) {
      double w = 0;
      swe_houses_ex2_r(ctx, t, 0, 47, 8.55, sy[k], c1, a1, NULL, NULL, serr);
      r = swe_houses_ex2_r(cn, t, 0, 47, 8.55, sy[k], c2, a2, NULL, NULL, serr);
      if (r < 0) BAD("swe_houses_ex2_r %c without files: ERR", sy[k]);
      for (i = 1; i <= 12; i++) if (fabs(dif(c1[i], c2[i])) > w) w = fabs(dif(c1[i], c2[i]));
      printf("  Sunshine %c, no files: ex2 retc %d, worst cusp difference %.2e deg, declination %.4f\n", sy[k], r, w, a2[9]);
      if (!(w < 1e-4) || fabs(a1[9] - a2[9]) > 1e-3) BAD("%c cusps/declination differ from the Swiss-file call", sy[k]);
      r = swe_houses_ex_r(cn, t, 0, 47, 8.55, sy[k], c2, a2);
      if (r < 0) BAD("swe_houses_ex_r %c without files: ERR", sy[k]);
      r = swe_houses_r(cn, t, 47, 8.55, sy[k], c2, a2);
      if (r < 0) BAD("swe_houses_r %c without files: ERR", sy[k]);
    }
    swe_ctx_free(cn);
  }
  swe_ctx_free(ctx);
  if (bad)
    printf("G58 FAIL: %d\n", bad);
  else
    printf("G58 PASS\n");
  return bad != 0;
}
