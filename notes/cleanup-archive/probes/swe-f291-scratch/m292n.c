#include <stdio.h>
#include <string.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  double x[24], te = 2433282.5 + swe_deltat_ex(2433282.5, SEFLG_SWIEPH, NULL), xe[6];
  char serr[256];
  int p, m, f;
  static const int PL[2] = { SE_SUN, SE_EARTH };
  static const int32 M[2] = { 0, SE_NODBIT_OSCU };
  static const int32 F[2] = { 0, SEFLG_HELCTR };
  swe_set_ephe_path(argv[1]);
  swe_calc(te, SE_SUN, SEFLG_SWIEPH, xe, serr);
  printf("Sun geo %.6f %.6f %.9f\n", xe[0], xe[1], xe[2]);
  for (p = 0; p < 2; p++) for (m = 0; m < 2; m++) for (f = 0; f < 2; f++) {
    int32 r;
    memset(x, 0, sizeof x); serr[0] = 0;
    r = swe_nod_aps(te, PL[p], SEFLG_SWIEPH | SEFLG_SPEED | F[f], M[m], x, x + 6, x + 12, x + 18, serr);
    printf("%s %s %s r %d asc %.4f %.4f %.6f dsc %.4f %.4f %.6f peri %.4f %.4f %.6f aph %.4f %.4f %.6f %s\n", p ? "Earth" : "Sun", m ? "osc" : "mean", f ? "hel" : "geo", r,
      x[0], x[1], x[2], x[6], x[7], x[8], x[12], x[13], x[14], x[18], x[19], x[20], serr);
  }
  return 0;
}
