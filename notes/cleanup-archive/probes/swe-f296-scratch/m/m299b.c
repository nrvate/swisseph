#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
static const double TU[] = { 2378496.5, 2433282.5, 2451545.0, 2488069.5 };
static const double LT[] = { 47.37, -33.87, 60, -60 };
int main(int argc, char **argv)
{
  static const double BL[] = { 1e-9, -1e-9, 1, -1, 5, -5 };
  swe_ctx *c = swe_ctx_new();
  int l, it, k, b;
  char serr[256];
  swe_set_ephe_path_r(c, argv[1]);
  for (b = 0; b < (int) (sizeof BL / sizeof BL[0]); b++) {
    printf("lat %g:\n", BL[b]);
    for (l = 0; l < (int) (sizeof LT / sizeof LT[0]); l++)
      for (it = 0; it < 4; it++) {
        double cu[13], am[10], x[6];
        swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LT[l], 8.55, 'K', cu, am, NULL, NULL, serr);
        swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
        printf("  %6.2f %d:", LT[l], it);
        for (k = 4; k <= 10; k += 6) {
          double xp[3] = { cu[k], BL[b], 1 }, hp, xe[3];
          serr[0] = 0;
          hp = swe_house_pos_r(c, am[2], LT[l], x[0], 'K', xp, serr);
          xe[0] = xp[0]; xe[1] = xp[1]; xe[2] = 1;
          swe_cotrans(xe, xe, -x[0]);
          printf("  c%d %.10f md %+.2e%s", k, hp, swe_difdeg2n(xe[0], am[2] + (k == 4) * 180), hp == 0 ? " FAILED" : "");
        }
        printf("\n");
      }
  }
  return 0;
}
