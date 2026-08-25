/* G22: the JPL reader's byte-swapping path, reorder().
 *
 * swejpl.c carries a full big-endian/little-endian conversion. fsizer()
 * decides on it from one number -- ss[2], the segment size in days -- and
 * turns it on when that lands outside 1..200, which is what a byte-swapped
 * double looks like. Eleven call sites then run it over the header fields,
 * the 400 constants, and every coefficient record.
 *
 * None of it was ever executed. ephe/de200.eph matches this host's byte
 * order, so do_reorder is 0 for the whole suite and reorder() measured 0.00%
 * -- the largest untested unit in the least-covered file (swejpl.c, 73.9%).
 *
 * It is not merely untested. reorder() byte-reverses through a fixed
 * char s[8] using a caller-supplied `size`, with no bound of its own:
 *
 *     static void reorder(char *x, int size, int number) {
 *       char s[8];
 *       ... for (j = 0; j < size; j++) *(sp2 + j) = ...
 *
 * Every current caller passes sizeof(double) or sizeof(int32), so it is
 * correct today and there is nothing to fix. But that is the same
 * trust-the-file shape as REVIEW.md's J1 (ksize/ncf against fixed arrays),
 * in the same file -- and unlike J1 this had no test at all.
 *
 * WHAT THIS ASSERTS, and why it is stronger than reaching the code:
 *
 * The fixture is not synthesised. It is ephe/de200.eph rewritten into the
 * opposite byte order, field for field, so it holds the SAME ephemeris.
 * Read correctly it must therefore produce bit-identical numbers -- not
 * numbers within a tolerance, the same doubles. Anything else means
 * reorder() got a byte wrong, and a single wrong byte in a mantissa is a
 * value that no tolerance would forgive anyway.
 *
 * That also makes the gate self-proving. If reorder() were broken or
 * removed, the swapped file would not merely compute badly, it would not
 * load: fsizer() checks the reordered ss[] against a plausibility range
 * (-5583942..9025909, segment 1..200) and rejects the file. So a PASS here
 * means the path ran AND was right.
 *
 * The fixture costs one 41 MB write per run -- the length is checked
 * against the header, so it cannot be truncated to save space.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

/* The first record's layout, as fsizer() reads it. Same map jplguard.c
 * documents; text first, then the binary fields this has to reverse.
 *    0     252 B    title            (text, left alone)
 *  252    2400 B    constant names   (text, left alone)
 * 2652    3 doubles ss[]
 * 2676    int32     ncon
 * 2680    double    au
 * 2688    double    emrat
 * 2696    int32     ipt[36]
 * 2840    int32     numde
 * 2844    int32     lpt[3]  -> ipt[36..38]
 */
#define OFF_SS    2652
#define OFF_NCON  2676
#define OFF_AU    2680
#define OFF_EMRAT 2688
#define OFF_IPT   2696
#define OFF_NUMDE 2840
#define OFF_LPT   2844
#define HDR_BIN_END 2856

static const int BODIES[] = { SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS,
                              SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE,
                              SE_PLUTO };
static const char *NAMES[] = { "Sun", "Moon", "Mercury", "Venus", "Mars",
                               "Jupiter", "Saturn", "Uranus", "Neptune",
                               "Pluto" };
#define NBODY ((int)(sizeof(BODIES)/sizeof(BODIES[0])))

static void swap_n(unsigned char *p, size_t width, size_t count)
{
  size_t i, j;
  for (i = 0; i < count; i++, p += width)
    for (j = 0; j < width / 2; j++) {
      unsigned char t = p[j];
      p[j] = p[width - 1 - j];
      p[width - 1 - j] = t;
    }
}

static int32 get32(const unsigned char *p)
{
  int32 v;
  memcpy(&v, p, sizeof v);
  return v;
}

/* fsizer()'s own computation, kept identical on purpose: the record size has
 * to be derived the way the reader derives it, or the two disagree about
 * where every record after the first begins. */
static int32 ksize_of(const unsigned char *hdr)
{
  int32 ipt[39];
  int32 kmx = 0, khi = 0, ksize, nd;
  int i;
  for (i = 0; i < 36; i++) ipt[i] = get32(hdr + OFF_IPT + i * 4);
  for (i = 0; i < 3; i++)  ipt[36 + i] = get32(hdr + OFF_LPT + i * 4);
  for (i = 0; i < 13; i++)
    if (ipt[i * 3] > kmx) { kmx = ipt[i * 3]; khi = i + 1; }
  nd = (khi == 12) ? 2 : 3;
  ksize = (ipt[khi * 3 - 3] + nd * ipt[khi * 3 - 2] * ipt[khi * 3 - 1] - 1) * 2;
  if (ksize == 1546) ksize = 1652;      /* the de102 hand-fix fsizer carries */
  return ksize;
}

/* Rewrites `src` into `dst` in the opposite byte order, reversing exactly
 * the fields the reader reverses and nothing else -- the title and the
 * constant names are text and must survive intact. Returns the record size,
 * or -1. */
static long build_swapped(const char *src, const char *dst)
{
  unsigned char *rec;
  FILE *in, *out;
  long irecsz;
  int32 ksize;
  size_t n;
  int recno;

  if ((in = fopen(src, "rb")) == NULL) return -1;
  rec = (unsigned char *) malloc(HDR_BIN_END);
  if (rec == NULL) { fclose(in); return -1; }
  if (fread(rec, 1, HDR_BIN_END, in) != HDR_BIN_END) {
    free(rec); fclose(in); return -1;
  }
  ksize = ksize_of(rec);
  free(rec);
  if (ksize <= 0) { fclose(in); return -1; }
  irecsz = 4L * ksize;
  if (irecsz % 8 != 0) { fclose(in); return -1; }   /* records are doubles */

  rewind(in);
  if ((out = fopen(dst, "wb")) == NULL) { fclose(in); return -1; }
  rec = (unsigned char *) malloc((size_t) irecsz);
  if (rec == NULL) { fclose(in); fclose(out); return -1; }

  for (recno = 0; (n = fread(rec, 1, (size_t) irecsz, in)) > 0; recno++) {
    if (recno == 0) {
      /* Header: the binary fields only. */
      swap_n(rec + OFF_SS,    8, 3);
      swap_n(rec + OFF_NCON,  4, 1);
      swap_n(rec + OFF_AU,    8, 1);
      swap_n(rec + OFF_EMRAT, 8, 1);
      swap_n(rec + OFF_IPT,   4, 36);
      swap_n(rec + OFF_NUMDE, 4, 1);
      swap_n(rec + OFF_LPT,   4, 3);
    } else if (recno == 1) {
      swap_n(rec, 8, 400);                  /* cval[400], the constants */
    } else {
      swap_n(rec, 8, n / 8);                /* a record of coefficients */
    }
    if (fwrite(rec, 1, n, out) != n) {
      free(rec); fclose(in); fclose(out); return -1;
    }
  }
  free(rec);
  fclose(in);
  if (fclose(out) != 0) return -1;
  return irecsz;
}

/* Epochs spread across DE200's 1600..2170, so the comparison lands in
 * different records rather than one. It matters more than it looks: with a
 * single epoch, corrupting one coefficient in the middle of a record changed
 * no result at all -- ten bodies at one instant do not touch everything a
 * record holds, so the numeric half of this gate only covers what gets used.
 * Several epochs and SEFLG_SPEED (which brings in the derivative chain)
 * widen that considerably. Measured: with these, the same one-coefficient
 * corruption is caught. */
static const double EPOCHS[] = {
  2305456.5,     /* 1600-01-01, near the start of DE200's range */
  2378496.5,     /* 1800-01-01 */
  2415020.5,     /* 1900-01-01 */
  2444204.095138889,
  2451544.5,     /* 2000-01-01 */
  2488069.5,     /* 2100-01-01 */
};
#define NEPOCH ((int)(sizeof(EPOCHS)/sizeof(EPOCHS[0])))
#define NSAMP  (NBODY * NEPOCH)

/* Asks for a date far outside any ephemeris and returns what the reader
 * says. state() answers that with
 *     "jd %f outside JPL eph. range %.2f .. %.2f;"
 * built from eh_ss[0] and eh_ss[1] -- the reordered header. So the reply is
 * a direct read-out of whether the HEADER byte-swapped correctly, and it
 * needs no coefficient record to produce.
 *
 * That separation is the point. Without it the gate can only say "the
 * swapped file did not answer" and has to guess whether the header or the
 * coefficients were at fault; guessing got it wrong, and a probe whose
 * header was fine was reported as a header failure. */
static void range_probe(const char *dir, const char *file, char *serr)
{
  char jf[AS_MAXCH];
  double xx[6];
  swe_close();
  swe_set_ephe_path((char *) dir);
  serr[0] = '\0';
  if (strlen(file) >= AS_MAXCH) { strcpy(serr, "<name too long>"); return; }
  strcpy(jf, file);
  swe_set_jpl_file(jf);
  swe_calc(1.0, SE_MARS, SEFLG_JPLEPH, xx, serr);
}

/* Reads every body at every epoch through the JPL path from `dir`/`file`.
 * Returns 0 on success. Each call starts from swe_close() so the second run
 * cannot be answered out of the first one's open file. */
static int read_all(const char *dir, const char *file, double out[][6],
                    int32 rflags[], char *serr, int *failed_at)
{
  char jf[AS_MAXCH];
  int i, e;
  *failed_at = -1;
  swe_close();
  swe_set_ephe_path((char *) dir);
  if (strlen(file) >= AS_MAXCH) return 1;
  strcpy(jf, file);
  swe_set_jpl_file(jf);
  for (e = 0; e < NEPOCH; e++)
    for (i = 0; i < NBODY; i++) {
      int k = e * NBODY + i;
      serr[0] = '\0';
      rflags[k] = swe_calc(EPOCHS[e], BODIES[i], SEFLG_JPLEPH | SEFLG_SPEED,
                           out[k], serr);
      if (rflags[k] == ERR) { *failed_at = k; return 1; }
    }
  return 0;
}

int main(int argc, char **argv)
{
  const char *ephe = getenv("SE_TEST_EPHE");
  const char *jplfile = getenv("SE_JPL_FILE");
  const char *outdir = (argc > 1) ? argv[1] : ".";
  char src[AS_MAXCH * 2], dst[AS_MAXCH * 2];
  static double xa[NSAMP][6], xb[NSAMP][6];
  static int32 fa[NSAMP], fb[NSAMP];
  char serr[AS_MAXCH] = "", sa[AS_MAXCH] = "", sb[AS_MAXCH] = "";
  long irecsz;
  int i, k, at = -1, bad = 0, differing = 0;

  if (ephe == NULL || *ephe == '\0') ephe = "../ephe";
  if (jplfile == NULL || *jplfile == '\0') jplfile = "de200.eph";

  printf("G22: the JPL byte-swapping path (reorder), against a swapped %s\n",
         jplfile);

  if (snprintf(src, sizeof src, "%s/%s", ephe, jplfile) >= (int) sizeof src ||
      snprintf(dst, sizeof dst, "%s/swapped_%s", outdir, jplfile) >= (int) sizeof dst) {
    printf("  FAIL: path too long\n");
    return 1;
  }

  irecsz = build_swapped(src, dst);
  if (irecsz < 0) {
    printf("  FAIL: could not build the swapped fixture from %s\n", src);
    return 1;
  }
  printf("  fixture %s  (record size %ld bytes)\n", dst, irecsz);

  /* 1. The header. Both files must describe the same epoch range, which the
   *    reader can only report if ss[] came back correctly. */
  range_probe(ephe, jplfile, sa);
  range_probe(outdir, dst + strlen(outdir) + 1, sb);
  if (strcmp(sa, sb) != 0) {
    printf("  FAIL: the header did not byte-swap -- the two files disagree "
           "about their own range\n");
    printf("        native : %.110s\n", sa);
    printf("        swapped: %.110s\n", sb);
    remove(dst);
    return 1;
  }
  printf("  header reorders: both report the same range   OK\n");

  /* 2. The coefficients. */
  if (read_all(ephe, jplfile, xa, fa, serr, &at) != 0) {
    printf("  FAIL: the native file did not answer: %.100s\n", serr);
    remove(dst);
    return 1;
  }
  if (read_all(outdir, dst + strlen(outdir) + 1, xb, fb, serr, &at) != 0) {
    printf("  FAIL: the swapped file opened -- so its header is right -- then "
           "refused at\n        sample %d (%s, jd %.4f): %.100s\n",
           at, NAMES[at % NBODY], EPOCHS[at / NBODY], serr);
    remove(dst);
    return 1;
  }

  for (i = 0; i < NSAMP; i++) {
    if (!(fa[i] & SEFLG_JPLEPH) || !(fb[i] & SEFLG_JPLEPH)) {
      printf("  FAIL %-9s at jd %.4f not answered by JPL (rf %d / %d)\n",
             NAMES[i % NBODY], EPOCHS[i / NBODY], (int) fa[i], (int) fb[i]);
      bad++;
      continue;
    }
    /* Bit-exact: the same ephemeris, so the same doubles. Compared as
     * bytes rather than with ==, so a NaN would have to match too. */
    for (k = 0; k < 6; k++)
      if (memcmp(&xa[i][k], &xb[i][k], sizeof(double)) != 0) {
        if (differing++ < 5)
          printf("  DIFFERS  %-9s jd %.4f  x[%d]  %.17g vs %.17g\n",
                 NAMES[i % NBODY], EPOCHS[i / NBODY], k, xa[i][k], xb[i][k]);
        bad++;
        break;
      }
  }
  printf("  %d samples (%d bodies x %d epochs, with speed), %d differing\n",
         NSAMP, NBODY, NEPOCH, differing);

  swe_close();
  remove(dst);
  printf(bad ? "G22 FAIL\n" : "G22 PASS (bit-identical in both byte orders)\n");
  return bad ? 1 : 0;
}
