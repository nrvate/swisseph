#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "swephexp.h"
int main(int argc, char **argv) {
  double lat = atof(argv[1]), tu = 2451545.0 + atoi(argv[2]) * 0.91, cu[13], am[10], x[6], xp[2];
  char serr[256]; int i;
  swe_set_ephe_path("/nvm/work/swe-f290/ephe");
  swe_calc_ut(tu, SE_ECL_NUT, 0, x, serr);
  swe_houses_ex2(tu, 0, lat, 8.55, 'I', cu, am, NULL, NULL, serr);
  printf("armc %.10f sundec %.6f asc %.10f mc %.10f serr '%s'\n", am[2], am[9], am[0], am[1], serr);
  for (i = 1; i <= 12; i++) {
    double h0, h1;
    xp[0] = cu[i]; xp[1] = 0; h0 = swe_house_pos(am[2], lat, x[0], 'I', xp, serr);
    xp[1] = 1e-300; h1 = swe_house_pos(am[2], lat, x[0], 'I', xp, serr);
    printf("%2d %.13f  d10 %+.3e  hp0 %.9f hp1 %.9f\n", i, cu[i], swe_difdeg2n(cu[i], cu[10]), h0, h1);
  }
  return 0;
}
