/* hposflag.c -- G167: no flag that names a place's frame or form moves a
 * Gauquelin sector, nor swetest's house positions.
 *
 * The defect (UPSTREAM-BUGS 252): swe_gauquelin_sector()'s methods 0 and
 * 1 set the place the body's call answered with iflag in the frame of the
 * true ecliptic of date (the true obliquity, the ARMC), so every flag that
 * names that place's frame or form, not its direction, moved the sector:
 * Mars at UT 2433282.5, Zurich, Swiss files, method 0, SEFLG_NONUT
 * -1.14e-4 sector, SEFLG_ICRS -1.5e-8, SEFLG_J2000 -0.0875,
 * SEFLG_EQUATORIAL -0.148, SEFLG_RADIANS +17.8, SEFLG_XYZ +18.3, and
 * SEFLG_HELCTR and SEFLG_BARYCTR, a direction from the Sun, +3.88 (bug 236
 * cleared SEFLG_SIDEREAL). iflag is "flag for ephemeris and SEFLG_TOPOCTR"
 * (swephprg 16.1); methods 2 to 5 pass the ephemeris bits alone.
 * Check (a): methods 0 and 1 with each of SEFLG_SPEED, SPEED3, NONUT,
 * ICRS, J2000, EQUATORIAL, RADIANS, XYZ, HELCTR, BARYCTR, and all of them
 * with SEFLG_SIDEREAL, on each base of the flags that choose the
 * direction (none, SEFLG_TOPOCTR, TRUEPOS, NOABERR | NOGDEFL, CENTER_BODY,
 * JPLHOR_APPROX; with SEFLG_SIDEREAL the base less the Horizons bits, which
 * the sidereal place drops), the call with the base alone, values and
 * returns bit for bit: the Sun, Moon, Mars, Jupiter, mean node and Spica
 * at G164's six dates 1800..2100, on de200, the Swiss files and Moshier;
 * one check per flag, ephemeris and method: 66. Check (b): on each base,
 * methods 0 and 1 are swe_house_pos() of the place the base gives, in the
 * frame of SE_ECL_NUT's true obliquity and the sidereal time with the
 * Horizons bits the place's call returned, within 1e-11 sector: 18
 * checks. Check (c): methods 2 to 5 with each flag as without, Mars and
 * the Moon at 1950 and 2000: 4 checks.
 *
 * The defect (UPSTREAM-BUGS 253): swetest's house positions (-fg, -fG,
 * -fj) were swe_house_pos() of the place it printed: Mars at 1.1.1950 0h
 * UT, Zurich, 'G', -fj 5.3075773 with -sid1 for 2.6855985 without, -j2000
 * 2.5980926, -nonut 2.6855004, -hel 6.5610087; in the diff modes the
 * position of the difference or midpoint, less or about the second
 * body's (Mars - Sun, 'P', -fg -15.306 for -100.930); the cusps' those of
 * the cusps printed (sidereal Placidus house 1 at 330.07 deg).
 * Check (d): the built swetest at Zurich, 1.1.1800 0h UT and 150 and 300
 * years on, the house positions (-fPgj) of the Moon, Mars, Jupiter and
 * Spica, of Mars and Jupiter less, about and less the heliocentric Moon
 * (-d1, -D1, -dh1), and of the cusps and axes, with each of -sid1,
 * -sidt01, -sidsp1, -j2000, -nonut, -icrs, -hel, -bary, -speed, -speed3,
 * -i's SEFLG_EQUATORIAL, RADIANS, XYZ and all of them with J2000 and
 * NONUT, and -sid1 -j2000 -nonut -icrs -hel together, and on -topo, -i
 * with SEFLG_TOPOCTR, -true, de200 with -jplhora and without, and
 * Moshier, the run without, as printed: in P, W and 'G' by method 1, and
 * for five options also in G K E I N and 'G' by method 3 ('W' and 'N' not
 * with -sid, bug 254's); one check per option and body set: 84. Check (e):
 * swetest's printed house numbers of the Moon, Mars, Jupiter and Spica are
 * swe_house_pos() of their tropical places in the frame of the true
 * obliquity and swetest's ARMC, the diff modes' the difference and
 * midpoint of the bodies' (Mars less the Moon, Jupiter about it), each
 * cusp at its own number, within 6e-8 house (the print's 5e-8; 'G''s
 * cusps 13 to 36 miss swe_house_pos()'s exact-cusp test by 2.78e-8 sector),
 * in P K G R C E and 'G' by method 1, plain, with -sid1 and with -j2000:
 * 21 checks.
 *
 * Measured: (a) all equal, (b) within 7.1e-15 sector, (c) all equal, (d)
 * all equal, (e) within 4.9e-8 house. Before the fix 144 of the 193 fail:
 * 52 of (a) (NONUT up to 6.1e-4 sector, ICRS 9.9e-7 off the Swiss files
 * and Moshier, J2000 0.33, EQUATORIAL 8.09, RADIANS 17.9, XYZ 17.7, HELCTR
 * 13.9, BARYCTR 17.8, all 17.7; SPEED, SPEED3 and ICRS on de200 pass), 72
 * of (d) (up to 179.9 deg; -speed, -speed3, -hel on -topo and -icrs on
 * de200 pass) and 20 of (e) (5.3 houses plain, the diff modes; 11.1 with
 * -sid1, 33.2 sectors in 'G'; 'G' by method 1 plain passes). The upstream
 * build fails 158 of the 193: those, 8 more of (a), SPEED and SPEED3
 * moving the sector there (7.3e-9 on the Swiss files), 3 of (b), bug 223's
 * Horizons frame (1.9e-6 sector), and 3 more of (d), -speed3 in the diff
 * modes (3.3e-6). Run as
 *   hposflag EPHEDIR SWETEST
 * with SWETEST a swetest built from ../swetest.c on the same objects.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swephexp.h"

static int n, bad;
static const char *EPH, *SWT;

static const double TQ[] = {
  2378496.5, 2433282.5, 2437684.5 - 1.0 / 256, 2437684.5 + 1.0 / 256, 2451545.0, 2488069.5,
};
#define NTQ ((int) (sizeof TQ / sizeof TQ[0]))
static const int32 EPHF[3] = { SEFLG_JPLEPH, SEFLG_SWIEPH, SEFLG_MOSEPH };
static const char *EPHN[3] = { "de200", "Swiss", "Moshier" };
static double GEO[3] = { 8.55, 47.37, 400 };

/* the flags that name the place's frame or form, or a centre not the
 * observer's, one at a time and all */
#define ALLF (SEFLG_SPEED | SEFLG_NONUT | SEFLG_ICRS | SEFLG_J2000 | SEFLG_EQUATORIAL \
              | SEFLG_RADIANS | SEFLG_XYZ | SEFLG_HELCTR | SEFLG_SIDEREAL)
static const int32 FF[] = {
  SEFLG_SPEED, SEFLG_SPEED3, SEFLG_NONUT, SEFLG_ICRS, SEFLG_J2000, SEFLG_EQUATORIAL,
  SEFLG_RADIANS, SEFLG_XYZ, SEFLG_HELCTR, SEFLG_BARYCTR, ALLF,
};
static const char *FFN[] = {
  "SPEED", "SPEED3", "NONUT", "ICRS", "J2000", "EQUATORIAL", "RADIANS", "XYZ", "HELCTR",
  "BARYCTR", "all with SIDEREAL",
};
#define NFF ((int) (sizeof FF / sizeof FF[0]))
/* the flags that choose the body's direction from the observer */
static const int32 DD[] = {
  0, SEFLG_TOPOCTR, SEFLG_TRUEPOS, SEFLG_NOABERR | SEFLG_NOGDEFL, SEFLG_CENTER_BODY,
  SEFLG_JPLHOR_APPROX,
};
static const char *DDN[] = {
  "no flag", "TOPOCTR", "TRUEPOS", "NOABERR|NOGDEFL", "CENTER_BODY", "JPLHOR_APPROX",
};
#define NDD ((int) (sizeof DD / sizeof DD[0]))
/* the bodies; -1 Spica */
static const int PL[] = { SE_SUN, SE_MOON, SE_MARS, SE_JUPITER, SE_MEAN_NODE, -1 };
#define NPL ((int) (sizeof PL / sizeof PL[0]))

static swe_ctx *open_ctx(void)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  swe_set_ephe_path_r(c, EPH);
  swe_set_jpl_file_r(c, "de200.eph");
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  swe_set_topo_r(c, GEO[0], GEO[1], GEO[2]);
  return c;
}

static int32 sector(swe_ctx *c, double t, int pl, int32 fl, int m, double *s)
{
  char serr[256], star[64] = "Spica";
  return swe_gauquelin_sector_r(c, t, pl < 0 ? 0 : pl, pl < 0 ? star : NULL, fl, m, GEO, 0, 0,
                                s, serr);
}

/* the sector's difference, the shorter way round the 36 */
static double sect_diff(double a, double b)
{
  double d = fabs(a - b);
  return d > 18 ? 36 - d : d;
}

/* (a) methods 0 and 1 with each frame flag F on each direction base D, the
 * call with D alone (with SEFLG_SIDEREAL, D less the Horizons bits, which
 * the sidereal place drops), values and returns bit for bit */
static int check_a(void)
{
  swe_ctx *c = open_ctx();
  int f, e, m, d, b, i, n0 = n, b0 = bad;
  double ww = 0;
  if (c == NULL)
    return -1;
  for (f = 0; f < NFF; f++)
    for (e = 0; e < 3; e++)
      for (m = 0; m < 2; m++) {
        int ndiff = 0, nok = 0;
        double w = 0;
        for (d = 0; d < NDD; d++)
          for (b = 0; b < NPL; b++)
            for (i = 0; i < NTQ; i++) {
              int32 base = DD[d], r0, r1;
              double s0 = 0, s1 = 0;
              if (FF[f] & SEFLG_SIDEREAL)
                base &= ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
              r0 = sector(c, TQ[i], PL[b], EPHF[e] | base, m, &s0);
              r1 = sector(c, TQ[i], PL[b], EPHF[e] | DD[d] | FF[f], m, &s1);
              if (r0 >= 0)
                nok++;
              if (r0 != r1 || memcmp(&s0, &s1, sizeof s0) != 0) {
                ndiff++;
                if (r0 >= 0 && r1 >= 0 && sect_diff(s0, s1) > w)
                  w = sect_diff(s0, s1);
              }
            }
        n++;
        if (w > ww)
          ww = w;
        if (ndiff || nok == 0) {
          bad++;
          printf("    FAIL method %d, %s, %s: %d of %d calls off the call without, up to %.3g"
                 " sector\n", m, EPHN[e], FFN[f], ndiff, NDD * NPL * NTQ, w);
        }
      }
  swe_ctx_free(c);
  printf("  (a) methods 0, 1 with a frame flag as without, bit for bit (up to %.2e sector):"
         " %d checks, %d fail\n", ww, n - n0, bad - b0);
  return 0;
}

/* (b) methods 0 and 1 with each direction flag: swe_house_pos() of the
 * place that flag gives, in the frame of the true obliquity and the
 * sidereal time with the Horizons bits the place's call returned */
static int check_b(void)
{
  swe_ctx *c = open_ctx();
  int d, e, m, b, i, n0 = n, b0 = bad;
  double ww = 0;
  char serr[256], star[64];
  if (c == NULL)
    return -1;
  for (d = 0; d < NDD; d++)
    for (e = 0; e < 3; e++) {
      double w = 0;
      int nok = 0;
      for (m = 0; m < 2; m++)
        for (b = 0; b < NPL; b++)
          for (i = 0; i < NTQ; i++) {
            double tu = TQ[i], te, x[6], xn[6], g, armc;
            int32 fl = EPHF[e] | DD[d], r, hb;
            if (sector(c, tu, PL[b], fl, m, &g) < 0)
              continue;
            te = tu + swe_deltat_ex_r(c, tu, fl, NULL);
            strcpy(star, "Spica");
            r = PL[b] < 0 ? swe_fixstar_r(c, star, te, fl, x, serr)
                          : swe_calc_r(c, te, PL[b], fl, x, serr);
            hb = r & (SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
            if (r < 0 || swe_calc_r(c, te, SE_ECL_NUT, EPHF[e] | hb, xn, serr) < 0) {
              printf("    ERROR place of body %d, %s, %s at JD %.8f\n", PL[b], EPHN[e], DDN[d], tu);
              swe_ctx_free(c);
              return -1;
            }
            if (m == 1)
              x[1] = 0;
            armc = swe_degnorm(swe_sidtime0_r(c, tu, xn[0], xn[2]) * 15 + GEO[0]);
            g = sect_diff(g, swe_house_pos_r(c, armc, GEO[1], xn[0], 'G', x, NULL));
            if (g > w)
              w = g;
            nok++;
          }
      n++;
      if (w > ww)
        ww = w;
      if (!(w <= 1e-11) || nok == 0) {
        bad++;
        printf("    FAIL %s, %s: %.2e sector off the place's own (%d calls)\n", EPHN[e], DDN[d],
               w, nok);
      }
    }
  swe_ctx_free(c);
  printf("  (b) methods 0, 1 with a direction flag in its place's frame, within %.1e sector:"
         " %d checks, %d fail\n", ww, n - n0, bad - b0);
  return 0;
}

/* (c) methods 2 to 5 with each frame flag, the call without, bit for bit */
static int check_c(void)
{
  static const int pl[2] = { SE_MARS, SE_MOON };
  static const double tq[2] = { 2433282.5, 2451545.0 };
  swe_ctx *c = open_ctx();
  int m, f, b, i, n0 = n, b0 = bad;
  if (c == NULL)
    return -1;
  for (m = 2; m < 6; m++) {
    int ndiff = 0, nok = 0;
    for (f = 0; f < NFF; f++)
      for (b = 0; b < 2; b++)
        for (i = 0; i < 2; i++) {
          double s0 = 0, s1 = 0;
          int32 r0 = sector(c, tq[i], pl[b], SEFLG_SWIEPH, m, &s0);
          int32 r1 = sector(c, tq[i], pl[b], SEFLG_SWIEPH | FF[f], m, &s1);
          if (r0 >= 0)
            nok++;
          if (r0 != r1 || memcmp(&s0, &s1, sizeof s0) != 0)
            ndiff++;
        }
    n++;
    if (ndiff || nok == 0) {
      bad++;
      printf("    FAIL method %d: %d of %d calls off the call without\n", m, ndiff, NFF * 4);
    }
  }
  swe_ctx_free(c);
  printf("  (c) methods 2 to 5 with a frame flag as without, bit for bit: %d checks, %d fail\n",
         n - n0, bad - b0);
  return 0;
}

/* swetest's output: a line's name (up to the first comma) and numbers */
#define MAXL 2048
struct out {
  int nl;
  char name[MAXL][24];
  double v[MAXL][3];
  int nv[MAXL];
};
static struct out O0, O1;

/* swetest at Zurich from 1.1.1800 0h UT, three dates 150 years apart
 * (to 2100), with the house system hs ("G1": 'G' by method 1) and args */
#define JD0 2378496.5
#define STEP 54786.375
#define NDATE 3
static int run(struct out *o, const char *bodies, const char *hs, const char *args)
{
  char cmd[1024], line[512], hsy[16] = "";
  FILE *fp;
  if (hs[1] != '\0')
    snprintf(hsy, sizeof hsy, "-hsy%s", hs);
  snprintf(cmd, sizeof cmd,
           "%s -edir%s -b1.1.1800 -n%d -s%.4f -ut0 %s -house8.55,47.37,%c %s %s -fPgj -head -g,",
           SWT, EPH, NDATE, STEP, bodies, hs[0], hsy, args);
  fp = popen(cmd, "r");
  if (fp == NULL)
    return -1;
  o->nl = 0;
  while (fgets(line, sizeof line, fp) != NULL && o->nl < MAXL) {
    char *p = strchr(line, ','), *q;
    int k = 0;
    size_t ln = p ? (size_t) (p - line) : strcspn(line, "\n");
    if (ln >= sizeof o->name[0])
      ln = sizeof o->name[0] - 1;
    while (ln > 0 && line[ln - 1] == ' ')
      ln--;
    memcpy(o->name[o->nl], line, ln);
    o->name[o->nl][ln] = '\0';
    /* the numbers are the fields that are numbers: a star's name has a
     * comma of its own ("Spica,alVir") */
    while (p != NULL && k < 3) {
      double v = strtod(p + 1, &q);
      if (q != p + 1 && strspn(q, " \n") == strcspn(q, ","))
        o->v[o->nl][k++] = v;
      p = strchr(p + 1, ',');
    }
    o->nv[o->nl++] = k;
  }
  if (pclose(fp) != 0 || o->nl == 0) {
    printf("    ERROR %s\n", cmd);
    return -1;
  }
  return 0;
}

/* the largest difference of two runs' numbers, -1 if their lines differ */
static double out_diff(const struct out *a, const struct out *b)
{
  int i, k;
  double w = 0;
  if (a->nl != b->nl)
    return -1;
  for (i = 0; i < a->nl; i++) {
    if (strcmp(a->name[i], b->name[i]) != 0 || a->nv[i] != b->nv[i])
      return -1;
    for (k = 0; k < a->nv[i]; k++) {
      double d = fabs(a->v[i][k] - b->v[i][k]);
      if (d > 180)
        d = 360 - d;
      if (d > w)
        w = d;
    }
  }
  return w;
}

/* (d) swetest: the house positions of bodies, a star, the diff modes and
 * the cusps with each frame option, those without, as printed. Each base
 * runs once per body set and system, its options against it. */
static int check_d(void)
{
  static const char *BOD[4] = { "-p145f -xfSpica", "-p45 -d1", "-p45 -D1", "-p45 -dh1" };
  /* the systems: every one is the same path but for 'W' and 'N' (left
   * sidereal), Sunshine's side call, 'G' by methods 1 and 3 */
  static const char *HS[] = { "P", "W", "G1", "G", "K", "E", "I", "N", "G3", NULL };
#define HS_FEW 3
  static const struct {
    const char *base, *extra;
    int all;
  } CS[] = {
    { "", "-sid1", 1 }, { "", "-sidt01", 0 }, { "", "-sidsp1", 0 }, { "", "-j2000", 1 },
    { "", "-nonut", 1 }, { "", "-icrs", 0 }, { "", "-hel", 1 }, { "", "-bary", 0 },
    { "", "-speed", 0 }, { "", "-speed3", 0 }, { "", "-i2050", 0 }, { "", "-i8194", 0 },
    { "", "-i4098", 0 }, { "", "-i14434", 1 }, { "", "-sid1 -j2000 -nonut -icrs -hel", 0 },
    { "-topo8.55,47.37,400", "-sid1", 0 }, { "-topo8.55,47.37,400", "-j2000 -nonut", 0 },
    { "-topo8.55,47.37,400", "-hel", 0 }, { "-i32770", "-i47106", 0 },
    { "-true", "-sid1", 0 }, { "-true", "-j2000", 0 },
    { "-ejplde200.eph -jplhora", "-j2000", 0 }, { "-ejplde200.eph -jplhora", "-nonut", 0 },
    { "-ejplde200.eph -jplhora", "-icrs", 0 }, { "-ejplde200.eph", "-sid1 -jplhora", 0 },
    { "-emos", "-sid1", 0 }, { "-emos", "-nonut", 0 },
  };
#define NCS ((int) (sizeof CS / sizeof CS[0]))
  double w[NCS][4];
  int nd[NCS][4];
  int k0, k, b, h, n0 = n, b0 = bad;
  double ww = 0;
  memset(w, 0, sizeof w);
  memset(nd, 0, sizeof nd);
  for (k0 = 0; k0 < NCS; k0 = k) {
    int k1;
    for (k1 = k0; k1 < NCS && strcmp(CS[k1].base, CS[k0].base) == 0; k1++)
      ;
    /* the options on the plain call in every body set, on another base
     * the star's and the difference's */
    for (b = 0; b < (*CS[k0].base ? 2 : 4); b++)
      for (h = 0; HS[h] != NULL; h++) {
        int ran = 0;
        for (k = k0; k < k1; k++) {
          char args[256];
          double d;
          if (h >= HS_FEW && !CS[k].all)
            continue;
          /* sidereal 'W' and 'N' are bug 254's, left */
          if (strstr(CS[k].extra, "-sid") != NULL && (HS[h][0] == 'W' || HS[h][0] == 'N'))
            continue;
          if (!ran && run(&O0, BOD[b], HS[h], CS[k0].base) < 0)
            return -1;
          ran = 1;
          snprintf(args, sizeof args, "%s %s", CS[k].base, CS[k].extra);
          if (run(&O1, BOD[b], HS[h], args) < 0)
            return -1;
          d = out_diff(&O0, &O1);
          if (d != 0)
            nd[k][b]++;
          if (d < 0 || d > w[k][b])
            w[k][b] = d < 0 ? 999 : d;
        }
      }
    k = k1;
  }
  for (k = 0; k < NCS; k++)
    for (b = 0; b < (*CS[k].base ? 2 : 4); b++) {
      n++;
      if (w[k][b] > ww)
        ww = w[k][b];
      if (nd[k][b]) {
        bad++;
        printf("    FAIL %s, %s on \"%s\": %d systems off, up to %.4g\n", BOD[b], CS[k].extra,
               CS[k].base, nd[k][b], w[k][b]);
      }
    }
  printf("  (d) swetest's house positions with a frame option as without (up to %.3g):"
         " %d checks, %d fail\n", ww, n - n0, bad - b0);
  return 0;
}

/* the line of the k-th occurrence of name, -1 if none */
static int find_line(const struct out *o, const char *name, int k)
{
  int i;
  for (i = 0; i < o->nl; i++)
    if (strcmp(o->name[i], name) == 0 && k-- == 0)
      return i;
  return -1;
}

/* swetest's house number of body pl (-1 Spica) at the date i: by the
 * library, from its tropical place in the true obliquity's frame and
 * swetest's ARMC; the sector for "G1" with the latitude 0 */
static double hpos_ref(int pl, int i, const char *hs)
{
  double tu = JD0 + i * STEP, te, x[6], xo[6], armc;
  char serr[256], star[64] = "Spica";
  te = tu + swe_deltat_ex(tu, SEFLG_SWIEPH, serr);
  if ((pl < 0 ? swe_fixstar(star, te, SEFLG_SWIEPH, x, serr)
              : swe_calc(te, pl, SEFLG_SWIEPH, x, serr)) < 0
      || swe_calc(te, SE_ECL_NUT, SEFLG_SWIEPH, xo, serr) < 0)
    return -1;
  if (hs[1] == '1')
    x[1] = 0;
  armc = swe_degnorm(swe_sidtime(tu) * 15 + 8.55);
  return swe_house_pos(armc, 47.37, xo[0], hs[0], x, serr);
}

/* (e) swetest's numbers themselves: each body's house number the
 * library's (as printed, 5e-8), the diff modes' the bodies' house
 * positions' difference and midpoint, each cusp at its own number; with
 * no option, -sid1 and -j2000 */
static int check_e(void)
{
  static const char *HS[] = { "P", "K", "G", "R", "C", "E", "G1" };
  static const char *OPT[] = { "", "-sid1", "-j2000" };
  static const char *NM[4] = { "Moon", "Mars", "Jupiter", "Spica" };
  static const int PLE[4] = { SE_MOON, SE_MARS, SE_JUPITER, -1 };
  int o, h, i, b, n0 = n, b0 = bad;
  double ww = 0;
  swe_set_ephe_path(EPH);
  for (o = 0; o < 3; o++)
    for (h = 0; h < 7; h++) {
      double w = 0;
      int bodyfac = HS[h][0] == 'G' ? 10 : 30, nc = HS[h][0] == 'G' ? 36 : 12;
      if (run(&O0, "-p145f -xfSpica", HS[h], OPT[o]) < 0)
        return -1;
      for (i = 0; i < NDATE; i++) {
        for (b = 0; b < 4; b++) {
          int l = find_line(&O0, NM[b], i);
          double r = hpos_ref(PLE[b], i, HS[h]);
          double d = l < 0 || r < 0 ? 999 : fabs(O0.v[l][1] - r);
          if (d > w)
            w = d;
        }
        for (b = 1; b <= nc && HS[h][1] == '\0'; b++) {
          char nm[24];
          int l;
          double d;
          snprintf(nm, sizeof nm, "house %2d", b);
          l = find_line(&O0, nm, i);
          d = l < 0 ? 999 : fabs(swe_difdeg2n(O0.v[l][0], (b - 1) * bodyfac)) / bodyfac;
          if (d > w)
            w = d;
        }
      }
      if (HS[h][1] == '\0') {
        /* the diff modes, Mars less and Moon about Jupiter */
        static const char *DM[2] = { "-p4 -d1", "-p5 -D1" };
        static const char *DN[2] = { "Mar-Moo", "Jup/Moo" };
        static const int P1[2] = { SE_MARS, SE_JUPITER };
        int k;
        for (k = 0; k < 2; k++) {
          if (run(&O1, DM[k], HS[h], OPT[o]) < 0)
            return -1;
          for (i = 0; i < NDATE; i++) {
            int l = find_line(&O1, DN[k], i);
            double h1 = (hpos_ref(P1[k], i, HS[h]) - 1) * bodyfac;
            double h2 = (hpos_ref(SE_MOON, i, HS[h]) - 1) * bodyfac;
            double r = k == 0 ? swe_difdeg2n(h1, h2) : swe_deg_midp(h1, h2);
            double d = l < 0 ? 999 : fabs(swe_difdeg2n(O1.v[l][0], r)) / bodyfac;
            if (d > w)
              w = d;
          }
        }
      }
      n++;
      if (w > ww)
        ww = w;
      /* 'G' cusps 13 to 36 miss swe_house_pos()'s exact-cusp test by its
       * MILLIARCSEC: 2.78e-8 sector */
      if (!(w <= 6e-8)) {
        bad++;
        printf("    FAIL %s, %s: %.3g house off the library's\n", HS[h], *OPT[o] ? OPT[o] : "plain",
               w);
      }
    }
  swe_close();
  printf("  (e) swetest's house numbers the library's, within %.1e house: %d checks, %d fail\n",
         ww, n - n0, bad - b0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 3) {
    fprintf(stderr, "usage: hposflag EPHEDIR SWETEST\n");
    return 2;
  }
  EPH = argv[1];
  SWT = argv[2];
  printf("G167: no flag that names a place's frame or form moves a Gauquelin sector or"
         " swetest's house positions\n");
  r = check_a();
  if (r == 0)
    r = check_b();
  if (r == 0)
    r = check_c();
  if (r == 0)
    r = check_d();
  if (r == 0)
    r = check_e();
  if (r < 0) {
    printf("G167 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G167 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG167 PASS\n", n);
  return 0;
}
