#include <stdio.h>
#include <math.h>
#include <string.h>
#include "swephexp.h"
static double jd(int32 *v, double s){ return swe_julday(v[0], v[1], v[2], v[3] + v[4]/60.0 + s/3600.0, SE_GREG_CAL); }
static void rt(int y, int m, int d, int h, int mi, double sec){
  double dr[2]; char serr[AS_MAXCH]; int32 v[5], w[5]; double s, s2;
  if (swe_utc_to_jd(y,m,d,h,mi,sec,SE_GREG_CAL,dr,serr) != OK) { printf("%d-%d-%d fwd ERR %s\n", y,m,d,serr); return; }
  swe_jdut1_to_utc(dr[1], SE_GREG_CAL, &v[0],&v[1],&v[2],&v[3],&v[4],&s);
  swe_jdet_to_utc(dr[0], SE_GREG_CAL, &w[0],&w[1],&w[2],&w[3],&w[4],&s2);
  int32 in[5] = {y,m,d,h,mi};
  double j = jd(in, sec);
  printf("%d-%02d-%02d %02d:%02d:%09.6f  ut1->  %d-%02d-%02d %02d:%02d:%09.6f (%.3g s)   tt->  %d-%02d-%02d %02d:%02d:%09.6f (%.3g s)\n",
    y,m,d,h,mi,sec, v[0],v[1],v[2],v[3],v[4],s,(jd(v,s)-j)*86400, w[0],w[1],w[2],w[3],w[4],s2,(jd(w,s2)-j)*86400);
}
int main(void){
  swe_set_ephe_path("/nvm/work/swe-f308/ephe");
  rt(1971,12,31,23,59,59.9); rt(1972,1,1,0,0,0); rt(1972,1,1,0,0,0.5);
  rt(2016,12,31,23,59,60.5); rt(2017,1,1,0,0,0.5);
  rt(2033,9,16,0,0,0); rt(2033,9,16,12,0,0); rt(2033,9,16,23,59,59); rt(2033,9,16,23,59,59.9); rt(2033,9,17,0,0,0); rt(2033,9,17,0,0,0.5); rt(2033,9,17,12,0,0); rt(2033,9,18,0,0,0);
  rt(100000,1,1,12,0,0); rt(-100000,1,1,12,0,0); rt(-1000000,1,1,12,0,0);
  return 0;
}
