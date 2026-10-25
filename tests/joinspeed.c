/* joinspeed.c -- G166: across a join of the Swiss files every speed is the
 * rate of the places on the instant's own side of the join.
 *
 * The defect (UPSTREAM-BUGS 250). Each read comes from the file of its own
 * instant (entry 89), and at a 600-year join the two files' places step:
 * at JD 2378496.5 (sepl_12 / sepl_18) Mars 6.3e-8 deg, the Earth 1.6e-9
 * AU and 2.1e-9 AU/day in velocity, the barycentric Sun 2.5e-9 AU/day,
 * Chiron (seas) 1.3e-7 AU; Mars's heliocentric osculating node 4.2e-6 deg.
 * A speed formed from reads at other instants than the place's took that
 * step for motion wherever its reads straddled a join:
 *  - the true node's and the osculating apogee's five points 2^-6 day
 *    apart (lunar_osc_elem()): 2^-7 day after the join 4.5e-6 and 1.5e-5
 *    deg/day off their places' rate;
 *  - swe_nod_aps()'s osculating ellipse at tjd + k 2^-5 day: Mars's
 *    heliocentric node 7.8e-5, its perihelion 3.3e-3, Pluto's 6.4e-3,
 *    Chiron's node 1.3e-2, Ceres's 2.7e-3, the Moon's apsides 1.5e-5;
 *  - swe_nod_aps()'s aberration rate, the observer read at tjd less the
 *    points' light time: the Sun's mean perigee 2^-10 day after the join
 *    1.4e-7, Mars's mean node 1.3e-8, Pluto's points 2.9e-9;
 *  - swe_calc()'s aberration rate, V(t) - V(t - tau) of the Earth, within
 *    the light time after the join: the Sun 1.2e-7, Mercury 1.7e-7, Mars
 *    5.0e-8, Jupiter 3.0e-8, Chiron 1.0e-8, the Moon 1.0e-5 (its 1.3 s);
 *  - a star's, the Earth's velocities at tjd -+ 2^-9 day: Aldebaran 1.7e-7
 *    within 2^-9 day of the join;
 *  - swe_calc_pctr()'s, the centre's velocities at tjd - d and tjd +
 *    2^-10: Mars from Jupiter 2^-11 day before the join 1.0e-7.
 *
 * The check, the complete directory, at each of the 50 joins -12600..16800
 * (1 January, Julian before 1600, as swi_gen_filename() reckons them; each
 * confirmed a change of file name), every speed on the sky (|dv x r| / r^2
 * of the equatorial XYZ of date) against the one-sided five-point
 * difference, (-25, 48, -36, 16, -3) / 12, of the places without
 * SEFLG_SPEED on the side of the instant's reads, never crossing a step:
 *  (a) SE_TRUE_NODE and SE_OSCU_APOG, apparent and SEFLG_TRUEPOS, at J -
 *      2^-7, J, J + 2^-20 and J + 2^-7 (the apparent Moon read 1.3 s
 *      earlier: at J and J + 2^-20 before the join), over 2^-6 day:
 *      within 5e-8 (node) and 1e-6 deg/day (apogee), the references' own
 *      noise at the far joins (2.2e-8 and 6.1e-7 a day off them), the
 *      distance's within 1e-11 AU/day;
 *  (b) swe_nod_aps() osculating, the ascending node and the perihelion:
 *      Mars heliocentric and geocentric, Saturn's barycentric ellipse
 *      (SE_NODBIT_OSCU_BAR), the Sun (the Earth's orbit, SEFLG_NOGDEFL)
 *      and the Earth heliocentric (perihelion), the Moon (2^-7 day), Ceres
 *      heliocentric, and Chiron at the joins of its file (1200-4200), at J
 *      - 2^-7, J, J + 2^-7 and J + 2^-5 + 2^-7, over 2^-5 day: within 2e-9
 *      deg/day (the Moon's 1e-7, its perigee's reference 5.9e-8 off the
 *      joins) and 1e-11 AU/day;
 *  (c) swe_nod_aps() mean, SEFLG_NOGDEFL: the Sun's perigee and Mars's
 *      node geocentric, Mars's topocentric, at J + 2^-10 and J + 2^-14,
 *      over 2^-8 day forward: within 1e-9 (topocentric 3e-9, 2.2e-9 off
 *      the joins);
 *  (d) swe_calc() apparent within the light time after the join, the
 *      reference short of J + the light time, where the body's own read
 *      crosses it: the Sun and Mercury at J + 2^-10 over 2^-11 day, Mars
 *      and Mars topocentric there over 2^-12, Jupiter and Ceres at J +
 *      2^-8 over 2^-10, Cupido at J + 2^-4 over 2^-6: within 6e-9 (4.4e-9
 *      off the joins, Mercury's and Mars's references);
 *  (e) the Moon, whose 1.3 s no difference resolves: the aberration's share
 *      of its speed (the speed less the one under SEFLG_NOABERR, longitude
 *      by cos lat, and latitude), geocentric and topocentric, at J, J +
 *      2^-20 and J + 2^-17, within 5e-9 deg/day of its line from J -
 *      2^-17 to J + 2^-15 (both outside the light time);
 *  (f) Aldebaran and Polaris by swe_fixstar2() at J - 2^-10, J and J +
 *      2^-10, over 2^-8 day: within 1e-9;
 *  (g) swe_calc_pctr(), Mars from Jupiter and the Moon from Mars, at J -
 *      2^-11 and J + 2^-11, over 2^-12 day: within 6e-9 (3.5e-9 off the
 *      joins);
 * and every speed call of (a)-(g) bit for bit the same call in a fresh
 * context (this one's warm from the rows before it, other joins and
 * sides), its place bit for bit the call's without SEFLG_SPEED; a row
 * fails once.
 * Measured: 4698 rows pass, (a) within 6.1e-7 deg/day (the apogee), (b)
 * 2.6e-8 (the Moon), (c) 2.2e-9, (d) 2.7e-9, (e) 1.9e-9, (f) 6.2e-11, (g)
 * 3.1e-9, the distances 3.6e-12 AU/day; the same rows 0.5, 3 and -3 days
 * off the joins pass too. The upstream build fails 4530 rows, ts20 before
 * the fix 4150: (a) all 800 (up to 1.2e-3 deg/day; 2^-7 day after the
 * 1800 join the node 4.5e-6), (b) 2348 (0.12; Chiron's node at 1800
 * 1.3e-2), (c) 252 (1.4e-7), (d) 56 (1.7e-7), (e) all 300 (9.6e-6), (f)
 * 294 (1.8e-7), (g) 100 (1.0e-6).
 *
 * Run as
 *   joinspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

extern void swi_gen_filename(double tjd, int ipli, char *fname);

#define SEI_SUNBARY_ 10	/* sweph.h's SEI_SUNBARY: a planet file's name */
#define FLX (SEFLG_XYZ | SEFLG_EQUATORIAL)

static const char *EPHE;
static swe_ctx *ctx;
static int nrow, nbad;
static double worst[8], worstr;
static const char *const PART[8] = {"(a) true node, apogee", "(b) nod_aps osculating",
                                    "(c) nod_aps mean", "(d) swe_calc in the light time",
                                    "(e) the Moon's aberration share", "(f) stars",
                                    "(g) swe_calc_pctr", "bit for bit"};

/* one call: what it is and its answer */
enum { K_CALC, K_NOD, K_STAR, K_PCTR };
struct call { int kind, ipl, ictr, pt; int32 fl, meth; const char *star; };
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

static void fail(int part, const char *what, const struct call *k, double J, double jd, double v)
{
  nbad++;
  if (nbad <= 30)
    printf("    FAIL %s: %s, body %d (pt %d, ctr %d%s%s), flags %#x, JD %.10f (J %+.3g): %.3e\n",
           PART[part], what, k->ipl, k->pt, k->ictr, k->star ? ", " : "", k->star ? k->star : "",
           (unsigned) k->fl, jd, jd - J, v);
}

/* the speed call at jd against the five-point one-sided difference of the
 * places on side dir, step s, and its fresh and unflagged twins; lim on
 * the sky, deg/day, and radial, AU/day (rlim 0: not checked) */
static void check_rate(int part, const struct call *k, double J, double jd, int dir, double s,
                       double lim, double rlim)
{
  int32 fl = k->fl | FLX;
  struct ans a, b, z, p[5];
  double v[3], d[3], r = 0, dr = 0, dd = 0, ang;
  int i, j, bad0 = nbad;
  nrow++;
  run(ctx, k, jd, fl | SEFLG_SPEED, &a);
  if (a.r < 0) {
    fail(part, a.serr, k, J, jd, 0);
    return;
  }
  /* bit for bit: a fresh context, and the place without SEFLG_SPEED */
  {
    swe_ctx *c = fresh();
    run(c, k, jd, fl | SEFLG_SPEED, &b);
    swe_ctx_free(c);
  }
  run(ctx, k, jd, fl, &z);
  if (b.r != a.r || memcmp(b.x, a.x, sizeof a.x) != 0)
    fail(7, "the speed call in a fresh context differs", k, J, jd, fabs(b.x[3] - a.x[3]));
  if (memcmp(z.x, a.x, 3 * sizeof(double)) != 0)
    fail(7, "the place differs from the call's without SEFLG_SPEED", k, J, jd,
         fabs(z.x[0] - a.x[0]));
  for (j = 0; j < 5; j++)
    run(ctx, k, jd + dir * j * s, fl, &p[j]);
  for (i = 0; i < 3; i++) {
    v[i] = (-25 * p[0].x[i] + 48 * p[1].x[i] - 36 * p[2].x[i] + 16 * p[3].x[i] - 3 * p[4].x[i])
           / (12 * s * dir);
    d[i] = a.x[i + 3] - v[i];
    r += a.x[i] * a.x[i];
  }
  r = sqrt(r);
  for (i = 0; i < 3; i++)
    dr += d[i] * a.x[i] / r;
  for (i = 0; i < 3; i++) {
    double q = d[i] - dr * a.x[i] / r;
    dd += q * q;
  }
  ang = sqrt(dd) / r * 180 / M_PI;
  if (ang > worst[part])
    worst[part] = ang;
  if (rlim > 0 && fabs(dr) > worstr)
    worstr = fabs(dr);
  if (!(ang <= lim))
    fail(part, "speed off its places' rate, deg/day", k, J, jd, ang);
  else if (rlim > 0 && !(fabs(dr) <= rlim))
    fail(part, "distance rate off its places', AU/day", k, J, jd, fabs(dr));
  if (nbad > bad0)
    nbad = bad0 + 1;	/* a row fails once */
}

/* (e): the aberration's share of the Moon's speed */
static void moon_share(swe_ctx *c, double jd, int32 fl, double *sh)
{
  struct call k = {K_CALC, SE_MOON, 0, 0, 0, 0, NULL};
  struct ans a, b;
  run(c, &k, jd, fl | SEFLG_SPEED, &a);
  run(c, &k, jd, fl | SEFLG_SPEED | SEFLG_NOABERR, &b);
  sh[0] = (a.x[3] - b.x[3]) * cos(a.x[1] * M_PI / 180);
  sh[1] = a.x[4] - b.x[4];
}

int main(int argc, char **argv)
{
  static const double HP7 = 1.0 / 128, HP5 = 1.0 / 32, HP6 = 1.0 / 64, HP8 = 1.0 / 256;
  double rgJ[64];
  int nj = 0, ij, i, j, y, nb[8], nr[8], part;
  if (argc != 2) {
    fprintf(stderr, "usage: joinspeed EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  ctx = fresh();
  for (y = -12600; y <= 16800; y += 600) {
    double J = swe_julday(y, 1, 1, 0, y >= 1600 ? SE_GREG_CAL : SE_JUL_CAL);
    char s0[AS_MAXCH], s1[AS_MAXCH];
    swi_gen_filename(nextafter(J, -HUGE_VAL), SEI_SUNBARY_, s0);
    swi_gen_filename(J, SEI_SUNBARY_, s1);
    if (strcmp(s0, s1) == 0) {
      printf("  not a join: %d, JD %.1f (%s)\n", y, J, s1);
      nbad++;
    }
    rgJ[nj++] = J;
  }
  printf("G166: across a join of the Swiss files the speeds are the rates of the\n"
         "      places on the instant's own side\n");
  for (part = 0; part < 8; part++)
    nb[part] = nr[part] = 0;
  for (ij = 0; ij < nj; ij++) {
    double J = rgJ[ij];
    int yr = -12600 + 600 * ij;
    /* (a) */
    part = 0;
    nb[part] -= nbad;
    nr[part] -= nrow;
    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++) {
        struct call k = {K_CALC, i ? SE_OSCU_APOG : SE_TRUE_NODE, 0, 0, j ? SEFLG_TRUEPOS : 0, 0, NULL};
        double lim = i ? 1e-6 : 5e-8;
        check_rate(part, &k, J, J - HP7, -1, HP6, lim, 1e-11);
        check_rate(part, &k, J, J, j ? 1 : -1, HP6, lim, 1e-11);
        check_rate(part, &k, J, J + 1.0 / 1048576, j ? 1 : -1, HP6, lim, 1e-11);
        check_rate(part, &k, J, J + HP7, 1, HP6, lim, 1e-11);
      }
    nb[part] += nbad;
    nr[part] += nrow;
    /* (b) */
    part = 1;
    nb[part] -= nbad;
    nr[part] -= nrow;
    {
      static const struct call kb[] = {
        {K_NOD, SE_MARS, 0, 0, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_MARS, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_MARS, 0, 0, 0, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_MARS, 0, 2, 0, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_SATURN, 0, 0, SEFLG_BARYCTR, SE_NODBIT_OSCU_BAR, NULL},
        {K_NOD, SE_SATURN, 0, 2, SEFLG_BARYCTR, SE_NODBIT_OSCU_BAR, NULL},
        {K_NOD, SE_SUN, 0, 2, SEFLG_NOGDEFL, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_EARTH, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_MOON, 0, 0, 0, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_MOON, 0, 2, 0, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_CERES, 0, 0, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_CERES, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_CHIRON, 0, 0, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL},
        {K_NOD, SE_CHIRON, 0, 2, SEFLG_HELCTR, SE_NODBIT_OSCU, NULL},
      };
      for (i = 0; i < (int) (sizeof kb / sizeof kb[0]); i++) {
        double s = kb[i].ipl == SE_MOON ? HP7 : HP5, lim = kb[i].ipl == SE_MOON ? 1e-7 : 2e-9;
        if (kb[i].ipl == SE_CHIRON && (yr < 1200 || yr > 4200))
          continue;	/* Chiron's file: 675 to 4650 */
        check_rate(part, &kb[i], J, J - HP7, -1, s, lim, 1e-11);
        check_rate(part, &kb[i], J, J, 1, s, lim, 1e-11);
        check_rate(part, &kb[i], J, J + HP7, 1, s, lim, 1e-11);
        check_rate(part, &kb[i], J, J + HP5 + HP7, 1, s, lim, 1e-11);
      }
    }
    nb[part] += nbad;
    nr[part] += nrow;
    /* (c) */
    part = 2;
    nb[part] -= nbad;
    nr[part] -= nrow;
    {
      static const struct call kc[] = {
        {K_NOD, SE_SUN, 0, 2, SEFLG_NOGDEFL, SE_NODBIT_MEAN, NULL},
        {K_NOD, SE_MARS, 0, 0, SEFLG_NOGDEFL, SE_NODBIT_MEAN, NULL},
        {K_NOD, SE_MARS, 0, 0, SEFLG_NOGDEFL | SEFLG_TOPOCTR, SE_NODBIT_MEAN, NULL},
      };
      for (i = 0; i < (int) (sizeof kc / sizeof kc[0]); i++) {
        double lim = kc[i].fl & SEFLG_TOPOCTR ? 3e-9 : 1e-9;
        check_rate(part, &kc[i], J, J + 1.0 / 1024, 1, HP8, lim, 0);
        check_rate(part, &kc[i], J, J + 1.0 / 16384, 1, HP8, lim, 0);
      }
    }
    nb[part] += nbad;
    nr[part] += nrow;
    /* (d) */
    part = 3;
    nb[part] -= nbad;
    nr[part] -= nrow;
    {
      /* the reference stays short of J + the light time, where the
       * body's own read crosses the join */
      static const struct { struct call k; double d, s; } kd[] = {
        {{K_CALC, SE_SUN, 0, 0, 0, 0, NULL}, 1.0 / 1024, 1.0 / 2048},
        {{K_CALC, SE_MERCURY, 0, 0, 0, 0, NULL}, 1.0 / 1024, 1.0 / 2048},
        {{K_CALC, SE_MARS, 0, 0, 0, 0, NULL}, 1.0 / 1024, 1.0 / 4096},
        {{K_CALC, SE_MARS, 0, 0, SEFLG_TOPOCTR, 0, NULL}, 1.0 / 1024, 1.0 / 4096},
        {{K_CALC, SE_JUPITER, 0, 0, 0, 0, NULL}, 1.0 / 256, 1.0 / 1024},
        {{K_CALC, SE_CERES, 0, 0, 0, 0, NULL}, 1.0 / 256, 1.0 / 1024},
        {{K_CALC, SE_CUPIDO, 0, 0, 0, 0, NULL}, 1.0 / 16, 1.0 / 64},
      };
      for (i = 0; i < (int) (sizeof kd / sizeof kd[0]); i++)
        check_rate(part, &kd[i].k, J, J + kd[i].d, 1, kd[i].s, 6e-9, 0);
    }
    nb[part] += nbad;
    nr[part] += nrow;
    /* (e) */
    part = 4;
    nb[part] -= nbad;
    nr[part] -= nrow;
    for (j = 0; j < 2; j++) {
      int32 fl = j ? SEFLG_TOPOCTR : 0;
      double ta = J - 1.0 / 131072, tb = J + 1.0 / 32768, a[2], b[2], c[2];
      double tin[3] = {J, J + 1.0 / 1048576, J + 1.0 / 131072};
      struct call k = {K_CALC, SE_MOON, 0, 0, fl, 0, NULL};
      moon_share(ctx, ta, fl, a);
      moon_share(ctx, tb, fl, b);
      for (i = 0; i < 3; i++) {
        double f = (tin[i] - ta) / (tb - ta), e = 0;
        int m;
        nrow++;
        moon_share(ctx, tin[i], fl, c);
        for (m = 0; m < 2; m++)
          if (fabs(c[m] - (a[m] + f * (b[m] - a[m]))) > e)
            e = fabs(c[m] - (a[m] + f * (b[m] - a[m])));
        if (e > worst[part])
          worst[part] = e;
        if (!(e <= 5e-9))
          fail(part, "the aberration's share of the speed off its line, deg/day", &k, J, tin[i], e);
      }
    }
    nb[part] += nbad;
    nr[part] += nrow;
    /* (f) */
    part = 5;
    nb[part] -= nbad;
    nr[part] -= nrow;
    for (i = 0; i < 2; i++) {
      struct call k = {K_STAR, 0, 0, 0, 0, 0, i ? "Polaris" : "Aldebaran"};
      check_rate(part, &k, J, J - 1.0 / 1024, -1, HP8, 1e-9, 0);
      check_rate(part, &k, J, J, 1, HP8, 1e-9, 0);
      check_rate(part, &k, J, J + 1.0 / 1024, 1, HP8, 1e-9, 0);
    }
    nb[part] += nbad;
    nr[part] += nrow;
    /* (g) */
    part = 6;
    nb[part] -= nbad;
    nr[part] -= nrow;
    for (i = 0; i < 2; i++) {
      struct call k = {K_PCTR, i ? SE_MOON : SE_MARS, i ? SE_MARS : SE_JUPITER, 0, 0, 0, NULL};
      check_rate(part, &k, J, J - 1.0 / 2048, -1, 1.0 / 4096, 6e-9, 0);
      check_rate(part, &k, J, J + 1.0 / 2048, 1, 1.0 / 4096, 6e-9, 0);
    }
    nb[part] += nbad;
    nr[part] += nrow;
  }
  swe_ctx_free(ctx);
  for (part = 0; part < 7; part++)
    printf("  %-34s %4d rows, %3d failed, within %.3g deg/day\n", PART[part], nr[part], nb[part],
           worst[part]);
  printf("  distance rates within %.3g AU/day\n", worstr);
  printf("  rows %d, failed %d\n", nrow, nbad);
  printf(nbad ? "G166 FAIL: %d\n" : "G166 PASS\n", nbad);
  return nbad != 0;
}
