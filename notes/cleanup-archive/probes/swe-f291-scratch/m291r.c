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

static double OUT[200];
static int32 call(swe_ctx *c, int k, char *serr)
{
  double *tret = OUT, *attr = OUT + 20, *geo = OUT + 60, *dret = OUT + 80, s, datm[4] = { 1013.25, 15, 40, 0 },
    dobs[6] = { 36, 1, 0, 0, 0, 0 }, x[6] = { 100, 10, 1 }, xa[3];
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
  case 14: { int32 r = swe_gauquelin_sector_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH, 2, geo, 0, 0, &s, serr); OUT[150] = s; return r; }
  case 15: { int32 r = swe_gauquelin_sector_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH, 4, geo, 0, 0, &s, serr); OUT[150] = s; return r; }
  case 16: { int32 r = swe_gauquelin_sector_r(c, T, SE_MOON, NULL, SEFLG_SWIEPH | SEFLG_TOPOCTR, 0, geo, 0, 0, &s, serr); OUT[150] = s; return r; }
  case 17: return swe_vis_limit_mag_r(c, T + 0.75, geo, datm, dobs, obj, SEFLG_SWIEPH, dret, serr);
  case 18: return swe_heliacal_pheno_ut_r(c, T + 0.75, geo, datm, dobs, obj, 1, SEFLG_SWIEPH, dret, serr);
  case 19: return swe_heliacal_ut_r(c, T, geo, datm, dobs, obj, 1, SEFLG_SWIEPH, dret, serr);
  case 20: return swe_topo_arcus_visionis_r(c, T, geo, datm, dobs, SEFLG_SWIEPH, -4, 100, 10, 90, 0, -10, dret, serr);
  case 21: return swe_heliacal_angle_r(c, T, geo, datm, dobs, SEFLG_SWIEPH, -4, 100, 90, 0, -10, dret, serr);
  case 22: swe_azalt_r(c, T, SE_ECL2HOR, geo, 0, 0, x, xa); swe_azalt_rev_r(c, T, SE_HOR2ECL, geo, xa, x); OUT[150] = xa[0]; OUT[151] = x[0]; return 0;
  case 23: obj[0] = 0; return swe_lun_occult_when_loc_r(c, T, SE_VENUS, NULL, SEFLG_SWIEPH, geo, tret, attr, 0, serr);
  }
  return -99;
}


static unsigned long long fnv(const void *p, size_t n, unsigned long long h)
{
  const unsigned char *b = p;
  while (n--) { h ^= *b++; h *= 1099511628211ULL; }
  return h;
}

int main(int argc, char **argv)
{
  int k, s, rep;
  EPH = argc > 1 ? argv[1] : "ephe";
  for (k = 0; k < NF; k++) {
    unsigned long long h[3];
    for (s = 0; s < 3; s++) {
      swe_ctx *c = swe_ctx_new();
      h[s] = 1469598103934665603ULL;
      swe_set_ephe_path_r(c, EPH);
      if (s == 1) swe_set_topo_r(c, ZUR[0], ZUR[1], ZUR[2]);
      if (s == 2) swe_set_topo_r(c, FAR[0], FAR[1], FAR[2]);
      for (rep = 0; rep < 2; rep++) {
        char serr[256];
        int32 r;
        memset(OUT, 0, sizeof OUT);
        memset(serr, 0, sizeof serr);
        r = call(c, k, serr);
        h[s] = fnv(&r, sizeof r, h[s]);
        h[s] = fnv(OUT, sizeof OUT, h[s]);
        h[s] = fnv(serr, sizeof serr, h[s]);
      }
      swe_ctx_free(c);
    }
    printf("%-30s unset %016llx geopos %016llx far %016llx %s\n", NAMES[k], h[0], h[1], h[2],
           h[0] == h[1] && h[1] == h[2] ? "" : "DIFFER");
  }
  return 0;
}
