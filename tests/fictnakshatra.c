/* fictnakshatra.c -- G44: the built-in fictitious elements are seorbel.txt's,
 * and swe_split_deg()'s nakshatras are shifted for Sheoran alone.
 *
 * 1. The built-in Neely table of swemplan.c had Kronos' semi-axis as
 * 64.81960, a swap of the last two digits of seorbel.txt's 64.81690 (and of
 * the original Witte elements' 64.816896). Without seorbel.txt on the path
 * the built-in table is used, and Kronos' mean motion was off by 0.0043 deg
 * per century from 1900: -0.0086 deg at 1700, +0.0169 deg at 2300. The other
 * fourteen rows agree with the file.
 *
 * The check: the 15 built-in bodies (Cupido..Pickering), heliocentric J2000
 * Moshier, geometric, at 1700, 1900, 2100, 2300 and 2500, once with the
 * shipped seorbel.txt on the path and once with an empty directory only (the
 * built-in table); the two longitudes, latitudes and distances agree to
 * 1e-9 deg and 1e-12 AU. Measured with the fix: 0 deg, 5.7e-14 AU. Before
 * it Kronos was off by 0.0086 deg at 1700, 0.0169 deg at 2300 and 0.0257 deg
 * at 2500 (2.8e-3 AU in distance), the others 0.
 *
 * 2. split_deg_nakshatra() moves the place by 3 deg 20' for the Sheoran
 * "Vedic" ayanamsa (0 Aries = 3 deg 20' of Ashvini), and tested the mode as
 * a bit mask, (sid_mode & 39) == 39. SE_SIDM_USER is 255, which has every
 * bit of 39, with or without the SIDBITs: a user-defined ayanamsa's
 * nakshatras began 3 deg 20' early, 5 deg read as Ashvini 8 deg 20' and
 * 10 deg as Bharani 0 deg.
 *
 * The check: 5 deg and 10 deg split with SE_SPLIT_DEG_NAKSHATRA, rounded
 * to the second (the shift's constant, 3.33333333333333, leaves Sheoran's
 * 5 deg at 8 deg 19' 59.99999" unrounded), under the 47 predefined modes,
 * SE_SIDM_USER alone and with SE_SIDBIT_USER_UT, ECL_DATE, ECL_T0 and
 * SSY_PLANE, Sheoran with ECL_DATE (which swe_set_sid_mode() drops for a
 * star mode) and Lahiri with ECL_DATE, ECL_T0, SSY_PLANE and
 * NO_PREC_OFFSET: Ashvini 5 deg 00' and 10 deg 00' in every mode but
 * Sheoran, Ashvini 8 deg 20' and Bharani 0 deg 00' in Sheoran: 114
 * checks. Before the fix the 10 USER ones fail. Run as
 *   fictnakshatra EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

#define NB 15
#define ND 5

static int bad = 0;

/* Part 1: the built-in elements against seorbel.txt's. */
static void builtin_elements(const char *ephe)
{
  const int32 fl = SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS;
  const double jd[ND] = {2341972.5, 2415020.5, 2488069.5, 2561118.5, 2634167.5};
  char dir[] = "/tmp/fictnakshXXXXXX", serr[256];
  static double ref[NB][ND][6], got[NB][ND][6];
  double wang[NB] = {0}, wdist = 0, wmax = 0;
  int ip, id, phase;

  if (mkdtemp(dir) == NULL) {
    perror("mkdtemp");
    bad++;
    return;
  }
  for (phase = 0; phase < 2; phase++) {
    swe_set_ephe_path(phase == 0 ? ephe : dir);
    for (ip = 0; ip < NB; ip++)
      for (id = 0; id < ND; id++) {
        double *x = phase == 0 ? ref[ip][id] : got[ip][id];
        if (swe_calc(jd[id], SE_FICT_OFFSET + ip, fl, x, serr) < 0) {
          printf("  ERROR body %d: %s\n", ip, serr);
          bad++;
        }
      }
    swe_close();
  }
  for (ip = 0; ip < NB; ip++)
    for (id = 0; id < ND; id++) {
      double dl = fabs(swe_difdeg2n(got[ip][id][0], ref[ip][id][0]));
      double db = fabs(got[ip][id][1] - ref[ip][id][1]);
      wang[ip] = fmax(wang[ip], fmax(dl, db));
      wdist = fmax(wdist, fabs(got[ip][id][2] - ref[ip][id][2]));
    }
  for (ip = 0; ip < NB; ip++) {
    char nm[64];
    swe_get_planet_name(SE_FICT_OFFSET + ip, nm);
    wmax = fmax(wmax, wang[ip]);
    if (wang[ip] > 1e-9) {
      printf("  FAIL %-18s built-in off seorbel.txt by %.2e deg\n", nm, wang[ip]);
      bad++;
    }
  }
  printf("  built-in elements: worst angle %.2e deg, distance %.2e AU\n",
         wmax, wdist);
  if (wdist > 1e-12) {
    printf("  FAIL: a distance differs\n");
    bad++;
  }
  rmdir(dir);
}

static int nchk = 0;

static void check_split(int mode, double deg, int nak, int d, int m)
{
  int id, im, is, in;
  double fr;
  swe_split_deg(deg, SE_SPLIT_DEG_NAKSHATRA | SE_SPLIT_DEG_ROUND_SEC, &id, &im, &is, &fr, &in);
  nchk++;
  if (in != nak || id != d || im != m) {
    printf("    FAIL: mode %d, %g deg: nakshatra %d %d deg %02d', expected %d %d deg %02d'\n",
           mode, deg, in, id, im, nak, d, m);
    bad++;
  }
}

/* Part 2: the Sheoran shift for Sheoran alone. */
static void nakshatras(void)
{
  static const int bits[5] = {SE_SIDBIT_USER_UT, SE_SIDBIT_ECL_DATE, SE_SIDBIT_ECL_T0,
                              SE_SIDBIT_SSY_PLANE, SE_SIDBIT_NO_PREC_OFFSET};
  int modes[64], nm = 0, k;
  for (k = 0; k < SE_NSIDM_PREDEF; k++)
    modes[nm++] = k;
  modes[nm++] = SE_SIDM_USER;
  for (k = 0; k < 4; k++)
    modes[nm++] = SE_SIDM_USER | bits[k];
  modes[nm++] = SE_SIDM_TRUE_SHEORAN | SE_SIDBIT_ECL_DATE;
  for (k = 1; k < 5; k++)
    modes[nm++] = SE_SIDM_LAHIRI | bits[k];
  for (k = 0; k < nm; k++) {
    int sheoran = modes[k] % SE_SIDBITS == SE_SIDM_TRUE_SHEORAN;
    swe_set_sid_mode(modes[k], 2451545.0, 23.5);
    if (sheoran) {
      check_split(modes[k], 5, 0, 8, 20);
      check_split(modes[k], 10, 1, 0, 0);
    } else {
      check_split(modes[k], 5, 0, 5, 0);
      check_split(modes[k], 10, 0, 10, 0);
    }
  }
  swe_close();
  printf("  nakshatras: %d checks\n", nchk);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: fictnakshatra EPHEDIR\n");
    return 2;
  }
  printf("G44: built-in fictitious elements are seorbel.txt's; nakshatras shifted for Sheoran alone\n");
  builtin_elements(argv[1]);
  nakshatras();
  printf(bad ? "G44 FAIL: %d\n" : "G44 PASS\n", bad);
  return bad ? 1 : 0;
}
