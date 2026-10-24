/* sidsplice.c -- G35: sidereal time is continuous where the default model
 * changes formula, under any precession, nutation or delta-t model.
 *
 * SEMOD_SIDT_LONGTERM, the default, uses the IERS Conventions 2010 sidereal
 * time between 1850 and 2050 and sidtime_long_term() outside, which derives
 * it from the mean Earth of Simon et al. 1994 through the context's own
 * precession, nutation and delta t. The two are spliced by constant offsets,
 * SIDT_LTERM_OFS0 and OFS1, fitted once. Under today's default models they
 * no longer meet: sidereal time steps by -1.908" at 2050-01-01 0h UT and by
 * -0.097" at 1850-01-01, the same in upstream 2.10.03 and in the fork, so a
 * house cusp or an ascendant jumps by about that much across the instant.
 * And a constant cannot fit every model: sidtime_long_term() takes the
 * mean Earth at TT for a UT, so its value moves 0.041" for every second the
 * delta-t model moves at the boundary.
 *
 * The check: across each boundary, the change of swe_sidtime_r over the
 * two seconds around it less the sidereal rate times the exact interval
 * (the JDs are differenced, so their rounding drops out), under the
 * default models and four others in turn on one context, then the default
 * again, so an offset kept from earlier models would show. Within 0.001".
 * The upstream build fails at both boundaries. Moshier, no files. Run as
 *   sidsplice
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(void)
{
  static const double rgjd[2] = {2396758.5, 2469807.5};   /* 1850-01-01, 2050-01-01 0h UT */
  static const char *rgsz[2] = {"1850-01-01", "2050-01-01"};
  static const char *rgModels[6] = {"0,0,0,0,0,0,0,0", "0,0,0,4", "0,8,8", "4",
    "2", "0,0,0,0,0,0,0,0"};
  static const char *rgszModels[6] = {"default", "nutation IAU 2000B",
    "precession IAU 2006", "delta t Espenak-Meeus", "delta t Stephenson 1997",
    "default again"};
  const double rate = 24.0 * 1.00273781191135448;   /* sidereal hours per UT day */
  int m, i, bad = 0;

  printf("G35: sidereal time is continuous across the 1850 and 2050 splices\n");
  /* one context throughout, so an offset kept from the previous models
   * would show as a step */
  swe_ctx *ctx = swe_ctx_new();
  for (m = 0; m < 6; m++) {
    swe_set_astro_models_r(ctx, (char *) rgModels[m], 0);
    for (i = 0; i < 2; i++) {
      double ta = rgjd[i] - 1.0 / 86400.0, tb = rgjd[i] + 1.0 / 86400.0;
      double d = swe_sidtime_r(ctx, tb) - swe_sidtime_r(ctx, ta);
      double step;
      if (d < -12) d += 24;
      step = (d - rate * (tb - ta)) * 15 * 3600;   /* arcsec */
      printf("  %-24s %s  step %+.6f\"\n", rgszModels[m], rgsz[i], step);
      if (fabs(step) > 1e-3) {
        printf("    FAIL: sidereal time steps at the splice\n");
        bad++;
      }
    }
  }
  swe_ctx_free(ctx);
  printf(bad ? "G35 FAIL: %d\n" : "G35 PASS\n", bad);
  return bad ? 1 : 0;
}
