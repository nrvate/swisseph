/* s301 DIR FB T0: fresh-process answers at T0 + offsets for many bodies/flags; print ret + serr class */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  const char *dir = argv[1]; int fb = atoi(argv[2]); double t0 = atof(argv[3]);
  static const int bodies[] = {SE_SUN, SE_MOON, SE_MERCURY, SE_MARS, SE_JUPITER, SE_PLUTO, SE_CHIRON, SE_CERES, SE_CUPIDO, SE_WALDEMATH, SE_TRUE_NODE, SE_OSCU_APOG, SE_EARTH};
  static const int32 fls[] = {0, SEFLG_SPEED, SEFLG_TOPOCTR, SEFLG_TOPOCTR|SEFLG_SPEED, SEFLG_HELCTR, SEFLG_HELCTR|SEFLG_SPEED, SEFLG_BARYCTR|SEFLG_SPEED, SEFLG_SPEED3, SEFLG_EQUATORIAL|SEFLG_SPEED};
  static const double off[] = {1.0/1048576, 1.0/1024, 0.001, 1.0/64, 1.0/8, 0.5};
  for (unsigned ib = 0; ib < sizeof bodies / sizeof *bodies; ib++)
    for (unsigned jf = 0; jf < sizeof fls / sizeof *fls; jf++)
      for (unsigned ko = 0; ko < sizeof off / sizeof *off; ko++) {
        int fd[2]; char buf[1200]; ssize_t n;
        double jd = t0 + off[ko];
        if (pipe(fd)) return 1; fflush(stdout);
        if (fork() == 0) {
          char serr[AS_MAXCH] = ""; double x[6] = {0};
          close(fd[0]);
          swe_set_ephe_path(dir); swe_set_ephe_fallback(fb); swe_set_topo(8.55, 47.37, 400);
          int32 r = swe_calc(jd, bodies[ib], fls[jf] | SEFLG_SWIEPH, x, serr);
          snprintf(buf, sizeof buf, "%d %a %a %a %a|%s", r, x[0], x[1], x[2], x[3], serr);
          for (char *p = buf; *p; p++) if (*p == '\n') *p = '~';
          if (write(fd[1], buf, strlen(buf) + 1) < 0) _exit(1);
          _exit(0);
        }
        close(fd[1]); n = read(fd[0], buf, sizeof buf - 1); buf[n > 0 ? n : 0] = 0; close(fd[0]); wait(NULL);
        printf("%d %d %.10f %s\n", bodies[ib], fls[jf], off[ko], buf);
      }
  return 0;
}
