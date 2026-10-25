#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "swephexp.h"
/* max |hpos - k| of Treindl's cusps (lat 1e-300), non-circumpolar Sun, per latitude */
int main(int argc, char **argv) {
  static const double LT[] = {0, 20, 40, 47.37, 55, 60, 63, 65, 66, 66.3, 67, 68, 70, 75, 80, 85, -33.87, -60, -66, -70, -80};
  int l, it, i;
  char hs = argc > 1 ? argv[1][0] : 'I';
  swe_set_ephe_path("/nvm/work/swe-f290/ephe");
  for (l = 0; l < (int) (sizeof LT / sizeof LT[0]); l++) {
    double lat = LT[l], w = 0, wk = 0, warg = 0; int wi = 0, nerr = 0, nc = 0;
    for (it = 0; it < 400; it++) {
      double tu = 2451545.0 + it * 0.91, cu[13], am[10], x[6], xp[2], hp;
      char serr[256];
      swe_calc_ut(tu, SE_ECL_NUT, 0, x, serr);
      if (swe_houses_ex2(tu, 0, lat, 8.55, hs, cu, am, NULL, NULL, serr) == ERR) { nerr++; continue; }
      double arg = tan(am[9] * M_PI / 180) * tan(lat * M_PI / 180);
      if (fabs(arg) >= 1) { nc++; continue; }
      for (i = 1; i <= 12; i++) {
        xp[0] = cu[i]; xp[1] = 1e-300;
        hp = swe_house_pos(am[2], lat, x[0], hs, xp, serr);
        double e = fabs(remainder(hp - i, 12));
        if (e > w) { w = e; wi = it; wk = i; warg = arg; }
      }
    }
    printf("%c lat %6.2f: max %.3e house (it %d cusp %g arg %.5f); %d ERR, %d circumpolar dates\n", hs, lat, w, wi, wk, warg, nerr, nc);
  }
  return 0;
}
