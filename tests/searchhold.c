/* searchhold.c -- G121: a search, a house frame, any public function that
 * makes several calculations, takes the configuration in effect at its
 * entry and finishes with it.
 *
 * The defect (UPSTREAM-BUGS 124, this fork's): bug 115 made the calculation
 * entry points -- swe_calc(), swe_calc_ut(), the star functions and a few
 * more -- sync the shared configuration once, at their outermost entry, and
 * hold every nested sync until they return. The other public functions
 * that make several calculations were not held: the eclipse, occultation,
 * rise and set and heliacal searches, the crossings, the houses, the
 * sidereal time, swe_azalt(), swe_pheno() and the rest synced at each
 * calculation they made, so a setter another thread called while one ran
 * was adopted between two of its iterations, and the call finished on a
 * configuration it had not started with, or on a mix of two.
 *
 * The check is G112's (cfghold.c), with no timing: swe_deltat.txt, which a
 * context reads at its first delta t in the table's years, is a FIFO. A
 * worker thread's default context starts on a call that needs no delta t;
 * then its call under test, at its first delta t, opens the FIFO and waits
 * for its lines. The main thread, once the open has returned, calls a
 * setter (published to the worker's default context) and only then writes
 * the lines. Each answer must equal, bit for bit, the answer of a fresh
 * explicit context with the configuration the call started with: a
 * directory holding only the fixture, the site 8.55E 47.37N 400 m,
 * Lahiri, reading the same lines from a regular file. Five cases, each in
 * its own process (a crash or a hang is a FAIL, not a lost run):
 *
 *   eclloc     swe_sol_eclipse_when_loc() at that site from 2003 May 29,
 *              Moshier; swe_set_topo() to 74W 40.7N
 *   eclglob    swe_sol_eclipse_when_glob() from J2000, Moshier;
 *              swe_set_astro_models("1,3,1,1,1,0,0,1")
 *   mooncross  swe_mooncross_ut() of sidereal 0 deg from J2000, Moshier;
 *              swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY)
 *   rise       swe_rise_trans() of the Moon's rising at the site, Moshier;
 *              swe_set_delta_t_userdef(0.001)
 *   houses     swe_houses_ex2() sidereal Placidus with speeds at the site,
 *              Moshier; swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY)
 *
 * and a sixth for the hold's other half, that a call adopts at its entry
 * what another thread set before it:
 *
 *   entry      a worker whose context has started waits while the main
 *              thread calls swe_set_delta_t_userdef(0.001), then calls
 *              swe_utc_to_jd(), swe_jdet_to_utc(), swe_deltat_ex() with -1
 *              and with SEFLG_MOSEPH, swe_sidtime(); each must answer as a
 *              fresh context with that delta t
 *
 * Measured: 0 in every case. Before the fix the default build fails all
 * six. eclloc matches neither configuration (the call names its site, so
 * the setter's answers as the starting one): the search ran at the
 * master's new site from its second calculation, and the eclipse of 2003
 * May 31 came out with its maximum 3752 s late, its first contact and
 * maximum visible (retflag 5010 for 4242), the contacts up to 4990 s off
 * and the Sun's azimuth 11.2 deg. The other four answer the setter's
 * configuration: eclglob 1.49e-3 s, mooncross 5600 s (Fagan-Bradley's
 * crossing), rise 1.03 s (the user delta t's), houses 0.883 deg in every
 * cusp. entry: the UTC conversions and delta t with -1 never synced on a
 * started context, and swe_deltat_ex() read the user delta t before its
 * one sync, so all four kept the old delta t, 22.2 s in TT from
 * swe_utc_to_jd() and in swe_jdet_to_utc()'s UTC, 22.6 s in both delta
 * t; swe_sidtime(), after them, was right. The upstream build
 * (SWE_UPSTREAM_COMPAT, which keeps the nested syncs) fails all six the
 * same way: 4990 s, 1.53e-3 s, 5600 s, 1.03 s, 0.883 deg, 22.6 s.
 * Run as
 *   searchhold EPHEDIR WORKDIR
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

#define TJD 2451545.25		/* UT */
#define TJD_ECL 2452789.0
#define SLON 8.55
#define SLAT 47.37
#define SALT 400.0
#define NOUT 48

static const char *DTFILE = "# fixture: the built-in table throughout\n";

enum { C_ECLLOC, C_ECLGLOB, C_MOONX, C_RISE, C_HOUSES, C_ENTRY, NCASE };
static const char *cname[NCASE] = {"eclloc", "eclglob", "mooncross", "rise", "houses", "entry"};

static char dfifo[PATH_MAX], dreg[PATH_MAX];
static int cur, in_call;
static double got[NOUT];
static int32 gotrc;
static char goterr[AS_MAXCH];

/* the call under test, on the context c (NULL: this thread's default);
 * out[] gets its answer, isday[] marks the values that are instants */
static int32 calc(swe_ctx *c, int k, double *out, int *isday, char *serr)
{
  double geopos[3] = {SLON, SLAT, SALT}, tret[10], attr[20];
  double cusp[13], ascmc[10], cs[13], as[10], t;
  int32 rc = OK, fl = SEFLG_MOSEPH;
  int i, n = 0;
  memset(out, 0, NOUT * sizeof *out);
  memset(isday, 0, NOUT * sizeof *isday);
  memset(tret, 0, sizeof tret);
  memset(attr, 0, sizeof attr);
  switch (k) {
  case C_ECLLOC:	/* two days before the eclipse of 2003 May 31 */
    rc = c ? swe_sol_eclipse_when_loc_r(c, TJD_ECL, fl, geopos, tret, attr, 0, serr)
           : swe_sol_eclipse_when_loc(TJD_ECL, fl, geopos, tret, attr, 0, serr);
    for (i = 0; i < 7; i++, n++) {
      out[n] = tret[i];
      isday[n] = 1;
    }
    for (i = 0; i < 11; i++)
      out[n++] = attr[i];
    break;
  case C_ECLGLOB:
    rc = c ? swe_sol_eclipse_when_glob_r(c, TJD, fl, 0, tret, 0, serr)
           : swe_sol_eclipse_when_glob(TJD, fl, 0, tret, 0, serr);
    for (i = 0; i < 10; i++, n++) {
      out[n] = tret[i];
      isday[n] = 1;
    }
    break;
  case C_MOONX:
    fl |= SEFLG_SIDEREAL;
    t = c ? swe_mooncross_ut_r(c, 0.0, TJD, fl, serr)
          : swe_mooncross_ut(0.0, TJD, fl, serr);
    out[0] = t;
    isday[0] = 1;
    rc = t < TJD ? ERR : OK;
    break;
  case C_RISE:
    rc = c ? swe_rise_trans_r(c, TJD, SE_MOON, NULL, fl, SE_CALC_RISE, geopos, 1013.25, 10, tret, serr)
           : swe_rise_trans(TJD, SE_MOON, NULL, fl, SE_CALC_RISE, geopos, 1013.25, 10, tret, serr);
    out[0] = tret[0];
    isday[0] = 1;
    break;
  default:
    fl |= SEFLG_SIDEREAL;
    rc = c ? swe_houses_ex2_r(c, TJD, fl, SLAT, SLON, 'P', cusp, ascmc, cs, as, serr)
           : swe_houses_ex2(TJD, fl, SLAT, SLON, 'P', cusp, ascmc, cs, as, serr);
    for (i = 1; i <= 12; i++) {
      out[n++] = cusp[i];
      out[n++] = cs[i];
    }
    for (i = 0; i < 8; i++) {
      out[n++] = ascmc[i];
      out[n++] = as[i];
    }
    break;
  }
  return rc;
}

/* the starting configuration, on c (NULL: process-wide) */
static void start(swe_ctx *c, const char *dir)
{
  if (c) {
    swe_set_ephe_path_r(c, dir);
    swe_set_topo_r(c, SLON, SLAT, SALT);
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  } else {
    swe_set_ephe_path(dir);
    swe_set_topo(SLON, SLAT, SALT);
    swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  }
}

/* the setter the main thread calls while the worker's call waits (c NULL),
 * or the same on a reference context */
static void change(swe_ctx *c, int k)
{
  char models[] = "1,3,1,1,1,0,0,1";
  switch (k) {
  case C_ECLLOC:
    if (c) swe_set_topo_r(c, -74.0, 40.7, 10.0); else swe_set_topo(-74.0, 40.7, 10.0);
    break;
  case C_ECLGLOB:
    if (c) swe_set_astro_models_r(c, models, 0); else swe_set_astro_models(models, 0);
    break;
  case C_RISE:
    if (c) swe_set_delta_t_userdef_r(c, 0.001); else swe_set_delta_t_userdef(0.001);
    break;
  default:
    if (c) swe_set_sid_mode_r(c, SE_SIDM_FAGAN_BRADLEY, 0, 0);
    else swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
    break;
  }
}

static void *worker(void *arg)
{
  double x[6];
  int isday[NOUT];
  char serr[AS_MAXCH];
  (void) arg;
  /* first call: this thread's context starts and adopts path, site, mode;
   * an instant in TT, no delta t */
  swe_calc(TJD, SE_SUN, SEFLG_MOSEPH, x, serr);
  __atomic_store_n(&in_call, 1, __ATOMIC_SEQ_CST);
  *goterr = '\0';
  gotrc = calc(NULL, cur, got, isday, goterr);
  return NULL;
}

/* the largest differences of a against b: instants in s, the rest in deg;
 * an instant one of them has and the other not (0) is counted in *nmiss */
static void diff(const double *a, const double *b, const int *isday, double *ds, double *dd,
                 int *nmiss)
{
  int i;
  *ds = *dd = 0;
  *nmiss = 0;
  for (i = 0; i < NOUT; i++) {
    double d = fabs(a[i] - b[i]);
    if (isday[i] && (a[i] == 0) != (b[i] == 0))
      (*nmiss)++;
    else if (isday[i] && d * 86400 > *ds)
      *ds = d * 86400;
    else if (!isday[i] && d > *dd)
      *dd = d;
  }
}

/* one case, in a child process: 0 = equal, 1 = differs, 2 = could not run */
static int run_case(int k)
{
  char fifo[PATH_MAX + 32], tmp[PATH_MAX + 40], serr[AS_MAXCH];
  double want[NOUT], newer[NOUT], ds, dd, ns, nd;
  int isday[NOUT], dmiss, nmiss;
  int32 wantrc, newrc;
  swe_ctx *c;
  pthread_t th;
  FILE *fp;
  int fd;
  cur = k;
  /* the reference: a fresh context with the starting configuration */
  c = swe_ctx_new();
  start(c, dreg);
  wantrc = calc(c, k, want, isday, serr);
  swe_ctx_free(c);
  /* and one with the setter's, for the record */
  c = swe_ctx_new();
  start(c, dreg);
  change(c, k);
  newrc = calc(c, k, newer, isday, serr);
  swe_ctx_free(c);
  /* the same configuration, process-wide, along the FIFO's directory */
  start(NULL, dfifo);
  snprintf(fifo, sizeof fifo, "%s/swe_deltat.txt", dfifo);
  if (pthread_create(&th, NULL, worker, NULL) != 0)
    return 2;
  /* returns once the worker's call has opened the FIFO and waits for lines */
  if ((fd = open(fifo, O_WRONLY)) < 0)
    return 2;
  if (!__atomic_load_n(&in_call, __ATOMIC_SEQ_CST)) {
    printf("  %-9s the FIFO was opened before the call under test\n", cname[k]);
    return 2;
  }
  change(NULL, k);
  /* a regular file in the FIFO's place, for a call that opens it again */
  snprintf(tmp, sizeof tmp, "%s.new", fifo);
  if ((fp = fopen(tmp, "w")) == NULL)
    return 2;
  fputs(DTFILE, fp);
  fclose(fp);
  if (rename(tmp, fifo) != 0)
    return 2;
  if (write(fd, DTFILE, strlen(DTFILE)) != (ssize_t) strlen(DTFILE))
    return 2;
  close(fd);
  pthread_join(th, NULL);
  diff(got, want, isday, &ds, &dd, &dmiss);
  diff(got, newer, isday, &ns, &nd, &nmiss);
  printf("  %-9s %.2e s, %.2e deg, %d instants missing or extra (%.9f, want %.9f);\n"
         "            against the setter's configuration %.2e s, %.2e deg, %d\n",
         cname[k], ds, dd, dmiss, got[0], want[0], ns, nd, nmiss);
  if (gotrc != wantrc) {
    printf("  %-9s retflag %d, want %d (the setter's %d) %s\n", cname[k], gotrc,
           wantrc, newrc, goterr);
    return 1;
  }
  return ds != 0 || dd != 0 || dmiss != 0;
}

/* The other half of the hold: a call adopts at its entry what another
 * thread set before it. A worker whose default context has started (and
 * read swe_deltat.txt) waits while the main thread calls
 * swe_set_delta_t_userdef(0.001), then converts UTC both ways and asks for
 * delta t; each answer must equal a fresh context's with that delta t. */
static pthread_barrier_t bar;
static double eget[6];

static void entry_calls(swe_ctx *c, double *r)
{
  double dret[2], s;
  int32 y, mo, d, h, mi;
  char serr[AS_MAXCH];
  if (c) swe_utc_to_jd_r(c, 2000, 1, 1, 12, 0, 0, SE_GREG_CAL, dret, serr);
  else swe_utc_to_jd(2000, 1, 1, 12, 0, 0, SE_GREG_CAL, dret, serr);
  r[0] = dret[0];
  r[1] = dret[1];
  if (c) swe_jdet_to_utc_r(c, TJD, SE_GREG_CAL, &y, &mo, &d, &h, &mi, &s);
  else swe_jdet_to_utc(TJD, SE_GREG_CAL, &y, &mo, &d, &h, &mi, &s);
  r[2] = h * 3600.0 + mi * 60 + s;
  r[3] = c ? swe_deltat_ex_r(c, TJD, -1, serr) : swe_deltat_ex(TJD, -1, serr);
  r[4] = c ? swe_deltat_ex_r(c, TJD, SEFLG_MOSEPH, serr) : swe_deltat_ex(TJD, SEFLG_MOSEPH, serr);
  r[5] = c ? swe_sidtime_r(c, TJD) : swe_sidtime(TJD);
}

static void *entry_worker(void *arg)
{
  double x[6], r[6];
  char serr[AS_MAXCH];
  (void) arg;
  swe_calc(TJD, SE_SUN, SEFLG_MOSEPH, x, serr);
  entry_calls(NULL, r);		/* the context reads the table */
  pthread_barrier_wait(&bar);
  pthread_barrier_wait(&bar);	/* the main thread's setter in between */
  entry_calls(NULL, eget);
  return NULL;
}

static int run_entry(void)
{
  static const char *what[6] = {"utc_to_jd TT", "utc_to_jd UT1", "jdet_to_utc",
                                "deltat_ex(-1)", "deltat_ex", "sidtime"};
  double want[6];
  pthread_t th;
  swe_ctx *c;
  int i, bad = 0;
  c = swe_ctx_new();
  start(c, dreg);
  swe_set_delta_t_userdef_r(c, 0.001);
  entry_calls(c, want);
  swe_ctx_free(c);
  start(NULL, dreg);
  pthread_barrier_init(&bar, NULL, 2);
  if (pthread_create(&th, NULL, entry_worker, NULL) != 0)
    return 2;
  pthread_barrier_wait(&bar);
  swe_set_delta_t_userdef(0.001);
  pthread_barrier_wait(&bar);
  pthread_join(th, NULL);
  for (i = 0; i < 6; i++)
    if (eget[i] != want[i]) {
      printf("  %-9s %s %.2e off (%.9f, want %.9f)\n", cname[C_ENTRY], what[i],
             fabs(eget[i] - want[i]), eget[i], want[i]);
      bad = 1;
    }
  if (!bad)
    printf("  %-9s the setter made before the calls: adopted by all six\n", cname[C_ENTRY]);
  return bad;
}

static int mkdirs(const char *work)
{
  char p[PATH_MAX + 64];
  FILE *fp;
  snprintf(dfifo, sizeof dfifo, "%s/fifo", work);
  snprintf(dreg, sizeof dreg, "%s/reg", work);
  if (mkdir(dfifo, 0700) != 0 || mkdir(dreg, 0700) != 0)
    return -1;
  snprintf(p, sizeof p, "%s/swe_deltat.txt", dreg);
  if ((fp = fopen(p, "w")) == NULL)
    return -1;
  fputs(DTFILE, fp);
  fclose(fp);
  return 0;
}

int main(int argc, char **argv)
{
  char p[PATH_MAX + 64];
  int k, bad = 0;
  if (argc != 3) {
    fprintf(stderr, "usage: searchhold EPHEDIR WORKDIR\n");
    return 2;
  }
  /* EPHEDIR is not read: every case is Moshier's, and the directories are
   * the work directory's own, so that nothing but the fixture is found */
  if (mkdirs(argv[2]) != 0) {
    printf("G121 FAIL: cannot set up %s\n", argv[2]);
    return 1;
  }
  printf("G121: a search, a house frame, a time conversion: one configuration, the one at its entry\n");
  for (k = 0; k < NCASE; k++) {
    pid_t pid;
    int st;
    snprintf(p, sizeof p, "%s/swe_deltat.txt", dfifo);
    unlink(p);
    if (mkfifo(p, 0600) != 0) {
      printf("G121 FAIL: mkfifo %s: %s\n", p, strerror(errno));
      return 1;
    }
    fflush(stdout);
    if ((pid = fork()) < 0)
      return 2;
    if (pid == 0) {
      int r;
      alarm(20);			/* a backstop: no case takes a second */
      r = k == C_ENTRY ? run_entry() : run_case(k);
      fflush(stdout);
      _exit(r);
    }
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st)) {
      printf("  %-9s killed by signal %d\n", cname[k], WTERMSIG(st));
      bad++;
    } else if (WEXITSTATUS(st) != 0) {
      if (WEXITSTATUS(st) == 2)
        printf("  %-9s could not run\n", cname[k]);
      bad++;
    }
    unlink(p);
  }
  if (bad) {
    printf("G121 FAIL: %d\n", bad);
    return 1;
  }
  printf("G121 PASS\n");
  return 0;
}
