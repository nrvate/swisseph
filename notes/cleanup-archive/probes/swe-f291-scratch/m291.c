/* m291: after each geopos function at Zurich, does the caller's observer
 * (70 W 30 S, or unset) answer as before? */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

static const char *EPH;
static double ZUR[3] = { 8.55, 47.37, 400 };
static double FAR[3] = { -70, -30, 0 };
#define T 2433282.5

static swe_ctx *open_ctx(int set)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, EPH);
  swe_set_jpl_file_r(c, "de200.eph");
  if (set)
    swe_set_topo_r(c, FAR[0], FAR[1], FAR[2]);
  return c;
}

static int32 moon(swe_ctx *c, double *x, char *serr)
{
  double te;
  if (c == NULL) {
    te = T + swe_deltat_ex(T, SEFLG_SWIEPH, NULL);
    return swe_calc(te, SE_MOON, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr);
  }
  te = T + swe_deltat_ex_r(c, T, SEFLG_SWIEPH, NULL);
  return swe_calc_r(c, te, SE_MOON, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED, x, serr);
}

static const char *NAMES[] = {
  "rise_trans Moon rise (fast)", "rise_trans Moon rise slow", "rise_trans Moon mtransit",
  "rise_trans_true_hor Sun set", "sol_eclipse_how", "sol_eclipse_when_loc",
  "sol_eclipse_where", "sol_eclipse_when_glob", "lun_occult_when_loc Regulus",
  "lun_occult_where Regulus", "lun_occult_when_glob Regulus", "lun_eclipse_how",
  "lun_eclipse_when_loc", "lun_eclipse_when", "gauquelin m2", "gauquelin m4",
  "gauquelin m0 topo", "vis_limit_mag venus", "heliacal_pheno_ut venus",
  "heliacal_ut venus", "topo_arcus_visionis", "heliacal_angle", "azalt",
  "lun_occult_when_loc Venus",
};
#define NF ((int) (sizeof NAMES / sizeof NAMES[0]))

static int32 call(swe_ctx *c, int k, char *serr)
{
  double tret[20], attr[40], geo[20], s, datm[4] = { 1013.25, 15, 40, 0 },
    dobs[6] = { 36, 1, 0, 0, 0, 0 }, dret[50], x[6] = { 100, 10, 1 }, xa[3];
  char star[64] = "Regulus", obj[64] = "venus";
  memcpy(geo, ZUR, sizeof ZUR);
  switch (k) {
  case 0: return swe_rise_trans_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH, SE_CALC_RISE, geo, 0, 0, tret, serr);
  case 1: return swe_rise_trans_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH, SE_CALC_RISE | SE_BIT_FORCE_SLOW_METHOD, geo, 0, 0, tret, serr);
  case 2: return swe_rise_trans_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH, SE_CALC_MTRANSIT, geo, 0, 0, tret, serr);
  case 3: return swe_rise_trans_true_hor_r(c, T, SE_SUN, NULL, SEFLG_SWIEPH, SE_CALC_SET, geo, 0, 0, 1, tret, serr);
  case 4: return swe_sol_eclipse_how_r(c, 2451401.9375, SEFLG_SWIEPH, geo, attr, serr);
  case 5: return swe_sol_eclipse_when_loc_r(c, T, SEFLG_SWIEPH, geo, tret, attr, 0, serr);
  case 6: return swe_sol_eclipse_where_r(c, 2451401.9375, SEFLG_SWIEPH, geo, attr, serr);
  case 7: return swe_sol_eclipse_when_glob_r(c, T, SEFLG_SWIEPH, 0, tret, 0, serr);
  case 8: return swe_lun_occult_when_loc_r(c, T, 0, star, SEFLG_SWIEPH, geo, tret, attr, 0, serr);
  case 9: return swe_lun_occult_where_r(c, T, 0, star, SEFLG_SWIEPH, geo, attr, serr);
  case 10: return swe_lun_occult_when_glob_r(c, T, 0, star, SEFLG_SWIEPH, 0, tret, 0, serr);
  case 11: return swe_lun_eclipse_how_r(c, T, SEFLG_SWIEPH, geo, attr, serr);
  case 12: return swe_lun_eclipse_when_loc_r(c, T, SEFLG_SWIEPH, geo, tret, attr, 0, serr);
  case 13: return swe_lun_eclipse_when_r(c, T, SEFLG_SWIEPH, 0, tret, 0, serr);
  case 14: return swe_gauquelin_sector_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH, 2, geo, 0, 0, &s, serr);
  case 15: return swe_gauquelin_sector_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH, 4, geo, 0, 0, &s, serr);
  case 16: return swe_gauquelin_sector_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH | SEFLG_TOPOCTR, 0, geo, 0, 0, &s, serr);
  case 17: return swe_vis_limit_mag_r(c, T + 0.75, geo, datm, dobs, obj, SEFLG_SWIEPH, dret, serr);
  case 18: return swe_heliacal_pheno_ut_r(c, T + 0.75, geo, datm, dobs, obj, 1, SEFLG_SWIEPH, dret, serr);
  case 19: return swe_heliacal_ut_r(c, T, geo, datm, dobs, obj, 1, SEFLG_SWIEPH, dret, serr);
  case 20: return swe_topo_arcus_visionis_r(c, T, geo, datm, dobs, SEFLG_SWIEPH, -4, 100, 10, 90, 0, -10, dret, serr);
  case 21: return swe_heliacal_angle_r(c, T, geo, datm, dobs, SEFLG_SWIEPH, -4, 100, 90, 0, -10, dret, serr);
  case 22: swe_azalt_r(c, T, SE_ECL2HOR, geo, 0, 0, x, xa); swe_azalt_rev_r(c, T, SE_HOR2ECL, geo, xa, x); return 0;
  case 23: obj[0] = 0; return swe_lun_occult_when_loc_r(c, T, SE_VENUS, NULL, SEFLG_SWIEPH, geo, tret, attr, 0, serr);
  }
  return -99;
}

int main(int argc, char **argv)
{
  int k, nbad = 0;
  EPH = argc > 1 ? argv[1] : "ephe";
  for (k = 0; k < NF; k++) {
    swe_ctx *c = open_ctx(1), *u = open_ctx(0);
    double x0[6], x1[6], xu[6];
    char serr[256] = "", se2[256] = "";
    int32 r0, r1, rf, ru;
    r0 = moon(c, x0, serr);
    rf = call(c, k, serr);
    r1 = moon(c, x1, serr);
    rf = call(u, k, se2);
    se2[0] = 0;
    ru = moon(u, xu, se2);
    printf("%-30s ret %3d: far %s (%.4f -> %.4f, %.3g)  unset %s%s\n", NAMES[k], rf,
           r0 == r1 && memcmp(x0, x1, sizeof x0) == 0 ? "same" : "MOVED", x0[0], x1[0],
           x1[0] - x0[0], ru < 0 ? "refused" : "ANSWERS", ru < 0 ? "" : "");
    if (!(r0 == r1 && memcmp(x0, x1, sizeof x0) == 0) || ru >= 0)
      nbad++;
    swe_ctx_free(c);
    swe_ctx_free(u);
  }
  /* the default context: at 70 W 30 S, rise_trans at Zurich */
  {
    double x0[6], x1[6], tret[10];
    char serr[256];
    swe_set_ephe_path(EPH);
    swe_set_topo(FAR[0], FAR[1], FAR[2]);
    moon(NULL, x0, serr);
    swe_rise_trans(T, SE_MOON, NULL, SEFLG_SWIEPH, SE_CALC_RISE, ZUR, 0, 0, tret, serr);
    moon(NULL, x1, serr);
    printf("default ctx rise_trans: %.4f -> %.4f %s\n", x0[0], x1[0],
           memcmp(x0, x1, sizeof x0) == 0 ? "same" : "MOVED");
  }
  printf("%d of %d leave a moved or answering observer\n", nbad, NF);
  return 0;
}
