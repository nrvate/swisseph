/* G25: true JPL Horizons mode -- the path behind SEFLG_JPLHOR.
 *
 * OPT-IN. It needs a JPL ephemeris with real coefficients AND a DE number
 * >= 403, which is a 2.6 GB download that cannot live in the repository or
 * in CI. Without one the gate skips, loudly enough to notice:
 *
 *   make check-jplhor SE_TEST_EPHE=/path/holding/de431.eph SE_JPL_FILE=de431.eph
 *
 * Why it needed that file. SEFLG_JPLHOR survives plaus_iflag() only when
 * SEFLG_JPLEPH is set AND the IERS dpsi/deps corrections loaded, and
 * load_dpsi_deps() refuses a DE below 403. de200.eph is shipped but is
 * DE200, and G19's synthetic header carries numde=431 but no coefficients to
 * compute from -- each supplies half of what this needs. So bessel(), which
 * interpolates those corrections, measured 0.00% for the life of this fork.
 *
 * What is asserted, in order of what would actually break:
 *
 *   1. true Horizons mode is ENTERED -- the returned flags keep SEFLG_JPLHOR
 *      and do not carry SEFLG_JPLHOR_APPROX. Without this the rest is
 *      measuring the approximation and would pass while the real path rots.
 *   2. it produces a DIFFERENT answer from the approximation, so the
 *      corrections demonstrably reached the result rather than being loaded
 *      and dropped.
 *   3. it holds across the loaded span, which is what drives bessel()'s
 *      interpolation off the ends and through the middle.
 *   4. without the EOP file the same request degrades to the approximation --
 *      the control that makes 1 mean something.
 *
 *   jplhor <dir-with-eop> <dir-without-eop> <jpl-file>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "swephexp.h"

/* The Makefile writes 40 daily EOPC04 rows from MJD 51544 = 2000-01-01. */
#define EOP_BEG 2451544.5
#define EOP_N   40

static int32 calc_at(const char *dir, const char *jplfile, double tjd,
                     int32 extra, double *xx, char *serr)
{
  char jf[AS_MAXCH];
  swe_close();
  swe_set_ephe_path((char *) dir);
  strcpy(jf, jplfile);
  swe_set_jpl_file(jf);
  serr[0] = '\0';
  return swe_calc(tjd, SE_MARS, SEFLG_JPLEPH | extra, xx, serr);
}

int main(int argc, char **argv)
{
  const char *dir_eop  = (argc > 1) ? argv[1] : ".";
  const char *dir_bare = (argc > 2) ? argv[2] : ".";
  const char *jplfile  = (argc > 3) ? argv[3] : "de431.eph";
  double xh[6], xa[6], xx[6];
  char serr[AS_MAXCH] = "";
  int32 rh, ra, rf;
  double mid = EOP_BEG + EOP_N / 2.0;
  int bad = 0, i;

  printf("G25: true JPL Horizons mode (%s)\n", jplfile);

  /* 1. Horizons mode entered, not silently downgraded. */
  rh = calc_at(dir_eop, jplfile, mid, SEFLG_JPLHOR, xh, serr);
  if (rh != ERR && (rh & SEFLG_JPLHOR) && !(rh & SEFLG_JPLHOR_APPROX)) {
    printf("  SEFLG_JPLHOR kept, not downgraded        OK\n");
  } else {
    printf("  FAIL: JPLHOR not entered (rf=%d): %.90s\n", (int) rh, serr);
    return 1;   /* everything below is meaningless if this failed */
  }

  /* 2. The corrections reached the answer. */
  ra = calc_at(dir_eop, jplfile, mid, SEFLG_JPLHOR_APPROX, xa, serr);
  if (ra != ERR && xh[0] != xa[0]) {
    printf("  differs from the approximation           OK  (%.4f arcsec)\n",
           fabs(xh[0] - xa[0]) * 3600.0);
  } else {
    printf("  FAIL: Horizons and its approximation agree exactly -- the\n"
           "        dpsi/deps corrections were loaded but not applied\n");
    bad++;
  }

  /* 3. Across the span, which is what bessel() interpolates over. */
  {
    int held = 0;
    for (i = 1; i < EOP_N; i += 6) {
      rf = calc_at(dir_eop, jplfile, EOP_BEG + i, SEFLG_JPLHOR, xx, serr);
      if (rf != ERR && (rf & SEFLG_JPLHOR) && !(rf & SEFLG_JPLHOR_APPROX))
        held++;
      else
        printf("  FAIL: day %d of the span did not hold Horizons mode\n", i);
    }
    if (held == (EOP_N - 1 + 5) / 6)
      printf("  held across %d days of the loaded span   OK\n", EOP_N);
    else
      bad++;
  }

  /* 4. The control: no EOP file, so the same request must degrade. */
  rf = calc_at(dir_bare, jplfile, mid, SEFLG_JPLHOR, xx, serr);
  if (rf == ERR || !(rf & SEFLG_JPLHOR)) {
    printf("  without eop_1962_today.txt: degrades     OK\n");
  } else {
    printf("  FAIL: Horizons mode claimed without the corrections loaded\n");
    bad++;
  }

  swe_close();
  printf(bad ? "G25 FAIL\n" : "G25 PASS\n");
  return bad ? 1 : 0;
}
