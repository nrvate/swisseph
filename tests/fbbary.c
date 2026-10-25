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
 * Two more of the same family. UPSTREAM-BUGS 149: swe_nod_aps() with
 * barycentric output on Moshier, asked for or fallen to, answered Moshier's
 * heliocentric points, its change of centre adding no barycentric Sun
 * there: Mars's osculating node at 2000 Jul 1.5 49.5626 deg for the files'
 * barycentric 49.6398, the Moon's mean node 0.37 deg off; only the
 * SE_NODBIT_OSCU_BAR ellipse beyond 6 AU, read barycentric, was refused.
 * UPSTREAM-BUGS 150: swe_calc_pctr() returned the caller's flags: with only
 * the JPL file missing and the fallback allowed, Mars from Jupiter 257
 * (SEFLG_JPLEPH) with the Swiss answer bit for bit, and with the Moon
 * files missing the Moon from Mars 258 (SEFLG_SWIEPH) with Moshier's Moon.
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
 *      swe_nod_aps() of (g)'s bodies barycentric there answered, the
 *      Moon's points within 1e-7 deg (mean) and 1e-4 deg (osculating, the
 *      ellipse of Moshier's Moon), the others' within 1e-12 deg;
 *  (f) one process, the save areas warm: Mars barycentric strict (refused
 *      for the files), then the fallback allowed at the same instant
 *      (refused as Moshier's, twice), Moshier asked with the flags of the
 *      substituted answer (18180: refused, nothing kept answers it), Mars
 *      heliocentric before and after a barycentric request answering
 *      alike, swe_nod_aps() of Mars barycentric refused twice with
 *      heliocentric calls around them answering alike, then strict again
 *      (the downgrade's message); and a process with the planet and Moon
 *      files of 1800-2400 alone, the fallback allowed: swe_nod_aps() of
 *      Mars barycentric answered at the instant, refused at 1500, where it
 *      falls to Moshier (it answered Moshier's heliocentric points plus the
 *      files' Sun of the call before), and at the instant again bit for bit;
 *  (g) swe_nod_aps() and swe_nod_aps_ut() barycentric, every method and
 *      body class: the Sun, the Moon, Mercury, Mars, Saturn, Pluto and the
 *      Earth, mean, osculating, SE_NODBIT_OSCU_BAR, SE_NODBIT_FOPOINT and
 *      method 0, SEFLG_TOPOCTR | SEFLG_BARYCTR (barycentric here), and
 *      Chiron in a directory with the asteroid files alone (Moshier asked
 *      in the complete directory too): the Moshier request refused,
 *      "barycentric Moshier positions are not supported.", the fallback
 *      alike, strict the downgrade's refusal; the complete directory
 *      answers;
 *  (h) unchanged: those calls heliocentric, geocentric and with
 *      SEFLG_HELCTR | SEFLG_BARYCTR (heliocentric here): the fallback
 *      answers as the Moshier request does, bit for bit, and SEFLG_HELCTR
 *      | SEFLG_BARYCTR as SEFLG_HELCTR; the Sun's geocentric points too,
 *      seen from the Sun on Moshier as on the files (UPSTREAM-BUGS 167).
 *      Not the UT calls, whose fallback takes the files' delta t;
 *  (i) swe_calc_pctr()'s return flag: the complete directory's the
 *      ephemeris asked; the JPL file alone missing, JPL asked, the
 *      fallback allowed: Mars from Jupiter (SEFLG_SPEED, SEFLG_TRUEPOS,
 *      SEFLG_BARYCTR), the Moon from Mars and Mars from the Moon
 *      SEFLG_SWIEPH, the Swiss request's return and answer bit for bit,
 *      strict the downgrade's refusal; the Moon files alone missing,
 *      Swiss, the fallback allowed: the Moon from Mars and Mars from the
 *      Moon SEFLG_MOSEPH within 1e-6 deg of the complete directory's, Mars
 *      from Jupiter its answer bit for bit, strict bug 120's refusal of
 *      the two and the complete directory's answer for the rest.
 * Measured: 1080 checks, none fail; (e)'s Moon 5.2e-7 deg off the files',
 * swe_nod_aps()'s Moon points 6.4e-9 deg (mean) and 2.1e-5 deg
 * (osculating), swe_calc_pctr()'s Moon 2.1e-7 deg.
 * Before bug 139's fix 72 of the first 368 failed: the 60 fallback calls
 * of (a) (answered, 18180 for swe_calc(), or swe_calc_pctr() without
 * SEFLG_BARYCTR refused for the barycentric Sun's file where Moshier's own
 * refusal is due) and 12 of (f). Before bug 149's, 162: 152 of (g) (each
 * Moshier and fallback call answered but the Saturn ellipse's) and 10 of
 * (f). Before bug 150's, 24 of (i): with the JPL file missing 257 and its
 * kin, the Moshier Moon 258. The upstream build (SWE_UPSTREAM_COMPAT)
 * fails 296: those 258, the 8 Moshier requests of swe_calc_orbel() in (a)
 * (entry 135 restored), entry 139's 4 fallen barycentric ellipses of
 * Saturn in (g) and their 12 heliocentric and geocentric neighbours in
 * (h), entry 120's Moon twice in (e) and its 4 strict answers in (i), the
 * Sun's and the Earth's osculating apsides in (e) 3.9e-10 deg off with
 * the Moon files missing (entry 108's mass ratio restored), and 6 of
 * Mars from Jupiter in (i) at 1900, 2.1e-11 deg off there (compat's
 * obliquity keyed to the SE_ECL_NUT call at tjd + delta t, whose tidal
 * term the missing Moon file changes). UPSTREAM-BUGS 167 adds the Sun's 8
 * geocentric rows of (h): they fail before its fix, and in the upstream
 * build, which fails 296 with them, 288 without, on the tests' compat
 * objects.
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

enum { CALC, CALC_UT, PCTR, NODAPS, NODAPS_UT, ORBEL, ORBEL_UT, STAR2, STAR };
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
/* (g): swe_nod_aps() with barycentric output, every method and body class
 * (UPSTREAM-BUGS 149); ctr is the method. SEFLG_TOPOCTR with
 * SEFLG_BARYCTR is the barycentric output here (the function reads the
 * flags so). Saturn's SE_NODBIT_OSCU_BAR ellipse is barycentric itself. */
static const struct cs G[] = {
  {"nod_aps Sun mean", NODAPS, SE_SUN, SE_NODBIT_MEAN, BS},
  {"nod_aps Sun osculating", NODAPS, SE_SUN, SE_NODBIT_OSCU, BS},
  {"nod_aps Moon mean", NODAPS, SE_MOON, SE_NODBIT_MEAN, BS},
  {"nod_aps Moon osculating", NODAPS, SE_MOON, SE_NODBIT_OSCU, BS},
  {"nod_aps Moon OSCU_BAR", NODAPS, SE_MOON, SE_NODBIT_OSCU_BAR, BS},
  {"nod_aps Mercury mean, FOPOINT", NODAPS, SE_MERCURY, SE_NODBIT_MEAN | SE_NODBIT_FOPOINT, BS},
  {"nod_aps Mars, method 0", NODAPS, SE_MARS, 0, BS},
  {"nod_aps Mars mean", NODAPS, SE_MARS, SE_NODBIT_MEAN, BS},
  {"nod_aps Mars osculating", NODAPS, SE_MARS, SE_NODBIT_OSCU, BS},
  {"nod_aps Mars OSCU_BAR", NODAPS, SE_MARS, SE_NODBIT_OSCU_BAR, BS},
  {"nod_aps Mars osc, FOPOINT", NODAPS, SE_MARS, SE_NODBIT_OSCU | SE_NODBIT_FOPOINT, BS},
  {"nod_aps Mars osc, topo+bary", NODAPS, SE_MARS, SE_NODBIT_OSCU, BS | SEFLG_TOPOCTR},
  {"nod_aps_ut Mars osculating", NODAPS_UT, SE_MARS, SE_NODBIT_OSCU, BS},
  {"nod_aps_ut Moon mean", NODAPS_UT, SE_MOON, SE_NODBIT_MEAN, BS},
  {"nod_aps Saturn mean", NODAPS, SE_SATURN, SE_NODBIT_MEAN, BS},
  {"nod_aps Saturn osculating", NODAPS, SE_SATURN, SE_NODBIT_OSCU, BS},
  {"nod_aps Saturn OSCU_BAR, FOPOINT", NODAPS, SE_SATURN, SE_NODBIT_OSCU_BAR | SE_NODBIT_FOPOINT, BS},
  {"nod_aps Pluto, method 0", NODAPS, SE_PLUTO, 0, BS},
  {"nod_aps Earth osculating", NODAPS, SE_EARTH, SE_NODBIT_OSCU, BS},
};
/* (g): the asteroid class, in a directory with the asteroid files alone */
static const struct cs GA = {"nod_aps Chiron osculating", NODAPS, SE_CHIRON, SE_NODBIT_OSCU, BS};
/* (h): the centres Moshier serves, unchanged: G's calls heliocentric,
 * geocentric, and SEFLG_HELCTR | SEFLG_BARYCTR, which this function reads
 * as heliocentric */
static const int32 HCTR[3] = {SEFLG_HELCTR, 0, SEFLG_HELCTR | SEFLG_BARYCTR};
/* (i): swe_calc_pctr()'s return flag (UPSTREAM-BUGS 150) */
static const struct cs P[] = {
  {"pctr Mars from Jupiter", PCTR, SE_MARS, SE_JUPITER, SEFLG_SPEED},
  {"pctr Mars from Jupiter, TRUEPOS", PCTR, SE_MARS, SE_JUPITER, SEFLG_TRUEPOS},
  {"pctr Mars from Jupiter, BARYCTR", PCTR, SE_MARS, SE_JUPITER, BS},
  {"pctr Moon from Mars", PCTR, SE_MOON, SE_MARS, SEFLG_SPEED},
  {"pctr Mars from Moon", PCTR, SE_MARS, SE_MOON, SEFLG_SPEED},
};
#define NG ((int) (sizeof G / sizeof G[0]))
#define NP ((int) (sizeof P / sizeof P[0]))
#define NA ((int) (sizeof A / sizeof A[0]))
#define NF ((int) (sizeof F / sizeof F[0]))
#define NN ((int) (sizeof N / sizeof N[0]))
#define NM ((int) (sizeof M / sizeof M[0]))

static const double JD[2] = {2451727.0, 2415020.25};
static const int32 EPH[2] = {SEFLG_SWIEPH, SEFLG_JPLEPH};
static char DIR_[] = "/tmp/fbbary-XXXXXX";	/* sefstars.txt, seorbel.txt */
static char DIRM[] = "/tmp/fbbarym-XXXXXX";	/* all but the Moon files */
static char DIRA[] = "/tmp/fbbarya-XXXXXX";	/* DIR_'s and the asteroid files */
static char DIRJ[] = "/tmp/fbbaryj-XXXXXX";	/* all but the JPL file */
static char DIR18[] = "/tmp/fbbary18-XXXXXX";	/* the planet and Moon files of 1800-2400 */
static char EPHE[PATH_MAX];
static int nbad, nrow;

struct ans { int32 r; double x[24]; char serr[AS_MAXCH]; int ran; };

static void ask(const struct cs *c, int32 fl, double jd, struct ans *a)
{
  static const double el[6] = {100, 1.2, 0.1, 30, 40, 5};
  char star[64] = "Aldebaran";
  memset(a->x, 0, sizeof a->x);
  *a->serr = '\0';
  switch (c->kind) {
    case CALC: a->r = swe_calc(jd, c->ipl, fl, a->x, a->serr); break;
    case CALC_UT: a->r = swe_calc_ut(jd, c->ipl, fl, a->x, a->serr); break;
    case PCTR: a->r = swe_calc_pctr(jd, c->ipl, c->ctr, fl, a->x, a->serr); break;
    case NODAPS: a->r = swe_nod_aps(jd, c->ipl, fl, c->ctr, a->x, a->x + 6, a->x + 12, a->x + 18, a->serr); break;
    case NODAPS_UT: a->r = swe_nod_aps_ut(jd, c->ipl, fl, c->ctr, a->x, a->x + 6, a->x + 12, a->x + 18, a->serr); break;
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
    if (c->fl & SEFLG_TOPOCTR)
      swe_set_topo(8.55, 47.37, 400);
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

static int refused_bary(const struct ans *a)
{
  return a->ran && a->r < 0 && strcmp(a->serr, BARYMSG) == 0;
}

/* the four points' longitudes and latitudes, the larger difference, deg */
static double nodaps_off(const struct ans *a, const struct ans *b)
{
  double d = 0, e;
  int k;
  for (k = 0; k < 4; k++) {
    e = fabs(swe_difdeg2n(a->x[6 * k], b->x[6 * k])) + fabs(a->x[6 * k + 1] - b->x[6 * k + 1]);
    if (e > d)
      d = e;
  }
  return d;
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
  /* swe_nod_aps() alike (bug 149): barycentric refused twice, heliocentric
   * answered before and after alike */
  {
    static const struct cs nb = {"nod_aps Mars", NODAPS, SE_MARS, SE_NODBIT_OSCU, BS};
    static const struct cs nh = {"nod_aps Mars, hel.", NODAPS, SE_MARS, SE_NODBIT_OSCU,
                                 SEFLG_HELCTR | SEFLG_SPEED};
    ask(&nh, (nh.fl & ~EPHMASK) | eph, jd, &h1);
    for (k = 0; k < 2; k++) {
      ask(&nb, (nb.fl & ~EPHMASK) | eph, jd, &a);
      bad += !(a.r < 0 && strcmp(a.serr, BARYMSG) == 0);
    }
    ask(&nh, (nh.fl & ~EPHMASK) | eph, jd, &h2);
    bad += !(h1.r == 0 && h1.r == h2.r && memcmp(h1.x, h2.x, sizeof h1.x) == 0);
  }
  swe_set_ephe_fallback(0);
  ask(&bary, fb, jd, &a);
  bad += !strict_msg(&a);
  swe_close();
  return bad;
}
#define NWARM 10

/* (f): one process, the files of 1800-2400 alone, the fallback allowed:
 * swe_nod_aps() of Mars barycentric answered at jd, refused at 1500 (it
 * falls to Moshier there; it was answered with Moshier's heliocentric
 * points plus the files' Sun of the call before), and at jd again the
 * first answer bit for bit (bug 149); returns the failures */
static int warm18(double jd)
{
  static const struct cs nb = {"nod_aps Mars", NODAPS, SE_MARS, SE_NODBIT_OSCU, BS};
  struct ans a, b, c;
  int bad = 0;
  setup(DIR18, 1, SEFLG_SWIEPH);
  ask(&nb, nb.fl | SEFLG_SWIEPH, jd, &a);
  bad += !(a.r == 0);
  ask(&nb, nb.fl | SEFLG_SWIEPH, 2268923.5, &b);
  bad += !(b.r < 0 && strcmp(b.serr, BARYMSG) == 0);
  ask(&nb, nb.fl | SEFLG_SWIEPH, jd, &c);
  bad += !(c.r == 0 && memcmp(a.x, c.x, sizeof a.x) == 0);
  swe_close();
  return bad;
}
#define NWARM18 3

static int warm_child(int32 eph, double jd, int at18)
{
  int st;
  pid_t pid;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return 99;
  if (pid == 0)
    _exit(at18 ? warm18(jd) : warm(eph, jd));
  if (waitpid(pid, &st, 0) != pid || !WIFEXITED(st))
    return 99;
  return WEXITSTATUS(st);
}

/* which files a directory links: */
enum { STARS_ONLY, NO_MOON, ASTEROIDS, NO_JPL, FILES_18 };
static int build(char *dir, int which)
{
  DIR *d;
  struct dirent *e;
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  int stars, take;
  if (mkdtemp(dir) == NULL || (d = opendir(EPHE)) == NULL)
    return -1;
  while ((e = readdir(d)) != NULL) {
    if (e->d_name[0] == '.')
      continue;
    stars = strcmp(e->d_name, "sefstars.txt") == 0 || strcmp(e->d_name, "seorbel.txt") == 0;
    switch (which) {
      case NO_MOON: take = strncmp(e->d_name, "semo", 4) != 0; break;
      case ASTEROIDS: take = stars || strncmp(e->d_name, "seas_", 5) == 0; break;
      case NO_JPL: take = strstr(e->d_name, ".eph") == NULL; break;
      case FILES_18: take = stars || strcmp(e->d_name, "sepl_18.se1") == 0
                            || strcmp(e->d_name, "semo_18.se1") == 0; break;
      default: take = stars; break;
    }
    if (!take)
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
  int id, ie, k, j, fb, nwarm = 0;
  double wmoon = 0, wnodm = 0, wnodo = 0, wpctr = 0;
  if (argc != 2 || realpath(argv[1], EPHE) == NULL) {
    fprintf(stderr, "usage: fbbary EPHEDIR\n");
    return 2;
  }
  if (build(DIR_, STARS_ONLY) != 0 || build(DIRM, NO_MOON) != 0 || build(DIRA, ASTEROIDS) != 0
      || build(DIRJ, NO_JPL) != 0 || build(DIR18, FILES_18) != 0) {
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
        /* swe_nod_aps() there (bug 149): the Moon's points from Moshier's
         * Moon on the files' Earth, a barycentric place, answered; the
         * others' Sun and Earth are the files' */
        for (k = 0; k < NG; k++) {
          const struct cs *c = &G[k];
          double tol = c->ipl != SE_MOON ? 1e-12 : (c->ctr & SE_NODBIT_MEAN) ? 1e-7 : 1e-4;
          /* (the UT calls' delta t takes the Moon file's tidal term, which
           * is missing here: not this check's) */
          if ((c->fl & SEFLG_TOPOCTR) || c->kind == NODAPS_UT)
            continue;
          child(DIRM, 1, eph, c, jd, &a);
          child(EPHE, 0, eph, c, jd, &full);
          d = nodaps_off(&a, &full);
          if (a.ran && a.r == 0 && c->ipl == SE_MOON && d > ((c->ctr & SE_NODBIT_MEAN) ? wnodm : wnodo)) {
            if (c->ctr & SE_NODBIT_MEAN)
              wnodm = d;
            else
              wnodo = d;
          }
          check(a.ran && a.r == full.r && (full.r < 0 ? strcmp(a.serr, full.serr) == 0 : d <= tol),
                "e", c->sz, eph, jd, &a, "not the complete directory's answer within its tolerance");
        }
      }
      /* (g) swe_nod_aps() barycentric, bug 149 */
      for (k = 0; k < NG; k++) {
        struct ans m, f, s, full;
        const struct cs *c = &G[k];
        child(DIR_, 0, SEFLG_MOSEPH, c, jd, &m);
        child(DIR_, 1, eph, c, jd, &f);
        child(DIR_, 0, eph, c, jd, &s);
        child(EPHE, 0, eph, c, jd, &full);
        check(refused_bary(&m), "g", c->sz, eph, jd, &m, "the Moshier request not refused as barycentric");
        check(refused_bary(&f), "g", c->sz, eph, jd, &f, "not refused as the Moshier request is");
        check(strict_msg(&s), "g", c->sz, eph, jd, &s, "strict: not the downgrade's refusal");
        check(full.ran && full.r == 0, "g", c->sz, eph, jd, &full, "the complete directory does not answer");
      }
      {
        struct ans m, me, f, s, full;
        child(DIRA, 0, SEFLG_MOSEPH, &GA, jd, &m);
        child(EPHE, 0, SEFLG_MOSEPH, &GA, jd, &me);
        child(DIRA, 1, eph, &GA, jd, &f);
        child(DIRA, 0, eph, &GA, jd, &s);
        child(EPHE, 0, eph, &GA, jd, &full);
        check(refused_bary(&m) && refused_bary(&me), "g", GA.sz, eph, jd, refused_bary(&m) ? &me : &m,
              "the Moshier request not refused as barycentric");
        check(refused_bary(&f), "g", GA.sz, eph, jd, &f, "not refused as the Moshier request is");
        check(strict_msg(&s), "g", GA.sz, eph, jd, &s, "strict: not the downgrade's refusal");
        check(full.ran && full.r == 0, "g", GA.sz, eph, jd, &full, "the complete directory does not answer");
      }
      /* (h) the centres Moshier serves, as before: the fallback answers as
       * the Moshier request does, and SEFLG_HELCTR | SEFLG_BARYCTR as
       * SEFLG_HELCTR; the Sun's geocentric points too, seen from the Sun
       * on Moshier now (bug 167). Not the UT calls, whose fallback takes
       * the files' delta t. */
      for (k = 0; k < NG; k++) {
        struct ans m[3], f;
        if (G[k].kind == NODAPS_UT)
          continue;
        for (j = 0; j < 3; j++) {
          struct cs c = G[k];
          c.fl = (c.fl & ~(SEFLG_BARYCTR | SEFLG_TOPOCTR)) | HCTR[j];
          child(DIR_, 0, SEFLG_MOSEPH, &c, jd, &m[j]);
          child(DIR_, 1, eph, &c, jd, &f);
          check(m[j].ran && f.ran && f.r == m[j].r
                && (f.r < 0 ? strcmp(f.serr, m[j].serr) == 0 : memcmp(f.x, m[j].x, sizeof f.x) == 0),
                "h", c.sz, eph, jd, &f, j == 0 ? "heliocentric: not the Moshier request's answer"
                : j == 1 ? "geocentric: not the Moshier request's answer"
                : "HELCTR | BARYCTR: not the Moshier request's answer");
        }
        check(same(&m[2], &m[0]), "h", G[k].sz, eph, jd, &m[2],
              "Moshier, HELCTR | BARYCTR: not its heliocentric answer");
      }
      /* (i) swe_calc_pctr()'s return flag, bug 150 */
      for (k = 0; k < NP; k++) {
        const struct cs *c = &P[k];
        struct ans f, s, full, sw;
        int moon = c->ipl == SE_MOON || c->ctr == SE_MOON;
        child(EPHE, 0, eph, c, jd, &full);
        check(full.ran && full.r >= 0 && (full.r & EPHMASK) == eph, "i", c->sz, eph, jd, &full,
              "the complete directory: not the ephemeris asked");
        if (eph == SEFLG_JPLEPH) {
          /* the JPL file alone missing: Swiss answers */
          child(DIRJ, 1, eph, c, jd, &f);
          child(DIRJ, 1, SEFLG_SWIEPH, c, jd, &sw);
          child(DIRJ, 0, eph, c, jd, &s);
          check(f.ran && f.r >= 0 && (f.r & EPHMASK) == SEFLG_SWIEPH, "i", c->sz, eph, jd, &f,
                "JPL file missing: not flagged SEFLG_SWIEPH");
          check(f.ran && sw.ran && f.r == sw.r && memcmp(f.x, sw.x, sizeof f.x) == 0, "i", c->sz, eph, jd,
                &f, "JPL file missing: not the Swiss request's answer and flag");
          check(s.ran && s.r < 0 && strstr(s.serr, "JPL ephemeris is not available for jd") != NULL
                && strstr(s.serr, "falling back to Swiss") != NULL, "i", c->sz, eph, jd, &s,
                "JPL file missing, strict: not the downgrade's refusal");
        } else {
          /* the Moon files alone missing: a Moon from Moshier answers so */
          double d;
          child(DIRM, 1, eph, c, jd, &f);
          child(DIRM, 0, eph, c, jd, &s);
          d = fabs(swe_difdeg2n(f.x[0], full.x[0])) + fabs(f.x[1] - full.x[1]);
          if (moon && f.ran && f.r >= 0 && d > wpctr)
            wpctr = d;
          check(f.ran && f.r >= 0 && (f.r & EPHMASK) == (moon ? SEFLG_MOSEPH : SEFLG_SWIEPH)
                && (moon ? d < 1e-6 : same(&f, &full)), "i", c->sz, eph, jd, &f, moon
                ? "Moon files missing: not flagged SEFLG_MOSEPH, or not within 1e-6 deg"
                : "Moon files missing: not the complete directory's answer and flag");
          check(moon ? s.ran && s.r < 0 && strstr(s.serr, "Moon file is not available") != NULL
                : same(&s, &full), "i", c->sz, eph, jd, &s, moon
                ? "Moon files missing, strict: not bug 120's refusal"
                : "Moon files missing, strict: not the complete directory's answer");
        }
      }
      k = warm_child(eph, jd, 0);
      nwarm += NWARM;
      nrow += NWARM;
      if (k != 0) {
        nbad += k;
        printf("    FAIL (f) warm Mars %s jd %.2f: %d of %d\n",
               eph == SEFLG_JPLEPH ? "JPL" : "Swiss", jd, k, NWARM);
      }
      if (eph == SEFLG_SWIEPH) {
        k = warm_child(eph, jd, 1);
        nwarm += NWARM18;
        nrow += NWARM18;
        if (k != 0) {
          nbad += k;
          printf("    FAIL (f) warm nod_aps Mars, files of 1800-2400, jd %.2f: %d of %d\n", jd, k, NWARM18);
        }
      }
    }
  unbuild(DIR_);
  unbuild(DIRM);
  unbuild(DIRA);
  unbuild(DIRJ);
  unbuild(DIR18);
  printf("  rows %d (%d warm), failed %d; the Moon from Moshier on the files' Earth %.2e deg off the files'\n",
         nrow, nwarm, nbad, wmoon);
  printf("  there, swe_nod_aps()'s barycentric Moon points %.2e deg (mean), %.2e (osculating); swe_calc_pctr() %.2e\n",
         wnodm, wnodo, wpctr);
  printf(nbad ? "G133 FAIL: %d\n" : "G133 PASS\n", nbad);
  return nbad ? 1 : 0;
}
