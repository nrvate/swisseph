/* m292: what the functions that take a body answer for the Earth, SE_ECL_NUT
 * and invalid numbers */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

static double ZUR[3] = { 8.55, 47.37, 400 };
#define T 2433282.5

static const int IPL[] = { SE_EARTH, SE_ECL_NUT, -2, -100, SE_NPLANETS, 30, 39, 1000, 8999,
                           SE_AST_OFFSET, SE_FICT_OFFSET, SE_MOON, SE_SUN, SE_MEAN_NODE, SE_INTP_PERG };
#define NIPL ((int) (sizeof IPL / sizeof IPL[0]))

static void show(const char *what, int ipl, int32 r, const double *v, int nv, const char *serr)
{
  int k;
  printf("%-26s ipl %6d: ret %6d", what, ipl, r);
  for (k = 0; k < nv; k++)
    printf(" %.10g", v[k]);
  printf("  [%.70s]\n", serr);
}

int main(int argc, char **argv)
{
  int i, m;
  const char *eph = argc > 1 ? argv[1] : "ephe";
  int only = argc > 2 ? atoi(argv[2]) : -1;
  setvbuf(stdout, NULL, _IOLBF, 0);
  if (getenv("ALRM")) alarm(atoi(getenv("ALRM")));
  swe_set_ephe_path(eph);
  for (i = 0; i < NIPL; i++) {
    int ipl = IPL[i];
    double tret[20], attr[40], v[20], s, x[24], te = T + swe_deltat_ex(T, SEFLG_SWIEPH, NULL);
    double geo[20];
    char serr[256];
    if (only >= 0 && only != i)
      continue;
#define RUN(name, expr, arr, nv) do { memset(arr, 0, sizeof arr); serr[0] = 0; memcpy(geo, ZUR, sizeof ZUR); \
      { int32 r_ = (expr); show(name, ipl, r_, arr, nv, serr); } } while (0)
    RUN("rise fast", swe_rise_trans(T, ipl, NULL, SEFLG_SWIEPH, SE_CALC_RISE, geo, 0, 0, tret, serr), tret, 1);
    RUN("rise slow", swe_rise_trans(T, ipl, NULL, SEFLG_SWIEPH, SE_CALC_RISE | SE_BIT_FORCE_SLOW_METHOD, geo, 0, 0, tret, serr), tret, 1);
    RUN("set slow", swe_rise_trans(T, ipl, NULL, SEFLG_SWIEPH, SE_CALC_SET, geo, 0, 0, tret, serr), tret, 1);
    RUN("mtransit", swe_rise_trans(T, ipl, NULL, SEFLG_SWIEPH, SE_CALC_MTRANSIT, geo, 0, 0, tret, serr), tret, 1);
    RUN("itransit", swe_rise_trans(T, ipl, NULL, SEFLG_SWIEPH, SE_CALC_ITRANSIT, geo, 0, 0, tret, serr), tret, 1);
    RUN("rise hindu", swe_rise_trans(T, ipl, NULL, SEFLG_SWIEPH, SE_CALC_RISE | SE_BIT_HINDU_RISING, geo, 0, 0, tret, serr), tret, 1);
    RUN("true_hor rise", swe_rise_trans_true_hor(T, ipl, NULL, SEFLG_SWIEPH, SE_CALC_RISE, geo, 0, 0, 2, tret, serr), tret, 1);
    for (m = 0; m < 6; m++) {
      char nm[32];
      snprintf(nm, sizeof nm, "gauquelin m%d", m);
      memset(v, 0, sizeof v);
      serr[0] = 0;
      memcpy(geo, ZUR, sizeof ZUR);
      { int32 r = swe_gauquelin_sector(T, ipl, NULL, SEFLG_SWIEPH, m, geo, 0, 0, &s, serr); v[0] = s; show(nm, ipl, r, v, 1, serr); }
    }
    RUN("occult_when_glob", swe_lun_occult_when_glob(T, ipl, NULL, SEFLG_SWIEPH, 0, tret, 0, serr), tret, 2);
    RUN("occult_when_loc", swe_lun_occult_when_loc(T, ipl, NULL, SEFLG_SWIEPH, geo, tret, attr, 0, serr), tret, 2);
    RUN("occult_where", swe_lun_occult_where(T, ipl, NULL, SEFLG_SWIEPH, geo, attr, serr), geo, 2);
    RUN("pheno", swe_pheno(te, ipl, SEFLG_SWIEPH, attr, serr), attr, 5);
    RUN("pheno topo", (swe_set_topo(8.55, 47.37, 400), swe_pheno(te, ipl, SEFLG_SWIEPH | SEFLG_TOPOCTR, attr, serr)), attr, 5);
    RUN("nod_aps geo m0", swe_nod_aps(te, ipl, SEFLG_SWIEPH, 0, x, x + 6, x + 12, x + 18, serr), x, 13);
    RUN("nod_aps geo osc", swe_nod_aps(te, ipl, SEFLG_SWIEPH, SE_NODBIT_OSCU, x, x + 6, x + 12, x + 18, serr), x, 13);
    RUN("nod_aps hel osc", swe_nod_aps(te, ipl, SEFLG_SWIEPH | SEFLG_HELCTR, SE_NODBIT_OSCU, x, x + 6, x + 12, x + 18, serr), x, 13);
    RUN("orbital_elements", swe_get_orbital_elements(te, ipl, SEFLG_SWIEPH, v, serr), v, 3);
    RUN("orbit_max_min", swe_orbit_max_min_true_distance(te, ipl, SEFLG_SWIEPH, v, v + 1, v + 2, serr), v, 3);
    if (!getenv("SKIPHC") || ipl != SE_ECL_NUT) RUN("helio_cross", swe_helio_cross(ipl, 100, te, SEFLG_SWIEPH, 0, v, serr), v, 1);
    RUN("calc_pctr sun", swe_calc_pctr(te, ipl, SE_SUN, SEFLG_SWIEPH, v, serr), v, 3);
    RUN("calc", swe_calc(te, ipl, SEFLG_SWIEPH, v, serr), v, 3);
    RUN("calc topo", swe_calc(te, ipl, SEFLG_SWIEPH | SEFLG_TOPOCTR, v, serr), v, 3);
  }
  return 0;
}
