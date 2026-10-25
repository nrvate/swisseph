#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#define J2K 2451545.0
int main(int argc, char **argv)
{
  swe_ctx *c = swe_ctx_new();
  double x[6], t0s[] = { 1e9, J2K + 2001 * 36525.0, J2K + 1999 * 36525.0, 1e7 };
  char serr[AS_MAXCH], st[64];
  int i, k; int32 r;
  swe_set_ephe_path_r(c, argv[1]);
  for (i = 0; i < 4; i++) {
    int32 bits[3] = { 0, SE_SIDBIT_ECL_T0, SE_SIDBIT_SSY_PLANE };
    for (k = 0; k < 3; k++) {
      swe_set_sid_mode_r(c, SE_SIDM_USER | bits[k], t0s[i], 0);
      serr[0] = 0; memset(x, 0, sizeof x);
      r = swe_calc_r(c, J2K, SE_MARS, SEFLG_SWIEPH | SEFLG_SIDEREAL | SEFLG_SPEED, x, serr);
      printf("t0=%-12.6g bits=%d mars r=%d x=%.10g %.10g '%s'\n", t0s[i], bits[k], r, x[0], x[3], serr);
      strcpy(st, "Spica"); serr[0] = 0; memset(x, 0, sizeof x);
      r = swe_fixstar2_r(c, st, J2K, SEFLG_SWIEPH | SEFLG_SIDEREAL, x, serr);
      printf("t0=%-12.6g bits=%d spica r=%d x=%.10g '%s'\n", t0s[i], bits[k], r, x[0], serr);
      serr[0] = 0; memset(x, 0, sizeof x);
      { double cu[13], a[10];
      r = swe_houses_ex_r(c, J2K, SEFLG_SIDEREAL, 47, 8, 'P', cu, a);
      printf("t0=%-12.6g bits=%d houses r=%d asc=%.10g\n", t0s[i], bits[k], r, a[0]); }
    }
  }
  /* polynomial long-term model: orbel explicit equinox past 17191 */
  {
    char mod[40] = "0,8";
    double el[6] = { 10, 1.5, 0.1, 20, 30, 5 };
    swe_set_astro_models_r(c, mod, 0);
    for (i = 0; i < 3; i++) {
      double te = i == 0 ? 8000016.5 : i == 1 ? 8000017.5 : 1e7;
      serr[0] = 0;
      r = swe_calc_orbel_r(c, J2K, el, SE_SUN, SE_ORBEL_EQ_EXPLICIT, te, SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_NONUT, x, serr);
      printf("model 0,8 orbel equinox %.10g r=%d x=%.10g '%s'\n", te, r, x[0], serr);
    }
  }
  swe_ctx_free(c);
  return 0;
}
