/* ephswitch.c -- G91: what a path or ephemeris switch makes stale is
 * emptied, and a manual tidal acceleration survives it.
 *
 * The defects (UPSTREAM-BUGS 92), each an answer that depended on what
 * was asked before it:
 *
 * 1. The fixed-star memos in sweph.c -- swe_fixstar()'s and
 *    swe_fixstar_mag()'s last record (sp.fs1, sp.fs1mag), swe_fixstar2()'s
 *    and swe_fixstar2_mag()'s last star (sp.fs2, sp.fs2mag) -- outlived
 *    swe_close() and a path to another sefstars.txt, and the catalogue
 *    swe_fixstar2() reads into memory outlived the path: Sirius stayed at
 *    -1.46 and 2.64 pc under a catalogue giving -1.00 and 3.33 pc, and at
 *    -1.00 after the path back. A thread adopting another thread's path
 *    kept the old star file and catalogue, and looked its star up before
 *    adopting it.
 * 2. A manual tidal acceleration was reset to automatic by every path
 *    change -- swe_set_ephe_path(), swe_set_jpl_file(), and the implicit
 *    path the first calculation sets when the files are named by
 *    SE_EPHE_PATH -- where the manual says that once swe_set_tid_acc()
 *    has been used the automatic setting is blocked until
 *    swe_set_tid_acc(SE_TIDAL_AUTOMATIC): -25.936 for DE406's -25.826, and
 *    swe_deltat() in AD 1000 1474.79 s for 1465.66.
 * 3. The topocentric observer was kept for the instant alone, but it is
 *    placed by the delta t of the request's ephemeris and, in this fork,
 *    framed by its DE number (the J2000 -> ICRS bias for DE403 and later):
 *    a topocentric request at the instant of one on another ephemeris took
 *    that one's site. De200 after Swiss at 2000 Jan 1 12h, the Moon 3.7e-8
 *    deg in right ascension, 3.9e-8 in azimuth, 4.1e-7 deg/day in its
 *    latitude speed; at TT 1600 Jan 1 4.4" (0.00122 deg), at 1000 Moshier
 *    after Swiss 5.4".
 *
 * The check, each answer against the same request in a fresh process
 * (fork):
 *   stars   one sequence on the default context -- path A, swe_close() and
 *           path B, path A, path B, swe_close() and path A -- asking Sirius,
 *           Aldebaran and Sirius again with each of the four star
 *           functions (return code, magnitude, the J2000 ICRS geometric
 *           place and distance); catalogue B, written to a temporary
 *           directory, gives both stars other magnitudes and parallaxes.
 *           And the same with a second thread asking while the first sets
 *           the paths.
 *   tidal   swe_set_tid_acc(SE_TIDAL_DE406) and then the files named by
 *           SE_EPHE_PATH and a calculation, a second swe_set_ephe_path(),
 *           swe_set_jpl_file(), a de200 swe_calc_ut(): the tidal
 *           acceleration and swe_deltat() in AD 1000 as with the setting
 *           alone; then swe_close() and a path, and
 *           swe_set_tid_acc(SE_TIDAL_AUTOMATIC): as with no setting.
 *   ephem   at 2000 Jan 1 12h, 1600 Jan 1 and 1000 Jan 1 (no de200), each
 *           of a topocentric equatorial Moon in UT and in TT, a topocentric
 *           Moon in UT with speeds, the swe_azalt() of the first, and
 *           swe_houses_ex(), on each of Swiss, Moshier and de200, right
 *           after a topocentric Moon in UT or in TT at the same instant on
 *           each other ephemeris.
 * Within 1e-10 (deg, AU, deg/day, s). After the fix all 156 pass.
 *
 * Measured before it, 34 of 156 fail: stars 6 of 9 (after each change of
 * catalogue, Sirius's distance 2.64 pc for 3.33 or the reverse, and on the
 * second thread every answer after each path B), tidal 4 of 7 (SE_EPHE_PATH,
 * a second path, swe_set_jpl_file(), a calculation and a path: 1474.79 s
 * for 1465.66, 1305.37 after the de200 file), ephem 24 of 140 (at 2000
 * every topocentric request between de200 and the others, 3.7e-8 deg in
 * right ascension, 3.9e-8 in azimuth, 4.1e-7 deg/day; at 1600 and 1000 the
 * TT requests, up to 0.00149 deg). The compat build fails 18: those stars
 * and tidal cases, and ephem 8 (the delta t cases at 1600 and 1000; it
 * has no frame bias). The upstream build (legacy-master) fails 18: stars
 * 8 of 9 (its second thread finds no path at all), tidal 4, ephem 6 (up to
 * 0.00856 deg at 1000). Run as
 *   ephswitch EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include "swephexp.h"

#define TOL 1e-10
#define NV 64
#define T_STAR 2451545.0
#define T_1000 2086302.5	/* 1000 Jan 1, where the tidal term is 9 s */

static const char *EPHE;
static char PATH_B[2 * AS_MAXCH];

struct ans { int n; double v[NV]; };

static void put(struct ans *a, double x)
{
  if (a->n < NV)
    a->v[a->n++] = x;
}

/* run fn(k, out) in a forked child, which starts from this process's
 * state: a library nobody has called yet */
static int in_child(void (*fn)(int, struct ans *), int k, struct ans *out, int nout)
{
  int fd[2];
  ssize_t want = (ssize_t) (nout * sizeof *out), got = 0, r;
  pid_t pid;
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    close(fd[0]);
    memset(out, 0, (size_t) want);
    fn(k, out);
    got = 0;
    while (got < want && (r = write(fd[1], (char *) out + got, want - got)) > 0)
      got += r;
    _exit(got == want ? 0 : 3);
  }
  close(fd[1]);
  while (got < want && (r = read(fd[0], (char *) out + got, want - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return got == want ? 0 : -1;
}

static int compare(const char *what, const struct ans *s, const struct ans *f)
{
  double d = 0;
  int j, at = -1;
  for (j = 0; j < f->n; j++) {
    double e = fabs(s->v[j] - f->v[j]);
    if (!(e <= d)) { d = e; at = j; }
  }
  if (s->n != f->n || d > TOL) {
    printf("  %-44s FAIL: [%d] %.10g, fresh %.10g (off by %.3g)\n", what, at,
           at >= 0 ? s->v[at] : 0, at >= 0 ? f->v[at] : 0, d);
    return 1;
  }
  return 0;
}

/*----------------------------------------------------------------------
 * stars
 *--------------------------------------------------------------------*/
static void ask_star(const char *name, struct ans *a)
{
  static const int32 fl = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_TRUEPOS
    | SEFLG_NOABERR | SEFLG_NOGDEFL | SEFLG_EQUATORIAL;
  char s[AS_MAXCH], serr[AS_MAXCH];
  double m, x[6];
  int j;
  strcpy(s, name);
  put(a, swe_fixstar_mag(s, &m, serr));
  put(a, m);
  strcpy(s, name);
  put(a, swe_fixstar2_mag(s, &m, serr));
  put(a, m);
  strcpy(s, name);
  put(a, swe_fixstar(s, T_STAR, fl, x, serr));
  for (j = 0; j < 3; j++)
    put(a, x[j]);
  strcpy(s, name);
  put(a, swe_fixstar2(s, T_STAR, fl, x, serr));
  for (j = 0; j < 3; j++)
    put(a, x[j]);
}

static void ask_stars(struct ans *a)
{
  ask_star("sirius", a);
  ask_star("aldebaran", a);
  ask_star("sirius", a);
}

/* the sequence's steps: swe_close() first or not, and the catalogue */
static const struct { int close_first, cat_b; const char *what; } S[] = {
  { 0, 0, "stars, path A" },
  { 1, 1, "stars, swe_close() and path B" },
  { 0, 0, "stars, path A again" },
  { 0, 1, "stars, path B again" },
  { 1, 0, "stars, swe_close() and path A" },
};
#define NS ((int) (sizeof S / sizeof S[0]))

static const char *cat_path(int b)
{
  return b ? PATH_B : EPHE;
}

static void stars_fresh(int k, struct ans *a)
{
  swe_set_ephe_path(cat_path(S[k].cat_b));
  ask_stars(a);
}

static void stars_sequence(int k, struct ans *a)
{
  int i;
  (void) k;
  for (i = 0; i < NS; i++) {
    if (S[i].close_first)
      swe_close();
    swe_set_ephe_path(cat_path(S[i].cat_b));
    ask_stars(&a[i]);
  }
}

/* The same steps with the asking done on a second thread, whose default
 * context adopts the paths the first thread sets. Without swe_close():
 * that is the whole library's, not a thread's. */
static const int ST[] = { 0, 1, 0, 1 };
#define NST ((int) (sizeof ST / sizeof ST[0]))
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int turn;	/* even: the setter's, odd: the asker's */
static struct ans *thread_out;

static void wait_turn(int t)
{
  pthread_mutex_lock(&mu);
  while (turn != t)
    pthread_cond_wait(&cv, &mu);
  pthread_mutex_unlock(&mu);
}

static void next_turn(void)
{
  pthread_mutex_lock(&mu);
  turn++;
  pthread_cond_broadcast(&cv);
  pthread_mutex_unlock(&mu);
}

static void *asker(void *arg)
{
  int i;
  (void) arg;
  for (i = 0; i < NST; i++) {
    wait_turn(2 * i + 1);
    if (i == 0) {	/* a context that has calculated: what follows is adoption */
      double x[6];
      char serr[AS_MAXCH];
      swe_calc_ut(T_STAR, SE_SUN, SEFLG_SWIEPH, x, serr);
    }
    ask_stars(&thread_out[i]);
    next_turn();
  }
  return NULL;
}

static void stars_thread_fresh(int k, struct ans *a)
{
  swe_set_ephe_path(cat_path(ST[k]));
  ask_stars(a);
}

static void stars_thread_sequence(int k, struct ans *a)
{
  pthread_t th;
  int i;
  (void) k;
  thread_out = a;
  turn = 0;
  if (pthread_create(&th, NULL, asker, NULL) != 0)
    return;
  for (i = 0; i < NST; i++) {
    wait_turn(2 * i);
    swe_set_ephe_path(cat_path(ST[i]));
    next_turn();
  }
  pthread_join(th, NULL);
}

/*----------------------------------------------------------------------
 * tidal acceleration
 *--------------------------------------------------------------------*/
static const struct { int manual; const char *what; } TS[] = {
  { 1, "tidal: set, then SE_EPHE_PATH and a calc" },
  { 1, "tidal: path, set, path" },
  { 1, "tidal: path, set, swe_set_jpl_file()" },
  { 1, "tidal: path, jpl file, set, de200 calc" },
  { 1, "tidal: path, set, Swiss calc, path" },
  { 0, "tidal: path, set, swe_close(), path" },
  { 0, "tidal: path, set, path, set automatic" },
};
#define NTS ((int) (sizeof TS / sizeof TS[0]))

/* after a calculation, which derives an automatic value from the files */
static void tidal_observe(struct ans *a)
{
  double x[6];
  char serr[AS_MAXCH];
  swe_calc_ut(T_1000, SE_MOON, SEFLG_SWIEPH, x, serr);
  put(a, x[0]);
  put(a, swe_get_tid_acc());
  put(a, swe_deltat(T_1000) * 86400);
}

static void tidal_scenario(int k, struct ans *a)
{
  double x[6];
  char serr[AS_MAXCH];
  switch (k) {
  case 0:
    setenv("SE_EPHE_PATH", EPHE, 1);
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_calc_ut(T_1000, SE_MOON, SEFLG_SWIEPH, x, serr);
    break;
  case 1:
    swe_set_ephe_path(EPHE);
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_set_ephe_path(EPHE);
    break;
  case 2:
    swe_set_ephe_path(EPHE);
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_set_jpl_file("de200.eph");
    break;
  case 3:
    swe_set_ephe_path(EPHE);
    swe_set_jpl_file("de200.eph");
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_calc_ut(2451545.0, SE_MOON, SEFLG_JPLEPH, x, serr);
    break;
  case 4:
    swe_set_ephe_path(EPHE);
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_calc_ut(T_1000, SE_MOON, SEFLG_SWIEPH, x, serr);
    swe_set_ephe_path(EPHE);
    break;
  case 5:
    swe_set_ephe_path(EPHE);
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_close();
    swe_set_ephe_path(EPHE);
    break;
  case 6:
    swe_set_ephe_path(EPHE);
    swe_set_tid_acc(SE_TIDAL_DE406);
    swe_set_ephe_path(EPHE);
    swe_set_tid_acc(SE_TIDAL_AUTOMATIC);
    break;
  }
  tidal_observe(a);
}

/* what the scenario should leave: the setting alone, or no setting */
static void tidal_reference(int manual, struct ans *a)
{
  swe_set_ephe_path(EPHE);
  if (manual)
    swe_set_tid_acc(SE_TIDAL_DE406);
  tidal_observe(a);
}

/*----------------------------------------------------------------------
 * the observer across ephemerides
 *--------------------------------------------------------------------*/
static const int32 EPH[3] = { SEFLG_SWIEPH, SEFLG_MOSEPH, SEFLG_JPLEPH };
static const char *EN[3] = { "Swiss", "Moshier", "de200" };
static const double T[3] = { 2451545.0, 2305447.5, T_1000 };
static const char *TN[3] = { "2000", "1600", "1000" };
#define NOP 5
static const char *ON[NOP] = { "Moon UT", "Moon TT", "Moon UT speed", "azalt", "houses" };
static const char *PN[2] = { "UT", "TT" };
static double GEO[3] = { 8.55, 47.37, 400 };

static void open_lib(void)
{
  swe_set_ephe_path(EPHE);
  swe_set_jpl_file("de200.eph");
  swe_set_topo(GEO[0], GEO[1], GEO[2]);
}

static void eph_op(int o, int e, double t, struct ans *a)
{
  static const int32 topo = SEFLG_TOPOCTR | SEFLG_EQUATORIAL;
  double x[6], xaz[3], cusp[13], ascmc[10];
  char serr[AS_MAXCH];
  int j;
  switch (o) {
  case 0:
    put(a, swe_calc_ut(t, SE_MOON, EPH[e] | topo, x, serr));
    for (j = 0; j < 3; j++)
      put(a, x[j]);
    break;
  case 1:
    put(a, swe_calc(t, SE_MOON, EPH[e] | topo, x, serr));
    for (j = 0; j < 3; j++)
      put(a, x[j]);
    break;
  case 2:
    put(a, swe_calc_ut(t, SE_MOON, EPH[e] | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr));
    for (j = 0; j < 6; j++)
      put(a, x[j]);
    break;
  case 3:
    swe_calc_ut(t, SE_MOON, EPH[e] | topo, x, serr);
    swe_azalt(t, SE_EQU2HOR, GEO, 1013.25, 15, x, xaz);
    for (j = 0; j < 3; j++)
      put(a, xaz[j]);
    break;
  case 4:
    put(a, swe_houses_ex(t, EPH[e], GEO[1], GEO[0], 'P', cusp, ascmc));
    put(a, cusp[1]);
    put(a, cusp[10]);
    put(a, ascmc[2]);
    break;
  }
}

/* k packs instant, op, ephemeris, and the prior request: none (-1), or a
 * topocentric Moon in UT (0) or TT (1) on ephemeris pe */
#define PACK(ti, o, e, p, pe) (((((ti) * NOP + (o)) * 3 + (e)) * 3 + ((p) + 1)) * 3 + (pe))
static void eph_run(int k, struct ans *a)
{
  int pe = k % 3, p = (k / 3) % 3 - 1, e = (k / 9) % 3, o = (k / 27) % NOP, ti = k / (27 * NOP);
  struct ans scratch;
  open_lib();
  if (p >= 0) {
    memset(&scratch, 0, sizeof scratch);
    eph_op(p, pe, T[ti], &scratch);
  }
  eph_op(o, e, T[ti], a);
}

int main(int argc, char **argv)
{
  static struct ans seq[NS > NST ? NS : NST], one, two;
  static char CATDIR[] = "/tmp/ephswitch-XXXXXX";
  char catfile[sizeof CATDIR + 16], what[128];
  FILE *fp;
  int i, ti, o, e, p, pe, n = 0, nfail = 0;
  int sfail = 0, tfail = 0, efail = 0, ns = 0, nt = 0, ne = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  if (mkdtemp(CATDIR) == NULL) {
    printf("G91 FAIL: cannot make the second catalogue's directory\n");
    return 1;
  }
  snprintf(catfile, sizeof catfile, "%s/sefstars.txt", CATDIR);
  if ((fp = fopen(catfile, "w")) == NULL) {
    printf("G91 FAIL: cannot write the second catalogue\n");
    return 1;
  }
  fputs("Sirius     ,alCMa,ICRS,06,45,08.91728,-16,42,58.0171,-546.01,-1223.07,-5.5,300.00,-1.00,-16, 1591\n"
        "Aldebaran  ,alTau,ICRS,04,35,55.23907,+16,30,33.4885,63.45,-188.94,54.26,40.00,0.50, 16,  629\n", fp);
  fclose(fp);
  snprintf(PATH_B, sizeof PATH_B, "%s:%s", CATDIR, EPHE);

  /* stars, one thread */
  if (in_child(stars_sequence, 0, seq, NS) != 0) {
    printf("  stars: the sequence did not run\n");
    sfail++;
  } else {
    for (i = 0; i < NS; i++, ns++) {
      if (in_child(stars_fresh, i, &one, 1) != 0) {
        printf("  %-44s the fresh request did not run\n", S[i].what);
        sfail++;
        continue;
      }
      sfail += compare(S[i].what, &seq[i], &one);
    }
  }
  /* stars, a second thread asking */
  if (in_child(stars_thread_sequence, 0, seq, NST) != 0) {
    printf("  stars: the thread sequence did not run\n");
    sfail++;
  } else {
    for (i = 0; i < NST; i++, ns++) {
      snprintf(what, sizeof what, "stars, thread, path %s (%d)", ST[i] ? "B" : "A", i);
      if (in_child(stars_thread_fresh, i, &one, 1) != 0) {
        printf("  %-44s the fresh request did not run\n", what);
        sfail++;
        continue;
      }
      sfail += compare(what, &seq[i], &one);
    }
  }
  remove(catfile);
  rmdir(CATDIR);

  /* tidal acceleration */
  for (i = 0; i < NTS; i++, nt++) {
    if (in_child(tidal_scenario, i, &one, 1) != 0
        || in_child(tidal_reference, TS[i].manual, &two, 1) != 0) {
      printf("  %-44s did not run\n", TS[i].what);
      tfail++;
      continue;
    }
    tfail += compare(TS[i].what, &one, &two);
  }

  /* the observer across ephemerides */
  for (ti = 0; ti < 3; ti++)
    for (o = 0; o < NOP; o++)
      for (e = 0; e < 3; e++) {
        if (e == 2 && ti == 2)
          continue;	/* de200 starts in 1599 */
        if (in_child(eph_run, PACK(ti, o, e, -1, 0), &two, 1) != 0) {
          printf("  ephem %s %s %s: the fresh request did not run\n", TN[ti], ON[o], EN[e]);
          efail++;
          continue;
        }
        for (p = 0; p < 2; p++)
          for (pe = 0; pe < 3; pe++) {
            if (pe == e || (pe == 2 && ti == 2))
              continue;
            ne++;
            snprintf(what, sizeof what, "ephem %s %s %s after Moon %s %s",
                     TN[ti], ON[o], EN[e], PN[p], EN[pe]);
            if (in_child(eph_run, PACK(ti, o, e, p, pe), &one, 1) != 0) {
              printf("  %-44s did not run\n", what);
              efail++;
              continue;
            }
            efail += compare(what, &one, &two);
          }
      }
  n = ns + nt + ne;
  nfail = sfail + tfail + efail;
  if (nfail)
    printf("G91 FAIL: %d of %d (stars %d of %d, tidal %d of %d, ephem %d of %d)\n",
           nfail, n, sfail, ns, tfail, nt, efail, ne);
  else
    printf("G91 PASS (%d: stars %d, tidal %d, ephem %d)\n", n, ns, nt, ne);
  return nfail != 0;
}
