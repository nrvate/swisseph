/* pctrspeed.c -- G76: swe_calc_pctr()'s speeds are the rates of its
 * positions, its light time is at its fixed point, and its deflection is
 * seen from the centre.
 *
 * Two defects in swe_calc_pctr() (UPSTREAM-BUGS 77). Both bodies were read
 * at t = tjd - dt rounded to a double, up to half an ulp of the date
 * (2.3e-10 day) from the emission instant, so the apparent position jumped
 * by the body's motion over that from one date to the next: a noise that a
 * five-point difference over 2^-9 day turns into 9.8e-8 deg/day on Mercury
 * from Mars, 1.8e-7 on the Moon from Venus, against 4e-10 for true
 * positions (bug 63's, in app_pos_etc_plan()). And the aberration rate
 * added the centre's velocity at tjd less that at t whole: its radial part
 * put the distance rate off by 1.5e-6 AU/day on Mercury from Mars, 5.5e-6
 * on the Moon from Venus, 1.3e-4 on Saturn from Mercury, and as tau
 * a(tjd - tau/2) it put Saturn's longitude speed from Mercury off by
 * 2.1e-6 deg/day. The body is now moved along its velocity by the
 * rounding, and the rate is bug 52's exact term with tau a(tjd).
 *
 * The check, Swiss files: Mercury from Mars, the Moon from Venus, Jupiter
 * from Saturn, the Sun from Jupiter, Venus from the Moon, Saturn from
 * Mercury; default flags, SEFLG_NOABERR, SEFLG_TRUEPOS, SEFLG_J2000 and
 * SEFLG_EQUATORIAL; thirteen dates -3000..+3000, binary-fraction instants:
 * the longitude and latitude speeds within 5e-9 deg/day and the distance
 * rate within 5e-10 AU/day of five-point differences of the positions over
 * 2^-6, 2^-7, 2^-8 and 2^-9 day. Measured <= 5.9e-10 deg/day (true
 * positions 4.0e-10) and <= 3.4e-11 AU/day. The build before the fix fails
 * 22 of the 30 rows (the true positions pass, and the Sun from Jupiter
 * and Saturn from Mercury without aberration), by up to 2.1e-6
 * deg/day and 1.3e-4 AU/day; the upstream build fails all 30, by up to
 * 2.3e-6 deg/day in longitude, 1.9e-5 in declination, 1.3e-4 AU/day.
 *
 * Three more defects there. The deflection was swi_deflect_light()'s,
 * seen from the save area's Earth at t, not the centre; and the light time
 * was solved along a straight line, short of its fixed point. Now the
 * deflection is seen from the centre (deflect_light_from()) and the light
 * time takes one Newton step. Checked, 400 dates from 2000: the Sun from
 * Jupiter not deflected at all (0.33" before), Saturn from Mercury within
 * 1e-5" of the deflection written out here from Mercury's and the Sun's
 * places (6.4e-7"; 0.435" before); Sun..Pluto from the Earth at thirteen
 * dates within 1e-5" and 5e-9 deg/day of swe_calc() (8.7e-7", which is
 * swe_calc()'s own light time until bug 78, and 4.2e-10; 5.2e-3" and
 * 1.2e-6 before); and six pairs, Mercury from Pluto among them, thirteen
 * dates, within 1e-11 deg of the light time's fixed point iterated here
 * on true positions (6.0e-14; 5.0e-9 before). The build before entry 77
 * fails 25 checks, the build before its deflection and light-time fixes
 * 3, the upstream build all 33.
 * Run as
 *   pctrspeed EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include "swephexp.h"

/* the Sun's deflection of u (observer to body, ICRS, undeflected) seen
 * from xo at tjd, the Sun at tjd xs0 and at the emission instant xs1, all
 * barycentric: Explanatory Supplement (1992) 3.326, written out here */
static void defl_ref(const double *u, const double *xo, const double *xs0,
                     const double *xs1, double *out)
{
  const double mu = 2.0 * 1.32712440017987e20 / (299792458.0 * 299792458.0)
                    / 149597870700.0;   /* 2 GM_sun / c^2, AU */
  double e[3], q[3], uh[3], ru = 0, re = 0, rq = 0, uq = 0, ue = 0, qe = 0;
  int i;
  for (i = 0; i < 3; i++) {
    e[i] = xo[i] - xs0[i];
    q[i] = u[i] + xo[i] - xs1[i];
    ru += u[i] * u[i];
    re += e[i] * e[i];
    rq += q[i] * q[i];
  }
  ru = sqrt(ru);
  re = sqrt(re);
  rq = sqrt(rq);
  for (i = 0; i < 3; i++) {
    uh[i] = u[i] / ru;
    e[i] /= re;
    q[i] /= rq;
  }
  for (i = 0; i < 3; i++) {
    uq += uh[i] * q[i];
    ue += uh[i] * e[i];
    qe += q[i] * e[i];
  }
  for (i = 0; i < 3; i++)
    out[i] = ru * (uh[i] + mu / re * (uq * e[i] - ue * q[i]) / (1 + qe));
}

/* the angle between two vectors, arcsec */
static double angle(const double *a, const double *b)
{
  double c[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
                 a[0] * b[1] - a[1] * b[0]};
  return atan2(sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]),
               a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) * 180 / M_PI * 3600;
}

int main(int argc, char **argv)
{
  static const struct { int ipl, ictr; const char *sz; } rgp[] = {
    {SE_MERCURY, SE_MARS, "Mer/Mars"}, {SE_MOON, SE_VENUS, "Moon/Ven"},
    {SE_JUPITER, SE_SATURN, "Jup/Sat"}, {SE_SUN, SE_JUPITER, "Sun/Jup"},
    {SE_VENUS, SE_MOON, "Ven/Moon"}, {SE_SATURN, SE_MERCURY, "Sat/Mer"},
  };
  static const struct { int32 fl; const char *sz; } rgf[] = {
    {0, "default"}, {SEFLG_NOABERR, "noaberr"}, {SEFLG_TRUEPOS, "truepos"},
    {SEFLG_J2000, "j2000"}, {SEFLG_EQUATORIAL, "equat"},
  };
  char serr[256];
  int ip, ic, y, s, k, i, bad = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: pctrspeed EPHEDIR\n");
    return 2;
  }
  swe_set_ephe_path(argv[1]);
  printf("G76: swe_calc_pctr() speeds against its positions' rates\n");
  for (ip = 0; ip < 6; ip++) {
    for (ic = 0; ic < 5; ic++) {
      int32 fl = SEFLG_SWIEPH | rgf[ic].fl;
      double w[3] = {0, 0, 0};
      for (y = 0; y < 13; y++) {
        double yr = -3000 + y * 500.0;
        double jd = floor((2451545.0 + yr * 365.25 + 37.3 * y) * 256) / 256;
        double x[6], d[5][6];
        if (swe_calc_pctr(jd, rgp[ip].ipl, rgp[ip].ictr, fl | SEFLG_SPEED, x, serr) < 0) {
          printf("  %-8s ERROR %s\n", rgp[ip].sz, serr);
          bad++;
          break;
        }
        for (s = 6; s <= 9; s++) {
          double h = ldexp(1, -s);
          for (k = 0; k < 5; k++)
            swe_calc_pctr(jd + (k - 2) * h, rgp[ip].ipl, rgp[ip].ictr, fl, d[k], serr);
          for (i = 0; i < 3; i++) {
            double v[5], r, e;
            for (k = 0; k < 5; k++) {
              v[k] = d[k][i];
              if (i == 0)
                v[k] = d[2][0] + swe_difdeg2n(v[k], d[2][0]);
            }
            r = (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h);
            e = fabs(x[3 + i] - r);
            if (e > w[i])
              w[i] = e;
          }
        }
      }
      printf("  %-8s %-7s lon %.2e lat %.2e deg/day, dist %.2e AU/day\n",
             rgp[ip].sz, rgf[ic].sz, w[0], w[1], w[2]);
      if (w[0] > 5e-9 || w[1] > 5e-9 || w[2] > 5e-10) {
        printf("    FAIL: speed off its positions' rate (5e-9 deg/day, 5e-10 AU/day)\n");
        bad++;
      }
    }
  }
  /* the deflection, seen from the centre */
  {
    const int32 fx = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_EQUATORIAL
                     | SEFLG_XYZ | SEFLG_NONUT | SEFLG_NOABERR;
    const int32 fb = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_EQUATORIAL
                     | SEFLG_XYZ | SEFLG_BARYCTR | SEFLG_TRUEPOS | SEFLG_NOABERR
                     | SEFLG_NOGDEFL;
    const double c = 299792458.0 * 86400.0 / 149597870700.0;   /* AU/day */
    double wsun = 0, wref = 0, dmax = 0, wgeo = 0, wgeos = 0;
    for (k = 0; k < 400; k++) {
      double jd = 2451545.0 + k * 18.37, a[6], u[6], xo[6], s0[6], s1[6], r[3];
      double tau, d;
      /* the Sun from Jupiter: not deflected by itself */
      swe_calc_pctr(jd, SE_SUN, SE_JUPITER, fx, a, serr);
      swe_calc_pctr(jd, SE_SUN, SE_JUPITER, fx | SEFLG_NOGDEFL, u, serr);
      if ((d = angle(a, u)) > wsun)
        wsun = d;
      /* Saturn from Mercury against the deflection from Mercury */
      swe_calc_pctr(jd, SE_SATURN, SE_MERCURY, fx, a, serr);
      swe_calc_pctr(jd, SE_SATURN, SE_MERCURY, fx | SEFLG_NOGDEFL, u, serr);
      swe_calc(jd, SE_MERCURY, fb, xo, serr);
      swe_calc(jd, SE_SUN, fb, s0, serr);
      tau = sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]) / c;
      swe_calc(jd - tau, SE_SUN, fb, s1, serr);
      defl_ref(u, xo, s0, s1, r);
      if ((d = angle(a, u)) > dmax)
        dmax = d;
      if ((d = angle(a, r)) > wref)
        wref = d;
    }
    /* the planets from the Earth are swe_calc()'s */
    for (k = 0; k < 13; k++) {
      double jd = 2451545.0 + (k - 6) * 500 * 365.25 + 0.137, a[6], b[6], d;
      int ipl;
      for (ipl = SE_SUN; ipl <= SE_PLUTO; ipl++) {
        if (ipl == SE_MOON)
          continue;
        swe_calc_pctr(jd, ipl, SE_EARTH, SEFLG_SWIEPH | SEFLG_SPEED, a, serr);
        swe_calc(jd, ipl, SEFLG_SWIEPH | SEFLG_SPEED, b, serr);
        if ((d = fabs(swe_difdeg2n(a[0], b[0])) * 3600) > wgeo)
          wgeo = d;
        if ((d = fabs(a[3] - b[3])) > wgeos)
          wgeos = d;
      }
    }
    printf("  deflection: Sun from Jupiter %.2e\", Saturn from Mercury %.3f\" "
           "and %.2e\" off its reference\n", wsun, dmax, wref);
    if (wsun != 0 || wref > 1e-5 || dmax < 0.1) {
      printf("    FAIL: the Sun deflected by itself, or Saturn's deflection not "
             "seen from Mercury (1e-5\")\n");
      bad++;
    }
    printf("  from the Earth: swe_calc()'s to %.2e\", speeds %.2e deg/day\n",
           wgeo, wgeos);
    if (wgeo > 1e-5 || wgeos > 5e-9) {
      printf("    FAIL: the planets from the Earth not swe_calc()'s (1e-5\", "
             "5e-9 deg/day)\n");
      bad++;
    }
  }
  /* the light time at its fixed point, tau = |X(tjd - tau) - O(tjd)| / c,
   * iterated here on true positions, each read corrected for the
   * rounding of its instant */
  {
    static const int rgl[][2] = {
      {SE_MERCURY, SE_MARS}, {SE_MOON, SE_VENUS}, {SE_VENUS, SE_MOON},
      {SE_MERCURY, SE_PLUTO}, {SE_SATURN, SE_MERCURY}, {SE_JUPITER, SE_SATURN},
    };
    const int32 fx = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_EQUATORIAL
                     | SEFLG_XYZ | SEFLG_NONUT | SEFLG_NOABERR | SEFLG_NOGDEFL;
    const int32 fb = SEFLG_SWIEPH | SEFLG_J2000 | SEFLG_ICRS | SEFLG_EQUATORIAL
                     | SEFLG_XYZ | SEFLG_BARYCTR | SEFLG_TRUEPOS | SEFLG_NOABERR
                     | SEFLG_NOGDEFL | SEFLG_SPEED;
    const double c = 299792458.0 * 86400.0 / 149597870700.0;   /* AU/day */
    double wlt = 0;
    for (ip = 0; ip < 6; ip++) {
      for (k = 0; k < 13; k++) {
        double jd = 2451545.0 + (k - 6) * 500 * 365.25 + 0.137, a[6], o[6], x[6];
        double r[3], tau = 0, d;
        int it;
        swe_calc_pctr(jd, rgl[ip][0], rgl[ip][1], fx, a, serr);
        swe_calc(jd, rgl[ip][1], fb, o, serr);
        for (it = 0; it < 6; it++) {
          double te = jd - tau, ep;
          swe_calc(te, rgl[ip][0], fb, x, serr);
          ep = (jd - te) - tau;
          for (i = 0; i < 3; i++)
            r[i] = x[i] + ep * x[i+3] - o[i];
          tau = sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]) / c;
        }
        if ((d = angle(a, r) / 3600) > wlt)
          wlt = d;
      }
    }
    printf("  light time: %.2e deg off its fixed point\n", wlt);
    if (wlt > 1e-11) {
      printf("    FAIL: the light time not at its fixed point (1e-11 deg)\n");
      bad++;
    }
  }
  printf(bad ? "G76 FAIL: %d\n" : "G76 PASS\n", bad);
  return bad ? 1 : 0;
}
