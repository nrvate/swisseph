/* obliq.c -- G28: the mean obliquity of date under Vondrák 2011 is the angle
 * between the model's own mean ecliptic pole and mean equator pole.
 *
 * swi_epsiln() used the paper's separately fitted epsilon_A series instead.
 * The two agree within 0.02" over 1000-2650 and part by 1" at -1000, 17.6" at
 * -5000 and 46" at -13000, and the series puts the ecliptic pole of date off
 * latitude 90 in the model's own frame. The ephemeris protocol's 3.5a defines
 * the obliquity as the pole angle (the Astrolog and Ephemeris Prometheia
 * projects, 2026-09-29).
 *
 * The expected values are this library's pole angle, and they are REFEREED:
 * their differences from the series (-1.0391" at -1000, -17.6048" at -5000,
 * -45.9029" at -13000, +3.5722" at 7000, +64.5042" at 15000, 0.0173" at 2650)
 * match the Ephemeris Prometheia project's independently computed ones
 * (-1.0, -17.6, -46, +3.6, +65, <=0.02") to their printed precision. At J2000
 * the value is the model's defined 84381.406". The compat build, which keeps
 * the series, fails this at all eight epochs -- at J2000 by only 0.000002",
 * the two constructions' rounding apart, which the 1e-6" tolerance sees.
 *
 * Moshier, no files. Run as
 *   obliq
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static const struct { double jd, eps; const char *sz; } rgCase[] = {
    {2451545.0, 23.439279444722231, "J2000"},
    {1356173.5, 23.814190795614632, "-1000"},
    {-105192.5, 24.161108417655633, "-5000"},
    {-3027205.0, 23.870771747108989, "-13000"},
    {4278044.5, 22.855286535818262, "7000"},
    {7199795.0, 22.691677907127449, "15000"},
    {1721423.5, 23.694842042557877, "+1"},
    {2689465.5, 23.354687386447722, "2650"},
};

int main(void)
{
  char serr[256];
  double x[6];
  int i, bad = 0, n = (int)(sizeof(rgCase) / sizeof(rgCase[0]));

  printf("G28: the Vondrak 2011 mean obliquity is the pole angle\n");
  for (i = 0; i < n; i++) {
    if (swe_calc(rgCase[i].jd, SE_ECL_NUT, SEFLG_MOSEPH, x, serr) < 0) {
      printf("  %-7s ERROR %s\n", rgCase[i].sz, serr);
      bad++;
      continue;
    }
    double d = (x[1] - rgCase[i].eps) * 3600.0;
    printf("  %-7s mean eps %.9f deg, %+.6f\" from expected\n", rgCase[i].sz,
           x[1], d);
    if (fabs(d) > 1e-6) bad++;
  }
  printf(bad ? "G28 FAIL: %d\n" : "G28 PASS\n", bad);
  return bad ? 1 : 0;
}
