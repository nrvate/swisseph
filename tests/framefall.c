/* framefall.c -- G170: across a step of the frame's models, the Swiss
 * request's Moon fallen to Moshier's is read at its light-time instant in
 * its instant's side's frame, and a topocentric star's site for its
 * aberration rate on its instant's side.
 *
 * The defect (UPSTREAM-BUGS 258): with the Moon files missing and the
 * fallback allowed, a Swiss request's Moon is Moshier's, and its
 * light-time place is read again at the instant the light left, of date
 * and put into J2000 by the frame there. Within the light time after a
 * step of the frame's models (here IAU 1976's window under Vondrak,
 * "0,9,1", at J2000 -+ 2 centuries) that was the other side's model's:
 * after + 2 centuries the Moon's longitude 1.715e-4 deg off the side's
 * model alone's ("0,9,9"; Moshier's own within 1e-12), latitude 7.8e-6,
 * its speed 2.29e-2 deg/day off its places' rate (after - 2: 1.588e-4,
 * 2.12e-2) and up to 1.27e-5 off the side's model's; heliocentric and
 * barycentric, within 500 s, 4.6e-7 deg and 6.1e-5 deg/day.
 *
 * The defect (UPSTREAM-BUGS 259): a topocentric star's aberration rate
 * reads the site at tjd - dm and tjd + dp (1e-5 day), of date and put into
 * J2000 by the frame at its instant, and within 1e-5 day of a step one
 * read was the other model's, on every ephemeris: Polaris's topocentric
 * right ascension speed 8.33e-5 deg/day off its places' rate within 1e-5
 * day of J2000 + 2 centuries under "0,9,1" (latitude 1.9e-6), 3.41e-4
 * with the IERS 2010 sidereal time, 2.52e-4 at - 2; under
 * SEFLG_JPLHOR_APPROX across its 1962 Jan 20 step 9.49e-5.
 *
 * Check (a), 258: a directory of links to EPHEDIR's files but the Moon's
 * (semo*), the fallback allowed, the Moon with SEFLG_SPEED and no
 * ephemeris flag under "0,9,1": ecliptic of date, equatorial, J2000,
 * without aberration, heliocentric, barycentric, and topocentric (Zurich)
 * with the IERS 2010 sidereal time (the long-term one is left out: within
 * 0.3 s after - 2 centuries it is another model's, a defect of its own,
 * see UPSTREAM-BUGS 258's notes); at J2000 -+ 2 centuries -+ 2^-k day, k
 * = 13..26, each side judged on its 14 instants:
 *  (a1) the place bit for bit the side's model alone's ("0,1,1" inside,
 *       "0,9,9" outside), each in a fresh context, and the whole answer
 *       in one warm context of "0,9,1" bit for bit the fresh one's;
 *  (a2) the longitude's and latitude's speeds within 1e-8 deg/day of the
 *       side's model alone's;
 *  (a3) and of a one-sided five-point difference over 2^-6 day of the
 *       places on the instant's side, within 2e-8 (the reference's own
 *       noise on this Moon: 6.4e-9 under the side's model alone within
 *       a day of the edges, 1.6e-9 under the default models), the
 *       topocentric Moon left out (its one-sided difference carries the
 *       diurnal parallax's curvature, 5e-5).
 * 28 + 28 + 24 checks.
 *
 * Check (b), 259: Polaris (swe_fixstar2()) topocentric at Zurich with
 * SEFLG_SPEED, ecliptic and equatorial, under "0,9,1" with the IERS 2010
 * sidereal time at J2000 + 2 centuries on the Swiss files and Moshier and
 * at - 2 on those and de200, with the long-term sidereal time at + 2 on
 * the Swiss files and Moshier, and on de200 with SEFLG_JPLHOR_APPROX at
 * 1962 Jan 20 under the default models; at the step -+ 2^-k day, k =
 * 12..26: the longitude's and latitude's speeds against a one-sided
 * five-point difference over 2^-9 day of the places on the instant's
 * side, each side judged on the worst of its 15 instants: 32 checks
 * within 1e-8 deg/day.
 *
 * Measured after the fix: (a1) all bit for bit; (a2) within 8.5e-14, the
 * topocentric Moon 4.1e-9 (its site's acceleration over -+ 2^-6 day,
 * left, under the floor); (a3) within 9.0e-9; (b) within 2.4e-9. Before
 * the fix 72 of the 112 fail: every flag set's (a1), (a2) and (a3) after
 * both edges (the places up to 1.75e-4 deg off, the speeds 1.27e-5 off
 * the side's model's and 2.34e-2 off the places' rate), and all 32 of
 * (b) (8.33e-5 to 3.41e-4 deg/day in right ascension, 1.9e-6 to 7.9e-6
 * in the ecliptic). The upstream build fails all 112 (its
 * Moshier speeds differences of places across the edge, the Moon's 5.4
 * deg/day, Polaris's right ascension 2.3; on the files its star's site
 * read 1e-5 day before the instant, across, 6.8e-4).
 *
 * Run as
 *   framefall EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

#define J2000 2451545.0
#define EDGE_P (J2000 + 73050)	/* J2000 + 2 centuries, the window's */
#define EDGE_M (J2000 - 73050)	/* J2000 - 2 centuries, the window's */
#define STEP_1962 2437684.5	/* SEMOD_JPLHORA_3's step, the left side's */
#define KMIN_A 13
#define KMIN_B 12
#define KMAX 26

static const char *ephe;
static int bad = 0;
static double worst_a1 = 0, worst_a2 = 0, worst_a3 = 0, worst_b = 0;

static swe_ctx *ctx_for(const char *dir, const char *mod)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, (char *) dir);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_ephe_fallback_r(c, 1);
  if (mod != NULL)
    swe_set_astro_models_r(c, (char *) mod, 0);
  swe_set_topo_r(c, 8.55, 47.38, 400);
  return c;
}

static double dlon(double a, double b)
{
  return swe_difdeg2n(a, b);
}

/* one-sided five-point derivative at t of f(t + s j h), j = 0..4 */
static double d5(const double *f, double s, double h)
{
  return s * (-25 * f[0] + 48 * f[1] - 36 * f[2] + 16 * f[3] - 3 * f[4]) / (12 * h);
}

/* ---- check (a): the Moon files missing, the fallback allowed ---- */

static int build(char *dir)
{
  DIR *d;
  struct dirent *e;
  char src[PATH_MAX + 300], dst[PATH_MAX + 300], abs_ephe[PATH_MAX];
  if (realpath(ephe, abs_ephe) == NULL || mkdtemp(dir) == NULL || (d = opendir(abs_ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || strncmp(e->d_name, "semo", 4) == 0)
      continue;
    snprintf(src, sizeof src, "%s/%s", abs_ephe, e->d_name);
    snprintf(dst, sizeof dst, "%s/%s", dir, e->d_name);
    if (symlink(src, dst) != 0) {
      closedir(d);
      return -1;
    }
  }
  closedir(d);
  return 0;
}

static void unbuild(const char *dir)
{
  DIR *d = opendir(dir);
  struct dirent *e;
  char p[PATH_MAX + 300];
  if (d == NULL)
    return;
  while ((e = readdir(d)) != NULL)
    if (e->d_name[0] != '.') {
      snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
      unlink(p);
    }
  closedir(d);
  rmdir(dir);
}

static const struct { int32 fl; const char *sidt; const char *nm; } rgA[] = {
  { 0, "", "ecliptic of date" },
  { SEFLG_EQUATORIAL, "", "equatorial" },
  { SEFLG_J2000 | SEFLG_EQUATORIAL, "", "J2000 equatorial" },
  { SEFLG_NOABERR, "", "without aberration" },
  { SEFLG_HELCTR, "", "heliocentric" },
  { SEFLG_BARYCTR, "", "barycentric" },
  { SEFLG_TOPOCTR, ",0,0,0,0,3", "topocentric, IERS 2010 sid. time" },
};
#define NA ((int) (sizeof rgA / sizeof rgA[0]))

static int moon(const char *dir, swe_ctx *warm, const char *mod, double t, int32 fl, double *x)
{
  char serr[AS_MAXCH];
  swe_ctx *c = warm != NULL ? warm : ctx_for(dir, mod);
  int r = swe_calc_r(c, t, SE_MOON, fl, x, serr);
  if (warm == NULL)
    swe_ctx_free(c);
  return r;
}

static void check_a(const char *dir)
{
  const double edges[2] = {EDGE_P, EDGE_M};
  int e, s, f, k, j;
  for (f = 0; f < NA; f++)
    for (e = 0; e < 2; e++)
      for (s = -1; s <= 1; s += 2) {
        char m91[64], min[64], mout[64];
        const char *where = e ? "- 2 cy" : "+ 2 cy", *side = s < 0 ? "before" : "after ";
        int nb1 = 0, topo = (rgA[f].fl & SEFLG_TOPOCTR) != 0;
        double w2 = 0, w3 = 0;
        swe_ctx *warm;
        snprintf(m91, sizeof m91, "0,9,1%s", rgA[f].sidt);
        snprintf(min, sizeof min, "0,1,1%s", rgA[f].sidt);
        snprintf(mout, sizeof mout, "0,9,9%s", rgA[f].sidt);
        warm = ctx_for(dir, m91);
        for (k = KMIN_A; k <= KMAX; k++) {
          double t = edges[e] + s * ldexp(1, -k), h = 1.0 / 64, d;
          double x[6], xw[6], xr[6], p[5][6], f0[5], f1[5];
          int inside = fabs((t - J2000) / 36525.0) <= 2;
          int32 fl = rgA[f].fl | SEFLG_SPEED;
          int r = moon(dir, NULL, m91, t, fl, x);
          int rw = moon(dir, warm, m91, t, fl, xw);
          int rr = moon(dir, NULL, inside ? min : mout, t, fl, xr);
          /* (a1): the place the side's model alone's, the warm answer the
           * fresh one */
          if (r < 0 || r != rr || rw != r || memcmp(x, xr, 3 * sizeof(double)) != 0
              || memcmp(x, xw, sizeof x) != 0) {
            d = fabs(dlon(x[0], xr[0]));
            if (fabs(x[1] - xr[1]) > d)
              d = fabs(x[1] - xr[1]);
            if (d > worst_a1)
              worst_a1 = d;
            nb1++;
          }
          /* (a2): the speed the side's model alone's */
          for (j = 3; j <= 4; j++)
            if (fabs(x[j] - xr[j]) > w2)
              w2 = fabs(x[j] - xr[j]);
          /* (a3): and the rate of the places on the instant's side */
          if (topo)
            continue;
          for (j = 0; j < 5; j++)
            moon(dir, NULL, m91, t + s * j * h, rgA[f].fl, p[j]);
          for (j = 0; j < 5; j++) {
            f0[j] = p[0][0] + dlon(p[j][0], p[0][0]);
            f1[j] = p[j][1];
          }
          d = fabs(x[3] - d5(f0, s, h));
          if (d > w3) w3 = d;
          d = fabs(x[4] - d5(f1, s, h));
          if (d > w3) w3 = d;
        }
        swe_ctx_free(warm);
        if (w2 > worst_a2)
          worst_a2 = w2;
        if (w3 > worst_a3)
          worst_a3 = w3;
        if (nb1 > 0) {
          bad++;
          printf("  FAIL (a1) %-34s %s, %s: %d of %d not the side's model's place, or warm not fresh\n",
                 rgA[f].nm, where, side, nb1, KMAX - KMIN_A + 1);
        }
        if (!(w2 <= 1e-8)) {
          bad++;
          printf("  FAIL (a2) %-34s %s, %s: speed %.3e deg/day off the side's model's\n",
                 rgA[f].nm, where, side, w2);
        }
        if (!topo && !(w3 <= 2e-8)) {
          bad++;
          printf("  FAIL (a3) %-34s %s, %s: speed %.3e deg/day off the places' rate\n",
                 rgA[f].nm, where, side, w3);
        }
      }
}

/* ---- check (b): a topocentric star's aberration rate ---- */

static const struct { double step; int32 eph; const char *mod; const char *nm; } rgB[] = {
  { EDGE_P, SEFLG_SWIEPH, "0,9,1,0,0,0,0,3", "Swiss, \"0,9,1\" IERS 2010, + 2 cy" },
  { EDGE_P, SEFLG_MOSEPH, "0,9,1,0,0,0,0,3", "Moshier, \"0,9,1\" IERS 2010, + 2 cy" },
  { EDGE_M, SEFLG_SWIEPH, "0,9,1,0,0,0,0,3", "Swiss, \"0,9,1\" IERS 2010, - 2 cy" },
  { EDGE_M, SEFLG_MOSEPH, "0,9,1,0,0,0,0,3", "Moshier, \"0,9,1\" IERS 2010, - 2 cy" },
  { EDGE_M, SEFLG_JPLEPH, "0,9,1,0,0,0,0,3", "de200, \"0,9,1\" IERS 2010, - 2 cy" },
  { EDGE_P, SEFLG_SWIEPH, "0,9,1", "Swiss, \"0,9,1\", + 2 cy" },
  { EDGE_P, SEFLG_MOSEPH, "0,9,1", "Moshier, \"0,9,1\", + 2 cy" },
  { STEP_1962, SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX, NULL, "de200 JPLHOR_APPROX, 1962 Jan 20" },
};
#define NB ((int) (sizeof rgB / sizeof rgB[0]))

static void star(swe_ctx *c, double t, int32 fl, double *x)
{
  char s[64] = "Polaris", serr[AS_MAXCH];
  if (swe_fixstar2_r(c, s, t, fl, x, serr) < 0) {
    printf("  ERR Polaris at %.10f: %s\n", t, serr);
    x[0] = x[1] = x[2] = x[3] = x[4] = x[5] = 1e9;
  }
}

static void check_b(void)
{
  int b, s, q, k, j;
  for (b = 0; b < NB; b++)
    for (q = 0; q < 2; q++)
      for (s = -1; s <= 1; s += 2) {
        int32 fl = rgB[b].eph | SEFLG_TOPOCTR | (q ? SEFLG_EQUATORIAL : 0);
        double w = 0, h = 1.0 / 512;
        swe_ctx *c = ctx_for(ephe, rgB[b].mod);
        for (k = KMIN_B; k <= KMAX; k++) {
          double t = rgB[b].step + s * ldexp(1, -k), x[6], p[5][6], f0[5], f1[5], d;
          star(c, t, fl | SEFLG_SPEED, x);
          for (j = 0; j < 5; j++)
            star(c, t + s * j * h, fl, p[j]);
          for (j = 0; j < 5; j++) {
            f0[j] = p[0][0] + dlon(p[j][0], p[0][0]);
            f1[j] = p[j][1];
          }
          d = fabs(x[3] - d5(f0, s, h));
          if (d > w) w = d;
          d = fabs(x[4] - d5(f1, s, h));
          if (d > w) w = d;
        }
        swe_ctx_free(c);
        if (w > worst_b)
          worst_b = w;
        if (!(w <= 1e-8)) {
          bad++;
          printf("  FAIL (b) %-40s %s, %s: speed %.3e deg/day off the places' rate\n",
                 rgB[b].nm, q ? "equatorial" : "ecliptic", s < 0 ? "before" : "after ", w);
        }
      }
}

int main(int argc, char **argv)
{
  char dir[] = "/tmp/framefallXXXXXX";
  if (argc < 2) {
    fprintf(stderr, "usage: framefall EPHEDIR\n");
    return 2;
  }
  ephe = argv[1];
  if (build(dir) != 0) {
    printf("G170 FAIL: cannot build the directory without the Moon files\n");
    return 1;
  }
  check_a(dir);
  unbuild(dir);
  check_b();
  printf("  worst: (a1) %.3e deg, (a2) %.3e deg/day, (a3) %.3e, (b) %.3e\n",
         worst_a1, worst_a2, worst_a3, worst_b);
  printf(bad ? "G170 FAIL: %d\n" : "G170 PASS\n", bad);
  return bad != 0;
}
