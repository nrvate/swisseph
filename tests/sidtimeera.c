/* sidtimeera.c -- G53: the default sidereal time is the Earth's rotation
 * angle plus the precession in right ascension at every date, continuous
 * where it changes formula, and costs a bounded time at any date.
 *
 * The defects. SEMOD_SIDT_LONGTERM, the default, is the IERS Conventions
 * 2010 sidereal time between 1850 and 2050 and a long-term one outside.
 * That one was the mean Sun's hour angle plus the mean Earth's longitude
 * evaluated at TT for a UT instant, so it moved with delta t at the Sun's
 * mean motion, 0.041" a second, where sidereal time at a UT1 is ERA(UT1)
 * plus the equinox's accumulated precession in right ascension, which moves
 * with TT only at the precession rate: +37" at 1000, +328" at year 0,
 * +2379" at -3000 against the IERS 2010 expression. And the two were
 * spliced by constants fitted under an older delta t, so the default
 * stepped by -0.097" at 1850-01-01 and -1.908" at 2050-01-01 0h UT.
 *
 * The long-term sidereal time is now ERA(UT1) plus that precession,
 * integrated from the context's own precession matrices (swephlib.c,
 * sidt_pra()), tabulated to 400 centuries either side, integrated from the
 * table's end to 2000 centuries and continued past them at the rate there;
 * the splice offsets are the two formulas' difference at each boundary;
 * and the rotation angle is formed from the fraction of the day, not as
 * 1.0027 * (JD - J2000), whose rounding (0.3 mas at -999) differed where
 * the multiply-add fuses.
 *
 * The checks, Moshier, no files:
 *   (a) across 1850-01-01 and 2050-01-01 0h UT the change of swe_sidtime()
 *       over two seconds, less the sidereal rate times the interval, within
 *       0.001", under the default and four other model sets in turn on one
 *       context and the default again (a kept offset would show);
 *   (b) delta t + 100 s at a fixed UT moves swe_sidtime0() by less than
 *       0.001", at nine epochs from -50000 to +50000;
 *   (c) on the IAU 2006 precession the long-term formula reproduces the
 *       IERS 2010 expression, the IAU 2006 polynomial, within 0.2 mas over
 *       1500-2500 (this checks the integral and SIDT_PRA_NUTC together);
 *   (d) no step where the table ends, at -+400 centuries, within 0.01" (a
 *       JD there resolves 4.8 mas of rotation);
 *   (e) where the integral stops, at TT -+2000 centuries, no step in the
 *       precession part (sidereal time less the rotation angle) within
 *       0.5", and its rate over a century either side within 20"/cy;
 *   (f) at J -+1e8, 1e10, 1e12, 1e13, 1e20, 1e300, -+inf and NaN, each in
 *       a child under a 10 s alarm: a fresh context's first swe_sidtime()
 *       within 10 times one that builds the table plus 0.05 s, the next 50
 *       within 10 times a call in the table plus 0.1 ms each (CPU time),
 *       then swe_sidtime0() returning, and within |J| 1e12 the houses with
 *       speeds, swe_azalt(), swe_azalt_rev() and swe_gauquelin_sector();
 *       swe_sidtime() finite within |J| 1e12;
 *   (g) over 64 consecutive representable JDs at -999, -13000 and +12000
 *       sidereal time lies on the straight line through the first and the
 *       last within 0.01 mas.
 * Measured (-O0): (a) 0.000008", (b) 0.00014 .. 0.00015", (c) 0.077 mas,
 * (d) 0.000008", (e) 0.00004" and 2.7"/cy, (f) 15 of 15, the first call at
 * -+1e12 0.030 s against the table's 0.021, (g) 0.0033 mas. Before the fix
 * 25 checks fail: 10 of 12 in (a) (-1.908" at 2050), all of (b) (4.10")
 * and (c) (up to 18.8"). The fix without its 2000-century bound fails (f)
 * at 10 of 15 dates: 6 hung, -1e300 and -inf crashed, -+1e8 0.012 s a call;
 * with the rotation angle formed as before, (g) at all three (0.33, 1.03
 * and 0.78 mas). Run as
 *   sidtimeera
 */
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "swephexp.h"

#define J2000 2451545.0

static double wrap(double d)   /* hours, to (-12, 12] */
{
  while (d > 12) d -= 24;
  while (d <= -12) d += 24;
  return d;
}

static double wrapdeg(double d)   /* degrees, to (-180, 180] */
{
  d = fmod(d, 360);
  if (d > 180) d -= 360;
  if (d <= -180) d += 360;
  return d;
}

static double cpu(void)
{
  struct timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec + 1e-9 * t.tv_nsec;
}

/* (a) */
static int splices(void)
{
  static const double rgjd[2] = {2396758.5, 2469807.5};   /* 1850-01-01, 2050-01-01 0h UT */
  static const char *rgsz[2] = {"1850-01-01", "2050-01-01"};
  static const char *rgModels[6] = {"0,0,0,0,0,0,0,0", "0,0,0,4", "0,8,8", "4",
    "2", "0,0,0,0,0,0,0,0"};
  static const char *rgszModels[6] = {"default", "nutation IAU 2000B",
    "precession IAU 2006", "delta t Espenak-Meeus", "delta t Stephenson 1997",
    "default again"};
  const double rate = 24.0 * 1.00273781191135448;   /* sidereal hours per UT day */
  int m, i, bad = 0;
  swe_ctx *ctx = swe_ctx_new();
  for (m = 0; m < 6; m++) {
    char mod[40];
    snprintf(mod, sizeof(mod), "%s", rgModels[m]);
    swe_set_astro_models_r(ctx, mod, 0);
    for (i = 0; i < 2; i++) {
      double ta = rgjd[i] - 1.0 / 86400.0, tb = rgjd[i] + 1.0 / 86400.0;
      double d = wrap(swe_sidtime_r(ctx, tb) - swe_sidtime_r(ctx, ta));
      double step = (d - rate * (tb - ta)) * 54000;   /* arcsec */
      printf("  (a) %-24s %s  step %+.6f\"\n", rgszModels[m], rgsz[i], step);
      if (!(fabs(step) <= 1e-3)) {
        printf("      FAIL: sidereal time steps at the splice\n");
        bad++;
      }
    }
  }
  swe_ctx_free(ctx);
  return bad;
}

/* (b) */
static int deltat_free(void)
{
  static const double rgYear[9] = {-50000, -13000, -3000, 0, 1000, 1700, 2500,
                                   12000, 50000};
  int i, bad = 0;
  for (i = 0; i < 9; i++) {
    double jd = J2000 + (rgYear[i] - 2000) * 365.25, dt, s0, s1, d;
    swe_ctx *c = swe_ctx_new();
    dt = swe_deltat_ex_r(c, jd, -1, NULL);   /* as the library reads it */
    s0 = swe_sidtime0_r(c, jd, 23.4, 0);
    swe_set_delta_t_userdef_r(c, dt + 100.0 / 86400.0);
    s1 = swe_sidtime0_r(c, jd, 23.4, 0);
    d = wrap(s1 - s0) * 54000;
    printf("  (b) %7.0f  delta t + 100 s moves it %+.6f\"\n", rgYear[i], d);
    if (!(fabs(d) <= 1e-3)) {
      printf("      FAIL: sidereal time follows delta t\n");
      bad++;
    }
    swe_ctx_free(c);
  }
  return bad;
}

/* (c) */
static int iau2006(void)
{
  static const double rgYear[6] = {1500, 1700, 1840, 2060, 2300, 2500};
  int i, bad = 0;
  swe_ctx *ca = swe_ctx_new(), *cb = swe_ctx_new();
  char ma[] = "0,8,8,0,0,0,0,4", mb[] = "0,8,8,0,0,0,0,3";
  swe_set_astro_models_r(ca, ma, 0);   /* IAU 2006 precession, long term */
  swe_set_astro_models_r(cb, mb, 0);   /* IAU 2006 precession, IERS 2010 */
  for (i = 0; i < 6; i++) {
    double jd = J2000 + (rgYear[i] - 2000) * 365.25;
    double d = wrap(swe_sidtime0_r(ca, jd, 23.4, 0) - swe_sidtime0_r(cb, jd, 23.4, 0)) * 54000;
    printf("  (c) %7.0f  IAU 2006 precession: long term - IERS 2010 %+.6f\"\n", rgYear[i], d);
    if (!(fabs(d) <= 2e-4)) {
      printf("      FAIL: the integral does not reproduce the IAU 2006 polynomial\n");
      bad++;
    }
  }
  swe_ctx_free(ca);
  swe_ctx_free(cb);
  return bad;
}

/* (d): on the default precession, Vondrak 2011; the slope is taken from a
 * pair of seconds on each side, so the drift of the precession far from
 * J2000 does not count as a step */
static int table_end(void)
{
  int i, bad = 0;
  for (i = 0; i < 2; i++) {
    double jd = J2000 + (i ? 400 : -400) * 36525.0, t[4], v[4], step;
    swe_ctx *c = swe_ctx_new();
    int j;
    for (j = 0; j < 4; j++) {
      t[j] = jd + (2 * j - 3) / 86400.0;   /* -3, -1, +1, +3 s */
      v[j] = swe_sidtime_r(c, t[j]);
    }
    step = (wrap(v[2] - v[1]) - (wrap(v[1] - v[0]) / (t[1] - t[0])
            + wrap(v[3] - v[2]) / (t[3] - t[2])) / 2 * (t[2] - t[1])) * 54000;
    swe_ctx_free(c);
    printf("  (d) %s400 cy   step where the table ends %+.6f\"\n", i ? "+" : "-", step);
    if (!(fabs(step) <= 1e-2)) {
      printf("      FAIL: sidereal time steps at the end of the table\n");
      bad++;
    }
  }
  return bad;
}

/* (e): the precession part, degrees, at TT tc centuries: mean sidereal time
 * less the rotation angle, formed as the library forms it, at the UT whose
 * TT that is */
static double pra_at(swe_ctx *c, double tc)
{
  double tt = J2000 + tc * 36525.0, ut = tt, era;
  int i;
  for (i = 0; i < 8; i++)
    ut = tt - swe_deltat_ex_r(c, ut, -1, NULL);
  era = (ut - floor(ut)) + 0.7790572732640 + 0.00273781191135448 * (ut - J2000);
  era = (era - floor(era)) * 360;
  return swe_sidtime0_r(c, ut, 23.4, 0) * 15 - era;
}

static int integral_end(void)
{
  int i, bad = 0;
  for (i = 0; i < 2; i++) {
    double tf = i ? 2000 : -2000, eps = 1e-6, d, wl, wr;
    swe_ctx *c = swe_ctx_new();
    wl = wrapdeg(pra_at(c, tf) - pra_at(c, tf - 1)) * 3600;
    wr = wrapdeg(pra_at(c, tf + 1) - pra_at(c, tf)) * 3600;
    /* less the precession over the 2 eps */
    d = wrapdeg(pra_at(c, tf + eps) - pra_at(c, tf - eps)) * 3600 - (wl + wr) * eps;
    swe_ctx_free(c);
    printf("  (e) TT %+.0f cy: step %+.6f\", rate %.4f before, %.4f after \"/cy\n",
           tf, d, wl, wr);
    if (!(fabs(d) <= 0.5) || !(fabs(wr - wl) <= 20)) {
      printf("      FAIL: the precession is not continued continuously past the integral\n");
      bad++;
    }
  }
  return bad;
}

/* (g): sidereal time through 0 h over 64 ulps of the JD, nutation 0, so
 * the equation of the equinoxes adds nothing to bend it */
static int straight(void)
{
  static const double rgYear[3] = {-999, -13000, 12000};
  int i, bad = 0;
  swe_ctx *c = swe_ctx_new();
  for (i = 0; i < 3; i++) {
    double jd0 = floor(J2000 + (rgYear[i] - 2000) * 365.25) + 0.5, jd = jd0;
    double v[65], dev, worst = 0;
    int j;
    for (j = 0; j <= 64; j++) {
      v[j] = wrap(swe_sidtime0_r(c, jd, 23.4, 0) - swe_sidtime0_r(c, jd0, 23.4, 0));
      jd = nextafter(jd, 1e10);
    }
    for (j = 1; j < 64; j++) {
      dev = (v[j] - v[64] * j / 64) * 54000;
      if (fabs(dev) > worst) worst = fabs(dev);
    }
    printf("  (g) %7.0f  64 consecutive JDs: worst off the line %.6f mas\n", rgYear[i], worst * 1e3);
    if (!(worst <= 1e-5)) {
      printf("      FAIL: the rotation angle is rounded at the size of the day count\n");
      bad++;
    }
  }
  swe_ctx_free(c);
  return bad;
}

/* (f) */
static double ref_first, ref_warm;
static int bounded_at(double t)
{
  swe_ctx *c = swe_ctx_new();
  double t0, t1, t2, s, first, warm, x[6], cu[13], a[10], cs[13], as[10];
  double geo[3] = {10, 50, 100}, xin[3] = {100, 1, 1}, hin[2] = {100, 20};
  double sg = t < 0 ? 1 : -1;
  char serr[AS_MAXCH];
  int k, bad = 0;
  t0 = cpu();
  s = swe_sidtime_r(c, t);
  t1 = cpu();
  for (k = 1; k <= 50; k++)
    swe_sidtime_r(c, t + sg * k * 0.37);
  t2 = cpu();
  first = t1 - t0;
  warm = (t2 - t1) / 50;
  if (!(first < 10 * ref_first + 0.05) || !(warm < 10 * ref_warm + 1e-4)) {
    printf("  (f) J %g: first call %.4f s, then %.2e s a call: over the bound\n", t, first, warm);
    bad++;
  } else if (fabs(t) == 1e12)
    printf("  (f) J %g: first call %.4f s, then %.2e s a call\n", t, first, warm);
  if (fabs(t) <= 1e12 && !isfinite(s)) {
    printf("  (f) J %g: swe_sidtime() %g\n", t, s);
    bad++;
  }
  swe_sidtime0_r(c, t, 23.44, -0.004);
  /* The functions built on the sidereal time, within |J| 1e12, where it is
   * finite. Past that, and at inf and NaN, the houses still take any date
   * and cast a NaN to int in Asc1(): a range refusal planned for ts.21. */
  if (fabs(t) <= 1e12) {
    swe_houses_ex2_r(c, t, SEFLG_SPEED, 50, 10, 'P', cu, a, cs, as, serr);
    swe_azalt_r(c, t, SE_ECL2HOR, geo, 0, 10, xin, x);
    swe_azalt_rev_r(c, t, SE_HOR2ECL, geo, hin, x);
    swe_gauquelin_sector_r(c, t, SE_SUN, NULL, SEFLG_MOSEPH, 0, geo, 0, 10, x, serr);
  }
  swe_ctx_free(c);
  return bad;
}

static int bounded(void)
{
  static const double rgt[8] = {1e8, 1e10, 1e12, 1e13, 1e20, 1e300, INFINITY, NAN};
  double t0, t1, tr = J2000 + 399.5 * 36525.0;
  int i, k, bad = 0;
  swe_ctx *c = swe_ctx_new();
  t0 = cpu();
  swe_sidtime_r(c, tr);   /* builds the table to its end */
  t1 = cpu();
  ref_first = t1 - t0;
  for (k = 1; k <= 50; k++)
    swe_sidtime_r(c, tr - k * 0.37);
  ref_warm = (cpu() - t1) / 50;
  swe_ctx_free(c);
  printf("  (f) the table built in %.4f s, then %.2e s a call\n", ref_first, ref_warm);
  for (i = 0; i < 15; i++) {
    double t = i < 7 ? rgt[i] : i < 14 ? -rgt[i - 7] : rgt[7];
    pid_t pid;
    int st;
    fflush(stdout);
    if ((pid = fork()) == 0) {
      int r;
      alarm(10);
      r = bounded_at(t);
      fflush(stdout);
      _exit(r ? 1 : 0);
    }
    if (pid < 0 || waitpid(pid, &st, 0) != pid) {
      printf("  (f) J %g: fork failed\n", t);
      bad++;
    } else if (!WIFEXITED(st)) {
      printf("  (f) J %g: %s\n", t, WIFSIGNALED(st) && WTERMSIG(st) == SIGALRM
             ? "hung, killed by the 10 s alarm" : "crashed");
      bad++;
    } else if (WEXITSTATUS(st) != 0)
      bad++;
  }
  return bad;
}

int main(void)
{
  int bad = 0;
  printf("G53: the default sidereal time is rotation plus precession, continuous and bounded\n");
  bad += splices();
  bad += deltat_free();
  bad += iau2006();
  bad += table_end();
  bad += integral_end();
  bad += bounded();
  bad += straight();
  printf(bad ? "G53 FAIL: %d\n" : "G53 PASS\n", bad);
  return bad ? 1 : 0;
}
