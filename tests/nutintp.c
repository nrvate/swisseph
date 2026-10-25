/* nutintp.c -- G122: swe_set_interpolate_nut() empties what was cached
 * under the old setting.
 *
 * The defect (UPSTREAM-BUGS 126): the setter reset the interpolation's
 * nodes and nothing cached on the instant alone -- the nutation
 * swi_check_nutation() keeps in ctx->nut (and its rate), the topocentric
 * observer, whose sidereal time carries the nutation, and the positions in
 * pldat, nddat and savedat -- and a thread on the default context adopting
 * the change through the shared configuration (swi_config_apply()) reset
 * the same nodes and no more. Interpolation on, the nutation at J2000 and
 * J2000 + 0.37, off, and J2000 + 0.37 again answered the interpolated
 * dpsi -13.926980" for the exact -13.927054" (7.42e-5"; deps 3.13e-4"),
 * and every place of date there with it; off and then on, the exact value
 * stayed where the interpolated one was due.
 *
 * The check, each run in a forked, pristine process: the path, an
 * observer and a sidereal mode (True Citra) set, the interpolation set to
 * A; the probe at J2000 and at J2000 + 0.37, and a topocentric Moon at
 * J2000 + 0.37 (the nutation and the observer cached there); the
 * interpolation set to B; then swe_sidtime() at J2000 (the interpolation's
 * nodes there) and the probe at J2000 + 0.37. The answer must equal, bit
 * for bit with its return flag, that of a fresh process that set B first
 * and made the same calls after it. Nine probes -- swe_calc()'s
 * SE_ECL_NUT, Mars and the true node with SEFLG_SPEED, the topocentric
 * Moon, swe_get_ayanamsa_ex(), Spica by swe_fixstar2(), swe_sidtime(),
 * swe_houses_ex2()'s Placidus cusps, the sidereal Mars -- both ways (on
 * then off, off then on), on the setter's own thread, on a second thread
 * on the default context that adopts the change (it makes the calls, the
 * main thread the setter), and on an explicit context through
 * swe_set_interpolate_nut_r(): 54 checks.
 *
 * Measured before the fix, 36 of 54 fail, all but swe_sidtime(), the
 * houses and the sidereal Mars (which read the nutation afresh or not at
 * all) in each mode and direction: the nutation 3.13e-4" (deps), the
 * longitudes of Mars, the true node and Spica and the ayanamsa 7.42e-5",
 * the topocentric Moon's 7.53e-5" (on then off; 7.39e-5" and 7.51e-5" off
 * then on). The observer alone, left stale, is 1.49e-5" on the Moon. The
 * upstream build (SWE_UPSTREAM_COMPAT, whose setter is upstream's) fails
 * 22, the setter's own thread and the explicit context (dpsi 8.38e-5", the
 * ayanamsa 8.38e-5"); the second thread adopts through this fork's code,
 * fixed in both builds, and passes there. Run as
 *   nutintp EPHEDIR
 */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T0 2451545.0		/* J2000 */
#define T1 (2451545.0 + 0.37)
#define NV 24

static const char *EPHE;
static const double GEO[3] = { 8.55, 47.37, 400 };

struct obs {
  int ret;
  double v[NV];
};

enum { P_NUT, P_MARS, P_TOPOMOON, P_TNODE, P_AYA, P_STAR, P_SIDT, P_HOUSE,
       P_SIDMARS, NP };
static const char *PN[NP] = { "SE_ECL_NUT", "Mars", "the topocentric Moon",
  "the true node", "the ayanamsa", "Spica", "swe_sidtime()", "the houses",
  "the sidereal Mars" };
enum { M_OWN, M_THREAD, M_CTX, NM };
static const char *MN[NM] = { "the setter's thread", "a second thread",
  "an explicit context" };

/* the probe at t, on c, or on the default context for c NULL */
static void probe(swe_ctx *c, int p, double t, struct obs *o)
{
  char serr[AS_MAXCH], s[64];
  int32 fl = SEFLG_SWIEPH | SEFLG_SPEED;
  memset(o, 0, sizeof *o);
  switch (p) {
    case P_NUT:
      o->ret = c ? swe_calc_r(c, t, SE_ECL_NUT, fl, o->v, serr)
                 : swe_calc(t, SE_ECL_NUT, fl, o->v, serr);
      break;
    case P_MARS:
      o->ret = c ? swe_calc_r(c, t, SE_MARS, fl, o->v, serr)
                 : swe_calc(t, SE_MARS, fl, o->v, serr);
      break;
    case P_TOPOMOON:
      o->ret = c ? swe_calc_r(c, t, SE_MOON, fl | SEFLG_TOPOCTR, o->v, serr)
                 : swe_calc(t, SE_MOON, fl | SEFLG_TOPOCTR, o->v, serr);
      break;
    case P_TNODE:
      o->ret = c ? swe_calc_r(c, t, SE_TRUE_NODE, fl, o->v, serr)
                 : swe_calc(t, SE_TRUE_NODE, fl, o->v, serr);
      break;
    case P_AYA:
      o->ret = c ? swe_get_ayanamsa_ex_r(c, t, SEFLG_SWIEPH, o->v, serr)
                 : swe_get_ayanamsa_ex(t, SEFLG_SWIEPH, o->v, serr);
      break;
    case P_STAR:
      strcpy(s, "Spica");
      o->ret = c ? swe_fixstar2_r(c, s, t, fl, o->v, serr)
                 : swe_fixstar2(s, t, fl, o->v, serr);
      break;
    case P_SIDT:
      o->v[0] = c ? swe_sidtime_r(c, t) : swe_sidtime(t);
      break;
    case P_HOUSE:
      o->ret = c ? swe_houses_ex2_r(c, t, 0, GEO[1], GEO[0], 'P', o->v, o->v + 13, NULL, NULL, serr)
                 : swe_houses_ex2(t, 0, GEO[1], GEO[0], 'P', o->v, o->v + 13, NULL, NULL, serr);
      break;
    case P_SIDMARS:
      o->ret = c ? swe_calc_r(c, t, SE_MARS, fl | SEFLG_SIDEREAL, o->v, serr)
                 : swe_calc(t, SE_MARS, fl | SEFLG_SIDEREAL, o->v, serr);
      break;
  }
}

static void setup(swe_ctx *c, int intp)
{
  if (c) {
    swe_set_ephe_path_r(c, EPHE);
    swe_set_topo_r(c, GEO[0], GEO[1], GEO[2]);
    swe_set_sid_mode_r(c, SE_SIDM_TRUE_CITRA, 0, 0);
    swe_set_interpolate_nut_r(c, intp);
  } else {
    swe_set_ephe_path(EPHE);
    swe_set_topo(GEO[0], GEO[1], GEO[2]);
    swe_set_sid_mode(SE_SIDM_TRUE_CITRA, 0, 0);
    swe_set_interpolate_nut(intp);
  }
}

/* before the change: the probe at T0 (the nodes there when on) and at
 * T1, and the nutation and observer cached at T1 */
static void before(swe_ctx *c, int p)
{
  struct obs o;
  probe(c, p, T0, &o);
  probe(c, p, T1, &o);
  probe(c, P_TOPOMOON, T1, &o);
}

/* after it: the nodes at T0 by sidereal time, then the probe at T1 */
static void after(swe_ctx *c, int p, struct obs *o)
{
  if (c)
    swe_sidtime_r(c, T0);
  else
    swe_sidtime(T0);
  probe(c, p, T1, o);
}

/* the second thread makes the calls; the main thread the setter between */
static pthread_barrier_t bar;
static int th_p;
static struct obs th_o;
static void *second(void *arg)
{
  (void) arg;
  pthread_barrier_wait(&bar);
  before(NULL, th_p);
  pthread_barrier_wait(&bar);
  pthread_barrier_wait(&bar);
  after(NULL, th_p, &th_o);
  return NULL;
}

/* the interpolation a, then b (fresh: b from the start) */
static void run(int m, int p, int a, int b, int fresh, struct obs *o)
{
  if (m == M_CTX) {
    swe_ctx *c = swe_ctx_new();
    if (c == NULL)
      exit(3);
    setup(c, fresh ? b : a);
    if (!fresh) {
      before(c, p);
      swe_set_interpolate_nut_r(c, b);
    }
    after(c, p, o);
    swe_ctx_free(c);
  } else if (m == M_THREAD && !fresh) {
    pthread_t th;
    setup(NULL, a);
    th_p = p;
    pthread_barrier_init(&bar, NULL, 2);
    if (pthread_create(&th, NULL, second, NULL) != 0)
      exit(3);
    pthread_barrier_wait(&bar);
    pthread_barrier_wait(&bar);
    swe_set_interpolate_nut(b);
    pthread_barrier_wait(&bar);
    pthread_join(th, NULL);
    *o = th_o;
  } else {
    setup(NULL, fresh ? b : a);
    if (!fresh) {
      before(NULL, p);
      swe_set_interpolate_nut(b);
    }
    after(NULL, p, o);
  }
}

static int in_child(int m, int p, int a, int b, int fresh, struct obs *o)
{
  int fd[2], st;
  pid_t pid;
  ssize_t got;
  fflush(stdout);
  if (pipe(fd) != 0)
    return -1;
  pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    struct obs r;
    close(fd[0]);
    run(m, p, a, b, fresh, &r);
    _exit(write(fd[1], &r, sizeof r) == (ssize_t) sizeof r ? 0 : 4);
  }
  close(fd[1]);
  got = read(fd[0], o, sizeof *o);
  close(fd[0]);
  if (waitpid(pid, &st, 0) != pid || !WIFEXITED(st) || WEXITSTATUS(st) != 0
      || got != (ssize_t) sizeof *o)
    return -1;
  return 0;
}

int main(int argc, char **argv)
{
  int m, p, dir, i, n = 0, nfail = 0;
  if (argc < 2) {
    fprintf(stderr, "usage: nutintp EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  for (m = 0; m < NM; m++)
    for (dir = 0; dir < 2; dir++)
      for (p = 0; p < NP; p++) {
        struct obs ot, of;
        int a = dir == 0, b = !a, bad;
        double d = 0;
        n++;
        if (in_child(m, p, a, b, 0, &ot) != 0 || in_child(m, p, a, b, 1, &of) != 0) {
          printf("    %s, %s, %s: child failed\n", MN[m], dir ? "off then on" : "on then off", PN[p]);
          nfail++;
          continue;
        }
        bad = ot.ret != of.ret;
        for (i = 0; i < NV; i++) {
          if (ot.v[i] != of.v[i])
            bad = 1;
          if (fabs(ot.v[i] - of.v[i]) > d)
            d = fabs(ot.v[i] - of.v[i]);
        }
        if (bad) {
          printf("    %s, %s, %s: %.3e\" off a fresh process's (return %d, %d) FAIL\n",
                 MN[m], dir ? "off then on" : "on then off", PN[p], d * 3600, ot.ret, of.ret);
          nfail++;
        }
      }
  if (nfail)
    printf("G122 FAIL: %d of %d\n", nfail, n);
  else
    printf("G122 PASS (%d)\n", n);
  return nfail ? 1 : 0;
}
