#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"
static const char *EPH="/nvm/work/swe-f294/ephe";
static double LON=8.55, LAT=47.37;
static int32 FL = SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX;
static void radec(double tu, int hs, int k, int nc, double *a, double *d)
{
  double cu[37], am[10], xo[6], xh[3], xe[3];
  double te = tu + swe_deltat_ex(tu, SEFLG_JPLEPH, NULL);
  swe_houses(tu, LAT, LON, hs, cu, am);
  swe_calc(te, SE_ECL_NUT, FL, xo, NULL);
  xh[0] = k > nc ? am[k-nc-1] : cu[k]; xh[1]=0; xh[2]=1;
  if (k > nc && k-nc-1 == 2) { *a = xh[0]; *d = 0; return; }
  swe_cotrans(xh, xe, -xo[0]);
  *a = xe[0]; *d = xe[1];
}
int main(int argc, char **argv)
{
  char cmd[1024], line[1024];
  int i, s;
  double TU[2] = {2437684.5 - 10.0/4096, 2437684.5 + 6.0/4096};
  swe_set_ephe_path(EPH);
  swe_set_jpl_file("de200.eph");
  for (s = 1; s < argc; s++)
  for (i = 0; i < 2; i++) {
    double tu = TU[i], hh = 1.0/4096, w = 0, te = tu + swe_deltat_ex(tu, SEFLG_JPLEPH, NULL);
    FILE *fp; int k = 0, nc = 12;
    snprintf(cmd, sizeof cmd, "%s -edir%s -ejplde200.eph -jplhora -bj%.10f -ut -p1 -house%.2f,%.2f,P -fPadSS -ep -head -g,", argv[s], EPH, tu, LON, LAT);
    fp = popen(cmd, "r");
    fgets(line, sizeof line, fp);
    while (fgets(line, sizeof line, fp)) {
      double v[4], a[5], d[5], ra1, de1; int j; char *p = strchr(line, ',');
      k++;
      for (j = 0; j < 4; j++) { v[j] = strtod(p+1, &p); p = strchr(p, ','); if (j<3 && p && p[1]==',') p++; }
      for (j = -2; j <= 2; j++) radec(tu + j*hh, 'P', k, nc, &a[j+2], &d[j+2]);
      ra1 = (8*swe_difdeg2n(a[3],a[1]) - swe_difdeg2n(a[4],a[0]))/(12*hh);
      de1 = (8*(d[3]-d[1]) - (d[4]-d[0]))/(12*hh);
      if (fabs(v[2]-ra1) > w) w = fabs(v[2]-ra1);
      if (fabs(v[3]-de1) > w) w = fabs(v[3]-de1);
    }
    pclose(fp);
    printf("%s te-step %+.5f: off %.3g\n", argv[s], te - 2437684.5, w);
  }
  return 0;
}
