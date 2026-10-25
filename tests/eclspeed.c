/* eclspeed.c -- G41: a latitude speed in the ecliptic of date is the rate
 * of the latitude, though that ecliptic turns.
 *
 * The ecliptic of date is the equator rotated about the equinox by the
 * obliquity, which changes: the mean obliquity at -47" a century (3.6e-7
 * deg/day) and the nutation in obliquity by up to 1.3e-5 deg/day (its
 * fortnightly and semiannual terms). The library rotated each velocity
 * with the position's matrix and left the rotation's rate out, so every
 * ecliptic latitude speed of date was off by up to ~1.4e-5 deg/day, and
 * every equatorial speed of a point computed in the ecliptic (the nodes,
 * the apsides) the same way round (UPSTREAM-BUGS 33).
 *
 * The check: speeds against a five-point difference of the library's own
 * positions, h = 0.01 day, at seven dates -200..+200 years, Swiss files:
 * the ecliptic latitude speed of the Sun, Moon, Mars, Chiron, the mean
 * and interpolated apogees, Aldebaran and the planetocentric Moon seen
 * from Mars; the equatorial declination speed of the mean and true nodes
 * and the interpolated apogee. Within 1e-6 deg/day (measured <= 3.5e-8),
 * and within 2.5e-7 where only the mean obliquity's 3.6e-7 is at stake
 * (the mean node's and the interpolated apogee's declination, which leave
 * the mean ecliptic, and the Sun without nutation), and within 2e-8 for
 * the true node (measured 1.7e-9; 2.8e-7 before UPSTREAM-BUGS 75, its
 * speed off by 1.7e-6 of itself from the rounded step, which the build
 * before that fix fails). The upstream build fails all twelve, by
 * 1.1-1.6e-5 and 3.2-3.9e-7. Run as
 *   eclspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define KIND_CALC 0
#define KIND_STAR 1
#define KIND_PCTR 2

static int get(swe_ctx *c, int kind, int ipl, double jd, int32 fl, double *x)
{
  char serr[256], star[64];
  if (kind == KIND_STAR) {
    strcpy(star, "Aldebaran");
    return swe_fixstar2_r(c, star, jd, fl, x, serr);
  }
  if (kind == KIND_PCTR)
    return swe_calc_pctr_r(c, jd, SE_MOON, SE_MARS, fl, x, serr);
  return swe_calc_r(c, jd, ipl, fl, x, serr);
}

int main(int argc, char **argv)
{
  struct { int kind, ipl; int32 fl; const char *sz; double lim; } rgCase[] = {
    {KIND_CALC, SE_SUN, 0, "Sun, latitude", 1e-6},
    {KIND_CALC, SE_MOON, 0, "Moon, latitude", 1e-6},
    {KIND_CALC, SE_MARS, 0, "Mars, latitude", 1e-6},
    {KIND_CALC, SE_CHIRON, 0, "Chiron, latitude", 1e-6},
    {KIND_CALC, SE_MEAN_APOG, 0, "mean apogee, latitude", 1e-6},
    {KIND_CALC, SE_INTP_APOG, 0, "interpolated apogee, latitude", 1e-6},
    {KIND_STAR, 0, 0, "Aldebaran, latitude", 1e-6},
    {KIND_PCTR, 0, 0, "Moon from Mars, latitude", 1e-6},
    {KIND_CALC, SE_MEAN_NODE, SEFLG_EQUATORIAL, "mean node, declination", 2.5e-7},
    {KIND_CALC, SE_TRUE_NODE, SEFLG_EQUATORIAL, "true node, declination", 2e-8},
    {KIND_CALC, SE_INTP_APOG, SEFLG_EQUATORIAL, "interpolated apogee, declination", 2.5e-7},
    {KIND_CALC, SE_SUN, SEFLG_NONUT, "Sun, latitude, no nutation", 2.5e-7},
  };
  int k, y, j, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: eclspeed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G41: latitude speeds in the ecliptic of date carry its turning\n");
  for (k = 0; k < (int) (sizeof(rgCase) / sizeof(rgCase[0])); k++) {
    int32 fl = SEFLG_SWIEPH | rgCase[k].fl;
    double w = 0;
    for (y = 0; y < 7; y++) {
      double jd = 2451545.0 + (-200 + y * 66.0) * 365.25 + 0.137, x[6], f[5][6], r;
      if (get(c, rgCase[k].kind, rgCase[k].ipl, jd, fl | SEFLG_SPEED, x) < 0) {
        printf("  %-34s ERROR\n", rgCase[k].sz);
        bad++;
        break;
      }
      for (j = 0; j < 5; j++)
        get(c, rgCase[k].kind, rgCase[k].ipl, jd + (j - 2) * 0.01, fl, f[j]);
      r = (f[0][1] - 8 * f[1][1] + 8 * f[3][1] - f[4][1]) / 0.12;
      if (fabs(x[4] - r) > w) w = fabs(x[4] - r);
    }
    printf("  %-34s worst %.2e deg/day (limit %.1e)\n", rgCase[k].sz, w, rgCase[k].lim);
    if (w > rgCase[k].lim) {
      printf("    FAIL: the speed leaves out the ecliptic's turning\n");
      bad++;
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G41 FAIL: %d\n" : "G41 PASS\n", bad);
  return bad ? 1 : 0;
}
