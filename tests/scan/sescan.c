/* sescan.c -- a by-hand audit of the library's numerics: where a result
 * loses precision beyond what its time argument resolves, and where a
 * returned speed is not the rate of the returned position.
 *
 * Both kinds were found one at a time: the Earth rotation angle formed as
 * 1.0027 * (JD - J2000), which a double holds only to 2.3e-10 turn at year
 * -999 (0.3 mas); a three-point speed that was the parabola's slope at
 * t + dt, not t; the Moon's speed without the rate of its light time; the
 * latitude speed in the ecliptic of date without the ecliptic's own
 * rotation (obliquity and nutation-in-obliquity rate, ~1e-5 deg/day on
 * every body). This sweeps a matrix of entry points x flag sets x
 * ephemerides x epochs for more of the same, and ranks what it finds.
 *
 * A. Smoothness. Each output field is evaluated at 64 consecutive
 *    representable JDs from the epoch (jd = nextafter(jd, +inf)), angles
 *    unwrapped as differences from the first, and fitted with a
 *    least-squares line. Over 64 ulps every result is a straight line to
 *    far below a double's resolution, so the worst residual is arithmetic
 *    loss alone. It is reported in natural units (mas, AU, s) and as
 *        S = residual / max(|slope| x ulp(jd), floor),
 *    the slope taken from positions +-2^-6 day away (the 64 samples' own
 *    fit is no measure of it where the loss is large), the floor 4 ulps of
 *    the value or 1e-15 rad (1e-15 x the distance; 1.75e-14 rad/day for a
 *    rate) so a field that hardly moves is not divided by nothing. An
 *    exact function of the exact JD gives S ~ 0. Each rounding of a
 *    time-like intermediate at the JD's own scale -- t + delta t,
 *    t - light time, 1.0027 x (JD - J2000) -- adds up to S ~ 0.5: the old
 *    rotation angle was S = 0.5 at -999 (0.15 mas). S >> 1 is a formula
 *    losing precision beyond its input's resolution. Rates (A') are
 *    ranked separately: there S >> 1 is the signature of a finite-difference
 *    speed, its step rounded to the JD's resolution. Each list comes twice,
 *    by S (a formula loses) and by size (anyone could notice).
 * B. Speed consistency. The returned rate (fields 3..5 under SEFLG_SPEED
 *    or SEFLG_SPEED3, the house speeds of swe_houses_ex2_r) against a
 *    five-point central difference d(h) of the library's own positions
 *    computed WITHOUT the speed flag. Steps are powers of two, 2^-2 ..
 *    2^-12 day, so t +- k h is exact and the input rounds nowhere. By
 *    default a ladder: R_k = d(h_k) + (d(h_k) - d(h_k-1)) / 15 cancels the
 *    h^4 term, and for each field the reference is the R_k that agrees best
 *    with both neighbours, that disagreement u its uncertainty -- large
 *    steps lose to truncation (topocentric diurnal terms, house cusps),
 *    small ones to rounding noise. --h H fixes the step instead:
 *    ref = d(H), u = |d(H) - d(H/2)|. A rate is flagged when
 *    |lib - ref| > 3u and above 1e-10 deg/day (or 1.75e-12 x distance per
 *    day, the same angle). Ranked by the angle-equivalent error.
 * C. As a by-product: the position at the epoch with and without the speed
 *    flag, which must be the same number; differences above 1e-6 mas (a
 *    last bit of another evaluation order) are listed.
 * The top lists keep one row per object (its worst field, flag set and
 * epoch) and come in two groups, the file ephemerides with the
 * ephemeris-free calls, and Moshier, whose series would crowd out the rest.
 * Each scan ends with two pattern tables -- by (api, flags, field) and by
 * (api, ephemeris, object, field) -- that say how widely a finding repeats.
 *
 * The matrix. swe_calc_r: Sun..Pluto, the mean and true node, the mean,
 * osculating and interpolated apogee, the interpolated perigee, Chiron,
 * Ceres (17 and SE_AST_OFFSET + 1) and, helio- and barycentric, the Earth;
 * under 14 flag sets (default, EQUATORIAL, NOABERR, TRUEPOS, NONUT,
 * J2000|NONUT, TOPOCTR at Zurich and at 70N 0E, TOPOCTR|NOABERR, HELCTR,
 * BARYCTR, SIDEREAL Lahiri, XYZ, SPEED3); Swiss files, Moshier, and
 * de200.eph (1600-2169 only). swe_calc_ut_r: six bodies, default and
 * TOPOCTR. swe_calc_pctr_r: Moon from Mars, Venus from Jupiter, Mars from
 * the Moon. swe_houses_ex2_r (with speeds; the positions without them, for
 * B and C, from swe_houses_ex_r): P K R E W at 47N and 70N, tropical and
 * Lahiri. swe_sidtime_r, swe_sidtime0_r (eps 23.4392911, nut 0),
 * swe_deltat_ex_r per ephemeris, swe_get_ayanamsa_ex_r (Lahiri, Fagan,
 * True Citra, J2000; with and without NONUT), swe_fixstar2_r (Aldebaran,
 * Spica; eight flag sets), swe_nod_aps_r (Mars, Moon; mean, osculating,
 * barycentric osculating). Epochs: years -13000, -3000, -999, 1, 1300,
 * 1800, 1900, 2000, 2026.5, 2100, 3000, 12000 (+0.1234567 day, so no
 * argument is accidentally round). The Swiss planet files start at JD
 * -3026605, a few months after -13000: there only the analytic points
 * answer.
 *
 * Every case runs on a fresh context, so its numbers do not depend on what
 * ran before it or on which thread ran it: the report is the same for any
 * -j, and a filtered run reproduces the same rows. A case the library
 * refuses (date outside an ephemeris, file absent, body undefined there),
 * answers from another ephemeris than asked, or returns a zero vector
 * (the heliocentric Sun) is skipped and counted.
 *
 * Build (from tests/scan; or `make` there):
 *   cc -std=c17 -O2 -ffp-contract=off -D_GNU_SOURCE -Wall -Wextra -Werror \
 *      -I../.. sescan.c ../../{sweconfig,swedate,swehouse,swejpl,swemmoon,\
 *      swemplan,sweph,swephlib,swecl,swehel}.c -lm -ldl -lpthread -o sescan
 * Run as
 *   sescan [EPHEDIR] [--quick] [--only API[,API..]] [--body N] [--ephe S|M|J]
 *          [--year Y] [--top K] [--scan ABC] [--h H] [--smin S] [--var FLAGS]
 *          [--obj NAME] [--dump] [--tsv FILE] [-j N]
 * EPHEDIR defaults to ../../ephe (the repo's ephe/, which holds de200.eph);
 * APIs: calc calc_ut calc_pctr houses sidtime sidtime0 deltat ayanamsa
 * fixstar2 nod_aps. --var and --obj select a flag set and an object by the
 * names the report prints; --dump prints one case's 64 residuals and its
 * difference ladder (use it with filters); --tsv writes every row of every
 * scan. The full matrix is ~9900 cases: 5-10 s on 12 cores at -O2 (-j 1:
 * ~21 s). It is an audit, not a gate: it exits 0 whatever it finds.
 */
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "swephexp.h"

#define NSAMP   64      /* scan A: consecutive JDs */
#define MAXF    40      /* fields per probe (houses: 12 + 8, and rates) */
#define NLADDER 11      /* scan B: steps 2^-2 .. 2^-12 day */
#define DEG2RAD 0.017453292519943295
#define RAD2MAS 206264806.24709636

enum { API_CALC, API_CALCUT, API_PCTR, API_HOUSES, API_SIDT, API_SIDT0,
       API_DELTAT, API_AYAN, API_STAR, API_NODAPS, NAPI };
static const char *rgszApi[NAPI] = {"calc", "calc_ut", "calc_pctr", "houses",
  "sidtime", "sidtime0", "deltat", "ayanamsa", "fixstar2", "nod_aps"};

/* field types: positions, then rates */
enum { FT_LON, FT_LAT, FT_DIST, FT_X, FT_HOUR, FT_DAY, FT_VANG, FT_VDIST, FT_VX };
static const char *rgszUnit[] = {"mas", "mas", "AU", "AU", "mas", "s",
                                 "deg/d", "AU/d", "AU/d"};
#define ISRATE(t) ((t) >= FT_VANG)

enum { ST_OK, ST_REFUSED, ST_FALLBACK, ST_DEGEN, NST };
static const char *rgszSt[NST] = {"ok", "refused", "other-ephemeris", "zero-vector"};

static const int32 rgEphFlag[3] = {SEFLG_SWIEPH, SEFLG_MOSEPH, SEFLG_JPLEPH};
static const char rgchEph[4] = "SMJ";

static const double rgSite[3][3] = {{0, 0, 0}, {8.55, 47.37, 400}, {0, 70, 0}};

typedef struct {
  int api, ie;          /* ie: 0 Swiss, 1 Moshier, 2 JPL, -1 none needed */
  int32 ipl, ictr, iflag, meth;
  int site, sidm, hsys;
  const char *star, *szVar;
  char szObj[24];
  int nf, fspd;         /* fields; fspd: probe has rates */
  signed char type[MAXF], spd[MAXF];   /* spd: index of the field's rate */
  const char *name[MAXF];
} Probe;

typedef struct {
  int ip, iy, st, bok;
  double jd0, ulp;
  double v0[MAXF], rn[MAXF];
  double res[MAXF], scl[MAXF], flo[MAXF];          /* A */
  double lib[MAXF], ref[MAXF], unc[MAXF], hb[MAXF]; /* B, by position field */
  double dsp[MAXF];                                /* C, by position field */
} Case;

static Probe *g_rgp;
static int g_np, g_capp;
static Case *g_rgc;
static int g_nc;
static atomic_int g_next, g_done;
static const char *g_ephe = "../../ephe";
static double g_hfix = 0;   /* >0: fixed step for scan B */
static int g_dump;          /* --dump: every case's samples and ladder */
static const char *g_var, *g_obj;   /* --var, --obj filters */
static FILE *g_tsv;                 /* --tsv FILE: every row, for grep and sort */

static const double rgYearAll[] = {-13000, -3000, -999, 1, 1300, 1800, 1900,
                                   2000, 2026.5, 2100, 3000, 12000};
static const double rgYearQuick[] = {-999, 2000, 12000};
static const double *g_rgYear = rgYearAll;
static int g_ny = sizeof rgYearAll / sizeof *rgYearAll;

static double YearJd(double y)
{
  /* off the round numbers, so no argument is accidentally exact */
  return 2451545.0 + (y - 2000) * 365.25 + 0.1234567;
}

static const char *BodyName(int32 ipl)
{
  static const char *rgsz[] = {"Sun", "Moon", "Mercury", "Venus", "Mars",
    "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto", "MeanNode", "TrueNode",
    "MeanApog", "OscuApog", "Earth", "Chiron", "Pholus", "Ceres", "Pallas",
    "Juno", "Vesta", "IntpApog", "IntpPerg"};
  if (ipl >= 0 && ipl < (int32) (sizeof rgsz / sizeof *rgsz))
    return rgsz[ipl];
  return ipl == SE_AST_OFFSET + 1 ? "ast1(Ceres)" : "?";
}

/* ---- probes ---------------------------------------------------------- */

static Probe *NewProbe(int api, int ie)
{
  Probe *p;
  if (g_np == g_capp) {
    g_capp = g_capp ? 2 * g_capp : 256;
    g_rgp = realloc(g_rgp, g_capp * sizeof *g_rgp);
    if (g_rgp == NULL) { perror("sescan"); exit(2); }
  }
  p = &g_rgp[g_np++];
  memset(p, 0, sizeof *p);
  p->api = api;
  p->ie = ie;
  p->sidm = -1;
  p->szVar = "default";
  return p;
}

static void Field(Probe *p, const char *name, int type, int spd)
{
  p->name[p->nf] = name;
  p->type[p->nf] = (signed char) type;
  p->spd[p->nf] = (signed char) spd;
  if (spd >= 0)
    p->fspd = 1;
  p->nf++;
}

/* six fields at base b: position and rate, polar or cartesian */
static void Fields6(Probe *p, int b, int32 iflag, const char **rgszName)
{
  static const char *rgszEcl[6] = {"lon", "lat", "dist", "dlon", "dlat", "ddist"};
  static const char *rgszEqu[6] = {"ra", "dec", "dist", "dra", "ddec", "ddist"};
  static const char *rgszXyz[6] = {"x", "y", "z", "dx", "dy", "dz"};
  const char **n = rgszName ? rgszName : (iflag & SEFLG_XYZ) ? rgszXyz :
                   (iflag & SEFLG_EQUATORIAL) ? rgszEqu : rgszEcl;
  int i;
  if (iflag & SEFLG_XYZ) {
    for (i = 0; i < 3; i++) Field(p, n[i], FT_X, b + 3 + i);
    for (i = 3; i < 6; i++) Field(p, n[i], FT_VX, -1);
  } else {
    Field(p, n[0], FT_LON, b + 3);
    Field(p, n[1], FT_LAT, b + 4);
    Field(p, n[2], FT_DIST, b + 5);
    Field(p, n[3], FT_VANG, -1);
    Field(p, n[4], FT_VANG, -1);
    Field(p, n[5], FT_VDIST, -1);
  }
}

typedef struct { const char *sz; int32 fl; int site, sidm; } Var;
static const Var rgVar[] = {
  {"default", 0, 0, -1},
  {"EQU", SEFLG_EQUATORIAL, 0, -1},
  {"NOABERR", SEFLG_NOABERR, 0, -1},
  {"TRUEPOS", SEFLG_TRUEPOS, 0, -1},
  {"NONUT", SEFLG_NONUT, 0, -1},
  {"J2000|NONUT", SEFLG_J2000 | SEFLG_NONUT, 0, -1},
  {"TOPO", SEFLG_TOPOCTR, 1, -1},
  {"TOPO70N", SEFLG_TOPOCTR, 2, -1},
  {"TOPO|NOABERR", SEFLG_TOPOCTR | SEFLG_NOABERR, 1, -1},
  {"HELCTR", SEFLG_HELCTR, 0, -1},
  {"BARYCTR", SEFLG_BARYCTR, 0, -1},
  {"SID-LAHIRI", SEFLG_SIDEREAL, 0, SE_SIDM_LAHIRI},
  {"XYZ", SEFLG_XYZ, 0, -1},
  {"SPEED3", SEFLG_SPEED3, 0, -1},
};
#define NVAR ((int) (sizeof rgVar / sizeof *rgVar))

static int VarIndex(const char *sz)
{
  int i;
  for (i = 0; i < NVAR; i++)
    if (strcmp(rgVar[i].sz, sz) == 0) return i;
  return 0;
}

static void UseVar(Probe *p, const Var *v)
{
  p->iflag = v->fl;
  p->site = v->site;
  p->sidm = v->sidm;
  p->szVar = v->sz;
}

static int g_onlyApi = -1 & ((1 << NAPI) - 1), g_body = -999, g_ephe1 = -1;

static int WantApi(int api) { return (g_onlyApi >> api) & 1; }
static int WantEph(int ie) { return g_ephe1 < 0 || ie < 0 || ie == g_ephe1; }
static int WantBody(int32 ipl) { return g_body == -999 || ipl == g_body; }

static void BuildProbes(void)
{
  static const int32 rgBody[] = {SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS,
    SE_MARS, SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO,
    SE_MEAN_NODE, SE_TRUE_NODE, SE_MEAN_APOG, SE_OSCU_APOG, SE_INTP_APOG,
    SE_INTP_PERG, SE_CHIRON, SE_CERES, SE_AST_OFFSET + 1, SE_EARTH};
  static const int32 rgUtBody[] = {SE_SUN, SE_MOON, SE_MARS, SE_JUPITER,
                                   SE_MEAN_NODE, SE_TRUE_NODE};
  static const int32 rgPctr[][2] = {{SE_MOON, SE_MARS}, {SE_VENUS, SE_JUPITER},
                                    {SE_MARS, SE_MOON}};
  static const char *rgszPctrVar[] = {"default", "EQU", "NOABERR", "J2000|NONUT", "XYZ"};
  static const char rgHsys[] = "PKREW";
  static const char *rgszHouse[MAXF] = {"c1", "c2", "c3", "c4", "c5", "c6",
    "c7", "c8", "c9", "c10", "c11", "c12", "asc", "mc", "armc", "vertex",
    "equasc", "coasc1", "coasc2", "polasc", "dc1", "dc2", "dc3", "dc4", "dc5",
    "dc6", "dc7", "dc8", "dc9", "dc10", "dc11", "dc12", "dasc", "dmc",
    "darmc", "dvertex", "dequasc", "dcoasc1", "dcoasc2", "dpolasc"};
  static const int rgSidm[] = {SE_SIDM_LAHIRI, SE_SIDM_FAGAN_BRADLEY,
                               SE_SIDM_TRUE_CITRA, SE_SIDM_J2000};
  static const char *rgszSidm[] = {"Lahiri", "Fagan", "TrueCitra", "J2000"};
  static const char *rgszStar[] = {"Aldebaran", "Spica"};
  static const char *rgszStarVar[] = {"default", "EQU", "NOABERR",
    "J2000|NONUT", "TOPO", "HELCTR", "SID-LAHIRI", "XYZ"};
  static const char *rgszNod[4][6] = {
    {"nasc.lon", "nasc.lat", "nasc.dist", "nasc.dlon", "nasc.dlat", "nasc.ddist"},
    {"ndsc.lon", "ndsc.lat", "ndsc.dist", "ndsc.dlon", "ndsc.dlat", "ndsc.ddist"},
    {"peri.lon", "peri.lat", "peri.dist", "peri.dlon", "peri.dlat", "peri.ddist"},
    {"aphe.lon", "aphe.lat", "aphe.dist", "aphe.dlon", "aphe.dlat", "aphe.ddist"}};
  static const int32 rgMeth[] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR};
  static const char *rgszMeth[] = {"mean", "oscu", "oscu-bar"};
  static const char *rgszNodVar[] = {"default", "EQU"};
  int ie, ib, iv, i, j, k;
  Probe *p;

  for (ie = 0; ie < 3; ie++) {
    if (!WantEph(ie)) continue;
    if (WantApi(API_CALC))
      for (ib = 0; ib < (int) (sizeof rgBody / sizeof *rgBody); ib++)
        for (iv = 0; iv < NVAR; iv++) {
          int32 ipl = rgBody[ib];
          if (!WantBody(ipl)) continue;
          /* the geocentric Earth is the origin */
          if (ipl == SE_EARTH && !(rgVar[iv].fl & (SEFLG_HELCTR | SEFLG_BARYCTR)))
            continue;
          p = NewProbe(API_CALC, ie);
          p->ipl = ipl;
          UseVar(p, &rgVar[iv]);
          snprintf(p->szObj, sizeof p->szObj, "%s", BodyName(ipl));
          Fields6(p, 0, p->iflag, NULL);
        }
    if (WantApi(API_CALCUT))
      for (ib = 0; ib < (int) (sizeof rgUtBody / sizeof *rgUtBody); ib++)
        for (iv = 0; iv < 2; iv++) {
          if (!WantBody(rgUtBody[ib])) continue;
          p = NewProbe(API_CALCUT, ie);
          p->ipl = rgUtBody[ib];
          UseVar(p, &rgVar[iv ? VarIndex("TOPO") : 0]);
          snprintf(p->szObj, sizeof p->szObj, "%s", BodyName(p->ipl));
          Fields6(p, 0, p->iflag, NULL);
        }
    if (WantApi(API_PCTR))
      for (ib = 0; ib < 3; ib++)
        for (iv = 0; iv < 5; iv++) {
          if (!WantBody(rgPctr[ib][0])) continue;
          p = NewProbe(API_PCTR, ie);
          p->ipl = rgPctr[ib][0];
          p->ictr = rgPctr[ib][1];
          UseVar(p, &rgVar[VarIndex(rgszPctrVar[iv])]);
          snprintf(p->szObj, sizeof p->szObj, "%s@%s", BodyName(p->ipl),
                   BodyName(p->ictr));
          Fields6(p, 0, p->iflag, NULL);
        }
    if (WantApi(API_DELTAT)) {
      p = NewProbe(API_DELTAT, ie);
      snprintf(p->szObj, sizeof p->szObj, "deltat");
      Field(p, "dt", FT_DAY, -1);
    }
    if (WantApi(API_AYAN))
      for (i = 0; i < 4; i++)
        for (j = 0; j < 2; j++) {
          p = NewProbe(API_AYAN, ie);
          p->sidm = rgSidm[i];
          p->iflag = j ? SEFLG_NONUT : 0;
          p->szVar = j ? "NONUT" : "default";
          snprintf(p->szObj, sizeof p->szObj, "%s", rgszSidm[i]);
          Field(p, "ayan", FT_LON, -1);
        }
    if (WantApi(API_STAR))
      for (i = 0; i < 2; i++)
        for (iv = 0; iv < 8; iv++) {
          p = NewProbe(API_STAR, ie);
          p->star = rgszStar[i];
          UseVar(p, &rgVar[VarIndex(rgszStarVar[iv])]);
          snprintf(p->szObj, sizeof p->szObj, "%s", p->star);
          Fields6(p, 0, p->iflag, NULL);
        }
    if (WantApi(API_NODAPS))
      for (ib = 0; ib < 2; ib++)
        for (k = 0; k < 3; k++)
          for (iv = 0; iv < 2; iv++) {
            int32 ipl = ib ? SE_MOON : SE_MARS;
            if (!WantBody(ipl)) continue;
            p = NewProbe(API_NODAPS, ie);
            p->ipl = ipl;
            p->meth = rgMeth[k];
            UseVar(p, &rgVar[VarIndex(rgszNodVar[iv])]);
            snprintf(p->szObj, sizeof p->szObj, "%s/%s", BodyName(ipl), rgszMeth[k]);
            for (j = 0; j < 4; j++)
              Fields6(p, 6 * j, 0, rgszNod[j]);
          }
  }
  /* no ephemeris of their own (Swiss context, for its delta t) */
  if (WantApi(API_HOUSES) && WantEph(0))
    for (i = 0; i < 5; i++)
      for (j = 1; j <= 2; j++)
        for (k = 0; k < 2; k++) {
          p = NewProbe(API_HOUSES, -1);
          p->hsys = rgHsys[i];
          p->site = j;
          p->iflag = k ? SEFLG_SIDEREAL : 0;
          p->sidm = k ? SE_SIDM_LAHIRI : -1;
          p->szVar = k ? "SID-LAHIRI" : "default";
          snprintf(p->szObj, sizeof p->szObj, "%c@%.0fN", rgHsys[i], rgSite[j][1]);
          for (iv = 0; iv < 20; iv++) Field(p, rgszHouse[iv], FT_LON, 20 + iv);
          for (iv = 20; iv < 40; iv++) Field(p, rgszHouse[iv], FT_VANG, -1);
        }
  if (WantApi(API_SIDT) && WantEph(0)) {
    p = NewProbe(API_SIDT, -1);
    snprintf(p->szObj, sizeof p->szObj, "sidtime");
    Field(p, "gast", FT_HOUR, -1);
  }
  if (WantApi(API_SIDT0) && WantEph(0)) {
    p = NewProbe(API_SIDT0, -1);
    snprintf(p->szObj, sizeof p->szObj, "sidtime0");
    Field(p, "gast", FT_HOUR, -1);
  }
}

/* ---- evaluation ------------------------------------------------------ */

static swe_ctx *NewCtx(const Probe *p)
{
  swe_ctx *ctx = swe_ctx_new();
  if (ctx == NULL) { fprintf(stderr, "sescan: swe_ctx_new failed\n"); exit(2); }
  swe_set_ephe_path_r(ctx, g_ephe);
  if (p->ie == 2)
    swe_set_jpl_file_r(ctx, "de200.eph");
  if (p->sidm >= 0)
    swe_set_sid_mode_r(ctx, p->sidm, 0, 0);
  if (p->site > 0)
    swe_set_topo_r(ctx, rgSite[p->site][0], rgSite[p->site][1], rgSite[p->site][2]);
  return ctx;
}

/* the probe's fields at jd, with its rates when spd */
static int Eval(const Probe *p, swe_ctx *ctx, double jd, int spd, double *v)
{
  char serr[AS_MAXCH], st[2 * SE_MAX_STNAME];
  int32 ef = p->ie >= 0 ? rgEphFlag[p->ie] : SEFLG_SWIEPH, fl = ef | p->iflag, ret;
  double cusp[13], ascmc[10], cs[13], as[10];
  int i;

  serr[0] = '\0';
  if (!spd)
    fl &= ~SEFLG_SPEED3;
  else if (!(fl & SEFLG_SPEED3))
    fl |= SEFLG_SPEED;
  switch (p->api) {
  case API_CALC: case API_CALCUT: case API_PCTR: case API_STAR:
    if (p->api == API_CALC)
      ret = swe_calc_r(ctx, jd, p->ipl, fl, v, serr);
    else if (p->api == API_CALCUT)
      ret = swe_calc_ut_r(ctx, jd, p->ipl, fl, v, serr);
    else if (p->api == API_PCTR)
      ret = swe_calc_pctr_r(ctx, jd, p->ipl, p->ictr, fl, v, serr);
    else {
      snprintf(st, sizeof st, "%s", p->star);
      ret = swe_fixstar2_r(ctx, st, jd, fl, v, serr);
    }
    if (ret < 0) return ST_REFUSED;
    if ((ret & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)) != ef) return ST_FALLBACK;
    if (v[0] == 0 && v[1] == 0 && v[2] == 0) return ST_DEGEN;
    return ST_OK;
  case API_NODAPS:
    ret = swe_nod_aps_r(ctx, jd, p->ipl, fl, p->meth, v, v + 6, v + 12, v + 18, serr);
    if (ret < 0) return ST_REFUSED;
    return ST_OK;
  case API_HOUSES:
    if (spd)
      ret = swe_houses_ex2_r(ctx, jd, p->iflag, rgSite[p->site][1], rgSite[p->site][0],
                             p->hsys, cusp, ascmc, cs, as, serr);
    else
      ret = swe_houses_ex_r(ctx, jd, p->iflag, rgSite[p->site][1], rgSite[p->site][0],
                            p->hsys, cusp, ascmc);
    if (ret < 0) return ST_REFUSED;
    for (i = 0; i < 12; i++) v[i] = cusp[i + 1];
    for (i = 0; i < 8; i++) v[12 + i] = ascmc[i];
    if (spd) {
      for (i = 0; i < 12; i++) v[20 + i] = cs[i + 1];
      for (i = 0; i < 8; i++) v[32 + i] = as[i];
    }
    return ST_OK;
  case API_SIDT:
    v[0] = swe_sidtime_r(ctx, jd);
    return ST_OK;
  case API_SIDT0:
    v[0] = swe_sidtime0_r(ctx, jd, 23.4392911, 0.0);
    return ST_OK;
  case API_DELTAT:
    v[0] = swe_deltat_ex_r(ctx, jd, ef, serr);
    return serr[0] ? ST_FALLBACK : ST_OK;
  case API_AYAN:
    ret = swe_get_ayanamsa_ex_r(ctx, jd, fl & ~(SEFLG_SPEED | SEFLG_SPEED3), v, serr);
    if (ret < 0) return ST_REFUSED;
    if ((ret & (SEFLG_JPLEPH | SEFLG_SWIEPH | SEFLG_MOSEPH)) != ef) return ST_FALLBACK;
    return ST_OK;
  }
  return ST_REFUSED;
}

static double Dif(int t, double a, double b)
{
  double d = a - b, per = t == FT_LON ? 360 : t == FT_HOUR ? 24 : 0;
  if (per != 0) {
    d = fmod(d, per);
    if (d > per / 2) d -= per;
    else if (d <= -per / 2) d += per;
  }
  return d;
}

static double Ulp(double x)
{
  x = fabs(x);
  return nextafter(x, INFINITY) - x;
}

/* 1e-15 rad, 1.75e-14 rad/day, in the field's own units */
static double Floor(int t, double v, double rn)
{
  double f = 4 * Ulp(v), g;
  switch (t) {
  case FT_LON: case FT_LAT: g = 1e-15 / DEG2RAD; break;
  case FT_HOUR: g = 1e-15 / DEG2RAD / 15; break;
  case FT_DIST: case FT_X: g = 1e-15 * rn; break;
  case FT_VANG: g = 1.75e-14 / DEG2RAD; break;
  case FT_VDIST: case FT_VX: g = 1.75e-14 * rn; break;
  default: g = 0;
  }
  return f > g ? f : g;
}

static double Nat(int t, double x)   /* field units -> report units */
{
  switch (t) {
  case FT_LON: case FT_LAT: return x * 3.6e6;
  case FT_HOUR: return x * 5.4e7;
  case FT_DAY: return x * 86400;
  default: return x;
  }
}

static double EqMas(int t, double x, double rn)   /* angle equivalent, mas or mas/d */
{
  switch (t) {
  case FT_DIST: case FT_X: case FT_VDIST: case FT_VX: return rn > 0 ? x / rn * RAD2MAS : 0;
  case FT_DAY: return 0;
  case FT_VANG: return x * 3.6e6;
  default: return Nat(t, x);
  }
}

static double RateRad(int t, double x, double rn)  /* rad/day equivalent */
{
  if (t == FT_VANG) return x * DEG2RAD;
  return rn > 0 ? x / rn : 0;
}

static double FieldScale(const Probe *p, const double *v, int f)
{
  int t = p->type[f], b = (f / 6) * 6;
  if (t == FT_DIST) return fabs(v[f]);
  if (t == FT_VDIST) return fabs(v[f - 3]);
  if (t == FT_X || t == FT_VX) return sqrt(v[b] * v[b] + v[b + 1] * v[b + 1] + v[b + 2] * v[b + 2]);
  return 0;
}

static void RunCase(Case *c)
{
  const Probe *p = &g_rgp[c->ip];
  static _Thread_local double v[NSAMP][MAXF], w[4][MAXF], d[NLADDER][MAXF],
                              rr[NLADDER][MAXF];
  swe_ctx *ctx = NewCtx(p);
  double jd = c->jd0, x[NSAMP], hc = ldexp(1.0, -6);
  int i, f, k, nl, st, fslope;

  c->ulp = Ulp(c->jd0);
  /* A: consecutive JDs, with rates where the probe has them */
  for (i = 0; i < NSAMP; i++) {
    if (i) jd = nextafter(jd, INFINITY);
    x[i] = jd - c->jd0;
    if ((st = Eval(p, ctx, jd, p->fspd, v[i])) != ST_OK) {
      c->st = st;
      goto done;
    }
  }
  /* the slope that S is measured in, from +-hc rather than from the 64
   * samples, whose own scatter would set it where the loss is large */
  fslope = Eval(p, ctx, c->jd0 - hc, p->fspd, w[0]) == ST_OK &&
           Eval(p, ctx, c->jd0 + hc, p->fspd, w[1]) == ST_OK;
  for (f = 0; f < p->nf; f++) {
    int t = p->type[f];
    double xm = 0, ym = 0, sxx = 0, sxy = 0, a, b, r = 0, den;
    c->v0[f] = v[0][f];
    c->rn[f] = FieldScale(p, v[0], f);
    for (i = 0; i < NSAMP; i++) {
      xm += x[i];
      ym += Dif(t, v[i][f], v[0][f]);
    }
    xm /= NSAMP;
    ym /= NSAMP;
    for (i = 0; i < NSAMP; i++) {
      double y = Dif(t, v[i][f], v[0][f]) - ym;
      sxx += (x[i] - xm) * (x[i] - xm);
      sxy += (x[i] - xm) * y;
    }
    b = sxy / sxx;
    a = ym - b * xm;
    for (i = 0; i < NSAMP; i++) {
      double e = fabs(Dif(t, v[i][f], v[0][f]) - a - b * x[i]);
      if (e > r) r = e;
    }
    c->res[f] = r;
    c->scl[f] = fabs(fslope ? Dif(t, w[1][f], w[0][f]) / (2 * hc) : b) * c->ulp;
    c->flo[f] = Floor(t, v[0][f], c->rn[f]);
    den = c->scl[f] > c->flo[f] ? c->scl[f] : c->flo[f];
    if (g_dump) {
      printf("dump A %s %c %s %s %g %s: v0 %.17g slope %.6e/d S %.2f; "
             "residuals / max(slope*ulp, floor):\n ", rgszApi[p->api],
             p->ie >= 0 ? rgchEph[p->ie] : '-', p->szVar, p->szObj,
             g_rgYear[c->iy], p->name[f], v[0][f], c->scl[f] / c->ulp, r / den);
      for (i = 0; i < NSAMP; i++)
        printf(" %+.1f%s", (Dif(t, v[i][f], v[0][f]) - a - b * x[i]) / den,
               i % 16 == 15 ? "\n " : "");
      printf("\n");
    }
  }
  if (!p->fspd)
    goto done;
  /* C: the same epoch without the rate */
  if (Eval(p, ctx, c->jd0, 0, w[0]) == ST_OK)
    for (f = 0; f < p->nf; f++)
      c->dsp[f] = p->spd[f] >= 0 ? fabs(Dif(p->type[f], v[0][f], w[0][f])) : -1;
  else
    for (f = 0; f < p->nf; f++) c->dsp[f] = -1;
  /* B: five-point central differences of positions without the rate */
  nl = g_hfix > 0 ? 2 : NLADDER;
  for (k = 0; k < nl; k++) {
    double h = g_hfix > 0 ? g_hfix / (1 << k) : ldexp(1.0, -2 - k);
    static const int rgm[4] = {-2, -1, 1, 2};
    for (i = 0; i < 4; i++)
      if (Eval(p, ctx, c->jd0 + rgm[i] * h, 0, w[i]) != ST_OK)
        goto done;
    for (f = 0; f < p->nf; f++)
      if (p->spd[f] >= 0) {
        int t = p->type[f];
        d[k][f] = (8 * Dif(t, w[2][f], w[1][f]) + Dif(t, w[0][f], w[3][f])) / (12 * h);
        /* Richardson: the h^4 term of d(h) and d(h/2) cancels */
        if (k) rr[k - 1][f] = d[k][f] + (d[k][f] - d[k - 1][f]) / 15;
      }
  }
  for (f = 0; f < p->nf; f++) {
    int kb = 0;
    double ub = INFINITY;
    if (p->spd[f] < 0) continue;
    if (g_hfix > 0) {
      ub = fabs(d[0][f] - d[1][f]);
      c->ref[f] = d[0][f];
      c->hb[f] = g_hfix;
    } else {
      /* the extrapolation that agrees best with both its neighbours */
      for (k = 0; k + 2 < nl - 1; k++) {
        double u1 = fabs(rr[k][f] - rr[k + 1][f]), u2 = fabs(rr[k + 1][f] - rr[k + 2][f]);
        double u = u1 > u2 ? u1 : u2;
        if (u < ub) { ub = u; kb = k + 1; }
      }
      c->ref[f] = rr[kb][f];
      c->hb[f] = ldexp(1.0, -3 - kb);   /* the finer of its two steps */
    }
    c->unc[f] = ub;
    c->lib[f] = v[0][(int) p->spd[f]];
    if (g_dump) {
      printf("dump B %s %c %s %s %g %s: lib %.12e; differences by step "
             "(and extrapolated from it and the one before):\n",
             rgszApi[p->api], p->ie >= 0 ? rgchEph[p->ie] : '-', p->szVar, p->szObj,
             g_rgYear[c->iy], p->name[(int) p->spd[f]], c->lib[f]);
      for (k = 0; k < nl; k++) {
        printf("   h %-11g %.12e  lib-d %+.3e", g_hfix > 0 ? g_hfix / (1 << k) :
               ldexp(1.0, -2 - k), d[k][f], c->lib[f] - d[k][f]);
        if (k && g_hfix <= 0)
          printf("   R %.12e  lib-R %+.3e%s", rr[k - 1][f], c->lib[f] - rr[k - 1][f],
                 k - 1 == kb ? "  <- ref" : "");
        printf("\n");
      }
      printf("   u %.3e\n", ub);
    }
  }
  c->bok = 1;
done:
  swe_close_r(ctx);
  swe_ctx_free(ctx);
}

static void *Worker(void *arg)
{
  int i;
  (void) arg;
  while ((i = atomic_fetch_add(&g_next, 1)) < g_nc) {
    int n;
    RunCase(&g_rgc[i]);
    n = atomic_fetch_add(&g_done, 1) + 1;
    if (n % (g_nc / 20 + 1) == 0)
      fprintf(stderr, "\r  %d/%d cases", n, g_nc);
  }
  return NULL;
}

/* ---- report ---------------------------------------------------------- */

typedef struct { int ic, f; double key; int flag; } Row;

static int CmpRow(const void *a, const void *b)
{
  const Row *x = a, *y = b;
  if (x->key != y->key) return x->key < y->key ? 1 : -1;
  if (x->ic != y->ic) return x->ic - y->ic;
  return x->f - y->f;
}

static void Label(char *sz, size_t n, const Case *c, int f)
{
  const Probe *p = &g_rgp[c->ip];
  snprintf(sz, n, "%-9s %c %-12s %-16s %7g %s", rgszApi[p->api],
           p->ie >= 0 ? rgchEph[p->ie] : '-', p->szVar, p->szObj,
           g_rgYear[c->iy], p->name[f]);
}

static int g_byObj;   /* GroupKey: by flags (0) or by ephemeris and object (1) */

static void GroupKey(char *sz, size_t n, const Row *r)
{
  const Case *c = &g_rgc[r->ic];
  const Probe *p = &g_rgp[c->ip];
  if (g_byObj)
    snprintf(sz, n, "%s %c %s %s", rgszApi[p->api], p->ie >= 0 ? rgchEph[p->ie] : '-',
             p->szObj, p->name[r->f]);
  else
    snprintf(sz, n, "%s %s %s", rgszApi[p->api], p->szVar, p->name[r->f]);
}

typedef struct { Row r; char k[96]; } KRow;
typedef struct { Row worst; int n, nfl; char k[96]; } Group;

static int CmpKRow(const void *a, const void *b)
{
  const KRow *x = a, *y = b;
  int i = strcmp(x->k, y->k);
  return i ? i : CmpRow(&x->r, &y->r);
}

static int CmpGroup(const void *a, const void *b)
{
  return CmpRow(&((const Group *) a)->worst, &((const Group *) b)->worst);
}

/* by (api, flags, field): how many cases, how many flagged, the worst
 * flagged one; fRate says the key is a rate (B) rather than S (A) */
static void Patterns(const Row *r, int n, int top, const char *szWhat, int fRate)
{
  KRow *kr = malloc((n + 1) * sizeof *kr);
  Group *g = malloc((n + 1) * sizeof *g);
  int i, j, ng = 0;
  char sz[32];
  if (!kr || !g) { perror("sescan"); exit(2); }
  for (i = 0; i < n; i++) {
    kr[i].r = r[i];
    GroupKey(kr[i].k, sizeof kr[i].k, &r[i]);
  }
  qsort(kr, n, sizeof *kr, CmpKRow);
  for (i = 0; i < n; i = j) {
    Group gg;
    memset(&gg, 0, sizeof gg);
    memcpy(gg.k, kr[i].k, sizeof gg.k);
    for (j = i; j < n && strcmp(kr[j].k, kr[i].k) == 0; j++) {
      gg.n++;
      if (kr[j].r.flag && !gg.nfl++)
        gg.worst = kr[j].r;   /* rows are worst first within a key */
    }
    if (gg.nfl) g[ng++] = gg;
  }
  qsort(g, ng, sizeof *g, CmpGroup);
  printf("\n-- %s by pattern (%s): %d patterns with a flagged case --\n", szWhat,
         g_byObj ? "api, ephemeris, object, field; over flags and epochs" :
         "api, flags, field; over ephemerides, objects and epochs", ng);
  printf("  %-40s %11s  %-9s  worst case\n", "pattern", "flagged/all", "worst");
  for (i = 0; i < ng && i < top; i++) {
    const Case *c = &g_rgc[g[i].worst.ic];
    const Probe *p = &g_rgp[c->ip];
    snprintf(sz, sizeof sz, "%d/%d", g[i].nfl, g[i].n);
    if (fRate)
      printf("  %-40s %11s  %9.2e  ", g[i].k, sz, g[i].worst.key);
    else
      printf("  %-40s %11s  S=%7.2f  ", g[i].k, sz, g[i].worst.key);
    if (g_byObj)
      printf("%s %g\n", p->szVar, g_rgYear[c->iy]);
    else
      printf("%c %s %g\n", p->ie >= 0 ? rgchEph[p->ie] : '-', p->szObj, g_rgYear[c->iy]);
  }
  free(kr);
  free(g);
}

/* The first `top` rows (flagged ones only, if fFlag), one per object --
 * (api, ephemeris, object), its worst field, flag set and epoch: one
 * finding repeats under ten flag sets, twelve cusps and a dozen epochs and
 * would fill a list otherwise. ndup[j] counts the rows folded into
 * idx[j]; the pattern tables after each list give the breadth. grp 0
 * keeps the file ephemerides and the ephemeris-free calls, 1 Moshier: its
 * analytic series lose more than anything else and would hide the rest. */
static const char *rgszGrp[2] = {"Swiss, JPL and ephemeris-free", "Moshier"};
static int Distinct(const Row *r, int n, int top, int fFlag, int grp, int *idx, int *ndup)
{
  int i, j, m = 0;
  for (i = 0; i < n; i++) {
    const Case *c = &g_rgc[r[i].ic];
    const Probe *p = &g_rgp[c->ip];
    if (fFlag && !r[i].flag) continue;
    if (grp >= 0 && (p->ie == 1) != grp) continue;
    for (j = 0; j < m; j++) {
      const Case *d = &g_rgc[r[idx[j]].ic];
      const Probe *q = &g_rgp[d->ip];
      if (q->api == p->api && q->ie == p->ie && strcmp(q->szObj, p->szObj) == 0)
        break;
    }
    if (j < m)
      ndup[j]++;
    else if (m < top) {
      idx[m] = i;
      ndup[m++] = 0;
    }
  }
  return m;
}

static void ReportA(int fRate, int top, double smin, int *pnflag, int *pnrow)
{
  Row *r = malloc((size_t) g_nc * MAXF * sizeof *r);
  int *idx = malloc((top + 1) * sizeof *idx), *ndup = malloc((top + 1) * sizeof *ndup);
  int i, f, m, gr, n = 0, nfl = 0, bins[6] = {0};
  static const double edges[5] = {0.5, 1, 2, 5, 20};
  char sz[160];
  if (!r || !idx || !ndup) { perror("sescan"); exit(2); }
  for (i = 0; i < g_nc; i++) {
    const Case *c = &g_rgc[i];
    const Probe *p = &g_rgp[c->ip];
    if (c->st != ST_OK) continue;
    for (f = 0; f < p->nf; f++) {
      double den, s;
      int t = p->type[f], b;
      if (ISRATE(t) != fRate) continue;
      den = c->scl[f] > c->flo[f] ? c->scl[f] : c->flo[f];
      s = c->res[f] / den;
      r[n].ic = i; r[n].f = f; r[n].key = s; r[n].flag = s > smin;
      nfl += r[n].flag;
      for (b = 0; b < 5 && s >= edges[b]; b++) ;
      bins[b]++;
      n++;
    }
  }
  qsort(r, n, sizeof *r, CmpRow);
  printf("\n== A%s. Smoothness of %s over %d consecutive JDs: %d fields, %d with S > %g ==\n",
         fRate ? "'" : "", fRate ? "the rates" : "the positions", NSAMP, n, nfl, smin);
  printf("   S < 0.5: %d   0.5-1: %d   1-2: %d   2-5: %d   5-20: %d   >= 20: %d\n",
         bins[0], bins[1], bins[2], bins[3], bins[4], bins[5]);
  printf("  (one row per object, its worst; +n counts its other rows over fields, flags, epochs)\n");
  printf("  %4s %9s %10s %-5s %10s %10s  %-9s %c %-12s %-16s %7s %-9s %s\n", "rank", "S",
         "residual", "unit", "slope*ulp", fRate ? "mas/d" : "eq.mas", "api", 'E', "flags", "object",
         "year", "field", "+n");
  for (gr = 0; gr < 2; gr++) {
    printf("  -- %s --\n", rgszGrp[gr]);
    m = Distinct(r, n, top, 0, gr, idx, ndup);
    for (i = 0; i < m; i++) {
      const Row *w = &r[idx[i]];
      const Case *c = &g_rgc[w->ic];
      const Probe *p = &g_rgp[c->ip];
      int t = p->type[w->f];
      Label(sz, sizeof sz, c, w->f);
      printf("  %4d %9.2f %10.3e %-5s %10.3e %10.3e  %-60s +%d\n", i + 1, w->key,
             Nat(t, c->res[w->f]), rgszUnit[t], Nat(t, c->scl[w->f]),
             EqMas(t, c->res[w->f], c->rn[w->f]), sz, ndup[i]);
    }
  }
  /* the same flagged rows by size: S says a formula loses, this says
   * whether anyone could notice */
  {
    Row *q = malloc((nfl + 1) * sizeof *q);
    if (!q) { perror("sescan"); exit(2); }
    m = 0;
    for (i = 0; i < n; i++)
      if (r[i].flag) {
        const Case *c = &g_rgc[r[i].ic];
        int t = g_rgp[c->ip].type[r[i].f];
        q[m] = r[i];
        q[m].key = t == FT_DAY ? Nat(t, c->res[r[i].f]) * 1e6 : EqMas(t, c->res[r[i].f], c->rn[r[i].f]);
        m++;
      }
    qsort(q, m, sizeof *q, CmpRow);
    printf("\n  -- the same %d (S > %g) by size, %s equivalent (delta t: us) --\n", m, smin,
           fRate ? "mas/day" : "mas");
    for (gr = 0; gr < 2; gr++) {
      int mm = Distinct(q, m, top, 0, gr, idx, ndup);
      printf("  -- %s --\n", rgszGrp[gr]);
      for (i = 0; i < mm; i++) {
        const Row *w = &q[idx[i]];
        const Case *c = &g_rgc[w->ic];
        Label(sz, sizeof sz, c, w->f);
        printf("  %4d %10.3e  S %9.2f  %-60s +%d\n", i + 1, w->key,
               c->res[w->f] / (c->scl[w->f] > c->flo[w->f] ? c->scl[w->f] : c->flo[w->f]),
               sz, ndup[i]);
      }
    }
    free(q);
  }
  for (g_byObj = 0; g_byObj < 2; g_byObj++)
    Patterns(r, n, top, fRate ? "A'" : "A", 0);
  *pnflag = nfl;
  *pnrow = n;
  free(r);
  free(idx);
  free(ndup);
}

static double g_bfloor = 1e-10;   /* deg/day, and its equivalent */

static void ReportB(int top, int *pnflag, int *pnrow)
{
  Row *r = malloc((size_t) g_nc * MAXF * sizeof *r);
  int *idx = malloc((top + 1) * sizeof *idx), *ndup = malloc((top + 1) * sizeof *ndup);
  int i, f, m, gr, n = 0, nfl = 0;
  char sz[160];
  if (!r || !idx || !ndup) { perror("sescan"); exit(2); }
  for (i = 0; i < g_nc; i++) {
    const Case *c = &g_rgc[i];
    const Probe *p = &g_rgp[c->ip];
    if (c->st != ST_OK || !c->bok) continue;
    for (f = 0; f < p->nf; f++) {
      int ts;
      double rn, e, u;
      if (p->spd[f] < 0) continue;
      ts = p->type[(int) p->spd[f]];
      rn = c->rn[(int) p->spd[f]];
      e = RateRad(ts, fabs(c->lib[f] - c->ref[f]), rn);
      u = RateRad(ts, c->unc[f], rn);
      r[n].ic = i; r[n].f = f; r[n].key = e;
      r[n].flag = e > 3 * u && e > g_bfloor * DEG2RAD;
      nfl += r[n].flag;
      n++;
    }
  }
  qsort(r, n, sizeof *r, CmpRow);
  printf("\n== B. Speed consistency: returned rate vs 5-point difference of positions without it: "
         "%d rates, %d flagged (|lib-ref| > 3u and > %g deg/d) ==\n", n, nfl, g_bfloor);
  {
    int bb[5] = {0};
    for (i = 0; i < n; i++)
      if (r[i].flag) {
        double e = r[i].key / DEG2RAD;   /* deg/day equivalent */
        bb[e > 1e-3 ? 0 : e > 1e-5 ? 1 : e > 1e-7 ? 2 : e > 1e-9 ? 3 : 4]++;
      }
    printf("   flagged by size, deg/day equivalent: > 1e-3: %d   1e-5..1e-3: %d   "
           "1e-7..1e-5: %d   1e-9..1e-7: %d   below: %d\n", bb[0], bb[1], bb[2], bb[3], bb[4]);
  }
  printf("  (ranked by the angle-equivalent error; |lib-ref| in the rate's unit; err/u = "
         "|lib-ref| / u;\n   h the finer step of the reference; one row per object, +n its other flagged rows)\n");
  printf("  %4s %10s %-5s %13s %13s %9s %7s %8s  %-9s %c %-12s %-16s %7s %-9s %s\n", "rank",
         "|lib-ref|", "unit", "lib", "ref", "u", "err/u", "h", "api", 'E', "flags",
         "object", "year", "field", "+n");
  for (gr = 0; gr < 2; gr++) {
    printf("  -- %s --\n", rgszGrp[gr]);
    m = Distinct(r, n, top, 1, gr, idx, ndup);
    for (i = 0; i < m; i++) {
      const Row *w = &r[idx[i]];
      const Case *c = &g_rgc[w->ic];
      const Probe *p = &g_rgp[c->ip];
      int ff = w->f, ts = p->type[(int) p->spd[ff]];
      double e = fabs(c->lib[ff] - c->ref[ff]);
      Label(sz, sizeof sz, c, (int) p->spd[ff]);
      printf("  %4d %10.3e %-5s %13.6e %13.6e %9.2e %7.0f %8.6f  %-60s +%d\n", i + 1, e,
             rgszUnit[ts], c->lib[ff], c->ref[ff], c->unc[ff],
             c->unc[ff] > 0 ? e / c->unc[ff] : 99999., c->hb[ff], sz, ndup[i]);
    }
  }
  /* patterns are named by the rate field */
  for (i = 0; i < n; i++)
    r[i].f = g_rgp[g_rgc[r[i].ic].ip].spd[r[i].f];
  for (g_byObj = 0; g_byObj < 2; g_byObj++)
    Patterns(r, n, top, "B (rad/day equivalent)", 1);
  *pnflag = nfl;
  *pnrow = n;
  free(r);
  free(idx);
  free(ndup);
}

static void ReportC(int top, int *pnflag)
{
  Row *r = malloc((size_t) g_nc * MAXF * sizeof *r);
  int *idx = malloc((top + 1) * sizeof *idx), *ndup = malloc((top + 1) * sizeof *ndup);
  int i, f, m, gr, n = 0;
  char sz[160];
  if (!r || !idx || !ndup) { perror("sescan"); exit(2); }
  for (i = 0; i < g_nc; i++) {
    const Case *c = &g_rgc[i];
    const Probe *p = &g_rgp[c->ip];
    if (c->st != ST_OK || !p->fspd) continue;
    for (f = 0; f < p->nf; f++)
      if (p->spd[f] >= 0 && c->dsp[f] > 0) {
        r[n].ic = i; r[n].f = f;
        r[n].key = EqMas(p->type[f], c->dsp[f], c->rn[f]);
        r[n].flag = 1;
        n++;
      }
  }
  qsort(r, n, sizeof *r, CmpRow);
  {
    int b1 = 0, b2 = 0, b3 = 0;
    for (i = 0; i < n; i++) {
      b1 += r[i].key > 1e-6;
      b2 += r[i].key > 1e-3;
      b3 += r[i].key > 1;
    }
    printf("\n== C. Position at the epoch with vs without the speed flag: %d differ "
           "(> 1e-6 mas: %d, > 0.001 mas: %d, > 1 mas: %d) ==\n", n, b1, b2, b3);
  }
  /* below 1e-6 mas is a last-bit difference of another evaluation order */
  for (i = 0; i < n; i++)
    r[i].flag = r[i].key > 1e-6;
  for (gr = 0; gr < 2; gr++) {
    printf("  -- %s --\n", rgszGrp[gr]);
    m = Distinct(r, n, top, 1, gr, idx, ndup);
    for (i = 0; i < m; i++) {
      const Case *c = &g_rgc[r[idx[i]].ic];
      Label(sz, sizeof sz, c, r[idx[i]].f);
      printf("  %4d %10.3e mas  %-60s +%d\n", i + 1, r[idx[i]].key, sz, ndup[i]);
    }
  }
  for (g_byObj = 0; g_byObj < 2; g_byObj++)
    Patterns(r, n, top, "C (mas equivalent)", 1);
  *pnflag = n;
  free(r);
  free(idx);
  free(ndup);
}

/* every row of every scan, tab-separated, one line each */
static void WriteTsv(void)
{
  int i, f;
  fprintf(g_tsv, "scan\tapi\teph\tflags\tobject\tyear\tfield\tunit\t"
          "S|lib\tresidual|ref\tslope*ulp|u\teq|err\tflag\th\n");
  for (i = 0; i < g_nc; i++) {
    const Case *c = &g_rgc[i];
    const Probe *p = &g_rgp[c->ip];
    char e = p->ie >= 0 ? rgchEph[p->ie] : '-';
    if (c->st != ST_OK) continue;
    for (f = 0; f < p->nf; f++) {
      int t = p->type[f];
      double den = c->scl[f] > c->flo[f] ? c->scl[f] : c->flo[f];
      fprintf(g_tsv, "A%s\t%s\t%c\t%s\t%s\t%g\t%s\t%s\t%.4g\t%.4e\t%.4e\t%.4e\t-\t-\n",
              ISRATE(t) ? "'" : "", rgszApi[p->api], e, p->szVar, p->szObj,
              g_rgYear[c->iy], p->name[f], rgszUnit[t], c->res[f] / den,
              Nat(t, c->res[f]), Nat(t, c->scl[f]), EqMas(t, c->res[f], c->rn[f]));
      if (p->spd[f] >= 0 && c->bok) {
        int fs = p->spd[f], ts = p->type[fs];
        double e1 = fabs(c->lib[f] - c->ref[f]), er = RateRad(ts, e1, c->rn[fs]);
        int fl = er > 3 * RateRad(ts, c->unc[f], c->rn[fs]) && er > g_bfloor * DEG2RAD;
        fprintf(g_tsv, "B\t%s\t%c\t%s\t%s\t%g\t%s\t%s\t%.12e\t%.12e\t%.3e\t%.4e\t%d\t%g\n",
                rgszApi[p->api], e, p->szVar, p->szObj, g_rgYear[c->iy], p->name[fs],
                rgszUnit[ts], c->lib[f], c->ref[f], c->unc[f], e1, fl, c->hb[f]);
      }
      if (p->spd[f] >= 0 && c->dsp[f] > 0)
        fprintf(g_tsv, "C\t%s\t%c\t%s\t%s\t%g\t%s\tmas\t-\t-\t-\t%.4e\t-\t-\n",
                rgszApi[p->api], e, p->szVar, p->szObj, g_rgYear[c->iy], p->name[f],
                EqMas(t, c->dsp[f], c->rn[f]));
    }
  }
}

static int ParseApi(const char *sz)
{
  int m = 0, i;
  char buf[256], *t, *save = NULL;
  snprintf(buf, sizeof buf, "%s", sz);
  for (t = strtok_r(buf, ",", &save); t; t = strtok_r(NULL, ",", &save)) {
    for (i = 0; i < NAPI; i++)
      if (strcmp(t, rgszApi[i]) == 0) break;
    if (i == NAPI) { fprintf(stderr, "sescan: unknown api '%s'\n", t); exit(2); }
    m |= 1 << i;
  }
  return m;
}

int main(int argc, char **argv)
{
  int i, j, nth = (int) sysconf(_SC_NPROCESSORS_ONLN), top = 30;
  int scans = 7, nfa = 0, nra = 0, nfa2 = 0, nra2 = 0, nfb = 0, nrb = 0, nfc = 0;
  int skip[NST] = {0}, skipApi[NAPI][NST], nbpart = 0;
  double smin = 0.5, year1 = -1e9;
  pthread_t *rgth;

  memset(skipApi, 0, sizeof skipApi);
  for (i = 1; i < argc; i++) {
    const char *a = argv[i], *n = i + 1 < argc ? argv[i + 1] : NULL;
    if (strcmp(a, "--quick") == 0) {
      g_rgYear = rgYearQuick;
      g_ny = sizeof rgYearQuick / sizeof *rgYearQuick;
    } else if (strcmp(a, "--only") == 0 && n) { g_onlyApi = ParseApi(n); i++; }
    else if (strcmp(a, "--body") == 0 && n) { g_body = atoi(n); i++; }
    else if (strcmp(a, "--ephe") == 0 && n) {
      const char *q = strchr(rgchEph, n[0]);
      if (!q || !n[0]) { fprintf(stderr, "sescan: --ephe S|M|J\n"); return 2; }
      g_ephe1 = (int) (q - rgchEph);
      i++;
    } else if (strcmp(a, "--year") == 0 && n) { year1 = atof(n); i++; }
    else if (strcmp(a, "--top") == 0 && n) { top = atoi(n); i++; }
    else if (strcmp(a, "--smin") == 0 && n) { smin = atof(n); i++; }
    else if (strcmp(a, "--h") == 0 && n) {
      /* snapped to a power of two, so t +- k h stays exact */
      int e;
      frexp(atof(n), &e);
      g_hfix = ldexp(1.0, e - 1);
      i++;
    } else if (strcmp(a, "--scan") == 0 && n) {
      scans = (strchr(n, 'A') ? 1 : 0) | (strchr(n, 'B') ? 2 : 0) | (strchr(n, 'C') ? 4 : 0);
      i++;
    } else if (strcmp(a, "-j") == 0 && n) { nth = atoi(n); i++; }
    else if (strcmp(a, "--dump") == 0) g_dump = 1;
    else if (strcmp(a, "--var") == 0 && n) { g_var = n; i++; }
    else if (strcmp(a, "--obj") == 0 && n) { g_obj = n; i++; }
    else if (strcmp(a, "--tsv") == 0 && n) {
      if ((g_tsv = fopen(n, "w")) == NULL) { perror(n); return 2; }
      i++;
    }
    else if (a[0] != '-') g_ephe = a;
    else {
      fprintf(stderr, "usage: sescan [EPHEDIR] [--quick] [--only API[,API..]] [--body N]\n"
              "              [--ephe S|M|J] [--year Y] [--top K] [--scan ABC] [--h H]\n"
              "              [--smin S] [--var FLAGS] [--obj NAME] [--dump] [--tsv FILE]\n"
              "              [-j N]\n");
      return 2;
    }
  }
  if (nth < 1 || g_dump) nth = 1;
  if (year1 > -1e9) {
    static double y1;
    y1 = year1;
    g_rgYear = &y1;
    g_ny = 1;
  }
  BuildProbes();
  for (i = j = 0; i < g_np; i++)   /* --var LABEL, --obj NAME */
    if ((!g_var || strcmp(g_rgp[i].szVar, g_var) == 0) &&
        (!g_obj || strcmp(g_rgp[i].szObj, g_obj) == 0))
      g_rgp[j++] = g_rgp[i];
  g_np = j;
  g_rgc = calloc((size_t) g_np * g_ny + 1, sizeof *g_rgc);
  if (!g_rgc) { perror("sescan"); return 2; }
  for (i = 0; i < g_np; i++)
    for (j = 0; j < g_ny; j++) {
      double y = g_rgYear[j];
      /* DE200 spans 1599-2169: no point asking it outside */
      if (g_rgp[i].ie == 2 && (y < 1600 || y > 2169)) continue;
      g_rgc[g_nc].ip = i;
      g_rgc[g_nc].iy = j;
      g_rgc[g_nc].jd0 = YearJd(y);
      g_nc++;
    }
  printf("sescan: smoothness and speed-consistency audit of libswe\n");
  printf("ephe path %s; %d probes x %d epochs = %d cases; B step %s\n", g_ephe, g_np,
         g_ny, g_nc, g_hfix > 0 ? "fixed" : "ladder 2^-2..2^-12 day, Richardson-extrapolated");
  if (g_hfix > 0) printf("B step h = %g day (and h/2)\n", g_hfix);
  fflush(stdout);
  rgth = malloc(nth * sizeof *rgth);
  for (i = 0; i < nth; i++)
    pthread_create(&rgth[i], NULL, Worker, NULL);
  for (i = 0; i < nth; i++)
    pthread_join(rgth[i], NULL);
  fprintf(stderr, "\r  %d/%d cases done\n", g_nc, g_nc);
  free(rgth);
  for (i = 0; i < g_nc; i++) {
    skip[g_rgc[i].st]++;
    skipApi[g_rgp[g_rgc[i].ip].api][g_rgc[i].st]++;
    if (g_rgc[i].st == ST_OK && g_rgp[g_rgc[i].ip].fspd && !g_rgc[i].bok) nbpart++;
  }
  if (scans & 1) {
    ReportA(0, top, smin, &nfa, &nra);
    ReportA(1, top, smin, &nfa2, &nra2);
  }
  if (scans & 2) ReportB(top, &nfb, &nrb);
  if (scans & 4) ReportC(top, &nfc);
  if (g_tsv) {
    WriteTsv();
    fclose(g_tsv);
  }

  printf("\n== Summary ==\n");
  printf("  cases %d: ok %d, refused %d, other-ephemeris %d, zero-vector %d; "
         "B unavailable near a limit %d\n", g_nc, skip[ST_OK], skip[ST_REFUSED],
         skip[ST_FALLBACK], skip[ST_DEGEN], nbpart);
  for (i = 0; i < NAPI; i++) {
    int s = 0;
    for (j = 0; j < NST; j++) s += skipApi[i][j];
    if (!s) continue;
    printf("    %-9s %5d cases:", rgszApi[i], s);
    for (j = 0; j < NST; j++)
      if (skipApi[i][j]) printf(" %s %d", rgszSt[j], skipApi[i][j]);
    printf("\n");
  }
  if (scans & 1) {
    printf("  A  positions: %d of %d fields with S > %g\n", nfa, nra, smin);
    printf("  A' rates:     %d of %d fields with S > %g\n", nfa2, nra2, smin);
  }
  if (scans & 2) printf("  B  rates:     %d of %d flagged\n", nfb, nrb);
  if (scans & 4) printf("  C  positions depending on the speed flag: %d\n", nfc);
  free(g_rgc);
  free(g_rgp);
  return 0;
}
