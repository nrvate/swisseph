/* framestep.c -- G139: across SEMOD_JPLHORA_3's step at 1962 Jan 20 every
 * speed of date is the rate of its own side's model.
 *
 * The defect (UPSTREAM-BUGS 153): with SEFLG_JPLHOR_APPROX under
 * SEMOD_JPLHORA_3 (the default mode), the precession, the mean obliquity
 * and the nutation are SEFLG_JPLHOR's (IAU 1976, IAU 1980 plus a fixed
 * offset) up to and including JD 2437684.5 and the chosen models after it:
 * steps of 0.113" in the precession (in longitude), 3.19 mas in the mean
 * obliquity, 7.17e-2" in dpsi on the default models. The rates that
 * difference those models reached across the step: the precession's for a
 * model other than Vondrak's, (P(t + 1) - P(t - 1)) / 2, the topocentric
 * site's precession in right ascension the same way, the obliquity's
 * eps(t + 1) - eps(t), the nutation's central difference over 1e-4 day,
 * and SEFLG_SPEED3's three places over 1e-4 day. On de200, for t in
 * (switch - 1, switch], every longitude speed of date was 1.57e-5 deg/day
 * off (Polaris's right ascension 1.31e-4, latitudes up to 8.9e-7 from the
 * obliquity), the topocentric Moon's 1.58e-7 more from its site; within
 * 1e-4 day of it 9.96e-2 deg/day (Polaris's right ascension 1.09); under
 * SEFLG_SPEED3 within 1e-4 day 5.75e-2; with IAU 2006 precession and IAU
 * 2000B nutation after it, the same within a day after it too.
 *
 * The check: de200, SEFLG_JPLHOR_APPROX, the default models and IAU 2006
 * precession with IAU 2000B nutation ("0,8,8,4", which difference after
 * the step too); the Sun, the Moon, Mars, Polaris (swe_fixstar2()),
 * sidereal Mars (Lahiri; swe_calc() drops the Horizons flags for it, so it
 * has no step), the topocentric Moon at Zurich, and Mars under
 * SEFLG_SPEED3; ecliptic and equatorial, longitude and latitude speeds, at
 * 105 instants across (switch - 2, switch + 2]: every 1/16 day, and the
 * switch -+ 2^-k for k = 7..26. The reference is a five-point difference
 * of the same call's positions over 2^-6 day, centred where its points lie
 * on the instant's side of the step (at or before it, or after it), else
 * one-sided on that side (t, t -+ h, ..., t -+ 4h). The topocentric Moon
 * takes delta t set to 2^-11 day, so that its UT1 is exact and the site has
 * no jitter, and a centred nine-point difference over 2^-7 day, at the
 * instants where that lies on their side (the one-sided ones carry the
 * diurnal parallax's curvature, 3e-6 at five points over 2^-8). Each
 * setting, body and frame is judged on the worst of its two speeds over
 * three spans, the day before the step, the day after it and the rest: 84
 * checks, within 1e-8 deg/day, SEFLG_SPEED3 within 1e-5 (its own three
 * places over 1e-4 day are 1.3e-6 off everywhere).
 *
 * Measured after the fix: within 1.45e-9 (Polaris's right ascension; the
 * Moon 1.04e-9, the topocentric Moon 9.6e-10, the others 6e-11), SPEED3
 * 1.41e-6. Before it 38 of the 84 fail: every body but sidereal Mars on the
 * day before the step under both settings and on the day after under the
 * second, and SPEED3 on both days under both, by 1.57e-5 to 1.09 deg/day.
 * The upstream build fails 80 of 84: its nutation rate, a backward
 * difference, reaches across the step just after it (0.200 deg/day,
 * Polaris's right ascension 2.14), and its own speeds are off elsewhere by
 * the defects earlier entries fixed (the topocentric Moon 4e-3, sidereal
 * Mars 5e-7); only SPEED3 away from the step passes. Run as
 *   framestep EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define TS 2437684.5	/* HORIZONS_TJD0_DPSI_DEPS_IAU1980 */
#define FL (SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX)

enum { B_SUN, B_MOON, B_MARS, B_POLARIS, B_SIDMARS, B_TOPOMOON, B_SPEED3, NB };
static const char *BN[NB] = { "Sun", "Moon", "Mars", "Polaris", "sidereal Mars",
  "topocentric Moon", "Mars SPEED3" };
static const char *MODS[2] = { "0,0,0,0,0,0,0,0", "0,8,8,4,0,0,0,0" };
static const char *RN[3] = { "day before", "day after", "outside" };

/* the place at t, in degrees, and its speeds; the flags returned checked to
 * be the Horizons approximation's on de200 (but for sidereal Mars) */
static int place(swe_ctx *c, int b, double t, int32 xf, double *x)
{
  char serr[256] = "", s[SE_MAX_STNAME * 2 + 1];
  int32 fl = FL | SEFLG_SPEED | xf, rf;
  switch (b) {
  case B_SUN: rf = swe_calc_r(c, t, SE_SUN, fl, x, serr); break;
  case B_MOON: rf = swe_calc_r(c, t, SE_MOON, fl, x, serr); break;
  case B_MARS: rf = swe_calc_r(c, t, SE_MARS, fl, x, serr); break;
  case B_POLARIS:
    strcpy(s, "Polaris");
    rf = swe_fixstar2_r(c, s, t, fl, x, serr);
    break;
  case B_SIDMARS: rf = swe_calc_r(c, t, SE_MARS, fl | SEFLG_SIDEREAL, x, serr); break;
  case B_TOPOMOON: rf = swe_calc_r(c, t, SE_MOON, fl | SEFLG_TOPOCTR, x, serr); break;
  default:
    rf = swe_calc_r(c, t, SE_MARS, (fl & ~SEFLG_SPEED) | SEFLG_SPEED3, x, serr);
    break;
  }
  if (rf < 0 || !(rf & SEFLG_JPLEPH)
      || (b != B_SIDMARS && !(rf & SEFLG_JPLHOR_APPROX))) {
    printf("  ERROR %s at %.8f: flags 0x%x %s\n", BN[b], t, (unsigned) rf, serr);
    return -1;
  }
  return 0;
}

/* on the instant's side of the step: at or before it, or after it */
static int same_side(double p, double t)
{
  return (t <= TS) ? (p <= TS) : (p > TS);
}

/* |speed - reference| for the two angles, the larger; 0 with *skip set
 * where the body's reference does not lie on t's side */
static double off(swe_ctx *c, int b, double t, int32 xf, int *skip, int *err)
{
  static const double c5[2] = { 2.0 / 3, -1.0 / 12 };
  static const double c9[4] = { 4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280 };
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  int n = (b == B_TOPOMOON) ? 4 : 2, k, j, centred = 1, dir;
  double h = (b == B_TOPOMOON) ? 1.0 / 128 : 1.0 / 64, x[6], f[9][6], r = 0;
  *skip = 0;
  for (k = 1; k <= n; k++)
    if (!same_side(t - k * h, t) || !same_side(t + k * h, t))
      centred = 0;
  if (!centred && b == B_TOPOMOON) {
    *skip = 1;
    return 0;
  }
  if (place(c, b, t, xf, x) < 0) {
    *err = 1;
    return 0;
  }
  dir = (t <= TS) ? -1 : 1;
  for (k = 0; k <= 2 * n; k++) {
    double tk = centred ? t + (k - n) * h : t + dir * k * h;
    if (place(c, b, tk, xf, f[k]) < 0) {
      *err = 1;
      return 0;
    }
  }
  for (j = 0; j < 2; j++) {
    double d = 0, v;
    for (k = 0; k <= 2 * n; k++) {
      v = f[k][j] - f[centred ? n : 0][j];
      if (j == 0)
        v -= 360 * floor(v / 360 + 0.5);
      if (centred) {
        if (k > n) d += v * (n == 2 ? c5 : c9)[k - n - 1];
        if (k < n) d -= v * (n == 2 ? c5 : c9)[n - k - 1];
      } else {
        d += v * o5[k];
      }
    }
    d /= h;
    if (!centred)
      d *= dir;
    if (fabs(x[3 + j] - d) > r)
      r = fabs(x[3 + j] - d);
  }
  return r;
}

int main(int argc, char **argv)
{
  double tq[105];
  int nq = 0, m, b, f, k, i, bad = 0, n = 0, err = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: framestep EPHEDIR\n");
    return 2;
  }
  for (k = -32; k <= 32; k++)
    tq[nq++] = TS + k / 16.0;
  for (k = 7; k <= 26; k++) {
    tq[nq++] = TS - ldexp(1, -k);
    tq[nq++] = TS + ldexp(1, -k);
  }
  printf("G139: the speeds across SEMOD_JPLHORA_3's step at 1962 Jan 20 are each side's rate\n");
  for (m = 0; m < 2; m++) {
    char mod[40];
    swe_ctx *c = swe_ctx_new();
    strcpy(mod, MODS[m]);
    swe_set_ephe_path_r(c, argv[1]);
    swe_set_jpl_file_r(c, "de200.eph");
    swe_set_topo_r(c, 8.55, 47.37, 400);
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    swe_set_delta_t_userdef_r(c, ldexp(1, -11));
    swe_set_astro_models_r(c, mod, 0);
    printf("  models %s: worst |speed - reference|, deg/day, %s / %s / %s\n",
           MODS[m], RN[0], RN[1], RN[2]);
    for (b = 0; b < NB; b++)
      for (f = 0; f < 2; f++) {
        int32 xf = f ? SEFLG_EQUATORIAL : 0;
        double w[3] = { 0, 0, 0 }, lim = (b == B_SPEED3) ? 1e-5 : 1e-8;
        for (i = 0; i < nq; i++) {
          int skip, reg;
          double d = off(c, b, tq[i], xf, &skip, &err);
          if (err) {
            swe_ctx_free(c);
            printf("G139 FAIL: ERROR\n");
            return 1;
          }
          if (skip)
            continue;
          reg = (tq[i] > TS - 1 && tq[i] <= TS) ? 0 : (tq[i] > TS && tq[i] <= TS + 1) ? 1 : 2;
          if (d > w[reg])
            w[reg] = d;
        }
        printf("    %-16s %s  %.2e  %.2e  %.2e\n", BN[b], f ? "equ" : "ecl", w[0], w[1], w[2]);
        for (k = 0; k < 3; k++) {
          n++;
          if (!(w[k] <= lim)) {
            bad++;
            printf("    FAIL %s %s, %s: %.2e > %.0e\n", BN[b], f ? "equatorial" : "ecliptic",
                   RN[k], w[k], lim);
          }
        }
      }
    swe_ctx_free(c);
  }
  if (bad) {
    printf("G139 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG139 PASS\n", n);
  return 0;
}
