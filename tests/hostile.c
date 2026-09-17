/* G25: hostile arguments reach no undefined behaviour.
 *
 * The ephemeris server passes body numbers from the wire into swe_calc_r()
 * and friends, and a fuzz there under UBSan found two sites in this library:
 * ipl * 100 overflowing int32 in swe_calc()'s center-of-body setup, and a
 * double-to-int overflow in get_new_segment(). The second was not about
 * hostile input at all -- a legal sequence (Sun under Swiss, SE_ECL_NUT under
 * Moshier, Mars under Swiss) wiped the open files' constants, and the
 * segment lookup divided by zero. A fuzz of this file's own found a third,
 * swe_pheno(SE_ECL_NUT) indexing its tables with -1.
 *
 * None of it shows in a plain build's numbers reliably, so this is built
 * with -fsanitize=undefined and -fno-sanitize-recover: any report is a
 * nonzero exit. Two passes -- every entry point over a grid of edge ids and
 * flag set, then a fixed-seed random walk over ids, flags and dates, which
 * is what reaches order-dependent defects like the second (found at call 123
 * of such a walk, and now also spelled out first). Deterministic: the seed
 * is fixed, so a failure reproduces. The gate runs 2000 random calls, about
 * 3 s; pass a larger count to fuzz for longer (400000 were clean).
 *
 *   hostile <ephe-dir> [random-calls]
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "swephexp.h"

static const int ids[] = {
  INT_MIN, INT_MIN + 1, -2147474548, -21475000, -1000000, -100, -3, -2, -1,
  0, 1, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 39, 40,
  58, 59, 60, 999, 1000, 8999, 9000, 9099, 9401, 9999, 10000, 10001, 10433,
  99999, 1000000, (INT_MAX - 9099) / 100, (INT_MAX - 9099) / 100 + 1,
  INT_MAX - 10000, INT_MAX
};
#define NIDS ((unsigned) (sizeof ids / sizeof *ids))

static unsigned long long st = 88172645463325252ULL;
static unsigned rnd(void)
{
  st ^= st << 13; st ^= st >> 7; st ^= st << 17;
  return (unsigned) st;
}

static void call(swe_ctx *C, unsigned op, double t, int id, int id2, int fl,
                 int method)
{
  /* swe_pheno() writes 20 values, the orbital elements 50. */
  double x[50], a[50], b[50], c[50], d[50];
  char s[AS_MAXCH];
  switch (op % 8) {
  case 0: swe_calc_r(C, t, id, fl, x, s); break;
  case 1: swe_calc_ut_r(C, t, id, fl, x, s); break;
  case 2: swe_calc_pctr_r(C, t, id, id2, fl, x, s); break;
  case 3: swe_nod_aps_r(C, t, id, fl, method, a, b, c, d, s); break;
  case 4: swe_pheno_r(C, t, id, fl, x, s); break;
  case 5: swe_get_orbital_elements_r(C, t, id, fl, x, s); break;
  case 6: swe_orbit_max_min_true_distance_r(C, t, id, fl, a, b, c, s); break;
  case 7: swe_get_planet_name_r(C, id, s); break;
  }
}

int main(int argc, char **argv)
{
  static const int flags[] = {
    SEFLG_SWIEPH | SEFLG_SPEED, SEFLG_MOSEPH | SEFLG_SPEED,
    SEFLG_SWIEPH | SEFLG_CENTER_BODY | SEFLG_SPEED, SEFLG_SWIEPH | SEFLG_HELCTR,
    SEFLG_SWIEPH | SEFLG_BARYCTR | SEFLG_CENTER_BODY,
    SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED, SEFLG_SWIEPH | SEFLG_SIDEREAL,
    SEFLG_SWIEPH | SEFLG_TEST_PLMOON, SEFLG_MOSEPH | SEFLG_CENTER_BODY
  };
  const char *ephe = argc > 1 ? argv[1] : "../ephe";
  long n = argc > 2 ? atol(argv[2]) : 2000, grid = 0;
  swe_ctx *C = swe_ctx_new();
  swe_set_ephe_path_r(C, (char *) ephe);
  swe_set_topo_r(C, 10, 50, 100);
  /* The order-dependent one, spelled out so it does not rest on the random
   * pass finding it: its segment lookup divided by zero. */
  call(C, 0, 2440000.0, SE_SUN, 0, SEFLG_SWIEPH, 0);
  call(C, 0, 2440000.0, SE_ECL_NUT, 0, SEFLG_MOSEPH, 0);
  call(C, 0, 2451545.0, SE_MARS, 0, SEFLG_SWIEPH | SEFLG_SPEED, 0);
  for (unsigned i = 0; i < NIDS; i++)
    for (unsigned f = 0; f < sizeof flags / sizeof *flags; f++)
      for (unsigned op = 0; op < 8; op++, grid++)
        call(C, op, 2451545.5, ids[i], SE_MARS, flags[f], SE_NODBIT_OSCU);
  /* Ephemeris-flag bits are masked to one Swiss/Moshier pair: JPL would
   * only measure how fast a missing file is reported. */
  for (long k = 0; k < n; k++) {
    int id = (rnd() & 3) ? ids[rnd() % NIDS] : (int) rnd();
    int id2 = ids[rnd() % NIDS];
    int fl = (int) (rnd() & rnd()) & ~SEFLG_JPLEPH;
    if (rnd() & 1) fl &= 0xFFFFFF;
    double t = 2451545.0 + (double) (rnd() % 20000) - 10000;
    call(C, rnd(), t, id, id2, fl, (int) (rnd() % 8));
  }
  swe_ctx_free(C);
  printf("G25: %ld grid calls and %ld random calls, no undefined behaviour\n",
         grid, n);
  return 0;
}
