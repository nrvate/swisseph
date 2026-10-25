/* rate301 DIR FB T0: speeds vs five-point one-sided differences of places without the flag */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  const char *dir = argv[1]; int fb = atoi(argv[2]); double t0 = atof(argv[3]);
  static const int bodies[] = {SE_SUN, SE_MERCURY, SE_MARS, SE_JUPITER, SE_PLUTO, SE_CHIRON, SE_CERES};
  static const int32 fls[] = {0, SEFLG_TOPOCTR, SEFLG_EQUATORIAL, SEFLG_J2000};
  static const double off[] = {1.0/1048576, 1.0/4096, 1.0/512, 1.0/64, 1.0/8};
  static const double c5[5] = {-25, 48, -36, 16, -3};
  const double h = 1.0 / 256;
  double worst = 0;
  swe_set_ephe_path(dir); swe_set_ephe_fallback(fb); swe_set_topo(8.55, 47.37, 400);
  for (unsigned ib = 0; ib < sizeof bodies / sizeof *bodies; ib++)
    for (unsigned jf = 0; jf < sizeof fls / sizeof *fls; jf++)
      for (unsigned ko = 0; ko < sizeof off / sizeof *off; ko++) {
        double jd = floor((t0 + off[ko]) * 1048576) / 1048576, x[6], y[5][6];
        char serr[AS_MAXCH];
        int32 fl = fls[jf] | SEFLG_SWIEPH, r = swe_calc(jd, bodies[ib], fl | SEFLG_SPEED, x, serr), rr = 0;
        if (r < 0) { printf("%d %d %g refused: %s\n", bodies[ib], fl, off[ko], serr); continue; }
        for (int k = 0; k < 5; k++) rr |= swe_calc(jd + k * h, bodies[ib], fl, y[k], serr) < 0;
        if (rr) { printf("%d %d %g place refused\n", bodies[ib], fl, off[ko]); continue; }
        for (int c = 0; c < 2; c++) {
          double d = 0;
          for (int k = 1; k < 5; k++) d += c5[k] * (c == 0 ? swe_difdeg2n(y[k][0], y[0][0]) : y[k][c] - y[0][c]);
          d /= 12 * h;
          double e = fabs(x[3 + c] - d);
          if (e > worst) worst = e;
          if (getenv("V")) printf("%d %d %g c%d spd %.12g rate %.12g err %.3g\n", bodies[ib], fl, off[ko], c, x[3+c], d, e);
        }
      }
  printf("worst %.3g\n", worst);
  return 0;
}
