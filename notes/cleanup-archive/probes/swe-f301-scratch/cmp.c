/* cmp DIR MODE: many swe_calc answers, printed bit for bit */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"
static unsigned long long st = 88172645463325252ULL;
static double rnd(void) { st ^= st << 13; st ^= st >> 7; st ^= st << 17; return (st >> 11) * (1.0 / 9007199254740992.0); }
int main(int argc, char **argv)
{
  static const int bodies[] = {SE_SUN, SE_MOON, SE_MERCURY, SE_MARS, SE_JUPITER, SE_PLUTO, SE_TRUE_NODE, SE_OSCU_APOG, SE_MEAN_NODE, SE_CERES, SE_CUPIDO, SE_EARTH};
  static const int32 fls[] = {0, SEFLG_SPEED, SEFLG_SPEED | SEFLG_TOPOCTR, SEFLG_SPEED | SEFLG_HELCTR, SEFLG_SPEED | SEFLG_BARYCTR,
    SEFLG_SPEED | SEFLG_EQUATORIAL, SEFLG_J2000 | SEFLG_SPEED, SEFLG_NOABERR | SEFLG_SPEED, SEFLG_TRUEPOS | SEFLG_SPEED, SEFLG_SPEED3, SEFLG_HELCTR, SEFLG_BARYCTR, SEFLG_TOPOCTR};
  static const int32 ephs[] = {SEFLG_JPLEPH, SEFLG_SWIEPH, SEFLG_MOSEPH};
  int n = atoi(argv[3]);
  swe_set_ephe_path(argv[1]);
  if (!getenv("NOJF")) swe_set_jpl_file("de200.eph");
  swe_set_ephe_fallback(atoi(argv[2]));
  swe_set_topo(8.55, 47.37, 400);
  for (int k = 0; k < n; k++) {
    double u = rnd(), jd;
    int r = (int) (rnd() * 4);
    if (r == 0) jd = 2305424.5 + u * (2513392.5 - 2305424.5);      /* de200 */
    else if (r == 1) jd = 2378496.5 + u * 1000;                       /* near 1800 */
    else if (r == 2) jd = 625000.2 + u * 2193000.6;                   /* Moshier's */
    else jd = 2451545.0 + (u - 0.5) * 100000;
    int ipl = bodies[(int) (rnd() * 12)];
    int32 fl = fls[(int) (rnd() * 13)] | ephs[(int) (rnd() * 3)];
    if (ipl == SE_SUN && (fl & SEFLG_BARYCTR) && (fl & SEFLG_JPLEPH) && (fl & (SEFLG_SPEED | SEFLG_SPEED3)) && getenv("NOJPL")) continue;
    double x[6] = {0}; char serr[AS_MAXCH] = "";
    int32 ret = swe_calc(jd, ipl, fl, x, serr);
    printf("%d %d %d %a %a %a %a %a %a %a|%s\n", ipl, fl, ret, jd, x[0], x[1], x[2], x[3], x[4], x[5], serr);
  }
  return 0;
}
