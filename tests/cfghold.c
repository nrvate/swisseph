/* cfghold.c -- G112: a calculation finishes with the configuration it
 * started with.
 *
 * The defect (UPSTREAM-BUGS 115, this fork's): a calculation on the default
 * context adopted the shared configuration at every swi_init_swed_if_start()
 * it passed, and a calculation passes several -- a topocentric one at the
 * observer's swe_deltat_ex() and swe_sidtime0(), swe_calc_ut() at its
 * delta t and again at swe_calc(), a star at the Earth's swe_calc(). A
 * setter another thread called in between was adopted in the middle of the
 * calculation: swi_config_apply() closed the files it was reading and
 * emptied the caches it had filled. A topocentric JPL Jupiter by
 * swe_calc_ut() while another thread switched swe_set_jpl_file() crashed in
 * swejpl.c's state() on the closed file within 0.3 s; where nothing
 * crashed, one answer mixed two configurations, up to 2.6e-5 deg.
 *
 * The check is deterministic, with no timing: the file a calculation reads
 * after its entry and before its first nested sync -- seorbel.txt for a
 * fictitious body, sefstars.txt for a star -- is a FIFO. A worker thread's
 * call opens it and waits for its lines; the main thread, once the open
 * has returned, calls a setter (published to the worker's default context)
 * and only then writes the lines. Each answer must equal, bit for bit, the
 * answer of a fresh explicit context with the configuration the call
 * started with, reading the same lines from a regular file. Four cases,
 * each in its own process (a crash is a FAIL, not a lost run):
 *
 *   models  Moshier fictitious body, topocentric; swe_set_astro_models()
 *   jpl     de200 fictitious body, topocentric; swe_set_jpl_file() to a
 *           missing file
 *   tidacc  swe_calc_ut() of that Moshier body in AD 1000;
 *           swe_set_tid_acc(-20)
 *   star    swe_fixstar2_ut() of Sirius, Moshier, topocentric;
 *           swe_set_astro_models()
 *
 * Measured: 0 in every case. Before the fix the default build fails all
 * four: models 4.3e-6 deg in latitude, jpl 4.3e-6 (the JPL file closed
 * and reopened half-way), tidacc 4.8e-6 deg/day, star 1.1e-5 deg. The
 * upstream build (SWE_UPSTREAM_COMPAT, which keeps the nested syncs) fails
 * them too: 0.41 deg/day in the longitude speed, the jpl call refused (-1,
 * "JPL ephemeris is not available") though it started on de200.eph, 1.1e-2
 * deg/day, 2.5e-4 in the distance speed.
 * Run as
 *   cfghold EPHEDIR WORKDIR
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

#define TJD 2451545.25
#define TJD_1000 2086302.75
#define TOPO 8.55, 47.37, 400.0

static const char *ORBEL =
  "J1900, J1900, 163.7409, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, ZZTESTBODY\n";
static const char *STARS =
  "Sirius     ,alCMa,ICRS,06,45,08.91728,-16,42,58.0171,-546.01,-1223.07,-5.5,379.21,-1.46,-16, 1591\n";

enum { C_MODELS, C_JPL, C_TIDACC, C_STAR, NCASE };
static const char *cname[NCASE] = {"models", "jpl", "tidacc", "star"};

static char dfifo[PATH_MAX], dreg[PATH_MAX];
static int cur;
static double got[6];
static int32 gotrc;
static char goterr[AS_MAXCH];

/* the calculation under test, on the context c (NULL: this thread's default) */
static int32 calc(swe_ctx *c, int k, double *x, char *serr)
{
  char star[64];
  switch (k) {
  case C_MODELS:
    return c ? swe_calc_r(c, TJD, SE_FICT_OFFSET, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr)
             : swe_calc(TJD, SE_FICT_OFFSET, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr);
  case C_JPL:
    return c ? swe_calc_r(c, TJD, SE_FICT_OFFSET, SEFLG_JPLEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr)
             : swe_calc(TJD, SE_FICT_OFFSET, SEFLG_JPLEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr);
  case C_TIDACC:	/* AD 1000, where the tidal term moves delta t by minutes */
    return c ? swe_calc_ut_r(c, TJD_1000, SE_FICT_OFFSET, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr)
             : swe_calc_ut(TJD_1000, SE_FICT_OFFSET, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr);
  default:
    strcpy(star, "Sirius");
    return c ? swe_fixstar2_ut_r(c, star, TJD, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr)
             : swe_fixstar2_ut(star, TJD, SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr);
  }
}

/* the setter the main thread calls while the worker's call waits */
static void change(int k)
{
  switch (k) {
  case C_MODELS: case C_STAR: swe_set_astro_models("1,3,1,1,1,0,0,1", 0); break;
  case C_JPL: swe_set_jpl_file("de999.eph"); break;
  default: swe_set_tid_acc(-20.0); break;
  }
}

static void *worker(void *arg)
{
  double x[6];
  char serr[AS_MAXCH];
  (void) arg;
  /* first call: this thread's context starts and adopts path, site, models */
  swe_calc(TJD, SE_SUN, SEFLG_MOSEPH, x, serr);
  *goterr = '\0';
  gotrc = calc(NULL, cur, got, goterr);
  return NULL;
}

/* one case, in a child process: 0 = equal, 1 = differs */
static int run_case(int k)
{
  const char *file = k == C_STAR ? "sefstars.txt" : "seorbel.txt";
  const char *text = k == C_STAR ? STARS : ORBEL;
  char fifo[PATH_MAX + 32], tmp[PATH_MAX + 40], serr[AS_MAXCH];
  double want[6], d = 0;
  int32 wantrc;
  swe_ctx *c;
  pthread_t th;
  FILE *fp;
  int fd, i, imax = 0;
  cur = k;
  /* the reference: a fresh context with the starting configuration */
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, dreg);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_topo_r(c, TOPO);
  wantrc = calc(c, k, want, serr);
  swe_ctx_free(c);
  /* the same configuration, process-wide, along the FIFO's directory */
  swe_set_ephe_path(dfifo);
  swe_set_jpl_file("de200.eph");
  swe_set_topo(TOPO);
  snprintf(fifo, sizeof fifo, "%s/%s", dfifo, file);
  if (pthread_create(&th, NULL, worker, NULL) != 0)
    return 2;
  /* returns once the worker's call has opened the FIFO and waits for lines */
  if ((fd = open(fifo, O_WRONLY)) < 0)
    return 2;
  change(k);
  /* a regular file in the FIFO's place: a call that closed the file under
   * itself and opens it again reads the lines from there, rather than
   * waiting forever */
  snprintf(tmp, sizeof tmp, "%s.new", fifo);
  if ((fp = fopen(tmp, "w")) == NULL)
    return 2;
  fputs(text, fp);
  fclose(fp);
  if (rename(tmp, fifo) != 0)
    return 2;
  if (write(fd, text, strlen(text)) != (ssize_t) strlen(text))
    return 2;
  close(fd);
  pthread_join(th, NULL);
  if (gotrc != wantrc) {
    printf("  %-7s retflag %d, want %d: %s\n", cname[k], gotrc, wantrc, goterr);
    return 1;
  }
  for (i = 0; i < 6; i++)
    if (fabs(got[i] - want[i]) > d) {
      d = fabs(got[i] - want[i]);
      imax = i;
    }
  printf("  %-7s %.2e in x[%d] (lon %.10f, want %.10f)\n", cname[k], d, imax, got[0], want[0]);
  return d != 0;
}

static int mkdirs(const char *work, const char *ephe_abs)
{
  char p[PATH_MAX + 64], q[PATH_MAX + 64];
  FILE *fp;
  snprintf(dfifo, sizeof dfifo, "%s/fifo", work);
  snprintf(dreg, sizeof dreg, "%s/reg", work);
  if (mkdir(dfifo, 0700) != 0 || mkdir(dreg, 0700) != 0)
    return -1;
  snprintf(q, sizeof q, "%s/de200.eph", ephe_abs);
  snprintf(p, sizeof p, "%s/de200.eph", dfifo);
  if (symlink(q, p) != 0) return -1;
  snprintf(p, sizeof p, "%s/de200.eph", dreg);
  if (symlink(q, p) != 0) return -1;
  snprintf(p, sizeof p, "%s/seorbel.txt", dreg);
  if ((fp = fopen(p, "w")) == NULL) return -1;
  fputs(ORBEL, fp);
  fclose(fp);
  snprintf(p, sizeof p, "%s/sefstars.txt", dreg);
  if ((fp = fopen(p, "w")) == NULL) return -1;
  fputs(STARS, fp);
  fclose(fp);
  return 0;
}

int main(int argc, char **argv)
{
  char ephe_abs[PATH_MAX], p[PATH_MAX + 64];
  int k, bad = 0;
  if (argc != 3) {
    fprintf(stderr, "usage: cfghold EPHEDIR WORKDIR\n");
    return 2;
  }
  if (realpath(argv[1], ephe_abs) == NULL || mkdirs(argv[2], ephe_abs) != 0) {
    printf("G112 FAIL: cannot set up %s\n", argv[2]);
    return 1;
  }
  printf("G112: a calculation finishes with the configuration it started with\n");
  for (k = 0; k < NCASE; k++) {
    pid_t pid;
    int st;
    const char *file = k == C_STAR ? "sefstars.txt" : "seorbel.txt";
    snprintf(p, sizeof p, "%s/%s", dfifo, file);
    unlink(p);
    if (mkfifo(p, 0600) != 0) {
      printf("G112 FAIL: mkfifo %s: %s\n", p, strerror(errno));
      return 1;
    }
    fflush(stdout);
    if ((pid = fork()) < 0)
      return 2;
    if (pid == 0) {
      int r;
      alarm(20);			/* a backstop: no case takes a second */
      r = run_case(k);
      fflush(stdout);
      _exit(r);
    }
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st)) {
      printf("  %-7s killed by signal %d\n", cname[k], WTERMSIG(st));
      bad++;
    } else if (WEXITSTATUS(st) != 0) {
      if (WEXITSTATUS(st) == 2)
        printf("  %-7s could not run\n", cname[k]);
      bad++;
    }
    unlink(p);
  }
  if (bad) {
    printf("G112 FAIL: %d\n", bad);
    return 1;
  }
  printf("G112 PASS\n");
  return 0;
}
