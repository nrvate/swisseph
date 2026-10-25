/* filejoin.c -- G88: at the Swiss files' 600-year joins, an answer does not
 * depend on which file an earlier call left open.
 *
 * The defect (UPSTREAM-BUGS 89): the files run past their nominal 600-year
 * ends (sepl_12 to JD 2378575.7, 79 days past 1800.0; seas_12 to
 * 2378857.5, 361 days; every seas file about 350 days, the sepl files 2-89,
 * the semo files 1-30), and the semo files also start 1-27 days before
 * their nominal starts. sweph() kept an open file for any instant inside
 * its data, so a context warm from one side of a join answered just across
 * it from the other side's file, and a fresh one from the file named for
 * the instant: at JD 2378496.6 after a call 1.5 days before, the
 * barycentric Sun 47.6 mas, Juno 8.7 mas, Chiron 0.67 mas geocentric;
 * where the light time crosses the join, speeds up to 4.7e-3 mas/day.
 *
 * At exactly 1800.0 (JD 2378496.5) a fictitious body's heliocentric speed
 * was the older file's in a context that had it open, the newer's in a
 * fresh one: Vulcan 8.2e-7 deg/day apart, at any difference step.
 *
 * Each read now comes from the file whose nominal 600 years hold its own
 * instant; where the light time crosses a join, the read at t from the
 * newer file and the one at t - tau from the older (the sepl and seas files
 * start exactly at the join, so no one file holds both). A position steps
 * by the two files' difference at J and again at J + tau, as it does on a
 * fresh context: Vulcan 0.4e-6 deg there, Juno and the Moon some 4 mas.
 *
 * The checks, bit for bit (memcmp of the six doubles and the return flag),
 * at each of the 49 joins -12600..16800 of sepl, semo and seas (the same
 * instants for the three):
 *   1. at J - 4, J - 2^-7, J, J + 2^-17, J + 2^-7, J + 2^-4, J + 2, J + 20
 *      and J + 200 -- either side of the join and on it, the Moon's light
 *      time (1.3 s) across it at J + 2^-17, the planets' and asteroids' at
 *      J + 2^-7 and J + 2^-4, inside every older file's overrun and the
 *      newer semo's early start -- the Sun, Moon, Mercury, Mars, Jupiter,
 *      Neptune, Pluto, Chiron, Ceres, Juno, Cupido and Vulcan, geocentric,
 *      barycentric, heliocentric and J2000 equatorial XYZ true positions,
 *      all with SEFLG_SPEED: a context warmed 1.5 days on the join's other
 *      side, by the same call or by Mercury, the Moon and Ceres (sepl, semo
 *      and seas open), equals a fresh context;
 *   2. one context stepping 2^-5 day from J - 1/4 to J + 1/4 (the light
 *      time across the join on the steps after it), the barycentric Sun,
 *      the Moon, Jupiter, Pluto, Juno and heliocentric Vulcan, both
 *      directions, at every fourth join: every step equals a fresh context;
 *   and every coordinate is finite.
 * Measured: 42684 rows, none differ. ts20 before the fix fails 30538 (1:
 * 29346, 2: 1192; up to 1.03 arcsec on the barycentric Sun at -7800, 220
 * mas on the Moon, 37 km on Juno); the upstream build fails 30958 (1:
 * 29766, 2: 1192). Run as
 *   filejoin EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static const char *EPHE;
static int nrow, nbad, nnan;
static double worst_mas, worst_m;

static swe_ctx *fresh(void)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, EPHE);
  return c;
}

struct ans { int32 r; double x[6]; };

static struct ans calc(swe_ctx *c, double jd, int ipl, int32 fl)
{
  struct ans a;
  char serr[256];
  memset(&a, 0, sizeof a);
  a.r = swe_calc_r(c, jd, ipl, fl, a.x, serr);
  if (a.r < 0)
    memset(a.x, 0, sizeof a.x);
  return a;
}

static struct ans fresh_calc(double jd, int ipl, int32 fl)
{
  swe_ctx *c = fresh();
  struct ans a = calc(c, jd, ipl, fl);
  swe_ctx_free(c);
  return a;
}

static void same(const char *what, const struct ans *a, const struct ans *b,
                 double jd, int ipl, int32 fl)
{
  int i;
  nrow++;
  for (i = 0; i < 6; i++)
    if (!isfinite(a->x[i]) || !isfinite(b->x[i])) {
      if (nnan++ < 5)
        printf("  NaN: %s, body %d, flags %#x, JD %.8f\n", what, ipl, (unsigned) fl, jd);
      nbad++;
      return;
    }
  if (a->r != b->r || memcmp(a->x, b->x, sizeof a->x) != 0) {
    /* the size, in mas of longitude for angles */
    double d = (fl & SEFLG_XYZ) ? fabs(a->x[0] - b->x[0]) * 1.496e11
                                : fabs(a->x[0] - b->x[0]) * 3.6e6;
    if ((fl & SEFLG_XYZ) && d > worst_m)
      worst_m = d;
    if (!(fl & SEFLG_XYZ) && d > worst_mas)
      worst_mas = d;
    if (nbad < 20)
      printf("  differs: %s, body %d, flags %#x, JD %.8f: %.12f / %.12f (%.3g %s; flags %#x / %#x)\n",
             what, ipl, (unsigned) fl, jd, a->x[0], b->x[0], d,
             (fl & SEFLG_XYZ) ? "m" : "mas", (unsigned) a->r, (unsigned) b->r);
    nbad++;
  }
}

static const int rgBody[] = {
  SE_SUN, SE_MOON, SE_MERCURY, SE_MARS, SE_JUPITER, SE_NEPTUNE, SE_PLUTO,
  SE_CHIRON, SE_CERES, SE_JUNO, SE_CUPIDO, SE_VULCAN
};
static const int32 rgFl[] = {
  SEFLG_SPEED, SEFLG_SPEED | SEFLG_BARYCTR, SEFLG_SPEED | SEFLG_HELCTR,
  SEFLG_SPEED | SEFLG_TRUEPOS | SEFLG_J2000 | SEFLG_NONUT | SEFLG_EQUATORIAL | SEFLG_XYZ
};
#define NBODY ((int) (sizeof rgBody / sizeof rgBody[0]))
#define NFL ((int) (sizeof rgFl / sizeof rgFl[0]))
static const double rgDt[] = {
  -4, -1.0 / 128, 0, 1.0 / 131072, 1.0 / 128, 1.0 / 16, 2, 20, 200
};
#define NDT ((int) (sizeof rgDt / sizeof rgDt[0]))

int main(int argc, char **argv)
{
  double rgJ[64];
  int nj = 0, ij, ib, ifl, id, k, nb1;
  if (argc != 2) {
    fprintf(stderr, "usage: filejoin EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  /* the joins: 1 January of every 600th year, Julian before 1600 as
   * swi_gen_filename() reckons them */
  for (k = -12600; k <= 16800; k += 600)
    rgJ[nj++] = swe_julday(k, 1, 1, 0, k >= 1600 ? SE_GREG_CAL : SE_JUL_CAL);
  printf("G88: answers at the files' 600-year joins independent of the file left open\n");
  /* 1 */
  for (ij = 0; ij < nj; ij++)
    for (id = 0; id < NDT; id++)
      for (ib = 0; ib < NBODY; ib++)
        for (ifl = 0; ifl < NFL; ifl++) {
          double jd = rgJ[ij] + rgDt[id];
          double jw = rgJ[ij] + (rgDt[id] < 0 ? 1.5 : -1.5);
          int ipl = rgBody[ib];
          int32 fl = rgFl[ifl];
          struct ans a = fresh_calc(jd, ipl, fl);
          if (a.r < 0)
            continue;
          for (k = 0; k < 2; k++) {
            swe_ctx *c = fresh();
            struct ans b;
            if (k == 0) {
              calc(c, jw, ipl, fl);
            } else {
              calc(c, jw, SE_MERCURY, SEFLG_SPEED);
              calc(c, jw, SE_MOON, SEFLG_SPEED);
              calc(c, jw, SE_CERES, SEFLG_SPEED);
            }
            b = calc(c, jd, ipl, fl);
            swe_ctx_free(c);
            same(k == 0 ? "warm by itself across the join"
                        : "warm by Mercury, Moon, Ceres across the join",
                 &a, &b, jd, ipl, fl);
          }
        }
  nb1 = nbad;
  printf("  1. fresh against warm from the join's other side: %d rows, %d differ\n", nrow, nb1);
  /* 2 */
  {
    static const struct { int ipl; int32 fl; } rgStep[] = {
      {SE_SUN, SEFLG_SPEED | SEFLG_BARYCTR}, {SE_MOON, SEFLG_SPEED},
      {SE_JUPITER, SEFLG_SPEED}, {SE_PLUTO, SEFLG_SPEED}, {SE_JUNO, SEFLG_SPEED},
      {SE_VULCAN, SEFLG_SPEED | SEFLG_HELCTR}
    };
    int n = (int) (sizeof rgStep / sizeof rgStep[0]), dir, i, nr0 = nrow;
    for (ij = 0; ij < nj; ij += 4)
      for (k = 0; k < n; k++)
        for (dir = 0; dir < 2; dir++) {
          swe_ctx *c = fresh();
          for (i = -8; i <= 8; i++) {
            double jd = rgJ[ij] + (dir ? -i : i) / 32.0;
            struct ans a = fresh_calc(jd, rgStep[k].ipl, rgStep[k].fl);
            struct ans b = calc(c, jd, rgStep[k].ipl, rgStep[k].fl);
            if (a.r < 0 && b.r < 0)
              continue;
            same(dir ? "stepping back across the join" : "stepping across the join",
                 &a, &b, jd, rgStep[k].ipl, rgStep[k].fl);
          }
          swe_ctx_free(c);
        }
    printf("  2. one context stepping across the join: %d rows, %d differ\n",
           nrow - nr0, nbad - nb1);
  }
  printf("  %d rows, %d differ (%d not finite); largest %.3g mas, %.3g m\n", nrow, nbad, nnan,
         worst_mas, worst_m);
  printf(nbad ? "G88 FAIL: %d\n" : "G88 PASS\n", nbad);
  return nbad != 0;
}
