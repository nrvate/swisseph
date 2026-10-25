/* absdate.c -- G160: a date that is no number, infinite, or far past every
 * range is refused, with ERR, its outputs 0 and a message that fits serr
 * and names it; where there is no error to return, no answer; inside the
 * ranges nothing changes.
 *
 * The defects (UPSTREAM-BUGS 210, 211, 216, 237, 238, 243, 255):
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
 * - 237: swe_lmt_to_lat() and swe_lat_to_lmt() past 1e12, at NaN and -+inf
 *   and at a NaN, infinite or huge longitude: ERR from swe_time_equ() (bug
 *   215), E 0, and the output the date plus 0: NaN, -+inf, 1e300.
 * - 238: swe_utc_to_jd() OK at NaN seconds, both dates NaN;
 *   swe_date_conversion() OK of year, month and day INT_MIN at a NaN hour;
 *   swe_utc_time_zone() at NaN or -+inf seconds or time zone the hour and
 *   minute (int) of NaN, the date a real one; swe_jdet_to_utc() and
 *   swe_jdut1_to_utc() at NaN and -+inf the same casts, past 1e12 a time
 *   ("INT_MIN INT_MIN 0 12:00:00" at 1e300), and where the year holds in
 *   no int, from TT -6.19e11 and +9.14e11, INT_MIN with a month, day and
 *   time, with the Julian calendar the year -2147439551.
 * - 243: the refusals' "jd %f" (and swe_utc_to_jd()'s seconds) cut at 255
 *   characters, the range lost from about 1e188, the text from 1e237;
 *   the "%.10g" of this fork's bounds printed the double next past
 *   -+7.8e11 or -+1e12 as the bound.
 * - 255: Moshier's planets and Moon take 0.3 and 0.2 day past their
 *   ranges' ends, and their refusals named the ends: Mars at 2818000.79
 *   answered, at 2818000.81 "outside Moshier planet range 625000.50 ..
 *   2818000.50".
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
 * the outputs 0 (swe_calc_pctr(), swe_get_orbital_elements() and
 * swe_orbit_max_min_true_distance() too since bug 239). 267 calls.
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
 * Check (e), 237: swe_time_equ(), swe_lmt_to_lat() and swe_lat_to_lmt()
 * at 10 E at NaN, -+inf, -+1e300, -+DBL_MAX, -+2e12 and the doubles next
 * past -+1e12, and the two at J2000 at longitude NaN, -+inf and 1e300:
 * ERR, the output 0, serr "outside the delta t range" with the date the
 * equation is taken at (LMT less the longitude), read back; at -+1e12 at
 * 0 E, inside, as before: ERR from the files, the output the date. 47
 * calls.
 *
 * Check (f), 238: swe_utc_to_jd() at NaN and -+inf seconds, either
 * calendar: ERR, both dates 0, "invalid time: 12:0:nan" ("inf", "-inf");
 * swe_date_conversion() at those hours, of 2000 Jan 1 and of INT_MIN
 * INT_MIN INT_MIN: ERR, the date 0; swe_utc_time_zone() at those seconds
 * and time zones, swe_jdet_to_utc() and swe_jdut1_to_utc(), either
 * calendar, at those dates, -+1e300, -+DBL_MAX, -+2e12 and the doubles
 * next past -+1e12, and where the year holds in no int (TT 1e12, -7e11,
 * 9.2e11, UT 7.9e11, -8e11, 1e12): every int INT_MIN, the seconds NaN;
 * swe_julday() at those hours NaN, -+inf, and swe_revjul() at those dates
 * INT_MIN and NaN, as they were; inside, TT 9e11 and -6e11, UT 7.5e11 and
 * -7.9e11, answered. 75 calls.
 *
 * Check (g), 243 and 255: at |J| 1e188, 1e200, 1e300 and DBL_MAX either
 * sign, Mars, the Moon, the mean node and apogee and Cupido on Moshier,
 * Mars on de200.eph and swe_nod_aps() of the Moon's mean points: ERR, serr
 * under 100 characters with "outside" and "range" and the date, read back;
 * and swe_utc_to_jd() with those seconds; the doubles next past -+7.8e11 (Mars
 * on the files) and -+1e12 (SE_ECL_NUT, the houses, swe_time_equ(),
 * swe_deltat_ex(), swe_get_ayanamsa_ex()): the date read back, not the
 * bound; five messages of ordinary dates, the dates' digits as before
 * (the Moshier ranges the ones tested, 255); and, 255, Mars and the Moon
 * on Moshier 0.01 day inside and outside the ranges tested (Mars 0.05
 * inside at the start, its light time): the one inside answered, the
 * refusal's range holding it and not the other. 85 calls.
 *
 * Measured: (a) 100 of 100, (b) 267 of 267, (c) 194 of 194, (d) the same,
 * (e) 47 of 47, (f) 75 of 75, (g) 85 of 85. Before the fix of 237, 238,
 * 243 and 255: (e) 15 of 47, (f) 30 of 75 (x86-64's casts of NaN to
 * INT_MIN pass most of the NaN cases by chance), (g) 3 of 85.
 * Before the fix (a) 2 of 100 (61 aborted, 37 overran serr), (b) 161 of
 * 267 (17 crashed, 12 hung, 77 answered or refused without a message or
 * with outputs left), (c) 24 of 194 (the file named), (d) crashed (the NaN
 * mean node). The upstream build, which keeps the memory and UB fixes
 * (210's, the mean node's and the JPL file's NaN guards, 216's) and none of
 * the NaN refusals: (a) 80 of 100, (b) 155 of 267 (12 hung: the
 * occultation searches and the crossings at NaN, three at -inf), (c) 164 of
 * 194, (d) the same; its swe_nod_aps() leaves the caller's points on a
 * refusal (entry 179), the rest of (a) and (c); (e) 8 of 47, (f) 22 of
 * 75, (g) 4 of 85 (216's message, in both builds, prints its date). Run as
 *   absdate EPHEDIR
 */
#include <float.h>
#include <limits.h>
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
  /* these three zero their outputs on a refusal since bug 239 */
  case 17: r = swe_calc_pctr_r(c, T, SE_MARS, SE_JUPITER, SEFLG_SWIEPH | SEFLG_SPEED, x, SERR); break;
  case 18: r = swe_get_orbital_elements_r(c, T, SE_MARS, SEFLG_MOSEPH, x, SERR); nz = 17; break;
  case 19: r = swe_orbit_max_min_true_distance_r(c, T, SE_MARS, SEFLG_MOSEPH, &x[0], &x[1], &x[2], SERR); nz = 3; break;
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

/* the number a message prints after key, read back: the date it names */
static int reads_back(const char *s, const char *key, double t)
{
  const char *p = strstr(s, key);
  double v;
  if (p == NULL)
    return 0;
  v = strtod(p + strlen(key), NULL);
  return isnan(t) ? isnan(v) : v == t;
}

/* ---- (e) 237 ---- */
static double G;	/* the longitude of the call */
#define NE 3
static int probe_e(int k)
{
  swe_ctx *c = fresh();
  double o = 7, t0 = T - G / 360;
  int32 r;
  swe_set_jpl_file_r(c, "");
  serr_arm();
  switch (k) {
  case 0: r = swe_time_equ_r(c, T, &o, SERR); t0 = T; break;
  case 1: r = swe_lmt_to_lat_r(c, T, G, &o, SERR); break;
  default: r = swe_lat_to_lmt_r(c, T, G, &o, SERR); break;
  }
  if (!(t0 >= -1e12 && t0 <= 1e12))
    return !(r == ERR && o == 0 && serr_kept() && strstr(SERR, "outside the delta t range") != NULL
             && reads_back(SERR, "jd ", t0));
  /* the bound itself, refused by the files: as before, the date plus E 0 */
  return !(r == ERR && o == (k == 0 ? 0 : T) && SERR[0] != '\0'
           && strstr(SERR, "delta t range") == NULL);
}

/* ---- (f) 238 ---- */
static int none(const int32 *v, double s)
{
  int i;
  for (i = 0; i < 5; i++)
    if (v[i] != INT_MIN)
      return 0;
  return isnan(s);
}
#define NF 9
static int probe_f(int k)
{
  swe_ctx *c = fresh();
  int32 v[5] = { 7, 7, 7, 7, 7 }, r;
  int iv[3] = { 7, 7, 7 };
  double s = 7, d[2] = { 7, 7 }, j;
  serr_arm();
  switch (k) {
  case 0: case 1:	/* T the seconds, Gregorian and Julian */
    r = swe_utc_to_jd_r(c, 2000, 1, 1, 12, 0, T, k == 0 ? SE_GREG_CAL : SE_JUL_CAL, d, SERR);
    return !(r == ERR && d[0] == 0 && d[1] == 0 && serr_kept()
             && strcmp(SERR, isnan(T) ? "invalid time: 12:0:nan" : T > 0 ? "invalid time: 12:0:inf" : "invalid time: 12:0:-inf") == 0);
  case 2:	/* T the hour */
    j = 7;
    r = swe_date_conversion(2000, 1, 1, T, 'g', &j);
    if (!(r == ERR && j == 0))
      return 1;
    j = 7;	/* OK upstream: swe_revjul() of NaN is INT_MIN, INT_MIN, INT_MIN */
    r = swe_date_conversion(INT_MIN, INT_MIN, INT_MIN, T, 'g', &j);
    return !(r == ERR && j == 0);
  case 3:	/* T the seconds, then the time zone */
    swe_utc_time_zone(2000, 1, 1, 12, 0, T, 1, &v[0], &v[1], &v[2], &v[3], &v[4], &s);
    if (!none(v, s))
      return 1;
    swe_utc_time_zone(2000, 1, 1, 12, 0, 30, T, &v[0], &v[1], &v[2], &v[3], &v[4], &s);
    return !none(v, s);
  case 4: case 5:	/* T the TT, no answer */
    swe_jdet_to_utc_r(c, T, k == 4 ? SE_GREG_CAL : SE_JUL_CAL, &v[0], &v[1], &v[2], &v[3], &v[4], &s);
    return !none(v, s);
  case 6: case 7:	/* T the UT, no answer */
    swe_jdut1_to_utc_r(c, T, k == 6 ? SE_GREG_CAL : SE_JUL_CAL, &v[0], &v[1], &v[2], &v[3], &v[4], &s);
    return !none(v, s);
  default:	/* T the hour or the date: swe_julday() and swe_revjul() as they were */
    j = swe_julday(2000, 1, 1, T, SE_GREG_CAL);
    if (!(isnan(T) ? isnan(j) : j == T))
      return 1;
    j = swe_julday(2000, 1, 1, T, SE_JUL_CAL);
    if (!(isnan(T) ? isnan(j) : j == T))
      return 1;
    s = 7;
    swe_revjul(T, SE_GREG_CAL, &iv[0], &iv[1], &iv[2], &s);
    return !(iv[0] == INT_MIN && iv[1] == INT_MIN && iv[2] == INT_MIN && isnan(s));
  }
}

/* inside the bound with the year an int: answered as before */
static int probe_f_in(int k)
{
  swe_ctx *c = fresh();
  int32 v[5] = { 7, 7, 7, 7, 7 };
  double s = 7;
  if (k == 0)
    swe_jdet_to_utc_r(c, T, SE_GREG_CAL, &v[0], &v[1], &v[2], &v[3], &v[4], &s);
  else
    swe_jdut1_to_utc_r(c, T, SE_GREG_CAL, &v[0], &v[1], &v[2], &v[3], &v[4], &s);
  return !(v[0] != INT_MIN && v[3] >= 0 && v[3] < 24 && s >= 0 && s < 61);
}

/* ---- (g) 243 ---- */
#define NG 8
static int probe_g(int k)
{
  swe_ctx *c = fresh();
  double x[6], b[6], p[6], q[6], d[2];
  int32 r;
  serr_arm();
  switch (k) {
  case 0: r = swe_calc_r(c, T, SE_MARS, SEFLG_MOSEPH, x, SERR); break;
  case 1: r = swe_calc_r(c, T, SE_MOON, SEFLG_MOSEPH, x, SERR); break;
  case 2: r = swe_calc_r(c, T, SE_MEAN_NODE, SEFLG_MOSEPH, x, SERR); break;
  case 3: r = swe_calc_r(c, T, SE_MEAN_APOG, SEFLG_MOSEPH, x, SERR); break;
  case 4: r = swe_calc_r(c, T, SE_CUPIDO, SEFLG_MOSEPH, x, SERR); break;
  case 5: r = swe_calc_r(c, T, SE_MARS, SEFLG_JPLEPH, x, SERR); break;
  case 6: r = swe_nod_aps_r(c, T, SE_MOON, SEFLG_MOSEPH, SE_NODBIT_MEAN, x, b, p, q, SERR); break;
  default:
    r = swe_utc_to_jd_r(c, 2000, 1, 1, 12, 0, T, SE_GREG_CAL, d, SERR);
    return !(r == ERR && strlen(SERR) < 64 && reads_back(SERR, "invalid time: 12:0:", T));
  }
  return !(r == ERR && strlen(SERR) < 100 && strstr(SERR, "outside") != NULL
           && strstr(SERR, "range") != NULL && reads_back(SERR, "jd ", T));
}

/* the doubles next past the bounds: their date, not the bound's */
#define NGB 6
static int probe_g_bound(int k)
{
  swe_ctx *c = fresh();
  double x[6], cu[13], a[10], e;
  int32 r;
  serr_arm();
  switch (k) {
  case 0: r = swe_calc_r(c, T, SE_MARS, SEFLG_SWIEPH, x, SERR); break;	/* T past -+7.8e11 */
  case 1: r = swe_calc_r(c, T, SE_ECL_NUT, SEFLG_MOSEPH, x, SERR); break;	/* the rest past -+1e12 */
  case 2: r = swe_houses_ex2_r(c, T, 0, 50, 10, 'P', cu, a, NULL, NULL, SERR); break;
  case 3: r = swe_time_equ_r(c, T, &e, SERR); break;
  case 4: e = swe_deltat_ex_r(c, T, SEFLG_MOSEPH, SERR); r = isnan(e) ? ERR : OK; break;
  default: r = swe_get_ayanamsa_ex_r(c, T, SEFLG_MOSEPH, &e, SERR); break;
  }
  return !(r == ERR && strstr(SERR, "outside") != NULL && reads_back(SERR, "jd ", T));
}

/* messages of ordinary dates, byte for byte as before */
#define NGO 5
static int probe_g_same(int k)
{
  static const char *const want[NGO] = {
    "jd 100000000000000.000000 outside Moshier planet range 625000.20 .. 2818000.80 ",
    "jd -10000000.000000 outside Moshier's Moon range 625000.30 .. 2818000.70 ",
    "jd 7.9e+11 outside the range the Swiss Ephemeris files are named for, -7.8e+11 .. 7.8e+11",
    "jd 2e+12 outside the obliquity and nutation range -1e+12 .. 1e+12",
    "invalid time: 12:0:61.00",
  };
  swe_ctx *c = fresh();
  double x[6], d[2];
  serr_arm();
  switch (k) {
  case 0: swe_calc_r(c, 1e14, SE_MARS, SEFLG_MOSEPH, x, SERR); break;
  case 1: swe_calc_r(c, -1e7, SE_MOON, SEFLG_MOSEPH, x, SERR); break;
  case 2: swe_calc_r(c, 7.9e11, SE_MARS, SEFLG_SWIEPH, x, SERR); break;
  case 3: swe_calc_r(c, 2e12, SE_ECL_NUT, SEFLG_MOSEPH, x, SERR); break;
  default: swe_utc_to_jd_r(c, 2000, 1, 1, 12, 0, 61, SE_GREG_CAL, d, SERR); break;
  }
  return strcmp(SERR, want[k]) != 0;
}

/* 255: a date the Moshier test takes answers, one past it is refused,
 * and the range the refusal names holds the one and not the other */
#define NGM 4
static int probe_g_margin(int k)
{
  static const double in[NGM] = { 2818000.79, 625000.25, 2818000.69, 625000.31 };	/* Mars's light time 0.015 day */
  static const double out[NGM] = { 2818000.81, 625000.19, 2818000.71, 625000.29 };
  swe_ctx *c = fresh();
  double x[6], a, b;
  int ipl = k < 2 ? SE_MARS : SE_MOON;
  const char *p;
  if (swe_calc_r(c, in[k], ipl, SEFLG_MOSEPH, x, SERR) < 0)
    return 1;
  serr_arm();
  if (swe_calc_r(c, out[k], ipl, SEFLG_MOSEPH, x, SERR) != ERR
      || (p = strstr(SERR, "range ")) == NULL || sscanf(p + 6, "%lf .. %lf", &a, &b) != 2)
    return 1;
  return !(in[k] >= a && in[k] <= b && !(out[k] >= a && out[k] <= b));
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
    switch (part) {
    case 0: r = probe_a(k); break;
    case 1: r = probe_b(k); break;
    case 2: r = probe_c(k, warm); break;
    case 3: r = probe_d() != 0; break;
    case 4: r = probe_e(k); break;
    case 5: r = probe_f(k); break;
    case 6: r = probe_f_in(k); break;
    case 7: r = probe_g(k); break;
    case 8: r = probe_g_bound(k); break;
    case 9: r = probe_g_same(k); break;
    default: r = probe_g_margin(k); break;
    }
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
  int ne = 0, te = 0, nf = 0, tf = 0, ng = 0, tg = 0;
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
  /* (e) 237: the three at dates past the bound, NaN and -+inf, and at a
   * NaN, infinite or huge longitude; at the bound itself as before */
  {
    double de[13], ge[4] = { NAN, INFINITY, -INFINITY, 1e300 };
    int n = 0;
    de[n++] = NAN; de[n++] = INFINITY; de[n++] = -INFINITY;
    de[n++] = 1e300; de[n++] = -1e300; de[n++] = DBL_MAX; de[n++] = -DBL_MAX;
    de[n++] = 2e12; de[n++] = -2e12;
    de[n++] = nextafter(1e12, HUGE_VAL); de[n++] = nextafter(-1e12, -HUGE_VAL);
    de[n++] = 1e12; de[n++] = -1e12;
    for (i = 0; i < n + 4; i++)
      for (k = 0; k < NE; k++) {
        if (i >= n && k == 0)
          continue;	/* swe_time_equ() takes no longitude */
        G = i >= n ? ge[i - n] : fabs(de[i]) == 1e12 ? 0 : 10;
        te++;
        res = child(4, k, 0, i >= n ? 2451545.0 : de[i]);
        if (res == 0)
          ne++;
        else {
          crash += res == 2; hang += res == 3;
          printf("  (e) call %d at %.17g longitude %g: %s\n", k, i >= n ? 2451545.0 : de[i], G,
                 res == 2 ? "crashed" : res == 3 ? "hung" : "not refused as it should be");
        }
      }
  }
  /* (f) 238: swedate.c at NaN and -+inf in each double, past the bound
   * and where the year holds in no int; swe_julday() and swe_revjul() as
   * they were; inside the bound with the year an int, answered */
  {
    double df[11], tin[4] = { 9.0e11, -6.0e11, 7.5e11, -7.9e11 };
    double dyr[6] = { 1e12, -7e11, 9.2e11, 7.9e11, -8e11, 1e12 };
    int n = 0;
    df[n++] = NAN; df[n++] = INFINITY; df[n++] = -INFINITY;
    df[n++] = 1e300; df[n++] = -1e300; df[n++] = DBL_MAX; df[n++] = -DBL_MAX;
    df[n++] = nextafter(1e12, HUGE_VAL); df[n++] = nextafter(-1e12, -HUGE_VAL);
    df[n++] = 2e12; df[n++] = -2e12;
    for (i = 0; i < n; i++)
      for (k = 0; k < NF; k++) {
        if (i >= 3 && (k < 4 || k == 8))
          continue;	/* the seconds, hours and swe_julday() at NaN and -+inf */
        tf++;
        res = child(5, k, 0, df[i]);
        if (res == 0)
          nf++;
        else {
          crash += res == 2; hang += res == 3;
          printf("  (f) call %d at %.17g: %s\n", k, df[i], res == 2 ? "crashed" : res == 3 ? "hung" : "not refused as it should be");
        }
      }
    for (i = 0; i < 6; i++)	/* the year no int holds: TT, then UT */
      for (k = 0; k < 2; k++) {
        tf++;
        res = child(5, (i < 3 ? 4 : 6) + k, 0, dyr[i]);
        if (res == 0)
          nf++;
        else {
          crash += res == 2; hang += res == 3;
          printf("  (f) call %d at %.17g: %s\n", (i < 3 ? 4 : 6) + k, dyr[i], res == 2 ? "crashed" : res == 3 ? "hung" : "not refused as it should be");
        }
      }
    for (i = 0; i < 4; i++) {
      tf++;
      res = child(6, i / 2, 0, tin[i]);
      if (res == 0)
        nf++;
      else {
        crash += res == 2; hang += res == 3;
        printf("  (f) %s at %.17g: %s\n", i < 2 ? "TT" : "UT", tin[i], res == 2 ? "crashed" : res == 3 ? "hung" : "not answered as it should be");
      }
    }
  }
  /* (g) 243: the messages at |J| 1e188 to DBL_MAX, the doubles next past
   * the bounds, and ordinary dates' as before */
  {
    static const double dg[4] = { 1e188, 1e200, 1e300, DBL_MAX };
    double db2[4] = { nextafter(7.8e11, HUGE_VAL), nextafter(-7.8e11, -HUGE_VAL),
                      nextafter(1e12, HUGE_VAL), nextafter(-1e12, -HUGE_VAL) };
    for (i = 0; i < 4; i++)
      for (s = -1; s <= 1; s += 2)
        for (k = 0; k < NG; k++) {
          tg++;
          res = child(7, k, 0, s * dg[i]);
          if (res == 0)
            ng++;
          else {
            crash += res == 2; hang += res == 3;
            printf("  (g) call %d at %g: %s\n", k, s * dg[i], res == 2 ? "crashed" : res == 3 ? "hung" : "message unreadable");
          }
        }
    for (i = 0; i < 4; i++)
      for (k = i < 2 ? 0 : 1; k < (i < 2 ? 1 : NGB); k++) {
        tg++;
        res = child(8, k, 0, db2[i]);
        if (res == 0)
          ng++;
        else {
          crash += res == 2; hang += res == 3;
          printf("  (g) bound call %d at %.17g: %s\n", k, db2[i], res == 2 ? "crashed" : res == 3 ? "hung" : "the date not distinguishable from the bound");
        }
      }
    for (k = 0; k < NGO; k++) {
      tg++;
      res = child(9, k, 0, 0);
      if (res == 0)
        ng++;
      else {
        crash += res == 2; hang += res == 3;
        printf("  (g) ordinary message %d: %s\n", k, res == 2 ? "crashed" : res == 3 ? "hung" : "changed");
      }
    }
    for (k = 0; k < NGM; k++) {
      tg++;
      res = child(10, k, 0, 0);
      if (res == 0)
        ng++;
      else {
        crash += res == 2; hang += res == 3;
        printf("  (g) Moshier margin %d: %s\n", k, res == 2 ? "crashed" : res == 3 ? "hung" : "the range named not the one tested");
      }
    }
  }
  printf("G160 (a) refusal messages within serr past 1e200: %d of %d\n", na, ta);
  printf("G160 (b) NaN and -+inf refused with ERR, outputs 0: %d of %d\n", nb, tb);
  printf("G160 (c) Swiss files past -+7.8e11, NaN, -+inf refused before a name: %d of %d\n", nc, tc);
  printf("G160 (d) after the refusals, J2000 and 2460000.5 as a fresh context: %s\n",
         nd == 0 ? "same" : nd == 2 ? "crashed" : nd == 3 ? "hung" : "differ");
  printf("G160 (e) the time equation's LMT and LAT past the bound refused, output 0: %d of %d\n", ne, te);
  printf("G160 (f) swedate.c at NaN, -+inf, past the bound, a year no int holds: %d of %d\n", nf, tf);
  printf("G160 (g) refusal messages readable, their date not the bound's: %d of %d\n", ng, tg);
  if (crash || hang)
    printf("G160 %d crashed, %d hung\n", crash, hang);
  k = (ta - na) + (tb - nb) + (tc - nc) + (nd != 0) + (te - ne) + (tf - nf) + (tg - ng);
  if (k)
    printf("G160 FAIL: %d\n", k);
  else
    printf("G160 PASS\n");
  return k != 0;
}
