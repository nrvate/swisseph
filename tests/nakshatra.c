/* nakshatra.c -- G96: swe_split_deg()'s nakshatras are shifted for the
 * Sheoran ayanamsa alone.
 *
 * The defect (UPSTREAM-BUGS 97): split_deg_nakshatra() moves the place by
 * 3 deg 20' for the Sheoran "Vedic" ayanamsa (0 Aries = 3 deg 20' of
 * Ashvini), and tested the mode as a bit mask, (sid_mode & 39) == 39.
 * SE_SIDM_USER is 255, which has every bit of 39, with or without the
 * SIDBITs: a user-defined ayanamsa's nakshatras began 3 deg 20' early,
 * 5 deg read as Ashvini 8 deg 20' and 10 deg as Bharani 0 deg.
 *
 * The check: 5 deg and 10 deg split with SE_SPLIT_DEG_NAKSHATRA, rounded
 * to the second (the shift's constant, 3.33333333333333, leaves Sheoran's
 * 5 deg at 8 deg 19' 59.99999" unrounded), under the 47 predefined modes,
 * SE_SIDM_USER alone and with SE_SIDBIT_USER_UT, ECL_DATE, ECL_T0 and
 * SSY_PLANE, Sheoran with ECL_DATE (which swe_set_sid_mode() drops for a
 * star mode) and Lahiri with ECL_DATE, ECL_T0, SSY_PLANE and
 * NO_PREC_OFFSET: Ashvini 5 deg 00' and 10 deg 00' in every mode but
 * Sheoran, Ashvini 8 deg 20' and Bharani 0 deg 00' in Sheoran: 114
 * checks. The build before the fix and the upstream build fail the 10
 * USER ones. Run as
 *   nakshatra
 */
#include <stdio.h>
#include "swephexp.h"

static int bad = 0, nchk = 0;

static void check(int mode, double deg, int nak, int d, int m)
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

int main(void)
{
  static const int bits[5] = {SE_SIDBIT_USER_UT, SE_SIDBIT_ECL_DATE, SE_SIDBIT_ECL_T0,
                              SE_SIDBIT_SSY_PLANE, SE_SIDBIT_NO_PREC_OFFSET};
  int modes[64], nm = 0, k;
  printf("G96: the nakshatras are shifted for Sheoran alone\n");
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
      check(modes[k], 5, 0, 8, 20);
      check(modes[k], 10, 1, 0, 0);
    } else {
      check(modes[k], 5, 0, 5, 0);
      check(modes[k], 10, 0, 10, 0);
    }
  }
  swe_close();
  printf("  %d checks\n", nchk);
  printf(bad ? "G96 FAIL: %d\n" : "G96 PASS\n", bad);
  return bad ? 1 : 0;
}
