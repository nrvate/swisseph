/* sidtrange.c -- G159: the long-term sidereal time costs a bounded time at
 * every date within |J| 1e12 days; past it and at no date the functions
 * without an error return answer NaN and the houses refuse.
 *
 * The defects. UPSTREAM-BUGS 209: past its table's 400 centuries the
 * long-term sidereal time (this fork's, bug 29; swephlib.c sidt_pra())
 * integrated the precession's rate in 10-century panels from the table's
 * end on every call, out to any date: swe_sidtime() took 0.02 s at J 1e8,
 * 1.1 s at 1e10 and some 140 s at 1e12, and swe_houses_ex2(), swe_azalt(),
 * swe_azalt_rev() and swe_gauquelin_sector() with it; past |J| 7.8e14 the
 * panel count's cast overflowed, and at -1e300 and -inf, TT NaN, (int)
 * floor(NaN) indexed the table at INT_MIN: SIGSEGV in all five. The houses
 * answered any date: Placidus at UT 1e20 ERR "within polar circle", its
 * cusps filled, NaN at 1e84, +1e300, +inf and NaN. UPSTREAM-BUGS 214:
 * swe_sidtime(), swe_sidtime0(), swe_azalt() and swe_azalt_rev() answered
 * NaN at 1e84, +1e300, +inf and NaN and a number past 1e12, where
 * SE_ECL_NUT refuses (bug 196); and swe_azalt() and swe_azalt_rev() read
 * SE_ECL_NUT's obliquity without its return, so from UT 8.13e11 (TT 1e12)
 * they turned the ecliptic by 0, the ecliptic place (100, 1) at UT 9e11
 * answered as the equatorial place (100, 1).
 *
 * Every case runs in a child process under an alarm, so a hang or a crash
 * is a failure of the gate, not of the run.
 *
 * Check (a): the cost. On a fresh context, the default models, the first
 * swe_sidtime() at 14 dates (|J| 1e8, 1e9, 1e10, 1e11, 8e11, 1e12 and the
 * double next inside 1e12, either sign) and 200 more about each, within
 * 10 times the first call at J2000 + 399.5 centuries (the table built) plus
 * 0.05 s, and 100 times a call about it plus 0.1 ms; then the houses with
 * speeds, swe_azalt(), swe_azalt_rev() and swe_gauquelin_sector() there,
 * under a 5 s alarm each date.
 *
 * Check (b): answered within the bound. 44 dates (+-1e12, the doubles next
 * inside, |J| log-uniform over 1e8..1e12 and UT 3e7 and 7.3e7, TT 754 and
 * 1931 centuries, the integral's, either side) under the default models,
 * the eleven long-term precession models with the long-term sidereal time,
 * and the default precession with IAU 1976, IAU 2006 and IERS 2010
 * sidereal time: swe_sidtime() and swe_sidtime0() finite (that they lie
 * in [0, 24) is check (e)'s, UPSTREAM-BUGS 234); Placidus
 * with speeds not refused, its cusps, ascmc and speeds finite;
 * swe_azalt() SE_EQU2HOR and swe_azalt_rev() SE_HOR2EQU finite; their
 * ecliptic forms finite where TT is within the bound and all NaN where it
 * is past (SE_ECL_NUT refuses there). Where TT is past the bound or, under
 * a polynomial precession model, past the model's range (UPSTREAM-BUGS
 * 213, 217: every date here under the ten, |J| 3e7 and more; under the
 * other five sets UT 1e12 and the double next inside, TT 1.28e12), the
 * houses refuse, ERR with serr naming the range and the cusps 0, and the
 * ecliptic forms are NaN. 660 dates by models, 450 of them refusing.
 *
 * Check (c): past the bound and at no date. 21 dates (|J| the double past
 * 1e12, 1.5e12, 1e13, 1e20, 1e84, 1e200, 1e300, 1e308, DBL_MAX and inf,
 * either sign, and NaN): swe_sidtime() and swe_sidtime0() NaN, swe_azalt()
 * and swe_azalt_rev() NaN in every output with either flag;
 * swe_houses_ex2() (Placidus with speeds, Gauquelin sidereal, Koch in
 * radians), swe_houses_ex() and swe_houses() ERR with every cusp and ascmc
 * and speed 0, serr naming the range; swe_gauquelin_sector() returning.
 * 13 checks a date.
 *
 * Check (d): a call's answer is its date's alone. swe_sidtime() at 20
 * dates (TT within the table, past it, past 2000 centuries, either side,
 * UT 1e9 to 1e12) under the default models and IAU 2006 long-term, on a
 * fresh context in order, against a context that first took 1e12, -1e12,
 * 1e13, NaN, -1e300 and -inf and then the dates in reverse, and one that
 * took them with the other model set between each: bit for bit.
 *
 * UPSTREAM-BUGS 234: the sidereal times were their formulas evaluated in
 * double and then reduced, IAU 2006's and IAU 1976's seconds by 86400
 * floor(): from |J| 2e10 IAU 2006's left [0, 24) (1165 h at J -6.3e10),
 * a sum just under 0 came back 24 h under every model, and each was off
 * its formula by the rounding of its largest term, 180 deg for IAU 2006
 * and IERS 2010 at 1e12; the long-term one's line past 2000 centuries by
 * 1.3e-4 deg over consecutive JDs.
 *
 * Check (e): in range. Under the default models, IAU 1976, IAU 2006 and
 * IERS 2010 sidereal time, at 202 dates (|J| log-uniform over 1e2..1e12,
 * the doubles next inside 1e12, either side; a delta t per date, so the
 * houses' agree), swe_sidtime() and swe_sidtime0() in [0, 24) and the
 * Placidus ARMC in [0, 360) and the sidereal time's, bit for bit (the
 * houses refused where delta t takes TT past 1e12, bug 213: UT next
 * inside +1e12); and at
 * five dates the 401 nutations about the one that makes swe_sidtime0()'s
 * seconds 0 (some leave them just under) answer in [0, 24). 2207 a model.
 *
 * Check (f): the formulas. IAU 1976, IAU 2006 and IERS 2010 at 36 dates
 * (|J| log-uniform over 1e3..1e12, delta t growing as T^2) against mpmath
 * at 400 bits, from the same doubles: within 1e-12 deg past 5e5 days of
 * J2000, 2e-10 deg within (the double forms, unchanged there).
 *
 * Check (g): the default sidereal time over 64 consecutive JDs at J2000
 * -+ 5.1e5, 1e6, 1e7, 7.3e7, 1e8, 1e9, 1e10, 1e11, 5e11 and 1e12 days
 * (the last just inside) lies on the line through the first and last
 * within 1e-11 deg (delta t 0.3 day, its own rounding kept out). At -+7.3e7
 * days (2000 centuries) it needs bug 235 too: the integral's rate.
 *
 * Measured (the -O0 test build): (a) 14 of 14 bounded, the first call at
 * 1e12 0.0050 s against the table's 0.0029 (0.031 and 0.022 before bug
 * 235's analytic rate), a call there 1.1e-4 s against 2.8e-5 (the
 * nutation's arguments in double-double past 4e8 days, bug 188);
 * (b) 660 of 660 as expected; (c) 273 of 273 refused or NaN; (d) 0 differ;
 * (e) 0 of 8828; (f) 0 of 108, worst 1.3e-13 deg past 5e5 days, 1.2e-11
 * within; (g) 0 of 20, worst 2.5e-12 deg. Before bug 234's fix: (e) 26 of
 * 8828, (f) 75 of 108 (IAU 1976 off by 0.049 deg, IAU 2006 by 163, IERS
 * 2010 by 160), (g) 20 of 20, worst 1.3e-4 deg; bug 234 fixed but not 235,
 * (g) fails at -+7.3e7 days, 7.0e-9 and 5.0e-9 deg. Before bug 209's fix
 * (-O2): (a) 0 of 14, 12 hung and +-1e8 at 9.5e-3 s a call;
 * (b) 126 of 660, the twelve long-term sidereal time contexts hung and the
 * ecliptic forms turned by 0 at UT 1e12 under the other three; (c) 50 of
 * 273, 6 dates hung (+-1e12 next, +-1.5e12, +-1e13), 5 crashed (-1e200,
 * -1e300, -1e308, -DBL_MAX, -inf), the rest answered; (d) hung. Before
 * bugs 213 and 217, (b) 210 of 660, the houses and ecliptic forms answered
 * past the models' ranges and the houses at TT past 1e12. The upstream
 * build fails 755: (b) 450 of 660 (210 as expected), the same and the
 * ecliptic forms answered where TT is past 1e12 (its SE_ECL_NUT answers
 * there); (c) 187 of 273 (86 refused or NaN, the NaN of 1e84 and beyond
 * its only no-answers); (e) 19 of 8828; (f) 83 of 108 (IERS 2010 4.4e-8
 * deg off within 5e5 days, its terms in plain double); (g) 16 of 20,
 * worst 2.8e-8 deg; (a) and (d) pass. Run as
 *   sidtrange EPHEDIR
 */
#include <float.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "swephexp.h"

static const char *EPHE;
static const char *MOD[15] = {
  "0", "0,1", "0,2", "0,3", "0,4", "0,5", "0,6", "0,7", "0,8", "0,9", "0,10", "0,11",
  "0,0,0,0,0,0,0,1", "0,0,0,0,0,0,0,2", "0,0,0,0,0,0,0,3" };

static swe_ctx *fresh(const char *models)
{
  char mod[40];
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, EPHE);
  strcpy(mod, models);
  swe_set_astro_models_r(c, mod, 0);
  return c;
}

static double cpu(void)
{
  struct timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec + 1e-9 * t.tv_nsec;
}

/* fn(arg) in a child under an alarm of secs: its return (failures, at most
 * 100), or -1 if it hung or crashed */
static int in_child(const char *what, unsigned secs, int (*fn)(double), double arg)
{
  pid_t pid;
  int st;
  fflush(stdout);
  if ((pid = fork()) == 0) {
    int r;
    alarm(secs);
    r = fn(arg);
    fflush(stdout);
    _exit(r > 100 ? 100 : r);
  }
  if (pid < 0 || waitpid(pid, &st, 0) != pid) {
    printf("  %s: fork failed\n", what);
    return -1;
  }
  if (WIFEXITED(st))
    return WEXITSTATUS(st);
  printf("  %s, %g: %s\n", what, arg,
         WIFSIGNALED(st) && WTERMSIG(st) == SIGALRM ? "hung, killed by the alarm" : "crashed");
  return -1;
}

/* (a) */
static double ref_first, ref_warm;
static int cost_at(double t)
{
  swe_ctx *c = fresh("0");
  double t0, t1, t2, s = 0, first, warm, x[6], cu[13], a[10], cs[13], as[10];
  double geo[3] = {10, 50, 100}, xin[3] = {100, 1, 1}, hin[2] = {100, 20}, sg = t < 0 ? 1 : -1;
  char serr[AS_MAXCH];
  int k, bad = 0;
  t0 = cpu();
  s += swe_sidtime_r(c, t);
  t1 = cpu();
  for (k = 1; k <= 200; k++)
    s += swe_sidtime_r(c, t + sg * k * 0.37);
  t2 = cpu();
  first = t1 - t0;
  warm = (t2 - t1) / 200;
  if (!(first < 10 * ref_first + 0.05) || !(warm < 100 * ref_warm + 1e-4)) {
    printf("  (a) J %g: first call %.4f s, then %.2e s a call: over the bound\n", t, first, warm);
    bad++;
  } else if (fabs(t) == 1e12)
    printf("  (a) J %g: first call %.4f s, then %.2e s a call\n", t, first, warm);
  swe_houses_ex2_r(c, t, SEFLG_SPEED, 50, 10, 'P', cu, a, cs, as, serr);
  swe_azalt_r(c, t, SE_ECL2HOR, geo, 0, 10, xin, x);
  swe_azalt_rev_r(c, t, SE_HOR2ECL, geo, hin, x);
  swe_gauquelin_sector_r(c, t, SE_SUN, NULL, SEFLG_MOSEPH, 0, geo, 0, 10, x, serr);
  swe_ctx_free(c);
  (void) s;
  return bad;
}

/* (b) */
static double TB[44];
static int fin(const double *x, int n)
{
  int i;
  for (i = 0; i < n; i++)
    if (!isfinite(x[i]))
      return 0;
  return 1;
}
static int allnan(const double *x, int n)
{
  int i;
  for (i = 0; i < n; i++)
    if (!isnan(x[i]))
      return 0;
  return 1;
}
static int answered_under(double m)
{
  swe_ctx *c = fresh(MOD[(int) m]);
  double geo[3] = {10, 50, 100}, xin[3] = {100, 1, 1}, hin[2] = {100, 20};
  char serr[AS_MAXCH];
  int i, bad = 0;
  for (i = 0; i < 44; i++) {
    double t = TB[i], s, s0, cu[13], a[10], cs[13], as[10], x[6];
    double tt = t + swe_deltat_ex_r(c, t, -1, NULL);
    /* TT past the bound, or past a polynomial precession model's range
     * (UPSTREAM-BUGS 217: Owen 1990's pieces, the others the widest body
     * range), where SE_ECL_NUT and the houses refuse */
    int poly = m >= 1 && m <= 11 && m != 9;
    double lo = m == 10 ? -4853455.5 : -3100015.5, hi = m == 10 ? 9756544.5 : 8000016.5;
    int r, past = !(tt >= -1e12 && tt <= 1e12) || (poly && !(tt >= lo && tt <= hi)), ok = 1;
    s = swe_sidtime_r(c, t);
    s0 = swe_sidtime0_r(c, t, 23.44, -0.004);
    if (!isfinite(s) || !isfinite(s0))
      ok = 0;
    serr[0] = '\0';
    r = swe_houses_ex2_r(c, t, SEFLG_SPEED, 50, 10, 'P', cu, a, cs, as, serr);
    if (past ? !(r == ERR && strstr(serr, "outside the") != NULL && cu[1] == 0 && as[0] == 0)
        : ((r < 0 && strstr(serr, "outside the") != NULL)
           || !fin(cu + 1, 12) || !fin(a, 8) || !fin(cs + 1, 12) || !fin(as, 8)))
      ok = 0;
    swe_azalt_r(c, t, SE_EQU2HOR, geo, 0, 10, xin, x);
    if (!fin(x, 3)) ok = 0;
    swe_azalt_rev_r(c, t, SE_HOR2EQU, geo, hin, x);
    if (!fin(x, 2)) ok = 0;
    swe_azalt_r(c, t, SE_ECL2HOR, geo, 0, 10, xin, x);
    if (past ? !allnan(x, 3) : !fin(x, 3)) ok = 0;
    swe_azalt_rev_r(c, t, SE_HOR2ECL, geo, hin, x);
    if (past ? !allnan(x, 2) : !fin(x, 2)) ok = 0;
    if (!ok) {
      if (bad < 3)
        printf("  (b) models %s J %.17g: sidtime %g %g, houses %d asc %g asc' %g, azalt %g\n",
               MOD[(int) m], t, s, s0, r, a[0], as[0], x[0]);
      bad++;
    }
  }
  swe_ctx_free(c);
  return bad;
}

/* (c) */
static int refused_h(int r, const double *cu, int nc, const double *a, const double *cs,
                     const double *as, const char *serr)
{
  int i;
  if (r != ERR || (serr != NULL && strstr(serr, "outside the obliquity and nutation range") == NULL))
    return 0;
  for (i = 0; i <= nc; i++)
    if (cu[i] != 0 || (cs != NULL && cs[i] != 0))
      return 0;
  for (i = 0; i < 10; i++)
    if (a[i] != 0 || (as != NULL && as[i] != 0))
      return 0;
  return 1;
}
static double cu[37], a[10], cs[37], as[10], x[6];
static char serr[AS_MAXCH];
static void FILL(void)
{
  int i;
  for (i = 0; i < 37; i++)
    cu[i] = cs[i] = 7;
  for (i = 0; i < 10; i++)
    a[i] = as[i] = 7;
  for (i = 0; i < 6; i++)
    x[i] = 7;
  serr[0] = '\0';
}
static int refused_at(double t)
{
  swe_ctx *c = fresh("0");
  double geo[3] = {10, 50, 100}, xin[3] = {100, 1, 1}, hin[2] = {100, 20};
  int bad = 0, n = 0;
#define CHECK(cond, what) do { n++; if (!(cond)) { bad++; if (bad <= 3) printf("  (c) J %g: %s answered\n", t, what); } } while (0)
  FILL(); CHECK(isnan(swe_sidtime_r(c, t)), "swe_sidtime()");
  FILL(); CHECK(isnan(swe_sidtime0_r(c, t, 23.44, -0.004)), "swe_sidtime0()");
  FILL(); swe_azalt_r(c, t, SE_ECL2HOR, geo, 0, 10, xin, x); CHECK(allnan(x, 3), "swe_azalt() SE_ECL2HOR");
  FILL(); swe_azalt_r(c, t, SE_EQU2HOR, geo, 0, 10, xin, x); CHECK(allnan(x, 3), "swe_azalt() SE_EQU2HOR");
  FILL(); swe_azalt_rev_r(c, t, SE_HOR2ECL, geo, hin, x); CHECK(allnan(x, 2), "swe_azalt_rev() SE_HOR2ECL");
  FILL(); swe_azalt_rev_r(c, t, SE_HOR2EQU, geo, hin, x); CHECK(allnan(x, 2), "swe_azalt_rev() SE_HOR2EQU");
  FILL(); CHECK(refused_h(swe_houses_ex2_r(c, t, SEFLG_SPEED, 50, 10, 'P', cu, a, cs, as, serr), cu, 12, a, cs, as, serr),
              "swe_houses_ex2() Placidus with speeds");
  FILL(); CHECK(refused_h(swe_houses_ex2_r(c, t, SEFLG_SIDEREAL, 50, 10, 'G', cu, a, NULL, NULL, serr), cu, 36, a, NULL, NULL, serr),
              "swe_houses_ex2() Gauquelin sidereal");
  FILL(); CHECK(refused_h(swe_houses_ex2_r(c, t, SEFLG_RADIANS, -30, 10, 'K', cu, a, cs, NULL, serr), cu, 12, a, cs, NULL, serr),
              "swe_houses_ex2() Koch radians");
  FILL(); CHECK(refused_h(swe_houses_ex2_r(c, t, 0, 50, 10, 'I', cu, a, NULL, as, NULL), cu, 12, a, NULL, as, NULL),
              "swe_houses_ex2() Sunshine, no serr");
  FILL(); CHECK(refused_h(swe_houses_ex_r(c, t, 0, 50, 10, 'P', cu, a), cu, 12, a, NULL, NULL, NULL), "swe_houses_ex()");
  FILL(); CHECK(refused_h(swe_houses_r(c, t, 50, 10, 'E', cu, a), cu, 12, a, NULL, NULL, NULL), "swe_houses()");
  FILL(); swe_gauquelin_sector_r(c, t, SE_SUN, NULL, SEFLG_MOSEPH, 0, geo, 0, 10, x, serr); CHECK(1, "");
#undef CHECK
  swe_ctx_free(c);
  return bad;
}

/* (d) */
static const double TD[20] = {
  2451545.3, 2451545 + 100 * 36525.0 + 0.25, 2451545 - 399.9 * 36525.0, 2451545 + 400.1 * 36525.0,
  2451545 - 1000 * 36525.0, 2451545 + 1999.9 * 36525.0, 2451545 - 2000.1 * 36525.0,
  2451545 + 2000.1 * 36525.0, 2451545 - 2500 * 36525.0, 2451545 + 1e5 * 36525.0,
  1e9, -1e9, 1e10, -1e10, 1e11, -1e11, 8e11, -8e11, 1e12, -1e12 };
static int order_free(double unused)
{
  static const char *MD[2] = { "0,0,0,0,0,0,0,0", "0,8,0,0,0,0,0,0" };
  static const double dis[6] = { 1e12, -1e12, 1e13, NAN, -1e300, -INFINITY };
  int m, i, k, bad = 0;
  (void) unused;
  for (m = 0; m < 2; m++) {
    swe_ctx *a = fresh(MD[m]), *b = fresh(MD[m]), *c = fresh(MD[m]);
    double va[20], vb[20], vc[20];
    char mo[40];
    for (i = 0; i < 20; i++)
      va[i] = swe_sidtime_r(a, TD[i]);
    for (k = 0; k < 6; k++)
      swe_sidtime_r(b, dis[k]);
    for (i = 19; i >= 0; i--)
      vb[i] = swe_sidtime_r(b, TD[i]);
    for (i = 0; i < 20; i++) {
      strcpy(mo, MD[!m]);
      swe_set_astro_models_r(c, mo, 0);
      swe_sidtime_r(c, TD[19 - i]);
      strcpy(mo, MD[m]);
      swe_set_astro_models_r(c, mo, 0);
      vc[i] = swe_sidtime_r(c, TD[i]);
    }
    for (i = 0; i < 20; i++)
      if (memcmp(&va[i], &vb[i], sizeof(double)) != 0 || memcmp(&va[i], &vc[i], sizeof(double)) != 0) {
        if (bad < 3)
          printf("  (d) models %s J %.17g: %.17g, %.17g after other dates, %.17g between models\n",
                 MD[m], TD[i], va[i], vb[i], vc[i]);
        bad++;
      }
    swe_ctx_free(a);
    swe_ctx_free(b);
    swe_ctx_free(c);
  }
  return bad;
}

/* (e), (f), (g): bug 234 */
static const char *MS[4] = { "0", "0,0,0,0,0,0,0,1", "0,0,0,0,0,0,0,2", "0,0,0,0,0,0,0,3" };
static double wrap_deg(double h)	/* hours to (-180, 180] degrees */
{
  h = fmod(h, 24);
  if (h > 12) h -= 24;
  if (h <= -12) h += 24;
  return h * 15;
}
static int in_range_under(double m)
{
  static const double te[5] = { 2451544.5, 2451545.3, 2451545 - 1.1e6, 2451545 + 3.3e8, -9e11 };
  swe_ctx *c = fresh(MS[(int) m]);
  char serr[AS_MAXCH];
  int i, s, j, bad = 0;
  for (i = 0; i <= 100; i++)
    for (s = 0; s < 2; s++) {
      double d = pow(10, 2 + 10.0 * i / 100) * (1 + 1e-3 * (i % 7)), t = s ? 2451545 - d : 2451545 + d;
      double v, v0, cu[13], a[10], cs[13], as[10];
      if (i == 100)
        t = nextafter(s ? -1e12 : 1e12, 0);
      /* delta t as (f)'s, so the houses take the sidereal time's (theirs
       * follows the ephemeris's tidal acceleration, 1.8e-9 deg apart in
       * 1134 on the Swiss files) */
      double dt = 3.7e-4 * (d / 36525) * (d / 36525) + 7.5e-4;
      int32 rh;
      swe_set_delta_t_userdef_r(c, dt);
      v = swe_sidtime_r(c, t);
      v0 = swe_sidtime0_r(c, t, 23.44, -0.004);
      rh = swe_houses_ex2_r(c, t, SEFLG_SPEED, 50, 10, 'P', cu, a, cs, as, serr);
      /* the houses refuse a TT past 1e12 (bug 213), where delta t takes
       * the double next inside UT 1e12 */
      if (!(v >= 0 && v < 24) || !(v0 >= 0 && v0 < 24)
          || (!(fabs(t + dt) <= 1e12) ? rh != ERR
              : !(a[2] >= 0 && a[2] < 360) || a[2] != swe_degnorm(v * 15 + 10))) {
        if (bad < 3)
          printf("  (e) models %s J %.17g: sidtime %.17g, sidtime0 %.17g, armc %.17g\n",
                 MS[(int) m], t, v, v0, a[2]);
        bad++;
      }
    }
  /* the day's edge: the nutation that makes the sidereal seconds 0, and
   * the 400 doubles about it, some of which leave them just under 0 */
  for (i = 0; i < 5; i++) {
    double nut = -swe_sidtime0_r(c, te[i], 0, 0) * 15;
    for (j = -200; j <= 200; j++) {
      double v = swe_sidtime0_r(c, te[i], 0, nut + j * 1e-16 * fabs(nut));
      if (!(v >= 0 && v < 24)) {
        if (bad < 3)
          printf("  (e) models %s J %.17g, nut %.17g: sidtime0 %.17g\n",
                 MS[(int) m], te[i], nut + j * 1e-16 * fabs(nut), v);
        bad++;
      }
    }
  }
  swe_ctx_free(c);
  return bad;
}
/* the formulas in mpmath at 400 bits at each row's J and delta t (exact
 * doubles; eps 23.44, nut -0.004) */
static const double TF[36][5] = {	/* J, delta t, IAU 1976, IAU 2006, IERS 2010 (h) */
  {2452545.0448714285, 0.0007542773704868509, 13.486817114306499, 13.486817244792439, 13.486817292082183},
  {2449563.776564059, 0.0007550886497773735, 3.148823275525392, 3.1488238227155296, 3.1488237867246096},
  {2454947.4781376207, 0.000757210790442622, 13.748674032428758, 13.748673826232258, 13.7486737851781},
  {2444827.6088088104, 0.0007665147608732642, 23.90994649369092, 23.909947700362537, 23.90994774160451},
  {2463103.1250378154, 0.0007910506687911479, 13.180406906322151, 13.180405551321067, 13.180405513602969},
  {2431708.966662025, 0.0008631266777897585, 10.47475831813687, 10.47476133519097, 10.474761383714357},
  {2490751.12279821, 0.001180313481292548, 5.87176195790539, 5.871756641734676, 5.871756618759382},
  {2384127.0731280055, 0.0020145847884939076, 6.432152694837842, 6.4321620651622835, 6.432162113171709},
  {2567170.328016523, 0.004461893319168865, 16.28978792969733, 16.289771111824166, 16.28977115467265},
  {2222746.8080284903, 0.015272657201528398, 3.801807058590721, 3.8018344897696545, 3.8018344721146424},
  {2844731.630490248, 0.04363044395468341, 22.05634412425263, 22.056273327382815, 22.056273415572036},
  {1777682.1934540472, 0.12669416201818812, 23.942140109261718, 23.942130405632945, 23.94212995621035},
  {3786557.267386171, 0.49505533993076956, 4.570551806474194, 4.569288975955419, 4.569292559020764},
  {158832.37038242677, 1.458629781774315, 21.944300953085786, 21.935907810365933, 21.935889577285813},
  {6978023.753550404, 5.683290674296099, 15.300391557230425, 15.150703440406568, 15.150848924168084},
  {-5336893.398089905, 16.824474838098112, 9.415975715905953, 8.550602355713496, 8.549819364410276},
  {15818278.168284727, 49.55396847319415, 23.8047635915962, 9.446124699517245, 9.450991187545316},
  {-23967939.08436634, 193.58496818959063, 3.1948064618644483, 9.645578821172858, 9.582252450269502},
  {47881881.83113194, 572.4186307589268, 6.797546955223402, 12.544192247965187, 13.245116373975495},
  {-75463931.9440891, 1683.7163773532156, 22.13585622568084, 3.9974245433020963, 19.18476710854036},
  {156629965.10051456, 6592.778830681806, 5.309181725854571, 2.639683713624714, 17.553557833229675},
  {-262502235.8853736, 19469.80196711952, 13.32394651002524, 10.58457941272005, 18.61378473650548},
  {456542506.23186314, 57188.27476718921, 13.551404470336553, 16.75076399079952, 9.346252813567194},
  {-897163357.9342428, 224457.72609855334, 16.96447592522475, 14.016691405903416, 19.19680077907861},
  {1547424985.718487, 662008.112201537, 21.2818564195895, 15.10339546861106, 20.657830364328177},
  {-3047773574.2779217, 2580388.2849349543, 17.345629220442614, 15.214431056641587, 3.453899177096367},
  {5250789773.442972, 7639499.477592344, 8.382558754873674, 14.09425839901972, 4.05402308330946},
  {-9004891522.222645, 22501666.48486971, 18.688632809677504, 3.1205585075599567, 19.47584436762798},
  {17805556776.095142, 87904840.98133594, 4.4099266882926065, 11.974776920298575, 13.288256845485993},
  {-30611358536.581867, 259929781.75741455, 9.690271860565304, 21.81920083859579, 6.933957035234287},
  {52506786368.41178, 764560041.2218837, 14.03716384314011, 18.234068273819123, 12.834314015153065},
  {-103892637466.19429, 2993720931.4046793, 23.955501094422218, 20.892687697716863, 4.417523809117686},
  {178544926099.3294, 8841060739.262701, 20.837945283717346, 5.238273846275351, 9.070348381136663},
  {-305992517175.8991, 25968678294.776203, 10.123515669010805, 21.24724528904027, 21.249763902836108},
  {606219393992.0951, 101924224186.81528, 0.33479706417403793, 10.273321525053147, 3.4433807409459987},
  {-999995096909.4, 277344236657.1091, 14.943426808735435, 6.930808171502779, 10.664096849742753},
};
static int formula_under(double m)
{
  swe_ctx *c = fresh(MS[(int) m]);
  double worst[2] = {0, 0};	/* within 5e5 days, past */
  int i, bad = 0;
  for (i = 0; i < 36; i++) {
    int far = fabs(TF[i][0] - 2451545) >= 5e5;
    double v, e;
    swe_set_delta_t_userdef_r(c, TF[i][1]);
    v = swe_sidtime0_r(c, TF[i][0], 23.44, -0.004);
    e = fabs(wrap_deg(v - TF[i][1 + (int) m]));
    if (!(e <= worst[far]))
      worst[far] = e;
    if (!(e <= (far ? 1e-12 : 2e-10))) {
      if (bad < 3)
        printf("  (f) models %s J %.17g: %.17g h, the formula's %.17g, off %.2e deg\n",
               MS[(int) m], TF[i][0], v, TF[i][1 + (int) m], e);
      bad++;
    }
  }
  swe_ctx_free(c);
  printf("  (f) models %s: worst %.2e deg within 5e5 days of J2000, %.2e past\n", MS[(int) m],
         worst[0], worst[1]);
  return bad;
}
static double worst_g;
static int line_at(double d)
{
  swe_ctx *c = fresh("0");
  double jd = 2451545 + d, v[65], a0, w = 0;
  int j;
  swe_set_delta_t_userdef_r(c, 0.3);
  if (jd > 1e12 - 1) jd = 1e12 - 1;
  if (jd < -1e12 + 1) jd = -1e12 + 1;
  a0 = swe_sidtime0_r(c, jd, 23.4, 0);
  for (j = 0; j <= 64; j++) {
    v[j] = wrap_deg(swe_sidtime0_r(c, jd, 23.4, 0) - a0);
    jd = nextafter(jd, 1e13);
  }
  for (j = 1; j < 64; j++)
    if (fabs(v[j] - v[64] * j / 64) > w)
      w = fabs(v[j] - v[64] * j / 64);
  swe_ctx_free(c);
  if (w > worst_g)
    worst_g = w;
  if (!(w <= 1e-11)) {
    printf("  (g) J2000 %+g days: %.2e deg off the line\n", d, w);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  static const double ja[7] = { 1e8, 1e9, 1e10, 1e11, 8e11, 1e12, 0 };
  static const double jc[10] = { 0, 1.5e12, 1e13, 1e20, 1e84, 1e200, 1e300, 1e308, DBL_MAX, INFINITY };
  static const double jg[10] = { 5.1e5, 1e6, 1e7, 7.3e7, 1e8, 1e9, 1e10, 1e11, 5e11, 1e12 };
  int i, m, k, ba = 0, bb = 0, bc = 0, bd, be = 0, bf = 0, bg = 0, nb = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: sidtrange EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  setvbuf(stdout, NULL, _IOLBF, 0);
  printf("G159: the sidereal time's cost bounded within |J| 1e12 days, NaN or refused past it\n");

  /* (a), the reference in this process (it is the fixed table's cost) */
  {
    swe_ctx *c = fresh("0");
    double t0, t1, t2, s = 0, t = 2451545 + 399.5 * 36525;
    t0 = cpu();
    s += swe_sidtime_r(c, t);
    t1 = cpu();
    for (k = 1; k <= 200; k++)
      s += swe_sidtime_r(c, t + k * 0.37);
    t2 = cpu();
    swe_ctx_free(c);
    ref_first = t1 - t0;
    ref_warm = (t2 - t1) / 200;
    printf("  (a) the table, J2000 + 399.5 centuries: first call %.4f s, then %.2e s a call (%g)\n",
           ref_first, ref_warm, s);
  }
  for (i = 0; i < 7; i++)
    for (k = 0; k < 2; k++) {
      double t = i == 6 ? nextafter(1e12, 0) : ja[i];
      ba += in_child("(a) the cost at J", 5, cost_at, k ? -t : t) != 0;
    }
  printf("  (a) %d of 14 dates bounded\n", 14 - ba);

  /* (b) */
  for (i = 0; i < 20; i++) {
    TB[2 * i] = pow(10, 8 + 4 * (i + 0.5) / 20);
    TB[2 * i + 1] = -TB[2 * i];
  }
  TB[40] = 1e12; TB[41] = -1e12; TB[42] = nextafter(1e12, 0); TB[43] = -TB[42];
  TB[0] = 3e7; TB[1] = 2451545 - (3e7 - 2451545); TB[2] = 7.3e7; TB[3] = 2451545 - (7.3e7 - 2451545);
  for (m = 0; m < 15; m++) {
    int r = in_child("(b) answered, the models of MOD[]", 5, answered_under, m);
    bb += r < 0 ? 44 : r;
    nb += 44;
  }
  printf("  (b) %d of %d dates by models as expected (answered; the houses and the ecliptic"
         " forms refused where TT is past the bound or the model's range)\n", nb - bb, nb);

  /* (c) */
  for (i = 0; i < 10; i++)
    for (k = 0; k < 2; k++) {
      double t = i == 0 ? nextafter(1e12, INFINITY) : jc[i];
      int r = in_child("(c) past the bound, J", 5, refused_at, k ? -t : t);
      bc += r < 0 ? 13 : r;
    }
  {
    int r = in_child("(c) past the bound, J", 5, refused_at, NAN);
    bc += r < 0 ? 13 : r;
  }
  printf("  (c) %d of 273 refused or NaN\n", 273 - bc);

  /* (d) */
  bd = in_child("(d) the order of the calls", 10, order_free, 0);
  if (bd < 0)
    bd = 40;
  printf("  (d) %d values differ\n", bd);

  /* (e) */
  for (m = 0; m < 4; m++) {
    int r = in_child("(e) in range, the models of MS[]", 10, in_range_under, m);
    be += r < 0 ? 2207 : r;
  }
  printf("  (e) %d of 8828 sidereal times and ARMCs out of range or apart\n", be);

  /* (f) */
  for (m = 1; m < 4; m++) {
    int r = in_child("(f) the formulas, the models of MS[]", 10, formula_under, m);
    bf += r < 0 ? 36 : r;
  }
  printf("  (f) %d of 108 off their formulas\n", bf);

  /* (g) */
  for (i = 0; i < 10; i++)
    for (k = 0; k < 2; k++)
      bg += line_at(k ? -jg[i] : jg[i]);
  printf("  (g) %d of 20 off the line, worst %.2e deg\n", bg, worst_g);

  if (ba + bb + bc + bd + be + bf + bg)
    printf("G159 FAIL: %d\n", ba + bb + bc + bd + be + bf + bg);
  else
    printf("G159 PASS\n");
  return ba + bb + bc + bd + be + bf + bg != 0;
}
