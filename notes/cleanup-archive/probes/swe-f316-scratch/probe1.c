#include <stdio.h>
#include <math.h>
#include "swephexp.h"
int main(int argc, char **argv) {
  double t0 = 7090348.28, x[4][6];
  char serr[256];
  int methods[] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR, SE_NODBIT_OSCU|SE_NODBIT_FOPOINT};
  swe_set_ephe_path(argc > 1 ? argv[1] : "ephe");
  int noflag = argc > 2 ? atoi(argv[2]) : 0;
  for (int m = 0; m < 4; m++) {
    for (int k = -8; k <= 8; k++) {
      double t = floor(t0 * 256) / 256 + k / 1024.0;
      int fl = SEFLG_SWIEPH | SEFLG_SPEED | noflag;
      if (swe_nod_aps(t, SE_MARS, fl, methods[m], x[0], x[1], x[2], x[3], serr) < 0) { printf("ERR %s\n", serr); return 1; }
      printf("m%d k%+d", methods[m], k);
      for (int j = 0; j < 4; j++) printf("  %.10f %.10f %.4f", x[j][0], x[j][1], x[j][2]);
      printf("\n");
    }
  }
  return 0;
}
