/* ctxtest.c -- G4: two swe_ctx handles really are independent.
 *
 * Everything else on this branch proves the OLD behaviour survived. G1 is
 * bit-exactness of the legacy path; G2 is that threads sharing the default
 * context agree. Neither can tell whether swe_ctx_new() does anything at
 * all -- a swe_X_r() that ignored its context argument entirely would pass
 * both. This is the test that would fail.
 *
 * Four properties, each able to fail on its own:
 *
 *   1. INDEPENDENCE      two contexts holding different sidereal modes and
 *                        different observer positions return different
 *                        answers, each matching what the process-wide API
 *                        returns when configured the same way.
 *
 *   2. NO LEAK           configuring context B does not move context A, and
 *                        neither moves the process-wide default. This is
 *                        the property the _r setters would have broken by
 *                        inheriting swi_config_publish().
 *
 *   3. CONCURRENCY       the same, with the two contexts driven from two
 *                        threads at once. The contract is one thread per
 *                        context, not one context per process.
 *
 *   4. INHERITANCE       swe_ctx_new() picks up the configuration already
 *                        published through the legacy setters, rather than
 *                        starting from library defaults and falling back to
 *                        Moshier -- the decision in PHASE3-API.md section 5.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "swephexp.h"

/* Same shim as threadshim.c: MSVC has no <pthread.h>, and this test must
 * run on the Windows job to be worth anything. */
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

#define TJD   2451545.0
#define IFLAG (SEFLG_SWIEPH | SEFLG_SPEED)
#define NLOOP 400

static const char *g_ephe = "../ephe";
static int failures = 0;

static void fail(const char *what, const char *detail)
{
  printf("  FAIL: %s -- %s\n", what, detail);
  failures++;
}

/* Bit-exact: these are the same computation on the same inputs, so any
 * difference at all is a real difference, not rounding. */
static int same(const double *a, const double *b, int n)
{
  int i;
  for (i = 0; i < n; i++)
    if (memcmp(&a[i], &b[i], sizeof(double)) != 0) return 0;
  return 1;
}

static void show(const char *tag, const double *x)
{
  printf("    %-26s %.9f %.9f %.9f\n", tag, x[0], x[1], x[2]);
}

/*--------------------------------------------------------------------
 * Reference values, taken through the process-wide API so the contexts
 * are compared against the behaviour callers already rely on.
 *------------------------------------------------------------------*/
static void reference(int32 sidmode, double lon, double lat, double *xx)
{
  char serr[AS_MAXCH];
  swe_close();
  swe_set_ephe_path((char *) g_ephe);
  swe_set_sid_mode(sidmode, 0, 0);
  swe_set_topo(lon, lat, 0);
  swe_calc_ut(TJD, SE_MOON, IFLAG | SEFLG_SIDEREAL | SEFLG_TOPOCTR, xx, serr);
}

static void configure(swe_ctx *c, int32 sidmode, double lon, double lat)
{
  swe_set_ephe_path_r(c, (char *) g_ephe);
  swe_set_sid_mode_r(c, sidmode, 0, 0);
  swe_set_topo_r(c, lon, lat, 0);
}

static void compute(swe_ctx *c, double *xx)
{
  char serr[AS_MAXCH];
  swe_calc_ut_r(c, TJD, SE_MOON,
                IFLAG | SEFLG_SIDEREAL | SEFLG_TOPOCTR, xx, serr);
}


/* A fresh thread reading the process-wide configuration.
 *
 * The leak cannot be seen from the main thread: it set PATH/SID/TOPO
 * itself, so cfg_local marks those groups as its own and swi_config_sync()
 * deliberately skips them. That is correct Phase 2 behaviour. A thread that
 * has expressed no opinion adopts the master wholesale, which is exactly
 * what a leaked publish would poison. */
static double g_probe[6];

static thr_ret_t THR_CALL probe_default(void *arg)
{
  char serr[AS_MAXCH];
  (void) arg;
  swe_calc_ut(TJD, SE_MOON, IFLAG | SEFLG_SIDEREAL | SEFLG_TOPOCTR,
              g_probe, serr);
  return (thr_ret_t) 0;
}

static void probe(double *out)
{
  thr_t t;
  memset(g_probe, 0, sizeof g_probe);
  if (thr_create(&t, probe_default, NULL) != 0) { fail("probe", "no thread"); return; }
  thr_join(t);
  memcpy(out, g_probe, sizeof g_probe);
}

/*--------------------------------------------------------------------
 * 3. Concurrency: each thread drives its own context, in a loop, so a
 *    context that reached shared state would be caught by interleaving
 *    rather than by luck.
 *------------------------------------------------------------------*/
struct job { swe_ctx *ctx; double want[6]; int stable; };

static thr_ret_t THR_CALL worker(void *arg)
{
  struct job *j = (struct job *) arg;
  double xx[6];
  int i;
  j->stable = 1;
  for (i = 0; i < NLOOP; i++) {
    compute(j->ctx, xx);
    if (!same(xx, j->want, 3)) { j->stable = 0; break; }
  }
  return (thr_ret_t) 0;
}

int main(int argc, char **argv)
{
  double refA[6], refB[6], a[6], b[6], d[6], a2[6];
  swe_ctx *A, *B;
  struct job jA, jB;
  thr_t tA, tB;
  int i;

  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "--ephe") == 0 && i + 1 < argc) g_ephe = argv[++i];

  /* Fagan/Bradley at Greenwich versus Lahiri at Sydney: two different
   * ayanamsas AND two different observers, so a leak in either group
   * shows up. */
  reference(SE_SIDM_FAGAN_BRADLEY,   0.0,  51.5, refA);
  reference(SE_SIDM_LAHIRI,        151.2, -33.9, refB);
  swe_close();

  printf("G4: independent contexts\n");
  printf("  reference (process-wide API):\n");
  show("A  fagan/bradley @UK", refA);
  show("B  lahiri @sydney",    refB);

  if (same(refA, refB, 3)) {
    fail("test fixture", "the two configurations give the SAME answer, so "
                         "this test could never detect a leak");
    printf("FAIL\n");
    return 1;
  }

  A = swe_ctx_new();
  B = swe_ctx_new();
  if (A == NULL || B == NULL) { fail("swe_ctx_new", "returned NULL"); printf("FAIL\n"); return 1; }

  /* 1. INDEPENDENCE ------------------------------------------------ */
  configure(A, SE_SIDM_FAGAN_BRADLEY,   0.0,  51.5);
  configure(B, SE_SIDM_LAHIRI,        151.2, -33.9);
  compute(A, a);
  compute(B, b);
  printf("  contexts:\n");
  show("A", a);
  show("B", b);
  if (!same(a, refA, 3)) fail("independence", "context A does not match its reference");
  if (!same(b, refB, 3)) fail("independence", "context B does not match its reference");
  if (same(a, b, 3))
    fail("independence", "A and B agree -- the context argument is being ignored");

  /* 2. NO LEAK ----------------------------------------------------- */
  configure(B, SE_SIDM_LAHIRI, 151.2, -33.9);   /* reconfigure B again */
  compute(A, a2);
  if (!same(a2, a, 3))
    fail("no leak", "configuring B moved A");

  /* The default context is where a leak actually lands. A non-default
   * context never syncs from the master (swi_config_sync refuses it), so
   * publishing from B could not move A even if the guard were gone -- but
   * it WOULD move the process-wide default, which does sync.
   *
   * This has to be measured with SIDEREAL|TOPOCTR set. An earlier version
   * computed the default tropical and geocentric, where a leaked ayanamsa
   * and observer position cannot change the answer: the test passed with
   * the publish guard deliberately removed, i.e. it was measuring nothing. */
  {
    char serr[AS_MAXCH];
    double before[6], after[6];
    swe_close();
    swe_set_ephe_path((char *) g_ephe);
    swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
    swe_set_topo(0.0, 51.5, 0);
    swe_calc_ut(TJD, SE_MOON, IFLAG, d, serr);   /* make the main thread publish */

    probe(before);                               /* fresh thread: adopts the master */

    /* Now reconfigure an explicit context to something quite different. If
     * its setters publish, the master moves and the NEXT fresh thread sees
     * a configuration nobody asked for. */
    configure(A, SE_SIDM_LAHIRI, 151.2, -33.9);
    configure(B, SE_SIDM_LAHIRI, 151.2, -33.9);
    compute(A, a2);
    compute(B, b);

    probe(after);
    if (!same(before, after, 3)) {
      fail("no leak", "configuring a context leaked into the process-wide master");
      show("fresh thread before", before);
      show("fresh thread after",  after);
    }
    if (same(a2, refA, 3))
      fail("independence", "reconfiguring A changed nothing");
  }

  /* 3. CONCURRENCY ------------------------------------------------- */
  configure(A, SE_SIDM_FAGAN_BRADLEY,   0.0,  51.5);
  configure(B, SE_SIDM_LAHIRI,        151.2, -33.9);
  jA.ctx = A; memcpy(jA.want, refA, sizeof refA);
  jB.ctx = B; memcpy(jB.want, refB, sizeof refB);
  if (thr_create(&tA, worker, &jA) != 0 || thr_create(&tB, worker, &jB) != 0) {
    fail("concurrency", "could not create threads");
  } else {
    thr_join(tA); thr_join(tB);
    if (!jA.stable) fail("concurrency", "context A drifted while B ran");
    if (!jB.stable) fail("concurrency", "context B drifted while A ran");
    printf("  concurrent: %d iterations each, both stable\n", NLOOP);
  }

  swe_ctx_free(A);
  swe_ctx_free(B);

  /* 4. INHERITANCE -------------------------------------------------- */
  {
    double inh[6], want[6];
    swe_ctx *C;
    reference(SE_SIDM_LAHIRI, 151.2, -33.9, want);
    /* the process-wide state is now Lahiri @ Sydney; a new context must
     * adopt it rather than starting from library defaults */
    C = swe_ctx_new();
    if (C == NULL) { fail("swe_ctx_new", "returned NULL"); }
    else {
      compute(C, inh);
      if (!same(inh, want, 3))
        fail("inheritance", "swe_ctx_new() did not adopt the published configuration");
      swe_ctx_free(C);
    }
  }

  /* swe_ctx_free() must tolerate NULL and must refuse the default context
   * rather than freeing memory it does not own. */
  swe_ctx_free(NULL);

  printf("%s\n", failures ? "FAIL" : "PASS");
  return failures != 0;
}
