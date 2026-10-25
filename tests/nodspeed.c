/* nodspeed.c -- G42: swe_nod_aps()'s speeds are the rates of its own
 * positions, through the ecliptic of date's turning and for the mean
 * apsides' latitude.
 *
 * Two upstream defects in swe_nod_aps() (UPSTREAM-BUGS 37, 38):
 *
 * - The nodes and apsides are built in the ecliptic of date -- the
 *   osculating ones by swi_plan_for_osc_elem() and differenced there, the
 *   mean ones from elements of the mean ecliptic -- so their speeds carry
 *   its turning (the obliquity's rate, bug 33's subject) about the
 *   HELIOCENTRIC point. The conversion to the equator left it in, and the
 *   conversion back into the ecliptic, on the GEOCENTRIC point, put none
 *   back: what survived was de/dt x (the Earth's heliocentric position),
 *   up to 1.6e-5 deg/day on the latitude and declination speeds of every
 *   geocentric node and apsis of a planet (and 6e-6 on the right ascension
 *   speed); nothing heliocentric or for the Moon, where the two cancel.
 *
 * - The mean perihelion's and aphelion's latitude speed was never taken:
 *   the aux. position's latitude, a day on, went out in its place, so the
 *   speed was the latitude itself (1.8 deg/day heliocentric for Mars,
 *   against a true 1e-6), and through the cartesian state the geocentric
 *   longitude speed carried 0.01-0.04 deg/day of it.
 *
 * The check: the longitude and latitude (right ascension and declination)
 * speeds of the ascending node, the descending node, the perihelion and
 * the aphelion of Mars, Venus and Jupiter, osculating and mean, ecliptic
 * and equatorial, against a five-point difference of the library's own
 * positions (h = 0.01 day) at seven dates -200..+200 years, Swiss files.
 * Latitude and declination within 2.5e-7 deg/day (measured <= 9e-8),
 * longitude and right ascension within 1e-6 (measured <= 1.7e-7: the
 * apparent speed's aberration and deflection terms are approximate, and
 * Venus's osculating perihelion wobbles at the 1e-6 level on some dates
 * however computed, its eccentricity being 0.007). The dates fall 285
 * days into the year so that no node or apsis is within 3 degrees of the
 * Sun: within 0.05 degree of it, the light deflection's speed is off by
 * 1.7e-2 deg/day in either build (Jupiter's descending node on 1 January).
 * The upstream build fails every case, by 1.3e-5 to 3.9 deg/day.
 * SEFLG_J2000 is checked by G47 (tests/nodaps2.c, UPSTREAM-BUGS 47). Run as
 *   nodspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static const char *rgPt[4] = {"asc node", "dsc node", "perihelion", "aphelion"};

static int nodaps(swe_ctx *c, double jd, int ipl, int32 fl, int32 method, double x[4][6])
{
  char serr[256];
  return swe_nod_aps_r(c, jd, ipl, fl, method, x[0], x[1], x[2], x[3], serr);
}

int main(int argc, char **argv)
{
  struct { int ipl; const char *sz; } rgPl[] = {
    {SE_MARS, "Mars"}, {SE_VENUS, "Venus"}, {SE_JUPITER, "Jupiter"},
  };
  struct { int32 method; const char *sz; } rgMeth[] = {
    {SE_NODBIT_OSCU, "osculating"}, {SE_NODBIT_MEAN, "mean"},
  };
  struct { int32 fl; const char *sz; } rgFr[] = {
    {0, "ecliptic"}, {SEFLG_EQUATORIAL, "equatorial"},
  };
  const double lim[2] = {1e-6, 2.5e-7};        /* lon/RA, lat/dec */
  int p, m, f, y, j, k, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: nodspeed EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  printf("G42: swe_nod_aps speeds are the rates of its positions\n");
  for (p = 0; p < 3; p++) {
    for (m = 0; m < 2; m++) {
      for (f = 0; f < 2; f++) {
        int32 fl = SEFLG_SWIEPH | rgFr[f].fl;
        double w[2] = {0, 0};
        int wj[2] = {0, 0}, fail = 0;
        char sz[48];
        for (y = 0; y < 7; y++) {
          double jd = 2451545.0 + (-200 + y * 66.0) * 365.25 + 285.137;
          double x[4][6], d[5][4][6];
          if (nodaps(c, jd, rgPl[p].ipl, fl | SEFLG_SPEED, rgMeth[m].method, x) < 0) {
            printf("  %-28s ERROR\n", rgPl[p].sz);
            bad++;
            break;
          }
          for (k = 0; k < 5; k++)
            nodaps(c, jd + (k - 2) * 0.01, rgPl[p].ipl, fl, rgMeth[m].method, d[k]);
          for (j = 0; j < 4; j++) {
            int i;
            for (i = 0; i < 2; i++) {
              /* the five positions, unwrapped about the middle one */
              double v[5], r, e;
              for (k = 0; k < 5; k++) {
                v[k] = d[k][j][i];
                if (i == 0)
                  v[k] = d[2][j][0] + swe_difdeg2n(v[k], d[2][j][0]);
              }
              r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / 0.12;
              e = fabs(x[j][3 + i] - r);
              if (e > w[i]) { w[i] = e; wj[i] = j; }
            }
          }
        }
        snprintf(sz, sizeof sz, "%s, %s, %s", rgPl[p].sz, rgMeth[m].sz, rgFr[f].sz);
        printf("  %-30s %s %.2e (%s) %s %.2e (%s) deg/day\n", sz,
               f ? "RA " : "lon", w[0], rgPt[wj[0]],
               f ? "dec" : "lat", w[1], rgPt[wj[1]]);
        if (w[0] > lim[0]) {
          printf("    FAIL: %s speed off by more than %.0e\n", f ? "RA" : "longitude", lim[0]);
          fail++;
        }
        if (w[1] > lim[1]) {
          printf("    FAIL: %s speed off by more than %.0e\n", f ? "declination" : "latitude", lim[1]);
          fail++;
        }
        bad += fail;
      }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G42 FAIL: %d\n" : "G42 PASS\n", bad);
  return bad ? 1 : 0;
}
