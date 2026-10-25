#include <stdio.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  double x[6] = {0}, te = 2433282.5 + swe_deltat_ex(2433282.5, SEFLG_SWIEPH, NULL);
  char serr[256] = ""; int32 r;
  swe_set_ephe_path(argv[1]);
  r = swe_calc_pctr(te, SE_ECL_NUT, SE_SUN, SEFLG_SWIEPH, x, serr);
  printf("pctr(ECL_NUT from Sun) %d %.4f %.4f %.4f [%s]\n", r, x[0], x[1], x[2], serr);
  r = swe_calc_pctr(te, SE_MARS, SE_ECL_NUT, SEFLG_SWIEPH, x, serr);
  printf("pctr(Mars from ECL_NUT) %d %.4f %.4f %.4f [%s]\n", r, x[0], x[1], x[2], serr);
  r = swe_calc_pctr(te, SE_MARS, -2, SEFLG_SWIEPH, x, serr);
  printf("pctr(Mars from -2) %d %.4f %.4f %.4f [%s]\n", r, x[0], x[1], x[2], serr);
  r = swe_calc_pctr(te, SE_MARS, SE_EARTH, SEFLG_SWIEPH, x, serr);
  printf("pctr(Mars from Earth) %d %.4f %.4f %.4f [%s]\n", r, x[0], x[1], x[2], serr);
  r = swe_calc(te, SE_MARS, SEFLG_SWIEPH, x, serr);
  printf("calc(Mars) %d %.4f %.4f %.4f [%s]\n", r, x[0], x[1], x[2], serr);
  return 0;
}
