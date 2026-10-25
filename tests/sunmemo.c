/* sunmemo.c -- G129: the Sunshine houses' remembered declination survives
 * a context's first start.
 *
 * The defect (UPSTREAM-BUGS 133, this fork's): swe_houses_armc() and
 * swe_houses_armc_ex2() remember the Sun's declination a Sunshine ('I')
 * call is given in ascmc[9], and a later call with 99 there recalls it,
 * as swe_house_pos() always does. They do not start the context, so on a
 * thread's first calls the memo can come before the start; the start's
 * memset (swi_init_swed_if_start()) then wiped it, and left 0 where 99
 * means "none yet". The recall answered with declination 0: the cusps
 * depended on whether a calculation had come between. Upstream's memo is
 * a function-scope static nothing resets; Phase 3c moved it into the
 * context.
 *
 * The check: each case in its own process, at ARMC 123.4, latitude 47.37 N
 * and obliquity 23.44, with 20 deg remembered. The process makes the
 * memo, then its first start, then the recall; the answer must equal, bit
 * for bit, a fresh process's that recalled straight after the memo:
 *
 *   calc    swe_calc_ut() of the Sun on Moshier starts the context;
 *           swe_houses_armc() recalls, the cusps and ascmc[0..9]
 *   path    swe_set_ephe_path() starts it
 *   pos     swe_calc_ut(); swe_house_pos() of a body at 200 deg, 1 deg N
 *   speed   the memo and the recall by swe_houses_armc_ex2() with speeds
 *   thread  calc's three steps on a new thread
 *   marker  no memo: the context after its start reads 99, as one never
 *           started does, not 0
 *
 * Measured: 0 in every case. Before the fix the default build fails all
 * six, the recall taking declination 0 for 20: calc, path and thread 6.71
 * deg in cusp 3 (260.801 for 254.087; cusp 2 228.717 for 223.821), pos
 * 0.0435 of a house (12.7645 for 12.8080), speed the same cusps and 9.53
 * deg/day in cusp 3's speed, marker 0 for 99. The upstream build
 * (SWE_UPSTREAM_COMPAT) failed the same six before the fix and passes now:
 * upstream's static was never reset, so the fix is in both builds. Run as
 *   sunmemo EPHEDIR
 */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"
#include "sweph.h"	/* swi_default_ctx(), for the marker */

#define TJD 2451545.25
#define ARMC 123.4
#define LAT 47.37
#define EPS 23.44
#define DECL 20.0
#define NOUT 48

enum { C_CALC, C_PATH, C_POS, C_SPEED, C_THREAD, C_MARKER, NCASE };
static const char *cname[NCASE] = {"calc", "path", "pos", "speed", "thread", "marker"};

static const char *EPHE;
static int cur, cur_start;

struct ans {
  double v[NOUT];
};
static struct ans thr_ans;

/* the Sunshine call: ascmc[9] = decl, 99 to recall; with speeds for speed */
static void sunshine(int k, double decl, struct ans *a)
{
  double cusp[13], ascmc[10], cs[13], as[10], xpin[2] = {200, 1};
  char serr[AS_MAXCH];
  int i;
  memset(ascmc, 0, sizeof ascmc);
  memset(cs, 0, sizeof cs);
  memset(as, 0, sizeof as);
  ascmc[9] = decl;
  if (k == C_POS && decl == 99) {
    a->v[0] = swe_house_pos(ARMC, LAT, EPS, 'I', xpin, serr);
    return;
  }
  if (k == C_SPEED)
    a->v[0] = swe_houses_armc_ex2(ARMC, LAT, EPS, 'I', cusp, ascmc, cs, as, serr);
  else
    a->v[0] = swe_houses_armc(ARMC, LAT, EPS, 'I', cusp, ascmc);
  for (i = 1; i <= 12; i++) {
    a->v[i] = cusp[i];
    a->v[22 + i] = cs[i];
  }
  for (i = 0; i < 10; i++) {
    a->v[13 + i] = ascmc[i];
    a->v[35 + i] = as[i];
  }
}

/* the context's first start, the case's way */
static void start(int k)
{
  double x[6];
  char serr[AS_MAXCH];
  if (k == C_PATH)
    swe_set_ephe_path(EPHE);
  else
    swe_calc_ut(TJD, SE_SUN, SEFLG_MOSEPH, x, serr);
}

/* the memo, the start if asked, the recall; marker reads the memo itself */
static void steps(int k, int with_start, struct ans *a)
{
  struct ans memo;
  memset(a, 0, sizeof *a);
  if (k == C_MARKER) {
    if (with_start)
      start(k);
    a->v[0] = swi_default_ctx()->saved_sundec;
    return;
  }
  sunshine(k, DECL, &memo);
  if (with_start)
    start(k);
  sunshine(k, 99, a);
}

static void *worker(void *arg)
{
  (void) arg;
  steps(cur, cur_start, &thr_ans);
  return NULL;
}

/* in a child: the steps, with the start or without, here or (thread) on a
 * new thread */
static int get(int k, int with_start, struct ans *a)
{
  int p[2], st;
  pid_t pid;
  ssize_t got = 0, n;
  if (pipe(p) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    pthread_t th;
    close(p[0]);
    alarm(20);			/* a backstop: no case takes a second */
    cur = k;
    cur_start = with_start;
    if (k == C_THREAD && with_start) {
      if (pthread_create(&th, NULL, worker, NULL) != 0)
        _exit(2);
      pthread_join(th, NULL);
    } else
      steps(k, with_start, &thr_ans);
    if (write(p[1], &thr_ans, sizeof thr_ans) != (ssize_t) sizeof thr_ans)
      _exit(2);
    _exit(0);
  }
  close(p[1]);
  while (got < (ssize_t) sizeof *a && (n = read(p[0], (char *) a + got, sizeof *a - got)) > 0)
    got += n;
  close(p[0]);
  waitpid(pid, &st, 0);
  if (!WIFEXITED(st) || WEXITSTATUS(st) != 0 || got != (ssize_t) sizeof *a)
    return -1;
  return 0;
}

static int run_case(int k)
{
  struct ans t, r;
  double worst = 0;
  int i, at = 0;
  if (get(k, 1, &t) != 0 || get(k, 0, &r) != 0) {
    printf("  %-7s could not run\n", cname[k]);
    return 1;
  }
  for (i = 0; i < NOUT; i++)
    if (i != 22 && fabs(t.v[i] - r.v[i]) > worst) {	/* [22]: ascmc[9] */
      worst = fabs(t.v[i] - r.v[i]);
      at = i;
    }
  if (memcmp(t.v, r.v, sizeof t.v) != 0) {
    printf("  %-7s %.3g off in [%d] (%.12g, want %.12g)", cname[k], worst, at, t.v[at],
           r.v[at]);
    if (t.v[22] != r.v[22])
      printf(", the declination %g for %g", t.v[22], r.v[22]);
    printf("\n");
    return 1;
  }
  if (k == C_MARKER)
    printf("  %-7s no memo: %g after the start, as before it\n", cname[k], t.v[0]);
  else if (k == C_POS)
    printf("  %-7s the recall after the start as straight after the memo (house %.12g)\n",
           cname[k], t.v[0]);
  else
    printf("  %-7s the recall after the start as straight after the memo (cusp 2 %.12g)\n",
           cname[k], t.v[2]);
  return 0;
}

int main(int argc, char **argv)
{
  int k, bad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: sunmemo EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  printf("G129: the Sunshine houses' remembered declination survives the first start\n");
  for (k = 0; k < NCASE; k++)
    bad += run_case(k);
  if (bad) {
    printf("G129 FAIL: %d\n", bad);
    return 1;
  }
  printf("G129 PASS\n");
  return 0;
}
