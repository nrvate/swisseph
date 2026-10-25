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
#include <unistd.h>
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
 * against a five-point difference over 2^-6 day, deg/day. Upstream divides
 * by 2 x 0.0001 day, not by the interval between the Moon's rounded
 * epochs, and leaves J2000's mean obliquity rate out (UPSTREAM-BUGS 75). */
static double osc_speed_off(int j2000)
{
  double t = floor(2460000.3 * 256) / 256, h = 1.0 / 64, x[6], f[5][6];
  int32 fl = SEFLG_SWIEPH | (j2000 ? SEFLG_J2000 : 0);
  int ipl = j2000 ? SE_TRUE_NODE : SE_OSCU_APOG, i = j2000 ? 1 : 0, k;
  char serr[256];
  swe_close();
  swe_set_ephe_path(EPHE);
  swe_calc(t, ipl, fl | SEFLG_SPEED, x, serr);
  for (k = 0; k < 5; k++)
    swe_calc(t + (k - 2) * h, ipl, fl, f[k], serr);
  for (k = 0; k < 5; k++)
    f[k][0] = f[2][0] + swe_difdeg2n(f[k][0], f[2][0]);
  return fabs(x[3 + i] - (8 * (f[3][i] - f[1][i]) - (f[4][i] - f[0][i])) / (12 * h));
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
 * over 2^-5 day in 2100, deg/day. The Earth's velocity at t + dp was carried
 * on linearly from t - dm and t, the acceleration at t - dm/2: 1.03e-9 in a
 * right ascension near the pole (UPSTREAM-BUGS 76); upstream's star speeds
 * have bugs 49 and 58 besides. */
static double star_aberr_accel_off(void)
{
  double t = floor(2488070.1 * 256) / 256, h = 1.0 / 32, x[6], q[5];
  int32 fl = SEFLG_SWIEPH | SEFLG_EQUATORIAL;
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

int main(int argc, char **argv)
{
  int bad = 0, contam, wiped, pheno;
  double drift, moonlt, jpltt, sidstep, hlt, obs, gm, siddt, era, spd3, spdlt, latspd, toposun, merclt, moonanc;
  double nodspd, apslat, deflr, nodflag, nodj2k, natopo, orbel, intpsf, intprough, stardd, fictneg, nutr, startd, hspd, hsid, moshr, moshs, n3spd, moonpspd, moonpull, hfixp, hresp, apsrate, sflags, ayajd0, abr2, intpfix, moonpulle, nutob, merclr, aberrate, kronos, staracc, intplt, pctrdd, oscspd, oscj2k, moshlt, nodjd0;
  int hfix, hres;

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
  sflags = star_ret_flags_off();
  ayajd0 = ayanamsa_jd0_off();
  nodjd0 = nodaps_jd0_off();
  n3spd = node3_speed_off();
  moonpspd = moon_perigee_speed_off();
  apsrate = planet_apsis_rate_off();
  aberrate = planet_aberr_rate_off();
  nutr = nut_rate_off();
  nutob = nut_obliq_rate_off();
  staracc = star_aberr_accel_off();
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
  moshlt = mosh_lt_off();

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

  if (WANT_DEFECT ? (stardd > 1e-4) : (stardd < 1e-5))
    printf("  star distance rate     %.2e AU/day OK\n", stardd);
  else {
    printf("  star distance rate     %.2e AU/day FAIL: expected %s\n",
           stardd, WANT_DEFECT ? "> 1e-4" : "< 1e-5");
    bad++;
  }

  if (WANT_DEFECT ? (oscspd > 1e-6) : (oscspd < 3e-7))
    printf("  osc. apogee speed      %.2e deg/day OK\n", oscspd);
  else {
    printf("  osc. apogee speed      %.2e deg/day FAIL: expected %s\n",
           oscspd, WANT_DEFECT ? "> 1e-6" : "< 3e-7");
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

  if (WANT_DEFECT ? (moshlt > 1e-7) : (moshlt < 1e-9))
    printf("  Moshier light time     %.2e AU/day OK\n", moshlt);
  else {
    printf("  Moshier light time     %.2e AU/day FAIL: expected %s\n",
           moshlt, WANT_DEFECT ? "> 1e-7" : "< 1e-9");
    bad++;
  }

  printf(bad ? "G24 FAIL\n" : "G24 PASS\n");
  return bad ? 1 : 0;
}
