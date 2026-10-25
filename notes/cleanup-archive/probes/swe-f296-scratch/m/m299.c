#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
static const double TU[] = { 2378496.5, 2433282.5, 2451545.0, 2488069.5 };
static const double LT[] = { 47.37, -33.87, -47.37, 60, -60, 0, 10, -10 };
int main(int argc, char **argv)
{
  static const double BL[] = { 1e-300, -1e-300, 1e-15, -1e-15, 1e-12, -1e-12, 1e-9, -1e-9, 1e-6, -1e-6, 1, -1 };
  swe_ctx *c = swe_ctx_new();
  int l, it, k, b, nf = 0;
  char serr[256];
  swe_set_ephe_path_r(c, argv[1]);
  for (b = 0; b < (int) (sizeof BL / sizeof BL[0]); b++) {
    nf = 0;
    printf("lat %g:", BL[b]);
    for (l = 0; l < (int) (sizeof LT / sizeof LT[0]); l++)
      for (it = 0; it < 4; it++) {
        double cu[13], am[10], x[6];
        swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LT[l], 8.55, 'K', cu, am, NULL, NULL, serr);
        swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
        for (k = 4; k <= 10; k += 6) {
          double xp[2] = { cu[k], BL[b] }, hp;
          serr[0] = 0;
          hp = swe_house_pos_r(c, am[2], LT[l], x[0], 'K', xp, serr);
          if (hp == 0 || fabs(hp - k) > 1e-6) {
            nf++;
            if (fabs(BL[b]) < 1e-5) printf(" [%.2f %d c%d %.12f %s]", LT[l], it, k, hp, serr);
          }
        }
      }
    printf("  -> %d of %d off\n", nf, 2 * 4 * (int) (sizeof LT / sizeof LT[0]));
  }
  return 0;
}
