/* missfile.c -- G38: a Swiss file missing whose neighbour does not hold the
 * instant falls back as a missing file does: never a silent NaN or another
 * instant's place, and a warm context answers as a fresh one.
 *
 * sweph() asks the neighbouring 600-year bucket's file when the file named
 * for the instant is missing, because that file's data runs a little into
 * the bucket. With semo_12.se1 missing, at JD 2159377.5 (1200.0 + 20 days)
 * it opened semo_06.se1, whose data ends 16 days past 1200.0, refused the
 * instant, and left the file open. sweplan() takes "the moon file is not
 * open" as its sign to use the Moshier Moon, so it went on with a Moon it
 * never read: a fresh context returned the Moon as NaN with flags 0x100 and
 * no error, a warm one the Moon of its last read (26.44179 deg for
 * 349.29034), and the Sun, whose Earth is the EMB less that Moon, 6.03
 * arcsec apart fresh and warm. Planets and the main asteroids already fell
 * back: the caller looks at the return value there.
 *
 * The checks, in directories built in /tmp of symbolic links to EPHEDIR's
 * files: (a) without semo_12, (b) without semo_18 -- so the neighbour asked
 * is the earlier bucket's or the later's, and holds the instant or not. At
 * 1200.0 + 5, 20, 50, 200, 400 and 50000 days, 1800.0 - 20, - 5, + 5, 20,
 * 100 and 50000, 600.0 + 5, 400, 50000 and - 50000, the Sun, Moon, Mercury,
 * Mars, Jupiter, Pluto, Chiron, Ceres and Juno, geocentric, heliocentric,
 * barycentric and J2000 equatorial XYZ true positions, all with SEFLG_SPEED,
 * the substitution of Moshier for a missing file allowed:
 *   1. every answer is an error or six finite numbers;
 *   2. contexts warmed by the same call 1.5 and 30 days either side, or by
 *      Mercury, the Moon and Ceres 25 days before, answer as a fresh one:
 *      the same return flags, and a place within 1e-6 AU (the Sun's shift
 *      from a Moon read for another instant is 2e-5 AU; two neighbouring
 *      files of the same body differ by up to 3e-7 AU in the overrun they
 *      share, which is not this matter);
 *   3. an answer is within 60 arcsec (XYZ: 3e-4 of the distance) of the
 *      complete directory's, and one more than 0.5 arcsec from it (a
 *      fallback, not a neighbour's file) says "Moshier" in serr;
 * and, with a planet file's neighbour not holding the instant either,
 * (c) sepl_12 missing: the Sun at 1200.0 + 100 days says Moshier and is
 * within 60 arcsec of the complete directory's, and (d) both semo_12 and
 * semo_06, or both sepl_12 and sepl_06 missing: the Moon, or the Sun, at
 * 1200.0 + 5 says in serr that semo_12, or sepl_12, is not found, the file
 * named for the instant, not the neighbour last tried.
 * Run as
 *   missfile EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

static int nrow, nbad[3];

struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; };

static swe_ctx *open_ctx(const char *dir)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, dir);
  swe_set_ephe_fallback_r(c, 1);
  return c;
}

static struct ans calc(swe_ctx *c, double jd, int ipl, int32 fl)
{
  struct ans a;
  memset(&a, 0, sizeof a);
  a.r = swe_calc_r(c, jd, ipl, fl, a.x, a.serr);
  if (a.r < 0)
    memset(a.x, 0, sizeof a.x);
  return a;
}

static struct ans fresh_calc(const char *dir, double jd, int ipl, int32 fl)
{
  swe_ctx *c = open_ctx(dir);
  struct ans a = calc(c, jd, ipl, fl);
  swe_ctx_free(c);
  return a;
}

/* the distance in AU between two answers' places */
static double dist_au(const struct ans *a, const struct ans *b, int32 fl)
{
  double c[2][3];
  const struct ans *p[2];
  int k;
  p[0] = a;
  p[1] = b;
  for (k = 0; k < 2; k++) {
    const double *x = p[k]->x;
    if (fl & SEFLG_XYZ) {
      memcpy(c[k], x, sizeof c[k]);
    } else {
      double lon = x[0] * M_PI / 180, lat = x[1] * M_PI / 180;
      c[k][0] = x[2] * cos(lat) * cos(lon);
      c[k][1] = x[2] * cos(lat) * sin(lon);
      c[k][2] = x[2] * sin(lat);
    }
  }
  return sqrt(pow(c[0][0] - c[1][0], 2) + pow(c[0][1] - c[1][1], 2) + pow(c[0][2] - c[1][2], 2));
}

static void bad(int k, const char *what, double jd, int ipl, int32 fl, const struct ans *a,
                const struct ans *b)
{
  if (nbad[k] < 6)
    printf("  %s: body %d, flags %#x, JD %.4f: %.9f / %.9f (flags %#x / %#x)\n", what, ipl,
           (unsigned) fl, jd, a->x[0], b ? b->x[0] : 0.0, (unsigned) a->r,
           b ? (unsigned) b->r : 0u);
  nbad[k]++;
}

/* a directory of links to every file of ephe but the nname named */
static int build(char *dir, const char *ephe, const char *const *name, int nname)
{
  DIR *d;
  struct dirent *e;
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  int k;
  if (mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    int skip = e->d_name[0] == '.';
    for (k = 0; k < nname; k++)
      skip |= strcmp(e->d_name, name[k]) == 0;
    if (skip)
      continue;
    snprintf(src, sizeof src, "%s/%s", ephe, e->d_name);
    snprintf(dst, sizeof dst, "%s/%s", dir, e->d_name);
    if (symlink(src, dst) != 0)
      return -1;
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

static const int rgBody[] = {
  SE_SUN, SE_MOON, SE_MERCURY, SE_MARS, SE_JUPITER, SE_PLUTO, SE_CHIRON, SE_CERES, SE_JUNO
};
static const int32 rgFl[] = {
  SEFLG_SPEED, SEFLG_SPEED | SEFLG_HELCTR, SEFLG_SPEED | SEFLG_BARYCTR,
  SEFLG_SPEED | SEFLG_TRUEPOS | SEFLG_J2000 | SEFLG_NONUT | SEFLG_EQUATORIAL | SEFLG_XYZ
};
#define NBODY ((int) (sizeof rgBody / sizeof rgBody[0]))
#define NFL ((int) (sizeof rgFl / sizeof rgFl[0]))

static void run(const char *dir, const char *full, const double *rgJd, int njd)
{
  static const double rgW[4] = {-1.5, 1.5, -30, 30};
  int id, ib, ifl, k, i;
  for (id = 0; id < njd; id++)
    for (ib = 0; ib < NBODY; ib++)
      for (ifl = 0; ifl < NFL; ifl++) {
        double jd = rgJd[id];
        int ipl = rgBody[ib];
        int32 fl = rgFl[ifl];
        struct ans a = fresh_calc(dir, jd, ipl, fl), b, f;
        nrow++;
        /* 1 */
        for (i = 0; i < 6; i++)
          if (a.r >= 0 && !isfinite(a.x[i])) {
            bad(0, "not finite", jd, ipl, fl, &a, NULL);
            break;
          }
        /* 2 */
        for (k = 0; k < 5; k++) {
          swe_ctx *c = open_ctx(dir);
          if (k < 4) {
            calc(c, jd + rgW[k], ipl, fl);
          } else {
            calc(c, jd - 25, SE_MERCURY, SEFLG_SPEED);
            calc(c, jd - 25, SE_MOON, SEFLG_SPEED);
            calc(c, jd - 25, SE_CERES, SEFLG_SPEED);
          }
          b = calc(c, jd, ipl, fl);
          swe_ctx_free(c);
          {
            if (a.r != b.r || !(dist_au(&a, &b, fl) < 1e-6)) {
              bad(1, "warm differs from fresh", jd, ipl, fl, &a, &b);
              break;
            }
          }
        }
        /* 3 */
        if (a.r < 0)
          continue;
        f = fresh_calc(full, jd, ipl, fl);
        if (f.r < 0)
          continue;
        {
          double d;
          if (fl & SEFLG_XYZ) {
            double r = sqrt(f.x[0] * f.x[0] + f.x[1] * f.x[1] + f.x[2] * f.x[2]);
            d = sqrt(pow(a.x[0] - f.x[0], 2) + pow(a.x[1] - f.x[1], 2) + pow(a.x[2] - f.x[2], 2))
                / r * 206265;
          } else {
            d = hypot(swe_difdeg2n(a.x[0], f.x[0]), a.x[1] - f.x[1]) * 3600;
          }
          if (!(d < 60))
            bad(2, "far from the complete directory's", jd, ipl, fl, &a, &f);
          else if (d > 0.5 && strstr(a.serr, "Moshier") == NULL)
            bad(2, "fell back without saying so", jd, ipl, fl, &a, &f);
        }
      }
}

int main(int argc, char **argv)
{
  static const char *const rgName[1] = {"semo_12.se1"};
  static const char *const rgName2[1] = {"semo_18.se1"};
  char ephe[PATH_MAX], dir[2][32] = {"/tmp/missfileXXXXXX", "/tmp/missfilXXXXXX"};
  double J600, J1200, J1800, rgJd[16];
  int k, n = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: missfile EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], ephe) == NULL) {
    perror(argv[1]);
    return 2;
  }
  J600 = swe_julday(600, 1, 1, 0, SE_JUL_CAL);
  J1200 = swe_julday(1200, 1, 1, 0, SE_JUL_CAL);
  J1800 = swe_julday(1800, 1, 1, 0, SE_GREG_CAL);
  rgJd[n++] = J1200 + 5;  rgJd[n++] = J1200 + 20; rgJd[n++] = J1200 + 50;
  rgJd[n++] = J1200 + 200; rgJd[n++] = J1200 + 400; rgJd[n++] = J1200 + 50000;
  rgJd[n++] = J1800 - 20; rgJd[n++] = J1800 - 5;  rgJd[n++] = J1800 + 5;
  rgJd[n++] = J1800 + 20; rgJd[n++] = J1800 + 100; rgJd[n++] = J1800 + 50000;
  rgJd[n++] = J600 + 5;   rgJd[n++] = J600 + 400; rgJd[n++] = J600 + 50000;
  rgJd[n++] = J600 - 50000;
  printf("G38: a missing Swiss file whose neighbour does not hold the instant\n");
  for (k = 0; k < 2; k++) {
    int nb0 = nbad[0] + nbad[1] + nbad[2];
    if (build(dir[k], ephe, k ? rgName2 : rgName, 1) != 0) {
      perror(dir[k]);
      unbuild(dir[k]);
      return 2;
    }
    run(dir[k], ephe, rgJd, n);
    unbuild(dir[k]);
    printf("  %s: %d fail\n", k ? "(b) semo_18 missing" : "(a) semo_12 missing",
           nbad[0] + nbad[1] + nbad[2] - nb0);
  }
  /* (c): sepl_12 missing, its neighbour not holding the instant */
  {
    char dc[] = "/tmp/missfilcXXXXXX";
    static const char *const rgOne[1] = {"sepl_12.se1"};
    struct ans a, f;
    if (build(dc, ephe, rgOne, 1) != 0) {
      perror(dc);
      unbuild(dc);
      return 2;
    }
    a = fresh_calc(dc, J1200 + 100, SE_SUN, SEFLG_SPEED);
    unbuild(dc);
    f = fresh_calc(ephe, J1200 + 100, SE_SUN, SEFLG_SPEED);
    if (a.r < 0 || strstr(a.serr, "Moshier") == NULL
        || hypot(swe_difdeg2n(a.x[0], f.x[0]), a.x[1] - f.x[1]) * 3600 > 60) {
      printf("  (c) sepl_12 missing: %d, %.9f for %.9f, serr '%s'\n", a.r, a.x[0], f.x[0], a.serr);
      nbad[2]++;
    } else
      printf("  (c) sepl_12 missing: the Sun from Moshier, 0 fail\n");
  }
  /* (d): the instant's file and its neighbour both missing -- serr names
   * the instant's own file, not the neighbour last tried */
  for (k = 0; k < 2; k++) {
    char dd[] = "/tmp/missfildXXXXXX";
    static const char *const rgTwo[2][2] = {{"semo_12.se1", "semo_06.se1"},
                                            {"sepl_12.se1", "sepl_06.se1"}};
    static const char *const rgWant[2] = {"semo_12.se1", "sepl_12.se1"};
    static const char *const rgNot[2] = {"semo_06.se1", "sepl_06.se1"};
    struct ans a;
    if (build(dd, ephe, rgTwo[k], 2) != 0) {
      perror(dd);
      unbuild(dd);
      return 2;
    }
    a = fresh_calc(dd, J1200 + 5, k ? SE_SUN : SE_MOON, SEFLG_SPEED);
    unbuild(dd);
    if (strstr(a.serr, rgWant[k]) == NULL || strstr(a.serr, rgNot[k]) != NULL) {
      printf("  (d) %s and %s missing: serr '%s'\n", rgWant[k], rgNot[k], a.serr);
      nbad[2]++;
    } else
      printf("  (d) %s and %s missing: names %s, 0 fail\n", rgWant[k], rgNot[k], rgWant[k]);
  }
  printf("  %d rows: 1. %d not finite, 2. %d warm differ, 3. %d off or unannounced\n", nrow,
         nbad[0], nbad[1], nbad[2]);
  k = nbad[0] + nbad[1] + nbad[2];
  printf(k ? "G38 FAIL: %d\n" : "G38 PASS\n", k);
  return k != 0;
}
