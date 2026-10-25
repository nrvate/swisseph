/* orbelgm.c -- G107: swe_get_orbital_elements() takes the ephemeris's GM.
 *
 * The defect (UPSTREAM-BUGS 108): get_gmsm() gave the osculating elements
 * the Astronomical Almanac's constants, not the ephemeris's, as
 * swe_nod_aps() had (bug 105): the Moon's G M_earth (1 + 1 /
 * EARTH_MOON_MRAT), 3.3e-8 above DE431's GMB, the Sun's HELGRAVCONST /
 * AUNIT^3, 1.8e-10 below every JPL header's GMS, and plmass's planets; and
 * ipl_to_elem[] sent Pluto to Mercury's mass, so Pluto's two-body GM was
 * the Sun's plus Mercury's, and SEFLG_ORBEL_AA's sum of the masses inside
 * the orbit counted Mercury twice and Pluto never. The Earth's orbit (the
 * Earth-Moon barycentre's) was built with EARTH_MOON_MRAT on a JPL file
 * too, whose Earth comes from its own EMRAT. Pluto's a was 2.5e-7 off
 * (relative), its perihelion 2e-5 deg; DE200's Neptune's perihelion 1.4e-3
 * deg; every planet's a about 2e-10.
 *
 * The check: the Swiss files, de200 and Moshier, and a DE4xx JPL file if
 * one is named, eight dates (-2500..2800; JPL 1700..2100), Mercury..Pluto,
 * the Earth (EMB) and the Moon, two-body and SEFLG_ORBEL_AA. Against the
 * eccentricity vector of swe_calc()'s own heliocentric (the Moon
 * geocentric) J2000 state, with the header's GMS + GMi (the Moon GMB) or,
 * for SEFLG_ORBEL_AA, GMS and the GMs of every planet out to this one (the
 * Earth's EMB counts as one, GMB), the Swiss files' header by their DE
 * number: the semi-axis and the perihelion distance (dret[15]) within
 * 1e-12 relative, the eccentricity within 1e-12, the longitude of
 * perihelion (dret[5], node + argument) within 1e-8 deg (measured 0 (the
 * same expression), 3.7e-14, 3.6e-14 and 4.8e-9 deg). The 4.8e-9 deg is
 * not this defect: near aphelion the true anomaly's half-angle formula,
 * tan(E / 2) as sin E / (1 + cos E), loses digits on a near-circular orbit
 * (Venus, Neptune; UPSTREAM-BUGS 108's notes); 1e-8 still sees Pluto's
 * 2e-5 deg.
 *
 * 20 rows an ephemeris, 60 without a DE4xx file. The build before the fix
 * and the upstream build (SWE_UPSTREAM_COMPAT) fail all 60 (the Moon's too:
 * its AA GM is 3.3e-8 above GMB). Run as
 *   orbelgm EPHEDIR [DE4XX_FILE]
 * DE4XX_FILE a path to a JPL file of DE number 403 or above (SE_TEST_DE4 in
 * the Makefile); without one that leg is skipped and says so.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

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
/* in order out from the Sun, so the SEFLG_ORBEL_AA sum is gm[1..ip+1] */
static const int rgPl[10] = {SE_MERCURY, SE_VENUS, SE_EARTH, SE_MARS, SE_JUPITER,
                             SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO, SE_MOON};
static const char *rgPlName[10] = {"Mercury", "Venus", "Earth (EMB)", "Mars",
  "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto", "Moon"};

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
  if (*de < 0 || *de > 10000) {
    for (k = 0; k < 4; k++) t[k] = b[2843 - k];
    memcpy(de, t, 4);
    for (k = 0; k < 8; k++) t[k] = b[2695 - k];
    memcpy(emrat, t, 8);
  }
  return 0;
}

/* one ephemeris: 10 bodies x (two-body, SEFLG_ORBEL_AA) */
static int leg(swe_ctx *c, int32 eph, const char *name, const double *gm,
               double emrat, int jpl, int *pbad)
{
  char serr[AS_MAXCH];
  int iy, ip, aa, j, nrow = 0;
  for (ip = 0; ip < 10; ip++) for (aa = 0; aa < 2; aa++) {
    double mu = gm[0], wa = 0, we = 0, wq = 0, wp = 0;
    int bad;
    if (rgPl[ip] == SE_MOON)
      mu = gm[3];
    else if (aa)
      for (j = 0; j <= ip; j++) mu += gm[1 + j];
    else
      mu += gm[1 + ip];
    for (iy = 0; iy < 8; iy++) {
      double y = jpl ? rgYearJ[iy] : rgYear[iy];
      double t = floor((2451545.0 + (y - 2000) * 365.25 + 0.37) * 256) / 256;
      double x[6], xm[6], d[50], h[3], ev[3];
      double r, v2, rv, e, a, q, om, w, nx, ny, peri;
      int32 fl = eph | SEFLG_J2000 | SEFLG_XYZ | SEFLG_TRUEPOS | SEFLG_NONUT | SEFLG_SPEED;
      int32 rf = swe_calc_r(c, t, rgPl[ip], fl | (rgPl[ip] == SE_MOON ? 0 : SEFLG_HELCTR), x, serr);
      if ((rf & EPHBITS) != eph
          || swe_get_orbital_elements_r(c, t, rgPl[ip], eph | (aa ? SEFLG_ORBEL_AA : 0), d, serr) < 0) {
        printf("  %s %s ERROR at %.4f: %s\n", name, rgPlName[ip], t, serr);
        return -1;
      }
      if (rgPl[ip] == SE_EARTH) {	/* the Earth-Moon barycentre */
        swe_calc_r(c, t, SE_MOON, fl, xm, serr);
        for (j = 0; j <= 5; j++)
          x[j] += xm[j] / (emrat + 1.0);
      }
      r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
      v2 = x[3] * x[3] + x[4] * x[4] + x[5] * x[5];
      rv = x[0] * x[3] + x[1] * x[4] + x[2] * x[5];
      for (j = 0; j < 3; j++)
        ev[j] = ((v2 - mu / r) * x[j] - rv * x[j+3]) / mu;
      e = sqrt(ev[0] * ev[0] + ev[1] * ev[1] + ev[2] * ev[2]);
      a = 1 / (2 / r - v2 / mu);
      q = a * (1 - e);
      h[0] = x[1] * x[5] - x[2] * x[4];
      h[1] = x[2] * x[3] - x[0] * x[5];
      h[2] = x[0] * x[4] - x[1] * x[3];
      nx = -h[1]; ny = h[0];	/* towards the ascending node */
      om = atan2(ny, nx);
      {	/* the argument of perihelion, n to ev about h, by atan2 (acos
         * loses half the digits near the nodes) */
        double hn = sqrt(h[0] * h[0] + h[1] * h[1] + h[2] * h[2]);
        double cw = nx * ev[0] + ny * ev[1];
        double sw = ((ny * ev[2]) * h[0] - (nx * ev[2]) * h[1]
                     + (nx * ev[1] - ny * ev[0]) * h[2]) / hn;
        w = atan2(sw, cw);
      }
      peri = swe_degnorm((om + w) * RADTODEG);
      if (fabs(d[0] - a) / a > wa) wa = fabs(d[0] - a) / a;
      if (fabs(d[1] - e) > we) we = fabs(d[1] - e);
      if (fabs(d[15] - q) / q > wq) wq = fabs(d[15] - q) / q;
      if (fabs(swe_difdeg2n(d[5], peri)) > wp) wp = fabs(swe_difdeg2n(d[5], peri));
    }
    bad = wa > 1e-12 || we > 1e-12 || wq > 1e-12 || wp > 1e-8;
    printf("  %-8s %-12s %-3s da/a %.1e, de %.1e, dq/q %.1e, dperi %.1e deg%s\n",
           name, rgPlName[ip], aa ? "AA" : "2b", wa, we, wq, wp, bad ? "  FAIL" : "");
    *pbad += bad;
    nrow++;
  }
  return nrow;
}

int main(int argc, char **argv)
{
  int bad = 0, nrow = 0, n, de;
  double t0, t1, emrat, x[6];
  int32 denum = 0;
  char serr[AS_MAXCH], name[16];
  const double *gm;
  swe_ctx *c;
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "usage: orbelgm EPHEDIR [DE4XX_FILE]\n");
    return 2;
  }
  printf("G107: swe_get_orbital_elements() takes the ephemeris's GM\n");
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_calc_r(c, 2451545.0, SE_MARS, SEFLG_SWIEPH, x, serr);
  swe_get_current_file_data_r(c, 0, &t0, &t1, &denum);
  printf("  Swiss planet file DE%d\n", denum);
  if ((gm = gm_of(denum)) == NULL) {
    printf("  no GMs for DE%d\nG107 FAIL: 1\n", denum);
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
    printf("G107 FAIL: error\n");
    return 1;
  }
  nrow += n;
  if (argc == 3 && argv[2][0] != '\0') {	/* the DE4xx JPL file */
    char dir[1024], *sl;
    if (jpl_header(argv[2], &de, &emrat) < 0 || de < 403 || (gm = gm_of(de)) == NULL) {
      printf("  %s: not a JPL file of a DE number this test knows (>= 403)\nG107 FAIL: 1\n", argv[2]);
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
      printf("G107 FAIL: error\n");
      return 1;
    }
    nrow += n;
  } else
    printf("  DE4xx leg SKIPPED: no JPL file named (SE_TEST_DE4=<path to a DE >= 403 file>)\n");
  printf("G107: %d rows\n", nrow);
  printf(bad ? "G107 FAIL: %d\n" : "G107 PASS\n", bad);
  return bad ? 1 : 0;
}
