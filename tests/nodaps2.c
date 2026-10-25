/* nodaps2.c -- G47: swe_nod_aps()'s positions do not depend on
 * SEFLG_SPEED, and under SEFLG_J2000 they are the J2000 frame of the same
 * points, with speeds that are their rate.
 *
 * Two upstream defects in swe_nod_aps() (UPSTREAM-BUGS 46, 47):
 *
 * - With SEFLG_SPEED the apparent speed's aberration term computes the
 *   body at tjd - dt, dt each point's light time, which leaves the Sun and
 *   the Earth in the save area at tjd - dt, and then "restores" them by
 *   computing the Sun at tjd with the body's internal flags. Those carry
 *   SEFLG_HELCTR for every body but the Moon, and a heliocentric Sun is
 *   zeros, returned before anything is computed; for the Moon the call
 *   finds the setup's Sun in swe_calc()'s save area. The first point was
 *   right, and the deflection and change of centre of the other three used
 *   the Earth of the point before: Mars's descending node, perihelion and
 *   aphelion were 6-35 mas from where they are without SEFLG_SPEED, the
 *   Sun's mean aphelion 10, Venus's osculating focal point 660.
 *
 * - SEFLG_J2000 did not set SEFLG_NONUT as swe_calc() does, and the
 *   points, in the ecliptic of date, were turned to the equator by the
 *   obliquity of J2000: the Moon's mean node was 7.8" in longitude and
 *   6.9" in latitude from swe_calc(SE_MEAN_NODE, J2000) on 2026-10-24,
 *   148" in latitude in 2326. The speeds had none of the frame's turning
 *   that bug 37 accounts for of date: Mars's osculating points'
 *   declination speeds 3.2e-5 deg/day from the rate of their positions.
 *
 * The check, Swiss files unless said:
 *  A. The four points with and without SEFLG_SPEED: the Sun, the Moon and
 *     Mercury..Pluto; mean, osculating, barycentric osculating and focal
 *     point; geocentric, heliocentric, barycentric, topocentric,
 *     equatorial and J2000; five dates -3000..2100, and Mars at 1800 with
 *     de200.eph. Within 1e-4 mas (measured <= 8e-6, the Moon's osculating
 *     points; <= 6e-7 for the rest; 0 under J2000): rounding, through
 *     swi_cartpol_sp()'s and swi_precess_speed()'s own formulas for the
 *     position. Moshier's Moon's focal point, 0.0003 AU from the Earth,
 *     moves by 4e-5.
 *  B. J2000 against swe_calc(): the Moon's mean node and mean apogee
 *     against SE_MEAN_NODE and SE_MEAN_APOG under SEFLG_J2000, ecliptic
 *     and equatorial, 21 dates -300..+300 years: within 1e-7" (measured
 *     4.6e-9"). The osculating ones are made by another method in
 *     swe_calc() and differ by up to 0.5" of date; the angle between the
 *     two is a property of the points, not of the frame, so it must be the
 *     same in J2000 as in the mean ecliptic of date (SEFLG_NONUT): within
 *     1e-6" (measured 2.8e-10"). Venus, Mars and Jupiter, geocentric, mean
 *     and osculating, the same for the angle between the ascending node
 *     and each other point (measured 4.7e-10").
 *  C. J2000 speeds, ecliptic and equatorial: the four points of Mars,
 *     Venus and Jupiter, osculating and mean, and the Moon's mean nodes,
 *     against a five-point difference of the library's own positions at
 *     binary-fraction instants (h = 2^-6 day) at seven dates -200..+200
 *     years: latitude and declination within 2.5e-7 deg/day (measured
 *     <= 1.4e-7), longitude and right ascension within 1e-6 (<= 3.6e-7,
 *     Venus's osculating perihelion, as of date in G42). The Moon's mean
 *     apsides are left out (a one-day difference, 2e-5 on the latitude
 *     speed of date as well), and so are its osculating points (three
 *     positions NODE_CALC_INTV apart, 2e-5 of date as well).
 * The upstream build fails 34 of the 35 rows (43 checks): A by 32-620
 * mas (6e8 on the topocentric Earth, whose save area it leaves unset), B
 * by 0.5-152", C by up to 2.6 deg/day on the mean points (bug 38 under
 * it) and 3e-5 on the osculating ones; the Moon's mean nodes' equatorial
 * speeds pass. Run as
 *   nodaps2 EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define D2R 0.017453292519943295

static int nodaps(swe_ctx *c, double jd, int ipl, int32 fl, int32 method, double x[4][6])
{
  char serr[256];
  return swe_nod_aps_r(c, jd, ipl, fl, method, x[0], x[1], x[2], x[3], serr);
}

/* angle between two directions given as lon/lat in degrees, arcsec */
static double sep(const double *a, const double *b)
{
  double ua[3], ub[3], cr[3], dot;
  ua[0] = cos(a[1] * D2R) * cos(a[0] * D2R);
  ua[1] = cos(a[1] * D2R) * sin(a[0] * D2R);
  ua[2] = sin(a[1] * D2R);
  ub[0] = cos(b[1] * D2R) * cos(b[0] * D2R);
  ub[1] = cos(b[1] * D2R) * sin(b[0] * D2R);
  ub[2] = sin(b[1] * D2R);
  cr[0] = ua[1] * ub[2] - ua[2] * ub[1];
  cr[1] = ua[2] * ub[0] - ua[0] * ub[2];
  cr[2] = ua[0] * ub[1] - ua[1] * ub[0];
  dot = ua[0] * ub[0] + ua[1] * ub[1] + ua[2] * ub[2];
  return atan2(sqrt(cr[0] * cr[0] + cr[1] * cr[1] + cr[2] * cr[2]), dot) / D2R * 3600;
}

/* A: the largest move of any point with SEFLG_SPEED, mas */
static double speed_flag_move(swe_ctx *c, double jd, int ipl, int32 fl, int32 method, int *ok)
{
  double a[4][6], b[4][6], w = 0;
  int j;
  *ok = 0;
  if (nodaps(c, jd, ipl, fl | SEFLG_SPEED, method, a) < 0)
    return 0;
  if (nodaps(c, jd, ipl, fl, method, b) < 0)
    return 0;
  *ok = 1;
  for (j = 0; j < 4; j++) {
    double e;
    if (ipl == SE_SUN && j < 2)
      continue;     /* the Earth has no nodes */
    e = sep(a[j], b[j]) * 1000;
    if (e > w) w = e;
  }
  return w;
}

int main(int argc, char **argv)
{
  static const int rgPl[11] = {SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS,
    SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO, SE_EARTH};
  static const int32 rgMeth[4] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU,
    SE_NODBIT_OSCU_BAR, SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
  static const int32 rgFl[6] = {0, SEFLG_HELCTR, SEFLG_BARYCTR, SEFLG_TOPOCTR,
    SEFLG_EQUATORIAL, SEFLG_J2000};
  static const char *rgszFl[6] = {"geocentric", "heliocentric", "barycentric",
    "topocentric", "equatorial", "J2000"};
  static const double rgYearA[5] = {-3000, 1, 1800, 2026.8, 2100};
  char serr[256];
  int p, m, f, y, j, k, i, n, ok, bad = 0;
  double w, wmax;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: nodaps2 EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_topo_r(c, 8.55, 47.37, 400);
  printf("G47: swe_nod_aps positions do not depend on SEFLG_SPEED; J2000 is the J2000 frame\n");

  /* A */
  for (f = 0; f < 6; f++) {
    int wp = 0, wm = 0;
    wmax = 0;
    n = 0;
    for (p = 0; p < 11; p++)
      for (m = 0; m < 4; m++)
        for (y = 0; y < 5; y++) {
          double jd = 2451545.0 + (rgYearA[y] - 2000) * 365.25 + 0.1234567;
          w = speed_flag_move(c, jd, rgPl[p], SEFLG_SWIEPH | rgFl[f], rgMeth[m], &ok);
          n += ok;
          if (w > wmax) { wmax = w; wp = p; wm = m; }
        }
    printf("  A %-13s %3d cases: points move with SEFLG_SPEED by %.2e mas (body %d, method %d)\n",
           rgszFl[f], n, wmax, rgPl[wp], rgMeth[wm]);
    if (n < 150) {
      printf("    FAIL: only %d cases computed\n", n);
      bad++;
    }
    if (wmax > 1e-4) {
      printf("    FAIL: more than 1e-4 mas\n");
      bad++;
    }
  }
  swe_set_jpl_file_r(c, "de200.eph");
  wmax = 0;
  n = 0;
  for (m = 0; m < 4; m++)
    for (f = 0; f < 6; f++) {
      w = speed_flag_move(c, 2378496.5 + 100.1234567, SE_MARS, SEFLG_JPLEPH | rgFl[f], rgMeth[m], &ok);
      n += ok;
      if (w > wmax) wmax = w;
    }
  printf("  A de200.eph Mars 1800 %2d cases: %.2e mas\n", n, wmax);
  if (n < 24 || wmax > 1e-4) {
    printf("    FAIL: %d of 24 cases, more than 1e-4 mas\n", n);
    bad++;
  }

  /* B: the Moon's mean points against swe_calc() under SEFLG_J2000 */
  {
    static const struct { int ipl, pt; int32 meth; const char *sz; } rgQ[4] = {
      {SE_MEAN_NODE, 0, SE_NODBIT_MEAN, "mean node"},
      {SE_MEAN_APOG, 3, SE_NODBIT_MEAN, "mean apogee"},
      {SE_TRUE_NODE, 0, SE_NODBIT_OSCU, "true node"},
      {SE_OSCU_APOG, 3, SE_NODBIT_OSCU, "osculating apogee"},
    };
    double jd0 = swe_julday(2026, 10, 24, 0, SE_GREG_CAL);
    for (k = 0; k < 4; k++) {
      for (f = 0; f < 2; f++) {
        int32 fr = f ? SEFLG_EQUATORIAL : 0;
        double wl = 0, wb = 0, ws = 0;
        for (y = -10; y <= 10; y++) {
          double jd = jd0 + y * 30 * 365.25, a[4][6], x[6], aD[4][6], xD[6];
          nodaps(c, jd, SE_MOON, SEFLG_SWIEPH | SEFLG_J2000 | fr, rgQ[k].meth, a);
          swe_calc_r(c, jd, rgQ[k].ipl, SEFLG_SWIEPH | SEFLG_J2000 | fr, x, serr);
          if (k < 2) {
            double dl = fabs(swe_difdeg2n(a[rgQ[k].pt][0], x[0])) * cos(x[1] * D2R) * 3600;
            double db = fabs(a[rgQ[k].pt][1] - x[1]) * 3600;
            if (dl > wl) wl = dl;
            if (db > wb) wb = db;
          } else {
            nodaps(c, jd, SE_MOON, SEFLG_SWIEPH | SEFLG_NONUT | fr, rgQ[k].meth, aD);
            swe_calc_r(c, jd, rgQ[k].ipl, SEFLG_SWIEPH | SEFLG_NONUT | fr, xD, serr);
            w = fabs(sep(a[rgQ[k].pt], x) - sep(aD[rgQ[k].pt], xD));
            if (w > ws) ws = w;
          }
        }
        if (k < 2) {
          printf("  B Moon %-17s %s J2000 vs swe_calc: %.2e\" %.2e\"\n", rgQ[k].sz,
                 f ? "equ" : "ecl", wl, wb);
          if (wl > 1e-7 || wb > 1e-7) {
            printf("    FAIL: more than 1e-7\"\n");
            bad++;
          }
        } else {
          printf("  B Moon %-17s %s angle to swe_calc's, J2000 less of date: %.2e\"\n",
                 rgQ[k].sz, f ? "equ" : "ecl", ws);
          if (ws > 1e-6) {
            printf("    FAIL: more than 1e-6\"\n");
            bad++;
          }
        }
      }
    }
    /* the planets: the angles between the points, J2000 and of date */
    for (p = 3; p <= 5; p++) {  /* Venus, Mars, Jupiter */
      for (m = 0; m < 2; m++) {
        double ws = 0;
        for (y = -10; y <= 10; y++) {
          double jd = jd0 + y * 30 * 365.25, a[4][6], aD[4][6];
          nodaps(c, jd, rgPl[p], SEFLG_SWIEPH | SEFLG_J2000, rgMeth[m], a);
          nodaps(c, jd, rgPl[p], SEFLG_SWIEPH | SEFLG_NONUT, rgMeth[m], aD);
          for (j = 1; j < 4; j++) {
            w = fabs(sep(a[0], a[j]) - sep(aD[0], aD[j]));
            if (w > ws) ws = w;
          }
        }
        printf("  B %-7s %-10s angles between the points, J2000 less of date: %.2e\"\n",
               p == 3 ? "Venus" : p == 4 ? "Mars" : "Jupiter",
               m ? "osculating" : "mean", ws);
        if (ws > 1e-6) {
          printf("    FAIL: more than 1e-6\"\n");
          bad++;
        }
      }
    }
  }

  /* C: J2000 speeds against five-point differences */
  {
    static const struct { int ipl; const char *sz; } rgC[4] = {
      {SE_MARS, "Mars"}, {SE_VENUS, "Venus"}, {SE_JUPITER, "Jupiter"}, {SE_MOON, "Moon"},
    };
    const double lim[2] = {1e-6, 2.5e-7}, h = 1.0 / 64;
    for (p = 0; p < 4; p++) {
      for (m = 0; m < 2; m++) {
        if (rgC[p].ipl == SE_MOON && m == 1)
          continue;  /* osculating: three points NODE_CALC_INTV apart, also of date */
        for (f = 0; f < 2; f++) {
          int32 fl = SEFLG_SWIEPH | SEFLG_J2000 | (f ? SEFLG_EQUATORIAL : 0);
          double wv[2] = {0, 0};
          int npt = rgC[p].ipl == SE_MOON ? 2 : 4;  /* the Moon's mean apsides: G42's note */
          for (y = 0; y < 7; y++) {
            double jd = 2451545.0 + (-200 + y * 66.0) * 365.25 + 285.137;
            double x[4][6], d[5][4][6];
            jd = floor(jd * 256) / 256;
            if (nodaps(c, jd, rgC[p].ipl, fl | SEFLG_SPEED, rgMeth[m], x) < 0) {
              printf("  C %s ERROR\n", rgC[p].sz);
              bad++;
              break;
            }
            for (k = 0; k < 5; k++)
              nodaps(c, jd + (k - 2) * h, rgC[p].ipl, fl, rgMeth[m], d[k]);
            for (j = 0; j < npt; j++)
              for (i = 0; i < 2; i++) {
                double v[5], r, e;
                for (k = 0; k < 5; k++) {
                  v[k] = d[k][j][i];
                  if (i == 0)
                    v[k] = d[2][j][0] + swe_difdeg2n(v[k], d[2][j][0]);
                }
                r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
                e = fabs(x[j][3 + i] - r);
                if (e > wv[i]) wv[i] = e;
              }
          }
          printf("  C %-7s %-10s J2000 %s %s %.2e %s %.2e deg/day\n", rgC[p].sz,
                 m ? "osculating" : "mean", f ? "equ" : "ecl",
                 f ? "RA " : "lon", wv[0], f ? "dec" : "lat", wv[1]);
          for (i = 0; i < 2; i++)
            if (wv[i] > lim[i]) {
              printf("    FAIL: more than %.1e\n", lim[i]);
              bad++;
            }
        }
      }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G47 FAIL: %d\n" : "G47 PASS\n", bad);
  return bad ? 1 : 0;
}
