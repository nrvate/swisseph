/* segspeed.c -- G176: across a boundary of the Swiss files' segments, and
 * across a join for SEFLG_SPEED3, every speed is the rate of the places on
 * the instant's own side; SEFLG_SPEED3 divides by its instants' spacing.
 *
 * The defect (entries 276 and 266). A Swiss file holds each body as
 * Chebyshev segments (Moon 27.55 days, EMB and Sun 365.26, Mercury 87.97,
 * Venus 224.70, Mars 687.00, Jupiter to Pluto 4000, asteroids 1000 or 500)
 * whose ends do not meet: each boundary is a step of the model, as a join
 * of two files is (entry 250). The stencils bug 250 kept on the instant's
 * side of a join crossed a segment boundary freely, and their speeds took
 * the step: the true node 8.1e-4 deg/day and the osculating apogee 9.2e-3
 * at the Moon's boundary JD 2451562.2258 (J2000 + 17.23 days), swe_nod_aps()
 * Mars's heliocentric osculating node 1.2e-4 and perihelion 3.1e-3 at
 * sepl_12's last EMB boundary, the Sun's mean perigee 7.6e-7, swe_calc()
 * within the light time after an Earth step 6.0e-7 (the Moon's aberration
 * share 2.1e-4), Aldebaran 8.6e-7, swe_calc_pctr() Moon from Mars 2.7e-6.
 * SEFLG_SPEED3 differenced its three reads across joins and boundaries
 * alike, up to 0.25 deg/day (the Moon at its own boundary), and divided by
 * 2 dt though tjd -+ dt round: 1.7e-6 of the speed off everywhere (the Moon
 * 1.7e-5 deg/day at J2000, Mars 8.6e-7). Boundaries are not 600 years
 * apart as joins are: the Moon's first in semo_114 is 0.088 day after the
 * join of 11400, and the stencils that keep to one side must fit between.
 *
 * The check. Every boundary of the EMB's and the Moon's segments in the
 * middle of each file from -12300 to 16500 (50 and 51, with sepl_12's last,
 * J2000 + 17.23 and semom36's first), and Mars's, Jupiter's, Saturn's,
 * Ceres's and Chiron's in some files, from the files' own data; the 50
 * joins. At each, on both sides, the speeds against the five-point
 * one-sided differences of the places on that side, binary-fraction
 * instants, power-of-two steps: (a) the true node and the osculating
 * apogee, (b) swe_nod_aps()'s osculating node and perihelion, (c) its mean
 * points (the aberration rate), (d) swe_calc() in the light time after an
 * Earth step, (e) the Moon's aberration share on its line across the step,
 * (f) stars, (g) swe_calc_pctr(), (h) SEFLG_SPEED3 with its reads at the
 * instants and at the instants less the light time astride the step, and
 * (i) SEFLG_SPEED3 at 64 dates against the central difference; and
 * between that join and that boundary, where the stencils cross one or the
 * other (the true node, apogee and the Moon's osculating points at J +
 * 61/1024, the Sun's and the Earth's perigee at J + 5/128). Every answer
 * also from a fresh context and its place against the call without a
 * speed flag, bit for bit. 7938 rows, about 5 s.
 *
 * Measured, as fixed: (a) 4.3e-7 deg/day (the apogee), (b) 1.4e-7 (the
 * Moon; the planets 4.2e-11), (c) 1.1e-9, (d) 4.2e-9, (e) 1.8e-8 (at EMB
 * steps: the Moon's place itself steps there, noticed in entry 276),
 * (f) 6.7e-11, (g) 2.2e-9, (h) 1.9e-6 (the apogee; the Moon 1.6e-6, the
 * planets 1.9e-8), (i) 2.5e-7 (the Moon; the planets 3.6e-10); distance
 * rates 2.7e-12 AU/day. Off the steps (SEGSPEED_OFF=0.5 or -3) it passes.
 *
 * The upstream build fails 7918 of the 7938 rows; ts.20 before the fix
 * 7109: (a) 818 to 3.9e-2 deg/day, (b) 2220 to 8.0e-2, (c) 300 to 7.6e-7,
 * (d) 318 to 6.0e-7, (e) 588 to 2.1e-4, (f) 300 to 8.6e-7, (g) 105 to
 * 2.7e-6, (h) 2076 to 0.25, (i) 384 to 3.2e-5 (between the join and the
 * boundary the true node 1.1e-5, the apogee and the Moon's perigee 6.8e-4).
 * A boundary skipped as a join is, without the halving of the step there
 * (osc_span_segs(), swe_nod_aps()'s nodaps_again), refuses the Sun's
 * perigee at J + 5/128: "the osculating speed's epochs leave the
 * ephemeris's span both sides" (G166 sees it too).
 *
 * SEGSPEED_V=1 prints the failing rows, 2 every row; SEGSPEED_OFF=d moves
 * every step by d days (a control: away from the steps nothing changes).
 *
 * Run as
 *   segspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"

#define FLX (SEFLG_XYZ | SEFLG_EQUATORIAL)
#define F20 1048576.0
#define NPART 10

static const char *EPHE;
static swe_ctx *ctx;
static int nrow, nbad;
static double worst[NPART], worstr, worstbt[NPART][9];
static int cur_bt = 8;
static const char *const PART[NPART] = {"(a) true node, apogee", "(b) nod_aps osculating",
                                        "(c) nod_aps mean", "(d) swe_calc in the light time",
                                        "(e) the Moon's aberration share", "(f) stars",
                                        "(g) swe_calc_pctr", "(h) SPEED3 at a step",
                                        "(i) SPEED3's spacing", "bit for bit"};

/* one call: what it is and its answer; spd the speed flag (0: SEFLG_SPEED) */
enum { K_CALC, K_NOD, K_STAR, K_PCTR };
struct call { int kind, ipl, ictr, pt; int32 fl, meth; const char *star; int32 spd; };
struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; };

static swe_ctx *fresh(void)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, EPHE);
  swe_set_topo_r(c, 8.55, 47.37, 400);
  return c;
}

static void run(swe_ctx *c, const struct call *k, double jd, int32 fl, struct ans *a)
{
  double xx[4][6];
  char st[64];
  memset(a, 0, sizeof *a);
  switch (k->kind) {
    case K_CALC:
      a->r = swe_calc_r(c, jd, k->ipl, fl, a->x, a->serr);
      break;
    case K_NOD:
      a->r = swe_nod_aps_r(c, jd, k->ipl, fl, k->meth, xx[0], xx[1], xx[2], xx[3], a->serr);
      memcpy(a->x, xx[k->pt], sizeof a->x);
      break;
    case K_STAR:
      strcpy(st, k->star);
      a->r = swe_fixstar2_r(c, st, jd, fl, a->x, a->serr);
      break;
    default:
      a->r = swe_calc_pctr_r(c, jd, k->ipl, k->ictr, fl, a->x, a->serr);
      break;
  }
}

static void fail(int part, const char *what, const struct call *k, double tau, double jd, double v)
{
  nbad++;
  if (nbad <= 30)
    printf("    FAIL %s: %s, body %d (pt %d, ctr %d%s%s), flags %#x, JD %.10f (tau %+.3g): %.3e\n",
           PART[part], what, k->ipl, k->pt, k->ictr, k->star ? ", " : "", k->star ? k->star : "",
           (unsigned) (k->fl | k->spd), jd, jd - tau, v);
}

/* the speed call at jd against the five-point difference of the places,
 * one-sided on side dir (0: central), step s, and its fresh and unflagged
 * twins; lim on the sky, deg/day, plus rel times the speed on the sky, and
 * radial, AU/day (rlim 0: not checked) */
static void check_rate(int part, const struct call *k, double tau, double jd, int dir, double s,
                       double lim, double rel, double rlim)
{
  int32 fl = k->fl | FLX, spd = k->spd ? k->spd : SEFLG_SPEED;
  struct ans a, b, z, p[5];
  double v[3], d[3], r = 0, dr = 0, dd = 0, ang, sky = 0, vr = 0;
  int i, j, bad0 = nbad;
  nrow++;
  run(ctx, k, jd, fl | spd, &a);
  if (a.r < 0) {
    fail(part, a.serr, k, tau, jd, 0);
    return;
  }
  /* bit for bit: a fresh context, and the place without the speed flag */
  {
    swe_ctx *c = fresh();
    run(c, k, jd, fl | spd, &b);
    swe_ctx_free(c);
  }
  run(ctx, k, jd, fl, &z);
  if (b.r != a.r || memcmp(b.x, a.x, sizeof a.x) != 0)
    fail(NPART - 1, "the speed call in a fresh context differs", k, tau, jd, fabs(b.x[3] - a.x[3]));
  if (memcmp(z.x, a.x, 3 * sizeof(double)) != 0)
    fail(NPART - 1, "the place differs from the call's without a speed flag", k, tau, jd,
         fabs(z.x[0] - a.x[0]));
  if (dir != 0) {
    for (j = 0; j < 5; j++)
      run(ctx, k, jd + dir * j * s, fl, &p[j]);
    for (i = 0; i < 3; i++)
      v[i] = (-25 * p[0].x[i] + 48 * p[1].x[i] - 36 * p[2].x[i] + 16 * p[3].x[i] - 3 * p[4].x[i])
             / (12 * s * dir);
  } else {
    for (j = 0; j < 5; j++)
      run(ctx, k, jd + (j - 2) * s, fl, &p[j]);
    for (i = 0; i < 3; i++)
      v[i] = (p[0].x[i] - 8 * p[1].x[i] + 8 * p[3].x[i] - p[4].x[i]) / (12 * s);
  }
  for (i = 0; i < 3; i++) {
    d[i] = a.x[i + 3] - v[i];
    r += a.x[i] * a.x[i];
  }
  r = sqrt(r);
  for (i = 0; i < 3; i++) {
    dr += d[i] * a.x[i] / r;
    vr += v[i] * a.x[i] / r;
  }
  for (i = 0; i < 3; i++) {
    double q = d[i] - dr * a.x[i] / r, w = v[i] - vr * a.x[i] / r;
    dd += q * q;
    sky += w * w;
  }
  ang = sqrt(dd) / r * 180 / M_PI;
  sky = sqrt(sky) / r * 180 / M_PI;
  lim += rel * sky;
  if (ang > worst[part])
    worst[part] = ang;
  if (ang > worstbt[part][cur_bt])
    worstbt[part][cur_bt] = ang;
  if (getenv("SEGSPEED_V") != NULL && (ang > lim || atoi(getenv("SEGSPEED_V")) > 1))
    printf("      row part %d bt %d body %d pt %d fl %#x jd %.8f (tau %+.3g) %.3e\n", part, cur_bt,
           k->ipl, k->pt, (unsigned) (k->fl | k->spd), jd, jd - tau, ang);
  if (rlim > 0 && fabs(dr) > worstr)
    worstr = fabs(dr);
  if (!(ang <= lim))
    fail(part, "speed off its places' rate, deg/day", k, tau, jd, ang);
  else if (rlim > 0 && !(fabs(dr) <= rlim))
    fail(part, "distance rate off its places', AU/day", k, tau, jd, fabs(dr));
  if (nbad > bad0)
    nbad = bad0 + 1;	/* a row fails once */
}

/* (e): the aberration's share of the Moon's speed */
static void moon_share(swe_ctx *c, double jd, int32 fl, double *sh)
{
  struct call k = {K_CALC, SE_MOON, 0, 0, 0, 0, NULL, 0};
  struct ans a, b;
  run(c, &k, jd, fl | SEFLG_SPEED, &a);
  run(c, &k, jd, fl | SEFLG_SPEED | SEFLG_NOABERR, &b);
  sh[0] = (a.x[3] - b.x[3]) * cos(a.x[1] * M_PI / 180);
  sh[1] = a.x[4] - b.x[4];
}

/* binary-fraction instants: the first at or after t, the last at or before */
static double up(double t) { return ceil(t * F20) / F20; }
static double dn(double t) { return floor(t * F20) / F20; }

/* the k-th boundary (k >= 1) of body b's segments after the start of the
 * segment that holds D, in the file that answers D (read with body ipl),
 * from the file's data as the library reckons it; 0 if no file answers or
 * the boundary is within 2 days of the file's nominal ends */
static double boundary(int b, int ipl, double D, int k, int near_end_ok)
{
  swe_ctx *c = fresh();
  double x[6], tau = 0;
  char serr[AS_MAXCH], s0[AS_MAXCH], s1[AS_MAXCH], s2[AS_MAXCH];
  int32 r = swe_calc_r(c, D, ipl, SEFLG_SWIEPH, x, serr);
  if (r >= 0 && (r & SEFLG_SWIEPH)) {
    const struct plan_data *p = &c->pldat[b];
    if (p->dseg > 0 && D >= p->tfstart && D < p->tfend)
      tau = p->tfstart + ((int32) ((D - p->tfstart) / p->dseg) + k) * p->dseg;
  }
  swe_ctx_free(c);
  if (tau != 0 && !near_end_ok) {
    swi_gen_filename(D, SEI_SUNBARY, s0);
    swi_gen_filename(tau - 2, SEI_SUNBARY, s1);
    swi_gen_filename(tau + 2, SEI_SUNBARY, s2);
    if (strcmp(s0, s1) != 0 || strcmp(s0, s2) != 0)
      tau = 0;
  }
  return tau;
}

/* the light time to body ipl at t, geocentric (fl 0) or heliocentric */
static double light_time(int ipl, double t, int32 fl)
{
  double x[6];
  char serr[AS_MAXCH];
  if (swe_calc_r(ctx, t, ipl, fl | SEFLG_SPEED, x, serr) < 0)
    return 0;
  return x[2] * AUNIT / CLIGHT / 86400.0;
}

enum { B_JOIN, B_EMB, B_MOON, B_MARS, B_JUP, B_SAT, B_CERES, B_CHIRON, NBT };
static const char *const BNAME[NBT] = {"joins", "EMB", "Moon", "Mars", "Jupiter", "Saturn",
                                       "Ceres", "Chiron"};

static void part_begin(int part, int *nb, int *nr) { nb[part] -= nbad; nr[part] -= nrow; }
static void part_end(int part, int *nb, int *nr) { nb[part] += nbad; nr[part] += nrow; }

/* the rows at one step tau of type bt */
static void rows(int bt, double tau, int *nb, int *nr)
{
  static const double HP7 = 1.0 / 128, HP5 = 1.0 / 32, HP6 = 1.0 / 64, HP8 = 1.0 / 256;
  int i, j, part;
  double ta = up(tau);	/* the first instant answered past the step */
  /* (a) the true node and the osculating apogee across a Moon step */
  if (bt == B_MOON) {
    part = 0;
    part_begin(part, nb, nr);
    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++) {
        struct call k = {K_CALC, i ? SE_OSCU_APOG : SE_TRUE_NODE, 0, 0, j ? SEFLG_TRUEPOS : 0, 0,
                         NULL, 0};
        double lim = i ? 1e-6 : 5e-8;
        check_rate(part, &k, tau, dn(tau - HP7), -1, HP6, lim, 0, 1e-11);
        check_rate(part, &k, tau, ta, j ? 1 : -1, HP6, lim, 0, 1e-11);
        check_rate(part, &k, tau, ta + 1 / F20, j ? 1 : -1, HP6, lim, 0, 1e-11);
        check_rate(part, &k, tau, up(tau + HP7), 1, HP6, lim, 0, 1e-11);
      }
    part_end(part, nb, nr);
  }
  /* (b) swe_nod_aps()'s osculating points */
  if (bt != B_JOIN && bt != B_JUP) {
    static const struct call kemb[] = {
      {K_NOD, SE_MARS, 0, 0, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_MARS, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_MARS, 0, 0, SEFLG_NOGDEFL, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_MARS, 0, 2, SEFLG_NOGDEFL, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_SUN, 0, 2, SEFLG_NOGDEFL, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_EARTH, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_CERES, 0, 0, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_CERES, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
    }, kmoon[] = {
      {K_NOD, SE_MOON, 0, 0, 0, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_MOON, 0, 2, 0, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_EARTH, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
    }, kmars[] = {
      {K_NOD, SE_MARS, 0, 0, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_MARS, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_MARS, 0, 0, SEFLG_NOGDEFL, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_MARS, 0, 2, SEFLG_NOGDEFL, SE_NODBIT_OSCU, NULL, 0},
    }, ksat[] = {
      {K_NOD, SE_SATURN, 0, 0, SEFLG_BARYCTR, SE_NODBIT_OSCU_BAR, NULL, 0},
      {K_NOD, SE_SATURN, 0, 2, SEFLG_BARYCTR, SE_NODBIT_OSCU_BAR, NULL, 0},
    }, kcer[] = {
      {K_NOD, SE_CERES, 0, 0, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_CERES, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
    }, kchi[] = {
      {K_NOD, SE_CHIRON, 0, 0, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_CHIRON, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
    };
    const struct call *kb;
    int n;
    switch (bt) {
      case B_EMB: kb = kemb; n = (int) (sizeof kemb / sizeof kemb[0]); break;
      case B_MOON: kb = kmoon; n = (int) (sizeof kmoon / sizeof kmoon[0]); break;
      case B_MARS: kb = kmars; n = (int) (sizeof kmars / sizeof kmars[0]); break;
      case B_SAT: kb = ksat; n = (int) (sizeof ksat / sizeof ksat[0]); break;
      case B_CERES: kb = kcer; n = (int) (sizeof kcer / sizeof kcer[0]); break;
      default: kb = kchi; n = (int) (sizeof kchi / sizeof kchi[0]); break;
    }
    part = 1;
    part_begin(part, nb, nr);
    for (i = 0; i < n; i++) {
      double s = kb[i].ipl == SE_MOON ? HP7 : HP5, lim = kb[i].ipl == SE_MOON ? 2e-7 : 2e-9;
      check_rate(part, &kb[i], tau, dn(tau - HP7), -1, s, lim, 0, 1e-11);
      check_rate(part, &kb[i], tau, ta, 1, s, lim, 0, 1e-11);
      check_rate(part, &kb[i], tau, up(tau + HP7), 1, s, lim, 0, 1e-11);
      check_rate(part, &kb[i], tau, up(tau + HP5 + HP7), 1, s, lim, 0, 1e-11);
    }
    part_end(part, nb, nr);
  }
  if (bt == B_EMB) {
    /* (c) swe_nod_aps() mean, its aberration rate's observer read across */
    static const struct call kc[] = {
      {K_NOD, SE_SUN, 0, 2, SEFLG_NOGDEFL, SE_NODBIT_MEAN, NULL, 0},
      {K_NOD, SE_MARS, 0, 0, SEFLG_NOGDEFL, SE_NODBIT_MEAN, NULL, 0},
      {K_NOD, SE_MARS, 0, 0, SEFLG_NOGDEFL | SEFLG_TOPOCTR, SE_NODBIT_MEAN, NULL, 0},
    };
    /* (d) swe_calc() within the light time after the step */
    static const struct { struct call k; double d, s; } kd[] = {
      {{K_CALC, SE_SUN, 0, 0, 0, 0, NULL, 0}, 1.0 / 1024, 1.0 / 2048},
      {{K_CALC, SE_MERCURY, 0, 0, 0, 0, NULL, 0}, 1.0 / 1024, 1.0 / 2048},
      {{K_CALC, SE_MARS, 0, 0, 0, 0, NULL, 0}, 1.0 / 1024, 1.0 / 4096},
      {{K_CALC, SE_MARS, 0, 0, SEFLG_TOPOCTR, 0, NULL, 0}, 1.0 / 1024, 1.0 / 4096},
      {{K_CALC, SE_JUPITER, 0, 0, 0, 0, NULL, 0}, 1.0 / 256, 1.0 / 1024},
      {{K_CALC, SE_CERES, 0, 0, 0, 0, NULL, 0}, 1.0 / 256, 1.0 / 1024},
      {{K_CALC, SE_CUPIDO, 0, 0, 0, 0, NULL, 0}, 1.0 / 16, 1.0 / 64},
    };
    part = 2;
    part_begin(part, nb, nr);
    for (i = 0; i < (int) (sizeof kc / sizeof kc[0]); i++) {
      double lim = kc[i].fl & SEFLG_TOPOCTR ? 3e-9 : 1e-9;
      check_rate(part, &kc[i], tau, up(tau + 1.0 / 1024), 1, HP8, lim, 0, 0);
      check_rate(part, &kc[i], tau, up(tau + 1.0 / 16384), 1, HP8, lim, 0, 0);
    }
    part_end(part, nb, nr);
    part = 3;
    part_begin(part, nb, nr);
    for (i = 0; i < (int) (sizeof kd / sizeof kd[0]); i++)
      check_rate(part, &kd[i].k, tau, up(tau + kd[i].d), 1, kd[i].s, 6e-9, 0, 0);
    part_end(part, nb, nr);
    /* (f) stars */
    part = 5;
    part_begin(part, nb, nr);
    for (i = 0; i < 2; i++) {
      struct call k = {K_STAR, 0, 0, 0, 0, 0, i ? "Polaris" : "Aldebaran", 0};
      check_rate(part, &k, tau, dn(tau - 1.0 / 1024), -1, HP8, 1e-9, 0, 0);
      check_rate(part, &k, tau, ta, 1, HP8, 1e-9, 0, 0);
      check_rate(part, &k, tau, up(tau + 1.0 / 1024), 1, HP8, 1e-9, 0, 0);
    }
    part_end(part, nb, nr);
  }
  /* (e) the Moon, whose light time no difference resolves */
  if (bt == B_EMB || bt == B_MOON) {
    part = 4;
    part_begin(part, nb, nr);
    for (j = 0; j < 2; j++) {
      int32 fl = j ? SEFLG_TOPOCTR : 0;
      double t0 = dn(tau - 1.0 / 131072), t1 = up(tau + 1.0 / 32768), a[2], b[2], c[2];
      double tin[3] = {ta, ta + 1 / F20, up(tau + 1.0 / 131072)};
      struct call k = {K_CALC, SE_MOON, 0, 0, fl, 0, NULL, 0};
      moon_share(ctx, t0, fl, a);
      moon_share(ctx, t1, fl, b);
      for (i = 0; i < 3; i++) {
        double f = (tin[i] - t0) / (t1 - t0), e = 0;
        int m;
        nrow++;
        moon_share(ctx, tin[i], fl, c);
        for (m = 0; m < 2; m++)
          if (fabs(c[m] - (a[m] + f * (b[m] - a[m]))) > e)
            e = fabs(c[m] - (a[m] + f * (b[m] - a[m])));
        if (e > worst[part])
          worst[part] = e;
        if (e > worstbt[part][cur_bt])
          worstbt[part][cur_bt] = e;
        if (getenv("SEGSPEED_V") != NULL && e > 5e-9)
          printf("      share bt %d fl %#x tau %.8f t %+.3g a %.6e %.6e b %.6e %.6e c %.6e %.6e e %.3e\n", cur_bt,
                 (unsigned) fl, tau, tin[i] - tau, a[0], a[1], b[0], b[1], c[0], c[1], e);
        if (!(e <= (bt == B_EMB ? 3e-8 : 5e-9)))
          fail(part, "the aberration's share of the speed off its line, deg/day", &k, tau, tin[i], e);
      }
    }
    part_end(part, nb, nr);
  }
  /* (g) swe_calc_pctr(), the centre's acceleration */
  if (bt == B_JUP || bt == B_MARS || bt == B_MOON) {
    struct call k = {K_PCTR, bt == B_JUP ? SE_MARS : bt == B_MARS ? SE_MOON : SE_MARS,
                     bt == B_JUP ? SE_JUPITER : bt == B_MARS ? SE_MARS : SE_MOON, 0, 0, 0, NULL, 0};
    part = 6;
    part_begin(part, nb, nr);
    check_rate(part, &k, tau, dn(tau - 1.0 / 2048), -1, 1.0 / 4096, 6e-9, 0, 0);
    check_rate(part, &k, tau, up(tau + 1.0 / 2048), 1, 1.0 / 4096, 6e-9, 0, 0);
    part_end(part, nb, nr);
  }
  /* (h) SEFLG_SPEED3, its reads at tjd -+ dt straddling the step: the
   * Earth's at the instants (nominal), the body's own at the instants less
   * its light time (retarded). The Moon's apparent place itself straddles
   * an Earth step for its light time after it (the Earth read at teval and
   * at t, whose difference takes the step), which no speed resolves: not
   * asked there (noticed, entry 276). */
  {
    static const struct { int ipl; int32 fl; int bt, ret, sides; double h; } kh[] = {
      {SE_SUN, 0, B_JOIN, 0, 3, 1.0 / 32768}, {SE_SUN, 0, B_JOIN, 1, 3, 1.0 / 32768},
      {SE_SUN, 0, B_EMB, 0, 3, 1.0 / 32768}, {SE_SUN, 0, B_EMB, 1, 3, 1.0 / 32768},
      {SE_MARS, 0, B_JOIN, 0, 3, 1.0 / 32768}, {SE_MARS, 0, B_JOIN, 1, 3, 1.0 / 32768},
      {SE_MARS, 0, B_EMB, 0, 3, 1.0 / 32768}, {SE_MARS, 0, B_MARS, 1, 3, 1.0 / 32768},
      {SE_MARS, SEFLG_HELCTR, B_JOIN, 0, 3, 1.0 / 32768},
      {SE_MARS, SEFLG_HELCTR, B_JOIN, 1, 3, 1.0 / 32768},
      {SE_MARS, SEFLG_HELCTR, B_EMB, 0, 3, 1.0 / 32768},
      {SE_MARS, SEFLG_HELCTR, B_MARS, 1, 3, 1.0 / 32768},
      {SE_MERCURY, 0, B_EMB, 1, 3, 1.0 / 32768},
      {SE_JUPITER, 0, B_JUP, 1, 3, 1.0 / 32768},
      {SE_MOON, 0, B_JOIN, 0, 3, 1.0 / 32768}, {SE_MOON, 0, B_JOIN, 1, 2, 1.0 / 65536},
      {SE_MOON, 0, B_EMB, 0, 3, 1.0 / 32768}, {SE_MOON, 0, B_MOON, 1, 3, 1.0 / 65536},
      {SE_MOON, SEFLG_TRUEPOS, B_MOON, 0, 3, 1.0 / 65536},
      {SE_TRUE_NODE, 0, B_JOIN, 1, 3, 1.0 / 2048}, {SE_TRUE_NODE, 0, B_MOON, 1, 3, 1.0 / 2048},
      {SE_TRUE_NODE, SEFLG_TRUEPOS, B_MOON, 0, 3, 1.0 / 2048},
      {SE_OSCU_APOG, 0, B_MOON, 1, 3, 1.0 / 2048},
    };
    part = 7;
    part_begin(part, nb, nr);
    for (i = 0; i < (int) (sizeof kh / sizeof kh[0]); i++) {
      struct call k = {K_CALC, kh[i].ipl, 0, 0, kh[i].fl, 0, NULL, SEFLG_SPEED3};
      int ipl = kh[i].ipl, moon = ipl == SE_MOON || ipl == SE_TRUE_NODE || ipl == SE_OSCU_APOG;
      double s = ipl == SE_TRUE_NODE || ipl == SE_OSCU_APOG ? 1.0 / 512 : ipl == SE_MOON ? 1.0 / 8192 : 1.0 / 4096;
      /* Mercury's one-sided SEFLG_SPEED3 is noisy to 1.1e-8 deg/day near
       * JD 4241320 (its own difference over 1e-4 day, no step) */
      double lim = ipl == SE_OSCU_APOG ? 1e-5 : ipl == SE_TRUE_NODE ? 1e-6 : ipl == SE_MOON ? 2e-6
                   : ipl == SE_MERCURY ? 4e-8 : 2e-8;
      double lt = 0;
      if (kh[i].bt != bt)
        continue;
      if (kh[i].ret)
        lt = light_time(moon ? SE_MOON : ipl, tau, kh[i].fl & SEFLG_HELCTR);
      if (kh[i].sides & 1)
        check_rate(part, &k, tau, dn(tau + lt - kh[i].h), -1, s, lim, 1e-9, 0);
      if (kh[i].sides & 2)
        check_rate(part, &k, tau, up(tau + lt + kh[i].h), 1, s, lim, 1e-9, 0);
    }
    part_end(part, nb, nr);
  }
}

int main(int argc, char **argv)
{
  double tau[NBT][64];
  int nt[NBT], ib, it, y, nb[NPART], nr[NPART], part, i;
  if (argc != 2) {
    fprintf(stderr, "usage: segspeed EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  ctx = fresh();
  for (ib = 0; ib < NBT; ib++)
    nt[ib] = 0;
  /* the joins, each a change of file name */
  for (y = -12600; y <= 16800; y += 600) {
    double J = swe_julday(y, 1, 1, 0, y >= 1600 ? SE_GREG_CAL : SE_JUL_CAL);
    char s0[AS_MAXCH], s1[AS_MAXCH];
    swi_gen_filename(nextafter(J, -HUGE_VAL), SEI_SUNBARY, s0);
    swi_gen_filename(J, SEI_SUNBARY, s1);
    if (strcmp(s0, s1) != 0)
      tau[B_JOIN][nt[B_JOIN]++] = J;
  }
  /* a boundary in the middle of each file: the EMB's and the Moon's in
   * each, Mars's in every other, Jupiter's, Saturn's and Ceres's in some,
   * Chiron's in its files; and the three the entry names */
  for (y = -12300; y <= 16500; y += 600) {
    double D = swe_julday(y, 1, 1, 0, y >= 1600 ? SE_GREG_CAL : SE_JUL_CAL), t;
    int k = (y + 12300) / 600;
    if ((t = boundary(SEI_EMB, SE_MARS, D, 1, 0)) != 0)
      tau[B_EMB][nt[B_EMB]++] = t;
    if ((t = boundary(SEI_MOON, SE_MOON, D, 1, 0)) != 0)
      tau[B_MOON][nt[B_MOON]++] = t;
    if (k % 2 == 0 && (t = boundary(SEI_MARS, SE_MARS, D, 1, 0)) != 0)
      tau[B_MARS][nt[B_MARS]++] = t;
    if (k % 5 == 0 && (t = boundary(SEI_JUPITER, SE_JUPITER, D, 1, 0)) != 0)
      tau[B_JUP][nt[B_JUP]++] = t;
    if (k % 5 == 2 && (t = boundary(SEI_SATURN, SE_SATURN, D, 1, 0)) != 0)
      tau[B_SAT][nt[B_SAT]++] = t;
    if (k % 2 == 1 && (t = boundary(SEI_CERES, SE_CERES, D, 1, 0)) != 0)
      tau[B_CERES][nt[B_CERES]++] = t;
    if (y >= 1200 && y <= 4200 && (t = boundary(SEI_CHIRON, SE_CHIRON, D, 1, 0)) != 0)
      tau[B_CHIRON][nt[B_CHIRON]++] = t;
  }
  {
    double t;
    if ((t = boundary(SEI_EMB, SE_MARS, 2378495.0, 1, 1)) != 0)	/* sepl_12's last */
      tau[B_EMB][nt[B_EMB]++] = t;
    if ((t = boundary(SEI_MOON, SE_MOON, 2451545.0, 1, 0)) != 0)	/* J2000 + 17.23 */
      tau[B_MOON][nt[B_MOON]++] = t;
    if ((t = boundary(SEI_MOON, SE_MOON, 406157.6, 1, 1)) != 0)	/* semom36's first */
      tau[B_MOON][nt[B_MOON]++] = t;
  }
  printf("G176: across a segment boundary of the Swiss files, and a join for\n"
         "      SEFLG_SPEED3, the speeds are the rates of the places on the\n"
         "      instant's own side; SEFLG_SPEED3 divides by its spacing\n");
  for (ib = 0; ib < NBT; ib++)
    printf("  %-8s %2d steps\n", BNAME[ib], nt[ib]);
  for (part = 0; part < NPART; part++)
    nb[part] = nr[part] = 0;
  for (ib = 0; ib < NBT; ib++)
    for (it = 0; it < nt[ib]; it++) {
      cur_bt = ib;
      rows(ib, tau[ib][it] + (getenv("SEGSPEED_OFF") ? atof(getenv("SEGSPEED_OFF")) : 0), nb, nr);
    }
  /* steps on both sides: the Moon's first boundary in semo_114, 0.088 day
   * after the join of 11400. The true node's and the Moon's osculating
   * points' stencil (2^-6 day) crosses it ahead, and its one-sided reach
   * back the join, at J + 61/1024; the Earth's (2^-5) the join behind and
   * the boundary ahead at J + 5/128. On their own sides' places. */
  {
    double J = swe_julday(11400, 1, 1, 0, SE_GREG_CAL), tm = boundary(SEI_MOON, SE_MOON, J + 1.0 / 64, 1, 1);
    double ta = J + 61.0 / 1024, tb = J + 5.0 / 128;
    static const struct call ka[] = {
      {K_CALC, SE_TRUE_NODE, 0, 0, 0, 0, NULL, 0}, {K_CALC, SE_OSCU_APOG, 0, 0, 0, 0, NULL, 0},
    }, kb[] = {
      {K_NOD, SE_MOON, 0, 0, 0, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_MOON, 0, 2, 0, SE_NODBIT_OSCU, NULL, 0},
    }, kc[] = {
      {K_NOD, SE_SUN, 0, 2, SEFLG_NOGDEFL, SE_NODBIT_OSCU, NULL, 0},
      {K_NOD, SE_EARTH, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL, 0},
    };
    cur_bt = B_JOIN;
    if (!(tm - 2.0 / 64 < ta && ta < J + 4.0 / 64 && tb < J + 2.0 / 32 && tm < tb + 4.0 / 32)) {
      printf("    FAIL: the Moon's boundary after the join of 11400 at %+.6f, not within the stencils\n",
             tm - J);
      nbad++;
    } else
      for (i = 0; i < 2; i++) {
        part_begin(0, nb, nr);
        check_rate(0, &ka[i], tm, ta, -1, 1.0 / 512, i ? 1e-6 : 5e-8, 0, 1e-11);
        part_end(0, nb, nr);
        part_begin(1, nb, nr);
        check_rate(1, &kb[i], tm, ta, -1, 1.0 / 256, 2e-7, 0, 1e-11);
        check_rate(1, &kc[i], tm, tb, 1, 1.0 / 128, 2e-9, 0, 1e-11);
        part_end(1, nb, nr);
      }
  }
  cur_bt = 8;
  /* (i) SEFLG_SPEED3 elsewhere against the places' central rate */
  part = 8;
  part_begin(part, nb, nr);
  for (i = 0; i < 64; i++) {
    static const struct { int ipl; int32 fl; double s, lim; } ki[] = {
      {SE_SUN, 0, 1.0 / 256, 2e-9},
      {SE_MOON, 0, 1.0 / 1024, 3e-7},
      {SE_MOON, SEFLG_TOPOCTR, 1.0 / 1024, 3e-7},
      {SE_MARS, 0, 1.0 / 256, 2e-9},
      {SE_JUPITER, SEFLG_HELCTR, 1.0 / 256, 2e-9},
      {SE_CHIRON, 0, 1.0 / 256, 2e-9},
    };
    double jd = floor((2378500.0 + i * 3456.789) * 256) / 256 + 1.0 / 512;
    int m;
    for (m = 0; m < (int) (sizeof ki / sizeof ki[0]); m++) {
      struct call k = {K_CALC, ki[m].ipl, 0, 0, ki[m].fl, 0, NULL, SEFLG_SPEED3};
      check_rate(part, &k, jd, jd, 0, ki[m].s, ki[m].lim, 1e-9, 0);
    }
  }
  part_end(part, nb, nr);
  swe_ctx_free(ctx);
  for (part = 0; part < NPART - 1; part++)
    printf("  %-34s %4d rows, %3d failed, within %.3g deg/day\n", PART[part], nr[part], nb[part],
           worst[part]);
  if (getenv("SEGSPEED_V") != NULL)
    for (part = 0; part < NPART - 1; part++) {
      printf("  %d:", part);
      for (ib = 0; ib <= NBT; ib++)
        printf(" %s %.2e", ib < NBT ? BNAME[ib] : "else", worstbt[part][ib == NBT ? 8 : ib]);
      printf("\n");
    }
  printf("  distance rates within %.3g AU/day\n", worstr);
  printf("  rows %d, failed %d\n", nrow, nbad);
  printf(nbad ? "G176 FAIL: %d\n" : "G176 PASS\n", nbad);
  return nbad != 0;
}
