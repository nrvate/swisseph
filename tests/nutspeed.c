/* nutspeed.c -- G46: the default nutation's cost stays within a few times
 * IAU 2000B at any spacing of instants, and the sparse table it runs on
 * matches the series.
 *
 * ts.17 made full IAU 2000A the default and paid for it with a quarter-day
 * grid: the series only at nodes, cached, and a quintic Lagrange polynomial
 * through the six around the instant. Its cost was measured on hourly Moon
 * rows, where consecutive instants share their nodes. An instant a day or
 * more from the last one shares none and evaluated the series six times, so
 * a search stepping by days or a list of unrelated charts ran 25-40 times
 * slower than under 2000B (ts.19, -O2, Sun with speed from the Swiss files:
 * 257 us a call at one-day steps and 404 us scattered, against 10 and 16
 * under ts.14). An astromcp return search went from 0.6 s to 6.7 s.
 *
 * ts.20 evaluates the series by products of e^(ik x) powers over each term's
 * nonzero multipliers (swenut2000a_sparse.h), with its first three
 * derivatives, at nodes a day apart, and interpolates by the septic Hermite
 * polynomial on the two nodes around the instant. A fresh instant still
 * costs a node or two of the full series, which 2000B's 77 terms never
 * did, so the limits are not 1: they are set to catch a return of ts.19's
 * kind (17x daily, 25x scattered at -O0) with room for a loaded machine.
 * Measured at -O2 on a lightly loaded machine: hourly 0.66x, daily 2.6x,
 * scattered 4.4x.
 *
 * The check, part 1: every row of nut_ls_sparse[] and nut_pl_sparse[] holds
 * exactly the nonzero multipliers of the same row of nls[] and npl[], within
 * NUT_LS_KMAX and NUT_PL_KMAX (the generated header against the tables it
 * was generated from; tools/gen_nut_sparse.py).
 *
 * Part 2: a context on the default nutation against one set to
 * SEMOD_NUT_IAU_2000B, calling swe_calc_r for the Sun with speed at the same
 * instants, the two sides' rounds interleaved and each its best of fifteen,
 * every round on a fresh context. Three spacings:
 * hourly (shared nodes), daily, and scattered across 1800-2100. A ratio, not
 * a time, so the machine's speed cancels. Moshier, no files. Built at -O2,
 * as a release is. Run as
 *   nutspeed
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "swephexp.h"
#include "sweph.h"
#include "swenut2000a.h"
#include "swenut2000a_sparse.h"

#define NCALL 1000
#define NROUND 15

static double now(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* The nonzero multipliers of each dense row, in order, and no others. */
static int check_sparse(const char *name, const int16 *dense, int width, int nrow,
                        const struct nut_sparse *sp, int kmax)
{
  int i, j, m, bad = 0;
  for (i = 0; i < nrow; i++) {
    m = 0;
    for (j = 0; j < width; j++) {
      int k = dense[i * width + j];
      if (k == 0)
        continue;
      if (m >= sp[i].n || sp[i].j[m] != j || sp[i].k[m] != k || abs(k) > kmax)
        bad++;
      m++;
    }
    if (m != sp[i].n)
      bad++;
  }
  printf("  %s: %d rows, %d mismatches\n", name, nrow, bad);
  return bad;
}

/* One round: NCALL calls on a fresh context, so no round starts warm. */
static double round_cost(const char *models, const double *tjd)
{
  char serr[256];
  double xx[6], t0;
  int i;
  swe_ctx *ctx = swe_ctx_new();
  if (models != NULL)
    swe_set_astro_models_r(ctx, (char *) models, 0);
  t0 = now();
  for (i = 0; i < NCALL; i++)
    if (swe_calc_r(ctx, tjd[i], SE_SUN, SEFLG_MOSEPH | SEFLG_SPEED, xx, serr) < 0) {
      printf("  ERROR %s\n", serr);
      exit(1);
    }
  t0 = now() - t0;
  swe_ctx_free(ctx);
  return t0 / NCALL * 1e6;
}

/* The two sides' rounds interleave, and each keeps its best: a busy
 * machine (make check runs gates in parallel) slows whichever side it
 * happens to land on, and timing one side's rounds and then the other's
 * let that skew the ratio by itself (5.0x daily on a loaded run, against
 * 2.7x idle). */
static void cost_pair(const double *tjd, double *pd, double *pb)
{
  int r;
  *pd = *pb = 1e30;
  for (r = 0; r < NROUND; r++) {
    double d = round_cost(NULL, tjd), b = round_cost("0,0,0,4", tjd);   /* SEMOD_NUT_IAU_2000B */
    if (d < *pd)
      *pd = d;
    if (b < *pb)
      *pb = b;
  }
}

int main(void)
{
  static const char *rgszSpacing[3] = {"hourly", "daily", "scattered"};
  static const double rgLimit[3] = {1.5, 5.0, 8.0};
  static double tjd[NCALL];
  int s, i, bad = 0;
  printf("G46: the default nutation's cost and its sparse table\n");
  bad += check_sparse("luni-solar", nls, 5, NLS, nut_ls_sparse, NUT_LS_KMAX) != 0;
  bad += check_sparse("planetary", npl, 14, NPL, nut_pl_sparse, NUT_PL_KMAX) != 0;
  srand(20261024);
  for (s = 0; s < 3; s++) {
    double cd, cb;
    for (i = 0; i < NCALL; i++)
      tjd[i] = s == 0 ? 2451545.0 + i / 24.0
             : s == 1 ? 2451545.0 + i
             : 2378496.5 + 109573.0 * (rand() / (double) RAND_MAX);
    cost_pair(tjd, &cd, &cb);
    printf("  %-10s default %7.2f us, 2000B %7.2f us, ratio %5.2f (limit %.1f)\n",
           rgszSpacing[s], cd, cb, cd / cb, rgLimit[s]);
    if (cd / cb > rgLimit[s])
      bad++;
  }
  printf(bad ? "G46 FAIL: %d\n" : "G46 PASS\n", bad);
  return bad ? 1 : 0;
}
