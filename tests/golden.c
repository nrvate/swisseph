/* golden.c -- bit-exact regression baseline for Swiss Ephemeris.
 *
 * Dumps every result as %a (C99 hex float) so that ANY change in the
 * numerical output is visible as a byte diff.  Used to prove that the
 * thread-safety refactor is numerically a no-op.
 *
 * Build:  see tests/Makefile
 * Use:    ./golden > baseline.txt   (before)
 *         ./golden | diff baseline.txt -   (after; must be empty)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Portable thread shim, same as tests/threadshim.c and tests/ctxtest.c.
 *
 * golden.c hardcoded <pthread.h>, which MSVC does not have -- so the one
 * transcript that verifies the library's NUMBERS could not be built on
 * Windows at all. Windows shipped a DLL whose only numerical check was a
 * smoke test asserting the Sun is near 280 degrees.
 *
 * It also means G2 (worker threads agree with the main thread) can now run
 * against the SRWLOCK backend, which no other platform exercises. */
#if defined(_WIN32)
# include <windows.h>
  typedef HANDLE thr_t;
  typedef DWORD  thr_ret_t;
# define THR_CALL __stdcall
  static int thr_create(thr_t *t, thr_ret_t (THR_CALL *fn)(void *), void *arg) {
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) fn, arg, 0, NULL);
    return *t == NULL;
  }
  static void thr_join(thr_t t) { WaitForSingleObject(t, INFINITE); CloseHandle(t); }
#else
# include <pthread.h>
  typedef pthread_t thr_t;
  typedef void *    thr_ret_t;
# define THR_CALL
  static int thr_create(thr_t *t, thr_ret_t (THR_CALL *fn)(void *), void *arg) {
    return pthread_create(t, NULL, fn, arg);
  }
  static void thr_join(thr_t t) { pthread_join(t, NULL); }
#endif
#include "swephexp.h"

#define DEFAULT_EPHE "../ephe"
static const char *EPHE = DEFAULT_EPHE;

/* Per-thread output sink, so N workers can each produce a full transcript.
 *
 * Named TRANSCRIPT, not OUT. `windows.h` defines OUT as an EMPTY macro --
 * it is one of the SAL parameter annotations in winnt.h -- so on MSVC the
 * declaration below expanded to `__declspec(thread) FILE *;` and every use
 * `fprintf(TRANSCRIPT, ...)` became `fprintf(, ...)`. That produced a C2059 here
 * and around thirty cascading errors at the call sites, all of which look
 * like a broken thread-local and are nothing of the kind.
 *
 * TLS rather than a bare __thread: that spelling is GCC/clang only, and
 * sweodef.h already picks __declspec(thread) for MSVC. */
static TLS FILE *TRANSCRIPT;

/* Normalise serr for the transcript.
 *
 * Two things have to go:
 *  - the absolute ephemeris path, so baselines are portable across machines;
 *  - embedded NEWLINES. The library emits multi-line messages such as
 *    "...not found in PATH '...'\ntrying Swiss Eph; ", which broke the
 *    one-row-per-line invariant this format depends on: 151 continuation
 *    lines all keyed on the token "trying", so a comparator keyed by first
 *    token silently collapsed them to one. Runs of whitespace become a
 *    single space. */
/* The ephemeris path is rewritten to $EPHE so the transcript does not depend
 * on where it was run, and the result is compared across platforms.
 *
 * Two things that has to get right, both learnt from MSVC:
 *
 * The match must be a whole path element. The Linux jobs pass "../ephe", but
 * the MSVC job passes "ephe", and a blind substring replace turns "Chiron's
 * ephemeris" into "Chiron's $EPHEmeris" and "swe_set_ephe_fallback(1)" into
 * "swe_set_$EPHE_fallback(1)". So the character on each side must be one an
 * identifier cannot contain -- a quote or a separator, not a letter, digit
 * or underscore.
 *
 * A backslash is a path separator here and nothing else -- no message in this
 * transcript contains one for any other reason -- so it becomes '/'. Without
 * that, every message carrying a path differs from the Linux spelling, both
 * after $EPHE and inside file names swi_gen_filename() built with DIR_GLUE. */
static int path_boundary(char c) {
  return !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
           || (c >= '0' && c <= '9') || c == '_');
}

static void sanitize(char *d, size_t n, const char *s) {
  size_t el = strlen(EPHE), j = 0;
  for (size_t i = 0; s[i] && j + 8 < n; ) {
    if (el && strncmp(s + i, EPHE, el) == 0
        && (i == 0 || path_boundary(s[i - 1])) && path_boundary(s[i + el])) {
      memcpy(d + j, "$EPHE", 5); j += 5; i += el;
    } else if (s[i] == '\n' || s[i] == '\r' || s[i] == '\t') {
      d[j++] = ' '; i++;
    } else if (s[i] == '\\') {
      d[j++] = '/'; i++;
    } else d[j++] = s[i++];
  }
  d[j] = 0;
}

/* Row tags are whitespace-delimited keys in the transcript, so a tag may not
 * contain a space. "Galactic Center" did: all nine of its rows shared the key
 * "star[Galactic" and eight were silently dropped by the comparator -- and
 * the ~illcond marker, which sits after the space, was never seen either. */
static void notag_space(char *s) {
  for (; *s; s++) if (*s == ' ') *s = '_';
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
  /* No SEFLG_JPLEPH. It never reached JPL -- no .eph is shipped, so all 189
   * of those rows were Swiss results recorded under a JPL label. Real JPL is
   * tested by tests/jplreal.c (G15) against a real file; the refusal a
   * missing one now produces is recorded by cov:set_jpl_file below. */
};
#define NFLAGS (sizeof(FLAGS)/sizeof(FLAGS[0]))

/* Some quantities this suite records are numerically ill-conditioned: a
 * change of a couple of ULP in the INPUT moves the output by arcseconds.
 * They are perfectly reproducible for a fixed binary, so they stay in the
 * bit-exact baseline, but they are NOT reproducible across math libraries
 * and must be skipped when comparing transcripts from different toolchains.
 *
 * Rather than teach cmpgolden.py a list of row/field coordinates -- which
 * would silently rot the moment FLAGS[] or a loop bound here changed -- the
 * rows tag THEMSELVES with this marker, and cmpgolden.py skips any row whose
 * tag contains it.
 *
 * Which rows get tagged was decided by MEASUREMENT, not by guessing from the
 * failures. Perturb the input tjd by +2 ULP and take the worst movement over
 * all nine dates:
 *
 *   body         via Moshier          via SWIEPH        ratio
 *   TRUE_NODE     1.42     arcsec      0.0000092         154,000x   <-- tagged
 *   OSCU_APOG    17.4      arcsec      0.000012        1,440,000x   <-- tagged
 *   MEAN_NODE     0.00000024           same                    1x
 *   MEAN_APOG     0.00000072           same                    1x
 *   INTP_APOG     0.0000205            same                    1x
 *   INTP_PERG     0.00119              same                    1x
 *   MOON          0.0000483            0.108
 *
 * Only the two OSCULATING lunar elements are affected, and only under
 * Moshier: they are derived by solving Kepler elements from the Moon's
 * analytic position and velocity, and recovering node/apse direction from a
 * near-circular osculating orbit is inherently unstable. The mean and
 * interpolated elements are analytic and identical either way.
 *
 * Also tagged: the fixed-star distance SPEED (field[5]), extracted by
 * differencing a distance of ~1e7 AU to recover a quantity ~3e-10 of it.
 *
 * A SECOND measurement, of a different sensitivity. The table above perturbs
 * the input; a compiler that contracts a*b+c into a fused multiply-add
 * perturbs every intermediate instead, and the two do not rank the same
 * rows. Apple clang on arm64 contracts by default, and the macOS CI job
 * reported OSCU_APOG under SWIEPH at 5.7e-06 -- 57% of the 1e-05 tolerance,
 * untagged. Reproduced on x86 with -O2 -mfma -ffp-contract=fast against
 * the bit-exact baseline, worst untagged rows:
 *
 *   OSCU_APOG  swieph/equat  speed (field 3)   2.7e-05   OVER, 2.7x   <-- tagged
 *   OSCU_APOG  swieph/equat  speed (field 4)   6.1e-06
 *   MOON       moseph        speed (field 3)   3.5e-06   many dates
 *   everything else                            < 1.6e-06
 *
 * So the osculating apogee is tagged under EVERY ephemeris: its position is
 * fine, but its speed is a difference of two nearly equal unstable
 * solutions and inherits their noise. The Moshier Moon's speed is left in
 * at ~3x headroom; if a toolchain moves it past tolerance, that is the row
 * to look at first.
 */
#define ILLCOND "~illcond"

/* Bodies whose values are not reproducible across math libraries -- see the
 * measurements above. The osculating apogee under any ephemeris; the true
 * node only under Moshier. */
static int is_illcond(int32 iflag, int ipl) {
  if (ipl == SE_OSCU_APOG)
    return 1;
  return (iflag & SEFLG_MOSEPH) && ipl == SE_TRUE_NODE;
}

/* The library version goes in the transcript as provenance, but must NOT
 * gate the comparison: bumping SE_VERSION would otherwise read as a
 * numerical regression and force a baseline regeneration that hides real
 * changes in the noise. The value is written to stderr, where it is visible
 * when running by hand, and a fixed placeholder goes into the transcript. */
static void emit_version(void) {
  char sv[AS_MAXCH];
  swe_version(sv);
  fprintf(stderr, "swe_version=%s\n", sv);
  fprintf(TRANSCRIPT, "# swe_version=<not compared; see stderr>\n");
}

static void row(const char *tag, int32 rf, double *x, int n, const char *serr) {
  char cl[AS_MAXCH * 2];
  fprintf(TRANSCRIPT, "%-46s rf=%-6d", tag, rf);
  for (int i = 0; i < n; i++) fprintf(TRANSCRIPT, " %a", x[i]);
  if (serr && *serr) { sanitize(cl, sizeof cl, serr); fprintf(TRANSCRIPT, " | %s", cl); }
  fprintf(TRANSCRIPT, "\n");
}

static void planets(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256];
  for (size_t d = 0; d < NDATES; d++)
    for (size_t f = 0; f < NFLAGS; f++)
      for (int p = SE_SUN; p <= SE_VESTA; p++) {
        serr[0] = 0; memset(x, 0, sizeof x);
        int32 rf = swe_calc(DATES[d], p, FLAGS[f], x, serr);
        snprintf(tag, sizeof tag, "calc[%zu,%zu,%d]%s", d, f, p,
                 is_illcond(FLAGS[f], p) ? ILLCOND : "");
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
        fprintf(TRANSCRIPT, "%-46s rc=%-4d", tag, rc);
        for (int i = 0; i < 13; i++) fprintf(TRANSCRIPT, " %a", cusp[i]);
        for (int i = 0; i < 10; i++) fprintf(TRANSCRIPT, " %a", ascmc[i]);
        fprintf(TRANSCRIPT, "\n");
      }
}

/* Sunshine houses (hsys 'I') carry cross-call state in saved_sundec.
 * swe_houses_ex2() always supplies ascmc[9] itself, so it only ever WRITES
 * that state; the read path is reachable only by calling
 * swe_houses_armc_ex2() directly with ascmc[9] == 99.  Both branches are
 * pinned here, in a fixed order, because the read path's result depends on
 * what the preceding call stored. */
static void sunshine(void) {
  double cusp[37], ascmc[10], csp[37], asp[10]; char serr[AS_MAXCH]; char tag[256];
  const double decs[] = {23.44, -23.44, 0.0, 12.5, -8.25};
  const double lats[] = {0.0, 47.37, 60.0, -33.9};
  const double armcs[] = {0.0, 90.0, 180.0, 271.3};
  const double eps = 23.4392911;
  int step = 0;
  for (size_t i = 0; i < sizeof(decs)/sizeof(decs[0]); i++)
    for (size_t la = 0; la < sizeof(lats)/sizeof(lats[0]); la++)
      for (size_t am = 0; am < sizeof(armcs)/sizeof(armcs[0]); am++) {
        /* (a) explicit declination -> takes the WRITE branch */
        memset(cusp,0,sizeof cusp); memset(ascmc,0,sizeof ascmc);
        memset(csp,0,sizeof csp);   memset(asp,0,sizeof asp);
        serr[0] = 0;
        ascmc[9] = decs[i];
        int rc = swe_houses_armc_ex2(armcs[am], lats[la], eps, 'I',
                                     cusp, ascmc, csp, asp, serr);
        snprintf(tag, sizeof tag, "sun_set[%d]", step);
        fprintf(TRANSCRIPT, "%-46s rc=%-4d", tag, rc);
        for (int k = 0; k < 13; k++) fprintf(TRANSCRIPT, " %a", cusp[k]);
        fprintf(TRANSCRIPT, " | dec=%a %s\n", ascmc[9], serr);

        /* (b) sentinel 99 -> takes the READ branch, must recover (a)'s value */
        memset(cusp,0,sizeof cusp); memset(ascmc,0,sizeof ascmc);
        memset(csp,0,sizeof csp);   memset(asp,0,sizeof asp);
        serr[0] = 0;
        ascmc[9] = 99;
        rc = swe_houses_armc_ex2(armcs[am], lats[la], eps, 'I',
                                 cusp, ascmc, csp, asp, serr);
        snprintf(tag, sizeof tag, "sun_recall[%d]", step);
        fprintf(TRANSCRIPT, "%-46s rc=%-4d", tag, rc);
        for (int k = 0; k < 13; k++) fprintf(TRANSCRIPT, " %a", cusp[k]);
        fprintf(TRANSCRIPT, " | dec=%a %s\n", ascmc[9], serr);
        step++;
      }
  /* out-of-range declination must still be rejected */
  memset(cusp,0,sizeof cusp); memset(ascmc,0,sizeof ascmc); serr[0]=0;
  ascmc[9] = 45.0;
  int rc = swe_houses_armc_ex2(0.0, 47.37, eps, 'I', cusp, ascmc, NULL, NULL, serr);
  fprintf(TRANSCRIPT, "%-46s rc=%-4d | %s\n", "sun_badrange", rc, serr);
}

static void fixstars(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256]; char nm[AS_MAXCH];
  const char *stars[] = {"Aldebaran","Regulus","Sirius","Spica","Antares",
                         "Algol","Polaris","Galactic Center","Alcyone","Vega"};
  for (size_t s = 0; s < sizeof(stars)/sizeof(stars[0]); s++)
    for (size_t d = 0; d < NDATES; d++) {
      serr[0]=0; memset(x,0,sizeof x); strcpy(nm, stars[s]);
      int32 rf = swe_fixstar2_ut(nm, DATES[d], SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
      /* field[5] is the star's distance SPEED, extracted by differencing a
       * distance of ~1e7 AU to recover a quantity ~3e-10 of it -- see the
       * ILLCOND note above and cmpgolden.py's header. */
      snprintf(tag, sizeof tag, "star[%s,%zu]%s", stars[s], d, ILLCOND);
      notag_space(tag);
      row(tag, rf, x, 6, serr);
      double mag = 0; strcpy(nm, stars[s]);
      swe_fixstar2_mag(nm, &mag, serr);
      snprintf(tag, sizeof tag, "starmag[%s]", stars[s]);
      notag_space(tag);
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

    /* A fixed observer is almost never inside the eclipse path, so
     * swe_sol_eclipse_how() there returns 0 and never reaches the saros
     * lookup. Locate the point of maximum eclipse first, then ask "how"
     * THERE -- that exercises swe_sol_eclipse_where() and populates
     * attr[9]/attr[10] from saros_data_solar[]. */
    serr[0]=0; memset(attr,0,sizeof attr);
    double gmax[2] = {0, 0}, attrw[20];
    memset(attrw, 0, sizeof attrw);
    int32 rw = swe_sol_eclipse_where(tret[0], SEFLG_SWIEPH, gmax, attrw, serr);
    snprintf(tag, sizeof tag, "solecl_where[%d]", i);
    row(tag, rw, attrw, 11, serr);

    double gm[3] = { gmax[0], gmax[1], 0 };
    serr[0]=0; memset(attr,0,sizeof attr);
    rf = swe_sol_eclipse_how(tret[0], SEFLG_SWIEPH, gm, attr, serr);
    snprintf(tag, sizeof tag, "solecl_how_max[%d]", i);   /* saros in attr[9] */
    row(tag, rf, attr, 11, serr);

    /* keep the fixed-observer case too: it pins the not-visible path */
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
    /* lunar saros lives in saros_data_lunar[], filled by lun_eclipse_how */
    serr[0]=0; memset(attr,0,sizeof attr);
    int32 rh = swe_lun_eclipse_how(tret[0], SEFLG_SWIEPH, geo, attr, serr);
    snprintf(tag, sizeof tag, "lunecl_how[%d]", i);
    row(tag, rh, attr, 11, serr);
    tjd = tret[0] + 10;
  }
  serr[0]=0; memset(tret,0,sizeof tret);
  int32 rf = swe_rise_trans(2451545.0, SE_SUN, NULL, SEFLG_SWIEPH,
                            SE_CALC_RISE, geo, 1013.25, 15.0, tret, serr);
  row("rise_trans", rf, tret, 1, serr);
  serr[0]=0;
  /* swe_nod_aps() writes SIX doubles to EACH of its four output arrays
   * (swecl.c:5637-5641, `for (i = 0; i <= 5; i++)`). The first version of
   * this passed a single double[4] aliased four ways -- a stack buffer
   * overflow, caught by AddressSanitizer in CI rather than here. */
  double xasc[6], xdsc[6], xper[6], xaph[6];
  memset(xasc,0,sizeof xasc); memset(xdsc,0,sizeof xdsc);
  memset(xper,0,sizeof xper); memset(xaph,0,sizeof xaph);
  rf = swe_nod_aps_ut(2451545.0, SE_MOON, SEFLG_SWIEPH|SEFLG_SPEED,
                      SE_NODBIT_MEAN, xasc, xdsc, xper, xaph, serr);
  row("nod_aps_asc",  rf, xasc, 6, serr);
  row("nod_aps_dsc",  rf, xdsc, 6, NULL);
  row("nod_aps_peri", rf, xper, 6, NULL);
  row("nod_aps_aphe", rf, xaph, 6, NULL);
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

/* swehel.c had ZERO line coverage before this was added, yet Phase 1.4
 * modified it. Heliacal events, visibility limits and the atmospheric
 * extinction chain (kR/kW/kOZ/ka) all live there. */
static void heliacal(void) {
  char serr[AS_MAXCH]; double dret[50]; char tag[256]; char obj[AS_MAXCH];
  double datm[4] = {1013.25, 15, 40, 0};
  double dobs[6] = {36, 1, 1, 1, 1, 1};
  const double sites[][3] = {{8.55,47.37,400},{31.2,30.0,20},{-70.7,-29.3,2400}};
  const char *objs[] = {"Venus", "Mercury", "Moon", "Sirius", "Mars"};
  const int32 events[] = {SE_HELIACAL_RISING, SE_HELIACAL_SETTING,
                          SE_EVENING_FIRST, SE_MORNING_LAST};
  for (size_t g = 0; g < sizeof(sites)/sizeof(sites[0]); g++) {
    double dgeo[3] = { sites[g][0], sites[g][1], sites[g][2] };
    for (size_t o = 0; o < sizeof(objs)/sizeof(objs[0]); o++) {
      for (size_t e = 0; e < sizeof(events)/sizeof(events[0]); e++) {
        memset(dret, 0, sizeof dret); serr[0] = 0; strcpy(obj, objs[o]);
        int32 rf = swe_heliacal_ut(2451545.0, dgeo, datm, dobs, obj,
                                   events[e], SEFLG_SWIEPH, dret, serr);
        snprintf(tag, sizeof tag, "hel_ut[%zu,%s,%d]", g, objs[o], (int)events[e]);
        row(tag, rf, dret, 3, serr);
      }
      memset(dret, 0, sizeof dret); serr[0] = 0; strcpy(obj, objs[o]);
      int32 rf = swe_vis_limit_mag(2451545.0, dgeo, datm, dobs, obj,
                                   SEFLG_SWIEPH, dret, serr);
      snprintf(tag, sizeof tag, "vislim[%zu,%s]", g, objs[o]);
      row(tag, rf, dret, 8, serr);

      memset(dret, 0, sizeof dret); serr[0] = 0; strcpy(obj, objs[o]);
      rf = swe_heliacal_pheno_ut(2451545.0, dgeo, datm, dobs, obj,
                                 SE_HELIACAL_RISING, SEFLG_SWIEPH, dret, serr);
      snprintf(tag, sizeof tag, "helpheno[%zu,%s]", g, objs[o]);
      row(tag, rf, dret, 12, serr);
    }
    memset(dret, 0, sizeof dret); serr[0] = 0;
    int32 rf = swe_topo_arcus_visionis(2451545.0, dgeo, datm, dobs, SEFLG_SWIEPH,
                                       -1, 0, 100, 10, 120, 20, dret, serr);
    snprintf(tag, sizeof tag, "tav[%zu]", g);
    row(tag, rf, dret, 1, serr);
    memset(dret, 0, sizeof dret); serr[0] = 0;
    rf = swe_heliacal_angle(2451545.0, dgeo, datm, dobs, SEFLG_SWIEPH,
                            -1, 0, 100, 120, 20, dret, serr);
    snprintf(tag, sizeof tag, "helangle[%zu]", g);
    row(tag, rf, dret, 3, serr);
  }
}

/* Name lookups and small conversions. swe_get_ayanamsa_name() in particular
 * reads the ayanamsa_name[] table that Phase 1.5 const-qualified, and had
 * zero coverage. */
static void misc(void) {
  char tag[256], buf[AS_MAXCH], serr[AS_MAXCH];
  for (int sid = 0; sid <= 47; sid++) {
    const char *nm = swe_get_ayanamsa_name(sid);
    fprintf(TRANSCRIPT, "%-46s %s\n",
            (snprintf(tag, sizeof tag, "ayanname[%d]", sid), tag),
            nm ? nm : "(null)");
  }
  for (int p = SE_SUN; p <= SE_VESTA; p++) {
    buf[0] = 0; swe_get_planet_name(p, buf);
    fprintf(TRANSCRIPT, "%-46s %s\n",
            (snprintf(tag, sizeof tag, "plname[%d]", p), tag), buf);
  }
  const char *hs = "PKORCAEVXHTBGWMNQLIUSDFY";
  for (const char *h = hs; *h; h++)
    fprintf(TRANSCRIPT, "%-46s %s\n",
            (snprintf(tag, sizeof tag, "hname[%c]", *h), tag), swe_house_name(*h));
  /* degree splitting across rounding modes */
  for (int i = 0; i < 6; i++) {
    int32 ideg, imin, isec, isgn; double dsecfr;
    const int32 fl[] = {0, SE_SPLIT_DEG_ROUND_SEC, SE_SPLIT_DEG_ROUND_MIN,
                        SE_SPLIT_DEG_ROUND_DEG, SE_SPLIT_DEG_ZODIACAL,
                        SE_SPLIT_DEG_NAKSHATRA};
    swe_split_deg(123.456789 + i, fl[i], &ideg, &imin, &isec, &dsecfr, &isgn);
    fprintf(TRANSCRIPT, "%-46s %d %d %d %a %d\n",
            (snprintf(tag, sizeof tag, "splitdeg[%d]", i), tag),
            ideg, imin, isec, dsecfr, isgn);
  }
  /* calendar / UTC conversions */
  for (int y = 1500; y <= 2100; y += 100) {
    double jd, dret[2]; int32 iy, im, id, ih, mi; double sec;
    swe_utc_to_jd(y, 6, 15, 12, 30, 30.5, SE_GREG_CAL, dret, serr);
    swe_jdet_to_utc(dret[0], SE_GREG_CAL, &iy, &im, &id, &ih, &mi, &sec);
    jd = swe_julday(y, 6, 15, 12.5, SE_GREG_CAL);
    fprintf(TRANSCRIPT, "%-46s %a %a %a %d-%d-%d %d:%d:%a\n",
            (snprintf(tag, sizeof tag, "utc[%d]", y), tag),
            dret[0], dret[1], jd, iy, im, id, ih, mi, sec);
  }
  /* refraction + horizontal coordinates */
  double xin[3] = {45.0, 10.0, 0}, xaz[3], xout[6], geo[3] = {8.55, 47.37, 400};
  for (int i = 0; i < 5; i++) {
    xin[1] = -2.0 + i * 3.0;
    swe_azalt(2451545.0, SE_ECL2HOR, geo, 1013.25, 15.0, xin, xaz);
    double r = swe_refrac_extended(xin[1], 400, 1013.25, 15.0, 0.0065,
                                   SE_TRUE_TO_APP, xout);
    fprintf(TRANSCRIPT, "%-46s %a %a %a | %a %a\n",
            (snprintf(tag, sizeof tag, "azalt[%d]", i), tag),
            xaz[0], xaz[1], xaz[2], r, xout[0]);
  }
  /* library path: value is machine-specific, so assert only its shape */
  buf[0] = 0; swe_get_library_path(buf);
  fprintf(TRANSCRIPT, "%-46s len_nonzero=%d terminated=%d\n", "libpath",
          buf[0] != '\0', memchr(buf, '\0', AS_MAXCH) != NULL);
}

/* Compute-only suite: touches no swe_set_* function.
 *
 * This is what the --threads mode runs, and it is the precise statement of
 * what Phase 2 guarantees: configuration applied ONCE, before workers
 * start, is visible to every worker. It is also exactly the pyswisseph
 * bug -- set_ephe_path()/set_sid_mode()/set_topo() on the main thread,
 * compute on a pool.
 *
 * What it deliberately does NOT assert is N threads holding N different
 * live configurations at once. A single shared master cannot provide that,
 * and pretending otherwise here would be a test that can only be made to
 * pass by weakening it. That capability is Phase 3 (the context handle).
 */
static void suite_compute_only(void) {
  emit_version();
  planets();
  planets_ut();
  asteroids();
  houses();
  fixstars();
  timeconv();
  eclipses();
  pheno();
  heliacal();
  /* sidereal positions and topocentric positions computed against
   * whatever the main thread configured -- the fields that were broken */
  {
    char serr[AS_MAXCH]; double x[6]; char tag[256];
    for (size_t d = 0; d < NDATES; d++) {
      double ay = swe_get_ayanamsa_ut(DATES[d]);
      snprintf(tag, sizeof tag, "inherit_ayan[%zu]", d);
      row(tag, 0, &ay, 1, NULL);
      for (int p = SE_SUN; p <= SE_SATURN; p++) {
        serr[0]=0; memset(x,0,sizeof x);
        int32 rf = swe_calc_ut(DATES[d], p,
                     SEFLG_SWIEPH|SEFLG_SIDEREAL|SEFLG_SPEED, x, serr);
        snprintf(tag, sizeof tag, "inherit_sid[%zu,%d]", d, p);
        row(tag, rf, x, 6, serr);
        serr[0]=0; memset(x,0,sizeof x);
        rf = swe_calc_ut(DATES[d], p,
                     SEFLG_SWIEPH|SEFLG_TOPOCTR|SEFLG_SPEED, x, serr);
        snprintf(tag, sizeof tag, "inherit_topo[%zu,%d]", d, p);
        row(tag, rf, x, 6, serr);
      }
    }
    fprintf(TRANSCRIPT, "%-46s %a\n", "inherit_tidacc", swe_get_tid_acc());
  }
}

/* Runs the entire suite against the current thread's library state. */
/* Entry points no other section reaches.
 *
 * Measured with gcov: 36 of the 77 swe_*_r entry points were never executed
 * by this suite. Phase 3 rewrote all 77 mechanically -- and two of the
 * generated shims silently landed inside a `#if 0` and vanished from the
 * ABI before the linker caught them -- so an untested entry point is
 * exactly where such a mistake survives.
 *
 * Appended last on purpose: existing rows keep their order, so the baseline
 * diff for this addition is a pure append and stays reviewable.
 *
 * Setters are save/restored, since a leaked configuration change here would
 * corrupt every row that follows in the threaded run. */
static void coverage(void) {
  char serr[AS_MAXCH]; char tag[256]; double x[24]; int32 rf;
  double cusp[13], ascmc[10], cs[13], as[10];
  int i;

  /* --- houses: the variants the houses() section does not call ------- */
  for (i = 0; i < 3; i++) {
    double tjd = 2451545.0 + i * 3000.0;
    int hs = "PKO"[i];
    rf = swe_houses(tjd, 48.2 + i, 16.4 - i, hs, cusp, ascmc);
    sprintf(tag, "cov:houses[%d,%c]", i, hs);          row(tag, rf, cusp, 13, "");
    rf = swe_houses_ex(tjd, SEFLG_SWIEPH, 48.2 + i, 16.4 - i, hs, cusp, ascmc);
    sprintf(tag, "cov:houses_ex[%d,%c]", i, hs);       row(tag, rf, ascmc, 10, "");
    rf = swe_houses_armc(30.0 * i, 48.2, 23.44, hs, cs, as);
    sprintf(tag, "cov:houses_armc[%d,%c]", i, hs);     row(tag, rf, cs, 13, "");
    x[0] = swe_house_pos(30.0 * i, 48.2, 23.44, hs, (double[]){ 120.0, 1.5, 1.0 }, serr);
    sprintf(tag, "cov:house_pos[%d,%c]", i, hs);       row(tag, 0, x, 1, serr);
  }

  /* --- ayanamsa: three of the four forms are otherwise untouched ------ */
  for (i = 0; i < 2; i++) {
    double tjd = 2451545.0 + i * 10000.0;
    x[0] = swe_get_ayanamsa(tjd);
    sprintf(tag, "cov:ayanamsa[%d]", i);               row(tag, 0, x, 1, "");
    *serr = 0; rf = swe_get_ayanamsa_ex(tjd, SEFLG_SWIEPH, &x[0], serr);
    sprintf(tag, "cov:ayanamsa_ex[%d]", i);            row(tag, rf, x, 1, serr);
    *serr = 0; rf = swe_get_ayanamsa_ex_ut(tjd, SEFLG_SWIEPH, &x[0], serr);
    sprintf(tag, "cov:ayanamsa_ex_ut[%d]", i);         row(tag, rf, x, 1, serr);
  }

  /* --- time conversions not covered by timeconv() -------------------- */
  for (i = 0; i < 2; i++) {
    double tjd = 2451545.0 + i * 5000.0;
    int32 y, mo, d, h, mi; double sec;
    x[0] = swe_deltat(tjd);
    sprintf(tag, "cov:deltat[%d]", i);                 row(tag, 0, x, 1, "");
    *serr = 0; rf = swe_time_equ(tjd, &x[0], serr);
    sprintf(tag, "cov:time_equ[%d]", i);               row(tag, rf, x, 1, serr);
    *serr = 0; rf = swe_lmt_to_lat(tjd, 16.4, &x[0], serr);
    sprintf(tag, "cov:lmt_to_lat[%d]", i);             row(tag, rf, x, 1, serr);
    *serr = 0; rf = swe_lat_to_lmt(tjd, 16.4, &x[0], serr);
    sprintf(tag, "cov:lat_to_lmt[%d]", i);             row(tag, rf, x, 1, serr);
    swe_jdut1_to_utc(tjd, SE_GREG_CAL, &y, &mo, &d, &h, &mi, &sec);
    x[0] = y; x[1] = mo; x[2] = d; x[3] = h; x[4] = mi; x[5] = sec;
    sprintf(tag, "cov:jdut1_to_utc[%d]", i);           row(tag, 0, x, 6, "");
  }

  /* --- crossings ------------------------------------------------------ */
  for (i = 0; i < 2; i++) {
    double jd = 2451545.0 + i * 400.0;
    *serr = 0; x[0] = swe_solcross(90.0 * (i + 1), jd, SEFLG_SWIEPH, serr);
    sprintf(tag, "cov:solcross[%d]", i);               row(tag, 0, x, 1, serr);
    *serr = 0; x[0] = swe_solcross_ut(90.0 * (i + 1), jd, SEFLG_SWIEPH, serr);
    sprintf(tag, "cov:solcross_ut[%d]", i);            row(tag, 0, x, 1, serr);
    *serr = 0; x[0] = swe_mooncross(90.0 * (i + 1), jd, SEFLG_SWIEPH, serr);
    sprintf(tag, "cov:mooncross[%d]", i);              row(tag, 0, x, 1, serr);
    *serr = 0; x[0] = swe_mooncross_ut(90.0 * (i + 1), jd, SEFLG_SWIEPH, serr);
    sprintf(tag, "cov:mooncross_ut[%d]", i);           row(tag, 0, x, 1, serr);
    *serr = 0; x[1] = 0;
    x[0] = swe_mooncross_node(jd, SEFLG_SWIEPH, &x[1], &x[2], serr);
    sprintf(tag, "cov:mooncross_node[%d]", i);         row(tag, 0, x, 3, serr);
    *serr = 0; x[1] = 0;
    x[0] = swe_mooncross_node_ut(jd, SEFLG_SWIEPH, &x[1], &x[2], serr);
    sprintf(tag, "cov:mooncross_node_ut[%d]", i);      row(tag, 0, x, 3, serr);
    *serr = 0; rf = swe_helio_cross(SE_VENUS, 90.0 * (i + 1), jd, SEFLG_SWIEPH, 1, &x[0], serr);
    sprintf(tag, "cov:helio_cross[%d]", i);            row(tag, rf, x, 1, serr);
    *serr = 0; rf = swe_helio_cross_ut(SE_VENUS, 90.0 * (i + 1), jd, SEFLG_SWIEPH, 1, &x[0], serr);
    sprintf(tag, "cov:helio_cross_ut[%d]", i);         row(tag, rf, x, 1, serr);
  }

  /* --- misc compute --------------------------------------------------- */
  for (i = 0; i < 2; i++) {
    double tjd = 2451545.0 + i * 2000.0;
    *serr = 0; rf = swe_calc_pctr(tjd, SE_MARS, SE_JUPITER, SEFLG_SWIEPH, x, serr);
    sprintf(tag, "cov:calc_pctr[%d]", i);              row(tag, rf, x, 6, serr);
    *serr = 0; rf = swe_gauquelin_sector(tjd, SE_MARS, NULL, SEFLG_SWIEPH, 0,
                                         (double[]){ 16.4, 48.2, 190.0 }, 1013.25, 15.0, &x[0], serr);
    sprintf(tag, "cov:gauquelin[%d]", i);              row(tag, rf, x, 1, serr);
    *serr = 0; rf = swe_get_orbital_elements(tjd, SE_MARS, SEFLG_SWIEPH, x, serr);
    sprintf(tag, "cov:orbital_elements[%d]", i);       row(tag, rf, x, 17, serr);
    /* star is an IN/OUT parameter -- swe_fixstar_ut() writes the resolved
     * name back into it, so it must be a writable AS_MAXCH buffer. Passing
     * a string literal segfaults in fixstar_cut_string(). */
    { char star[AS_MAXCH]; strcpy(star, "Sirius");
      *serr = 0; rf = swe_fixstar_ut(star, tjd, SEFLG_SWIEPH, x, serr);
      sprintf(tag, "cov:fixstar_ut[%d]", i);           row(tag, rf, x, 6, serr); }
  }

  /* --- file/model accessors ------------------------------------------- */
  {
    double tfstart = 0, tfend = 0; int32 denum = 0;
    const char *fn = swe_get_current_file_data(0, &tfstart, &tfend, &denum);
    x[0] = tfstart; x[1] = tfend; x[2] = denum;
    sprintf(tag, "cov:current_file_data[%s]", fn ? "set" : "null");
    row(tag, 0, x, 3, "");
    /* NULL samod, deliberately: swe_get_astro_models() CALLS
     * swe_set_astro_models() whenever samod is non-NULL, so the obvious
     * "read the models into a buffer" usage would mutate process-wide
     * state. Passing NULL is the only read-only form. */
    char sdet[AS_MAXCH * 4];
    *sdet = '\0';
    swe_get_astro_models(NULL, sdet, 0);
    { char cl[AS_MAXCH * 4]; sanitize(cl, sizeof cl, sdet);
      fprintf(TRANSCRIPT, "%-46s rf=%-6d | %s\n", "cov:astro_models", 0, cl); }
  }

  /* --- eclipse/occultation searches the eclipses() section skips ------
   * These are iterative searches, so one call each: the point is to
   * execute the code path, not to survey it. */
  {
    double geo[3] = { 16.4, 48.2, 190.0 };
    double tret[10], attr[20];
    memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr);
    *serr = 0;
    rf = swe_sol_eclipse_when_loc(2451545.0, SEFLG_SWIEPH, geo, tret, attr, FALSE, serr);
    row("cov:sol_eclipse_when_loc", rf, tret, 7, serr);
    memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr);
    *serr = 0;
    rf = swe_lun_eclipse_when_loc(2451545.0, SEFLG_SWIEPH, geo, tret, attr, FALSE, serr);
    row("cov:lun_eclipse_when_loc", rf, tret, 7, serr);
    memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr);
    *serr = 0;
    rf = swe_lun_occult_when_loc(2451545.0, SE_VENUS, NULL, SEFLG_SWIEPH,
                                 geo, tret, attr, FALSE, serr);
    row("cov:lun_occult_when_loc", rf, tret, 7, serr);
    memset(tret, 0, sizeof tret);
    *serr = 0;
    rf = swe_lun_occult_when_glob(2451545.0, SE_VENUS, NULL, SEFLG_SWIEPH, 0,
                                  tret, FALSE, serr);
    row("cov:lun_occult_when_glob", rf, tret, 7, serr);
    memset(attr, 0, sizeof attr);
    { double g2[10]; memset(g2, 0, sizeof g2); *serr = 0;
      rf = swe_lun_occult_where(2451545.0, SE_VENUS, NULL, SEFLG_SWIEPH, g2, attr, serr);
      row("cov:lun_occult_where", rf, attr, 8, serr); }
  }

  /* --- setters: exercised, then restored ------------------------------
   * Each publishes to the process-wide master, so leaving one set would
   * move every row computed afterwards -- including in the threaded run,
   * where a worker would adopt it mid-suite. */
  {
    swe_set_delta_t_userdef(70.0);
    x[0] = swe_deltat(2451545.0);
    row("cov:set_delta_t_userdef", 0, x, 1, "");
    swe_set_delta_t_userdef(SE_DELTAT_AUTOMATIC);

    swe_set_lapse_rate(0.0070);
    *serr = 0;
    rf = swe_rise_trans(2451545.0, SE_SUN, NULL, SEFLG_SWIEPH, SE_CALC_RISE,
                        (double[]){ 16.4, 48.2, 190.0 }, 1013.25, 15.0, &x[0], serr);
    row("cov:set_lapse_rate", rf, x, 1, serr);
    swe_set_lapse_rate(0.0065);          /* SE_LAPSE_RATE, the documented default */

    /* swe_set_astro_models("") re-applies the library defaults, which is
     * what is already in force -- so this exercises the setter without
     * moving any row computed afterwards. Verified: the 5137 pre-existing
     * baseline rows are byte-identical with and without this call. */
    { char sam[AS_MAXCH]; sam[0] = '\0'; swe_set_astro_models(sam, 0); }
    *serr = 0; rf = swe_calc(2451545.0, SE_SUN, SEFLG_SWIEPH, x, serr);
    row("cov:set_astro_models", rf, x, 6, serr);

    swe_set_interpolate_nut(TRUE);
    *serr = 0; rf = swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
    row("cov:interpolate_nut", rf, x, 6, serr);
    swe_set_interpolate_nut(FALSE);

    /* swe_set_jpl_file() only records a name; no .eph is shipped with this
     * repository, so the observable effect is what happens when the file is
     * missing. Under the strict default that is now a refusal (rf=-1) rather
     * than a Swiss answer wearing a JPL label -- which is the whole point,
     * and this row is the transcript's record of it. */
    { char jf[AS_MAXCH]; strcpy(jf, "de431.eph"); swe_set_jpl_file(jf); }
    *serr = 0; rf = swe_calc(2451545.0, SE_MARS, SEFLG_JPLEPH, x, serr);
    row("cov:set_jpl_file", rf, x, 6, serr);
    { char jf[AS_MAXCH]; strcpy(jf, "de440.eph"); swe_set_jpl_file(jf); }
    swe_close();                          /* drop the failed JPL attempt */
    swe_set_ephe_path((char *) EPHE);

    /* ⛔ Order independence: the same Swiss request must answer the same
     * whether or not a Moshier calculation preceded it.
     *
     * It did not. Switching ephemeris closes the .se1 files and used to zero
     * the DE number recorded with them, and swe_calc_ut() asks for delta-t
     * BEFORE the position that would reopen them -- so the tidal term fell
     * back to the default -25.8 instead of DE441's -25.936. At -3000, where
     * delta-t is hours, that moved the Sun 4.56 arcsec depending on what had
     * been computed before it.
     *
     * These two rows are identical by construction and worthless if they
     * ever stop being identical, which is exactly what makes them a test. */
    *serr = 0; rf = swe_calc_ut(DATES[0], SE_SUN, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    row("cov:order_swiss_alone", rf, x, 6, serr);
    *serr = 0; swe_calc(DATES[0], SE_SUN, SEFLG_MOSEPH|SEFLG_SPEED, x, serr);
    *serr = 0; rf = swe_calc_ut(DATES[0], SE_SUN, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    row("cov:order_swiss_after_moseph", rf, x, 6, serr);

    /* Merely NAMING a JPL file must not move a result computed from another
     * ephemeris. swe_set_jpl_file() both closes the .se1 files and sets
     * jpldenum, and each leaked somewhere it had no business:
     *
     *   the close forgot the Moon file's DE number, so a later SWISS calc_ut
     *   took the default tidal term      -> 4.56 arcsec  (swe_set_jpl_file_r)
     *   jpldenum reached MOSHIER's tidal
     *   term through swe_deltat_ex_r     -> 56 arcsec    (swi_get_tid_acc)
     *
     * All three rows below must therefore equal their _alone counterparts. */
    *serr = 0; rf = swe_calc_ut(DATES[0], SE_SUN, SEFLG_MOSEPH|SEFLG_SPEED, x, serr);
    row("cov:order_moseph_alone", rf, x, 6, serr);
    { char jf[AS_MAXCH]; strcpy(jf, "de200.eph"); swe_set_jpl_file(jf); }
    *serr = 0; rf = swe_calc_ut(DATES[0], SE_SUN, SEFLG_MOSEPH|SEFLG_SPEED, x, serr);
    row("cov:order_moseph_after_jplname", rf, x, 6, serr);
    *serr = 0; rf = swe_calc_ut(DATES[0], SE_SUN, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    row("cov:order_swiss_after_jplname", rf, x, 6, serr);

    /* One request, one answer, however the ephemeris bits are spelled.
     * swe_calc() resolves SWIEPH|MOSEPH to Swiss (plaus_iflag: JPL over
     * Swiss over Moshier) and the strict guard must read the bits in the
     * same order. It used to read them the other way round and refuse the
     * call as "Moshier answered by Swiss". This row must equal
     * cov:order_swiss_alone. */
    *serr = 0; rf = swe_calc_ut(DATES[0], SE_SUN, SEFLG_SWIEPH|SEFLG_MOSEPH|SEFLG_SPEED, x, serr);
    row("cov:flags_swi_or_mos", rf, x, 6, serr);

    /* swe_fixstar() computes the Earth through main_planet_bary(), not
     * swe_calc(), so the guard at swe_calc()'s exit never saw it: with the
     * JPL file missing it answered from Swiss and still returned
     * SEFLG_JPLEPH. The real file (de200, named above) must go through
     * (rf=1); a missing one must be refused (rf=-1), for both star APIs. */
    { char st[AS_MAXCH]; strcpy(st, "Sirius"); *serr = 0; rf = swe_fixstar(st, 2451545.0, SEFLG_JPLEPH, x, serr);
      row("cov:fixstar_jpl_real", rf, x, 6, serr); }
    { char st[AS_MAXCH]; strcpy(st, "Sirius"); *serr = 0; rf = swe_fixstar2(st, 2451545.0, SEFLG_JPLEPH, x, serr);
      row("cov:fixstar2_jpl_real", rf, x, 6, serr); }
    { char jf[AS_MAXCH]; strcpy(jf, "nope.eph"); swe_set_jpl_file(jf); }
    { char st[AS_MAXCH]; strcpy(st, "Sirius"); *serr = 0; rf = swe_fixstar(st, 2451545.0, SEFLG_JPLEPH, x, serr);
      row("cov:fixstar_jpl_missing", rf, x, 6, serr); }
    { char st[AS_MAXCH]; strcpy(st, "Sirius"); *serr = 0; rf = swe_fixstar2(st, 2451545.0, SEFLG_JPLEPH, x, serr);
      row("cov:fixstar2_jpl_missing", rf, x, 6, serr); }
    { char jf[AS_MAXCH]; strcpy(jf, "de440.eph"); swe_set_jpl_file(jf); }
    swe_close();
    swe_set_ephe_path((char *) EPHE);

    /* Error strings from the star lookups. The transcript pins serr, but
     * only for the messages some row happens to reach -- and none reached
     * these, so the bounded-string sweep respelt "star  not found" with one
     * space and every gate stayed green. Both entry points, because they
     * fail through different code: swe_fixstar() scans the file, while
     * swe_fixstar2() searches the parsed list. The 250-character name is
     * the input that reaches the bare message, the branch that gives up on
     * appending the name. */
    { static const char *bad[] = { "NoSuchStar", "NoSuchStar%", ",noSuchBayer", "99999", "" };
      char st[AS_MAXCH], tg[64];
      for (size_t b = 0; b < sizeof bad / sizeof *bad; b++) {
        snprintf(tg, sizeof tg, "cov:starerr[%zu,v1]", b);
        strcpy(st, bad[b]); *serr = 0;
        rf = swe_fixstar(st, 2451545.0, SEFLG_SWIEPH, x, serr);
        row(tg, rf, NULL, 0, serr);
        snprintf(tg, sizeof tg, "cov:starerr[%zu,v2]", b);
        strcpy(st, bad[b]); *serr = 0;
        rf = swe_fixstar2(st, 2451545.0, SEFLG_SWIEPH, x, serr);
        row(tg, rf, NULL, 0, serr);
      }
      memset(st, 'q', 250); st[250] = '\0'; *serr = 0;
      rf = swe_fixstar(st, 2451545.0, SEFLG_SWIEPH, x, serr);
      row("cov:starerr[long,v1]", rf, NULL, 0, serr);
      memset(st, 'q', 250); st[250] = '\0'; *serr = 0;
      rf = swe_fixstar2(st, 2451545.0, SEFLG_SWIEPH, x, serr);
      row("cov:starerr[long,v2]", rf, NULL, 0, serr);
    }

    /* Entry points the transcript had never reached: swe_date_conversion(),
     * the v1 swe_fixstar_mag(), and the fictitious bodies. The last group
     * reads its elements from seorbel.txt through a per-context cache; these
     * rows are what holds that cache to the file's exact contents. */
    for (int y = -1000; y <= 2000; y += 1000) {
      double tjd = 0; *serr = 0;
      rf = swe_date_conversion(y, 3, 21, 12.5, 'g', &tjd);
      snprintf(tag, sizeof tag, "cov:date_conversion[%d]", y);
      row(tag, rf, &tjd, 1, serr);
    }
    { const char *st3[] = {"Sirius", "Aldebaran", "Polaris"};
      for (size_t s = 0; s < 3; s++) {
        char st[AS_MAXCH]; double mag = 0; strcpy(st, st3[s]); *serr = 0;
        rf = swe_fixstar_mag(st, &mag, serr);
        snprintf(tag, sizeof tag, "cov:fixstar_mag[%s]", st3[s]);
        row(tag, rf, &mag, 1, serr);
      } }
    for (int ipl = SE_FICT_OFFSET; ipl <= SE_FICT_OFFSET + 16; ipl++) {
      char nm[AS_MAXCH];
      for (size_t d = 0; d < NDATES; d += 4) {
        *serr = 0; memset(x, 0, sizeof x);
        rf = swe_calc(DATES[d], ipl, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
        snprintf(tag, sizeof tag, "cov:fict[%d,%zu]", ipl, d);
        row(tag, rf, x, 6, serr);
      }
      swe_get_planet_name(ipl, nm);
      snprintf(tag, sizeof tag, "cov:fictname[%d]", ipl);
      row(tag, 0, x, 0, nm);
    }
  }
}

/* Broad date sweep: every major body at 120 pseudo-random dates.
 *
 * The nine dates above are hand-picked EDGE cases -- -3000, year 1, the
 * Gregorian switch, J2000, a fractional day, 2100 -- and they are good at
 * catching boundary faults. They are poor at catching a fault that lives in
 * the middle of the range, which is most of it: 9 samples across 5000 years
 * is one every 550 years.
 *
 * This walks 120 dates spread across roughly 1000..2400 CE and reports the
 * full state vector for each body: ecliptic longitude and latitude,
 * distance, and all three speeds. Plus an equatorial pass, because a fault
 * in the coordinate transform would otherwise hide behind correct ecliptic
 * values.
 *
 * The dates are DETERMINISTIC -- a fixed 64-bit LCG, not rand(), whose
 * sequence cannot change with the platform's libc. A transcript that varied
 * between runs would be worthless as a bit-exact baseline.
 */
#define NSWEEP 120

static double sweep_date(int i)
{
  /* splitmix64-style mixing of the index. Deterministic everywhere, and
   * well spread, which a plain linear stride would not be -- a stride can
   * alias with an orbital period and sample the same phase every time. */
  unsigned long long z = (unsigned long long) i * 0x9E3779B97F4A7C15ULL + 0x123456789ABCDEFULL;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  z ^=  z >> 31;
  /* JD 2086302.5 (1000-01-01) .. 2597641.5 (2400-01-01), ~1400 years */
  return 2086302.5 + (double) (z % 511339ULL) + (double) ((z >> 20) % 1000ULL) / 1000.0;
}

static void sweep(void) {
  char serr[AS_MAXCH]; double x[6]; char tag[256];
  static const struct { int32 f; const char *n; } SF[] = {
    { SEFLG_SWIEPH | SEFLG_SPEED,                     "swieph" },
    { SEFLG_MOSEPH | SEFLG_SPEED,                     "moseph" },
    { SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_EQUATORIAL,  "equat"  },
  };
  for (int d = 0; d < NSWEEP; d++) {
    double tjd = sweep_date(d);
    for (size_t f = 0; f < sizeof SF / sizeof SF[0]; f++)
      for (int p = SE_SUN; p <= SE_VESTA; p++) {
        serr[0] = 0; memset(x, 0, sizeof x);
        int32 rf = swe_calc(tjd, p, SF[f].f, x, serr);
        snprintf(tag, sizeof tag, "sweep[%d,%s,%d]%s", d, SF[f].n, p,
                 is_illcond(SF[f].f, p) ? ILLCOND : "");
        row(tag, rf, x, 6, serr);
      }
  }
}

static void suite(void) {
  emit_version();
  planets();
  planets_ut();
  asteroids();
  sidereal();
  topo();
  houses();
  sunshine();
  fixstars();
  timeconv();
  eclipses();
  pheno();
  heliacal();
  misc();
  coverage();
  sweep();
}

struct targ { int id; char *buf; size_t len; int setup; };

/* Capturing each worker's transcript used to call open_memstream(), which is
 * POSIX and has no MSVC equivalent, so G2 -- the gate this file exists for --
 * did not run on Windows at all. A per-thread temp file is standard C and
 * hands the comparison below exactly what it wants, a->buf and a->len. One
 * implementation for every platform, so Windows exercises the same path.
 *
 * "w+b": no newline translation, so the bytes are the bytes on either side. */
static FILE *capture_open(int id, char *path, size_t n) {
  snprintf(path, n, ".golden_capture_%d.tmp", id);
  return fopen(path, "w+b");
}

static void capture_close(struct targ *a, FILE *f, const char *path) {
  long end;
  fflush(f);
  end = ftell(f);
  a->len = end > 0 ? (size_t) end : 0;
  a->buf = malloc(a->len + 1);
  if (a->buf != NULL) {
    rewind(f);
    a->len = fread(a->buf, 1, a->len, f);
    a->buf[a->len] = '\0';
  } else {
    a->len = 0;
  }
  fclose(f);
  remove(path);
}

static thr_ret_t THR_CALL thread_suite(void *p) {
  struct targ *a = p;
  char path[64];
  FILE *f = capture_open(a->id, path, sizeof path);
  if (f == NULL) {
    fprintf(stderr, "could not open capture file %s\n", path);
    a->buf = NULL; a->len = 0;
    return (thr_ret_t) 0;
  }
  TRANSCRIPT = f;
  /* a->setup mirrors what a well-behaved caller does on a worker thread.
   * 0 = configure only on the main thread (the pyswisseph pattern).
   * 1 = re-apply configuration on every worker thread (today's workaround). */
  if (a->setup) swe_set_ephe_path((char *)EPHE);
  suite_compute_only();
  capture_close(a, f, path);
  return (thr_ret_t) 0;
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
    TRANSCRIPT = stdout;
    suite();
    swe_close();
    return 0;
  }

  /* thread-consistency mode: every thread must reproduce the main-thread
   * transcript byte for byte. */
  /* Configure EVERYTHING here, on the main thread, once. Workers below
   * call no setter at all. */
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  swe_set_topo(8.55, 47.37, 400);
  swe_set_tid_acc(-25.85);

  struct targ ref = { -1, NULL, 0, 0 };
  thread_suite(&ref);

  struct targ *a = calloc(nthreads, sizeof *a);
  thr_t *t = calloc(nthreads, sizeof *t);
  for (int i = 0; i < nthreads; i++) {
    a[i].id = i; a[i].setup = setup;
    thr_create(&t[i], thread_suite, &a[i]);
  }
  for (int i = 0; i < nthreads; i++) thr_join(t[i]);

  if (getenv("SE_DUMP")) {
    /* relative, not /tmp: this has to land somewhere on Windows too */
    FILE *f = fopen("golden_ref.txt","wb"); fwrite(ref.buf,1,ref.len,f); fclose(f);
    for (int i = 0; i < nthreads; i++) { char p[64]; snprintf(p,sizeof p,"golden_thr%d.txt",i);
      f=fopen(p,"wb"); fwrite(a[i].buf,1,a[i].len,f); fclose(f); }
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
