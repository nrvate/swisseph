/* topopole.c -- G48: the topocentric observer turns about the TRUE pole of
 * date.
 *
 * swi_get_observer() builds the site from the geodetic coordinates and the
 * sidereal time, then carries it to J2000. Every caller passed it
 * SEFLG_NONUT, under which it used the MEAN sidereal time and the mean
 * obliquity, skipped its own nutation step, and precessed a vector that was
 * never on the true equator: the site was placed relative to the mean pole
 * of date, which is up to 9" (the nutation in obliquity) and 17" x sin(eps)
 * (the nutation in longitude) from the pole the Earth actually turns about.
 * 6378 km x that angle is 200-270 m, and 200-270 m seen from the Moon is
 * 0.10-0.15": measured against Ephemeris Prometheia on the same delta t and
 * against JPL Horizons at six sites and six instants 1800-2100 (2026-10-24;
 * 10-148 mas, every one of them explained by the site offset to ~1 mas).
 * Planets: under 1 mas.
 *
 * The check needs no reference ephemeris. The geocentric and topocentric
 * GEOMETRIC Moon (SEFLG_TRUEPOS, so no light time enters) in the true
 * equator and equinox of date differ by exactly the observer's position in
 * that frame; that vector must equal the site built here from the geodetic
 * coordinates and the APPARENT sidereal time, to 5 cm (the frame bias the
 * site is carried through is 45 cm). The defective library misses by
 * 200-270 m. Moshier, no files. Run as
 *   topopole
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"   /* EARTH_RADIUS, EARTH_OBLATENESS, AUNIT: the library's own figure */

struct site { const char *name; double lon, lat, alt; };

int main(void)
{
  static const struct site sites[] = {
    { "Zurich",       8.55,   47.37,   400.0 },
    { "Longyearbyen", 15.63,  78.22,    20.0 },
    { "Quito",       -78.5,   -0.2,   2850.0 },
    { "Santiago",    -70.6,  -33.4,    520.0 },
    { "89N",           0.0,   89.0,      0.0 } };
  static const double dates[] = { 2378496.5, 2415020.5, 2451545.0, 2461300.5, 2488069.5 };
  int32 geom = SEFLG_MOSEPH | SEFLG_TRUEPOS | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_NOABERR | SEFLG_NOGDEFL;
  double worst = 0.0;
  int bad = 0;
  size_t is, id;
  char serr[AS_MAXCH];

  printf("G48: the topocentric observer turns about the true pole of date\n");
  for (is = 0; is < sizeof sites / sizeof *sites; is++) {
    swe_set_topo(sites[is].lon, sites[is].lat, sites[is].alt);
    for (id = 0; id < sizeof dates / sizeof *dates; id++) {
      double tjd = dates[id], xg[6], xt[6], xn[6], ut, gast, cosfi, sinfi, cc, ss, lam, ref[3], err;
      double f = EARTH_OBLATENESS, re = EARTH_RADIUS;
      int i;
      if (swe_calc(tjd, SE_MOON, geom, xg, serr) < 0 ||
          swe_calc(tjd, SE_MOON, geom | SEFLG_TOPOCTR, xt, serr) < 0 ||
          swe_calc(tjd, SE_ECL_NUT, SEFLG_MOSEPH, xn, serr) < 0) {
        printf("  %-13s %10.1f ERROR %s\n", sites[is].name, tjd, serr);
        bad++;
        continue;
      }
      /* the observer the library used, in the true equator and equinox of
       * date, metres */
      for (i = 0; i < 3; i++)
        xg[i] = (xg[i] - xt[i]) * AUNIT;
      /* the site built here: UT from the same delta t the library takes
       * (it hands swe_deltat_ex() the TT instant), the apparent sidereal
       * time from the library's own true obliquity and nutation in
       * longitude, and the library's ellipsoid */
      ut = tjd - swe_deltat_ex(tjd, SEFLG_MOSEPH, serr);
      gast = swe_sidtime0(ut, xn[0], xn[2]) * 15.0;
      cosfi = cos(sites[is].lat * DEGTORAD);
      sinfi = sin(sites[is].lat * DEGTORAD);
      cc = 1 / sqrt(cosfi * cosfi + (1 - f) * (1 - f) * sinfi * sinfi);
      ss = (1 - f) * (1 - f) * cc;
      lam = (sites[is].lon + gast) * DEGTORAD;
      ref[0] = (re * cc + sites[is].alt) * cosfi * cos(lam);
      ref[1] = (re * cc + sites[is].alt) * cosfi * sin(lam);
      ref[2] = (re * ss + sites[is].alt) * sinfi;
      err = sqrt((xg[0]-ref[0])*(xg[0]-ref[0]) + (xg[1]-ref[1])*(xg[1]-ref[1]) + (xg[2]-ref[2])*(xg[2]-ref[2]));
      printf("  %-13s %10.1f  observer off its true-pole place by %9.4f m%s\n",
             sites[is].name, tjd, err, err > 0.05 ? "   FAIL" : "");
      if (err > worst) worst = err;
      if (err > 0.05) bad++;
    }
  }
  swe_close();
  printf("  worst %.4f m (limit 0.05 m)\n", worst);
  printf(bad ? "G48 FAIL: %d\n" : "G48 PASS\n", bad);
  return bad ? 1 : 0;
}
