/* agree.c -- G177: a function answers as its siblings do: every function
 * that takes a site's height refuses one outside the bound the others
 * take, the two orbit functions take the same bodies, swe_heliacal_pheno_ut()
 * writes every slot of its array, and an eclipse or occultation search
 * finds the nearest event the ephemeris covers, from either end of it and
 * at any date.
 *
 * The defects (UPSTREAM-BUGS 277, 278, 279, 281):
 * - 277: the eclipse, occultation, heliacal and rise and set functions
 *   refuse a site below -500 or above 25000 m ("location for ... must be
 *   between -500 and 25000 m above sea"), but swe_rise_trans()'s fast
 *   method (rising and setting of the Sun to the true node within 60 deg
 *   of the equator) took no height test, the Sun's rising from Zurich at
 *   50000 m OK, 2451545.800726, and with it swe_gauquelin_sector()'s
 *   methods 2 to 5; swe_vis_limit_mag() (-3.5629) and
 *   swe_topo_arcus_visionis() (99 deg) had none at all; and the bound's
 *   test passed a NaN height: then a date's range message ("Swiss (.se1)
 *   ephemeris is not available for jd 2451545.0007") or OK with NaN
 *   (swe_vis_limit_mag() 1 and -nan, swe_heliacal_angle() 2.97 deg).
 * - 278: swe_orbit_max_min_true_distance() takes the Sun as the Earth's
 *   orbit (1.0166992, 0.9832941, 0.9833100 AU at J2000), as swe_nod_aps()
 *   gives the Sun's points from it, while swe_get_orbital_elements()
 *   refused it, "object 0 not valid".
 * - 279: swe_heliacal_pheno_ut() never wrote darr[28..30] on success, which
 *   kept what the caller had there.
 * - 281: the eclipse searches began at the lunation the start's days from
 *   J2000 times 12.3685 / 365.2425 give, 1.8e-5 a lunation short of their
 *   candidates' 29.530588853 days, 3.3 lunations late at sepl_168.se1's
 *   end: a backward search from 7929133.0 began 102 days after it and
 *   refused "jd 7929235.715052 > upper limit 7929133.500000" (on de200.eph
 *   the lunar searches likewise), and past about 4300 years from J2000 a
 *   forward search in the future (backward in the past) began after
 *   eclipses it then never saw (from 7927148.0 7927313.519468 for
 *   7927165.810722). The occultation searches took the start, a UT, as
 *   TT, and stepped back from a maximum 20 days plus delta t, past 7 days
 *   of it more than a lunar month: backward they missed an occultation
 *   within delta t before the start (Regulus's at 1048161.947536 from
 *   1048162.0 on Moshier) and, far out, every other one (Venus's at
 *   -3026582.474721 from -3026575.471, the search then refusing at the
 *   file's start; Regulus's at 7927587.969003 from 7927700).
 *
 * Check (a), 277: eighteen calls (swe_rise_trans() of the Sun rising, the
 * Moon setting, the Sun's transit; swe_rise_trans_true_hor();
 * swe_gauquelin_sector() methods 2 to 5; swe_vis_limit_mag(),
 * swe_topo_arcus_visionis(), swe_heliacal_angle(), swe_heliacal_pheno_ut(),
 * swe_heliacal_ut(); swe_sol_eclipse_how(), swe_lun_eclipse_how(),
 * swe_sol_eclipse_when_loc(), swe_lun_eclipse_when_loc(),
 * swe_lun_occult_when_loc()) from Zurich at 50000, 25000.5, -500.5, -1000
 * m, NaN, +inf and -inf: ERR, the bound's message, the outputs 0 (set to 7
 * before; swe_vis_limit_mag()'s seven); at 400, 25000 and -500 m
 * (swe_heliacal_ut() at 400 alone) answered, as are methods 0 and 1 of
 * swe_gauquelin_sector(), which read no height, at 50000. 126 refusals, 54
 * answers.
 *
 * Check (b), 278: the Sun's elements on the Swiss files at six dates from
 * -1000000.5 to 5000000.5 and on Moshier at four, a call of the Earth's
 * first (a fresh context's first answer of the Earth is another's by
 * 1.0e-11 AU): OK, the Earth's (the Earth-Moon barycentre's) bit for bit,
 * the argument and longitude of periapsis and the mean longitude turned
 * by 180 deg; the periapsis and apoapsis distances
 * swe_orbit_max_min_true_distance()'s of the Sun bit for bit; at J2000 the
 * longitude of periapsis within 1e-9 deg of swe_nod_aps()'s osculating
 * perigee of the Sun (J2000, no nutation, true positions; measured 3.9e-11
 * and 4.7e-11 deg; away from J2000 they part, the elements' periapsis
 * measured along the orbit). 10 checks.
 *
 * Check (c), 279: swe_heliacal_pheno_ut() of five objects, four events
 * (the Moon two), at three dates, the early answer of the evening first
 * and morning last of Mars and Sirius among them, the array set to 7:
 * answered, darr[28..30] 0. 54 calls.
 *
 * Check (d), 281: the six searches (the solar and lunar eclipses, global
 * and from Zurich, Venus's occultation, global and from Zurich) backward
 * from 0.5, 3, 10, 30 and 100 days before the end of the Swiss files, of
 * de200.eph and of Moshier, and forward from as far after their starts:
 * answered, inside the range on the start's side, and the search the other
 * way from just past the answer finds nothing nearer the start. 180
 * probes. And the nearest event likewise (a search towards the near end
 * may refuse at it) from 21 starts 97 days apart at each end of the Swiss
 * files, the solar and lunar eclipses global (and local from every other
 * start) both ways; Regulus's occultation backward from 12 starts at the
 * upper end, Venus's from 12 at the lower; Regulus's from 1048162.0 on
 * Moshier. 281 starts.
 *
 * Each probe in a child with a 30 s alarm. Measured: (a) 180 of 180, (b)
 * 10 of 10, (c) 54 of 54, (d) 180 of 180 and 281 of 281. Before the fix:
 * (a) 114 of 180 (the fast method's, Gauquelin methods 2 to 5's,
 * swe_vis_limit_mag()'s and swe_topo_arcus_visionis()'s 56 answers at
 * finite heights, the ten others' at NaN), (b) 0 of 10, (c) 0 of 54, (d)
 * 160 of 180 (the Swiss eclipse searches backward from within 30 days of
 * the end, de200's lunar from within 10) and 248 of 281. The upstream
 * build fails likewise: (a) 54 of 180 (its refusals keep the outputs,
 * entry 239), (b) 0 of 10, (c) 0 of 54, (d) 160 of 180 and 248 of 281.
 * Run as
 *   agree EPHEDIR
 */
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

static const char *EPHE;
static int32 EPH;	/* (d)'s ephemeris */

static swe_ctx *fresh(void)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    _exit(3);
  swe_set_ephe_path_r(c, (char *) EPHE);
  if (EPH == SEFLG_JPLEPH) {
    swe_set_jpl_file_r(c, "de200.eph");
    swe_set_ephe_fallback_r(c, 0);
  }
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

/* ---- (a) 277 ---- */
#define NA 17
static const double BADH[7] = { 50000, 25000.5, -500.5, -1000, NAN, INFINITY, -INFINITY };
static const double GOODH[3] = { 400, 25000, -500 };
static int probe_a(int k, double h, int good)
{
  swe_ctx *c = fresh();
  double geo[3] = { 8.55, 47.37, h };
  double datm[4] = { 1013.25, 15, 40, 0 }, dobs[6] = { 36, 1, 0, 1, 0, 0 };
  double o[64], j = 2451545.0;
  char s[AS_MAXCH], ob[AS_MAXCH];
  int32 r, M = SEFLG_SWIEPH;
  int i, n;
  for (i = 0; i < 64; i++)
    o[i] = 7;
  *s = '\0';
  strcpy(ob, "Venus");
  switch (k) {
  case 0: n = 1; r = swe_rise_trans_r(c, j, SE_SUN, NULL, M, SE_CALC_RISE, geo, 1013.25, 10, o, s); break;
  case 1: n = 1; r = swe_rise_trans_r(c, j, SE_MOON, NULL, M, SE_CALC_SET, geo, 1013.25, 10, o, s); break;
  case 2: n = 1; r = swe_rise_trans_r(c, j, SE_SUN, NULL, M, SE_CALC_MTRANSIT, geo, 1013.25, 10, o, s); break;
  case 3: n = 1; r = swe_rise_trans_true_hor_r(c, j, SE_SUN, NULL, M, SE_CALC_RISE, geo, 1013.25, 10, 0, o, s); break;
  case 4: case 5: case 6: case 7:
    n = 1; r = swe_gauquelin_sector_r(c, j, SE_MARS, NULL, M, k - 2, geo, 1013.25, 10, o, s); break;
  case 8: n = 7; r = swe_vis_limit_mag_r(c, j, geo, datm, dobs, ob, M, o, s); break;
  case 9: n = 1; r = swe_topo_arcus_visionis_r(c, j, geo, datm, dobs, M, -4, 100, 5, 280, 0, -10, o, s); break;
  case 10: n = 3; r = swe_heliacal_angle_r(c, j, geo, datm, dobs, M, -4, 100, 280, 0, -10, o, s); break;
  case 11: n = SE_HELIACAL_DARR_SIZE; r = swe_heliacal_pheno_ut_r(c, j, geo, datm, dobs, ob, SE_HELIACAL_RISING, M, o, s); break;
  case 12: n = 3; r = swe_heliacal_ut_r(c, j, geo, datm, dobs, ob, SE_HELIACAL_RISING, M, o, s); break;
  case 13: n = 20; r = swe_sol_eclipse_how_r(c, j, M, geo, o, s); break;
  case 14: n = 20; r = swe_lun_eclipse_how_r(c, j, M, geo, o, s); break;
  case 15: n = 30; r = swe_sol_eclipse_when_loc_r(c, j, M, geo, o, o + 10, 0, s); break;
  case 16: n = 30; r = swe_lun_eclipse_when_loc_r(c, j, M, geo, o, o + 10, 0, s); break;
  case 17: n = 30; r = swe_lun_occult_when_loc_r(c, j, SE_VENUS, NULL, M, geo, o, o + 10, 0, s); break;
  default: n = 1; r = swe_gauquelin_sector_r(c, j, SE_MARS, NULL, M, k - 18, geo, 1013.25, 10, o, s); break;
  }
  if (good)
    return !(r != ERR);
  return !(r == ERR && strstr(s, "must be between -500 and 25000 m above sea") != NULL && zero(o, n));
}

/* ---- (b) 278 ---- */
static int probe_b(double t, int32 eph)
{
  swe_ctx *c = fresh();
  double e[50], s[50], xn[6], xd[6], xp[6], xa[6], dmax, dmin, dtrue;
  char serr[AS_MAXCH];
  int k, ok;
  swe_get_orbital_elements_r(c, t, SE_EARTH, eph, e, serr);	/* warm */
  if (swe_get_orbital_elements_r(c, t, SE_SUN, eph, s, serr) == ERR)
    return 1;
  if (swe_get_orbital_elements_r(c, t, SE_EARTH, eph, e, serr) == ERR
      || swe_orbit_max_min_true_distance_r(c, t, SE_SUN, eph, &dmax, &dmin, &dtrue, serr) == ERR)
    return 1;
  for (k = 0, ok = 1; k < 17; k++)
    ok &= (k == 4 || k == 5 || k == 9) ? s[k] == swe_degnorm(e[k] + 180) : s[k] == e[k];
  ok &= s[15] == dmin && s[16] == dmax;
  if (t == 2451545.0) {
    if (swe_nod_aps_r(c, t, SE_SUN, eph | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS, SE_NODBIT_OSCU,
                      xn, xd, xp, xa, serr) == ERR)
      return 1;
    ok &= fabs(swe_difdeg2n(s[5], xp[0])) < 1e-9;
  }
  return !ok;
}

/* ---- (c) 279 ---- */
static int probe_c(int k)
{
  static const char *obj[5] = { "Venus", "Mars", "Mercury", "Sirius", "Moon" };
  static const int32 ev[4] = { SE_HELIACAL_RISING, SE_HELIACAL_SETTING, SE_EVENING_FIRST, SE_MORNING_LAST };
  static const double tj[3] = { 2451545.0, 2452000.3, 2460000.7 };
  swe_ctx *c = fresh();
  double geo[3] = { 8.55, 47.37, 400 }, datm[4] = { 1013.25, 15, 40, 0 }, dobs[6] = { 36, 1, 0, 1, 0, 0 };
  double d[SE_HELIACAL_DARR_SIZE];
  char s[AS_MAXCH], ob[AS_MAXCH];
  int i, o = k / 12, e = (k / 3) % 4;
  int32 r;
  for (i = 0; i < SE_HELIACAL_DARR_SIZE; i++)
    d[i] = 7;
  strcpy(ob, obj[o]);
  r = swe_heliacal_pheno_ut_r(c, tj[k % 3], geo, datm, dobs, ob, ev[e], SEFLG_SWIEPH, d, s);
  return !(r >= 0 && zero(d + 28, 3));
}

/* ---- (d) 281 ---- */
static int32 srch(swe_ctx *c, int k, double t, int back, double *tr)
{
  double geo[3] = { 8.55, 47.37, 400 }, at[20];
  char s[AS_MAXCH], st[AS_MAXCH];
  strcpy(st, "Regulus");
  memset(tr, 0, 10 * sizeof(double));
  switch (k) {
  case 0: return swe_sol_eclipse_when_glob_r(c, t, EPH, 0, tr, back, s);
  case 1: return swe_sol_eclipse_when_loc_r(c, t, EPH, geo, tr, at, back, s);
  case 2: return swe_lun_eclipse_when_r(c, t, EPH, 0, tr, back, s);
  case 3: return swe_lun_eclipse_when_loc_r(c, t, EPH, geo, tr, at, back, s);
  case 4: return swe_lun_occult_when_glob_r(c, t, SE_VENUS, NULL, EPH, 0, tr, back, s);
  case 5: return swe_lun_occult_when_loc_r(c, t, SE_VENUS, NULL, EPH, geo, tr, at, back, s);
  default: return swe_lun_occult_when_glob_r(c, t, 0, st, EPH, 0, tr, back, s);
  }
}
/* the search from S answers inside lo .. hi on S's side, and from just
 * past its answer the search the other way finds nothing nearer S; with
 * out, a search towards the near end may instead refuse at it */
static int probe_d(int k, double S, int back, double lo, double hi, int out)
{
  swe_ctx *c = fresh();
  double tr[10], x[10], a;
  int32 r;
  if ((r = srch(c, k, S, back, tr)) <= 0)
    return !(out && r == ERR);
  a = tr[0];
  if (!(a > lo && a < hi && (back ? a < S : a > S)))
    return 1;
  r = srch(c, k, back ? a + 0.5 : a - 0.5, !back, x);
  return r > 0 && (back ? x[0] < S : x[0] > S);
}

/* one probe in a child: 0 passed, 1 failed, 2 crashed, 3 hung */
static int child(int part, int k, double t, int m, double lo, double hi)
{
  int st = 0;
  pid_t pid;
  fflush(stdout);
  if ((pid = fork()) == 0) {
    int r;
    alarm(30);
    switch (part) {
    case 0: r = probe_a(k, t, m); break;
    case 1: EPH = m; r = probe_b(t, m); break;
    case 2: r = probe_c(k); break;
    default: r = probe_d(k, t, m & 1, lo, hi, m >> 1); break;
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
  printf("  %s %d at %.17g: %s\n", what, k, t,
         res == 2 ? "crashed" : res == 3 ? "hung" : "not as it should be");
  return 0;
}

int main(int argc, char **argv)
{
  static const double db_sw[6] = { 2451545.0, 2300000.5, 2600000.5, 1000000.5, 5000000.5, -1000000.5 };
  static const double dd[5] = { 0.5, 3, 10, 30, 100 };
  static const struct { int32 eph; const char *name; double lo, hi; } R[3] = {
    { SEFLG_SWIEPH, "Swiss", -3026605.471196, 7929133.5 },
    { SEFLG_JPLEPH, "de200", 2305424.5, 2513392.5 },
    { SEFLG_MOSEPH, "Moshier", 625000.5, 2818000.5 } };
  int i, k, e, b, na = 0, ta = 0, nb = 0, tb = 0, nc = 0, tc = 0, nd = 0, td = 0, nd2 = 0, td2 = 0, bad;
  char what[64];
  if (argc != 2) {
    fprintf(stderr, "usage: agree EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  setvbuf(stdout, NULL, _IOLBF, 0);
  /* (a): k 0..17 the seventeen calls (4..7 Gauquelin 2..5), 18 and 19
   * Gauquelin 0 and 1 */
  for (k = 0; k < 18; k++) {
    for (i = 0; i < 7; i++, ta++)
      na += tally(child(0, k, BADH[i], 0, 0, 0), "(a) refused", k, BADH[i]);
    for (i = 0; i < (k == 12 ? 1 : 3); i++, ta++)
      na += tally(child(0, k, GOODH[i], 1, 0, 0), "(a) answered", k, GOODH[i]);
  }
  for (k = 18; k < 20; k++, ta++)
    na += tally(child(0, k, 50000, 1, 0, 0), "(a) answered, no height read", k, 50000);
  for (i = 0; i < 6; i++, tb++)
    nb += tally(child(1, 0, db_sw[i], SEFLG_SWIEPH, 0, 0), "(b) Swiss", 0, db_sw[i]);
  for (i = 0; i < 4; i++, tb++)
    nb += tally(child(1, 0, db_sw[i], SEFLG_MOSEPH, 0, 0), "(b) Moshier", 0, db_sw[i]);
  for (k = 0; k < 60; k++) {
    if (k / 12 == 4 && (k / 3) % 4 < 2)
      continue;	/* the Moon has no heliacal rising or setting */
    tc++;
    nc += tally(child(2, k, 0, 0, 0, 0), "(c)", k, 0);
  }
  for (e = 0; e < 3; e++) {
    EPH = R[e].eph;
    for (k = 0; k < 6; k++)
      for (i = 0; i < 5; i++)
        for (b = 0; b < 2; b++, td++) {
          double S = b ? R[e].hi - dd[i] : R[e].lo + dd[i];
          snprintf(what, sizeof what, "(d) %s %s search", R[e].name, b ? "backward" : "forward");
          nd += tally(child(3, k, S, b, R[e].lo, R[e].hi), what, k, S);
        }
  }
  EPH = SEFLG_SWIEPH;
  for (k = 0; k < 4; k++)
    for (b = 0; b < 2; b++)
      for (i = 0; i < 21; i++) {
        double S = b ? 7927000.0 + i * 97 : -3026000.0 + i * 97;
        int back;
        for (back = 0; back < 2; back++) {
          if (k % 2 && i % 2)	/* the local searches at half the starts */
            continue;
          td2++;
          nd2 += tally(child(3, k, S, back | (b == back ? 0 : 2), R[0].lo, R[0].hi), "(d) Swiss far", k, S);
        }
      }
  for (i = 0; i < 12; i++, td2++)
    nd2 += tally(child(3, 6, 7925000.0 + i * 291, 1, R[0].lo, R[0].hi), "(d) Regulus far backward", 6, 7925000.0 + i * 291);
  for (i = 0; i < 12; i++, td2++)
    nd2 += tally(child(3, 4, -3024000.0 + i * 291, 1, R[0].lo, R[0].hi), "(d) Venus far backward", 4, -3024000.0 + i * 291);
  EPH = SEFLG_MOSEPH;
  td2++;
  nd2 += tally(child(3, 6, 1048162.0, 1, R[2].lo, R[2].hi), "(d) Regulus within delta t", 6, 1048162.0);
  printf("G177 (a) the site's height outside -500 .. 25000 m refused by every function that takes it: %d of %d\n", na, ta);
  printf("G177 (b) the Sun's elements the Earth's orbit's, as its distances: %d of %d\n", nb, tb);
  printf("G177 (c) swe_heliacal_pheno_ut() writes darr[28..30]: %d of %d\n", nc, tc);
  printf("G177 (d) the searches find the nearest event from the ephemerides' ends: %d of %d; at far dates: %d of %d\n", nd, td, nd2, td2);
  if (crash || hang)
    printf("G177 %d crashed, %d hung\n", crash, hang);
  bad = (ta - na) + (tb - nb) + (tc - nc) + (td - nd) + (td2 - nd2);
  if (bad)
    printf("G177 FAIL: %d\n", bad);
  else
    printf("G177 PASS\n");
  return bad != 0;
}
