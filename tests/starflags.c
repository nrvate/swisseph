/* starflags.c -- G66: the star functions return the flags used, as
 * swe_calc() does, and a request at tjd = 0 does not read an empty nutation
 * cache.
 *
 * Two defects (UPSTREAM-BUGS 67):
 * - swe_fixstar2(), swe_fixstar() and their _ut forms returned the caller's
 *   own iflag; swe_calc() returns the flags it used (NONUT added for a
 *   sidereal request, NOABERR and NOGDEFL for heliocentric Moshier, the
 *   ephemeris bits of a substituted ephemeris). swe_fixstar_ut()'s check of
 *   the ephemeris returned could never fire;
 * - ctx->nut and the other nutation caches are emptied by tnut = 0, which is
 *   also JD 0 (-4712): a request at exactly tjd = 0 found the emptied cache
 *   and used its zero angles. swe_get_ayanamsa_ex() at tjd 0, default flags,
 *   lacked the nutation in longitude, 4.2e-4 deg.
 *
 * The check: twelve flag sets (default, sidereal, heliocentric Moshier,
 * barycentric, topocentric, J2000, equatorial, XYZ, speed asked or not, and
 * a JPL request with the file absent and the fallback allowed), four star
 * functions each: the returned value equals swe_calc()'s for Mars with the
 * same flags (and, for the _ut forms, swe_calc_ut()'s). And the sidereal
 * ayanamsa at tjd = 0 against tjd = 1e-9 (within 1e-7 deg, measured 1.5e-9
 * of real motion) and against the NONUT value (the difference is the
 * nutation, 4.2e-4 deg, not zero). The upstream build fails 28 of the 48
 * flag rows (the ones whose used flags differ from the asked) and the
 * ayanamsa. Run as
 *   starflags EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define EPHMASK (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)

int main(int argc, char **argv)
{
  static const int32 set[12] = {
    0, SEFLG_SIDEREAL, SEFLG_HELCTR | SEFLG_MOSEPH, SEFLG_BARYCTR, SEFLG_TOPOCTR,
    SEFLG_J2000, SEFLG_EQUATORIAL, SEFLG_XYZ, SEFLG_SPEED, SEFLG_SIDEREAL | SEFLG_SPEED,
    SEFLG_MOSEPH, SEFLG_JPLEPH,
  };
  const double t = 2460000.3;
  char serr[256], s[64];
  double x[6], a0, a1, an;
  int32 fl, want, got;
  int k, f, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: starflags EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  swe_set_ephe_fallback(1);
  printf("G66: the star functions return the flags used\n");
  for (k = 0; k < 12; k++) {
    int ut;
    for (ut = 0; ut <= 1; ut++) {
      want = ut ? swe_calc_ut(t, SE_MARS, set[k], x, serr)
                : swe_calc(t, SE_MARS, set[k], x, serr);
      for (f = 0; f < 2; f++) {
        strcpy(s, "Spica");
        if (ut)
          got = f ? swe_fixstar_ut(s, t, set[k], x, serr) : swe_fixstar2_ut(s, t, set[k], x, serr);
        else
          got = f ? swe_fixstar(s, t, set[k], x, serr) : swe_fixstar2(s, t, set[k], x, serr);
        if (set[k] == SEFLG_JPLEPH && (want & EPHMASK) == SEFLG_JPLEPH) {
          printf("  FAIL: no fallback happened, the row proves nothing\n");
          bad++;
        }
        if (got != ERR && (got & EPHMASK) != (want & EPHMASK)) {
          printf("  FAIL: ephemeris bits %#x name another ephemeris than swe_calc's %#x\n",
                 (unsigned) (got & EPHMASK), (unsigned) (want & EPHMASK));
          bad++;
        }
        if (got != want) {
          printf("  flags %#010x %-14s returned %#010x, swe_calc %#010x\n",
                 (unsigned) set[k], ut ? (f ? "swe_fixstar_ut" : "swe_fixstar2_ut")
                                       : (f ? "swe_fixstar" : "swe_fixstar2"),
                 (unsigned) got, (unsigned) want);
          bad++;
        }
      }
    }
  }
  fl = SEFLG_SWIEPH | SEFLG_SIDEREAL;
  swe_close();
  swe_set_ephe_path(argv[1]);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_get_ayanamsa_ex(0.0, fl, &a0, serr);
  swe_close();
  swe_set_ephe_path(argv[1]);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_get_ayanamsa_ex(1e-9, fl, &a1, serr);
  swe_close();
  swe_set_ephe_path(argv[1]);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_get_ayanamsa_ex(0.0, fl | SEFLG_NONUT, &an, serr);
  printf("  ayanamsa at tjd 0: %.9f, at 1e-9: %.9f (differ by %.2e), NONUT %.9f (nutation %.2e)\n",
         a0, a1, fabs(a0 - a1), an, a0 - an);
  if (fabs(a0 - a1) > 1e-7) {
    printf("  FAIL: the ayanamsa at tjd 0 is not the one at 1e-9\n");
    bad++;
  }
  if (fabs(a0 - an) < 1e-4) {
    printf("  FAIL: the ayanamsa at tjd 0 lacks the nutation\n");
    bad++;
  }
  swe_close();
  printf(bad ? "G66 FAIL: %d\n" : "G66 PASS\n", bad);
  return bad ? 1 : 0;
}
