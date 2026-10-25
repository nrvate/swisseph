/* firstset.c -- G82: what is set before the first calculation stays set.
 *
 * The defects (UPSTREAM-BUGS 83). On the process-wide default context,
 *
 *   swe_set_ephe_path(path);
 *   swe_vis_limit_mag(tjd_ut, dgeo, datm, dobs, "venus", 0, dret, serr);
 *
 * returned ERR, "geographic position has not been set", at any instant;
 * the same call again worked, and after any other setter, or swe_close()
 * and swe_set_ephe_path(), the next one failed again. A setter publishes
 * its group to the shared configuration master, whose other groups held
 * zeros (the master starts, and swe_close() re-arms it, as zeros), and the
 * first swi_config_sync() after it -- the first swe_calc() inside the call
 * -- adopted every group the context had not claimed: the observer that
 * swe_vis_limit_mag() had set for itself a moment before, unclaimed under
 * SWI_CFG_LOCAL, came back "not set" (swehel.c did not sync first, the
 * order G20 fixed in swecl.c), and the lapse rate came back 0 for 0.0065
 * after any setter at all. Both are this fork's. So is the lapse rate set
 * before any calculation being wiped by the default context's first-use
 * memset (swi_init_swed_if_start()): upstream kept it in a static. Shared
 * with upstream: swe_set_delta_t_userdef(), swe_set_tid_acc() and
 * swe_set_interpolate_nut() did not initialise the context, so their
 * settings made before any calculation were wiped by that memset.
 *
 * The check, one forked process per scenario so the library starts
 * pristine in each: every public setter (topo, sidereal mode, user delta
 * t, tidal acceleration, JPL file, astro models, nutation interpolation,
 * lapse rate, ephemeris fallback) and every heliacal entry point
 * (swe_vis_limit_mag, swe_heliacal_pheno_ut, swe_heliacal_ut,
 * swe_topo_arcus_visionis, swe_heliacal_angle), done
 *   first   swe_set_ephe_path(); action; observe
 *   bare    action; observe, the files named by SE_EPHE_PATH (setters)
 *   reopen  path; calc; swe_close(); path; action; observe
 *   renv    SE_EPHE_PATH; calc; swe_close(); action; observe (setters)
 * against the same action in a context warm from a calculation. And with
 * no setter, the observation after swe_set_ephe_path() against the one
 * with SE_EPHE_PATH: the library's defaults either way. The observations
 * are what each setter governs (a topocentric Moon, the ayanamsa and a
 * sidereal Sun, delta t and a Moon in UT, the tidal acceleration and
 * swe_deltat() in AD 1000, a de200 Mars, the true obliquity and nutation,
 * a nutation between interpolation nodes, a Sun refracted or not by the
 * dip at 2000 m, a JPL request with the fallback on), with return flags
 * and serr; within 1e-9 (deg, s, days, mag). Each setter must move its
 * observation against the unset library by more than 1e-7, so the check
 * sees the setting.
 *
 * Measured before the fix, 9 of 54 fail: swe_vis_limit_mag() and
 * swe_heliacal_pheno_ut() first and after reopen (ERR, "geographic
 * position has not been set"); the lapse rate unset (0 after the first
 * calculation, so the Sun 15.75 min before its true rise is refracted to
 * -1.243 deg where it is -2.215, below the dip: 0.97 deg) and set first
 * (0.05 K/m read as 0.0065: 0.90 deg); and set first, the user delta t
 * (63.83 s, the automatic one, for 86.4), the tidal acceleration (-25.80
 * for DE200's -23.89, swe_deltat() in AD 1000 1463.43 s for 1305.30) and
 * the nutation interpolation (lost: 8.6e-8 deg). The compat build before
 * the fix fails the same 9, after it the three shared with upstream. The
 * upstream build (3fd0f95, built with -DUPSTREAM_API, which leaves out the
 * fork-only fallback setter) fails 4 of 49: those three, and
 * swe_set_astro_models() set first (1.2e-5 deg in SE_ECL_NUT), which
 * this fork already keeps. Run as
 *   firstset EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T_OBS   2451545.3     /* 2000 Jan 1, 19:12 UT */
#define T_WARM  2451560.0     /* the warming calculation, elsewhere */
#define NV 24
#define TOL 1e-9
#define EFFECT 1e-7

static const char *EPHE;
static double GEO[3] = { 8.55, 47.37, 400 };

struct obs { int n; double v[NV]; char err[AS_MAXCH]; };

static void put(struct obs *o, double v) { if (o->n < NV) o->v[o->n++] = v; }

/* the setters ------------------------------------------------------- */
enum { S_TOPO, S_SID, S_DT, S_TID, S_JPL, S_MODELS, S_INTP, S_LAPSE,
#ifndef UPSTREAM_API
       S_FALLBACK,
#endif
       NSET };
static const char *setname[] = { "swe_set_topo", "swe_set_sid_mode",
  "swe_set_delta_t_userdef", "swe_set_tid_acc", "swe_set_jpl_file",
  "swe_set_astro_models", "swe_set_interpolate_nut", "swe_set_lapse_rate",
  "swe_set_ephe_fallback" };

static void do_set(int k)
{
  char s[40];
  switch (k) {
    case S_TOPO:   swe_set_topo(GEO[0], GEO[1], GEO[2]); break;
    case S_SID:    swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0); break;
    case S_DT:     swe_set_delta_t_userdef(86.4 / 86400.0); break;
    case S_TID:    swe_set_tid_acc(SE_TIDAL_DE200); break;
    case S_JPL:    swe_set_jpl_file("de200.eph"); break;
    case S_MODELS: strcpy(s, "SE1.64"); swe_set_astro_models(s, SEFLG_SWIEPH); break;
    case S_INTP:   swe_set_interpolate_nut(TRUE); break;
    case S_LAPSE:  swe_set_lapse_rate(0.05); break;
#ifndef UPSTREAM_API
    case S_FALLBACK: swe_set_ephe_fallback(1); break;
#endif
  }
}

static void observe_set(int k, struct obs *o)
{
  double x[6], tret[10];
  char serr[AS_MAXCH] = "";
  int32 rf = 0;
  double geo[3] = { GEO[0], GEO[1], 2000 };
  switch (k) {
    case S_TOPO:
      rf = swe_calc_ut(T_OBS, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_TOPOCTR, x, serr);
      put(o, rf); put(o, x[0]); put(o, x[1]); put(o, x[2] * 1e4); put(o, x[3]);
      break;
    case S_SID:
      put(o, swe_get_ayanamsa_ut(T_OBS));
      rf = swe_calc_ut(T_OBS, SE_SUN, SEFLG_SWIEPH | SEFLG_SIDEREAL, x, serr);
      put(o, rf); put(o, x[0]);
      break;
    case S_DT:     /* the calculation first: it is what initialises */
      rf = swe_calc_ut(T_OBS, SE_MOON, SEFLG_SWIEPH, x, serr);
      put(o, rf); put(o, x[0]);
      put(o, swe_deltat_ex(T_OBS, SEFLG_SWIEPH, serr) * 86400);
      break;
    case S_TID:    /* no swe_calc(): its ephemeris setup resets it */
      put(o, swe_get_tid_acc());
      put(o, swe_deltat(2086308.0) * 86400);   /* AD 1000 */
      break;
    case S_JPL:
      rf = swe_calc(T_OBS, SE_MARS, SEFLG_JPLEPH, x, serr);
      put(o, rf); put(o, x[0]); put(o, x[1]);
      break;
    case S_MODELS:
      rf = swe_calc(T_OBS, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
      put(o, rf); put(o, x[0]); put(o, x[1]); put(o, x[2]); put(o, x[3]);
      break;
    case S_INTP:   /* the first sets the nodes, the second interpolates */
      swe_calc(T_OBS, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
      rf = swe_calc(T_OBS + 0.37, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
      put(o, rf); put(o, x[0] * 1e3); put(o, x[2] * 1e3); put(o, x[3] * 1e3);
      break;
    case S_LAPSE:  /* the Sun from 2000 m 14 and 15.75 min before its true
                    * rise, refracted to -1.066 and -1.243 deg: the dip is
                    * -1.235 at the default lapse rate, -0.977 at 0.05 K/m
                    * and -1.269 at 0, so the first point is refracted or
                    * not by 0.05 against the default, the second by 0 */
      rf = swe_rise_trans(T_OBS, SE_SUN, NULL, SEFLG_SWIEPH,
                          SE_CALC_RISE | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION,
                          geo, 0, 10, tret, serr);
      put(o, rf);
      if (rf != ERR) {
        double xaz[3], t;
        int i;
        for (i = 0; i < 2; i++) {
          t = tret[0] - (i ? 15.75 : 14.0) / 1440;
          swe_calc_ut(t, SE_SUN, SEFLG_SWIEPH, x, serr);
          swe_azalt(t, SE_ECL2HOR, geo, 1013.25, 10, x, xaz);
          put(o, xaz[2]);
        }
      }
      break;
#ifndef UPSTREAM_API
    case S_FALLBACK:
      swe_set_jpl_file("de999.eph");   /* not there */
      rf = swe_calc(T_OBS, SE_MARS, SEFLG_JPLEPH, x, serr);
      put(o, rf); put(o, rf == ERR ? 0 : x[0]);
      break;
#endif
  }
  if (rf == ERR)
    snprintf(o->err, sizeof o->err, "%s", serr);
}

/* the heliacal entry points ------------------------------------------ */
enum { H_VISLIM, H_PHENO, H_HELUT, H_ARCVIS, H_HELANG, NHEL };
static const char *helname[] = { "swe_vis_limit_mag", "swe_heliacal_pheno_ut",
  "swe_heliacal_ut", "swe_topo_arcus_visionis", "swe_heliacal_angle" };

static void do_hel(int k, struct obs *o)
{
  double datm[4] = { 1013.25, 15, 40, 0 }, dobs[6] = { 36, 1, 0, 0, 0, 0 };
  double dret[50];
  char serr[AS_MAXCH] = "", obj[40];
  int32 rf = 0;
  int i, n = 8;
  memset(dret, 0, sizeof dret);
  strcpy(obj, "venus");
  switch (k) {
    case H_VISLIM:
      rf = swe_vis_limit_mag(T_OBS - 0.3, GEO, datm, dobs, obj, 0, dret, serr);
      break;
    case H_PHENO:   /* Venus's morning first, 2000 */
      rf = swe_heliacal_pheno_ut(2451545.0 - 7 * 30, GEO, datm, dobs, obj,
                                 SE_HELIACAL_RISING, 0, dret, serr);
      n = 30;
      break;
    case H_HELUT:
      rf = swe_heliacal_ut(T_OBS, GEO, datm, dobs, obj, SE_HELIACAL_RISING,
                           SEFLG_SWIEPH, dret, serr);
      n = 3;
      break;
    case H_ARCVIS:
      rf = swe_topo_arcus_visionis(T_OBS, GEO, datm, dobs, 0, -4.0, 250, 5,
                                   270, 0, -30, dret, serr);
      n = 1;
      break;
    case H_HELANG:
      rf = swe_heliacal_angle(T_OBS, GEO, datm, dobs, 0, -4.0, 250, 270, 0,
                              -30, dret, serr);
      n = 3;
      break;
  }
  put(o, rf);
  for (i = 0; i < n; i++)
    put(o, dret[i]);
  if (rf == ERR)
    snprintf(o->err, sizeof o->err, "%s", serr);
}

/* the orders ----------------------------------------------------------
 * "env" scenarios name the files by SE_EPHE_PATH instead of
 * swe_set_ephe_path(), so the action can be the very first library call. */
enum { M_NONE, M_NONEENV, M_WARM, M_WARMENV, M_FIRST, M_BARE, M_REOPEN, M_RENV };

static void warm(void)
{
  double x[6];
  char serr[AS_MAXCH];
  swe_calc_ut(T_WARM, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
}

/* one scenario, in this (forked, pristine) process */
static void scenario(int hel, int k, int mode, struct obs *o)
{
  int env = (mode == M_NONEENV || mode == M_WARMENV || mode == M_BARE || mode == M_RENV);
  if (env)
    setenv("SE_EPHE_PATH", EPHE, 1);
  else
    swe_set_ephe_path((char *) EPHE);
  if (mode == M_WARM || mode == M_WARMENV || mode == M_REOPEN || mode == M_RENV)
    warm();
  if (mode == M_REOPEN || mode == M_RENV) {
    swe_close();
    if (mode == M_REOPEN)
      swe_set_ephe_path((char *) EPHE);
  }
  if (hel) {
    do_hel(k, o);
    return;
  }
  if (mode != M_NONE && mode != M_NONEENV)
    do_set(k);
  observe_set(k, o);
}

static int run(int hel, int k, int mode, struct obs *o)
{
  int fd[2], st;
  pid_t pid;
  memset(o, 0, sizeof *o);
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    struct obs r;
    memset(&r, 0, sizeof r);
    close(fd[0]);
    scenario(hel, k, mode, &r);
    if (write(fd[1], &r, sizeof r) != (ssize_t) sizeof r)
      _exit(3);
    _exit(0);
  }
  close(fd[1]);
  st = (int) read(fd[0], o, sizeof *o);
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return st == (int) sizeof *o ? 0 : -1;
}

static double maxdiff(const struct obs *a, const struct obs *b)
{
  double d = 0;
  int i;
  if (a->n != b->n)
    return HUGE_VAL;
  for (i = 0; i < a->n; i++) {
    double e = fabs(a->v[i] - b->v[i]);
    if (e != e || e > d)
      d = (e != e) ? HUGE_VAL : e;
  }
  return d;
}

static const char *modename[] = { "", "", "", "", "first", "bare", "reopen", "renv" };

int main(int argc, char **argv)
{
  struct obs ref, refb, unset, got;
  int k, m, nfail = 0, nrun = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  for (k = 0; k < NSET; k++) {
    double eff;
    if (run(0, k, M_WARM, &ref) || run(0, k, M_WARMENV, &refb) || run(0, k, M_NONE, &unset)) {
      printf("  fork failed\n");
      return 2;
    }
    /* unset: the library's defaults, whichever way the files were named
     * (not the tidal acceleration: swe_set_ephe_path() reads it from the
     * Moon file, SE_EPHE_PATH waits for a calculation) */
    if (run(0, k, M_NONEENV, &got)) {
      printf("  fork failed\n");
      return 2;
    }
    if (k != S_TID)
      nrun++;
    if (k != S_TID && (maxdiff(&unset, &got) > TOL || strcmp(unset.err, got.err) != 0)) {
      printf("  %-24s unset  FAIL: after swe_set_ephe_path() off by %.6g from SE_EPHE_PATH %s\n",
             setname[k], maxdiff(&unset, &got), unset.err);
      nfail++;
    }
    eff = maxdiff(&ref, &unset);
    if (!(eff > EFFECT) || ref.err[0]) {
      printf("  %-24s no effect to see (%.3g) %s\n", setname[k], eff, ref.err);
      nfail++;
    }
    for (m = M_FIRST; m <= M_RENV; m++) {
      const struct obs *want = (m == M_BARE || m == M_RENV) ? &refb : &ref;
      double d;
      run(0, k, m, &got);
      nrun++;
      d = maxdiff(&got, want);
      if (d > TOL || strcmp(got.err, want->err) != 0) {
        printf("  %-24s %-6s FAIL: off by %.6g (effect %.3g) %s\n", setname[k], modename[m], d, eff, got.err);
        nfail++;
      }
    }
  }
  for (k = 0; k < NHEL; k++) {
    if (run(1, k, M_WARM, &ref)) {
      printf("  fork failed\n");
      return 2;
    }
    if (ref.err[0] || ref.v[0] < 0) {
      printf("  %-24s warm: rf %g %s\n", helname[k], ref.v[0], ref.err);
      nfail++;
    }
    for (m = M_FIRST; m <= M_RENV; m += 2) {   /* first, reopen */
      double d;
      run(1, k, m, &got);
      nrun++;
      d = maxdiff(&got, &ref);
      if (d > TOL || strcmp(got.err, ref.err) != 0) {
        printf("  %-24s %-6s FAIL: off by %.6g %s\n", helname[k], modename[m], d, got.err);
        nfail++;
      }
    }
  }
  if (nfail)
    printf("G82 FAIL: %d (of %d scenarios)\n", nfail, nrun);
  else
    printf("G82 PASS (%d scenarios)\n", nrun);
  return nfail != 0;
}
