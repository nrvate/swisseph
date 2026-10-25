#include <stdio.h>
#include <math.h>
#include <string.h>
#include "swephexp.h"
static void one(int y){
  double d[2]; char serr[AS_MAXCH]; int32 v[5]; double s;
  int r = swe_utc_to_jd(y, 1, 1, 12, 0, 0, SE_GREG_CAL, d, serr);
  if (r != OK) { printf("year %d: forward ERR %s\n", y, serr); return; }
  swe_jdut1_to_utc(d[1], SE_GREG_CAL, &v[0],&v[1],&v[2],&v[3],&v[4],&s);
  double back = swe_julday(v[0], v[1], v[2], v[3] + v[4]/60.0 + s/3600.0, SE_GREG_CAL);
  int32 w[5]; double s2;
  swe_jdet_to_utc(d[0], SE_GREG_CAL, &w[0],&w[1],&w[2],&w[3],&w[4],&s2);
  double back2 = swe_julday(w[0], w[1], w[2], w[3] + w[4]/60.0 + s2/3600.0, SE_GREG_CAL);
  printf("year %11d UT1 %.6f dt %.6g d: ut1->utc %d-%d-%d %d:%d:%.6f off %.6g s; tt->utc %d-%d-%d %d:%d:%.6f off %.6g s\n",
    y, d[1], d[0]-d[1], v[0],v[1],v[2],v[3],v[4],s,(back-d[1])*86400, w[0],w[1],w[2],w[3],w[4],s2,(back2-d[1])*86400);
}
int main(void){
  int ys[] = {2000, 2020, 2030, 2100, 3000, 10000, 100000, 300000, 1000000, 10000000, 100000000, 1000000000, 2000000000, 2147483000,
     -1000, -10000, -100000, -300000, -1000000, -100000000, -1000000000, -2000000000};
  swe_set_ephe_path("/nvm/work/swe-f308/ephe");
  for (unsigned i=0;i<sizeof ys/sizeof ys[0];i++) one(ys[i]);
  return 0;
}
