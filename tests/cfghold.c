/* cfghold.c -- G36: a call takes the configuration in effect at its entry
 * and finishes with it.
 *
 * The defects. A calculation on the default context adopted the shared
 * configuration at every swi_init_swed_if_start() it passed, and one call
 * passes several -- a topocentric one at the observer's swe_deltat_ex() and
 * swe_sidtime0(), swe_calc_ut() at its delta t and again at swe_calc(), a
 * star at the Earth's swe_calc(), a search at each calculation it makes. A
 * setter another thread called in between was adopted in the middle of the
 * call: swi_config_apply() closed the files it was reading and emptied the
 * caches it had filled, and the rest of the call ran on the new
 * configuration, or on a mix of two. A topocentric JPL Jupiter by
 * swe_calc_ut() while another thread switched swe_set_jpl_file() crashed in
 * swejpl.c's state() on the closed file. Some calls synced only after
 * reading part of the configuration, or not at all, and a thread's first
 * call read it from a context nothing had started yet.
 *
 * Three parts, every case in its own process (a crash or a hang is a FAIL,
 * not a lost run).
 *
 * calc -- the file a calculation reads after its entry and before its
 * first nested sync (seorbel.txt for a fictitious body, sefstars.txt for a
 * star) is a FIFO. A worker thread's call opens it and waits for its
 * lines; the main thread, once the open has returned, calls a setter
 * (published to the worker's default context) and only then writes the
 * lines. The answer must equal, bit for bit, a fresh explicit context's
 * with the configuration the call started with:
 *   models  Moshier fictitious body, topocentric; swe_set_astro_models()
 *   jpl     de200 fictitious body, topocentric; swe_set_jpl_file() to a
 *           missing file
 *   tidacc  swe_calc_ut() of that Moshier body in AD 1000;
 *           swe_set_tid_acc(-20)
 *   star    swe_fixstar2_ut() of Sirius, Moshier, topocentric;
 *           swe_set_astro_models()
 * Before the fix: 4.3e-6 deg in latitude, 4.3e-6 (the JPL file closed and
 * reopened half-way), 4.8e-6 deg/day, 1.1e-5 deg.
 *
 * search -- the same with swe_deltat.txt, which a context reads at its
 * first delta t, as the FIFO; the configuration is a directory holding
 * only the fixture, the site 8.55E 47.37N 400 m, Lahiri:
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
 * and for the other half of the hold, that a call adopts at its entry what
 * another thread set before it:
 *   entry      a worker whose context has started waits while the main
 *              thread calls swe_set_delta_t_userdef(0.001), then calls
 *              swe_utc_to_jd(), swe_jdet_to_utc(), swe_deltat_ex() with -1
 *              and with SEFLG_MOSEPH, swe_sidtime(); each must answer as a
 *              fresh context with that delta t
 * Before the fix: eclloc the eclipse of 2003 May 31 with its maximum
 * 3752 s late, its first contact and maximum visible (retflag 5010 for
 * 4242), contacts up to 4990 s off -- neither configuration's answer;
 * eclglob 1.49e-3 s, mooncross 5600 s, rise 1.03 s, houses 0.883 deg in
 * every cusp; entry 22.2 s in TT and UTC, 22.6 s in both delta t.
 *
 * first -- the main thread sets the path and a setter, then a new thread
 * makes a call as its first; the answer must equal, bit for bit, a fresh
 * process's that set the same two and made the call on its main thread:
 *   deltat    swe_set_delta_t_userdef(0.001); swe_deltat_ex(), Swiss
 *   calc_ut   the same; swe_calc_ut() of the Moon with speed
 *   utc       the same; swe_jdut1_to_utc(), the seconds of the day
 *   eclwhere  the same; swe_sol_eclipse_where() at 2003 May 31 4:05 TT
 *   houses    the same; swe_houses() Placidus at 8.55E 47.37N
 *   sidtime0  swe_set_astro_models("1,3,1,1,1,0,0,1"); swe_sidtime0()
 *   models    the same; swe_get_astro_models(NULL, ...), the whole text
 * and what the fix must keep: with no path at all (an empty current
 * directory, no SE_EPHE_PATH), a context's first swe_deltat_ex() still
 * carries "Please call swe_set_ephe_path() ...", on the main thread and on
 * a new one (warn): the hold starts the context now, and the warning keys
 * on the start. Before the fix: deltat 22.6 s (63.83 s for 86.40 s),
 * calc_ut 3.1e-3 deg, utc 22.6 s, eclwhere 0.238 deg in the central line's
 * latitude, houses 2.8e-9 deg, sidtime0 3.1e-7 h, models
 * "-amod0,0,0,0,0,0,0,0" for "-amod1,3,1,1,1,0,0,1".
 *
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
#define TJD_ECL 2452789.0
#define SLON 8.55
#define SLAT 47.37
#define SALT 400.0
#define NOUT 48

static char ephe_abs[PATH_MAX];

/* run fn(k) in a child process: 0 = equal, 1 = differs, 2 = could not run */
static int in_child(int (*fn)(int), int k, const char *name)
{
  pid_t pid;
  int st;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return 1;
  if (pid == 0) {
    int r;
    alarm(20);			/* a backstop: no case takes a second */
    r = fn(k);
    fflush(stdout);
    _exit(r);
  }
  waitpid(pid, &st, 0);
  if (WIFSIGNALED(st)) {
    printf("  %-9s killed by signal %d\n", name, WTERMSIG(st));
    return 1;
  }
  if (WEXITSTATUS(st) == 2)
    printf("  %-9s could not run\n", name);
  return WEXITSTATUS(st) != 0;
}

/* Swap the FIFO for a regular file holding the same lines, so that a call
 * that closed the file under itself and opens it again reads them from
 * there rather than waiting forever, then feed the waiting reader. */
static int feed(int fd, const char *fifo, const char *text)
{
  char tmp[PATH_MAX + 40];
  FILE *fp;
  snprintf(tmp, sizeof tmp, "%s.new", fifo);
  if ((fp = fopen(tmp, "w")) == NULL)
    return -1;
  fputs(text, fp);
  fclose(fp);
  if (rename(tmp, fifo) != 0)
    return -1;
  if (write(fd, text, strlen(text)) != (ssize_t) strlen(text))
    return -1;
  close(fd);
  return 0;
}

static int write_file(const char *dir, const char *name, const char *text)
{
  char p[PATH_MAX + 64];
  FILE *fp;
  snprintf(p, sizeof p, "%s/%s", dir, name);
  if ((fp = fopen(p, "w")) == NULL)
    return -1;
  fputs(text, fp);
  fclose(fp);
  return 0;
}

static int make_fifo(const char *dir, const char *name)
{
  char p[PATH_MAX + 64];
  snprintf(p, sizeof p, "%s/%s", dir, name);
  unlink(p);
  if (mkfifo(p, 0600) != 0) {
    printf("G36 FAIL: mkfifo %s: %s\n", p, strerror(errno));
    return -1;
  }
  return 0;
}

/*======================================================================
 * calc: one calculation, a setter between its entry and its nested syncs
 *====================================================================*/
static const char *ORBEL =
  "J1900, J1900, 163.7409, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, ZZTESTBODY\n";
static const char *STARS =
  "Sirius     ,alCMa,ICRS,06,45,08.91728,-16,42,58.0171,-546.01,-1223.07,-5.5,379.21,-1.46,-16, 1591\n";

enum { A_MODELS, A_JPL, A_TIDACC, A_STAR, NA };
static const char *aname[NA] = {"models", "jpl", "tidacc", "star"};

static char a_fifo[PATH_MAX], a_reg[PATH_MAX];
static int a_cur;
static double a_got[6];
static int32 a_gotrc;
static char a_goterr[AS_MAXCH];

static int32 a_calc(swe_ctx *c, int k, double *x, char *serr)
{
  char star[64];
  int32 fl = SEFLG_MOSEPH | SEFLG_TOPOCTR | SEFLG_SPEED;
  switch (k) {
  case A_MODELS:
    return c ? swe_calc_r(c, TJD, SE_FICT_OFFSET, fl, x, serr)
             : swe_calc(TJD, SE_FICT_OFFSET, fl, x, serr);
  case A_JPL:
    fl = SEFLG_JPLEPH | SEFLG_TOPOCTR | SEFLG_SPEED;
    return c ? swe_calc_r(c, TJD, SE_FICT_OFFSET, fl, x, serr)
             : swe_calc(TJD, SE_FICT_OFFSET, fl, x, serr);
  case A_TIDACC:	/* AD 1000, where the tidal term moves delta t by minutes */
    return c ? swe_calc_ut_r(c, TJD_1000, SE_FICT_OFFSET, fl, x, serr)
             : swe_calc_ut(TJD_1000, SE_FICT_OFFSET, fl, x, serr);
  default:
    strcpy(star, "Sirius");
    return c ? swe_fixstar2_ut_r(c, star, TJD, fl, x, serr)
             : swe_fixstar2_ut(star, TJD, fl, x, serr);
  }
}

static void a_change(int k)
{
  char models[] = "1,3,1,1,1,0,0,1";
  switch (k) {
  case A_MODELS: case A_STAR: swe_set_astro_models(models, 0); break;
  case A_JPL: swe_set_jpl_file("de999.eph"); break;
  default: swe_set_tid_acc(-20.0); break;
  }
}

static void *a_worker(void *arg)
{
  double x[6];
  char serr[AS_MAXCH];
  (void) arg;
  /* first call: this thread's context starts and adopts path, site, models */
  swe_calc(TJD, SE_SUN, SEFLG_MOSEPH, x, serr);
  *a_goterr = '\0';
  a_gotrc = a_calc(NULL, a_cur, a_got, a_goterr);
  return NULL;
}

static int a_case(int k)
{
  const char *file = k == A_STAR ? "sefstars.txt" : "seorbel.txt";
  const char *text = k == A_STAR ? STARS : ORBEL;
  char fifo[PATH_MAX + 32], serr[AS_MAXCH];
  double want[6], d = 0;
  int32 wantrc;
  swe_ctx *c;
  pthread_t th;
  int fd, i, imax = 0;
  a_cur = k;
  /* the reference: a fresh context with the starting configuration */
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, a_reg);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_topo_r(c, SLON, SLAT, SALT);
  wantrc = a_calc(c, k, want, serr);
  swe_ctx_free(c);
  /* the same configuration, process-wide, along the FIFO's directory */
  swe_set_ephe_path(a_fifo);
  swe_set_jpl_file("de200.eph");
  swe_set_topo(SLON, SLAT, SALT);
  snprintf(fifo, sizeof fifo, "%s/%s", a_fifo, file);
  if (pthread_create(&th, NULL, a_worker, NULL) != 0)
    return 2;
  /* returns once the worker's call has opened the FIFO and waits for lines */
  if ((fd = open(fifo, O_WRONLY)) < 0)
    return 2;
  a_change(k);
  if (feed(fd, fifo, text) != 0)
    return 2;
  pthread_join(th, NULL);
  if (a_gotrc != wantrc) {
    printf("  %-9s retflag %d, want %d: %s\n", aname[k], a_gotrc, wantrc, a_goterr);
    return 1;
  }
  for (i = 0; i < 6; i++)
    if (fabs(a_got[i] - want[i]) > d) {
      d = fabs(a_got[i] - want[i]);
      imax = i;
    }
  printf("  %-9s %.2e in x[%d] (lon %.10f, want %.10f)\n", aname[k], d, imax, a_got[0], want[0]);
  return d != 0;
}

static int a_setup(const char *work)
{
  char p[PATH_MAX + 64], q[PATH_MAX + 64];
  if (snprintf(a_fifo, sizeof a_fifo, "%s/calc-fifo", work) >= (int) sizeof a_fifo
      || snprintf(a_reg, sizeof a_reg, "%s/calc-reg", work) >= (int) sizeof a_reg)
    return -1;
  if (mkdir(a_fifo, 0700) != 0 || mkdir(a_reg, 0700) != 0)
    return -1;
  snprintf(q, sizeof q, "%s/de200.eph", ephe_abs);
  snprintf(p, sizeof p, "%s/de200.eph", a_fifo);
  if (symlink(q, p) != 0) return -1;
  snprintf(p, sizeof p, "%s/de200.eph", a_reg);
  if (symlink(q, p) != 0) return -1;
  if (write_file(a_reg, "seorbel.txt", ORBEL) != 0) return -1;
  if (write_file(a_reg, "sefstars.txt", STARS) != 0) return -1;
  return 0;
}

static int part_calc(void)
{
  int k, bad = 0;
  printf("  calc: one calculation, a setter between its entry and its nested syncs\n");
  for (k = 0; k < NA; k++) {
    const char *file = k == A_STAR ? "sefstars.txt" : "seorbel.txt";
    if (make_fifo(a_fifo, file) != 0)
      return NA;
    bad += in_child(a_case, k, aname[k]);
  }
  return bad;
}

/*======================================================================
 * search: a call that makes several calculations, a setter between two
 *====================================================================*/
static const char *DTFILE = "# fixture: the built-in table throughout\n";

enum { B_ECLLOC, B_ECLGLOB, B_MOONX, B_RISE, B_HOUSES, B_ENTRY, NB };
static const char *bname[NB] = {"eclloc", "eclglob", "mooncross", "rise", "houses", "entry"};

static char b_fifo[PATH_MAX], b_reg[PATH_MAX];
static int b_cur, b_in_call;
static double b_got[NOUT];
static int32 b_gotrc;
static char b_goterr[AS_MAXCH];

/* the call under test, on the context c (NULL: this thread's default);
 * out[] gets its answer, isday[] marks the values that are instants */
static int32 b_calc(swe_ctx *c, int k, double *out, int *isday, char *serr)
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
  case B_ECLLOC:	/* two days before the eclipse of 2003 May 31 */
    rc = c ? swe_sol_eclipse_when_loc_r(c, TJD_ECL, fl, geopos, tret, attr, 0, serr)
           : swe_sol_eclipse_when_loc(TJD_ECL, fl, geopos, tret, attr, 0, serr);
    for (i = 0; i < 7; i++, n++) {
      out[n] = tret[i];
      isday[n] = 1;
    }
    for (i = 0; i < 11; i++)
      out[n++] = attr[i];
    break;
  case B_ECLGLOB:
    rc = c ? swe_sol_eclipse_when_glob_r(c, TJD, fl, 0, tret, 0, serr)
           : swe_sol_eclipse_when_glob(TJD, fl, 0, tret, 0, serr);
    for (i = 0; i < 10; i++, n++) {
      out[n] = tret[i];
      isday[n] = 1;
    }
    break;
  case B_MOONX:
    fl |= SEFLG_SIDEREAL;
    t = c ? swe_mooncross_ut_r(c, 0.0, TJD, fl, serr)
          : swe_mooncross_ut(0.0, TJD, fl, serr);
    out[0] = t;
    isday[0] = 1;
    rc = t < TJD ? ERR : OK;
    break;
  case B_RISE:
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
static void b_start(swe_ctx *c, const char *dir)
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
static void b_change(swe_ctx *c, int k)
{
  char models[] = "1,3,1,1,1,0,0,1";
  switch (k) {
  case B_ECLLOC:
    if (c) swe_set_topo_r(c, -74.0, 40.7, 10.0); else swe_set_topo(-74.0, 40.7, 10.0);
    break;
  case B_ECLGLOB:
    if (c) swe_set_astro_models_r(c, models, 0); else swe_set_astro_models(models, 0);
    break;
  case B_RISE:
    if (c) swe_set_delta_t_userdef_r(c, 0.001); else swe_set_delta_t_userdef(0.001);
    break;
  default:
    if (c) swe_set_sid_mode_r(c, SE_SIDM_FAGAN_BRADLEY, 0, 0);
    else swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
    break;
  }
}

static void *b_worker(void *arg)
{
  double x[6];
  int isday[NOUT];
  char serr[AS_MAXCH];
  (void) arg;
  /* first call: this thread's context starts and adopts path, site, mode;
   * an instant in TT, no delta t */
  swe_calc(TJD, SE_SUN, SEFLG_MOSEPH, x, serr);
  __atomic_store_n(&b_in_call, 1, __ATOMIC_SEQ_CST);
  *b_goterr = '\0';
  b_gotrc = b_calc(NULL, b_cur, b_got, isday, b_goterr);
  return NULL;
}

/* the largest differences of a against b: instants in s, the rest in deg;
 * an instant one of them has and the other not (0) is counted in *nmiss */
static void b_diff(const double *a, const double *b, const int *isday, double *ds, double *dd,
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

static int b_case(int k)
{
  char fifo[PATH_MAX + 32], serr[AS_MAXCH];
  double want[NOUT], newer[NOUT], ds, dd, ns, nd;
  int isday[NOUT], dmiss, nmiss;
  int32 wantrc, newrc;
  swe_ctx *c;
  pthread_t th;
  int fd;
  b_cur = k;
  /* the reference: a fresh context with the starting configuration */
  c = swe_ctx_new();
  b_start(c, b_reg);
  wantrc = b_calc(c, k, want, isday, serr);
  swe_ctx_free(c);
  /* and one with the setter's, for the record */
  c = swe_ctx_new();
  b_start(c, b_reg);
  b_change(c, k);
  newrc = b_calc(c, k, newer, isday, serr);
  swe_ctx_free(c);
  /* the same configuration, process-wide, along the FIFO's directory */
  b_start(NULL, b_fifo);
  snprintf(fifo, sizeof fifo, "%s/swe_deltat.txt", b_fifo);
  if (pthread_create(&th, NULL, b_worker, NULL) != 0)
    return 2;
  /* returns once the worker's call has opened the FIFO and waits for lines */
  if ((fd = open(fifo, O_WRONLY)) < 0)
    return 2;
  if (!__atomic_load_n(&b_in_call, __ATOMIC_SEQ_CST)) {
    printf("  %-9s the FIFO was opened before the call under test\n", bname[k]);
    return 2;
  }
  b_change(NULL, k);
  if (feed(fd, fifo, DTFILE) != 0)
    return 2;
  pthread_join(th, NULL);
  b_diff(b_got, want, isday, &ds, &dd, &dmiss);
  b_diff(b_got, newer, isday, &ns, &nd, &nmiss);
  printf("  %-9s %.2e s, %.2e deg, %d instants missing or extra (%.9f, want %.9f);\n"
         "            against the setter's configuration %.2e s, %.2e deg, %d\n",
         bname[k], ds, dd, dmiss, b_got[0], want[0], ns, nd, nmiss);
  if (b_gotrc != wantrc) {
    printf("  %-9s retflag %d, want %d (the setter's %d) %s\n", bname[k], b_gotrc,
           wantrc, newrc, b_goterr);
    return 1;
  }
  return ds != 0 || dd != 0 || dmiss != 0;
}

/* The other half of the hold: a call adopts at its entry what another
 * thread set before it. A worker whose default context has started (and
 * read swe_deltat.txt) waits while the main thread calls
 * swe_set_delta_t_userdef(0.001), then converts UTC both ways and asks for
 * delta t; each answer must equal a fresh context's with that delta t. */
static pthread_barrier_t b_bar;
static double b_eget[6];

static void b_entry_calls(swe_ctx *c, double *r)
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

static void *b_entry_worker(void *arg)
{
  double x[6], r[6];
  char serr[AS_MAXCH];
  (void) arg;
  swe_calc(TJD, SE_SUN, SEFLG_MOSEPH, x, serr);
  b_entry_calls(NULL, r);		/* the context reads the table */
  pthread_barrier_wait(&b_bar);
  pthread_barrier_wait(&b_bar);	/* the main thread's setter in between */
  b_entry_calls(NULL, b_eget);
  return NULL;
}

static int b_entry(int k)
{
  static const char *what[6] = {"utc_to_jd TT", "utc_to_jd UT1", "jdet_to_utc",
                                "deltat_ex(-1)", "deltat_ex", "sidtime"};
  double want[6];
  pthread_t th;
  swe_ctx *c;
  int i, bad = 0;
  (void) k;
  c = swe_ctx_new();
  b_start(c, b_reg);
  swe_set_delta_t_userdef_r(c, 0.001);
  b_entry_calls(c, want);
  swe_ctx_free(c);
  b_start(NULL, b_reg);
  pthread_barrier_init(&b_bar, NULL, 2);
  if (pthread_create(&th, NULL, b_entry_worker, NULL) != 0)
    return 2;
  pthread_barrier_wait(&b_bar);
  swe_set_delta_t_userdef(0.001);
  pthread_barrier_wait(&b_bar);
  pthread_join(th, NULL);
  for (i = 0; i < 6; i++)
    if (b_eget[i] != want[i]) {
      printf("  %-9s %s %.2e off (%.9f, want %.9f)\n", bname[B_ENTRY], what[i],
             fabs(b_eget[i] - want[i]), b_eget[i], want[i]);
      bad = 1;
    }
  if (!bad)
    printf("  %-9s the setter made before the calls: adopted by all six\n", bname[B_ENTRY]);
  return bad;
}

static int b_setup(const char *work)
{
  if (snprintf(b_fifo, sizeof b_fifo, "%s/search-fifo", work) >= (int) sizeof b_fifo
      || snprintf(b_reg, sizeof b_reg, "%s/search-reg", work) >= (int) sizeof b_reg)
    return -1;
  if (mkdir(b_fifo, 0700) != 0 || mkdir(b_reg, 0700) != 0)
    return -1;
  return write_file(b_reg, "swe_deltat.txt", DTFILE);
}

static int part_search(void)
{
  int k, bad = 0;
  printf("  search: a call that makes several calculations, a setter between two\n");
  for (k = 0; k < NB; k++) {
    if (make_fifo(b_fifo, "swe_deltat.txt") != 0)
      return NB;
    bad += in_child(k == B_ENTRY ? b_entry : b_case, k, bname[k]);
  }
  return bad;
}

/*======================================================================
 * first: a new thread's first call after the main thread's setter
 *====================================================================*/
#define WARN "Please call swe_set_ephe_path()"

enum { C_DELTAT, C_CALCUT, C_UTC, C_ECLWHERE, C_HOUSES, C_SIDTIME0, C_MODELS, C_WARN, NC };
static const char *cname[NC] = {"deltat", "calc_ut", "utc", "eclwhere", "houses",
                                "sidtime0", "models", "warn"};
static const char *cunit[NC] = {"s", "deg", "s", "deg", "deg", "h", "", ""};
static const int shown[NC] = {0, 1, 0, 1, 1, 0, 0, 0};	/* the value printed */

static char c_empty[PATH_MAX];

struct ans {
  double v[NOUT];
  char s[AS_MAXCH * 16];	/* swe_get_astro_models() writes some 530 */
};

static int c_cur;
static struct ans c_ans;

/* the call under test, on this thread's default context */
static void c_call(int k, struct ans *a)
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
    a->v[0] = swe_houses(TJD, SLAT, SLON, 'P', cusp, ascmc);
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
static void c_setters(int k)
{
  char models[] = "1,3,1,1,1,0,0,1";
  if (k == C_WARN) {
    if (chdir(c_empty) != 0)
      _exit(2);
    return;
  }
  swe_set_ephe_path(ephe_abs);
  if (k == C_SIDTIME0 || k == C_MODELS)
    swe_set_astro_models(models, 0);
  else
    swe_set_delta_t_userdef(0.001);
}

static void *c_worker(void *arg)
{
  (void) arg;
  c_call(c_cur, &c_ans);
  return NULL;
}

/* in a child: the setters, then the call on a new thread (thr) or here */
static int c_get(int k, int thr, struct ans *a)
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
    c_cur = k;
    c_setters(k);
    if (thr) {
      if (pthread_create(&th, NULL, c_worker, NULL) != 0)
        _exit(2);
      pthread_join(th, NULL);
    } else
      c_call(k, &c_ans);
    if (write(p[1], &c_ans, sizeof c_ans) != (ssize_t) sizeof c_ans)
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

static int c_case(int k)
{
  static struct ans t, m;
  double worst = 0, scale = 1;
  int i, at = 0;
  if (c_get(k, 1, &t) != 0 || c_get(k, 0, &m) != 0) {
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
           cunit[k], at, t.v[at], m.v[at]);
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
    printf("  %-9s a new thread's first call as a fresh process's (%.12g)\n", cname[k],
           t.v[shown[k]]);
  return 0;
}

static int c_setup(const char *work)
{
  if (snprintf(c_empty, sizeof c_empty, "%s/first-empty", work) >= (int) sizeof c_empty)
    return -1;
  return mkdir(c_empty, 0700);
}

static int part_first(void)
{
  int k, bad = 0;
  printf("  first: a new thread's first call after the main thread's setter\n");
  for (k = 0; k < NC; k++)
    bad += c_case(k);
  return bad;
}

/* split -- swe_split_deg() with SE_SPLIT_DEG_NAKSHATRA reads the sidereal
 * mode; a new thread's first call must see the mode the main thread set. */
static void *s_worker(void *arg)
{
  int32 *r = arg;
  double f;
  swe_split_deg(10.0, SE_SPLIT_DEG_NAKSHATRA, &r[0], &r[1], &r[2], &f, &r[3]);
  return NULL;
}

static int s_case(int k)
{
  int32 m[4], w[4];
  double f;
  pthread_t th;
  (void) k;
  swe_set_sid_mode(SE_SIDM_TRUE_SHEORAN, 0, 0);
  swe_split_deg(10.0, SE_SPLIT_DEG_NAKSHATRA, &m[0], &m[1], &m[2], &f, &m[3]);
  if (pthread_create(&th, NULL, s_worker, w) != 0)
    return 2;
  pthread_join(th, NULL);
  printf("  split     10 deg under True Sheoran: main nakshatra %d %d deg %d', worker nakshatra %d %d deg %d'%s\n",
         m[3], m[0], m[1], w[3], w[0], w[1],
         memcmp(m, w, sizeof m) ? "  FAIL" : "");
  return memcmp(m, w, sizeof m) != 0;
}

static int part_split(void)
{
  return in_child(s_case, 0, "split");
}

int main(int argc, char **argv)
{
  char work_abs[PATH_MAX];
  int bad = 0;
  if (argc != 3) {
    fprintf(stderr, "usage: cfghold EPHEDIR WORKDIR\n");
    return 2;
  }
  if (realpath(argv[1], ephe_abs) == NULL || realpath(argv[2], work_abs) == NULL
      || a_setup(work_abs) != 0 || b_setup(work_abs) != 0 || c_setup(work_abs) != 0) {
    printf("G36 FAIL: cannot set up %s\n", argv[2]);
    return 1;
  }
  printf("G36: a call takes the configuration in effect at its entry and finishes with it\n");
  bad += part_calc();
  bad += part_search();
  bad += part_first();
  bad += part_split();
  if (bad) {
    printf("G36 FAIL: %d\n", bad);
    return 1;
  }
  printf("G36 PASS (%d cases)\n", NA + NB + NC + 1);
  return 0;
}
