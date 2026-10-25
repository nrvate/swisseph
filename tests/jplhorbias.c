/* jplhorbias.c -- G141: SEFLG_JPLHOR_APPROX's frame bias and Horizons
 * offset in right ascension, on a JPL file of DE 403 or later, take the
 * side of SEMOD_JPLHORA_3's step at 1962 Jan 20 that the precession and
 * nutation take.
 *
 * The defect (UPSTREAM-BUGS 161): with SEFLG_JPLHOR_APPROX under
 * SEMOD_JPLHORA_3 (the default mode), swi_precess(), swi_epsiln() and
 * calc_nutation() take SEFLG_JPLHOR's models up to and including JD
 * 2437684.5 and the chosen models after it, while swi_bias() left the frame
 * bias (23 mas) and the Horizons offset in right ascension (-51.26 mas)
 * out for its tjd < the step only, and was given another instant than they
 * are: the light-time instant the body was read at, and for the true Sun
 * 0. So at the step a fixed star, a true position and the topocentric site
 * had the left side's models with the right side's bias and offset, and
 * within the light time after it the Sun, the Moon and the planets the
 * chosen models without them (the true Sun at every date after it).
 *
 * The check: de200 read as DE 431 (the context's DE number set after the
 * file opens: white-box, on purpose -- no file of DE 403 or later ships,
 * and the bias and the offset are rotations, whatever the coefficients),
 * SEFLG_JPLHOR_APPROX, the default models; the Sun, the Moon, Mars,
 * Saturn, Polaris (swe_fixstar2()), the true Sun and true Mars, the
 * topocentric Moon at Zurich and heliocentric Mars, ecliptic and
 * equatorial. After the step: at 148 instants in (step, step + 1] (every
 * 1/64 day, every 1/1024 day over the first 1/16, and step + 2^-k for k =
 * 7..26) the place under SEMOD_JPLHORA_3 against SEMOD_JPLHORA_1's, which
 * has the chosen models with the bias and offset throughout. At the step:
 * the place against its extrapolation from the left, a five-point one over
 * 2^-12 day. Both within 1e-9 deg; 36 checks. And SEFLG_SPEED3 at the
 * step, true Mars's longitude speed against the five-point difference
 * backward over 2^-8 day, within 1e-5 deg/day: 37 checks.
 *
 * Measured after the fix: SEMOD_JPLHORA_3 is SEMOD_JPLHORA_1 bit for bit
 * after the step; at the step within 3.0e-11 deg (the Moon); SPEED3
 * 1.01e-9. Before it 21 of 37 fail: after the step every body with a
 * light time, 1.11e-5 to 1.24e-5 deg over it (the Sun's 8.2 minutes,
 * Mars's 20, Saturn's 91), the true Sun throughout; at the step Polaris
 * 1.15e-5 in longitude and 4.95e-5 in right ascension, true Mars 1.12e-5,
 * the topocentric Moon 1.7e-7 (its site's frame); SPEED3 0.162 deg/day.
 * The upstream build fails 25: those, and at the step Mars and
 * heliocentric Mars by 1.3e-9 and the topocentric Moon by 1.1e-6, its own
 * defects that earlier entries fixed. Run as
 *   jplhorbias EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"	/* ctx->jpldenum: de200 read as DE 431 */

#define TS 2437684.5	/* HORIZONS_TJD0_DPSI_DEPS_IAU1980 */
#define FL (SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX)

enum { B_SUN, B_MOON, B_MARS, B_SATURN, B_POLARIS, B_TRUESUN, B_TRUEMARS,
  B_TOPOMOON, B_HELMARS, NB };
static const char *BN[NB] = { "Sun", "Moon", "Mars", "Saturn", "Polaris",
  "true Sun", "true Mars", "topocentric Moon", "heliocentric Mars" };

/* the place at t, degrees and AU, and its speeds; the return checked to be
 * the Horizons approximation's on the JPL file */
static int place(swe_ctx *c, int b, double t, int32 xf, double *x)
{
  char serr[256] = "", s[SE_MAX_STNAME * 2 + 1];
  int32 fl = FL | SEFLG_SPEED | xf, rf;
  switch (b) {
  case B_SUN: rf = swe_calc_r(c, t, SE_SUN, fl, x, serr); break;
  case B_MOON: rf = swe_calc_r(c, t, SE_MOON, fl, x, serr); break;
  case B_MARS: rf = swe_calc_r(c, t, SE_MARS, fl, x, serr); break;
  case B_SATURN: rf = swe_calc_r(c, t, SE_SATURN, fl, x, serr); break;
  case B_POLARIS:
    strcpy(s, "Polaris");
    rf = swe_fixstar2_r(c, s, t, fl, x, serr);
    break;
  case B_TRUESUN: rf = swe_calc_r(c, t, SE_SUN, fl | SEFLG_TRUEPOS, x, serr); break;
  case B_TRUEMARS: rf = swe_calc_r(c, t, SE_MARS, fl | SEFLG_TRUEPOS, x, serr); break;
  case B_TOPOMOON: rf = swe_calc_r(c, t, SE_MOON, fl | SEFLG_TOPOCTR, x, serr); break;
  default: rf = swe_calc_r(c, t, SE_MARS, fl | SEFLG_HELCTR, x, serr); break;
  }
  if (rf < 0 || !(rf & SEFLG_JPLEPH) || !(rf & SEFLG_JPLHOR_APPROX) || c->jpldenum != 431) {
    printf("  ERROR %s at %.8f: flags 0x%x, DE %d %s\n", BN[b], t, (unsigned) rf,
           (int) c->jpldenum, serr);
    return -1;
  }
  return 0;
}

/* a context on de200 read as DE 431, the models given */
static swe_ctx *open_ctx(const char *ephe, const char *models)
{
  char mod[40], serr[256];
  double x[6];
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  strcpy(mod, models);
  swe_set_ephe_path_r(c, ephe);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_topo_r(c, 8.55, 47.37, 400);
  swe_set_astro_models_r(c, mod, 0);
  if (swe_calc_r(c, 2451545.0, SE_MARS, SEFLG_JPLEPH, x, serr) < 0 || c->jpldenum != 200) {
    printf("  ERROR de200.eph: DE %d %s\n", (int) c->jpldenum, serr);
    swe_ctx_free(c);
    return NULL;
  }
  c->jpldenum = 431;
  return c;
}

static double dang(double a, double b)
{
  double d = a - b;
  return d - 360 * floor(d / 360 + 0.5);
}

/* the larger of the two angles' differences, degrees */
static double off2(const double *x, const double *y)
{
  return fmax(fabs(dang(x[0], y[0])), fabs(x[1] - y[1]));
}

static int bad = 0, n = 0;

static void judge(const char *what, int b, int f, double d, double lim)
{
  n++;
  if (!(d <= lim)) {
    bad++;
    printf("    FAIL %s %s %s: %.2e > %.0e\n", BN[b], f ? "equatorial" : "ecliptic", what, d, lim);
  }
}

/* the side of the step: after it SEMOD_JPLHORA_3 against SEMOD_JPLHORA_1,
 * at it against the extrapolation from the left */
static int side(swe_ctx *c3, swe_ctx *c1)
{
  double tq[148], x[6], y[6], f5[5][6], h = ldexp(1, -12);
  int nq = 0, b, f, k, j;
  for (k = 1; k <= 64; k++)
    tq[nq++] = TS + k / 64.0;
  for (k = 1; k <= 64; k++)
    tq[nq++] = TS + k / 1024.0;
  for (k = 7; k <= 26; k++)
    tq[nq++] = TS + ldexp(1, -k);
  printf("  161: after the step |JPLHORA_3 - JPLHORA_1|, at it |place - left extrapolation|, deg\n");
  for (b = 0; b < NB; b++)
    for (f = 0; f < 2; f++) {
      int32 xf = f ? SEFLG_EQUATORIAL : 0;
      double wa = 0, wb = 0, e[2];
      for (k = 0; k < nq; k++) {
        if (place(c3, b, tq[k], xf, x) < 0 || place(c1, b, tq[k], xf, y) < 0)
          return -1;
        if (off2(x, y) > wa)
          wa = off2(x, y);
      }
      if (place(c3, b, TS, xf, x) < 0)
        return -1;
      for (k = 1; k <= 5; k++)
        if (place(c3, b, TS - k * h, xf, f5[k - 1]) < 0)
          return -1;
      for (j = 0; j < 2; j++) {
        double d1 = f5[0][j] - f5[4][j], d2 = f5[1][j] - f5[4][j], d3 = f5[2][j] - f5[4][j],
               d4 = f5[3][j] - f5[4][j];
        if (j == 0) {
          d1 = dang(f5[0][0], f5[4][0]);
          d2 = dang(f5[1][0], f5[4][0]);
          d3 = dang(f5[2][0], f5[4][0]);
          d4 = dang(f5[3][0], f5[4][0]);
        }
        e[j] = f5[4][j] + 5 * d1 - 10 * d2 + 10 * d3 - 5 * d4;
      }
      wb = off2(x, e);
      printf("    %-17s %s  %.2e  %.2e\n", BN[b], f ? "equ" : "ecl", wa, wb);
      judge("after the step", b, f, wa, 1e-9);
      judge("at the step", b, f, wb, 1e-9);
    }
  {
    /* SEFLG_SPEED3 at the step: its places at t, t - dt, t - 2 dt */
    static const double o5[5] = { 25.0 / 12, -4, 3, -4.0 / 3, 1.0 / 4 };
    char serr[256];
    double d = 0, hs = 1.0 / 256;
    if (swe_calc_r(c3, TS, SE_MARS, FL | SEFLG_TRUEPOS | SEFLG_SPEED3, x, serr) < 0)
      return -1;
    for (k = 0; k < 5; k++) {
      if (place(c3, B_TRUEMARS, TS - k * hs, 0, f5[k]) < 0)
        return -1;
      d += o5[k] * dang(f5[k][0], f5[0][0]);
    }
    d = fabs(x[3] - d / hs);
    printf("    true Mars SPEED3 at the step: %.2e deg/day\n", d);
    judge("SPEED3 at the step", B_TRUEMARS, 0, d, 1e-5);
  }
  return 0;
}

int main(int argc, char **argv)
{
  swe_ctx *c3, *c1;
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: jplhorbias EPHEDIR\n");
    return 2;
  }
  printf("G141: SEFLG_JPLHOR_APPROX's bias and Horizons offset on DE 431 take their frame's side and instant\n");
  c3 = open_ctx(argv[1], "0,0,0,0,0,0,0,0");
  c1 = open_ctx(argv[1], "0,0,0,0,0,0,1,0");
  r = (c3 == NULL || c1 == NULL) ? -1 : side(c3, c1);
  if (c3 != NULL)
    swe_ctx_free(c3);
  if (c1 != NULL)
    swe_ctx_free(c1);
  if (r < 0) {
    printf("G141 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G141 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG141 PASS\n", n);
  return 0;
}
