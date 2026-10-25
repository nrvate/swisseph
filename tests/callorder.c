/* callorder.c -- G83: a position does not depend on SEFLG_SPEED, on what
 * the context computed before, or on which ephemeris answered last.
 *
 * The defects (UPSTREAM-BUGS 84):
 * - jplplan()'s Earth and Sun caches answered for whatever ephemeris had
 *   filled the two slots at tjd. free_planets() runs when the ephemeris
 *   asked for changes, not when a call answers from another, and a JPL call
 *   whose light time leaves the file falls to Moshier -- answered with
 *   swe_set_ephe_fallback(1), refused without -- leaving the series'
 *   heliocentric Earth in the slot at tjd. The next JPL call there took it
 *   for the barycentric Earth, and returned its answer flagged JPL: after
 *   Pluto 0.125 day into DE200, the Moon 160.1 deg off, Mercury 0.30, Mars
 *   0.090, Jupiter 0.010, either way;
 * - swe_calc()'s JPL barycentric Sun read the Sun into its slot without
 *   the slot's ephemeris and finished it at the Earth slot's instant, which
 *   it never set: on a fresh context its speeds were NaN (sidereal, every
 *   coordinate; upstream there took JD 0, 270.004 for 177.493), after a call
 *   3000 days on the ayanamsa was that instant's (177.378, 0.11 deg off),
 *   and a heliocentric asteroid after it, its Sun of no ephemeris, kept its
 *   barycentric place (Pallas 0.24 deg, Juno 0.14, Chiron 0.033).
 * - (latent) app_pos_etc_plan() built the aberration rate's observer from
 *   an Earth JPL and Moshier never read for a heliocentric or barycentric
 *   request. plaus_iflag() turns aberration off for both, and with the
 *   function's locals poisoned with NaN every result was bit-identical, so
 *   nothing reached an answer; no check here can see it.
 *
 * The checks, bit for bit (memcmp of the six doubles and the return flag):
 *   1. fresh context, the position with SEFLG_SPEED equals the one without:
 *      25 bodies (Sun-Pluto, Moon, the four nodes and apsides, both
 *      interpolated apsides, Earth, Chiron-Vesta, Cupido) x 11 flag sets
 *      (geocentric, helio-, barycentric, topocentric, J2000, true
 *      positions, no aberration, sidereal, equatorial XYZ, ICRS, centre of
 *      body) x the Swiss files, Moshier and DE200 x four instants
 *      1700-2050 away from the files' 600-year joins;
 *   2. with and without SEFLG_SPEED, the answer of a context warmed first
 *      by one of eleven calls (the Sun, its barycentric place plain and
 *      sidereal, the Moon, Mercury, the same body with the speed flag the
 *      other way, the same body 1.5 and 3000 days off, heliocentric
 *      Chiron, Jupiter's centre of body, the topocentric Moon) equals the
 *      fresh context's, at J2000;
 *   3. the JPL answers at DE200's start + 0.125 day after a JPL Pluto,
 *      whose light time leaves the file, with the fallback refused and
 *      allowed: the Sun, Moon, Mercury, Mars, Jupiter, Chiron and the
 *      heliocentric Earth equal a fresh context's;
 *   4. one instant through a sequence of per-call ephemeris switches
 *      (Swiss, JPL, Moshier, the barycentric Sun, heliocentric asteroids,
 *      the fallback turned on and off, a JPL file that is not there and the
 *      real one set again): every answer equals a fresh context's;
 *   and every coordinate of every answer is finite.
 * Measured: 1-4 hold to the bit, 0 of 20001 rows differ. ts20 before the
 * fix fails 42 (1: the fresh barycentric Sun's NaN speeds, 4; 2: 23, the
 * JPL heliocentric asteroids after the barycentric Sun and that Sun NaN in
 * every warm state; 3: 12; 4: 3); without the ephemeris key alone 12 (3),
 * without the barycentric Sun's Earth alone 18 (its NaN). The upstream
 * build fails 2027 (1: 1903, its SEFLG_SPEED dependences that earlier
 * entries fixed; 2: 114; 3: 8; 4: 2). Left out because they are another
 * defect: instants where the
 * light time or the warm-up crosses a 600-year join, where an open file is
 * kept past its nominal end (47.6 mas on the barycentric Sun, 8.7 on Juno
 * at JD 2378496.6 after a call 1.5 days before). Run as
 *   callorder EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static const char *EPHE;
static int nrow, nbad, nnan;

static swe_ctx *fresh(int fallback)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_topo_r(c, 8.55, 47.37, 400);
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  swe_set_ephe_fallback_r(c, fallback);
  return c;
}

struct ans { int32 r; double x[6]; };

static struct ans calc(swe_ctx *c, double jd, int ipl, int32 fl)
{
  struct ans a;
  char serr[256];
  memset(&a, 0, sizeof a);
  a.r = swe_calc_r(c, jd, ipl, fl, a.x, serr);
  if (a.r < 0)
    memset(a.x, 0, sizeof a.x);
  return a;
}

static struct ans fresh_calc(double jd, int ipl, int32 fl, int fallback)
{
  swe_ctx *c = fresh(fallback);
  struct ans a = calc(c, jd, ipl, fl);
  swe_ctx_free(c);
  return a;
}

/* n doubles of a and b, and the flags, the same bits */
static void same(const char *what, const struct ans *a, const struct ans *b, int n,
                 double jd, int ipl, int32 fl)
{
  int i;
  nrow++;
  for (i = 0; i < 6; i++)
    if (!isfinite(a->x[i]) || !isfinite(b->x[i])) {
      if (nnan++ < 5)
        printf("  NaN: %s, body %d, flags %#x, JD %.6f\n", what, ipl, (unsigned) fl, jd);
      nbad++;
      return;
    }
  if (a->r != b->r || memcmp(a->x, b->x, n * sizeof(double)) != 0) {
    if (nbad < 20)
      printf("  differs: %s, body %d, flags %#x, JD %.6f: %.12f / %.12f (flags %#x / %#x)\n",
             what, ipl, (unsigned) fl, jd, a->x[0], b->x[0], (unsigned) a->r, (unsigned) b->r);
    nbad++;
  }
}

static const int rgBody[] = {
  SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER, SE_SATURN, SE_URANUS,
  SE_NEPTUNE, SE_PLUTO, SE_MEAN_NODE, SE_TRUE_NODE, SE_MEAN_APOG, SE_OSCU_APOG, SE_EARTH,
  SE_CHIRON, SE_PHOLUS, SE_CERES, SE_PALLAS, SE_JUNO, SE_VESTA, SE_INTP_APOG, SE_INTP_PERG,
  SE_CUPIDO
};
static const int32 rgFl[] = {
  0, SEFLG_HELCTR, SEFLG_BARYCTR, SEFLG_TOPOCTR, SEFLG_J2000 | SEFLG_NONUT, SEFLG_TRUEPOS,
  SEFLG_NOABERR, SEFLG_SIDEREAL, SEFLG_EQUATORIAL | SEFLG_XYZ, SEFLG_ICRS, SEFLG_CENTER_BODY
};
static const int32 rgEph[] = {SEFLG_SWIEPH, SEFLG_MOSEPH, SEFLG_JPLEPH};
#define NBODY ((int) (sizeof rgBody / sizeof rgBody[0]))
#define NFL ((int) (sizeof rgFl / sizeof rgFl[0]))

/* the warm-up calls of check 2 */
static void warm(swe_ctx *c, int k, double jd, int ipl, int32 fl, int32 eph)
{
  switch (k) {
  case 0: calc(c, jd, SE_SUN, eph | SEFLG_SPEED); break;
  case 1: calc(c, jd, SE_SUN, eph | SEFLG_SPEED | SEFLG_BARYCTR); break;
  case 2: calc(c, jd + 3000, SE_SUN, eph | SEFLG_SPEED | SEFLG_BARYCTR | SEFLG_SIDEREAL); break;
  case 3: calc(c, jd, SE_MOON, eph | SEFLG_SPEED); break;
  case 4: calc(c, jd, SE_MERCURY, eph); break;
  case 5: calc(c, jd, ipl, fl ^ SEFLG_SPEED); break;
  case 6: calc(c, jd - 1.5, ipl, fl); break;
  case 7: calc(c, jd + 3000, ipl, fl); break;
  case 8: calc(c, jd, SE_CHIRON, eph | SEFLG_SPEED | SEFLG_HELCTR); break;
  case 9: calc(c, jd, SE_JUPITER, eph | SEFLG_SPEED | SEFLG_CENTER_BODY); break;
  case 10: calc(c, jd, SE_MOON, eph | SEFLG_TOPOCTR); break;
  }
}
#define NWARM 11

int main(int argc, char **argv)
{
  const double rgJd[4] = {
    2451545.0, floor(2433282.4 * 256) / 256, floor(2469807.7 * 256) / 256,
    floor(2341972.6 * 256) / 256
  };
  const double jd0 = 2305424.5 + 0.125;	/* DE200's first day */
  int ie, ib, ifl, id, k, sp, nb1, nb2, nb3;
  if (argc != 2) {
    fprintf(stderr, "usage: callorder EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  printf("G83: positions independent of SEFLG_SPEED, of call order, of the last ephemeris\n");
  /* 1 */
  for (ie = 0; ie < 3; ie++)
    for (ib = 0; ib < NBODY; ib++)
      for (ifl = 0; ifl < NFL; ifl++)
        for (id = 0; id < 4; id++) {
          int32 fl = rgFl[ifl] | rgEph[ie];
          struct ans a = fresh_calc(rgJd[id], rgBody[ib], fl, 0);
          struct ans b = fresh_calc(rgJd[id], rgBody[ib], fl | SEFLG_SPEED, 0);
          if (a.r < 0 && b.r < 0)
            continue;
          b.r &= ~SEFLG_SPEED;
          same("SEFLG_SPEED", &a, &b, 3, rgJd[id], rgBody[ib], fl);
        }
  nb1 = nbad;
  printf("  1. with and without SEFLG_SPEED, fresh: %d rows, %d differ\n", nrow, nb1);
  /* 2 */
  for (ie = 0; ie < 3; ie++)
    for (ib = 0; ib < NBODY; ib++)
      for (ifl = 0; ifl < NFL; ifl++)
        for (id = 0; id < 1; id++)
          for (sp = 0; sp < 2; sp++) {
            int32 fl = rgFl[ifl] | rgEph[ie] | (sp ? SEFLG_SPEED : 0);
            struct ans a = fresh_calc(rgJd[id], rgBody[ib], fl, 0);
            if (a.r < 0)
              continue;
            for (k = 0; k < NWARM; k++) {
              swe_ctx *c = fresh(0);
              struct ans b;
              char what[32];
              warm(c, k, rgJd[id], rgBody[ib], fl, rgEph[ie]);
              b = calc(c, rgJd[id], rgBody[ib], fl);
              swe_ctx_free(c);
              sprintf(what, "warm-up %d", k);
              same(what, &a, &b, 6, rgJd[id], rgBody[ib], fl);
            }
          }
  nb2 = nbad - nb1;
  printf("  2. fresh against warm: %d differ\n", nb2);
  /* 3 */
  {
    static const int rgPl[7] = {SE_SUN, SE_MOON, SE_MERCURY, SE_MARS, SE_JUPITER, SE_CHIRON, SE_EARTH};
    int fb;
    for (fb = 0; fb < 2; fb++)
      for (k = 0; k < 7; k++) {
        int32 fl = SEFLG_JPLEPH | SEFLG_SPEED | (rgPl[k] == SE_EARTH ? SEFLG_HELCTR : 0);
        struct ans a = fresh_calc(jd0, rgPl[k], fl, fb), b;
        swe_ctx *c = fresh(fb);
        calc(c, jd0, SE_PLUTO, SEFLG_JPLEPH | SEFLG_SPEED);
        b = calc(c, jd0, rgPl[k], fl);
        swe_ctx_free(c);
        same(fb ? "after JPL Pluto, fallback allowed" : "after JPL Pluto, refused",
             &a, &b, 6, jd0, rgPl[k], fl);
      }
  }
  nb3 = nbad - nb1 - nb2;
  printf("  3. JPL after a light time out of the file: %d differ\n", nb3);
  /* 4 */
  {
    static const struct { int ipl; int32 fl; int fb; int jpl; } rgSeq[] = {
      {SE_MARS, SEFLG_SWIEPH, 0, 1}, {SE_MOON, SEFLG_JPLEPH, 0, 1},
      {SE_MARS, SEFLG_MOSEPH, 0, 1}, {SE_MERCURY, SEFLG_JPLEPH, 0, 1},
      {SE_SUN, SEFLG_JPLEPH | SEFLG_BARYCTR | SEFLG_SIDEREAL, 0, 1},
      {SE_PALLAS, SEFLG_JPLEPH | SEFLG_HELCTR, 0, 1}, {SE_MOON, SEFLG_SWIEPH, 0, 1},
      {SE_SUN, SEFLG_SWIEPH | SEFLG_BARYCTR, 0, 1}, {SE_CERES, SEFLG_JPLEPH | SEFLG_HELCTR, 0, 1},
      {SE_MOON, SEFLG_MOSEPH, 0, 1}, {SE_JUPITER, SEFLG_JPLEPH, 1, 0},
      {SE_MARS, SEFLG_JPLEPH, 1, 0}, {SE_MOON, SEFLG_JPLEPH, 0, 0},
      {SE_MARS, SEFLG_JPLEPH, 1, 1}, {SE_MOON, SEFLG_JPLEPH, 1, 1},
      {SE_SUN, SEFLG_JPLEPH | SEFLG_BARYCTR, 0, 1}, {SE_VESTA, SEFLG_JPLEPH | SEFLG_HELCTR, 0, 1},
    };
    int n = (int) (sizeof rgSeq / sizeof rgSeq[0]), nb0 = nbad;
    for (sp = 0; sp < 2; sp++) {
      swe_ctx *c = fresh(0);
      int jpl = 1;
      for (k = 0; k < n; k++) {
        int32 fl = rgSeq[k].fl | (sp ? SEFLG_SPEED : 0);
        struct ans a, b;
        swe_ctx *f = fresh(rgSeq[k].fb);
        if (!rgSeq[k].jpl)
          swe_set_jpl_file_r(f, "nosuchfile.eph");
        a = calc(f, rgJd[0], rgSeq[k].ipl, fl);
        swe_ctx_free(f);
        if (rgSeq[k].jpl != jpl) {
          swe_set_jpl_file_r(c, rgSeq[k].jpl ? "de200.eph" : "nosuchfile.eph");
          jpl = rgSeq[k].jpl;
        }
        swe_set_ephe_fallback_r(c, rgSeq[k].fb);
        b = calc(c, rgJd[0], rgSeq[k].ipl, fl);
        same("switch sequence", &a, &b, 6, rgJd[0], rgSeq[k].ipl, fl);
      }
      swe_ctx_free(c);
    }
    printf("  4. a sequence of ephemeris switches: %d differ\n", nbad - nb0);
  }
  printf("  %d rows, %d differ (%d not finite)\n", nrow, nbad, nnan);
  printf(nbad ? "G83 FAIL: %d\n" : "G83 PASS\n", nbad);
  return nbad != 0;
}
