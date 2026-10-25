/* nodaposcrate.c -- G154: swe_nod_aps()'s osculating nodes' and apsides'
 * speeds are their positions' rate.
 *
 * The defect (UPSTREAM-BUGS 201). The osculating points (SE_NODBIT_OSCU)
 * took their speeds as a three-point difference of the points at tjd -+ h,
 * h = 2^-6 day for the planets and 2^-11 for the Moon (entry 80), off by
 * h^2 / 6 of the point's third derivative, which a near-circular orbit's
 * apsides make large, and over the Moon's short step by its points' noise
 * over 2 h. Against nine-point differences of the positions over 2^-4 day
 * (the Moon 2^-5), at 1125 dates 1800-2100 on the Swiss files: Neptune's
 * heliocentric perihelion 5.3e-7 deg/day off in 1815 (turning at 0.1
 * deg/day, its third derivative 1.3e-2 deg/day^3), Uranus's 4.2e-8, the
 * Earth's 3.5e-8, Jupiter's 2.2e-8, Saturn's 2.0e-8, Venus's 1.3e-8,
 * Mercury's 1.2e-8; the Moon's geocentric apsides 7.9e-8 and nodes 1.6e-8
 * (7.8e-8 and 6.8e-8 on de200 and Moshier); the distances' speeds up to
 * 1.2e-8 AU/day (Neptune's points, geocentric).
 *
 * The check, the Swiss files, default models, ecliptic of date:
 *  (b) the Sun's points (the Earth's orbit), the Moon's and Mercury's to
 *      Pluto's, geocentric and heliocentric, at eleven dates of that survey
 *      where the three-point speeds were worst (1808-2096, binary
 *      fractions): the longitude's, latitude's and distance's speeds of the
 *      four points against a nine-point central difference of their
 *      positions over 2^-4 day (the Moon 2^-5), within 1e-8 deg/day and
 *      5e-11 AU/day (the reference's own noise is 2e-11 on Neptune's). A
 *      date counts where that reference agrees with the one over half the
 *      step within 3e-9 deg/day; it does not where a point passes behind
 *      the Sun (the deflection inside the disc turns in hours: Jupiter's
 *      descending node at the end of 1815, the Sun's perigee in January),
 *      at most 2 of 11 a body and centre.
 * Measured: within 2.3e-9 deg/day for the Moon, 1.3e-9 for the planets
 * (Neptune's, the reference's noise) but Pluto's geocentric aphelion in
 * 1812, 7.7e-9 (the aberration's rate there, which SEFLG_NOABERR takes to
 * 2e-12; not this item's), the distances 2.0e-11 AU/day; 3 dates not
 * counted. 20 checks; before the fix 18 fail (Neptune 5.3e-7, the Moon
 * 7.9e-8, the distances 1.2e-8 AU/day), Mars's heliocentric and the Moon's
 * heliocentric pass. The upstream build (SWE_UPSTREAM_COMPAT) fails all 20
 * (its difference over 1e-3 day per AU, the Moon 1e-4, at rounded
 * instants: 2.9e-5 deg/day, 7.9e-5 AU/day).
 * Run as
 *   nodaposcrate EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

/* the nine-point central difference's weights, eighth order */
static const double c9[4] = { 4.0 / 5, -1.0 / 5, 4.0 / 105, -1.0 / 280 };
static const char *PT[4] = { "asc node", "dsc node", "perihelion", "aphelion" };

static int nodaps(swe_ctx *c, double t, int ipl, int32 fl, double x[4][6])
{
  char serr[256];
  if (swe_nod_aps_r(c, t, ipl, fl, SE_NODBIT_OSCU, x[0], x[1], x[2], x[3], serr) < 0) {
    printf("    ERROR at %.8f: %s\n", t, serr);
    return -1;
  }
  return 0;
}

/* the nine-point rate over h of the four points' longitude, latitude and
 * distance, in r[point][coordinate] */
static int rate9(swe_ctx *c, double t, int ipl, int32 fl, double h, double r[4][3])
{
  double p[9][4][6];
  int k, q, j;
  for (k = 0; k < 9; k++)
    if (nodaps(c, t + (k - 4) * h, ipl, fl, p[k]) < 0)
      return -1;
  for (q = 0; q < 4; q++)
    for (j = 0; j < 3; j++) {
      double d = 0;
      for (k = 1; k <= 4; k++)
        d += c9[k - 1] * (j == 0 ? swe_difdeg2n(p[4 + k][q][j], p[4 - k][q][j])
                                 : p[4 + k][q][j] - p[4 - k][q][j]);
      r[q][j] = d / h;
    }
  return 0;
}

/* check (b): away from the edges; returns the failures, *n the checks */
static int check_b(const char *ephe, int *n)
{
  static const double D[] = {
    2381419.296875, 2382878.94921875, 2384338.59765625, 2395140.0078125,
    2395431.9375, 2403119.4296875, 2408082.23828125, 2426571.13671875,
    2461408.1171875, 2485151.7578125, 2486611.40625,
  };
  static const struct {
    int ipl;
    const char *name;
  } SB[] = {
    { SE_SUN, "Sun" }, { SE_MOON, "Moon" }, { SE_MERCURY, "Mercury" }, { SE_VENUS, "Venus" },
    { SE_MARS, "Mars" }, { SE_JUPITER, "Jupiter" }, { SE_SATURN, "Saturn" },
    { SE_URANUS, "Uranus" }, { SE_NEPTUNE, "Neptune" }, { SE_PLUTO, "Pluto" },
  };
  const double lim = 1e-8, dlim = 5e-11, agree = 3e-9;
  swe_ctx *c = swe_ctx_new();
  int b, hc, i, q, j, bad = 0;
  swe_set_ephe_path_r(c, ephe);
  printf("  (b) Swiss files, default models, 11 dates 1808-2096: worst |speed - reference|,"
         " deg/day (distance AU/day)\n");
  for (b = 0; b < (int) (sizeof SB / sizeof SB[0]); b++)
    for (hc = 0; hc < 2; hc++) {
      int32 fl = SEFLG_SWIEPH | (hc ? SEFLG_HELCTR : 0);
      double h = SB[b].ipl == SE_MOON ? ldexp(1, -5) : ldexp(1, -4);
      double w = 0, wd = 0, wt = 0;
      int wq = 0, nskip = 0;
      for (i = 0; i < (int) (sizeof D / sizeof D[0]); i++) {
        double x[4][6], r[4][3], r2[4][3], dis = 0;
        if (nodaps(c, D[i], SB[b].ipl, fl | SEFLG_SPEED, x) < 0
            || rate9(c, D[i], SB[b].ipl, fl, h, r) < 0
            || rate9(c, D[i], SB[b].ipl, fl, h / 2, r2) < 0) {
          bad++;
          continue;
        }
        /* the reference resolved: over h / 2 the same within agree, else a
         * point behind the Sun or a joint within the stencil */
        for (q = 0; q < 4; q++)
          for (j = 0; j < 2; j++)
            if (!(fabs(r[q][j] - r2[q][j]) <= dis))
              dis = fabs(r[q][j] - r2[q][j]);
        if (!(dis <= agree)) {
          nskip++;
          printf("    %-8s %-6s JD %.4f: reference unresolved (%.1e over h and h/2), not counted\n",
                 SB[b].name, hc ? "helioc" : "geoc", D[i], dis);
          continue;
        }
        for (q = 0; q < 4; q++)
          for (j = 0; j < 3; j++) {
            double e = fabs(x[q][3 + j] - r[q][j]);
            if (j < 2 && !(e <= w)) {
              w = e;
              wq = q;
              wt = D[i];
            }
            if (j == 2 && !(e <= wd))
              wd = e;
          }
      }
      (*n)++;
      printf("    %-8s %-6s lon/lat %.2e (%s, JD %.4f)  distance %.1e\n", SB[b].name,
             hc ? "helioc" : "geoc", w, PT[wq], wt, wd);
      if (!(w <= lim && wd <= dlim && nskip <= 2)) {
        bad++;
        printf("    FAIL (b) %s %s: %.2e > %.0e or %.1e > %.0e or %d dates unresolved\n",
               SB[b].name, hc ? "heliocentric" : "geocentric", w, lim, wd, dlim, nskip);
      }
    }
  swe_ctx_free(c);
  return bad;
}

int main(int argc, char **argv)
{
  int n = 0, bad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: nodaposcrate EPHEDIR\n");
    return 2;
  }
  bad += check_b(argv[1], &n);
  printf("  %d checks\n", n);
  printf(bad ? "G154 FAIL: %d\n" : "G154 PASS\n", bad);
  return bad ? 1 : 0;
}
