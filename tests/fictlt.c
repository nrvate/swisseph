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
 * t - tau, which this reconstruction does not model. Moshier, and the
 * element file is written here, so it needs no ephemeris files.
 *
 * The barycentre (UPSTREAM-BUGS 125). This check first took B and E as
 * Moshier's heliocentric places, the Sun held still: Moshier serves no
 * barycentric positions (SEFLG_BARYCTR is refused). Since bugs 102 and 109
 * the library solves the light time about the barycentre, B(t - tau)
 * gaining S(t - tau) - S(t), the Sun's barycentric place S from Moshier's
 * own planets as point masses (the Earth-Moon barycentre for the Earth and
 * the Moon, DE431's masses). So the heliocentric check went 0.0043" off on
 * Proserpina and Vulcan, v_S tau. S is built here the same way from the
 * public heliocentric places of the nine planets (the Earth-Moon
 * barycentre from the Earth and the geocentric Moon), within 2e-13 AU of
 * the library's own; B is moved from the read instant jd - tau, a rounded
 * double, to the emission instant along its velocity (bug 81's rounding,
 * 8.6e-7" on Vulcan), and c is the library's, DE431's AU (the old
 * 173.1446326846693 AU/day is the AA 2006 AU's: 6e-11 of every light
 * time, VEA's 2.8e-8 day long). Five dates 1725..2125, and a fourth and a
 * fifth body: a circular orbit at 80000 AU through swe_calc_orbel() (its
 * own light-time path, app_pos_etc_orbel()), the mean anomaly given at
 * each instant, seen from the Sun (E = 0, the light time from the Sun at
 * t) and from the Earth. Until bug 130 paired its frame bias the orbit's
 * geocentric places were 3.5e-8 to 1.1e-7 AU off its heliocentric ones
 * less the Earth, and only the first was checked here.
 *
 * Measured, check 2, within 1e-6" (it was 1e-3"): Proserpina 1.2e-9",
 * Vulcan 4.0e-9", VEA 2.9e-12", the orbel orbit 1.8e-10" from the Sun and
 * from the Earth (1.1e-9" and 2.7e-7" before bug 130). The build before
 * bug 125 fails VEA at 1.12e-3" and the orbel orbit from the Sun at
 * 1.13e-3": over their light time of 462 days the Sun was carried by the
 * Taylor step s v_S + s^2 a_S / 2, which leaves out the turn of its loop
 * about the barycentre (1.9e-4 AU across the line of sight in 2025, up to
 * 5.8e-4 AU). The build before bug 109 fails those four at 7.3e-3" to
 * 9.6e-3" (the Sun held still); the upstream build fails eight of the ten
 * checks (bug 26's 63.7" on the first besides). Run as
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

/* body ib at jd: a set of the element file, or the orbel orbit */
static int Calc(double jd, int ib, int32 fl, double *x, char *serr)
{
  const double a = 80000, n = 0.01720209895 / sqrt(a * a * a) * 180 / M_PI;
  double el[6] = {0, 0, 0, 0, 0, 0};
  if (ib < 3)
    return swe_calc(jd, SE_FICT_OFFSET + ib, fl, x, serr);
  el[0] = fmod(n * (jd - 2451545.0), 360);
  if (el[0] < 0)
    el[0] += 360;
  el[1] = a;
  return swe_calc_orbel(jd, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, fl, x, serr);
}

/* The Sun's barycentric place at jd from Moshier's planets, as the
 * library's light time takes it: -sum m_i x_i / (1 + sum m_i), in units of
 * the Sun's mass, x_i heliocentric and geometric (fl has SEFLG_TRUEPOS),
 * the Earth-Moon barycentre for the Earth and the Moon. */
static int SunBary(double jd, int32 fl, double *xs, char *serr)
{
  static const int ipl[9] = {SE_MERCURY, SE_VENUS, SE_EARTH, SE_MARS, SE_JUPITER,
                             SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO};
  static const double mrat[9] = {	/* M_Sun / m, DE431 */
    6023600, 408523.719, 328900.5, 3098703.59, 1047.348644,
    3497.9018, 22902.98, 19412.26, 136566000};
  const double emrat = 1 / 0.0123000383;	/* Earth / Moon, sweph.h */
  double x[6], xm[6], msum = 1, p[3] = {0, 0, 0};
  int i, k;
  for (k = 0; k < 9; k++) {
    if (swe_calc(jd, ipl[k], fl | SEFLG_HELCTR, x, serr) < 0)
      return -1;
    if (ipl[k] == SE_EARTH) {
      if (swe_calc(jd, SE_MOON, fl, xm, serr) < 0)
        return -1;
      for (i = 0; i < 3; i++)
        x[i] += xm[i] / (1 + emrat);
    }
    msum += 1 / mrat[k];
    for (i = 0; i < 3; i++)
      p[i] += x[i] / mrat[k];
  }
  for (i = 0; i < 3; i++)
    xs[i] = -p[i] / msum;
  return 0;
}

int main(void)
{
  static const char *rgszName[] = {"Proserpina", "Vulcan", "VEA", "orbel", "orbel geo"};
  const double c = 299792458.0 * 86400 / 1.495978707e11;   /* AU per day */
  char dir[] = "/tmp/fictltXXXXXX", path[256], serr[256];
  int32 base = SEFLG_MOSEPH | SEFLG_J2000 | SEFLG_NONUT | SEFLG_XYZ;
  int32 astr = base | SEFLG_NOABERR | SEFLG_NOGDEFL;
  int ib, id, it, k, bad = 0;
  FILE *pf;

  if (mkdtemp(dir) == NULL) { perror("mkdtemp"); return 2; }
  snprintf(path, sizeof(path), "%s/seorbel.txt", dir);
  if ((pf = fopen(path, "w")) == NULL) { perror(path); return 2; }
  fputs(szElements, pf);
  fclose(pf);
  swe_set_ephe_path(dir);
  printf("G27: fictitious light time on the orbit, independent of SEFLG_SPEED\n");
  for (ib = 0; ib < 5; ib++) {
    double wSpeed = 0, wDef = 0, tau = 0.0;
    for (id = 0; id < 5; id++) {
      const double jd = 2461300.5 + 36525.0 * (id - 3);
      /* the orbel orbit seen from the Sun (E = 0), and from the Earth */
      int32 ctr = ib == 3 ? SEFLG_HELCTR : 0;
      double xs[6], xn[6], xe[6] = {0, 0, 0}, xb[6], s0[3], s1[3], d[3], r, dSpeed, dDef;
      if (Calc(jd, ib, astr | ctr | SEFLG_SPEED, xs, serr) < 0 ||
          Calc(jd, ib, astr | ctr, xn, serr) < 0 ||
          (!ctr && swe_calc(jd, SE_EARTH, base | SEFLG_HELCTR | SEFLG_TRUEPOS, xe, serr) < 0) ||
          SunBary(jd, base | SEFLG_TRUEPOS, s0, serr) < 0) {
        printf("  %-10s ERROR %s\n", rgszName[ib], serr);
        bad++;
        break;
      }
      /* tau solved here, on this library's own geometric barycentric
       * orbit: B(t - tau) + S(t - tau) - E(t) - S(t) */
      tau = 0.0;
      for (it = 0; it < 50; it++) {
        double tr = jd - tau, eps;
        if (Calc(tr, ib, base | SEFLG_HELCTR | SEFLG_TRUEPOS | SEFLG_SPEED, xb, serr) < 0 ||
            SunBary(tr, base | SEFLG_TRUEPOS, s1, serr) < 0) {
          printf("  %-10s ERROR %s\n", rgszName[ib], serr);
          bad++;
          break;
        }
        eps = (jd - tr) - tau;	/* the read instant's rounding */
        for (k = 0; k < 3; k++)
          d[k] = (xb[k] + eps * xb[k+3] + s1[k]) - (xe[k] + s0[k]);
        r = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        if (fabs(r / c - tau) < 1e-13) break;
        tau = r / c;
      }
      dSpeed = Sep(xs, xn);
      dDef = Sep(xs, d);
      if (dSpeed > wSpeed) wSpeed = dSpeed;
      if (dDef > wDef) wDef = dDef;
    }
    printf("  %-10s tau %11.6f d   speed flag moves it %.6f\"   "
           "from B(t-tau)-E(t) %.2e\"\n", rgszName[ib], tau, wSpeed, wDef);
    if (wSpeed > 1e-6) {
      printf("    FAIL: the position depends on SEFLG_SPEED\n");
      bad++;
    }
    if (wDef > 1e-6) {
      printf("    FAIL: the light time is not the solution of its equation\n");
      bad++;
    }
  }
  unlink(path);
  rmdir(dir);
  printf(bad ? "G27 FAIL: %d\n" : "G27 PASS\n", bad);
  return bad ? 1 : 0;
}
