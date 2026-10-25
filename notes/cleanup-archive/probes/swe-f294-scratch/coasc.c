#include <math.h>
#include <stdio.h>
#include "swephexp.h"
int main(void)
{
  double tu = 2378496.5, cu[37], am[10], cs[37], as[10], xo[6];
  int e, j, ax;
  swe_set_ephe_path("/nvm/work/swe-f294/ephe");
  swe_houses_ex2(tu, 0, 47.37, 8.55, 'P', cu, am, cs, as, NULL);
  for (ax = 0; ax < 8; ax++) {
    printf("ascmc[%d] lib speed %.10f:", ax, as[ax]);
    for (e = 10; e <= 13; e++) {
      double h = ldexp(1, -e), a[5];
      for (j = -2; j <= 2; j++) { double c2[37], a2[10]; swe_houses(tu + j*h, 47.37, 8.55, 'P', c2, a2); a[j+2] = a2[ax]; }
      printf(" %d:%+.2e", e, (8*swe_difdeg2n(a[3],a[1]) - swe_difdeg2n(a[4],a[0]))/(12*h) - as[ax]);
    }
    printf("\n");
  }
  (void) xo;
  return 0;
}
