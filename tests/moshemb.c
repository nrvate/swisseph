/* moshemb.c -- G150: Moshier's Earth is its EMB less the Moon the Moon is
 * computed with.
 *
 * The defect (UPSTREAM-BUGS 191). swi_moshplan() made the Earth from the
 * series' EMB less the share of the short lunar series' Moon
 * (embofs_mosh(), six terms in longitude), 0.24 deg off the full series'
 * Moon at 2000 and 1.55 deg at -3000, which swe_calc() returns for the
 * Moon: the Earth was 1.3e-7 AU (7.9e-7 at -3000) off its EMB less its own
 * Moon's share, in the terms of 9 to 35 days the short series leaves out,
 * and every geocentric place with it.
 *
 * The check, Moshier, thirteen dates -3000..3000, eight instants each
 * 1.37 day apart (multiples of 2^-8 day):
 *   (a) the heliocentric Earth plus the geometric geocentric Moon over
 *       (1 + EMRAT), both swe_calc()'s J2000 equatorial places (SEFLG_ICRS:
 *       no frame bias), against the EMB series there (swi_moshplan2()):
 *       within 5e-16 AU (measured 1.1e-16, an ulp of the EMB);
 *   (b) the Earth's heliocentric velocity against the five-point
 *       difference of its places over 2^-5 day: within 1e-12 AU/day
 *       (measured 1.6e-13, the difference's own rounding, as before the
 *       fix; a velocity with the other series' Moon would be 3e-8 off);
 *   (c) the Earth's acceleration (swi_mosh_earth_accel(), whose reads at
 *       +-2^-8 day carry the saved Earth's Moon) less the EMB series'
 *       second difference over the same reads, the Moon's share it used,
 *       against the share of the geometric Moon's second difference over
 *       +-2^-6 day: within 5e-11 AU/day^2 (measured 2.1e-11; a
 *       gravitational model of the full Moon, the pair's pull and the
 *       Sun's tide, is 3e-5 to 9e-5 of the pull off the series' own
 *       acceleration, about 1.2e-10 here).
 * Before the fix (a) was 6.7e-7 AU and (c) 3.9e-8 AU/day^2. The upstream
 * build fails (a) at every instant (6.7e-7 AU at most) and (b), 9.3e-8
 * AU/day (its velocity a difference over 8.64 s, UPSTREAM-BUGS 51); (c)
 * is not built there (swi_mosh_earth_accel() is the fork's). Run as
 *   moshemb EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"
#include "sweph.h"	/* the context, EARTH_MOON_MRAT, the Moshier series */
#include "swephlib.h"	/* swi_polcart(), swi_coortrf2() */

static swe_ctx *c;

/* the EMB series at t, J2000 equatorial cartesian, AU */
static void emb(double t, double *x)
{
  swi_moshplan2(c, t, 2, x);	/* Moshier's EMB is its body 2 */
  swi_polcart(x, x);
  swi_coortrf2(x, x, -c->oec2000.seps, c->oec2000.ceps);
}

/* Moshier, J2000 equatorial cartesian (ICRS), geometric, with speed */
static int calc(double t, int ipl, int32 fl, double *x)
{
  char serr[AS_MAXCH];
  fl |= SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_TRUEPOS
        | SEFLG_NOABERR | SEFLG_NOGDEFL | SEFLG_ICRS | SEFLG_SPEED;
  if (swe_calc(t, ipl, fl, x, serr) < 0) {
    printf("  ERROR %s\n", serr);
    return 0;
  }
  return 1;
}

static double dist3(const double *a, const double *b)
{
  return sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1])
              + (a[2] - b[2]) * (a[2] - b[2]));
}

int main(int argc, char **argv)
{
  const double mr = EARTH_MOON_MRAT + 1.0, h = 1.0 / 32;
  double wa = 0, wb = 0;
#ifndef SWE_UPSTREAM_COMPAT
  double wc = 0;
#endif
  int y, k, i, j, bad;
  if (argc != 2) {
    fprintf(stderr, "usage: moshemb EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  c = swi_default_ctx();
  printf("G150: Moshier's Earth is its EMB less the Moon the Moon is computed with\n");
  for (y = -3000; y <= 3000; y += 500)
    for (k = 0; k < 8; k++) {
      double t = floor((2451545.0 + (y - 2000) * 365.25 + 100.37 + k * 1.37) * 256) / 256;
      double xe[6], xm[6], xb[3], q[5][6], r[3];
      /* (a) */
      if (!calc(t, SE_MOON, 0, xm) || !calc(t, SE_EARTH, SEFLG_HELCTR, xe))
        return 1;
      emb(t, xb);
      for (i = 0; i <= 2; i++)
        r[i] = xe[i] + xm[i] / mr;
      if (dist3(r, xb) > wa)
        wa = dist3(r, xb);
      /* (b) */
      for (j = 0; j < 5; j++)
        if (!calc(t + (j - 2) * h, SE_EARTH, SEFLG_HELCTR, q[j]))
          return 1;
      for (i = 0; i <= 2; i++)
        r[i] = (8 * (q[3][i] - q[1][i]) - (q[4][i] - q[0][i])) / (12 * h);
      if (dist3(xe + 3, r) > wb)
        wb = dist3(xe + 3, r);
#ifndef SWE_UPSTREAM_COMPAT
      /* (c): the Moon at t +- H, then the Earth saved at t again */
      {
        const double H = 1.0 / 64, ha = 1.0 / 256;
        double mp[6], mm[6], a[3], ep[3], em[3], e0[3];
        if (!calc(t + H, SE_MOON, 0, mp) || !calc(t - H, SE_MOON, 0, mm)
            || !calc(t, SE_MOON, 0, xm) || !calc(t, SE_EARTH, SEFLG_HELCTR, xe))
          return 1;
        swi_mosh_earth_accel(c, t, c->pldat[SEI_EARTH].x, a);
        emb(t + ha, ep);
        emb(t - ha, em);
        emb(t, e0);
        for (i = 0; i <= 2; i++) {
          r[i] = (ep[i] - 2 * e0[i] + em[i]) / (ha * ha) - a[i];
          xb[i] = (mp[i] - 2 * xm[i] + mm[i]) / (H * H) / mr;
        }
        if (dist3(r, xb) > wc)
          wc = dist3(r, xb);
      }
#endif
    }
  printf("  (a) Earth + Moon / (1 + EMRAT) - EMB series     %.2e AU%s\n", wa,
         wa > 5e-16 ? "  FAIL" : "");
  printf("  (b) Earth's velocity - five-point difference     %.2e AU/day%s\n", wb,
         wb > 1e-12 ? "  FAIL" : "");
  bad = (wa > 5e-16) + (wb > 1e-12);
#ifndef SWE_UPSTREAM_COMPAT
  printf("  (c) acceleration's Moon share - the Moon's       %.2e AU/day^2%s\n", wc,
         wc > 5e-11 ? "  FAIL" : "");
  bad += wc > 5e-11;
#endif
  if (bad)
    printf("G150 FAIL: %d\n", bad);
  else
    printf("G150 PASS\n");
  return bad ? 1 : 0;
}
