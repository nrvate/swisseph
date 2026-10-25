/* closetid.c -- G127: after swe_close() the kept path gives the first
 * calculation the automatic tidal term a fresh process with that path
 * gives it, on every thread and context.
 *
 * The defect (UPSTREAM-BUGS 131): swe_close() keeps the path and the JPL
 * file's name, as upstream's does, and forgets the DE numbers the setters
 * read from them (the 2026-09-16 review's F2; ctx_release(), and the
 * shared configuration's reset after it, bug 123), so the first delta t
 * after it, before any file is open, took the default tidal term (-25.80)
 * for the moon file's (-25.936): delta t in AD 1000 1463.43 s for
 * 1474.72, swe_calc_ut()'s Moon there 197.3747177 deg on the first call
 * and 197.3763253 on the second (1.6e-3 deg), swe_get_tid_acc() -25.80.
 * With de200.eph set, delta t in AD 1700 14.29 s for 3.01, the JPL Moon
 * 1.8e-3 deg off, swe_get_tid_acc() -25.80 for -23.8946. And where a TT
 * calculation came first, the getter and the next delta t disagreed,
 * where bug 101 has the getter answer the term the next delta t uses:
 * after a JPL Sun the getter said de200's -23.8946 and the next
 * swe_deltat(), guessing the Swiss ephemeris, used -25.80 (18.604686 s in
 * AD 1800 for 14.438958), the getter -25.80 after it; after a Swiss Sun,
 * which opens the moon file for the Earth, the getter -25.80 and the delta
 * t -25.936 (found by bug 132's work). So on the closing
 * thread, on a thread that had computed before the close and adopts it, on
 * one started after it, on a context made after it, and on an explicit
 * context closed by swe_close_r().
 *
 * The check, each scenario in a forked, pristine process: the setting, a
 * Swiss (or JPL) Moon by swe_calc_ut(), swe_close(), then the observation,
 * twice: on the main thread, a thread that had computed before the close,
 * a thread started then, a context made then by swe_ctx_new(), and an
 * explicit context given the same calls with swe_close_r(). Each must
 * answer bit for bit -- the values, the return flag, serr -- as in a fresh
 * process given the same calls without the close. The settings: the path
 * by swe_set_ephe_path(); the path and swe_set_jpl_file("de200.eph"),
 * observed in AD 1700 with SEFLG_JPLEPH; the files named by SE_EPHE_PATH
 * alone; and no path at all, the files nowhere along the compiled default,
 * which the close must leave as a fresh process has it. The observations:
 * swe_deltat_ex() and swe_deltat() in AD 1000, swe_get_tid_acc(), the Moon
 * and its speed by swe_calc_ut(), a UT search, swe_mooncross_ut() from
 * there, and a TT Sun at J2000 by swe_calc() followed by swe_get_tid_acc(),
 * swe_deltat() in AD 1800 and swe_get_tid_acc() again. 4 settings x 6
 * observations x 5 readers = 120 checks. And, for the
 * path and for de200.eph, the thread that adopted the close and read the
 * files again must not be taken back to the close by another thread's
 * unrelated setter (swe_set_topo()): its next Moon opens no file (fopen()
 * interposed) and answers as the fresh process's: 2 checks.
 *
 * Measured before the fix, 59 of 122 fail: with the path all five
 * readers in delta t (both), swe_get_tid_acc(), the Moon and the TT Sun's
 * getter, 25, and the running thread's Moon of the second part; with
 * de200.eph the same but the running thread's swe_deltat_ex() and JPL
 * Moon, 23 (the TT Sun's swe_deltat() 4.17 s off); with SE_EPHE_PATH the
 * main thread and the explicit context, 10 (its threads and the context
 * made then share no path and name their own). The search converges past
 * its first delta t and passes everywhere, and so does no path at all
 * (the default term either way). The compat build fails 44, upstream's
 * close (the JPL Sun's getter and delta t agree there, compat's guess
 * following the open JPL file; the running thread's swe_get_tid_acc() and
 * Swiss Moon pass by what compat's adoption copies). The fix without its
 * keeping a context's own reading out of the shared configuration's
 * comparison passes the 120 and fails the 2: the running
 * thread reopens 3 files at the next sync (2 with de200.eph). The
 * upstream build (legacy-master) has no contexts and does not build
 * this; a main-thread program with the path, a Moon and swe_close()
 * measures there the same 1463.43 s, -25.80 and 197.3747175 then
 * 197.3763252 deg, and with de200.eph 14.29 s and 40.0057511 then
 * 40.0039618 deg. Run as
 *   closetid EPHEDIR
 */
#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T_1000 2086308.0	/* AD 1000 */
#define T_1700 2341972.5	/* AD 1700, inside de200.eph */
#define T_1800 2378496.5	/* AD 1800 */
#define T_WARM 2451600.0
#define NV     8

static const char *EPHE;
static __thread int nfopen;	/* this thread's fopen() calls */

FILE *fopen(const char *p, const char *m)
{
  static FILE *(*real)(const char *, const char *);
  if (real == NULL)
    *(void **) &real = dlsym(RTLD_NEXT, "fopen");
  nfopen++;
  return real(p, m);
}

enum { K_PATH, K_JPL, K_ENV, K_NONE, NK };
static const char *KN[NK] = {
  "swe_set_ephe_path()", "the path and de200.eph", "SE_EPHE_PATH alone",
  "no path at all" };
enum { O_DT, O_DTG, O_TID, O_CALC, O_CROSS, O_TTFIRST, NO };
static const char *ON[NO] = {
  "swe_deltat_ex()", "swe_deltat()", "swe_get_tid_acc()", "swe_calc_ut() Moon",
  "swe_mooncross_ut()", "a TT Sun, then the tidal term and swe_deltat()" };
enum { R_MAIN, R_RUN, R_NEW, R_MADE, R_CTX, NR };
static const char *RN[NR] = { "main thread", "running thread", "new thread",
                              "context made then", "explicit context" };

struct obs { int ret, n; double v[NV]; char err[AS_MAXCH]; };

static int32 flag_of(int k)
{
  return k == K_JPL ? SEFLG_JPLEPH : SEFLG_SWIEPH;
}

static double t_of(int k)
{
  return k == K_JPL ? T_1700 : T_1000;
}

static void put(struct obs *o, double v)
{
  if (o->n < NV)
    o->v[o->n++] = v;
}

/* c NULL: the default context through the process-wide API */
static void setup(swe_ctx *c, int k)
{
  if (k == K_PATH || k == K_JPL) {
    if (c) swe_set_ephe_path_r(c, EPHE); else swe_set_ephe_path((char *) EPHE);
  }
  if (k == K_JPL) {
    if (c) swe_set_jpl_file_r(c, "de200.eph"); else swe_set_jpl_file("de200.eph");
  }
}

static int32 moon(swe_ctx *c, int k, double t, double *x, char *serr)
{
  int32 fl = flag_of(k) | SEFLG_SPEED;
  memset(x, 0, 6 * sizeof(double));
  return c ? swe_calc_ut_r(c, t, SE_MOON, fl, x, serr)
           : swe_calc_ut(t, SE_MOON, fl, x, serr);
}

static void warm(swe_ctx *c, int k)
{
  double x[6];
  char serr[AS_MAXCH];
  moon(c, k, T_WARM, x, serr);
}

/* the observation twice: the first call after the close, and the second */
static void observe(swe_ctx *c, int k, int ob, struct obs *o)
{
  double x[6], t = t_of(k), v;
  char serr[AS_MAXCH];
  int32 fl = flag_of(k), rf = 0;
  int pass;
  memset(o, 0, sizeof *o);
  for (pass = 0; pass < 2; pass++) {
    *serr = '\0';
    switch (ob) {
    case O_DT:
      v = c ? swe_deltat_ex_r(c, t, fl, serr) : swe_deltat_ex(t, fl, serr);
      put(o, v * 86400.0);
      break;
    case O_DTG:
      v = c ? swe_deltat_r(c, t) : swe_deltat(t);
      put(o, v * 86400.0);
      break;
    case O_TID:
      put(o, c ? swe_get_tid_acc_r(c) : swe_get_tid_acc());
      break;
    case O_CALC:
      rf = moon(c, k, t, x, serr);
      put(o, x[0]); put(o, x[3]);
      break;
    case O_CROSS:
      v = c ? swe_mooncross_ut_r(c, 0.0, t, fl, serr)
            : swe_mooncross_ut(0.0, t, fl, serr);
      put(o, v);
      break;
    case O_TTFIRST:	/* the getter must give the term the delta t uses */
      memset(x, 0, sizeof x);
      rf = c ? swe_calc_r(c, 2451545.0, SE_SUN, fl, x, serr)
             : swe_calc(2451545.0, SE_SUN, fl, x, serr);
      put(o, c ? swe_get_tid_acc_r(c) : swe_get_tid_acc());
      v = c ? swe_deltat_r(c, T_1800) : swe_deltat(T_1800);
      put(o, v * 86400.0);
      put(o, c ? swe_get_tid_acc_r(c) : swe_get_tid_acc());
      break;
    }
    if (pass == 0) {
      o->ret = rf;
      snprintf(o->err, sizeof o->err, "%s", serr);
    }
  }
}

struct ans { struct obs r[NR]; int ran; };
struct job { int k, ob; struct obs *o; pthread_barrier_t *b; };

static void *thr_new(void *arg)
{
  struct job *j = arg;
  observe(NULL, j->k, j->ob, j->o);
  return NULL;
}

static void *thr_run(void *arg)	/* computes, waits out the close */
{
  struct job *j = arg;
  warm(NULL, j->k);
  pthread_barrier_wait(j->b);
  pthread_barrier_wait(j->b);
  observe(NULL, j->k, j->ob, j->o);
  return NULL;
}

/* closes: with swe_close() (and swe_close_r()), or the reference */
static void scenario(int k, int ob, int closes, struct ans *a)
{
  pthread_t th;
  pthread_barrier_t b;
  struct job j;
  swe_ctx *c;
  if (k == K_ENV)
    setenv("SE_EPHE_PATH", EPHE, 1);
  pthread_barrier_init(&b, NULL, 2);
  setup(NULL, k);
  warm(NULL, k);
  j.k = k; j.ob = ob; j.o = &a->r[R_RUN]; j.b = &b;
  if (pthread_create(&th, NULL, thr_run, &j) != 0)
    return;
  pthread_barrier_wait(&b);	/* it has synced */
  if (closes)
    swe_close();
  pthread_barrier_wait(&b);
  pthread_join(th, NULL);
  observe(NULL, k, ob, &a->r[R_MAIN]);
  j.o = &a->r[R_NEW]; j.b = NULL;
  if (pthread_create(&th, NULL, thr_new, &j) != 0)
    return;
  pthread_join(th, NULL);
  /* a context made now, inheriting the configuration */
  if ((c = swe_ctx_new()) == NULL)
    return;
  observe(c, k, ob, &a->r[R_MADE]);
  swe_ctx_free(c);
  /* an explicit context, the same calls */
  if ((c = swe_ctx_new()) == NULL)
    return;
  setup(c, k);
  warm(c, k);
  if (closes)
    swe_close_r(c);
  observe(c, k, ob, &a->r[R_CTX]);
  swe_ctx_free(c);
  a->ran = 1;
}

/* the second part: the running thread's Moon after it read the files
 * again, then after another thread's swe_set_topo(), and its fopen()
 * calls for the second */
struct quiet { double v[4]; int ret, nopen, ran; };
struct qjob { int k; struct quiet *q; pthread_barrier_t *b; };

static void *thr_quiet(void *arg)
{
  struct qjob *j = arg;
  double x[6];
  char serr[AS_MAXCH];
  warm(NULL, j->k);
  pthread_barrier_wait(j->b);
  pthread_barrier_wait(j->b);	/* closed */
  moon(NULL, j->k, t_of(j->k), x, serr);
  j->q->v[0] = x[0]; j->q->v[1] = x[3];
  pthread_barrier_wait(j->b);
  pthread_barrier_wait(j->b);	/* another group published */
  nfopen = 0;
  j->q->ret = moon(NULL, j->k, t_of(j->k), x, serr);
  j->q->nopen = nfopen;
  j->q->v[2] = x[0]; j->q->v[3] = x[3];
  return NULL;
}

static void quiet_scenario(int k, int closes, struct quiet *q)
{
  pthread_t th;
  pthread_barrier_t b;
  struct qjob j;
  pthread_barrier_init(&b, NULL, 2);
  setup(NULL, k);
  warm(NULL, k);
  j.k = k; j.q = q; j.b = &b;
  if (pthread_create(&th, NULL, thr_quiet, &j) != 0)
    return;
  pthread_barrier_wait(&b);
  if (closes)
    swe_close();
  pthread_barrier_wait(&b);
  pthread_barrier_wait(&b);	/* it has read the files again */
  swe_set_topo(8.55, 47.37, 400);
  pthread_barrier_wait(&b);
  pthread_join(th, NULL);
  q->ran = 1;
}

/* a scenario in a forked child: part 0 the first, 1 the second */
static int in_child(int part, int k, int ob, int closes, void *out, size_t size)
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
    struct quiet q;
    void *p = part ? (void *) &q : (void *) &a;
    close(fd[0]);
    memset(&a, 0, sizeof a);
    memset(&q, 0, sizeof q);
    if (part)
      quiet_scenario(k, closes, &q);
    else
      scenario(k, ob, closes, &a);
    _exit(write(fd[1], p, size) == (ssize_t) size ? 0 : 3);
  }
  close(fd[1]);
  memset(out, 0, size);
  while (got < (ssize_t) size
         && (r = read(fd[0], (char *) out + got, size - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  if (got != (ssize_t) size)
    return -1;
  return (part ? ((struct quiet *) out)->ran : ((struct ans *) out)->ran) ? 0 : -1;
}

/* bit for bit: 0, or the largest difference; INFINITY for another return
 * or message, a NaN, or -0 for 0 */
static double off(const double *p, const double *q, int n)
{
  double m = 0, d;
  int i;
  for (i = 0; i < n; i++) {
    if (memcmp(&p[i], &q[i], sizeof(double)) == 0)
      continue;
    d = fabs(p[i] - q[i]);
    if (!(d > 0))
      d = INFINITY;
    if (d > m)
      m = d;
  }
  return m;
}

static double obs_off(const struct obs *p, const struct obs *q)
{
  if (p->ret != q->ret || p->n != q->n || strcmp(p->err, q->err) != 0)
    return INFINITY;
  return off(p->v, q->v, p->n);
}

static void show(const char *who, const struct obs *o)
{
  int i;
  printf("        %-13s ret %d", who, o->ret);
  for (i = 0; i < o->n; i++)
    printf(" %.10f", o->v[i]);
  if (o->err[0])
    printf(" (%.70s)", o->err);
  printf("\n");
}

int main(int argc, char **argv)
{
  int k, ob, r, n = 0, nfail = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  unsetenv("SE_EPHE_PATH");
  unsetenv("SE_EPHE_FALLBACK");

  for (k = 0; k < NK; k++) {
    for (ob = 0; ob < NO; ob++) {
      struct ans a, ref;
      if (in_child(0, k, ob, 1, &a, sizeof a) != 0
          || in_child(0, k, ob, 0, &ref, sizeof ref) != 0) {
        printf("    %s, %s: did not run\n", KN[k], ON[ob]);
        n += NR;
        nfail += NR;
        continue;
      }
      for (r = 0; r < NR; r++) {
        double d = obs_off(&a.r[r], &ref.r[r]);
        n++;
        if (d != 0) {
          printf("    %s, %s: %s FAIL (%.3g)\n", KN[k], ON[ob], RN[r], d);
          show("closed", &a.r[r]);
          show("never closed", &ref.r[r]);
          nfail++;
        }
      }
    }
  }
  for (k = K_PATH; k <= K_JPL; k++) {
    struct quiet q, qref;
    double d;
    n++;
    if (in_child(1, k, 0, 1, &q, sizeof q) != 0
        || in_child(1, k, 0, 0, &qref, sizeof qref) != 0) {
      printf("    %s, the next sync: did not run\n", KN[k]);
      nfail++;
      continue;
    }
    d = q.ret != qref.ret ? INFINITY : off(q.v, qref.v, 4);
    if (d != 0 || q.nopen != 0) {
      printf("    %s, the running thread at the sync after its reading: FAIL"
             " (%.3g, %d fopen() for %d)\n", KN[k], d, q.nopen, qref.nopen);
      nfail++;
    }
  }
  if (nfail)
    printf("G127 FAIL: %d of %d\n", nfail, n);
  else
    printf("G127 PASS (%d)\n", n);
  return nfail != 0;
}
