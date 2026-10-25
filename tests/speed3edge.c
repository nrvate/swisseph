/* speed3edge.c -- G165: at the edge of an ephemeris's span swe_calc() with
 * SEFLG_SPEED3 answers as without a speed flag, its three places from the
 * ephemeris that answers at the instant, its speeds their rate on the
 * instant's side.
 *
 * The defect (UPSTREAM-BUGS 249). SEFLG_SPEED3 reads the body at tjd - dt,
 * tjd + dt and tjd (dt 1e-4 day, the Moon's 5e-5, the true node's and the
 * osculating apogee's 2^-9, the interpolated apsides' 2^-8), and each read
 * fell back on its own: within dt of a span's edge the speed was the
 * difference of places from two ephemerides. On de200.eph 2^-16 day before
 * its end Mars 0.72180 deg/day for 0.76100 (SEFLG_SPEED), the Moon 3.3725
 * for 14.0460, the Sun 1.0239 for 0.9719, Pluto 45.98 for 0.0144, flag 129
 * (JPL), "outside JPL eph. range ... using Moshier Eph" in serr, which the
 * call without a flag does not say, and strict not refusing; the true node
 * 2^-10 day before it 0.5835 for 0.0181, the apogee 7.34 for 3.08; at the
 * Swiss files' and the Moon files' edges alike (the node 2^-12 day inside
 * semo_18's end with sepl_24 -0.1731 for 0.0173; the Moon there 2^-16 day
 * inside 132, Moshier's label, its speed 19.506 for 14.664, strict
 * refused); within dt of where Moshier's Earth takes the short lunar
 * series' Moon (0.2 day past its range) Mars 0.42217 for 0.42618; and
 * where a read left Moshier's range (the files asked with none on the path
 * within dt of Moshier's end or start; Moshier asked within dt of its
 * margins) the whole call was refused, -1, where the call without a flag
 * answers 4.
 *
 * The check, the complete directory (de200.eph and the Swiss files), the
 * files of 1800-2400 alone (sepl_18, semo_18, seas_18), with sepl_24 and
 * seas_24 (semo_18's end alone), every file but the Moon files, and none,
 * linked in /tmp: the Sun, the Moon, Mars, Pluto, the true node, the
 * osculating apogee and the interpolated apogee, geocentric, topocentric,
 * heliocentric, barycentric and equatorial, the fallback allowed and
 * strict, at
 *   de200.eph's end - 2^-16 and - 2^-10; its start at the planets' light
 *   time + 2^-18 (the instant searched where the call without a flag is
 *   first answered, strict), the Moon at + 2^-15 (geocentric, topocentric,
 *   equatorial), the nodes and apsides at + 2^-10;
 *   sepl_18's start at the light time + 2^-18 and its end - 2^-16;
 *   semo_18's start + 2^-10 (the nodes and apsides); its end - 2^-16 and
 *   - 2^-10 with sepl_24;
 *   1500 without the Moon files, 2000 without any;
 *   Moshier's end - 2^-16 and start + 2^-16, the files asked, none on the
 *   path; Moshier asked at its planets' margin's end - 2^-16 and its
 *   Moon's, and their starts at the light time + 2^-18;
 *   2000 on de200.eph, the files and Moshier:
 * each call with SEFLG_SPEED3 in a pristine process answers as the call
 * without a speed flag in another: its flag with SEFLG_SPEED3, the place
 * bit for bit, serr the same (refused alike); its longitude, latitude and
 * distance speeds within 2e-6 deg/day + 3e-6 of the rate (the nodes' and
 * apsides' 3e-6 deg/day: their places' noise) of five-point one-sided
 * differences, inward, of the places without a flag (2^-8 day, the nodes'
 * and apsides' 2^-6), each answered as the instant's; and every call of an
 * edge and setting in one process as its pristine process. Where the call
 * with SEFLG_SPEED itself does not answer as the one without (refused, or
 * its flag or serr not that one's), the row is counted apart, as
 * SEFLG_SPEED's own (not this defect's), and the SEFLG_SPEED3 call still
 * checked against the call without a flag.
 * Measured: 1212 rows pass, the speeds within 2.79e-5 deg/day of their
 * places' rate (the Moon's central difference, its nominal 2 dt 1.7e-6 of
 * itself off the instants' interval: UPSTREAM-BUGS 249's notes), the nodes
 * and apsides 6.8e-7; 46 rows SEFLG_SPEED's own (the barycentric Sun past
 * de200.eph's and sepl_18's start, Moshier's Sun at its planets' start,
 * Moshier's node and apogee within 2^-5 day of the Moon's margins:
 * refused). Before the fix 494 fail (173 the flag, 223 serr, 96 the speed,
 * off by 8.9e-5 to 4.5 deg/day, 2 the warm process). The upstream build
 * fails 730 (also its SEFLG_SPEED3 speed, 2a + b, entry 31's).
 *
 * Run as
 *   speed3edge EPHEDIR
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
#define GRID 16777216.0		/* 2^24: the instants' grid */
#ifndef SPSHOW
#define SPSHOW 6		/* SEFLG_SPEED's own rows shown */
#endif

static char EPHE[PATH_MAX];
static char D18[] = "/tmp/speed3e18-XXXXXX";	/* sepl_18, semo_18, seas_18 */
static char DMO[] = "/tmp/speed3emo-XXXXXX";	/* and sepl_24, seas_24 */
static char DNM[] = "/tmp/speed3enm-XXXXXX";	/* every file but semo_* */
static char DNO[] = "/tmp/speed3eno-XXXXXX";	/* none */
enum { D_FULL, D_18, D_MO, D_NM, D_NO };

static const char *dirof(int d)
{
  return d == D_18 ? D18 : d == D_MO ? DMO : d == D_NM ? DNM : d == D_NO ? DNO : EPHE;
}

static const struct { const char *sz; int ipl; double h; int nod; } BODY[] = {
  {"Sun", SE_SUN, 1.0 / 256, 0}, {"Moon", SE_MOON, 1.0 / 256, 0}, {"Mars", SE_MARS, 1.0 / 256, 0},
  {"Pluto", SE_PLUTO, 1.0 / 256, 0}, {"true node", SE_TRUE_NODE, 1.0 / 64, 1},
  {"osc. apogee", SE_OSCU_APOG, 1.0 / 64, 1}, {"intp. apogee", SE_INTP_APOG, 1.0 / 64, 1},
};
#define NBODY ((int) (sizeof BODY / sizeof BODY[0]))
static const int32 FL[] = {0, SEFLG_TOPOCTR, SEFLG_HELCTR, SEFLG_BARYCTR, SEFLG_EQUATORIAL};
static const char *const FLSZ[] = {"geocentric", "topocentric", "heliocentric", "barycentric",
                                   "equatorial"};
#define NFL ((int) (sizeof FL / sizeof FL[0]))

/* the bodies and flag sets of an edge, as bit masks */
#define B_ALL 0x7f
#define B_PLAN 0x0d		/* Sun, Mars, Pluto */
#define B_MOON 0x02
#define B_NOD 0x70
#define F_ALL 0x1f
#define F_GEO 0x13		/* geocentric, topocentric, equatorial */

/* find: jd is a span's start; the row's instant is 2^-18 day after the
 * first instant on the 2^-24 grid at which the call without a flag is
 * answered, strict (searched in jd - 2^-10 .. jd + 0.5) */
static const struct edge { const char *sz; int dir; int32 eph; double jd; int inward, find, bodies, flags; } EDGES[] = {
  {"de200.eph's end - 2^-16", D_FULL, SEFLG_JPLEPH, 2513392.5 - 1.0 / 65536, -1, 0, B_ALL, F_ALL},
  {"de200.eph's end - 2^-10", D_FULL, SEFLG_JPLEPH, 2513392.5 - 1.0 / 1024, -1, 0, B_ALL, F_ALL},
  {"de200.eph's start, light time + 2^-18", D_FULL, SEFLG_JPLEPH, 2305424.5, 1, 1, B_PLAN, F_ALL},
  {"de200.eph's start + 2^-15", D_FULL, SEFLG_JPLEPH, 2305424.5 + 1.0 / 32768, 1, 0, B_MOON, F_GEO},
  {"de200.eph's start + 2^-10", D_FULL, SEFLG_JPLEPH, 2305424.5 + 1.0 / 1024, 1, 0, B_NOD, F_ALL},
  {"sepl_18's start, light time + 2^-18", D_18, SEFLG_SWIEPH, 2378496.5, 1, 1, B_PLAN | B_MOON, F_ALL},
  {"sepl_18's end - 2^-16", D_18, SEFLG_SWIEPH, 2597651.3591314554, -1, 0, B_ALL, F_ALL},
  {"semo_18's start + 2^-10", D_18, SEFLG_SWIEPH, 2378487.556347251, 1, 0, B_NOD, F_ALL},
  {"semo_18's end - 2^-16", D_MO, SEFLG_SWIEPH, 2597656.4574371576, -1, 0, B_ALL, F_ALL},
  {"semo_18's end - 2^-10", D_MO, SEFLG_SWIEPH, 2597656.456475854, -1, 0, B_ALL, F_ALL},
  {"1500, no Moon files", D_NM, SEFLG_SWIEPH, 2268923.5, 1, 0, B_ALL, F_ALL},
  {"2000, no files", D_NO, SEFLG_SWIEPH, 2451545.0, 1, 0, B_ALL, F_ALL},
  {"Moshier's end - 2^-16, the files asked", D_NO, SEFLG_SWIEPH, 2818000.5 - 1.0 / 65536, -1, 0, B_ALL, F_ALL},
  {"Moshier's start + 2^-16, the files asked", D_NO, SEFLG_SWIEPH, 625000.5 + 1.0 / 65536, 1, 0, B_ALL, F_ALL},
  {"Moshier's planets' end + 0.3 - 2^-16", D_NO, SEFLG_MOSEPH, 2818000.7999846935, -1, 0, B_ALL, F_ALL},
  {"Moshier's Moon's end + 0.2 - 2^-16", D_NO, SEFLG_MOSEPH, 2818000.6999847293, -1, 0, B_ALL, F_ALL},
  {"Moshier's planets' start - 0.3, light time + 2^-18", D_NO, SEFLG_MOSEPH, 625000.2, 1, 1, B_PLAN, F_ALL},
  {"Moshier's Moon's start - 0.2, light time + 2^-18", D_NO, SEFLG_MOSEPH, 625000.3, 1, 1, B_MOON | B_NOD, F_GEO},
  {"2000, de200.eph", D_FULL, SEFLG_JPLEPH, 2451545.0, 1, 0, B_ALL, F_ALL},
  {"2000, the files", D_FULL, SEFLG_SWIEPH, 2451545.0, -1, 0, B_ALL, F_ALL},
  {"2000, Moshier", D_FULL, SEFLG_MOSEPH, 2451545.0, 1, 0, B_ALL, F_ALL},
};
#define NEDGE ((int) (sizeof EDGES / sizeof EDGES[0]))

/* the answer of a call, as a pipe carries it */
struct ans { int32 r; double x[6]; char serr[AS_MAXCH]; int ran; };
/* a row's first process: SEFLG_SPEED3 first, then the stencil without a flag */
struct rowres { struct ans s3; struct ans st[5]; int ran; };
#define MAXROW (NBODY * NFL)
struct warm { struct ans a[MAXROW]; int ran; };

static int nbad, nrow, nsp, nspbad;
static double maxerr[2];	/* the planets' and the nodes' worst, deg/day */

static void setup(const char *dir, int fb)
{
  swe_set_ephe_path(dir);
  swe_set_jpl_file("de200.eph");
  swe_set_ephe_fallback(fb);
  swe_set_topo(8.55, 47.37, 400);
}

static void call(double jd, int ipl, int32 fl, struct ans *a)
{
  memset(a, 0, sizeof *a);
  a->r = swe_calc(jd, ipl, fl, a->x, a->serr);
  a->ran = 1;
}

/* the jobs a forked, pristine process runs */
enum { P_ROW, P_ONE, P_FIND, P_WARM };
struct job {
  int what, fb, ib, jf;
  const struct edge *e;
  double jd;			/* the row's instant */
  int32 fl;			/* P_ONE's flags */
  double jds[MAXROW];		/* P_WARM: each row's instant, or 0 */
};

static void row_proc(const struct job *j, struct rowres *out)
{
  int k;
  int32 fl = FL[j->jf] | j->e->eph;
  setup(dirof(j->e->dir), j->fb);
  call(j->jd, BODY[j->ib].ipl, fl | SEFLG_SPEED3, &out->s3);
  for (k = 0; k < 5; k++)
    call(j->jd + j->e->inward * k * BODY[j->ib].h, BODY[j->ib].ipl, fl, &out->st[k]);
  out->ran = 1;
}

static void find_proc(const struct job *j, double *out)
{
  double x[6], lo, hi, mid;
  char serr[AS_MAXCH];
  int32 fl = FL[j->jf] | j->e->eph;
  int ipl = BODY[j->ib].ipl;
  setup(dirof(j->e->dir), 0);
  lo = floor((j->e->jd - 1.0 / 1024) * GRID) / GRID;
  hi = floor((j->e->jd + 0.5) * GRID) / GRID;
  if (swe_calc(lo, ipl, fl, x, serr) >= 0 || swe_calc(hi, ipl, fl, x, serr) < 0) {
    *out = floor((j->e->jd + 1.0 / 1024) * GRID) / GRID;	/* no turn: past it */
    return;
  }
  while (hi - lo > 1 / GRID) {
    mid = floor((lo + hi) / 2 * GRID) / GRID;
    if (swe_calc(mid, ipl, fl, x, serr) >= 0)
      hi = mid;
    else
      lo = mid;
  }
  *out = hi + 1.0 / 262144;
}

static void warm_proc(const struct job *j, struct warm *out)
{
  int ib, jf;
  setup(dirof(j->e->dir), j->fb);
  for (ib = 0; ib < NBODY; ib++)
    for (jf = 0; jf < NFL; jf++)
      if (j->jds[ib * NFL + jf] != 0)
        call(j->jds[ib * NFL + jf], BODY[ib].ipl, FL[jf] | j->e->eph | SEFLG_SPEED3,
             &out->a[ib * NFL + jf]);
  out->ran = 1;
}

static int run(const struct job *j, void *out, size_t n)
{
  int fd[2];
  size_t got = 0;
  ssize_t r;
  pid_t pid;
  memset(out, 0, n);
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    void *buf = calloc(1, n);
    close(fd[0]);
    if (buf == NULL)
      _exit(3);
    switch (j->what) {
      case P_ROW: row_proc(j, buf); break;
      case P_ONE:
        setup(dirof(j->e->dir), j->fb);
        call(j->jd, BODY[j->ib].ipl, j->fl, buf);
        break;
      case P_FIND: find_proc(j, buf); break;
      default: warm_proc(j, buf); break;
    }
    _exit(write(fd[1], buf, n) == (ssize_t) n ? 0 : 3);
  }
  close(fd[1]);
  while (got < n && (r = read(fd[0], (char *) out + got, n - got)) > 0)
    got += (size_t) r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return got == n ? 0 : -1;
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
  printf("    FAIL %s: %s; ret %d serr '%.80s'\n", sz, what, a->r, s);
}

/* one row: the SEFLG_SPEED3 call in a pristine process against the call
 * without a flag and the one with SEFLG_SPEED in theirs, its speeds
 * against its places' rate; the pristine answer in *s3 */
static void row(const struct edge *e, int fb, int ib, int jf, double jd, struct ans *s3)
{
  struct job j;
  struct rowres w;
  struct ans z, sp;
  const char *why = NULL;
  char sz[300];
  int k, c, spok;
  memset(&j, 0, sizeof j);
  j.e = e;
  j.fb = fb;
  j.ib = ib;
  j.jf = jf;
  j.jd = jd;
  snprintf(sz, sizeof sz, "%s, %s, %s %s jd %.8f", e->sz, fb ? "fallback" : "strict", BODY[ib].sz,
           FLSZ[jf], jd);
  nrow++;
  j.what = P_ROW;
  if (run(&j, &w, sizeof w) != 0 || !w.ran) {
    memset(s3, 0, sizeof *s3);
    report(sz, "the process gave no answer", s3);
    return;
  }
  *s3 = w.s3;
  j.what = P_ONE;
  j.fl = FL[jf] | e->eph;
  if (run(&j, &z, sizeof z) != 0 || !z.ran) {
    report(sz, "the call without a flag gave no answer", &w.s3);
    return;
  }
  j.fl |= SEFLG_SPEED;
  if (run(&j, &sp, sizeof sp) != 0 || !sp.ran) {
    report(sz, "the call with SEFLG_SPEED gave no answer", &w.s3);
    return;
  }
  spok = sp.r == (z.r < 0 ? z.r : (z.r | SEFLG_SPEED)) && strcmp(sp.serr, z.serr) == 0
         && (z.r < 0 || memcmp(sp.x, z.x, 3 * sizeof(double)) == 0);
  if (!spok) {
    nsp++;
    if (nsp <= SPSHOW)
      printf("    (SEFLG_SPEED's own) %s: ret %d for %d, serr '%.60s'\n", sz, sp.r, z.r, sp.serr);
  }
  if (w.s3.r != (z.r < 0 ? z.r : (z.r | SEFLG_SPEED3)))
    why = "flag not the one without a flag, with SEFLG_SPEED3";
  else if (strcmp(w.s3.serr, z.serr) != 0)
    why = "serr not the one without a flag";
  else if (z.r >= 0 && memcmp(w.s3.x, z.x, 3 * sizeof(double)) != 0)
    why = "place not the one without a flag";
  for (k = 0; why == NULL && z.r >= 0 && k < 5; k++)
    if (w.st[k].r != z.r)
      why = "a stencil instant answered otherwise";
  for (c = 0; why == NULL && z.r >= 0 && c < 3; c++) {
    double y[5], d, err, lim;
    for (k = 0; k < 5; k++)
      y[k] = (c == 0 && !(FL[jf] & SEFLG_XYZ)) ? swe_difdeg2n(w.st[k].x[0], w.st[0].x[0])
                                                : w.st[k].x[c] - w.st[0].x[c];
    d = e->inward * (48 * y[1] - 36 * y[2] + 16 * y[3] - 3 * y[4]) / (12 * BODY[ib].h);
    err = fabs(w.s3.x[3 + c] - d);
    lim = (c == 2 ? 1e-9 : BODY[ib].nod ? 3e-6 : 2e-6) + 3e-6 * fabs(d);
    if (c < 2 && err > maxerr[BODY[ib].nod])
      maxerr[BODY[ib].nod] = err;
    if (!(err <= lim))
      why = c == 0 ? "longitude speed off its places' rate"
            : c == 1 ? "latitude speed off its places' rate" : "distance speed off its places' rate";
  }
  if (why != NULL) {
    if (!spok)
      nspbad++;
    report(sz, why, &w.s3);
  }
}

static int link_all(char *dir, int which)
{
  static const char *const f18[] = {"sepl_18.se1", "semo_18.se1", "seas_18.se1", "sepl_24.se1",
                                    "seas_24.se1"};
  char src[PATH_MAX + 300], dst[PATH_MAX + 300];
  int k;
  if (mkdtemp(dir) == NULL)
    return -1;
  if (which == D_18 || which == D_MO) {
    for (k = 0; k < (which == D_MO ? 5 : 3); k++) {
      snprintf(src, sizeof src, "%s/%s", EPHE, f18[k]);
      snprintf(dst, sizeof dst, "%s/%s", dir, f18[k]);
      if (access(src, R_OK) != 0 || symlink(src, dst) != 0)
        return -1;
    }
  } else if (which == D_NM) {
    DIR *d = opendir(EPHE);
    struct dirent *de;
    int n = 0;
    if (d == NULL)
      return -1;
    while ((de = readdir(d)) != NULL) {
      size_t l = strlen(de->d_name);
      if (l < 5 || strcmp(de->d_name + l - 4, ".se1") != 0 || strncmp(de->d_name, "semo", 4) == 0)
        continue;
      snprintf(src, sizeof src, "%s/%s", EPHE, de->d_name);
      snprintf(dst, sizeof dst, "%s/%s", dir, de->d_name);
      if (symlink(src, dst) != 0) {
        closedir(d);
        return -1;
      }
      n++;
    }
    closedir(d);
    if (n == 0)
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
  int ie, fb, ib, jf, ok;
  static struct ans s3[MAXROW];
  static struct warm w;
  static struct job j;
  if (argc != 2 || realpath(argv[1], EPHE) == NULL) {
    fprintf(stderr, "usage: speed3edge EPHEDIR\n");
    return 2;
  }
  ok = link_all(D18, D_18) == 0 && link_all(DMO, D_MO) == 0 && link_all(DNM, D_NM) == 0
       && link_all(DNO, D_NO) == 0;
  if (!ok) {
    printf("G165 FAIL: cannot build the directories in /tmp\n");
    unlink_all(D18);
    unlink_all(DMO);
    unlink_all(DNM);
    unlink_all(DNO);
    return 1;
  }
  printf("G165: at a span's edge SEFLG_SPEED3 answers as without a speed flag, its\n"
         "      places from the ephemeris answering at the instant\n");
  for (ie = 0; ie < NEDGE; ie++) {
    const struct edge *e = &EDGES[ie];
    double jds[MAXROW];
    memset(jds, 0, sizeof jds);
    for (ib = 0; ib < NBODY; ib++)
      for (jf = 0; jf < NFL; jf++) {
        if (!(e->bodies & (1 << ib)) || !(e->flags & (1 << jf)))
          continue;
        jds[ib * NFL + jf] = e->jd;
        if (e->find) {
          memset(&j, 0, sizeof j);
          j.what = P_FIND;
          j.e = e;
          j.ib = ib;
          j.jf = jf;
          if (run(&j, &jds[ib * NFL + jf], sizeof(double)) != 0)
            jds[ib * NFL + jf] = e->jd;
        }
      }
    for (fb = 0; fb < 2; fb++) {
      char sz[200];
      memset(s3, 0, sizeof s3);
      for (ib = 0; ib < NBODY; ib++)
        for (jf = 0; jf < NFL; jf++)
          if (jds[ib * NFL + jf] != 0)
            row(e, fb, ib, jf, jds[ib * NFL + jf], &s3[ib * NFL + jf]);
      memset(&j, 0, sizeof j);
      j.what = P_WARM;
      j.e = e;
      j.fb = fb;
      memcpy(j.jds, jds, sizeof jds);
      snprintf(sz, sizeof sz, "%s, %s, in one process", e->sz, fb ? "fallback" : "strict");
      nrow++;
      if (run(&j, &w, sizeof w) != 0 || !w.ran) {
        report(sz, "the process gave no answer", &s3[0]);
        continue;
      }
      for (ib = 0; ib < NBODY; ib++)
        for (jf = 0; jf < NFL; jf++) {
          const struct ans *a = &w.a[ib * NFL + jf], *b = &s3[ib * NFL + jf];
          if (jds[ib * NFL + jf] != 0
              && (a->r != b->r || memcmp(a->x, b->x, sizeof a->x) != 0 || strcmp(a->serr, b->serr) != 0)) {
            char s[300];
            snprintf(s, sizeof s, "%s %s: not its pristine process's answer", BODY[ib].sz, FLSZ[jf]);
            report(sz, s, a);
            break;
          }
        }
    }
  }
  unlink_all(D18);
  unlink_all(DMO);
  unlink_all(DNM);
  unlink_all(DNO);
  printf("  rows %d, failed %d; SEFLG_SPEED3's speeds within %.2e deg/day of their places' rate\n"
         "  (the nodes and apsides %.2e); %d rows where SEFLG_SPEED does not answer as the call\n"
         "  without a flag (its own, not this gate's; %d of them failed here)\n",
         nrow, nbad, maxerr[0], maxerr[1], nsp, nspbad);
  printf(nbad ? "G165 FAIL: %d\n" : "G165 PASS\n", nbad);
  return nbad ? 1 : 0;
}
