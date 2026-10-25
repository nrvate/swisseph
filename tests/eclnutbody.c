/* eclnutbody.c -- G52: sidereal time reads no unwritten nutation, and
 * SE_ECL_NUT is no body to rise, to have a sector, to be occulted or to be
 * a centre.
 *
 * (a) calc_nutation() writes nothing for a nutation model it does not
 * know, and swe_sidtime() passed it an uninitialised local pair and used
 * both elements: the sidereal time was stack garbage. A run shows it only
 * by luck, the slot holding what the calls before left (here the bits of a
 * pointer, a denormal): the Makefile compiles swephlib.c at -O3 with -Wall
 * -Wextra -Werror, where gcc rejects the read as maybe-uninitialized. The
 * run checks that the sidereal time under the unknown model 99 is the
 * same after two different dirtyings of the stack, and equal to
 * swe_sidtime0() with no nutation: 2 checks.
 *
 * (b) SE_ECL_NUT (-1) is swe_calc()'s obliquity and nutation, not a body.
 * swe_rise_trans() took them for a right ascension and declination and,
 * for a rise of the disc's edge, read pla_diam[-1]; the sectors of
 * swe_gauquelin_sector() were the same (methods 4 and 5 through
 * pla_diam[-1]); the occultation functions set every negative number
 * without a star to the Sun; swe_calc_pctr() answered the nutation as a
 * place from the Sun; swe_helio_cross() took the obliquity's speed for a
 * longitude's and searched on, past 5 s. At UT 2433282.5, Zurich, on
 * Moshier (swe_calc_pctr() on the Swiss files), with -1, -2 and -100:
 * swe_rise_trans() rising, setting and transiting,
 * swe_rise_trans_true_hor(), swe_gauquelin_sector() in each method, the
 * three occultation functions, swe_calc_pctr() as the body and as the
 * centre, and swe_helio_cross() and its _ut form (each in a child process
 * with a 5 s alarm): ERR and "illegal planet number" (the crossings: "not
 * possible"). 12 calls with each of the three numbers, 36 checks. A star
 * asked with ipl -1 is the star, as with ipl 0, in swe_rise_trans() and
 * the occultation of the Moon, and the Sun in the occultations still
 * answers the eclipse of 1999 Aug 11: 3 checks.
 *
 * Run as
 *   eclnutbody EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

static int n, bad;

static void check(int ok, const char *what, const char *detail)
{
  n++;
  if (!ok) {
    bad++;
    printf("    %s: %s FAIL\n", what, detail);
  }
}

/* writes over the stack the next call is about to use */
static double __attribute__((noinline)) dirty(double v)
{
  volatile double a[1024];
  int i;
  for (i = 0; i < 1024; i++)
    a[i] = v;
  return a[1023];
}

static void sidtime_unknown_model(const char *ephe)
{
  const double T = 2451545.0;
  swe_ctx *c = swe_ctx_new();
  double s1, s2, s0;
  char what[64];
  swe_set_ephe_path_r(c, (char *) ephe);
  swe_set_astro_models_r(c, "0,0,0,99,0,0,0,0", 0);
  (void) dirty(1e5);
  s1 = swe_sidtime_r(c, T);
  (void) dirty(-3e4);
  s2 = swe_sidtime_r(c, T);
  s0 = swe_sidtime0_r(c, T, 23.4392911, 0.0);
  snprintf(what, sizeof what, "%.12g, %.12g", s1, s2);
  check(s1 == s2, "sidereal time under an unknown nutation model, the stack dirtied twice", what);
  snprintf(what, sizeof what, "%.12g, no nutation %.12g", s1, s0);
  check(fabs(s1 - s0) < 1e-12, "sidereal time under an unknown nutation model is that of no nutation", what);
  swe_ctx_free(c);
}

#define NB 8
static const char *BN[NB] = { "rise", "set", "transit", "true_hor rise", "sector",
  "occultation where", "occultation when_glob", "occultation when_loc" };

/* body k of the refused numbers: the message, and whether it returned ERR */
static int refused(swe_ctx *c, int k, int ipl, char *serr)
{
  const double T = 2433282.5;
  double geo[3] = { 8.55, 47.37, 400 }, tr[10], at[20];
  int32 f = SEFLG_MOSEPH;
  *serr = '\0';
  switch (k) {
  case 0: return swe_rise_trans_r(c, T, ipl, NULL, f, SE_CALC_RISE, geo, 1013.25, 10, tr, serr) == ERR;
  case 1: return swe_rise_trans_r(c, T, ipl, NULL, f, SE_CALC_SET | SE_BIT_DISC_CENTER, geo, 1013.25, 10, tr, serr) == ERR;
  case 2: return swe_rise_trans_r(c, T, ipl, NULL, f, SE_CALC_MTRANSIT, geo, 1013.25, 10, tr, serr) == ERR;
  case 3: return swe_rise_trans_true_hor_r(c, T, ipl, NULL, f, SE_CALC_RISE, geo, 1013.25, 10, 0, tr, serr) == ERR;
  case 4: {
      int m, all = 1;
      for (m = 0; m < 6; m++) {
        double sect;
        all &= swe_gauquelin_sector_r(c, T, ipl, NULL, f, m, geo, 1013.25, 10, &sect, serr) == ERR;
      }
      return all;
    }
  case 5: return swe_lun_occult_where_r(c, T, ipl, NULL, f, tr, at, serr) == ERR;
  case 6: return swe_lun_occult_when_glob_r(c, T, ipl, NULL, f, 0, tr, 0, serr) == ERR;
  default: return swe_lun_occult_when_loc_r(c, T, ipl, NULL, f, geo, tr, at, 0, serr) == ERR;
  }
}

/* the calls that must be tried in a child process: swe_calc_pctr() and the
 * crossings; 0 for refused with a message, else the code */
static int child_refused(int k, int ipl, const char *ephe)
{
  int fd[2], st;
  pid_t pid;
  char serr[AS_MAXCH], got[AS_MAXCH];
  ssize_t r;
  if (pipe(fd) != 0)
    return 1;
  fflush(stdout);
  pid = fork();
  if (pid < 0)
    return 1;
  if (pid == 0) {
    swe_ctx *c = swe_ctx_new();
    double x[6], jd = 7;
    int32 ret = 0;
    close(fd[0]);
    alarm(5);
    swe_set_ephe_path_r(c, (char *) ephe);
    *serr = '\0';
    switch (k) {
    case 0: ret = swe_calc_pctr_r(c, 2433282.5, ipl, SE_SUN, SEFLG_SWIEPH, x, serr); break;
    case 1: ret = swe_calc_pctr_r(c, 2433282.5, SE_MARS, ipl, SEFLG_SWIEPH, x, serr); break;
    case 2: ret = swe_helio_cross_r(c, ipl, 100, 2433282.5, SEFLG_MOSEPH, 0, &jd, serr); break;
    default: ret = swe_helio_cross_ut_r(c, ipl, 100, 2433282.5, SEFLG_MOSEPH, 0, &jd, serr); break;
    }
    memset(got, 0, sizeof got);
    if (ret == ERR)
      strcpy(got, serr);
    else
      strcpy(got, "answered");
    _exit(write(fd[1], got, sizeof got) == (ssize_t) sizeof got ? 0 : 4);
  }
  close(fd[1]);
  r = read(fd[0], got, sizeof got);
  close(fd[0]);
  if (waitpid(pid, &st, 0) != pid || !WIFEXITED(st) || WEXITSTATUS(st) != 0
      || r != (ssize_t) sizeof got)
    return 2;		/* died, or did not end */
  return (strstr(got, "illegal planet number") != NULL
          || strstr(got, "not possible") != NULL) ? 0 : 3;
}

int main(int argc, char **argv)
{
  static const int NUM[3] = { SE_ECL_NUT, -2, -100 };
  static const char *CN[4] = { "swe_calc_pctr() body", "swe_calc_pctr() centre",
    "swe_helio_cross()", "swe_helio_cross_ut()" };
  const char *ephe;
  swe_ctx *c;
  char serr[AS_MAXCH], what[128];
  int i, k;
  if (argc < 2) {
    fprintf(stderr, "usage: eclnutbody EPHEDIR\n");
    return 2;
  }
  ephe = argv[1];
  sidtime_unknown_model(ephe);
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, (char *) ephe);
  for (i = 0; i < 3; i++) {
    for (k = 0; k < NB; k++) {
      int ok = refused(c, k, NUM[i], serr);
      snprintf(what, sizeof what, "%s with %d", BN[k], NUM[i]);
      check(ok && strstr(serr, "illegal planet number") != NULL, what, serr);
    }
    for (k = 0; k < 4; k++) {
      int r = child_refused(k, NUM[i], ephe);
      snprintf(what, sizeof what, "%s with %d", CN[k], NUM[i]);
      check(r == 0, what, r == 2 ? "died or did not end" : r == 3 ? "no refusal" : "failed");
    }
  }
  /* a star asked with ipl -1 is the star, as with ipl 0 */
  {
    double geo[3] = { 8.55, 47.37, 400 }, ta[10], tb[10], ga[20], gb[20], aa[20], ab[20];
    char star[AS_MAXCH];
    int32 ra, rb;
    char sa[AS_MAXCH], sb[AS_MAXCH];
    strcpy(star, "Spica");
    ra = swe_rise_trans_r(c, 2433282.5, -1, star, SEFLG_MOSEPH, SE_CALC_RISE, geo, 1013.25, 10, ta, sa);
    strcpy(star, "Spica");
    rb = swe_rise_trans_r(c, 2433282.5, 0, star, SEFLG_MOSEPH, SE_CALC_RISE, geo, 1013.25, 10, tb, sb);
    check(ra == OK && ra == rb && ta[0] == tb[0], "swe_rise_trans() of a star with ipl -1", sa);
    strcpy(star, "Regulus");
    ra = swe_lun_occult_where_r(c, 2433282.5, -1, star, SEFLG_MOSEPH, ga, aa, sa);
    strcpy(star, "Regulus");
    rb = swe_lun_occult_where_r(c, 2433282.5, 0, star, SEFLG_MOSEPH, gb, ab, sb);
    check(ra != ERR && ra == rb && ga[0] == gb[0] && ga[1] == gb[1] && strcmp(sa, sb) == 0,
          "swe_lun_occult_where() of a star with ipl -1", sa);
    ra = swe_lun_occult_where_r(c, 2451401.96, SE_SUN, NULL, SEFLG_MOSEPH, ga, aa, sa);
    check(ra > 0 && (ra & SE_ECL_TOTAL), "the Sun in swe_lun_occult_where(), the eclipse of 1999 Aug 11", sa);
  }
  swe_ctx_free(c);
  if (bad)
    printf("G52 FAIL: %d of %d\n", bad, n);
  else
    printf("G52 PASS (%d)\n", n);
  return bad ? 1 : 0;
}
