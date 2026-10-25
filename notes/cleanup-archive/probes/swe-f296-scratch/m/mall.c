#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
static const double TU[] = { 2378496.5, 2433282.5, 2451545.0, 2488069.5 };
static const double LT[] = { 47.37, -33.87, -47.37, 60, -60, 0, 1e-9, 10, -10 };
int main(int argc, char **argv)
{
  static const char HS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxy";
  swe_ctx *c = swe_ctx_new();
  int l, it, k, h;
  char serr[256];
  swe_set_ephe_path_r(c, argv[1]);
  for (h = 0; HS[h]; h++) {
    int hs = HS[h], nc = (hs == 'G' || hs == 'g') ? 36 : 12;
    for (l = 0; l < (int) (sizeof LT / sizeof LT[0]); l++) {
      double w = 0; int nb = 0;
      for (it = 0; it < 4; it++) {
        double cu[37], am[10], x[6];
        swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LT[l], 8.55, hs, cu, am, NULL, NULL, serr);
        swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
        for (k = 1; k <= nc; k++) {
          double xp[2] = { cu[k], 1e-300 }, hp, d;
          hp = swe_house_pos_r(c, am[2], LT[l], x[0], hs, xp, serr);
          d = fmod(hp - k + 1.5 * nc, nc) - nc / 2.0;
          if (fabs(d) > 1e-7) nb++;
          w = fmax(w, fabs(d));
        }
      }
      if (w > 1e-7) printf("'%c' at %g: %d cusps off, max %.3e house\n", hs, LT[l], nb, w);
    }
  }
  return 0;
}
