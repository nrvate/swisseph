/* aberrspeed.c -- G51: the aberration's part of a speed is the rate of the
 * aberration's part of the position, at second order in v/c too.
 *
 * The defect (UPSTREAM-BUGS 52): the aberration rate term, bug 34's tau a
 * (a the observer's acceleration, tau the light time), was added as the
 * first-order derivative of the aberrated place in v = V/c, its part along
 * the line of sight taken out, and a site's acceleration had its radial
 * part stripped before that. The aberrated place moves with v at second
 * order as well, terms in v times dv of relative size v/c = 1e-4, and a
 * radial dv turns it by v_perp |dv| / 2: a site's centripetal acceleration
 * against the Earth's orbital velocity. That was 4.5e-5 of the diurnal
 * aberration rate, 2.8-3.2e-8 deg/day on every topocentric longitude
 * speed (the Sun's 2.5e-8 on the equator, 1.8e-8 at 47N, 1.1e-8 at 70N,
 * its sign with the hour), and, from the Earth's own acceleration,
 * +4-5e-9 deg/day on every geocentric one, the Sun's at every epoch.
 *
 * The check: the Sun, Mercury, Venus, Mars, Jupiter and Ceres, ten epochs
 * -2900..2900, geocentric of date and J2000 equatorial, topocentric of date
 * and J2000 equatorial at 0, 47.37, 70 and -33.9 deg latitude, Swiss
 * ephemeris. Each speed is measured against a five-point difference of
 * the same call's positions over 2^-8 day at instants that are multiples
 * of it, with and without SEFLG_NOABERR; the difference of the two
 * residuals is the aberration's own, free of the ephemeris' speed noise
 * (3e-8 deg/day on Mars, the same in both). Longitude times cos(latitude)
 * and latitude within 5e-9 deg/day (measured <= 3.3e-9, Jupiter's
 * topocentric longitude; the Sun's <= 1e-9); the distance rate within
 * 1e-11 of the distance per day (measured <= 5.4e-12, as before).
 * Saturn and beyond are left out: the Moon's pull's rate on the Earth,
 * left out of earth_at_lt_sun()'s light-time series, gives tau j / 2c
 * there, a lunar-month wave of 1.2e-8 deg/day on Neptune. Before the fix
 * all ten sets failed, the topocentric by 1.4e-8 (70N) to 3.2e-8 (the
 * equator), the geocentric by 6.5-6.8e-9. The upstream build fails all 20:
 * the angles by 5.3e-7 geocentric and 1.7-2.7e-5 topocentric, the distance
 * rate by 3e-7..1.8e-6 of the distance per day. Run as
 *   aberrspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

/* the speed less the five-point rate of the position: longitude times
 * cos(latitude), latitude, deg/day; distance, of the distance per day */
static int resid(double t, int ipl, int32 fl, double h, double *res, char *serr)
{
  double x[6], p[5][6];
  int i, j;
  if (swe_calc(t, ipl, fl | SEFLG_SPEED, x, serr) < 0)
    return -1;
  for (i = 0; i < 5; i++)
    if (swe_calc(t + (i - 2) * h, ipl, fl, p[i], serr) < 0)
      return -1;
  for (j = 0; j < 3; j++) {
    double d1 = p[3][j] - p[1][j], d2 = p[4][j] - p[0][j];
    if (j == 0) {
      d1 -= 360 * floor(d1 / 360 + 0.5);
      d2 -= 360 * floor(d2 / 360 + 0.5);
    }
    res[j] = x[3 + j] - (8 * d1 - d2) / (12 * h);
  }
  res[0] *= cos(x[1] * DEGTORAD);
  res[2] /= x[2];
  return 0;
}

int main(int argc, char **argv)
{
  static const int body[6] = {SE_SUN, SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER, SE_CERES};
  static const double yrs[10] = {-2900, -1000, 500, 1500, 1900, 1990, 2000.3, 2030, 2200, 2900};
  static const double lat[4] = {0, 47.37, 70, -33.9};
  static const struct { int32 f; const char *n; } set[4] = {
    {0, "geo of date"}, {SEFLG_J2000 | SEFLG_NONUT | SEFLG_EQUATORIAL, "geo J2000 eq"},
    {SEFLG_TOPOCTR, "topo of date"}, {SEFLG_TOPOCTR | SEFLG_J2000 | SEFLG_NONUT | SEFLG_EQUATORIAL, "topo J2000 eq"},
  };
  const double h = 1.0 / 256;
  char serr[256];
  int k, l, b, y, j, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: aberrspeed EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G51: the aberration's part of a speed is the rate of its part of the position\n");
  for (k = 0; k < 4; k++) {
    const int32 fl = SEFLG_SWIEPH | set[k].f;
    const int nl = (fl & SEFLG_TOPOCTR) ? 4 : 1;
    for (l = 0; l < nl; l++) {
      double ang = 0, rel = 0, e;
      swe_set_topo(8.55, lat[l], 400);
      for (b = 0; b < 6; b++) {
        for (y = 0; y < 10; y++) {
          double t = floor((2451545.0 + (yrs[y] - 2000) * 365.25) * 256) / 256, ra[3], rn[3];
          if (resid(t, body[b], fl, h, ra, serr) < 0
              || resid(t, body[b], fl | SEFLG_NOABERR, h, rn, serr) < 0) {
            printf("  body %d ERROR %s\n", body[b], serr);
            return 1;
          }
          for (j = 0; j < 2; j++)
            if ((e = fabs(ra[j] - rn[j])) > ang) ang = e;
          if ((e = fabs(ra[2] - rn[2])) > rel) rel = e;
        }
      }
      if (nl > 1)
        printf("  %-13s lat %6.2f  angles %.2e deg/day, distance %.2e of it per day\n",
               set[k].n, lat[l], ang, rel);
      else
        printf("  %-13s             angles %.2e deg/day, distance %.2e of it per day\n",
               set[k].n, ang, rel);
      if (ang > 5e-9) {
        printf("    FAIL: the aberration's part of an angular speed is not its rate\n");
        bad++;
      }
      if (rel > 1e-11) {
        printf("    FAIL: the aberration's part of the distance rate is not its rate\n");
        bad++;
      }
    }
  }
  swe_close();
  printf(bad ? "G51 FAIL: %d\n" : "G51 PASS\n", bad);
  return bad ? 1 : 0;
}
