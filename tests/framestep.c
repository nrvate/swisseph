/* framestep.c -- G139: across the steps of the frame's models every speed of
 * date is the rate of its own side's model: SEMOD_JPLHORA_3's at 1962 Jan
 * 20, SEFLG_JPLHOR's at 1799 Jan 1 and 2202 Jan 1, the edges of a
 * short-term precession model's window, and the joins of Owen 1990's
 * pieces; and across a window's edges the long-term sidereal time is its
 * own side's model's, and the house speeds and the true node's and
 * osculating apogee's each side's own rate.
 *
 * The defect (UPSTREAM-BUGS 153): with SEFLG_JPLHOR_APPROX under
 * SEMOD_JPLHORA_3 (the default mode), the precession, the mean obliquity
 * and the nutation are SEFLG_JPLHOR's (IAU 1976, IAU 1980 plus a fixed
 * offset) up to and including JD 2437684.5 and the chosen models after it:
 * steps of 0.113" in the precession (in longitude), 3.19 mas in the mean
 * obliquity, 7.17e-2" in dpsi on the default models. The rates that
 * difference those models reached across the step: the precession's for a
 * model other than Vondrak's, (P(t + 1) - P(t - 1)) / 2, the topocentric
 * site's precession in right ascension the same way, the obliquity's
 * eps(t + 1) - eps(t), the nutation's central difference over 1e-4 day,
 * and SEFLG_SPEED3's three places over 1e-4 day. On de200, for t in
 * (switch - 1, switch], every longitude speed of date was 1.57e-5 deg/day
 * off (Polaris's right ascension 1.31e-4, latitudes up to 8.9e-7 from the
 * obliquity), the topocentric Moon's 1.58e-7 more from its site; within
 * 1e-4 day of it 9.96e-2 deg/day (Polaris's right ascension 1.09); under
 * SEFLG_SPEED3 within 1e-4 day 5.75e-2; with IAU 2006 precession and IAU
 * 2000B nutation after it, the same within a day after it too.
 *
 * The defect (UPSTREAM-BUGS 159): SEFLG_JPLHOR's precession and obliquity
 * are IAU 1976's strictly between 1799 Jan 1 (JD 2378131.5) and 2202 Jan 1
 * (2525323.5) and Owen 1990's outside, so each boundary's own instant is
 * Owen's; SEMOD_JPLHORA_3 takes them before 1962 too. The mean obliquity
 * steps by 17.6 and 19.8 mas there, the precession by 6.5 and 7.4 mas, and
 * the same differences reached across: the precession's on both sides (both
 * models are differenced), the obliquity's from the day before, SPEED3's
 * within 1e-4 day.
 *
 * The defect (UPSTREAM-BUGS 160): a short-term precession model chosen with
 * swe_set_astro_models() holds within its window, IAU 1976 and 2000 within
 * 2 centuries of J2000, IAU 2006 within 75 (the edge's instant inside), and
 * the long-term model outside: with IAU 1976 and Vondrak ("0,9,1") the
 * precession steps by 0.53" at J2000 - 2 centuries and 0.57" at + 2, the
 * mean obliquity by 3.4 and 81.6 mas; with IAU 2006 at -+ 75 centuries the
 * precession by 42" and 107", the obliquity by 68" and 53". The same rates
 * reached across: the
 * precession's within a day inside the window (Vondrak's own rate holds
 * outside, unless the long-term model is another one differenced too),
 * the obliquity's the day before the edge, SPEED3's within 1e-4 day.
 *
 * The defect (UPSTREAM-BUGS 183): Owen 1990's precession and obliquity,
 * the long-term model chosen ("0,10") or SEFLG_JPLHOR's outside 1799-2202,
 * are fits in pieces of 80 centuries joined at JD -1931455.5, 990544.5,
 * 3912544.5 and 6834544.5 (about -10000, -2000, +6000, +14000; the join's
 * instant the later piece's), and they do not meet: the mean obliquity
 * steps by 0.095 mas at -10000, the precession by 0.086 and 0.091 mas at
 * +6000 and +14000. The same rates reached across: the obliquity's 2.6e-8
 * deg/day the day before -10000, the precession's 9.8e-9 and 1.1e-8 on
 * both days at +6000 and +14000.
 *
 * The defect (UPSTREAM-BUGS 184): the Moshier Moon's velocity turns the
 * series' own derivative into the J2000 equator and adds the frame's
 * turning as (R(t + 1) x - R(t - 1) x) / 2, which reached across the
 * windows' edges: with "0,9,1" its speed 8.0e-5 to 9.0e-5 deg/day off
 * within a day of J2000 -+ 2 centuries, the topocentric Moon's alike, and
 * its place 1.4e-9 deg through the light time (5e-8 deg/day in the speed
 * of the five points a day further out).
 *
 * The defect (UPSTREAM-BUGS 185): the elements of the equinox of date, the
 * JDATE bodies of seorbel.txt and swe_calc_orbel()'s SE_ORBEL_EQ_DATE,
 * add their frame's turning as (R(t + 1) x - R(t - 1) x) / 2 at the
 * elements' instant (under light time the emission instant), which reached
 * across the same edges: with "0,9,1" Selena's speed 8.0e-5 to 9.5e-5
 * deg/day off within a day of them, an Earth orbit of 0.053 AU 8.7e-5, a
 * Sun orbit of 1.2 AU 4.6e-5, Vulcan 7.7e-6.
 *
 * The defect (UPSTREAM-BUGS 181): the long-term sidereal time (outside
 * 1850-2050) integrates the precession in right ascension from the
 * precession matrices at nodes a century apart, each with its rate, a
 * difference over -+ 50 days, for a cubic Hermite between two. The edges
 * fall on nodes, whose rate took the step over 100 days, and the cubic
 * carried it a century either side and through the 1850 splice: against
 * "0,9,9" the sidereal time under "0,9,1" was +24.0" before 1700, +50.8" at
 * 1776, -22.2" at 2215, under "0,9,8" 1143" at -5550; and the topocentric
 * Moon's site turned by it, its speed 1.6e-8 to 1.9e-8 deg/day off the
 * places' rate within two days of J2000 - 2 centuries.
 *
 * The defect (UPSTREAM-BUGS 187): swe_houses_ex2()'s speeds take the rates
 * of the ARMC and the true obliquity (and of the sidereal houses' ayanamsa
 * and auxiliary frames) as central differences of the frames 1/64 day of
 * UT either side, which within 1/64 day of an edge reached across the
 * obliquity's step and, with bug 181 fixed, the sidereal time's: every
 * cusp and ascmc speed 2.1e-3 to 8.1e-3 deg/day off under "0,9,1", 4.65e-5
 * with the IERS 2010 sidereal time, the sidereal ones 3.9e-3 to 5.1e-3.
 *
 * The defect (UPSTREAM-BUGS 186): the true node's and the osculating
 * apogee's speeds difference the Moon's osculating elements at epochs 1/64
 * day apart, each epoch's Moon put into the frame of date at that epoch,
 * so within 1/32 day of an edge the difference took the frame's step: on
 * de200 and the Swiss files under "0,9,1" the node's speed 2.90e-3
 * deg/day off at J2000 - 2 centuries, 4.29e-3 at + 2, the apogee's 5.96e-3
 * and 6.49e-3; under "0,9,8" at -+ 75 centuries 7.90 and 0.68.
 *
 * Check (a): de200, SEFLG_JPLHOR_APPROX; at 1962 Jan 20 the default models
 * and IAU 2006 precession with IAU 2000B nutation ("0,8,8,4", which
 * difference after the step too), at 1799 Jan 1 the default models; and
 * without the flag IAU 1976's window at J2000 - 2 centuries ("0,9,1"),
 * with the IERS 2010 sidereal time throughout and with the long-term one,
 * which steps with the precession there (bug 181); the Sun, the Moon, Mars, Polaris (swe_fixstar2()), sidereal Mars
 * (Lahiri; swe_calc() drops the Horizons flags for it, so it has no step
 * at 1962 and 1799), the topocentric Moon at Zurich, and Mars under
 * SEFLG_SPEED3; ecliptic and equatorial, longitude and latitude speeds, at
 * 105 instants across (step - 2, step + 2]: every 1/16 day, and the step
 * -+ 2^-k for k = 7..26. The
 * reference is a five-point difference of the same call's positions over
 * 2^-6 day, centred where its points lie on the instant's side of the step,
 * else one-sided on that side (t, t -+ h, ..., t -+ 4h). The topocentric
 * Moon takes delta t set to 2^-11 day, so that its UT1 is exact and the
 * site has no jitter, and a centred nine-point difference over 2^-7 day, at
 * the instants where that lies on their side (the one-sided ones carry the
 * diurnal parallax's curvature, 3e-6 at five points over 2^-8). Each
 * setting, body and frame is judged on the worst of its two speeds over
 * three spans, the day before the step, the day after it and the rest: 210
 * checks, within 1e-8 deg/day, SEFLG_SPEED3 within 1e-5 (its own three
 * places over 1e-4 day are 1.3e-6 off everywhere).
 *
 * Check (c): bodies whose frame turning is their own difference of the
 * frame: the Moshier Moon and topocentric Moon with "0,9,1" and the IERS
 * 2010 sidereal time at J2000 -+ 2 centuries, ecliptic and equatorial,
 * over the same three spans, against the references of check (a); and
 * Selena, Vulcan and two swe_calc_orbel() orbits of the equinox of date
 * (0.053 AU about the Earth, 1.2 AU about the Sun, moved by their mean
 * motion), apparent places, on de200 at - 2 centuries (Vulcan, behind the
 * Sun's disc there, left out: the deflection's curvature truncates its
 * reference to 1.8e-8; the Swiss files join a day and a half after it)
 * and on the Swiss files at + 2: each instant judged in its cell, the same
 * side of the step at the instant (the precession of date) and at its
 * emission instant (the elements' frame), t less the light time of the
 * distance; an instant between the two, a light time wide, has no
 * five-point reference and is passed over. 66 checks within 1e-8
 * deg/day.
 *
 * Check (b): the rates themselves under SEFLG_JPLHOR, which swe_calc()
 * takes only with a DE 403 file and the IERS corrections: the precession's
 * (swi_precess_speed() of a unit vector of date) and the mean obliquity's
 * (swi_obliquity_rate()), against five-point differences of swi_precess()
 * and swi_epsiln() over 2^-6 day on the instant's side, at the same 105
 * instants about 1799 Jan 1 and 2202 Jan 1, the worst over the three spans;
 * and without the flag at the short-term windows' edges, IAU 1976 ("0,9,1")
 * at J2000 -+ 2 centuries, IAU 2000 ("0,9,6") at + 2, IAU 2006 ("0,9,8")
 * at -+ 75, IAU 1976 within IAU 2006 long-term ("0,8,1", differenced on
 * both sides) at - 2, and IAU 1976 at + 2 with SEFLG_JPLHOR_APPROX, whose
 * mode 3 takes the chosen models after 1962: 54 checks within 1e-8
 * deg/day; and Owen 1990's joins, "0,10" at all four and SEFLG_JPLHOR at
 * the three that step, 42 checks within 1e-9 (the residuals sat about the
 * 1e-8 floor). It calls the fork's own functions and is left out of the
 * upstream build.
 *
 * Check (d): the long-term sidereal time across the windows' edges, the
 * mean sidereal time of swe_sidtime0() under "0,9,1" at J2000 -+ 2
 * centuries, "0,9,6" at + 2, "0,9,8" at -+ 75 and "0,8,1" at - 2, against
 * the same of the side's model alone ("0,1,1", "0,6,6" or "0,8,8" inside,
 * "0,9,9" or "0,8,8" outside), at the 105 instants about the edge and
 * every 1/8 century to 1 3/8 inside and outside, and 3 and 6 centuries
 * outside (through the 1850 splice at - 2 centuries): delta t 2^-11 day,
 * the worst inside and outside, 12 checks within 1e-9 deg.
 *
 * Check (e): the house speeds across the windows' edges, "0,9,1" at J2000
 * -+ 2 centuries with the long-term sidereal time and at - 2 with the IERS
 * 2010 one; Placidus, Koch, equal, Regiomontanus and sidereal (Lahiri)
 * Placidus at Zurich on the Swiss files: the 12 cusps' and 8 ascmc's
 * speeds against five-point central differences over 2^-12 day of the
 * cusps of the instant's side's model alone, which has no step and whose
 * cusps there are the models' (check (d)), at the 105 instants (TT, the UT
 * 2^-11 day before), the worst within 1/64 day of the edge and the rest:
 * 30 checks within 1e-7 deg/day, the reference's own noise being 4.5e-8
 * (the rotation angle's rounding, 1e-11 deg at 2 centuries from J2000,
 * over 2^-12 day).
 *
 * Check (f): the true node's and the osculating apogee's speeds across the
 * windows' edges, "0,9,1" at J2000 - 2 centuries on de200 and the Swiss
 * files and at + 2 on the Swiss files, "0,9,8" at -+ 75 on the Swiss
 * files, ecliptic and equatorial of date: the longitude's, latitude's and
 * distance's speeds against nine-point central differences over 2^-3 day
 * of the positions of the instant's side's model alone, at the 49 of the
 * 105 instants within 1/4 day of the edge (beyond, the reference's half
 * day meets joints of the Swiss Moon's segments and files, whose kinks the
 * node's rate takes: 4.1e-6 deg/day across JD 2378496.5), the worst within
 * 1/32 day and from there to 1/4: 40 checks within 1e-9 deg/day for the
 * node, 5e-8 for the apogee (G74's) and 4e-13 AU/day. Moshier's, whose
 * Moon's frame has a step of its own there, is left out; so is the
 * ecliptic of J2000, whose obliquity is the window's at J2000 on both
 * sides and so not the outside model's alone.
 *
 * Measured after the fix: (a) within 2.4e-9 (the Moon's one-sided
 * reference at J2000 - 2 centuries, 2.0e-9 at 1799; Polaris's right
 * ascension 1.45e-9 at 1962, the topocentric Moon 9.6e-10, the others
 * 7e-11), SPEED3 1.41e-6; (b) within 8.5e-12, at Owen's joins 1.9e-11.
 * Before the fix of bug 183, 10 of the 42 at Owen's joins fail: the
 * obliquity's rate 2.63e-8 the day before -10000, the precession's 9.80e-9
 * and 1.09e-8 on both days at +6000 and +14000, under both settings.
 * (c) within 2.97e-9 (the reference's own on the Moshier Moon); before the
 * fix of bug 184 all 24 fail (8.0e-5 to 9.0e-5 the day either side,
 * 4.4e-8 to 1.1e-7 a day further out); the upstream build fails them too
 * (its Moon's speed 5.2 deg/day off within 5e-5 day of the edge). The
 * elements of date within 1.03e-9 (Vulcan); before the fix of bug 185, 34
 * of those 42 fail (Selena 7.6e-5 to 9.5e-5, Vulcan 4.6e-6 to 7.7e-6);
 * the upstream build fails all 66 of (c) (its Selena on a fixed ellipse
 * 1.1e-5 off, Vulcan 3.0e-3), 226 of its 234.
 * Before the fix of bug 153, 38 of the first 84 checks fail: every body but sidereal Mars on the day
 * before the step under both settings and on the day after under the
 * second, and SPEED3 on both days under both, by 1.57e-5 to 1.09 deg/day.
 * Before the fix of bug 159, 24 of
 * the 42 at 1799 fail, every body but sidereal Mars on both days (the
 * Sun's latitude speed 5.0e-6 deg/day off, Polaris's right ascension
 * 5.6e-6, SPEED3 8.9e-3), and 6 of (b)'s 12: the precession's rate 2.8e-7
 * and 1.0e-6 on both days, the obliquity's 4.9e-6 and 5.5e-6 the day
 * before. Before the fix of bug 160, 22 of the 42 at J2000 - 2 centuries
 * fail, every body on the day after (inside the window, the precession's,
 * 6.5e-6 to 8.7e-5 deg/day) and in the ecliptic on the day before (the
 * obliquity's, 6.5e-7 to 9.3e-7), SPEED3 0.80 to 0.83 on both, and 15 of
 * (b)'s 42 at the windows: the precession's rate 2.6e-6 to 1.5e-2
 * deg/day, the obliquity's 4.5e-7 to 1.9e-2. Before the fix of bug 181,
 * 6 of the 42 at J2000 - 2 centuries with the long-term sidereal time fail,
 * the topocentric Moon's both frames over the three spans (1.5e-8 to
 * 1.9e-8 deg/day; 6.0e-10 to 8.6e-10 after), and all 12 of (d), by
 * 3.1e-4 deg ("0,9,6") to 0.38 ("0,9,8"); after, (d) is 0 bit for bit
 * everywhere. Before the fix of bug 187, 15 of (e)'s 30 fail, every one
 * within 1/64 day of the edge (2.1e-3 to 8.1e-3 deg/day, 4.65e-5 with the
 * IERS 2010 sidereal time, sidereal 3.9e-3 to 5.1e-3); after, (e) within
 * 4.5e-8 everywhere. Before the fix of bug 186, 20 of (f)'s 40 fail, every
 * one within 1/32 day of the edge (2.80e-3 to 7.90 deg/day); after, (f)
 * within 6.1e-10 for the node and 7.2e-9 for the apogee everywhere. The
 * upstream build fails 200 of (a)'s 210, the 66 of
 * (c), 3 of (d)'s 12 (outside the IAU 1976 window by 4.6e-9 to 6.6e-9 deg:
 * its long-term formula takes the obliquity of J2000, inside the window,
 * at every date), all 30 of (e) (its house speeds at ARMCS, entry 42:
 * equal houses 3.0e-5 to 8.6e-5 deg/day, Koch 199) and all 40 of (f) (its
 * difference over 1e-4 day across the edge, 0.36 to 951 deg/day, and its
 * interval's rounding beyond, entry 75), 339 of its 358. Of
 * (a)'s first 168 it fails 160: its nutation rate, a backward
 * difference, reaches across the 1962 step just after it (0.200 deg/day,
 * Polaris's right ascension 2.14), its ayanamsa rate, a backward
 * difference too, across the window's edge (sidereal Mars 0.160), its
 * SPEED3 2.48 there, and its own speeds are off elsewhere by the defects
 * earlier entries fixed (the topocentric Moon 4e-3, sidereal Mars 5e-7);
 * only SPEED3 away from the steps passes.
 * Run as
 *   framestep EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#ifndef SWE_UPSTREAM_COMPAT
#include "sweph.h"
#include "swephlib.h"
#endif

/* check (a)'s settings: the step, whether its own instant is the left
 * side's model (at or before it) or the right side's, the flags with
 * SEFLG_JPLEPH, the models */
static const struct {
  double ts;
  int left;
  int32 fl;
  const char *mod;
} SET[] = {
  /* HORIZONS_TJD0_DPSI_DEPS_IAU1980 */
  { 2437684.5, 1, SEFLG_JPLHOR_APPROX, "0,0,0,0,0,0,0,0" },
  { 2437684.5, 1, SEFLG_JPLHOR_APPROX, "0,8,8,4,0,0,0,0" },
  /* SEFLG_JPLHOR's 1799 Jan 1 */
  { 2378131.5, 1, SEFLG_JPLHOR_APPROX, "0,0,0,0,0,0,0,0" },
  /* IAU 1976's short-term window, J2000 - 2 centuries; the sidereal
   * time the IERS 2010 formula's throughout */
  { 2378495.0, 0, 0, "0,9,1,0,0,0,0,3" },
  /* the same with the long-term sidereal time, each side's own (bug 181),
   * which the topocentric Moon's site turns by */
  { 2378495.0, 0, 0, "0,9,1,0,0,0,0,0" },
};
#define NSET ((int) (sizeof SET / sizeof SET[0]))

#define EPHMASK (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)
/* light time, days per AU */
#define LTDAY (499.004783836 / 86400.0)

/* check (a)'s bodies, then check (c)'s own: Selena and Vulcan (seorbel.txt,
 * equinox of date), and swe_calc_orbel() orbits of the equinox of date, one
 * about the Earth and one about the Sun */
enum { B_SUN, B_MOON, B_MARS, B_POLARIS, B_SIDMARS, B_TOPOMOON, B_SPEED3, NB,
  B_SELENA = NB, B_VULCAN, B_ORBGEO, B_ORBHEL, NBC };
static const char *BN[NBC] = { "Sun", "Moon", "Mars", "Polaris", "sidereal Mars",
  "topocentric Moon", "Mars SPEED3", "Selena", "Vulcan", "orbit about Earth",
  "orbit about Sun" };
static const char *RN[3] = { "day before", "day after", "outside" };

/* the step and flags in use; LTCELL: a place's frame of date is its
 * emission instant's too (the equinox of date of check (c)'s elements), so
 * the step is seen at the instant and again a light time after it */
static double TS;
static int LEFT;
static int32 FL;
static int LTCELL;

/* the place at t, in degrees, and its speeds; the flags returned checked to
 * be the ephemeris asked for (de200's in check (a)), with the Horizons
 * approximation where asked (but for sidereal Mars) */
static int place(swe_ctx *c, int b, double t, int32 xf, double *x)
{
  char serr[256] = "", s[SE_MAX_STNAME * 2 + 1];
  int32 fl = FL | SEFLG_SPEED | xf, rf;
  switch (b) {
  case B_SUN: rf = swe_calc_r(c, t, SE_SUN, fl, x, serr); break;
  case B_MOON: rf = swe_calc_r(c, t, SE_MOON, fl, x, serr); break;
  case B_MARS: rf = swe_calc_r(c, t, SE_MARS, fl, x, serr); break;
  case B_POLARIS:
    strcpy(s, "Polaris");
    rf = swe_fixstar2_r(c, s, t, fl, x, serr);
    break;
  case B_SIDMARS: rf = swe_calc_r(c, t, SE_MARS, fl | SEFLG_SIDEREAL, x, serr); break;
  case B_TOPOMOON: rf = swe_calc_r(c, t, SE_MOON, fl | SEFLG_TOPOCTR, x, serr); break;
  case B_SPEED3:
    rf = swe_calc_r(c, t, SE_MARS, (fl & ~SEFLG_SPEED) | SEFLG_SPEED3, x, serr);
    break;
  case B_SELENA: rf = swe_calc_r(c, t, SE_WHITE_MOON, fl, x, serr); break;
  case B_VULCAN: rf = swe_calc_r(c, t, SE_VULCAN, fl, x, serr); break;
  default: {
    /* moved from TS by the two-body mean motion swe_calc_orbel() takes */
    double el[6] = { 40, 0.053, 0.05, 30, 60, 5 }, mu;
    int32 ctr = SE_EARTH;
    if (b == B_ORBHEL) {
      el[1] = 1.2;
      el[2] = 0.2;
      el[4] = 80;
      el[5] = 12;
      ctr = SE_SUN;
    }
    mu = (ctr == SE_SUN) ? 0.01720209895 * 0.01720209895
                         : 0.0000298122353216 * 0.0000298122353216;
    el[0] += sqrt(mu / (el[1] * el[1] * el[1])) * 180 / M_PI * (t - TS);
    rf = swe_calc_orbel_r(c, t, el, ctr, SE_ORBEL_EQ_DATE, 0, fl, x, serr);
    break;
  }
  }
  if (rf < 0 || (rf & EPHMASK) != (FL & EPHMASK)
      || (b != B_SIDMARS && (rf & SEFLG_JPLHOR_APPROX) != (FL & SEFLG_JPLHOR_APPROX))) {
    printf("  ERROR %s at %.8f: flags 0x%x %s\n", BN[b], t, (unsigned) rf, serr);
    return -1;
  }
  return 0;
}

/* t on the step's left side: before it, or at it where that is the left's */
static int left_of(double t)
{
  return LEFT ? (t <= TS) : (t < TS);
}

/* on the instant's side of the step */
static int same_side(double p, double t)
{
  return left_of(p) == left_of(t);
}

/* the span of t: the day before the step, the day after it, the rest */
static int span(double t)
{
  if (left_of(t))
    return (t > TS - 1 || (!LEFT && t == TS - 1)) ? 0 : 2;
  return (t < TS + 1 || (LEFT && t == TS + 1)) ? 1 : 2;
}

/* |speed - reference| for the two angles, the larger; 0 with *skip set
 * where the body's reference does not lie on t's side */
static double off(swe_ctx *c, int b, double t, int32 xf, int *skip, int *err)
{
  static const double c5[2] = { 2.0 / 3, -1.0 / 12 };
  static const double c9[4] = { 4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280 };
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  int n = (b == B_TOPOMOON) ? 4 : 2, k, j, centred = 1, dir;
  double h = (b == B_TOPOMOON) ? 1.0 / 128 : 1.0 / 64, x[6], f[9][6], r = 0;
  *skip = 0;
  if (LTCELL) {
    /* the places' cell: the instant's side of the step and its emission
     * instant's, t less the light time of the distance; none of the
     * references fits between the two, a light time wide */
    double te;
    int pass;
    if (place(c, b, t, xf, x) < 0) {
      *err = 1;
      return 0;
    }
    te = t - x[2] * LTDAY;
    dir = left_of(t) ? -1 : 1;
    for (pass = 0; pass < 2; pass++) {
      int in = (left_of(t) == left_of(te));
      centred = (pass == 0);
      for (k = 0; k <= 2 * n && in; k++) {
        double tk = centred ? t + (k - n) * h : t + dir * k * h;
        if (place(c, b, tk, xf, f[k]) < 0) {
          *err = 1;
          return 0;
        }
        in = same_side(tk, t) && same_side(tk - f[k][2] * LTDAY, te);
      }
      if (in)
        break;
    }
    if (pass == 2) {
      *skip = 1;
      return 0;
    }
  } else {
  for (k = 1; k <= n; k++)
    if (!same_side(t - k * h, t) || !same_side(t + k * h, t))
      centred = 0;
  if (!centred && b == B_TOPOMOON) {
    *skip = 1;
    return 0;
  }
  if (place(c, b, t, xf, x) < 0) {
    *err = 1;
    return 0;
  }
  dir = left_of(t) ? -1 : 1;
  for (k = 0; k <= 2 * n; k++) {
    double tk = centred ? t + (k - n) * h : t + dir * k * h;
    if (place(c, b, tk, xf, f[k]) < 0) {
      *err = 1;
      return 0;
    }
  }
  }
  for (j = 0; j < 2; j++) {
    double d = 0, v;
    for (k = 0; k <= 2 * n; k++) {
      v = f[k][j] - f[centred ? n : 0][j];
      if (j == 0)
        v -= 360 * floor(v / 360 + 0.5);
      if (centred) {
        if (k > n) d += v * (n == 2 ? c5 : c9)[k - n - 1];
        if (k < n) d -= v * (n == 2 ? c5 : c9)[n - k - 1];
      } else {
        d += v * o5[k];
      }
    }
    d /= h;
    if (!centred)
      d *= dir;
    if (fabs(x[3 + j] - d) > r)
      r = fabs(x[3 + j] - d);
  }
  return r;
}

/* the 105 instants about TS */
static int instants(double *tq)
{
  int nq = 0, k;
  for (k = -32; k <= 32; k++)
    tq[nq++] = TS + k / 16.0;
  for (k = 7; k <= 26; k++) {
    tq[nq++] = TS - ldexp(1, -k);
    tq[nq++] = TS + ldexp(1, -k);
  }
  return nq;
}

#ifndef SWE_UPSTREAM_COMPAT
/* check (b)'s rate r of g at t, rad/day: 0 the precession of a unit
 * vector of date (three components), 1 the mean obliquity */
static void rate_b(swe_ctx *c, int g, double t, int32 fl, double *r)
{
  if (g == 0) {
    double xx[6] = { 0.3, 0.5, 0, 0, 0, 0 };
    xx[2] = sqrt(1 - xx[0] * xx[0] - xx[1] * xx[1]);
    swi_precess(c, xx, t, fl, J2000_TO_J);
    swi_precess_speed(c, xx, t, fl, J2000_TO_J);
    r[0] = xx[3];
    r[1] = xx[4];
    r[2] = xx[5];
  } else {
    r[0] = swi_obliquity_rate(c, t, fl, FALSE);
  }
}

static void value_b(swe_ctx *c, int g, double t, int32 fl, double *v)
{
  if (g == 0) {
    v[0] = 0.3;
    v[1] = 0.5;
    v[2] = sqrt(1 - v[0] * v[0] - v[1] * v[1]);
    swi_precess(c, v, t, fl, J2000_TO_J);
  } else {
    v[0] = swi_epsiln(c, t, fl);
  }
}

/* check (b): the worst |rate - reference| of g over each span, deg/day */
static void check_b(swe_ctx *c, int g, int32 fl, double *w)
{
  static const double c5[2] = { 2.0 / 3, -1.0 / 12 };
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  double tq[105], h = 1.0 / 64;
  int nq = instants(tq), i, k, j, ng = (g == 0) ? 3 : 1;
  w[0] = w[1] = w[2] = 0;
  for (i = 0; i < nq; i++) {
    double t = tq[i], r[3], v[5][3], e = 0;
    int centred = 1, dir = left_of(t) ? -1 : 1;
    for (k = 1; k <= 2; k++)
      if (!same_side(t - k * h, t) || !same_side(t + k * h, t))
        centred = 0;
    rate_b(c, g, t, fl, r);
    for (k = 0; k <= 4; k++)
      value_b(c, g, centred ? t + (k - 2) * h : t + dir * k * h, fl, v[k]);
    for (j = 0; j < ng; j++) {
      double d = 0;
      if (centred) {
        d = (c5[0] * (v[3][j] - v[1][j]) + c5[1] * (v[4][j] - v[0][j])) / h;
      } else {
        for (k = 0; k <= 4; k++)
          d += o5[k] * (v[k][j] - v[0][j]);
        d *= dir / h;
      }
      if (fabs(r[j] - d) > e)
        e = fabs(r[j] - d);
    }
    e *= RADTODEG;
    if (e > w[span(t)])
      w[span(t)] = e;
  }
}
#endif

/* check (d): the long-term sidereal time across a window's edge, the
 * mean sidereal time of swe_sidtime0() (fixed obliquity, no nutation) of
 * the models against the same of the model alone on the instant's side:
 * inside the short-term one, outside the long-term one. The instants in
 * TT, their UT 2^-11 day before (delta t set to it); the worst inside and
 * outside, degrees; returns the failures, *n the checks. */
static int check_d(int *n)
{
  static const struct {
    double ts;
    int left;
    const char *mod, *in, *out;
  } SC[] = {
    { 2378495.0, 0, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 2524595.0, 1, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 2524595.0, 1, "0,9,6,0,0,0,0,0", "0,6,6,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { -287830.0, 0, "0,9,8,0,0,0,0,0", "0,8,8,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 5190920.0, 1, "0,9,8,0,0,0,0,0", "0,8,8,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 2378495.0, 0, "0,8,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,8,8,0,0,0,0,0" },
  };
  static const double cy[13] = { 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1, 1.125,
                                 1.25, 1.375, 3, 6 };
  static const char *sn[2] = { "inside", "outside" };
  double tq[105 + 24];
  int m, i, k, bad = 0;
  for (m = 0; m < (int) (sizeof SC / sizeof SC[0]); m++) {
    char s0[40], s1[40], s2[40];
    swe_ctx *c = swe_ctx_new(), *ci = swe_ctx_new(), *co = swe_ctx_new();
    double w[2] = { 0, 0 };
    int nq;
    strcpy(s0, SC[m].mod);
    strcpy(s1, SC[m].in);
    strcpy(s2, SC[m].out);
    swe_set_astro_models_r(c, s0, 0);
    swe_set_astro_models_r(ci, s1, 0);
    swe_set_astro_models_r(co, s2, 0);
    swe_set_delta_t_userdef_r(c, ldexp(1, -11));
    swe_set_delta_t_userdef_r(ci, ldexp(1, -11));
    swe_set_delta_t_userdef_r(co, ldexp(1, -11));
    TS = SC[m].ts;
    LEFT = SC[m].left;
    nq = instants(tq);
    /* to 1.375 centuries inside, and through the 1850 splice outside */
    for (k = 0; k < 13; k++) {
      double sg = (TS < 2451545.0) ? -1 : 1;
      tq[nq++] = TS + sg * cy[k] * 36525;
      if (k < 11)
        tq[nq++] = TS - sg * cy[k] * 36525;
    }
    for (i = 0; i < nq; i++) {
      double ut = tq[i] - ldexp(1, -11), d;
      /* the window is J2000's side of the edge */
      int inside = (TS < 2451545.0) ? !left_of(tq[i]) : left_of(tq[i]);
      d = swe_sidtime0_r(c, ut, 23.4, 0) - swe_sidtime0_r(inside ? ci : co, ut, 23.4, 0);
      d = fabs(d - 24 * floor(d / 24 + 0.5)) * 15;
      if (d > w[!inside])
        w[!inside] = d;
    }
    printf("  (d) models %s, edge JD %.1f: worst |sidereal time - the side's model alone's|,"
           " deg, %s / %s\n", SC[m].mod, TS, sn[0], sn[1]);
    printf("    sidereal time         %.2e  %.2e\n", w[0], w[1]);
    for (k = 0; k < 2; k++) {
      (*n)++;
      if (!(w[k] <= 1e-9)) {
        bad++;
        printf("    FAIL models %s, sidereal time %s: %.2e > 1e-9\n", SC[m].mod, sn[k], w[k]);
      }
    }
    swe_ctx_free(c);
    swe_ctx_free(ci);
    swe_ctx_free(co);
  }
  return bad;
}

/* check (e): the house speeds of swe_houses_ex2() across a window's edge,
 * the 12 cusps' and 8 ascmc's, against five-point central differences over
 * 2^-12 day of the cusps of the instant's side's model alone, which has no
 * step and whose cusps on that side are the models' (check (d)); Zurich,
 * the Swiss files, the instants in TT, their UT 2^-11 day before; the
 * worst within 1/64 day of the edge and the rest, deg/day; returns the
 * failures, *n the checks. */
static int check_e(const char *ephe, int *n)
{
  static const struct {
    double ts;
    int left;
    const char *mod, *in, *out;
  } SD[] = {
    { 2378495.0, 0, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 2524595.0, 1, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    /* the IERS 2010 sidereal time: the obliquity alone steps */
    { 2378495.0, 0, "0,9,1,0,0,0,0,3", "0,1,1,0,0,0,0,3", "0,9,9,0,0,0,0,3" },
  };
  static const struct {
    int hs;
    int32 fl;
    const char *name;
  } SH[] = {
    { 'P', 0, "Placidus" }, { 'K', 0, "Koch" }, { 'E', 0, "equal" },
    { 'R', 0, "Regiomontanus" }, { 'P', SEFLG_SIDEREAL, "sidereal Placidus" },
  };
  static const char *sn[2] = { "within 1/64 day", "the rest" };
  const double h = ldexp(1, -12), dts = ldexp(1, -11);
  double tq[105];
  int m, s, i, j, k, q, bad = 0, nq;
  for (m = 0; m < (int) (sizeof SD / sizeof SD[0]); m++) {
    swe_ctx *cc[3];
    const char *ms[3] = { SD[m].mod, SD[m].in, SD[m].out };
    for (k = 0; k < 3; k++) {
      char mod[40];
      cc[k] = swe_ctx_new();
      strcpy(mod, ms[k]);
      swe_set_ephe_path_r(cc[k], ephe);
      swe_set_astro_models_r(cc[k], mod, 0);
      swe_set_delta_t_userdef_r(cc[k], dts);
      swe_set_sid_mode_r(cc[k], SE_SIDM_LAHIRI, 0, 0);
    }
    TS = SD[m].ts;
    LEFT = SD[m].left;
    nq = instants(tq);
    printf("  (e) models %s, edge JD %.1f: worst |house speed - reference|, deg/day, %s / %s\n",
           SD[m].mod, TS, sn[0], sn[1]);
    for (s = 0; s < (int) (sizeof SH / sizeof SH[0]); s++) {
      double w[2] = { 0, 0 };
      for (i = 0; i < nq; i++) {
        double t = tq[i] - dts, cusp[13], ascmc[10], cs[13], as[10], f[5][20];
        int inside = (TS < 2451545.0) ? !left_of(tq[i]) : left_of(tq[i]);
        int sp = fabs(tq[i] - TS) <= 1.0 / 64 ? 0 : 1;
        swe_ctx *cr = cc[inside ? 1 : 2];
        swe_houses_ex2_r(cc[0], t, SH[s].fl, 47.37, 8.55, SH[s].hs, cusp, ascmc, cs, as, NULL);
        for (k = 0; k < 5; k++) {
          double cu[13], am[10];
          swe_houses_ex2_r(cr, t + (k - 2) * h, SH[s].fl, 47.37, 8.55, SH[s].hs, cu, am,
                           NULL, NULL, NULL);
          for (j = 0; j < 12; j++)
            f[k][j] = cu[j + 1];
          for (j = 0; j < 8; j++)
            f[k][12 + j] = am[j];
        }
        for (j = 0; j < 20; j++) {
          double v[5], d, e;
          int jump = 0;
          for (k = 0; k < 5; k++) {
            v[k] = swe_difdeg2n(f[k][j], f[2][j]);
            if (fabs(v[k]) > 5)
              jump = 1;
          }
          if (jump)	/* a cusp across a sign: whole signs, none here */
            continue;
          d = (8 * (v[3] - v[1]) - (v[4] - v[0])) / (12 * h);
          e = fabs((j < 12 ? cs[j + 1] : as[j - 12]) - d);
          if (!(e <= w[sp]))
            w[sp] = e;
        }
      }
      printf("    %-18s    %.2e  %.2e\n", SH[s].name, w[0], w[1]);
      for (q = 0; q < 2; q++) {
        (*n)++;
        if (!(w[q] <= 1e-7)) {
          bad++;
          printf("    FAIL models %s, %s, %s: %.2e > 1e-7\n", SD[m].mod, SH[s].name, sn[q], w[q]);
        }
      }
    }
    for (k = 0; k < 3; k++)
      swe_ctx_free(cc[k]);
  }
  return bad;
}

/* check (f): the true node's and the osculating apogee's speeds across the
 * windows' edges: "0,9,1" at J2000 - 2 centuries on de200 and the Swiss
 * files and at + 2 on the Swiss files, "0,9,8" at -+ 75 on the Swiss
 * files; ecliptic and equatorial of date, the longitude's, latitude's and
 * distance's speeds against a nine-point central difference over 2^-3 day
 * of the positions of the instant's side's model alone, at the 49 of the
 * 105 instants within 1/4 day of the edge (TT; the reference's half
 * day either side then meets no joint of the Swiss Moon's segments or
 * files, whose kinks the node's and apogee's rates take); the worst
 * within 1/32 day of the edge (the speed's own stencil) and the rest,
 * deg/day and AU/day; returns the failures, *n the checks. */
static int check_f(const char *ephe, int *n)
{
  static const struct {
    double ts;
    int left;
    int32 eph;
    const char *mod, *in, *out;
  } SE[] = {
    { 2378495.0, 0, SEFLG_JPLEPH, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 2378495.0, 0, SEFLG_SWIEPH, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 2524595.0, 1, SEFLG_SWIEPH, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { -287830.0, 0, SEFLG_SWIEPH, "0,9,8,0,0,0,0,0", "0,8,8,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 5190920.0, 1, SEFLG_SWIEPH, "0,9,8,0,0,0,0,0", "0,8,8,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
  };
  static const struct {
    int ipl;
    const char *name;
    double lim;
  } SB[] = { { SE_TRUE_NODE, "true node", 1e-9 }, { SE_OSCU_APOG, "osc. apogee", 5e-8 } };
  static const struct {
    int32 fl;
    const char *name;
  } SF[] = { { 0, "ecl" }, { SEFLG_EQUATORIAL, "equ" } };
  /* the nine-point central difference's weights, eighth order */
  static const double c9[4] = { 4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280 };
  static const char *sn[2] = { "within 1/32 day", "1/32 to 1/4 day" };
  const double h = ldexp(1, -3), dlim = 4e-13;
  double tq[105];
  int m, b, f, i, j, k, q, bad = 0, nq;
  for (m = 0; m < (int) (sizeof SE / sizeof SE[0]); m++) {
    swe_ctx *cc[3];
    const char *ms[3] = { SE[m].mod, SE[m].in, SE[m].out };
    for (k = 0; k < 3; k++) {
      char mod[40];
      cc[k] = swe_ctx_new();
      strcpy(mod, ms[k]);
      swe_set_ephe_path_r(cc[k], ephe);
      swe_set_jpl_file_r(cc[k], "de200.eph");
      swe_set_astro_models_r(cc[k], mod, 0);
    }
    TS = SE[m].ts;
    LEFT = SE[m].left;
    nq = instants(tq);
    printf("  (f) %s, models %s, edge JD %.1f: worst |speed - reference|, deg/day"
           " (distance AU/day), %s / %s\n", SE[m].eph == SEFLG_JPLEPH ? "de200" : "Swiss",
           SE[m].mod, TS, sn[0], sn[1]);
    for (b = 0; b < 2; b++)
      for (f = 0; f < 2; f++) {
        double w[2] = { 0, 0 }, wd[2] = { 0, 0 };
        int32 fl = SE[m].eph | SF[f].fl;
        for (i = 0; i < nq; i++) {
          double x[6], p[9][6];
          char serr[256];
          int inside = (TS < 2451545.0) ? !left_of(tq[i]) : left_of(tq[i]);
          int sp = fabs(tq[i] - TS) <= 1.0 / 32 ? 0 : 1;
          swe_ctx *cr = cc[inside ? 1 : 2];
          if (fabs(tq[i] - TS) > 0.25)
            continue;
          /* the ephemeris asked, not a fallback */
          if ((swe_calc_r(cc[0], tq[i], SB[b].ipl, fl | SEFLG_SPEED, x, serr)
               & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)) != SE[m].eph) {
            printf("    ERROR %s\n", serr);
            bad++;
            continue;
          }
          for (k = 0; k < 9; k++)
            swe_calc_r(cr, tq[i] + (k - 4) * h, SB[b].ipl, fl, p[k], serr);
          for (j = 0; j < 3; j++) {
            double d = 0, e;
            for (k = 1; k <= 4; k++) {
              if (j == 0)
                d += c9[k - 1] * swe_difdeg2n(p[4 + k][j], p[4 - k][j]);
              else
                d += c9[k - 1] * (p[4 + k][j] - p[4 - k][j]);
            }
            d /= h;
            e = fabs(x[3 + j] - d);
            if (j < 2 && !(e <= w[sp]))
              w[sp] = e;
            if (j == 2 && !(e <= wd[sp]))
              wd[sp] = e;
          }
        }
        printf("    %-12s %-9s  %.2e  %.2e  (distance %.1e  %.1e)\n", SB[b].name, SF[f].name,
               w[0], w[1], wd[0], wd[1]);
        for (q = 0; q < 2; q++) {
          (*n)++;
          if (!(w[q] <= SB[b].lim && wd[q] <= dlim)) {
            bad++;
            printf("    FAIL %s, models %s, %s %s, %s: %.2e > %.0e or %.1e > %.0e\n",
                   SE[m].eph == SEFLG_JPLEPH ? "de200" : "Swiss", SE[m].mod, SB[b].name,
                   SF[f].name, sn[q], w[q], SB[b].lim, wd[q], dlim);
          }
        }
      }
    for (k = 0; k < 3; k++)
      swe_ctx_free(cc[k]);
  }
  return bad;
}

int main(int argc, char **argv)
{
  double tq[105];
  int nq, m, b, f, k, i, bad = 0, n = 0, err = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: framestep EPHEDIR\n");
    return 2;
  }
  printf("G139: the speeds across the steps of the frame's models are each side's rate\n");
  for (m = 0; m < NSET; m++) {
    char mod[40];
    swe_ctx *c = swe_ctx_new();
    TS = SET[m].ts;
    LEFT = SET[m].left;
    FL = SEFLG_JPLEPH | SET[m].fl;
    nq = instants(tq);
    strcpy(mod, SET[m].mod);
    swe_set_ephe_path_r(c, argv[1]);
    swe_set_jpl_file_r(c, "de200.eph");
    swe_set_topo_r(c, 8.55, 47.37, 400);
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    swe_set_delta_t_userdef_r(c, ldexp(1, -11));
    swe_set_astro_models_r(c, mod, 0);
    printf("  (a) step JD %.1f, models %s: worst |speed - reference|, deg/day, %s / %s / %s\n",
           TS, SET[m].mod, RN[0], RN[1], RN[2]);
    for (b = 0; b < NB; b++)
      for (f = 0; f < 2; f++) {
        int32 xf = f ? SEFLG_EQUATORIAL : 0;
        double w[3] = { 0, 0, 0 }, lim = (b == B_SPEED3) ? 1e-5 : 1e-8;
        for (i = 0; i < nq; i++) {
          int skip;
          double d = off(c, b, tq[i], xf, &skip, &err);
          if (err) {
            swe_ctx_free(c);
            printf("G139 FAIL: ERROR\n");
            return 1;
          }
          if (skip)
            continue;
          if (d > w[span(tq[i])])
            w[span(tq[i])] = d;
        }
        printf("    %-16s %s  %.2e  %.2e  %.2e\n", BN[b], f ? "equ" : "ecl", w[0], w[1], w[2]);
        for (k = 0; k < 3; k++) {
          n++;
          if (!(w[k] <= lim)) {
            bad++;
            printf("    FAIL %s %s, %s: %.2e > %.0e\n", BN[b], f ? "equatorial" : "ecliptic",
                   RN[k], w[k], lim);
          }
        }
      }
    swe_ctx_free(c);
  }
  {
    /* check (c): the bodies whose own frame turning is differenced */
    static const struct {
      double ts;
      int left;
      int32 fl;
      const char *mod;
      int ltcell, nb, b[4];
    } SC[] = {
      /* the Moshier Moon (UPSTREAM-BUGS 184) at IAU 1976's window edges,
       * the IERS 2010 sidereal time */
      { 2524595.0, 1, SEFLG_MOSEPH, "0,9,1,0,0,0,0,3", 0, 2, { B_MOON, B_TOPOMOON } },
      { 2378495.0, 0, SEFLG_MOSEPH, "0,9,1,0,0,0,0,3", 0, 2, { B_MOON, B_TOPOMOON } },
      /* the elements of the equinox of date (UPSTREAM-BUGS 185): at - 2
       * centuries on de200 (the Swiss files join a day and a half after
       * it, where their Earth steps; Vulcan, behind the Sun's disc there,
       * is left to the Swiss files at + 2 centuries) */
      { 2378495.0, 0, SEFLG_JPLEPH, "0,9,1,0,0,0,0,3", 1, 3, { B_SELENA, B_ORBGEO, B_ORBHEL } },
      { 2524595.0, 1, SEFLG_SWIEPH, "0,9,1,0,0,0,0,3", 1, 4,
        { B_SELENA, B_VULCAN, B_ORBGEO, B_ORBHEL } },
    };
    for (m = 0; m < (int) (sizeof SC / sizeof SC[0]); m++) {
      char mod[40];
      swe_ctx *c = swe_ctx_new();
      TS = SC[m].ts;
      LEFT = SC[m].left;
      FL = SC[m].fl;
      LTCELL = SC[m].ltcell;
      nq = instants(tq);
      strcpy(mod, SC[m].mod);
      swe_set_ephe_path_r(c, argv[1]);
      swe_set_jpl_file_r(c, "de200.eph");
      swe_set_topo_r(c, 8.55, 47.37, 400);
      swe_set_delta_t_userdef_r(c, ldexp(1, -11));
      swe_set_astro_models_r(c, mod, 0);
      printf("  (c) step JD %.1f, flags 0x%x, models %s: worst |speed - reference|, deg/day,"
             " %s / %s / %s\n", TS, (unsigned) FL, SC[m].mod, RN[0], RN[1], RN[2]);
      for (i = 0; i < SC[m].nb; i++)
        for (f = 0; f < 2; f++) {
          int32 xf = f ? SEFLG_EQUATORIAL : 0;
          double w[3] = { 0, 0, 0 };
          int q;
          b = SC[m].b[i];
          for (q = 0; q < nq; q++) {
            int skip;
            double d = off(c, b, tq[q], xf, &skip, &err);
            if (err) {
              swe_ctx_free(c);
              printf("G139 FAIL: ERROR\n");
              return 1;
            }
            if (skip)
              continue;
            if (d > w[span(tq[q])])
              w[span(tq[q])] = d;
          }
          printf("    %-16s %s  %.2e  %.2e  %.2e\n", BN[b], f ? "equ" : "ecl", w[0], w[1], w[2]);
          for (k = 0; k < 3; k++) {
            n++;
            if (!(w[k] <= 1e-8)) {
              bad++;
              printf("    FAIL %s %s, %s: %.2e > 1e-8\n", BN[b], f ? "equatorial" : "ecliptic",
                     RN[k], w[k]);
            }
          }
        }
      swe_ctx_free(c);
    }
    LTCELL = 0;
  }
#ifndef SWE_UPSTREAM_COMPAT
  {
    /* check (b): the rates themselves */
    static const struct {
      double ts;
      int left;
      int32 fl;
      const char *mod;
      double lim;
    } SB[] = {
      /* SEFLG_JPLHOR's own steps, both Owen's at their instant */
      { 2378131.5, 1, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-8 },
      { 2525323.5, 0, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-8 },
      /* the short-term windows' edges, the window's at its instant: IAU
       * 1976 and 2000 at J2000 -+ 2 centuries, IAU 2006 at -+ 75; with a
       * long-term model differenced too (IAU 2006, "0,8,1"); and under
       * SEMOD_JPLHORA_3 after 1962 */
      { 2378495.0, 0, 0, "0,9,1,0,0,0,0,0", 1e-8 },
      { 2524595.0, 1, 0, "0,9,1,0,0,0,0,0", 1e-8 },
      { 2524595.0, 1, 0, "0,9,6,0,0,0,0,0", 1e-8 },
      { -287830.0, 0, 0, "0,9,8,0,0,0,0,0", 1e-8 },
      { 5190920.0, 1, 0, "0,9,8,0,0,0,0,0", 1e-8 },
      { 2378495.0, 0, 0, "0,8,1,0,0,0,0,0", 1e-8 },
      { 2524595.0, 1, SEFLG_JPLHOR_APPROX, "0,9,1,0,0,0,0,0", 1e-8 },
      /* Owen 1990's pieces, the later piece's at their joins: chosen as
       * the long-term model ("0,10") at all four, and SEFLG_JPLHOR's
       * outside 1799-2202 at the three where it steps; within 1e-9, as
       * the residuals they had sit about the 1e-8 floor (9.8e-9 to 2.6e-8)
       * and the rates come within 1e-11 */
      { -1931455.5, 0, 0, "0,10,0,0,0,0,0,0", 1e-9 },
      { 990544.5, 0, 0, "0,10,0,0,0,0,0,0", 1e-9 },
      { 3912544.5, 0, 0, "0,10,0,0,0,0,0,0", 1e-9 },
      { 6834544.5, 0, 0, "0,10,0,0,0,0,0,0", 1e-9 },
      { -1931455.5, 0, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-9 },
      { 3912544.5, 0, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-9 },
      { 6834544.5, 0, SEFLG_JPLHOR, "0,0,0,0,0,0,0,0", 1e-9 },
    };
    static const char *gn[2] = { "precession", "obliquity" };
    int g;
    for (m = 0; m < (int) (sizeof SB / sizeof SB[0]); m++) {
      char mod[40];
      swe_ctx *c = swe_ctx_new();
      strcpy(mod, SB[m].mod);
      swe_set_astro_models_r(c, mod, 0);
      TS = SB[m].ts;
      LEFT = SB[m].left;
      printf("  (b) flags 0x%x, models %s, step JD %.1f: worst |rate - reference|, deg/day,"
             " %s / %s / %s\n", (unsigned) SB[m].fl, SB[m].mod, TS, RN[0], RN[1], RN[2]);
      for (g = 0; g < 2; g++) {
        double w[3];
        check_b(c, g, SB[m].fl, w);
        printf("    %-16s      %.2e  %.2e  %.2e\n", gn[g], w[0], w[1], w[2]);
        for (k = 0; k < 3; k++) {
          n++;
          if (!(w[k] <= SB[m].lim)) {
            bad++;
            printf("    FAIL flags 0x%x, models %s, %s, %s: %.2e > %.0e\n",
                   (unsigned) SB[m].fl, SB[m].mod, gn[g], RN[k], w[k], SB[m].lim);
          }
        }
      }
      swe_ctx_free(c);
    }
  }
#endif
  bad += check_d(&n);
  bad += check_e(argv[1], &n);
  bad += check_f(argv[1], &n);
  if (bad) {
    printf("G139 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG139 PASS\n", n);
  return 0;
}
