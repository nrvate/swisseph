/* closekeep.c -- G120: swe_close() and the shared configuration. The
 * master's generation only grows, and a setting swe_close() keeps stays
 * the master's.
 *
 * The defects (UPSTREAM-BUGS 123, this fork's), both in swi_config_reset(),
 * which swe_close() calls after it has released the default context:
 *  1. It zeroed the master's generation, so a thread that had synced at
 *     generation N missed the Nth publish after the close:
 *     swe_set_delta_t_userdef(0.001), a second thread computes,
 *     swe_close(), swe_set_delta_t_userdef(0.002), and the second thread
 *     still answered 0.001. And as the counter read 0, "nothing
 *     published", the reset itself reached nobody.
 *  2. It emptied the whole master, which the next publish filled from the
 *     defaults, so a setting swe_close() keeps in the context -- the path,
 *     the JPL file, delta t, the nutation interpolation, the lapse rate
 *     -- went back to its default at the context's next sync after a
 *     setter of any other group: swe_set_delta_t_userdef(0.001) and
 *     swe_close() gave delta t at J2000 0.001 days, and after a
 *     swe_set_topo() 0.000739, the computed value. A thread started after
 *     the close never saw the kept settings at all; the fallback setting,
 *     which bug 122 had the closing context keep owning, stayed in the
 *     context but not in the master.
 *
 * The check, each scenario in a forked, pristine process:
 *   A. Each kept setting -- the path, the JPL file (de200.eph), delta t
 *      0.001, the nutation interpolation on, the lapse rate 0.05, the
 *      fallback 1 -- set after the path, then a Swiss Sun, swe_close(),
 *      and another group's setter or none: the path again, swe_set_topo(),
 *      swe_set_sid_mode(), swe_set_tid_acc(). The main thread and a thread
 *      started then observe the setting -- the Mars it reads and its file,
 *      the JPL Mars, delta t, the nutation at J2000 + 0.37 interpolated,
 *      the Sun refracted below the dip from 2000 m, the setting read back
 *      -- and both must answer as a fresh process given the same calls
 *      without swe_close() does, within 1e-9. 28 scenarios.
 *   B. Each group set to one value, a second thread computes (at another
 *      instant), then swe_close() and either nothing or the path and the
 *      group's other value (as many publishes as before the close); the
 *      second thread observes the group and must answer as the main
 *      thread. The eight groups: the path (../ephe, then ../ephe/.), the
 *      observer, the sidereal mode, the tidal acceleration, delta t, the
 *      nutation interpolation, the lapse rate, the fallback. 16 scenarios.
 *
 * Measured before the fix, 38 of 44 fail. In A all 28: the main thread
 * after another setter for every kept setting but the fallback (delta t
 * 0.000739 for 0.001; the path back to the default, the Mars refused; the
 * JPL Mars refused; the nutation at J2000 + 0.37 exact, -13.927054", for
 * the interpolated -13.926980"; the Sun at -14 min refracted to -1.066 deg
 * where 0.05 K/m leaves it below the dip at -1.965), and the new thread in
 * every scenario, which adopted nothing. In B 7 with the other value (the
 * second thread kept the first: delta t 0.001 for 0.002) and 3 with
 * nothing after the close (the closed session's observer, sidereal mode and
 * tidal acceleration on the second thread, the defaults on the main one).
 * The compat build fails 39, the interpolation set off again too: in the
 * default build that setter's own first sync had already put the main
 * thread's setting back to off (defect 2), so it published nothing and no
 * generation collided. The upstream build has no shared configuration and
 * no swe_set_ephe_fallback(), and does not build this. Run as
 *   closekeep EPHEDIR
 */
#include <limits.h>
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
#define TOL    1e-9
#define NV     8

static const double GEO_A[3] = { 8.55, 47.37, 400 };
static const double GEO_B[3] = { 151.21, -33.87, 50 };
static const char *EPHE;
static char ALT[PATH_MAX];	/* EPHE "/.": the same files, another path */

struct obs {
  int ret, n;
  double v[NV];
  char file[300];
  char err[AS_MAXCH];
};

/* the settings: the first six are what swe_close() keeps */
enum { K_PATH, K_JPL, K_DT, K_INTP, K_LAPSE, K_FB, K_TOPO, K_SID, K_TID, NK };
#define NKEPT 6
static const char *KN[NK] = {
  "the path", "the JPL file", "delta t", "the nutation interpolation",
  "the lapse rate", "the fallback", "the observer", "the sidereal mode",
  "the tidal acceleration" };

/* the setting's first (v 0) or second (v 1) value */
static void set(int k, int v)
{
  switch (k) {
    case K_PATH:  swe_set_ephe_path(v ? ALT : (char *) EPHE); break;
    case K_JPL:   swe_set_jpl_file("de200.eph"); break;
    case K_DT:    swe_set_delta_t_userdef(v ? 0.002 : 0.001); break;
    case K_INTP:  swe_set_interpolate_nut(v ? FALSE : TRUE); break;
    case K_LAPSE: swe_set_lapse_rate(v ? 0.0 : 0.05); break;
    case K_FB:    swe_set_ephe_fallback(v ? 0 : 1); break;
    case K_TOPO:
      if (v) swe_set_topo(GEO_B[0], GEO_B[1], GEO_B[2]);
      else   swe_set_topo(GEO_A[0], GEO_A[1], GEO_A[2]);
      break;
    case K_SID:   swe_set_sid_mode(v ? SE_SIDM_RAMAN : SE_SIDM_LAHIRI, 0, 0); break;
    case K_TID:   swe_set_tid_acc(v ? -25.0 : SE_TIDAL_DE200); break;
  }
}

static void put(struct obs *o, double v)
{
  if (o->n < NV)
    o->v[o->n++] = v;
}

static void observe(int k, struct obs *o)
{
  double x[6], tret[10], ts, te;
  char serr[AS_MAXCH] = "";
  int32 rf;
  int d;
  const char *f;
  memset(o, 0, sizeof *o);
  /* the Moon first: it syncs, and opens the moon file, whose DE number
   * the automatic tidal term then takes in every process alike */
  rf = swe_calc(T_OBS, SE_MOON, SEFLG_SWIEPH, x, serr);
  switch (k) {
    case K_PATH:
      rf = swe_calc(T_OBS, SE_MARS, SEFLG_SWIEPH, x, serr);
      put(o, x[0]);
      f = swe_get_current_file_data(0, &ts, &te, &d);
      snprintf(o->file, sizeof o->file, "%s", f ? f : "(none)");
      break;
    case K_JPL:
      rf = swe_calc(T_OBS, SE_MARS, SEFLG_JPLEPH, x, serr);
      put(o, x[0]); put(o, x[1]);
      break;
    case K_DT:
      put(o, swe_deltat(T_OBS));
      put(o, swe_deltat_ex(T_OBS, SEFLG_SWIEPH, serr));
      break;
    case K_INTP:	/* the Moon set the nodes at T_OBS; this interpolates */
      rf = swe_calc(T_OBS + 0.37, SE_ECL_NUT, 0, x, serr);
      put(o, x[2] * 3600); put(o, x[3] * 3600);
      break;
    case K_LAPSE: {	/* G82's: the Sun from 2000 m 14 and 15.75 min before
			 * its true rise; the dip is -1.235 deg at the default
			 * lapse rate, -0.977 at 0.05 K/m, -1.269 at 0 */
      double geo[3] = { GEO_A[0], GEO_A[1], 2000 }, xaz[3];
      int i;
      rf = swe_rise_trans(T_OBS, SE_SUN, NULL, SEFLG_SWIEPH,
                          SE_CALC_RISE | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION,
                          geo, 0, 10, tret, serr);
      if (rf != ERR) {
        for (i = 0; i < 2; i++) {
          double t = tret[0] - (i ? 15.75 : 14.0) / 1440;
          swe_calc_ut(t, SE_SUN, SEFLG_SWIEPH, x, serr);
          swe_azalt(t, SE_ECL2HOR, geo, 1013.25, 10, x, xaz);
          put(o, xaz[2]);
        }
      }
      break;
    }
    case K_FB:
      rf = 0;
      put(o, swe_get_ephe_fallback());
      break;
    case K_TOPO:
      rf = swe_calc_ut(T_OBS, SE_MOON, SEFLG_SWIEPH | SEFLG_TOPOCTR, x, serr);
      put(o, x[0]); put(o, x[1]);
      break;
    case K_SID:
      put(o, swe_get_ayanamsa_ut(T_OBS));
      break;
    case K_TID:
      put(o, swe_get_tid_acc());
      put(o, swe_deltat(2086308.0) * 86400);	/* AD 1000, seconds */
      break;
  }
  o->ret = rf;
  if (rf < 0)
    snprintf(o->err, sizeof o->err, "%s", serr);
}

static void warm(void)
{
  double x[6];
  char serr[AS_MAXCH];
  swe_calc(T_WARM, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
}

/* --- A: a kept setting, swe_close(), another group's setter ---------- */
enum { Y_NONE, Y_PATH, Y_TOPO, Y_SID, Y_TID, NY };
static const char *YN[NY] = { "nothing", "the path", "swe_set_topo()",
                              "swe_set_sid_mode()", "swe_set_tid_acc()" };

static void other(int y)
{
  switch (y) {
    case Y_PATH: swe_set_ephe_path((char *) EPHE); break;
    case Y_TOPO: swe_set_topo(GEO_B[0], GEO_B[1], GEO_B[2]); break;
    case Y_SID:  swe_set_sid_mode(SE_SIDM_RAMAN, 0, 0); break;
    case Y_TID:  swe_set_tid_acc(SE_TIDAL_DE200); break;
  }
}

struct ans { struct obs main_, thr; int ran; };
struct job { int k; struct ans *a; pthread_barrier_t *b; int two; };

static void *thr_observe(void *arg)
{
  struct job *j = arg;
  if (j->two) {		/* B: compute, let the main thread close, again */
    /* at another instant: the nutation cached at T_OBS + 0.37 would
     * outlive a change of the interpolation, which is not this check */
    warm();
    pthread_barrier_wait(j->b);
    pthread_barrier_wait(j->b);
  }
  observe(j->k, &j->a->thr);
  return NULL;
}

static void scenario_a(int k, int y, int closes, struct ans *a)
{
  pthread_t th;
  struct job j;
  swe_set_ephe_path((char *) EPHE);
  if (k != K_PATH)
    set(k, 0);
  warm();
  if (closes)
    swe_close();
  other(y);
  observe(k, &a->main_);
  if (closes) {
    j.k = k; j.a = a; j.b = NULL; j.two = 0;
    if (pthread_create(&th, NULL, thr_observe, &j) != 0)
      return;
    pthread_join(th, NULL);
  }
  a->ran = 1;
}

/* --- B: a second thread across swe_close() -------------------------- */
static void scenario_b(int k, int again, struct ans *a)
{
  pthread_t th;
  pthread_barrier_t b;
  struct job j;
  pthread_barrier_init(&b, NULL, 2);
  swe_set_ephe_path((char *) EPHE);
  if (k != K_PATH)
    set(k, 0);
  j.k = k; j.a = a; j.b = &b; j.two = 1;
  if (pthread_create(&th, NULL, thr_observe, &j) != 0)
    return;
  pthread_barrier_wait(&b);	/* the thread has synced */
  swe_close();
  if (again) {			/* as many publishes as before the close */
    if (k == K_PATH) {
      set(K_PATH, 1);
    } else {
      swe_set_ephe_path((char *) EPHE);
      set(k, 1);
    }
  }
  pthread_barrier_wait(&b);
  pthread_join(th, NULL);
  observe(k, &a->main_);
  a->ran = 1;
}

/* part 'a' (y; closes or not) or 'b' (again or not), in a forked child */
static int in_child(int part, int k, int y, int flag, struct ans *out)
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
    if (part == 'a')
      scenario_a(k, y, flag, &a);
    else
      scenario_b(k, flag, &a);
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

static int differ(const struct obs *p, const struct obs *q)
{
  int i;
  if (p->ret != q->ret || p->n != q->n || strcmp(p->file, q->file) != 0)
    return 1;
  for (i = 0; i < p->n; i++)
    if (!(fabs(p->v[i] - q->v[i]) <= TOL))	/* a NaN differs */
      return 1;
  return 0;
}

static void show(const char *who, const struct obs *o)
{
  int i;
  printf("        %-9s ret %d", who, o->ret);
  for (i = 0; i < o->n; i++)
    printf(" %.10g", o->v[i]);
  if (o->file[0])
    printf(" %s", o->file);
  if (o->ret < 0 && o->err[0])
    printf(" (%.60s)", o->err);
  printf("\n");
}

int main(int argc, char **argv)
{
  int k, y, again, n = 0, nfail = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  snprintf(ALT, sizeof ALT, "%s/.", EPHE);
  unsetenv("SE_EPHE_PATH");
  unsetenv("SE_EPHE_FALLBACK");

  printf("  A. a kept setting, swe_close(), another group's setter: the main\n"
         "     thread and a new thread against a process without the close\n");
  for (k = 0; k < NKEPT; k++) {
    for (y = 0; y < NY; y++) {
      struct ans s, ref;
      if (y == Y_PATH && (k == K_PATH || k == K_JPL))
        continue;	/* the same group */
      n++;
      if (in_child('a', k, y, 1, &s) != 0 || in_child('a', k, y, 0, &ref) != 0) {
        printf("    %s, then %s: did not run\n", KN[k], YN[y]);
        nfail++;
        continue;
      }
      if (differ(&s.main_, &ref.main_) || differ(&s.thr, &ref.main_)) {
        printf("    %s, swe_close(), then %s: FAIL\n", KN[k], YN[y]);
        show("closed", &s.main_);
        show("thread", &s.thr);
        show("no close", &ref.main_);
        nfail++;
      }
    }
  }

  printf("  B. a second thread across swe_close(), against the main thread\n");
  for (k = 0; k < NK; k++) {
    if (k == K_JPL)
      continue;		/* one value; the path's group is the path's */
    for (again = 0; again <= 1; again++) {
      struct ans s;
      n++;
      if (in_child('b', k, 0, again, &s) != 0) {
        printf("    %s, swe_close()%s: did not run\n", KN[k],
               again ? ", the other value" : "");
        nfail++;
        continue;
      }
      if (differ(&s.thr, &s.main_)) {
        printf("    %s, swe_close()%s: FAIL\n", KN[k],
               again ? ", the path and the other value" : "");
        show("thread", &s.thr);
        show("main", &s.main_);
        nfail++;
      }
    }
  }

  if (nfail)
    printf("G120 FAIL: %d of %d\n", nfail, n);
  else
    printf("G120 PASS (%d)\n", n);
  return nfail != 0;
}
