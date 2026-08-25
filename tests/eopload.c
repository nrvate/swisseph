/* eopload.c -- G19: the IERS Earth-orientation loader.
 *
 * SEFLG_JPLHOR asks for positions that match JPL Horizons exactly rather
 * than to model precision, by using the same MEASURED Earth orientation
 * Horizons uses -- the dpsi/deps corrections the IERS has published since
 * 1962. load_dpsi_deps() is what reads them, from eop_1962_today.txt in the
 * EOPC04 format and optionally eop_finals.txt in the fixed-width "finals"
 * format.
 *
 * Nothing had ever run it. Two things have to line up first, and the
 * shipped tree provides neither:
 *
 *   - it is called only from swe_set_jpl_file(), and only when the file
 *     opens AND reports a DE number of 403 or higher. The repository ships
 *     de200.eph, which is 200.
 *   - the EOP files are not shipped either. They would need updating, and
 *     they are large.
 *
 * So this builds both. The JPL file is synthesised rather than copied: a
 * header whose ipt[] and ss[] are self-consistent, padded to exactly the
 * length fsizer() computes from them, carrying numde = 431. Roughly 20 KB,
 * against 43 MB for a relabelled copy of de200.eph -- and truncating that
 * copy does not work, because the length check catches it. The coefficients
 * inside are not real and are never evaluated: nothing here computes a JPL
 * position. What is under test is the parser and the flag plumbing.
 *
 * The check is white-box, on ctx->eop_dpsi_loaded and the arrays the loader
 * fills, and that is deliberate. The black-box observable would be
 * swe_calc()'s returned flag -- plaus_iflag() strips SEFLG_JPLHOR and
 * substitutes SEFLG_JPLHOR_APPROX when eop_dpsi_loaded <= 0 -- but that
 * path is out of reach here twice over: it keeps SEFLG_JPLHOR only when
 * SEFLG_JPLEPH is also set (the comment above that line says "JPL and Swiss
 * Ephemeris", the code says JPL alone), and computing with SEFLG_JPLEPH
 * needs real coefficients, which a synthetic file cannot supply. It returns
 * ERR, and ERR is -1, whose bits read as every flag set.
 *
 * So this tests the loader rather than the flag: does it find the file,
 * parse the columns it claims to, record the span, and set the status
 * plaus_iflag() later branches on. Two contexts, one with the EOP file and
 * one without.
 *
 *   eopload <dir-with-eop> <dir-without-eop>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "swephexp.h"
#include "sweph.h"   /* ctx->eop_* : this test is white-box on purpose */

#define JPL_FIXTURE "de431fixture.eph"

static void put32(unsigned char *p, int v)    { memcpy(p, &v, 4); }
static void putd (unsigned char *p, double v) { memcpy(p, &v, 8); }

/* A JPL header fsizer() will accept, and the padding that makes the file
 * the length that header implies:
 *
 *   nb = sum(i=0..12) ipt[i*3+1] * ipt[i*3+2] * k * nseg    k = 3, k = 2 at i == 11
 *   nb += 2 * nseg                                          segment epochs
 *   nb *= 8                                                 doubles to bytes
 *   nb += 2 * ksize * nrecl                                 header + constants
 *
 * ss[] is chosen to give nseg = 1, which is what keeps the file small.
 * ss[2] must stay in 1..200 or the reader decides the file needs byte
 * reordering. */
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
    put32(h + 2696 + (i * 3 + 0) * 4, 1);
    put32(h + 2696 + (i * 3 + 1) * 4, ncf);
    put32(h + 2696 + (i * 3 + 2) * 4, na);
  }
  put32(h + 2696 + 30 * 4, maxoff);   /* body 10 carries the max offset */
  put32(h + 2696 + 31 * 4, ncf);
  put32(h + 2696 + 32 * 4, na);
  put32(h + 2840, 431);               /* numde -- the whole point */
  put32(h + 2844, 1);                 /* lpt[] -> ipt[36..38], i.e. i = 12 */
  put32(h + 2848, ncf);
  put32(h + 2852, na);

  ksize = (maxoff + 3 * ncf * na - 1) * 2;
  for (i = 0; i < 13; i++)
    nb += (long) ncf * na * (i == 11 ? 2 : 3) * nseg;
  nb += 2 * nseg;
  nb *= 8;
  nb += 2 * (long) ksize * nrecl;

  if ((fp = fopen(path, "wb")) == NULL) return -1;
  fwrite(h, 1, sizeof h, fp);
  for (i = (int) sizeof h; i < nb; i++) fputc(0, fp);
  /* The header occupies the first two records; the data records follow, and
   * each opens with its segment's start and end epoch. state() reads that
   * first pair back and refuses the file if it disagrees with ss[0]/ss[1],
   * so zero padding is not enough -- the one segment has to say when it is.
   * The 152 coefficients after them stay zero: nothing evaluates them. */
  if ((fp = freopen(path, "r+b", fp)) == NULL) return -1;
  fseek(fp, 2L * ksize * nrecl, SEEK_SET);
  { double ep[2]; ep[0] = ss0; ep[1] = ss1; fwrite(ep, 8, 2, fp); }
  fclose(fp);
  return 0;
}

/* eop_1962_today.txt, EOPC04 format: whitespace-separated, year in column 0,
 * MJD in 3, dpsi in 8, deps in 9. The MJD must advance by exactly one per
 * line or the loader gives up and records eop_dpsi_loaded = -2. A line
 * whose first token is not a number is skipped, which is how the real
 * file's header survives. */
static int write_eopc04(const char *path)
{
  FILE *fp = fopen(path, "w");
  int i;
  if (fp == NULL) return -1;
  fprintf(fp, "# EOPC04 fixture -- header line, skipped because column 0 is not a year\n");
  for (i = 0; i < 40; i++)
    fprintf(fp, "2000  1 %2d  %5d   0.0 0.0 0.0 0.0 %.9f %.9f\n",
            i + 1, 51544 + i, -0.05 - i * 1e-5, -0.005 - i * 1e-6);
  fclose(fp);
  return 0;
}

int main(int argc, char **argv)
{
  const char *dir_eop  = (argc > 1) ? argv[1] : ".";
  const char *dir_bare = (argc > 2) ? argv[2] : ".";
  char p[512], jf[AS_MAXCH];
  swe_ctx *a, *b;
  int bad = 0;

  snprintf(p, sizeof p, "%s/%s", dir_eop, JPL_FIXTURE);
  if (write_jpl(p) != 0) { printf("FAIL: cannot write %s\n", p); return 1; }
  snprintf(p, sizeof p, "%s/%s", dir_bare, JPL_FIXTURE);
  if (write_jpl(p) != 0) { printf("FAIL: cannot write %s\n", p); return 1; }
  snprintf(p, sizeof p, "%s/eop_1962_today.txt", dir_eop);
  if (write_eopc04(p) != 0) { printf("FAIL: cannot write %s\n", p); return 1; }

  a = swe_ctx_new();
  b = swe_ctx_new();
  if (a == NULL || b == NULL) { printf("FAIL: swe_ctx_new() returned NULL\n"); return 1; }

  swe_set_ephe_path_r(a, (char *) dir_eop);
  strcpy(jf, JPL_FIXTURE); swe_set_jpl_file_r(a, jf);
  swe_set_ephe_path_r(b, (char *) dir_bare);
  strcpy(jf, JPL_FIXTURE); swe_set_jpl_file_r(b, jf);

  /* 1 = eop_1962_today.txt parsed. -1 = not found. Anything else is the
   * loader reporting a format it could not use. */
  printf("with eop files    : eop_dpsi_loaded=%d span=%.1f..%.1f (%d days)\n",
         a->eop_dpsi_loaded, a->eop_tjd_beg, a->eop_tjd_end,
         (int) (a->eop_tjd_end - a->eop_tjd_beg) + 1);
  printf("without eop files : eop_dpsi_loaded=%d\n", b->eop_dpsi_loaded);

  if (a->eop_dpsi_loaded != 1) {
    printf("FAIL: the file is there and was not loaded\n");
    bad = 1;
  }
  if (b->eop_dpsi_loaded != -1) {
    printf("FAIL: expected -1 (file not found) without it, got %d\n",
           b->eop_dpsi_loaded);
    bad = 1;
  }
  /* The fixture runs MJD 51544..51583, so tjd 2451544.5..2451583.5. */
  if (a->eop_tjd_beg != 2451544.5 || a->eop_tjd_end != 2451583.5) {
    printf("FAIL: span is not the fixture's 2451544.5..2451583.5\n");
    bad = 1;
  }
  /* Columns 8 and 9 of the fixture, first and last row. Reading them back
   * is what says the loader picked the right two fields out of ten rather
   * than merely counting lines. */
  if (a->dpsi == NULL || a->deps == NULL) {
    printf("FAIL: dpsi/deps not allocated\n");
    bad = 1;
  } else {
    printf("  dpsi[0]=%.9f deps[0]=%.9f  dpsi[39]=%.9f deps[39]=%.9f\n",
           a->dpsi[0], a->deps[0], a->dpsi[39], a->deps[39]);
    /* Compared with a tolerance, not exactly: the fixture writes these
     * through %.9f and the loader reads them back with atof(), so what
     * comes out is the decimal, not the double that produced it. The point
     * is that the right column was read, and 1e-12 settles that -- the
     * columns either side differ by 1e-3 or more. */
    if (fabs(a->dpsi[0] - (-0.05)) > 1e-12 || fabs(a->deps[0] - (-0.005)) > 1e-12) {
      printf("FAIL: first row is not what the fixture wrote\n");
      bad = 1;
    }
    if (fabs(a->dpsi[39] - (-0.05 - 39e-5)) > 1e-12
        || fabs(a->deps[39] - (-0.005 - 39e-6)) > 1e-12) {
      printf("FAIL: last row is not what the fixture wrote\n");
      bad = 1;
    }
  }

  swe_ctx_free(a);
  swe_ctx_free(b);
  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad;
}
