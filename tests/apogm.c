/* apogm.c -- G38: the osculating lunar apogee is computed with the
 * gravitational parameter of the ephemeris the Moon came from.
 *
 * lunar_osc_elem() fits a two-body ellipse to the Moon's geocentric state and
 * reports its apogee. The ellipse's GM was a compile-time constant,
 * GEOGCONST (G M_earth, Astronomical Almanac 1996) x (1 + 1/EARTH_MOON_MRAT),
 * 3.3e-8 above the GMB (G (M_earth + M_moon)) that DE440/441 integrated the
 * Moon with, 3.5e-8 above DE403's. The eccentricity vector of a near-circular
 * orbit amplifies a relative error in GM by about (1 + e)/e = 19, so the
 * apogee's direction was off by up to 0.2" (median 0.08" over 36 instants
 * 1600-2600 against Ephemeris Prometheia, 2026-10-24); with the ephemeris's
 * own GMB the two agree to 0.004" geometric. Entry 28 in
 * notes/UPSTREAM-BUGS.md.
 *
 * The check is an invariant, with no reference ephemeris: the apogee
 * DISTANCE the library reports, r_a = a (1 + e), together with the Moon's
 * geometric geocentric state it was fitted to (SEFLG_TRUEPOS on both, so no
 * light time enters), determines the GM of the fitted ellipse uniquely;
 * solving for it must give the GM of the ephemeris in use to 1e-10
 * relative. Three ephemerides:
 *   - JPLEPH, de200.eph from ephe/: the expected GMB is read from the file's
 *     own header HERE, independently of the library's reader;
 *   - SWIEPH: the file's DE number (swe_get_current_file_data) names the
 *     JPL header constant;
 *   - MOSEPH: DE403's, the generation Moshier's lunar theory is fitted to.
 * The header values are JPL's (ssd.jpl.nasa.gov/ftp/eph/planets/ascii).
 * Run as
 *   apogm <ephe-dir>
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "swephexp.h"

/* GMB, AU^3/day^2, from the JPL ASCII headers */
static double gmb_for_denum(int denum)
{
  if (denum >= 440) return 8.997011392947347e-10;   /* header.440, header.441 */
  if (denum >= 430) return 8.997011390199871e-10;   /* header.430, header.431 */
  if (denum >= 405) return 8.997011346712499e-10;   /* header.405, header.406 */
  if (denum >= 403) return 8.997011374291876e-10;   /* header.403 */
  return 8.997011658557308e-10;                     /* header.200 */
}

/* a(1+e) of the two-body ellipse through state (p, v) under gm */
static double apogee_dist(const double *p, const double *v, double gm)
{
  double r = sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
  double v2 = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
  double hx = p[1]*v[2] - p[2]*v[1], hy = p[2]*v[0] - p[0]*v[2], hz = p[0]*v[1] - p[1]*v[0];
  double h2 = hx*hx + hy*hy + hz*hz;
  double a = 1.0 / (2.0 / r - v2 / gm);
  double e = sqrt(1.0 - h2 / (gm * a));
  return a * (1.0 + e);
}

/* the gm whose ellipse through (p, v) has apogee distance ra: secant
 * iteration from the DE440 value; r_a is monotonic in gm here */
static double solve_gm(const double *p, const double *v, double ra)
{
  double g0 = 8.997e-10, g1 = 9.0e-10, f0 = apogee_dist(p, v, g0) - ra, f1, g2;
  int it;
  for (it = 0; it < 60; it++) {
    f1 = apogee_dist(p, v, g1) - ra;
    if (f1 == f0) break;
    g2 = g1 - f1 * (g1 - g0) / (f1 - f0);
    g0 = g1; f0 = f1; g1 = g2;
    if (fabs(g1 - g0) < 1e-24) break;
  }
  return g1;
}

/* GMB from a JPL binary's own header, read here: record 1 holds the constant
 * names (400 x 6 chars after the 252-char title) and, after ss[3], ncon, au,
 * emrat, ipt[36], numde, lpt[3]; record 2 holds the values. The record
 * length follows from the coefficient layout, as in the library's fsizer(). */
static int header_gmb(const char *fname, double *gmb)
{
  FILE *fp = fopen(fname, "rb");
  unsigned char hdr[2856];
  char cnam[400 * 6];
  int32_t ncon, ipt[39], khi = 0, kmx = 0, nd, i, ksize;
  double ss[3], cval[400];
  int swap;
  if (fp == NULL) return 0;
  if (fread(hdr, 1, sizeof hdr, fp) != sizeof hdr) { fclose(fp); return 0; }
  memcpy(cnam, hdr + 252, sizeof cnam);
  memcpy(ss, hdr + 2652, sizeof ss);
  /* byte order: the segment size in days is 1..200 when read right */
  swap = !(ss[2] >= 1 && ss[2] <= 200);
  if (swap) {
    for (i = 0; i < 3; i++) {
      unsigned char *b = hdr + 2652 + 8 * i, t;
      t = b[0]; b[0] = b[7]; b[7] = t; t = b[1]; b[1] = b[6]; b[6] = t;
      t = b[2]; b[2] = b[5]; b[5] = t; t = b[3]; b[3] = b[4]; b[4] = t;
    }
    memcpy(ss, hdr + 2652, sizeof ss);
    if (!(ss[2] >= 1 && ss[2] <= 200)) { fclose(fp); return 0; }
  }
  memcpy(&ncon, hdr + 2676, 4);
  memcpy(ipt, hdr + 2696, 36 * 4);
  memcpy(ipt + 36, hdr + 2844, 3 * 4);
  if (swap) {
    unsigned char *b = (unsigned char *) &ncon, t;
    t = b[0]; b[0] = b[3]; b[3] = t; t = b[1]; b[1] = b[2]; b[2] = t;
    for (i = 0; i < 39; i++) {
      b = (unsigned char *) &ipt[i];
      t = b[0]; b[0] = b[3]; b[3] = t; t = b[1]; b[1] = b[2]; b[2] = t;
    }
  }
  if (ncon < 1 || ncon > 400) { fclose(fp); return 0; }
  for (i = 0; i < 13; i++)
    if (ipt[i * 3] > kmx) { kmx = ipt[i * 3]; khi = i; }
  nd = khi == 11 ? 2 : 3;
  ksize = 2 * (ipt[khi * 3] - 1 + nd * ipt[khi * 3 + 1] * ipt[khi * 3 + 2]);
  if (fseek(fp, (long) ksize * 4, SEEK_SET) != 0) { fclose(fp); return 0; }
  if (fread(cval, sizeof(double), 400, fp) != 400) { fclose(fp); return 0; }
  fclose(fp);
  for (i = 0; i < ncon; i++) {
    if (strncmp(cnam + 6 * i, "GMB   ", 6) == 0) {
      unsigned char *b = (unsigned char *) &cval[i], t;
      if (swap) {
        t = b[0]; b[0] = b[7]; b[7] = t; t = b[1]; b[1] = b[6]; b[6] = t;
        t = b[2]; b[2] = b[5]; b[5] = t; t = b[3]; b[3] = b[4]; b[4] = t;
      }
      *gmb = cval[i];
      return 1;
    }
  }
  return 0;
}

int main(int argc, char **argv)
{
  static const double dates[] = { 2415020.5, 2451545.0, 2461300.5, 2488069.5 };
  static const int32 ephs[] = { SEFLG_MOSEPH, SEFLG_SWIEPH, SEFLG_JPLEPH };
  static const char *names[] = { "MOSEPH", "SWIEPH", "JPLEPH (de200.eph)" };
  const char *ephe = argc > 1 ? argv[1] : "../ephe";
  char path[512], serr[AS_MAXCH];
  double worst = 0.0, gmb_hdr = 0.0;
  int bad = 0, e, have_hdr;
  size_t id;

  swe_set_ephe_path((char *) ephe);
  swe_set_jpl_file((char *) "de200.eph");
  snprintf(path, sizeof(path), "%s/de200.eph", ephe);
  have_hdr = header_gmb(path, &gmb_hdr);
  printf("G38: the osculating apogee's ellipse has the ephemeris's own GM\n");
  if (!have_hdr) {
    printf("  cannot read %s's header\n", path);
    bad++;
  } else {
    printf("  de200.eph header GMB %.15e AU^3/d^2 (table %.15e)\n", gmb_hdr, gmb_for_denum(200));
  }
  for (e = 0; e < 3; e++) {
    int32 flg = ephs[e] | SEFLG_TRUEPOS | SEFLG_NONUT;
    printf("  %s\n", names[e]);
    for (id = 0; id < sizeof dates / sizeof *dates; id++) {
      double xa[6], xm[6], gm, expect, rel, tfs, tfe;
      int denum = 0;
      if (swe_calc(dates[id], SE_OSCU_APOG, flg, xa, serr) < 0 ||
          swe_calc(dates[id], SE_MOON, flg | SEFLG_J2000 | SEFLG_XYZ | SEFLG_SPEED, xm, serr) < 0) {
        printf("    %10.1f ERROR %s\n", dates[id], serr);
        bad++;
        continue;
      }
      gm = solve_gm(xm, xm + 3, xa[2]);
      if (ephs[e] == SEFLG_JPLEPH) {
        expect = gmb_hdr;
      } else if (ephs[e] == SEFLG_SWIEPH) {
        if (swe_get_current_file_data(1, &tfs, &tfe, &denum) == NULL) denum = 0;
        expect = gmb_for_denum(denum);
      } else {
        denum = 403;
        expect = gmb_for_denum(403);
      }
      rel = gm / expect - 1.0;
      printf("    %10.1f  r_a %.9f AU   fitted GM %.12e   expected %.12e (DE%d)   %+.2e%s\n",
             dates[id], xa[2], gm, expect, ephs[e] == SEFLG_JPLEPH ? 200 : denum, rel,
             fabs(rel) > 1e-10 ? "   FAIL" : "");
      if (fabs(rel) > worst) worst = fabs(rel);
      if (fabs(rel) > 1e-10) bad++;
    }
  }
  swe_close();
  printf("  worst %.2e relative (limit 1e-10)\n", worst);
  printf(bad ? "G38 FAIL: %d\n" : "G38 PASS\n", bad);
  return bad ? 1 : 0;
}
