/* jpldenum.c -- G132: the JPL DE number behind the automatic tidal term,
 * and the tables a path's files give, are those of the files a context has
 * now, on every thread and context.
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
 * The defect (UPSTREAM-BUGS 145): calc_deltat() read a flag with no
 * ephemeris bit as no ephemeris and took the DE number of the JPL file
 * the context had open, else the default term: swe_deltat_ex(tjd, 0) at
 * 1950 answered 28.927550 s (de200's) on the thread that set de200.eph
 * and 28.931886 (-25.80) on a second thread until it opened the file
 * (4.3 ms), and in AD 1000 1305.30 s or 1463.43, where swe_calc() reads a
 * 0 flag as the Swiss files (SEFLG_DEFAULTEPH) and every UT function sets
 * SEFLG_SWIEPH before its delta t: 28.932196 and 1474.72.
 *
 * The defect (UPSTREAM-BUGS 147): the setting thread's path change sends
 * the tables read along the path back to be read again (in
 * swi_close_keep_topo_etc(): swe_deltat.txt, seleapsec.txt, sefstars.txt
 * and seorbel.txt, the planet-name memo), and a thread adopting the change
 * kept all but the last two: after another thread's path from a directory
 * whose swe_deltat.txt gives 80.0 s for 1990 and whose seleapsec.txt adds
 * a leap second at 2025 Jul 1, to EPHEDIR, its delta t at 1990 Jan 1 was
 * 79.987645 s for the fresh process's 56.856191, swe_utc_to_jd()'s TT of
 * 2026 Jan 1 2461041.50081231 for .50080074 (1 s), and
 * swe_get_planet_name(SE_CUPIDO) the old seorbel.txt's name; and on the
 * setting thread and an explicit context too, delta t at an instant asked
 * before the change stayed 79.987645 s: calc_deltat()'s memo, consulted
 * before init_dt() reads the table again, outlived it.
 *
 * The check, each case in a forked, pristine process: the path and
 * swe_set_jpl_file("de200.eph"); the reader computes a JPL Sun at J2000,
 * which opens the file; then the last setter: none, the path again by
 * another string (EPHEDIR/.), a path without de200.eph (a /tmp directory
 * of links to EPHEDIR's other files), or a JPL file that is not there
 * (de406.eph, the branch for a JPL file alone, as a control); or the first
 * path is a /tmp directory of links with its own swe_deltat.txt,
 * seleapsec.txt, sefstars.txt (Sirius and Aldebaran 10 s of RA on) and
 * seorbel.txt (Cupido 1 deg on, under another name), and the last setter
 * the path to EPHEDIR. The readers: the setting thread, a thread that
 * computed before the last setter and adopts it, a thread started after
 * it, an explicit context given the same calls, and a context made by
 * swe_ctx_new() after it. The observations: swe_deltat_ex() in AD 1700
 * with SEFLG_JPLEPH (and its serr), swe_get_tid_acc(), swe_deltat() there
 * and swe_get_tid_acc() again (bug 137); a TT JPL Sun at J2000, then
 * swe_get_tid_acc(), swe_deltat() in AD 1800 and swe_get_tid_acc() again
 * (bug 138); swe_deltat_ex() in 1950 with iflag 0 (and its serr) and with
 * SEFLG_SWIEPH, the getter, and iflag 0 in AD 1000 (bug 145), where the
 * first two must also agree; and the tables (bug 147): delta t at 1990
 * Jan 1 by swe_deltat_ex() with SEFLG_SWIEPH and by swe_deltat(),
 * swe_utc_to_jd()'s TT of 2026 Jan 1, Sirius by swe_fixstar2() and
 * swe_fixstar(), Aldebaran by swe_fixstar(), Cupido and its name, where
 * the readers that compute first read them before the last setter, delta
 * t at 1990 last (the instant memoised). Each bit for bit as a fresh
 * process given the same setters on its main thread and nothing else, no
 * Sun: the guess follows the configuration, and a calculation does not
 * choose for it (as G128's closecalc). 5 settings x 5 readers x 4
 * observations = 100 checks, and 1 more: the first path's tables, read
 * alone, move every one of those values.
 *
 * Measured before the fixes, 22 of the first 48 (4 settings, the first 4
 * readers, 3 observations) fail. Bug 137's 2: the running thread after
 * the path again and after the path without the file, delta t 3.012003 s
 * for 14.286718. Bug 138's 4: every reader after the path again, the
 * getter -23.8946 for -25.936 before the delta t. Bug 145's 16: iflag 0
 * answers 28.927550 s where the reader has de200.eph open (the setting
 * thread, the running thread and the context after the path and the file
 * alone), 28.931886 elsewhere, for 28.932196; the new thread there also
 * differs from the fresh process. Bug 147's 3 of 101: the setting thread
 * and the explicit context, delta t 79.987645 s for 56.856191 (the memo),
 * and the running thread, that and the TT 1 s late and the name Cupido147
 * for Cupido. The fixes of 137 and 147 are in both builds (the adoption
 * and the memo are this fork's), 138's and 145's in the shipped build
 * only. The compat build (SWE_UPSTREAM_COMPAT) fails 52 of 101, with the
 * fixes and without, by upstream behaviour it restores: bug 145's 25,
 * upstream's 0; bug 137's observation 10 -- its running thread's first
 * delta t reads the DE number before the call syncs (bugs 124, 128), 4
 * cases, and a new thread warns on its first call (bug 110) and it and a
 * context made after guess from the file they have open (bug 103), 6;
 * with a swe_get_tid_acc() first, which syncs, its running thread answers
 * 14.286718 with the fix and 3.012003 without -- bug 138's 14: after the
 * path again and after the tables' path its getter and swe_deltat() both
 * follow the open file (-23.8946, 14.438958 s), 10, and after the path
 * without the file the refused JPL Sun's sync puts the master's -25.80
 * over the moon file's -25.936 on the default context (bug 101's copy of
 * an automatic value), 4; and bug 147's observation 3: swe_fixstar2()'s
 * catalogue kept across the path setter (bug 92), Sirius 104.136831 deg
 * for 104.085304 on the setting thread, the explicit context and, with
 * the fix as the setting thread, the running thread, whose swe_fixstar()
 * read the old sefstars.txt too before the fix. The upstream build
 * (legacy-master) does not build this: its threads share no configuration
 * and adopt nothing. Run as
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
#define T_1950 2433282.5	/* AD 1950 */
#define T_1000 2086308.0	/* AD 1000 */
#define T_1990 2447892.5	/* 1990 Jan 1 */
#define T_WARM 2451545.0
#define NV     7

static char EPHE[PATH_MAX], SAMEDIR[PATH_MAX + 8], NOFILE[] = "/tmp/jpldenumXXXXXX";
static char TABDIR[] = "/tmp/jpldenumtXXXXXX";

/* bug 147: TABDIR's own tables, each moving what it gives */
static const char *DELTAT = "1990 80.0\n";	/* 56.86 s built in */
static const char *LEAPSEC = "20250701\n";	/* a leap second after 2017's */
static const char *STARS =			/* Sirius, Aldebaran 10 s of RA on */
  "Sirius     ,alCMa,ICRS,06,45,18.91728,-16,42,58.0171,-546.01,-1223.07,-5.5,379.21,-1.46,-16, 1591\n"
  "Aldebaran  ,alTau,ICRS,04,36,05.23907,+16,30,33.4885,63.45,-188.94,54.26,48.94,0.86, 16,  629\n";
static const char *ORBEL =			/* Cupido 1 deg on, renamed */
  "J1900, J1900, 164.7409, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, Cupido147\n";
static const char *const TABFILES[] = { "swe_deltat.txt", "seleapsec.txt", "sefstars.txt",
                                        "seorbel.txt", NULL };

enum { K_SET, K_SAMEDIR, K_NOFILE, K_JPLONLY, K_TABLES, NK };
static const char *KN[NK] = {
  "the path and de200.eph", "then the path again (EPHEDIR/.)",
  "then a path without de200.eph", "then a JPL file not there",
  "a path with its own tables then EPHEDIR" };
enum { R_MAIN, R_RUN, R_NEW, R_CTX, R_CTXNEW, NR };
static const char *RN[NR] = { "setting thread", "running thread", "new thread",
                              "explicit context", "context made after" };
enum { O_DT, O_TT, O_F0, O_TAB, NO };
static const char *ON[NO] = { "JPLEPH delta t, the tidal term, swe_deltat()",
                              "a TT JPL Sun, the tidal term, swe_deltat()",
                              "swe_deltat_ex() with no ephemeris bit",
                              "the path's tables: delta t, leap seconds, stars, seorbel.txt" };

struct obs { double v[NV]; char err[AS_MAXCH]; };

static int K, O, REF;
static struct obs out;
static pthread_barrier_t b1, b2;

static int skipped(const char *name, const char *const *skip)
{
  for (; *skip != NULL; skip++)
    if (strcmp(name, *skip) == 0)
      return 1;
  return 0;
}

static int link_all(const char *from, const char *to, const char *const *skip)
{
  DIR *d = opendir(from);
  struct dirent *e;
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (d == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || skipped(e->d_name, skip))
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

static int write_file(const char *dir, const char *name, const char *text)
{
  char p[PATH_MAX + 300];
  FILE *fp;
  snprintf(p, sizeof p, "%s/%s", dir, name);
  if ((fp = fopen(p, "w")) == NULL)
    return -1;
  fputs(text, fp);
  return fclose(fp);
}

static void first_setters(swe_ctx *c)
{
  set_path(c, K == K_TABLES ? TABDIR : EPHE);
  set_jpl(c, "de200.eph");
}

static void last_setter(swe_ctx *c)
{
  switch (K) {
  case K_SAMEDIR: set_path(c, SAMEDIR); break;
  case K_NOFILE: set_path(c, NOFILE); break;
  case K_JPLONLY: set_jpl(c, "de406.eph"); break;
  case K_TABLES: set_path(c, EPHE); break;
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

/* bug 147: what the path's own files give -- swe_deltat.txt,
 * seleapsec.txt, sefstars.txt (both star functions) and seorbel.txt (the
 * body and its name) */
static void tables(swe_ctx *c, struct obs *a)
{
  double x[6], dret[2];
  char star[AS_MAXCH], serr[AS_MAXCH];
  /* the Swiss files' term, the same along both paths: an instant computed
   * before the path change is asked again under the same memo key */
  a->v[0] = (c != NULL ? swe_deltat_ex_r(c, T_1990, SEFLG_SWIEPH, NULL)
                       : swe_deltat_ex(T_1990, SEFLG_SWIEPH, NULL)) * 86400;
  a->v[5] = dt(c, T_1990);
  if ((c != NULL ? swe_utc_to_jd_r(c, 2026, 1, 1, 0, 0, 0, SE_GREG_CAL, dret, serr)
                 : swe_utc_to_jd(2026, 1, 1, 0, 0, 0, SE_GREG_CAL, dret, serr)) == OK)
    a->v[1] = dret[0];
  strcpy(star, "Sirius");
  if ((c != NULL ? swe_fixstar2_r(c, star, T_WARM, SEFLG_SWIEPH, x, serr)
                 : swe_fixstar2(star, T_WARM, SEFLG_SWIEPH, x, serr)) >= 0)
    a->v[2] = x[0];
  strcpy(star, "Sirius");
  if ((c != NULL ? swe_fixstar_r(c, star, T_WARM, SEFLG_SWIEPH, x, serr)
                 : swe_fixstar(star, T_WARM, SEFLG_SWIEPH, x, serr)) >= 0)
    a->v[3] = x[0];
  strcpy(star, "Aldebaran");	/* swe_fixstar() reads it through the file */
  if ((c != NULL ? swe_fixstar_r(c, star, T_WARM, SEFLG_SWIEPH, x, serr)
                 : swe_fixstar(star, T_WARM, SEFLG_SWIEPH, x, serr)) >= 0)
    a->v[6] = x[0];
  if ((c != NULL ? swe_calc_r(c, T_WARM, SE_CUPIDO, SEFLG_SWIEPH, x, serr)
                 : swe_calc(T_WARM, SE_CUPIDO, SEFLG_SWIEPH, x, serr)) >= 0)
    a->v[4] = x[0];
  if (c != NULL)
    swe_get_planet_name_r(c, SE_CUPIDO, a->err);
  else
    swe_get_planet_name(SE_CUPIDO, a->err);
}

/* what a reader computes before the last setter: a JPL Sun at J2000,
 * which opens the file, or the tables, read along the first path, with
 * delta t in 1990 asked last, so that the memo holds it */
static void warm(swe_ctx *c)
{
  struct obs scratch;
  if (O == O_TAB) {
    memset(&scratch, 0, sizeof scratch);
    tables(c, &scratch);
    (void) (c != NULL ? swe_deltat_ex_r(c, T_1990, SEFLG_SWIEPH, NULL)
                      : swe_deltat_ex(T_1990, SEFLG_SWIEPH, NULL));
    (void) dt(c, T_1990);
  } else
    jpl_sun(c);
}

static void observe(swe_ctx *c, struct obs *a)
{
  memset(a, 0, sizeof *a);
  if (O == O_TAB) {
    tables(c, a);
    return;
  }
  if (O == O_DT) {
    a->v[0] = (c != NULL ? swe_deltat_ex_r(c, T_1700, SEFLG_JPLEPH, a->err)
                         : swe_deltat_ex(T_1700, SEFLG_JPLEPH, a->err)) * 86400;
    a->v[1] = tid(c);
    a->v[2] = dt(c, T_1700);
    a->v[3] = tid(c);
    return;
  }
  if (O == O_F0) {		/* bug 145: the default ephemeris, Swiss */
    a->v[0] = (c != NULL ? swe_deltat_ex_r(c, T_1950, 0, a->err)
                         : swe_deltat_ex(T_1950, 0, a->err)) * 86400;
    a->v[1] = (c != NULL ? swe_deltat_ex_r(c, T_1950, SEFLG_SWIEPH, NULL)
                         : swe_deltat_ex(T_1950, SEFLG_SWIEPH, NULL)) * 86400;
    a->v[2] = tid(c);
    a->v[3] = (c != NULL ? swe_deltat_ex_r(c, T_1000, 0, NULL)
                         : swe_deltat_ex(T_1000, 0, NULL)) * 86400;
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
  warm(NULL);			/* the file open, the tables read, on this thread */
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
    warm(NULL);
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
    warm(c);
    last_setter(c);
    observe(c, &out);
    swe_ctx_free(c);
    return 0;
  case R_CTXNEW:		/* inheriting the configuration the setters left */
    first_setters(NULL);
    warm(NULL);
    last_setter(NULL);
    if ((c = swe_ctx_new()) == NULL)
      return -1;
    observe(c, &out);
    swe_ctx_free(c);
    return 0;
  case -2:			/* the first path's tables, read alone */
    first_setters(NULL);
    observe(NULL, &out);
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
  {
    static const char *const NODE200[] = { "de200.eph", NULL };
    if (mkdtemp(NOFILE) == NULL || link_all(EPHE, NOFILE, NODE200) != 0
        || mkdtemp(TABDIR) == NULL || link_all(EPHE, TABDIR, TABFILES) != 0
        || write_file(TABDIR, TABFILES[0], DELTAT) != 0
        || write_file(TABDIR, TABFILES[1], LEAPSEC) != 0
        || write_file(TABDIR, TABFILES[2], STARS) != 0
        || write_file(TABDIR, TABFILES[3], ORBEL) != 0) {
      printf("G132 FAIL: could not make %s and %s\n", NOFILE, TABDIR);
      unlink_all(NOFILE);
      unlink_all(TABDIR);
      return 1;
    }
  }
  printf("G132: the JPL DE number behind the automatic tidal term, and the path's tables\n");
  for (O = 0; O < NO; O++)
    for (K = 0; K < NK; K++) {
      if (get(-1, &ref) != 0) {
        printf("  %s, %s: the reference could not run\n", KN[K], ON[O]);
        bad++;
        continue;
      }
      if (O == O_TAB && K == K_TABLES) {	/* each table moves what it gives */
        n++;
        if (get(-2, &a) != 0) {
          printf("  %s, %s: the first path's tables could not run\n", KN[K], ON[O]);
          bad++;
        } else {
          for (i = 0; i < NV && a.v[i] != ref.v[i]; i++)
            ;
          if (i < NV || strcmp(a.err, ref.err) == 0) {
            printf("  %s, %s: FAIL: the first path's tables do not move value %d\n",
                   KN[K], ON[O], i);
            bad++;
          } else
            printf("  %-34s first path: delta t in 1990 %.6f s, TT %.8f, Sirius %.9f, %s %.9f\n",
                   "", a.v[0], a.v[1], a.v[2], a.err, a.v[4]);
        }
      }
      for (r = 0; r < NR; r++, n++) {
        if (get(r, &a) != 0) {
          printf("  %s, %s, %s: could not run\n", KN[K], ON[O], RN[r]);
          bad++;
          continue;
        }
        if (O == O_F0 && a.v[0] != a.v[1]) {
          printf("  %s, %s, %s: FAIL: 0 answers %.6f s, SEFLG_SWIEPH %.6f s\n",
                 KN[K], ON[O], RN[r], a.v[0], a.v[1]);
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
      else if (O == O_TT)
        printf("  %-34s after a JPL Sun: tidal term %.4f, swe_deltat() in 1800 %.6f s\n",
               KN[K], ref.v[0], ref.v[1]);
      else if (O == O_F0)
        printf("  %-34s flag 0: delta t in 1950 %.6f s, in AD 1000 %.6f s\n",
               KN[K], ref.v[0], ref.v[3]);
      else
        printf("  %-34s tables: delta t in 1990 %.6f s, TT of 2026 UTC %.8f,\n"
               "  %-34s Sirius %.9f and %.9f, %s %.9f deg\n",
               KN[K], ref.v[0], ref.v[1], "", ref.v[2], ref.v[3], ref.err, ref.v[4]);
    }
  unlink_all(NOFILE);
  unlink_all(TABDIR);
  if (bad) {
    printf("G132 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("G132 PASS (%d checks)\n", n);
  return 0;
}
