/* fictbuiltin.c -- G64: the built-in fictitious elements are seorbel.txt's.
 *
 * The built-in Neely table of swemplan.c (SE_NEELY) had Kronos' semi-axis as
 * 64.81960, a swap of the last two digits of seorbel.txt's 64.81690 (and of
 * the original Witte elements' 64.816896) (UPSTREAM-BUGS 65). Without
 * seorbel.txt on the path the built-in table is used, and Kronos' mean
 * motion was off by 0.0043 deg per century from 1900: -0.0086 deg at 1700,
 * +0.0169 deg at 2300. The other fourteen rows agree with the file.
 *
 * The check: the 15 built-in bodies (Cupido..Pickering), heliocentric J2000
 * Moshier, geometric, at 1700, 1900, 2100, 2300 and 2500, once with the
 * shipped seorbel.txt on the path and once with an empty directory only (the
 * built-in table); the two longitudes, latitudes and distances agree to
 * 1e-9 deg and 1e-12 AU. Measured with the fix: 0 deg, 5.7e-14 AU. The
 * upstream build fails: Kronos is off by 0.0086 deg at 1700, 0.0169 deg at
 * 2300 and 0.0257 deg at 2500 (2.8e-3 AU in distance), the others 0. Run as
 *   fictbuiltin EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

#define NB 15
#define ND 5

int main(int argc, char **argv)
{
  const int32 fl = SEFLG_MOSEPH | SEFLG_HELCTR | SEFLG_J2000 | SEFLG_NONUT | SEFLG_TRUEPOS;
  const double jd[ND] = {2341972.5, 2415020.5, 2488069.5, 2561118.5, 2634167.5};
  char dir[] = "/tmp/fictbuiltXXXXXX", serr[256];
  static double ref[NB][ND][6], got[NB][ND][6];
  double wang[NB] = {0}, wdist = 0, wmax = 0;
  int ip, id, phase, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: fictbuiltin EPHEDIR\n");
    return 2;
  }
  if (mkdtemp(dir) == NULL) { perror("mkdtemp"); return 2; }
  printf("G64: the built-in fictitious elements are seorbel.txt's\n");
  for (phase = 0; phase < 2; phase++) {
    swe_set_ephe_path(phase == 0 ? argv[1] : dir);
    for (ip = 0; ip < NB; ip++)
      for (id = 0; id < ND; id++) {
        double *x = phase == 0 ? ref[ip][id] : got[ip][id];
        if (swe_calc(jd[id], SE_FICT_OFFSET + ip, fl, x, serr) < 0) {
          printf("  ERROR body %d: %s\n", ip, serr);
          bad++;
        }
      }
    swe_close();
  }
  for (ip = 0; ip < NB; ip++)
    for (id = 0; id < ND; id++) {
      double dl = fabs(swe_difdeg2n(got[ip][id][0], ref[ip][id][0]));
      double db = fabs(got[ip][id][1] - ref[ip][id][1]);
      wang[ip] = fmax(wang[ip], fmax(dl, db));
      wdist = fmax(wdist, fabs(got[ip][id][2] - ref[ip][id][2]));
    }
  for (ip = 0; ip < NB; ip++) {
    char nm[64];
    swe_get_planet_name(SE_FICT_OFFSET + ip, nm);
    wmax = fmax(wmax, wang[ip]);
    if (wang[ip] > 1e-9) {
      printf("  FAIL %-18s built-in off seorbel.txt by %.2e deg\n", nm, wang[ip]);
      bad++;
    }
  }
  printf("  worst angle %.2e deg, distance %.2e AU\n",
         wmax, wdist);
  if (wdist > 1e-12) { printf("  FAIL: a distance differs\n"); bad++; }
  rmdir(dir);
  printf(bad ? "G64 FAIL: %d\n" : "G64 PASS\n", bad);
  return bad ? 1 : 0;
}
