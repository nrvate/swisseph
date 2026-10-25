/* pathkeep.c -- G97: a star function as the first call finds the files
 * SE_EPHE_PATH names, and a path change keeps the models chosen.
 *
 * The defects (UPSTREAM-BUGS 98):
 *
 * 1. The four star functions (swe_fixstar, swe_fixstar2, swe_fixstar_mag,
 *    swe_fixstar2_mag) read sefstars.txt along the ephemeris path but
 *    never named one, so as the first library call, with the files named
 *    by SE_EPHE_PATH alone, they failed: "SwissEph file 'sefstars.txt' not
 *    found in PATH ''" (upstream; the compiled default after bug 92's
 *    sync). swe_calc() names the path on first use from the variable.
 * 2. swi_close_keep_topo_etc(), run by every swe_set_ephe_path() and
 *    swe_set_jpl_file() and by the implicit path of a first calculation,
 *    zeroed the astronomical models, so a path change put the defaults
 *    back over what swe_set_astro_models() or swe_set_sid_mode(...,
 *    SE_SIDBIT_PREC_ORIG) had set: delta t Stephenson & Morrison 1984 and
 *    nutation IAU 1980 set, the nutation at J2000 2.39e-6 deg off and
 *    delta t in AD 1000 18.27 s (1474.79 for 1456.52); Lahiri with its
 *    original precession 0.00056 deg in AD 1000. Upstream's too.
 *
 * The check, each scenario in a forked, pristine process against a
 * reference in another:
 *   star    each star function on Sirius as the first call, files by
 *           SE_EPHE_PATH, against the same after swe_set_ephe_path()
 *           (return code, value, serr);
 *   models  swe_set_astro_models("1,0,0,1") followed by a second path,
 *           swe_set_jpl_file(), or (set first) a calculation with the
 *           files by SE_EPHE_PATH, against the setting after one path
 *           (the true nutation in longitude and obliquity at J2000,
 *           delta t in AD 1000); swe_close() and a path after it against
 *           no setting; Lahiri with SE_SIDBIT_PREC_ORIG and a second
 *           path against one path (the ayanamsa in AD 1000).
 * Within 1e-12 (deg, s); each setting must move its observation by more
 * than 1e-7 against none, so the check sees it. After the fix all 9 pass.
 *
 * Measured before it, 8 of 9 fail: the four star functions (ERR), the
 * three model scenarios (2.39e-6 deg, 18.27 s), the original precession
 * (0.00056 deg). The compat build fails the same 8, the upstream build
 * (legacy-master) the same 8. Run as
 *   pathkeep EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define TOL 1e-12
#define NV 16
#define T_1000 2086302.5

static const char *EPHE;
struct ans { int n; double v[NV]; char serr[AS_MAXCH]; };

static void put(struct ans *a, double x)
{
  if (a->n < NV)
    a->v[a->n++] = x;
}

static int in_child(void (*fn)(int, struct ans *), int k, struct ans *out)
{
  int fd[2];
  ssize_t want = (ssize_t) sizeof *out, got = 0, r;
  pid_t pid;
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    close(fd[0]);
    memset(out, 0, sizeof *out);
    fn(k, out);
    while (got < want && (r = write(fd[1], (char *) out + got, want - got)) > 0)
      got += r;
    _exit(got == want ? 0 : 3);
  }
  close(fd[1]);
  while (got < want && (r = read(fd[0], (char *) out + got, want - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return got == want ? 0 : -1;
}

static double off(const struct ans *s, const struct ans *f)
{
  double d = 0;
  int j;
  if (s->n != f->n)
    return HUGE_VAL;
  for (j = 0; j < f->n; j++)
    if (!(fabs(s->v[j] - f->v[j]) <= d))
      d = fabs(s->v[j] - f->v[j]);
  return d;
}

/*---------------------------------------------------------------- stars */
static const char *SN[4] = { "swe_fixstar", "swe_fixstar2", "swe_fixstar_mag", "swe_fixstar2_mag" };

static void star(int k, struct ans *a)
{
  char s[AS_MAXCH];
  double x[6], m;
  int j;
  if (k >= 4) {
    swe_set_ephe_path(EPHE);
    k -= 4;
  } else {
    setenv("SE_EPHE_PATH", EPHE, 1);
  }
  strcpy(s, "sirius");
  switch (k) {
  case 0: put(a, swe_fixstar(s, 2451545.0, SEFLG_SWIEPH, x, a->serr)); break;
  case 1: put(a, swe_fixstar2(s, 2451545.0, SEFLG_SWIEPH, x, a->serr)); break;
  case 2: put(a, swe_fixstar_mag(s, &m, a->serr)); x[0] = m; break;
  case 3: put(a, swe_fixstar2_mag(s, &m, a->serr)); x[0] = m; break;
  }
  for (j = 0; j < (k < 2 ? 3 : 1); j++)
    put(a, x[j]);
}

/*--------------------------------------------------------------- models */
static char MODELS[] = "1,0,0,1";	/* delta t SM 1984, nutation IAU 1980 */

static void observe_models(struct ans *a)
{
  double x[6];
  char serr[AS_MAXCH];
  swe_calc(2451545.0, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  put(a, x[2]);
  put(a, x[3]);
  put(a, swe_deltat_ex(T_1000, SEFLG_SWIEPH, serr) * 86400);
}

static void observe_aya(struct ans *a)
{
  double aya;
  char serr[AS_MAXCH];
  swe_get_ayanamsa_ex(T_1000, SEFLG_SWIEPH | SEFLG_SIDEREAL, &aya, serr);
  put(a, aya);
}

static const struct { int ref; const char *what; } M[] = {
  { 5, "models: path, set, path" },
  { 5, "models: path, set, swe_set_jpl_file()" },
  { 5, "models: set first, SE_EPHE_PATH, calc" },
  { 6, "models: path, set, swe_close(), path" },
  { 7, "PREC_ORIG: path, sid mode, path" },
};
#define NM ((int) (sizeof M / sizeof M[0]))

static void models(int k, struct ans *a)
{
  int aya = 0;
  switch (k) {
  case 0:
    swe_set_ephe_path(EPHE);
    swe_set_astro_models(MODELS, 0);
    swe_set_ephe_path(EPHE);
    break;
  case 1:
    swe_set_ephe_path(EPHE);
    swe_set_astro_models(MODELS, 0);
    swe_set_jpl_file("de200.eph");
    break;
  case 2:
    setenv("SE_EPHE_PATH", EPHE, 1);
    swe_set_astro_models(MODELS, 0);
    break;
  case 3:
    swe_set_ephe_path(EPHE);
    swe_set_astro_models(MODELS, 0);
    swe_close();
    swe_set_ephe_path(EPHE);
    break;
  case 4:
    swe_set_ephe_path(EPHE);
    swe_set_sid_mode(SE_SIDM_LAHIRI | SE_SIDBIT_PREC_ORIG, 0, 0);
    swe_set_ephe_path(EPHE);
    aya = 1;
    break;
  case 5:	/* references: the setting after one path */
    swe_set_ephe_path(EPHE);
    swe_set_astro_models(MODELS, 0);
    break;
  case 6:	/* no setting */
    swe_set_ephe_path(EPHE);
    break;
  case 7:
    swe_set_ephe_path(EPHE);
    swe_set_sid_mode(SE_SIDM_LAHIRI | SE_SIDBIT_PREC_ORIG, 0, 0);
    aya = 1;
    break;
  case 8:	/* Lahiri without the bit: the setting must show */
    swe_set_ephe_path(EPHE);
    swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
    aya = 1;
    break;
  }
  if (aya)
    observe_aya(a);
  else
    observe_models(a);
}

int main(int argc, char **argv)
{
  static struct ans s, f, z;
  int k, n = 0, nfail = 0;
  double d;
  EPHE = argc > 1 ? argv[1] : "../ephe";

  for (k = 0; k < 4; k++, n++) {
    if (in_child(star, k, &s) != 0 || in_child(star, k + 4, &f) != 0) {
      printf("  %-40s did not run\n", SN[k]);
      nfail++;
      continue;
    }
    d = off(&s, &f);
    if (d > TOL || strcmp(s.serr, f.serr) != 0 || f.v[0] < 0) {
      printf("  %-40s FAIL: rc %g (%s), after a path rc %g, off by %.3g\n", SN[k],
             s.v[0], s.serr, f.v[0], d);
      nfail++;
    }
  }
  for (k = 0; k < NM; k++, n++) {
    if (in_child(models, k, &s) != 0 || in_child(models, M[k].ref, &f) != 0
        || in_child(models, M[k].ref == 7 ? 8 : (M[k].ref == 5 ? 6 : 5), &z) != 0) {
      printf("  %-40s did not run\n", M[k].what);
      nfail++;
      continue;
    }
    if (!(off(&f, &z) > 1e-7)) {
      printf("  %-40s FAIL: the setting does not show (%.3g)\n", M[k].what, off(&f, &z));
      nfail++;
      continue;
    }
    d = off(&s, &f);
    if (d > TOL) {
      int j, at = 0;
      for (j = 0; j < f.n; j++)
        if (fabs(s.v[j] - f.v[j]) >= fabs(s.v[at] - f.v[at]))
          at = j;
      printf("  %-40s FAIL: [%d] %.10g, expected %.10g (off by %.3g)\n", M[k].what,
             at, s.v[at], f.v[at], d);
      nfail++;
    }
  }
  if (nfail)
    printf("G97 FAIL: %d of %d\n", nfail, n);
  else
    printf("G97 PASS (%d scenarios)\n", n);
  return nfail != 0;
}
