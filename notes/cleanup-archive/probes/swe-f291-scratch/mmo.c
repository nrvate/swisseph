#include <stdio.h>
#include <time.h>
#include <string.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  double tret[10] = {0}, attr[20] = {0}, geo[3] = { 8.55, 47.37, 400 }; char serr[256] = ""; int32 r;
  clock_t c0;
  swe_set_ephe_path(argv[1]);
  c0 = clock();
  r = swe_lun_occult_where(2433282.5, SE_MOON, NULL, SEFLG_SWIEPH, geo, attr, serr);
  printf("occult_where(Moon): %d %.4f %.4f [%s] %.1f s\n", r, geo[0], geo[1], serr, (double) (clock() - c0) / CLOCKS_PER_SEC);
  fflush(stdout);
  c0 = clock(); serr[0] = 0;
  r = swe_lun_occult_when_loc(2433282.5, SE_MOON, NULL, SEFLG_SWIEPH, geo, tret, attr, 0, serr);
  printf("occult_when_loc(Moon): %d %.4f [%s] %.1f s\n", r, tret[0], serr, (double) (clock() - c0) / CLOCKS_PER_SEC);
  fflush(stdout);
  c0 = clock(); serr[0] = 0;
  r = swe_lun_occult_when_glob(2433282.5, SE_MOON, NULL, SEFLG_SWIEPH, 0, tret, 0, serr);
  printf("occult_when_glob(Moon): %d %.4f [%s] %.1f s\n", r, tret[0], serr, (double) (clock() - c0) / CLOCKS_PER_SEC);
  return 0;
}
