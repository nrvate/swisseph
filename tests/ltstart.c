/* ltstart.c -- G171: within the light time after an ephemeris's start the
 * Moon (and an asteroid) answers whole from the ephemeris that answers the
 * instant it is seen at, as the fallback answers before the start, or is
 * refused under the strict setting; and SEFLG_SPEED answers where the
 * call without it does, its speed from reads on the instant's side.
 *
 * The defects. UPSTREAM-BUGS 264: the Moon is read at tjd and at the
 * light-time instant t = tjd - tau, and a read at t past the span of the
 * ephemeris that answered at tjd was not taken as the planets' is (they
 * fall back, or are refused): on de200.eph within tau (1.4e-5 day) after
 * its start app_pos_etc_moon() went on with the vectors swi_pleph() had
 * zeroed, the Moon at the barycentre: 1e-5 day after the start 256.12 deg
 * at 0.98 AU, the Sun's direction, for 159.82 (Moshier, 159.81), flag 1,
 * strict answering; heliocentric (tau 0.0057 day) 148.4 deg for 76.7,
 * barycentric 1e-4 AU off; with the files of 1800-2400 alone after
 * sepl_18's start -1 ("sepl_12.se1 not found") with the fallback allowed
 * too, where the call before the start answers 4; 1e-5 day after
 * semo_18's start (semo_12 missing) Moshier's Moon at t with the Earth at
 * tjd made from the files' Moon, 2.7e-6 deg off the Moshier Moon the call
 * is flagged with; within tau after semo_18's end (semo_24 missing) the
 * files' Moon at t under Moshier's flag, no note, 4.8e-4 deg off. An
 * asteroid's Sun at t, past de200.eph's start, was overwritten by the
 * asteroid's read: 2^-6 day after it Chiron 124.8395 deg for 124.8200,
 * Ceres 215.196 for 215.320, flag 1, "outside JPL eph. range" noted,
 * strict answering, and with SEFLG_SPEED a segfault.
 * UPSTREAM-BUGS 265: SEFLG_SPEED refused where the call without it
 * answers: the barycentric Sun within its light time (4.4e-5 day) after
 * de200.eph's and sepl_18's starts (the velocity read at t, bug 78's),
 * -1 for 17921 and 17922; Moshier's Sun within its light time after its
 * planets' start - 0.3 day (the aberration rate's Earth at t, bug 34's),
 * -1 for 4; Moshier's true node and osculating apogee within 2^-4 day
 * inside the Moon's margins (0.2 day past its range, bug 255), a speed
 * epoch past them, -1 for 4: bug 225's defect on Moshier.
 *
 * The check, the files linked in /tmp, de200.eph in EPHEDIR:
 *  (a) the Moon (and on de200.eph and sepl_18 the Sun, Mars and Pluto, on
 *      de200.eph Chiron and Ceres), geocentric, topocentric, heliocentric,
 *      barycentric and equatorial, with and without SEFLG_SPEED, the
 *      fallback allowed and strict, at 2^-20, 2^-18 and 2^-17 day
 *      (geocentric, topocentric, equatorial Moon), 2^-9, 2^-6 and 2^-5
 *      (Chiron), 2^-9, 2^-8 and 2^-7 (Ceres) or 2^-20, 2^-12 and 2^-9 day
 *      after: de200.eph's start; with the files of 1800-2400 alone
 *      sepl_18's start; with sepl_12, sepl_18, semo_18 and seas_*
 *      semo_18's start; with sepl_18, sepl_24, semo_18 and seas_*
 *      semo_18's end. Each in a pristine process answers as the fallback
 *      whole: on de200.eph and sepl_18 the Moshier request's flag and place
 *      bit for bit, its refusal if it refuses, the note as the call before
 *      the start notes it ("outside JPL eph. range ... using Moshier Eph; ",
 *      an asteroid's "sun: " before it or its own fall's "using Moshier
 *      eph.; ", the files' "not found ... using Moshier eph.; "), strict
 *      that call's refusal at tjd; at semo_18's edges the call in the same
 *      directory less semo_18, flag, place and serr (its path read for the
 *      other's); and with SEFLG_SPEED the flag, place and serr of the call
 *      without it. Each edge's calls in one process as their pristine ones,
 *      with the Sun and Mars at the instants after the Moon at semo_18's
 *      edges (the Earth there is the files').
 *  (b) SEFLG_SPEED, both settings: the barycentric Sun 2^-20, 2^-17 and
 *      2^-15 day after de200.eph's and sepl_18's starts; Moshier's Sun,
 *      geocentric, topocentric and equatorial, 2^-20, 2^-12 and 2^-9 day
 *      after its planets' start - 0.3; Moshier's true node and osculating
 *      apogee, geocentric, equatorial and J2000, 2^-20, 2^-8, 2^-5 + 2^-10
 *      and 2^-4 - 2^-10 day inside its Moon's start - 0.2 and end + 0.2
 *      (and 2^-10 day outside: refused alike): each answers as the call
 *      without the flag, its flag with SEFLG_SPEED, the place bit for bit,
 *      serr the same; its longitude and latitude speeds within 1e-8
 *      deg/day of one-sided differences inward of the places without the
 *      flag (five points 2^-8 day apart, the node's and apogee's nine 2^-4
 *      day apart); each edge's calls in one process as their pristine ones.
 * Measured: 840 rows pass, (b)'s speeds within 2.35e-9 deg/day of their
 * places' rate (the barycentric Sun's from the files, as inside; the
 * node's 6.6e-10, the apogee's 3.0e-9). Before the fix 391 fail ((a) 241:
 * de200.eph's start 122, of them 62 the asteroids', a segfault each with
 * SEFLG_SPEED, sepl_18's 60, semo_18's start 29, its end 30; (b) 150). The
 * upstream build fails 550.
 *
 * Run as
 *   ltstart EPHEDIR
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

static char EPHE[PATH_MAX];
static char D18[] = "/tmp/ltstart18-XXXXXX";	/* sepl_18, semo_18, seas_18 */
static char DM12[] = "/tmp/ltstartm12-XXXXXX";	/* sepl_12, sepl_18, semo_18, seas_* */
static char DN12[] = "/tmp/ltstartn12-XXXXXX";	/* the same but semo_18 */
static char DM24[] = "/tmp/ltstartm24-XXXXXX";	/* sepl_18, sepl_24, semo_18, seas_* */
static char DN24[] = "/tmp/ltstartn24-XXXXXX";	/* the same but semo_18 */
static char DNO[] = "/tmp/ltstartno-XXXXXX";	/* none */
enum { D_FULL, D_18, D_M12, D_N12, D_M24, D_N24, D_NO };

static const char *dirof(int d)
{
  return d == D_18 ? D18 : d == D_M12 ? DM12 : d == D_N12 ? DN12 : d == D_M24 ? DM24
         : d == D_N24 ? DN24 : d == D_NO ? DNO : EPHE;
}

static const int32 FL[] = {0, SEFLG_TOPOCTR, SEFLG_HELCTR, SEFLG_BARYCTR, SEFLG_EQUATORIAL};
static const char *const FLSZ[] = {"geocentric", "topocentric", "heliocentric", "barycentric",
                                   "equatorial"};
#define NFL 5

/* the answer of a call, as a pipe carries it */
struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; int ran; };
#define MAXCALL 200
struct many { struct ans a[MAXCALL]; int ran; };
struct req { int ipl; int32 fl; double jd; };

static int nbad, nrow;
static double maxerr[3];	/* (b): the Suns', the node's, the apogee's */

static void setup(const char *dir, int fb)
{
  swe_set_ephe_path(dir);
  swe_set_jpl_file("de200.eph");
  swe_set_ephe_fallback(fb);
  swe_set_topo(8.55, 47.37, 400);
}

static void call(const struct req *q, struct ans *a)
{
  memset(a, 0, sizeof *a);
  a->r = swe_calc(q->jd, q->ipl, q->fl, a->x, a->serr);
  a->ran = 1;
}

/* the calls q[0 .. n - 1] in one forked, pristine process */
static int run(int dir, int fb, const struct req *q, int n, struct many *out)
{
  int fd[2], k;
  size_t got = 0, sz = sizeof *out;
  ssize_t r;
  pid_t pid;
  memset(out, 0, sz);
  if (n > MAXCALL || pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    struct many *buf = calloc(1, sz);
    close(fd[0]);
    if (buf == NULL)
      _exit(3);
    setup(dirof(dir), fb);
    for (k = 0; k < n; k++)
      call(&q[k], &buf->a[k]);
    buf->ran = 1;
    _exit(write(fd[1], buf, sz) == (ssize_t) sz ? 0 : 3);
  }
  close(fd[1]);
  while (got < sz && (r = read(fd[0], (char *) out + got, sz - got)) > 0)
    got += (size_t) r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return got == sz && out->ran ? 0 : -1;
}

/* one call in a pristine process */
static int one(int dir, int fb, int ipl, int32 fl, double jd, struct ans *a)
{
  static struct many m;
  struct req q;
  q.ipl = ipl;
  q.fl = fl;
  q.jd = jd;
  if (run(dir, fb, &q, 1, &m) != 0)
    return -1;
  *a = m.a[0];
  return 0;
}

static void report(const char *sz, const char *what, const struct ans *a)
{
  char s[AS_MAXCH], *p;
  nbad++;
  if (nbad > 40)
    return;
  memcpy(s, a->serr, sizeof s);
  s[sizeof s - 1] = '\0';
  for (p = s; *p; p++)
    if (*p == '\n')
      *p = '|';
  printf("    FAIL %s: %s; ret %d lon %.10f serr '%.90s'\n", sz, what, a->r, a->x[0], s);
}

/* s with the first from replaced by to */
static void subst(char *s, const char *from, const char *to)
{
  char t[2 * AS_MAXCH], *p = strstr(s, from);
  if (p == NULL || *from == '\0')
    return;
  snprintf(t, sizeof t, "%.*s%s%s", (int) (p - s), s, to, p + strlen(from));
  t[AS_MAXCH - 1] = '\0';
  memcpy(s, t, AS_MAXCH);
}

static AS_BOOL ends_with(const char *s, const char *e)
{
  size_t a = strlen(s), b = strlen(e);
  return a >= b && strcmp(s + a - b, e) == 0;
}

static AS_BOOL same_place(const struct ans *a, const struct ans *b)
{
  return memcmp(a->x, b->x, sizeof a->x) == 0;
}

/* (a): the edges */
enum { REF_MOSH, REF_DIR };
static struct edge {
  const char *sz;
  int dir, refdir, ref;
  int32 eph;
  double t0;		/* the start (or the Moon file's end); 0: read from the file */
  int moonfile;		/* 1 the Moon file's start, 2 its end */
  int planets;		/* 1 the Sun, Mars and Pluto too, 2 and Chiron and Ceres */
} EDGES[] = {
  {"de200.eph's start", D_FULL, D_FULL, REF_MOSH, SEFLG_JPLEPH, 2305424.5, 0, 2},
  {"sepl_18's start, the files of 1800-2400 alone", D_18, D_18, REF_MOSH, SEFLG_SWIEPH,
   2378496.5, 0, 1},
  {"semo_18's start, semo_12 missing", D_M12, D_N12, REF_DIR, SEFLG_SWIEPH, 0, 1, 0},
  {"semo_18's end, semo_24 missing", D_M24, D_N24, REF_DIR, SEFLG_SWIEPH, 0, 2, 0},
};
#define NEDGE ((int) (sizeof EDGES / sizeof EDGES[0]))

static const struct { const char *sz; int ipl; } ABODY[] = {
  {"Moon", SE_MOON}, {"Sun", SE_SUN}, {"Mars", SE_MARS}, {"Pluto", SE_PLUTO},
  {"Chiron", SE_CHIRON}, {"Ceres", SE_CERES},
};
#define NABODY ((int) (sizeof ABODY / sizeof ABODY[0]))

/* the Moon file's span, from a call in the edge's directory */
static int moon_file_span(int dir, double *t0, double *t1)
{
  int fd[2];
  double v[2] = {0, 0};
  pid_t pid;
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    double x[6], s = 0, e = 0;
    char serr[AS_MAXCH];
    int denum;
    close(fd[0]);
    setup(dirof(dir), 1);
    swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
    if (swe_get_current_file_data(1, &s, &e, &denum) == NULL)
      s = e = 0;
    v[0] = s;
    v[1] = e;
    _exit(write(fd[1], v, sizeof v) == (ssize_t) sizeof v ? 0 : 3);
  }
  close(fd[1]);
  if (read(fd[0], v, sizeof v) != (ssize_t) sizeof v)
    v[0] = v[1] = 0;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  *t0 = v[0];
  *t1 = v[1];
  return v[0] > 0 && v[1] > v[0] ? 0 : -1;
}

static const double OFF_GEO[3] = {1.0 / 1048576, 1.0 / 262144, 1.0 / 131072};
static const double OFF_FAR[3] = {1.0 / 1048576, 1.0 / 4096, 1.0 / 512};
/* the asteroids', within their light time (Chiron's 0.054 day, Ceres's
 * 0.019, heliocentric 0.015) */
static const double OFF_CHIRON[3] = {1.0 / 512, 1.0 / 64, 1.0 / 32};
static const double OFF_CERES[3] = {1.0 / 512, 1.0 / 256, 1.0 / 128};

static void part_a(void)
{
  static struct req q[MAXCALL];
  static struct ans pr[MAXCALL];
  static int extra[MAXCALL];		/* the Sun and Mars after the Moon */
  static struct ans plain[NABODY][NFL][3];	/* the rows without SEFLG_SPEED */
  static struct many w;
  int ie, fb, ib, jf, sp, ko, n, k;
  for (ie = 0; ie < NEDGE; ie++) {
    struct edge *e = &EDGES[ie];
    double t0 = e->t0, s0, s1;
    int r0 = nrow, b0 = nbad;
    if (e->moonfile) {
      if (moon_file_span(e->dir, &s0, &s1) != 0) {
        struct ans z;
        memset(&z, 0, sizeof z);
        nrow++;
        report(e->sz, "the Moon file's span not read", &z);
        continue;
      }
      t0 = e->moonfile == 1 ? s0 : s1;
    }
    for (fb = 0; fb < 2; fb++) {
      struct ans before, beforez;
      n = 0;
      /* the call before the start: its note and its refusal */
      if (e->ref == REF_MOSH
          && (one(e->dir, 1, SE_MOON, e->eph, t0 - 1.0 / 64, &beforez) != 0
              || one(e->dir, 0, SE_MOON, e->eph, t0 - 1.0 / 64, &before) != 0)) {
        nrow++;
        report(e->sz, "the call before the start gave no answer", &before);
        continue;
      }
      for (ib = 0; ib < (e->planets == 2 ? NABODY : e->planets ? 4 : 1); ib++)
        for (jf = 0; jf < NFL; jf++)
          for (sp = 0; sp < 2; sp++)
            for (ko = 0; ko < 3; ko++) {
              int ipl = ABODY[ib].ipl;
              int32 fl = FL[jf] | e->eph | (sp ? SEFLG_SPEED : 0);
              AS_BOOL geo = ipl == SE_MOON && !(FL[jf] & (SEFLG_HELCTR | SEFLG_BARYCTR));
              double jd = t0 + (geo ? OFF_GEO[ko] : ipl == SE_CHIRON ? OFF_CHIRON[ko]
                                : ipl == SE_CERES ? OFF_CERES[ko] : OFF_FAR[ko]);
              struct ans a, m;
              char sz[300], want[AS_MAXCH], st[32], st2[32];
              const char *why = NULL;
              /* the Sun's heliocentric place is none, its barycentric
               * one reads no light-time instant (part (b)) */
              if (ipl == SE_SUN && (FL[jf] & (SEFLG_HELCTR | SEFLG_BARYCTR)))
                continue;
              snprintf(sz, sizeof sz, "%s, %s, %s %s%s jd %.10f", e->sz, fb ? "fallback" : "strict",
                       ABODY[ib].sz, FLSZ[jf], sp ? " SEFLG_SPEED" : "", jd);
              nrow++;
              if (one(e->dir, fb, ipl, fl, jd, &a) != 0) {
                memset(&a, 0, sizeof a);
                report(sz, "the process gave no answer", &a);
              }
              if (!sp)
                plain[ib][jf][ko] = a;
              q[n].ipl = ipl;
              q[n].fl = fl;
              q[n].jd = jd;
              extra[n] = 0;
              pr[n++] = a;
              /* at the Moon file's edges the Sun and Mars after the Moon at
               * its instant, in the one process: the Earth there is the
               * files' (their pristine flag and place the reference; their
               * serr is not, a file opened by a later read emptying it) */
              if (e->moonfile && ko == 0 && n + 4 <= MAXCALL) {
                static const int pl[2] = {SE_SUN, SE_MARS};
                int j;
                for (j = 0; j < 4; j++) {
                  extra[n] = 1;
                  q[n].ipl = pl[j / 2];
                  q[n].fl = e->eph | ((j & 1) ? SEFLG_SPEED : 0);
                  q[n].jd = jd;
                  if (one(e->dir, fb, q[n].ipl, q[n].fl, q[n].jd, &pr[n]) != 0)
                    memset(&pr[n], 0, sizeof pr[0]);
                  n++;
                }
              }
              if (!a.ran)
                continue;
              if (e->ref == REF_MOSH) {
                if (!fb) {
                  /* the refusal of the call before the start, at tjd */
                  snprintf(want, sizeof want, "%s", before.serr);
                  snprintf(st, sizeof st, "%.4f", t0 - 1.0 / 64);
                  snprintf(st2, sizeof st2, "%.4f", jd);
                  subst(want, st, st2);
                  if (a.r != ERR)
                    why = "answered where the fallback is refused";
                  else if (strcmp(a.serr, want) != 0)
                    why = "serr not the refusal the call before the start gives";
                } else if (one(e->dir, 1, ipl, (fl & ~e->eph) | SEFLG_MOSEPH, jd, &m) != 0) {
                  why = "the Moshier request gave no answer";
                } else if (m.r < 0) {
                  if (a.r != m.r || strcmp(a.serr, m.serr) != 0)
                    why = "not refused as the Moshier request is";
                } else if (a.r != m.r)
                  why = "flag not the Moshier request's";
                else if (!same_place(&a, &m))
                  why = "place not the Moshier request's";
                else if (e->eph == SEFLG_JPLEPH
                         ? !(strncmp(a.serr, ib >= 4 ? "sun: jd " : "jd ", ib >= 4 ? 8 : 3) == 0
                             && strstr(a.serr, " outside JPL eph. range 2305424.50 .. 2513392.50;") != NULL
                             && (ends_with(a.serr, " \nusing Moshier Eph; ")
                                 /* the asteroid's own light-time fall */
                                 || (ib >= 4 && ends_with(a.serr, ";\nusing Moshier eph.; "))))
                         : strcmp(a.serr, beforez.serr) != 0)
                  why = "serr not the fallback's note";
              } else {
                if (one(e->refdir, fb, ipl, fl, jd, &m) != 0)
                  why = "the call without the Moon file gave no answer";
                else {
                  subst(m.serr, dirof(e->refdir), dirof(e->dir));
                  if (a.r != m.r)
                    why = "flag not the one without the Moon file";
                  else if (a.r >= 0 && !same_place(&a, &m))
                    why = "place not the one without the Moon file";
                  else if (strcmp(a.serr, m.serr) != 0)
                    why = "serr not the one without the Moon file";
                }
              }
              if (why == NULL && sp) {
                const struct ans *z = &plain[ib][jf][ko];
                if (!z->ran)
                  why = "the call without SEFLG_SPEED gave no answer";
                else if (a.r != (z->r < 0 ? z->r : (z->r | SEFLG_SPEED)) || strcmp(a.serr, z->serr) != 0
                         || (z->r >= 0 && memcmp(a.x, z->x, 3 * sizeof(double)) != 0))
                  why = "not the call's without SEFLG_SPEED";
              }
              if (why != NULL)
                report(sz, why, &a);
            }
      /* in one process */
      {
        char sz[300];
        snprintf(sz, sizeof sz, "%s, %s, in one process", e->sz, fb ? "fallback" : "strict");
        nrow++;
        if (run(e->dir, fb, q, n, &w) != 0) {
          report(sz, "the process gave no answer", &pr[0]);
          continue;
        }
        for (k = 0; k < n; k++)
          if (w.a[k].r != pr[k].r || memcmp(w.a[k].x, pr[k].x, sizeof pr[k].x) != 0
              || (!extra[k] && strcmp(w.a[k].serr, pr[k].serr) != 0)) {
            char s[300];
            snprintf(s, sizeof s, "call %d (body %d, flag %d, jd %.10f) not its pristine process's",
                     k, q[k].ipl, q[k].fl, q[k].jd);
            report(sz, s, &w.a[k]);
            break;
          }
      }
    }
    printf("  (a) %s: %d rows, %d failed\n", e->sz, nrow - r0, nbad - b0);
  }
}

/* (b): SEFLG_SPEED where the call without it answers */
static const struct sedge {
  const char *sz;
  int dir;
  int32 eph;
  double t0;
  int inward;
  int nod;		/* 0 the Sun, 1 the true node and the osculating apogee */
  int32 fl[3];
  int nfl;
  double off[5];
  int noff;
} SEDGES[] = {
  {"de200.eph's start, the barycentric Sun", D_FULL, SEFLG_JPLEPH, 2305424.5, 1, 0,
   {SEFLG_BARYCTR}, 1, {1.0 / 1048576, 1.0 / 131072, 1.0 / 32768}, 3},
  {"sepl_18's start, the barycentric Sun", D_18, SEFLG_SWIEPH, 2378496.5, 1, 0,
   {SEFLG_BARYCTR}, 1, {1.0 / 1048576, 1.0 / 131072, 1.0 / 32768}, 3},
  {"Moshier's planets' start - 0.3, the Sun", D_NO, SEFLG_MOSEPH, 625000.2, 1, 0,
   {0, SEFLG_TOPOCTR, SEFLG_EQUATORIAL}, 3, {1.0 / 1048576, 1.0 / 4096, 1.0 / 512}, 3},
  {"Moshier's Moon's start - 0.2", D_NO, SEFLG_MOSEPH, 625000.3, 1, 1,
   {0, SEFLG_EQUATORIAL, SEFLG_J2000}, 3,
   {-1.0 / 1024, 1.0 / 1048576, 1.0 / 256, 1.0 / 32 + 1.0 / 1024, 1.0 / 16 - 1.0 / 1024}, 5},
  {"Moshier's Moon's end + 0.2", D_NO, SEFLG_MOSEPH, 2818000.7, -1, 1,
   {0, SEFLG_EQUATORIAL, SEFLG_J2000}, 3,
   {-1.0 / 1024, 1.0 / 1048576, 1.0 / 256, 1.0 / 32 + 1.0 / 1024, 1.0 / 16 - 1.0 / 1024}, 5},
};
#define NSEDGE ((int) (sizeof SEDGES / sizeof SEDGES[0]))

static void part_b(void)
{
  static const double c5[5] = {-25, 48, -36, 16, -3};	/* / 12 */
  static const double c9[9] = {-761.0 / 280, 8, -14, 56.0 / 3, -35.0 / 2, 56.0 / 5, -14.0 / 3,
                               8.0 / 7, -1.0 / 8};
  static struct req q[MAXCALL];
  static struct ans pr[MAXCALL];
  static struct many w, pl;
  int ie, fb, ib, jf, ko, n, k, c;
  for (ie = 0; ie < NSEDGE; ie++) {
    const struct sedge *e = &SEDGES[ie];
    int r0 = nrow, b0 = nbad;
    for (fb = 0; fb < 2; fb++) {
      n = 0;
      for (ib = 0; ib < (e->nod ? 2 : 1); ib++)
        for (jf = 0; jf < e->nfl; jf++)
          for (ko = 0; ko < e->noff; ko++) {
            int ipl = e->nod ? (ib ? SE_OSCU_APOG : SE_TRUE_NODE) : SE_SUN;
            int32 fl = e->fl[jf] | e->eph;
            double jd = e->t0 + e->inward * e->off[ko];
            int npt = e->nod ? 9 : 5;
            double h = e->nod ? 1.0 / 16 : 1.0 / 256, lim = 1e-8;
            struct req st[9];
            struct ans a;
            const char *why = NULL;
            char sz[300];
            snprintf(sz, sizeof sz, "%s, %s, %s, flags %d, jd %.10f", e->sz,
                     fb ? "fallback" : "strict", e->nod ? (ib ? "osc. apogee" : "true node") : "Sun",
                     fl, jd);
            nrow++;
            q[n].ipl = ipl;
            q[n].fl = fl | SEFLG_SPEED;
            q[n].jd = jd;
            if (one(e->dir, fb, ipl, fl | SEFLG_SPEED, jd, &a) != 0) {
              memset(&pr[n++], 0, sizeof pr[0]);
              report(sz, "the process gave no answer", &a);
              continue;
            }
            pr[n++] = a;
            for (k = 0; k < npt; k++) {
              st[k].ipl = ipl;
              st[k].fl = fl;
              st[k].jd = jd + e->inward * k * h;
            }
            if (run(e->dir, fb, st, npt, &pl) != 0) {
              report(sz, "the places without the flag gave no answer", &a);
              continue;
            }
            if (pl.a[0].r < 0) {
              /* past the range: refused alike, tjd named */
              if (a.r != pl.a[0].r || strcmp(a.serr, pl.a[0].serr) != 0)
                why = "not refused as the call without the flag is";
              else if (e->off[ko] > 0)
                why = "the call without the flag refused inside the range";
              if (why != NULL)
                report(sz, why, &a);
              continue;
            } else if (a.r != (pl.a[0].r | SEFLG_SPEED))
              why = "flag not the one without SEFLG_SPEED, with it";
            else if (strcmp(a.serr, pl.a[0].serr) != 0)
              why = "serr not the one without SEFLG_SPEED";
            else if (memcmp(a.x, pl.a[0].x, 3 * sizeof(double)) != 0)
              why = "place not the one without SEFLG_SPEED";
            for (k = 1; why == NULL && k < npt; k++)
              if (pl.a[k].r != pl.a[0].r)
                why = "a place inward answered otherwise";
            for (c = 0; why == NULL && c < 2; c++) {
              double d = 0, err;
              for (k = 1; k < npt; k++) {
                double y = c == 0 ? swe_difdeg2n(pl.a[k].x[0], pl.a[0].x[0])
                                  : pl.a[k].x[c] - pl.a[0].x[c];
                d += (npt == 5 ? c5[k] / 12 : c9[k]) * y;
              }
              d *= e->inward / h;
              err = fabs(a.x[3 + c] - d);
              if (getenv("LTSTART_V") != NULL)
                printf("      %s c%d speed %.12g rate %.12g err %.3g\n", sz, c, a.x[3 + c], d, err);
              if (err > maxerr[e->nod ? 1 + ib : 0])
                maxerr[e->nod ? 1 + ib : 0] = err;
              if (!(err <= lim))
                why = c == 0 ? "longitude speed off its places' rate" : "latitude speed off its places' rate";
            }
            if (why != NULL)
              report(sz, why, &a);
          }
      {
        char sz[300];
        snprintf(sz, sizeof sz, "%s, %s, in one process", e->sz, fb ? "fallback" : "strict");
        nrow++;
        if (run(e->dir, fb, q, n, &w) != 0) {
          report(sz, "the process gave no answer", &pr[0]);
          continue;
        }
        for (k = 0; k < n; k++)
          if (w.a[k].r != pr[k].r || memcmp(w.a[k].x, pr[k].x, sizeof pr[k].x) != 0
              || strcmp(w.a[k].serr, pr[k].serr) != 0) {
            char s[300];
            snprintf(s, sizeof s, "call %d (flag %d, jd %.10f) not its pristine process's", k,
                     q[k].fl, q[k].jd);
            report(sz, s, &w.a[k]);
            break;
          }
      }
    }
    printf("  (b) %s: %d rows, %d failed\n", e->sz, nrow - r0, nbad - b0);
  }
}

static int link_set(char *dir, const char *const *f, int nf)
{
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  int k;
  if (mkdtemp(dir) == NULL)
    return -1;
  for (k = 0; k < nf; k++) {
    snprintf(src, sizeof src, "%s/%s", EPHE, f[k]);
    snprintf(dst, sizeof dst, "%s/%s", dir, f[k]);
    if (access(src, R_OK) != 0 || symlink(src, dst) != 0)
      return -1;
  }
  return 0;
}

static void unlink_all(const char *dir)
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
  static const char *const f18[] = {"sepl_18.se1", "semo_18.se1", "seas_18.se1"};
  static const char *const fm12[] = {"sepl_12.se1", "sepl_18.se1", "seas_12.se1", "seas_18.se1",
                                     "semo_18.se1"};
  static const char *const fm24[] = {"sepl_18.se1", "sepl_24.se1", "seas_18.se1", "seas_24.se1",
                                     "semo_18.se1"};
  int ok;
  if (argc != 2 || realpath(argv[1], EPHE) == NULL) {
    fprintf(stderr, "usage: ltstart EPHEDIR\n");
    return 2;
  }
  ok = link_set(D18, f18, 3) == 0 && link_set(DM12, fm12, 5) == 0 && link_set(DN12, fm12, 4) == 0
       && link_set(DM24, fm24, 5) == 0 && link_set(DN24, fm24, 4) == 0 && link_set(DNO, NULL, 0) == 0;
  if (!ok) {
    printf("G171 FAIL: cannot build the directories in /tmp\n");
  } else {
    printf("G171: within the light time after a span's start the Moon answers whole from the\n"
           "      ephemeris that answers its light-time instant, and SEFLG_SPEED answers where\n"
           "      the call without it does\n");
    part_a();
    part_b();
  }
  unlink_all(D18);
  unlink_all(DM12);
  unlink_all(DN12);
  unlink_all(DM24);
  unlink_all(DN24);
  unlink_all(DNO);
  if (!ok)
    return 1;
  printf("  rows %d, failed %d; (b)'s speeds within %.2e deg/day of their places' rate (the\n"
         "  node's %.2e, the apogee's %.2e)\n", nrow, nbad, maxerr[0], maxerr[1], maxerr[2]);
  printf(nbad ? "G171 FAIL: %d\n" : "G171 PASS\n", nbad);
  return nbad ? 1 : 0;
}
