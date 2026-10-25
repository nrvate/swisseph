#include <stdio.h>
#include <time.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  double jd = 0; char serr[256] = ""; int32 r;
  clock_t c0 = clock();
  swe_set_ephe_path(argv[1]);
  r = swe_helio_cross(SE_ECL_NUT, 100, 2433282.5, SEFLG_SWIEPH, 0, &jd, serr);
  printf("helio_cross(SE_ECL_NUT): %d %.6f [%s] %.1f s\n", r, jd, serr, (double) (clock() - c0) / CLOCKS_PER_SEC);
  return 0;
}
