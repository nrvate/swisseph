#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  /* probe DIR FB IPL FLAGS JD-OFFSET-FROM-SEMO-START [JD absolute if OFF starts with '='] */
  char serr[AS_MAXCH] = "";
  double x[6], s = 0, e = 0;
  int denum;
  swe_set_ephe_path(argv[1]);
  swe_set_jpl_file("de200.eph");
  swe_set_ephe_fallback(atoi(argv[2]));
  int ipl = atoi(argv[3]);
  int32 fl = atoi(argv[4]);
  double jd;
  if (argv[5][0] == '=') jd = atof(argv[5] + 1);
  else {
    /* the Moon file's span from a fork-free way: hardcode via env */
    jd = atof(getenv("T0")) + atof(argv[5]);
  }
  int32 r = swe_calc(jd, ipl, fl, x, serr);
  printf("jd %.10f ipl %d fl %d -> r %d lon %.12f lat %.12f dist %.14f spd %.10g serr '%s'\n", jd, ipl, fl, r, x[0], x[1], x[2], x[3], serr);
  (void)s; (void)e; (void)denum;
  return 0;
}
