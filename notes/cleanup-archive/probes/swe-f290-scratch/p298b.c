#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "swephexp.h"
int main(int argc, char **argv) {
  double lat = atof(argv[1]), cu[13], am[10], x[6], xp[2], hp;
  char serr[256];
  int it, i, shown = 0, lt;
  long hist[2][13] = {{0}};
  swe_set_ephe_path("/nvm/work/swe-f290/ephe");
  for (it = 0; it < 400; it++) {
    double tu = 2451545.0 + it * 0.91;
    swe_calc_ut(tu, SE_ECL_NUT, 0, x, serr);
    swe_houses_ex2(tu, 0, lat, 8.55, 'I', cu, am, NULL, NULL, serr);
    double arg = tan(am[9] * M_PI / 180) * tan(lat * M_PI / 180);
    int circ = fabs(arg) >= 1;
    for (lt = 0; lt < 2; lt++)
    for (i = 1; i <= 12; i++) {
      xp[0] = cu[i]; xp[1] = lt ? 1e-300 : 0;
      hp = swe_house_pos(am[2], lat, x[0], 'I', xp, serr);
      double e = remainder(hp - i, 12);
      if (fabs(e) > 1e-7) {
        hist[lt][(int) floor(fabs(e) + 0.5)]++;
        if (!circ && shown < 10) { shown++; printf("noncirc it %d lat%d cusp %d hp %.6f arg %.4f acmc %.2f\n", it, lt, i, hp, arg, swe_difdeg2n(am[0], am[1])); }
      }
    }
  }
  for (lt = 0; lt < 2; lt++) { printf("lat %g xlat %s:", lat, lt ? "1e-300" : "0"); for (i = 0; i < 7; i++) printf(" %d:%ld", i, hist[lt][i]); printf("\n"); }
  return 0;
}
