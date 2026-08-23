/* jplguard.c -- exercise swejpl.c's header-validation guards with real
 * malformed ephemeris files.
 *
 * Background: REVIEW.md finding J1. swejpl.c sized two fixed arrays from
 * values it read out of the .eph header without checking them:
 *
 *   state()   fills js->buf[] with `ncoeffs` doubles, ncoeffs = ksize/2,
 *             and ksize was accepted anywhere in [1000, 5000] -- so up to
 *             2500 doubles into what was then a double buf[1500]. Measured
 *             before the fix: js is CALLOC(1, 19096), buf sits at offset
 *             6512, and buf[2499] ends at 26512 -- 7416 bytes past the
 *             allocation, corrupting pc/vc/ac/jc (offset 18512) and do_km
 *             (19088) on its way out of the block.
 *
 *   interp()  indexes js->pc/vc/ac/jc[18] up to ncf-1, where ncf comes
 *             straight from the file's ipt[] header with no bound at all.
 *
 * Both are now guarded (commits 04bb51a, 4822202): buf[] is JPL_NCOEFF_MAX
 * and ksize is bounded to JPL_NCOEFF_MAX*2 so the two cannot drift apart,
 * and a runtime loop rejects any body claiming more coefficients than pc[]
 * holds.
 *
 * The ksize half is now backed by a static_assert, so it cannot regress
 * silently. The ncf half CANNOT be -- it is a runtime value read from a
 * file -- and it had only ever been verified by reading the code. This test
 * exercises it against actual malformed files.
 *
 * Build with -fsanitize=address for the strongest signal: if a guard is
 * ever removed, this turns from a clean rejection into a diagnosed heap
 * overflow rather than silent corruption.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"
#include "swejpl.h"

/* Header layout fsizer()/state() expect, little-endian:
 *    0     252 B    title
 *  252    2400 B    constant names
 * 2652    3 doubles ss[]      (ss[2] must be in 1..200 or reorder kicks in)
 * 2676    int32     ncon
 * 2680    double    au
 * 2688    double    emrat
 * 2696    int32     ipt[36]   (per body: offset, ncf, na)
 * 2840    int32     numde
 * 2844    int32     lpt[3]    -> ipt[36..38]
 */
static void put32(unsigned char *p, int v)    { memcpy(p, &v, 4); }
static void putd (unsigned char *p, double v) { memcpy(p, &v, 8); }

/* Writes a header whose ipt[] is filled from `ncf` and `na`, with body 10
 * (ipt[30]) carrying `maxoff` so ksize is computed from it. ss0/ss1/ss2 set
 * the segment range, which is what drives nseg in the length computation. */
static int write_eph_ss(const char *path, int ncf, int na, int maxoff,
                        double ss0, double ss1, double ss2)
{
  unsigned char h[2856];
  int i, ksize, irecsz;
  FILE *fp;
  memset(h, ' ', 252);
  memcpy(h, "SWISSEPH JPL GUARD FIXTURE", 26);
  memset(h + 252, ' ', 2400);
  putd(h + 2652, ss0);
  putd(h + 2660, ss1);
  putd(h + 2668, ss2);             /* must be 1..200 or reordering kicks in */
  put32(h + 2676, 0);              /* ncon */
  putd (h + 2680, 149597870.7);    /* au    */
  putd (h + 2688, 81.3);           /* emrat */
  for (i = 0; i < 12; i++) {
    put32(h + 2696 + (i * 3 + 0) * 4, 1);
    put32(h + 2696 + (i * 3 + 1) * 4, ncf);
    put32(h + 2696 + (i * 3 + 2) * 4, na);
  }
  put32(h + 2696 + 30 * 4, maxoff);   /* ipt[30]: the argmax -> khi = 11 */
  put32(h + 2696 + 31 * 4, ncf);
  put32(h + 2696 + 32 * 4, na);
  put32(h + 2840, 431);               /* numde */
  put32(h + 2844, 1);                 /* lpt[0] -> ipt[36] */
  put32(h + 2848, ncf);
  put32(h + 2852, na);

  /* khi = 11 (ipt[30] is the max), nd = 3 */
  ksize  = (maxoff + 3 * ncf * na - 1) * 2;
  irecsz = 4 * ksize;
  if ((fp = fopen(path, "wb")) == NULL) return -1;
  fwrite(h, 1, sizeof h, fp);
  for (i = (int) sizeof h; i < irecsz; i++) fputc(0, fp);
  /* four plausible records so the read loop has data to consume */
  for (i = 0; i < 4 * ksize / 2; i++) { double d = 1.0; fwrite(&d, 8, 1, fp); }
  fclose(fp);
  return ksize;
}

/* the ordinary case: a short segment range, nseg small */
static int write_eph(const char *path, int ncf, int na, int maxoff)
{
  return write_eph_ss(path, ncf, na, maxoff, 2451545.0, 2460000.0, 32.0);
}

static int expect_reject(const char *what, const char *file, const char *want)
{
  char serr[AS_MAXCH] = ""; double ss[3];
  int r = swi_open_jpl_file(swi_default_ctx(), ss, (char *) file, "/tmp", serr);
  printf("  %-34s rc=%-3d %s\n", what, r, serr);
  if (r == OK) {
    printf("    FAIL: malformed file was ACCEPTED -- the guard is gone\n");
    swi_close_jpl_file(swi_default_ctx());
    return 1;
  }
  if (want && strstr(serr, want) == NULL) {
    printf("    FAIL: rejected, but not by the expected guard (wanted \"%s\")\n", want);
    return 1;
  }
  return 0;
}

int main(void)
{
  int bad = 0, ksize;

  /* 1. ncf larger than pc[]/vc[]/ac[]/jc[] can hold.
   *    Pre-fix this reached interp() and wrote past those arrays. */
  ksize = write_eph("/tmp/jplguard_ncf.eph", 100, 1, 1200);
  printf("  fixture ncf=100 (pc[] holds 18), ksize=%d\n", ksize);
  bad |= expect_reject("ncf > sizeof(pc)/sizeof(pc[0])",
                       "jplguard_ncf.eph", "coefficient count");

  /* 2. ksize beyond JPL_NCOEFF_MAX*2, i.e. ncoeffs beyond buf[].
   *    Pre-fix this overflowed buf[1500] by up to 1000 doubles. */
  ksize = write_eph("/tmp/jplguard_ksize.eph", 4, 300, 1200);
  printf("  fixture ksize=%d (limit is JPL_NCOEFF_MAX*2 = 5000)\n", ksize);
  bad |= expect_reject("ksize > JPL_NCOEFF_MAX*2",
                       "jplguard_ksize.eph", "ksize");

  /* 3. A header that satisfies both guards must still be REJECTED here, but
   *    by a later check rather than a memory error -- this is the control
   *    case proving the two above fail for their stated reason and not
   *    because any malformed file is refused outright. */
  ksize = write_eph("/tmp/jplguard_ok.eph", 4, 1, 1200);
  printf("  fixture ncf=4 na=1 ksize=%d (both guards satisfied)\n", ksize);
  {
    char serr[AS_MAXCH] = ""; double ss[3];
    int r = swi_open_jpl_file(swi_default_ctx(), ss, (char *) "jplguard_ok.eph", "/tmp", serr);
    printf("  %-34s rc=%-3d %s\n", "control: passes both guards", r, serr);
    if (r != OK && (strstr(serr, "coefficient count") || strstr(serr, "ksize"))) {
      printf("    FAIL: control rejected by a guard it should satisfy\n");
      bad = 1;
    }
    if (r == OK) swi_close_jpl_file(swi_default_ctx());
  }

  /* 4. The expected-length computation must not wrap.
   *
   *    nb accumulates ncf * na * k * nseg over 13 bodies. ipt[] and nseg are
   *    int32 and k is int, so before the (off_t64) cast each term was
   *    evaluated in 32-bit and only then widened into the 64-bit nb.
   *
   *    ncf is capped at 18 and nseg at 14609851 (the ss[] plausibility
   *    bounds), but na is unbounded, so na >= 3 suffices:
   *      18 * 3 * 3 * 14609851 = 2366795862 > INT_MAX
   *    With the wrap, 12 of 13 terms overflow and nb becomes NEGATIVE.
   *
   *    The file is rejected either way -- a negative expected length fails
   *    the comparison, so this is fail-safe -- which is exactly why it needs
   *    an explicit test. The observable signature is the expected length in
   *    the error message going negative. That is what the very first J1
   *    probe accidentally printed:
   *        "length = 48440 instead of -1760248016"
   *    and it is what this asserts against.
   */
  {
    char serr[AS_MAXCH] = ""; double ss[3];
    long long expected = 0;
    int r;
    /* ss[2] = 1 day across the full accepted range -> nseg at its maximum */
    ksize = write_eph_ss("/tmp/jplguard_nb.eph", 18, 3, 1200,
                         -5583940.0, 9025900.0, 1.0);
    printf("  fixture ncf=18 na=3 nseg~14.6e6 ksize=%d\n", ksize);
    r = swi_open_jpl_file(swi_default_ctx(), ss, (char *) "jplguard_nb.eph", "/tmp", serr);
    printf("  %-34s rc=%-3d %s\n", "expected length must not wrap", r, serr);
    if (r == OK) {
      printf("    FAIL: accepted -- the length check did not run\n");
      bad = 1; swi_close_jpl_file(swi_default_ctx());
    } else if (sscanf(serr, "%*[^0-9-]%*d%*[^0-9-]%lld", &expected) == 1
               && expected < 0) {
      printf("    FAIL: expected length is NEGATIVE (%lld) -- nb wrapped\n",
             expected);
      bad = 1;
    } else if (strstr(serr, "instead of -") != NULL) {
      printf("    FAIL: expected length is negative -- nb wrapped\n");
      bad = 1;
    }
  }

  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad;
}
