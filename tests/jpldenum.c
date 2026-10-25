/* jpldenum.c -- G132: the JPL DE number behind the automatic tidal term is
 * the one of the files a context has now, on every thread and context.
 *
 * The defect (UPSTREAM-BUGS 137): a thread on the default context that
 * follows the shared configuration adopts another thread's
 * swe_set_ephe_path() in swi_config_apply(). Its branch for a new path
 * closed the JPL file and kept the file's DE number (ctx->jpldenum), which
 * the branch for a new JPL file alone zeroes, and which calc_deltat()
 * takes for every SEFLG_JPLEPH delta t. So a thread that had de200.eph
 * open and adopted another thread's path to a directory without it
 * answered delta t in AD 1700 3.012003 s, de200's term, for the 14.286718
 * s a fresh process with that path gives (the default term, the file being
 * nowhere); with the path again by another string, the file still there
 * but its DE number forgotten by the path setter (the 2026-09-16 review's
 * F4), the same. The setting thread, whose swe_set_ephe_path() zeroes it
 * in swi_close_keep_topo_etc(), answered 14.286718.
 *
 * The defect (UPSTREAM-BUGS 138): a JPL file a calculation opened set the
 * automatic term for itself (open_jpl_file(), upstream's line), while
 * swe_deltat()'s guess follows the JPL file the configuration sets (bug
 * 103). After the path, de200.eph and the path again by another string,
 * which forgets the JPL DE number by design (F4), a TT JPL Sun left
 * de200's -23.8946 in swe_get_tid_acc(), and the next swe_deltat(),
 * guessing the Swiss files, used the moon file's -25.936 (18.902020 s in
 * AD 1800), the getter -25.936 after it: bug 101 has the getter answer
 * the term the next delta t uses. So on every reader.
 *
 * The check, each case in a forked, pristine process: the path and
 * swe_set_jpl_file("de200.eph"); the reader computes a JPL Sun at J2000,
 * which opens the file; then the last setter: none, the path again by
 * another string (EPHEDIR/.), a path without de200.eph (a /tmp directory
 * of links to EPHEDIR's other files), or a JPL file that is not there
 * (de406.eph, the branch for a JPL file alone, as a control). The readers:
 * the setting thread, a thread that computed before the last setter and
 * adopts it, a thread started after it, and an explicit context given the
 * same calls. The observations: swe_deltat_ex() in AD 1700 with
 * SEFLG_JPLEPH (and its serr), swe_get_tid_acc(), swe_deltat() there and
 * swe_get_tid_acc() again (bug 137); and a TT JPL Sun at J2000, then
 * swe_get_tid_acc(), swe_deltat() in AD 1800 and swe_get_tid_acc() again
 * (bug 138). Each bit for bit as a fresh process given the same setters
 * on its main thread and nothing else, no Sun: the guess follows the
 * configuration, and a calculation does not choose for it (as G128's
 * closecalc). 4 settings x 4 readers x 2 observations = 32 checks.
 *
 * Measured before the fixes, 6 of 32 fail. Bug 137's 2: the running
 * thread after the path again and after the path without the file, delta
 * t 3.012003 s for 14.286718. Bug 138's 4: every reader after the path
 * again, the getter -23.8946 for -25.936 before the delta t. The fix of
 * 137 is in both builds (the adoption is this fork's), 138's in the
 * shipped build only. The compat build (SWE_UPSTREAM_COMPAT) fails 14,
 * with the fixes and without, by the upstream behaviour it restores:
 * bug 137's observation 7 -- its running thread's first delta t reads the
 * DE number before the call syncs (bugs 124, 128), 3 cases, and a new
 * thread warns on its first call (bug 110) and guesses from the file it
 * has open (bug 103), 4; with a swe_get_tid_acc() first, which syncs, its
 * running thread answers 14.286718 with the fix and 3.012003 without --
 * and bug 138's 7: after the path again its getter and swe_deltat() both
 * follow the open file (-23.8946, 14.438958 s), 4, and after the path
 * without the file the refused JPL Sun's sync puts the master's -25.80
 * over the moon file's -25.936 on the default context (bug 101's copy of
 * an automatic value), 3. The upstream build (legacy-master) does not
 * build this: its threads share no configuration and adopt nothing. Run as
 *   jpldenum EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T_1700 2341972.5	/* AD 1700, inside de200.eph */
#define T_1800 2378496.5	/* AD 1800 */
#define T_WARM 2451545.0
#define NV     4

static char EPHE[PATH_MAX], SAMEDIR[PATH_MAX + 8], NOFILE[] = "/tmp/jpldenumXXXXXX";

enum { K_SET, K_SAMEDIR, K_NOFILE, K_JPLONLY, NK };
static const char *KN[NK] = {
  "the path and de200.eph", "then the path again (EPHEDIR/.)",
  "then a path without de200.eph", "then a JPL file not there" };
enum { R_MAIN, R_RUN, R_NEW, R_CTX, NR };
static const char *RN[NR] = { "setting thread", "running thread", "new thread",
                              "explicit context" };
enum { O_DT, O_TT, NO };
static const char *ON[NO] = { "JPLEPH delta t, the tidal term, swe_deltat()",
                              "a TT JPL Sun, the tidal term, swe_deltat()" };

struct obs { double v[NV]; char err[AS_MAXCH]; };

static int K, O, REF;
static struct obs out;
static pthread_barrier_t b1, b2;

static int link_all(const char *from, const char *to, const char *skip)
{
  DIR *d = opendir(from);
  struct dirent *e;
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (d == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || strcmp(e->d_name, skip) == 0)
      continue;
    snprintf(p, sizeof p, "%s/%s", from, e->d_name);
    snprintf(q, sizeof q, "%s/%s", to, e->d_name);
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

static void set_path(swe_ctx *c, const char *p)
{
  if (c != NULL)
    swe_set_ephe_path_r(c, p);
  else
    swe_set_ephe_path(p);
}

static void set_jpl(swe_ctx *c, const char *f)
{
  if (c != NULL)
    swe_set_jpl_file_r(c, f);
  else
    swe_set_jpl_file(f);
}

static void first_setters(swe_ctx *c)
{
  set_path(c, EPHE);
  set_jpl(c, "de200.eph");
}

static void last_setter(swe_ctx *c)
{
  switch (K) {
  case K_SAMEDIR: set_path(c, SAMEDIR); break;
  case K_NOFILE: set_path(c, NOFILE); break;
  case K_JPLONLY: set_jpl(c, "de406.eph"); break;
  default: break;
  }
}

static void jpl_sun(swe_ctx *c)
{
  double x[6];
  char serr[AS_MAXCH];
  if (c != NULL)
    swe_calc_r(c, T_WARM, SE_SUN, SEFLG_JPLEPH, x, serr);
  else
    swe_calc(T_WARM, SE_SUN, SEFLG_JPLEPH, x, serr);
}

static double tid(swe_ctx *c)
{
  return c != NULL ? swe_get_tid_acc_r(c) : swe_get_tid_acc();
}

static double dt(swe_ctx *c, double t)
{
  return (c != NULL ? swe_deltat_r(c, t) : swe_deltat(t)) * 86400;
}

static void observe(swe_ctx *c, struct obs *a)
{
  memset(a, 0, sizeof *a);
  if (O == O_DT) {
    a->v[0] = (c != NULL ? swe_deltat_ex_r(c, T_1700, SEFLG_JPLEPH, a->err)
                         : swe_deltat_ex(T_1700, SEFLG_JPLEPH, a->err)) * 86400;
    a->v[1] = tid(c);
    a->v[2] = dt(c, T_1700);
    a->v[3] = tid(c);
    return;
  }
  /* bug 138: a calculation that opens the file does not choose the term
   * the getter and swe_deltat() take; the reference makes none */
  if (!REF)
    jpl_sun(c);
  a->v[0] = tid(c);
  a->v[1] = dt(c, T_1800);
  a->v[2] = tid(c);
}

static void *runner(void *arg)
{
  (void) arg;
  jpl_sun(NULL);		/* the file open on this thread */
  pthread_barrier_wait(&b1);
  pthread_barrier_wait(&b2);	/* the main thread's last setter */
  observe(NULL, &out);
  return NULL;
}

static void *starter(void *arg)
{
  (void) arg;
  observe(NULL, &out);
  return NULL;
}

/* in the child: reader r, or the reference (r < 0) */
static int scenario(int r)
{
  pthread_t th;
  swe_ctx *c;
  switch (r) {
  case R_MAIN:
    first_setters(NULL);
    jpl_sun(NULL);
    last_setter(NULL);
    observe(NULL, &out);
    return 0;
  case R_RUN:
    first_setters(NULL);
    if (pthread_barrier_init(&b1, NULL, 2) != 0 || pthread_barrier_init(&b2, NULL, 2) != 0
        || pthread_create(&th, NULL, runner, NULL) != 0)
      return -1;
    pthread_barrier_wait(&b1);
    last_setter(NULL);
    pthread_barrier_wait(&b2);
    return pthread_join(th, NULL);
  case R_NEW:
    first_setters(NULL);
    last_setter(NULL);
    if (pthread_create(&th, NULL, starter, NULL) != 0)
      return -1;
    return pthread_join(th, NULL);
  case R_CTX:
    if ((c = swe_ctx_new()) == NULL)
      return -1;
    first_setters(c);
    jpl_sun(c);
    last_setter(c);
    observe(c, &out);
    swe_ctx_free(c);
    return 0;
  default:			/* the reference: the setters alone */
    first_setters(NULL);
    last_setter(NULL);
    observe(NULL, &out);
    return 0;
  }
}

static int get(int r, struct obs *a)
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
    REF = r < 0;
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
    fprintf(stderr, "usage: jpldenum EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], EPHE) == NULL) {
    perror(argv[1]);
    return 2;
  }
  snprintf(SAMEDIR, sizeof SAMEDIR, "%s/.", EPHE);
  if (mkdtemp(NOFILE) == NULL || link_all(EPHE, NOFILE, "de200.eph") != 0) {
    printf("G132 FAIL: could not make %s\n", NOFILE);
    unlink_all(NOFILE);
    return 1;
  }
  printf("G132: the JPL DE number behind the automatic tidal term\n");
  for (O = 0; O < NO; O++)
    for (K = 0; K < NK; K++) {
      if (get(-1, &ref) != 0) {
        printf("  %s, %s: the reference could not run\n", KN[K], ON[O]);
        bad++;
        continue;
      }
      for (r = 0; r < NR; r++, n++) {
        if (get(r, &a) != 0) {
          printf("  %s, %s, %s: could not run\n", KN[K], ON[O], RN[r]);
          bad++;
          continue;
        }
        if (memcmp(a.v, ref.v, sizeof a.v) == 0 && strcmp(a.err, ref.err) == 0)
          continue;
        printf("  %s, %s, %s: FAIL\n   ", KN[K], ON[O], RN[r]);
        for (i = 0; i < NV; i++)
          printf(" %.6f", a.v[i]);
        printf(" [%s]\n    want", a.err);
        for (i = 0; i < NV; i++)
          printf(" %.6f", ref.v[i]);
        printf(" [%s]\n", ref.err);
        bad++;
      }
      if (O == O_DT)
        printf("  %-34s JPLEPH delta t %.6f s, swe_deltat() %.6f s, tidal term %.4f\n",
               KN[K], ref.v[0], ref.v[2], ref.v[1]);
      else
        printf("  %-34s after a JPL Sun: tidal term %.4f, swe_deltat() in 1800 %.6f s\n",
               KN[K], ref.v[0], ref.v[1]);
    }
  unlink_all(NOFILE);
  if (bad) {
    printf("G132 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("G132 PASS (%d checks)\n", n);
  return 0;
}
