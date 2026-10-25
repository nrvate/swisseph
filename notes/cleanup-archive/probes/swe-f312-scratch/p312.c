/* probe for bug 312: no-Earth paths past Vondrak's precession range */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define J2K 2451545.0
static const char *EPHE;

static void star(swe_ctx *c, const char *nm, double t, int32 fl, int which, const char *lab)
{
  char st[AS_MAXCH], serr[AS_MAXCH] = "";
  double x[6];
  int32 r;
  strcpy(st, nm);
  memset(x, 0, sizeof x);
  r = which ? swe_fixstar2_r(c, st, t, fl, x, serr) : swe_fixstar_r(c, st, t, fl, x, serr);
  printf("%-28s %s t=%-12.6g fl=%7d r=%7d x=%.10g %.10g %.6g sp=%.6g '%s'\n", lab, which ? "fs2" : "fs ",
         t, fl, r, x[0], x[1], x[2], x[3], serr);
}

int main(int argc, char **argv)
{
  double ts[] = { 1e9, 1e11, -1e9, J2K + 2000 * 36525.0, J2K + 2001 * 36525.0, J2K - 2001 * 36525.0,
                  J2K - 2000 * 36525.0, 5e7 };
  int i;
  EPHE = argc > 1 ? argv[1] : "ephe";
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, (char *) EPHE);
  for (i = 0; i < 8; i++) {
    double t = ts[i];
    printf("---- t = %.17g (T %.6g cy)\n", t, (t - J2K) / 36525);
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_NONUT, 1, "bary mosh nonut");
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_NONUT | SEFLG_SPEED, 1, "bary mosh nonut spd");
    star(c, "Spica", t, SEFLG_SWIEPH | SEFLG_BARYCTR | SEFLG_NONUT | SEFLG_EQUATORIAL, 1, "bary swi nonut equ");
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_J2000, 1, "bary mosh j2000");
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_J2000 | SEFLG_ICRS, 1, "bary mosh j2000 icrs");
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_NONUT | SEFLG_ICRS, 1, "bary mosh nonut icrs");
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_NONUT, 0, "helio mosh nonut (fs)");
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_NONUT | SEFLG_SPEED, 1, "helio mosh nonut spd");
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_NONUT, 0, "bary mosh nonut (fs)");
    star(c, "Spica", t, SEFLG_JPLEPH | SEFLG_BARYCTR | SEFLG_NONUT, 1, "bary jpl nonut");
    star(c, "Spica", t, SEFLG_JPLEPH | SEFLG_BARYCTR | SEFLG_NONUT | SEFLG_JPLHOR_APPROX, 1, "bary jpl nonut horA");
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_SIDEREAL, 1, "bary mosh sid lahiri");
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI | SE_SIDBIT_ECL_T0, 0, 0);
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_SIDEREAL, 1, "bary mosh sid ecl_t0");
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, 0, 0);
    star(c, "Spica", t, SEFLG_MOSEPH | SEFLG_BARYCTR | SEFLG_SIDEREAL, 1, "bary mosh sid ssy");
    swe_set_sid_mode_r(c, SE_SIDM_FAGAN_BRADLEY, 0, 0);
    {
      double x[6];
      char serr[AS_MAXCH] = "";
      int32 r;
      r = swe_calc_r(c, t, SE_FICT_OFFSET, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_NONUT, x, serr);
      printf("%-28s t=%-12.6g r=%d x=%.10g '%s'\n", "cupido helio mosh nonut", t, r, x[0], serr);
      serr[0] = 0;
      r = swe_calc_r(c, t, SE_FICT_OFFSET, SEFLG_SWIEPH | SEFLG_BARYCTR | SEFLG_NONUT, x, serr);
      printf("%-28s t=%-12.6g r=%d x=%.10g '%s'\n", "cupido bary swi nonut", t, r, x[0], serr);
      serr[0] = 0;
      r = swe_calc_r(c, t, SE_MARS, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_NONUT, x, serr);
      printf("%-28s t=%-12.6g r=%d x=%.10g '%s'\n", "mars helio mosh nonut", t, r, x[0], serr);
      serr[0] = 0;
      r = swe_calc_r(c, t, SE_MEAN_NODE, SEFLG_MOSEPH | SEFLG_NONUT, x, serr);
      printf("%-28s t=%-12.6g r=%d x=%.10g '%s'\n", "mean node mosh nonut", t, r, x[0], serr);
      serr[0] = 0;
      {
        double el[6] = { 10, 1.5, 0.1, 20, 30, 5 };
        r = swe_calc_orbel_r(c, t, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_NONUT, x, serr);
        printf("%-28s t=%-12.6g r=%d x=%.10g '%s'\n", "orbel at t helio", t, r, x[0], serr);
        serr[0] = 0;
        r = swe_calc_orbel_r(c, J2K, el, SE_SUN, SE_ORBEL_EQ_EXPLICIT, t, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_NONUT, x, serr);
        printf("%-28s t=%-12.6g r=%d x=%.10g '%s'\n", "orbel equinox t helio", t, r, x[0], serr);
        serr[0] = 0;
        r = swe_calc_orbel_r(c, J2K, el, SE_SUN, SE_ORBEL_EQ_EXPLICIT, t, SEFLG_MOSEPH | SEFLG_SPEED, x, serr);
        printf("%-28s t=%-12.6g r=%d x=%.10g sp=%.6g '%s'\n", "orbel equinox t geo", t, r, x[0], x[3], serr);
      }
    }
  }
  swe_ctx_free(c);
  return 0;
}
