/* golden.c -- bit-exact regression baseline for Swiss Ephemeris.
 *
 * Dumps every result as %a (C99 hex float) so that ANY change in the
 * numerical output is visible as a byte diff.  Used to prove that the
 * thread-safety refactor is numerically a no-op.
 *
 * Build:  cc -O0 -I.. golden.c ../*.c -lm -ldl -o golden
 * Use:    ./golden > baseline.txt   (before)
 *         ./golden | diff baseline.txt -   (after; must be empty)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "swephexp.h"

#define DEFAULT_EPHE "../ephe"
static const char *EPHE = DEFAULT_EPHE;

/* Per-thread output sink so N threads can each produce a full transcript. */
static __thread FILE *OUT;

/* serr may embed the absolute ephemeris path; strip it so baselines are
 * portable across machines and checkouts. */
static void sanitize(char *d, size_t n, const char *s) {
  size_t el = strlen(EPHE), j = 0;
  for (size_t i = 0; s[i] && j + 8 < n; ) {
    if (el && strncmp(s + i, EPHE, el) == 0) {
      memcpy(d + j, "$EPHE", 5); j += 5; i += el;
    } else d[j++] = s[i++];
  }
  d[j] = 0;
}

/* Dates spanning the ephemeris range, incl. pre/post file boundaries. */
static const double DATES[] = {
  1356173.5,   /* -3000 Jan 1  */
  1721423.5,   /*     1 Jan 1  */
  2195883.5,   /*  1300 Jan 1  */
  2415020.5,   /*  1900 Jan 1  */
  2451545.0,   /*  2000 Jan 1.5 (J2000) */
  2451545.123456789,
  2469807.5,   /*  2050 Jan 1  */
  2488070.5,   /*  2100 Jan 1  */
  2816787.5,   /*  3000 Jan 1  */
};
#define NDATES (sizeof(DATES)/sizeof(DATES[0]))

static const int32 FLAGS[] = {
  SEFLG_SWIEPH,
  SEFLG_SWIEPH|SEFLG_SPEED,
  SEFLG_SWIEPH|SEFLG_SPEED|SEFLG_EQUATORIAL,
  SEFLG_SWIEPH|SEFLG_SPEED|SEFLG_XYZ,
  SEFLG_SWIEPH|SEFLG_HELCTR,
  SEFLG_SWIEPH|SEFLG_BARYCTR,
  SEFLG_SWIEPH|SEFLG_J2000,
  SEFLG_SWIEPH|SEFLG_TRUEPOS,
  SEFLG_SWIEPH|SEFLG_NOABERR|SEFLG_NOGDEFL,
  SEFLG_MOSEPH|SEFLG_SPEED,
  SEFLG_JPLEPH|SEFLG_SPEED,
};
#define NFLAGS (sizeof(FLAGS)/sizeof(FLAGS[0]))

static void row(const char *tag, int32 rf, double *x, int n, const char *serr) {
  char cl[AS_MAXCH * 2];
  fprintf(OUT, "%-46s rf=%-6d", tag, rf);
  for (int i = 0; i < n; i++) fprintf(OUT, " %a", x[i]);
  if (serr && *serr) { sanitize(cl, sizeof cl, serr); fprintf(OUT, " | %s", cl); }
  fprintf(OUT, "\n");
}

static void planets(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256];
  for (size_t d = 0; d < NDATES; d++)
    for (size_t f = 0; f < NFLAGS; f++)
      for (int p = SE_SUN; p <= SE_VESTA; p++) {
        serr[0] = 0; memset(x, 0, sizeof x);
        int32 rf = swe_calc(DATES[d], p, FLAGS[f], x, serr);
        snprintf(tag, sizeof tag, "calc[%zu,%zu,%d]", d, f, p);
        row(tag, rf, x, 6, serr);
      }
}

static void planets_ut(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256];
  for (size_t d = 0; d < NDATES; d++)
    for (int p = SE_SUN; p <= SE_CHIRON; p++) {
      serr[0] = 0; memset(x, 0, sizeof x);
      int32 rf = swe_calc_ut(DATES[d], p, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
      snprintf(tag, sizeof tag, "calc_ut[%zu,%d]", d, p);
      row(tag, rf, x, 6, serr);
    }
}

static void asteroids(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256];
  const int ast[] = {1, 2, 3, 4, 433, 1221, 5145};
  for (size_t d = 0; d < NDATES; d++)
    for (size_t i = 0; i < sizeof(ast)/sizeof(ast[0]); i++) {
      serr[0] = 0; memset(x, 0, sizeof x);
      int32 rf = swe_calc(DATES[d], SE_AST_OFFSET + ast[i],
                          SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
      snprintf(tag, sizeof tag, "ast[%zu,%d]", d, ast[i]);
      row(tag, rf, x, 6, serr);
    }
}

static void sidereal(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256];
  for (int sid = 0; sid <= 40; sid++) {
    swe_set_sid_mode(sid, 0, 0);
    for (size_t d = 0; d < NDATES; d++) {
      double ay = swe_get_ayanamsa_ut(DATES[d]);
      snprintf(tag, sizeof tag, "ayan[%d,%zu]", sid, d);
      row(tag, 0, &ay, 1, NULL);
      serr[0] = 0; memset(x, 0, sizeof x);
      int32 rf = swe_calc_ut(DATES[d], SE_MOON,
                             SEFLG_SWIEPH|SEFLG_SIDEREAL|SEFLG_SPEED, x, serr);
      snprintf(tag, sizeof tag, "sidmoon[%d,%zu]", sid, d);
      row(tag, rf, x, 6, serr);
    }
  }
  swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
}

static void topo(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256];
  const double geo[][3] = {{8.55,47.37,400},{-74.0,40.7,10},{139.7,35.7,40},{0,90,0},{0,-89.9,0}};
  for (size_t g = 0; g < sizeof(geo)/sizeof(geo[0]); g++) {
    swe_set_topo(geo[g][0], geo[g][1], geo[g][2]);
    for (size_t d = 0; d < NDATES; d++)
      for (int p = SE_SUN; p <= SE_MARS; p++) {
        serr[0] = 0; memset(x, 0, sizeof x);
        int32 rf = swe_calc_ut(DATES[d], p, SEFLG_SWIEPH|SEFLG_TOPOCTR|SEFLG_SPEED, x, serr);
        snprintf(tag, sizeof tag, "topo[%zu,%zu,%d]", g, d, p);
        row(tag, rf, x, 6, serr);
      }
  }
}

static void houses(void) {
  double cusp[37], ascmc[10], csp[37], asp[10]; char tag[256];
  const char *hsys = "PKORCAEVXHTBGWMNQLIUSDFY";
  const double lats[] = {0.0, 47.37, 60.0, 66.5, 75.0, -33.9};
  for (const char *h = hsys; *h; h++)
    for (size_t la = 0; la < sizeof(lats)/sizeof(lats[0]); la++)
      for (size_t d = 0; d < NDATES; d++) {
        memset(cusp,0,sizeof cusp); memset(ascmc,0,sizeof ascmc);
        memset(csp,0,sizeof csp);   memset(asp,0,sizeof asp);
        ascmc[9] = 99;   /* sunshine houses sentinel */
        int rc = swe_houses_ex2(DATES[d], SEFLG_SWIEPH, lats[la], 8.55,
                                *h, cusp, ascmc, csp, asp, NULL);
        snprintf(tag, sizeof tag, "hsys[%c,%zu,%zu]", *h, la, d);
        fprintf(OUT, "%-46s rc=%-4d", tag, rc);
        for (int i = 0; i < 13; i++) fprintf(OUT, " %a", cusp[i]);
        for (int i = 0; i < 10; i++) fprintf(OUT, " %a", ascmc[i]);
        fprintf(OUT, "\n");
      }
}

static void fixstars(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256]; char nm[AS_MAXCH];
  const char *stars[] = {"Aldebaran","Regulus","Sirius","Spica","Antares",
                         "Algol","Polaris","Galactic Center","Alcyone","Vega"};
  for (size_t s = 0; s < sizeof(stars)/sizeof(stars[0]); s++)
    for (size_t d = 0; d < NDATES; d++) {
      serr[0]=0; memset(x,0,sizeof x); strcpy(nm, stars[s]);
      int32 rf = swe_fixstar2_ut(nm, DATES[d], SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
      snprintf(tag, sizeof tag, "star[%s,%zu]", stars[s], d);
      row(tag, rf, x, 6, serr);
      double mag = 0; strcpy(nm, stars[s]);
      swe_fixstar2_mag(nm, &mag, serr);
      snprintf(tag, sizeof tag, "starmag[%s]", stars[s]);
      if (d == 0) row(tag, 0, &mag, 1, NULL);
    }
}

static void timeconv(void) {
  char serr[AS_MAXCH]; char tag[256]; double v[4];
  for (size_t d = 0; d < NDATES; d++) {
    serr[0]=0;
    v[0] = swe_deltat_ex(DATES[d], SEFLG_SWIEPH, serr);
    v[1] = swe_sidtime(DATES[d]);
    v[2] = swe_julday(2000, 1, 1, 12.0 + d, SE_GREG_CAL);
    v[3] = swe_get_tid_acc();
    snprintf(tag, sizeof tag, "time[%zu]", d);
    row(tag, 0, v, 4, serr);
    double e[6]; memset(e,0,sizeof e);
    swe_calc(DATES[d], SE_ECL_NUT, 0, e, serr);
    snprintf(tag, sizeof tag, "eclnut[%zu]", d);
    row(tag, 0, e, 6, serr);
  }
}

static void eclipses(void) {
  char serr[AS_MAXCH]; double tret[10], attr[20]; char tag[256];
  double geo[3] = {8.55, 47.37, 400};
  double tjd = 2451545.0;
  for (int i = 0; i < 6; i++) {
    serr[0]=0; memset(tret,0,sizeof tret); memset(attr,0,sizeof attr);
    int32 rf = swe_sol_eclipse_when_glob(tjd, SEFLG_SWIEPH, 0, tret, 0, serr);
    snprintf(tag, sizeof tag, "solecl_glob[%d]", i);
    row(tag, rf, tret, 10, serr);
    if (rf < 0) break;
    tjd = tret[0] + 10;
    serr[0]=0; memset(attr,0,sizeof attr);
    rf = swe_sol_eclipse_how(tret[0], SEFLG_SWIEPH, geo, attr, serr);
    snprintf(tag, sizeof tag, "solecl_how[%d]", i);
    row(tag, rf, attr, 11, serr);
  }
  tjd = 2451545.0;
  for (int i = 0; i < 6; i++) {
    serr[0]=0; memset(tret,0,sizeof tret); memset(attr,0,sizeof attr);
    int32 rf = swe_lun_eclipse_when(tjd, SEFLG_SWIEPH, 0, tret, 0, serr);
    snprintf(tag, sizeof tag, "lunecl[%d]", i);
    row(tag, rf, tret, 10, serr);
    if (rf < 0) break;
    tjd = tret[0] + 10;
  }
  serr[0]=0; memset(tret,0,sizeof tret);
  int32 rf = swe_rise_trans(2451545.0, SE_SUN, NULL, SEFLG_SWIEPH,
                            SE_CALC_RISE, geo, 1013.25, 15.0, tret, serr);
  row("rise_trans", rf, tret, 1, serr);
  serr[0]=0;
  double xn[4];
  rf = swe_nod_aps_ut(2451545.0, SE_MOON, SEFLG_SWIEPH, SE_NODBIT_MEAN,
                      xn, xn, xn, xn, serr);
  row("nod_aps", rf, xn, 4, serr);
}

static void pheno(void) {
  char serr[AS_MAXCH]; double attr[20]; char tag[256];
  for (size_t d = 0; d < NDATES; d++)
    for (int p = SE_SUN; p <= SE_SATURN; p++) {
      serr[0]=0; memset(attr,0,sizeof attr);
      int32 rf = swe_pheno_ut(DATES[d], p, SEFLG_SWIEPH, attr, serr);
      snprintf(tag, sizeof tag, "pheno[%zu,%d]", d, p);
      row(tag, rf, attr, 11, serr);
    }
}

/* Runs the entire suite against the current thread's library state. */
static void suite(void) {
  char sv[AS_MAXCH];
  fprintf(OUT, "# swe_version=%s\n", swe_version(sv));
  planets();
  planets_ut();
  asteroids();
  sidereal();
  topo();
  houses();
  fixstars();
  timeconv();
  eclipses();
  pheno();
}

struct targ { int id; char *buf; size_t len; int setup; };

static void *thread_suite(void *p) {
  struct targ *a = p;
  OUT = open_memstream(&a->buf, &a->len);
  /* a->setup mirrors what a well-behaved caller does on a worker thread.
   * 0 = configure only on the main thread (the pyswisseph pattern).
   * 1 = re-apply configuration on every worker thread (today's workaround). */
  if (a->setup) swe_set_ephe_path((char *)EPHE);
  suite();
  fclose(OUT);
  return NULL;
}

int main(int argc, char **argv) {
  int nthreads = 0, setup = 0;
  const char *e = getenv("SE_TEST_EPHE");
  if (e && *e) EPHE = e;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--threads") && i + 1 < argc) nthreads = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--per-thread-setup")) setup = 1;
    else if (!strcmp(argv[i], "--ephe") && i + 1 < argc) EPHE = argv[++i];
    else { fprintf(stderr, "usage: %s [--ephe DIR] [--threads N [--per-thread-setup]]\n", argv[0]); return 2; }
  }

  swe_set_ephe_path((char *)EPHE);

  if (nthreads <= 0) {                 /* baseline mode: dump to stdout */
    OUT = stdout;
    suite();
    swe_close();
    return 0;
  }

  /* thread-consistency mode: every thread must reproduce the main-thread
   * transcript byte for byte. */
  struct targ ref = { -1, NULL, 0, 1 };
  thread_suite(&ref);

  struct targ *a = calloc(nthreads, sizeof *a);
  pthread_t *t = calloc(nthreads, sizeof *t);
  for (int i = 0; i < nthreads; i++) {
    a[i].id = i; a[i].setup = setup;
    pthread_create(&t[i], NULL, thread_suite, &a[i]);
  }
  for (int i = 0; i < nthreads; i++) pthread_join(t[i], NULL);

  if (getenv("SE_DUMP")) {
    FILE *f = fopen("/tmp/ref.txt","w"); fwrite(ref.buf,1,ref.len,f); fclose(f);
    for (int i = 0; i < nthreads; i++) { char p[64]; snprintf(p,sizeof p,"/tmp/thr%d.txt",i);
      f=fopen(p,"w"); fwrite(a[i].buf,1,a[i].len,f); fclose(f); }
  }
  int bad = 0;
  for (int i = 0; i < nthreads; i++) {
    if (a[i].len != ref.len || memcmp(a[i].buf, ref.buf, ref.len) != 0) {
      bad++;
      /* report the first differing line */
      size_t off = 0; int line = 1;
      size_t lim = a[i].len < ref.len ? a[i].len : ref.len;
      while (off < lim && a[i].buf[off] == ref.buf[off]) { if (ref.buf[off]=='\n') line++; off++; }
      char rl[300] = "", tl[300] = "";
      sscanf(ref.buf + (off - (off ? 0 : 0)), "%299[^\n]", rl);
      sscanf(a[i].buf + off, "%299[^\n]", tl);
      fprintf(stderr, "thread %d: MISMATCH at line ~%d\n  main  : ...%.120s\n  thread: ...%.120s\n",
              i, line, rl, tl);
    }
  }
  fprintf(stderr, "%d/%d threads matched the main-thread transcript%s\n",
          nthreads - bad, nthreads, setup ? " (--per-thread-setup)" : "");
  swe_close();
  return bad ? 1 : 0;
}
