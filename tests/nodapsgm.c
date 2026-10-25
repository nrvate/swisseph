/* nodapsgm.c -- G104: swe_nod_aps()'s osculating ellipses take the
 * ephemeris's own GM, frame and Earth-Moon ratio.
 *
 * The defects. UPSTREAM-BUGS 105: swe_nod_aps() fitted its osculating
 * ellipses (SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR) with the Astronomical
 * Almanac's constants, not the ephemeris's: the Moon's about G M_earth (1 +
 * 1 / EARTH_MOON_MRAT), 3.3e-8 above DE431's GMB (bug 28 fixed only
 * lunar_osc_elem(), SE_OSCU_APOG), the planets' about HELGRAVCONST /
 * AUNIT^3, 1.8e-10 below the Sun's GM in every JPL header, with plmass's
 * planets (DE200's Uranus and Neptune 2.5e-3 and 5e-3 off them). The
 * eccentricity vector turns by about dGM / (GM e) and the semi-axis moves
 * by dGM / GM: the Moon's osculating apogee was 4.9e-6 deg off SE_OSCU_APOG
 * on de200, its distance 2e-10 AU off; Uranus's perihelion on de200 off by
 * up to 1e-4 deg. UPSTREAM-BUGS 108: on an ephemeris with a frame bias (DE
 * >= 403: the Swiss files, Moshier, DE4xx) swe_nod_aps() turned
 * swe_calc()'s J2000 state, already in the J2000 frame, by the bias once
 * more (the Moon's apogee 6.2e-6 deg off SE_OSCU_APOG in latitude, twice
 * that with SEFLG_ICRS); and it built the Earth-Moon barycentre with the
 * AA's ratio 1 / 0.0123000383 (EARTH_MOON_MRAT) on a JPL file too, whose
 * Earth comes from its own EMRAT (3.6e-7 deg on de200's EMB perihelion).
 * UPSTREAM-BUGS 180: on Moshier it added Moshier's Moon to an Earth made
 * from the EMB with the short lunar series' (embofs_mosh()), 0.24 deg away
 * at 2000: the EMB 1.3e-7 AU off the series', the perihelion 2.6e-2 deg
 * at most here, 5.1e-6 AU.
 *
 * The check: the Swiss files, de200 and Moshier, and a DE4xx JPL file if
 * one is named, eight dates (-2500..2800; JPL 1700..2100).
 *   (a) The Moon: swe_nod_aps()'s osculating apogee against SE_OSCU_APOG
 *       and its ascending node against SE_TRUE_NODE, SEFLG_TRUEPOS and
 *       SEFLG_ICRS | SEFLG_TRUEPOS, distance within 1e-14 AU, longitude
 *       and latitude within 1e-9 deg (measured 1.4e-16 AU, 4.1e-11 deg;
 *       the extra bias turned the node, on an orbit inclined 5 deg, by
 *       7.2e-5 deg and moved it 1.5e-10 AU, twice that with SEFLG_ICRS).
 *   (b) Mercury..Pluto and the Earth (the EMB's orbit), heliocentric,
 *       J2000, in both frames: the osculating perihelion against the
 *       eccentricity vector of swe_calc()'s own state with the header's
 *       GMS + GMi (or GMB; the Swiss files' header by their DE number,
 *       DE441 here), the EMB built with the file's EMRAT on a JPL file and
 *       EARTH_MOON_MRAT on the Swiss files and Moshier (their Earth is made
 *       from the EMB with it) and the Moon the Earth was made with (on
 *       Moshier the short series', which the context keeps), perihelion
 *       distance within 1e-12 AU and direction within 1e-9 deg (measured
 *       5.3e-14 AU, 5.9e-12 deg).
 *
 * 11 rows an ephemeris, 33 without a DE4xx file. The build before bug 108
 * fails 23 of the 33, all the Swiss and Moshier rows and de200's Earth (34
 * of 44 with DE441, every DE441 row); before bug 180 Moshier's Earth; the
 * upstream build
 * (SWE_UPSTREAM_COMPAT) 31 (42 of 44; only de200's Moon rows pass: its two
 * GMs agree there, and DE200 has no bias). Run as
 *   nodapsgm EPHEDIR [DE4XX_FILE]
 * DE4XX_FILE a path to a JPL file of DE number 403 or above (SE_TEST_DE4 in
 * the Makefile); without one that leg is skipped and says so.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"	/* the context's earth_moon, Moshier's EMB (bug 180) */
#include "swephlib.h"	/* swi_bias() */

#define EPHBITS (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)
static const double rgYear[8] = {-2500, -1000, 0, 1000, 1700, 1900, 2000, 2800};
static const double rgYearJ[8] = {1700, 1750, 1800, 1850, 1900, 1950, 2000, 2100};
/* DE number, GMS and GM1..GM9 (GMB third), AU^3/day^2, ssd.jpl.nasa.gov/
 * ftp/eph/planets/ascii, GROUP 1041: DE200, DE403 (Moshier, fitted to
 * DE404), DE405/406, DE430/431, DE440/441 */
static const struct { int de; double gm[10]; } rgGm[5] = {
  {200, {2.959122082855911e-4, 4.912547451450812e-11, 7.243456209632766e-10,
   8.997011658557308e-10, 9.549528942224058e-11, 2.8253421034459264e-07,
   8.459468504830659e-08, 1.288816238138035e-08, 1.5321124812842762e-08,
   2.2762477518636993e-12}},
  {403, {2.959122082855911e-4, 4.912547451450812e-11, 7.243452486162703e-10,
   8.997011374291876e-10, 9.549535105779258e-11, 2.825345909524227e-07,
   8.459715185680659e-08, 1.29202491678197e-08, 1.524358900784276e-08,
   2.18869976542597e-12}},
  {405, {2.959122082855911e-4, 4.912547451450812e-11, 7.243452486162703e-10,
   8.997011346712499e-10, 9.549535105779258e-11, 2.8253459095242264e-07,
   8.459715185680659e-08, 1.2920249167819694e-08, 1.5243589007842763e-08,
   2.1886997654259697e-12}},
  {430, {2.959122082855911e-4, 4.91248045036476e-11, 7.24345233264412e-10,
   8.997011390199871e-10, 9.54954869555077e-11, 2.82534584083387e-07,
   8.45970607324503e-08, 1.29202482578296e-08, 1.52435734788511e-08,
   2.17844105197418e-12}},
  {440, {2.9591220828411956e-4, 4.912500194889318e-11, 7.243452332644119e-10,
   8.997011392947347e-10, 9.549548829725812e-11, 2.825345825225792e-07,
   8.45970599337629e-08, 1.29202656496824e-08, 1.524357347885194e-08,
   2.175096464893358e-12}},
};
static const int rgPl[9] = {SE_MERCURY, SE_VENUS, SE_EARTH, SE_MARS, SE_JUPITER,
                            SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO};
static const char *rgPlName[9] = {"Mercury", "Venus", "Earth (EMB)", "Mars",
  "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"};
static const int32 rgFrame[2] = {SEFLG_TRUEPOS, SEFLG_TRUEPOS | SEFLG_ICRS};

/* the GM row of a DE number: each pair of a generation shares its GMs */
static const double *gm_of(int de)
{
  int k = de == 200 ? 0 : de == 403 || de == 404 ? 1 : de == 405 || de == 406 ? 2
        : de == 430 || de == 431 ? 3 : de == 440 || de == 441 ? 4 : -1;
  return k < 0 ? NULL : rgGm[k].gm;
}

/* a JPL file's DE number and EMRAT, read from its first record (numde at
 * byte 2840, EMRAT at 2688, either byte order) */
static int jpl_header(const char *path, int *de, double *emrat)
{
  unsigned char b[2848], t[8];
  int k, ok;
  FILE *f = fopen(path, "rb");
  if (f == NULL) return -1;
  ok = fread(b, 1, sizeof b, f) == sizeof b;
  fclose(f);
  if (!ok) return -1;
  memcpy(de, b + 2840, 4);
  memcpy(emrat, b + 2688, 8);
  if (*de < 0 || *de > 10000) {	/* the other byte order */
    for (k = 0; k < 4; k++) t[k] = b[2843 - k];
    memcpy(de, t, 4);
    for (k = 0; k < 8; k++) t[k] = b[2695 - k];
    memcpy(emrat, t, 8);
  }
  return 0;
}

static int row(const char *sz, double dd, double dlim, double da, double alim)
{
  int bad = dd > dlim || da > alim;
  printf("  %-36s distance %.1e AU, direction %.1e deg%s\n", sz, dd, da,
         bad ? "  FAIL" : "");
  return bad;
}

/* one ephemeris: 2 Moon rows and 9 planet rows; *pbad counts failures,
 * returns -1 on an error */
static int leg(swe_ctx *c, int32 eph, const char *name, const double *gm,
               double emrat, int jpl, int *pbad)
{
  char serr[AS_MAXCH], sz[80];
  int iy, ip, ifr, j, nrow = 0;
  for (ifr = 0; ifr < 2; ifr++) {	/* (a) the Moon's apogee */
    int32 fl = eph | rgFrame[ifr];
    double wd = 0, wa = 0;
    for (iy = 0; iy < 8; iy++) {
      double y = jpl ? rgYearJ[iy] : rgYear[iy];
      double t = floor((2451545.0 + (y - 2000) * 365.25 + 0.37) * 256) / 256;
      double o[6], on[6], xn[6], xd[6], xp[6], xa[6], d;
      if ((swe_calc_r(c, t, SE_OSCU_APOG, fl, o, serr) & EPHBITS) != eph
          || (swe_calc_r(c, t, SE_TRUE_NODE, fl, on, serr) & EPHBITS) != eph
          || swe_nod_aps_r(c, t, SE_MOON, fl, SE_NODBIT_OSCU, xn, xd, xp, xa, serr) < 0) {
        printf("  %s Moon ERROR at %.4f: %s\n", name, t, serr);
        return -1;
      }
      if (fabs(o[2] - xa[2]) > wd) wd = fabs(o[2] - xa[2]);
      d = fabs(swe_difdeg2n(o[0], xa[0]));
      if (fabs(o[1] - xa[1]) > d) d = fabs(o[1] - xa[1]);
      if (fabs(on[2] - xn[2]) > wd) wd = fabs(on[2] - xn[2]);
      if (fabs(swe_difdeg2n(on[0], xn[0])) > d) d = fabs(swe_difdeg2n(on[0], xn[0]));
      if (d > wa) wa = d;
    }
    snprintf(sz, sizeof sz, "%s Moon apogee, node%s", name, ifr ? ", ICRS" : "");
    *pbad += row(sz, wd, 1e-14, wa, 1e-9);
    nrow++;
  }
  for (ip = 0; ip < 9; ip++) {	/* (b) the perihelia, both frames */
    double mu = gm[0] + gm[1 + ip], wd = 0, wa = 0;
    for (ifr = 0; ifr < 2; ifr++) {
      int32 fl = eph | rgFrame[ifr];
      for (iy = 0; iy < 8; iy++) {
        double y = jpl ? rgYearJ[iy] : rgYear[iy];
        double t = floor((2451545.0 + (y - 2000) * 365.25 + 0.37) * 256) / 256;
        double x[6], xm[6], xn[6], xd[6], xp[6], xa[6];
        double r, v2, rv, ev[3], e, a, q, lon, lat, d;
        int32 rf = swe_calc_r(c, t, rgPl[ip], fl | SEFLG_HELCTR | SEFLG_J2000
                              | SEFLG_XYZ | SEFLG_SPEED, x, serr);
        if ((rf & EPHBITS) != eph) {
          printf("  %s planet %d ERROR at %.4f: %s\n", name, rgPl[ip], t, serr);
          return -1;
        }
        if (rgPl[ip] == SE_EARTH) {	/* the Earth-Moon barycentre */
          /* with the Moon the Earth was made from: on Moshier not its
           * Moon but the short lunar series' (embofs_mosh()), which the
           * Earth's computation keeps, ICRS equatorial (UPSTREAM-BUGS 180),
           * here in the frame and the J2000 ecliptic of the places */
          double em[6] = {0, 0, 0, 0, 0, 0};
#ifndef SWE_UPSTREAM_COMPAT	/* the upstream build keeps no such Moon */
          int moshem = eph == SEFLG_MOSEPH && c->earth_moon.t == t
                       && c->earth_moon.iephe == SEFLG_MOSEPH;
          for (j = 0; j <= 5; j++)
            em[j] = c->earth_moon.x[j];
          if (moshem && !(fl & SEFLG_ICRS))
            swi_bias(c, em, t, fl | SEFLG_SPEED, FALSE);
          swi_coortrf2(em, em, c->oec2000.seps, c->oec2000.ceps);
          swi_coortrf2(em + 3, em + 3, c->oec2000.seps, c->oec2000.ceps);
#else
          int moshem = 0;
#endif
          swe_calc_r(c, t, SE_MOON, fl | SEFLG_J2000 | SEFLG_XYZ | SEFLG_SPEED, xm, serr);
          for (j = 0; j <= 5; j++)
            x[j] += (moshem ? em[j] : xm[j]) / (emrat + 1.0);
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
    }
    snprintf(sz, sizeof sz, "%s %s perihelion", name, rgPlName[ip]);
    *pbad += row(sz, wd, 1e-12, wa, 1e-9);
    nrow++;
  }
  return nrow;
}

int main(int argc, char **argv)
{
  int bad = 0, nrow = 0, n, de;
  double t0, t1, emrat;
  int32 denum = 0;
  char serr[AS_MAXCH], name[16];
  const double *gm;
  swe_ctx *c;
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "usage: nodapsgm EPHEDIR [DE4XX_FILE]\n");
    return 2;
  }
  printf("G104: swe_nod_aps()'s osculating ellipses take the ephemeris's GM, frame and EMRAT\n");
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_jpl_file_r(c, "de200.eph");
  {	/* the Swiss planet file's DE number */
    double x[6];
    swe_calc_r(c, 2451545.0, SE_MARS, SEFLG_SWIEPH, x, serr);
    swe_get_current_file_data_r(c, 0, &t0, &t1, &denum);
    printf("  Swiss planet file DE%d\n", denum);
  }
  if ((gm = gm_of(denum)) == NULL) {
    printf("  no GMs for DE%d\nG104 FAIL: 1\n", denum);
    swe_ctx_free(c);
    return 1;
  }
  n = leg(c, SEFLG_SWIEPH, "Swiss", gm, 1 / 0.0123000383, 0, &bad);
  if (n >= 0) { nrow += n;
    n = leg(c, SEFLG_JPLEPH, "de200", gm_of(200), 81.300587, 1, &bad); }
  if (n >= 0) { nrow += n;
    n = leg(c, SEFLG_MOSEPH, "Moshier", gm_of(403), 1 / 0.0123000383, 0, &bad); }
  swe_ctx_free(c);
  if (n < 0) {
    printf("G104 FAIL: error\n");
    return 1;
  }
  nrow += n;
  if (argc == 3 && argv[2][0] != '\0') {	/* the DE4xx JPL file */
    char dir[1024], *sl;
    if (jpl_header(argv[2], &de, &emrat) < 0 || de < 403 || (gm = gm_of(de)) == NULL) {
      printf("  %s: not a JPL file of a DE number this test knows (>= 403)\nG104 FAIL: 1\n", argv[2]);
      return 1;
    }
    snprintf(dir, sizeof dir, "%s", argv[2]);
    sl = strrchr(dir, '/');
    c = swe_ctx_new();
    if (sl != NULL) {
      *sl = '\0';
      swe_set_ephe_path_r(c, dir[0] ? dir : "/");
      swe_set_jpl_file_r(c, sl + 1);
    } else {
      swe_set_ephe_path_r(c, ".");
      swe_set_jpl_file_r(c, dir);
    }
    snprintf(name, sizeof name, "DE%d", de);
    printf("  %s, EMRAT %.14g\n", argv[2], emrat);
    n = leg(c, SEFLG_JPLEPH, name, gm, emrat, 1, &bad);
    swe_ctx_free(c);
    if (n < 0) {
      printf("G104 FAIL: error\n");
      return 1;
    }
    nrow += n;
  } else
    printf("  DE4xx leg SKIPPED: no JPL file named (SE_TEST_DE4=<path to a DE >= 403 file>)\n");
  printf("G104: %d rows\n", nrow);
  printf(bad ? "G104 FAIL: %d\n" : "G104 PASS\n", bad);
  return bad ? 1 : 0;
}
