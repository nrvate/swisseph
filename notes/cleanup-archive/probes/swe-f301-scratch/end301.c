/* end301 DIRA DIRB T1: speeds after the Moon file's end in A (with the file) and B (without), fallback allowed */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"
static void get(const char *dir, int ipl, int32 fl, double jd, double *x, int32 *r)
{
  int fd[2]; pipe(fd); fflush(stdout);
  if (fork() == 0) { char serr[AS_MAXCH]; double y[7]; close(fd[0]); swe_set_ephe_path(dir); swe_set_ephe_fallback(1); swe_set_topo(8.55, 47.37, 400);
    y[6] = swe_calc(jd, ipl, fl, y, serr); write(fd[1], y, sizeof y); _exit(0); }
  double y[7]; close(fd[1]); read(fd[0], y, sizeof y); close(fd[0]); wait(NULL); memcpy(x, y, 6 * sizeof(double)); *r = (int32) y[6];
}
int main(int argc, char **argv)
{
  double t1 = atof(argv[3]);
  static const int bodies[] = {SE_SUN, SE_MARS, SE_PLUTO, SE_CERES};
  static const int32 fls[] = {SEFLG_SPEED, SEFLG_SPEED | SEFLG_TOPOCTR};
  static const double off[] = {1.0/1048576, 1.0/4096, 1.0/512, 1.0/64, 1.0/8};
  double worst = 0;
  for (unsigned ib = 0; ib < 4; ib++) for (unsigned jf = 0; jf < 2; jf++) for (unsigned ko = 0; ko < 5; ko++) {
    double jd = t1 + off[ko], a[6], b[6]; int32 ra, rb;
    get(argv[1], bodies[ib], fls[jf] | SEFLG_SWIEPH, jd, a, &ra);
    get(argv[2], bodies[ib], fls[jf] | SEFLG_SWIEPH, jd, b, &rb);
    double dp = fabs(swe_difdeg2n(a[0], b[0])), dv = fabs(a[3] - b[3]);
    printf("%d %d %g r %d/%d dlon %.3g dspd %.3g\n", bodies[ib], fls[jf], off[ko], ra, rb, dp, dv);
    if (dv > worst) worst = dv;
  }
  printf("worst dspd %.3g\n", worst);
  return 0;
}
