/* nodaposcrate.c -- G154: swe_nod_aps()'s osculating nodes' and apsides'
 * speeds are their positions' rate.
 *
 * The defects. (UPSTREAM-BUGS 201) The osculating points (SE_NODBIT_OSCU)
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
 * (UPSTREAM-BUGS 199) Each epoch of that difference is put into the frame
 * of date at that epoch, so where the stencil spans a short-term
 * precession model's window edge (entry 160) the frame's step was taken
 * as the orbit's motion, as the true node's was (entry 186): under
 * "0,9,1" on the Swiss files within 1/8 day of J2000 - 2 centuries Mars's
 * ascending node 5.2e-3 deg/day off, the Moon's nodes 8.0e-2 and apsides
 * 0.16; with the five-point difference of 201 the Moon's points 6.5e-3,
 * Uranus's 1.7e-2, and under "0,9,8" at -+ 75 centuries the Moon's 7.9,
 * Mars's 8.1, Jupiter's 41.5, the distances' speeds 1.0 AU/day.
 *
 * The checks, the four points' longitude's, latitude's and distance's
 * speeds against a nine-point central difference of their positions:
 *  (a) across the windows' edges: "0,9,1" at J2000 - 2 centuries (de200
 *      and the Swiss files) and + 2 centuries, "0,9,8" at -+ 75 centuries
 *      (the Swiss files); the Moon and Mercury to Pluto, geocentric,
 *      ecliptic and equator of date; 105 instants within 1/4 day of the
 *      edge (every 1/16 day and at 2^-7 to 2^-26 day either side), the
 *      reference over 2^-5 day from a context of the instant's side's
 *      model alone ("0,1,1" or "0,8,8" inside, "0,9,9" outside), within
 *      1e-8 deg/day and 5e-11 AU/day, within the stencil (1/32 day for the
 *      Moon, 1/16 for the planets) and from there to 1/4 day. A point
 *      counts where the reference agrees with the one over half the step
 *      within 1e-8 deg/day (the Moon's at 75 centuries, 4e-9, is the
 *      reference's own), at least 7 in 8 a body and frame.
 *  (b) away from the edges, the Swiss files, default models, ecliptic of
 *      date: the Sun's points (the Earth's orbit), the Moon's and Mercury's
 *      to Pluto's, geocentric and heliocentric, at eleven dates of that
 *      survey where the three-point speeds were worst (1808-2096, binary
 *      fractions), the reference over 2^-4 day (the Moon 2^-5), within
 *      1e-8 deg/day and 5e-11 AU/day (the reference's own noise is 2e-11
 *      on Neptune's). A date counts where that reference agrees with the
 *      one over half the step within 3e-9 deg/day; it does not where a
 *      point passes behind the Sun (the deflection inside the disc turns
 *      in hours: Jupiter's descending node at the end of 1815, the Sun's
 *      perigee in January), at most 2 of 11 a body and centre.
 * Measured: (a) within 4.6e-9 deg/day for the Moon (4.1e-9 and 4.6e-9 at
 * 75 centuries, 1.6e-9 at 2 centuries), 8.4e-10 for the planets, the
 * distances 6.1e-12 AU/day; Jupiter's points 3 of 196 unresolved at - 2
 * centuries (its descending node behind the Sun), 1 at + 2. (b) within 2.3e-9 deg/day for the
 * Moon, 1.3e-9 for the planets (Neptune's, the reference's noise) but
 * Pluto's geocentric aphelion in 1812, 7.7e-9 (the aberration's rate
 * there, which SEFLG_NOABERR takes to 2e-12; not this item's), the
 * distances 2.0e-11 AU/day; 3 dates not counted. 200 checks. Before 199's
 * fix 90 fail, all of (a) within the stencil (the numbers above); before
 * 201's too 136 (118 of (a): the Moon 0.16 at 2 centuries, 217 at 75,
 * Uranus 2.9e-2, Jupiter 71; 18 of (b): Neptune 5.3e-7, the Moon 7.9e-8,
 * the distances 1.2e-8 AU/day; Mars's heliocentric and the Moon's
 * heliocentric pass). The upstream build (SWE_UPSTREAM_COMPAT) fails all
 * 200: (a) the Moon 0.87 deg/day at 2 centuries and 951 at 75, beyond the
 * stencil 1.1e-5, the planets 0.16 and 210 (its difference over 1e-3 day
 * per AU, the Moon 1e-4, at rounded instants); (b) 2.9e-5 deg/day, 7.9e-5
 * AU/day.
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

/* the edge of check (a) and the side its own instant is on */
static double TS;
static int LEFT;

/* t on the edge's left side: before it, or at it where that is the left's */
static int left_of(double t)
{
  return LEFT ? (t <= TS) : (t < TS);
}

/* check (a): across the windows' edges; returns the failures, *n the
 * checks */
static int check_a(const char *ephe, int *n)
{
  static const struct {
    double ts;
    int left;
    int32 eph;
    const char *mod, *in, *out;
  } SE[] = {
    { 2378495.0, 0, SEFLG_JPLEPH, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 2378495.0, 0, SEFLG_SWIEPH, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 2524595.0, 1, SEFLG_SWIEPH, "0,9,1,0,0,0,0,0", "0,1,1,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { -287830.0, 0, SEFLG_SWIEPH, "0,9,8,0,0,0,0,0", "0,8,8,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
    { 5190920.0, 1, SEFLG_SWIEPH, "0,9,8,0,0,0,0,0", "0,8,8,0,0,0,0,0", "0,9,9,0,0,0,0,0" },
  };
  static const struct {
    int ipl;
    const char *name;
  } SB[] = {
    { SE_MOON, "Moon" }, { SE_MERCURY, "Mercury" }, { SE_VENUS, "Venus" }, { SE_MARS, "Mars" },
    { SE_JUPITER, "Jupiter" }, { SE_SATURN, "Saturn" }, { SE_URANUS, "Uranus" },
    { SE_NEPTUNE, "Neptune" }, { SE_PLUTO, "Pluto" },
  };
  static const struct {
    int32 fl;
    const char *name;
  } SF[] = { { 0, "ecl" }, { SEFLG_EQUATORIAL, "equ" } };
  static const char *sn[2] = { "within the stencil", "from there to 1/4 day" };
  const double lim = 1e-8, dlim = 5e-11, agree = 1e-8, h = ldexp(1, -5);
  double tq[105];
  int m, b, f, i, k, q, j, bad = 0, nq;
  for (m = 0; m < (int) (sizeof SE / sizeof SE[0]); m++) {
    swe_ctx *cc[3];
    const char *ms[3] = { SE[m].mod, SE[m].in, SE[m].out };
    for (k = 0; k < 3; k++) {
      char mod[40];
      cc[k] = swe_ctx_new();
      strcpy(mod, ms[k]);
      swe_set_ephe_path_r(cc[k], ephe);
      swe_set_jpl_file_r(cc[k], "de200.eph");
      swe_set_astro_models_r(cc[k], mod, 0);
    }
    TS = SE[m].ts;
    LEFT = SE[m].left;
    nq = 0;
    for (k = -32; k <= 32; k++)
      tq[nq++] = TS + k / 16.0;
    for (k = 7; k <= 26; k++) {
      tq[nq++] = TS - ldexp(1, -k);
      tq[nq++] = TS + ldexp(1, -k);
    }
    printf("  (a) %s, models %s, edge JD %.1f: worst |speed - reference|, deg/day"
           " (distance AU/day), %s / %s\n", SE[m].eph == SEFLG_JPLEPH ? "de200" : "Swiss",
           SE[m].mod, TS, sn[0], sn[1]);
    for (b = 0; b < (int) (sizeof SB / sizeof SB[0]); b++)
      for (f = 0; f < 2; f++) {
        double w[2] = { 0, 0 }, wd[2] = { 0, 0 };
        double stencil = SB[b].ipl == SE_MOON ? 1.0 / 32 : 1.0 / 16;
        int32 fl = SE[m].eph | SF[f].fl;
        int nskip = 0, ncmp = 0;
        for (i = 0; i < nq; i++) {
          double x[4][6], r[4][3], r2[4][3], y[6];
          char serr[256];
          int inside = (TS < 2451545.0) ? !left_of(tq[i]) : left_of(tq[i]);
          int sp = fabs(tq[i] - TS) <= stencil ? 0 : 1;
          swe_ctx *cr = cc[inside ? 1 : 2];
          if (fabs(tq[i] - TS) > 0.25)
            continue;
          /* the ephemeris asked, not a fallback */
          if ((swe_calc_r(cc[0], tq[i], SB[b].ipl, fl, y, serr)
               & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)) != SE[m].eph
              || nodaps(cc[0], tq[i], SB[b].ipl, fl | SEFLG_SPEED, x) < 0
              || rate9(cr, tq[i], SB[b].ipl, fl, h, r) < 0
              || rate9(cr, tq[i], SB[b].ipl, fl, h / 2, r2) < 0) {
            printf("    ERROR %s at %.8f %s\n", SB[b].name, tq[i], serr);
            bad++;
            continue;
          }
          for (q = 0; q < 4; q++) {
            /* the reference resolved, as in (b): a point behind the Sun is
             * not (Jupiter's descending node at - 2 centuries) */
            ncmp++;
            if (!(fabs(r[q][0] - r2[q][0]) <= agree && fabs(r[q][1] - r2[q][1]) <= agree)) {
              nskip++;
              continue;
            }
            for (j = 0; j < 3; j++) {
              double e = fabs(x[q][3 + j] - r[q][j]);
              if (j < 2 && !(e <= w[sp]))
                w[sp] = e;
              if (j == 2 && !(e <= wd[sp]))
                wd[sp] = e;
            }
          }
        }
        printf("    %-8s %-4s %.2e  %.2e  (distance %.1e  %.1e)", SB[b].name, SF[f].name,
               w[0], w[1], wd[0], wd[1]);
        if (nskip)
          printf("  %d of %d unresolved", nskip, ncmp);
        printf("\n");
        for (q = 0; q < 2; q++) {
          (*n)++;
          if (!(w[q] <= lim && wd[q] <= dlim && 8 * nskip <= ncmp)) {
            bad++;
            printf("    FAIL (a) %s, models %s, %s %s, %s: %.2e > %.0e or %.1e > %.0e"
                   " or %d of %d unresolved\n", SE[m].eph == SEFLG_JPLEPH ? "de200" : "Swiss",
                   SE[m].mod, SB[b].name, SF[f].name, sn[q], w[q], lim, wd[q], dlim, nskip, ncmp);
          }
        }
      }
    for (k = 0; k < 3; k++)
      swe_ctx_free(cc[k]);
  }
  return bad;
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
  bad += check_a(argv[1], &n);
  bad += check_b(argv[1], &n);
  printf("  %d checks\n", n);
  printf(bad ? "G154 FAIL: %d\n" : "G154 PASS\n", bad);
  return bad ? 1 : 0;
}
