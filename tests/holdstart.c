/* holdstart.c -- G124: a thread's first call reads the configuration the
 * main thread set, as a later call does.
 *
 * The defect (UPSTREAM-BUGS 128, this fork's): swi_config_hold(), which
 * syncs the shared configuration at the entry of every public function
 * that calculates (bugs 115 and 124), did not sync a context not yet
 * initialised, and left it to the first swi_init_swed_if_start() inside
 * the call to start and sync it. What the call read before that it read
 * from the unstarted context, all zeros: on a thread's very first call
 * the user delta t that swe_deltat_ex() checks first (and so every
 * function of UT through it), the sidereal-time model swe_sidtime0()
 * reads first. swe_get_astro_models(), a getter not held, listed the
 * models without starting the context at all. The second call was right.
 *
 * The check: each case in its own process. The main thread sets the path
 * and the case's setter, then a new thread makes the case's call as its
 * first; the answer must equal, bit for bit, the answer of a fresh
 * process that set the same two first and made the call on its main
 * thread:
 *
 *   deltat    swe_set_delta_t_userdef(0.001); swe_deltat_ex(), Swiss
 *   calc_ut   the same; swe_calc_ut() of the Moon with speed
 *   utc       the same; swe_jdut1_to_utc(), the seconds of the day
 *   eclwhere  the same; swe_sol_eclipse_where() at 2003 May 31 4:05 TT
 *   houses    the same; swe_houses() Placidus at 8.55E 47.37N
 *   sidtime0  swe_set_astro_models("1,3,1,1,1,0,0,1"); swe_sidtime0()
 *   models    the same; swe_get_astro_models(NULL, ...), the whole text
 *
 * and one for what the fix must keep: with no path at all (an empty
 * current directory, no SE_EPHE_PATH), a context's first swe_deltat_ex()
 * still carries "Please call swe_set_ephe_path() ...", on the main thread
 * and on a new one (warn). The hold starts the context now, and the
 * warning keys on the start.
 *
 * Measured: 0 in every case. Before the fix the default build fails the
 * seven: deltat 22.6 s (63.83 s for 86.40 s), calc_ut 3.1e-3 deg in the
 * Moon's longitude, utc 22.6 s, eclwhere 0.238 deg in the central line's
 * latitude, houses 2.8e-9 deg, sidtime0 3.1e-7 h, models
 * "-amod0,0,0,0,0,0,0,0" for "-amod1,3,1,1,1,0,0,1". The upstream build
 * (SWE_UPSTREAM_COMPAT, whose hold does nothing) fails the same seven --
 * 22.6 s (and the warning bug 110 restores), 3.1e-3 deg, 0.355 s, 0.238
 * deg, 2.8e-9 deg, 3.1e-7 h, a tidal acceleration of 0 (bug 101) -- and
 * passes warn. Run as
 *   holdstart EPHEDIR
 */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

#define TJD 2451545.25
#define NOUT 24
#define WARN "Please call swe_set_ephe_path()"

enum { C_DELTAT, C_CALCUT, C_UTC, C_ECLWHERE, C_HOUSES, C_SIDTIME0, C_MODELS, C_WARN, NCASE };
static const char *cname[NCASE] = {"deltat", "calc_ut", "utc", "eclwhere", "houses",
                                   "sidtime0", "models", "warn"};
static const char *what[NCASE] = {"s", "deg", "s", "deg", "deg", "h", "", ""};
static const int shown[NCASE] = {0, 1, 0, 1, 1, 0, 0, 0};	/* the value printed */

static const char *EPHE;
static char EMPTY[] = "/tmp/holdstart-XXXXXX";

struct ans {
  double v[NOUT];
  char s[AS_MAXCH * 16];	/* swe_get_astro_models() writes some 530 */
};

static int cur;
static struct ans thr_ans;

/* the call under test, on this thread's default context */
static void call(int k, struct ans *a)
{
  double x[6], geopos[10], attr[20], cusp[13], ascmc[10], sec;
  int32 y, mo, d, h, mi;
  int i;
  memset(a, 0, sizeof *a);
  switch (k) {
  case C_DELTAT:
  case C_WARN:
    a->v[0] = swe_deltat_ex(TJD, SEFLG_SWIEPH, a->s);
    break;
  case C_CALCUT:
    a->v[0] = swe_calc_ut(TJD, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, x, a->s);
    for (i = 0; i < 6; i++)
      a->v[1 + i] = x[i];
    break;
  case C_UTC:
    swe_jdut1_to_utc(TJD, SE_GREG_CAL, &y, &mo, &d, &h, &mi, &sec);
    a->v[0] = h * 3600.0 + mi * 60 + sec;
    break;
  case C_ECLWHERE:
    a->v[0] = swe_sol_eclipse_where(2452790.67, SEFLG_SWIEPH, geopos, attr, a->s);
    for (i = 0; i < 2; i++)
      a->v[1 + i] = geopos[i];
    for (i = 0; i < 11; i++)
      a->v[3 + i] = attr[i];
    break;
  case C_HOUSES:
    a->v[0] = swe_houses(TJD, 47.37, 8.55, 'P', cusp, ascmc);
    for (i = 1; i <= 12; i++)
      a->v[i] = cusp[i];
    for (i = 0; i < 8; i++)
      a->v[13 + i] = ascmc[i];
    break;
  case C_SIDTIME0:
    a->v[0] = swe_sidtime0(TJD, 23.44, -0.004);
    break;
  default:
    swe_get_astro_models(NULL, a->s, 0);
    break;
  }
}

/* the path and the case's setter, the process's first calls */
static void setters(int k)
{
  char models[] = "1,3,1,1,1,0,0,1";
  if (k == C_WARN) {
    if (chdir(EMPTY) != 0)
      _exit(2);
    return;
  }
  swe_set_ephe_path(EPHE);
  if (k == C_SIDTIME0 || k == C_MODELS)
    swe_set_astro_models(models, 0);
  else
    swe_set_delta_t_userdef(0.001);
}

static void *worker(void *arg)
{
  (void) arg;
  call(cur, &thr_ans);
  return NULL;
}

/* in a child: the setters, then the call on a new thread (thr) or here */
static int get(int k, int thr, struct ans *a)
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
    setters(k);
    if (thr) {
      if (pthread_create(&th, NULL, worker, NULL) != 0)
        _exit(2);
      pthread_join(th, NULL);
    } else
      call(k, &thr_ans);
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

/* the first line on which two texts differ, for the record */
static void first_diff(const char *a, const char *b, char *la, char *lb, size_t n)
{
  size_t j = 0;
  while (a[j] && a[j] == b[j])
    j++;
  while (j > 0 && a[j - 1] != '\n')
    j--;
  snprintf(la, n, "%.*s", (int) strcspn(a + j, "\n"), a + j);
  snprintf(lb, n, "%.*s", (int) strcspn(b + j, "\n"), b + j);
}

static int run_case(int k)
{
  struct ans t, m;
  double worst = 0, scale = 1;
  int i, at = 0;
  if (get(k, 1, &t) != 0 || get(k, 0, &m) != 0) {
    printf("  %-9s could not run\n", cname[k]);
    return 1;
  }
  if (k == C_WARN) {
    int ok = strncmp(t.s, WARN, strlen(WARN)) == 0 && strncmp(m.s, WARN, strlen(WARN)) == 0;
    printf("  %-9s first swe_deltat_ex() with no path: thread [%.40s], main [%.40s]%s\n",
           cname[k], t.s, m.s, ok ? "" : " -- the warning is missing");
    return !ok;
  }
  if (k == C_DELTAT)
    scale = 86400;		/* days */
  for (i = 0; i < NOUT; i++)
    if (fabs(t.v[i] - m.v[i]) > worst) {
      worst = fabs(t.v[i] - m.v[i]);
      at = i;
    }
  if (memcmp(t.v, m.v, sizeof t.v) != 0)
    printf("  %-9s %.2e %s off in [%d] (%.12g, want %.12g)\n", cname[k], worst * scale,
           what[k], at, t.v[at], m.v[at]);
  if (strcmp(t.s, m.s) != 0) {
    char la[160], lb[160];
    first_diff(t.s, m.s, la, lb, sizeof la);
    printf("  %-9s the text differs: [%s] for [%s]\n", cname[k], la, lb);
  }
  if (memcmp(t.v, m.v, sizeof t.v) != 0 || strcmp(t.s, m.s) != 0)
    return 1;
  if (k == C_MODELS) {
    const char *p = strstr(t.s, "-amod");
    printf("  %-9s a new thread's first call as a fresh process's (%.*s)\n", cname[k],
           p ? (int) strcspn(p, " ") : 0, p ? p : "");
  } else
    printf("  %-9s a new thread's first call as a fresh process's (%.12g)\n", cname[k], t.v[shown[k]]);
  return 0;
}

int main(int argc, char **argv)
{
  int k, bad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: holdstart EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  if (mkdtemp(EMPTY) == NULL) {
    printf("G124 FAIL: cannot make %s\n", EMPTY);
    return 1;
  }
  printf("G124: a thread's first call sees the main thread's setter\n");
  for (k = 0; k < NCASE; k++)
    bad += run_case(k);
  rmdir(EMPTY);
  if (bad) {
    printf("G124 FAIL: %d\n", bad);
    return 1;
  }
  printf("G124 PASS\n");
  return 0;
}
