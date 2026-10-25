/* housecusps.c -- G33: the cusps and speeds a house call answers.
 *
 * Placidus and Gauquelin cusps are the fixed point of C = Asc1(ra, f(C)).
 * The iteration stopped at a step of 1/360000 deg and left the cusp up to
 * 3.5e-8 deg off (4.5e-6 at 65N), an error that moves with the instant.
 * Sunshine houses need the Sun's declination, and swe_houses_ex2() has
 * four more things to answer for. Seven checks, each failing before the
 * fix. Run as
 *   housecusps EPHEDIR
 *
 * 1. Converged cusps. A Placidus cusp 11, 12, 2, 3 stands at the fraction
 *    1/3, 2/3, 2/3, 1/3 of its own semi-arc from the meridian, and a
 *    Gauquelin sector ih at ih2/9 of it, ih2 = 10 - ih (sectors 2..9) or
 *    ih - 28 (29..36); the semi-arc is 90 + AD for the cusp's declination d,
 *    AD = asin(tan fi tan d), and 90 - AD on the nocturnal side. The test
 *    puts each cusp's right ascension against that, at 47N, 60N and 65N
 *    over 24 ARMCs: the residual is under 1e-11 deg (up to 3e-7 before).
 * 2. A cusp on an equinox point (declination 0) moves at the rate of Asc1
 *    in right ascension at the pole height m tan(fi), not at the ARMC's
 *    360.99 deg/day. The speeds of Placidus cusps 11, 12, 2, 3 and
 *    Gauquelin sectors 9 and 29, at the ARMC that puts each on 0 Aries,
 *    are compared at 47N and 60N with five-point differences of the cusps
 *    over 2^-8 and 2^-9 deg of ARMC (Richardson), 1e-7 deg/day + 1e-10 of
 *    the rate. They are 415..788 deg/day.
 * 3. With SEFLG_RADIANS the speeds are the degree speeds times pi/180, as
 *    the cusps and the ascmc are (they stayed in deg/day).
 * 4. Sidereal Makransky Sunshine ('i') is the tropical 'i' less the
 *    ayanamsa, not Treindl's 'I' (41 deg apart at 60S).
 * 5. Sunshine houses without the Swiss files (and the ephemeris fallback
 *    off, as shipped) answer OK from swe_houses_ex2_r(), swe_houses_ex_r()
 *    and swe_houses_r(), with the Swiss-file cusps to 1e-4 deg: the Sun's
 *    declination comes from Moshier's, 0.1" off. They returned ERR and
 *    Porphyry, 16.7 deg off.
 * 6. The Sun of Sunshine houses comes from the ephemeris the caller chose:
 *    ascmc[9] of a SEFLG_MOSEPH call is Moshier's declination to 1e-12 deg
 *    (it was the Swiss one, 6.0e-6 deg apart).
 * 7. A Gauquelin call that falls back to Porphyry (80N) has 12 cusps; the
 *    entries 13..36 of cusp and cusp_speed come back 0, in degrees and in
 *    radians, from swe_houses_ex2_r() (tropical and the three sidereal
 *    kinds), swe_houses_r() and swe_houses_armc_ex2_r(). Arrays filled
 *    with 7 came back 7 (and 7 pi/180 in radians).
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define BAD(...) do { printf("  FAIL: " __VA_ARGS__); printf("\n"); bad++; } while (0)
#define EPS 23.4392911

static double dif(double a, double b)
{
  double d = fmod(a - b, 360);
  if (d > 180) d -= 360;
  else if (d <= -180) d += 360;
  return d;
}

/* The residual of cusp ih's defining condition, in degrees: its right
 * ascension less the right ascension its fraction of its own semi-arc
 * gives. */
static double residual(int hs, int ih, double armc, double lat, double c)
{
  double r = M_PI / 180, e = EPS * r, ad, ra, target, m;
  double dec = asin(sin(e) * sin(c * r));
  ad = asin(tan(lat * r) * tan(dec)) / r;
  ra = atan2(cos(e) * sin(c * r), cos(c * r)) / r;
  if (hs == 'P') {
    static const double day[4] = {30, 60, 120, 150};
    static const double frac[4] = {1.0 / 3, 2.0 / 3, 2.0 / 3, 1.0 / 3};
    int k = ih == 11 ? 0 : ih == 12 ? 1 : ih == 2 ? 2 : 3;
    target = armc + day[k] + frac[k] * ad;
  } else if (ih <= 9) {
    m = (10 - ih) / 9.0;
    target = armc + m * (90 + ad);
  } else {
    m = (ih - 28) / 9.0;
    target = armc + 180 - m * (90 - ad);
  }
  return dif(ra, target);
}

int main(int argc, char **argv)
{
  char serr[256];
  double cusp[37], ascmc[10], cs[37], as[10];
  int bad = 0, i, k;
  swe_ctx *ctx;

  if (argc != 2) {
    fprintf(stderr, "usage: housecusps EPHEDIR\n");
    return 2;
  }
  ctx = swe_ctx_new();
  swe_set_ephe_path_r(ctx, argv[1]);
  printf("G33: the cusps and speeds a house call answers\n");

  /* 1: converged Placidus and Gauquelin cusps */
  {
    static const int hsy[2] = {'P', 'G'};
    static const double lats[3] = {47, 60, 65};
    double worst = 0;
    int a, b, n = 0;
    for (a = 0; a < 2; a++) for (b = 0; b < 3; b++) for (k = 0; k < 24; k++) {
      double armc = 7.3 + k * 14.9;
      int nc = hsy[a] == 'G' ? 36 : 12;
      if (swe_houses_armc_ex2_r(ctx, armc, lats[b], EPS, hsy[a], cusp, ascmc, NULL, NULL, serr) < 0)
        continue;
      for (i = 1; i <= nc; i++) {
        double e;
        if (hsy[a] == 'P' && i != 11 && i != 12 && i != 2 && i != 3)
          continue;
        if (hsy[a] == 'G' && !((i >= 2 && i <= 9) || (i >= 29 && i <= 36)))
          continue;
        e = fabs(residual(hsy[a], i, armc, lats[b], cusp[i]));
        n++;
        if (e > worst) worst = e;
        if (e > 1e-11)
          BAD("%c cusp %d at %gN, ARMC %g: residual %.2e deg", hsy[a], i, lats[b], armc, e);
      }
    }
    printf("  converged cusps: %d checked, worst residual %.2e deg\n", n, worst);
    if (n < 1000) BAD("only %d cusps checked", n);
  }

  /* 2: the cusp on an equinox point */
  {
    static const struct { int hs, ih; double armc; } c2[6] = {
      {'P', 11, 330}, {'P', 12, 300}, {'P', 2, 240}, {'P', 3, 210}, {'G', 9, 350}, {'G', 29, 190}};
    static const double lats[2] = {47, 60};
    static const int off[4] = {-2, -1, 1, 2};
    double worst = 0;
    int b;
    for (k = 0; k < 6; k++) for (b = 0; b < 2; b++) {
      double h[2] = {1.0 / 256, 1.0 / 512}, w[2][4][37], d[2], ref, err;
      int s, j, ih = c2[k].ih;
      swe_houses_armc_ex2_r(ctx, c2[k].armc, lats[b], EPS, c2[k].hs, cusp, ascmc, cs, as, serr);
      for (s = 0; s < 2; s++) for (j = 0; j < 4; j++)
        swe_houses_armc_ex2_r(ctx, c2[k].armc + off[j] * h[s], lats[b], EPS, c2[k].hs, w[s][j], ascmc, NULL, NULL, serr);
      for (s = 0; s < 2; s++) {
        double a1 = dif(w[s][2][ih], cusp[ih]) - dif(w[s][1][ih], cusp[ih]);
        double a2 = dif(w[s][3][ih], cusp[ih]) - dif(w[s][0][ih], cusp[ih]);
        d[s] = (8 * a1 - a2) / (12 * h[s]);
      }
      ref = (d[1] + (d[1] - d[0]) / 15) * 360.98564736629;	/* per day */
      err = fabs(cs[ih] - ref);
      if (err > worst) worst = err;
      if (err > 1e-7 + 1e-10 * fabs(ref))
        BAD("%c cusp %d at %gN: speed %.9f, difference %.9f", c2[k].hs, ih, lats[b], cs[ih], ref);
    }
    printf("  equinox-point cusps: worst %.2e deg/day from the differences\n", worst);
  }

  /* 3: SEFLG_RADIANS speeds */
  {
    double t = 2461223.5 + 0.3125, wr = 0, cr[37], ar[10], csr[37], asr[10];
    swe_houses_ex2_r(ctx, t, 0, 47, 8.55, 'P', cusp, ascmc, cs, as, serr);
    swe_houses_ex2_r(ctx, t, SEFLG_RADIANS, 47, 8.55, 'P', cr, ar, csr, asr, serr);
    for (i = 1; i <= 12; i++) {
      double e = fabs(csr[i] - cs[i] * M_PI / 180) / fmax(fabs(cs[i]), 1);
      if (e > wr) wr = e;
    }
    for (i = 0; i < 8; i++) {
      double e = fabs(asr[i] - as[i] * M_PI / 180) / fmax(fabs(as[i]), 1);
      if (e > wr) wr = e;
    }
    printf("  RADIANS speeds: worst relative difference from degrees x pi/180 %.2e\n", wr);
    if (!(wr < 1e-12))
      BAD("RADIANS speeds are not the degree speeds in radians (%.2e)", wr);
  }

  /* 4: sidereal Makransky is not Treindl */
  {
    double t = floor((2461000.5 + 5.0 / 48 + 5 * 40) * 256) / 256, ay;
    double ti[37], si[37], sI[37], wi = 0, wI = 0;
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

  /* 5: Sunshine houses without the Swiss files */
  {
    swe_ctx *cn = swe_ctx_new();
    static const int sy[2] = {'I', 'i'};
    double t = 2461223.5 + 0.3125, c1[37], c2[37], a1[10], a2[10];
    int r;
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

  /* 6: the Sun's ephemeris for Sunshine houses */
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

  /* 7: Gauquelin fallen back to Porphyry */
  {
    static const struct { const char *name; int32 mode, flag; } kinds[4] = {
      {"tropical", 0, 0},
      {"sidereal", SE_SIDM_LAHIRI, SEFLG_SIDEREAL},
      {"sidereal, ecliptic of t0", SE_SIDM_LAHIRI | SE_SIDBIT_ECL_T0, SEFLG_SIDEREAL},
      {"sidereal, solar system plane", SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, SEFLG_SIDEREAL}};
    double t = 2461223.5 + 0.3125, c[37], a[10], sp[37], sa[10];
    int r, nz, nr;
    for (k = 0; k < 4; k++) {
      int rad;
      nz = 0;
      swe_set_sid_mode_r(ctx, kinds[k].mode, 0, 0);
      for (rad = 0; rad < 2; rad++) {
        int32 iflag = kinds[k].flag | (rad ? SEFLG_RADIANS : 0);
        for (i = 0; i < 37; i++) c[i] = sp[i] = 7;
        r = swe_houses_ex2_r(ctx, t, iflag, 80, 8.55, 'G', c, a, sp, sa, serr);
        if (r >= 0) BAD("Gauquelin at 80N did not fall back (retc %d)", r);
        for (i = 13; i <= 36; i++)
          nz += (c[i] != 0) + (sp[i] != 0);
      }
      printf("  Gauquelin at 80N, %s: %d of 96 entries 13..36 not 0\n", kinds[k].name, nz);
      if (nz) BAD("%d of cusps/speeds 13..36 left unset after the Porphyry fallback (%s)", nz, kinds[k].name);
    }
    swe_set_sid_mode_r(ctx, SE_SIDM_FAGAN_BRADLEY, 0, 0);
    {
      /* swe_houses_r() and swe_houses_armc_ex2_r() the same way */
      double c1[37], c2[37], s2[37];
      for (i = 0; i < 37; i++) c1[i] = c2[i] = s2[i] = 7;
      swe_houses_r(ctx, t, 80, 8.55, 'G', c1, a);
      swe_houses_armc_ex2_r(ctx, 100, 80, EPS, 'G', c2, a, s2, sa, serr);
      nr = 0;
      for (i = 13; i <= 36; i++)
        nr += (c1[i] != 0) + (c2[i] != 0) + (s2[i] != 0);
      printf("  swe_houses_r / swe_houses_armc_ex2_r: %d of 72 not 0\n", nr);
      if (nr) BAD("%d of cusps/speeds 13..36 left unset by swe_houses_r/armc_ex2_r", nr);
    }
  }
  swe_ctx_free(ctx);
  if (bad)
    printf("G33 FAIL: %d\n", bad);
  else
    printf("G33 PASS\n");
  return bad != 0;
}
