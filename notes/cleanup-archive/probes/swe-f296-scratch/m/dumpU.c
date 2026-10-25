#include <math.h>
#include <stdio.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  swe_ctx *c = swe_ctx_new();
  char serr[256];
  int il, it, ip;
  char hs = argv[2][0];
  swe_set_ephe_path_r(c, argv[1]);
  for (il = -90; il <= 90; il++)
    for (it = 0; it < 40; it++) {
      double lat = il, tu = 2451545.0 + it * 9.13, cu[13], am[10], x[6];
      if (il == 90) lat = 89.99;
      if (il == -90) lat = -89.99;
      swe_houses_ex2_r(c, tu, SEFLG_SWIEPH, lat, 8.55, hs, cu, am, NULL, NULL, serr);
      swe_calc_ut_r(c, tu, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
      for (ip = 0; ip < 48; ip++) {
        double xp[2] = { ip * 7.5 + 0.3, (ip % 7 - 3) * 2.0 }, hp;
        serr[0] = 0;
        hp = swe_house_pos_r(c, am[2], lat, x[0], hs, xp, serr);
        printf("%g %d %d %.17g %s\n", lat, it, ip, hp, serr);
      }
    }
  return 0;
}
