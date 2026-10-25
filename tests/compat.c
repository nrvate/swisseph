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
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
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

/* UPSTREAM-BUGS 39: Venus 0.02 of the Sun's radius behind its centre
 * (2016-06-06): the spread of its J2000 longitude and latitude speeds over
 * seven instants 1e-9 day apart, deg/day; the deflection's rate formed
 * from two cancelling quantities was rough by 1.5e-5 */
static double defl_rough_off(void)
{
  double lo[2] = {1e9, 1e9}, hi[2] = {-1e9, -1e9}, x[6];
  char serr[256];
  int k, j;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = -3; k <= 3; k++) {
    swe_calc(2457546.412153 + k * 1e-9, SE_VENUS,
             SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_SPEED, x, serr);
    for (j = 0; j < 2; j++) {
      if (x[3 + j] < lo[j]) lo[j] = x[3 + j];
      if (x[3 + j] > hi[j]) hi[j] = x[3 + j];
    }
  }
  return fmax(hi[0] - lo[0], hi[1] - lo[1]);
}

/* UPSTREAM-BUGS 74: Isis-Transpluto 0.005 deg from the Sun's centre
 * (2025-09-21): the deflection's part of its longitude speed (speed less
 * the speed with SEFLG_NOGDEFL) against a five-point difference of the
 * deflection over 2^-14 day, deg/day; the rate as a one-sided step was
 * off by half the step times the deflection's curvature, 1.8e-6 */
static double defl_disc_rate_off(void)
{
  const double t = 2460914.78704834, h = 1.0 / 16384;
  const int32 fl = SEFLG_SWIEPH;
  double x[6], y[6], a[6], b[6], d[5], r;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_ISIS, fl | SEFLG_SPEED, x, serr);
  swe_calc(t, SE_ISIS, fl | SEFLG_SPEED | SEFLG_NOGDEFL, y, serr);
  for (k = 0; k < 5; k++) {
    swe_calc(t + (k - 2) * h, SE_ISIS, fl, a, serr);
    swe_calc(t + (k - 2) * h, SE_ISIS, fl | SEFLG_NOGDEFL, b, serr);
    d[k] = a[0] - b[0];
  }
  r = (8 * (d[3] - d[1]) - (d[4] - d[0])) / (12 * h);
  return fabs(x[3] - y[3] - r);
}

/* UPSTREAM-BUGS 91: True Revati with zeta Psc 0.3 deg from the Sun
 * (1981-04-08): the ayanamsa's speed in the Sun's sidereal speed (its
 * tropical speed without nutation less it) against a five-point
 * difference of tropical less sidereal longitude over 2^-8 day, deg/day.
 * The rate was a backward difference over 0.001 day of the aberrated,
 * deflected star, off by half its curvature times the step, 2.1e-6 */
static double aya_star_rate_off(void)
{
  const double t = 2444704.30859375, h = 1.0 / 256;
  double xt[6], xs[6], d[5], r;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_sid_mode(SE_SIDM_TRUE_REVATI, 0, 0);
  for (k = 0; k < 5; k++) {
    swe_calc(t + (k - 2) * h, SE_SUN, SEFLG_SWIEPH | SEFLG_NONUT, xt, serr);
    swe_calc(t + (k - 2) * h, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL, xs, serr);
    d[k] = xt[0] - xs[0];
  }
  r = (8 * (d[3] - d[1]) - (d[4] - d[0])) / (12 * h);
  swe_calc(t, SE_SUN, SEFLG_SWIEPH | SEFLG_NONUT | SEFLG_SPEED, xt, serr);
  swe_calc(t, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL | SEFLG_SPEED, xs, serr);
  swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  return fabs(xt[3] - xs[3] - r);
}

/* UPSTREAM-BUGS 117: the mean apogee's longitude speed, Swiss files, at
 * a binary instant, against a five-point difference over 2^-4 day,
 * deg/day. Upstream divides x(t) - x(t - 0.001) by 0.001, the rounded
 * step being 0.001 to half an ulp of the date */
static double mean_apog_speed_off(void)
{
  double t = floor(2460000.3 * 256) / 256, h = 1.0 / 16, x[6], f[5][6], d1, d2;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_MEAN_APOG, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(t + (k - 2) * h, SE_MEAN_APOG, SEFLG_SWIEPH, f[k], serr);
  d1 = swe_difdeg2n(f[3][0], f[1][0]);
  d2 = swe_difdeg2n(f[4][0], f[0][0]);
  return fabs(x[3] - (8 * d1 - d2) / (12 * h));
}

/* UPSTREAM-BUGS 97: 10 deg split into nakshatras under SE_SIDM_USER, the
 * place read back from nakshatra, degrees and minutes, less 10, deg. The
 * Sheoran test was a bit mask that 255 passes: 3 deg 20' */
static double user_nakshatra_off(void)
{
  int d, m, sec, n;
  double fr;
  swe_set_sid_mode(SE_SIDM_USER, 2451545.0, 23.5);
  swe_split_deg(10.0, SE_SPLIT_DEG_NAKSHATRA | SE_SPLIT_DEG_ROUND_SEC, &d, &m, &sec, &fr, &n);
  swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  return fabs(n * 40.0 / 3 + d + m / 60.0 - 10);
}

/* UPSTREAM-BUGS 34 in swe_nod_aps(): Pluto's osculating ascending node
 * seen from Zurich, its longitude speed against a five-point difference,
 * deg/day; the aberration rate was upstream's chord, 3e-4 off */
static double nodaps_topo_rate_off(void)
{
  double jd = floor(2461338.137 * 256) / 256, h = 1.0 / 64, x[6], f[5][6], u[6], d[5], r;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 47.37, 400);
  swe_nod_aps(jd, SE_PLUTO, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED, SE_NODBIT_OSCU, x, u, u, u, serr);
  for (k = 0; k < 5; k++)
    swe_nod_aps(jd + (k - 2) * h, SE_PLUTO, SEFLG_SWIEPH | SEFLG_TOPOCTR, SE_NODBIT_OSCU, f[k], u, u, u, serr);
  for (k = 0; k < 5; k++) {
    d[k] = f[k][0] - f[2][0];
    if (d[k] > 180) d[k] -= 360;
    if (d[k] < -180) d[k] += 360;
  }
  r = (d[0] - 8 * d[1] + 8 * d[3] - d[4]) / (12 * h);
  return fabs(x[3] - r);
}

/* UPSTREAM-BUGS 48: Vulcan (seorbel.txt's set 16, mean anomaly, perihelion
 * and node in T) against a five-point difference of its heliocentric
 * geometric J2000 longitude over 2^-6 day, deg/day; the two-body speed of
 * a fixed ellipse was off by 2.9e-2 of it */
static double orbel_speed_off(void)
{
  double jd = 2461338.125, h = 1.0 / 64, x[6], f[5][6], r;
  int32 fl = SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_TRUEPOS | SEFLG_J2000 | SEFLG_NONUT;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  if (swe_calc(jd, SE_FICT_OFFSET + 15, fl | SEFLG_SPEED, x, serr) < 0)
    return -1;
  for (k = 0; k < 5; k++)
    swe_calc(jd + (k - 2) * h, SE_FICT_OFFSET + 15, fl, f[k], serr);
  for (k = 0; k < 5; k++)
    f[k][0] = f[2][0] + swe_difdeg2n(f[k][0], f[2][0]);
  r = (f[0][0] - 8 * f[1][0] + 8 * f[3][0] - f[4][0]) / (12 * h);
  return fabs(x[3] - r);
}

/* UPSTREAM-BUGS 60: a fictitious body whose mean anomaly is a plain negative
 * number, "-12.5", against the same ellipse written 347.5, heliocentric
 * longitudes 100 days after the epoch, deg. A '-' made the field a
 * polynomial in T, so the epoch was set to tjd, no daily motion was added,
 * and the body stood still: 113 deg apart */
static double fict_negative_mano_off(void)
{
  char dir[] = "/tmp/compatfnXXXXXX", path[4096], serr[256];
  double x[6], p[6], r;
  FILE *pf;
  swe_close();
  if (mkdtemp(dir) == NULL)
    return -1;
  snprintf(path, sizeof(path), "%s/seorbel.txt", dir);
  if ((pf = fopen(path, "w")) == NULL)
    return -1;
  fputs("J2000, J2000, -12.5, 1.0, 0.1, 30, 40, 5, Negative\n"
        "J2000, J2000, 347.5, 1.0, 0.1, 30, 40, 5, Positive\n", pf);
  fclose(pf);
  snprintf(path, sizeof(path), "%s:%s", dir, EPHE);
  swe_set_ephe_path(path);
  r = -1;
  if (swe_calc(2451645.0, SE_FICT_OFFSET, SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS, x, serr) >= 0 &&
      swe_calc(2451645.0, SE_FICT_OFFSET + 1, SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS, p, serr) >= 0)
    r = fabs(swe_difdeg2n(x[0], p[0]));
  swe_close();
  snprintf(path, sizeof(path), "%s/seorbel.txt", dir);
  unlink(path);
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return r;
}

/* UPSTREAM-BUGS 44: the interpolated perigee in 2100, its longitude with
 * and without SEFLG_SPEED, mas. Its light-time step took the speed, zero
 * without the flag: 107 mas apart */
static double intp_speedflag_off(void)
{
  double jd = 2451545.0 + 36525.0 + 0.1234567, x[6], xs[6];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(jd, SE_INTP_PERG, SEFLG_SWIEPH, x, serr);
  swe_calc(jd, SE_INTP_PERG, SEFLG_SWIEPH | SEFLG_SPEED, xs, serr);
  return fabs(swe_difdeg2n(x[0], xs[0])) * 3600000.0;
}

/* UPSTREAM-BUGS 45: the same perigee's longitude at 16 consecutive
 * representable JDs, the largest residual from their least-squares line,
 * mas: the apsis's parabola steps of 1.8" cancelled to 1.3 mas */
static double intp_rough_off(void)
{
  double jd = 2451545.0 + 36525.0 + 0.1234567, x[6], v[16], x0 = 0;
  double st = 0, sv = 0, stt = 0, stv = 0, a, b, w = 0;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 16; k++) {
    swe_calc(jd, SE_INTP_PERG, SEFLG_SWIEPH, x, serr);
    if (k == 0) x0 = x[0];
    v[k] = swe_difdeg2n(x[0], x0) * 3600000.0;
    st += k; sv += v[k]; stt += (double) k * k; stv += k * v[k];
    jd = nextafter(jd, 1e9);
  }
  b = (16 * stv - st * sv) / (16 * stt - st * st);
  a = (sv - b * st) / 16;
  for (k = 0; k < 16; k++)
    w = fmax(w, fabs(v[k] - a - b * k));
  return w;
}

/* UPSTREAM-BUGS 56: the interpolated perigee at JD 687314.573 (-2832),
 * where its iteration stopped farthest from its fixed point, TRUEPOS and
 * NONUT (the iteration's own place), its longitude from the fixed point,
 * 73.311264799172 deg (fourteen more Newton passes), mas: five passes left
 * it 1.17e3 mas off */
static double intp_fixpoint_off(void)
{
  double x[6];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(687314.573, SE_INTP_PERG, SEFLG_SWIEPH | SEFLG_TRUEPOS | SEFLG_NONUT, x, serr);
  return fabs(swe_difdeg2n(x[0], 73.311264799172)) * 3600000.0;
}

/* Bug 72: the interpolated perigee at JD 2486223.6015625 (2094), NONUT,
 * against its TRUEPOS position at t - tau, tau its TRUEPOS distance at t
 * over c, mas. The fork's one-pass light-time estimate left it 4.4e-3 mas
 * off; upstream has no light time without SEFLG_SPEED, 91 mas. */
static double intp_lighttime_off(void)
{
  double t = 2486223.6015625, xa[6], xt[6], tau, d2 = 0, r2 = 0;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_INTP_PERG, SEFLG_SWIEPH | SEFLG_NONUT | SEFLG_XYZ, xa, serr);
  swe_calc(t, SE_INTP_PERG, SEFLG_SWIEPH | SEFLG_TRUEPOS | SEFLG_NONUT | SEFLG_XYZ, xt, serr);
  tau = sqrt(xt[0] * xt[0] + xt[1] * xt[1] + xt[2] * xt[2]) * (1.49597870700e11 / 299792458.0 / 86400.0);
  swe_calc(t - tau, SE_INTP_PERG, SEFLG_SWIEPH | SEFLG_TRUEPOS | SEFLG_NONUT | SEFLG_XYZ, xt, serr);
  for (k = 0; k < 3; k++) {
    d2 += (xa[k] - xt[k]) * (xa[k] - xt[k]);
    r2 += xt[k] * xt[k];
  }
  return sqrt(d2 / r2) * 206264806.24709636;
}

/* SE_ECL_NUT with SEFLG_SPEED3 against without a speed flag at a binary
 * instant: the largest slot difference, deg. Upstream runs the position
 * speed over the four angles: x[3], the nutation in obliquity, comes back
 * as the true obliquity's rate, 1.6e-3 off (UPSTREAM-BUGS 90). */
static double ecl_nut_speed3_off(void)
{
  double t = floor(2460000.3 * 256) / 256, a[6], b[6], r = 0;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_ECL_NUT, SEFLG_SWIEPH, a, serr);
  swe_calc(t, SE_ECL_NUT, SEFLG_SWIEPH | SEFLG_SPEED3, b, serr);
  for (k = 0; k < 6; k++)
    if (fabs(a[k] - b[k]) > r)
      r = fabs(a[k] - b[k]);
  /* and with SEFLG_EQUATORIAL after Cupido's equatorial call, which
   * upstream answers with Cupido's right ascension etc. (part 2) */
  swe_calc(t + 0.25, SE_FICT_OFFSET + 1, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_EQUATORIAL, b, serr);
  swe_calc(t, SE_ECL_NUT, SEFLG_SWIEPH | SEFLG_EQUATORIAL, b, serr);
  for (k = 0; k < 6; k++)
    if (fabs(a[k] - b[k]) > r)
      r = fabs(a[k] - b[k]);
  return r;
}

/* A fixed star's distance rate: Spica's, geocentric apparent, against a
 * five-point difference of its distance over 2^-8 day, AU/day. Upstream
 * takes the deflection's and the aberration's rates as differences of two
 * vectors of the star's distance, 1.6e7 AU, over 5e-7 and 1e-5 day, and
 * their last bits come out as 2e-3 AU/day (UPSTREAM-BUGS 49). */
static double star_dist_rate_off(void)
{
  double t = floor(2460000.3 * 256) / 256, h = 1.0 / 256, x[6], f[5][6];
  char serr[256], s[64];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  strcpy(s, "Spica");
  swe_fixstar2(s, t, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++) {
    strcpy(s, "Spica");
    swe_fixstar2(s, t + (k - 2) * h, SEFLG_SWIEPH, f[k], serr);
  }
  return fabs(x[5] - (8 * (f[3][2] - f[1][2]) - (f[4][2] - f[0][2])) / (12 * h));
}

/* The osculating apogee's longitude speed (j2000 = 0) or the true node's
 * J2000 latitude speed (j2000 = 1), Swiss files, at a binary instant,
 * against a nine-point difference over 2^-3 day, deg/day. Upstream divides
 * by 2 x 0.0001 day, not by the interval between the Moon's rounded
 * epochs, and leaves J2000's mean obliquity rate out (UPSTREAM-BUGS 75). */
static double osc_speed_off(int j2000)
{
  static const double c9[4] = {4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280};
  double t = floor(2460000.3 * 256) / 256, h = 1.0 / 8, x[6], f[9][6], r = 0;
  int32 fl = SEFLG_SWIEPH | (j2000 ? SEFLG_J2000 : 0);
  int ipl = j2000 ? SE_TRUE_NODE : SE_OSCU_APOG, i = j2000 ? 1 : 0, k;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, ipl, fl | SEFLG_SPEED, x, serr);
  for (k = 0; k < 9; k++)
    swe_calc(t + (k - 4) * h, ipl, fl, f[k], serr);
  for (k = 0; k < 9; k++)
    f[k][0] = f[4][0] + swe_difdeg2n(f[k][0], f[4][0]);
  for (k = 1; k <= 4; k++)
    r += c9[k - 1] * (f[4 + k][i] - f[4 - k][i]);
  return fabs(x[3 + i] - r / h);
}

/* The noise of the Moon's osculating perigee's longitude speed from
 * swe_nod_aps() on Moshier, at 24 instants 2^-30 day apart: the rms of
 * the fourth differences over sqrt(70), deg/day. Upstream's Moshier Moon
 * velocity is a difference over +-4.32 s (bug 51), ts20's a five-point one
 * over 2^-5 day, whose rounding the near-circular orbit amplifies about
 * 36-fold (UPSTREAM-BUGS 95). */
static double moon_oscu_peri_noise(void)
{
  double t = floor(2460000.3 * 256) / 256, v[24], xn[6], xd[6], xp[6], xa[6], s = 0;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 24; k++) {
    swe_nod_aps(t + k * ldexp(1, -30), SE_MOON, SEFLG_MOSEPH | SEFLG_SPEED,
                SE_NODBIT_OSCU, xn, xd, xp, xa, serr);
    v[k] = xp[3];
  }
  for (k = 0; k + 4 < 24; k++) {
    double d = v[k] - 4 * v[k+1] + 6 * v[k+2] - 4 * v[k+3] + v[k+4];
    s += d * d;
  }
  return sqrt(s / 20 / 70);
}

/* Venus's osculating perihelion distance from swe_nod_aps() on Moshier
 * against the eccentricity vector of swe_calc()'s heliocentric state with
 * DE403's GMS + GM2 (Moshier's planets are fitted to DE404), AU. Upstream
 * takes the Sun's GM as HELGRAVCONST / AUNIT^3, 1.8e-10 below it
 * (UPSTREAM-BUGS 105). */
static double nodaps_gm_off(void)
{
  double t = floor(2460000.3 * 256) / 256, x[6], xn[6], xd[6], xp[6], xa[6];
  double mu = 2.959122082855911e-4 + 7.243452486162703e-10, r, v2, rv, ev[3], e, a;
  char serr[256];
  int j;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_VENUS, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_TRUEPOS
           | SEFLG_XYZ | SEFLG_SPEED, x, serr);
  swe_nod_aps(t, SE_VENUS, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_TRUEPOS,
              SE_NODBIT_OSCU, xn, xd, xp, xa, serr);
  r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
  v2 = x[3] * x[3] + x[4] * x[4] + x[5] * x[5];
  rv = x[0] * x[3] + x[1] * x[4] + x[2] * x[5];
  for (j = 0; j < 3; j++)
    ev[j] = ((v2 - mu / r) * x[j] - rv * x[j+3]) / mu;
  e = sqrt(ev[0] * ev[0] + ev[1] * ev[1] + ev[2] * ev[2]);
  a = 1 / (2 / r - v2 / mu);
  return fabs(a * (1 - e) - xp[2]);
}

/* The Moon's osculating apogee from swe_nod_aps() on Moshier, SEFLG_TRUEPOS,
 * against SE_OSCU_APOG's, the larger of the longitude and latitude
 * differences, deg. Upstream turns swe_calc()'s J2000 state, already in the
 * J2000 frame, by the frame bias once more (UPSTREAM-BUGS 108). */
static double nodaps_bias_off(void)
{
  double t = floor(2460000.3 * 256) / 256, o[6], xn[6], xd[6], xp[6], xa[6], d;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_OSCU_APOG, SEFLG_MOSEPH | SEFLG_TRUEPOS, o, serr);
  swe_nod_aps(t, SE_MOON, SEFLG_MOSEPH | SEFLG_TRUEPOS, SE_NODBIT_OSCU, xn, xd, xp, xa, serr);
  d = fabs(swe_difdeg2n(o[0], xa[0]));
  return fabs(o[1] - xa[1]) > d ? fabs(o[1] - xa[1]) : d;
}

/* Pluto's semi-axis from swe_get_orbital_elements() on Moshier against
 * 1 / (2 / r - v^2 / mu) of swe_calc()'s heliocentric state with DE403's
 * GMS + GM9, relative. Upstream gives Pluto Mercury's mass, and the Sun
 * HELGRAVCONST / AUNIT^3 (UPSTREAM-BUGS 108). */
static double orbel_pluto_off(void)
{
  double t = floor(2460000.3 * 256) / 256, x[6], d[50], r, v2, a;
  double mu = 2.959122082855911e-4 + 2.18869976542597e-12;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_PLUTO, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_TRUEPOS
           | SEFLG_NONUT | SEFLG_XYZ | SEFLG_SPEED, x, serr);
  swe_get_orbital_elements(t, SE_PLUTO, SEFLG_MOSEPH, d, serr);
  r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
  v2 = x[3] * x[3] + x[4] * x[4] + x[5] * x[5];
  a = 1 / (2 / r - v2 / mu);
  return fabs(d[0] - a) / a;
}

/* Jupiter's sidereal period from swe_get_orbital_elements() on Moshier
 * against 2 pi sqrt(a^3 / (GMS + GM5)) with its own a and DE403's GMs,
 * relative. Upstream takes a^1.5 sidereal years, without Jupiter's mass
 * (UPSTREAM-BUGS 111). */
static double orbel_period_off(void)
{
  double t = floor(2460000.3 * 256) / 256, d[50], p;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_get_orbital_elements(t, SE_JUPITER, SEFLG_MOSEPH, d, serr);
  p = 2 * M_PI * sqrt(d[0] * d[0] * d[0] / (2.959122082855911e-4 + 2.825345909524227e-07));
  return fabs(d[10] * 365.242189 - p) / p;
}

/* Venus's true anomaly from swe_get_orbital_elements() on Moshier near
 * aphelion (E 178 deg, 1700), against the angle from the eccentricity
 * vector of the same state to r, deg. Upstream's 2 atan(k sin E / (1 +
 * cos E)) cancels there (UPSTREAM-BUGS 111): 4.8e-9 deg with the
 * ephemeris's GMs, 5.6e-8 with upstream's own (108). */
static double orbel_anom_off(void)
{
  double t = floor((2451545.0 - 300 * 365.25 + 0.37) * 256) / 256, x[6], d[50];
  double mu = 2.959122082855911e-4 + 7.243452486162703e-10, r, v2, rv, ev[3], h[3], c[3], nu;
  char serr[256];
  int j;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_VENUS, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_TRUEPOS
           | SEFLG_NONUT | SEFLG_XYZ | SEFLG_SPEED, x, serr);
  swe_get_orbital_elements(t, SE_VENUS, SEFLG_MOSEPH, d, serr);
  r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
  v2 = x[3] * x[3] + x[4] * x[4] + x[5] * x[5];
  rv = x[0] * x[3] + x[1] * x[4] + x[2] * x[5];
  for (j = 0; j < 3; j++)
    ev[j] = ((v2 - mu / r) * x[j] - rv * x[j+3]) / mu;
  h[0] = x[1] * x[5] - x[2] * x[4];
  h[1] = x[2] * x[3] - x[0] * x[5];
  h[2] = x[0] * x[4] - x[1] * x[3];
  c[0] = ev[1] * x[2] - ev[2] * x[1];
  c[1] = ev[2] * x[0] - ev[0] * x[2];
  c[2] = ev[0] * x[1] - ev[1] * x[0];
  nu = atan2((c[0] * h[0] + c[1] * h[1] + c[2] * h[2])
             / sqrt(h[0] * h[0] + h[1] * h[1] + h[2] * h[2]),
             ev[0] * x[0] + ev[1] * x[1] + ev[2] * x[2]) * RADTODEG;
  return fabs(swe_difdeg2n(d[7], nu));
}

/* The flags swe_fixstar2() returns against swe_calc()'s for Mars, sidereal
 * and heliocentric Moshier: the bits that differ, counted. Upstream returns
 * the caller's iflag, without NONUT (1) and NOABERR|NOGDEFL (2)
 * (UPSTREAM-BUGS 67). */
static double star_ret_flags_off(void)
{
  static const int32 set[2] = {SEFLG_SWIEPH | SEFLG_SIDEREAL, SEFLG_HELCTR | SEFLG_MOSEPH};
  double x[6];
  char serr[256], s[64];
  int k, n = 0;
  int32 a, b, d;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 2; k++) {
    strcpy(s, "Spica");
    a = swe_fixstar2(s, 2460000.3, set[k], x, serr);
    b = swe_calc(2460000.3, SE_MARS, set[k], x, serr);
    for (d = a ^ b; d; d &= d - 1)
      n++;
  }
  return n;
}

/* The sidereal ayanamsa at tjd = 0 against tjd = 1e-9, deg: upstream's
 * emptied nutation cache (tnut 0) answers for JD 0 with zero angles
 * (UPSTREAM-BUGS 67). */
static double ayanamsa_jd0_off(void)
{
  double a0, a1;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_get_ayanamsa_ex(0.0, SEFLG_SWIEPH | SEFLG_SIDEREAL, &a0, serr);
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_get_ayanamsa_ex(1e-9, SEFLG_SWIEPH | SEFLG_SIDEREAL, &a1, serr);
  return fabs(a0 - a1);
}

/* A user delta t set as the very first library call, read back after the
 * first swe_calc_ut(), s off 86.4. Upstream's swe_set_delta_t_userdef()
 * does not initialise the default context, and that first calculation's
 * memset wipes it: 63.83 s, the automatic delta t (UPSTREAM-BUGS 83). The
 * library must be pristine, so main() calls this before anything else and
 * it runs in a forked child, files named by SE_EPHE_PATH. */
static double dt_userdef_first_off(void)
{
  int fd[2];
  double v = -1;
  pid_t pid;
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    double x[6], dt;
    char serr[256];
    close(fd[0]);
    setenv("SE_EPHE_PATH", EPHE, 1);
    swe_set_delta_t_userdef(86.4 / 86400);
    swe_calc_ut(2451545.3, SE_MOON, SEFLG_SWIEPH, x, serr);
    dt = fabs(swe_deltat_ex(2451545.3, SEFLG_SWIEPH, serr) * 86400 - 86.4);
    _exit(write(fd[1], &dt, sizeof dt) == (ssize_t) sizeof dt ? 0 : 3);
  }
  close(fd[1]);
  if (read(fd[0], &v, sizeof v) != (ssize_t) sizeof v)
    v = -1;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return v;
}

/* The first swe_calc_ut() Moon at J2000 against the second, deg, with the
 * files named by SE_EPHE_PATH: a directory of links to them plus its own
 * swe_deltat.txt (70 s over 2000-2002). Upstream's first delta t reads the
 * table along the compiled default path, before swe_calc() sets the
 * implicit one, and takes the built-in 63.83 s: 8.6e-4 deg (UPSTREAM-BUGS
 * 107). Pristine library, so forked first in main(), as above. */
static double calc_ut_first_path_off(void)
{
  static const char *F[] = { "sepl_18.se1", "semo_18.se1", "seas_18.se1" };
  char dir[] = "/tmp/compat-fp-XXXXXX", ephe[PATH_MAX], p[PATH_MAX + 32], q[sizeof dir + 32];
  int fd[2];
  unsigned j;
  double v = -1;
  pid_t pid;
  FILE *fp;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL)
    return -1;
  for (j = 0; j < sizeof F / sizeof F[0]; j++) {
    snprintf(p, sizeof p, "%s/%s", ephe, F[j]);
    snprintf(q, sizeof q, "%s/%s", dir, F[j]);
    if (symlink(p, q) != 0)
      return -1;
  }
  snprintf(q, sizeof q, "%s/swe_deltat.txt", dir);
  if ((fp = fopen(q, "w")) == NULL)
    return -1;
  fputs("2000 70.0\n2001 70.0\n2002 70.0\n", fp);
  fclose(fp);
  if (pipe(fd) == 0) {
    fflush(stdout);
    if ((pid = fork()) == 0) {
      double x[2][6], d;
      char serr[256];
      close(fd[0]);
      setenv("SE_EPHE_PATH", dir, 1);
      swe_calc_ut(2451545.0, SE_MOON, SEFLG_SWIEPH, x[0], serr);
      swe_calc_ut(2451545.0, SE_MOON, SEFLG_SWIEPH, x[1], serr);
      d = fabs(swe_difdeg2n(x[0][0], x[1][0]));
      _exit(write(fd[1], &d, sizeof d) == (ssize_t) sizeof d ? 0 : 3);
    }
    close(fd[1]);
    if (pid < 0 || read(fd[0], &v, sizeof v) != (ssize_t) sizeof v)
      v = -1;
    close(fd[0]);
    if (pid > 0)
      waitpid(pid, NULL, 0);
  }
  unlink(q);
  for (j = 0; j < sizeof F / sizeof F[0]; j++) {
    snprintf(q, sizeof q, "%s/%s", dir, F[j]);
    unlink(q);
  }
  rmdir(dir);
  return v;
}

/* With the files named by SE_EPHE_PATH, how many of two first calls --
 * swe_deltat_ex() and swe_get_ayanamsa_ex_ut(), each in a pristine forked
 * child -- write the "Please call swe_set_ephe_path()" warning into serr.
 * Upstream writes it whenever the call initialises the context: 2
 * (UPSTREAM-BUGS 110; swe_calc_orbel(), this fork's, no longer refuses on
 * it in either build, UPSTREAM-BUGS 112). */
static double path_warn_first_count(void)
{
  int k, n = 0;
  for (k = 0; k < 2; k++) {
    int fd[2];
    char w = -1;
    pid_t pid;
    if (pipe(fd) != 0)
      return -1;
    fflush(stdout);
    if ((pid = fork()) < 0)
      return -1;
    if (pid == 0) {
      double x[6];
      char serr[256] = "";
      close(fd[0]);
      setenv("SE_EPHE_PATH", EPHE, 1);
      if (k == 0)
        swe_deltat_ex(2451545.0, SEFLG_SWIEPH, serr);
      else
        swe_get_ayanamsa_ex_ut(2451545.0, SEFLG_SWIEPH, x, serr);
      w = strstr(serr, "swe_set_ephe_path()") != NULL;
      _exit(write(fd[1], &w, 1) == 1 ? 0 : 3);
    }
    close(fd[1]);
    if (read(fd[0], &w, 1) != 1 || w < 0)
      n = -100;
    close(fd[0]);
    waitpid(pid, NULL, 0);
    n += w;
  }
  return n < 0 ? -1 : n;
}

/* swe_get_tid_acc() after swe_set_astro_models("SE1.80") and a Moon on
 * the default context, against SE_TIDAL_DE406, which the version sets.
 * Upstream's configuration publish (this fork's) takes only the SID group,
 * and the calculation's sync puts the automatic -25.936 back: 0.110
 * (UPSTREAM-BUGS 116). Forked, swe_close() first, so the master is fresh. */
static double models_tid_off(void)
{
  int fd[2];
  double v = -1;
  pid_t pid;
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    double x[6], d;
    char serr[256], m[AS_MAXCH] = "SE1.80";
    close(fd[0]);
    swe_close();
    swe_set_ephe_path(EPHE);
    swe_set_astro_models(m, 0);
    swe_calc_ut(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
    d = fabs(swe_get_tid_acc() - SE_TIDAL_DE406);
    _exit(write(fd[1], &d, sizeof d) == (ssize_t) sizeof d ? 0 : 3);
  }
  close(fd[1]);
  if (read(fd[0], &v, sizeof v) != (ssize_t) sizeof v)
    v = -1;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return v;
}

/* Venus's limiting magnitude at humidity 80 % right after one at 40 %,
 * same instant and place, against the same request with the memos elsewhere
 * first, mag. Upstream's ka() and Deltam() memos are keyed on the Sun's
 * and the object's altitudes alone and answer with the 40 % extinction:
 * 0.67 mag (UPSTREAM-BUGS 82). The caller's swe_set_topo() keeps bug 83
 * out of it. */
static double hel_memo_off(void)
{
  double geo[3] = { 8.55, 47.37, 400 }, dobs[6] = { 36, 1, 0, 0, 0, 0 }, d[50], m[2];
  double a40[4] = { 1013.25, 15, 40, 0 }, a80[4] = { 1013.25, 15, 80, 0 };
  char serr[256], obj[32];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(geo[0], geo[1], geo[2]);
  strcpy(obj, "venus");
  swe_vis_limit_mag(2451545.01, geo, a80, dobs, obj, 0, d, serr);
  strcpy(obj, "venus");
  swe_vis_limit_mag(2451545.0, geo, a80, dobs, obj, 0, d, serr);
  m[0] = d[0];
  strcpy(obj, "venus");
  swe_vis_limit_mag(2451545.01, geo, a40, dobs, obj, 0, d, serr);
  strcpy(obj, "venus");
  swe_vis_limit_mag(2451545.0, geo, a40, dobs, obj, 0, d, serr);
  strcpy(obj, "venus");
  swe_vis_limit_mag(2451545.0, geo, a80, dobs, obj, 0, d, serr);
  m[1] = d[0];
  swe_close();
  return fabs(m[1] - m[0]);
}

/* swe_deltat() in AD 1000 after swe_set_tid_acc(SE_TIDAL_DE406) and a
 * second swe_set_ephe_path(), against right after the setting, s.
 * Upstream's path change resets a manual tidal acceleration to automatic,
 * the file's -25.936: 9.13 s (UPSTREAM-BUGS 92). */
static double tid_acc_path_off(void)
{
  double d0, d1;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_tid_acc(SE_TIDAL_DE406);
  d0 = swe_deltat(2086302.5);
  swe_set_ephe_path(EPHE);
  d1 = swe_deltat(2086302.5);
  swe_set_tid_acc(SE_TIDAL_AUTOMATIC);
  swe_close();
  return fabs(d1 - d0) * 86400;
}

/* delta t in AD 1000 with the models "1,0,0,1" (Stephenson & Morrison
 * 1984) after a second swe_set_ephe_path(), against right after setting
 * them, s. Upstream's path change zeroes the models: 18.27 s
 * (UPSTREAM-BUGS 98). swe_close() resets them after. */
static double models_path_off(void)
{
  double d0, d1;
  char serr[256], m[16];
  strcpy(m, "1,0,0,1");
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_astro_models(m, 0);
  d0 = swe_deltat_ex(2086302.5, SEFLG_SWIEPH, serr);
  swe_set_ephe_path(EPHE);
  d1 = swe_deltat_ex(2086302.5, SEFLG_SWIEPH, serr);
  swe_close();
  return fabs(d1 - d0) * 86400;
}

/* swe_get_tid_acc() right after swe_set_tid_acc(SE_TIDAL_AUTOMATIC),
 * against after the next swe_deltat(). Upstream's automatic is -25.80
 * until a delta t derives the moon file's -25.936: 0.136
 * (UPSTREAM-BUGS 101). */
static double tid_auto_get_off(void)
{
  double g1, g2;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_tid_acc(SE_TIDAL_DE406);
  swe_set_tid_acc(SE_TIDAL_AUTOMATIC);
  g1 = swe_get_tid_acc();
  swe_deltat(2086302.5);
  g2 = swe_get_tid_acc();
  swe_close();
  return fabs(g1 - g2);
}

/* swe_deltat() in AD 1000 on a second thread after swe_set_jpl_file(
 * "de200.eph") on the first, against the first's, s. Upstream's guess
 * follows whether the thread has the file open: 1474.79 for 1305.37,
 * 169.4 s (UPSTREAM-BUGS 103). */
static double dt_thread_val;
static void *dt_thread(void *arg)
{
  (void) arg;
  dt_thread_val = swe_deltat(2086302.5);
  return NULL;
}
static double dt_guess_thread_off(void)
{
  pthread_t th;
  double d0;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  d0 = swe_deltat(2086302.5);
  dt_thread_val = 0;
  if (pthread_create(&th, NULL, dt_thread, NULL) != 0)
    return -1;
  pthread_join(th, NULL);
  swe_close();
  return fabs(dt_thread_val - d0) * 86400;
}

/* swe_time_equ() at J2000 + 6h on a second thread after swe_set_jpl_file(
 * "de200.eph") on the first, against the first's, s. Upstream's Sun is
 * JPL's when the thread has the file open, so the second answered from
 * the Swiss files: 1.03e-3 s (UPSTREAM-BUGS 132). */
static double teq_thread_val;
static void *teq_thread(void *arg)
{
  char serr[256];
  (void) arg;
  swe_time_equ(2451545.25, &teq_thread_val, serr);
  return NULL;
}
static double time_equ_thread_off(void)
{
  pthread_t th;
  double e0 = 0;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  swe_time_equ(2451545.25, &e0, serr);
  teq_thread_val = 0;
  if (pthread_create(&th, NULL, teq_thread, NULL) != 0)
    return -1;
  pthread_join(th, NULL);
  swe_close();
  swe_set_ephe_path(EPHE);
  return fabs(teq_thread_val - e0) * 86400;
}

/* Mars's heliocentric osculating ascending node at tjd = 0 against tjd =
 * 1e-9, deg, each after swe_close(). Upstream forces sweph()'s EMB to be
 * recomputed with the Earth's teval = 0, which at tjd 0 is the Earth slot
 * claiming JD 0: the "EMB" is the slot's Earth, of its own instant, and the
 * barycentric Sun after the light-time step is wrong: 0.025 deg
 * (UPSTREAM-BUGS 71). */
static double nodaps_jd0_off(void)
{
  double a[2][6], d[6], p[6], q[6];
  char serr[256];
  int k;
  for (k = 0; k < 2; k++) {
    swe_close();
    swe_set_ephe_path(EPHE);
    swe_nod_aps(k ? 1e-9 : 0.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_HELCTR,
                SE_NODBIT_OSCU, a[k], d, p, q, serr);
  }
  return fabs(swe_difdeg2n(a[1][0], a[0][0]));
}

/* The SEFLG_SPEED3 speed of the osculating apogee, Moshier, year 2800,
 * against a five-point difference of its longitude over 2^-5 day, deg/day.
 * Upstream takes it over +-0.1 day, and the h^2 term is 1.5e-3
 * (UPSTREAM-BUGS 54). */
static double node3_speed_off(void)
{
  double t = floor((2451545.0 + 800 * 365.25 + 0.1234567) * 256) / 256, h = 1.0 / 32;
  double x[6], f[5][6];
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_OSCU_APOG, SEFLG_MOSEPH | SEFLG_SPEED3, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(t + (k - 2) * h, SE_OSCU_APOG, SEFLG_MOSEPH, f[k], serr);
  for (k = 0; k < 5; k++)
    if (k != 2)
      f[k][0] = swe_difdeg2n(f[k][0], f[2][0]);
  f[2][0] = 0;
  return fabs(x[3] - (f[0][0] - 8 * f[1][0] + 8 * f[3][0] - f[4][0]) / (12 * h));
}

/* The nutation's rate in a speed of date: Polaris's right ascension speed
 * minus the same with NONUT, against a five-point difference of the same
 * two calls' positions over 2^-8 day, deg/day. Upstream takes the nutation
 * angles' rate as a backward difference over 1e-4 day, the rate at t - h/2,
 * 6.5e-10 deg/day short of the fortnightly term, 2.7e-8 in a polar star's
 * right ascension (UPSTREAM-BUGS 58). */
static double nut_rate_off(void)
{
  double t = floor(2451545.0 * 256) / 256, h = 1.0 / 256, xa[6], xb[6], q[5];
  int32 fl = SEFLG_SWIEPH | SEFLG_EQUATORIAL;
  char serr[256], s[64];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  strcpy(s, "Polaris");
  swe_fixstar2(s, t, fl | SEFLG_SPEED, xa, serr);
  strcpy(s, "Polaris");
  swe_fixstar2(s, t, fl | SEFLG_SPEED | SEFLG_NONUT, xb, serr);
  for (k = 0; k < 5; k++) {
    double ya[6], yb[6], df;
    strcpy(s, "Polaris");
    swe_fixstar2(s, t + (k - 2) * h, fl, ya, serr);
    strcpy(s, "Polaris");
    swe_fixstar2(s, t + (k - 2) * h, fl | SEFLG_NONUT, yb, serr);
    df = ya[0] - yb[0];
    q[k] = df - 360 * floor(df / 360 + 0.5);
  }
  return fabs(xa[3] - xb[3] - (8 * (q[3] - q[1]) - (q[4] - q[0])) / (12 * h));
}

/* The mean obliquity's turning inside the nutation matrix: Polaris
 * Australis's right ascension speed minus the same with NONUT, against a
 * five-point difference of the same two calls' positions over 2^-6 day,
 * the worse of 1900 and 2000, deg/day. The rate of N = R1(-eps - deps)
 * R3(-dpsi) R1(eps) held eps, and eps' dpsi, 3e-11 deg/day, is 2e-9 in a
 * right ascension near the pole (UPSTREAM-BUGS 69); upstream has bug 58's
 * backward difference besides. */
static double nut_obliq_rate_off(void)
{
  static const double date[2] = {2415020.3, 2451545.0};
  double h = 1.0 / 64, worst = 0;
  int32 fl = SEFLG_SWIEPH | SEFLG_EQUATORIAL;
  char serr[256], s[64];
  int d, k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (d = 0; d < 2; d++) {
    double t = floor(date[d] * 256) / 256, xa[6], xb[6], q[5], err;
    strcpy(s, "Polaris Australis");
    swe_fixstar2(s, t, fl | SEFLG_SPEED, xa, serr);
    strcpy(s, "Polaris Australis");
    swe_fixstar2(s, t, fl | SEFLG_SPEED | SEFLG_NONUT, xb, serr);
    for (k = 0; k < 5; k++) {
      double ya[6], yb[6], df;
      strcpy(s, "Polaris Australis");
      swe_fixstar2(s, t + (k - 2) * h, fl, ya, serr);
      strcpy(s, "Polaris Australis");
      swe_fixstar2(s, t + (k - 2) * h, fl | SEFLG_NONUT, yb, serr);
      df = ya[0] - yb[0];
      q[k] = df - 360 * floor(df / 360 + 0.5);
    }
    err = fabs(xa[3] - xb[3] - (8 * (q[3] - q[1]) - (q[4] - q[0])) / (12 * h));
    if (err > worst) worst = err;
  }
  return worst;
}

/* The Earth's acceleration in a star's aberration rate: Polaris's right
 * ascension speed against a five-point difference of its right ascension
 * over 2^-5 day, deg/day, on the Swiss files in 2100 and on Moshier in
 * 2000. The Earth's velocity at t + dp was carried on linearly from t - dm
 * and t, the acceleration at t - dm/2: 1.03e-9 in a right ascension near
 * the pole; on Moshier its velocities 1e-5 day apart are rough, 1.56e-8
 * (UPSTREAM-BUGS 76); upstream's star speeds have bugs 49 and 58 besides. */
static double star_aberr_accel_off(int32 eph, double jd)
{
  double t = floor(jd * 256) / 256, h = 1.0 / 32, x[6], q[5];
  int32 fl = eph | SEFLG_EQUATORIAL;
  char serr[256], s[64];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  strcpy(s, "Polaris");
  swe_fixstar2(s, t, fl | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++) {
    double y[6];
    strcpy(s, "Polaris");
    swe_fixstar2(s, t + (k - 2) * h, fl, y, serr);
    q[k] = y[0];
  }
  return fabs(x[3] - (8 * (q[3] - q[1]) - (q[4] - q[0])) / (12 * h));
}

/* The Moshier Earth's velocity, heliocentric J2000, in 2000: its
 * acceleration from the velocities at t +- 1e-5 day against the five-point
 * second difference of the positions over 2^-3 day, relative. The
 * Earth-Moon offset's rate was a difference of the short lunar series over
 * +-2^-8 day, whose rounded arguments made the velocity rough at 2e-14
 * AU/day: 2.1e-6 (UPSTREAM-BUGS 85); upstream's velocity is a backward
 * difference over 1e-4 day (bug 51). */
static double mosh_earth_rough_off(void)
{
  const int32 fl = SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT
                   | SEFLG_TRUEPOS | SEFLG_XYZ | SEFLG_SPEED | SEFLG_ICRS;
  double t = 2451545.0, H = 1.0 / 8, d = 1e-5, q[5][6], xp[6], xm[6], e = 0, an = 0;
  char serr[256];
  int i, k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 5; k++)
    swe_calc(t + (k - 2) * H, SE_EARTH, fl, q[k], serr);
  swe_calc(t + d, SE_EARTH, fl, xp, serr);
  swe_calc(t - d, SE_EARTH, fl, xm, serr);
  for (i = 0; i < 3; i++) {
    double ar = (-q[4][i] + 16 * q[3][i] - 30 * q[2][i] + 16 * q[1][i] - q[0][i]) / (12 * H * H);
    double av = (xp[i+3] - xm[i+3]) / ((t + d) - (t - d));
    e += (av - ar) * (av - ar);
    an += ar * ar;
  }
  return sqrt(e / an);
}

/* The Sun's ecliptic latitude of date at exactly J2000 against the mean of
 * its latitudes at J2000 +- 2^-30 day, deg, Swiss files. Upstream answers
 * swi_precess() at J2000 with the identity, and the Vondrak series there is
 * turned 4.848e-12 rad about x: 2.7e-10 deg (UPSTREAM-BUGS 87). */
static double j2000_lat_off(void)
{
  double x0[6], xm[6], xp[6], d = ldexp(1, -30);
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(2451545.0, SE_SUN, SEFLG_SWIEPH, x0, serr);
  swe_calc(2451545.0 - d, SE_SUN, SEFLG_SWIEPH, xm, serr);
  swe_calc(2451545.0 + d, SE_SUN, SEFLG_SWIEPH, xp, serr);
  return fabs(x0[1] - (xm[1] + xp[1]) / 2);
}

/* The Moshier Earth's velocity, heliocentric J2000, in 1900, against the
 * five-point difference of its positions over 2^-4 day, AU/day. The
 * Earth-Moon offset's rate left out the obliquity's turning, which each
 * position takes at its own date: 1.87e-13 (UPSTREAM-BUGS 85 part 2);
 * upstream's velocity is a backward difference (bug 51). */
static double mosh_earth_vbias_off(void)
{
  const int32 fl = SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT
                   | SEFLG_TRUEPOS | SEFLG_XYZ | SEFLG_SPEED | SEFLG_ICRS;
  double t = 2415020.5, H = 1.0 / 16, q[5][6], x[6], e = 0;
  char serr[256];
  int i, k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 5; k++)
    swe_calc(t + (k - 2) * H, SE_EARTH, fl, q[k], serr);
  swe_calc(t, SE_EARTH, fl, x, serr);
  for (i = 0; i < 3; i++) {
    double r = (8 * (q[3][i] - q[1][i]) - (q[4][i] - q[0][i])) / (12 * H);
    e += (x[i+3] - r) * (x[i+3] - r);
  }
  return sqrt(e);
}

/* UPSTREAM-BUGS 62: VEA (seorbel.txt's set 26, 80,000 AU, 462 days of light
 * time): its geocentric longitude speed against a five-point difference of
 * its longitude, deg/day. The aberration rate there is tau times the
 * Earth's acceleration, from the Sun's pull alone, and upstream has none at
 * all (bug 34); the Moon's 0.6% of it is 6e-7 deg/day on its own. */
static double far_body_moon_pull_off(void)
{
  double t = floor(2456000.7 * 256) / 256, h = 1.0 / 128, x[6], f[5][6], r;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_FICT_OFFSET + 25, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(t + (k - 2) * h, SE_FICT_OFFSET + 25, SEFLG_SWIEPH, f[k], serr);
  r = (swe_difdeg2n(f[0][0], f[2][0]) - 8 * swe_difdeg2n(f[1][0], f[2][0]) +
       8 * swe_difdeg2n(f[3][0], f[2][0]) - swe_difdeg2n(f[4][0], f[2][0])) / (12 * h);
  return fabs(x[3] - r);
}

/* UPSTREAM-BUGS 66: the same on JPL (de200.eph) and Moshier, VEA's
 * geocentric longitude speed at 2456000.7 (2012), the larger of the two,
 * deg/day. The Moon's pull reached only the Swiss path (bug 62) before. */
static double far_body_moon_pull_eph_off(void)
{
  double t = floor(2456000.7 * 256) / 256, h = 1.0 / 128, x[6], f[5][6], r, w = 0;
  const int32 fe[2] = {SEFLG_JPLEPH, SEFLG_MOSEPH};
  char serr[256];
  int k, e;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  for (e = 0; e < 2; e++) {
    swe_calc(t, SE_FICT_OFFSET + 25, fe[e] | SEFLG_SPEED, x, serr);
    for (k = 0; k < 5; k++)
      swe_calc(t + (k - 2) * h, SE_FICT_OFFSET + 25, fe[e], f[k], serr);
    r = (swe_difdeg2n(f[0][0], f[2][0]) - 8 * swe_difdeg2n(f[1][0], f[2][0]) +
         8 * swe_difdeg2n(f[3][0], f[2][0]) - swe_difdeg2n(f[4][0], f[2][0])) / (12 * h);
    if (fabs(x[3] - r) > w)
      w = fabs(x[3] - r);
  }
  return w;
}

/* UPSTREAM-BUGS 65: Kronos from the built-in table (an empty directory on
 * the path) against seorbel.txt's elements, heliocentric J2000 Moshier at
 * JD 2341972.5 (1700), degrees. The built-in semi-axis was 64.81960 for
 * the file's 64.81690: 0 shipped, 8.6e-3 compat. */
static double kronos_builtin_off(void)
{
  const int32 fl = SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS;
  char dir[] = "/tmp/compatkronXXXXXX", serr[256];
  double a[6], b[6];
  if (mkdtemp(dir) == NULL)
    return -1;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(2341972.5, SE_FICT_OFFSET + 3, fl, a, serr);
  swe_close();
  swe_set_ephe_path(dir);
  swe_calc(2341972.5, SE_FICT_OFFSET + 3, fl, b, serr);
  swe_close();
  rmdir(dir);
  swe_set_ephe_path((char *) EPHE);
  return fabs(swe_difdeg2n(a[0], b[0]));
}

/* A topocentric fixed star's deflection by the Sun: the part of it that
 * comes from the observer's site, Regulus near the Sun on 2023-08-10,
 * (topocentric - topocentric NOGDEFL) - (geocentric - geocentric NOGDEFL)
 * as unit vectors of equatorial J2000 XYZ, arcsec. Upstream never passes the
 * topocentric flag to the deflection, so the site is not in it: 0 for 7.4e-6
 * (UPSTREAM-BUGS 61). */
static double star_topo_defl_off(void)
{
  double t = 2460167.3, p[4][6], r[4], s2 = 0;
  const int32 f = SEFLG_SWIEPH | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_J2000 | SEFLG_NONUT | SEFLG_NOABERR;
  const int32 fl[4] = {f | SEFLG_TOPOCTR, f | SEFLG_TOPOCTR | SEFLG_NOGDEFL, f, f | SEFLG_NOGDEFL};
  char serr[256], s[64];
  int k, i;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 47.37, 400);
  for (k = 0; k < 4; k++) {
    strcpy(s, "Regulus");
    swe_fixstar2(s, t, fl[k], p[k], serr);
    r[k] = sqrt(p[k][0] * p[k][0] + p[k][1] * p[k][1] + p[k][2] * p[k][2]);
  }
  for (i = 0; i < 3; i++) {
    double d = (p[0][i] / r[0] - p[1][i] / r[1]) - (p[2][i] / r[2] - p[3][i] / r[3]);
    s2 += d * d;
  }
  return sqrt(s2) * 180 / M_PI * 3600;
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

/* The Moon's mean perigee, geocentric: its longitude speed against a
 * five-point difference of its longitude over 2^-6 day, deg/day. Upstream
 * takes it as the elements' position less their position a day back, and
 * the aux. position's a day on, half a day's curvature: 2e-6 deg/day off,
 * and 2e-5 on the latitude speed (UPSTREAM-BUGS 55; the latitude speed is
 * not used here, it is 5 deg/day off in upstream anyway, bug 38). */
static double moon_perigee_speed_off(void)
{
  double jd = floor(2461338.137 * 256) / 256, h = 1.0 / 64, x[6], f[5][6], u[6], v[5];
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN, u, u, x, u, serr);
  for (k = 0; k < 5; k++)
    swe_nod_aps(jd + (k - 2) * h, SE_MOON, SEFLG_SWIEPH, SE_NODBIT_MEAN, u, u, f[k], u, serr);
  for (k = 0; k < 5; k++)
    v[k] = f[2][0] + swe_difdeg2n(f[k][0], f[2][0]);
  return fabs(x[3] - (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h));
}

/* Jupiter's mean aphelion, heliocentric, true position, at the year -3000:
 * its longitude speed against a five-point difference of its longitude over
 * 2^-6 day, deg/day. Upstream's rate is the elements' linear term only,
 * ep[1] / 36525, without the t^2 and t^3 terms: 3.8e-6 deg/day off at
 * the ends of the range (UPSTREAM-BUGS 64). */
static double planet_apsis_rate_off(void)
{
  double jd = floor((2451545.0 - 5000 * 365.25 + 100.37) * 256) / 256, h = 1.0 / 64;
  double x[6], f[5][6], u[6], v[5];
  int32 fl = SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_TRUEPOS | SEFLG_NOGDEFL;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_JUPITER, fl | SEFLG_SPEED, SE_NODBIT_MEAN, u, u, u, x, serr);
  for (k = 0; k < 5; k++)
    swe_nod_aps(jd + (k - 2) * h, SE_JUPITER, fl, SE_NODBIT_MEAN, u, u, u, f[k], serr);
  for (k = 0; k < 5; k++)
    v[k] = f[2][0] + swe_difdeg2n(f[k][0], f[2][0]);
  return fabs(x[3] - (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h));
}

/* UPSTREAM-BUGS 70: Neptune's mean aphelion, heliocentric, default flags
 * (aberration seen from the Sun), at J2000 + 100 days: its distance rate
 * against a five-point difference of its distance over 2^-6 day, AU/day.
 * Upstream adds the Sun's V(t) - V(t - dt) whole, radial part and all,
 * ~2e-9 (bug 64's t^2 and t^3 terms vanish near J2000). */
static double planet_aberr_rate_off(void)
{
  double jd = floor((2451545.0 + 100.37) * 256) / 256, h = 1.0 / 64;
  double x[6], f[5][6], u[6];
  int32 fl = SEFLG_SWIEPH | SEFLG_HELCTR;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_NEPTUNE, fl | SEFLG_SPEED, SE_NODBIT_MEAN, u, u, u, x, serr);
  for (k = 0; k < 5; k++)
    swe_nod_aps(jd + (k - 2) * h, SE_NEPTUNE, fl, SE_NODBIT_MEAN, u, u, u, f[k], serr);
  return fabs(x[5] - (f[0][2] - 8 * f[1][2] + 8 * f[3][2] - f[4][2]) / (12 * h));
}

/* UPSTREAM-BUGS 46: Mars's osculating descending node, perihelion and
 * aphelion, geocentric, with and without SEFLG_SPEED: the largest move in
 * longitude or latitude, mas. Upstream's aberration-rate call leaves the
 * Earth of the point before for the three after the first, ~30 mas. */
static double nodaps_speed_flag_move(void)
{
  double jd = 2461338.137, a[4][6], b[4][6], w = 0;
  char serr[256];
  int j;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_OSCU, a[0], a[1], a[2], a[3], serr);
  swe_nod_aps(jd, SE_MARS, SEFLG_SWIEPH, SE_NODBIT_OSCU, b[0], b[1], b[2], b[3], serr);
  for (j = 0; j < 4; j++) {
    w = fmax(w, fabs(swe_difdeg2n(a[j][0], b[j][0])) * 3.6e6);
    w = fmax(w, fabs(a[j][1] - b[j][1]) * 3.6e6);
  }
  return w;
}

/* UPSTREAM-BUGS 47: the Moon's mean node from swe_nod_aps() against
 * swe_calc(SE_MEAN_NODE) under SEFLG_J2000 on 2026-10-24, the larger of
 * the longitude and latitude differences, arcsec. Upstream turns the
 * point of date to the equator by J2000's obliquity and nutates J2000
 * coordinates: 7.9". */
static double nodaps_j2000_off(void)
{
  double jd = 2461337.5, a[6], x[6], u[6];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_MOON, SEFLG_SWIEPH | SEFLG_J2000, SE_NODBIT_MEAN, a, u, u, u, serr);
  swe_calc(jd, SE_MEAN_NODE, SEFLG_SWIEPH | SEFLG_J2000, x, serr);
  return fmax(fabs(swe_difdeg2n(a[0], x[0])), fabs(a[1] - x[1])) * 3600;
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

/* UPSTREAM-BUGS 63: Mercury's heliocentric speed against a five-point
 * difference of its longitude over 2^-8 day, nine dates 1800-2335,
 * deg/day. The light-time position was read at teval - dt rounded to a
 * double, and the heliocentric light-time rate took the body relative to
 * the Sun: 2.1e-7 before the fix, 1.8e-9 after. Upstream has no
 * heliocentric rate at all (bug 35): 1.5e-4. */
static double mercury_lt_round_off(void)
{
  double w = 0, d;
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 9; k++)
    if ((d = lon_speed_off(SE_MERCURY, SEFLG_SWIEPH | SEFLG_HELCTR,
                           2378496.5 + k * 24413.7 + 0.137)) > w)
      w = d;
  return w;
}

/* UPSTREAM-BUGS 81: Waldemath's geocentric longitude speed, default
 * flags, against a five-point difference over 2^-8 day at nine dates
 * 1800-2335, deg/day. The elements were read at the rounded emission
 * instant, so the Earth-centred orbit's focus jittered by the Earth's
 * motion over an ulp of the date. Compat also has bug 48's speed. */
static double waldemath_lt_round_off(void)
{
  double w = 0, d;
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 9; k++)
    if ((d = lon_speed_off(SE_FICT_OFFSET + 18, SEFLG_SWIEPH,
                           2378496.5 + k * 24413.7 + 0.137)) > w)
      w = d;
  return w;
}

/* UPSTREAM-BUGS 81 part 2: Vulcan's heliocentric true longitude at -3000,
 * Moshier, the largest fourth difference of eight places 2^-9 day apart,
 * deg. Its mean anomaly's 707550.7341 T was rounded as 3.5e7 deg before
 * being reduced to 0..360. */
static double vulcan_true_jitter_off(void)
{
  double w = 0, d[8][6], e, jd = floor((2451545.0 - 5000 * 365.25) * 512) / 512;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 8; k++)
    swe_calc(jd + k / 512.0, SE_FICT_OFFSET + 15,
             SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_TRUEPOS, d[k], serr);
  for (k = 0; k + 4 < 8; k++) {
    e = d[k][0] - 4 * d[k + 1][0] + 6 * d[k + 2][0] - 4 * d[k + 3][0] + d[k + 4][0];
    if (fabs(e) > w)
      w = fabs(e);
  }
  return w;
}

/* UPSTREAM-BUGS 81 part 3: swe_calc_orbel()'s position for an orbit with
 * a = 0.137 AU (Vulcan's), equatorial J2000 XYZ (which needs no obliquity,
 * so compat's first-call defect, bug 88, does not hide it), with
 * SEFLG_SPEED against without it, nine dates 1800-2335, AU. The light time was a straight line, and
 * the orbit was read at t - tau only for the speed. */
static double orbel_lt_flag_off(void)
{
  double w = 0, a[6], b[6], el[6] = {10, 0.13744, 0.019, 30, 47, 7.5};
  char serr[256];
  int k, i;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 9; k++) {
    double t = floor((2378496.5 + k * 24413.7 + 0.137) * 256) / 256;
    int32 fl = SEFLG_SWIEPH | SEFLG_EQUATORIAL | SEFLG_J2000 | SEFLG_NONUT | SEFLG_XYZ;
    swe_calc_orbel(t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, fl, a, serr);
    swe_calc_orbel(t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, fl | SEFLG_SPEED, b, serr);
    for (i = 0; i < 3; i++)
      if (fabs(a[i] - b[i]) > w)
        w = fabs(a[i] - b[i]);
  }
  return w;
}

/* UPSTREAM-BUGS 88: swe_calc_orbel() as a context's first call, its
 * ecliptic longitude against the same call made after a swe_calc() at
 * another instant, deg. The caches of date were never brought to tjd, so
 * the first call answered zeros and a later one from another instant's
 * obliquity and nutation. */
static double orbel_first_call_off(void)
{
  double a[6], b[6], el[6] = {10, 0.13744, 0.019, 30, 47, 7.5}, t = 2458850.5;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc_orbel(t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, SEFLG_SWIEPH, a, serr);
  swe_calc(t - 3000.5, SE_MARS, SEFLG_SWIEPH, b, serr);
  swe_calc_orbel(t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, SEFLG_SWIEPH, b, serr);
  return fabs(swe_difdeg2n(a[0], b[0]));
}

/* UPSTREAM-BUGS 77: swe_calc_pctr()'s distance rate of Mercury seen from
 * Mars, default flags, against a five-point difference of its distance
 * over 2^-8 day, nine dates 1800-2335, AU/day. The aberration rate added
 * the centre's velocity change over the light time whole, radial part and
 * all. */
static double pctr_dist_rate_off(void)
{
  double w = 0, h = 1.0 / 256, x[6], f[5][6], d;
  char serr[256];
  int j, k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (j = 0; j < 9; j++) {
    double t = floor((2378496.5 + j * 24413.7 + 0.137) * 256) / 256;
    swe_calc_pctr(t, SE_MERCURY, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
    for (k = 0; k < 5; k++)
      swe_calc_pctr(t + (k - 2) * h, SE_MERCURY, SE_MARS, SEFLG_SWIEPH, f[k], serr);
    d = fabs(x[5] - (8 * (f[3][2] - f[1][2]) - (f[4][2] - f[0][2])) / (12 * h));
    if (d > w)
      w = d;
  }
  return w;
}

/* UPSTREAM-BUGS 77: swe_calc_pctr()'s deflection of the Sun seen from
 * Jupiter, arcsec, the worst of nine dates 1800-2335: the deflection was
 * the Earth's geometry from the save area, and deflected the Sun by
 * itself. Shipped none. */
static double pctr_sun_defl_off(void)
{
  double w = 0, a[6], b[6], d;
  char serr[256];
  int j;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (j = 0; j < 9; j++) {
    double t = 2378496.5 + j * 24413.7 + 0.137;
    swe_calc_pctr(t, SE_SUN, SE_JUPITER, SEFLG_SWIEPH | SEFLG_NOABERR, a, serr);
    swe_calc_pctr(t, SE_SUN, SE_JUPITER, SEFLG_SWIEPH | SEFLG_NOABERR | SEFLG_NOGDEFL, b, serr);
    d = fabs(swe_difdeg2n(a[0], b[0])) * 3600 + fabs(a[1] - b[1]) * 3600;
    if (d > w)
      w = d;
  }
  return w;
}

/* UPSTREAM-BUGS 78: the heliocentric Earth's longitude speed against a
 * five-point difference over 2^-8 day, nine dates 1800-2335, deg/day. It
 * had no light-time rate, the planets' bug 35: 1.7e-6 upstream and
 * before the fix, 6.0e-10 after. */
static double earth_helio_lt_rate_off(void)
{
  double w = 0, d;
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 9; k++)
    if ((d = lon_speed_off(SE_EARTH, SEFLG_SWIEPH | SEFLG_HELCTR,
                           2378496.5 + k * 24413.7 + 0.137)) > w)
      w = d;
  return w;
}

/* UPSTREAM-BUGS 102: Ceres heliocentric under Moshier, read from the same
 * file as on the Swiss path, against the Swiss files' place, five dates
 * 1900-2100 mid-way between the files' yearly joins of the barycentric
 * Sun, deg. Moshier's light time held the Sun still: 2.8e-6 before the
 * fix, 1.3e-5 upstream (whose heliocentric light time is also bug 26's),
 * 1.9e-10 after. */
static double mosh_sun_lt_off(void)
{
  double w = 0, d, a[6], b[6], ra, rb;
  int32 fl = SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_HELCTR;
  char serr[256];
  int k, i;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 5; k++) {
    double t = 2451547.5 + 182.625 + 365.25 * (-100 + 50 * k);
    if (swe_calc(t, SE_CERES, fl | SEFLG_MOSEPH, a, serr) < 0
        || swe_calc(t, SE_CERES, fl | SEFLG_SWIEPH, b, serr) < 0)
      return 1;
    ra = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    rb = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
    for (d = 0, i = 0; i < 3; i++)
      d += (a[i] / ra - b[i] / rb) * (a[i] / ra - b[i] / rb);
    d = sqrt(d) * 180 / M_PI;
    if (d > w)
      w = d;
  }
  return w;
}

/* UPSTREAM-BUGS 109: Moshier against the Swiss files at five dates
 * 1900-2100, deg, the larger of two: Sirius without aberration, whose
 * parallax was taken from the heliocentric Earth (9.2e-7 before the fix),
 * and Cupido heliocentric, whose light time held the Sun still (2.9e-6);
 * 3.1e-6 upstream, 1.2e-10 after. */
static double mosh_bary_off(void)
{
  double w = 0, d, a[6], b[6], ra, rb;
  int32 fl = SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ;
  char serr[256], s[SE_MAX_STNAME * 2 + 1];
  int k, i, j;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 5; k++)
    for (j = 0; j < 2; j++) {
      double t = 2451547.5 + 182.625 + 365.25 * (-100 + 50 * k);
      if (j == 0) {
        strcpy(s, "Sirius");
        if (swe_fixstar2(s, t, fl | SEFLG_NOABERR | SEFLG_MOSEPH, a, serr) < 0)
          return 1;
        strcpy(s, "Sirius");
        if (swe_fixstar2(s, t, fl | SEFLG_NOABERR | SEFLG_SWIEPH, b, serr) < 0)
          return 1;
      } else if (swe_calc(t, SE_CUPIDO, fl | SEFLG_HELCTR | SEFLG_MOSEPH, a, serr) < 0
                 || swe_calc(t, SE_CUPIDO, fl | SEFLG_HELCTR | SEFLG_SWIEPH, b, serr) < 0)
        return 1;
      ra = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
      rb = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
      for (d = 0, i = 0; i < 3; i++)
        d += (a[i] / ra - b[i] / rb) * (a[i] / ra - b[i] / rb);
      d = sqrt(d) * 180 / M_PI;
      if (d > w)
        w = d;
    }
  return w;
}

/* UPSTREAM-BUGS 125: VEA (seorbel.txt's set at 80000 AU) heliocentric,
 * Moshier against the Swiss files at five dates 1900-2100, deg. Over its
 * light time of 462 days Moshier carried the Sun about the barycentre by
 * a Taylor step: 2.7e-7 before the fix; the Sun held still upstream; the
 * Sun's own place past a day of light time after. */
static double mosh_far_lt_off(void)
{
  double w = 0, d, a[6], b[6], ra, rb;
  int32 fl = SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_HELCTR;
  char serr[256];
  int k, i;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 5; k++) {
    double t = 2451547.5 + 182.625 + 365.25 * (-100 + 50 * k);
    if (swe_calc(t, SE_FICT_OFFSET + 25, fl | SEFLG_MOSEPH, a, serr) < 0
        || swe_calc(t, SE_FICT_OFFSET + 25, fl | SEFLG_SWIEPH, b, serr) < 0)
      return 1;
    ra = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    rb = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
    for (d = 0, i = 0; i < 3; i++)
      d += (a[i] / ra - b[i] / rb) * (a[i] / ra - b[i] / rb);
    d = sqrt(d) * 180 / M_PI;
    if (d > w)
      w = d;
  }
  return w;
}

/* UPSTREAM-BUGS 130: swe_calc_orbel()'s geocentric true J2000 equatorial
 * XYZ place of an orbit with a = 1.2 AU against its heliocentric place less
 * swe_calc()'s heliocentric Earth, Swiss files, four dates through 2025,
 * AU. The orbel path never paired the frame bias (bug 20's, on its copy of
 * the fictitious path): the ICRS Earth in a J2000 answer, 3.2e-8 to
 * 1.1e-7 AU by the Earth's direction; the upstream build's fictitious
 * path, which it follows there, has no pair either. */
static double orbel_bias_off(void)
{
  double w = 0, el[6] = {40, 1.2, 0.2, 30, 80, 12}, xg[6], xh[6], xe[6];
  int32 fl = SEFLG_SWIEPH | SEFLG_TRUEPOS | SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ;
  char serr[256];
  int k, i;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 4; k++) {
    double t = 2460676.5 + 91.3 * k;
    if (swe_calc_orbel(t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, fl, xg, serr) < 0
        || swe_calc_orbel(t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, fl | SEFLG_HELCTR, xh, serr) < 0
        || swe_calc(t, SE_EARTH, fl | SEFLG_HELCTR, xe, serr) < 0)
      return 1;
    for (i = 0; i < 3; i++)
      if (fabs(xg[i] - (xh[i] - xe[i])) > w)
        w = fabs(xg[i] - (xh[i] - xe[i]));
  }
  return w;
}

/* UPSTREAM-BUGS 139: a Swiss barycentric Mars at 2000 Jul 1.5 in an empty
 * directory with the fallback allowed: the return. It fell to Moshier past
 * swecalc()'s refusal of a barycentric Moshier request and was answered
 * with Moshier's heliocentric place under SEFLG_BARYCTR | SEFLG_MOSEPH
 * (18180), 7.2e-3 AU off the files' barycentric Mars; refused now as that
 * request is, -1. */
static double fallback_mosh_bary_ret(void)
{
  char dir[] = "/tmp/compatfbXXXXXX", serr[256];
  double x[6], r;
  int fb;
  if (mkdtemp(dir) == NULL)
    return 1e9;
  swe_close();
  swe_set_ephe_path(dir);
  fb = swe_get_ephe_fallback();
  swe_set_ephe_fallback(1);
  r = swe_calc(2451727.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_BARYCTR, x, serr);
  swe_set_ephe_fallback(fb);
  swe_close();
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return r;
}

/* UPSTREAM-BUGS 150: swe_calc_pctr() of Mars from Jupiter at 2000 Jul
 * 1.5, JPL asked, the JPL file named missing and the Swiss files there,
 * the fallback allowed: the return. Swiss answered, bit for bit the Swiss
 * request, and the return was the caller's flags, 257 (SEFLG_JPLEPH |
 * SEFLG_SPEED); now the ephemeris that answered, 258. The JPL file the
 * cases around it name, de200.eph, is named again after. */
static double pctr_ephe_ret(void)
{
  double x[6], r;
  char serr[256];
  int fb;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de999.eph");	/* no such file */
  fb = swe_get_ephe_fallback();
  swe_set_ephe_fallback(1);
  r = swe_calc_pctr(2451727.0, SE_MARS, SE_JUPITER, SEFLG_JPLEPH | SEFLG_SPEED, x, serr);
  swe_set_ephe_fallback(fb);
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  return r;
}

/* UPSTREAM-BUGS 158: swe_calc_pctr() of Mars from Ceres with SEFLG_SPEED
 * at 1800 Jan 1 + 2^-10 day, in a directory of seas_18.se1 (Ceres from
 * 1800 Jan 1) and the planet and Moon files of 1200-2400: the return.
 * The centre's read at the light-time instant, 0.024 day before, fails,
 * and the body's read overwrote its return: 258 with the speed taken from
 * a zeroed centre (0.130 deg/day off, 5.09e-3 in the shipped build before
 * the fix); refused now, -1. */
static double pctr_ctr_edge_ret(void)
{
  static const char *f[] = { "sepl_12.se1", "sepl_18.se1", "semo_12.se1", "semo_18.se1",
                             "seas_18.se1" };
  char base[400], p[480], q[PATH_MAX + 20], src[PATH_MAX];
  const char *tmp = getenv("TMPDIR");
  double x[6], r = 1e9;
  char serr[256];
  int i, n = 0;
  snprintf(base, sizeof base, "%s/compat158.XXXXXX", tmp && *tmp ? tmp : "/tmp");
  if (mkdtemp(base) == NULL)
    return r;
  for (; n < 5; n++) {
    snprintf(q, sizeof q, "%s/%s", EPHE, f[n]);
    snprintf(p, sizeof p, "%s/%s", base, f[n]);
    if (realpath(q, src) == NULL || symlink(src, p) != 0)
      goto out;
  }
  swe_close();
  swe_set_ephe_path(base);
  r = swe_calc_pctr(2378496.5 + 1.0 / 1024, SE_MARS, SE_CERES, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
out:
  swe_close();
  swe_set_ephe_path(EPHE);
  for (i = 0; i < n; i++) {
    snprintf(p, sizeof p, "%s/%s", base, f[i]);
    unlink(p);
  }
  rmdir(base);
  return r;
}

/* UPSTREAM-BUGS 149: swe_nod_aps() of Mars on Moshier, barycentric,
 * osculating, at 2000 Jul 1.5: the return. It answered 0 (OK) with
 * Moshier's heliocentric points, the ascending node 49.5626 deg where the
 * files' barycentric one is 49.6398; refused now as swe_calc() refuses a
 * barycentric Moshier request, -1. */
static double nodaps_mosh_bary_ret(void)
{
  double xn[6], xd[6], xp[6], xa[6];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  return swe_nod_aps(2451727.0, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED | SEFLG_BARYCTR,
                     SE_NODBIT_OSCU, xn, xd, xp, xa, serr);
}

/* UPSTREAM-BUGS 167: swe_nod_aps() of the Sun on Moshier, geocentric,
 * mean, at 2000 Jul 1.5: its perigee's distance less the Earth's
 * heliocentric perihelion distance, AU. The observer was the Earth, as for
 * a planet, and the perigee came out 1.9993 AU away at 281.4259 deg; seen
 * from the Sun now, as on the files, at the Earth's 0.9833 AU. */
static double nodaps_mosh_sun_off(void)
{
  double s[24], e[24];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  if (swe_nod_aps(2451727.0, SE_SUN, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN,
                  s, s + 6, s + 12, s + 18, serr) < 0
      || swe_nod_aps(2451727.0, SE_EARTH, SEFLG_MOSEPH | SEFLG_SPEED | SEFLG_HELCTR, SE_NODBIT_MEAN,
                     e, e + 6, e + 12, e + 18, serr) < 0)
    return 1e9;
  return fabs(s[14] - e[14]);
}

/* UPSTREAM-BUGS 168: swe_nod_aps() of Mars at 2000 Jul 1.5, geocentric,
 * osculating, SEFLG_SWIEPH | SEFLG_MOSEPH against SEFLG_SWIEPH: the
 * ascending node's longitude difference, deg. Answered from the files,
 * the mixed request took Moshier's frame from its SEFLG_MOSEPH bit and
 * added no barycentric Sun: 69.7303 deg for 69.8382; now one ephemeris
 * bit, swe_calc()'s, and the frame of the ephemeris that answered. */
static double nodaps_mixed_eph_off(void)
{
  double a[24], b[24];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  if (swe_nod_aps(2451727.0, SE_MARS, SEFLG_SWIEPH | SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_OSCU,
                  a, a + 6, a + 12, a + 18, serr) < 0
      || swe_nod_aps(2451727.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_OSCU,
                     b, b + 6, b + 12, b + 18, serr) < 0)
    return 1e9;
  return fabs(swe_difdeg2n(a[0], b[0]));
}

/* UPSTREAM-BUGS 169: swe_nod_aps() of Mars at 2000 Jul 1.5, Swiss files,
 * osculating, SEFLG_HELCTR | SEFLG_BARYCTR against SEFLG_BARYCTR: the
 * ascending node's longitude difference, deg. SEFLG_HELCTR won, the points
 * heliocentric (49.5625 deg for the barycentric 49.6398), where swe_calc()
 * takes SEFLG_BARYCTR; now swe_calc()'s precedence. */
static double nodaps_helbary_off(void)
{
  double a[24], b[24];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  if (swe_nod_aps(2451727.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_HELCTR | SEFLG_BARYCTR,
                  SE_NODBIT_OSCU, a, a + 6, a + 12, a + 18, serr) < 0
      || swe_nod_aps(2451727.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_BARYCTR, SE_NODBIT_OSCU,
                     b, b + 6, b + 12, b + 18, serr) < 0)
    return 1e9;
  return fabs(swe_difdeg2n(a[0], b[0]));
}

/* UPSTREAM-BUGS 176: swe_nod_aps() of Mars at 1500 Jan 1, heliocentric,
 * osculating, SEFLG_JPLEPH on de200 with the fallback allowed (Moshier
 * answers outside de200's years) against SEFLG_MOSEPH: the ascending
 * node's longitude difference, deg. The ellipse took the frame bias of the
 * DE number asked, DE200's none, for Moshier's state: 1.58e-4 deg; now
 * the bias of the ephemeris that answered, Moshier's request's. */
static double nodaps_jplfb_bias_off(void)
{
  double a[24], b[24];
  char serr[256];
  int fb;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  fb = swe_get_ephe_fallback();
  swe_set_ephe_fallback(1);
  if (swe_nod_aps(2268923.5, SE_MARS, SEFLG_JPLEPH | SEFLG_SPEED | SEFLG_HELCTR, SE_NODBIT_OSCU,
                  a, a + 6, a + 12, a + 18, serr) < 0
      || swe_nod_aps(2268923.5, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED | SEFLG_HELCTR, SE_NODBIT_OSCU,
                     b, b + 6, b + 12, b + 18, serr) < 0) {
    swe_set_ephe_fallback(fb);
    return 1e9;
  }
  swe_set_ephe_fallback(fb);
  return fabs(swe_difdeg2n(a[0], b[0]));
}

/* UPSTREAM-BUGS 178: swe_nod_aps() of Jupiter at J2000, heliocentric,
 * mean, Swiss files: the descending node's longitude, the default flags
 * against SEFLG_NOABERR | SEFLG_NOGDEFL, deg. The node, at 280.46 deg,
 * is in line with the Sun from the Earth (280.37), and the point took
 * swi_deflect_light()'s deflection as if the Earth looked along it:
 * 9.62e-4 deg (8.22e-4 upstream); now none, as swe_calc()'s heliocentric
 * places. */
static double nodaps_hel_defl_off(void)
{
  double a[24], b[24];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  if (swe_nod_aps(2451545.0, SE_JUPITER, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_HELCTR, SE_NODBIT_MEAN,
                  a, a + 6, a + 12, a + 18, serr) < 0
      || swe_nod_aps(2451545.0, SE_JUPITER, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_HELCTR | SEFLG_NOABERR
                     | SEFLG_NOGDEFL, SE_NODBIT_MEAN, b, b + 6, b + 12, b + 18, serr) < 0)
    return 1e9;
  return fabs(swe_difdeg2n(a[6], b[6]));
}

/* UPSTREAM-BUGS 180: swe_nod_aps() of the Earth on Moshier at 2000 Jan
 * 30.5, heliocentric, osculating: the perihelion's longitude speed less
 * the five-point difference of its positions over 2^-6 day, deg/day. The
 * EMB was Moshier's Earth plus Moshier's Moon, where the Earth is made
 * with the short lunar series' Moon: the perihelion wobbled with their
 * difference, 3.18e-8 deg/day in the default build before the fix; now
 * the series' EMB, 7e-11. Upstream's speeds are rougher besides (bug 80):
 * 1.2e-3. */
static double nodaps_mosh_emb_rate_off(void)
{
  double x[24], p[5][24], d1, d2;
  const double h = 1.0 / 64, jd = 2451574.0;
  char serr[256];
  int j;
  swe_close();
  swe_set_ephe_path(EPHE);
  if (swe_nod_aps(jd, SE_EARTH, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_SPEED, SE_NODBIT_OSCU,
                  x, x + 6, x + 12, x + 18, serr) < 0)
    return 1e9;
  for (j = 0; j < 5; j++)
    if (swe_nod_aps(jd + (j - 2) * h, SE_EARTH, SEFLG_MOSEPH | SEFLG_HELCTR, SE_NODBIT_OSCU,
                    p[j], p[j] + 6, p[j] + 12, p[j] + 18, serr) < 0)
      return 1e9;
  d1 = swe_difdeg2n(p[3][12], p[1][12]);
  d2 = swe_difdeg2n(p[4][12], p[0][12]);
  return fabs(x[15] - (8 * d1 - d2) / (12 * h));
}

/* UPSTREAM-BUGS 170: swe_nod_aps_ut() of Mars at 1900 Jan 0.75 UT,
 * osculating, geocentric, Swiss asked in an empty directory with the
 * fallback allowed, against the Moshier request: the perihelion's
 * longitude difference, deg. Fallen to Moshier, it kept the Swiss delta t
 * (-1.9915 s for Moshier's -2.0521 s), 2.8e-7 deg; now delta t is redone
 * with the ephemeris that answered, as swe_calc_ut() does. */
static double nodaps_ut_fallback_off(void)
{
  char dir[] = "/tmp/compatnutXXXXXX", serr[256];
  double a[24], b[24], r = 1e9;
  int fb;
  if (mkdtemp(dir) == NULL)
    return 1e9;
  swe_close();
  swe_set_ephe_path(dir);
  fb = swe_get_ephe_fallback();
  swe_set_ephe_fallback(1);
  if (swe_nod_aps_ut(2415020.25, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_OSCU,
                     a, a + 6, a + 12, a + 18, serr) >= 0
      && swe_nod_aps_ut(2415020.25, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_OSCU,
                        b, b + 6, b + 12, b + 18, serr) >= 0)
    r = fabs(swe_difdeg2n(a[12], b[12]));
  swe_set_ephe_fallback(fb);
  swe_close();
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return r;
}

/* UPSTREAM-BUGS 140: the site's move for a user delta t a quarter ulp of
 * UT1 longer, at TT 2000 Jan 1.5, Zurich, Swiss files: the site as the
 * Sun's geocentric less topocentric true J2000 equatorial ICRS XYZ place,
 * its displacement against -v 2^-33 day, v the site's velocity, relative.
 * UT1 = TT - delta t is a double, 2^-31 day apart there, and the site was
 * placed by it rounded: a quarter ulp did not move it (1); the rounding
 * recovered, it moves by its velocity over the 2^-33 day (0). */
static double topo_ut_round_off(void)
{
  double xs[2][6], v[3], d = 0, e = 0;
  int32 fl = SEFLG_SWIEPH | SEFLG_TRUEPOS | SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ
             | SEFLG_ICRS | SEFLG_SPEED;
  char serr[256];
  int k, i;
  for (k = 0; k < 2; k++) {
    double xg[6], xt[6];
    swe_close();
    swe_set_ephe_path(EPHE);
    swe_set_topo(8.55, 47.37, 400);
    swe_set_delta_t_userdef(1.0 / 2048 + (k ? ldexp(1, -33) : 0));
    if (swe_calc(2451545.0, SE_SUN, fl, xg, serr) < 0
        || swe_calc(2451545.0, SE_SUN, fl | SEFLG_TOPOCTR, xt, serr) < 0) {
      swe_set_delta_t_userdef(SE_DELTAT_AUTOMATIC);
      return -1;
    }
    for (i = 0; i < 6; i++)
      xs[k][i] = xg[i] - xt[i];
  }
  swe_set_delta_t_userdef(SE_DELTAT_AUTOMATIC);	/* swe_close() keeps it */
  for (i = 0; i < 3; i++) {
    v[i] = -xs[0][i + 3] * ldexp(1, -33);
    d += (xs[1][i] - xs[0][i] - v[i]) * (xs[1][i] - xs[0][i] - v[i]);
    e += v[i] * v[i];
  }
  return sqrt(d / e);
}

/* UPSTREAM-BUGS 86: the JPL barycentric Sun's sidereal (Lahiri) longitude,
 * right after a request for Mars ten years on, against its tropical
 * longitude less the ayanamsa at its own date, deg. The Sun is answered in
 * the Earth's slot, whose instant on JPL was the Mars request's: 0.14 deg
 * upstream and before the fix, 0 after. */
static double bary_sun_sid_off(void)
{
  double t = 2451545.25, xs[6], xt[6], z[6], aya = 0, d;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_get_ayanamsa_ex(t, SEFLG_JPLEPH, &aya, serr);
  swe_calc(t + 3652.5, SE_MARS, SEFLG_JPLEPH, z, serr);
  swe_calc(t, SE_SUN, SEFLG_JPLEPH | SEFLG_BARYCTR | SEFLG_SIDEREAL, xs, serr);
  swe_calc(t, SE_SUN, SEFLG_JPLEPH | SEFLG_BARYCTR, xt, serr);
  d = fabs(swe_difdeg2n(xs[0], swe_degnorm(xt[0] - aya)));
  swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  return d;
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

/* UPSTREAM-BUGS 80: Venus's osculating perihelion, heliocentric, at J2000
 * + 100 days: its longitude speed against a five-point difference over
 * 2^-6 day, deg/day. Upstream differences the points over 7e-4 day, where
 * their eccentricity's rounding (sqrt(1 - p / a), 1e-16 / e^2) is 5e-10
 * deg of jitter: 4.7e-7. */
static double oscu_apsis_speed_off(void)
{
  double jd = floor((2451545.0 + 100.37) * 256) / 256, h = 1.0 / 64;
  double x[6], f[5][6], u[6], v[5];
  int32 fl = SEFLG_SWIEPH | SEFLG_HELCTR;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_VENUS, fl | SEFLG_SPEED, SE_NODBIT_OSCU, u, u, x, u, serr);
  for (k = 0; k < 5; k++)
    swe_nod_aps(jd + (k - 2) * h, SE_VENUS, fl, SE_NODBIT_OSCU, u, u, f[k], u, serr);
  for (k = 0; k < 5; k++)
    v[k] = f[2][0] + swe_difdeg2n(f[k][0], f[2][0]);
  return fabs(x[3] - (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h));
}

/* UPSTREAM-BUGS 79: Saturn's mean ascending node seen from the equator on
 * 2026-10-24, default flags, its latitude speed against the slope of a
 * least-squares polynomial (degree 8, 257 places 2^-11 day apart; a
 * five-point difference cannot resolve it, see tests/nodapstopo.c),
 * deg/day. The site's acceleration, for the aberration's rate, was a
 * difference over +-2^-11 day, 1.6e-6 low: 3.5e-10 (upstream, through
 * bug 34's chord, 8e-6). */
static double topo_accel_off(void)
{
  enum { NS = 128, ND = 9 };
  double jd = floor(2461338.137 * 256) / 256, h = 1.0 / 2048, x[6], f[6], u[6];
  double a[ND][ND + 1], c[ND], v[2 * NS + 1];
  char serr[256];
  int32 fl = SEFLG_SWIEPH | SEFLG_TOPOCTR;
  int i, j, k, r;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 0, 400);
  swe_nod_aps(jd, SE_SATURN, fl | SEFLG_SPEED, SE_NODBIT_MEAN, x, u, u, u, serr);
  for (k = -NS; k <= NS; k++) {
    swe_nod_aps(jd + k * h, SE_SATURN, fl, SE_NODBIT_MEAN, f, u, u, u, serr);
    v[k + NS] = f[1] - x[1];
  }
  for (i = 0; i < ND; i++)
    for (j = 0; j <= ND; j++)
      a[i][j] = 0;
  for (k = -NS; k <= NS; k++) {
    double s = (double) k / NS, p[ND];
    p[0] = 1;
    for (i = 1; i < ND; i++)
      p[i] = p[i - 1] * s;
    for (i = 0; i < ND; i++) {
      for (j = 0; j < ND; j++)
        a[i][j] += p[i] * p[j];
      a[i][ND] += p[i] * v[k + NS];
    }
  }
  for (i = 0; i < ND; i++)
    for (r = i + 1; r < ND; r++) {
      double q = a[r][i] / a[i][i];
      for (j = i; j <= ND; j++)
        a[r][j] -= q * a[i][j];
    }
  for (i = ND - 1; i >= 0; i--) {
    double s = a[i][ND];
    for (j = i + 1; j < ND; j++)
      s -= a[i][j] * c[j];
    c[i] = s / a[i][i];
  }
  return fabs(x[4] - c[1] / (NS * h));
}

/* UPSTREAM-BUGS 50: Mercury's Moshier geometric longitude at -3000 over
 * 64 consecutive JDs, its worst residual from a straight line, mas. The
 * arguments freq * T rounded at 2e-6" and more time than a JD resolves:
 * 1.6e-3 mas. */
static double mosh_rough(void)
{
  double jd = 2451545.0 - 5000 * 365.25 + 0.1234567, v[64], x[6], x0 = 0;
  double sx = 0, sy = 0, sxx = 0, sxy = 0, a, b, m = 0;
  char serr[256];
  int i;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (i = 0; i < 64; i++, jd = nextafter(jd, 1e10)) {
    swe_calc(jd, SE_MERCURY, SEFLG_MOSEPH | SEFLG_TRUEPOS, x, serr);
    if (i == 0)
      x0 = x[0];
    v[i] = remainder(x[0] - x0, 360.0) * 3.6e6;
    sx += i; sy += v[i]; sxx += (double) i * i; sxy += i * v[i];
  }
  b = (64 * sxy - sx * sy) / (64 * sxx - sx * sx);
  a = (sy - b * sx) / 64;
  for (i = 0; i < 64; i++)
    if (fabs(v[i] - a - b * i) > m)
      m = fabs(v[i] - a - b * i);
  return m;
}

/* UPSTREAM-BUGS 51: the Moshier Moon's and Mercury's J2000 geometric
 * longitude speeds at -2500 against a five-point difference, deg/day: a
 * difference over +-4.32 s at rounded instants (the Moon) and a backward
 * one over 8.64 s (Mercury), 1e-5 and more; now <= 1e-9 */
static double mosh_speed_off(void)
{
  double jd = 2451545.0 - 4500 * 365.25 + 0.137, a, b;
  swe_close();
  swe_set_ephe_path(EPHE);
  a = lon_speed_off(SE_MOON, SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS, jd);
  b = lon_speed_off(SE_MERCURY, SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS, jd);
  return fmax(a, b);
}

/* UPSTREAM-BUGS 52: the aberration's part of the topocentric Sun's
 * longitude speed on the equator in 1990 (the residual against a
 * five-point difference with aberration less that without), deg/day. Bug
 * 34's term was first order in v/c, a site's radial acceleration stripped:
 * 3.1e-8 here, a turning site against the Earth's orbital v; now 5.4e-10.
 * Upstream has no such term for the Sun at all: 3.0e-6. */
static double topo_aberr_second_order_off(void)
{
  double t = floor(2447892.5 * 256) / 256, h = 1.0 / 256, r[2];
  char serr[256];
  int c, k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 0, 400);
  for (c = 0; c < 2; c++) {
    int32 fl = SEFLG_SWIEPH | SEFLG_TOPOCTR | (c ? SEFLG_NOABERR : 0);
    double x[6], f[5][6], d[5];
    swe_calc(t, SE_SUN, fl | SEFLG_SPEED, x, serr);
    for (k = 0; k < 5; k++)
      swe_calc(t + (k - 2) * h, SE_SUN, fl, f[k], serr);
    for (k = 0; k < 5; k++)
      d[k] = swe_difdeg2n(f[k][0], f[2][0]);
    r[c] = x[3] - (d[0] - 8 * d[1] + 8 * d[3] - d[4]) / (12 * h);
  }
  return fabs(r[0] - r[1]);
}

/* UPSTREAM-BUGS 53: Mercury's heliocentric distance rate under Moshier
 * against a five-point difference of its distance over 2^-5 day, AU/day.
 * Moshier moved the body back along its velocity, x(t) - tau v(t), and
 * kept v(t) for the speed, without tau times the Sun's pull: ~5e-6; read
 * at t - tau now, ~1e-11. */
static double mosh_lt_off(void)
{
  double t = floor(2460000.3 * 256) / 256, h = 1.0 / 32, x[6], f[5][6];
  int32 fl = SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, SE_MERCURY, fl | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(t + (k - 2) * h, SE_MERCURY, fl, f[k], serr);
  return fabs(x[5] - (8 * (f[3][2] - f[1][2]) - (f[4][2] - f[0][2])) / (12 * h));
}

/* UPSTREAM-BUGS 84: the JPL Moon 0.125 day into DE200 after a JPL Pluto,
 * whose light time leaves the file and falls to Moshier, against the Moon
 * alone, deg. jplplan() took the series' heliocentric Earth, left in the
 * slot at that instant, for the barycentric one: 160 deg; keyed on the
 * ephemeris now, 0. */
static double jpl_stale_earth_off(void)
{
  double t = 2305424.5 + 0.125, x[6], y[6];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  if (swe_calc(t, SE_MOON, SEFLG_JPLEPH, x, serr) < 0)
    return -1;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  swe_calc(t, SE_PLUTO, SEFLG_JPLEPH, y, serr);
  if (swe_calc(t, SE_MOON, SEFLG_JPLEPH, y, serr) < 0)
    return -1;
  return fabs(swe_difdeg2n(x[0], y[0]));
}

/* UPSTREAM-BUGS 89: the barycentric Sun at JD 2378496.6, just past 1800.0
 * where sepl_12 hands over to sepl_18, after a call 1.5 days before the
 * join, against the Sun alone, mas. sepl_12 runs 79 days past its nominal
 * end and an open file answered any instant it held: 47.6 mas; every
 * instant from the file of its own 600 years now, 0. */
static double file_join_off(void)
{
  double t = 2378496.6, x[6], y[6];
  int32 fl = SEFLG_SWIEPH | SEFLG_BARYCTR | SEFLG_SPEED;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  if (swe_calc(t, SE_SUN, fl, x, serr) < 0)
    return -1;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t - 1.5, SE_SUN, fl, y, serr);
  if (swe_calc(t, SE_SUN, fl, y, serr) < 0)
    return -1;
  return fabs(swe_difdeg2n(x[0], y[0])) * 3.6e6;
}

/* UPSTREAM-BUGS 100: with semo_12.se1 missing (a directory of links to
 * the others), the Moon at JD 2159377.5, 1200.0 + 20 days, past the end of
 * semo_06's data: fresh against warm from 1200.0 + 10, where semo_06
 * answers, deg; 1e9 if either is not finite. The neighbour-file lookup
 * that went wrong (F11) is this fork's and absent under the switch, whose
 * build takes the Moshier Moon both times: 0 both ways, the shipped build
 * NaN fresh and 37 deg off warm before the fix. With the fallback allowed:
 * the strict setting refuses the Moshier Moon since entry 120. */
static double moon_neighbour_off(void)
{
  char dir[] = "/tmp/compatmoXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  double t = 2159377.5, x[6], y[6], r = -1;
  int32 fl = SEFLG_SWIEPH | SEFLG_SPEED;
  int fb;
  char serr[256];
  struct dirent *e;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "semo_12.se1") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  swe_close();
  swe_set_ephe_path(dir);
  fb = swe_get_ephe_fallback();
  swe_set_ephe_fallback(1);
  if (swe_calc(t, SE_MOON, fl, x, serr) >= 0) {
    swe_close();
    swe_set_ephe_path(dir);
    swe_calc(t - 10, SE_MOON, fl, y, serr);
    if (swe_calc(t, SE_MOON, fl, y, serr) >= 0)
      r = (isfinite(x[0]) && isfinite(y[0])) ? fabs(swe_difdeg2n(x[0], y[0])) : 1e9;
  }
  swe_close();
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  swe_set_ephe_fallback(fb);
  swe_set_ephe_path(EPHE);
  return r;
}

/* UPSTREAM-BUGS 120: with semo_12.se1 missing (a directory of links to
 * the others), the Swiss Moon at 1500 Jan 5 under the strict setting, its
 * return code: sweplan() took Moshier's Moon, noted in serr, and the call
 * returned SEFLG_SWIEPH | SEFLG_SPEED, 258, which swi_refuse_downgrade()
 * could not see; refused now, -1. */
static double moshmoon_strict_ret(void)
{
  char dir[] = "/tmp/compatmmXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  double t = 2268936.5, x[6], r;
  char serr[256];
  struct dirent *e;
  DIR *d;
  int fb;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return 1e9;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "semo_12.se1") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  swe_close();
  swe_set_ephe_path(dir);
  fb = swe_get_ephe_fallback();
  swe_set_ephe_fallback(0);
  r = swe_calc(t, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
  swe_set_ephe_fallback(fb);
  swe_close();
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return r;
}

/* UPSTREAM-BUGS 119: a second thread follows the main thread's path and
 * de200, computes Mercury on JPL at J2000, and the main thread then sets
 * de200x.eph, de200 with every EMB coefficient scaled by 1 + 1e-5, in a
 * directory of links; the second thread's Mercury again, against the main
 * thread's, deg (1e9 if the file cannot be built, a call fails or the copy
 * does not move Mercury by 1e-6). The shipped build kept the old file's
 * Earth: 5.78e-5. The adoption shortcut that went wrong (bug 103) is this
 * fork's, and under the switch the next calculation frees every body's
 * data: 0 both ways. */
static double adopt_t = 2451545.0;
static double adopt_x[6];
static int32 adopt_r;
static pthread_barrier_t adopt_b1, adopt_b2;

static void *adopt_worker(void *arg)
{
  char serr[256];
  (void) arg;
  swe_calc(adopt_t, SE_MERCURY, SEFLG_JPLEPH | SEFLG_SPEED, adopt_x, serr);
  pthread_barrier_wait(&adopt_b1);
  pthread_barrier_wait(&adopt_b2);
  adopt_r = swe_calc(adopt_t, SE_MERCURY, SEFLG_JPLEPH | SEFLG_SPEED, adopt_x, serr);
  return NULL;
}

static double adopt_jpl_off(void)
{
  char dir[] = "/tmp/compatajXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  double r = 1e9, x[6], x0[6];
  char serr[256];
  struct dirent *e;
  DIR *d;
  FILE *f;
  unsigned char *b = NULL;
  long n = 0, rec, k;
  int32 ipt[13][3], numde = 0, nc, j, m;
  pthread_t th;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return 1e9;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.') {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  snprintf(p, sizeof p, "%s/de200.eph", ephe);
  if ((f = fopen(p, "rb")) != NULL) {
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    rewind(f);
    if ((b = malloc((size_t) n)) != NULL && fread(b, 1, (size_t) n, f) != (size_t) n) {
      free(b);
      b = NULL;
    }
    fclose(f);
  }
  if (b != NULL) {	/* the header: ss, ncon, au, emrat, ipt, numde, lpt */
    memcpy(ipt, b + 2696, 36 * 4);
    memcpy(&numde, b + 2840, 4);
    memcpy(ipt[12], b + 2844, 12);
  }
  if (b != NULL && numde == 200) {
    for (m = 0, j = 1; j < 13; j++)
      if (ipt[j][0] > ipt[m][0])
        m = j;
    nc = ipt[m][0] - 1 + ipt[m][1] * ipt[m][2] * (m == 11 ? 2 : 3);
    rec = (long) nc * 8;
    for (k = 2; (k + 1) * rec <= n; k++)
      for (j = 0; j < ipt[2][2] * 3; j++) {
        double v;
        long off = k * rec + (long) (ipt[2][0] - 1 + j * ipt[2][1]) * 8;
        memcpy(&v, b + off, 8);
        v *= 1 + 1e-5;
        memcpy(b + off, &v, 8);
      }
    snprintf(q, sizeof q, "%s/de200x.eph", dir);
    if ((f = fopen(q, "wb")) != NULL) {
      if (fwrite(b, 1, (size_t) n, f) == (size_t) n && fclose(f) == 0) {
        swe_close();
        swe_set_ephe_path(dir);
        swe_set_jpl_file("de200.eph");
        swe_calc(adopt_t, SE_MERCURY, SEFLG_JPLEPH | SEFLG_SPEED, x0, serr);
        pthread_barrier_init(&adopt_b1, NULL, 2);
        pthread_barrier_init(&adopt_b2, NULL, 2);
        if (pthread_create(&th, NULL, adopt_worker, NULL) == 0) {
          pthread_barrier_wait(&adopt_b1);
          swe_set_jpl_file("de200x.eph");
          j = swe_calc(adopt_t, SE_MERCURY, SEFLG_JPLEPH | SEFLG_SPEED, x, serr);
          pthread_barrier_wait(&adopt_b2);
          pthread_join(th, NULL);
          if (j >= 0 && adopt_r >= 0 && fabs(swe_difdeg2n(x[0], x0[0])) > 1e-6)
            r = fabs(swe_difdeg2n(adopt_x[0], x[0]));
        }
        pthread_barrier_destroy(&adopt_b1);
        pthread_barrier_destroy(&adopt_b2);
      } else
        fclose(f);
    }
  }
  free(b);
  swe_close();
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return r;
}

/* UPSTREAM-BUGS 104: seas_06.se1 a link to seas_00.se1, which read_const()
 * refuses for its name; the Sun 25 days after a Ceres there that met it,
 * against the Sun alone, deg (1e9 if the warm call fails). Upstream's
 * refusal ran free_planets(), emptying the open sepl and semo files'
 * constants: "sepl_06.se1 is damaged (2)"; only the refused file is
 * cleared now, 0. */
static double refused_file_off(void)
{
  char dir[] = "/tmp/compatrfXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  double t = swe_julday(600, 1, 1, 0, SE_JUL_CAL) + 400, x[6], y[6], r = -1;
  int32 fl = SEFLG_SWIEPH | SEFLG_SPEED;
  char serr[256];
  struct dirent *e;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "seas_06.se1") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  snprintf(p, sizeof p, "%s/seas_00.se1", ephe);
  snprintf(q, sizeof q, "%s/seas_06.se1", dir);
  if (symlink(p, q) == 0) {
    swe_close();
    swe_set_ephe_path(dir);
    if (swe_calc(t, SE_SUN, fl, x, serr) >= 0) {
      swe_close();
      swe_set_ephe_path(dir);
      swe_calc(t - 25, SE_CERES, fl, y, serr);
      r = swe_calc(t, SE_SUN, fl, y, serr) >= 0 ? fabs(swe_difdeg2n(x[0], y[0])) : 1e9;
    }
  }
  swe_close();
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return r;
}

/* UPSTREAM-BUGS 113: a malformed seorbel.txt or sefstars.txt line. Each
 * bit is one of upstream's behaviours, shown: (1) Cupido's line cut to
 * five fields refuses Hades too; (2) Cupido's epoch "2451545.0abc" is
 * read as 2451545 and answers; (3) its mean anomaly empty is 0 and
 * answers; (4) with no sefstars.txt, swe_fixstar2() says only that the
 * star was not found; (5) swe_fixstar() with serr NULL answers an unknown
 * star with the catalogue's last; (6) UPSTREAM-BUGS 114: Cupido's mean
 * anomaly "163.7409 + .0 * T" read as "+ T", not as "+ 0.0 * T". Upstream
 * all six (63), shipped none. */
static int fict_star_line(const char *dir, const char *ephe, const char *cupido)
{
  char src[PATH_MAX + 40], dst[PATH_MAX + 40], s[1024];
  FILE *f, *g;
  int done = 0;
  snprintf(src, sizeof src, "%s/seorbel.txt", ephe);
  snprintf(dst, sizeof dst, "%s/seorbel.txt", dir);
  unlink(dst);
  if ((f = fopen(src, "r")) == NULL || (g = fopen(dst, "w")) == NULL)
    return -1;
  while (fgets(s, sizeof s, f) != NULL) {
    char *sp = s;
    while (*sp == ' ' || *sp == '\t')
      sp++;
    if (!done && *sp != '#' && *sp != '\r' && *sp != '\n' && *sp != '\0') {
      fputs(cupido, g);
      done = 1;
    } else
      fputs(s, g);
  }
  fclose(f);
  return fclose(g);
}

/* swe_fixstar2() after sefstars.txt failed to load (broken at line 500)
 * and was repaired in place, with neither a new path nor swe_close(): 1 if
 * it answered from the repaired file, 0 if refused as before. Upstream
 * reread the file on every call (UPSTREAM-BUGS 121); now the failure is
 * kept, and the file not touched, until the path changes or swe_close(). */
static double star_load_kept(void)
{
  char dir[] = "/tmp/compatslXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  char serr[256], star[64], s[1024];
  double x[6], r = -1;
  int n, k;
  struct dirent *e;
  DIR *d;
  FILE *f, *g;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "sefstars.txt") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  snprintf(p, sizeof p, "%s/sefstars.txt", ephe);
  snprintf(q, sizeof q, "%s/sefstars.txt", dir);
  for (k = 0; k < 2; k++) {	/* broken, then repaired */
    if ((f = fopen(p, "r")) == NULL || (g = fopen(q, "w")) == NULL)
      break;
    for (n = 1; fgets(s, sizeof s, f) != NULL; n++)
      fputs(n == 500 && k == 0 ? "a line without a comma\n" : s, g);
    fclose(f);
    fclose(g);
    if (k == 0) {
      swe_close();
      swe_set_ephe_path(dir);
      strcpy(star, "Aldebaran");
      if (swe_fixstar2(star, 2451545.0, 0, x, serr) >= 0)
        break;
    } else {
      strcpy(star, "Aldebaran");
      r = swe_fixstar2(star, 2451545.0, 0, x, serr) >= 0;
    }
  }
  swe_close();
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  return r;
}

static double fict_star_parse(void)
{
  char dir[] = "/tmp/compatfsXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  char serr[256], star[64];
  double x[6];
  int mask = 0;
  struct dirent *e;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "seorbel.txt") != 0
        && strcmp(e->d_name, "sefstars.txt") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  if (fict_star_line(dir, ephe, "J1900, J1900, 163.7409, 40.99837, 0.00460\n") == 0) {
    swe_close();
    swe_set_ephe_path(dir);
    if (swe_calc(2451545.0, SE_FICT_OFFSET + 1, 0, x, serr) < 0)
      mask |= 1;
  }
  if (fict_star_line(dir, ephe, "2451545.0abc, J1900, 163.7409, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, Cupido\n") == 0) {
    swe_close();
    swe_set_ephe_path(dir);
    if (swe_calc(2451545.0, SE_FICT_OFFSET, 0, x, serr) >= 0)
      mask |= 2;
  }
  if (fict_star_line(dir, ephe, "J1900, J1900, , 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, Cupido\n") == 0) {
    swe_close();
    swe_set_ephe_path(dir);
    if (swe_calc(2451545.0, SE_FICT_OFFSET, 0, x, serr) >= 0)
      mask |= 4;
  }
  {
    double xa[6] = {0}, xb[6] = {1};
    if (fict_star_line(dir, ephe, "J1900, J1900, 163.7409 + .0 * T, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, Cupido\n") == 0) {
      swe_close();
      swe_set_ephe_path(dir);
      swe_calc(2451545.0, SE_FICT_OFFSET, 0, xa, serr);
    }
    if (fict_star_line(dir, ephe, "J1900, J1900, 163.7409 + 0.0 * T, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, Cupido\n") == 0) {
      swe_close();
      swe_set_ephe_path(dir);
      swe_calc(2451545.0, SE_FICT_OFFSET, 0, xb, serr);
    }
    if (xa[0] != xb[0])
      mask |= 32;
  }
  swe_close();
  swe_set_ephe_path(dir);
  snprintf(star, sizeof star, "Aldebaran");
  if (swe_fixstar2(star, 2451545.0, 0, x, serr) < 0 && strstr(serr, "sefstars.txt") == NULL)
    mask |= 8;
  swe_close();
  swe_set_ephe_path(EPHE);
  snprintf(star, sizeof star, "Nonexistentstar");
  if (swe_fixstar(star, 2451545.0, 0, x, NULL) >= 0)
    mask |= 16;
  swe_close();
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return mask;
}

/* UPSTREAM-BUGS 118, this fork's code (bugs 101, 103, 107, 110), so both
 * builds must be clean: (1) the cwd a directory of links to the files, 100
 * x (swe_ctx_new, swe_set_ephe_path_r(EPHE), swe_ctx_free) left an
 * ephemeris descriptor each, the default path's moon file a context's
 * initialisation opened and the setter overwrote; (2) a moon file linked
 * into a path that had none, the same path set again: delta t in AD 1000
 * the default term's until a Moon (1463.44 s, then 1474.72). The count of
 * the two shown, 0 in both builds. */
static int path_fds(void)
{
  DIR *d = opendir("/proc/self/fd");
  struct dirent *e;
  char p[300], l[PATH_MAX];
  int n = 0;
  if (d == NULL)
    return 0;
  while ((e = readdir(d)) != NULL) {
    ssize_t k;
    if (e->d_name[0] == '.')
      continue;
    snprintf(p, sizeof p, "/proc/self/fd/%s", e->d_name);
    if ((k = readlink(p, l, sizeof l - 1)) > 0) {
      l[k] = '\0';
      if (strstr(l, ".se1") != NULL)
        n++;
    }
  }
  closedir(d);
  return n;
}

static double path_setter_118(void)
{
  char dir[] = "/tmp/compatpsXXXXXX", ephe[PATH_MAX], cwd[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  char serr[256];
  double x[6], t = 2086302.5, a, b;
  int i, n0, bad = 0;
  struct dirent *e;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || getcwd(cwd, sizeof cwd) == NULL || mkdtemp(dir) == NULL
      || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "semo_18.se1") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  /* (1), with the moon file in cwd too */
  snprintf(p, sizeof p, "%s/semo_18.se1", ephe);
  snprintf(q, sizeof q, "%s/semo_18.se1", dir);
  swe_close();	/* no master configuration: the contexts start unnamed */
  if (symlink(p, q) == 0 && chdir(dir) == 0) {
    n0 = path_fds();
    for (i = 0; i < 100; i++) {
      swe_ctx *c = swe_ctx_new();
      swe_set_ephe_path_r(c, ephe);
      swe_ctx_free(c);
    }
    if (path_fds() > n0)
      bad++;
    if (chdir(cwd) != 0)
      bad += 10;
  }
  /* (2) */
  unlink(q);
  swe_close();
  swe_set_ephe_path(dir);
  swe_deltat(t);
  if (symlink(p, q) == 0) {
    swe_set_ephe_path(dir);
    a = swe_deltat(t);
    swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
    b = swe_deltat(t);
    if (a != b)
      bad++;
  }
  swe_close();
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return bad;
}

/* A topocentric Moshier fictitious body on a worker thread's default
 * context, swe_set_astro_models() called on the main thread while that
 * call waits for its seorbel.txt (a FIFO the main thread fills only after
 * the setter), against a fresh context with the starting models: the
 * largest of the six values' differences. Upstream's call adopts the new
 * models at its observer's swe_deltat_ex(), half-way through, and its
 * longitude speed comes out 0.413 deg/day off (UPSTREAM-BUGS 115). */
static const char *hold_line =
  "J1900, J1900, 163.7409, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, ZZTESTBODY\n";
static double hold_got[6];
static void *hold_thread(void *arg)
{
  char serr[256];
  double x[6];
  (void) arg;
  swe_calc(2451545.25, SE_SUN, SEFLG_MOSEPH, x, serr);
  swe_calc(2451545.25, SE_FICT_OFFSET, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, hold_got, serr);
  return NULL;
}
static double cfg_hold_off(void)
{
  char dir[] = "/tmp/compathoXXXXXX", fifo[PATH_MAX], reg[PATH_MAX], tmp[PATH_MAX + 8];
  char serr[256];
  double want[6], r = 0;
  swe_ctx *c;
  pthread_t th;
  FILE *fp;
  int fd, k;
  if (mkdtemp(dir) == NULL)
    return -1;
  snprintf(reg, sizeof reg, "%s/seorbel.txt", dir);
  snprintf(fifo, sizeof fifo, "%s/fifo", dir);
  if (mkdir(fifo, 0700) != 0 || (fp = fopen(reg, "w")) == NULL)
    return -1;
  fputs(hold_line, fp);
  fclose(fp);
  swe_close();
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, dir);
  swe_set_topo_r(c, 8.55, 47.37, 400);
  swe_calc_r(c, 2451545.25, SE_FICT_OFFSET, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, want, serr);
  swe_ctx_free(c);
  swe_set_ephe_path(fifo);
  swe_set_topo(8.55, 47.37, 400);
  snprintf(fifo, sizeof fifo, "%s/fifo/seorbel.txt", dir);
  snprintf(tmp, sizeof tmp, "%s.new", fifo);
  if (mkfifo(fifo, 0600) != 0 || pthread_create(&th, NULL, hold_thread, NULL) != 0)
    return -1;
  if ((fd = open(fifo, O_WRONLY)) < 0)	/* the worker's call has opened it */
    return -1;
  swe_set_astro_models("1,3,1,1,1,0,0,1", 0);
  /* a regular file in its place, for a call that opens it again */
  if ((fp = fopen(tmp, "w")) == NULL)
    return -1;
  fputs(hold_line, fp);
  fclose(fp);
  rename(tmp, fifo);
  if (write(fd, hold_line, strlen(hold_line)) < 0)
    return -1;
  close(fd);
  pthread_join(th, NULL);
  for (k = 0; k < 6; k++)
    if (fabs(hold_got[k] - want[k]) > r)
      r = fabs(hold_got[k] - want[k]);
  swe_set_astro_models("", 0);
  swe_close();
  unlink(fifo);
  unlink(reg);
  snprintf(fifo, sizeof fifo, "%s/fifo", dir);
  rmdir(fifo);
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return r;
}

/* A sidereal Moshier swe_mooncross_ut() on a worker thread's default
 * context, swe_set_sid_mode() from Lahiri to Fagan-Bradley called on the
 * main thread while the call waits for its swe_deltat.txt (a FIFO the main
 * thread fills only after the setter), against a fresh context under
 * Lahiri: the crossing's difference, s. Upstream's search adopts the new
 * mode at its second step and answers Fagan-Bradley's crossing, 5600 s
 * off (UPSTREAM-BUGS 124). */
static double search_got;
static void *search_thread(void *arg)
{
  char serr[256];
  double x[6];
  (void) arg;
  swe_calc(2451545.25, SE_SUN, SEFLG_MOSEPH, x, serr);
  search_got = swe_mooncross_ut(0.0, 2451545.25, SEFLG_MOSEPH | SEFLG_SIDEREAL, serr);
  return NULL;
}
static double search_hold_off(void)
{
  static const char *line = "# fixture: the built-in table throughout\n";
  char dir[] = "/tmp/compatshXXXXXX", fifo[PATH_MAX], reg[PATH_MAX], tmp[PATH_MAX + 8];
  char serr[256];
  double want;
  swe_ctx *c;
  pthread_t th;
  FILE *fp;
  int fd;
  if (mkdtemp(dir) == NULL)
    return -1;
  snprintf(reg, sizeof reg, "%s/swe_deltat.txt", dir);
  snprintf(fifo, sizeof fifo, "%s/fifo", dir);
  if (mkdir(fifo, 0700) != 0 || (fp = fopen(reg, "w")) == NULL)
    return -1;
  fputs(line, fp);
  fclose(fp);
  swe_close();
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, dir);
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  want = swe_mooncross_ut_r(c, 0.0, 2451545.25, SEFLG_MOSEPH | SEFLG_SIDEREAL, serr);
  swe_ctx_free(c);
  swe_set_ephe_path(fifo);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  snprintf(fifo, sizeof fifo, "%s/fifo/swe_deltat.txt", dir);
  snprintf(tmp, sizeof tmp, "%s.new", fifo);
  if (mkfifo(fifo, 0600) != 0 || pthread_create(&th, NULL, search_thread, NULL) != 0)
    return -1;
  if ((fd = open(fifo, O_WRONLY)) < 0)	/* the worker's search has opened it */
    return -1;
  swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  /* a regular file in its place, for a call that opens it again */
  if ((fp = fopen(tmp, "w")) == NULL)
    return -1;
  fputs(line, fp);
  fclose(fp);
  rename(tmp, fifo);
  if (write(fd, line, strlen(line)) < 0)
    return -1;
  close(fd);
  pthread_join(th, NULL);
  swe_close();
  unlink(fifo);
  unlink(reg);
  snprintf(fifo, sizeof fifo, "%s/fifo", dir);
  rmdir(fifo);
  rmdir(dir);
  swe_set_ephe_path(EPHE);
  return fabs(search_got - want) * 86400;
}

/* The nutation after swe_set_interpolate_nut() changes: on, SE_ECL_NUT at
 * J2000 and at J2000 + 0.37 (interpolated from J2000's nodes), off, and
 * J2000 + 0.37 again, against the exact value there after another
 * instant; dpsi, arcsec. Upstream's setter resets the nodes alone, and the
 * nutation cached at the instant answers interpolated, 8.38e-5" off
 * (UPSTREAM-BUGS 126). */
static double nut_intp_off(void)
{
  double x[6], y[6];
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_interpolate_nut(TRUE);
  swe_calc(2451545.0, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  swe_calc(2451545.37, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  swe_set_interpolate_nut(FALSE);	/* swe_close() keeps it: back to the default */
  swe_calc(2451545.37, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  swe_calc(2451545.0, SE_ECL_NUT, SEFLG_SWIEPH, y, serr);
  swe_calc(2451545.37, SE_ECL_NUT, SEFLG_SWIEPH, y, serr);
  return fabs(x[2] - y[2]) * 3600;
}

/* A new thread's first swe_deltat_ex() after the main thread's
 * swe_set_delta_t_userdef(0.001), against the 0.001 days set: s.
 * Upstream's checks the user delta t before anything has started the
 * thread's context, and answers the computed delta t, 22.6 s off
 * (UPSTREAM-BUGS 128). */
static double hstart_got;
static void *hstart_thread(void *arg)
{
  char serr[256];
  (void) arg;
  hstart_got = swe_deltat_ex(2451545.25, SEFLG_SWIEPH, serr);
  return NULL;
}
static double hold_start_off(void)
{
  pthread_t th;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_delta_t_userdef(0.001);
  if (pthread_create(&th, NULL, hstart_thread, NULL) != 0)
    return -1;
  pthread_join(th, NULL);
  swe_set_delta_t_userdef(SE_DELTAT_AUTOMATIC);	/* swe_close() keeps it */
  swe_close();
  swe_set_ephe_path(EPHE);
  return fabs(hstart_got - 0.001) * 86400;
}

/* The nutation after a change of nutation model, the interpolation on:
 * the nutation at J2000, IAU 1980 set, dpsi at J2000 + 0.37; then the
 * nutation at J2000 + 10 and swe_close(), which puts the default model
 * back and keeps the interpolation, and dpsi at J2000 + 10.37. Each
 * against a fresh context's with the final model, arcsec, the larger.
 * Upstream keeps the interpolation's nodes of the old model, 8.6e-3" off
 * (UPSTREAM-BUGS 129). */
static double nut_model_intp_off(void)
{
  double x[6], y[6], d;
  char serr[256], m1980[] = "0,0,0,1", mdef[] = "0,0,0,0";
  swe_ctx *c;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_interpolate_nut(TRUE);
  swe_calc(2451545.0, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  swe_set_astro_models(m1980, 0);
  swe_calc(2451545.37, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  if ((c = swe_ctx_new()) == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_interpolate_nut_r(c, TRUE);
  swe_set_astro_models_r(c, m1980, 0);
  swe_calc_r(c, 2451545.37, SE_ECL_NUT, SEFLG_SWIEPH, y, serr);
  d = fabs(x[2] - y[2]);
  swe_calc(2451555.0, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  swe_close();
  swe_calc(2451555.37, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  swe_ctx_free(c);
  if ((c = swe_ctx_new()) == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_interpolate_nut_r(c, TRUE);
  swe_set_astro_models_r(c, mdef, 0);
  swe_calc_r(c, 2451555.37, SE_ECL_NUT, SEFLG_SWIEPH, y, serr);
  swe_ctx_free(c);
  if (fabs(x[2] - y[2]) > d)
    d = fabs(x[2] - y[2]);
  swe_set_interpolate_nut(FALSE);	/* swe_close() keeps it: back to the default */
  return d * 3600;
}

/* UPSTREAM-BUGS 134: and across the JPL Horizons flags and the
 * approximation's mode. On de200, the interpolation on: dpsi at 1950 +
 * 0.37 without SEFLG_JPLHOR_APPROX after it at 1950; and with it at 1950
 * + 10.37 after it at 1950 + 10 and SEMOD_JPLHORA_2. Against a fresh
 * context making the same calls under the final flags and mode (the
 * instant and then + 0.37), the larger, arcsec. */
static double nut_jplhor_intp_off(void)
{
  double x[6], y[6], d, t = 2433282.5;
  char serr[256], m2[] = "0,0,0,0,0,0,2", mdef[] = "0,0,0,0,0,0,0";
  const int32 je = SEFLG_JPLEPH, ja = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX;
  swe_ctx *c;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  swe_set_interpolate_nut(TRUE);
  swe_calc(t, SE_ECL_NUT, ja, x, serr);
  swe_calc(t, SE_ECL_NUT, je, x, serr);
  swe_calc(t + 0.37, SE_ECL_NUT, je, x, serr);
  if ((c = swe_ctx_new()) == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_interpolate_nut_r(c, TRUE);
  swe_calc_r(c, t, SE_ECL_NUT, je, y, serr);
  swe_calc_r(c, t + 0.37, SE_ECL_NUT, je, y, serr);
  d = fabs(x[2] - y[2]);
  swe_calc(t + 10, SE_ECL_NUT, ja, x, serr);
  swe_set_astro_models(m2, 0);
  swe_calc(t + 10, SE_ECL_NUT, ja, x, serr);
  swe_calc(t + 10.37, SE_ECL_NUT, ja, x, serr);
  swe_set_astro_models_r(c, m2, 0);
  swe_calc_r(c, t + 10, SE_ECL_NUT, ja, y, serr);
  swe_calc_r(c, t + 10.37, SE_ECL_NUT, ja, y, serr);
  swe_ctx_free(c);
  if (fabs(x[2] - y[2]) > d)
    d = fabs(x[2] - y[2]);
  swe_set_astro_models(mdef, 0);
  swe_set_interpolate_nut(FALSE);
  swe_close();
  return d * 3600;
}

/* UPSTREAM-BUGS 146: the interpolation's nodes across SEMOD_JPLHORA_3's
 * switch at 1962 Jan 20 (7.17e-2" in dpsi). On de200 with
 * SEFLG_JPLHOR_APPROX, the interpolation on: dpsi at the switch + 0.3
 * after a call at - 0.5, and at - 0.3 after one at + 0.5, against a
 * context with it off, the larger, arcsec. */
static double nut_step_intp_off(void)
{
  double x[6], y[6], d = 0, ts = 2437684.5;
  const double tc[2] = { -0.5, 0.5 }, tq[2] = { 0.3, -0.3 };
  char serr[256];
  const int32 ja = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX;
  swe_ctx *ci, *ce;
  int k;
  for (k = 0; k < 2; k++) {
    if ((ci = swe_ctx_new()) == NULL || (ce = swe_ctx_new()) == NULL)
      return -1;
    swe_set_ephe_path_r(ci, EPHE);
    swe_set_jpl_file_r(ci, "de200.eph");
    swe_set_interpolate_nut_r(ci, TRUE);
    swe_set_ephe_path_r(ce, EPHE);
    swe_set_jpl_file_r(ce, "de200.eph");
    swe_calc_r(ci, ts + tc[k], SE_ECL_NUT, ja, x, serr);
    swe_calc_r(ci, ts + tq[k], SE_ECL_NUT, ja, x, serr);
    swe_calc_r(ce, ts + tq[k], SE_ECL_NUT, ja, y, serr);
    if (fabs(x[2] - y[2]) > d)
      d = fabs(x[2] - y[2]);
    swe_ctx_free(ci);
    swe_ctx_free(ce);
  }
  return d * 3600;
}

/* UPSTREAM-BUGS 144: the IERS corrections across swe_set_ephe_path(). On
 * G19's synthetic DE431 header in two directories of other corrections
 * (G134's A and B, made in $TMPDIR): path A, the fixture, SE_ECL_NUT with
 * SEFLG_JPLHOR at 2000 Jan 7, then path B and the fixture, and dpsi at Jan
 * 7.37 against a fresh context given B and the fixture, arcsec; -1 if the
 * fixtures cannot be written. Upstream loads them once: A's for B's,
 * 0.131". */
static void eop_put32(unsigned char *p, int v)    { memcpy(p, &v, 4); }
static void eop_putd (unsigned char *p, double v) { memcpy(p, &v, 8); }
static int eop_write_jpl(const char *path)
{
  unsigned char h[2856];
  const int ncf = 4, na = 1, maxoff = 1200, nrecl = 4;
  const double ss0 = 2451544.5, ss1 = 2451576.5, ss2 = 32.0;
  int i, ksize;
  long nb = 0;
  FILE *fp;
  memset(h, ' ', 2652);
  memcpy(h, "SWISSEPH EOP LOADER FIXTURE", 27);
  eop_putd(h + 2652, ss0);
  eop_putd(h + 2660, ss1);
  eop_putd(h + 2668, ss2);
  eop_put32(h + 2676, 0);
  eop_putd(h + 2680, 149597870.7);
  eop_putd(h + 2688, 81.3);
  for (i = 0; i < 12; i++) {
    eop_put32(h + 2696 + (i * 3 + 0) * 4, 3);
    eop_put32(h + 2696 + (i * 3 + 1) * 4, ncf);
    eop_put32(h + 2696 + (i * 3 + 2) * 4, na);
  }
  eop_put32(h + 2696 + 30 * 4, maxoff);
  eop_put32(h + 2696 + 31 * 4, ncf);
  eop_put32(h + 2696 + 32 * 4, na);
  eop_put32(h + 2840, 431);
  eop_put32(h + 2844, 3);
  eop_put32(h + 2848, ncf);
  eop_put32(h + 2852, na);
  ksize = (maxoff + 3 * ncf * na - 1) * 2;
  for (i = 0; i < 13; i++)
    nb += (long) ncf * na * (i == 11 ? 2 : 3);
  nb = (nb + 2) * 8 + 2 * (long) ksize * nrecl;
  if ((fp = fopen(path, "wb")) == NULL)
    return -1;
  fwrite(h, 1, sizeof h, fp);
  for (i = (int) sizeof h; i < nb; i++)
    fputc(0, fp);
  fseek(fp, 2L * ksize * nrecl, SEEK_SET);
  { double ep[2]; ep[0] = ss0; ep[1] = ss1; fwrite(ep, 8, 2, fp); }
  return fclose(fp);
}
static int eop_write_eop(const char *path, double a, double b, double c, double d)
{
  FILE *fp = fopen(path, "w");
  int i;
  if (fp == NULL)
    return -1;
  for (i = 0; i < 40; i++)
    fprintf(fp, "2000  1 %2d  %5d   0.0 0.0 0.0 0.0 %.9f %.9f\n",
            i + 1, 51544 + i, a + i * b, c + i * d);
  return fclose(fp);
}
static double eop_path_kept_off(void)
{
  char base[400], da[420], db[420], p[480], jf[] = "de431fixture.eph";
  const char *tmp = getenv("TMPDIR");
  double x[6], y[6], t = 2451550.5, r = -1;
  char serr[256];
  const int32 jh = SEFLG_JPLEPH | SEFLG_JPLHOR;
  swe_ctx *c;
  snprintf(base, sizeof base, "%s/compat144.XXXXXX", tmp && *tmp ? tmp : "/tmp");
  if (mkdtemp(base) == NULL)
    return -1;
  snprintf(da, sizeof da, "%s/A", base);
  snprintf(db, sizeof db, "%s/B", base);
  if (mkdir(da, 0700) != 0 || mkdir(db, 0700) != 0)
    goto out;
  snprintf(p, sizeof p, "%s/%s", da, jf);
  if (eop_write_jpl(p) != 0)
    goto out;
  snprintf(p, sizeof p, "%s/%s", db, jf);
  if (eop_write_jpl(p) != 0)
    goto out;
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", da);
  if (eop_write_eop(p, -0.05, -1e-5, -0.005, -1e-6) != 0)
    goto out;
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", db);
  if (eop_write_eop(p, 0.08, 1e-4, 0.01, 1e-5) != 0)
    goto out;
  swe_close();
  swe_set_ephe_path(da);
  swe_set_jpl_file(jf);
  swe_calc(t, SE_ECL_NUT, jh, x, serr);
  swe_set_ephe_path(db);
  swe_set_jpl_file(jf);
  swe_calc(t + 0.37, SE_ECL_NUT, jh, x, serr);
  if ((c = swe_ctx_new()) == NULL)
    goto out;
  swe_set_ephe_path_r(c, db);
  swe_set_jpl_file_r(c, jf);
  swe_calc_r(c, t + 0.37, SE_ECL_NUT, jh, y, serr);
  swe_ctx_free(c);
  r = fabs(x[2] - y[2]) * 3600;
out:
  swe_close();
  swe_set_ephe_path(EPHE);
  snprintf(p, sizeof p, "%s/%s", da, jf); unlink(p);
  snprintf(p, sizeof p, "%s/%s", db, jf); unlink(p);
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", da); unlink(p);
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", db); unlink(p);
  rmdir(da);
  rmdir(db);
  rmdir(base);
  return r;
}

/* UPSTREAM-BUGS 157: SE_SIDM_LAHIRI_ICRC's mean ayanamsa at its epoch, JD
 * 2435553.5, with SE_SIDBIT_PREC_ORIG, less upstream's 23.25 - 0.00464207,
 * arcsec. Upstream subtracted 16.7115", the dpsi of a Woolard series whose
 * Moon's anomaly ran 10^4 times too fast (bug 151); Woolard's is 16.792088"
 * there, so the true ayanamsa is the committee's 23 deg 15' again: -0.0806
 * shipped, 0 compat. */
static double lahiri_icrc_t0_off(void)
{
  double a = 0;
  char serr[256];
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return 1e9;
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI_ICRC | SE_SIDBIT_PREC_ORIG, 0, 0);
  swe_get_ayanamsa_ex_r(c, 2435553.5, SEFLG_MOSEPH | SEFLG_NONUT, &a, serr);
  swe_ctx_free(c);
  return (a - (23.25 - 0.00464207)) * 3600;
}

/* UPSTREAM-BUGS 151: Woolard's nutation turned the Moon's mean anomaly at
 * 10^4 times its rate, a 3.97-minute cycle in its terms (0".12 in dpsi).
 * The second difference of Woolard's dpsi over 2^-11 day at eight binary
 * instants 1900-2100, the largest, arcsec: the series' own curvature gives
 * at most 1.6e-8". */
static double woolard_md_cycle(void)
{
  double x[6], f[3], h = 1.0 / 2048, d = 0;
  char serr[256];
  swe_ctx *c = swe_ctx_new();
  int i, k;
  if (c == NULL)
    return -1;
  swe_set_astro_models_r(c, "0,0,0,5", 0);	/* SEMOD_NUT_WOOLARD */
  for (i = 0; i < 8; i++) {
    double t = floor((2415020.3 + i * 9131.7) * 1024) / 1024;
    for (k = 0; k < 3; k++) {
      swe_calc_r(c, t + (k - 1) * h, SE_ECL_NUT, SEFLG_MOSEPH, x, serr);
      f[k] = x[2] * 3600;
    }
    if (fabs(f[0] - 2 * f[1] + f[2]) > d)
      d = fabs(f[0] - 2 * f[1] + f[2]);
  }
  swe_ctx_free(c);
  return d;
}

/* UPSTREAM-BUGS 153: the speeds across SEMOD_JPLHORA_3's step at 1962 Jan
 * 20, where the precession, obliquity and nutation change model. On de200
 * with SEFLG_JPLHOR_APPROX, Mars's ecliptic longitude speed at the step
 * - 0.5 day and -+ 2^-14 day against a five-point difference of its
 * longitudes over 2^-6 day on the instant's side (centred, else one-sided),
 * the largest, deg/day. The shipped build differenced the precession and
 * obliquity across the step within a day before it and the nutation within
 * 1e-4 day; upstream differences the nutation backward over 1e-4 day, across
 * the step just after it. */
static double jplhora_step_speed_off(void)
{
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  const double ts = 2437684.5, h = 1.0 / 64, dt[3] = { -0.5, -1.0 / 16384, 1.0 / 16384 };
  const int32 ja = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX;
  double x[6], f[5][6], r = 0;
  char serr[256];
  int i, k;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  for (i = 0; i < 3; i++) {
    double t = ts + dt[i], d = 0;
    int dir = (t <= ts) ? -1 : 1, centred = (i == 0);
    if (swe_calc_r(c, t, SE_MARS, ja | SEFLG_SPEED, x, serr) < 0) {
      swe_ctx_free(c);
      return -1;
    }
    for (k = 0; k < 5; k++)
      if (swe_calc_r(c, centred ? t + (k - 2) * h : t + dir * k * h, SE_MARS, ja, f[k], serr) < 0) {
        swe_ctx_free(c);
        return -1;
      }
    if (centred)
      d = (8 * (f[3][0] - f[1][0]) - (f[4][0] - f[0][0])) / (12 * h);
    else {
      for (k = 0; k < 5; k++)
        d += o5[k] * (f[k][0] - f[0][0]);
      d *= dir / h;
    }
    if (fabs(x[3] - d) > r)
      r = fabs(x[3] - d);
  }
  swe_ctx_free(c);
  return r;
}

/* UPSTREAM-BUGS 159: the speeds across SEFLG_JPLHOR's step of the
 * precession and obliquity at 1799 Jan 1 (IAU 1976 after it, Owen 1990 up
 * to and including it), which SEMOD_JPLHORA_3 takes before 1962. On de200
 * with SEFLG_JPLHOR_APPROX, the Sun's ecliptic speeds at the step - 0.5 day
 * and -+ 2^-14 day against five-point differences of its places over 2^-6
 * day on the instant's side (centred, else one-sided), the largest,
 * deg/day. The shipped build differenced the obliquity across the step
 * within a day before it (5.0e-6) and the precession within a day either
 * side; upstream's speeds of date are off there by the defects earlier
 * entries fixed (the ecliptic's turning, entry 33). */
static double jplhor_1799_speed_off(void)
{
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  const double ts = 2378131.5, h = 1.0 / 64, dt[3] = { -0.5, -1.0 / 16384, 1.0 / 16384 };
  const int32 ja = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX;
  double x[6], f[5][6], r = 0;
  char serr[256];
  int i, j, k;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  for (i = 0; i < 3; i++) {
    double t = ts + dt[i];
    int dir = (t <= ts) ? -1 : 1, centred = (i == 0);
    if (swe_calc_r(c, t, SE_SUN, ja | SEFLG_SPEED, x, serr) < 0) {
      swe_ctx_free(c);
      return -1;
    }
    for (k = 0; k < 5; k++)
      if (swe_calc_r(c, centred ? t + (k - 2) * h : t + dir * k * h, SE_SUN, ja, f[k], serr) < 0) {
        swe_ctx_free(c);
        return -1;
      }
    for (j = 0; j < 2; j++) {
      double d = 0;
      if (centred)
        d = (8 * (f[3][j] - f[1][j]) - (f[4][j] - f[0][j])) / (12 * h);
      else {
        for (k = 0; k < 5; k++)
          d += o5[k] * (f[k][j] - f[0][j]);
        d *= dir / h;
      }
      if (fabs(x[3 + j] - d) > r)
        r = fabs(x[3 + j] - d);
    }
  }
  swe_ctx_free(c);
  return r;
}

/* UPSTREAM-BUGS 160: the speeds across a short-term precession window's
 * edge. On de200 with IAU 1976 short-term and Vondrak long-term ("0,9,1"),
 * Mars's ecliptic speeds at J2000 - 2 centuries -+ 0.5 day and -+ 2^-14 day
 * (the edge's instant is the window's) against five-point differences of
 * its places over 2^-6 day on the instant's side (centred, else
 * one-sided), the largest, deg/day. The shipped build differenced the
 * precession across the edge within a day inside it (8.0e-5) and the
 * obliquity within a day outside; upstream's speeds of date are off there
 * by the defects earlier entries fixed. */
static double prec_window_speed_off(void)
{
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  const double ts = 2378495.0, h = 1.0 / 64;
  const double dt[4] = { -0.5, -1.0 / 16384, 1.0 / 16384, 0.5 };
  double x[6], f[5][6], r = 0;
  char serr[256], mod[] = "0,9,1,0,0,0,0,0";
  int i, j, k;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_astro_models_r(c, mod, 0);
  for (i = 0; i < 4; i++) {
    double t = ts + dt[i];
    int dir = (t < ts) ? -1 : 1, centred = (i == 0 || i == 3);
    if (swe_calc_r(c, t, SE_MARS, SEFLG_JPLEPH | SEFLG_SPEED, x, serr) < 0) {
      swe_ctx_free(c);
      return -1;
    }
    for (k = 0; k < 5; k++)
      if (swe_calc_r(c, centred ? t + (k - 2) * h : t + dir * k * h, SE_MARS, SEFLG_JPLEPH,
                     f[k], serr) < 0) {
        swe_ctx_free(c);
        return -1;
      }
    for (j = 0; j < 2; j++) {
      double d = 0;
      if (centred)
        d = (8 * (f[3][j] - f[1][j]) - (f[4][j] - f[0][j])) / (12 * h);
      else {
        for (k = 0; k < 5; k++)
          d += o5[k] * (f[k][j] - f[0][j]);
        d *= dir / h;
      }
      if (fabs(x[3 + j] - d) > r)
        r = fabs(x[3 + j] - d);
    }
  }
  swe_ctx_free(c);
  return r;
}

/* UPSTREAM-BUGS 183: the speeds across a join of Owen 1990's pieces. On
 * the Swiss files with Owen 1990 the long-term model ("0,10"), Mars's
 * ecliptic speeds at the join near +6000 (JD 3912544.5, the later
 * piece's) -+ 0.5 day and -+ 2^-14 day against five-point differences of
 * its places over 2^-6 day on the instant's side (centred, else
 * one-sided), the largest, deg/day. The shipped build differenced the
 * precession across the join within a day either side (1.1e-8);
 * upstream's speeds of date are off there by the defects earlier entries
 * fixed. */
static double owen_join_speed_off(void)
{
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  const double ts = 3912544.5, h = 1.0 / 64;
  const double dt[4] = { -0.5, -1.0 / 16384, 1.0 / 16384, 0.5 };
  double x[6], f[5][6], r = 0;
  char serr[256], mod[] = "0,10,0,0,0,0,0,0";
  int i, j, k;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_astro_models_r(c, mod, 0);
  for (i = 0; i < 4; i++) {
    double t = ts + dt[i];
    int dir = (t < ts) ? -1 : 1, centred = (i == 0 || i == 3);
    if (swe_calc_r(c, t, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, x, serr) < 0) {
      swe_ctx_free(c);
      return -1;
    }
    for (k = 0; k < 5; k++)
      if (swe_calc_r(c, centred ? t + (k - 2) * h : t + dir * k * h, SE_MARS, SEFLG_SWIEPH,
                     f[k], serr) < 0) {
        swe_ctx_free(c);
        return -1;
      }
    for (j = 0; j < 2; j++) {
      double d = 0;
      if (centred)
        d = (8 * (f[3][j] - f[1][j]) - (f[4][j] - f[0][j])) / (12 * h);
      else {
        for (k = 0; k < 5; k++)
          d += o5[k] * (f[k][j] - f[0][j]);
        d *= dir / h;
      }
      if (fabs(x[3 + j] - d) > r)
        r = fabs(x[3 + j] - d);
    }
  }
  swe_ctx_free(c);
  return r;
}

/* UPSTREAM-BUGS 184: the Moshier Moon's speed across a short-term
 * precession window's edge. With IAU 1976 short-term and Vondrak long-term
 * ("0,9,1"), the Moon's ecliptic speeds on Moshier at J2000 + 2 centuries
 * (the edge's instant the window's) -+ 0.5 day and -+ 2^-14 day against
 * five-point differences of its places over 2^-6 day on the instant's side
 * (centred, else one-sided), the largest, deg/day. The shipped build
 * differenced the Moon's frame, R(t -+ 1), across the edge within a day
 * either side (8.7e-5); upstream's speed differences its places over
 * -+ 5e-5 day, each turned with the instant's cached obliquity. */
static double mosh_moon_window_off(void)
{
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  const double ts = 2524595.0, h = 1.0 / 64;
  const double dt[4] = { -0.5, -1.0 / 16384, 1.0 / 16384, 0.5 };
  double x[6], f[5][6], r = 0;
  char serr[256], mod[] = "0,9,1,0,0,0,0,0";
  int i, j, k;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_astro_models_r(c, mod, 0);
  for (i = 0; i < 4; i++) {
    double t = ts + dt[i];
    int dir = (t <= ts) ? -1 : 1, centred = (i == 0 || i == 3);
    if (swe_calc_r(c, t, SE_MOON, SEFLG_MOSEPH | SEFLG_SPEED, x, serr) < 0) {
      swe_ctx_free(c);
      return -1;
    }
    for (k = 0; k < 5; k++)
      if (swe_calc_r(c, centred ? t + (k - 2) * h : t + dir * k * h, SE_MOON, SEFLG_MOSEPH,
                     f[k], serr) < 0) {
        swe_ctx_free(c);
        return -1;
      }
    for (j = 0; j < 2; j++) {
      double d = 0;
      if (centred)
        d = (8 * (f[3][j] - f[1][j]) - (f[4][j] - f[0][j])) / (12 * h);
      else {
        for (k = 0; k < 5; k++)
          d += o5[k] * (f[k][j] - f[0][j]);
        d *= dir / h;
      }
      if (fabs(x[3 + j] - d) > r)
        r = fabs(x[3 + j] - d);
    }
  }
  swe_ctx_free(c);
  return r;
}

/* UPSTREAM-BUGS 185: the speeds of elements of the equinox of date across a
 * short-term precession window's edge. On the Swiss files with "0,9,1",
 * geometric (SEFLG_TRUEPOS), at J2000 + 2 centuries (the edge's instant
 * the window's) -+ 0.5 day and -+ 2^-14 day: Selena (seorbel.txt, JDATE)
 * and swe_calc_orbel() of an Earth orbit of 0.053 AU (SE_ORBEL_EQ_DATE,
 * moved by its mean motion), their ecliptic speeds against five-point
 * differences of their places over 2^-6 day on the instant's side
 * (centred, else one-sided), the largest, deg/day. The shipped build
 * differenced the frame, R(t -+ 1), across the edge within a day either
 * side (8.7e-5); upstream's Selena moves on a fixed ellipse in a fixed
 * frame, and the compat build keeps the orbit's central difference. */
static double jdate_window_off(void)
{
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  const double ts = 2524595.0, h = 1.0 / 64;
  const double dt[4] = { -0.5, -1.0 / 16384, 1.0 / 16384, 0.5 };
  const int32 fl = SEFLG_SWIEPH | SEFLG_TRUEPOS;
  double x[6], f[5][6], r = 0;
  char serr[256], mod[] = "0,9,1,0,0,0,0,0";
  int i, j, k, b;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_astro_models_r(c, mod, 0);
  for (b = 0; b < 2; b++)
    for (i = 0; i < 4; i++) {
      double t = ts + dt[i];
      int dir = (t <= ts) ? -1 : 1, centred = (i == 0 || i == 3);
      for (k = -1; k < 5; k++) {
        double tk = k < 0 ? t : centred ? t + (k - 2) * h : t + dir * k * h;
        double *y = k < 0 ? x : f[k];
        int32 fk = k < 0 ? fl | SEFLG_SPEED : fl;
        int32 rc;
        if (b == 0) {
          rc = swe_calc_r(c, tk, SE_WHITE_MOON, fk, y, serr);
        } else {
          double el[6] = { 40, 0.053, 0.05, 30, 60, 5 };
          el[0] += sqrt(0.0000298122353216 * 0.0000298122353216 / (el[1] * el[1] * el[1]))
                   * 180 / M_PI * (tk - ts);
          rc = swe_calc_orbel_r(c, tk, el, SE_EARTH, SE_ORBEL_EQ_DATE, 0, fk, y, serr);
        }
        if (rc < 0) {
          swe_ctx_free(c);
          return -1;
        }
      }
      for (j = 0; j < 2; j++) {
        double d = 0;
        if (centred)
          d = (8 * (f[3][j] - f[1][j]) - (f[4][j] - f[0][j])) / (12 * h);
        else {
          for (k = 0; k < 5; k++)
            d += o5[k] * (f[k][j] - f[0][j]);
          d *= dir / h;
        }
        if (fabs(x[3 + j] - d) > r)
          r = fabs(x[3 + j] - d);
      }
    }
  swe_ctx_free(c);
  return r;
}

/* UPSTREAM-BUGS 188: the direct IAU 2000B series past 4e8 days, its
 * arguments' rounding. SE_ECL_NUT's dpsi and deps under IAU 2000B set
 * explicitly (upstream's default) at eight instants 8e8 to 1e9 days either
 * side of J2000, their fourth differences over 2^-12 day / sqrt(70), the
 * worst, deg. The terms' own curvature puts it at 1e-16; the arguments'
 * polynomials evaluated in double, some 1e14" there, jittered it by 4e-9
 * in the shipped build before the fix and do in upstream's. */
static double nut_args_jitter(void)
{
  static const double dj[8] = { -9.9e8, -9.3e8, -8.6e8, -8.1e8, 8.1e8, 8.6e8, 9.3e8, 9.9e8 };
  const double h = 1.0 / 4096;
  double worst = 0;
  char serr[256], mod[] = "0,0,0,4";
  int d, k, j;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_astro_models_r(c, mod, 0);
  for (d = 0; d < 8; d++) {
    double t = floor((2451545.0 + dj[d] + 0.3) * 256) / 256, v[5][6];
    for (k = 0; k < 5; k++)
      if (swe_calc_r(c, t + (k - 2) * h, SE_ECL_NUT, SEFLG_MOSEPH, v[k], serr) < 0) {
        swe_ctx_free(c);
        return -1;
      }
    for (j = 2; j < 4; j++) {
      double e = fabs(v[0][j] + v[4][j] - 4 * (v[1][j] + v[3][j]) + 6 * v[2][j]) / sqrt(70);
      if (e > worst)
        worst = e;
    }
  }
  swe_ctx_free(c);
  return worst;
}

/* UPSTREAM-BUGS 163: the nutation's rate far from J2000. The Sun's right
 * ascension speed minus the same with SEFLG_NONUT under IAU 2000A set
 * explicitly, against a five-point difference of the same two calls'
 * positions over 2^-6 day, the worst of eight instants 2.6e6 to 5.3e6 days
 * either side of J2000, deg/day. The shipped build differenced the series
 * over -+1e-4 day, its rounding there over 2e-4 day (4.2e-10); upstream's
 * backward difference over 1e-4 day has it twice and bug 58's half step
 * besides (1.9e-9). */
static double nut_far_rate_off(void)
{
  static const double dj[8] = { -5.3e6, -4.1e6, -3.3e6, -2.6e6, 2.6e6, 3.3e6, 4.1e6, 5.3e6 };
  double h = 1.0 / 64, worst = 0;
  int32 fl = SEFLG_SWIEPH | SEFLG_EQUATORIAL;
  char serr[256], mod[] = "0,0,0,3";
  int d, k;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_astro_models_r(c, mod, 0);
  for (d = 0; d < 8; d++) {
    double t = floor((2451545.0 + dj[d]) * 256) / 256, xa[6], xb[6], q[5], e;
    if (swe_calc_r(c, t, SE_SUN, fl | SEFLG_SPEED, xa, serr) < 0
        || swe_calc_r(c, t, SE_SUN, fl | SEFLG_SPEED | SEFLG_NONUT, xb, serr) < 0) {
      swe_ctx_free(c);
      return -1;
    }
    for (k = 0; k < 5; k++) {
      double ya[6], yb[6];
      if (swe_calc_r(c, t + (k - 2) * h, SE_SUN, fl, ya, serr) < 0
          || swe_calc_r(c, t + (k - 2) * h, SE_SUN, fl | SEFLG_NONUT, yb, serr) < 0) {
        swe_ctx_free(c);
        return -1;
      }
      q[k] = swe_difdeg2n(ya[0], yb[0]);
    }
    e = fabs(xa[3] - xb[3] - (8 * (q[3] - q[1]) - (q[4] - q[0])) / (12 * h));
    if (e > worst)
      worst = e;
  }
  swe_ctx_free(c);
  return worst;
}

/* UPSTREAM-BUGS 154: the IERS corrections across swe_set_jpl_file() of a
 * file below DE 403. G134's directory A (its corrections, de200.eph linked
 * in): path A, the DE431 fixture, SE_ECL_NUT with SEFLG_JPLHOR at 2000 Jan
 * 7, then de200.eph, and dpsi at Jan 7.37 against a fresh context given A
 * and de200.eph, arcsec; -1 if the fixtures cannot be written. Upstream
 * keeps A's under SEFLG_JPLHOR where the fresh context falls to
 * SEFLG_JPLHOR_APPROX: 4.19e-2". */
/* UPSTREAM-BUGS 171: G134's fixture, eop_1962_today.txt for 2000 Jan 1 to
 * Feb 9 and an eop_finals.txt whose one day, Feb 12, leaves a gap after
 * it: dpsi with SEFLG_JPLHOR at 2000 Jan 7.37 on a context set to that
 * directory and the fixture, against one set after eop_finals.txt is
 * removed, arcsec. The broken finals file threw the complete
 * eop_1962_today.txt away (-3), and SEFLG_JPLHOR fell to
 * SEFLG_JPLHOR_APPROX, 4.19e-2" off; now its days are kept, 0. */
static double eop_finals_cut_off(void)
{
  char base[400], p[480], l[200], jf[] = "de431fixture.eph";
  const char *tmp = getenv("TMPDIR");
  double x[6], y[6], t = 2451550.87, r = -1;
  char serr[256];
  const int32 jh = SEFLG_JPLEPH | SEFLG_JPLHOR;
  swe_ctx *c;
  FILE *fp;
  snprintf(base, sizeof base, "%s/compat171.XXXXXX", tmp && *tmp ? tmp : "/tmp");
  if (mkdtemp(base) == NULL)
    return -1;
  snprintf(p, sizeof p, "%s/%s", base, jf);
  if (eop_write_jpl(p) != 0)
    goto out;
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", base);
  if (eop_write_eop(p, -0.05, -1e-5, -0.005, -1e-6) != 0)
    goto out;
  snprintf(p, sizeof p, "%s/eop_finals.txt", base);
  if ((fp = fopen(p, "w")) == NULL)
    goto out;
  memset(l, ' ', sizeof l - 1);	/* finals.all: MJD at 7, Bulletin B at 168, 178 */
  l[sizeof l - 1] = '\0';
  l[7 + snprintf(l + 7, 8, "%5d", 51586)] = ' ';
  l[168 + snprintf(l + 168, 9, "%7.3f", -50.0)] = ' ';
  l[178 + snprintf(l + 178, 9, "%7.3f", -5.0)] = ' ';
  fprintf(fp, "%s\n", l);
  if (fclose(fp) != 0 || (c = swe_ctx_new()) == NULL)
    goto out;
  swe_set_ephe_path_r(c, base);
  swe_set_jpl_file_r(c, jf);
  swe_calc_r(c, t, SE_ECL_NUT, jh, x, serr);
  swe_ctx_free(c);
  unlink(p);
  if ((c = swe_ctx_new()) == NULL)
    goto out;
  swe_set_ephe_path_r(c, base);
  swe_set_jpl_file_r(c, jf);
  swe_calc_r(c, t, SE_ECL_NUT, jh, y, serr);
  swe_ctx_free(c);
  r = fabs(x[2] - y[2]) * 3600;
out:
  snprintf(p, sizeof p, "%s/%s", base, jf); unlink(p);
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", base); unlink(p);
  snprintf(p, sizeof p, "%s/eop_finals.txt", base); unlink(p);
  rmdir(base);
  return r;
}

static double eop_jpl_kept_off(void)
{
  char base[400], p[480], q[PATH_MAX + 20], de200[PATH_MAX];
  char jf[] = "de431fixture.eph", jd[] = "de200.eph";
  const char *tmp = getenv("TMPDIR");
  double x[6], y[6], t = 2451550.5, r = -1;
  char serr[256];
  const int32 jh = SEFLG_JPLEPH | SEFLG_JPLHOR;
  swe_ctx *c;
  snprintf(base, sizeof base, "%s/compat154.XXXXXX", tmp && *tmp ? tmp : "/tmp");
  if (mkdtemp(base) == NULL)
    return -1;
  snprintf(q, sizeof q, "%s/%s", EPHE, jd);
  snprintf(p, sizeof p, "%s/%s", base, jd);
  if (realpath(q, de200) == NULL || symlink(de200, p) != 0)
    goto out;
  snprintf(p, sizeof p, "%s/%s", base, jf);
  if (eop_write_jpl(p) != 0)
    goto out;
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", base);
  if (eop_write_eop(p, -0.05, -1e-5, -0.005, -1e-6) != 0)
    goto out;
  swe_close();
  swe_set_ephe_path(base);
  swe_set_jpl_file(jf);
  swe_calc(t, SE_ECL_NUT, jh, x, serr);
  swe_set_jpl_file(jd);
  swe_calc(t + 0.37, SE_ECL_NUT, jh, x, serr);
  if ((c = swe_ctx_new()) == NULL)
    goto out;
  swe_set_ephe_path_r(c, base);
  swe_set_jpl_file_r(c, jd);
  swe_calc_r(c, t + 0.37, SE_ECL_NUT, jh, y, serr);
  swe_ctx_free(c);
  r = fabs(x[2] - y[2]) * 3600;
out:
  swe_close();
  swe_set_ephe_path(EPHE);
  snprintf(p, sizeof p, "%s/%s", base, jd); unlink(p);
  snprintf(p, sizeof p, "%s/%s", base, jf); unlink(p);
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", base); unlink(p);
  rmdir(base);
  return r;
}

/* UPSTREAM-BUGS 161: SEFLG_JPLHOR_APPROX under SEMOD_JPLHORA_3, the frame
 * bias and the Horizons offset in right ascension against the side of the
 * step at 1962 Jan 20 that the precession and nutation take, deg. star 0:
 * Mars 2^-8 day after the step, inside its light time, against
 * SEMOD_JPLHORA_1 (the chosen models, the bias and the offset throughout),
 * on the Swiss files (DE 431) that SEFLG_JPLEPH falls to with the JPL file
 * missing and the fallback allowed: upstream gives the bias the instant
 * read, 1.23e-5. star 1: barycentric Polaris on de200 (the bias whatever the
 * DE number) at the step against 2^-20 day before it less its motion:
 * upstream's bias starts at the step, 4.92e-5. -1 if a call fails. */
/* UPSTREAM-BUGS 173: barycentric Polaris on de200 with SEFLG_JPLHOR_APPROX
 * under SEMOD_JPLHORA_1, right ascension, across 2006 Jul 1, a year of the
 * Horizons offset's table: the place 2^-10 day after less that 2^-10 day
 * before less 2^-9 day of their mean speed, degrees. The table was
 * interpolated backwards, and the offset jumped 0.99 mas there (2.87e-7
 * deg); 1.5e-11 now. */
static double jplhora_table_jump(void)
{
  const double b = 2437846.5 + 44 * 365.25, e = ldexp(1, -10);
  const int32 ja = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX | SEFLG_EQUATORIAL | SEFLG_SPEED | SEFLG_BARYCTR;
  double x[6], y[6], d, r = -1;
  char serr[256], s[64], m1[] = "0,0,0,0,0,0,1,0";
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return r;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_astro_models_r(c, m1, 0);
  strcpy(s, "Polaris");
  if (swe_fixstar2_r(c, s, b - e, ja, x, serr) >= 0) {
    strcpy(s, "Polaris");
    if (swe_fixstar2_r(c, s, b + e, ja, y, serr) >= 0) {
      d = y[0] - x[0];
      r = fabs(d - 360 * floor(d / 360 + 0.5) - e * (x[3] + y[3]));
    }
  }
  swe_ctx_free(c);
  return r;
}

/* UPSTREAM-BUGS 182: SE_ECL_NUT's mean obliquity on de200 with
 * SEFLG_JPLHOR_APPROX under SEMOD_JPLHORA_1, its second difference over
 * 2^-10 day at 1972 Jul 1, a year of the obliquity's offset table,
 * degrees: interpolated backwards, the offset jumped 0.44 mas there
 * (1.22e-7 deg); 1.6e-13 now. */
static double jplhora_eps_table_jump(void)
{
  const double b = 2437846.5 + 10 * 365.25, e = ldexp(1, -10);
  const int32 ja = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX;
  double x[6], y[6], z[6], r = -1;
  char serr[256], m1[] = "0,0,0,0,0,0,1,0";
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return r;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_astro_models_r(c, m1, 0);
  if (swe_calc_r(c, b - e, SE_ECL_NUT, ja, x, serr) >= 0
      && swe_calc_r(c, b, SE_ECL_NUT, ja, z, serr) >= 0
      && swe_calc_r(c, b + e, SE_ECL_NUT, ja, y, serr) >= 0)
    r = fabs(y[1] - 2 * z[1] + x[1]);
  swe_ctx_free(c);
  return r;
}

static double jplhora_bias_side_off(int star)
{
  const double ts = 2437684.5, e = ldexp(1, -20);
  const int32 ja = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX | SEFLG_EQUATORIAL | SEFLG_SPEED;
  double x[6], y[6], r = -1;
  char serr[256], s[64], m1[] = "0,0,0,0,0,0,1,0";
  int32 fx, fy;
  swe_ctx *c = swe_ctx_new(), *c1 = swe_ctx_new();
  if (c == NULL || c1 == NULL)
    goto out;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_ephe_path_r(c1, EPHE);
  if (star) {
    swe_set_jpl_file_r(c, "de200.eph");
    strcpy(s, "Polaris");
    fx = swe_fixstar2_r(c, s, ts, ja | SEFLG_BARYCTR, x, serr);
    strcpy(s, "Polaris");
    fy = swe_fixstar2_r(c, s, ts - e, ja | SEFLG_BARYCTR, y, serr);
    y[0] += y[3] * e;
    y[1] += y[4] * e;
  } else {
    swe_set_ephe_fallback_r(c, 1);
    swe_set_ephe_fallback_r(c1, 1);
    swe_set_jpl_file_r(c, "de-none.eph");
    swe_set_jpl_file_r(c1, "de-none.eph");
    swe_set_astro_models_r(c1, m1, 0);
    fx = swe_calc_r(c, ts + 1.0 / 256, SE_MARS, ja, x, serr);
    fy = swe_calc_r(c1, ts + 1.0 / 256, SE_MARS, ja, y, serr);
  }
  if (fx >= 0 && fy >= 0 && (fx & fy & SEFLG_JPLHOR_APPROX))
    r = fmax(fabs(x[0] - y[0]), fabs(x[1] - y[1]));
out:
  if (c != NULL)
    swe_ctx_free(c);
  if (c1 != NULL)
    swe_ctx_free(c1);
  return r;
}

/* UPSTREAM-BUGS 162: SEFLG_JPLHOR_APPROX's offset in right ascension turns
 * the position about the pole and not the velocity. True Mars, J2000
 * equatorial cartesian (no frame rate in it), on the Swiss files (DE 431)
 * that SEFLG_JPLEPH falls to with the JPL file missing and the fallback
 * allowed, at a binary instant in 2000: the velocity against a five-point
 * difference of the positions over 2^-6 day, the largest component,
 * AU/day; upstream 4.57e-9. -1 if a call fails. */
static double jplhora_vel_off(void)
{
  const double t = floor(2451545.3 * 256) / 256, h = 1.0 / 64;
  const int32 fl = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX | SEFLG_TRUEPOS | SEFLG_J2000
                   | SEFLG_EQUATORIAL | SEFLG_XYZ;
  double x[6], f[5][6], r = -1;
  char serr[256];
  int32 rf;
  int i, k;
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return -1;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_ephe_fallback_r(c, 1);
  swe_set_jpl_file_r(c, "de-none.eph");
  rf = swe_calc_r(c, t, SE_MARS, fl | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5 && rf >= 0; k++)
    if (k != 2 && swe_calc_r(c, t + (k - 2) * h, SE_MARS, fl, f[k], serr) < 0)
      rf = -1;
  if (rf >= 0 && (rf & SEFLG_JPLHOR_APPROX))
    for (i = 0, r = 0; i < 3; i++) {
      double d = (8 * (f[3][i] - f[1][i]) - (f[4][i] - f[0][i])) / (12 * h);
      if (fabs(x[3 + i] - d) > r)
        r = fabs(x[3 + i] - d);
    }
  swe_ctx_free(c);
  return r;
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

/* swe_houses_ex2()'s cusp speeds against a five-point difference of
 * swe_houses_ex()'s cusps over 2^-12 day, at 47N on 2026-07-02 (a
 * multiple of 2^-8 day), deg/day: Koch's cusp 11 (the rate of the MC's
 * ascensional difference left out, UPSTREAM-BUGS 41, ~200 upstream) and
 * whole signs' cusp 1 (the Asc's speed on a cusp that stands still, bug
 * 40, ~400) when sidereal is 0; with sidereal 1, the equal houses' Asc
 * under Lahiri, which upstream moves at ARMCS without the equation of the
 * equinoxes' and the ayanamsa's rates (bugs 42, 43, ~5e-5). */
static double house_speed_off(int sidereal)
{
  const double jd = 2461223.5 + 0.3125, h = 1.0 / 4096;
  static const int hs[2] = {'K', 'W'};
  static const int ic[2] = {11, 1};
  double cusp[13], ascmc[10], cs[13], as[10], f[4], worst = 0;
  int32 fl = sidereal ? SEFLG_SIDEREAL : 0;
  int i, k, n = sidereal ? 1 : 2;
  swe_close();
  swe_set_ephe_path(EPHE);
  if (sidereal)
    swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  for (i = 0; i < n; i++) {
    int sys = sidereal ? 'E' : hs[i], c = sidereal ? 1 : ic[i];
    double ref, d1, d2;
    for (k = 0; k < 4; k++) {
      static const int off[4] = {-2, -1, 1, 2};
      swe_houses_ex(jd + off[k] * h, fl, 47.37, 8.55, sys, cusp, ascmc);
      f[k] = cusp[c];
    }
    swe_houses_ex2(jd, fl, 47.37, 8.55, sys, cusp, ascmc, cs, as, NULL);
    d1 = swe_difdeg2n(f[2], f[1]);
    d2 = swe_difdeg2n(f[3], f[0]);
    ref = (8 * d1 - d2) / (12 * h);
    if (fabs(cs[c] - ref) > worst)
      worst = fabs(cs[c] - ref);
  }
  swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  return worst;
}

/* UPSTREAM-BUGS 59, the four small house defects, as the number present
 * (0..4): the Placidus cusp speed at 60N against a five-point difference
 * (> 1e-6 deg/day), the SEFLG_RADIANS cusp speed left in degrees, sidereal
 * Makransky ('i') equal to Treindl's ('I') and Sunshine houses failing
 * without the Swiss files. *placid gets the Placidus error, deg/day. */
static int house_fix_off(double *placid)
{
  const double jd = 2461223.5 + 0.3125, h = 1.0 / 4096;
  double cusp[13], ascmc[10], cs[13], as[10], csr[13], f[4], c2[13], c3[13];
  char serr[256];
  int k, n = 0;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (k = 0; k < 4; k++) {
    static const int off[4] = {-2, -1, 1, 2};
    swe_houses_ex(jd + off[k] * h, 0, 60, 8.55, 'P', cusp, ascmc);
    f[k] = cusp[11];
  }
  swe_houses_ex2(jd, 0, 60, 8.55, 'P', cusp, ascmc, cs, as, NULL);
  *placid = fabs(cs[11] - (8 * (swe_difdeg2n(f[2], f[1])) - swe_difdeg2n(f[3], f[0])) / (12 * h));
  if (*placid > 1e-6)
    n++;
  swe_houses_ex2(jd, SEFLG_RADIANS, 60, 8.55, 'P', cusp, ascmc, csr, as, NULL);
  if (fabs(csr[11] - cs[11]) < 1e-9 * fabs(cs[11]))
    n++;
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_houses_ex(floor((2461000.5 + 5.0 / 48 + 200) * 256) / 256, SEFLG_SIDEREAL, -60, 8.55, 'i', c2, ascmc);
  swe_houses_ex(floor((2461000.5 + 5.0 / 48 + 200) * 256) / 256, SEFLG_SIDEREAL, -60, 8.55, 'I', c3, ascmc);
  swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  for (k = 1; k <= 12; k++)
    if (fabs(swe_difdeg2n(c2[k], c3[k])) > 1)
      break;
  if (k > 12)
    n++;
  swe_close();
  swe_set_ephe_path("/nonexistent-ephe-dir");
  if (swe_houses_ex2(jd, 0, 47, 8.55, 'I', cusp, ascmc, NULL, NULL, serr) < 0)
    n++;
  swe_close();
  return n;
}

/* UPSTREAM-BUGS 68, three house residues, as the number present (0..3):
 * a SEFLG_MOSEPH Sunshine call returning the Swiss Sun's declination,
 * Gauquelin at 80N (fallen back to Porphyry) leaving cusp 13 as the caller
 * had it (7), by swe_houses_ex2() or swe_houses_armc_ex2(), and the Placidus cusp 11 speed at the ARMC (330) that puts it
 * on the equinox point being ARMCS (465.61 against 360.99). *plac gets the
 * latter's error from a five-point difference, deg/day. */
static int house_resid_off(double *plac)
{
  const double jd = 2461223.5 + 0.3125, h = 1.0 / 256;
  double c[37], a[10], cs[37], as[10], xs[6], f[4];
  char serr[256];
  int i, k, n = 0;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc_ut(jd, SE_SUN, SEFLG_EQUATORIAL, xs, serr);
  swe_houses_ex2(jd, SEFLG_MOSEPH, 47, 8.55, 'I', c, a, NULL, NULL, serr);
  if (fabs(a[9] - xs[1]) < 1e-9)
    n++;
  for (k = 0; k < 37; k++)
    c[k] = 7;
  swe_houses_ex2(jd, SEFLG_RADIANS, 80, 8.55, 'G', c, a, cs, as, serr);
  k = c[13] != 0;
  for (i = 0; i < 37; i++)
    c[i] = 7;
  swe_houses_armc_ex2(100, 80, 23.44, 'G', c, a, NULL, NULL, serr);
  if (k || c[13] != 0)
    n++;
  for (k = 0; k < 4; k++) {
    static const int off[4] = {-2, -1, 1, 2};
    swe_houses_armc(330 + off[k] * h, 47, 23.4392911, 'P', c, a);
    f[k] = c[11];
  }
  swe_houses_armc_ex2(330, 47, 23.4392911, 'P', c, a, cs, as, serr);
  *plac = fabs(cs[11] - (8 * (swe_difdeg2n(f[2], f[1])) - swe_difdeg2n(f[3], f[0])) / (12 * h) * 360.98564736629);
  if (*plac > 1e-3)
    n++;
  swe_close();
  return n;
}

/* The Moon's speed against five-point differences of its longitude, deg/day,
 * worst of two: geocentric apparent over 2^-6 day, and topocentric true at
 * Zurich over 2^-9 day with delta t set to 2^-11 day (UT1 exact, the site
 * without jitter), at 500 and 2000. Here: the site turns at the sidereal
 * time's rate with the nutation matrix's, the files' rate is per TT day,
 * the velocity is moved to teval - tau with the place, the aberration's
 * shift rate is central (UPSTREAM-BUGS 73): 2.4e-9. Upstream: 9.7e-4 (its
 * topocentric paths, bugs 31-36, besides). */
static double moon_site_speed_off(void)
{
  static const double yr[2] = {500, 2000};
  double w = 0;
  char serr[256];
  int c, y, k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 47.37, 400);
  for (c = 0; c < 2; c++) {
    int32 fl = SEFLG_SWIEPH | (c ? SEFLG_TOPOCTR | SEFLG_TRUEPOS : 0);
    double h = c ? 1.0 / 512 : 1.0 / 64;
    if (c)
      swe_set_delta_t_userdef(1.0 / 2048);
    for (y = 0; y < 2; y++) {
      double t = floor((2451545.0 + (yr[y] - 2000) * 365.25) * 256) / 256, x[6], f[4];
      static const int off[4] = {-2, -1, 1, 2};
      swe_calc(t, SE_MOON, fl | SEFLG_SPEED, x, serr);
      for (k = 0; k < 4; k++) {
        double g[6];
        swe_calc(t + off[k] * h, SE_MOON, fl, g, serr);
        f[k] = g[0];
      }
      x[3] -= (8 * swe_difdeg2n(f[2], f[1]) - swe_difdeg2n(f[3], f[0])) / (12 * h);
      if (fabs(x[3]) > w)
        w = fabs(x[3]);
    }
  }
  swe_set_delta_t_userdef(SE_DELTAT_AUTOMATIC);
  swe_close();
  return w;
}

/* UPSTREAM-BUGS 93: Neptune's mean ascending node, geocentric, default
 * flags, Moshier, at 2000 BC (J2000 - 4000 years + 100 days): its latitude
 * speed against a five-point difference of its latitude over 2^-6 day,
 * deg/day. The aberration's rate took the Earth's acceleration from the
 * Taylor form at the light time, off by 1e-9 AU/day^2 on Moshier's Earth,
 * 1.7e-10 deg/day here; upstream's rate is wrong by 1e-2 besides. */
static double nodaps_mosh_lat_off(void)
{
  double jd = floor((2451545.0 - 4000 * 365.25 + 100.37) * 256) / 256, h = 1.0 / 64;
  double x[6], f[5][6], u[6];
  int32 fl = SEFLG_MOSEPH;
  char serr[256];
  int k;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_nod_aps(jd, SE_NEPTUNE, fl | SEFLG_SPEED, SE_NODBIT_MEAN, x, u, u, u, serr);
  for (k = 0; k < 5; k++)
    swe_nod_aps(jd + (k - 2) * h, SE_NEPTUNE, fl, SE_NODBIT_MEAN, f[k], u, u, u, serr);
  return fabs(x[4] - (f[0][1] - 8 * f[1][1] + 8 * f[3][1] - f[4][1]) / (12 * h));
}

/* UPSTREAM-BUGS 96: Waldemath on Moshier, geocentric equatorial J2000 XYZ,
 * the largest fourth difference of its place over 2^-10 day, 64 instants
 * from 2026-10-24, AU: 2.5e-15 shipped, 1.3e-14 before the fix, 1.2e-13
 * compat. The Earth at the emission instant was a second read of the
 * series, whose rounding the Earth at teval does not share. */
static double mosh_geo_lt_jitter(void)
{
  const double h = 1.0 / 1024, t0 = 2461337.5;
  double x[68][6], w = 0;
  char serr[256];
  int m, i;
  swe_close();
  swe_set_ephe_path(EPHE);
  for (m = 0; m < 68; m++)
    swe_calc(t0 + m * h, SE_WALDEMATH, SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_NONUT
             | SEFLG_EQUATORIAL | SEFLG_XYZ, x[m], serr);
  for (m = 2; m < 66; m++)
    for (i = 0; i < 3; i++) {
      double d4 = x[m-2][i] - 4 * x[m-1][i] + 6 * x[m][i] - 4 * x[m+1][i] + x[m+2][i];
      if (fabs(d4) > w)
        w = fabs(d4);
    }
  return w;
}

/* The path, a Swiss Moon and swe_close(), then delta t in AD 1000 by
 * swe_deltat_ex(), against a fresh context given the path, s. Upstream's
 * close forgets the moon file's DE number and keeps the path, so this
 * first delta t, before any file opens, takes the default tidal term:
 * 1463.43 s for 1474.72 (UPSTREAM-BUGS 131). */
static double close_tid_off(void)
{
  double x[6], dt, ref;
  char serr[256];
  swe_ctx *c;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc_ut(2451600.0, SE_MOON, SEFLG_SWIEPH, x, serr);
  swe_close();
  dt = swe_deltat_ex(2086308.0, SEFLG_SWIEPH, serr);
  if ((c = swe_ctx_new()) == NULL)
    return INFINITY;
  swe_set_ephe_path_r(c, EPHE);
  ref = swe_deltat_ex_r(c, 2086308.0, SEFLG_SWIEPH, serr);
  swe_ctx_free(c);
  swe_close();
  swe_set_ephe_path(EPHE);
  return fabs(dt - ref) * 86400;
}

/* swe_get_tid_acc() after a path whose de431.eph, the JPL file's default
 * name, is de200.eph (a directory of links), no swe_set_jpl_file(), and a
 * TT JPL Sun, which opens it, against a context then made and given that
 * path. Upstream sets the term for the file a calculation opens (its
 * guess follows the open file): de200's -23.8946 for the moon file's
 * -25.936, 2.04 (UPSTREAM-BUGS 138). The path again after
 * swe_set_jpl_file() no longer reaches this: it reads the file's DE number
 * again (bug 148). No JPL file may have been set, so main() calls this
 * first and it runs in a forked child. */
static double jpl_open_tid_off(void)
{
  char dir[] = "/tmp/compatdftXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  double v = -1;
  int fd[2];
  pid_t pid;
  struct dirent *e;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, SE_FNAME_DFT) != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  snprintf(p, sizeof p, "%s/de200.eph", ephe);
  snprintf(q, sizeof q, "%s/%s", dir, SE_FNAME_DFT);
  fflush(stdout);
  if (symlink(p, q) == 0 && pipe(fd) == 0) {
    if ((pid = fork()) == 0) {
      double x[6], g, r = 100;
      char serr[256];
      swe_ctx *c;
      close(fd[0]);
      swe_set_ephe_path(dir);
      swe_calc(2451545.0, SE_SUN, SEFLG_JPLEPH, x, serr);
      g = swe_get_tid_acc();
      if ((c = swe_ctx_new()) != NULL) {
        swe_set_ephe_path_r(c, dir);
        r = swe_get_tid_acc_r(c);
        swe_ctx_free(c);
      }
      g = fabs(g - r);
      _exit(write(fd[1], &g, sizeof g) == (ssize_t) sizeof g ? 0 : 3);
    }
    close(fd[1]);
    if (pid < 0 || read(fd[0], &v, sizeof v) != (ssize_t) sizeof v)
      v = -1;
    close(fd[0]);
    if (pid > 0)
      waitpid(pid, NULL, 0);
  }
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  return v;
}

/* dir: links to EPHE's files but de200.eph, and de200.eph under the name
 * jf; 0, or -1 */
static int jpl_alias_dir(char *dir, const char *jf)
{
  char ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  struct dirent *e;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "de200.eph") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  snprintf(p, sizeof p, "%s/de200.eph", ephe);
  snprintf(q, sizeof q, "%s/%s", dir, jf);
  return symlink(p, q);
}

static void rm_dir(const char *dir)
{
  char q[PATH_MAX + 300];
  struct dirent *e;
  DIR *d;
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
}

/* UPSTREAM-BUGS 194: the Moon files missing (a directory of links to
 * EPHE's others), Mars at 1900 Jan 0.75 with SEFLG_TOPOCTR | SEFLG_HELCTR
 * under the strict setting, its return: bug 120's test read the centre
 * off the caller's bits, so the topocentric answer from the Earth of
 * Moshier's Moon came back, 32770; refused now, -1, as SEFLG_TOPOCTR is.
 * The compat build has no bug 120 and answers both. */
static double moshmoon_topohel_ret(void)
{
  char dir[] = "/tmp/compatthXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  double x[6];
  char serr[AS_MAXCH];
  struct dirent *e;
  DIR *d;
  swe_ctx *c;
  int32 r = -2;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return 1e9;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strncmp(e->d_name, "semo", 4) != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  if ((c = swe_ctx_new()) != NULL) {
    swe_set_ephe_path_r(c, dir);
    swe_set_ephe_fallback_r(c, 0);
    swe_set_topo_r(c, 8.55, 47.37, 400);
    r = swe_calc_r(c, 2415020.25, SE_MARS, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_HELCTR, x, serr);
    swe_ctx_free(c);
  }
  rm_dir(dir);
  return r;
}

/* Topocentric Aldebaran by swe_fixstar2() at AD 1500 along a directory
 * with the planet and Moon files of 1800-2400 and sefstars.txt alone
 * (links to EPHE's), the fallback allowed, against the Moshier request,
 * the longitude, deg. Upstream places the site by the delta t of the
 * caller's SEFLG_SWIEPH, the files', for the Moshier Earth that answered:
 * 2.0e-8 deg (UPSTREAM-BUGS 195). */
static double star_topo_fallen_off(void)
{
  char dir[] = "/tmp/compatstXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  static const char *const f[3] = {"sepl_18.se1", "semo_18.se1", "sefstars.txt"};
  double x[6], y[6], r = 1e9;
  char serr[AS_MAXCH], s[64];
  swe_ctx *a, *b;
  int i;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL)
    return -1;
  for (i = 0; i < 3; i++) {
    snprintf(p, sizeof p, "%s/%s", ephe, f[i]);
    snprintf(q, sizeof q, "%s/%s", dir, f[i]);
    if (symlink(p, q) != 0)
      break;
  }
  a = swe_ctx_new();
  b = swe_ctx_new();
  if (a != NULL && b != NULL) {
    swe_set_ephe_path_r(a, dir);
    swe_set_ephe_path_r(b, dir);
    swe_set_ephe_fallback_r(a, 1);
    swe_set_topo_r(a, 8.55, 47.37, 400);
    swe_set_topo_r(b, 8.55, 47.37, 400);
    strcpy(s, "Aldebaran");
    if (swe_fixstar2_r(a, s, 2268923.5, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr) >= 0) {
      strcpy(s, "Aldebaran");
      if (swe_fixstar2_r(b, s, 2268923.5, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, y, serr) >= 0)
        r = fabs(swe_difdeg2n(x[0], y[0]));
    }
  }
  if (a != NULL)
    swe_ctx_free(a);
  if (b != NULL)
    swe_ctx_free(b);
  rm_dir(dir);
  return r;
}

/* swe_calc_ut()'s JPL Moon in AD 1700 with no JPL file set, the first call
 * against the second, deg, the larger of: a path whose de431.eph (the
 * default name) is de200.eph, and, after swe_close(), a path without
 * de431.eph whose de406.eph (the calculation's fallback) is; 1 if the
 * fallback's warning is not in the first call's serr. Upstream's first
 * delta t takes the default term, the calculation then opening the file:
 * 40.0057511 deg for 40.0039618 both (UPSTREAM-BUGS 165). No JPL file may
 * have been set, so main() calls this first and it runs in a forked
 * child. */
static double jpl_dft_moon_off(void)
{
  char da[] = "/tmp/compatdfaXXXXXX", db[] = "/tmp/compatdfbXXXXXX";
  double v = -1;
  int fd[2];
  pid_t pid;
  if (jpl_alias_dir(da, SE_FNAME_DFT) == 0 && jpl_alias_dir(db, SE_FNAME_DFT2) == 0
      && pipe(fd) == 0) {
    fflush(stdout);
    if ((pid = fork()) == 0) {
      double x[6], y[6], r = 0;
      char serr[256];
      close(fd[0]);
      swe_set_ephe_path(da);
      if (swe_calc_ut(2341972.5, SE_MOON, SEFLG_JPLEPH, x, serr) < 0
          || swe_calc_ut(2341972.5, SE_MOON, SEFLG_JPLEPH, y, serr) < 0)
        x[0] = 1e9;
      r = fabs(swe_difdeg2n(x[0], y[0]));
      swe_close();
      swe_set_ephe_path(db);
      *serr = '\0';
      if (swe_calc_ut(2341972.5, SE_MOON, SEFLG_JPLEPH, x, serr) < 0)
        x[0] = 1e9;
      if (strstr(serr, SE_FNAME_DFT2) == NULL)
        r = 1;
      else if (swe_calc_ut(2341972.5, SE_MOON, SEFLG_JPLEPH, y, serr) < 0)
        r = 1e9;
      else if (fabs(swe_difdeg2n(x[0], y[0])) > r)
        r = fabs(swe_difdeg2n(x[0], y[0]));
      _exit(write(fd[1], &r, sizeof r) == (ssize_t) sizeof r ? 0 : 3);
    }
    close(fd[1]);
    if (pid < 0 || read(fd[0], &v, sizeof v) != (ssize_t) sizeof v)
      v = -1;
    close(fd[0]);
    if (pid > 0)
      waitpid(pid, NULL, 0);
  }
  rm_dir(da);
  rm_dir(db);
  return v;
}

/* swe_deltat_ex() in AD 1000 with SEFLG_SWIEPH along a directory without
 * the moon files (links to EPHE's other files) against EPHE, s, each on a
 * context made and given its path. Upstream's term without the moon file
 * is the default, -25.80, though the planet files are DE441's (-25.936):
 * 1463.430567 s for 1474.717432, 11.29 (UPSTREAM-BUGS 166). */
static double moonless_tid_off(void)
{
  char dir[] = "/tmp/compatnomXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  double a = 0, b = 1e9;
  struct dirent *e;
  swe_ctx *c;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strncmp(e->d_name, "semo", 4) != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  if ((c = swe_ctx_new()) != NULL) {
    swe_set_ephe_path_r(c, dir);
    a = swe_deltat_ex_r(c, 2086308.0, SEFLG_SWIEPH, NULL);
    swe_ctx_free(c);
  }
  if ((c = swe_ctx_new()) != NULL) {
    swe_set_ephe_path_r(c, EPHE);
    b = swe_deltat_ex_r(c, 2086308.0, SEFLG_SWIEPH, NULL);
    swe_ctx_free(c);
  }
  rm_dir(dir);
  return fabs(a - b) * 86400;
}

/* swe_deltat_ex() in AD 1500 with SEFLG_SWIEPH along a directory without
 * the files for J2000, semo_18.se1 and sepl_18.se1 (links to EPHE's other
 * files), against EPHE, s, each on a context made and given its path.
 * Upstream reads the DE number from those two alone, so the first delta t
 * takes the default term, 291.693033 s for 294.255183, 2.56
 * (UPSTREAM-BUGS 175). */
static double noj2000_tid_off(void)
{
  char dir[] = "/tmp/compatno18XXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  double a = 0, b = 1e9;
  struct dirent *e;
  swe_ctx *c;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "semo_18.se1") != 0
        && strcmp(e->d_name, "sepl_18.se1") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  if ((c = swe_ctx_new()) != NULL) {
    swe_set_ephe_path_r(c, dir);
    a = swe_deltat_ex_r(c, 2268923.5, SEFLG_SWIEPH, NULL);
    swe_ctx_free(c);
  }
  if ((c = swe_ctx_new()) != NULL) {
    swe_set_ephe_path_r(c, EPHE);
    b = swe_deltat_ex_r(c, 2268923.5, SEFLG_SWIEPH, NULL);
    swe_ctx_free(c);
  }
  rm_dir(dir);
  return fabs(a - b) * 86400;
}

/* The heliocentric Moon at AD 1500 along a directory with the planet and
 * Moon files of 1800-2400 alone (links to EPHE's), the fallback allowed:
 * on a context that computed a Swiss Sun at 2000 first, against a fresh
 * context, deg. Upstream's Moshier paths read the barycentric Sun's slot,
 * which Moshier does not fill: the Swiss Sun of 2000 taken from the
 * heliocentric Moon, 0.41 deg (UPSTREAM-BUGS 177). */
static double mosh_sun_slot_off(void)
{
  char dir[] = "/tmp/compat18XXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  static const char *const f[2] = {"sepl_18.se1", "semo_18.se1"};
  double x[6], y[6], r = 1e9;
  char serr[AS_MAXCH];
  swe_ctx *a, *b;
  int i;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL)
    return -1;
  for (i = 0; i < 2; i++) {
    snprintf(p, sizeof p, "%s/%s", ephe, f[i]);
    snprintf(q, sizeof q, "%s/%s", dir, f[i]);
    if (symlink(p, q) != 0)
      break;
  }
  a = swe_ctx_new();
  b = swe_ctx_new();
  if (a != NULL && b != NULL) {
    swe_set_ephe_path_r(a, dir);
    swe_set_ephe_path_r(b, dir);
    swe_set_ephe_fallback_r(a, 1);
    swe_set_ephe_fallback_r(b, 1);
    if (swe_calc_r(a, 2451545.0, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, x, serr) >= 0
        && swe_calc_r(a, 2268923.5, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_HELCTR, x, serr) >= 0
        && swe_calc_r(b, 2268923.5, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_HELCTR, y, serr) >= 0)
      r = fabs(swe_difdeg2n(x[0], y[0])) + fabs(x[1] - y[1]);
  }
  if (a != NULL)
    swe_ctx_free(a);
  if (b != NULL)
    swe_ctx_free(b);
  rm_dir(dir);
  return r;
}

/* UPSTREAM-BUGS 174: swe_nod_aps() of the Moon's mean points at 1900 Jan
 * 0.75, Swiss, geocentric, along an empty directory under the strict
 * setting, on a context made and given its path: the return. The points
 * read no file, and swe_calc() answers SE_MEAN_NODE there, but upstream
 * reads the Moon for the save area, which the strict setting refuses (-1,
 * "Swiss (.se1) ephemeris is not available"); answered now (0). */
static double nodaps_moon_mean_ret(void)
{
  char dir[] = "/tmp/compatnmmXXXXXX";
  double xn[6], xd[6], xp[6], xa[6], r = -99;
  char serr[256];
  swe_ctx *c;
  if (mkdtemp(dir) == NULL)
    return -99;
  if ((c = swe_ctx_new()) != NULL) {
    swe_set_ephe_path_r(c, dir);
    swe_set_ephe_fallback_r(c, 0);
    r = swe_nod_aps_r(c, 2415020.25, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN,
                      xn, xd, xp, xa, serr);
    swe_ctx_free(c);
  }
  rm_dir(dir);
  return r;
}

/* swe_calc_ut()'s JPL Moon in AD 1700 after the path,
 * swe_set_jpl_file("de200.eph") and the path again by another string, the
 * first call against the second, deg. Upstream's path setter forgets the
 * file's DE number and keeps its name: the first delta t takes the default
 * term until the calculation opens the file, 40.0057511 deg for 40.0039618
 * (UPSTREAM-BUGS 148). */
static double jpl_path_moon_off(void)
{
  double x[6], y[6];
  char serr[256], p2[AS_MAXCH];
  snprintf(p2, sizeof p2, "%s/.", EPHE);
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  swe_set_ephe_path(p2);
  if (swe_calc_ut(2341972.5, SE_MOON, SEFLG_JPLEPH, x, serr) < 0
      || swe_calc_ut(2341972.5, SE_MOON, SEFLG_JPLEPH, y, serr) < 0)
    x[0] = 1e9;
  swe_close();
  swe_set_ephe_path(EPHE);
  return fabs(swe_difdeg2n(x[0], y[0]));
}

/* swe_calc_ut()'s JPL Moon in AD 1700 after a path without de200.eph (a
 * directory of links), swe_set_jpl_file("de200.eph"), which fails there,
 * and the path to EPHE, the first call against the second, deg. No DE
 * number was read, so the path setter read none along the new path: the
 * first delta t takes the default term until the calculation opens the
 * file, 40.0057511 deg for 40.0039618 (UPSTREAM-BUGS 164, upstream's as
 * 148's). */
static double jpl_named_moon_off(void)
{
  char dir[] = "/tmp/compatnamXXXXXX", ephe[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  char serr[256];
  double x[6], y[6], r;
  struct dirent *e;
  DIR *d;
  if (realpath(EPHE, ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.' && strcmp(e->d_name, "de200.eph") != 0) {
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
      if (symlink(p, q) != 0)
        break;
    }
  closedir(d);
  swe_close();
  swe_set_ephe_path(dir);
  swe_set_jpl_file("de200.eph");
  swe_set_ephe_path(EPHE);
  if (swe_calc_ut(2341972.5, SE_MOON, SEFLG_JPLEPH, x, serr) < 0
      || swe_calc_ut(2341972.5, SE_MOON, SEFLG_JPLEPH, y, serr) < 0)
    x[0] = 1e9;
  r = fabs(swe_difdeg2n(x[0], y[0]));
  swe_close();
  swe_set_ephe_path(EPHE);
  if ((d = opendir(dir)) != NULL) {
    while ((e = readdir(d)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
        unlink(q);
      }
    closedir(d);
  }
  rmdir(dir);
  return r;
}

/* swe_deltat_ex() in AD 1000 with no ephemeris bit against SEFLG_SWIEPH,
 * the default ephemeris, after the path and swe_set_jpl_file("de200.eph")
 * on this thread, s. Upstream's 0 takes the JPL file this context has
 * open: de200's 1305.30 s for 1474.72, 169.4 (UPSTREAM-BUGS 145). */
static double dt_flag0_off(void)
{
  double d0, ds;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  d0 = swe_deltat_ex(2086308.0, 0, serr);
  ds = swe_deltat_ex(2086308.0, SEFLG_SWIEPH, serr);
  swe_close();
  swe_set_ephe_path(EPHE);
  return fabs(d0 - ds) * 86400;
}

/* swe_set_topo(), swe_set_sid_mode(SE_SIDM_LAHIRI), a Swiss Sun and
 * swe_close(), then the topocentric Moon at J2000 by swe_calc_ut() and the
 * ayanamsa there by swe_get_ayanamsa_ex(), against a fresh context given
 * the two: the larger difference, deg, 360 for a refused Moon. The manual
 * has both survive swe_close(); upstream's resets both, the Moon refused
 * and the ayanamsa Fagan/Bradley's, 0.883 deg off (UPSTREAM-BUGS 127).
 * Called last: in the shipped build the two outlive the close. */
static double close_keeps_off(void)
{
  double x[6], moon[6], aya = 0, aya_ref = 0, r;
  char serr[256];
  int32 rf;
  swe_ctx *c;
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_set_topo(8.55, 47.37, 400);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_calc(2451600.0, SE_SUN, SEFLG_SWIEPH, x, serr);
  swe_close();
  rf = swe_calc_ut(2451545.0, SE_MOON, SEFLG_SWIEPH | SEFLG_TOPOCTR, moon, serr);
  swe_get_ayanamsa_ex(2451545.0, SEFLG_SWIEPH, &aya, serr);
  if ((c = swe_ctx_new()) == NULL)
    return 360;
  swe_set_ephe_path_r(c, EPHE);
  swe_set_topo_r(c, 8.55, 47.37, 400);
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  if (swe_calc_ut_r(c, 2451545.0, SE_MOON, SEFLG_SWIEPH | SEFLG_TOPOCTR, x, serr) < 0)
    rf = -1;
  swe_get_ayanamsa_ex_r(c, 2451545.0, SEFLG_SWIEPH, &aya_ref, serr);
  swe_ctx_free(c);
  r = rf < 0 ? 360 : fabs(moon[0] - x[0]);
  if (fabs(aya - aya_ref) > r)
    r = fabs(aya - aya_ref);
  swe_close();
  swe_set_ephe_path(EPHE);
  return r;
}

int main(int argc, char **argv)
{
  int bad = 0, contam, wiped, pheno;
  double drift, moonlt, jpltt, sidstep, hlt, obs, gm, siddt, era, spd3, spdlt, latspd, toposun, merclt, moonanc;
  double nodspd, apslat, deflr, nodflag, nodj2k, natopo, orbel, intpsf, intprough, stardd, fictneg, nutr, startd, hspd, hsid, moshr, moshs, n3spd, moonpspd, moonpull, hfixp, hresp, apsrate, sflags, ayajd0, abr2, intpfix, moonpulle, nutob, merclr, aberrate, kronos, staracc, intplt, pctrdd, waldlr, vulcj, orbellt, orbel1st, oscspd, oscj2k, moshlt, nodjd0, staraccm, pctrsd, moshrough, earthlr, moonsite, moshvb, barysid, jplstale, defldisc, toposite, oscaps, helmemo, dtfirst, eclnut, nodmosh, j2klat, ayastar, tidpath, moshgeojit, filejoin, modpath, tidget, usernak, moonnbr, moshmoonr, operinoise, refused, dtthr, nodapsgm, moshsunlt, dtpath, nodapsbias, orbelpluto, pathwarn, orbelper, orbelanom, fictstar, moshbary, modtid, mapogspd, starkept, adoptjpl, pathset, cfghold, searchhold, nutintp, moshfarlt, closetid, closekeep, holdstart, nutmodel, orbelbias, teqthr, nutjplhor, jplopen, jplmoon, jplnamed, jpldft, moonless, noj2000, moshslot, dtflag0, fbmb, nutstep, toputr, eoppath, woolmd, framestep, nodmb, pctreph, icrct0, eopjpl, pctredge, eopcut, biaslt, biasstep, biasvel, biastab, epstab, nodmsun, nodmix, nodhb, jplhor1799, precwin, nutfar, nodfbb, nodhdefl, nodmemb, owenjoin, moshmoonwin, jdatewin, nutjitter, mmtopohel, startopofb, nodutfb, nodmoonm;
  int hfix, hres;

  EPHE = (argc > 1) ? argv[1] : "../ephe";
  printf("G24: SWE_UPSTREAM_COMPAT restores what it claims -- %s\n", MODE);

  dtfirst = dt_userdef_first_off();   /* first: it needs a pristine library */
  dtpath = calc_ut_first_path_off();  /* so does this */
  pathwarn = path_warn_first_count(); /* and this */
  jplopen = jpl_open_tid_off();       /* and this: no JPL file set */
  jpldft = jpl_dft_moon_off();        /* and this */
  modtid = models_tid_off();
  nodmosh = nodaps_mosh_lat_off();

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
  deflr = defl_rough_off();
  natopo = nodaps_topo_rate_off();
  nodflag = nodaps_speed_flag_move();
  nodj2k = nodaps_j2000_off();
  intpsf = intp_speedflag_off();
  intprough = intp_rough_off();
  intpfix = intp_fixpoint_off();
  intplt = intp_lighttime_off();
  stardd = star_dist_rate_off();
  oscspd = osc_speed_off(0);
  oscj2k = osc_speed_off(1);
  operinoise = moon_oscu_peri_noise();
  nodapsgm = nodaps_gm_off();
  nodapsbias = nodaps_bias_off();
  orbelpluto = orbel_pluto_off();
  orbelper = orbel_period_off();
  orbelanom = orbel_anom_off();
  sflags = star_ret_flags_off();
  ayajd0 = ayanamsa_jd0_off();
  nodjd0 = nodaps_jd0_off();
  helmemo = hel_memo_off();
  tidpath = tid_acc_path_off();
  modpath = models_path_off();
  tidget = tid_auto_get_off();
  dtthr = dt_guess_thread_off();
  teqthr = time_equ_thread_off();
  n3spd = node3_speed_off();
  moonpspd = moon_perigee_speed_off();
  apsrate = planet_apsis_rate_off();
  aberrate = planet_aberr_rate_off();
  nutr = nut_rate_off();
  nutob = nut_obliq_rate_off();
  staracc = star_aberr_accel_off(SEFLG_SWIEPH, 2488070.1);
  staraccm = star_aberr_accel_off(SEFLG_MOSEPH, 2451545.0);
  moshrough = mosh_earth_rough_off();
  moonsite = moon_site_speed_off();
  moshvb = mosh_earth_vbias_off();
  j2klat = j2000_lat_off();
  startd = star_topo_defl_off();
  toposun = topo_sun_speed_off();
  merclt = mercury_lt_rate_off();
  moonanc = topo_moon_ancient_off();
  orbel = orbel_speed_off();
  fictneg = fict_negative_mano_off();
  hspd = house_speed_off(0);
  hsid = house_speed_off(1);
  hfix = house_fix_off(&hfixp);
  hres = house_resid_off(&hresp);
  moshr = mosh_rough();
  moshs = mosh_speed_off();
  moonpull = far_body_moon_pull_off();
  abr2 = topo_aberr_second_order_off();
  moonpulle = far_body_moon_pull_eph_off();
  merclr = mercury_lt_round_off();
  kronos = kronos_builtin_off();
  pctrdd = pctr_dist_rate_off();
  waldlr = waldemath_lt_round_off();
  vulcj = vulcan_true_jitter_off();
  orbellt = orbel_lt_flag_off();
  orbel1st = orbel_first_call_off();
  moshgeojit = mosh_geo_lt_jitter();
  moshlt = mosh_lt_off();
  pctrsd = pctr_sun_defl_off();
  earthlr = earth_helio_lt_rate_off();
  barysid = bary_sun_sid_off();
  jplstale = jpl_stale_earth_off();
  defldisc = defl_disc_rate_off();
  toposite = topo_accel_off();
  oscaps = oscu_apsis_speed_off();
  eclnut = ecl_nut_speed3_off();
  ayastar = aya_star_rate_off();
  filejoin = file_join_off();
  usernak = user_nakshatra_off();
  mapogspd = mean_apog_speed_off();
  moonnbr = moon_neighbour_off();
  moshmoonr = moshmoon_strict_ret();
  refused = refused_file_off();
  fictstar = fict_star_parse();
  pathset = path_setter_118();
  cfghold = cfg_hold_off();
  searchhold = search_hold_off();
  nutintp = nut_intp_off();
  holdstart = hold_start_off();
  nutmodel = nut_model_intp_off();
  nutjplhor = nut_jplhor_intp_off();
  nutstep = nut_step_intp_off();
  woolmd = woolard_md_cycle();
  icrct0 = lahiri_icrc_t0_off();
  framestep = jplhora_step_speed_off();
  moshsunlt = mosh_sun_lt_off();
  moshbary = mosh_bary_off();
  moshfarlt = mosh_far_lt_off();
  orbelbias = orbel_bias_off();
  fbmb = fallback_mosh_bary_ret();
  toputr = topo_ut_round_off();
  nodmb = nodaps_mosh_bary_ret();
  nodmsun = nodaps_mosh_sun_off();
  nodmix = nodaps_mixed_eph_off();
  nodhb = nodaps_helbary_off();
  nodfbb = nodaps_jplfb_bias_off();
  nodhdefl = nodaps_hel_defl_off();
  nodmemb = nodaps_mosh_emb_rate_off();
  mmtopohel = moshmoon_topohel_ret();
  startopofb = star_topo_fallen_off();
  nodutfb = nodaps_ut_fallback_off();
  pctreph = pctr_ephe_ret();
  pctredge = pctr_ctr_edge_ret();
  biastab = jplhora_table_jump();
  epstab = jplhora_eps_table_jump();
  eopcut = eop_finals_cut_off();
  starkept = star_load_kept();
  eoppath = eop_path_kept_off();
  eopjpl = eop_jpl_kept_off();
  biaslt = jplhora_bias_side_off(0);
  biasstep = jplhora_bias_side_off(1);
  biasvel = jplhora_vel_off();
  jplhor1799 = jplhor_1799_speed_off();
  precwin = prec_window_speed_off();
  nutfar = nut_far_rate_off();
  owenjoin = owen_join_speed_off();
  moshmoonwin = mosh_moon_window_off();
  jdatewin = jdate_window_off();
  nutjitter = nut_args_jitter();
  adoptjpl = adopt_jpl_off();
  closetid = close_tid_off();
  jplmoon = jpl_path_moon_off();
  jplnamed = jpl_named_moon_off();
  moonless = moonless_tid_off();
  noj2000 = noj2000_tid_off();
  moshslot = mosh_sun_slot_off();
  nodmoonm = nodaps_moon_mean_ret();
  dtflag0 = dt_flag0_off();
  closekeep = close_keeps_off();	/* last: see the function */

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

  if (WANT_DEFECT ? (deflr > 3e-6) : (deflr < 1e-7))
    printf("  deflection speed rough %.2e deg/day OK\n", deflr);
  else {
    printf("  deflection speed rough %.2e deg/day FAIL: expected %s\n",
           deflr, WANT_DEFECT ? "> 3e-6" : "< 1e-7");
    bad++;
  }

  if (WANT_DEFECT ? (natopo > 1e-5) : (natopo < 2e-7))
    printf("  nod_aps topo aberr rate %.2e deg/day OK\n", natopo);
  else {
    printf("  nod_aps topo aberr rate %.2e deg/day FAIL: expected %s\n",
           natopo, WANT_DEFECT ? "> 1e-5" : "< 2e-7");
    bad++;
  }

  if (WANT_DEFECT ? (nodflag > 1) : (nodflag < 1e-4))
    printf("  nod_aps speed flag     %.2e mas     OK\n", nodflag);
  else {
    printf("  nod_aps speed flag     %.2e mas     FAIL: expected %s\n",
           nodflag, WANT_DEFECT ? "> 1" : "< 1e-4");
    bad++;
  }

  if (WANT_DEFECT ? (nodj2k > 1) : (nodj2k < 1e-6))
    printf("  nod_aps J2000 node     %.2e arcsec  OK\n", nodj2k);
  else {
    printf("  nod_aps J2000 node     %.2e arcsec  FAIL: expected %s\n",
           nodj2k, WANT_DEFECT ? "> 1" : "< 1e-6");
    bad++;
  }

  if (WANT_DEFECT ? (intpsf > 50) : (intpsf < 1e-6))
    printf("  intp perigee vs SPEED  %.2e mas     OK\n", intpsf);
  else {
    printf("  intp perigee vs SPEED  %.2e mas     FAIL: expected %s\n",
           intpsf, WANT_DEFECT ? "> 50" : "< 1e-6");
    bad++;
  }

  if (WANT_DEFECT ? (intprough > 0.3) : (intprough < 0.01))
    printf("  intp perigee rough     %.2e mas     OK\n", intprough);
  else {
    printf("  intp perigee rough     %.2e mas     FAIL: expected %s\n",
           intprough, WANT_DEFECT ? "> 0.3" : "< 0.01");
    bad++;
  }

  if (WANT_DEFECT ? (intpfix > 500) : (intpfix < 0.01))
    printf("  intp perigee fixed pt  %.2e mas     OK\n", intpfix);
  else {
    printf("  intp perigee fixed pt  %.2e mas     FAIL: expected %s\n",
           intpfix, WANT_DEFECT ? "> 500" : "< 0.01");
    bad++;
  }

  if (WANT_DEFECT ? (intplt > 10) : (intplt < 1e-3))
    printf("  intp perigee light t.  %.2e mas     OK\n", intplt);
  else {
    printf("  intp perigee light t.  %.2e mas     FAIL: expected %s\n",
           intplt, WANT_DEFECT ? "> 10" : "< 1e-3");
    bad++;
  }

  if (WANT_DEFECT ? (startopofb > 1e-8 && startopofb < 1e9) : (startopofb == 0))
    printf("  topo star fallen site  %.2e deg OK\n", startopofb);
  else {
    printf("  topo star fallen site  %.2e deg FAIL: expected %s\n",
           startopofb, WANT_DEFECT ? "> 1e-8" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (stardd > 1e-4) : (stardd < 1e-5))
    printf("  star distance rate     %.2e AU/day OK\n", stardd);
  else {
    printf("  star distance rate     %.2e AU/day FAIL: expected %s\n",
           stardd, WANT_DEFECT ? "> 1e-4" : "< 1e-5");
    bad++;
  }

  if (WANT_DEFECT ? (oscspd > 1e-6) : (oscspd < 5e-8))
    printf("  osc. apogee speed      %.2e deg/day OK\n", oscspd);
  else {
    printf("  osc. apogee speed      %.2e deg/day FAIL: expected %s\n",
           oscspd, WANT_DEFECT ? "> 1e-6" : "< 5e-8");
    bad++;
  }

  if (WANT_DEFECT ? (oscj2k > 1e-7) : (oscj2k < 1e-8))
    printf("  true node J2000 lat sp %.2e deg/day OK\n", oscj2k);
  else {
    printf("  true node J2000 lat sp %.2e deg/day FAIL: expected %s\n",
           oscj2k, WANT_DEFECT ? "> 1e-7" : "< 1e-8");
    bad++;
  }

  if (WANT_DEFECT ? (sflags > 2.5) : (sflags < 0.5))
    printf("  star returned flags    %.0f bits off OK\n", sflags);
  else {
    printf("  star returned flags    %.0f bits off FAIL: expected %s\n",
           sflags, WANT_DEFECT ? "3" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (ayajd0 > 1e-4) : (ayajd0 < 1e-7))
    printf("  ayanamsa at JD 0       %.2e deg OK\n", ayajd0);
  else {
    printf("  ayanamsa at JD 0       %.2e deg FAIL: expected %s\n",
           ayajd0, WANT_DEFECT ? "> 1e-4" : "< 1e-7");
    bad++;
  }

  if (WANT_DEFECT ? (nodjd0 > 1e-3) : (nodjd0 < 1e-8))
    printf("  Mars osc. node at JD 0 %.2e deg OK\n", nodjd0);
  else {
    printf("  Mars osc. node at JD 0 %.2e deg FAIL: expected %s\n",
           nodjd0, WANT_DEFECT ? "> 1e-3" : "< 1e-8");
    bad++;
  }

  if (WANT_DEFECT ? (dtfirst > 10) : (dtfirst >= 0 && dtfirst < 1e-9))
    printf("  delta t set first      %.2e s OK\n", dtfirst);
  else {
    printf("  delta t set first      %.2e s FAIL: expected %s\n",
           dtfirst, WANT_DEFECT ? "> 10" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (nodmosh > 1e-6) : (nodmosh < 1e-11))
    printf("  Moshier node lat speed %.2e deg/day OK\n", nodmosh);
  else {
    printf("  Moshier node lat speed %.2e deg/day FAIL: expected %s\n",
           nodmosh, WANT_DEFECT ? "> 1e-6" : "< 1e-11");
    bad++;
  }

  if (WANT_DEFECT ? (helmemo > 0.1) : (helmemo < 1e-9))
    printf("  heliacal memo, RH 80 %%  %.2e mag OK\n", helmemo);
  else {
    printf("  heliacal memo, RH 80 %%  %.2e mag FAIL: expected %s\n",
           helmemo, WANT_DEFECT ? "> 0.1" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (tidpath > 1) : (tidpath < 1e-9))
    printf("  tidal acc. after path  %.2e s OK\n", tidpath);
  else {
    printf("  tidal acc. after path  %.2e s FAIL: expected %s\n",
           tidpath, WANT_DEFECT ? "> 1" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (modpath > 1) : (modpath < 1e-9))
    printf("  models after path      %.2e s OK\n", modpath);
  else {
    printf("  models after path      %.2e s FAIL: expected %s\n",
           modpath, WANT_DEFECT ? "> 1" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (tidget > 0.1) : (tidget < 1e-12))
    printf("  tidal acc. automatic   %.2e OK\n", tidget);
  else {
    printf("  tidal acc. automatic   %.2e FAIL: expected %s\n",
           tidget, WANT_DEFECT ? "> 0.1" : "< 1e-12");
    bad++;
  }

  if (WANT_DEFECT ? (dtthr > 1) : (dtthr >= 0 && dtthr < 1e-9))
    printf("  delta t, other thread  %.2e s OK\n", dtthr);
  else {
    printf("  delta t, other thread  %.2e s FAIL: expected %s\n",
           dtthr, WANT_DEFECT ? "> 1" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (teqthr > 1e-4) : (teqthr == 0))
    printf("  time equ, other thread %.2e s OK\n", teqthr);
  else {
    printf("  time equ, other thread %.2e s FAIL: expected %s\n",
           teqthr, WANT_DEFECT ? "> 1e-4" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (dtpath > 1e-4) : (dtpath >= 0 && dtpath < 1e-12))
    printf("  first calc_ut, path    %.2e deg OK\n", dtpath);
  else {
    printf("  first calc_ut, path    %.2e deg FAIL: expected %s\n",
           dtpath, WANT_DEFECT ? "> 1e-4" : "< 1e-12");
    bad++;
  }

  if (WANT_DEFECT ? (pathwarn == 2) : (pathwarn == 0))
    printf("  first-call path warning %.0f of 2 OK\n", pathwarn);
  else {
    printf("  first-call path warning %.0f of 2 FAIL: expected %s\n",
           pathwarn, WANT_DEFECT ? "2" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (modtid > 0.1) : (modtid >= 0 && modtid < 1e-12))
    printf("  SE1.80 tidal term      %.2e OK\n", modtid);
  else {
    printf("  SE1.80 tidal term      %.2e FAIL: expected %s\n",
           modtid, WANT_DEFECT ? "> 0.1" : "< 1e-12");
    bad++;
  }

  if (WANT_DEFECT ? (n3spd > 5e-4) : (n3spd < 1e-5))
    printf("  apogee SPEED3          %.2e deg/day OK\n", n3spd);
  else {
    printf("  apogee SPEED3          %.2e deg/day FAIL: expected %s\n",
           n3spd, WANT_DEFECT ? "> 5e-4" : "< 1e-5");
    bad++;
  }

  if (WANT_DEFECT ? (moonpspd > 1e-6) : (moonpspd < 1e-8))
    printf("  Moon perigee speed     %.2e deg/day OK\n", moonpspd);
  else {
    printf("  Moon perigee speed     %.2e deg/day FAIL: expected %s\n",
           moonpspd, WANT_DEFECT ? "> 1e-6" : "< 1e-8");
    bad++;
  }

  if (WANT_DEFECT ? (apsrate > 1e-6) : (apsrate < 1e-9))
    printf("  planet apsis rate      %.2e deg/day OK\n", apsrate);
  else {
    printf("  planet apsis rate      %.2e deg/day FAIL: expected %s\n",
           apsrate, WANT_DEFECT ? "> 1e-6" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (aberrate > 1e-9) : (aberrate < 2e-11))
    printf("  apsis aberration rate %.2e AU/day OK\n", aberrate);
  else {
    printf("  apsis aberration rate %.2e AU/day FAIL: expected %s\n",
           aberrate, WANT_DEFECT ? "> 1e-9" : "< 2e-11");
    bad++;
  }

  if (WANT_DEFECT ? (nutr > 5e-9) : (nutr < 3e-9))
    printf("  nutation rate          %.2e deg/day OK\n", nutr);
  else {
    printf("  nutation rate          %.2e deg/day FAIL: expected %s\n",
           nutr, WANT_DEFECT ? "> 5e-9" : "< 3e-9");
    bad++;
  }

  if (WANT_DEFECT ? (nutob > 1e-9) : (nutob < 1e-10))
    printf("  nutation's obliquity   %.2e deg/day OK\n", nutob);
  else {
    printf("  nutation's obliquity   %.2e deg/day FAIL: expected %s\n",
           nutob, WANT_DEFECT ? "> 1e-9" : "< 1e-10");
    bad++;
  }

  if (WANT_DEFECT ? (staracc > 5e-10) : (staracc < 1e-10))
    printf("  star aberration accel. %.2e deg/day OK\n", staracc);
  else {
    printf("  star aberration accel. %.2e deg/day FAIL: expected %s\n",
           staracc, WANT_DEFECT ? "> 5e-10" : "< 1e-10");
    bad++;
  }

  if (WANT_DEFECT ? (staraccm > 5e-10) : (staraccm < 1e-10))
    printf("  star aberr. Moshier    %.2e deg/day OK\n", staraccm);
  else {
    printf("  star aberr. Moshier    %.2e deg/day FAIL: expected %s\n",
           staraccm, WANT_DEFECT ? "> 5e-10" : "< 1e-10");
    bad++;
  }

  if (WANT_DEFECT ? (moshrough > 1e-6) : (moshrough < 1e-7))
    printf("  Moshier Earth rough    %.2e of a     OK\n", moshrough);
  else {
    printf("  Moshier Earth rough    %.2e of a     FAIL: expected %s\n",
           moshrough, WANT_DEFECT ? "> 1e-6" : "< 1e-7");
    bad++;
  }

  if (WANT_DEFECT ? (moonsite > 1e-6) : (moonsite < 3e-8))
    printf("  Moon site/file speed   %.2e deg/day OK\n", moonsite);
  else {
    printf("  Moon site/file speed   %.2e deg/day FAIL: expected %s\n",
           moonsite, WANT_DEFECT ? "> 1e-6" : "< 3e-8");
    bad++;
  }

  if (WANT_DEFECT ? (moshvb > 1e-10) : (moshvb < 1e-13))
    printf("  Moshier Earth v bias   %.2e AU/day OK\n", moshvb);
  else {
    printf("  Moshier Earth v bias   %.2e AU/day FAIL: expected %s\n",
           moshvb, WANT_DEFECT ? "> 1e-10" : "< 1e-13");
    bad++;
  }

  if (WANT_DEFECT ? (j2klat > 1e-10) : (j2klat < 1e-12))
    printf("  Sun latitude at J2000  %.2e deg OK\n", j2klat);
  else {
    printf("  Sun latitude at J2000  %.2e deg FAIL: expected %s\n",
           j2klat, WANT_DEFECT ? "> 1e-10" : "< 1e-12");
    bad++;
  }

  if (WANT_DEFECT ? (startd < 1e-8) : (startd > 1e-6))
    printf("  star topocentric defl. %.2e arcsec  OK\n", startd);
  else {
    printf("  star topocentric defl. %.2e arcsec  FAIL: expected %s\n",
           startd, WANT_DEFECT ? "< 1e-8" : "> 1e-6");
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

  if (WANT_DEFECT ? (orbel > 1e-3) : (orbel >= 0 && orbel < 1e-7))
    printf("  orbital elements' rate %.2e deg/day OK\n", orbel);
  else {
    printf("  orbital elements' rate %.2e deg/day FAIL: expected %s\n",
           orbel, WANT_DEFECT ? "> 1e-3" : "< 1e-7");
    bad++;
  }

  if (WANT_DEFECT ? (fictneg > 1) : (fictneg >= 0 && fictneg < 1e-9))
    printf("  negative mean anomaly  %.2e deg OK\n", fictneg);
  else {
    printf("  negative mean anomaly  %.2e deg FAIL: expected %s\n",
           fictneg, WANT_DEFECT ? "> 1" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (hspd > 1) : (hspd < 1e-5))
    printf("  house cusp speeds      %.2e deg/day OK\n", hspd);
  else {
    printf("  house cusp speeds      %.2e deg/day FAIL: expected %s\n",
           hspd, WANT_DEFECT ? "> 1" : "< 1e-5");
    bad++;
  }

  if (WANT_DEFECT ? (hsid > 2e-5) : (hsid < 1e-6))
    printf("  sidereal house speeds  %.2e deg/day OK\n", hsid);
  else {
    printf("  sidereal house speeds  %.2e deg/day FAIL: expected %s\n",
           hsid, WANT_DEFECT ? "> 2e-5" : "< 1e-6");
    bad++;
  }

  if (WANT_DEFECT ? (hfix == 4) : (hfix == 0))
    printf("  house defects (59)     %d/4, Placidus %.2e deg/day OK\n", hfix, hfixp);
  else {
    printf("  house defects (59)     %d/4, Placidus %.2e deg/day FAIL: expected %s\n",
           hfix, hfixp, WANT_DEFECT ? "4/4" : "0/4");
    bad++;
  }

  if (WANT_DEFECT ? (hres == 3) : (hres == 0))
    printf("  house residue (68)     %d/3, equinox cusp %.2e deg/day OK\n", hres, hresp);
  else {
    printf("  house residue (68)     %d/3, equinox cusp %.2e deg/day FAIL: expected %s\n",
           hres, hresp, WANT_DEFECT ? "3/3" : "0/3");
    bad++;
  }

  if (WANT_DEFECT ? (moshr > 3e-4) : (moshr < 1e-5))
    printf("  Moshier series rough   %.2e mas OK\n", moshr);
  else {
    printf("  Moshier series rough   %.2e mas FAIL: expected %s\n",
           moshr, WANT_DEFECT ? "> 3e-4" : "< 1e-5");
    bad++;
  }

  if (WANT_DEFECT ? (moshs > 1e-6) : (moshs < 1e-8))
    printf("  Moshier speed          %.2e deg/day OK\n", moshs);
  else {
    printf("  Moshier speed          %.2e deg/day FAIL: expected %s\n",
           moshs, WANT_DEFECT ? "> 1e-6" : "< 1e-8");
    bad++;
  }

  if (WANT_DEFECT ? (moonpull > 1e-5) : (moonpull < 2e-8))
    printf("  far body, Moon's pull  %.2e deg/day OK\n", moonpull);
  else {
    printf("  far body, Moon's pull  %.2e deg/day FAIL: expected %s\n",
           moonpull, WANT_DEFECT ? "> 1e-5" : "< 2e-8");
    bad++;
  }

  if (WANT_DEFECT ? (abr2 > 1e-6) : (abr2 < 8e-9))
    printf("  aberr. rate 2nd order  %.2e deg/day OK\n", abr2);
  else {
    printf("  aberr. rate 2nd order  %.2e deg/day FAIL: expected %s\n",
           abr2, WANT_DEFECT ? "> 1e-6" : "< 8e-9");
    bad++;
  }

  if (WANT_DEFECT ? (moonpulle > 1e-5) : (moonpulle < 2e-8))
    printf("  far body, Moon, JPL/Mosh %.2e deg/day OK\n", moonpulle);
  else {
    printf("  far body, Moon, JPL/Mosh %.2e deg/day FAIL: expected %s\n",
           moonpulle, WANT_DEFECT ? "> 1e-5" : "< 2e-8");
    bad++;
  }

  if (WANT_DEFECT ? (merclr > 1e-5) : (merclr < 5e-9))
    printf("  light time, rounding   %.2e deg/day OK\n", merclr);
  else {
    printf("  light time, rounding   %.2e deg/day FAIL: expected %s\n",
           merclr, WANT_DEFECT ? "> 1e-5" : "< 5e-9");
    bad++;
  }

  if (WANT_DEFECT ? (kronos > 1e-3) : (kronos >= 0 && kronos < 1e-9))
    printf("  Kronos, built-in table   %.2e deg OK\n", kronos);
  else {
    printf("  Kronos, built-in table   %.2e deg FAIL: expected %s\n",
           kronos, WANT_DEFECT ? "> 1e-3" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (pctrdd > 1e-7) : (pctrdd < 5e-10))
    printf("  pctr distance rate     %.2e AU/day  OK\n", pctrdd);
  else {
    printf("  pctr distance rate     %.2e AU/day  FAIL: expected %s\n",
           pctrdd, WANT_DEFECT ? "> 1e-7" : "< 5e-10");
    bad++;
  }

  if (WANT_DEFECT ? (waldlr > 1e-5) : (waldlr < 5e-8))
    printf("  fict. light time, round %.2e deg/day OK\n", waldlr);
  else {
    printf("  fict. light time, round %.2e deg/day FAIL: expected %s\n",
           waldlr, WANT_DEFECT ? "> 1e-5" : "< 5e-8");
    bad++;
  }

  if (WANT_DEFECT ? (vulcj > 1e-9) : (vulcj < 2e-11))
    printf("  fict. element rounding %.2e deg     OK\n", vulcj);
  else {
    printf("  fict. element rounding %.2e deg     FAIL: expected %s\n",
           vulcj, WANT_DEFECT ? "> 1e-9" : "< 2e-11");
    bad++;
  }

  if (WANT_DEFECT ? (orbellt > 1e-8) : (orbellt == 0))
    printf("  orbel, light time flag  %.2e AU      OK\n", orbellt);
  else {
    printf("  orbel, light time flag  %.2e AU      FAIL: expected %s\n",
           orbellt, WANT_DEFECT ? "> 1e-8" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (orbel1st > 1) : (orbel1st == 0))
    printf("  orbel, first call       %.2e deg     OK\n", orbel1st);
  else {
    printf("  orbel, first call       %.2e deg     FAIL: expected %s\n",
           orbel1st, WANT_DEFECT ? "> 1" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (moshlt > 1e-7) : (moshlt < 1e-9))
    printf("  Moshier light time     %.2e AU/day OK\n", moshlt);
  else {
    printf("  Moshier light time     %.2e AU/day FAIL: expected %s\n",
           moshlt, WANT_DEFECT ? "> 1e-7" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (pctrsd > 0.01) : (pctrsd == 0))
    printf("  pctr Sun deflected     %.2e arcsec  OK\n", pctrsd);
  else {
    printf("  pctr Sun deflected     %.2e arcsec  FAIL: expected %s\n",
           pctrsd, WANT_DEFECT ? "> 0.01" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (earthlr > 1e-6) : (earthlr < 5e-9))
    printf("  Earth helio, lt rate   %.2e deg/day OK\n", earthlr);
  else {
    printf("  Earth helio, lt rate   %.2e deg/day FAIL: expected %s\n",
           earthlr, WANT_DEFECT ? "> 1e-6" : "< 5e-9");
    bad++;
  }

  if (WANT_DEFECT ? (barysid > 0.01) : (barysid < 1e-9))
    printf("  JPL bary Sun sidereal  %.2e deg OK\n", barysid);
  else {
    printf("  JPL bary Sun sidereal  %.2e deg FAIL: expected %s\n",
           barysid, WANT_DEFECT ? "> 0.01" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (jplstale > 1) : (jplstale >= 0 && jplstale < 1e-12))
    printf("  JPL Moon after Moshier %.2e deg OK\n", jplstale);
  else {
    printf("  JPL Moon after Moshier %.2e deg FAIL: expected %s\n",
           jplstale, WANT_DEFECT ? "> 1" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (defldisc > 1e-6) : (defldisc < 2e-8))
    printf("  deflection rate in disc %.2e deg/day OK\n", defldisc);
  else {
    printf("  deflection rate in disc %.2e deg/day FAIL: expected %s\n",
           defldisc, WANT_DEFECT ? "> 1e-6" : "< 2e-8");
    bad++;
  }
  if (WANT_DEFECT ? (mapogspd > 5e-9) : (mapogspd < 1e-9))
    printf("  mean apogee speed       %.2e deg/day OK\n", mapogspd);
  else {
    printf("  mean apogee speed       %.2e deg/day FAIL: expected %s\n",
           mapogspd, WANT_DEFECT ? "> 5e-9" : "< 1e-9");
    bad++;
  }
  if (WANT_DEFECT ? (usernak > 3) : (usernak < 1e-9))
    printf("  USER nakshatra shift    %.2e deg     OK\n", usernak);
  else {
    printf("  USER nakshatra shift    %.2e deg     FAIL: expected %s\n",
           usernak, WANT_DEFECT ? "> 3" : "0");
    bad++;
  }
  if (WANT_DEFECT ? (ayastar > 1e-7) : (ayastar < 2e-10))
    printf("  ayanamsa rate, star     %.2e deg/day OK\n", ayastar);
  else {
    printf("  ayanamsa rate, star     %.2e deg/day FAIL: expected %s\n",
           ayastar, WANT_DEFECT ? "> 1e-7" : "< 2e-10");
    bad++;
  }

  if (WANT_DEFECT ? (toposite > 1e-7) : (toposite < 5e-11))
    printf("  topocentric site accel.  %.2e deg/day OK\n", toposite);
  else {
    printf("  topocentric site accel.  %.2e deg/day FAIL: expected %s\n",
           toposite, WANT_DEFECT ? "> 1e-7" : "< 5e-11");
    bad++;
  }

  if (WANT_DEFECT ? (oscaps > 1e-7) : (oscaps < 2e-9))
    printf("  osculating apsis speed   %.2e deg/day OK\n", oscaps);
  else {
    printf("  osculating apsis speed   %.2e deg/day FAIL: expected %s\n",
           oscaps, WANT_DEFECT ? "> 1e-7" : "< 2e-9");
    bad++;
  }

  if (WANT_DEFECT ? (eclnut > 1e-4) : (eclnut == 0))
    printf("  ecl_nut with SPEED3    %.2e deg OK\n", eclnut);
  else {
    printf("  ecl_nut with SPEED3    %.2e deg FAIL: expected %s\n",
           eclnut, WANT_DEFECT ? "> 1e-4" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (moshgeojit > 3e-14) : (moshgeojit < 6e-15))
    printf("  Moshier geo. fict. jitter %.2e AU OK\n", moshgeojit);
  else {
    printf("  Moshier geo. fict. jitter %.2e AU FAIL: expected %s\n",
           moshgeojit, WANT_DEFECT ? "> 3e-14" : "< 6e-15");
    bad++;
  }

  if (WANT_DEFECT ? (filejoin > 1) : (filejoin == 0))
    printf("  Sun past a file join   %.2e mas OK\n", filejoin);
  else {
    printf("  Sun past a file join   %.2e mas FAIL: expected %s\n",
           filejoin, WANT_DEFECT ? "> 1" : "0");
    bad++;
  }

  /* the same both ways: the code it fixes is this fork's (see the function) */
  if (moonnbr == 0)
    printf("  Moon, neighbour refused %.2e deg OK\n", moonnbr);
  else {
    printf("  Moon, neighbour refused %.2e deg FAIL: expected 0\n", moonnbr);
    bad++;
  }

  if (WANT_DEFECT ? (moshmoonr == (SEFLG_SWIEPH | SEFLG_SPEED)) : (moshmoonr < 0))
    printf("  Moon, semo missing, strict %.0f OK\n", moshmoonr);
  else {
    printf("  Moon, semo missing, strict %.0f FAIL: expected %s\n",
           moshmoonr, WANT_DEFECT ? "258" : "< 0");
    bad++;
  }

  if (WANT_DEFECT ? (mmtopohel == (SEFLG_SWIEPH | SEFLG_TOPOCTR)) : (mmtopohel == -1))
    printf("  TOPOCTR|HELCTR, semo missing, strict %.0f OK\n", mmtopohel);
  else {
    printf("  TOPOCTR|HELCTR, semo missing, strict %.0f FAIL: expected %s\n",
           mmtopohel, WANT_DEFECT ? "32770" : "-1");
    bad++;
  }

  if (WANT_DEFECT ? (operinoise > 1e-6) : (operinoise < 5e-8))
    printf("  Moshier oscu perigee speed noise %.2e deg/day OK\n", operinoise);
  else {
    printf("  Moshier oscu perigee speed noise %.2e deg/day FAIL: expected %s\n",
           operinoise, WANT_DEFECT ? "> 1e-6" : "< 5e-8");
    bad++;
  }

  /* fork-only, so clean both ways */
  if (pathset == 0)
    printf("  path setter, 118       %.0f of 2 OK\n", pathset);
  else {
    printf("  path setter, 118       %.0f of 2 FAIL: expected 0 in both builds\n", pathset);
    bad++;
  }

  if (WANT_DEFECT ? (fictstar == 63) : (fictstar == 0))
    printf("  malformed orbel/star lines %2.0f of 63 OK\n", fictstar);
  else {
    printf("  malformed orbel/star lines %2.0f of 63 FAIL: expected %s\n",
           fictstar, WANT_DEFECT ? "63" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (refused > 1) : (refused == 0))
    printf("  after a refused file   %.2e deg OK\n", refused);
  else {
    printf("  after a refused file   %.2e deg FAIL: expected %s\n",
           refused, WANT_DEFECT ? "> 1" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nodapsgm > 1e-11) : (nodapsgm < 1e-13))
    printf("  nod_aps Venus GM q     %.2e AU OK\n", nodapsgm);
  else {
    printf("  nod_aps Venus GM q     %.2e AU FAIL: expected %s\n",
           nodapsgm, WANT_DEFECT ? "> 1e-11" : "< 1e-13");
    bad++;
  }

  if (WANT_DEFECT ? (moshsunlt > 1e-6) : (moshsunlt < 1e-9))
    printf("  Moshier helio lt, Sun  %.2e deg OK\n", moshsunlt);
  else {
    printf("  Moshier helio lt, Sun  %.2e deg FAIL: expected %s\n",
           moshsunlt, WANT_DEFECT ? "> 1e-6" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (nodapsbias > 1e-6) : (nodapsbias < 1e-9))
    printf("  nod_aps Moon apog bias %.2e deg OK\n", nodapsbias);
  else {
    printf("  nod_aps Moon apog bias %.2e deg FAIL: expected %s\n",
           nodapsbias, WANT_DEFECT ? "> 1e-6" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (orbelpluto > 1e-8) : (orbelpluto < 1e-12))
    printf("  orbel Pluto GM a       %.2e OK\n", orbelpluto);
  else {
    printf("  orbel Pluto GM a       %.2e FAIL: expected %s\n",
           orbelpluto, WANT_DEFECT ? "> 1e-8" : "< 1e-12");
    bad++;
  }

  if (WANT_DEFECT ? (orbelper > 1e-4) : (orbelper < 1e-12))
    printf("  orbel Jupiter period   %.2e OK\n", orbelper);
  else {
    printf("  orbel Jupiter period   %.2e FAIL: expected %s\n",
           orbelper, WANT_DEFECT ? "> 1e-4" : "< 1e-12");
    bad++;
  }

  if (WANT_DEFECT ? (orbelanom > 1e-9) : (orbelanom < 1e-11))
    printf("  orbel Venus anomaly    %.2e deg OK\n", orbelanom);
  else {
    printf("  orbel Venus anomaly    %.2e deg FAIL: expected %s\n",
           orbelanom, WANT_DEFECT ? "> 1e-9" : "< 1e-11");
    bad++;
  }

  if (WANT_DEFECT ? (moshbary > 1e-7) : (moshbary < 2e-9))
    printf("  Moshier star, fict     %.2e deg OK\n", moshbary);
  else {
    printf("  Moshier star, fict     %.2e deg FAIL: expected %s\n",
           moshbary, WANT_DEFECT ? "> 1e-7" : "< 2e-9");
    bad++;
  }

  if (WANT_DEFECT ? (moshfarlt > 1e-7) : (moshfarlt < 1e-9))
    printf("  Moshier VEA light time %.2e deg OK\n", moshfarlt);
  else {
    printf("  Moshier VEA light time %.2e deg FAIL: expected %s\n",
           moshfarlt, WANT_DEFECT ? "> 1e-7" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (orbelbias > 1e-8) : (orbelbias < 1e-14))
    printf("  orbel frame bias       %.2e AU OK\n", orbelbias);
  else {
    printf("  orbel frame bias       %.2e AU FAIL: expected %s\n",
           orbelbias, WANT_DEFECT ? "> 1e-8" : "< 1e-14");
    bad++;
  }

  if (WANT_DEFECT ? (fbmb == 18180) : (fbmb < 0))
    printf("  fallback to Moshier, barycentric %.0f OK\n", fbmb);
  else {
    printf("  fallback to Moshier, barycentric %.0f FAIL: expected %s\n",
           fbmb, WANT_DEFECT ? "18180" : "< 0");
    bad++;
  }

  if (WANT_DEFECT ? (pctreph == 257) : (pctreph == 258))
    printf("  pctr ephemeris returned %.0f OK\n", pctreph);
  else {
    printf("  pctr ephemeris returned %.0f FAIL: expected %s\n",
           pctreph, WANT_DEFECT ? "257" : "258");
    bad++;
  }

  if (eopcut >= 0 && (WANT_DEFECT ? (eopcut > 1e-2) : (eopcut == 0)))
    printf("  IERS kept past a broken eop_finals.txt %.2e arcsec OK\n", eopcut);
  else {
    printf("  IERS kept past a broken eop_finals.txt %.2e arcsec FAIL: expected %s\n",
           eopcut, WANT_DEFECT ? "> 1e-2" : "0");
    bad++;
  }

  if (epstab >= 0 && (WANT_DEFECT ? (epstab > 1e-7) : (epstab < 1e-11)))
    printf("  Horizons obliquity across a table year %.2e deg OK\n", epstab);
  else {
    printf("  Horizons obliquity across a table year %.2e deg FAIL: expected %s\n",
           epstab, WANT_DEFECT ? "> 1e-7" : "< 1e-11");
    bad++;
  }

  if (biastab >= 0 && (WANT_DEFECT ? (biastab > 1e-7) : (biastab < 1e-9)))
    printf("  Horizons offset across a table year %.2e deg OK\n", biastab);
  else {
    printf("  Horizons offset across a table year %.2e deg FAIL: expected %s\n",
           biastab, WANT_DEFECT ? "> 1e-7" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (pctredge == 258) : (pctredge == ERR))
    printf("  pctr centre's files begun after t %.0f OK\n", pctredge);
  else {
    printf("  pctr centre's files begun after t %.0f FAIL: expected %s\n",
           pctredge, WANT_DEFECT ? "258" : "-1");
    bad++;
  }

  if (WANT_DEFECT ? (nodmb == 0) : (nodmb < 0))
    printf("  nod_aps Moshier barycentre %.0f OK\n", nodmb);
  else {
    printf("  nod_aps Moshier barycentre %.0f FAIL: expected %s\n",
           nodmb, WANT_DEFECT ? "0" : "< 0");
    bad++;
  }

  if (WANT_DEFECT ? (nodmsun > 0.5 && nodmsun < 1e9) : (nodmsun < 1e-12))
    printf("  nod_aps Moshier Sun    %.2e AU OK\n", nodmsun);
  else {
    printf("  nod_aps Moshier Sun    %.2e AU FAIL: expected %s\n",
           nodmsun, WANT_DEFECT ? "> 0.5" : "< 1e-12");
    bad++;
  }

  if (WANT_DEFECT ? (nodmix > 0.05 && nodmix < 1e9) : (nodmix == 0))
    printf("  nod_aps SWIEPH|MOSEPH  %.2e deg OK\n", nodmix);
  else {
    printf("  nod_aps SWIEPH|MOSEPH  %.2e deg FAIL: expected %s\n",
           nodmix, WANT_DEFECT ? "> 0.05" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nodhb > 0.05 && nodhb < 1e9) : (nodhb == 0))
    printf("  nod_aps HELCTR|BARYCTR %.2e deg OK\n", nodhb);
  else {
    printf("  nod_aps HELCTR|BARYCTR %.2e deg FAIL: expected %s\n",
           nodhb, WANT_DEFECT ? "> 0.05" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nodfbb > 1e-4 && nodfbb < 1e9) : (nodfbb == 0))
    printf("  nod_aps de200 fallen   %.2e deg OK\n", nodfbb);
  else {
    printf("  nod_aps de200 fallen   %.2e deg FAIL: expected %s\n",
           nodfbb, WANT_DEFECT ? "> 1e-4" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nodhdefl > 5e-4 && nodhdefl < 1e9) : (nodhdefl == 0))
    printf("  nod_aps hel deflection %.2e deg OK\n", nodhdefl);
  else {
    printf("  nod_aps hel deflection %.2e deg FAIL: expected %s\n",
           nodhdefl, WANT_DEFECT ? "> 5e-4" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nodmemb > 1e-8 && nodmemb < 1e9) : (nodmemb < 1e-9))
    printf("  nod_aps Moshier EMB    %.2e deg/day OK\n", nodmemb);
  else {
    printf("  nod_aps Moshier EMB    %.2e deg/day FAIL: expected %s\n",
           nodmemb, WANT_DEFECT ? "> 1e-8" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (nodutfb > 1e-7 && nodutfb < 1e9) : (nodutfb == 0))
    printf("  nod_aps_ut fallback dt %.2e deg OK\n", nodutfb);
  else {
    printf("  nod_aps_ut fallback dt %.2e deg FAIL: expected %s\n",
           nodutfb, WANT_DEFECT ? "> 1e-7" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (toputr > 0.9) : (toputr >= 0 && toputr < 0.05))
    printf("  site at UT1's rounding %.2e OK\n", toputr);
  else {
    printf("  site at UT1's rounding %.2e FAIL: expected %s\n",
           toputr, WANT_DEFECT ? "> 0.9" : "< 0.05");
    bad++;
  }

  if (WANT_DEFECT ? (starkept == 1) : (starkept == 0))
    printf("  failed star load kept  %.0f OK\n", starkept);
  else {
    printf("  failed star load kept  %.0f FAIL: expected %s\n",
           starkept, WANT_DEFECT ? "1 (reread)" : "0 (kept)");
    bad++;
  }

  /* the same both ways: the shortcut it fixes is this fork's (see the function) */
  if (adoptjpl == 0)
    printf("  adopted JPL file       %.2e deg OK\n", adoptjpl);
  else {
    printf("  adopted JPL file       %.2e deg FAIL: expected 0\n", adoptjpl);
    bad++;
  }

  if (WANT_DEFECT ? (cfghold > 1e-3) : (cfghold == 0))
    printf("  setter mid-calculation %.2e OK\n", cfghold);
  else {
    printf("  setter mid-calculation %.2e FAIL: expected %s\n",
           cfghold, WANT_DEFECT ? "> 1e-3" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (searchhold > 100) : (searchhold == 0))
    printf("  setter mid-search      %.2e s OK\n", searchhold);
  else {
    printf("  setter mid-search      %.2e s FAIL: expected %s\n",
           searchhold, WANT_DEFECT ? "> 100" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nutintp > 5e-5) : (nutintp == 0))
    printf("  interpolation unset    %.2e arcsec OK\n", nutintp);
  else {
    printf("  interpolation unset    %.2e arcsec FAIL: expected %s\n",
           nutintp, WANT_DEFECT ? "> 5e-5" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (closetid > 10) : (closetid == 0))
    printf("  delta t after close    %.2e s OK\n", closetid);
  else {
    printf("  delta t after close    %.2e s FAIL: expected %s\n",
           closetid, WANT_DEFECT ? "> 10" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (jplopen > 1) : (jplopen == 0))
    printf("  term of a JPL opened   %.2e OK\n", jplopen);
  else {
    printf("  term of a JPL opened   %.2e FAIL: expected %s\n",
           jplopen, WANT_DEFECT ? "> 1" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (jplmoon > 1e-3) : (jplmoon == 0))
    printf("  JPL Moon, path again   %.2e deg OK\n", jplmoon);
  else {
    printf("  JPL Moon, path again   %.2e deg FAIL: expected %s\n",
           jplmoon, WANT_DEFECT ? "> 1e-3" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (jplnamed > 1e-3) : (jplnamed == 0))
    printf("  JPL Moon, named late   %.2e deg OK\n", jplnamed);
  else {
    printf("  JPL Moon, named late   %.2e deg FAIL: expected %s\n",
           jplnamed, WANT_DEFECT ? "> 1e-3" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (jpldft > 1e-3 && jpldft < 0.5) : (jpldft == 0))
    printf("  JPL Moon, default file %.2e deg OK\n", jpldft);
  else {
    printf("  JPL Moon, default file %.2e deg FAIL: expected %s\n",
           jpldft, WANT_DEFECT ? "> 1e-3, < 0.5" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (moonless > 10) : (moonless == 0))
    printf("  delta t, no moon file  %.2e s OK\n", moonless);
  else {
    printf("  delta t, no moon file  %.2e s FAIL: expected %s\n",
           moonless, WANT_DEFECT ? "> 10" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (noj2000 > 2) : (noj2000 == 0))
    printf("  delta t, no J2000 file %.2e s OK\n", noj2000);
  else {
    printf("  delta t, no J2000 file %.2e s FAIL: expected %s\n",
           noj2000, WANT_DEFECT ? "> 2" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (moshslot > 0.1 && moshslot < 1) : (moshslot == 0))
    printf("  Moshier Sun slot       %.2e deg OK\n", moshslot);
  else {
    printf("  Moshier Sun slot       %.2e deg FAIL: expected %s\n",
           moshslot, WANT_DEFECT ? "> 0.1, < 1" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nodmoonm == -1) : (nodmoonm == 0))
    printf("  nod_aps Moon mean, no files %.0f OK\n", nodmoonm);
  else {
    printf("  nod_aps Moon mean, no files %.0f FAIL: expected %s\n",
           nodmoonm, WANT_DEFECT ? "-1" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (dtflag0 > 100) : (dtflag0 == 0))
    printf("  delta t, flag 0        %.2e s OK\n", dtflag0);
  else {
    printf("  delta t, flag 0        %.2e s FAIL: expected %s\n",
           dtflag0, WANT_DEFECT ? "> 100" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (closekeep > 0.5) : (closekeep == 0))
    printf("  topo, sid after close  %.2e deg OK\n", closekeep);
  else {
    printf("  topo, sid after close  %.2e deg FAIL: expected %s\n",
           closekeep, WANT_DEFECT ? "> 0.5" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (holdstart > 1) : (holdstart == 0))
    printf("  thread's first setting %.2e s OK\n", holdstart);
  else {
    printf("  thread's first setting %.2e s FAIL: expected %s\n",
           holdstart, WANT_DEFECT ? "> 1" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nutmodel > 1e-3) : (nutmodel == 0))
    printf("  nutation model kept    %.2e arcsec OK\n", nutmodel);
  else {
    printf("  nutation model kept    %.2e arcsec FAIL: expected %s\n",
           nutmodel, WANT_DEFECT ? "> 1e-3" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nutjplhor > 1e-2) : (nutjplhor == 0))
    printf("  nutation JPLHOR kept   %.2e arcsec OK\n", nutjplhor);
  else {
    printf("  nutation JPLHOR kept   %.2e arcsec FAIL: expected %s\n",
           nutjplhor, WANT_DEFECT ? "> 1e-2" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (nutstep > 1e-2) : (nutstep == 0))
    printf("  nutation across a step %.2e arcsec OK\n", nutstep);
  else {
    printf("  nutation across a step %.2e arcsec FAIL: expected %s\n",
           nutstep, WANT_DEFECT ? "> 1e-2" : "0");
    bad++;
  }

  if (eoppath >= 0 && (WANT_DEFECT ? (eoppath > 0.1) : (eoppath == 0)))
    printf("  IERS corrections kept  %.2e arcsec OK\n", eoppath);
  else {
    printf("  IERS corrections kept  %.2e arcsec FAIL: expected %s\n",
           eoppath, WANT_DEFECT ? "> 0.1" : "0");
    bad++;
  }

  if (WANT_DEFECT ? (fabs(icrct0) < 1e-6) : (icrct0 < -0.08 && icrct0 > -0.081))
    printf("  Lahiri ICRC epoch value %.4f arcsec OK\n", icrct0);
  else {
    printf("  Lahiri ICRC epoch value %.4f arcsec FAIL: expected %s\n",
           icrct0, WANT_DEFECT ? "0" : "-0.0806");
    bad++;
  }

  if (WANT_DEFECT ? (woolmd > 1e-2) : (woolmd < 1e-6))
    printf("  Woolard minute cycle   %.2e arcsec OK\n", woolmd);
  else {
    printf("  Woolard minute cycle   %.2e arcsec FAIL: expected %s\n",
           woolmd, WANT_DEFECT ? "> 1e-2" : "< 1e-6");
    bad++;
  }

  if (WANT_DEFECT ? (framestep > 1e-3) : (framestep >= 0 && framestep < 1e-8))
    printf("  speed across a step    %.2e deg/day OK\n", framestep);
  else {
    printf("  speed across a step    %.2e deg/day FAIL: expected %s\n",
           framestep, WANT_DEFECT ? "> 1e-3" : "< 1e-8");
    bad++;
  }

  if (eopjpl >= 0 && (WANT_DEFECT ? (eopjpl > 1e-2) : (eopjpl == 0)))
    printf("  IERS kept by de200.eph %.2e arcsec OK\n", eopjpl);
  else {
    printf("  IERS kept by de200.eph %.2e arcsec FAIL: expected %s\n",
           eopjpl, WANT_DEFECT ? "> 1e-2" : "0");
    bad++;
  }

  if (biaslt >= 0 && (WANT_DEFECT ? (biaslt > 1e-6) : (biaslt < 1e-9)))
    printf("  bias in light time     %.2e deg OK\n", biaslt);
  else {
    printf("  bias in light time     %.2e deg FAIL: expected %s\n",
           biaslt, WANT_DEFECT ? "> 1e-6" : "< 1e-9");
    bad++;
  }

  if (biasstep >= 0 && (WANT_DEFECT ? (biasstep > 1e-6) : (biasstep < 1e-9)))
    printf("  bias at the step       %.2e deg OK\n", biasstep);
  else {
    printf("  bias at the step       %.2e deg FAIL: expected %s\n",
           biasstep, WANT_DEFECT ? "> 1e-6" : "< 1e-9");
    bad++;
  }

  if (biasvel >= 0 && (WANT_DEFECT ? (biasvel > 1e-9) : (biasvel < 1e-11)))
    printf("  Horizons offset rate   %.2e AU/day OK\n", biasvel);
  else {
    printf("  Horizons offset rate   %.2e AU/day FAIL: expected %s\n",
           biasvel, WANT_DEFECT ? "> 1e-9" : "< 1e-11");
    bad++;
  }

  if (WANT_DEFECT ? (jplhor1799 > 1e-6) : (jplhor1799 >= 0 && jplhor1799 < 1e-8))
    printf("  speed across 1799      %.2e deg/day OK\n", jplhor1799);
  else {
    printf("  speed across 1799      %.2e deg/day FAIL: expected %s\n",
           jplhor1799, WANT_DEFECT ? "> 1e-6" : "< 1e-8");
    bad++;
  }

  if (WANT_DEFECT ? (precwin > 1e-6) : (precwin >= 0 && precwin < 1e-8))
    printf("  speed across a window  %.2e deg/day OK\n", precwin);
  else {
    printf("  speed across a window  %.2e deg/day FAIL: expected %s\n",
           precwin, WANT_DEFECT ? "> 1e-6" : "< 1e-8");
    bad++;
  }

  if (WANT_DEFECT ? (nutfar > 5e-10) : (nutfar >= 0 && nutfar < 5e-11))
    printf("  nutation rate far out  %.2e deg/day OK\n", nutfar);
  else {
    printf("  nutation rate far out  %.2e deg/day FAIL: expected %s\n",
           nutfar, WANT_DEFECT ? "> 5e-10" : "< 5e-11");
    bad++;
  }

  if (WANT_DEFECT ? (owenjoin > 1e-9) : (owenjoin >= 0 && owenjoin < 1e-9))
    printf("  speed across Owen join %.2e deg/day OK\n", owenjoin);
  else {
    printf("  speed across Owen join %.2e deg/day FAIL: expected %s\n",
           owenjoin, WANT_DEFECT ? "> 1e-9" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (moshmoonwin > 1e-7) : (moshmoonwin >= 0 && moshmoonwin < 1e-8))
    printf("  Moshier Moon at window %.2e deg/day OK\n", moshmoonwin);
  else {
    printf("  Moshier Moon at window %.2e deg/day FAIL: expected %s\n",
           moshmoonwin, WANT_DEFECT ? "> 1e-7" : "< 1e-8");
    bad++;
  }

  if (WANT_DEFECT ? (jdatewin > 1e-6) : (jdatewin >= 0 && jdatewin < 1e-9))
    printf("  equinox of date window %.2e deg/day OK\n", jdatewin);
  else {
    printf("  equinox of date window %.2e deg/day FAIL: expected %s\n",
           jdatewin, WANT_DEFECT ? "> 1e-6" : "< 1e-9");
    bad++;
  }

  if (WANT_DEFECT ? (nutjitter > 1e-10) : (nutjitter >= 0 && nutjitter < 1e-13))
    printf("  2000B past 4e8 jitter  %.2e deg OK\n", nutjitter);
  else {
    printf("  2000B past 4e8 jitter  %.2e deg FAIL: expected %s\n",
           nutjitter, WANT_DEFECT ? "> 1e-10" : "< 1e-13");
    bad++;
  }

  printf(bad ? "G24 FAIL\n" : "G24 PASS\n");
  return bad ? 1 : 0;
}
