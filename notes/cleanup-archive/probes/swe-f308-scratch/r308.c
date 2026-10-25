#include <stdio.h>
#include <string.h>
#include "swephexp.h"
int main(void){
  char serr[AS_MAXCH]; double x[6], xn[6], xs[6], xp[6], xa[6];
  struct { double t; int ipl; } c[] = {
    {8000016.49, SE_MEAN_NODE},{8000016.49, SE_MEAN_APOG},{-3100015.49, SE_MEAN_NODE},{-3100015.49, SE_MEAN_APOG},
    {8000016.5, SE_MEAN_NODE},{8000016.5, SE_MEAN_APOG},{-3100015.5, SE_MEAN_NODE},{-3100015.5, SE_MEAN_APOG},
    {8000016.48, SE_MEAN_NODE},{-3100015.48, SE_MEAN_APOG},
    {8000016.49, SE_INTP_APOG},{8000016.49, SE_INTP_PERG},{-3100015.49, SE_INTP_APOG},{-3100015.49, SE_INTP_PERG},
  };
  swe_set_ephe_path("/nvm/work/swe-f308/ephe");
  for (unsigned i=0;i<sizeof c/sizeof c[0];i++){
    int fl;
    for (fl=0; fl<2; fl++){
      int32 iflag = SEFLG_MOSEPH | (fl?SEFLG_SPEED:0);
      serr[0]=0; memset(x,0,sizeof x);
      int r = swe_calc(c[i].t, c[i].ipl, iflag, x, serr);
      printf("%.6f ipl %d %s r %d x %.10f %.10f spd %.8f  '%s'\n", c[i].t, c[i].ipl, fl?"SPD":"   ", r, x[0], x[1], x[3], serr);
    }
  }
  double tt[] = {8000016.49, -3100015.49, 8000016.5, -3100015.5};
  for (int i=0;i<4;i++) for (int fl=0;fl<2;fl++) for (int m=0;m<2;m++){
    int32 iflag = SEFLG_MOSEPH | (fl?SEFLG_SPEED:0);
    serr[0]=0;
    int r = swe_nod_aps(tt[i], SE_MOON, iflag, m?SE_NODBIT_MEAN:SE_NODBIT_OSCU, xn, xs, xp, xa, serr);
    printf("nod_aps %.6f %s %s r %d %.8f %.8f %.8f '%s'\n", tt[i], fl?"SPD":"   ", m?"mean":"oscu", r, xn[0], xp[0], xn[3], serr);
  }
  return 0;
}
