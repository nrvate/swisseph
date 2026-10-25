#include <stdio.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  double geo[3] = { 8.55, 47.37, 400 }, tr, s = 0; char serr[256]; int i;
  swe_set_ephe_path(argv[1]);
  if (argc > 2) swe_set_topo(-70, -30, 0);
  for (i = 0; i < 2000; i++) {
    swe_rise_trans(2433282.5 + i, SE_MOON, NULL, SEFLG_SWIEPH, SE_CALC_RISE, geo, 0, 0, &tr, serr);
    s += tr;
  }
  printf("%.6f\n", s);
  return 0;
}
