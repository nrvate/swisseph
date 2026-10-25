/* nodapshel.c -- G146: swe_nod_aps()'s heliocentric and barycentric points
 * take no aberration and no light deflection, as swe_calc()'s heliocentric
 * and barycentric places take none.
 *
 * The defect (UPSTREAM-BUGS 178). plaus_iflag() sets SEFLG_NOABERR |
 * SEFLG_NOGDEFL for heliocentric and barycentric output; swe_nod_aps() did
 * not. Its heliocentric points took the aberration of the Sun's
 * barycentric velocity (3.0e-6 deg on Mars's), and its heliocentric and
 * barycentric points swi_deflect_light()'s deflection, whose observer is
 * the Earth whatever the centre: a vector from the Sun or the barycentre
 * deflected as if the Earth looked along it. Jupiter's mean descending
 * node at J2000, at 280.46 deg with the Sun at 280.37 from the Earth, was
 * 9.6e-4 deg off its SEFLG_NOABERR | SEFLG_NOGDEFL place, its barycentric
 * speeds 3.4e-2 deg/day; Mars's heliocentric points 4.1e-6 deg, Neptune's
 * 5.8e-6, the Sun's and the Earth's 4.0e-6.
 *
 * The check, the complete directory:
 *  (a) the Swiss files, de200 and Moshier, heliocentric and barycentric
 *      (Moshier refusing the barycentric points and the barycentric
 *      ellipses beyond 6 AU either way): the Sun, the
 *      Moon, Mercury to Pluto, the Earth and Chiron, mean, osculating,
 *      SE_NODBIT_OSCU_BAR and osculating with SE_NODBIT_FOPOINT, at 2000
 *      Jan 1.5, 2000 Jul 1.5, 1900 Jan 0.75 and 2050 Jan 1; the default
 *      flags with and without SEFLG_SPEED, and SEFLG_TRUEPOS with it,
 *      against the same with SEFLG_NOABERR | SEFLG_NOGDEFL: the return and
 *      the four points bit for bit;
 *  (b) swe_nod_aps_ut(), the Moon and Mars, mean and osculating,
 *      heliocentric and barycentric on the files: the same, bit for bit;
 *  (c) the geocentric and topocentric points still apparent: Mars's and
 *      Jupiter's, mean and osculating, the default flags against
 *      SEFLG_NOABERR | SEFLG_NOGDEFL more than 1e-3 deg apart (the
 *      aberration, 20") on one point at least.
 * Measured: (a), (b) 0; (c) 4.8e-3 deg at least. 3520 rows. Before the
 * fix 1808 fail, 1784 of (a) (9.64e-4 deg) and 24 of (b) (4.1e-6); the
 * upstream build (SWE_UPSTREAM_COMPAT) 2120. Run as
 *   nodapshel EPHEDIR
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

/* the largest direction difference of the four points, deg */
static double pts_off(const double *a, const double *b)
{
  double w = 0;
  int k;
  for (k = 0; k < 4; k++) {
    double d = fabs(swe_difdeg2n(a[6 * k], b[6 * k])) + fabs(a[6 * k + 1] - b[6 * k + 1]);
    if (d > w)
      w = d;
  }
  return w;
}

static int32 nodaps(int ut, double jd, int ipl, int32 fl, int32 meth, double x[24])
{
  char serr[AS_MAXCH];
  memset(x, 0, 24 * sizeof(double));
  if (ut)
    return swe_nod_aps_ut(jd, ipl, fl, meth, x, x + 6, x + 12, x + 18, serr);
  return swe_nod_aps(jd, ipl, fl, meth, x, x + 6, x + 12, x + 18, serr);
}

static const int32 M[4] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR,
                           SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
static const int32 C[2] = {SEFLG_HELCTR, SEFLG_BARYCTR};
static const double JD[4] = {2451545.0, 2451727.0, 2415020.25, 2469807.5};

/* (a) heliocentric and barycentric points as with SEFLG_NOABERR |
 * SEFLG_NOGDEFL, bit for bit */
static double part_a(void)
{
  static const int ipl[12] = {SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER,
                              SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO, SE_EARTH, SE_CHIRON};
  static const int32 E[3] = {SEFLG_SWIEPH, SEFLG_JPLEPH, SEFLG_MOSEPH};
  static const int32 F[3] = {SEFLG_SPEED, 0, SEFLG_SPEED | SEFLG_TRUEPOS};
  double a[24], b[24], w = 0;
  char what[160];
  int ie, ic, ib, im, id, iff;
  for (ie = 0; ie < 3; ie++)
    for (ic = 0; ic < 2; ic++)
      for (ib = 0; ib < 12; ib++)
        for (im = 0; im < 4; im++)
          for (id = 0; id < 4; id++)
            for (iff = 0; iff < 3; iff++) {
              int32 fa = E[ie] | C[ic] | F[iff], ra, rb;
              double d;
              ra = nodaps(0, JD[id], ipl[ib], fa, M[im], a);
              rb = nodaps(0, JD[id], ipl[ib], fa | SEFLG_NOABERR | SEFLG_NOGDEFL, M[im], b);
              d = pts_off(a, b);
              if (d > w)
                w = d;
              snprintf(what, sizeof what, "eph %d centre %d body %d method %d jd %.2f flags %d: ret %d %d, deg",
                       (int) E[ie], (int) C[ic], ipl[ib], (int) M[im], JD[id], (int) F[iff], ra, rb);
              /* Moshier refuses the barycentric points and the
               * barycentric ellipses beyond 6 AU (bug 149) */
              check(ra == rb && memcmp(a, b, sizeof a) == 0
                    && (ra >= 0 || (E[ie] == SEFLG_MOSEPH
                                    && (C[ic] == SEFLG_BARYCTR || M[im] == SE_NODBIT_OSCU_BAR))),
                    "a", what, d, 0);
            }
  return w;
}

/* (b) the UT form alike */
static double part_b(void)
{
  static const int ipl[2] = {SE_MOON, SE_MARS};
  double a[24], b[24], w = 0;
  char what[160];
  int ic, ib, im, id;
  for (ic = 0; ic < 2; ic++)
    for (ib = 0; ib < 2; ib++)
      for (im = 0; im < 2; im++)
        for (id = 0; id < 4; id++) {
          int32 fa = SEFLG_SWIEPH | SEFLG_SPEED | C[ic], ra, rb;
          double d;
          ra = nodaps(1, JD[id], ipl[ib], fa, M[im], a);
          rb = nodaps(1, JD[id], ipl[ib], fa | SEFLG_NOABERR | SEFLG_NOGDEFL, M[im], b);
          d = pts_off(a, b);
          if (d > w)
            w = d;
          snprintf(what, sizeof what, "swe_nod_aps_ut centre %d body %d method %d jd %.2f: ret %d %d, deg",
                   (int) C[ic], ipl[ib], (int) M[im], JD[id], ra, rb);
          check(ra == rb && ra >= 0 && memcmp(a, b, sizeof a) == 0, "b", what, d, 0);
        }
  return w;
}

/* (c) the geocentric and topocentric points keep their aberration */
static double part_c(void)
{
  static const int ipl[2] = {SE_MARS, SE_JUPITER};
  static const int32 G[2] = {0, SEFLG_TOPOCTR};
  double a[24], b[24], w = 1e9;
  char what[160];
  int ic, ib, im, id;
  for (ic = 0; ic < 2; ic++)
    for (ib = 0; ib < 2; ib++)
      for (im = 0; im < 2; im++)
        for (id = 0; id < 4; id++) {
          int32 fa = SEFLG_SWIEPH | SEFLG_SPEED | G[ic], ra, rb;
          double d;
          ra = nodaps(0, JD[id], ipl[ib], fa, M[im], a);
          rb = nodaps(0, JD[id], ipl[ib], fa | SEFLG_NOABERR | SEFLG_NOGDEFL, M[im], b);
          d = pts_off(a, b);
          if (d < w)
            w = d;
          snprintf(what, sizeof what, "centre %d body %d method %d jd %.2f, apparent against astrometric: ret %d %d, deg",
                   (int) G[ic], ipl[ib], (int) M[im], JD[id], ra, rb);
          check(ra >= 0 && rb >= 0 && d > 1e-3, "c", what, d, 1e-3);
        }
  return w;
}

int main(int argc, char **argv)
{
  double wa, wb, wc;
  if (argc != 2) {
    fprintf(stderr, "usage: nodapshel EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  swe_set_jpl_file("de200.eph");
  printf("G146: swe_nod_aps()'s heliocentric and barycentric points take no aberration or deflection\n");
  wa = part_a();
  wb = part_b();
  wc = part_c();
  printf("  (a) heliocentric and barycentric against SEFLG_NOABERR | SEFLG_NOGDEFL: %.2e deg\n", wa);
  printf("  (b) swe_nod_aps_ut() alike: %.2e deg\n", wb);
  printf("  (c) geocentric and topocentric apparent against astrometric: %.2e deg at least\n", wc);
  printf("  rows %d, failed %d\n", nrow, nbad);
  printf(nbad ? "G146 FAIL: %d\n" : "G146 PASS\n", nbad);
  return nbad ? 1 : 0;
}
