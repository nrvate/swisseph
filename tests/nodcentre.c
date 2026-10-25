/* nodcentre.c -- G143: swe_nod_aps()'s change of centre is the one its
 * points are in.
 *
 * The defects. UPSTREAM-BUGS 167: the Sun's geocentric points are the
 * Earth's orbit seen from the Sun and negated, so swe_nod_aps() takes the
 * Sun for the observer: the barycentric Sun, the files' points being made
 * barycentric. On Moshier, whose points stay heliocentric, the test
 * against SEFLG_MOSEPH sent the Sun to the planets' branch and the observer
 * was the Earth: the mean perigee at 2000 Jul 1.5 281.4259 deg at 1.9993
 * AU, the apogee 191.4583 deg at 0.0531 AU, for the files' 282.9417 deg at
 * 0.9833 AU and 102.9417 deg at 1.0167 AU; every method, SEFLG_TOPOCTR the
 * site added (its speeds 1.3e-2 deg/day off their positions' rate).
 * UPSTREAM-BUGS 168: it read the ephemeris bits as the caller gave them,
 * not as swe_calc() resolves them (JPL, then Swiss, then Moshier):
 * SEFLG_SWIEPH | SEFLG_MOSEPH, answered from the files, took Moshier's
 * frame, Mars's geocentric node 69.7303 deg for 69.8382 and the
 * barycentric one the heliocentric; JPLEPH | MOSEPH on de200 turned the
 * osculating ellipse by the frame bias, which swi_get_denum() gives
 * Moshier (Mars's node 1.5e-4 deg off). (The same entry's fallback, the
 * frame and site of the ephemeris that answered, is G133's (f).)
 * UPSTREAM-BUGS 169: it read the centre bits otherwise than swe_calc()
 * (plaus_iflag(): SEFLG_TOPOCTR over SEFLG_BARYCTR over SEFLG_HELCTR):
 * SEFLG_HELCTR | SEFLG_BARYCTR answered heliocentric (Mars's node 49.5625
 * deg for the barycentric 49.6398), SEFLG_TOPOCTR | SEFLG_HELCTR
 * heliocentric, SEFLG_TOPOCTR | SEFLG_BARYCTR the barycentric points less
 * the site's offset (1.2e-3 deg off the barycentric node).
 *
 * The check, the complete directory:
 *  (a) the Swiss files and Moshier, seven instants -3000..3000: the Sun's
 *      geocentric and topocentric perigee and apogee against the Earth's
 *      heliocentric points of the same ephemeris and method (method 0,
 *      mean, osculating, SE_NODBIT_OSCU_BAR, mean and osculating with
 *      SE_NODBIT_FOPOINT): each the Earth's at its own distance (within
 *      1e-12 AU) and in the direction of the opposite one (the perigee the
 *      aphelion's, the apogee or focal point the perihelion's; within 1e-5
 *      deg, the deflection, which takes the distance, the difference);
 *      Moshier's mean points the files' within 1e-5 deg and 1e-12 AU (the
 *      elements are the same; the files' observer moves with the Sun's
 *      barycentric velocity); and Moshier's mean points' speeds,
 *      geocentric and topocentric, against five-point differences of
 *      their positions over 2^-6 day at binary instants, within 1e-10
 *      deg/day and 1e-12 AU/day;
 *  (b) SEFLG_SWIEPH | SEFLG_MOSEPH against SEFLG_SWIEPH, and SEFLG_JPLEPH
 *      with SEFLG_MOSEPH, SEFLG_SWIEPH or both against SEFLG_JPLEPH
 *      (de200): the Sun, the Moon, Mars, Saturn and the Earth, mean,
 *      osculating and SE_NODBIT_OSCU_BAR, geocentric, topocentric,
 *      heliocentric and barycentric, 2000 Jul 1.5 and 1900 Jan 0.75, the
 *      return and the four points bit for bit;
 *  (c) SEFLG_HELCTR | SEFLG_BARYCTR against SEFLG_BARYCTR, and
 *      SEFLG_TOPOCTR with SEFLG_BARYCTR, SEFLG_HELCTR or both against
 *      SEFLG_TOPOCTR: the Swiss files, Moshier and de200, (b)'s bodies,
 *      methods and instants, swe_nod_aps() and, for the Moon and Mars,
 *      mean and osculating, swe_nod_aps_ut(); the return, message and
 *      points bit for bit (Moshier refusing the barycentric points as
 *      SEFLG_BARYCTR's, and Saturn's SE_NODBIT_OSCU_BAR ellipse).
 * Measured: (a) distance 4.4e-16 AU, direction 7.6e-7 deg (the focal
 * point, 0.03 AU out), Moshier's mean points 2.9e-6 deg and 2.2e-16 AU off
 * the files', their speeds 2.9e-12 deg/day and 2.9e-14 AU/day; (b), (c)
 * 0. 1293 rows. Before bug 167's fix the 168 Moshier rows of (a)'s first
 * check fail (143 deg, 0.97 AU), the 14 of its second and 7 of the
 * third's 14 (the topocentric); before bug 168's, 340 of (b)'s 480 (12
 * deg); before bug 169's, 448 of (c)'s 456 (180 deg: the Sun's
 * topocentric points heliocentric, the Earth's orbit's). The upstream
 * build (SWE_UPSTREAM_COMPAT) fails 1025. Run as
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

/* (b) the ephemeris bits resolved as swe_calc() resolves them, bug 168:
 * SEFLG_SWIEPH | SEFLG_MOSEPH answers as SEFLG_SWIEPH, SEFLG_JPLEPH with
 * either or both as SEFLG_JPLEPH, return and points bit for bit */
static void part_b(void)
{
  static const int ipl[5] = {SE_SUN, SE_MOON, SE_MARS, SE_SATURN, SE_EARTH};
  static const int32 M[3] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR};
  static const int32 C[4] = {0, SEFLG_TOPOCTR, SEFLG_HELCTR, SEFLG_BARYCTR};
  static const int32 MIX[4][2] = {
    {SEFLG_SWIEPH | SEFLG_MOSEPH, SEFLG_SWIEPH}, {SEFLG_JPLEPH | SEFLG_MOSEPH, SEFLG_JPLEPH},
    {SEFLG_JPLEPH | SEFLG_SWIEPH, SEFLG_JPLEPH}, {SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH, SEFLG_JPLEPH}};
  static const double JD[2] = {2451727.0, 2415020.25};
  double a[24], b[24], w = 0;
  char sa[AS_MAXCH], sb[AS_MAXCH], what[160];
  int ib, im, ic, ix, id, ut;
  for (ut = 0; ut < 1; ut++)
    for (ix = 0; ix < 4; ix++)
      for (id = 0; id < 2; id++)
        for (ib = 0; ib < 5; ib++)
          for (im = 0; im < 3; im++)
            for (ic = 0; ic < 4; ic++) {
              int32 fa = MIX[ix][0] | SEFLG_SPEED | C[ic], fb = MIX[ix][1] | SEFLG_SPEED | C[ic], ra, rb;
              int k, same;
              double d = 0;
              if (ut && (ipl[ib] == SE_SUN || ipl[ib] == SE_SATURN || ipl[ib] == SE_EARTH || im == 2))
                continue;
              memset(a, 0, sizeof a);
              memset(b, 0, sizeof b);
              if (ut) {
                ra = swe_nod_aps_ut(JD[id], ipl[ib], fa, M[im], a, a + 6, a + 12, a + 18, sa);
                rb = swe_nod_aps_ut(JD[id], ipl[ib], fb, M[im], b, b + 6, b + 12, b + 18, sb);
              } else {
                ra = swe_nod_aps(JD[id], ipl[ib], fa, M[im], a, a + 6, a + 12, a + 18, sa);
                rb = swe_nod_aps(JD[id], ipl[ib], fb, M[im], b, b + 6, b + 12, b + 18, sb);
              }
              for (k = 0; k < 4; k++)
                if (dir_off(a, k, b, k) > d)
                  d = dir_off(a, k, b, k);
              if (d > w)
                w = d;
              same = ra == rb && memcmp(a, b, sizeof a) == 0;
              snprintf(what, sizeof what, "%s body %d method %d centre %d jd %.2f, flags %d against %d: ret %d %d, deg",
                       ut ? "swe_nod_aps_ut" : "swe_nod_aps", ipl[ib], (int) M[im], (int) C[ic], JD[id],
                       (int) MIX[ix][0], (int) MIX[ix][1], ra, rb);
              check(same && rb == 0, "b", what, d, 0);
            }
  printf("  (b) mixed ephemeris bits against the ephemeris that answers: %.2e deg\n", w);
}

/* (c) the centre bits read as swe_calc() reads them, bug 169: SEFLG_HELCTR
 * | SEFLG_BARYCTR answers as SEFLG_BARYCTR (on Moshier refused alike),
 * SEFLG_TOPOCTR with either or both as SEFLG_TOPOCTR, return, message and
 * points bit for bit */
static void part_c(void)
{
  static const int ipl[5] = {SE_SUN, SE_MOON, SE_MARS, SE_SATURN, SE_EARTH};
  static const int32 M[3] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR};
  static const int32 E[3] = {SEFLG_SWIEPH, SEFLG_MOSEPH, SEFLG_JPLEPH};
  static const int32 CC[4][2] = {
    {SEFLG_HELCTR | SEFLG_BARYCTR, SEFLG_BARYCTR}, {SEFLG_TOPOCTR | SEFLG_BARYCTR, SEFLG_TOPOCTR},
    {SEFLG_TOPOCTR | SEFLG_HELCTR, SEFLG_TOPOCTR},
    {SEFLG_TOPOCTR | SEFLG_HELCTR | SEFLG_BARYCTR, SEFLG_TOPOCTR}};
  static const double JD[2] = {2451727.0, 2415020.25};
  double a[24], b[24], w = 0;
  char sa[AS_MAXCH], sb[AS_MAXCH], what[160];
  int ib, im, ic, ie, id, ut, k;
  for (ut = 0; ut < 2; ut++)
    for (ie = 0; ie < 3; ie++)
      for (id = 0; id < 2; id++)
        for (ib = 0; ib < 5; ib++)
          for (im = 0; im < 3; im++)
            for (ic = 0; ic < 4; ic++) {
              int32 fa = E[ie] | SEFLG_SPEED | CC[ic][0], fb = E[ie] | SEFLG_SPEED | CC[ic][1], ra, rb;
              double d = 0;
              int same;
              if (ut && ((ipl[ib] != SE_MOON && ipl[ib] != SE_MARS) || im == 2))
                continue;
              memset(a, 0, sizeof a);
              memset(b, 0, sizeof b);
              *sa = *sb = '\0';
              if (ut) {
                ra = swe_nod_aps_ut(JD[id], ipl[ib], fa, M[im], a, a + 6, a + 12, a + 18, sa);
                rb = swe_nod_aps_ut(JD[id], ipl[ib], fb, M[im], b, b + 6, b + 12, b + 18, sb);
              } else {
                ra = swe_nod_aps(JD[id], ipl[ib], fa, M[im], a, a + 6, a + 12, a + 18, sa);
                rb = swe_nod_aps(JD[id], ipl[ib], fb, M[im], b, b + 6, b + 12, b + 18, sb);
              }
              for (k = 0; k < 4; k++)
                if (dir_off(a, k, b, k) > d)
                  d = dir_off(a, k, b, k);
              if (d > w)
                w = d;
              same = ra == rb && memcmp(a, b, sizeof a) == 0 && (rb == 0 || strcmp(sa, sb) == 0);
              snprintf(what, sizeof what, "%s eph %d body %d method %d jd %.2f, centre %d against %d: ret %d %d, deg",
                       ut ? "swe_nod_aps_ut" : "swe_nod_aps", (int) E[ie], ipl[ib], (int) M[im], JD[id],
                       (int) CC[ic][0], (int) CC[ic][1], ra, rb);
              /* Moshier refuses the barycentric points (bug 149), and
               * every SE_NODBIT_OSCU_BAR ellipse beyond 6 AU (Saturn's) */
              check(same && (rb == 0 || (E[ie] == SEFLG_MOSEPH
                                         && strcmp(sb, "barycentric Moshier positions are not supported.") == 0)),
                    "c", what, d, 0);
            }
  printf("  (c) combined centre bits against the centre swe_calc() takes: %.2e deg\n", w);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: nodcentre EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  swe_set_jpl_file("de200.eph");
  printf("G143: swe_nod_aps()'s change of centre is the one its points are in\n");
  part_a();
  part_b();
  part_c();
  printf("  rows %d, failed %d\n", nrow, nbad);
  printf(nbad ? "G143 FAIL: %d\n" : "G143 PASS\n", nbad);
  return nbad ? 1 : 0;
}
