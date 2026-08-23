
/************************************************************

  Authors: Dieter Koch and Alois Treindl, Astrodienst Zurich

************************************************************/
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

/* swe_ctx is defined in sweph.h, which this header does not include -- and
 * swetest.c/swevents.c include this one first. An incomplete type is all the
 * prototypes below need, so forward-declare it (swejpl.h does the same). */
#include <math.h>       /* floor(), for swi_mods3600() below */
#include "swethread.h" /* SWI_INLINE */

struct swe_ctx;
typedef struct swe_ctx swe_ctx;

/* Reduce an angle in arcseconds modulo a full circle (1296000").
 *
 * Was a macro in swemplan.c substituting its argument TWICE -- the classic
 * SQR(x++) shape (notes/C17_PERFORMANCE.md A2). Every call today passes
 * plain arithmetic, so nothing is broken, but the hazard is live: one
 * future mods3600(next_value()) would silently evaluate it twice.
 *
 * swemmoon.c had already made its own copy a real function, independently.
 * This is that function, shared, so the two cannot drift. */
SWI_INLINE double swi_mods3600(double x)
{
  return x - 1296000.0 * floor(x / 1296000.0);
}

#define PREC_IAU_1976_CTIES          2.0 	/* J2000 +/- two centuries */
#define PREC_IAU_2000_CTIES          2.0 	/* J2000 +/- two centuries */
/* we use P03 for whole ephemeris */
#define PREC_IAU_2006_CTIES          75.0 	/* J2000 +/- 75 centuries */

/* For reproducing JPL Horizons to 2 mas (SEFLG_JPLHOR): 
 * The user has to keep the following files up to date which contain
 * the earth orientation parameters related to the IAU 1980 nutation
 * theory. 
 * Download the file 
 * datacenter.iers.org/eop/-/somos/5Rgv/document/tx13iers.u24/eopc04_08.62-now
 * and rename it as eop_1962_today.txt. For current data and estimations for
 * the near future, also download maia.usno.navy.mil/ser7/finals.all and 
 * rename it as eop_finals.txt */
#define DPSI_DEPS_IAU1980_FILE_EOPC04   "eop_1962_today.txt"
#define DPSI_DEPS_IAU1980_FILE_FINALS   "eop_finals.txt"
#define DPSI_DEPS_IAU1980_TJD0_HORIZONS  2437684.5 
#define HORIZONS_TJD0_DPSI_DEPS_IAU1980  2437684.5 
#define DPSI_IAU1980_TJD0	(64.284 / 1000.0)  // arcsec
#define DEPS_IAU1980_TJD0	(6.151 / 1000.0)   // arcsec

/* The above files must be available in order to reproduce JPL Horizons 
 * in agreement with IERS Conventions 1996 (1992), p. 22. 
 * Call swe_calc_ut() with iflag|SEFLG_JPLHOR.  
 * This options works only, if the files DPSI_DEPS_IAU1980_FILE_EOPC04 
 * and DPSI_DEPS_IAU1980_FILE_FINALS are in the ephemeris path.
 *
 * If the software does not find the earth orientation files 
 * in the ephemeris path, then SEFLG_JPLHOR will run as 
 * SEFLG_JPLHOR_APPROX.
 */

/* coordinate transformation */
extern void swi_coortrf(double *xpo, double *xpn, double eps);

/* coordinate transformation */
extern void swi_coortrf2(double *xpo, double *xpn, double sineps, double coseps);

/* cartesian to polar coordinates */
extern void swi_cartpol(double *x, double *l);
 
/* cartesian to polar coordinates with velocity */
extern void swi_cartpol_sp(double *x, double *l);
extern void swi_polcart_sp(double *l, double *x);
 
/* polar to cartesian coordinates */
extern void swi_polcart(double *l, double *x);

/* GCRS to J2000 */
extern void swi_bias(swe_ctx *ctx, double *x, double tjd, int32 iflag, AS_BOOL backward);
extern void swi_get_eop_time_range(void);
/* GCRS to FK5 */
extern void swi_icrs2fk5(double *x, int32 iflag, AS_BOOL backward);

/* precession */
extern int swi_precess(swe_ctx *ctx, double *R, double J, int32 iflag, int direction );
extern void swi_precess_speed(swe_ctx *ctx, double *xx, double t, int32 iflag, int direction);

extern int32 swi_guess_ephe_flag(swe_ctx *ctx);

/* from sweph.c, light deflection, aberration, etc. */
extern void swi_deflect_light(swe_ctx *ctx, double *xx, double dt, int32 iflag);
extern void swi_aberr_light(double *xx, double *xe, int32 iflag);
extern int swi_plan_for_osc_elem(swe_ctx *ctx, int32 iflag, double tjd, double *xx);
extern int swi_trop_ra2sid_lon(swe_ctx *ctx, double *xin, double *xout, double *xoutr, int32 iflag);
extern int swi_trop_ra2sid_lon_sosy(swe_ctx *ctx, const double * SWI_RESTRICT xin,
                                    double * SWI_RESTRICT xout, int32 iflag);
extern int swi_get_observer(swe_ctx *ctx, double tjd, int32 iflag, 
	AS_BOOL do_save, double *xobs, char *serr);
extern void swi_force_app_pos_etc(swe_ctx *ctx);

/* obliquity of ecliptic */
extern void swi_check_ecliptic(swe_ctx *ctx, double tjd, int32 iflag);
extern double swi_epsiln(swe_ctx *ctx, double J, int32 iflag);
extern void swi_ldp_peps(double J, double *dpre, double *deps);

/* nutation */
extern void swi_check_nutation(swe_ctx *ctx, double tjd, int32 iflag);
extern int swi_nutation(swe_ctx *ctx, double J, int32 iflag, double *nutlo);
extern void swi_nutate(swe_ctx *ctx, double *xx, int32 iflag, AS_BOOL backward);

extern void swi_mean_lunar_elements(swe_ctx *ctx, double tjd, 
							 double *node, double *dnode, 
							 double *peri, double *dperi);
/* */
extern double swi_mod2PI(double x);

/* evaluation of chebyshew series and derivative */
extern double swi_echeb(double x, double *coef, int ncf);
extern double swi_edcheb(double x, double *coef, int ncf);

/* cross product of vectors */
extern void swi_cross_prod(double *a, double *b, double *x);
/* dot product of vecotrs */
extern double swi_dot_prod_unit(double *x, double *y);

extern double swi_angnorm(double x);

/* generation of SWISSEPH file names */
extern void swi_gen_filename(double tjd, int ipli, char *fname);

/* cyclic redundancy checksum (CRC), 32 bit */
extern uint32 swi_crc32(unsigned char *buf, int len);
extern void swi_seed_leap_table(swe_ctx *ctx);
extern void swi_seed_dt_table(swe_ctx *ctx);

extern int swi_cutstr(char *s, char *cutlist, char *cpos[], int nmax);
extern char *swi_right_trim(char *s);

extern double swi_kepler(double E, double M, double ecce);

extern char *swi_get_fict_name(swe_ctx *ctx, int32 ipl, char *s);

extern void swi_FK4_FK5(double *xp, double tjd);

extern char *swi_strcpy(char *to, char *from);
extern char *swi_strncpy(char *to, char *from, size_t n);

extern double swi_deltat_ephe(double tjd_ut, int32 epheflag);

#ifdef TRACE
#  define TRACE_COUNT_MAX         10000
  /* Process-wide, not per-thread: see the comment on the definitions in
   * swephlib.c. Read or written only while holding the trace lock. */
  extern FILE *swi_fp_trace_c;
  extern FILE *swi_fp_trace_out;
  extern int32 swi_trace_count;
  extern void swi_open_trace(char *serr);
  /* Bracket one whole trace record, so concurrent threads cannot shred
   * each other's output. swi_open_trace() takes and releases the lock for
   * its own work (the counter bump and the lazy fopen); every record
   * emission takes it again around the writes themselves. Records are
   * emitted both with and without a preceding swi_open_trace(), so the
   * bracketing has to live at the record, not be handed over from open. */
  extern void swi_trace_lock(void);
  extern void swi_trace_unlock(void);
  static const char *fname_trace_c = "swetrace.c";
  static const char *fname_trace_out = "swetrace.txt";
#ifdef FORCE_IFLAG
  static const char *fname_force_flg = "force.flg";
#endif
#endif /* TRACE */
