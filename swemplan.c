/* SWISSEPH
   Moshier planet routines

   modified for SWISSEPH by Dieter Koch

**************************************************************/
/* Copyright (C) 1997 - 2021 Astrodienst AG, Switzerland.  All rights reserved.

  License conditions
  ------------------

  This file is part of Swiss Ephemeris.

  Swiss Ephemeris is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  

  Swiss Ephemeris is made available by its authors under a dual licensing
  system. The software developer, who uses any part of Swiss Ephemeris
  in his or her software, must choose between one of the two license models,
  which are
  a) GNU Affero General Public License (AGPL)
  b) Swiss Ephemeris Professional License

  The choice must be made before the software developer distributes software
  containing parts of Swiss Ephemeris to others, and before any public
  service using the developed software is activated.

  If the developer choses the AGPL software license, he or she must fulfill
  the conditions of that license, which includes the obligation to place his
  or her whole software project under the AGPL or a compatible license.
  See https://www.gnu.org/licenses/agpl-3.0.html

  If the developer choses the Swiss Ephemeris Professional license,
  he must follow the instructions as found in http://www.astro.com/swisseph/ 
  and purchase the Swiss Ephemeris Professional Edition from Astrodienst
  and sign the corresponding license contract.

  The License grants you the right to use, copy, modify and redistribute
  Swiss Ephemeris, but only under certain conditions described in the License.
  Among other things, the License requires that the copyright notices and
  this notice be preserved on all copies.

  Authors of the Swiss Ephemeris: Dieter Koch and Alois Treindl

  The authors of Swiss Ephemeris have no control or influence over any of
  the derived works, i.e. over software or services created by other
  programmers which use Swiss Ephemeris functions.

  The names of the authors or of the copyright holder (Astrodienst) must not
  be used for promoting any software, product or service which uses or contains
  the Swiss Ephemeris. This copyright notice is the ONLY place where the
  names of the authors can legally appear, except in cases where they have
  given special permission in writing.

  The trademarks 'Swiss Ephemeris' and 'Swiss Ephemeris inside' may be used
  for promoting such software, products or services.
*/

#include <string.h>
#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"
#include "swemptab.h"

#define TIMESCALE 3652500.0


#define FICT_GEO 1
#ifndef SWE_UPSTREAM_COMPAT
#define FICT_MANO_T 2	/* the mean anomaly is a polynomial in T, epoch = tjd */
#define FICT_JDATE  4	/* the elements are referred to the equinox of date */
#endif
#define KGAUSS_GEO 0.0000298122353216 /* Earth only */
/* #define KGAUSS_GEO 0.00002999502129737  Earth + Moon */

static void embofs_mosh(swe_ctx *ctx, double J, double *xemb);
#ifndef SWE_UPSTREAM_COMPAT
static void mosh_series_cart(swe_ctx *ctx, double tjd, int iplm, double *x);
#endif
#ifndef SWE_UPSTREAM_COMPAT
static void embofs_mosh_rate(swe_ctx *ctx, double tjd, double *vemb, double *moon);
#endif
static int check_t_terms(double t, double tlo, double mod, char *sinp, double *doutp, double *drate);

static int read_elements_file(swe_ctx *ctx, int32 ipl, double tjd, 
  double *tjd0, double *tequ, 
  double *mano, double *sema, double *ecce, 
  double *parg, double *node, double *incl,
  char *pname, int32 *fict_ifl, double *drate, char *serr);

static const int pnoint2msh[]   = {2, 2, 0, 1, 3, 4, 5, 6, 7, 8, };


/* From Simon et al (1994)  */
static const double freqs[] =
{
/* Arc sec per 10000 Julian years.  */
  53810162868.8982,
  21066413643.3548,
  12959774228.3429,
  6890507749.3988,
  1092566037.7991,
  439960985.5372,
  154248119.3933,
  78655032.0744,
  52272245.1795
};

static const double phases[] =
{
/* Arc sec.  */
  252.25090552 * 3600.,
  181.97980085 * 3600.,
  100.46645683 * 3600.,
  355.43299958 * 3600.,
  34.35151874 * 3600.,
  50.07744430 * 3600.,
  314.05500511 * 3600.,
  304.34866548 * 3600.,
  860492.1546,
};

static const struct plantbl *planets[] =
{
  &mer404,
  &ven404,
  &ear404,
  &mar404,
  &jup404,
  &sat404,
  &ura404,
  &nep404,
  &plu404
};

/* were TLS statics; now ctx->plan_ss / ctx->plan_cc (Phase 3c).
 * Renamed on the way in: swemmoon.c has its own ss/cc of a different
 * shape, and identical names for different arrays in one struct would
 * be a trap. */

static void sscc (swe_ctx *ctx, int k, double arg, int n);

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 50: the series' arguments were formed as freq * T and
 * then reduced, T = (J - J2000) / 10000 years: three roundings of a
 * number of up to 2.7e10" (Mercury's mean longitude at -3000), each worth
 * ~2e-6" and ~1.3e-10 day of the planet's motion, against the 1.2e-10
 * day a JD resolves there; the geometric places were rough at S ~ 3-8.
 * Each argument is now rate * (J - J2000) / TIMESCALE to the last bit of
 * its reduced value: the day count exactly as dh + dl, the product with
 * its rounding error (fma), the quotient with its remainder, the whole
 * reduced modulo 360 degrees before the small parts are added. */
static double mosh_arg(double rate, double dh, double dl)
{
  double p = rate * dh;
  double pe = fma(rate, dh, -p);
  double q = p / TIMESCALE;
  double r = fma(-q, TIMESCALE, p);
  return swi_mods3600(q) + (r + pe + rate * dl) / TIMESCALE;
}

/* The series and, in dobj, its rates in rad/day and AU/day: each term
 * C(T) cos A + S(T) sin A has the rate C' cos A + S' sin A
 * + A' (S cos A - C sin A), A' the sum of its harmonics' frequencies. */
static void moshplan2(swe_ctx *ctx, double J, int iplm, double *pobj, double *dobj)
{
  int i, j, k, m, k1, ip, np, nt;
  const signed char *p;
  const double *pl, *pb, *pr;
  double su, cu, sv, cv, T, dh, dl, bb, w, a1, a0;
  double t, sl, sb, sr, dsl, dsb, dsr, dcu, dsu;
  const struct plantbl *plan = planets[iplm];

  /* J - J2000 = dh + dl exactly (two-sum) */
  dh = J - J2000;
  bb = dh - J;
  dl = (J - (dh - bb)) + (-J2000 - bb);
  T = dh / TIMESCALE;
  /* Calculate sin( i*MM ), etc. for needed multiple angles.  */
  for (i = 0; i < 9; i++)
    {
      if ((j = plan->max_harmonic[i]) > 0)
	{
	  sr = (mosh_arg(freqs[i], dh, dl) + phases[i]) * STR;
	  sscc (ctx, i, sr, j);
	}
    }

  /* Point to start of table of arguments. */
  p = plan->arg_tbl;
  /* Point to tabulated cosine and sine amplitudes.  */
  pl = plan->lon_tbl;
  pb = plan->lat_tbl;
  pr = plan->rad_tbl;
  sl = sb = sr = 0.0;
  dsl = dsb = dsr = 0.0;

  for (;;)
    {
      /* argument of sine and cosine */
      /* Number of periodic arguments. */
      np = *p++;
      if (np < 0)
	break;
      if (np == 0)
	{			/* It is a polynomial term.  */
	  nt = *p++;
	  /* Longitude polynomial: the mean longitude, its T term the
	   * one large number, taken exactly like the arguments. */
	  cu = *pl++;
	  dcu = 0.0;
	  if (nt >= 2)
	    {
	      for (ip = 0; ip < nt - 2; ip++)
		{
		  dcu = dcu * T + cu;
		  cu = cu * T + *pl++;
		}
	      a1 = *pl++;
	      a0 = *pl++;
	      sl += swi_mods3600(mosh_arg(a1, dh, dl) + (cu * T * T + a0));
	      dsl += (dcu * T + 2.0 * cu) * T + a1;
	    }
	  else
	    {
	      for (ip = 0; ip < nt; ip++)
		{
		  dcu = dcu * T + cu;
		  cu = cu * T + *pl++;
		}
	      sl += swi_mods3600(cu);
	      dsl += dcu;
	    }
	  /* Latitude polynomial. */
	  cu = *pb++;
	  dcu = 0.0;
	  for (ip = 0; ip < nt; ip++)
	    {
	      dcu = dcu * T + cu;
	      cu = cu * T + *pb++;
	    }
	  sb += cu;
	  dsb += dcu;
	  /* Radius polynomial. */
	  cu = *pr++;
	  dcu = 0.0;
	  for (ip = 0; ip < nt; ip++)
	    {
	      dcu = dcu * T + cu;
	      cu = cu * T + *pr++;
	    }
	  sr += cu;
	  dsr += dcu;
	  continue;
	}
      k1 = 0;
      cv = 0.0;
      sv = 0.0;
      w = 0.0;
      for (ip = 0; ip < np; ip++)
	{
	  /* What harmonic.  */
	  j = *p++;
	  /* Which planet.  */
	  m = *p++ - 1;
	  if (j)
	    {
	      w += j * freqs[m];
	      k = j;
	      if (j < 0)
		k = -k;
	      k -= 1;
	      su = ctx->plan_ss[m][k];	/* sin(k*angle) */
	      if (j < 0)
		su = -su;
	      cu = ctx->plan_cc[m][k];
	      if (k1 == 0)
		{		/* set first angle */
		  sv = su;
		  cv = cu;
		  k1 = 1;
		}
	      else
		{		/* combine angles */
		  t = su * cv + cu * sv;
		  cv = cu * cv - su * sv;
		  sv = t;
		}
	    }
	}
      w *= STR;
      /* Highest power of T.  */
      nt = *p++;
      /* Longitude. */
      cu = *pl++;
      su = *pl++;
      dcu = dsu = 0.0;
      for (ip = 0; ip < nt; ip++)
	{
	  dcu = dcu * T + cu;
	  dsu = dsu * T + su;
	  cu = cu * T + *pl++;
	  su = su * T + *pl++;
	}
      sl += cu * cv + su * sv;
      dsl += dcu * cv + dsu * sv + w * (su * cv - cu * sv);
      /* Latitiude. */
      cu = *pb++;
      su = *pb++;
      dcu = dsu = 0.0;
      for (ip = 0; ip < nt; ip++)
	{
	  dcu = dcu * T + cu;
	  dsu = dsu * T + su;
	  cu = cu * T + *pb++;
	  su = su * T + *pb++;
	}
      sb += cu * cv + su * sv;
      dsb += dcu * cv + dsu * sv + w * (su * cv - cu * sv);
      /* Radius. */
      cu = *pr++;
      su = *pr++;
      dcu = dsu = 0.0;
      for (ip = 0; ip < nt; ip++)
	{
	  dcu = dcu * T + cu;
	  dsu = dsu * T + su;
	  cu = cu * T + *pr++;
	  su = su * T + *pr++;
	}
      sr += cu * cv + su * sv;
      dsr += dcu * cv + dsu * sv + w * (su * cv - cu * sv);
    }
  pobj[0] = STR * sl;
  pobj[1] = STR * sb;
  pobj[2] = STR * plan->distance * sr + plan->distance;
  if (dobj != NULL) {
    dobj[0] = STR * dsl / TIMESCALE;
    dobj[1] = STR * dsb / TIMESCALE;
    dobj[2] = STR * plan->distance * dsr / TIMESCALE;
  }
}

int swi_moshplan2 (swe_ctx *ctx, double J, int iplm, double *pobj)
{
  moshplan2(ctx, J, iplm, pobj, NULL);
  return OK;
}
#else
int swi_moshplan2 (swe_ctx *ctx, double J, int iplm, double *pobj)
{
  int i, j, k, m, k1, ip, np, nt;
  const signed char *p;
  const double *pl, *pb, *pr;
  double su, cu, sv, cv, T;
  double t, sl, sb, sr;
  const struct plantbl *plan = planets[iplm];

  T = (J - J2000) / TIMESCALE;
  /* Calculate sin( i*MM ), etc. for needed multiple angles.  */
  for (i = 0; i < 9; i++)
    {
      if ((j = plan->max_harmonic[i]) > 0)
	{
	  sr = (swi_mods3600(freqs[i] * T) + phases[i]) * STR;
	  sscc (ctx, i, sr, j);
	}
    }

  /* Point to start of table of arguments. */
  p = plan->arg_tbl;
  /* Point to tabulated cosine and sine amplitudes.  */
  pl = plan->lon_tbl;
  pb = plan->lat_tbl;
  pr = plan->rad_tbl;
  sl = 0.0;
  sb = 0.0;
  sr = 0.0;

  for (;;)
    {
      /* argument of sine and cosine */
      /* Number of periodic arguments. */
      np = *p++;
      if (np < 0)
	break;
      if (np == 0)
	{			/* It is a polynomial term.  */
	  nt = *p++;
	  /* Longitude polynomial. */
	  cu = *pl++;
	  for (ip = 0; ip < nt; ip++)
	    {
	      cu = cu * T + *pl++;
	    }
	  sl +=  swi_mods3600(cu);
	  /* Latitude polynomial. */
	  cu = *pb++;
	  for (ip = 0; ip < nt; ip++)
	    {
	      cu = cu * T + *pb++;
	    }
	  sb += cu;
	  /* Radius polynomial. */
	  cu = *pr++;
	  for (ip = 0; ip < nt; ip++)
	    {
	      cu = cu * T + *pr++;
	    }
	  sr += cu;
	  continue;
	}
      k1 = 0;
      cv = 0.0;
      sv = 0.0;
      for (ip = 0; ip < np; ip++)
	{
	  /* What harmonic.  */
	  j = *p++;
	  /* Which planet.  */
	  m = *p++ - 1;
	  if (j)
	    {
	      k = j;
	      if (j < 0)
		k = -k;
	      k -= 1;
	      su = ctx->plan_ss[m][k];	/* sin(k*angle) */
	      if (j < 0)
		su = -su;
	      cu = ctx->plan_cc[m][k];
	      if (k1 == 0)
		{		/* set first angle */
		  sv = su;
		  cv = cu;
		  k1 = 1;
		}
	      else
		{		/* combine angles */
		  t = su * cv + cu * sv;
		  cv = cu * cv - su * sv;
		  sv = t;
		}
	    }
	}
      /* Highest power of T.  */
      nt = *p++;
      /* Longitude. */
      cu = *pl++;
      su = *pl++;
      for (ip = 0; ip < nt; ip++)
	{
	  cu = cu * T + *pl++;
	  su = su * T + *pl++;
	}
      sl += cu * cv + su * sv;
      /* Latitiude. */
      cu = *pb++;
      su = *pb++;
      for (ip = 0; ip < nt; ip++)
	{
	  cu = cu * T + *pb++;
	  su = su * T + *pb++;
	}
      sb += cu * cv + su * sv;
      /* Radius. */
      cu = *pr++;
      su = *pr++;
      for (ip = 0; ip < nt; ip++)
	{
	  cu = cu * T + *pr++;
	  su = su * T + *pr++;
	}
      sr += cu * cv + su * sv;
    }
  pobj[0] = STR * sl;
  pobj[1] = STR * sb;
  pobj[2] = STR * plan->distance * sr + plan->distance;
  return OK;
}
#endif /* SWE_UPSTREAM_COMPAT */

/* Moshier ephemeris.
 * computes heliocentric cartesian equatorial coordinates of
 * equinox 2000
 * for earth and a planet
 * tjd		julian day
 * ipli		internal SWEPH planet number
 * xp		array of 6 doubles for planet's position and speed
 * xe		                       earth's
 * serr		error string
 */
int swi_moshplan(swe_ctx *ctx, double tjd, int ipli, AS_BOOL do_save, double *xpret, double *xeret, char *serr) 
{
  int i;
  int do_earth = FALSE;
  double xxe[6], xxp[6];
  double *xp, *xe;
#ifdef SWE_UPSTREAM_COMPAT
  double dx[3], x2[3];
  double dt; 
#endif
  char s[AS_MAXCH];
  int iplm = pnoint2msh[ipli];
  struct plan_data *pdp = &ctx->pldat[ipli];
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
#ifdef SWE_UPSTREAM_COMPAT	/* else in mosh_series_cart() (bug 109) */
  double seps2000 = ctx->oec2000.seps;
  double ceps2000 = ctx->oec2000.ceps;
#endif
  if (do_save) {
    xp = pdp->x;
    xe = pedp->x;
  } else {
    xp = xxp;
    xe = xxe;
  }
  if (do_save || ipli == SEI_EARTH || xeret != NULL) 
    do_earth = TRUE;
  /* tjd beyond ephemeris limits, give some margin for spped at edge */
  if (tjd < MOSHPLEPH_START - 0.3 || tjd > MOSHPLEPH_END + 0.3) {
    if (serr != NULL) {
      sprintf(s, "jd %f outside Moshier planet range %.2f .. %.2f ",
		    tjd, MOSHPLEPH_START, MOSHPLEPH_END);
      if (strlen(serr) + strlen(s) < AS_MAXCH)
	strcat(serr, s);
    }
    return(ERR);
  }  
  /* earth, for geocentric position */
  if (do_earth) {
    if (tjd == pedp->teval
	  && pedp->iephe == SEFLG_MOSEPH) {
      xe = pedp->x;
    } else {
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 51: the velocity was (x(t) - x(t - h)) / h, h =
       * 8.64 s: the speed at t - h/2, its instants rounded at the JD's
       * resolution and the series' own rounding divided by h -- ~3e-6 of
       * the speed at -3000, which the light time (x - tau v) and the
       * aberration carried into the apparent places (0.19 mas on
       * Mercury, S ~ 250) and the osculating elements amplified. Now the
       * series' derivative, and the Earth-Moon offset's rate from a
       * central difference over exact instants. */
      mosh_series_cart(ctx, tjd, pnoint2msh[SEI_EMB], xe);	/* bug 109 */
      embofs_mosh(ctx, tjd, xe);
      embofs_mosh_rate(ctx, tjd, xe + 3, do_save ? ctx->earth_moon.x : NULL);
      if (do_save) {
	/* Upstream bug 177: the barycentric Sun's slot, which the files and
	 * JPL fill with the Earth's, kept whatever they last left in it
	 * where Moshier filled the Earth's: zero in a fresh context, another
	 * instant's Swiss Sun after a warm fallback, and the Moshier paths
	 * that read it took that for their Sun -- the light deflection
	 * (Mercury at 1500 1.66e-7 deg off the fresh fallback's after a
	 * Swiss call at 2000, Mars 1.7e-9, swe_nod_aps()'s Venus 1.5e-7),
	 * the heliocentric Moon (0.40 deg), an asteroid's frame from the
	 * slot's ephemeris (Ceres heliocentric 0.14 deg), the fictitious
	 * bodies' and swe_calc_orbel()'s Sun at teval (Cupido 9.1e-3 deg,
	 * the orbit about the Sun 0.071 deg, both SEFLG_TRUEPOS). It holds
	 * Moshier's Sun now, the origin of its heliocentric frame at this
	 * instant, as it holds the files' Sun with their Earth. */
	struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
	for (i = 0; i <= 5; i++)
	  psdp->x[i] = 0;
	psdp->teval = tjd;
	psdp->xflgs = -1;
	psdp->iephe = SEFLG_MOSEPH;
	ctx->earth_moon.t = tjd;
	ctx->earth_moon.iephe = SEFLG_MOSEPH;
	pedp->teval = tjd;
	pedp->xflgs = -1;
	pedp->iephe = SEFLG_MOSEPH;
      }
#else
      /* emb */
      swi_moshplan2(ctx, tjd, pnoint2msh[SEI_EMB], xe); /* emb hel. ecl. 2000 polar */ 
      swi_polcart(xe, xe);			  /* to cartesian */
      swi_coortrf2(xe, xe, -seps2000, ceps2000);/* and equator 2000 */
      embofs_mosh(ctx, tjd, xe);		  /* emb -> earth */
      if (do_save) {
	pedp->teval = tjd;		  
	pedp->xflgs = -1;
	pedp->iephe = SEFLG_MOSEPH;
      }
      /* one more position for speed. */
      swi_moshplan2(ctx, tjd - PLAN_SPEED_INTV, pnoint2msh[SEI_EMB], x2); 
      swi_polcart(x2, x2);
      swi_coortrf2(x2, x2, -seps2000, ceps2000);
      embofs_mosh(ctx, tjd - PLAN_SPEED_INTV, x2);/**/
      for (i = 0; i <= 2; i++) 
	dx[i] = (xe[i] - x2[i]) / PLAN_SPEED_INTV;
      /* store speed */
      for (i = 0; i <= 2; i++) {
	xe[i+3] = dx[i];
      }
#endif
    }
    if (xeret != NULL)
      for (i = 0; i <= 5; i++) 
	xeret[i] = xe[i];
  }
  /* earth is the planet wanted */
  if (ipli == SEI_EARTH) {
    xp = xe;
  } else {
    /* other planet */
    /* if planet has already been computed, return */
    if (tjd == pdp->teval && pdp->iephe == SEFLG_MOSEPH) {
      xp = pdp->x;
    } else { 
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 51: the series' derivative, see the Earth above;
       * bug 109: memoised with the Sun's barycentric motion */
      mosh_series_cart(ctx, tjd, iplm, xp);
      if (do_save) {
	pdp->teval = tjd;/**/
	pdp->xflgs = -1;
	pdp->iephe = SEFLG_MOSEPH;
      }
#else
      swi_moshplan2(ctx, tjd, iplm, xp); 
      swi_polcart(xp, xp);
      swi_coortrf2(xp, xp, -seps2000, ceps2000);
      if (do_save) {
	pdp->teval = tjd;/**/
	pdp->xflgs = -1;
	pdp->iephe = SEFLG_MOSEPH;
      }
      /* one more position for speed. 
       * the following dt gives good speed for light-time correction
       */
      dt = PLAN_SPEED_INTV;
      swi_moshplan2(ctx, tjd - dt, iplm, x2); 
      swi_polcart(x2, x2);
      swi_coortrf2(x2, x2, -seps2000, ceps2000);
      for (i = 0; i <= 2; i++) 
	dx[i] = (xp[i] - x2[i]) / dt;
      /* store speed */
      for (i = 0; i <= 2; i++) {
	xp[i+3] = dx[i];
      }
#endif
    }
    if (xpret != NULL)
      for (i = 0; i <= 5; i++)
	xpret[i] = xp[i];
  }
  return(OK);
}

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bugs 102, 109: a planet's series, heliocentric equatorial J2000
 * cartesian with its analytic velocity, as swi_moshplan() forms it,
 * memoised per planet and instant (Moshier's order, 2 the Earth-Moon
 * barycentre), so that the Sun's barycentric motion below and the bodies of
 * a chart share every series: computed once, the same bits either way. */
static void mosh_series_eval(swe_ctx *ctx, double tjd, int iplm, double *c)
{
  moshplan2(ctx, tjd, iplm, c, c + 3);
  swi_polcart_sp(c, c);
  swi_coortrf2(c, c, -ctx->oec2000.seps, ctx->oec2000.ceps);
  swi_coortrf2(c + 3, c + 3, -ctx->oec2000.seps, ctx->oec2000.ceps);
}

static void mosh_series_cart(swe_ctx *ctx, double tjd, int iplm, double *x)
{
  int i;
  double *c = ctx->mosh_sun_bary.xp[iplm];
  if (ctx->mosh_sun_bary.tp[iplm] != tjd) {
    mosh_series_eval(ctx, tjd, iplm, c);
    ctx->mosh_sun_bary.tp[iplm] = tjd;
  }
  for (i = 0; i <= 5; i++)
    x[i] = c[i];
}

/* Upstream bug 102: the Sun's barycentric place, velocity and acceleration,
 * equatorial J2000, AU, AU/day and AU/day^2, from Moshier's own planets, for
 * the light times that Moshier's heliocentric frame cannot see the Sun
 * move over. The barycentre of the Sun and the planets as point masses (the
 * Earth-Moon barycentre's series for the Earth and the Moon): x_S =
 * -sum m_i x_i / (M_S + sum m_i), v_S the same of the velocities, x_i, v_i
 * heliocentric; a_S = k^2 sum (m_i / M_S) x_i / r_i^3, the planets' pull.
 * The masses DE431's, as swecl.c's. Left out: the asteroids (Ceres 8e-6
 * m/s). Against the Swiss files' barycentric Sun <= 6.5 mm/s 1800..2200,
 * as the files' own planets give the same way (1.2e-9 deg of v_S / c).
 * Upstream bug 109: Pluto joins (3.4e-5 m/s, 5.5e-12 AU over Pluto's light
 * time); SWI_MOSH_SUN_MOON takes Venus, the Earth-Moon barycentre and the
 * four giants alone, for the Moon's apparent place, whose direction does not
 * depend on v_S and whose distance wants it to 0.11 m/s over 1.3 s (left
 * out 0.016 m/s, 1.4e-13 AU). Memoised per instant, the series per planet:
 * a chart's bodies share them. */
static const double mosh_plmass[9] = {	/* M_Sun / m, Moshier's order */
  6023600,        /* Mercury */
  408523.719,     /* Venus */
  328900.5,       /* Earth and Moon */
  3098703.59,     /* Mars */
  1047.348644,    /* Jupiter */
  3497.9018,      /* Saturn */
  22902.98,       /* Uranus */
  19412.26,       /* Neptune */
  136566000,      /* Pluto */
};
static const int mosh_set_moon[9] = {0, 1, 1, 0, 1, 1, 1, 1, 0};
/* the sums; cached: the series memoised (a chart's), else evaluated aside,
 * leaving the memo to the instant a chart is at */
static void mosh_sun_sum(swe_ctx *ctx, double tjd, int set, int cached,
                         double *xs, double *as)
{
  int i, ip;
  double x[6], r, m, msum = 1, p[6] = {0, 0, 0, 0, 0, 0}, a[3] = {0, 0, 0};
  const double k2 = HELGRAVCONST * 86400.0 * 86400.0 / (AUNIT * AUNIT * AUNIT);
  for (ip = 0; ip < 9; ip++) {
    if (set == SWI_MOSH_SUN_MOON && !mosh_set_moon[ip])
      continue;
    if (cached)
      mosh_series_cart(ctx, tjd, ip, x);
    else
      mosh_series_eval(ctx, tjd, ip, x);
    m = 1.0 / mosh_plmass[ip];
    r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    r = r * r * r;
    msum += m;
    for (i = 0; i <= 2; i++) {
      p[i] += m * x[i];
      p[i+3] += m * x[i+3];
      a[i] += m * x[i] / r;
    }
  }
  for (i = 0; i <= 5; i++)
    xs[i] = -p[i] / msum;
  if (as != NULL)
    for (i = 0; i <= 2; i++)
      as[i] = k2 * a[i];
}

void swi_mosh_sun_bary(swe_ctx *ctx, double tjd, int set, double *xs, double *as)
{
  int i;
  struct swi_mosh_sun *ms = &ctx->mosh_sun_bary.s[set];
  if (ms->t != tjd) {
    mosh_sun_sum(ctx, tjd, set, 1, ms->x, ms->a);
    ms->t = tjd;
  }
  for (i = 0; i <= 5; i++)
    xs[i] = ms->x[i];
  if (as != NULL)
    for (i = 0; i <= 2; i++)
      as[i] = ms->a[i];
}

/* Upstream bug 125: the same place and velocity at a light time's
 * emission instant more than a day from the chart's (a body past 173
 * AU), evaluated aside: the memos stay at the chart's instant. */
void swi_mosh_sun_bary_aside(swe_ctx *ctx, double tjd, int set, double *xs)
{
  mosh_sun_sum(ctx, tjd, set, 0, xs, NULL);
}

/* Upstream bug 109: the rate of swi_mosh_sun_bary()'s v_S (every planet)
 * at tjd, for a star's aberration rate: a star's speed is the rate of its
 * positions, which take the series' v_S. The point masses' pull above is
 * the Newtonian rate; the giants' series are 1.2e-4 (Jupiter) to 7e-3
 * (Neptune) of their acceleration off it, 2e-12 AU/day^2 on the Sun,
 * 1.2e-14 rad/day of aberration rate, 1.05e-10 deg/day in Polaris's right
 * ascension near the pole. (1 + sum m_i) a_S = -sum m_i a_i, a_i each
 * planet's heliocentric acceleration: the four giants' from their series,
 * one-sided over 2^-9 day (1.5e-14 AU/day^2 of the Sun's from Jupiter's
 * rate of change), the others' Newtonian with the planets' pull and its
 * indirect part (their series are within 5e-6 of it, 3e-15 AU/day^2 on
 * the Sun). Four series at tjd + 2^-9, outside the memo. */
void swi_mosh_sun_accel(swe_ctx *ctx, double tjd, double *as)
{
  int i, ip, jp;
  const double h = 1.0 / 512;
  const double k2 = HELGRAVCONST * 86400.0 * 86400.0 / (AUNIT * AUNIT * AUNIT);
  double x[9][6], xh[6], ai[3], d[3], r, msum = 1, sum[3] = {0, 0, 0}, rj3[9];
  for (ip = 0; ip < 9; ip++) {
    mosh_series_cart(ctx, tjd, ip, x[ip]);
    r = sqrt(x[ip][0] * x[ip][0] + x[ip][1] * x[ip][1] + x[ip][2] * x[ip][2]);
    rj3[ip] = r * r * r;
  }
  for (ip = 0; ip < 9; ip++) {
    double m = 1.0 / mosh_plmass[ip];
    if (ip >= 4 && ip <= 7) {	/* Jupiter to Neptune */
      mosh_series_eval(ctx, tjd + h, ip, xh);
      for (i = 0; i <= 2; i++)
        ai[i] = (xh[i+3] - x[ip][i+3]) / h;
    } else {
      for (i = 0; i <= 2; i++)
        ai[i] = -k2 * (1 + m) * x[ip][i] / rj3[ip];
      for (jp = 0; jp < 9; jp++) {
        if (jp == ip)
          continue;
        for (i = 0; i <= 2; i++)
          d[i] = x[jp][i] - x[ip][i];
        r = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        r = r * r * r;
        for (i = 0; i <= 2; i++)
          ai[i] += k2 / mosh_plmass[jp] * (d[i] / r - x[jp][i] / rj3[jp]);
      }
    }
    msum += m;
    for (i = 0; i <= 2; i++)
      sum[i] += m * ai[i];
  }
  for (i = 0; i <= 2; i++)
    as[i] = -sum[i] / msum;
}

#endif


/* Prepare lookup table of sin and cos ( i*Lj )
 * for required multiple angles
 */
static void sscc (swe_ctx *ctx, int k, double arg, int n)
{
  double cu, su, cv, sv, s;
  int i;

  su = sin (arg);
  cu = cos (arg);
  ctx->plan_ss[k][0] = su;		/* sin(L) */
  ctx->plan_cc[k][0] = cu;		/* cos(L) */
  sv = 2.0 * su * cu;
  cv = cu * cu - su * su;
  ctx->plan_ss[k][1] = sv;		/* sin(2L) */
  ctx->plan_cc[k][1] = cv;
  for (i = 2; i < n; i++)
    {
      s = su * cv + cu * sv;
      cv = cu * cv - su * sv;
      sv = s;
      ctx->plan_ss[k][i] = sv;		/* sin( i+1 L ) */
      ctx->plan_cc[k][i] = cv;
    }
}


/* Adjust position from Earth-Moon barycenter to Earth
 *
 * J = Julian day number
 * xemb = rectangular equatorial coordinates of Earth
 */
#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 85 (part 2): an argument of the short lunar series, rate
 * degrees per century times T, reduced modulo 360 to its last bit, as bug
 * 50's mosh_arg() does for the planets: T exact as th + tl, the product
 * rate * th with its rounding error (fma), the large part reduced exactly
 * (p - 360 k is a multiple of ulp(p) below 360). Formed as rate * T, the
 * arguments rounded at ulp(rate T): 1e-12 rad near T = 1, 6.5e-11 at -3000
 * (T = -49). */
static double moon_arg(double rate, double th, double tl)
{
  double p = rate * th;
  double pe = fma(rate, th, -p);
  return (p - 360.0 * floor(p * (1.0 / 360.0))) + (pe + rate * tl);
}
#endif

/* The geocentric Moon of the short series below, J2000 equatorial, AU; with
 * v != NULL also its velocity, AU/day: the series' derivative, term by term
 * (UPSTREAM-BUGS 85). The position's arithmetic is the series' own, to the
 * bit, with or without v. Reads ctx->oec, held over the call. */
static void moon_short(swe_ctx *ctx, double tjd, double *xyz, double *v)
{
  double T, M, a, L, B, p;
  double smp, cmp, s2mp, c2mp, s2d, c2d, sf, cf;
  double s2f, sx, cx;
  double seps = ctx->oec.seps;
  double ceps = ctx->oec.ceps;
  /* rates, per day: arguments in radians, L B p in degrees */
  double dmp = 0, d2d = 0, df = 0, dM = 0, dL = 0, dB = 0, dp = 0;
  double dsmp = 0, dcmp = 0, ds2mp = 0, dc2mp = 0, ds2d = 0, dc2d = 0;
  double dsf = 0, dcf = 0, ds2f = 0, dsx = 0, dcx = 0, da, dsx2;
  double pm[9], pdm[9];
  int havem = 0;
#ifndef SWE_UPSTREAM_COMPAT
  double deps = 0;
#endif
  const double tc = 1.0 / 36525.0;
#ifndef SWE_UPSTREAM_COMPAT
  /* tjd - J1900 = dh + dl exactly (two-sum), and T = th + tl, for
   * moon_arg() */
  double dh = tjd - J1900, bb = dh - tjd;
  double dl = (tjd - (dh - bb)) + (-J1900 - bb);
  double th = dh / 36525.0;
  double tl = (fma(-th, 36525.0, dh) + dl) / 36525.0;
#endif
  /* Short series for position of the Moon
   */
  T = (tjd-J1900)/36525.0;
  /* Mean anomaly of moon (MP) */
#ifndef SWE_UPSTREAM_COMPAT
  a = swe_degnorm(moon_arg(477198.8491, th, tl) + ((1.44e-5*T + 0.009192)*T*T + 296.104608));
#else
  a = swe_degnorm(((1.44e-5*T + 0.009192)*T + 477198.8491)*T + 296.104608);
#endif
  a *= DEGTORAD;
  smp = sin(a);
  cmp = cos(a);
  s2mp = 2.0*smp*cmp;		/* sin(2MP) */
  c2mp = cmp*cmp - smp*smp;	/* cos(2MP) */
  if (v) {
    dmp = ((3 * 1.44e-5*T + 2 * 0.009192)*T + 477198.8491) * DEGTORAD * tc;
    dsmp = cmp * dmp;
    dcmp = -smp * dmp;
    ds2mp = 2 * c2mp * dmp;
    dc2mp = -2 * s2mp * dmp;
  }
  /* Mean elongation of moon (D) */
#ifndef SWE_UPSTREAM_COMPAT
  a = swe_degnorm(moon_arg(445267.1142, th, tl) + ((1.9e-6*T - 0.001436)*T*T + 350.737486));
#else
  a = swe_degnorm(((1.9e-6*T - 0.001436)*T + 445267.1142)*T + 350.737486);
#endif
  a  = 2.0 * DEGTORAD * a;
  s2d = sin(a);
  c2d = cos(a);
  if (v) {
    d2d = 2.0 * ((3 * 1.9e-6*T - 2 * 0.001436)*T + 445267.1142) * DEGTORAD * tc;
    ds2d = c2d * d2d;
    dc2d = -s2d * d2d;
  }
  /* Mean distance of moon from its ascending node (F) */
#ifndef SWE_UPSTREAM_COMPAT
  a = swe_degnorm(moon_arg(483202.0251, th, tl) + (( -3.e-7*T - 0.003211)*T*T + 11.250889));
#else
  a = swe_degnorm((( -3.e-7*T - 0.003211)*T + 483202.0251)*T + 11.250889);
#endif
  a  *= DEGTORAD;
  sf = sin(a);
  cf = cos(a);
  s2f = 2.0*sf*cf;	/* sin(2F) */
  sx = s2d*cmp - c2d*smp;	/* sin(2D - MP) */
  cx = c2d*cmp + s2d*smp;	/* cos(2D - MP) */
  if (v) {
    df = (( -3 * 3.e-7*T - 2 * 0.003211)*T + 483202.0251) * DEGTORAD * tc;
    dsf = cf * df;
    dcf = -sf * df;
    ds2f = 2.0 * (cf*cf - sf*sf) * df;
    dsx = cx * (d2d - dmp);
    dcx = -sx * (d2d - dmp);
  }
  /* Mean longitude of moon (LP) */
#ifndef SWE_UPSTREAM_COMPAT
  L = moon_arg(481267.8831, th, tl) + ((1.9e-6*T - 0.001133)*T*T + 270.434164);
  /* Mean anomaly of sun (M) */
  M = swe_degnorm(moon_arg(35999.0498, th, tl) + (( -3.3e-6*T - 1.50e-4)*T*T + 358.475833));
#else
  L = ((1.9e-6*T - 0.001133)*T + 481267.8831)*T + 270.434164;
  /* Mean anomaly of sun (M) */
  M = swe_degnorm((( -3.3e-6*T - 1.50e-4)*T + 35999.0498)*T + 358.475833);
#endif
  if (v) {
    dM = (( -3 * 3.3e-6*T - 2 * 1.50e-4)*T + 35999.0498) * DEGTORAD * tc;
    dL = ((3 * 1.9e-6*T - 2 * 0.001133)*T + 481267.8831) * tc
      + 6.288750*dsmp
      + 1.274018*dsx
      + 0.658309*ds2d
      + 0.213616*ds2mp
      - 0.185596*cos( DEGTORAD * M ) * dM
      - 0.114336*ds2f;
  }
  /* Ecliptic longitude of the moon */
  L =	L
	+ 6.288750*smp
	+ 1.274018*sx
	+ 0.658309*s2d
	+ 0.213616*s2mp
	- 0.185596*sin( DEGTORAD * M )
	- 0.114336*s2f;
  /* Ecliptic latitude of the moon */
  a = smp*cf;
  sx = cmp*sf;
  B =	  5.128189*sf
	+ 0.280606*(a+sx)		/* sin(MP+F) */
	+ 0.277693*(a-sx)		/* sin(MP-F) */
	+ 0.173238*(s2d*cf - c2d*sf);	/* sin(2D-F) */
  B *= DEGTORAD;
  if (v) {
    da = dsmp*cf + smp*dcf;
    dsx2 = dcmp*sf + cmp*dsf;
    dB = (5.128189*dsf
	+ 0.280606*(da+dsx2)
	+ 0.277693*(da-dsx2)
	+ 0.173238*(ds2d*cf + s2d*dcf - dc2d*sf - c2d*dsf)) * DEGTORAD;
    dp = (0.051818*dcmp
	+ 0.009531*dcx
	+ 0.007843*dc2d
	+ 0.002824*dc2mp) * DEGTORAD;
  }
  /* Parallax of the moon */
  p =	 0.950724
	+0.051818*cmp
	+0.009531*cx
	+0.007843*c2d
	+0.002824*c2mp;
  p *= DEGTORAD;
  /* Elongation of Moon from Sun
   */
  L = swe_degnorm(L);
  L *= DEGTORAD;
  /* Distance in au */
  a = 4.263523e-5/sin(p);
  /* Convert to rectangular ecliptic coordinates */
  xyz[0] = L;
  xyz[1] = B;
  xyz[2] = a;
  if (v) {
    double x6[6];
    x6[0] = L;
    x6[1] = B;
    x6[2] = a;
    x6[3] = dL * DEGTORAD;
    x6[4] = dB;
    x6[5] = -a * cos(p) / sin(p) * dp;
    swi_polcart_sp(x6, x6);
    /* the ecliptic of date turns about the equinox as the obliquity
     * changes: each place is rotated with its own date's ctx->oec. The
     * rate from the precession series' poles, computed for the
     * precession's rate below anyway (UPSTREAM-BUGS 85 part 2) */
#ifndef SWE_UPSTREAM_COMPAT	/* (the compat build never asks for v) */
    havem = swi_precess_matrix_rate_eps(ctx, tjd, 0, pm, pdm, &deps);
    if (!havem)
      deps = swi_obliquity_rate(ctx, tjd, 0, FALSE);
    swi_ecl_frame_speed(x6, -deps);
#endif
    swi_coortrf2(x6 + 3, x6 + 3, -seps, ceps);
    for (int i = 0; i <= 2; i++)
      v[i] = x6[i+3];
  }
  swi_polcart(xyz, xyz);
  /* Convert to equatorial */
  swi_coortrf2(xyz, xyz, -seps, ceps);
  /* Precess to equinox of J2000.0 */
  swi_precess(ctx, xyz, tjd, 0, J_TO_J2000);/**/
  if (v) {
    /* the precession's own turning, dP/dt x0 */
    if (havem) {
      /* as swi_precess_speed() does it, M and dM/dt in hand: x(J) = M
       * x(J2000), so v(J2000) = M^T v + dM^T x0, x0 the date's x */
      double x0[3], w[3];
      for (int i = 0; i <= 2; i++)
        x0[i] = xyz[0] * pm[3 * i] + xyz[1] * pm[3 * i + 1] + xyz[2] * pm[3 * i + 2];
      for (int i = 0; i <= 2; i++)
        w[i] = v[0] * pm[i] + v[1] * pm[i + 3] + v[2] * pm[i + 6]
          + x0[0] * pdm[i] + x0[1] * pdm[i + 3] + x0[2] * pdm[i + 6];
      for (int i = 0; i <= 2; i++)
        v[i] = w[i];
    } else {
      double x6[6];
      for (int i = 0; i <= 2; i++) {
        x6[i] = xyz[i];
        x6[i+3] = v[i];
      }
      swi_precess_speed(ctx, x6, tjd, 0, J_TO_J2000);
      for (int i = 0; i <= 2; i++)
        v[i] = x6[i+3];
    }
  }
}

/* Adjust position from Earth-Moon barycenter to Earth
 *
 * J = Julian day number
 * xemb = rectangular equatorial coordinates of Earth
 */
/* reads ctx->oec */
static void embofs_mosh(swe_ctx *ctx, double tjd, double *xemb) 
{
  double xyz[6];
  int i;
  moon_short(ctx, tjd, xyz, NULL);
  /* now emb -> earth */
  for (i = 0; i <= 2; i++)
    xemb[i] -= xyz[i] / (EARTH_MOON_MRAT + 1.0);
}

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 66 (part 3): the acceleration of the Earth this series
 * returns, a = (x(t+h) - 2 x(t) + x(t-h)) / h^2 over exact instants, h =
 * 2^-8 day, of the EMB series plus its Moon offset, from x0 the saved Earth
 * at tjd. Truncation (w h)^2 / 12 = 7e-8 of the Moon's part, rounding
 * 1e-16 / h^2 = 7e-12 AU/day^2 against 3e-4. The aberration rate's tau a
 * wants the Earth's own acceleration, not the gravitational sum's: the
 * series' small monthly terms move it by 1e-4 of a (4-6e-9 deg/day on the
 * far bodies). */
void swi_mosh_earth_accel(swe_ctx *ctx, double tjd, const double *x0, double *acc)
{
  const double h = 1.0 / 256;
  double xp[3], xm[3], xx[6];
  double seps2000 = ctx->oec2000.seps, ceps2000 = ctx->oec2000.ceps;
  int i, k;
  for (k = 0; k < 2; k++) {
    double *x = k ? xm : xp;
    double t = tjd + (k ? -h : h);
    swi_moshplan2(ctx, t, pnoint2msh[SEI_EMB], xx);
    swi_polcart(xx, xx);
    swi_coortrf2(xx, xx, -seps2000, ceps2000);
    embofs_mosh(ctx, t, xx);
    for (i = 0; i <= 2; i++)
      x[i] = xx[i];
  }
  for (i = 0; i <= 2; i++)
    acc[i] = (xp[i] - 2 * x0[i] + xm[i]) / (h * h);
}

/* Subtracts the Earth-Moon offset's rate from vemb. Upstream bug 85: it was
 * a central difference of the offset over +-2^-8 day, and the series'
 * arguments, degrees of 481267.88 T, round at 1e-12 rad once T is near 1
 * (at -3000, T = -49, 5e-11): 2e-14 AU/day of roughness from one instant
 * to the next in every Moshier Earth velocity, 2e-6 of the Earth's
 * acceleration from velocities 1e-5 day apart at 2000, 4e-5 at -3000. Now
 * the series' derivative, term by term, with the precession's turning;
 * the obliquity held, as the positions hold ctx->oec. */
static void embofs_mosh_rate(swe_ctx *ctx, double tjd, double *vemb, double *moon)
{
  double xyz[3], v[3];
  int i;
  moon_short(ctx, tjd, xyz, v);
  for (i = 0; i <= 2; i++)
    vemb[i] -= v[i] / (EARTH_MOON_MRAT + 1.0);
  /* UPSTREAM-BUGS 66: the geocentric Moon of this series, place and
   * velocity. The aberration rate's Earth wants it (acceleration, 0.6%). */
  if (moon != NULL)
    for (i = 0; i <= 2; i++) {
      moon[i] = xyz[i];
      moon[i+3] = v[i];
    }
}
#endif

/* orbital elements of planets that are computed from osculating elements
 *   epoch
 *   equinox
 *   mean anomaly,
 *   semi axis,
 *   eccentricity,
 *   argument of perihelion,
 *   ascending node
 *   inclination
 */
#define SE_NEELY                /* use James Neely's revised elements 
                                 *      of Uranian planets*/
static const char *plan_fict_nam[SE_NFICT_ELEM] =
  {"Cupido", "Hades", "Zeus", "Kronos", 
   "Apollon", "Admetos", "Vulkanus", "Poseidon",
   "Isis-Transpluto", "Nibiru", "Harrington",
   "Leverrier", "Adams",
   "Lowell", "Pickering",};

char *swi_get_fict_name(swe_ctx *ctx, int32 ipl, char *snam) 
{
  if (read_elements_file(ctx, ipl, 0, NULL, NULL, 
       NULL, NULL, NULL, NULL, NULL, NULL, 
       snam, NULL, NULL, NULL) == ERR)
    strcpy(snam, "name not found");
  return snam;
}

static const double plan_oscu_elem[SE_NFICT_ELEM][8] = {
#ifdef SE_NEELY
  {J1900, J1900, 163.7409, 40.99837, 0.00460, 171.4333, 129.8325, 1.0833},/* Cupido Neely */ 
  {J1900, J1900,  27.6496, 50.66744, 0.00245, 148.1796, 161.3339, 1.0500},/* Hades Neely */
  {J1900, J1900, 165.1232, 59.21436, 0.00120, 299.0440,   0.0000, 0.0000},/* Zeus Neely */
  /* Upstream bug 65: Kronos' semi-axis was 64.81960, a digit swap of
   * seorbel.txt's 64.81690 (and Witte's original 64.816896): without the
   * file Kronos drifted 0.0043 deg per century from 1900, -0.0086 deg at
   * 1700, +0.0169 deg at 2300. It is seorbel.txt's now. The other fourteen
   * rows agree with the file. */
#ifndef SWE_UPSTREAM_COMPAT
  {J1900, J1900, 169.0193, 64.81690, 0.00305, 208.8801,   0.0000, 0.0000},/* Kronos Neely */ 
#else
  {J1900, J1900, 169.0193, 64.81960, 0.00305, 208.8801,   0.0000, 0.0000},/* Kronos Neely */ 
#endif
  {J1900, J1900, 138.0533, 70.29949, 0.00000,   0.0000,   0.0000, 0.0000},/* Apollon Neely */
  {J1900, J1900, 351.3350, 73.62765, 0.00000,   0.0000,   0.0000, 0.0000},/* Admetos Neely */
  {J1900, J1900,  55.8983, 77.25568, 0.00000,   0.0000,   0.0000, 0.0000},/* Vulcanus Neely */
  {J1900, J1900, 165.5163, 83.66907, 0.00000,   0.0000,   0.0000, 0.0000},/* Poseidon Neely */
#else
  {J1900, J1900, 104.5959, 40.99837,  0, 0, 0, 0}, /* Cupido   */
  {J1900, J1900, 337.4517, 50.667443, 0, 0, 0, 0}, /* Hades    */
  {J1900, J1900, 104.0904, 59.214362, 0, 0, 0, 0}, /* Zeus     */
  {J1900, J1900,  17.7346, 64.816896, 0, 0, 0, 0}, /* Kronos   */
  {J1900, J1900, 138.0354, 70.361652, 0, 0, 0, 0}, /* Apollon  */
  {J1900, J1900,  -8.678,  73.736476, 0, 0, 0, 0}, /* Admetos  */
  {J1900, J1900,  55.9826, 77.445895, 0, 0, 0, 0}, /* Vulkanus */
  {J1900, J1900, 165.3595, 83.493733, 0, 0, 0, 0}, /* Poseidon */
#endif
  /* Isis-Transpluto; elements from "Die Sterne" 3/1952, p. 70ff. 
   * Strubell does not give an equinox. 1945 is taken to best reproduce 
   * ASTRON ephemeris. (This is a strange choice, though.)
   * The epoch is 1772.76. The year is understood to have 366 days.
   * The fraction is counted from 1 Jan. 1772 */
  {2368547.66, 2431456.5, 0.0, 77.775, 0.3, 0.7, 0, 0},
  /* Nibiru, elements from Christian Woeltge, Hannover */
  {1856113.380954, 1856113.380954, 0.0, 234.8921, 0.981092, 103.966, -44.567, 158.708},
  /* Harrington, elements from Astronomical Journal 96(4), Oct. 1988 */
  {2374696.5, J2000, 0.0, 101.2, 0.411, 208.5, 275.4, 32.4},
  /* Leverrier's Neptune, 
	according to W.G. Hoyt, "Planets X and Pluto", Tucson 1980, p. 63 */
  {2395662.5, 2395662.5, 34.05, 36.15, 0.10761, 284.75, 0, 0}, 
  /* Adam's Neptune */
  {2395662.5, 2395662.5, 24.28, 37.25, 0.12062, 299.11, 0, 0}, 
  /* Lowell's Pluto */
  {2425977.5, 2425977.5, 281, 43.0, 0.202, 204.9, 0, 0}, 
  /* Pickering's Pluto */
  {2425977.5, 2425977.5, 48.95, 55.1, 0.31, 280.1, 100, 15}, /**/
#if 0	/* Ceres JPL 1600, without perturbations from other minor planets,
	 * from following initial elements: 
	 * 2450600.5 2000 0 1 164.7073602 73.0340746 80.5995101 
	 * 10.5840296 0.07652422 0.0 2.770176095 */
  {2305447.5, J2000, 0.5874558977449977e+02, 0.2766536058742327e+01, 
    0.7870946565779195e-01, 0.5809199028919189e+02, 
    0.8650119410725021e+02, 0.1066835622280712e+02}, 
  /* Chiron, Bowell database 18-mar-1997 */
  {2450500.5, J2000, 7.258191, 13.67387471, 0.38174778, 339.558345, 209.379239, 6.933360}, /**/
#endif
};

/* computes a planet from osculating elements *
 * tjd		julian day
 * ipl		body number
 * ipli 	body number in planetary data structure
 * iflag	flags
 */
/* restrict: xp accumulates from xearth and xsun and is never either of
 * them -- both call sites (sweph.c:1131, :3702) pass three distinct
 * plan_data vectors or three distinct locals. */
int swi_osc_el_plan(swe_ctx *ctx, double tjd, double * SWI_RESTRICT xp,
                    int ipl, int ipli, const double * SWI_RESTRICT xearth,
                    const double * SWI_RESTRICT xsun, char *serr)
{
  double pqr[9], x[6];
  double eps, fac, rho, cose, sine;
  double alpha, beta, zeta, sigma, M2, Msgn, M_180_or_0;
  double tjd0, tequ, mano, sema, ecce, parg, node, incl, dmot;
  double cosnode, sinnode, cosincl, sinincl, cosparg, sinparg;
  double M, E;
#ifdef SWE_UPSTREAM_COMPAT
  double K;
#else
  double drate[6], dM, dE, dsema, decce, dparg, dnode, dincl, dfac;
  double dP[3], dQ[3], xe[3], xr[3];
#endif
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *pdp = &ctx->pldat[ipli];
  int32 fict_ifl = 0;
  int i;
  /* orbital elements, either from file or, if file not found,
   * from above built-in set  
   */
#ifndef SWE_UPSTREAM_COMPAT
  if (read_elements_file(ctx, ipl, tjd, &tjd0, &tequ, 
       &mano, &sema, &ecce, &parg, &node, &incl, 
       NULL, &fict_ifl, drate, serr) == ERR)
    return ERR;
#else
  if (read_elements_file(ctx, ipl, tjd, &tjd0, &tequ, 
       &mano, &sema, &ecce, &parg, &node, &incl, 
       NULL, &fict_ifl, NULL, serr) == ERR)
    return ERR;
#endif
  dmot = 0.9856076686 * DEGTORAD / sema / sqrt(sema);	/* daily motion */
  if (fict_ifl & FICT_GEO)
    dmot /= sqrt(SUN_EARTH_MRAT);
  cosnode = cos(node);
  sinnode = sin(node);
  cosincl = cos(incl);
  sinincl = sin(incl); 
  cosparg = cos(parg);
  sinparg = sin(parg);
  /* Gaussian vector */
  pqr[0] = cosparg * cosnode - sinparg * cosincl * sinnode;
  pqr[1] = -sinparg * cosnode - cosparg * cosincl * sinnode;
  pqr[2] = sinincl * sinnode;
  pqr[3] = cosparg * sinnode + sinparg * cosincl * cosnode;
  pqr[4] = -sinparg * sinnode + cosparg * cosincl * cosnode;
  pqr[5] = -sinincl * cosnode;
  pqr[6] = sinparg * sinincl;
  pqr[7] = cosparg * sinincl;
  pqr[8] = cosincl;
  /* Kepler problem */
  E = M = swi_mod2PI(mano + (tjd - tjd0) * dmot); /* mean anomaly of date */
  /* better E for very high eccentricity and small M */
  if (ecce > 0.975) {
    M2 = M * RADTODEG;
    if (M2 > 150 && M2 < 210) {
      M2 -= 180;
#ifndef SWE_UPSTREAM_COMPAT
      M_180_or_0 = M_PI;
#else
      M_180_or_0 = 180;
#endif
    } else
      M_180_or_0 = 0;
    if (M2 > 330) 
      M2 -= 360;
    if (M2 < 0) {
      M2 = -M2;
      Msgn = -1;
    } else
      Msgn = 1;
    if (M2 < 30) {
      M2 *= DEGTORAD;
      alpha = (1 - ecce) / (4 * ecce + 0.5);
      beta = M2 / (8 * ecce + 1);
      /* Upstream bug 65: 1/3 is integer division, so zeta was pow(x, 0) = 1,
       * and M_180_or_0 added 180 to an angle in radians. The starting guess
       * was 1-based nonsense; swi_kepler() converged to the same root, so
       * no result moves beyond rounding (see UPSTREAM-BUGS 65). It is now
       * the cube root and pi. */
#ifndef SWE_UPSTREAM_COMPAT
      zeta = pow(beta + sqrt(beta * beta + alpha * alpha), 1.0 / 3);
#else
      zeta = pow(beta + sqrt(beta * beta + alpha * alpha), 1/3);
#endif
      sigma = zeta - alpha / 2;
      sigma = sigma - 0.078 * sigma * sigma * sigma * sigma * sigma / (1 + ecce);
      E = Msgn * (M2 + ecce * (3 * sigma - 4 * sigma * sigma * sigma))
			+ M_180_or_0;
    }
  }
  E = swi_kepler(E, M, ecce);
  /* position and speed, referred to orbital plane */
#ifdef SWE_UPSTREAM_COMPAT
  if (fict_ifl & FICT_GEO)
    K = KGAUSS_GEO / sqrt(sema);
  else
    K = KGAUSS / sqrt(sema);
#endif
  cose = cos(E);
  sine = sin(E);
  fac = sqrt((1 - ecce) * (1 + ecce));
  rho = 1 - ecce * cose;
  x[0] = sema * (cose - ecce);
  x[1] = sema * fac * sine;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 48: the speed was the two-body motion of a fixed ellipse,
   * K / sqrt(a) with K Gauss's constant, while the position follows the
   * element file's polynomials: a mean anomaly with its own rate (Vulcan's
   * 707550.7341 deg/cy, 19.3717 deg/day, against the 19.3428 of a = 0.13744
   * AU), and the semi-axis, eccentricity, perihelion, node and inclination
   * in T as well (Vulcan's node and perihelion 1670 deg/cy, a planet's
   * elements of date), and for an equinox of date (JDATE) a frame that
   * precesses with t. Vulcan's heliocentric longitude speed was off by
   * 2.9e-2 deg/day, Waldemath's geocentric by 1.3e-2, Mercury's set by
   * 9.4e-6, and every JDATE set by the precession, 3.8e-5. Even for a
   * fixed ellipse K and the position's daily motion were different
   * constants for an Earth-centred one (KGAUSS_GEO, 7.6e-8 off 0.9856..
   * / sqrt(SUN_EARTH_MRAT): 4.8e-7 deg/day on Selena). The speed is now
   * the derivative of the position as it is computed: dM/dt as
   * evaluated (the polynomial's, or the daily motion with the semi-axis's
   * rate in it), Kepler's equation differentiated, the elements' rates
   * through the Gaussian vectors, and the frame's turning below. G48. */
  dsema = drate[1];
  decce = drate[2];
  dparg = drate[3];
  dnode = drate[4];
  dincl = drate[5];
  if (fict_ifl & FICT_MANO_T)
    dM = drate[0];	/* M is the polynomial itself, epoch = tjd */
  else			/* M = mano + (tjd - tjd0) * dmot(a(t)) */
    dM = dmot - 1.5 * dmot * dsema / sema * (tjd - tjd0);
  dE = (dM + sine * decce) / rho;	/* E - e sin E = M */
  dfac = -ecce * decce / fac;
  x[3] = dsema * (cose - ecce) - sema * (sine * dE + decce);
  x[4] = dsema * fac * sine + sema * (dfac * sine + fac * cose * dE);
  /* d/dt of the Gaussian vectors P (pqr[0,3,6]) and Q (pqr[1,4,7]):
   * dP/dw = Q, dQ/dw = -P; about the node, (-y, x, 0); and through the
   * inclination, sin/cos w * (sin i sin N, -sin i cos N, cos i) */
  dP[0] = pqr[1] * dparg - pqr[3] * dnode + sinparg * sinincl * sinnode * dincl;
  dP[1] = pqr[4] * dparg + pqr[0] * dnode - sinparg * sinincl * cosnode * dincl;
  dP[2] = pqr[7] * dparg + sinparg * cosincl * dincl;
  dQ[0] = -pqr[0] * dparg - pqr[4] * dnode + cosparg * sinincl * sinnode * dincl;
  dQ[1] = -pqr[3] * dparg + pqr[1] * dnode - cosparg * sinincl * cosnode * dincl;
  dQ[2] = -pqr[6] * dparg + cosparg * cosincl * dincl;
#else
  x[3] = -K * sine / rho;
  x[4] = K * fac * cose / rho;
#endif
  /* transformation to ecliptic */
  xp[0] = pqr[0] * x[0] + pqr[1] * x[1];
  xp[1] = pqr[3] * x[0] + pqr[4] * x[1];
  xp[2] = pqr[6] * x[0] + pqr[7] * x[1];
  xp[3] = pqr[0] * x[3] + pqr[1] * x[4];
  xp[4] = pqr[3] * x[3] + pqr[4] * x[4];
  xp[5] = pqr[6] * x[3] + pqr[7] * x[4];
#ifndef SWE_UPSTREAM_COMPAT
  for (i = 0; i <= 2; i++) {
    xp[3+i] += dP[i] * x[0] + dQ[i] * x[1];
    xe[i] = xp[i];
  }
#endif
  /* transformation to equator */
  eps = swi_epsiln(ctx, tequ, 0);
  swi_coortrf(xp, xp, -eps);
  swi_coortrf(xp+3, xp+3, -eps);
  /* precess to J2000 */
  if (tequ != J2000) {
    swi_precess(ctx, xp, tequ, 0, J_TO_J2000);
    swi_precess(ctx, xp+3, tequ, 0, J_TO_J2000);
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Elements of the equinox of date: the frame turns with t, by the
   * obliquity's and the precession's rates. Its turning, applied to the
   * ecliptic position, as a central difference of the same rotation over
   * +-1 day: smooth polynomials and long-period series, so the
   * difference is exact to rounding (1e-16 of the 3.8e-5 deg/day). */
  if (fict_ifl & FICT_JDATE) {
    swi_coortrf(xe, xr, -swi_epsiln(ctx, tequ + 1, 0));
    swi_precess(ctx, xr, tequ + 1, 0, J_TO_J2000);
    swi_coortrf(xe, xe, -swi_epsiln(ctx, tequ - 1, 0));
    swi_precess(ctx, xe, tequ - 1, 0, J_TO_J2000);
    for (i = 0; i <= 2; i++)
      xp[3+i] += (xr[i] - xe[i]) / 2;
  }
#endif
#ifndef SWE_UPSTREAM_COMPAT
  /* J2000 (dynamical) to ICRS, before the Sun or Earth -- which are ICRS --
   * is added; app_pos_etc_plan_osc() takes the result back to J2000 as it
   * does a planet. Same condition on both sides, so the pair always
   * cancels for the body and rotates only what came from the ephemeris.
   * G27. */
  if (swi_get_denum(ctx, SEI_EARTH, pedp->iephe) >= 403)
    swi_bias(ctx, xp, tjd, SEFLG_SPEED, TRUE);
#endif
  /* to solar system barycentre */
  if (fict_ifl & FICT_GEO) {
    for (i = 0; i <= 5; i++) {
      xp[i] += xearth[i];
    }
  } else {
	for (i = 0; i <= 5; i++) {
	  xp[i] += xsun[i];
	}
  }
  if (pdp->x == xp) {
    pdp->teval = tjd;	/* for precession! */
    pdp->iephe = pedp->iephe;
  }
  return OK;
}

#if 1
void swi_free_fict_lines(swe_ctx *ctx)
{
  free(ctx->fict_lines);
  ctx->fict_lines = NULL;
  ctx->n_fict_lines = 0;
}

/* Read seorbel.txt once into ctx->fict_lines: every line that the per-body
 * scan below used to consider, after the same whitespace and comment
 * stripping. ERR when the file cannot be opened, with serr set by swi_fopen
 * exactly as before, so the built-in table is reached the same way. */
static int load_fict_lines(swe_ctx *ctx, char *serr)
{
  FILE *fp;
  char s[AS_MAXCH], *sp;
  int iline = 0, n = 0, cap = 0;
  struct fict_line *lines = NULL;
  /* -1: file information is not saved, the file is closed again below */
  if ((fp = swi_fopen(ctx, -1, SE_FICTFILE, ctx->ephepath, serr)) == NULL)
    return ERR;
  while (fgets(s, AS_MAXCH, fp) != NULL) {
    iline++;
    sp = s;
    while(*sp == ' ' || *sp == '\t')
      sp++;
    swi_strcpy(s, sp);
    if (*s == '#' || *s == '\r' || *s == '\n' || *s == '\0')
      continue;
    if ((sp = strchr(s, '#')) != NULL)
      *sp = '\0';
    if (n == cap) {
      struct fict_line *p = realloc(lines, (size_t) (cap + 16) * sizeof *lines);
      if (p == NULL) {
        free(lines);
        fclose(fp);
        if (serr != NULL)
          snprintf(serr, AS_MAXCH, "out of memory reading %.40s", SE_FICTFILE);
        return ERR;
      }
      lines = p;
      cap += 16;
    }
    lines[n].iline = iline;
    strcpy(lines[n].s, s);
    n++;
  }
  fclose(fp);
  if (lines == NULL)		/* empty file: still "loaded", not "absent" */
    lines = malloc(sizeof *lines);
  ctx->fict_lines = lines;
  ctx->n_fict_lines = n;
  return OK;
}

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 113: a field that must be a number is one, whole and
 * finite, blanks around it allowed */
static int field_number(const char *sp, double *d)
{
  char *end;
  while (*sp == ' ' || *sp == '\t')
    sp++;
  *d = strtod(sp, &end);
  if (end == sp)
    return 0;
  while (*end != '\0' && strchr(" \t\r\n", *end) != NULL)
    end++;
  return *end == '\0' && isfinite(*d);
}
#endif

/* note: input parameter tjd is required for T terms in elements */
static int read_elements_file(swe_ctx *ctx, int32 ipl, double tjd,
  double *tjd0, double *tequ,
  double *mano, double *sema, double *ecce,
  double *parg, double *node, double *incl,
  char *pname, int32 *fict_ifl, double *drate, char *serr)
{
  int i, k, iplan, retc, ncpos;
  char s[AS_MAXCH], *sp;
  char *cpos[20], serri[AS_MAXCH];
  AS_BOOL elem_found = FALSE;
  double tt = 0, ttlo = 0;
  if (ctx->fict_lines == NULL && load_fict_lines(ctx, serr) == ERR) {
    /* file does not exist, use built-in bodies */
    if (ipl >= SE_NFICT_ELEM) {
      if (serr != NULL)
        sprintf(serr, "error no elements for fictitious body no %7.0f", (double) ipl);
      return ERR;
    }
    if (tjd0 != NULL)
      *tjd0 = plan_oscu_elem[ipl][0];			/* epoch */
    if (tequ != NULL)
      *tequ = plan_oscu_elem[ipl][1];			/* equinox */
    if (mano != NULL)
      *mano = plan_oscu_elem[ipl][2] * DEGTORAD;	/* mean anomaly */
    if (sema != NULL)
      *sema = plan_oscu_elem[ipl][3];			/* semi-axis */
    if (ecce != NULL)
      *ecce = plan_oscu_elem[ipl][4];			/* eccentricity */
    if (parg != NULL)
      *parg = plan_oscu_elem[ipl][5] * DEGTORAD;	/* arg. of peri. */
    if (node != NULL)
      *node = plan_oscu_elem[ipl][6] * DEGTORAD;	/* asc. node */
    if (incl != NULL)
      *incl = plan_oscu_elem[ipl][7] * DEGTORAD;	/* inclination */
    if (pname != NULL)
      strcpy(pname, plan_fict_nam[ipl]);
    if (drate != NULL)
      for (i = 0; i < 6; i++)
        drate[i] = 0;	/* constant elements, fixed equinox */
    return OK;
  }
  /*
   * find elements in the cached file
   */
  iplan = -1;
  for (k = 0; k < ctx->n_fict_lines; k++) {
    strcpy(s, ctx->fict_lines[k].s);	/* swi_cutstr() writes into it */
    ncpos = swi_cutstr(s, ",", cpos, 20);
    snprintf(serri, AS_MAXCH, "error in file %.40s, line %7.0f:", SE_FICTFILE, (double) ctx->fict_lines[k].iline);
    if (ncpos < 9) {
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 113: the scan stopped at a short line, so every body
       * after it was refused too. It is one body's line, numbered as the
       * others; only that body is refused. */
      if (++iplan != ipl)
        continue;
#endif
      if (serr != NULL) {
        snprintf(serr, AS_MAXCH, "%.200s nine elements required", serri);
      }
      goto return_err;
    }
    iplan++;
    if (iplan != ipl)
      continue;
    elem_found = TRUE;
    /* epoch of elements */
    if (tjd0 != NULL) {
      sp = cpos[0];
	  for (i = 0; i < 5 && sp[i] != '\0'; i++)	/* not into the next field */
       sp[i] = tolower(sp[i]);
      if (strncmp(sp, "j2000", 5) == OK)
        *tjd0 = J2000;
      else if (strncmp(sp, "b1950", 5) == OK)
        *tjd0 = B1950;
      else if (strncmp(sp, "j1900", 5) == OK)
        *tjd0 = J1900;
      else if (*sp == 'j' || *sp == 'b') {
        if (serr != NULL) {
          snprintf(serr, AS_MAXCH, "%.200s invalid epoch", serri);
	}
        goto return_err;
#ifndef SWE_UPSTREAM_COMPAT
      } else if (!field_number(sp, tjd0)) {
        /* Upstream bug 113: atof() made any text, or nothing, JD 0 */
        if (serr != NULL)
          snprintf(serr, AS_MAXCH, "%.200s invalid epoch", serri);
        goto return_err;
      }
#else
      } else
        *tjd0 = atof(sp);
#endif
      tt = tjd - *tjd0;
#ifndef SWE_UPSTREAM_COMPAT
      {	/* the rounding of tt, exact (TwoSum), for check_t_terms() */
        double bb = tt - tjd;
        ttlo = (tjd - (tt - bb)) + (-*tjd0 - bb);
      }
#endif
    }
    /* equinox */
    if (tequ != NULL) {
      sp = cpos[1];
      while(*sp == ' ' || *sp == '\t')
        sp++;
	  for (i = 0; i < 5 && sp[i] != '\0'; i++)
       sp[i] = tolower(sp[i]);
      if (strncmp(sp, "j2000", 5) == OK)
        *tequ = J2000;
      else if (strncmp(sp, "b1950", 5) == OK)
        *tequ = B1950;
      else if (strncmp(sp, "j1900", 5) == OK)
        *tequ = J1900;
      else if (strncmp(sp, "jdate", 5) == OK) {
        *tequ = tjd;
#ifndef SWE_UPSTREAM_COMPAT
        if (fict_ifl != NULL)
          *fict_ifl |= FICT_JDATE;
#endif
      }
      else if (*sp == 'j' || *sp == 'b') {
        if (serr != NULL) {
          snprintf(serr, AS_MAXCH, "%.200s invalid equinox", serri);
	}
        goto return_err;
#ifndef SWE_UPSTREAM_COMPAT
      } else if (!field_number(sp, tequ)) {
        if (serr != NULL)
          snprintf(serr, AS_MAXCH, "%.200s invalid equinox", serri);
        goto return_err;
      }
#else
      } else
        *tequ = atof(sp);
#endif
    }
    /* mean anomaly t0 */
    if (mano != NULL) {
      retc = check_t_terms(tt, ttlo, 360, cpos[2], mano, drate);
	  *mano = swe_degnorm(*mano);
      if (retc == ERR) {
        if (serr != NULL) {
          snprintf(serr, AS_MAXCH, "%.200s mean anomaly value invalid", serri);
	}
        goto return_err;
      }
      /* if mean anomaly has t terms (which happens with fictitious 
       * planet Vulcan), we set
       * epoch = tjd, so that no motion will be added anymore 
       * equinox = tjd */
      if (retc == 1) {
	*tjd0 = tjd;
#ifndef SWE_UPSTREAM_COMPAT
	if (fict_ifl != NULL)
	  *fict_ifl |= FICT_MANO_T;
#endif
      }
      *mano *= DEGTORAD;
      if (drate != NULL)
        drate[0] *= DEGTORAD;
    }
    /* semi-axis */
    if (sema != NULL) {
      retc = check_t_terms(tt, ttlo, 0, cpos[3], sema, drate != NULL ? drate + 1 : NULL);
      if (*sema <= 0 || retc == ERR) {
        if (serr != NULL) {
          snprintf(serr, AS_MAXCH, "%.200s semi-axis value invalid", serri);
	}
        goto return_err;
      }
    }
    /* eccentricity */
    if (ecce != NULL) {
      retc = check_t_terms(tt, ttlo, 0, cpos[4], ecce, drate != NULL ? drate + 2 : NULL);
      if (*ecce >= 1 || *ecce < 0 || retc == ERR) {
        if (serr != NULL) {
          snprintf(serr, AS_MAXCH, "%.150s eccentricity invalid (no parabolic or hyperbolic orbits allowed)", serri);
	}
        goto return_err;
      }
    }
    /* perihelion argument */
    if (parg != NULL) {
      retc = check_t_terms(tt, ttlo, 360, cpos[5], parg, drate != NULL ? drate + 3 : NULL);
	  *parg = swe_degnorm(*parg);
      if (retc == ERR) {
        if (serr != NULL) {
          snprintf(serr, AS_MAXCH, "%.200s perihelion argument value invalid", serri);
	}
        goto return_err;
      }
      *parg *= DEGTORAD;
      if (drate != NULL)
        drate[3] *= DEGTORAD;
    }
    /* node */
    if (node != NULL) {
      retc = check_t_terms(tt, ttlo, 360, cpos[6], node, drate != NULL ? drate + 4 : NULL);
	  *node = swe_degnorm(*node);
      if (retc == ERR) {
        if (serr != NULL) {
          snprintf(serr, AS_MAXCH, "%.200s node value invalid", serri);
	}
        goto return_err;
      }
      *node *= DEGTORAD;
      if (drate != NULL)
        drate[4] *= DEGTORAD;
    }
    /* inclination */
    if (incl != NULL) {
      retc = check_t_terms(tt, ttlo, 360, cpos[7], incl, drate != NULL ? drate + 5 : NULL);
	  *incl = swe_degnorm(*incl);
      if (retc == ERR) {
        if (serr != NULL) {
          snprintf(serr, AS_MAXCH, "%.200s inclination value invalid", serri);
	}
        goto return_err;
      }
      *incl *= DEGTORAD;
      if (drate != NULL)
        drate[5] *= DEGTORAD;
    }
    /* planet name */
    if (pname != NULL) {
      sp = cpos[8];
      while(*sp == ' ' || *sp == '\t')
        sp++;
      swi_right_trim(sp);
      strcpy(pname, sp);
    }
    /* geocentric */
    if (fict_ifl != NULL && ncpos > 9) {
      for (sp = cpos[9]; *sp != '\0'; sp++)
        *sp = tolower(*sp);
      if (strstr(cpos[9], "geo") != NULL)
        *fict_ifl |= FICT_GEO;
    }
    break;
  }
  if (!elem_found) {
    if (serr != NULL) {
      snprintf(serr, AS_MAXCH, "%.150s elements for planet %7.0f not found", serri, (double) ipl);
    }
    goto return_err;
  }
  return OK;
return_err:
  return ERR;
}
#endif

/* Upstream bug 48 (rate part): drate, when not NULL, receives d/dt of the
 * polynomial, per day -- each term's factor carried with its derivative
 * through the products, so a term like 3 * T * T2 differentiates too. The
 * value is formed exactly as before. */
/* Upstream bug 81 (part 2): an element's linear term n T was formed as n
 * times T = t / 36525 and the sum reduced to 0..360 afterwards, so its
 * rounding was that of the unreduced angle: Vulcan's mean anomaly is
 * 707550.7341 T, 1.6e7 deg at -200 and 3.5e7 at -3000, an ulp of 1.9e-9
 * and 3.7e-9 deg, and the rounding of T and of the product as much again.
 * Its true heliocentric places jittered by 1.2e-8 deg at -200 and 3.0e-8 at
 * -3000 between instants 2^-9 day apart (fourth differences), 4.3e-7 and
 * 2.4e-6 deg/day in a five-point difference. For an angle (mod = 360) a
 * term in T alone is now (n / 36525) (t + tlo), the rate split in two
 * doubles and the product by fma, its whole revolutions taken out exactly
 * before it is added: the angle within a few ulps of 360 deg. tlo is the
 * rounding of t = tjd - epoch. The value differs from before by the old
 * rounding only, and the rate (drate) is the analytic one as before. */
static int check_t_terms(double t, double tlo, double mod, char *sinp, double *doutp, double *drate)
{
  int isgn = 1, z;
#ifdef SWE_UPSTREAM_COMPAT
  int i;
#endif
  int retc = 0;
  char *sp;
  double tt[5], fac;
  double dtt[5], dfac;
#ifndef SWE_UPSTREAM_COMPAT
  double cnum = 1;	/* the term's numbers and sign */
  int pw = 0;		/* its power of T */
#else
  (void) tlo;
  (void) mod;
#endif
  tt[0] = t / 36525;
  tt[1] = tt[0];
  tt[2] = tt[1] * tt[1];
  tt[3] = tt[2] * tt[1];
  tt[4] = tt[3] * tt[1];
  dtt[0] = dtt[1] = 1.0 / 36525;
  dtt[2] = 2 * tt[1] / 36525;
  dtt[3] = 3 * tt[2] / 36525;
  dtt[4] = 4 * tt[3] / 36525;
  /* Upstream bug 60: a '-' (or '+') anywhere made the field a polynomial, so
   * a plain negative mean anomaly such as "-12.5" set epoch = tjd and the
   * body stood still (daily motion 0 against 0.2 deg/day on a = 1 AU).
   * Now only a T makes it one; a leading sign is a sign. */
#ifndef SWE_UPSTREAM_COMPAT
  if ((sp = strpbrk(sinp, "tT")) != NULL)
    retc = 1; /* with T terms */
#else
  if ((sp = strpbrk(sinp, "+-")) != NULL)
    retc = 1; /* with additional terms */
#endif
  sp = sinp;
  *doutp = 0;
  if (drate != NULL)
    *drate = 0;
  fac = 1;
  dfac = 0;
  z = 0;
  while (1) {
    while(*sp != '\0' && strchr(" \t", *sp) != NULL)
      sp++;
    if (strchr("+-", *sp) || *sp == '\0') {
	  if (z > 0) {
#ifndef SWE_UPSTREAM_COMPAT
		if (mod > 0 && pw == 1) {
		  /* n T, exact to rounding of the reduced angle */
		  double rhi = cnum / 36525;
		  double rlo = fma(-rhi, 36525, cnum) / 36525;
		  double p = rhi * t;
		  double e = fma(rhi, t, -p) + rhi * tlo + rlo * t;
		  p -= mod * floor(p / mod);	/* exact: 360 k is a multiple of ulp(p) */
		  *doutp += p + e;
		} else
#endif
		*doutp += fac;
		if (drate != NULL)
		  *drate += dfac;
	  }
	  isgn = 1;
	  if (*sp == '-')
	    isgn = -1;
	  fac = 1 * isgn;
	  dfac = 0;
#ifndef SWE_UPSTREAM_COMPAT
	  cnum = isgn;
	  pw = 0;
#endif
	  if (*sp == '\0') {
#ifndef SWE_UPSTREAM_COMPAT
		/* Upstream bug 113: an empty field was the value 0 */
		if (z == 0)
		  return ERR;
#endif
		return retc;
	  }
	  sp++;
	} else {
      char *sp0 = sp;
      while(*sp != '\0' && strchr("* \t", *sp) != NULL)
        sp++;
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 114: a power after T was atoi(), which skips blanks
       * and returns 0 for none -- "T5" to "T9" (and "T12") were ignored,
       * the term read as its factors without them, "T0" read as T and
       * "T 2" as T2 followed by a factor 2; a number was taken only if
       * atof() was not 0 or it began with '0', so ".0" was skipped and the
       * term read as if its factor were 1, and "1.2.3" as 1.2. Now T is
       * followed by nothing (T) or by the digits of a power, 1 to 4 (the
       * documented T2, T3 and the T4 the code has); a number is its whole
       * run of digits and points, read as its value. Anything else refuses
       * the field. */
      if (*sp != '\0' && strchr("tT", *sp) != NULL) {
        int n = 1;
        sp++;
        if (*sp >= '0' && *sp <= '9') {
          char *end;
          long ln = strtol(sp, &end, 10);
          if (ln < 1 || ln > 4 || *end == '.')
            return ERR;
          n = (int) ln;
          sp = end;
        }
        dfac = dfac * tt[n] + fac * dtt[n];
        fac *= tt[n];
        pw += n;
      } else if (*sp != '\0' && strchr("0123456789.", *sp) != NULL) {
        char num[AS_MAXCH], *end;
        size_t len = strspn(sp, "0123456789.");
        double v;
        if (len >= sizeof num)
          return ERR;
        memcpy(num, sp, len);
        num[len] = '\0';
        v = strtod(num, &end);
        if (end != num + len)
          return ERR;	/* ".", "1.2.3" */
        dfac *= v;
        fac *= v;
        cnum *= v;
        sp += len;
      }
#else
      if (*sp != '\0' && strchr("tT", *sp) != NULL) {
		/* a T */
        sp++;
        if (*sp != '\0' && strchr("+-", *sp)) {
		  dfac = dfac * tt[0] + fac * dtt[0];
		  fac *= tt[0];
	    } else if ((i = atoi(sp)) <= 4 && i >= 0) {
		  dfac = dfac * tt[i] + fac * dtt[i];
          fac *= tt[i];
	    }
	  } else {
        /* a number */
        if (atof(sp) != 0 || *sp == '0') {
          dfac *= atof(sp);
          fac *= atof(sp);
	    }
	  }
#endif
      while (*sp != '\0' && strchr("0123456789.", *sp))
	    sp++;
      /* Upstream bug 113: a character that is no sign, blank, '*', T or
       * digit -- "12x", "1e5", "nan" -- was consumed by nothing, and the
       * loop came back to it for ever (swe_calc() hung). Every field that
       * ends is read as before; this one is refused. */
      if (sp == sp0)
        return ERR;
	}
	z++;
  }
  return retc;	/* there have been additional terms */
}
