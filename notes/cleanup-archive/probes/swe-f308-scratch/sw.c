#include <stdio.h>
#include <math.h>
#include "swephexp.h"
/* find the forward function's switch from UTC to UT1 mode */
int main(void){
  double d[2]; char serr[256];
  swe_set_ephe_path("/nvm/work/swe-f308/ephe");
  double j0 = swe_julday(2017,1,1,0,1);
  int prev = -1;
  for (double j = j0; j < j0 + 365*200; j += 1) {
    int y,m,dd; double h;
    swe_revjul(j, 1, &y,&m,&dd,&h);
    swe_utc_to_jd(y,m,dd,0,0,0,1,d,serr);
    int mode = fabs(d[1] - j) < 1e-12;  /* UT1 mode: UT1 == input */
    if (mode != prev) { printf("%d-%d-%d mode %s UT1-UTC %.6f s dt %.6f s\n", y,m,dd, mode?"UT1":"UTC", (d[1]-j)*86400, swe_deltat_ex(j,-1,NULL)*86400); prev = mode; }
  }
  return 0;
}
