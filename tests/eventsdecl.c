/* eventsdecl.c -- G55: swevents names the ephemeris it reads, keeps its
 * header inside cmdline, and finds every zero declination to the second.
 *
 * The defects (UPSTREAM-BUGS 46), in upstream's swevents of 2026-10-19
 * (8a36c49), and two in a first form of the fix (PR #35's first push):
 *  - the header, "based on JPL DE%d", asked swe_get_denum() after a Sun of
 *    its own, made before the run's path, JPL file and ephemeris were set:
 *    a Moshier run and a de200.eph run both said DE441 here;
 *  - it was appended with sprintf() to cmdline[1024], which the command
 *    line before it fills to within two bytes: a long command line wrote
 *    up to 18 bytes past the end (ASan: global-buffer-overflow);
 *  - a zero declination was the root of one parabola through the step's
 *    three points: the Moon's on 2000 Mar 8 came out 83 s late at a
 *    1-day step;
 *  - the first form refined that root with four Newton steps, none kept
 *    inside the step that brackets the crossing. At a coarse step a step
 *    left it: the Moon at a 12-day step printed a 2001 Jan 15 crossing in
 *    a run over 2000 and lost the one near May 1, and at a 14-day step
 *    one in 2004; Mercury at a 15-day step from 1 Jan 2002 printed 2002
 *    Oct 15 twice, 6 s apart and out of order, and lost the crossing of
 *    Oct 4; at 12 days from 1 Jan 2005 four steps left the crossing of
 *    Apr 14 52 s out. The root now stays inside the bracket, by bisection
 *    when a Newton step leaves it;
 *  - the first form also asked the header of a fixed day in 1959, not of
 *    the run's begin date: with SE_EPHE_FALLBACK=1, -ejplde200.eph
 *    -b1.3.1500 said "based on JPL DE200" over Moshier positions.
 * And one older than 8a36c49 that its zero declination makes common
 * (UPSTREAM-BUGS 47): dms() rounds 29 Pisces 59'59.5" up to 360 and
 * prints it as znam[12], past the twelve signs, as the Sun's March equinox
 * of 2002 is (ASan: global-buffer-overflow, a read).
 *
 * The checks, on swevents built with ASan (swevents_asan), each run
 * exiting 0 with no sanitizer report:
 *  (a) the header says "based on JPL DE441" (the Swiss files' own number)
 *      for -eswe, "based on the Moshier ephemeris" for -emos, and "based
 *      on JPL DE200" for -ejplde200.eph; for -ejplde200.eph -b1.3.1500,
 *      a date DE200 does not hold, it says "based on the Moshier
 *      ephemeris" with SE_EPHE_FALLBACK=1 and "(no ephemeris" without;
 *  (b) a command line that fills cmdline to three bytes short runs clean;
 *  (c) -dodecl at a 1-day step over March 2000: the Moon's and the Sun's
 *      zero declinations, which a root left unrefined misses by 83 s;
 *  (d) -dodecl at coarse steps, which (c) cannot see: the Moon at 12 and
 *      at 14 days over 2000, Mercury at 12 days over 2005 and at 15 days
 *      over 2002. Each of the four fails on the first form, as described
 *      above;
 *  (e) -dodecl for the Sun over March 2002: the equinox prints as 0 Aries
 *      0'0", with no overflow.
 * For (c) and (d): as many zero declinations as bisection on swe_calc()
 * finds, each printed once and in time order, inside the run (from the
 * begin date to the last step), and each within TOL seconds of the
 * crossing found by bisection. swevents prints whole seconds of UT,
 * rounded to the nearest, so half a second is the least any run can
 * differ by; TOL is 0.6 s, and the fixed code's worst over (c) and (d) is
 * 0.5 s.
 * Bisection looks where swevents can. Its steps are the begin date 0h UT
 * plus k * tstep days, k = 0..n, in ephemeris time, and it compares the
 * declinations of steps k - 1 and k from k = 2 on (steps 0 and 1 only fill
 * its window), so it sees the crossings between step 1 and step n. One
 * between step 0 and step 1 is never printed, and bisection does not look
 * for it. Nor are two crossings in one step ever printed, for they leave
 * the sign as it was (Mercury's pairs, 6 to 10 days apart, like 1993 Feb 26
 * and Mar 5); the runs in (d) have none at their step.
 * Run as
 *   eventsdecl EPHEDIR SWEVENTS
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "swephexp.h"

#define TOL 0.6		/* seconds, for (c) and (d) */
#define NZMAX 64

static int bad, n;
static char out[1 << 16];

/* run swevents with SE_EPHE_PATH=ephe, SE_EPHE_FALLBACK=fb and args; its
 * output, both streams */
static int run(const char *ephe, const char *exe, int fb, const char *args)
{
  char cmd[4096];
  size_t len = 0, r;
  int status;
  FILE *fp;
  snprintf(cmd, sizeof cmd,
           "SE_EPHE_PATH='%s' SE_EPHE_FALLBACK=%d ASAN_OPTIONS=detect_leaks=0 '%s' %s 2>&1",
           ephe, fb, exe, args);
  if ((fp = popen(cmd, "r")) == NULL)
    return -1;
  while ((r = fread(out + len, 1, sizeof out - 1 - len, fp)) > 0)
    len += r;
  out[len] = '\0';
  status = pclose(fp);
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static void check(const char *what, int ok, const char *detail)
{
  n++;
  printf("  %-62s %s\n", what, ok ? "ok" : "FAIL");
  if (!ok) {
    bad++;
    if (detail != NULL)
      printf("    %s\n", detail);
  }
}

static int clean(int status)
{
  return status == 0 && strstr(out, "Sanitizer") == NULL && strstr(out, "runtime error") == NULL;
}

/* the header of a run of opt, from begin date bdate, with fallback fb */
static void header(const char *ephe, const char *exe, const char *opt, const char *bdate, int fb,
                   const char *want)
{
  char args[256], what[128], line[512] = "";
  const char *cmd;
  int status;
  snprintf(args, sizeof args, "%s -p0 -b%s -n2 -s1", opt, bdate);
  status = run(ephe, exe, fb, args);
  if ((cmd = strstr(out, "Command:")) != NULL)
    sscanf(cmd, "%511[^\n]", line);
  snprintf(what, sizeof what, "(a) %s -b%s%s: \"%s\"", opt, bdate, fb ? " fallback" : "", want);
  check(what, clean(status) && strstr(line, want) != NULL, line);
}

static double dec(double t, int ipl)
{
  double x[6];
  char serr[AS_MAXCH];
  swe_calc(t, ipl, SEFLG_SWIEPH | SEFLG_EQUATORIAL, x, serr);
  return x[1];
}

/* ephemeris time of the UT tut, as swevents makes it */
static double et(double tut)
{
  return tut + swe_deltat_ex(tut, SEFLG_SWIEPH, NULL);
}

/* The zero declinations of ipl that swevents can see in a run of nstep
 * steps of tstep days from the UT tb, by bisection: those between its
 * steps 1 and nstep. It compares the declinations of two steps from its
 * step 2 on, so a crossing between step 0 and step 1 is never reported.
 * UT, in order. */
static int exact_zeros(int ipl, double tb, double tstep, int nstep, double *tz, int nmax)
{
  double t0 = et(tb + tstep), t1 = et(tb + nstep * tstep), h, lo, hi, m;
  int i, k, nsub = (int) ceil((t1 - t0) / 0.05), nz = 0;
  h = (t1 - t0) / nsub;
  for (i = 0; i < nsub && nz < nmax; i++) {
    lo = t0 + i * h;
    hi = lo + h;
    if ((dec(lo, ipl) < 0) == (dec(hi, ipl) < 0))
      continue;
    for (k = 0; k < 50; k++) {
      m = (lo + hi) / 2;
      if ((dec(lo, ipl) < 0) != (dec(m, ipl) < 0)) hi = m; else lo = m;
    }
    tz[nz++] = lo - swe_deltat_ex(lo, SEFLG_SWIEPH, NULL);
  }
  return nz;
}

/* -dodecl of ipl for nstep steps of tstep days from y m d 0h UT, tagged
 * tag in the report: swevents' zero declinations are those of bisection,
 * once each, in time order, inside the run, and within TOL */
static void zeros(const char *ephe, const char *exe, const char *tag, int ipl, const char *name,
                  int y, int m, int d, int nstep, double tstep)
{
  static const char *mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  char args[256], what[128], detail[256] = "", mname[8];
  double tz[NZMAX], tg[NZMAX], worst = 0, tb = swe_julday(y, m, d, 0, 1);
  int nz, ng = 0, status, py, pd, hh, mm, ss, k, ok, inorder = 1, inside = 1;
  const char *p;
  nz = exact_zeros(ipl, tb, tstep, nstep, tz, NZMAX);
  snprintf(args, sizeof args, "-eswe -p%d -b%d.%d.%d -n%d -s%g -dodecl", ipl, d, m, y, nstep,
           tstep);
  status = run(ephe, exe, 0, args);
  p = out;
  while ((p = strstr(p, "zero declination")) != NULL && ng < NZMAX) {
    p += strlen("zero declination");
    if (sscanf(p, " %d %3s %d %d:%d:%d", &py, mname, &pd, &hh, &mm, &ss) != 6)
      continue;
    for (k = 0; k < 12 && strcmp(mname, mon[k]) != 0; k++)
      ;
    tg[ng++] = swe_julday(py, k + 1, pd, hh + mm / 60.0 + ss / 3600.0, 1);
  }
  for (k = 0; k < ng; k++) {
    if (k > 0 && tg[k] <= tg[k - 1])
      inorder = 0;
    if (tg[k] < tb || tg[k] > tb + nstep * tstep)
      inside = 0;
    if (k < nz && fabs(tg[k] - tz[k]) * 86400 > worst)
      worst = fabs(tg[k] - tz[k]) * 86400;
  }
  ok = clean(status) && ng == nz && nz > 0 && inorder && inside && worst <= TOL;
  snprintf(detail, sizeof detail, "%d found of %d, worst %.2f s%s%s", ng, nz, worst,
           inorder ? "" : ", not in time order", inside ? "" : ", outside the run");
  snprintf(what, sizeof what, "(%s) %s's zero declinations, %g-day step (%d, %.2f s)", tag, name,
           tstep, ng, worst);
  check(what, ok, detail);
}

int main(int argc, char **argv)
{
  char args[2048];
  size_t base;
  int status;
  if (argc != 3) {
    fprintf(stderr, "usage: eventsdecl EPHEDIR SWEVENTS\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G55: swevents' header and its zero declinations\n");
  header(argv[1], argv[2], "-eswe", "1.3.2000", 0, "based on JPL DE441");
  header(argv[1], argv[2], "-emos", "1.3.2000", 0, "based on the Moshier ephemeris");
  header(argv[1], argv[2], "-ejplde200.eph", "1.3.2000", 0, "based on JPL DE200");
  header(argv[1], argv[2], "-ejplde200.eph", "1.3.1500", 1, "based on the Moshier ephemeris");
  header(argv[1], argv[2], "-ejplde200.eph", "1.3.1500", 0, "(no ephemeris");

  /* "Command: " + argv[0] + " " + "-emos " + the long argument + " ",
   * three bytes short of cmdline's 1024 */
  base = strlen("Command: ") + strlen(argv[2]) + 1 + strlen("-emos ");
  strcpy(args, "-emos -tzone");
  memset(args + strlen(args), '0', 1021 - base - 1 - strlen("-tzone"));
  args[strlen("-emos ") + 1021 - base - 1] = '\0';
  strcat(args, " -p0 -b1.3.2000 -n2 -s1");
  status = run(argv[1], argv[2], 0, args);
  check("(b) a command line that fills cmdline: no overflow", clean(status),
        strstr(out, "Sanitizer") != NULL ? strstr(out, "ERROR") : NULL);

  zeros(argv[1], argv[2], "c", SE_MOON, "the Moon", 2000, 3, 1, 30, 1);
  zeros(argv[1], argv[2], "c", SE_SUN, "the Sun", 2000, 3, 1, 30, 1);
  zeros(argv[1], argv[2], "d", SE_MOON, "the Moon", 2000, 1, 1, 30, 12);
  zeros(argv[1], argv[2], "d", SE_MOON, "the Moon", 2000, 1, 1, 26, 14);
  zeros(argv[1], argv[2], "d", SE_MERCURY, "Mercury", 2005, 1, 1, 31, 12);
  zeros(argv[1], argv[2], "d", SE_MERCURY, "Mercury", 2002, 1, 1, 25, 15);

  status = run(argv[1], argv[2], 0, "-eswe -p0 -b1.3.2002 -n30 -s1 -dodecl");
  check("(e) the Sun's 2002 March equinox: 0 Aries, no overflow",
        clean(status) && strstr(out, " 0 AR  0' 0\"") != NULL,
        strstr(out, "Sanitizer") != NULL ? strstr(out, "ERROR") : NULL);
  swe_close();
  if (bad) {
    printf("G55 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("G55 PASS (%d)\n", n);
  return 0;
}
