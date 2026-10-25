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
 * fixed in both builds, and passes there.
 *
 * And the models (UPSTREAM-BUGS 129): the nodes were kept, for their
 * instants alone, across a change of nutation model, and the topocentric
 * observer, kept for its instant, across any change of model, though the
 * nutation, the sidereal-time model and delta t's place it. The same
 * check, the interpolation on throughout, with the nutation model changed
 * from the default to IAU 1980 and back by swe_set_astro_models() on the
 * setter's thread, adopted by a second thread, and by
 * swe_set_astro_models_r() on an explicit context; from the default to
 * IAU 1980 by swe_set_sid_mode(SE_SIDM_LAHIRI | SE_SIDBIT_PREC_ORIG) (from
 * True Citra); from IAU 1980 to the default by swe_close(), on the
 * setter's thread and adopted by a second thread, against a fresh process
 * that set what the close keeps (the path, the interpolation, and since
 * entry 127 the observer and the sidereal mode); and, the
 * interpolation off, the sidereal-time model changed to IAU 1976 by the
 * first three routes. Nine probes each, 108 checks, 162 in all.
 *
 * Measured before the fix, 73 of the 108 fail: every probe but the
 * sidereal Mars in each nutation change (the topocentric Moon too after
 * swe_close(), which has dropped the observer, and both answer an error),
 * dpsi, Mars, the true node, the ayanamsa and Spica 8.70e-3", the
 * topocentric Moon 8.65e-3", swe_sidtime() 5.32e-4 s, the houses 1.95e-2";
 * and the topocentric Moon after the sidereal-time model, 3.12e-4". The
 * upstream build fails 74 of 162: 126's 22, and 52 of these -- every
 * nutation change but the two a second thread adopts, which this fork's
 * adoption fixes in both builds (92 before), and after swe_close() the
 * topocentric Moon, the ayanamsa and the sidereal Mars on the second
 * thread too, as upstream's close drops the observer and the sidereal
 * mode (entry 127). The sidereal-time cases pass
 * there, as upstream builds that Moon's observer afresh.
 *
 * And the JPL Horizons flags (UPSTREAM-BUGS 134): the nodes were keyed on
 * neither the flags' SEFLG_JPLHOR and SEFLG_JPLHOR_APPROX nor the
 * approximation's mode, all of which calc_nutation() reads, nor were the
 * nodes of SEFLG_JPLHOR dropped with the IERS corrections they were built
 * from; and swe_get_ayanamsa_ex() took the nutation kept at its instant
 * whatever flags had computed it. The same check, the interpolation on,
 * on de200 at 1950 (before 1962, where SEMOD_JPLHORA_3 takes IAU 1980 with
 * an offset, SEMOD_JPLHORA_2 IAU 2000 with another): SEFLG_JPLHOR_APPROX
 * before and not after, and the reverse; and with it throughout, the mode
 * from 3 to 2 and back by swe_set_astro_models() on the setter's thread,
 * adopted by a second thread, and by swe_set_astro_models_r() on an
 * explicit context, and from 2 to the default by swe_close() with the
 * files set again. Nine probes each, the flags theirs (swe_sidtime(), the
 * houses, the true node and the sidereal Mars take none). After the change
 * the probe at the instant and at + 0.37 (swe_sidtime() at the instant,
 * of no flags, would build nodes of its own). SEFLG_JPLHOR needs a DE >=
 * 403 and eop_1962_today.txt: G19's synthetic DE431 header (SE_ECL_NUT
 * reads none of its coefficients) and two files of other corrections, in
 * directories A and B made in $TMPDIR (or /tmp); SE_ECL_NUT at 2000 Jan 7
 * with SEFLG_JPLHOR and then SEFLG_JPLHOR_APPROX, the reverse, and with
 * SEFLG_JPLHOR throughout across swe_close() from A to B. 84 checks, 246
 * in all.
 *
 * Measured before the fix, 50 of the 84 fail: SE_ECL_NUT, Mars, the
 * ayanamsa and Spica 6.19e-2", the topocentric Moon 6.22e-2", after the
 * flag per call (either way); 1.04e-1" (the Moon 1.04e-1") after each
 * change of mode, and the houses on the second thread 2.0e-10" (nodes
 * built at its delta t before it opened the JPL file, 4.3 ms off the one
 * after); SE_ECL_NUT 4.20e-2" between
 * SEFLG_JPLHOR and SEFLG_JPLHOR_APPROX, 0.131" across the corrections.
 * With the nodes keyed and the ayanamsa not, its two flag cases fail
 * (6.19e-2"); without the drop at swe_close(), the corrections' case. The
 * upstream build fails 113 of 246: 129's 74 and 39 of these, all but the
 * second thread's ten (adopted through this fork's code, fixed in both
 * builds), and after swe_close() the topocentric Moon, the ayanamsa and
 * the sidereal Mars as in 129 (125 before the fix).
 *
 * And the steps (UPSTREAM-BUGS 146): the three nodes could lie both sides
 * of a step of what they interpolate. With SEFLG_JPLHOR_APPROX,
 * SEMOD_JPLHORA_3 takes IAU 1980 with a fixed offset up to 1962 Jan 20
 * and the nutation model after it, 7.17e-2" in dpsi on the default model
 * (6.43e-2", the offset, on IAU 1980); the interpolation across it was
 * 6.93e-2" off at worst. calc_nutation() steps nowhere else: SEFLG_JPLHOR's
 * corrections hold their end values beyond their table, SEMOD_JPLHORA_2's
 * offset is constant, and the series take no date. The check, on de200 and
 * explicit contexts: SE_ECL_NUT and Mars at the switch + 0.3 after a call
 * at - 0.5, bit for bit (with the return) the interpolation off's; and a
 * scan, each node centre c within 2 days of the switch by 1/4 day after a
 * call far off, then the queries c + j/4 (|j| <= 3) against it off: bit
 * for bit where c's nodes straddle the switch, within 3.5 mas elsewhere
 * (the interpolation's own "about 3 mas"; 3.06 mas worst over 1800-2100).
 * On the default model and on IAU 1980, and where nothing steps, within
 * 3.5 mas throughout: SEMOD_JPLHORA_2 and no flag at the switch's date, and
 * SEFLG_JPLHOR on fixture A about the corrections' first and last day. 104
 * checks, 350 in all.
 *
 * Measured before the fix, 18 of the 104 fail: SE_ECL_NUT and Mars at the
 * switch + 0.3, -10.105320" for -10.124814" (1.95e-2"), and the eight
 * straddling centres on each model, the default's 2.74e-2" to 6.01e-2",
 * IAU 1980's 2.46e-2" to 5.38e-2". Away from them the answers are bit for
 * bit the tip's. The upstream build fails 131 of 350: 113 and these 18.
 * (Since UPSTREAM-BUGS 231 the scan takes the nodes of each query's day
 * and the limit 0.1 mas: below.)
 *
 * And the order (UPSTREAM-BUGS 231): the nodes were placed by whichever
 * call found none about it, t - 1, t and t + 1 of that call, which it
 * answered exactly, so an instant's nutation depended on the calls before
 * it. They are at 0h TT of the instant's day and of the next now, and the
 * nutation the cubic through its values and rates there. On explicit
 * contexts, Moshier, the interpolation on, under the default, IAU 1980
 * and IAU 2000B: SE_ECL_NUT and Mars with SEFLG_SPEED at 40 instants
 * 1900-2100 on a fresh context against one that asked the same 0.6..1.3
 * day before, and one after, bit for bit with the return; SE_ECL_NUT at
 * 400 instants 1800-2100, each after a call within a day of it, against
 * the interpolation off within OR_TOL, 0.1 mas; Mars's speeds in longitude
 * and right ascension against five-point differences of its positions over
 * 2^-6 day at 40 instants floor(t * 256) / 256 within a day's middle and
 * 40 at or within 2^-5 day of 0h TT, where the stencil spans a day's end,
 * within 1e-8 deg/day. 21 checks, 371 in all. The scan above follows the
 * grid: bit for bit where the query's day is the one the step begins,
 * within ST_TOL, 0.1 mas, elsewhere (3.5 mas before, the quadratic's).
 *
 * Measured after the fix: the orders 0; the interpolation within 5.9e-5"
 * in dpsi and 3.1e-5" in deps; the speeds within 5.9e-11 deg/day in a
 * day's middle and 7.0e-10 about 0h TT (the cubic's curvature steps at its
 * nodes; upstream's quadratic on the same grid would step the rate there,
 * 2.3e-6 deg/day off the five-point difference); the scan 0 on the step's
 * day, 1.9e-5" elsewhere. Before the fix 101 of 371 fail: the 12 orders
 * (SE_ECL_NUT 6.3e-7 deg, Mars 4.2e-6 with its speed), the 3 accuracies
 * (2.8 mas) and 86 of the scan's 102 at its new limit (1.0 mas); the
 * speeds pass, a fresh context's nodes being about its first call. The
 * upstream build fails 238 of 371: entry 134's 113, entry 146's 104 (its
 * scan at the new limit) and these 21.
 * Run as
 *   nutintp EPHEDIR
 */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
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

/* the probes' ephemeris and JPL Horizons flags (UPSTREAM-BUGS 134) */
static int32 XFL = SEFLG_SWIEPH;

/* the probe at t, on c, or on the default context for c NULL */
static void probe(swe_ctx *c, int p, double t, struct obs *o)
{
  char serr[AS_MAXCH], s[64];
  int32 fl = XFL | SEFLG_SPEED;
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
      o->ret = c ? swe_get_ayanamsa_ex_r(c, t, XFL, o->v, serr)
                 : swe_get_ayanamsa_ex(t, XFL, o->v, serr);
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

/* The models (UPSTREAM-BUGS 129): the interpolation on, a change of
 * nutation model by each route; and off, a change of sidereal-time model,
 * which places the observer as the nutation does. */
#define MOD_DEF  "0,0,0,0,0,0,0,0"
#define MOD_1980 "0,0,0,1,0,0,0,0"	/* SEMOD_NUT_IAU_1980 */
#define MOD_ST76 "0,0,0,0,0,0,0,1"	/* SEMOD_SIDT_IAU_1976 */
#define SID_ORIG (SE_SIDM_LAHIRI | SE_SIDBIT_PREC_ORIG)	/* IAU 1976, IAU 1980 */
enum { R_SET, R_THREAD, R_CTX, R_SID, R_CLOSE, R_CLOSETH, NR };
static const char *RN[NR] = { "swe_set_astro_models()",
  "a second thread adopting swe_set_astro_models()",
  "swe_set_astro_models_r() on an explicit context",
  "swe_set_sid_mode(SE_SIDBIT_PREC_ORIG)", "swe_close()",
  "a second thread adopting swe_close()" };
struct scen {
  int route, intp;
  const char *ma, *mb;	/* the models before and after */
  int32 sa, sb;		/* the sidereal mode before and after */
};
static const struct scen SC[] = {
  { R_SET, 1, MOD_DEF, MOD_1980, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_SET, 1, MOD_1980, MOD_DEF, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_THREAD, 1, MOD_DEF, MOD_1980, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_THREAD, 1, MOD_1980, MOD_DEF, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_CTX, 1, MOD_DEF, MOD_1980, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_CTX, 1, MOD_1980, MOD_DEF, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_SID, 1, MOD_DEF, MOD_DEF, SE_SIDM_TRUE_CITRA, SID_ORIG },
  { R_CLOSE, 1, MOD_1980, MOD_DEF, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_CLOSETH, 1, MOD_1980, MOD_DEF, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_SET, 0, MOD_DEF, MOD_ST76, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_THREAD, 0, MOD_DEF, MOD_ST76, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
  { R_CTX, 0, MOD_DEF, MOD_ST76, SE_SIDM_TRUE_CITRA, SE_SIDM_TRUE_CITRA },
};
#define NSC ((int) (sizeof SC / sizeof SC[0]))

static void setup2(swe_ctx *c, int32 sid, const char *mod, int intp)
{
  char m[40];
  strcpy(m, mod);
  if (c) {
    swe_set_ephe_path_r(c, EPHE);
    swe_set_topo_r(c, GEO[0], GEO[1], GEO[2]);
    swe_set_astro_models_r(c, m, 0);
    swe_set_sid_mode_r(c, sid, 0, 0);
    swe_set_interpolate_nut_r(c, intp);
  } else {
    swe_set_ephe_path(EPHE);
    swe_set_topo(GEO[0], GEO[1], GEO[2]);
    swe_set_astro_models(m, 0);
    swe_set_sid_mode(sid, 0, 0);
    swe_set_interpolate_nut(intp);
  }
}

static void change(swe_ctx *c, const struct scen *s)
{
  char m[40];
  strcpy(m, s->mb);
  switch (s->route) {
    case R_SET: case R_THREAD:
      swe_set_astro_models(m, 0);
      break;
    case R_CTX:
      swe_set_astro_models_r(c, m, 0);
      break;
    case R_SID:
      swe_set_sid_mode(s->sb, 0, 0);
      break;
    case R_CLOSE: case R_CLOSETH:
      swe_close();
      break;
  }
}

/* the scenario, or (fresh) a process that set its final state first:
 * for swe_close() what it keeps, the path and the interpolation, and the
 * observer and the sidereal mode (entry 127) */
static void run2(int k, int p, int fresh, struct obs *o)
{
  const struct scen *s = &SC[k];
  if (s->route == R_CTX) {
    swe_ctx *c = swe_ctx_new();
    if (c == NULL)
      exit(3);
    setup2(c, fresh ? s->sb : s->sa, fresh ? s->mb : s->ma, s->intp);
    if (!fresh) {
      before(c, p);
      change(c, s);
    }
    after(c, p, o);
    swe_ctx_free(c);
  } else if (fresh && (s->route == R_CLOSE || s->route == R_CLOSETH)) {
    swe_set_ephe_path(EPHE);
    swe_set_topo(GEO[0], GEO[1], GEO[2]);
    swe_set_sid_mode(s->sb, 0, 0);
    swe_set_interpolate_nut(s->intp);
    after(NULL, p, o);
  } else if (!fresh && (s->route == R_THREAD || s->route == R_CLOSETH)) {
    pthread_t th;
    setup2(NULL, s->sa, s->ma, s->intp);
    th_p = p;
    pthread_barrier_init(&bar, NULL, 2);
    if (pthread_create(&th, NULL, second, NULL) != 0)
      exit(3);
    pthread_barrier_wait(&bar);
    pthread_barrier_wait(&bar);
    change(NULL, s);
    pthread_barrier_wait(&bar);
    pthread_join(th, NULL);
    *o = th_o;
  } else {
    setup2(NULL, fresh ? s->sb : s->sa, fresh ? s->mb : s->ma, s->intp);
    if (!fresh) {
      before(NULL, p);
      change(NULL, s);
    }
    after(NULL, p, o);
  }
}

/* scenario k's run (run2(), run3()) in a forked process */
static int in_child2(void (*run)(int, int, int, struct obs *), int k, int p,
                     int fresh, struct obs *o)
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
    run(k, p, fresh, &r);
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

/* The JPL Horizons flags, the approximation's mode and the IERS
 * corrections (UPSTREAM-BUGS 134), the interpolation on throughout. On
 * de200 at 1950, before 1962, where SEMOD_JPLHORA_3 takes IAU 1980 with
 * an offset and SEMOD_JPLHORA_2 IAU 2000 with another: the flag per call,
 * and the mode by each route. SEFLG_JPLHOR itself needs a DE >= 403 and
 * eop_1962_today.txt: G19's synthetic DE431 header, whose coefficients
 * nothing reads for SE_ECL_NUT, and two such files of other corrections,
 * in two directories made here (A and B). */
#define T0H 2433282.5		/* 1950 Jan 1 */
#define T0F 2451550.5		/* in the corrections' span, 2000 Jan 1..Feb 9 */
#define JE SEFLG_JPLEPH
#define JA (SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX)
#define JH (SEFLG_JPLEPH | SEFLG_JPLHOR)
#define MOD_HA3 "0,0,0,0,0,0,3,0"	/* SEMOD_JPLHORA_3, the default */
#define MOD_HA2 "0,0,0,0,0,0,2,0"	/* SEMOD_JPLHORA_2 */
#define JPL_FIXTURE "de431fixture.eph"
static char DIRS[3][512];	/* EPHE, A, B */
enum { H_FLAG, H_SET, H_THREAD, H_CTX, H_CLOSE, NH };
static const char *HN[NH] = { "the flag per call", "swe_set_astro_models()",
  "a second thread adopting swe_set_astro_models()",
  "swe_set_astro_models_r() on an explicit context",
  "swe_close() and the files set again" };
struct scen3 {
  int route;
  int32 fa, fb;		/* the flags before and after */
  const char *ma, *mb;	/* the models before and after */
  int da, db;		/* the directory before and after: DIRS[] */
};
static const struct scen3 SH[] = {
  { H_FLAG, JA, JE, MOD_HA3, MOD_HA3, 0, 0 },
  { H_FLAG, JE, JA, MOD_HA3, MOD_HA3, 0, 0 },
  { H_SET, JA, JA, MOD_HA3, MOD_HA2, 0, 0 },
  { H_SET, JA, JA, MOD_HA2, MOD_HA3, 0, 0 },
  { H_THREAD, JA, JA, MOD_HA3, MOD_HA2, 0, 0 },
  { H_THREAD, JA, JA, MOD_HA2, MOD_HA3, 0, 0 },
  { H_CTX, JA, JA, MOD_HA3, MOD_HA2, 0, 0 },
  { H_CTX, JA, JA, MOD_HA2, MOD_HA3, 0, 0 },
  { H_CLOSE, JA, JA, MOD_HA2, MOD_DEF, 0, 0 },
  /* SEFLG_JPLHOR, SE_ECL_NUT alone */
  { H_FLAG, JH, JA, MOD_DEF, MOD_DEF, 1, 1 },
  { H_FLAG, JA, JH, MOD_DEF, MOD_DEF, 1, 1 },
  { H_CLOSE, JH, JH, MOD_DEF, MOD_DEF, 1, 2 },
};
#define NSH ((int) (sizeof SH / sizeof SH[0]))

static void setup3(swe_ctx *c, int d, const char *mod)
{
  char m[40], jf[40];
  strcpy(m, mod);
  strcpy(jf, d ? JPL_FIXTURE : "de200.eph");
  if (c) {
    swe_set_ephe_path_r(c, DIRS[d]);
    swe_set_jpl_file_r(c, jf);
    swe_set_topo_r(c, GEO[0], GEO[1], GEO[2]);
    swe_set_astro_models_r(c, m, 0);
    swe_set_sid_mode_r(c, SE_SIDM_TRUE_CITRA, 0, 0);
    swe_set_interpolate_nut_r(c, 1);
  } else {
    swe_set_ephe_path(DIRS[d]);
    swe_set_jpl_file(jf);
    swe_set_topo(GEO[0], GEO[1], GEO[2]);
    swe_set_astro_models(m, 0);
    swe_set_sid_mode(SE_SIDM_TRUE_CITRA, 0, 0);
    swe_set_interpolate_nut(1);
  }
}

/* before the change, under fl: the probe at t0 (the nodes there) and at
 * t0 + 0.37, and the nutation and observer cached there */
static void before3(swe_ctx *c, int p, double t0, int32 fl)
{
  struct obs o;
  XFL = fl;
  probe(c, p, t0, &o);
  probe(c, p, t0 + 0.37, &o);
  probe(c, P_TOPOMOON, t0 + 0.37, &o);
}

/* after it, under fl: the probe at t0 and at t0 + 0.37. Not
 * swe_sidtime()'s nutation at t0, which has no flags and so would build
 * nodes of its own whatever the probe's */
static void after3(swe_ctx *c, int p, double t0, int32 fl, struct obs *o)
{
  XFL = fl;
  probe(c, p, t0, o);
  probe(c, p, t0 + 0.37, o);
}

static void change3(swe_ctx *c, const struct scen3 *s)
{
  char m[40], jf[40];
  strcpy(m, s->mb);
  switch (s->route) {
    case H_FLAG:		/* after3() takes the flags fb */
      break;
    case H_SET: case H_THREAD:
      swe_set_astro_models(m, 0);
      break;
    case H_CTX:
      swe_set_astro_models_r(c, m, 0);
      break;
    case H_CLOSE:
      strcpy(jf, s->db ? JPL_FIXTURE : "de200.eph");
      swe_close();
      swe_set_ephe_path(DIRS[s->db]);
      swe_set_jpl_file(jf);
      break;
  }
}

static const struct scen3 *th_s;
static void *second3(void *arg)
{
  double t0 = th_s->da ? T0F : T0H;
  (void) arg;
  pthread_barrier_wait(&bar);
  before3(NULL, th_p, t0, th_s->fa);
  pthread_barrier_wait(&bar);
  pthread_barrier_wait(&bar);
  after3(NULL, th_p, t0, th_s->fb, &th_o);
  return NULL;
}

/* the scenario, or (fresh) a process that set its final state first */
static void run3(int k, int p, int fresh, struct obs *o)
{
  const struct scen3 *s = &SH[k];
  double t0 = s->da ? T0F : T0H;
  if (s->route == H_CTX) {
    swe_ctx *c = swe_ctx_new();
    if (c == NULL)
      exit(3);
    setup3(c, fresh ? s->db : s->da, fresh ? s->mb : s->ma);
    if (!fresh) {
      before3(c, p, t0, s->fa);
      change3(c, s);
    }
    after3(c, p, t0, s->fb, o);
    swe_ctx_free(c);
  } else if (!fresh && s->route == H_THREAD) {
    pthread_t th;
    setup3(NULL, s->da, s->ma);
    th_p = p;
    th_s = s;
    pthread_barrier_init(&bar, NULL, 2);
    if (pthread_create(&th, NULL, second3, NULL) != 0)
      exit(3);
    pthread_barrier_wait(&bar);
    pthread_barrier_wait(&bar);
    change3(NULL, s);
    pthread_barrier_wait(&bar);
    pthread_join(th, NULL);
    *o = th_o;
  } else {
    setup3(NULL, fresh ? s->db : s->da, fresh ? s->mb : s->ma);
    if (!fresh) {
      before3(NULL, p, t0, s->fa);
      change3(NULL, s);
    }
    after3(NULL, p, t0, s->fb, o);
  }
}

/* G19's synthetic JPL header (eopload.c), DE 431: fsizer() accepts it and
 * the file is the length it implies; SE_ECL_NUT reads no coefficient. */
static void put32(unsigned char *p, int v)    { memcpy(p, &v, 4); }
static void putd (unsigned char *p, double v) { memcpy(p, &v, 8); }
static int write_jpl(const char *path)
{
  unsigned char h[2856];
  const int ncf = 4, na = 1, maxoff = 1200, nrecl = 4;
  const double ss0 = 2451544.5, ss1 = 2451576.5, ss2 = 32.0;   /* nseg = 1 */
  int i, ksize, nseg = (int) ((ss1 - ss0) / ss2);
  long nb = 0;
  FILE *fp;
  memset(h, ' ', 252);
  memcpy(h, "SWISSEPH EOP LOADER FIXTURE", 27);
  memset(h + 252, ' ', 2400);
  putd (h + 2652, ss0);
  putd (h + 2660, ss1);
  putd (h + 2668, ss2);
  put32(h + 2676, 0);                 /* ncon  */
  putd (h + 2680, 149597870.7);       /* au    */
  putd (h + 2688, 81.3);              /* emrat */
  for (i = 0; i < 12; i++) {
    put32(h + 2696 + (i * 3 + 0) * 4, 3);
    put32(h + 2696 + (i * 3 + 1) * 4, ncf);
    put32(h + 2696 + (i * 3 + 2) * 4, na);
  }
  put32(h + 2696 + 30 * 4, maxoff);
  put32(h + 2696 + 31 * 4, ncf);
  put32(h + 2696 + 32 * 4, na);
  put32(h + 2840, 431);               /* numde */
  put32(h + 2844, 3);
  put32(h + 2848, ncf);
  put32(h + 2852, na);
  ksize = (maxoff + 3 * ncf * na - 1) * 2;
  for (i = 0; i < 13; i++)
    nb += (long) ncf * na * (i == 11 ? 2 : 3) * nseg;
  nb += 2 * nseg;
  nb *= 8;
  nb += 2 * (long) ksize * nrecl;
  if ((fp = fopen(path, "wb")) == NULL)
    return -1;
  fwrite(h, 1, sizeof h, fp);
  for (i = (int) sizeof h; i < nb; i++)
    fputc(0, fp);
  if ((fp = freopen(path, "r+b", fp)) == NULL)
    return -1;
  fseek(fp, 2L * ksize * nrecl, SEEK_SET);
  { double ep[2]; ep[0] = ss0; ep[1] = ss1; fwrite(ep, 8, 2, fp); }
  fclose(fp);
  return 0;
}

/* eop_1962_today.txt (EOPC04 columns) for MJD 51544..51583, dpsi and deps
 * (arcsec) a + i b and c + i d */
static int write_eop(const char *path, double a, double b, double c, double d)
{
  FILE *fp = fopen(path, "w");
  int i;
  if (fp == NULL)
    return -1;
  fprintf(fp, "# EOPC04 fixture\n");
  for (i = 0; i < 40; i++)
    fprintf(fp, "2000  1 %2d  %5d   0.0 0.0 0.0 0.0 %.9f %.9f\n",
            i + 1, 51544 + i, a + i * b, c + i * d);
  return fclose(fp);
}

static int make_fixtures(void)
{
  char p[600], base[400];
  const char *tmp = getenv("TMPDIR");
  if (snprintf(base, sizeof base, "%s/nutintp.XXXXXX", tmp && *tmp ? tmp : "/tmp")
      >= (int) sizeof base || mkdtemp(base) == NULL)
    return -1;
  snprintf(DIRS[1], sizeof DIRS[1], "%s", base);
  snprintf(DIRS[2], sizeof DIRS[2], "%s/B", base);
  if (mkdir(DIRS[2], 0700) != 0)
    return -1;
  snprintf(p, sizeof p, "%s/%s", DIRS[1], JPL_FIXTURE);
  if (write_jpl(p) != 0)
    return -1;
  snprintf(p, sizeof p, "%s/%s", DIRS[2], JPL_FIXTURE);
  if (write_jpl(p) != 0)
    return -1;
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", DIRS[1]);
  if (write_eop(p, -0.05, -1e-5, -0.005, -1e-6) != 0)
    return -1;
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", DIRS[2]);
  if (write_eop(p, 0.08, 1e-4, 0.01, 1e-5) != 0)
    return -1;
  return 0;
}

static void remove_fixtures(void)
{
  char p[600];
  int d;
  for (d = 2; d >= 1; d--) {
    snprintf(p, sizeof p, "%s/%s", DIRS[d], JPL_FIXTURE);
    unlink(p);
    snprintf(p, sizeof p, "%s/eop_1962_today.txt", DIRS[d]);
    unlink(p);
  }
  rmdir(DIRS[2]);
  rmdir(DIRS[1]);
}

/* The steps of calc_nutation() (UPSTREAM-BUGS 146). With
 * SEFLG_JPLHOR_APPROX, SEMOD_JPLHORA_3 takes IAU 1980 with a fixed offset
 * up to 1962 Jan 20 and the nutation model after it. A scan about it: for
 * each c = TS + i/4 (|i| <= 8), on an explicit context with the
 * interpolation on, a call at c + 500 and at c (other days' nodes), and
 * the queries c + j/4 (|j| <= 3), each against a context with it off: bit
 * for bit, with the return, where the query's day's nodes (0h TT of it and
 * of the next, UPSTREAM-BUGS 231) would lie both sides of the step, the
 * day it begins, and within the interpolation's own accuracy (ST_TOL)
 * elsewhere. And where nothing steps, the same scan within ST_TOL
 * throughout: SEMOD_JPLHORA_2 and no flag at the switch, and SEFLG_JPLHOR
 * at the first and the last day of the corrections (fixture A), which
 * hold their end values beyond them. */
#define TSTEP 2437684.5		/* 1962 Jan 20, HORIZONS_TJD0_DPSI_DEPS_IAU1980 */
#define ST_TOL (0.1e-3 / 3600)	/* deg: the day-grid cubic's (UPSTREAM-BUGS 231) */
#define MOD_1980HA3 "0,0,0,1,0,0,3,0"	/* IAU 1980, SEMOD_JPLHORA_3 */
struct scen4 {
  const char *mod;
  int32 fl;
  int d;		/* DIRS[] */
  double ts;		/* the instant scanned about */
  int step;		/* whether the nutation steps there */
  const char *lab;
};
static const struct scen4 SS[] = {
  { MOD_HA3, JA, 0, TSTEP, 1, "SEMOD_JPLHORA_3's switch, the default model" },
  { MOD_1980HA3, JA, 0, TSTEP, 1, "SEMOD_JPLHORA_3's switch, IAU 1980" },
  { MOD_HA2, JA, 0, TSTEP, 0, "the switch's date under SEMOD_JPLHORA_2" },
  { MOD_DEF, JE, 0, TSTEP, 0, "the switch's date without the flag" },
  { MOD_DEF, JH, 1, 2451544.5, 0, "SEFLG_JPLHOR, the corrections' first day" },
  { MOD_DEF, JH, 1, 2451583.5, 0, "SEFLG_JPLHOR, the corrections' last day" },
};
#define NSS ((int) (sizeof SS / sizeof SS[0]))

static swe_ctx *ctx4(const struct scen4 *s, int intp)
{
  char m[40], jf[40];
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  strcpy(m, s->mod);
  strcpy(jf, s->d ? JPL_FIXTURE : "de200.eph");
  swe_set_ephe_path_r(c, DIRS[s->d]);
  swe_set_jpl_file_r(c, jf);
  swe_set_astro_models_r(c, m, 0);
  swe_set_interpolate_nut_r(c, intp);
  return c;
}

/* body ipl at q after calls at c + 500 and c, interpolated (on ci) and
 * exact (on ce): the larger difference of the first four values, deg, or
 * -1 if the returns differ; *same whether they agree bit for bit */
static double vs_exact(swe_ctx *ci, swe_ctx *ce, int ipl, int32 fl, double c,
                       double q, int *same, double *vi, double *ve)
{
  char serr[AS_MAXCH];
  double x[6], y[6], w = 0;
  int ri, re, k;
  swe_calc_r(ci, c + 500, ipl, fl, x, serr);
  swe_calc_r(ci, c, ipl, fl, x, serr);
  ri = swe_calc_r(ci, q, ipl, fl, x, serr);
  re = swe_calc_r(ce, q, ipl, fl, y, serr);
  *same = ri == re;
  for (k = 0; k < 4; k++) {
    if (x[k] != y[k])
      *same = 0;
    if (fabs(x[k] - y[k]) > w)
      w = fabs(x[k] - y[k]);
  }
  if (vi != NULL) {
    *vi = x[2];
    *ve = y[2];
  }
  return ri == re ? w : -1;
}

/* the headline (entry 134's noticed item): SE_ECL_NUT and Mars at the
 * switch + 0.3 after a call at - 0.5, bit for bit; and the scans */
static int steps(int *n)
{
  int k, i, j, nfail = 0;
  for (k = 0; k < 2; k++) {
    swe_ctx *ci = ctx4(&SS[0], 1), *ce = ctx4(&SS[0], 0);
    double w, vi = 0, ve = 0;
    int same;
    (*n)++;
    if (ci == NULL || ce == NULL) {
      printf("    no context\n");
      nfail++;
      continue;
    }
    w = vs_exact(ci, ce, k ? SE_MARS : SE_ECL_NUT, JA, TSTEP - 0.5, TSTEP + 0.3,
                 &same, &vi, &ve);
    if (!same) {
      printf("    %s at SEMOD_JPLHORA_3's switch + 0.3 after a call at - 0.5: %.3e\" off the exact%s FAIL\n",
             k ? "Mars" : "SE_ECL_NUT", w * 3600, w < 0 ? " (return)" : "");
      if (!k)
        printf("      dpsi %.6f\" for %.6f\"\n", vi * 3600, ve * 3600);
      nfail++;
    }
    swe_ctx_free(ci);
    swe_ctx_free(ce);
  }
  for (k = 0; k < NSS; k++) {
    const struct scen4 *s = &SS[k];
    swe_ctx *ci = ctx4(s, 1), *ce = ctx4(s, 0);
    double wstr = 0, woff = 0;
    int bad = 0;
    for (i = -8; i <= 8; i++) {
      double c = s->ts + i / 4.0, wc = 0;
      int cbad = 0;
      (*n)++;
      if (ci == NULL || ce == NULL) {
        nfail++;
        continue;
      }
      for (j = -3; j <= 3; j++) {
        int same;
        double q = c + j / 4.0, qd = floor(q - 0.5) + 0.5;
        int straddle = s->step && qd <= s->ts && qd + 1.0 > s->ts;
        double w = vs_exact(ci, ce, SE_ECL_NUT, s->fl, c, q, &same, NULL, NULL);
        if (w < 0 || (straddle ? !same : w > ST_TOL))
          cbad = 1;
        if (w < 0)
          w = 360;
        if (w > wc)
          wc = w;
        if (straddle && w > wstr)
          wstr = w;
        if (!straddle && w > woff)
          woff = w;
      }
      if (cbad) {
        printf("    %s, after a call at %+.2f d: %.3e\" off the exact FAIL\n", s->lab, i / 4.0,
               wc * 3600);
        nfail++;
        bad++;
      }
    }
    if (s->step)
      printf("    %s: the day it begins %.3e\", the others %.3e\"%s\n", s->lab,
             wstr * 3600, woff * 3600, bad ? "" : " OK");
    else
      printf("    %s: %.3e\"%s\n", s->lab, woff * 3600, bad ? "" : " OK");
    if (ci)
      swe_ctx_free(ci);
    if (ce)
      swe_ctx_free(ce);
  }
  return nfail;
}

/* The order (UPSTREAM-BUGS 231): the nodes were placed by whichever call
 * found none about it, so an instant's nutation depended on the calls
 * before it. On explicit contexts, Moshier, the interpolation on, under
 * the default, IAU 1980 and IAU 2000B:
 *  - SE_ECL_NUT, and Mars with SEFLG_SPEED, at 40 instants 1900-2100, on a
 *    fresh context against one that asked the same at t - dt, and one at
 *    t + dt, dt 0.6..1.3 day: bit for bit with the return;
 *  - SE_ECL_NUT at 400 instants 1800-2100 on one context, each after a
 *    call at another instant within a day of it, against the interpolation
 *    off: within OR_TOL;
 *  - Mars's speed in longitude and right ascension against five-point
 *    differences of its positions over 2^-6 day at 40 instants
 *    floor(t * 256) / 256 within a day's middle, and 40 at 0h TT or within
 *    2^-5 day of it, where the stencil spans a day's end: within 1e-8
 *    deg/day, a context each. */
#define OR_TOL (0.1e-3 / 3600)	/* deg */
static swe_ctx *ctx_or(const char *mod, int intp)
{
  char m[40];
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  strcpy(m, mod);
  swe_set_ephe_path_r(c, EPHE);
  swe_set_astro_models_r(c, m, 0);
  swe_set_interpolate_nut_r(c, intp);
  return c;
}

static int order(int *n)
{
  static const char *mod[3] = { MOD_DEF, MOD_1980, "0,0,0,4,0,0,0,0" };
  static const char *mn[3] = { "the default", "IAU 1980", "IAU 2000B" };
  static const int32 pfl[2] = { SEFLG_MOSEPH, SEFLG_MOSEPH | SEFLG_SPEED };
  static const int ppl[2] = { SE_ECL_NUT, SE_MARS };
  const double h = 1.0 / 64;
  char serr[AS_MAXCH];
  int m, p, dir, i, k, nfail = 0;
  for (m = 0; m < 3; m++) {
    /* the order */
    double wo[4] = { 0, 0, 0, 0 };
    for (p = 0; p < 2; p++)
      for (dir = -1; dir <= 1; dir += 2) {
        double w = 0;
        int bad = 0;
        srand(231 + m);
        for (i = 0; i < 40; i++) {
          double t = 2415020.5 + 73049.0 * (rand() / (double) RAND_MAX);
          double dt = 0.6 + 0.7 * (rand() / (double) RAND_MAX), x[6], y[6];
          swe_ctx *a = ctx_or(mod[m], 1), *b = ctx_or(mod[m], 1);
          int ra, rb;
          if (a == NULL || b == NULL)
            return nfail + 1;
          ra = swe_calc_r(a, t, ppl[p], pfl[p], x, serr);
          swe_calc_r(b, t + dir * dt, ppl[p], pfl[p], y, serr);
          rb = swe_calc_r(b, t, ppl[p], pfl[p], y, serr);
          if (ra != rb || ra < 0)
            bad = 1;
          for (k = 0; k < 6; k++) {
            if (x[k] != y[k])
              bad = 1;
            if (fabs(x[k] - y[k]) > w)
              w = fabs(x[k] - y[k]);
          }
          swe_ctx_free(a);
          swe_ctx_free(b);
        }
        (*n)++;
        wo[2 * p + (dir > 0)] = bad && w == 0 ? 360 : w;
        if (bad)
          nfail++;
      }
    printf("    %s, after a call 0.6..1.3 d before, after: SE_ECL_NUT %.3e, %.3e deg,"
           " Mars %.3e, %.3e deg off a fresh context's%s\n", mn[m], wo[0], wo[1], wo[2], wo[3],
           wo[0] || wo[1] || wo[2] || wo[3] ? " FAIL" : " OK");
    /* the accuracy */
    {
      double w[2] = { 0, 0 };
      swe_ctx *ci = ctx_or(mod[m], 1), *ce = ctx_or(mod[m], 0);
      if (ci == NULL || ce == NULL)
        return nfail + 1;
      srand(2310 + m);
      for (i = 0; i < 400; i++) {
        double t = 2378496.5 + 109574.0 * (rand() / (double) RAND_MAX), x[6], y[6];
        double u = 2 * (rand() / (double) RAND_MAX) - 1;
        if (swe_calc_r(ci, t, SE_ECL_NUT, SEFLG_MOSEPH, x, serr) < 0
            || swe_calc_r(ci, t + u, SE_ECL_NUT, SEFLG_MOSEPH, x, serr) < 0
            || swe_calc_r(ce, t + u, SE_ECL_NUT, SEFLG_MOSEPH, y, serr) < 0)
          w[0] = 360;
        for (k = 0; k < 2; k++)
          if (fabs(x[2 + k] - y[2 + k]) > w[k])
            w[k] = fabs(x[2 + k] - y[2 + k]);
      }
      swe_ctx_free(ci);
      swe_ctx_free(ce);
      (*n)++;
      printf("    %s, interpolated against the series, 1800-2100: dpsi %.3e\", deps %.3e\"%s\n", mn[m],
             w[0] * 3600, w[1] * 3600, w[0] > OR_TOL || w[1] > OR_TOL ? " FAIL" : " OK");
      if (w[0] > OR_TOL || w[1] > OR_TOL)
        nfail++;
    }
    /* the rate */
    for (p = 0; p < 2; p++) {
      double w = 0;
      srand(2320 + m);
      for (i = 0; i < 40; i++) {
        double t = 2415020.5 + 73049.0 * (rand() / (double) RAND_MAX), d = floor(t - 0.5) + 0.5;
        int e;
        if (p == 0)
          t = d + 0.0625 + 0.875 * (rand() / (double) RAND_MAX);
        else
          t = d + (i % 4 == 0 ? 0 : 2 * h * (2 * (rand() / (double) RAND_MAX) - 1));
        t = floor(t * 256) / 256;
        for (e = 0; e < 2; e++) {
          int32 fl = SEFLG_MOSEPH | (e ? SEFLG_EQUATORIAL : 0);
          double sp[6], x[5][6], fd;
          swe_ctx *c = ctx_or(mod[m], 1);
          if (c == NULL)
            return nfail + 1;
          if (swe_calc_r(c, t, SE_MARS, fl | SEFLG_SPEED, sp, serr) < 0)
            w = 360;
          for (k = 0; k < 5; k++)
            if (swe_calc_r(c, t + (k - 2) * h, SE_MARS, fl, x[k], serr) < 0)
              w = 360;
          swe_ctx_free(c);
          fd = (8 * (x[3][0] - x[1][0]) - (x[4][0] - x[0][0])) / (12 * h);
          if (fabs(sp[3] - fd) > w)
            w = fabs(sp[3] - fd);
        }
      }
      (*n)++;
      printf("    %s, Mars's speed against its positions' five-point difference, %s: %.3e deg/day%s\n",
             mn[m], p ? "about 0h TT" : "within a day", w, w > 1e-8 ? " FAIL" : " OK");
      if (w > 1e-8)
        nfail++;
    }
  }
  return nfail;
}

/* how far a from b, and whether they differ at all (with the return) */
static int differ(const struct obs *a, const struct obs *b, double *d)
{
  int i, bad = a->ret != b->ret;
  *d = 0;
  for (i = 0; i < NV; i++) {
    if (a->v[i] != b->v[i])
      bad = 1;
    if (fabs(a->v[i] - b->v[i]) > *d)
      *d = fabs(a->v[i] - b->v[i]);
  }
  return bad;
}

int main(int argc, char **argv)
{
  int m, p, dir, i, k, n = 0, nfail = 0;
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
  for (k = 0; k < NSC; k++)
    for (p = 0; p < NP; p++) {
      struct obs ot, of;
      double d;
      n++;
      if (in_child2(run2, k, p, 0, &ot) != 0 || in_child2(run2, k, p, 1, &of) != 0) {
        printf("    %s, %s to %s: child failed\n", RN[SC[k].route], SC[k].ma, SC[k].mb);
        nfail++;
        continue;
      }
      if (differ(&ot, &of, &d)) {
        printf("    %s, interpolation %s, %s to %s, %s: %.3e\" off a fresh process's (return %d, %d) FAIL\n",
               RN[SC[k].route], SC[k].intp ? "on" : "off", SC[k].ma, SC[k].mb, PN[p], d * 3600, ot.ret, of.ret);
        nfail++;
      }
    }
  snprintf(DIRS[0], sizeof DIRS[0], "%s", EPHE);
  if (make_fixtures() != 0) {
    printf("    cannot write the SEFLG_JPLHOR fixtures in %s\n", DIRS[1]);
    nfail++;
  } else {
    for (k = 0; k < NSH; k++)
      for (p = 0; p < (SH[k].da ? 1 : NP); p++) {	/* the fixtures: SE_ECL_NUT */
        struct obs ot, of;
        double d;
        n++;
        if (in_child2(run3, k, p, 0, &ot) != 0 || in_child2(run3, k, p, 1, &of) != 0) {
          printf("    %s, flags %#x to %#x, %s to %s: child failed\n", HN[SH[k].route],
                 (unsigned) SH[k].fa, (unsigned) SH[k].fb, SH[k].ma, SH[k].mb);
          nfail++;
          continue;
        }
        if (differ(&ot, &of, &d)) {
          printf("    %s, flags %#x to %#x, %s to %s%s, %s: %.3e\" off a fresh process's (return %d, %d) FAIL\n",
                 HN[SH[k].route], (unsigned) SH[k].fa, (unsigned) SH[k].fb, SH[k].ma, SH[k].mb,
                 SH[k].da != SH[k].db ? ", corrections A to B" : "", PN[p], d * 3600, ot.ret, of.ret);
          nfail++;
        }
      }
    nfail += steps(&n);
    nfail += order(&n);
    remove_fixtures();
  }
  if (nfail)
    printf("G122 FAIL: %d of %d\n", nfail, n);
  else
    printf("G122 PASS (%d)\n", n);
  return nfail ? 1 : 0;
}
