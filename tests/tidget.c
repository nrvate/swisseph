/* tidget.c -- G100: swe_get_tid_acc() answers with the tidal acceleration
 * the next delta t uses, whenever it is asked.
 *
 * The defects (UPSTREAM-BUGS 101):
 *
 * 1. swe_set_tid_acc(SE_TIDAL_AUTOMATIC) set SE_TIDAL_DEFAULT (-25.80),
 *    which only the next delta t replaced with the ephemeris's value, so
 *    swe_get_tid_acc() straight after it answered -25.80 where a fresh
 *    path answers the moon file's -25.936 (after swe_set_jpl_file(de200)
 *    -23.8946), and after a Moshier calculation -25.80 for -25.936.
 * 2. swe_get_tid_acc() did not initialise or sync the context: as the
 *    first call it answered 0, on a second thread 0 for the value the
 *    first had set (DE406's -25.826 or the files' -25.936).
 * 3. This fork's: the shared configuration compared and copied an
 *    automatic tidal acceleration like a manual one, so a sync put the
 *    master's -25.80 over the value a context had derived from its files
 *    (a context from swe_ctx_new() -25.80 for -25.936), and a context
 *    adopting another's path kept the old path's value.
 *
 * The check, each scenario in a forked, pristine process: swe_get_tid_acc()
 * read before anything else, then swe_deltat() in AD 1000, then
 * swe_get_tid_acc() again; the two reads agree (within 1e-12), and where a
 * reference scenario is named, both values agree with its. Scenarios:
 * after a path; after a path and the de200 file; automatic after a manual
 * DE406, with and without the de200 file; automatic after a Moshier
 * calculation; as the very first call; on a second thread after the first
 * set a path, and a path and DE406; on a context from swe_ctx_new() after
 * a path. After the fix all 9 pass.
 *
 * Measured before it, 7 of 9 fail: automatic after DE406 (-25.80 then
 * -25.936; with de200 -25.80 then -23.8946), after Moshier (-25.80), the
 * first call (0 then -25.80), the second thread (0 then -25.936, and 0 for
 * DE406's -25.826), swe_ctx_new() (-25.80 then -25.936). The compat build
 * fails the same 7, the upstream build (legacy-master) 6 of 8 (it has no
 * swe_ctx_new()). Run as
 *   tidget EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include "swephexp.h"

#define TOL 1e-12
#define T_1000 2086302.5

static const char *EPHE;
struct ans { double g1, dt, g2; };

static void observe(struct ans *a)
{
  a->g1 = swe_get_tid_acc();
  a->dt = swe_deltat(T_1000) * 86400;
  a->g2 = swe_get_tid_acc();
}

static struct ans *thread_out;
static void *worker(void *arg)
{
  (void) arg;
  observe(thread_out);
  return NULL;
}

enum { S_PATH, S_JPL, S_AUTO, S_AUTOJPL, S_AUTOMOSH, S_FIRST, S_THREAD, S_THREAD406,
#ifndef UPSTREAM_API
       S_CTXNEW,
#endif
       NS };
static const struct { int ref; const char *what; } S[] = {
  { -1,     "after a path" },
  { -1,     "after a path and the de200 file" },
  { S_PATH, "path, DE406, automatic" },
  { S_JPL,  "path, de200 file, DE406, automatic" },
  { S_PATH, "path, Moshier calc, automatic" },
  { -1,     "the very first call" },
  { S_PATH, "second thread, after a path" },
  { -1,     "second thread, after a path and DE406" },
  { S_PATH, "swe_ctx_new() after a path" },
};

static void scenario(int k, struct ans *a)
{
  double x[6];
  char serr[AS_MAXCH];
  pthread_t th;
  if (k != S_FIRST)
    swe_set_ephe_path(EPHE);
  switch (k) {
  case S_JPL:
    swe_set_jpl_file("de200.eph");
    break;
  case S_AUTO:
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_set_tid_acc(SE_TIDAL_AUTOMATIC);
    break;
  case S_AUTOJPL:
    swe_set_jpl_file("de200.eph");
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_set_tid_acc(SE_TIDAL_AUTOMATIC);
    break;
  case S_AUTOMOSH:
    swe_calc_ut(2451545.0, SE_SUN, SEFLG_MOSEPH, x, serr);
    swe_set_tid_acc(SE_TIDAL_AUTOMATIC);
    break;
  case S_THREAD406:
    swe_set_tid_acc(SE_TIDAL_DE406);
    /* fall through */
  case S_THREAD:
    thread_out = a;
    if (pthread_create(&th, NULL, worker, NULL) == 0)
      pthread_join(th, NULL);
    return;
#ifndef UPSTREAM_API
  case S_CTXNEW: {
    swe_ctx *c = swe_ctx_new();
    a->g1 = swe_get_tid_acc_r(c);
    a->dt = swe_deltat_r(c, T_1000) * 86400;
    a->g2 = swe_get_tid_acc_r(c);
    swe_ctx_free(c);
    return;
  }
#endif
  }
  observe(a);
}

static int in_child(int k, struct ans *out)
{
  int fd[2];
  ssize_t got = 0, r;
  pid_t pid;
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    struct ans a;
    close(fd[0]);
    memset(&a, 0, sizeof a);
    scenario(k, &a);
    _exit(write(fd[1], &a, sizeof a) == (ssize_t) sizeof a ? 0 : 3);
  }
  close(fd[1]);
  while (got < (ssize_t) sizeof *out && (r = read(fd[0], (char *) out + got, sizeof *out - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return got == (ssize_t) sizeof *out ? 0 : -1;
}

int main(int argc, char **argv)
{
  struct ans a[NS];
  int k, nfail = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  for (k = 0; k < NS; k++)
    if (in_child(k, &a[k]) != 0) {
      printf("G100 FAIL: scenario %d did not run\n", k);
      return 1;
    }
  for (k = 0; k < NS; k++) {
    int r = S[k].ref;
    if (fabs(a[k].g1 - a[k].g2) > TOL) {
      printf("  %-40s FAIL: %.4f, after a delta t %.4f\n", S[k].what, a[k].g1, a[k].g2);
      nfail++;
    } else if (r >= 0 && (fabs(a[k].g1 - a[r].g1) > TOL || fabs(a[k].dt - a[r].dt) > 1e-9)) {
      printf("  %-40s FAIL: %.4f and %.3f s, %s %.4f and %.3f s\n", S[k].what,
             a[k].g1, a[k].dt, S[r].what, a[r].g1, a[r].dt);
      nfail++;
    } else if (k == S_THREAD406 && a[k].g1 != SE_TIDAL_DE406) {
      printf("  %-40s FAIL: %.4f for %.4f\n", S[k].what, a[k].g1, SE_TIDAL_DE406);
      nfail++;
    }
  }
  if (nfail)
    printf("G100 FAIL: %d of %d\n", nfail, NS);
  else
    printf("G100 PASS (%d scenarios)\n", NS);
  return nfail != 0;
}
