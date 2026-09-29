/* fictlt.c -- G27: a fictitious body's light time is solved on its orbit,
 * and its position does not depend on whether speeds were asked for.
 *
 * app_pos_etc_plan_osc() corrected a fictitious body for light time with a
 * straight line, x(t) - dt * v(t), and only on the SEFLG_SPEED path went on
 * to re-evaluate the orbit at t - dt. So the same question answered two
 * different positions depending on a flag that is only supposed to add
 * columns: 63.7" apart for an element set at 80,000 AU (seorbel.txt's set
 * 26), 0.095" for the intramercurial Vulcan, whose orbit curves appreciably
 * within its light time.
 *
 * Two checks per body, both with no golden file:
 *   1. the position with SEFLG_SPEED equals the position without it;
 *   2. the astrometric direction equals B(t - tau) - E(t), with tau the
 *      solution of tau = |B(t - tau) - E(t)| / c built here from this
 *      library's own geometric barycentric positions. Under light time the
 *      instant evaluated is the emission instant, for the elements and for
 *      an equinox of date alike (the ephemeris protocol's 3.5a, agreed by
 *      the Astrolog and Ephemeris Prometheia projects on 2026-09-29).
 *
 * Heliocentric sets only: an Earth-centred orbit's focus is the Earth at
 * t - tau, which this reconstruction does not model. Under Moshier the
 * library's barycentre IS the Sun (it serves no barycentric positions), so
 * B and E are its heliocentric ones, consistently. Moshier, and the
 * element file is written here, so it needs no ephemeris files. Run as
 *   fictlt
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

static const char *szElements =
  "J1900,JDATE, 170.73, 79.225630, 0, 0, 0, 0, Proserpina\n"
  "J1900,JDATE, 252.8987988 + 707550.7341 * T, 0.13744, 0.019, "
    "322.212069+1670.056*T, 47.787931-1670.056*T, 7.5, Vulcan\n"
  "J2000,JDATE, 0.0 + 0.00000 * T, 80000, 0.0, 0.0, 0.0, 0.0, VEA\n";

static double Sep(const double *a, const double *b)   /* arcsec */
{
  double ra = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
  double rb = sqrt(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]);
  double cx = a[1]*b[2] - a[2]*b[1], cy = a[2]*b[0] - a[0]*b[2];
  double cz = a[0]*b[1] - a[1]*b[0];
  double dot = (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]) / (ra * rb);
  return atan2(sqrt(cx*cx + cy*cy + cz*cz) / (ra * rb), dot) * 206264.806247;
}

int main(void)
{
  static const char *rgszName[] = {"Proserpina", "Vulcan", "VEA"};
  const double c = 173.1446326846693;   /* AU per day */
  const double jd = 2461300.5;
  char dir[] = "/tmp/fictltXXXXXX", path[256], serr[256];
  int32 base = SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_XYZ;
  int32 astr = base | SEFLG_NOABERR | SEFLG_NOGDEFL;
  int ib, it, bad = 0;
  FILE *pf;

  if (mkdtemp(dir) == NULL) { perror("mkdtemp"); return 2; }
  snprintf(path, sizeof(path), "%s/seorbel.txt", dir);
  if ((pf = fopen(path, "w")) == NULL) { perror(path); return 2; }
  fputs(szElements, pf);
  fclose(pf);
  swe_set_ephe_path(dir);
  printf("G27: fictitious light time on the orbit, independent of SEFLG_SPEED\n");
  for (ib = 0; ib < 3; ib++) {
    int ipl = SE_FICT_OFFSET + ib;
    double xs[6], xn[6], xe[6], xb[6], d[3], tau, r, dSpeed, dDef;
    if (swe_calc(jd, ipl, astr | SEFLG_SPEED, xs, serr) < 0 ||
        swe_calc(jd, ipl, astr, xn, serr) < 0 ||
        swe_calc(jd, SE_EARTH, base | SEFLG_HELCTR | SEFLG_TRUEPOS, xe, serr) < 0) {
      printf("  %-10s ERROR %s\n", rgszName[ib], serr);
      bad++;
      continue;
    }
    /* tau solved here, on this library's own geometric barycentric orbit */
    tau = 0.0;
    for (it = 0; it < 50; it++) {
      if (swe_calc(jd - tau, ipl, base | SEFLG_HELCTR | SEFLG_TRUEPOS, xb, serr) < 0) {
        printf("  %-10s ERROR %s\n", rgszName[ib], serr);
        bad++;
        break;
      }
      d[0] = xb[0] - xe[0]; d[1] = xb[1] - xe[1]; d[2] = xb[2] - xe[2];
      r = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
      if (fabs(r / c - tau) < 1e-13) break;
      tau = r / c;
    }
    dSpeed = Sep(xs, xn);
    dDef = Sep(xs, d);
    printf("  %-10s tau %11.6f d   speed flag moves it %.6f\"   "
           "from B(t-tau)-E(t) %.6f\"\n", rgszName[ib], tau, dSpeed, dDef);
    if (dSpeed > 1e-6) {
      printf("    FAIL: the position depends on SEFLG_SPEED\n");
      bad++;
    }
    if (dDef > 1e-3) {
      printf("    FAIL: the light time is not the solution of its equation\n");
      bad++;
    }
  }
  unlink(path);
  rmdir(dir);
  printf(bad ? "G27 FAIL: %d\n" : "G27 PASS\n", bad);
  return bad ? 1 : 0;
}
