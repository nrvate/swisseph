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
#include <errno.h>
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
# include <unistd.h>          /* getpid(), for the capture-file name */
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

/* Put astro_models[] back to the library defaults.
 *
 * swe_set_astro_models("") is the documented way to do it, but the argument
 * is `char *` rather than `const char *`, so every caller needs a writable
 * buffer for an empty string. Five coverage blocks below select a model and
 * have to undo it; this is that line, once. */
static void reset_astro_models(void) {
  char sam[AS_MAXCH];
  sam[0] = '\0';
  swe_set_astro_models(sam, 0);
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

  /* Sunshine houses have two solutions and the letter picks between them:
   * 'I' is Treindl's, lowercase 'i' is Makransky's. The table above is
   * uppercase throughout, so sunshine_solution_makransky() -- 40 lines,
   * and the only one of the two that can fail into Porphyry -- was never
   * run. Latitudes either side of the polar circle, because that failure
   * is the interesting half. */
  {
    const double plat[] = { 47.37, 75.0 };
    for (size_t la = 0; la < sizeof(plat)/sizeof(plat[0]); la++) {
      memset(cusp,0,sizeof cusp); memset(ascmc,0,sizeof ascmc);
      memset(csp,0,sizeof csp);   memset(asp,0,sizeof asp);
      ascmc[9] = 99;
      int rc = swe_houses_ex2(DATES[0], SEFLG_SWIEPH, plat[la], 8.55,
                              'i', cusp, ascmc, csp, asp, NULL);
      snprintf(tag, sizeof tag, "hsys_makransky[%zu]", la);
      fprintf(TRANSCRIPT, "%-46s rc=%-4d", tag, rc);
      for (int i = 0; i < 13; i++) fprintf(TRANSCRIPT, " %a", cusp[i]);
      fprintf(TRANSCRIPT, "\n");
    }
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

  /* The rest of swe_nod_aps(). The four rows above are one call -- the Moon,
   * mean method -- and left 41% of its 367 lines run.
   *
   * The method argument is most of what was missing. 0 and SE_NODBIT_MEAN
   * take the closed-form branch, and only for the Sun through Neptune plus
   * the Earth; everything else falls through to the osculating one, which
   * integrates. SE_NODBIT_OSCU_BAR switches that to barycentric, but only
   * for bodies beyond Jupiter, so it needs a distant planet to mean
   * anything. SE_NODBIT_FOPOINT replaces the aphelion with the orbit's
   * second focus, which changes the fourth output array and nothing else.
   *
   * The objects are chosen against those branches rather than for variety:
   * the Moon has its own path, the Earth is special-cased beside the
   * planets, Mercury is an ordinary inner one, and Pluto is far enough out
   * for OSCU_BAR to take effect. */
  {
    static const struct { int32 m; const char *n; } meth[] = {
      { 0,                                    "dflt"     },
      { SE_NODBIT_MEAN,                       "mean"     },
      { SE_NODBIT_OSCU,                       "oscu"     },
      { SE_NODBIT_OSCU_BAR,                   "oscubar"  },
      { SE_NODBIT_OSCU | SE_NODBIT_FOPOINT,   "oscufoc"  },
    };
    static const struct { int32 ipl; const char *n; } obj[] = {
      { SE_MOON,    "moon"    },
      { SE_EARTH,   "earth"   },
      { SE_MERCURY, "mercury" },
      { SE_PLUTO,   "pluto"   },
    };
    for (size_t m = 0; m < sizeof(meth)/sizeof(meth[0]); m++)
      for (size_t o = 0; o < sizeof(obj)/sizeof(obj[0]); o++) {
        memset(xasc,0,sizeof xasc); memset(xdsc,0,sizeof xdsc);
        memset(xper,0,sizeof xper); memset(xaph,0,sizeof xaph);
        serr[0] = 0;
        rf = swe_nod_aps(2451545.0, obj[o].ipl, SEFLG_SWIEPH|SEFLG_SPEED,
                         meth[m].m, xasc, xdsc, xper, xaph, serr);
        snprintf(tag, sizeof tag, "nodaps[%s,%s,asc]", meth[m].n, obj[o].n);
        row(tag, rf, xasc, 6, serr);
        snprintf(tag, sizeof tag, "nodaps[%s,%s,dsc]", meth[m].n, obj[o].n);
        row(tag, rf, xdsc, 6, NULL);
        snprintf(tag, sizeof tag, "nodaps[%s,%s,peri]", meth[m].n, obj[o].n);
        row(tag, rf, xper, 6, NULL);
        snprintf(tag, sizeof tag, "nodaps[%s,%s,aphe]", meth[m].n, obj[o].n);
        row(tag, rf, xaph, 6, NULL);
      }

    /* And the refusals. A node or an apogee has no nodes of its own, and
     * neither does an object number in the gap between the planets and the
     * asteroid offset. Each returns ERR with a message, and the messages
     * are as much a part of the contract as the numbers. */
    { static const struct { int32 ipl; const char *n; } bad[] = {
        { SE_MEAN_NODE, "meannode" }, { SE_TRUE_NODE, "truenode" },
        { SE_OSCU_APOG, "oscuapog" }, { SE_NPLANETS + 1, "gap" },
      };
      for (size_t k = 0; k < sizeof(bad)/sizeof(bad[0]); k++) {
        memset(xasc,0,sizeof xasc); memset(xdsc,0,sizeof xdsc);
        memset(xper,0,sizeof xper); memset(xaph,0,sizeof xaph);
        serr[0] = 0;
        rf = swe_nod_aps(2451545.0, bad[k].ipl, SEFLG_SWIEPH,
                         SE_NODBIT_MEAN, xasc, xdsc, xper, xaph, serr);
        snprintf(tag, sizeof tag, "nodaps[refused,%s]", bad[k].n);
        row(tag, rf, xasc, 6, serr);
      }
    }

    /* Heliocentric, which takes a different iflg0 at the top, and the _ut
     * entry point, which is the one most callers actually use and which
     * only the single row above had reached. */
    memset(xasc,0,sizeof xasc); memset(xdsc,0,sizeof xdsc);
    memset(xper,0,sizeof xper); memset(xaph,0,sizeof xaph);
    serr[0] = 0;
    rf = swe_nod_aps(2451545.0, SE_JUPITER, SEFLG_SWIEPH|SEFLG_HELCTR,
                     SE_NODBIT_OSCU, xasc, xdsc, xper, xaph, serr);
    row("nodaps[helctr,jupiter,asc]", rf, xasc, 6, serr);
    row("nodaps[helctr,jupiter,peri]", rf, xper, 6, NULL);

    memset(xasc,0,sizeof xasc); memset(xdsc,0,sizeof xdsc);
    memset(xper,0,sizeof xper); memset(xaph,0,sizeof xaph);
    serr[0] = 0;
    rf = swe_nod_aps_ut(2451545.0, SE_MARS, SEFLG_SWIEPH|SEFLG_SPEED,
                        SE_NODBIT_OSCU, xasc, xdsc, xper, xaph, serr);
    row("nodaps[ut,mars,asc]", rf, xasc, 6, serr);
    row("nodaps[ut,mars,aphe]", rf, xaph, 6, NULL);
  }
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

  /* The OTHER heliacal strategy. Everything above leaves SE_HELFLAG_AVKIND
   * clear, which sends swe_heliacal_ut() down heliacal_ut_vis_lim(); the
   * arcus-visionis walk in heliacal_ut_arc_vis() -- and moon_event_arc_vis()
   * and get_acronychal_day() with it -- had no coverage at all. That is the
   * branch with the twice-per-step ObjectLoc() evaluation, so the whole
   * "half of these calls are duplicates" argument was being made about code
   * no gate ran.
   *
   * One site, because the point is reaching the branch rather than
   * re-sweeping the sky. The objects are not interchangeable here:
   * acronychal events are remapped onto the arcus-visionis walk only for
   * Planet >= SE_MARS or a fixed star, so Venus never reaches
   * get_acronychal_day() however the flags are set, and the Moon has its
   * own entry point in moon_event_arc_vis(). VR and MIN7 differ again
   * inside, on the solar depression they search to. */
  {
    double dgeo[3] = { sites[0][0], sites[0][1], sites[0][2] };
    const struct { const char *obj; int32 ev, kind; const char *tag; } av[] = {
      { "Mars",  SE_HELIACAL_RISING,   SE_HELFLAG_AVKIND_VR,   "mars,vr"    },
      { "Mars",  SE_ACRONYCHAL_RISING, SE_HELFLAG_AVKIND_VR,   "mars,acro"  },
      { "Mars",  SE_HELIACAL_RISING,   SE_HELFLAG_AVKIND_MIN7, "mars,min7"  },
      { "Moon",  SE_EVENING_FIRST,     SE_HELFLAG_AVKIND_VR,   "moon,vr"    },
    };
    for (size_t k = 0; k < sizeof(av)/sizeof(av[0]); k++) {
      memset(dret, 0, sizeof dret); serr[0] = 0; strcpy(obj, av[k].obj);
      int32 rf = swe_heliacal_ut(2451545.0, dgeo, datm, dobs, obj, av[k].ev,
                                 SEFLG_SWIEPH | av[k].kind, dret, serr);
      snprintf(tag, sizeof tag, "hel_avkind[%s]", av[k].tag);
      row(tag, rf, dret, 3, serr);
    }

    /* An object named by number. DeterObject() maps it to SE_AST_OFFSET +
     * the number, and find_conjunct_sun() then indexed its 18-entry table
     * of conjunction epochs at ipl * 2 -- tcon[20866] for this one. It read
     * whatever followed the table: undefined, different per build, and a
     * SEGV as soon as the address was not mapped. Found by sweeping every
     * object class against every event type under ASan, which nothing had
     * done because the AVKIND half of this file had no coverage at all.
     * The row pins the refusal that replaced it. */
    memset(dret, 0, sizeof dret); serr[0] = 0; strcpy(obj, "433");
    int32 rfa = swe_heliacal_ut(2451545.0, dgeo, datm, dobs, obj,
                                SE_HELIACAL_RISING, SEFLG_SWIEPH, dret, serr);
    row("hel_asteroid[433]", rfa, dret, 3, serr);
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
  /* Through SE_INTP_PERG, not SE_VESTA: the interpolated apogee and perigee
   * are the last two names in the switch and the loop stopped two short of
   * them. */
  for (int p = SE_SUN; p <= SE_INTP_PERG; p++) {
    buf[0] = 0; swe_get_planet_name(p, buf);
    fprintf(TRANSCRIPT, "%-46s %s\n",
            (snprintf(tag, sizeof tag, "plname[%d]", p), tag), buf);
  }
  /* Asking for Pluto by its asteroid number is aliased back to SE_PLUTO. */
  buf[0] = 0; swe_get_planet_name(SE_AST_OFFSET + 134340, buf);
  fprintf(TRANSCRIPT, "%-46s %s\n", "plname[pluto-as-asteroid]", buf);
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
    /* Only ever asked forwards, for a planet with a usable heliocentric
     * speed, and never for something it must refuse. dir < 0 searches
     * backwards (dist = 360 - dist); Chiron is the one body whose speed is
     * replaced by a mean value; the Moon is rejected outright, as are the
     * nodes, the apogees and everything from SE_INTP_APOG up. */
    *serr = 0; rf = swe_helio_cross(SE_VENUS, 90.0 * (i + 1), jd, SEFLG_SWIEPH, -1, &x[0], serr);
    sprintf(tag, "cov:helio_cross[back,%d]", i);       row(tag, rf, x, 1, serr);
    *serr = 0; rf = swe_helio_cross_ut(SE_VENUS, 90.0 * (i + 1), jd, SEFLG_SWIEPH, -1, &x[0], serr);
    sprintf(tag, "cov:helio_cross_ut[back,%d]", i);    row(tag, rf, x, 1, serr);
    *serr = 0; rf = swe_helio_cross(SE_CHIRON, 90.0 * (i + 1), jd, SEFLG_SWIEPH, 1, &x[0], serr);
    sprintf(tag, "cov:helio_cross[chiron,%d]", i);     row(tag, rf, x, 1, serr);
    /* Zeroed for the same reason: a refusal leaves *jd_cross untouched, so
     * these rows were recording the Chiron crossing computed just above. */
    *serr = 0; x[0] = 0;
    rf = swe_helio_cross(SE_MOON, 90.0 * (i + 1), jd, SEFLG_SWIEPH, 1, &x[0], serr);
    sprintf(tag, "cov:helio_cross[refused,%d]", i);    row(tag, rf, x, 1, serr);
    *serr = 0; x[0] = 0;
    rf = swe_helio_cross_ut(SE_MEAN_NODE, 90.0 * (i + 1), jd, SEFLG_SWIEPH, 1, &x[0], serr);
    sprintf(tag, "cov:helio_cross_ut[refused,%d]", i); row(tag, rf, x, 1, serr);
  }

  /* --- misc compute --------------------------------------------------- */
  for (i = 0; i < 2; i++) {
    double tjd = 2451545.0 + i * 2000.0;
    *serr = 0; rf = swe_calc_pctr(tjd, SE_MARS, SE_JUPITER, SEFLG_SWIEPH, x, serr);
    sprintf(tag, "cov:calc_pctr[%d]", i);              row(tag, rf, x, 6, serr);
    /* Without SEFLG_SPEED the whole light-time-of-the-speed block is skipped
     * -- it is what corrects apparent motion for dt changing with time.
     * SEFLG_TRUEPOS skips the light-time iteration entirely, and asking for
     * a body centred on itself is the one argument error the function has. */
    *serr = 0; rf = swe_calc_pctr(tjd, SE_MARS, SE_JUPITER, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    sprintf(tag, "cov:calc_pctr[speed,%d]", i);        row(tag, rf, x, 6, serr);
    *serr = 0; rf = swe_calc_pctr(tjd, SE_MARS, SE_JUPITER, SEFLG_SWIEPH|SEFLG_TRUEPOS, x, serr);
    sprintf(tag, "cov:calc_pctr[truepos,%d]", i);      row(tag, rf, x, 6, serr);
    /* x is zeroed first: swe_calc_pctr() rejects ipl == iplctr without
     * writing it, so the row recorded the PREVIOUS call's six values --
     * byte-identical to the row above and silently re-pinned by any edit
     * to it. Same reason as the memset in the fictitious-body loop. */
    *serr = 0; memset(x, 0, sizeof x);
    rf = swe_calc_pctr(tjd, SE_MARS, SE_MARS, SEFLG_SWIEPH, x, serr);
    sprintf(tag, "cov:calc_pctr[same,%d]", i);         row(tag, rf, x, 6, serr);
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

    /* The eclipse and occultation searches, by TYPE and DIRECTION.
     *
     * Every row above passes ifltype 0 and backward FALSE, which is one
     * path through functions built around those two arguments: ifltype
     * filters which kind of eclipse counts and each kind is searched for
     * differently, and backward reverses the walk. Together they were most
     * of what left this cluster between 62% and 77% -- swecl.c is the
     * least-covered file in the library and this is why.
     *
     * SE_ECL_ONE_TRY rides in the backward argument rather than in ifltype,
     * which is easy to misread: it stops the search after a single synodic
     * period instead of hunting until it finds something. */
    {
      static const struct { int32 t; const char *n; } ecl[] = {
        { 0,                                "any"      },
        { SE_ECL_TOTAL,                     "total"    },
        { SE_ECL_ANNULAR,                   "annular"  },
        { SE_ECL_PARTIAL,                   "partial"  },
        { SE_ECL_TOTAL | SE_ECL_CENTRAL,    "totcen"   },
        { SE_ECL_ANNULAR_TOTAL,             "anntot"   },
        { SE_ECL_PARTIAL | SE_ECL_CENTRAL,  "refused"  },
      };
      for (size_t k = 0; k < sizeof(ecl)/sizeof(ecl[0]); k++) {
        memset(tret, 0, sizeof tret); *serr = 0;
        rf = swe_sol_eclipse_when_glob(2451545.0, SEFLG_SWIEPH, ecl[k].t,
                                       tret, FALSE, serr);
        snprintf(tag, sizeof tag, "cov:sol_ecl_glob[%s]", ecl[k].n);
        row(tag, rf, tret, 7, serr);
      }
      /* Lunar types are their own set -- a lunar eclipse can be penumbral
       * and cannot be annular. */
      { static const struct { int32 t; const char *n; } lun[] = {
          { SE_ECL_TOTAL,     "total"     },
          { SE_ECL_PARTIAL,   "partial"   },
          { SE_ECL_PENUMBRAL, "penumbral" },
        };
        for (size_t k = 0; k < sizeof(lun)/sizeof(lun[0]); k++) {
          memset(tret, 0, sizeof tret); *serr = 0;
          rf = swe_lun_eclipse_when(2451545.0, SEFLG_SWIEPH, lun[k].t,
                                    tret, FALSE, serr);
          snprintf(tag, sizeof tag, "cov:lun_ecl_when[%s]", lun[k].n);
          row(tag, rf, tret, 7, serr);
        }
      }

      /* Backwards, for each of the five searches that take the flag. The
       * walk runs the other way and the first hit is the one before the
       * start date, not after it. */
      memset(tret, 0, sizeof tret); *serr = 0;
      rf = swe_sol_eclipse_when_glob(2451545.0, SEFLG_SWIEPH, 0, tret, TRUE, serr);
      row("cov:sol_ecl_glob[back]", rf, tret, 7, serr);
      memset(tret, 0, sizeof tret); *serr = 0;
      rf = swe_lun_eclipse_when(2451545.0, SEFLG_SWIEPH, 0, tret, TRUE, serr);
      row("cov:lun_ecl_when[back]", rf, tret, 7, serr);
      memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr); *serr = 0;
      rf = swe_sol_eclipse_when_loc(2451545.0, SEFLG_SWIEPH, geo, tret, attr, TRUE, serr);
      row("cov:sol_ecl_loc[back]", rf, tret, 7, serr);
      memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr); *serr = 0;
      rf = swe_lun_eclipse_when_loc(2451545.0, SEFLG_SWIEPH, geo, tret, attr, TRUE, serr);
      row("cov:lun_ecl_loc[back]", rf, tret, 7, serr);
      memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr); *serr = 0;
      rf = swe_lun_occult_when_loc(2451545.0, SE_VENUS, NULL, SEFLG_SWIEPH,
                                   geo, tret, attr, TRUE, serr);
      row("cov:lun_occ_loc[back]", rf, tret, 7, serr);

      /* Occultation by type, plus the two refusals the function spells out:
       * PARTIAL|CENTRAL is contradictory, and only the Sun can be occulted
       * annularly -- ask for an annular occultation of a planet and it says
       * so rather than searching for something that cannot happen. */
      { static const struct { int32 t; const char *n; } occ[] = {
          { SE_ECL_TOTAL,                    "total"    },
          { SE_ECL_PARTIAL,                  "partial"  },
          { SE_ECL_ANNULAR,                  "annular"  },
          { SE_ECL_PARTIAL | SE_ECL_CENTRAL, "refused"  },
        };
        for (size_t k = 0; k < sizeof(occ)/sizeof(occ[0]); k++) {
          memset(tret, 0, sizeof tret); *serr = 0;
          rf = swe_lun_occult_when_glob(2451545.0, SE_VENUS, NULL, SEFLG_SWIEPH,
                                        occ[k].t, tret, FALSE, serr);
          snprintf(tag, sizeof tag, "cov:lun_occ_glob[%s]", occ[k].n);
          row(tag, rf, tret, 7, serr);
        }
      }
      /* A star, which takes the swe_fixstar() branch instead of swe_calc(),
       * and ONE_TRY, which bounds the search instead of letting it hunt. */
      { char st[AS_MAXCH]; strcpy(st, "Aldebaran");
        memset(tret, 0, sizeof tret); *serr = 0;
        rf = swe_lun_occult_when_glob(2451545.0, 0, st, SEFLG_SWIEPH, 0,
                                      tret, FALSE, serr);
        row("cov:lun_occ_glob[star]", rf, tret, 7, serr); }
      memset(tret, 0, sizeof tret); *serr = 0;
      rf = swe_lun_occult_when_glob(2451545.0, SE_VENUS, NULL, SEFLG_SWIEPH, 0,
                                    tret, SE_ECL_ONE_TRY, serr);
      row("cov:lun_occ_glob[onetry]", rf, tret, 7, serr);

      /* The local searches, pointed where the geometry actually is.
       *
       * Everything above asks about one arbitrary place, which finds a
       * partial eclipse or nothing, and leaves eclipse_when_loc()'s central
       * branches unreached: the `retflag = SE_ECL_TOTAL` and
       * `SE_ECL_ANNULAR` arms and the contact-timing block that only runs
       * for a central eclipse.
       *
       * The dates and places are not guesses. swe_sol_eclipse_when_glob()
       * was asked for an eclipse of each type and swe_sol_eclipse_where()
       * for the point it is central over, and those answers are what is
       * pinned here -- the library was used to find the circumstances that
       * exercise the library. They are written out as constants rather than
       * recomputed at run time so that these rows test the local search
       * alone, not a chain of three functions. */
      { static const struct { double t, lon, lat; const char *n; } cen[] = {
          { 2452258.369414, -130.7016,   0.6351, "annular" },
          { 2453469.358171, -118.9839, -10.5676, "total"   },
        };
        for (size_t k = 0; k < sizeof(cen)/sizeof(cen[0]); k++) {
          double g[3] = { cen[k].lon, cen[k].lat, 0 };
          memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr);
          *serr = 0;
          rf = swe_sol_eclipse_when_loc(cen[k].t - 5, SEFLG_SWIEPH, g,
                                        tret, attr, FALSE, serr);
          snprintf(tag, sizeof tag, "cov:sol_ecl_loc[%s]", cen[k].n);
          row(tag, rf, tret, 7, serr);
        }
      }

      /* occult_when_loc's two remaining shapes. A star far off the ecliptic
       * can never be occulted and the function says so by name rather than
       * searching; an asteroid takes the branch that derives the occulted
       * body's radius from ast_diam instead of a table. */
      { char st[AS_MAXCH]; strcpy(st, "Polaris");
        memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr); *serr = 0;
        rf = swe_lun_occult_when_loc(2451545.0, 0, st, SEFLG_SWIEPH,
                                     geo, tret, attr, FALSE, serr);
        row("cov:lun_occ_loc[polaris]", rf, tret, 7, serr); }
      memset(tret, 0, sizeof tret); memset(attr, 0, sizeof attr); *serr = 0;
      rf = swe_lun_occult_when_loc(2451545.0, SE_AST_OFFSET + 1, NULL,
                                   SEFLG_SWIEPH, geo, tret, attr, FALSE, serr);
      row("cov:lun_occ_loc[ceres]", rf, tret, 7, serr);

      /* swe_pheno() at 62%: the rows elsewhere ask geocentrically for the
       * Sun through Saturn. Heliocentric and topocentric take different
       * flag paths at the top, the Moon has its own branch, and an
       * out-of-range body is refused. */
      { memset(attr, 0, sizeof attr); *serr = 0;
        rf = swe_pheno(2451545.0, SE_JUPITER, SEFLG_SWIEPH|SEFLG_HELCTR, attr, serr);
        row("cov:pheno[helctr]", rf, attr, 8, serr);
        memset(attr, 0, sizeof attr); *serr = 0;
        rf = swe_pheno(2451545.0, SE_MOON, SEFLG_SWIEPH|SEFLG_TOPOCTR, attr, serr);
        row("cov:pheno[topoctr_moon]", rf, attr, 8, serr);
        memset(attr, 0, sizeof attr); *serr = 0;
        rf = swe_pheno(2451545.0, SE_NPLANETS + 1, SEFLG_SWIEPH, attr, serr);
        row("cov:pheno[refused]", rf, attr, 8, serr);
        /* The two remappings at the top of the function, which nothing had
         * reached: an object numbered as minor planet 134340 is Pluto and
         * becomes SE_PLUTO, and minor planets 1 to 4 become SE_CERES
         * onwards, because those four have their own ephemeris rather than
         * living in the asteroid files. */
        memset(attr, 0, sizeof attr); *serr = 0;
        rf = swe_pheno(2451545.0, SE_AST_OFFSET + 134340, SEFLG_SWIEPH, attr, serr);
        row("cov:pheno[ast_pluto]", rf, attr, 8, serr);
        memset(attr, 0, sizeof attr); *serr = 0;
        rf = swe_pheno(2451545.0, SE_AST_OFFSET + 1, SEFLG_SWIEPH, attr, serr);
        row("cov:pheno[ast_ceres]", rf, attr, 8, serr); }
    }
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
    reset_astro_models();
    *serr = 0; rf = swe_calc(2451545.0, SE_SUN, SEFLG_SWIEPH, x, serr);
    row("cov:set_astro_models", rf, x, 6, serr);

    swe_set_interpolate_nut(TRUE);
    *serr = 0; rf = swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
    row("cov:interpolate_nut", rf, x, 6, serr);
    swe_set_interpolate_nut(FALSE);

    /* The five nutation models, all at ONE instant, through swe_sidtime().
     *
     * Two things at once. The models had no coverage at all -- nothing
     * outside the default SEMOD_NUT_IAU_2000B was ever computed -- and the
     * shared instant makes these rows a probe for calc_nutation()'s memo,
     * which is keyed on astro_models[SE_MODEL_NUT] among other things. Drop
     * the model from that key and every row below returns model 1's
     * nutation, so all five collapse to the same number and this fails.
     *
     * swe_sidtime() rather than swe_calc() precisely because it does NOT
     * cache: swe_calc() would answer the second and later calls out of
     * savedat[] on (tjd, ipl, iflag) without recomputing anything, and the
     * rows would agree whatever the memo did. It is also the call that made
     * the memo worth having -- it reaches swi_nutation() directly rather
     * than through swi_check_nutation(). */
    for (int nm = SEMOD_NUT_IAU_1980; nm <= SEMOD_NUT_WOOLARD; nm++) {
      char sam[AS_MAXCH], tag[64];
      snprintf(sam, sizeof sam, "0,0,0,%d", nm);
      swe_set_astro_models(sam, 0);
      x[0] = swe_sidtime(2451545.0);
      snprintf(tag, sizeof tag, "cov:nutmodel[%d]", nm);
      row(tag, 0, x, 1, "");
    }
    reset_astro_models();

    /* Changing the nutation model has to reach a position already computed
     * for that instant. swi_check_nutation() keeps the computed nutation
     * keyed on tjd and the speed flag ALONE -- no model -- and
     * swi_force_app_pos_etc() clears pldat/nddat/savedat but not that, so
     * the second call here used to be answered with the first model's
     * nutation however loudly the caller had asked for another one.
     * sweconfig.c clears it on a model change now, and this pair is what
     * says so: undo that and [after] becomes byte-identical to [before]. */
    *serr = 0; rf = swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
    row("cov:nutswitch[before]", rf, x, 6, serr);
    { char sam[AS_MAXCH];
      snprintf(sam, sizeof sam, "0,0,0,%d", SEMOD_NUT_IAU_1980);
      swe_set_astro_models(sam, 0); }
    *serr = 0; rf = swe_calc(2451545.0, SE_MOON, SEFLG_SWIEPH, x, serr);
    row("cov:nutswitch[after]", rf, x, 6, serr);
    reset_astro_models();

    /* The five delta-t models, all at ONE instant. Same two jobs as the
     * nutation rows above: the models had no coverage -- nothing computed
     * anything but the default SEMOD_DELTAT_STEPHENSON_ETC_2016 -- and the
     * shared instant makes these a probe for calc_deltat()'s memo, which is
     * keyed on astro_models[SE_MODEL_DELTAT] among other things. Drop that
     * field and all five collapse to model 1's answer.
     *
     * Year 1000 because that is where the models disagree: from 1620 on
     * they all read the same tabulated values through deltat_aa(), and
     * below 948 the 1984 model switches to Borkowski. SE_MODEL_DELTAT is
     * index 0, so the model string is just the number. */
    for (int dm = SEMOD_DELTAT_STEPHENSON_MORRISON_1984;
         dm <= SEMOD_DELTAT_STEPHENSON_ETC_2016; dm++) {
      char sam[AS_MAXCH], tag[64];
      snprintf(sam, sizeof sam, "%d", dm);
      swe_set_astro_models(sam, 0);
      x[0] = swe_deltat(2086308.0);
      snprintf(tag, sizeof tag, "cov:deltatmodel[%d]", dm);
      row(tag, 0, x, 1, "");
    }
    reset_astro_models();

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

    /* swe_set_ephe_fallback(), and the refusal it exists to switch off.
     *
     * This is the fork's defining behavioural break -- upstream quietly
     * answers from a weaker ephemeris than you asked for, and this fork
     * refuses unless you say otherwise -- and until these rows the SWITCH
     * had no coverage at all. Only the SE_EPHE_FALLBACK environment
     * variable was ever exercised, by G8, and only as a means of making
     * upstream's own suite runnable. swe_set_ephe_fallback() could have
     * been a no-op and every gate would have stayed green.
     *
     * The pair is one request asked twice. de431.eph is not shipped, so
     * SEFLG_JPLEPH cannot be honoured: strict refuses (rf=-1, nothing
     * written), permissive answers from the Swiss files and says so in
     * serr, with the ephemeris bit in the return flag showing what actually
     * produced the numbers. Flip the default and [strict] starts returning
     * a position; make the setter a no-op and [fallback] stops.
     *
     * [strict] repeats cov:set_jpl_file above on purpose. That row exists
     * to record the refusal; this one exists to sit next to [fallback], so
     * the same request answered two ways reads as a pair rather than as two
     * facts fifteen lines apart. */
    { char jf[AS_MAXCH]; strcpy(jf, "de431.eph"); swe_set_jpl_file(jf); }
    x[0] = swe_get_ephe_fallback();
    row("cov:ephe_fallback[default]", 0, x, 1, "");
    *serr = 0; rf = swe_calc(2451545.0, SE_MARS, SEFLG_JPLEPH, x, serr);
    row("cov:ephe_fallback[strict]", rf, x, 6, serr);

    swe_set_ephe_fallback(1);
    x[0] = swe_get_ephe_fallback();
    row("cov:ephe_fallback[set]", 0, x, 1, "");
    *serr = 0; rf = swe_calc(2451545.0, SE_MARS, SEFLG_JPLEPH, x, serr);
    row("cov:ephe_fallback[fallback]", rf, x, 6, serr);

    swe_set_ephe_fallback(0);
    x[0] = swe_get_ephe_fallback();
    row("cov:ephe_fallback[restored]", 0, x, 1, "");

    /* swe_orbit_max_min_true_distance() -- public, and until now called by
     * nothing. It reaches osc_iterate_min_dist()/osc_iterate_max_dist(),
     * which were likewise unreached. Mars for an ordinary planet and the
     * Moon because it takes the geocentric branch. */
    {
      const int32 opl[] = { SE_MARS, SE_MOON };
      for (size_t o = 0; o < sizeof(opl)/sizeof(opl[0]); o++) {
        *serr = 0;
        rf = swe_orbit_max_min_true_distance(2451545.0, opl[o], SEFLG_SWIEPH,
                                             &x[0], &x[1], &x[2], serr);
        snprintf(tag, sizeof tag, "cov:orbit_max_min[%d]", (int) opl[o]);
        row(tag, rf, x, 3, serr);
      }
    }

    /* SE_INTP_APOG and SE_INTP_PERG, the interpolated lunar apsides. The
     * planet loop stops at SE_VESTA (20) and these are 21 and 22, so
     * intp_apsides() was never called. */
    for (int32 p = SE_INTP_APOG; p <= SE_INTP_PERG; p++) {
      *serr = 0;
      rf = swe_calc(2451545.0, p, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
      snprintf(tag, sizeof tag, "cov:intp_apsides[%d]", (int) p);
      row(tag, rf, x, 6, serr);
    }

    /* The three sidereal house branches. houses() reaches only the
     * tropical path because it passes SEFLG_SWIEPH and nothing else;
     * sidereal_houses_trad(), _ecl_t0() and _ssypl() all need
     * SEFLG_SIDEREAL, and the last two need a bit on the sidereal mode.
     *
     * These live HERE rather than in houses() because houses() is part of
     * suite_compute_only(), the subset the worker threads run, and its
     * contract is that a worker calls no setter at all --
     * swe_set_sid_mode() publishes to the shared config master, so a
     * worker doing it changes what every thread that has not claimed the
     * group computes. Putting them there cost two of eight threads on G2. */
    {
      double cusp[37], ascmc[10], csp[37], asp[10];
      const struct { int32 bit; const char *name; } sid[] = {
        { 0,                   "trad"  },
        { SE_SIDBIT_ECL_T0,    "eclt0" },
        { SE_SIDBIT_SSY_PLANE, "ssypl" },
      };
      for (size_t s = 0; s < sizeof(sid)/sizeof(sid[0]); s++) {
        memset(cusp,0,sizeof cusp); memset(ascmc,0,sizeof ascmc);
        memset(csp,0,sizeof csp);   memset(asp,0,sizeof asp);
        ascmc[9] = 99;
        swe_set_sid_mode(SE_SIDM_LAHIRI | sid[s].bit, 0, 0);
        int rc = swe_houses_ex2(2451545.0, SEFLG_SWIEPH | SEFLG_SIDEREAL,
                                47.37, 8.55, 'P', cusp, ascmc, csp, asp, NULL);
        snprintf(tag, sizeof tag, "cov:hsys_sid[%s]", sid[s].name);
        fprintf(TRANSCRIPT, "%-46s rc=%-4d", tag, rc);
        for (int i = 0; i < 13; i++) fprintf(TRANSCRIPT, " %a", cusp[i]);
        fprintf(TRANSCRIPT, "\n");
      }
      swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
    }

    /* Sidereal positions projected onto the solar-system plane. The
     * sidereal section elsewhere never sets SE_SIDBIT_SSY_PLANE, so
     * swi_trop_ra2sid_lon_sosy() -- a distinct projection, not a variant of
     * the traditional one -- was never entered. */
    swe_set_sid_mode(SE_SIDM_LAHIRI | SE_SIDBIT_SSY_PLANE, 0, 0);
    *serr = 0;
    rf = swe_calc(2451545.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SIDEREAL | SEFLG_SPEED, x, serr);
    row("cov:sid_ssy_plane", rf, x, 6, serr);
    swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);

    /* The eleven precession models. SEMOD_PREC_DEFAULT is VONDRAK_2011, so
     * the other ten were arithmetic nothing ran -- precess_2() alone is 76
     * lines at zero coverage, and the Owen 1990 chain
     * (owen_pre_matrix/epsiln_owen_1986/get_owen_t0_icof) another 78.
     * Precession is under every position this library returns, which makes
     * it the worst place to have untested branches.
     *
     * -3000, because swi_epsiln() and swi_precess() both check |T| against
     * a per-model century limit and use the SHORT-term model inside it; a
     * date near J2000 would take the same branch whatever is configured.
     * Both model slots are set to the same value so the dispatch is
     * unambiguous. SE_MODEL_PREC_LONGTERM is index 1 and _SHORTTERM is 2.
     *
     * swe_set_astro_models() clears savedat[] through
     * swi_invalidate_models(), so asking for the same body at the same
     * instant really does recompute rather than answering from the save
     * area -- which is what makes one date enough. */
    for (int32 pm = SEMOD_PREC_IAU_1976; pm <= SEMOD_PREC_NEWCOMB; pm++) {
      char sam[AS_MAXCH];
      snprintf(sam, sizeof sam, "0,%d,%d", (int) pm, (int) pm);
      swe_set_astro_models(sam, 0);
      *serr = 0;
      rf = swe_calc(625307.5, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
      snprintf(tag, sizeof tag, "cov:precmodel[%d]", (int) pm);
      row(tag, rf, x, 6, serr);
    }
    reset_astro_models();

    /* meff(), the one part of the gravitational-deflection maths nothing
     * had ever run.
     *
     * Light from a planet grazing the Sun is bent, and the textbook formula
     * treats the Sun as a point mass, so it diverges as the line of sight
     * approaches the centre. The Astronomical Almanac says to set the
     * deflection to zero there, which puts a step in the planet's apparent
     * motion; this library instead models the Sun's mass distribution, so
     * only the mass interior to the grazing radius bends the light.
     * meff(r) is that fraction, interpolated from a 25-entry table.
     *
     * It runs only while the planet is inside the solar disc as seen from
     * Earth -- sin(elongation) < SUN_RADIUS / r_sun -- which is why no row
     * had reached it. These five are the deepest conjunction each planet
     * makes in 2000-2030, found by scanning elongation; they enter meff()
     * at r = 0.021, 0.134, 0.328, 0.351 and 0.427, so the interpolation is
     * exercised across its range rather than at one point. */
    {
      const struct { double t; int32 ipl; const char *n; } occ[] = {
        { 2457546.412153, SE_VENUS,   "venus"   },   /* r = 0.021 */
        { 2458862.136111, SE_SATURN,  "saturn"  },   /* r = 0.134 */
        { 2459892.222917, SE_MERCURY, "mercury" },   /* r = 0.328 */
        { 2458845.268055, SE_JUPITER, "jupiter" },   /* r = 0.351 */
        { 2460266.724653, SE_MARS,    "mars"    },   /* r = 0.427 */
      };
      for (size_t i = 0; i < sizeof(occ)/sizeof(occ[0]); i++) {
        *serr = 0;
        rf = swe_calc_ut(occ[i].t, occ[i].ipl, SEFLG_SWIEPH | SEFLG_SPEED,
                         x, serr);
        snprintf(tag, sizeof tag, "cov:meff[%s]", occ[i].n);
        row(tag, rf, x, 6, serr);
      }
    }

    /* The small public conversions. Every one of these is exported, and
     * every one had zero coverage: nothing in the suite called them, so
     * they could have returned anything. swe_cs2lonlatstr() and its two
     * neighbours format into a caller-supplied buffer, which is where this
     * codebase's bugs have lived.
     *
     * One row, because the interesting property is that the values are
     * what they were, not that thirteen separate rows exist. */
    {
      char ts[AS_MAXCH], ls[AS_MAXCH], ds[AS_MAXCH];
      double xpo[6] = {123.456, 12.345, 1.5, 0.01, 0.002, 0.0003}, xpn[6];
      centisec cs = 1234567;
      swe_cotrans_sp(xpo, xpn, 23.4392911);
      swe_cs2timestr(cs, ':', FALSE, ts);
      swe_cs2lonlatstr(cs, 'E', 'W', ls);
      swe_cs2degstr(cs, ds);
      x[0] = swe_deg_midp(350.0, 10.0);
      x[1] = swe_rad_midp(6.1, 0.1);
      x[2] = swe_difdegn(10.0, 350.0);
      x[3] = (double) swe_difcsn(360000, 129240000);
      x[4] = (double) swe_difcs2n(360000, 129240000);
      x[5] = (double) swe_csnorm(-360000);
      fprintf(TRANSCRIPT, "%-46s rf=%-6d", "cov:conversions", 0);
      for (int i = 0; i < 6; i++) fprintf(TRANSCRIPT, " %a", x[i]);
      for (int i = 0; i < 6; i++) fprintf(TRANSCRIPT, " %a", xpn[i]);
      fprintf(TRANSCRIPT, " %a %a %a",
              (double) swe_csroundsec(cs), (double) swe_d2l(1234.567),
              (double) swe_day_of_week(2451545.0));
      {
        char cl[AS_MAXCH * 2];
        sanitize(cl, sizeof cl, ts); fprintf(TRANSCRIPT, " | %s", cl);
        sanitize(cl, sizeof cl, ls); fprintf(TRANSCRIPT, " %s", cl);
        sanitize(cl, sizeof cl, ds); fprintf(TRANSCRIPT, " %s", cl);
      }
      fprintf(TRANSCRIPT, "\n");
    }
    { char jf[AS_MAXCH]; strcpy(jf, "de440.eph"); swe_set_jpl_file(jf); }
    swe_close();
    swe_set_ephe_path((char *) EPHE);

    /* Four exported functions with next to nothing behind them. Between them
     * they were the largest untested surface left in the library:
     * swe_house_pos() alone is 453 lines of which 19.6% ran, and the other
     * three sat at or near zero. All four are callable by anyone linking
     * this library. */

    /* swe_refrac(): atmospheric refraction, both directions. Zero coverage.
     * The altitudes straddle the horizon on purpose -- refraction is largest
     * and worst-behaved there, and the function has a branch for going
     * below it. */
    {
      const double alt[] = { 60.0, 10.0, 2.0, 0.0, -0.5, -2.0 };
      for (size_t k = 0; k < sizeof(alt)/sizeof(alt[0]); k++) {
        x[0] = swe_refrac(alt[k], 1013.25, 15.0, SE_TRUE_TO_APP);
        x[1] = swe_refrac(alt[k], 1013.25, 15.0, SE_APP_TO_TRUE);
        /* A thinner atmosphere: the pressure argument has to reach the
         * arithmetic, and 500 mbar is roughly 5500 m up. */
        x[2] = swe_refrac(alt[k], 500.0, -10.0, SE_TRUE_TO_APP);
        snprintf(tag, sizeof tag, "cov:refrac[%.1f]", alt[k]);
        row(tag, 0, x, 3, "");
      }
    }

    /* swe_utc_time_zone(): the UTC/local conversion, zero coverage. The
     * offsets include a negative one, a fractional one (Kathmandu is
     * +5:45), and one large enough to carry the date across a year
     * boundary in each direction -- which is the arithmetic worth pinning,
     * since the day, month and year outputs all move together. */
    {
      const double tz[] = { 0.0, 5.75, -8.0, 13.0, -13.0 };
      for (size_t k = 0; k < sizeof(tz)/sizeof(tz[0]); k++) {
        int32 y2, mo2, d2, h2, mi2; double s2;
        swe_utc_time_zone(2000, 1, 1, 0, 30, 15.5, tz[k],
                          &y2, &mo2, &d2, &h2, &mi2, &s2);
        x[0] = y2; x[1] = mo2; x[2] = d2; x[3] = h2; x[4] = mi2; x[5] = s2;
        snprintf(tag, sizeof tag, "cov:utc_tz[%.2f]", tz[k]);
        row(tag, 0, x, 6, "");
      }
      /* And back the other way, so the round trip is on the record. */
      { int32 y2, mo2, d2, h2, mi2; double s2;
        swe_utc_time_zone(1999, 12, 31, 23, 45, 0.25, -13.0,
                          &y2, &mo2, &d2, &h2, &mi2, &s2);
        x[0] = y2; x[1] = mo2; x[2] = d2; x[3] = h2; x[4] = mi2; x[5] = s2;
        row("cov:utc_tz[wrap]", 0, x, 6, ""); }
    }

    /* swe_gauquelin_sector(): 29% covered, and calc_mer_trans() -- the
     * meridian-transit search it reaches for the higher methods -- at zero.
     * imeth runs 0..5: 0 and 1 use rising and setting, 2 to 5 use the
     * meridian, so the whole range is needed to reach both halves. Method 6
     * and above is rejected, and that refusal is worth a row too.
     *
     * Tagged gauq_meth, not gauquelin: cov:gauquelin[0] and [1] already
     * exist further up, indexed by DATE at imeth 0. Two blocks sharing a
     * tag would not just read badly -- cmpgolden.py keys its rows by tag,
     * so the duplicate would silently drop one of them from every
     * tolerance comparison while the bit-exact diff still passed. */
    {
      double geopos[3] = { 8.55, 47.37, 400 };
      for (int32 im = 0; im <= 6; im++) {
        double dg = 0;
        *serr = 0;
        rf = swe_gauquelin_sector(2451545.0, SE_MARS, NULL, SEFLG_SWIEPH,
                                  im, geopos, 1013.25, 15.0, &dg, serr);
        x[0] = dg;
        snprintf(tag, sizeof tag, "cov:gauq_meth[%d]", (int) im);
        row(tag, rf, x, 1, serr);
      }
      /* A fixed star takes the other branch: starname non-NULL means the
       * position comes from swe_fixstar() rather than swe_calc(). */
      { char st[AS_MAXCH]; double dg = 0;
        strcpy(st, "Aldebaran"); *serr = 0;
        rf = swe_gauquelin_sector(2451545.0, 0, st, SEFLG_SWIEPH,
                                  0, geopos, 1013.25, 15.0, &dg, serr);
        x[0] = dg;
        row("cov:gauq_meth[star]", rf, x, 1, serr); }
    }

    /* calc_mer_trans(), 44 lines at zero. It is reached from
     * swe_rise_trans() and nowhere else, when rsmi asks for a MERIDIAN
     * transit rather than a rising or a setting -- upper (MTRANSIT) or
     * lower (ITRANSIT). Every existing swe_rise_trans row asks for
     * SE_CALC_RISE, so the whole transit half of that function was
     * unreached. A star as well as a planet, because starname takes the
     * other branch inside it. */
    {
      double geo[3] = { 8.55, 47.37, 400 };
      const int32 rs[] = { SE_CALC_MTRANSIT, SE_CALC_ITRANSIT };
      const char *rsn[] = { "m", "i" };
      for (size_t k = 0; k < sizeof(rs)/sizeof(rs[0]); k++) {
        double tr = 0;
        *serr = 0;
        rf = swe_rise_trans(2451545.0, SE_MARS, NULL, SEFLG_SWIEPH, rs[k],
                            geo, 1013.25, 15.0, &tr, serr);
        x[0] = tr;
        snprintf(tag, sizeof tag, "cov:mer_trans[%s]", rsn[k]);
        row(tag, rf, x, 1, serr);
      }
      { char st[AS_MAXCH]; double tr = 0;
        strcpy(st, "Aldebaran"); *serr = 0;
        rf = swe_rise_trans(2451545.0, 0, st, SEFLG_SWIEPH, SE_CALC_MTRANSIT,
                            geo, 1013.25, 15.0, &tr, serr);
        x[0] = tr;
        row("cov:mer_trans[star]", rf, x, 1, serr); }
    }

    /* swe_house_pos(): 453 lines, 19.6% of them run. It answers "which
     * house is this body in, and how far through it", and it carries a
     * separate branch per house system -- which is why one call barely
     * dents it and why this loops over the same 24 letters the hsys[] rows
     * above use.
     *
     * Two latitudes and two positions each. 47 deg is ordinary; 66.7 deg is
     * inside the polar circle, where several systems degenerate and fall
     * back to Porphyry, and that fallback is code too. The second position
     * carries ecliptic latitude, because some systems use it and others
     * discard it, and a row where it changes nothing is as much a fact as
     * one where it does. */
    {
      const char *hs = "PKORCAEVXHTBGWMNQLIUSDFY";
      const double lat[] = { 47.37, 66.7 };
      const double pos[][2] = { { 123.456, 0.0 }, { 300.5, 12.25 } };
      for (const char *h = hs; *h; h++)
        for (size_t la = 0; la < sizeof(lat)/sizeof(lat[0]); la++)
          for (size_t p = 0; p < sizeof(pos)/sizeof(pos[0]); p++) {
            double xpin[2] = { pos[p][0], pos[p][1] };
            *serr = 0;
            x[0] = swe_house_pos(90.0, lat[la], 23.4392911, (int) *h, xpin, serr);
            snprintf(tag, sizeof tag, "cov:house_pos[%c,%zu,%zu]", *h, la, p);
            row(tag, 0, x, 1, serr);
          }
    }

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

  /* swephlib.c was the least-covered file at 77.8%; these were its reachable
   * holes.
   *
   * swe_sidtime0() at 0%: the transcript reaches sidereal time only through
   * swe_sidtime(), which derives obliquity and nutation itself. The form
   * where the CALLER supplies them is a separate entry point nothing used. */
  {
    double eps = 23.4392911, nut = -0.00389;
    x[0] = swe_sidtime0(2451545.0, eps, nut);
    x[1] = swe_sidtime0(1356173.5, eps, nut);   /* -3000, far from the fit */
    row("cov:sidtime0", 0, x, 2, "");
  }

  /* The five model-description switches read 17% to 57%: the transcript only
   * ever selected a handful of models. A '+' in samod makes
   * swe_get_astro_models() enumerate all of them in one call, walking every
   * case. The output is ~1.9 KB over many lines, so the row records length
   * and a byte sum -- one line, and it still moves if a description does. */
  {
    static char sdet[8000];    /* the header now documents 4000 */
    char sam[AS_MAXCH];
    unsigned long sum = 0;
    strcpy(sam, "+");
    memset(sdet, 0, sizeof sdet);
    swe_get_astro_models(sam, sdet, SEFLG_SWIEPH);
    for (const char *p = sdet; *p; p++) sum = sum * 31u + (unsigned char) *p;
    x[0] = (double) strlen(sdet);
    x[1] = (double) (sum & 0xffffffUL);
    row("cov:astro_models_all", 0, x, 2, "");
    reset_astro_models();
  }

  /* deltat_longterm_morrison_stephenson() at 0%, and the three models that
   * reach it at 27/52/57%. The existing cov:deltatmodel[] rows select every
   * model but ask at year 1000, inside the tables, so the extrapolation below
   * was never entered -- and an extreme date under the DEFAULT model does not
   * help either, since Stephenson etc. 2016 carries its own long-term branch.
   * It needs the older models AND an early date: Espenak-Meeus falls through
   * below -500, the 1997 and 2004 families below -1000. Two dates per model,
   * one either side of -1000, because the pair is what separates them. */
  {
    static const double DT_FAR[2] = { -3026613.5, 1721057.5 };  /* ~-20000, ~-700 */
    for (int dm = SEMOD_DELTAT_STEPHENSON_MORRISON_1984;
         dm <= SEMOD_DELTAT_STEPHENSON_ETC_2016; dm++) {
      char sam[AS_MAXCH], tg[64];
      snprintf(sam, sizeof sam, "%d", dm);
      swe_set_astro_models(sam, 0);
      for (int d = 0; d < 2; d++) {
        *serr = 0;
        x[0] = swe_deltat_ex(DT_FAR[d], SEFLG_SWIEPH, serr);
        snprintf(tg, sizeof tg, "cov:deltat_longterm[%d,%d]", dm, d);
        row(tg, 0, x, 1, serr);
      }
    }
    reset_astro_models();
  }

  /* plaus_iflag()'s JPL Horizons handling, none of which ran. SEFLG_JPLHOR
   * survives only alongside SEFLG_JPLEPH, and with no EOP data loaded it
   * downgrades to SEFLG_JPLHOR_APPROX and says which of the four reasons it
   * was. The calc then fails on the missing JPL file, which is what the row
   * records -- the flag reasoning happens before that and is the point. The
   * SEMOD_JPLHORA_2 row reaches the frame-bias branch, which the default
   * model (3) skips. */
  {
    char sam[AS_MAXCH];
    *serr = 0; rf = swe_calc(DATES[0], SE_MARS, SEFLG_JPLEPH|SEFLG_JPLHOR, x, serr);
    row("cov:jplhor[downgrade]", rf, x, 6, serr);
    *serr = 0; rf = swe_calc(DATES[0], SE_MARS, SEFLG_JPLEPH|SEFLG_JPLHOR_APPROX, x, serr);
    row("cov:jplhor[approx]", rf, x, 6, serr);
    /* JPLHOR is also stripped for the nodes and apogees, and for fictitious
     * bodies, before any of that is considered. */
    *serr = 0; rf = swe_calc(DATES[0], SE_MEAN_NODE, SEFLG_JPLEPH|SEFLG_JPLHOR, x, serr);
    row("cov:jplhor[node]", rf, x, 6, serr);
    *serr = 0; rf = swe_calc(DATES[0], SE_FICT_OFFSET, SEFLG_JPLEPH|SEFLG_JPLHOR, x, serr);
    row("cov:jplhor[fict]", rf, x, 6, serr);
    snprintf(sam, sizeof sam, "0,0,0,0,0,0,%d,0", SEMOD_JPLHORA_2);
    swe_set_astro_models(sam, 0);
    *serr = 0; rf = swe_calc(DATES[0], SE_MARS, SEFLG_JPLEPH|SEFLG_JPLHOR_APPROX, x, serr);
    row("cov:jplhor[hora2]", rf, x, 6, serr);
    reset_astro_models();
  }

  /* quadratic_intp() at 0%: it interpolates nutation between whole days and
   * runs only under swe_set_interpolate_nut(), which nothing enabled. Switched
   * back off afterwards -- it is global state later rows would inherit. */
  {
    swe_set_interpolate_nut(TRUE);
    *serr = 0; rf = swe_calc(2451545.3, SE_MOON, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    row("cov:interp_nut[on]", rf, x, 6, serr);
    *serr = 0; rf = swe_calc(2451545.7, SE_MOON, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    row("cov:interp_nut[mid]", rf, x, 6, serr);
    swe_set_interpolate_nut(FALSE);
    *serr = 0; rf = swe_calc(2451545.3, SE_MOON, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    row("cov:interp_nut[off]", rf, x, 6, serr);
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

/* wrote and read_back are what capture_close() saw, kept apart so a failure
 * can say whether the transcript was short because the worker produced less
 * or because reading it back returned less. (read_back rather than read:
 * <unistd.h> is in scope and a member called read beside it reads badly.)
 * path survives the run when a thread mismatches, so there is something on
 * disk to look at. */
struct targ {
  int id; char *buf; size_t len; int setup;
  long wrote; size_t read_back; char path[64];
  int io_flush, io_ferror, io_close, io_errno;   /* see capture_close() */
};

/* Capturing each worker's transcript used to call open_memstream(), which is
 * POSIX and has no MSVC equivalent, so G2 -- the gate this file exists for --
 * did not run on Windows at all. A per-thread temp file is standard C and
 * hands the comparison below exactly what it wants, a->buf and a->len. One
 * implementation for every platform, so Windows exercises the same path.
 *
 * "w+b": no newline translation, so the bytes are the bytes on either side.
 *
 * The process id is in the name because more than one golden runs at once.
 * `make check` sets -j$(NPROC), and check-threads and
 * check-threads-workaround each start ./golden --threads 8 in this same
 * directory. Keyed on the thread id alone, both opened
 * .golden_capture_0.tmp .. _7.tmp, and "w+b" truncates -- so one process
 * could empty a file the other was still writing.
 *
 * That is wrong on its own terms and this fixes it. It is NOT established
 * that it caused the intermittent thread mismatch seen on this tree, where
 * a worker's transcript comes back a prefix of the reference: that was
 * observed twice, on main as well as here, and has not been reproducible
 * since -- not by racing two golden processes, not by `make -j2` on the two
 * thread gates, and not over six cold `make check` runs with this name
 * collision still in place. Both sightings were while the machine was
 * heavily loaded. See notes/REVIEW.md; if it returns, this is not the
 * explanation. */
static FILE *capture_open(int id, char *path, size_t n) {
#if defined(_WIN32)
  unsigned long pid = (unsigned long) GetCurrentProcessId();
#else
  unsigned long pid = (unsigned long) getpid();
#endif
  snprintf(path, n, ".golden_capture_%lu_%d.tmp", pid, id);
  return fopen(path, "w+b");
}

static void capture_close(struct targ *a, FILE *f) {
  long end;
  /* Nothing here ever checked whether the writes landed. fprintf() returns
   * a count nobody reads, fflush() a status nobody reads, and a stream that
   * has failed simply stops accepting bytes -- which produces exactly the
   * symptom G2 keeps reporting: a transcript that is a strict prefix of the
   * reference, with nothing to say why. A full disk, a short write, an
   * interrupted one: all of them look like the library computing less.
   *
   * These are the four places the truth is available. ferror() after the
   * flush covers everything written during the run; fclose() is where a
   * deferred write error finally surfaces, so its return matters too. */
  a->io_flush  = fflush(f);
  a->io_ferror = ferror(f);
  a->io_errno  = (a->io_flush != 0 || a->io_ferror != 0) ? errno : 0;
  end = ftell(f);
  a->wrote = end;
  a->len = end > 0 ? (size_t) end : 0;
  a->buf = malloc(a->len + 1);
  if (a->buf != NULL) {
    rewind(f);
    a->len = fread(a->buf, 1, a->len, f);
    a->buf[a->len] = '\0';
  } else {
    a->len = 0;
  }
  a->read_back = a->len;
  a->io_close = fclose(f);
  if (a->io_close != 0 && a->io_errno == 0) a->io_errno = errno;
  /* Deliberately NOT removed here. main() deletes these once every thread
   * has matched; a run that fails leaves them behind, because the one thing
   * the old report could not tell you was what the file actually held. */
}

static thr_ret_t THR_CALL thread_suite(void *p) {
  struct targ *a = p;
  char path[64];
  FILE *f = capture_open(a->id, path, sizeof path);
  snprintf(a->path, sizeof a->path, "%s", path);
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
  capture_close(a, f);
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

  struct targ ref = { -1, NULL, 0, 0, 0, 0, "", 0, 0, 0, 0 };
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
  int bad = 0, io_bad = 0;
  /* A stream that failed is reported on its own, before any comparison, and
   * fails the run whatever the bytes say. Two reasons. A transcript that was
   * not written in full makes the comparison meaningless rather than merely
   * wrong, so "8/8 matched" would be a lie about a run that lost data. And
   * the report can name the cause -- ENOSPC, EFBIG, EIO -- instead of
   * leaving a short transcript to be diagnosed from its length. This is the
   * check that would have said whether the G2 sightings were the library or
   * the disk; it did not exist when they happened.
   *
   * Counted apart from `bad` so the "N/M threads matched" line below stays
   * about threads. The reference has a stream too, and folding its failure
   * into the same counter made that line read -1/4. */
  for (int i = -1; i < nthreads; i++) {
    struct targ *w = (i < 0) ? &ref : &a[i];
    char who[32];
    if (w->io_flush == 0 && w->io_ferror == 0 && w->io_close == 0) continue;
    io_bad++;
    if (i < 0) snprintf(who, sizeof who, "reference");
    else       snprintf(who, sizeof who, "thread %d", i);
    fprintf(stderr,
            "%s: TRANSCRIPT I/O FAILED -- fflush=%d ferror=%d fclose=%d errno=%d (%s)\n"
            "  the capture was not written in full, so any comparison of it is void\n",
            who, w->io_flush, w->io_ferror, w->io_close, w->io_errno,
            w->io_errno ? strerror(w->io_errno) : "no errno recorded");
  }
  /* If the reference itself did not survive, there is nothing to compare
   * against and every thread would be reported as differing from a
   * truncated file. Say that once instead. */
  if (ref.io_flush || ref.io_ferror || ref.io_close) {
    fprintf(stderr, "the reference transcript is incomplete; thread comparison skipped\n");
    fprintf(stderr, "%d/%d threads matched the main-thread transcript%s\n",
            0, nthreads, setup ? " (--per-thread-setup)" : "");
    swe_close();
    return 1;
  }
  for (int i = 0; i < nthreads; i++) {
    /* buf is NULL when the worker could not open or allocate its capture at
     * all. The comparison below indexes both buffers, so say so and move on
     * rather than dereferencing it -- the old code walked into that on the
     * one path where a capture file cannot be created. */
    if (a[i].buf == NULL || ref.buf == NULL) {
      bad++;
      fprintf(stderr, "thread %d: NO TRANSCRIPT (capture failed; %s)\n",
              i, a[i].path[0] ? a[i].path : "no file was opened");
      continue;
    }
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

      /* Which KIND of mismatch, because the two want opposite
       * investigations and the lines above cannot tell them apart. A
       * truncated transcript prints as an empty "thread:" line, which looks
       * like a wrong value and is not one.
       *
       * SHORT   the worker's bytes are a prefix of the reference. Nothing
       *         computed differently; output went missing. Compare wrote
       *         against read below -- if they disagree, reading the file
       *         back came up short, and if they agree the worker really did
       *         stop early.
       * LONGER  more output than the reference, which no ordinary failure
       *         produces and would suggest two writers sharing a file.
       * DIVERGED  same length, different bytes: an actual disagreement, and
       *         the only one of the three that is about the library.
       */
      {
        const char *kind =
          (a[i].len < ref.len && off == a[i].len) ? "SHORT (a prefix of the reference)" :
          (a[i].len > ref.len && off == ref.len)  ? "LONGER than the reference"        :
          (a[i].len == ref.len)                   ? "DIVERGED (same length, different bytes)" :
                                                    "DIVERGED, and a different length too";
        fprintf(stderr,
                "  kind  : %s\n"
                "  bytes : reference %zu, thread %zu, first difference at %zu\n"
                "  capture: ftell reported %ld, fread returned %zu%s\n"
                "  file  : %s (kept for inspection)\n",
                kind, ref.len, a[i].len, off,
                a[i].wrote, a[i].read_back,
                (a[i].wrote >= 0 && (size_t) a[i].wrote != a[i].read_back)
                  ? "  <-- SHORT READ: the file held more than was read back" : "",
                a[i].path);
      }
    }
  }
  /* Only on success. A failed run leaves every capture file in place --
   * including the reference's -- so the transcripts can be diffed directly
   * rather than reconstructed from the one line printed above. */
  if (bad == 0) {
    remove(ref.path);
    for (int i = 0; i < nthreads; i++) remove(a[i].path);
  } else {
    fprintf(stderr, "reference transcript: %s\n", ref.path);
  }
  /* The I/O count rides on this line rather than only appearing above it.
   * "4/4 threads matched" printed underneath a stream that failed is the
   * exact reassurance this whole change exists to stop giving. */
  fprintf(stderr, "%d/%d threads matched the main-thread transcript%s%s",
          nthreads - bad, nthreads, setup ? " (--per-thread-setup)" : "",
          io_bad ? "" : "\n");
  if (io_bad)
    fprintf(stderr, " -- but %d transcript%s incomplete, so that count means"
                    " less than it says\n", io_bad, io_bad == 1 ? " was" : "s were");
  swe_close();
  return (bad || io_bad) ? 1 : 0;
}
