/* fbshare.c -- G119: swe_set_ephe_fallback() reaches every thread on the
 * default context, as every other setter does, and an explicit context
 * keeps its own.
 *
 * The defect (UPSTREAM-BUGS 122, this fork's): the setting belonged to no
 * configuration group, so it was never published. With SE_EPHE_FALLBACK
 * unset the main thread read 1 after swe_set_ephe_fallback(1) and a second
 * thread 0 -- its Swiss request at a missing file refused where the main
 * thread's answered from Moshier; with the variable set,
 * swe_set_ephe_fallback(0) left the second thread answering from Moshier.
 *
 * The check: a directory of links to the files without sepl_18.se1, so a
 * Swiss Mars at J2000 has no file. Each scenario in a forked, pristine
 * process, SE_EPHE_FALLBACK unset and set to 1, the setting 0 and 1:
 *   - the path and the setting on the main thread, then a second thread;
 *   - a second thread that has computed already, then the setting;
 *   - the setting, swe_close() and the path again, then a second thread:
 *     the main thread keeps its setting (swe_close() keeps it), the new
 *     thread starts from the variable (swe_close() drops the master);
 * the second thread reads the setting back (its first call, and after its
 * calculation) and its Mars is refused (return < 0) with the setting 0 and
 * answered from Moshier (SEFLG_MOSEPH in the return) with 1; the main
 * thread the same. And an explicit context, on a second thread: made
 * before the setting, it keeps the variable's value; made after, it
 * inherits the setting; set to the other value, it keeps that across the
 * main thread setting its own again, and the main thread keeps its own.
 * 12 default-context and 16 explicit-context scenarios.
 *
 * Measured before the fix, 9 of 28 fail: the 4 default-context scenarios
 * whose setting differs from the variable -- the second thread reads the
 * variable (0 for 1, 1 for 0) and refuses or answers from Moshier by it;
 * the 2 contexts made after such a setting, which take the variable too;
 * and 3 first reads on a new thread with the variable set, 0 for 1, the
 * getter reading the context before anything initialised it. The compat
 * build fails the same 9 (the setting is this fork's, outside the guard).
 * With the setting published but swe_close() dropping its ownership, 2
 * fail: the main thread then takes the variable's value back at the next
 * sync. The upstream build has no swe_set_ephe_fallback() and does not
 * build this. Run as
 *   fbshare EPHEDIR
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

#define T0 2451545.0
#define MISSING "sepl_18.se1"

static char DIR_[] = "/tmp/fbshare-XXXXXX";	/* the files but MISSING */
static const char *EPHE;

/* What a reader saw: the setting read back, and Mars refused or not. */
struct seen { int get_first, get_after, ret; };
struct ans {
  struct seen main_, thr;
  int initial_get, initial_ret;	/* the thread before the setting (O_AFTER) */
  int ran;
};

/* the setting before / after the thread; before, then swe_close() and the
 * path again (a setter of another group) */
enum { O_BEFORE, O_AFTER, O_CLOSE, NO };
static const char *ON[NO] = { "set, then a thread", "a thread, then set",
                              "set, close, path, a thread" };
enum { X_OLD, X_NEW, X_OWN, X_NEWOWN, NX };
static const char *XN[NX] = {
  "context made before the setting",
  "context made after the setting",
  "context made before, set to the other",
  "context made after, set to the other" };

static int link_all_but(const char *from, const char *to, const char *skip)
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

static int mars(swe_ctx *c)
{
  double x[6];
  char serr[AS_MAXCH];
  return c ? swe_calc_r(c, T0, SE_MARS, SEFLG_SWIEPH, x, serr)
           : swe_calc(T0, SE_MARS, SEFLG_SWIEPH, x, serr);
}

static int get(swe_ctx *c)
{
  return c ? swe_get_ephe_fallback_r(c) : swe_get_ephe_fallback();
}

static void look(swe_ctx *c, struct seen *s)
{
  s->get_first = get(c);
  s->ret = mars(c);
  s->get_after = get(c);
}

struct job { int order; swe_ctx *c; struct ans *a; pthread_barrier_t *b; };

static void *worker(void *arg)
{
  struct job *j = arg;
  if (j->order == O_AFTER) {
    j->a->initial_ret = mars(NULL);
    j->a->initial_get = get(NULL);
    pthread_barrier_wait(j->b);	/* computed: now the main thread sets */
    pthread_barrier_wait(j->b);	/* set */
  }
  look(j->c, &j->a->thr);
  return NULL;
}

/* the default context: the path, the setting before or after the thread */
static void scenario(int order, int fb, struct ans *a)
{
  pthread_t th;
  pthread_barrier_t b;
  struct job j;
  j.order = order;
  j.c = NULL;
  j.a = a;
  j.b = &b;
  pthread_barrier_init(&b, NULL, 2);
  swe_set_ephe_path(DIR_);
  if (order != O_AFTER)
    swe_set_ephe_fallback(fb);
  if (order == O_CLOSE) {
    swe_close();
    swe_set_ephe_path(DIR_);
  }
  if (pthread_create(&th, NULL, worker, &j) != 0)
    return;
  if (order == O_AFTER) {
    pthread_barrier_wait(&b);
    swe_set_ephe_fallback(fb);
    pthread_barrier_wait(&b);
  }
  pthread_join(th, NULL);
  look(NULL, &a->main_);
  a->ran = 1;
}

/* an explicit context, used on a second thread */
static void xscenario(int x, int fb, struct ans *a)
{
  pthread_t th;
  struct job j;
  swe_ctx *c = NULL;
  swe_set_ephe_path(DIR_);
  if (x == X_OLD || x == X_OWN)
    c = swe_ctx_new();
  swe_set_ephe_fallback(fb);
  if (c == NULL)
    c = swe_ctx_new();
  swe_set_ephe_path_r(c, DIR_);
  if (x == X_OWN || x == X_NEWOWN) {
    swe_set_ephe_fallback_r(c, !fb);
    swe_set_ephe_fallback(fb);	/* the main thread's again: not c's */
  }
  j.order = O_BEFORE;
  j.c = c;
  j.a = a;
  j.b = NULL;
  if (pthread_create(&th, NULL, worker, &j) != 0)
    return;
  pthread_join(th, NULL);
  look(NULL, &a->main_);
  swe_ctx_free(c);
  a->ran = 1;
}

static int in_child(int env, int kind, int which, int fb, struct ans *out)
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
    if (env)
      setenv("SE_EPHE_FALLBACK", "1", 1);
    else
      unsetenv("SE_EPHE_FALLBACK");
    if (kind == 0)
      scenario(which, fb, &a);
    else
      xscenario(which, fb, &a);
    _exit(write(fd[1], &a, sizeof a) == (ssize_t) sizeof a ? 0 : 3);
  }
  close(fd[1]);
  memset(out, 0, sizeof *out);
  while (got < (ssize_t) sizeof *out
         && (r = read(fd[0], (char *) out + got, sizeof *out - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return (got == (ssize_t) sizeof *out && out->ran) ? 0 : -1;
}

/* does a reader that should hold `want` hold it, and act on it? */
static int wrong(const struct seen *s, int want)
{
  int moshier = s->ret >= 0 && (s->ret & SEFLG_MOSEPH);
  int refused = s->ret < 0;
  return s->get_first != want || s->get_after != want
         || (want ? !moshier : !refused);
}

static void show(const char *who, const struct seen *s)
{
  printf("        %s: reads %d, %d after; Mars returns %d%s\n", who,
         s->get_first, s->get_after, s->ret,
         s->ret < 0 ? " (refused)" : (s->ret & SEFLG_MOSEPH) ? " (Moshier)" : "");
}

int main(int argc, char **argv)
{
  int env, o, x, fb, n = 0, nfail = 0;
  char p[PATH_MAX + 300];
  EPHE = argc > 1 ? argv[1] : "../ephe";
  snprintf(p, sizeof p, "%s/%s", EPHE, MISSING);
  if (access(p, R_OK) != 0) {
    printf("G119 FAIL: %s not found -- the check needs it to leave it out\n", p);
    return 1;
  }
  if (mkdtemp(DIR_) == NULL || link_all_but(EPHE, DIR_, MISSING) != 0) {
    perror("fbshare: links");
    return 2;
  }
  for (env = 0; env <= 1; env++) {
    printf("  SE_EPHE_FALLBACK %s\n", env ? "=1" : "unset");
    for (fb = 0; fb <= 1; fb++) {
      for (o = 0; o < NO; o++, n++) {
        struct ans a;
        int bad;
        if (in_child(env, 0, o, fb, &a) != 0) {
          printf("    %d, %-26s did not run\n", fb, ON[o]);
          nfail++;
          continue;
        }
        /* after swe_close() the master is the defaults again, so a new
         * thread starts from the variable; this one keeps its setting */
        bad = wrong(&a.thr, o == O_CLOSE ? env : fb) || wrong(&a.main_, fb)
              || (o == O_AFTER && (a.initial_get != env
                                   || (env ? a.initial_ret < 0 : a.initial_ret >= 0)));
        if (bad) {
          printf("    %d, %-26s FAIL\n", fb, ON[o]);
          if (o == O_AFTER)
            printf("        thread before: reads %d; Mars returns %d\n",
                   a.initial_get, a.initial_ret);
          show("thread", &a.thr);
          show("main  ", &a.main_);
          nfail++;
        }
      }
      for (x = 0; x < NX; x++, n++) {
        struct ans a;
        int want;
        if (in_child(env, 1, x, fb, &a) != 0) {
          printf("    %d, %-38s did not run\n", fb, XN[x]);
          nfail++;
          continue;
        }
        want = x == X_OLD ? env : x == X_NEW ? fb : !fb;
        if (wrong(&a.thr, want) || wrong(&a.main_, fb)) {
          printf("    %d, %-38s FAIL (context should hold %d)\n", fb, XN[x], want);
          show("context", &a.thr);
          show("main   ", &a.main_);
          nfail++;
        }
      }
    }
  }
  unlink_all(DIR_);
  if (nfail)
    printf("G119 FAIL: %d of %d\n", nfail, n);
  else
    printf("G119 PASS (%d)\n", n);
  return nfail != 0;
}
