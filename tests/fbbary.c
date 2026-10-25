/* fbbary.c -- G133: with the fallback allowed, a Swiss or JPL barycentric
 * request whose files are missing is refused as the Moshier request it
 * falls to is, and nothing else changes.
 *
 * The defect (UPSTREAM-BUGS 139): swecalc() refuses every barycentric
 * Moshier request ("barycentric Moshier positions are not supported."),
 * Moshier serving heliocentric places only, the Sun its origin. A Swiss or
 * JPL request passed that test, fell to Moshier for want of its files
 * (upstream's silent substitution, swe_set_ephe_fallback(1) here) and was
 * answered with Moshier's heliocentric place under SEFLG_BARYCTR |
 * SEFLG_MOSEPH (18180 with SEFLG_SPEED): Mars at 2000 Jul 1.5 0.24 deg and
 * 7.2e-3 AU off the files' barycentric Mars and equal to Moshier's
 * heliocentric one to the last bit, the Moon 0.37 deg, the Earth 0.37,
 * Cupido 5.1e-3, swe_calc_orbel() of an orbit with a = 1.2 AU 0.18 deg.
 * swe_calc_pctr(), which reads its two bodies barycentric, answered a
 * request with SEFLG_BARYCTR from the two heliocentric places (1.6e-4 deg
 * off) under SEFLG_SWIEPH, and swe_nod_aps() with SE_NODBIT_OSCU_BAR from
 * the heliocentric ellipse (Uranus's node 0.025 deg off); both refuse a
 * Moshier request.
 *
 * The check, each case in a forked, pristine process, a directory in /tmp
 * holding links to sefstars.txt and seorbel.txt alone, two instants (2000
 * Jul 1.5, 1900 Jan 0.75), Swiss and JPL (de200.eph named, missing too):
 *  (a) fallback allowed: Mars (swe_calc(), swe_calc_ut(), SEFLG_SPEED3,
 *      J2000 equatorial XYZ), Jupiter, Pluto, the Earth, the Moon,
 *      Cupido, swe_calc_orbel() and swe_calc_orbel_ut() of a = 1.2 AU
 *      about the Sun, all barycentric; swe_calc_pctr() of Mars from Jupiter
 *      with and without SEFLG_BARYCTR; swe_nod_aps() of Uranus with
 *      SE_NODBIT_OSCU_BAR, geocentric and barycentric: refused, -1, with
 *      the return and message of the same call asking for Moshier, itself
 *      -1 and "barycentric Moshier positions are not supported.";
 *  (b) strict: the same calls refused for the missing files, -1 and the
 *      downgrade's message ("... is not available for jd ...; falling back
 *      to Moshier ..."), as before;
 *  (c) the complete directory: every call of (a) answers, swe_calc() and
 *      swe_calc_pctr() with the ephemeris asked for;
 *  (d) unchanged, both settings: the barycentric Sun and Ceres and Chiron
 *      refused for their files ("not found"), as the fallback never
 *      reached Moshier for them; the mean and true nodes and the mean,
 *      osculating and interpolated apsides, barycentric, and Aldebaran
 *      barycentric (swe_fixstar2(), swe_fixstar()), which read no file,
 *      bit for bit and flag for flag the complete directory's; with the
 *      fallback, Mars heliocentric and geocentric answered from Moshier,
 *      bit for bit the Moshier request's;
 *  (e) the Moon files alone missing, fallback allowed: the barycentric
 *      Moon, Moshier's geocentric Moon on the files' Earth, a barycentric
 *      place, still answered (SEFLG_MOSEPH, bug 120's flag) within 1e-6
 *      deg of the complete directory's, and barycentric Mars bit for bit;
 *  (f) one process, the save areas warm: Mars barycentric strict (refused
 *      for the files), then the fallback allowed at the same instant
 *      (refused as Moshier's, twice), Moshier asked with the flags of the
 *      substituted answer (18180: refused, nothing kept answers it), Mars
 *      heliocentric before and after a barycentric request answering
 *      alike, then strict again (the downgrade's message).
 * Measured: 368 checks, none fail; (e)'s Moon 5.2e-7 deg off the files'.
 * The build before the fix fails 72: the 60 fallback calls of (a)
 * (answered, 18180 for swe_calc(), or swe_calc_pctr() without
 * SEFLG_BARYCTR refused for the barycentric Sun's file where Moshier's own
 * refusal is due) and 12 of (f). The upstream build (SWE_UPSTREAM_COMPAT)
 * fails 82: those 72, the 8 Moshier requests of swe_calc_orbel() in (a)
 * (answered, entry 135 restored) and (e)'s Moon twice (SEFLG_SWIEPH,
 * entry 120 restored).
 * Run as
 *   fbbary EPHEDIR
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

#define EPHMASK (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)
#define BARYMSG "barycentric Moshier positions are not supported."

enum { CALC, CALC_UT, PCTR, NODAPS, ORBEL, ORBEL_UT, STAR2, STAR };
struct cs { const char *sz; int kind, ipl, ctr; int32 fl; };
#define BS (SEFLG_BARYCTR | SEFLG_SPEED)
/* (a)-(c): fall to Moshier, which refuses them */
static const struct cs A[] = {
  {"Mars", CALC, SE_MARS, 0, BS},
  {"Mars, swe_calc_ut()", CALC_UT, SE_MARS, 0, BS},
  {"Mars, SEFLG_SPEED3", CALC, SE_MARS, 0, SEFLG_BARYCTR | SEFLG_SPEED3},
  {"Mars, J2000 equatorial XYZ", CALC, SE_MARS, 0,
   BS | SEFLG_XYZ | SEFLG_J2000 | SEFLG_EQUATORIAL | SEFLG_TRUEPOS},
  {"Jupiter", CALC, SE_JUPITER, 0, BS},
  {"Pluto", CALC, SE_PLUTO, 0, BS},
  {"Earth", CALC, SE_EARTH, 0, BS},
  {"Moon", CALC, SE_MOON, 0, BS},
  {"Cupido", CALC, SE_CUPIDO, 0, BS},
  {"swe_calc_orbel()", ORBEL, 0, SE_SUN, BS},
  {"swe_calc_orbel_ut()", ORBEL_UT, 0, SE_SUN, BS},
  {"pctr Mars from Jupiter", PCTR, SE_MARS, SE_JUPITER, SEFLG_SPEED},
  {"pctr Mars from Jupiter, bary", PCTR, SE_MARS, SE_JUPITER, BS},
  {"nod_aps Uranus OSCU_BAR", NODAPS, SE_URANUS, SE_NODBIT_OSCU_BAR, SEFLG_SPEED},
  {"nod_aps Uranus OSCU_BAR, bary", NODAPS, SE_URANUS, SE_NODBIT_OSCU_BAR, BS},
};
/* (d): refused for their files whatever the setting */
static const struct cs F[] = {
  {"Sun", CALC, SE_SUN, 0, BS},
  {"Ceres", CALC, SE_CERES, 0, BS},
  {"Chiron", CALC, SE_CHIRON, 0, BS},
};
/* (d): read no file: the complete directory's answer */
static const struct cs N[] = {
  {"mean node", CALC, SE_MEAN_NODE, 0, BS},
  {"true node", CALC, SE_TRUE_NODE, 0, BS},
  {"mean apogee", CALC, SE_MEAN_APOG, 0, BS},
  {"osculating apogee", CALC, SE_OSCU_APOG, 0, BS},
  {"interpolated apogee", CALC, SE_INTP_APOG, 0, BS},
  {"interpolated perigee", CALC, SE_INTP_PERG, 0, BS},
  {"Aldebaran, swe_fixstar2()", STAR2, 0, 0, BS},
  {"Aldebaran, swe_fixstar()", STAR, 0, 0, BS},
};
/* (d): what Moshier serves, with the fallback */
static const struct cs M[] = {
  {"Mars, heliocentric", CALC, SE_MARS, 0, SEFLG_HELCTR | SEFLG_SPEED},
  {"Mars, geocentric", CALC, SE_MARS, 0, SEFLG_SPEED},
};
#define NA ((int) (sizeof A / sizeof A[0]))
#define NF ((int) (sizeof F / sizeof F[0]))
#define NN ((int) (sizeof N / sizeof N[0]))
#define NM ((int) (sizeof M / sizeof M[0]))

static const double JD[2] = {2451727.0, 2415020.25};
static const int32 EPH[2] = {SEFLG_SWIEPH, SEFLG_JPLEPH};
static char DIR_[] = "/tmp/fbbary-XXXXXX";	/* sefstars.txt, seorbel.txt */
static char DIRM[] = "/tmp/fbbarym-XXXXXX";	/* all but the Moon files */
static char EPHE[PATH_MAX];
static int nbad, nrow;

struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; int ran; };

static void ask(const struct cs *c, int32 fl, double jd, struct ans *a)
{
  static const double el[6] = {100, 1.2, 0.1, 30, 40, 5};
  double xd[6], xp[6], xa[6];
  char star[64] = "Aldebaran";
  memset(a->x, 0, sizeof a->x);
  *a->serr = '\0';
  switch (c->kind) {
    case CALC: a->r = swe_calc(jd, c->ipl, fl, a->x, a->serr); break;
    case CALC_UT: a->r = swe_calc_ut(jd, c->ipl, fl, a->x, a->serr); break;
    case PCTR: a->r = swe_calc_pctr(jd, c->ipl, c->ctr, fl, a->x, a->serr); break;
    case NODAPS: a->r = swe_nod_aps(jd, c->ipl, fl, c->ctr, a->x, xd, xp, xa, a->serr); break;
    case ORBEL: a->r = swe_calc_orbel(jd, el, c->ctr, SE_ORBEL_EQ_J2000, 0, fl, a->x, a->serr); break;
    case ORBEL_UT: a->r = swe_calc_orbel_ut(jd, el, c->ctr, SE_ORBEL_EQ_J2000, 0, fl, a->x, a->serr); break;
    case STAR2: a->r = swe_fixstar2(star, jd, fl, a->x, a->serr); break;
    default: a->r = swe_fixstar(star, jd, fl, a->x, a->serr); break;
  }
}

static void setup(const char *dir, int fb, int32 eph)
{
  swe_set_ephe_path(dir);
  swe_set_ephe_fallback(fb);
  if (eph == SEFLG_JPLEPH)
    swe_set_jpl_file("de200.eph");
}

/* one call in a forked, pristine process */
static void child(const char *dir, int fb, int32 eph, const struct cs *c, double jd, struct ans *out)
{
  int fd[2];
  ssize_t got = 0, r;
  pid_t pid;
  memset(out, 0, sizeof *out);
  if (pipe(fd) != 0)
    return;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return;
  if (pid == 0) {
    struct ans a;
    close(fd[0]);
    memset(&a, 0, sizeof a);
    setup(dir, fb, eph);
    ask(c, (c->fl & ~EPHMASK) | eph, jd, &a);
    a.ran = 1;
    _exit(write(fd[1], &a, sizeof a) == (ssize_t) sizeof a ? 0 : 3);
  }
  close(fd[1]);
  while (got < (ssize_t) sizeof *out
         && (r = read(fd[0], (char *) out + got, sizeof *out - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  if (got != (ssize_t) sizeof *out)
    out->ran = 0;
}

static void check(int ok, const char *part, const char *sz, int32 eph, double jd,
                  const struct ans *a, const char *why)
{
  char s[AS_MAXCH], *p;
  nrow++;
  if (ok)
    return;
  nbad++;
  if (nbad > 40)
    return;
  memcpy(s, a->serr, sizeof s);
  s[sizeof s - 1] = '\0';
  for (p = s; *p; p++)
    if (*p == '\n')
      *p = '|';
  printf("    FAIL (%s) %-30s %s jd %.2f: ret %d, %s; serr '%.70s'\n", part, sz,
         eph == SEFLG_JPLEPH ? "JPL" : "Swiss", jd, a->r, why, s);
}

static int same(const struct ans *a, const struct ans *b)
{
  return a->ran && b->ran && a->r == b->r && memcmp(a->x, b->x, sizeof a->x) == 0;
}

static int strict_msg(const struct ans *a)
{
  return a->ran && a->r < 0 && strstr(a->serr, "ephemeris is not available for jd") != NULL
         && strstr(a->serr, "falling back to Moshier") != NULL;
}

/* (f): one process, warm; returns the failures */
static int warm(int32 eph, double jd)
{
  static const struct cs bary = {"Mars", CALC, SE_MARS, 0, BS};
  static const struct cs hel = {"Mars, heliocentric", CALC, SE_MARS, 0, SEFLG_HELCTR | SEFLG_SPEED};
  struct ans a, h1, h2;
  int bad = 0, k;
  int32 fb = (bary.fl & ~EPHMASK) | eph, fh = (hel.fl & ~EPHMASK) | eph;
  setup(DIR_, 0, eph);
  ask(&bary, fb, jd, &a);
  bad += !strict_msg(&a);
  swe_set_ephe_fallback(1);
  for (k = 0; k < 2; k++) {
    ask(&bary, fb, jd, &a);
    bad += !(a.r < 0 && strcmp(a.serr, BARYMSG) == 0);
  }
  /* Moshier asked with the flags the substituted answer had (18180) */
  ask(&bary, (fb & ~EPHMASK) | SEFLG_MOSEPH | SEFLG_NOABERR | SEFLG_NOGDEFL, jd, &a);
  bad += !(a.r < 0 && strcmp(a.serr, BARYMSG) == 0);
  ask(&hel, fh, jd, &h1);
  ask(&bary, fb, jd, &a);
  bad += !(a.r < 0 && strcmp(a.serr, BARYMSG) == 0);
  ask(&hel, fh, jd, &h2);
  bad += !(h1.r == h2.r && (h1.r & EPHMASK) == SEFLG_MOSEPH
           && memcmp(h1.x, h2.x, sizeof h1.x) == 0);
  swe_set_ephe_fallback(0);
  ask(&bary, fb, jd, &a);
  bad += !strict_msg(&a);
  swe_close();
  return bad;
}

static int warm_child(int32 eph, double jd)
{
  int st;
  pid_t pid;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return 99;
  if (pid == 0)
    _exit(warm(eph, jd));
  if (waitpid(pid, &st, 0) != pid || !WIFEXITED(st))
    return 99;
  return WEXITSTATUS(st);
}

static int build(char *dir, int moon_only)
{
  DIR *d;
  struct dirent *e;
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  if (mkdtemp(dir) == NULL || (d = opendir(EPHE)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.')
      continue;
    if (moon_only ? strncmp(e->d_name, "semo", 4) == 0
                  : strcmp(e->d_name, "sefstars.txt") != 0 && strcmp(e->d_name, "seorbel.txt") != 0)
      continue;
    snprintf(src, sizeof src, "%s/%s", EPHE, e->d_name);
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
  int id, ie, k, fb, nwarm = 0;
  double wmoon = 0;
  if (argc != 2 || realpath(argv[1], EPHE) == NULL) {
    fprintf(stderr, "usage: fbbary EPHEDIR\n");
    return 2;
  }
  if (build(DIR_, 0) != 0 || build(DIRM, 1) != 0) {
    printf("G133 FAIL: cannot build the directories in /tmp\n");
    return 1;
  }
  printf("G133: a barycentric request fallen to Moshier is refused as Moshier's\n");
  for (id = 0; id < 2; id++)
    for (ie = 0; ie < 2; ie++) {
      double jd = JD[id];
      int32 eph = EPH[ie];
      for (k = 0; k < NA; k++) {
        struct ans m, f, s, full;
        const struct cs *c = &A[k];
        child(DIR_, 0, SEFLG_MOSEPH, c, jd, &m);
        child(DIR_, 1, eph, c, jd, &f);
        child(DIR_, 0, eph, c, jd, &s);
        child(EPHE, 0, eph, c, jd, &full);
        check(m.ran && m.r < 0 && strcmp(m.serr, BARYMSG) == 0, "a", c->sz, eph, jd, &m,
              "the Moshier request not refused as barycentric");
        check(f.ran && f.r == m.r && strcmp(f.serr, m.serr) == 0, "a", c->sz, eph, jd, &f,
              "not refused as the Moshier request is");
        check(strict_msg(&s), "b", c->sz, eph, jd, &s, "strict: not the downgrade's refusal");
        check(full.ran && full.r >= 0
              && (c->kind == NODAPS || c->kind == ORBEL || c->kind == ORBEL_UT
                  || (full.r & EPHMASK) == eph),
              "c", c->sz, eph, jd, &full, "the complete directory does not answer as asked");
      }
      for (fb = 0; fb < 2; fb++) {
        for (k = 0; k < NF; k++) {
          struct ans a;
          child(DIR_, fb, eph, &F[k], jd, &a);
          check(a.ran && a.r < 0 && strstr(a.serr, "not found") != NULL, "d", F[k].sz, eph, jd,
                &a, "not refused for its file");
        }
        for (k = 0; k < NN; k++) {
          struct ans a, full;
          child(DIR_, fb, eph, &N[k], jd, &a);
          child(EPHE, 0, eph, &N[k], jd, &full);
          check(same(&a, &full) && a.r >= 0, "d", N[k].sz, eph, jd, &a,
                "not the complete directory's answer");
        }
      }
      for (k = 0; k < NM; k++) {
        struct ans a, m;
        child(DIR_, 1, eph, &M[k], jd, &a);
        child(DIR_, 0, SEFLG_MOSEPH, &M[k], jd, &m);
        check(same(&a, &m) && (a.r & EPHMASK) == SEFLG_MOSEPH, "d", M[k].sz, eph, jd, &a,
              "not the Moshier request's answer");
      }
      if (eph == SEFLG_SWIEPH) {
        static const struct cs moon = {"Moon, Moon files missing", CALC, SE_MOON, 0, BS};
        static const struct cs mars = {"Mars, Moon files missing", CALC, SE_MARS, 0, BS};
        struct ans a, full;
        double d;
        child(DIRM, 1, eph, &moon, jd, &a);
        child(EPHE, 0, eph, &moon, jd, &full);
        d = fabs(swe_difdeg2n(a.x[0], full.x[0])) + fabs(a.x[1] - full.x[1]);
        if (a.ran && a.r >= 0 && d > wmoon)
          wmoon = d;
        check(a.ran && a.r >= 0 && (a.r & EPHMASK) == SEFLG_MOSEPH && (a.r & SEFLG_BARYCTR)
              && d < 1e-6, "e", moon.sz, eph, jd, &a,
              "not answered, or not the files' barycentric Moon within 1e-6 deg");
        child(DIRM, 1, eph, &mars, jd, &a);
        child(EPHE, 0, eph, &mars, jd, &full);
        check(same(&a, &full), "e", mars.sz, eph, jd, &a, "not the complete directory's");
      }
      k = warm_child(eph, jd);
      nwarm += 7;
      nrow += 7;
      if (k != 0) {
        nbad += k;
        printf("    FAIL (f) warm Mars %s jd %.2f: %d of 7\n",
               eph == SEFLG_JPLEPH ? "JPL" : "Swiss", jd, k);
      }
    }
  unbuild(DIR_);
  unbuild(DIRM);
  printf("  rows %d (%d warm), failed %d; the Moon from Moshier on the files' Earth %.2e deg off the files'\n",
         nrow, nwarm, nbad, wmoon);
  printf(nbad ? "G133 FAIL: %d\n" : "G133 PASS\n", nbad);
  return nbad ? 1 : 0;
}
