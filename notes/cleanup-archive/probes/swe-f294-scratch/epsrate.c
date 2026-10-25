#include <math.h>
#include <stdio.h>
#include "swephexp.h"
static double eps(double t) { double x[6]; swe_calc(t, SE_ECL_NUT, SEFLG_SWIEPH, x, NULL); return x[0]; }
int main(void)
{
  int i; double w = 0, wr = 0;
  swe_set_ephe_path("/nvm/work/swe-f294/ephe");
  for (i = 0; i < 4000; i++) {
    double t = 2378496.5 + i * 27.3917, h = 1.0/256, h2 = 1.0/64;
    double c = (eps(t + h) - eps(t - h)) / (2*h);
    /* five-point over 2^-6 as the reference */
    double r = (8*(eps(t+h2)-eps(t-h2)) - (eps(t+2*h2)-eps(t-2*h2)))/(12*h2);
    if (fabs(c - r) > w) w = fabs(c - r);
    if (fabs(c) > wr) wr = fabs(c);
  }
  printf("central 2^-8 vs five-point 2^-6: %.3g deg/day; |eps'| max %.3g\n", w, wr);
  return 0;
}
