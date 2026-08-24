/* dtmemo.c -- G18: calc_deltat()'s memo must not outlive the delta-t table.
 *
 * ctx->dt[], the tabulated delta-t, is an input to that memo which its key
 * deliberately does not carry: comparing 220 entries on every one of the
 * 135,204 calls a heliacal search makes, to catch a change that happens at
 * most once in a context's life, is the wrong trade. So the two functions
 * that write the table -- swi_seed_dt_table() at context creation and
 * init_dt() on first use -- empty the memo instead. This checks that they
 * really do, because nothing in the numerical transcript can: it only
 * reaches the table through the built-in copy, which never changes.
 *
 * The window is narrow but real. init_dt() runs lazily, the first time a
 * date from 1620 on is asked for. A date BETWEEN 1600 and 1620 under
 * SEMOD_DELTAT_STEPHENSON_MORRISON_2004 is interpolated against dt[0] --
 * the 1620 entry -- and never goes near init_dt(), so it can be memoised
 * while the table still holds the built-in 124.00. If a swe_deltat.txt then
 * replaces that entry, the memo is holding an answer the library would no
 * longer give.
 *
 * Two contexts, the same three questions, different order:
 *
 *   A: 1610, then 2020 -- which loads the file -- then 1610 again
 *   B: 2020, which loads the file, then 1610
 *
 * A's second 1610 and B's only 1610 are the same question put to the same
 * loaded table, so they have to agree. Without the invalidation A answers
 * out of the memo it filled before the load, and they do not. No golden
 * file: the test is its own reference.
 *
 * Needs an ephemeris directory containing a swe_deltat.txt whose 1620 entry
 * differs from the built-in 124.00; the Makefile builds one. Run as
 *   dtmemo <ephe-dir-with-swe_deltat.txt>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "swephexp.h"

/* 1610-01-01 and 2020-01-01. The first lands in the 1600..1620 interpolation
 * that reads dt[0]; the second is what drags init_dt() in. */
#define TJD_1610  2308000.5
#define TJD_2020  2458849.5

/* SE_MODEL_DELTAT is index 0, so the model string is just the number. */
static void use_sm2004(swe_ctx *c)
{
  char sam[AS_MAXCH];
  snprintf(sam, sizeof sam, "%d", SEMOD_DELTAT_STEPHENSON_MORRISON_2004);
  swe_set_astro_models_r(c, sam, 0);
}

int main(int argc, char **argv)
{
  const char *ephe = (argc > 1) ? argv[1] : ".";
  swe_ctx *a, *b;
  double a1, a2020, a2, b2020, b1;
  int bad = 0;

  a = swe_ctx_new();
  b = swe_ctx_new();
  if (a == NULL || b == NULL) {
    printf("FAIL: swe_ctx_new() returned NULL\n");
    return 1;
  }
  swe_set_ephe_path_r(a, (char *) ephe);
  swe_set_ephe_path_r(b, (char *) ephe);
  use_sm2004(a);
  use_sm2004(b);

  /* MOSEPH so that neither context needs an .se1 file to resolve its tidal
   * acceleration; the delta-t table is the only file in play here. */
  a1    = swe_deltat_ex_r(a, TJD_1610, SEFLG_MOSEPH, NULL);  /* memoised pre-load */
  a2020 = swe_deltat_ex_r(a, TJD_2020, SEFLG_MOSEPH, NULL);  /* loads the file */
  a2    = swe_deltat_ex_r(a, TJD_1610, SEFLG_MOSEPH, NULL);  /* must recompute */

  b2020 = swe_deltat_ex_r(b, TJD_2020, SEFLG_MOSEPH, NULL);  /* loads the file */
  b1    = swe_deltat_ex_r(b, TJD_1610, SEFLG_MOSEPH, NULL);  /* nothing stale to hit */

  printf("1620 entry loaded : A %.9f   B %.9f %s\n",
         a2020, b2020, a2020 == b2020 ? "OK" : "MISMATCH");
  if (a2020 != b2020) bad = 1;

  /* The test is vacuous unless the file actually moved dt[0]: if a1 already
   * equals a2 the invalidation had nothing to invalidate. */
  if (a1 == a2) {
    printf("1610 before/after : %.9f unchanged -- swe_deltat.txt did not move\n"
           "                    dt[0], so this run proves nothing\n", a1);
    bad = 1;
  } else {
    printf("1610 before load  : %.9f   (memoised against the built-in table)\n", a1);
  }

  printf("1610 after load   : A %.9f   B %.9f %s\n",
         a2, b1, a2 == b1 ? "OK" : "STALE");
  if (a2 != b1) bad = 1;

  swe_ctx_free(a);
  swe_ctx_free(b);
  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad;
}
