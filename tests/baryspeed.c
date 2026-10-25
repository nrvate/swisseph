/* baryspeed.c -- G85: a barycentric body's speeds are the rates of its
 * positions on JPL as on the Swiss files, the Sun's included, and its
 * sidereal place takes the ayanamsa of its own date.
 *
 * The defect (UPSTREAM-BUGS 86): the barycentric Sun is answered in the
 * Earth's save area, and app_pos_etc_sbar() took the instant of date from
 * that slot for swi_bias() and app_pos_rest(), which takes the ecliptic's
 * turning (bug 33) and the ayanamsa from it. On the Swiss files sweplan()
 * has just filled the Earth's slot for the Sun's instant; on JPL only the
 * Sun's slot is read, so the instant was whatever the last request left:
 * the latitude speed of date off by the nutation in obliquity's rate at
 * another date, up to 1.9e-5 deg/day (1.85e-5 at these eight dates), NaN as
 * the first request of a context (the slot's empty mark), and the sidereal
 * longitude off by the ayanamsa's change since then, 0.14 deg for a
 * request ten years on.
 *
 * The check: with SEFLG_BARYCTR, the Sun, Earth, Moon, Mercury, Mars,
 * Jupiter, Pluto and Chiron, in the ecliptic of date, without nutation, the
 * J2000 ecliptic, the equator of date and sidereal (Lahiri), eight dates
 * 1925-2075, JPL (de200.eph) and the Swiss files; each speed taken right
 * after a request for Mars ten years on, so the Earth's slot is elsewhere.
 * Against five-point differences over 2^-6 day at binary-fraction
 * instants: longitude times cos(latitude) and latitude within 2e-11
 * deg/day (measured <= 5e-12, sidereal included: it was 5.7e-11 on every
 * sidereal body, the ayanamsa's rate as a backward difference over 0.001
 * day, until UPSTREAM-BUGS 91, and the limit 2e-10), 2e-9 for the Sun on
 * the files (9.7e-10, their own); the distance rate within 5e-11 of the distance per day
 * (<= 9.1e-12). Also the JPL Sun's latitude speed as
 * the first request of a context, within 2e-9, and its sidereal longitude
 * after the Mars request against the tropical less the ayanamsa, within
 * 1e-9 deg (measured 0): 162 checks. The build before the fix fails 4,
 * all the JPL Sun's: the first request (NaN), of date (1.85e-5 deg/day),
 * sidereal (3.8e-5 deg/day; 0.140 deg in longitude); without nutation the
 * stale mean obliquity's rate is 5.9e-12, within the limit. The build
 * before bug 91's fix fails the other 15 sidereal rows (5.3e-11 to
 * 5.7e-11). The upstream build fails all 162 (with entries 73 and 78 not
 * there either). Run as
 *   baryspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

static int bad = 0;

/* speed less the five-point rate of the position: longitude times
 * cos(latitude) and latitude, deg/day; distance, of the distance per day.
 * A request ten years on precedes the speed's. */
static int resid(swe_ctx *c, double t, int ipl, int32 fl, double h, double *res)
{
  double x[6], p[5][6], z[6];
  char serr[256];
  int i, j;
  for (i = 0; i < 5; i++)
    if (swe_calc_r(c, t + (i - 2) * h, ipl, fl, p[i], serr) < 0)
      return -1;
  swe_calc_r(c, t + 3652.5, SE_MARS, (fl & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)) | SEFLG_SPEED, z, serr);
  if (swe_calc_r(c, t, ipl, fl | SEFLG_SPEED, x, serr) < 0)
    return -1;
  for (j = 0; j < 3; j++) {
    double d1 = p[3][j] - p[1][j], d2 = p[4][j] - p[0][j];
    if (j == 0) {
      d1 -= 360 * floor(d1 / 360 + 0.5);
      d2 -= 360 * floor(d2 / 360 + 0.5);
    }
    res[j] = x[3 + j] - (8 * d1 - d2) / (12 * h);
  }
  res[0] *= cos(x[1] * DEGTORAD);
  res[2] /= x[2];
  return 0;
}

static void judge(const char *what, double ang, double rel, double lang, double lrel)
{
  printf("  %-34s angles %.2e deg/day, distance %.2e of it per day\n", what, ang, rel);
  /* !(<=) so that a NaN fails */
  if (!(ang <= lang)) {
    printf("    FAIL: angular speed off its positions' rate (limit %.0e)\n", lang);
    bad++;
  }
  if (!(rel <= lrel)) {
    printf("    FAIL: distance rate off its positions' rate (limit %.0e)\n", lrel);
    bad++;
  }
}

static void worst(const double *r, double *ang, double *rel)
{
  int j;
  for (j = 0; j < 2; j++)
    if (!(fabs(r[j]) <= *ang)) *ang = fabs(r[j]);
  if (!(fabs(r[2]) <= *rel)) *rel = fabs(r[2]);
}

int main(int argc, char **argv)
{
  static const int body[8] = {SE_SUN, SE_EARTH, SE_MOON, SE_MERCURY, SE_MARS, SE_JUPITER, SE_PLUTO, SE_CHIRON};
  static const char *bname[8] = {"Sun", "Earth", "Moon", "Mercury", "Mars", "Jupiter", "Pluto", "Chiron"};
  static const int32 frame[5] = {0, SEFLG_NONUT, SEFLG_J2000, SEFLG_EQUATORIAL, SEFLG_SIDEREAL};
  static const char *fname[5] = {"of date", "no nutation", "J2000", "equatorial", "sidereal"};
  static const int32 eph[2] = {SEFLG_JPLEPH, SEFLG_SWIEPH};
  static const char *ename[2] = {"JPL", "Swiss"};
  char serr[256], what[80];
  swe_ctx *c;
  int e, b, f, y;

  if (argc != 2) {
    fprintf(stderr, "usage: baryspeed EPHEDIR\n");
    return 2;
  }
  printf("G85: barycentric speeds are their positions' rates, the JPL Sun's included\n");

  /* the JPL barycentric Sun as the first request of a context */
  {
    double x[6], p[5][6], h = 1.0 / 64, t = floor(2451545.3 * 256) / 256, r;
    int i;
    c = swe_ctx_new();
    swe_set_ephe_path_r(c, argv[1]);
    swe_set_jpl_file_r(c, "de200.eph");
    swe_calc_r(c, t, SE_SUN, SEFLG_JPLEPH | SEFLG_BARYCTR | SEFLG_SPEED, x, serr);
    for (i = 0; i < 5; i++)
      swe_calc_r(c, t + (i - 2) * h, SE_SUN, SEFLG_JPLEPH | SEFLG_BARYCTR, p[i], serr);
    r = fabs(x[4] - (8 * (p[3][1] - p[1][1]) - (p[4][1] - p[0][1])) / (12 * h));
    printf("  JPL Sun, first request, latitude  %.2e deg/day\n", r);
    if (!(r <= 2e-9)) {
      printf("    FAIL: its latitude speed is not its rate\n");
      bad++;
    }
    swe_ctx_free(c);
  }

  c = swe_ctx_new();
  swe_set_ephe_path_r(c, argv[1]);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);

  for (e = 0; e < 2; e++)
    for (b = 0; b < 8; b++)
      for (f = 0; f < 5; f++) {
        double ang = 0, rel = 0, r[3];
        int32 fl = eph[e] | SEFLG_BARYCTR | frame[f];
        for (y = 0; y < 8; y++) {
          double t = floor((2451545.0 + (-75 + y * 21.7) * 365.25 + 0.3) * 256) / 256;
          if (resid(c, t, body[b], fl, 1.0 / 64, r) < 0) {
            printf("  %s %s ERROR\n", ename[e], bname[b]);
            bad++;
            break;
          }
          worst(r, &ang, &rel);
        }
        snprintf(what, sizeof what, "%s %s, bary, %s", ename[e], bname[b], fname[f]);
        judge(what, ang, rel,
              (e == 1 && body[b] == SE_SUN) ? 2e-9 : 2e-11,
              5e-11);
      }
  /* the sidereal JPL barycentric Sun takes the ayanamsa of its own date */
  {
    double t = 2451545.25, xs[6], xt[6], z[6], aya, d;
    if (swe_get_ayanamsa_ex_r(c, t, SEFLG_JPLEPH, &aya, serr) < 0)
      aya = 0;
    swe_calc_r(c, t + 3652.5, SE_MARS, SEFLG_JPLEPH, z, serr);
    swe_calc_r(c, t, SE_SUN, SEFLG_JPLEPH | SEFLG_BARYCTR | SEFLG_SIDEREAL, xs, serr);
    swe_calc_r(c, t, SE_SUN, SEFLG_JPLEPH | SEFLG_BARYCTR, xt, serr);
    d = fabs(swe_difdeg2n(xs[0], swe_degnorm(xt[0] - aya)));
    printf("  JPL Sun, bary, sidereal longitude %.2e deg\n", d);
    if (!(d <= 1e-9)) {
      printf("    FAIL: the ayanamsa is not the Sun's date's\n");
      bad++;
    }
  }

  swe_ctx_free(c);
  printf(bad ? "G85 FAIL: %d\n" : "G85 PASS\n", bad);
  return bad ? 1 : 0;
}
