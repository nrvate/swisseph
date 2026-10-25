/* hposinhouse.c -- G41: swe_house_pos() puts every system's cusps at their
 * numbers and its places in their houses, north and south of the equator
 * and on it.
 *
 * Two systems answered a place in another house:
 * - Sripati ('S') is Porphyry's position plus 0.5 (its cusps are the
 *   middles of Porphyry's houses), and "if (hpos > 12) hpos = 1" answered
 *   1.0 for all of house 12 and the first half of house 1, up to 11.5
 *   houses off. Porphyry's position runs 1 .. 13, so Sripati's 1.5 ..
 *   13.5, of which 13 .. 13.5 is house 1.
 * - The horizon system ('H') turned the place by 90 - lat, where
 *   swe_houses() takes -90 - lat south of the equator and on it: there the
 *   houses counted the other way from the cusps, the middles of houses
 *   1 .. 12 at 33.87 S answering 12.50, 11.52 .. 1.48.
 *
 * The checks, tropical, at 8.55 E, UT 2378496.5, 2433282.5, 2451545.0 and
 * 2488069.5:
 * (a) at 47.37 N, 33.87 S, the equator, 60 N and 60 S, in every system
 *   letter both functions take, A .. Y and a .. y: the cusps (latitude 0)
 *   at their numbers within 1e-7 house, and the places 1e-6, 0.5 and
 *   1 - 1e-6 of the way through each house in it (k <= hpos < k + 1;
 *   Gauquelin's sectors clockwise), the first and last within 1e-3 of its
 *   cusps. One check per letter and latitude. Left out: Makransky's 'i'
 *   at 60 N and 60 S, whose cusps there are off the Sunshine circles
 *   swe_house_pos() answers; and horizon charts with the MC beyond the
 *   zenith (in the tropics), whose cusps run clockwise from the Vertex.
 * (b) at the same latitudes, Porphyry's cusps, the middles of Sripati's
 *   houses, at k + 0.5 in 'S' within 1e-7 house. One check per latitude.
 *
 * Run as
 *   hposinhouse EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

static int n, bad;

#define LON 8.55
static const double TU[] = { 2378496.5, 2433282.5, 2451545.0, 2488069.5 };
#define NTU ((int) (sizeof TU / sizeof TU[0]))
static const double LT[] = { 47.37, -33.87, 0, 60, -60 };
#define NLT ((int) (sizeof LT / sizeof LT[0]))

/* d wrapped into (-nc/2, nc/2] */
static double wrap(double d, double nc)
{
  d = fmod(d, nc);
  if (d > nc / 2)
    d -= nc;
  if (d <= -nc / 2)
    d += nc;
  return d;
}

static void verdict(int ok, const char *what, int hs, double lat, double v)
{
  n++;
  if (!ok) {
    bad++;
    printf("    FAIL %s, '%c' at %.2f: %.3e\n", what, hs, lat, v);
  }
}

/* (a) every system's cusps at their numbers, its places in their houses */
static int check_a(swe_ctx *c)
{
  static const char HS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxy";
  static const double FR[3] = { 1e-6, 0.5, 1 - 1e-6 };
  int k, l, it, i, j, n0 = n, b0 = bad, nskip = 0;
  double wc = 0;
  char serr[256];
  for (l = 0; l < NLT; l++) {
    int n1 = n, b1 = bad;
    for (k = 0; HS[k] != '\0'; k++) {
      int hs = HS[k], cw = hs == 'G' || hs == 'g', nc = cw ? 36 : 12;
      double w = 0;
      if (hs == 'i' && fabs(LT[l]) == 60)
        continue;
      for (it = 0; it < NTU; it++) {
        double cu[37], am[10], x[6];
        if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LT[l], LON, hs, cu, am, NULL, NULL,
                             serr) == ERR
            || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
          printf("    ERROR '%c' at %.2f, UT %.1f: %s\n", hs, LT[l], TU[it], serr);
          return -1;
        }
        if (hs == 'H' || hs == 'h') {
          /* the horizon houses take the MC on the equator's side of the
           * zenith; with the MC beyond it they run clockwise */
          double xm[3] = { am[1], 0, 1 };
          swe_cotrans(xm, xm, -x[0]);
          if (LT[l] > 0 ? xm[1] > LT[l] : xm[1] < LT[l]) {
            nskip++;
            continue;
          }
        }
        for (i = 1; i <= nc; i++) {
          double span = cw ? swe_degnorm(cu[i] - cu[i % nc + 1])
                           : swe_degnorm(cu[i % nc + 1] - cu[i]);
          double xp[2], hp;
          xp[0] = cu[i];
          xp[1] = 0;
          hp = swe_house_pos_r(c, am[2], LT[l], x[0], hs, xp, serr);
          wc = fmax(wc, fabs(wrap(hp - i, nc)));
          if (!(fabs(wrap(hp - i, nc)) <= 1e-7))
            w = fmax(w, fabs(wrap(hp - i, nc)));
          if (span == 0)
            continue;
          for (j = 0; j < 3; j++) {
            xp[0] = swe_degnorm(cu[i] + (cw ? -1 : 1) * FR[j] * span);
            hp = swe_house_pos_r(c, am[2], LT[l], x[0], hs, xp, serr);
            if (!(hp >= i && hp < i + 1)
                || (j == 0 && hp - i > 1e-3) || (j == 2 && i + 1 - hp > 1e-3))
              w = fmax(w, fabs(wrap(hp - (i + FR[j]), nc)));
          }
        }
      }
      verdict(w == 0, "a cusp off its number or a place outside its house (house)", hs, LT[l],
              w);
    }
    printf("    at %6.2f: %d letters, %d fail\n", LT[l], n - n1, bad - b1);
  }
  printf("    the cusps at most %.3e house off their numbers; %d horizon charts with the MC\n"
         "    beyond the zenith left out\n", wc, nskip);
  printf("  (a) the cusps at their numbers and the places in their houses: %d checks, %d fail\n",
         n - n0, bad - b0);
  return 0;
}

/* (b) Porphyry's cusps at the middles of Sripati's houses */
static int check_b(swe_ctx *c)
{
  int l, it, i, n0 = n, b0 = bad;
  char serr[256];
  for (l = 0; l < NLT; l++) {
    double w = 0;
    for (it = 0; it < NTU; it++) {
      double cp[13], am[10], x[6];
      if (swe_houses_ex2_r(c, TU[it], SEFLG_SWIEPH, LT[l], LON, 'O', cp, am, NULL, NULL,
                           serr) == ERR
          || swe_calc_ut_r(c, TU[it], SE_ECL_NUT, SEFLG_SWIEPH, x, serr) == ERR) {
        printf("    ERROR 'O' at %.2f, UT %.1f: %s\n", LT[l], TU[it], serr);
        return -1;
      }
      for (i = 1; i <= 12; i++) {
        double xp[2] = { cp[i], 0 };
        double hp = swe_house_pos_r(c, am[2], LT[l], x[0], 'S', xp, serr);
        w = fmax(w, fabs(wrap(hp - (i + 0.5), 12)));
        if (!(hp >= 1 && hp < 13))
          w = fmax(w, 1);
      }
    }
    verdict(w <= 1e-7, "Porphyry's cusps off Sripati's houses' middles (house)", 'S', LT[l], w);
    printf("    'S' at %6.2f: Porphyry's cusps %.3e house off Sripati's middles\n", LT[l], w);
  }
  printf("  (b) Sripati's middles: %d checks, %d fail\n", n - n0, bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  swe_ctx *c;
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: hposinhouse EPHEDIR\n");
    return 2;
  }
  c = swe_ctx_new();
  if (c == NULL) {
    printf("G41 FAIL: no context\n");
    return 1;
  }
  swe_set_ephe_path_r(c, argv[1]);
  printf("G41: swe_house_pos() at every system's cusps and in its houses, both hemispheres\n");
  r = check_a(c);
  if (r == 0)
    r = check_b(c);
  swe_ctx_free(c);
  if (r < 0) {
    printf("G41 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G41 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG41 PASS\n", n);
  return 0;
}
