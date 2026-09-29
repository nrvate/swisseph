/* sidnod.c -- G30: a sidereal node or apsis is the mean ecliptic of date
 * less the ayanamsa, like a sidereal planet.
 *
 * swe_calc() sets SEFLG_NONUT whenever SEFLG_SIDEREAL is set (plaus_iflag()),
 * so a sidereal planet, node or apogee from it is the tropical mean-of-date
 * longitude less the ayanamsa. swe_nod_aps() has no such line, so its
 * sidereal nodes and apsides carried the nutation in longitude on top: the
 * whole of delta-psi, +12.82" for the Moon's mean node on 1990-06-16.
 *
 * The check needs no golden file: for the Moon, Mars and Jupiter, the mean
 * and osculating methods, all four points, three instants and two ayanamsas,
 * the sidereal longitude equals the NONUT tropical longitude less
 * swe_get_ayanamsa_ex() within 1e-9 degrees (3.6 microarcseconds). The
 * compat build keeps upstream's behaviour and fails all of them by delta-psi.
 * Moshier, no files. Run as
 *   sidnod
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

int main(void)
{
  static const int rgipl[] = {SE_MOON, SE_MARS, SE_JUPITER};
  static const char *rgszPl[] = {"Moon", "Mars", "Jupiter"};
  static const double rgjd[] = {2415020.5, 2448058.0, 2461300.5};
  static const int rgsid[] = {SE_SIDM_FAGAN_BRADLEY, SE_SIDM_LAHIRI};
  static const char *rgszPt[] = {"asc", "desc", "peri", "aphe"};
  int32 fl = SEFLG_MOSEPH | SEFLG_SPEED;
  double worst = 0.0;
  int ip, ij, is, im, k, bad = 0, n = 0;
  char serr[256];

  printf("G30: sidereal nodes and apsides carry no nutation\n");
  for (is = 0; is < 2; is++)
    for (ij = 0; ij < 3; ij++)
      for (ip = 0; ip < 3; ip++)
        for (im = 0; im < 2; im++) {
          double xt[4][6], xs[4][6], xa[4][6], xp[4][6], ay;
          int meth = im ? SE_NODBIT_OSCU : SE_NODBIT_MEAN;

          swe_set_sid_mode(rgsid[is], 0, 0);
          if (swe_get_ayanamsa_ex(rgjd[ij], fl | SEFLG_NONUT, &ay, serr) < 0 ||
              swe_nod_aps(rgjd[ij], rgipl[ip], fl | SEFLG_NONUT, meth,
                xt[0], xt[1], xt[2], xt[3], serr) < 0 ||
              swe_nod_aps(rgjd[ij], rgipl[ip], fl | SEFLG_SIDEREAL, meth,
                xs[0], xs[1], xs[2], xs[3], serr) < 0) {
            printf("  ERROR %s\n", serr);
            bad++;
            continue;
          }
          (void) xa; (void) xp;
          for (k = 0; k < 4; k++) {
            double want = swe_degnorm(xt[k][0] - ay);
            double d = fabs(swe_difdeg2n(xs[k][0], want));

            n++;
            if (d > worst) worst = d;
            if (d > 1e-9) {
              if (bad < 8)
                printf("  FAIL %-7s %-4s %s jd %.1f sid %d: %.9f vs %.9f "
                  "(%+.3f\")\n", rgszPl[ip], rgszPt[k], im ? "osc" : "mean",
                  rgjd[ij], rgsid[is], xs[k][0], want, (xs[k][0] - want) * 3600);
              bad++;
            }
          }
        }
  printf("  %d longitudes, worst %.3e degrees (%.3f microarcsec)\n", n, worst,
         worst * 3.6e9);
  printf(bad ? "G30 FAIL: %d\n" : "G30 PASS\n", bad);
  return bad ? 1 : 0;
}
