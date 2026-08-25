/* firstcall.c -- G20: a public function must not need a warm-up call.
 *
 * swe_sol_eclipse_when_loc() returned "geographic position has not been
 * set" when it was the FIRST library call in a process, and the correct
 * answer if literally anything had run before it. It takes geopos as an
 * argument and sets the observer from it internally, so a caller has no
 * reason to expect that.
 *
 * The cause was ordering inside the configuration layer.
 * swi_init_swed_if_start() carries swi_config_sync(), which adopts what
 * another thread has published. swe_set_topo_r() calls it, but under
 * swi_config_begin_apply(), and sync is a documented no-op while a setter
 * is mid-apply; SWI_CFG_LOCAL wraps it the same way again. So on the first
 * call the adopt never happened there -- it happened later, at the first
 * swe_calc() inside the search, and re-applied the master configuration
 * over the observer that had just been set. Every entry point in sweph.c
 * and swephlib.c was immune because they call swi_init_swed_if_start()
 * themselves; nothing in swecl.c did.
 *
 * The transcript cannot catch this. It is one process, and by the time it
 * reaches an eclipse row a hundred other calls have already synced, so the
 * failing path does not exist there. This needs a fresh process per
 * function, which is what the argument is for: the Makefile runs this once
 * per index, and each run makes exactly one library call.
 *
 *   firstcall <ephe-dir> <index>     one function, as the first call
 *   firstcall <ephe-dir> count       how many indices there are
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

/* A place and a date where each of these has something to find. The
 * eclipse dates come from swe_sol_eclipse_when_glob() and the location
 * from swe_sol_eclipse_where(), so the searches are pointed somewhere the
 * answer exists rather than at a guess. */
#define TJD    2452082.002563
#define GLON      2.7443
#define GLAT    (-11.2584)

/* Bump this when a case is added below. */
#define NFIRSTCALL 9

static int run(int k, const char *ephe, const char **name)
{
  double geo[3] = { GLON, GLAT, 0 };
  double tret[10], attr[20], dret[10];
  char serr[AS_MAXCH] = "", star[AS_MAXCH];
  int32 rf = 0;

  memset(tret, 0, sizeof tret);
  memset(attr, 0, sizeof attr);
  memset(dret, 0, sizeof dret);
  swe_set_ephe_path((char *) ephe);   /* configuration, not a calculation */

  switch (k) {
    case 0: *name = "swe_sol_eclipse_when_loc";
      rf = swe_sol_eclipse_when_loc(TJD - 5, SEFLG_SWIEPH, geo, tret, attr, 0, serr);
      break;
    case 1: *name = "swe_lun_eclipse_when_loc";
      rf = swe_lun_eclipse_when_loc(TJD - 5, SEFLG_SWIEPH, geo, tret, attr, 0, serr);
      break;
    case 2: *name = "swe_lun_occult_when_loc";
      rf = swe_lun_occult_when_loc(TJD - 5, SE_VENUS, NULL, SEFLG_SWIEPH,
                                   geo, tret, attr, 0, serr);
      break;
    case 3: *name = "swe_sol_eclipse_how";
      rf = swe_sol_eclipse_how(TJD, SEFLG_SWIEPH, geo, attr, serr);
      break;
    case 4: *name = "swe_lun_eclipse_how";
      rf = swe_lun_eclipse_how(2451564.696873, SEFLG_SWIEPH, geo, attr, serr);
      break;
    case 5: *name = "swe_rise_trans";
      rf = swe_rise_trans(TJD, SE_SUN, NULL, SEFLG_SWIEPH, SE_CALC_RISE,
                          geo, 1013.25, 15.0, dret, serr);
      break;
    case 6: *name = "swe_rise_trans_true_hor";
      rf = swe_rise_trans_true_hor(TJD, SE_SUN, NULL, SEFLG_SWIEPH, SE_CALC_RISE,
                                   geo, 1013.25, 15.0, 0.0, dret, serr);
      break;
    case 7: *name = "swe_gauquelin_sector";
      rf = swe_gauquelin_sector(TJD, SE_MARS, NULL, SEFLG_SWIEPH, 0,
                                geo, 1013.25, 15.0, dret, serr);
      break;
    case 8: *name = "swe_heliacal_ut";
      { double datm[4] = {1013.25, 15, 40, 0}, dobs[6] = {36, 1, 1, 1, 1, 1};
        strcpy(star, "venus");
        rf = swe_heliacal_ut(TJD, geo, datm, dobs, star,
                             SE_HELIACAL_RISING, SEFLG_SWIEPH, dret, serr); }
      break;
    default: return -2;
  }
  /* A refusal that names the observer is the failure this gate exists for.
   * Anything else -- including "no eclipse found" -- is the function
   * working. */
  if (rf == ERR && strstr(serr, "geographic position") != NULL) {
    printf("  %-28s FAIL  rf=%d  %s\n", *name, (int) rf, serr);
    return 1;
  }
  printf("  %-28s ok    rf=%d\n", *name, (int) rf);
  return 0;
}

int main(int argc, char **argv)
{
  const char *ephe = (argc > 1) ? argv[1] : "../ephe";
  const char *name = "?";
  int k, bad;

  /* The Makefile asks how many indices to loop over rather than carrying a
   * copy of the number that would rot the moment one is added here. */
  if (argc > 2 && strcmp(argv[2], "count") == 0) {
    printf("%d\n", NFIRSTCALL);
    return 0;
  }
  k = (argc > 2) ? atoi(argv[2]) : 0;
  bad = run(k, ephe, &name);
  if (bad == -2) { printf("no such index %d\n", k); return 2; }
  return bad;
}
