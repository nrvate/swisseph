/* starfail.c -- G118: a sefstars.txt that failed to load is not read again,
 * until the path changes or swe_close().
 *
 * The defect (UPSTREAM-BUGS 121): a load of the catalogue that failed left
 * n_fixstars_records 0, so every later swe_fixstar2() and swe_fixstar2_mag()
 * loaded it again. A catalogue broken at line 500 was rewound, reread and
 * reparsed up to that line on every call (842 us a call, for 2.8 with the
 * whole catalogue; the file stays open, so no fopen()), and a missing one
 * was searched for along the path as sefstars.txt and fixstars.cat, two
 * fopen() a call. Now the failure and its error are kept until the path
 * changes or swe_close(), and each later call reports the same error
 * without touching the file (0.3 us a call).
 *
 * The check, each case in a child process, in a directory of links to
 * EPHEDIR's files built in /tmp, counting the library's fopen() and fgets()
 * calls (this program interposes both):
 *   C1  sefstars.txt broken at line 500: after the first failed call, 40
 *       rounds of swe_fixstar2() for Aldebaran and Sirius and
 *       swe_fixstar2_mag() for Aldebaran open and read nothing (0 and 0;
 *       before, 0 opens and 60000 lines), each refused with the first
 *       call's load error ("invalid line in fixed stars file") at the head
 *       of its message; built-in Spica still answers;
 *   C2  no sefstars.txt: the same, the error naming the missing file (0
 *       opens; before, 240);
 *   C3  a path to the complete catalogue after the failure loads it:
 *       Aldebaran answers bit for bit as it did before the broken path;
 *   C4  the broken file repaired in place, then swe_close() and the same
 *       path: Aldebaran answers bit for bit as from the complete catalogue;
 *   C5  the broken file repaired in place without either: the failure is
 *       kept, the call refused and nothing opened or read (the contract;
 *       before, the next call read the repaired file and answered).
 * Measured: the 5 cases pass. The build before the fix fails C1, C2 and C5.
 * The upstream build (SWE_UPSTREAM_COMPAT) fails all five, but for bug 92:
 * it keeps the first path's catalogue across the change of path, so the
 * broken one is never read; G24's star_load_kept(), which starts from
 * swe_close(), asserts this defect's C5 both ways. Run as
 *   starfail EPHEDIR
 */
#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

#define TJD 2451545.0
#define NROUND 40

static int nopen, nread;
FILE *fopen(const char *p, const char *m)
{
  static FILE *(*real)(const char *, const char *);
  if (real == NULL)
    *(void **) &real = dlsym(RTLD_NEXT, "fopen");
  nopen++;
  return real(p, m);
}
char *fgets(char *s, int n, FILE *f)
{
  static char *(*real)(char *, int, FILE *);
  if (real == NULL)
    *(void **) &real = dlsym(RTLD_NEXT, "fgets");
  nread++;
  return real(s, n, f);
}

static char EPHE[PATH_MAX];

static int link_all(const char *from, const char *to)
{
  DIR *d = opendir(from);
  struct dirent *e;
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (d == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.' || strcmp(e->d_name, "sefstars.txt") == 0)
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

/* dir/sefstars.txt: EPHEDIR's, with line 500 replaced if broken; the
 * test's own opens and reads are not counted */
static int write_catalogue(const char *dir, int broken)
{
  char src[PATH_MAX + 40], dst[PATH_MAX + 40], s[1024];
  int n = 0, o = nopen, r = nread, ok;
  FILE *f, *g;
  snprintf(src, sizeof src, "%s/sefstars.txt", EPHE);
  snprintf(dst, sizeof dst, "%s/sefstars.txt", dir);
  if ((f = fopen(src, "r")) == NULL || (g = fopen(dst, "w")) == NULL)
    return -1;
  while (fgets(s, sizeof s, f) != NULL)
    fputs(++n == 500 && broken ? "a line without a comma\n" : s, g);
  fclose(f);
  ok = fclose(g);
  nopen = o;
  nread = r;
  return ok;
}

/* the load error at the head of a refusal: the text before "; " */
static void head_of(const char *serr, char *h, size_t n)
{
  const char *e = strstr(serr, "; ");
  size_t k = e != NULL ? (size_t) (e - serr) : strlen(serr);
  if (k >= n)
    k = n - 1;
  memcpy(h, serr, k);
  h[k] = '\0';
}

/* after one failed call: NROUND rounds that must touch nothing and be
 * refused with the same load error; built-in Spica answers */
static int rounds_untouched(const char *word)
{
  char serr[AS_MAXCH], star[64], head[AS_MAXCH];
  double x[6], mag;
  int k, o, r;
  strcpy(star, "Aldebaran");
  if (swe_fixstar2(star, TJD, 0, x, serr) >= 0 || strstr(serr, word) == NULL) {
    printf("    first call: \"%s\"\n", serr);
    return 1;
  }
  head_of(serr, head, sizeof head);
  o = nopen;
  r = nread;
  for (k = 0; k < NROUND; k++) {
    strcpy(star, "Aldebaran");
    if (swe_fixstar2(star, TJD, 0, x, serr) >= 0 || strncmp(serr, head, strlen(head)) != 0)
      break;
    strcpy(star, "Sirius");
    if (swe_fixstar2(star, TJD, 0, x, serr) >= 0 || strncmp(serr, head, strlen(head)) != 0)
      break;
    strcpy(star, "Aldebaran");
    if (swe_fixstar2_mag(star, &mag, serr) >= 0 || strncmp(serr, head, strlen(head)) != 0)
      break;
  }
  printf("    %d opens, %d lines read in %d rounds; \"%s\"\n", nopen - o, nread - r,
         NROUND, head);
  if (k < NROUND) {
    printf("    round %d: \"%s\"\n", k, serr);
    return 1;
  }
  if (nopen != o || nread != r)
    return 1;
  strcpy(star, "Spica");	/* opens the planet files, as any star does */
  if (swe_fixstar2(star, TJD, 0, x, serr) < 0) {
    printf("    built-in Spica: \"%s\"\n", serr);
    return 1;
  }
  return 0;
}

static int aldebaran(double *x)
{
  char serr[AS_MAXCH], star[64];
  strcpy(star, "Aldebaran");
  return swe_fixstar2(star, TJD, 0, x, serr) < 0;
}

static int same(const double *a, const double *b)
{
  return memcmp(a, b, 6 * sizeof(double)) == 0;
}

static int run(int c, const char *dir)
{
  double xg[6], x[6];
  char serr[AS_MAXCH], star[64];
  int o, r;
  swe_set_ephe_path(EPHE);
  if (aldebaran(xg))	/* the complete catalogue's answer */
    return 1;
  swe_set_ephe_path(dir);
  switch (c) {
  case 1:
    return rounds_untouched("invalid line");
  case 2:
    return rounds_untouched("sefstars.txt");
  case 3:
    if (aldebaran(x) == 0)
      return 1;
    swe_set_ephe_path(EPHE);
    return aldebaran(x) || !same(x, xg);
  case 4:
    if (aldebaran(x) == 0 || write_catalogue(dir, 0) != 0)
      return 1;
    swe_close();
    swe_set_ephe_path(dir);
    return aldebaran(x) || !same(x, xg);
  case 5:
    if (aldebaran(x) == 0 || write_catalogue(dir, 0) != 0)
      return 1;
    o = nopen;
    r = nread;
    strcpy(star, "Aldebaran");
    if (swe_fixstar2(star, TJD, 0, x, serr) >= 0) {
      printf("    answered from the repaired file\n");
      return 1;
    }
    printf("    %d opens, %d lines read; \"%s\"\n", nopen - o, nread - r, serr);
    return nopen != o || nread != r;
  }
  return 1;
}

static const char *rgName[6] = {"", "C1 broken at line 500, kept", "C2 missing, kept",
  "C3 a new path loads", "C4 swe_close() loads", "C5 kept until then"};

int main(int argc, char **argv)
{
  int c, nbad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: starfail EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], EPHE) == NULL) {
    perror(argv[1]);
    return 2;
  }
  printf("G118: a sefstars.txt that failed to load is not read again\n");
  for (c = 1; c <= 5; c++) {
    char dir[] = "/tmp/starfailXXXXXX";
    int st = 0;
    pid_t pid;
    if (mkdtemp(dir) == NULL || link_all(EPHE, dir) != 0
        || (c != 2 && write_catalogue(dir, 1) != 0)) {
      printf("  %s: set-up failed\n", rgName[c]);
      unlink_all(dir);
      nbad++;
      continue;
    }
    printf("  %s\n", rgName[c]);
    fflush(stdout);
    if ((pid = fork()) == 0) {
      int r = run(c, dir);
      fflush(stdout);
      _exit(r != 0);
    }
    waitpid(pid, &st, 0);
    unlink_all(dir);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
      printf("    FAIL\n");
      nbad++;
    }
  }
  printf(nbad ? "G118 FAIL: %d\n" : "G118 PASS\n", nbad);
  return nbad != 0;
}
