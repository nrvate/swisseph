/* nodapsgm.c -- G104: swe_nod_aps()'s osculating ellipses take the
 * ephemeris's own GM.
 *
 * The defect (UPSTREAM-BUGS 105): swe_nod_aps() fitted its osculating
 * ellipses (SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR) with the Astronomical
 * Almanac's constants, not the ephemeris's: the Moon's about G M_earth (1 +
 * 1 / EARTH_MOON_MRAT), 3.3e-8 above DE431's GMB (bug 28 fixed only
 * lunar_osc_elem(), SE_OSCU_APOG), the planets' about HELGRAVCONST /
 * AUNIT^3, 1.8e-10 below the Sun's GM in every JPL header, with plmass's
 * planets (DE200's Uranus and Neptune 2.5e-3 and 5e-3 off them). The
 * eccentricity vector turns by about dGM / (GM e) and the semi-axis moves
 * by dGM / GM: the Moon's osculating apogee was 4.9e-6 deg off SE_OSCU_APOG
 * on de200 (5.3e-5 on the Swiss files, with the frame difference below),
 * its distance 2e-10 AU off; Uranus's perihelion on de200 off by up to 1e-4
 * deg.
 *
 * The check: the Swiss files, de200 and Moshier, eight dates (-2500..2800,
 * de200 1700..2100), SEFLG_TRUEPOS.
 *   (a) The Moon: swe_nod_aps()'s osculating apogee against SE_OSCU_APOG,
 *       distance within 1e-14 AU on all three, longitude and latitude
 *       within 1e-9 deg on de200 (measured 1.4e-16 AU, 4.1e-11 deg).
 *   (b) Mercury..Pluto and the Earth (the EMB's orbit), heliocentric,
 *       J2000: the osculating perihelion against the eccentricity vector of
 *       swe_calc()'s own state with the header's GMS + GMi (or GMB; the
 *       Swiss files' header by their DE number, DE441 here),
 *       perihelion distance within 1e-12 AU on all three, direction within
 *       1e-9 deg on de200 (measured 6e-14 AU, 3.9e-12 deg).
 * On the Swiss files and Moshier the directions are not compared: there
 * swe_nod_aps() turns swe_calc()'s J2000 state by the frame bias once more
 * (5.5e-6 deg on the Moon's apogee; not this defect, UPSTREAM-BUGS 105's
 * notes), which de200, an ephemeris without one, does not.
 *
 * 30 rows. The build before the fix fails all 30; the upstream build
 * (SWE_UPSTREAM_COMPAT) 27 (its two GMs agree for the Moon). Run as
 *   nodapsgm EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define EPHBITS (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)
static const struct { int32 fl; const char *sz; } rgEph[3] = {
  {SEFLG_SWIEPH, "Swiss"}, {SEFLG_JPLEPH, "de200"}, {SEFLG_MOSEPH, "Moshier"},
};
static const double rgYear[8] = {-2500, -1000, 0, 1000, 1700, 1900, 2000, 2800};
static const double rgYearJ[8] = {1700, 1750, 1800, 1850, 1900, 1950, 2000, 2100};
/* GMS and GM1..GM9 (GMB third), AU^3/day^2, ssd.jpl.nasa.gov/ftp/eph/
 * planets/ascii, GROUP 1041: DE431, DE200, DE403 (Moshier, fitted to
 * DE404), DE440/441; the Swiss files take DE431's or DE441's by their DE
 * number */
static const double rgGm[4][10] = {
  {2.959122082855911e-4, 4.91248045036476e-11, 7.24345233264412e-10,
   8.997011390199871e-10, 9.54954869555077e-11, 2.82534584083387e-07,
   8.45970607324503e-08, 1.29202482578296e-08, 1.52435734788511e-08,
   2.17844105197418e-12},
  {2.959122082855911e-4, 4.912547451450812e-11, 7.243456209632766e-10,
   8.997011658557308e-10, 9.549528942224058e-11, 2.8253421034459264e-07,
   8.459468504830659e-08, 1.288816238138035e-08, 1.5321124812842762e-08,
   2.2762477518636993e-12},
  {2.959122082855911e-4, 4.912547451450812e-11, 7.243452486162703e-10,
   8.997011374291876e-10, 9.549535105779258e-11, 2.825345909524227e-07,
   8.459715185680659e-08, 1.29202491678197e-08, 1.524358900784276e-08,
   2.18869976542597e-12},
  {2.9591220828411956e-4, 4.912500194889318e-11, 7.243452332644119e-10,
   8.997011392947347e-10, 9.549548829725812e-11, 2.825345825225792e-07,
   8.45970599337629e-08, 1.29202656496824e-08, 1.524357347885194e-08,
   2.175096464893358e-12},
};
static const int rgPl[9] = {SE_MERCURY, SE_VENUS, SE_EARTH, SE_MARS, SE_JUPITER,
                            SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO};
static const char *rgPlName[9] = {"Mercury", "Venus", "Earth (EMB)", "Mars",
  "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"};

static double jd_of(int ie, int iy)
{
  double y = ie == 1 ? rgYearJ[iy] : rgYear[iy];
  return floor((2451545.0 + (y - 2000) * 365.25 + 0.37) * 256) / 256;
}

static int row(const char *sz, double dd, double dlim, double da, double alim)
{
  int bad = dd > dlim || (alim > 0 && da > alim);
  if (alim > 0)
    printf("  %-28s distance %.1e AU, direction %.1e deg%s\n", sz, dd, da,
           bad ? "  FAIL" : "");
  else
    printf("  %-28s distance %.1e AU%s\n", sz, dd, bad ? "  FAIL" : "");
  return bad;
}

int main(int argc, char **argv)
{
  char serr[AS_MAXCH], sz[64];
  int ie, iy, ip, j, ig = 0, bad = 0, nrow = 0;
  swe_ctx *c;
  if (argc != 2) {
    fprintf(stderr, "usage: nodapsgm EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_jpl_file_r(c, "de200.eph");
  printf("G104: swe_nod_aps()'s osculating ellipses take the ephemeris's GM\n");
  for (ie = 0; ie < 3; ie++) {
    int32 fl = rgEph[ie].fl | SEFLG_TRUEPOS;
    double wd = 0, wa = 0;
    /* (a) the Moon's apogee against SE_OSCU_APOG */
    for (iy = 0; iy < 8; iy++) {
      double t = jd_of(ie, iy), o[6], xn[6], xd[6], xp[6], xa[6], d;
      if ((swe_calc_r(c, t, SE_OSCU_APOG, fl, o, serr) & EPHBITS) != rgEph[ie].fl
          || swe_nod_aps_r(c, t, SE_MOON, fl, SE_NODBIT_OSCU, xn, xd, xp, xa, serr) < 0) {
        printf("  %s Moon ERROR at %.4f\n", rgEph[ie].sz, t);
        swe_ctx_free(c);
        return 1;
      }
      if (fabs(o[2] - xa[2]) > wd) wd = fabs(o[2] - xa[2]);
      d = fabs(swe_difdeg2n(o[0], xa[0]));
      if (fabs(o[1] - xa[1]) > d) d = fabs(o[1] - xa[1]);
      if (d > wa) wa = d;
    }
    snprintf(sz, sizeof sz, "%s Moon apogee", rgEph[ie].sz);
    bad += row(sz, wd, 1e-14, wa, ie == 1 ? 1e-9 : 0);
    nrow++;
    /* (b) the planets' perihelia against their own eccentricity vectors */
    if (ie == 0) {	/* the Swiss planet file's DE number */
      double t0, t1;
      int32 denum = 0;
      swe_get_current_file_data_r(c, 0, &t0, &t1, &denum);
      if (denum >= 440)
        ig = 3;
      printf("  Swiss planet file DE%d\n", denum);
    } else
      ig = ie;
    for (ip = 0; ip < 9; ip++) {
      double mu = rgGm[ig][0] + rgGm[ig][1 + ip];
      wd = wa = 0;
      for (iy = 0; iy < 8; iy++) {
        double t = jd_of(ie, iy), x[6], xm[6], xn[6], xd[6], xp[6], xa[6];
        double r, v2, rv, ev[3], e, a, q, lon, lat, d;
        int32 rf = swe_calc_r(c, t, rgPl[ip], fl | SEFLG_HELCTR | SEFLG_J2000
                              | SEFLG_XYZ | SEFLG_SPEED, x, serr);
        if ((rf & EPHBITS) != rgEph[ie].fl) {
          printf("  %s planet %d ERROR at %.4f\n", rgEph[ie].sz, rgPl[ip], t);
          swe_ctx_free(c);
          return 1;
        }
        if (rgPl[ip] == SE_EARTH) {	/* the Earth-Moon barycentre's orbit,
          * built as swe_nod_aps() builds it, with the AA's 1 / 0.0123000383
          * (EARTH_MOON_MRAT), not the ephemeris's EMRAT (UPSTREAM-BUGS 105's
          * notes) */
          swe_calc_r(c, t, SE_MOON, fl | SEFLG_J2000 | SEFLG_XYZ | SEFLG_SPEED, xm, serr);
          for (j = 0; j <= 5; j++)
            x[j] += xm[j] / (1 / 0.0123000383 + 1.0);
        }
        swe_nod_aps_r(c, t, rgPl[ip], fl | SEFLG_HELCTR | SEFLG_J2000,
                      SE_NODBIT_OSCU, xn, xd, xp, xa, serr);
        r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
        v2 = x[3] * x[3] + x[4] * x[4] + x[5] * x[5];
        rv = x[0] * x[3] + x[1] * x[4] + x[2] * x[5];
        for (j = 0; j < 3; j++)
          ev[j] = ((v2 - mu / r) * x[j] - rv * x[j+3]) / mu;
        e = sqrt(ev[0] * ev[0] + ev[1] * ev[1] + ev[2] * ev[2]);
        a = 1 / (2 / r - v2 / mu);
        q = a * (1 - e);
        lon = atan2(ev[1], ev[0]) * RADTODEG;
        lat = atan2(ev[2], sqrt(ev[0] * ev[0] + ev[1] * ev[1])) * RADTODEG;
        if (fabs(q - xp[2]) > wd) wd = fabs(q - xp[2]);
        d = fabs(swe_difdeg2n(lon, xp[0]));
        if (fabs(lat - xp[1]) > d) d = fabs(lat - xp[1]);
        if (d > wa) wa = d;
      }
      snprintf(sz, sizeof sz, "%s %s perihelion", rgEph[ie].sz, rgPlName[ip]);
      bad += row(sz, wd, 1e-12, wa, ie == 1 ? 1e-9 : 0);
      nrow++;
    }
  }
  swe_ctx_free(c);
  printf("G104: %d rows\n", nrow);
  printf(bad ? "G104 FAIL: %d\n" : "G104 PASS\n", bad);
  return bad ? 1 : 0;
}
