/* bench.c -- wall-clock benchmark for notes/C17_PERFORMANCE.md.
 *
 * That plan is the first on this branch to claim SPEED. Every other gate
 * proves "didn't break it": tests/golden is a numerical no-op check. It
 * cannot tell whether a change helped, hurt, or did nothing at all, and
 * "restrict should help here" is exactly the kind of claim that turns out
 * to be worth 0% once measured.
 *
 * So: record a baseline BEFORE landing anything, and put a real number in
 * each commit -- including when the number does not move.
 *
 * Workloads target the specific findings:
 *
 *   calc      swe_calc over the planets, Moshier and SWIEPH. The general
 *             hot path: swi_coortrf2, swi_cartpol/polcart, embofs.
 *   jpl       the same restricted to bodies that read .se1 files, which is
 *             what actually exercises interp() -- the plan's headline
 *             restrict candidate (C17_PERFORMANCE.md section 4.1).
 *   moon      swe_calc on the Moon alone: swemmoon.c, the largest single
 *             numerical routine in the library.
 *   houses    swe_houses, which is trigonometry rather than ephemeris
 *             reading -- a control workload that the section 4.1 changes
 *             should NOT move. If it moves, something else changed.
 *
 * Reports the MEDIAN of N repetitions, not the mean: one descheduled run
 * skews a mean and hides a real regression. Also reports min and the
 * spread, because a median is only trustworthy if the spread is small
 * relative to the difference being claimed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "swephexp.h"

#define NREP_DEFAULT 7

/* Iteration counts are sized so each workload runs for roughly half a
 * second. The first version ran each in 3-70 ms with up to 50% spread
 * between fastest and slowest repetition -- useless for detecting the
 * few-percent codegen differences C17_PERFORMANCE.md is about, since the
 * noise was an order of magnitude larger than the effect. Scale with
 * --scale if the machine is much faster or slower. */
static int g_scale = 1;

static const char *g_ephe = "../ephe";

/* CLOCK_MONOTONIC where available; this is wall-clock, so the caller is
 * responsible for a quiet machine. clock() would measure CPU time, which
 * hides nothing useful here and is coarser on some platforms. */
#if defined(_WIN32)
# include <windows.h>
static double now_sec(void)
{
  LARGE_INTEGER f, t;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&t);
  return (double) t.QuadPart / (double) f.QuadPart;
}
#else
static double now_sec(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}
#endif

static int cmp_double(const void *a, const void *b)
{
  double x = *(const double *) a, y = *(const double *) b;
  return (x > y) - (x < y);
}

/* Kept in a global and printed at the end so the optimiser cannot decide
 * the whole workload is dead code. */
static volatile double sink = 0;

/*--------------------------------------------------------------------*/

static void work_calc(int32 epheflag)
{
  char serr[AS_MAXCH];
  double xx[6], tjd;
  int ipl, i;
  for (i = 0; i < 6000 * g_scale; i++) {
    tjd = 2451545.0 + (i % 800) * 37.0;
    for (ipl = SE_SUN; ipl <= SE_PLUTO; ipl++) {
      if (swe_calc(tjd, ipl, epheflag | SEFLG_SPEED, xx, serr) >= 0)
        sink += xx[0];
    }
  }
}

/* Bodies whose positions come out of the .se1 files, i.e. the path that
 * reaches interp(). Moshier would bypass it entirely. */
static void work_jpl(void)
{
  char serr[AS_MAXCH];
  double xx[6], tjd;
  int ipl, i;
  for (i = 0; i < 12000 * g_scale; i++) {
    tjd = 2451545.0 + (i % 900) * 11.0;
    for (ipl = SE_MERCURY; ipl <= SE_PLUTO; ipl++) {
      if (swe_calc(tjd, ipl, SEFLG_SWIEPH | SEFLG_SPEED, xx, serr) >= 0)
        sink += xx[0];
    }
  }
}

static void work_moon(void)
{
  char serr[AS_MAXCH];
  double xx[6];
  int i;
  for (i = 0; i < 24000 * g_scale; i++) {
    if (swe_calc(2451545.0 + i * 0.37, SE_MOON,
                 SEFLG_MOSEPH | SEFLG_SPEED, xx, serr) >= 0)
      sink += xx[0];
  }
}

static void work_houses(void)
{
  double cusp[13], ascmc[10];
  int i;
  for (i = 0; i < 60000 * g_scale; i++) {
    if (swe_houses(2451545.0 + i * 0.37, 51.5 - (i % 90),
                   (double) (i % 360) - 180.0, 'P', cusp, ascmc) == OK)
      sink += cusp[1];
  }
}

static void work_calc_swieph(void) { work_calc(SEFLG_SWIEPH); }
static void work_calc_moseph(void) { work_calc(SEFLG_MOSEPH); }

struct bench { const char *name; void (*fn)(void); const char *note; };

static const struct bench BENCHES[] = {
  { "calc-swieph", work_calc_swieph, "planets from .se1 files"      },
  { "calc-moseph", work_calc_moseph, "planets, Moshier (no file IO)"},
  { "jpl-interp",  work_jpl,         "interp()-heavy (section 4.1)" },
  { "moon",        work_moon,        "swemmoon.c"                   },
  { "houses",      work_houses,      "control: should not move"     },
};
#define NBENCH ((int)(sizeof BENCHES / sizeof BENCHES[0]))

int main(int argc, char **argv)
{
  int nrep = NREP_DEFAULT, i, r, b;
  const char *only = NULL;
  double *t;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--ephe") == 0 && i + 1 < argc) g_ephe = argv[++i];
    else if (strcmp(argv[i], "--rep") == 0 && i + 1 < argc) nrep = atoi(argv[++i]);
    else if (strcmp(argv[i], "--only") == 0 && i + 1 < argc) only = argv[++i];
    else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) g_scale = atoi(argv[++i]);
    else { fprintf(stderr, "usage: %s [--ephe DIR] [--rep N] [--only NAME] [--scale N]\n", argv[0]); return 2; }
  }
  if (nrep < 1) nrep = 1;
  if (g_scale < 1) g_scale = 1;
  t = (double *) malloc(sizeof(double) * nrep);
  if (t == NULL) return 1;

  printf("bench: median of %d runs, seconds (lower is better)\n", nrep);
  printf("  %-13s %9s %9s %8s   %s\n", "workload", "median", "min", "spread", "note");

  for (b = 0; b < NBENCH; b++) {
    if (only != NULL && strcmp(only, BENCHES[b].name) != 0) continue;
    /* Fresh state per workload, and one untimed warm-up run: the first
     * pass pays for opening .se1 files and filling the save areas, which
     * is startup cost, not the thing being compared. */
    swe_close();
    swe_set_ephe_path((char *) g_ephe);
    BENCHES[b].fn();

    for (r = 0; r < nrep; r++) {
      double t0 = now_sec();
      BENCHES[b].fn();
      t[r] = now_sec() - t0;
    }
    qsort(t, nrep, sizeof(double), cmp_double);
    printf("  %-13s %9.4f %9.4f %7.1f%%   %s\n",
           BENCHES[b].name, t[nrep / 2], t[0],
           t[0] > 0 ? 100.0 * (t[nrep - 1] - t[0]) / t[0] : 0.0,
           BENCHES[b].note);
  }
  swe_close();
  free(t);
  /* Printed so the workloads cannot be optimised away entirely. */
  fprintf(stderr, "(checksum %.6f)\n", (double) sink);
  return 0;
}
