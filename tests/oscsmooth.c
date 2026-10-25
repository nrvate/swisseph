/* oscsmooth.c -- G94: the osculating apogee and perigee are as smooth as
 * the Moon's state they are found from.
 *
 * The defects (UPSTREAM-BUGS 95): on Moshier the Moon's velocity was a
 * five-point difference of the series over +-2^-5 and +-2^-4 day (bug
 * 51), which divided each sample's last bits by 0.03 day: it jittered by
 * 1e-14..1.2e-13 of itself between instants 2^-30 day apart, against
 * 1e-15..2e-15 on the Swiss files and JPL. The osculating apogee and
 * perigee turn by about 2 dv / (v e), e = 0.055: SE_OSCU_APOG and
 * swe_nod_aps()'s osculating perigee and apogee jittered by up to 3.4e-9
 * deg, their speeds by 1.4e-7 and 5.5e-6 deg/day. And lunar_osc_elem()
 * took the eccentricity as sqrt(1 - p / a), e^2 less a few ulps of 1, and
 * the true anomaly from it (bug 80 fixed the same in swe_nod_aps()):
 * SE_OSCU_APOG 1.1e-11 deg and 5.4e-10 deg/day on the Swiss files, 3.5e-10
 * and 2.4e-8 on de200, where the Moon's state allows 1e-12.
 *
 * The check: on the Swiss files, de200 and Moshier, at eight dates
 * (-2500..2100, de200 1700..2100), binary-fraction instants, 32 instants
 * 2^-30 day apart and 32 2^-20 day apart: the noise of the longitude of
 * SE_OSCU_APOG and of swe_nod_aps()'s osculating perigee and apogee
 * (SE_NODBIT_OSCU), and of their longitude speeds, each the rms of the
 * fourth differences over sqrt(70) (a smooth function's are ~1e-21 at
 * 2^-20 day). SE_OSCU_APOG within 3e-12 deg and 1e-10 deg/day (measured
 * <= 1.5e-12 and 4.1e-11), the swe_nod_aps() points within 6e-11 deg and
 * 8e-8 deg/day (<= 2.4e-11 and 3.6e-8). The latter is the floor of the
 * Moon's state as swe_nod_aps() reads it through swe_calc(), 2e-14 of its
 * distance on all three ephemerides, times 2 / e: <= 5.0e-11 deg (printed
 * per ephemeris); its speed is a three-point difference over 2^-11 day.
 *
 * The build before the fix fails 10 of the 18 rows (SE_OSCU_APOG on all
 * three, Moshier's swe_nod_aps() points: 3.4e-9 deg, 5.5e-6 deg/day), the
 * upstream build (SWE_UPSTREAM_COMPAT) all 18. Run as
 *   oscsmooth EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define NPT 32
#define EPHBITS (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)
static const struct { int32 fl; const char *sz; } rgEph[3] = {
  {SEFLG_SWIEPH, "Swiss"}, {SEFLG_JPLEPH, "de200"}, {SEFLG_MOSEPH, "Moshier"},
};
static const char *rgQ[3] = {"OSCU_APOG", "nod_aps perigee", "nod_aps apogee"};
static const double rgLim[3][2] = {{3e-12, 1e-10}, {6e-11, 8e-8}, {6e-11, 8e-8}};
static const double rgYear[8] = {-2500, -1000, 0, 1000, 1700, 1900, 2000, 2100};
static const double rgYearJ[8] = {1700, 1750, 1800, 1850, 1900, 1950, 2000, 2100};

/* longitude and its speed of quantity q at t */
static int val(swe_ctx *c, int q, double t, int32 fl, double *lon, double *spd)
{
  char serr[AS_MAXCH];
  double x[6], xn[6], xd[6], xp[6], xa[6], xm[6];
  int32 r;
  if (q == 0)
    r = swe_calc_r(c, t, SE_OSCU_APOG, fl | SEFLG_SPEED, x, serr);
  else
    r = swe_nod_aps_r(c, t, SE_MOON, fl | SEFLG_SPEED, SE_NODBIT_OSCU,
                      xn, xd, xp, xa, serr);
  if (r < 0)
    return -1;
  /* the ephemeris asked for, not a fall-back */
  if (swe_calc_r(c, t, SE_MOON, fl, xm, serr) < 0
      || (swe_calc_r(c, t, SE_MOON, fl, xm, serr) & EPHBITS) != (fl & EPHBITS))
    return -1;
  if (q == 1) {
    *lon = xp[0]; *spd = xp[3];
  } else if (q == 2) {
    *lon = xa[0]; *spd = xa[3];
  } else {
    *lon = x[0]; *spd = x[3];
  }
  return 0;
}

/* the noise of v[0..NPT-1]: rms fourth difference / sqrt(70) */
static double noise(const double *v)
{
  double s = 0;
  int k;
  for (k = 0; k + 4 < NPT; k++) {
    double d = v[k] - 4 * v[k+1] + 6 * v[k+2] - 4 * v[k+3] + v[k+4];
    s += d * d;
  }
  return sqrt(s / (NPT - 4) / 70);
}

int main(int argc, char **argv)
{
  int ie, q, ih, iy, k, bad = 0;
  swe_ctx *c;
  if (argc != 2) {
    fprintf(stderr, "usage: oscsmooth EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_jpl_file_r(c, "de200.eph");
  printf("G94: the osculating apogee and perigee are as smooth as the Moon's state\n");
  for (ie = 0; ie < 3; ie++) {
    /* the floor: the Moon's state as swe_nod_aps() reads it, its noise
     * relative to its distance, times 2 / e, in degrees */
    double fl0 = 0;
    for (iy = 0; iy < 8; iy++) {
      double y = ie == 1 ? rgYearJ[iy] : rgYear[iy];
      double t0 = floor((2451545.0 + (y - 2000) * 365.25 + 0.37) * 256) / 256;
      double comp[6][NPT], x[6], r, v2, rv, gm = 8.997011374291876e-10, ev[3], e, w = 0;
      char serr[AS_MAXCH];
      int j;
      for (k = 0; k < NPT; k++) {
        swe_calc_r(c, t0 + k * ldexp(1, -30), SE_MOON, rgEph[ie].fl | SEFLG_J2000
                   | SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_TRUEPOS | SEFLG_NONUT
                   | SEFLG_SPEED, x, serr);
        for (j = 0; j < 6; j++)
          comp[j][k] = x[j];
      }
      r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
      v2 = x[3] * x[3] + x[4] * x[4] + x[5] * x[5];
      rv = x[0] * x[3] + x[1] * x[4] + x[2] * x[5];
      for (j = 0; j < 3; j++)
        ev[j] = ((v2 - gm / r) * x[j] - rv * x[j+3]) / gm;
      e = sqrt(ev[0] * ev[0] + ev[1] * ev[1] + ev[2] * ev[2]);
      for (j = 0; j < 3; j++) {
        double a = noise(comp[j]) / r, b = noise(comp[j+3]) / sqrt(v2);
        if (a > w) w = a;
        if (b > w) w = b;
      }
      if (2 * w / e * RADTODEG > fl0)
        fl0 = 2 * w / e * RADTODEG;
    }
    printf("  %-8s the Moon's state through swe_calc(): 2 x noise / e <= %.1e deg\n",
           rgEph[ie].sz, fl0);
    for (q = 0; q < 3; q++) {
      for (ih = 0; ih < 2; ih++) {
        double h = ldexp(1, ih ? -20 : -30), wp = 0, ws = 0;
        for (iy = 0; iy < 8; iy++) {
          double y = ie == 1 ? rgYearJ[iy] : rgYear[iy];
          double t0 = floor((2451545.0 + (y - 2000) * 365.25 + 0.37) * 256) / 256;
          double vp[NPT], vs[NPT];
          for (k = 0; k < NPT; k++) {
            if (val(c, q, t0 + k * h, rgEph[ie].fl, &vp[k], &vs[k]) < 0) {
              printf("  %s %s ERROR at %.4f\n", rgEph[ie].sz, rgQ[q], t0 + k * h);
              swe_ctx_free(c);
              return 1;
            }
            if (k > 0)
              vp[k] = vp[0] + swe_difdeg2n(vp[k], vp[0]);
          }
          if (noise(vp) > wp) wp = noise(vp);
          if (noise(vs) > ws) ws = noise(vs);
        }
        printf("  %-8s %-16s 2^-%d day: %.1e deg, speed %.1e deg/day\n",
               rgEph[ie].sz, rgQ[q], ih ? 20 : 30, wp, ws);
        if (wp > rgLim[q][0] || ws > rgLim[q][1]) {
          printf("    FAIL: beyond %.0e deg or %.0e deg/day\n", rgLim[q][0], rgLim[q][1]);
          bad++;
        }
      }
    }
  }
  swe_ctx_free(c);
  printf(bad ? "G94 FAIL: %d\n" : "G94 PASS\n", bad);
  return bad ? 1 : 0;
}
