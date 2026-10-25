/* missfile.c -- G99: a Swiss file missing, its neighbour asked and not
 * holding the instant, falls back as a missing file does: never a silent
 * NaN or another instant's place, and a warm context answers as a fresh one.
 * A file refused -- under another bucket's name, or a segment index that
 * does not read -- is cleared alone, leaving the other open files and the
 * spares of entry 89 as they were (UPSTREAM-BUGS 104).
 *
 * The defect (UPSTREAM-BUGS 100): with semo_12.se1 missing, at JD
 * 2159377.5 (1200.0 + 20 days) sweph() asked the neighbouring bucket's
 * file (F11), semo_06.se1, whose data ends 16 days past 1200.0; its range
 * check refused the instant and left the file open. sweplan() takes "the
 * moon file is not open" as its sign to use the Moshier Moon, so it went on
 * with a Moon it never read: a fresh context returned the Moon as NaN with
 * flags 0x100 and no error, a warm one the Moon of its last read (26.44179
 * deg for 349.29034), and the Sun, whose Earth is the EMB less that Moon,
 * 6.03 arcsec apart fresh and warm. Planets and the main asteroids already
 * fell back (sweplan() returns their NOT_AVAILABLE; the asteroid an error).
 *
 * Entry 104: read_const()'s and get_new_segment()'s refusals ran
 * free_planets(), which empties every body's constants while the other
 * files stay open: after a Ceres that met seas_06.se1 holding seas_00's
 * data, the next calls failed "sepl_06.se1 is damaged (2)", "semo_06.se1 is
 * damaged (2)", three of them before one answered, and the spares were
 * dropped and reopened; and the path setter's read of semo_18's header
 * closed a refused file twice, fclose(NULL): a crash.
 *
 * The checks, in directories built in /tmp of symbolic links to EPHEDIR's
 * files: (a) without semo_12, sepl_18 and seas_06, (b) without semo_18,
 * sepl_12 and seas_12 -- so the neighbour asked is the earlier bucket's or
 * the later's, and holds the instant or not; (c) with copies of semo_06,
 * sepl_12 and seas_00 as semo_12, sepl_18 and seas_06 (read_const() refuses
 * the name) and a copy of seas_00 with Pallas's segment index overwritten
 * (get_new_segment() refuses Pallas there, and only Pallas). At 1200.0 + 5,
 * 20, 50, 200, 400 and 50000 days, 1800.0 - 20, - 5, + 5, 20, 100 and
 * 50000, 600.0 + 5, 400, 50000 and - 50000, the Sun, Moon, Mercury, Mars,
 * Jupiter, Pluto, Chiron, Ceres and Juno, geocentric, heliocentric,
 * barycentric and J2000 equatorial XYZ true positions, all with SEFLG_SPEED:
 *   1. every answer is an error or six finite numbers;
 *   2. contexts warmed by the same call 1.5 and 30 days either side, by
 *      Mercury, the Moon and Ceres 25 days before, and by the call then the
 *      four that meet (c)'s refused files (Ceres 600.0 + 375, the Moon
 *      1200.0 + 20, Mercury 1800.0 + 100, Pallas 600.0 - 50000), or those
 *      four alone, answer as a fresh one, bit for bit, the return flag
 *      included;
 *   3. an answer is within 60 arcsec (XYZ: 3e-4 of the distance) of the
 *      complete directory's, and one more than 0.5 arcsec from it (a
 *      fallback, not a neighbour's file) says "Moshier" in serr.
   and (d) with a copy of semo_12 as semo_18, setting the path does not crash,
 * the Sun at J2000 is refused for the name and the Sun at 1800.0 - 100
 * days answers as the complete directory's.
 * Measured: 1728 rows, none fail. Before entry 100's fix (a) and (b) failed
 * 215 (21 not finite, 84 warm, 110 off or unannounced, all through the
 * Moon); before entry 104's, (c) fails 77 (all check 2) and (d) crashes;
 * the upstream build fails 521, all check 2 ((c) 201; (d) passes there,
 * its fix not behind the switch).
 * Run as
 *   missfile EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

static int nrow, nbad[3];
/* calls that meet a refused file in directory (c), run between two of
 * the call checked in its warm-ups 5 and 6 */
static struct { double jd; int ipl; } rgRefuse[4];

struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; };

static swe_ctx *open_ctx(const char *dir)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, dir);
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

static void bad(int k, const char *what, double jd, int ipl, int32 fl, const struct ans *a,
                const struct ans *b)
{
  if (nbad[0] + nbad[1] + nbad[2] < 20)
    printf("  %s: body %d, flags %#x, JD %.4f: %.9f / %.9f (flags %#x / %#x)\n", what, ipl,
           (unsigned) fl, jd, a->x[0], b ? b->x[0] : 0.0, (unsigned) a->r,
           b ? (unsigned) b->r : 0u);
  nbad[k]++;
}

/* a copy of ephe/src as dir/name, bytes [off, off + len) set to 0xff */
static int copy(const char *dir, const char *ephe, const char *name, const char *src,
                long off, long len)
{
  char p[PATH_MAX + 300];
  FILE *fi, *fo;
  long i = 0;
  int ch;
  snprintf(p, sizeof p, "%s/%s", ephe, src);
  if ((fi = fopen(p, "rb")) == NULL)
    return -1;
  snprintf(p, sizeof p, "%s/%s", dir, name);
  unlink(p);	/* never write through a link into EPHEDIR */
  if ((fo = fopen(p, "wb")) == NULL) {
    fclose(fi);
    return -1;
  }
  for (; (ch = getc(fi)) != EOF; i++)
    putc(i >= off && i < off + len ? 0xff : ch, fo);
  fclose(fi);
  return fclose(fo) == 0 ? 0 : -1;
}

/* directory (c)'s copies: three files under the previous bucket's name
 * (read_const() refuses them), and seas_00 with Pallas's segment index --
 * 439 entries of 3 bytes at offset 2722 -- overwritten, so
 * get_new_segment() refuses Pallas there */
static int build_copies(const char *dir, const char *ephe)
{
  return copy(dir, ephe, "semo_12.se1", "semo_06.se1", 0, 0)
      | copy(dir, ephe, "sepl_18.se1", "sepl_12.se1", 0, 0)
      | copy(dir, ephe, "seas_06.se1", "seas_00.se1", 0, 0)
      | copy(dir, ephe, "seas_00.se1", "seas_00.se1", 2722, 1317);
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
        for (k = 0; k < 7; k++) {
          swe_ctx *c = open_ctx(dir);
          if (k < 4) {
            calc(c, jd + rgW[k], ipl, fl);
          } else if (k == 4) {
            calc(c, jd - 25, SE_MERCURY, SEFLG_SPEED);
            calc(c, jd - 25, SE_MOON, SEFLG_SPEED);
            calc(c, jd - 25, SE_CERES, SEFLG_SPEED);
          } else {
            if (k == 5)
              calc(c, jd, ipl, fl);
            for (i = 0; i < 4; i++)
              calc(c, rgRefuse[i].jd, rgRefuse[i].ipl, SEFLG_SPEED);
          }
          b = calc(c, jd, ipl, fl);
          swe_ctx_free(c);
          if (a.r != b.r || memcmp(a.x, b.x, sizeof a.x) != 0) {
            bad(1, "warm differs from fresh", jd, ipl, fl, &a, &b);
            break;
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
  static const char *const rgName[3] = {"semo_12.se1", "sepl_18.se1", "seas_06.se1"};
  static const char *const rgName2[3] = {"semo_18.se1", "sepl_12.se1", "seas_12.se1"};
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
  rgRefuse[0].jd = J600 + 375;    rgRefuse[0].ipl = SE_CERES;
  rgRefuse[1].jd = J1200 + 20;    rgRefuse[1].ipl = SE_MOON;
  rgRefuse[2].jd = J1800 + 100;   rgRefuse[2].ipl = SE_MERCURY;
  rgRefuse[3].jd = J600 - 50000;  rgRefuse[3].ipl = SE_PALLAS;
  printf("G99: a missing, refused or non-covering Swiss file, the same warm and fresh\n");
  for (k = 0; k < 2; k++) {
    int nb0 = nbad[0] + nbad[1] + nbad[2];
    if (build(dir[k], ephe, k ? rgName2 : rgName, 3) != 0) {
      perror(dir[k]);
      unbuild(dir[k]);
      return 2;
    }
    run(dir[k], ephe, rgJd, n);
    unbuild(dir[k]);
    printf("  %s: %d fail\n", k ? "(b) semo_18, sepl_12, seas_12 missing"
                                : "(a) semo_12, sepl_18, seas_06 missing",
           nbad[0] + nbad[1] + nbad[2] - nb0);
  }
  /* (c) */
  {
    char dc[] = "/tmp/missfilcXXXXXX";
    static const char *const rgCopy[4] = {"semo_12.se1", "sepl_18.se1", "seas_06.se1",
                                          "seas_00.se1"};
    int nb0 = nbad[0] + nbad[1] + nbad[2];
    struct ans a;
    if (build(dc, ephe, rgCopy, 4) != 0 || build_copies(dc, ephe) != 0) {
      perror(dc);
      unbuild(dc);
      return 2;
    }
    /* the corrupted index refuses Pallas, and only Pallas, there */
    a = fresh_calc(dc, J600 - 50000, SE_PALLAS, SEFLG_SPEED);
    if (a.r >= 0 || strstr(a.serr, "damaged") == NULL
        || fresh_calc(dc, J600 - 50000, SE_CERES, SEFLG_SPEED).r < 0) {
      printf("  (c) set-up: Pallas's index not refused (%d, %s)\n", a.r, a.serr);
      nbad[1]++;
    }
    run(dc, ephe, rgJd, n);
    unbuild(dc);
    printf("  (c) semo_12, sepl_18, seas_06 under the previous bucket's name, seas_00 with\n"
           "      Pallas's segment index overwritten: %d fail\n",
           nbad[0] + nbad[1] + nbad[2] - nb0);
  }
  /* (d): the path setter reads semo_18's header; a refused one crashed it */
  {
    char dd[] = "/tmp/missfildXXXXXX";
    static const char *const rgOne[1] = {"semo_18.se1"};
    int st = 0;
    pid_t pid;
    fflush(stdout);
    if (build(dd, ephe, rgOne, 1) != 0 || copy(dd, ephe, "semo_18.se1", "semo_12.se1", 0, 0) != 0) {
      perror(dd);
      unbuild(dd);
      return 2;
    }
    if ((pid = fork()) == 0) {
      struct ans x = fresh_calc(dd, 2451545.0, SE_SUN, SEFLG_SPEED);
      struct ans y = fresh_calc(dd, J1800 - 100, SE_SUN, SEFLG_SPEED);
      struct ans z = fresh_calc(ephe, J1800 - 100, SE_SUN, SEFLG_SPEED);
      _exit(x.r < 0 && strstr(x.serr, "wrong") != NULL && y.r == z.r
            && memcmp(y.x, z.x, sizeof y.x) == 0 ? 0 : 1);
    }
    waitpid(pid, &st, 0);
    unbuild(dd);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
      printf("  (d) semo_18 under semo_12's name: %s\n",
             WIFSIGNALED(st) ? "the path setter crashed" : "wrong answers");
      nbad[1]++;
    } else
      printf("  (d) semo_18 under semo_12's name: 0 fail\n");
  }
  /* (e): both the instant's file and its neighbour missing -- serr names
   * the instant's own file, not the neighbour last tried */
  {
    char de[] = "/tmp/missfileXXXXXX";
    static const char *const rgTwo[2] = {"semo_12.se1", "semo_06.se1"};
    struct ans a;
    if (build(de, ephe, rgTwo, 2) != 0) {
      perror(de);
      unbuild(de);
      return 2;
    }
    a = fresh_calc(de, J1200 + 5, SE_MOON, SEFLG_SPEED);
    unbuild(de);
    if (strstr(a.serr, "semo_12.se1") == NULL || strstr(a.serr, "semo_06.se1") != NULL) {
      printf("  (e) semo_12 and semo_06 missing: serr '%s'\n", a.serr);
      nbad[2]++;
    } else
      printf("  (e) semo_12 and semo_06 missing: names semo_12, 0 fail\n");
  }
  printf("  %d rows: 1. %d not finite, 2. %d warm differ, 3. %d off or unannounced\n", nrow,
         nbad[0], nbad[1], nbad[2]);
  k = nbad[0] + nbad[1] + nbad[2];
  printf(k ? "G99 FAIL: %d\n" : "G99 PASS\n", k);
  return k != 0;
}
