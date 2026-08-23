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
 * (ipt[30]) carrying `maxoff` so ksize is computed from it. */
static int write_eph(const char *path, int ncf, int na, int maxoff)
{
  unsigned char h[2856];
  int i, ksize, irecsz;
  FILE *fp;
  memset(h, ' ', 252);
  memcpy(h, "SWISSEPH JPL GUARD FIXTURE", 26);
  memset(h + 252, ' ', 2400);
  putd(h + 2652, 2451545.0);
  putd(h + 2660, 2460000.0);
  putd(h + 2668, 32.0);            /* ss[2]: valid, so no byte reordering */
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

static int expect_reject(const char *what, const char *file, const char *want)
{
  char serr[AS_MAXCH] = ""; double ss[3];
  int r = swi_open_jpl_file(ss, (char *) file, "/tmp", serr);
  printf("  %-34s rc=%-3d %s\n", what, r, serr);
  if (r == OK) {
    printf("    FAIL: malformed file was ACCEPTED -- the guard is gone\n");
    swi_close_jpl_file();
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
    int r = swi_open_jpl_file(ss, (char *) "jplguard_ok.eph", "/tmp", serr);
    printf("  %-34s rc=%-3d %s\n", "control: passes both guards", r, serr);
    if (r != OK && (strstr(serr, "coefficient count") || strstr(serr, "ksize"))) {
      printf("    FAIL: control rejected by a guard it should satisfy\n");
      bad = 1;
    }
    if (r == OK) swi_close_jpl_file();
  }

  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad;
}
