#include <math.h>
#include <stdio.h>
#include "swephexp.h"
int main(int argc, char **argv) {
  double lat = argc > 1 ? atof(argv[1]) : 70, cu[13], am[10], x[6], xp[2], hp, hp0;
  char serr[256];
  int it, i, shown = 0;
  long cnt[8] = {0};
  swe_set_ephe_path("/nvm/work/swe-f290/ephe");
  for (it = 0; it < 400; it++) {
    double tu = 2451545.0 + it * 0.91;
    swe_calc_ut(tu, SE_ECL_NUT, 0, x, serr);
    swe_houses_ex2(tu, 0, lat, 8.55, 'I', cu, am, NULL, NULL, serr);
    double mcdec = atan(sin(am[2] * M_PI / 180) * tan(x[0] * M_PI / 180)) * 180 / M_PI;
    int mcu = fabs(lat - mcdec) > 90;
    double arg = tan(am[9] * M_PI / 180) * tan(lat * M_PI / 180);
    int circ = fabs(arg) >= 1;
    double acmc = swe_difdeg2n(am[0], am[1]);
    int nb = 0; char buf[400]; int o = 0;
    for (i = 1; i <= 12; i++) {
      xp[0] = cu[i]; xp[1] = 1e-300;
      hp = swe_house_pos(am[2], lat, x[0], 'I', xp, serr);
      xp[1] = 0;
      hp0 = swe_house_pos(am[2], lat, x[0], 'I', xp, serr);
      if (fabs(remainder(hp - i, 12)) > 1e-7) { nb++; o += sprintf(buf + o, " %d:%.3f", i, hp); }
      (void) hp0;
    }
    cnt[(mcu << 2) | (circ << 1) | (acmc < 0)] += nb;
    if (nb && shown < 12) {
      shown++;
      printf("it %3d armc %7.2f mcdec %6.2f mcunder %d sundec %6.2f circ %d acmc %7.2f nbad %d:%s\n", it, am[2], mcdec, mcu, am[9], circ, acmc, nb, buf);
    }
  }
  for (i = 0; i < 8; i++) printf("mcunder %d circumpolar %d acmc<0 %d: %ld bad cusps\n", i >> 2, (i >> 1) & 1, i & 1, cnt[i]);
  return 0;
}
