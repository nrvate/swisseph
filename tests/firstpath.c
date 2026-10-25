/* firstpath.c -- G106: with the files named by SE_EPHE_PATH, the first call
 * of any entry point reads every file along that path, once.
 *
 * The defect (UPSTREAM-BUGS 107): with no swe_set_ephe_path() call the
 * path is the one SE_EPHE_PATH names, set by the first swe_calc() and star
 * function, and whatever read a file before that read it along the
 * compiled default path; a context not yet initialised was initialised
 * after the read, which wiped the table, so it was read again.
 *
 * The check: a directory of links to the files, with its own
 * swe_deltat.txt (70.0 s for 2000-2002, the built-in table 63.83 s at
 * J2000), named by SE_EPHE_PATH. Each of 22 entry points is called first,
 * twice, in a fresh process, counting fopen() (interposed): no open outside
 * the directory, no file opened twice, and the first answer equal to the
 * second and to a fresh process's after swe_set_ephe_path() of the same
 * directory, within 1e-12 (deg, s, h, day).
 *
 * Measured before the fix (16 of 22 fail): swe_calc_ut() Moon 8.6e-4 deg
 * and 6 opens outside the path, swe_deltat() 6.17 s, swe_sidtime()
 * 1.76e-10 h and 8 opens, swe_houses() 3.9e-9 deg, swe_azalt() 7.6e-5
 * deg, the stars 2.9e-6 deg, the ayanamsa 2.7e-9 deg, swe_lun_eclipse_how()
 * 5.6e-4, swe_pheno_ut() 7.8e-4 deg, swe_rise_trans() 6 opens,
 * swe_utc_to_jd() 6.7e-5 day of UT1 and 3 opens (seleapsec.txt),
 * swe_set_jpl_file() 5.5e-5 deg on Mars, 9 opens, de200.eph opened twice.
 * The compat build fails 17 (its star functions without the catalogue,
 * UPSTREAM-BUGS 98), the upstream build (legacy-master) 17. Run as
 *   firstpath EPHEDIR
 */
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T0 2451545.0
#define TOL 1e-12

static char DIR[] = "/tmp/firstpath-XXXXXX";	/* links + its own swe_deltat.txt */
static const char *EPHE;

/* every fopen() this process makes: outside DIR, and per file inside it */
#define NF 16
static int n_outside, n_in[NF];
static char f_in[NF][64];
static char first_outside[2 * AS_MAXCH];
FILE *fopen(const char *p, const char *m)
{
  static FILE *(*real)(const char *, const char *);
  size_t nd = strlen(DIR);
  FILE *fp;
  if (real == NULL)
    *(void **) &real = dlsym(RTLD_NEXT, "fopen");
  fp = real(p, m);
  if (strncmp(p, DIR, nd) != 0 || p[nd] != '/') {
    if (n_outside++ == 0)
      snprintf(first_outside, sizeof first_outside, "%s", p);
  } else if (fp != NULL) {
    const char *b = p + nd;
    int i;
    while (*b == '/')
      b++;
    for (i = 0; i < NF && f_in[i][0] != '\0' && strcmp(f_in[i], b) != 0; i++)
      ;
    if (i < NF) {
      snprintf(f_in[i], sizeof f_in[i], "%s", b);
      n_in[i]++;
    }
  }
  return fp;
}

struct ans {
  double v[2][6];	/* first call, second call */
  int outside, most, ran;
  char most_name[64], outside_name[2 * AS_MAXCH];
};

enum { C_CALC_UT, C_CALC, C_DELTAT, C_DELTAT_EX, C_SIDTIME, C_HOUSES,
       C_HOUSES_EX2, C_AZALT, C_FIXSTAR_UT, C_FIXSTAR2_UT, C_FIXSTAR2_MAG,
       C_AYA_UT, C_AYA_EX_UT, C_SOL_GLOB, C_SOL_LOC, C_LUN_WHEN, C_LUN_HOW,
       C_RISE, C_PHENO_UT, C_HELIACAL, C_UTC_TO_JD, C_JPL, NC };
static const char *CN[NC] = {
  "swe_calc_ut Moon", "swe_calc Moon (TT)", "swe_deltat", "swe_deltat_ex SWIEPH",
  "swe_sidtime", "swe_houses Placidus", "swe_houses_ex2 Koch",
  "swe_azalt Sun", "swe_fixstar_ut Aldebaran", "swe_fixstar2_ut Spica",
  "swe_fixstar2_mag Sirius", "swe_get_ayanamsa_ut", "swe_get_ayanamsa_ex_ut",
  "swe_sol_eclipse_when_glob", "swe_sol_eclipse_when_loc",
  "swe_lun_eclipse_when", "swe_lun_eclipse_how", "swe_rise_trans Sun",
  "swe_pheno_ut Moon", "swe_heliacal_ut Venus", "swe_utc_to_jd",
  "swe_set_jpl_file + JPL Mars" };

static void call(int k, double *v)
{
  double x[6], c[13], a[10], tret[10], attr[20];
  double geo[3] = { 8.55, 47.37, 400 }, atm[4] = { 1013.25, 15, 40, 0 };
  double obs[6] = { 36, 1, 0, 1, 0, 0 };
  char serr[AS_MAXCH], star[64];
  int32 r;
  memset(x, 0, sizeof x);
  switch (k) {
  case C_CALC_UT: swe_calc_ut(T0, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, x, serr); break;
  case C_CALC: swe_calc(T0, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, x, serr); break;
  case C_DELTAT: x[0] = swe_deltat(T0) * 86400; break;
  case C_DELTAT_EX: x[0] = swe_deltat_ex(T0, SEFLG_SWIEPH, serr) * 86400; break;
  case C_SIDTIME: x[0] = swe_sidtime(T0); break;
  case C_HOUSES: swe_houses(T0, 47.37, 8.55, 'P', c, a); x[0] = c[1]; x[1] = c[10]; x[2] = a[0]; x[3] = a[1]; break;
  case C_HOUSES_EX2: swe_houses_ex2(T0, 0, 47.37, 8.55, 'K', c, a, NULL, NULL, serr); x[0] = c[2]; x[1] = c[11]; x[2] = a[0]; x[3] = a[1]; break;
  case C_AZALT: { double s[6]; swe_calc_ut(T0, SE_SUN, SEFLG_SWIEPH, s, serr);
    x[3] = s[0]; swe_azalt(T0, SE_ECL2HOR, geo, 0, 10, s, x); } break;
  case C_FIXSTAR_UT: strcpy(star, "Aldebaran"); swe_fixstar_ut(star, T0, SEFLG_SWIEPH, x, serr); break;
  case C_FIXSTAR2_UT: strcpy(star, "Spica"); swe_fixstar2_ut(star, T0, SEFLG_SWIEPH, x, serr); break;
  case C_FIXSTAR2_MAG: { double m = 0; strcpy(star, "Sirius"); swe_fixstar2_mag(star, &m, serr); x[0] = m; } break;
  case C_AYA_UT: x[0] = swe_get_ayanamsa_ut(T0); break;
  case C_AYA_EX_UT: swe_get_ayanamsa_ex_ut(T0, SEFLG_SWIEPH, &x[0], serr); break;
  case C_SOL_GLOB: swe_sol_eclipse_when_glob(T0, SEFLG_SWIEPH, 0, tret, 0, serr); memcpy(x, tret, sizeof x); break;
  case C_SOL_LOC: swe_sol_eclipse_when_loc(T0, SEFLG_SWIEPH, geo, tret, attr, 0, serr); x[0] = tret[0]; x[1] = tret[1]; x[2] = attr[0]; break;
  case C_LUN_WHEN: swe_lun_eclipse_when(T0, SEFLG_SWIEPH, 0, tret, 0, serr); memcpy(x, tret, sizeof x); break;
  case C_LUN_HOW: swe_lun_eclipse_how(2451564.69, SEFLG_SWIEPH, geo, attr, serr); x[0] = attr[0]; x[1] = attr[1]; x[2] = attr[4]; x[3] = attr[5]; break;
  case C_RISE: swe_rise_trans(T0, SE_SUN, NULL, SEFLG_SWIEPH, SE_CALC_RISE, geo, 1013.25, 15, x, serr); break;
  case C_PHENO_UT: swe_pheno_ut(T0, SE_MOON, SEFLG_SWIEPH, attr, serr); memcpy(x, attr, sizeof x); break;
  case C_HELIACAL: strcpy(star, "Venus");
    swe_heliacal_ut(T0, geo, atm, obs, star, SE_HELIACAL_RISING, SEFLG_SWIEPH, tret, serr); x[0] = tret[0]; break;
  case C_UTC_TO_JD: swe_utc_to_jd(2000, 1, 1, 12, 0, 0, SE_GREG_CAL, tret, serr); x[0] = tret[0]; x[1] = tret[1]; break;
  case C_JPL: swe_set_jpl_file("de200.eph"); r = swe_calc_ut(T0, SE_MARS, SEFLG_JPLEPH | SEFLG_SPEED, x, serr);
    x[5] = r; break;
  }
  memcpy(v, x, 6 * sizeof(double));
}

/* implicit: SE_EPHE_PATH names DIR, the entry point called twice;
 * explicit: swe_set_ephe_path(DIR), called once */
static void scenario(int k, int explicit_path, struct ans *a)
{
  int i;
  n_outside = 0;	/* not the parent's own writing of swe_deltat.txt */
  memset(n_in, 0, sizeof n_in);
  memset(f_in, 0, sizeof f_in);
  if (explicit_path) {
    unsetenv("SE_EPHE_PATH");
    swe_set_ephe_path(DIR);
  } else
    setenv("SE_EPHE_PATH", DIR, 1);
  call(k, a->v[0]);
  if (!explicit_path)
    call(k, a->v[1]);
  a->outside = n_outside;
  snprintf(a->outside_name, sizeof a->outside_name, "%s", first_outside);
  for (i = 0; i < NF; i++)
    if (n_in[i] > a->most) {
      a->most = n_in[i];
      memcpy(a->most_name, f_in[i], sizeof a->most_name);	/* both 64 */
    }
  a->ran = 1;
}

static int in_child(int k, int explicit_path, struct ans *out)
{
  int fd[2], st;
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
    scenario(k, explicit_path, &a);
    _exit(write(fd[1], &a, sizeof a) == (ssize_t) sizeof a ? 0 : 3);
  }
  close(fd[1]);
  memset(out, 0, sizeof *out);
  while (got < (ssize_t) sizeof *out
         && (r = read(fd[0], (char *) out + got, sizeof *out - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, &st, 0);
  return (got == (ssize_t) sizeof *out && out->ran) ? 0 : -1;
}

static double maxdiff(const double *a, const double *b)
{
  double d = 0;
  int j;
  for (j = 0; j < 6; j++)
    if (!(fabs(a[j] - b[j]) <= d))
      d = fabs(a[j] - b[j]);
  return d;
}

static const char *LNK[] = { "sepl_18.se1", "semo_18.se1", "seas_18.se1",
                             "sefstars.txt", "seasnam.txt", "de200.eph" };
#define NLNK (sizeof LNK / sizeof LNK[0])

int main(int argc, char **argv)
{
  char lnk[sizeof DIR + 32], tgt[2 * AS_MAXCH], cwd[AS_MAXCH];
  struct ans a, e;
  int k, nfail = 0;
  unsigned j;
  FILE *fp;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  if (mkdtemp(DIR) == NULL || getcwd(cwd, sizeof cwd) == NULL) {
    printf("G106 FAIL: no temporary directory\n");
    return 1;
  }
  for (j = 0; j < NLNK; j++) {
    if (EPHE[0] == '/')
      snprintf(tgt, sizeof tgt, "%s/%s", EPHE, LNK[j]);
    else
      snprintf(tgt, sizeof tgt, "%s/%s/%s", cwd, EPHE, LNK[j]);
    snprintf(lnk, sizeof lnk, "%s/%s", DIR, LNK[j]);
    if (symlink(tgt, lnk) != 0)
      printf("  note: no link %s\n", lnk);
  }
  /* delta t of 70 s over 2000-2002, against the built-in 63.83 s at J2000 */
  snprintf(lnk, sizeof lnk, "%s/swe_deltat.txt", DIR);
  if ((fp = fopen(lnk, "w")) == NULL) {
    printf("G106 FAIL: cannot write %s\n", lnk);
    return 1;
  }
  fputs("2000 70.0\n2001 70.0\n2002 70.0\n", fp);
  fclose(fp);
  for (k = 0; k < NC; k++) {
    double d12, d1e;
    if (in_child(k, 0, &a) != 0 || in_child(k, 1, &e) != 0) {
      printf("  %-30s did not run\n", CN[k]);
      nfail++;
      continue;
    }
    d12 = maxdiff(a.v[0], a.v[1]);
    d1e = maxdiff(a.v[0], e.v[0]);
    if (a.outside != 0 || a.most > 1 || d12 > TOL || d1e > TOL) {
      printf("  %-30s FAIL:", CN[k]);
      if (a.outside)
        printf(" %d opens outside the path (%s)", a.outside, a.outside_name);
      if (a.most > 1)
        printf(" %s opened %d times", a.most_name, a.most);
      if (d12 > TOL)
        printf(" first %.10g, second %.10g (%.3g)", a.v[0][0], a.v[1][0], d12);
      if (d1e > TOL)
        printf(" off the explicit path by %.3g", d1e);
      printf("\n");
      nfail++;
    } else
      printf("  %-30s %.10g\n", CN[k], a.v[0][0]);
  }
  for (j = 0; j < NLNK; j++) {
    snprintf(lnk, sizeof lnk, "%s/%s", DIR, LNK[j]);
    unlink(lnk);
  }
  snprintf(lnk, sizeof lnk, "%s/swe_deltat.txt", DIR);
  unlink(lnk);
  rmdir(DIR);
  if (nfail)
    printf("G106 FAIL: %d of %d\n", nfail, NC);
  else
    printf("G106 PASS (%d)\n", NC);
  return nfail != 0;
}
