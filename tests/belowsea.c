/* belowsea.c -- G47: a site below sea level has a horizon, so the refraction
 * and the rising and setting there are numbers.
 *
 * The dip of the horizon, the angle the sea horizon lies below the geometric
 * one, was acos(1 / (1 + h / R)) of an argument above 1 for h < 0 (the
 * range swe_rise_trans() and the eclipse functions accept is -500 to 25000
 * m): NaN. An observer below sea level has no sea horizon, so the dip is
 * that of sea level, 0. What a caller saw:
 * - swe_rise_trans_true_hor() with horhgt -100 ("the horizon is the dip")
 *   answered success with the time two hours after the start, for any body
 *   and event;
 * - swe_refrac_extended() reported dret[3] NaN, never converted an
 *   apparent altitude to a true one, and refracted a body any way below
 *   the horizon;
 * - swe_azalt() gave a refracted "apparent altitude" for a body below the
 *   horizon where sea level gives the true altitude.
 *
 * The checks, with pressure 1013.25 hPa and 10 C:
 * (a) swe_refrac_extended(), both directions, at 13 altitudes from +90 to
 *   -20, at -500, -430, -86, -10 and -1e-6 m: the answer and dret[0..3]
 *   are those at 0 m bit for bit, the dip is 0, an apparent altitude of 0
 *   is a body below the true horizon by the refraction, a true altitude of
 *   -3 stays -3. Above sea level the dip is unchanged: -0.027612, -0.087317
 *   and -0.276118 deg at 1, 10 and 100 m (1e-5).
 * (b) swe_rise_trans_true_hor() with horhgt -100, the Sun and the Moon,
 *   rising and setting, at 4 sites (the Netherlands -10 m, the Dead Sea
 *   -430 m, Death Valley -86 m, and -500 m): it succeeds, the time lies
 *   after the start and within 2 days of it, is the time of horhgt 0.0001
 *   (the dip of sea level, and 0.0001 deg of margin) to 1e-9 day, and is
 *   within 1 s of the time swe_rise_trans() gives at the site at sea level
 *   (the observer's depth moves a rising by 0.1 s at most).
 * (c) swe_azalt_r() of a fixed place on the sky over a day, from +60 to -60
 *   deg: at the four heights the azimuth, the true and the apparent
 *   altitude are those at 0 m bit for bit; with the pressure estimated
 *   from the height (0) they are finite.
 *
 * Run as
 *   belowsea EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static int n, bad;

static void verdict(int ok, const char *what, double h, double v)
{
  n++;
  if (!ok) {
    bad++;
    printf("    FAIL %s at %.6g m: %.9g\n", what, h, v);
  }
}

static const double HT[] = { -500, -430, -86, -10, -1e-6 };
#define NHT ((int) (sizeof HT / sizeof HT[0]))

/* (a) the refraction and the dip */
static void check_a(void)
{
  static const double AL[] = { 90, 45, 10, 1, 0.5, 0.1, 0, -0.2, -0.5, -1, -3, -10, -20 };
  static const double HP[] = { 1, 10, 100 };
  static const double DP[] = { -0.027612, -0.087317, -0.276118 };
  int h, a, d, i, n0 = n, b0 = bad;
  for (h = 0; h < NHT; h++) {
    for (d = 0; d < 2; d++) {
      int cf = d ? SE_APP_TO_TRUE : SE_TRUE_TO_APP;
      for (a = 0; a < (int) (sizeof AL / sizeof AL[0]); a++) {
        double r0[4], r1[4], v0, v1;
        v0 = swe_refrac_extended(AL[a], 0, 1013.25, 10, 0.0065, cf, r0);
        v1 = swe_refrac_extended(AL[a], HT[h], 1013.25, 10, 0.0065, cf, r1);
        verdict(v1 == v0, d ? "apparent to true altitude as at 0 m"
                            : "true to apparent altitude as at 0 m", HT[h], v1 - v0);
        for (i = 0; i < 4; i++)
          if (r1[i] != r0[i])
            break;
        verdict(i == 4, "dret[] as at 0 m", HT[h], i < 4 ? r1[i] - r0[i] : 0);
      }
    }
    {
      double dr[4], v;
      swe_refrac_extended(1, HT[h], 1013.25, 10, 0.0065, SE_TRUE_TO_APP, dr);
      verdict(dr[3] == 0, "the dip is 0", HT[h], dr[3]);
      v = swe_refrac_extended(0, HT[h], 1013.25, 10, 0.0065, SE_APP_TO_TRUE, dr);
      verdict(v < -0.5 && v > -0.65, "an apparent altitude of 0 is 0.58 deg below the true horizon", HT[h], v);
      v = swe_refrac_extended(-3, HT[h], 1013.25, 10, 0.0065, SE_TRUE_TO_APP, dr);
      verdict(v == -3, "a body 3 deg below the horizon is not refracted", HT[h], v);
    }
  }
  for (h = 0; h < 3; h++) {
    double dr[4];
    swe_refrac_extended(1, HP[h], 1013.25, 10, 0.0065, SE_TRUE_TO_APP, dr);
    verdict(fabs(dr[3] - DP[h]) < 1e-5, "the dip above sea level is unchanged", HP[h], dr[3] - DP[h]);
  }
  printf("  (a) refraction and dip at five heights below sea level: %d checks, %d fail\n",
         n - n0, bad - b0);
}

/* (b) rising and setting with the horizon at the dip */
static void check_b(swe_ctx *c)
{
  static double SITE[4][3] = { { 4.9, 52.4, -10 }, { 35.5, 31.5, -430 },
                                     { -116.8, 36.5, -86 }, { 35.5, 31.5, -500 } };
  static const char *NAME[4] = { "Netherlands", "Dead Sea", "Death Valley", "-500 m" };
  static const int BODY[2] = { SE_SUN, SE_MOON };
  int s, b, ev, n0 = n, b0 = bad;
  double tjd = 2461338.5;
  char serr[256];
  for (s = 0; s < 4; s++) {
    for (b = 0; b < 2; b++) {
      for (ev = 0; ev < 2; ev++) {
        int32 rsmi = ev ? SE_CALC_SET : SE_CALC_RISE;
        double geo0[3], t, t1, t2, t3;
        int32 r1, r2, r3;
        memcpy(geo0, SITE[s], sizeof geo0);
        geo0[2] = 0;
        r1 = swe_rise_trans_true_hor_r(c, tjd, BODY[b], NULL, SEFLG_SWIEPH, rsmi,
                                       SITE[s], 1013.25, 10, -100, &t1, serr);
        r2 = swe_rise_trans_true_hor_r(c, tjd, BODY[b], NULL, SEFLG_SWIEPH, rsmi,
                                       SITE[s], 1013.25, 10, 0.0001, &t2, serr);
        r3 = swe_rise_trans_r(c, tjd, BODY[b], NULL, SEFLG_SWIEPH, rsmi, geo0, 1013.25, 10, &t3,
                              serr);
        t = t1 - tjd;
        printf("    %-12s %-4s %-4s: %.6f, after the start %.4f d, sea level %+.3f s\n",
               NAME[s], b ? "Moon" : "Sun", ev ? "set" : "rise", t1, t, (t1 - t3) * 86400);
        verdict(r1 >= 0 && r2 >= 0 && r3 >= 0, "a rising or setting is found", SITE[s][2], r1);
        verdict(t > 0 && t < 2, "the time lies within 2 days after the start", SITE[s][2], t);
        verdict(fabs(t1 - t2) < 1e-9, "the time is that of horhgt 0.0001", SITE[s][2], t1 - t2);
        verdict(fabs(t1 - t3) < 1.0 / 86400, "the time is within 1 s of sea level's", SITE[s][2],
                (t1 - t3) * 86400);
      }
    }
  }
  printf("  (b) rising and setting at four sites below sea level: %d checks, %d fail\n",
         n - n0, bad - b0);
}

/* (c) azimuth and altitude */
static void check_c(swe_ctx *c)
{
  static const double GEO[3] = { 35.5, 31.5, 0 };
  int h, i, n0 = n, b0 = bad;
  double xin[3] = { 100, 20, 1 }, lo = 90, hi = -90;
  for (i = 0; i < 24; i++) {
    double tj = 2461338.5 + i / 24.0, g0[3], x0[3];
    g0[0] = GEO[0];
    g0[1] = GEO[1];
    g0[2] = 0;
    swe_azalt_r(c, tj, SE_EQU2HOR, g0, 1013.25, 10, xin, x0);
    lo = fmin(lo, x0[1]);
    hi = fmax(hi, x0[1]);
    for (h = 0; h < NHT; h++) {
      double g[3], x[3], xe[3];
      g[0] = GEO[0];
      g[1] = GEO[1];
      g[2] = HT[h];
      swe_azalt_r(c, tj, SE_EQU2HOR, g, 1013.25, 10, xin, x);
      verdict(x[0] == x0[0] && x[1] == x0[1] && x[2] == x0[2],
              "azimuth, true and apparent altitude as at 0 m", HT[h], x[2] - x0[2]);
      swe_azalt_r(c, tj, SE_EQU2HOR, g, 0, 10, xin, xe);
      verdict(isfinite(xe[0]) && isfinite(xe[1]) && isfinite(xe[2]),
              "finite with the pressure estimated from the height", HT[h], xe[2]);
    }
  }
  verdict(lo < -30 && hi > 60, "the day's altitudes reach both sides of the horizon", 0, lo);
  printf("  (c) azimuth and altitude over a day (%.1f to %.1f deg): %d checks, %d fail\n",
         lo, hi, n - n0, bad - b0);
}

int main(int argc, char **argv)
{
  swe_ctx *c;
  if (argc != 2) {
    fprintf(stderr, "usage: belowsea EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  if (c == NULL) {
    printf("G47 FAIL: no context\n");
    return 1;
  }
  swe_set_ephe_path_r(c, argv[1]);
  printf("G47: refraction, rising and setting below sea level\n");
  check_a();
  check_b(c);
  check_c(c);
  swe_ctx_free(c);
  if (bad) {
    printf("G47 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG47 PASS\n", n);
  return 0;
}
