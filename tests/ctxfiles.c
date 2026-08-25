/* G23: the data files a context reads along its OWN ephemeris path.
 *
 * init_leapsec() seeds ctx->leap_seconds from the 27 built-in entries and
 * then EXTENDS them from seleapsec.txt in the ephemeris directory, so a
 * caller can add leap seconds announced after this release. Nothing tested
 * the file half: the reading loop, the "<= last built-in" skip and the
 * NLEAP_SECONDS_SPACE cap were all unexecuted, and swedate.c was the
 * third-least-covered file at 81.2%.
 *
 * Observable without white-box access. A UTC time of 23:59:60 is valid only
 * on a day the table lists, so swe_utc_to_jd() accepting it is a direct
 * read-out of whether the file was found and parsed:
 *
 *   with the fixture   2026-12-31 23:59:60 -> OK
 *   without it         2026-12-31 23:59:60 -> "invalid time (no leap second!)"
 *
 * Two contexts rather than two processes, because leap_seconds is per-context
 * and loaded once per context -- the same reason eopload.c uses two.
 *
 * The second half is seorbel.txt, the fictitious-body element file, which had
 * the identical defect: swe_get_planet_name_r() fetched the name through
 * swi_get_fict_name(swi_default_ctx(), ...) instead of the caller's context,
 * so it read whichever directory the DEFAULT context happened to point at.
 * The fixture renames body 0 to ZZTESTBODY; the built-in name is Cupido.
 *
 * The third assertion is what makes the first mean something: a date the
 * fixture does NOT list must still be refused even by the context that read
 * it. Without that, a bug that accepted 23:59:60 on any day would pass.
 *
 *   leapsec <dir-with-seleapsec> <dir-without>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

#define LEAP_Y  2026            /* the fixture's added leap second */
#define NOLEAP_Y 2025           /* a year the fixture does not list */

static int check(swe_ctx *c, const char *who, int year, int expect_ok)
{
  double dret[2];
  char serr[AS_MAXCH] = "";
  int32 rf;
  memset(dret, 0, sizeof dret);
  rf = swe_utc_to_jd_r(c, year, 12, 31, 23, 59, 60.0, SE_GREG_CAL, dret, serr);
  if ((rf != ERR) == (expect_ok != 0)) {
    printf("  %-22s %d-12-31 23:59:60  %-8s OK\n", who, year,
           expect_ok ? "accepted" : "refused");
    return 0;
  }
  printf("  %-22s %d-12-31 23:59:60  FAIL: expected %s, got rf=%d %.80s\n",
         who, year, expect_ok ? "accepted" : "refused", (int) rf, serr);
  return 1;
}

int main(int argc, char **argv)
{
  const char *dir_leap = (argc > 1) ? argv[1] : ".leapsec-with";
  const char *dir_bare = (argc > 2) ? argv[2] : ".leapsec-without";
  swe_ctx *a, *b;
  int bad = 0;

  printf("G23: per-context ephemeris files (seleapsec.txt, seorbel.txt)\n");
  a = swe_ctx_new();
  b = swe_ctx_new();
  if (a == NULL || b == NULL) {
    printf("  FAIL: swe_ctx_new() returned NULL\n");
    return 1;
  }
  swe_set_ephe_path_r(a, (char *) dir_leap);
  swe_set_ephe_path_r(b, (char *) dir_bare);

  /* The fixture's leap second is accepted only where the file was read. */
  bad |= check(a, "with seleapsec.txt", LEAP_Y, 1);
  bad |= check(b, "without the file", LEAP_Y, 0);
  /* ...and the file adds only what it lists, not "any 23:59:60". */
  bad |= check(a, "with seleapsec.txt", NOLEAP_Y, 0);

  /* seorbel.txt: the name must come from the context that was asked. */
  {
    char nm[AS_MAXCH] = "";
    swe_get_planet_name_r(a, SE_FICT_OFFSET, nm);
    if (strstr(nm, "ZZTESTBODY") != NULL) {
      printf("  %-22s fictitious body 0 -> %-12s OK\n", "with seorbel.txt", nm);
    } else {
      printf("  %-22s fictitious body 0 -> %s  FAIL: read another context's path\n",
             "with seorbel.txt", nm);
      bad = 1;
    }
    nm[0] = '\0';
    swe_get_planet_name_r(b, SE_FICT_OFFSET, nm);
    if (strstr(nm, "ZZTESTBODY") == NULL) {
      printf("  %-22s fictitious body 0 -> %-12s OK\n", "without the file", nm);
    } else {
      printf("  %-22s fictitious body 0 -> %s  FAIL: saw the other context's file\n",
             "without the file", nm);
      bad = 1;
    }
  }

  swe_ctx_free(a);
  swe_ctx_free(b);
  printf(bad ? "G23 FAIL\n" : "G23 PASS\n");
  return bad ? 1 : 0;
}
