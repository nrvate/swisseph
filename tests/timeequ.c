/* timeequ.c -- G128: swe_time_equ(), swe_lmt_to_lat() and swe_lat_to_lmt()
 * answer from the JPL file the configuration sets, whoever opened it.
 *
 * The defect (UPSTREAM-BUGS 132): the three take no ephemeris flag, and
 * swe_time_equ() computed its Sun with SEFLG_JPLEPH only when THIS context
 * had the JPL file open -- the test bug 103 replaced in
 * swi_guess_ephe_flag(), which swe_deltat() and swe_get_ayanamsa_ut() use
 * for the same choice. So after swe_set_jpl_file("de200.eph") on the main
 * thread a second thread, a thread that had computed before the setter and
 * a context made after it answered from the Swiss files while their own
 * swe_deltat() answered for de200; and after swe_close(), where the guess
 * is Swiss (the close forgets the DE numbers, F2), a JPLEPH calculation,
 * which reopens the file by its kept name, made them answer JPL.
 *
 * The check: each case in its own process, the three functions at J2000 +
 * 6h and at 1800 Jan 1 (E, LAT - LMT and LMT - LAT at 8.55 E, with their
 * return codes), bit for bit against a fresh process given the same
 * settings, which opened the file itself and calls on its main thread:
 *
 *   thread     the path and de200 on the main thread; a new thread calls
 *   early      a thread computes on the path alone, the main thread then
 *              sets de200, and the thread calls again
 *   ctx        the path and de200; swe_ctx_new() and the _r() calls on it
 *   reset      the path and de200, a call, swe_close(), the path and de200
 *              again; a new thread calls
 *   closecalc  the path and de200, swe_close(), a JPLEPH Sun; the main
 *              thread calls -- against the same without the JPLEPH Sun:
 *              the guess after the close is Swiss (swe_deltat()'s too),
 *              and a calculation does not choose for it
 *
 * Measured: 0 in every case. Before the fix the default build fails the
 * five: thread, early, ctx and reset answer from the Swiss files, E 1.19e-8
 * day (1.03 ms) off at J2000 and 1.17e-7 day in 1800, closecalc from de200,
 * 1.19e-8 day. The upstream build (SWE_UPSTREAM_COMPAT, upstream's test of
 * the open file) fails the same five by the same amounts. Run as
 *   timeequ EPHEDIR
 */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

#define NV 12
static const double T[2] = {2451545.25, 2378496.5};	/* J2000 + 6h, 1800 Jan 1 */
static const double LON = 8.55;

enum { C_THREAD, C_EARLY, C_CTX, C_RESET, C_CLOSECALC, NCASE };
static const char *cname[NCASE] = {"thread", "early", "ctx", "reset", "closecalc"};
static const char *vname[6] = {"E", "LAT-LMT", "LMT-LAT", "E 1800", "LAT-LMT 1800", "LMT-LAT 1800"};

static const char *EPHE;

struct ans {
  double v[NV];
  int32 r[6];
};

static struct ans out;
static int early;
static pthread_barrier_t b1, b2;

/* the three functions at the two instants, on c (NULL: the default context) */
static void call(swe_ctx *c, struct ans *a)
{
  char serr[AS_MAXCH];
  double e, lat, lmt;
  int k;
  memset(a, 0, sizeof *a);
  for (k = 0; k < 2; k++) {
    e = lat = lmt = 0;
    if (c != NULL) {
      a->r[3 * k] = swe_time_equ_r(c, T[k], &e, serr);
      a->r[3 * k + 1] = swe_lmt_to_lat_r(c, T[k], LON, &lat, serr);
      a->r[3 * k + 2] = swe_lat_to_lmt_r(c, T[k], LON, &lmt, serr);
    } else {
      a->r[3 * k] = swe_time_equ(T[k], &e, serr);
      a->r[3 * k + 1] = swe_lmt_to_lat(T[k], LON, &lat, serr);
      a->r[3 * k + 2] = swe_lat_to_lmt(T[k], LON, &lmt, serr);
    }
    a->v[3 * k] = e;
    a->v[3 * k + 1] = lat - T[k];
    a->v[3 * k + 2] = lmt - T[k];
  }
}

static void *worker(void *arg)
{
  (void) arg;
  if (early) {
    call(NULL, &out);		/* on the path alone */
    pthread_barrier_wait(&b1);
    pthread_barrier_wait(&b2);	/* the main thread has set de200 */
  }
  call(NULL, &out);
  return NULL;
}

static int on_thread(void)
{
  pthread_t th;
  if (pthread_create(&th, NULL, worker, NULL) != 0)
    return -1;
  return pthread_join(th, NULL);
}

static void jpl_sun(void)
{
  double x[6];
  char serr[AS_MAXCH];
  swe_calc(2451545.0, SE_SUN, SEFLG_JPLEPH, x, serr);
}

/* in the child: the case (ref 0) or its reference (ref 1) */
static int scenario(int k, int ref)
{
  swe_ctx *c;
  pthread_t th;
  swe_set_ephe_path(EPHE);
  if (k == C_EARLY && !ref) {
    early = 1;
    if (pthread_barrier_init(&b1, NULL, 2) != 0 || pthread_barrier_init(&b2, NULL, 2) != 0
        || pthread_create(&th, NULL, worker, NULL) != 0)
      return -1;
    pthread_barrier_wait(&b1);
    swe_set_jpl_file("de200.eph");
    pthread_barrier_wait(&b2);
    return pthread_join(th, NULL);
  }
  swe_set_jpl_file("de200.eph");
  if (k == C_CLOSECALC) {
    swe_close();
    if (!ref)
      jpl_sun();
    call(NULL, &out);
    return 0;
  }
  if (ref) {
    call(NULL, &out);
    return 0;
  }
  switch (k) {
  case C_THREAD:
    return on_thread();
  case C_CTX:
    if ((c = swe_ctx_new()) == NULL)
      return -1;
    call(c, &out);
    swe_ctx_free(c);
    return 0;
  default:	/* C_RESET */
    call(NULL, &out);
    swe_close();
    swe_set_ephe_path(EPHE);
    swe_set_jpl_file("de200.eph");
    return on_thread();
  }
}

static int get(int k, int ref, struct ans *a)
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
    close(p[0]);
    alarm(20);			/* a backstop: no case takes a second */
    if (scenario(k, ref) != 0)
      _exit(2);
    if (write(p[1], &out, sizeof out) != (ssize_t) sizeof out)
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
  struct ans t, m;
  double worst = 0;
  int i, at = 0;
  if (get(k, 0, &t) != 0 || get(k, 1, &m) != 0) {
    printf("  %-9s could not run\n", cname[k]);
    return 1;
  }
  for (i = 0; i < 6; i++)
    if (fabs(t.v[i] - m.v[i]) > worst) {
      worst = fabs(t.v[i] - m.v[i]);
      at = i;
    }
  if (memcmp(t.r, m.r, sizeof t.r) != 0)
    printf("  %-9s a return code differs\n", cname[k]);
  if (memcmp(t.v, m.v, sizeof t.v) != 0)
    printf("  %-9s E %.2e day off at J2000 (%.12e, want %.12e), the worst %.2e in %s\n",
           cname[k], fabs(t.v[0] - m.v[0]), t.v[0], m.v[0], worst, vname[at]);
  if (memcmp(t.r, m.r, sizeof t.r) != 0 || memcmp(t.v, m.v, sizeof t.v) != 0)
    return 1;
  printf("  %-9s as the fresh process (E %.12e d, %.12e d in 1800)\n", cname[k], t.v[0], t.v[3]);
  return 0;
}

int main(int argc, char **argv)
{
  int k, bad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: timeequ EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  printf("G128: the equation of time from the JPL file the configuration sets\n");
  for (k = 0; k < NCASE; k++)
    bad += run_case(k);
  if (bad) {
    printf("G128 FAIL: %d\n", bad);
    return 1;
  }
  printf("G128 PASS\n");
  return 0;
}
