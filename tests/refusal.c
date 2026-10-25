/* refusal.c -- G174: a refusal is ERR with the outputs 0 and a message that
 * names the caller's own input and the reason.
 *
 * The defects (UPSTREAM-BUGS 239, 240, 241, 242, 244, 248):
 * - 239: on a refusal swe_calc() zeroes the place; swe_calc_pctr(),
 *   swe_get_orbital_elements(), swe_orbit_max_min_true_distance(),
 *   swe_gauquelin_sector(), the eclipse and occultation functions, the
 *   crossings that return a place or a date by pointer, the heliacal
 *   functions, swe_utc_to_jd() and, refusing the site, rise and set
 *   (swe_rise_trans_true_hor(), swe_rise_trans() of a star) left the
 *   caller's outputs as they were, or as far as the call had filled them
 *   (swe_sol_eclipse_how()'s attributes, the local searches' times,
 *   swe_heliacal_ut()'s dates), on every refusal: a date's, a body's, a
 *   method's, a site's, an event's, the strict setting's.
 * - 240: an eclipse search from a start no ephemeris answers refused with
 *   the range message of the first possible eclipse's date, from the
 *   lunation number's polynomial: "jd 10000160.328309" for a start at 1e7,
 *   "jd 5314742918287432" at 6e10, "jd 6634702579155901" from
 *   1e11 to DBL_MAX (the lunation number held at -2^31).
 * - 241: swe_heliacal_ut() from such a start: "no heliacal date found
 *   within 5 synodic periods" from about -+1e100, nearer the range message
 *   of a date the search had reached ("jd 10000331.937800" at 1e7);
 *   swe_heliacal_angle() and swe_topo_arcus_visionis() answered OK at any
 *   finite date (6.2214 deg at 1e300), from a calendar date swe_revjul()
 *   has no year for past |J| 7.84e11.
 * - 242: sweph()'s file-range refusal called the planet file "asteroid eph.
 *   file (sepl_168.se1)" for every planet, the Moon and the main asteroids
 *   (the wording went by the body's index, and the Sun's barycentre, 10,
 *   is past Pluto's).
 * - 244: a JPL request falling to the Swiss files with the JPL file missing
 *   lost main_planet()'s " \ntrying Swiss Eph; " where sweph() opened a
 *   Swiss file (swe_calc() of Mars fresh, serr ""), and swe_pheno() lost it
 *   always; other calls kept it (SEFLG_SPEED3, swe_calc_pctr()).
 * - 248: the barycentric Sun's fall from a missing JPL file kept
 *   SEFLG_JPLEPH; with SEFLG_SPEED this fork's barycentric speed (bug 78)
 *   asked the JPL reader with no file: SIGSEGV; without it the return flag
 *   said JPL (17921 for 17922), and the strict setting answered.
 *
 * Check (a), 239: 64 refusals -- of each of those functions, each reason,
 * at 1e7 days where a date is refused, and by the strict setting with
 * de999.eph -- each in a child with a 20 s alarm, the outputs set to 7:
 * ERR, a message, the outputs 0 (of the eclipse and occultation places
 * geopos[0..1], the two doubles they write).
 *
 * Check (b), 240: the six eclipse and occultation searches, Moshier and
 * the Swiss files, forward and backward, at -+1e7, -+6.2e10, 1e12, 1e13,
 * -1e100, 1e200, DBL_MAX and -DBL_MAX: ERR, tret[] and attr[] 0, and serr
 * the message the caller's own date gets, swe_calc()'s of the Sun there
 * (inside delta t's range) or swe_calc_ut()'s (past it). 240 calls. And
 * eight starts 50 days outside Moshier's range whose search reaches inside
 * it answer the eclipse the search from the range's end answers.
 *
 * Check (c), 241: swe_heliacal_ut() of Venus, Moshier and the Swiss files,
 * at -+1e7, 6.2e10, 1e12, 1e13, 1e100, -1e200, 1e300 and -+DBL_MAX:
 * as (b); swe_heliacal_pheno_ut() and swe_vis_limit_mag() at 1e13 and
 * 1e300 likewise; swe_heliacal_angle() and swe_topo_arcus_visionis() at
 * -+7.9e11, -+1e12, -+1e300, -+DBL_MAX: ERR, the output 0, "outside the
 * range the calendar date is defined for"; the two at 1e7 and -+7.8e11
 * answered; swe_heliacal_ut() on the Swiss files from 100 days before
 * their start answered inside them. 47 calls.
 *
 * Check (d), 242: nine bodies on the Swiss files at 8000000.5 and
 * -3100000 (past sepl_168.se1's and before seplm132.se1's ranges):
 * "planets eph. file (sepl_168.se1): jd " ("(seplm132.se1)"); the true node
 * and the osculating apogee "moon eph. file (semo_168.se1): jd "
 * ("(semom132.se1)"). 22 calls.
 *
 * Check (e), 244: 19 kinds of call (swe_calc() of eight bodies and flag
 * sets, swe_calc_ut(), swe_nod_aps_ut(), swe_calc_pctr(), swe_calc_orbel(),
 * the star functions, swe_pheno() and _ut) asking de999.eph with the fall
 * allowed, fresh and after a fallen call and a Swiss call on the context:
 * the return flag and every output of the Swiss request, and serr
 * main_planet()'s note, "'de999.eph' not found ... \ntrying Swiss Eph; ".
 * 38 calls.
 *
 * Check (f), 248: the barycentric Sun with SEFLG_SPEED, without, with
 * SEFLG_SPEED3 and by swe_calc_ut(), as (e), fresh and warm; with the fall
 * not allowed: ERR, the place 0, "JPL ephemeris is not available". 12
 * calls.
 *
 * Measured: (a) 64 of 64, (b) 240 of 240 and 8 of 8, (c) 47 of 47, (d) 22
 * of 22, (e) 38 of 38, (f) 12 of 12. Before the fix: (a) 12 of 64 (the
 * refusals that zeroed already), (b) 28 of 240 (swe_lun_occult_when_glob()
 * inside delta t's range, which named the caller's date, and the Swiss
 * global eclipse searches at -+1e7, whose file the first eclipse's date
 * shares) and 8 of 8, (c) 9 of 47, (d) 4 of 22 (the Moon file's), (e) 20 of
 * 38 (16 fresh calls and swe_pheno() warm without the note), (f) 0 of 12 (6
 * crashed, the strict setting's two among them). The upstream build fails
 * likewise: (a) 12 of 64, (b) 48 of 240 and 8 of 8, (c) 7 of 47, (d) 4 of
 * 22, (e) 4 of 38 (warm calls too without the note, the stars' and
 * swe_calc_pctr()'s flags JPL's), (f) 0 of 12 (the flag JPL's, 17921, and the
 * strict setting answering). Run as
 *   refusal EPHEDIR
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
static int32 E;		/* its ephemeris */
static int BACK;	/* its direction */

static swe_ctx *fresh(void)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    _exit(3);
  swe_set_ephe_path_r(c, (char *) EPHE);
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

static double O[64];	/* the outputs, set to 7 */
static char S[AS_MAXCH];

/* ---- (a) 239 ---- */
#define NA 64
static int probe_a(int k)
{
  swe_ctx *c = fresh();
  double geo[3] = { 8.55, 47.37, 400 }, hgeo[3] = { 8.55, 47.37, 50000 };
  double datm[4] = { 1013.25, 15, 40, 0 }, dobs[6] = { 36, 1, 0, 1, 0, 0 };
  double el[6] = { 19.4, 1.5237, 0.0934, 286.5, 49.56, 1.85 };
  char st[AS_MAXCH], ob[AS_MAXCH];
  double t = 1e7, j = 2451545, *o = O;
  int32 M = SEFLG_MOSEPH, J = SEFLG_JPLEPH, r;
  int i, n, where = 0;
  for (i = 0; i < 64; i++)
    O[i] = 7;
  *S = '\0';
  strcpy(st, "Nosuchstarxx");
  strcpy(ob, "Venus");
  if (k >= 46 && k <= 61) {
    swe_set_jpl_file_r(c, "de999.eph");
    swe_set_ephe_fallback_r(c, 0);
  }
  switch (k) {
#define C(i, nn, expr) case i: n = nn; r = (expr); break
  C(0, 6, swe_calc_pctr_r(c, t, SE_MARS, SE_JUPITER, SEFLG_SWIEPH | SEFLG_SPEED, o, S));
  C(1, 6, swe_calc_pctr_r(c, j, SE_MARS, SE_MARS, M, o, S));
  C(2, 6, swe_calc_pctr_r(c, j, -5, SE_MARS, SEFLG_SWIEPH, o, S));
  C(3, 17, swe_get_orbital_elements_r(c, t, SE_MARS, M, o, S));
  C(4, 17, swe_get_orbital_elements_r(c, j, SE_SUN, M, o, S));
  C(5, 3, swe_orbit_max_min_true_distance_r(c, t, SE_MARS, M, o, o + 1, o + 2, S));
  C(6, 1, swe_gauquelin_sector_r(c, t, SE_MARS, NULL, M, 0, geo, 1013.25, 10, o, S));
  C(7, 1, swe_gauquelin_sector_r(c, j, SE_MARS, NULL, M, 9, geo, 1013.25, 10, o, S));
  C(8, 1, swe_gauquelin_sector_r(c, t, SE_MARS, NULL, M, 3, geo, 1013.25, 10, o, S));
  C(9, 30, (where = 1, swe_sol_eclipse_where_r(c, t, M, o, o + 10, S)));
  C(10, 30, (where = 1, swe_lun_occult_where_r(c, t, SE_VENUS, NULL, M, o, o + 10, S)));
  C(11, 30, (where = 1, swe_lun_occult_where_r(c, j, 0, st, M, o, o + 10, S)));
  C(12, 20, swe_sol_eclipse_how_r(c, t, M, geo, o, S));
  C(13, 20, swe_sol_eclipse_how_r(c, j, M, hgeo, o, S));
  C(14, 20, swe_lun_eclipse_how_r(c, t, M, geo, o, S));
  C(15, 20, swe_lun_eclipse_how_r(c, j, M, hgeo, o, S));
  C(16, 10, swe_sol_eclipse_when_glob_r(c, t, M, 0, o, 0, S));
  C(17, 10, swe_sol_eclipse_when_glob_r(c, j, M, SE_ECL_PARTIAL | SE_ECL_CENTRAL, o, 0, S));
  C(18, 30, swe_sol_eclipse_when_loc_r(c, t, M, geo, o, o + 10, 0, S));
  C(19, 30, swe_sol_eclipse_when_loc_r(c, j, M, hgeo, o, o + 10, 0, S));
  C(20, 10, swe_lun_eclipse_when_r(c, t, M, 0, o, 0, S));
  C(21, 10, swe_lun_eclipse_when_r(c, j, M, SE_ECL_ANNULAR, o, 0, S));
  C(22, 30, swe_lun_eclipse_when_loc_r(c, t, M, geo, o, o + 10, 0, S));
  C(23, 30, swe_lun_eclipse_when_loc_r(c, j, M, hgeo, o, o + 10, 0, S));
  C(24, 10, swe_lun_occult_when_glob_r(c, t, SE_VENUS, NULL, M, 0, o, 0, S));
  C(25, 10, swe_lun_occult_when_glob_r(c, j, 0, st, M, 0, o, 0, S));
  C(26, 30, swe_lun_occult_when_loc_r(c, t, SE_VENUS, NULL, M, geo, o, o + 10, 0, S));
  C(27, 30, swe_lun_occult_when_loc_r(c, j, SE_VENUS, NULL, M, hgeo, o, o + 10, 0, S));
  C(28, 2, swe_mooncross_node_r(c, t, M, o, o + 1, S) < t ? ERR : OK);
  C(29, 2, swe_mooncross_node_ut_r(c, t, M, o, o + 1, S) < t ? ERR : OK);
  C(30, 1, swe_helio_cross_r(c, SE_MARS, 0, t, M, 0, o, S));
  C(31, 1, swe_helio_cross_r(c, SE_SUN, 0, j, M, 0, o, S));
  C(32, 1, swe_helio_cross_ut_r(c, SE_MARS, 0, t, M, 0, o, S));
  C(33, 1, swe_helio_cross_ut_r(c, SE_MOON, 0, j, M, 0, o, S));
  C(34, 3, swe_heliacal_ut_r(c, t, geo, datm, dobs, ob, SE_HELIACAL_RISING, M, o, S));
  C(35, 3, swe_heliacal_ut_r(c, j, hgeo, datm, dobs, ob, SE_HELIACAL_RISING, M, o, S));
  C(36, 3, (strcpy(ob, "Sun"), swe_heliacal_ut_r(c, j, geo, datm, dobs, ob, SE_HELIACAL_RISING, M, o, S)));
  C(37, 3, (strcpy(ob, "Moon"), swe_heliacal_ut_r(c, j, geo, datm, dobs, ob, SE_MORNING_FIRST, M, o, S)));
  C(38, 3, (strcpy(ob, "Mars"), swe_heliacal_ut_r(c, j, geo, datm, dobs, ob, SE_EVENING_FIRST, M, o, S)));
  C(39, SE_HELIACAL_DARR_SIZE, swe_heliacal_pheno_ut_r(c, t, geo, datm, dobs, ob, SE_HELIACAL_RISING, M, o, S));
  C(40, SE_HELIACAL_DARR_SIZE, swe_heliacal_pheno_ut_r(c, j, hgeo, datm, dobs, ob, SE_HELIACAL_RISING, M, o, S));
  C(41, 7, swe_vis_limit_mag_r(c, t, geo, datm, dobs, ob, M, o, S));
  C(42, 3, swe_heliacal_angle_r(c, j, hgeo, datm, dobs, M, -4, 100, 280, 0, -10, o, S));
  C(43, 2, swe_utc_to_jd_r(c, 2000, 2, 30, 0, 0, 0, SE_GREG_CAL, o, S));
  C(44, 2, swe_utc_to_jd_r(c, 2000, 2, 3, 25, 0, 0, SE_GREG_CAL, o, S));
  C(45, 2, swe_utc_to_jd_r(c, 2000, 2, 3, 23, 59, 60.5, SE_GREG_CAL, o, S));
  /* the strict setting's refusal of de999.eph */
  C(46, 6, swe_calc_pctr_r(c, j, SE_MARS, SE_JUPITER, J, o, S));
  C(47, 17, swe_get_orbital_elements_r(c, j, SE_MARS, J, o, S));
  C(48, 3, swe_orbit_max_min_true_distance_r(c, j, SE_MARS, J, o, o + 1, o + 2, S));
  C(49, 30, (where = 1, swe_sol_eclipse_where_r(c, j, J, o, o + 10, S)));
  C(50, 1, swe_gauquelin_sector_r(c, j, SE_MARS, NULL, J, 0, geo, 1013.25, 10, o, S));
  C(51, 1, swe_helio_cross_r(c, SE_MARS, 0, j, J, 0, o, S));
  C(52, 2, swe_mooncross_node_r(c, j, J, o, o + 1, S) < j ? ERR : OK);
  C(53, 30, swe_sol_eclipse_when_loc_r(c, j, J, geo, o, o + 10, 0, S));
  C(54, SE_HELIACAL_DARR_SIZE, swe_heliacal_pheno_ut_r(c, j, geo, datm, dobs, ob, SE_HELIACAL_RISING, J, o, S));
  C(55, 30, (where = 1, swe_lun_occult_where_r(c, j, SE_VENUS, NULL, J, o, o + 10, S)));
  C(56, 20, swe_pheno_r(c, j, SE_MARS, J, o, S));
  C(57, 20, swe_lun_eclipse_how_r(c, j, J, geo, o, S));
  C(58, 10, swe_lun_occult_when_glob_r(c, j, SE_VENUS, NULL, J, 0, o, 0, S));
  C(59, 6, swe_calc_orbel_r(c, j, el, SE_SUN, 0, 0, J, o, S));
  C(60, 6, (strcpy(st, "Spica"), swe_fixstar2_r(c, st, j, J, o, S)));
  C(61, 1, swe_rise_trans_r(c, j, SE_SUN, NULL, J, SE_CALC_RISE, geo, 1013.25, 10, o, S));
  /* the site's refusal of rise and set */
  C(62, 1, swe_rise_trans_true_hor_r(c, j, SE_MOON, NULL, M, SE_CALC_SET, hgeo, 1013.25, 10, 0, o, S));
  default: n = 1; strcpy(st, "Spica"); r = swe_rise_trans_r(c, j, 0, st, M, SE_CALC_RISE, hgeo, 1013.25, 10, o, S); break;
#undef C
  }
  if (where)	/* geopos[0..1] and the 20 attributes */
    return !(r == ERR && *S != '\0' && zero(O, 2) && zero(O + 10, 20));
  return !(r == ERR && *S != '\0' && zero(O, n));
}

/* The message the caller's date gets: swe_calc()'s of the Sun there, or
 * past delta t's range swe_calc_ut()'s, on a fresh context. */
static void own_msg(double t, int32 eph, char *s)
{
  swe_ctx *c = fresh();
  double x[6];
  *s = '\0';
  if (swe_calc_ut_r(c, t, SE_SUN, eph, x, s) == ERR && strstr(s, "delta t range") != NULL)
    return;
  *s = '\0';
  swe_calc_r(c, t, SE_SUN, eph, x, s);
}

/* ---- (b) 240 ---- */
#define NB 6
static int probe_b(int k)
{
  swe_ctx *c = fresh();
  double geo[3] = { 8.55, 47.37, 400 }, tr[10], at[20];
  char own[AS_MAXCH];
  int32 r;
  int i;
  for (i = 0; i < 10; i++)
    tr[i] = 7;
  for (i = 0; i < 20; i++)
    at[i] = 7;
  *S = '\0';
  switch (k) {
  case 0: r = swe_sol_eclipse_when_glob_r(c, T, E, 0, tr, BACK, S); break;
  case 1: r = swe_sol_eclipse_when_loc_r(c, T, E, geo, tr, at, BACK, S); break;
  case 2: r = swe_lun_eclipse_when_r(c, T, E, 0, tr, BACK, S); break;
  case 3: r = swe_lun_eclipse_when_loc_r(c, T, E, geo, tr, at, BACK, S); break;
  case 4: r = swe_lun_occult_when_glob_r(c, T, SE_VENUS, NULL, E, 0, tr, BACK, S); break;
  default: r = swe_lun_occult_when_loc_r(c, T, SE_VENUS, NULL, E, geo, tr, at, BACK, S); break;
  }
  own_msg(T, E, own);
  return !(r == ERR && zero(tr, 10) && (k % 2 == 0 || zero(at, 20))
           && *own != '\0' && strcmp(S, own) == 0);
}

/* the near-edge starts: T outside the range, the search from the range's
 * end (start) answers the same eclipse */
static int probe_b2(int k, double start)
{
  double geo[3] = { 8.55, 47.37, 400 }, tr[2][10], at[20];
  int32 r[2];
  int i;
  for (i = 0; i < 2; i++) {
    swe_ctx *c = fresh();
    double t = i ? start : T;
    switch (k) {
    case 0: r[i] = swe_sol_eclipse_when_glob_r(c, t, SEFLG_MOSEPH, 0, tr[i], BACK, S); break;
    case 1: r[i] = swe_lun_eclipse_when_r(c, t, SEFLG_MOSEPH, 0, tr[i], BACK, S); break;
    case 2: r[i] = swe_sol_eclipse_when_loc_r(c, t, SEFLG_MOSEPH, geo, tr[i], at, BACK, S); break;
    default: r[i] = swe_lun_eclipse_when_loc_r(c, t, SEFLG_MOSEPH, geo, tr[i], at, BACK, S); break;
    }
  }
  return !(r[0] > 0 && r[0] == r[1] && tr[0][0] == tr[1][0]
           && tr[0][0] > 625000.5 && tr[0][0] < 2818000.5);
}

/* ---- (c) 241 ---- */
static int probe_c(int k)
{
  swe_ctx *c = fresh();
  double geo[3] = { 8.55, 47.37, 400 }, datm[4] = { 1013.25, 15, 40, 0 }, dobs[6] = { 36, 1, 0, 1, 0, 0 };
  double d[SE_HELIACAL_DARR_SIZE];
  char ob[AS_MAXCH], own[AS_MAXCH];
  int32 r;
  int i, n;
  for (i = 0; i < SE_HELIACAL_DARR_SIZE; i++)
    d[i] = 7;
  strcpy(ob, "Venus");
  *S = '\0';
  switch (k) {
  case 0: r = swe_heliacal_ut_r(c, T, geo, datm, dobs, ob, SE_HELIACAL_RISING, E, d, S); n = 3; break;
  case 1: r = swe_heliacal_pheno_ut_r(c, T, geo, datm, dobs, ob, SE_HELIACAL_RISING, E, d, S); n = SE_HELIACAL_DARR_SIZE; break;
  case 2: r = swe_vis_limit_mag_r(c, T, geo, datm, dobs, ob, E, d, S); n = 7; break;
  case 3: r = swe_heliacal_angle_r(c, T, geo, datm, dobs, E, -4, 100, 280, 0, -10, d, S); n = 3; break;
  case 4: r = swe_topo_arcus_visionis_r(c, T, geo, datm, dobs, E, -4, 100, 5, 280, 0, -10, d, S); n = 1; break;
  default:	/* from 100 days before the Swiss files' start */
    r = swe_heliacal_ut_r(c, T, geo, datm, dobs, ob, SE_HELIACAL_RISING, SEFLG_SWIEPH, d, S);
    return !(r >= 0 && d[0] > -3026605.471196 && d[0] < T + 2000);
  }
  if (k >= 3) {
    if (fabs(T) <= 7.8e11)	/* answered */
      return !(r >= 0 && isfinite(d[0]) && d[0] != 7 && d[0] != 0);
    return !(r == ERR && zero(d, n) && strstr(S, "outside the range the calendar date is defined for") != NULL);
  }
  own_msg(T, E, own);
  return !(r == ERR && zero(d, n) && *own != '\0' && strcmp(S, own) == 0);
}

/* ---- (d) 242 ---- */
static const int D_BODY[11] = { SE_SUN, SE_MOON, SE_MERCURY, SE_MARS, SE_JUPITER, SE_PLUTO,
  SE_EARTH, SE_CERES, SE_CUPIDO, SE_TRUE_NODE, SE_OSCU_APOG };
static int probe_d(int k)
{
  swe_ctx *c = fresh();
  double x[6];
  char want[64];
  int ipl = D_BODY[k];
  int32 r = swe_calc_r(c, T, ipl, SEFLG_SWIEPH | SEFLG_SPEED | (ipl == SE_EARTH ? SEFLG_HELCTR : 0), x, S);
  snprintf(want, sizeof want, "%s eph. file (%s.se1): jd ", k >= 9 ? "moon" : "planets",
           k >= 9 ? (T > 0 ? "semo_168" : "semom132") : (T > 0 ? "sepl_168" : "seplm132"));
  return !(r == ERR && zero(x, 6) && strncmp(S, want, strlen(want)) == 0);
}

/* ---- (e) 244, (f) 248 ---- */
static int32 call_e(swe_ctx *c, int k, int32 eph, double *x, char *serr)
{
  double el[6] = { 19.4, 1.5237, 0.0934, 286.5, 49.56, 1.85 }, t = 2451545.0;
  char star[AS_MAXCH];
  *serr = '\0';
  memset(x, 0, 30 * sizeof(double));
  strcpy(star, "Spica");
  switch (k) {
  case 0: return swe_calc_r(c, t, SE_MARS, eph, x, serr);
  case 1: return swe_calc_r(c, t, SE_MARS, eph | SEFLG_SPEED, x, serr);
  case 2: return swe_calc_r(c, t, SE_MARS, eph | SEFLG_SPEED3, x, serr);
  case 3: return swe_calc_r(c, t, SE_MOON, eph | SEFLG_SPEED, x, serr);
  case 4: return swe_calc_r(c, t, SE_SUN, eph | SEFLG_SPEED, x, serr);
  case 5: return swe_calc_r(c, t, SE_EARTH, eph | SEFLG_SPEED | SEFLG_HELCTR, x, serr);
  case 6: return swe_calc_r(c, t, SE_CERES, eph | SEFLG_SPEED, x, serr);
  case 7: return swe_calc_r(c, t, SE_TRUE_NODE, eph | SEFLG_SPEED, x, serr);
  case 8: return swe_calc_r(c, t, SE_CUPIDO, eph | SEFLG_SPEED, x, serr);
  case 9: return swe_calc_ut_r(c, t, SE_MARS, eph | SEFLG_SPEED, x, serr);
  case 10: return swe_nod_aps_ut_r(c, t, SE_MOON, eph | SEFLG_SPEED, SE_NODBIT_OSCU, x, x + 6, x + 12, x + 18, serr);
  case 11: return swe_calc_pctr_r(c, t, SE_MARS, SE_JUPITER, eph | SEFLG_SPEED, x, serr);
  case 12: return swe_calc_orbel_r(c, t, el, SE_SUN, 0, 0, eph | SEFLG_SPEED, x, serr);
  case 13: return swe_fixstar2_r(c, star, t, eph | SEFLG_SPEED, x, serr);
  case 14: return swe_fixstar_ut_r(c, star, t, eph | SEFLG_SPEED, x, serr);
  case 15: return swe_pheno_r(c, t, SE_MARS, eph, x, serr);
  case 16: return swe_pheno_ut_r(c, t, SE_MOON, eph, x, serr);
  case 17: return swe_calc_r(c, t, SE_MARS, eph | SEFLG_SPEED | SEFLG_BARYCTR, x, serr);
  case 18: return swe_calc_r(c, t, SE_MARS, eph | SEFLG_SPEED | SEFLG_HELCTR, x, serr);
  /* (f): the barycentric Sun */
  case 19: return swe_calc_r(c, t, SE_SUN, eph | SEFLG_SPEED | SEFLG_BARYCTR, x, serr);
  case 20: return swe_calc_r(c, t, SE_SUN, eph | SEFLG_BARYCTR, x, serr);
  case 21: return swe_calc_r(c, t, SE_SUN, eph | SEFLG_SPEED3 | SEFLG_BARYCTR, x, serr);
  default: return swe_calc_ut_r(c, t, SE_SUN, eph | SEFLG_SPEED | SEFLG_BARYCTR, x, serr);
  }
}
#define NE 19
#define NF 4
static int probe_e(int k, int mode)	/* mode 0 fresh, 1 warm, 2 strict */
{
  static const char note[] = " \ntrying Swiss Eph; ";
  swe_ctx *c = fresh(), *s = fresh();
  double x[30], y[30], z[6];
  char s2[AS_MAXCH];
  int32 r, rs;
  size_t n;
  swe_set_jpl_file_r(c, "de999.eph");
  swe_set_ephe_fallback_r(c, mode < 2);
  if (mode == 1) {
    call_e(c, k, SEFLG_JPLEPH, x, S);
    swe_calc_r(c, 2451545.0, SE_JUPITER, SEFLG_SWIEPH | SEFLG_SPEED, z, S);
  }
  r = call_e(c, k, SEFLG_JPLEPH, x, S);
  if (mode == 2)
    return !(r == ERR && zero(x, 6) && strncmp(S, "JPL ephemeris is not available", 30) == 0);
  rs = call_e(s, k, SEFLG_SWIEPH, y, s2);
  n = strlen(S);
  return !(r == rs && memcmp(x, y, sizeof x) == 0 && strstr(S, "'de999.eph' not found") != NULL
           && n >= sizeof note - 1 && strcmp(S + n - (sizeof note - 1), note) == 0);
}

/* one probe in a child: 0 passed, 1 failed, 2 crashed, 3 hung */
static int child(int part, int k, int m, double t)
{
  int st = 0;
  pid_t pid;
  fflush(stdout);
  if ((pid = fork()) == 0) {
    int r;
    alarm(20);
    T = t;
    switch (part) {
    case 0: r = probe_a(k); break;
    case 1: r = probe_b(k); break;
    case 2: r = probe_b2(k, BACK ? 2818000.5 : 625000.5); break;
    case 3: r = probe_c(k); break;
    case 4: r = probe_d(k); break;
    default: r = probe_e(k, m); break;
    }
    _exit(r);
  }
  if (pid < 0 || waitpid(pid, &st, 0) != pid)
    return 2;
  if (WIFSIGNALED(st))
    return WTERMSIG(st) == SIGALRM ? 3 : 2;
  return WEXITSTATUS(st) != 0;
}

static int crash, hang;
static int tally(int res, const char *what, int k, double t)
{
  if (res == 0)
    return 1;
  crash += res == 2;
  hang += res == 3;
  printf("  %s call %d at %.17g: %s\n", what, k, t,
         res == 2 ? "crashed" : res == 3 ? "hung" : "not as it should be");
  return 0;
}

int main(int argc, char **argv)
{
  static const double db[10] = { 1e7, -1e7, 6.2e10, -6.2e10, 1e12, 1e13, -1e100, 1e200, DBL_MAX, -DBL_MAX };
  static const double dc[10] = { 1e7, -1e7, 6.2e10, 1e12, 1e13, 1e100, -1e200, 1e300, DBL_MAX, -DBL_MAX };
  static const double dca[8] = { 7.9e11, -7.9e11, 1e12, -1e12, 1e300, -1e300, DBL_MAX, -DBL_MAX };
  static const double dcok[3] = { 1e7, 7.8e11, -7.8e11 };
  static const int32 eph[2] = { SEFLG_MOSEPH, SEFLG_SWIEPH };
  int i, k, e, m, na = 0, ta = 0, nb = 0, tb = 0, nb2 = 0, tb2 = 0, nc = 0, tc = 0, nd = 0, td = 0;
  int ne = 0, te = 0, nf = 0, tf = 0, bad;
  if (argc != 2) {
    fprintf(stderr, "usage: refusal EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  setvbuf(stdout, NULL, _IOLBF, 0);
  for (k = 0; k < NA; k++, ta++)
    na += tally(child(0, k, 0, 0), "(a)", k, 1e7);
  for (k = 0; k < NB; k++)
    for (e = 0; e < 2; e++)
      for (BACK = 0; BACK < 2; BACK++)
        for (i = 0; i < 10; i++, tb++) {
          E = eph[e];
          nb += tally(child(1, k, 0, db[i]), E == SEFLG_MOSEPH ? (BACK ? "(b) Moshier back" : "(b) Moshier") : (BACK ? "(b) Swiss back" : "(b) Swiss"), k, db[i]);
        }
  for (k = 0; k < 4; k++)
    for (BACK = 0; BACK < 2; BACK++, tb2++)
      nb2 += tally(child(2, k, 0, BACK ? 2818050.5 : 624950.5), "(b) near the edge", k, BACK ? 2818050.5 : 624950.5);
  BACK = 0;
  for (e = 0; e < 2; e++) {
    E = eph[e];
    for (i = 0; i < 10; i++, tc++)
      nc += tally(child(3, 0, 0, dc[i]), e ? "(c) heliacal_ut Swiss" : "(c) heliacal_ut Moshier", 0, dc[i]);
  }
  E = SEFLG_MOSEPH;
  for (k = 1; k <= 2; k++)
    for (i = 0; i < 2; i++, tc++)
      nc += tally(child(3, k, 0, i ? 1e300 : 1e13), "(c) pheno, vis_limit_mag", k, i ? 1e300 : 1e13);
  for (k = 3; k <= 4; k++) {
    for (i = 0; i < 8; i++, tc++)
      nc += tally(child(3, k, 0, dca[i]), "(c) angle, arcus", k, dca[i]);
    for (i = 0; i < 3; i++, tc++)
      nc += tally(child(3, k, 0, dcok[i]), "(c) angle, arcus answered", k, dcok[i]);
  }
  tc++;
  nc += tally(child(3, 5, 0, -3026705.5), "(c) heliacal_ut Swiss start", 5, -3026705.5);
  for (k = 0; k < 11; k++)
    for (i = 0; i < 2; i++, td++)
      nd += tally(child(4, k, 0, i ? -3100000.0 : 8000000.5), "(d)", k, i ? -3100000.0 : 8000000.5);
  for (k = 0; k < NE; k++)
    for (m = 0; m < 2; m++, te++)
      ne += tally(child(5, k, m, 0), m ? "(e) warm" : "(e) fresh", k, 2451545.0);
  for (k = NE; k < NE + NF; k++)
    for (m = 0; m < 3; m++, tf++)
      nf += tally(child(5, k, m, 0), m == 2 ? "(f) strict" : m ? "(f) warm" : "(f) fresh", k, 2451545.0);
  printf("G174 (a) refusals with ERR, a message, the outputs 0: %d of %d\n", na, ta);
  printf("G174 (b) eclipse and occultation searches from far starts refused at the caller's date: %d of %d; near the edge as before: %d of %d\n", nb, tb, nb2, tb2);
  printf("G174 (c) heliacal functions from far dates refused at the caller's date: %d of %d\n", nc, tc);
  printf("G174 (d) the Swiss files' range refusals name the file's kind: %d of %d\n", nd, td);
  printf("G174 (e) a JPL request fallen to the Swiss files: the Swiss answer and the note: %d of %d\n", ne, te);
  printf("G174 (f) the barycentric Sun fallen likewise, strict refused: %d of %d\n", nf, tf);
  if (crash || hang)
    printf("G174 %d crashed, %d hung\n", crash, hang);
  bad = (ta - na) + (tb - nb) + (tb2 - nb2) + (tc - nc) + (td - nd) + (te - ne) + (tf - nf);
  if (bad)
    printf("G174 FAIL: %d\n", bad);
  else
    printf("G174 PASS\n");
  return bad != 0;
}
