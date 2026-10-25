/* getdenum.c -- G54: swe_get_denum() names the JPL ephemeris behind the
 * calculation that was made.
 *
 * The defect (UPSTREAM-BUGS 45). Upstream added swe_get_denum() on
 * 2026-10-19 (9a8b759) for swevents' header, "based on JPL DE%d". It reads
 * the DE number in the Swiss planet file's slot, whatever the ephemeris,
 * and answers SE_DE_NUMBER, 431, where that slot holds none. With the
 * shipped DE441 files, as merged here:
 *   before any call                       431, where nothing is known
 *   after swe_set_ephe_path()             431: the path setter reads the
 *                                         moon file, and the slot read is
 *                                         the planet file's
 *   after a Swiss Mars                    441
 *   after a JPL de200.eph Mars            441, the Swiss file's, kept
 *                                         when the ephemeris switch closed
 *                                         it; 200 was used
 *   after a Moshier Mars                  441
 *   after swe_close()                     431
 * so a caller printing it reports DE441 for a DE200 or Moshier run, and
 * DE431 for files that are DE441.
 *
 * The checks: each row above on the default context, the fresh process
 * first, and 0 for "nothing known"; a Moon alone (the moon file's); the
 * fallback on, Swiss asked of a directory that has none, Moshier answered
 * (404); after the JPL Mars, SE_ECL_NUT and a Swiss mean node, which read
 * no file, and a Swiss Mars out of range, which fails, each leaving the
 * answer as it was (the fix's first form forgot it at the close the switch
 * to Swiss makes, and named the Swiss files); a fixed star (swe_fixstar(),
 * swe_fixstar2()) and swe_calc_orbel(), which calculate without swe_calc()
 * (the fix's first form forgot the record at a star's switch and never
 * wrote one); a thread's first call after the main thread's
 * swe_set_ephe_path() (the published DE number); and a context of its own,
 * which answers for its own calculations whatever the default context did.
 * Last, files of mixed generations, as an old asteroid download beside new
 * planet files gives: EPHEDIR's, but for a moon file of DE406 and a main
 * asteroid file of DE431, crafted. The path setter answers the moon
 * file's; Mars, a fixed star and the planet file's; the Moon and the true
 * node the moon file's; Ceres, and Ceres as asteroid 10001 (which the
 * fix's first form looked for in the numbered-asteroid slot), the asteroid
 * file's; a mean node leaves the answer as it was. The shipped files are
 * all DE441, so without these the Swiss checks could not tell the files
 * apart. The expected Swiss numbers are the files' own, from
 * swe_get_current_file_data(). Moshier's is 404, the ephemeris its theory
 * was fitted to, which the library's tidal term takes for it too. Run as
 *   getdenum EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"
#include "swephlib.h"	/* swi_crc32(), for the crafted headers */

static int bad, n;
static const double TJD = 2451545.0;

static void check(const char *what, int32 got, int32 want)
{
  n++;
  printf("  %-52s %4d%s\n", what, got, got == want ? "" : "  FAIL");
  if (got != want) {
    printf("    expected %d\n", want);
    bad++;
  }
}

static int file_denum(int ifno)
{
  double t0, t1;
  int denum = 0;
  return swe_get_current_file_data(ifno, &t0, &t1, &denum) != NULL ? denum : 0;
}

/* Copy ephe/name to dir/name with the DE number de, its header's CRC
 * redone: after three text lines, the endian test, the length, the DE
 * number, start and end, nplan and its body list, then the CRC of all
 * before it. 0 or -1. */
static int craft(const char *ephe, const char *dir, const char *name, int32 de)
{
  char path[PATH_MAX + 64];
  unsigned char *b;
  long n, i = 0, at;
  int k = 0, rc = -1;
  uint32 crc;
  FILE *f;
  snprintf(path, sizeof path, "%s/%s", ephe, name);
  if ((f = fopen(path, "rb")) == NULL)
    return -1;
  fseek(f, 0, SEEK_END);
  n = ftell(f);
  rewind(f);
  if ((b = malloc((size_t) n)) == NULL || fread(b, 1, (size_t) n, f) != (size_t) n) {
    fclose(f);
    free(b);
    return -1;
  }
  fclose(f);
  while (k < 3 && i + 1 < n)
    if (b[i++] == '\r' && b[i] == '\n')
      i++, k++;
  if (k == 3 && i + 30 < n && memcmp(b + i, "cba", 3) == 0) {
    at = i + 30 + 2L * (b[i + 28] | b[i + 29] << 8);
    memcpy(b + i + 8, &de, 4);
    crc = swi_crc32(b, (int) at);
    memcpy(b + at, &crc, 4);
    snprintf(path, sizeof path, "%s/%s", dir, name);
    if ((f = fopen(path, "wb")) != NULL) {
      rc = fwrite(b, 1, (size_t) n, f) == (size_t) n ? 0 : -1;
      if (fclose(f) != 0)
        rc = -1;
    }
  }
  free(b);
  return rc;
}

/* dir: links to ephe's files, but for the moon file and the main asteroid
 * file, crafted with the DE numbers 406 and 431 */
static int mixed(const char *ephe, const char *dir)
{
  DIR *d;
  struct dirent *e;
  char abs[PATH_MAX], p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (realpath(ephe, abs) == NULL || (d = opendir(abs)) == NULL)
    return -1;
  ephe = abs;	/* the links are read from dir */
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || strcmp(e->d_name, "semo_18.se1") == 0
        || strcmp(e->d_name, "seas_18.se1") == 0)
      continue;
    snprintf(p, sizeof p, "%s/%s", ephe, e->d_name);
    snprintf(q, sizeof q, "%s/%s", dir, e->d_name);
    if (symlink(p, q) != 0) {
      closedir(d);
      return -1;
    }
  }
  closedir(d);
  return craft(ephe, dir, "semo_18.se1", 406) | craft(ephe, dir, "seas_18.se1", 431);
}

static void unmixed(const char *dir)
{
  DIR *d = opendir(dir);
  struct dirent *e;
  char p[PATH_MAX + 300];
  if (d == NULL)
    return;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.')
      continue;
    snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
    unlink(p);
  }
  closedir(d);
  rmdir(dir);
}

static int32 thread_answer;
static void *first_call(void *arg)
{
  (void) arg;
  thread_answer = swe_get_denum();
  return NULL;
}

int main(int argc, char **argv)
{
  double x[6];
  char serr[AS_MAXCH], empty[] = "/tmp/getdenum-XXXXXX", mix[] = "/tmp/getdenum-XXXXXX";
  int32 swiss, moon;
  swe_ctx *c;
  pthread_t th;
  if (argc != 2) {
    fprintf(stderr, "usage: getdenum EPHEDIR\n");
    return 2;
  }
  printf("G54: swe_get_denum() names the ephemeris that answered\n");
  check("before any call", swe_get_denum(), 0);

  swe_set_ephe_path(argv[1]);
  moon = file_denum(1);
  check("after swe_set_ephe_path(): the moon file's", swe_get_denum(), moon);
  if (pthread_create(&th, NULL, first_call, NULL) == 0) {
    pthread_join(th, NULL);
    check("a new thread before its first calculation", thread_answer, moon);
  } else {
    printf("  cannot start a thread\n");
    bad++;
  }

  swe_calc(TJD, SE_MARS, SEFLG_SWIEPH, x, serr);
  swiss = file_denum(0);
  check("after a Swiss Mars: the planet file's", swe_get_denum(), swiss);

  swe_set_jpl_file("de200.eph");
  if (swe_calc(TJD, SE_MARS, SEFLG_JPLEPH, x, serr) != SEFLG_JPLEPH)
    printf("  (de200.eph Mars did not answer from JPL: %s)\n", serr);
  check("after a JPL de200.eph Mars", swe_get_denum(), 200);
  /* calls that read no file, or fail, after the switch's close of the
   * other ephemeris: the record is the JPL Mars's still */
  swe_calc(TJD, SE_ECL_NUT, 0, x, serr);
  check("SE_ECL_NUT after it: unchanged", swe_get_denum(), 200);
  swe_calc(TJD, SE_MEAN_NODE, SEFLG_SWIEPH, x, serr);
  check("a Swiss mean node after it: unchanged", swe_get_denum(), 200);
  swe_calc(1e7, SE_MARS, SEFLG_SWIEPH, x, serr);
  check("a Swiss Mars out of range after it: unchanged", swe_get_denum(), 200);

  swe_calc(TJD, SE_MARS, SEFLG_MOSEPH, x, serr);
  check("after a Moshier Mars", swe_get_denum(), 404);

  /* the fixed stars and swe_calc_orbel() calculate without swe_calc() */
  {
    char star[64] = "Aldebaran";
    static const double el[6] = { 10, 2.77, 0.08, 73, 80, 10.6 };
    swe_fixstar(star, TJD, SEFLG_JPLEPH, x, serr);
    check("a JPL de200.eph Aldebaran after it", swe_get_denum(), 200);
    swe_fixstar2(star, TJD, SEFLG_MOSEPH, x, serr);
    check("a Moshier Aldebaran (swe_fixstar2) after it", swe_get_denum(), 404);
    swe_calc_orbel(TJD, el, SE_SUN, SE_ORBEL_EQ_J2000, 0, SEFLG_JPLEPH, x, serr);
    check("a JPL swe_calc_orbel() after it", swe_get_denum(), 200);
  }

  swe_calc(TJD, SE_MOON, SEFLG_SWIEPH, x, serr);
  check("after a Swiss Moon alone: the moon file's", swe_get_denum(), file_denum(1));

  /* its own context: answers for itself, whatever the default context does */
  c = swe_ctx_new();
  swe_set_jpl_file_r(c, "de200.eph");
  swe_calc_r(c, TJD, SE_MARS, SEFLG_JPLEPH, x, serr);
  swe_calc(TJD, SE_MARS, SEFLG_SWIEPH, x, serr);
  check("a context of its own after its JPL Mars", swe_get_denum_r(c), 200);
  check("the default context meanwhile, a Swiss Mars", swe_get_denum(), swiss);
  swe_ctx_free(c);

  swe_close();
  check("after swe_close()", swe_get_denum(), 0);

  /* the fallback on: Swiss asked, none there, Moshier answered */
  if (mkdtemp(empty) == NULL) {
    printf("  cannot make %s\n", empty);
    bad++;
  } else {
    int32 r;
    swe_set_ephe_fallback(1);
    swe_set_ephe_path(empty);
    check("after swe_set_ephe_path() on no files", swe_get_denum(), 0);
    r = swe_calc(TJD, SE_MARS, SEFLG_SWIEPH, x, serr);
    if (!(r & SEFLG_MOSEPH))
      printf("  (Mars on no files returned %d, not Moshier)\n", r);
    check("Swiss asked of no files, Moshier answered", swe_get_denum(), 404);
    swe_set_ephe_fallback(0);
    swe_close();
    rmdir(empty);
  }

  /* files of mixed generations: each body answers for the file it is read
   * from, a fixed star for the planet file */
  if (mkdtemp(mix) == NULL || mixed(argv[1], mix) != 0) {
    printf("  cannot build the mixed files in %s\n", mix);
    bad++;
  } else {
    char star[64] = "Aldebaran";
    swe_set_ephe_path(mix);
    check("mixed files: the path setter, the moon file's", swe_get_denum(), 406);
    swe_calc(TJD, SE_MARS, SEFLG_SWIEPH, x, serr);
    check("mixed files: Mars, the planet file's", swe_get_denum(), 441);
    swe_calc(TJD, SE_MOON, SEFLG_SWIEPH, x, serr);
    check("mixed files: the Moon, the moon file's", swe_get_denum(), 406);
    swe_calc(TJD, SE_MEAN_NODE, SEFLG_SWIEPH, x, serr);
    check("mixed files: a mean node after it, unchanged", swe_get_denum(), 406);
    swe_calc(TJD, SE_CERES, SEFLG_SWIEPH, x, serr);
    check("mixed files: Ceres, the asteroid file's", swe_get_denum(), 431);
    swe_calc(TJD, SE_TRUE_NODE, SEFLG_SWIEPH, x, serr);
    check("mixed files: the true node, the moon file's", swe_get_denum(), 406);
    swe_calc(TJD, SE_AST_OFFSET + 1, SEFLG_SWIEPH, x, serr);
    check("mixed files: Ceres as 10001, the asteroid file's", swe_get_denum(), 431);
    swe_fixstar(star, TJD, SEFLG_SWIEPH, x, serr);
    check("mixed files: Aldebaran, the planet file's", swe_get_denum(), 441);
    swe_close();
  }
  unmixed(mix);

  if (bad) {
    printf("G54 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("G54 PASS (%d)\n", n);
  return 0;
}
