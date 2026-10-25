/* pathwarn.c -- G109: a first call warns "Please call swe_set_ephe_path()
 * or swe_set_jplfile() before calling ..." only when no path is in effect.
 *
 * The defect (UPSTREAM-BUGS 110): the warning was written whenever the call
 * initialised the context, whether or not a path was in effect. A caller
 * that takes a non-empty serr as a problem had a false alarm on a correct
 * answer, and swe_calc_orbel() (this fork's) refused on it.
 *
 * The check: 21 first calls, each in a fresh process, in four settings --
 * SE_EPHE_PATH naming a directory of links to the files; no SE_EPHE_PATH,
 * the current directory holding the files ("." starts the default path);
 * the path set by another thread; and no path at all (an empty current
 * directory). With a path in effect, every call answers (return >= 0) with
 * serr empty. With none, swe_deltat_ex(), swe_get_ayanamsa_ex_ut() (its
 * delta t) and swe_calc_orbel() still carry the warning; the others answer
 * with the missing file's own message, which is not checked.
 *
 * Measured before the fix, 9 of 84 fail: in each of the three settings with
 * a path, swe_deltat_ex() and swe_get_ayanamsa_ex_ut() answer with the
 * warning in serr, and swe_calc_orbel() returns -1 on it. The compat build
 * fails 15 (with entries 98 and 107 restored the stars miss their catalogue
 * and the True Citra ayanamsa its files); the upstream build
 * (legacy-master, no swe_calc_orbel) 24, swe_calc() carrying the warning
 * too and its threads sharing no path. Run as
 *   pathwarn EPHEDIR [v]
 */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T0 2451545.0
#define WARN "swe_set_ephe_path() or swe_set_jplfile() before"

static char DIR[] = "/tmp/pathwarn-XXXXXX";	/* the files, through links */
static char EMPTY[] = "/tmp/pathwarn-none-XXXXXX";	/* no files */
static const char *EPHE;
static int verbose;

enum { C_CALC_UT, C_CALC, C_ORBEL, C_DELTAT_EX, C_AYA_EX_UT, C_AYA_EX,
       C_FIXSTAR_UT, C_FIXSTAR2_UT, C_FIXSTAR, C_TIME_EQU, C_LMT_TO_LAT,
       C_HOUSES_EX2, C_SOL_GLOB, C_LUN_HOW, C_PHENO_UT, C_RISE, C_NOD_APS_UT,
       C_UTC_TO_JD, C_HELIACAL, C_AYA_STAR_UT, C_AYA_STAR, NC };
static const char *CN[NC] = {
  "swe_calc_ut Moon", "swe_calc Moon", "swe_calc_orbel", "swe_deltat_ex SWIEPH",
  "swe_get_ayanamsa_ex_ut", "swe_get_ayanamsa_ex", "swe_fixstar_ut", "swe_fixstar2_ut",
  "swe_fixstar", "swe_time_equ", "swe_lmt_to_lat", "swe_houses_ex2",
  "swe_sol_eclipse_when_glob", "swe_lun_eclipse_how", "swe_pheno_ut",
  "swe_rise_trans", "swe_nod_aps_ut", "swe_utc_to_jd", "swe_heliacal_ut",
  "True Citra ayanamsa_ex_ut", "True Citra ayanamsa_ex" };

enum { S_ENV, S_CWD, S_THREAD, S_NONE, NS };
static const char *SN[NS] = { "SE_EPHE_PATH", "files in the default path",
                              "path set by another thread", "no path at all" };

static int32 call(int k, char *serr)
{
  int32 r = 0;
  double x[20], c[13], a[10], tret[10], attr[20], d;
  double geo[3] = { 8.55, 47.37, 400 }, atm[4] = { 1013.25, 15, 40, 0 };
  double obs[6] = { 36, 1, 0, 1, 0, 0 }, el[6] = { 200, 0.387, 0.2056, 29, 48, 7.0 };
  char star[64];
  serr[0] = '\0';
  switch (k) {
  case C_CALC_UT: r = swe_calc_ut(T0, SE_MOON, SEFLG_SWIEPH, x, serr); break;
  case C_CALC: r = swe_calc(T0, SE_MOON, SEFLG_SWIEPH, x, serr); break;
#ifdef SE_ORBEL_EQ_J2000	/* this fork's */
  case C_ORBEL: r = swe_calc_orbel(T0, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, SEFLG_SWIEPH, x, serr); break;
#else
  case C_ORBEL: (void) el; break;	/* upstream has none */
#endif
  case C_DELTAT_EX: d = swe_deltat_ex(T0, SEFLG_SWIEPH, serr); r = d > 0 ? 0 : -1; break;
  case C_AYA_EX_UT: r = swe_get_ayanamsa_ex_ut(T0, SEFLG_SWIEPH, &d, serr); break;
  case C_AYA_EX: r = swe_get_ayanamsa_ex(T0, SEFLG_SWIEPH, &d, serr); break;
  case C_FIXSTAR_UT: strcpy(star, "Aldebaran"); r = swe_fixstar_ut(star, T0, SEFLG_SWIEPH, x, serr); break;
  case C_FIXSTAR2_UT: strcpy(star, "Spica"); r = swe_fixstar2_ut(star, T0, SEFLG_SWIEPH, x, serr); break;
  case C_FIXSTAR: strcpy(star, "Sirius"); r = swe_fixstar(star, T0, SEFLG_SWIEPH, x, serr); break;
  case C_TIME_EQU: r = swe_time_equ(T0, &d, serr); break;
  case C_LMT_TO_LAT: r = swe_lmt_to_lat(T0, 8.55, &d, serr); break;
  case C_HOUSES_EX2: r = swe_houses_ex2(T0, 0, 47.37, 8.55, 'K', c, a, NULL, NULL, serr); break;
  case C_SOL_GLOB: r = swe_sol_eclipse_when_glob(T0, SEFLG_SWIEPH, 0, tret, 0, serr); break;
  case C_LUN_HOW: r = swe_lun_eclipse_how(2451564.69, SEFLG_SWIEPH, geo, attr, serr); break;
  case C_PHENO_UT: r = swe_pheno_ut(T0, SE_MOON, SEFLG_SWIEPH, attr, serr); break;
  case C_RISE: r = swe_rise_trans(T0, SE_SUN, NULL, SEFLG_SWIEPH, SE_CALC_RISE, geo, 1013.25, 15, x, serr); break;
  case C_NOD_APS_UT: r = swe_nod_aps_ut(T0, SE_MARS, SEFLG_SWIEPH, SE_NODBIT_MEAN, x, c, a, attr, serr); break;
  case C_UTC_TO_JD: r = swe_utc_to_jd(2000, 1, 1, 12, 0, 0, SE_GREG_CAL, tret, serr); break;
  case C_HELIACAL: strcpy(star, "Venus");
    r = swe_heliacal_ut(T0, geo, atm, obs, star, SE_HELIACAL_RISING, SEFLG_SWIEPH, tret, serr); break;
  case C_AYA_STAR_UT: swe_set_sid_mode(SE_SIDM_TRUE_CITRA, 0, 0);
    r = swe_get_ayanamsa_ex_ut(T0, SEFLG_SWIEPH, &d, serr); break;
  case C_AYA_STAR: swe_set_sid_mode(SE_SIDM_TRUE_CITRA, 0, 0);
    r = swe_get_ayanamsa_ex(T0, SEFLG_SWIEPH, &d, serr); break;
  }
  return r;
}

struct res { int32 r; int ran; char serr[AS_MAXCH]; };

static void *worker(void *arg)
{
  struct res *a = arg;
  a->r = call(a->ran, a->serr);	/* ran: the case, until it has */
  a->ran = 1;
  return NULL;
}

static void scenario(int k, int s, struct res *a)
{
  pthread_t th;
  unsetenv("SE_EPHE_PATH");
  if (chdir(s == S_CWD ? DIR : EMPTY) != 0)
    return;
  if (s == S_ENV)
    setenv("SE_EPHE_PATH", DIR, 1);
  if (s != S_THREAD) {
    a->r = call(k, a->serr);
    a->ran = 1;
    return;
  }
  swe_set_ephe_path(DIR);	/* this thread; the call is another's first */
  a->ran = k;
  if (pthread_create(&th, NULL, worker, a) != 0) {
    a->ran = 0;
    return;
  }
  pthread_join(th, NULL);
}

static int in_child(int k, int s, struct res *out)
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
    struct res a;
    close(fd[0]);
    memset(&a, 0, sizeof a);
    scenario(k, s, &a);
    _exit(write(fd[1], &a, sizeof a) == (ssize_t) sizeof a ? 0 : 3);
  }
  close(fd[1]);
  memset(out, 0, sizeof *out);
  while (got < (ssize_t) sizeof *out
         && (r = read(fd[0], (char *) out + got, sizeof *out - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  out->serr[AS_MAXCH - 1] = '\0';
  return (got == (ssize_t) sizeof *out && out->ran == 1) ? 0 : -1;
}

/* With no path at all, the cases whose warning reaches the caller: the
 * others answer with the refusal of the missing file instead */
static int warns_without_path(int k)
{
#ifdef SE_ORBEL_EQ_J2000
  if (k == C_ORBEL)
    return 1;
#endif
  return k == C_DELTAT_EX || k == C_AYA_EX_UT;
}

static const char *LNK[] = { "sepl_18.se1", "semo_18.se1", "seas_18.se1",
                             "sefstars.txt", "seasnam.txt" };
#define NLNK (sizeof LNK / sizeof LNK[0])

int main(int argc, char **argv)
{
  char lnk[sizeof DIR + 32], tgt[2 * AS_MAXCH], cwd[AS_MAXCH];
  int k, s, nfail = 0, n = 0;
  unsigned j;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  verbose = argc > 2;
  if (mkdtemp(DIR) == NULL || mkdtemp(EMPTY) == NULL || getcwd(cwd, sizeof cwd) == NULL) {
    printf("G109 FAIL: no temporary directory\n");
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
  for (s = 0; s < NS; s++) {
    if (verbose)
      printf("  %s:\n", SN[s]);
    for (k = 0; k < NC; k++, n++) {
      struct res a;
      int ok;
      if (in_child(k, s, &a) != 0) {
        printf("  %s, %-28s did not run\n", SN[s], CN[k]);
        nfail++;
        continue;
      }
      if (s != S_NONE)	/* a path in effect: no message, an answer */
        ok = a.serr[0] == '\0' && a.r >= 0;
      else if (warns_without_path(k))
        ok = strstr(a.serr, WARN) != NULL;
      else
        ok = 1;
      if (!ok) {
        printf("  %s, %-28s FAIL: returned %d, serr [%s]\n", SN[s], CN[k], a.r, a.serr);
        nfail++;
      } else if (verbose)
        printf("    %-28s %d [%s]\n", CN[k], a.r, a.serr);
    }
  }
  for (j = 0; j < NLNK; j++) {
    snprintf(lnk, sizeof lnk, "%s/%s", DIR, LNK[j]);
    unlink(lnk);
  }
  rmdir(DIR);
  rmdir(EMPTY);
  if (nfail)
    printf("G109 FAIL: %d of %d\n", nfail, n);
  else
    printf("G109 PASS (%d)\n", n);
  return nfail != 0;
}
