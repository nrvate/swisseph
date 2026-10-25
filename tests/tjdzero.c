/* tjdzero.c -- G70: tjd = 0 is an instant like any other.
 *
 * The defect (UPSTREAM-BUGS 71): the instant-keyed caches -- the planets'
 * and the Earth's slots (pldat[].teval), swe_calc()'s save area
 * (savedat[].tsave), the nodes' (nddat[].teval), the obliquity
 * (oec.teps), the observer (topd.teval), SunRA()'s memo in swehel.c --
 * were emptied, or a recomputation forced, by setting the instant to 0,
 * and 0 is also JD 0 (-4712). Some readers guarded it with `tjd == 0` or
 * `tjd != 0`; sweph() did not when it forced the EMB to be recomputed by
 * setting the Earth slot's teval to 0: at tjd 0 that slot then claimed to
 * hold JD 0, and whatever Earth it held -- of its own, other, instant --
 * came back as the EMB, and the barycentric Sun built from it was wrong.
 * Measured on ts20 before the fix: after swe_close(), Mars's heliocentric
 * osculating ascending node 356.99952 at tjd 0 against 357.02455 at 1e-9
 * (0.025 deg; its speed 1.2e-4 deg/day), its geocentric osculating
 * perihelion 0.22 deg, its mean one 5.5e-3 deg; NaN for the osculating
 * points without SEFLG_SPEED after a call at J2000; in a context warm from
 * other instants, swe_calc() at tjd 0: the barycentric Sun 15.2 deg, the
 * Sun with SEFLG_TRUEPOS 4.95 deg, Mercury 2.6e-4 deg; and
 * swe_vis_limit_mag() at UT 0 on a fresh context read SunRA()'s empty memo
 * as RA 0 (Venus -6.4973 mag against -6.2006, the Moon -13.93 against
 * -12.46).
 *
 * The check: the same requests at tjd 0 and at tjd 1e-9, in four states --
 * a fresh context, the default context after swe_close() and re-open, the
 * default context warm from calls at J2000 and nearby instants, and that
 * after the invalidating setters (swe_set_astro_models, swe_set_sid_mode,
 * swe_set_topo, swe_set_delta_t_userdef, swe_set_tid_acc). The requests:
 * swe_calc() for nine bodies and ten flag sets (sidereal and not, with and
 * without speed, equatorial, helio- and barycentric, topocentric, true
 * positions, J2000; not Moshier, whose range starts at JD 625000.5),
 * swe_nod_aps() for seven bodies, the mean, osculating and barycentric
 * osculating methods, with and without speed and heliocentric,
 * swe_get_ayanamsa_ex() for four sidereal modes with and without
 * nutation, swe_vis_limit_mag() for Venus and the Moon. Positions must
 * agree within the motion over 1e-9 day (from the speed, or 20 deg/day
 * where there is none) plus 2e-8 deg / 1e-10 AU; speeds within 2e-7
 * deg/day / 1e-9 AU/day; the magnitudes within 1e-5; the returned flags
 * equal, and none an error. Measured: 7676 rows, the largest position
 * residual past the motion 7.6e-11 deg, the largest speed difference
 * 4.4e-8 deg/day (the osculating points' speeds, differences over a short
 * step), 6.4e-7 mag. At every other instant the fix changes nothing: the
 * same requests at four instants (J2000 and 1e-9 among them), in the four
 * states, are bit-identical before and after.
 *
 * The upstream build fails 3342 rows (the nodes' and apsides' of every
 * body by every method, swe_calc() rows for seven of the nine bodies, most
 * in the warm states, the Venus magnitude, and the Moon's, which it
 * refuses); ts20 before the fix failed 4176. Run as
 *   tjdzero EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"
#include "sweph.h"   /* swi_default_ctx() */

#define NVAL 4000

/* one state's answers at one instant: values and their kinds */
enum { K_ANG, K_LON, K_DIST, K_SANG, K_SDIST, K_MAG, K_FLAG };
struct run {
  int n;
  double v[NVAL];
  double spd[NVAL];   /* the speed that goes with a position, or NAN */
  int kind[NVAL];
  char what[NVAL][64];
};

static const char *EPHE;

static void put(struct run *r, int kind, double v, double spd, const char *what)
{
  if (r->n >= NVAL)
    return;
  r->v[r->n] = v;
  r->spd[r->n] = spd;
  r->kind[r->n] = kind;
  snprintf(r->what[r->n], sizeof r->what[0], "%s", what);
  r->n++;
}

/* a 6-vector: lon/lat/dist and their speeds (the speeds only if asked) */
static void put6(struct run *r, const double *x, int speed, const char *what)
{
  char w[80];
  static const char *nm[6] = {"lon", "lat", "dist", "lon'", "lat'", "dist'"};
  int j;
  for (j = 0; j < 3; j++) {
    snprintf(w, sizeof w, "%s %s", what, nm[j]);
    put(r, j == 0 ? K_LON : (j == 1 ? K_ANG : K_DIST), x[j], speed ? x[j + 3] : NAN, w);
  }
  if (speed)
    for (j = 3; j < 6; j++) {
      snprintf(w, sizeof w, "%s %s", what, nm[j]);
      put(r, j == 5 ? K_SDIST : K_SANG, x[j], NAN, w);
    }
}

static void requests(swe_ctx *ctx, double t, struct run *r)
{
  static const int body[9] = {SE_SUN, SE_MOON, SE_MERCURY, SE_MARS, SE_JUPITER, SE_PLUTO,
                              SE_MEAN_NODE, SE_TRUE_NODE, SE_OSCU_APOG};
  static const int32 fset[10] = {
    0, SEFLG_SPEED, SEFLG_SIDEREAL, SEFLG_SIDEREAL | SEFLG_SPEED,
    SEFLG_EQUATORIAL | SEFLG_SPEED, SEFLG_HELCTR | SEFLG_SPEED, SEFLG_BARYCTR | SEFLG_SPEED,
    SEFLG_TOPOCTR | SEFLG_SPEED, SEFLG_TRUEPOS | SEFLG_SPEED, SEFLG_J2000 | SEFLG_NONUT | SEFLG_SPEED,
  };
  static const int npl[7] = {SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS, SE_JUPITER, SE_NEPTUNE, SE_EARTH};
  static const int32 meth[3] = {SE_NODBIT_MEAN, SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR};
  static const int32 nfl[3] = {SEFLG_SPEED, 0, SEFLG_SPEED | SEFLG_HELCTR};
  static const int32 sidm[4] = {SE_SIDM_LAHIRI, SE_SIDM_FAGAN_BRADLEY, SE_SIDM_TRUE_CITRA,
                                SE_SIDM_J2000};
  double x[6], pt[4][6], daya;
  double dgeo[3] = {8.55, 47.37, 400}, datm[4] = {1013.25, 15, 40, 0};
  double dobs[6] = {36, 1, 0, 0, 0, 0}, dret[10];
  char serr[256], w[64], obj[16];
  int b, f, p, m, k, q;
  int32 rf;
  for (b = 0; b < 9; b++)
    for (f = 0; f < 10; f++) {
      int32 fl = SEFLG_SWIEPH | fset[f];
      rf = swe_calc_r(ctx, t, body[b], fl, x, serr);
      snprintf(w, sizeof w, "calc %d %#x", body[b], (unsigned) fl);
      put(r, K_FLAG, rf, NAN, w);
      put6(r, x, (fl & SEFLG_SPEED) != 0, w);
    }
  for (p = 0; p < 7; p++)
    for (m = 0; m < 3; m++)
      for (f = 0; f < 3; f++) {
        int32 fl = SEFLG_SWIEPH | nfl[f];
        rf = swe_nod_aps_r(ctx, t, npl[p], fl, meth[m], pt[0], pt[1], pt[2], pt[3], serr);
        for (k = 0; k < 4; k++) {
          static const char *pn[4] = {"asc", "dsc", "peri", "aph"};
          snprintf(w, sizeof w, "nod_aps %d m%d %#x %s", npl[p], (int) meth[m], (unsigned) fl, pn[k]);
          if (k == 0)
            put(r, K_FLAG, rf, NAN, w);
          put6(r, pt[k], (fl & SEFLG_SPEED) != 0, w);
        }
      }
  for (m = 0; m < 4; m++)
    for (q = 0; q < 2; q++) {
      int32 fl = SEFLG_SWIEPH | (q ? SEFLG_NONUT : 0);
      swe_set_sid_mode_r(ctx, sidm[m], 0, 0);
      rf = swe_get_ayanamsa_ex_r(ctx, t, fl, &daya, serr);
      snprintf(w, sizeof w, "ayanamsa mode %d %#x", (int) sidm[m], (unsigned) fl);
      put(r, K_FLAG, rf, NAN, w);
      put(r, K_LON, daya, NAN, w);
    }
  swe_set_sid_mode_r(ctx, SE_SIDM_LAHIRI, 0, 0);
  for (k = 0; k < 2; k++) {
    strcpy(obj, k ? "moon" : "venus");
    rf = swe_vis_limit_mag_r(ctx, t, dgeo, datm, dobs, obj, 0, dret, serr);
    snprintf(w, sizeof w, "vis_limit_mag %s", obj);
    put(r, K_FLAG, rf, NAN, w);
    put(r, K_MAG, dret[0], NAN, w);
  }
  swe_set_topo_r(ctx, 8.55, 47.37, 400);   /* swe_vis_limit_mag set its own */
}

static void configure(swe_ctx *ctx)
{
  swe_set_ephe_path_r(ctx, EPHE);
  swe_set_topo_r(ctx, 8.55, 47.37, 400);
  swe_set_sid_mode_r(ctx, SE_SIDM_LAHIRI, 0, 0);
}

/* the warm-up: other instants, J2000 among them, and the same bodies */
static void warm(swe_ctx *ctx, double t)
{
  double x[6], a[6], b[6], c[6], d[6];
  char serr[256];
  swe_calc_r(ctx, 2451545.0, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
  swe_nod_aps_r(ctx, 2451545.0, SE_MARS, SEFLG_SWIEPH, SE_NODBIT_OSCU, a, b, c, d, serr);
  swe_calc_r(ctx, t + 0.25, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED | SEFLG_TOPOCTR, x, serr);
  swe_calc_r(ctx, t - 0.01, SE_SUN, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
  swe_calc_r(ctx, t + 5, SE_MOON, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
}

/* one state at one instant */
static void state(int s, double t, struct run *r)
{
  swe_ctx *ctx = swi_default_ctx();
  r->n = 0;
  switch (s) {
  case 0:   /* a fresh context */
    ctx = swe_ctx_new();
    configure(ctx);
    break;
  case 1:   /* the default context after swe_close() and re-open */
    swe_close();
    configure(ctx);
    break;
  case 2:   /* the default context, warm */
    swe_close();
    configure(ctx);
    warm(ctx, t);
    break;
  case 3:   /* warm, then every setter that empties a cache */
    swe_close();
    configure(ctx);
    warm(ctx, t);
    swe_set_astro_models("", 0);
    swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0);
    swe_set_topo(0, 0, 0);
    swe_set_topo(8.55, 47.37, 400);
    swe_set_delta_t_userdef(SE_DELTAT_AUTOMATIC);
    swe_set_tid_acc(SE_TIDAL_AUTOMATIC);
    break;
  }
  requests(ctx, t, r);
  if (s == 0)
    swe_ctx_free(ctx);
}

int main(int argc, char **argv)
{
  static struct run r0, r1;
  static const char *sname[4] = {"fresh context", "after swe_close", "warm", "after setters"};
  int s, i, bad = 0, nrow = 0, nerr = 0;
  double wpos = 0, wspd = 0, wmag = 0;
  if (argc != 2) {
    fprintf(stderr, "usage: tjdzero EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  printf("G70: tjd = 0 is an instant like any other\n");
  for (s = 0; s < 4; s++) {
    int sbad = 0;
    state(s, 0.0, &r0);
    state(s, 1e-9, &r1);
    for (i = 0; i < r0.n; i++) {
      double d = r1.v[i] - r0.v[i], lim, res;
      nrow++;
      switch (r0.kind[i]) {
      case K_FLAG:
        if (r1.v[i] < 0)
          nerr++;
        if (r0.v[i] != r1.v[i]) {
          if (sbad < 12)
            printf("  %s: %s returned %#x at 0, %#x at 1e-9\n", sname[s], r0.what[i],
                   (unsigned) (int32) r0.v[i], (unsigned) (int32) r1.v[i]);
          sbad++;
        }
        continue;
      case K_LON:
        d = swe_difdeg2n(r1.v[i], r0.v[i]);
        /* fall through */
      case K_ANG:
      case K_DIST:
        lim = r0.kind[i] == K_DIST ? 1e-10 : 2e-8;
        res = fabs(d) - (isnan(r1.spd[i]) ? (r0.kind[i] == K_DIST ? 0 : 20e-9)
                                          : fabs(r1.spd[i]) * 1e-9);
        if (r0.kind[i] != K_DIST && res > wpos)
          wpos = res;
        break;
      case K_SANG:
      case K_SDIST:
        lim = r0.kind[i] == K_SDIST ? 1e-9 : 2e-7;
        res = fabs(d);
        if (r0.kind[i] == K_SANG && res > wspd)
          wspd = res;
        break;
      default:   /* K_MAG */
        lim = 1e-5;
        res = fabs(d);
        if (res > wmag)
          wmag = res;
        break;
      }
      if (!(res <= lim)) {
        if (sbad < 12)
          printf("  %s: %s %.10g at 0, %.10g at 1e-9 (%.2e, limit %.0e)\n", sname[s], r0.what[i],
                 r0.v[i], r1.v[i], d, lim);
        sbad++;
      }
    }
    printf("  %-16s %4d rows, %d off\n", sname[s], r0.n, sbad);
    bad += sbad;
  }
  printf("  largest past the motion: %.1e deg; speeds %.1e deg/day; magnitudes %.1e (%d rows)\n",
         wpos, wspd, wmag, nrow);
  /* every request answers: an error at both instants would compare equal */
  if (nerr) {
    printf("  FAIL: %d requests returned an error\n", nerr);
    bad += nerr;
  }
  swe_close();
  printf(bad ? "G70 FAIL: %d\n" : "G70 PASS\n", bad);
  return bad ? 1 : 0;
}
