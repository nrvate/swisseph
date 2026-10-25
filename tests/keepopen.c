/* keepopen.c -- G102: nothing that leaves the configuration as it was
 * closes or reopens a file, and swe_deltat() answers for the configuration,
 * not for the files this context happens to have open.
 *
 * The defects (UPSTREAM-BUGS 103), each counted in fopen() calls (this
 * program interposes fopen() and counts per thread) over 100 rounds after
 * a warm-up, against the same answer in a fresh process:
 *
 *   setter + calc, files by SE_EPHE_PATH   794 -> 0   any setter's publish
 *       made the next sync adopt the master's "no path", and the next
 *       calculation set the implicit path again (52.8 us a round for 4.0)
 *   swe_set_ephe_path(same) + calc         398 -> 0
 *   swe_set_jpl_file(same) + JPL calc      299 -> 0
 *   swe_set_ephe_path(same) + swe_fixstar2 300 -> 0   (catalogue reread)
 *   Swiss / de200 / Moshier alternating    101 -> 0   every switch of
 *       ephemeris closed every file (3000 opens in 2000 Mars alternating
 *       Swiss and de200, 35.6 us each for 6.7)
 *   a second thread, Swiss, while the first sets the de200 file
 *                                            2 -> 0   (closed its Swiss files)
 * and a real change of path still reopens (396) and answers as a fresh
 * process does. And the ephemeris swe_deltat() and swe_get_ayanamsa_ut()
 * assume: after swe_set_jpl_file("de200.eph"), AD 1000, 1305.37 s on the
 * setting thread and 1474.79 on a second one (the ayanamsa 7.5e-8 deg
 * apart); after another path and a JPLEPH calculation, which reopened the
 * file by its name, 1305.37 for the 1474.79 just before. Within 1e-12
 * (deg, s).
 *
 * Measured before the fix, 9 of 10 fail (the real change passes); the
 * compat build the same 9; the upstream build (legacy-master) 8 (its
 * threads share no configuration, so the setter case opens nothing and
 * its second thread finds no files at all: 600 opens). Run as
 *   keepopen EPHEDIR
 */
#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define N 100
#define T0 2451545.0
#define T_1000 2086302.5
#define TOL 1e-12

static __thread int nopen;
FILE *fopen(const char *p, const char *m)
{
  static FILE *(*real)(const char *, const char *);
  if (real == NULL)
    *(void **) &real = dlsym(RTLD_NEXT, "fopen");
  nopen++;
  return real(p, m);
}

static const char *EPHE;
static char DIR_B[] = "/tmp/keepopen-XXXXXX";	/* the same files, another path */

struct ans { int opens; double v[6]; };

static int in_child(void (*fn)(int, struct ans *), int k, struct ans *out)
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
    fn(k, &a);
    _exit(write(fd[1], &a, sizeof a) == (ssize_t) sizeof a ? 0 : 3);
  }
  close(fd[1]);
  while (got < (ssize_t) sizeof *out && (r = read(fd[0], (char *) out + got, sizeof *out - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return got == (ssize_t) sizeof *out ? 0 : -1;
}

static void moon(int32 fl, int i, double *v)
{
  double x[6];
  char serr[AS_MAXCH];
  swe_calc_ut(T0 + i, SE_MOON, fl, x, serr);
  memcpy(v, x, sizeof x);
}

static void sirius(int i, double *v)
{
  double x[6];
  char s[AS_MAXCH], serr[AS_MAXCH];
  strcpy(s, "sirius");
  swe_fixstar2(s, T0 + i, SEFLG_SWIEPH, x, serr);
  memcpy(v, x, sizeof x);
}

/*-------------------------------------------------------- the counts */
enum { C_SETTER, C_PATH, C_JPL, C_STAR, C_SWITCH, C_THREAD, C_REAL, NC };
static const char *CN[NC] = {
  "setter + calc, files by SE_EPHE_PATH",
  "swe_set_ephe_path(same) + calc",
  "swe_set_jpl_file(same) + JPL calc",
  "swe_set_ephe_path(same) + swe_fixstar2",
  "Swiss / de200 / Moshier alternating",
  "second thread while first sets de200",
  "a real path change (must reopen)",
};
static const int32 EPH3[3] = { SEFLG_SWIEPH, SEFLG_JPLEPH, SEFLG_MOSEPH };

static sem_t s1, s2;
static struct ans *thr_out;
static void *worker(void *arg)
{
  int i;
  (void) arg;
  moon(SEFLG_SWIEPH, 0, thr_out->v);
  sem_post(&s1);
  sem_wait(&s2);
  nopen = 0;
  for (i = 0; i < N; i++)
    moon(SEFLG_SWIEPH, i, thr_out->v);
  thr_out->opens = nopen;
  return NULL;
}

/* fresh: the last round's request, in a pristine process */
static void count(int k, struct ans *a)
{
  int i, fresh = k >= 100;
  if (fresh)
    k -= 100;
  if (k == C_SETTER)
    setenv("SE_EPHE_PATH", EPHE, 1);
  else
    swe_set_ephe_path(k == C_REAL && fresh ? DIR_B : EPHE);
  if (k == C_JPL || k == C_SWITCH || (k == C_THREAD && fresh))
    swe_set_jpl_file("de200.eph");
  if (k == C_SETTER)
    swe_set_topo(8.55, 47.37, 400 + ((N - 1) & 1));
  if (fresh) {
    switch (k) {
    case C_STAR: sirius(N - 1, a->v); break;
    case C_JPL: moon(SEFLG_JPLEPH, N - 1, a->v); break;
    case C_SWITCH: moon(EPH3[(N - 1) % 3], N - 1, a->v); break;
    case C_THREAD: moon(SEFLG_SWIEPH, N - 1, a->v); break;
    default: moon(SEFLG_SWIEPH, N - 1, a->v); break;
    }
    return;
  }
  /* warm-up: what a first use and a first setter do once */
  moon(SEFLG_SWIEPH, 0, a->v);
  if (k == C_SETTER) {
    swe_set_topo(8.55, 47.37, 401);
    moon(SEFLG_SWIEPH, 0, a->v);
  }
  if (k == C_JPL || k == C_SWITCH)
    for (i = 0; i < 3; i++)
      moon(EPH3[i], 0, a->v);
  if (k == C_STAR)
    sirius(0, a->v);
  if (k == C_THREAD) {
    pthread_t th;
    sem_init(&s1, 0, 0);
    sem_init(&s2, 0, 0);
    thr_out = a;
    if (pthread_create(&th, NULL, worker, NULL) != 0)
      return;
    sem_wait(&s1);
    swe_set_jpl_file("de200.eph");
    sem_post(&s2);
    pthread_join(th, NULL);
    return;
  }
  nopen = 0;
  for (i = 0; i < N; i++) {
    switch (k) {
    case C_SETTER:
      swe_set_topo(8.55, 47.37, 400 + (i & 1));
      moon(SEFLG_SWIEPH, i, a->v);
      break;
    case C_PATH:
      swe_set_ephe_path(EPHE);
      moon(SEFLG_SWIEPH, i, a->v);
      break;
    case C_JPL:
      swe_set_jpl_file("de200.eph");
      moon(SEFLG_JPLEPH, i, a->v);
      break;
    case C_STAR:
      swe_set_ephe_path(EPHE);
      sirius(i, a->v);
      break;
    case C_SWITCH:
      moon(EPH3[i % 3], i, a->v);
      break;
    case C_REAL:
      swe_set_ephe_path((i & 1) ? DIR_B : EPHE);
      moon(SEFLG_SWIEPH, i, a->v);
      break;
    }
  }
  a->opens = nopen;
}

/*------------------------------------------- swe_deltat() and the guess */
enum { D_THREAD, D_AYA, D_LAZY, ND };
static const char *DN[ND] = {
  "swe_deltat() on a second thread, de200 set",
  "swe_get_ayanamsa_ut() on a second thread",
  "swe_deltat() around a JPLEPH calculation",
};
static double dt_val[2];
static void *dt_worker(void *arg)
{
  int k = *(int *) arg;
  dt_val[1] = k == D_AYA ? swe_get_ayanamsa_ut(T_1000) : swe_deltat(T_1000) * 86400;
  return NULL;
}

static void deltat_case(int k, struct ans *a)
{
  pthread_t th;
  double x[6];
  char serr[AS_MAXCH];
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  if (k == D_AYA)
    swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  if (k == D_LAZY) {
    /* another path: the file closed and no longer the configuration's,
     * its name kept, so a JPLEPH request opens it again by itself */
    swe_set_ephe_path(DIR_B);
    dt_val[0] = swe_deltat(T_1000) * 86400;
    swe_calc_ut(T0, SE_MOON, SEFLG_JPLEPH, x, serr);
    dt_val[1] = swe_deltat(T_1000) * 86400;
  } else {
    dt_val[0] = k == D_AYA ? swe_get_ayanamsa_ut(T_1000) : swe_deltat(T_1000) * 86400;
    if (pthread_create(&th, NULL, dt_worker, &k) == 0)
      pthread_join(th, NULL);
  }
  a->v[0] = dt_val[0];
  a->v[1] = dt_val[1];
}

int main(int argc, char **argv)
{
  struct ans a, f;
  char lnk[sizeof DIR_B + 32];
  int k, nfail = 0, n = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  if (mkdtemp(DIR_B) == NULL) {
    printf("G102 FAIL: no temporary directory\n");
    return 1;
  }
  {	/* path B: the same files through symbolic links */
    static const char *F[] = { "sepl_18.se1", "semo_18.se1", "seas_18.se1",
                               "sefstars.txt", "de200.eph" };
    char tgt[2 * AS_MAXCH];
    unsigned j;
    for (j = 0; j < sizeof F / sizeof F[0]; j++) {
      if (EPHE[0] == '/')
        snprintf(tgt, sizeof tgt, "%s/%s", EPHE, F[j]);
      else {
        char cwd[AS_MAXCH];
        if (getcwd(cwd, sizeof cwd) == NULL)
          return 1;
        snprintf(tgt, sizeof tgt, "%s/%s/%s", cwd, EPHE, F[j]);
      }
      snprintf(lnk, sizeof lnk, "%s/%s", DIR_B, F[j]);
      if (symlink(tgt, lnk) != 0)
        printf("  note: no link %s\n", lnk);
    }
  }
  for (k = 0; k < NC; k++, n++) {
    if (in_child(count, k, &a) != 0 || in_child(count, k + 100, &f) != 0) {
      printf("  %-44s did not run\n", CN[k]);
      nfail++;
      continue;
    }
    {
      double d = 0;
      int j;
      for (j = 0; j < 6; j++)
        if (!(fabs(a.v[j] - f.v[j]) <= d))
          d = fabs(a.v[j] - f.v[j]);
      if (k == C_REAL ? a.opens == 0 : a.opens != 0) {
        printf("  %-44s FAIL: %d opens in %d rounds\n", CN[k], a.opens, N);
        nfail++;
      } else if (d > TOL) {
        printf("  %-44s FAIL: off a fresh process by %.3g\n", CN[k], d);
        nfail++;
      } else
        printf("  %-44s %d opens\n", CN[k], a.opens);
    }
  }
  for (k = 0; k < ND; k++, n++) {
    if (in_child(deltat_case, k, &a) != 0) {
      printf("  %-44s did not run\n", DN[k]);
      nfail++;
      continue;
    }
    if (fabs(a.v[0] - a.v[1]) > TOL) {
      printf("  %-44s FAIL: %.10f and %.10f\n", DN[k], a.v[0], a.v[1]);
      nfail++;
    }
  }
  {
    unsigned j;
    static const char *F[] = { "sepl_18.se1", "semo_18.se1", "seas_18.se1",
                               "sefstars.txt", "de200.eph" };
    for (j = 0; j < sizeof F / sizeof F[0]; j++) {
      snprintf(lnk, sizeof lnk, "%s/%s", DIR_B, F[j]);
      unlink(lnk);
    }
    rmdir(DIR_B);
  }
  if (nfail)
    printf("G102 FAIL: %d of %d\n", nfail, n);
  else
    printf("G102 PASS (%d)\n", n);
  return nfail != 0;
}
