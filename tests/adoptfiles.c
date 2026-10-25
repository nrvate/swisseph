/* adoptfiles.c -- G116: a thread that adopts another thread's file setting
 * answers from the new files, as the setting thread and a fresh process do.
 *
 * The defects (UPSTREAM-BUGS 119), both in swi_config_apply(), where a
 * thread following the shared configuration takes on another thread's
 * swe_set_jpl_file() or swe_set_ephe_path():
 *
 * 1. A new JPL file on the same path closed the old one and kept every
 *    position it had filled, and jplplan()'s Earth, cached on the instant
 *    and the ephemeris alone, answered from it: with de200 and a copy of it
 *    whose EMB coefficients are scaled by 1 + 1e-5, Mercury at J2000 on the
 *    adopting thread kept de200's 271.8881245810 for 271.8881823960
 *    (5.8e-5 deg).
 * 2. A new path closed the Swiss files and kept their bodies' constants
 *    and segments, which the next calculation's switch of ephemeris keeps
 *    too (bug 103); read_const() frees a segment only for a body with a
 *    reference ellipse, so Jupiter to Pluto, Chiron and the main asteroids
 *    were read from the old directory's segment: with a copy of
 *    sepl_18.se1 whose Jupiter segment at J2000 has one coefficient moved,
 *    Jupiter at J2000 kept 25.253056935371 for 25.253053515612, and a day
 *    later 25.295503129714 for 25.295499716142 (3.4e-6 deg).
 *
 * The check, each scenario in a forked, pristine process with the crafted
 * file built in a /tmp directory of links to EPHEDIR's files: the main
 * thread sets the path and the file, a second thread computes from them,
 * the main thread sets the crafted file (or its directory) and computes
 * the reference, and the second thread, which had computed J2000 and the
 * case's instant before, computes again. Its return and its longitude,
 * latitude, distance and longitude speed must equal the main thread's
 * within 1e-12 (deg, AU, deg/day), and the crafted file must move the
 * answer by more than 1e-7 deg where it was crafted to, or the case proves
 * nothing. Cases: Mercury and the Sun on JPL at J2000 and J2000 + 0.5;
 * Jupiter on the Swiss files at J2000 and J2000 + 1; Saturn at the same
 * two, whose segment was not moved, as controls. After the fix all 8 pass.
 *
 * Measured before it, 6 of 8 fail: Mercury 5.78e-5 and 5.24e-5 deg off,
 * the Sun 1.01e-5 at both instants, Jupiter 3.42e-6 and 3.41e-6; the two
 * controls pass. The compat build passes all 8: there the next
 * calculation's switch of ephemeris frees every body's data, as
 * upstream's does. Not run against
 * the upstream build (legacy-master): this program finds Jupiter's segment
 * through swe_ctx, which it has not, and its threads share no
 * configuration, so its second thread never adopts the files. Run as
 *   adoptfiles EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"
#include "sweph.h"

#define TOL 1e-12
#define MOVED 1e-7

static char EPHE[PATH_MAX];

static int link_all(const char *from, const char *to, const char *skip)
{
  DIR *d = opendir(from);
  struct dirent *e;
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (d == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || (skip != NULL && strcmp(e->d_name, skip) == 0))
      continue;
    snprintf(p, sizeof p, "%s/%s", from, e->d_name);
    snprintf(q, sizeof q, "%s/%s", to, e->d_name);
    if (symlink(p, q) != 0) {
      closedir(d);
      return -1;
    }
  }
  closedir(d);
  return 0;
}

static void unlink_all(const char *dir)
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

static unsigned char *slurp(const char *path, long *n)
{
  FILE *f = fopen(path, "rb");
  unsigned char *b;
  if (f == NULL)
    return NULL;
  fseek(f, 0, SEEK_END);
  *n = ftell(f);
  rewind(f);
  b = malloc((size_t) *n);
  if (b == NULL || fread(b, 1, (size_t) *n, f) != (size_t) *n) {
    free(b);
    b = NULL;
  }
  fclose(f);
  return b;
}

static int spill(const char *path, const unsigned char *b, long n)
{
  FILE *f = fopen(path, "wb");
  if (f == NULL)
    return -1;
  if (fwrite(b, 1, (size_t) n, f) != (size_t) n) {
    fclose(f);
    return -1;
  }
  return fclose(f);
}

/* de200x.eph: de200.eph with every EMB coefficient scaled by 1 + 1e-5.
 * The JPL header: three 84-byte titles, 400 6-byte constant names, then
 * ss[3], ncon, au, emrat, ipt[12][3], numde, lpt[3] (native byte order). */
static int craft_jpl(const char *dir)
{
  char src[PATH_MAX + 40], dst[PATH_MAX + 40];
  unsigned char *b;
  long n, rec, r;
  int32 ipt[13][3], numde, i, k, nc, j;
  snprintf(src, sizeof src, "%s/de200.eph", EPHE);
  snprintf(dst, sizeof dst, "%s/de200x.eph", dir);
  if ((b = slurp(src, &n)) == NULL)
    return -1;
  memcpy(ipt, b + 2652 + 24 + 4 + 16, 36 * 4);
  memcpy(&numde, b + 2652 + 24 + 4 + 16 + 144, 4);
  memcpy(ipt[12], b + 2652 + 24 + 4 + 16 + 148, 12);
  if (numde != 200) {		/* another byte order: not crafted here */
    free(b);
    return -1;
  }
  for (k = 0, i = 1; i < 13; i++)
    if (ipt[i][0] > ipt[k][0])
      k = i;
  nc = ipt[k][0] - 1 + ipt[k][1] * ipt[k][2] * (k == 11 ? 2 : 3);
  rec = (long) nc * 8;
  for (r = 2; (r + 1) * rec <= n; r++)
    for (j = 0; j < ipt[2][2] * 3; j++) {
      double v;
      long off = r * rec + (long) (ipt[2][0] - 1 + j * ipt[2][1]) * 8;
      memcpy(&v, b + off, 8);
      v *= 1 + 1e-5;
      memcpy(b + off, &v, 8);
    }
  j = spill(dst, b, n);
  free(b);
  return j;
}

/* sepl_18.se1 with one coefficient of Jupiter's segment at J2000 moved:
 * bit 9 of the first packed coefficient (its sign bit, bit 0, kept). The
 * segment's place from the file's own index, as get_new_segment() reads
 * it. */
static int craft_sepl(const char *dir)
{
  char src[PATH_MAX + 40], dst[PATH_MAX + 40], serr[AS_MAXCH];
  unsigned char *b;
  long n, fpos, p;
  double x[6];
  int iseg, lit, hdr, ret;
  struct plan_data *pdp;
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, EPHE);
  ret = swe_calc_r(c, J2000, SE_JUPITER, SEFLG_SWIEPH, x, serr);
  pdp = &c->pldat[SEI_JUPITER];
  if (ret < 0 || pdp->dseg <= 0) {
    swe_ctx_free(c);
    return -1;
  }
  iseg = (int) ((J2000 - pdp->tfstart) / pdp->dseg);
  p = pdp->lndx0 + iseg * 3L;
  lit = (c->fidat[SEI_FILE_PLANET].iflg & SEI_FILE_LITENDIAN) != 0;
  swe_ctx_free(c);
  snprintf(src, sizeof src, "%s/sepl_18.se1", EPHE);
  snprintf(dst, sizeof dst, "%s/sepl_18.se1", dir);
  if ((b = slurp(src, &n)) == NULL)
    return -1;
  fpos = lit ? b[p] | (long) b[p + 1] << 8 | (long) b[p + 2] << 16
             : b[p + 2] | (long) b[p + 1] << 8 | (long) b[p] << 16;
  hdr = (b[fpos] & 128) ? 4 : 2;
  b[fpos + hdr + (lit ? 1 : 2)] ^= 0x02;
  ret = spill(dst, b, n);
  free(b);
  return ret;
}

/* ------------------------------------------------------------------ */

struct scen {
  const char *name;
  int jpl;		/* 1: JPL file switch; 0: path switch */
  int ipl;
  double tjd;
};

static const struct scen rgScen[] = {
  {"J1 Mercury, JPL, J2000", 1, SE_MERCURY, J2000},
  {"J2 Sun, JPL, J2000", 1, SE_SUN, J2000},
  {"J3 Mercury, JPL, J2000 + 0.5", 1, SE_MERCURY, J2000 + 0.5},
  {"J4 Sun, JPL, J2000 + 0.5", 1, SE_SUN, J2000 + 0.5},
  {"S1 Jupiter, Swiss, J2000", 0, SE_JUPITER, J2000},
  {"S2 Jupiter, Swiss, J2000 + 1", 0, SE_JUPITER, J2000 + 1},
  {"S3 Saturn, Swiss, J2000", 0, SE_SATURN, J2000},
  {"S4 Saturn, Swiss, J2000 + 1", 0, SE_SATURN, J2000 + 1},
};
#define NSCEN ((int) (sizeof rgScen / sizeof rgScen[0]))

static const struct scen *cur;
static int32 cur_flag;
static pthread_barrier_t bar1, bar2;
static double x_warm[6], x_old[6];
static int32 r_warm, r_old;

static void *worker(void *arg)
{
  char serr[AS_MAXCH];
  (void) arg;
  /* both instants of the scenario's pair, so the warm thread holds what a
   * real one would: the instant itself, and its segment */
  r_old = swe_calc(J2000, cur->ipl, cur_flag, x_old, serr);
  r_old = swe_calc(cur->tjd, cur->ipl, cur_flag, x_old, serr);
  pthread_barrier_wait(&bar1);
  pthread_barrier_wait(&bar2);
  r_warm = swe_calc(cur->tjd, cur->ipl, cur_flag, x_warm, serr);
  return NULL;
}

/* 0 pass, 1 fail, 2 set-up failed */
static int run(const struct scen *s, const char *dir)
{
  pthread_t th;
  char serr[AS_MAXCH];
  double x_ref[6];
  int32 r_ref;
  int i, bad = 0;
  double dmove = 0, dwarm = 0;
  cur = s;
  cur_flag = (s->jpl ? SEFLG_JPLEPH : SEFLG_SWIEPH) | SEFLG_SPEED;
  pthread_barrier_init(&bar1, NULL, 2);
  pthread_barrier_init(&bar2, NULL, 2);
  if (s->jpl) {
    swe_set_ephe_path(dir);
    swe_set_jpl_file("de200.eph");
  } else {
    swe_set_ephe_path(EPHE);
  }
  if (pthread_create(&th, NULL, worker, NULL) != 0)
    return 2;
  pthread_barrier_wait(&bar1);
  if (s->jpl)
    swe_set_jpl_file("de200x.eph");
  else
    swe_set_ephe_path(dir);
  r_ref = swe_calc(s->tjd, s->ipl, cur_flag, x_ref, serr);
  pthread_barrier_wait(&bar2);
  pthread_join(th, NULL);
  if (r_ref < 0 || r_old < 0)
    return 2;
  for (i = 0; i < 4; i++) {
    double d = fabs(x_warm[i] - x_ref[i]);
    if (i == 0 && d > 180) d = 360 - d;
    if (d > dwarm) dwarm = d;
  }
  dmove = fabs(x_ref[0] - x_old[0]) + fabs(x_ref[1] - x_old[1]);
  if (r_warm != r_ref || dwarm > TOL)
    bad = 1;
  printf("  %-32s %s  adopting thread off by %.3g deg (crafted file moves it %.3g)\n",
         s->name, bad ? "FAIL" : "ok  ", dwarm, dmove);
  /* the crafted file must matter where the defect is: at J2000, and for
   * Jupiter a day on; elsewhere it may or may not */
  if ((s->tjd == J2000 && s->ipl != SE_SATURN) || s->ipl == SE_JUPITER)
    if (dmove < MOVED) {
      printf("  %-32s the crafted file does not move the answer\n", s->name);
      return 2;
    }
  return bad;
}

int main(int argc, char **argv)
{
  char dirj[] = "/tmp/adoptfjXXXXXX", dirs[] = "/tmp/adoptfsXXXXXX";
  int i, nbad = 0, setup = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: adoptfiles EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], EPHE) == NULL) {
    perror(argv[1]);
    return 2;
  }
  printf("G116: a thread adopting another's JPL file or path answers from the new files\n");
  if (mkdtemp(dirj) == NULL || mkdtemp(dirs) == NULL) {
    perror("mkdtemp");
    return 2;
  }
  if (link_all(EPHE, dirj, NULL) != 0 || craft_jpl(dirj) != 0
      || link_all(EPHE, dirs, "sepl_18.se1") != 0 || craft_sepl(dirs) != 0)
    setup = 1;
  for (i = 0; !setup && i < NSCEN; i++) {
    int st = 0;
    pid_t pid;
    fflush(stdout);
    if ((pid = fork()) == 0)
    {
      int r = run(&rgScen[i], rgScen[i].jpl ? dirj : dirs);
      fflush(stdout);
      _exit(r);
    }
    if (pid < 0 || waitpid(pid, &st, 0) != pid || !WIFEXITED(st) || WEXITSTATUS(st) == 2) {
      printf("  %s: set-up failed or crashed\n", rgScen[i].name);
      setup = 1;
    } else if (WEXITSTATUS(st) != 0) {
      nbad++;
    }
  }
  unlink_all(dirj);
  unlink_all(dirs);
  if (setup) {
    printf("G116 FAIL: set-up\n");
    return 1;
  }
  if (nbad == 0)
    printf("G116 PASS\n");
  else
    printf("G116 FAIL: %d\n", nbad);
  return nbad != 0;
}
