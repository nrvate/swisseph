/* Order-dependence probe.
 *
 * The tid_acc bug had a signature: a result that depended on what had been
 * computed before it. This looks for more of the same shape by brute force.
 *
 * Usage: ./order <prior> <target>   -- prints the target's value(s)
 *
 * The driver runs every (prior, target) pair in its OWN process and compares
 * each against prior 0 ("nothing"). A separate process per measurement is the
 * point: swe_close() is itself part of what is under suspicion, so it cannot
 * be trusted to produce a clean slate.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

#define EPHE "../ephe"

static const double JD_OLD = 1356173.5;      /* -3000: delta-t is hours here */
static const double JD_MID = 2451545.0;      /* J2000 */
static const double JD_NEW = 2469807.5;      /* 2050  */

static void setup(void) { swe_set_ephe_path(EPHE); }

/* ---- priors: things a program might plausibly do first ---------------- */
static const char *PRIOR_NAME[] = {
  "nothing",
  "moseph calc",
  "jpl calc (de200)",
  "swieph calc, far date",
  "sidereal LAHIRI calc",
  "sidereal FAGAN calc",
  "topocentric calc",
  "heliocentric calc",
  "barycentric calc",
  "J2000 calc",
  "houses(placidus)",
  "fixstar Aldebaran",
  "set_tid_acc manual",
  "set_delta_t_userdef",
  "set_sid_mode then reset",
  "swe_close()",
  "calc TRUE_NODE moseph",
  "set_jpl_file de200",
};
#define NPRIOR ((int)(sizeof(PRIOR_NAME)/sizeof(PRIOR_NAME[0])))

static void run_prior(int p)
{
  double x[6], c[13], a[10]; char serr[AS_MAXCH]; char s[AS_MAXCH];
  switch (p) {
    case 0: break;
    case 1: swe_calc(JD_MID, SE_SUN, SEFLG_MOSEPH, x, serr); break;
    case 2: strcpy(s, "de200.eph"); swe_set_jpl_file(s);
            swe_calc(2444204.0, SE_SUN, SEFLG_JPLEPH, x, serr); break;
    case 3: swe_calc(JD_NEW, SE_PLUTO, SEFLG_SWIEPH, x, serr); break;
    case 4: swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
            swe_calc(JD_MID, SE_SUN, SEFLG_SWIEPH|SEFLG_SIDEREAL, x, serr); break;
    case 5: swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
            swe_calc(JD_MID, SE_SUN, SEFLG_SWIEPH|SEFLG_SIDEREAL, x, serr); break;
    case 6: swe_set_topo(13.4, 52.5, 100);
            swe_calc(JD_MID, SE_SUN, SEFLG_SWIEPH|SEFLG_TOPOCTR, x, serr); break;
    case 7: swe_calc(JD_MID, SE_MARS, SEFLG_SWIEPH|SEFLG_HELCTR, x, serr); break;
    case 8: swe_calc(JD_MID, SE_MARS, SEFLG_SWIEPH|SEFLG_BARYCTR, x, serr); break;
    case 9: swe_calc(JD_MID, SE_SUN, SEFLG_SWIEPH|SEFLG_J2000, x, serr); break;
    case 10: swe_houses(JD_MID, 52.5, 13.4, 'P', c, a); break;
    case 11: strcpy(s, "Aldebaran"); swe_fixstar2(s, JD_MID, SEFLG_SWIEPH, x, serr); break;
    case 12: swe_set_tid_acc(-25.0); break;
    case 13: swe_set_delta_t_userdef(70.0 / 86400.0); break;
    case 14: swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
             swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0); break;
    case 15: swe_close(); setup(); break;
    case 16: swe_calc(JD_MID, SE_TRUE_NODE, SEFLG_MOSEPH, x, serr); break;
    case 17: strcpy(s, "de200.eph"); swe_set_jpl_file(s); break;
  }
}

/* ---- targets: things whose value must not depend on the above --------- */
static const char *TARGET_NAME[] = {
  "calc_ut Sun swieph -3000",
  "calc_ut Moon swieph -3000",
  "calc_ut Sun swieph J2000",
  "calc    Sun swieph J2000",
  "calc_ut Pluto swieph 2050",
  "deltat -3000",
  "ayanamsa_ut J2000",
  "houses placidus cusp1",
  "calc_ut Sun moseph -3000",
  "sidtime J2000",
};
#define NTARGET ((int)(sizeof(TARGET_NAME)/sizeof(TARGET_NAME[0])))

static int run_target(int t, double *out)
{
  double x[6], c[13], a[10]; char serr[AS_MAXCH];
  int n = 1;
  switch (t) {
    case 0: swe_calc_ut(JD_OLD, SE_SUN, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
            out[0]=x[0]; out[1]=x[3]; n=2; break;
    case 1: swe_calc_ut(JD_OLD, SE_MOON, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
            out[0]=x[0]; out[1]=x[3]; n=2; break;
    case 2: swe_calc_ut(JD_MID, SE_SUN, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
            out[0]=x[0]; n=1; break;
    case 3: swe_calc(JD_MID, SE_SUN, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
            out[0]=x[0]; n=1; break;
    case 4: swe_calc_ut(JD_NEW, SE_PLUTO, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
            out[0]=x[0]; n=1; break;
    case 5: out[0] = swe_deltat_ex(JD_OLD, SEFLG_SWIEPH, serr); n=1; break;
    case 6: out[0] = swe_get_ayanamsa_ut(JD_MID); n=1; break;
    case 7: swe_houses(JD_MID, 52.5, 13.4, 'P', c, a); out[0]=c[1]; out[1]=a[0]; n=2; break;
    case 8: swe_calc_ut(JD_OLD, SE_SUN, SEFLG_MOSEPH|SEFLG_SPEED, x, serr);
            out[0]=x[0]; n=1; break;
    case 9: out[0] = swe_sidtime(JD_MID); n=1; break;
  }
  return n;
}

int main(int argc, char **argv)
{
  double out[6] = {0};
  int p, t, n, i;
  if (argc == 2 && strcmp(argv[1], "--list") == 0) {
    printf("%d %d\n", NPRIOR, NTARGET);
    for (i = 0; i < NPRIOR; i++) printf("P%d\t%s\n", i, PRIOR_NAME[i]);
    for (i = 0; i < NTARGET; i++) printf("T%d\t%s\n", i, TARGET_NAME[i]);
    return 0;
  }
  if (argc != 3) { fprintf(stderr, "usage: order <prior> <target>\n"); return 2; }
  p = atoi(argv[1]); t = atoi(argv[2]);
  setup();
  run_prior(p);
  n = run_target(t, out);
  for (i = 0; i < n; i++) printf("%.17g%s", out[i], i + 1 < n ? " " : "\n");
  swe_close();
  return 0;
}
