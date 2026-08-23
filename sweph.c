/* SWISSEPH

   Ephemeris computations

  Authors: Dieter Koch and Alois Treindl, Astrodienst Zurich

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


#include <assert.h>
#include <string.h>
#include <ctype.h>
#if MSDOS
#include <tchar.h>
#include <windows.h>
#endif
#include "swejpl.h"
#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"

#ifdef _MSC_VER
#define CMP_CALL_CONV __cdecl
#else
#define CMP_CALL_CONV
#endif

#define IS_PLANET 		0
#define IS_MOON			1
#define IS_ANY_BODY		2
#define IS_MAIN_ASTEROID	3

#define DO_SAVE			TRUE
#define NO_SAVE			FALSE

#define SEFLG_EPHMASK	(SEFLG_JPLEPH|SEFLG_SWIEPH|SEFLG_MOSEPH)
#define SEFLG_COORDSYS  (SEFLG_EQUATORIAL | SEFLG_XYZ | SEFLG_RADIANS)

struct meff_ele {double r,m;};

/****************
 * global stuff *
 ****************/
/* Designated initializer (C99+, confirmed safe -- see notes/C17_MIGRATION.md
 * §3/§9): every swe_data member is FALSE/0/0.0/NULL/"" by default except
 * const_lapse_rate, so this is behaviour-identical to the field-by-field
 * positional initializer it replaces, without that initializer's ordering
 * hazard (a member inserted anywhere but the very end used to be silently
 * zero-filled with no diagnostic; a member renamed here fails to compile
 * instead). */
TLS struct swe_ctx swed = {
  .const_lapse_rate = SE_LAPSE_RATE,
  /* 99 is the "no declination recorded yet" sentinel for Sunshine houses;
   * 0 would be a legitimate declination. */
  .saved_sundec     = 99,
};

/*************
 * constants *
 *************/

static const char * const ayanamsa_name[] = {
   "Fagan/Bradley",                    /*  0 SE_SIDM_FAGAN_BRADLEY */
   "Lahiri",                           /*  1 SE_SIDM_LAHIRI */
   "De Luce",                          /*  2 SE_SIDM_DELUCE */
   "Raman",                            /*  3 SE_SIDM_RAMAN */
   "Usha/Shashi",                      /*  4 SE_SIDM_USHASHASHI */
   "Krishnamurti",                     /*  5 SE_SIDM_KRISHNAMURTI */
   "Djwhal Khul",                      /*  6 SE_SIDM_DJWHAL_KHUL */
   "Yukteshwar",                       /*  7 SE_SIDM_YUKTESHWAR */
   "J.N. Bhasin",                      /*  8 SE_SIDM_JN_BHASIN */
   "Babylonian/Kugler 1",              /*  9 SE_SIDM_BABYL_KUGLER1 */
   "Babylonian/Kugler 2",              /* 10 SE_SIDM_BABYL_KUGLER2 */
   "Babylonian/Kugler 3",              /* 11 SE_SIDM_BABYL_KUGLER3 */
   "Babylonian/Huber",                 /* 12 SE_SIDM_BABYL_HUBER */
   "Babylonian/Eta Piscium",           /* 13 SE_SIDM_BABYL_ETPSC */
   "Babylonian/Aldebaran = 15 Tau",    /* 14 SE_SIDM_ALDEBARAN_15TAU */
   "Hipparchos",                       /* 15 SE_SIDM_HIPPARCHOS */
   "Sassanian",                        /* 16 SE_SIDM_SASSANIAN */
   "Galact. Center = 0 Sag",           /* 17 SE_SIDM_GALCENT_0SAG */
   "J2000",                            /* 18 SE_SIDM_J2000 */
   "J1900",                            /* 19 SE_SIDM_J1900 */
   "B1950",                            /* 20 SE_SIDM_B1950 */
   "Suryasiddhanta",                   /* 21 SE_SIDM_SURYASIDDHANTA */
   "Suryasiddhanta, mean Sun",         /* 22 SE_SIDM_SURYASIDDHANTA_MSUN */
   "Aryabhata",                        /* 23 SE_SIDM_ARYABHATA */
   "Aryabhata, mean Sun",              /* 24 SE_SIDM_ARYABHATA_MSUN */
   "SS Revati",                        /* 25 SE_SIDM_SS_REVATI */
   "SS Citra",                         /* 26 SE_SIDM_SS_CITRA */
   "True Citra",                       /* 27 SE_SIDM_TRUE_CITRA */
   "True Revati",                      /* 28 SE_SIDM_TRUE_REVATI */
   "True Pushya (PVRN Rao)",           /* 29 SE_SIDM_TRUE_PUSHYA */
   "Galactic Center (Gil Brand)",      /* 30 SE_SIDM_GALCENT_RGILBRAND */
   "Galactic Equator (IAU1958)",       /* 31 SE_SIDM_GALEQU_IAU1958 */
   "Galactic Equator",                 /* 32 SE_SIDM_GALEQU_TRUE */
   "Galactic Equator mid-Mula",        /* 33 SE_SIDM_GALEQU_MULA */
   "Skydram (Mardyks)",                /* 34 SE_SIDM_GALALIGN_MARDYKS */
   "True Mula (Chandra Hari)",         /* 35 SE_SIDM_TRUE_MULA */
   "Dhruva/Gal.Center/Mula (Wilhelm)", /* 36 SE_SIDM_GALCENT_MULA_WILHELM */
   "Aryabhata 522",                    /* 37 SE_SIDM_ARYABHATA_522 */
   "Babylonian/Britton",               /* 38 SE_SIDM_BABYL_BRITTON */
   "\"Vedic\"/Sheoran",                /* 39 SE_SIDM_TRUE_SHEORAN */
   "Cochrane (Gal.Center = 0 Cap)",    /* 40 SE_SIDM_GALCENT_COCHRANE */
   "Galactic Equator (Fiorenza)",      /* 41 SE_SIDM_GALEQU_FIORENZA */
   "Vettius Valens",                   /* 42 SE_SIDM_VALENS_MOON */
   "Lahiri 1940",                      /* 43 SE_SIDM_LAHIRI_1940 */
   "Lahiri VP285",                     /* 44 SE_SIDM_LAHIRI_VP285 */
   "Krishnamurti-Senthilathiban",      /* 45 SE_SIDM_KRISHNAMURTI_VP291 */
   "Lahiri ICRC",                      /* 46 SE_SIDM_LAHIRI_ICRC */
   /*"Manjula/Laghumanasa",*/
};
static const int pnoint2jpl[]   = PNOINT2JPL;

/* Indices 10-13 (SE_MEAN_NODE/TRUE_NODE/MEAN_APOG/OSCU_APOG) and 21-22
 * (SE_INTP_APOG/INTP_PERG) are 0 placeholders, not real SEI_* indices --
 * those bodies live in swed.nddat[], not swed.pldat[], and are dispatched
 * to their own branches before any code indexes this array with them.
 * 21-22 were missing entirely until C17_PERFORMANCE.md's B1 finding: the
 * array had 21 entries against SE_NPLANETS == 23, latent because nothing
 * currently reaches pnoext2int[21] or [22] -- one branch-reorder away
 * from a real out-of-bounds read. The static_assert below is what would
 * have caught it. */
static const int pnoext2int[] = {SEI_SUN, SEI_MOON, SEI_MERCURY, SEI_VENUS, SEI_MARS, SEI_JUPITER, SEI_SATURN, SEI_URANUS, SEI_NEPTUNE, SEI_PLUTO, 0, 0, 0, 0, SEI_EARTH, SEI_CHIRON, SEI_PHOLUS, SEI_CERES, SEI_PALLAS, SEI_JUNO, SEI_VESTA, 0, 0, };
static_assert(sizeof(pnoext2int) / sizeof(pnoext2int[0]) == SE_NPLANETS,
    "pnoext2int[] must cover every SE_* body number 0..SE_NPLANETS-1 -- see C17_PERFORMANCE.md B1");

static int32 swecalc(swe_ctx *ctx, double tjd, int ipl, int iplmoon, int32 iflag, double *x, char *serr);
static int do_fread(swe_ctx *ctx, void *targ, int size, int count, int corrsize, 
		    FILE *fp, int32 fpos, int freord, int fendian, int ifno, 
		    char *serr);
static int get_new_segment(swe_ctx *ctx, double tjd, int ipli, int ifno, char *serr);
static int main_planet(swe_ctx *ctx, double tjd, int ipli, int iplmoon, int32 epheflag, int32 iflag,
		       char *serr);
static int main_planet_bary(swe_ctx *ctx, double tjd, int ipli, int32 epheflag, int32 iflag, 
		AS_BOOL do_save, 
		double *xp, double *xe, double *xs, double *xm, 
		char *serr);
static int sweplan(swe_ctx *ctx, double tjd, int ipli, int ifno, int32 iflag, AS_BOOL do_save, 
		   double *xp, double *xpe, double *xps, double *xpm,
		   char *serr);
static int swemoon(swe_ctx *ctx, double tjd, int32 iflag, AS_BOOL do_save, double *xp, char *serr);
static int sweph(swe_ctx *ctx, double tjd, int ipli, int ifno, int32 iflag, double *xsunb, AS_BOOL do_save, 
		double *xp, char *serr);
static int jplplan(swe_ctx *ctx, double tjd, int ipli, int32 iflag, AS_BOOL do_save,
		   double *xp, double *xpe, double *xps, char *serr);
static void rot_back(swe_ctx *ctx, int ipl);
static int read_const(swe_ctx *ctx, int ifno, char *serr);
static void embofs(double * SWI_RESTRICT xemb, const double * SWI_RESTRICT xmoon);
static int app_pos_etc_plan(swe_ctx *ctx, int ipli, int iplmoon, int32 iflag, char *serr);
static int app_pos_etc_plan_osc(swe_ctx *ctx, int ipl, int ipli, int32 iflag, char *serr);
static int app_pos_etc_sun(swe_ctx *ctx, int32 iflag, char *serr);
static int app_pos_etc_moon(swe_ctx *ctx, int32 iflag, char *serr);
static int app_pos_etc_sbar(swe_ctx *ctx, int32 iflag, char *serr);
extern int swi_plan_for_osc_elem(swe_ctx *ctx, int32 iflag, double tjd, double *xx);
static void swi_close_keep_topo_etc(swe_ctx *ctx); 
static int app_pos_etc_mean(swe_ctx *ctx, int ipl, int32 iflag, char *serr);
static void nut_matrix(struct nut *nu, struct epsilon *oec); 
static void calc_epsilon(swe_ctx *ctx, double tjd, int32 iflag, struct epsilon *e);
static int lunar_osc_elem(swe_ctx *ctx, double tjd, int ipl, int32 iflag, char *serr);
static int intp_apsides(swe_ctx *ctx, double tjd, int ipl, int32 iflag, char *serr); 
static double meff(double r);
static void denormalize_positions(double *x0, double *x1, double *x2);
static void calc_speed(double *x0, double *x1, double *x2, double dt);
static int32 plaus_iflag(swe_ctx *ctx, int32 iflag, int32 ipl, double tjd, char *serr);
static int app_pos_rest(swe_ctx *ctx, struct plan_data *pdp, int32 iflag, 
    double *xx, double *x2000, struct epsilon *oe, char *serr);
static int open_jpl_file(swe_ctx *ctx, double *ss, char *fname, char *fpath, char *serr);
static void free_planets(swe_ctx *ctx);
static void ctx_release(swe_ctx *ctx);

#ifdef TRACE
static void trace_swe_calc(int param, double tjd, int ipl, int32 iflag, double *xx, char *serr);
static void trace_swe_fixstar(int swtch, char *star, double tjd, int32 iflag, double *xx, char *serr);
static void trace_swe_get_planet_name(int swtch, int ipl, char *s);
#endif


char *CALL_CONV swe_version(char *s)
{
  strcpy(s, SE_VERSION);
  return s;
}

#ifndef NO_SWE_GLP	// -DNO_SWE_GLP to suppress this function
#if MSDOS
HANDLE dllhandle = NULL;        // global used in swe_version
				// if DLL, set by DllMain()
#else
#ifdef __GNUC__
// C17_MIGRATION.md Phase 5: the root Makefile now compiles with
// -D_GNU_SOURCE, which is what this comment used to ask for -- the
// hand-forced __USE_GNU define it replaced is redundant with that.
#include <dlfcn.h>		// must be linked with -ldl
#endif
#endif // MSDOS

/* The caller's buffer is char[AS_MAXCH], so the last writable index is
 * AS_MAXCH-1. The guard below used to read `#if !defined(__APPLE)`; nothing
 * ever defines __APPLE (the predefined macro is spelled __APPLE__), so the
 * body has always run on every platform, macOS included. Correcting the
 * spelling would therefore be a regression -- it would make this function
 * return "" on macOS -- so the dead conditional is simply dropped.
 */
char *CALL_CONV swe_get_library_path(char *s)
{
  size_t bytes = 0;
  size_t len = AS_MAXCH - 1;	// leave room for the terminator at s[len]
  *s = '\0';
#if MSDOS
  bytes = GetModuleFileName((HMODULE) dllhandle, (TCHAR*) s, (DWORD) len);
#else
  #ifdef __GNUC__
    /* Must be a local. As a file-scope static this was shared mutable state
     * written by every concurrent caller. */
    Dl_info dli;
    if (dladdr((void *)swe_version, &dli) != 0) {
      strncpy(s, dli.dli_fname, len);
      s[len] = '\0';
      bytes = strlen(s);
    } else {
      ssize_t r = readlink("/proc/self/exe", s, len);
      bytes = (r < 0) ? 0 : (size_t) r;	// readlink() returns -1 on failure
    }
  #else
    ssize_t r = readlink("/proc/self/exe", s, len);
    bytes = (r < 0) ? 0 : (size_t) r;
  #endif
#endif
  s[bytes] = '\0';
  return s;
}
#else	// NO_SWE_GLP
// we need this function because swetest requires it
char *CALL_CONV swe_get_library_path(char *s)
{
  *s = '\0';
  return s;
}
#endif	// NO_SWE_GLP

/* The routine called by the user.
 * It checks whether a position for the same planet, the same t, and the
 * same flag bits has already been computed. 
 * If yes, this position is returned. Otherwise it is computed.
 * -> If the SEFLG_SPEED flag has been specified, the speed will be returned
 * at offset 3 of position array x[]. Its precision is probably better 
 * than 0.002"/day.
 * -> If the SEFLG_SPEED3 flag has been specified, the speed will be computed
 * from three positions. This speed is less accurate than SEFLG_SPEED,
 * i.e. better than 0.1"/day. And it is much slower. It is used for 
 * program tests only.
 * -> If no speed flag has been specified, no speed will be returned.
 */
int32 CALL_CONV swe_calc_r(swe_ctx *ctx, double tjd, int ipl, int32 iflag, 
	double *xx, char *serr)
{
  int i, j;
  int32 iplmoon = 0, iflgsave = iflag;
  int32 epheflag;
  AS_BOOL use_speed3 = FALSE;
  struct save_positions *sd;
  double x[6], *xs, x0[24], x2[24];
  double dt;
  if (serr != NULL) 
    *serr = '\0';
#ifdef TRACE
#ifdef FORCE_IFLAG
  /*
   * If this source file is compiled with /DFORCE_IFLAG or -DFORCE_IFLAG
   * and also with TRACE, then the actual value of iflag used in swe_calc()
   * can be manipulated from the outside of an application:
   * Create a text file 'force.flg' and put one text line into it
   * containing a number, e.g. 1024
   * This number will be or'ed into the iflag used by the caller of swe_calc()
   *
   * See the code below for the details.
   * This is not an important mechanism. We used it to debug an application
   * which showed strange behaviour, by compiling a special DLL with TRACE and
   * FORCE_IFLAG and then running the application with this DLL (we had no
   * source code of the application itself).
   */
  /* moved to ctx->sp.calcflag (Phase 3c) */
  /* moved to ctx->sp.calcflag (Phase 3c) */
  /* moved to ctx->sp.calcflag (Phase 3c) */
  FILE *fp;
  char s[AS_MAXCH], *sp;
  memset(x, 0, sizeof(double) * 6);
  /* if the following file exists, flag is read from it and or'ed into iflag */
  if (!ctx->sp.calcflag.force_flag_checked) {
    if ((fp = fopen(fname_force_flg, BFILE_R_ACCESS)) != NULL) {
      ctx->sp.calcflag.force_flag = 1;
      fgets(s, AS_MAXCH, fp);
      if ((sp = strchr(s, '\n')) != NULL)
	*sp = '\0';
      ctx->sp.calcflag.iflag_forced = atol(s);
      fclose(fp);
    }
    ctx->sp.calcflag.force_flag_checked = 1;
  }
  if (ctx->sp.calcflag.force_flag)
    iflag |= ctx->sp.calcflag.iflag_forced;
#endif
  swi_open_trace(serr);
  trace_swe_calc(1, tjd, ipl, iflag, xx, NULL);
#endif /* TRACE */
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO.
   * Reason: Our numerical integrator takes into account Pluto
   * perturbation and therefore crashes with body 134340 Pluto. */
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  /* if ephemeris flag != ephemeris flag of last call,
   * we clear the save area, to prevent swecalc(ctx) using
   * previously computed data for current calculation.
   * except with ipl = SE_ECL_NUT which is not dependent 
   * on ephemeris, and except if change is from 
   * ephemeris = 0 to ephemeris = SEFLG_DEFAULTEPH
   * or vice-versa.
   */
  epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag & SEFLG_MOSEPH) {
    epheflag = SEFLG_MOSEPH;
  } else if (epheflag & SEFLG_JPLEPH) {
    epheflag = SEFLG_JPLEPH;
  } else  {
    epheflag = SEFLG_SWIEPH;
  }
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL) {
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_calc() or swe_calc_ut()");
  }
  if (ctx->last_epheflag != epheflag) {
    free_planets(ctx);
    /* close and free ephemeris files */
    if (ipl != SE_ECL_NUT) {  /* because file will not be reopened with this ipl */
      if (ctx->jpl_file_is_open) {
	swi_close_jpl_file(ctx);
	ctx->jpl_file_is_open = FALSE;
      }
      for (i = 0; i < SEI_NEPHFILES; i ++) {
	if (ctx->fidat[i].fptr != NULL) 
	  fclose(ctx->fidat[i].fptr);
	memset((void *) &ctx->fidat[i], 0, sizeof(struct file_data));
      }
      ctx->last_epheflag = epheflag;
    }
  }
  /* high precision speed prevails fast speed */
  if ((iflag & SEFLG_SPEED3) && (iflag & SEFLG_SPEED))
    iflag = iflag & ~SEFLG_SPEED3;
  if (iflag & SEFLG_SPEED3) 
    use_speed3 = TRUE;
  /* topocentric with SEFLG_SPEED is not good if aberration is included. 
   * in such cases we calculate speed from three positions */
  if ((iflag & SEFLG_SPEED) && (iflag & SEFLG_TOPOCTR) && !(iflag & SEFLG_NOABERR)) 
    use_speed3 = TRUE;
  /* cartesian flag excludes radians flag */
  if ((iflag & SEFLG_XYZ) && (iflag & SEFLG_RADIANS))
    iflag = iflag & ~SEFLG_RADIANS;
/*  if (iflag & SEFLG_ICRS)
    iflag |= SEFLG_J2000;*/
  /* planetary center of body or planetary moon: either planet is called
   * with SEFLG_CENTER_BODY or center of body with ipl = 9n99 is called.
   * we want to handle both cases the same way. */
  // planet is called with SE_PLUTO etc. and SEFLG_CENTER_BODY:
  // get number of center of body 
  if ((iflag & SEFLG_CENTER_BODY) && ipl <= SE_PLUTO && (iflag & SEFLG_TEST_PLMOON) != SEFLG_TEST_PLMOON) {
    iplmoon = ipl * 100 + 9099; // planetary center of body
  }
  // planet center of body or planetary moon is called using 9... number:
  // moon number and planet number
  if (ipl >= SE_PLMOON_OFFSET && ipl < SE_AST_OFFSET && (iflag & SEFLG_TEST_PLMOON) != SEFLG_TEST_PLMOON) {
    iplmoon = ipl; // planetary center of body or planetary moon
    ipl = (int) ((ipl - 9000) / 100);
    iflag |= SEFLG_CENTER_BODY;
  }
  // with Mercury to Mars, we do not have center of body different from barycenter
  if ((iflag & SEFLG_CENTER_BODY) && ipl <= SE_MARS && (iplmoon % 100) == 99) {
    iplmoon = 0;
    iflag &= ~SEFLG_CENTER_BODY;
  }
  if ((iflag & SEFLG_CENTER_BODY) || iplmoon > 0)
    swi_force_app_pos_etc(ctx);
  /* pointer to save area */
  if (ipl < SE_NPLANETS && ipl >= SE_SUN) {
    sd = &ctx->savedat[ipl];
//    if (iflag & SEFLG_CENTER_BODY)
//      sd = &swed.savedat[SE_NPLANETS];
  } else {
    /* other bodies, e.g. asteroids called with ipl = SE_AST_OFFSET + MPC# */
    sd = &ctx->savedat[SE_NPLANETS];
  }
  /* 
   * if position is available in save area, it is returned.
   * this is the case, if tjd = tsave and iflag = iflgsave.
   * coordinate flags can be neglected, because save area 
   * provides all coordinate types.
   * if ipl > SE_AST(EROID)_OFFSET, ipl must be checked, 
   * because all asteroids called by MPC number share the same
   * save area.
   */ 
  if (sd->tsave == tjd && tjd != 0 && ipl == sd->ipl && iplmoon == 0) {
    if ((sd->iflgsave & ~SEFLG_COORDSYS) == (iflag & ~SEFLG_COORDSYS)) 
      goto end_swe_calc;
  }
  /* 
   * otherwise, new position must be computed 
   */
  if (!use_speed3) {
    /* 
     * with high precision speed from one call of swecalc(ctx) 
     * (FAST speed)
     */
    sd->tsave = tjd;
    sd->ipl = ipl;
    if ((sd->iflgsave = swecalc(ctx, tjd, ipl, iplmoon, iflag, sd->xsaves, serr)) == ERR) 
      goto return_error;
  } else {
    /* 
     * with speed from three calls of swecalc(ctx), slower and less accurate.
     * (SLOW speed, for test only)
     */
    sd->tsave = tjd;
    sd->ipl = ipl;
    switch(ipl) {
      case SE_MOON:
	dt = MOON_SPEED_INTV;
	break;
      case SE_OSCU_APOG:
      case SE_TRUE_NODE:
	/* this is the optimum dt with Moshier ephemeris, but not with
	 * JPL ephemeris or SWISSEPH. To avoid completely false speed
	 * in case that JPL is wanted but the program returns Moshier,
	 * we use Moshier optimum.
	 * For precise speed, use JPL and FAST speed computation,
	 */
	dt = NODE_CALC_INTV_MOSH;
	break;
      default:
	dt = PLAN_SPEED_INTV;
	break;
    } 
    if ((sd->iflgsave = swecalc(ctx, tjd-dt, ipl, iplmoon, iflag, x0, serr)) == ERR)
      goto return_error; 
    if ((sd->iflgsave = swecalc(ctx, tjd+dt, ipl, iplmoon, iflag, x2, serr)) == ERR)
      goto return_error; 
    if ((sd->iflgsave = swecalc(ctx, tjd, ipl, iplmoon, iflag, sd->xsaves, serr)) == ERR)
      goto return_error; 
    denormalize_positions(x0, sd->xsaves, x2);
    calc_speed(x0, sd->xsaves, x2, dt);
  }
  end_swe_calc:
  if (iflag & SEFLG_EQUATORIAL) {
    xs = sd->xsaves+12;	/* equatorial coordinates */
  } else {
    xs = sd->xsaves;	/* ecliptic coordinates */
  }
  if (iflag & SEFLG_XYZ)
    xs = xs+6;		/* cartesian coordinates */
  if (ipl == SE_ECL_NUT) {
    i = 4;
  } else {
    i = 3;
  }
  for (j = 0; j < i; j++)
    x[j] = *(xs + j);
  for (j = i; j < 6; j++)
    x[j] = 0;
  if (iflag & (SEFLG_SPEED3 | SEFLG_SPEED)) {
    for (j = 3; j < 6; j++)
      x[j] = *(xs + j);
  }
#if 1
  if (iflag & SEFLG_RADIANS) {
    if (ipl == SE_ECL_NUT) {
      for (j = 0; j < 4; j++)
        x[j] *= DEGTORAD;
    } else {
      for (j = 0; j < 2; j++)
        x[j] *= DEGTORAD;
      if (iflag & (SEFLG_SPEED3 | SEFLG_SPEED)) {
        for (j = 3; j < 5; j++) 
	  x[j] *= DEGTORAD;
      }
    }
  }
#endif
  for (i = 0; i <= 5; i++)
    xx[i] = x[i];
  //iflag = sd->iflgsave | (iflag & SEFLG_COORDSYS);
  // iflag from previous call of swe_calc(), without coordinate system flags
  iflag = sd->iflgsave & ~SEFLG_COORDSYS; 
  // add correct coordinate system flags
  iflag |= (iflgsave & SEFLG_COORDSYS); 
  /* if no ephemeris has been specified, do not return chosen ephemeris */
  if ((iflgsave & SEFLG_EPHMASK) == 0)
    iflag = iflag & ~SEFLG_DEFAULTEPH;
#ifdef TRACE
  trace_swe_calc(2, tjd, ipl, iflag, xx, serr);
#endif
  return iflag;
return_error:
  for (i = 0; i <= 5; i++)
    xx[i] = 0;
#ifdef TRACE
  trace_swe_calc(2, tjd, ipl, iflag, xx, serr);
#endif
  return ERR; 
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_calc(double tjd, int ipl, int32 iflag, 
	double *xx, char *serr)
{
  return swe_calc_r(swi_default_ctx(), tjd, ipl, iflag, xx, serr);
}

int32 CALL_CONV swe_calc_ut_r(swe_ctx *ctx, double tjd_ut, int32 ipl, int32 iflag, 
	double *xx, char *serr)
{
  double deltat;
  int32 retval = OK;
  int32 epheflag = 0;
  iflag = plaus_iflag(ctx, iflag, ipl, tjd_ut, serr);
  epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag == 0) {
    epheflag = SEFLG_SWIEPH;
    iflag |= SEFLG_SWIEPH;
  }
  deltat = swe_deltat_ex_r(ctx, tjd_ut, iflag, serr);
  retval = swe_calc_r(ctx, tjd_ut + deltat, ipl, iflag, xx, serr);
  /* if ephe required is not ephe returned, adjust delta t: */
  if ((retval & SEFLG_EPHMASK) != epheflag) {
    deltat = swe_deltat_ex_r(ctx, tjd_ut, retval, NULL);
    retval = swe_calc_r(ctx, tjd_ut + deltat, ipl, iflag, xx, NULL);
  }
  return retval;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_calc_ut(double tjd_ut, int32 ipl, int32 iflag, 
	double *xx, char *serr)
{
  return swe_calc_ut_r(swi_default_ctx(), tjd_ut, ipl, iflag, xx, serr);
}

static int32 swecalc(swe_ctx *ctx, double tjd, int ipl, int32 iplmoon, int32 iflag, double *x, char *serr) 
{
  int i;
  int ipli, ipli_ast, ifno;
  int retc;
  int32 epheflag = SEFLG_DEFAULTEPH;
  struct plan_data *pdp;
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  struct plan_data *ndp;
  double *xp, *xp2;
  double ss[3];
  char serr2[AS_MAXCH];
  //if (serr != NULL)
  //  *serr = '\0';  // is done in calling function
  serr2[0] = '\0';
  /****************************************** 
   * iflag plausible?                       * 
   ******************************************/
  iflag = plaus_iflag(ctx, iflag, ipl, tjd, serr);
  /****************************************** 
   * which ephemeris is wanted, which is used?
   * Three ephemerides are possible: MOSEPH, SWIEPH, JPLEPH.
   * JPLEPH is best, SWIEPH is nearly as good, MOSEPH is least precise.
   * The availability of the various ephemerides depends on the installed
   * ephemeris files in the users ephemeris directory. This can change at
   * any time.
   * Swisseph should try to fulfil the wish of the user for a specific
   * ephemeris, but use a less precise one if the desired ephemeris is not
   * available for the given date and body. 
   * If internal ephemeris errors are detected (data error, file length error)
   * an error is returned.
   * If the time range is bad but another ephemeris can deliver this range,
   * the other ephemeris is used.
   * If no ephemeris is specified, DEFAULTEPH is assumed as desired.
   * DEFAULTEPH is defined at compile time, usually as JPLEPH.
   * The caller learns from the return flag which ephemeris was used.
   * ephe_flag is extracted from iflag, but can change later if the
   * desired ephe is not available.
   ******************************************/
  if (iflag & SEFLG_MOSEPH)
    epheflag = SEFLG_MOSEPH;
  if (iflag & SEFLG_SWIEPH)
    epheflag = SEFLG_SWIEPH;
  if (iflag & SEFLG_JPLEPH)
    epheflag = SEFLG_JPLEPH;
  /* no barycentric calculations with Moshier ephemeris */
  if ((iflag & SEFLG_BARYCTR) && (iflag & SEFLG_MOSEPH)) {
    if (serr != NULL)
      strcpy(serr, "barycentric Moshier positions are not supported.");
    return ERR;
  }
  if (epheflag != SEFLG_MOSEPH && !ctx->ephe_path_is_set && !ctx->jpl_file_is_open)
    SWI_CFG_LOCAL(ctx, swe_set_ephe_path_r(ctx, NULL));
  if ((iflag & SEFLG_SIDEREAL) && !ctx->ayana_is_set)
    SWI_CFG_LOCAL(ctx, swe_set_sid_mode_r(ctx, SE_SIDM_FAGAN_BRADLEY, 0, 0));
  /****************************************** 
   * obliquity of ecliptic 2000 and of date * 
   ******************************************/
  swi_check_ecliptic(ctx, tjd, iflag);
  /******************************************
   * nutation                               * 
   ******************************************/
  swi_check_nutation(ctx, tjd, iflag);
  /****************************************** 
   * select planet and ephemeris            * 
   *                                        * 
   * ecliptic and nutation                  * 
   ******************************************/
  if (ipl == SE_ECL_NUT) {
    x[0] = ctx->oec.eps + ctx->nut.nutlo[1];	/* true ecliptic */
    x[1] = ctx->oec.eps;			/* mean ecliptic */
    x[2] = ctx->nut.nutlo[0];		/* nutation in longitude */
    x[3] = ctx->nut.nutlo[1];		/* nutation in obliquity */
    /*if ((iflag & SEFLG_RADIANS) == 0)*/
      for (i = 0; i <= 3; i++)
	x[i] *= RADTODEG;
    return(iflag);
  /****************************************** 
   * moon                                   * 
   ******************************************/
  } else if (ipl == SE_MOON) {
    /* internal planet number */
    ipli = SEI_MOON;
    pdp = &ctx->pldat[ipli];
    xp = pdp->xreturn;
    switch(epheflag) {
      case SEFLG_JPLEPH:
	retc = jplplan(ctx, tjd, ipli, iflag, DO_SAVE, NULL, NULL, NULL, serr);
	/* read error or corrupt file */
	if (retc == ERR) 
	  goto return_error;
        /* jpl ephemeris not on disk or date beyond ephemeris range 
	 *     or file corrupt */
        if (retc == NOT_AVAILABLE) {
	  iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_SWIEPH;
	  if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	    strcat(serr, " \ntrying Swiss Eph; ");
	  goto sweph_moon;
	} else if (retc == BEYOND_EPH_LIMITS) {
	  if (tjd > MOSHLUEPH_START && tjd < MOSHLUEPH_END) {
	    iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_MOSEPH;
	    if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	      strcat(serr, " \nusing Moshier Eph; ");
	    goto moshier_moon;
	  } else
	    goto return_error;
	}
	break;
      case SEFLG_SWIEPH:
	sweph_moon:
	retc = sweplan(ctx, tjd, ipli, SEI_FILE_MOON, iflag, DO_SAVE,
			NULL, NULL, NULL, NULL, serr);
	if (retc == ERR)
	  goto return_error;
	/* if sweph file not found, switch to moshier */
        if (retc == NOT_AVAILABLE) {
	  if (tjd > MOSHLUEPH_START && tjd < MOSHLUEPH_END) {
	    iflag = (iflag & ~SEFLG_SWIEPH) | SEFLG_MOSEPH;
	    if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	      strcat(serr, " \nusing Moshier eph.; ");
	    goto moshier_moon;
	  } else 
	    goto return_error;
	}
	break;
      case SEFLG_MOSEPH:
	moshier_moon:
        retc = swi_moshmoon(ctx, tjd, DO_SAVE, NULL, serr);/**/
	if (retc == ERR)
	  goto return_error;
	/* for hel. position, we need earth as well */
	retc = swi_moshplan(ctx, tjd, SEI_EARTH, DO_SAVE, NULL, NULL, serr);/**/
	if (retc == ERR)
	  goto return_error;
	break;
      default:
	break;
    } 
    /* heliocentric, lighttime etc. */
    if ((retc = app_pos_etc_moon(ctx, iflag, serr)) != OK)
      goto return_error; /* retc may be wrong with sidereal calculation */
  /********************************************** 
   * barycentric sun                            * 
   * (only JPL and SWISSEPH ephemerises)        *
   **********************************************/
  } else if (ipl == SE_SUN && (iflag & SEFLG_BARYCTR)) {
    /* barycentric sun must be handled separately because of
     * the following reasons:
     * ordinary planetary computations use the function 
     * main_planet(ctx) and its subfunction jplplan(ctx),
     * see further below.
     * now, these functions need the swisseph internal 
     * planetary indices, where SEI_EARTH = SEI_SUN = 0.
     * therefore they don't know the difference between
     * a barycentric sun and a barycentric earth and 
     * always return barycentric earth.
     * to avoid this problem, many functions would have to
     * be changed. as an alternative, we choose a more 
     * separate handling. */
    ipli = SEI_SUN;	/* = SEI_EARTH ! */
    xp = pedp->xreturn;
    switch(epheflag) {
      case SEFLG_JPLEPH:
	/* open ephemeris, if still closed */
	if (!ctx->jpl_file_is_open) {
	  retc = open_jpl_file(ctx, ss, ctx->jplfnam, ctx->ephepath, serr);
	  if (retc != OK)
	    goto sweph_sbar;
	}
	retc = swi_pleph(ctx, tjd, J_SUN, J_SBARY, psdp->x, serr);
	if (retc == ERR || retc == BEYOND_EPH_LIMITS) {
	  swi_close_jpl_file(ctx);
	  ctx->jpl_file_is_open = FALSE;
	  goto return_error;
	}
        /* jpl ephemeris not on disk or date beyond ephemeris range 
	 *     or file corrupt */
        if (retc == NOT_AVAILABLE) {
	  iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_SWIEPH;
	  if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	    strcat(serr, " \ntrying Swiss Eph; ");
	  goto sweph_sbar;
	}
	psdp->teval = tjd;
	break;
      case SEFLG_SWIEPH:
	sweph_sbar:
	/* sweplan(ctx) provides barycentric sun as a by-product in save area;
	 * it is saved in swed.pldat[SEI_SUNBARY].x */
	retc = sweplan(ctx, tjd, SEI_EARTH, SEI_FILE_PLANET, iflag, DO_SAVE, NULL, NULL, NULL, NULL, serr);
#if 1
	if (retc == ERR || retc == NOT_AVAILABLE)
	  goto return_error;
#else	/* this code would be needed if barycentric moshier calculation
	 * were implemented */
	if (retc == ERR)
	  goto return_error;
	/* if sweph file not found, switch to moshier */
        if (retc == NOT_AVAILABLE) {
	  if (tjd > MOSHLUEPH_START && tjd < MOSHLUEPH_END) {
	    iflag = (iflag & ~SEFLG_SWIEPH) | SEFLG_MOSEPH;
	    if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	      strcat(serr, " \nusing Moshier; ");
	    goto moshier_sbar;
	  } else
	    goto return_error;
	}
#endif
	psdp->teval = tjd;
	/* pedp->teval = tjd; */
	break;
      default:
	return ERR;
	break;
    }
    /* flags */
    if ((retc = app_pos_etc_sbar(ctx, iflag, serr)) != OK)
      goto return_error; 
    /* iflag has possibly changed */
    iflag = pedp->xflgs;
    /* barycentric sun is now in save area of barycentric earth.
     * (pedp->xreturn = swed.pldat[SEI_EARTH].xreturn).
     * in case a barycentric earth computation follows for the same
     * date, the planetary functions will return the barycentric 
     * SUN unless we force a new computation of pedp->xreturn.
     * this can be done by initializing the save of iflag.
     */
    pedp->xflgs = -1;
  /****************************************** 
   * mercury - pluto                        * 
   ******************************************/
  } else if (ipl == SE_SUN 	/* main planet */
	  || ipl == SE_MERCURY
	  || ipl == SE_VENUS
	  || ipl == SE_MARS
	  || ipl == SE_JUPITER
	  || ipl == SE_SATURN
	  || ipl == SE_URANUS
	  || ipl == SE_NEPTUNE
	  || ipl == SE_PLUTO
	  || ipl == SE_EARTH) {
    if (iflag & SEFLG_HELCTR) {
      if (ipl == SE_SUN) {
	/* heliocentric position of Sun does not exist */
	for (i = 0; i < 24; i++)
	  x[i] = 0;
	return iflag;
      } 
    } else if (iflag & SEFLG_BARYCTR) {
      ;
    } else {		/* geocentric */
      if (ipl == SE_EARTH) {
	/* geocentric position of Earth does not exist */
	for (i = 0; i < 24; i++)
	  x[i] = 0;
	return iflag;
      }
    }
    /* internal planet number */
    ipli = pnoext2int[ipl];
    pdp = &ctx->pldat[ipli];
    xp = pdp->xreturn;
    retc = main_planet(ctx, tjd, ipli, iplmoon, epheflag, iflag, serr);
    if (retc == ERR)
      goto return_error;
    /* iflag has possibly changed in main_planet(ctx) */
    iflag = pdp->xflgs;
  /*********************i************************ 
   * mean lunar node                            * 
   * for comment s. moshmoon.c, swi_mean_node(ctx) *
   **********************************************/
  } else if (ipl == SE_MEAN_NODE) {
    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
      /* heliocentric/barycentric lunar node not allowed */
      for (i = 0; i < 24; i++)
	x[i] = 0;
      return iflag;
    }
    ndp = &ctx->nddat[SEI_MEAN_NODE];
    xp = ndp->xreturn;
    xp2 = ndp->x;
    retc = swi_mean_node(ctx, tjd, xp2, serr);
    if (retc == ERR)
      goto return_error;
    /* speed (is almost constant; variation < 0.001 arcsec) */
    retc = swi_mean_node(ctx, tjd - MEAN_NODE_SPEED_INTV, xp2+3, serr);
    if (retc == ERR)
      goto return_error;
    xp2[3] = swe_difrad2n(xp2[0], xp2[3]) / MEAN_NODE_SPEED_INTV;
    xp2[4] = xp2[5] = 0;
    ndp->teval = tjd;
    ndp->xflgs = -1; 	
    /* lighttime etc. */
    if ((retc = app_pos_etc_mean(ctx, SEI_MEAN_NODE, iflag, serr)) != OK)
      goto return_error;
    /* to avoid infinitesimal deviations from latitude = 0 
     * that result from conversions */
    if (!(iflag & SEFLG_SIDEREAL) && !(iflag & SEFLG_J2000)) {
      ndp->xreturn[1] = 0.0;	/* ecl. latitude       */
      ndp->xreturn[4] = 0.0;	/*               speed */
      ndp->xreturn[5] = 0.0;	/*      radial   speed */
      ndp->xreturn[8] = 0.0;	/* z coordinate        */
      ndp->xreturn[11] = 0.0;	/*               speed */
    }
    if (retc == ERR)
      goto return_error;
  /********************************************** 
   * mean lunar apogee ('dark moon', 'lilith')  *
   * for comment s. moshmoon.c, swi_mean_apog(ctx) *
   **********************************************/
  } else if (ipl == SE_MEAN_APOG) {
    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
      /* heliocentric/barycentric lunar apogee not allowed */
      for (i = 0; i < 24; i++)
	x[i] = 0;
      return iflag;
    }
    ndp = &ctx->nddat[SEI_MEAN_APOG];
    xp = ndp->xreturn;
    xp2 = ndp->x;
    retc = swi_mean_apog(ctx, tjd, xp2, serr);
    if (retc == ERR)
      goto return_error;
    /* speed (is not constant! variation ~= several arcsec) */
    retc = swi_mean_apog(ctx, tjd - MEAN_NODE_SPEED_INTV, xp2+3, serr);
    if (retc == ERR)
      goto return_error;
    for(i = 0; i <= 1; i++)
      xp2[3+i] = swe_difrad2n(xp2[i], xp2[3+i]) / MEAN_NODE_SPEED_INTV;
    xp2[5] = 0;
    ndp->teval = tjd;
    ndp->xflgs = -1; 	
    /* lighttime etc. */
    if ((retc = app_pos_etc_mean(ctx, SEI_MEAN_APOG, iflag, serr)) != OK)
      goto return_error;
    /* to avoid infinitesimal deviations from r-speed = 0 
     * that result from conversions */
    ndp->xreturn[5] = 0.0;	/*               speed */
    if (retc == ERR)
      goto return_error;
  /*********************************************** 
   * osculating lunar node ('true node')         *    
   ***********************************************/
  } else if (ipl == SE_TRUE_NODE) {
    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
      /* heliocentric/barycentric lunar node not allowed */
      for (i = 0; i < 24; i++)
	x[i] = 0;
      return iflag;
    }
    ndp = &ctx->nddat[SEI_TRUE_NODE];
    xp = ndp->xreturn;
    retc = lunar_osc_elem(ctx, tjd, SEI_TRUE_NODE, iflag, serr); 
    iflag = ndp->xflgs;
    /* to avoid infinitesimal deviations from latitude = 0 
     * that result from conversions */
    if (!(iflag & SEFLG_SIDEREAL) && !(iflag & SEFLG_J2000)) {
      ndp->xreturn[1] = 0.0;	/* ecl. latitude       */
      ndp->xreturn[4] = 0.0;	/*               speed */
      ndp->xreturn[8] = 0.0;	/* z coordinate        */
      ndp->xreturn[11] = 0.0;	/*               speed */
    }
    if (retc == ERR)
      goto return_error;
  /*********************************************** 
   * osculating lunar apogee                     *    
   ***********************************************/
  } else if (ipl == SE_OSCU_APOG) {
    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
      /* heliocentric/barycentric lunar apogee not allowed */
      for (i = 0; i < 24; i++)
	x[i] = 0;
      return iflag;
    }
    ndp = &ctx->nddat[SEI_OSCU_APOG];
    xp = ndp->xreturn;
    retc = lunar_osc_elem(ctx, tjd, SEI_OSCU_APOG, iflag, serr); 
    iflag = ndp->xflgs;
    if (retc == ERR)
      goto return_error;
  /*********************************************** 
   * interpolated lunar apogee                   *    
   ***********************************************/
  } else if (ipl == SE_INTP_APOG) {
    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
      /* heliocentric/barycentric lunar apogee not allowed */
      for (i = 0; i < 24; i++)
	x[i] = 0;
      return iflag;
    }
    if (tjd < MOSHLUEPH_START || tjd > MOSHLUEPH_END) {
      for (i = 0; i < 24; i++)
	x[i] = 0;
      if (serr != NULL)
	sprintf(serr, "Interpolated apsides are restricted to JD %8.1f - JD %8.1f",
		MOSHLUEPH_START, MOSHLUEPH_END);
      return ERR;
    }
    ndp = &ctx->nddat[SEI_INTP_APOG];
    xp = ndp->xreturn;
    retc = intp_apsides(ctx, tjd, SEI_INTP_APOG, iflag, serr); 
    iflag = ndp->xflgs;
    if (retc == ERR)
      goto return_error;
  /*********************************************** 
   * interpolated lunar perigee                  *    
   ***********************************************/
  } else if (ipl == SE_INTP_PERG) {
    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
      /* heliocentric/barycentric lunar apogee not allowed */
      for (i = 0; i < 24; i++)
	x[i] = 0;
      return iflag;
    }
    if (tjd < MOSHLUEPH_START || tjd > MOSHLUEPH_END) {
      for (i = 0; i < 24; i++)
	x[i] = 0;
      if (serr != NULL)
	sprintf(serr, "Interpolated apsides are restricted to JD %8.1f - JD %8.1f",
		MOSHLUEPH_START, MOSHLUEPH_END);
      return ERR;
    }
    ndp = &ctx->nddat[SEI_INTP_PERG];
    xp = ndp->xreturn;
    retc = intp_apsides(ctx, tjd, SEI_INTP_PERG, iflag, serr); 
    iflag = ndp->xflgs;
    if (retc == ERR)
      goto return_error;
  /*********************************************** 
   * minor planets                               *    
   ***********************************************/
  } else if (ipl == SE_CHIRON 
    || ipl == SE_PHOLUS
    || ipl == SE_CERES		/* Ceres - Vesta */
    || ipl == SE_PALLAS		
    || ipl == SE_JUNO	
    || ipl == SE_VESTA
    || ipl > SE_PLMOON_OFFSET
    || ipl > SE_AST_OFFSET // obsolete after previous condition
    ) {
    /* internal planet number */
    if (ipl < SE_NPLANETS) {
      ipli = pnoext2int[ipl];
    } else if (ipl <= SE_AST_OFFSET + MPC_VESTA && ipl > SE_AST_OFFSET) {
      ipli = SEI_CERES + ipl - SE_AST_OFFSET - 1;
      ipl = SE_CERES + ipl - SE_AST_OFFSET - 1;
    } else {			/* any asteroid except*/
      ipli = SEI_ANYBODY;
    }
    if (ipli == SEI_ANYBODY) {
      ipli_ast = ipl;
    } else {
      ipli_ast = ipli;
    }
    pdp = &ctx->pldat[ipli];
    xp = pdp->xreturn;
    if (ipli_ast > SE_AST_OFFSET) {
      ifno = SEI_FILE_ANY_AST;
    } else if (ipli_ast > SE_PLMOON_OFFSET) {
      ifno = SEI_FILE_ANY_AST;
    } else {
      ifno = SEI_FILE_MAIN_AST;
    }
    if (ipli == SEI_CHIRON && (tjd < CHIRON_START || tjd > CHIRON_END)) {
      if (serr != NULL)
	sprintf(serr, "Chiron's ephemeris is restricted to JD %8.1f - JD %8.1f",
		CHIRON_START, CHIRON_END);
      return ERR;
    }
    if (ipli == SEI_PHOLUS && (tjd < PHOLUS_START || tjd > PHOLUS_END)) {
      if (serr != NULL)
	sprintf(serr, 
		"Pholus's ephemeris is restricted to JD %8.1f - JD %8.1f",
		PHOLUS_START, PHOLUS_END);
      return ERR;
    }
  do_asteroid:
    /* earth and sun are also needed */
    retc = main_planet(ctx, tjd, SEI_EARTH, 0, epheflag, iflag, serr);
    if (retc == ERR) 
      goto return_error;
    /* iflag (ephemeris bit) has possibly changed in main_planet(ctx) */
    iflag = ctx->pldat[SEI_EARTH].xflgs;
    /* asteroid */
    if (serr != NULL) {
      strcpy(serr2, serr); 
      *serr = '\0';
    }
    /* asteroid */
    retc = sweph(ctx, tjd, ipli_ast, ifno, iflag, psdp->x, DO_SAVE, NULL, serr);
    if (retc == ERR || retc == NOT_AVAILABLE) 
      goto return_error;
    retc = app_pos_etc_plan(ctx, ipli_ast, 0, iflag, serr);
    if (retc == ERR)
      goto return_error;
    /* app_pos_etc_plan(ctx) might have failed, if t(light-time)
     * is beyond ephemeris range. in this case redo with Moshier 
     */
    if (retc == NOT_AVAILABLE || retc == BEYOND_EPH_LIMITS) {
      if (epheflag != SEFLG_MOSEPH) { 
	iflag = (iflag & ~SEFLG_EPHMASK) | SEFLG_MOSEPH;
	epheflag = SEFLG_MOSEPH;
	if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	  strcat(serr, "\nusing Moshier eph.; ");
	goto do_asteroid;
      } else
	goto return_error;
    }
    /* add warnings from earth/sun computation */
    if (serr != NULL && *serr == '\0' && *serr2 != '\0') {
      strcpy(serr, "sun: ");
      serr2[AS_MAXCH-5] = '\0';
      strcat(serr, serr2);
    }
  /*********************************************** 
   * fictitious planets                          *    
   * (Isis-Transpluto and Uranian planets)       *
   ***********************************************/
  } else if (ipl >= SE_FICT_OFFSET && ipl <= SE_FICT_MAX) {
    /* internal planet number */
    ipli = SEI_ANYBODY;
    pdp = &ctx->pldat[ipli];
    xp = pdp->xreturn;
  do_fict_plan:
    /* the earth for geocentric position */
    retc = main_planet(ctx, tjd, SEI_EARTH, 0, epheflag, iflag, serr);
    /* iflag (ephemeris bit) has possibly changed in main_planet(ctx) */
    iflag = ctx->pldat[SEI_EARTH].xflgs;
    /* planet from osculating elements */
    if (swi_osc_el_plan(ctx, tjd, pdp->x, ipl-SE_FICT_OFFSET, ipli, pedp->x, psdp->x, serr) != OK)
      goto return_error;
    if (retc == ERR)
      goto return_error;
    retc = app_pos_etc_plan_osc(ctx, ipl, ipli, iflag, serr);
    if (retc == ERR)
      goto return_error;
    /* app_pos_etc_plan_osc(ctx) might have failed, if t(light-time)
     * is beyond ephemeris range. in this case redo with Moshier 
     */
    if (retc == NOT_AVAILABLE || retc == BEYOND_EPH_LIMITS) {
      if (epheflag != SEFLG_MOSEPH) { 
	iflag = (iflag & ~SEFLG_EPHMASK) | SEFLG_MOSEPH;
	epheflag = SEFLG_MOSEPH;
	if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	  strcat(serr, "\nusing Moshier eph.; ");
	goto do_fict_plan;
      } else
	goto return_error;
    }
  /*********************************************** 
   * invalid body number                         *    
   ***********************************************/
  } else {
    if (serr != NULL) {
      sprintf(serr, "illegal planet number %d.", ipl);
    }
    goto return_error;
  }
  for (i = 0; i < 24; i++)
    x[i] = xp[i];
  return(iflag);
  /*********************************************** 
   * return error                                * 
   ***********************************************/
  return_error:;
  for (i = 0; i < 24; i++)
    x[i] = 0;
  return ERR;
}

static void free_planets(swe_ctx *ctx)
{
  int i;
  /* free planets data space */
  for (i = 0; i < SEI_NPLANETS; i++) {
    if (ctx->pldat[i].segp != NULL) {
      free((void *) ctx->pldat[i].segp);
    }
    if (ctx->pldat[i].refep != NULL) {
      free((void *) ctx->pldat[i].refep);
    }
    memset((void *) &ctx->pldat[i], 0, sizeof(struct plan_data));
  }
  for (i = 0; i <= SE_NPLANETS; i++) /* "<=" is correct! see decl. */
    memset((void *) &ctx->savedat[i], 0, sizeof(struct save_positions));
  /* clear node data space */
  for (i = 0; i < SEI_NNODE_ETC; i++) {
    memset((void *) &ctx->nddat[i], 0, sizeof(struct plan_data));
  }
}

/* Function initialises swed structure. 
 * Returns 1 if initialisation is done, otherwise 0 */
/* The default context. See notes/PHASE3-API.md section 4: for now this is
 * the same TLS `swed` the library has always used, reached through a
 * function so that call sites can be converted to take a context before the
 * question of WHICH context is settled. */
swe_ctx *swi_default_ctx(void)
{
  return &swed;
}

/*======================================================================
 * Phase 3d: explicit contexts.
 *
 * Threading contract (notes/PHASE3-API.md section 3.1): a swe_ctx may be
 * used by ONE thread at a time. Contexts are independent of each other and
 * take no locks between them -- the FILE * / sqlite3 * contract. Passing a
 * context between threads is fine with external synchronisation; using it
 * from two at once is not.
 *====================================================================*/

/* Bring a freshly zeroed context up to library defaults.
 *
 * This is swi_init_swed_if_start()'s fresh-start block, minus the memset
 * (calloc already did it) and minus swe_set_tid_acc(), which is a
 * publishing setter and must not broadcast a new context's defaults as the
 * process-wide master. The two fields it would have set are set directly.
 */
static void ctx_init_defaults(swe_ctx *ctx)
{
  strcpy(ctx->ephepath, SE_EPHE_PATH);
  strcpy(ctx->jplfnam, SE_FNAME_DFT);
  ctx->const_lapse_rate = SE_LAPSE_RATE;   /* calloc leaves 0.0, not 0.0065 */
  ctx->saved_sundec     = 99;              /* "no Sunshine memo yet" */
  ctx->tid_acc          = SE_TIDAL_DEFAULT;
  ctx->is_tid_acc_manual = FALSE;
  /* The two builtin tables are working copies, not statics, since Phase 3c.
   * A context that never reaches init_dt()/init_leapsec() would otherwise
   * read them as zeros -- which is how a 2.71 arcsec Delta-T regression got
   * in once already (notes/PLAN.md section 15). */
  swi_seed_dt_table(ctx);
  swi_seed_leap_table(ctx);
  ctx->swed_is_initialised = TRUE;
}

swe_ctx *CALL_CONV swe_ctx_new(void)
{
  swe_ctx *ctx = (swe_ctx *) calloc(1, sizeof(struct swe_ctx));
  if (ctx == NULL)
    return NULL;
  /* Hold the re-entrancy guard across setup so nothing below publishes: a
   * new context's defaults becoming the master would overwrite whatever
   * the caller had already configured. */
  ctx->cfg_applying = TRUE;
  ctx_init_defaults(ctx);
  ctx->cfg_applying = FALSE;
  /* Inherit the current configuration rather than starting from library
   * defaults -- the decision recorded in notes/PHASE3-API.md section 5.
   * The common case is "I configured the library, now give me a context",
   * and the alternative is a silent Moshier fallback, which is the exact
   * failure this branch exists to eliminate.
   *
   * swi_config_sync() is a no-op when no setter has ever run, which leaves
   * the library defaults just set above. Note this inherits what has been
   * PUBLISHED; a default context configured only through the SE_EPHE_PATH
   * environment variable has published nothing, and the new context picks
   * the same variable up on its own first use. */
  swi_config_inherit(ctx);
  return ctx;
}

void CALL_CONV swe_ctx_free(swe_ctx *ctx)
{
  if (ctx == NULL)          /* free(NULL) semantics */
    return;
  /* Freeing the process-wide default would leave swi_default_ctx()
   * returning a dangling pointer, and it is not heap-allocated. */
  if (ctx == swi_default_ctx())
    return;
  ctx_release(ctx);
  free((void *) ctx);
}

int32 swi_init_swed_if_start(swe_ctx *ctx)
{
  int32 started = 0;
  /* initialisation of swed, when called first time from */
  if (!ctx->swed_is_initialised) {
    /* Guard the whole block: swe_set_tid_acc() below is a publishing
     * setter, and this thread's defaults must not become the master. */
    /* These three must survive the memset. They describe this context's
     * relationship to the shared config master, not library state that a
     * fresh start should discard.
     *
     * While they were TLS in sweconfig.c the memset could not reach them.
     * As swe_ctx fields (Phase 3d) it clears cfg_applying in the middle of
     * the guard that is meant to be held right here -- and swe_set_tid_acc()
     * below then publishes this context's freshly-zeroed defaults as the
     * master, which is precisely the failure swi_config_publish() documents.
     * G2 caught it: all 8 workers diverged from the main thread at calc_ut,
     * i.e. delta-T, i.e. tid_acc. */
    swi_gen_t saved_seen  = ctx->cfg_seen;
    int32     saved_local = ctx->cfg_local;
    AS_BOOL swi_cfg_was = swi_config_begin_apply(ctx);
    /* ctx, not &swed: a non-default context must clear itself. */
    memset((void *) ctx, 0, sizeof(struct swe_ctx));
    ctx->cfg_seen     = saved_seen;
    ctx->cfg_local    = saved_local;
    ctx->cfg_applying = TRUE;      /* re-arm the guard the memset cleared */
    strcpy(ctx->ephepath, SE_EPHE_PATH);
    strcpy(ctx->jplfnam, SE_FNAME_DFT);
    /* the memset above would leave this 0.0, not the 0.0065 default */
    ctx->const_lapse_rate = SE_LAPSE_RATE;
    swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC);
    ctx->swed_is_initialised = TRUE;
    swi_config_end_apply(ctx, swi_cfg_was);
    started = 1;
  }
  /* Adopt any configuration published by another thread. This is the
   * hook the whole of Phase 2 hangs off: it is already called from 16
   * sites across sweph.c and swephlib.c, so the entry points are wired
   * up for free. Cheap -- one atomic load and a compare -- unless the
   * configuration actually moved. No-op while a setter on this thread is
   * mid-apply. */
  swi_config_sync(ctx);
  return started;
}

/* closes all open files, frees space of planetary data, 
 * deletes memory of all computed positions 
 */
static void swi_close_keep_topo_etc(swe_ctx *ctx) 
{
  int i;
  /* close SWISSEPH files */
  for (i = 0; i < SEI_NEPHFILES; i ++) {
    if (ctx->fidat[i].fptr != NULL) 
      fclose(ctx->fidat[i].fptr);
    memset((void *) &ctx->fidat[i], 0, sizeof(struct file_data));
  }
  free_planets(ctx);
  memset((void *) &ctx->oec, 0, sizeof(struct epsilon));
  memset((void *) &ctx->oec2000, 0, sizeof(struct epsilon));
  memset((void *) &ctx->nut, 0, sizeof(struct nut));
  memset((void *) &ctx->nut2000, 0, sizeof(struct nut));
  memset((void *) &ctx->nutv, 0, sizeof(struct nut));
  memset((void *) &ctx->astro_models, 0, SEI_NMODELS * sizeof(int32));
  /* close JPL file */
  swi_close_jpl_file(ctx);
  ctx->jpl_file_is_open = FALSE;
  ctx->jpldenum = 0;
  /* close fixed stars */
  if (ctx->fixfp != NULL) {
    fclose(ctx->fixfp);
    ctx->fixfp = NULL;
  }
  swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC);
  ctx->is_old_starfile = FALSE;
  ctx->i_saved_planet_name = 0;
  *(ctx->saved_planet_name) = '\0';
  ctx->timeout = 0;
}

/* closes all open files, frees space of planetary data, 
 * deletes memory of all computed positions 
 */
/* Release every resource a context holds: open ephemeris and star files,
 * the JPL file, the planet save areas, the EOP and fixed-star tables, and
 * the cached angles. Shared by swe_close() and swe_ctx_free().
 *
 * Deliberately NOT here: swi_config_reset(), which drops the process-wide
 * config master. That is right for swe_close() -- documented as releasing
 * everything the library holds -- and wrong for swe_ctx_free(), where
 * freeing one context must not reconfigure every other one. */
static void ctx_release(swe_ctx *ctx)
{
  int i;
  /* close SWISSEPH files */
  for (i = 0; i < SEI_NEPHFILES; i ++) {
    if (ctx->fidat[i].fptr != NULL) 
      fclose(ctx->fidat[i].fptr);
    memset((void *) &ctx->fidat[i], 0, sizeof(struct file_data));
  }
  free_planets(ctx);
  memset((void *) &ctx->oec, 0, sizeof(struct epsilon));
  memset((void *) &ctx->oec2000, 0, sizeof(struct epsilon));
  memset((void *) &ctx->nut, 0, sizeof(struct nut));
  memset((void *) &ctx->nut2000, 0, sizeof(struct nut));
  memset((void *) &ctx->nutv, 0, sizeof(struct nut));
  memset((void *) &ctx->astro_models, 0, SEI_NMODELS * sizeof(int32));
  /* close JPL file */
  swi_close_jpl_file(ctx);
  ctx->jpl_file_is_open = FALSE;
  ctx->jpldenum = 0;
  /* close fixed stars */
  if (ctx->fixfp != NULL) {
    fclose(ctx->fixfp);
    ctx->fixfp = NULL;
  }
  SWI_CFG_LOCAL(ctx, swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC));
  ctx->geopos_is_set = FALSE;
  ctx->ayana_is_set = FALSE;
  ctx->is_old_starfile = FALSE;
  ctx->i_saved_planet_name = 0;
  *(ctx->saved_planet_name) = '\0';
  memset((void *) &ctx->topd, 0, sizeof(struct topo_data));
  memset((void *) &ctx->sidd, 0, sizeof(struct sid_data));
  ctx->timeout = 0;
  ctx->last_epheflag = 0;
  if (ctx->dpsi != NULL) {
    free(ctx->dpsi);
    ctx->dpsi = NULL;
  }
  if (ctx->deps != NULL) {
    free(ctx->deps);
    ctx->deps = NULL;
  }
  if (ctx->n_fixstars_records > 0) {
    free(ctx->fixed_stars);
    ctx->fixed_stars = NULL;
    ctx->n_fixstars_real = 0;
    ctx->n_fixstars_named = 0;
    ctx->n_fixstars_records = 0;
  }
/*  swed.ephe_path_is_set = FALSE;
  *swed.ephepath = '\0'; */
  /* Drop the shared master too.
   *
   * swe_close() is documented as releasing everything the library holds.
   * Without this the master outlives it, and a thread starting afterwards
   * would silently adopt the configuration of a session that had already
   * been closed -- reopening files the caller believed were released.
   *
   * This is process-wide, which matches what swe_close() already means
   * for a library whose configuration is process-wide. A caller that
   * closes on one thread while another is still computing is misusing the
   * API in exactly the way it always has been. */
}

/* Releases this context's resources. Deliberately does NOT reset the
 * shared configuration master -- see the note on swe_close() below. */
void CALL_CONV swe_close_r(swe_ctx *ctx)
{
  ctx_release(ctx);
#ifdef TRACE
#define TRACE_CLOSE FALSE
  swi_open_trace(NULL);
  swi_trace_lock();
  if (swi_fp_trace_c != NULL) {
    if (swi_trace_count < TRACE_COUNT_MAX) {
      fputs("\n/*SWE_CLOSE*/\n", swi_fp_trace_c);
      fputs("  swe_close();\n", swi_fp_trace_c);
#if TRACE_CLOSE
      fputs("}\n", swi_fp_trace_c);
#endif
      fflush(swi_fp_trace_c);
    }
#if TRACE_CLOSE
    fclose(swi_fp_trace_c);
#endif
  }
#if TRACE_CLOSE
  if (swi_fp_trace_out != NULL)
    fclose(swi_fp_trace_out);
  swi_fp_trace_c = NULL;
  swi_fp_trace_out = NULL;
#endif
  swi_trace_unlock();
#endif  /* TRACE */
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
/* swe_close() keeps its historical meaning: release everything the LIBRARY
 * holds, the process-wide configuration master included.
 *
 * swe_close_r() deliberately does not do that second part, and the split
 * is not cosmetic. swed is TLS, so every thread has its own default
 * context whose ephemeris segments are freed only by a close ON THAT
 * THREAD -- roughly 9 KB per worker, measured with LeakSanitizer. Before
 * the split a worker had no way to release its own memory: the only close
 * available also wiped the configuration every other thread was reading,
 * so cleaning up after yourself broke your neighbours.
 *
 * A worker thread should now call swe_close_r(swi_default_ctx()) to
 * release its own state, and swe_close() only when shutting the library
 * down for the whole process. */
void CALL_CONV swe_close(void)
{
  swe_ctx *ctx = swi_default_ctx();
  swe_close_r(ctx);
  swi_config_reset(ctx);
}

/* sets ephemeris file path. 
 * also calls swe_close(). this makes sure that swe_calc()
 * won't return planet positions previously computed from other
 * ephemerides
 */
void CALL_CONV swe_set_ephe_path_r(swe_ctx *ctx, const char *path)
{
  AS_BOOL swi_cfg_was = swi_config_begin_apply(ctx);
  int i, iflag;
  char s[AS_MAXCH];
  char serr[AS_MAXCH];
  char *sp;
  double xx[6];
  /* close all open files and delete all planetary data */
  swi_close_keep_topo_etc(ctx);
  swi_init_swed_if_start(ctx);
  ctx->ephe_path_is_set = TRUE;
  /* environment variable SE_EPHE_PATH has priority */
  if ((sp = getenv("SE_EPHE_PATH")) != NULL 
    && strlen(sp) != 0
    && strlen(sp) <= AS_MAXCH-1-13) {
    strcpy(s, sp);
  } else if (path == NULL || *path == '\0') {
    strcpy(s, SE_EPHE_PATH);
  } else if (strlen(path) <= AS_MAXCH-1-13) {
    strcpy(s, path);
  } else {
    strcpy(s, SE_EPHE_PATH);
  }
  i = (int) strlen(s);
  if (*(s + i - 1) != *DIR_GLUE && *s != '\0')
    strcat(s, DIR_GLUE);
  strcpy(ctx->ephepath, s);
//swe_set_interpolate_nut(TRUE);
  /* try to open lunar ephemeris, in order to get DE number and set
   * tidal acceleration of the Moon */
  iflag = SEFLG_SWIEPH|SEFLG_J2000|SEFLG_TRUEPOS|SEFLG_ICRS;
  ctx->last_epheflag = 2;
  swe_calc_r(ctx, J2000, SE_MOON, iflag, xx, serr);
  if (ctx->fidat[SEI_FILE_MOON].fptr != NULL) {
    swi_set_tid_acc(ctx, 0, 0, ctx->fidat[SEI_FILE_MOON].sweph_denum, NULL);
  } 
#ifdef TRACE
  swi_open_trace(NULL);
  swi_trace_lock();
  if (swi_trace_count < TRACE_COUNT_MAX) {
    if (swi_fp_trace_c != NULL) {
      fputs("\n/*SWE_SET_EPHE_PATH*/\n", swi_fp_trace_c);
      if (path == NULL) {
        fputs("  *s = '\\0';\n", swi_fp_trace_c);
      } else {
	fprintf(swi_fp_trace_c, "  strcpy(s, \"%s\");\n", path);
      }
      fputs("  swe_set_ephe_path(s);\n", swi_fp_trace_c);
      fputs("  printf(\"swe_set_ephe_path: path_in = \");", swi_fp_trace_c);
      fputs("  printf(s);\n", swi_fp_trace_c);
      fputs("  \tprintf(\"\\tpath_set = unknown to swetrace\\n\"); /* unknown to swetrace */\n", swi_fp_trace_c);
      fflush(swi_fp_trace_c);
    }
    if (swi_fp_trace_out != NULL) {
      fputs("swe_set_ephe_path: path_in = ", swi_fp_trace_out);
      if (path != NULL)
	fputs(path, swi_fp_trace_out);
      fputs("\tpath_set = ", swi_fp_trace_out);
      fputs(s, swi_fp_trace_out);
      fputs("\n", swi_fp_trace_out);
      fflush(swi_fp_trace_out);
    }
  }
  swi_trace_unlock();
#endif
  swi_config_end_apply(ctx, swi_cfg_was);
  swi_config_publish(ctx, SWI_CFG_PATH);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
void CALL_CONV swe_set_ephe_path(const char *path)
{
  swe_set_ephe_path_r(swi_default_ctx(), path);
}

void load_dpsi_deps(swe_ctx *ctx)
{
  FILE *fp;
  char s[AS_MAXCH];
  char *cpos[20];
  int n = 0, iyear, mjd = 0, mjdsv = 0;
  double dpsi, deps, TJDOFS = 2400000.5;
  if (ctx->eop_dpsi_loaded > 0) 
    return;
  fp = swi_fopen(ctx, -1, DPSI_DEPS_IAU1980_FILE_EOPC04, ctx->ephepath, NULL);
  if (fp == NULL) {
    ctx->eop_dpsi_loaded = ERR;
    return;
  }
  if ((ctx->dpsi = (double *) calloc((size_t) SWE_DATA_DPSI_DEPS, sizeof(double))) == NULL) {
    ctx->eop_dpsi_loaded = ERR;
    return;
  }
  if ((ctx->deps = (double *) calloc((size_t) SWE_DATA_DPSI_DEPS, sizeof(double))) == NULL) {
    ctx->eop_dpsi_loaded = ERR;
    return;
  }
  ctx->eop_tjd_beg_horizons = DPSI_DEPS_IAU1980_TJD0_HORIZONS;
  while (fgets(s, AS_MAXCH, fp) != NULL) {
    swi_cutstr(s, " ", cpos, 16);
    if ((iyear = atoi(cpos[0])) == 0) 
      continue;
    mjd = atoi(cpos[3]);
    /* is file in one-day steps? */
    if (mjdsv > 0 && mjd - mjdsv != 1) {
      /* we cannot return error but we note it as follows: */
      ctx->eop_dpsi_loaded = -2;
      fclose(fp);
      return;
    }
    if (n == 0)
      ctx->eop_tjd_beg = mjd + TJDOFS;
    ctx->dpsi[n] = atof(cpos[8]);
    ctx->deps[n] = atof(cpos[9]);
/*    fprintf(stderr, "n=%d, tjd=%f, dpsi=%f, deps=%f\n", n, mjd + 2400000.5, swed.dpsi[n] * 1000, swed.deps[n] * 1000);exit(0);*/
    n++;
    mjdsv = mjd;
  }
  ctx->eop_tjd_end = mjd + TJDOFS;
  ctx->eop_dpsi_loaded = 1;
  fclose(fp);
  /* file finals.all may have some more data, and especially estimations 
   * for the near future */
  fp = swi_fopen(ctx, -1, DPSI_DEPS_IAU1980_FILE_FINALS, ctx->ephepath, NULL);
  if (fp == NULL) 
    return; /* return without error as existence of file is not mandatory */
  while (fgets(s, AS_MAXCH, fp) != NULL) {
    mjd = atoi(s + 7);
    if (mjd + TJDOFS <= ctx->eop_tjd_end)
      continue;
    if (n >= SWE_DATA_DPSI_DEPS)
      return;
    /* are data in one-day steps? */
    if (mjdsv > 0 && mjd - mjdsv != 1) {
      /* no error, as we do have data; however, if this file is usefull,
       * then swed.eop_dpsi_loaded will be set to 2 */
      ctx->eop_dpsi_loaded = -3;
      fclose(fp);
      return;
    }
    /* dpsi, deps Bulletin B */
    dpsi = atof(s + 168);
    deps = atof(s + 178);
    if (dpsi == 0) {
      /* try dpsi, deps Bulletin A */
      dpsi = atof(s + 99);
      deps = atof(s + 118);
    }
    if (dpsi == 0) {
      ctx->eop_dpsi_loaded = 2;
      /*printf("dpsi from %f to %f \n", swed.eop_tjd_beg, swed.eop_tjd_end);*/
      fclose(fp);
      return;
    }
    ctx->eop_tjd_end = mjd + TJDOFS;
    ctx->dpsi[n] = dpsi / 1000.0;
    ctx->deps[n] = deps / 1000.0;
    /*fprintf(stderr, "tjd=%f, dpsi=%f, deps=%f\n", mjd + 2400000.5, swed.dpsi[n] * 1000, swed.deps[n] * 1000);*/
    n++;
    mjdsv = mjd;
  }
  ctx->eop_dpsi_loaded = 2;
  fclose(fp);
}

/* sets jpl file name.
 * also calls swe_close(). this makes sure that swe_calc()
 * won't return planet positions previously computed from other
 * ephemerides
 */
void CALL_CONV swe_set_jpl_file_r(swe_ctx *ctx, const char *fname)
{
  AS_BOOL swi_cfg_was = swi_config_begin_apply(ctx);
  char *sp, s[AS_MAXCH];
  int retc;
  double ss[3];
  /* close all open files and delete all planetary data */
  swi_close_keep_topo_etc(ctx);
  swi_init_swed_if_start(ctx);
  /* if path is contained in fname, it is filled into the path variable */
  if (strlen(fname) >= AS_MAXCH) {
     strncpy(s, fname, AS_MAXCH - 1);
     s[AS_MAXCH - 1] = '\0';
  } else {
    strcpy(s, fname);
  }
  sp = strrchr(s, (int) *DIR_GLUE);
  if (sp == NULL) {
    sp = s;
  } else {
    sp = sp + 1;
  }
  if (strlen(sp) >= AS_MAXCH)
    sp[AS_MAXCH - 1] = '\0';
  strcpy(ctx->jplfnam, sp);
  /* open ephemeris */
  retc = open_jpl_file(ctx, ss, ctx->jplfnam, ctx->ephepath, NULL);
  if (retc == OK) {
    if (ctx->jpldenum >= 403) {
      /*if (INCLUDE_CODE_FOR_DPSI_DEPS_IAU1980) */
	load_dpsi_deps(ctx);
    }
  }
#ifdef TRACE
  swi_open_trace(NULL);
  swi_trace_lock();
  if (swi_trace_count < TRACE_COUNT_MAX) {
    if (swi_fp_trace_c != NULL) {
      fputs("\n/*SWE_SET_JPL_FILE*/\n", swi_fp_trace_c);
      fprintf(swi_fp_trace_c, "  strcpy(s, \"%s\");\n", fname);
      fputs("  swe_set_jpl_file(s);\n", swi_fp_trace_c);
      fputs("  printf(\"swe_set_jpl_file: fname_in = \");", swi_fp_trace_c);
      fputs("  printf(s);\n", swi_fp_trace_c);
      fputs("  printf(\"\\tfname_set = unknown to swetrace\\n\");  /* unknown to swetrace */\n", swi_fp_trace_c);
      fflush(swi_fp_trace_c);
    }
    if (swi_fp_trace_out != NULL) {
      fputs("swe_set_jpl_file: fname_in = ", swi_fp_trace_out);
      fputs(fname, swi_fp_trace_out);
      fputs("\tfname_set = ", swi_fp_trace_out);
      fputs(sp, swi_fp_trace_out);
      fputs("\n", swi_fp_trace_out);
      fflush(swi_fp_trace_out);
    }
  }
  swi_trace_unlock();
#endif
  swi_config_end_apply(ctx, swi_cfg_was);
  swi_config_publish(ctx, SWI_CFG_PATH);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
void CALL_CONV swe_set_jpl_file(const char *fname)
{
  swe_set_jpl_file_r(swi_default_ctx(), fname);
}

/* calculates obliquity of ecliptic and stores it together
 * with its date, sine, and cosine
 */
static void calc_epsilon(swe_ctx *ctx, double tjd, int32 iflag, struct epsilon *e)
{
    e->teps = tjd;
    e->eps = swi_epsiln(ctx, tjd, iflag);
    e->seps = sin(e->eps);
    e->ceps = cos(e->eps);
}

/* computes a main planet from any ephemeris, if it 
 * has not yet been computed for this date.
 * since a geocentric position requires the earth, the
 * earth's position will be computed as well. With SWISSEPH
 * files the barycentric sun will be done as well.
 * With Moshier, the moon will be done as well.
 *
 * tjd 		= julian day
 * ipli		= body number
 * epheflag	= which ephemeris? JPL, SWISSEPH, Moshier?
 * iflag	= other flags
 *
 * the geocentric apparent position of ipli (or whatever has
 * been specified in iflag) will be saved in
 * &swed.pldat[ipli].xreturn[];
 *
 * the barycentric (heliocentric with Moshier) position J2000
 * will be kept in 
 * &swed.pldat[ipli].x[];
 */
static int main_planet(swe_ctx *ctx, double tjd, int ipli, int iplmoon, int32 epheflag, int32 iflag,
		       char *serr)
{
  int retc;
  if ((iflag & SEFLG_CENTER_BODY) 
    && ipli >= SE_MARS && ipli <= SE_PLUTO) {
    //ipli_com = ipli * 100 + 9099;
    /* jupiter center of body, relative to jupiter barycenter */
    retc = sweph(ctx, tjd, iplmoon, SEI_FILE_ANY_AST, iflag, NULL, DO_SAVE, NULL, serr);
    if (retc == ERR || retc == NOT_AVAILABLE) 
      return ERR;
  }
  switch(epheflag) {
    case SEFLG_JPLEPH:
      retc = jplplan(ctx, tjd, ipli, iflag, DO_SAVE, NULL, NULL, NULL, serr);
      /* read error or corrupt file */
      if (retc == ERR) 
	return ERR;
      /* jpl ephemeris not on disk or date beyond ephemeris range */
      if (retc == NOT_AVAILABLE) {
	iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_SWIEPH;
	if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	  strcat(serr, " \ntrying Swiss Eph; ");
	goto sweph_planet;
      } else if (retc == BEYOND_EPH_LIMITS) {
	if (tjd > MOSHPLEPH_START && tjd < MOSHPLEPH_END) {
	  iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_MOSEPH;
	  if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	    strcat(serr, " \nusing Moshier Eph; ");
	  goto moshier_planet;
	} else {
	  return ERR;
	}
      }
      /* geocentric, lighttime etc. */
      if (ipli == SEI_SUN) {
	retc = app_pos_etc_sun(ctx, iflag, serr)/**/;
      } else {
	retc = app_pos_etc_plan(ctx, ipli, iplmoon, iflag, serr);
      }
      if (retc == ERR)
	return ERR;
      /* t for light-time beyond ephemeris range */
      if (retc == NOT_AVAILABLE) {
	iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_SWIEPH;
	if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	  strcat(serr, " \ntrying Swiss Eph; ");
	goto sweph_planet;
      } else if (retc == BEYOND_EPH_LIMITS) {
	if (tjd > MOSHPLEPH_START && tjd < MOSHPLEPH_END) {
	  iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_MOSEPH;
	  if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	    strcat(serr, " \nusing Moshier Eph; ");
	  goto moshier_planet;
	} else
	  return ERR;
      }
      break;
    case SEFLG_SWIEPH:
      sweph_planet:
      /* compute barycentric planet (+ earth, sun, moon) */
      retc = sweplan(ctx, tjd, ipli, SEI_FILE_PLANET, iflag, DO_SAVE, NULL, NULL, NULL, NULL, serr);
      if (retc == ERR)
	return ERR;
      /* if sweph file not found, switch to moshier */
      if (retc == NOT_AVAILABLE) {
	if (tjd > MOSHPLEPH_START && tjd < MOSHPLEPH_END) {
	  iflag = (iflag & ~SEFLG_SWIEPH) | SEFLG_MOSEPH;
	  if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	    strcat(serr, " \nusing Moshier eph.; ");
	  goto moshier_planet;
	} else 
	  return ERR;
      }
      /* geocentric, lighttime etc. */
      if (ipli == SEI_SUN) {
	retc = app_pos_etc_sun(ctx, iflag, serr)/**/;
      } else {
	retc = app_pos_etc_plan(ctx, ipli, iplmoon, iflag, serr);
      }
      if (retc == ERR)
	return ERR;
      /* if sweph file for t(lighttime) not found, switch to moshier */
      if (retc == NOT_AVAILABLE) {
	if (tjd > MOSHPLEPH_START && tjd < MOSHPLEPH_END) {
	  iflag = (iflag & ~SEFLG_SWIEPH) | SEFLG_MOSEPH;
	  if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	    strcat(serr, " \nusing Moshier eph.; ");
	  goto moshier_planet;
	} else
	  return ERR;
      }
      break;
    case SEFLG_MOSEPH:
      moshier_planet:
      retc = swi_moshplan(ctx, tjd, ipli, DO_SAVE, NULL, NULL, serr);/**/
      if (retc == ERR)
	return ERR;
      /* geocentric, lighttime etc. */
      if (ipli == SEI_SUN) {
	retc = app_pos_etc_sun(ctx, iflag, serr)/**/;
      } else {
	retc = app_pos_etc_plan(ctx, ipli, iplmoon, iflag, serr);
      }
      if (retc == ERR)
	return ERR;
      break;
    default:
      break;
  } 
  return OK;
}

/* Computes a main planet from any ephemeris or returns
 * it again, if it has been computed before.
 * In barycentric equatorial position of the J2000 equinox.
 * The earth's position is computed as well. With SWISSEPH
 * and JPL ephemeris the barycentric sun is computed, too.
 * With Moshier, the moon is returned, as well.
 *
 * tjd 		= julian day
 * ipli		= body number
 * epheflag	= which ephemeris? JPL, SWISSEPH, Moshier?
 * iflag	= other flags
 * xp, xe, xs, and xm are the pointers, where the program
 * either finds or stores (if not found) the barycentric 
 * (heliocentric with Moshier) positions of the following 
 * bodies:
 * xp		planet
 * xe		earth
 * xs		sun
 * xm		moon
 * 
 * xm is used with Moshier only 
 */
static int main_planet_bary(swe_ctx *ctx, double tjd, int ipli, int32 epheflag, int32 iflag, AS_BOOL do_save,
		       double *xp, double *xe, double *xs, double *xm, 
		       char *serr)
{
  int i, retc;
  switch(epheflag) {
    case SEFLG_JPLEPH:
      retc = jplplan(ctx, tjd, ipli, iflag, do_save, xp, xe, xs, serr);
      /* read error or corrupt file */
      if (retc == ERR || retc == BEYOND_EPH_LIMITS) 
	return retc;
      /* jpl ephemeris not on disk or date beyond ephemeris range */
      if (retc == NOT_AVAILABLE) {
	iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_SWIEPH;
	if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	  strcat(serr, " \ntrying Swiss Eph; ");
	goto sweph_planet;
      }
      break;
    case SEFLG_SWIEPH:
      sweph_planet:
      /* compute barycentric planet (+ earth, sun, moon) */
      retc = sweplan(ctx, tjd, ipli, SEI_FILE_PLANET, iflag, do_save, xp, xe, xs, xm, serr);
   /* if barycentric moshier calculation were implemented */
      if (retc == ERR)
	return ERR;
      /* if sweph file not found, switch to moshier */
      if (retc == NOT_AVAILABLE) {
	if (tjd > MOSHPLEPH_START && tjd < MOSHPLEPH_END) {
	  iflag = (iflag & ~SEFLG_SWIEPH) | SEFLG_MOSEPH;
	  if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	    strcat(serr, " \nusing Moshier eph.; ");
	  goto moshier_planet;
	} else {
	  return ERR;
	}
      }
      break;
    case SEFLG_MOSEPH:
      moshier_planet:
      retc = swi_moshplan(ctx, tjd, ipli, do_save, xp, xe, serr);/**/
      if (retc == ERR)
	return ERR;
      for (i = 0; i <= 5; i++)
	xs[i] = 0;
      break;
    default:
      break;
  } 
  return OK;
}

/* SWISSEPH 
 * this routine computes heliocentric cartesian equatorial coordinates
 * of equinox 2000 of
 * geocentric moon
 * 
 * tjd 		julian date
 * iflag	flag
 * do_save	save J2000 position in save area pdp->x ?
 * xp		array of 6 doubles for lunar position and speed
 * serr		error string
 */
static int swemoon(swe_ctx *ctx, double tjd, int32 iflag, AS_BOOL do_save, double *xpret, char *serr)
{
  int i, retc;
  struct plan_data *pdp = &ctx->pldat[SEI_MOON];
  int32 speedf1, speedf2;
  double xx[6], *xp;
  if (do_save) {
    xp = pdp->x;
  } else {
    xp = xx;
  }
  /* if planet has already been computed for this date, return 
   * if speed flag has been turned on, recompute planet */
  speedf1 = pdp->xflgs & SEFLG_SPEED;
  speedf2 = iflag & SEFLG_SPEED;
  if (tjd == pdp->teval 
	&& pdp->iephe == SEFLG_SWIEPH
	&& (!speedf2 || speedf1)) {
    xp = pdp->x;
  } else {
    /* call sweph for moon */
    retc = sweph(ctx, tjd, SEI_MOON, SEI_FILE_MOON, iflag, NULL, do_save, xp, serr);
    if (retc != OK)
      return(retc);
    if (do_save) {
      pdp->teval = tjd;
      pdp->xflgs = -1;
      pdp->iephe = SEFLG_SWIEPH;
    }
  }
  if (xpret != NULL)
    for (i = 0; i <= 5; i++)
      xpret[i] = xp[i];
  return(OK);
}

/* SWISSEPH 
 * this function computes 
 * 1. a barycentric planet 
 * plus, under certain conditions, 
 * 2. the barycentric sun, 
 * 3. the barycentric earth, and 
 * 4. the geocentric moon,
 * in barycentric cartesian equatorial coordinates J2000.
 *
 * these are the data needed for calculation of light-time etc.
 *
 * tjd 		julian date
 * ipli		SEI_ planet number
 * ifno		ephemeris file number
 * do_save	write new positions in save area
 * xp		array of 6 doubles for planet's position and velocity
 * xpe                                 earth's  
 * xps                                 sun's
 * xpm                                 moon's
 * serr		error string
 *
 * xp - xpm can be NULL. if do_save is TRUE, all of them can be NULL.
 * the positions will be written into the save area (swed.pldat[ipli].x)
 */
static int sweplan(swe_ctx *ctx, double tjd, int ipli, int ifno, int32 iflag, AS_BOOL do_save,
		   double *xpret, double *xperet, double *xpsret, double *xpmret,
		   char *serr)
{
  int i, retc;
  int do_earth = FALSE, do_moon = FALSE, do_sunbary = FALSE;
  struct plan_data *pdp = &ctx->pldat[ipli];
  struct plan_data *pebdp = &ctx->pldat[SEI_EMB];
  struct plan_data *psbdp = &ctx->pldat[SEI_SUNBARY];
  struct plan_data *pmdp = &ctx->pldat[SEI_MOON];
  double xxp[6], xxm[6], xxs[6], xxe[6];
  double *xp, *xpe, *xpm, *xps;
  int32 speedf1, speedf2;
  /* xps (barycentric sun) may be necessary because some planets on sweph 
   * file are heliocentric, other ones are barycentric. without xps, 
   * the heliocentric ones cannot be returned barycentrically.
   */
  if (do_save || ipli == SEI_SUNBARY || (pdp->iflg & SEI_FLG_HELIO) 
    || xpsret != NULL || (iflag & SEFLG_HELCTR)) 
    do_sunbary = TRUE;
  if (do_save || ipli == SEI_EARTH || xperet != NULL)
    do_earth = TRUE;
  if (ipli == SEI_MOON) { 
    do_earth = TRUE;
    do_sunbary = TRUE;
  }
  if (do_save || ipli == SEI_MOON || ipli == SEI_EARTH || xperet != NULL || xpmret != NULL)
    do_moon = TRUE;
  if (do_save)  {
    xp = pdp->x;
    xpe = pebdp->x;
    xps = psbdp->x;
    xpm = pmdp->x;
  } else {
    xp = xxp;
    xpe = xxe;
    xps = xxs;
    xpm = xxm;
  }
  speedf2 = iflag & SEFLG_SPEED;
  /* barycentric sun */
  if (do_sunbary) {
    speedf1 = psbdp->xflgs & SEFLG_SPEED;
    /* if planet has already been computed for this date, return 
     * if speed flag has been turned on, recompute planet */
    if (tjd == psbdp->teval 
	  && psbdp->iephe == SEFLG_SWIEPH
	  && (!speedf2 || speedf1)) {
      for (i = 0; i <= 5; i++)
	xps[i] = psbdp->x[i];
    } else {
      retc = sweph(ctx, tjd, SEI_SUNBARY, SEI_FILE_PLANET, iflag, NULL, do_save, xps, serr);/**/
      if (retc != OK)
	return(retc);
    }
    if (xpsret != NULL)
      for (i = 0; i <= 5; i++)
	xpsret[i] = xps[i];
  }
  /* moon */
  if (do_moon) {
    speedf1 = pmdp->xflgs & SEFLG_SPEED;
    if (tjd == pmdp->teval 
	  && pmdp->iephe == SEFLG_SWIEPH
	  && (!speedf2 || speedf1)) {
      for (i = 0; i <= 5; i++)
	xpm[i] = pmdp->x[i];
    } else {
      retc = sweph(ctx, tjd, SEI_MOON, SEI_FILE_MOON, iflag, NULL, do_save, xpm, serr);
      if (retc == ERR) 
	return(retc);
      /* if moon file doesn't exist, take moshier moon */
      if (ctx->fidat[SEI_FILE_MOON].fptr == NULL) {
	if (serr != NULL && strlen(serr) + 35 < AS_MAXCH)
	  strcat(serr, " \nusing Moshier eph. for moon; ");
	retc = swi_moshmoon(ctx, tjd, do_save, xpm, serr);
	if (retc != OK)
	  return(retc);
      }
    }
    if (xpmret != NULL)
      for (i = 0; i <= 5; i++)
	xpmret[i] = xpm[i];
  }
  /* barycentric earth */
  if (do_earth) {
    speedf1 = pebdp->xflgs & SEFLG_SPEED;
    if (tjd == pebdp->teval 
	  && pebdp->iephe == SEFLG_SWIEPH
	  && (!speedf2 || speedf1)) {
      for (i = 0; i <= 5; i++)
	xpe[i] = pebdp->x[i];
    } else {
      retc = sweph(ctx, tjd, SEI_EMB, SEI_FILE_PLANET, iflag, NULL, do_save, xpe, serr);
      if (retc != OK)
	return(retc);
      /* earth from emb and moon */
      embofs(xpe, xpm);
      /* speed is needed, if
       * 1. true position is being computed before applying light-time etc.
       *    this is the position saved in pdp->x.
       *    in this case, speed is needed for light-time correction.
       * 2. the speed flag has been specified.
       */
      if (xpe == pebdp->x || (iflag & SEFLG_SPEED))
	embofs(xpe+3, xpm+3);
    } 
    if (xperet != NULL)
      for (i = 0; i <= 5; i++)
	xperet[i] = xpe[i];
  } 
  if (ipli == SEI_MOON) {
    for (i = 0; i <= 5; i++)
      xp[i] = xpm[i];
  } else if (ipli == SEI_EARTH) {
    for (i = 0; i <= 5; i++)
      xp[i] = xpe[i];
  } else if (ipli == SEI_SUN) {
    for (i = 0; i <= 5; i++)
      xp[i] = xps[i];
  } else {
    /* planet */
    speedf1 = pdp->xflgs & SEFLG_SPEED;
    if (tjd == pdp->teval 
	  && pdp->iephe == SEFLG_SWIEPH
	  && (!speedf2 || speedf1)) {
      for (i = 0; i <= 5; i++)
	xp[i] = pdp->x[i];
      return(OK);
    } else {
      retc = sweph(ctx, tjd, ipli, ifno, iflag, NULL, do_save, xp, serr);
      if (retc != OK)
	return(retc);
      /* if planet is heliocentric, it must be transformed to barycentric */
      if (pdp->iflg & SEI_FLG_HELIO) {
	/* now barycentric planet */
	for (i = 0; i <= 2; i++) 
	  xp[i] += xps[i];
	if (do_save || (iflag & SEFLG_SPEED))
	  for (i = 3; i <= 5; i++) 
	    xp[i] += xps[i];
      }
    }
  }
  if (xpret != NULL)
    for (i = 0; i <= 5; i++)
      xpret[i] = xp[i];
  return(OK);
}

/* jpl ephemeris.
 * this function computes 
 * 1. a barycentric planet position
 * plus, under certain conditions,
 * 2. the barycentric sun, 
 * 3. the barycentric earth,
 * in barycentric cartesian equatorial coordinates J2000.

 * tjd		julian day
 * ipli		sweph internal planet number
 * do_save	write new positions in save area
 * xp		array of 6 doubles for planet's position and speed vectors
 * xpe		                       earth's
 * xps		                       sun's
 * serr		pointer to error string
 *
 * xp - xps can be NULL. if do_save is TRUE, all of them can be NULL.
 * the positions will be written into the save area (swed.pldat[ipli].x)
 */
static int jplplan(swe_ctx *ctx, double tjd, int ipli, int32 iflag, AS_BOOL do_save,
		   double *xpret, double *xperet, double *xpsret, char *serr)
{
  int i, retc;
  AS_BOOL do_earth = FALSE, do_sunbary = FALSE;
  double ss[3];
  double xxp[6], xxe[6], xxs[6];
  double *xp, *xpe, *xps;
  int ictr = J_SBARY;
  struct plan_data *pdp = &ctx->pldat[ipli];
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  (void) iflag; /* currently not used */
  /* we assume Teph ~= TDB ~= TT. The maximum error is < 0.002 sec, 
   * corresponding to an ephemeris error < 0.001 arcsec for the moon */
  /* double tjd_tdb, T;
     T = (tjd - 2451545.0)/36525.0;
     tjd_tdb = tjd + (0.001657 * sin(628.3076 * T + 6.2401)
		+ 0.000022 * sin(575.3385 * T + 4.2970)
		+ 0.000014 * sin(1256.6152 * T + 6.1969)) / 8640.0;*/
  if (do_save) {
    xp = pdp->x;
    xpe = pedp->x;
    xps = psdp->x;
  } else {
    xp = xxp;
    xpe = xxe;
    xps = xxs;
  }
  if (do_save || ipli == SEI_EARTH || xperet != NULL 
    || (ipli == SEI_MOON)) /* && (iflag & (SEFLG_HELCTR | SEFLG_BARYCTR | SEFLG_NOABERR)))) */
    do_earth = TRUE;
  if (do_save || ipli == SEI_SUNBARY || xpsret != NULL 
    || (ipli == SEI_MOON)) /* && (iflag & (SEFLG_HELCTR | SEFLG_NOABERR)))) */
    do_sunbary = TRUE;
  if (ipli == SEI_MOON)
    ictr = J_EARTH;
  /* open ephemeris, if still closed */
  if (!ctx->jpl_file_is_open) {
    retc = open_jpl_file(ctx, ss, ctx->jplfnam, ctx->ephepath, serr);
    if (retc != OK)
      return (retc);
  }
  if (do_earth) {
    /* barycentric earth */
    if (tjd != pedp->teval || tjd == 0) {
      retc = swi_pleph(ctx, tjd, J_EARTH, J_SBARY, xpe, serr);
      if (do_save) {
	pedp->teval = tjd;
	pedp->xflgs = -1;	/* new light-time etc. required */
	pedp->iephe = SEFLG_JPLEPH;
      }
      if (retc != OK) {
	swi_close_jpl_file(ctx);
	ctx->jpl_file_is_open = FALSE;
	return retc;
      }
    } else {
      xpe = pedp->x;
    }
    if (xperet != NULL)
      for (i = 0; i <= 5; i++)
	xperet[i] = xpe[i];
      
  } 
  if (do_sunbary) {
    /* barycentric sun */
    if (tjd != psdp->teval || tjd == 0) {
      retc = swi_pleph(ctx, tjd, J_SUN, J_SBARY, xps, serr);
      if (do_save) {
	psdp->teval = tjd;
	psdp->xflgs = -1;
	psdp->iephe = SEFLG_JPLEPH;
      }
      if (retc != OK) {
	swi_close_jpl_file(ctx);
	ctx->jpl_file_is_open = FALSE;
	return retc;
      }
    } else {
      xps = psdp->x;
    }
    if (xpsret != NULL)
      for (i = 0; i <= 5; i++)
	xpsret[i] = xps[i];
  }
  /* earth is wanted */
  if (ipli == SEI_EARTH) {
    for (i = 0; i <= 5; i++)
      xp[i] = xpe[i];
  /* sunbary is wanted */
  } if (ipli == SEI_SUNBARY) {
    for (i = 0; i <= 5; i++)
      xp[i] = xps[i];
  /* other planet */
  } else {
    /* if planet already computed */
    if (tjd == pdp->teval && pdp->iephe == SEFLG_JPLEPH) {
      xp = pdp->x;
    } else {
      retc = swi_pleph(ctx, tjd, pnoint2jpl[ipli], ictr, xp, serr);
      if (do_save) {
	pdp->teval = tjd;
	pdp->xflgs = -1;
	pdp->iephe = SEFLG_JPLEPH;
      }
      if (retc != OK) {
	swi_close_jpl_file(ctx);
	ctx->jpl_file_is_open = FALSE;
	return retc;
      }
    }
  }
  if (xpret != NULL)
    for (i = 0; i <= 5; i++)
      xpret[i] = xp[i];
  return (OK);
}

/* 
 * this function looks for an ephemeris file, 
 * opens it, if not yet open,
 * reads constants, if not yet read,
 * computes a planet, if not yet computed 
 * attention: asteroids are heliocentric
 *            other planets barycentric
 * 
 * tjd 		julian date
 * ipli		SEI_ planet number
 * ifno		ephemeris file number
 * xsunb	INPUT (!) array of 6 doubles containing barycentric sun
 *              (must be given with asteroids)
 * do_save	boolean: save result in save area
 * xp		return array of 6 doubles for planet's position
 * serr		error string
 */
static int sweph(swe_ctx *ctx, double tjd, int ipli, int ifno, int32 iflag, double *xsunb, AS_BOOL do_save, double *xpret, char *serr)
{
  int i, ipl, retc, subdirlen;
  char s[2 * AS_MAXCH], subdirnam[AS_MAXCH], fname[AS_MAXCH], *sp;
  double t, tsv;       
  double xemb[6], xx[6], *xp;
  struct plan_data *pdp;
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  struct file_data *fdp = &ctx->fidat[ifno];
  int32 speedf1, speedf2;
  AS_BOOL need_speed;
  ipl = ipli;
  if (ipli > SE_AST_OFFSET) 
    ipl = SEI_ANYBODY;
  if (ipli > SE_PLMOON_OFFSET) 
    ipl = SEI_ANYBODY;
  pdp = &ctx->pldat[ipl];
  if (do_save) {
    xp = pdp->x;
  } else {
    xp = xx;
  }
  /* if planet has already been computed for this date, return.
   * if speed flag has been turned on, recompute planet */
  speedf1 = pdp->xflgs & SEFLG_SPEED;
  speedf2 = iflag & SEFLG_SPEED;
  if (tjd == pdp->teval 
	&& pdp->iephe == SEFLG_SWIEPH
	&& (!speedf2 || speedf1)
        && ipl < SEI_ANYBODY) {
    if (xpret != NULL)
      for (i = 0; i <= 5; i++)
	xpret[i] = pdp->x[i];
    return(OK);
  }
  /****************************** 
   * get correct ephemeris file * 
   ******************************/
  if (fdp->fptr != NULL) {
    /* if tjd is beyond file range, close old file.
     * if new asteroid, close old file. */
    if (tjd < fdp->tfstart || tjd > fdp->tfend
      || (ipl == SEI_ANYBODY && ipli != pdp->ibdy)) { 	
      fclose(fdp->fptr);
      fdp->fptr = NULL;
      if (pdp->refep != NULL) 
	free((void *) pdp->refep);
      pdp->refep = NULL;
      if (pdp->segp != NULL)
	free((void *) pdp->segp);
      pdp->segp = NULL;
    }
  }
  /* if sweph file not open, find and open it */
  if (fdp->fptr == NULL) {
    swi_gen_filename(tjd, ipli, fname); 
    strcpy(subdirnam, fname);
    sp = strrchr(subdirnam, (int) *DIR_GLUE);
    if (sp != NULL) {
      *sp = '\0';
      subdirlen = (int) strlen(subdirnam);
    } else {
      subdirlen = 0;
    }
    strcpy(s, fname);
again:
    fdp->fptr = swi_fopen(ctx, ifno, s, ctx->ephepath, serr);
    if (fdp->fptr == NULL) {
      // if it is a planetary moon, also try without the directory "sat/"
      if (ipli > SE_PLMOON_OFFSET && ipli < SE_AST_OFFSET) { 
	if (subdirlen > 0 && strncmp(s, subdirnam, (size_t) subdirlen) == 0) {
	  swi_strcpy(s, s + subdirlen + 1);	/* remove "sat/" etc. */
	  goto again;
	}
      /*
       * if it is a numbered asteroid file, try also for short files (..s.se1)
       * On the second try, the inserted 's' will be seen and not tried again.
       */
      } else if (ipli > SE_AST_OFFSET) { 
	char *spp;
	spp = strchr(s, '.');
	if (spp > s && *(spp-1) != 's') {	/* no 's' before '.' ? */
	  sprintf(spp, "s.%s", SE_FILE_SUFFIX);	/* insert an 's' */
	  goto again;
	}
	/*
	 * if we still have 'ast0' etc. in front of the filename, 
	 * we remove it now, remove the 's' also, 
	 * and try in the main ephemeris directory instead of the 
	 * asteroid subdirectory.
	 */
        spp--;	/* point to the character before '.' which must be a 's' */
	swi_strcpy(spp, spp + 1);	/* remove the s */
	if (subdirlen > 0 && strncmp(s, subdirnam, (size_t) subdirlen) == 0) {
	  swi_strcpy(s, s + subdirlen + 1);	/* remove "ast0/" etc. */
	  goto again;
	}
      }
      return(NOT_AVAILABLE);
    }
    /* during the search error messages may have been built, delete them */
    if (serr != NULL) *serr = '\0';	
    retc = read_const(ctx, ifno, serr);
    if (retc != OK)
      return(retc);
  }
  /* if first ephemeris file (J-3000), it might start a mars period
   * after -3000. if last ephemeris file (J3000), it might end a
   * 4000-day-period before 3000. */
  if (tjd < fdp->tfstart || tjd > fdp->tfend) {
    if (serr != NULL) {
      sp = strrchr(fname, (int) *DIR_GLUE);
      if (sp != NULL) {
        sp++;
      } else {
        sp = fname;
      }
      if (ipli > SE_AST_OFFSET) {
        sprintf(s, "asteroid No. %d (%s): ", ipli - SE_AST_OFFSET, sp);
      } else if (ipli > SE_PLMOON_OFFSET) {
	if (strstr(fname, "99.") != NULL) 
	  sprintf(s, "plan. COB No. %d (%s): ", ipli, sp);
	else
	  sprintf(s, "plan. moon No. %d (%s): ", ipli, sp);
      } else if (ipli > SEI_PLUTO) {
        sprintf(s, "asteroid eph. file (%s): ", sp);
      } else if (ipli != SEI_MOON) {
        sprintf(s, "planets eph. file (%s): ", sp);
      } else {
        sprintf(s, "moon eph. file (%s): ", sp);
      }
      if (tjd < fdp->tfstart) {
	sprintf(s + strlen(s), "jd %f < lower limit %f;", 
		  tjd, fdp->tfstart); 
      } else {
	sprintf(s + strlen(s), "jd %f > upper limit %f;", 
		  tjd, fdp->tfend); 
      }
      if (strlen(serr) + strlen(s) < AS_MAXCH)
	strcat(serr, s);
    }
    return(NOT_AVAILABLE);
  }
  /******************************
   * get planet's position      
   ******************************/
  /* get new segment, if necessary */
  if (pdp->segp == NULL || tjd < pdp->tseg0 || tjd > pdp->tseg1) {
    retc = get_new_segment(ctx, tjd, ipl, ifno, serr);
    if (retc != OK)
      return(retc);
    /* rotate cheby coeffs back to equatorial system.
     * if necessary, add reference orbit. */
    if (pdp->iflg & SEI_FLG_ROTATE) {
      rot_back(ctx, ipl); /**/
    } else {
      pdp->neval = pdp->ncoe;
    }
  }
  /* evaluate chebyshew polynomial for tjd */
  t = (tjd - pdp->tseg0) / pdp->dseg;
  t = t * 2 - 1;
  /* speed is needed, if
   * 1. true position is being computed before applying light-time etc.
   *    this is the position saved in pdp->x.
   *    in this case, speed is needed for light-time correction.
   * 2. the speed flag has been specified.
   */
  need_speed = (do_save || (iflag & SEFLG_SPEED));
  for (i = 0; i <= 2; i++) {
    xp[i]  = swi_echeb (t, pdp->segp+(i*pdp->ncoe), pdp->neval);
    if (need_speed) {
      xp[i+3] = swi_edcheb(t, pdp->segp+(i*pdp->ncoe), pdp->neval) / pdp->dseg * 2;
    } else {
      xp[i+3] = 0;	/* von Alois als billiger fix, evtl. illegal */
    }
  }
  /* if planet wanted is barycentric sun:
   * current sepl* files have do not have barycentric sun,
   * but have heliocentric earth and barycentric earth.
   * So barycentric sun and must be computed
   * from heliocentric earth and barycentric earth: the 
   * computation above gives heliocentric earth, therefore we
   * have to compute barycentric earth and subtract heliocentric
   * earth from it. this may be necessary with calls from 
   * sweplan(ctx) and from app_pos_etc_sun(ctx) (light-time). */
  if (ipl == SEI_SUNBARY && (pdp->iflg & SEI_FLG_EMBHEL)) {
    /* sweph(ctx) calls sweph(ctx) !!! for EMB.
     * Attention: a new calculation must be forced in any case.
     * Otherwise EARTH (instead of EMB) will possibly taken from 
     * save area.
     * to force new computation, set pedp->teval = 0 and restore it
     * after call of sweph(ctx, EMB). 
     */
    tsv = pedp->teval;
    pedp->teval = 0;
    retc = sweph(ctx, tjd, SEI_EMB, ifno, iflag | SEFLG_SPEED, NULL, NO_SAVE, xemb, serr);
    if (retc != OK) 
      return(retc);
    pedp->teval = tsv;
    for (i = 0; i <= 2; i++)
      xp[i] = xemb[i] - xp[i];
    if (need_speed)
      for (i = 3; i <= 5; i++) 	
	xp[i] = xemb[i] - xp[i];
  }
#if 1
  /* asteroids are heliocentric.
   * if JPL or SWISSEPH, convert to barycentric */
  if (xsunb != NULL && ((iflag & SEFLG_JPLEPH) || (iflag & SEFLG_SWIEPH))) {
    if (ipl >= SEI_ANYBODY) {
      for (i = 0; i <= 2; i++)
	xp[i] += xsunb[i];
      if (need_speed)
	for (i = 3; i <= 5; i++)
	  xp[i] += xsunb[i];
    }
  }
#endif
  if (do_save) {
    pdp->teval = tjd;
    pdp->xflgs = -1;	/* do new computation of light-time etc. */
    if (ifno == SEI_FILE_PLANET || ifno == SEI_FILE_MOON) {
      pdp->iephe = SEFLG_SWIEPH;/**/
    } else {
      pdp->iephe = psdp->iephe;
    }
  }
  if (xpret != NULL)
    for (i = 0; i <= 5; i++)
      xpret[i] = xp[i];
  return(OK);
}

/*
 * Alois 2.12.98: inserted error message generation for file not found 
 */
FILE *swi_fopen(swe_ctx *ctx, int ifno, char *fname, char *ephepath, char *serr)
{
  int np, i, j;
  FILE *fp = NULL;
  char *fnamp, fn[AS_MAXCH];
  char *cpos[20];
  char s[2 * AS_MAXCH];
  char s1[AS_MAXCH];
  if (ifno >= 0) {
    fnamp = ctx->fidat[ifno].fnam;
  } else {
    fnamp = fn; 
  }
  strcpy(s1, ephepath);
  np = swi_cutstr(s1, PATH_SEPARATOR, cpos, 20);
  *s = '\0';
  for (i = 0; i < np; i++) {
    strcpy(s, cpos[i]);
    if (strcmp(s, ".") == 0) { /* current directory */
      *s = '\0';
    } else {
      j = (int) strlen(s);
      if (*s != '\0' && *(s + j - 1) != *DIR_GLUE)
	strcat(s, DIR_GLUE);
    }
    if (strlen(s) + strlen(fname) < AS_MAXCH) {
      strcat(s, fname);
    } else {
      if (serr != NULL)
	sprintf(serr, "error: file path and name must be shorter than %d.", AS_MAXCH);
      return NULL;
    }
    strcpy(fnamp, s);
    fp = fopen(fnamp, BFILE_R_ACCESS);
    if (fp != NULL) 
      return fp;
  }
  sprintf(s, "SwissEph file '%s' not found in PATH '%s'", fname, ephepath);
  s[AS_MAXCH-1] = '\0';		/* s must not be longer then AS_MAXCH */
  if (serr != NULL)
    strcpy(serr, s);
  return NULL;
}

int32 swi_get_denum(swe_ctx *ctx, int32 ipli, int32 iflag)
{
  struct file_data *fdp = NULL;
  if (iflag & SEFLG_MOSEPH)
    return 403;
  if (iflag & SEFLG_JPLEPH) {
    if (ctx->jpldenum > 0) {
      return ctx->jpldenum;
    } else {
      return SE_DE_NUMBER;
    }
  }
  if (ipli > SE_AST_OFFSET) {
    fdp = &ctx->fidat[SEI_FILE_ANY_AST];
  } else if (ipli > SE_PLMOON_OFFSET) {
    fdp = &ctx->fidat[SEI_FILE_ANY_AST];
  } else if (ipli == SEI_CHIRON
      || ipli == SEI_PHOLUS
      || ipli == SEI_CERES
      || ipli == SEI_PALLAS
      || ipli == SEI_JUNO
      || ipli == SEI_VESTA) {
    fdp = &ctx->fidat[SEI_FILE_MAIN_AST];
  } else if (ipli == SEI_MOON) {
    fdp = &ctx->fidat[SEI_FILE_MOON];
  } else {
    fdp = &ctx->fidat[SEI_FILE_PLANET];
  }
  if (fdp != NULL) {
    if (fdp->sweph_denum != 0) {
      return fdp->sweph_denum;
    } else {
      return SE_DE_NUMBER;
    }
  }
  return SE_DE_NUMBER;
}

/* restrict: xx is the caller's working vector and xcom is either
 * ctx->pldat[SEI_ANYBODY].x (sweph.c:2697) or app_pos_etc_plan's local
 * xcom[6] (sweph.c:2876, declared at :2657 alongside xx[6]). Never the
 * same object. */
static int calc_center_body(int32 ipli, int32 iflag, double * SWI_RESTRICT xx,
                            const double * SWI_RESTRICT xcom, char *serr)
{
  int i;
  (void) serr;
  if (!(iflag & SEFLG_CENTER_BODY))
    return OK;
  if (ipli < SEI_MARS || ipli > SEI_PLUTO)
    return OK;
  for (i = 0; i <= 5; i++)
    xx[i] += xcom[i];
  return OK;
}

/* converts planets from barycentric to geocentric,
 * apparent positions
 * precession and nutation
 * according to flags
 * ipli		planet number
 * iflag	flags
 * serr         error string
 */
static int app_pos_etc_plan(swe_ctx *ctx, int ipli, int iplmoon, int32 iflag, char *serr)
{
  int i, j, niter, retc = OK;
  int ipl, ifno, ibody;
  int32 flg1, flg2;
  double xx[6], xx0[6], dx[3], dt, t, dtsave_for_defl;
  double xobs[6], xobs2[6];
  double xearth[6], xsun[6], xcom[6];
  double xxsp[6], xxsv[6];
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *pdp;
  struct epsilon *oe = &ctx->oec2000;
  int32 epheflag = iflag & SEFLG_EPHMASK;
  dtsave_for_defl = 0;	
  /* ephemeris file */
  if (ipli > SE_PLMOON_OFFSET || ipli > SE_AST_OFFSET) { // 2nd condition obsolete
    ifno = SEI_FILE_ANY_AST;	
    ibody = IS_ANY_BODY;
    pdp = &ctx->pldat[SEI_ANYBODY];
  } else if (ipli == SEI_CHIRON
      || ipli == SEI_PHOLUS
      || ipli == SEI_CERES
      || ipli == SEI_PALLAS
      || ipli == SEI_JUNO
      || ipli == SEI_VESTA) {
    ifno = SEI_FILE_MAIN_AST;	
    ibody = IS_MAIN_ASTEROID;
    pdp = &ctx->pldat[ipli];
  } else {
    ifno = SEI_FILE_PLANET;
    ibody = IS_PLANET;
    pdp = &ctx->pldat[ipli];
  }
  t = pdp->teval;
  /* if the same conversions have already been done for the same 
   * date, then return */
  flg1 = iflag & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  flg2 = pdp->xflgs & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  if (flg1 == flg2) {
    pdp->xflgs = iflag;
    pdp->iephe = iflag & SEFLG_EPHMASK;
    return OK;
  }
  /* the conversions will be done with xx[]. */
  for (i = 0; i <= 5; i++) 
    xx[i] = pdp->x[i];
  /* center body of planet, if SEFLG_CENTER_BODY (which is checked inside function) */
  calc_center_body(ipli, iflag, xx, ctx->pldat[SEI_ANYBODY].x, serr);
  for (i = 0; i <= 5; i++) 
    xx0[i] = xx[i];
  /* if heliocentric position is wanted */
  if (iflag & SEFLG_HELCTR) {
    if (pdp->iephe == SEFLG_JPLEPH || pdp->iephe == SEFLG_SWIEPH)
      for (i = 0; i <= 5; i++) 
	xx[i] -= ctx->pldat[SEI_SUNBARY].x[i];
  }
  /************************************
   * observer: geocenter or topocenter
   ************************************/
  /* if topocentric position is wanted  */
  if (iflag & SEFLG_TOPOCTR) { 
    if (ctx->topd.teval != pedp->teval
      || ctx->topd.teval == 0) {
      if (swi_get_observer(ctx, pedp->teval, iflag | SEFLG_NONUT, DO_SAVE, xobs, serr) != OK)
        return ERR;
    } else {
      for (i = 0; i <= 5; i++)
        xobs[i] = ctx->topd.xobs[i];
    }
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++)
      xobs[i] = xobs[i] + pedp->x[i];	
  } else {
    /* barycentric position of geocenter */
    for (i = 0; i <= 5; i++)
      xobs[i] = pedp->x[i];
  }
  /*******************************
   * light-time geocentric       * 
   *******************************/
  if (!(iflag & SEFLG_TRUEPOS)) {
    /* number of iterations - 1 */
    if (pdp->iephe == SEFLG_JPLEPH || pdp->iephe == SEFLG_SWIEPH) {
      niter = 1;
    } else { 	/* SEFLG_MOSEPH or planet from osculating elements */
      niter = 0;
    }
    if (iflag & SEFLG_SPEED) {
      /* 
       * Apparent speed is influenced by the fact that dt changes with
       * time. This makes a difference of several hundredths of an
       * arc second / day. To take this into account, we compute 
       * 1. true position - apparent position at time t - 1.
       * 2. true position - apparent position at time t.
       * 3. the difference between the two is the part of the daily motion 
       * that results from the change of dt.
       */
      for (i = 0; i <= 2; i++)
	xxsv[i] = xxsp[i] = xx[i] - xx[i+3];
      for (j = 0; j <= niter; j++) {
	for (i = 0; i <= 2; i++) {
	  dx[i] = xxsp[i];
	  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
	    dx[i] -= (xobs[i] - xobs[i+3]);
	}
	/* new dt */
	dt = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;     
	for (i = 0; i <= 2; i++) { 	/* rough apparent position at t-1 */
	  //xxsp[i] = xxsv[i] - dt * pdp->x[i+3];
	  xxsp[i] = xxsv[i] - dt * xx0[i+3];
	}
      }
      /* true position - apparent position at time t-1 */
      for (i = 0; i <= 2; i++) 
	xxsp[i] = xxsv[i] - xxsp[i];
    }
    /* dt and t(apparent) */
    for (j = 0; j <= niter; j++) {
      for (i = 0; i <= 2; i++) {
	dx[i] = xx[i];
	if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
	  dx[i] -= xobs[i];
      }
      dt = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;    
      /* new t */
      t = pdp->teval - dt;
      dtsave_for_defl = dt;
      for (i = 0; i <= 2; i++) {	/* rough apparent position at t*/
	//xx[i] = pdp->x[i] - dt * pdp->x[i+3];
	xx[i] = xx0[i] - dt * xx0[i+3];
      }
    }
    /* part of daily motion resulting from change of dt */
    if (iflag & SEFLG_SPEED) {
      for (i = 0; i <= 2; i++) {
	//xxsp[i] = pdp->x[i] - xx[i] - xxsp[i];
	xxsp[i] = xx0[i] - xx[i] - xxsp[i];
      }
    }
    /* new position, accounting for light-time (accurate) */
    if ((iflag & SEFLG_CENTER_BODY)
      && ipli >= SE_MARS && ipli <= SE_PLUTO) {
      //ipli_com = ipli * 100 + 9099;
      /* jupiter center of body, relative to jupiter barycenter */
      retc = sweph(ctx, t, iplmoon, SEI_FILE_ANY_AST, iflag, NULL, NO_SAVE, xcom, serr);
      if (retc == ERR || retc == NOT_AVAILABLE)
	return ERR;
    }
    switch(epheflag) {
      case SEFLG_JPLEPH:
	if (ibody >= IS_ANY_BODY)
	  ipl = -1; /* will not be used */ /*pnoint2jpl[SEI_ANYBODY];*/
	else
	  ipl = pnoint2jpl[ipli];
	if (ibody == IS_PLANET) {
	  retc = swi_pleph(ctx, t, ipl, J_SBARY, xx, serr);
	  if (retc != OK) {
	    swi_close_jpl_file(ctx);
	    ctx->jpl_file_is_open = FALSE;
	  } 
	} else { 	/* asteroid */
	  /* first sun */
	  retc = swi_pleph(ctx, t, J_SUN, J_SBARY, xsun, serr);
	  if (retc != OK) {
	    swi_close_jpl_file(ctx);
	    ctx->jpl_file_is_open = FALSE;
	  } 
	  /* asteroid */
	  retc = sweph(ctx, t, ipli, ifno, iflag, xsun, NO_SAVE, xx, serr);
	}
	if (retc != OK)
	  return(retc);
        /* for accuracy in speed, we need earth as well */
	if ((iflag & SEFLG_SPEED)
	  && !(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR)) { 	
	  retc = swi_pleph(ctx, t, J_EARTH, J_SBARY, xearth, serr);
	  if (retc != OK) {
	    swi_close_jpl_file(ctx);
	    ctx->jpl_file_is_open = FALSE;
	    return(retc);
	  } 
	}
	break;
      case SEFLG_SWIEPH:
	if (ibody == IS_PLANET) {
	  retc = sweplan(ctx, t, ipli, ifno, iflag, NO_SAVE, xx, xearth, xsun, NULL, serr);
	} else { 		/*asteroid*/
	  retc = sweplan(ctx, t, SEI_EARTH, SEI_FILE_PLANET, iflag, NO_SAVE, xearth, NULL, xsun, NULL, serr);
	  if (retc == OK)
	    retc = sweph(ctx, t, ipli, ifno, iflag, xsun, NO_SAVE, xx, serr);
	}
	if (retc != OK)
	  return(retc);
	break;
      case SEFLG_MOSEPH:
      default:
	/* 
	 * with moshier or other ephemerides, subtraction of dt * speed 
	 * is sufficient (has been done in light-time iteration above)
	 */
        /* if speed flag is true, we call swi_moshplan(ctx, ) for new t.
	 * this does not increase position precision,
	 * but speed precision, which becomes better than 0.01"/day.
	 * for precise speed, we need earth as well.
	 */
	if (iflag & SEFLG_SPEED
	  && !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR))) { 	
	  if (ibody == IS_PLANET) {
	    retc = swi_moshplan(ctx, t, ipli, NO_SAVE, xxsv, xearth, serr);
          } else {		/* if asteroid */
	    retc = sweph(ctx, t, ipli, ifno, iflag, NULL, NO_SAVE, xxsv, serr);
	    if (retc == OK)
	      retc = swi_moshplan(ctx, t, SEI_EARTH, NO_SAVE, xearth, xearth, serr);
          }
	  if (retc != OK)
	    return(retc);
	  /* only speed is taken from this computation, otherwise position
	   * calculations with and without speed would not agree. The difference
	   * would be about 0.01", which is far below the intrinsic error of the
	   * moshier ephemeris.
	   */
	  for (i = 3; i <= 5; i++)
	    xx[i] = xxsv[i];
        }
	break;
    }
    calc_center_body(ipli, iflag, xx, xcom, serr);
    if (iflag & SEFLG_HELCTR) {
      if (pdp->iephe == SEFLG_JPLEPH || pdp->iephe == SEFLG_SWIEPH) 
	for (i = 0; i <= 5; i++) 
	  xx[i] -= ctx->pldat[SEI_SUNBARY].x[i];
    }
    if (iflag & SEFLG_SPEED) {
      /* observer position for t(light-time) */
      if (iflag & SEFLG_TOPOCTR) {
        if (swi_get_observer(ctx, t, iflag | SEFLG_NONUT, NO_SAVE, xobs2, serr) != OK)
          return ERR;
        for (i = 0; i <= 5; i++)
          xobs2[i] += xearth[i];
      } else {
        for (i = 0; i <= 5; i++)
          xobs2[i] = xearth[i];
      }
    }
  }
  /*******************************
   * conversion to geocenter     * 
   *******************************/
  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR)) {
    /* subtract earth */
    for (i = 0; i <= 5; i++) 
      xx[i] -= xobs[i]; 
    if ((iflag & SEFLG_TRUEPOS) == 0 ) {
      /* 
       * Apparent speed is also influenced by
       * the change of dt during motion.
       * Neglect of this would result in an error of several 0.01"
       */
      if (iflag & SEFLG_SPEED)
	for (i = 3; i <= 5; i++) 
	  xx[i] -= xxsp[i-3]; 	
    }
  }
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /************************************
   * relativistic deflection of light *
   ************************************/
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOGDEFL))
		/* SEFLG_NOGDEFL is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_deflect_light(ctx, xx, dtsave_for_defl, iflag);
  /**********************************
   * 'annual' aberration of light   *
   **********************************/
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOABERR)) {
		/* SEFLG_NOABERR is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_aberr_light(xx, xobs, iflag);
    /* 
     * Apparent speed is also influenced by
     * the difference of speed of the earth between t and t-dt. 
     * Neglecting this would involve an error of several 0.1"
     */
    if (iflag & SEFLG_SPEED) {
      for (i = 3; i <= 5; i++) 
	xx[i] += xobs[i] - xobs2[i];
    }
  }
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && swi_get_denum(ctx, ipli, epheflag) >= 403) {
    swi_bias(ctx, xx, t, iflag, FALSE);
  }/**/
  /* save J2000 coordinates; required for sidereal positions */
  for (i = 0; i <= 5; i++)
    xxsv[i] = xx[i];
  /************************************************
   * precession, equator 2000 -> equator of date *
   ************************************************/
  if (!(iflag & SEFLG_J2000)) {
    swi_precess(ctx, xx, pdp->teval, iflag, J2000_TO_J);
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, xx, pdp->teval, iflag, J2000_TO_J);
    oe = &ctx->oec;
  } else {
    oe = &ctx->oec2000;
  }
  return app_pos_rest(ctx, pdp, iflag, xx, xxsv, oe, serr);
}

static int app_pos_rest(swe_ctx *ctx, struct plan_data *pdp, int32 iflag, 
                        double *xx, double *x2000, 
                        struct epsilon *oe, char *serr) 
{
  int i;
  double daya[2];
  double xxsv[24];
  /************************************************
   * nutation                                     *
   ************************************************/
  if (!(iflag & SEFLG_NONUT))
    swi_nutate(ctx, xx, iflag, FALSE);
  /* now we have equatorial cartesian coordinates; save them */
  for (i = 0; i <= 5; i++)
    pdp->xreturn[18+i] = xx[i];
  /************************************************
   * transformation to ecliptic.                  *
   * with sidereal calc. this will be overwritten *
   * afterwards.                                  *
   ************************************************/
  swi_coortrf2(xx, xx, oe->seps, oe->ceps);
  if (iflag & SEFLG_SPEED)
    swi_coortrf2(xx+3, xx+3, oe->seps, oe->ceps);
  if (!(iflag & SEFLG_NONUT)) {
    swi_coortrf2(xx, xx, ctx->nut.snut, ctx->nut.cnut);
    if (iflag & SEFLG_SPEED)
      swi_coortrf2(xx+3, xx+3, ctx->nut.snut, ctx->nut.cnut);
  }
  /* now we have ecliptic cartesian coordinates */
  for (i = 0; i <= 5; i++)
    pdp->xreturn[6+i] = xx[i];
  /************************************
   * sidereal positions               *
   ************************************/
  if (iflag & SEFLG_SIDEREAL) {
    /* project onto ecliptic t0 */
    if (ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) {
      if (swi_trop_ra2sid_lon(ctx, x2000, pdp->xreturn+6, pdp->xreturn+18, iflag) != OK)
	return ERR;
    /* project onto solar system equator */
    } else if (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE) {
      if (swi_trop_ra2sid_lon_sosy(ctx, x2000, pdp->xreturn+6, iflag) != OK)
	return ERR;
    } else {
    /* traditional algorithm */
      swi_cartpol_sp(pdp->xreturn+6, pdp->xreturn); 
      /* note, swi_get_ayanamsa_ex(ctx) disturbs present calculations, if sun is calculated with 
       * TRUE_CHITRA ayanamsha, because the ayanamsha also calculates the sun.
       * Therefore current values are saved... */
      for (i = 0; i < 24; i++)
        xxsv[i] = pdp->xreturn[i];
      if (swi_get_ayanamsa_with_speed(ctx, pdp->teval, iflag, daya, serr) == ERR)
        return ERR;
      /* ... and restored */
      for (i = 0; i < 24; i++)
        pdp->xreturn[i] = xxsv[i];
      pdp->xreturn[0] -= daya[0] * DEGTORAD;
      pdp->xreturn[3] -= daya[1] * DEGTORAD;
      swi_polcart_sp(pdp->xreturn, pdp->xreturn+6); 
    }
  } 
  /************************************************
   * transformation to polar coordinates          *
   ************************************************/
  swi_cartpol_sp(pdp->xreturn+18, pdp->xreturn+12); 
  swi_cartpol_sp(pdp->xreturn+6, pdp->xreturn); 
  /********************** 
   * radians to degrees *
   **********************/
  /*if ((iflag & SEFLG_RADIANS) == 0) {*/
    for (i = 0; i < 2; i++) {
      pdp->xreturn[i] *= RADTODEG;		/* ecliptic */
      pdp->xreturn[i+3] *= RADTODEG;
      pdp->xreturn[i+12] *= RADTODEG;	/* equator */
      pdp->xreturn[i+15] *= RADTODEG;
    }
/*pdp->xreturn[12] -= (0.053 / 3600.0); */
  /*}*/
  /* save, what has been done */
  pdp->xflgs = iflag;
  pdp->iephe = iflag & SEFLG_EPHMASK;
  return OK;
}

void CALL_CONV swe_set_sid_mode_r(swe_ctx *ctx, int32 sid_mode, double t0, double ayan_t0)
{
  AS_BOOL swi_cfg_was = swi_config_begin_apply(ctx);
  struct sid_data *sip = &ctx->sidd;
  swi_init_swed_if_start(ctx);
  if (sid_mode < 0)
    sid_mode = 0;
  sip->sid_mode = sid_mode;
  if (sid_mode >= SE_SIDBITS)
    sid_mode %= SE_SIDBITS;
  /* standard equinoxes: positions always referred to ecliptic of t0 */
  if (sid_mode == SE_SIDM_J2000 
	  || sid_mode == SE_SIDM_J1900 
	  || sid_mode == SE_SIDM_B1950
	  || sid_mode == SE_SIDM_GALALIGN_MARDYKS
	  ) {
    //sip->sid_mode &= ~SE_SIDBIT_SSY_PLANE;
    sip->sid_mode = sid_mode;
    sip->sid_mode |= SE_SIDBIT_ECL_T0;
  }
  if (sid_mode == SE_SIDM_TRUE_CITRA 
      || sid_mode == SE_SIDM_TRUE_REVATI 
      || sid_mode == SE_SIDM_TRUE_PUSHYA 
      || sid_mode == SE_SIDM_TRUE_SHEORAN 
      || sid_mode == SE_SIDM_TRUE_MULA 
      || sid_mode == SE_SIDM_GALCENT_0SAG 
      || sid_mode == SE_SIDM_GALCENT_COCHRANE 
      || sid_mode == SE_SIDM_GALCENT_RGILBRAND 
      || sid_mode == SE_SIDM_GALCENT_MULA_WILHELM
      || sid_mode == SE_SIDM_GALEQU_IAU1958 
      || sid_mode == SE_SIDM_GALEQU_TRUE
      || sid_mode == SE_SIDM_GALEQU_MULA
      ) {
    //sip->sid_mode &= ~(SE_SIDBIT_ECL_T0 | SE_SIDBIT_SSY_PLANE | SE_SIDBIT_USER_UT);
    sip->sid_mode = sid_mode;
  }
  // make sure that sid_mode is either SE_SIDM_USER or < SE_NSIDM_PREDEF
  if (sid_mode >= SE_NSIDM_PREDEF && sid_mode != SE_SIDM_USER)
    sip->sid_mode = sid_mode = SE_SIDM_FAGAN_BRADLEY;
  ctx->ayana_is_set = TRUE;
  if (sid_mode == SE_SIDM_USER) {
    sip->t0 = t0;
    sip->ayan_t0 = ayan_t0;
    sip->t0_is_UT = FALSE;
    if (sip->sid_mode & SE_SIDBIT_USER_UT)
      sip->t0_is_UT = TRUE;
  } else {
    sip->t0 = ayanamsa[sid_mode].t0;
    sip->ayan_t0 = ayanamsa[sid_mode].ayan_t0;
    sip->t0_is_UT = ayanamsa[sid_mode].t0_is_UT;
  }
  // test feature: ayanamsha using its original precession model
  if (sid_mode < SE_NSIDM_PREDEF && (sip->sid_mode & SE_SIDBIT_PREC_ORIG) && ayanamsa[sid_mode].prec_offset > 0) {
    ctx->astro_models[SE_MODEL_PREC_LONGTERM] = ayanamsa[sid_mode].prec_offset;
    ctx->astro_models[SE_MODEL_PREC_SHORTTERM] = ayanamsa[sid_mode].prec_offset;
    // add a corresponding nutation model
    switch(ayanamsa[sid_mode].prec_offset) {
      case SEMOD_PREC_NEWCOMB:
        ctx->astro_models[SE_MODEL_NUT] = SEMOD_NUT_WOOLARD;
	break;
      case SEMOD_PREC_IAU_1976:
        ctx->astro_models[SE_MODEL_NUT] = SEMOD_NUT_IAU_1980;
	break;
      default:
        break;
    }
  }
  swi_force_app_pos_etc(ctx);
  swi_config_end_apply(ctx, swi_cfg_was);
  swi_config_publish(ctx, SWI_CFG_SID);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
void CALL_CONV swe_set_sid_mode(int32 sid_mode, double t0, double ayan_t0)
{
  swe_set_sid_mode_r(swi_default_ctx(), sid_mode, t0, ayan_t0);
}

int32 CALL_CONV swe_get_ayanamsa_ex_r(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr)
{
  struct nut nuttmp;
  struct nut *nutp = &nuttmp;	/* dummy assign, to silence gcc warning */
  int32 retval = swi_get_ayanamsa_ex(ctx, tjd_et, iflag, daya, serr);
  if (!(iflag & SEFLG_NONUT)) {
    if (tjd_et == ctx->nut.tnut) {
      nutp = &ctx->nut;
    } else {
      nutp = &nuttmp;
      swi_nutation(ctx, tjd_et, iflag, nutp->nutlo);
    }
    *daya += nutp->nutlo[0] * RADTODEG;
    retval &= (~SEFLG_NONUT); // must remove flag which was added internally in swi_get_ayanamsa_ex(ctx)
  }
  return retval;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_get_ayanamsa_ex(double tjd_et, int32 iflag, double *daya, char *serr)
{
  return swe_get_ayanamsa_ex_r(swi_default_ctx(), tjd_et, iflag, daya, serr);
}

/*
 * Function calculates a correction for ayanamsha if the ayanamsha was
 * defined using a different precession model than our standard one.
 * This allows us to use this ayanamsha with our standard precession
 * model and still remain very accurate in ephemeris positions.
 * It has the effect that ayanamsha values change depending on the precession
 * model used, but sidereal planetary positions remain the same.
 *
 * For this function to work correctly, our standard precession model 
 * must be relative to J2000. Any future precession model should not
 * use a different starting epoch.
 */
static int get_aya_correction(swe_ctx *ctx, int iflag, double *corr, char *serr) {
  double x[6], eps, t0;
  struct sid_data *sip = &ctx->sidd;
  int prec_model = ctx->astro_models[SE_MODEL_PREC_LONGTERM];
  int prec_model_short = ctx->astro_models[SE_MODEL_PREC_SHORTTERM];
  int prec_offset = 0;
  int sid_mode = sip->sid_mode;
  sid_mode %= SE_SIDBITS;
  *corr = 0;
  if (sip->t0 == J2000) 
    return 0;
  if (sip->sid_mode & SE_SIDBIT_NO_PREC_OFFSET) 
    return 0;
  if (sid_mode < SE_NSIDM_PREDEF)
    prec_offset = ayanamsa[sid_mode].prec_offset;
  if (prec_offset < 0) prec_offset = 0;
  if (prec_model == prec_offset)
    return 0;
  t0 = sip->t0;
  if (sip->t0_is_UT)
    t0 += swe_deltat_ex_r(ctx, t0, iflag, serr);
  /* vernal point (tjd), cartesian */
  x[0] = 1; 
  x[1] = x[2] = 0;
  swi_precess(ctx, x, t0, 0, J_TO_J2000);
  ctx->astro_models[SE_MODEL_PREC_LONGTERM] = prec_offset;
  ctx->astro_models[SE_MODEL_PREC_SHORTTERM] = prec_offset;
  swi_precess(ctx, x, t0, 0, J2000_TO_J);
  ctx->astro_models[SE_MODEL_PREC_LONGTERM] = prec_model;
  ctx->astro_models[SE_MODEL_PREC_SHORTTERM] = prec_model_short;
  /* to ecliptic */
  eps = swi_epsiln(ctx, t0, 0);
  swi_coortrf(x, x, eps);
  /* to polar */
  swi_cartpol(x, x);
  /* get ayanamsa */
  *corr = x[0] * RADTODEG;
  if (*corr > 350 /*correct!*/) *corr -= 360; // a signed value near 0
  //fprintf(stderr, "corr=%f\n", *corr * 3600.0);
  return OK;
}

int32 swi_get_ayanamsa_ex(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr)
{
  double x[6], eps, t0, corr;
  struct sid_data *sip = &ctx->sidd;
  char star[AS_MAXCH];
  int32 epheflag, otherflag, retflag, iflag_true, iflag_galequ;
  int sid_mode = sip->sid_mode;
  iflag = plaus_iflag(ctx, iflag, -1, tjd_et, serr);
  epheflag = iflag & SEFLG_EPHMASK;
  otherflag = iflag & ~SEFLG_EPHMASK;
  *daya = 0.0;
  iflag &= SEFLG_EPHMASK;
  iflag |= SEFLG_NONUT;
  sid_mode %= SE_SIDBITS;
  /* ayanamshas based on the intersection point of galactic equator and
   * ecliptic always need SEFLG_TRUEPOS, because position of galactic
   * pole is required without aberration or light deflection */
  iflag_galequ = iflag | SEFLG_TRUEPOS;
#if 1
  /* _TRUE_ ayanamshas can have the following SEFLG_s;
   * The star will have the intended fixed position even if these flags are 
   * provided */
  iflag_true = iflag;
  if (otherflag & SEFLG_TRUEPOS) iflag_true |= SEFLG_TRUEPOS;
  if (otherflag & SEFLG_NOABERR) iflag_true |= SEFLG_NOABERR;
  if (otherflag & SEFLG_NOGDEFL) iflag_true |= SEFLG_NOGDEFL;
#endif
  /* warning, if swe_set_ephe_path() or swe_set_jplfile() was not called yet,
   * although ephemeris files are required */
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) 
     && (sid_mode ==  SE_SIDM_TRUE_CITRA 
      || sid_mode == SE_SIDM_TRUE_REVATI 
      || sid_mode == SE_SIDM_TRUE_PUSHYA 
      || sip->sid_mode == SE_SIDM_TRUE_SHEORAN 
      || sid_mode == SE_SIDM_TRUE_MULA 
      || sid_mode == SE_SIDM_GALCENT_0SAG
      || sid_mode == SE_SIDM_GALCENT_COCHRANE
      || sid_mode == SE_SIDM_GALCENT_RGILBRAND 
      || sid_mode == SE_SIDM_GALCENT_MULA_WILHELM
      || sid_mode == SE_SIDM_GALEQU_IAU1958 
      || sid_mode == SE_SIDM_GALEQU_TRUE
      || sid_mode == SE_SIDM_GALEQU_MULA) 
      && serr != NULL) {
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_get_ayanamsa_ex()");
  }
  if (!ctx->ayana_is_set)
    SWI_CFG_LOCAL(ctx, swe_set_sid_mode_r(ctx, SE_SIDM_FAGAN_BRADLEY, 0, 0));
  if (sid_mode == SE_SIDM_TRUE_CITRA) {
    strcpy(star, "Spica"); /* Citra */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR) {
      return ERR; 
    }
    /*fprintf(stderr, "serr=%s\n", serr);*/
    *daya = swe_degnorm(x[0] - 180);
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_TRUE_REVATI) {
    strcpy(star, ",zePsc"); /* Revati */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 359.8333333333);
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_TRUE_PUSHYA) {
    strcpy(star, ",deCnc"); /* Pushya = Asellus Australis */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 106);
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_TRUE_SHEORAN) {
    strcpy(star, ",deCnc"); /* Asellus Australis */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 103.49264221625);
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_TRUE_MULA) {
    strcpy(star, ",laSco"); /* Mula = lambda Scorpionis */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 240);
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode ==  SE_SIDM_GALCENT_0SAG) {
    strcpy(star, ",SgrA*"); /* Galactic Centre */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 240.0);
    return (retflag & SEFLG_EPHMASK);
    /*return swe_degnorm(x[0] - 359.83333333334);*/
  }
  if (sid_mode ==  SE_SIDM_GALCENT_COCHRANE) {
    strcpy(star, ",SgrA*"); /* Galactic Centre */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 270.0);
    return (retflag & SEFLG_EPHMASK);
    /*return swe_degnorm(x[0] - 359.83333333334);*/
  }
  if (sid_mode ==  SE_SIDM_GALCENT_RGILBRAND) {
    strcpy(star, ",SgrA*"); /* Galactic Centre */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 210.0 - 90.0 * 0.3819660113);
    return (retflag & SEFLG_EPHMASK);
    /*return swe_degnorm(x[0] - 359.83333333334);*/
  }
  if (sid_mode == SE_SIDM_GALCENT_MULA_WILHELM) {
    strcpy(star, ",SgrA*"); /* Galactic Centre */
    /* right ascension in polar projection onto the ecliptic, 
     * and that point is put in the middle of Mula */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true | SEFLG_EQUATORIAL, x, serr)) == ERR)
      return ERR;
    eps = swi_epsiln(ctx, tjd_et, iflag) * RADTODEG;
    *daya = swi_armc_to_mc(x[0], eps);
    *daya = swe_degnorm(*daya - 246.6666666667);
    return (retflag & SEFLG_EPHMASK);
    /*return swe_degnorm(x[0] - 359.83333333334);*/
  }
  if (sid_mode == SE_SIDM_GALEQU_IAU1958) {
    strcpy(star, ",GP1958"); /* Galactic Pole IAU 1958 */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_galequ, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 150);
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_GALEQU_TRUE) {
    strcpy(star, ",GPol"); /* Galactic Pole modern, true */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_galequ, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 150);
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_GALEQU_MULA) {
    strcpy(star, ",GPol"); /* Galactic Pole modern, true */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_galequ, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 150 - 6.6666666667);
    return (retflag & SEFLG_EPHMASK);
  }
  if (!(sip->sid_mode & SE_SIDBIT_ECL_DATE)) {
    // Now calculate precession for ayanamsha. 
    // The following is the original method implemented in 1999 and
    // still used as our default method, although it is not logical.
    // Precession is measured on the ecliptic of the start epoch t0 (ayan_t0),
    // then the initial value of ayanamsha is added.
    // The procedure is as follows: The vernal point of the end epoch tjd_et
    // is precessed to t0. Ayanamsha is the resulting longitude of that
    // point at t0 plus the initial value.
    // This method is not really consistent because later this ayanamsha,
    // which is based on the ecliptic t0, will be applied to planetary
    // positions relative to the ecliptic of date.
    //
    /* vernal point (tjd), cartesian */
    x[0] = 1; 
    x[1] = x[2] = x[3] = x[4] = x[5] = 0;
    /* to J2000 */
    if (tjd_et != J2000)
      swi_precess(ctx, x, tjd_et, 0, J_TO_J2000);
    /* to t0 */
    t0 = sip->t0;
    if (sip->t0_is_UT)
      t0 += swe_deltat_ex_r(ctx, t0, iflag, serr);
    swi_precess(ctx, x, t0, 0, J2000_TO_J);
    /* to ecliptic t0 */
    eps = swi_epsiln(ctx, t0, 0);
    swi_coortrf(x, x, eps);
    /* to polar */
    swi_cartpol(x, x);
    /* subtract initial value of ayanamsa */
    x[0] = -x[0] * RADTODEG + sip->ayan_t0;
  } else {
    // Alternative method, more consistent, programmed on 15 may 2020.
    // The ayanamsha is measured on the ecliptic of date. This is more
    // correct because the ayanamsha will be applied to planetary positions
    // relative to the ecliptic of date.
    //
    // at t0, we have ayanamsha sip->ayan_t0
    x[0] = swe_degnorm(sip->ayan_t0) * DEGTORAD;
    x[1] = 0; x[2] = 1;
    // get epsilon for t0
    t0 = sip->t0;
    if (sip->t0_is_UT)
      t0 += swe_deltat_ex_r(ctx, t0, iflag, serr);
    eps = swi_epsiln(ctx, t0, 0);
    // to polar equatorial relative to equinox t0
    swi_polcart(x, x);
    swi_coortrf(x, x, -eps);
    // precess to J2000
    if (t0 != J2000)
      swi_precess(ctx, x, t0, 0, J_TO_J2000);
    // precess to date
    swi_precess(ctx, x, tjd_et, 0, J2000_TO_J);
    // epsilon of date
    eps = swi_epsiln(ctx, tjd_et, 0);
    // to polar
    swi_coortrf(x, x, eps);
    swi_cartpol(x, x);
    x[0] = swe_degnorm(x[0] * RADTODEG);
  }
  get_aya_correction(ctx, iflag, &corr, serr);
  /* get ayanamsa */
  *daya = swe_degnorm(x[0] - corr);
  //*daya = swe_degnorm(x[0]);
  return iflag;
}

int32 swi_get_ayanamsa_with_speed(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr)
{
  double daya_t2, t2;
  double tintv = 0.001;
  int32 retflag;
  t2 = tjd_et - tintv;
  retflag = swi_get_ayanamsa_ex(ctx, t2, iflag, &daya_t2, serr);
  if (retflag == ERR) 
    return ERR;
  retflag = swi_get_ayanamsa_ex(ctx, tjd_et, iflag, daya, serr);
  if (retflag == ERR) 
    return ERR;
  daya[1] = (daya[0] - daya_t2) / tintv;
  return retflag;
}

int32 CALL_CONV swe_get_ayanamsa_ex_ut_r(swe_ctx *ctx, double tjd_ut, int32 iflag, double *daya, char *serr)
{
  double deltat;
  int32 retflag = OK;
  int32 epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag == 0) {
    epheflag = SEFLG_SWIEPH;
    iflag |= SEFLG_SWIEPH;
  }
  deltat = swe_deltat_ex_r(ctx, tjd_ut, iflag, serr);
  // swe... includes nutation, unless SEFLG_NONUT
  retflag = swe_get_ayanamsa_ex_r(ctx, tjd_ut + deltat, iflag, daya, serr);
  /* if ephe required is not ephe returned, adjust delta t: */
  if ((retflag & SEFLG_EPHMASK) != epheflag) {
    deltat = swe_deltat_ex_r(ctx, tjd_ut, retflag, serr);
    retflag = swe_get_ayanamsa_ex_r(ctx, tjd_ut + deltat, iflag, daya, serr);
  }
  return retflag;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_get_ayanamsa_ex_ut(double tjd_ut, int32 iflag, double *daya, char *serr)
{
  return swe_get_ayanamsa_ex_ut_r(swi_default_ctx(), tjd_ut, iflag, daya, serr);
}

/* the ayanamsa (precession in longitude) 
 * according to Newcomb's definition: 360 -
 * longitude of the vernal point of t referred to the
 * ecliptic of t0.
 */
double CALL_CONV swe_get_ayanamsa_r(swe_ctx *ctx, double tjd_et)
{
  double daya;
  int32 iflag = swi_guess_ephe_flag(ctx);
  // swi... function never includes nutation
  swi_get_ayanamsa_ex(ctx, tjd_et, iflag, &daya, NULL);
  return daya;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_get_ayanamsa(double tjd_et)
{
  return swe_get_ayanamsa_r(swi_default_ctx(), tjd_et);
}

double CALL_CONV swe_get_ayanamsa_ut_r(swe_ctx *ctx, double tjd_ut)
{
  double daya;
  int32 iflag = swi_guess_ephe_flag(ctx);
  swi_get_ayanamsa_ex(ctx, tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, iflag, NULL), 0, &daya, NULL);
  return daya;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_get_ayanamsa_ut(double tjd_ut)
{
  return swe_get_ayanamsa_ut_r(swi_default_ctx(), tjd_ut);
}

/* 
 * input coordinates are J2000, cartesian.
 * xout 	ecliptical sidereal position (relative to ecliptic t0)
 * xoutr 	equatorial sidereal position (relative to equator t0)
 */
int swi_trop_ra2sid_lon(swe_ctx *ctx, double *xin, double *xout, double *xoutr, int32 iflag)
{
  double x[6], corr;
  int i;
  struct sid_data *sip = &ctx->sidd;
  struct epsilon oectmp;
  for (i = 0; i <= 5; i++)
    x[i] = xin[i];
  if (sip->t0 != J2000) {
    /* iflag must not contain SEFLG_JPLHOR here */
    swi_precess(ctx, x, sip->t0, 0, J2000_TO_J);  
    swi_precess(ctx, x+3, sip->t0, 0, J2000_TO_J);	/* speed */
  }
  for (i = 0; i <= 5; i++)
    xoutr[i] = x[i];
  calc_epsilon(ctx, ctx->sidd.t0, iflag, &oectmp);
  swi_coortrf2(x, x, oectmp.seps, oectmp.ceps);
  if (iflag & SEFLG_SPEED)
    swi_coortrf2(x+3, x+3, oectmp.seps, oectmp.ceps);
  /* to polar coordinates */
  swi_cartpol_sp(x, x); 
  /* subtract ayan_t0 */
  get_aya_correction(ctx, iflag, &corr, NULL);
  x[0] -= sip->ayan_t0 * DEGTORAD;
  x[0] = swe_radnorm(x[0] + corr * DEGTORAD);
  /* back to cartesian */
  swi_polcart_sp(x, xout); 
  return OK;
}

/* 
 * input coordinates are J2000, cartesian.
 * xout 	ecliptical sidereal position
 * xoutr 	equatorial sidereal position
 */
/* restrict: all 7 call sites pass distinct arrays -- verified, including
 * sweph.c:6847 and :8134, where xxsv[6] and x[6] are separate locals
 * declared together at sweph.c:6638. Note this is NOT true of the
 * sibling swi_trop_ra2sid_lon(), which IS called with input and output
 * aliased; see notes/C17_PERFORMANCE.md section 4.1. */
int swi_trop_ra2sid_lon_sosy(swe_ctx *ctx, const double * SWI_RESTRICT xin,
                             double * SWI_RESTRICT xout, int32 iflag)
{
  double x[6], x0[6], corr;
  int i;
  struct sid_data *sip = &ctx->sidd;
  struct epsilon *oe = &ctx->oec2000;
  double plane_node = SSY_PLANE_NODE_E2000;
  double plane_incl = SSY_PLANE_INCL;
  for (i = 0; i <= 5; i++)
    x[i] = xin[i];
  /* planet to ecliptic 2000 */
  swi_coortrf2(x, x, oe->seps, oe->ceps);
  if (iflag & SEFLG_SPEED)
    swi_coortrf2(x+3, x+3, oe->seps, oe->ceps);
  /* to polar coordinates */
  swi_cartpol_sp(x, x); 
  /* to solar system equator */
  x[0] -= plane_node;
  swi_polcart_sp(x, x);
  swi_coortrf(x, x, plane_incl);
  swi_coortrf(x+3, x+3, plane_incl);
  swi_cartpol_sp(x, x); 
  /* zero point of t0 in J2000 system */
  x0[0] = 1; 
  x0[1] = x0[2] = 0;
  if (sip->t0 != J2000) {
    /* iflag must not contain SEFLG_JPLHOR here */
    swi_precess(ctx, x0, sip->t0, 0, J_TO_J2000);
  }
  /* zero point to ecliptic 2000 */
  swi_coortrf2(x0, x0, oe->seps, oe->ceps);
  /* to polar coordinates */
  swi_cartpol(x0, x0); 
  /* to solar system equator */
  x0[0] -= plane_node;
  swi_polcart(x0, x0);
  swi_coortrf(x0, x0, plane_incl);
  swi_cartpol(x0, x0); 
  /* measure planet from zero point */
  x[0] -= x0[0];
  x[0] *= RADTODEG;
  /* subtract ayan_t0 */
  get_aya_correction(ctx, iflag, &corr, NULL);
  x[0] -= sip->ayan_t0;
  x[0] = swe_degnorm(x[0] + corr) * DEGTORAD;
  /* back to cartesian */
  swi_polcart_sp(x, xout); 
  return OK;
}

/* converts planets from barycentric to geocentric,
 * apparent positions
 * precession and nutation
 * according to flags
 * ipli		planet number
 * iflag	flags
 */
static int app_pos_etc_plan_osc(swe_ctx *ctx, int ipl, int ipli, int32 iflag, char *serr)
{
  int i, j, niter, retc;
  double xx[6], dx[3], dt, dtsave_for_defl;
  double xearth[6], xsun[6], xmoon[6];
  double xxsv[6], xxsp[3]={0}, xobs[6], xobs2[6];
  double t;
  struct plan_data *pdp = &ctx->pldat[ipli];
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  struct epsilon *oe = &ctx->oec2000;
  int32 epheflag = SEFLG_DEFAULTEPH;
  dt = dtsave_for_defl = 0;	/* dummy assign to silence gcc */
  if (iflag & SEFLG_MOSEPH) {
    epheflag = SEFLG_MOSEPH;
  } else if (iflag & SEFLG_SWIEPH) {
    epheflag = SEFLG_SWIEPH;
  } else if (iflag & SEFLG_JPLEPH) {
    epheflag = SEFLG_JPLEPH;
  }
  /* the conversions will be done with xx[]. */
  for (i = 0; i <= 5; i++) 
    xx[i] = pdp->x[i];
  /************************************
   * barycentric position is required *
   ************************************/
  /* = heliocentric position with Moshier ephemeris */
  /************************************
   * observer: geocenter or topocenter
   ************************************/
  /* if topocentric position is wanted  */
  if (iflag & SEFLG_TOPOCTR) { 
    if (ctx->topd.teval != pedp->teval
      || ctx->topd.teval == 0) {
      if (swi_get_observer(ctx, pedp->teval, iflag | SEFLG_NONUT, DO_SAVE, xobs, serr) != OK)
        return ERR;
    } else {
      for (i = 0; i <= 5; i++)
        xobs[i] = ctx->topd.xobs[i];
    }
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++)
      xobs[i] = xobs[i] + pedp->x[i];	
  } else if (iflag & SEFLG_BARYCTR) {
    for (i = 0; i <= 5; i++)
      xobs[i] = 0;	
  } else if (iflag & SEFLG_HELCTR) {
    if (iflag & SEFLG_MOSEPH) {
      for (i = 0; i <= 5; i++)
        xobs[i] = 0;	
    } else {
      for (i = 0; i <= 5; i++)
        xobs[i] = psdp->x[i];	
    }
  } else {
    for (i = 0; i <= 5; i++)
      xobs[i] = pedp->x[i];	
  }
  /*******************************
   * light-time                  * 
   *******************************/
  if (!(iflag & SEFLG_TRUEPOS)) {
    niter = 1;
    if (iflag & SEFLG_SPEED) {
      /* 
       * Apparent speed is influenced by the fact that dt changes with
       * motion. This makes a difference of several hundredths of an
       * arc second. To take this into account, we compute 
       * 1. true position - apparent position at time t - 1.
       * 2. true position - apparent position at time t.
       * 3. the difference between the two is the daily motion resulting from
       * the change of dt.
       */
      for (i = 0; i <= 2; i++)
	xxsv[i] = xxsp[i] = xx[i] - xx[i+3];
      for (j = 0; j <= niter; j++) {
	for (i = 0; i <= 2; i++) {
	  dx[i] = xxsp[i];
	  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
	    dx[i] -= (xobs[i] - xobs[i+3]);
	}
	/* new dt */
	dt = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;     
	for (i = 0; i <= 2; i++) 
	  xxsp[i] = xxsv[i] - dt * pdp->x[i+3];/* rough apparent position */
      }
      /* true position - apparent position at time t-1 */
      for (i = 0; i <= 2; i++) 
	xxsp[i] = xxsv[i] - xxsp[i];
    }
    /* dt and t(apparent) */
    for (j = 0; j <= niter; j++) {
      for (i = 0; i <= 2; i++) {
	dx[i] = xx[i];
	if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
	  dx[i] -= xobs[i];
      }
      /* new dt */
      dt = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;     
      dtsave_for_defl = dt;
      /* new position: subtract t * speed 
       */
      for (i = 0; i <= 2; i++) {
	xx[i] = pdp->x[i] - dt * pdp->x[i+3];/**/
	xx[i+3] = pdp->x[i+3];
      }
    }
    if (iflag & SEFLG_SPEED) {
      /* part of daily motion resulting from change of dt */
      for (i = 0; i <= 2; i++) 
	xxsp[i] = pdp->x[i] - xx[i] - xxsp[i];
      t = pdp->teval - dt;
      /* for accuracy in speed, we will need earth as well */
      retc = main_planet_bary(ctx, t, SEI_EARTH, epheflag, iflag, NO_SAVE, xearth, xearth, xsun, xmoon, serr);
      if (swi_osc_el_plan(ctx, t, xx, ipl-SE_FICT_OFFSET, ipli, xearth, xsun, serr) != OK)
	return ERR;
      if (retc != OK)
	return(retc);
      if (iflag & SEFLG_TOPOCTR) {
        if (swi_get_observer(ctx, t, iflag | SEFLG_NONUT, NO_SAVE, xobs2, serr) != OK)
          return ERR;
        for (i = 0; i <= 5; i++)
          xobs2[i] += xearth[i];
      } else {
        for (i = 0; i <= 5; i++)
          xobs2[i] = xearth[i];
      }
    }
  }
  /*******************************
   * conversion to geocenter     * 
   *******************************/
  for (i = 0; i <= 5; i++) 
    xx[i] -= xobs[i]; 
  if (!(iflag & SEFLG_TRUEPOS)) {
    /* 
     * Apparent speed is also influenced by
     * the change of dt during motion.
     * Neglect of this would result in an error of several 0.01"
     */
    if (iflag & SEFLG_SPEED)
      for (i = 3; i <= 5; i++) 
	xx[i] -= xxsp[i-3]; 
  }
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /************************************
   * relativistic deflection of light *
   ************************************/
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOGDEFL)) 
		/* SEFLG_NOGDEFL is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_deflect_light(ctx, xx, dtsave_for_defl, iflag);
  /**********************************
   * 'annual' aberration of light   *
   **********************************/
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOABERR)) {
		/* SEFLG_NOABERR is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_aberr_light(xx, xobs, iflag);
    /* 
     * Apparent speed is also influenced by
     * the difference of speed of the earth between t and t-dt. 
     * Neglecting this would involve an error of several 0.1"
     */
    if (iflag & SEFLG_SPEED)
      for (i = 3; i <= 5; i++) 
	xx[i] += xobs[i] - xobs2[i];
  }
  /* save J2000 coordinates; required for sidereal positions */
  for (i = 0; i <= 5; i++)
    xxsv[i] = xx[i];
  /************************************************
   * precession, equator 2000 -> equator of date *
   ************************************************/
  if (!(iflag & SEFLG_J2000)) {
    swi_precess(ctx, xx, pdp->teval, iflag, J2000_TO_J);
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, xx, pdp->teval, iflag, J2000_TO_J);
    oe = &ctx->oec;
  } else
    oe = &ctx->oec2000;
  return app_pos_rest(ctx, pdp, iflag, xx, xxsv, oe, serr);
}

/* influence of precession on speed 
 * xx		position and speed of planet in equatorial cartesian
 *		coordinates */
void swi_precess_speed(swe_ctx *ctx, double *xx, double t, int32 iflag, int direction) 
{
  struct epsilon *oe;
  double fac, dpre, dpre2;
  double tprec = (t - J2000) / 36525.0;
  int prec_model = ctx->astro_models[SE_MODEL_PREC_LONGTERM];
  if (prec_model == 0) prec_model = SEMOD_PREC_DEFAULT;
  if (direction == J2000_TO_J) {
    fac = 1;
    oe = &ctx->oec;
  } else {
    fac = -1;
    oe = &ctx->oec2000;
  }
  /* first correct rotation.
   * this costs some sines and cosines, but neglect might
   * involve an error > 1"/day */
  swi_precess(ctx, xx+3, t, iflag, direction); 
  /* then add 0.137"/day */
  swi_coortrf2(xx, xx, oe->seps, oe->ceps);
  swi_coortrf2(xx+3, xx+3, oe->seps, oe->ceps);
  swi_cartpol_sp(xx, xx);
if (1) {
  if (prec_model == SEMOD_PREC_VONDRAK_2011) {
    swi_ldp_peps(t, &dpre, NULL);
    swi_ldp_peps(t + 1, &dpre2, NULL);
    xx[3] += (dpre2 - dpre) * fac;
  } else {
    xx[3] += (50.290966 + 0.0222226 * tprec) / 3600 / 365.25 * DEGTORAD * fac;
			/* formula from Montenbruck, German 1994, p. 18 */
  }
}
  swi_polcart_sp(xx, xx);
  swi_coortrf2(xx, xx, -oe->seps, oe->ceps);
  swi_coortrf2(xx+3, xx+3, -oe->seps, oe->ceps);
}

/* multiplies cartesian equatorial coordinates with previously
 * calculated nutation matrix. also corrects speed. 
 */
void swi_nutate(swe_ctx *ctx, double *xx, int32 iflag, AS_BOOL backward)
{
  int i;
  double x[6], xv[6];
  for (i = 0; i <= 2; i++) {
    if (backward) {
      x[i] = xx[0] * ctx->nut.matrix[i][0] + 
	     xx[1] * ctx->nut.matrix[i][1] + 
	     xx[2] * ctx->nut.matrix[i][2];
    } else {
      x[i] = xx[0] * ctx->nut.matrix[0][i] + 
	     xx[1] * ctx->nut.matrix[1][i] + 
	     xx[2] * ctx->nut.matrix[2][i];
    }
  }
  if (iflag & SEFLG_SPEED) {
    /* correct speed:
     * first correct rotation */
    for (i = 0; i <= 2; i++) {
      if (backward) {
	x[i+3] = xx[3] * ctx->nut.matrix[i][0] + 
		 xx[4] * ctx->nut.matrix[i][1] + 
		 xx[5] * ctx->nut.matrix[i][2];
      } else {
	x[i+3] = xx[3] * ctx->nut.matrix[0][i] + 
		 xx[4] * ctx->nut.matrix[1][i] + 
		 xx[5] * ctx->nut.matrix[2][i];
      }
    }
    /* then apparent motion due to change of nutation during day.
     * this makes a difference of 0.01" */
    for (i = 0; i <= 2; i++) {
      if (backward) {
	xv[i] = xx[0] * ctx->nutv.matrix[i][0] + 
	       xx[1] * ctx->nutv.matrix[i][1] + 
	       xx[2] * ctx->nutv.matrix[i][2];
      } else {
	xv[i] = xx[0] * ctx->nutv.matrix[0][i] + 
	       xx[1] * ctx->nutv.matrix[1][i] + 
	       xx[2] * ctx->nutv.matrix[2][i];
      }
      /* new speed */
      xx[3+i] = x[3+i] + (x[i] - xv[i]) / NUT_SPEED_INTV;
    }
  }
  /* new position */
  for (i = 0; i <= 2; i++) 
    xx[i] = x[i];
}

/* computes 'annual' aberration
 * xx		planet's position accounted for light-time 
 *              and gravitational light deflection
 * xe    	earth's position and speed
 */
static void aberr_light(double *xx, double *xe) {
  int i;
  double xxs[6], v[6], u[6], ru;
  double b_1, f1, f2;
  double v2;
  for (i = 0; i <= 5; i++)
    u[i] = xxs[i] = xx[i];
  ru = sqrt(square_sum(u));
  for (i = 0; i <= 2; i++) 
    v[i] = xe[i+3] / 24.0 / 3600.0 / CLIGHT * AUNIT;
  v2 = square_sum(v);
  b_1 = sqrt(1 - v2);
  f1 = dot_prod(u, v) / ru;
  f2 = 1.0 + f1 / (1.0 + b_1);
  for (i = 0; i <= 2; i++) 
    xx[i] = (b_1*xx[i] + f2*ru*v[i]) / (1.0 + f1);
}

/* computes 'annual' aberration
 * xx		planet's position accounted for light-time 
 *              and gravitational light deflection
 * xe    	earth's position and speed
 * xe_dt    	earth's position and speed at t - dt
 * dt    	time difference for which xe_dt is given
 */
void swi_aberr_light_ex(double *xx, double *xe, double *xe_dt, double dt, int32 iflag) {
  int i;
  double xxs[6];
  double xx2[6];
  for (i = 0; i <= 5; i++) {
    xxs[i] = xx[i];
  }
  aberr_light(xx, xe);
  /* correction of speed
   * the influence of aberration on apparent velocity can
   * reach 0.4"/day
   */
  if (iflag & SEFLG_SPEED) {
    for (i = 0; i <= 2; i++) 
      xx2[i] = xxs[i] - dt * xxs[i + 3];
    aberr_light(xx2, xe_dt);
    for (i = 0; i <= 2; i++) {
      xx[i+3] = (xx[i] - xx2[i]) / dt;
    }
  }
}

/* computes 'annual' aberration
 * xx		planet's position accounted for light-time 
 *              and gravitational light deflection
 * xe    	earth's position and speed
 */
void swi_aberr_light(double *xx, double *xe, int32 iflag) {
  int i;
  double xxs[6], v[6], u[6], ru;
  double xx2[6], dx1, dx2;
  double b_1, f1, f2;
  double v2;
  double intv = PLAN_SPEED_INTV;
  for (i = 0; i <= 5; i++)
    u[i] = xxs[i] = xx[i];
  ru = sqrt(square_sum(u));
  for (i = 0; i <= 2; i++) 
    v[i] = xe[i+3] / 24.0 / 3600.0 / CLIGHT * AUNIT;
  v2 = square_sum(v);
  b_1 = sqrt(1 - v2);
  f1 = dot_prod(u, v) / ru;
  f2 = 1.0 + f1 / (1.0 + b_1);
  for (i = 0; i <= 2; i++) 
    xx[i] = (b_1*xx[i] + f2*ru*v[i]) / (1.0 + f1);
  if (iflag & SEFLG_SPEED) {
    /* correction of speed
     * the influence of aberration on apparent velocity can
     * reach 0.4"/day
     */
    for (i = 0; i <= 2; i++) 
      u[i] = xxs[i] - intv * xxs[i+3];
    ru = sqrt(square_sum(u));
    f1 = dot_prod(u, v) / ru;
    f2 = 1.0 + f1 / (1.0 + b_1);
    for (i = 0; i <= 2; i++) 
      xx2[i] = (b_1*u[i] + f2*ru*v[i]) / (1.0 + f1);
    for (i = 0; i <= 2; i++) {
      dx1 = xx[i] - xxs[i];
      dx2 = xx2[i] - u[i];
      dx1 -= dx2;
      xx[i+3] += dx1 / intv;
    }
  }
}

/* computes relativistic light deflection by the sun
 * ipli 	sweph internal planet number 
 * xx		planet's position accounted for light-time
 * dt		dt of light-time
 */
void swi_deflect_light(swe_ctx *ctx, double *xx, double dt, int32 iflag) 
{
  int i;
  double xx2[6];
  double u[6], e[6], q[6], ru, re, rq, uq, ue, qe, g1, g2;
#if 1
  double xx3[6], dx1, dx2, dtsp;
#endif
  double xsun[6], xearth[6];
  double sina, sin_sunr, meff_fact;
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  int32 iephe = pedp->iephe;
  for (i = 0; i <= 5; i++)
    xearth[i] = pedp->x[i];
  if (iflag & SEFLG_TOPOCTR)
    for (i = 0; i <= 5; i++)
      xearth[i] += ctx->topd.xobs[i];
  /* U = planetbary(t-tau) - earthbary(t) = planetgeo */
  for (i = 0; i <= 2; i++) 
    u[i] = xx[i]; 
  /* Eh = earthbary(t) - sunbary(t) = earthhel */
  if (iephe == SEFLG_JPLEPH || iephe == SEFLG_SWIEPH) {
    for (i = 0; i <= 2; i++) 
      e[i] = xearth[i] - psdp->x[i]; 
  } else {
    for (i = 0; i <= 2; i++) 
      e[i] = xearth[i];
  }
  /* Q = planetbary(t-tau) - sunbary(t-tau) = 'planethel' */
  /* first compute sunbary(t-tau) for */
  if (iephe == SEFLG_JPLEPH || iephe == SEFLG_SWIEPH) {
    for (i = 0; i <= 2; i++)
      /* this is sufficient precision */
      xsun[i] = psdp->x[i] - dt * psdp->x[i+3];
    for (i = 3; i <= 5; i++)
      xsun[i] = psdp->x[i];
  } else {
    for (i = 0; i <= 5; i++)
      xsun[i] = psdp->x[i];
  }
  for (i = 0; i <= 2; i++)
    q[i] = xx[i] + xearth[i] - xsun[i];
  ru = sqrt(square_sum(u));
  rq = sqrt(square_sum(q));
  re = sqrt(square_sum(e));
  for (i = 0; i <= 2; i++) {
    u[i] /= ru;
    q[i] /= rq;
    e[i] /= re;
  }
  uq = dot_prod(u,q);
  ue = dot_prod(u,e);
  qe = dot_prod(q,e);
  /* When a planet approaches the center of the sun in superior
   * conjunction, the formula for the deflection angle as given
   * in Expl. Suppl. p. 136 cannot be used. The deflection seems
   * to increase rapidly towards infinity. The reason is that the 
   * formula considers the sun as a point mass. AA recommends to 
   * set deflection = 0 in such a case. 
   * However, to get a continous motion, we modify the formula
   * for a non-point-mass, taking into account the mass distribution
   * within the sun. For more info, s. meff().
   */
  sina = sqrt(1 - ue * ue);	/* sin(angle) between sun and planet */
  sin_sunr = SUN_RADIUS / re; 	/* sine of sun radius (= sun radius) */
  if (sina < sin_sunr) 	{
    meff_fact = meff(sina / sin_sunr);
  } else {
    meff_fact = 1;
  }
  g1 = 2.0 * HELGRAVCONST * meff_fact / CLIGHT / CLIGHT / AUNIT / re; 
  g2 = 1.0 + qe;
  /* compute deflected position */
  for (i = 0; i <= 2; i++) 
    xx2[i] = ru * (u[i] + g1/g2 * (uq * e[i] - ue * q[i]));
  if (iflag & SEFLG_SPEED) {
    /* correction of speed
     * influence of light deflection on a planet's apparent speed:
     * for an outer planet at the solar limb with 
     * |v(planet) - v(sun)| = 1 degree, this makes a difference of 7"/day. 
     * if the planet is within the solar disc, the difference may increase
     * to 30" or more.
     * e.g. mercury at j2434871.45: 
     *	distance from sun 		45"
     *	1. speed without deflection     2d10'10".4034
     *    2. speed with deflection        2d10'42".8460 (-speed flag)
     *    3. speed with deflection        2d10'43".4824 (< 3 positions/
     *							   -speed3 flag)
     * 3. is not very precise. Smaller dt would give result closer to 2.,
     * but will probably never be as good as 2, unless int32 doubles are 
     * used. (try also j2434871.46!!)
     * however, in such a case speed changes rapidly. before being
     * passed by the sun, the planet accelerates, and after the sun
     * has passed it slows down. some time later it regains 'normal'
     * speed.
     * to compute speed, we do the same calculation as above with
     * slightly different u, e, q, and find out the difference in
     * deflection.
     */
    dtsp = -DEFL_SPEED_INTV;
    /* U = planetbary(t-tau) - earthbary(t) = planetgeo */
    for (i = 0; i <= 2; i++) 
      u[i] = xx[i] - dtsp * xx[i+3]; 
    /* Eh = earthbary(t) - sunbary(t) = earthhel */
    if (iephe == SEFLG_JPLEPH || iephe == SEFLG_SWIEPH) {
      for (i = 0; i <= 2; i++) 
	e[i] = xearth[i] - psdp->x[i] -
	       dtsp * (xearth[i+3] - psdp->x[i+3]); 
    } else
      for (i = 0; i <= 2; i++) 
	e[i] = xearth[i] - dtsp * xearth[i+3];
    /* Q = planetbary(t-tau) - sunbary(t-tau) = 'planethel' */
    for (i = 0; i <= 2; i++)
      q[i] = u[i] + xearth[i] - xsun[i] -
	     dtsp * (xearth[i+3] - xsun[i+3]); 
    ru = sqrt(square_sum(u));
    rq = sqrt(square_sum(q));
    re = sqrt(square_sum(e));
    for (i = 0; i <= 2; i++) {
      u[i] /= ru;
      q[i] /= rq;
      e[i] /= re;
    }
    uq = dot_prod(u,q);
    ue = dot_prod(u,e);
    qe = dot_prod(q,e);
    sina = sqrt(1 - ue * ue);	/* sin(angle) between sun and planet */
    sin_sunr = SUN_RADIUS / re; 	/* sine of sun radius (= sun radius) */
    if (sina < sin_sunr) {
      meff_fact = meff(sina / sin_sunr);
    } else {
      meff_fact = 1;
    }
    g1 = 2.0 * HELGRAVCONST * meff_fact / CLIGHT / CLIGHT / AUNIT / re; 
    g2 = 1.0 + qe;
    for (i = 0; i <= 2; i++) 
      xx3[i] = ru * (u[i] + g1/g2 * (uq * e[i] - ue * q[i]));
    for (i = 0; i <= 2; i++) {
      dx1 = xx2[i] - xx[i];
      dx2 = xx3[i] - u[i] * ru;
      dx1 -= dx2;
      xx[i+3] += dx1 / dtsp;
    }
  } /* endif speed */
  /* deflected position */
  for (i = 0; i <= 2; i++) 
    xx[i] = xx2[i];
}

/* converts the sun from barycentric to geocentric,
 *          the earth from barycentric to heliocentric
 * computes
 * apparent position,
 * precession, and nutation
 * according to flags
 * iflag	flags
 * serr         error string
 */
static int app_pos_etc_sun(swe_ctx *ctx, int32 iflag, char *serr)
{
  int i, j, niter, retc = OK;
  int32 flg1, flg2;
  double xx[6], xxsv[6], dx[3], dt, t = 0;
  double xearth[6], xsun[6], xobs[6];
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  struct epsilon *oe = &ctx->oec2000;
  /* if the same conversions have already been done for the same 
   * date, then return */
  flg1 = iflag & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  flg2 = pedp->xflgs & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  if (flg1 == flg2) {
    pedp->xflgs = iflag;
    pedp->iephe = iflag & SEFLG_EPHMASK;
    return OK;
  }
  /************************************
   * observer: geocenter or topocenter
   ************************************/
  /* if topocentric position is wanted  */
  if (iflag & SEFLG_TOPOCTR) { 
    if (ctx->topd.teval != pedp->teval
      || ctx->topd.teval == 0) {
      if (swi_get_observer(ctx, pedp->teval, iflag | SEFLG_NONUT, DO_SAVE, xobs, serr) != OK)
        return ERR;
    } else {
      for (i = 0; i <= 5; i++)
        xobs[i] = ctx->topd.xobs[i];
    }
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++)
      xobs[i] = xobs[i] + pedp->x[i];	
  } else {
    /* barycentric position of geocenter */
    for (i = 0; i <= 5; i++)
      xobs[i] = pedp->x[i];
  }
  /***************************************
   * true heliocentric position of earth *
   ***************************************/
  if (pedp->iephe == SEFLG_MOSEPH || (iflag & SEFLG_BARYCTR)) {
    for (i = 0; i <= 5; i++) 
      xx[i] = xobs[i];
  } else {
    for (i = 0; i <= 5; i++) 
      xx[i] = xobs[i] - psdp->x[i];
  }
  /*******************************
   * light-time                  * 
   *******************************/
  if (!(iflag & SEFLG_TRUEPOS)) {
    /* number of iterations - 1 
     * the following if() does the following:
     * with jpl and swiss ephemeris:
     *   with geocentric computation of sun:
     *     light-time correction of barycentric sun position.
     *   with heliocentric or barycentric computation of earth:
     *     light-time correction of barycentric earth position.
     * with moshier ephemeris (heliocentric!!!): 
     *   with geocentric computation of sun:
     *     nothing! (aberration will be done later)
     *   with heliocentric or barycentric computation of earth:
     *     light-time correction of heliocentric earth position.
     */
    if (pedp->iephe == SEFLG_JPLEPH || pedp->iephe == SEFLG_SWIEPH
      || (iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
      for (i = 0; i <= 5; i++) {
        xearth[i] = xobs[i];
	if (pedp->iephe == SEFLG_MOSEPH)
	  xsun[i] = 0;
	else 
	  xsun[i] = psdp->x[i];
      }
      niter = 1;	/* # of iterations */
      for (j = 0; j <= niter; j++) {
	/* distance earth-sun */
	for (i = 0; i <= 2; i++) {
	  dx[i] = xearth[i];
	  if (!(iflag & SEFLG_BARYCTR))
	    dx[i] -= xsun[i];
	}
	/* new t */
	dt = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;     
	t = pedp->teval - dt;
	/* new position */
	switch(pedp->iephe) {
	  /* if geocentric sun, new sun at t' 
	   * if heliocentric or barycentric earth, new earth at t' */
	  case SEFLG_JPLEPH:
	    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR))
	      retc = swi_pleph(ctx, t, J_EARTH, J_SBARY, xearth, serr);
	    else
	      retc = swi_pleph(ctx, t, J_SUN, J_SBARY, xsun, serr);
	    if (retc != OK) {
	      swi_close_jpl_file(ctx);
	      ctx->jpl_file_is_open = FALSE;
	      return(retc);
	    } 
	    break;
	  case SEFLG_SWIEPH:
	    /*
	      retc = sweph(ctx, t, SEI_SUN, SEI_FILE_PLANET, iflag, NULL, NO_SAVE, xearth, serr);
	    */
	    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
	      retc = sweplan(ctx, t, SEI_EARTH, SEI_FILE_PLANET, iflag, NO_SAVE, xearth, NULL, xsun, NULL, serr);
            } else {
	      retc = sweph(ctx, t, SEI_SUNBARY, SEI_FILE_PLANET, iflag, NULL, NO_SAVE, xsun, serr);
	    }
	    break;
	  case SEFLG_MOSEPH:
	    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR))
	      retc = swi_moshplan(ctx, t, SEI_EARTH, NO_SAVE, xearth, xearth, serr);
	    /* with moshier there is no barycentric sun */
	    break;
          default:
	    retc = ERR;
	    break;
	} 
	if (retc != OK)
	  return(retc);
      } 
      /* apparent heliocentric earth */
      for (i = 0; i <= 5; i++) {
        xx[i] = xearth[i];
	if (!(iflag & SEFLG_BARYCTR))
	  xx[i] -= xsun[i];
      }
    } 
  } 
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /*******************************
   * conversion to geocenter     * 
   *******************************/
  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
    for (i = 0; i <= 5; i++) 
      xx[i] = -xx[i]; 
  /**********************************
   * 'annual' aberration of light   *
   **********************************/
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOABERR)) {
		/* SEFLG_NOABERR is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_aberr_light(xx, xobs, iflag);
  }
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && swi_get_denum(ctx, SEI_SUN, iflag) >= 403) {
    swi_bias(ctx, xx, t, iflag, FALSE);
  }/**/
  /* save J2000 coordinates; required for sidereal positions */
  for (i = 0; i <= 5; i++)
    xxsv[i] = xx[i];
  /************************************************
   * precession, equator 2000 -> equator of date *
   ************************************************/
  if (!(iflag & SEFLG_J2000)) {
    swi_precess(ctx, xx, pedp->teval, iflag, J2000_TO_J);/**/
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, xx, pedp->teval, iflag, J2000_TO_J);/**/ 
    oe = &ctx->oec;
  } else
    oe = &ctx->oec2000;
  return app_pos_rest(ctx, pedp, iflag, xx, xxsv, oe, serr);
}


/* transforms the position of the moon:
 * heliocentric position
 * barycentric position
 * astrometric position
 * apparent position
 * precession and nutation
 * 
 * note: 
 * for apparent positions, we consider the earth-moon
 * system as independant.
 * for astrometric positions (SEFLG_NOABERR), we 
 * consider the motions of the earth and the moon 
 * related to the solar system barycenter.
 */
static int app_pos_etc_moon(swe_ctx *ctx, int32 iflag, char *serr)
{
  int i;
  int32 flg1, flg2;
  double xx[6], xxsv[6], xobs[6], xxm[6], xs[6], xe[6], xobs2[6], dt;
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  struct plan_data *pdp = &ctx->pldat[SEI_MOON];
  struct epsilon *oe = &ctx->oec;
  double t = 0; 
  int32 retc; 
  /* if the same conversions have already been done for the same 
   * date, then return */
  flg1 = iflag & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  flg2 = pdp->xflgs & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  if (flg1 == flg2) {
    pdp->xflgs = iflag;
    pdp->iephe = iflag & SEFLG_EPHMASK;
    return OK;
  }
  /* the conversions will be done with xx[]. */
  for (i = 0; i <= 5; i++) {
    xx[i] = pdp->x[i];
    xxm[i] = xx[i];
  }
  /***********************************
   * to solar system barycentric
   ***********************************/
  for (i = 0; i <= 5; i++)
	xx[i] += pedp->x[i]; 
  /*******************************
   * observer
   *******************************/
  if (iflag & SEFLG_TOPOCTR) {
    if (ctx->topd.teval != pdp->teval
      || ctx->topd.teval == 0) {
      if (swi_get_observer(ctx, pdp->teval, iflag | SEFLG_NONUT, DO_SAVE, xobs, serr) != OK)
        return ERR;
    } else {
      for (i = 0; i <= 5; i++)
        xobs[i] = ctx->topd.xobs[i];
    }
    for (i = 0; i <= 5; i++)
      xxm[i] -= xobs[i];
    for (i = 0; i <= 5; i++)
      xobs[i] += pedp->x[i];
  } else if (iflag & SEFLG_BARYCTR) { 
    for (i = 0; i <= 5; i++)
      xobs[i] = 0;
    for (i = 0; i <= 5; i++)
      xxm[i] += pedp->x[i];
  } else if (iflag & SEFLG_HELCTR) {
    for (i = 0; i <= 5; i++)
      xobs[i] = psdp->x[i];
    for (i = 0; i <= 5; i++)
      xxm[i] += pedp->x[i] - psdp->x[i];
  } else {
    for (i = 0; i <= 5; i++)
      xobs[i] = pedp->x[i];
  }
  /*******************************
   * light-time                  * 
   *******************************/
  t = pdp->teval;
  if ((iflag & SEFLG_TRUEPOS) == 0) {
    dt = sqrt(square_sum(xxm)) * AUNIT / CLIGHT / 86400.0;     
    t = pdp->teval - dt;
    switch(pdp->iephe) {
      case SEFLG_JPLEPH:
        retc = swi_pleph(ctx, t, J_MOON, J_EARTH, xx, serr);
        if (retc == OK)
          retc = swi_pleph(ctx, t, J_EARTH, J_SBARY, xe, serr);
        if (retc == OK && (iflag & SEFLG_HELCTR))
          retc = swi_pleph(ctx, t, J_SUN, J_SBARY, xs, serr);
        if (retc != OK) {
	      swi_close_jpl_file(ctx);
	      ctx->jpl_file_is_open = FALSE;
        } 
	for (i = 0; i <= 5; i++)
	  xx[i] += xe[i];
	    break;
      case SEFLG_SWIEPH:
        retc = sweplan(ctx, t, SEI_MOON, SEI_FILE_MOON, iflag, NO_SAVE, xx, xe, xs, NULL, serr);
        if (retc != OK)
          return(retc);
	for (i = 0; i <= 5; i++)
	  xx[i] += xe[i];
	    break;
      case SEFLG_MOSEPH:
        /* this method results in an error of a milliarcsec in speed */
        for (i = 0; i <= 2; i++) {
          xx[i] -= dt * xx[i+3];
          xe[i] = pedp->x[i] - dt * pedp->x[i+3];
		  xe[i+3] = pedp->x[i+3];
	  xs[i] = 0;
	  xs[i+3] = 0;
        }
        break;
    } 
    if (iflag & SEFLG_TOPOCTR) {
      if (swi_get_observer(ctx, t, iflag | SEFLG_NONUT, NO_SAVE, xobs2, NULL) != OK)
	  return ERR;
      for (i = 0; i <= 5; i++)
	xobs2[i] += xe[i];
    } else if (iflag & SEFLG_BARYCTR) {
      for (i = 0; i <= 5; i++)
	xobs2[i] = 0;
    } else if (iflag & SEFLG_HELCTR) {
      for (i = 0; i <= 5; i++)
	xobs2[i] = xs[i];
    } else {
      for (i = 0; i <= 5; i++)
	xobs2[i] = xe[i];
    }
  }
  /*************************
   * to correct center 
   *************************/
  for (i = 0; i <= 5; i++)
    xx[i] -= xobs[i];
  /**********************************
   * 'annual' aberration of light   *
   **********************************/
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOABERR)) {
		/* SEFLG_NOABERR is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_aberr_light(xx, xobs, iflag);
    /* 
     * Apparent speed is also influenced by
     * the difference of speed of the earth between t and t-dt. 
     * Neglecting this would lead to an error of several 0.1"
     */
#if 1
    if (iflag & SEFLG_SPEED)
      for (i = 3; i <= 5; i++) 
        xx[i] += xobs[i] - xobs2[i];
#endif
  }
  /* if !speedflag, speed = 0 */
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && swi_get_denum(ctx, SEI_MOON, iflag) >= 403) {
    swi_bias(ctx, xx, t, iflag, FALSE);
  }/**/
  /* save J2000 coordinates; required for sidereal positions */
  for (i = 0; i <= 5; i++)
    xxsv[i] = xx[i];
  /************************************************
   * precession, equator 2000 -> equator of date *
   ************************************************/
  if (!(iflag & SEFLG_J2000)) {
    swi_precess(ctx, xx, pdp->teval, iflag, J2000_TO_J);
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, xx, pdp->teval, iflag, J2000_TO_J); 
    oe = &ctx->oec;
  } else
    oe = &ctx->oec2000;
  return app_pos_rest(ctx, pdp, iflag, xx, xxsv, oe, serr);
}

/* transforms the position of the barycentric sun:
 * precession and nutation
 * according to flags
 * iflag	flags
 * serr         error string
 */
static int app_pos_etc_sbar(swe_ctx *ctx, int32 iflag, char *serr)
{
  int i;
  double xx[6], xxsv[6], dt;
  struct plan_data *psdp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psbdp = &ctx->pldat[SEI_SUNBARY];
  struct epsilon *oe = &ctx->oec;
  /* the conversions will be done with xx[]. */
  for (i = 0; i <= 5; i++) 
    xx[i] = psbdp->x[i];
  /**************
   * light-time *
   **************/
  if (!(iflag & SEFLG_TRUEPOS)) {
    dt = sqrt(square_sum(xx)) * AUNIT / CLIGHT / 86400.0;     
    for (i = 0; i <= 2; i++) 
      xx[i] -= dt * xx[i+3];	/* apparent position */
  }
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && swi_get_denum(ctx, SEI_SUN, iflag) >= 403) {
    swi_bias(ctx, xx, psdp->teval, iflag, FALSE);
  }/**/
  /* save J2000 coordinates; required for sidereal positions */
  for (i = 0; i <= 5; i++)
    xxsv[i] = xx[i];
  /************************************************
   * precession, equator 2000 -> equator of date *
   ************************************************/
  if (!(iflag & SEFLG_J2000)) {
    swi_precess(ctx, xx, psbdp->teval, iflag, J2000_TO_J);
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, xx, psbdp->teval, iflag, J2000_TO_J); 
    oe = &ctx->oec;
  } else
    oe = &ctx->oec2000;
  return app_pos_rest(ctx, psdp, iflag, xx, xxsv, oe, serr);
}

/* transforms position of mean lunar node or apogee:
 * input is polar coordinates in mean ecliptic of date.
 * output is, according to iflag:
 * position accounted for light-time
 * position referred to J2000 (i.e. precession subtracted)
 * position with nutation 
 * equatorial coordinates
 * cartesian coordinates
 * heliocentric position is not allowed ??????????????
 *         DAS WAERE ZIEMLICH AUFWENDIG. SONNE UND ERDE MUESSTEN
 *         SCHON VORHANDEN SEIN!
 * ipl		bodynumber (SE_MEAN_NODE or SE_MEAN_APOG)
 * iflag	flags
 * serr         error string
 */
static int app_pos_etc_mean(swe_ctx *ctx, int ipl, int32 iflag, char *serr) 
{
  int i;
  int32 flg1, flg2;
  double xx[6], xxsv[6];
  struct plan_data *pdp = &ctx->nddat[ipl];
  struct epsilon *oe;
  /* if the same conversions have already been done for the same 
   * date, then return */
  flg1 = iflag & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  flg2 = pdp->xflgs & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  if (flg1 == flg2) {
    pdp->xflgs = iflag;
    pdp->iephe = iflag & SEFLG_EPHMASK;
    return OK;
  }
  for (i = 0; i <= 5; i++)
    xx[i] = pdp->x[i];
  /* cartesian equatorial coordinates */
  swi_polcart_sp(xx, xx);
  swi_coortrf2(xx, xx, -ctx->oec.seps, ctx->oec.ceps);
  swi_coortrf2(xx+3, xx+3, -ctx->oec.seps, ctx->oec.ceps);
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /* J2000 coordinates; required for sidereal positions */
  if (((iflag & SEFLG_SIDEREAL) 
    && (ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0))
      || (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE)) {
    for (i = 0; i <= 5; i++)
      xxsv[i] = xx[i];
    /* xxsv is not J2000 yet! */
    if (pdp->teval != J2000) {
      swi_precess(ctx, xxsv, pdp->teval, iflag, J_TO_J2000);
      if (iflag & SEFLG_SPEED)
        swi_precess_speed(ctx, xxsv, pdp->teval, iflag, J_TO_J2000); 
    }
  }
  /*****************************************************
   * if no precession, equator of date -> equator 2000 *
   *****************************************************/
  if (iflag & SEFLG_J2000) {
    swi_precess(ctx, xx, pdp->teval, iflag, J_TO_J2000); 
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, xx, pdp->teval, iflag, J_TO_J2000); 
    oe = &ctx->oec2000;
  } else
    oe = &ctx->oec;
  return app_pos_rest(ctx, pdp, iflag, xx, xxsv, oe, serr);
}

/* fetch chebyshew coefficients from sweph file for
 * tjd 		time
 * ipli		planet number
 * ifno		file number
 * serr		error string
 */
static int get_new_segment(swe_ctx *ctx, double tjd, int ipli, int ifno, char *serr) 
{
  int i, j, k, m, n, o, icoord, retc;
  int32 iseg;
  int32 fpos;
  int nsizes, nsize[6];
  int nco;
  int idbl;
  unsigned char c[4];
  struct plan_data *pdp = &ctx->pldat[ipli];
  struct file_data *fdp = &ctx->fidat[ifno];
  FILE *fp = fdp->fptr;
  int freord  = (int) fdp->iflg & SEI_FILE_REORD;
  int fendian = (int) fdp->iflg & SEI_FILE_LITENDIAN;
  uint32 longs[MAXORD+1];
  /* compute segment number */
  iseg = (int32) ((tjd - pdp->tfstart) / pdp->dseg);
  /*if (tjd - pdp->tfstart < 0)
      return(NOT_AVAILABLE);*/
  pdp->tseg0 = pdp->tfstart + iseg * pdp->dseg;
  pdp->tseg1 = pdp->tseg0 + pdp->dseg;
  /* get file position of coefficients from file */
  fpos = pdp->lndx0 + iseg * 3;
  retc = do_fread(ctx, (void *) &fpos, 3, 1, 4, fp, fpos, freord, fendian, ifno, serr);
  if (retc != OK)
    goto return_error_gns;
  fseek(fp, fpos, SEEK_SET);
  /* clear space of chebyshew coefficients */
  if (pdp->segp == NULL)
    pdp->segp = (double *) malloc((size_t) pdp->ncoe * 3 * 8);
  memset((void *) pdp->segp, 0, (size_t) pdp->ncoe * 3 * 8);
  /* read coefficients for 3 coordinates */
  for (icoord = 0; icoord < 3; icoord++) {
    idbl = icoord * pdp->ncoe;
    /* first read header */
    /* first bit indicates number of sizes of packed coefficients */
    retc = do_fread(ctx, (void *) &c[0], 1, 2, 1, fp, SEI_CURR_FPOS, freord, fendian, ifno, serr);
    if (retc != OK)
      goto return_error_gns;
    if (c[0] & 128) {
      nsizes = 6;
      retc = do_fread(ctx, (void *) (c+2), 1, 2, 1, fp, SEI_CURR_FPOS, freord, fendian, ifno, serr);
      if (retc != OK)
	goto return_error_gns;
      nsize[0] = (int) c[1] / 16;
      nsize[1] = (int) c[1] % 16;
      nsize[2] = (int) c[2] / 16;
      nsize[3] = (int) c[2] % 16;
      nsize[4] = (int) c[3] / 16;
      nsize[5] = (int) c[3] % 16;
      nco = nsize[0] + nsize[1] + nsize[2] + nsize[3] + nsize[4] + nsize[5];
    } else {
      nsizes = 4;
      nsize[0] = (int) c[0] / 16;
      nsize[1] = (int) c[0] % 16;
      nsize[2] = (int) c[1] / 16;
      nsize[3] = (int) c[1] % 16;
      nco = nsize[0] + nsize[1] + nsize[2] + nsize[3];
    }
    /* there may not be more coefficients than interpolation
     * order + 1 */
    if (nco > pdp->ncoe) {
      if (serr != NULL) {
	sprintf(serr, "error in ephemeris file: %d coefficients instead of %d. ", nco, pdp->ncoe);
	if (strlen(serr) + strlen(fdp->fnam) < AS_MAXCH - 1) {
	  sprintf(serr, "error in ephemeris file %s: %d coefficients instead of %d. ", fdp->fnam, nco, pdp->ncoe);
	}
      }
      free(pdp->segp);
      pdp->segp = NULL;
      return (ERR);
    }
    /* now unpack */
    for (i = 0; i < nsizes; i++) {
      if (nsize[i] == 0) 
	continue;
      if (i < 4) {
	j = (4 - i);
	k = nsize[i];
	retc = do_fread(ctx, (void *) &longs[0], j, k, 4, fp, SEI_CURR_FPOS, freord, fendian, ifno, serr);
	if (retc != OK)
	  goto return_error_gns;
	for (m = 0; m < k; m++, idbl++) {
	  if (longs[m] & 1) 	/* will be negative */
	    pdp->segp[idbl] = -(((longs[m]+1) / 2) / 1e+9 * pdp->rmax / 2); 
	  else
	    pdp->segp[idbl] = (longs[m] / 2) / 1e+9 * pdp->rmax / 2;
	}
      } else if (i == 4) {		/* half byte packing */
	j = 1;
	k = (nsize[i] + 1) / 2;
	retc = do_fread(ctx, (void *) longs, j, k, 4, fp, SEI_CURR_FPOS, freord, fendian, ifno, serr);
	if (retc != OK)
	  goto return_error_gns;
	for (m = 0, j = 0; 
	     m < k && j < nsize[i]; 
	     m++) {
	  for (n = 0, o = 16; 
	       n < 2 && j < nsize[i]; 
	       n++, j++, idbl++, longs[m] %= o, o /= 16) {
	    if (longs[m] & o) 
	      pdp->segp[idbl] = 
		   -(((longs[m]+o) / o / 2) * pdp->rmax / 2 / 1e+9);
	    else
	      pdp->segp[idbl] = (longs[m] / o / 2) * pdp->rmax / 2 / 1e+9;
	  } 
	}
      } else if (i == 5) {		/* quarter byte packing */
	j = 1;
	k = (nsize[i] + 3) / 4;
	retc = do_fread(ctx, (void *) longs, j, k, 4, fp, SEI_CURR_FPOS, freord, fendian, ifno, serr);
	if (retc != OK)
	  goto return_error_gns;
	for (m = 0, j = 0; 
	     m < k && j < nsize[i]; 
	     m++) {
	  for (n = 0, o = 64; 
	       n < 4 && j < nsize[i]; 
	       n++, j++, idbl++, longs[m] %= o, o /= 4) {
	    if (longs[m] & o) 
	      pdp->segp[idbl] = 
		   -(((longs[m]+o) / o / 2) * pdp->rmax / 2 / 1e+9);
	    else
	      pdp->segp[idbl] = (longs[m] / o / 2) * pdp->rmax / 2 / 1e+9;
	  } 
	}
      }
    }
  }
  return(OK);
return_error_gns:
  fclose(fdp->fptr);
  // free(fdp->fptr);  is not from malloc(), must not be freed by us
  fdp->fptr = NULL;
  free_planets(ctx);
  return ERR;
}

/* SWISSEPH
 * reads constants on ephemeris file
 * ifno         file #
 * serr         error string
 */
static int read_const(swe_ctx *ctx, int ifno, char *serr) 
{ 
  char *c, c2, *sp;
  char s[AS_MAXCH*2], s2[AS_MAXCH];
  char sastnam[41];
  int i, ipli, kpl;
  int retc;
  int fendian, freord;
  int lastnam = 19;
  FILE *fp;
  int32 lng;
  uint32 ulng;
  int32 flen, fpos;
  short nplan;
  int32 testendian;
  double doubles[20];
  struct plan_data *pdp;
  struct file_data *fdp = &ctx->fidat[ifno];
  char *serr_file_damage = "Ephemeris file %s is damaged (0%s). ";
  char *smsg = "";
  int nbytes_ipl = 2;
  fp = fdp->fptr;
  /************************************* 
   * version number of file            *
   *************************************/
  sp = fgets(s, AS_MAXCH, fp);
  if (sp == NULL || strstr(sp, "\r\n") == NULL) {
    goto file_damage;
  }
  sp = strchr(s, '\r');
  *sp = '\0';
  sp = s;
  while (isdigit((int) *sp) == 0 && *sp != '\0')
    sp++;
  if (*sp == '\0') {
    smsg = "a";
    goto file_damage;
  }
  /* version unused so far */ 
  fdp->fversion = atoi(sp);
  /************************************* 
   * correct file name?                *
   *************************************/
  sp = fgets(s, AS_MAXCH, fp);
  if (sp == NULL || strstr(sp, "\r\n") == NULL) {
    smsg = "b";
    goto file_damage;
  }
  /* file name, without path */
  sp = strrchr(fdp->fnam, (int) *DIR_GLUE);
  if (sp == NULL) {
    sp = fdp->fnam;
  } else {
    sp++;
  }
  strcpy(s2, sp);
  /* to lower case */
  for (sp = s2; *sp != '\0'; sp++)
    *sp = tolower((int) *sp);
  /* prepare string of should-be file name */
  sp = s + strlen(s) - 1;
  while (*sp == '\n' || *sp == '\r' || *sp == ' ') {
    *sp = '\0';
    sp--;
  }
  for (sp = s; *sp != '\0'; sp++)
    *sp = tolower((int) *sp);
  if (strcmp(s2, s) != 0) {
    if (serr != NULL) {
      sprintf(serr, "Ephemeris file name '%s' wrong; rename '%s' ", s2, s);
    }
    goto return_error;
  }
  /************************************* 
   * copyright                         *
   *************************************/
  sp = fgets(s, AS_MAXCH, fp);
  if (sp == NULL || strstr(sp, "\r\n") == NULL) {
    smsg = "c";
    goto file_damage;
  }
  /**************************************** 
   * orbital elements, if single asteroid *
   ****************************************/
  if (ifno == SEI_FILE_ANY_AST) {
    sp = fgets(s, AS_MAXCH * 2, fp);
    if (sp == NULL || strstr(sp, "\r\n") == NULL) {
      smsg = "d";
      goto file_damage;
    }
    /* MPC number and name; will be analyzed below:
     * search "asteroid name" */
    while(*sp == ' ') sp++;
    while(isdigit((int) *sp)) sp++;
    sp++;
    i = (int) (sp - s);
    strncpy(sastnam, s, lastnam+i);	// fixed 19-nov-19
    *(sastnam+lastnam+i) = '\0';
    /* save elements, they are required for swe_plan_pheno() */
    strcpy(ctx->astelem, s);
    /* required for magnitude */
    ctx->ast_H = atof(s + 35 + i);
    ctx->ast_G = atof(s + 42 + i);
    if (ctx->ast_G == 0) ctx->ast_G = 0.15;
    /* diameter in kilometers, not always given: */
    strncpy(s2, s+51+i, 7);
    *(s2 + 7) = '\0';
    ctx->ast_diam = atof(s2);
    if (ctx->ast_diam == 0) {
      /* estimate the diameter from magnitude; assume albedo = 0.15 */
      ctx->ast_diam = 1329/sqrt(0.15) * pow(10, -0.2 * ctx->ast_H);
    }
  }
  /************************************* 
   * one int32 for test of byte order   * 
   *************************************/
  if (fread((void *) &testendian, 4, 1, fp) != 1) {
    smsg = "e";
    goto file_damage;
  }
  /* is byte order correct?            */
  if (testendian == SEI_FILE_TEST_ENDIAN) {
    freord = SEI_FILE_NOREORD;
  } else {
    freord = SEI_FILE_REORD;
    sp = (char *) &lng;
    c = (char *) &testendian;
    for (i = 0; i < 4; i++)
      *(sp+i) = *(c+3-i);
    if (lng != SEI_FILE_TEST_ENDIAN) {
      smsg = "f";
      goto file_damage;
    }
  }
  /* is file bigendian or littlendian? 
   * test first byte of test integer, which is highest if bigendian */
  c = (char *) &testendian;
  c2 = SEI_FILE_TEST_ENDIAN / 16777216L;
  if (*c == c2) {
    fendian = SEI_FILE_BIGENDIAN;
  } else {
    fendian = SEI_FILE_LITENDIAN;
  }
  fdp->iflg = (int32) freord | fendian;
  /************************************* 
   * length of file correct?           * 
   *************************************/
  retc = do_fread(ctx, (void *) &lng, 4, 1, 4, fp, SEI_CURR_FPOS, freord,
fendian, ifno, serr);
  if (retc != OK)
    goto return_error;
  fpos = ftell(fp);
  if (fseek(fp, 0L, SEEK_END) != 0) {
    smsg = "g";
    goto file_damage;
  }
  flen = ftell(fp);
  if (lng != flen) {
    smsg = "h";
    goto file_damage;
  }
  /********************************************************** 
   * DE number of JPL ephemeris which this file is based on * 
   **********************************************************/
  retc = do_fread(ctx, (void *) &fdp->sweph_denum, 4, 1, 4, fp, fpos, freord,
fendian, ifno, serr);
  if (retc != OK)
    goto return_error;
  /************************************* 
   * start and end epoch of file       * 
   *************************************/
  retc = do_fread(ctx, (void *) &fdp->tfstart, 8, 1, 8, fp, SEI_CURR_FPOS,
freord, fendian, ifno, serr);
  if (retc != OK)
    goto return_error;
  retc = do_fread(ctx, (void *) &fdp->tfend, 8, 1, 8, fp, SEI_CURR_FPOS, freord,
fendian, ifno, serr);
  if (retc != OK)
    goto return_error;
  /************************************* 
   * how many planets are in file?     * 
   *************************************/
  retc = do_fread(ctx, (void *) &nplan, 2, 1, 2, fp, SEI_CURR_FPOS, freord, fendian, ifno, serr);
  if (retc != OK)
    goto return_error;
  if (nplan > 256) {
    nbytes_ipl = 4;
    nplan %= 256;
  }
  /* The bound is a file-format limit, but it also has to keep nplan within
   * fdp->ipl[], which is SEI_FILE_NMAXPLAN long. Those were independent
   * numbers -- a bare 20 here and a 50 in the header -- so shrinking
   * SEI_FILE_NMAXPLAN below 20 would have silently turned this check into
   * no protection at all. (C17_PERFORMANCE.md B2.) */
  static_assert(SEI_FILE_MAXPLAN <= SEI_FILE_NMAXPLAN,
      "the nplan bound must not exceed fdp->ipl[]'s capacity");
  if (nplan < 1 || nplan > SEI_FILE_MAXPLAN) {
    smsg = "i";
    goto file_damage;
  }
  fdp->npl = nplan;
  /* which ones?                       */
  retc = do_fread(ctx, (void *) fdp->ipl, nbytes_ipl, (int) nplan, sizeof(int), fp, SEI_CURR_FPOS,
freord, fendian, ifno, serr);
  if (retc != OK)
    goto return_error;
  /************************************* 
   * asteroid name                     * 
   *************************************/
  if (ifno == SEI_FILE_ANY_AST) {
    char sastno[12];
    int j;
    /* name of asteroid is taken from orbital elements record
     * read above */
    j = 4;	/* old astorb.dat had only 4 characters for MPC# */
    while (sastnam[j] != ' ' && j < 10)	/* new astorb.dat has 5 */
      j++;
    strncpy(sastno, sastnam, j);
    sastno[j] = '\0';
    i = (int) atol(sastno);
    if (i == fdp->ipl[0] - SE_AST_OFFSET ||
        i == fdp->ipl[0] // planetary moon
	) {
      /* element record is from bowell database */
      strncpy(fdp->astnam, sastnam+j+1, lastnam);
      fdp->astnam[lastnam] = '\0';
      /* overread old ast. name field */
      if (fread((void *) s, 30, 1, fp) != 1) {
	smsg = "j";
        goto file_damage;
      }
    } else {
      /* older elements record structure: the name
       * is taken from old name field */
      if (fread((void *) fdp->astnam, 30, 1, fp) != 1) {
	smsg = "k";
        goto file_damage;
      }
    }
    /* in worst case strlen of not null terminated area! */
    i = (int) (strlen(fdp->astnam) - 1);
    if (i < 0) 
      i = 0;
    sp = fdp->astnam + i;
    while(*sp == ' ') {
      sp--;
    }
    sp[1] = '\0';
    if ((sp = strstr(fdp->astnam, "  ")) != NULL)
      *sp = '\0';
  }
  /************************************* 
   * check CRC                         * 
   *************************************/
  fpos = ftell(fp);
  /* read CRC from file */
  retc = do_fread(ctx, (void *) &ulng, 4, 1, 4, fp, SEI_CURR_FPOS, freord,
fendian, ifno, serr);
  if (retc != OK)
    goto return_error;
  /* read check area from file */
  fseek(fp, 0L, SEEK_SET);
  /* must check that defined length of s is less than fpos */
  if (fpos - 1 > 2 * AS_MAXCH) {
    smsg = "l";
    goto file_damage;
  }
  if (fread((void *) s, (size_t) fpos, 1, fp) != 1) {
    smsg = "m";
    goto file_damage;
  }
#if 1
  if (swi_crc32((unsigned char *) s, (int) fpos) != ulng) {
    smsg = "n";
    goto file_damage;
  }
#endif
  fseek(fp, fpos+4, SEEK_SET);
  /************************************* 
   * read general constants            * 
   *************************************/
  /* clight, aunit, helgravconst, ratme, sunradius 
   * these constants are currently not in use */
  retc = do_fread(ctx, (void *) &doubles[0], 8, 5, 8, fp, SEI_CURR_FPOS, freord,
fendian, ifno, serr);
  if (retc != OK)
    goto return_error;
  ctx->gcdat.clight       = doubles[0];
  ctx->gcdat.aunit        = doubles[1];
  ctx->gcdat.helgravconst = doubles[2];
  ctx->gcdat.ratme        = doubles[3];
  ctx->gcdat.sunradius    = doubles[4];
  /************************************* 
   * read constants of planets         * 
   *************************************/
  for (kpl = 0; kpl < fdp->npl; kpl++) {
    /* get SEI_ planet number */
    ipli = fdp->ipl[kpl];
    if (ipli >= SE_AST_OFFSET) {
      pdp = &ctx->pldat[SEI_ANYBODY];
    } else if (ipli >= SE_PLMOON_OFFSET) {
      pdp = &ctx->pldat[SEI_ANYBODY];
    } else {
      pdp = &ctx->pldat[ipli];
    }
    pdp->ibdy = ipli;
    /* file position of planet's index */
    retc = do_fread(ctx, (void *) &pdp->lndx0, 4, 1, 4, fp, SEI_CURR_FPOS,
freord, fendian, ifno, serr);
    if (retc != OK)
      goto return_error;
    /* flags: helio/geocentric, rotation, reference ellipse */
    retc = do_fread(ctx, (void *) &pdp->iflg, 1, 1, sizeof(int32), fp,
SEI_CURR_FPOS, freord, fendian, ifno, serr);
    if (retc != OK)
      goto return_error;
    /* number of chebyshew coefficients / segment  */
    /* = interpolation order +1                    */
    retc = do_fread(ctx, (void *) &pdp->ncoe, 1, 1, sizeof(int), fp,
SEI_CURR_FPOS, freord, fendian, ifno, serr);
    if (retc != OK)
      goto return_error;
    /* rmax = normalisation factor */
    retc = do_fread(ctx, (void *) &lng, 4, 1, 4, fp, SEI_CURR_FPOS, freord,
fendian, ifno, serr);
    if (retc != OK)
      goto return_error;
    pdp->rmax = lng / 1000.0; 
    // planet's center of body, e.g. 9599 for Jupiter or Mars moons
    if (ipli >= SE_PLMOON_OFFSET && ipli < SE_AST_OFFSET) {
      if ((ipli % 100) == 99 || (ipli - 9000) / 100 == SE_MARS)
	pdp->rmax = lng / 1000000.0;
    }
    /* start and end epoch of planetary ephemeris,   */
    /* segment length, and orbital elements          */
    retc = do_fread(ctx, (void *) doubles, 8, 10, 8, fp, SEI_CURR_FPOS, freord,
fendian, ifno, serr);
    if (retc != OK)
      goto return_error;
    pdp->tfstart  = doubles[0];
    pdp->tfend    = doubles[1];
    pdp->dseg     = doubles[2];
    pdp->nndx     = (int32) ((doubles[1] - doubles[0] + 0.1) /doubles[2]);
    pdp->telem    = doubles[3];
    pdp->prot     = doubles[4];
    pdp->dprot    = doubles[5];
    pdp->qrot     = doubles[6];
    pdp->dqrot    = doubles[7];
    pdp->peri     = doubles[8];
    pdp->dperi    = doubles[9];
    /* alloc space for chebyshew coefficients */
    /* if reference ellipse is used, read its coefficients */
    if (pdp->iflg & SEI_FLG_ELLIPSE) {
      if (pdp->refep != NULL) { /* if switch to other eph. file */
        free((void *) pdp->refep);
	pdp->refep = NULL;    /* 2015-may-5 */  
        if (pdp->segp != NULL) {        
          free((void *) pdp->segp);     /* array of coefficients of */
          pdp->segp = NULL;     /* ephemeris segment        */  
        }
      }
      pdp->refep = (double *) malloc((size_t) pdp->ncoe * 2 * 8); 
      retc = do_fread(ctx, (void *) pdp->refep, 8, 2*pdp->ncoe, 8, fp,
SEI_CURR_FPOS, freord, fendian, ifno, serr); 
      if (retc != OK) {
	free(pdp->refep);  /* 2015-may-5 */
	pdp->refep = NULL;  /* 2015-may-5 */
	goto return_error;
      }
    }/**/
  }
  return(OK);
file_damage:
  if (serr != NULL) {
    *serr = '\0';
    if (strlen(serr_file_damage) + strlen(fdp->fnam) + strlen(smsg) < AS_MAXCH) {
      sprintf(serr, serr_file_damage, fdp->fnam, smsg);
    }
  }
return_error:
  fclose(fdp->fptr);
  // free(fdp->fptr);  is not from malloc(), must not be freed by us
  fdp->fptr = NULL;
  free_planets(ctx);
  return(ERR);
}

/* SWISSEPH
 * reads from a file and, if necessary, reorders bytes 
 * targ 	target pointer
 * size		size of item to be read
 * count	number of items
 * corrsize	in what size should it be returned 
 *		(e.g. 3 byte int -> 4 byte int)
 * fp		file pointer
 * fpos		file position: if (fpos >= 0) then fseek
 * freord	reorder bytes or no
 * fendian	little/bigendian
 * ifno		file number
 * serr		error string
 */
static int do_fread(swe_ctx *ctx, void *trg, int size, int count, int corrsize, FILE *fp, int32 fpos, int freord, int fendian, int ifno, char *serr)
{
  int i, j, k; 
  int totsize;
  unsigned char space[1000];
  unsigned char *targ = (unsigned char *) trg;
  totsize = size * count;
  if (fpos >= 0) 
    fseek(fp, fpos, SEEK_SET);
  /* if no byte reorder has to be done, and read size == return size */
  if (!freord && size == corrsize) {
    if (fread((void *) targ, (size_t) totsize, 1, fp) == 0) {
      if (serr != NULL) {
	strcpy(serr, "Ephemeris file is damaged (1). ");
	if (strlen(serr) + strlen(ctx->fidat[ifno].fnam) < AS_MAXCH - 1) {
	  sprintf(serr, "Ephemeris file %s is damaged (2).", ctx->fidat[ifno].fnam);
	}
      }
      return(ERR);
    } else
      return(OK);
  } else {
    if (fread((void *) &space[0], (size_t) totsize, 1, fp) == 0) {
      if (serr != NULL) {
	strcpy(serr, "Ephemeris file is damaged (3). ");
	if (strlen(serr) + strlen(ctx->fidat[ifno].fnam) < AS_MAXCH - 1) {
	  sprintf(serr, "Ephemeris file %s is damaged (4).", ctx->fidat[ifno].fnam);
	}
      }
      return(ERR);
    }
    if (size != corrsize) {
      memset((void *) targ, 0, (size_t) count * corrsize);
    }
    for(i = 0; i < count; i++) {
      for (j = size-1; j >= 0; j--) {
	if (freord) {
	  k = size-j-1;
	} else {
	  k = j;
	}
        if (size != corrsize) {
          if ((fendian == SEI_FILE_BIGENDIAN && !freord) ||
              (fendian == SEI_FILE_LITENDIAN &&  freord))
	    k += corrsize - size;
	}
        targ[i*corrsize+k] = space[i*size+j];
      }
    }
  }
  return(OK);
}

/* SWISSEPH
 * adds reference orbit to chebyshew series (if SEI_FLG_ELLIPSE),
 * rotates series to mean equinox of J2000
 *
 * ipli		planet number
 */
static void rot_back(swe_ctx *ctx, int ipli)
{
  int i;
  double t, tdiff;
  double qav, pav, dn;
  double omtild, com, som, cosih2;
  double x[MAXORD+1][3];
  double uix[3], uiy[3], uiz[3];
  double xrot, yrot, zrot;
  double *chcfx, *chcfy, *chcfz;
  double *refepx, *refepy;
  // epsilon as used in chopt.c
  // double eps2000 = 0.409092804;       	// eps 2000 in radians 
  double seps2000 = 0.39777715572793088;  	// sin(eps2000) 
  double ceps2000 = 0.91748206215761929;	// cos(eps2000) 
  struct plan_data *pdp = &ctx->pldat[ipli];
  int nco = pdp->ncoe;
  t = pdp->tseg0 + pdp->dseg / 2;
  chcfx = pdp->segp;
  chcfy = chcfx + nco;
  chcfz = chcfx + 2 * nco;
  tdiff= (t - pdp->telem) / 365250.0;
  if (ipli == SEI_MOON) {
    dn = pdp->prot + tdiff * pdp->dprot;
    i = (int) (dn / TWOPI);
    dn -= i * TWOPI;
    qav = (pdp->qrot + tdiff * pdp->dqrot) * cos(dn);
    pav = (pdp->qrot + tdiff * pdp->dqrot) * sin(dn);
  } else {
    qav = pdp->qrot + tdiff * pdp->dqrot;
    pav = pdp->prot + tdiff * pdp->dprot;
  }
  /*calculate cosine and sine of average perihelion longitude. */
  for (i = 0; i < nco; i++) {
    x[i][0] = chcfx[i];
    x[i][1] = chcfy[i];
    x[i][2] = chcfz[i];
  }
  if (pdp->iflg & SEI_FLG_ELLIPSE) {
    refepx = pdp->refep;
    refepy = refepx + nco;
    omtild = pdp->peri + tdiff * pdp->dperi;
    i = (int) (omtild / TWOPI);
    omtild -= i * TWOPI;
    com = cos(omtild);
    som = sin(omtild);
    /*add reference orbit.  */ 
    for (i = 0; i < nco; i++) {
      x[i][0] = chcfx[i] + com * refepx[i] - som * refepy[i];
      x[i][1] = chcfy[i] + com * refepy[i] + som * refepx[i];
    }
  }
  /* construct right handed orthonormal system with first axis along
     origin of longitudes and third axis along angular momentum    
     this uses the standard formulas for equinoctal variables   
     (see papers by broucke and by cefola).      */
  cosih2 = 1.0 / (1.0 + qav * qav + pav * pav);
  /*     calculate orbit pole. */
  uiz[0] = 2.0 * pav * cosih2;
  uiz[1] = -2.0 * qav * cosih2;
  uiz[2] = (1.0 - qav * qav - pav * pav) * cosih2;
  /*     calculate origin of longitudes vector. */
  uix[0] = (1.0 + qav * qav - pav * pav) * cosih2;
  uix[1] = 2.0 * qav * pav * cosih2;
  uix[2] = -2.0 * pav * cosih2;
  /*     calculate vector in orbital plane orthogonal to origin of    
        longitudes.                                               */ 
  uiy[0] =2.0 * qav * pav * cosih2;
  uiy[1] =(1.0 - qav * qav + pav * pav) * cosih2;
  uiy[2] =2.0 * qav * cosih2;
  /*     rotate to actual orientation in space.         */ 
  for (i = 0; i < nco; i++) {
    xrot = x[i][0] * uix[0] + x[i][1] * uiy[0] + x[i][2] * uiz[0];
    yrot = x[i][0] * uix[1] + x[i][1] * uiy[1] + x[i][2] * uiz[1];
    zrot = x[i][0] * uix[2] + x[i][1] * uiy[2] + x[i][2] * uiz[2];
    if (fabs(xrot) + fabs(yrot) + fabs(zrot) >= 1e-14) 
      pdp->neval = i;
    x[i][0] = xrot;
    x[i][1] = yrot;
    x[i][2] = zrot;
    if (ipli == SEI_MOON) {
      /* rotate to j2000 equator */
      x[i][1] = ceps2000 * yrot - seps2000 * zrot;
      x[i][2] = seps2000 * yrot + ceps2000 * zrot;
    }
  }
  for (i = 0; i < nco; i++) {
    chcfx[i] = x[i][0];
    chcfy[i] = x[i][1];
    chcfz[i] = x[i][2];
  }
}

/* Adjust position from Earth-Moon barycenter to Earth
 *
 * xemb = hel./bar. position or velocity vectors of emb (input)
 *                                                  earth (output)
 * xmoon= geocentric position or velocity vector of moon
 */
/* restrict: the only two call sites are embofs(xpe, xpm) and
 * embofs(xpe+3, xpm+3) at sweph.c:2101,2109 -- distinct arrays. */
static void embofs(double * SWI_RESTRICT xemb, const double * SWI_RESTRICT xmoon)
{
  int i;
  for (i = 0; i <= 2; i++)
    xemb[i] -= xmoon[i] / (EARTH_MOON_MRAT + 1.0);
}

/* calculates the nutation matrix
 * nu		pointer to nutation data structure
 * oe		pointer to epsilon data structure
 */
static void nut_matrix(struct nut *nu, struct epsilon *oe) 
{
  double psi, eps;
  double sinpsi, cospsi, sineps, coseps, sineps0, coseps0;
  psi = nu->nutlo[0];
  eps = oe->eps + nu->nutlo[1];
  sinpsi = sin(psi);
  cospsi = cos(psi);
  sineps0 = oe->seps;
  coseps0 = oe->ceps;
  sineps = sin(eps);
  coseps = cos(eps);
  nu->matrix[0][0] = cospsi;
  nu->matrix[0][1] = sinpsi * coseps;
  nu->matrix[0][2] = sinpsi * sineps;
  nu->matrix[1][0] = -sinpsi * coseps0;
  nu->matrix[1][1] = cospsi * coseps * coseps0 + sineps * sineps0;
  nu->matrix[1][2] = cospsi * sineps * coseps0 - coseps * sineps0;
  nu->matrix[2][0] = -sinpsi * sineps0;
  nu->matrix[2][1] = cospsi * coseps * sineps0 - sineps * coseps0;
  nu->matrix[2][2] = cospsi * sineps * sineps0 + coseps * coseps0;
}

/* lunar osculating elements, i.e.
 * osculating node ('true' node) and
 * osculating apogee ('black moon', 'lilith').
 * tjd		julian day
 * ipl		body number, i.e. SEI_TRUE_NODE or SEI_OSCU_APOG
 * iflag	flags (which ephemeris, nutation, etc.)
 * serr		error string
 *
 * definitions and remarks:
 * the osculating node and the osculating apogee are defined
 * as the orbital elements of the momentary lunar orbit.
 * their advantage is that when the moon crosses the ecliptic,
 * it is really at the osculating node, and when it passes
 * its greatest distance from earth it is really at the
 * osculating apogee. with the mean elements this is not
 * the case. (some define the apogee as the second focus of 
 * the lunar ellipse. but, as seen from the geocenter, both 
 * points are in the same direction.)
 * problems:
 * the osculating apogee is given in the 'New International
 * Ephemerides' (Editions St. Michel) as the 'True Lilith'.
 * however, this name is misleading. this point is based on
 * the idea that the lunar orbit can be approximated by an
 * ellipse. 
 * arguments against this: 
 * 1. this procedure considers celestial motions as two body
 *    problems. this is quite good for planets, but not for
 *    the moon. the strong gravitational attraction of the sun 
 *    destroys the idea of an ellipse.
 * 2. the NIE 'True Lilith' has strong oscillations around the
 *    mean one with an amplitude of about 30 degrees. however,
 *    when the moon is in apogee, its distance from the mean
 *    apogee never exceeds 5 degrees.
 * besides, the computation of NIE is INACCURATE. the mistake 
 * reaches 20 arc minutes.
 * According to Santoni, the point was calculated using 'les 58
 * premiers termes correctifs au Perigee moyen' published by
 * Chapront and Chapront-Touze. And he adds: "Nous constatons
 * que meme en utilisant ces 58 termes CORRECTIFS, l'erreur peut
 * atteindre 0,5d!" (p. 13) We avoid this error, computing the
 * orbital elements directly from the position and the speed vector.
 *
 * how about the node? it is less problematic, because we
 * we needn't derive it from an orbital ellipse. we can say:
 * the axis of the osculating nodes is the intersection line of
 * the actual orbital plane of the moon and the plane of the 
 * ecliptic. or: the osculating nodes are the intersections of
 * the two great circles representing the momentary apparent 
 * orbit of the moon and the ecliptic. in this way they make
 * some sense. then, the nodes are really an axis, and they
 * have no geocentric distance. however, in this routine
 * we give a distance derived from the osculating ellipse.
 * the node could also be defined as the intersection axis
 * of the lunar orbital plane and the solar orbital plane,
 * which is not precisely identical to the ecliptic. this 
 * would make a difference of several arcseconds.
 *
 * is it possible to keep the idea of a continuously moving
 * apogee that is exact at the moment when the moon passes
 * its greatest distance from earth?
 * to achieve this, we would probably have to interpolate between 
 * the actual apogees. 
 * the nodes could also be computed by interpolation. the resulting
 * nodes would deviate from the so-called 'true node' by less than
 * 30 arc minutes.
 *
 * sidereal and j2000 true node are first computed for the ecliptic
 * of epoch and then precessed to ecliptic of t0(ayanamsa) or J2000.
 * there is another procedure that computes the node for the ecliptic
 * of t0(ayanamsa) or J2000. it is excluded by
 * #ifdef SID_TNODE_FROM_ECL_T0
 */ 
static int lunar_osc_elem(swe_ctx *ctx, double tjd, int ipl, int32 iflag, char *serr) 
{
  int i, j, istart;
  int ipli = SEI_MOON;
  int32 epheflag = SEFLG_DEFAULTEPH; 
  int retc = ERR; 
  int32 flg1, flg2;
  double daya[2];
  struct plan_data *ndp, *ndnp, *ndap;
  struct epsilon *oe;
  double speed_intv = NODE_CALC_INTV;	/* to silence gcc warning */
  double a, b;
  double xpos[3][6], xx[3][6], xxa[3][6], xnorm[6], r[6];
  double *xp;
  double rxy, rxyz, t, dt, fac, sgn;
  double sinnode, cosnode, sinincl, cosincl, sinu, cosu, sinE, cosE;
  double uu, ny, sema, ecce, Gmsm, c2, v2, pp;
  int32 speedf1, speedf2;
#ifdef SID_TNODE_FROM_ECL_T0
  struct sid_data *sip = &ctx->sidd;
  struct epsilon oectmp;
#endif
  oe = &ctx->oec;
#ifdef SID_TNODE_FROM_ECL_T0
  if (iflag & SEFLG_SIDEREAL) {
    calc_epsilon(ctx, sip->t0, iflag, &oectmp);
    oe = &oectmp;
  } else if (iflag & SEFLG_J2000) {
    oe = &ctx->oec2000;
  }
#endif
  ndp = &ctx->nddat[ipl];
  /* if elements have already been computed for this date, return 
   * if speed flag has been turned on, recompute */
  flg1 = iflag & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  flg2 = ndp->xflgs & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  speedf1 = ndp->xflgs & SEFLG_SPEED;
  speedf2 = iflag & SEFLG_SPEED;
  if (tjd == ndp->teval 
	&& tjd != 0 
	&& flg1 == flg2
	&& (!speedf2 || speedf1)) {
    ndp->xflgs = iflag;
    ndp->iephe = iflag & SEFLG_EPHMASK;
    return OK;
  }
  /* the geocentric position vector and the speed vector of the
   * moon make up the lunar orbital plane. the position vector 
   * of the node is along the intersection line of the orbital 
   * plane and the plane of the ecliptic.
   * to calculate the osculating node, we need one lunar position
   * with speed.
   * to calculate the speed of the osculating node, we need 
   * three lunar positions and the speed of each of them.
   * this is relatively cheap, if the jpl-moon or the swisseph
   * moon is used. with the moshier moon this is much more 
   * expensive, because then we need 9 lunar positions for 
   * three speeds. but one position and speed can normally
   * be taken from swed.pldat[moon], which corresponds to
   * three moshier moon calculations.
   * the same is also true for the osculating apogee: we need 
   * three lunar positions and speeds.
   */
  /*********************************************
   * now three lunar positions with speeds     * 
   *********************************************/
  if (iflag & SEFLG_MOSEPH) {
    epheflag = SEFLG_MOSEPH;
  } else if (iflag & SEFLG_SWIEPH) {
    epheflag = SEFLG_SWIEPH;
  } else if (iflag & SEFLG_JPLEPH) {
    epheflag = SEFLG_JPLEPH;
  }
  /* there may be a moon of wrong ephemeris in save area
   * force new computation: */
  ctx->pldat[SEI_MOON].teval = 0;
  if (iflag & SEFLG_SPEED) {
    istart = 0;
  } else {
    istart = 2;
  }
  if (serr != NULL)
    *serr = '\0';
  three_positions:
  switch(epheflag) {
    case SEFLG_JPLEPH:
      speed_intv = NODE_CALC_INTV;
      for (i = istart; i <= 2; i++) {
	if (i == 0) {
	  t = tjd - speed_intv;
        } else if (i == 1) {
	  t = tjd + speed_intv;
        } else  {
	  t = tjd;
	}
	xp = xpos[i];
	retc = jplplan(ctx, t, ipli, iflag, NO_SAVE, xp, NULL, NULL, serr);
	/* read error or corrupt file */
	if (retc == ERR)
	  return(ERR);
	/* light-time-corrected moon for apparent node 
	 * this makes a difference of several milliarcseconds with
	 * the node and 0.1" with the apogee.
	 * the simple formual 'x[j] -= dt * speed' should not be 
	 * used here. the error would be greater than the advantage
	 * of computation speed. */
	if ((iflag & SEFLG_TRUEPOS) == 0 && retc >= OK) { 
	  dt = sqrt(square_sum(xpos[i])) * AUNIT / CLIGHT / 86400.0;     
	  retc = jplplan(ctx, t-dt, ipli, iflag, NO_SAVE, xpos[i], NULL, NULL, serr);/**/
	  /* read error or corrupt file */
	  if (retc == ERR)
	    return(ERR);
        }
	/* jpl ephemeris not on disk, or date beyond ephemeris range */
	if (retc == NOT_AVAILABLE) {
	  iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_SWIEPH;
	  epheflag = SEFLG_SWIEPH;
	  if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	    strcat(serr, " \ntrying Swiss Eph; ");
	  break;
	} else if (retc == BEYOND_EPH_LIMITS) {
	  if (tjd > MOSHLUEPH_START && tjd < MOSHLUEPH_END) {
	    iflag = (iflag & ~SEFLG_JPLEPH) | SEFLG_MOSEPH;
	    epheflag = SEFLG_MOSEPH;
	    if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	      strcat(serr, " \nusing Moshier Eph; ");
	    break;
	  } else
	    return ERR;
	}
	/* precession and nutation etc. */
	retc = swi_plan_for_osc_elem(ctx, iflag|SEFLG_SPEED, t, xpos[i]); /* retc is always ok */
      }
      break;
    case SEFLG_SWIEPH:
      speed_intv = NODE_CALC_INTV;
      for (i = istart; i <= 2; i++) {
	if (i == 0) {
	  t = tjd - speed_intv;
        } else if (i == 1) {
	  t = tjd + speed_intv;
        } else  {
	  t = tjd;
	}
	retc = swemoon(ctx, t, iflag | SEFLG_SPEED, NO_SAVE, xpos[i], serr);/**/
	if (retc == ERR)
	  return(ERR);
	/* light-time-corrected moon for apparent node (~ 0.006") */
	if ((iflag & SEFLG_TRUEPOS) == 0 && retc >= OK) { 
	  dt = sqrt(square_sum(xpos[i])) * AUNIT / CLIGHT / 86400.0;     
	  retc = swemoon(ctx, t-dt, iflag | SEFLG_SPEED, NO_SAVE, xpos[i], serr);/**/
	  if (retc == ERR)
	    return(ERR);
        }
	if (retc == NOT_AVAILABLE) {
	  if (tjd > MOSHPLEPH_START && tjd < MOSHPLEPH_END) {
	    iflag = (iflag & ~SEFLG_SWIEPH) | SEFLG_MOSEPH;
	    epheflag = SEFLG_MOSEPH;
	    if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
	      strcat(serr, " \nusing Moshier eph.; ");
	    break;
	  } else
	    return ERR;
	}
	/* precession and nutation etc. */
	retc = swi_plan_for_osc_elem(ctx, iflag|SEFLG_SPEED, t, xpos[i]); /* retc is always ok */
      }
      break;
    case SEFLG_MOSEPH:
      /* with moshier moon, we need a greater speed_intv, because here the
       * node and apogee oscillate wildly within small intervals */
      speed_intv = NODE_CALC_INTV_MOSH;
      for (i = istart; i <= 2; i++) {
	if (i == 0) {
	  t = tjd - speed_intv;
        } else if (i == 1) {
	  t = tjd + speed_intv;
        } else  {
	  t = tjd;
	}
	retc = swi_moshmoon(ctx, t, NO_SAVE, xpos[i], serr);/**/
	if (retc == ERR)
	  return(retc);
	/* precession and nutation etc. */
	retc = swi_plan_for_osc_elem(ctx, iflag|SEFLG_SPEED, t, xpos[i]); /* retc is always ok */
      }
      break;
    default:
      break;
  } 
  if (retc == NOT_AVAILABLE || retc == BEYOND_EPH_LIMITS)
    goto three_positions;
  /*********************************************
   * node with speed                           * 
   *********************************************/
  /* node is always needed, even if apogee is wanted */
  ndnp = &ctx->nddat[SEI_TRUE_NODE];
  /* three nodes */
  for (i = istart; i <= 2; i++) {
    if (fabs(xpos[i][5]) < 1e-15)
      xpos[i][5] = 1e-15;
    fac = xpos[i][2] / xpos[i][5];
    sgn = xpos[i][5] / fabs(xpos[i][5]);
    for (j = 0; j <= 2; j++)
      xx[i][j] = (xpos[i][j] - fac * xpos[i][j+3]) * sgn;
  }
  /* now we have the correct direction of the node, the
   * intersection of the lunar plane and the ecliptic plane.
   * the distance is the distance of the point where the tangent
   * of the lunar motion penetrates the ecliptic plane.
   * this can be very large, e.g. j2415080.37372.
   * below, a new distance will be derived from the osculating 
   * ellipse. 
   */
  /* save position and speed */
  for (i = 0; i <= 2; i++) {
    ndnp->x[i] = xx[2][i];
    if (iflag & SEFLG_SPEED) {
      b = (xx[1][i] - xx[0][i]) / 2;
      a = (xx[1][i] + xx[0][i]) / 2 - xx[2][i];
      ndnp->x[i+3] = (2 * a + b) / speed_intv;
    } else
      ndnp->x[i+3] = 0;
    ndnp->teval = tjd;
    ndnp->iephe = epheflag;
  }
  /************************************************************
   * apogee with speed                                        * 
   * must be computed anyway to get the node's distance       *
   ************************************************************/
  ndap = &ctx->nddat[SEI_OSCU_APOG];
  Gmsm = GEOGCONST * (1 + 1 / EARTH_MOON_MRAT) /AUNIT/AUNIT/AUNIT*86400.0*86400.0;
  /* three apogees */
  for (i = istart; i <= 2; i++) {
    /* node */
    rxy =  sqrt(xx[i][0] * xx[i][0] + xx[i][1] * xx[i][1]);
    cosnode = xx[i][0] / rxy;	
    sinnode = xx[i][1] / rxy;
    /* inclination */
    swi_cross_prod(xpos[i], xpos[i]+3, xnorm);
    rxy =  xnorm[0] * xnorm[0] + xnorm[1] * xnorm[1];
    c2 = (rxy + xnorm[2] * xnorm[2]);
    rxyz = sqrt(c2);
    rxy = sqrt(rxy);
    sinincl = rxy / rxyz;
    cosincl = sqrt(1 - sinincl * sinincl);
    /* argument of latitude */
    cosu = xpos[i][0] * cosnode + xpos[i][1] * sinnode;
    sinu = xpos[i][2] / sinincl;	
    uu = atan2(sinu, cosu);	
    /* semi-axis */
    rxyz = sqrt(square_sum(xpos[i]));
    v2 = square_sum((xpos[i]+3));
    sema = 1 / (2 / rxyz - v2 / Gmsm);	
    /* eccentricity */
    pp = c2 / Gmsm;
    ecce = sqrt(1 - pp / sema);	
    /* eccentric anomaly */
    cosE = 1 / ecce * (1 - rxyz / sema);	
    sinE = 1 / ecce / sqrt(sema * Gmsm) * dot_prod(xpos[i], (xpos[i]+3));
    /* true anomaly */
    ny = 2 * atan(sqrt((1+ecce)/(1-ecce)) * sinE / (1 + cosE));
    /* distance of apogee from ascending node */
    xxa[i][0] = swi_mod2PI(uu - ny + PI);
    xxa[i][1] = 0;			/* latitude */
    xxa[i][2] = sema * (1 + ecce);	/* distance */
    /* transformation to ecliptic coordinates */
    swi_polcart(xxa[i], xxa[i]);
    swi_coortrf2(xxa[i], xxa[i], -sinincl, cosincl);
    swi_cartpol(xxa[i], xxa[i]);
    /* adding node, we get apogee in ecl. coord. */
    xxa[i][0] += atan2(sinnode, cosnode);
    swi_polcart(xxa[i], xxa[i]);
    /* new distance of node from orbital ellipse:
     * true anomaly of node: */
    ny = swi_mod2PI(ny - uu);
    /* eccentric anomaly */
    cosE = cos(2 * atan(tan(ny / 2) / sqrt((1+ecce) / (1-ecce))));
    /* new distance */
    r[0] = sema * (1 - ecce * cosE);
    /* old node distance */
    r[1] = sqrt(square_sum(xx[i]));
    /* correct length of position vector */
    for (j = 0; j <= 2; j++)
      xx[i][j] *= r[0] / r[1];
  }
  /* save position and speed */
  for (i = 0; i <= 2; i++) {
    /* apogee */
    ndap->x[i] = xxa[2][i];
    if (iflag & SEFLG_SPEED) {
      ndap->x[i+3] = (xxa[1][i] - xxa[0][i]) / speed_intv / 2;
    } else {
      ndap->x[i+3] = 0;
    }
    ndap->teval = tjd;
    ndap->iephe = epheflag;
    /* node */
    ndnp->x[i] = xx[2][i];
    if (iflag & SEFLG_SPEED) {
      ndnp->x[i+3] = (xx[1][i] - xx[0][i]) / speed_intv / 2;/**/    
    } else {
      ndnp->x[i+3] = 0;
    }
  }
  /**********************************************************************
   * precession and nutation have already been taken into account
   * because the computation is on the basis of lunar positions
   * that have gone through swi_plan_for_osc_elem. 
   * light-time is already contained in lunar positions.
   * now compute polar and equatorial coordinates:
   **********************************************************************/
  for (j = 0; j <= 1; j++) {
    double x[6];
    if (j == 0) {
      ndp = &ctx->nddat[SEI_TRUE_NODE];
    } else {
      ndp = &ctx->nddat[SEI_OSCU_APOG];
    }
    memset((void *) ndp->xreturn, 0, 24 * sizeof(double));
    /* cartesian ecliptic */
    for (i = 0; i <= 5; i++) 
      ndp->xreturn[6+i] = ndp->x[i];
    /* polar ecliptic */
    swi_cartpol_sp(ndp->xreturn+6, ndp->xreturn);
    /* cartesian equatorial */
    swi_coortrf2(ndp->xreturn+6, ndp->xreturn+18, -oe->seps, oe->ceps);
    if (iflag & SEFLG_SPEED)
      swi_coortrf2(ndp->xreturn+9, ndp->xreturn+21, -oe->seps, oe->ceps);
#ifdef SID_TNODE_FROM_ECL_T0
    /* sideral: we return NORMAL equatorial coordinates, there are no
     * sidereal ones */
    if (iflag & SEFLG_SIDEREAL) {
      /* to J2000 */
      swi_precess(ctx, ndp->xreturn+18, sip->t0, iflag, J_TO_J2000);
      if (iflag & SEFLG_SPEED)
	swi_precess_speed(ctx, ndp->xreturn+21, sip->t0, iflag, J_TO_J2000);
      if (!(iflag & SEFLG_J2000)) {
	/* to tjd */
	swi_precess(ctx, ndp->xreturn+18, tjd, iflag, J2000_TO_J);
	if (iflag & SEFLG_SPEED)
	  swi_precess_speed(ctx, ndp->xreturn+21, tjd, iflag, J2000_TO_J);
      }
    }
#endif
    if (!(iflag & SEFLG_NONUT)) {
      swi_coortrf2(ndp->xreturn+18, ndp->xreturn+18, -ctx->nut.snut, ctx->nut.cnut);
      if (iflag & SEFLG_SPEED)
	swi_coortrf2(ndp->xreturn+21, ndp->xreturn+21, -ctx->nut.snut, ctx->nut.cnut);
    }
    /* polar equatorial */
    swi_cartpol_sp(ndp->xreturn+18, ndp->xreturn+12);
    ndp->xflgs = iflag;
    ndp->iephe = iflag & SEFLG_EPHMASK;
#ifdef SID_TNODE_FROM_ECL_T0
    /* node and apogee are already referred to t0; 
     * nothing has to be done */
#else
    if (iflag & SEFLG_SIDEREAL) {
      /* node and apogee are referred to t; 
       * the ecliptic position must be transformed to t0 */
      /* rigorous algorithm */
      if ((ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) 
        || (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE)) {
	for (i = 0; i <= 5; i++)
	  x[i] = ndp->xreturn[18+i];
	/* remove nutation */
	if (!(iflag & SEFLG_NONUT))
	  swi_nutate(ctx, x, iflag, TRUE);
	/* precess to J2000 */
	swi_precess(ctx, x, tjd, iflag, J_TO_J2000);
	if (iflag & SEFLG_SPEED)
	  swi_precess_speed(ctx, x, tjd, iflag, J_TO_J2000);
        if (ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) {
	  swi_trop_ra2sid_lon(ctx, x, ndp->xreturn+6, ndp->xreturn+18, iflag);
	  /* project onto solar system equator */
        } else if (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE) {
          swi_trop_ra2sid_lon_sosy(ctx, x, ndp->xreturn+6, iflag);
	}
	/* to polar */
	swi_cartpol_sp(ndp->xreturn+6, ndp->xreturn);
        swi_cartpol_sp(ndp->xreturn+18, ndp->xreturn+12);
      /* traditional algorithm;
       * this is a bit clumsy, but allows us to keep the
       * sidereal code together */
      } else {
	swi_cartpol_sp(ndp->xreturn+6, ndp->xreturn); 
	if (swi_get_ayanamsa_with_speed(ctx, ndp->teval, iflag, daya, serr) == ERR)
	  return ERR;
	ndp->xreturn[0] -= daya[0] * DEGTORAD;
	ndp->xreturn[3] -= daya[1] * DEGTORAD;
	swi_polcart_sp(ndp->xreturn, ndp->xreturn+6); 
      }
    } else if (iflag & SEFLG_J2000) {
      /* node and apogee are referred to t; 
       * the ecliptic position must be transformed to J2000 */
      for (i = 0; i <= 5; i++)
        x[i] = ndp->xreturn[18+i];
      /* precess to J2000 */
      swi_precess(ctx, x, tjd, iflag, J_TO_J2000);
      if (iflag & SEFLG_SPEED)
        swi_precess_speed(ctx, x, tjd, iflag, J_TO_J2000);
      for (i = 0; i <= 5; i++)
        ndp->xreturn[18+i] = x[i];
      swi_cartpol_sp(ndp->xreturn+18, ndp->xreturn+12);
      swi_coortrf2(ndp->xreturn+18, ndp->xreturn+6, ctx->oec2000.seps, ctx->oec2000.ceps);
      if (iflag & SEFLG_SPEED)
        swi_coortrf2(ndp->xreturn+21, ndp->xreturn+9, ctx->oec2000.seps, ctx->oec2000.ceps);
      swi_cartpol_sp(ndp->xreturn+6, ndp->xreturn);
    }
#endif
    /********************** 
     * radians to degrees *
     **********************/
    /*if (!(iflag & SEFLG_RADIANS)) {*/
      for (i = 0; i < 2; i++) {
        ndp->xreturn[i] *= RADTODEG;	/* ecliptic */
        ndp->xreturn[i+3] *= RADTODEG;
        ndp->xreturn[i+12] *= RADTODEG;	/* equator */
        ndp->xreturn[i+15] *= RADTODEG;
      }
      ndp->xreturn[0] = swe_degnorm(ndp->xreturn[0]);
      ndp->xreturn[12] = swe_degnorm(ndp->xreturn[12]);
    /*}*/
  }
  return OK;
}

/* lunar osculating elements, i.e.
 */ 
static int intp_apsides(swe_ctx *ctx, double tjd, int ipl, int32 iflag, char *serr) 
{
  int i;
  int32 flg1, flg2;
  struct plan_data *ndp;
  struct epsilon *oe;
  struct nut *nut;
  double daya[2];
  double speed_intv = 0.1;
  double t, dt;
  double xpos[3][6], xx[6], x[6];
  int32 speedf1, speedf2;
  oe = &ctx->oec;
  nut = &ctx->nut;
  ndp = &ctx->nddat[ipl];
  /* if same calculation was done before, return
   * if speed flag has been turned on, recompute */
  flg1 = iflag & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  flg2 = ndp->xflgs & ~SEFLG_EQUATORIAL & ~SEFLG_XYZ;
  speedf1 = ndp->xflgs & SEFLG_SPEED;
  speedf2 = iflag & SEFLG_SPEED;
  if (tjd == ndp->teval 
	&& tjd != 0 
	&& flg1 == flg2
	&& (!speedf2 || speedf1)) {
    ndp->xflgs = iflag;
    ndp->iephe = iflag & SEFLG_MOSEPH;
    return OK;
  }
  /*********************************************
   * now three apsides * 
   *********************************************/
  for (t = tjd - speed_intv, i = 0; i < 3; t += speed_intv, i++) {
    if (! (iflag & SEFLG_SPEED) && i != 1) continue;
    swi_intp_apsides(ctx, t, xpos[i], ipl);
  }
  /************************************************************
   * apsis with speed                                         * 
   ************************************************************/
  for (i = 0; i < 3; i++) {
    xx[i] = xpos[1][i];
    xx[i+3] = 0;
  }
  if (iflag & SEFLG_SPEED) {
    xx[3] = swe_difrad2n(xpos[2][0], xpos[0][0]) / speed_intv / 2.0;
    xx[4] = (xpos[2][1] - xpos[0][1]) / speed_intv / 2.0;
    xx[5] = (xpos[2][2] - xpos[0][2]) / speed_intv / 2.0;
  }
  memset((void *) ndp->xreturn, 0, 24 * sizeof(double));
  /* ecliptic polar to cartesian */
  swi_polcart_sp(xx, xx);
  /* light-time */
  if (!(iflag & SEFLG_TRUEPOS)) {
    dt = sqrt(square_sum(xx)) * AUNIT / CLIGHT / 86400.0;     
    for (i = 1; i < 3; i++)
      xx[i] -= dt * xx[i+3];
  }
  for (i = 0; i <= 5; i++) 
    ndp->xreturn[i+6] = xx[i];
  /*printf("%.10f, %.10f, %.10f, %.10f\n", xx[0] /DEGTORAD, xx[1] / DEGTORAD, xx [2], xx[3] /DEGTORAD);*/
  /* equatorial cartesian */
  swi_coortrf2(ndp->xreturn+6, ndp->xreturn+18, -oe->seps, oe->ceps);
  if (iflag & SEFLG_SPEED)
    swi_coortrf2(ndp->xreturn+9, ndp->xreturn+21, -oe->seps, oe->ceps);
  ndp->teval = tjd;
  ndp->xflgs = iflag;
  ndp->iephe = iflag & SEFLG_EPHMASK;
  if (iflag & SEFLG_SIDEREAL) {
    /* apogee is referred to t; 
     * the ecliptic position must be transformed to t0 */
    /* rigorous algorithm */
    if ((ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) 
	|| (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE)) {
      for (i = 0; i <= 5; i++)
	x[i] = ndp->xreturn[18+i];
      /* precess to J2000 */
      swi_precess(ctx, x, tjd, iflag, J_TO_J2000);
      if (iflag & SEFLG_SPEED)
	swi_precess_speed(ctx, x, tjd, iflag, J_TO_J2000);
      if (ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) {
	swi_trop_ra2sid_lon(ctx, x, ndp->xreturn+6, ndp->xreturn+18, iflag);
      /* project onto solar system equator */
      } else if (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE) {
	swi_trop_ra2sid_lon_sosy(ctx, x, ndp->xreturn+6, iflag);
      }
      /* to polar */
      swi_cartpol_sp(ndp->xreturn+6, ndp->xreturn);
      swi_cartpol_sp(ndp->xreturn+18, ndp->xreturn+12);
    } else {
    /* traditional algorithm */
      swi_cartpol_sp(ndp->xreturn+6, ndp->xreturn); 
      if (swi_get_ayanamsa_with_speed(ctx, ndp->teval, iflag, daya, serr) == ERR)
        return ERR;
      ndp->xreturn[0] -= daya[0] * DEGTORAD;
      ndp->xreturn[3] -= daya[1] * DEGTORAD;
      swi_polcart_sp(ndp->xreturn, ndp->xreturn+6); 
      swi_cartpol_sp(ndp->xreturn+18, ndp->xreturn+12);
    }
  } else if (iflag & SEFLG_J2000) {
    /* node and apogee are referred to t; 
     * the ecliptic position must be transformed to J2000 */
    for (i = 0; i <= 5; i++)
      x[i] = ndp->xreturn[18+i];
    /* precess to J2000 */
    swi_precess(ctx, x, tjd, iflag, J_TO_J2000);
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, x, tjd, iflag, J_TO_J2000);
    for (i = 0; i <= 5; i++)
      ndp->xreturn[18+i] = x[i];
    swi_cartpol_sp(ndp->xreturn+18, ndp->xreturn+12);
    swi_coortrf2(ndp->xreturn+18, ndp->xreturn+6, ctx->oec2000.seps, ctx->oec2000.ceps);
    if (iflag & SEFLG_SPEED)
      swi_coortrf2(ndp->xreturn+21, ndp->xreturn+9, ctx->oec2000.seps, ctx->oec2000.ceps);
    swi_cartpol_sp(ndp->xreturn+6, ndp->xreturn);
  } else {
    /* tropical ecliptic positions */
    /* precession has already been taken into account, but not nutation */
    if (!(iflag & SEFLG_NONUT)) {
      swi_nutate(ctx, ndp->xreturn+18, iflag, FALSE);
    }
    /* equatorial polar */
    swi_cartpol_sp(ndp->xreturn+18, ndp->xreturn+12);
    /* ecliptic cartesian */
    swi_coortrf2(ndp->xreturn+18, ndp->xreturn+6, oe->seps, oe->ceps);
    if (iflag & SEFLG_SPEED)
      swi_coortrf2(ndp->xreturn+21, ndp->xreturn+9, oe->seps, oe->ceps);
    if (!(iflag & SEFLG_NONUT)) {
      swi_coortrf2(ndp->xreturn+6, ndp->xreturn+6, nut->snut, nut->cnut);
      if (iflag & SEFLG_SPEED)
	swi_coortrf2(ndp->xreturn+9, ndp->xreturn+9, nut->snut, nut->cnut);
    }
    /* ecliptic polar */
    swi_cartpol_sp(ndp->xreturn+6, ndp->xreturn);
  }
  /********************** 
   * radians to degrees *
   **********************/
  /*if (!(iflag & SEFLG_RADIANS)) {*/
  for (i = 0; i < 2; i++) {
    ndp->xreturn[i] *= RADTODEG;		/* ecliptic */
    ndp->xreturn[i+3] *= RADTODEG;
    ndp->xreturn[i+12] *= RADTODEG;	/* equator */
    ndp->xreturn[i+15] *= RADTODEG;
  }
  ndp->xreturn[0] = swe_degnorm(ndp->xreturn[0]);
  ndp->xreturn[12] = swe_degnorm(ndp->xreturn[12]);
  /*}*/
  return OK;
}

/* transforms the position of the moon in a way we can use it
 * for calculation of osculating node and apogee:
 * precession and nutation (attention to speed vector!)
 * according to flags
 * iflag	flags
 * tjd          time for which the element is computed
 *              i.e. date of ecliptic
 * xx           array equatorial cartesian position and speed
 * serr         error string
 */
int swi_plan_for_osc_elem(swe_ctx *ctx, int32 iflag, double tjd, double *xx)
{
  int i;
  double x[6];
  struct nut nuttmp;
  struct nut *nutp = &nuttmp;	/* dummy assign, to silence gcc warning */
  struct epsilon *oe = &ctx->oec;
  struct epsilon oectmp;
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && swi_get_denum(ctx, SEI_SUN, iflag) >= 403) {
    swi_bias(ctx, xx, tjd, iflag, FALSE);
  }/**/
  /************************************************
   * precession, equator 2000 -> equator of date  *
   * attention: speed vector has to be rotated,   *
   * but daily precession 0.137" may not be added!*/
#ifdef SID_TNODE_FROM_ECL_T0
  struct sid_data *sip = &ctx->sidd;
  /* For sidereal calculation we need node refered*
   * to ecliptic of t0 of ayanamsa                *
   ************************************************/
  if (iflag & SEFLG_SIDEREAL) {
    tjd = sip->t0;
    swi_precess(ctx, xx, tjd, iflag, J2000_TO_J);
    swi_precess(ctx, xx+3, tjd, iflag, J2000_TO_J); 
    calc_epsilon(ctx, tjd, iflag, &oectmp);
    oe = &oectmp;
  } else if (!(iflag & SEFLG_J2000)) {
#endif
    swi_precess(ctx, xx, tjd, iflag, J2000_TO_J);
    swi_precess(ctx, xx+3, tjd, iflag, J2000_TO_J); 
    /* epsilon */
    if (tjd == ctx->oec.teps) {
      oe = &ctx->oec;
    } else if (tjd == J2000) {
      oe = &ctx->oec2000;
    } else {
      calc_epsilon(ctx, tjd, iflag, &oectmp);
      oe = &oectmp;
    }
#ifdef SID_TNODE_FROM_ECL_T0
  } else {	/* if SEFLG_J2000 */
    oe = &ctx->oec2000;
  }
#endif
  /************************************************
   * nutation                                     *
   * again: speed vector must be rotated, but not *
   * added 'speed' of nutation                    *
   ************************************************/
  if (!(iflag & SEFLG_NONUT)) {
    if (tjd == ctx->nut.tnut) {
      nutp = &ctx->nut;
    } else if (tjd == J2000) {
      nutp = &ctx->nut2000;
    } else if (tjd == ctx->nutv.tnut) {
      nutp = &ctx->nutv;
    } else {
      nutp = &nuttmp;
      swi_nutation(ctx, tjd, iflag, nutp->nutlo);
      nutp->tnut = tjd;
      nutp->snut = sin(nutp->nutlo[1]);
      nutp->cnut = cos(nutp->nutlo[1]);
      nut_matrix(nutp, oe);
    }
    for (i = 0; i <= 2; i++) {
      x[i] = xx[0] * nutp->matrix[0][i] + 
	     xx[1] * nutp->matrix[1][i] + 
	     xx[2] * nutp->matrix[2][i];
    }
    /* speed:
     * rotation only */
    for (i = 0; i <= 2; i++) {
      x[i+3] = xx[3] * nutp->matrix[0][i] + 
	       xx[4] * nutp->matrix[1][i] + 
	       xx[5] * nutp->matrix[2][i];
    }
    for (i = 0; i <= 5; i++) 
      xx[i] = x[i];
  }
  /************************************************
   * transformation to ecliptic                   *
   ************************************************/
  swi_coortrf2(xx, xx, oe->seps, oe->ceps);
  swi_coortrf2(xx+3, xx+3, oe->seps, oe->ceps);
#ifdef SID_TNODE_FROM_ECL_T0
  if (iflag & SEFLG_SIDEREAL) {
    /* subtract ayan_t0 */
    swi_cartpol_sp(xx, xx);
    xx[0] -= sip->ayan_t0;
    swi_polcart_sp(xx, xx);
  } else 
#endif
  if (!(iflag & SEFLG_NONUT)) {
    swi_coortrf2(xx, xx, nutp->snut, nutp->cnut);
    swi_coortrf2(xx+3, xx+3, nutp->snut, nutp->cnut);
  }
  return(OK);
}

static const struct meff_ele eff_arr[] = {
  /*
   * r , m_eff for photon passing the sun at min distance r (fraction of Rsun)
   * the values where computed with sun_model.c, which is a classic
   * treatment of a photon passing a gravity field, multiplied by 2.
   * The sun mass distribution m(r) is from Michael Stix, The Sun, p. 47.
   */
  {1.000, 1.000000},
  {0.990, 0.999979},
  {0.980, 0.999940},
  {0.970, 0.999881},
  {0.960, 0.999811},
  {0.950, 0.999724},
  {0.940, 0.999622},
  {0.930, 0.999497},
  {0.920, 0.999354},
  {0.910, 0.999192},
  {0.900, 0.999000},
  {0.890, 0.998786},
  {0.880, 0.998535},
  {0.870, 0.998242},
  {0.860, 0.997919},
  {0.850, 0.997571},
  {0.840, 0.997198},
  {0.830, 0.996792},
  {0.820, 0.996316},
  {0.810, 0.995791},
  {0.800, 0.995226},
  {0.790, 0.994625},
  {0.780, 0.993991},
  {0.770, 0.993326},
  {0.760, 0.992598},
  {0.750, 0.991770},
  {0.740, 0.990873},
  {0.730, 0.989919},
  {0.720, 0.988912},
  {0.710, 0.987856},
  {0.700, 0.986755},
  {0.690, 0.985610},
  {0.680, 0.984398},
  {0.670, 0.982986},
  {0.660, 0.981437},
  {0.650, 0.979779},
  {0.640, 0.978024},
  {0.630, 0.976182},
  {0.620, 0.974256},
  {0.610, 0.972253},
  {0.600, 0.970174},
  {0.590, 0.968024},
  {0.580, 0.965594},
  {0.570, 0.962797},
  {0.560, 0.959758},
  {0.550, 0.956515},
  {0.540, 0.953088},
  {0.530, 0.949495},
  {0.520, 0.945741},
  {0.510, 0.941838},
  {0.500, 0.937790},
  {0.490, 0.933563},
  {0.480, 0.928668},
  {0.470, 0.923288},
  {0.460, 0.917527},
  {0.450, 0.911432},
  {0.440, 0.905035},
  {0.430, 0.898353},
  {0.420, 0.891022},
  {0.410, 0.882940},
  {0.400, 0.874312},
  {0.390, 0.865206},
  {0.380, 0.855423},
  {0.370, 0.844619},
  {0.360, 0.833074},
  {0.350, 0.820876},
  {0.340, 0.808031},
  {0.330, 0.793962},
  {0.320, 0.778931},
  {0.310, 0.763021},
  {0.300, 0.745815},
  {0.290, 0.727557},
  {0.280, 0.708234},
  {0.270, 0.687583},
  {0.260, 0.665741},
  {0.250, 0.642597},
  {0.240, 0.618252},
  {0.230, 0.592586},
  {0.220, 0.565747},
  {0.210, 0.537697},
  {0.200, 0.508554},
  {0.190, 0.478420},
  {0.180, 0.447322},
  {0.170, 0.415454},
  {0.160, 0.382892},
  {0.150, 0.349955},
  {0.140, 0.316691},
  {0.130, 0.283565},
  {0.120, 0.250431},
  {0.110, 0.218327},
  {0.100, 0.186794},
  {0.090, 0.156287},
  {0.080, 0.128421},
  {0.070, 0.102237},
  {0.060, 0.077393},
  {0.050, 0.054833},
  {0.040, 0.036361},
  {0.030, 0.020953},
  {0.020, 0.009645},
  {0.010, 0.002767},
  {0.000, 0.000000}
};
static double meff(double r)
{
  double f, m;
  int i;
  if (r <= 0) {
    return 0.0;
  } else if (r >= 1) {
    return 1.0;
  }
  for (i = 0; eff_arr[i].r > r; i++)
    ;	/* empty body */
  f = (r - eff_arr[i-1].r) / (eff_arr[i].r - eff_arr[i-1].r);
  m = eff_arr[i-1].m + f * (eff_arr[i].m - eff_arr[i-1].m);
  return m;
}

static void denormalize_positions(double *x0, double *x1, double *x2) 
{
  int i;
  /* x*[0] = ecliptic longitude, x*[12] = rectascension */
  for (i = 0; i <= 12; i += 12) {
    if (x1[i] - x0[i] < -180)
      x0[i] -= 360;
    if (x1[i] - x0[i] > 180)
      x0[i] += 360;
    if (x1[i] - x2[i] < -180)
      x2[i] -= 360;
    if (x1[i] - x2[i] > 180)
      x2[i] += 360;
  }
}

static void calc_speed(double *x0, double *x1, double *x2, double dt)
{
  int i, j, k;
  double a, b;
  for (j = 0; j <= 18; j += 6) {
    for (i = 0; i < 3; i++) {
      k = j + i;
      b = (x2[k] - x0[k]) / 2;
      a = (x2[k] + x0[k]) / 2 - x1[k];
      x1[k+3] = (2 * a + b) / dt;
    }
  }
}

void swi_check_ecliptic(swe_ctx *ctx, double tjd, int32 iflag)
{
  if (ctx->oec2000.teps != J2000) {
    calc_epsilon(ctx, J2000, iflag, &ctx->oec2000);
  }
  if (tjd == J2000) {
    ctx->oec.teps = ctx->oec2000.teps;
    ctx->oec.eps = ctx->oec2000.eps;
    ctx->oec.seps = ctx->oec2000.seps;
    ctx->oec.ceps = ctx->oec2000.ceps;
    return;
  }
  if (ctx->oec.teps != tjd || tjd == 0) {
    calc_epsilon(ctx, tjd, iflag, &ctx->oec);
  }
}

/* computes nutation, if it is wanted and has not yet been computed.
 * if speed flag has been turned on since last computation, 
 * nutation is recomputed */
void swi_check_nutation(swe_ctx *ctx, double tjd, int32 iflag)
{
  int32 speedf1, speedf2;
  /* moved to ctx->sp.nut (Phase 3c) */
  double t;
  speedf1 = ctx->sp.nut.nutflag & SEFLG_SPEED;
  speedf2 = iflag & SEFLG_SPEED;
  if (!(iflag & SEFLG_NONUT)
	&& (tjd != ctx->nut.tnut || tjd == 0
	|| (!speedf1 && speedf2))) {
    swi_nutation(ctx, tjd, iflag, ctx->nut.nutlo);
    ctx->nut.tnut = tjd;
    ctx->nut.snut = sin(ctx->nut.nutlo[1]);
    ctx->nut.cnut = cos(ctx->nut.nutlo[1]);
    ctx->sp.nut.nutflag = iflag;
    nut_matrix(&ctx->nut, &ctx->oec);
    if (iflag & SEFLG_SPEED) {
      /* once more for 'speed' of nutation, which is needed for 
       * planetary speeds */
      t = tjd - NUT_SPEED_INTV;
      swi_nutation(ctx, t, iflag, ctx->nutv.nutlo);
      ctx->nutv.tnut = t;
      ctx->nutv.snut = sin(ctx->nutv.nutlo[1]);
      ctx->nutv.cnut = cos(ctx->nutv.nutlo[1]);
      nut_matrix(&ctx->nutv, &ctx->oec);
    } 
  } 
} 

/* function
 * - corrects nonsensical iflags
 * - completes incomplete iflags
 */
static int32 plaus_iflag(swe_ctx *ctx, int32 iflag, int32 ipl, double tjd, char *serr)
{
  int32 epheflag = 0;
  (void) tjd;
  int jplhor_model = ctx->astro_models[SE_MODEL_JPLHOR_MODE];
  int jplhora_model = ctx->astro_models[SE_MODEL_JPLHORA_MODE];
  if (jplhor_model == 0) jplhor_model = SEMOD_JPLHOR_DEFAULT;
  if (jplhora_model == 0) jplhora_model = SEMOD_JPLHORA_DEFAULT;
  /* either Horizons mode or simplified Horizons mode, not both */
  if (iflag & SEFLG_JPLHOR)
    iflag &= ~SEFLG_JPLHOR_APPROX;
  /* if topocentric bit, turn helio- and barycentric bits off;
   */
  if (iflag & SEFLG_TOPOCTR) {
    iflag = iflag & ~(SEFLG_HELCTR | SEFLG_BARYCTR); 
  }
  /* if barycentric bit, turn heliocentric bit off */
  if (iflag & SEFLG_BARYCTR) 
    iflag = iflag & ~(SEFLG_HELCTR); 
  if (iflag & SEFLG_HELCTR) 
    iflag = iflag & ~(SEFLG_BARYCTR); 
  /* if heliocentric bit, turn aberration and deflection off */
  if (iflag & (SEFLG_HELCTR|SEFLG_BARYCTR)) 
    iflag |= SEFLG_NOABERR | SEFLG_NOGDEFL; /*iflag |= SEFLG_TRUEPOS;*/
  /* if no_precession bit is set, set also no_nutation bit */
  if (iflag & SEFLG_J2000)
    iflag |= SEFLG_NONUT;
  /* if sidereal bit is set, set also no_nutation bit *
   * also turn JPL Horizons mode off */
  if (iflag & SEFLG_SIDEREAL) {
    iflag |= SEFLG_NONUT;
    iflag = iflag & ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
  }
  /* if truepos is set, turn off grav. defl. and aberration */
  if (iflag & SEFLG_TRUEPOS)
    iflag |= (SEFLG_NOGDEFL | SEFLG_NOABERR);
  if (iflag & SEFLG_MOSEPH)
    epheflag = SEFLG_MOSEPH;
  if (iflag & SEFLG_SWIEPH)
    epheflag = SEFLG_SWIEPH;
  if (iflag & SEFLG_JPLEPH)
    epheflag = SEFLG_JPLEPH;
  if (epheflag == 0)
    epheflag = SEFLG_DEFAULTEPH;
  iflag = (iflag & ~SEFLG_EPHMASK) | epheflag;
  /* SEFLG_JPLHOR only with JPL and Swiss Ephemeeris */
  if (!(epheflag & SEFLG_JPLEPH)) 
    iflag = iflag & ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
  /* planets that have no JPL Horizons mode */
  if (ipl == SE_OSCU_APOG || ipl == SE_TRUE_NODE 
      || ipl == SE_MEAN_APOG || ipl == SE_MEAN_NODE
      || ipl == SE_INTP_APOG || ipl == SE_INTP_PERG) 
    iflag = iflag & ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
  if (ipl >= SE_FICT_OFFSET && ipl <= SE_FICT_MAX)
    iflag = iflag & ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
  /* SEFLG_JPLHOR requires SEFLG_ICRS, if calculated with * precession/nutation IAU 1980 and corrections dpsi, deps */
  if (iflag & SEFLG_JPLHOR) {
    if (ctx->eop_dpsi_loaded <= 0) {
      if (serr != NULL) {
	switch (ctx->eop_dpsi_loaded) {
	  case 0:
	    strcpy(serr, "you did not call swe_set_jpl_file(); default to SEFLG_JPLHOR_APPROX");
	    break;
	  case -1:
	    strcpy(serr, "file eop_1962_today.txt not found; default to SEFLG_JPLHOR_APPROX");
	    break;
	  case -2:
	    strcpy(serr, "file eop_1962_today.txt corrupt; default to SEFLG_JPLHOR_APPROX");
	    break;
	  case -3:
	    strcpy(serr, "file eop_finals.txt corrupt; default to SEFLG_JPLHOR_APPROX");
	    break;
	}
      }
      iflag &= ~SEFLG_JPLHOR;
      iflag |= SEFLG_JPLHOR_APPROX;
    }
  }
  if (iflag & SEFLG_JPLHOR) 
    iflag |= SEFLG_ICRS;
  if ((iflag & SEFLG_JPLHOR_APPROX) && jplhora_model == SEMOD_JPLHORA_2)
    iflag |= SEFLG_ICRS;
  return iflag;
}

/* function formats the input search name of a star:
 * - remove white spaces
 * - traditional name to lower case (Bayer designation remains as it is)
 */
static int32 fixstar_format_search_name(char *star, char *sstar, char *serr)
{
  char *sp;
  size_t cmplen;
  strncpy(sstar, star, SWI_STAR_LENGTH);
  sstar[SWI_STAR_LENGTH] = '\0';
  // remove whitespaces from search name
  while ((sp = strchr(sstar, ' ')) != NULL)
    swi_strcpy(sp, sp+1);
  /* traditional name of star to lower case;
   * keep uppercase with Bayer/Flamsteed designations after comma */
  for (sp = sstar; *sp != '\0' && *sp != ','; sp++) 
    *sp = tolower((int) *sp);
  cmplen = strlen(sstar);
  if (cmplen == 0) {
    if (serr != NULL)
      sprintf(serr, "swe_fixstar(): star name empty");
    return ERR; 
  }
  return OK;
}

/* function saves a fixstar in fixed stars list
 */
static int32 save_star_in_struct(swe_ctx *ctx, int nrecs, struct fixed_star *fstp, char *serr)
{
  int sizestru = sizeof(struct fixed_star);
  struct fixed_star *ftarget;
  char *serr_alloc = "error in function load_all_fixed_stars(ctx): could not resize fixed stars array";
  if ((ctx->fixed_stars = (struct fixed_star *) realloc(ctx->fixed_stars, nrecs * sizestru)) == NULL) {
    if (serr != NULL) strcpy(serr, serr_alloc);
    return ERR;
  }
  ftarget = ctx->fixed_stars + (nrecs - 1);
  memcpy((void *) ftarget, (void *) fstp, sizestru);
  return OK;
}

/* function for sorting fixed stars with qsort() */
static int CMP_CALL_CONV fixedstar_name_compare(const void *star1, const void *star2)
{
  const struct fixed_star *fs1 = (const struct fixed_star *) star1;
  const struct fixed_star *fs2 = (const struct fixed_star *) star2;
  return strcmp(fs1->skey, fs2->skey);
}

/* help function for finding a fixed star with bsearch() */
static int CMP_CALL_CONV fstar_node_compare(const void *node1, const void *node2)
{
  const struct fixed_star *n1 = (const struct fixed_star *) node1;
  const struct fixed_star *n2 = (const struct fixed_star *) node2;
  return strcmp(n1->skey, n2->skey);
}

/* function cuts a comma-separated fixed star data record from sefstars.txt 
 * and fills it into a struct fixed_star.
 */
int32 fixstar_cut_string(swe_ctx *ctx, char *srecord, char *star, struct fixed_star *stardata, char *serr)
{
  int i;
  char s[AS_MAXCH + 20];
  char *sde_d;
  char *cpos[20];
  double epoch, radv, parall, mag;
  double ra_s, ra_pm, de_pm, ra, de;
  double ra_h, ra_m, de_d, de_m, de_s;
  strcpy(s, srecord);
  i = swi_cutstr(s, ",", cpos, 20);
  /* return trad. name, nomeclature name */
  swi_right_trim(cpos[0]);
  swi_right_trim(cpos[1]);
  if (i < 14) {
    if (serr != NULL) {
      if (i >= 2) {
	sprintf(serr, "data of star '%s,%s' incomplete", cpos[0], cpos[1]);
      } else {
        if (strlen(s) > 200) s[200] = '\0';
	sprintf(serr, "invalid line in fixed stars file: '%s'", s);
      }
    }
    return ERR;
  }
  if (strlen(cpos[0]) > SWI_STAR_LENGTH)
    cpos[0][SWI_STAR_LENGTH] = '\0';
  if (strlen(cpos[1]) > SWI_STAR_LENGTH-1)
    cpos[1][SWI_STAR_LENGTH-1] = '\0';
  if (star != NULL) {
    strcpy(star, cpos[0]);
    if (strlen(cpos[0]) + strlen(cpos[1]) + 1 < SWI_STAR_LENGTH - 1)
      sprintf(star + strlen(star), ",%s", cpos[1]);
  }
  strcpy(stardata->starname, cpos[0]);
  strcpy(stardata->starbayer, cpos[1]);
  // star data
  epoch = atof(cpos[2]);
  ra_h = atof(cpos[3]);
  ra_m = atof(cpos[4]);
  ra_s = atof(cpos[5]);
  de_d = atof(cpos[6]);
  sde_d = cpos[6];
  de_m = atof(cpos[7]);
  de_s = atof(cpos[8]);
  ra_pm = atof(cpos[9]);
  de_pm = atof(cpos[10]);
  radv = atof(cpos[11]);
  parall = atof(cpos[12]);
  if (parall < 0) parall = -parall;	// to fix bug like old Rasalgheti
  mag = atof(cpos[13]);
  /****************************************
   * position and speed (equinox)
   ****************************************/
  /* ra and de in degrees */
  ra = (ra_s / 3600.0 + ra_m / 60.0 + ra_h) * 15.0;
  if (strchr(sde_d, '-') == NULL) {
    de = de_s / 3600.0 + de_m / 60.0 + de_d;
  } else {
    de = -de_s / 3600.0 - de_m / 60.0 + de_d;
  }
  /* speed in ra and de, degrees per century */
  if (ctx->is_old_starfile == TRUE) {
    ra_pm = ra_pm * 15 / 3600.0;
    de_pm = de_pm / 3600.0;
  } else {
    ra_pm = ra_pm / 10.0 / 3600.0;
    de_pm = de_pm / 10.0 / 3600.0;
    parall /= 1000.0;
  }
  /* parallax, degrees */
  if (parall > 1) {
    parall = (1 / parall / 3600.0);
  } else {
    parall /= 3600;
  }
  /* radial velocity in AU per century */
  radv *= KM_S_TO_AU_CTY;
  /*printf("ra=%.17f,de=%.17f,ma=%.17f,md=%.17f,pa=%.17f,rv=%.17f\n",ra,de,ra_pm,de_pm,parall,radv);*/
  /* radians */
  ra *= DEGTORAD;
  de *= DEGTORAD;
  ra_pm *= DEGTORAD;
  de_pm *= DEGTORAD;
  ra_pm /= cos(de); /* catalogues give proper motion in RA as great circle */
  parall *= DEGTORAD;
  stardata->epoch = epoch;
  stardata->ra = ra;
  stardata->de = de;
  stardata->ramot = ra_pm;
  stardata->demot = de_pm;
  stardata->parall = parall;
  stardata->radvel = radv;
  stardata->mag = mag;
  return OK;
}

/* function loads all fixed stars from file sefstars.txt,
 * into swed.fixed_stars, which is a pointer to an array
 * of struct fixed_stars.
 * Every star has a record with its Bayer/Flamsteed designation 
 * as its search key.
 * Every star also has a record with its sequential number in
 * the file as its search key. (Good for calculating all stars in a loop.)
 * If a star has a traditional name, we create a record that has 
 * this name as its search key.
 * The array is sorted in ascending order by search key. 
 *
 * If an error occurs, the function returns value ERR.
 * If the stars were loaded at an earlier time the function returns
 * value -2, without doing anything and without error string.
 * On success, the function returns value OK.
 * */
static int32 load_all_fixed_stars(swe_ctx *ctx, char *serr) 
{
  int32 retc = OK;
  int nstars = 0, nrecs = 0, nnamed = 0;
  char s[AS_MAXCH], *sp;
  char srecord[AS_MAXCH];
  struct fixed_star fstdata;
  char last_starbayer[SWI_STAR_LENGTH + 1];
  *last_starbayer = '\0';
  if (ctx->n_fixstars_records > 0) {
    return -2;
  }
  if (ctx->fixfp == NULL) {
    if ((ctx->fixfp = swi_fopen(ctx, SEI_FILE_FIXSTAR, SE_STARFILE, ctx->ephepath, serr)) == NULL) {
      ctx->is_old_starfile = TRUE;
      if ((ctx->fixfp = swi_fopen(ctx, SEI_FILE_FIXSTAR, SE_STARFILE_OLD, ctx->ephepath, NULL)) == NULL) {
	ctx->is_old_starfile = FALSE;
	/* no fixed star file available, error message is already in serr. */
	return ERR;
      }
    }
  }
  rewind(ctx->fixfp);
  ctx->fixed_stars = NULL;
  while (fgets(s, AS_MAXCH, ctx->fixfp) != NULL) {
    // skip comment lines
    if (*s == '#') continue;
    if (*s == '\n') continue;
    if (*s == '\r') continue;
    if (*s == '\0') continue;
    strcpy(srecord, s);
    retc = fixstar_cut_string(ctx, srecord, NULL, &fstdata, serr);
    if (retc == ERR) return ERR;
    // if star has a traditional name, save it with that name as its search key
    if (*fstdata.starname != '\0') {
      nrecs++;
      nnamed++;
      strcpy(fstdata.skey, fstdata.starname);
      // remove white spaces from star name
      while ((sp = strchr(fstdata.skey, ' ')) != NULL)
	swi_strcpy(sp, sp+1);
      // star name to lowercase and compare with search string
      for (sp = fstdata.skey; *sp != '\0'; sp++) 
	*sp = tolower((int) *sp);
      if ((retc = save_star_in_struct(ctx, nrecs, &fstdata, serr)) == ERR) return ERR;
    }
    // also save it with Bayer designation as search key;
    // only if it has not been saved already
    if (strcmp(fstdata.starbayer, last_starbayer) == 0)
      continue;
    nstars++;
    nrecs++;
    //sprintf(fstdata.skey, "~%s", fstdata.starbayer); // ~ sorts after alnum
    sprintf(fstdata.skey, ",%s", fstdata.starbayer); // , sorts before alnum
    // remove white spaces from star bayer name
    while ((sp = strchr(fstdata.skey, ' ')) != NULL)
      swi_strcpy(sp, sp+1);
    strcpy(last_starbayer, fstdata.starbayer);
    if ((retc = save_star_in_struct(ctx, nrecs, &fstdata, serr)) == ERR) return ERR;
    // also save it with sequential star number as search key (NO!!!!)
    // nrecs++;
    // sprintf(fstdata.skey, "%07d", nstars);
    // if ((retc = save_star_in_struct(ctx, nrecs, &fstdata, serr)) == ERR) return ERR;
  }
  ctx->n_fixstars_real = nstars;
  ctx->n_fixstars_named = nnamed;
  ctx->n_fixstars_records = nrecs;
  // fprintf(stderr, "nstars=%d, nrecords=%d\n", nstars, nrecs);
  (void) qsort ((void *) ctx->fixed_stars, (size_t) nrecs, sizeof (struct fixed_star),
                    (int (CMP_CALL_CONV *)(const void *,const void *))(fixedstar_name_compare));
  return retc;
}

/* function calculates a fixstar from a star data struct 
 * input:
 * struct fixed_star stardata      fixed star data struct
 * double tjd        julian daynumber 
 * int32 iflag       SEFLG_ specifications
 * output:
 * char *star        star name, Bayer designation
 * double xx[6]      position and speed
 * char *serr        error return string
 */
static int32 fixstar_calc_from_struct(swe_ctx *ctx, struct fixed_star *stardata, double tjd, int32 iflag, char *star, double *xx, char *serr)
{
  int i;
  int32 retc = OK;
  double epoch, radv, parall;
  double ra_pm, de_pm, ra, de, t;
  double daya[2], rdist;
  double x[6], xxsv[6], xobs[6], xobs_dt[6], *xpo = NULL, *xpo_dt = NULL;
  /* moved to ctx->sp.fx_struct (Phase 3c) */
  double dt = PLAN_SPEED_INTV * 0.1;
  int32 epheflag, iflgsave;
  struct epsilon *oe = &ctx->oec2000;
  iflgsave = iflag;
  iflag |= SEFLG_SPEED; /* we need this in order to work correctly */
  if (serr != NULL)
    *serr = '\0';
  iflag = plaus_iflag(ctx, iflag, -1, tjd, serr);
  epheflag = iflag & SEFLG_EPHMASK;
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL) {
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_fixstar() or swe_fixstar_ut()");
  }
  if (ctx->last_epheflag != epheflag) {
    free_planets(ctx);
    /* close and free ephemeris files */
    if (ctx->jpl_file_is_open) {
      swi_close_jpl_file(ctx);
      ctx->jpl_file_is_open = FALSE;
    }
    for (i = 0; i < SEI_NEPHFILES; i ++) {
      if (ctx->fidat[i].fptr != NULL) 
	fclose(ctx->fidat[i].fptr);
      memset((void *) &ctx->fidat[i], 0, sizeof(struct file_data));
    }
    ctx->last_epheflag = epheflag;
  }
  /* high precision speed prevails fast speed */
  /* JPL Horizons is only reproduced with SEFLG_JPLEPH */
  if (iflag & SEFLG_SIDEREAL && !ctx->ayana_is_set)
    SWI_CFG_LOCAL(ctx, swe_set_sid_mode_r(ctx, SE_SIDM_FAGAN_BRADLEY, 0, 0));
  /****************************************** 
   * obliquity of ecliptic 2000 and of date * 
   ******************************************/
  swi_check_ecliptic(ctx, tjd, iflag);
  /******************************************
   * nutation                               * 
   ******************************************/
  swi_check_nutation(ctx, tjd, iflag);
  sprintf(star, "%s,%s", stardata->starname, stardata->starbayer);
  epoch = stardata->epoch;
  ra_pm = stardata->ramot; de_pm = stardata->demot;
  radv = stardata->radvel; parall = stardata->parall; 
  ra = stardata->ra; de = stardata->de;
  if (epoch == 1950) {
    t= (tjd - B1950);	/* days since 1950.0 */
  } else { /* epoch == 2000 */
    t= (tjd - J2000);	/* days since 2000.0 */
  }
  x[0] = ra;
  x[1] = de;
  x[2] = 1;	
  if (parall == 0) {
    rdist = 1000000000;  
  } else {
    rdist = 1.0 / (parall * RADTODEG * 3600) * PARSEC_TO_AUNIT;	
    //rdist += t * radv / 36525.0;
  }
// rdist = 10000;  // to reproduce pre-SE2.07 star positions
  x[2] = rdist;
  x[3] = ra_pm / 36525.0;
  x[4] = de_pm / 36525.0;
  x[5] = radv / 36525.0;
  // Cartesian space motion vector
  swi_polcart_sp(x, x);
  /******************************************
   * FK5
   ******************************************/
  if (epoch == 1950) {
    swi_FK4_FK5(x, B1950);
    swi_precess(ctx, x, B1950, 0, J_TO_J2000);
    swi_precess(ctx, x+3, B1950, 0, J_TO_J2000);
  } 
  /* FK5 to ICRF, if jpl ephemeris is referred to ICRF.
   * With data that are already ICRF, epoch = 0 */
  if (epoch != 0) {
    swi_icrs2fk5(x, iflag, TRUE); /* backward, i. e. to icrf */
    /* with ephemerides < DE403, we now convert to J2000 */
    if (swi_get_denum(ctx, SEI_SUN, iflag) >= 403) {
      swi_bias(ctx, x, J2000, SEFLG_SPEED, FALSE);
    }
  }
  /**************************************************** 
   * earth/sun 
   * for parallax, light deflection, and aberration,
   ****************************************************/
  if (!(iflag & SEFLG_BARYCTR) && (!(iflag & SEFLG_HELCTR) || !(iflag & SEFLG_MOSEPH))) {
    if ((retc =  main_planet_bary(ctx, tjd - dt, SEI_EARTH, epheflag, iflag, NO_SAVE, ctx->sp.fx_struct.xearth_dt, ctx->sp.fx_struct.xearth_dt, ctx->sp.fx_struct.xsun_dt, NULL, serr)) != OK) {
      return ERR;
    }
    if ((retc =  main_planet_bary(ctx, tjd, SEI_EARTH, epheflag, iflag, DO_SAVE, ctx->sp.fx_struct.xearth, ctx->sp.fx_struct.xearth, ctx->sp.fx_struct.xsun, NULL, serr)) != OK) {
      return ERR;
    }
  }
  /************************************
   * observer: geocenter or topocenter
   ************************************/
  /* if topocentric position is wanted  */
  if (iflag & SEFLG_TOPOCTR) { 
    if (swi_get_observer(ctx, tjd - dt, iflag | SEFLG_NONUT, NO_SAVE, xobs_dt, serr) != OK)
      return ERR;
    if (swi_get_observer(ctx, tjd, iflag | SEFLG_NONUT, NO_SAVE, xobs, serr) != OK)
      return ERR;
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++) {
      xobs[i] = xobs[i] + ctx->sp.fx_struct.xearth[i];	
      xobs_dt[i] = xobs_dt[i] + ctx->sp.fx_struct.xearth_dt[i];	
    }
  } else if (!(iflag & SEFLG_BARYCTR) && (!(iflag & SEFLG_HELCTR) || !(iflag & SEFLG_MOSEPH))) {
    /* barycentric position of geocenter */
    for (i = 0; i <= 5; i++) {
      xobs[i] = ctx->sp.fx_struct.xearth[i];
      xobs_dt[i] = ctx->sp.fx_struct.xearth_dt[i];
    }
  }
  /************************************
   * position and speed at tjd        *
   ************************************/
  /* for parallax */ 
  if ((iflag & SEFLG_HELCTR) && (iflag & SEFLG_MOSEPH)) {
    xpo = NULL;		/* no parallax, if moshier and heliocentric */
    xpo_dt = NULL;	/* no parallax, if moshier and heliocentric */
  } else if (iflag & SEFLG_HELCTR) {
    xpo = ctx->sp.fx_struct.xsun;//psdp->x;
    xpo_dt = ctx->sp.fx_struct.xsun_dt; 
  } else if (iflag & SEFLG_BARYCTR) {
    xpo = NULL;		/* no parallax, if barycentric */
    xpo_dt = NULL;	/* no parallax, if moshier and heliocentric */
  } else {
    xpo = xobs;
    xpo_dt = xobs_dt;
  }
  if (xpo == NULL) {
    for (i = 0; i <= 2; i++) {
      x[i] += t * x[i+3];	
    }
  } else {
    for (i = 0; i <= 2; i++) {
      x[i] += t * x[i+3];
      x[i] -= xpo[i];
      x[i+3] -= xpo[i+3];
    }
  }
  /************************************
   * relativistic deflection of light *
   ************************************/
  if ((iflag & SEFLG_TRUEPOS) == 0 && (iflag & SEFLG_NOGDEFL) == 0) {
    swi_deflect_light(ctx, x, 0, iflag & SEFLG_SPEED);
  }
  /**********************************
   * 'annual' aberration of light   *
   * speed is incorrect !!!         *
   **********************************/
  if ((iflag & SEFLG_TRUEPOS) == 0 && (iflag & SEFLG_NOABERR) == 0)
    swi_aberr_light_ex(x, xpo, xpo_dt, dt, iflag & SEFLG_SPEED);
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && (swi_get_denum(ctx, SEI_SUN, iflag) >= 403 || (iflag & SEFLG_BARYCTR))) {
    swi_bias(ctx, x, tjd, iflag, FALSE);
  }/**/
  /* save J2000 coordinates; required for sidereal positions */
  for (i = 0; i <= 5; i++)
    xxsv[i] = x[i];
  /************************************************
   * precession, equator 2000 -> equator of date *
   ************************************************/
  /*x[0] = -0.374018403; x[1] = -0.312548592; x[2] = -0.873168719;*/
  if ((iflag & SEFLG_J2000) == 0) {
    swi_precess(ctx, x, tjd, iflag, J2000_TO_J);
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, x, tjd, iflag, J2000_TO_J);
    oe = &ctx->oec;
  } else
    oe = &ctx->oec2000;
  /************************************************
   * nutation                                     *
   ************************************************/
  if (!(iflag & SEFLG_NONUT))
    swi_nutate(ctx, x, iflag, FALSE);
  /************************************************
   * transformation to ecliptic.                  *
   * with sidereal calc. this will be overwritten *
   * afterwards.                                  *
   ************************************************/
  if ((iflag & SEFLG_EQUATORIAL) == 0) {
    swi_coortrf2(x, x, oe->seps, oe->ceps);
    if (iflag & SEFLG_SPEED)
      swi_coortrf2(x+3, x+3, oe->seps, oe->ceps);
    if (!(iflag & SEFLG_NONUT)) {
      swi_coortrf2(x, x, ctx->nut.snut, ctx->nut.cnut);
      if (iflag & SEFLG_SPEED)
	swi_coortrf2(x+3, x+3, ctx->nut.snut, ctx->nut.cnut);
    }
  }
//  printf("%.17f, %.17f\n", x[0], x[3]);
  /************************************
   * sidereal positions               *
   ************************************/
  if (iflag & SEFLG_SIDEREAL) {
    /* rigorous algorithm */
    if (ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) {
      if (swi_trop_ra2sid_lon(ctx, xxsv, x, xxsv, iflag) != OK)
        return ERR;
      if (iflag & SEFLG_EQUATORIAL) {
        for (i = 0; i <= 5; i++)
          x[i] = xxsv[i];
      }
    /* project onto solar system equator */
    } else if (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE) {
      if (swi_trop_ra2sid_lon_sosy(ctx, xxsv, x, iflag) != OK)
	return ERR;
      if (iflag & SEFLG_EQUATORIAL) {
        for (i = 0; i <= 5; i++)
          x[i] = xxsv[i];
      }
    /* traditional algorithm */
    } else {
      swi_cartpol_sp(x, x); 
      // ACHTUNG: siehe Z. 2770!!!!!
      if (swi_get_ayanamsa_with_speed(ctx, tjd, iflag, daya, serr) == ERR)
        return ERR;
      x[0] -= daya[0] * DEGTORAD;
      x[3] -= daya[1] * DEGTORAD;
      swi_polcart_sp(x, x); 
    }
  } 
  /************************************************
   * transformation to polar coordinates          *
   ************************************************/
  if ((iflag & SEFLG_XYZ) == 0)
    swi_cartpol_sp(x, x); 
  /********************** 
   * radians to degrees *
   **********************/
  if ((iflag & SEFLG_RADIANS) == 0 && (iflag & SEFLG_XYZ) == 0) {
    for (i = 0; i < 2; i++) {
      x[i] *= RADTODEG;
      x[i+3] *= RADTODEG;
    }
  }
  for (i = 0; i <= 5; i++)
    xx[i] = x[i];
  if (!(iflgsave & SEFLG_SPEED)) {
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  }
  /* if no ephemeris has been specified, do not return chosen ephemeris */
  if ((iflgsave & SEFLG_EPHMASK) == 0)
    iflag = iflag & ~SEFLG_DEFAULTEPH;
  iflag = iflag & ~SEFLG_SPEED;
  return iflag;
}

/* function searches a star in fixed stars list, i.e. the data loaded from file 
 * sefstars.txt
 */
static int32 search_star_in_list(swe_ctx *ctx, char *sstar, struct fixed_star *stardata, char *serr)
{
  int i, star_nr = 0, ndata = 0, len;
  char *sp;
  char searchkey[AS_MAXCH];
  AS_BOOL is_bayer = FALSE;
  struct fixed_star *stardatap;
  struct fixed_star *stardatabegp;
  if (*sstar == ',') {
    is_bayer = TRUE;
  } else if (isdigit((int) *sstar)) {
    star_nr = atoi(sstar);
  } else {
    if ((sp = strchr(sstar, ',')) != NULL) {
      swi_strcpy(sstar, sp);
      is_bayer = TRUE;
    }
  }
  if (star_nr > 0) {
    if (star_nr > ctx->n_fixstars_real) {
      if (serr != NULL) 
	sprintf(serr, "error, swe_fixstar(): sequential fixed star number %d is not available", star_nr);
      return ERR;
    }
    *stardata = ctx->fixed_stars[star_nr - 1]; // keys start from 1
    //printf("seq.number: %s, %s, %s, %f\n", stardata.skey, stardata.starname, stardata.starbayer, stardata.mag);
    return OK;
  /* traditional name with wildcard '%' at end of string */
  } else if (!is_bayer && (sp = strchr(sstar, '%')) != NULL) {
    stardatabegp = &(ctx->fixed_stars[ctx->n_fixstars_real]);
    ndata = ctx->n_fixstars_named;
    if ((size_t) (sp - sstar) != strlen(sstar) - 1) {	/* sp >= sstar: strchr() found sp within sstar */
      if (serr != NULL)
	sprintf(serr, "error, swe_fixstar(): invalid search string %s", sstar);
      return ERR;
    }
    strcpy(searchkey, sstar);
    len = (int) (strlen(sstar) - 1);
    searchkey[len] = '\0';
    for (i = 0; i < ndata; i++) {
      if (strncmp(stardatabegp[i].skey, sstar, len) == 0) {
        *stardata = stardatabegp[i];
	return OK;
      }
    }
    if (serr != NULL)
      sprintf(serr, "error, swe_fixstar(): star search string %s did not match", sstar);
    return ERR;
  /* traditional name or Bayer/Flamsteed: find it with binary search */
  } else {
    strcpy(searchkey, sstar);
    if (is_bayer) {
      //*searchkey = '~';
      //stardatabegp = &(swed.fixed_stars[swed.n_fixstars_real + swed.n_fixstars_named]);
      //stardatabegp = &(swed.fixed_stars[0]);
      stardatabegp = ctx->fixed_stars;
      ndata = ctx->n_fixstars_real;
    } else {
      stardatabegp = &(ctx->fixed_stars[ctx->n_fixstars_real]);
      ndata = ctx->n_fixstars_named;
    }
    stardatap = (struct fixed_star *) bsearch((void *) searchkey,
	       (void *) stardatabegp, (size_t) ndata,
	       sizeof (struct fixed_star),
	       fstar_node_compare);
    if (stardatap == NULL) {
      if (serr != NULL) 
	sprintf(serr, "error, swe_fixstar(): could not find star name %s", sstar);
      return ERR;
    }
    *stardata = *stardatap;
    //printf("name search: %s, %s, %s, %f\n", stardata.skey, stardata.starname, stardata.starbayer, stardata.mag);
    return OK;
  }
}

static AS_BOOL get_builtin_star(char *star, char *sstar, char *srecord)
{
  /* some stars are built-in, because they are required for Hindu
   * sidereal ephemerides */
  /* Ayanamsha SE_SIDM_TRUE_CITRA */
  if (strncmp(star, "spica", 5) == 0 || strncmp(star, "Spica", 5) == 0) {
    strcpy(srecord, "Spica,alVir,ICRS,13,25,11.57937,-11,09,40.7501,-42.35,-30.67,1,13.06,0.97,-10,3672");
    strcpy(sstar, "spica");
    return TRUE;
  /* Ayanamsha SE_SIDM_TRUE_REVATI */
  } else if (strstr(star, ",zePsc") != NULL || strncmp(star, "revati", 6) == 0 || strncmp(star, "Revati", 6) == 0) {
    strcpy(srecord, "Revati,zePsc,ICRS,01,13,43.88735,+07,34,31.2745,145,-55.69,15,18.76,5.187,06,174");
    strcpy(sstar, "revati");
    return TRUE;
  /* Ayanamsha SE_SIDM_TRUE_PUSHYA */
  } else if (strstr(star, ",deCnc") != NULL || strncmp(star, "pushya", 6) == 0 || strncmp(star, "Pushya", 6) == 0 ) {
    strcpy(srecord, "Pushya,deCnc,ICRS,08,44,41.09921,+18,09,15.5034,-17.67,-229.26,17.14,24.98,3.94,18,2027");
    strcpy(sstar, "pushya");
    return TRUE;
  /* Ayanamsha SE_SIDM_TRUE_SHEORAN */
  } else if (strstr(star, ",deCnc") != NULL) {
    strcpy(srecord, "Pushya,deCnc,ICRS,08,44,41.09921,+18,09,15.5034,-17.67,-229.26,17.14,24.98,3.94,18,2027");
    strcpy(sstar, "pushya");
    return TRUE;
  /* Ayanamsha SE_SIDM_TRUE_MULA */
  } else if (strstr(star, ",laSco") != NULL || strncmp(star, "mula", 6) == 0 || strncmp(star, "Mula", 6) == 0) {
    strcpy(srecord, "Mula,laSco,ICRS,17,33,36.52012,-37,06,13.7648,-8.53,-30.8,-3,5.71,1.62,-37,11673");
    strcpy(sstar, "mula");
    return TRUE;
  /* Ayanamsha SE_SIDM_GALCENT_0SAG */
  /* Ayanamsha SE_SIDM_GALCENT_COCHRANE */
  /* Ayanamsha SE_SIDM_GALCENT_RGILBRAND */
  } else if (strstr(star, ",SgrA*") != NULL) {
    strcpy(srecord, "Gal. Center,SgrA*,2000,17,45,40.03599,-29,00,28.1699,-2.755718425,-5.547,0.0,0.125,999.99,0,0");
    strcpy(sstar, ",SgrA*");
    return TRUE;
  /* Ayanamsha SE_SIDM_GALEQU_IAU1958 */
  } else if (strstr(star, ",GP1958") != NULL) {
    strcpy(srecord, "Gal. Pole IAU1958,GP1958,1950,12,49,0.0,27,24,0.0,0.0,0.0,0.0,0.0,0.0,0,0");
    strcpy(sstar, ",GP1958");
    return TRUE;
  /* Ayanamsha SE_SIDM_GALEQU_TRUE */
  } else if (strstr(star, ",GPol") != NULL) {
    strcpy(srecord, "Gal. Pole,GPol,ICRS,12,51,36.7151981,27,06,11.193172,0.0,0.0,0.0,0.0,0.0,0,0");
    strcpy(sstar, ",GPol");
    return TRUE;
  /* Ayanamsha SE_SIDM_GALEQU_MULA */
  } else if (strstr(star, ",GPol") != NULL) {
    strcpy(srecord, "Gal. Pole,GPol,ICRS,12,51,36.7151981,27,06,11.193172,0.0,0.0,0.0,0.0,0.0,0,0");
    strcpy(sstar, ",GPol");
    return TRUE;
  }
  return FALSE;
}

/**********************************************************
 * function gets fixstar positions
 * parameters:
 * star 	name of star or line number in star file 
 *		(start from 1, don't count comment).
 *    		If no error occurs, the name of the star is returned
 *	        in the format trad_name, nomeclat_name
 *
 * tjd 		absolute julian day
 * iflag	s. swecalc(ctx); speed bit does not function
 * x		pointer to 6 doubles for returning position coordinates
 * serr		error return string
**********************************************************/
int32 CALL_CONV swe_fixstar2_r(swe_ctx *ctx, char *star, double tjd, int32 iflag, 
  double *xx, char *serr)
{
  int i;
  AS_BOOL is_builtin_star = FALSE;
  char sstar[SWI_STAR_LENGTH + 1];
  //static TLS char slast_stardata[AS_MAXCH];
  /* moved to ctx->sp.fs2 (Phase 3c) */
  /* moved to ctx->sp.fs2 (Phase 3c) */
  char srecord[AS_MAXCH + 20];	/* 20 byte for SE_STARFILE */
  int retc;
  struct fixed_star stardata;
  if (serr != NULL)
    *serr = '\0';
#ifdef TRACE
  swi_open_trace(serr);
  trace_swe_fixstar(1, star, tjd, iflag, xx, serr);
#endif /* TRACE */
  load_all_fixed_stars(ctx, serr); // loads stars unless loaded with an earlier call of function
  retc = fixstar_format_search_name(star, sstar, serr);
  if (retc == ERR)
    goto return_err;
  /* star elements from last call: */
  if (ctx->n_fixstars_records > 0 && strcmp(ctx->sp.fs2.slast_starname, sstar) == 0) {
 //   strcpy(srecord, slast_stardata);
    stardata = ctx->sp.fs2.last_stardata;
    goto found;
  }
  if (get_builtin_star(star, sstar, srecord)) {
    is_builtin_star = TRUE;
  }
  if (is_builtin_star) {
    retc = fixstar_cut_string(ctx, srecord, star, &stardata, serr);
    //printf("builtin: %s, %s, %s, %f\n", stardata.skey, stardata.starname, stardata.starbayer, stardata.mag);
    goto found;
  /* sequential fixed star number: get it from array directly */
  } 
  retc = search_star_in_list(ctx, sstar, &stardata, serr);
  if (retc == ERR)
    goto return_err;
  /******************************************************/
  found:
  //strcpy(slast_stardata, srecord);
  ctx->sp.fs2.last_stardata = stardata;
  strcpy(ctx->sp.fs2.slast_starname, sstar);
  if ((retc = fixstar_calc_from_struct(ctx, &stardata, tjd, iflag, star, xx, serr)) == ERR)
    goto return_err;
#ifdef TRACE
  trace_swe_fixstar(2, star, tjd, iflag, xx, serr);
#endif
  return iflag;
  return_err:
  for (i = 0; i <= 5; i++)
    xx[i] = 0;
#ifdef TRACE
  trace_swe_fixstar(2, star, tjd, iflag, xx, serr);
#endif
  return retc;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar2(char *star, double tjd, int32 iflag, 
  double *xx, char *serr)
{
  return swe_fixstar2_r(swi_default_ctx(), star, tjd, iflag, xx, serr);
}

int32 CALL_CONV swe_fixstar2_ut_r(swe_ctx *ctx, char *star, double tjd_ut, int32 iflag, 
  double *xx, char *serr)
{
  double deltat;
  int32 retflag;
  int32 epheflag = 0;
  iflag = plaus_iflag(ctx, iflag, -1, tjd_ut, serr);
  epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag == 0) {
    epheflag = SEFLG_SWIEPH;
    iflag |= SEFLG_SWIEPH;
  }
  deltat = swe_deltat_ex_r(ctx, tjd_ut, iflag, serr);
  /* if ephe required is not ephe returned, adjust delta t: */
  retflag = swe_fixstar2_r(ctx, star, tjd_ut + deltat, iflag, xx, serr);
  if (retflag != ERR && (retflag & SEFLG_EPHMASK) != epheflag) {
    deltat = swe_deltat_ex_r(ctx, tjd_ut, retflag, NULL);
    retflag = swe_fixstar2_r(ctx, star, tjd_ut + deltat, iflag, xx, NULL);
  }
  return retflag;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar2_ut(char *star, double tjd_ut, int32 iflag, 
  double *xx, char *serr)
{
  return swe_fixstar2_ut_r(swi_default_ctx(), star, tjd_ut, iflag, xx, serr);
}

/**********************************************************
 * get fixstar magnitude
 * parameters:
 * star 	name of star or line number in star file 
 *		(start from 1, don't count comment).
 *    		If no error occurs, the name of the star is returned
 *	        in the format trad_name, nomeclat_name
 *
 * mag 		pointer to a double, for star magnitude
 * serr		error return string
**********************************************************/
int32 CALL_CONV swe_fixstar2_mag_r(swe_ctx *ctx, char *star, double *mag, char *serr)
{
  char sstar[SWI_STAR_LENGTH + 1];
  //static TLS char slast_stardata[AS_MAXCH];
  /* moved to ctx->sp.fs2mag (Phase 3c) */
  /* moved to ctx->sp.fs2mag (Phase 3c) */
  int retc;
  struct fixed_star stardata;
  if (serr != NULL)
    *serr = '\0';
  load_all_fixed_stars(ctx, serr); // loads stars unless loaded with an earlier call of function
  retc = fixstar_format_search_name(star, sstar, serr);
  if (retc == ERR)
    goto return_err;
  /* star elements from last call: */
  if (ctx->n_fixstars_records > 0 && strcmp(ctx->sp.fs2mag.slast_starname, sstar) == 0) {
 //   strcpy(srecord, slast_stardata);
    stardata = ctx->sp.fs2mag.last_stardata;
    goto found;
  }
  retc = search_star_in_list(ctx, sstar, &stardata, serr);
  if (retc == ERR)
    goto return_err;
  /******************************************************/
  found:
  ctx->sp.fs2mag.last_stardata = stardata;
  strcpy(ctx->sp.fs2mag.slast_starname, sstar);
  *mag = stardata.mag;
  sprintf(star, "%s,%s", stardata.starname, stardata.starbayer);
  return OK;
  return_err:
  *mag = 0;
  return retc;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar2_mag(char *star, double *mag, char *serr)
{
  return swe_fixstar2_mag_r(swi_default_ctx(), star, mag, serr);
}

char * CALL_CONV swe_get_planet_name_r(swe_ctx *ctx, int ipl, char *s)
{
  int i;
  int32 retc;
  double xp[6];
#ifdef TRACE
  swi_open_trace(NULL);
  trace_swe_get_planet_name(1, ipl, s);
#endif
  swi_init_swed_if_start(ctx);
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO */
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  if (ipl != 0 && ipl == ctx->i_saved_planet_name) {
    strcpy(s, ctx->saved_planet_name);
    return s;
  }
  switch(ipl) {
    case SE_SUN: 
      strcpy(s, SE_NAME_SUN);
      break;
    case SE_MOON: 
      strcpy(s, SE_NAME_MOON);
      break;
    case SE_MERCURY: 
      strcpy(s, SE_NAME_MERCURY);
      break;
    case SE_VENUS: 
      strcpy(s, SE_NAME_VENUS);
      break;
    case SE_MARS: 
      strcpy(s, SE_NAME_MARS);
      break;
    case SE_JUPITER: 
      strcpy(s, SE_NAME_JUPITER);
      break;
    case SE_SATURN: 
      strcpy(s, SE_NAME_SATURN);
      break;
    case SE_URANUS: 
      strcpy(s, SE_NAME_URANUS);
      break;
    case SE_NEPTUNE: 
      strcpy(s, SE_NAME_NEPTUNE);
      break;
    case SE_PLUTO: 
      strcpy(s, SE_NAME_PLUTO);
      break;
    case SE_MEAN_NODE: 
      strcpy(s, SE_NAME_MEAN_NODE);
      break;
    case SE_TRUE_NODE: 
      strcpy(s, SE_NAME_TRUE_NODE);
      break;
    case SE_MEAN_APOG: 
      strcpy(s, SE_NAME_MEAN_APOG);
      break;
    case SE_OSCU_APOG: 
      strcpy(s, SE_NAME_OSCU_APOG);
      break;  
    case SE_INTP_APOG: 
      strcpy(s, SE_NAME_INTP_APOG);
      break;  
    case SE_INTP_PERG: 
      strcpy(s, SE_NAME_INTP_PERG);
      break;  
    case SE_EARTH: 
      strcpy(s, SE_NAME_EARTH);
      break;
    case SE_CHIRON: 
    case SE_AST_OFFSET + MPC_CHIRON: 
      strcpy(s, SE_NAME_CHIRON);
      break;
    case SE_PHOLUS: 
    case SE_AST_OFFSET + MPC_PHOLUS: 
      strcpy(s, SE_NAME_PHOLUS);
      break;
    case SE_CERES: 
    case SE_AST_OFFSET + MPC_CERES: 
      strcpy(s, SE_NAME_CERES);
      break;
    case SE_PALLAS: 
    case SE_AST_OFFSET + MPC_PALLAS: 
      strcpy(s, SE_NAME_PALLAS);
      break;
    case SE_JUNO: 
    case SE_AST_OFFSET + MPC_JUNO: 
      strcpy(s, SE_NAME_JUNO);
      break;
    case SE_VESTA: 
    case SE_AST_OFFSET + MPC_VESTA: 
      strcpy(s, SE_NAME_VESTA);
      break;
    default: 
      /* fictitious planets */
      if (ipl >= SE_FICT_OFFSET && ipl <= SE_FICT_MAX) {
        swi_get_fict_name(swi_default_ctx(), ipl - SE_FICT_OFFSET, s);
        break;
      }
      /* asteroids */
      if (ipl > SE_PLMOON_OFFSET || ipl > SE_AST_OFFSET) { // 2nd condition obsolete
	/* if name is already available */
	if (ipl == ctx->fidat[SEI_FILE_ANY_AST].ipl[0]) {
	  strcpy(s, ctx->fidat[SEI_FILE_ANY_AST].astnam);
        /* else try to get it from ephemeris file */
	} else {
	  retc = sweph(ctx, J2000, ipl, SEI_FILE_ANY_AST, 0, NULL, NO_SAVE, xp, NULL);
	  if (retc != ERR && retc != NOT_AVAILABLE) {
	    strcpy(s, ctx->fidat[SEI_FILE_ANY_AST].astnam);
	  } else {
	    if (ipl > SE_AST_OFFSET) {
	      sprintf(s, "%d: not found (asteroid)", ipl - SE_AST_OFFSET);
	    } else {
	      sprintf(s, "%d: not found (planetary moon)", ipl);
	    }
	  }
	}
        /* If there is a provisional designation only in ephemeris file,
         * we look for a name in seasnam.txt, which can be updated by
         * the user.
         * Some old ephemeris files return a '?' in the first position.
         * There are still a couple of unnamed bodies that got their
         * provisional designation before 1925, when the current method
         * of provisional designations was introduced. They have an 'A'
         * as the first character, e.g. A924 RC. 
         * The file seasnam.txt may contain comments starting with '#'.
         * There must be at least two columns: 
         * 1. asteroid catalog number
         * 2. asteroid name
         * The asteroid number may or may not be in brackets
         */
        if (ipl > SE_AST_OFFSET && (s[0] == '?' || isdigit((int) s[1]))) {
          int ipli = (int) (ipl - SE_AST_OFFSET), iplf = 0;
          FILE *fp;
          char si[AS_MAXCH], *sp, *sp2;
          if ((fp = swi_fopen(ctx, -1, SE_ASTNAMFILE, ctx->ephepath, NULL)) != NULL) {
            while(ipli != iplf && (sp = fgets(si, AS_MAXCH, fp)) != NULL) {
              while (*sp == ' ' || *sp == '\t' 
                     || *sp == '(' || *sp == '[' || *sp == '{')
                sp++;
              if (*sp == '#' || *sp == '\r' || *sp == '\n' || *sp == '\0')
                continue;
              /* catalog number of body of current line */
              iplf = atoi(sp);
              if (ipli != iplf)
                continue;
              /* set pointer after catalog number */
              sp = strpbrk(sp, " \t");
              if (sp == NULL)
                continue; /* there is no name */
              while (*sp == ' ' || *sp == '\t')
                sp++;
              sp2 = strpbrk(sp, "#\r\n");
              if (sp2 != NULL)
                *sp2 = '\0'; 
              if (*sp == '\0')
                continue;
              swi_right_trim(sp);
              strcpy(s, sp);
            }
            fclose(fp);
          }
        }
      } else  {
	i = ipl;
	sprintf(s, "%d", i);
      }
      break;
  }
#ifdef TRACE
  swi_open_trace(NULL);
  trace_swe_get_planet_name(2, ipl, s);
#endif
  if (strlen(s) < 80) {
    ctx->i_saved_planet_name = ipl;
    strcpy(ctx->saved_planet_name, s);
  }
  return s;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
char * CALL_CONV swe_get_planet_name(int ipl, char *s)
{
  return swe_get_planet_name_r(swi_default_ctx(), ipl, s);
}

const char *CALL_CONV swe_get_ayanamsa_name(int32 isidmode) 
{
  isidmode %= SE_SIDBITS;
  if (isidmode < SE_NSIDM_PREDEF)
    return ayanamsa_name[isidmode];
  return NULL;
}

#ifdef TRACE
static void trace_swe_calc(int swtch, double tjd, int ipl, int32 iflag, double *xx, char *serr)
{
  swi_trace_lock();
  if (swi_trace_count >= TRACE_COUNT_MAX) {
    swi_trace_unlock();
    return;
  }
  switch(swtch) {
    case 1:
      if (swi_fp_trace_c != NULL) {
	fputs("\n/*SWE_CALC*/\n", swi_fp_trace_c);
	fprintf(swi_fp_trace_c, "  tjd = %.9f;", tjd);
	fprintf(swi_fp_trace_c, " ipl = %d;", ipl);
	fprintf(swi_fp_trace_c, " iflag = %d;\n", iflag);
	fprintf(swi_fp_trace_c, "  iflgret = swe_calc(tjd, ipl, iflag, xx, serr);");
	fprintf(swi_fp_trace_c, "	/* xx = %p */\n", xx);
	fflush(swi_fp_trace_c);
      } 
      break;
    case 2:
      if (swi_fp_trace_c != NULL) {
	fputs("  printf(\"swe_calc: %f\\t%d\\t%d\\t%f\\t%f\\t%f\\t%f\\t%f\\t%f\\t\", ", swi_fp_trace_c); 
	fputs("\n\ttjd, ipl, iflgret, xx[0], xx[1], xx[2], xx[3], xx[4], xx[5]);\n", swi_fp_trace_c);
	fputs("  if (*serr != '\\0')", swi_fp_trace_c);
	fputs(" printf(serr);", swi_fp_trace_c);
	fputs(" printf(\"\\n\");\n", swi_fp_trace_c);
	fflush(swi_fp_trace_c);
      }
      if (swi_fp_trace_out != NULL) {
	fprintf(swi_fp_trace_out, "swe_calc: %f\t%d\t%d\t%f\t%f\t%f\t%f\t%f\t%f\t", 
		      tjd, ipl, iflag, xx[0], xx[1], xx[2], xx[3], xx[4], xx[5]);
	if (serr != NULL && *serr != '\0') {
	  fputs(serr, swi_fp_trace_out);
	}
	fputs("\n", swi_fp_trace_out);
	fflush(swi_fp_trace_out);
      }
      break;
    default:
      break;
  }
  swi_trace_unlock();
}

static void trace_swe_fixstar(int swtch, char *star, double tjd, int32 iflag, double *xx, char *serr)
{
  swi_trace_lock();
  if (swi_trace_count >= TRACE_COUNT_MAX) {
    swi_trace_unlock();
    return;
  }
  switch(swtch) {
  case 1:
    if (swi_fp_trace_c != NULL) {
      fputs("\n/*SWE_FIXSTAR*/\n", swi_fp_trace_c);
      fprintf(swi_fp_trace_c, "  strcpy(star, \"%s\");", star);
      fprintf(swi_fp_trace_c, " tjd = %.9f;", tjd);
      fprintf(swi_fp_trace_c, " iflag = %d;\n", iflag);
      fprintf(swi_fp_trace_c, "  iflgret = swe_fixstar(star, tjd, iflag, xx, serr);");
      fprintf(swi_fp_trace_c, "   /* xx = %p */\n", xx);
      fflush(swi_fp_trace_c);
    } 
    break;
  case 2:
    if (swi_fp_trace_c != NULL) {
      fputs("  printf(\"swe_fixstar: %s\\t%f\\t%d\\t%f\\t%f\\t%f\\t%f\\t%f\\t%f\\t\", ", swi_fp_trace_c);
      fputs("\n\tstar, tjd, iflgret, xx[0], xx[1], xx[2], xx[3], xx[4], xx[5]);\n", swi_fp_trace_c);/**/
      fputs("  if (*serr != '\\0')", swi_fp_trace_c);
      fputs(" printf(serr);", swi_fp_trace_c);
      fputs(" printf(\"\\n\");\n", swi_fp_trace_c);
      fflush(swi_fp_trace_c);
    }
    if (swi_fp_trace_out != NULL) {
      fprintf(swi_fp_trace_out, "swe_fixstar: %s\t%f\t%d\t%f\t%f\t%f\t%f\t%f\t%f\t", 
		    star, tjd, iflag, xx[0], xx[1], xx[2], xx[3], xx[4], xx[5]);
      if (serr != NULL && *serr != '\0') {
	fputs(serr, swi_fp_trace_out);
      }
      fputs("\n", swi_fp_trace_out);
      fflush(swi_fp_trace_out);
    }
    break;
  default:
    break;
  }
  swi_trace_unlock();
}

static void trace_swe_get_planet_name(int swtch, int ipl, char *s)
{
  swi_trace_lock();
  if (swi_trace_count >= TRACE_COUNT_MAX) {
    swi_trace_unlock();
    return;
  }
  switch(swtch) {
    case 1:
      if (swi_fp_trace_c != NULL) {
	fputs("\n/*SWE_GET_PLANET_NAME*/\n", swi_fp_trace_c);
	fprintf(swi_fp_trace_c, "  ipl = %d;\n", ipl);
	fprintf(swi_fp_trace_c, "  swe_get_planet_name(ipl, s);");
	fprintf(swi_fp_trace_c, "   /* s = %p */\n", s);
	fflush(swi_fp_trace_c);
      } 
      break;
    case 2:
      if (swi_fp_trace_c != NULL) {
	fputs("  printf(\"swe_get_planet_name: %d\\t%s\\t\\n\", ", swi_fp_trace_c);
	fputs("ipl, s);\n", swi_fp_trace_c);/**/
	fflush(swi_fp_trace_c);
      }
      if (swi_fp_trace_out != NULL) {
	fprintf(swi_fp_trace_out, "swe_get_planet_name: %d\t%s\t\n", ipl, s);
	fflush(swi_fp_trace_out);
      }
      break;
    default:
      break;
  }
  swi_trace_unlock();
}

#endif

/* set geographic position and altitude of observer */
void CALL_CONV swe_set_topo_r(swe_ctx *ctx, double geolon, double geolat, double geoalt)
{
  AS_BOOL was = swi_config_begin_apply(ctx);
  swi_init_swed_if_start(ctx);
  if (ctx->geopos_is_set == TRUE
    && ctx->topd.geolon == geolon
    && ctx->topd.geolat == geolat
    && ctx->topd.geoalt == geoalt) {
    swi_config_end_apply(ctx, was);
    swi_config_claim(ctx, SWI_CFG_TOPO);
    return;
  }
  ctx->topd.geolon = geolon;
  ctx->topd.geolat = geolat;
  ctx->topd.geoalt = geoalt;
  ctx->geopos_is_set = TRUE;
  /* to force new calculation of observer position vector */
  ctx->topd.teval = 0;
  /* to force new calculation of light-time etc.
   */
  swi_force_app_pos_etc(ctx);
  swi_config_end_apply(ctx, was);
  swi_config_publish(ctx, SWI_CFG_TOPO);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
void CALL_CONV swe_set_topo(double geolon, double geolat, double geoalt)
{
  swe_set_topo_r(swi_default_ctx(), geolon, geolat, geoalt);
}

void swi_force_app_pos_etc(swe_ctx *ctx)
{
  int i;
  for (i = 0; i < SEI_NPLANETS; i++)
    ctx->pldat[i].xflgs = -1;
  for (i = 0; i < SEI_NNODE_ETC; i++)
    ctx->nddat[i].xflgs = -1;
  for (i = 0; i <= SE_NPLANETS; i++) { // "=" because save area for asteroids > SE_AST_OFFSET is at i == SE_NPLANETS
    ctx->savedat[i].tsave = 0;
    ctx->savedat[i].iflgsave = -1;
  }
}

int swi_get_observer(swe_ctx *ctx, double tjd, int32 iflag, 
	AS_BOOL do_save, double *xobs, char *serr)
{
  int i;
  double sidt, delt, tjd_ut, eps, nut, nutlo[2];
  double f = EARTH_OBLATENESS;
  double re = EARTH_RADIUS; 
  double cosfi, sinfi, cc, ss, cosl, sinl, h;
  if (!ctx->geopos_is_set) {
    if (serr != NULL)
      strcpy(serr, "geographic position has not been set");
    return ERR;
  }
  /* geocentric position of observer depends on sidereal time,
   * which depends on UT. 
   * compute UT from ET. this UT will be slightly different
   * from the user's UT, but this difference is extremely small.
   */
  delt = swe_deltat_ex_r(ctx, tjd, iflag, serr);
  tjd_ut = tjd - delt;
  if (ctx->oec.teps == tjd && ctx->nut.tnut == tjd) {
    eps = ctx->oec.eps;
    nutlo[1] = ctx->nut.nutlo[1];
    nutlo[0] = ctx->nut.nutlo[0];
  } else {
    eps = swi_epsiln(ctx, tjd, iflag);
    if (!(iflag & SEFLG_NONUT)) 
      swi_nutation(ctx, tjd, iflag, nutlo);
  }
  if (iflag & SEFLG_NONUT) {
    nut = 0;
  } else {
    eps += nutlo[1];
    nut = nutlo[0];
  }
  /* mean or apparent sidereal time, depending on whether or
   * not SEFLG_NONUT is set */
  sidt = swe_sidtime0_r(ctx, tjd_ut, eps * RADTODEG, nut * RADTODEG);
  sidt *= 15;	/* in degrees */
  /* length of position and speed vectors;
   * the height above sea level must be taken into account.
   * with the moon, an altitude of 3000 m makes a difference 
   * of about 2 arc seconds.
   * height is referred to the average sea level. however, 
   * the spheroid (geoid), which is defined by the average 
   * sea level (or rather by all points of same gravitational
   * potential), is of irregular shape and cannot easily
   * be taken into account. therefore, we refer height to 
   * the surface of the ellipsoid. the resulting error 
   * is below 500 m, i.e. 0.2 - 0.3 arc seconds with the moon.
   */
  cosfi = cos(ctx->topd.geolat * DEGTORAD);
  sinfi = sin(ctx->topd.geolat * DEGTORAD);
  cc= 1 / sqrt(cosfi * cosfi + (1-f) * (1-f) * sinfi * sinfi); 
  ss= (1-f) * (1-f) * cc; 
  /* neglect polar motion (displacement of a few meters), as long as 
   * we use the earth ellipsoid */
  /* ... */
  /* add sidereal time */
  cosl = cos((ctx->topd.geolon + sidt) * DEGTORAD);
  sinl = sin((ctx->topd.geolon + sidt) * DEGTORAD);
  h = ctx->topd.geoalt;
  xobs[0] = (re * cc + h) * cosfi * cosl;
  xobs[1] = (re * cc + h) * cosfi * sinl;
  xobs[2] = (re * ss + h) * sinfi;
  /* polar coordinates */
  swi_cartpol(xobs, xobs);
  /* speed */
  xobs[3] = EARTH_ROT_SPEED;		
  xobs[4] = xobs[5] = 0;
  swi_polcart_sp(xobs, xobs);
  /* to AUNIT */
  for (i = 0; i <= 5; i++)
    xobs[i] /= AUNIT;
  /* subtract nutation, set backward flag */
  if (!(iflag & SEFLG_NONUT)) {
    swi_coortrf2(xobs, xobs, -ctx->nut.snut, ctx->nut.cnut);
    /* speed of xobs is always required, namely for aberration!!! */
    /*if (iflag & SEFLG_SPEED)*/
      swi_coortrf2(xobs+3, xobs+3, -ctx->nut.snut, ctx->nut.cnut);
    swi_nutate(ctx, xobs, iflag | SEFLG_SPEED, TRUE);
  }
  /* precess to J2000 */
  swi_precess(ctx, xobs, tjd, iflag, J_TO_J2000);
  /*if (iflag & SEFLG_SPEED)*/
    swi_precess_speed(ctx, xobs, tjd, iflag, J_TO_J2000);
  /* neglect frame bias (displacement of 45cm) */
  /* ... */
  /* save */
  if (do_save) {
    for (i = 0; i <= 5; i++)
      ctx->topd.xobs[i] = xobs[i];
    ctx->topd.teval = tjd;
    ctx->topd.tjd_ut = tjd_ut;	/* -> save area */
  }
  return OK;
}

/* Equation of Time
 *
 * The function returns the difference between 
 * local apparent and local mean time in days.
 * E = LAT - LMT
 * Input variable tjd is UT.
 */
int32 CALL_CONV swe_time_equ_r(swe_ctx *ctx, double tjd_ut, double *E, char *serr)
{
  int32 retval;
  double t, dt, x[6];
  double sidt = swe_sidtime_r(ctx, tjd_ut);
  int32 iflag = SEFLG_EQUATORIAL;
  iflag = plaus_iflag(ctx, iflag, -1, tjd_ut, serr);
  if (swi_init_swed_if_start(ctx) == 1 && !(iflag & SEFLG_MOSEPH) && serr != NULL) {
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_time_equ(), swe_lmt_to_lat() or swe_lat_to_lmt()");
  }
  if (ctx->jpl_file_is_open)
    iflag |= SEFLG_JPLEPH;
  t = tjd_ut + 0.5;
  dt = t - floor(t);
  sidt -= dt * 24;
  sidt *= 15;
  if ((retval = swe_calc_ut_r(ctx, tjd_ut, SE_SUN, iflag, x, serr)) == ERR) {
    *E = 0;
    return ERR;
  }
  dt = swe_degnorm(sidt - x[0] - 180);
  if (dt > 180)
    dt -= 360;
  dt *= 4;
  *E = dt / 1440.0;
  return OK;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_time_equ(double tjd_ut, double *E, char *serr)
{
  return swe_time_equ_r(swi_default_ctx(), tjd_ut, E, serr);
}

int32 CALL_CONV swe_lmt_to_lat_r(swe_ctx *ctx, double tjd_lmt, double geolon, double *tjd_lat, char *serr)
{
  int32 retval;
  double E, tjd_lmt0;
  tjd_lmt0 = tjd_lmt - geolon / 360.0;
  retval = swe_time_equ_r(ctx, tjd_lmt0, &E, serr);
  *tjd_lat = tjd_lmt + E;
  return retval;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_lmt_to_lat(double tjd_lmt, double geolon, double *tjd_lat, char *serr)
{
  return swe_lmt_to_lat_r(swi_default_ctx(), tjd_lmt, geolon, tjd_lat, serr);
}

int32 CALL_CONV swe_lat_to_lmt_r(swe_ctx *ctx, double tjd_lat, double geolon, double *tjd_lmt, char *serr)
{
  int32 retval;
  double E, tjd_lmt0;
  tjd_lmt0 = tjd_lat - geolon / 360.0;
  retval = swe_time_equ_r(ctx, tjd_lmt0, &E, serr);
  /* iteration */
  retval = swe_time_equ_r(ctx, tjd_lmt0 - E, &E, serr);
  retval = swe_time_equ_r(ctx, tjd_lmt0 - E, &E, serr);
  *tjd_lmt = tjd_lat - E;
  return retval;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_lat_to_lmt(double tjd_lat, double geolon, double *tjd_lmt, char *serr)
{
  return swe_lat_to_lmt_r(swi_default_ctx(), tjd_lat, geolon, tjd_lmt, serr);
}

static int open_jpl_file(swe_ctx *ctx, double *ss, char *fname, char *fpath, char *serr)
{
  int retc;
  char serr2[AS_MAXCH];
  retc = swi_open_jpl_file(ctx, ss, fname, fpath, serr);
  /* If we fail with default JPL ephemeris (DE431), we try the second default
   * (DE406), but only if serr is not NULL and an warning message can be 
   * returned. */
  if (retc != OK && strstr(fname, SE_FNAME_DFT) != NULL && serr != NULL) {
    retc = swi_open_jpl_file(ctx, ss, SE_FNAME_DFT2, fpath, serr2);
    if (retc == OK) {
      strcpy(ctx->jplfnam, SE_FNAME_DFT2);
      if (serr != NULL) {
        strcpy(serr2, "Error with JPL ephemeris file ");
	if (strlen(serr2) + strlen(SE_FNAME_DFT) < AS_MAXCH)
	  strcat(serr2, SE_FNAME_DFT);
	if (strlen(serr2) + strlen(serr) + 2 < AS_MAXCH) 
	  sprintf(serr2 + strlen(serr2), ": %s", serr);
	if (strlen(serr2) + 17 < AS_MAXCH) 
	  strcat(serr2, ". Defaulting to ");
	if (strlen(serr2) + strlen(SE_FNAME_DFT2) < AS_MAXCH) 
	  strcat(serr2, SE_FNAME_DFT2);
        strcpy(serr, serr2);
      }
    }
  }
  if (retc == OK) {
    ctx->jpldenum = swi_get_jpl_denum(ctx);
    ctx->jpl_file_is_open = TRUE;
    swi_set_tid_acc(ctx, 0, 0, ctx->jpldenum, serr);
  }
  return retc;
}

#if 1
static int32 swi_fixstar_load_record(swe_ctx *ctx, char *star, char *srecord, char *sname, char *sbayer, double *dparams, char *serr)
{
  char s[AS_MAXCH + 20], *sp, *sp2;	/* 20 byte for SE_STARFILE */
  char sstar[SWI_STAR_LENGTH + 1];
  char fstar[SWI_STAR_LENGTH + 1];
  int i, star_nr = 0;
  int line = 0;
  int fline = 0;
  int32 retc = OK;
  AS_BOOL  is_bayer = FALSE;
  size_t cmplen;
  size_t slen;
  struct fixed_star stardata;
  (void) sname;
  (void) sbayer;
  /* function formats the input search name of a star:
   * - remove white spaces
   * - traditional name to lower case (Bayer designation remains as it is)
   */
  retc = fixstar_format_search_name(star, sstar, serr);
  if (retc == ERR)
    return ERR;
  // search name is Bayer designation
  if (*sstar == ',') {
    is_bayer = TRUE;
  // search name star number in sefstars.txt
  } else if (isdigit((int) *sstar)) {
    star_nr = atoi(sstar);
  // traditional name: cut off Bayer designation
  } else {
    if ((sp = strchr(sstar, ',')) != NULL)
      *sp = '\0';
  }
  cmplen = strlen(sstar);
  /******************************************************
   * Star file
   * close to the beginning, a few stars selected by Astrodienst.
   * These can be accessed by giving their number instead of a name.
   * All other stars can be accessed by name.
   * Comment lines start with # and are ignored.
   ******************************************************/
  if (ctx->fixfp == NULL) {
    if ((ctx->fixfp = swi_fopen(ctx, SEI_FILE_FIXSTAR, SE_STARFILE, ctx->ephepath, serr)) == NULL) {
      ctx->is_old_starfile = TRUE;
      if ((ctx->fixfp = swi_fopen(ctx, SEI_FILE_FIXSTAR, SE_STARFILE_OLD, ctx->ephepath, NULL)) == NULL) {
	ctx->is_old_starfile = FALSE;
	/* no fixed star file available, error message is already in serr. */
	return ERR;
      }
    }
  }
  rewind(ctx->fixfp);
  while (fgets(s, AS_MAXCH, ctx->fixfp) != NULL) {
    fline++;	
    // skip comment lines
    if (*s == '#') continue;
    line++;
    // search string is star number in sefstars.txt
    if (star_nr == line) {
      goto found;
    } else if (star_nr > 0) {
      continue;
    }
    // invalid line without comma
    if ((sp = strchr(s, ',')) == NULL) {
      if (serr != NULL) {
	sprintf(serr, "star file %s damaged at line %d", SE_STARFILE, fline);
      }
      return ERR;
    } 
    // search string is Bayer or Flamsteed designation
    if (is_bayer) {
      if (strncmp(sp, sstar, cmplen) == 0) {
        goto found;
      } else {
        continue;
      }
    }
    // search string is traditional name
    *sp = '\0';	/* cut off after first field to get star name, ',' -> '\0' */
    //slen = swi_strnlen(s, SE_MAX_STNAME);
    slen = strlen(s);
    if (slen > SE_MAX_STNAME) slen = SE_MAX_STNAME;
    memcpy(fstar, s, slen);
    fstar[slen] = '\0';  /* force termination */
    *sp = ',';  /* add comma again */
    //fstar[SWI_STAR_LENGTH] = '\0';	/* force termination */
    // remove white spaces from star name
    while ((sp = strchr(fstar, ' ')) != NULL)
      swi_strcpy(sp, sp+1);
    i = (int) strlen(fstar);
    // length of star name differs from length of search string: continue
    if (i < (int) cmplen)
      continue;
    // star name to lowercase and compare with search string
    for (sp2 = fstar; *sp2 != '\0'; sp2++) {
      *sp2 = tolower((int) *sp2);
    }
    if (strncmp(fstar, sstar, cmplen) == 0) 
      goto found;
  }
  if (serr != NULL) {
    sprintf(serr, "star  not found");
    if (strlen(serr) + strlen(star) < AS_MAXCH) {
      sprintf(serr, "star %s not found", star);
    }
    return ERR;
  }
  found:
  strcpy(srecord, s);
  retc = fixstar_cut_string(ctx, srecord, star, &stardata, serr);
  if (retc == ERR) return ERR;
  if (dparams != NULL) {
    dparams[0] = stardata.epoch; // epoch
    // RA(epoch)
    dparams[1] = stardata.ra;
    // Decl(epoch)
    dparams[2] = stardata.de;
    // RA proper motion
    dparams[3] = stardata.ramot;
    // decl proper motion
    dparams[4] = stardata.demot;
    // radial velocity
    dparams[5] = stardata.radvel;
    // parallax
    dparams[6] = stardata.parall;
    // magnitude V
    dparams[7] = stardata.mag;
  }
  return OK;
}

/* function calculates a fixstar from a record from sefstars.txt
 * input:
 * char *srecord     fixed star data record from sefstars.txt
 * double tjd        julian daynumber 
 * int32 iflag       SEFLG_ specifications
 * output:
 * char *star        star name, Bayer designation
 * double xx[6]      position and speed
 * char *serr        error return string
 */
static int32 swi_fixstar_calc_from_record(swe_ctx *ctx, char *srecord, double tjd, int32 iflag, char *star, double *xx, char *serr)
{
  int i;
  int32 retc = OK;
  double epoch, radv, parall;
  double ra_pm, de_pm, ra, de, t;
  struct fixed_star stardata;
  double daya, rdist;
  double x[6], xxsv[6], xobs[6], xobs_dt[6], *xpo = NULL, *xpo_dt = NULL;
  /* moved to ctx->sp.fx_record (Phase 3c) */
  double dt = PLAN_SPEED_INTV * 0.1;
  int32 epheflag, iflgsave;
  // char s[AS_MAXCH];
  struct epsilon *oe = &ctx->oec2000;
  iflgsave = iflag;
  iflag |= SEFLG_SPEED; /* we need this in order to work correctly */
  if (serr != NULL)
    *serr = '\0';
  iflag = plaus_iflag(ctx, iflag, -1, tjd, serr);
  epheflag = iflag & SEFLG_EPHMASK;
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL) {
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_fixstar() or swe_fixstar_ut()");
  }
  if (ctx->last_epheflag != epheflag) {
    free_planets(ctx);
    /* close and free ephemeris files */
    if (ctx->jpl_file_is_open) {
      swi_close_jpl_file(ctx);
      ctx->jpl_file_is_open = FALSE;
    }
    for (i = 0; i < SEI_NEPHFILES; i ++) {
      if (ctx->fidat[i].fptr != NULL) 
	fclose(ctx->fidat[i].fptr);
      memset((void *) &ctx->fidat[i], 0, sizeof(struct file_data));
    }
    ctx->last_epheflag = epheflag;
  }
  /* high precision speed prevails fast speed */
  /* JPL Horizons is only reproduced with SEFLG_JPLEPH */
  if (iflag & SEFLG_SIDEREAL && !ctx->ayana_is_set)
    SWI_CFG_LOCAL(ctx, swe_set_sid_mode_r(ctx, SE_SIDM_FAGAN_BRADLEY, 0, 0));
  /****************************************** 
   * obliquity of ecliptic 2000 and of date * 
   ******************************************/
  swi_check_ecliptic(ctx, tjd, iflag);
  /******************************************
   * nutation                               * 
   ******************************************/
  swi_check_nutation(ctx, tjd, iflag);
  retc = fixstar_cut_string(ctx, srecord, star, &stardata, serr);
  if (retc == ERR) return ERR;
  epoch = stardata.epoch;
  ra_pm = stardata.ramot; de_pm = stardata.demot;
  radv = stardata.radvel; parall = stardata.parall; 
  ra = stardata.ra; de = stardata.de;
  if (epoch == 1950) {
    t= (tjd - B1950);	/* days since 1950.0 */
  } else { /* epoch == 2000 */
    t= (tjd - J2000);	/* days since 2000.0 */
  }
  x[0] = ra;
  x[1] = de;
  x[2] = 1;	
  if (parall == 0) {
    rdist = 1000000000;  
  } else {
    rdist = 1.0 / (parall * RADTODEG * 3600) * PARSEC_TO_AUNIT;	
    //rdist += t * radv / 36525.0;
  }
// rdist = 10000;  // to reproduce pre-SE2.07 star positions
  x[2] = rdist;
  x[3] = ra_pm / 36525.0;
  x[4] = de_pm / 36525.0;
  x[5] = radv / 36525.0;
  // Cartesian space motion vector
  swi_polcart_sp(x, x);
  /******************************************
   * FK5
   ******************************************/
  if (epoch == 1950) {
    swi_FK4_FK5(x, B1950);
    swi_precess(ctx, x, B1950, 0, J_TO_J2000);
    swi_precess(ctx, x+3, B1950, 0, J_TO_J2000);
  } 
  /* FK5 to ICRF, if jpl ephemeris is referred to ICRF.
   * With data that are already ICRF, epoch = 0 */
  if (epoch != 0) {
    swi_icrs2fk5(x, iflag, TRUE); /* backward, i. e. to icrf */
    /* with ephemerides < DE403, we now convert to J2000 */
    if (swi_get_denum(ctx, SEI_SUN, iflag) >= 403) {
      swi_bias(ctx, x, J2000, SEFLG_SPEED, FALSE);
    }
  }
  /**************************************************** 
   * earth/sun 
   * for parallax, light deflection, and aberration,
   ****************************************************/
  if (!(iflag & SEFLG_BARYCTR) && (!(iflag & SEFLG_HELCTR) || !(iflag & SEFLG_MOSEPH))) {
    if ((retc =  main_planet_bary(ctx, tjd - dt, SEI_EARTH, epheflag, iflag, NO_SAVE, ctx->sp.fx_record.xearth_dt, ctx->sp.fx_record.xearth_dt, ctx->sp.fx_record.xsun_dt, NULL, serr)) != OK) {
      return ERR;
    }
    if ((retc =  main_planet_bary(ctx, tjd, SEI_EARTH, epheflag, iflag, DO_SAVE, ctx->sp.fx_record.xearth, ctx->sp.fx_record.xearth, ctx->sp.fx_record.xsun, NULL, serr)) != OK) {
      return ERR;
    }
  }
  /************************************
   * observer: geocenter or topocenter
   ************************************/
  /* if topocentric position is wanted  */
  if (iflag & SEFLG_TOPOCTR) { 
    if (swi_get_observer(ctx, tjd - dt, iflag | SEFLG_NONUT, NO_SAVE, xobs_dt, serr) != OK)
      return ERR;
    if (swi_get_observer(ctx, tjd, iflag | SEFLG_NONUT, NO_SAVE, xobs, serr) != OK)
      return ERR;
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++) {
      xobs[i] = xobs[i] + ctx->sp.fx_record.xearth[i];	
      xobs_dt[i] = xobs_dt[i] + ctx->sp.fx_record.xearth_dt[i];	
    }
  } else if (!(iflag & SEFLG_BARYCTR) && (!(iflag & SEFLG_HELCTR) || !(iflag & SEFLG_MOSEPH))) {
    /* barycentric position of geocenter */
    for (i = 0; i <= 5; i++) {
      xobs[i] = ctx->sp.fx_record.xearth[i];
      xobs_dt[i] = ctx->sp.fx_record.xearth_dt[i];
    }
  }
  /************************************
   * position and speed at tjd        *
   ************************************/
  /* for parallax */ 
  if ((iflag & SEFLG_HELCTR) && (iflag & SEFLG_MOSEPH)) {
    xpo = NULL;		/* no parallax, if moshier and heliocentric */
    xpo_dt = NULL;	/* no parallax, if moshier and heliocentric */
  } else if (iflag & SEFLG_HELCTR) {
    xpo = ctx->sp.fx_record.xsun;//psdp->x;
    xpo_dt = ctx->sp.fx_record.xsun_dt; 
  } else if (iflag & SEFLG_BARYCTR) {
    xpo = NULL;		/* no parallax, if barycentric */
    xpo_dt = NULL;	/* no parallax, if moshier and heliocentric */
  } else {
    xpo = xobs;
    xpo_dt = xobs_dt;
  }
  if (xpo == NULL) {
    for (i = 0; i <= 2; i++) {
      x[i] += t * x[i+3];	
    }
  } else {
    for (i = 0; i <= 2; i++) {
      x[i] += t * x[i+3];
      x[i] -= xpo[i];
      x[i+3] -= xpo[i+3];
    }
    // rdist of date
    //rdist = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    // parallax of date
    //parall = PARSEC_TO_AUNIT / rdist / RADTODEG / 3600.0;
  }
  /************************************
   * relativistic deflection of light *
   ************************************/
  if ((iflag & SEFLG_TRUEPOS) == 0 && (iflag & SEFLG_NOGDEFL) == 0) {
    swi_deflect_light(ctx, x, 0, iflag & SEFLG_SPEED);
  }
  /**********************************
   * 'annual' aberration of light   *
   * speed is incorrect !!!         *
   **********************************/
  if ((iflag & SEFLG_TRUEPOS) == 0 && (iflag & SEFLG_NOABERR) == 0)
    swi_aberr_light_ex(x, xpo, xpo_dt, dt, iflag & SEFLG_SPEED);
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && (swi_get_denum(ctx, SEI_SUN, iflag) >= 403 || (iflag & SEFLG_BARYCTR))) {
    swi_bias(ctx, x, tjd, iflag, FALSE);
  }/**/
  /* save J2000 coordinates; required for sidereal positions */
  for (i = 0; i <= 5; i++)
    xxsv[i] = x[i];
  /************************************************
   * precession, equator 2000 -> equator of date *
   ************************************************/
  /*x[0] = -0.374018403; x[1] = -0.312548592; x[2] = -0.873168719;*/
  if ((iflag & SEFLG_J2000) == 0) {
    swi_precess(ctx, x, tjd, iflag, J2000_TO_J);
    if (iflag & SEFLG_SPEED) {
      swi_precess_speed(ctx, x, tjd, iflag, J2000_TO_J);
    }
    oe = &ctx->oec;
  } else {
    oe = &ctx->oec2000;
  }
  /************************************************
   * nutation                                     *
   ************************************************/
  if (!(iflag & SEFLG_NONUT))
    swi_nutate(ctx, x, iflag, FALSE);
  /************************************************
   * transformation to ecliptic.                  *
   * with sidereal calc. this will be overwritten *
   * afterwards.                                  *
   ************************************************/
  if ((iflag & SEFLG_EQUATORIAL) == 0) {
    swi_coortrf2(x, x, oe->seps, oe->ceps);
    if (iflag & SEFLG_SPEED)
      swi_coortrf2(x+3, x+3, oe->seps, oe->ceps);
    if (!(iflag & SEFLG_NONUT)) {
      swi_coortrf2(x, x, ctx->nut.snut, ctx->nut.cnut);
      if (iflag & SEFLG_SPEED)
	swi_coortrf2(x+3, x+3, ctx->nut.snut, ctx->nut.cnut);
    }
  }
  /************************************
   * sidereal positions               *
   ************************************/
  if (iflag & SEFLG_SIDEREAL) {
    /* rigorous algorithm */
    if (ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) {
      if (swi_trop_ra2sid_lon(ctx, xxsv, x, xxsv, iflag) != OK)
        return ERR;
      if (iflag & SEFLG_EQUATORIAL) {
        for (i = 0; i <= 5; i++)
          x[i] = xxsv[i];
      }
    /* project onto solar system equator */
    } else if (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE) {
      if (swi_trop_ra2sid_lon_sosy(ctx, xxsv, x, iflag) != OK)
	return ERR;
      if (iflag & SEFLG_EQUATORIAL) {
        for (i = 0; i <= 5; i++)
          x[i] = xxsv[i];
      }
    /* traditional algorithm */
    } else {
      swi_cartpol_sp(x, x); 
      if (swi_get_ayanamsa_ex(ctx, tjd, iflag, &daya, serr) == ERR)
        return ERR;
      x[0] -= daya * DEGTORAD;
      swi_polcart_sp(x, x); 
    }
  } 
  /************************************************
   * transformation to polar coordinates          *
   ************************************************/
  if ((iflag & SEFLG_XYZ) == 0)
    swi_cartpol_sp(x, x); 
  /********************** 
   * radians to degrees *
   **********************/
  if ((iflag & SEFLG_RADIANS) == 0 && (iflag & SEFLG_XYZ) == 0) {
    for (i = 0; i < 2; i++) {
      x[i] *= RADTODEG;
      x[i+3] *= RADTODEG;
    }
  }
  for (i = 0; i <= 5; i++)
    xx[i] = x[i];
  if (!(iflgsave & SEFLG_SPEED)) {
    iflag = iflag & ~SEFLG_SPEED;
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  }
  /* if no ephemeris has been specified, do not return chosen ephemeris */
  if ((iflgsave & SEFLG_EPHMASK) == 0)
    iflag = iflag & ~SEFLG_DEFAULTEPH;
  return iflag;
}

/**********************************************************
 * get fixstar positions
 * parameters:
 * star 	name of star or line number in star file 
 *		(start from 1, don't count comment).
 *    		If no error occurs, the name of the star is returned
 *	        in the format trad_name, nomeclat_name
 *
 * tjd 		absolute julian day
 * iflag	s. swecalc(ctx); speed bit does not function
 * x		pointer for returning the ecliptic coordinates
 * serr		error return string
**********************************************************/
int32 CALL_CONV swe_fixstar_r(swe_ctx *ctx, char *star, double tjd, int32 iflag, 
  double *xx, char *serr)
{
  int i;
  char sstar[SWI_STAR_LENGTH + 1];
  /* moved to ctx->sp.fs1 (Phase 3c) */
  /* moved to ctx->sp.fs1 (Phase 3c) */
  char srecord[AS_MAXCH + 20], *sp;	/* 20 byte for SE_STARFILE */
  int retc;
  if (serr != NULL)
    *serr = '\0';
#ifdef TRACE
  swi_open_trace(serr);
  trace_swe_fixstar(1, star, tjd, iflag, xx, serr);
#endif /* TRACE */
  retc = fixstar_format_search_name(star, sstar, serr);
  if (retc == ERR)
    goto return_err;
  if (*sstar == ',') {
    ; // is Bayer designation
  } else if (isdigit((int) *sstar)) {
    ; // is a sequential star number
  } else {
    if ((sp = strchr(sstar, ',')) != NULL) // cut off Bayer, if trad. name
      *sp = '\0';
  }
  /* star elements from last call: */
  if (*ctx->sp.fs1.slast_stardata != '\0' && strcmp(ctx->sp.fs1.slast_starname, sstar) == 0) {
    strcpy(srecord, ctx->sp.fs1.slast_stardata);
    goto found;
  }
  if (get_builtin_star(star, sstar, srecord)) {
    goto found;
  }
  /******************************************************
   * Star file
   * close to the beginning, a few stars selected by Astrodienst.
   * These can be accessed by giving their number instead of a name.
   * All other stars can be accessed by name.
   * Comment lines start with # and are ignored.
   ******************************************************/
  if ((retc = swi_fixstar_load_record(ctx, star, srecord, NULL, NULL, NULL, serr)) != OK)
    goto return_err;
  found:
  strcpy(ctx->sp.fs1.slast_stardata, srecord);
  strcpy(ctx->sp.fs1.slast_starname, sstar);
  if ((retc = swi_fixstar_calc_from_record(ctx, srecord, tjd, iflag, star, xx, serr)) == ERR)
    goto return_err;
#ifdef TRACE
  trace_swe_fixstar(2, star, tjd, iflag, xx, serr);
#endif
  return iflag;
  return_err:
  for (i = 0; i <= 5; i++)
    xx[i] = 0;
#ifdef TRACE
  trace_swe_fixstar(2, star, tjd, iflag, xx, serr);
#endif
  return retc;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar(char *star, double tjd, int32 iflag, 
  double *xx, char *serr)
{
  return swe_fixstar_r(swi_default_ctx(), star, tjd, iflag, xx, serr);
}

int32 CALL_CONV swe_fixstar_ut_r(swe_ctx *ctx, char *star, double tjd_ut, int32 iflag, 
  double *xx, char *serr)
{
  double deltat;
  int32 retflag;
  int32 epheflag = 0;
  iflag = plaus_iflag(ctx, iflag, -1, tjd_ut, serr);
  epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag == 0) {
    epheflag = SEFLG_SWIEPH;
    iflag |= SEFLG_SWIEPH;
  }
  deltat = swe_deltat_ex_r(ctx, tjd_ut, iflag, serr);
  /* if ephe required is not ephe returned, adjust delta t: */
  retflag = swe_fixstar_r(ctx, star, tjd_ut + deltat, iflag, xx, serr);
  if (retflag != ERR && (retflag & SEFLG_EPHMASK) != epheflag) {
    deltat = swe_deltat_ex_r(ctx, tjd_ut, retflag, NULL);
    retflag = swe_fixstar_r(ctx, star, tjd_ut + deltat, iflag, xx, NULL);
  }
  return retflag;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar_ut(char *star, double tjd_ut, int32 iflag, 
  double *xx, char *serr)
{
  return swe_fixstar_ut_r(swi_default_ctx(), star, tjd_ut, iflag, xx, serr);
}

/**********************************************************
 * get fixstar magnitude
 * parameters:
 * star 	name of star or line number in star file 
 *		(start from 1, don't count comment).
 *    		If no error occurs, the name of the star is returned
 *	        in the format trad_name, nomeclat_name
 *
 * mag 		pointer to a double, for star magnitude
 * serr		error return string
**********************************************************/
int32 CALL_CONV swe_fixstar_mag_r(swe_ctx *ctx, char *star, double *mag, char *serr)
{
  char sstar[SWI_STAR_LENGTH + 1];
  /* moved to ctx->sp.fs1mag (Phase 3c) */
  /* moved to ctx->sp.fs1mag (Phase 3c) */
  char srecord[AS_MAXCH + 20], *sp;	/* 20 byte for SE_STARFILE */
  struct fixed_star stardata;
  int retc;
  double dparams[20];
  if (serr != NULL)
    *serr = '\0';
  retc = fixstar_format_search_name(star, sstar, serr);
  if (retc == ERR)
    goto return_err;
  if (*sstar == ',') {
    ; // is Bayer designation
  } else if (isdigit((int) *sstar)) {
    ; // is a sequential star number
  } else {
    if ((sp = strchr(sstar, ',')) != NULL) // cut off Bayer, if trad. name
      *sp = '\0';
  }
  /* star elements from last call: */
  if (*ctx->sp.fs1mag.slast_stardata != '\0' && strcmp(ctx->sp.fs1mag.slast_starname, sstar) == 0) {
    strcpy(srecord, ctx->sp.fs1mag.slast_stardata);
    retc = fixstar_cut_string(ctx, srecord, star, &stardata, serr);
    if (retc == ERR) goto return_err;
    // magnitude V
    dparams[7] = stardata.mag;
    goto found;
  }
  /******************************************************
   * Star file
   * close to the beginning, a few stars selected by Astrodienst.
   * These can be accessed by giving their number instead of a name.
   * All other stars can be accessed by name.
   * Comment lines start with # and are ignored.
   ******************************************************/
  if ((retc = swi_fixstar_load_record(ctx, star, srecord, NULL, NULL, dparams, serr)) != OK)
    goto return_err;
  found:
  strcpy(ctx->sp.fs1mag.slast_stardata, srecord);
  strcpy(ctx->sp.fs1mag.slast_starname, sstar);
  *mag = dparams[7];
  return OK;
  return_err:
  *mag = 0;
  return retc;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar_mag(char *star, double *mag, char *serr)
{
  return swe_fixstar_mag_r(swi_default_ctx(), star, mag, serr);
}

#endif

int32 CALL_CONV swe_calc_pctr_r(swe_ctx *ctx, double tjd, int32 ipl, int32 iplctr, int32 iflag, double *xxret, char *serr)
{
  double t = 0, dt, daya[2], dtsave_for_defl = 0;
  double xx[6], xxctr[6], xxctr2[6], xx0[6], xxsv[24], xxsp[6], dx[6], xreturn[24];
  double *xs;
  int i, j, niter;
  int32 iflag2, epheflag, retc;
  struct epsilon *oe;
  if (ipl == iplctr) {
    if (serr != NULL) 
	  sprintf(serr, "ipl and iplctr (= %d) must not be identical\n", ipl);
	return ERR;
  }
  iflag = plaus_iflag(ctx, iflag, ipl, tjd, serr);
  epheflag = iflag & SEFLG_EPHMASK;
  // this fills in obliquity and nutation values in swed
  swe_calc_r(ctx, tjd + swe_deltat_ex_r(ctx, tjd, epheflag, serr), SE_ECL_NUT, iflag, xx, serr);
  iflag &= ~(SEFLG_HELCTR|SEFLG_BARYCTR);
  iflag2 = epheflag;
  iflag2 |= (SEFLG_BARYCTR|SEFLG_J2000|SEFLG_ICRS|SEFLG_TRUEPOS|SEFLG_EQUATORIAL|SEFLG_XYZ|SEFLG_SPEED);
  iflag2 |= (SEFLG_NOABERR|SEFLG_NOGDEFL);
  retc = swe_calc_r(ctx, tjd, iplctr, iflag2, xxctr, serr);
  if (retc == ERR) 
    return ERR;
  retc = swe_calc_r(ctx, tjd, ipl, iflag2, xx, serr);
  if (retc == ERR) 
    return ERR;
  for (i = 0; i <= 5; i++) {
    xx0[i] = xx[i];
    //xx[i] -= xxctr[i];
  }
  /*******************************
   * light-time geocentric       * 
   *******************************/
  if (!(iflag & SEFLG_TRUEPOS)) {
    /* number of iterations - 1 */
    niter = 1;
    if (iflag & SEFLG_SPEED) {
      /* 
       * Apparent speed is influenced by the fact that dt changes with
       * time. This makes a difference of several hundredths of an
       * arc second / day. To take this into account, we compute 
       * 1. true position - apparent position at time t - 1.
       * 2. true position - apparent position at time t.
       * 3. the difference between the two is the part of the daily motion 
       * that results from the change of dt.
       */
      for (i = 0; i <= 2; i++)
	    xxsv[i] = xxsp[i] = xx[i] - xx[i+3];
      for (j = 0; j <= niter; j++) {
        for (i = 0; i <= 2; i++) {
          dx[i] = xxsp[i];
          dx[i] -= (xxctr[i] - xxctr[i+3]);
        }
        /* new dt */
        dt = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;     
        for (i = 0; i <= 2; i++) 	/* rough apparent position at t-1 */
          xxsp[i] = xxsv[i] - dt * xx0[i+3];
      }
      /* true position - apparent position at time t-1 */
      for (i = 0; i <= 2; i++) 
        xxsp[i] = xxsv[i] - xxsp[i];
    }
    /* dt and t(apparent) */
    for (j = 0; j <= niter; j++) {
      for (i = 0; i <= 2; i++) {
        dx[i] = xx[i];
    	dx[i] -= xxctr[i];
      }
      dt = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;    
      /* new t */
      t = tjd - dt;
      dtsave_for_defl = dt;
      for (i = 0; i <= 2; i++) 		/* rough apparent position at t*/
        xx[i] = xx0[i] - dt * xx0[i+3];
    }
    /* part of daily motion resulting from change of dt */
    if (iflag & SEFLG_SPEED) {
      for (i = 0; i <= 2; i++) 
        xxsp[i] = xx0[i] - xx[i] - xxsp[i];
    }
    retc = swe_calc_r(ctx, t, iplctr, iflag2, xxctr2, serr);
    retc = swe_calc_r(ctx, t, ipl, iflag2, xx, serr);
  }
  /*******************************
   * conversion to planetocenter     * 
   *******************************/
  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR)) {
    /* subtract earth */
    for (i = 0; i <= 5; i++) 
      xx[i] -= xxctr[i]; 
    if ((iflag & SEFLG_TRUEPOS) == 0 ) {
      /* 
       * Apparent speed is also influenced by
       * the change of dt during motion.
       * Neglect of this would result in an error of several 0.01"
       */
      if (iflag & SEFLG_SPEED)
        for (i = 3; i <= 5; i++) 
          xx[i] -= xxsp[i-3]; 	
    }
  }
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /************************************
   * relativistic deflection of light *
   ************************************/
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOGDEFL))
    	/* SEFLG_NOGDEFL is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_deflect_light(ctx, xx, dtsave_for_defl, iflag);
  /**********************************
   * 'annual' aberration of light   *
   **********************************/
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOABERR)) {
    	/* SEFLG_NOABERR is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_aberr_light(xx, xxctr, iflag);
    /* 
     * Apparent speed is also influenced by
     * the difference of speed of the earth between t and t-dt. 
     * Neglecting this would involve an error of several 0.1"
     */
    if (iflag & SEFLG_SPEED) {
      for (i = 3; i <= 5; i++) 
        xx[i] += xxctr[i] - xxctr2[i];
    }
  }
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && swi_get_denum(ctx, ipl, epheflag) >= 403) {
    swi_bias(ctx, xx, t, iflag, FALSE);
  }/**/
  /* save J2000 coordinates; required for sidereal positions */
  for (i = 0; i <= 5; i++)
    xxsv[i] = xx[i];
  /************************************************
   * precession, equator 2000 -> equator of date *
   ************************************************/
  if (!(iflag & SEFLG_J2000)) {
    swi_precess(ctx, xx, tjd, iflag, J2000_TO_J);
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, xx, tjd, iflag, J2000_TO_J);
    oe = &ctx->oec;
  } else {
    oe = &ctx->oec2000;
  }
  /************************************************
   * nutation                                     *
   ************************************************/
  if (!(iflag & SEFLG_NONUT))
    swi_nutate(ctx, xx, iflag, FALSE);
  /* now we have equatorial cartesian coordinates; save them */
  for (i = 0; i <= 5; i++)
    xreturn[18+i] = xx[i];
  /************************************************
   * transformation to ecliptic.                  *
   * with sidereal calc. this will be overwritten *
   * afterwards.                                  *
   ************************************************/
  swi_coortrf2(xx, xx, oe->seps, oe->ceps);
  if (iflag & SEFLG_SPEED)
    swi_coortrf2(xx+3, xx+3, oe->seps, oe->ceps);
  if (!(iflag & SEFLG_NONUT)) {
    swi_coortrf2(xx, xx, ctx->nut.snut, ctx->nut.cnut);
    if (iflag & SEFLG_SPEED)
      swi_coortrf2(xx+3, xx+3, ctx->nut.snut, ctx->nut.cnut);
  }
  /* now we have ecliptic cartesian coordinates */
  for (i = 0; i <= 5; i++)
    xreturn[6+i] = xx[i];
  /************************************
   * sidereal positions               *
   ************************************/
  if (iflag & SEFLG_SIDEREAL) {
    /* project onto ecliptic t0 */
    if (ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) {
      if (swi_trop_ra2sid_lon(ctx, xxsv, xreturn+6, xreturn+18, iflag) != OK)
	return ERR;
    /* project onto solar system equator */
    } else if (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE) {
      if (swi_trop_ra2sid_lon_sosy(ctx, xxsv, xreturn+6, iflag) != OK)
        return ERR;
    } else {
    /* traditional algorithm */
      swi_cartpol_sp(xreturn+6, xreturn); 
      /* note, swi_get_ayanamsa_ex(ctx) disturbs present calculations, if sun is calculated with 
       * TRUE_CHITRA ayanamsha, because the ayanamsha also calculates the sun.
       * Therefore current values are saved... */
      for (i = 0; i < 24; i++)
        xxsv[i] = xreturn[i];
      if (swi_get_ayanamsa_with_speed(ctx, tjd, iflag, daya, serr) == ERR)
        return ERR;
      /* ... and restored */
      for (i = 0; i < 24; i++)
        xreturn[i] = xxsv[i];
      xreturn[0] -= daya[0] * DEGTORAD;
      xreturn[3] -= daya[1] * DEGTORAD;
      swi_polcart_sp(xreturn, xreturn+6); 
    }
  } 
  /************************************************
   * transformation to polar coordinates          *
   ************************************************/
  swi_cartpol_sp(xreturn+18, xreturn+12); 
  swi_cartpol_sp(xreturn+6, xreturn); 
  /********************** 
   * radians to degrees *
   **********************/
  for (i = 0; i < 2; i++) {
    xreturn[i] *= RADTODEG;		/* ecliptic */
    xreturn[i+3] *= RADTODEG;
    xreturn[i+12] *= RADTODEG;	/* equator */
    xreturn[i+15] *= RADTODEG;
  }
  // return values
  if (iflag & SEFLG_EQUATORIAL) {
    xs = xreturn+12;	/* equatorial coordinates */
  } else {
    xs = xreturn;	/* ecliptic coordinates */
  }
  if (iflag & SEFLG_XYZ)
    xs = xs+6;		/* cartesian coordinates */
  for (i = 0; i < 6; i++)
    xxret[i] = xs[i];
  if (!(iflag & SEFLG_SPEED)) {
    for (i = 3; i < 6; i++)
      xxret[i] = 0;
  }
  if (iflag & SEFLG_RADIANS) {
    for (i = 0; i < 2; i++)
      xxret[i] *= DEGTORAD;
    if (iflag & SEFLG_SPEED) {
      for (i = 3; i < 5; i++) 
	xxret[i] *= DEGTORAD;
    }
  }
  if (retc == ERR)
    return ERR;
  return(iflag);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_calc_pctr(double tjd, int32 ipl, int32 iplctr, int32 iflag, double *xxret, char *serr)
{
  return swe_calc_pctr_r(swi_default_ctx(), tjd, ipl, iplctr, iflag, xxret, serr);
}

// returns data from internal file structures sweph.fidat
// used in last call to swe_calc() or swe_fixstar()
// ifno = 0     planet file sepl_xxx, used for Sun .. Pluto, or jpl file
// ifno = 1     moon file semo_xxx
// ifno = 2     main asteroid file seas_xxx  if such an object was computed
// ifno = 3     other asteroid or planetary moon file, if such object was computed
// ifno = 4     star file
// Return value: full file pathname, or NULL if no data
// tfstart = start date of file,
// tfend   = end data of fila,
// denum   = jpl ephemeris number 406 or 431 from which file was derived
// all three return values are zero for a jpl file or a star file.
const char * CALL_CONV swe_get_current_file_data_r(swe_ctx *ctx, int ifno, double *tfstart, double *tfend, int *denum)
{
  if (ifno < 0 || ifno > 4) return NULL;
  struct file_data *pfp = &ctx->fidat[ifno];
  if (strlen(pfp->fnam) == 0) return NULL;
  *tfstart = pfp->tfstart;
  *tfend = pfp->tfend;
  *denum = pfp->sweph_denum;
  return pfp->fnam;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
const char * CALL_CONV swe_get_current_file_data(int ifno, double *tfstart, double *tfend, int *denum)
{
  return swe_get_current_file_data_r(swi_default_ctx(), ifno, tfstart, tfend, denum);
}

#define CROSS_PRECISION (1 / 3600000.0) 	// one milliarc sec

/*************************************************
 * compute Sun'scrossing over some longitude
 * flag covers the following bits as used by swe_calc():
   SEFLG_HELCTR 		0 = geocentric, SUN, 1 = heliocentric, EARTH
   SEFLG_TRUEPOS 	   	0 = apparent positions, 1 = true positions
   SEFLG_NONUT 		0 = do nutation (true equinox of date)
 * returns juldate of the next crossing, with jd > jd_et
 * The returned time is ephemeris time; to get UT we must do
 * jd_ut = jd - deltat(jd) or use swe_solcross_ut.
 * Errors are indicated by returning a jd < jd_et!
 *************************************************/
double CALL_CONV swe_solcross_r(swe_ctx *ctx, double x2cross, double jd_et, int flag, char *serr)
{
  double x[6], xlp, dist;
  double jd;
  int ipl = SE_SUN;
  /*
   * compute the SUN at start date, and then estimate the crossing date
   */
  flag |= SEFLG_SPEED;
  if (swe_calc_r(ctx, jd_et, ipl, flag, x, serr) < 0) 
    return jd_et - 1;
  xlp = 360.0 / 365.24;	/* mean solar speed */
  dist = swe_degnorm(x2cross - x[0]);
  jd = jd_et + dist / xlp;
  for(;;) {
    if (swe_calc_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return jd_et - 1;
    dist = swe_difdeg2n(x2cross, x[0]);
    jd += dist / x[3];
    if (fabs(dist) < CROSS_PRECISION) break;
  } 
  return jd;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_solcross(double x2cross, double jd_et, int flag, char *serr)
{
  return swe_solcross_r(swi_default_ctx(), x2cross, jd_et, flag, serr);
}

/*************************************************
 * compute Sun'scrossing over some longitude, in UT
 * flag covers the following bits as used by swe_calc():
   SEFLG_HELCTR 		0 = geocentric, SUN, 1 = heliocentric, EARTH
   SEFLG_TRUEPOS 	   	0 = apparent positions, 1 = true positions
   SEFLG_NONUT 		0 = do nutation (true equinox of date)
 * returns juldate of the next crossing, with jd > jd_ut
 * The returned time is universal time;
 * Errors are indicated by returning a jd < jd_ut!
 *************************************************/
double CALL_CONV swe_solcross_ut_r(swe_ctx *ctx, double x2cross, double jd_ut, int flag, char *serr)
{
  double x[6], xlp, dist;
  double jd;
  int ipl = SE_SUN;
  /*
   * compute the SUN at start date, and then estimate the crossing date
   */
  flag |= SEFLG_SPEED;
  if (swe_calc_ut_r(ctx, jd_ut, ipl, flag, x, serr) < 0) 
    return jd_ut - 1;
  xlp = 360.0 / 365.24;	/* mean solar speed */
  dist = swe_degnorm(x2cross - x[0]);
  jd = jd_ut + dist / xlp;
  for(;;) {
    if (swe_calc_ut_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return jd_ut - 1;
    dist = swe_difdeg2n(x2cross, x[0]);
    jd += dist / x[3];
    if (fabs(dist) < CROSS_PRECISION) break;
  } 
  return jd;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_solcross_ut(double x2cross, double jd_ut, int flag, char *serr)
{
  return swe_solcross_ut_r(swi_default_ctx(), x2cross, jd_ut, flag, serr);
}

/*************************************************
 * compute Moon's crossing over some longitude
 * flag covers the following bits as used by swe_calc():
   SEFLG_TRUEPOS 	   	0 = apparent positions, 1 = true positions
   SEFLG_NONUT 		0 = do nutation (true equinox of date)
 * returns juldate of the next crossing, with jd > jd_et
 * The returned time is ephemeris time; to get UT we must do
 * jd_ut = jd - deltat(jd);
 * Errors are indicated by returning a jd < jd_et!
 *************************************************/
double CALL_CONV swe_mooncross_r(swe_ctx *ctx, double x2cross, double jd_et, int flag, char *serr)
{
  double x[6], xlp, dist;
  double jd;
  int ipl = SE_MOON;
  /*
   * compute the SUN at start date, and then estimate the crossing date
   */
  flag |= SEFLG_SPEED;
  if (swe_calc_r(ctx, jd_et, ipl, flag, x, serr) < 0) 
    return jd_et - 1;
  xlp = 360.0 / 27.32;	/* mean lunar speed */
  dist = swe_degnorm(x2cross - x[0]);
  jd = jd_et + dist / xlp;
  for(;;) {
    if (swe_calc_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return jd_et - 1;
    dist = swe_difdeg2n(x2cross, x[0]);
    jd += dist / x[3];
    if (fabs(dist) < CROSS_PRECISION) break;
  } 
  return jd;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_mooncross(double x2cross, double jd_et, int flag, char *serr)
{
  return swe_mooncross_r(swi_default_ctx(), x2cross, jd_et, flag, serr);
}

/*************************************************
 * compute Moon's crossing over some longitude
 * flag covers the following bits as used by swe_calc_ut():
   SEFLG_TRUEPOS 	0 = apparent positions, 1 = true positions
   SEFLG_NONUT 		0 = do nutation (true equinox of date)
   SEFLG_SIDEREAL       0 = do tropical
 * returns juldate of the next crossing, with jd > jd_ut
 * The returned time is UT
 * Errors are indicated by returning a jd < jd_ut!
 * If sidereal is chosen, default mode is Fagan/Bradley. For different aynamshas,
 * swe_set_sid_mode() must be called first.
 *************************************************/
double CALL_CONV swe_mooncross_ut_r(swe_ctx *ctx, double x2cross, double jd_ut, int flag, char *serr)
{
  double x[6], xlp, dist;
  double jd;
  int ipl = SE_MOON;
  /*
   * compute the SUN at start date, and then estimate the crossing date
   */
  flag |= SEFLG_SPEED;
  if (swe_calc_ut_r(ctx, jd_ut, ipl, flag, x, serr) < 0) 
    return jd_ut - 1;
  xlp = 360.0 / 27.32;	/* mean lunar speed */
  dist = swe_degnorm(x2cross - x[0]);
  jd = jd_ut + dist / xlp;
  for(;;) {
    if (swe_calc_ut_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return jd_ut - 1;
    dist = swe_difdeg2n(x2cross, x[0]);
    jd += dist / x[3];
    if (fabs(dist) < CROSS_PRECISION) break;
  } 
  return jd;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_mooncross_ut(double x2cross, double jd_ut, int flag, char *serr)
{
  return swe_mooncross_ut_r(swi_default_ctx(), x2cross, jd_ut, flag, serr);
}

/*************************************************
 * compute next Moon crossing over node, by finding zero latitude crossing
 * returns juldate of the next crossing, with jd > jd_et
 * The returned time is ephemeris time; to get UT we must do
 * jd_ut = jd - deltat(jd);
 * Errors are indicated by returning a jd < jd_et!
 *************************************************/
double CALL_CONV swe_mooncross_node_r(swe_ctx *ctx, double jd_et, int flag, double *xlon, double *xla, char *serr)
{
  double x[6], xlat, dist;
  double jd;
  int ipl = SE_MOON;
  flag |= SEFLG_SPEED;
  if (swe_calc_r(ctx, jd_et, ipl, flag, x, serr) < 0) 
    return jd_et - 1;
  xlat = x[1];
  jd = jd_et + 1;
  for(;;) {	// get to sign change
    if (swe_calc_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return jd_et - 1;
    if ((x[1] >= 0 && xlat < 0) || (x[1] < 0 && xlat > 0)) 
      break;
    jd += 1;
  }
  dist = x[1];
  for(;;) {
    jd -= dist / x[4];
    if (swe_calc_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return jd_et - 1;
    dist = x[1];
    if (fabs(dist) < CROSS_PRECISION) {
      *xlon = x[0];
      *xla = x[1];
      break;
    }
  } 
  return jd;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_mooncross_node(double jd_et, int flag, double *xlon, double *xla, char *serr)
{
  return swe_mooncross_node_r(swi_default_ctx(), jd_et, flag, xlon, xla, serr);
}
/*************************************************
 * compute next Moon crossing over node in UT, by finding zero latitude crossing
 * returns juldate of the next crossing, with jd > jd_ut
 * The returned time is universal time;
 * Errors are indicated by returning a jd < jd_ut!
 *************************************************/
double CALL_CONV swe_mooncross_node_ut_r(swe_ctx *ctx, double jd_ut, int flag, double *xlon, double *xla, char *serr)
{
  double x[6], xlat, dist;
  double jd;
  int ipl = SE_MOON;
  flag |= SEFLG_SPEED;
  if (swe_calc_ut_r(ctx, jd_ut, ipl, flag, x, serr) < 0) 
    return jd_ut - 1;
  xlat = x[1];
  jd = jd_ut + 1;
  for(;;) {	// get to sign change
    if (swe_calc_ut_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return jd_ut - 1;
    if ((x[1] >= 0 && xlat < 0) || (x[1] < 0 && xlat > 0)) 
      break;
    jd += 1;
  }
  dist = x[1];
  for(;;) {
    jd -= dist / x[4];
    if (swe_calc_ut_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return jd_ut - 1;
    dist = x[1];
    if (fabs(dist) < CROSS_PRECISION) {
      *xlon = x[0];
      *xla = x[1];
      break;
    }
  } 
  return jd;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_mooncross_node_ut(double jd_ut, int flag, double *xlon, double *xla, char *serr)
{
  return swe_mooncross_node_ut_r(swi_default_ctx(), jd_ut, flag, xlon, xla, serr);
}

/*************************************************
 * compute a planets heliocentric crossing over some longitude
 * returns juldate of the next crossing, with jd > jd_et if dir >= 0,
 * or the previous crossing, if dir < 0.
 * The returned time is ephemeris time.
 * Errors are indicated by returning ERR;
 * This should only be used for rought house entry or exit times.
 *************************************************/
int32 CALL_CONV swe_helio_cross_r(swe_ctx *ctx, int ipl, double x2cross, double jd_et, int iflag, int dir, double *jd_cross, char *serr)
{
  double x[6], xlp, dist;
  double jd;
  int flag = iflag | SEFLG_SPEED | SEFLG_HELCTR;
  if (ipl == SE_SUN 
    || ipl == SE_MOON 
    || (ipl >= SE_MEAN_NODE && ipl <= SE_OSCU_APOG)
    || (ipl >= SE_INTP_APOG && ipl < SE_NPLANETS)
  ) {
    char snam[AS_MAXCH];
    swe_get_planet_name_r(ctx, ipl, snam);
    if (serr != NULL) sprintf(serr, "swe_helio_cross: not possible for object %d = %s", ipl, snam);
    return ERR;
  }
  if (swe_calc_r(ctx, jd_et, ipl, flag, x, serr) < 0) 
    return ERR;
  xlp = x[3];	
  if (ipl == SE_CHIRON)
    xlp = 0.01971;	// use mean speeed
  dist = swe_degnorm(x2cross - x[0]);
  if (dir >= 0) {
    jd = jd_et + dist / xlp;
  } else {
    dist = 360.0 - dist;
    jd = jd_et - dist / xlp;
  }
  for(;;) {
    if (swe_calc_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return ERR;
    dist = swe_difdeg2n(x2cross, x[0]);
    jd += dist / x[3];
    if (fabs(dist) < CROSS_PRECISION) break;
  } 
  *jd_cross = jd;
  return OK;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_helio_cross(int ipl, double x2cross, double jd_et, int iflag, int dir, double *jd_cross, char *serr)
{
  return swe_helio_cross_r(swi_default_ctx(), ipl, x2cross, jd_et, iflag, dir, jd_cross, serr);
}

/*************************************************
 * compute a planets heliocentric crossing over some longitude
 * returns juldate of the next crossing, with jd > jd_ut if dir >= 0,
 * or the previous crossing, if dir < 0.
 * The returned time is Universal time.
 * Errors are indicated by returning ERR;
 * This should only be used for rought house entry or exit times.
 *************************************************/
int32 CALL_CONV swe_helio_cross_ut_r(swe_ctx *ctx, int ipl, double x2cross, double jd_ut, int iflag, int dir, double *jd_cross, char *serr)
{
  double x[6], xlp, dist;
  double jd;
  int flag = iflag | SEFLG_SPEED | SEFLG_HELCTR;
  if (ipl == SE_SUN 
    || ipl == SE_MOON 
    || (ipl >= SE_MEAN_NODE && ipl <= SE_OSCU_APOG)
    || (ipl >= SE_INTP_APOG && ipl < SE_NPLANETS)
  ) {
    char snam[AS_MAXCH];
    swe_get_planet_name_r(ctx, ipl, snam);
    if (serr != NULL) sprintf(serr, "swe_helio_cross: not possible for object %d = %s", ipl, snam);
    return ERR;
  }
  if (swe_calc_ut_r(ctx, jd_ut, ipl, flag, x, serr) < 0) 
    return ERR;
  xlp = x[3];	
  if (ipl == SE_CHIRON)
    xlp = 0.01971;	// use mean speeed
  dist = swe_degnorm(x2cross - x[0]);
  if (dir >= 0) {
    jd = jd_ut + dist / xlp;
  } else {
    dist = 360.0 - dist;
    jd = jd_ut - dist / xlp;
  }
  for(;;) {
    if (swe_calc_ut_r(ctx, jd, ipl, flag, x, serr) < 0) 
      return ERR;
    dist = swe_difdeg2n(x2cross, x[0]);
    jd += dist / x[3];
    if (fabs(dist) < CROSS_PRECISION) break;
  } 
  *jd_cross = jd;
  return OK;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_helio_cross_ut(int ipl, double x2cross, double jd_ut, int iflag, int dir, double *jd_cross, char *serr)
{
  return swe_helio_cross_ut_r(swi_default_ctx(), ipl, x2cross, jd_ut, iflag, dir, jd_cross, serr);
}
