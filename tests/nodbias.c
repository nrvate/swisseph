/* nodbias.c -- G163: swe_nod_aps() changes centre in one frame.
 *
 * The defect (UPSTREAM-BUGS 221). swe_nod_aps() builds its points in the
 * J2000 frame: the mean ones from elements of the ecliptic of date, the
 * osculating ones from a state that swi_plan_for_osc_elem() takes from
 * ICRS to J2000 with the frame bias. It then changed their centre by
 * adding and taking away the barycentric Sun and Earth and the site, which
 * are in the ephemeris's frame, ICRS for DE 403 and later (the Swiss files,
 * Moshier, DE4xx; swi_get_observer() turns the site into it). So every
 * point was off by the bias, 23 mas, times whatever of those vectors does
 * not cancel: a planet's geocentric and topocentric points 3.5e-8 AU off
 * its heliocentric ones less the Earth at J2000, 9.6e-8 AU and 1.9e-9
 * AU/day at the instants here (up to 2.6e-6 deg, Mercury's, on the
 * apparent points of date); the barycentric ones 9.7e-10 AU off the
 * heliocentric plus the Sun, as the heliocentric points of a barycentric
 * ellipse (SE_NODBIT_OSCU_BAR beyond 6 AU) off the barycentric less it;
 * the Moon's heliocentric and barycentric points 9.6e-8 AU off its
 * geocentric ones plus the Earth (1.9e-6 deg); its and the Sun's
 * topocentric points 4.8e-12 AU and 2.0e-11 AU/day off the geocentric
 * ones less the site (8.8e-7 deg on the Moon's focal point). With
 * SEFLG_ICRS, where all are ICRS, and on de200, which has no bias, 7e-15.
 *
 * The check, the complete directory, the Swiss files and Moshier at
 * -3000, -1000, 2000 and 3000, de200 at 1900, 2000 and 2050; the Sun, the
 * Moon, Mercury, Venus, Mars, Jupiter, Saturn, Neptune, Pluto and the
 * Earth; mean, osculating, SE_NODBIT_OSCU_BAR, each with SE_NODBIT_FOPOINT
 * but the osculating one without; geometric (SEFLG_TRUEPOS |
 * SEFLG_NOABERR | SEFLG_NOGDEFL) cartesian with speed, J2000 equatorial and
 * ecliptic of date, each with and without SEFLG_ICRS:
 *  (a) the points of each centre against one centre's points and
 *      swe_calc()'s vectors in the same frame: a planet's, the Sun's and
 *      the Earth's geocentric the heliocentric less the Earth's
 *      heliocentric place (the Sun's the heliocentric negated, the Earth's
 *      orbit seen from the Sun), topocentric that less the site (the Sun's
 *      geocentric less topocentric place), barycentric the heliocentric
 *      plus the barycentric Sun; the Moon's heliocentric and barycentric
 *      the geocentric plus the Earth's place, topocentric less the site;
 *      position and velocity within 1e-14 of the vectors' sizes plus 1 AU
 *      (0.02 AU/day), the Earth's orbit's nodes left out (zero), Moshier's
 *      refusals of the barycentric points and ellipses (bug 149) taken;
 *  (b) the osculating apsides (and focal point) on the files and Moshier,
 *      J2000 equatorial, every centre: the SEFLG_ICRS ones turned by the
 *      IAU 2006 bias matrix, within 1e-12 of their distance (plus 1e-15
 *      AU) and 2e-11 of it a day (plus 1e-13 AU/day): the ellipse's
 *      rounding, built in two frames, on Neptune's and Venus's small
 *      eccentricities;
 *  (c) where nothing is added or taken away, the points as with
 *      SEFLG_ICRS bit for bit: the Moon's mean geocentric points on the
 *      three ephemerides (they read no file, bug 174, and must not take a
 *      DE number's frame), Moshier's mean heliocentric ones; J2000
 *      cartesian, geometric and apparent of date, with speed.
 * Measured: (a) 3.8e-14 AU and 1.4e-17 AU/day at most, 4.7e-16 of the
 * sizes; (b) 3.1e-12 AU and 1.3e-10 AU/day (Neptune's); (c) 210 rows. 71218
 * rows. Before the fix 18698 fail, 16128 of (a) (9.6e-8 AU) and 2570 of (b)
 * (9.6e-8 AU); with SEFLG_ICRS and on de200 none. The upstream build
 * (SWE_UPSTREAM_COMPAT) fails 53006 of its 77906 (its other defects in the
 * same rows too: the J2000 frame, bug 47; the Sun's observer, 167;
 * Moshier's barycentric points, 149). Run as
 *   nodbias EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static int nbad, nrow;

static void check(int ok, const char *part, const char *what, double got, double lim)
{
  nrow++;
  if (ok)
    return;
  nbad++;
  if (nbad <= 40)
    printf("    FAIL (%s) %s: %.3e, limit %.0e\n", part, what, got, lim);
}

static int32 nodaps(double jd, int ipl, int32 fl, int32 meth, double x[24], char *serr)
{
  memset(x, 0, 24 * sizeof(double));
  return swe_nod_aps(jd, ipl, fl, meth, x, x + 6, x + 12, x + 18, serr);
}

static int32 calc(double jd, int ipl, int32 fl, double x[6], char *serr)
{
  memset(x, 0, 6 * sizeof(double));
  return swe_calc(jd, ipl, fl, x, serr);
}

/* the frame bias, IAU 2006 (the default model): x_J2000 = RB^T x_ICRS */
static const double RB[3][3] = {
  {+0.99999999999999412, +0.00000007078368695, -0.00000008056214212},
  {-0.00000007078368961, +0.99999999999999700, -0.00000003306427981},
  {+0.00000008056213978, +0.00000003306428553, +0.99999999999999634},
};

static void bias(const double *x, double *y)
{
  int i, v;
  for (v = 0; v < 6; v += 3)
    for (i = 0; i < 3; i++)
      y[v + i] = x[v] * RB[0][i] + x[v + 1] * RB[1][i] + x[v + 2] * RB[2][i];
}

/* the length of point k's position (v = 0) or velocity (v = 1) */
static double size(const double *x, int k, int v)
{
  const double *p = x + 6 * k + 3 * v;
  return sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
}

/* |got - want| of point k's position or velocity */
static double off(const double *got, const double *want, int k, int v)
{
  double s = 0;
  int i;
  for (i = 0; i < 3; i++) {
    double d = got[6 * k + 3 * v + i] - want[6 * k + 3 * v + i];
    s += d * d;
  }
  return sqrt(s);
}

static const int32 E[3] = {SEFLG_SWIEPH, SEFLG_MOSEPH, SEFLG_JPLEPH};
static const char *EN[3] = {"Swiss", "Moshier", "de200"};
static const double JDF[4] = {625400.5, 1355818.5, 2451545.0, 2816500.5};	/* -3000..3000 */
static const double JDJ[3] = {2415020.25, 2451545.0, 2469807.5};	/* de200's years */
static const int B[10] = {SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER,
                          SE_SATURN, SE_NEPTUNE, SE_PLUTO, SE_EARTH};
static const int32 M[6] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR,
                           SE_NODBIT_MEAN | SE_NODBIT_FOPOINT, SE_NODBIT_OSCU | SE_NODBIT_FOPOINT,
                           SE_NODBIT_OSCU_BAR | SE_NODBIT_FOPOINT};
/* J2000 equatorial and ecliptic of date, each without and with SEFLG_ICRS */
static const int32 F[4] = {SEFLG_J2000 | SEFLG_EQUATORIAL, SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_ICRS,
                           0, SEFLG_ICRS};
static const char *FN[4] = {"J2000", "J2000 ICRS", "of date", "of date ICRS"};
static const int32 CF[4] = {0, SEFLG_TOPOCTR, SEFLG_HELCTR, SEFLG_BARYCTR};
static const char *CN[4] = {"geo", "topo", "helio", "bary"};

int main(int argc, char **argv)
{
  const int32 geom = SEFLG_TRUEPOS | SEFLG_NOABERR | SEFLG_NOGDEFL | SEFLG_XYZ | SEFLG_SPEED;
  double wa[2] = {0, 0}, wq[2] = {0, 0}, wb[2] = {0, 0};
  char serr[AS_MAXCH], what[300];
  int ie, it, ib, im, iff, ic, k, v, i, nc = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: nodbias EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  swe_set_jpl_file("de200.eph");
  printf("G163: swe_nod_aps() changes centre in one frame\n");
  for (ie = 0; ie < 3; ie++)
    for (it = 0; it < (ie == 2 ? 3 : 4); it++)
      for (iff = 0; iff < 4; iff++)
        for (ib = 0; ib < 10; ib++)
          for (im = 0; im < 6; im++) {
            double jd = ie == 2 ? JDJ[it] : JDF[it];
            int32 fl = E[ie] | geom | F[iff], r[4];
            int ipl = B[ib], moon = ipl == SE_MOON, sun = ipl == SE_SUN;
            int earthorb = sun || ipl == SE_EARTH;
            int beyond = ipl == SE_SATURN || ipl == SE_NEPTUNE || ipl == SE_PLUTO;
            int osc = M[im] & (SE_NODBIT_OSCU | SE_NODBIT_OSCU_BAR);
            /* Moshier refuses the barycentric points, and its barycentric
             * ellipses beyond 6 AU whatever the centre (bug 149) */
            int mosh_refuses_all = ie == 1 && beyond && (M[im] & SE_NODBIT_OSCU_BAR);
            double p[4][24], ref[4][24], pi[24], eh[6], eb[6], sb[6], sg[6], st[6], site[6];
            for (ic = 0; ic < 4; ic++) {
              r[ic] = nodaps(jd, ipl, fl | CF[ic], M[im], p[ic], serr);
              if (r[ic] < 0 && !(ie == 1 && (ic == 3 || mosh_refuses_all))) {
                snprintf(what, sizeof what, "%s jd %.2f body %d method %d %s %s refused: %.100s",
                         EN[ie], jd, ipl, (int) M[im], FN[iff], CN[ic], serr);
                check(0, "a", what, 0, 0);
              }
            }
            if (mosh_refuses_all)
              continue;
            if (calc(jd, SE_EARTH, fl | SEFLG_HELCTR, eh, serr) < 0
                || calc(jd, SE_SUN, fl, sg, serr) < 0
                || calc(jd, SE_SUN, fl | SEFLG_TOPOCTR, st, serr) < 0
                || (ie != 1 && (calc(jd, SE_EARTH, fl | SEFLG_BARYCTR, eb, serr) < 0
                                || calc(jd, SE_SUN, fl | SEFLG_BARYCTR, sb, serr) < 0))) {
              snprintf(what, sizeof what, "%s jd %.2f swe_calc() refused: %.100s", EN[ie], jd, serr);
              check(0, "a", what, 0, 0);
              continue;
            }
            for (i = 0; i < 6; i++)
              site[i] = sg[i] - st[i];
            /* (a) every centre from one centre's points and swe_calc()'s vectors */
            for (k = 0; k < 4; k++)
              for (i = 0; i < 6; i++) {
                int j = 6 * k + i;
                if (moon) {
                  double g = p[0][j];
                  ref[0][j] = g;
                  ref[1][j] = g - site[i];
                  ref[2][j] = g + eh[i];
                  ref[3][j] = ie != 1 ? g + eb[i] : 0;
                } else {
                  double h = p[2][j];
                  ref[0][j] = sun ? -h : h - eh[i];
                  ref[1][j] = ref[0][j] - site[i];
                  ref[2][j] = h;
                  ref[3][j] = ie != 1 ? h + sb[i] : 0;
                }
              }
            for (ic = 0; ic < 4; ic++) {
              if (r[ic] < 0)
                continue;
              for (k = earthorb ? 2 : 0; k < 4; k++)
                for (v = 0; v < 2; v++) {
                  double d = off(p[ic], ref[ic], k, v);
                  double sc = size(p[ic], k, v) + size(ref[ic], k, v) + size(p[moon ? 0 : 2], k, v)
                              + (v ? 0.02 : 1);
                  if (d > wa[v])
                    wa[v] = d;
                  if (d / sc > wq[v])
                    wq[v] = d / sc;
                  snprintf(what, sizeof what, "%s jd %.2f body %d method %d %s %s point %d %s, AU%s",
                           EN[ie], jd, ipl, (int) M[im], FN[iff], CN[ic], k,
                           v ? "velocity" : "position", v ? "/day" : "");
                  check(d <= 1e-14 * sc, "a", what, d, 1e-14 * sc);
                }
            }
            /* (b) the osculating apsides the SEFLG_ICRS ones biased */
            if (!osc || iff != 0 || ie == 2)
              continue;
            for (ic = 0; ic < 4; ic++) {
              if (r[ic] < 0)
                continue;
              if (nodaps(jd, ipl, fl | SEFLG_ICRS | CF[ic], M[im], pi, serr) < 0) {
                snprintf(what, sizeof what, "%s jd %.2f body %d method %d %s SEFLG_ICRS refused: %.100s",
                         EN[ie], jd, ipl, (int) M[im], CN[ic], serr);
                check(0, "b", what, 0, 0);
                continue;
              }
              for (k = 2; k < 4; k++) {
                double y[24];
                bias(pi + 6 * k, y + 6 * k);
                for (v = 0; v < 2; v++) {
                  double d = off(p[ic], y, k, v);
                  double lim = v ? 2e-11 * size(p[ic], k, 0) + 1e-13 : 1e-12 * size(p[ic], k, 0) + 1e-15;
                  if (d > wb[v])
                    wb[v] = d;
                  snprintf(what, sizeof what, "%s jd %.2f body %d method %d %s apsis %d %s, AU%s",
                           EN[ie], jd, ipl, (int) M[im], CN[ic], k,
                           v ? "velocity" : "position", v ? "/day" : "");
                  check(d <= lim, "b", what, d, lim);
                }
              }
            }
          }
  /* (c) where nothing is added or taken away, the points as with
   * SEFLG_ICRS, bit for bit: the Moon's mean geocentric points on every
   * ephemeris, Moshier's mean heliocentric ones */
  for (ie = 0; ie < 3; ie++)
    for (it = 0; it < (ie == 2 ? 3 : 4); it++)
      for (ib = 1; ib < (ie == 1 ? 8 : 2); ib++)
        for (im = 0; im < 4; im += 3)
          for (iff = 0; iff < 3; iff++) {
            static const int32 G[3] = {SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ, SEFLG_TRUEPOS, 0};
            double jd = ie == 2 ? JDJ[it] : JDF[it], a[24], b[24];
            int32 fl = E[ie] | SEFLG_SPEED | G[iff] | (B[ib] == SE_MOON ? 0 : SEFLG_HELCTR);
            int32 ra = nodaps(jd, B[ib], fl, M[im], a, serr), rb;
            rb = nodaps(jd, B[ib], fl | SEFLG_ICRS, M[im], b, serr);
            snprintf(what, sizeof what, "%s jd %.2f body %d method %d flags %d: ret %d %d", EN[ie], jd, B[ib],
                     (int) M[im], (int) fl, ra, rb);
            check(ra >= 0 && ra == rb && memcmp(a, b, sizeof a) == 0, "c", what, 0, 0);
            nc++;
          }
  printf("  (a) every centre from one centre's points and swe_calc()'s vectors: %.2e AU, %.2e AU/day"
         " (%.1e, %.1e of the sizes)\n", wa[0], wa[1], wq[0], wq[1]);
  printf("  (b) the osculating apsides the SEFLG_ICRS ones biased: %.2e AU, %.2e AU/day\n", wb[0], wb[1]);
  printf("  (c) the points where nothing is added as with SEFLG_ICRS: %d rows bit for bit\n", nc);
  printf("  rows %d, failed %d\n", nrow, nbad);
  printf(nbad ? "G163 FAIL: %d\n" : "G163 PASS\n", nbad);
  return nbad ? 1 : 0;
}
