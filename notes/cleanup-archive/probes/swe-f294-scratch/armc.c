#include <math.h>
#include <stdio.h>
#include "swephexp.h"
int main(void)
{
  double tu0[3] = {2378496.5, 2378496.5 + 54786.375, 2378496.5 + 2*54786.375};
  int i, e;
  swe_set_ephe_path("/nvm/work/swe-f294/ephe");
  for (i = 0; i < 3; i++) {
    double cu[37], am[10], cs[37], as[10];
    swe_houses_ex2(tu0[i], 0, 47.37, 8.55, 'P', cu, am, cs, as, NULL);
    printf("tu %.3f lib ARMC rate %.12f Asc %.12f\n", tu0[i], as[2], as[0]);
    for (e = 6; e <= 14; e += 2) {
      double h = ldexp(1, -e), a[5], s[5];
      int j;
      for (j = -2; j <= 2; j++) { double c2[37], a2[10]; swe_houses(tu0[i] + j*h, 47.37, 8.55, 'P', c2, a2); a[j+2] = a2[2]; s[j+2]=a2[0]; }
      printf("  h 2^-%d: ARMC %.12f Asc %.12f\n", e, (8*swe_difdeg2n(a[3],a[1]) - swe_difdeg2n(a[4],a[0]))/(12*h), (8*swe_difdeg2n(s[3],s[1]) - swe_difdeg2n(s[4],s[0]))/(12*h));
    }
  }
  return 0;
}
