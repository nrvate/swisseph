/* geoltearth.c -- G95: an Earth-centred fictitious body's place on Moshier
 * is as smooth as on the files, and its speed is the rate of that place.
 *
 * The defect (UPSTREAM-BUGS 96): an Earth-centred set (seorbel.txt's
 * "geo", or swe_calc_orbel() about the Earth) is seen from the Earth at
 * teval with its focus at the Earth at the emission instant t, so its
 * geocentric place holds E(t) - E(teval), a few 1e-7 AU, as the difference
 * of two reads of the Earth 4e-5 day apart. Each Moshier read is off a
 * smooth curve by its series' rounding, and the two do not share it: the
 * fourth difference over 2^-10 day of Waldemath's place, and of an orbit
 * like it, reached 8.9e-14 and 1.0e-13 AU (the place 3e-15 AU rms off a
 * smooth curve, 2.7e-11 deg), Selena's 8.0e-14, where the same bodies
 * without light time are smooth to <= 1.3e-15 and the Swiss files give
 * 7-9e-15. A nine-point difference over 2^-10 day read that as 3.4e-8
 * deg/day of speed. The Earth at t is now the Earth at teval moved back
 * along its own motion, below a light time of 2^-10 day.
 *
 * The check, Moshier, nine dates 1800-2150, Waldemath and Selena through
 * swe_calc() (seorbel.txt on the path) and an Earth-centred orbit through
 * swe_calc_orbel() (a = 0.00684 AU):
 *  (a) jitter: the largest fourth difference over 2^-10 day of the
 *      geocentric equatorial J2000 XYZ place, 64 instants a date, apparent
 *      and without aberration: within 2e-14 AU (measured <= 3.0e-15, what
 *      the 1 AU vectors' own rounding leaves; before 1.0e-13). The smooth
 *      part of the fourth difference is below 2e-19 AU for these orbits;
 *      topocentric places are left out, where the site's turning puts
 *      4e-14 AU of curvature into it.
 *  (b) speed: longitude, latitude and distance rate (per unit distance) in
 *      the ecliptic of date, apparent, against a nine-point difference of
 *      the positions over 2^-10 day (the orbit's elements moved by their
 *      mean motion): within 5e-9 deg/day (measured <= 1.7e-9; before
 *      7.2e-8 on the orbit, 3.4e-8 on Waldemath, 6.3e-9 on Selena; the
 *      Swiss files 4.4e-9).
 * The build before the fix fails all nine checks, the SWE_UPSTREAM_COMPAT
 * build too. Run as
 *   geoltearth EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define LIM_JIT 2e-14		/* AU */
#define LIM_SPEED 5e-9

static const double el[6] = {50, 0.00684, 0.1587, 8, 136, 2.5};
static const struct { int ipl; const char *sz; } rb[3] = {
  {SE_WALDEMATH, "Waldemath"}, {SE_WHITE_MOON, "Selena"}, {-1, "orbel, Earth, 0.0068 AU"},
};

static int calc(int ib, double t, int32 fl, double *x, char *serr)
{
  if (rb[ib].ipl < 0)
    return swe_calc_orbel(t, el, SE_EARTH, SE_ORBEL_EQ_J2000, 0, fl, x, serr);
  return swe_calc(t, rb[ib].ipl, fl, x, serr);
}

/* the same at t + dh, the orbit's elements moved by their mean motion
 * (seorbel.txt's are polynomials in the date already) */
static int calc_moved(int ib, double t, double dh, int32 fl, double *x, char *serr)
{
  if (rb[ib].ipl < 0) {
    const double k = 0.0000298122353216;	/* KGAUSS_GEO, AU^1.5 / day */
    double e[6];
    int i;
    for (i = 0; i < 6; i++)
      e[i] = el[i];
    e[0] += k / (el[1] * sqrt(el[1])) * 180 / M_PI * dh;
    return swe_calc_orbel(t + dh, e, SE_EARTH, SE_ORBEL_EQ_J2000, 0, fl, x, serr);
  }
  return swe_calc(t + dh, rb[ib].ipl, fl, x, serr);
}

static double date(int k)
{
  return floor((2378496.5 + k * 16000.3 + 0.137) * 256) / 256;
}

int main(int argc, char **argv)
{
  static const double c9[4] = {4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280};
  const double h = 1.0 / 1024;
  char serr[256];
  int ib, k, m, i, j, f, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: geoltearth EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G95: Earth-centred fictitious bodies on Moshier: smooth places, speeds their rate\n");
  for (ib = 0; ib < 3; ib++) {
    /* (a) jitter */
    for (f = 0; f < 2; f++) {
      int32 fl = SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_EQUATORIAL | SEFLG_XYZ
               | (f ? SEFLG_NOABERR : 0);
      double w = 0;
      for (k = 0; k < 9; k++) {
        double x[68][6];
        for (m = 0; m < 68; m++)
          if (calc(ib, date(k) + m * h, fl, x[m], serr) < 0) {
            printf("  %s: ERROR %s\n", rb[ib].sz, serr);
            return 1;
          }
        for (m = 2; m < 66; m++) {
          for (i = 0; i < 3; i++) {
            double d4 = x[m-2][i] - 4 * x[m-1][i] + 6 * x[m][i] - 4 * x[m+1][i] + x[m+2][i];
            if (fabs(d4) > w)
              w = fabs(d4);
          }
        }
      }
      printf("  jitter, %-26s %-12s %.2e AU (limit %.0e)\n", rb[ib].sz,
             f ? "no aberr." : "apparent", w, LIM_JIT);
      if (!(w <= LIM_JIT)) {
        printf("    FAIL: the place is not smooth\n");
        bad++;
      }
    }
    /* (b) speed */
    {
      double w[3] = {0, 0, 0};
      for (k = 0; k < 9; k++) {
        double t = date(k), x[6], d[9][6];
        if (calc(ib, t, SEFLG_MOSEPH | SEFLG_SPEED, x, serr) < 0) {
          printf("  %s: ERROR %s\n", rb[ib].sz, serr);
          return 1;
        }
        for (j = 0; j < 9; j++)
          calc_moved(ib, t, (j - 4) * h, SEFLG_MOSEPH, d[j], serr);
        for (i = 0; i < 3; i++) {
          double v[9], r = 0, s;
          for (j = 0; j < 9; j++)
            v[j] = i == 0 ? d[4][0] + swe_difdeg2n(d[j][0], d[4][0]) : d[j][i];
          for (j = 1; j <= 4; j++)
            r += c9[j - 1] * (v[4 + j] - v[4 - j]);
          r /= h;
          s = fabs(x[3 + i] - r);
          if (i == 2)
            s /= x[2];
          if (s > w[i])
            w[i] = s;
        }
      }
      printf("  speed,  %-26s %.2e %.2e %.2e (limit %.0e)\n", rb[ib].sz, w[0], w[1], w[2], LIM_SPEED);
      if (!(w[0] <= LIM_SPEED && w[1] <= LIM_SPEED && w[2] <= LIM_SPEED)) {
        printf("    FAIL: the speed is not the rate of the place\n");
        bad++;
      }
    }
  }
  printf(bad ? "G95 FAIL: %d\n" : "G95 PASS\n", bad);
  return bad ? 1 : 0;
}
