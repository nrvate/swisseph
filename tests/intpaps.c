/* intpaps.c -- G46: the interpolated lunar apogee and perigee are the same
 * with or without SEFLG_SPEED, light-time corrected in all three
 * coordinates, smooth at the JD's resolution, and their speeds are the
 * rates of their positions.
 *
 * Two upstream defects in SE_INTP_APOG / SE_INTP_PERG (UPSTREAM-BUGS 44,
 * 45):
 *
 * - intp_apsides() moved the apsis back along its light time as
 *   x -= tau v, with v the speed -- zero without SEFLG_SPEED -- and from
 *   the second coordinate on. The position depended on the speed flag
 *   (the perigee by 111 mas, the apogee by 13) and the x coordinate never
 *   had the correction. Now X(t) - tau V(t), V always computed.
 *
 * - swi_intp_apsides() finds the apsis as the vertex of parabolas through
 *   three lunar distances 5 deg, 0.5 deg, ... 1.8" apart, and at the small
 *   steps the parabola's slope and curvature (~1e-6 km) came out of the
 *   last digits of a 3.6e5 km distance: the perigee jumped 1.3 mas between
 *   consecutive JDs (2100), the apogee 0.023 mas. The speed was the
 *   difference over +-0.1 day (1.1e-4 deg/day off on the perigee) and the
 *   SEFLG_SPEED3 one over +-8.64 s of that jitter (1.5e-3). Now the late
 *   steps are Newton steps from five points 180" apart, and both speeds
 *   difference over +-2^-8 day.
 *
 * The check, perigee and apogee at eight epochs -3000..3000 (Swiss files;
 * the apsides are analytic): (a) the position with and without
 * SEFLG_SPEED, default, EQUATORIAL, XYZ, NONUT, TRUEPOS and Lahiri, equal
 * to 1e-6 mas (measured: identical); (b) the cartesian position less the
 * TRUEPOS one is -tau V, V a five-point difference of the TRUEPOS
 * positions, with and without the speed flag, within 0.01 mas (measured
 * <= 1.9e-3: the ecliptic's and the nutation's turning over tau, which V
 * carries and the correction, made in the mean ecliptic of date, does
 * not); (c) the longitude and latitude over 64 consecutive representable
 * JDs lie on a line within 0.01 mas (measured <= 1.8e-3); (d) the
 * longitude, latitude and distance speeds (right ascension, declination;
 * default, EQUATORIAL, TRUEPOS, Lahiri; SEFLG_SPEED and SEFLG_SPEED3)
 * against a five-point difference over 2^-4 day of the positions at
 * multiples of 2^-8 day, within 1e-6 deg/day (measured <= 2.1e-7) and
 * 1e-11 AU/day (<= 9e-13). SEFLG_J2000 is not checked for (d): its
 * latitude speed carries ~5e-7 deg/day of the J2000 conversion's own, as
 * the mean apogee's does. The upstream build fails (a) by 140 mas
 * (perigee; 15 the apogee), (b) by 120 (13), (c) by 1.3 (0.023) and (d)
 * by 1.8e-3 deg/day (1.1e-4) and 6e-11 AU/day. Run as
 *   intpaps EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#define RAD2MAS 206264806.24709636
#define DEG2MAS 3600000.0
#define AU_LT_DAYS (1.49597870700e11 / 299792458.0 / 86400.0)

static const struct { int ipl; const char *sz; } rgObj[2] = {
  {SE_INTP_PERG, "perigee"}, {SE_INTP_APOG, "apogee"},
};
static const double rgYear[8] = {-3000, -999, 1, 1300, 1800, 2000, 2100, 3000};

static int calc(swe_ctx *c, double jd, int ipl, int32 fl, double *x)
{
  char serr[256];
  if (swe_calc_r(c, jd, ipl, SEFLG_SWIEPH | fl, x, serr) < 0) {
    printf("  ERROR jd %.6f ipl %d flags %d: %s\n", jd, ipl, fl, serr);
    return -1;
  }
  return 0;
}

/* the angle between two positions, mas: polar (lon, lat, dist) or xyz */
static double sep_mas(const double *a, const double *b, int xyz)
{
  if (xyz) {
    double d[3], r = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    int i;
    for (i = 0; i < 3; i++)
      d[i] = a[i] - b[i];
    return sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) / r * RAD2MAS;
  }
  return fmax(fabs(swe_difdeg2n(a[0], b[0])) * cos(a[1] * 0.017453292519943295),
              fabs(a[1] - b[1])) * DEG2MAS
         + fabs(a[2] - b[2]) / a[2] * RAD2MAS;
}

int main(int argc, char **argv)
{
  static const struct { int32 fl; const char *sz; } rgFa[6] = {
    {0, "default"}, {SEFLG_EQUATORIAL, "EQUATORIAL"}, {SEFLG_XYZ, "XYZ"},
    {SEFLG_NONUT, "NONUT"}, {SEFLG_TRUEPOS, "TRUEPOS"}, {SEFLG_SIDEREAL, "Lahiri"},
  };
  static const struct { int32 fl; const char *sz; } rgFd[4] = {
    {0, "default"}, {SEFLG_EQUATORIAL, "EQUATORIAL"},
    {SEFLG_TRUEPOS, "TRUEPOS"}, {SEFLG_SIDEREAL, "Lahiri"},
  };
  int o, y, f, k, j, s, bad = 0;
  swe_ctx *c;

  if (argc != 2) {
    fprintf(stderr, "usage: intpaps EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  printf("G46: the interpolated apsides: one position whatever the speed flag, smooth, with their rates\n");
  for (o = 0; o < 2; o++) {
    int ipl = rgObj[o].ipl;
    double wa = 0, wb = 0, wc = 0, wd[2] = {0, 0};
    const char *sza = "", *szd = "";
    for (y = 0; y < 8; y++) {
      double jd = 2451545.0 + (rgYear[y] - 2000) * 365.25 + 0.1234567;
      double t = floor(jd * 256) / 256, h = 1.0 / 16;
      double x[6], xs[6], p[5][6], xt[6], xa[6];
      /* (a) */
      for (f = 0; f < 6; f++) {
        double e;
        if (calc(c, jd, ipl, rgFa[f].fl, x) || calc(c, jd, ipl, rgFa[f].fl | SEFLG_SPEED, xs))
          return 1;
        e = sep_mas(x, xs, rgFa[f].fl & SEFLG_XYZ);
        if (e > wa) { wa = e; sza = rgFa[f].sz; }
      }
      /* (b) the light time: X - X_true = -tau V */
      for (k = 0; k < 5; k++)
        if (calc(c, t + (k - 2) * h, ipl, SEFLG_XYZ | SEFLG_TRUEPOS, p[k]))
          return 1;
      for (s = 0; s < 2; s++) {
        double tau, d[3], e;
        int32 fs = s ? SEFLG_SPEED : 0;
        if (calc(c, t, ipl, SEFLG_XYZ | SEFLG_TRUEPOS | fs, xt) || calc(c, t, ipl, SEFLG_XYZ | fs, xa))
          return 1;
        tau = sqrt(xt[0] * xt[0] + xt[1] * xt[1] + xt[2] * xt[2]) * AU_LT_DAYS;
        for (j = 0; j < 3; j++) {
          double v = (p[0][j] - 8 * p[1][j] + 8 * p[3][j] - p[4][j]) / (12 * h);
          d[j] = xt[j] - tau * v;
        }
        e = sep_mas(xa, d, 1);
        if (e > wb) wb = e;
      }
      /* (c) 64 consecutive JDs */
      for (j = 0; j < 2; j++) {
        double v[64], st = 0, sv = 0, stt = 0, stv = 0, b, a, tt = jd, x0 = 0;
        for (k = 0; k < 64; k++) {
          if (calc(c, tt, ipl, 0, x))
            return 1;
          if (k == 0) x0 = x[j];
          v[k] = (j == 0 ? swe_difdeg2n(x[0], x0) : x[1] - x0) * DEG2MAS;
          st += k; sv += v[k]; stt += (double) k * k; stv += k * v[k];
          tt = nextafter(tt, 1e9);
        }
        b = (64 * stv - st * sv) / (64 * stt - st * st);
        a = (sv - b * st) / 64;
        for (k = 0; k < 64; k++)
          if (fabs(v[k] - a - b * k) > wc) wc = fabs(v[k] - a - b * k);
      }
      /* (d) the speeds */
      for (f = 0; f < 4; f++) {
        for (k = 0; k < 5; k++)
          if (calc(c, t + (k - 2) * h, ipl, rgFd[f].fl, p[k]))
            return 1;
        for (s = 0; s < 2; s++) {
          if (calc(c, t, ipl, rgFd[f].fl | (s ? SEFLG_SPEED3 : SEFLG_SPEED), xs))
            return 1;
          for (j = 0; j < 3; j++) {
            double v[5], r, e;
            for (k = 0; k < 5; k++)
              v[k] = j == 0 ? p[2][0] + swe_difdeg2n(p[k][0], p[2][0]) : p[k][j];
            r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
            e = fabs(xs[3 + j] - r);
            if (e > wd[j == 2]) {
              wd[j == 2] = e;
              if (j < 2) szd = rgFd[f].sz;
            }
          }
        }
      }
    }
    printf("  %-8s (a) speed flag moves it %.1e mas (%s)  (b) light time off %.1e mas\n",
           rgObj[o].sz, wa, sza, wb);
    printf("  %-8s (c) rough %.1e mas  (d) speeds off %.1e deg/day (%s), %.1e AU/day\n",
           "", wc, wd[0], szd, wd[1]);
    if (wa > 1e-6) { printf("    FAIL: (a) above 1e-6 mas\n"); bad++; }
    if (wb > 0.01) { printf("    FAIL: (b) above 0.01 mas\n"); bad++; }
    if (wc > 0.01) { printf("    FAIL: (c) above 0.01 mas\n"); bad++; }
    if (wd[0] > 1e-6) { printf("    FAIL: (d) angle speed above 1e-6 deg/day\n"); bad++; }
    if (wd[1] > 1e-11) { printf("    FAIL: (d) distance rate above 1e-11 AU/day\n"); bad++; }
  }
  swe_ctx_free(c);
  printf(bad ? "G46 FAIL: %d\n" : "G46 PASS\n", bad);
  return bad ? 1 : 0;
}
