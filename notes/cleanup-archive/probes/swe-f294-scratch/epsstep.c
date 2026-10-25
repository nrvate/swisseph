#include <math.h>
#include <stdio.h>
#include "swephexp.h"
int main(void)
{
  double t0 = 2437684.5, x1[6], x2[6], d;
  int32 fl[3] = {SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, SEFLG_JPLEPH | SEFLG_JPLHOR, SEFLG_SWIEPH | SEFLG_JPLHOR_APPROX};
  int k;
  swe_set_ephe_path("/nvm/work/swe-f294/ephe");
  swe_set_jpl_file("de200.eph");
  for (k = 0; k < 3; k++) {
    for (d = 1e-3; d > 1e-7; d /= 100) {
      swe_calc(t0 - d, SE_ECL_NUT, fl[k], x1, NULL);
      swe_calc(t0 + d, SE_ECL_NUT, fl[k], x2, NULL);
      printf("fl %d d %g: eps step %.3e deg\n", fl[k], d, x2[0] - x1[0]);
    }
  }
  return 0;
}
