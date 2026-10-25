/* horfall.c -- G157: a JPL request with SEFLG_JPLHOR_APPROX or SEFLG_JPLHOR
 * that falls to the Swiss files or to Moshier answers as the request of
 * the ephemeris that answered: return flag, place and speeds, bit for bit.
 *
 * The defect (UPSTREAM-BUGS 207): plaus_iflag() keeps the Horizons bits
 * for a JPL request alone (SEFLG_JPLHOR_APPROX, and SEFLG_JPLHOR, which
 * becomes it without the IERS corrections; SEFLG_ICRS added for SEFLG_JPLHOR
 * and for SEMOD_JPLHORA_2's SEFLG_JPLHOR_APPROX), and a request that fell
 * to another ephemeris went on with them: its place the Horizons frame on
 * that ephemeris's, its return a flag no request returns. With the
 * fallback allowed, Mars at 1500 on de200.eph (1600-2170) 524548
 * (SEFLG_MOSEPH | SEFLG_JPLHOR_APPROX | SEFLG_SPEED), 3.5e-4 deg off the
 * Moshier request, with the JPL file missing 524546 and as far off the
 * Swiss one; Aldebaran by the four star functions so (since UPSTREAM-BUGS
 * 198 falls to Moshier there), 1.4e-5 deg at 2000; swe_calc_pctr() of Mars
 * from Jupiter 524546 and 3.5e-4 deg off the Swiss request; and by
 * swe_calc_ut() and the star functions' _ut forms under SEMOD_JPLHORA_2,
 * which add SEFLG_ICRS before swe_calc(), 131332 and 1.9e-6 deg off.
 *
 * The check, every call in a fresh context unless said, five directories
 * in /tmp of links to EPHEDIR's files, the JPL file named: F, all of them
 * with de200.eph, at 1500 Jan 5 and 2406 (outside de200's span: Moshier
 * answers); S, without the JPL file, at 1500 and 2000 Jul 1.5 (Swiss); N,
 * sefstars.txt and seorbel.txt alone (Moshier); M, without the JPL file
 * and the Moon files (Swiss, the Moon from Moshier, flagged so by entry
 * 120); E, the Swiss files, G19's synthetic DE 431 header (2000 Jan 1-Feb
 * 2) and an eop_1962_today.txt, so that SEFLG_JPLHOR stays itself and takes
 * SEFLG_ICRS, at 1500 and 2406 (Moshier). 26 calls: swe_calc() of Mars
 * (with and without SEFLG_SPEED, topocentric, equatorial, SEFLG_SPEED3,
 * SEFLG_TRUEPOS, J2000, heliocentric, barycentric), the Sun, the Moon,
 * Ceres, Cupido; swe_calc_ut() of Mars and the Moon; Aldebaran by
 * swe_fixstar2() (with SEFLG_SPEED, topocentric, equatorial without it),
 * swe_fixstar(), and their _ut forms; swe_nod_aps() of Mars and
 * swe_nod_aps_ut() of the Moon (osculating; the mean points read no
 * file, and do not fall); swe_calc_pctr() of Mars from
 * Jupiter and the Moon from Mars; swe_calc_orbel() of an orbit about the
 * Sun. Each with SEFLG_JPLHOR_APPROX and SEFLG_JPLHOR:
 *  (a) the fallback allowed, under SEMOD_JPLHORA_3 (the default), 1 and
 *      2: the JPL request's return and answer (all six numbers; the four
 *      points of swe_nod_aps()) bit for bit those of the same request of
 *      the ephemeris that answered (refused alike where that one is
 *      refused: a barycentric Moshier place, an asteroid without its
 *      file), and no Horizons bit in its return;
 *  (b) strict: refused, -1, with the message of the same JPL request
 *      without the Horizons bit (the downgrade's);
 *  (c) one context, the fallback allowed: the JPL request, that
 *      ephemeris's request, the JPL request again, each bit for bit a
 *      fresh context's; in F and S a JPL request at 2000 Jul 1.5, within
 *      de200's span, before and after them, its answer the fresh one's and
 *      its return keeping SEFLG_JPLEPH and the Horizons bit.
 * Measured: 3848 checks, none fail.
 * The build before the fix fails 1200, every (a) row where the JPL request
 * answered and fell (228 in F, 264 in S, 216 in N, 264 in M, 228 in E):
 * the Horizons bit kept in the return (524548 for Moshier's 260, 524546
 * for Swiss 258; with SEFLG_ICRS too under SEMOD_JPLHORA_2, 655620), and
 * the place up to 5.1e-4 deg off (Mars equatorial; Mars, the Sun, the
 * Moon, Ceres and the stars 3.9e-4 to 4.9e-4, swe_calc_pctr() 3.6e-4,
 * Mars in J2000 2.4e-5). swe_nod_aps(), swe_calc_orbel() and Cupido drop
 * the bits themselves and pass; (b) and (c) pass (the fresh request as
 * wrong as the warm one). The upstream build (SWE_UPSTREAM_COMPAT) fails
 * 1766: 1350 of (a), with its own defects that earlier entries fixed (the
 * stars refused outside de200's span, entry 198; a fallen barycentric
 * request answered, entry 139), and 416 of (c), its caches'.
 * Run as
 *   horfall EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "swephexp.h"

#define EPHMASK (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)
#define HORMASK (SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX)
#define JPL_FIXTURE "de431fixture.eph"

enum { CALC, CALC_UT, STAR2, STAR, STAR2_UT, STAR_UT, NODAPS, NODAPS_UT, PCTR, ORBEL };
static const struct { const char *sz; int kind, ipl, ctr; int32 fl; } rgCall[] = {
  {"Mars", CALC, SE_MARS, 0, SEFLG_SPEED},
  {"Mars, no speed", CALC, SE_MARS, 0, 0},
  {"Mars, topocentric", CALC, SE_MARS, 0, SEFLG_TOPOCTR | SEFLG_SPEED},
  {"Mars, equatorial", CALC, SE_MARS, 0, SEFLG_EQUATORIAL | SEFLG_SPEED},
  {"Mars, SEFLG_SPEED3", CALC, SE_MARS, 0, SEFLG_SPEED3},
  {"Mars, SEFLG_TRUEPOS", CALC, SE_MARS, 0, SEFLG_TRUEPOS | SEFLG_SPEED},
  {"Mars, J2000", CALC, SE_MARS, 0, SEFLG_J2000 | SEFLG_SPEED},
  {"Mars, heliocentric", CALC, SE_MARS, 0, SEFLG_HELCTR | SEFLG_SPEED},
  {"Mars, barycentric", CALC, SE_MARS, 0, SEFLG_BARYCTR | SEFLG_SPEED},
  {"the Sun", CALC, SE_SUN, 0, SEFLG_SPEED},
  {"the Moon", CALC, SE_MOON, 0, SEFLG_SPEED},
  {"Ceres", CALC, SE_CERES, 0, SEFLG_SPEED},
  {"Cupido", CALC, SE_CUPIDO, 0, SEFLG_SPEED},
  {"Mars, swe_calc_ut()", CALC_UT, SE_MARS, 0, SEFLG_SPEED},
  {"the Moon, swe_calc_ut()", CALC_UT, SE_MOON, 0, SEFLG_SPEED},
  {"Aldebaran, swe_fixstar2()", STAR2, 0, 0, SEFLG_SPEED},
  {"Aldebaran, swe_fixstar2(), topocentric", STAR2, 0, 0, SEFLG_TOPOCTR | SEFLG_SPEED},
  {"Aldebaran, swe_fixstar2(), equatorial", STAR2, 0, 0, SEFLG_EQUATORIAL},
  {"Aldebaran, swe_fixstar()", STAR, 0, 0, SEFLG_SPEED},
  {"Aldebaran, swe_fixstar2_ut()", STAR2_UT, 0, 0, SEFLG_SPEED},
  {"Aldebaran, swe_fixstar_ut()", STAR_UT, 0, 0, SEFLG_SPEED},
  {"Mars, swe_nod_aps()", NODAPS, SE_MARS, SE_NODBIT_OSCU, SEFLG_SPEED},
  {"the Moon, swe_nod_aps_ut()", NODAPS_UT, SE_MOON, SE_NODBIT_OSCU, SEFLG_SPEED},
  {"Mars from Jupiter, swe_calc_pctr()", PCTR, SE_MARS, SE_JUPITER, SEFLG_SPEED},
  {"the Moon from Mars, swe_calc_pctr()", PCTR, SE_MOON, SE_MARS, SEFLG_SPEED},
  {"an orbit, swe_calc_orbel()", ORBEL, 0, 0, SEFLG_SPEED},
};
#define NCALL ((int) (sizeof rgCall / sizeof rgCall[0]))

/* the directories: name, files left out ("de": de*.eph; "semo"; "*": all
 * but the two text files), the JPL file, the instants, the ephemeris that
 * answers, a JPL instant within the span (0: none) */
enum { D_F, D_S, D_N, D_M, D_E, ND };
static const struct { const char *sz, *out1, *out2, *jpl; double jd[2]; int32 eph; double tin; } rgDir[ND] = {
  {"F (de200.eph outside its span)", "", "", "de200.eph", {2268923.5, 2600000.5}, SEFLG_MOSEPH, 2451727.0},
  {"S (no JPL file)", "de", "", "de200.eph", {2268923.5, 2451727.0}, SEFLG_SWIEPH, 0},
  {"N (no ephemeris file)", "*", "", "de200.eph", {2268923.5, 2451727.0}, SEFLG_MOSEPH, 0},
  {"M (no JPL file, no Moon file)", "de", "semo", "de200.eph", {2268923.5, 2451727.0}, SEFLG_SWIEPH, 0},
  {"E (DE 431 header, the corrections)", "de", "", JPL_FIXTURE, {2268923.5, 2600000.5}, SEFLG_MOSEPH, 0},
};
static char rgPath[ND][PATH_MAX];

static const char *rgMod[3] = {"0,0,0,0,0,0,0,0", "0,0,0,0,0,0,1,0", "0,0,0,0,0,0,2,0"};
static int nbad, nrow;

struct ans { int32 r; double x[24]; char serr[AS_MAXCH]; };

static swe_ctx *open_ctx(int d, int mod, int fallback)
{
  char m[40], jpl[64];
  swe_ctx *c = swe_ctx_new();
  strcpy(m, rgMod[mod]);
  strcpy(jpl, rgDir[d].jpl);
  swe_set_ephe_path_r(c, rgPath[d]);
  swe_set_jpl_file_r(c, jpl);
  swe_set_astro_models_r(c, m, 0);
  swe_set_ephe_fallback_r(c, fallback);
  swe_set_topo_r(c, 8.55, 47.37, 400);
  return c;
}

static struct ans ask(swe_ctx *c, int k, int32 fl, double jd)
{
  static const double els[6] = {1.2, 0.0934, 0.0323, 49.56, 286.5, 19.4};
  struct ans a;
  char star[64] = "Aldebaran";
  int ipl = rgCall[k].ipl, ctr = rgCall[k].ctr;
  memset(&a, 0, sizeof a);
  fl |= rgCall[k].fl;
  switch (rgCall[k].kind) {
    case CALC: a.r = swe_calc_r(c, jd, ipl, fl, a.x, a.serr); break;
    case CALC_UT: a.r = swe_calc_ut_r(c, jd, ipl, fl, a.x, a.serr); break;
    case STAR2: a.r = swe_fixstar2_r(c, star, jd, fl, a.x, a.serr); break;
    case STAR: a.r = swe_fixstar_r(c, star, jd, fl, a.x, a.serr); break;
    case STAR2_UT: a.r = swe_fixstar2_ut_r(c, star, jd, fl, a.x, a.serr); break;
    case STAR_UT: a.r = swe_fixstar_ut_r(c, star, jd, fl, a.x, a.serr); break;
    case NODAPS: a.r = swe_nod_aps_r(c, jd, ipl, fl, ctr, a.x, a.x + 6, a.x + 12, a.x + 18, a.serr); break;
    case NODAPS_UT: a.r = swe_nod_aps_ut_r(c, jd, ipl, fl, ctr, a.x, a.x + 6, a.x + 12, a.x + 18, a.serr); break;
    case PCTR: a.r = swe_calc_pctr_r(c, jd, ipl, ctr, fl, a.x, a.serr); break;
    default: a.r = swe_calc_orbel_r(c, jd, els, SE_SUN, SE_ORBEL_EQ_J2000, 0, fl, a.x, a.serr); break;
  }
  return a;
}

static struct ans ask_fresh(int d, int mod, int fallback, int k, int32 fl, double jd)
{
  swe_ctx *c = open_ctx(d, mod, fallback);
  struct ans a = ask(c, k, fl, jd);
  swe_ctx_free(c);
  return a;
}

/* return and every number bit for bit */
static int same(const struct ans *a, const struct ans *b)
{
  return a->r == b->r && memcmp(a->x, b->x, sizeof a->x) == 0;
}

static double off(const struct ans *a, const struct ans *b)
{
  double w = 0;
  int i;
  for (i = 0; i < 24; i += 6) {
    double d = fabs(swe_difdeg2n(a->x[i], b->x[i])) + fabs(a->x[i + 1] - b->x[i + 1]);
    if (d > w)
      w = d;
  }
  return w;
}

static void check(int ok, const char *part, int d, int k, int32 hor, int mod, double jd,
                  const struct ans *a, const struct ans *ref)
{
  nrow++;
  if (ok)
    return;
  nbad++;
  if (nbad <= 40)
    printf("    FAIL (%s) %c %-38s %s mode %d jd %.1f: ret %d for %d, %.2e deg; serr '%.60s'\n",
           part, rgDir[d].sz[0], rgCall[k].sz, hor == SEFLG_JPLHOR ? "JPLHOR" : "JPLHOR_APPROX",
           mod == 0 ? 3 : mod, jd, a->r, ref->r, off(a, ref), a->serr);
}

/* G19's synthetic JPL header (eopload.c, eopfollow.c), DE 431, 2000 Jan
 * 1.0 to Feb 2.0: fsizer() accepts it, and outside its span the read is
 * BEYOND_EPH_LIMITS */
static void put32(unsigned char *p, int v)    { memcpy(p, &v, 4); }
static void putd (unsigned char *p, double v) { memcpy(p, &v, 8); }
static int write_jpl(const char *path)
{
  unsigned char h[2856];
  const int ncf = 4, na = 1, maxoff = 1200, nrecl = 4;
  const double ss0 = 2451544.5, ss1 = 2451576.5, ss2 = 32.0;
  int i, ksize, nseg = (int) ((ss1 - ss0) / ss2);
  long nb = 0;
  FILE *fp;
  memset(h, ' ', 252);
  memcpy(h, "SWISSEPH EOP LOADER FIXTURE", 27);
  memset(h + 252, ' ', 2400);
  putd (h + 2652, ss0);
  putd (h + 2660, ss1);
  putd (h + 2668, ss2);
  put32(h + 2676, 0);
  putd (h + 2680, 149597870.7);
  putd (h + 2688, 81.3);
  for (i = 0; i < 12; i++) {
    put32(h + 2696 + (i * 3 + 0) * 4, 3);
    put32(h + 2696 + (i * 3 + 1) * 4, ncf);
    put32(h + 2696 + (i * 3 + 2) * 4, na);
  }
  put32(h + 2696 + 30 * 4, maxoff);
  put32(h + 2696 + 31 * 4, ncf);
  put32(h + 2696 + 32 * 4, na);
  put32(h + 2840, 431);
  put32(h + 2844, 3);
  put32(h + 2848, ncf);
  put32(h + 2852, na);
  ksize = (maxoff + 3 * ncf * na - 1) * 2;
  for (i = 0; i < 13; i++)
    nb += (long) ncf * na * (i == 11 ? 2 : 3) * nseg;
  nb += 2 * nseg;
  nb *= 8;
  nb += 2 * (long) ksize * nrecl;
  if ((fp = fopen(path, "wb")) == NULL)
    return -1;
  fwrite(h, 1, sizeof h, fp);
  for (i = (int) sizeof h; i < nb; i++)
    fputc(0, fp);
  if ((fp = freopen(path, "r+b", fp)) == NULL)
    return -1;
  fseek(fp, 2L * ksize * nrecl, SEEK_SET);
  { double ep[2]; ep[0] = ss0; ep[1] = ss1; fwrite(ep, 8, 2, fp); }
  fclose(fp);
  return 0;
}

/* eop_1962_today.txt (EOPC04 columns) for MJD 51544..51583 */
static int write_eop(const char *path)
{
  FILE *fp = fopen(path, "w");
  int i;
  if (fp == NULL)
    return -1;
  fprintf(fp, "# EOPC04 fixture\n");
  for (i = 0; i < 40; i++)
    fprintf(fp, "2000  1 %2d  %5d   0.0 0.0 0.0 0.0 %.9f %.9f\n",
            i + 1, 51544 + i, -0.05 - i * 1e-5, -0.005 - i * 1e-6);
  return fclose(fp);
}

static int left_out(int d, const char *f)
{
  if (strcmp(rgDir[d].out1, "*") == 0)
    return strcmp(f, "sefstars.txt") != 0 && strcmp(f, "seorbel.txt") != 0;
  return (*rgDir[d].out1 && strncmp(f, rgDir[d].out1, strlen(rgDir[d].out1)) == 0
          && (strcmp(rgDir[d].out1, "de") != 0 || strstr(f, ".eph") != NULL))
      || (*rgDir[d].out2 && strncmp(f, rgDir[d].out2, strlen(rgDir[d].out2)) == 0);
}

static int build(const char *ephe)
{
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  struct dirent *e;
  DIR *dp;
  int d;
  for (d = 0; d < ND; d++) {
    strcpy(rgPath[d], "/tmp/g157-XXXXXX");
    if (mkdtemp(rgPath[d]) == NULL || (dp = opendir(ephe)) == NULL)
      return -1;
    while ((e = readdir(dp)) != NULL) {
      if (e->d_name[0] == '.' || left_out(d, e->d_name))
        continue;
      snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
      snprintf(q, sizeof q, "%.*s/%s", PATH_MAX - 1, rgPath[d], e->d_name);
      if (symlink(p, q) != 0) {
        closedir(dp);
        return -1;
      }
    }
    closedir(dp);
  }
  snprintf(p, sizeof p, "%s/%s", rgPath[D_E], JPL_FIXTURE);
  snprintf(q, sizeof q, "%s/eop_1962_today.txt", rgPath[D_E]);
  return write_jpl(p) == 0 && write_eop(q) == 0 ? 0 : -1;
}

static void unbuild(void)
{
  char p[PATH_MAX + 300];
  struct dirent *e;
  DIR *dp;
  int d;
  for (d = 0; d < ND; d++) {
    if ((dp = opendir(rgPath[d])) == NULL)
      continue;
    while ((e = readdir(dp)) != NULL)
      if (e->d_name[0] != '.') {
        snprintf(p, sizeof p, "%.*s/%s", PATH_MAX - 1, rgPath[d], e->d_name);
        unlink(p);
      }
    closedir(dp);
    rmdir(rgPath[d]);
  }
}

int main(int argc, char **argv)
{
  static const int32 rgHor[2] = {SEFLG_JPLHOR_APPROX, SEFLG_JPLHOR};
  char ephe[PATH_MAX];
  double worst = 0;
  int d, t, k, h, mod;

  if (argc != 2 || realpath(argv[1], ephe) == NULL) {
    fprintf(stderr, "usage: horfall EPHEDIR\n");
    return 2;
  }
  if (build(ephe) != 0) {
    printf("G157 FAIL: cannot build the directories\n");
    unbuild();
    return 1;
  }
  printf("G157: a fallen JPL request with a Horizons bit answers as the request of the ephemeris that answered\n");
  for (d = 0; d < ND; d++)
    for (t = 0; t < 2; t++) {
      double jd = rgDir[d].jd[t];
      int32 eph = rgDir[d].eph;
      for (k = 0; k < NCALL; k++)
        for (h = 0; h < 2; h++) {
          int32 hor = rgHor[h];
          struct ans ref0 = {0, {0}, ""};
          /* (a) */
          for (mod = 0; mod < 3; mod++) {
            struct ans a = ask_fresh(d, mod, 1, k, SEFLG_JPLEPH | hor, jd);
            struct ans ref = ask_fresh(d, mod, 1, k, eph | hor, jd);
            int ok = same(&a, &ref) && (a.r < 0 || !(a.r & HORMASK));
            if (a.r >= 0 && ref.r >= 0 && off(&a, &ref) > worst)
              worst = off(&a, &ref);
            check(ok, "a", d, k, hor, mod, jd, &a, &ref);
            if (mod == 0)
              ref0 = ref;
          }
          /* (b) */
          {
            struct ans a = ask_fresh(d, 0, 0, k, SEFLG_JPLEPH | hor, jd);
            struct ans ref = ask_fresh(d, 0, 0, k, SEFLG_JPLEPH, jd);
            check(a.r < 0 && ref.r < 0 && strcmp(a.serr, ref.serr) == 0, "b", d, k, hor, 0, jd, &a, &ref);
          }
          /* (c) */
          {
            swe_ctx *w = open_ctx(d, 0, 1);
            struct ans in0 = {0, {0}, ""}, in1, inref, a1, r1, a2;
            struct ans afresh = ask_fresh(d, 0, 1, k, SEFLG_JPLEPH | hor, jd);
            if (rgDir[d].tin != 0)
              in0 = ask(w, k, SEFLG_JPLEPH | hor, rgDir[d].tin);
            a1 = ask(w, k, SEFLG_JPLEPH | hor, jd);
            r1 = ask(w, k, eph | hor, jd);
            a2 = ask(w, k, SEFLG_JPLEPH | hor, jd);
            check(same(&a1, &afresh), "c, the JPL request", d, k, hor, 0, jd, &a1, &afresh);
            check(same(&r1, &ref0), "c, that ephemeris's after it", d, k, hor, 0, jd, &r1, &ref0);
            check(same(&a2, &afresh), "c, the JPL request again", d, k, hor, 0, jd, &a2, &afresh);
            if (rgDir[d].tin != 0) {
              in1 = ask(w, k, SEFLG_JPLEPH | hor, rgDir[d].tin);
              inref = ask_fresh(d, 0, 1, k, SEFLG_JPLEPH | hor, rgDir[d].tin);
              /* swe_nod_aps() returns OK, not a flag; swe_calc_orbel() and
               * the fictitious bodies drop the Horizons bits themselves */
              int kept = rgCall[k].kind == NODAPS || rgCall[k].kind == NODAPS_UT
                         || rgCall[k].kind == ORBEL || rgCall[k].ipl == SE_CUPIDO
                         || ((inref.r & SEFLG_JPLEPH) && (inref.r & HORMASK));
              check(same(&in0, &inref) && kept, "c, within the span, before", d, k, hor, 0,
                    rgDir[d].tin, &in0, &inref);
              check(same(&in1, &inref), "c, within the span, after", d, k, hor, 0,
                    rgDir[d].tin, &in1, &inref);
            }
            swe_ctx_free(w);
          }
        }
    }
  unbuild();
  printf("  rows %d, failed %d; the fallen answers %.2e deg off the request of the ephemeris that answered\n",
         nrow, nbad, worst);
  printf(nbad ? "G157 FAIL: %d\n" : "G157 PASS\n", nbad);
  return nbad ? 1 : 0;
}
