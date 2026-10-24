/* G24: SWE_UPSTREAM_COMPAT must actually restore the defects it claims to.
 *
 * G8 compares this fork against pristine upstream byte for byte, which only
 * works while the fork's deliberate numerical fixes can be switched back off
 * for that one build. If the switch ever stops restoring them -- a macro
 * renamed, a fix moved outside the #ifdef, a header not included -- then G8
 * quietly starts comparing a fixed library against a broken reference and
 * fails for a reason nobody will connect to this.
 *
 * The failure mode that actually worries me is the opposite one: the switch
 * still compiles, still restores SOME of the defects, and G8 keeps passing
 * while a fix silently stops shipping. So this asserts both directions --
 * the default build must NOT have the defect, and the compat build MUST.
 *
 * One binary, built twice. The Makefile passes -DSWE_UPSTREAM_COMPAT to one
 * of them and EXPECT_COMPAT to tell this file which it is.
 *
 *   compat <ephe-dir>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "swephexp.h"

#ifdef EXPECT_COMPAT
# define WANT_DEFECT 1
# define MODE "SWE_UPSTREAM_COMPAT"
#else
# define WANT_DEFECT 0
# define MODE "default (shipped)"
#endif

static const char *EPHE;

/* Defect 1: a JPLHOR request contaminates later answers at the same epoch,
 * because the obliquity and nutation caches key on the epoch alone. */
static int jplhor_contaminates(void)
{
  static const int bodies[] = { SE_SUN, SE_MOON, SE_MARS, SE_MEAN_NODE };
  static const double eps[] = { 1356173.5, 2451545.0 };
  double a[6], c[6], junk[6];
  char s[AS_MAXCH] = "";
  int n = 0;
  for (size_t e = 0; e < sizeof eps / sizeof *eps; e++)
    for (size_t b = 0; b < sizeof bodies / sizeof *bodies; b++) {
      swe_close(); swe_set_ephe_path((char *) EPHE);
      swe_calc(eps[e], bodies[b], SEFLG_SWIEPH, a, s);
      swe_close(); swe_set_ephe_path((char *) EPHE);
      swe_calc(eps[e], SE_MARS, SEFLG_JPLEPH | SEFLG_JPLHOR, junk, s);
      swe_calc(eps[e], bodies[b], SEFLG_SWIEPH, c, s);
      if (a[0] != c[0]) n++;
    }
  return n;
}

/* Defect 2: swe_calc_pctr() never keyed those caches to its own tjd, so its
 * answer followed whichever epoch had been computed before it. */
static double pctr_drift(void)
{
  double x[6], a, b;
  char s[AS_MAXCH] = "";
  swe_close(); swe_set_ephe_path((char *) EPHE);
  swe_calc_pctr(2451545.0, SE_MARS, SE_JUPITER, SEFLG_SWIEPH, x, s);
  a = x[0];
  swe_calc(2451545.0 + 10000.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, x, s);
  swe_calc_pctr(2451545.0, SE_MARS, SE_JUPITER, SEFLG_SWIEPH, x, s);
  b = x[0];
  return fabs(a - b) * 3600.0;
}

/* Defect 3: swe_calc(SE_ECL_NUT) under a different ephemeris flag freed the
 * planets' data but left the files open, so the next call under the old
 * flag looked its segment up with dseg = 0 and reported a damaged file.
 * Returns 1 when Mars afterwards is not the fresh answer. */
static int eclnut_wipes_constants(void)
{
  double a[6], b[6], junk[6];
  char s[AS_MAXCH] = "";
  int ra, rb;
  swe_close(); swe_set_ephe_path((char *) EPHE);
  ra = swe_calc(2451545.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, a, s);
  swe_close(); swe_set_ephe_path((char *) EPHE);
  swe_calc(2440000.0, SE_SUN, SEFLG_SWIEPH, junk, s);
  swe_calc(2440000.0, SE_ECL_NUT, SEFLG_MOSEPH, junk, s);
  rb = swe_calc(2451545.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, b, s);
  return ra != rb || a[0] != b[0] || a[3] != b[3];
}

/* Defect 4: swe_pheno(SE_ECL_NUT) indexed pla_diam[] and mag_elem[] with -1
 * and returned OK. Returns 1 when it is accepted. */
static int pheno_accepts_eclnut(void)
{
  double attr[20];
  char s[AS_MAXCH] = "";
  swe_close(); swe_set_ephe_path((char *) EPHE);
  return swe_pheno(2451545.0, SE_ECL_NUT, SEFLG_SWIEPH, attr, s) != ERR;
}

/* Defect 23: the Moon's light time is taken once from the distance at the
 * instant of observation, and read at that instant rounded to a double.
 * The worst over 40 dates in 1800-2100 of the astrometric direction against
 * M(t - tau) - E(t) solved here on the library's own geometric vectors
 * (G33 checks the rest), in mas. */
static double moon_lighttime(void)
{
  const double c = 173.1446326846693;   /* AU per day */
  int32 base = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_NONUT | SEFLG_XYZ;
  int32 geom = base | SEFLG_BARYCTR | SEFLG_TRUEPOS | SEFLG_SPEED;
  char s[AS_MAXCH] = "";
  double w = 0;
  int id, it;
  swe_close(); swe_set_ephe_path((char *) EPHE);
  for (id = 0; id < 40; id++) {
    double jd = 2378496.5 + id * 2739.325 + id * 0.0371;
    double xa[6], xo[6], xb[6], d[3], tau = 0, tr, e, r, dot;
    if (swe_calc(jd, SE_MOON, base | SEFLG_NOABERR | SEFLG_NOGDEFL, xa, s) < 0 ||
        swe_calc(jd, SE_EARTH, geom, xo, s) < 0)
      return 1e9;
    for (it = 0; it < 20; it++) {
      tr = jd - tau;
      if (swe_calc(tr, SE_MOON, geom, xb, s) < 0)
        return 1e9;
      e = (jd - tr) - tau;
      d[0] = xb[0] + xb[3] * e - xo[0];
      d[1] = xb[1] + xb[4] * e - xo[1];
      d[2] = xb[2] + xb[5] * e - xo[2];
      r = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
      tau = r / c;
    }
    dot = xa[0]*d[0] + xa[1]*d[1] + xa[2]*d[2];
    e = sqrt(pow(xa[1]*d[2] - xa[2]*d[1], 2) + pow(xa[2]*d[0] - xa[0]*d[2], 2)
             + pow(xa[0]*d[1] - xa[1]*d[0], 2));
    e = atan2(e, dot) * 206264806.247;   /* acos cannot resolve 1e-8 rad */
    if (e > w) w = e;
  }
  return w;
}

/* Defect 24: the JPL file is read at TT where its argument is TDB. The worst
 * barycentric Earth from de200.eph against JPL's testpo.200 rows
 * (testpo200.txt), asked at the TT whose TDB is the row's JED, in metres:
 * 50 m of Earth motion in the annual 1.66 ms (G34 holds the Moon too). */
static double jpl_tt_read(void)
{
  int32 f = SEFLG_JPLEPH | SEFLG_BARYCTR | SEFLG_TRUEPOS | SEFLG_J2000 |
            SEFLG_ICRS | SEFLG_NONUT | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_SPEED;
  char s[AS_MAXCH] = "", line[256], date[32];
  double w = 0;
  FILE *pf = fopen("testpo200.txt", "r");
  if (pf == NULL)
    return 1e9;
  swe_close(); swe_set_ephe_path((char *) EPHE); swe_set_jpl_file("de200.eph");
  while (fgets(line, sizeof(line), pf) != NULL) {
    int de, targ, ctr, k;
    double jed, val, tt, T, d, x[6];
    if (line[0] == '#' || sscanf(line, "%d %31s %lf %d %d %d %lf", &de, date,
        &jed, &targ, &ctr, &k, &val) != 7 || targ != 3)
      continue;
    T = (jed - 2451545.0) / 36525.0;   /* USNO Circular 179, eq. 2.6 */
    d = (0.001657 * sin(628.3076 * T + 6.2401)
         + 0.000022 * sin(575.3385 * T + 4.2970)
         + 0.000014 * sin(1256.6152 * T + 6.1969)
         + 0.000005 * sin(606.9777 * T + 4.0212)
         + 0.000005 * sin(52.9691 * T + 0.4444)
         + 0.000002 * sin(21.3299 * T + 5.5431)
         + 0.000010 * T * sin(628.3076 * T + 4.2490)) / 86400.0;
    tt = jed - d;
    if (swe_calc(tt, SE_EARTH, f, x, s) < 0) {
      fclose(pf);
      return 1e9;
    }
    d = fabs(x[k - 1] + x[k + 2] * ((jed - tt) - d) - val) * 1.495978707e11;
    if (d > w) w = d;
  }
  fclose(pf);
  return w;
}

/* Defect 25: the default sidereal time steps where it changes formula at
 * 2050-01-01 0h UT, the long-term formula's constant offset having been
 * fitted under an older delta t. The step over two seconds less the
 * sidereal rate, in arcsec (G35 holds 1850 and four other model sets). */
static double sidt_splice_step(void)
{
  double ta = 2469807.5 - 1.0 / 86400.0, tb = 2469807.5 + 1.0 / 86400.0, d;
  swe_close();
  d = swe_sidtime(tb) - swe_sidtime(ta);
  if (d < -12) d += 24;
  return (d - 24.0 * 1.00273781191135448 * (tb - ta)) * 15 * 3600;
}

/* Defect 26 (G36): a heliocentric light time was measured from the
 * barycentre on its second pass. Returns the angle, arcsec, between
 * Mercury's heliocentric astrometric place under SWIEPH at J2000 and
 * B(t - tau) - S(t) rebuilt from the library's own geometric positions:
 * 0.27" with the defect, roundoff without. */
static double helio_lt_from_barycentre(void)
{
  const double c = 173.1446326846693, jd = 2451545.0;
  int32 base = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_XYZ;
  int32 geom = base | SEFLG_BARYCTR | SEFLG_TRUEPOS;
  double xh[6], xs[6], xb[6], d[3], tau = 0.0, r, cx, cy, cz, dot;
  char s[AS_MAXCH] = "";
  int it;
  swe_close(); swe_set_ephe_path((char *) EPHE);
  if (swe_calc(jd, SE_MERCURY, base | SEFLG_HELCTR | SEFLG_NOABERR | SEFLG_NOGDEFL, xh, s) < 0 ||
      swe_calc(jd, SE_SUN, geom, xs, s) < 0)
    return -1.0;
  for (it = 0; it < 50; it++) {
    if (swe_calc(jd - tau, SE_MERCURY, geom, xb, s) < 0) return -1.0;
    d[0] = xb[0] - xs[0]; d[1] = xb[1] - xs[1]; d[2] = xb[2] - xs[2];
    r = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
    if (fabs(r / c - tau) < 1e-13) break;
    tau = r / c;
  }
  r = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]) * sqrt(xh[0]*xh[0] + xh[1]*xh[1] + xh[2]*xh[2]);
  cx = xh[1]*d[2] - xh[2]*d[1]; cy = xh[2]*d[0] - xh[0]*d[2]; cz = xh[0]*d[1] - xh[1]*d[0];
  dot = (xh[0]*d[0] + xh[1]*d[1] + xh[2]*d[2]) / r;
  return atan2(sqrt(cx*cx + cy*cy + cz*cz) / r, dot) * 206264.806247;
}

/* Defect 27 (G37): the topocentric observer was built on the mean equator of
 * date. Returns, in metres, how far the observer the library used (the
 * geocentric minus the topocentric geometric Moon, true equator of date)
 * sits from the site built on the apparent sidereal time: ~200 m with the
 * defect, millimetres without. Moshier. */
static double observer_off_true_pole(void)
{
  const double tjd = 2451545.0, lon = 8.55, lat = 47.37, alt = 400.0;
  const double f = 1.0 / 298.25642, re = 6378136.6, au = 1.49597870700e+11;   /* sweph.h */
  int32 geom = SEFLG_MOSEPH | SEFLG_TRUEPOS | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_NOABERR | SEFLG_NOGDEFL;
  double xg[6], xt[6], xn[6], ut, gast, cosfi, sinfi, cc, ss, lam, ref[3];
  char s[AS_MAXCH] = "";
  int i;
  swe_close(); swe_set_ephe_path((char *) EPHE);
  swe_set_topo(lon, lat, alt);
  if (swe_calc(tjd, SE_MOON, geom, xg, s) < 0 || swe_calc(tjd, SE_MOON, geom | SEFLG_TOPOCTR, xt, s) < 0 ||
      swe_calc(tjd, SE_ECL_NUT, SEFLG_MOSEPH, xn, s) < 0)
    return -1.0;
  for (i = 0; i < 3; i++) xg[i] = (xg[i] - xt[i]) * au;
  ut = tjd - swe_deltat_ex(tjd, SEFLG_MOSEPH, s);
  gast = swe_sidtime0(ut, xn[0], xn[2]) * 15.0;
  cosfi = cos(lat * DEGTORAD); sinfi = sin(lat * DEGTORAD);
  cc = 1 / sqrt(cosfi * cosfi + (1 - f) * (1 - f) * sinfi * sinfi); ss = (1 - f) * (1 - f) * cc;
  lam = (lon + gast) * DEGTORAD;
  ref[0] = (re * cc + alt) * cosfi * cos(lam); ref[1] = (re * cc + alt) * cosfi * sin(lam); ref[2] = (re * ss + alt) * sinfi;
  return sqrt((xg[0]-ref[0])*(xg[0]-ref[0]) + (xg[1]-ref[1])*(xg[1]-ref[1]) + (xg[2]-ref[2])*(xg[2]-ref[2]));
}

/* Defect 28 (G38): the osculating apogee's ellipse was fitted with a 1996
 * almanac GM. Returns the relative difference between the GM solved back
 * from the reported apogee distance and the Moon's geometric state, and
 * DE403's GMB (Moshier's generation): 3.5e-8 with the defect, ~1e-14
 * without. Moshier. */
static double apo_dist(const double *xm, double gm)   /* a(1+e) through state xm under gm */
{
  double r = sqrt(xm[0]*xm[0] + xm[1]*xm[1] + xm[2]*xm[2]);
  double v2 = xm[3]*xm[3] + xm[4]*xm[4] + xm[5]*xm[5];
  double hx = xm[1]*xm[5] - xm[2]*xm[4], hy = xm[2]*xm[3] - xm[0]*xm[5], hz = xm[0]*xm[4] - xm[1]*xm[3];
  double a = 1.0 / (2.0 / r - v2 / gm);
  return a * (1.0 + sqrt(1.0 - (hx*hx + hy*hy + hz*hz) / (gm * a)));
}

static double apogee_gm_off(void)
{
  const double gmb403 = 8.997011374291876e-10, tjd = 2451545.0;
  int32 flg = SEFLG_MOSEPH | SEFLG_TRUEPOS | SEFLG_NONUT;
  double xa[6], xm[6], g0 = 8.997e-10, g1 = 9.0e-10, f0, f1, g2;
  char s[AS_MAXCH] = "";
  int it;
  swe_close(); swe_set_ephe_path((char *) EPHE);
  if (swe_calc(tjd, SE_OSCU_APOG, flg, xa, s) < 0 ||
      swe_calc(tjd, SE_MOON, flg | SEFLG_J2000 | SEFLG_XYZ | SEFLG_SPEED, xm, s) < 0)
    return -1.0;
  f0 = apo_dist(xm, g0) - xa[2];
  for (it = 0; it < 60; it++) {
    f1 = apo_dist(xm, g1) - xa[2];
    if (f1 == f0) break;
    g2 = g1 - f1 * (g1 - g0) / (f1 - f0);
    g0 = g1; f0 = f1; g1 = g2;
    if (fabs(g1 - g0) < 1e-24) break;
  }
  return fabs(g1 / gmb403 - 1.0);
}

/* Defect 29: the long-term sidereal time follows delta t at the Sun's mean
 * motion. Raising delta t 100 s at 1000-01-01 0h UT, in arcsec: 4.1" on the
 * compat build, the precession rate's 0.00015" shipped (G39 holds the rest). */
static double sidt_follows_deltat(void)
{
  double jd = 2086307.5, dt, s0, s1, d;
  swe_close();
  dt = swe_deltat_ex(jd, -1, NULL);
  s0 = swe_sidtime0(jd, 23.4, 0);
  swe_set_delta_t_userdef(dt + 100.0 / 86400.0);
  s1 = swe_sidtime0(jd, 23.4, 0);
  swe_set_delta_t_userdef(SE_DELTAT_AUTOMATIC);
  d = s1 - s0;
  if (d > 12) d -= 24;
  if (d < -12) d += 24;
  return d * 54000;
}

/* The topocentric Moon's speed against a five-point difference of its own
 * positions, J2000 equator, deg/day, at one instant: through the three-point
 * path (aberration on; UPSTREAM-BUGS 31) or the analytic one (NOABERR; 32). */
static double moon_speed_off(int32 extra)
{
  int32 fl = SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_J2000 | SEFLG_NONUT
             | SEFLG_EQUATORIAL | extra;
  double jd = 2461338.137, x[6], f[5][6], r;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 47.37, 400);
  swe_calc(jd, SE_MOON, fl | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(jd + (k - 2) * 0.01, SE_MOON, fl, f[k], serr);
  r = (f[0][0] - 8 * f[1][0] + 8 * f[3][0] - f[4][0]) / 0.12;
  return fabs(x[3] - r);
}

/* The Sun's ecliptic latitude speed of date against a five-point
 * difference of its latitude, deg/day: the turning of the ecliptic of date
 * (UPSTREAM-BUGS 33) is ~1e-5 deg/day of it. */
static double ecl_lat_speed_off(void)
{
  double jd = 2461338.137, x[6], f[5][6], r;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(jd, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(jd + (k - 2) * 0.01, SE_SUN, SEFLG_SWIEPH, f[k], serr);
  r = (f[0][1] - 8 * f[1][1] + 8 * f[3][1] - f[4][1]) / 0.12;
  return fabs(x[4] - r);
}

/* swe_nod_aps()'s geocentric speeds: the osculating ascending node of Mars
 * against a five-point difference of its latitude, deg/day. The points are
 * built in the ecliptic of date, whose turning is taken out of the
 * heliocentric point and must go back on the geocentric one (UPSTREAM-BUGS
 * 37); upstream does neither, and de/dt x (the Earth's position) survives,
 * ~1e-5 deg/day on this date. */
static double nodaps_turning_off(void)
{
  double jd = 2461338.137, x[6], f[5][6], r, u[6];
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_OSCU, x, u, u, u, serr);
  for (k = 0; k < 5; k++)
    swe_nod_aps(jd + (k - 2) * 0.01, SE_MARS, SEFLG_SWIEPH, SE_NODBIT_OSCU, f[k], u, u, u, serr);
  r = (f[0][1] - 8 * f[1][1] + 8 * f[3][1] - f[4][1]) / 0.12;
  return fabs(x[4] - r);
}

/* The mean perihelion of Mars, heliocentric: its latitude speed against a
 * five-point difference of its latitude, deg/day. Upstream never takes
 * it; the aux. position's latitude, a day on, goes out in its place, so
 * the speed is the latitude itself, ~1.8 deg/day (UPSTREAM-BUGS 38). */
static double mean_apsis_lat_speed_off(void)
{
  double jd = 2461338.137, x[6], f[5][6], r, u[6];
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_MARS, SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_SPEED, SE_NODBIT_MEAN, u, u, x, u, serr);
  for (k = 0; k < 5; k++)
    swe_nod_aps(jd + (k - 2) * 0.01, SE_MARS, SEFLG_SWIEPH | SEFLG_HELCTR, SE_NODBIT_MEAN, u, u, f[k], u, serr);
  r = (f[0][1] - 8 * f[1][1] + 8 * f[3][1] - f[4][1]) / 0.12;
  return fabs(x[4] - r);
}

/* A speed against a five-point difference of its own longitude, deg/day,
 * at an instant and a step that are binary fractions (t + k h exact). */
static double lon_speed_off(int ipl, int32 fl, double jd)
{
  double x[6], f[5][6], h = 1.0 / 256, r, d[5];
  char serr[256];
  int k;
  jd = floor(jd * 256) / 256;
  swe_calc(jd, ipl, fl | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(jd + (k - 2) * h, ipl, fl, f[k], serr);
  for (k = 0; k < 5; k++) {
    d[k] = f[k][0] - f[2][0];
    if (d[k] > 180) d[k] -= 360;
    if (d[k] < -180) d[k] += 360;
  }
  r = (d[0] - 8 * d[1] + 8 * d[3] - d[4]) / (12 * h);
  return fabs(x[3] - r);
}

/* UPSTREAM-BUGS 34: the topocentric Sun's analytic speed with aberration
 * (no term for the site's turning; upstream reached it only through
 * SEFLG_SPEED3, as the analytic path was not used). Here through the
 * default path, which upstream sends to three points: 7e-6 there. */
static double topo_sun_speed_off(void)
{
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 47.37, 400);
  return lon_speed_off(SE_SUN, SEFLG_SWIEPH | SEFLG_TOPOCTR, 2461338.137);
}

/* UPSTREAM-BUGS 35: Mercury's speed, geocentric (the light time's rate
 * taken over a day) and heliocentric (not taken at all) */
static double mercury_lt_rate_off(void)
{
  double w = 0, d;
  int k, c;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (c = 0; c < 2; c++)
    for (k = 0; k < 9; k++)
      if ((d = lon_speed_off(SE_MERCURY, SEFLG_SWIEPH | (c ? SEFLG_HELCTR : 0),
                             2378496.5 + k * 13698.6 + 0.137)) > w)
        w = d;
  return w;
}

/* UPSTREAM-BUGS 36: the topocentric Moon at -999, true positions, the
 * site turning per UT day instead of per TT day */
static double topo_moon_ancient_off(void)
{
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 47.37, 400);
  return lon_speed_off(SE_MOON, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_TRUEPOS | SEFLG_NOABERR,
                       2451545.0 - 2999 * 365.25 + 0.137);
}

/* The Earth rotation angle as 1.00273781191135448 * (JD - J2000) rounds at
 * the size of the day count: on the IERS 2010 sidereal time at -13000,
 * consecutive representable JDs scatter off a straight line by ~1 mas.
 * Worst over 64 of them, in mas. */
static double era_scatter(void)
{
  double jd0 = 2451545.0 - 15000 * 365.25 + 0.5, jd = jd0, v[65], worst = 0;
  int j;
  swe_close();
  swe_set_astro_models("0,0,0,0,0,0,0,3", 0);   /* SEMOD_SIDT_IERS_CONV_2010 */
  for (j = 0; j <= 64; j++) {
    v[j] = swe_sidtime0(jd, 23.4, 0) - swe_sidtime0(jd0, 23.4, 0);
    jd = nextafter(jd, 1e10);
  }
  for (j = 1; j < 64; j++)
    if (fabs(v[j] - v[64] * j / 64) > worst) worst = fabs(v[j] - v[64] * j / 64);
  swe_set_astro_models("0,0,0,0,0,0,0,0", 0);
  return worst * 54000 * 1e3;
}

int main(int argc, char **argv)
{
  int bad = 0, contam, wiped, pheno;
  double drift, moonlt, jpltt, sidstep, hlt, obs, gm, siddt, era, spd3, spdlt, latspd, toposun, merclt, moonanc;
  double nodspd, apslat;

  EPHE = (argc > 1) ? argv[1] : "../ephe";
  printf("G24: SWE_UPSTREAM_COMPAT restores what it claims -- %s\n", MODE);

  contam = jplhor_contaminates();
  drift  = pctr_drift();
  wiped  = eclnut_wipes_constants();
  pheno  = pheno_accepts_eclnut();
  moonlt = moon_lighttime();
  jpltt  = jpl_tt_read();
  sidstep = sidt_splice_step();
  siddt = sidt_follows_deltat();
  hlt    = helio_lt_from_barycentre();
  obs    = observer_off_true_pole();
  gm     = apogee_gm_off();
  era    = era_scatter();
  spd3   = moon_speed_off(0);
  spdlt  = moon_speed_off(SEFLG_NOABERR);
  latspd = ecl_lat_speed_off();
  nodspd = nodaps_turning_off();
  apslat = mean_apsis_lat_speed_off();
  toposun = topo_sun_speed_off();
  merclt = mercury_lt_rate_off();
  moonanc = topo_moon_ancient_off();

  /* Both are asserted in BOTH directions: a switch that restores nothing is
   * as broken as one that never turns off. */
  if (WANT_DEFECT ? (contam > 0) : (contam == 0))
    printf("  JPLHOR contamination   %2d/8 answers   OK\n", contam);
  else {
    printf("  JPLHOR contamination   %2d/8 answers   FAIL: expected %s\n",
           contam, WANT_DEFECT ? "the defect" : "none");
    bad++;
  }

  if (WANT_DEFECT ? (drift > 1.0) : (drift == 0.0))
    printf("  swe_calc_pctr drift    %.2f arcsec    OK\n", drift);
  else {
    printf("  swe_calc_pctr drift    %.2f arcsec    FAIL: expected %s\n",
           drift, WANT_DEFECT ? "> 1 arcsec" : "0");
    bad++;
  }

  if (wiped == WANT_DEFECT)
    printf("  ECL_NUT wipes files    %s              OK\n", wiped ? "yes" : "no ");
  else {
    printf("  ECL_NUT wipes files    %s              FAIL: expected %s\n",
           wiped ? "yes" : "no ", WANT_DEFECT ? "the defect" : "none");
    bad++;
  }

  if (pheno == WANT_DEFECT)
    printf("  pheno(SE_ECL_NUT)      %s        OK\n", pheno ? "accepted" : "refused ");
  else {
    printf("  pheno(SE_ECL_NUT)      %s        FAIL: expected %s\n",
           pheno ? "accepted" : "refused ", WANT_DEFECT ? "the defect" : "none");
    bad++;
  }

  if (WANT_DEFECT ? (moonlt > 0.5) : (moonlt < 0.01))
    printf("  Moon light time        %.4f mas      OK\n", moonlt);
  else {
    printf("  Moon light time        %.4f mas      FAIL: expected %s\n",
           moonlt, WANT_DEFECT ? "> 0.5 mas" : "< 0.01 mas");
    bad++;
  }

  if (WANT_DEFECT ? (jpltt > 10 && jpltt < 1e8) : (jpltt < 1))
    printf("  JPL read at TT         %.3f m        OK\n", jpltt);
  else {
    printf("  JPL read at TT         %.3f m        FAIL: expected %s\n",
           jpltt, WANT_DEFECT ? "> 10 m" : "< 1 m");
    bad++;
  }

  if (WANT_DEFECT ? (fabs(sidstep) > 1) : (fabs(sidstep) < 0.001))
    printf("  sidereal step 2050     %+.6f\"     OK\n", sidstep);
  else {
    printf("  sidereal step 2050     %+.6f\"     FAIL: expected %s\n",
           sidstep, WANT_DEFECT ? "> 1\"" : "< 0.001\"");
    bad++;
  }

  if (WANT_DEFECT ? (hlt > 0.01) : (hlt >= 0.0 && hlt < 1e-4))
    printf("  helio light time       %.4f arcsec  OK\n", hlt);
  else {
    printf("  helio light time       %.4f arcsec  FAIL: expected %s\n",
           hlt, WANT_DEFECT ? "> 0.01 arcsec" : "< 0.0001");
    bad++;
  }

  if (WANT_DEFECT ? (obs > 100.0) : (obs >= 0.0 && obs < 1.0))
    printf("  observer off true pole %.1f m        OK\n", obs);
  else {
    printf("  observer off true pole %.1f m        FAIL: expected %s\n",
           obs, WANT_DEFECT ? "> 100 m" : "< 1 m");
    bad++;
  }

  if (WANT_DEFECT ? (gm > 1e-8) : (gm >= 0.0 && gm < 1e-10))
    printf("  apogee GM off          %.1e         OK\n", gm);
  else {
    printf("  apogee GM off          %.1e         FAIL: expected %s\n",
           gm, WANT_DEFECT ? "> 1e-8" : "< 1e-10");
    bad++;
  }

  swe_close();
  if (WANT_DEFECT ? (fabs(siddt) > 1) : (fabs(siddt) < 0.001))
    printf("  sidereal follows dT    %+.6f\"     OK\n", siddt);
  else {
    printf("  sidereal follows dT    %+.6f\"     FAIL: expected %s\n",
           siddt, WANT_DEFECT ? "> 1\"" : "< 0.001\"");
    bad++;
  }

  if (WANT_DEFECT ? (era > 0.1) : (era < 0.01))
    printf("  rotation angle scatter %.4f mas      OK\n", era);
  else {
    printf("  rotation angle scatter %.4f mas      FAIL: expected %s\n",
           era, WANT_DEFECT ? "> 0.1 mas" : "< 0.01 mas");
    bad++;
  }

  if (WANT_DEFECT ? (spd3 > 5e-4) : (spd3 < 1e-4))
    printf("  3-point speed late     %.2e deg/day OK\n", spd3);
  else {
    printf("  3-point speed late     %.2e deg/day FAIL: expected %s\n",
           spd3, WANT_DEFECT ? "> 5e-4" : "< 1e-4");
    bad++;
  }

  if (WANT_DEFECT ? (spdlt > 5e-5) : (spdlt < 5e-6))
    printf("  Moon light-time rate   %.2e deg/day OK\n", spdlt);
  else {
    printf("  Moon light-time rate   %.2e deg/day FAIL: expected %s\n",
           spdlt, WANT_DEFECT ? "> 5e-5" : "< 5e-6");
    bad++;
  }

  if (WANT_DEFECT ? (latspd > 2e-6) : (latspd < 2e-7))
    printf("  ecliptic turning       %.2e deg/day OK\n", latspd);
  else {
    printf("  ecliptic turning       %.2e deg/day FAIL: expected %s\n",
           latspd, WANT_DEFECT ? "> 2e-6" : "< 2e-7");
    bad++;
  }

  if (WANT_DEFECT ? (nodspd > 2e-6) : (nodspd < 2.5e-7))
    printf("  nod_aps turning        %.2e deg/day OK\n", nodspd);
  else {
    printf("  nod_aps turning        %.2e deg/day FAIL: expected %s\n",
           nodspd, WANT_DEFECT ? "> 2e-6" : "< 2.5e-7");
    bad++;
  }

  if (WANT_DEFECT ? (apslat > 0.1) : (apslat < 1e-6))
    printf("  mean apsis lat speed   %.2e deg/day OK\n", apslat);
  else {
    printf("  mean apsis lat speed   %.2e deg/day FAIL: expected %s\n",
           apslat, WANT_DEFECT ? "> 0.1" : "< 1e-6");
    bad++;
  }

  if (WANT_DEFECT ? (toposun > 2e-6) : (toposun < 2e-7))
    printf("  topocentric Sun speed  %.2e deg/day OK\n", toposun);
  else {
    printf("  topocentric Sun speed  %.2e deg/day FAIL: expected %s\n",
           toposun, WANT_DEFECT ? "> 2e-6" : "< 2e-7");
    bad++;
  }

  if (WANT_DEFECT ? (merclt > 1e-6) : (merclt < 3e-7))
    printf("  planet light-time rate %.2e deg/day OK\n", merclt);
  else {
    printf("  planet light-time rate %.2e deg/day FAIL: expected %s\n",
           merclt, WANT_DEFECT ? "> 1e-6" : "< 3e-7");
    bad++;
  }

  if (WANT_DEFECT ? (moonanc > 1.5e-6) : (moonanc < 1e-6))
    printf("  site turns per TT day  %.2e deg/day OK\n", moonanc);
  else {
    printf("  site turns per TT day  %.2e deg/day FAIL: expected %s\n",
           moonanc, WANT_DEFECT ? "> 1.5e-6" : "< 1e-6");
    bad++;
  }

  printf(bad ? "G24 FAIL\n" : "G24 PASS\n");
  return bad ? 1 : 0;
}
