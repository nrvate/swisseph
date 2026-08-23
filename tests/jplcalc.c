/* jplcalc.c -- G10: numerical regression coverage for the JPL reader.
 *
 * WHY THIS EXISTS
 *
 * swejpl.c's state() and interp() -- the record reader and the Chebyshev
 * evaluator, ~250 lines between them -- are executed ZERO times by every
 * other gate on this branch. Measured, not assumed: an fprintf at the top
 * of interp() fires 0 times for the whole 5137-row tests/golden suite and
 * 0 times for tests/bench.
 *
 * The reason is that they are reached only through swi_pleph(), which
 * jplplan() calls only once a JPL .eph file is open, and this repository
 * ships .se1 files and no .eph at all. tests/jplguard.c exercises header
 * REJECTION, which is a different code path that returns before any
 * computation happens.
 *
 * So Phase 3c restructured both functions -- moving seven statics into
 * struct jpl_save, changing interp()'s signature, rewriting its Chebyshev
 * cache -- with every gate green and not one line of it ever run. That is
 * not a state this branch should stay in.
 *
 * WHAT IT ACTUALLY CHECKS
 *
 * A synthetic .eph is generated with deterministic coefficients, and
 * swi_pleph() is called across bodies, centres and dates. The output is
 * printed as C99 %a hex floats and compared against a checked-in baseline,
 * exactly as tests/golden does.
 *
 * The numbers are NOT astronomically meaningful -- the coefficients are
 * synthetic, so the "positions" are arbitrary. That is fine and is the
 * point: this is a regression test over the reader's arithmetic and record
 * addressing, not an accuracy test. If a refactor changes how a record is
 * located, how ipt[] offsets are applied, how the Chebyshev recurrence is
 * seeded, or how the cache in js->np/nv/nac/njk/twot is reset, these
 * numbers move.
 *
 * swi_pleph() is called directly rather than through swe_calc(), so that
 * higher-level plausibility handling cannot mask a reader fault.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "swephexp.h"
#include "sweph.h"
#include "swejpl.h"

/* Fixture geometry. Small on purpose: a record holds one block per body,
 * and every block must fit inside ncoeffs doubles. */
#define F_NCF   8        /* Chebyshev coefficients per component */
#define F_NA    2        /* subintervals per record                */
#define F_SS0   2451536.5
#define F_SS2   32.0     /* days per record                        */
#define F_NSEG  16
#define F_SS1   (F_SS0 + F_NSEG * F_SS2)

static void put32(unsigned char *p, int32 v)  { memcpy(p, &v, 4); }
static void putd (unsigned char *p, double v) { memcpy(p, &v, 8); }

/* Deterministic, portable and bit-reproducible across platforms: no
 * rand(), no floating-point library calls whose last bit could differ. The
 * values are built from integers and exact binary fractions. */
static double coeff(int block, int j)
{
  double v = (double) ((block * 7919 + j * 104729) % 2003) / 2048.0;
  return (j == 0) ? (1.0e8 + block * 1.0e7) : v * 1.0e5;
}

/* Layout of one data record, in 1-based double offsets:
 *
 *   1,2                      segment start and end epoch
 *   then, in this order:     bodies 0..9, nutation, libration, body 10
 *
 * Body 10 (the Sun) MUST come last. fsizer() picks khi as the body with
 * the largest ipt[] offset and derives ksize from it, and ncoeffs works
 * out to exactly the end of that block -- so anything placed after body 10
 * would fall outside the record.
 */
static int32 g_ipt[39];
static int32 g_ncoeffs, g_ksize, g_irecsz;

static void build_layout(void)
{
  int i;
  int32 off = 3;                 /* 1-based; [1] and [2] are the epochs */
  for (i = 0; i < 10; i++) {     /* bodies 0..9, 3 components           */
    g_ipt[i*3+0] = off; g_ipt[i*3+1] = F_NCF; g_ipt[i*3+2] = F_NA;
    off += F_NCF * 3 * F_NA;
  }
  g_ipt[33] = off; g_ipt[34] = F_NCF; g_ipt[35] = F_NA;   /* nutation, 2 */
  off += F_NCF * 2 * F_NA;
  g_ipt[36] = off; g_ipt[37] = F_NCF; g_ipt[38] = F_NA;   /* libration, 3 */
  off += F_NCF * 3 * F_NA;
  g_ipt[30] = off; g_ipt[31] = F_NCF; g_ipt[32] = F_NA;   /* body 10 LAST */

  g_ksize   = (g_ipt[30] + 3 * F_NCF * F_NA - 1) * 2;
  g_ncoeffs = g_ksize / 2;
  g_irecsz  = 4 * g_ksize;
}

/* fsizer()'s expected-length formula, mirrored exactly. Getting this wrong
 * is what makes a fixture "mutilated"; the reader compares byte counts. */
static long long expected_len(void)
{
  long long nb = 0;
  int i, k;
  for (i = 0; i < 13; i++) {
    k = (i == 11) ? 2 : 3;
    nb += (long long) g_ipt[i*3+1] * g_ipt[i*3+2] * k * F_NSEG;
  }
  nb += 2 * F_NSEG;
  nb *= 8;
  nb += 2 * (long long) g_ksize * 4;
  return nb;
}

static int write_eph(const char *path)
{
  unsigned char h[2856];
  FILE *fp;
  int i, r, j;
  double *rec;

  memset(h, ' ', sizeof h);
  memcpy(h, "SWISSEPH SYNTHETIC JPL FIXTURE (tests/jplcalc.c)", 48);
  putd (h + 2652, F_SS0);
  putd (h + 2660, F_SS1);
  putd (h + 2668, F_SS2);
  put32(h + 2676, 0);                 /* ncon  */
  putd (h + 2680, 149597870.691);     /* au    */
  putd (h + 2688, 81.30056);          /* emrat */
  for (i = 0; i < 36; i++) put32(h + 2696 + i * 4, g_ipt[i]);
  put32(h + 2840, 431);               /* numde */
  for (i = 0; i < 3; i++) put32(h + 2844 + i * 4, g_ipt[36 + i]);

  if ((fp = fopen(path, "wb")) == NULL) return -1;
  fwrite(h, 1, sizeof h, fp);
  for (i = (int) sizeof h; i < g_irecsz; i++) fputc(0, fp);

  /* record 1: the constants block, read as 400 doubles */
  rec = (double *) calloc((size_t) g_ncoeffs, sizeof(double));
  if (rec == NULL) { fclose(fp); return -1; }
  fwrite(rec, sizeof(double), (size_t) g_ncoeffs, fp);

  /* records 2 .. nseg+1: the data segments */
  for (r = 0; r < F_NSEG; r++) {
    rec[0] = F_SS0 + r * F_SS2;
    rec[1] = F_SS0 + (r + 1) * F_SS2;
    for (j = 2; j < g_ncoeffs; j++) rec[j] = coeff(r, j);
    fwrite(rec, sizeof(double), (size_t) g_ncoeffs, fp);
  }
  free(rec);
  fclose(fp);
  return 0;
}

/* Bit-exact, like tests/golden: %a is exact for binary floating point, so
 * a comparison never has to pick a tolerance. */
static void emit(const char *tag, const double *x, int n, int retc)
{
  int i;
  printf("%-34s rc=%-3d", tag, retc);
  for (i = 0; i < n; i++) printf(" %a", x[i]);
  printf("\n");
}

int main(int argc, char **argv)
{
  const char *dir = ".";
  char path[512], tag[128];
  double ss[3], rrd[6];
  swe_ctx *ctx;
  long long want;
  char serr[AS_MAXCH];
  int i, retc, targ, cent, bad = 0;
  static const int TARGETS[] = { J_MERCURY, J_EARTH, J_MOON, J_SUN, J_SBARY, J_NUT };
  static const int CENTRES[] = { J_SBARY, J_EARTH, J_SUN };

  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) dir = argv[++i];

  build_layout();
  sprintf(path, "%s/jplcalc_fixture.eph", dir);
  if (write_eph(path) != 0) { printf("cannot write %s\nFAIL\n", path); return 1; }

  want = expected_len();
  {
    FILE *f = fopen(path, "rb");
    long long got;
    fseek(f, 0, SEEK_END); got = ftell(f); fclose(f);
    fprintf(stderr, "fixture: ksize=%d ncoeffs=%d irecsz=%d len=%lld want=%lld\n",
            (int) g_ksize, (int) g_ncoeffs, (int) g_irecsz, got, want);
    if (got != want) {
      printf("fixture length %lld != expected %lld -- the reader will "
             "reject it as mutilated\nFAIL\n", got, want);
      return 1;
    }
  }

  ctx = swi_default_ctx();
  *serr = '\0';
  retc = swi_open_jpl_file(ctx, ss, (char *) "jplcalc_fixture.eph",
                           (char *) dir, serr);
  if (retc != OK) {
    printf("swi_open_jpl_file rejected the fixture: %s\nFAIL\n", serr);
    return 1;
  }
  fprintf(stderr, "opened: ss = %.1f %.1f %.1f, denum = %d\n",
          ss[0], ss[1], ss[2], (int) swi_get_jpl_denum(ctx));

  /* Three dates: inside the first record, inside a later record, and one
   * that forces a record change on every other call -- so the js->nrl
   * "record already buffered" cache is exercised both ways, and so is the
   * Chebyshev cache reset that keys off pc[1]. */
  for (i = 0; i < 6; i++) {
    double et = F_SS0 + 1.5 + i * 37.25;
    for (targ = 0; targ < (int) (sizeof TARGETS / sizeof TARGETS[0]); targ++) {
      for (cent = 0; cent < (int) (sizeof CENTRES / sizeof CENTRES[0]); cent++) {
        if (TARGETS[targ] == J_NUT && cent > 0) continue;  /* centre ignored */
        memset(rrd, 0, sizeof rrd);
        *serr = '\0';
        retc = swi_pleph(ctx, et, TARGETS[targ], CENTRES[cent], rrd, serr);
        sprintf(tag, "pleph[%d,%d,%d]", i, TARGETS[targ], CENTRES[cent]);
        emit(tag, rrd, 6, retc);
        if (retc != OK && *serr != '\0')
          fprintf(stderr, "  %s: %s\n", tag, serr);
      }
    }
  }

  swi_close_jpl_file(ctx);
  remove(path);
  if (bad) { printf("FAIL\n"); return 1; }
  return 0;
}
