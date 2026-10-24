/* jpltdb.c -- G34: the JPL ephemeris is read at TDB, its argument.
 *
 * A JPL ephemeris (and the Swiss files, fitted to one) takes TDB as its time
 * argument. Swiss reads both at the TT it is given: "we assume Teph ~= TDB
 * ~= TT", with the conversion commented out (jplplan()). TDB - TT is an
 * annual 1.66 ms, which the geocentric Moon at 1 km/s turns into up to
 * 1.7 m, 0.88 mas, and the barycentric Earth at 30 km/s into 50 m. Found
 * comparing with Ephemeris Prometheia, which reads at TDB.
 *
 * The check: JPL's own test positions for DE200 (testpo200.txt, the rows of
 * the geocentric Moon and the barycentric Earth from JPL's testpo.200),
 * each given at its ephemeris argument JED. The TT whose TDB is JED is
 * solved here with the seven-term series of USNO Circular 179, eq. 2.6
 * (written out here, not taken from the library); swe_calc_r at that TT
 * from de200.eph, geometric, J2000 equatorial, must equal JPL's value,
 * after the rounding of that TT to a double is put back through the
 * velocity. Moon within 5e-13 AU (7 cm), Earth within 5e-12 AU (75 cm: the
 * series is good to ~9 us over DE200's span, 27 cm at 30 km/s). The
 * upstream build fails, by up to ~1e-11 and ~3e-10 AU. Run as
 *   jpltdb EPHEDIR FIXTURE
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static double tdb_minus_tt(double tjd)   /* days */
{
  double T = (tjd - 2451545.0) / 36525.0;
  return (0.001657 * sin(628.3076 * T + 6.2401)
        + 0.000022 * sin(575.3385 * T + 4.2970)
        + 0.000014 * sin(1256.6152 * T + 6.1969)
        + 0.000005 * sin(606.9777 * T + 4.0212)
        + 0.000005 * sin(52.9691 * T + 0.4444)
        + 0.000002 * sin(21.3299 * T + 5.5431)
        + 0.000010 * T * sin(628.3076 * T + 4.2490)) / 86400.0;
}

int main(int argc, char **argv)
{
  int32 iflag = SEFLG_JPLEPH | SEFLG_TRUEPOS | SEFLG_NOABERR | SEFLG_NOGDEFL |
                SEFLG_J2000 | SEFLG_ICRS | SEFLG_NONUT | SEFLG_EQUATORIAL |
                SEFLG_XYZ | SEFLG_SPEED;
  char line[256], date[32], serr[256];
  double wMoon = 0, wEarth = 0;
  int n = 0, bad = 0;
  FILE *pf;
  swe_ctx *ctx;

  if (argc != 3) {
    fprintf(stderr, "usage: jpltdb EPHEDIR FIXTURE\n");
    return 2;
  }
  if ((pf = fopen(argv[2], "r")) == NULL) {
    perror(argv[2]);
    return 2;
  }
  ctx = swe_ctx_new();
  swe_set_ephe_path_r(ctx, argv[1]);
  swe_set_jpl_file_r(ctx, "de200.eph");
  printf("G34: the JPL ephemeris is read at TDB\n");
  while (fgets(line, sizeof(line), pf) != NULL) {
    int de, targ, ctr, coord;
    double jed, val, tt, e, x[6], got;
    if (line[0] == '#')
      continue;
    if (sscanf(line, "%d %31s %lf %d %d %d %lf", &de, date, &jed, &targ, &ctr,
               &coord, &val) != 7 || coord < 1 || coord > 3)
      continue;
    tt = jed - tdb_minus_tt(jed);
    tt = jed - tdb_minus_tt(tt);
    e = (jed - tt) - tdb_minus_tt(tt);   /* what rounding tt left unread */
    if (swe_calc_r(ctx, tt, targ == 10 ? SE_MOON : SE_EARTH,
                   iflag | (targ == 10 ? 0 : SEFLG_BARYCTR), x, serr) < 0) {
      printf("  %s ERROR %s\n", date, serr);
      bad++;
      continue;
    }
    got = x[coord - 1] + x[coord + 2] * e;
    if (targ == 10) {
      if (fabs(got - val) > wMoon) wMoon = fabs(got - val);
    } else {
      if (fabs(got - val) > wEarth) wEarth = fabs(got - val);
    }
    n++;
  }
  fclose(pf);
  swe_ctx_free(ctx);
  printf("  %d rows; worst Moon %.2e AU (%.3f m), Earth %.2e AU (%.3f m)\n", n,
         wMoon, wMoon * 1.495978707e11, wEarth, wEarth * 1.495978707e11);
  if (n < 40) {
    printf("    FAIL: the fixture has %d rows, expected 47\n", n);
    bad++;
  }
  if (wMoon > 5e-13) {
    printf("    FAIL: the Moon is not read at TDB\n");
    bad++;
  }
  if (wEarth > 5e-12) {
    printf("    FAIL: the Earth is not read at TDB\n");
    bad++;
  }
  printf(bad ? "G34 FAIL: %d\n" : "G34 PASS\n", bad);
  return bad ? 1 : 0;
}
