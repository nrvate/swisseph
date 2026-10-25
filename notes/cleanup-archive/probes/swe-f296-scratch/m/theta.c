#include <math.h>
#include <stdio.h>
#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"
int main(int argc, char **argv)
{
  swe_ctx *c = swe_ctx_new();
  double e2, em, nut[2], x[3], l, r, Om = 107.582569 * DEGTORAD, in = 1.578701 * DEGTORAD;
  int j, iy;
  swe_set_ephe_path_r(c, argv[1]);
  for (iy = -1; iy <= 1; iy++) {
    double tjde = J2000 + iy * 36525.0, M[3][3];
    e2 = swi_epsiln(c, J2000, 0); em = swi_epsiln(c, tjde, 0);
    swi_nutation(c, tjde, 0, nut);
    for (j = 0; j < 2; j++) {
      double v[3] = { j == 0, j == 1, 0 }, w[3];
      w[0] = v[0]; w[1] = v[1] * cos(in) - v[2] * sin(in); w[2] = v[1] * sin(in) + v[2] * cos(in);
      x[0] = w[0] * cos(Om) - w[1] * sin(Om); x[1] = w[0] * sin(Om) + w[1] * cos(Om); x[2] = w[2];
      swi_coortrf(x, x, -e2);
      swi_precess(c, x, tjde, 0, J2000_TO_J);
      swi_coortrf(x, x, em);
      l = atan2(x[1], x[0]) + nut[0]; r = hypot(x[0], x[1]);
      x[0] = r * cos(l); x[1] = r * sin(l);
      swi_coortrf(x, x, -(em + nut[1]));
      M[0][j] = x[0]; M[1][j] = x[1]; M[2][j] = x[2];
    }
    printf("TT J2000%+d cy: theta %.6f\n", iy, atan2(M[2][0], M[2][1]) * RADTODEG);
  }
  return 0;
}
