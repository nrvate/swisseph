/* nodcentre.c -- G143: swe_nod_aps()'s change of centre is the one its
 * points are in.
 *
 * The defect. UPSTREAM-BUGS 167: the Sun's geocentric points are the
 * Earth's orbit seen from the Sun and negated, so swe_nod_aps() takes the
 * Sun for the observer: the barycentric Sun, the files' points being made
 * barycentric. On Moshier, whose points stay heliocentric, the test
 * against SEFLG_MOSEPH sent the Sun to the planets' branch and the observer
 * was the Earth: the mean perigee at 2000 Jul 1.5 281.4259 deg at 1.9993
 * AU, the apogee 191.4583 deg at 0.0531 AU, for the files' 282.9417 deg at
 * 0.9833 AU and 102.9417 deg at 1.0167 AU; every method, SEFLG_TOPOCTR the
 * site added (its speeds 1.3e-2 deg/day off their positions' rate).
 *
 * The check, the Swiss files and Moshier, seven instants -3000..3000:
 *  (a) the Sun's geocentric and topocentric perigee and apogee against the
 *      Earth's heliocentric points of the same ephemeris and method
 *      (method 0, mean, osculating, SE_NODBIT_OSCU_BAR, mean and
 *      osculating with SE_NODBIT_FOPOINT): each the Earth's at its own
 *      distance (within 1e-12 AU) and in the direction of the opposite one
 *      (the perigee the aphelion's, the apogee or focal point the
 *      perihelion's; within 1e-5 deg, the deflection, which takes the
 *      distance, the difference); Moshier's mean points the files' within
 *      1e-5 deg and 1e-12 AU (the elements are the same; the files'
 *      observer moves with the Sun's barycentric velocity); and Moshier's
 *      mean points' speeds, geocentric and topocentric, against five-point
 *      differences of their positions over 2^-6 day at binary instants,
 *      within 1e-10 deg/day and 1e-12 AU/day.
 * Measured: distance 4.4e-16 AU, direction 7.6e-7 deg (the focal point,
 * 0.03 AU out), Moshier's mean points 2.9e-6 deg and 2.2e-16 AU off the
 * files', their speeds 2.9e-12 deg/day and 2.9e-14 AU/day. Before the fix
 * the 168 Moshier rows of (a)'s first check fail (143 deg, 0.97 AU), the 14
 * of its second and 7 of the third's 14 (the topocentric). The upstream
 * build fails the same. Run as
 *   nodcentre EPHEDIR
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

static int32 nodaps(double jd, int ipl, int32 fl, int32 meth, double x[24])
{
  char serr[AS_MAXCH];
  memset(x, 0, 24 * sizeof(double));
  return swe_nod_aps(jd, ipl, fl, meth, x, x + 6, x + 12, x + 18, serr);
}

/* the direction of point k of a against point j of b, deg */
static double dir_off(const double *a, int k, const double *b, int j)
{
  return fabs(swe_difdeg2n(a[6 * k], b[6 * j])) + fabs(a[6 * k + 1] - b[6 * j + 1]);
}

static double jd_of(int y)
{
  return floor((2451545.0 + (y - 2000) * 365.25 + 100.37) * 256) / 256;
}

/* (a) the Sun's geocentric points, bug 167 */
static void part_a(void)
{
  static const int32 M[6] = {0, SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR,
                             SE_NODBIT_MEAN | SE_NODBIT_FOPOINT, SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
  static const int32 E[2] = {SEFLG_MOSEPH, SEFLG_SWIEPH};
  const double H = 1.0 / 64;
  double wd = 0, wr = 0, wm = 0, wmr = 0, ws = 0, wsr = 0;
  char w[160];
  int ie, it, im, y, k, j, c;
  for (ie = 0; ie < 2; ie++)
    for (it = 0; it < 2; it++)
      for (im = 0; im < 6; im++)
        for (y = -3000; y <= 3000; y += 1000) {
          double s[24], e[24], jd = jd_of(y);
          int32 fl = E[ie] | SEFLG_SPEED;
          int32 rs = nodaps(jd, SE_SUN, fl | (it ? SEFLG_TOPOCTR : 0), M[im], s);
          int32 re = nodaps(jd, SE_EARTH, fl | SEFLG_HELCTR, M[im], e);
          for (k = 2; k < 4; k++) {
            double d = dir_off(s, k, e, 5 - k), r = fabs(s[6 * k + 2] - e[6 * k + 2]);
            if (d > wd) wd = d;
            if (r > wr) wr = r;
            snprintf(w, sizeof w, "%s %s method %d %d, the Sun's %s against the Earth's: dist %.2e AU, dir",
                     E[ie] == SEFLG_MOSEPH ? "Moshier" : "Swiss", it ? "topo" : "geo", (int) M[im], y,
                     k == 2 ? "perigee" : "apogee", r);
            check(rs == 0 && re == 0 && r <= 1e-12 && d <= 1e-5, "a", w, d, 1e-5);
          }
        }
  /* Moshier's mean points the files' */
  for (y = -3000; y <= 3000; y += 1000) {
    double m[24], f[24], jd = jd_of(y);
    int32 rm = nodaps(jd, SE_SUN, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN, m);
    int32 rf = nodaps(jd, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN, f);
    double d = 0, r = 0;
    for (k = 2; k < 4; k++) {
      if (dir_off(m, k, f, k) > d) d = dir_off(m, k, f, k);
      if (fabs(m[6 * k + 2] - f[6 * k + 2]) > r) r = fabs(m[6 * k + 2] - f[6 * k + 2]);
    }
    if (d > wm) wm = d;
    if (r > wmr) wmr = r;
    snprintf(w, sizeof w, "Moshier %d, the Sun's mean points against the files': dist %.2e AU, dir", y, r);
    check(rm == 0 && rf == 0 && r <= 1e-12 && d <= 1e-5, "a", w, d, 1e-5);
  }
  /* Moshier's mean points' speeds, the rates of their positions */
  for (it = 0; it < 2; it++)
    for (y = -3000; y <= 3000; y += 1000) {
      double x[24], f[5][24], jd = jd_of(y), sa = 0, sr = 0;
      int32 fl = SEFLG_MOSEPH | (it ? SEFLG_TOPOCTR : 0);
      int32 r = nodaps(jd, SE_SUN, fl | SEFLG_SPEED, SE_NODBIT_MEAN, x);
      for (j = 0; j < 5; j++)
        nodaps(jd + (j - 2) * H, SE_SUN, fl, SE_NODBIT_MEAN, f[j]);
      for (k = 2; k < 4; k++)
        for (c = 0; c < 3; c++) {
          double d1 = c ? f[3][6 * k + c] - f[1][6 * k + c] : swe_difdeg2n(f[3][6 * k], f[1][6 * k]);
          double d2 = c ? f[4][6 * k + c] - f[0][6 * k + c] : swe_difdeg2n(f[4][6 * k], f[0][6 * k]);
          double e = fabs(x[6 * k + 3 + c] - (8 * d1 - d2) / (12 * H));
          if (c < 2 && e > sa) sa = e;
          if (c == 2 && e > sr) sr = e;
        }
      if (sa > ws) ws = sa;
      if (sr > wsr) wsr = sr;
      snprintf(w, sizeof w, "Moshier %s %d, the Sun's mean points' speeds: dist %.2e AU/day, deg/day",
               it ? "topo" : "geo", y, sr);
      check(r == 0 && sr <= 1e-12 && sa <= 1e-10, "a", w, sa, 1e-10);
    }
  printf("  (a) the Sun's points against the Earth's: %.2e AU, %.2e deg; Moshier's mean against the\n"
         "      files' %.2e deg, %.2e AU; their speeds %.2e deg/day, %.2e AU/day\n",
         wr, wd, wm, wmr, ws, wsr);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: nodcentre EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  printf("G143: swe_nod_aps()'s change of centre is the one its points are in\n");
  part_a();
  printf("  rows %d, failed %d\n", nrow, nbad);
  printf(nbad ? "G143 FAIL: %d\n" : "G143 PASS\n", nbad);
  return nbad ? 1 : 0;
}
