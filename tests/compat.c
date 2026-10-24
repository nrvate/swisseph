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

int main(int argc, char **argv)
{
  int bad = 0, contam, wiped, pheno;
  double drift, moonlt, jpltt, sidstep, hlt;

  EPHE = (argc > 1) ? argv[1] : "../ephe";
  printf("G24: SWE_UPSTREAM_COMPAT restores what it claims -- %s\n", MODE);

  contam = jplhor_contaminates();
  drift  = pctr_drift();
  wiped  = eclnut_wipes_constants();
  pheno  = pheno_accepts_eclnut();
  moonlt = moon_lighttime();
  jpltt  = jpl_tt_read();
  sidstep = sidt_splice_step();
  hlt    = helio_lt_from_barycentre();

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

  swe_close();
  printf(bad ? "G24 FAIL\n" : "G24 PASS\n");
  return bad ? 1 : 0;
}
