/* starspeed.c -- G49: a fixed star's speeds are the rates of its returned
 * position, distance included.
 *
 * Four defects, all in the star's path (UPSTREAM-BUGS 49):
 * - the deflection's and the aberration's rates were differences of two
 *   vectors of the star's distance (1.6e7 AU for Spica) over 5e-7 and 1e-5
 *   day, whose last bits made 2.3e-3 AU/day of the distance rate and 1e-8
 *   deg/day of the longitude speed; the nutation's, over 1e-4 day, 2e-5
 *   AU/day;
 * - the precession's rate was the general precession about the ecliptic
 *   pole of date, without the ecliptic's own turning: 3e-8 deg/day on every
 *   latitude speed of date, 7e-8 on a polar star's longitude speed;
 * - the aberration's rate was a backward difference over a nominal 1e-5
 *   day that rounding makes 2.3e-5 longer or shorter: 1.5e-8 deg/day of the
 *   topocentric speed, from the diurnal part;
 * - swe_fixstar() took the ayanamsa off a sidereal longitude but not its
 *   rate off the speed, 3.8e-5 deg/day.
 *
 * The check: nine stars (Spica, Aldebaran, Sirius, Regulus, Barnard's
 * Star, Polaris, the galactic centre, and 42 Dra and delta Dor near the
 * ecliptic poles), five dates 1900-2100, ten flag sets (apparent of date,
 * NOABERR, J2000 NONUT, topocentric, heliocentric, barycentric, sidereal,
 * equatorial, XYZ, equatorial XYZ), swe_fixstar2() and swe_fixstar(), all
 * against five-point differences of the same call's positions over 2^-8
 * day at instants that are multiples of it. Per flag set and function: the
 * angular speeds, longitude times cos(latitude), within 1e-10 deg/day
 * (measured <= 3.5e-11, the five-point reference's own noise, the apparent
 * ones 9e-12 to 1.2e-11; they were up to 4.0e-11, the apparent 1.9e-11 to
 * 2.2e-11, while the aberration's rate took the Earth's acceleration half a
 * step back, UPSTREAM-BUGS 76, and 6.6e-10 while the nutation's rate was a
 * backward difference, UPSTREAM-BUGS 58, with the limit 1e-8); the distance rate within 1e-12 of the distance
 * per day (measured <= 1.7e-13, likewise); XYZ rates within 1.7e-11 of the
 * distance per day (measured 7e-13). The upstream build fails all 36: the
 * distance rate by 4.9e-10 of the distance per day, swe_fixstar()'s
 * sidereal speed by 3.8e-5 deg/day, and, with bug 33's turning restored too, 1e-5 of date. Run as
 *   starspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static int star_at(int old, const char *name, double t, int32 fl, double *x, char *serr)
{
  char s[SE_MAX_STNAME * 2 + 1];
  strcpy(s, name);
  return old ? swe_fixstar(s, t, fl, x, serr) : swe_fixstar2(s, t, fl, x, serr);
}

int main(int argc, char **argv)
{
  static const char *star[9] = {"Spica", "Aldebaran", "Sirius", "Regulus", "Barnard's star",
                                "Polaris", ",SgrA*", "Fafnir", ",deDor"};
  static const double date[5] = {2415020.3, 2451545.0, 2460000.3, 2470000.7, 2488070.1};
  static const struct { int32 f; const char *n; } set[10] = {
    {0, "apparent"}, {SEFLG_NOABERR, "noaberr"}, {SEFLG_J2000 | SEFLG_NONUT, "j2000"},
    {SEFLG_TOPOCTR, "topo"}, {SEFLG_HELCTR, "helio"}, {SEFLG_BARYCTR, "bary"},
    {SEFLG_SIDEREAL, "sidereal"}, {SEFLG_EQUATORIAL, "equatorial"}, {SEFLG_XYZ, "xyz"},
    {SEFLG_EQUATORIAL | SEFLG_XYZ, "eq xyz"},
  };
  const double h = 1.0 / 256;
  char serr[256];
  int s, d, k, i, j, old, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: starspeed EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  printf("G49: a fixed star's speeds are the rates of its returned position\n");
  for (k = 0; k < 10; k++) {
    for (old = 0; old <= 1; old++) {
      const int32 fl = SEFLG_SWIEPH | set[k].f;
      const int xyz = (fl & SEFLG_XYZ) != 0;
      double ang = 0, rel = 0;
      for (s = 0; s < 9; s++) {
        for (d = 0; d < 5; d++) {
          double t = floor(date[d] * 256) / 256, x[6], p[5][6], r[3], e;
          if (star_at(old, star[s], t, fl | SEFLG_SPEED, x, serr) < 0) {
            printf("  %s ERROR %s\n", star[s], serr);
            return 1;
          }
          for (i = 0; i < 5; i++)
            star_at(old, star[s], t + (i - 2) * h, fl, p[i], serr);
          for (j = 0; j < 3; j++) {
            double d1 = p[3][j] - p[1][j], d2 = p[4][j] - p[0][j];
            if (!xyz && j == 0) {
              d1 -= 360 * floor(d1 / 360 + 0.5);
              d2 -= 360 * floor(d2 / 360 + 0.5);
            }
            r[j] = (8 * d1 - d2) / (12 * h);
          }
          if (xyz) {
            double rr = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
            for (j = 0; j < 3; j++)
              if ((e = fabs(x[3 + j] - r[j]) / rr) > rel) rel = e;
          } else {
            if ((e = fabs(x[3] - r[0]) * cos(x[1] * DEGTORAD)) > ang) ang = e;
            if ((e = fabs(x[4] - r[1])) > ang) ang = e;
            if ((e = fabs(x[5] - r[2]) / x[2]) > rel) rel = e;
          }
        }
      }
      if (xyz) {
        printf("  %-10s %-12s XYZ rates %.2e of the distance per day\n",
               set[k].n, old ? "swe_fixstar" : "swe_fixstar2", rel);
        if (rel > 1.7e-11) {
          printf("    FAIL: an XYZ speed is not the rate of the position\n");
          bad++;
        }
      } else {
        printf("  %-10s %-12s angles %.2e deg/day, distance %.2e of it per day\n",
               set[k].n, old ? "swe_fixstar" : "swe_fixstar2", ang, rel);
        if (ang > 1e-10) {
          printf("    FAIL: an angular speed is not the rate of the position\n");
          bad++;
        }
        if (rel > 1e-12) {
          printf("    FAIL: the distance rate is not the rate of the distance\n");
          bad++;
        }
      }
    }
  }
  swe_close();
  printf(bad ? "G49 FAIL: %d\n" : "G49 PASS\n", bad);
  return bad ? 1 : 0;
}
