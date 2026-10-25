#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
static const double TU[] = { 2378496.5, 2433282.5, 2451545.0, 2488069.5 };
static const double LT[] = { 0, 1e-12, 1e-9, 1e-6, 1e-3, 47.37, -33.87, 60, -60, 5, -5 };
int main(int argc, char **argv)
{
  swe_ctx *c = swe_ctx_new();
  int l, it, k;
  char serr[256];
  swe_set_ephe_path_r(c, argv[1]);
  for (l = 0; l < (int) (sizeof LT / sizeof LT[0]); l++) {
    double w = 0; int kw = 0, itw = 0;
    for (it = 0; it < 4; it++) {
      double cu[13], am[10], x[6];
      swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LT[l], 8.55, 'U', cu, am, NULL, NULL, serr);
      swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
      for (k = 1; k <= 12; k++) {
        double xp[2] = { cu[k], 1e-300 }, hp, d;
        hp = swe_house_pos_r(c, am[2], LT[l], x[0], 'U', xp, serr);
        d = hp - k; if (d > 6) d -= 12; if (d < -6) d += 12;
        d -= 1e-3 / 3600 / 30;   /* the 0.001" */
        if (fabs(d) > w) { w = fabs(d); kw = k; itw = it; }
      }
    }
    printf("lat %g: max |hpos - k - 0.001\"| %.3e house (cusp %d, date %d)\n", LT[l], w, kw, itw);
  }
  return 0;
}
