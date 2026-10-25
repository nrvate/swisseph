/* localsky.c -- G175: a Gauquelin sector is the one seen from geopos and
 * leaves the caller's observer, the Earth has none, and swetest's
 * meridian and zenith distances, azimuths and altitudes are those of each
 * point's own place on the sky of date.
 *
 * The defect (UPSTREAM-BUGS 273): swe_gauquelin_sector() methods 0 and 1
 * with SEFLG_TOPOCTR saw the place from the observer of swe_set_topo(),
 * not from geopos, the sector's: the Moon at UT 2433282.5 (1950), method
 * 0, geopos Zurich, 13.8513 with swe_set_topo() at Zurich, 13.7248 at 70
 * W 30 S, refused without it; methods 2 to 5 observe from geopos.
 * Check (a): methods 0 and 1 with SEFLG_TOPOCTR, the Moon, Mars and Spica
 * at G164's six dates 1800..2100, geopos Zurich (400 m) and 70 W 30 S, on
 * de200, the Swiss files and Moshier: the sector bit for bit the same
 * with the caller's observer unset, at geopos and at the other site, and
 * within 1e-11 sector of swe_house_pos() of the place seen from geopos in
 * SE_ECL_NUT's true frame; one check per method and ephemeris: 6.
 * Check (b): after those calls at Zurich the caller's observer answers as
 * before them and as a fresh context's, the topocentric Moon, Mars and
 * Spica with speeds bit for bit, or the refusal and its message: on the
 * default context unset and at 70 W 30 S (published: a context made after
 * them inherits it), on explicit contexts unset and at 70 W 30 S, each
 * ephemeris, every sector answering: 9 checks.
 *
 * The defect (UPSTREAM-BUGS 274): the Earth answered OK, the sector of
 * swe_calc()'s geocentric zero vector: methods 0 and 1 20.8625 at 1950,
 * Zurich, with SEFLG_HELCTR too, methods 2 to 5 17.6567.
 * Check (c): the Earth in each method, with no flag, SEFLG_HELCTR,
 * BARYCTR, TOPOCTR and SIDEREAL, on each ephemeris, refused with the
 * sector 0 and a message, 6 checks; Spica asked with ipl SE_EARTH the
 * star, methods 0, 2, 4, 3 checks.
 *
 * The defect (UPSTREAM-BUGS 275): swetest's meridian and zenith distances
 * (-fm, -fz) were those of the place printed with SEFLG_EQUATORIAL, so
 * that the frame and centre options moved them (Mars at 1.1.1950 0h UT,
 * Zurich, MD 74.4031 plain, 75.0440 with -j2000, 40.6078 with -hel), the
 * diff modes' those of the RA's difference or midpoint, and the cusps'
 * the printed cusp's in xobl[0]'s frame (-sid1: the Ascendant's ZD
 * 69.0860, RA 171.39 for 192.69), or the last body's without a format
 * letter of the equator; -fz wrote its horizon over the azimuth and
 * altitude (-fzh: the Moon's geocentric altitude); the cusps' azimuths
 * and altitudes were the printed cusp's (-sid1: the Ascendant at 20.914
 * deg), the ARMC's of its RA taken for a longitude; the diff modes'
 * azimuth with -i8192 a difference in radians of degrees.
 * Check (d): the built swetest at Zurich (0 m), 1.1.1800 0h UT and 150 and
 * 300 years on, -fPmzadhik: the Moon, Mars, Jupiter and Spica, Mars and
 * Jupiter less, about and less the heliocentric Moon, and the cusps and
 * axes in P, W, G, K and N ('W' and 'N' not with -sid: other points), with
 * each of -sid1, -sidt01, -sidsp1, -j2000, -nonut, -icrs, -hel, -bary,
 * -speed3, -i's SEFLG_EQUATORIAL, RADIANS, XYZ, and -sid1 -j2000 -nonut
 * -icrs -hel together, and on -topo, -true, de200 with -jplhora and
 * Moshier, as the run without: a body's MD, ZD, altitude, azimuth and
 * apparent altitude (its RA and declination are the frame's asked), a
 * cusp's all seven; one check per option and body set: 68. Check (e):
 * swetest's numbers, plain, -sid1 and -j2000, in P K G W N: each body's MD
 * and ZD the library's from its geocentric place on the true equator of
 * date and swe_sidtime()'s ARMC, its azimuth and altitudes from the
 * topocentric place, at 1013.25 mbar and 15 C; the diff modes' (Mars less
 * and Jupiter about the Moon, in P and G) the bodies' difference and
 * midpoint; each cusp's and axis's RA, declination, MD, ZD from its
 * tropical point and SE_ECL_NUT's true obliquity, the azimuth and
 * altitudes from swe_azalt()'s ecliptic (the ARMC on the equator), the
 * sidereal 'W' and 'N' cusps the sidereal houses' plus the ayanamsa;
 * within 1e-7 (the print's 5e-8): 15 checks.
 *
 * Measured: (a) 0 sector apart, within 1e-11; (b), (c), (d) all as
 * required; (e) within 5.0e-8. Before the fix 88 of the 107 fail: all 6
 * of (a) (0.202 sector apart in method 0, 0.138 in method 1), 3 of (b)
 * (unset, no sector answering), all 6 of (c), 58 of (d) (-sid1 46.56 deg,
 * -j2000 4.57, -nonut 6.5e-3, -icrs 6.4e-6, -hel 110.4, -bary 110.2,
 * -i8194 179, -i4098 177.3; -speed3, -i2050 and -hel on -topo pass) and
 * all 15 of (e) (plain 0.915 deg, the Moon's altitude after -fz, 168 in
 * the diff modes; -sid1 47.1, -j2000 4.57). The upstream build fails 96
 * of the 107: those, and 8 more of (d), -i2050 23.45 deg (its SE_ECL_NUT
 * with SEFLG_EQUATORIAL, the cusps' obliquity) and -speed3 2.1e-5 (its
 * SEFLG_SPEED3 places). Run as
 *   localsky EPHEDIR SWETEST
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
/* the sites: Zurich, and one far from it */
static double SITE[2][3] = { { 8.55, 47.37, 400 }, { -70, -30, 0 } };
/* the bodies; -1 Spica */
static const int PL[] = { SE_MOON, SE_MARS, -1 };
#define NPL ((int) (sizeof PL / sizeof PL[0]))

static void check(int ok, const char *what)
{
  n++;
  if (!ok) {
    bad++;
    printf("    FAIL %s\n", what);
  }
}

/* a context on the files, its observer at site s, none if s < 0 */
static swe_ctx *open_ctx(int s)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    return NULL;
  swe_set_ephe_path_r(c, EPH);
  swe_set_jpl_file_r(c, "de200.eph");
  if (s >= 0)
    swe_set_topo_r(c, SITE[s][0], SITE[s][1], SITE[s][2]);
  return c;
}

/* the library's calls on context c, the default one if c is NULL */
static int32 sector(swe_ctx *c, double t, int pl, int32 fl, int m, double *geo, double *s,
                    char *serr)
{
  char star[64] = "Spica";
  *s = -1;
  if (c == NULL)
    return swe_gauquelin_sector(t, pl < 0 ? 0 : pl, pl < 0 ? star : NULL, fl, m, geo, 0, 0, s,
                                serr);
  return swe_gauquelin_sector_r(c, t, pl < 0 ? 0 : pl, pl < 0 ? star : NULL, fl, m, geo, 0, 0,
                                s, serr);
}

static int32 place(swe_ctx *c, double te, int pl, int32 fl, double *x, char *serr)
{
  char star[64] = "Spica";
  if (c == NULL)
    return pl < 0 ? swe_fixstar(star, te, fl, x, serr) : swe_calc(te, pl, fl, x, serr);
  return pl < 0 ? swe_fixstar_r(c, star, te, fl, x, serr) : swe_calc_r(c, te, pl, fl, x, serr);
}

static double tt_of(swe_ctx *c, double tu, int32 fl)
{
  return tu + (c == NULL ? swe_deltat_ex(tu, fl, NULL) : swe_deltat_ex_r(c, tu, fl, NULL));
}

static double sect_diff(double a, double b)
{
  double d = fabs(a - b);
  return d > 18 ? 36 - d : d;
}

/* (a) methods 0 and 1 with SEFLG_TOPOCTR, at either site as geopos: the
 * same sector bit for bit whether the caller's observer is unset, at
 * geopos or at the other site; within 1e-11 sector of swe_house_pos() of
 * the place seen from geopos */
static int check_a(void)
{
  swe_ctx *c[3];
  int e, m, g, b, i, k;
  double ww = 0;
  char serr[256], what[256];
  for (k = 0; k < 3; k++)
    if ((c[k] = open_ctx(k - 1)) == NULL)
      return -1;
  for (e = 0; e < 3; e++)
    for (m = 0; m < 2; m++) {
      int ndiff = 0, nok = 0;
      double w = 0;
      for (g = 0; g < 2; g++)
        for (b = 0; b < NPL; b++)
          for (i = 0; i < NTQ; i++) {
            int32 fl = EPHF[e] | SEFLG_TOPOCTR, r[3], rx, rn;
            double s[3], x[6], xn[6], te, armc, h;
            swe_ctx *cg = c[g + 1];	/* the one whose observer is geopos */
            for (k = 0; k < 3; k++)
              r[k] = sector(c[k], TQ[i], PL[b], fl, m, SITE[g], &s[k], serr);
            te = tt_of(cg, TQ[i], fl);
            rx = place(cg, te, PL[b], fl, x, serr);
            rn = swe_calc_r(cg, te, SE_ECL_NUT, EPHF[e], xn, serr);
            if (r[0] < 0 || r[1] < 0 || r[2] < 0 || rx < 0 || rn < 0
                || memcmp(&s[0], &s[1], sizeof s[0]) != 0
                || memcmp(&s[0], &s[2], sizeof s[0]) != 0) {
              ndiff++;
              for (k = 0; k < 3; k++)
                if (r[k] >= 0 && r[g + 1] >= 0 && sect_diff(s[k], s[g + 1]) > w)
                  w = sect_diff(s[k], s[g + 1]);
              continue;
            }
            if (m == 1)
              x[1] = 0;
            armc = swe_degnorm(swe_sidtime0_r(cg, TQ[i], xn[0], xn[2]) * 15 + SITE[g][0]);
            h = sect_diff(s[0], swe_house_pos_r(cg, armc, SITE[g][1], xn[0], 'G', x, NULL));
            if (h > 1e-11) {
              ndiff++;
              if (h > w)
                w = h;
            }
            nok++;
          }
      if (w > ww)
        ww = w;
      snprintf(what, sizeof what, "(a) method %d, %s: %d of %d calls off geopos's sector, up to"
               " %.3g", m, EPHN[e], ndiff, 2 * NPL * NTQ, w);
      check(ndiff == 0 && nok > 0, what);
    }
  for (k = 0; k < 3; k++)
    swe_ctx_free(c[k]);
  printf("  (a) methods 0, 1 with SEFLG_TOPOCTR seen from geopos whatever the caller's observer"
         " (up to %.3g sector apart): %d checks\n", ww, 6);
  return 0;
}

/* the topocentric Moon, Mars and Spica with speeds at the dates, the
 * returns and messages: what a caller's observer gives */
struct obs {
  int32 r[NPL][NTQ];
  double x[NPL][NTQ][6];
  char serr[NPL][NTQ][64];
};

static void observe(swe_ctx *c, int32 eph, struct obs *o)
{
  int b, i;
  char serr[256];
  memset(o, 0, sizeof *o);
  for (b = 0; b < NPL; b++)
    for (i = 0; i < NTQ; i++) {
      double te = tt_of(c, TQ[i], eph);
      serr[0] = '\0';
      o->r[b][i] = place(c, te, PL[b], eph | SEFLG_TOPOCTR | SEFLG_SPEED, o->x[b][i], serr);
      if (o->r[b][i] < 0)
        memset(o->x[b][i], 0, sizeof o->x[b][i]);
      else
        serr[0] = '\0';
      strncpy(o->serr[b][i], serr, sizeof o->serr[b][i] - 1);
    }
}

/* methods 0 and 1 with SEFLG_TOPOCTR at the first site, every body and
 * date: calls whose observer is not the caller's; how many answer */
static int sectors_at_site0(swe_ctx *c, int32 eph)
{
  int m, b, i, k = 0;
  double s;
  char serr[256];
  for (m = 0; m < 2; m++)
    for (b = 0; b < NPL; b++)
      for (i = 0; i < NTQ; i++)
        if (sector(c, TQ[i], PL[b], eph | SEFLG_TOPOCTR, m, SITE[0], &s, serr) >= 0)
          k++;
  return k;
}

static int obs_same(const struct obs *a, const struct obs *b)
{
  return memcmp(a, b, sizeof *a) == 0;
}

/* (b) after those calls the caller's observer, unset or at the other
 * site, answers as before them and as a fresh context's: the topocentric
 * places and speeds bit for bit, the refusal and its message. On the
 * default context, unset (before anything sets it), then at the other
 * site, published: a context made after the calls inherits that site;
 * and on explicit contexts, each ephemeris. */
static struct obs O_0, O_1, O_f;

static int check_b_default(int site)
{
  swe_ctx *f;
  if (site < 0)
    swe_set_ephe_path(EPH);
  else
    swe_set_topo(SITE[site][0], SITE[site][1], SITE[site][2]);
  if ((f = open_ctx(site)) == NULL)
    return -1;
  observe(NULL, SEFLG_SWIEPH, &O_0);
  sectors_at_site0(NULL, SEFLG_SWIEPH);
  observe(NULL, SEFLG_SWIEPH, &O_1);
  observe(f, SEFLG_SWIEPH, &O_f);
  swe_ctx_free(f);
  if (site < 0) {
    check(obs_same(&O_0, &O_f) && obs_same(&O_1, &O_f) && O_1.r[0][0] < 0,
          "(b) the default context unset: after the sectors answering, or not as before");
    return 0;
  }
  check(obs_same(&O_0, &O_f) && obs_same(&O_1, &O_f) && O_1.r[0][0] >= 0,
        "(b) the default context at 70 W 30 S: after the sectors off a fresh context's");
  /* a context made now inherits the published site, not the sectors' */
  if ((f = swe_ctx_new()) == NULL)
    return -1;
  observe(f, SEFLG_SWIEPH, &O_1);
  swe_ctx_free(f);
  check(obs_same(&O_1, &O_f), "(b) a context made after them: not the published observer's");
  return 0;
}

static int check_b_ctx(void)
{
  int e, s;
  char what[256];
  for (e = 0; e < 3; e++)
    for (s = -1; s < 2; s += 2) {
      swe_ctx *c = open_ctx(s), *f = open_ctx(s);
      int k;
      if (c == NULL || f == NULL)
        return -1;
      observe(c, EPHF[e], &O_0);
      k = sectors_at_site0(c, EPHF[e]);
      observe(c, EPHF[e], &O_1);
      observe(f, EPHF[e], &O_f);
      snprintf(what, sizeof what, "(b) %s, the caller's observer %s: %d of %d sectors answer,"
               " after them %s a fresh context's", EPHN[e], s < 0 ? "unset" : "at 70 W 30 S", k,
               2 * NPL * NTQ, obs_same(&O_1, &O_f) ? "as" : "off");
      check(obs_same(&O_0, &O_f) && obs_same(&O_1, &O_f) && k == 2 * NPL * NTQ, what);
      swe_ctx_free(c);
      swe_ctx_free(f);
    }
  return 0;
}

/* (c) the Earth, in every method, geocentric, with SEFLG_HELCTR,
 * BARYCTR, TOPOCTR and SIDEREAL, on each ephemeris: refused, the sector
 * 0, with a message; a star with ipl SE_EARTH is the star */
static int check_c(void)
{
  static const int32 FL[5] = { 0, SEFLG_HELCTR, SEFLG_BARYCTR, SEFLG_TOPOCTR, SEFLG_SIDEREAL };
  swe_ctx *c = open_ctx(0);
  int m, e, f, n0 = n;
  char what[256];
  if (c == NULL)
    return -1;
  swe_set_sid_mode_r(c, SE_SIDM_LAHIRI, 0, 0);
  for (m = 0; m < 6; m++) {
    int nans = 0;
    for (e = 0; e < 3; e++)
      for (f = 0; f < 5; f++) {
        char serr[256] = "";
        double s;
        int32 r = sector(c, TQ[1], SE_EARTH, EPHF[e] | FL[f], m, SITE[0], &s, serr);
        if (r >= 0 || s != 0 || serr[0] == '\0')
          nans++;
      }
    snprintf(what, sizeof what, "(c) the Earth, method %d: %d of 15 calls answer or say"
             " nothing", m, nans);
    check(nans == 0, what);
  }
  for (m = 0; m < 6; m += 2) {
    char serr[256], star[64] = "Spica";
    double s0, s1;
    int32 r0 = sector(c, TQ[1], -1, SEFLG_SWIEPH, m, SITE[0], &s0, serr);
    int32 r1 = swe_gauquelin_sector_r(c, TQ[1], SE_EARTH, star, SEFLG_SWIEPH, m, SITE[0], 0, 0,
                                      &s1, serr);
    snprintf(what, sizeof what, "(c) Spica with ipl SE_EARTH, method %d: %d %.10f for %d"
             " %.10f", m, r1, s1, r0, s0);
    check(r0 >= 0 && r1 == r0 && s1 == s0, what);
  }
  swe_ctx_free(c);
  printf("  (c) the Earth refused in every method, a star with its number not: %d checks\n",
         n - n0);
  return 0;
}

/* swetest's output: a line's name and up to NV numbers */
#define MAXL 512
#define NV 8
struct out {
  int nl;
  char name[MAXL][24];
  double v[MAXL][NV];
  int nv[MAXL];
};
static struct out O0, O1;

/* swetest at Zurich (0 m: -house) from 1.1.1800 0h UT, three dates 150
 * years apart, house system hs, format "Pmzadhik": MD, ZD, RA, decl,
 * altitude, azimuth, apparent altitude */
#define JD0 2378496.5
#define STEP 54786.375
#define NDATE 3
static const double LON = 8.55, LAT = 47.37;
static int run(struct out *o, const char *bodies, char hs, const char *args)
{
  char cmd[1024], line[512];
  FILE *fp;
  snprintf(cmd, sizeof cmd,
           "%s -edir%s -b1.1.1800 -n%d -s%.4f -ut0 %s -house%.2f,%.2f,%c %s -fPmzadhik -head -g,",
           SWT, EPH, NDATE, STEP, bodies, LON, LAT, hs, args);
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
    /* a star's name has a comma of its own ("Spica,alVir") */
    while (p != NULL && k < NV) {
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

/* swetest's names of the axes, ascmc[0 .. 7] */
static const char *AXN[8] = { "Ascendant", "MC", "ARMC", "Vertex", "equat. Asc.",
                              "co-Asc. W.Koch", "co-Asc Munkasey", "Polar Asc." };

/* a cusp's or axis's line, not a body's */
static int is_cusp(const char *name)
{
  int k;
  if (strncmp(name, "house", 5) == 0)
    return 1;
  for (k = 0; k < 8; k++)
    if (strcmp(name, AXN[k]) == 0)
      return 1;
  return 0;
}

/* the largest difference of two runs' local-sky numbers, -1 if their
 * lines differ: a body's MD, ZD, altitude, azimuth and apparent altitude
 * (its RA and declination are the frame's asked), a cusp's all seven */
static double out_diff(const struct out *a, const struct out *b)
{
  static const int BODYV[5] = { 0, 1, 4, 5, 6 };
  int i, k;
  double w = 0;
  if (a->nl != b->nl)
    return -1;
  for (i = 0; i < a->nl; i++) {
    int cusp = is_cusp(a->name[i]), nk = cusp ? 7 : 5;
    if (strcmp(a->name[i], b->name[i]) != 0 || a->nv[i] != b->nv[i])
      return -1;
    if (a->nv[i] < 7)
      continue;
    for (k = 0; k < nk; k++) {
      int j = cusp ? k : BODYV[k];
      double d = fabs(a->v[i][j] - b->v[i][j]);
      if (d > 180)
        d = 360 - d;
      if (d > w)
        w = d;
    }
  }
  return w;
}

/* (d) swetest's local sky with each frame or centre option as without:
 * the bodies, Spica, the diff modes and the cusps, on six bases */
static int check_d(void)
{
  static const char *BOD[4] = { "-p145f -xfSpica", "-p45 -d1", "-p45 -D1", "-p45 -dh1" };
  static const char HS[] = "PWGKN";
  static const struct {
    const char *base, *extra;
  } CS[] = {
    { "", "-sid1" }, { "", "-sidt01" }, { "", "-sidsp1" }, { "", "-j2000" }, { "", "-nonut" },
    { "", "-icrs" }, { "", "-hel" }, { "", "-bary" }, { "", "-speed3" }, { "", "-i2050" },
    { "", "-i8194" }, { "", "-i4098" }, { "", "-sid1 -j2000 -nonut -icrs -hel" },
    { "-topo8.55,47.37,400", "-sid1" }, { "-topo8.55,47.37,400", "-j2000 -nonut" },
    { "-topo8.55,47.37,400", "-hel" }, { "-true", "-j2000" },
    { "-ejplde200.eph -jplhora", "-nonut" }, { "-ejplde200.eph", "-sid1 -jplhora" },
    { "-emos", "-sid1" }, { "-emos", "-j2000" },
  };
#define NCS ((int) (sizeof CS / sizeof CS[0]))
  int k0, k, b, h, n0 = n;
  double ww = 0;
  char what[256];
  for (k0 = 0; k0 < NCS; k0 = k) {
    int k1;
    for (k1 = k0; k1 < NCS && strcmp(CS[k1].base, CS[k0].base) == 0; k1++)
      ;
    for (b = 0; b < (*CS[k0].base ? 2 : 4); b++) {
      double w[NCS];
      int nd[NCS];
      memset(w, 0, sizeof w);
      memset(nd, 0, sizeof nd);
      for (h = 0; HS[h] != '\0'; h++) {
        int ran = 0;
        for (k = k0; k < k1; k++) {
          char args[256];
          double d;
          /* the sidereal whole signs and 0 Aries are other points: (e) */
          if (strstr(CS[k].extra, "-sid") != NULL && (HS[h] == 'W' || HS[h] == 'N'))
            continue;
          if (!ran && run(&O0, BOD[b], HS[h], CS[k0].base) < 0)
            return -1;
          ran = 1;
          snprintf(args, sizeof args, "%s %s", CS[k].base, CS[k].extra);
          if (run(&O1, BOD[b], HS[h], args) < 0)
            return -1;
          d = out_diff(&O0, &O1);
          if (d != 0)
            nd[k]++;
          if (d < 0 || d > w[k])
            w[k] = d < 0 ? 999 : d;
        }
      }
      for (k = k0; k < k1; k++) {
        if (w[k] > ww)
          ww = w[k];
        snprintf(what, sizeof what, "(d) %s, %s on \"%s\": %d systems off, up to %.4g", BOD[b],
                 CS[k].extra, CS[k0].base, nd[k], w[k]);
        check(nd[k] == 0, what);
      }
    }
    k = k1;
  }
  printf("  (d) swetest's local sky with a frame or centre option as without (up to %.3g):"
         " %d checks\n", ww, n - n0);
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

/* the library's local sky of an equatorial place xe (true equator of
 * date) at tu: MD, ZD; and, of the place xz for the horizon (equatorial if
 * equ, else ecliptic of date), the altitude, azimuth and apparent
 * altitude at 0 m, swetest's 1013.25 mbar and 15 C */
static void sky_ref(double tu, double *xe, double *xz, int equ, double *v)
{
  double geo[3] = { LON, LAT, 0 }, xa[3], armc = swe_degnorm(swe_sidtime(tu) * 15 + LON);
  v[0] = fabs(swe_difdeg2n(xe[0], armc));
  swe_azalt(tu, SE_EQU2HOR, geo, 1013.25, 15, xe, xa);
  v[1] = 90 - xa[1];
  v[2] = xe[0];
  v[3] = xe[1];
  swe_azalt(tu, equ ? SE_EQU2HOR : SE_ECL2HOR, geo, 1013.25, 15, xz, xa);
  v[4] = xa[1];
  v[5] = xa[0];
  v[6] = xa[2];
}

/* the largest difference of a line's numbers k0 .. k1-1 from v */
static double line_diff(const struct out *o, int l, const double *v, int k0, int k1)
{
  int k;
  double w = 0;
  if (l < 0 || o->nv[l] < k1)
    return 999;
  for (k = k0; k < k1; k++) {
    double d = fabs(o->v[l][k] - v[k]);
    if (d > 180)
      d = 360 - d;
    if (d > w)
      w = d;
  }
  return w;
}

/* a body's library local sky (pl -1 Spica) at the date i: MD, ZD of its
 * geocentric place, the azimuth and altitudes of its topocentric one */
static int body_ref(int pl, int i, double *v)
{
  double tu = JD0 + i * STEP, te, x[6], xt[6];
  char serr[256], star[64] = "Spica";
  int32 fl = SEFLG_SWIEPH | SEFLG_EQUATORIAL;
  te = tu + swe_deltat_ex(tu, SEFLG_SWIEPH, serr);
  if ((pl < 0 ? swe_fixstar(star, te, fl, x, serr) : swe_calc(te, pl, fl, x, serr)) < 0
      || (pl < 0 ? swe_fixstar(star, te, fl | SEFLG_TOPOCTR, xt, serr)
                 : swe_calc(te, pl, fl | SEFLG_TOPOCTR, xt, serr)) < 0)
    return -1;
  sky_ref(tu, x, xt, 1, v);
  return 0;
}

/* (e) swetest's numbers themselves, plain, with -sid1 and with -j2000:
 * each body's MD and ZD the library's from its place, its azimuth and
 * altitudes from its topocentric one, the diff modes' the bodies'
 * difference and midpoint, each cusp's and axis's all seven from the
 * tropical point (the sidereal whole signs' and 0 Aries' cusps the printed
 * one less the ayanamsa), the ARMC's on the equator; within 1e-7 (the
 * print's 5e-8) */
static int check_e(void)
{
  static const char HS[] = "PKGWN";
  static const char *OPT[] = { "", "-sid1", "-j2000" };
  static const char *NM[4] = { "Moon", "Mars", "Jupiter", "Spica" };
  static const int PLE[4] = { SE_MOON, SE_MARS, SE_JUPITER, -1 };
  int o, h, i, b, n0 = n;
  double ww = 0;
  char what[256];
  swe_set_ephe_path(EPH);
  swe_set_topo(LON, LAT, 0);
  swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
  for (o = 0; o < 3; o++)
    for (h = 0; HS[h] != '\0'; h++) {
      double w = 0, v[7];
      int nc = HS[h] == 'G' ? 36 : 12;
      if (run(&O0, "-p145f -xfSpica", HS[h], OPT[o]) < 0)
        return -1;
      for (i = 0; i < NDATE; i++) {
        double tu = JD0 + i * STEP, te, cu[37], am[10], cus[37], ams[10], xo[6], ay = 0;
        int k;
        for (b = 0; b < 4; b++) {
          double d;
          if (body_ref(PLE[b], i, v) < 0)
            return -1;
          d = line_diff(&O0, find_line(&O0, NM[b], i), v, 0, 2);
          if (line_diff(&O0, find_line(&O0, NM[b], i), v, 4, 7) > d)
            d = line_diff(&O0, find_line(&O0, NM[b], i), v, 4, 7);
          if (d > w)
            w = d;
        }
        te = tu + swe_deltat_ex(tu, SEFLG_SWIEPH, NULL);
        swe_houses(tu, LAT, LON, HS[h], cu, am);
        swe_calc(te, SE_ECL_NUT, SEFLG_SWIEPH, xo, NULL);
        if (o == 1 && (HS[h] == 'W' || HS[h] == 'N')) {
          /* the sidereal cusps, in the tropical zodiac */
          swe_houses_ex(tu, SEFLG_SIDEREAL, LAT, LON, HS[h], cus, ams);
          swe_get_ayanamsa_ex(te, SEFLG_SWIEPH | SEFLG_SIDEREAL, &ay, NULL);
          for (k = 1; k <= nc; k++)
            cu[k] = swe_degnorm(cus[k] + ay);
        }
        for (k = 1; k <= nc + 8; k++) {
          char nm[24];
          double xh[3], xe[3], d;
          int ax = k > nc;
          if (ax)
            snprintf(nm, sizeof nm, "%s", AXN[k - nc - 1]);
          else
            snprintf(nm, sizeof nm, "house %2d", k);
          xh[0] = ax ? am[k - nc - 1] : cu[k];
          xh[1] = 0;
          xh[2] = 1;
          if (ax && k - nc - 1 == 2) {
            xe[0] = xh[0];
            xe[1] = 0;
            xe[2] = 1;
          } else {
            swe_cotrans(xh, xe, -xo[0]);
          }
          sky_ref(tu, xe, ax && k - nc - 1 == 2 ? xe : xh, ax && k - nc - 1 == 2, v);
          d = line_diff(&O0, find_line(&O0, nm, i), v, 0, 7);
          if (d > w)
            w = d;
        }
      }
      if (HS[h] == 'P' || HS[h] == 'G') {
        /* the diff modes, Mars less and Jupiter about the Moon */
        static const char *DM[2] = { "-p4 -d1", "-p5 -D1" };
        static const char *DN[2] = { "Mar-Moo", "Jup/Moo" };
        static const int P1[2] = { SE_MARS, SE_JUPITER };
        int k, j;
        for (k = 0; k < 2; k++) {
          if (run(&O1, DM[k], HS[h], OPT[o]) < 0)
            return -1;
          for (i = 0; i < NDATE; i++) {
            double v1[7], v2[7], d;
            if (body_ref(P1[k], i, v1) < 0 || body_ref(SE_MOON, i, v2) < 0)
              return -1;
            for (j = 0; j < 7; j++)
              v[j] = k == 0 ? v1[j] - v2[j] : (v1[j] + v2[j]) / 2;
            v[5] = k == 0 ? swe_difdeg2n(v1[5], v2[5]) : swe_deg_midp(v1[5], v2[5]);
            d = line_diff(&O1, find_line(&O1, DN[k], i), v, 0, 2);
            if (line_diff(&O1, find_line(&O1, DN[k], i), v, 4, 7) > d)
              d = line_diff(&O1, find_line(&O1, DN[k], i), v, 4, 7);
            if (d > w)
              w = d;
          }
        }
      }
      if (w > ww)
        ww = w;
      snprintf(what, sizeof what, "(e) %c, %s: %.3g off the library's", HS[h],
               *OPT[o] ? OPT[o] : "plain", w);
      check(w <= 1e-7, what);
    }
  swe_close();
  printf("  (e) swetest's local sky the library's, within %.1e: %d checks\n", ww, n - n0);
  return 0;
}

int main(int argc, char **argv)
{
  int r;
  if (argc != 3) {
    fprintf(stderr, "usage: localsky EPHEDIR SWETEST\n");
    return 2;
  }
  EPH = argv[1];
  SWT = argv[2];
  printf("G175: a Gauquelin sector is seen from geopos, the Earth has none, and swetest's"
         " local sky is each point's own\n");
  /* the default context unset first, and the explicit contexts made
   * before it publishes an observer, which they would inherit */
  r = check_b_default(-1);
  if (r == 0)
    r = check_a();
  if (r == 0)
    r = check_b_ctx();
  if (r == 0)
    r = check_b_default(1);
  if (r == 0)
    printf("  (b) the caller's observer after them as a fresh context's: 9 checks\n");
  if (r == 0)
    r = check_c();
  if (r == 0)
    r = check_d();
  if (r == 0)
    r = check_e();
  if (r < 0) {
    printf("G175 FAIL: ERROR\n");
    return 1;
  }
  if (bad) {
    printf("G175 FAIL: %d of %d\n", bad, n);
    return 1;
  }
  printf("  %d checks within their limits\nG175 PASS\n", n);
  return 0;
}
