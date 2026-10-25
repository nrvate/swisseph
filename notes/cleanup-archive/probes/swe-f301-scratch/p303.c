/* p303 DIR FB: fresh-process answers, plain vs flagged, serr */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"
static const char *dir; static int fb;
static void one(int kind, int ipl, int32 fl, double jd, int method)
{
  int fd[2]; char buf[2048]; ssize_t n;
  pipe(fd); fflush(stdout);
  if (fork() == 0) {
    char serr[AS_MAXCH] = ""; double x[6] = {0}, xa[6], xp[6], xq[6]; int32 r;
    close(fd[0]);
    swe_set_ephe_path(dir); swe_set_ephe_fallback(fb);
    if (kind == 0) r = swe_calc(jd, ipl, fl, x, serr);
    else if (kind == 1) r = swe_calc_ut(jd, ipl, fl, x, serr);
    else { r = swe_nod_aps(jd, ipl, fl, method, x, xa, xp, xq, serr); }
    snprintf(buf, sizeof buf, "r %d lon %.13f spd %.10g serr '%s'", r, x[0], x[3], serr);
    for (char *p = buf; *p; p++) if (*p == '\n') *p = '|';
    write(fd[1], buf, strlen(buf) + 1); _exit(0);
  }
  close(fd[1]); n = read(fd[0], buf, sizeof buf - 1); buf[n > 0 ? n : 0] = 0; close(fd[0]); wait(NULL);
  printf("  %s ipl %d fl %d m %d jd %.6f: %s\n", kind == 0 ? "calc" : kind == 1 ? "calc_ut" : "nod_aps", ipl, fl, method, jd, buf);
}
int main(int argc, char **argv)
{
  dir = argv[1]; fb = atoi(argv[2]);
  double jd = argc > 3 ? atof(argv[3]) : 2451545.0;
  int ipls[] = {SE_TRUE_NODE, SE_OSCU_APOG, SE_CHIRON, SE_CERES, SE_MARS};
  for (int i = 0; i < 5; i++) {
    one(0, ipls[i], SEFLG_JPLEPH, jd, 0);
    one(0, ipls[i], SEFLG_JPLEPH | SEFLG_SPEED, jd, 0);
    one(0, ipls[i], SEFLG_JPLEPH | SEFLG_SPEED3, jd, 0);
  }
  int np[] = {SE_MARS, SE_MOON, SE_SUN, SE_CHIRON};
  for (int i = 0; i < 4; i++)
    for (int m = 1; m <= 2; m++) {
      one(2, np[i], SEFLG_JPLEPH, jd, m);
      one(2, np[i], SEFLG_JPLEPH | SEFLG_SPEED, jd, m);
    }
  return 0;
}
