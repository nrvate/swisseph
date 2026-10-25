/* nutseries.c -- G162: SE_ECL_NUT answers the nutation its flags and model
 * ask for, whatever was asked before, and the series' own values.
 *
 * The defect (UPSTREAM-BUGS 218): under SEFLG_NONUT, and SEFLG_SIDEREAL
 * and SEFLG_J2000, which imply it, swi_check_nutation() computes no
 * nutation, and SE_ECL_NUT answered the one cached for the last date asked
 * without the flag: at 2460000.5 after a call at J2000, J2000's dpsi
 * -0.003870001 and deps -0.001602611 deg and the true obliquity
 * 23.434664974 for the mean 23.436267584, where a fresh context answers
 * zero nutation and the true obliquity the mean ("no nutation, i.e. mean
 * equinox of date"). Now that, at every call.
 *
 * The defect (UPSTREAM-BUGS 219): IAU 1980 and its Herring 1987
 * corrections, and SEFLG_JPLHOR's IAU 1980, evaluated their arguments,
 * polynomials to T^3 some 1e13" at 1e9 days, in double: against the series
 * in mpmath dpsi 1.2e-10 deg off at |J - J2000| 4e8 to 1e9 days, 9.0e-8
 * to 1e10, 3.7e-3 to 1e11 and 8.2 deg to 1e12. Now in double-double past
 * 4e8 days (bug 188's bound), as IAU 2000A's and 2000B's.
 *
 * The defect (UPSTREAM-BUGS 220): IAU 2000B as published (McCarthy &
 * Luzum 2003; SOFA iauNut00b) adds fixed offsets in lieu of the planetary
 * terms, dpsi -0.135 mas and deps +0.388 mas; they were left out, 3.75e-8
 * and 1.08e-7 deg at every date.
 *
 * Check (a): SE_ECL_NUT with SEFLG_NONUT; SEFLG_NONUT | SEFLG_SPEED;
 * SEFLG_NONUT | SEFLG_RADIANS; SEFLG_NONUT | SEFLG_EQUATORIAL;
 * SEFLG_SIDEREAL; SEFLG_J2000; SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX |
 * SEFLG_NONUT (Moshier for the others), under the default, IAU 1980 and
 * IAU 2000B, at 1950 Jan 1 (IAU 1976 and 1980 under the Horizons
 * approximation), J2000 + 0.37 and 2460000.5: on a context that asked the
 * same date and then J2000 + 0.5 without the flags, against a fresh
 * context, bit for bit with the return flag; dpsi and deps 0, the true
 * obliquity the mean, and the mean the one the call without the flags
 * (and SEFLG_EQUATORIAL) answers. 63 checks.
 *
 * Check (b): SE_ECL_NUT's dpsi and deps under IAU 1980, IAU 1987, and the
 * default with SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX before 1962 (IAU 1980 and
 * the fixed 64.284 and 6.151 mas), at 32 instants with |J - J2000|
 * log-uniform over 4e8..1e12 days, either sign (the Horizons approximation
 * at the 16 before J2000), against the series evaluated in mpmath at 400
 * bits at the same double J (nt[] read from swephlib.c; the table below):
 * per decade within 1e-12 deg. 12 checks.
 *
 * Check (c): IAU 2000B. SE_ECL_NUT at eight dates -1000..3500 against the
 * library's 77 terms on its Simon et al. arguments with the offsets, in
 * mpmath: within 1e-13 deg. And at TT 2453736.5 (2006 Jan 1) against
 * SOFA's test values for iauNut00b (t_sofa_c.c, -0.9632552291148362783e-5
 * and 0.4063197106621159367e-4 rad): within 3e-11 rad, the rest being
 * SOFA's arguments, their linear terms alone. 2 checks.
 *
 * Check (d): swi_nutation_rate() under IAU 1980 and 1987 at 32 instants
 * with |J - J2000| log-uniform over 4.1e8..1e9 days against five-point
 * differences of swi_nutation() over 2^-6 day: within 1e-12 deg/day, the
 * rate the places' derivative past 4e8 too. 4 checks; it calls the fork's
 * own functions and is left out of the upstream build.
 *
 * Measured: (a) 0 of 63 differ; (b) IAU 1980 and 1987 within 6.9e-17,
 * 5.6e-16, 6.7e-15 and 8.5e-14 deg per decade, the Horizons approximation
 * 1.8e-14 at most; (c) 1.6e-14 deg, and 1.02e-11 rad off SOFA's; (d)
 * 1.6e-14 and 2.5e-14 deg/day. Before the fix all 81 fail: (a) 63, the
 * nutation of J2000 + 0.5 answered (dpsi -0.00387 deg); (b) 1.4e-10,
 * 7.6e-8, 2.4e-3 and 8.7 deg; (c) 1.08e-7 deg and 1.88e-9 rad; (d) 8.2e-9
 * and 6.3e-9 deg/day (the places' jitter). The upstream build fails 77 of
 * its 77, (a)'s SEFLG_EQUATORIAL cases answering zeros besides (UPSTREAM-BUGS
 * 90). Run as
 *   nutseries EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"
#ifndef SWE_UPSTREAM_COMPAT
#include "sweph.h"
#include "swephlib.h"
#endif

/* check (b)'s reference: J, then dpsi and deps under IAU 1980 and under
 * IAU 1987, deg, the series in mpmath at 400 bits (calc_nutation_iau1980()
 * transcribed: its nt[] table, the first terms, the amplitudes' T terms,
 * the arguments' polynomials at the exact T of the double J, reduced mod
 * 1296000" exactly) */
static const double REF80[32][5] = {
  {-400016469.35546875, -0.045974932754378729, -0.00016074489048450538, -0.045974826245533944, -0.00016193146453473338},
  {499864552.51953125, 0.0088747201393216766, -0.0052646571416381455, 0.008874002877445426, -0.0052646091258479509},
  {-512449110.71875, -0.047601918589426646, 0.0018339859537874214, -0.047602640293331784, 0.0018335924216593301},
  {579695397.98828125, 0.029947916518512394, -0.0073872978204367144, 0.029947043535792212, -0.007387497656674992},
  {-667241439.19921875, -0.0074235309447653105, -0.0019682896541102783, -0.0074221398022401557, -0.0019685807697283619},
  {763069118.08203125, 0.10359800364340545, -5.5229135194938228e-05, 0.10360019884738381, -5.6615349988433247e-05},
  {-888465343.71484375, 0.052839151801151847, -0.0014263520734884372, 0.052838859879191728, -0.001426195513527681},
  {924864224.265625, 0.084379221952336242, -0.008580084155162435, 0.084380059007846259, -0.0085806156889117538},
  {-1138993749.2460938, 0.13750345394449337, -0.00024669502153312777, 0.13750101874396467, -0.00024582525478598677},
  {1408227474.7773438, 0.14810686947790802, -0.011101182252527987, 0.14810747376261937, -0.011101582968194504},
  {-2005300029.1445312, 0.2468894512638084, 0.0085691260122173138, 0.24688524114343186, 0.0085692569528113428},
  {2601143812.5820312, -0.33806818318367965, -0.0061616255550536415, -0.33807140946247077, -0.0061614843481316382},
  {-4105634084.1445312, -0.51840995748760388, 0.0040947709035101714, -0.51840817643955284, 0.0040936740694450212},
  {4296492753.8398438, 0.5657364834356472, -0.0038011136777119344, 0.56573752907028174, -0.0038019635135746223},
  {-6635551399.84375, 0.85700178239037006, 0.010746458147635684, 0.8569982668797429, 0.010746890193393694},
  {9044113319.1992188, -0.43443285125046793, 0.069957283113067495, -0.43443162857843887, 0.069958783026017435},
  {-12477570038.296875, 1.3043902560271905, -0.053691315525434616, 1.3043880566972794, -0.053690520385953552},
  {14436263508.664062, 0.73462001134833577, 0.084430799796492054, 0.73462133545264086, 0.084430351371951304},
  {-18546700207.097656, 2.3599916532172509, 0.060018474412556699, 2.3599905346558341, 0.060019191857492951},
  {27078515815.355469, 1.9994158668395876, -0.10070729911763096, 1.9994146913701114, -0.10070791123534592},
  {-42082158553.082031, -1.8763651052017978, 0.29624319781717201, -1.8763655835081858, 0.29624177021220954},
  {54194817600.261719, -5.4427191146018155, 0.16995046066328867, -5.4427209374587147, 0.16995093365860048},
  {-73165843366.367188, -0.92282937606058635, -0.41925850685475485, -0.9228266385743592, -0.41925807980863111},
  {98721168365.761719, 3.4448521084086359, -0.56560912242249928, 3.4448513187751186, -0.56561024745626132},
  {-123551434105.40625, -12.712483842641044, -0.74488466836826428, -12.712482309569534, -0.74488422715887948},
  {154134806312.75391, 20.02906416114871, 0.47981346027363309, 20.029066781953166, 0.47981372761250657},
  {-233977798803.60938, 31.224112426117557, -0.43070710107525673, 31.224110588864068, -0.43070701686772939},
  {283719054047.62109, -15.433088023719968, 2.475077745545792, -15.433088478333302, 2.4750782707188037},
  {-359776591120.05469, 13.532671850267416, 2.2364588199676163, 13.532671239680145, 2.2364576910535447},
  {453647837306.60547, 18.415603770426006, 2.1836375660906961, 18.415604310482809, 2.1836379041304603},
  {-660844656945.28125, 45.880068898298347, 4.6997321165227124, 45.880066036538807, 4.6997313542652508},
  {982278926802.22266, 72.94897354793882, 7.7850321450385254, 72.948976020148436, 7.7850316293065003},
};

/* check (c)'s reference: J, dpsi, deps, deg, IAU 2000B as
 * calc_nutation_iau2000ab() sums it (its 77 terms from swenut2000a.h on
 * the Simon et al. 1994 arguments to T^4) plus -0.135 and +0.388 mas, in
 * mpmath at 300 bits */
static const double REF00B[8][3] = {
  {1355808.5, 0.0009395889370187588, -0.0026789106812159591},
  {2086308.5, -0.0018211930211964437, 0.0021755435933891788},
  {2415020.5, 0.0048425947962552306, -0.00063607291456128272},
  {2451545, -0.0038699066358249398, -0.0016026158548035686},
  {2453736.5, -0.00055190517821218936, 0.002328040404026737},
  {2460000.5, -0.0025762925589874869, 0.0021506353929542001},
  {2488069.5, 0.00091368469256131873, 0.0023788520771171502},
  {3000000.5, 0.0046238098459064712, -0.001128480166981362},
};

static swe_ctx *ctx_model(const char *ephe, const char *model)
{
  char m[40];
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  swe_set_ephe_path_r(c, ephe);
  strcpy(m, model);
  swe_set_astro_models_r(c, m, 0);
  return c;
}

int main(int argc, char **argv)
{
  static const char *MOD[3] = { "0,0,0,0", "0,0,0,1", "0,0,0,4" };
  static const char *MN[3] = { "default", "IAU 1980", "IAU 2000B" };
  static const int32 FL[7] = {
    SEFLG_MOSEPH | SEFLG_NONUT, SEFLG_MOSEPH | SEFLG_NONUT | SEFLG_SPEED,
    SEFLG_MOSEPH | SEFLG_NONUT | SEFLG_RADIANS, SEFLG_MOSEPH | SEFLG_NONUT | SEFLG_EQUATORIAL,
    SEFLG_MOSEPH | SEFLG_SIDEREAL, SEFLG_MOSEPH | SEFLG_J2000,
    SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX | SEFLG_NONUT };
  static const char *FN[7] = { "NONUT", "NONUT|SPEED", "NONUT|RADIANS", "NONUT|EQUATORIAL",
    "SIDEREAL", "J2000", "JPLEPH|JPLHOR_APPROX|NONUT" };
  static const double TD[3] = { 2433282.5, 2451545.37, 2460000.5 };
  const int32 NONUTS = SEFLG_NONUT | SEFLG_SIDEREAL | SEFLG_J2000;
  char serr[256];
  int m, f, d, i, j, k, n = 0, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: nutseries EPHEDIR\n");
    return 2;
  }
  printf("G162: SE_ECL_NUT answers its flags' and model's nutation, the series' own\n");

  /* check (a) */
  printf("  (a) SEFLG_NONUT and the flags that imply it, after calls without them:"
         " failing cases per model\n");
  for (m = 0; m < 3; m++) {
    int fails = 0;
    for (f = 0; f < 7; f++)
      for (d = 0; d < 3; d++) {
        int32 base = FL[f] & ~NONUTS, ra, rb, rp;
        double xa[6], xb[6], xp[6], junk[6];
        int ok;
        swe_ctx *a = ctx_model(argv[1], MOD[m]), *b = ctx_model(argv[1], MOD[m]);
        if (a == NULL || b == NULL) {
          printf("  ERROR: no context\nG162 FAIL: ERROR\n");
          return 1;
        }
        ra = swe_calc_r(a, TD[d], SE_ECL_NUT, FL[f], xa, serr);
        rp = swe_calc_r(b, TD[d], SE_ECL_NUT, base & ~SEFLG_EQUATORIAL, xp, serr);
        swe_calc_r(b, 2451545.5, SE_ECL_NUT, base, junk, serr);
        rb = swe_calc_r(b, TD[d], SE_ECL_NUT, FL[f], xb, serr);
        ok = ra >= 0 && rp >= 0 && rb == ra && memcmp(xa, xb, sizeof xa) == 0
             && xa[2] == 0 && xa[3] == 0 && xa[0] == xa[1] && xa[1] == xp[1];
        for (k = 4; k < 6; k++)
          ok = ok && xa[k] == 0;
        n++;
        if (!ok) {
          bad++;
          if (fails++ < 3)
            printf("    FAIL %s, %s at %.2f: fresh %d %.9f %.9f %.9f %.9f, after %d %.9f %.9f %.9f %.9f,"
                   " mean without the flag %.9f\n", MN[m], FN[f], TD[d], ra, xa[0], xa[1], xa[2], xa[3],
                   rb, xb[0], xb[1], xb[2], xb[3], xp[1]);
        }
        swe_ctx_free(a);
        swe_ctx_free(b);
      }
    printf("    %-10s %d of 21\n", MN[m], fails);
  }

  /* check (b) */
  {
    static const char *bn[3] = { "IAU 1980", "IAU 1987", "JPLHOR_APPROX" };
    static const char *bm[3] = { "0,0,0,1", "0,0,0,2", "0,0,0,0" };
    static const int32 bf[3] = { SEFLG_MOSEPH, SEFLG_MOSEPH, SEFLG_JPLEPH | SEFLG_JPLHOR_APPROX };
    const double off[2] = { 0.064284 / 3600, 0.006151 / 3600 };
    printf("  (b) IAU 1980 past 4e8 days against the series in mpmath: worst |dpsi, deps - ref|, deg,"
           " |J - J2000| 4e8..1e9, ..1e10, ..1e11, ..1e12\n");
    for (m = 0; m < 3; m++) {
      double w[4] = { 0, 0, 0, 0 };
      swe_ctx *c = ctx_model(argv[1], bm[m]);
      if (c == NULL) {
        printf("  ERROR: no context\nG162 FAIL: ERROR\n");
        return 1;
      }
      for (i = 0; i < 32; i++) {
        double x[6], J = REF80[i][0];
        int col = (m == 1) ? 3 : 1;
        if (m == 2 && J > 0)
          continue;
        if (swe_calc_r(c, J, SE_ECL_NUT, bf[m], x, serr) < 0) {
          printf("  ERROR SE_ECL_NUT at %.8f: %s\nG162 FAIL: ERROR\n", J, serr);
          return 1;
        }
        for (j = 0; j < 2; j++) {
          double e = fabs(x[2 + j] - (REF80[i][col + j] + (m == 2 ? off[j] : 0)));
          if (!(e <= w[i / 8]))
            w[i / 8] = e;
        }
      }
      swe_ctx_free(c);
      printf("    %-14s %.2e  %.2e  %.2e  %.2e\n", bn[m], w[0], w[1], w[2], w[3]);
      for (k = 0; k < 4; k++) {
        n++;
        if (!(w[k] <= 1e-12)) {
          bad++;
          printf("    FAIL %s, |J - J2000| %.0e..%.0e: %.2e > 1e-12\n", bn[m], k ? pow(10, 8 + k) : 4e8,
                 pow(10, 9 + k), w[k]);
        }
      }
    }
  }

  /* check (c) */
  {
    static const double sofa[2] = { -0.9632552291148362783e-5, 0.4063197106621159367e-4 };
    double w = 0, ws = 0, x[6];
    swe_ctx *c = ctx_model(argv[1], "0,0,0,4");
    if (c == NULL) {
      printf("  ERROR: no context\nG162 FAIL: ERROR\n");
      return 1;
    }
    for (i = 0; i < 8; i++) {
      if (swe_calc_r(c, REF00B[i][0], SE_ECL_NUT, SEFLG_MOSEPH, x, serr) < 0) {
        printf("  ERROR SE_ECL_NUT at %.8f: %s\nG162 FAIL: ERROR\n", REF00B[i][0], serr);
        return 1;
      }
      for (j = 0; j < 2; j++)
        if (!(fabs(x[2 + j] - REF00B[i][1 + j]) <= w))
          w = fabs(x[2 + j] - REF00B[i][1 + j]);
      if (REF00B[i][0] == 2453736.5)
        for (j = 0; j < 2; j++)
          if (!(fabs(x[2 + j] * DEGTORAD - sofa[j]) <= ws))
            ws = fabs(x[2 + j] * DEGTORAD - sofa[j]);
    }
    swe_ctx_free(c);
    printf("  (c) IAU 2000B: worst |dpsi, deps - ref| %.2e deg, -1000..3500; against SOFA's"
           " iauNut00b test at 2006 Jan 1 %.2e rad\n", w, ws);
    n += 2;
    if (!(w <= 1e-13)) {
      bad++;
      printf("    FAIL IAU 2000B against its series with the offsets: %.2e > 1e-13\n", w);
    }
    if (!(ws <= 3e-11)) {
      bad++;
      printf("    FAIL IAU 2000B against SOFA's test value: %.2e > 3e-11\n", ws);
    }
  }

#ifndef SWE_UPSTREAM_COMPAT
  /* check (d) */
  {
    static const char *dm[2] = { "0,0,0,1", "0,0,0,2" };
    const double h = 1.0 / 64;
    printf("  (d) swi_nutation_rate() past 4e8 days: worst |rate - five-point difference|,"
           " deg/day, dpsi / deps\n");
    for (m = 0; m < 2; m++) {
      double w[2] = { 0, 0 };
      swe_ctx *c = ctx_model(argv[1], dm[m]);
      if (c == NULL) {
        printf("  ERROR: no context\nG162 FAIL: ERROR\n");
        return 1;
      }
      srand(20261025);
      for (i = 0; i < 32; i++) {
        double u = rand() / (double) RAND_MAX, dj = 4.1e8 * pow(1e9 / 4.1e8, u), r[2], v[5][2];
        double t = floor((2451545.0 + ((i & 1) ? dj : -dj)) * 256) / 256;
        swi_nutation_rate(c, t, 0, r);
        for (k = 0; k < 5; k++)
          swi_nutation(c, t + (k - 2) * h, 0, v[k]);
        for (j = 0; j < 2; j++) {
          double e = fabs(r[j] - (8 * (v[3][j] - v[1][j]) - (v[4][j] - v[0][j])) / (12 * h)) * RADTODEG;
          if (!(e <= w[j]))
            w[j] = e;
        }
      }
      swe_ctx_free(c);
      printf("    %-14s %.2e  %.2e\n", m ? "IAU 1987" : "IAU 1980", w[0], w[1]);
      for (j = 0; j < 2; j++) {
        n++;
        if (!(w[j] <= 1e-12)) {
          bad++;
          printf("    FAIL %s, %s: %.2e > 1e-12\n", m ? "IAU 1987" : "IAU 1980", j ? "deps" : "dpsi", w[j]);
        }
      }
    }
  }
#endif
  if (bad) {
    printf("G162 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG162 PASS\n", n);
  return 0;
}
