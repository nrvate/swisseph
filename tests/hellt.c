/* hellt.c -- G36: a heliocentric light time is measured from the Sun on
 * every pass of its iteration.
 *
 * app_pos_etc_plan() corrects a planet for light time in two passes. For a
 * heliocentric request the first pass measured |body - Sun| correctly, but
 * the second pass measured |xx0 - dt * v| with xx0 the BARYCENTRIC position
 * and nothing subtracted (the "observer" is only subtracted for a geocentric
 * request), so the light time it kept was the barycentric distance over c.
 * The Sun sits up to 0.01 AU from the barycentre, so the planet was read at
 * an instant off by up to 2.3% of its light time: 0.84" on Mercury
 * (2200-01-01), 0.28" on Venus, 0.09" on Mars, 0.016" on Jupiter, against
 * JPL Horizons' Sun-centred astrometric places and Ephemeris Prometheia,
 * which agree with each other to 0.00001" (2026-10-24). The same loop in
 * app_pos_etc_plan_osc() never subtracted the Sun at all, so a heliocentric
 * fictitious body had the barycentric light time on both passes.
 *
 * No golden file. The reference is built from this library's own geometric
 * barycentric positions: the heliocentric astrometric direction must equal
 * B(t - tau) - S(t), tau the solution of tau = |B(t - tau) - S(t)| / c, to
 * 0.0001". That is exactly the recipe app_pos_etc_plan() runs, less the
 * frame mix-up, so the rebuilt vector reproduces the fixed library to
 * roundoff and the unfixed one to its defect.
 *
 * Planets under SWIEPH (the ephe/ sepl files) and under JPLEPH (de200.eph,
 * which ephe/ ships), and one fictitious body under SWIEPH from a seorbel.txt
 * written here. Run as
 *   hellt <ephe-dir>
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

static double Sep(const double *a, const double *b)   /* arcsec */
{
  double ra = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
  double rb = sqrt(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]);
  double cx = a[1]*b[2] - a[2]*b[1], cy = a[2]*b[0] - a[0]*b[2];
  double cz = a[0]*b[1] - a[1]*b[0];
  double dot = (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]) / (ra * rb);
  return atan2(sqrt(cx*cx + cy*cy + cz*cz) / (ra * rb), dot) * 206264.806247;
}

/* One body at one instant under one ephemeris: the library's heliocentric
 * astrometric vector against B(t - tau) - S(t). Returns the angle in arcsec,
 * or -1 on a refused calculation. */
static double check(double jd, int ipl, int32 eph, const char *name, double *tau_out, double *ddist)
{
  const double c = 173.1446326846693;   /* AU per day */
  int32 base = eph | SEFLG_J2000 | SEFLG_NONUT | SEFLG_XYZ;
  int32 geom = base | SEFLG_BARYCTR | SEFLG_TRUEPOS;
  double xh[6], xs[6], xb[6], d[3], tau = 0.0, r = 0.0;
  char serr[AS_MAXCH] = "";
  int it;
  if (swe_calc(jd, ipl, base | SEFLG_HELCTR | SEFLG_NOABERR | SEFLG_NOGDEFL, xh, serr) < 0 ||
      swe_calc(jd, SE_SUN, geom, xs, serr) < 0) {
    printf("  %-10s ERROR %s\n", name, serr);
    return -1.0;
  }
  for (it = 0; it < 50; it++) {
    if (swe_calc(jd - tau, ipl, geom, xb, serr) < 0) {
      printf("  %-10s ERROR %s\n", name, serr);
      return -1.0;
    }
    d[0] = xb[0] - xs[0]; d[1] = xb[1] - xs[1]; d[2] = xb[2] - xs[2];
    r = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
    if (fabs(r / c - tau) < 1e-13) break;
    tau = r / c;
  }
  *tau_out = tau;
  *ddist = sqrt(xh[0]*xh[0] + xh[1]*xh[1] + xh[2]*xh[2]) - r;
  return Sep(xh, d);
}

int main(int argc, char **argv)
{
  static const int planets[] = { SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER, SE_SATURN,
                                 SE_URANUS, SE_NEPTUNE, SE_PLUTO };
  static const double dates[] = { 2415020.5, 2451545.0, 2461300.5 };   /* 1900, J2000, 2026-10 */
  const char *ephe = argc > 1 ? argv[1] : "../ephe";
  char dir[] = "/tmp/helltXXXXXX", path[512], serr[AS_MAXCH];
  double worst = 0.0, worst_dist = 0.0;
  int bad = 0, e, ip, id;
  FILE *pf;

  /* a fictitious body alongside the planets: Vulcan's element set from
   * seorbel.txt, written here so the gate pins its own fixture */
  if (mkdtemp(dir) == NULL) { perror("mkdtemp"); return 2; }
  snprintf(path, sizeof(path), "%s/seorbel.txt", dir);
  if ((pf = fopen(path, "w")) == NULL) { perror(path); return 2; }
  fputs("J1900,JDATE, 252.8987988 + 707550.7341 * T, 0.13744, 0.019, "
        "322.212069+1670.056*T, 47.787931-1670.056*T, 7.5, Vulcan\n", pf);
  fclose(pf);
  snprintf(path, sizeof(path), "%s:%s", dir, ephe);
  swe_set_ephe_path(path);
  swe_set_jpl_file((char *) "de200.eph");
  printf("G36: heliocentric light time measured from the Sun on every pass\n");
  for (e = 0; e < 2; e++) {
    int32 eph = e == 0 ? SEFLG_SWIEPH : SEFLG_JPLEPH;
    printf("  %s\n", e == 0 ? "SWIEPH" : "JPLEPH (de200.eph)");
    for (id = 0; id < 3; id++) {
      for (ip = 0; ip < 8 + (e == 0); ip++) {
        int ipl = ip < 8 ? planets[ip] : SE_FICT_OFFSET;   /* the one body in the file written above */
        double tau, dd, sep;
        swe_get_planet_name(ipl, serr);
        sep = check(dates[id], ipl, eph, serr, &tau, &dd);
        if (sep < 0.0) { bad++; continue; }
        printf("    %12.1f %-10s tau %9.6f d   from B(t-tau)-S(t) %10.6f\"   dist %+.1e AU%s\n",
               dates[id], serr, tau, sep, dd, sep > 1e-4 ? "   FAIL" : "");
        if (sep > worst) worst = sep;
        if (fabs(dd) > worst_dist) worst_dist = fabs(dd);
        if (sep > 1e-4) bad++;
      }
    }
  }
  unlink(path);
  snprintf(path, sizeof(path), "%s/seorbel.txt", dir);
  unlink(path);
  rmdir(dir);
  swe_close();
  printf("  worst %.6f\" (limit 0.0001\"), distance %.1e AU\n", worst, worst_dist);
  printf(bad ? "G36 FAIL: %d\n" : "G36 PASS\n", bad);
  return bad ? 1 : 0;
}
