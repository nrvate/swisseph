/* orbelcalc.c -- G87: swe_calc_orbel() answers its first call as a warm one,
 * and its speeds are the rates of its positions.
 *
 * swe_calc_orbel_r() (this fork's) set up what swecalc() does for any body
 * but two things (UPSTREAM-BUGS 88): it never brought the obliquity and
 * nutation caches to tjd, so app_pos_rest() rotated with whatever the last
 * swe_calc() left -- nothing as a context's first call, which answered
 * zeros (XYZ (0.17, -0, 0)) with no error, and another instant's frame of
 * date after one (1.8e-5 deg/day on a latitude speed); and it kept the save
 * areas across a change of ephemeris, so on Moshier, whose origin is the
 * Sun, a Swiss or JPL barycentric Sun was added to the orbit (0.009 AU,
 * 2.4e-3 deg/day).
 *
 * The check: (1) eight first calls (Swiss, Moshier, JPL; ecliptic, XYZ,
 * equator, heliocentric J2000 XYZ, sidereal, topocentric with speed), four
 * orbits each, equal to the bit to the same call made after a swe_calc()
 * at another instant. (2) Four orbits (a = 0.137, 0.387 and 5.2 AU about
 * the Sun, 0.0068 AU about the Earth) on Swiss, Moshier and de200,
 * geocentric of date, true, J2000, equator of date, heliocentric,
 * barycentric and topocentric: the speeds against five-point differences
 * of the positions over 2^-7 day, nine dates 1800-2150, every call after a
 * swe_calc() 1000 days away, within 5e-9 deg/day (Moshier 2e-8; measured
 * <= 1.6e-9 and 5.1e-9), distance rates per unit distance likewise. The
 * build before the fix fails 20 of the 37 checks, the SWE_UPSTREAM_COMPAT
 * build 33.
 *
 * Run as
 *   orbelcalc EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#ifndef LIM_SPEED
# define LIM_SPEED 5e-9		/* deg/day, Swiss and JPL */
# define LIM_SPEED_MOSH 2e-8	/* Moshier, whose Earth is 5e-14 AU off a smooth curve */
#endif

static int bad = 0;

static const struct { double el[6]; int32 ctr; const char *sz; } orb[] = {
  {{10, 0.13744, 0.019, 30, 47, 7.5}, SE_SUN, "a = 0.137 AU"},
  {{200, 0.387, 0.2056, 29, 48, 7.0}, SE_SUN, "a = 0.387 AU"},
  {{100, 5.2, 0.048, 274, 100, 1.3}, SE_SUN, "a = 5.2 AU"},
  {{50, 0.00684, 0.1587, 8, 136, 2.5}, SE_EARTH, "Earth, a = 0.0068"},
};

/* (1) a context's first call against the same call made warm, after a
 * swe_calc() at another instant */
static void first_call(const char *ephe, int32 fl, const char *sz)
{
  char serr[256];
  double a[6], b[6], c[6], t = 2451545.0 + 7305.25, w = 0;
  int ib, i;
  for (ib = 0; ib < 4; ib++) {
    int32 r1, r2;
    swe_close();
    swe_set_ephe_path(ephe);
    swe_set_jpl_file("de200.eph");
    swe_set_topo(8.55, 47.37, 400);
    r1 = swe_calc_orbel(t, orb[ib].el, orb[ib].ctr, SE_ORBEL_EQ_J2000, 0, fl, a, serr);
    swe_calc(t - 3000.5, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, c, serr);
    r2 = swe_calc_orbel(t, orb[ib].el, orb[ib].ctr, SE_ORBEL_EQ_J2000, 0, fl, b, serr);
    if (r1 < 0 || r2 < 0) {
      printf("  first call, %-6s %-18s %s: ERROR %s\n", sz, orb[ib].sz, "", serr);
      bad++;
      continue;
    }
    for (i = 0; i < 6; i++)
      if (fabs(a[i] - b[i]) > w)
        w = fabs(a[i] - b[i]);
  }
  printf("  first call against warm, %-28s %.2e\n", sz, w);
  if (w != 0) {
    printf("    FAIL: the first call does not answer as a warm one\n");
    bad++;
  }
}

/* (2) speeds against five-point differences over 2^-7 day, nine dates
 * 1800-2150; the elements are moved by their mean motion with the date */
static void speeds(int32 fl, const char *sz)
{
  char serr[256];
  double h = 1.0 / 128, w[3] = {0, 0, 0}, lim = (fl & SEFLG_MOSEPH) ? LIM_SPEED_MOSH : LIM_SPEED;
  int ib, k, j, i;
  for (ib = 0; ib < 4; ib++) {
    /* the topocentric speed of a body 0.0068 AU away carries the site's
     * velocity, 1.3e-7 of it off (2.9e-7 deg/day; swe_calc()'s Waldemath
     * the same): not this function's */
    if ((fl & SEFLG_TOPOCTR) && orb[ib].ctr == SE_EARTH)
      continue;
    double mu = orb[ib].ctr == SE_SUN ? 0.01720209895 * 0.01720209895
                                      : 0.0000298122353216 * 0.0000298122353216;
    double n = sqrt(mu / (orb[ib].el[1] * orb[ib].el[1] * orb[ib].el[1])) * 180 / M_PI;
    for (k = 0; k < 9; k++) {
      double t = floor((2378496.5 + k * 16000.3 + 0.137) * 256) / 256, x[6], d[5][6], q[6];
      /* each call after a swe_calc() at another instant, whose caches of
       * date it must not answer from */
      swe_calc(t + 1000.5, SE_MARS, fl & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH), q, serr);
      if (swe_calc_orbel(t, orb[ib].el, orb[ib].ctr, SE_ORBEL_EQ_J2000, 0, fl | SEFLG_SPEED, x, serr) < 0
          || (!(fl & SEFLG_XYZ) && !(x[2] > 0))) {
        printf("  speed, %s, %s: ERROR or no position %s\n", sz, orb[ib].sz, serr);
        bad++;
        break;
      }
      for (j = 0; j < 5; j++) {
        double e[6];
        memcpy(e, orb[ib].el, sizeof e);
        e[0] += n * (j - 2) * h;
        swe_calc(t + 1000.5, SE_MARS, fl & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH), q, serr);
        swe_calc_orbel(t + (j - 2) * h, e, orb[ib].ctr, SE_ORBEL_EQ_J2000, 0, fl, d[j], serr);
      }
      for (i = 0; i < 3; i++) {
        double v[5], r, s;
        for (j = 0; j < 5; j++)
          v[j] = (i == 0 && !(fl & SEFLG_XYZ)) ? d[2][0] + swe_difdeg2n(d[j][0], d[2][0]) : d[j][i];
        r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
        s = fabs(x[3 + i] - r);
        if (i == 2 && !(fl & SEFLG_XYZ))
          s /= x[2];		/* distance rate per unit distance */
        if (s > w[i])
          w[i] = s;
      }
    }
  }
  printf("  speed, %-40s %.2e %.2e %.2e\n", sz, w[0], w[1], w[2]);
  if (w[0] > lim || w[1] > lim || w[2] > lim) {
    printf("    FAIL: speed off its positions' rate by more than %.0e\n", lim);
    bad++;
  }
}

int main(int argc, char **argv)
{
  static const struct { int32 fl; const char *sz; } rf[] = {
    {SEFLG_SWIEPH, "Swiss, ecliptic of date"},
    {SEFLG_SWIEPH | SEFLG_XYZ, "Swiss, XYZ of date"},
    {SEFLG_SWIEPH | SEFLG_EQUATORIAL, "Swiss, equator of date"},
    {SEFLG_MOSEPH, "Moshier, ecliptic of date"},
    {SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_XYZ, "Swiss, helio J2000 XYZ"},
    {SEFLG_JPLEPH, "JPL, ecliptic of date"},
    {SEFLG_SWIEPH | SEFLG_SIDEREAL, "Swiss, sidereal"},
    {SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED, "Swiss, topo, speed"},
  };
  static const struct { int32 fl; const char *sz; } rs[] = {
    {0, "geo, of date"},
    {SEFLG_TRUEPOS, "geo, of date, true"},
    {SEFLG_J2000 | SEFLG_NONUT, "geo, J2000"},
    {SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS, "geo, J2000, true"},
    {SEFLG_EQUATORIAL, "geo, equator of date"},
    {SEFLG_HELCTR, "helio, of date"},
    {SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT, "helio, J2000"},
    {SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS, "helio, J2000, true"},
    {SEFLG_BARYCTR | SEFLG_J2000 | SEFLG_NONUT, "bary, J2000"},
    {SEFLG_TOPOCTR, "topo, of date"},
  };
  static const struct { int32 fl; const char *sz; } re[] = {
    {SEFLG_SWIEPH, "Swiss"}, {SEFLG_MOSEPH, "Mosh"}, {SEFLG_JPLEPH, "JPL"},
  };
  int i, j;
  char sz[80];

  if (argc != 2) {
    fprintf(stderr, "usage: orbelcalc EPHEDIR\n");
    return 2;
  }
  printf("G87: swe_calc_orbel()'s first call and its speeds\n");
  for (i = 0; i < 8; i++)
    first_call(argv[1], rf[i].fl, rf[i].sz);
  swe_close();
  swe_set_ephe_path(argv[1]);
  swe_set_jpl_file("de200.eph");
  swe_set_topo(8.55, 47.37, 400);
  for (j = 0; j < 3; j++)
    for (i = 0; i < 10; i++) {
      if ((re[j].fl & SEFLG_MOSEPH) && (rs[i].fl & SEFLG_BARYCTR))
        continue;
      snprintf(sz, sizeof sz, "%s, %s", re[j].sz, rs[i].sz);
      speeds(re[j].fl | rs[i].fl, sz);
    }
  printf(bad ? "G87 FAIL: %d\n" : "G87 PASS\n", bad);
  return bad ? 1 : 0;
}
