/* hdrbound.c -- G103: values read from an ephemeris file's header, or from
 * the delta-psi/epsilon and fixed-star tables, are checked against the
 * arrays and buffers they index or size before they are used; a file that
 * fails is refused, its own slot cleared, and every other answer stays.
 *
 * The defects (UPSTREAM-BUGS 106), each a crafted copy of a real file,
 * header CRC recomputed where the reader checks one:
 *   S1-S3  a body number in sepl_12's list out of pldat[] (500), another
 *          file's slot (1, the Moon's), or the numbered-asteroid slot
 *          (65000): read_const() wrote pldat[ipli] for any of them;
 *   S4     the Moon's coefficient count 60 in semo_12: rot_back() holds
 *          MAXORD + 1 = 41 on the stack;
 *   S5-S7  Mercury's segment length 0, index offset 0x7fffff00, or span
 *          starting 1000 days into the file's: a segment number of inf, an
 *          int32 overflow, the wrong segment;
 *   S8     Mercury not listed (its entry Venus again): its slot read with
 *          the constants of no file, dseg 0;
 *   S9     sepm9501's element line opening with 30 digits: strncpy() copied
 *          19 + 31 bytes into sastnam[41];
 *   S10    sepm9501's coefficient count 60;
 *   J1     DE200 with the Earth-Moon barycentre's sub-intervals 2 -> 402
 *          and the Moon's 8 -> -492 (15 and 12 coefficients), the sum the
 *          length check uses kept: interp() read past buf[]'s 2500 doubles
 *          and the struct around it;
 *   E1, E2 eop_1962_today.txt with a short line (cpos[3] unset) or 36600
 *          days (dpsi[] holds 36525): the table must load, the short line
 *          skipped, the long one cut at 36525;
 *   F1     sefstars.txt with a line without a comma (cpos[1] NULL).
 * The checks: each case runs in a child process, in a directory of links
 * to EPHEDIR's files built in /tmp with the crafted file in place. The
 * call that meets it returns an error with a message. For the Swiss cases
 * the other kinds of file are open first (Mercury, the Moon and Io at
 * J2000, the crafted file met at 1700 or, for Io's, J2000), so a write
 * into their slots shows: then those bodies 40 days on and Mars at J2000
 * answer bit for bit as a fresh context on EPHEDIR does; after the JPL
 * case Mars from the Swiss files, after the tables the Sun. A crash fails
 * the case. Built twice, with ASan and with UBSan (G25's flags).
 * Measured: the 14 cases pass in both builds. Before the fix the ASan
 * build fails 10 (S1, S2, S4, S7, S9, S10, J1, E1, E2, F1) and the UBSan
 * build 13 (all but J1): every case fails in one.
 * Run as
 *   hdrbound EPHEDIR
 */
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"
#include "swephlib.h"
#include "sweph.h"	/* ctx->eop_dpsi_loaded: white-box for E1, E2 */

void load_dpsi_deps(swe_ctx *ctx);

static char EPHE[PATH_MAX];

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
    fclose(f);
    free(b);
    return NULL;
  }
  fclose(f);
  return b;
}

static int spill(const char *path, const unsigned char *b, long n)
{
  FILE *f;
  unlink(path);	/* never write through a link into EPHEDIR */
  if ((f = fopen(path, "wb")) == NULL)
    return -1;
  if (fwrite(b, 1, (size_t) n, f) != (size_t) n) {
    fclose(f);
    return -1;
  }
  return fclose(f);
}

static int link_all(const char *from, const char *to)
{
  DIR *d = opendir(from);
  struct dirent *e;
  char p[PATH_MAX + 300], q[PATH_MAX + 300];
  if (d == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.')
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
    if (unlink(p) != 0)
      unlink_all(p);	/* a directory of our own */
  }
  closedir(d);
  rmdir(dir);
}

/* the Swiss file's header: after nline text lines, the endian test, the
 * length, DE number, start and end (8 each), nplan (2), its body list (2
 * each), for a single-body file a 30-byte name, then the CRC of everything
 * before it, five general constants, and per body lndx0 (4), flags (1),
 * coefficient count (1), rmax (4) and ten doubles */
struct hdr { long ipl, crc, body; int nplan; };

static int parse(const unsigned char *b, long n, int nline, int single, struct hdr *h)
{
  long i = 0;
  int k = 0;
  while (k < nline && i + 1 < n)
    if (b[i++] == '\r' && b[i] == '\n')
      i++, k++;
  if (k < nline || memcmp(b + i, "cba", 3) != 0)
    return -1;	/* a big-endian file: not built for here */
  h->nplan = b[i + 28] | b[i + 29] << 8;
  h->ipl = i + 30;
  h->crc = h->ipl + 2L * h->nplan + (single ? 30 : 0);
  h->body = h->crc + 4 + 40;
  return h->body + 90 <= n ? 0 : -1;
}

static void put32(unsigned char *p, unsigned long v)
{
  p[0] = v & 0xff, p[1] = v >> 8 & 0xff, p[2] = v >> 16 & 0xff, p[3] = v >> 24 & 0xff;
}

static void recrc(unsigned char *b, const struct hdr *h)
{
  put32(b + h->crc, swi_crc32(b, (int) h->crc));
}

/* body k's record (sepl's bodies have no reference ellipse before Mercury,
 * which is first; semo and the moons have one body) */
static unsigned char *body(unsigned char *b, const struct hdr *h)
{
  return b + h->body;
}

struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; };

#define J2000 2451545.0

static struct ans calc_c(swe_ctx *c, double jd, int ipl, int32 fl)
{
  struct ans a;
  memset(&a, 0, sizeof a);
  a.r = swe_calc_r(c, jd, ipl, fl, a.x, a.serr);
  return a;
}

/* the same question in a fresh context on the complete directory */
static struct ans fresh(double jd, int ipl, int32 fl)
{
  swe_ctx *c = swe_ctx_new();
  struct ans a;
  swe_set_ephe_path_r(c, EPHE);
  a = calc_c(c, jd, ipl, fl);
  swe_ctx_free(c);
  return a;
}

enum { SWISS, MOONFILE, SAT, JPL, EOPSHORT, EOPLONG, STARS };
struct kase { const char *name; int kind; const char *file; int patch; };

static const struct kase rgCase[] = {
  {"S1 a body number out of pldat[]", SWISS, "sepl_12.se1", 1},
  {"S2 another file's body (the Moon)", SWISS, "sepl_12.se1", 2},
  {"S3 the numbered-asteroid slot", SWISS, "sepl_12.se1", 3},
  {"S4 the Moon's coefficient count 60", MOONFILE, "semo_12.se1", 4},
  {"S5 Mercury's segment length 0", SWISS, "sepl_12.se1", 5},
  {"S6 Mercury's index offset 0x7fffff00", SWISS, "sepl_12.se1", 6},
  {"S7 Mercury's span short of the file's", SWISS, "sepl_12.se1", 7},
  {"S8 Mercury not listed", SWISS, "sepl_12.se1", 8},
  {"S9 a moon's element line of 30 digits", SAT, "sat/sepm9501.se1", 9},
  {"S10 a moon's coefficient count 60", SAT, "sat/sepm9501.se1", 10},
  {"J1 DE200's sub-intervals traded, its length kept", JPL, "de200.eph", 11},
  {"E1 a short line in the delta-psi table", EOPSHORT, "eop_1962_today.txt", 0},
  {"E2 36600 days in the delta-psi table", EOPLONG, "eop_1962_today.txt", 0},
  {"F1 a star line without a comma", STARS, "sefstars.txt", 0},
};
#define NCASE ((int) (sizeof rgCase / sizeof rgCase[0]))

/* build the crafted file in dir; 0 or -1 */
static int craft(const char *dir, const struct kase *k)
{
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  unsigned char *b;
  long n;
  struct hdr h;
  int rc = -1;
  snprintf(dst, sizeof dst, "%s/%s", dir, k->file);
  if (k->kind == EOPSHORT || k->kind == EOPLONG) {
    FILE *f;
    int i, nd = k->kind == EOPSHORT ? 10 : 36600;
    unlink(dst);
    if ((f = fopen(dst, "w")) == NULL)
      return -1;
    if (k->kind == EOPSHORT)
      fprintf(f, "1962   1   1\n");
    for (i = 0; i < nd; i++)
      fprintf(f, "1962   1   1  %5d   0.0  0.0  0.0  0.0   0.000100   0.000200  0 0 0 0\n",
              37665 + i);
    return fclose(f);
  }
  if (k->kind == STARS) {
    FILE *f;
    snprintf(src, sizeof src, "%s/%s", EPHE, k->file);
    if ((b = slurp(src, &n)) == NULL)
      return -1;
    unlink(dst);
    if ((f = fopen(dst, "w")) == NULL) {
      free(b);
      return -1;
    }
    fputs("a line without a comma\n", f);
    fwrite(b, 1, (size_t) n, f);
    free(b);
    return fclose(f);
  }
  snprintf(src, sizeof src, "%s/%s", EPHE, k->file);
  if ((b = slurp(src, &n)) == NULL)
    return -1;
  if (k->kind == JPL) {
    /* ipt[] after the 252-byte title, 2400 bytes of names, ss[3], ncon,
     * au and emrat; body i's (offset, ncf, na) at ipt[3 i] */
    int32 *ipt = (int32 *) (b + 252 + 2400 + 24 + 4 + 16);
    int ne = ipt[2 * 3 + 1], nm = ipt[9 * 3 + 1], g = ne, r = nm, t;
    if (ipt[2 * 3 + 2] != 2 || ipt[9 * 3 + 2] != 8 || ne < 1 || nm < 1)
      goto out;
    while (r != 0)
      t = g % r, g = r, r = t;	/* gcd */
    /* the Earth's (ne coefficients) up, the Moon's (nm) down, by the same
     * number of coefficients: the length the reader checks is kept */
    ipt[2 * 3 + 2] += 100 * nm / g;
    ipt[9 * 3 + 2] -= 100 * ne / g;
    rc = spill(dst, b, n);
    goto out;
  }
  if (parse(b, n, k->kind == SAT ? 4 : 3, k->kind == SAT, &h) != 0)
    goto out;
  switch (k->patch) {
  case 1: b[h.ipl] = 500 & 0xff; b[h.ipl + 1] = 500 >> 8; recrc(b, &h); break;
  case 2: b[h.ipl] = 1; b[h.ipl + 1] = 0; recrc(b, &h); break;
  case 3: b[h.ipl] = 65000 & 0xff; b[h.ipl + 1] = 65000 >> 8; recrc(b, &h); break;
  case 4: case 10: body(b, &h)[5] = 60; break;
  case 5: memset(body(b, &h) + 10 + 16, 0, 8); break;
  case 6: put32(body(b, &h), 0x7fffff00UL); break;
  case 7: {
    double t;
    memcpy(&t, body(b, &h) + 10, 8);
    t += 1000;
    memcpy(body(b, &h) + 10, &t, 8);
    break;
  }
  case 8: b[h.ipl] = 3; b[h.ipl + 1] = 0; recrc(b, &h); break;
  case 9: {
    long i;	/* the start of the element line, the fourth */
    int nl = 0;
    for (i = 0; i < h.ipl && nl < 3; i++)
      if (b[i] == '\n')
        nl++;
    memset(b + i, '1', 30);
    recrc(b, &h);
    break;
  }
  }
  if (h.nplan < 1 || (k->kind == SWISS && b[h.ipl + 0] == 2 && k->patch != 0 && k->patch < 4))
    goto out;
  rc = spill(dst, b, n);
out:
  free(b);
  return rc;
}

/* in the child: 0 when the case behaves */
static int run(const char *dir, const struct kase *k)
{
  struct ans a, b, f;
  double J1700 = 2341972.5;
  switch (k->kind) {
  case SWISS: case MOONFILE: case SAT: {
    /* the other kinds' files open first, so a write into their slots
     * shows in their next segment */
    static const int warm[] = {SE_MERCURY, SE_MOON, 9501};
    int tgt = k->kind == SWISS ? SE_MERCURY : k->kind == MOONFILE ? SE_MOON : 9501;
    int j, nbad = 0;
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, dir);
    for (j = 0; j < 3; j++)
      if (warm[j] != tgt)
        calc_c(c, J2000, warm[j], SEFLG_SPEED);
    a = calc_c(c, k->kind == SAT ? J2000 : J1700, tgt, SEFLG_SPEED);
    if (a.r >= 0 || *a.serr == '\0') {
      printf("    the crafted file was not refused: %d, %s\n", a.r, a.serr);
      swe_ctx_free(c);
      return 1;
    }
    for (j = 0; j < 4; j++) {
      int ipl = j < 3 ? warm[j] : SE_MARS;
      double jd = j < 3 ? J2000 + 40 : J2000;
      if (ipl == tgt)
        continue;
      b = calc_c(c, jd, ipl, SEFLG_SPEED);
      f = fresh(jd, ipl, SEFLG_SPEED);
      if (b.r != f.r || memcmp(b.x, f.x, sizeof b.x) != 0) {
        printf("    after it, body %d: %.9f (%d, %s) for %.9f\n", ipl, b.x[0], b.r, b.serr, f.x[0]);
        nbad++;
      }
    }
    swe_ctx_free(c);
    if (nbad == 0)
      printf("    %.100s\n", a.serr);
    return nbad != 0;
  }
  case JPL:
  {
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, dir);
    swe_set_jpl_file_r(c, "de200.eph");
    a = calc_c(c, J2000, SE_MARS, SEFLG_JPLEPH | SEFLG_SPEED);
    if (a.r >= 0 && (a.r & SEFLG_JPLEPH)) {
      printf("    answered from the crafted file: %.9f\n", a.x[0]);
      swe_ctx_free(c);
      return 1;
    }
    a.r = -1;	/* refused; the Swiss file answered, with the warning */
    b = calc_c(c, J2000, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED);
    f = fresh(J2000, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED);
    swe_ctx_free(c);
  }
    break;
  case EOPSHORT: case EOPLONG: {
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, dir);
    load_dpsi_deps(c);
    if (c->eop_dpsi_loaded != 1 || c->eop_tjd_beg != 37665 + 2400000.5) {
      printf("    the table: loaded %d from %.1f\n", c->eop_dpsi_loaded, c->eop_tjd_beg);
      swe_ctx_free(c);
      return 1;
    }
    swe_ctx_free(c);
    a.r = -1;	/* nothing to refuse: the line or the days are passed over */
    strcpy(a.serr, "-");
    {
      swe_ctx *d = swe_ctx_new();
      swe_set_ephe_path_r(d, dir);
      b = calc_c(d, J2000, SE_SUN, SEFLG_SPEED);
      swe_ctx_free(d);
    }
    f = fresh(J2000, SE_SUN, SEFLG_SPEED);
    break;
  }
  default: {
    char star[64] = "Aldebaran";
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, dir);
    a.r = swe_fixstar2_r(c, star, J2000, SEFLG_SPEED, a.x, a.serr);
    b = calc_c(c, J2000, SE_SUN, SEFLG_SPEED);
    swe_ctx_free(c);
    f = fresh(J2000, SE_SUN, SEFLG_SPEED);
    break;
  }
  }
  if (a.r >= 0 || *a.serr == '\0') {
    printf("    the crafted file was not refused: %d, %s\n", a.r, a.serr);
    return 1;
  }
  if (b.r != f.r || memcmp(b.x, f.x, sizeof b.x) != 0) {
    printf("    after it: %.9f (%d, %s) for %.9f\n", b.x[0], b.r, b.serr, f.x[0]);
    return 1;
  }
  printf("    %.100s\n", a.serr);
  return 0;
}

int main(int argc, char **argv)
{
  int i, nbad = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: hdrbound EPHEDIR\n");
    return 2;
  }
  if (realpath(argv[1], EPHE) == NULL) {
    perror(argv[1]);
    return 2;
  }
  printf("G103: file header values bounded by what they index; a refused file alone\n");
  for (i = 0; i < NCASE; i++) {
    const struct kase *k = &rgCase[i];
    char dir[] = "/tmp/hdrboundXXXXXX", sat[PATH_MAX + 40], esat[PATH_MAX + 40];
    int st = 0;
    pid_t pid;
    if (mkdtemp(dir) == NULL) {
      perror("mkdtemp");
      return 2;
    }
    snprintf(sat, sizeof sat, "%s/sat", dir);
    snprintf(esat, sizeof esat, "%s/sat", EPHE);
    if (link_all(EPHE, dir) != 0 || unlink(sat) != 0 || mkdir(sat, 0700) != 0
        || link_all(esat, sat) != 0 || craft(dir, k) != 0) {
      printf("  %s: set-up failed\n", k->name);
      unlink_all(dir);
      nbad++;
      continue;
    }
    fflush(stdout);
    if ((pid = fork()) == 0) {
      int r = run(dir, k);
      fflush(stdout);
      _exit(r);
    }
    waitpid(pid, &st, 0);
    unlink_all(dir);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
      printf("  %s: FAIL (%s)\n", k->name, WIFSIGNALED(st) ? "crashed" : "wrong answer or exit");
      nbad++;
    } else
      printf("  %s: refused, the rest unchanged\n", k->name);
  }
  printf(nbad ? "G103 FAIL: %d\n" : "G103 PASS\n", nbad);
  return nbad != 0;
}
