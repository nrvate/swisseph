/* framehold.c -- G155: the frame the true node's and osculating apogee's
 * speeds hold across a step of the precession is Owen 1990's piece too,
 * and Moshier's Moon's; and the long-term sidereal time's table takes at
 * each of Owen 1990's joins each interval's own piece's rate.
 *
 * The defect (UPSTREAM-BUGS 200): since entry 183 the model of an instant
 * is, with Owen 1990, its piece's (five fits of 80 centuries joined at JD
 * -1931455.5, 990544.5, 3912544.5 and 6834544.5, about -10000, -2000,
 * +6000 and +14000, the join's instant the later piece's), but two things
 * that take the instant's model took Owen 1990 whole. The frame hold of
 * the true node's and osculating apogee's speeds (entry 186), so within
 * 1/32 day of a join their five-point speed differenced the Moon in both
 * pieces' frames: on the Swiss files under "0,10" the node's 7.5e-6, 3.4e-6
 * and 1.1e-5 deg/day off at -10000, +6000 and +14000 (1.1e-11 at -2000,
 * where the pieces meet), the apogee's 1.5e-7, 9.3e-7 and 4.0e-7, the
 * node's distance up to 2.4e-11 AU/day. And the long-term sidereal time's
 * table (entry 181), whose nodes at -120, -40, +40 and +120 centuries,
 * half a day from the joins, took a rate over -+ 50 days of both pieces,
 * their mean, for the cubic Hermite of the intervals either side: its
 * rate's kink there 0 where the pieces' rates differ by 1.5e-5 to
 * 1.33e-4"/cy, the sidereal time 2.8e-9 deg off the per-piece Hermite at
 * +14000 (3.1e-10 to 6.6e-10 at the others).
 *
 * The defect (UPSTREAM-BUGS 202): the hold skipped Moshier's Moon, which is
 * of date and taken to J2000 and back by one frame at its epoch, but whose
 * velocity carries the frame's turning, which swi_plan_for_osc_elem() does
 * not take off: its node and apogee move with the precession model's rates
 * (5.0e-9 deg between IAU 1976 and Vondrak at 1800), and within 1/16 day
 * of a short-term window's edge the speed took that as a step between its
 * epochs: under "0,9,1" at J2000 - 2 and + 2 centuries the node's 9.5e-8
 * and 1.05e-7 deg/day off the side's model alone's, the apogee's 5.0e-6 and
 * 1.4e-6, the distances 2.5e-11 AU/day.
 *
 * Check (a): "0,10", the Swiss files, at the four joins' 105 instants
 * (G139's: every 1/16 day over -+ 2 days, and the join -+ 2^-k, k = 7..26):
 * the true node's and osculating apogee's true speeds (SEFLG_TRUEPOS: the
 * apparent places' retarded instants round at the JD's resolution, which
 * took the one-sided reference to 2.6e-7 deg/day on the apogee at +14000),
 * ecliptic and equatorial of date, the longitude's and latitude's and the
 * distance's, against five-point differences over 2^-6 day of the
 * library's places on the instant's side, centred where all five lie
 * there, else one-sided; the worst within 1/32 day and the rest: 32
 * checks, the node within 1e-9 deg/day, the apogee 1e-8, the distances
 * 1e-13 AU/day.
 *
 * Check (b): Moshier, "0,9,1" at J2000 -+ 2 centuries, the same bodies,
 * frames and speeds against G139 check (f)'s reference, nine-point central
 * differences over 2^-3 day of the places of the side's model alone
 * ("0,1,1" inside, "0,9,9" outside), at all 105 instants (Moshier's Moon
 * has no segments to keep the reference from), the worst within 1/16 day
 * (the speed's five points are 1/32 day apart) and the rest: 16 checks
 * within 1e-9 deg/day for the node, 5e-8 for the apogee (G74's), 4e-13
 * AU/day. "0,9,8"'s edges at -+ 75 centuries lie beyond Moshier's Moon (JD
 * 625000.5 to 2818000.5), and of Owen's joins only -2000 is within it.
 *
 * Check (c): the long-term sidereal time under "0,10" at the four join
 * nodes: its rate's kink there, the cubic-exact one-sided derivatives at
 * the node of the mean sidereal time less Vondrak's ("0,9,9": the same
 * nodes and no join; the rotation angle and the IERS terms cancel) from
 * points 1/4 century apart, left less right, against the precession's own,
 * -(dM/dt M^T)[1][0] by one-sided five-point differences over 32 days of
 * swi_precess() either side of the join: 4 checks within 2e-6"/cy (a rate
 * off by d puts the cubic off by up to 4/27 d over its century: 8e-11 deg
 * for 2e-6"/cy). It calls swi_precess() and is left out of the upstream
 * build.
 *
 * Measured after the fix: (a) within 1.4e-10 deg/day for the node, 1.1e-9
 * for the apogee, 2.2e-14 AU/day (the reference's own; against the side's
 * piece alone, a scratch build forcing the piece, the speeds are its own
 * to 1.4e-13 deg/day); (b) within 3.2e-10 and 8.2e-9, the reference's own
 * (4.5e-11 and 6.7e-9 within 1/16 day), 1.4e-13 AU/day; (c) within
 * 3.9e-7"/cy. Before the fix 24 of the 52 fail: 12 of (a), the node and
 * apogee within 1/32 day of the joins at -10000, +6000 and +14000 (3.2e-6
 * to 1.1e-5 and 1.5e-7 to 9.3e-7 deg/day), 8 of (b), all within 1/16 day
 * (9.2e-8 to 1.05e-7 and 1.4e-6 to 5.0e-6), and all 4 of (c) (the kink
 * 0). The upstream build fails all 48 of (a) and (b): its speeds are
 * differences over 1e-4 day (Moshier's over 0.1 day) across the join or
 * edge, the node 4.3e-4 to 1.5e-3 deg/day off within 1/32 day of the
 * three joins that step and Moshier's 0.12 within 1/16 day of the edges,
 * and are off elsewhere by the defects earlier entries fixed (the node
 * 1.6e-8 to 1.9e-5 beyond).
 * Run as
 *   framehold EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#ifndef SWE_UPSTREAM_COMPAT
#include "sweph.h"
#include "swephlib.h"
#endif

/* Owen 1990's joins (the join's instant the later piece's), and J2000 -+
 * 2 centuries, IAU 1976's window's edges (the edge's instant the
 * window's) */
static const double OWEN_JOIN[4] = { -1931455.5, 990544.5, 3912544.5, 6834544.5 };

#define EPHMASK (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)

static double TS;
static int LEFT;

/* t on the step's left side: before it, or at it where that is the left's */
static int left_of(double t)
{
  return LEFT ? (t <= TS) : (t < TS);
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

static const char *BN[2] = { "true node", "osc. apogee" };
static const int BI[2] = { SE_TRUE_NODE, SE_OSCU_APOG };

/* check (a): with Owen 1990 the long-term model ("0,10"), on the Swiss
 * files, at each join the true node's and the osculating apogee's true
 * speeds (SEFLG_TRUEPOS) against five-point differences over 2^-6 day of
 * the library's own places on the instant's side, centred where all five
 * lie there, else one-sided; the worst within 1/32 day of the join and the
 * rest, deg/day and AU/day; returns the failures, *n the checks. */
static int check_a(const char *ephe, int *n)
{
  static const double c5[2] = { 2.0 / 3, -1.0 / 12 };
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  static const double lim[2] = { 1e-9, 1e-8 }, dlim = 1e-13;
  static const char *sn[2] = { "within 1/32 day", "the rest" };
  const int32 eph = SEFLG_SWIEPH;
  const double h = 1.0 / 64;
  double tq[105];
  int m, b, f, i, j, k, q, bad = 0, nq;
  for (m = 0; m < 4; m++) {
    char mod[40] = "0,10,0,0,0,0,0,0", serr[256];
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, ephe);
    swe_set_astro_models_r(c, mod, 0);
    TS = OWEN_JOIN[m];
    LEFT = 0;
    nq = instants(tq);
    printf("  (a) Swiss, models 0,10, join JD %.1f: worst |speed - reference|, deg/day"
           " (distance AU/day), %s / %s\n", TS, sn[0], sn[1]);
    for (b = 0; b < 2; b++)
      for (f = 0; f < 2; f++) {
        double w[2] = { 0, 0 }, wd[2] = { 0, 0 };
        int32 fl = eph | SEFLG_TRUEPOS | (f ? SEFLG_EQUATORIAL : 0);
        for (i = 0; i < nq; i++) {
          double t = tq[i], x[6], p[5][6];
          int sp = fabs(t - TS) <= 1.0 / 32 ? 0 : 1, centred = 1;
          int dir = left_of(t) ? -1 : 1;
          if ((swe_calc_r(c, t, BI[b], fl | SEFLG_SPEED, x, serr) & EPHMASK) != eph) {
            printf("    ERROR %s\n", serr);
            bad++;
            continue;
          }
          for (k = 1; k <= 2; k++)
            if (left_of(t - k * h) != left_of(t) || left_of(t + k * h) != left_of(t))
              centred = 0;
          for (k = 0; k < 5; k++)
            swe_calc_r(c, centred ? t + (k - 2) * h : t + dir * k * h, BI[b], fl, p[k], serr);
          for (j = 0; j < 3; j++) {
            double d = 0, e, v[5];
            for (k = 0; k < 5; k++)
              v[k] = (j == 0) ? swe_difdeg2n(p[k][j], p[centred ? 2 : 0][j])
                              : p[k][j] - p[centred ? 2 : 0][j];
            if (centred)
              d = (c5[0] * (v[3] - v[1]) + c5[1] * (v[4] - v[0])) / h;
            else {
              for (k = 0; k < 5; k++)
                d += o5[k] * v[k];
              d *= dir / h;
            }
            e = fabs(x[3 + j] - d);
            if (j < 2 && !(e <= w[sp]))
              w[sp] = e;
            if (j == 2 && !(e <= wd[sp]))
              wd[sp] = e;
          }
        }
        printf("    %-12s %s  %.2e  %.2e  (distance %.1e  %.1e)\n", BN[b], f ? "equ" : "ecl",
               w[0], w[1], wd[0], wd[1]);
        for (q = 0; q < 2; q++) {
          (*n)++;
          if (!(w[q] <= lim[b] && wd[q] <= dlim)) {
            bad++;
            printf("    FAIL join JD %.1f, %s %s, %s: %.2e > %.0e or %.1e > %.0e\n",
                   TS, BN[b], f ? "equ" : "ecl", sn[q], w[q], lim[b], wd[q], dlim);
          }
        }
      }
    swe_ctx_free(c);
  }
  return bad;
}

/* check (b): on Moshier, with IAU 1976 short-term under Vondrak ("0,9,1"),
 * at J2000 -+ 2 centuries the true node's and the osculating apogee's
 * speeds against nine-point central differences over 2^-3 day of the
 * places of the instant's side's model alone ("0,1,1" inside, "0,9,9"
 * outside), G139 check (f)'s reference, at the 105 instants; the worst
 * within 1/16 day of the edge (the speed's five points 1/32 day apart)
 * and the rest; returns the failures, *n the checks. */
static int check_b(const char *ephe, int *n)
{
  static const struct {
    double ts;
    int left;
  } SE[] = { { 2378495.0, 0 }, { 2524595.0, 1 } };
  static const double c9[4] = { 4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280 };
  static const double lim[2] = { 1e-9, 5e-8 }, dlim = 4e-13;
  static const char *sn[2] = { "within 1/16 day", "the rest" };
  const char *ms[3] = { "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" };
  const double h = ldexp(1, -3);
  double tq[105];
  int m, b, f, i, j, k, q, bad = 0, nq;
  for (m = 0; m < 2; m++) {
    swe_ctx *cc[3];
    for (k = 0; k < 3; k++) {
      char mod[40];
      cc[k] = swe_ctx_new();
      strcpy(mod, ms[k]);
      swe_set_ephe_path_r(cc[k], ephe);
      swe_set_astro_models_r(cc[k], mod, 0);
    }
    TS = SE[m].ts;
    LEFT = SE[m].left;
    nq = instants(tq);
    printf("  (b) Moshier, models 0,9,1, edge JD %.1f: worst |speed - reference|, deg/day"
           " (distance AU/day), %s / %s\n", TS, sn[0], sn[1]);
    for (b = 0; b < 2; b++)
      for (f = 0; f < 2; f++) {
        double w[2] = { 0, 0 }, wd[2] = { 0, 0 };
        int32 fl = SEFLG_MOSEPH | (f ? SEFLG_EQUATORIAL : 0);
        for (i = 0; i < nq; i++) {
          double x[6], p[9][6];
          char serr[256];
          int inside = (TS < 2451545.0) ? !left_of(tq[i]) : left_of(tq[i]);
          int sp = fabs(tq[i] - TS) <= 1.0 / 16 ? 0 : 1;
          swe_ctx *cr = cc[inside ? 1 : 2];
          if ((swe_calc_r(cc[0], tq[i], BI[b], fl | SEFLG_SPEED, x, serr) & EPHMASK)
              != SEFLG_MOSEPH) {
            printf("    ERROR %s\n", serr);
            bad++;
            continue;
          }
          for (k = 0; k < 9; k++)
            swe_calc_r(cr, tq[i] + (k - 4) * h, BI[b], fl, p[k], serr);
          for (j = 0; j < 3; j++) {
            double d = 0, e;
            for (k = 1; k <= 4; k++)
              d += c9[k - 1] * (j == 0 ? swe_difdeg2n(p[4 + k][j], p[4 - k][j])
                                       : p[4 + k][j] - p[4 - k][j]);
            d /= h;
            e = fabs(x[3 + j] - d);
            if (j < 2 && !(e <= w[sp]))
              w[sp] = e;
            if (j == 2 && !(e <= wd[sp]))
              wd[sp] = e;
          }
        }
        printf("    %-12s %s  %.2e  %.2e  (distance %.1e  %.1e)\n", BN[b], f ? "equ" : "ecl",
               w[0], w[1], wd[0], wd[1]);
        for (q = 0; q < 2; q++) {
          (*n)++;
          if (!(w[q] <= lim[b] && wd[q] <= dlim)) {
            bad++;
            printf("    FAIL Moshier, edge JD %.1f, %s %s, %s: %.2e > %.0e or %.1e > %.0e\n",
                   TS, BN[b], f ? "equ" : "ecl", sn[q], w[q], lim[b], wd[q], dlim);
          }
        }
      }
    for (k = 0; k < 3; k++)
      swe_ctx_free(cc[k]);
  }
  return bad;
}

#ifndef SWE_UPSTREAM_COMPAT
/* the rotation rate of the mean equinox about the mean pole of date,
 * -(dM/dt M^T)[1][0] for M = J2000 -> mean of date (the long-term sidereal
 * time's integrand, swephlib.c sidt_pra_rate()), "/cy, at t from M at
 * t + dir k hh, k = 0..4, a one-sided five-point difference */
static double pra_rate(swe_ctx *c, double t, int dir, double hh)
{
  static const double o5[5] = { -25.0 / 12, 4, -3, 4.0 / 3, -1.0 / 4 };
  double m[5][3][3], w = 0;
  int i, j, k;
  for (j = 0; j < 5; j++)
    for (k = 0; k < 3; k++) {
      double x[6] = { 0, 0, 0, 0, 0, 0 };
      x[k] = 1;
      swi_precess(c, x, t + dir * j * hh, 0, J2000_TO_J);
      for (i = 0; i < 3; i++)
        m[j][i][k] = x[i];
    }
  for (k = 0; k < 3; k++) {
    double d = 0;
    for (j = 0; j < 5; j++)
      d += o5[j] * (m[j][1][k] - m[0][1][k]);
    w += d * dir / hh * m[0][0][k];
  }
  return w * RADTODEG * 3600.0 * 36525.0;
}

/* check (c): the long-term sidereal time with Owen 1990 the long-term
 * model ("0,10"), at the nodes of its table half a day after each join
 * (-120, -40, +40 and +120 centuries, TT): its rate from each side, the
 * cubic-exact one-sided derivative of the mean sidereal time less that of
 * Vondrak ("0,9,9", the same nodes, no join: the rotation angle and the
 * IERS terms cancel) at the node and 1/4, 1/2 and 3/4 century into the
 * interval, against the precession's own, pra_rate() on each side of the
 * join, the kink (left less right) of the two compared, "/cy; returns the
 * failures, *n the checks. */
static int check_c(int *n)
{
  static const double lim = 2e-6;
  const double dts = ldexp(1, -11), hc = 36525.0 / 4, hh = 32;
  char m0[40] = "0,10,0,0,0,0,0,0", m1[40] = "0,9,9,0,0,0,0,0";
  swe_ctx *co = swe_ctx_new(), *cv = swe_ctx_new();
  int m, s, j, bad = 0;
  swe_set_astro_models_r(co, m0, 0);
  swe_set_astro_models_r(cv, m1, 0);
  swe_set_delta_t_userdef_r(co, dts);
  swe_set_delta_t_userdef_r(cv, dts);
  printf("  (c) the long-term sidereal time, models 0,10 less 0,9,9: its rate's kink at the"
         " join's node against the precession's, \"/cy\n");
  for (m = 0; m < 4; m++) {
    double tn = 2451545.0 + (m < 2 ? -1 : 1) * (m % 3 == 0 ? 120 : 40) * 36525.0;
    double r[2], kp, ks, e;
    for (s = 0; s < 2; s++) {
      /* s = 0 the left interval's derivative at its end, 1 the right's */
      int dir = s ? 1 : -1;
      double v[4];
      for (j = 0; j < 4; j++) {
        double ut = tn + dir * j * hc - dts;
        double d = swe_sidtime0_r(co, ut, 23.4, 0) - swe_sidtime0_r(cv, ut, 23.4, 0);
        v[j] = (d - 24 * floor(d / 24 + 0.5)) * 54000;
      }
      r[s] = -dir * (11 * v[0] - 18 * v[1] + 9 * v[2] - 2 * v[3]) / (6 * hc) * 36525.0;
    }
    ks = r[0] - r[1];
    kp = pra_rate(co, OWEN_JOIN[m] - ldexp(1, -10), -1, hh) - pra_rate(co, OWEN_JOIN[m], 1, hh);
    e = fabs(ks - kp);
    printf("    node %+4.0f cy  kink %+.3e, the precession's %+.3e, off %.2e\n",
           (tn - 2451545.0) / 36525.0, ks, kp, e);
    (*n)++;
    if (!(e <= lim)) {
      bad++;
      printf("    FAIL node %+4.0f cy: %.2e > %.0e\n", (tn - 2451545.0) / 36525.0, e, lim);
    }
  }
  swe_ctx_free(co);
  swe_ctx_free(cv);
  return bad;
}
#endif

int main(int argc, char **argv)
{
  int bad = 0, n = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: framehold EPHEDIR\n");
    return 2;
  }
  printf("G155: the frame hold keeps Owen 1990's piece and Moshier's Moon's frame\n");
  bad += check_a(argv[1], &n);
  bad += check_b(argv[1], &n);
#ifndef SWE_UPSTREAM_COMPAT
  bad += check_c(&n);
#endif
  if (bad) {
    printf("G155 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG155 PASS\n", n);
  return 0;
}
