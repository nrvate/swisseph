/* moonfile.c -- G117: with the Swiss Moon file missing, an answer that
 * depends on the Moon is refused under the strict setting, and with the
 * fallback allowed the Moon says it is Moshier's.
 *
 * The defect (UPSTREAM-BUGS 120): with semo_12.se1 missing, sweplan() took
 * the Moon from Moshier, made the Earth from the EMB less that Moon, wrote
 * "using Moshier eph. for moon" into serr and carried on. swe_calc() of the
 * Moon returned SEFLG_SWIEPH (258 with SEFLG_SPEED) with Moshier's place,
 * within 1.1e-10 deg of SEFLG_MOSEPH's, 0.6-5.0" off the file's at
 * 1338-1776, and every geocentric planet, the Sun, a star and an orbit
 * about the Earth were seen from that Earth. The flags said Swiss, so swi_refuse_downgrade() saw no
 * substitution, and the strict default of this fork answered all of them.
 *
 * The check, a directory in /tmp of links to EPHEDIR's files without
 * semo_12.se1, four instants in 1200-1800 (binary fractions), against the
 * complete directory, every call in a fresh context unless said:
 *  (a) strict: the Moon (Swiss, no ephemeris flag, swe_calc_ut(),
 *      heliocentric), the Sun, Mars geocentric and topocentric (with
 *      SEFLG_HELCTR or SEFLG_BARYCTR too, and the Sun so), the Earth
 *      heliocentric, the true node, swe_fixstar2() and swe_fixstar() of
 *      Aldebaran and swe_calc_orbel() about the Earth are refused; and again in one warm context after a
 *      heliocentric Mars at the same instant (whose save area holds the
 *      Earth made from Moshier's Moon), each asked twice;
 *  (b) strict: what does not depend on the Moon or the Earth answers,
 *      bit for bit and flag for flag as with the complete directory, fresh
 *      and in that warm context: Mars heliocentric, Jupiter and the Sun
 *      barycentric, the mean node, Aldebaran heliocentric, an orbit about
 *      the Sun heliocentric;
 *  (c) fallback allowed: (a)'s calls answer; the Moon's flag is
 *      SEFLG_MOSEPH, not SEFLG_SWIEPH, and its geocentric place is
 *      SEFLG_MOSEPH's within 1e-9 deg (swe_calc_ut() then takes Moshier's
 *      delta t too: before, the files' delta t with Moshier's Moon, 1.9e-3
 *      deg off SEFLG_MOSEPH's at 1338); the Sun, Mars, Aldebaran and the
 *      orbit keep SEFLG_SWIEPH (their bodies and the EMB are the files';
 *      only the Earth's Moon offset, 1/82 of the Moon's error, is
 *      Moshier's) and say "Moshier" in serr, within 1e-6 deg of the
 *      complete directory's (the heliocentric Moon's place is G108 (5)'s
 *      to check, see rgDep);
 *  (d) the complete directory: every call of (a) answers with the flag
 *      asked;
 *  (e) UPSTREAM-BUGS 194: SEFLG_TOPOCTR with SEFLG_HELCTR or
 *      SEFLG_BARYCTR is topocentric (plaus_iflag()), so (a) refuses Mars
 *      and the Sun asked so; and in one context such a Mars, then
 *      SEFLG_TOPOCTR alone, the second's return and place bit for bit
 *      those of SEFLG_TOPOCTR in a fresh context, strict (refused) and with
 *      the fallback allowed (answered). Before, the strict test read the
 *      centre off the caller's bits and answered from the Earth of
 *      Moshier's Moon (Mars at 1900 32770, 9e-10 deg off the files'), and
 *      the save area answered the SEFLG_TOPOCTR request after it.
 * A refusal is ret < 0 with "Moshier" in serr (this entry's message, or
 * the true node's own downgrade). Measured: 472 checks, none fail; (c)'s
 * Moon 1.5e-11 deg off SEFLG_MOSEPH's, its planets 3.4e-8 deg off the
 * files'. The build before the fix fails 152: the 44 of (a), its 88 warm,
 * and 20 of (c)'s Moon (the flag, and swe_calc_ut()'s place); the
 * SWE_UPSTREAM_COMPAT build 156, four of (b)'s warm rows too (the
 * restored defects of its caches). Before bug 194's fix 44 of the 472
 * fail: (a)'s three combined rows, 12 fresh and 24 warm, and (e)'s 8
 * strict; the SWE_UPSTREAM_COMPAT build 212 (160 of the 372 before, with
 * the topocentric rows' lost note of (c)).
 * Run as
 *   moonfile EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

#define EPHMASK (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)

enum { CALC, CALC_UT, STAR2, STAR, ORBEL_E, ORBEL_S };
/* moon: 1 the Moon, its place Moshier's with the fallback; 2 the Moon, its
 * flag only (heliocentric: the fallback's place is the Moshier Moon with
 * the files' Earth, G108 (5), not Moshier's own heliocentric Moon);
 * 0 a body seen from the Earth, Swiss and noted; 3 the same, its note not
 * required (unused since topocentric speeds keep the note); -1 no check of
 * the flag (the true node goes to Moshier itself) */
static const struct { const char *sz; int kind, ipl; int32 fl; int moon; } rgDep[] = {
  {"Moon, Swiss", CALC, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, 1},
  {"Moon, no ephemeris flag", CALC, SE_MOON, SEFLG_SPEED, 1},
  {"Moon, swe_calc_ut()", CALC_UT, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, 1},
  {"Moon, heliocentric", CALC, SE_MOON, SEFLG_SWIEPH | SEFLG_HELCTR, 2},
  {"Sun", CALC, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, 0},
  {"Mars", CALC, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, 0},
  {"Mars, topocentric", CALC, SE_MARS, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED, 0},
  /* bug 194: SEFLG_TOPOCTR turns the other centres off (plaus_iflag()) */
  {"Mars, TOPOCTR | HELCTR", CALC, SE_MARS, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_HELCTR | SEFLG_SPEED, 0},
  {"Mars, TOPOCTR | BARYCTR", CALC, SE_MARS, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_BARYCTR, 0},
  {"Sun, TOPOCTR | BARYCTR", CALC, SE_SUN, SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_BARYCTR | SEFLG_SPEED, 0},
  {"Earth, heliocentric", CALC, SE_EARTH, SEFLG_SWIEPH | SEFLG_HELCTR, 0},
  {"true node", CALC, SE_TRUE_NODE, SEFLG_SWIEPH | SEFLG_SPEED, -1},
  {"Aldebaran, swe_fixstar2()", STAR2, 0, SEFLG_SWIEPH | SEFLG_SPEED, 0},
  {"Aldebaran, swe_fixstar()", STAR, 0, SEFLG_SWIEPH | SEFLG_SPEED, 0},
  {"orbit about the Earth", ORBEL_E, 0, SEFLG_SWIEPH | SEFLG_SPEED, 0},
};
static const struct { const char *sz; int kind, ipl; int32 fl; } rgInd[] = {
  {"Mars, heliocentric", CALC, SE_MARS, SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_SPEED},
  {"Jupiter, barycentric", CALC, SE_JUPITER, SEFLG_SWIEPH | SEFLG_BARYCTR | SEFLG_SPEED},
  {"Sun, barycentric", CALC, SE_SUN, SEFLG_SWIEPH | SEFLG_BARYCTR | SEFLG_SPEED},
  {"mean node", CALC, SE_MEAN_NODE, SEFLG_SWIEPH | SEFLG_SPEED},
  {"Aldebaran, heliocentric", STAR2, 0, SEFLG_SWIEPH | SEFLG_HELCTR},
  {"orbit about the Sun, helioc.", ORBEL_S, 0, SEFLG_SWIEPH | SEFLG_HELCTR | SEFLG_SPEED},
};
/* (e): SEFLG_TOPOCTR with another centre, then SEFLG_TOPOCTR (bug 194) */
static const struct { const char *sz; int ipl; int32 ctr; } rgComb[] = {
  {"Mars, TOPOCTR | HELCTR, then TOPOCTR", SE_MARS, SEFLG_HELCTR},
  {"Mars, TOPOCTR | BARYCTR, then TOPOCTR", SE_MARS, SEFLG_BARYCTR},
};
#define NDEP ((int) (sizeof rgDep / sizeof rgDep[0]))
#define NCOMB ((int) (sizeof rgComb / sizeof rgComb[0]))
#define NIND ((int) (sizeof rgInd / sizeof rgInd[0]))

static const double rgJd[4] = {2210000.25, 2268936.5, 2300000.75, 2370000.5};
static int nbad, nrow;

struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; };

static swe_ctx *open_ctx(const char *dir, int fallback)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, dir);
  swe_set_ephe_fallback_r(c, fallback);
  swe_set_topo_r(c, 8.55, 47.37, 400);
  return c;
}

static struct ans ask(swe_ctx *c, int kind, int ipl, int32 fl, double jd)
{
  static const double ele[6] = {50, 0.00684, 0.1587, 8, 136, 2.5};
  static const double els[6] = {10, 0.387, 0.2056, 29, 48, 7.0};
  struct ans a;
  char star[64] = "Aldebaran";
  memset(&a, 0, sizeof a);
  switch (kind) {
    case CALC: a.r = swe_calc_r(c, jd, ipl, fl, a.x, a.serr); break;
    case CALC_UT: a.r = swe_calc_ut_r(c, jd, ipl, fl, a.x, a.serr); break;
    case STAR2: a.r = swe_fixstar2_r(c, star, jd, fl, a.x, a.serr); break;
    case STAR: a.r = swe_fixstar_r(c, star, jd, fl, a.x, a.serr); break;
    case ORBEL_E: a.r = swe_calc_orbel_r(c, jd, ele, SE_EARTH, SE_ORBEL_EQ_J2000, 0, fl, a.x, a.serr); break;
    default: a.r = swe_calc_orbel_r(c, jd, els, SE_SUN, SE_ORBEL_EQ_J2000, 0, fl, a.x, a.serr); break;
  }
  return a;
}

static struct ans ask_fresh(const char *dir, int fallback, int kind, int ipl, int32 fl, double jd)
{
  swe_ctx *c = open_ctx(dir, fallback);
  struct ans a = ask(c, kind, ipl, fl, jd);
  swe_ctx_free(c);
  return a;
}

static void check(int ok, const char *part, const char *sz, double jd, const struct ans *a, const char *why)
{
  nrow++;
  if (ok)
    return;
  nbad++;
  if (nbad <= 40)
    printf("    FAIL (%s) %-30s jd %.2f: ret %d, %s; serr '%.80s'\n", part, sz, jd, a->r, why, a->serr);
}

static int refused(const struct ans *a)
{
  return a->r < 0 && strstr(a->serr, "Moshier") != NULL;
}

static int same(const struct ans *a, const struct ans *b)
{
  return a->r == b->r && memcmp(a->x, b->x, sizeof a->x) == 0;
}

/* a directory of links to every file of ephe but semo_12.se1 */
static int build(char *dir, const char *ephe)
{
  DIR *d;
  struct dirent *e;
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  if (mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || strcmp(e->d_name, "semo_12.se1") == 0)
      continue;
    snprintf(src, sizeof src, "%s/%s", ephe, e->d_name);
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

int main(int argc, char **argv)
{
  char dir[] = "/tmp/g117-XXXXXX", ephe[PATH_MAX];
  double wplan = 0, wmoon = 0;
  int id, k, rep;

  if (argc != 2 || realpath(argv[1], ephe) == NULL) {
    fprintf(stderr, "usage: moonfile EPHEDIR\n");
    return 2;
  }
  if (build(dir, ephe) != 0) {
    printf("G117 FAIL: cannot build %s\n", dir);
    return 1;
  }
  printf("G117: the Swiss Moon file missing: refused when strict, the Moon Moshier's when not\n");
  for (id = 0; id < 4; id++) {
    double jd = rgJd[id];
    swe_ctx *w;
    /* (a), (c), (d) */
    for (k = 0; k < NDEP; k++) {
      struct ans a = ask_fresh(dir, 0, rgDep[k].kind, rgDep[k].ipl, rgDep[k].fl, jd);
      struct ans f = ask_fresh(dir, 1, rgDep[k].kind, rgDep[k].ipl, rgDep[k].fl, jd);
      struct ans full = ask_fresh(ephe, 0, rgDep[k].kind, rgDep[k].ipl, rgDep[k].fl, jd);
      check(refused(&a), "a", rgDep[k].sz, jd, &a, "answered, not refused");
      check(full.r >= 0 && (full.r & EPHMASK) == (rgDep[k].fl & EPHMASK),
            "d", rgDep[k].sz, jd, &full, "complete directory not Swiss");
      check(f.r >= 0, "c", rgDep[k].sz, jd, &f, "refused with the fallback allowed");
      if (f.r < 0)
        continue;
      if (rgDep[k].moon == 2) {
        check((f.r & EPHMASK) == SEFLG_MOSEPH, "c", rgDep[k].sz, jd, &f,
              "the Moon's flag not SEFLG_MOSEPH");
      } else if (rgDep[k].moon == 1) {
        struct ans m = ask_fresh(dir, 0, rgDep[k].kind, rgDep[k].ipl,
                                 (rgDep[k].fl & ~EPHMASK) | SEFLG_MOSEPH, jd);
        double d = fabs(swe_difdeg2n(f.x[0], m.x[0])) + fabs(f.x[1] - m.x[1]);
        if (d > wmoon)
          wmoon = d;
        check((f.r & EPHMASK) == SEFLG_MOSEPH, "c", rgDep[k].sz, jd, &f,
              "the Moon's flag not SEFLG_MOSEPH");
        check(d < 1e-9, "c", rgDep[k].sz, jd, &f, "not SEFLG_MOSEPH's Moon");
      } else if (rgDep[k].moon == 0 || rgDep[k].moon == 3) {
        double d = fabs(swe_difdeg2n(f.x[0], full.x[0])) + fabs(f.x[1] - full.x[1]);
        if (d > wplan)
          wplan = d;
        check((f.r & EPHMASK) == SEFLG_SWIEPH
              && (rgDep[k].moon == 3 || strstr(f.serr, "Moshier") != NULL),
              "c", rgDep[k].sz, jd, &f, "not Swiss, or Moshier's Moon not noted");
        check(d < 1e-6, "c", rgDep[k].sz, jd, &f, "more than 1e-6 deg off the files'");
      }
    }
    /* (b) fresh */
    for (k = 0; k < NIND; k++) {
      struct ans a = ask_fresh(dir, 0, rgInd[k].kind, rgInd[k].ipl, rgInd[k].fl, jd);
      struct ans full = ask_fresh(ephe, 0, rgInd[k].kind, rgInd[k].ipl, rgInd[k].fl, jd);
      check(same(&a, &full), "b", rgInd[k].sz, jd, &a, "not the complete directory's");
    }
    /* (a), (b) warm: Mars heliocentric first, so that the save area holds
     * the Earth made from Moshier's Moon */
    w = open_ctx(dir, 0);
    for (k = 0; k < NIND; k++) {
      struct ans a = ask(w, rgInd[k].kind, rgInd[k].ipl, rgInd[k].fl, jd);
      struct ans full = ask_fresh(ephe, 0, rgInd[k].kind, rgInd[k].ipl, rgInd[k].fl, jd);
      check(same(&a, &full), "b warm", rgInd[k].sz, jd, &a, "not the complete directory's");
    }
    for (k = 0; k < NDEP; k++)
      for (rep = 0; rep < 2; rep++) {
        struct ans a = ask(w, rgDep[k].kind, rgDep[k].ipl, rgDep[k].fl, jd);
        check(refused(&a), "a warm", rgDep[k].sz, jd, &a, "answered, not refused");
      }
    swe_ctx_free(w);
    /* (e) bug 194: SEFLG_TOPOCTR after SEFLG_TOPOCTR with another centre,
     * one context, as SEFLG_TOPOCTR in a fresh one, both settings */
    for (k = 0; k < 2 * NCOMB; k++) {
      int fb = k / NCOMB;
      int32 fl = SEFLG_SWIEPH | SEFLG_TOPOCTR | SEFLG_SPEED;
      struct ans a, t, ref;
      w = open_ctx(dir, fb);
      a = ask(w, CALC, rgComb[k % NCOMB].ipl, fl | rgComb[k % NCOMB].ctr, jd);
      t = ask(w, CALC, rgComb[k % NCOMB].ipl, fl, jd);
      swe_ctx_free(w);
      ref = ask_fresh(dir, fb, CALC, rgComb[k % NCOMB].ipl, fl, jd);
      check(same(&t, &ref) && (fb ? t.r >= 0 : refused(&t)) && (fb ? a.r >= 0 : refused(&a)),
            fb ? "e fallback" : "e", rgComb[k % NCOMB].sz, jd, &t,
            "SEFLG_TOPOCTR after it not as in a fresh context");
    }
  }
  unbuild(dir);
  printf("  rows %d, failed %d; fallback: the Moon %.2e deg off SEFLG_MOSEPH's, the others %.2e deg off the files'\n",
         nrow, nbad, wmoon, wplan);
  printf(nbad ? "G117 FAIL: %d\n" : "G117 PASS\n", nbad);
  return nbad ? 1 : 0;
}
