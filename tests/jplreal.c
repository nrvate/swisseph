/* G15: SEFLG_JPLEPH really reaches a JPL ephemeris.
 *
 * Every other JPL test in this suite works on fabricated headers -- jplguard.c
 * feeds mutilated files to the reader, jplcalc.c checks the interpolator's
 * arithmetic. None of them opens a genuine JPL binary, because none is shipped:
 * the smallest is de200 at 41 MB and the useful ones are 2.6 GB, and this fork
 * keeps binaries out of the tree.
 *
 * So this gate is CONDITIONAL. Point SE_JPL_FILE at a JPL ephemeris and it
 * runs; leave it unset and it says so and passes. A skipped gate that
 * announces itself is honest; a gate that silently tests nothing is not --
 * which is exactly the defect this whole change is about.
 *
 *   SE_JPL_FILE=/path/to/de200.eph SE_TEST_EPHE=/path/to/ephe ./jplreal
 *
 * What it proves that nothing else can: that the JPL request is answered by
 * JPL. Before strict mode a missing file produced a Swiss answer with the
 * Swiss bit set and a note in serr, so a caller checking only the return code
 * could not tell. Here the strict default turns that into an error, and the
 * test additionally asserts the returned flag.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

static const int BODIES[] = { SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS,
                              SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO };
static const char *NAMES[] = { "Sun", "Moon", "Mercury", "Venus", "Mars",
                               "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto" };
#define NBODY ((int)(sizeof(BODIES)/sizeof(BODIES[0])))

/* DE200 is a 1981 ephemeris and the .se1 files are built from DE441, so they
 * genuinely disagree -- by a few hundredths of an arcsecond on the inner
 * planets and up to a few tenths on the outer ones. This bound is loose
 * enough to admit that and tight enough that a wrong body, a wrong epoch or a
 * silent substitution would blow straight through it. */
#define MAX_ARCSEC 2.0

static double shortest(double a, double b)
{
  double d = a - b;
  while (d > 180) d -= 360;
  while (d < -180) d += 360;
  return d * 3600.0;
}

int main(void)
{
  const char *jplfile = getenv("SE_JPL_FILE");
  const char *ephe = getenv("SE_TEST_EPHE");
  double xj[6], xs[6];
  char serr[AS_MAXCH], jf[AS_MAXCH];
  double jd = 2444204.095138889;    /* 1979-11-26, inside DE200's 1600..2170 */
  int i, bad = 0;

  if (jplfile == NULL || *jplfile == '\0') {
    printf("G15 SKIP: set SE_JPL_FILE to a JPL ephemeris to exercise the JPL path\n");
    return 0;
  }
  if (ephe != NULL && *ephe)
    swe_set_ephe_path((char *) ephe);
  if (strlen(jplfile) >= AS_MAXCH) {
    printf("G15 FAIL: SE_JPL_FILE too long\n");
    return 1;
  }
  strcpy(jf, jplfile);
  swe_set_jpl_file(jf);

  printf("G15: SEFLG_JPLEPH reaches a real JPL ephemeris (%s)\n", jplfile);
  printf("  %-9s %16s %16s %11s\n", "body", "JPL", "Swiss (.se1)", "diff \"");
  for (i = 0; i < NBODY; i++) {
    int32 rj, rs;
    double d;
    serr[0] = 0;
    rj = swe_calc(jd, BODIES[i], SEFLG_JPLEPH, xj, serr);
    if (rj == ERR) {
      /* Strict mode: a missing or unreadable file is an error, not a quiet
       * downgrade. That is the behaviour under test, so report it as such. */
      printf("  FAIL %-9s JPL refused: %.110s\n", NAMES[i], serr);
      bad++;
      continue;
    }
    if (!(rj & SEFLG_JPLEPH)) {
      printf("  FAIL %-9s answered by another ephemeris (rf=%d)\n", NAMES[i], (int) rj);
      bad++;
      continue;
    }
    serr[0] = 0;
    rs = swe_calc(jd, BODIES[i], SEFLG_SWIEPH, xs, serr);
    if (rs == ERR) {
      printf("  FAIL %-9s Swiss refused: %.110s\n", NAMES[i], serr);
      bad++;
      continue;
    }
    d = shortest(xj[0], xs[0]);
    printf("  %-9s %16.9f %16.9f %10.4f%s\n", NAMES[i], xj[0], xs[0], d,
           (d > MAX_ARCSEC || d < -MAX_ARCSEC) ? "  <-- OUT OF BOUND" : "");
    if (d > MAX_ARCSEC || d < -MAX_ARCSEC)
      bad++;
  }

  /* And the other half of the contract: with the JPL file deliberately
   * unavailable, the same request must fail rather than answer from Swiss. */
  swe_close();
  if (ephe != NULL && *ephe)
    swe_set_ephe_path((char *) ephe);
  strcpy(jf, "definitely-not-here.eph");
  swe_set_jpl_file(jf);
  serr[0] = 0;
  if (swe_calc(jd, SE_MARS, SEFLG_JPLEPH, xj, serr) != ERR) {
    printf("  FAIL: a missing JPL file was silently substituted\n");
    bad++;
  } else {
    printf("  missing JPL file refused rather than substituted   OK\n");
  }

  swe_close();
  printf(bad ? "G15 FAIL\n" : "G15 PASS\n");
  return bad ? 1 : 0;
}
