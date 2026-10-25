/* moonlesstid.c -- G142: without the moon files, the Swiss files' tidal
 * term is the planet files' DE number's, on every thread and context.
 *
 * The defect (UPSTREAM-BUGS 166): the Swiss files' tidal term came from
 * the moon file's header alone, so a directory with the planet files and
 * no semo_*.se1 took the default term, -25.80 (DE431's), though its planet
 * files are DE441's (-25.936): delta t in AD 1000 1463.430567 s for the
 * complete directory's 1474.717432, in 1900 -1.990764 for -1.953326, and
 * every answer that keeps SEFLG_SWIEPH with them -- the heliocentric Mars
 * by swe_calc_ut() at 1900 2.6e-7 deg off the complete directory's, in AD
 * 1000 5.8e-5, and with the fallback allowed swe_nod_aps_ut()'s mean node
 * and apogee at 1900 2.3e-8 and 4.9e-8 (the Sun 4.4e-7 and, in AD 1000,
 * 1.3e-4). The Moon itself, Moshier's there, answers SEFLG_MOSEPH and
 * takes Moshier's own term (bug 120).
 *
 * The check, each case in a forked, pristine process: a /tmp directory of
 * links to EPHEDIR's files but the moon files (semo*.se1), read by the
 * setting thread, a thread started after the path, a thread running
 * across it, a context made by swe_ctx_new() after it, an explicit context
 * given the path, a first use with SE_EPHE_PATH naming the directory and
 * no setter, and the setting thread after a heliocentric Mars and
 * swe_close() (bug 131's reading again). Under the strict setting:
 * swe_get_tid_acc() first, swe_deltat_ex() in AD 1000 and 1900 with
 * SEFLG_SWIEPH, swe_deltat() in AD 1000, the heliocentric Mars by
 * swe_calc_ut() at 1900 and in AD 1000, and the getter again; with the
 * fallback allowed, swe_nod_aps_ut()'s mean node and apogee of the Moon at
 * 1900 and in AD 1000, and the UT Moon's return flag (SEFLG_MOSEPH: the
 * directory has no moon file). Each bit for bit as the same reader given
 * EPHEDIR, but the Moon's flag (SEFLG_SWIEPH there). 7 readers x 2
 * observations = 14 checks.
 *
 * Measured: the 14 pass. Before the fix all 14 fail, the term -25.80 for
 * -25.936 and the values above; the compat build (SWE_UPSTREAM_COMPAT),
 * which keeps upstream's default term, fails the 14 with the fix and
 * without. The upstream build fails as the compat build on its main
 * thread. Run as
 *   moonlesstid EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T_1900 2415020.5
#define T_1000 2086308.0
#define NV     8

static char EPHE[PATH_MAX], NOMOON[] = "/tmp/moonlesstidXXXXXX";
static const char *DIR_;	/* the directory the reader is given */
static int O;			/* 0: strict, 1: the fallback allowed */

enum { R_MAIN, R_NEW, R_RUN, R_CTXNEW, R_CTX, R_ENV, R_CLOSE, NR };
static const char *RN[NR] = { "setting thread", "new thread", "running thread",
                              "context made after", "explicit context",
                              "SE_EPHE_PATH, no setter", "after swe_close()" };

struct obs { double v[NV]; };
static struct obs out;
static pthread_barrier_t b1, b2;

static int link_moonless(void)
{
  DIR *d = opendir(EPHE);
  struct dirent *e;
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (d == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || strncmp(e->d_name, "semo", 4) == 0)
      continue;
    snprintf(p, sizeof p, "%s/%s", EPHE, e->d_name);
    snprintf(q, sizeof q, "%s/%s", NOMOON, e->d_name);
    if (symlink(p, q) != 0) {
      closedir(d);
      return -1;
    }
  }
  closedir(d);
  return 0;
}

static void unlink_all(const char *dir)
{
  DIR *d = opendir(dir);
  struct dirent *e;
  char p[PATH_MAX + 300];
  if (d == NULL)
    return;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.')
      continue;
    snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
    unlink(p);
  }
  closedir(d);
  rmdir(dir);
}

static void observe(swe_ctx *c, struct obs *a)
{
  double x[6], xn[6], xs[6], xp[6], xa[6];
  char serr[AS_MAXCH];
  memset(a, 0, sizeof *a);
  if (O == 0) {
    a->v[0] = c != NULL ? swe_get_tid_acc_r(c) : swe_get_tid_acc();
    a->v[1] = (c != NULL ? swe_deltat_ex_r(c, T_1000, SEFLG_SWIEPH, NULL)
                         : swe_deltat_ex(T_1000, SEFLG_SWIEPH, NULL)) * 86400;
    a->v[2] = (c != NULL ? swe_deltat_ex_r(c, T_1900, SEFLG_SWIEPH, NULL)
                         : swe_deltat_ex(T_1900, SEFLG_SWIEPH, NULL)) * 86400;
    a->v[3] = (c != NULL ? swe_deltat_r(c, T_1000) : swe_deltat(T_1000)) * 86400;
    if ((c != NULL ? swe_calc_ut_r(c, T_1900, SE_MARS, SEFLG_SWIEPH | SEFLG_HELCTR, x, serr)
                   : swe_calc_ut(T_1900, SE_MARS, SEFLG_SWIEPH | SEFLG_HELCTR, x, serr)) >= 0)
      a->v[4] = x[0];
    if ((c != NULL ? swe_calc_ut_r(c, T_1000, SE_MARS, SEFLG_SWIEPH | SEFLG_HELCTR, x, serr)
                   : swe_calc_ut(T_1000, SE_MARS, SEFLG_SWIEPH | SEFLG_HELCTR, x, serr)) >= 0)
      a->v[5] = x[0];
    a->v[6] = c != NULL ? swe_get_tid_acc_r(c) : swe_get_tid_acc();
    return;
  }
  if (c != NULL)
    swe_set_ephe_fallback_r(c, 1);
  else
    swe_set_ephe_fallback(1);
  if ((c != NULL ? swe_nod_aps_ut_r(c, T_1900, SE_MOON, SEFLG_SWIEPH, SE_NODBIT_MEAN, xn, xs, xp, xa, serr)
                 : swe_nod_aps_ut(T_1900, SE_MOON, SEFLG_SWIEPH, SE_NODBIT_MEAN, xn, xs, xp, xa, serr)) >= 0) {
    a->v[0] = xn[0];
    a->v[1] = xa[0];
  }
  if ((c != NULL ? swe_nod_aps_ut_r(c, T_1000, SE_MOON, SEFLG_SWIEPH, SE_NODBIT_MEAN, xn, xs, xp, xa, serr)
                 : swe_nod_aps_ut(T_1000, SE_MOON, SEFLG_SWIEPH, SE_NODBIT_MEAN, xn, xs, xp, xa, serr)) >= 0) {
    a->v[2] = xn[0];
    a->v[3] = xa[0];
  }
  a->v[4] = c != NULL ? swe_calc_ut_r(c, T_1900, SE_MOON, SEFLG_SWIEPH, x, serr)
                      : swe_calc_ut(T_1900, SE_MOON, SEFLG_SWIEPH, x, serr);
}

static void *starter(void *arg)
{
  (void) arg;
  observe(NULL, &out);
  return NULL;
}

static void *runner(void *arg)
{
  (void) arg;
  (void) swe_get_tid_acc();	/* this thread's context started, synced */
  pthread_barrier_wait(&b1);
  pthread_barrier_wait(&b2);	/* the main thread's path */
  observe(NULL, &out);
  return NULL;
}

static int scenario(int r)
{
  pthread_t th;
  swe_ctx *c;
  double x[6];
  char serr[AS_MAXCH];
  switch (r) {
  case R_MAIN:
    swe_set_ephe_path(DIR_);
    observe(NULL, &out);
    return 0;
  case R_NEW:
    swe_set_ephe_path(DIR_);
    if (pthread_create(&th, NULL, starter, NULL) != 0)
      return -1;
    return pthread_join(th, NULL);
  case R_RUN:
    if (pthread_barrier_init(&b1, NULL, 2) != 0 || pthread_barrier_init(&b2, NULL, 2) != 0
        || pthread_create(&th, NULL, runner, NULL) != 0)
      return -1;
    pthread_barrier_wait(&b1);
    swe_set_ephe_path(DIR_);
    pthread_barrier_wait(&b2);
    return pthread_join(th, NULL);
  case R_CTXNEW:
    swe_set_ephe_path(DIR_);
    if ((c = swe_ctx_new()) == NULL)
      return -1;
    observe(c, &out);
    swe_ctx_free(c);
    return 0;
  case R_CTX:
    if ((c = swe_ctx_new()) == NULL)
      return -1;
    swe_set_ephe_path_r(c, DIR_);
    observe(c, &out);
    swe_ctx_free(c);
    return 0;
  case R_ENV:
    if (setenv("SE_EPHE_PATH", DIR_, 1) != 0)
      return -1;
    observe(NULL, &out);
    return 0;
  default:			/* R_CLOSE */
    swe_set_ephe_path(DIR_);
    swe_calc(T_1900, SE_MARS, SEFLG_SWIEPH | SEFLG_HELCTR, x, serr);
    swe_close();
    observe(NULL, &out);
    return 0;
  }
}

static int get(int r, const char *dir, struct obs *a)
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
    DIR_ = dir;
    if (scenario(r) != 0)
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

int main(int argc, char **argv)
{
  struct obs ref, a;
  int r, i, bad = 0, n = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: moonlesstid EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], EPHE) == NULL) {
    perror(argv[1]);
    return 2;
  }
  if (mkdtemp(NOMOON) == NULL || link_moonless() != 0) {
    printf("G142 FAIL: could not make %s\n", NOMOON);
    unlink_all(NOMOON);
    return 1;
  }
  printf("G142: without the moon files, the Swiss tidal term of the planet files\n");
  for (O = 0; O < 2; O++)
    for (r = 0; r < NR; r++, n++) {
      if (get(r, EPHE, &ref) != 0 || get(r, NOMOON, &a) != 0) {
        printf("  %s, %s: could not run\n", RN[r], O ? "fallback" : "strict");
        bad++;
        continue;
      }
      if (O == 1) {		/* the Moon: Moshier's here, the files' there */
        if ((int) a.v[4] != SEFLG_MOSEPH || (int) ref.v[4] != SEFLG_SWIEPH) {
          printf("  %s, fallback: FAIL: the Moon's flag %d, %d with the files\n",
                 RN[r], (int) a.v[4], (int) ref.v[4]);
          bad++;
          continue;
        }
        a.v[4] = ref.v[4];
      }
      if (memcmp(a.v, ref.v, sizeof a.v) == 0)
        continue;
      printf("  %s, %s: FAIL\n   ", RN[r], O ? "fallback" : "strict");
      for (i = 0; i < NV; i++)
        printf(" %.12g", a.v[i]);
      printf("\n    want");
      for (i = 0; i < NV; i++)
        printf(" %.12g", ref.v[i]);
      printf("\n");
      bad++;
    }
  O = 0;
  if (get(R_MAIN, NOMOON, &a) == 0)
    printf("  without the moon files: tidal term %.4f, delta t in AD 1000 %.6f s,"
           " in 1900 %.6f s\n", a.v[0], a.v[1], a.v[2]);
  unlink_all(NOMOON);
  if (bad) {
    printf("G142 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("G142 PASS (%d checks)\n", n);
  return 0;
}
