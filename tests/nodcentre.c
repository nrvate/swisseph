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
 * UPSTREAM-BUGS 176: a request fallen to Moshier had its osculating ellipse
 * turned by the frame bias of the DE number asked, swi_get_denum() of the
 * caller's bit: SEFLG_JPLEPH on de200 outside its years left Moshier's
 * state in DE200's frame, unbiased, Mars's node at 1500 Jan 1 1.58e-4 deg
 * off the Moshier request's. UPSTREAM-BUGS 180: on Moshier the Earth-Moon
 * barycentre of the Earth's and the Sun's ellipses was Moshier's Earth
 * plus Moshier's Moon, where the Earth is made from the EMB with the short
 * lunar series' Moon (0.24 deg away at 2000): the EMB 1.3e-7 AU off, the
 * perihelion wobbling 8e-3 deg with the Moon's inequalities, its speed
 * 4.7e-8 deg/day off its positions' rate, the distance's 2.5e-11 AU/day.
 * UPSTREAM-BUGS 170: swe_nod_aps_ut() took delta t from the bits as given,
 * where swe_calc_ut() resolves them first: swe_deltat_ex() takes the Swiss
 * files' tidal term for SEFLG_JPLEPH | SEFLG_SWIEPH, DE431's for de200's
 * DE200 (the Moon's topocentric osculating points 3.6e-5 deg off the
 * SEFLG_JPLEPH request's at 1900). (Its fallback, delta t not redone for
 * the ephemeris that answered, is G133's (h).)
 * UPSTREAM-BUGS 174: the Moon's mean points geocentric and topocentric
 * read the Moon for the save area, which nothing of them uses: refused
 * under the strict setting without the moon files, in an empty directory,
 * JPL without its file, where swe_calc() answers SE_MEAN_NODE; with the
 * fallback flagged by the ephemeris the read fell to, so swe_nod_aps_ut()'s
 * delta t taken again for it (bug 170) moved them (6.0e-8 deg at 1900,
 * 1.8e-5 in AD 1000), and the site took Moshier's delta t.
 * UPSTREAM-BUGS 179: a refusal by a calculation inside (the strict
 * setting's, a file's, a range's) returned -1 with the four points as the
 * caller had them, where swe_calc() zeroes a refused place and this
 * function an unimplemented body's and a barycentric Moshier request's.
 * UPSTREAM-BUGS 192: the Sun's points and the Earth's heliocentric and
 * barycentric ones, which the Moon does not reach (the elements with the
 * Sun, the ellipse fitted to the Earth-Moon barycentre), were refused under
 * the strict setting without the moon files, their reads of the Earth and
 * the Moon refused by bug 120; with the fallback 6.5e-15 deg off the
 * complete directory's. UPSTREAM-BUGS 193: and the planets' points
 * geocentric and topocentric were answered there from the Earth made from
 * Moshier's Moon, their read being heliocentric, where swe_calc() of the
 * planet refuses (Mars's at 1900 3.7e-9 deg off the complete directory's).
 * UPSTREAM-BUGS 190: the Sun's topocentric points were its geocentric
 * ones: the site, set as the observer, was overwritten by the Sun, so they
 * carried no parallax (the perigee 2.7e-3 deg and 2.7e-5 AU off its
 * topocentric place here, the focal point 8.7e-2 deg), where swe_calc()'s
 * topocentric Sun at J2000 is 2.3e-3 deg from its geocentric one.
 *
 * The check, the complete directory, and for (f) to (i) others:
 *  (a) the Swiss files and Moshier, seven instants -3000..3000: the Sun's
 *      geocentric and topocentric perigee and apogee against the Earth's
 *      heliocentric points of the same ephemeris and method (method 0,
 *      mean, osculating, SE_NODBIT_OSCU_BAR, mean and osculating with
 *      SE_NODBIT_FOPOINT): each the Earth's same point negated (in the
 *      direction of the opposite one: the perigee the aphelion's, the
 *      apogee or focal point the perihelion's), less the site for the
 *      topocentric ones (the Sun's geometric geocentric less topocentric
 *      place, from swe_calc() on a context of its own), its distance
 *      within 1e-12 AU (topocentric 1e-11: the site was subtracted in the
 *      ephemeris's frame from points in J2000's, as every body's, 1.8e-12
 *      AU, until UPSTREAM-BUGS 221) and its direction within 1e-5 deg (the deflection, which takes
 *      the distance, the difference); Moshier's mean points the files'
 *      within 1e-5 deg and 1e-12 AU (the elements are the same; the files'
 *      observer moves with the Sun's barycentric velocity); and Moshier's
 *      mean points' speeds, geocentric and topocentric, against five-point
 *      differences of their positions over 2^-6 day at binary instants
 *      (topocentric nine-point: the five-point's truncation of the site's
 *      daily turn is 3.2e-8 deg/day), within 1e-10 deg/day and 1e-12
 *      AU/day;
 *  (b) SEFLG_SWIEPH | SEFLG_MOSEPH against SEFLG_SWIEPH, and SEFLG_JPLEPH
 *      with SEFLG_MOSEPH, SEFLG_SWIEPH or both against SEFLG_JPLEPH
 *      (de200): the Sun, the Moon, Mars, Saturn and the Earth, mean,
 *      osculating and SE_NODBIT_OSCU_BAR, geocentric, topocentric,
 *      heliocentric and barycentric, 2000 Jul 1.5 and 1900 Jan 0.75, the
 *      return and the four points bit for bit; and swe_nod_aps_ut() of the
 *      Moon and Mars, mean and osculating, so (UPSTREAM-BUGS 170);
 *  (c) SEFLG_HELCTR | SEFLG_BARYCTR against SEFLG_BARYCTR, and
 *      SEFLG_TOPOCTR with SEFLG_BARYCTR, SEFLG_HELCTR or both against
 *      SEFLG_TOPOCTR: the Swiss files, Moshier and de200, (b)'s bodies,
 *      methods and instants, swe_nod_aps() and, for the Moon and Mars,
 *      mean and osculating, swe_nod_aps_ut(); the return, message and
 *      points bit for bit (Moshier refusing the barycentric points as
 *      SEFLG_BARYCTR's, and Saturn's SE_NODBIT_OSCU_BAR ellipse);
 *  (d) SEFLG_JPLEPH on de200, the fallback allowed, against SEFLG_MOSEPH
 *      at 1500 Jan 1 and 2200 Jan 1, outside de200's years: (b)'s bodies,
 *      mean, osculating, SE_NODBIT_OSCU_BAR and osculating with
 *      SE_NODBIT_FOPOINT, the four centres, each pair from a closed
 *      context; the return and points bit for bit (both refusing the
 *      barycentric points and Saturn's barycentric ellipse); the Moon's
 *      mean points geocentric and topocentric, which read no file (bug
 *      174), against SEFLG_JPLEPH strict;
 *  (e) Moshier and the files, 13 instants -3000..3000: the osculating
 *      apsides (and the focal point) of the Earth, heliocentric, and of
 *      the Sun, geocentric and topocentric, their speeds against
 *      five-point differences of their positions over 2^-6 day at binary
 *      instants (topocentric nine-point, as (a): the focal point, 0.03 AU
 *      out, turns with the site by 0.07 deg, and the five-point's
 *      truncation there is 9.0e-7 deg/day), within 2e-9 deg/day (the
 *      files' topocentric Sun 1e-8, at the residual floor: 3.9e-9 there,
 *      not this check's) and 1e-12 AU/day;
 *  (f) the Moon's mean points (SE_NODBIT_MEAN, 0, SE_NODBIT_FOPOINT)
 *      along directories of links to EPHEDIR's files but the moon files,
 *      but the JPL file, and none, and the complete one; Swiss, JPL
 *      (de200) and Moshier; strict and with the fallback; 1900 Jan 0.75
 *      and AD 1000; each directory, ephemeris, setting and instant on a
 *      context of its own: the geocentric and J2000 points answered bit
 *      for bit as the complete directory's; swe_nod_aps_ut()'s node within
 *      1e-10 deg of swe_calc_ut(SE_MEAN_NODE)'s there, which returns the
 *      ephemeris asked (its delta t); with the fallback the strict answer
 *      bit for bit, topocentric and UT too; the heliocentric and
 *      barycentric points without the moon files, which read the Earth
 *      made from Moshier's Moon, refused strict as bug 120 refuses it and
 *      with the fallback the complete directory's within 1e-7 deg; and
 *      about the ends of the mean node's range refused where swe_calc() is,
 *      with its message, and answered a day inside them;
 *  (g) (f)'s directories, the outputs filled with 1234.5: Mars's
 *      osculating and mean points, Swiss and JPL, the Moon's osculating
 *      and heliocentric mean points without the moon files, under the
 *      strict setting; the Moon's mean points beyond the mean node's
 *      range; swe_nod_aps_ut() of two of them; the unimplemented mean node
 *      and Mars barycentric on Moshier, zeroed before: refused (-1), the
 *      24 values zeroed, and with the apsides' pointers NULL the nodes'
 *      12 zeroed and nothing written past them;
 *  (h) (f)'s directory without the moon files and the complete one, AD
 *      1000, 1900 and 2000, swe_nod_aps() and swe_nod_aps_ut(): the Sun's
 *      points geocentric, topocentric and heliocentric and the Earth's
 *      heliocentric and barycentric, method 0, mean, osculating,
 *      SE_NODBIT_OSCU_BAR and osculating with SE_NODBIT_FOPOINT, answered
 *      under the strict setting, the fallback's answer bit for bit, the
 *      complete directory's within 1e-12 deg and AU (the speeds 1e-10
 *      deg/day, the barycentres' rounding over 2^-6 day); after the Sun's
 *      osculating points the Earth of their reads at the instant and 2^-6
 *      day after, with the flags those answers carry, refused, and the
 *      Earth's geocentric points refused, with bug 120's message;
 *  (i) (h)'s directories, 1900 and 2000, swe_nod_aps() and
 *      swe_nod_aps_ut(): Mercury's, Mars's, Jupiter's and Pluto's points,
 *      mean, osculating, SE_NODBIT_OSCU_BAR and osculating with
 *      SE_NODBIT_FOPOINT, the four centres, the outputs filled with
 *      1234.5: geocentric and topocentric refused strict without the moon
 *      files, with bug 120's message, as swe_calc() of the planet is, the
 *      24 values zeroed, with the fallback within 1e-7 deg of the complete
 *      directory's; heliocentric and barycentric answered strict, bit for
 *      bit the complete directory's and the fallback's.
 * Measured: (a) distance 4.4e-16 AU (topocentric 5.6e-16; 1.8e-12 before
 * bug 221's fix), direction
 * 2.9e-6 deg (the focal point, 0.03 AU out), Moshier's mean points 2.9e-6
 * deg and 2.2e-16 AU off the files', their speeds 3.6e-12 deg/day and
 * 3.7e-14 AU/day; (b), (c), (d) 0; (e) 3.7e-10 deg/day (the files'
 * topocentric Sun 3.9e-9), 1.7e-13 AU/day; (f) the node 1.1e-13 deg off
 * the mean node's, the heliocentric and barycentric points 1.6e-8 deg off;
 * (g) 0; (h) 6.5e-15 deg, the speeds 1.5e-11 deg/day; (i) the fallback 8.7e-9
 * deg. 2558 rows. Before bug 167's fix the 168 Moshier rows of (a)'s
 * first check fail (143 deg, 0.97 AU), the 14 of its second and 7 of the
 * third's 14 (the topocentric); before bug 168's, 340 of (b)'s 480 (12
 * deg); before bug 169's, 448 of (c)'s 456 (180 deg: the Sun's
 * topocentric points heliocentric, the Earth's orbit's); before bug 176's,
 * 84 of (d)'s 160, every osculating row that answers (1.75e-4 deg); before
 * bug 180's, (e)'s 78 Moshier rows (4.7e-8 deg/day, 2.5e-11 AU/day);
 * before bug 170's, the 32 UT rows of (b) at 1900 with SEFLG_JPLEPH |
 * SEFLG_SWIEPH, alone or with SEFLG_MOSEPH (3.6e-5 deg); before bug 174's,
 * 114 of (f)'s 390 (144 with bug 170's fix and not 174's) and (d)'s 4 rows
 * of the Moon's mean points (refused strict); before bug 179's, 18 of
 * (g)'s 22 (all but the two zeroed already); before bug 192's, all 153 of
 * (h)'s (the strict refusals); before bug 193's, the 128 geocentric and
 * topocentric rows of (i)'s 256 (answered strict); before bug 190's,
 * (a)'s 168 topocentric rows of its first check (3.6e-5 AU, 8.7e-2 deg;
 * that check took them for the geocentric ones before). The upstream
 * build (SWE_UPSTREAM_COMPAT) fails 1905. Run as
 *   nodcentre EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

/* polar (deg, deg, AU) to cartesian and back */
static void pol2cart(const double *p, double *c)
{
  double l = p[0] * DEGTORAD, b = p[1] * DEGTORAD;
  c[0] = p[2] * cos(b) * cos(l);
  c[1] = p[2] * cos(b) * sin(l);
  c[2] = p[2] * sin(b);
}

static void cart2pol(const double *c, double *p)
{
  p[0] = swe_degnorm(atan2(c[1], c[0]) * RADTODEG);
  p[1] = atan2(c[2], sqrt(c[0] * c[0] + c[1] * c[1])) * RADTODEG;
  p[2] = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
}

/* the topocentric site, the Sun's geometric geocentric less topocentric
 * place, ecliptic of date, AU (bug 190), on a context of its own: a
 * topocentric swe_calc() Sun at an instant moves the default context's
 * topocentric Sun points there by 6e-12 deg, their speeds by 4e-11 deg/day
 * (the build before bug 190 too; below the floor, not this check's) */
static swe_ctx *site_ctx;

static void site_of(double jd, int32 eph, double *site)
{
  char serr[AS_MAXCH];
  double g[6], t[6];
  int32 fl = eph | SEFLG_XYZ | SEFLG_TRUEPOS | SEFLG_NOABERR | SEFLG_NOGDEFL;
  swe_calc_r(site_ctx, jd, SE_SUN, fl, g, serr);
  swe_calc_r(site_ctx, jd, SE_SUN, fl | SEFLG_TOPOCTR, t, serr);
  site[0] = g[0] - t[0];
  site[1] = g[1] - t[1];
  site[2] = g[2] - t[2];
}

/* the rate of component c (0 longitude, 1 latitude, 2 distance) of point
 * k from its places f[j] at jd + (j - n / 2) H: the five-point central
 * difference (n 5) or, for a topocentric point, whose site turns in a day,
 * the nine-point one (n 9; the five-point's truncation is 3.2e-8 deg/day
 * there over 2^-6 day, the focal point's, 0.03 AU out, 9.0e-7) */
static double rate_of(double f[][24], int n, int k, int c, double H)
{
  static const double c9[4] = {4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280};
  int m = n / 2, q;
  double d = 0;
  if (n == 5) {
    double d1 = c ? f[3][6 * k + c] - f[1][6 * k + c] : swe_difdeg2n(f[3][6 * k], f[1][6 * k]);
    double d2 = c ? f[4][6 * k + c] - f[0][6 * k + c] : swe_difdeg2n(f[4][6 * k], f[0][6 * k]);
    return (8 * d1 - d2) / (12 * H);
  }
  for (q = 1; q <= 4; q++)
    d += c9[q - 1] * (c ? f[m + q][6 * k + c] - f[m - q][6 * k + c]
                      : swe_difdeg2n(f[m + q][6 * k], f[m - q][6 * k]));
  return d / H;
}

/* (a) the Sun's geocentric points, bug 167 */
static void part_a(void)
{
  static const int32 M[6] = {0, SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR,
                             SE_NODBIT_MEAN | SE_NODBIT_FOPOINT, SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
  static const int32 E[2] = {SEFLG_MOSEPH, SEFLG_SWIEPH};
  const double H = 1.0 / 64;
  double wd = 0, wr = 0, wrt = 0, wm = 0, wmr = 0, ws = 0, wsr = 0;
  char w[160];
  int ie, it, im, y, k, j, c;
  for (ie = 0; ie < 2; ie++)
    for (it = 0; it < 2; it++)
      for (im = 0; im < 6; im++)
        for (y = -3000; y <= 3000; y += 1000) {
          double s[24], e[24], jd = jd_of(y);
          int32 fl = E[ie] | SEFLG_SPEED;
          double site[3] = {0, 0, 0};
          int32 rs = nodaps(jd, SE_SUN, fl | (it ? SEFLG_TOPOCTR : 0), M[im], s);
          int32 re = nodaps(jd, SE_EARTH, fl | SEFLG_HELCTR, M[im], e);
          if (it)
            site_of(jd, E[ie], site);
          for (k = 2; k < 4; k++) {
            /* the Earth's same point negated (the direction of the
             * opposite one), less the site (bug 190) */
            double xe[3], ref[3], d, r;
            pol2cart(e + 6 * k, xe);
            for (c = 0; c < 3; c++)
              xe[c] = -xe[c] - site[c];
            cart2pol(xe, ref);
            d = dir_off(s, k, ref, 0);
            r = fabs(s[6 * k + 2] - ref[2]);
            if (d > wd) wd = d;
            if (it) {
              if (r > wrt) wrt = r;
            } else if (r > wr) {
              wr = r;
            }
            snprintf(w, sizeof w, "%s %s method %d %d, the Sun's %s against the Earth's: dist %.2e AU, dir",
                     E[ie] == SEFLG_MOSEPH ? "Moshier" : "Swiss", it ? "topo" : "geo", (int) M[im], y,
                     k == 2 ? "perigee" : "apogee", r);
            check(rs == 0 && re == 0 && r <= (it ? 1e-11 : 1e-12) && d <= 1e-5, "a", w, d, 1e-5);
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
      double x[24], f[9][24], jd = jd_of(y), sa = 0, sr = 0;
      int32 fl = SEFLG_MOSEPH | (it ? SEFLG_TOPOCTR : 0);
      int32 r = nodaps(jd, SE_SUN, fl | SEFLG_SPEED, SE_NODBIT_MEAN, x);
      int n = it ? 9 : 5;
      for (j = 0; j < n; j++)
        nodaps(jd + (j - n / 2) * H, SE_SUN, fl, SE_NODBIT_MEAN, f[j]);
      for (k = 2; k < 4; k++)
        for (c = 0; c < 3; c++) {
          double e = fabs(x[6 * k + 3 + c] - rate_of(f, n, k, c, H));
          if (c < 2 && e > sa) sa = e;
          if (c == 2 && e > sr) sr = e;
        }
      if (sa > ws) ws = sa;
      if (sr > wsr) wsr = sr;
      snprintf(w, sizeof w, "Moshier %s %d, the Sun's mean points' speeds: dist %.2e AU/day, deg/day",
               it ? "topo" : "geo", y, sr);
      check(r == 0 && sr <= 1e-12 && sa <= 1e-10, "a", w, sa, 1e-10);
    }
  printf("  (a) the Sun's points against the Earth's: %.2e AU (topocentric %.2e), %.2e deg; Moshier's\n"
         "      mean against the files' %.2e deg, %.2e AU; their speeds %.2e deg/day, %.2e AU/day\n",
         wr, wrt, wd, wm, wmr, ws, wsr);
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
  for (ut = 0; ut < 2; ut++)
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

/* (d) a request fallen to Moshier takes Moshier's frame bias on its
 * ellipse, bug 176: SEFLG_JPLEPH on de200 outside its years, the fallback
 * allowed, answers as SEFLG_MOSEPH, return and points bit for bit. Each
 * pair from a closed context: after a JPL call the first Moshier
 * deflection takes that call's barycentric Sun (5.8e-8 deg on the Sun's
 * mean points), which is not this check's. The Moon's mean points
 * geocentric and topocentric read no file since bug 174 and fall back
 * from nothing: answered under de200, its delta t placing the site, as
 * swe_calc()'s mean node is (1.57e-3 deg from Moshier's topocentric
 * points at 1500, its tidal term's), so against the strict request. */
static void part_d(const char *ephe)
{
  static const int ipl[5] = {SE_SUN, SE_MOON, SE_MARS, SE_SATURN, SE_EARTH};
  static const int32 M[4] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR,
                             SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
  static const int32 C[4] = {0, SEFLG_TOPOCTR, SEFLG_HELCTR, SEFLG_BARYCTR};
  /* 1500 Jan 1 and 2200 Jan 1, either side of de200's 1600-2169 */
  static const double JD[2] = {2268923.5, 2524593.5};
  double a[24], b[24], w = 0;
  char sa[AS_MAXCH], sb[AS_MAXCH], what[160];
  int ib, im, ic, id, k;
  swe_set_ephe_fallback(1);
  for (id = 0; id < 2; id++)
    for (ib = 0; ib < 5; ib++)
      for (im = 0; im < 4; im++)
        for (ic = 0; ic < 4; ic++) {
          int32 fa = SEFLG_JPLEPH | SEFLG_SPEED | C[ic], fb = SEFLG_MOSEPH | SEFLG_SPEED | C[ic], ra, rb;
          double d = 0;
          int same, moonmean = ipl[ib] == SE_MOON && M[im] == SE_NODBIT_MEAN
                               && (C[ic] == 0 || C[ic] == SEFLG_TOPOCTR);
          memset(a, 0, sizeof a);
          memset(b, 0, sizeof b);
          *sa = *sb = '\0';
          swe_close();
          swe_set_ephe_path(ephe);
          swe_set_jpl_file("de200.eph");
          ra = swe_nod_aps(JD[id], ipl[ib], fa, M[im], a, a + 6, a + 12, a + 18, sa);
          if (moonmean) {	/* bug 174: de200 strict */
            swe_set_ephe_fallback(0);
            rb = swe_nod_aps(JD[id], ipl[ib], fa, M[im], b, b + 6, b + 12, b + 18, sb);
            swe_set_ephe_fallback(1);
          } else {
            rb = swe_nod_aps(JD[id], ipl[ib], fb, M[im], b, b + 6, b + 12, b + 18, sb);
          }
          for (k = 0; k < 4; k++)
            if (dir_off(a, k, b, k) > d)
              d = dir_off(a, k, b, k);
          if (d > w)
            w = d;
          same = ra == rb && memcmp(a, b, sizeof a) == 0;
          snprintf(what, sizeof what, "body %d method %d centre %d jd %.1f, de200 fallen to Moshier against %s: ret %d %d, deg",
                   ipl[ib], (int) M[im], (int) C[ic], JD[id], moonmean ? "de200 strict" : "Moshier", ra, rb);
          /* both refuse the barycentric points (bug 149) and Saturn's
           * barycentric ellipse */
          check(same && (rb == 0 || C[ic] == SEFLG_BARYCTR
                         || (ipl[ib] == SE_SATURN && M[im] == SE_NODBIT_OSCU_BAR)), "d", what, d, 0);
        }
  swe_set_ephe_fallback(0);
  printf("  (d) de200 fallen to Moshier against Moshier: %.2e deg\n", w);
}

/* (e) Moshier's Earth-Moon barycentre is its series', bug 180: the
 * osculating points of the Earth (heliocentric) and the Sun (geocentric,
 * topocentric), their speeds the rates of their positions, five-point
 * differences over 2^-6 day at binary instants, on Moshier as on the files */
static void part_e(void)
{
  static const int32 E[2] = {SEFLG_MOSEPH, SEFLG_SWIEPH};
  static const int32 M[2] = {SE_NODBIT_OSCU, SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
  static const struct { int ipl; int32 ctr; } B[3] = {
    {SE_EARTH, SEFLG_HELCTR}, {SE_SUN, 0}, {SE_SUN, SEFLG_TOPOCTR}};
  const double H = 1.0 / 64;
  double wa = 0, wr = 0, wt = 0;
  char w[200];
  int ie, ib, im, y, j, k, c;
  for (ie = 0; ie < 2; ie++)
    for (ib = 0; ib < 3; ib++)
      for (im = 0; im < 2; im++)
        for (y = -3000; y <= 3000; y += 500) {
          double x[24], f[9][24], jd = jd_of(y), sa = 0, sr = 0;
          int32 fl = E[ie] | B[ib].ctr;
          int n = B[ib].ctr == SEFLG_TOPOCTR ? 9 : 5;
          /* the files' topocentric Sun at the floor: 3.9e-9 deg/day, not
           * this entry's */
          double lim = (E[ie] == SEFLG_SWIEPH && B[ib].ctr == SEFLG_TOPOCTR) ? 1e-8 : 2e-9;
          int32 r = nodaps(jd, B[ib].ipl, fl | SEFLG_SPEED, M[im], x);
          for (j = 0; j < n; j++)
            nodaps(jd + (j - n / 2) * H, B[ib].ipl, fl, M[im], f[j]);
          for (k = 2; k < 4; k++)
            for (c = 0; c < 3; c++) {
              double e = fabs(x[6 * k + 3 + c] - rate_of(f, n, k, c, H));
              if (c < 2 && e > sa) sa = e;
              if (c == 2 && e > sr) sr = e;
            }
          if (lim > 2e-9) {
            if (sa > wt) wt = sa;
          } else if (sa > wa) {
            wa = sa;
          }
          if (sr > wr) wr = sr;
          snprintf(w, sizeof w, "%s body %d centre %d method %d %d, the osculating apsides' speeds: dist %.2e AU/day, deg/day",
                   E[ie] == SEFLG_MOSEPH ? "Moshier" : "Swiss", B[ib].ipl, (int) B[ib].ctr, (int) M[im], y, sr);
          check(r == 0 && sr <= 1e-12 && sa <= lim, "e", w, sa, lim);
        }
  printf("  (e) the Earth's and the Sun's osculating apsides' speeds against their positions' rates:\n"
         "      %.2e deg/day (the files' topocentric Sun %.2e), %.2e AU/day\n", wa, wt, wr);
}

/* (f) the Moon's mean points read no file, bug 174: directories of links
 * to EPHEDIR's files but some */
enum { D_ALL, D_NOMOON, D_NOJPL, D_EMPTY, ND };
static const char *DN[ND] = {"complete", "no Moon files", "no JPL file", "empty"};
static char DIRS[ND][PATH_MAX];

static int link_dir(const char *ephe, const char *dir, int which)
{
  DIR *d = opendir(ephe);
  struct dirent *e;
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (d == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || which == D_EMPTY
        || (which == D_NOMOON && strncmp(e->d_name, "semo", 4) == 0)
        || (which == D_NOJPL && strstr(e->d_name, ".eph") != NULL))
      continue;
    snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
    snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
    if (symlink(p, q) != 0) {
      closedir(d);
      return -1;
    }
  }
  closedir(d);
  return 0;
}

static void unlink_dir(const char *dir)
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

static swe_ctx *dir_ctx(int id, int fb)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  swe_set_ephe_path_r(c, DIRS[id]);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_ephe_fallback_r(c, fb);
  swe_set_topo_r(c, 8.55, 47.37, 400);
  return c;
}

#define NDC 5	/* geo, J2000, topo (TT); geo, topo (UT) */
struct dres { int32 r[NDC], rn; double x[NDC][24], node; };

static void d_ask(swe_ctx *c, double jd, int32 eph, int32 meth, struct dres *a)
{
  static const int32 CF[NDC] = {0, SEFLG_J2000, SEFLG_TOPOCTR, 0, SEFLG_TOPOCTR};
  double x[6];
  char serr[AS_MAXCH];
  int k;
  memset(a, 0, sizeof *a);
  for (k = 0; k < NDC; k++) {
    int32 fl = eph | SEFLG_SPEED | CF[k];
    double *p = a->x[k];
    a->r[k] = k < 3 ? swe_nod_aps_r(c, jd, SE_MOON, fl, meth, p, p + 6, p + 12, p + 18, serr)
                    : swe_nod_aps_ut_r(c, jd, SE_MOON, fl, meth, p, p + 6, p + 12, p + 18, serr);
  }
  a->rn = swe_calc_ut_r(c, jd, SE_MEAN_NODE, eph | SEFLG_SPEED, x, serr);
  a->node = x[0];
}

static int part_f(const char *ephe)
{
  static const int32 M[3] = {SE_NODBIT_MEAN, 0, SE_NODBIT_FOPOINT};
  static const int32 E[3] = {SEFLG_SWIEPH, SEFLG_JPLEPH, SEFLG_MOSEPH};
  static const double JD[2] = {2415020.25, 2086308.0};
  static struct dres R[ND][3][2][2][3];	/* directory, ephemeris, fallback, instant, method */
  char what[200];
  double wn = 0, wb = 0;
  int id, ie, fb, it, im, k;
  snprintf(DIRS[D_ALL], PATH_MAX, "%s", ephe);
  for (id = 1; id < ND; id++) {
    snprintf(DIRS[id], PATH_MAX, "/tmp/nodcentre%d-XXXXXX", id);
    if (mkdtemp(DIRS[id]) == NULL || link_dir(ephe, DIRS[id], id) != 0) {
      check(0, "f", "could not make the directories in /tmp", 0, 0);
      while (--id > 0)
        unlink_dir(DIRS[id]);
      return -1;
    }
  }
  /* each directory, ephemeris, setting and instant on a context of its
   * own, the three methods one after another on it */
  for (id = 0; id < ND; id++)
    for (ie = 0; ie < 3; ie++)
      for (fb = 0; fb < 2; fb++)
        for (it = 0; it < 2; it++) {
          swe_ctx *c = dir_ctx(id, fb);
          for (im = 0; im < 3; im++) {
            if (c != NULL)
              d_ask(c, JD[it], E[ie], M[im], &R[id][ie][fb][it][im]);
            else
              R[id][ie][fb][it][im].r[0] = -99;
          }
          swe_ctx_free(c);
        }
  for (id = 0; id < ND; id++)
    for (ie = 0; ie < 3; ie++)
      for (fb = 0; fb < 2; fb++)
        for (it = 0; it < 2; it++)
          for (im = 0; im < 3; im++) {
            const struct dres *a = &R[id][ie][fb][it][im], *s = &R[id][ie][0][it][im];
            const struct dres *f = &R[D_ALL][ie][0][it][im];
            int ok = 1;
            double dn = fabs(swe_difdeg2n(a->x[3][0], a->node));
            for (k = 0; k < NDC; k++)
              ok = ok && a->r[k] == 0;
            if (ok && dn > wn)
              wn = dn;
            snprintf(what, sizeof what, "%s, eph %d, %s, jd %.2f, method %d: ret %d %d %d %d %d, deg",
                     DN[id], (int) E[ie], fb ? "fallback" : "strict", JD[it], (int) M[im],
                     a->r[0], a->r[1], a->r[2], a->r[3], a->r[4]);
            /* geocentric and J2000: the complete directory's points */
            check(ok && memcmp(a->x[0], f->x[0], sizeof a->x[0]) == 0
                  && memcmp(a->x[1], f->x[1], sizeof a->x[1]) == 0, "f", what,
                  fabs(swe_difdeg2n(a->x[0][0], f->x[0][0])), 0);
            /* swe_nod_aps_ut(): the delta t swe_calc_ut() takes for the
             * mean node with the same flags, there */
            check(ok && a->rn == (E[ie] | SEFLG_SPEED) && dn <= 1e-10, "f", what, dn, 1e-10);
            /* the same answer strict and with the fallback: nothing to
             * fall back from, the site placed by the delta t asked */
            if (fb)
              check(ok && memcmp(a->x, s->x, sizeof a->x) == 0, "f", what,
                    fabs(swe_difdeg2n(a->x[2][0], s->x[2][0])), 0);
          }
  /* the heliocentric and barycentric points read the Earth, made from
   * Moshier's Moon there: strict refused as bug 120 refuses the Earth;
   * with the fallback the complete directory's within 1e-7 deg */
  for (fb = 0; fb < 2; fb++)
    for (it = 0; it < 2; it++)
      for (im = 0; im < 3; im++)
        for (k = 0; k < 2; k++) {
          int32 fl = SEFLG_SWIEPH | SEFLG_SPEED | (k ? SEFLG_BARYCTR : SEFLG_HELCTR);
          double a[24], b[24], d = 0;
          char sa[AS_MAXCH] = "", sb[AS_MAXCH];
          swe_ctx *c = dir_ctx(D_NOMOON, fb), *cf = dir_ctx(D_ALL, 0);
          int32 ra = c != NULL ? swe_nod_aps_r(c, JD[it], SE_MOON, fl, M[im], a, a + 6, a + 12, a + 18, sa) : -99;
          int32 rb = cf != NULL ? swe_nod_aps_r(cf, JD[it], SE_MOON, fl, M[im], b, b + 6, b + 12, b + 18, sb) : -99;
          swe_ctx_free(c);
          swe_ctx_free(cf);
          if (ra == 0 && rb == 0)
            d = dir_off(a, 0, b, 0) > dir_off(a, 2, b, 2) ? dir_off(a, 0, b, 0) : dir_off(a, 2, b, 2);
          if (d > wb)
            wb = d;
          snprintf(what, sizeof what, "no Moon files, %s, %s, jd %.2f, method %d: ret %d, deg",
                   k ? "barycentric" : "heliocentric", fb ? "fallback" : "strict", JD[it], (int) M[im], ra);
          check(rb == 0 && (fb ? ra == 0 && d <= 1e-7
                            : ra < 0 && strstr(sa, "Moon file is not available") != NULL), "f", what, d, 1e-7);
        }
  /* swe_calc()'s range of the mean node (-3100015.5 .. 8000016.5, its
   * speed's instants 2^-5 day either side in it), the complete directory:
   * refused where swe_calc(SE_MEAN_NODE) is, with its message, and
   * answered where it is, past the planet files' -3026605.5 .. 7929133.5 */
  for (k = 0; k < 6; k++) {
    static const double JR[6] = {-3100015.5 - 1, -3100015.5 + 1.0 / 64, -3100015.5 + 1,
                                 8000016.5 + 1, 8000016.5 - 1.0 / 64, 8000016.5 - 1};
    double a[24], x[6];
    char sa[AS_MAXCH] = "", sn[AS_MAXCH] = "";
    swe_ctx *c = dir_ctx(D_ALL, 0);
    int32 ra = c != NULL ? swe_nod_aps_r(c, JR[k], SE_MOON, SEFLG_SWIEPH, SE_NODBIT_MEAN, a, a + 6, a + 12, a + 18, sa) : 0;
    int32 rn = c != NULL ? swe_calc_r(c, JR[k], SE_MEAN_NODE, SEFLG_SWIEPH, x, sn) : 0;
    swe_ctx_free(c);
    snprintf(what, sizeof what, "jd %.4f, the mean node's range: ret %d (swe_calc() %d), '%.60s'", JR[k], ra, rn, sa);
    check(c != NULL && (k % 3 == 2 ? ra == 0 && rn >= 0
                        : ra < 0 && rn < 0 && strstr(sa, "outside mean node range") != NULL
                          && strstr(sn, "outside mean node range") != NULL), "f", what, 0, 0);
  }
  printf("  (f) the Moon's mean points with files missing: swe_nod_aps_ut()'s node %.2e deg off the\n"
         "      mean node's; heliocentric and barycentric, the Moon files missing, %.2e deg\n", wn, wb);
  return 0;
}

/* (g) every refusal leaves the points it was given zeroed, as swe_calc()
 * zeroes a refused place and this function an unimplemented body's, bug
 * 179: (f)'s directories, the outputs filled with 1234.5 first */
static void part_g(void)
{
  struct ecase { const char *sz; int id, ipl, ut; int32 fl, meth; double jd; };
  static const struct ecase EC[] = {
    /* an inner swe_calc() refusing: the ellipse's, the save area's */
    {"Mars osculating, no files, strict", D_EMPTY, SE_MARS, 0, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_OSCU, 2451727.0},
    {"Mars mean, no files, strict", D_EMPTY, SE_MARS, 0, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN, 2451727.0},
    {"Mars osculating, JPL, no JPL file, strict", D_NOJPL, SE_MARS, 0, SEFLG_JPLEPH | SEFLG_SPEED, SE_NODBIT_OSCU,
     2451727.0},
    {"the Moon osculating, no Moon files, strict", D_NOMOON, SE_MOON, 0, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_OSCU,
     2451727.0},
    {"the Moon mean, heliocentric, no Moon files, strict", D_NOMOON, SE_MOON, 0,
     SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_HELCTR, SE_NODBIT_MEAN, 2451727.0},
    {"Mars osculating topocentric, no files, strict", D_EMPTY, SE_MARS, 0, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_TOPOCTR,
     SE_NODBIT_OSCU, 2451727.0},
    /* this function's own: the mean node's range (bug 174) */
    {"the Moon mean, beyond the mean node's range", D_ALL, SE_MOON, 0, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN,
     8000016.5 + 1},
    /* the UT form */
    {"Mars osculating, UT, no files, strict", D_EMPTY, SE_MARS, 1, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_OSCU,
     2451727.0},
    {"the Moon mean, UT, beyond the range", D_ALL, SE_MOON, 1, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN,
     8000016.5 + 1},
    /* zeroed before: an unimplemented body, barycentric Moshier (bug 149) */
    {"the mean node, not implemented", D_ALL, SE_MEAN_NODE, 0, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN, 2451727.0},
    {"Mars barycentric on Moshier", D_ALL, SE_MARS, 0, SEFLG_MOSEPH | SEFLG_SPEED | SEFLG_BARYCTR, SE_NODBIT_OSCU,
     2451727.0},
  };
  char what[200];
  int k, i, nul, left;
  for (k = 0; k < (int) (sizeof EC / sizeof EC[0]); k++)
    for (nul = 0; nul < 2; nul++) {
      const struct ecase *e = &EC[k];
      double a[24];
      char serr[AS_MAXCH] = "";
      swe_ctx *c = dir_ctx(e->id, 0);
      int32 r = -99;
      for (i = 0; i < 24; i++)
        a[i] = 1234.5;
      /* all four outputs; then the nodes' alone, the apsides NULL */
      if (c != NULL)
        r = e->ut ? swe_nod_aps_ut_r(c, e->jd, e->ipl, e->fl, e->meth, a, a + 6, nul ? NULL : a + 12,
                                     nul ? NULL : a + 18, serr)
                  : swe_nod_aps_r(c, e->jd, e->ipl, e->fl, e->meth, a, a + 6, nul ? NULL : a + 12,
                                  nul ? NULL : a + 18, serr);
      swe_ctx_free(c);
      for (i = left = 0; i < 24; i++)
        left += nul && i >= 12 ? a[i] != 1234.5 : a[i] != 0;
      snprintf(what, sizeof what, "%s%s: ret %d, '%.50s', outputs not zeroed (or NULL ones written)",
               e->sz, nul ? ", the apsides NULL" : "", r, serr);
      check(r < 0 && left == 0, "g", what, left, 0);
    }
  printf("  (g) refusals: the points zeroed, %d cases\n", (int) (sizeof EC / sizeof EC[0]));
}

/* (h) the Sun's points and the Earth's heliocentric and barycentric ones
 * without the moon files, bug 192: the Moon reaches none of them, so the
 * strict setting answers them as the fallback does, bit for bit, the
 * complete directory's within 1e-12; after them a strict Earth at the
 * instants of their reads refused, as before (no save area left), and the
 * Earth's geocentric points, seen from that Earth, refused */
static void part_h(void)
{
  static const struct { int ipl; int32 ctr; } B[5] = {
    {SE_SUN, 0}, {SE_SUN, SEFLG_TOPOCTR}, {SE_SUN, SEFLG_HELCTR}, {SE_EARTH, SEFLG_HELCTR},
    {SE_EARTH, SEFLG_BARYCTR}};
  static const int32 M[5] = {0, SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR,
                             SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
  static const double JD[3] = {2086308.0, 2415020.25, 2451727.0};
  char what[200];
  double w = 0, wv = 0;
  int ib, im, it, ut, i;
  for (ut = 0; ut < 2; ut++)
    for (it = 0; it < 3; it++)
      for (ib = 0; ib < 5; ib++)
        for (im = 0; im < 5; im++) {
          int32 fl = SEFLG_SWIEPH | SEFLG_SPEED | B[ib].ctr, r[3];
          double x[3][24], d = 0, dv = 0;
          char serr[AS_MAXCH] = "";
          int k;
          for (k = 0; k < 3; k++) {	/* strict, fallback without the moon files; complete */
            swe_ctx *c = dir_ctx(k < 2 ? D_NOMOON : D_ALL, k == 1);
            char se[AS_MAXCH] = "";
            memset(x[k], 0, sizeof x[k]);
            r[k] = c == NULL ? -99
                   : ut ? swe_nod_aps_ut_r(c, JD[it], B[ib].ipl, fl, M[im], x[k], x[k] + 6, x[k] + 12, x[k] + 18, se)
                        : swe_nod_aps_r(c, JD[it], B[ib].ipl, fl, M[im], x[k], x[k] + 6, x[k] + 12, x[k] + 18, se);
            if (k == 0)
              strcpy(serr, se);
            swe_ctx_free(c);
          }
          /* positions within 1e-12 deg and AU, speeds 1e-10: the ellipse's
           * speed differences its points over 2^-6 day, where the two
           * barycentres' rounding (the Earth less one Moon's share and that
           * share again) is 1e-11 deg/day */
          for (i = 0; i < 24; i++) {
            double e = i % 6 == 0 ? fabs(swe_difdeg2n(x[0][i], x[2][i])) : fabs(x[0][i] - x[2][i]);
            if (i % 6 < 3 && e > d)
              d = e;
            if (i % 6 >= 3 && e > dv)
              dv = e;
          }
          if (r[0] >= 0 && d > w)
            w = d;
          if (r[0] >= 0 && dv > wv)
            wv = dv;
          snprintf(what, sizeof what, "%s body %d centre %d method %d jd %.2f, no moon files, strict: ret %d %d %d, '%.50s'",
                   ut ? "UT" : "TT", B[ib].ipl, (int) B[ib].ctr, (int) M[im], JD[it], r[0], r[1], r[2], serr);
          check(r[0] >= 0 && r[1] == r[0] && r[2] == r[0] && memcmp(x[0], x[1], sizeof x[0]) == 0
                && d <= 1e-12 && dv <= 1e-10, "h", what, d > dv ? d : dv, 1e-10);
        }
  /* after the Sun's osculating points, strict, the Earth of their reads
   * at tjd and tjd + 2^-6 refused, with the flags the reads' answers carry
   * (plaus_iflag()'s SEFLG_NOABERR | SEFLG_NOGDEFL), which find a save area
   * left (the first answered so, 99.904 deg at 1900, before the wrappers
   * dropped it); the Earth's geocentric points refused */
  for (it = 0; it < 3; it++) {
    static const int32 FE[2] = {SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_TRUEPOS | SEFLG_HELCTR | SEFLG_NOABERR
                                | SEFLG_NOGDEFL,
                                SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_TRUEPOS
                                | SEFLG_NONUT | SEFLG_SPEED | SEFLG_ICRS | SEFLG_HELCTR | SEFLG_NOABERR
                                | SEFLG_NOGDEFL};
    double x[24], xe[6];
    char se[AS_MAXCH] = "", s1[AS_MAXCH] = "", s2[AS_MAXCH] = "", s3[AS_MAXCH] = "";
    swe_ctx *c = dir_ctx(D_NOMOON, 0);
    int32 r0 = -99, r1 = -99, r2 = -99, r3 = -99;
    if (c != NULL) {
      r0 = swe_nod_aps_r(c, JD[it], SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_OSCU, x, x + 6, x + 12, x + 18, se);
      r1 = swe_calc_r(c, JD[it], SE_EARTH, FE[0], xe, s1);
      r2 = swe_calc_r(c, JD[it] + 1.0 / 64, SE_EARTH, FE[1], xe, s2);
      r3 = swe_nod_aps_r(c, JD[it], SE_EARTH, SEFLG_SWIEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, x + 6, x + 12, x + 18, s3);
    }
    swe_ctx_free(c);
    snprintf(what, sizeof what, "jd %.2f, no moon files, strict: the Sun's points %d, then the Earth %d %d, its geocentric points %d",
             JD[it], r0, r1, r2, r3);
    check(r0 >= 0 && r1 < 0 && r2 < 0 && r3 < 0 && strstr(s1, "Moon file is not available") != NULL
          && strstr(s2, "Moon file is not available") != NULL && strstr(s3, "Moon file is not available") != NULL,
          "h", what, 0, 0);
  }
  printf("  (h) the Sun's and the Earth's heliocentric points without the moon files, strict: %.2e deg off,\n"
         "      the speeds %.2e deg/day\n", w, wv);
}

/* (i) a planet's points seen from the Earth without the moon files, bug
 * 193: refused under the strict setting, as swe_calc() of the planet is,
 * with its message, the points zeroed; with the fallback answered, within
 * 1e-7 deg of the complete directory's; heliocentric and barycentric,
 * which do not use that Earth, answered strict, bit for bit the complete
 * directory's and the fallback's */
static void part_i(void)
{
  static const int ipl[4] = {SE_MERCURY, SE_MARS, SE_JUPITER, SE_PLUTO};
  static const int32 M[4] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR,
                             SE_NODBIT_OSCU | SE_NODBIT_FOPOINT};
  static const int32 C[4] = {0, SEFLG_TOPOCTR, SEFLG_HELCTR, SEFLG_BARYCTR};
  static const double JD[2] = {2415020.25, 2451727.0};
  char what[220];
  double w = 0;
  int ut, it, ib, im, ic, i;
  for (ut = 0; ut < 2; ut++)
    for (it = 0; it < 2; it++)
      for (ib = 0; ib < 4; ib++)
        for (im = 0; im < 4; im++)
          for (ic = 0; ic < 4; ic++) {
            int32 fl = SEFLG_SWIEPH | SEFLG_SPEED | C[ic], r[3], rc = -99;
            double x[3][24], xc[6], d = 0;
            char serr[AS_MAXCH] = "", sc[AS_MAXCH] = "";
            int k, zero = 1;
            for (k = 0; k < 3; k++) {	/* strict, fallback without the moon files; complete */
              swe_ctx *c = dir_ctx(k < 2 ? D_NOMOON : D_ALL, k == 1);
              char se[AS_MAXCH] = "";
              for (i = 0; i < 24; i++)
                x[k][i] = 1234.5;
              r[k] = c == NULL ? -99
                     : ut ? swe_nod_aps_ut_r(c, JD[it], ipl[ib], fl, M[im], x[k], x[k] + 6, x[k] + 12, x[k] + 18, se)
                          : swe_nod_aps_r(c, JD[it], ipl[ib], fl, M[im], x[k], x[k] + 6, x[k] + 12, x[k] + 18, se);
              if (k == 0) {
                strcpy(serr, se);
                /* swe_calc() of the planet there, on a context of its own */
                swe_ctx_free(c);
                c = dir_ctx(D_NOMOON, 0);
                if (c != NULL)
                  rc = ut ? swe_calc_ut_r(c, JD[it], ipl[ib], fl, xc, sc) : swe_calc_r(c, JD[it], ipl[ib], fl, xc, sc);
              }
              swe_ctx_free(c);
            }
            for (i = 0; i < 24; i++) {
              double e = i % 6 == 0 ? fabs(swe_difdeg2n(x[1][i], x[2][i])) : fabs(x[1][i] - x[2][i]);
              if (i % 6 < 3 && e > d)
                d = e;
              zero = zero && x[0][i] == 0;
            }
            snprintf(what, sizeof what, "%s body %d method %d centre %d jd %.2f, no moon files: ret %d %d %d"
                     " (swe_calc() %d), '%.40s', deg", ut ? "UT" : "TT", ipl[ib], (int) M[im], (int) C[ic],
                     JD[it], r[0], r[1], r[2], rc, serr);
            if (ic < 2) {
              if (r[1] >= 0 && d > w)
                w = d;
              check(r[0] < 0 && zero && strstr(serr, "Moon file is not available") != NULL
                    && rc < 0 && strstr(sc, "Moon file is not available") != NULL
                    && r[1] >= 0 && r[2] >= 0 && d <= 1e-7, "i", what, d, 1e-7);
            } else {
              check(r[0] >= 0 && r[1] == r[0] && r[2] == r[0] && memcmp(x[0], x[2], sizeof x[0]) == 0
                    && memcmp(x[1], x[2], sizeof x[1]) == 0, "i", what, d, 0);
            }
          }
  printf("  (i) the planets' points seen from the Earth without the moon files: refused strict,\n"
         "      with the fallback %.2e deg off\n", w);
}

int main(int argc, char **argv)
{
  char ephe[PATH_MAX];
  if (argc != 2 || realpath(argv[1], ephe) == NULL) {
    fprintf(stderr, "usage: nodcentre EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  swe_set_topo(8.55, 47.37, 400);
  swe_set_jpl_file("de200.eph");
  site_ctx = swe_ctx_new();
  swe_set_ephe_path_r(site_ctx, argv[1]);
  swe_set_topo_r(site_ctx, 8.55, 47.37, 400);
  printf("G143: swe_nod_aps()'s change of centre is the one its points are in\n");
  part_a();
  part_b();
  part_c();
  part_d(argv[1]);
  part_e();
  if (part_f(ephe) == 0) {
    part_g();
    part_h();
    part_i();
    for (int id = 1; id < ND; id++)
      unlink_dir(DIRS[id]);
  }
  swe_ctx_free(site_ctx);
  printf("  rows %d, failed %d\n", nrow, nbad);
  printf(nbad ? "G143 FAIL: %d\n" : "G143 PASS\n", nbad);
  return nbad ? 1 : 0;
}
