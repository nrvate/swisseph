/* startopodefl.c -- G60: a topocentric fixed star is deflected by the Sun as
 * seen from the observer's site of this call.
 *
 * The defect (UPSTREAM-BUGS 61): the star's paths called
 * swi_deflect_light() with iflag & SEFLG_SPEED, so the topocentric flag did
 * not reach it: the Earth in the deflection was the geocenter, the site's
 * 4.3e-5 AU missing from the Earth-Sun vector, 4e-5 of the deflection
 * (3e-7 to 9e-6 arcsec for stars 5-40 degrees from the Sun). And had the flag been
 * passed, the deflection read the site from the planets' save area, which
 * the star's observer (computed NO_SAVE) never fills: the site of whatever
 * planet call came last, or none.
 *
 * The check: for four stars near the Sun's path on the dates the Sun is
 * 5-40 degrees from them, and two sites, swe_fixstar2() and swe_fixstar()
 * with SEFLG_TOPOCTR, the deflection's part that depends on the site,
 *   (topocentric - topocentric NOGDEFL) - (geocentric - geocentric NOGDEFL)
 * as unit vectors, equatorial J2000 XYZ without aberration, against
 * g1/g2 (u.q e - u.e q) formed here from swe_calc()'s barycentric Earth and
 * Sun and the site (geocentric minus topocentric Moon); within 2% of the
 * effect (measured: effect 3e-7 to 9e-6 arcsec, agreement 4e-11 arcsec). And the
 * star's topocentric position after topocentric planet calls at other sites
 * and instants equals the one on a fresh context, to the bit.
 * The upstream build fails all 16 of the first: the effect is missing
 * altogether, off by 100% of it.
 * Run as
 *   startopodefl EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define GM    1.32712440017987e+20
#define CC    2.99792458e+8
#define AU    1.49597870700e+11
#define ASEC  (180 / M_PI * 3600)

static int star_at(int old, const char *name, double t, int32 fl, double *x, char *serr)
{
  char s[SE_MAX_STNAME * 2 + 1];
  strcpy(s, name);
  return old ? swe_fixstar(s, t, fl, x, serr) : swe_fixstar2(s, t, fl, x, serr);
}

static void unit(double *a)
{
  double r = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  a[0] /= r; a[1] /= r; a[2] /= r;
}

/* the deflection's displacement of the unit direction u, seen from the
 * Earth-Sun vector e (AU) */
static void defl(const double *u, const double *e0, double *d)
{
  double e[3] = {e0[0], e0[1], e0[2]}, re = sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
  double ue, g1, g2;
  int i;
  unit(e);
  ue = u[0] * e[0] + u[1] * e[1] + u[2] * e[2];
  g1 = 2 * GM / CC / CC / AU / re;
  g2 = 1 + ue;                           /* q = u: the star is 1e5 AU away */
  for (i = 0; i < 3; i++)
    d[i] = g1 / g2 * (e[i] - ue * u[i]);
}

int main(int argc, char **argv)
{
  static const struct { const char *n; double t; } c[4] = {
    {"Regulus", 2460167.3}, {"Spica", 2460245.3}, {"Aldebaran", 2460110.3}, {"Sirius", 2460467.3},
  };
  static const double site[2][3] = {{8.55, 47.37, 400}, {-70.7, -30.2, 2600}};
  const int32 base = SEFLG_SWIEPH | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_J2000 | SEFLG_NONUT | SEFLG_NOABERR;
  const int32 xf = SEFLG_SWIEPH | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_J2000 | SEFLG_NONUT
                   | SEFLG_TRUEPOS | SEFLG_NOGDEFL;
  char serr[256];
  int k, g, old, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: startopodefl EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G60: a topocentric star is deflected from the site of its own call\n");
  for (k = 0; k < 4; k++) {
    for (g = 0; g < 2; g++) {
      double t = c[k].t, ea[6], su[6], mg[6], mt[6], st[3], e[3], ref[3], u[3];
      double eff, err;
      swe_set_topo(site[g][0], site[g][1], site[g][2]);
      if (swe_calc(t, SE_EARTH, xf | SEFLG_BARYCTR, ea, serr) < 0
          || swe_calc(t, SE_SUN, xf | SEFLG_BARYCTR, su, serr) < 0
          || swe_calc(t, SE_MOON, xf, mg, serr) < 0
          || swe_calc(t, SE_MOON, xf | SEFLG_TOPOCTR, mt, serr) < 0) {
        printf("  ERROR %s\n", serr);
        return 1;
      }
      for (i = 0; i < 3; i++) {
        st[i] = mg[i] - mt[i];                       /* the site */
        e[i] = ea[i] - su[i];
      }
      for (old = 0; old <= 1; old++) {
        double d0[3], d1[3], p[4][6], v[3];
        if (star_at(old, c[k].n, t, base, p[0], serr) < 0
            || star_at(old, c[k].n, t, base | SEFLG_NOGDEFL, p[1], serr) < 0
            || star_at(old, c[k].n, t, base | SEFLG_TOPOCTR, p[2], serr) < 0
            || star_at(old, c[k].n, t, base | SEFLG_TOPOCTR | SEFLG_NOGDEFL, p[3], serr) < 0) {
          printf("  %s ERROR %s\n", c[k].n, serr);
          return 1;
        }
        for (i = 0; i < 3; i++) {
          u[i] = p[1][i];
        }
        unit(u);
        for (i = 0; i < 3; i++)
          v[i] = e[i] + st[i];
        defl(u, v, d1);
        defl(u, e, d0);
        for (i = 0; i < 3; i++)
          ref[i] = d1[i] - d0[i];
        {
          double lib[3], rn = sqrt(p[0][0] * p[0][0] + p[0][1] * p[0][1] + p[0][2] * p[0][2]);
          double rn2 = sqrt(p[2][0] * p[2][0] + p[2][1] * p[2][1] + p[2][2] * p[2][2]);
          double rn1 = sqrt(p[1][0] * p[1][0] + p[1][1] * p[1][1] + p[1][2] * p[1][2]);
          double rn3 = sqrt(p[3][0] * p[3][0] + p[3][1] * p[3][1] + p[3][2] * p[3][2]);
          eff = err = 0;
          for (i = 0; i < 3; i++) {
            lib[i] = ((p[2][i] / rn2 - p[3][i] / rn3) - (p[0][i] / rn - p[1][i] / rn1));
            eff += ref[i] * ref[i];
            err += (lib[i] - ref[i]) * (lib[i] - ref[i]);
          }
          eff = sqrt(eff);
          err = sqrt(err);
        }
        printf("  %-9s site %d %-12s effect %.3e arcsec, off by %.3e (%.1f%%)\n", c[k].n, g,
               old ? "swe_fixstar" : "swe_fixstar2", eff * ASEC, err * ASEC, 100 * err / eff);
        if (err > 0.02 * eff) {
          printf("    FAIL: the deflection is not that of the site\n");
          bad++;
        }
      }
    }
  }
  /* the position does not depend on what was computed before */
  for (k = 0; k < 4; k++) {
    for (old = 0; old <= 1; old++) {
      const int32 fl = SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED;
      double t = c[k].t, a[6], b[6], pl[6];
      swe_close();
      swe_set_ephe_path(argv[1]);
      swe_set_topo(site[0][0], site[0][1], site[0][2]);
      star_at(old, c[k].n, t, fl, a, serr);
      swe_close();
      swe_set_ephe_path(argv[1]);
      swe_set_topo(site[1][0], site[1][1], site[1][2]);
      swe_calc(t + 37.3, SE_MARS, fl, pl, serr);
      swe_calc(t - 3.1, SE_MOON, fl, pl, serr);
      swe_set_topo(site[0][0], site[0][1], site[0][2]);
      star_at(old, c[k].n, t, fl, b, serr);
      if (memcmp(a, b, sizeof a) != 0) {
        printf("  %s %s FAIL: after other calls the position differs by %.3e arcsec\n", c[k].n,
               old ? "swe_fixstar" : "swe_fixstar2", fabs(a[0] - b[0]) * 3600);
        bad++;
      }
    }
  }
  swe_close();
  printf(bad ? "G60 FAIL: %d\n" : "G60 PASS\n", bad);
  return bad ? 1 : 0;
}
