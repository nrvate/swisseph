/* fictltround.c -- G80: a fictitious body's light-time position is read at
 * teval - tau, not at its rounding, at the light time's fixed point, and its
 * elements are exact to theirs.
 *
 * app_pos_etc_plan_osc() solved the light time on the orbit (bug 26, G27)
 * and read the elements at t = teval - dt rounded to a double, up to half
 * an ulp of the date (2.3e-10 day near the present) from the emission
 * instant, as the planets were before bug 63. An Earth-centred set's focus
 * is the Earth at t, which moves 0.0172 AU/day while the observer stays at
 * teval: one ulp is 8e-12 AU, 7e-8 deg on Waldemath at 0.0065 AU, a
 * sawtooth in its apparent place with the period of one ulp of light time
 * (UPSTREAM-BUGS 81). A five-point difference over 2^-7 day made it 1e-6
 * to 5.5e-6 deg/day on Waldemath, 2.7e-7 on Selena; Vulcan, at 19 deg/day,
 * 3.4e-7 heliocentric. The body is now moved along its barycentric
 * velocity at teval by the rounding, (teval - t) - dt.
 *
 * Part 2: an element's linear term n T was rounded as the unreduced angle
 * before the sum was taken to 0..360 -- Vulcan's mean anomaly 707550.7341 T
 * is 1.6e7 deg at -200 and 3.5e7 at -3000 -- so even the true places
 * jittered: Vulcan heliocentric by 3.0e-8 deg at -3000 between instants
 * 2^-9 day apart, Waldemath by 7.7e-9. check_t_terms() now forms the term
 * from the rate in two doubles and an fma product with its revolutions
 * taken out exactly.
 *
 * Part 3: the light-time loop stopped within 1e-12 day of its fixed point
 * with the body read at the last pass's dt (2.3e-11 deg on Waldemath,
 * 1.5e-11 on Selena), now finished by a Newton step; and this fork's
 * swe_calc_orbel() solved it along a straight line, reading the orbit at
 * t - tau only with SEFLG_SPEED (1.6e-5 deg off the fixed point without
 * the flag for a = 0.137 AU, 4.4e-9 with it), now as the fictitious
 * bodies.
 *
 * The check, Swiss files and Moshier, the shipped seorbel.txt, binary-
 * fraction instants. (1) Vulcan heliocentric and geocentric, Selena and
 * Waldemath geocentric, apparent (default flags), thirteen dates 1700..2300.
 * (2) The same four with SEFLG_TRUEPOS, three dates each at -3000 and -200.
 * For each row the longitude and latitude speeds against five-point
 * differences of the positions over 2^-7 and 2^-9 day, and the jitter:
 * fourth differences of the positions at eight instants 2^-9 day apart (a
 * smooth motion gives 1e-12). Limits below (LV_ speed deg/day, LJ_ jitter
 * deg); Waldemath's carry Moshier's Earth, whose own places are 5e-14 AU
 * off a smooth curve (3.4e-10 deg at 0.0065 AU, 1.8e-8 deg/day). Measured
 * and the builds that fail in UPSTREAM-BUGS 81: before part 1 and on the
 * upstream build every one of the 24 rows; with part 1 alone the four
 * Vulcan rows of (1) and all sixteen of (2). (3) The astrometric
 * direction of the four bodies (Swiss, Moshier) and of three
 * swe_calc_orbel() orbits (a = 0.137 and 0.387 AU about the Sun, 0.0068
 * about the Earth; with and without SEFLG_SPEED; Swiss) at the thirteen
 * dates of (1), against the light time's fixed point iterated here:
 * within 5e-12 deg (measured <= 1.5e-12) and 2e-11 (<= 3.6e-12). Before
 * part 3 the Earth-centred rows and all six orbel rows fail (2.3e-11,
 * 1.6e-5 deg); the upstream build fails all fourteen. On Moshier the
 * geocentric rows' Earth difference E(t - tau) - E(t) is a fit's, not two
 * reads' (UPSTREAM-BUGS 96, G95: the library carries the Earth at t back
 * along its motion): Waldemath 2.4e-12 deg, Selena 1.0e-12; before bug 96
 * 6.3e-11 and 9.7e-12, the reads' noise. Moshier's places are
 * heliocentric, and since UPSTREAM-BUGS 109 (G108) the library moves the
 * body read at t - tau by the Sun's barycentric motion over the light time,
 * -tau v_S + tau^2 a_S / 2, Moshier's planets as point masses; the
 * reference does the same (without it the four Moshier rows are 2.5e-6 to
 * 3.0e-6 deg off, the bug-109 fix itself; with it Vulcan heliocentric
 * 3.3e-14, Selena 1.05e-12, Waldemath 2.25e-12). Run as
 *   fictltround EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

#ifndef LV_VH	/* limits, overridable for measuring */
# define LV_VH 1e-9
# define LJ_VH 2e-11
# define LV_VG 5e-9
# define LJ_VG 5e-11
# define LV_S 5e-9
# define LJ_S 2e-9
# define LV_W 5e-8
# define LJ_W 1e-9
# define LV_T 1e-9
# define LJ_T 2e-11
# define LF_FICT 5e-12
# define LF_ORBEL 2e-11
#endif

static int bad = 0;

/* One row: the speeds against five-point differences over 2^-7 and 2^-9
 * day, and the fourth differences of the positions at eight instants 2^-9
 * day apart, at nd dates from jd0 + 37.3 k days, k < nd, every dy years. */
static void row(const char *seph, const char *sz, int ipl, int32 fl,
                double y0, double dy, int nd, double limv, double limj)
{
  char serr[256];
  double wv[2] = {0, 0}, wj[2] = {0, 0};
  int y, k, i, s;
  for (y = 0; y < nd; y++) {
    double jd = floor((2451545.0 + (y0 + y * dy - 2000) * 365.25 + 37.3 * y) * 256) / 256;
    double x[6], d[8][6];
    if (swe_calc(jd, ipl, fl | SEFLG_SPEED, x, serr) < 0) {
      printf("  %-6s %-22s ERROR %s\n", seph, sz, serr);
      bad++;
      return;
    }
    for (s = 0; s < 2; s++) {
      double h = s ? 1.0 / 512 : 1.0 / 128;
      for (k = 0; k < 5; k++)
        swe_calc(jd + (k - 2) * h, ipl, fl, d[k], serr);
      for (i = 0; i < 2; i++) {
        double v[5], e;
        for (k = 0; k < 5; k++)
          v[k] = i ? d[k][1] : d[2][0] + swe_difdeg2n(d[k][0], d[2][0]);
        e = fabs(x[3 + i] - (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h));
        if (e > wv[i])
          wv[i] = e;
      }
    }
    for (k = 0; k < 8; k++)
      swe_calc(jd + k / 512.0, ipl, fl, d[k], serr);
    for (i = 0; i < 2; i++) {
      double v[8];
      for (k = 0; k < 8; k++)
        v[k] = i ? d[k][1] : d[0][0] + swe_difdeg2n(d[k][0], d[0][0]);
      for (k = 0; k + 4 < 8; k++) {
        double e = fabs(v[k] - 4 * v[k + 1] + 6 * v[k + 2] - 4 * v[k + 3] + v[k + 4]);
        if (e > wj[i])
          wj[i] = e;
      }
    }
  }
  printf("  %-6s %-22s speed lon %.2e lat %.2e deg/day, jitter lon %.2e lat %.2e deg\n",
         seph, sz, wv[0], wv[1], wj[0], wj[1]);
  if (wv[0] > limv || wv[1] > limv) {
    printf("    FAIL: speed off its positions' rate by more than %.0e deg/day\n", limv);
    bad++;
  }
  if (wj[0] > limj || wj[1] > limj) {
    printf("    FAIL: position jitters by more than %.0e deg between neighbouring instants\n", limj);
    bad++;
  }
}

/* (3) The light time at its fixed point: the astrometric direction
 * (SEFLG_NOABERR | SEFLG_NOGDEFL, J2000 ICRS XYZ) against B(t - tau) - O(t)
 * with tau iterated here to 1e-18 day, B the body's true barycentric (on
 * Moshier heliocentric) place read at t - tau and moved by that instant's
 * rounding, O the observer's at t. Returns the angle, deg. */
#define AUNIT_ 1.49597870700e11
#define CLIGHT_ 2.99792458e8
static const int32 FR = SEFLG_J2000 | SEFLG_NONUT | SEFLG_ICRS | SEFLG_XYZ;

static double angle(const double *a, const double *b)
{
  double c[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
  return atan2(sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]),
               a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) * 180 / M_PI;
}

/* Moshier, Earth-centred (UPSTREAM-BUGS 96): each read of Moshier's Earth
 * is off a smooth curve by about 3e-15 AU, and the library now carries the
 * Earth at teval back to t - tau along its motion rather than reading it
 * again there. So the reference's Earth difference E(t - tau) - E(t) is not
 * two reads either: it is that of a least-squares polynomial (degree 8)
 * through 257 reads 2^-11 day apart about t, whose difference over a light
 * time of 4e-5 day has 1e-19 AU of the reads' noise; the body is its read
 * at t - tau less the Earth read there. */
#define FM 128
#define FDEG 8
static void earth_fit(double t, int32 cfl, double c[3][FDEG + 1])
{
  double a[FDEG + 1][FDEG + 2], x[6];
  char serr[256];
  int n = FDEG + 1, i, j, k, r, q;
  for (q = 0; q < 3; q++) {
    for (i = 0; i < n; i++)
      for (j = 0; j <= n; j++)
        a[i][j] = 0;
    for (k = -FM; k <= FM; k++) {
      double s = (double) k / FM, p[FDEG + 1];
      swe_calc(t + k / 2048.0, SE_EARTH, cfl | SEFLG_HELCTR, x, serr);
      p[0] = 1;
      for (i = 1; i < n; i++)
        p[i] = p[i - 1] * s;
      for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++)
          a[i][j] += p[i] * p[j];
        a[i][n] += p[i] * x[q];
      }
    }
    for (i = 0; i < n; i++)
      for (r = i + 1; r < n; r++) {
        double f = a[r][i] / a[i][i];
        for (j = i; j <= n; j++)
          a[r][j] -= f * a[i][j];
      }
    for (i = n - 1; i >= 0; i--) {
      double s = a[i][n];
      for (j = i + 1; j < n; j++)
        s -= a[i][j] * c[q][j];
      c[q][i] = s / a[i][i];
    }
  }
}

/* the fitted Earth at t + dx less the one at t (s = dx / (FM 2^-11)) */
static double earth_fit_diff(const double *c, double dx)
{
  double s = dx * 2048.0 / FM, v = 0;
  int i;
  for (i = FDEG; i >= 1; i--)
    v = (v + c[i]) * s;
  return v;
}

/* Moshier's Sun's barycentric velocity and acceleration, from its own
 * heliocentric planets as point masses (UPSTREAM-BUGS 102, 109; G101 has
 * the same): Moshier's places are heliocentric, and over the light time the
 * Sun moves by s v_S + s^2 a_S / 2 (s = t - tau - t) */
static void sun_bary_mosh(double jd, double *v, double *a)
{
  static const int ipl[9] = {SE_MERCURY, SE_VENUS, SE_EARTH, SE_MARS,
                             SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO};
  static const double rm[9] = {6023600, 408523.719, 328900.5, 3098703.59,
                               1047.348644, 3497.9018, 22902.98, 19412.26, 136566000};
  const double k2 = 1.32712440017987e+20 * 86400.0 * 86400.0
                    / (1.49597870700e11 * 1.49597870700e11 * 1.49597870700e11);
  const int32 fc = SEFLG_MOSEPH | FR | SEFLG_TRUEPOS | SEFLG_SPEED;
  char serr[256];
  double x[6], m[6], ms = 1, r;
  int k, i;
  for (i = 0; i < 3; i++)
    v[i] = a[i] = 0;
  for (k = 0; k < 9; k++) {
    swe_calc(jd, ipl[k], fc | SEFLG_HELCTR, x, serr);
    if (ipl[k] == SE_EARTH) {	/* the Earth-Moon barycentre */
      swe_calc(jd, SE_MOON, fc, m, serr);
      for (i = 0; i < 6; i++)
        x[i] += m[i] / (81.30056907419062 + 1);
    }
    r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    ms += 1 / rm[k];
    for (i = 0; i < 3; i++) {
      v[i] += x[i + 3] / rm[k];
      a[i] += k2 * x[i] / (r * r * r) / rm[k];
    }
  }
  for (i = 0; i < 3; i++)
    v[i] = -v[i] / ms;
}

static double fict_fixed_point_off(double t, int ipl, int32 eph, int hel)
{
  char serr[256];
  double O[6] = {0, 0, 0, 0, 0, 0}, B[6], E[6], X[6], d[3] = {0, 0, 0}, tau = 0, old = -1;
  double cf[3][FDEG + 1], vs[3] = {0, 0, 0}, as[3] = {0, 0, 0};
  int32 cfl = eph | FR | SEFLG_TRUEPOS | SEFLG_SPEED;
  int32 bfl = cfl | (eph == SEFLG_MOSEPH ? SEFLG_HELCTR : SEFLG_BARYCTR);
  int it, i, mgeo = eph == SEFLG_MOSEPH && !hel;
  if (eph != SEFLG_MOSEPH)
    swe_calc(t, hel ? SE_SUN : SE_EARTH, cfl | SEFLG_BARYCTR, O, serr);
  else if (!hel)
    swe_calc(t, SE_EARTH, cfl | SEFLG_HELCTR, O, serr);
  if (mgeo)
    earth_fit(t, cfl, cf);
  if (eph == SEFLG_MOSEPH)
    sun_bary_mosh(t, vs, as);
  for (it = 0; it < 30 && fabs(tau - old) > 1e-18; it++) {
    double tp = t - tau, eps;
    old = tau;
    swe_calc(tp, ipl, bfl, B, serr);
    eps = (t - tp) - tau;
    if (mgeo) {
      swe_calc(tp, SE_EARTH, cfl | SEFLG_HELCTR, E, serr);
      for (i = 0; i < 3; i++)
        d[i] = (B[i] - E[i]) + earth_fit_diff(cf[i], tp - t) + eps * B[i + 3];
    } else {
      for (i = 0; i < 3; i++)
        d[i] = B[i] + eps * B[i + 3] - O[i];
    }
    for (i = 0; i < 3; i++)	/* the Sun's motion over the light time */
      d[i] += -tau * vs[i] + tau * tau * as[i] / 2;
    tau = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) * AUNIT_ / CLIGHT_ / 86400;
  }
  swe_calc(t, ipl, eph | FR | SEFLG_NOABERR | SEFLG_NOGDEFL | (hel ? SEFLG_HELCTR : 0), X, serr);
  return angle(X, d);
}

/* The same for swe_calc_orbel(): the two-body orbit at t - tau is the
 * elements with M - n tau, n = sqrt(mu / a^3), the centre read at t - tau.
 * withspeed selects the call compared. */
static double orbel_fixed_point_off(double t, const double *el0, int32 ctr, double mu, int withspeed)
{
  char serr[256];
  double O[6], P[6], C[6], C0[6], X[6], el[6], d[3] = {0, 0, 0}, tau = 0, old = -1;
  double n = sqrt(mu / (el0[1] * el0[1] * el0[1])) * 180 / M_PI;
  int32 cfl = SEFLG_SWIEPH | FR | SEFLG_TRUEPOS | SEFLG_BARYCTR | SEFLG_SPEED;
  int it, i;
  swe_calc(t, SE_EARTH, cfl, O, serr);
  swe_calc(t, ctr, cfl, C0, serr);
  for (it = 0; it < 40 && fabs(tau - old) > 1e-18; it++) {
    double tp = t - tau, eps;
    old = tau;
    for (i = 0; i < 6; i++)
      el[i] = el0[i];
    el[0] -= n * tau;
    swe_calc_orbel(t, el, ctr, SE_ORBEL_EQ_J2000, 0, cfl, P, serr);
    swe_calc(tp, ctr, cfl, C, serr);
    eps = (t - tp) - tau;
    for (i = 0; i < 3; i++)
      d[i] = P[i] - C0[i] + C[i] + eps * C[i + 3] - O[i];
    tau = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) * AUNIT_ / CLIGHT_ / 86400;
  }
  swe_calc_orbel(t, el0, ctr, SE_ORBEL_EQ_J2000, 0,
                 SEFLG_SWIEPH | FR | SEFLG_NOABERR | SEFLG_NOGDEFL | (withspeed ? SEFLG_SPEED : 0), X, serr);
  return angle(X, d);
}

int main(int argc, char **argv)
{
  /* (1) apparent, thirteen dates 1700..2300; limits speed deg/day, jitter
   * deg */
  static const struct { int ipl; int32 fl; const char *sz; double limv, limj; } rgb[] = {
    {SE_FICT_OFFSET + 15, SEFLG_HELCTR, "Vulcan, helio", LV_VH, LJ_VH},
    {SE_FICT_OFFSET + 15, 0, "Vulcan, geo", LV_VG, LJ_VG},
    {SE_FICT_OFFSET + 16, 0, "Selena, geo", LV_S, LJ_S},
    {SE_FICT_OFFSET + 18, 0, "Waldemath, geo", LV_W, LJ_W},
  };
  /* (2) true positions, three dates each at -3000 and -200 */
  static const struct { int ipl; int32 fl; const char *sz; } rgt[] = {
    {SE_FICT_OFFSET + 15, SEFLG_HELCTR, "Vulcan, helio, true"},
    {SE_FICT_OFFSET + 15, 0, "Vulcan, geo, true"},
    {SE_FICT_OFFSET + 16, 0, "Selena, geo, true"},
    {SE_FICT_OFFSET + 18, 0, "Waldemath, geo, true"},
  };
  static const struct { int32 fl; const char *sz; } eph[2] = {
    {SEFLG_SWIEPH, "Swiss"}, {SEFLG_MOSEPH, "Mosh"},
  };
  int ie, ib, iy;

  if (argc != 2) {
    fprintf(stderr, "usage: fictltround EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G80: a fictitious body's position is exact to its rounding at t - tau\n");
  for (ie = 0; ie < 2; ie++)
    for (ib = 0; ib < 4; ib++)
      row(eph[ie].sz, rgb[ib].sz, rgb[ib].ipl, eph[ie].fl | rgb[ib].fl,
          1700, 50, 13, rgb[ib].limv, rgb[ib].limj);
  for (ie = 0; ie < 2; ie++)
    for (ib = 0; ib < 4; ib++)
      for (iy = 0; iy < 2; iy++) {
        char sz[48];
        snprintf(sz, sizeof sz, "%s %d", rgt[ib].sz, iy ? -200 : -3000);
        row(eph[ie].sz, sz, rgt[ib].ipl, eph[ie].fl | rgt[ib].fl | SEFLG_TRUEPOS,
            iy ? -200 : -3000, 0, 3, LV_T, LJ_T);
      }
  /* (3) the light time's fixed point */
  {
    static const struct { int ipl; int hel; const char *sz; } rgf[] = {
      {SE_FICT_OFFSET + 15, 1, "Vulcan, helio"}, {SE_FICT_OFFSET + 15, 0, "Vulcan, geo"},
      {SE_FICT_OFFSET + 16, 0, "Selena, geo"}, {SE_FICT_OFFSET + 18, 0, "Waldemath, geo"},
    };
    static const struct { double el[6]; int32 ctr; double mu; const char *sz; } rgo[] = {
      {{10, 0.13744, 0.019, 30, 47, 7.5}, SE_SUN, 0.01720209895 * 0.01720209895, "orbel, a = 0.137 AU"},
      {{10, 0.387, 0.2056, 29, 48, 7.0}, SE_SUN, 0.01720209895 * 0.01720209895, "orbel, a = 0.387 AU"},
      {{10, 0.00684, 0.1587, 8, 136, 2.5}, SE_EARTH, 0.0000298122353216 * 0.0000298122353216, "orbel, Earth, 0.0068"},
    };
    int y, sp;
    for (ie = 0; ie < 2; ie++)
      for (ib = 0; ib < 4; ib++) {
        double w = 0;
        for (y = 0; y < 13; y++) {
          double t = floor((2451545.0 + (-300 + y * 50) * 365.25 + 37.3 * y) * 256) / 256;
          double e = fict_fixed_point_off(t, rgf[ib].ipl, eph[ie].fl, rgf[ib].hel);
          if (e > w)
            w = e;
        }
        printf("  %-6s %-22s light time off its fixed point by %.2e deg\n", eph[ie].sz, rgf[ib].sz, w);
        if (w > LF_FICT) {
          printf("    FAIL: more than %.0e deg\n", LF_FICT);
          bad++;
        }
      }
    for (ib = 0; ib < 3; ib++)
      for (sp = 0; sp < 2; sp++) {
        double w = 0;
        for (y = 0; y < 13; y++) {
          double t = floor((2451545.0 + (-300 + y * 50) * 365.25 + 37.3 * y) * 256) / 256;
          double e = orbel_fixed_point_off(t, rgo[ib].el, rgo[ib].ctr, rgo[ib].mu, sp);
          if (e > w)
            w = e;
        }
        printf("  Swiss  %-22s %s light time off its fixed point by %.2e deg\n", rgo[ib].sz,
               sp ? "speed" : "plain", w);
        if (w > LF_ORBEL) {
          printf("    FAIL: more than %.0e deg\n", LF_ORBEL);
          bad++;
        }
      }
  }
  printf(bad ? "G80 FAIL: %d\n" : "G80 PASS\n", bad);
  return bad ? 1 : 0;
}
