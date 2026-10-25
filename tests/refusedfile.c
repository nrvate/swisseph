/* refusedfile.c -- G39: a Swiss file that is refused -- under another
 * bucket's name, or with a segment that cannot be read -- is closed and
 * cleared alone. The files left open keep their constants, so the calls
 * after it answer on healthy files as a fresh context does, and the path
 * setter does not crash on a refused moon file.
 *
 * read_const() refuses a file that is not what its name says ("Ephemeris
 * file name 'seas_06.se1' wrong; rename 'seas_00.se1'"), or is damaged;
 * get_new_segment() one whose segment cannot be read. Both closed the file
 * and ran free_planets(), which zeroes every body's plan_data, the constants
 * (tfstart, dseg, lndx0) of the files that stay open too. The next call on
 * one of those found its file open, skipped read_const(), and looked a
 * segment up with dseg 0: an error, which closed that file in turn. With
 * seas_06.se1 a link to seas_00.se1, after Ceres at 600.0 + 375 days the Sun
 * at 600.0 + 400 said "sepl_06.se1 is damaged (2)", Mars there "semo_06.se1
 * is damaged (2)", Mars 1000 days on "sepl_06.se1 is damaged (2)", and only
 * the fourth call answered; a fresh context answers all three. And the path
 * setter, which reads semo_18's header for the Moon's tidal term, closed a
 * file read_const() had already closed: fclose(NULL), a crash with
 * semo_18.se1 holding semo_12's data.
 *
 * The cases, in directories in /tmp of symbolic links to EPHEDIR's files
 * with one fixture each:
 *   (a) seas_06 a copy of seas_00: Ceres at 600.0 + 375 is refused for the
 *       name; then the Sun, Moon and Mars at 600.0 + 400 and Mars at
 *       600.0 + 1400 answer as a fresh context does;
 *   (b) seas_00 with Pallas's segment index overwritten (439 entries of
 *       three bytes at offset 2722): Ceres at 600.0 - 50000 answers, Pallas
 *       is refused as damaged, Ceres, Mars, the Sun and the Moon 25 days on
 *       answer as a fresh context does;
 *   (c) sepl_18 a copy of sepl_12: after the Sun at 1800.0 - 100, Mercury at
 *       1800.0 + 100 is refused for the name; the Moon, Mars and Ceres at
 *       1800.0 - 75 answer as a fresh context does;
 *   (d) semo_12 a copy of semo_06: after Mars at 1200.0 - 400, the Moon at
 *       1200.0 + 20 is refused for the name; the Sun, Moon and Mars at
 *       1200.0 - 375 answer as a fresh context does;
 *   (e) semo_18 a copy of semo_12: setting the path does not crash (in a
 *       child process), J2000 is refused for the name, and the Sun at
 *       1800.0 - 100 answers as the complete directory's.
 * "As a fresh context does" is the same return flags and the same six
 * numbers, bit for bit, with SEFLG_SPEED.
 * Before the fix every case fails: (a) and (b) the Sun, Moon and Mars after
 * it, (c) the Moon, Mars and Ceres, (d) the Sun, and (e) crashes.
 * Run as
 *   refusedfile EPHEDIR
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

static int nbad;
static double J600, J1200, J1800;

struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; };
struct call { double jd; int ipl; };

static swe_ctx *open_ctx(const char *dir)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, dir);
  return c;
}

static struct ans calc(swe_ctx *c, double jd, int ipl)
{
  struct ans a;
  memset(&a, 0, sizeof a);
  a.r = swe_calc_r(c, jd, ipl, SEFLG_SPEED, a.x, a.serr);
  if (a.r < 0)
    memset(a.x, 0, sizeof a.x);
  return a;
}

static struct ans fresh_calc(const char *dir, double jd, int ipl)
{
  swe_ctx *c = open_ctx(dir);
  struct ans a = calc(c, jd, ipl);
  swe_ctx_free(c);
  return a;
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

/* a directory of links to every file of ephe but the one named */
static int build(char *dir, const char *ephe, const char *name)
{
  DIR *d;
  struct dirent *e;
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  if (mkdtemp(dir) == NULL || (d = opendir(ephe)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || strcmp(e->d_name, name) == 0)
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

struct scen {
  const char *name;
  const char *file;	/* the file of ephe the fixture replaces */
  const char *src;	/* the file it is a copy of */
  long off, len;	/* bytes of the copy overwritten */
  int nwarm;
  struct call warm[2];
  struct call refuse;
  const char *why;	/* what the refusal says */
  int nafter;
  struct call after[5];
};

/* a context that did the warm-up calls, met the refused file, and then
 * answers the later calls as a fresh one does */
static void run(const char *ephe, const struct scen *sc)
{
  char dir[] = "/tmp/refusedfileXXXXXX";
  int k, nb = 0;
  swe_ctx *c;
  struct ans a;
  if (build(dir, ephe, sc->file) != 0 || copy(dir, ephe, sc->file, sc->src, sc->off, sc->len) != 0) {
    printf("  %s: set-up failed\n", sc->name);
    nbad++;
    unbuild(dir);
    return;
  }
  c = open_ctx(dir);
  for (k = 0; k < sc->nwarm; k++)
    calc(c, sc->warm[k].jd, sc->warm[k].ipl);
  a = calc(c, sc->refuse.jd, sc->refuse.ipl);
  if (a.r >= 0 || strstr(a.serr, sc->why) == NULL) {
    printf("  %s: the file was not refused as '%s': %d, '%s'\n", sc->name, sc->why, a.r, a.serr);
    nb++;
  }
  for (k = 0; k < sc->nafter; k++) {
    struct ans b = calc(c, sc->after[k].jd, sc->after[k].ipl);
    struct ans f = fresh_calc(dir, sc->after[k].jd, sc->after[k].ipl);
    if (f.r < 0 || b.r != f.r || memcmp(b.x, f.x, sizeof b.x) != 0) {
      printf("  %s: body %d at JD %.1f after it: %d '%s' for %d (%.9f / %.9f)\n", sc->name,
             sc->after[k].ipl, sc->after[k].jd, b.r, b.serr, f.r, b.x[0], f.x[0]);
      nb++;
    }
  }
  swe_ctx_free(c);
  unbuild(dir);
  if (nb == 0)
    printf("  %s: refused, the later calls as a fresh context's\n", sc->name);
  nbad += nb != 0;
}

int main(int argc, char **argv)
{
  char ephe[PATH_MAX];
  struct scen sc[4];
  int k;
  if (argc != 2) {
    fprintf(stderr, "usage: refusedfile EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], ephe) == NULL) {
    perror(argv[1]);
    return 2;
  }
  J600 = swe_julday(600, 1, 1, 0, SE_JUL_CAL);
  J1200 = swe_julday(1200, 1, 1, 0, SE_JUL_CAL);
  J1800 = swe_julday(1800, 1, 1, 0, SE_GREG_CAL);
  memset(sc, 0, sizeof sc);
  sc[0] = (struct scen){"(a) seas_06 under seas_00's name", "seas_06.se1", "seas_00.se1", 0, 0,
    0, {{0, 0}}, {J600 + 375, SE_CERES}, "wrong",
    4, {{J600 + 400, SE_SUN}, {J600 + 400, SE_MOON}, {J600 + 400, SE_MARS},
        {J600 + 1400, SE_MARS}}};
  sc[1] = (struct scen){"(b) seas_00 with Pallas's index overwritten", "seas_00.se1", "seas_00.se1",
    2722, 1317,
    1, {{J600 - 50000, SE_CERES}}, {J600 - 50000, SE_PALLAS}, "damaged",
    4, {{J600 - 50000 + 25, SE_SUN}, {J600 - 50000 + 25, SE_MOON}, {J600 - 50000 + 25, SE_MARS},
        {J600 - 50000 + 10, SE_CERES}}};
  sc[2] = (struct scen){"(c) sepl_18 under sepl_12's name", "sepl_18.se1", "sepl_12.se1", 0, 0,
    1, {{J1800 - 100, SE_SUN}}, {J1800 + 100, SE_MERCURY}, "wrong",
    3, {{J1800 - 75, SE_MOON}, {J1800 - 75, SE_MARS}, {J1800 - 75, SE_CERES}}};
  sc[3] = (struct scen){"(d) semo_12 under semo_06's name", "semo_12.se1", "semo_06.se1", 0, 0,
    1, {{J1200 - 400, SE_MARS}}, {J1200 + 20, SE_MOON}, "wrong",
    3, {{J1200 - 375, SE_SUN}, {J1200 - 375, SE_MOON}, {J1200 - 375, SE_MARS}}};
  printf("G39: a refused Swiss file closed and cleared alone\n");
  for (k = 0; k < 4; k++)
    run(ephe, &sc[k]);
  /* (e): the path setter reads semo_18's header; a refused one crashed it */
  {
    char dd[] = "/tmp/refusedfileXXXXXX";
    int st = 0;
    pid_t pid;
    fflush(stdout);
    if (build(dd, ephe, "semo_18.se1") != 0 || copy(dd, ephe, "semo_18.se1", "semo_12.se1", 0, 0) != 0) {
      perror(dd);
      unbuild(dd);
      return 2;
    }
    if ((pid = fork()) == 0) {
      struct ans x = fresh_calc(dd, 2451545.0, SE_SUN);
      struct ans y = fresh_calc(dd, J1800 - 100, SE_SUN);
      struct ans z = fresh_calc(ephe, J1800 - 100, SE_SUN);
      _exit(x.r < 0 && strstr(x.serr, "wrong") != NULL && y.r == z.r
            && memcmp(y.x, z.x, sizeof y.x) == 0 ? 0 : 1);
    }
    waitpid(pid, &st, 0);
    unbuild(dd);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
      printf("  (e) semo_18 under semo_12's name: %s\n",
             WIFSIGNALED(st) ? "the path setter crashed" : "wrong answers");
      nbad++;
    } else
      printf("  (e) semo_18 under semo_12's name: the path setter survives, J2000 refused\n");
  }
  printf(nbad ? "G39 FAIL: %d\n" : "G39 PASS\n", nbad);
  return nbad != 0;
}
