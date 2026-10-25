#include <stdio.h>
#include "swephexp.h"
int main(int argc, char **argv)
{
  char serr[AS_MAXCH]; double x[6], s, e; int d;
  swe_set_ephe_path(argv[1]);
  swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
  swe_get_current_file_data(1, &s, &e, &d);
  printf("%.10f %.10f\n", s, e);
  return 0;
}
