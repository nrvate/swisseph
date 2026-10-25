/* j2000inst.c -- G86: tjd = J2000 is an instant like any other.
 *
 * The defect (UPSTREAM-BUGS 87): swi_precess() answered J == J2000 with the
 * identity, and the Vondrak 2011 series, the default, is not the identity
 * there: its coefficients, published to 1e-6", put the equator of date at
 * T = 0 turned 4.848e-12 rad (1 uas) about the x axis. Every position of
 * date at exactly J2000 -- planets, Moon, nodes, apsides, stars, the
 * ayanamsa -- was therefore off its own neighbours by up to 2.8e-10 deg,
 * in latitude, declination and right ascension; SEFLG_J2000 output, which
 * is not precessed, was not. Measured on ts20 before the fix, as the
 * residual of the place at J2000 against a polynomial through its six
 * neighbours 2^-10 day apart: the Sun's latitude 2.73e-10 deg, Mars's
 * 1.47e-10, Pluto's 2.63e-10, against 1e-14 at J2000 + 1/4; the true
 * node's longitude 2.5e-9 (1.3e-13); Mars's latitude speed 4.2e-11
 * deg/day (1.1e-13). (The precession model OWEN_1990 is 3.1e-7 rad, 0.063",
 * off the identity at T = 0, and had the same jump.)
 *
 * The check: for the Swiss files, de200.eph and Moshier; fifteen bodies
 * (Sun to Pluto, the mean and true nodes, the mean and osculating apogees,
 * Chiron); fourteen flag sets with SEFLG_SPEED (of date, equatorial,
 * NONUT, sidereal Lahiri and its equatorial, J2000 and its equatorial,
 * TRUEPOS, NOABERR|NOGDEFL, heliocentric, barycentric, topocentric, XYZ,
 * equatorial XYZ); swe_fixstar2() for five stars and five flag sets;
 * swe_nod_aps() for the Moon, Mars and Jupiter, mean and osculating, its
 * four points; swe_get_ayanamsa_ex() for three modes with and without
 * nutation. Every value, position and speed, at J2000 is judged two ways:
 *   (a) its offset from the mean of its values at J2000 +- 2^-30 day;
 *   (b) its residual against the polynomial through its six neighbours at
 *       J2000 + k 2^-10, k = +-1..3;
 * each within three times the largest of the same at twelve other instants
 * J2000 + c/16 (c = +-1..6), plus 1e-12 deg (a) or 1e-13 deg (b), ten
 * times that for speeds: J2000 no rougher than its neighbourhood, which for
 * the differenced speeds (the osculating points') is far from 0. Distances
 * and cartesian components are scaled to degrees at the body's distance.
 * Rows a call refuses at any instant (Moshier barycentric) are skipped.
 * Measured: 744 rows, 15 skipped; the largest ratio to the tolerance 0.61
 * (Mercury's topocentric latitude speed, (b), rounding).
 *
 * The upstream build fails 549 rows; ts20 before this fix 526 (the defect
 * is upstream's; the rest are upstream's other roughness at J2000), this
 * build with SWE_UPSTREAM_COMPAT 549. Run as
 *   j2000inst EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define J2K 2451545.0
#define NV 24	/* values per row, at most (nod_aps: 4 x 6) */
#define EPHBITS (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)
#ifndef G86_PRINT
#define G86_PRINT 40	/* failing rows printed */
#endif

/* a request: what to call and with what */
struct req {
  int kind;		/* 0 swe_calc, 1 swe_fixstar2, 2 swe_nod_aps, 3 ayanamsa */
  int body;
  int32 flags;
  int method;
  int sidm;
  const char *star;
  const char *label;
};

static int nfail, nrows, nskip;
static double worst_a_pos, worst_a_spd, worst_q;
static char worst_row[192];

/* values of one request at t; n = number of values; scale[i] = what
 * value i is divided by to be degrees (0: it is an angle). Returns 0, or
 * -1 if the call refused. */
static int eval(const struct req *r, double t, double *v, double *scale, int *n)
{
  char serr[AS_MAXCH], star[64];
  double xx[6], xa[6], xd[6], xp[6], xe[6], aya;
  int32 rf;
  int i, j;
  if (r->sidm >= 0)
    swe_set_sid_mode(r->sidm, 0, 0);
  switch (r->kind) {
  case 0:
  case 1:
    if (r->kind == 0) {
      rf = swe_calc(t, r->body, r->flags, xx, serr);
    } else {
      strcpy(star, r->star);
      rf = swe_fixstar2(star, t, r->flags, xx, serr);
    }
    if (rf < 0 || (rf & EPHBITS) != (r->flags & EPHBITS))
      return -1;
    if (r->flags & SEFLG_XYZ) {
      double d = sqrt(xx[0] * xx[0] + xx[1] * xx[1] + xx[2] * xx[2]);
      for (i = 0; i < 6; i++)
	scale[i] = d * DEGTORAD;
    } else {
      for (i = 0; i < 6; i++)
	scale[i] = (i % 3 == 2) ? xx[2] * DEGTORAD : 0;
    }
    memcpy(v, xx, sizeof xx);
    *n = 6;
    return 0;
  case 2:
    rf = swe_nod_aps(t, r->body, r->flags, r->method, xa, xd, xp, xe, serr);
    if (rf < 0)
      return -1;
    for (j = 0; j < 4; j++) {
      double *p = j == 0 ? xa : j == 1 ? xd : j == 2 ? xp : xe;
      for (i = 0; i < 6; i++) {
	v[6 * j + i] = p[i];
	scale[6 * j + i] = (i % 3 == 2) ? p[2] * DEGTORAD : 0;
      }
    }
    *n = 24;
    return 0;
  default:
    if (swe_get_ayanamsa_ex(t, r->flags, &aya, serr) < 0)
      return -1;
    v[0] = aya;
    scale[0] = 0;
    *n = 1;
    return 0;
  }
}

/* a - b, as an angle where the value is one */
static double diff(double a, double b, double scale)
{
  double d = a - b;
  if (scale == 0) {
    while (d > 180) d -= 360;
    while (d < -180) d += 360;
    return d;
  }
  return d / scale;
}

/* residual at index 3 of seven values 2^-10 apart against the polynomial
 * through the other six */
static double fitres(double v[7], double scale)
{
  static const double w[7] = { 1.0 / 20, -3.0 / 10, 3.0 / 4, 0,
                               3.0 / 4, -3.0 / 10, 1.0 / 20 };
  double s = 0;
  int i;
  for (i = 0; i < 7; i++)
    s += w[i] * diff(v[i], v[3], scale);
  return -s;	/* v[3] - fit, the fit taken relative to v[3] */
}

/* at t0, for every value: its offset from the mean of its values at
 * t0 +- 2^-30 (mid) and its residual against the polynomial through its
 * values at t0 + k 2^-10, k = +-1..3 (fit) */
static int probe(const struct req *r, double t0, double *mid, double *fit,
                 double *v0, int *n)
{
  double v[7][NV], vm[NV], vp[NV], sc[NV], scx[NV], col[7];
  double d = ldexp(1, -30);
  int k, i, nx;
  for (k = -3; k <= 3; k++)
    if (eval(r, t0 + k * ldexp(1, -10), v[k + 3], k == 0 ? sc : scx,
             k == 0 ? n : &nx) < 0)
      return -1;
  if (eval(r, t0 - d, vm, scx, &nx) < 0 || eval(r, t0 + d, vp, scx, &nx) < 0)
    return -1;
  for (i = 0; i < *n; i++) {
    for (k = 0; k < 7; k++)
      col[k] = v[k][i];
    fit[i] = fitres(col, sc[i]);
    if (sc[i] == 0)	/* an angle: the mean of the neighbours across 360 */
      mid[i] = diff(v[3][i], vm[i] + diff(vp[i], vm[i], 0) / 2, 0);
    else
      mid[i] = (v[3][i] - (vm[i] + vp[i]) / 2) / sc[i];
    v0[i] = v[3][i];
  }
  return 0;
}

static void run(const struct req *r, const char *eph)
{
  double mid0[NV], fit0[NV], mid[NV], fit[NV], v0[NV], vc[NV];
  double nbm[NV], nbf[NV];
  int n, nc, i, c, bad = 0;
  if (probe(r, J2K, mid0, fit0, v0, &n) < 0) {
    nskip++;
    return;
  }
  for (i = 0; i < n; i++)
    nbm[i] = nbf[i] = 0;
  for (c = -6; c <= 6; c++) {
    if (c == 0)
      continue;
    if (probe(r, J2K + c / 16.0, mid, fit, vc, &nc) < 0) {
      nskip++;
      return;
    }
    for (i = 0; i < n; i++) {
      if (fabs(mid[i]) > nbm[i]) nbm[i] = fabs(mid[i]);
      if (fabs(fit[i]) > nbf[i]) nbf[i] = fabs(fit[i]);
    }
  }
  nrows++;
  for (i = 0; i < n; i++) {
    int speed = (i % 6) >= 3 && r->kind != 3;
    double tol_a = 3 * nbm[i] + (speed ? 1e-11 : 1e-12);
    double tol_b = 3 * nbf[i] + (speed ? 1e-12 : 1e-13);
    double qa = fabs(mid0[i]) / tol_a, qb = fabs(fit0[i]) / tol_b;
    if (speed) {
      if (fabs(mid0[i]) > worst_a_spd) worst_a_spd = fabs(mid0[i]);
    } else {
      if (fabs(mid0[i]) > worst_a_pos) worst_a_pos = fabs(mid0[i]);
    }
    if (qa > worst_q || qb > worst_q) {
      worst_q = qa > qb ? qa : qb;
      snprintf(worst_row, sizeof worst_row, "%s %s value %d", eph, r->label, i);
    }
    if (qa > 1 || qb > 1) {
      if (bad == 0 && nfail < G86_PRINT)
	printf("  %s %s value %d: at J2000 %.3e off the mean of +-2^-30 "
	       "(neighbourhood %.3e), fit residual %.3e (neighbourhood %.3e)\n",
	       eph, r->label, i, mid0[i], nbm[i], fit0[i], nbf[i]);
      bad = 1;
    }
  }
  if (bad)
    nfail++;
}

int main(int argc, char **argv)
{
  static const int bodies[] = { SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS,
    SE_MARS, SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO,
    SE_MEAN_NODE, SE_TRUE_NODE, SE_MEAN_APOG, SE_OSCU_APOG, SE_CHIRON };
  static const int32 fsets[] = { 0, SEFLG_EQUATORIAL, SEFLG_NONUT,
    SEFLG_SIDEREAL, SEFLG_SIDEREAL | SEFLG_EQUATORIAL, SEFLG_J2000,
    SEFLG_J2000 | SEFLG_EQUATORIAL, SEFLG_TRUEPOS,
    SEFLG_NOABERR | SEFLG_NOGDEFL, SEFLG_HELCTR, SEFLG_BARYCTR,
    SEFLG_TOPOCTR, SEFLG_XYZ, SEFLG_XYZ | SEFLG_EQUATORIAL };
  static const int32 ephs[] = { SEFLG_SWIEPH, SEFLG_JPLEPH, SEFLG_MOSEPH };
  static const char *ephn[] = { "swieph", "jpl", "moshier" };
  static const char *stars[] = { "Aldebaran", "Spica", "Sirius", "Polaris",
                                 "Regulus" };
  static const int32 sfsets[] = { 0, SEFLG_EQUATORIAL, SEFLG_SIDEREAL,
                                  SEFLG_J2000, SEFLG_TOPOCTR };
  static const int nabodies[] = { SE_MOON, SE_MARS, SE_JUPITER };
  static const int methods[] = { SE_NODBIT_MEAN, SE_NODBIT_OSCU };
  static const int sidms[] = { SE_SIDM_FAGAN_BRADLEY, SE_SIDM_LAHIRI,
                               SE_SIDM_J2000 };
  char label[128];
  struct req r;
  size_t e, b, f;
  if (argc < 2) {
    fprintf(stderr, "usage: j2000inst EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_jpl_file("de200.eph");
  swe_set_topo(8.55, 47.37, 400);
  for (e = 0; e < 3; e++) {
    for (b = 0; b < sizeof bodies / sizeof bodies[0]; b++)
      for (f = 0; f < sizeof fsets / sizeof fsets[0]; f++) {
	memset(&r, 0, sizeof r);
	r.kind = 0;
	r.body = bodies[b];
	r.flags = ephs[e] | SEFLG_SPEED | fsets[f];
	r.sidm = (fsets[f] & SEFLG_SIDEREAL) ? SE_SIDM_LAHIRI : -1;
	snprintf(label, sizeof label, "swe_calc body %d flags 0x%x",
		 bodies[b], (unsigned) r.flags);
	r.label = label;
	run(&r, ephn[e]);
      }
    for (b = 0; b < sizeof stars / sizeof stars[0]; b++)
      for (f = 0; f < sizeof sfsets / sizeof sfsets[0]; f++) {
	memset(&r, 0, sizeof r);
	r.kind = 1;
	r.star = stars[b];
	r.flags = ephs[e] | SEFLG_SPEED | sfsets[f];
	r.sidm = (sfsets[f] & SEFLG_SIDEREAL) ? SE_SIDM_LAHIRI : -1;
	snprintf(label, sizeof label, "swe_fixstar2 %s flags 0x%x",
		 stars[b], (unsigned) r.flags);
	r.label = label;
	run(&r, ephn[e]);
      }
    for (b = 0; b < sizeof nabodies / sizeof nabodies[0]; b++)
      for (f = 0; f < 2; f++) {
	int h;
	for (h = 0; h < 2; h++) {
	  memset(&r, 0, sizeof r);
	  r.kind = 2;
	  r.body = nabodies[b];
	  r.method = methods[f];
	  r.flags = ephs[e] | SEFLG_SPEED | (h ? SEFLG_EQUATORIAL : 0);
	  r.sidm = -1;
	  snprintf(label, sizeof label, "swe_nod_aps body %d method %d flags 0x%x",
		   nabodies[b], methods[f], (unsigned) r.flags);
	  r.label = label;
	  run(&r, ephn[e]);
	}
      }
    for (b = 0; b < sizeof sidms / sizeof sidms[0]; b++)
      for (f = 0; f < 2; f++) {
	memset(&r, 0, sizeof r);
	r.kind = 3;
	r.sidm = sidms[b];
	r.flags = ephs[e] | (f ? SEFLG_NONUT : 0);
	snprintf(label, sizeof label, "ayanamsa mode %d flags 0x%x",
		 sidms[b], (unsigned) r.flags);
	r.label = label;
	run(&r, ephn[e]);
      }
  }
  swe_close();
  printf("G86: %d rows (%d skipped); (a) largest %.2e deg, %.2e deg/day; "
	 "largest ratio to tolerance %.2f (%s)\n", nrows, nskip, worst_a_pos,
	 worst_a_spd, worst_q, worst_row);
  if (nfail) {
    printf("G86 FAIL: %d\n", nfail);
    return 1;
  }
  printf("G86 PASS\n");
  return 0;
}
