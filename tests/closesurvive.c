/* closesurvive.c -- G123: what swe_set_topo() and swe_set_sid_mode() set
 * survives swe_close(), as the manual says, on every thread and context.
 *
 * The defect (UPSTREAM-BUGS 127): the manual has "The parameters set by
 * swe_set_topo() survive swe_close()" and "the parameters set by
 * swe_set_sid_mode() survive calls of the function swe_close()", and
 * swe_close() -- ctx_release(), and swi_config_reset() in the shared
 * configuration after it (bug 123) -- reset both, as upstream's does.
 * After swe_set_topo(8.55, 47.37, 400) and swe_close() the topocentric
 * Moon at J2000 was refused, "geographic position has not been set", for
 * 222.988 deg; after swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0) the ayanamsa
 * at J2000 was Fagan/Bradley's 24.736 deg for 23.853, the sidereal Sun
 * 0.883 deg off; a user t0 and ayan_t0, SE_SIDM_J2000 with its
 * SE_SIDBIT_ECL_T0 (the sidereal Moon in AD 1000 at 186.365 deg for
 * 211.107) and Lahiri with SE_SIDBIT_PREC_ORIG went the same way, on the
 * closing thread, on a thread started after the close, on one that had
 * computed before it, and on an explicit context closed by swe_close_r().
 *
 * The check, each scenario in a forked, pristine process: the path, the
 * setting, a Swiss Sun, swe_close(), then nothing, the path again, or
 * another group's setter (the sidereal mode after the observer, the
 * observer after a sidereal mode); the main thread, a thread started then,
 * a thread that had computed before the close, and an explicit context
 * given the same calls with swe_close_r() must answer bit for bit as a
 * fresh process given the same calls without the close. The settings:
 *   - the observer: the topocentric Moon at J2000 by swe_calc_ut(), with
 *     its speed;
 *   - Lahiri; a user t0 and ayan_t0; Lahiri with SE_SIDBIT_PREC_ORIG,
 *     whose models the mode keeps; SE_SIDM_J2000: swe_get_ayanamsa_ex()
 *     at J2000 and in AD 1000, the sidereal Sun at J2000 and the sidereal
 *     Moon in AD 1000 by swe_calc(), the nutation at J2000 + 0.25;
 *   - and two the close still resets, the models of
 *     swe_set_astro_models() (nutation IAU 1980) against a process that
 *     never set them, and Lahiri with the bit and then the models
 *     "0,0,0,4" against Lahiri with the bit alone, the same observations.
 * The sidereal ones are in TT: after swe_close() without a path the tidal
 * term is the default's until a file opens (the path's DE numbers go with
 * the files, bug F2's choice), which moves a UT ayanamsa in AD 1000 by
 * 5e-9 deg against the process that never closed. 7 settings x 3 x 4
 * readers = 84 checks.
 *
 * Measured before the fix, 72 of 84 fail: the five settings the manual
 * keeps and PREC_ORIG with the models after it, in every case and reader
 * -- the Moon refused; the ayanamsa at J2000 Fagan/Bradley's 24.736 deg
 * for Lahiri's 23.853, the user mode's 23.853 and SE_SIDM_J2000's -0.004
 * -- and the models alone pass. The compat build fails the same 72. With
 * ctx_release() keeping the two and the shared configuration not, 54 fail:
 * the explicit context passes, and every reader on the default context
 * takes the master's reset at its next sync, the closing thread too, as
 * it gave up the groups; with the mode kept but not the models
 * SE_SIDBIT_PREC_ORIG set, 24 (the ayanamsa in AD 1000 5.6e-4 deg off).
 * The upstream build (legacy-master) has no contexts and no shared
 * configuration and does not build this; its swe_close() resets both on
 * the closing thread, as the compat build's does (measured with a
 * main-thread program: the Moon refused, the ayanamsa 24.736). Run as
 *   closesurvive EPHEDIR
 */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T_OBS  2451545.0	/* J2000 */
#define T_WARM 2451600.0
#define T_1000 2086308.0	/* AD 1000 */
#define NV     8

static const double GEO_A[3] = { 8.55, 47.37, 400 };
static const double GEO_B[3] = { 151.21, -33.87, 50 };
static const char *EPHE;
static char MODELS[] = "0,0,0,1";	/* nutation IAU 1980 */
static char MODELS_B[] = "0,0,0,4";	/* precession default, IAU 2000B */

struct obs { int ret, n; double v[NV]; char err[AS_MAXCH]; };

enum { S_TOPO, S_LAHIRI, S_USER, S_ORIG, S_J2000, S_MODELS, S_ORIG_MODELS, NS };
static const char *SN[NS] = {
  "the observer", "Lahiri", "a user t0 and ayan_t0", "Lahiri, PREC_ORIG",
  "SE_SIDM_J2000 (ECL_T0)", "the models (reset)", "PREC_ORIG, then models" };
enum { Y_NONE, Y_PATH, Y_OTHER, NY };
static const char *YN[NY] = { "nothing", "the path", "another setter" };
enum { R_MAIN, R_NEW, R_RUN, R_CTX, NR };
static const char *RN[NR] = { "main thread", "new thread", "running thread",
                              "explicit context" };

/* c NULL: the default context through the process-wide API */
static void sid(swe_ctx *c, int32 m, double t0, double a0)
{
  if (c) swe_set_sid_mode_r(c, m, t0, a0); else swe_set_sid_mode(m, t0, a0);
}

static void topo(swe_ctx *c, const double *g)
{
  if (c) swe_set_topo_r(c, g[0], g[1], g[2]); else swe_set_topo(g[0], g[1], g[2]);
}

static void models(swe_ctx *c, char *m)
{
  if (c) swe_set_astro_models_r(c, m, 0); else swe_set_astro_models(m, 0);
}

/* the setting; ref: as the process without the close has it, where the
 * close resets it */
static void set(swe_ctx *c, int s, int ref)
{
  switch (s) {
  case S_TOPO:   topo(c, GEO_A); break;
  case S_LAHIRI: sid(c, SE_SIDM_LAHIRI, 0, 0); break;
  case S_USER:   sid(c, SE_SIDM_USER, 2415020.0, 22.46); break;
  case S_ORIG:   sid(c, SE_SIDM_LAHIRI | SE_SIDBIT_PREC_ORIG, 0, 0); break;
  case S_J2000:  sid(c, SE_SIDM_J2000, 0, 0); break;
  case S_MODELS:
    if (!ref)
      models(c, MODELS);
    break;
  case S_ORIG_MODELS:
    sid(c, SE_SIDM_LAHIRI | SE_SIDBIT_PREC_ORIG, 0, 0);
    if (!ref)
      models(c, MODELS_B);
    break;
  }
}

static void after(swe_ctx *c, int s, int y)
{
  if (y == Y_PATH) {
    if (c) swe_set_ephe_path_r(c, EPHE); else swe_set_ephe_path((char *) EPHE);
  } else if (y == Y_OTHER) {
    if (s == S_TOPO)
      sid(c, SE_SIDM_RAMAN, 0, 0);
    else
      topo(c, GEO_B);
  }
}

static void put(struct obs *o, double v)
{
  if (o->n < NV)
    o->v[o->n++] = v;
}

static void observe(swe_ctx *c, int s, struct obs *o)
{
  double x[6], aya = 0;
  char serr[AS_MAXCH] = "";
  int32 rf, fl = SEFLG_SWIEPH | SEFLG_SIDEREAL;
  memset(o, 0, sizeof *o);
  if (s == S_TOPO) {
    fl = SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED;
    rf = c ? swe_calc_ut_r(c, T_OBS, SE_MOON, fl, x, serr)
           : swe_calc_ut(T_OBS, SE_MOON, fl, x, serr);
    put(o, x[0]); put(o, x[1]); put(o, x[2]); put(o, x[3]);
  } else {
    rf = c ? swe_get_ayanamsa_ex_r(c, T_OBS, SEFLG_SWIEPH, &aya, serr)
           : swe_get_ayanamsa_ex(T_OBS, SEFLG_SWIEPH, &aya, serr);
    put(o, aya);
    if (rf >= 0)
      rf = c ? swe_get_ayanamsa_ex_r(c, T_1000, SEFLG_SWIEPH, &aya, serr)
             : swe_get_ayanamsa_ex(T_1000, SEFLG_SWIEPH, &aya, serr);
    put(o, aya);
    if (rf >= 0)
      rf = c ? swe_calc_r(c, T_OBS, SE_SUN, fl, x, serr)
             : swe_calc(T_OBS, SE_SUN, fl, x, serr);
    put(o, x[0]); put(o, x[1]);
    if (rf >= 0)
      rf = c ? swe_calc_r(c, T_1000, SE_MOON, fl, x, serr)
             : swe_calc(T_1000, SE_MOON, fl, x, serr);
    put(o, x[0]);
    if (rf >= 0)
      rf = c ? swe_calc_r(c, T_OBS + 0.25, SE_ECL_NUT, 0, x, serr)
             : swe_calc(T_OBS + 0.25, SE_ECL_NUT, 0, x, serr);
    put(o, x[2]); put(o, x[3]);
  }
  o->ret = rf;
  if (rf < 0)
    snprintf(o->err, sizeof o->err, "%s", serr);
}

static void warm(swe_ctx *c)
{
  double x[6];
  char serr[AS_MAXCH];
  if (c) swe_calc_r(c, T_WARM, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
  else   swe_calc(T_WARM, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
}

struct ans { struct obs r[NR]; int ran; };
struct job { int s; struct obs *o; pthread_barrier_t *b; };

static void *thr_new(void *arg)
{
  struct job *j = arg;
  observe(NULL, j->s, j->o);
  return NULL;
}

static void *thr_run(void *arg)	/* computes, waits out the close */
{
  struct job *j = arg;
  warm(NULL);
  pthread_barrier_wait(j->b);
  pthread_barrier_wait(j->b);
  observe(NULL, j->s, j->o);
  return NULL;
}

/* closes: with swe_close() (and every reader), or the reference */
static void scenario(int s, int y, int closes, struct ans *a)
{
  pthread_t th;
  pthread_barrier_t b;
  struct job j;
  swe_ctx *c;
  pthread_barrier_init(&b, NULL, 2);
  swe_set_ephe_path((char *) EPHE);
  set(NULL, s, !closes);
  warm(NULL);
  if (closes) {
    j.s = s; j.o = &a->r[R_RUN]; j.b = &b;
    if (pthread_create(&th, NULL, thr_run, &j) != 0)
      return;
    pthread_barrier_wait(&b);	/* it has synced */
    swe_close();
  }
  after(NULL, s, y);
  if (closes) {
    pthread_barrier_wait(&b);
    pthread_join(th, NULL);
  }
  observe(NULL, s, &a->r[R_MAIN]);
  if (closes) {
    j.s = s; j.o = &a->r[R_NEW]; j.b = NULL;
    if (pthread_create(&th, NULL, thr_new, &j) != 0)
      return;
    pthread_join(th, NULL);
  }
  /* an explicit context, the same calls */
  if ((c = swe_ctx_new()) == NULL)
    return;
  swe_set_ephe_path_r(c, EPHE);
  set(c, s, !closes);
  warm(c);
  if (closes)
    swe_close_r(c);
  after(c, s, y);
  observe(c, s, &a->r[R_CTX]);
  swe_ctx_free(c);
  a->ran = 1;
}

static int in_child(int s, int y, int closes, struct ans *out)
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
    scenario(s, y, closes, &a);
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

/* bit for bit: 0, or the largest difference; INFINITY for another return,
 * a NaN, or -0 for 0 */
static double off(const struct obs *p, const struct obs *q)
{
  double m = 0, d;
  int i;
  if (p->ret != q->ret || p->n != q->n)
    return INFINITY;
  for (i = 0; i < p->n; i++) {
    if (memcmp(&p->v[i], &q->v[i], sizeof(double)) == 0)
      continue;
    d = fabs(p->v[i] - q->v[i]);
    if (!(d > 0))
      d = INFINITY;
    if (d > m)
      m = d;
  }
  return m;
}

static void show(const char *who, const struct obs *o)
{
  int i;
  printf("        %-17s ret %d", who, o->ret);
  for (i = 0; i < o->n; i++)
    printf(" %.10f", o->v[i]);
  if (o->ret < 0 && o->err[0])
    printf(" (%.60s)", o->err);
  printf("\n");
}

int main(int argc, char **argv)
{
  int s, y, k, n = 0, nfail = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  unsetenv("SE_EPHE_PATH");
  unsetenv("SE_EPHE_FALLBACK");

  for (s = 0; s < NS; s++) {
    for (y = 0; y < NY; y++) {
      struct ans a, ref;
      if (in_child(s, y, 1, &a) != 0 || in_child(s, y, 0, &ref) != 0) {
        printf("    %s, swe_close(), %s: did not run\n", SN[s], YN[y]);
        n += NR;
        nfail += NR;
        continue;
      }
      for (k = 0; k < NR; k++) {
        const struct obs *f = &ref.r[k == R_CTX ? R_CTX : R_MAIN];
        double d = off(&a.r[k], f);
        n++;
        if (d != 0) {
          printf("    %s, swe_close(), %s: %s FAIL (%.3g)\n", SN[s], YN[y], RN[k], d);
          show("closed", &a.r[k]);
          show("never closed", f);
          nfail++;
        }
      }
    }
  }
  if (nfail)
    printf("G123 FAIL: %d of %d\n", nfail, n);
  else
    printf("G123 PASS (%d)\n", n);
  return nfail != 0;
}
