/* pathset.c -- G115: the path and JPL setters leave no file open behind
 * them, open nothing outside the path they set, resolve the automatic
 * tidal acceleration for the state they put in place, and see a moon file
 * installed since the same path was last set (UPSTREAM-BUGS 118).
 *
 * The defects, each in a child process, the default path's "." a /tmp
 * directory of links to EPHEDIR's files (cwd), the path set EPHEDIR:
 *   P1  the first swe_set_ephe_path(): the tidal term the context's
 *       initialisation resolved named the default path and opened
 *       ./semo_18.se1, and the setter then overwrote that FILE: 2 ephemeris
 *       descriptors after it, 1 after swe_close() -- want 1 and 0;
 *   P2  100 x (swe_ctx_new, swe_set_ephe_path_r, swe_ctx_free): 100
 *       descriptors left -- want 0;
 *   P3  a thread's first Moon after the main thread's path: it opened
 *       ./semo_18.se1 before adopting the path -- want no open outside
 *       EPHEDIR in the whole process;
 *   P4  the path, then de200.eph, then a path without semo_18:
 *       swe_get_tid_acc() -23.8946 (the dropped JPL file's) for -25.80, on
 *       the setting thread; it and delta t in AD 1000 must be a fresh
 *       process's with that path alone, on a second thread too;
 *   P5  the path, de200.eph, then a JPL file that is not there: -23.8946
 *       for -25.936, against a fresh process with the path alone;
 *   P6  a path without semo_18, the file then linked in and the same path
 *       set again: delta t in AD 1000 1463.44 s (the default term) until a
 *       Moon was computed, then 1474.72 -- want a fresh process's with the
 *       file there at once, the tidal term too, unchanged by a Moon;
 *   P7  the same path again, nothing changed: no fopen() at all (G102's);
 *   P8  the same path again, no moon file there before or now: nothing
 *       opened;
 *   P9  no path, cwd without semo_18, a first Mars: swi_ephe_path_none()
 *       opened sepl_18.se1 to see it was there, and the calculation opened
 *       it again: 2 opens -- want 1.
 * fopen() is interposed and counts calls and successful opens. Measured:
 * the 9 pass. Before the fix 7 fail (P7, P8 pass). Each defect was in this
 * fork's code (bugs 101, 103, 107, 110): the compat build passes P1-P3, P6,
 * P8, P9 and fails P4, P5, P7 by what it restores of upstream (threads that
 * share no configuration, an automatic term of -25.80 until the next delta
 * t, the same path reopening every file), and G24's path_setter_118
 * asserts both builds clean of P2 and P6.
 * Run as
 *   pathset EPHEDIR
 */
#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

static char EPHE[PATH_MAX];
static int ncall, nok, noutside, nsepl;

FILE *fopen(const char *p, const char *m)
{
  static FILE *(*real)(const char *, const char *);
  FILE *f;
  if (real == NULL)
    *(void **) &real = dlsym(RTLD_NEXT, "fopen");
  f = real(p, m);
  __sync_fetch_and_add(&ncall, 1);
  if (f != NULL && (strstr(p, ".se1") != NULL || strstr(p, ".eph") != NULL)) {
    __sync_fetch_and_add(&nok, 1);
    if (strncmp(p, EPHE, strlen(EPHE)) != 0)
      __sync_fetch_and_add(&noutside, 1);
    if (strstr(p, "sepl_18.se1") != NULL)
      __sync_fetch_and_add(&nsepl, 1);
  }
  return f;
}

static void zero(void)
{
  ncall = nok = noutside = nsepl = 0;
}

/* ephemeris files open in this process */
static int nfd(void)
{
  DIR *d = opendir("/proc/self/fd");
  struct dirent *e;
  char p[300], l[PATH_MAX];
  int n = 0;
  if (d == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    ssize_t k;
    if (e->d_name[0] == '.')
      continue;
    snprintf(p, sizeof p, "/proc/self/fd/%s", e->d_name);
    if ((k = readlink(p, l, sizeof l - 1)) < 0)
      continue;
    l[k] = '\0';
    if (strstr(l, ".se1") != NULL || strstr(l, ".eph") != NULL)
      n++;
  }
  closedir(d);
  return n;
}

static int link_all(const char *to, const char *skip)
{
  DIR *d = opendir(EPHE);
  struct dirent *e;
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (d == NULL || mkdir(to, 0700) != 0)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || (skip != NULL && strcmp(e->d_name, skip) == 0))
      continue;
    snprintf(p, sizeof p, "%s/%s", EPHE, e->d_name);
    snprintf(q, sizeof q, "%s/%s", to, e->d_name);
    if (symlink(p, q) != 0) {
      closedir(d);
      return -1;
    }
  }
  closedir(d);
  return 0;
}

static void unlink_dir(const char *dir)
{
  DIR *d = opendir(dir);
  struct dirent *e;
  char p[PATH_MAX + 300];
  if (d == NULL)
    return;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.') {
      snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
      unlink(p);
    }
  closedir(d);
  rmdir(dir);
}

static char TOP[] = "/tmp/pathsetXXXXXX";
static char DALL[PATH_MAX + 20], DNOMOON[PATH_MAX + 20], DLATE[PATH_MAX + 20];
#define T1000 2086302.5	/* AD 1000 Jan 1, Julian */

struct tdt { double tid, dt; };

static struct tdt reading(void)
{
  struct tdt r;
  r.tid = swe_get_tid_acc();
  r.dt = swe_deltat(T1000) * 86400;
  return r;
}

static void *thr_reading(void *a)
{
  *(struct tdt *) a = reading();
  return NULL;
}

static struct tdt worker_reading(void)
{
  pthread_t th;
  struct tdt r = {0, 0};
  pthread_create(&th, NULL, thr_reading, &r);
  pthread_join(th, NULL);
  return r;
}

static int same(const char *what, struct tdt a, struct tdt b)
{
  if (a.tid == b.tid && a.dt == b.dt)
    return 0;
  printf("    %s: tid %.4f, delta t %.6f s; want %.4f, %.6f\n", what, a.tid, a.dt, b.tid, b.dt);
  return 1;
}

/* a fresh process's reading with only this path set */
static struct tdt fresh(const char *path)
{
  int fd[2], st;
  struct tdt r = {0, 0};
  pid_t pid;
  if (pipe(fd) != 0)
    return r;
  if ((pid = fork()) == 0) {
    struct tdt v;
    swe_set_ephe_path(path);
    v = reading();
    if (write(fd[1], &v, sizeof v) != (ssize_t) sizeof v)
      _exit(1);
    _exit(0);
  }
  if (read(fd[0], &r, sizeof r) != (ssize_t) sizeof r)
    r.tid = r.dt = -1;
  waitpid(pid, &st, 0);
  close(fd[0]);
  close(fd[1]);
  return r;
}

static int p1(void)
{
  int a, b;
  swe_set_ephe_path(EPHE);
  a = nfd();
  swe_close();
  b = nfd();
  if (a == 1 && b == 0)
    return 0;
  printf("    ephemeris descriptors %d after the path, %d after swe_close(); want 1, 0\n", a, b);
  return 1;
}

static int p2(void)
{
  int i, n;
  for (i = 0; i < 100; i++) {
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, EPHE);
    swe_ctx_free(c);
  }
  if ((n = nfd()) == 0)
    return 0;
  printf("    %d ephemeris descriptors left; want 0\n", n);
  return 1;
}

static void *moon_thread(void *a)
{
  double x[6];
  char serr[AS_MAXCH];
  *(int *) a = swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
  return NULL;
}

static int p3(void)
{
  pthread_t th;
  int r = -1;
  swe_set_ephe_path(EPHE);
  pthread_create(&th, NULL, moon_thread, &r);
  pthread_join(th, NULL);
  if (r >= 0 && noutside == 0)
    return 0;
  printf("    the thread's Moon %d, %d opens outside the path; want 0\n", r, noutside);
  return 1;
}

static int p4(void)
{
  struct tdt want = fresh(DNOMOON), m, w;
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  swe_set_ephe_path(DNOMOON);
  m = reading();
  w = worker_reading();
  return same("setting thread", m, want) + same("second thread", w, want);
}

static int p5(void)
{
  struct tdt want = fresh(EPHE), m;
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  swe_set_jpl_file("no_such_file.eph");
  m = reading();
  return same("after the missing JPL file", m, want);
}

static int p6(void)
{
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  double x[6];
  char serr[AS_MAXCH];
  struct tdt want = fresh(DALL), a, b;
  int bad = 0;
  swe_set_ephe_path(DLATE);
  reading();
  snprintf(p, sizeof p, "%s/semo_18.se1", EPHE);
  snprintf(q, sizeof q, "%s/semo_18.se1", DLATE);
  if (symlink(p, q) != 0)
    return 1;
  swe_set_ephe_path(DLATE);
  a = reading();
  swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
  b = reading();
  bad += same("set again", a, want);
  bad += same("after a Moon", b, want);
  return bad;
}

static int p7(void)
{
  double x[6];
  char serr[AS_MAXCH];
  swe_set_ephe_path(EPHE);
  swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
  zero();
  swe_set_ephe_path(EPHE);
  swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
  if (ncall == 0)
    return 0;
  printf("    %d fopen() calls; want 0\n", ncall);
  return 1;
}

static int p8(void)
{
  swe_set_ephe_path(DNOMOON);
  zero();
  swe_set_ephe_path(DNOMOON);
  if (nok == 0)
    return 0;
  printf("    %d files opened; want 0\n", nok);
  return 1;
}

static int p9(void)
{
  double x[6];
  char serr[AS_MAXCH];
  int r;
  if (chdir(DNOMOON) != 0)
    return 1;
  zero();
  /* No moon file here: the Earth comes from the Moshier Moon only with
   * the fallback allowed (UPSTREAM-BUGS 120); this case counts opens. */
  swe_set_ephe_fallback(1);
  r = swe_calc(2451545.0, SE_MARS, SEFLG_SWIEPH, x, serr);
  swe_set_ephe_fallback(0);
  if (r >= 0 && nsepl == 1)
    return 0;
  printf("    Mars %d, sepl_18.se1 opened %d times; want 1 (%s)\n", r, nsepl, serr);
  return 1;
}

static const struct { const char *name; int (*fn)(void); } rgCase[] = {
  {"P1 the first path: one moon file open, none after swe_close()", p1},
  {"P2 100 contexts' paths: no descriptor left", p2},
  {"P3 a thread's first Moon: nothing opened outside the path", p3},
  {"P4 path, de200, a path without semo_18: its tidal term", p4},
  {"P5 path, de200, a missing JPL file: the path's tidal term", p5},
  {"P6 a moon file installed, the same path again: read", p6},
  {"P7 the same path, nothing changed: no fopen()", p7},
  {"P8 the same path, still no moon file: nothing opened", p8},
  {"P9 a first Mars, no path, no moon file: sepl_18 opened once", p9},
};
#define NCASE ((int) (sizeof rgCase / sizeof rgCase[0]))

int main(int argc, char **argv)
{
  int i, nbad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: pathset EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], EPHE) == NULL || mkdtemp(TOP) == NULL) {
    perror(argv[1]);
    return 2;
  }
  snprintf(DALL, sizeof DALL, "%s/all", TOP);
  snprintf(DNOMOON, sizeof DNOMOON, "%s/nomoon", TOP);
  snprintf(DLATE, sizeof DLATE, "%s/late", TOP);
  if (link_all(DALL, NULL) != 0 || link_all(DNOMOON, "semo_18.se1") != 0) {
    printf("G115: set-up failed\n");
    return 2;
  }
  printf("G115: the path and JPL setters: no file left open or opened twice, the tidal term of what they set\n");
  for (i = 0; i < NCASE; i++) {
    int st = 0;
    pid_t pid;
    fflush(stdout);
    if (link_all(DLATE, "semo_18.se1") != 0) {
      printf("  set-up failed\n");
      nbad++;
      continue;
    }
    if ((pid = fork()) == 0) {
      int r;
      alarm(60);
      if (chdir(DALL) != 0)
        _exit(1);
      r = rgCase[i].fn();
      fflush(stdout);
      _exit(r != 0);
    }
    waitpid(pid, &st, 0);
    unlink_dir(DLATE);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
      printf("  %s: FAIL%s\n", rgCase[i].name, WIFSIGNALED(st) ? " (crashed)" : "");
      nbad++;
    } else
      printf("  %s: OK\n", rgCase[i].name);
  }
  unlink_dir(DALL);
  unlink_dir(DNOMOON);
  rmdir(TOP);
  printf(nbad ? "G115 FAIL: %d\n" : "G115 PASS\n", nbad);
  return nbad != 0;
}
