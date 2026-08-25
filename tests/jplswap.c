/* G22: reorder(), swejpl.c's byte-swapping path.
 *
 * fsizer() turns byte-swapping on when ss[2], the segment size in days,
 * lands outside 1..200 -- what a swapped double looks like -- and eleven
 * call sites then run it over the header, the 400 constants and every
 * coefficient record. None of it ran: ephe/de200.eph matches this host's
 * byte order, so reorder() measured 0.00%.
 *
 * The fixture is de200.eph rewritten field-for-field into the opposite byte
 * order, so it is the SAME ephemeris and must read back bit-identical, not
 * within a tolerance. Two assertions, because they fail for different
 * reasons: the header (both files must report the same epoch range, which
 * needs ss[] reordered) and the coefficients (60 samples).
 *
 * A PASS means the path ran AND was right: fsizer() rejects a file whose
 * reordered ss[] is implausible, so a broken reorder() cannot load at all.
 *
 * Costs one 41 MB write per run; the header carries the expected length, so
 * it cannot be truncated.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

/* The first record's layout, as fsizer() reads it -- the map jplguard.c
 * documents. Text first, then the binary fields to reverse.
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

/* fsizer()'s computation, kept identical: derive the record size any other
 * way and the two disagree about where every later record begins. */
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

/* Rewrites src into dst in the opposite byte order, reversing exactly what
 * the reader reverses -- title and constant names are text. Returns the
 * record size, or -1. */
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
      /* Refuse a short tail rather than copying it through unswapped: that
       * would produce a fixture wrong in a way the gate would blame on
       * reorder(). */
      if (n % 8 != 0) { free(rec); fclose(in); fclose(out); return -1; }
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

/* Spread across DE200's 1600..2170 so the comparison lands in different
 * records. With a single epoch, corrupting one coefficient mid-record changed
 * no result at all: ten bodies at one instant do not touch everything a record
 * holds. With these and SEFLG_SPEED, that same corruption is caught. */
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

/* A date outside any ephemeris makes state() answer "jd %f outside JPL eph.
 * range %.2f .. %.2f;" from eh_ss[] -- the reordered header, read back
 * without touching a coefficient record. That separates the two failures;
 * inferring them from the first refused sample got it wrong. */
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
  char src[AS_MAXCH * 2], dst[AS_MAXCH * 2], base[AS_MAXCH];
  static double xa[NSAMP][6], xb[NSAMP][6];
  static int32 fa[NSAMP], fb[NSAMP];
  char serr[AS_MAXCH] = "", sa[AS_MAXCH] = "", sb[AS_MAXCH] = "";
  long irecsz;
  int i, k, at = -1, bad = 0, differing = 0;

  if (ephe == NULL || *ephe == '\0') ephe = "../ephe";
  if (jplfile == NULL || *jplfile == '\0') jplfile = "de200.eph";

  printf("G22: the JPL byte-swapping path (reorder), against a swapped %s\n",
         jplfile);

  /* Keep the bare filename rather than slicing it back out of dst three
   * times; the reader is given a directory and a name. */
  if (snprintf(base, sizeof base, "swapped_%s", jplfile) >= (int) sizeof base ||
      snprintf(src, sizeof src, "%s/%s", ephe, jplfile) >= (int) sizeof src ||
      snprintf(dst, sizeof dst, "%s/%s", outdir, base) >= (int) sizeof dst) {
    printf("  FAIL: path too long\n");
    return 1;
  }

  irecsz = build_swapped(src, dst);
  if (irecsz < 0) {
    printf("  FAIL: could not build the swapped fixture from %s\n", src);
    remove(dst);        /* a partial 41 MB file, if it got that far */
    return 1;
  }
  printf("  fixture %s  (record size %ld bytes)\n", dst, irecsz);

  /* 1. The header. Both files must describe the same epoch range, which the
   *    reader can only report if ss[] came back correctly. */
  range_probe(ephe, jplfile, sa);
  range_probe(outdir, base, sb);
  /* Two files that both fail to open report the same thing, and equal is
   * equal -- so require the native probe to have actually reported a range. */
  if (strstr(sa, "range") == NULL) {
    printf("  FAIL: the native file did not report its range, so there is "
           "nothing to compare\n        %.110s\n", sa);
    remove(dst);
    return 1;
  }
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
  if (read_all(outdir, base, xb, fb, serr, &at) != 0) {
    /* at < 0 means it never got as far as a sample; NAMES[at % NBODY] would
     * be NAMES[-1], since -1 % 10 is -1 in C. */
    if (at < 0)
      printf("  FAIL: the swapped file could not be set up: %.100s\n", serr);
    else
      printf("  FAIL: the swapped file opened -- so its header is right -- "
             "then refused at\n        sample %d (%s, jd %.4f): %.100s\n",
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
