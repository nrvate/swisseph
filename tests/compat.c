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

int main(int argc, char **argv)
{
  int bad = 0, contam;
  double drift;

  EPHE = (argc > 1) ? argv[1] : "../ephe";
  printf("G24: SWE_UPSTREAM_COMPAT restores what it claims -- %s\n", MODE);

  contam = jplhor_contaminates();
  drift  = pctr_drift();

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

  swe_close();
  printf(bad ? "G24 FAIL\n" : "G24 PASS\n");
  return bad ? 1 : 0;
}
