/* orbelbias.c -- G126: swe_calc_orbel() turns the frame bias as the
 * fictitious bodies do, so its places are those of the same elements read
 * from seorbel.txt, and its geocentric places its heliocentric ones less
 * the Earth.
 *
 * The defect (UPSTREAM-BUGS 130): app_pos_etc_orbel() is this fork's copy
 * of the fictitious bodies' path, app_pos_etc_plan_osc(), made before bug
 * 20 paired the frame bias there, and it never got the pair. The elements'
 * state is dynamical J2000 and the Sun and the Earth added to it are ICRS
 * (DE403 and later, Moshier, the Swiss files), and nothing turned either:
 * a J2000 answer was seen from the ICRS Earth. Every geocentric and
 * topocentric place was off by (I - B)(S - E), 3.2e-8 to 1.1e-7 AU by the
 * Earth's direction whatever the orbit, a barycentric one by (I - B) S, an
 * Earth-centred orbit's heliocentric one by the whole Earth's turn, and
 * with SEFLG_ICRS the body itself stayed unturned from every centre.
 *
 * And (UPSTREAM-BUGS 135): swe_calc() refuses every barycentric Moshier
 * request ("barycentric Moshier positions are not supported."), in
 * swecalc(), which swe_calc_orbel() does not pass through: it answered the
 * heliocentric place, Moshier's origin being the Sun, under a return flag
 * with SEFLG_BARYCTR set (18180 with SEFLG_SPEED), 5.7e-3 AU off the Swiss
 * barycentric place, 0.29 deg at a = 1.2 AU.
 *
 * And (UPSTREAM-BUGS 136): elements of the equinox of date
 * (SE_ORBEL_EQ_DATE) are referred to a frame that turns with t, which
 * seorbel.txt's JDATE path evaluates at the instant it reads, under light
 * time the emission instant (the ephemeris protocol's 3.5a, G27), with the
 * frame's turning in the speed (bug 48, G48). The orbel path held the
 * equinox at the observation instant through its light-time loop and left
 * the turning out of the speed.
 *
 * Four checks, no golden file:
 *   1. swe_calc_orbel() against swe_calc() of the same elements written
 *      to a seorbel.txt in a temp dir (as G27 does): eight orbits (a =
 *      1.2 AU; 30 AU on the B1950 equinox; 80000 AU circular; 0.01 AU
 *      about the Earth; and the four with their elements of the equinox
 *      of date, JDATE in the file), five dates 1700-2150 spread over the
 *      year, the four centres (geo, topo, helio, bary), twelve flag sets
 *      (ecliptic of date with and without speed, J2000, ICRS, equatorial,
 *      equatorial XYZ J2000 and ICRS, ecliptic XYZ, true J2000 XYZ, no
 *      aberration or deflection, no nutation, sidereal), on the Swiss
 *      files, de200 and Moshier, and a DE4xx JPL file if one is named: 240
 *      rows an orbit, 180 on Moshier, whose 60 barycentric ones both refuse
 *      with swe_calc()'s message. Each element set is written with the date
 *      as its epoch and its mean anomaly as M0 + n T, n the two-body motion
 *      of this library's KGAUSS (KGAUSS_EARTH), so the two paths evaluate
 *      the same orbit to the last bit at the date and move it alike over
 *      the light time. Positions within 1e-11 deg, speeds 1e-10 deg/day
 *      (distances and XYZ as their angle, d / r).
 *   2. the frame: the geocentric true J2000 equatorial XYZ place and
 *      velocity of each Sun-centred orbit equal its heliocentric ones less
 *      swe_calc()'s heliocentric Earth, the barycentric ones the
 *      heliocentric plus swe_calc()'s barycentric Sun, and the Earth-centred
 *      orbit's heliocentric ones its geocentric plus the Earth, to 1e-14 of
 *      the distance (AU, AU/day; of 1 AU below it).
 *   3. the files missing (the temp dir alone on the path), strict and
 *      with the fallback allowed: each orbit at each date, Moshier, Swiss
 *      and JPL, geocentric, heliocentric and barycentric, with speed: 720
 *      calls, the same return flag and message as swe_calc() of the same
 *      elements, and where both answer the same place (the limits of 1).
 *      Strict, Swiss and JPL are refused by both for the missing files and
 *      Moshier's barycentre for itself; with the fallback both answer from
 *      Moshier and refuse every barycentric request, Swiss and JPL ones
 *      fallen to Moshier as Moshier's (UPSTREAM-BUGS 139; before it, both
 *      answered those with the heliocentric place under SEFLG_BARYCTR).
 *   4. the orbits of date: the speed against a five-point difference of
 *      the positions over 2^-7 day (2^-10 about the Earth) at a
 *      binary-fraction instant near each date, the mean anomaly moved along
 *      the mean motion, six flag sets (ecliptic of date, J2000, equatorial,
 *      ecliptic XYZ true, no aberration or deflection, sidereal), every
 *      centre but Moshier's barycentre and, about the Earth, the
 *      topocentric one (a residual of its own, 1.8e-8 to 4.6e-8 deg/day on
 *      every equinox and on the seorbel.txt path alike): within 1e-8
 *      deg/day (distances and XYZ as their angle).
 *
 * Measured before the fix, on the Swiss files, Moshier and DE431 alike:
 * check 1 off by 1.1e-5 to 1.45e-5 deg geocentric and topocentric at a =
 * 1.2 AU (1.5e-7 deg/day), 2.0e-7 deg at 30 AU, 6.2e-10 deg at 80000 AU,
 * 3.2e-8 deg barycentric at 1.2 AU, the Earth-centred orbit 6.1e-6 deg
 * heliocentric and barycentric, and with SEFLG_ICRS 4.9e-6 to 6.1e-6 deg
 * from every centre (the bias's 23 mas); check 2 by up to 7.1e-8 of the
 * distance at 1.2 AU (1.1e-7 AU), 8.5e-8 AU about the Earth. de200, an
 * ephemeris without a frame bias, passed both: 16 failures, 24 with
 * DE431. After: check 1 within 6.8e-13 deg and 1.0e-11 deg/day (that on
 * the Earth-centred orbit, the two paths' light-time rates, the same
 * before the fix and on de200), check 2 within 3.6e-16 and 6.7e-18 of the
 * distance. The upstream build (SWE_UPSTREAM_COMPAT: upstream's
 * fictitious path without bug 20's pair, swe_calc_orbel() following it)
 * fails 20, 28 with DE431: check 2 by the same 7.1e-8 on the ephemerides
 * with a bias (de200 passes), and check 1 on every orbit and ephemeris,
 * by 0.02 to 0.04 deg (its own light-time and speed defects, G27's and
 * G48's) or by an infinity on the first call (bug 88's zero obliquity).
 * Before bug 135's fix the Moshier barycentre, 60 rows an orbit in check
 * 1 and 40 calls in check 3 (the first four orbits, check 3's 360), was
 * answered (18180) where swe_calc() refuses: 5 failures, the rest passing.
 * Before bug 136's fix the four orbits of date were off in check 1 by
 * 3.5e-7 deg at a = 1.2 AU, 6.9e-6 deg at 30 AU, 1.9e-2 deg at 80000 AU
 * (462 days of the frame's turning) and 2.9e-9 deg about the Earth, and
 * by 4.0e-5 to 8.8e-5 deg/day in speed, the same in check 4, on every
 * ephemeris, and check 3 by those places: 25 failures. After: check 1
 * within 7.1e-13 deg and 1.0e-11 deg/day, Moshier's barycentre refused by
 * both; check 3's 720 calls agree with swe_calc() (320 refused by both,
 * 400 since bug 139's fix),
 * places within 5.1e-13 deg and 1.0e-11 deg/day; check 4 within 4.6e-9
 * deg/day (the 1.2 AU orbit topocentric; 2.4e-9 about the Earth). The
 * upstream build, whose swe_calc_orbel() keeps answering Moshier's
 * barycentre and holds the equinox of date as its copy of the fictitious
 * path did, fails 62: checks 1 and 2 as above on all eight orbits,
 * Moshier's barycentre on each, check 4 on the orbits of date by 9.3e-5
 * to 4.3e-4 deg/day, and check 3 by it and bug 88's infinity.
 * Run as
 *   orbelbias EPHEDIR [DE4XX_FILE]
 * DE4XX_FILE a path to a JPL file of DE number 403 or above (SE_TEST_DE4
 * in the Makefile).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

#define NB 8
#define ND 5
#define RAD2DEG (180 / M_PI)

/* the last four the first four's elements of the equinox of date */
static const struct {
  double M0, a, e, w, N, i;
  int geo, eq;
} B[NB] = {
  {40, 1.2, 0.2, 30, 80, 12, 0, SE_ORBEL_EQ_J2000},
  {200, 30, 0.05, 100, 200, 5, 0, SE_ORBEL_EQ_B1950},
  {10, 80000, 0, 0, 0, 0, 0, SE_ORBEL_EQ_J2000},
  {100, 0.01, 0.1, 40, 60, 20, 1, SE_ORBEL_EQ_J2000},
  {40, 1.2, 0.2, 30, 80, 12, 0, SE_ORBEL_EQ_DATE},
  {200, 30, 0.05, 100, 200, 5, 0, SE_ORBEL_EQ_DATE},
  {10, 80000, 0, 0, 0, 0, 0, SE_ORBEL_EQ_DATE},
  {100, 0.01, 0.1, 40, 60, 20, 1, SE_ORBEL_EQ_DATE},
};
static const char *bname[NB] = {"a 1.2 AU", "a 30 B1950", "a 80000", "a 0.01 Earth",
                                "a 1.2 date", "a 30 date", "a 80000 date", "a .01 E date"};
static const double JD[ND] = {2341972.8, 2415111.3, 2451727.0, 2460950.7, 2506377.2};

/* the two-body mean motion, deg/day, of this library's constants (sweph.h) */
static double Motion(int ib)
{
  double k = B[ib].geo ? 0.0000298122353216 : 0.01720209895;
  return k / (B[ib].a * sqrt(B[ib].a)) * RAD2DEG;
}

/* the mean anomaly at the date, deg, as the element file's line has it */
static double M0(int ib, int id)
{
  return fmod(B[ib].M0 + 37 * id, 360);
}

/* the elements of date id at jd, the mean anomaly moved along the mean
 * motion (exactly M0 at the date itself) */
static int OrbelAt(int ib, int id, double jd, int32 fl, double *x, char *serr)
{
  double el[6];
  el[0] = M0(ib, id) + Motion(ib) * (jd - JD[id]);
  el[1] = B[ib].a;
  el[2] = B[ib].e;
  el[3] = B[ib].w;
  el[4] = B[ib].N;
  el[5] = B[ib].i;
  return swe_calc_orbel(jd, el, B[ib].geo ? SE_EARTH : SE_SUN, B[ib].eq, 0, fl, x, serr);
}

static int Orbel(int ib, int id, int32 fl, double *x, char *serr)
{
  return OrbelAt(ib, id, JD[id], fl, x, serr);
}

/* check 4, elements of date: the speed against a five-point difference of
 * the positions at a binary-fraction instant near the date, the step 2^-7
 * day (2^-10 about the Earth); the worst, deg/day (distances and XYZ as
 * their angle), into *w */
static int FivePoint(int ib, int id, int32 fl, double *w, char *serr)
{
  static const double off[4] = {-2, -1, 1, 2};
  double jb = floor(JD[id] * 256) / 256, h = B[ib].geo ? 1.0 / 1024 : 1.0 / 128;
  double xc[6], xs[4][6], r;
  int j, k;
  if (OrbelAt(ib, id, jb, fl, xc, serr) < 0)
    return -1;
  for (k = 0; k < 4; k++)
    if (OrbelAt(ib, id, jb + off[k] * h, fl & ~SEFLG_SPEED, xs[k], serr) < 0)
      return -1;
  r = (fl & SEFLG_XYZ) ? sqrt(xc[0] * xc[0] + xc[1] * xc[1] + xc[2] * xc[2]) : xc[2];
  for (j = 0; j < 3; j++) {
    double y[4], d;
    for (k = 0; k < 4; k++)
      y[k] = (!(fl & SEFLG_XYZ) && j == 0) ? xc[0] + remainder(xs[k][0] - xc[0], 360) : xs[k][j];
    d = xc[j + 3] - (y[0] - 8 * y[1] + 8 * y[2] - y[3]) / (12 * h);
    if ((fl & SEFLG_XYZ) || j == 2)
      d = d / r * RAD2DEG;
    if (fabs(d) > *w)
      *w = fabs(d);
  }
  return 0;
}

/* the worst of a position (j < 3) or speed (j >= 3) difference, deg or
 * deg/day, distances and XYZ as their angle d / r */
static void Diff(int32 fl, const double *a, const double *b, double *wp, double *wv)
{
  double r = (fl & SEFLG_XYZ) ? sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]) : a[2];
  int j;
  for (j = 0; j < 6; j++) {
    double d = a[j] - b[j];
    if (j == 3 && !(fl & SEFLG_SPEED))
      break;
    if (!(fl & SEFLG_XYZ) && j == 0)
      d = remainder(d, 360);
    if ((fl & SEFLG_XYZ) || j == 2 || j == 5)
      d = d / r * RAD2DEG;
    d = fabs(d);
    if (j < 3 && d > *wp)
      *wp = d;
    if (j >= 3 && d > *wv)
      *wv = d;
  }
}

/* one ephemeris: both checks, the failures counted into *bad */
static void Leg(int32 eph, const char *name, int *bad)
{
  static const int32 fls[] = {
    SEFLG_SPEED, 0, SEFLG_SPEED | SEFLG_J2000, SEFLG_SPEED | SEFLG_ICRS,
    SEFLG_SPEED | SEFLG_EQUATORIAL,
    SEFLG_SPEED | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_J2000,
    SEFLG_SPEED | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_ICRS,
    SEFLG_SPEED | SEFLG_XYZ,
    SEFLG_SPEED | SEFLG_TRUEPOS | SEFLG_J2000 | SEFLG_XYZ,
    SEFLG_SPEED | SEFLG_NOABERR | SEFLG_NOGDEFL,
    SEFLG_SPEED | SEFLG_NONUT, SEFLG_SPEED | SEFLG_SIDEREAL,
  };
  static const int32 f5[] = {
    SEFLG_SPEED, SEFLG_SPEED | SEFLG_J2000, SEFLG_SPEED | SEFLG_EQUATORIAL,
    SEFLG_SPEED | SEFLG_TRUEPOS | SEFLG_XYZ, SEFLG_SPEED | SEFLG_NOABERR | SEFLG_NOGDEFL,
    SEFLG_SPEED | SEFLG_SIDEREAL,
  };
  static const int32 ctrs[] = {0, SEFLG_TOPOCTR, SEFLG_HELCTR, SEFLG_BARYCTR};
  const int32 tf = eph | SEFLG_SPEED | SEFLG_TRUEPOS | SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ;
  int nfl = (int) (sizeof fls / sizeof fls[0]), n5 = (int) (sizeof f5 / sizeof f5[0]);
  int ib, id, ic, ifl, j;
  char serr[256], serr2[256];
  for (ib = 0; ib < NB; ib++) {
    double wp = 0, wv = 0, wf = 0, wfv = 0;
    int nrow = 0, nerr = 0, nref = 0, nbary = 0;
    for (id = 0; id < ND; id++) {
      double xg[6], xh[6], xb[6], xe[6], xs[6], r;
      for (ic = 0; ic < 4; ic++)
        for (ifl = 0; ifl < nfl; ifl++) {
          int32 fl = eph | fls[ifl] | ctrs[ic], r1, r2;
          double xo[6], xf[6];
          *serr = *serr2 = '\0';
          r1 = Orbel(ib, id, fl, xo, serr);
          r2 = swe_calc(JD[id], SE_FICT_OFFSET + ib * ND + id, fl, xf, serr2);
          /* Moshier's barycentre: swe_calc() refuses it, and so must
           * swe_calc_orbel(), with the same message (bug 135) */
          if ((eph & SEFLG_MOSEPH) && ctrs[ic] == SEFLG_BARYCTR) {
            nref++;
            if (r1 >= 0 || r2 >= 0 || strcmp(serr, serr2) != 0) {
              if (nbary++ == 0)
                printf("    %s jd %.1f flags %d: orbel %d \"%s\", seorbel.txt %d \"%s\"\n",
                       bname[ib], JD[id], fl, r1, serr, r2, serr2);
            }
            continue;
          }
          if (r1 < 0 || r2 < 0) {
            if (nerr++ == 0)
              printf("    %s jd %.1f flags %d: orbel %d \"%s\", seorbel.txt %d \"%s\"\n",
                     bname[ib], JD[id], fl, r1, serr, r2, serr2);
            continue;
          }
          nrow++;
          Diff(fl, xo, xf, &wp, &wv);
        }
      /* the frame: geo = helio - Earth, bary = helio + Sun; about the
       * Earth, helio = geo + Earth */
      if (Orbel(ib, id, tf, xg, serr) < 0 || Orbel(ib, id, tf | SEFLG_HELCTR, xh, serr) < 0
          || swe_calc(JD[id], SE_EARTH, tf | SEFLG_HELCTR, xe, serr) < 0) {
        printf("    %s jd %.1f: %s\n", bname[ib], JD[id], serr);
        nerr++;
        continue;
      }
      r = sqrt(xh[0] * xh[0] + xh[1] * xh[1] + xh[2] * xh[2]);
      if (r < 1)
        r = 1;
      for (j = 0; j < 6; j++) {
        double d = B[ib].geo ? xh[j] - (xg[j] + xe[j]) : xg[j] - (xh[j] - xe[j]);
        d = fabs(d) / r;
        if (j < 3 && d > wf)
          wf = d;
        if (j >= 3 && d > wfv)
          wfv = d;
      }
      if (!(eph & SEFLG_MOSEPH)) {
        if (Orbel(ib, id, tf | SEFLG_BARYCTR, xb, serr) < 0
            || swe_calc(JD[id], SE_SUN, tf | SEFLG_BARYCTR, xs, serr) < 0) {
          printf("    %s jd %.1f: %s\n", bname[ib], JD[id], serr);
          nerr++;
          continue;
        }
        for (j = 0; j < 6; j++) {
          double d = fabs(xb[j] - (xh[j] + xs[j])) / r;
          if (j < 3 && d > wf)
            wf = d;
          if (j >= 3 && d > wfv)
            wfv = d;
        }
      }
    }
    printf("  %-6s %-12s %3d rows: vs seorbel.txt %.1e deg %.1e deg/day;"
           " geo/helio/bary %.1e %.1e of r\n", name, bname[ib], nrow, wp, wv, wf, wfv);
    if (nref)
      printf("  %-6s %-12s %3d barycentric rows refused by both, %d not\n",
             name, bname[ib], nref - nbary, nbary);
    if (nbary) {
      printf("    FAIL: Moshier's barycentre answered, or refused otherwise than by swe_calc()\n");
      (*bad)++;
    }
    if (nerr) {
      printf("    FAIL: %d calls refused by one path only, or failed\n", nerr);
      (*bad)++;
    }
    if (nrow == 0 || wp > 1e-11 || wv > 1e-10) {
      printf("    FAIL: not the same elements' places from seorbel.txt (1e-11, 1e-10)\n");
      (*bad)++;
    }
    if (wf > 1e-14 || wfv > 1e-14) {
      printf("    FAIL: the geocentric place is not the heliocentric less the Earth (1e-14)\n");
      (*bad)++;
    }
    /* check 4: elements of date, the speed the rate of the positions
     * (bug 136); every centre but Moshier's barycentre, and but the
     * topocentric one about the Earth, whose speed has a residual of its
     * own on every equinox and on the seorbel.txt path alike */
    if (B[ib].eq == SE_ORBEL_EQ_DATE) {
      double w5 = 0;
      int n5row = 0, n5err = 0;
      for (id = 0; id < ND; id++)
        for (ic = 0; ic < 4; ic++)
          for (ifl = 0; ifl < n5; ifl++) {
            if (((eph & SEFLG_MOSEPH) && ctrs[ic] == SEFLG_BARYCTR)
                || (B[ib].geo && ctrs[ic] == SEFLG_TOPOCTR))
              continue;
            if (FivePoint(ib, id, eph | f5[ifl] | ctrs[ic], &w5, serr) < 0) {
              if (n5err++ == 0)
                printf("    %s jd %.1f: %s\n", bname[ib], JD[id], serr);
              continue;
            }
            n5row++;
          }
      printf("  %-6s %-12s %3d rows: speeds against five-point differences %.1e deg/day\n",
             name, bname[ib], n5row, w5);
      if (n5err || n5row == 0 || w5 > 1e-8) {
        printf("    FAIL: the speed is not the rate of the positions (1e-8)\n");
        (*bad)++;
      }
    }
  }
}

/* check 3: the files missing (the element file's directory alone), strict
 * and with the fallback allowed: swe_calc_orbel() answers or refuses as
 * swe_calc() does for the same flags, with the same return flag, message
 * and place (bugs 112, 120, 135 and 139) */
static void MissLeg(const char *dir, int *bad)
{
  static const int32 ephs[] = {SEFLG_MOSEPH, SEFLG_SWIEPH, SEFLG_JPLEPH};
  static const int32 ctrs[] = {0, SEFLG_HELCTR, SEFLG_BARYCTR};
  int fb0 = swe_get_ephe_fallback(), fb, ie, ic, ib, id, nrow = 0, nfail = 0, nref = 0;
  double wp = 0, wv = 0;
  char serr[256], serr2[256];
  swe_close();
  swe_set_ephe_path(dir);
  for (fb = 0; fb < 2; fb++) {
    swe_set_ephe_fallback(fb);
    for (ib = 0; ib < NB; ib++)
      for (id = 0; id < ND; id++)
        for (ie = 0; ie < 3; ie++)
          for (ic = 0; ic < 3; ic++) {
            int32 fl = ephs[ie] | ctrs[ic] | SEFLG_SPEED, r1, r2;
            double xo[6], xf[6];
            *serr = *serr2 = '\0';
            r1 = Orbel(ib, id, fl, xo, serr);
            r2 = swe_calc(JD[id], SE_FICT_OFFSET + ib * ND + id, fl, xf, serr2);
            nrow++;
            if (r1 != r2 || strcmp(serr, serr2) != 0) {
              if (nfail++ == 0)
                printf("    %s jd %.1f flags %d fallback %d: orbel %d \"%s\", seorbel.txt %d \"%s\"\n",
                       bname[ib], JD[id], fl, fb, r1, serr, r2, serr2);
              continue;
            }
            if (r1 < 0)
              nref++;
            else
              Diff(fl, xo, xf, &wp, &wv);
          }
  }
  swe_set_ephe_fallback(fb0);
  printf("  files missing, strict and fallback: %d rows (%d refused by both), %d otherwise than"
         " swe_calc(); places %.1e deg %.1e deg/day\n", nrow, nref, nfail, wp, wv);
  if (nfail) {
    printf("    FAIL: a return or message not swe_calc()'s for the same flags\n");
    (*bad)++;
  }
  if (nref == nrow || wp > 1e-11 || wv > 1e-10) {
    printf("    FAIL: not the same elements' places from seorbel.txt (1e-11, 1e-10)\n");
    (*bad)++;
  }
}

int main(int argc, char **argv)
{
  char dir[] = "/tmp/orbelbiasXXXXXX", path[1024], file[256];
  const char *de4 = argc > 2 ? argv[2] : "";
  int ib, id, bad = 0;
  FILE *pf;
  if (argc < 2) {
    fprintf(stderr, "usage: orbelbias EPHEDIR [DE4XX_FILE]\n");
    return 2;
  }
  if (mkdtemp(dir) == NULL) {
    perror("mkdtemp");
    return 2;
  }
  snprintf(file, sizeof file, "%s/seorbel.txt", dir);
  if ((pf = fopen(file, "w")) == NULL) {
    perror(file);
    return 2;
  }
  /* one line a body and date: the date the epoch, M0 + n T */
  for (ib = 0; ib < NB; ib++)
    for (id = 0; id < ND; id++)
      fprintf(pf, "%.17g, %s, %.17g + %.17g * T, %.17g, %.17g, %.17g, %.17g, %.17g, b%d_%d%s\n",
              JD[id], B[ib].eq == SE_ORBEL_EQ_B1950 ? "B1950"
                      : B[ib].eq == SE_ORBEL_EQ_DATE ? "JDATE" : "J2000",
              M0(ib, id), Motion(ib) * 36525,
              B[ib].a, B[ib].e, B[ib].w, B[ib].N, B[ib].i, ib, id, B[ib].geo ? ", geo" : "");
  fclose(pf);
  printf("G126: swe_calc_orbel() against seorbel.txt, every centre and frame\n");
  swe_set_topo(8.55, 47.37, 400);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  snprintf(path, sizeof path, "%s:%s", dir, argv[1]);
  swe_set_ephe_path(path);
  Leg(SEFLG_SWIEPH, "Swiss", &bad);
  swe_set_jpl_file("de200.eph");
  Leg(SEFLG_JPLEPH, "de200", &bad);
  Leg(SEFLG_MOSEPH, "Moshier", &bad);
  if (*de4) {
    const char *sl = strrchr(de4, '/');
    if (sl != NULL)
      snprintf(path, sizeof path, "%s:%.*s", dir, (int) (sl - de4), de4);
    else
      snprintf(path, sizeof path, "%s:.", dir);
    swe_set_ephe_path(path);
    swe_set_jpl_file(sl != NULL ? sl + 1 : de4);
    Leg(SEFLG_JPLEPH, "DE4xx", &bad);
  } else
    printf("  DE4xx leg SKIPPED: no JPL file named (SE_TEST_DE4=<path to a DE >= 403 file>)\n");
  MissLeg(dir, &bad);
  swe_close();
  unlink(file);
  rmdir(dir);
  printf(bad ? "G126 FAIL: %d\n" : "G126 PASS\n", bad);
  return bad ? 1 : 0;
}
