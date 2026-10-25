/* moshbary.c -- G108: Moshier's fictitious bodies, fixed stars, apparent
 * distances and heliocentric Moon see the barycentre.
 *
 * The defects (UPSTREAM-BUGS 109). Moshier's places are heliocentric, and
 * bug 102 moved its light time about the barycentre only where aberration
 * was off; four paths were left heliocentric:
 * (a) the fictitious bodies (app_pos_etc_plan_osc(), and
 *     app_pos_etc_orbel() for swe_calc_orbel()) solved their light time
 *     with the Sun held still: the Sun's motion over it, v_S tau, missing,
 *     2.6e-6 to 3.0e-6 deg heliocentric (Cupido, Isis, Vulcan);
 * (b) a fixed star's parallax was taken from the heliocentric Earth (none
 *     at all heliocentric, where the Sun's 0.005-0.01 AU from the
 *     barycentre is the parallax) and its aberration with the
 *     heliocentric Earth's velocity, v_S short: 3.0e-6 deg on every
 *     apparent star, up to 9.2e-7 without aberration (Sirius);
 * (c) the heliocentric Moon's speed was a constant 4.2e-8 deg/day off its
 *     positions (7.4e-10 AU/day in distance): the Earth's Taylor carry over
 *     the Moon's light time with the gravity model's acceleration, not the
 *     series' (monthly terms 1e-4 of it), and the geocentric Moon's
 *     without aberration 1.1e-8 deg/day for the same reason;
 * (d) with aberration the light time stayed heliocentric, where the
 *     direction cancels against the aberration but the distance does not:
 *     rho . v_S tau, 5e-8 AU on the Sun to 2.4e-6 on Pluto, 8.3e-7 on
 *     Chiron, 1.3e-10 on the Moon.
 *
 * The checks, Swiss files and Moshier, binary-fraction instants, 13 dates
 * 1800..2200 (the files' range):
 * (1) Cupido, Isis and Vulcan heliocentric, J2000 ICRS XYZ, and a
 *     swe_calc_orbel() orbit about the Sun (a = 0.387 AU) heliocentric:
 *     the elements are heliocentric on both paths, so the place differs
 *     only by the Sun's motion over the light time; within 2e-9 deg of the
 *     Swiss files (measured <= 9.6e-10: the files' Sun is Moshier's
 *     planets' barycentre to 6.5 mm/s, 1.2e-9 deg of v_S / c, G101).
 * (2) Sirius, Spica and Polaris, J2000 ICRS XYZ: without aberration, true
 *     positions and heliocentric within 1e-9 deg of the Swiss files
 *     (measured <= 1.1e-10, Sirius's parallax of the Moshier Earth's own
 *     1e-7 AU); apparent within 3e-8 (measured 1.2e-8: Moshier's Earth's
 *     velocity is 5.4 cm/s off, 1.04e-8 deg of aberration).
 * (3) The Moon heliocentric, and geocentric without aberration: longitude
 *     and latitude speeds against five-point differences of the positions
 *     over 2^-7 day, 13 dates 1700..2300, within 1e-9 deg/day heliocentric
 *     (measured 1.9e-11) and 5e-9 geocentric (2.2e-9, the Moon's own
 *     series), the heliocentric distance rate within 1e-11 AU/day (9e-14).
 * (4) The apparent distance less the astrometric one (SEFLG_NOABERR), the
 *     light time's share, Moshier against the Swiss files: the Sun, the
 *     Moon, Mercury to Pluto and Chiron within 1e-12 AU (measured
 *     <= 2.1e-14, the Moon 1.3e-13: Venus, the Earth-Moon barycentre and
 *     the giants alone in its v_S); and the apparent distance rate of the
 *     Sun, the Moon, Mars and Pluto against five-point differences over
 *     2^-7 day within 1e-11 AU/day (<= 3.3e-12).
 * The build before the fix fails 30 of the 34 rows: the 4 of (1) (2.8e-6
 * to 3.0e-6 deg), the 12 of (2) (Sirius 9.2e-7 without aberration and
 * heliocentric, Spica 2.4e-8, Polaris 1.8e-8, 2.8e-6 to 2.9e-6
 * apparent), the 3 of (3) (4.1e-8, 5.1e-10, 6.3e-9) and the 11 distances
 * of (4) (Sun 4.8e-8 AU to Pluto 1.95e-6, Chiron 7.3e-7, the Moon 1.05e-10).
 * The upstream build fails 23: (1) to (3) (bugs 26, 35, 53 besides) and
 * the 4 distance rates; its 11 distance rows pass, both its light times
 * held heliocentric alike.
 * Run as
 *   moshbary EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static const int32 FR = SEFLG_J2000 | SEFLG_ICRS | SEFLG_XYZ;

static double norm(const double *x)
{
  return sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
}

static double ang(const double *a, const double *b)
{
  double d[3];
  int i;
  for (i = 0; i < 3; i++)
    d[i] = a[i] / norm(a) - b[i] / norm(b);
  return norm(d) * 180 / M_PI;
}

static double date(int y)	/* 13 dates 1800..2200 */
{
  return floor((2451545.0 + (1800 + y * 400.0 / 12 - 2000) * 365.25 + 23.1 * y) * 256) / 256;
}

static int verdict(const char *sz, double w, double lim, const char *unit)
{
  printf("  %-36s %.2e %s\n", sz, w, unit);
  if (w > lim) {
    printf("    FAIL: more than %.0e %s\n", lim, unit);
    return 1;
  }
  return 0;
}

/* (1) */
static int fict(int ipl, const char *sz)
{
  char serr[256];
  double a[6], b[6], w = 0;
  int y;
  for (y = 0; y < 13; y++) {
    double t = date(y);
    if (swe_calc(t, ipl, SEFLG_MOSEPH | FR | SEFLG_HELCTR, a, serr) < 0
        || swe_calc(t, ipl, SEFLG_SWIEPH | FR | SEFLG_HELCTR, b, serr) < 0) {
      printf("  %s ERROR %s\n", sz, serr);
      return 1;
    }
    if (ang(a, b) > w)
      w = ang(a, b);
  }
  return verdict(sz, w, 2e-9, "deg");
}

static int orbel(const char *sz)
{
  static const double el[6] = {10, 0.387, 0.2056, 29, 48, 7.0};
  char serr[256];
  double a[6], b[6], w = 0;
  int y;
  for (y = 0; y < 13; y++) {
    double t = date(y);
    if (swe_calc_orbel(t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, SEFLG_MOSEPH | FR | SEFLG_HELCTR, a, serr) < 0
        || swe_calc_orbel(t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, SEFLG_SWIEPH | FR | SEFLG_HELCTR, b, serr) < 0) {
      printf("  %s ERROR %s\n", sz, serr);
      return 1;
    }
    if (ang(a, b) > w)
      w = ang(a, b);
  }
  return verdict(sz, w, 2e-9, "deg");
}

/* (2) */
static int star(const char *name, int32 fl, const char *sz, double lim)
{
  char serr[256], s[SE_MAX_STNAME * 2 + 1];
  double a[6], b[6], w = 0;
  int y;
  for (y = 0; y < 13; y++) {
    double t = date(y);
    strcpy(s, name);
    if (swe_fixstar2(s, t, SEFLG_MOSEPH | FR | fl, a, serr) < 0) {
      printf("  %s ERROR %s\n", sz, serr);
      return 1;
    }
    strcpy(s, name);
    if (swe_fixstar2(s, t, SEFLG_SWIEPH | FR | fl, b, serr) < 0) {
      printf("  %s ERROR %s\n", sz, serr);
      return 1;
    }
    if (ang(a, b) > w)
      w = ang(a, b);
  }
  return verdict(sz, w, lim, "deg");
}

/* (3), (4): speeds against five-point differences over 2^-7 day; nc
 * coordinates from c0 (0 longitude, 1 latitude, 2 distance) */
static int rate(int ipl, int32 fl, int c0, int nc, const char *sz, double lim, const char *unit)
{
  char serr[256];
  double w = 0, h = 1.0 / 128;
  int y, k, i;
  for (y = 0; y < 13; y++) {
    double t = floor((2451545.0 + (1700 + y * 50 - 2000) * 365.25 + 37.3 * y) * 256) / 256;
    double x[6], d[5][6];
    if (swe_calc(t, ipl, SEFLG_MOSEPH | fl | SEFLG_SPEED, x, serr) < 0) {
      printf("  %s ERROR %s\n", sz, serr);
      return 1;
    }
    for (k = 0; k < 5; k++)
      swe_calc(t + (k - 2) * h, ipl, SEFLG_MOSEPH | fl, d[k], serr);
    for (i = c0; i < c0 + nc; i++) {
      double v[5], e;
      for (k = 0; k < 5; k++)
        v[k] = i ? d[k][i] : d[2][0] + swe_difdeg2n(d[k][0], d[2][0]);
      e = fabs(x[3 + i] - (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h));
      if (e > w)
        w = e;
    }
  }
  return verdict(sz, w, lim, unit);
}

/* (4) */
static int apdist(int ipl, const char *sz)
{
  char serr[256];
  double x[4][6], w = 0;
  int y, e;
  for (y = 0; y < 13; y++) {
    double t = date(y);
    for (e = 0; e < 2; e++) {
      int32 fl = e ? SEFLG_SWIEPH : SEFLG_MOSEPH;
      if (swe_calc(t, ipl, fl, x[2 * e], serr) < 0
          || swe_calc(t, ipl, fl | SEFLG_NOABERR, x[2 * e + 1], serr) < 0) {
        printf("  %s ERROR %s\n", sz, serr);
        return 1;
      }
    }
    double d = fabs((x[0][2] - x[1][2]) - (x[2][2] - x[3][2]));
    if (d > w)
      w = d;
  }
  return verdict(sz, w, 1e-12, "AU");
}

int main(int argc, char **argv)
{
  static const struct { int ipl; const char *sz; } rgf[] = {
    {SE_CUPIDO, "Cupido helio vs Swiss files"}, {SE_ISIS, "Isis helio vs Swiss files"},
    {SE_FICT_OFFSET + 15, "Vulcan helio vs Swiss files"},
  };
  static const char *rgs[3] = {"Sirius", "Spica", "Polaris"};
  static const struct { int32 fl; const char *sz; double lim; } rgsf[] = {
    {SEFLG_NOABERR, "noaberr", 1e-9}, {SEFLG_TRUEPOS, "true", 1e-9},
    {SEFLG_HELCTR, "helio", 1e-9}, {0, "apparent", 3e-8},
  };
  static const int rgd[11] = {SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER,
                              SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO, SE_CHIRON};
  static const int rgr[4] = {SE_SUN, SE_MOON, SE_MARS, SE_PLUTO};
  char sz[AS_MAXCH + 32], nm[AS_MAXCH];
  int i, j, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: moshbary EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G108: Moshier's fictitious bodies, stars, apparent distances and helio Moon see the barycentre\n");
  for (i = 0; i < 3; i++)
    bad += fict(rgf[i].ipl, rgf[i].sz);
  bad += orbel("orbel 0.387 AU helio vs Swiss files");
  for (i = 0; i < 3; i++)
    for (j = 0; j < 4; j++) {
      snprintf(sz, sizeof sz, "%s %s vs Swiss files", rgs[i], rgsf[j].sz);
      bad += star(rgs[i], rgsf[j].fl, sz, rgsf[j].lim);
    }
  bad += rate(SE_MOON, SEFLG_HELCTR, 0, 2, "Moon helio speed", 1e-9, "deg/day");
  bad += rate(SE_MOON, SEFLG_HELCTR, 2, 1, "Moon helio distance rate", 1e-11, "AU/day");
  bad += rate(SE_MOON, SEFLG_NOABERR, 0, 2, "Moon noaberr speed", 5e-9, "deg/day");
  for (i = 0; i < 11; i++) {
    swe_get_planet_name(rgd[i], nm);
    snprintf(sz, sizeof sz, "%s app - noaberr distance", nm);
    bad += apdist(rgd[i], sz);
  }
  for (i = 0; i < 4; i++) {
    swe_get_planet_name(rgr[i], nm);
    snprintf(sz, sizeof sz, "%s apparent distance rate", nm);
    bad += rate(rgr[i], 0, 2, 1, sz, 1e-11, "AU/day");
  }
  printf(bad ? "G108 FAIL: %d\n" : "G108 PASS\n", bad);
  return bad ? 1 : 0;
}
