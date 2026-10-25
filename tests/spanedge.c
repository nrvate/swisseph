/* spanedge.c -- G158: at the edge of an ephemeris's span a star, the true
 * node, the osculating apogee and swe_nod_aps()'s points with SEFLG_SPEED
 * answer as without it, and a fictitious body or an orbit of
 * swe_calc_orbel() whose light time leaves the span falls back or is
 * refused as swe_calc()'s planets are.
 *
 * The defects. UPSTREAM-BUGS 206: the star functions take the aberration's
 * rate from the Earth's acceleration, its velocities read at tjd +- 2^-9
 * day (bug 76), and within 2^-9 day of a span's edge one read left the
 * span of the ephemeris that answered at tjd: on de200.eph Aldebaran with
 * SEFLG_JPLEPH | SEFLG_SPEED 2^-10 day inside either end -1, "outside JPL
 * eph. range", and 1 without SEFLG_SPEED (Mars answers 257 at the end); on
 * the Swiss files of 1800-2400 alone 2^-10 day inside either end the
 * downgrade's refusal (strict) where the star answers 2 without the flag,
 * and with the fallback allowed Moshier's heliocentric velocity
 * differenced with the files' barycentric one: 6.6e-4 deg/day on the
 * longitude speed; at a Moon file's end, its Moon Moshier's, refused or
 * 2.4e-9 deg/day. UPSTREAM-BUGS 208: app_pos_etc_plan_osc() and
 * app_pos_etc_orbel() read the Earth and the Sun at the light-time instant
 * by main_planet_bary(), which falls from the Swiss files to Moshier on its
 * own: with the files of 1800-2400 alone Cupido at JD 2378496.6 (0.23 day
 * of light time after sepl_18's start) took Moshier's Sun, 0, for the
 * files' barycentric one, 258 under either setting and 4.33e-3 deg off both
 * the complete directory's and the Moshier request's. UPSTREAM-BUGS 225:
 * the true node and the osculating apogee with SEFLG_SPEED read the Moon at
 * tjd +- k 2^-6 day, k <= 2 (lunar_osc_elem()), and within 2^-5 day of a
 * span's edge the whole answer fell to Moshier: on de200.eph 2^-10 day
 * inside its end the node 260, 88.4083557 deg, for 257, 88.4061487
 * without the flag (its speed 1.89110e-2 deg/day for 1.80947e-2), the
 * apogee 308.7951857 for 308.7784839; strict refused. UPSTREAM-BUGS 226:
 * swe_nod_aps() with SEFLG_SPEED reads the osculating ellipse at tjd +- k
 * h and the observer at tjd less the light time, and near a start fell to
 * Moshier the same way: Pluto's points with the files of 1800-2400 at JD
 * 2378496.625 the node's speed -2.48353e-2 deg/day for -2.48232e-2, strict
 * refused ("jd 2378496.3944"), the barycentric ellipse refused either way.
 *
 * The check, the files of 1800-2400 alone (sepl_18, semo_18, seas_18) and
 * with sepl_24 and seas_24 (the Moon file's end alone) linked in /tmp,
 * de200.eph in EPHEDIR:
 *  (a) Aldebaran and Polaris by swe_fixstar2(), swe_fixstar() and their
 *      _ut forms, geocentric, topocentric, equatorial, J2000 and sidereal,
 *      the fallback allowed and strict, at de200.eph's end, 2^-10 day
 *      inside it, 2^-10 and 2^-20 day inside its start, 2^-10 day inside
 *      the start of sepl_18, 7.5e-4 day inside its end, 4.2e-4 day inside
 *      semo_18's, and at 2000 Jan 1.5 on de200 and the files: the call
 *      with SEFLG_SPEED answers as the one without, its flag with
 *      SEFLG_SPEED, the place bit for bit, serr the same; the place from
 *      the ephemeris asked; the longitude and latitude speeds within 1e-9
 *      deg/day of five-point one-sided differences (inward, 2^-8 day) of
 *      the places without the flag, each answered as the instant's (on the
 *      sky, the longitude's by cos lat; delta t set to 2^-11 day, so the UT
 *      forms take these instants exactly); and the last call of each
 *      process bit for bit a pristine process's.
 *  (b) Cupido, Isis, Poseidon and swe_calc_orbel()'s orbit with a = 40 AU
 *      about the Sun, with and without SEFLG_SPEED, topocentric,
 *      heliocentric, equatorial, at JD 2378496.6 and 2378496.625 with the
 *      files of 1800-2400 and at JD 2305424.625 on de200.eph: the fallback
 *      allowed bit for bit the Moshier request's, its flag too, "Moshier"
 *      noted; strict the downgrade's refusal; under SEFLG_TRUEPOS (no
 *      light time) and at JD 2378497.5 the complete directory's answer bit
 *      for bit, both settings; and in one process Cupido at 2378496.6,
 *      2378497.5 and 2378496.6 again, each as a pristine process.
 *  (c) SE_TRUE_NODE and SE_OSCU_APOG by swe_calc(), geocentric,
 *      equatorial, J2000, sidereal and SEFLG_TRUEPOS, the fallback allowed
 *      and strict, on de200.eph at its end, 2^-10 and 2^-5 + 2^-10 day
 *      inside it, 2^-10 and 2^-20 day inside its start (tjd's own reads
 *      leave the span there: the call with the flag falls back or is
 *      refused as the one without), 7.1e-3 day inside semo_18's start and
 *      4.3e-3 inside its end, and at 2000 Jan 1.5 on de200 and the files:
 *      the flag, place and serr the call's without SEFLG_SPEED, the place
 *      from the ephemeris asked, the speeds within 2e-8 (node) and 5e-7
 *      (apogee) deg/day of nine-point one-sided differences (inward, 2^-4
 *      day) of the places, and the last call of each process a pristine
 *      process's.
 *  (d) swe_nod_aps() for the Sun, the Moon, Mars, Saturn and Pluto, mean,
 *      osculating, SE_NODBIT_OSCU_BAR and SE_NODBIT_FOPOINT, geocentric,
 *      topocentric, heliocentric and barycentric, both settings, on
 *      de200.eph 2^-20, 2^-7 and 2^-3 day inside its start, at its end and
 *      2^-7 day inside it, 2^-7 and 2^-3 day inside sepl_18's start, 7.6e-3
 *      inside its end, 4.3e-3 inside semo_18's, and at 2000 Jan 1.5: the
 *      return, the four places and serr the call's without SEFLG_SPEED, the
 *      speeds within 2e-8 deg/day (the Moon's 3e-7) of nine-point one-sided
 *      differences of the places (2^-5 day, topocentric 2^-6, the Moon's
 *      2^-7; the Sun's points under SEFLG_NOGDEFL, their perigee behind the
 *      Sun's disk at sepl_18's end), and the last call a pristine
 *      process's.
 * Measured: 2951 rows, none fail; (a)'s speeds within 6.4e-11 deg/day of
 * the places' rate, (c)'s within 4.8e-9 (node) and 2.2e-7 (apogee, the
 * reference's own noise: 1.4e-7 at 2000), (d)'s within 1.5e-7 (the Moon;
 * the Sun's 5.9e-9, the planets' 1.8e-9). Before the fixes of 206 and 208
 * 640 fail: (a)'s 560 rows at the edges (refused, or the speed 5.9e-4 to
 * 6.6e-4 deg/day off, 2.4e-9 at the Moon file's end, serr noting Moshier)
 * and (b)'s 80 rows with the files at 2378496.6 and 2378496.625 (258 under
 * either setting); before those of 225 and 226 962 fail, (c) 112 and (d)
 * 850 (Moshier's answer or a refusal; the 2000 rows pass). The upstream
 * build fails 2694: (a) 664, its stars' speeds upstream's (bug 49 and on),
 * (b) 100, (c) 170, (d) 1760.
 *
 * Run as
 *   spanedge EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

#define EPHMASK (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)
#define LIM 1e-9	/* deg/day */

static char EPHE[PATH_MAX];
static char DIR18[] = "/tmp/spanedge18-XXXXXX";	/* sepl_18, semo_18, seas_18 */
static char DIRMO[] = "/tmp/spanedgemo-XXXXXX";	/* and sepl_24, seas_24: semo_18's end */
static int nbad, nrow;
static double maxerr;

/* the answer of a call, as a pipe carries it */
struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; int ran; };

/* (a) */
enum { STAR2, STAR, STAR2_UT, STAR_UT };
static const char *const KSZ[4] = {"swe_fixstar2()", "swe_fixstar()", "swe_fixstar2_ut()",
                                   "swe_fixstar_ut()"};
static const char *const STARS[2] = {"Aldebaran", "Polaris"};
static const int32 SFL[] = {0, SEFLG_TOPOCTR, SEFLG_EQUATORIAL, SEFLG_J2000 | SEFLG_NONUT,
                            SEFLG_SIDEREAL};
#define NSFL ((int) (sizeof SFL / sizeof SFL[0]))
static const struct edge { const char *sz; int dir; int32 eph; double jd; int inward; } EDGES[] = {
  {"de200.eph's end - 2^-10", 0, SEFLG_JPLEPH, 2513392.5 - 1.0 / 1024, -1},
  {"de200.eph's end", 0, SEFLG_JPLEPH, 2513392.5, -1},
  {"de200.eph's start + 2^-10", 0, SEFLG_JPLEPH, 2305424.5 + 1.0 / 1024, 1},
  {"de200.eph's start + 2^-20", 0, SEFLG_JPLEPH, 2305424.5 + 1.0 / 1048576, 1},
  {"sepl_18's start + 2^-10", 1, SEFLG_SWIEPH, 2378496.5 + 1.0 / 1024, 1},
  {"sepl_18's end - 7.5e-4", 1, SEFLG_SWIEPH, 2597651.3583984375, -1},
  {"semo_18's end - 4.2e-4", 2, SEFLG_SWIEPH, 2597656.45703125, -1},
  {"2000, de200.eph", 0, SEFLG_JPLEPH, 2451545.0, 1},
  {"2000, the files", 0, SEFLG_SWIEPH, 2451545.0, -1},
};
#define NEDGE ((int) (sizeof EDGES / sizeof EDGES[0]))

static const char *dirof(int d)
{
  return d == 1 ? DIR18 : d == 2 ? DIRMO : EPHE;
}

/* delta t 2^-11 day: a UT of 2^-11 day's steps is its TT exactly, so the
 * UT forms take the instants the TT forms do and their stencils are exact
 * (a UT rounded to the JD's ulp, 4.7e-10 day, is 1.6e-9 deg/day of noise
 * on Polaris's right ascension speed over 2^-8 day) */
#define DT (1.0 / 2048)
static void setup(const char *dir, int fb)
{
  swe_set_ephe_path(dir);
  swe_set_jpl_file("de200.eph");
  swe_set_ephe_fallback(fb);
  swe_set_topo(8.55, 47.37, 400);
  swe_set_delta_t_userdef(DT);
}

static void star_call(int kind, const char *name, double jd, int32 fl, struct ans *a)
{
  char star[64];
  strcpy(star, name);
  memset(a, 0, sizeof *a);
  switch (kind) {
    case STAR2: a->r = swe_fixstar2(star, jd, fl, a->x, a->serr); break;
    case STAR: a->r = swe_fixstar(star, jd, fl, a->x, a->serr); break;
    case STAR2_UT: a->r = swe_fixstar2_ut(star, jd, fl, a->x, a->serr); break;
    default: a->r = swe_fixstar_ut(star, jd, fl, a->x, a->serr); break;
  }
  a->ran = 1;
}

/* the instant a call of this kind takes: the edge's in TT, or the UT whose
 * TT it is */
static double call_jd(const struct edge *e, int kind)
{
  return (kind == STAR2 || kind == STAR) ? e->jd : e->jd - DT;
}

static void report(const char *part, const char *sz, const char *what, const struct ans *a)
{
  char s[AS_MAXCH], *p;
  nbad++;
  if (nbad > 40)
    return;
  memcpy(s, a->serr, sizeof s);
  s[sizeof s - 1] = '\0';
  for (p = s; *p; p++)
    if (*p == '\n')
      *p = '|';
  printf("    FAIL (%s) %s: %s; ret %d serr '%.80s'\n", part, sz, what, a->r, s);
}

/* the result of a forked process: its failures, rows and largest speed
 * error, and the last call's answer */
struct res { int bad, rows; double err; struct ans last; char first[200]; int ran; };

/* (a): the stars of one edge, one setting, one function, in one process */
static void stars_proc(const struct edge *e, int fb, int kind, struct res *out)
{
  int is, jf, k, c;
  const double S = 1.0 / 256;
  memset(out, 0, sizeof *out);
  setup(dirof(e->dir), fb);
  for (is = 0; is < 2; is++)
    for (jf = 0; jf < NSFL; jf++) {
      int32 fl = SFL[jf] | e->eph;
      double jd = call_jd(e, kind), y[5][2];
      struct ans s, z, p;
      const char *why = NULL;
      star_call(kind, STARS[is], jd, fl | SEFLG_SPEED, &s);
      star_call(kind, STARS[is], jd, fl, &z);
      out->rows++;
      if (z.r < 0 || (z.r & EPHMASK) != e->eph)
        why = "not answered from the ephemeris asked without SEFLG_SPEED";
      else if (s.r != (z.r | SEFLG_SPEED))
        why = "flag not the one without SEFLG_SPEED, with it";
      else if (memcmp(s.x, z.x, 3 * sizeof(double)) != 0)
        why = "place not the one without SEFLG_SPEED";
      else if (strcmp(s.serr, z.serr) != 0)
        why = "serr not the one without SEFLG_SPEED";
      for (k = 0; why == NULL && k < 5; k++) {
        if (k == 0)
          p = z;
        else
          star_call(kind, STARS[is], jd + e->inward * k * S, fl, &p);
        if (p.r != z.r)
          why = "a stencil instant answered otherwise";
        y[k][0] = p.x[0];
        y[k][1] = p.x[1];
      }
      for (c = 0; why == NULL && c < 2; c++) {
        double d = e->inward * (48 * swe_difdeg2n(y[1][c], y[0][c]) - 36 * swe_difdeg2n(y[2][c], y[0][c])
                                + 16 * swe_difdeg2n(y[3][c], y[0][c]) - 3 * swe_difdeg2n(y[4][c], y[0][c]))
                   / (12 * S);
        /* on the sky: the longitude's (right ascension's) by cos lat */
        double err = fabs(s.x[3 + c] - d) * (c ? 1 : cos(s.x[1] * M_PI / 180));
        if (err > out->err)
          out->err = err;
        if (!(err <= LIM))
          why = c ? "latitude speed off the places' rate" : "longitude speed off the places' rate";
      }
      if (why != NULL) {
        out->bad++;
        if (*out->first == '\0')
          snprintf(out->first, sizeof out->first, "%s %s fl %d: %s; ret %d/%d serr '%.60s'",
                   STARS[is], KSZ[kind], (int) fl, why, s.r, z.r, s.serr);
      }
      out->last = s;
    }
  out->ran = 1;
}

/* the last call of stars_proc(), alone */
static void star_fresh(const struct edge *e, int fb, int kind, struct res *out)
{
  memset(out, 0, sizeof *out);
  setup(dirof(e->dir), fb);
  star_call(kind, STARS[1], call_jd(e, kind), SFL[NSFL - 1] | e->eph | SEFLG_SPEED, &out->last);
  out->ran = 1;
}

/* (b) */
enum { CALC, ORBEL };
static const struct body { const char *sz; int kind, ipl; } BODIES[] = {
  {"Cupido", CALC, SE_CUPIDO},
  {"Isis", CALC, SE_ISIS},
  {"Poseidon", CALC, SE_POSEIDON},
  {"swe_calc_orbel() a = 40 AU", ORBEL, 0},
};
#define NBODY ((int) (sizeof BODIES / sizeof BODIES[0]))
static const int32 BFL[] = {0, SEFLG_SPEED, SEFLG_SPEED | SEFLG_TOPOCTR, SEFLG_SPEED | SEFLG_HELCTR,
                            SEFLG_SPEED | SEFLG_EQUATORIAL};
#define NBFL ((int) (sizeof BFL / sizeof BFL[0]))

static void body_call(const struct body *b, double jd, int32 fl, struct ans *a)
{
  static const double el[6] = {100, 40, 0.05, 30, 100, 2};
  memset(a, 0, sizeof *a);
  if (b->kind == CALC)
    a->r = swe_calc(jd, b->ipl, fl, a->x, a->serr);
  else
    a->r = swe_calc_orbel(jd, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, fl, a->x, a->serr);
  a->ran = 1;
}

struct bq { const struct body *b; double jd; int32 fl; const char *dir; int fb; };

/* (b)'s warm process: Cupido at 2378496.6, 2378497.5, 2378496.6 */
static void warm_proc(struct ans *out)
{
  static const struct body cup = {"Cupido", CALC, SE_CUPIDO};
  setup(DIR18, 1);
  body_call(&cup, 2378496.6, SEFLG_SWIEPH | SEFLG_SPEED, &out[0]);
  body_call(&cup, 2378497.5, SEFLG_SWIEPH | SEFLG_SPEED, &out[1]);
  body_call(&cup, 2378496.6, SEFLG_SWIEPH | SEFLG_SPEED, &out[2]);
}

/* (c) and (d): the edges, inward the side the places' rate is taken on;
 * mayfall: tjd's own light time leaves the span (the call without
 * SEFLG_SPEED falls back or is refused there, and the one with it must do
 * the same) */
struct sedge { const char *sz; int dir; int32 eph; double jd; int inward, mayfall; };
/* nine-point one-sided first derivative, k = 0 .. 8 steps */
static const double A9[9] = {-761.0 / 280, 8, -14, 56.0 / 3, -35.0 / 2, 56.0 / 5, -14.0 / 3, 8.0 / 7, -1.0 / 8};

/* (c) the true node and the osculating apogee by swe_calc() */
static const int NODB[2] = {SE_TRUE_NODE, SE_OSCU_APOG};
static const char *const NODSZ[2] = {"SE_TRUE_NODE", "SE_OSCU_APOG"};
static const int32 NFL[] = {0, SEFLG_EQUATORIAL, SEFLG_J2000 | SEFLG_NONUT, SEFLG_SIDEREAL, SEFLG_TRUEPOS};
#define NNFL ((int) (sizeof NFL / sizeof NFL[0]))
static const struct sedge CEDGES[] = {
  {"de200.eph's end - 2^-10", 0, SEFLG_JPLEPH, 2513392.5 - 1.0 / 1024, -1, 0},
  {"de200.eph's end", 0, SEFLG_JPLEPH, 2513392.5, -1, 0},
  {"de200.eph's end - 2^-5 - 2^-10", 0, SEFLG_JPLEPH, 2513392.5 - 1.0 / 32 - 1.0 / 1024, -1, 0},
  {"de200.eph's start + 2^-10", 0, SEFLG_JPLEPH, 2305424.5 + 1.0 / 1024, 1, 0},
  {"de200.eph's start + 2^-20", 0, SEFLG_JPLEPH, 2305424.5 + 1.0 / 1048576, 1, 1},
  {"semo_18's start + 7.1e-3", 1, SEFLG_SWIEPH, 2378487.5625, 1, 0},
  {"semo_18's end - 4.3e-3", 1, SEFLG_SWIEPH, 2597656.453125, -1, 0},
  {"2000, de200.eph", 0, SEFLG_JPLEPH, 2451545.0, 1, 0},
  {"2000, the files", 0, SEFLG_SWIEPH, 2451545.0, -1, 0},
};
#define NCEDGE ((int) (sizeof CEDGES / sizeof CEDGES[0]))
#define CH (1.0 / 16)		/* the places' step */
static const double CLIM[2] = {2e-8, 5e-7};	/* deg/day, the node's and the apogee's */
static double cerr[2];

static void node_call(int ib, double jd, int32 fl, struct ans *a)
{
  memset(a, 0, sizeof *a);
  a->r = swe_calc(jd, NODB[ib], fl, a->x, a->serr);
  a->ran = 1;
}

/* (c): the node and the apogee at one edge, one setting, in one process */
struct cres { int bad, rows; double err[2]; struct ans last; char first[200]; int ran; };
static void nodes_proc(const struct sedge *e, int fb, struct cres *out)
{
  int ib, jf, k, c;
  memset(out, 0, sizeof *out);
  setup(dirof(e->dir), fb);
  for (ib = 0; ib < 2; ib++)
    for (jf = 0; jf < NNFL; jf++) {
      int32 fl = NFL[jf] | e->eph;
      struct ans s, z, p;
      const char *why = NULL;
      double y[9][2];
      node_call(ib, e->jd, fl | SEFLG_SPEED, &s);
      node_call(ib, e->jd, fl, &z);
      out->rows++;
      if ((z.r < 0 || (z.r & EPHMASK) != e->eph) && !e->mayfall)
        why = "not answered from the ephemeris asked without SEFLG_SPEED";
      else if (s.r != (z.r < 0 ? z.r : (z.r | SEFLG_SPEED)))
        why = "flag not the one without SEFLG_SPEED, with it";
      else if (memcmp(s.x, z.x, 3 * sizeof(double)) != 0)
        why = "place not the one without SEFLG_SPEED";
      else if (strcmp(s.serr, z.serr) != 0)
        why = "serr not the one without SEFLG_SPEED";
      if (why == NULL && z.r >= 0 && (z.r & EPHMASK) == e->eph) {
        for (k = 0; why == NULL && k < 9; k++) {
          if (k == 0)
            p = z;
          else
            node_call(ib, e->jd + e->inward * k * CH, fl, &p);
          if (p.r != z.r)
            why = "a stencil instant answered otherwise";
          y[k][0] = p.x[0];
          y[k][1] = p.x[1];
        }
        for (c = 0; why == NULL && c < 2; c++) {
          double d = 0, err;
          for (k = 1; k < 9; k++)
            d += A9[k] * swe_difdeg2n(y[k][c], y[0][c]);
          d *= e->inward / CH;
          err = fabs(s.x[3 + c] - d) * (c ? 1 : cos(s.x[1] * M_PI / 180));
          if (err > out->err[ib])
            out->err[ib] = err;
          if (!(err <= CLIM[ib]))
            why = c ? "latitude speed off the places' rate" : "longitude speed off the places' rate";
        }
      }
      if (why != NULL) {
        out->bad++;
        if (*out->first == '\0')
          snprintf(out->first, sizeof out->first, "%s fl %d: %s; ret %d/%d serr '%.60s'",
                   NODSZ[ib], (int) fl, why, s.r, z.r, s.serr);
      }
      out->last = s;
    }
  out->ran = 1;
}

/* the last call of nodes_proc(), alone */
static void nodes_fresh(const struct sedge *e, int fb, struct cres *out)
{
  memset(out, 0, sizeof *out);
  setup(dirof(e->dir), fb);
  node_call(1, e->jd, NFL[NNFL - 1] | e->eph | SEFLG_SPEED, &out->last);
  out->ran = 1;
}

/* (d) swe_nod_aps(): the places' step and the limit; the Sun's points'
 * rate is taken under SEFLG_NOGDEFL, their perigee being behind the Sun's
 * disk at sepl_18's end, where the deflection's rate is not its places'
 * (2.9e-7 deg/day, with or without the files' edge) */
static const struct { const char *sz; int ipl; double h, lim; int32 xfl; } NPL[] = {
  {"Sun", SE_SUN, 1.0 / 32, 2e-8, SEFLG_NOGDEFL}, {"Moon", SE_MOON, 1.0 / 128, 3e-7, 0},
  {"Mars", SE_MARS, 1.0 / 32, 2e-8, 0}, {"Saturn", SE_SATURN, 1.0 / 32, 2e-8, 0},
  {"Pluto", SE_PLUTO, 1.0 / 32, 2e-8, 0},
};
#define NNPL ((int) (sizeof NPL / sizeof NPL[0]))
static const int32 NMETH[] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR,
                              SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
#define NNMETH ((int) (sizeof NMETH / sizeof NMETH[0]))
static const int32 NCTR[] = {0, SEFLG_TOPOCTR, SEFLG_HELCTR, SEFLG_BARYCTR};
#define NNCTR ((int) (sizeof NCTR / sizeof NCTR[0]))
static const struct sedge DEDGES[] = {
  {"de200.eph's start + 2^-20", 0, SEFLG_JPLEPH, 2305424.5 + 1.0 / 1048576, 1, 0},
  {"de200.eph's start + 2^-7", 0, SEFLG_JPLEPH, 2305424.5 + 1.0 / 128, 1, 0},
  {"de200.eph's start + 2^-3", 0, SEFLG_JPLEPH, 2305424.5 + 1.0 / 8, 1, 0},
  {"de200.eph's end - 2^-7", 0, SEFLG_JPLEPH, 2513392.5 - 1.0 / 128, -1, 0},
  {"de200.eph's end", 0, SEFLG_JPLEPH, 2513392.5, -1, 0},
  {"sepl_18's start + 2^-7", 1, SEFLG_SWIEPH, 2378496.5 + 1.0 / 128, 1, 0},
  {"sepl_18's start + 2^-3", 1, SEFLG_SWIEPH, 2378496.5 + 1.0 / 8, 1, 0},
  {"sepl_18's end - 7.6e-3", 1, SEFLG_SWIEPH, 2597651.3515625, -1, 0},
  {"semo_18's end - 4.3e-3", 2, SEFLG_SWIEPH, 2597656.453125, -1, 0},
  {"2000, de200.eph", 0, SEFLG_JPLEPH, 2451545.0, 1, 0},
  {"2000, the files", 0, SEFLG_SWIEPH, 2451545.0, -1, 0},
};
#define NDEDGE ((int) (sizeof DEDGES / sizeof DEDGES[0]))
static double derr;

struct nans { int32 r; double x[4][6]; char serr[AS_MAXCH]; int ran; };
static void nodaps_call(int ib, double jd, int32 fl, int32 m, struct nans *a)
{
  memset(a, 0, sizeof *a);
  a->r = swe_nod_aps(jd, NPL[ib].ipl, fl, m, a->x[0], a->x[1], a->x[2], a->x[3], a->serr);
  a->ran = 1;
}

/* (d): the points at one edge, one setting, in one process */
struct dres { int bad, rows; double err; struct nans last; char first[200]; int ran; };
static void nodaps_proc(const struct sedge *e, int fb, struct dres *out)
{
  int ib, im, ic, k, j, c;
  memset(out, 0, sizeof *out);
  setup(dirof(e->dir), fb);
  for (ib = 0; ib < NNPL; ib++)
    for (im = 0; im < NNMETH; im++)
      for (ic = 0; ic < NNCTR; ic++) {
        int32 fl = NCTR[ic] | e->eph, m = NMETH[im];
        /* the site turns: a shorter step */
        double h = (ic == 1 && NPL[ib].ipl != SE_MOON) ? NPL[ib].h / 2 : NPL[ib].h;
        struct nans s, z;
        static struct nans p[9];
        const char *why = NULL;
        nodaps_call(ib, e->jd, fl | SEFLG_SPEED, m, &s);
        nodaps_call(ib, e->jd, fl, m, &z);
        out->rows++;
        if (z.r < 0)
          why = "refused without SEFLG_SPEED";
        else if (s.r != z.r)
          why = "the return not the one without SEFLG_SPEED";
        else if (strcmp(s.serr, z.serr) != 0)
          why = "serr not the one without SEFLG_SPEED";
        for (j = 0; why == NULL && j < 4; j++)
          if (memcmp(s.x[j], z.x[j], 3 * sizeof(double)) != 0)
            why = "a place not the one without SEFLG_SPEED";
        if (why == NULL && NPL[ib].xfl != 0) {
          fl |= NPL[ib].xfl;
          nodaps_call(ib, e->jd, fl | SEFLG_SPEED, m, &s);
          nodaps_call(ib, e->jd, fl, m, &z);
          if (z.r < 0 || s.r != z.r || strcmp(s.serr, z.serr) != 0)
            why = "under SEFLG_NOGDEFL not answered as without SEFLG_SPEED";
        }
        for (k = 0; why == NULL && k < 9; k++) {
          if (k == 0)
            p[0] = z;
          else
            nodaps_call(ib, e->jd + e->inward * k * h, fl, m, &p[k]);
          if (p[k].r != z.r)
            why = "a stencil instant answered otherwise";
        }
        for (j = 0; why == NULL && j < 4; j++)
          for (c = 0; why == NULL && c < 2; c++) {
            double d = 0, err;
            if (z.x[j][2] == 0)	/* no point (the Earth's nodes) */
              continue;
            for (k = 1; k < 9; k++)
              d += A9[k] * swe_difdeg2n(p[k].x[j][c], p[0].x[j][c]);
            d *= e->inward / h;
            err = fabs(s.x[j][3 + c] - d) * (c ? 1 : cos(s.x[j][1] * M_PI / 180));
            if (err > out->err)
              out->err = err;
            if (!(err <= NPL[ib].lim))
              why = c ? "a latitude speed off the places' rate" : "a longitude speed off the places' rate";
          }
        if (why != NULL) {
          out->bad++;
          if (*out->first == '\0')
            snprintf(out->first, sizeof out->first, "%s method %d fl %d: %s; ret %d/%d serr '%.60s'",
                     NPL[ib].sz, (int) m, (int) fl, why, s.r, z.r, s.serr);
        }
        out->last = s;
      }
  out->ran = 1;
}

/* the last call of nodaps_proc(), alone */
static void nodaps_fresh(const struct sedge *e, int fb, struct dres *out)
{
  memset(out, 0, sizeof *out);
  setup(dirof(e->dir), fb);
  nodaps_call(NNPL - 1, e->jd, NCTR[NNCTR - 1] | e->eph | SEFLG_SPEED, NMETH[NNMETH - 1], &out->last);
  out->ran = 1;
}

/* run fn in a forked, pristine process; its result comes back by a pipe */
enum { P_STARS, P_FRESH, P_BODY, P_WARM, P_NODES, P_NODES_FRESH, P_NODAPS, P_NODAPS_FRESH };
struct job { int what; const struct edge *e; int fb, kind; struct bq q; const struct sedge *se; };
static int run(const struct job *j, void *out, size_t n)
{
  int fd[2];
  size_t got = 0;
  ssize_t r;
  pid_t pid;
  memset(out, 0, n);
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    void *buf = calloc(1, n);
    close(fd[0]);
    if (buf == NULL)
      _exit(3);
    switch (j->what) {
      case P_STARS: stars_proc(j->e, j->fb, j->kind, buf); break;
      case P_FRESH: star_fresh(j->e, j->fb, j->kind, buf); break;
      case P_BODY:
        setup(j->q.dir, j->q.fb);
        body_call(j->q.b, j->q.jd, j->q.fl, buf);
        break;
      case P_NODES: nodes_proc(j->se, j->fb, buf); break;
      case P_NODES_FRESH: nodes_fresh(j->se, j->fb, buf); break;
      case P_NODAPS: nodaps_proc(j->se, j->fb, buf); break;
      case P_NODAPS_FRESH: nodaps_fresh(j->se, j->fb, buf); break;
      default: warm_proc(buf); break;
    }
    _exit(write(fd[1], buf, n) == (ssize_t) n ? 0 : 3);
  }
  close(fd[1]);
  while (got < n && (r = read(fd[0], (char *) out + got, n - got)) > 0)
    got += (size_t) r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return got == n ? 0 : -1;
}

static void body_ask(const struct body *b, double jd, int32 fl, const char *dir, int fb, struct ans *a)
{
  struct job j;
  memset(&j, 0, sizeof j);
  j.what = P_BODY;
  j.q.b = b;
  j.q.jd = jd;
  j.q.fl = fl;
  j.q.dir = dir;
  j.q.fb = fb;
  if (run(&j, a, sizeof *a) != 0)
    a->ran = 0;
}

static int same(const struct ans *a, const struct ans *b)
{
  return a->ran && b->ran && a->r == b->r && memcmp(a->x, b->x, sizeof a->x) == 0;
}

static int strict_msg(const struct ans *a)
{
  return a->ran && a->r < 0 && strstr(a->serr, "ephemeris is not available for jd") != NULL
         && strstr(a->serr, "falling back to Moshier") != NULL;
}

static void check(int ok, const char *part, const char *sz, const char *what, const struct ans *a)
{
  nrow++;
  if (!ok)
    report(part, sz, what, a);
}

/* (b): one body, one flag set, one instant; answers 0: it falls back, 1: it answers */
static void body_rows(const struct body *b, double jd, int32 fl, const char *dir, int answers)
{
  char sz[200];
  struct ans f, s, m, full;
  int32 eph = strcmp(dir, DIR18) == 0 ? SEFLG_SWIEPH : SEFLG_JPLEPH;
  snprintf(sz, sizeof sz, "%s fl %d %s jd %.4f", b->sz, (int) fl,
           eph == SEFLG_JPLEPH ? "de200.eph" : "files of 1800-2400", jd);
  body_ask(b, jd, fl | eph, dir, 1, &f);
  body_ask(b, jd, fl | eph, dir, 0, &s);
  if (answers) {
    body_ask(b, jd, fl | eph, EPHE, 0, &full);
    check(same(&f, &full) && (f.r & EPHMASK) == eph, "b", sz,
          "fallback allowed: not the complete directory's answer", &f);
    check(same(&s, &full), "b", sz, "strict: not the complete directory's answer", &s);
  } else {
    body_ask(b, jd, fl | SEFLG_MOSEPH, dir, 0, &m);
    check(same(&f, &m) && strstr(f.serr, "Moshier") != NULL, "b", sz,
          "fallback allowed: not the Moshier request's answer, noted", &f);
    check(strict_msg(&s), "b", sz, "strict: not the downgrade's refusal", &s);
  }
}

/* which files a directory links */
static int build(char *dir, int moon_end)
{
  static const char *const f18[] = {"sepl_18.se1", "semo_18.se1", "seas_18.se1",
                                    "sefstars.txt", "seorbel.txt", "sepl_24.se1", "seas_24.se1"};
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  int k, n = moon_end ? 7 : 5;
  if (mkdtemp(dir) == NULL)
    return -1;
  for (k = 0; k < n; k++) {
    snprintf(src, sizeof src, "%s/%s", EPHE, f18[k]);
    snprintf(dst, sizeof dst, "%s/%s", dir, f18[k]);
    if (access(src, R_OK) != 0 || symlink(src, dst) != 0)
      return -1;
  }
  return 0;
}

static void unbuild(const char *dir)
{
  DIR *d = opendir(dir);
  struct dirent *e;
  char p[PATH_MAX + 300];
  if (d == NULL)
    return;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.') {
      snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
      unlink(p);
    }
  closedir(d);
  rmdir(dir);
}

int main(int argc, char **argv)
{
  int ie, fb, kind, ib, jf, k, nstar = 0, nbad_a, nrow_a, nbad_b, nrow_b, nbad_c, nrow_c;
  if (argc != 2 || realpath(argv[1], EPHE) == NULL) {
    fprintf(stderr, "usage: spanedge EPHEDIR\n");
    return 2;
  }
  if (build(DIR18, 0) != 0 || build(DIRMO, 1) != 0) {
    printf("G158 FAIL: cannot build the directories in /tmp\n");
    unbuild(DIR18);
    unbuild(DIRMO);
    return 1;
  }
  printf("G158: at a span's edge a star, the true node, the osculating apogee and\n"
         "      swe_nod_aps() with SEFLG_SPEED answer as without it, a fictitious body\n"
         "      or orbit falls back as swe_calc()'s planets\n");
  /* (a) */
  for (ie = 0; ie < NEDGE; ie++)
    for (fb = 0; fb < 2; fb++)
      for (kind = 0; kind < 4; kind++) {
        struct job j;
        struct res w, f;
        char sz[200];
        memset(&j, 0, sizeof j);
        j.e = &EDGES[ie];
        j.fb = fb;
        j.kind = kind;
        j.what = P_STARS;
        snprintf(sz, sizeof sz, "%s, %s, %s", EDGES[ie].sz, fb ? "fallback" : "strict", KSZ[kind]);
        if (run(&j, &w, sizeof w) != 0 || !w.ran) {
          nrow++;
          report("a", sz, "the process gave no answer", &w.last);
          continue;
        }
        nstar += w.rows;
        nrow += w.rows;
        if (w.err > maxerr)
          maxerr = w.err;
        if (w.bad) {
          nbad += w.bad - 1;
          report("a", sz, w.first, &w.last);
        }
        j.what = P_FRESH;
        nrow++;
        if (run(&j, &f, sizeof f) != 0 || !same(&w.last, &f.last)
            || strcmp(w.last.serr, f.last.serr) != 0)
          report("a", sz, "the last call not a pristine process's", &f.last);
      }
  nbad_a = nbad;
  nrow_a = nrow;
  /* (b) */
  for (ib = 0; ib < NBODY; ib++)
    for (jf = 0; jf < NBFL; jf++) {
      body_rows(&BODIES[ib], 2378496.6, BFL[jf], DIR18, 0);
      body_rows(&BODIES[ib], 2378496.625, BFL[jf], DIR18, 0);
      body_rows(&BODIES[ib], 2305424.625, BFL[jf], EPHE, 0);
      body_rows(&BODIES[ib], 2378497.5, BFL[jf], DIR18, 1);
    }
  for (ib = 0; ib < NBODY; ib++) {
    body_rows(&BODIES[ib], 2378496.6, SEFLG_TRUEPOS | SEFLG_SPEED, DIR18, 1);
    body_rows(&BODIES[ib], 2378496.6, SEFLG_TRUEPOS, DIR18, 1);
  }
  {
    struct job j;
    struct ans w[3], a;
    memset(&j, 0, sizeof j);
    j.what = P_WARM;
    if (run(&j, w, sizeof w) != 0)
      memset(w, 0, sizeof w);
    for (k = 0; k < 3; k++) {
      body_ask(&BODIES[0], k == 1 ? 2378497.5 : 2378496.6, SEFLG_SWIEPH | SEFLG_SPEED, DIR18, 1, &a);
      check(same(&w[k], &a) && strcmp(w[k].serr, a.serr) == 0, "b", "Cupido, one process",
            k == 1 ? "the call at 2378497.5 not a pristine process's"
                   : "the call at 2378496.6 not a pristine process's", &w[k]);
    }
  }
  nbad_b = nbad;
  nrow_b = nrow;
  /* (c) */
  for (ie = 0; ie < NCEDGE; ie++)
    for (fb = 0; fb < 2; fb++) {
      struct job j;
      struct cres w, f;
      char sz[200];
      memset(&j, 0, sizeof j);
      j.se = &CEDGES[ie];
      j.fb = fb;
      j.what = P_NODES;
      snprintf(sz, sizeof sz, "%s, %s", CEDGES[ie].sz, fb ? "fallback" : "strict");
      nrow++;
      if (run(&j, &w, sizeof w) != 0 || !w.ran) {
        report("c", sz, "the process gave no answer", &w.last);
        continue;
      }
      nrow += w.rows - 1;
      for (k = 0; k < 2; k++)
        if (w.err[k] > cerr[k])
          cerr[k] = w.err[k];
      if (w.bad) {
        nbad += w.bad - 1;
        report("c", sz, w.first, &w.last);
      }
      j.what = P_NODES_FRESH;
      nrow++;
      if (run(&j, &f, sizeof f) != 0 || !same(&w.last, &f.last)
          || strcmp(w.last.serr, f.last.serr) != 0)
        report("c", sz, "the last call not a pristine process's", &f.last);
    }
  nbad_c = nbad;
  nrow_c = nrow;
  /* (d) */
  for (ie = 0; ie < NDEDGE; ie++)
    for (fb = 0; fb < 2; fb++) {
      struct job j;
      static struct dres w, f;
      struct ans a;
      char sz[200];
      memset(&j, 0, sizeof j);
      j.se = &DEDGES[ie];
      j.fb = fb;
      j.what = P_NODAPS;
      snprintf(sz, sizeof sz, "%s, %s", DEDGES[ie].sz, fb ? "fallback" : "strict");
      memset(&a, 0, sizeof a);
      nrow++;
      if (run(&j, &w, sizeof w) != 0 || !w.ran) {
        report("d", sz, "the process gave no answer", &a);
        continue;
      }
      nrow += w.rows - 1;
      if (w.err > derr)
        derr = w.err;
      a.r = w.last.r;
      memcpy(a.serr, w.last.serr, sizeof a.serr);
      if (w.bad) {
        nbad += w.bad - 1;
        report("d", sz, w.first, &a);
      }
      j.what = P_NODAPS_FRESH;
      nrow++;
      if (run(&j, &f, sizeof f) != 0 || !f.ran || w.last.r != f.last.r
          || memcmp(w.last.x, f.last.x, sizeof w.last.x) != 0 || strcmp(w.last.serr, f.last.serr) != 0)
        report("d", sz, "the last call not a pristine process's", &a);
    }
  unbuild(DIR18);
  unbuild(DIRMO);
  printf("  rows %d, failed %d: (a) %d (%d star calls), failed %d, the stars' speeds within %.2e deg/day\n"
         "  of their places' rate; (b) %d, failed %d; (c) %d, failed %d, the node's and the apogee's\n"
         "  speeds within %.2e and %.2e of their places' rate; (d) %d, failed %d, swe_nod_aps()'s\n"
         "  within %.2e\n",
         nrow, nbad, nrow_a, nstar, nbad_a, maxerr, nrow_b - nrow_a, nbad_b - nbad_a,
         nrow_c - nrow_b, nbad_c - nbad_b, cerr[0], cerr[1], nrow - nrow_c, nbad - nbad_c, derr);
  printf(nbad ? "G158 FAIL: %d\n" : "G158 PASS\n", nbad);
  return nbad ? 1 : 0;
}
