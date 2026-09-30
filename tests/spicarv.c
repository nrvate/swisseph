/* spicarv.c -- G31: the built-in Spica record carries SIMBAD's radial velocity.
 *
 * swe_fixstar2() answers the names "Spica" and "spica" from a record built
 * into sweph.c (get_builtin_star(), there for SE_SIDM_TRUE_CITRA) and never
 * from sefstars.txt, so a catalogue file cannot correct that star. The
 * record carried the radial velocity +1 km/s of an old SIMBAD vintage; SIMBAD
 * now gives -3.31 (2023ApJS..266...11B, quality A). The radial velocity
 * moves no direction that matters, and is the whole of the distance rate.
 *
 * The check builds a catalogue file whose star "Citra" is the record's line
 * with -3.31, so it needs no golden file, and requires the built-in "Spica"
 * to have the same distance rate, differenced from its own distances (five
 * points, h = 1/1024 day), to 1e-9 AU/day. Before the fix they differ by
 * 2.5e-3 AU/day, which is the 4.3 km/s. Moshier, no ephemeris files. Run as
 *   spicarv
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

static double rate(const char *name, double jd)
{
  const double h = 1.0 / 1024.0;
  const double dj[4] = {-2 * h, -h, h, 2 * h};
  double x[4][6], d;
  char s[256], serr[256];
  int k;

  for (k = 0; k < 4; k++) {
    snprintf(s, sizeof(s), "%s", name);
    if (swe_fixstar2(s, jd + dj[k], SEFLG_MOSEPH, x[k], serr) < 0) {
      printf("%s: %s\n", name, serr);
      exit(1);
    }
  }
  d = (x[0][2] - 8 * x[1][2] + 8 * x[2][2] - x[3][2]) / (12 * h);
  return d;
}

int main(void)
{
  const char *dir = ".spicarv.d";
  char path[256];
  FILE *f;
  double a, b, worst = 0.0;
  const double rgjd[] = {2415020.5, 2451545.0, 2461300.5};
  int i, bad = 0;

  printf("G31: the built-in Spica record's radial velocity\n");
  snprintf(path, sizeof(path), "mkdir -p %s", dir);
  if (system(path) != 0) return 1;
  snprintf(path, sizeof(path), "%s/sefstars.txt", dir);
  f = fopen(path, "w");
  if (f == NULL) return 1;
  fprintf(f, "Citra,alVir,ICRS,13,25,11.57937,-11,09,40.7501,-42.35,-30.67,"
             "-3.31,13.06,0.97,-10,3672\n");
  fclose(f);
  swe_set_ephe_path(dir);
  for (i = 0; i < 3; i++) {
    double d;

    a = rate("Spica", rgjd[i]);
    b = rate("Citra", rgjd[i]);
    d = fabs(a - b);
    if (d > worst) worst = d;
    if (d > 1e-9) bad++;
    printf("  jd %.1f: built-in %.9f, file %.9f AU/day, differ %.3e%s\n",
           rgjd[i], a, b, d, d > 1e-9 ? "  <-- BAD" : "");
  }
  snprintf(path, sizeof(path), "rm -rf %s", dir);
  (void)system(path);
  if (bad) { printf("FAIL: %d of 3 differ\n", bad); return 1; }
  printf("OK: worst %.3e AU/day\n", worst);
  return 0;
}
