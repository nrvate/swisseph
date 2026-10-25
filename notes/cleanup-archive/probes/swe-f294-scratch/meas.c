/* scratch: swetest's cusp RA/dec speeds vs five-point differences */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"
static const char *EPH="/nvm/work/swe-f294/ephe", *SWT;
static double LON=8.55, LAT=47.37;
/* the library's tropical cusp k (1..nc, nc+1.. axes) RA/dec at tu */
static void radec(double tu, int hs, int k, int nc, double *a, double *d, double *lam)
{
  double cu[37], am[10], xo[6], xh[3], xe[3];
  double te = tu + swe_deltat_ex(tu, SEFLG_SWIEPH, NULL);
  swe_houses(tu, LAT, LON, hs, cu, am);
  swe_calc(te, SE_ECL_NUT, SEFLG_SWIEPH, xo, NULL);
  xh[0] = k > nc ? am[k-nc-1] : cu[k]; xh[1]=0; xh[2]=1;
  *lam = xh[0];
  if (k > nc && k-nc-1 == 2) { *a = xh[0]; *d = 0; return; }
  swe_cotrans(xh, xe, -xo[0]);
  *a = xe[0]; *d = xe[1];
}
int main(int argc, char **argv)
{
  char cmd[1024], line[1024];
  const char *HS = "PKGWN";
  int h, i;
  SWT = argv[1];
  swe_set_ephe_path(EPH);
  for (h = 0; HS[h]; h++) {
    int nc = HS[h]=='G'?36:12;
    double w=0, wd=0;
    for (i = 0; i < 3; i++) {
      double tu = 2378496.5 + i*54786.375, hh = 1.0/4096;
      FILE *fp;
      int k = 0;
      snprintf(cmd, sizeof cmd, "%s -edir%s -bj%.6f -ut -p1 -house%.2f,%.2f,%c -fPadSS -ep -head -g,", SWT, EPH, tu, LON, LAT, HS[h]);
      fp = popen(cmd, "r");
      fgets(line, sizeof line, fp); /* Moon */
      while (fgets(line, sizeof line, fp)) {
        double v[4], a[5], d[5], l, ra1, de1; int j; char *p = strchr(line, ',');
        k++;
        for (j = 0; j < 4; j++) { v[j] = strtod(p+1, &p); p = strchr(p, ','); if (j<3 && p && p[1]==',') p++; }
        for (j = -2; j <= 2; j++) radec(tu + j*hh, HS[h], k, nc, &a[j+2], &d[j+2], &l);
        ra1 = (8*swe_difdeg2n(a[3],a[1]) - swe_difdeg2n(a[4],a[0]))/(12*hh);
        de1 = (8*(d[3]-d[1]) - (d[4]-d[0]))/(12*hh);
        if (fabs(v[2]-ra1) > w) { w = fabs(v[2]-ra1); if (w > 1e-8) printf("   worst %c i%d k%d %.9f %.9f\n", HS[h], i, k, v[2], ra1);}
        if (fabs(v[3]-de1) > wd) wd = fabs(v[3]-de1);
        if (HS[h]=='P' && i==1 && (k==1 || k==nc+2 || k==nc+3)) printf("  k %d: %.9f %.9f | %.9f %.9f\n", k, v[2], v[3], ra1, de1);
      }
      pclose(fp);
    }
    printf("%c: RA speed off %.3g, dec speed off %.3g\n", HS[h], w, wd);
  }
  return 0;
}
