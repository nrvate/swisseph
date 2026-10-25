/* eopfollow.c -- G134: SEFLG_JPLHOR without the IERS corrections answers as
 * swe_calc() takes it, and does not crash.
 *
 * The defect (UPSTREAM-BUGS 142): plaus_iflag() turns SEFLG_JPLHOR into
 * SEFLG_JPLHOR_APPROX when no corrections are loaded (eop_dpsi_loaded <=
 * 0), but the functions whose flags skip it passed the flag on to
 * calc_nutation(), which read the corrections' table: NULL on de200 or on
 * a thread or context that never loaded them, and with an
 * eop_1962_today.txt that broke off (eop_dpsi_loaded -2) an end of 0, so
 * an index 2.45e6 doubles before the table. swe_get_ayanamsa_ex(), its _ut
 * form, the sidereal houses (through the ayanamsa) and
 * swe_gauquelin_sector()'s methods 0 and 1, for a planet or a star, with
 * SEFLG_JPLEPH | SEFLG_JPLHOR or SEFLG_SWIEPH | SEFLG_JPLHOR: SIGSEGV.
 *
 * The check, each call in a forked, pristine process: it must return, no
 * signal, and its answer must equal, bit for bit with its return flag,
 * that of a fresh process making the same call with SEFLG_JPLHOR_APPROX
 * for SEFLG_JPLHOR -- what plaus_iflag() makes of the flag for swe_calc().
 * Eight probes: swe_get_ayanamsa_ex() and swe_get_ayanamsa_ex_ut() with
 * SEFLG_JPLEPH, swe_get_ayanamsa_ex() with SEFLG_SWIEPH, swe_houses_ex2()
 * sidereal Placidus with the speeds, swe_houses_ex() sidereal Koch,
 * swe_gauquelin_sector() methods 0 and 1 for Mars and method 0 for Spica;
 * Lahiri. Six settings without corrections: de200 on the setter's thread at
 * J2000, the same with the nutation interpolated, a second thread on the
 * default context that makes the call after the main thread set de200, an
 * explicit context given the path alone, and G19's synthetic DE431 header
 * at 2000 Jan 7 in a directory without eop_1962_today.txt (-1) and in one
 * whose file has a day missing (-2), both made in $TMPDIR (or /tmp). 48
 * checks.
 *
 * Measured before the fix: all 48 crash (SIGSEGV). The upstream build
 * (SWE_UPSTREAM_COMPAT) crashed in 46, the interpolated houses passing
 * there, their ayanamsa's nutation interpolated from the nodes the houses'
 * own nutation of no flags builds (entry 134's unkeyed nodes); it passes
 * all 48 now, the fix being in both builds, as a crash's.
 *
 * And where the setting is adopted (UPSTREAM-BUGS 143): load_dpsi_deps()
 * ran in swe_set_jpl_file() alone, so a context taking the JPL file from
 * the shared configuration never loaded the corrections, and its
 * SEFLG_JPLHOR fell to SEFLG_JPLHOR_APPROX where the setter's did not. The
 * main thread sets path A (the fixture with eop_1962_today.txt) and the
 * fixture; SE_ECL_NUT with SEFLG_SPEED, swe_get_ayanamsa_ex() and
 * swe_houses_ex2() sidereal with the speeds, all with SEFLG_JPLEPH |
 * SEFLG_JPLHOR, at 2000 Jan 7 and then Jan 7.37, by a thread running
 * across the setter (started by the same call before it), the same with
 * the nutation interpolated, a thread started after it and a context made
 * after it, against a fresh process whose main thread set both and made
 * the calls. 12 checks, 60 in all.
 *
 * Measured before the fix, the 12 fail: dpsi -13.488712" for -13.530688"
 * (4.20e-2"; interpolated -13.489375" for -13.531351"), the return
 * SEFLG_JPLHOR_APPROX's, the ayanamsa and the cusps 4.20e-2" with it. The
 * upstream build failed 11, the interpolated houses passing there on entry
 * 134's unkeyed nodes; the adoption is this fork's, fixed in both builds,
 * and both pass. Run as
 *   eopfollow EPHEDIR
 */
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T0 2451545.0		/* J2000 */
#define T0F 2451550.5		/* 2000 Jan 7, in the fixtures' span */
#define NV 48
#define JH (SEFLG_JPLEPH | SEFLG_JPLHOR)
#define JPL_FIXTURE "de431fixture.eph"

static const char *EPHE;
static const double GEO[3] = { 8.55, 47.37, 400 };
/* EPHE, the fixtures: A, B (other corrections), N (none), G (a gap) */
enum { D_EPHE, D_A, D_B, D_N, D_G, ND };
static char DIRS[ND][512];

struct obs {
  int ret;
  double v[NV];
};

/* the probes of SEFLG_JPLHOR without the corrections (bug 142) */
enum { Q_AYA, Q_AYAUT, Q_AYASW, Q_HOUSE2, Q_HOUSE, Q_GQ0, Q_GQ1, Q_GQSTAR, NQ };
static const char *QN[NQ] = { "swe_get_ayanamsa_ex()",
  "swe_get_ayanamsa_ex_ut()", "swe_get_ayanamsa_ex(SEFLG_SWIEPH)",
  "swe_houses_ex2() sidereal", "swe_houses_ex() sidereal",
  "swe_gauquelin_sector() 0", "swe_gauquelin_sector() 1",
  "swe_gauquelin_sector() 0, Spica" };

/* probe q at t on c (or the default context for c NULL), with the flags
 * the probe takes, SEFLG_JPLHOR in them -- or SEFLG_JPLHOR_APPROX (apx) */
static void probe(swe_ctx *c, int q, double t, int apx, struct obs *o)
{
  char serr[AS_MAXCH], s[64];
  double geo[3];
  int32 fl = (q == Q_AYASW ? SEFLG_SWIEPH : SEFLG_JPLEPH)
             | (apx ? SEFLG_JPLHOR_APPROX : SEFLG_JPLHOR);
  memset(o, 0, sizeof *o);
  memcpy(geo, GEO, sizeof geo);
  switch (q) {
    case Q_AYA: case Q_AYASW:
      o->ret = c ? swe_get_ayanamsa_ex_r(c, t, fl, o->v, serr)
                 : swe_get_ayanamsa_ex(t, fl, o->v, serr);
      break;
    case Q_AYAUT:
      o->ret = c ? swe_get_ayanamsa_ex_ut_r(c, t, fl, o->v, serr)
                 : swe_get_ayanamsa_ex_ut(t, fl, o->v, serr);
      break;
    case Q_HOUSE2:
      o->ret = c ? swe_houses_ex2_r(c, t, fl | SEFLG_SIDEREAL, GEO[1], GEO[0], 'P',
                                    o->v, o->v + 13, o->v + 23, o->v + 36, serr)
                 : swe_houses_ex2(t, fl | SEFLG_SIDEREAL, GEO[1], GEO[0], 'P',
                                  o->v, o->v + 13, o->v + 23, o->v + 36, serr);
      break;
    case Q_HOUSE:
      o->ret = c ? swe_houses_ex_r(c, t, fl | SEFLG_SIDEREAL, GEO[1], GEO[0], 'K', o->v, o->v + 13)
                 : swe_houses_ex(t, fl | SEFLG_SIDEREAL, GEO[1], GEO[0], 'K', o->v, o->v + 13);
      break;
    case Q_GQ0: case Q_GQ1:
      o->ret = c ? swe_gauquelin_sector_r(c, t, SE_MARS, NULL, fl, q == Q_GQ1, geo, 0, 0, o->v, serr)
                 : swe_gauquelin_sector(t, SE_MARS, NULL, fl, q == Q_GQ1, geo, 0, 0, o->v, serr);
      break;
    case Q_GQSTAR:
      strcpy(s, "Spica");
      o->ret = c ? swe_gauquelin_sector_r(c, t, 0, s, fl, 0, geo, 0, 0, o->v, serr)
                 : swe_gauquelin_sector(t, 0, s, fl, 0, geo, 0, 0, o->v, serr);
      break;
  }
}

/* the settings without the corrections */
enum { X_DE200, X_DE200I, X_THREAD, X_CTX, X_NOEOP, X_GAP, NX };
static const char *XN[NX] = { "de200", "de200, interpolated",
  "de200, a second thread", "an explicit context, the path alone",
  "DE431 fixture, no eop_1962_today.txt", "DE431 fixture, a day missing" };

static int th_q, th_apx;
static double th_t;
static struct obs th_o;
static void *second(void *arg)
{
  (void) arg;
  probe(NULL, th_q, th_t, th_apx, &th_o);
  return NULL;
}

static void run1(int x, int q, int fresh, struct obs *o)
{
  char jf[64];
  int d = x == X_NOEOP ? D_N : x == X_GAP ? D_G : D_EPHE;
  double t = d == D_EPHE ? T0 : T0F;
  if (x == X_CTX) {
    swe_ctx *c = swe_ctx_new();
    if (c == NULL)
      exit(3);
    swe_set_ephe_path_r(c, DIRS[D_EPHE]);
    swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
    probe(c, q, t, fresh, o);
    swe_ctx_free(c);
    return;
  }
  strcpy(jf, d == D_EPHE ? "de200.eph" : JPL_FIXTURE);
  swe_set_ephe_path(DIRS[d]);
  swe_set_jpl_file(jf);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  if (x == X_DE200I)
    swe_set_interpolate_nut(1);
  if (x == X_THREAD && !fresh) {
    pthread_t th;
    th_q = q;
    th_t = t;
    th_apx = 0;
    if (pthread_create(&th, NULL, second, NULL) != 0)
      exit(3);
    pthread_join(th, NULL);
    *o = th_o;
    return;
  }
  probe(NULL, q, t, fresh, o);
}

/* The corrections where the setting is adopted (UPSTREAM-BUGS 143): the
 * main thread sets path A and the DE431 fixture, whose
 * swe_set_jpl_file() loads A's eop_1962_today.txt; then a thread started
 * before it, one started after it and a context made after it make the
 * calls. The probes with SEFLG_JPLEPH | SEFLG_JPLHOR (and SEFLG_SPEED for
 * SE_ECL_NUT) at 2000 Jan 7 and 7.37; the second is compared. */
enum { P_NUT, P_AYA, P_HOUSE, NP };
static const char *PN[NP] = { "SE_ECL_NUT", "swe_get_ayanamsa_ex()",
  "swe_houses_ex2() sidereal" };
static const int PV[NP] = { 2, 0, 1 };	/* dpsi, the ayanamsa, cusp 1 */
static void probe2(swe_ctx *c, int p, double t, struct obs *o)
{
  char serr[AS_MAXCH];
  memset(o, 0, sizeof *o);
  switch (p) {
    case P_NUT:
      o->ret = c ? swe_calc_r(c, t, SE_ECL_NUT, JH | SEFLG_SPEED, o->v, serr)
                 : swe_calc(t, SE_ECL_NUT, JH | SEFLG_SPEED, o->v, serr);
      break;
    case P_AYA:
      o->ret = c ? swe_get_ayanamsa_ex_r(c, t, JH, o->v, serr)
                 : swe_get_ayanamsa_ex(t, JH, o->v, serr);
      break;
    case P_HOUSE:
      o->ret = c ? swe_houses_ex2_r(c, t, JH | SEFLG_SIDEREAL, GEO[1], GEO[0], 'P',
                                    o->v, o->v + 13, o->v + 23, o->v + 36, serr)
                 : swe_houses_ex2(t, JH | SEFLG_SIDEREAL, GEO[1], GEO[0], 'P',
                                  o->v, o->v + 13, o->v + 23, o->v + 36, serr);
      break;
  }
}

/* the probe at T0F and T0F + 0.37, the second kept */
static void probe2x(swe_ctx *c, int p, struct obs *o)
{
  probe2(c, p, T0F, o);
  probe2(c, p, T0F + 0.37, o);
}

enum { A_THREAD, A_THREADI, A_NEWTHREAD, A_CTX, NA };
static const char *AN[NA] = { "a thread running across swe_set_jpl_file()",
  "the same, interpolated", "a thread started after it",
  "a context made after it" };

static pthread_barrier_t bar;
static int th_p;
static void *second2(void *arg)
{
  struct obs o;
  (void) arg;
  probe2(NULL, th_p, T0F, &o);	/* started, without the corrections */
  pthread_barrier_wait(&bar);
  pthread_barrier_wait(&bar);
  probe2x(NULL, th_p, &th_o);
  return NULL;
}
static void *second2new(void *arg)
{
  (void) arg;
  probe2x(NULL, th_p, &th_o);
  return NULL;
}

/* route a, or (fresh) the main thread with the final setting */
static void run2(int a, int p, int fresh, struct obs *o)
{
  char jf[64];
  pthread_t th;
  strcpy(jf, JPL_FIXTURE);
  swe_set_ephe_path(DIRS[D_A]);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  if (a == A_THREADI)
    swe_set_interpolate_nut(1);
  if (fresh) {
    swe_set_jpl_file(jf);
    probe2x(NULL, p, o);
    return;
  }
  th_p = p;
  switch (a) {
    case A_THREAD: case A_THREADI:
      pthread_barrier_init(&bar, NULL, 2);
      if (pthread_create(&th, NULL, second2, NULL) != 0)
        exit(3);
      pthread_barrier_wait(&bar);
      swe_set_jpl_file(jf);
      pthread_barrier_wait(&bar);
      pthread_join(th, NULL);
      *o = th_o;
      break;
    case A_NEWTHREAD:
      swe_set_jpl_file(jf);
      if (pthread_create(&th, NULL, second2new, NULL) != 0)
        exit(3);
      pthread_join(th, NULL);
      *o = th_o;
      break;
    case A_CTX: {
      swe_ctx *c;
      swe_set_jpl_file(jf);
      if ((c = swe_ctx_new()) == NULL)
        exit(3);
      probe2x(c, p, o);
      swe_ctx_free(c);
      break;
    }
  }
}

/* run(k, p, fresh) in a forked process: 0, -1 if it failed, or the signal
 * that ended it */
static int in_child(void (*run)(int, int, int, struct obs *), int k, int p,
                    int fresh, struct obs *o)
{
  int fd[2], st;
  pid_t pid;
  ssize_t got;
  fflush(stdout);
  if (pipe(fd) != 0)
    return -1;
  pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    struct obs r;
    close(fd[0]);
    run(k, p, fresh, &r);
    _exit(write(fd[1], &r, sizeof r) == (ssize_t) sizeof r ? 0 : 4);
  }
  close(fd[1]);
  got = read(fd[0], o, sizeof *o);
  close(fd[0]);
  if (waitpid(pid, &st, 0) != pid)
    return -1;
  if (WIFSIGNALED(st))
    return WTERMSIG(st);
  if (!WIFEXITED(st) || WEXITSTATUS(st) != 0 || got != (ssize_t) sizeof *o)
    return -1;
  return 0;
}

/* G19's synthetic JPL header (eopload.c), DE 431: fsizer() accepts it and
 * the file is the length it implies; the nutation reads no coefficient. */
static void put32(unsigned char *p, int v)    { memcpy(p, &v, 4); }
static void putd (unsigned char *p, double v) { memcpy(p, &v, 8); }
static int write_jpl(const char *path)
{
  unsigned char h[2856];
  const int ncf = 4, na = 1, maxoff = 1200, nrecl = 4;
  const double ss0 = 2451544.5, ss1 = 2451576.5, ss2 = 32.0;   /* nseg = 1 */
  int i, ksize, nseg = (int) ((ss1 - ss0) / ss2);
  long nb = 0;
  FILE *fp;
  memset(h, ' ', 252);
  memcpy(h, "SWISSEPH EOP LOADER FIXTURE", 27);
  memset(h + 252, ' ', 2400);
  putd (h + 2652, ss0);
  putd (h + 2660, ss1);
  putd (h + 2668, ss2);
  put32(h + 2676, 0);                 /* ncon  */
  putd (h + 2680, 149597870.7);       /* au    */
  putd (h + 2688, 81.3);              /* emrat */
  for (i = 0; i < 12; i++) {
    put32(h + 2696 + (i * 3 + 0) * 4, 3);
    put32(h + 2696 + (i * 3 + 1) * 4, ncf);
    put32(h + 2696 + (i * 3 + 2) * 4, na);
  }
  put32(h + 2696 + 30 * 4, maxoff);
  put32(h + 2696 + 31 * 4, ncf);
  put32(h + 2696 + 32 * 4, na);
  put32(h + 2840, 431);               /* numde */
  put32(h + 2844, 3);
  put32(h + 2848, ncf);
  put32(h + 2852, na);
  ksize = (maxoff + 3 * ncf * na - 1) * 2;
  for (i = 0; i < 13; i++)
    nb += (long) ncf * na * (i == 11 ? 2 : 3) * nseg;
  nb += 2 * nseg;
  nb *= 8;
  nb += 2 * (long) ksize * nrecl;
  if ((fp = fopen(path, "wb")) == NULL)
    return -1;
  fwrite(h, 1, sizeof h, fp);
  for (i = (int) sizeof h; i < nb; i++)
    fputc(0, fp);
  if ((fp = freopen(path, "r+b", fp)) == NULL)
    return -1;
  fseek(fp, 2L * ksize * nrecl, SEEK_SET);
  { double ep[2]; ep[0] = ss0; ep[1] = ss1; fwrite(ep, 8, 2, fp); }
  fclose(fp);
  return 0;
}

/* eop_1962_today.txt (EOPC04 columns) for MJD 51544..51583, dpsi and deps
 * (arcsec) a + i b and c + i d; day gap left out (0: none) */
static int write_eop(const char *path, double a, double b, double c, double d, int gap)
{
  FILE *fp = fopen(path, "w");
  int i;
  if (fp == NULL)
    return -1;
  fprintf(fp, "# EOPC04 fixture\n");
  for (i = 0; i < 40; i++)
    if (gap == 0 || i != gap)
      fprintf(fp, "2000  1 %2d  %5d   0.0 0.0 0.0 0.0 %.9f %.9f\n",
              i + 1, 51544 + i, a + i * b, c + i * d);
  return fclose(fp);
}

static const char *SUB[ND] = { NULL, "", "/B", "/N", "/G" };
static int make_fixtures(void)
{
  char p[600], base[400];
  const char *tmp = getenv("TMPDIR");
  int d;
  if (snprintf(base, sizeof base, "%s/eopfollow.XXXXXX", tmp && *tmp ? tmp : "/tmp")
      >= (int) sizeof base || mkdtemp(base) == NULL)
    return -1;
  for (d = D_A; d < ND; d++) {
    snprintf(DIRS[d], sizeof DIRS[d], "%s%s", base, SUB[d]);
    if (d != D_A && mkdir(DIRS[d], 0700) != 0)
      return -1;
    snprintf(p, sizeof p, "%s/%s", DIRS[d], JPL_FIXTURE);
    if (write_jpl(p) != 0)
      return -1;
  }
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", DIRS[D_A]);
  if (write_eop(p, -0.05, -1e-5, -0.005, -1e-6, 0) != 0)
    return -1;
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", DIRS[D_B]);
  if (write_eop(p, 0.08, 1e-4, 0.01, 1e-5, 0) != 0)
    return -1;
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", DIRS[D_G]);
  if (write_eop(p, -0.05, -1e-5, -0.005, -1e-6, 20) != 0)
    return -1;
  return 0;
}

static void remove_fixtures(void)
{
  char p[600];
  int d;
  for (d = ND - 1; d >= D_A; d--) {
    if (*DIRS[d] == '\0')
      continue;
    snprintf(p, sizeof p, "%s/%s", DIRS[d], JPL_FIXTURE);
    unlink(p);
    snprintf(p, sizeof p, "%s/eop_1962_today.txt", DIRS[d]);
    unlink(p);
    rmdir(DIRS[d]);
  }
}

/* how far a from b, and whether they differ at all (with the return) */
static int differ(const struct obs *a, const struct obs *b, double *d)
{
  int i, bad = a->ret != b->ret;
  *d = 0;
  for (i = 0; i < NV; i++) {
    if (a->v[i] != b->v[i])
      bad = 1;
    if (fabs(a->v[i] - b->v[i]) > *d)
      *d = fabs(a->v[i] - b->v[i]);
  }
  return bad;
}

int main(int argc, char **argv)
{
  int x, q, n = 0, nfail = 0;
  if (argc < 2) {
    fprintf(stderr, "usage: eopfollow EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  snprintf(DIRS[D_EPHE], sizeof DIRS[D_EPHE], "%s", EPHE);
  if (make_fixtures() != 0) {
    printf("G134 FAIL: cannot write the fixtures in %s\n", DIRS[D_A]);
    remove_fixtures();
    return 1;
  }
  for (x = 0; x < NX; x++)
    for (q = 0; q < NQ; q++) {
      struct obs ot, of;
      double d;
      int r;
      n++;
      if ((r = in_child(run1, x, q, 0, &ot)) != 0) {
        if (r > 0)
          printf("    %s, %s with SEFLG_JPLHOR: signal %d FAIL\n", XN[x], QN[q], r);
        else
          printf("    %s, %s with SEFLG_JPLHOR: child failed\n", XN[x], QN[q]);
        nfail++;
        continue;
      }
      if (in_child(run1, x, q, 1, &of) != 0) {
        printf("    %s, %s with SEFLG_JPLHOR_APPROX: child failed\n", XN[x], QN[q]);
        nfail++;
        continue;
      }
      if (differ(&ot, &of, &d)) {
        printf("    %s, %s: %.3e off SEFLG_JPLHOR_APPROX's (return %d, %d) FAIL\n",
               XN[x], QN[q], d, ot.ret, of.ret);
        nfail++;
      }
    }
  for (x = 0; x < NA; x++)
    for (q = 0; q < NP; q++) {
      struct obs ot, of;
      double d;
      n++;
      if (in_child(run2, x, q, 0, &ot) != 0 || in_child(run2, x, q, 1, &of) != 0) {
        printf("    %s, %s: child failed\n", AN[x], PN[q]);
        nfail++;
        continue;
      }
      if (differ(&ot, &of, &d)) {
        printf("    %s, %s: %.3e off the setter's (%.6f\" for %.6f\", return %d, %d) FAIL\n",
               AN[x], PN[q], d, ot.v[PV[q]] * 3600, of.v[PV[q]] * 3600, ot.ret, of.ret);
        nfail++;
      }
    }
  remove_fixtures();
  if (nfail)
    printf("G134 FAIL: %d of %d\n", nfail, n);
  else
    printf("G134 PASS (%d)\n", n);
  return nfail ? 1 : 0;
}
