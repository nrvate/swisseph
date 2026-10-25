/* fictstar.c -- G32: a malformed line in seorbel.txt or sefstars.txt is
 * refused with a message that says where, in bounded time, and the rest of
 * the file keeps answering.
 *
 * seorbel.txt, a copy with Cupido's line (line L, body SE_FICT_OFFSET)
 * changed:
 *   O1-O6  one element field with a character no term has: the mean
 *          anomaly "12x", the semi-axis "40.99837e0", the eccentricity
 *          "0.0046q", the perihelion "171.4333 * x", the node "nan", the
 *          inclination "1.0833k" (the term reader once came back to the
 *          character for ever: swe_calc() hung);
 *   O7     the epoch "2451545.0abc";
 *   O8     the equinox empty;
 *   O9     the mean anomaly empty;
 *   O10    the line cut to five fields (Hades, Zeus and every later body
 *          must not be refused with it);
 *   O11-O13 a power "T5", a power "T0", a number "1.2.3".
 * And three pairs, Cupido's line and Hades' replaced by two spellings of
 * the same elements, which must answer bit for bit alike:
 *   O14    ".0 * T" as "0.0 * T";
 *   O15    "T", "+T2", "-T" as "1 * T", "1 * T2", "-1 * T";
 *   O16    "T 2" as "2 * T".
 * Each: Cupido fails within the 5 s alarm, its message naming the file,
 * the line and the field; Hades, Zeus and Kronos answer bit for bit as
 * from the complete file, and swe_get_planet_name() names Hades.
 *
 * sefstars.txt:
 *   F1     a star line with too few fields first: the catalogue does not
 *          load, and swe_fixstar2() and swe_fixstar2_mag() must say why,
 *          not only that the star was not found; built-in Spica answers as
 *          with the complete catalogue;
 *   F2     no sefstars.txt: the message names the missing file; Spica
 *          answers;
 *   F3     a star the catalogue does not have, with serr NULL:
 *          swe_fixstar() and swe_fixstar_mag() must fail, not answer with
 *          the file's last star.
 * Each case runs in a child process, in a directory of links to EPHEDIR's
 * files built in /tmp.
 * Run as
 *   fictstar EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

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

enum { ORB, ORBEQ, STARBAD, STARNONE, STARNULL };
struct kase { const char *name; int kind; int field; const char *value; const char *word; };

static const struct kase rgCase[] = {
  {"O1 mean anomaly \"12x\"", ORB, 2, " 12x", "mean anomaly"},
  {"O2 semi-axis \"40.99837e0\"", ORB, 3, " 40.99837e0", "semi-axis"},
  {"O3 eccentricity \"0.0046q\"", ORB, 4, " 0.0046q", "eccentricity"},
  {"O4 perihelion \"171.4333 * x\"", ORB, 5, " 171.4333 * x", "perihelion"},
  {"O5 node \"nan\"", ORB, 6, " nan", "node"},
  {"O6 inclination \"1.0833k\"", ORB, 7, " 1.0833k", "inclination"},
  {"O7 epoch \"2451545.0abc\"", ORB, 0, "2451545.0abc", "epoch"},
  {"O8 equinox empty", ORB, 1, " ", "equinox"},
  {"O9 mean anomaly empty", ORB, 2, " ", "mean anomaly"},
  {"O10 the line cut to five fields", ORB, -1, NULL, "nine elements"},
  {"O11 a power T5", ORB, 2, " 163.7409 + 1 * T5", "mean anomaly"},
  {"O12 a power T0", ORB, 6, " 129.8325 + T0", "node"},
  {"O13 a number \"1.2.3\"", ORB, 2, " 163.7409 + 1.2.3 * T", "mean anomaly"},
  /* ORBEQ: Cupido's line, then Hades' line; the two must answer alike */
  {"O14 \".0\" as 0.0", ORBEQ, 0,
   "J1900, J1900, 163.7409 + .0 * T, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, A\n",
   "J1900, J1900, 163.7409 + 0.0 * T, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, B\n"},
  {"O15 \"T\", \"+T2\", \"-T\" as 1 * T, 1 * T2, -1 * T", ORBEQ, 0,
   "J1900, J1900, 163.7409 + T, 40.99837 +T2, 0.00460, 171.4333, 129.8325 -T, 1.0833, A\n",
   "J1900, J1900, 163.7409 + 1 * T, 40.99837 + 1 * T2, 0.00460, 171.4333, 129.8325 - 1 * T, 1.0833, B\n"},
  {"O16 \"T 2\" as T * 2", ORBEQ, 0,
   "J1900, J1900, 163.7409 + T 2, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, A\n",
   "J1900, J1900, 163.7409 + 2 * T, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833, B\n"},
  {"F1 a star line with too few fields", STARBAD, 0, NULL, "incomplete"},
  {"F2 no star catalogue", STARNONE, 0, NULL, "sefstars.txt"},
  {"F3 a star not there, serr NULL", STARNULL, 0, NULL, NULL},
};
#define NCASE ((int) (sizeof rgCase / sizeof rgCase[0]))

static int iline_cupido;	/* the file's line number of the first body */

/* seorbel.txt with the first body's line changed */
static int craft_orb(const char *dir, const struct kase *k)
{
  char src[PATH_MAX + 40], dst[PATH_MAX + 40], s[1024];
  FILE *f, *g;
  int done = 0, n = 0;
  if (k->kind == ORBEQ)
    done = -2;	/* two lines to replace */
  snprintf(src, sizeof src, "%s/seorbel.txt", EPHE);
  snprintf(dst, sizeof dst, "%s/seorbel.txt", dir);
  if ((f = fopen(src, "r")) == NULL || (g = fopen(dst, "w")) == NULL)
    return -1;
  while (fgets(s, sizeof s, f) != NULL) {
    char *sp = s;
    n++;
    while (*sp == ' ' || *sp == '\t')
      sp++;
    if (done < 0 && *sp != '#' && *sp != '\r' && *sp != '\n' && *sp != '\0') {
      fputs(done == -2 ? k->value : k->word, g);
      done = done == -2 ? -1 : 1;
      continue;
    }
    if (!done && *sp != '#' && *sp != '\r' && *sp != '\n' && *sp != '\0') {
      char *fld[16], *t = s;
      int nf = 0, i;
      iline_cupido = n;
      done = 1;
      if (k->field < 0) {
        fputs("J1900, J1900, 163.7409, 40.99837, 0.00460\n", g);
        continue;
      }
      while (nf < 16 && (fld[nf++] = t) != NULL && (t = strchr(t, ',')) != NULL)
        *t++ = '\0';
      for (i = 0; i < nf; i++)
        fprintf(g, "%s%s", i == k->field ? k->value : fld[i], i + 1 < nf ? "," : "");
      continue;
    }
    fputs(s, g);
  }
  fclose(f);
  return fclose(g);
}

struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; };

static struct ans fict(swe_ctx *c, int ipl)
{
  struct ans a;
  memset(&a, 0, sizeof a);
  a.r = swe_calc_r(c, 2451545.0, ipl, SEFLG_SPEED, a.x, a.serr);
  return a;
}

static struct ans star2(swe_ctx *c, const char *name, int mag)
{
  struct ans a;
  char s[64];
  memset(&a, 0, sizeof a);
  snprintf(s, sizeof s, "%s", name);
  if (mag)
    a.r = swe_fixstar2_mag_r(c, s, a.x, a.serr);
  else
    a.r = swe_fixstar2_r(c, s, 2451545.0, SEFLG_SPEED, a.x, a.serr);
  return a;
}

static int same(const struct ans *a, const struct ans *b)
{
  return a->r == b->r && memcmp(a->x, b->x, sizeof a->x) == 0;
}

static swe_ctx *open_ctx(const char *dir)
{
  swe_ctx *c = swe_ctx_new();
  swe_set_ephe_path_r(c, dir);
  return c;
}

/* in the child: the number of failed checks */
static int run(const char *dir, const struct kase *k)
{
  int nbad = 0, i;
  swe_ctx *c = open_ctx(dir), *ref = open_ctx(EPHE);
  if (k->kind == ORB) {
    char want[64], name[AS_MAXCH], rname[AS_MAXCH];
    struct ans a = fict(c, SE_FICT_OFFSET);
    snprintf(want, sizeof want, "line %7.0f", (double) iline_cupido);
    if (a.r >= 0 || strstr(a.serr, "seorbel.txt") == NULL || strstr(a.serr, want) == NULL
        || strstr(a.serr, k->word) == NULL) {
      printf("    Cupido: %d, \"%s\"\n", a.r, a.serr);
      nbad++;
    } else
      printf("    %s\n", a.serr);
    for (i = 1; i <= 3; i++) {
      struct ans b = fict(c, SE_FICT_OFFSET + i), f = fict(ref, SE_FICT_OFFSET + i);
      if (!same(&b, &f)) {
        printf("    body %d: %d %.9f \"%s\" for %.9f\n", SE_FICT_OFFSET + i, b.r, b.x[0], b.serr, f.x[0]);
        nbad++;
      }
    }
    swe_get_planet_name_r(c, SE_FICT_OFFSET + 1, name);
    swe_get_planet_name_r(ref, SE_FICT_OFFSET + 1, rname);
    if (strcmp(name, rname) != 0) {
      printf("    the name of body %d: \"%s\" for \"%s\"\n", SE_FICT_OFFSET + 1, name, rname);
      nbad++;
    }
  } else if (k->kind == ORBEQ) {
    struct ans a = fict(c, SE_FICT_OFFSET), b = fict(c, SE_FICT_OFFSET + 1);
    if (a.r < 0 || !same(&a, &b)) {
      printf("    A %d %.12f %.12f \"%s\", B %d %.12f %.12f \"%s\"\n", a.r, a.x[0], a.x[3], a.serr,
             b.r, b.x[0], b.x[3], b.serr);
      nbad++;
    } else
      printf("    both %.9f, %.9f deg/day\n", a.x[0], a.x[3]);
    for (i = 2; i <= 3; i++) {
      struct ans d = fict(c, SE_FICT_OFFSET + i), f = fict(ref, SE_FICT_OFFSET + i);
      if (!same(&d, &f)) {
        printf("    body %d: %d %.9f for %.9f\n", SE_FICT_OFFSET + i, d.r, d.x[0], f.x[0]);
        nbad++;
      }
    }
  } else if (k->kind == STARBAD || k->kind == STARNONE) {
    int mag;
    for (mag = 0; mag < 2; mag++) {
      struct ans a = star2(c, "Aldebaran", mag), b = star2(c, "Spica", mag), f = star2(ref, "Spica", mag);
      if (a.r >= 0 || strstr(a.serr, k->word) == NULL) {
        printf("    %s Aldebaran: %d, \"%s\"\n", mag ? "mag" : "pos", a.r, a.serr);
        nbad++;
      } else
        printf("    %s\n", a.serr);
      /* built-in Spica, positions only: swe_fixstar2_mag() has no
       * built-in stars */
      if (!mag && !same(&b, &f)) {
        printf("    Spica: %d %.9f \"%s\" for %.9f\n", b.r, b.x[0], b.serr, f.x[0]);
        nbad++;
      }
    }
  } else {
    char s[64] = "Nonexistentstar";
    double x[6], m = 0;
    int32 r = swe_fixstar_r(c, s, 2451545.0, 0, x, NULL);
    if (r >= 0) {
      printf("    swe_fixstar(): %d, as %s\n", r, s);
      nbad++;
    }
    snprintf(s, sizeof s, "Nonexistentstar");
    r = swe_fixstar_mag_r(c, s, &m, NULL);
    if (r >= 0) {
      printf("    swe_fixstar_mag(): %d, %.2f as %s\n", r, m, s);
      nbad++;
    }
  }
  swe_ctx_free(c);
  swe_ctx_free(ref);
  return nbad;
}

int main(int argc, char **argv)
{
  int i, nbad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: fictstar EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], EPHE) == NULL) {
    perror(argv[1]);
    return 2;
  }
  printf("G32: a malformed seorbel.txt or sefstars.txt line refused, in bounded time, alone\n");
  for (i = 0; i < NCASE; i++) {
    const struct kase *k = &rgCase[i];
    char dir[] = "/tmp/fictstarXXXXXX";
    const char *skip = k->kind == ORB || k->kind == ORBEQ ? "seorbel.txt" : k->kind == STARNULL ? NULL : "sefstars.txt";
    int st = 0, ok = 0;
    pid_t pid;
    if (mkdtemp(dir) == NULL) {
      perror("mkdtemp");
      return 2;
    }
    if (link_all(EPHE, dir, skip) != 0)
      ok = -1;
    else if (k->kind == ORB || k->kind == ORBEQ)
      ok = craft_orb(dir, k);
    else if (k->kind == STARBAD) {
      char src[PATH_MAX + 40], dst[PATH_MAX + 40], s[1024];
      FILE *f, *g;
      snprintf(src, sizeof src, "%s/sefstars.txt", EPHE);
      snprintf(dst, sizeof dst, "%s/sefstars.txt", dir);
      if ((f = fopen(src, "r")) == NULL || (g = fopen(dst, "w")) == NULL)
        ok = -1;
      else {
        fputs("Atest,alTes,ICRS,1,2,3\n", g);
        while (fgets(s, sizeof s, f) != NULL)
          fputs(s, g);
        fclose(f);
        ok = fclose(g);
      }
    }
    if (ok != 0) {
      printf("  %s: set-up failed\n", k->name);
      unlink_all(dir);
      nbad++;
      continue;
    }
    fflush(stdout);
    if ((pid = fork()) == 0) {
      int r;
      alarm(5);	/* O1-O6 hung */
      r = run(dir, k);
      fflush(stdout);
      _exit(r != 0);
    }
    waitpid(pid, &st, 0);
    unlink_all(dir);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
      printf("  %s: FAIL (%s)\n", k->name,
             WIFSIGNALED(st) ? (WTERMSIG(st) == SIGALRM ? "hung, killed by the 5 s alarm" : "crashed")
                             : "wrong answer");
      nbad++;
    } else
      printf("  %s: %s, the rest unchanged\n", k->name, k->kind == ORBEQ ? "read alike" : "refused");
  }
  printf(nbad ? "G32 FAIL: %d\n" : "G32 PASS\n", nbad);
  return nbad != 0;
}
