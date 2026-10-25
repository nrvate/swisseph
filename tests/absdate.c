/* absdate.c -- G160: a date that is no number, infinite, or far past every
 * range is refused, with ERR, its outputs 0 and a message that fits serr;
 * inside the ranges nothing changes.
 *
 * The defects (UPSTREAM-BUGS 210, 211, 216):
 * - 210: the refusals of a date outside a range wrote "jd %f outside ..."
 *   with sprintf(), the date's digits in full, into 256 bytes: the Moshier
 *   planets', Moon's, mean node's and apogee's into a local buffer, the JPL
 *   file's and swe_nod_aps()'s mean node straight into serr (360 characters
 *   at 1e300), from |J| about 1e188 (Moshier) to 1e196 (JPL); glibc's
 *   fortify aborted ("buffer overflow detected"), at -O0 the stack
 *   protector. swe_utc_to_jd()'s "invalid time" took the caller's seconds
 *   the same way, from 1e232.
 * - 211: a NaN date passed every "J < START || J > END": the mean node and
 *   apogee, and the Moon's mean points by swe_nod_aps(), SIGSEGV in
 *   corr_mean_node()'s table; the Moshier planets, Moon, fictitious bodies
 *   and stars, the interpolated apsides, swe_nod_aps() and
 *   swe_gauquelin_sector() answered NaN with their flags; the crossings
 *   looped; the eclipse searches cast it to int and refused with a date no
 *   caller gave ("jd 6634702579155901.000000 outside ..."); at -inf
 *   swe_gauquelin_sector(), swe_rise_trans() and the heliacal functions
 *   went on into this fork's sidereal time, which SIGSEGV there;
 *   swe_topo_arcus_visionis() and swe_heliacal_angle() answered OK; on the
 *   JPL file NaN became a record number.
 * - 216: on the Swiss files the file's name came from swe_revjul()'s year
 *   cast to int, which overflows past |J| about 7.84e11: at 7.9e11, 1e12,
 *   1e300, -+inf and NaN "SwissEph file 'seplm21474840.se1' not found".
 *
 * Check (a), 210: at |J| 1e200, 1e250, 1e300, 1e308 and DBL_MAX, either
 * sign, Mars, the Moon, the mean node, the mean apogee and Cupido on
 * Moshier, swe_nod_aps() of the Moon's and Mars's mean points, Spica by
 * swe_fixstar2() on Moshier and Mars on de200.eph; and swe_utc_to_jd()
 * with those seconds. Each in a child process: ERR, the outputs 0, serr
 * from "jd " (or "invalid time") under AS_MAXCH, and the bytes of the
 * caller's buffer past AS_MAXCH as they were. 100 calls.
 *
 * Check (b), 211: at NaN, +inf and -inf, each date-taking entry point --
 * swe_calc() of fourteen bodies on Moshier, the Swiss files and de200.eph,
 * swe_calc_ut(), the star functions, swe_nod_aps() and its _ut form,
 * swe_pheno() and _ut, swe_calc_orbel() and _ut, swe_calc_pctr(),
 * swe_get_orbital_elements(), swe_orbit_max_min_true_distance(), the six
 * Gauquelin methods, the four eclipse searches, the two occultation
 * searches, rise and set, the six crossings and the two heliocentric
 * ones, and the five heliacal functions -- in a child process with a 10 s
 * alarm: ERR (the crossings that return a date return ERR), a message, and
 * the outputs 0 (the three functions that leave the caller's outputs on any
 * refusal excepted: swe_calc_pctr(), swe_get_orbital_elements(),
 * swe_orbit_max_min_true_distance()). 267 calls.
 *
 * Check (c), 216: at -+7.9e11, -+1e12, -+1e300, -+DBL_MAX, the doubles
 * next past -+7.8e11, NaN and -+inf, Mars, the Moon, Ceres, Jupiter's
 * centre (9599) and swe_calc_ut() of Mars on the Swiss files, Spica by
 * swe_fixstar2() and Jupiter's mean points by swe_nod_aps(), fresh and
 * after J2000 on the same context: ERR, the outputs 0, serr "outside the
 * range the Swiss Ephemeris files are named for", no file named; and at
 * -+7.8e11 itself still the file named for it ("sepl_21355626.se1 not
 * found"). 194 calls (swe_calc_ut() not where its TT falls inside).
 *
 * Check (d), nothing behind: after (c)'s refusals and NaN on one context,
 * Mars and the Moon on the files and Moshier, the mean node and Spica at
 * J2000 and 2460000.5 against a fresh context, bit for bit.
 *
 * Measured: (a) 100 of 100, (b) 267 of 267, (c) 194 of 194, (d) the same.
 * Before the fix (a) 2 of 100 (61 aborted, 37 overran serr), (b) 168 of
 * 267 (17 crashed, 12 hung, 70 answered or refused without a message or
 * with outputs left), (c) 24 of 194 (the file named), (d) crashed (the NaN
 * mean node). The upstream build, which keeps the memory and UB fixes
 * (210's, the mean node's and the JPL file's NaN guards, 216's) and none of
 * the NaN refusals: (a) 80 of 100, (b) 162 of 267 (12 hung: the
 * occultation searches and the crossings at NaN, three at -inf), (c) 164 of
 * 194, (d) the same; its swe_nod_aps() leaves the caller's points on a
 * refusal (entry 179), the rest of (a) and (c). Run as
 *   absdate EPHEDIR
 */
#include <float.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

static const char *EPHE;
static double T;	/* the date of the call in the child */

static swe_ctx *fresh(void)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    _exit(3);
  swe_set_ephe_path_r(c, (char *) EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  return c;
}

static int zero(const double *x, int n)
{
  int i;
  for (i = 0; i < n; i++)
    if (x[i] != 0)
      return 0;
  return 1;
}

/* the caller's buffer: AS_MAXCH the API allows, and a canary after it */
static char SERR[4 * AS_MAXCH];
static void serr_arm(void) { memset(SERR, 0x5a, sizeof SERR); SERR[0] = '\0'; }
static int serr_kept(void)
{
  int i;
  for (i = AS_MAXCH; i < (int) sizeof SERR; i++)
    if (SERR[i] != 0x5a)
      return 0;
  return 1;
}

/* ---- (a) 210 ---- */
#define NA 10
static int probe_a(int k)
{
  swe_ctx *c = fresh();
  double x[6], b[6], p[6], q[6];
  char star[AS_MAXCH];
  int32 r;
  int i;
  for (i = 0; i < 6; i++)
    x[i] = 7;
  serr_arm();
  switch (k) {
  case 0: r = swe_calc_r(c, T, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 1: r = swe_calc_r(c, T, SE_MOON, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 2: r = swe_calc_r(c, T, SE_MEAN_NODE, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 3: r = swe_calc_r(c, T, SE_MEAN_APOG, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 4: r = swe_calc_r(c, T, SE_CUPIDO, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 5: r = swe_nod_aps_r(c, T, SE_MOON, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  case 6: r = swe_nod_aps_r(c, T, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  case 7: strcpy(star, "Spica"); r = swe_fixstar2_r(c, star, T, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 8: r = swe_calc_r(c, T, SE_MARS, SEFLG_JPLEPH | SEFLG_SPEED, x, SERR); break;
  default: {
      double d[2] = { 7, 7 };
      r = swe_utc_to_jd_r(c, 2000, 1, 1, 0, 0, T, SE_GREG_CAL, d, SERR);
      return !(r == ERR && serr_kept() && strlen(SERR) < AS_MAXCH
               && strncmp(SERR, "invalid time", 12) == 0);
    }
  }
  return !(r == ERR && zero(x, 6) && serr_kept() && strlen(SERR) < AS_MAXCH
           && strncmp(SERR, "jd ", 3) == 0);
}

/* ---- (b) 211 ---- */
static const int B_BODY[14] = { SE_SUN, SE_MOON, SE_MARS, SE_MEAN_NODE, SE_MEAN_APOG,
  SE_TRUE_NODE, SE_OSCU_APOG, SE_INTP_APOG, SE_INTP_PERG, SE_CUPIDO, SE_CHIRON,
  SE_PHOLUS, SE_CERES, SE_EARTH };
static const int32 B_EPH[3] = { SEFLG_MOSEPH, SEFLG_SWIEPH, SEFLG_JPLEPH };
#define NB_CALC 42
#define NB (NB_CALC + 47)
static int probe_b(int k)
{
  swe_ctx *c = fresh();
  double x[50], b[6], p[6], q[6], tr[10], at[20], dret[50];
  double geo[3] = { 10, 50, 100 }, datm[4] = { 1013.25, 15, 40, 0 }, dobs[6] = { 36, 1, 0, 1, 0, 0 };
  double el[6] = { 19.4, 1.5237, 0.0934, 286.5, 49.56, 1.85 };
  char star[AS_MAXCH], obj[AS_MAXCH];
  int32 r;
  int i, nz = 6;	/* outputs to be 0, of x[] */
  for (i = 0; i < 50; i++)
    x[i] = dret[i] = 7;
  for (i = 0; i < 10; i++)
    tr[i] = 7;
  for (i = 0; i < 20; i++)
    at[i] = 7;
  strcpy(star, "Spica");
  strcpy(obj, "Venus");
  serr_arm();
  swe_set_topo_r(c, 10, 50, 100);
  if (k < NB_CALC) {
    int32 fl = B_EPH[k % 3] | SEFLG_SPEED | (B_BODY[k / 3] == SE_EARTH ? SEFLG_HELCTR : 0);
    r = swe_calc_r(c, T, B_BODY[k / 3], fl, x, SERR);
  } else switch (k - NB_CALC) {
  case 0: r = swe_calc_ut_r(c, T, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 1: r = swe_calc_ut_r(c, T, SE_MARS, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, SERR); break;
  case 2: r = swe_fixstar2_r(c, star, T, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 3: r = swe_fixstar2_r(c, star, T, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); break;
  case 4: r = swe_fixstar2_r(c, star, T, SEFLG_JPLEPH | SEFLG_SPEED, x, SERR); break;
  case 5: r = swe_fixstar_r(c, star, T, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 6: r = swe_fixstar2_ut_r(c, star, T, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 7: r = swe_nod_aps_r(c, T, SE_MOON, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  case 8: r = swe_nod_aps_r(c, T, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  case 9: r = swe_nod_aps_r(c, T, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  case 10: r = swe_nod_aps_r(c, T, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_OSCU, x, b, p, q, SERR); break;
  case 11: r = swe_nod_aps_r(c, T, SE_JUPITER, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  case 12: r = swe_nod_aps_ut_r(c, T, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  case 13: r = swe_pheno_r(c, T, SE_MARS, SEFLG_MOSEPH, x, SERR); nz = 20; break;
  case 14: r = swe_pheno_ut_r(c, T, SE_MOON, SEFLG_MOSEPH, x, SERR); nz = 20; break;
  case 15: r = swe_calc_orbel_r(c, T, el, SE_SUN, 0, 0, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  case 16: r = swe_calc_orbel_ut_r(c, T, el, SE_SUN, 0, 0, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR); break;
  /* these three leave the caller's outputs on any refusal */
  case 17: r = swe_calc_pctr_r(c, T, SE_MARS, SE_JUPITER, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); nz = 0; break;
  case 18: r = swe_get_orbital_elements_r(c, T, SE_MARS, SEFLG_MOSEPH, x, SERR); nz = 0; break;
  case 19: r = swe_orbit_max_min_true_distance_r(c, T, SE_MARS, SEFLG_MOSEPH, &x[0], &x[1], &x[2], SERR); nz = 0; break;
  case 20: case 21: case 22: case 23: case 24: case 25:
    r = swe_gauquelin_sector_r(c, T, SE_SUN, NULL, SEFLG_MOSEPH, k - NB_CALC - 20, geo, 1013.25, 10, x, SERR);
    nz = 1; break;
  case 26: r = swe_sol_eclipse_when_glob_r(c, T, SEFLG_MOSEPH, 0, tr, 0, SERR); memcpy(x, tr, sizeof tr); nz = 10; break;
  case 27: r = swe_sol_eclipse_when_loc_r(c, T, SEFLG_MOSEPH, geo, tr, at, 0, SERR); memcpy(x, tr, sizeof tr); nz = 10; break;
  case 28: r = swe_lun_eclipse_when_r(c, T, SEFLG_MOSEPH, 0, tr, 0, SERR); memcpy(x, tr, sizeof tr); nz = 10; break;
  case 29: r = swe_lun_eclipse_when_loc_r(c, T, SEFLG_MOSEPH, geo, tr, at, 0, SERR); memcpy(x, tr, sizeof tr); nz = 10; break;
  case 30: r = swe_lun_occult_when_glob_r(c, T, SE_VENUS, NULL, SEFLG_MOSEPH, 0, tr, 0, SERR); memcpy(x, tr, sizeof tr); nz = 10; break;
  case 31: r = swe_lun_occult_when_loc_r(c, T, SE_VENUS, NULL, SEFLG_MOSEPH, geo, tr, at, 0, SERR); memcpy(x, tr, sizeof tr); nz = 10; break;
  case 32: r = swe_rise_trans_r(c, T, SE_SUN, NULL, SEFLG_MOSEPH, SE_CALC_RISE, geo, 1013.25, 10, x, SERR); nz = 1; break;
  case 33: r = swe_rise_trans_true_hor_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH, SE_CALC_SET, geo, 1013.25, 10, 0, x, SERR); nz = 1; break;
  case 34: x[0] = swe_solcross_r(c, 0, T, SEFLG_MOSEPH, SERR); r = x[0] == ERR ? ERR : 0; nz = 0; break;
  case 35: x[0] = swe_solcross_ut_r(c, 0, T, SEFLG_MOSEPH, SERR); r = x[0] == ERR ? ERR : 0; nz = 0; break;
  case 36: x[0] = swe_mooncross_r(c, 0, T, SEFLG_MOSEPH, SERR); r = x[0] == ERR ? ERR : 0; nz = 0; break;
  case 37: x[0] = swe_mooncross_ut_r(c, 0, T, SEFLG_SWIEPH, SERR); r = x[0] == ERR ? ERR : 0; nz = 0; break;
  case 38: x[0] = swe_mooncross_node_r(c, T, SEFLG_MOSEPH, &x[1], &x[2], SERR); r = x[0] == ERR ? ERR : 0; x[0] = 0; nz = 3; break;
  case 39: x[0] = swe_mooncross_node_ut_r(c, T, SEFLG_MOSEPH, &x[1], &x[2], SERR); r = x[0] == ERR ? ERR : 0; x[0] = 0; nz = 3; break;
  case 40: r = swe_helio_cross_r(c, SE_MARS, 0, T, SEFLG_MOSEPH, 0, x, SERR); nz = 1; break;
  case 41: r = swe_helio_cross_ut_r(c, SE_MARS, 0, T, SEFLG_MOSEPH, -1, x, SERR); nz = 1; break;
  case 42: r = swe_heliacal_ut_r(c, T, geo, datm, dobs, obj, SE_HELIACAL_RISING, SEFLG_MOSEPH, dret, SERR); memcpy(x, dret, 3 * sizeof(double)); nz = 3; break;
  case 43: r = swe_heliacal_pheno_ut_r(c, T, geo, datm, dobs, obj, SE_HELIACAL_RISING, SEFLG_MOSEPH, dret, SERR); memcpy(x, dret, 31 * sizeof(double)); nz = 31; break;
  case 44: r = swe_vis_limit_mag_r(c, T, geo, datm, dobs, obj, SEFLG_MOSEPH, dret, SERR); memcpy(x, dret, 7 * sizeof(double)); nz = 7; break;
  case 45: r = swe_topo_arcus_visionis_r(c, T, geo, datm, dobs, SEFLG_MOSEPH, -4, 100, 5, 280, 0, -10, dret, SERR); x[0] = dret[0]; nz = 1; break;
  default: r = swe_heliacal_angle_r(c, T, geo, datm, dobs, SEFLG_MOSEPH, -4, 100, 280, 0, -10, dret, SERR); memcpy(x, dret, 3 * sizeof(double)); nz = 3; break;
  }
  return !(r == ERR && SERR[0] != '\0' && strlen(SERR) < AS_MAXCH && serr_kept() && zero(x, nz));
}

/* ---- (c) 216 ---- */
#define NC 7
static int probe_c(int k, int warm)
{
  swe_ctx *c = fresh();
  double x[6], b[6], p[6], q[6];
  char star[AS_MAXCH];
  int32 r;
  int i;
  if (warm) {
    swe_calc_r(c, 2451545.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR);
    swe_calc_r(c, 2451545.0, SE_CERES, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR);
    swe_calc_r(c, 2451545.0, 9599, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR);
  }
  for (i = 0; i < 6; i++)
    x[i] = 7;
  strcpy(star, "Spica");
  serr_arm();
  switch (k) {
  case 0: r = swe_calc_r(c, T, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); break;
  case 1: r = swe_calc_r(c, T, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); break;
  case 2: r = swe_calc_r(c, T, SE_CERES, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); break;
  case 3: r = swe_calc_r(c, T, 9599, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); break;
  case 4: r = swe_calc_ut_r(c, T, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); break;
  case 5: r = swe_fixstar2_r(c, star, T, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); break;
  default: r = swe_nod_aps_r(c, T, SE_JUPITER, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  }
  /* the UT call past 1e12 or non-finite: delta t's own refusal (bug 215)
   * comes first */
  if (isnan(T) || fabs(T) > 7.8e11 || k == 4)
    return !(r == ERR && zero(x, 6) && serr_kept()
             && (strstr(SERR, "outside the range the Swiss Ephemeris files are named for") != NULL
                 || (k == 4 && !(fabs(T) <= 1e12)
                     && strstr(SERR, "outside the delta t range") != NULL)));
  /* the bound itself: refused by the files as before, Mars by the file
   * named for it */
  return !(r == ERR && zero(x, 6) && SERR[0] != '\0'
           && strstr(SERR, "named for") == NULL
           && (k != 0 || strstr(SERR, T > 0 ? "'sepl_21355626.se1' not found" : "'seplm21355284.se1' not found") != NULL));
}

/* ---- (d) nothing left behind ---- */
static int probe_d(void)
{
  static const int body[3] = { SE_MARS, SE_MOON, SE_MEAN_NODE };
  static const double far[5] = { 7.9e11, -1e12, 1e300, -DBL_MAX, 0 };
  swe_ctx *w = fresh(), *f;
  double x[6], y[6];
  char star[AS_MAXCH];
  int i, j, e, nd = 0;
  for (i = 0; i < 5; i++) {
    double t = i < 4 ? far[i] : NAN;
    for (j = 0; j < 3; j++)
      for (e = 0; e < 2; e++)
        swe_calc_r(w, t, body[j], (e ? SEFLG_SWIEPH : SEFLG_MOSEPH) | SEFLG_SPEED, x, SERR);
    strcpy(star, "Spica");
    swe_fixstar2_r(w, star, t, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR);
  }
  f = fresh();
  for (i = 0; i < 2; i++) {
    double t = i ? 2460000.5 : 2451545.0;
    for (j = 0; j < 3; j++)
      for (e = 0; e < 2; e++) {
        int32 fl = (e ? SEFLG_SWIEPH : SEFLG_MOSEPH) | SEFLG_SPEED;
        swe_calc_r(w, t, body[j], fl, x, SERR);
        swe_calc_r(f, t, body[j], fl, y, SERR);
        nd += memcmp(x, y, sizeof x) != 0;
      }
    strcpy(star, "Spica");
    swe_fixstar2_r(w, star, t, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR);
    strcpy(star, "Spica");
    swe_fixstar2_r(f, star, t, SEFLG_SWIEPH | SEFLG_SPEED, y, SERR);
    nd += memcmp(x, y, sizeof x) != 0;
  }
  return nd;
}

/* one probe in a child: 0 passed, 1 failed, 2 crashed, 3 hung */
static int child(int part, int k, int warm, double t)
{
  int st = 0;
  pid_t pid;
  fflush(stdout);
  if ((pid = fork()) == 0) {
    int r;
    alarm(10);
    T = t;
    r = part == 0 ? probe_a(k) : part == 1 ? probe_b(k) : part == 2 ? probe_c(k, warm) : probe_d() != 0;
    _exit(r);
  }
  if (pid < 0 || waitpid(pid, &st, 0) != pid)
    return 2;
  if (WIFSIGNALED(st))
    return WTERMSIG(st) == SIGALRM ? 3 : 2;
  return WEXITSTATUS(st) != 0;
}

int main(int argc, char **argv)
{
  static const double da[5] = { 1e200, 1e250, 1e300, 1e308, DBL_MAX };
  double db[3] = { NAN, INFINITY, -INFINITY };
  double dc[15];
  int i, k, s, w, res, na = 0, ta = 0, nb = 0, tb = 0, nc = 0, tc = 0, nd, crash = 0, hang = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: absdate EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  setvbuf(stdout, NULL, _IOLBF, 0);
  for (i = 0; i < 5; i++)
    for (s = -1; s <= 1; s += 2)
      for (k = 0; k < NA; k++, ta++) {
        res = child(0, k, 0, s * da[i]);
        if (res == 0)
          na++;
        else {
          crash += res == 2; hang += res == 3;
          printf("  (a) call %d at %g: %s\n", k, s * da[i], res == 2 ? "crashed" : res == 3 ? "hung" : "not refused as it should be");
        }
      }
  for (i = 0; i < 3; i++)
    for (k = 0; k < NB; k++, tb++) {
      res = child(1, k, 0, db[i]);
      if (res == 0)
        nb++;
      else {
        crash += res == 2; hang += res == 3;
        printf("  (b) call %d at %g: %s\n", k, db[i], res == 2 ? "crashed" : res == 3 ? "hung" : "not refused as it should be");
      }
    }
  {
    static const double far[4] = { 7.9e11, 1e12, 1e300, DBL_MAX };
    int n = 0;
    for (i = 0; i < 4; i++) {
      dc[n++] = far[i];
      dc[n++] = -far[i];
    }
    dc[n++] = nextafter(7.8e11, HUGE_VAL);
    dc[n++] = nextafter(-7.8e11, -HUGE_VAL);
    dc[n++] = NAN;
    dc[n++] = INFINITY;
    dc[n++] = -INFINITY;
    dc[n++] = 7.8e11;
    dc[n++] = -7.8e11;
    for (i = 0; i < n; i++)
      for (w = 0; w < 2; w++)
        for (k = 0; k < NC; k++) {
          if (k == 4 && (fabs(dc[i]) == 7.8e11 || dc[i] < 0))
            continue;	/* UT: TT past the bound, or (delta t) inside it */
          tc++;
          res = child(2, k, w, dc[i]);
          if (res == 0)
            nc++;
          else {
            crash += res == 2; hang += res == 3;
            printf("  (c) call %d at %.17g%s: %s\n", k, dc[i], w ? " warm" : "", res == 2 ? "crashed" : res == 3 ? "hung" : "not refused as it should be");
          }
        }
  }
  nd = child(3, 0, 0, 0);
  printf("G160 (a) refusal messages within serr past 1e200: %d of %d\n", na, ta);
  printf("G160 (b) NaN and -+inf refused with ERR, outputs 0: %d of %d\n", nb, tb);
  printf("G160 (c) Swiss files past -+7.8e11, NaN, -+inf refused before a name: %d of %d\n", nc, tc);
  printf("G160 (d) after the refusals, J2000 and 2460000.5 as a fresh context: %s\n",
         nd == 0 ? "same" : nd == 2 ? "crashed" : nd == 3 ? "hung" : "differ");
  if (crash || hang)
    printf("G160 %d crashed, %d hung\n", crash, hang);
  k = (ta - na) + (tb - nb) + (tc - nc) + (nd != 0);
  if (k)
    printf("G160 FAIL: %d\n", k);
  else
    printf("G160 PASS\n");
  return k != 0;
}
