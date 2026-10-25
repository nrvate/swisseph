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
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>	/* stat(): bug 118 */
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
		AS_BOOL do_save, int32 *iephe_used,
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
#ifndef SWE_UPSTREAM_COMPAT
static void file_nominal_span(double tjd, int ipli, const char *fname,
                              double *t0, double *t1);
static void file_spare_drop(swe_ctx *ctx, int ifno);
#endif
static void embofs(double * SWI_RESTRICT xemb, const double * SWI_RESTRICT xmoon);
static int app_pos_etc_plan(swe_ctx *ctx, int ipli, int iplmoon, int32 iflag, char *serr);
static int app_pos_etc_plan_osc(swe_ctx *ctx, int ipl, int ipli, int32 iflag, char *serr);
static int swi_orbel_to_state(swe_ctx *ctx, double tjd, const double *el, double mu, double tequ, AS_BOOL of_date, double *xp, char *serr);
static int app_pos_etc_orbel(swe_ctx *ctx, double tjd, const double *el, double mu, double tequ, AS_BOOL of_date, int32 iplctr, int32 iflag, char *serr);
static int app_pos_etc_sun(swe_ctx *ctx, int32 iflag, char *serr);
static int app_pos_etc_moon(swe_ctx *ctx, int32 iflag, char *serr);
static int app_pos_etc_sbar(swe_ctx *ctx, int32 iflag, char *serr);
extern int swi_plan_for_osc_elem(swe_ctx *ctx, int32 iflag, double tjd, double *xx);
static AS_BOOL swi_refuse_downgrade(swe_ctx *ctx, int32 wanted, int32 got, double tjd, char *serr);
#ifndef SWE_UPSTREAM_COMPAT
static AS_BOOL swi_refuse_moshmoon(swe_ctx *ctx, double tjd, char *serr);
static AS_BOOL swi_refuse_mosh_bary(swe_ctx *ctx, int32 wanted, int32 got_iflag, double tjd, char *serr);
#endif
static void swi_close_keep_topo_etc(swe_ctx *ctx, AS_BOOL forget_denum);
static int app_pos_etc_mean(swe_ctx *ctx, int ipl, int32 iflag, char *serr);
static void nut_matrix(struct nut *nu, struct epsilon *oec); 
static void calc_epsilon(swe_ctx *ctx, double tjd, int32 iflag, struct epsilon *e);
static int lunar_osc_elem(swe_ctx *ctx, double tjd, int ipl, int32 iflag, char *serr);
static int intp_apsides(swe_ctx *ctx, double tjd, int ipl, int32 iflag, char *serr); 
#ifndef SWE_UPSTREAM_COMPAT
static void add_aberr_rate(double *xx, const double *xobs, const double *xobs2);
static void earth_at_lt(swe_ctx *ctx, double teval, double t, const double *xeteval, const double *xe, double *xout);
static void lt_rate_exact(const double *xx, const double *xo, double *xxsp);
static void earth_place_at_lt(swe_ctx *ctx, double teval, double t, int *have_acc,
                              double *acc, double *xe);
static int observer_accel(swe_ctx *ctx, double tjd, int32 iflag, double *acc, char *serr);
static int observer_at_lt(swe_ctx *ctx, double teval, double t, int32 iflag,
                          const double *xobs, const double *xeteval,
                          const double *xe, double *xobs2, char *serr);
#endif
static double meff(double r);
#ifndef SWE_UPSTREAM_COMPAT
static double meff_slope(double r);
#endif
static void denormalize_positions(double *x0, double *x1, double *x2);
static void calc_speed(double *x0, double *x1, double *x2, double dt);
static int32 plaus_iflag(swe_ctx *ctx, int32 iflag, int32 ipl, double tjd, char *serr);
static int app_pos_rest(swe_ctx *ctx, struct plan_data *pdp, double tdate, int32 iflag, 
    double *xx, double *x2000, struct epsilon *oe, char *serr);
static int open_jpl_file(swe_ctx *ctx, double *ss, char *fname, char *fpath, char *serr);
static void free_planets(swe_ctx *ctx);
#ifndef SWE_UPSTREAM_COMPAT
static void planets_forget_instants(swe_ctx *ctx);
static void ephe_switch_forget(swe_ctx *ctx);
#endif
static void ephe_path_read_moon(swe_ctx *ctx);
#ifndef SWE_UPSTREAM_COMPAT
static void file_close_open(swe_ctx *ctx, int ifno);
#endif
static void ctx_release(swe_ctx *ctx);
static void dpsi_deps_free(swe_ctx *ctx);

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
    /* ISO C has no function-pointer-to-void* conversion; POSIX requires this
     * one, since dladdr() takes the address as void *. Kept visible rather
     * than laundered through uintptr_t, which is no more conforming. */
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    if (dladdr((void *)swe_version, &dli) != 0) {
    #pragma GCC diagnostic pop
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
static int32 swe_calc_r_body(swe_ctx *ctx, double tjd, int ipl, int32 iflag,
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
  /* ⚠️ Same precedence as plaus_iflag(): JPL over Swiss over Moshier. That
   * is the order swecalc() actually dispatches in, and this value is what
   * the strict guard at the end of this function compares the answer
   * against. Upstream read the bits the other way round here, which only
   * ever affected which save area got cleared -- until the guard: a request
   * spelled SEFLG_SWIEPH|SEFLG_MOSEPH was answered by Swiss, as always, and
   * then refused as a Moshier request answered by Swiss. */
  epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag & SEFLG_JPLEPH) {
    epheflag = SEFLG_JPLEPH;
  } else if (epheflag & SEFLG_SWIEPH) {
    epheflag = SEFLG_SWIEPH;
  } else if (epheflag & SEFLG_MOSEPH) {
    epheflag = SEFLG_MOSEPH;
  } else  {
    epheflag = SEFLG_SWIEPH;
  }
#ifndef SWE_UPSTREAM_COMPAT
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL
      && swi_ephe_path_none(ctx)) {	/* bug 110 */
#else
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL) {
#endif
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_calc() or swe_calc_ut()");
  }
  if (ctx->last_epheflag != epheflag) {
#ifndef SWE_UPSTREAM_COMPAT
    ephe_switch_forget(ctx);	/* bug 103: the files stay open */
    ctx->last_epheflag = epheflag;
#else
#ifdef SWE_UPSTREAM_COMPAT
    free_planets(ctx);
#endif
    /* close and free ephemeris files */
    if (ipl != SE_ECL_NUT) {  /* because file will not be reopened with this ipl */
#ifndef SWE_UPSTREAM_COMPAT
      /* ⚠️ Only where the files are closed too. Upstream freed the planets'
       * data for SE_ECL_NUT as well, which wiped every file's constants
       * (tfstart, dseg, lndx0) while leaving the files open and
       * last_epheflag unchanged -- so the next call under the old flag found
       * its file open, skipped read_const(), and looked a segment up with
       * dseg = 0: Sun (Swiss), SE_ECL_NUT (Moshier), Mars (Swiss) answered
       * "sepl_18.se1 is damaged", and a double-to-int overflow got there.
       * Nutation reads none of this data, and every save area keys on the
       * flags it was computed under, so leaving it all in place is safe. */
      free_planets(ctx);
#endif
      if (ctx->jpl_file_is_open) {
	swi_close_jpl_file(ctx);
	ctx->jpl_file_is_open = FALSE;
      }
      swi_close_ephe_files(ctx, KEEP_DENUM);
      ctx->last_epheflag = epheflag;
    }
#endif
  }
  /* high precision speed prevails fast speed */
  if ((iflag & SEFLG_SPEED3) && (iflag & SEFLG_SPEED))
    iflag = iflag & ~SEFLG_SPEED3;
  if (iflag & SEFLG_SPEED3) 
    use_speed3 = TRUE;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 90: SEFLG_SPEED3 ran calc_speed() over SE_ECL_NUT's four
   * angles, which are not a position: x[3], the nutation in obliquity
   * (-1.6026e-3 deg), came back as the true obliquity's rate (-6.06e-6),
   * x[4] and x[5] as rates of the mean obliquity and the nutation in
   * longitude, and denormalize_positions() read x0[12] and x2[12],
   * never written (valgrind: 576 errors). The nutation has no speed; it is
   * answered as without a speed flag (x[3] the nutation, x[4] = x[5] = 0). */
  if (ipl == SE_ECL_NUT)
    use_speed3 = FALSE;
#endif
  /* topocentric with SEFLG_SPEED is not good if aberration is included. 
   * in such cases we calculate speed from three positions */
#ifdef SWE_UPSTREAM_COMPAT
  if ((iflag & SEFLG_SPEED) && (iflag & SEFLG_TOPOCTR) && !(iflag & SEFLG_NOABERR)) 
    use_speed3 = TRUE;
#endif
  /* Not any more (bugs 31, 32, 34): the analytic topocentric speed now
   * carries the light time's rate and the observer's turning, and is the
   * rate of the position to ~1e-6 deg/day, where the three positions 8.64 s
   * apart carry their 0.1 mas roughness, ~3e-5 on the Moon, at three times
   * the cost. SEFLG_SPEED3 still asks for them. */
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
#ifndef SWE_UPSTREAM_COMPAT
  /* ⚠️ Not below SE_ECL_NUT: no body is numbered there, and ipl * 100
   * overflows int32 for ids under about -21.5 million, which a caller
   * passing an unchecked id reaches (UBSan, from the ephemeris server's
   * fuzz). -1 keeps upstream's arithmetic, flag clearing included; lower
   * ids fail as an illegal planet number either way. */
  if ((iflag & SEFLG_CENTER_BODY) && ipl >= SE_ECL_NUT && ipl <= SE_PLUTO && (iflag & SEFLG_TEST_PLMOON) != SEFLG_TEST_PLMOON) {
#else
  if ((iflag & SEFLG_CENTER_BODY) && ipl <= SE_PLUTO && (iflag & SEFLG_TEST_PLMOON) != SEFLG_TEST_PLMOON) {
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  if (sd->tsave == tjd && ipl == sd->ipl && iplmoon == 0) {	/* bug 71 */
#else
  if (sd->tsave == tjd && tjd != 0 && ipl == sd->ipl && iplmoon == 0) {
#endif
    if ((sd->iflgsave & ~SEFLG_COORDSYS) == (iflag & ~SEFLG_COORDSYS)) 
      goto end_swe_calc;
  }
  /* 
   * otherwise, new position must be computed 
   */
#ifndef SWE_UPSTREAM_COMPAT
  ctx->moshmoon_used = FALSE;	/* bug 120 */
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
	/* Upstream bug 54: the 0.1 day step above, for every ephemeris, left
	 * the h^2 term in the speed: 1.4e-3 deg/day on the apogee, 3.5e-5 on the
	 * node (against five-point differences, 8 epochs). Their positions
	 * are smooth now (bug 51), so the step is where that term and the
	 * noise balance, 2^-9 day: 1e-7..4e-7 deg/day on the apogee with Swiss
	 * files and de200, 1e-6 with Moshier; 1e-8 on the node. */
	dt = NODE_SPEED3_INTV;
#else
	dt = NODE_CALC_INTV_MOSH;
#endif
	break;
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 45: 8.64 s apart, the positions' 1.3 mas jitter was
       * 1.5e-3 deg/day in the perigee's speed (2.6e-6 once smooth). The
       * step of their own speed, 2^-8 day. */
      case SE_INTP_APOG:
      case SE_INTP_PERG:
	dt = INTP_APS_SPEED_INTV;
	break;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 139: with the fallback allowed, a Swiss or JPL barycentric
   * request whose files are missing fell to Moshier past swecalc()'s
   * refusal of a barycentric Moshier one, and was answered with Moshier's
   * heliocentric place (its origin is the Sun) under SEFLG_BARYCTR |
   * SEFLG_MOSEPH: Mars at 2000 Jul 1.5 0.24 deg, 7.2e-3 AU off the files'
   * barycentric Mars, the Moon 0.37 deg. Refused now as that request is,
   * with its message; under the strict setting with the downgrade's, as
   * before. On swecalc()'s flag, before bug 120's below: a Moon from
   * Moshier with the files' Earth is a barycentric place. The save area
   * cannot answer it later: its flag is no request's (a request naming
   * Moshier changes the ephemeris, which forgets the save areas). */
  if (swi_refuse_mosh_bary(ctx, epheflag, sd->iflgsave, tjd, serr))
    goto return_error;
  /* Upstream bug 120: the Moon, or the Earth through it, from Moshier for
   * want of the semo file. Refused under the strict setting when the
   * answer depends on it: the Moon and the Earth themselves, and any body
   * seen from the Earth (heliocentric and barycentric planets read the
   * Earth too, but do not use it). With the fallback allowed the Moon's
   * flag says Moshier, which is what answered; a planet keeps Swiss, its
   * own place and the EMB being the files' and only the Earth's Moon
   * offset, 1/82 of the Moon's error, Moshier's. */
  if (ctx->moshmoon_used && (sd->iflgsave & SEFLG_SWIEPH)
      && (ipl == SE_MOON || ipl == SE_EARTH
          || !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR)))) {
    if (swi_refuse_moshmoon(ctx, tjd, serr)) {
      sd->tsave = SWI_TJD_NEVER;	/* no cached answer to come back with */
      goto return_error;
    }
    if (ipl == SE_MOON)
      sd->iflgsave = (sd->iflgsave & ~SEFLG_EPHMASK) | SEFLG_MOSEPH;
  }
#endif
  end_swe_calc:
  if (iflag & SEFLG_EQUATORIAL) {
    xs = sd->xsaves+12;	/* equatorial coordinates */
  } else {
    xs = sd->xsaves;	/* ecliptic coordinates */
  }
  if (iflag & SEFLG_XYZ)
    xs = xs+6;		/* cartesian coordinates */
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 90 (part 2): the four angles are written to the first
   * four slots of the save area alone, and SEFLG_EQUATORIAL or SEFLG_XYZ
   * sent the read to slots 12.. or 6.. of the asteroids' shared area: the
   * last such body's equatorial or cartesian place (Cupido's RA 44.77 deg
   * as the "true obliquity" at 2460000.5, 23.4384), or zeros on a fresh
   * context. Always the angles. */
  if (ipl == SE_ECL_NUT)
    xs = sd->xsaves;
#endif
  if (ipl == SE_ECL_NUT) {
    i = 4;
  } else {
    i = 3;
  }
  for (j = 0; j < i; j++)
    x[j] = *(xs + j);
  for (j = i; j < 6; j++)
    x[j] = 0;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 90: SEFLG_SPEED copied xs[3..5] over x[3] (the same
   * nutation) and x[4], x[5], which for SE_ECL_NUT are no one's: the
   * asteroid save area it shares, stale. */
  if ((iflag & (SEFLG_SPEED3 | SEFLG_SPEED)) && ipl != SE_ECL_NUT) {
#else
  if (iflag & (SEFLG_SPEED3 | SEFLG_SPEED)) {
#endif
    for (j = 3; j < 6; j++)
      x[j] = *(xs + j);
  }
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
  /* The substitution paths under swecalc() all converge here, with the
   * ephemeris they actually used sitting in iflag. (The fixed-star functions
   * do not pass through here; they carry the same check themselves.) */
  if (swi_refuse_downgrade(ctx, epheflag, iflag & SEFLG_EPHMASK, tjd, serr))
    goto return_error;
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

int32 CALL_CONV swe_calc_r(swe_ctx *ctx, double tjd, int ipl, int32 iflag,
	double *xx, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_calc_r_body(ctx, tjd, ipl, iflag, xx, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_calc(double tjd, int ipl, int32 iflag, 
	double *xx, char *serr)
{
  return swe_calc_r(swi_default_ctx(), tjd, ipl, iflag, xx, serr);
}

static int32 swe_calc_ut_r_body(swe_ctx *ctx, double tjd_ut, int32 ipl, int32 iflag,
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

int32 CALL_CONV swe_calc_ut_r(swe_ctx *ctx, double tjd_ut, int32 ipl, int32 iflag,
	double *xx, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_calc_ut_r_body(ctx, tjd_ut, ipl, iflag, xx, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_calc_ut(double tjd_ut, int32 ipl, int32 iflag, 
	double *xx, char *serr)
{
  return swe_calc_ut_r(swi_default_ctx(), tjd_ut, ipl, iflag, xx, serr);
}

/* Position and speed from caller-supplied orbital elements, the reverse of
 * swe_get_orbital_elements(). The apparent-position machinery is the
 * fictitious bodies' (app_pos_etc_plan_osc, above in swecalc's flow); the
 * elements and the two-body mu come from the caller and the library's
 * constants instead of seorbel.txt. See swephexp.h for the contract. */
static int32 swe_calc_orbel_r_body(swe_ctx *ctx, double tjd_et, const double *el,
        int32 iplctr, int32 equinox, double equinoxJd, int32 iflag,
        double *xx, char *serr)
{
  int i, j;
  int32 epheflag, epheflag_asked, iflgsave = iflag;	/* bug 141 */
  double tequ, mu;
  double x[24], *xs;
  /* The scratch body: the fictitious bodies compute into SEI_ANYBODY the
   * same way, and nothing here outlives the call. */
  struct plan_data *pdp = &ctx->pldat[SEI_ANYBODY];
  double *xp = pdp->xreturn;

  if (serr != NULL)
    *serr = '\0';
  for (i = 0; i < 24; i++)
    x[i] = 0;
  /* Caller-supplied elements have no JPL Horizons meaning: the mode's
   * corrections belong to the ephemeris' own bodies. Strip before
   * plaus_iflag(), which would otherwise try to load EOP data for them. */
  iflag &= ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
  /* SEFLG_SPEED3 wants three positions; the two-body speed is already the
   * exact derivative of the model the elements define. */
  if (iflag & SEFLG_SPEED3) {
    if (serr != NULL)
      strcpy(serr, "SEFLG_SPEED3 is not supported for orbital elements: SEFLG_SPEED is the exact derivative of the two-body model.");
    goto return_error;
  }
  /* cartesian flag excludes radians flag, as in swe_calc() */
  if ((iflag & SEFLG_XYZ) && (iflag & SEFLG_RADIANS))
    iflag = iflag & ~SEFLG_RADIANS;
  /* centre and mu: pure two-body, the body massless, GM from this
   * library's own constants (sweph.h), as seorbel.txt's fictitious bodies
   * use KGAUSS/KGAUSS_GEO whatever ephemeris answers the centre. */
  if (iplctr == SE_SUN) {
    mu = KGAUSS * KGAUSS;
  } else if (iplctr == SE_EARTH) {
    mu = KGAUSS_EARTH * KGAUSS_EARTH;
  } else {
    if (serr != NULL)
      strcpy(serr, "orbital elements: centre must be SE_SUN or SE_EARTH.");
    goto return_error;
  }
  /* the elements' equinox, as seorbel.txt spells its own */
  switch (equinox) {
    case SE_ORBEL_EQ_J2000:
      tequ = J2000;
      break;
    case SE_ORBEL_EQ_B1950:
      tequ = B1950;
      break;
    case SE_ORBEL_EQ_J1900:
      tequ = J1900;
      break;
    case SE_ORBEL_EQ_DATE:
      tequ = tjd_et;
      break;
    case SE_ORBEL_EQ_EXPLICIT:
      if (!(equinoxJd > 0) || !(equinoxJd == equinoxJd)) {
        if (serr != NULL)
          strcpy(serr, "orbital elements: explicit equinox needs a positive Julian day.");
        goto return_error;
      }
      tequ = equinoxJd;
      break;
    default:
      if (serr != NULL)
        strcpy(serr, "orbital elements: unknown equinox.");
      goto return_error;
  }
  /* The ephemeris setup, as swe_calc() does it: warn without a path, then
   * resolve the ephemeris bits' precedence (JPL over Swiss over Moshier). */
#ifndef SWE_UPSTREAM_COMPAT
  if (swi_init_swed_if_start(ctx) == 1 && !(iflag & SEFLG_MOSEPH) && serr != NULL
      && swi_ephe_path_none(ctx)) {	/* bug 110 */
#else
  if (swi_init_swed_if_start(ctx) == 1 && !(iflag & SEFLG_MOSEPH) && serr != NULL) {
#endif
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_calc() or swe_calc_orbel()");
    /* Upstream bug 112 (this fork's swe_calc_orbel()): the warning was a
     * refusal, so with no path at all the first call returned -1 where
     * swe_calc() tries the default path and answers from the files, from
     * Moshier with its "using Moshier" note when the fallback is allowed,
     * or refuses only when it is not. A warning now, as swe_calc()'s is;
     * main_planet() below decides, as for swe_calc(). Unguarded: this
     * function is not upstream's, and the compat build's swe_calc() warns
     * and carries on too, so there is no upstream behaviour to restore. */
  }
  epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag & SEFLG_JPLEPH) {
    epheflag = SEFLG_JPLEPH;
  } else if (epheflag & SEFLG_SWIEPH) {
    epheflag = SEFLG_SWIEPH;
  } else if (epheflag & SEFLG_MOSEPH) {
    epheflag = SEFLG_MOSEPH;
  } else  {
    epheflag = SEFLG_SWIEPH;
    iflag |= SEFLG_SWIEPH;
  }
  epheflag_asked = epheflag;	/* for the strict check, bug 112 */
  /* Bug 112: and the path named on first use, as swe_calc_r() names it --
   * which this function never did, so in a build without bug 107's
   * naming at the first file it never read SE_EPHE_PATH at all */
  if (epheflag != SEFLG_MOSEPH && !ctx->ephe_path_is_set && !ctx->jpl_file_is_open)
#ifndef SWE_UPSTREAM_COMPAT
    swi_set_ephe_path_implicit(ctx);	/* bug 103 */
#else
    SWI_CFG_LOCAL(ctx, swe_set_ephe_path_r(ctx, NULL));
#endif
  iflag = plaus_iflag(ctx, iflag, -1, tjd_et, serr);
  /* Upstream bug 135 (this fork's swe_calc_orbel()): swecalc() refuses
   * every barycentric Moshier request, and this function, which does not
   * pass through it, answered: the heliocentric place (Moshier's origin is
   * the Sun) under a return flag with SEFLG_BARYCTR set, 5.7e-3 AU off the
   * Swiss files' barycentric one, 0.29 deg at a = 1.2 AU. Refused now with
   * swecalc()'s message, on the same normalised flags (plaus_iflag(): the
   * ephemeris bits' precedence, SEFLG_TOPOCTR over SEFLG_BARYCTR). A
   * Swiss or JPL request that falls to Moshier is answered as swe_calc()
   * answers it. Unguarded, as bug 112's: this function is not upstream's,
   * and the compat build's swe_calc() refuses the request too, so there is
   * no upstream behaviour to restore. G126. */
  if ((iflag & SEFLG_BARYCTR) && (iflag & SEFLG_MOSEPH)) {
    if (serr != NULL)
      strcpy(serr, "barycentric Moshier positions are not supported.");
    goto return_error;
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 88 (this fork's swe_calc_orbel()): the obliquity and
   * nutation caches of date were never brought to tjd here, as swecalc()
   * does for every body: app_pos_rest() took ctx->oec and ctx->nut as the
   * last swe_calc() left them, another instant's, and as a context's first
   * call none at all -- zeros out (sin eps = cos eps = 0), with no error. */
  if ((iflag & SEFLG_SIDEREAL) && !ctx->ayana_is_set)
    SWI_CFG_LOCAL(ctx, swe_set_sid_mode_r(ctx, SE_SIDM_FAGAN_BRADLEY, 0, 0));
  swi_check_ecliptic(ctx, tjd_et, iflag);
  swi_check_nutation(ctx, tjd_et, iflag);
  /* and the save areas were kept across a change of ephemeris, which
   * swe_calc_r() clears: on Moshier the barycentric Sun is the origin, but
   * a Swiss or JPL Sun left in SEI_SUNBARY was added to the orbit, 0.009
   * AU on a SEFLG_TRUEPOS position, its velocity 2.4e-3 deg/day on the
   * speed of an orbit with a = 0.137 AU. */
  if (ctx->last_epheflag != epheflag) {
    ephe_switch_forget(ctx);	/* bug 103: the files stay open */
    ctx->last_epheflag = epheflag;
  }
#endif
  retc_orbel:;
#ifndef SWE_UPSTREAM_COMPAT
  ctx->moshmoon_used = FALSE;	/* bug 120 */
#endif
  /* Earth (and through it the Sun) with speeds, saved as any swe_calc()
   * of the Earth would save them; app_pos_etc_orbel reads the saved
   * vectors, like app_pos_etc_plan_osc does. Re-run on every retry: the
   * saved Earth must be the ephemeris the rest of the call uses. */
  if (main_planet(ctx, tjd_et, SEI_EARTH, 0, epheflag, iflag, serr) == ERR)
    goto return_error;
  /* the ephemeris that actually answered: main_planet may have fallen back */
  iflag = ctx->pldat[SEI_EARTH].xflgs;
  epheflag = iflag & SEFLG_EPHMASK;
  i = app_pos_etc_orbel(ctx, tjd_et, el, mu, tequ, equinox == SE_ORBEL_EQ_DATE, iplctr, iflag, serr);
  if (i == ERR)
    goto return_error;
  if (i == NOT_AVAILABLE || i == BEYOND_EPH_LIMITS) {
    /* the light-time-retarded epoch fell outside the ephemeris: the same
     * redo-with-Moshier the fictitious bodies get from swecalc() */
    if (epheflag != SEFLG_MOSEPH
      && tjd_et > MOSHPLEPH_START && tjd_et < MOSHPLEPH_END) {
      iflag = (iflag & ~SEFLG_EPHMASK) | SEFLG_MOSEPH;
      epheflag = SEFLG_MOSEPH;
      if (serr != NULL && strlen(serr) + 30 < AS_MAXCH)
        strcat(serr, "\nusing Moshier eph.; ");
      goto retc_orbel;
    }
    goto return_error;
  }
  /* Bug 112: the strict fallback setting, as swe_calc_r() applies it --
   * which this function never did: with the files missing it answered
   * from Moshier whatever swe_set_ephe_fallback() said. */
  if (swi_refuse_downgrade(ctx, epheflag_asked, epheflag, tjd_et, serr))
    goto return_error;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 139: and a barycentric request fallen to Moshier, as
   * swe_calc_r() refuses it */
  if (swi_refuse_mosh_bary(ctx, epheflag_asked, iflag, tjd_et, serr))
    goto return_error;
  /* Upstream bug 120: an Earth made from Moshier's Moon, the semo file
   * missing, refused where the answer depends on it: an orbit about the
   * Earth, or one seen from it */
  if ((epheflag & SEFLG_SWIEPH)
      && (iplctr == SE_EARTH || !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR)))
      && swi_refuse_moshmoon(ctx, tjd_et, serr))
    goto return_error;
#endif
  /* same output selection as swe_calc() */
  for (j = 0; j < 24; j++)
    x[j] = xp[j];
  if (iflag & SEFLG_EQUATORIAL) {
    xs = x + 12;	/* equatorial coordinates */
  } else {
    xs = x;		/* ecliptic coordinates */
  }
  if (iflag & SEFLG_XYZ)
    xs = xs + 6;	/* cartesian coordinates */
  for (j = 0; j < 3; j++)
    xx[j] = *(xs + j);
  for (j = 3; j < 6; j++)
    xx[j] = 0;
  if (iflag & SEFLG_SPEED) {
    for (j = 3; j < 6; j++)
      xx[j] = *(xs + j);
  }
  if (iflag & SEFLG_RADIANS) {
    for (j = 0; j < 2; j++)
      xx[j] *= DEGTORAD;
    if (iflag & SEFLG_SPEED) {
      for (j = 3; j < 5; j++)
        xx[j] *= DEGTORAD;
    }
  }
  /* Upstream bug 141 (this fork's swe_calc_orbel()): the return flag was
   * the flags as the Earth's computation left them, where swe_calc()
   * returns its answer's flags with the caller's coordinate bits and,
   * when no ephemeris was named, without the default's bit: 18178 for
   * swe_calc()'s 18176 with SEFLG_BARYCTR | SEFLG_SPEED, 2 for 0, and
   * SEFLG_RADIANS dropped with SEFLG_XYZ (4354 for 12546). Now
   * swe_calc()'s statement. Unguarded, as bug 112's: this function is not
   * upstream's. G126. */
  iflag = (iflag & ~SEFLG_COORDSYS) | (iflgsave & SEFLG_COORDSYS);
  if ((iflgsave & SEFLG_EPHMASK) == 0)
    iflag &= ~SEFLG_DEFAULTEPH;
  return iflag;
  return_error:;
  for (i = 0; i <= 5; i++)
    xx[i] = 0;
  return ERR;
}

int32 CALL_CONV swe_calc_orbel_r(swe_ctx *ctx, double tjd_et, const double *el,
        int32 iplctr, int32 equinox, double equinoxJd, int32 iflag,
        double *xx, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_calc_orbel_r_body(ctx, tjd_et, el, iplctr, equinox, equinoxJd, iflag, xx, serr);
  swi_config_release(ctx);
  return ret;
}

/* UT wrapper, in the shape of swe_calc_ut_r(): delta t from the requested
 * ephemeris, recomputed if the answer came from a substituted one. */
static int32 swe_calc_orbel_ut_r_body(swe_ctx *ctx, double tjd_ut, const double *el,
        int32 iplctr, int32 equinox, double equinoxJd, int32 iflag,
        double *xx, char *serr)
{
  double deltat;
  int32 retval = OK;
  int32 epheflag = 0;
  iflag = plaus_iflag(ctx, iflag & ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX), -1, tjd_ut, serr);
  epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag == 0) {
    epheflag = SEFLG_SWIEPH;
    iflag |= SEFLG_SWIEPH;
  }
  deltat = swe_deltat_ex_r(ctx, tjd_ut, iflag, serr);
  retval = swe_calc_orbel_r(ctx, tjd_ut + deltat, el, iplctr, equinox, equinoxJd, iflag, xx, serr);
  /* if ephe required is not ephe returned, adjust delta t: */
  if ((retval & SEFLG_EPHMASK) != epheflag) {
    deltat = swe_deltat_ex_r(ctx, tjd_ut, retval, NULL);
    retval = swe_calc_orbel_r(ctx, tjd_ut + deltat, el, iplctr, equinox, equinoxJd, iflag, xx, NULL);
  }
  return retval;
}

int32 CALL_CONV swe_calc_orbel_ut_r(swe_ctx *ctx, double tjd_ut, const double *el,
        int32 iplctr, int32 equinox, double equinoxJd, int32 iflag,
        double *xx, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_calc_orbel_ut_r_body(ctx, tjd_ut, el, iplctr, equinox, equinoxJd, iflag, xx, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_calc_orbel(double tjd_et, const double *el, int32 iplctr,
        int32 equinox, double equinoxJd, int32 iflag, double *xx, char *serr)
{
  return swe_calc_orbel_r(swi_default_ctx(), tjd_et, el, iplctr, equinox, equinoxJd, iflag, xx, serr);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_calc_orbel_ut(double tjd_ut, const double *el, int32 iplctr,
        int32 equinox, double equinoxJd, int32 iflag, double *xx, char *serr)
{
  return swe_calc_orbel_ut_r(swi_default_ctx(), tjd_ut, el, iplctr, equinox, equinoxJd, iflag, xx, serr);
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
#ifndef SWE_UPSTREAM_COMPAT
    swi_set_ephe_path_implicit(ctx);	/* bug 103 */
#else
    SWI_CFG_LOCAL(ctx, swe_set_ephe_path_r(ctx, NULL));
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
	/* Upstream bug 84: the Sun was read into its slot without the slot's
	 * ephemeris, and app_pos_etc_sbar() finishes it at the Earth slot's
	 * instant, which nothing here set: on a fresh context its speeds
	 * came back NaN (sidereal, every coordinate), warm from another
	 * instant the ayanamsa and the ecliptic's turning were taken there
	 * (0.11 deg sidereal after a call 3000 days on), and a heliocentric
	 * asteroid after it, its Sun of no ephemeris, kept its barycentric
	 * place (Pallas 0.24 deg). Read as the Swiss files' branch below
	 * reads it, with the Earth: both slots at tjd, and JPL's. */
	retc = jplplan(ctx, tjd, SEI_EARTH, iflag, DO_SAVE, NULL, NULL, NULL, serr);
#else
	retc = swi_pleph(ctx, tjd, J_SUN, J_SBARY, psdp->x, serr);
#endif
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
	if (retc == ERR || retc == NOT_AVAILABLE)
	  goto return_error;
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 117: the speed was (x(t) - x(t - 0.001)) / 0.001, with
     * t - 0.001 rounded to a double: the step taken is 0.001 to half an
     * ulp of the date, 2.3e-10 day near the present, 2.3e-7 of it, so
     * 1.2e-8 deg/day of the node's 0.053 off the rate of its positions
     * (8.5e-9 at the present), and the mean apogee's 0.111 by 2.5e-8,
     * its latitude also off by half the projection's curvature times the
     * step (2.1e-8 together). Now the central difference over the
     * interval actually taken around tjd +- 2^-5 day, as swe_nod_aps()'s
     * mean points (bug 55): its truncation is 6e-12 deg/day on the
     * apogee's latitude. The position is read last, at tjd. */
    {
      double xm[6], tp = tjd + 1.0 / 32, tm = tjd - 1.0 / 32;
      if (swi_mean_node(ctx, tm, xm, serr) == ERR
          || swi_mean_node(ctx, tp, xp2 + 3, serr) == ERR)
        goto return_error;
      xp2[3] = swe_difrad2n(xp2[3], xm[0]) / (tp - tm);
      retc = swi_mean_node(ctx, tjd, xp2, serr);
      if (retc == ERR)
        goto return_error;
    }
#else
    retc = swi_mean_node(ctx, tjd, xp2, serr);
    if (retc == ERR)
      goto return_error;
    /* speed (is almost constant; variation < 0.001 arcsec) */
    retc = swi_mean_node(ctx, tjd - MEAN_NODE_SPEED_INTV, xp2+3, serr);
    if (retc == ERR)
      goto return_error;
    xp2[3] = swe_difrad2n(xp2[0], xp2[3]) / MEAN_NODE_SPEED_INTV;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 117: as the mean node's above */
    {
      double xm[6], tp = tjd + 1.0 / 32, tm = tjd - 1.0 / 32;
      if (swi_mean_apog(ctx, tm, xm, serr) == ERR
          || swi_mean_apog(ctx, tp, xp2 + 3, serr) == ERR)
        goto return_error;
      xp2[3] = swe_difrad2n(xp2[3], xm[0]) / (tp - tm);
      xp2[4] = (xp2[4] - xm[1]) / (tp - tm);
      retc = swi_mean_apog(ctx, tjd, xp2, serr);
      if (retc == ERR)
        goto return_error;
    }
#else
    retc = swi_mean_apog(ctx, tjd, xp2, serr);
    if (retc == ERR)
      goto return_error;
    /* speed (is not constant! variation ~= several arcsec) */
    retc = swi_mean_apog(ctx, tjd - MEAN_NODE_SPEED_INTV, xp2+3, serr);
    if (retc == ERR)
      goto return_error;
    for(i = 0; i <= 1; i++)
      xp2[3+i] = swe_difrad2n(xp2[i], xp2[3+i]) / MEAN_NODE_SPEED_INTV;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  memset((void *) &ctx->earth_moon, 0, sizeof(ctx->earth_moon));
  for (i = 0; i <= SEI_FILE_MAIN_AST; i++)	/* with the planets' data */
    file_spare_drop(ctx, i);
#endif
  for (i = 0; i <= SE_NPLANETS; i++) /* "<=" is correct! see decl. */
    memset((void *) &ctx->savedat[i], 0, sizeof(struct save_positions));
  /* clear node data space */
  for (i = 0; i < SEI_NNODE_ETC; i++) {
    memset((void *) &ctx->nddat[i], 0, sizeof(struct plan_data));
  }
#ifndef SWE_UPSTREAM_COMPAT
  planets_forget_instants(ctx);
#endif
}

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 71: these caches were emptied by an instant of 0, which is
 * also JD 0 (-4712), and only some readers guarded it with `tjd == 0`;
 * sweph() forced its EMB to be recomputed by setting the Earth slot's teval
 * to 0, so at tjd 0 the slot claimed JD 0 and its Earth, of whatever
 * instant, came back as the EMB, and the barycentric Sun with it. Against
 * tjd 1e-9: Mars's heliocentric osculating node 0.025 deg, its mean
 * perihelion 5.5e-3, NaN without SEFLG_SPEED after a call at J2000; in a
 * warm context swe_calc()'s barycentric Sun 15.2 deg, Mercury 2.6e-4.
 * Empty is now SWI_TJD_NEVER, which no instant equals, and the guards are
 * gone: tjd = 0 is an instant like any other. */
static void planets_forget_instants(swe_ctx *ctx)
{
  int i;
  for (i = 0; i < SEI_NPLANETS; i++)
    ctx->pldat[i].teval = SWI_TJD_NEVER;
  for (i = 0; i <= SE_NPLANETS; i++)
    ctx->savedat[i].tsave = SWI_TJD_NEVER;
  ctx->earth_moon.t = SWI_TJD_NEVER;   /* bug 66's Moon of the saved Earth */
  ctx->moshmoon_earth_t = SWI_TJD_NEVER;	/* bug 120's Earth */
  ctx->mosh_sun_bary.s[0].t = SWI_TJD_NEVER;	/* bug 102's Sun */
  ctx->mosh_sun_bary.s[1].t = SWI_TJD_NEVER;
  for (i = 0; i < 9; i++)		/* and bug 109's series */
    ctx->mosh_sun_bary.tp[i] = SWI_TJD_NEVER;
  for (i = 0; i < SEI_NNODE_ETC; i++)
    ctx->nddat[i].teval = SWI_TJD_NEVER;
}

/* Upstream bug 103: a request on another ephemeris than the last closed
 * every ephemeris file, the JPL file with them, and freed every planet's
 * file data, so alternating Swiss and de200 reopened 1.5 files a request
 * (3000 opens in 2000 Mars, 35.6 us each for 6.7). The files are those of
 * the path, whatever ephemeris is asked for, and every position cache is
 * keyed on the ephemeris that filled it (entries 84, 89); the positions
 * are forgotten, as before, and the files and their constants kept. */
static void ephe_switch_forget(swe_ctx *ctx)
{
  int i;
  /* free_planets() less the files' data: each body's last result goes,
   * its constants and segment stay. The result has to be cleared, not only
   * its instant: a Moshier request reads the barycentric Sun's slot, which
   * it never fills, as zero; left at the Swiss Sun it moved Mars 2.9e-9
   * deg (G83). */
  for (i = 0; i < SEI_NPLANETS; i++) {
    ctx->pldat[i].iephe = 0;
    memset((void *) ctx->pldat[i].x, 0, sizeof(ctx->pldat[i].x));
    memset((void *) ctx->pldat[i].xreturn, 0, sizeof(ctx->pldat[i].xreturn));
  }
  memset((void *) &ctx->earth_moon, 0, sizeof(ctx->earth_moon));
  for (i = 0; i <= SE_NPLANETS; i++)
    memset((void *) &ctx->savedat[i], 0, sizeof(struct save_positions));
  for (i = 0; i < SEI_NNODE_ETC; i++)
    memset((void *) &ctx->nddat[i], 0, sizeof(struct plan_data));
  planets_forget_instants(ctx);
  swi_force_app_pos_etc(ctx);
}

/* Upstream bug 119: what swi_config_apply() drops when it adopts another
 * thread's files, as the setter drops it on its own thread: the positions,
 * and with the Swiss files closed (a new path) their bodies' constants
 * and segments too; with only the JPL file closed (a new JPL file) the
 * positions alone, the Swiss files and their data being the path's. */
void swi_adopted_files_forget(swe_ctx *ctx, AS_BOOL swiss_closed)
{
  if (swiss_closed)
    free_planets(ctx);
  else
    ephe_switch_forget(ctx);
}

/* the same for the angles (bug 67 began it with ctx->nut) */
static void angles_forget_instants(swe_ctx *ctx)
{
  ctx->oec.teps = SWI_TJD_NEVER;
  ctx->oec2000.teps = SWI_TJD_NEVER;
  ctx->nut.tnut = SWI_TJD_NEVER;
  ctx->nut2000.tnut = SWI_TJD_NEVER;
  ctx->nutv.tnut = SWI_TJD_NEVER;
}

/* all of them, for a context the memset or calloc has just zeroed; and
 * SunRA()'s memo in swehel.c, whose empty tjdlast of 0 answered RA 0 for
 * UT 0: swe_vis_limit_mag() at tjd_ut 0 on a fresh context, Venus -6.4973
 * against -6.2006 at 1e-9, the Moon -13.93 against -12.46 */
static void ctx_forget_instants(swe_ctx *ctx)
{
  planets_forget_instants(ctx);
  angles_forget_instants(ctx);
  ctx->topd.teval = SWI_TJD_NEVER;
  swi_hel_forget(ctx);		/* SunRA()'s, and bug 82's */
}
#endif

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
#ifndef SWE_UPSTREAM_COMPAT
  ctx_forget_instants(ctx);                /* calloc leaves 0: a valid instant (bugs 67, 71) */
#endif
  ctx->tid_acc          = SE_TIDAL_DEFAULT;
  ctx->is_tid_acc_manual = FALSE;
  /* Strict by default, and stated rather than left to calloc: a caller who
   * asks for an ephemeris either gets it or gets an error. See sweph.h.
   *
   * SE_EPHE_FALLBACK=1 restores upstream's behaviour without touching the
   * caller. That matters for programs that cannot be recompiled or edited --
   * upstream's own setest suite is one: it asks for ephemerides the machine
   * may not have and expects numbers back, so under the strict default it
   * reads zeros and fails. An environment switch lets such a program run
   * unmodified while everything else stays strict. */
  ctx->ephe_fallback    = swi_env_ephe_fallback();
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
#ifndef SWE_UPSTREAM_COMPAT
    AS_BOOL saved_naming_off = ctx->path_naming_off;
#endif
    int32     saved_hold  = ctx->cfg_hold;	/* bug 115: the caller's hold */
    /* Upstream bug 133 (this fork's): the Sunshine houses' remembered
     * declination, which swe_houses_armc() and swe_houses_armc_ex2() keep
     * without starting the context, went with the memset below, which also
     * left 0 for 99, "none yet": after swe_houses_armc(123.4, 47.37, 23.44,
     * 'I') with 20 deg in ascmc[9], a first calculation and a recall with
     * 99, cusps 2 and 3 were declination 0's, 228.717 and 260.801 deg for
     * 223.821 and 254.087. Upstream's memo is a static nothing resets
     * (Phase 3c moved it here): it is kept across the memset now, in both
     * builds, and 99 if none was made. */
    double    saved_sundec = ctx->saved_sundec;
    AS_BOOL swi_cfg_was = swi_config_begin_apply(ctx);
    /* ctx, not &swed: a non-default context must clear itself. */
    memset((void *) ctx, 0, sizeof(struct swe_ctx));
    ctx->cfg_seen     = saved_seen;
    ctx->cfg_local    = saved_local;
    ctx->cfg_hold     = saved_hold;
    ctx->saved_sundec = saved_sundec;
    ctx->cfg_applying = TRUE;      /* re-arm the guard the memset cleared */
    strcpy(ctx->ephepath, SE_EPHE_PATH);
    strcpy(ctx->jplfnam, SE_FNAME_DFT);
    /* the memset above would leave this 0.0, not the 0.0065 default */
    ctx->const_lapse_rate = SE_LAPSE_RATE;
    /* Same reason, and the same hazard: the memset clears the fallback
     * policy, so re-read it here. FALSE (strict) is what zero already means;
     * this is what makes SE_EPHE_FALLBACK reach the default context, which
     * never passes through ctx_init_defaults(). */
    ctx->ephe_fallback = swi_env_ephe_fallback();
#ifndef SWE_UPSTREAM_COMPAT
    ctx_forget_instants(ctx);      /* the memset leaves 0: a valid instant (bug 71) */
#endif
    /* initialised before the tidal setter, which now initialises a context
     * itself (bug 83) and would otherwise come back here */
    ctx->swed_is_initialised = TRUE;
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 118: resolving the automatic term named the default
     * path (bug 107) and opened its moon file -- before the master's path
     * was adopted below, and inside the path setter before the path it was
     * setting: that FILE was then overwritten, not closed (see
     * swe_set_ephe_path_r()). Resolved here without naming; named after
     * the sync, if there is still no path. */
    ctx->path_naming_off = TRUE;
    swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC);
    ctx->path_naming_off = saved_naming_off;
#else
    swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC);
#endif
    swi_config_end_apply(ctx, swi_cfg_was);
    started = 1;
  }
  /* Adopt any configuration published by another thread. This is the
   * hook the whole of Phase 2 hangs off: it is already called from 16
   * sites across sweph.c and swephlib.c, so the entry points are wired
   * up for free. Cheap -- one atomic load and a compare -- unless the
   * configuration actually moved. No-op while a setter on this thread is
   * mid-apply. */
#ifndef SWE_UPSTREAM_COMPAT
  /* bug 115: a context started just now has nothing in use, so it adopts
   * the master even inside a held calculation (swi_config_hold()) */
  if (started) {
    int32 hold = ctx->cfg_hold;
    ctx->cfg_hold = 0;
    swi_config_sync(ctx);
    ctx->cfg_hold = hold;
  } else
#endif
  swi_config_sync(ctx);
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 131: after swe_close(), the DE numbers it forgot, read
   * again along the kept path as the setters read them, at the first entry
   * that is not a setter -- this context's own close, or one it adopted
   * just now -- and the automatic term resolved as on a first use */
  {
    AS_BOOL reread = FALSE;
    if (ctx->denum_unread != 0 && !ctx->cfg_applying && !ctx->path_naming_off) {
      swi_denum_reread(ctx);
      reread = TRUE;
    }
    /* bug 118: the automatic term of the path now in effect, adopted or,
     * if none, the default one named on this first use (bugs 101, 107);
     * not published -- nothing was set */
    if ((started || reread) && !ctx->is_tid_acc_manual && !ctx->path_naming_off) {
      AS_BOOL was = swi_config_begin_apply(ctx);
      swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC);
      swi_config_end_apply(ctx, was);
    }
  }
  /* bug 128: the start swi_config_hold() made, answered here as if made
   * here (the warnings on a context's first call key on it) */
  if (ctx->start_unreported) {
    ctx->start_unreported = FALSE;
    started = 1;
  }
#endif
  return started;
}

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 92: the fixed-star memos -- swe_fixstar()'s and
 * swe_fixstar_mag()'s last record (sp.fs1, sp.fs1mag), swe_fixstar2()'s and
 * swe_fixstar2_mag()'s last star (sp.fs2, sp.fs2mag) -- outlived
 * swe_close() and a path to another sefstars.txt, and the catalogue
 * swe_fixstar2() reads into memory outlived the path: Sirius stayed at
 * -1.46 and 2.64 pc under a catalogue giving -1.00 and 3.33 pc, and
 * stayed at -1.00 after the path back, until another star was asked for
 * (and in swe_fixstar2() even then, until swe_close()). A thread adopting
 * another's path kept its star file open on the old one too. All are
 * emptied, and the file closed, wherever the path can change and where
 * swi_hel_forget() runs. */
void swi_fixstar_forget(swe_ctx *ctx)
{
  *ctx->sp.fs1.slast_stardata = '\0';
  *ctx->sp.fs1.slast_starname = '\0';
  *ctx->sp.fs1mag.slast_stardata = '\0';
  *ctx->sp.fs1mag.slast_starname = '\0';
  *ctx->sp.fs2.slast_starname = '\0';
  *ctx->sp.fs2mag.slast_starname = '\0';
  if (ctx->fixfp != NULL) {
    fclose(ctx->fixfp);
    ctx->fixfp = NULL;
  }
  free(ctx->fixed_stars);
  ctx->fixed_stars = NULL;
  ctx->n_fixstars_real = 0;
  ctx->n_fixstars_named = 0;
  ctx->n_fixstars_records = 0;
  ctx->is_old_starfile = FALSE;
  ctx->fixstars_load_failed = FALSE;	/* bug 121 */
  *ctx->fixstars_load_err = '\0';
}
#endif

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 98: the star functions read sefstars.txt along the path
 * but never named one, so as the first library call, with the files named
 * by SE_EPHE_PATH alone, they looked in PATH '' (upstream) or the compiled
 * default (after bug 92's sync) and failed: "SwissEph file 'sefstars.txt'
 * not found", for Sirius in all four. swe_calc() names the path on its
 * first use (swe_set_ephe_path(NULL), which reads SE_EPHE_PATH); so do
 * they now, on the same condition. */
static void fixstar_first_use(swe_ctx *ctx)
{
  swi_init_swed_if_start(ctx);
  if (!ctx->ephe_path_is_set && !ctx->jpl_file_is_open)
    swi_set_ephe_path_implicit(ctx);	/* bug 103 */
}
#endif

/* closes all open files, frees space of planetary data, 
 * deletes memory of all computed positions 
 */
static void swi_close_keep_topo_etc(swe_ctx *ctx, AS_BOOL forget_denum)
{
  swi_close_ephe_files(ctx, forget_denum);
  free_planets(ctx);
  memset((void *) &ctx->oec, 0, sizeof(struct epsilon));
  memset((void *) &ctx->oec2000, 0, sizeof(struct epsilon));
  memset((void *) &ctx->nut, 0, sizeof(struct nut));
  memset((void *) &ctx->nut2000, 0, sizeof(struct nut));
  memset((void *) &ctx->nutv, 0, sizeof(struct nut));
#ifndef SWE_UPSTREAM_COMPAT
  angles_forget_instants(ctx);	/* Upstream bugs 67, 71 */
  swi_hel_forget(ctx);		/* Upstream bug 82 */
  swi_fixstar_forget(ctx);	/* Upstream bug 92 */
  ctx->topd.teval = SWI_TJD_NEVER;	/* bug 92: other files, another site */
  /* Upstream bug 98: the astronomical models were zeroed here, swe_close()'s
   * line, so every swe_set_ephe_path() and swe_set_jpl_file(), and the
   * implicit path of a first calculation with the files named by
   * SE_EPHE_PATH, put back the defaults over what swe_set_astro_models()
   * or swe_set_sid_mode(..., SE_SIDBIT_PREC_ORIG) had set: with delta t
   * Stephenson & Morrison 1984 and nutation IAU 1980, the nutation at J2000
   * moved by 2.39e-6 deg and delta t in AD 1000 by 18.27 s (1474.79 for
   * 1456.52); Lahiri with its original precession 0.00056 deg in AD 1000.
   * Nothing the models feed is read from the files, and the caches they
   * key were emptied above. They stay now; swe_close() still resets them. */
#else
  memset((void *) &ctx->astro_models, 0, SEI_NMODELS * sizeof(int32));
#endif
  /* close JPL file */
  swi_close_jpl_file(ctx);
  ctx->jpl_file_is_open = FALSE;
  ctx->jpldenum = 0;
  /* close fixed stars */
  if (ctx->fixfp != NULL) {
    fclose(ctx->fixfp);
    ctx->fixfp = NULL;
  }
  swi_free_fict_lines(ctx);
  /* Both guard a table read from the EPHEMERIS PATH, and this function runs
   * when that path is being replaced -- swe_set_ephe_path_r() calls it. Left
   * set, init_leapsec() and init_dt() return early and the tables loaded
   * from the OLD directory survive into the new one: a second path's
   * seleapsec.txt is simply never seen. The seorbel line cache one line up
   * is dropped for exactly this reason; these two were missed.
   * init_dt() clears the delta-t memo itself when it re-runs, so the flags
   * are all that have to go back. */
  ctx->leapsec_done = FALSE;
  ctx->init_dt_done = FALSE;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 92: a manual tidal acceleration was reset to automatic
   * here, so by every swe_set_ephe_path() and swe_set_jpl_file() and by
   * the implicit path a first calculation sets when the files are named by
   * SE_EPHE_PATH: swe_set_tid_acc(SE_TIDAL_DE406); swe_calc_ut(...) left
   * -25.936 and swe_deltat() in AD 1000 1474.79 s for 1465.66, and a
   * swe_set_jpl_file("de200.eph") after it -23.8946 and 1305.37 s -- where
   * the manual says that once swe_set_tid_acc() has been used the
   * automatic setting is blocked until swe_set_tid_acc(SE_TIDAL_AUTOMATIC).
   * A manual value stays now; swe_close(), which releases everything,
   * still returns it to automatic. */
  /* Upstream bug 118: and an automatic one was resolved here, before the
   * setter calling this had put its new state in place -- the path, the
   * JPL DE number it publishes -- so swe_get_tid_acc() reported the
   * dropped JPL file's term (-23.8946 after de200 and then a path without
   * the moon file, or a missing JPL file, for -25.80 and -25.936). The
   * setters resolve it now when they are done (setter_tid_acc_auto()). */
#else
  swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC);
#endif
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
 * Deliberately NOT here: swi_config_reset(), which resets the process-wide
 * config master in what this resets. That is right for swe_close() --
 * documented as releasing everything the library holds -- and wrong for
 * swe_ctx_free(), where freeing one context must not reconfigure every
 * other one. */
static void ctx_release(swe_ctx *ctx)
{
  /* The published DE numbers go with the files: a closed library knows
   * nothing about either until a setter reads a header again, which is
   * what upstream's swe_close() leaves. Kept, a Mercury after swe_close()
   * took the moon file's term where upstream takes the default, and G8
   * (check-setest) failed four setest cases (2026-09-16 review, F2). */
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 131: and the path and the JPL file's name stay, so the
   * first delta t after the close, before any file was open, took the
   * default tidal term (-25.80) for the path's moon file's (-25.936):
   * 1463.43 s in AD 1000 for 1474.72, swe_calc_ut()'s Moon there
   * 197.37472 deg for 197.37633 and right on the second call; with
   * de200.eph set, 14.29 s in AD 1700 for 3.01. What the two setters read
   * is read again, along the kept path, at the next entry
   * (swi_init_swed_if_start(), swi_denum_reread()); the close itself still
   * opens nothing. */
  if (ctx->ephe_path_is_set)
    ctx->denum_unread |= SWI_DENUM_MOON;
  if (ctx->jpldenum_cfg != 0)
    ctx->denum_unread |= SWI_DENUM_JPL;
#endif
  ctx->sweph_denum_moon = 0;
  ctx->jpldenum_cfg = 0;
  swi_close_ephe_files(ctx, FORGET_DENUM);
  free_planets(ctx);
  memset((void *) &ctx->oec, 0, sizeof(struct epsilon));
  memset((void *) &ctx->oec2000, 0, sizeof(struct epsilon));
  memset((void *) &ctx->nut, 0, sizeof(struct nut));
  memset((void *) &ctx->nut2000, 0, sizeof(struct nut));
  memset((void *) &ctx->nutv, 0, sizeof(struct nut));
#ifndef SWE_UPSTREAM_COMPAT
  angles_forget_instants(ctx);	/* Upstream bugs 67, 71 */
  swi_hel_forget(ctx);		/* Upstream bug 82 */
  swi_fixstar_forget(ctx);	/* Upstream bug 92 */
  ctx->ephe_path_live = FALSE;	/* bug 103: the next path is a real one */
#endif
  memset((void *) &ctx->astro_models, 0, SEI_NMODELS * sizeof(int32));
#ifndef SWE_UPSTREAM_COMPAT
  /* bug 127: the sidereal mode stays, with the models it set */
  swi_sid_mode_models(&ctx->sidd, ctx->astro_models);
#endif
  /* close JPL file */
  swi_close_jpl_file(ctx);
  ctx->jpl_file_is_open = FALSE;
  ctx->jpldenum = 0;
  /* close fixed stars */
  if (ctx->fixfp != NULL) {
    fclose(ctx->fixfp);
    ctx->fixfp = NULL;
  }
  swi_free_fict_lines(ctx);
  SWI_CFG_LOCAL(ctx, swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC));
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 127: the observer and the sidereal mode were reset here,
   * where the manual has "The parameters set by swe_set_topo() survive
   * swe_close()" and "the parameters set by swe_set_sid_mode() survive
   * calls of the function swe_close()": after swe_set_topo(8.55, 47.37,
   * 400) and swe_close() the topocentric Moon at J2000 was refused
   * ("geographic position has not been set") for 222.988 deg, and after
   * swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0) the ayanamsa was the default's
   * 24.740 deg for 23.857; a user t0 and ayan_t0, SE_SIDBIT_ECL_T0 and
   * SE_SIDBIT_PREC_ORIG's models went with it. Both stay now, the mode
   * with the models it set (above), and only the observer's computed
   * place goes, which the files and the tidal term the close resets
   * frame. swe_set_astro_models()' models and the tidal acceleration are
   * still reset: the manual says nothing of them. */
  {
    double geolon = ctx->topd.geolon, geolat = ctx->topd.geolat;
    double geoalt = ctx->topd.geoalt;
    memset((void *) &ctx->topd, 0, sizeof(struct topo_data));
    ctx->topd.geolon = geolon;
    ctx->topd.geolat = geolat;
    ctx->topd.geoalt = geoalt;
    ctx->topd.teval = SWI_TJD_NEVER;	/* Upstream bug 71 */
  }
#else
  ctx->geopos_is_set = FALSE;
  ctx->ayana_is_set = FALSE;
  memset((void *) &ctx->topd, 0, sizeof(struct topo_data));
  memset((void *) &ctx->sidd, 0, sizeof(struct sid_data));
#endif
  ctx->is_old_starfile = FALSE;
  ctx->i_saved_planet_name = 0;
  *(ctx->saved_planet_name) = '\0';
  ctx->timeout = 0;
  ctx->last_epheflag = 0;
  /* eop_dpsi_loaded has to go back with them. load_dpsi_deps() opens with
   * "if (eop_dpsi_loaded > 0) return;" -- a load-once guard -- so leaving the
   * flag set while freeing the arrays it describes means the next JPL
   * Horizons request reloads nothing and bessel() dereferences NULL:
   *
   *   swe_calc(..., SEFLG_JPLHOR); swe_close(); swe_calc(..., SEFLG_JPLHOR);
   *
   * segfaults. Unreachable until now only because SEFLG_JPLHOR needs a JPL
   * file with a DE number >= 403 and the IERS corrections, neither of which
   * this repository ships; G25 reaches it with both. */
  if (ctx->dpsi != NULL) {
    free(ctx->dpsi);
    ctx->dpsi = NULL;
  }
  if (ctx->deps != NULL) {
    free(ctx->deps);
    ctx->deps = NULL;
  }
  ctx->eop_dpsi_loaded = 0;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 134: the interpolation's nodes built under SEFLG_JPLHOR
   * are of these corrections, and the close keeps the nodes (entry 123):
   * after it, another directory's eop_1962_today.txt and swe_set_jpl_file(),
   * SE_ECL_NUT with SEFLG_JPLHOR answered the old file's dpsi (G122's
   * fixtures, 0.131"). They go with the corrections. */
  if (ctx->interpol.nut_jplhor & SEFLG_JPLHOR) {
    ctx->interpol.tjd_nut0 = 0;
    ctx->interpol.tjd_nut2 = 0;
  }
#endif
  /* Same shape, same close: both guard a per-context table read from the
   * EPHEMERIS PATH, and swe_set_ephe_path() closes before adopting the new
   * one. Left set, the tables loaded from the old directory survive into the
   * new one -- verified for seleapsec.txt, where a second path's leap second
   * is simply never seen. init_dt() clears the delta-t memo itself when it
   * re-runs, so the flag is all that has to go back. */
  ctx->leapsec_done = FALSE;
  ctx->init_dt_done = FALSE;
  if (ctx->n_fixstars_records > 0) {
    free(ctx->fixed_stars);
    ctx->fixed_stars = NULL;
    ctx->n_fixstars_real = 0;
    ctx->n_fixstars_named = 0;
    ctx->n_fixstars_records = 0;
  }
/*  swed.ephe_path_is_set = FALSE;
  *swed.ephepath = '\0'; */
  /* Drop the shared master too, in what this resets (bug 123: only that,
   * so swi_config_reset() has to follow any change here).
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
  /* The master resets what ctx_release() resets and keeps what it keeps,
   * and this context keeps owning what it kept: the fallback setting
   * (bug 122), every other (bug 123), the observer and the sidereal mode
   * (bug 127). */
  swi_config_reset(ctx);
}

/* sets ephemeris file path. 
 * also calls swe_close(). this makes sure that swe_calc()
 * won't return planet positions previously computed from other
 * ephemerides
 */
/* Open the lunar ephemeris file for J2000 and read its header, to learn
 * the DE number and set the tidal acceleration of the Moon. Upstream ran
 * a full swe_calc() of the Moon for this, which also opened the planet
 * file and filled the save area; the header is all that is used. */
static void ephe_path_read_moon(swe_ctx *ctx)
{
  struct file_data *fdp = &ctx->fidat[SEI_FILE_MOON];
  char fname[AS_MAXCH];
  swi_gen_filename(J2000, SEI_MOON, fname);
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 118: an open moon file was overwritten here, not closed
   * (the default path's, opened by a tidal term resolved before the path
   * was set: 106 descriptors after 100 contexts) */
  if (fdp->fptr != NULL)
    file_close_open(ctx, SEI_FILE_MOON);
#endif
  if ((fdp->fptr = swi_fopen(ctx, SEI_FILE_MOON, fname, ctx->ephepath, NULL)) != NULL) {
    if (read_const(ctx, SEI_FILE_MOON, NULL) != OK) {
      /* read_const() has closed it: fclose(NULL) here crashed the path
       * setter on a refused moon file (UPSTREAM-BUGS 104) */
      memset((void *) fdp, 0, sizeof(struct file_data));
    }
#ifndef SWE_UPSTREAM_COMPAT
    else	/* the span it answers for, as sweph() opens it (bug 89) */
      file_nominal_span(J2000, SEI_MOON, fname, &fdp->tnom0, &fdp->tnom1);
#endif
  }
  if (fdp->fptr != NULL)
    swi_set_tid_acc(ctx, 0, 0, fdp->sweph_denum, NULL);
  /* Whatever the pre-open found is what every context inheriting this
   * configuration must use for its tidal term -- including "nothing",
   * so that a tree without a moon file answers with the default term on
   * its first delta-t too, not only after a failed open. Published with
   * SWI_CFG_PATH. */
  ctx->sweph_denum_moon = (fdp->fptr != NULL) ? fdp->sweph_denum : 0;
}

/* The directory list swe_set_ephe_path() adopts for `path`: SE_EPHE_PATH
 * if set, else `path`, else the compiled default, ending in DIR_GLUE. */
static void ephe_path_resolve(const char *path, char *s)
{
  int i;
  char *sp;
  if ((sp = getenv("SE_EPHE_PATH")) != NULL
    && strlen(sp) != 0
    && strlen(sp) <= AS_MAXCH-1-13) {
    snprintf(s, AS_MAXCH, "%s", sp);
  } else if (path == NULL || *path == '\0') {
    snprintf(s, AS_MAXCH, "%s", SE_EPHE_PATH);
  } else if (strlen(path) <= AS_MAXCH-1-13) {
    snprintf(s, AS_MAXCH, "%s", path);
  } else {
    snprintf(s, AS_MAXCH, "%s", SE_EPHE_PATH);
  }
  i = (int) strlen(s);
  if (*(s + i - 1) != *DIR_GLUE && *s != '\0')
    strcat(s, DIR_GLUE);
}

#ifndef SWE_UPSTREAM_COMPAT
/* The path a calculation or a star lookup sets on first use when none has
 * been: the files SE_EPHE_PATH (or the compiled default) names. Local --
 * computing publishes nothing -- and marked as implicit, so that a sync
 * against a master that names no path keeps it (bug 103). */
void swi_set_ephe_path_implicit(swe_ctx *ctx)
{
  SWI_CFG_LOCAL(ctx, swe_set_ephe_path_r(ctx, NULL));
  ctx->ephe_path_implicit = TRUE;
}
#endif

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 107: the path a first use sets from SE_EPHE_PATH was set by
 * swe_calc() (and the star functions) only, after whatever the call had
 * read before it: swe_calc_ut()'s delta t read swe_deltat.txt along the
 * compiled default path, so with the files named by SE_EPHE_PATH the first
 * Moon at 2000 Jan 1 12h took the built-in delta t (63.83 s) and the
 * second the directory's table (70.0 s): 8.6e-4 deg apart, and the table
 * read twice; swe_deltat() (6.17 s), swe_sidtime(), swe_houses(), the
 * stars' and the ayanamsa's _ut forms, swe_azalt(), swe_pheno_ut(),
 * swe_lun_eclipse_how(), swe_rise_trans(), swe_utc_to_jd()'s leap seconds
 * and swe_set_jpl_file()'s file (de200.eph opened twice) the same. The path is named now at the first
 * file a context opens along its path, or the first tidal term it takes
 * from the moon file (swi_get_tid_acc()), before anything is read -- so
 * without closing anything, as there is nothing to close: the path, the
 * moon file's header for its DE number, local and marked implicit as
 * swi_set_ephe_path_implicit() does. The tidal term of the request under
 * way is left as it was resolved. */
void swi_ephe_path_name_first_use(swe_ctx *ctx)
{
  char s[AS_MAXCH];
  double tid_acc = ctx->tid_acc;
  if (!ctx->swed_is_initialised || ctx->ephe_path_is_set || ctx->jpl_file_is_open
      || ctx->path_naming_off)	/* bug 118 */
    return;
  ephe_path_resolve(NULL, s);
  snprintf(ctx->ephepath, AS_MAXCH, "%s", s);
  ctx->ephe_path_is_set = TRUE;
  ctx->ephe_path_implicit = TRUE;
  ctx->ephe_path_live = TRUE;
  ephe_path_read_moon(ctx);
  ctx->tid_acc = tid_acc;
}
#endif

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 110: a first call's "Please call swe_set_ephe_path() or
 * swe_set_jplfile() before calling ..." was written whenever that call
 * initialised the context, so with the files named by SE_EPHE_PATH, found
 * along the default path, or set by another thread and adopted, the first
 * swe_deltat_ex() and swe_get_ayanamsa_ex_ut() answered correctly with the
 * warning in serr, and the first swe_calc_orbel() refused (-1) on it. It
 * is due now only when no path is in effect: none set or adopted, no
 * SE_EPHE_PATH, and none of the files -- the moon and planet files at
 * J2000, the JPL file -- along the default path. Run on a first call
 * only; naming the path (bug 107) is what the call would do next. */
/* Upstream bug 118: whether a file is on the path, looked up as
 * swi_fopen() would find it, but not opened: swi_ephe_path_none() opened
 * the planet file and closed it, and the calculation then opened it again */
static AS_BOOL file_on_path(const char *ephepath, const char *fname)
{
  char s1[AS_MAXCH], s[2 * AS_MAXCH], *cpos[20];
  struct stat st;
  int np, i;
  snprintf(s1, sizeof s1, "%s", ephepath);
  np = swi_cutstr(s1, PATH_SEPARATOR, cpos, 20);
  for (i = 0; i < np; i++) {
    size_t j = strlen(cpos[i]);
    if (strcmp(cpos[i], ".") == 0)
      snprintf(s, sizeof s, "%s", fname);
    else
      snprintf(s, sizeof s, "%s%s%s", cpos[i],
               (j > 0 && cpos[i][j - 1] != *DIR_GLUE) ? DIR_GLUE : "", fname);
    if (stat(s, &st) == 0 && !(st.st_mode & S_IFDIR))
      return TRUE;
  }
  return FALSE;
}

AS_BOOL swi_ephe_path_none(swe_ctx *ctx)
{
  char *sp = getenv("SE_EPHE_PATH");
  char fname[AS_MAXCH];
  if (ctx->jpl_file_is_open)
    return FALSE;
  if (ctx->ephe_path_is_set && !ctx->ephe_path_implicit)
    return FALSE;			/* set, or adopted from the master */
  if (sp != NULL && *sp != '\0' && strlen(sp) <= AS_MAXCH-1-13)
    return FALSE;			/* as ephe_path_resolve() takes it */
  swi_ephe_path_name_first_use(ctx);	/* the default path, the moon file */
  if (ctx->fidat[SEI_FILE_MOON].fptr != NULL)
    return FALSE;
  swi_gen_filename(J2000, SEI_EARTH, fname);
  if (file_on_path(ctx->ephepath, fname) || file_on_path(ctx->ephepath, ctx->jplfnam))
    return FALSE;
  return TRUE;
}
#endif

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 118: an automatic tidal acceleration resolved for what a
 * path or JPL setter has just put in place, not for what it replaced */
static void setter_tid_acc_auto(swe_ctx *ctx)
{
  if (!ctx->is_tid_acc_manual)
    swe_set_tid_acc_r(ctx, SE_TIDAL_AUTOMATIC);
}
#endif

void CALL_CONV swe_set_ephe_path_r(swe_ctx *ctx, const char *path)
{
  AS_BOOL swi_cfg_was = swi_config_begin_apply(ctx);
  char s[AS_MAXCH];
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 103: the same path again closed every file, emptied every
   * cache and reopened the moon file for its header: 4 opens a call, 398
   * in 100 calls each followed by a Moon. A path the context already holds,
   * live (not since swe_close()), is kept now: its files stay open and its
   * caches full. An automatic tidal acceleration is resolved again, as the
   * moon file's header would give it. A caller who rewrote a file on disk
   * and wants it read again calls swe_close() first -- which releases
   * everything -- as the manual has it before a new swe_set_ephe_path(). */
  ephe_path_resolve(path, s);
  if (ctx->swed_is_initialised && ctx->ephe_path_live && ctx->ephe_path_is_set
      && strcmp(ctx->ephepath, s) == 0) {
    ctx->ephe_path_implicit = FALSE;
    /* Upstream bug 118: kept even when the path had no moon file, so one
     * installed since was never read: delta t in AD 1000 1463.44 s (the
     * default term) until a Moon was computed, then 1474.72. The same
     * path with a moon file it did not have is a change: looked for again
     * when none was found, which opens nothing when there is still none. */
    if (ctx->fidat[SEI_FILE_MOON].fptr == NULL && ctx->sweph_denum_moon == 0)
      ephe_path_read_moon(ctx);
    if (!ctx->is_tid_acc_manual) {
      int32 denumret;
      swi_get_tid_acc(ctx, 0, swi_guess_ephe_flag(ctx), 0, &denumret, &ctx->tid_acc, NULL);
    }
    swi_config_end_apply(ctx, swi_cfg_was);
    swi_config_publish(ctx, SWI_CFG_PATH);
    return;
  }
#endif
  /* The path is changing: the next open may find a different generation of
   * files, so their DE numbers go with them. */
#ifndef SWE_UPSTREAM_COMPAT
  ctx->path_naming_off = TRUE;	/* bug 118: this setter names it */
#endif
  swi_close_keep_topo_etc(ctx, FORGET_DENUM);
  swi_init_swed_if_start(ctx);
  ctx->ephe_path_is_set = TRUE;
  /* The JPL file is found through the path, so its published DE number
   * does not survive a path change either (2026-09-16 review, F4): kept,
   * a JPL delta-t after pointing the path at a directory without the
   * file still used the old file's term, where upstream uses the
   * default. swe_set_jpl_file() publishes it again. */
  ctx->jpldenum_cfg = 0;
#ifndef SWE_UPSTREAM_COMPAT
  ctx->denum_unread = 0;	/* bug 131: read here, or not owed */
  /* Upstream bug 144: the IERS corrections stayed, and load_dpsi_deps()
   * loads once: after path A, the DE431 fixture, path B and the fixture,
   * SEFLG_JPLHOR answered A's dpsi -13.530688" for B's -13.399987" (G134,
   * 0.131"), and after path B alone A's for none (SEFLG_JPLHOR_APPROX's,
   * 4.20e-2"). They are the old directory's, and go with the DE number;
   * the next swe_set_jpl_file() reads the new one's. */
  dpsi_deps_free(ctx);
#endif
  /* environment variable SE_EPHE_PATH has priority. The length tests are
   * upstream's: a path with no room left for a file name is dropped for the
   * default rather than silently truncated. They do not bound the file name
   * -- swi_fopen() checks that itself, per path element. snprintf here only
   * makes the copy structurally safe. */
  ephe_path_resolve(path, s);
  snprintf(ctx->ephepath, AS_MAXCH, "%s", s);
//swe_set_interpolate_nut(TRUE);
  /* Open the lunar ephemeris file for J2000 and read its header, to learn
   * the DE number and set the tidal acceleration of the Moon. Upstream ran
   * a full swe_calc() of the Moon for this, which also opened the planet
   * file and filled the save area; the header is all that is used. */
  ctx->last_epheflag = 2;
  ephe_path_read_moon(ctx);
#ifndef SWE_UPSTREAM_COMPAT
  ctx->path_naming_off = FALSE;
  setter_tid_acc_auto(ctx);	/* bug 118 */
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  ctx->ephe_path_live = TRUE;	/* bug 103 */
  ctx->ephe_path_implicit = FALSE;
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

/* Column offsets into a line of the IERS finals file (eop_finals.txt, the
 * "finals.all" product). The format is fixed-width, so every field is read
 * by position. */
#define IERS_FINALS_COL_MJD		7
#define IERS_FINALS_COL_DPSI_BULL_A	99
#define IERS_FINALS_COL_DEPS_BULL_A	118
#define IERS_FINALS_COL_DPSI_BULL_B	168
#define IERS_FINALS_COL_DEPS_BULL_B	178

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
    /* Upstream bug 106: a short line left cpos[3], [8] and [9] unset, and
     * a file of more than SWE_DATA_DPSI_DEPS days ran past dpsi[], deps[] */
    if (swi_cutstr(s, " ", cpos, 16) < 10)
      continue;
    if ((iyear = atoi(cpos[0])) == 0) 
      continue;
    if (n >= SWE_DATA_DPSI_DEPS)
      break;
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
    mjd = atoi(s + IERS_FINALS_COL_MJD);
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
    dpsi = atof(s + IERS_FINALS_COL_DPSI_BULL_B);
    deps = atof(s + IERS_FINALS_COL_DEPS_BULL_B);
    if (dpsi == 0) {
      /* try dpsi, deps Bulletin A */
      dpsi = atof(s + IERS_FINALS_COL_DPSI_BULL_A);
      deps = atof(s + IERS_FINALS_COL_DEPS_BULL_A);
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

/* What was computed from the IERS corrections: the nutation kept at an
 * instant and the topocentric observer, and the interpolation's nodes
 * built under SEFLG_JPLHOR (bug 134). Upstream keys none of them on the
 * flags, so its nodes all go. */
static void dpsi_deps_forget(swe_ctx *ctx)
{
#ifndef SWE_UPSTREAM_COMPAT
  ctx->nut.tnut = SWI_NUT_NEVER;
  ctx->nutv.tnut = SWI_TJD_NEVER;
  ctx->topd.teval = SWI_TJD_NEVER;
  if (!(ctx->interpol.nut_jplhor & SEFLG_JPLHOR))
    return;
#else
  ctx->nut.tnut = 0;
  ctx->nutv.tnut = 0;
  ctx->topd.teval = 0;
#endif
  ctx->interpol.tjd_nut0 = 0;
  ctx->interpol.tjd_nut2 = 0;
}

/* The IERS corrections dropped, and what was computed from them, so that
 * the next load reads them again (load_dpsi_deps() loads once): another
 * path's or JPL file's are not these (bug 144) */
static void dpsi_deps_free(swe_ctx *ctx)
{
  AS_BOOL had = ctx->eop_dpsi_loaded > 0;
  free(ctx->dpsi);
  free(ctx->deps);
  ctx->dpsi = NULL;
  ctx->deps = NULL;
  ctx->eop_dpsi_loaded = 0;
  if (had)
    dpsi_deps_forget(ctx);
}

/* Upstream bug 143 (this fork's): a context adopting another's JPL file
 * -- a thread on the default context at its sync, a thread's first start,
 * swe_ctx_new() -- never loaded the IERS corrections, which only
 * swe_set_jpl_file() read: its SEFLG_JPLHOR became SEFLG_JPLHOR_APPROX
 * ("you did not call swe_set_jpl_file()") where the setter's did not,
 * dpsi on G134's DE431 fixture at 2000 Jan 7.37 -13.488712" for
 * -13.530688" (4.20e-2"), the ayanamsa and the sidereal houses with it.
 * Loaded now as the setter loads them, for a JPL file of DE 403 or later,
 * along the adopted path, and what was computed without them goes.
 * Upstream bug 144, for the adoption: the old path's or JPL file's, loaded
 * once, stayed, A's dpsi -13.530688" for B's -13.399987" after another
 * thread's path B and the fixture (0.131"), A's after B alone for none.
 * They go first now, as the setter drops them. Called by
 * swi_config_apply() for an adopted path or JPL file, in both builds: the
 * adoption is this fork's. */
void swi_dpsi_deps_adopt(swe_ctx *ctx)
{
  dpsi_deps_free(ctx);
  if (ctx->jpldenum_cfg < 403)
    return;
  load_dpsi_deps(ctx);
  if (ctx->eop_dpsi_loaded > 0)
    dpsi_deps_forget(ctx);
}

/* Close the .se1 files.
 *
 * FORGET_DENUM when the next open may find different files (the ephemeris
 * path changed, or the context is being torn down); KEEP_DENUM when the same
 * files are coming back.
 *
 * The DE number matters after the close because swe_calc_ut_r() computes
 * delta-t BEFORE the position that reopens anything:
 *
 *   swe_calc_ut_r()   -> swe_deltat_ex_r()
 *                     -> denum = fidat[SEI_FILE_MOON].sweph_denum
 *                     -> swi_set_tid_acc() -> swi_get_tid_acc()
 *
 * and swi_get_tid_acc()'s fallback for denum == 0 also tests
 * fidat[SEI_FILE_MOON].fptr, which the close just set to NULL. So a zeroed
 * denum takes the default -25.8 instead of DE441's -25.936: at -3000, where
 * delta-t runs to hours, the Sun lands 4.56 arcsec out and the Moon 53. */
void swi_close_ephe_files(swe_ctx *ctx, AS_BOOL forget_denum)
{
  int i;
  for (i = 0; i < SEI_NEPHFILES; i++) {
    int32 denum = ctx->fidat[i].sweph_denum;
    if (ctx->fidat[i].fptr != NULL)
      fclose(ctx->fidat[i].fptr);
    memset((void *) &ctx->fidat[i], 0, sizeof(struct file_data));
    if (!forget_denum)
      ctx->fidat[i].sweph_denum = denum;
  }
#ifndef SWE_UPSTREAM_COMPAT
  for (i = 0; i <= SEI_FILE_MAIN_AST; i++)
    file_spare_drop(ctx, i);
#endif
}

/* Did the library answer from a different ephemeris than the one asked for?
 * If so, and fallback is off, write the refusal into serr and return TRUE.
 *
 * `wanted` and `got` are single SEFLG_*EPH bits. `wanted` has already
 * resolved an unspecified request to SEFLG_SWIEPH, so naming nothing is
 * treated as asking for Swiss -- which is what swe_calc_ut_r() does
 * explicitly anyway, and the two entry points must agree about identical
 * arguments. `got` may be 0 when no ephemeris was consulted at all (mean
 * node, obliquity), which is not a substitution. Asking for Moshier and
 * getting it is not a downgrade. */
static AS_BOOL swi_refuse_downgrade(swe_ctx *ctx, int32 wanted, int32 got, double tjd, char *serr)
{
  if (ctx->ephe_fallback || got == 0 || got == wanted)
    return FALSE;
  if (serr != NULL)
    snprintf(serr, AS_MAXCH,
      "%s ephemeris is not available for jd %.4f; falling back to %s would "
      "answer with lower precision. Install the data files, or call "
      "swe_set_ephe_fallback(1) to allow the substitution.",
      swi_ephe_name(wanted), tjd, swi_ephe_name(got));
  return TRUE;
}

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 120: with the semo file missing, sweplan() took the Moon
 * from Moshier (its place equal to SEFLG_MOSEPH's within 1e-10 deg, 0.6-5.0"
 * off the files) and made the Earth from it, noted in serr, and the call
 * returned SEFLG_SWIEPH: swi_refuse_downgrade() saw no substitution, the
 * planets' and the Moon's ephemeris bits being Swiss. Under the strict
 * setting a Swiss answer whose Moon, or whose Earth through the Moon, is
 * Moshier's is now refused; ctx->moshmoon_used says a read in this call
 * took it. The caller decides whether its answer depends on it. */
static AS_BOOL swi_refuse_moshmoon(swe_ctx *ctx, double tjd, char *serr)
{
  if (ctx->ephe_fallback || !ctx->moshmoon_used)
    return FALSE;
  if (serr != NULL)
    snprintf(serr, AS_MAXCH,
      "Swiss (.se1) Moon file is not available for jd %.4f; the Moon, and "
      "the Earth made from it, would come from Moshier with lower precision. "
      "Install the data files, or call swe_set_ephe_fallback(1) to allow the "
      "substitution.", tjd);
  return TRUE;
}

/* Upstream bug 139: Moshier serves no barycentric place (its origin is the
 * Sun, and swecalc() refuses SEFLG_MOSEPH | SEFLG_BARYCTR), so a fallback
 * to it serves none either. got_iflag is the answer's flag as the
 * ephemeris that answered left it. Under the strict setting the
 * substitution is refused as any other is, with swi_refuse_downgrade()'s
 * message; with the fallback allowed, with swecalc()'s. */
static AS_BOOL swi_refuse_mosh_bary(swe_ctx *ctx, int32 wanted, int32 got_iflag, double tjd, char *serr)
{
  if (!(got_iflag & SEFLG_BARYCTR) || !(got_iflag & SEFLG_MOSEPH))
    return FALSE;
  if (!swi_refuse_downgrade(ctx, wanted, SEFLG_MOSEPH, tjd, serr) && serr != NULL)
    strcpy(serr, "barycentric Moshier positions are not supported.");
  return TRUE;
}

#endif
/* SE_EPHE_FALLBACK=1 restores upstream's silent substitution without touching
 * the caller -- for programs that cannot be recompiled. Anything other than
 * unset, empty, or "0" enables it. */
AS_BOOL swi_env_ephe_fallback(void)
{
  const char *sp = getenv("SE_EPHE_FALLBACK");
  return (sp != NULL && *sp != '\0' && *sp != '0') ? TRUE : FALSE;
}

/* The name of an ephemeris flag, for error messages. Not a lookup table:
 * SEFLG_JPLEPH/SWIEPH/MOSEPH are separate bits, not an index. */
const char *swi_ephe_name(int32 epheflag)
{
  if (epheflag & SEFLG_JPLEPH)
    return "JPL";
  if (epheflag & SEFLG_MOSEPH)
    return "Moshier";
  if (epheflag & SEFLG_SWIEPH)
    return "Swiss (.se1)";
  return "default";
}

/* Allow, or refuse, an involuntary drop to a weaker ephemeris.
 *
 * Off by default in this fork, which is a deliberate break from upstream.
 * Upstream answers with Moshier when a .se1 file is missing or the date
 * falls outside it, mentions this in serr, and returns success -- so a
 * caller that checks the return value alone cannot tell a data-file
 * position from an analytic approximation. The discrepancy is small enough
 * to pass a spot check (the Sun agrees to about 0.02 arcsec) and large
 * enough to matter elsewhere (the Moon to ~2.9 arcsec, Neptune growing past
 * an arcsecond after 2030).
 *
 * Set it to 1 to restore upstream behaviour. The ephemeris actually used is
 * still reported in the return flag either way. */
void CALL_CONV swe_set_ephe_fallback_r(swe_ctx *ctx, int allow)
{
  /* ⚠️ Initialise FIRST. swi_init_swed_if_start() memsets the whole context
   * on a fresh start, so setting the flag before the library has started
   * would be silently erased by the first swe_calc() -- the setter would
   * appear to work and do nothing. Every other setter here calls it for the
   * same reason. */
  swi_init_swed_if_start(ctx);
  ctx->ephe_fallback = allow ? TRUE : FALSE;
  /* Upstream bug 122 (this fork's, as the setting is, so not behind
   * SWE_UPSTREAM_COMPAT): the setting belonged to no configuration group
   * and was never published. With SE_EPHE_FALLBACK unset the main thread
   * read 1 after swe_set_ephe_fallback(1) and a second thread 0, its Swiss
   * Mars at J2000 without sepl_18.se1 refused (-1) where the main thread's
   * answered from Moshier (4); with the variable set, a setting of 0 left
   * the second thread on Moshier; a context made after the setting took
   * the variable's value, not the setting. It is the FALLBACK group now,
   * published as every other setter's, its default the variable's. */
  swi_config_publish(ctx, SWI_CFG_FALLBACK);
}

void CALL_CONV swe_set_ephe_fallback(int allow)
{
  swe_set_ephe_fallback_r(swi_default_ctx(), allow);
}

int CALL_CONV swe_get_ephe_fallback_r(swe_ctx *ctx)
{
  /* Bug 122, as bug 101 for swe_get_tid_acc_r(): as a first call on a
   * thread this read the context before anything had initialised it, 0
   * with SE_EPHE_FALLBACK=1, and before the sync that brings another
   * thread's setting in. It initialises and syncs first now. */
  swi_init_swed_if_start(ctx);
  return ctx->ephe_fallback ? 1 : 0;
}

int CALL_CONV swe_get_ephe_fallback(void)
{
  return swe_get_ephe_fallback_r(swi_default_ctx());
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 103: the JPL file already open under this name, read as
   * set (jpldenum_cfg), was closed with every Swiss file and cache and
   * opened again: 3 opens a call, 299 in 100 calls each followed by a JPL
   * Moon. It is kept now. Another name, or one not open, is a real change. */
  {
    const char *nm = strrchr(fname, (int) *DIR_GLUE);
    nm = (nm == NULL) ? fname : nm + 1;
    if (ctx->swed_is_initialised && ctx->jpl_file_is_open && ctx->jpldenum_cfg != 0
        && strlen(nm) < AS_MAXCH && strcmp(ctx->jplfnam, nm) == 0) {
      swi_config_end_apply(ctx, swi_cfg_was);
      swi_config_publish(ctx, SWI_CFG_PATH);
      return;
    }
  }
#endif
  /* The ephemeris path is unchanged, so the same .se1 files come back. */
  swi_close_keep_topo_etc(ctx, KEEP_DENUM);
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
  /* The setter is what publishes the JPL DE number -- configuration, read
   * once here. open_jpl_file() used to publish it on EVERY open, lazy
   * opens on the calculation paths included, so a context's config
   * changed because it had computed something (2026-09-16 review, F4). */
  ctx->jpldenum_cfg = (retc == OK) ? ctx->jpldenum : 0;
#ifndef SWE_UPSTREAM_COMPAT
  ctx->denum_unread &= ~SWI_DENUM_JPL;	/* bug 131: read here */
#endif
  if (retc == OK) {
    if (ctx->jpldenum >= 403) {
      /*if (INCLUDE_CODE_FOR_DPSI_DEPS_IAU1980) */
	load_dpsi_deps(ctx);
    }
  }
#ifndef SWE_UPSTREAM_COMPAT
  setter_tid_acc_auto(ctx);	/* bug 118: for the file just read, or none */
#endif
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

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 131: what swe_set_ephe_path() and swe_set_jpl_file() read
 * and swe_close() forgot, read again along the kept path as the setters
 * read it: the moon file's header at J2000 for its DE number, the JPL file
 * opened for its own (with the IERS corrections of a DE 403 or later). A
 * file the context has opened since is taken as it is. The tidal term of
 * the call under way is left as it was; the caller resolves it. */
void swi_denum_reread(swe_ctx *ctx)
{
  int32 unread = ctx->denum_unread;
  double tid_acc = ctx->tid_acc;
  double ss[3];
  ctx->denum_unread = 0;
  if ((unread & SWI_DENUM_MOON) && ctx->ephe_path_is_set) {
    if (ctx->fidat[SEI_FILE_MOON].fptr != NULL)
      ctx->sweph_denum_moon = ctx->fidat[SEI_FILE_MOON].sweph_denum;
    else
      ephe_path_read_moon(ctx);
  }
  if ((unread & SWI_DENUM_JPL)
      && (ctx->jpl_file_is_open
          || open_jpl_file(ctx, ss, ctx->jplfnam, ctx->ephepath, NULL) == OK)) {
    ctx->jpldenum_cfg = ctx->jpldenum;
    if (ctx->jpldenum >= 403)
      load_dpsi_deps(ctx);
  }
  ctx->tid_acc = tid_acc;
}
#endif

/* calculates obliquity of ecliptic and stores it together
 * with its date, sine, and cosine
 */
static void calc_epsilon(swe_ctx *ctx, double tjd, int32 iflag, struct epsilon *e)
{
    e->teps = tjd;
    e->epsflag = SWI_EPS_KEY(iflag);
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
/* iephe_used, if not NULL, receives the ephemeris that actually answered --
 * this function substitutes on its own when the one asked for is missing,
 * and its callers outside swecalc() (the fixed-star functions) need to know
 * in order to refuse the answer under the strict default. */
static int main_planet_bary(swe_ctx *ctx, double tjd, int ipli, int32 epheflag, int32 iflag, AS_BOOL do_save,
		       int32 *iephe_used,
		       double *xp, double *xe, double *xs, double *xm,
		       char *serr)
{
  int i, retc;
  int32 used = epheflag;
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
      used = SEFLG_SWIEPH;
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
      used = SEFLG_MOSEPH;
      retc = swi_moshplan(ctx, tjd, ipli, do_save, xp, xe, serr);/**/
      if (retc == ERR)
	return ERR;
      /* xs NULL: the caller wants no Sun, as sweplan() and jplplan() allow */
      if (xs != NULL)
	for (i = 0; i <= 5; i++)
	  xs[i] = 0;
      break;
    default:
      break;
  }
  if (iephe_used != NULL)
    *iephe_used = used;
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
#ifndef SWE_UPSTREAM_COMPAT
  AS_BOOL moon_mosh = FALSE;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
	moon_mosh = TRUE;	/* bug 120: told, not hidden */
	ctx->moshmoon_used = TRUE;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
      if (tjd == ctx->moshmoon_earth_t)	/* bug 120: made from Moshier's Moon */
	ctx->moshmoon_used = TRUE;
#endif
    } else {
      retc = sweph(ctx, tjd, SEI_EMB, SEI_FILE_PLANET, iflag, NULL, do_save, xpe, serr);
      if (retc != OK)
	return(retc);
#ifndef SWE_UPSTREAM_COMPAT
      if (xpe == pebdp->x)
	ctx->moshmoon_earth_t = moon_mosh ? tjd : SWI_TJD_NEVER;
#endif
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
  /* This fork reads at TDB: state() in swejpl.c and sweph() add
   * swi_tdb_minus_tt() to the offset into the record (UPSTREAM-BUGS 24).
   * The formula below also divides by 8640 where a day is 86400 s. */
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 84: these two caches answered for any ephemeris that
     * had filled the Earth's and the Sun's slots at tjd, where the
     * planet's below, sweplan()'s and swi_moshplan()'s check it. free_planets()
     * runs when the ephemeris ASKED for changes, not when a call answers
     * from another: a JPL call whose light time leaves the file falls to
     * Moshier (with swe_set_ephe_fallback(1), or on the way to refusing
     * it), which leaves its heliocentric Earth in the slot at tjd, and
     * the next JPL call there took it for the barycentric one: the Moon
     * 160 deg off, Mercury 0.30 deg, Mars 0.090, Jupiter 0.010, flagged
     * JPL. Keyed on the ephemeris too. */
    if (tjd != pedp->teval || pedp->iephe != SEFLG_JPLEPH) {	/* bugs 71, 84 */
#else
    if (tjd != pedp->teval || tjd == 0) {
#endif
      retc = swi_pleph(ctx, tjd, J_EARTH, J_SBARY, xpe, serr);
#ifndef SWE_UPSTREAM_COMPAT
      if (do_save && retc == OK && swi_pleph_moon(ctx, tjd, ctx->earth_moon.x)) {
	ctx->earth_moon.t = tjd;
	ctx->earth_moon.iephe = SEFLG_JPLEPH;
      }
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    if (tjd != psdp->teval || psdp->iephe != SEFLG_JPLEPH) {	/* bugs 71, 84 */
#else
    if (tjd != psdp->teval || tjd == 0) {
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 90: no else before this if, so the Earth, copied above,
   * went through the "other planet" branch as well: with the Earth's slot
   * filled, the same Earth again (xp = pdp->x); without (do_save off), one
   * more swi_pleph() call for the same state, 0 difference in the answer. */
  } else if (ipli == SEI_SUNBARY) {
#else
  } if (ipli == SEI_SUNBARY) {
#endif
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

#ifndef SWE_UPSTREAM_COMPAT
/* [t0, t1): the instants swi_gen_filename() names fname for, the 600-year
 * bucket of tjd (UPSTREAM-BUGS 89). From the calendar, then moved to the
 * double where the name changes, so the open file's span and a fresh
 * context's choice cannot disagree by a rounding at the join. */
static void file_nominal_span(double tjd, int ipli, const char *fname,
                              double *t0, double *t1)
{
  char s[AS_MAXCH];
  int y, m, d, icty, ncties = (int) NCTIES;
  double ut, t;
  swe_revjul(tjd, tjd >= 2305447.5 ? SE_GREG_CAL : SE_JUL_CAL, &y, &m, &d, &ut);
  icty = y / 100;
  if (y < 0 && y % 100 != 0)
    icty -= 1;
  while (icty % ncties != 0)
    icty--;
  y = icty * 100;
  t = swe_julday(y, 1, 1, 0, y >= 1600 ? SE_GREG_CAL : SE_JUL_CAL);
  for (swi_gen_filename(t, ipli, s); strcmp(s, fname) != 0; swi_gen_filename(t, ipli, s))
    t = nextafter(t, HUGE_VAL);
  for (;;) {
    double tp = nextafter(t, -HUGE_VAL);
    swi_gen_filename(tp, ipli, s);
    if (strcmp(s, fname) != 0)
      break;
    t = tp;
  }
  *t0 = t;
  y += ncties * 100;
  t = swe_julday(y, 1, 1, 0, y >= 1600 ? SE_GREG_CAL : SE_JUL_CAL);
  for (swi_gen_filename(t, ipli, s); strcmp(s, fname) == 0; swi_gen_filename(t, ipli, s))
    t = nextafter(t, HUGE_VAL);
  for (;;) {
    double tp = nextafter(t, -HUGE_VAL);
    swi_gen_filename(tp, ipli, s);
    if (strcmp(s, fname) == 0)
      break;
    t = tp;
  }
  *t1 = t;
}

/* The file each of sepl, semo and seas had open before, kept open with its
 * planets' file data -- the part of plan_data before teval: constants,
 * reference ellipse, loaded segment -- so reads alternating across a join,
 * as a light time across it makes them, swap files instead of reopening
 * one (UPSTREAM-BUGS 89). Each body's slot belongs to one of the three
 * files. */
#define PLAN_FILE_PART offsetof(struct plan_data, teval)

static int file_of_body(int i)
{
  if (i == SEI_MOON)
    return SEI_FILE_MOON;
  if (i == SEI_ANYBODY)
    return -1;
  if (i >= SEI_CHIRON)
    return SEI_FILE_MAIN_AST;
  return SEI_FILE_PLANET;
}

/* the open file and its bodies' file data trade places with the spare */
static void file_spare_exchange(swe_ctx *ctx, int ifno)
{
  struct file_data f = ctx->fidat[ifno];
  unsigned char b[PLAN_FILE_PART];
  int i;
  ctx->fidat[ifno] = ctx->fidat_spare[ifno];
  ctx->fidat_spare[ifno] = f;
  for (i = 0; i < SEI_NPLANETS; i++)
    if (file_of_body(i) == ifno) {
      memcpy(b, &ctx->pldat[i], PLAN_FILE_PART);
      memcpy(&ctx->pldat[i], &ctx->pldat_spare[i], PLAN_FILE_PART);
      memcpy(&ctx->pldat_spare[i], b, PLAN_FILE_PART);
    }
}

static void file_spare_drop(swe_ctx *ctx, int ifno)
{
  int i;
  if (ctx->fidat_spare[ifno].fptr != NULL)
    fclose(ctx->fidat_spare[ifno].fptr);
  memset((void *) &ctx->fidat_spare[ifno], 0, sizeof(struct file_data));
  for (i = 0; i < SEI_NPLANETS; i++)
    if (file_of_body(i) == ifno) {
      free((void *) ctx->pldat_spare[i].segp);
      free((void *) ctx->pldat_spare[i].refep);
      memset((void *) &ctx->pldat_spare[i], 0, sizeof(struct plan_data));
    }
}

/* the open file closed, its bodies' file data with it (UPSTREAM-BUGS 100) */
static void file_close_open(swe_ctx *ctx, int ifno)
{
  int i;
  if (ctx->fidat[ifno].fptr != NULL)
    fclose(ctx->fidat[ifno].fptr);
  memset((void *) &ctx->fidat[ifno], 0, sizeof(struct file_data));
  for (i = 0; i < SEI_NPLANETS; i++)
    if (file_of_body(i) == ifno) {
      free((void *) ctx->pldat[i].segp);
      free((void *) ctx->pldat[i].refep);
      memset((void *) &ctx->pldat[i], 0, PLAN_FILE_PART);
    }
}

/* A file read_const() or get_new_segment() refuses, closed and cleared
 * alone: its slot and its bodies' file data, and the shared slot of the
 * numbered asteroids and planetary moons only if this file wrote to it.
 * Upstream ran free_planets(), emptying the constants of every file left
 * open (UPSTREAM-BUGS 104). */
static void file_reject(swe_ctx *ctx, int ifno, AS_BOOL anybody)
{
  struct plan_data *pdp = &ctx->pldat[SEI_ANYBODY];
  file_close_open(ctx, ifno);
  if (anybody && (ifno == SEI_FILE_ANY_AST || ifno == SEI_FILE_PLMOON)) {
    free((void *) pdp->segp);
    free((void *) pdp->refep);
    memset((void *) pdp, 0, PLAN_FILE_PART);
  }
}

/* the file answers tjd: inside its data and the nominal span it is for */
static AS_BOOL file_answers(const struct file_data *f, double tjd)
{
  return f->fptr != NULL && tjd >= f->tfstart && tjd <= f->tfend
    && tjd >= f->tnom0 && tjd < f->tnom1;
}
#endif

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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 89: an open file was kept for any instant inside its
   * data, and the files run past their nominal 600-year ends (sepl 2-89
   * days, seas about 350, semo 1-30; semo also starts 1-27 days early).
   * So a context warm from one side of a join answered just across it
   * from that side's file, and a fresh context from the file named for
   * the instant: JD 2378496.6 after a call 1.5 days before, the
   * barycentric Sun 47.6 mas, Juno 8.7 mas, the Moon up to 220 mas across
   * a join's light time, the barycentric Sun 1.03 arcsec at -7800, and at
   * 1800.0 itself Vulcan's heliocentric speed 8.2e-7 deg/day. Now an open
   * file answers only inside the nominal span it was opened for, as a
   * fresh context's would; every read, the light time's t - tau included,
   * comes from the file of its own instant (sepl and seas start exactly
   * at the join, so no one file holds both reads of a light time across
   * it). The file it replaces is kept as a spare, so going back is a swap:
   * 4.1 us a call inside a join's light time, 33 reopening. */
  if (ifno <= SEI_FILE_MAIN_AST) {
    if (!file_answers(fdp, tjd)) {
      if (file_answers(&ctx->fidat_spare[ifno], tjd)) {
        file_spare_exchange(ctx, ifno);
      } else if (fdp->fptr != NULL) {
        file_spare_drop(ctx, ifno);
        file_spare_exchange(ctx, ifno);	/* the open one is the spare */
      }
    }
  } else
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    int ineighbour = 0;
    char serr_own[AS_MAXCH];	/* why the instant's own file did not open */
    *serr_own = '\0';
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
      /* The file named for this instant's 600-year bucket is not here --
       * but the neighbouring bucket's file may be, and its data runs a
       * little past its nominal range. An OPEN file is kept for any
       * instant inside its data (the range check above), so a context
       * that had just computed on the neighbour answered this instant and
       * a fresh one did not: the same question, two answers, by call
       * order (2026-09-16 review, F11; measured at JD 2597730, a Sun that
       * answered inside a window starting in 2399 and failed alone).
       * Asking the neighbour here makes the fresh answer the warm one;
       * the range check below still refuses an instant it does not
       * cover. Planets, the Moon and the main asteroid files only: a
       * numbered asteroid's or a planetary moon's file is one file. */
      if (ipli < SE_PLMOON_OFFSET && ipl != SEI_ANYBODY) {
        if (ineighbour == 0 && serr != NULL)
          snprintf(serr_own, AS_MAXCH, "%s", serr);
        while (ineighbour < 2) {
          swi_gen_filename(tjd + (ineighbour++ == 0 ? -3652.5 : 3652.5),
                           ipli, s);
          if (strcmp(s, fname) != 0)
            goto again;
        }
        /* Neither neighbour is here either: report the instant's own
         * file, not the last neighbour tried (2026-10-25 review, D6:
         * semo_12 and semo_06 missing, the Moon at 1200 said
         * "semo_06.se1 not found"). */
        if (serr != NULL && *serr_own != '\0')
          snprintf(serr, AS_MAXCH, "%s", serr_own);
      }
#endif
      return(NOT_AVAILABLE);
    }
    /* during the search error messages may have been built, delete them */
    if (serr != NULL) *serr = '\0';	
    retc = read_const(ctx, ifno, serr);
    if (retc != OK)
      return(retc);
#ifndef SWE_UPSTREAM_COMPAT
    /* the span of the instant's own bucket, if a neighbour answers it */
    if (ipli < SE_PLMOON_OFFSET && ipl != SEI_ANYBODY) {
      /* Upstream bug 100 (this fork's, F11): a neighbour that does not
       * hold the instant either was left open by the range check below, so
       * the file looked present: sweplan(), which takes "the moon file is
       * not open" as its sign to use the Moshier Moon, went on with a Moon
       * it never read -- NaN with no error fresh, the last read's Moon warm
       * (semo_12 missing, JD 2159377.5: 26.44 deg for 349.29). Closed now,
       * and the instant's own file reported missing, as with no neighbour
       * at all. */
      if (ineighbour > 0 && (tjd < fdp->tfstart || tjd > fdp->tfend)) {
        file_close_open(ctx, ifno);
        if (serr != NULL)
          snprintf(serr, AS_MAXCH, "%s", serr_own);
        return(NOT_AVAILABLE);
      }
      file_nominal_span(tjd, ipli, fname, &fdp->tnom0, &fdp->tnom1);
    } else {
      fdp->tnom0 = -HUGE_VAL;	/* one file for all time */
      fdp->tnom1 = HUGE_VAL;
    }
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* An instant exactly on a segment boundary is inside two segments, and
   * the range test below keeps whichever is loaded -- while a fresh
   * context loads the one get_new_segment() computes, the segment that
   * STARTS there. Adjacent fits agree only to ~1e-10 degrees at the join,
   * so the same question had two answers by call order: Phobos at JD
   * 2451545.5, where the light-time step of the first call leaves the
   * PREVIOUS segment loaded for every call after it (2026-09-16 review,
   * F9). Choosing by the same arithmetic as get_new_segment() makes the
   * warm answer the fresh one. Not at the file's last instant, whose
   * index is one past the last segment. */
  if (pdp->segp == NULL || tjd < pdp->tseg0 || tjd > pdp->tseg1
      || (tjd < pdp->tfend
          && pdp->tfstart + (int32) ((tjd - pdp->tfstart) / pdp->dseg)
             * pdp->dseg != pdp->tseg0)) {
#else
  if (pdp->segp == NULL || tjd < pdp->tseg0 || tjd > pdp->tseg1) {
#endif
    /* Upstream bug 106: a file that does not list the body asked for left
     * it the constants of no file (dseg 0, a segment index of inf) or
     * those of the file read before. The file answers for the bodies it
     * lists; this one is refused, fresh or warm alike. */
    for (i = 0; i < fdp->npl && fdp->ipl[i] != ipli; i++)
      ;
    if (i == fdp->npl) {
      if (serr != NULL)
        snprintf(serr, AS_MAXCH, "Ephemeris file %.200s does not contain body %d. ", fdp->fnam, ipli);
      return(ERR);
    }
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 24: the files' argument is TDB, and tjd is TT. Added to
   * the offset into the segment, not to tjd, which would round it to the
   * 40 us a JD resolves; an instant within 1.66 ms of a segment's end
   * evaluates a hair outside [-1, 1], where the polynomial is continuous. */
  t = ((tjd - pdp->tseg0) + swi_tdb_minus_tt(ctx, tjd)) / pdp->dseg;
#else
  t = (tjd - pdp->tseg0) / pdp->dseg;
#endif
  t = t * 2 - 1;
  /* speed is needed, if
   * 1. true position is being computed before applying light-time etc.
   *    this is the position saved in pdp->x.
   *    in this case, speed is needed for light-time correction.
   * 2. the speed flag has been specified.
   */
  need_speed = (do_save || (iflag & SEFLG_SPEED));
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 73: read at TDB (bug 24), the derivative is per day of
   * TDB, and positions are functions of TT: per TT day it is that times
   * dTDB/dTT = 1 + d(TDB - TT)/dTT, up to 3.3e-10 more, 5e-9 deg/day of
   * the Moon's speed. */
  {
    double vfac = need_speed ? (1 + swi_tdb_minus_tt_rate(ctx, tjd)) / pdp->dseg * 2 : 0;
    for (i = 0; i <= 2; i++) {
      xp[i]  = swi_echeb (t, pdp->segp+(i*pdp->ncoe), pdp->neval);
      if (need_speed)
        xp[i+3] = swi_edcheb(t, pdp->segp+(i*pdp->ncoe), pdp->neval) * vfac;
      else
        xp[i+3] = 0;
    }
  }
#else
  for (i = 0; i <= 2; i++) {
    xp[i]  = swi_echeb (t, pdp->segp+(i*pdp->ncoe), pdp->neval);
    if (need_speed) {
      xp[i+3] = swi_edcheb(t, pdp->segp+(i*pdp->ncoe), pdp->neval) / pdp->dseg * 2;
    } else {
      xp[i+3] = 0;	/* von Alois als billiger fix, evtl. illegal */
    }
  }
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 71: 0 is JD 0, where the slot's Earth, of its own
     * instant, passed for the EMB */
    pedp->teval = SWI_TJD_NEVER;
#else
    pedp->teval = 0;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  if (ephepath == ctx->ephepath)
    swi_ephe_path_name_first_use(ctx);	/* bug 107 */
#endif
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
  if (serr != NULL)
    snprintf(serr, AS_MAXCH, "SwissEph file '%.100s' not found in PATH '%.100s'", fname, ephepath);
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
#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bugs 102, 109: a heliocentric Moshier state at teval + s into
 * the frame of the Sun at teval with barycentric velocities, the Sun's
 * displacement s v_S + s^2 a_S / 2 and its velocity v_S + s a_S added */
static void mosh_sun_shift(double *x, const double *vs, const double *as, double s)
{
  int i;
  for (i = 0; i <= 2; i++) {
    x[i] += s * (vs[i] + 0.5 * s * as[i]);
    x[i+3] += vs[i] + s * as[i];
  }
}

/* Upstream bug 125: the body read at t took the Sun's motion over every
 * light time by that Taylor step, VEA's 462 days (seorbel.txt's set at
 * 80000 AU) as a planet's hours. Over that the Sun turns 0.67 rad of its
 * loop about the barycentre, and the step left out up to 5.7e-4 AU of its
 * place and 3e-6 AU/day of its velocity (-3000..+3000): VEA 2e-7 deg off
 * its light time's fixed point (G27 1.1e-3"), its speed 5.4e-9 deg/day
 * off its positions (G65), the same on swe_calc_orbel()'s path. Past a
 * day of light time (a body beyond 173 AU) the Sun's place and velocity
 * at t are now the planets' own there, evaluated aside of the memos.
 * Within a day the step is at most 2.0e-11 AU and 5.9e-11 AU/day short,
 * 6.5e-12 deg and 2e-11 deg/day at that distance (the jump at the
 * switch), and the planets, under 0.3 day, keep its cost. The aberration
 * rate's observer (xobs2) keeps the step: its velocity is V - tau a by
 * design, the observer's acceleration at teval. */
#define MOSH_SUN_TAYLOR_MAX 1.0	/* day */
static void mosh_sun_move(swe_ctx *ctx, double *x, const double *xsb,
                          const double *as, double teval, double t)
{
  double s = t - teval, xs[6];
  int i;
  if (!(fabs(s) > MOSH_SUN_TAYLOR_MAX)) {
    mosh_sun_shift(x, xsb + 3, as, s);
    return;
  }
  swi_mosh_sun_bary_aside(ctx, t, SWI_MOSH_SUN_FULL, xs);
  for (i = 0; i <= 2; i++) {
    x[i] += xs[i] - xsb[i];
    x[i+3] += xs[i+3];
  }
}
#endif

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
#ifndef SWE_UPSTREAM_COMPAT
  /* bug 102: Moshier's light time in a frame where the Sun moves; bug
   * 109: with aberration too */
  AS_BOOL mosh_bary = epheflag == SEFLG_MOSEPH && !(iflag & SEFLG_TRUEPOS);
  double xsb[6] = {0, 0, 0, 0, 0, 0}, asun[3] = {0, 0, 0};
  const double *vsun = xsb + 3;
#endif
  dtsave_for_defl = 0;	
  /* Upstream bug 90 (-Og): dt is set by the light-time loop below, which
   * runs at least once (niter is 1 or 0, from j = 0), but -Og cannot see it
   * and warned at the bug 63 and 78 lines. Initialised; no result changes. */
  dt = 0;
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
#ifdef SWE_UPSTREAM_COMPAT	/* bug 71: no instant is special */
      || ctx->topd.teval == 0
#else				/* bug 92: nor is any ephemeris */
      || ctx->topd.iflgkey != SWI_TOPD_KEY(iflag)
#endif
      ) {
      if (swi_get_observer(ctx, pedp->teval, iflag | SEFLG_NONUT, DO_SAVE, xobs, serr) != OK)
        return ERR;
    } else {
      for (i = 0; i <= 5; i++)
        xobs[i] = ctx->topd.xobs[i];
    }
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++)
      xobs[i] = xobs[i] + pedp->x[i];	
#ifndef SWE_UPSTREAM_COMPAT
  } else if (iflag & SEFLG_BARYCTR) {
    /* Light time is measured from the centre the position is referred to:
     * the barycentre here, the Sun below. Upstream left xobs at the Earth
     * for both and skipped it in the light-time loops, see there. */
    for (i = 0; i <= 5; i++)
      xobs[i] = 0;
  } else if (iflag & SEFLG_HELCTR) {
    /* under Moshier the stored positions are heliocentric already */
    for (i = 0; i <= 5; i++)
      xobs[i] = (pdp->iephe == SEFLG_JPLEPH || pdp->iephe == SEFLG_SWIEPH)
                ? ctx->pldat[SEI_SUNBARY].x[i] : 0;
#endif
  } else {
    /* barycentric position of geocenter */
    for (i = 0; i <= 5; i++)
      xobs[i] = pedp->x[i];
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 102: Moshier's positions are heliocentric, and its light
   * time was solved in that frame, X(t - tau) - O(t) with the Sun held
   * still: it left out the Sun's motion over the light time, v_S tau,
   * 3.0e-6 deg (v_S / c) on every body without aberration and every
   * heliocentric one (an asteroid, read from the same file as on the
   * Swiss path, was 3.0e-6 deg off it heliocentric, its true places
   * agreeing to 1e-14). With aberration it cancels against the
   * aberration taken with the heliocentric Earth's velocity, the
   * direction depending on the relative velocity only (6.6e-10 deg from
   * the barycentric solution), but not the distance: rho . v_S tau, 5e-8
   * AU on the Sun to 2.4e-6 on Pluto (upstream bug 109). Now, with or
   * without aberration (bug 109), the light time is solved
   * with the origin at the Sun at teval and barycentric velocities, as
   * on the files: v_S and a_S from Moshier's own planets
   * (swi_mosh_sun_bary()), every velocity at teval gains v_S, the body
   * read at t moves by the Sun's displacement s v_S + s^2 a_S / 2,
   * s = t - teval, and the heliocentric centre is the Sun at teval,
   * (0, v_S). Asteroids heliocentric 2.5e-10 deg from the files after. */
  if (mosh_bary) {
    swi_mosh_sun_bary(ctx, pdp->teval, SWI_MOSH_SUN_FULL, xsb, asun);
    for (i = 0; i <= 2; i++) {
      xx0[i+3] += vsun[i];
      xobs[i+3] += vsun[i];
    }
  }
#endif
  /*******************************
   * light-time geocentric       * 
   *******************************/
  if (!(iflag & SEFLG_TRUEPOS)) {
    /* number of iterations - 1 */
#ifndef SWE_UPSTREAM_COMPAT
    /* two passes on every ephemeris: Moshier is read at t - dt below as
     * well now (bug 53). One pass left dt off by dt rdot / c, 1e-4 of it:
     * the place by v dt rdot / c, and the aberration rate, the Earth's
     * velocity change over a dt the place does not have, by 1e-4 of the
     * Earth's acceleration (5.7e-8 deg/day on topocentric Mercury). */
    niter = 1;
#else
    if (pdp->iephe == SEFLG_JPLEPH || pdp->iephe == SEFLG_SWIEPH) {
      niter = 1;
    } else { 	/* SEFLG_MOSEPH or planet from osculating elements */
      niter = 0;
    }
#endif
#ifndef SWE_UPSTREAM_COMPAT
    /* Every estimate the loops below form is xx0 - dt * v, with xx0 the
     * BARYCENTRIC position, and the distance they measure is estimate
     * minus xobs. Upstream measured the first pass from xx, which for a
     * heliocentric request already had the Sun taken out (above, for the
     * SEFLG_TRUEPOS answer), and the second from xx0 - dt * v with nothing
     * taken out, so the light time it kept was the barycentric distance
     * over c: 0.84" on Mercury, 0.28" on Venus, 0.09" on Mars against JPL
     * Horizons' Sun-centred places (UPSTREAM-BUGS.md 26, G36). Start from
     * the barycentric vector; xobs is the Sun. */
    for (i = 0; i <= 5; i++)
      xx[i] = xx0[i];
#endif
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
#ifdef SWE_UPSTREAM_COMPAT
	  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
#endif
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
#ifdef SWE_UPSTREAM_COMPAT
	if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
	/* Upstream bug 53: Moshier kept x(t) - dt v(t), the body moved back
	 * along its velocity, and read the series at t - dt for the speed
	 * alone (geocentric only): the place off by dt^2 a / 2 and, with
	 * one pass of the light time, by v dt rdot / c (13.0 mas on
	 * Mercury, 0.09 on Chiron), a heliocentric speed without dt a
	 * (Mercury's distance rate 5.6e-6 AU/day), and SEFLG_CENTER_BODY's
	 * offset, in x(t) already, added again below (4.7e-7 AU on
	 * Jupiter). Read at t - dt like the files, with or without
	 * SEFLG_SPEED; the Earth there only for the speed. With SEFLG_SPEED
	 * this is the call that was made already. */
	{
	  AS_BOOL need_earth = (iflag & SEFLG_SPEED)
	    && !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR));
	  if (ibody == IS_PLANET) {
	    retc = swi_moshplan(ctx, t, ipli, NO_SAVE, xx, need_earth ? xearth : NULL, serr);
	  } else {		/* asteroid, heliocentric under Moshier */
	    retc = sweph(ctx, t, ipli, ifno, iflag, NULL, NO_SAVE, xx, serr);
	    if (retc == OK && need_earth)
	      retc = swi_moshplan(ctx, t, SEI_EARTH, NO_SAVE, xearth, xearth, serr);
	  }
	  if (retc != OK)
	    return(retc);
	  /* bug 102: the Sun's displacement, see above (bug 125: the
	   * Sun's own past a day); xearth stays heliocentric for the
	   * aberration rate's observer (bug 109) */
	  if (mosh_bary)
	    mosh_sun_move(ctx, xx, xsb, asun, pdp->teval, t);
	}
	break;
#else
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
#endif
    }
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 63: the body was read at t = teval - dt rounded to a
     * double, up to half an ulp of the date (2.3e-10 day near the present)
     * from the emission instant, so the light-time position jumped by its
     * motion over that from one date to the next, ~1e-9 deg on Mercury:
     * 4.7e-7 deg/day in a five-point difference over 2^-9 day of its
     * heliocentric places, 2.2e-7 geocentric, against 7e-10 for its true
     * positions (G62). Move it along its velocity at teval (xx0, there
     * with or without SEFLG_SPEED) by the rounding, (teval - t) - dt,
     * exact since t is near teval (as the Moon's, bug 23). Moshier reads
     * the body at that rounded t too since bug 53, so it takes the same. */
    {
      double eps = (pdp->teval - t) - dt;
      for (i = 0; i <= 2; i++)
        xx[i] += eps * xx0[i+3];
    }
    /* Upstream bug 78: the light time was solved along a straight line,
     * xx0 - dt * v, in two passes, and never checked against the body read
     * at t, which is off that line by its curvature over the light time:
     * 7.8e-10 deg on Mercury, 1.1e-10 on Venus, against the fixed point of
     * tau = |X(t - tau) - O(t)| / c (G77). One Newton step on that equation
     * from the body read, f = tau - r / c, f' = 1 + rho^.v / c, moves it
     * along its velocity by the change: 7e-13 deg after. */
    {
      double rho[3], r, rv = 0, del;
      const double c = CLIGHT * 86400.0 / AUNIT;   /* AU per day */
      for (i = 0; i <= 2; i++)
        rho[i] = xx[i] - xobs[i];
      r = sqrt(square_sum(rho));
      for (i = 0; i <= 2; i++)
        rv += rho[i] * xx0[i+3];
      del = (r / c - dt) / (1 + rv / (r * c));
      for (i = 0; i <= 2; i++)
        xx[i] -= del * xx0[i+3];
      dt += del;
      dtsave_for_defl = dt;
    }
#endif
    calc_center_body(ipli, iflag, xx, xcom, serr);
    if (iflag & SEFLG_HELCTR) {
      if (pdp->iephe == SEFLG_JPLEPH || pdp->iephe == SEFLG_SWIEPH) 
	for (i = 0; i <= 5; i++) 
	  xx[i] -= ctx->pldat[SEI_SUNBARY].x[i];
#ifndef SWE_UPSTREAM_COMPAT
      else if (mosh_bary)	/* bug 102: the Sun at teval, (0, v_S) */
	for (i = 0; i <= 5; i++)
	  xx[i] -= xobs[i];
#endif
    }
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 84 (latent): JPL and Moshier read no Earth at t for a
     * heliocentric or barycentric request, and xobs2 was built from the
     * uninitialised xearth all the same. Only the aberration rate reads
     * xobs2, and plaus_iflag() turns aberration off for both centres, so
     * nothing reached a result (bit-identical with the locals poisoned
     * with NaN); not built where there is no Earth. */
    if ((iflag & SEFLG_SPEED) && !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR))) {
#else
    if (iflag & SEFLG_SPEED) {
#endif
      /* observer position for t(light-time) */
      if (iflag & SEFLG_TOPOCTR) {
#ifndef SWE_UPSTREAM_COMPAT
        /* bug 109: built heliocentric, then the Sun's motion */
        double xoh[6];
        for (i = 0; i <= 5; i++)
          xoh[i] = xobs[i] - (i >= 3 ? vsun[i-3] : 0);
        if (observer_at_lt(ctx, pedp->teval, t, iflag, xoh, pedp->x, xearth, xobs2, serr) != OK)
          return ERR;
#else
        if (swi_get_observer(ctx, t, iflag | SEFLG_NONUT, NO_SAVE, xobs2, serr) != OK)
          return ERR;
        for (i = 0; i <= 5; i++)
          xobs2[i] += xearth[i];
#endif
      } else {
#ifndef SWE_UPSTREAM_COMPAT
        earth_at_lt(ctx, pedp->teval, t, pedp->x, xearth, xobs2);
#else
        for (i = 0; i <= 5; i++)
          xobs2[i] = xearth[i];
#endif
      }
#ifndef SWE_UPSTREAM_COMPAT
      if (mosh_bary)
        mosh_sun_shift(xobs2, vsun, asun, t - pedp->teval);
#endif
    }
  }
  /*******************************
   * conversion to geocenter     * 
   *******************************/
  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR)) {
#ifndef SWE_UPSTREAM_COMPAT
    if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_TRUEPOS))
      lt_rate_exact(xx, xobs, xxsp);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Heliocentric and barycentric light times change too, and upstream
   * left their rate out entirely: 1.4e-4 deg/day on Mercury (bug 35).
   * Upstream bug 63: xx is relative to the centre at teval here, and the
   * rate was taken from it with the centre at the origin, so its rho^.v_P
   * and v_P dtau/dt had the Sun's barycentric velocity taken out: 3e-8
   * deg/day on Mercury's heliocentric speed. The rate wants the body's
   * barycentric velocity; xobs is the centre, barycentric (the Sun, or
   * zero for the barycentre and for Moshier's heliocentric positions). */
  else if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_TRUEPOS)) {
    double xb[6];
    for (i = 0; i <= 5; i++)
      xb[i] = xx[i] + xobs[i];
    lt_rate_exact(xb, xobs, xxsp);
    for (i = 3; i <= 5; i++)
      xx[i] -= xxsp[i-3];
  }
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    if (iflag & SEFLG_SPEED)
      add_aberr_rate(xx, xobs, xobs2);
#else
    if (iflag & SEFLG_SPEED) {
      for (i = 3; i <= 5; i++) 
	xx[i] += xobs[i] - xobs2[i];
    }
#endif
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
  return app_pos_rest(ctx, pdp, pdp->teval, iflag, xx, xxsv, oe, serr);
}

/* tdate: the instant the place is of, for the frame's and the ayanamsa's
 * rates; pdp->teval, except for the barycentric Sun (bug 86). */
static int app_pos_rest(swe_ctx *ctx, struct plan_data *pdp, double tdate, int32 iflag, 
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
#ifndef SWE_UPSTREAM_COMPAT
  /* the ecliptic of date turns as its obliquity changes (bug 33); J2000's
   * does not */
  if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_J2000))
    swi_ecl_frame_speed(xx, swi_obliquity_rate(ctx, tdate, iflag,
                                               !(iflag & SEFLG_NONUT)));
#endif
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
      if (swi_get_ayanamsa_with_speed(ctx, tdate, iflag, daya, serr) == ERR)
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
  swi_sid_mode_models(sip, ctx->astro_models);
  swi_invalidate_models(ctx);
  swi_config_end_apply(ctx, swi_cfg_was);
  swi_config_publish(ctx, SWI_CFG_SID);
}

/* The models swe_set_sid_mode_r() sets for SE_SIDBIT_PREC_ORIG, from the
 * mode it stored: the ayanamsa's own precession and its nutation. The
 * setter's code, moved here so that swe_close(), which keeps the mode and
 * resets the models, gives the mode its models back (UPSTREAM-BUGS 127).
 * sip->sid_mode % SE_SIDBITS is the setter's sid_mode wherever the stored
 * mode still has the bit. */
void swi_sid_mode_models(const struct sid_data *sip, int32 *models)
{
  int32 sid_mode = sip->sid_mode % SE_SIDBITS;
  if (sid_mode < SE_NSIDM_PREDEF && (sip->sid_mode & SE_SIDBIT_PREC_ORIG) && ayanamsa[sid_mode].prec_offset > 0) {
    models[SE_MODEL_PREC_LONGTERM] = ayanamsa[sid_mode].prec_offset;
    models[SE_MODEL_PREC_SHORTTERM] = ayanamsa[sid_mode].prec_offset;
    // add a corresponding nutation model
    switch(ayanamsa[sid_mode].prec_offset) {
      case SEMOD_PREC_NEWCOMB:
        models[SE_MODEL_NUT] = SEMOD_NUT_WOOLARD;
	break;
      case SEMOD_PREC_IAU_1976:
        models[SE_MODEL_NUT] = SEMOD_NUT_IAU_1980;
	break;
      default:
        break;
    }
  }
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
void CALL_CONV swe_set_sid_mode(int32 sid_mode, double t0, double ayan_t0)
{
  swe_set_sid_mode_r(swi_default_ctx(), sid_mode, t0, ayan_t0);
}

static int32 swe_get_ayanamsa_ex_r_body(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr)
{
  struct nut nuttmp;
  struct nut *nutp = &nuttmp;	/* dummy assign, to silence gcc warning */
  int32 retval = swi_get_ayanamsa_ex(ctx, tjd_et, iflag, daya, serr);
  /* Upstream bug 142: the nutation took SEFLG_JPLHOR without the IERS
   * corrections, which swi_get_ayanamsa_ex()'s plaus_iflag() turns into
   * SEFLG_JPLHOR_APPROX, and read their table, NULL: on de200, the
   * ayanamsa, its _ut form and the sidereal houses with SEFLG_JPLEPH |
   * SEFLG_JPLHOR crashed (SIGSEGV at J2000). As swe_calc() takes it now
   * (swi_jplhor_plaus()), the warning in serr as there. Both builds. */
  iflag = swi_jplhor_plaus(ctx, iflag);
  if (!(iflag & SEFLG_NONUT)) {
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 134: the nutation kept at the instant was taken for
     * its instant alone, of whatever JPL Horizons flags computed it: on
     * de200 at 1950 + 0.37, Mars with SEFLG_JPLHOR_APPROX and then the
     * ayanamsa without it (True Citra, Lahiri) answered the flag's
     * nutation, 6.19e-2" off, and the reverse. Keyed as
     * swi_check_nutation() keys it now. */
    if (tjd_et == ctx->nut.tnut
        && SWI_EPS_KEY(ctx->sp.nut.nutflag) == SWI_EPS_KEY(iflag)) {
#else
    if (tjd_et == ctx->nut.tnut) {
#endif
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

int32 CALL_CONV swe_get_ayanamsa_ex_r(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_get_ayanamsa_ex_r_body(ctx, tjd_et, iflag, daya, serr);
  swi_config_release(ctx);
  return ret;
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

/* swi_get_ayanamsa_ex()'s body. dspeed, when not NULL, receives the
 * ayanamsa's rate in deg/day, the derivative of the same computation; it
 * is non-NULL only from swi_get_ayanamsa_with_speed() (UPSTREAM-BUGS 91),
 * and the value does not depend on it. */
static int32 get_ayanamsa(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, double *dspeed, char *serr)
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
  /* _TRUE_ ayanamshas can have the following SEFLG_s;
   * The star will have the intended fixed position even if these flags are 
   * provided */
  iflag_true = iflag;
  if (otherflag & SEFLG_TRUEPOS) iflag_true |= SEFLG_TRUEPOS;
  if (otherflag & SEFLG_NOABERR) iflag_true |= SEFLG_NOABERR;
  if (otherflag & SEFLG_NOGDEFL) iflag_true |= SEFLG_NOGDEFL;
  if (dspeed != NULL) {
    iflag_galequ |= SEFLG_SPEED;
    iflag_true |= SEFLG_SPEED;
  }
  /* warning, if swe_set_ephe_path() or swe_set_jplfile() was not called yet,
   * although ephemeris files are required */
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) 
     && (sid_mode ==  SE_SIDM_TRUE_CITRA 
      || sid_mode == SE_SIDM_TRUE_REVATI 
      || sid_mode == SE_SIDM_TRUE_PUSHYA 
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 97: the unmasked mode, here alone of the list; equal
       * in effect, swe_set_sid_mode() keeping no SIDBITs on a star mode */
      || sid_mode == SE_SIDM_TRUE_SHEORAN 
#else
      || sip->sid_mode == SE_SIDM_TRUE_SHEORAN 
#endif
      || sid_mode == SE_SIDM_TRUE_MULA 
      || sid_mode == SE_SIDM_GALCENT_0SAG
      || sid_mode == SE_SIDM_GALCENT_COCHRANE
      || sid_mode == SE_SIDM_GALCENT_RGILBRAND 
      || sid_mode == SE_SIDM_GALCENT_MULA_WILHELM
      || sid_mode == SE_SIDM_GALEQU_IAU1958 
      || sid_mode == SE_SIDM_GALEQU_TRUE
      || sid_mode == SE_SIDM_GALEQU_MULA) 
#ifndef SWE_UPSTREAM_COMPAT
      && serr != NULL && swi_ephe_path_none(ctx)) {	/* bug 110 */
#else
      && serr != NULL) {
#endif
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
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_TRUE_REVATI) {
    strcpy(star, ",zePsc"); /* Revati */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 359.8333333333);
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_TRUE_PUSHYA) {
    strcpy(star, ",deCnc"); /* Pushya = Asellus Australis */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 106);
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_TRUE_SHEORAN) {
    strcpy(star, ",deCnc"); /* Asellus Australis */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 103.49264221625);
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_TRUE_MULA) {
    strcpy(star, ",laSco"); /* Mula = lambda Scorpionis */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 240);
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode ==  SE_SIDM_GALCENT_0SAG) {
    strcpy(star, ",SgrA*"); /* Galactic Centre */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 240.0);
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
    /*return swe_degnorm(x[0] - 359.83333333334);*/
  }
  if (sid_mode ==  SE_SIDM_GALCENT_COCHRANE) {
    strcpy(star, ",SgrA*"); /* Galactic Centre */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 270.0);
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
    /*return swe_degnorm(x[0] - 359.83333333334);*/
  }
  if (sid_mode ==  SE_SIDM_GALCENT_RGILBRAND) {
    strcpy(star, ",SgrA*"); /* Galactic Centre */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_true, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 210.0 - 90.0 * 0.3819660113);
    if (dspeed != NULL)
      *dspeed = x[3];
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
#ifndef SWE_UPSTREAM_COMPAT
    if (dspeed != NULL) {
      /* tan mc = tan ra / cos eps, differentiated */
      double sa = sin(x[0] * DEGTORAD), ca = cos(x[0] * DEGTORAD);
      double se = sin(eps * DEGTORAD), ce = cos(eps * DEGTORAD);
      double edot = swi_obliquity_rate(ctx, tjd_et, iflag, FALSE) * RADTODEG;
      *dspeed = (ce * x[3] + sa * ca * se * edot) / (ce * ce * ca * ca + sa * sa);
    }
#endif
    return (retflag & SEFLG_EPHMASK);
    /*return swe_degnorm(x[0] - 359.83333333334);*/
  }
  if (sid_mode == SE_SIDM_GALEQU_IAU1958) {
    strcpy(star, ",GP1958"); /* Galactic Pole IAU 1958 */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_galequ, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 150);
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_GALEQU_TRUE) {
    strcpy(star, ",GPol"); /* Galactic Pole modern, true */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_galequ, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 150);
    if (dspeed != NULL)
      *dspeed = x[3];
    return (retflag & SEFLG_EPHMASK);
  }
  if (sid_mode == SE_SIDM_GALEQU_MULA) {
    strcpy(star, ",GPol"); /* Galactic Pole modern, true */
    if ((retflag = swe_fixstar_r(ctx, star, tjd_et, iflag_galequ, x, serr)) == ERR)
      return ERR;
    *daya = swe_degnorm(x[0] - 150 - 6.6666666667);
    if (dspeed != NULL)
      *dspeed = x[3];
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
#ifndef SWE_UPSTREAM_COMPAT
    /* the vernal point of date, at rest there, moves in J2000 at dP^T/dt */
    if (dspeed != NULL)
      swi_precess_speed(ctx, x, tjd_et, 0, J_TO_J2000);
#endif
    /* to t0 */
    t0 = sip->t0;
    if (sip->t0_is_UT)
      t0 += swe_deltat_ex_r(ctx, t0, iflag, serr);
    swi_precess(ctx, x, t0, 0, J2000_TO_J);
    /* to ecliptic t0 */
    eps = swi_epsiln(ctx, t0, 0);
    swi_coortrf(x, x, eps);
#ifndef SWE_UPSTREAM_COMPAT
    /* t0's frame is fixed: the velocity turns with the position; the
     * longitude's rate is (x vy - y vx) / (x^2 + y^2), the ayanamsa's
     * its negative */
    if (dspeed != NULL) {
      swi_precess(ctx, x + 3, t0, 0, J2000_TO_J);
      swi_coortrf(x + 3, x + 3, eps);
      *dspeed = -(x[0] * x[4] - x[1] * x[3]) / (x[0] * x[0] + x[1] * x[1]) * RADTODEG;
    }
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* the point, at rest in J2000, moves in the date's equator at dP/dt,
     * and in its ecliptic with the mean obliquity's turning too */
    if (dspeed != NULL) {
      double xv[6] = {x[0], x[1], x[2], 0, 0, 0};
      swi_precess_speed(ctx, xv, tjd_et, 0, J2000_TO_J);
      swi_coortrf(xv, xv, eps);
      swi_coortrf(xv + 3, xv + 3, eps);
      swi_ecl_frame_speed(xv, swi_obliquity_rate(ctx, tjd_et, 0, FALSE));
      *dspeed = (xv[0] * xv[4] - xv[1] * xv[3]) / (xv[0] * xv[0] + xv[1] * xv[1]) * RADTODEG;
    }
#endif
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

int32 swi_get_ayanamsa_ex(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr)
{
  return get_ayanamsa(ctx, tjd_et, iflag, daya, NULL, serr);
}

int32 swi_get_ayanamsa_with_speed(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr)
{
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 91: the rate was (a(t) - a(t - 0.001)) / 0.001. In the
   * precession modes that divides the ayanamsa's own rounding, 1e-14 to
   * 7e-14 deg, by a thousandth of a day: 1.3e-11 to 9e-11 deg/day off
   * five-point differences of its values (Lahiri 6.3e-11), on every
   * sidereal speed. In the star modes it is a backward difference of an
   * aberrated, deflected longitude, off by half its curvature times the
   * step: 9e-10 to 1.1e-9 deg/day, 2.1e-6 for True Revati with zeta Psc
   * 0.3 deg from the Sun (1981). Across 0 deg (Lahiri, 285 AD) it took
   * the 360 deg jump: 3.6e5 deg/day for 0.001 day. Now get_ayanamsa()
   * gives the rate with the value: the star modes the star's own speed
   * (Wilhelm's through its projection), the precession modes the vernal
   * point's or the t0 point's velocity carried by swi_precess_speed()
   * through the same rotations, the ecliptic of date's turning included. */
  return get_ayanamsa(ctx, tjd_et, iflag, daya, daya + 1, serr);
#else
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
#endif
}

static int32 swe_get_ayanamsa_ex_ut_r_body(swe_ctx *ctx, double tjd_ut, int32 iflag, double *daya, char *serr)
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

int32 CALL_CONV swe_get_ayanamsa_ex_ut_r(swe_ctx *ctx, double tjd_ut, int32 iflag, double *daya, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_get_ayanamsa_ex_ut_r_body(ctx, tjd_ut, iflag, daya, serr);
  swi_config_release(ctx);
  return ret;
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
static double swe_get_ayanamsa_r_body(swe_ctx *ctx, double tjd_et)
{
  double daya;
  int32 iflag;
#ifndef SWE_UPSTREAM_COMPAT
  swi_init_swed_if_start(ctx);	/* bug 103: the guess reads the configuration */
#endif
  iflag = swi_guess_ephe_flag(ctx);
  // swi... function never includes nutation
  swi_get_ayanamsa_ex(ctx, tjd_et, iflag, &daya, NULL);
  return daya;
}

double CALL_CONV swe_get_ayanamsa_r(swe_ctx *ctx, double tjd_et)
{
  double ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_get_ayanamsa_r_body(ctx, tjd_et);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
double CALL_CONV swe_get_ayanamsa(double tjd_et)
{
  return swe_get_ayanamsa_r(swi_default_ctx(), tjd_et);
}

static double swe_get_ayanamsa_ut_r_body(swe_ctx *ctx, double tjd_ut)
{
  double daya;
  int32 iflag;
#ifndef SWE_UPSTREAM_COMPAT
  swi_init_swed_if_start(ctx);	/* bug 103: the guess reads the configuration */
#endif
  iflag = swi_guess_ephe_flag(ctx);
  swi_get_ayanamsa_ex(ctx, tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, iflag, NULL), 0, &daya, NULL);
  return daya;
}

double CALL_CONV swe_get_ayanamsa_ut_r(swe_ctx *ctx, double tjd_ut)
{
  double ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_get_ayanamsa_ut_r_body(ctx, tjd_ut);
  swi_config_release(ctx);
  return ret;
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
#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 81 (part 3): one Newton step on the light-time equation
 * f(tau) = tau - |X(teval - tau) - O| / c from tau = dt, where the body read
 * at teval - dt is dx from the observer with barycentric velocity v and
 * gives dtNew = |dx| / c: f' = 1 + (dx/|dx|).v / c, so tau = dt + (dtNew -
 * dt) / f'. The caller moves the body along v by the change. v is the
 * body's velocity at teval (pdp->x), there with or without SEFLG_SPEED;
 * the Earth read at t - dt has none without it. */
static double lt_newton_step(const double *dx, const double *v, double dt, double dtNew)
{
  double r = sqrt(square_sum(dx));
  double k = r > 0 ? dot_prod(dx, v) / r * AUNIT / CLIGHT / 86400.0 : 0;
  return dt + (dtNew - dt) / (1 + k);
}
#endif

static int app_pos_etc_plan_osc(swe_ctx *ctx, int ipl, int ipli, int32 iflag, char *serr)
{
  int i, j, niter, retc;
  double xx[6], dx[3], dt, dtsave_for_defl;
#ifndef SWE_UPSTREAM_COMPAT
  double dtNew, acc_e[3];
  int have_acc = FALSE;
  /* bug 109: Moshier's light time in a frame where the Sun moves, as a
   * planet's (bug 102); xv0 the body's velocity at teval in it */
  AS_BOOL mosh_bary = (iflag & SEFLG_MOSEPH) && !(iflag & SEFLG_TRUEPOS);
  double xsb[6] = {0, 0, 0, 0, 0, 0}, asun[3] = {0, 0, 0}, xv0[3];
  const double *vsun = xsb + 3;
#endif
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
#ifdef SWE_UPSTREAM_COMPAT	/* bug 71: no instant is special */
      || ctx->topd.teval == 0
#else				/* bug 92: nor is any ephemeris */
      || ctx->topd.iflgkey != SWI_TOPD_KEY(iflag)
#endif
      ) {
      if (swi_get_observer(ctx, pedp->teval, iflag | SEFLG_NONUT, DO_SAVE, xobs, serr) != OK)
        return ERR;
    } else {
      for (i = 0; i <= 5; i++)
        xobs[i] = ctx->topd.xobs[i];
    }
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++)
      xobs[i] = xobs[i] + pedp->x[i];	
  /* The light-time loops below measure from xobs for every centre (the
   * Sun for a heliocentric request). Upstream set it and then skipped it
   * unless geocentric, so a heliocentric fictitious body had the
   * barycentric light time on both passes (UPSTREAM-BUGS.md 26, G36). */
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

#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 109: bug 102's on the fictitious bodies. Moshier's
   * light time held the Sun still, v_S tau off: 2.6e-6 to 3.0e-6 deg on
   * Cupido, Isis and Vulcan heliocentric and without aberration (their
   * elements the files' own), the apparent distance by rho^.v_S tau.
   * Solved with the origin at the Sun at teval and barycentric
   * velocities: the observer's and the body's velocity at teval gain v_S,
   * the body read at t (with its focus, the Earth at t, for an
   * Earth-centred set) moves by the Sun's displacement, the heliocentric
   * centre is (0, v_S). */
  for (i = 0; i <= 2; i++)
    xv0[i] = pdp->x[i+3];
  if (mosh_bary) {
    swi_mosh_sun_bary(ctx, pedp->teval, SWI_MOSH_SUN_FULL, xsb, asun);
    for (i = 0; i <= 2; i++) {
      xobs[i+3] += vsun[i];
      xv0[i] += vsun[i];
    }
  }
#endif
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
#ifdef SWE_UPSTREAM_COMPAT
	  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
#endif
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
#ifdef SWE_UPSTREAM_COMPAT
	if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    if (iflag & SEFLG_SPEED) {
      /* part of daily motion resulting from change of dt */
      for (i = 0; i <= 2; i++) 
	xxsp[i] = pdp->x[i] - xx[i] - xxsp[i];
    }
    /* The light-time equation solved on the ORBIT, for every flag set.
     * The straight line above, x(t) - dt * v(t), is only a first guess:
     * it ignores the orbit's curvature over dt, and it used to be the
     * whole answer whenever SEFLG_SPEED was off, so the same question
     * gave different positions depending on a flag that should only add
     * columns -- 63.7" at 80,000 AU, 0.095" for the intramercurial Vulcan.
     * Re-evaluating the elements at t - dt also takes an equinox of date
     * at the emission instant, as the ephemeris protocol's 3.5a defines
     * it. Converged to 1e-12 day, about 26 m of light travel. G27. */
    for (j = 0; j < 10; j++) {
      t = pdp->teval - dt;
      retc = main_planet_bary(ctx, t, SEI_EARTH, epheflag, iflag, NO_SAVE, NULL, xearth, xearth, xsun, xmoon, serr);
      /* Upstream bug 96: the Earth at t as the one at teval moved back */
      earth_place_at_lt(ctx, pdp->teval, t, &have_acc, acc_e, xearth);
      if (swi_osc_el_plan(ctx, t, xx, ipl-SE_FICT_OFFSET, ipli, xearth, xsun, serr) != OK)
	return ERR;
      if (retc != OK)
	return(retc);
      if (mosh_bary)	/* bug 109: the Sun's displacement (bug 125) */
        mosh_sun_move(ctx, xx, xsb, asun, pdp->teval, t);
      for (i = 0; i <= 2; i++) {
	dx[i] = xx[i] - xobs[i];   /* xobs: the centre, see app_pos_etc_plan() */
      }
      dtNew = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;
      if (fabs(dtNew - dt) < 1e-12 || j == 9)
	break;
      dt = dtNew;
    }
    /* Upstream bug 81 (part 3): the loop stops within 1e-12 day of the
     * fixed point with the body read at the last dt, not the converged
     * one: 2.3e-11 deg on Waldemath, 1.5e-11 on Selena (their whole
     * barycentric motion, 0.017 AU/day, over 1e-12 day at 0.0065 AU). One
     * Newton step on tau = |X(teval - tau) - O|/c finishes it; the body
     * is moved by the change with the rounding below. */
    dt = lt_newton_step(dx, xv0, dt, dtNew);
    /* Upstream bug 81: as a planet's (bug 63), the body was read at t =
     * teval - dt rounded to a double, up to half an ulp of the date from
     * the emission instant. An Earth-centred set's focus is the Earth at t,
     * which moves 0.0172 AU/day while the observer stays at teval: one ulp
     * (4.7e-10 day) is 8e-12 AU, 7e-8 deg on Waldemath at 0.0065 AU, a
     * sawtooth in its apparent place that a five-point difference over
     * 2^-7 day made 1e-6 to 5.5e-6 deg/day. Moved along the barycentric
     * velocity at teval by the rounding, (teval - t) - dt (with dt now
     * part 3's Newton step, by that change too). */
    {
      double eps = (pdp->teval - t) - dt;
      for (i = 0; i <= 2; i++)
        xx[i] += eps * xv0[i];
    }
    dtsave_for_defl = dt;
    if (iflag & SEFLG_SPEED) {
#else
    if (iflag & SEFLG_SPEED) {
      /* part of daily motion resulting from change of dt */
      for (i = 0; i <= 2; i++) 
	xxsp[i] = pdp->x[i] - xx[i] - xxsp[i];
      t = pdp->teval - dt;
      /* for accuracy in speed, we will need earth as well */
      retc = main_planet_bary(ctx, t, SEI_EARTH, epheflag, iflag, NO_SAVE, NULL, xearth, xearth, xsun, xmoon, serr);
      if (swi_osc_el_plan(ctx, t, xx, ipl-SE_FICT_OFFSET, ipli, xearth, xsun, serr) != OK)
	return ERR;
      if (retc != OK)
	return(retc);
#endif
      if (iflag & SEFLG_TOPOCTR) {
#ifndef SWE_UPSTREAM_COMPAT
        double xoh[6];	/* bug 109: heliocentric, then the Sun's motion */
        for (i = 0; i <= 5; i++)
          xoh[i] = xobs[i] - (i >= 3 ? vsun[i-3] : 0);
        if (observer_at_lt(ctx, pedp->teval, t, iflag, xoh, pedp->x, xearth, xobs2, serr) != OK)
          return ERR;
#else
        if (swi_get_observer(ctx, t, iflag | SEFLG_NONUT, NO_SAVE, xobs2, serr) != OK)
          return ERR;
        for (i = 0; i <= 5; i++)
          xobs2[i] += xearth[i];
#endif
      } else {
#ifndef SWE_UPSTREAM_COMPAT
        earth_at_lt(ctx, pedp->teval, t, pedp->x, xearth, xobs2);
#else
        for (i = 0; i <= 5; i++)
          xobs2[i] = xearth[i];
#endif
      }
#ifndef SWE_UPSTREAM_COMPAT
      if (mosh_bary)
        mosh_sun_shift(xobs2, vsun, asun, t - pedp->teval);
#endif
    }
  }
  /*******************************
   * conversion to geocenter     * 
   *******************************/
#ifndef SWE_UPSTREAM_COMPAT
  if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_TRUEPOS))
    lt_rate_exact(xx, xobs, xxsp);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    if (iflag & SEFLG_SPEED)
      add_aberr_rate(xx, xobs, xobs2);
#else
    if (iflag & SEFLG_SPEED)
      for (i = 3; i <= 5; i++) 
	xx[i] += xobs[i] - xobs2[i];
#endif
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* ICRS to J2000, as app_pos_etc_plan() does for a planet. The elements
   * are dynamical (a mean ecliptic and equinox), so swi_osc_el_plan()
   * carries the body into ICRS before adding the Sun or the Earth, which
   * are ICRS; everything since is ICRS, and this returns it to the frame a
   * planet is answered in. Without the pair, a geocentric fictitious body
   * subtracted an unrotated ICRS Earth from a dynamical body: 15.5 km, a
   * constant 0.023" frame-bias rotation on every one of them. G27. */
  if (!(iflag & SEFLG_ICRS) && swi_get_denum(ctx, SEI_EARTH, pedp->iephe) >= 403)
    swi_bias(ctx, xx, pdp->teval, iflag, FALSE);
#endif
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
  return app_pos_rest(ctx, pdp, pdp->teval, iflag, xx, xxsv, oe, serr);
}

/* The two-body state of caller-supplied orbital elements at one instant
 * (protocol v4 kind 4), referred to the mean ecliptic and equinox of tequ
 * and rotated to the J2000 equator, relative to the centre body. el[] =
 * mean anomaly M (deg), semi-major axis a (AU), eccentricity e, argument
 * of perihelion (deg), ascending node (deg), inclination (deg), all
 * already EVALUATED at tjd; mu = GM of the centre in AU^3/day^2. The same
 * Gaussian-vector and Kepler machinery as swi_osc_el_plan() (swemplan.c),
 * which reads its elements from seorbel.txt; here they come from the
 * caller. The speed is the exact derivative of the two-body motion
 * dM/dt = n = sqrt(mu/a^3), so a call at a retarded epoch with
 * M - n * dt moves along the same motion.
 *
 * of_date: the elements are referred to the equinox of date, tequ the
 * instant evaluated, and the speed carries that frame's turning.
 *
 * Invalid elements are errors, not clamps, as read_elements_file() refuses
 * them for the fictitious bodies. */
static int swi_orbel_to_state(swe_ctx *ctx, double tjd, const double *el,
    double mu, double tequ, AS_BOOL of_date, double *xp, char *serr)
{
  double pqr[9], x[6];
  double xe[3], xr[3];
  int i;
  double eps, K, fac, rho, cose, sine;
  double alpha, beta, zeta, sigma, M2, Msgn, M_180_or_0;
  double mano, sema, ecce, parg, node, incl;
  double cosnode, sinnode, cosincl, sinincl, cosparg, sinparg;
  double M, E;
  (void) tjd;	/* the elements are already evaluated at the instant */
  mano = el[0];
  sema = el[1];
  ecce = el[2];
  parg = el[3];
  node = el[4];
  incl = el[5];
  /* NaN fails every comparison, so one test per quantity */
  if (!(mano == mano) || !(parg == parg) || !(node == node) || !(incl == incl)) {
    if (serr != NULL)
      strcpy(serr, "orbital elements: mean anomaly or orientation angle is not finite.");
    return ERR;
  }
  if (!(sema > 0)) {
    if (serr != NULL)
      strcpy(serr, "orbital elements: semi-major axis invalid.");
    return ERR;
  }
  if (!(ecce >= 0) || !(ecce < 1)) {
    if (serr != NULL)
      strcpy(serr, "orbital elements: eccentricity invalid (no parabolic or hyperbolic orbits allowed).");
    return ERR;
  }
  if (!(incl >= 0) || !(incl <= 180)) {
    if (serr != NULL)
      strcpy(serr, "orbital elements: inclination out of range (0..180 degrees).");
    return ERR;
  }
  cosnode = cos(node * DEGTORAD);
  sinnode = sin(node * DEGTORAD);
  cosincl = cos(incl * DEGTORAD);
  sinincl = sin(incl * DEGTORAD);
  cosparg = cos(parg * DEGTORAD);
  sinparg = sin(parg * DEGTORAD);
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
  /* Kepler problem; M is already the mean anomaly of the instant */
  E = M = swi_mod2PI(mano * DEGTORAD);
  /* better E for very high eccentricity and small M, as in
   * swi_osc_el_plan() */
  if (ecce > 0.975) {
    M2 = M * RADTODEG;
    if (M2 > 150 && M2 < 210) {
      M2 -= 180;
      M_180_or_0 = 180;
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
      zeta = pow(beta + sqrt(beta * beta + alpha * alpha), 1/3);
      sigma = zeta - alpha / 2;
      sigma = sigma - 0.078 * sigma * sigma * sigma * sigma * sigma / (1 + ecce);
      E = Msgn * (M2 + ecce * (3 * sigma - 4 * sigma * sigma * sigma))
			+ M_180_or_0;
    }
  }
  E = swi_kepler(E, M, ecce);
  /* position and speed, referred to orbital plane. K = sqrt(mu/a) plays
   * the KGAUSS / sqrt(sema) of swi_osc_el_plan(): KGAUSS is
   * sqrt(GM_sun), KGAUSS_GEO sqrt(GM_earth). */
  K = sqrt(mu) / sqrt(sema);
  cose = cos(E);
  sine = sin(E);
  fac = sqrt((1 - ecce) * (1 + ecce));
  rho = 1 - ecce * cose;
  x[0] = sema * (cose - ecce);
  x[1] = sema * fac * sine;
  x[2] = x[5] = 0;
  x[3] = -K * sine / rho;
  x[4] = K * fac * cose / rho;
  /* transformation to ecliptic */
  xp[0] = pqr[0] * x[0] + pqr[1] * x[1];
  xp[1] = pqr[3] * x[0] + pqr[4] * x[1];
  xp[2] = pqr[6] * x[0] + pqr[7] * x[1];
  xp[3] = pqr[0] * x[3] + pqr[1] * x[4];
  xp[4] = pqr[3] * x[3] + pqr[4] * x[4];
  xp[5] = pqr[6] * x[3] + pqr[7] * x[4];
  for (i = 0; i <= 2; i++)
    xe[i] = xp[i];
  /* transformation to equator */
  eps = swi_epsiln(ctx, tequ, 0);
  swi_coortrf(xp, xp, -eps);
  swi_coortrf(xp+3, xp+3, -eps);
  /* precess to J2000 */
  if (tequ != J2000) {
    swi_precess(ctx, xp, tequ, 0, J_TO_J2000);
    swi_precess(ctx, xp+3, tequ, 0, J_TO_J2000);
  }
  /* Upstream bug 136 (this fork's swe_calc_orbel()): elements of the
   * equinox of date (SE_ORBEL_EQ_DATE) are referred to a frame that turns
   * with t, and the speed was the two-body motion in the frame held at
   * tequ: the precession's and the obliquity's rates left out, 4.2e-5 to
   * 8.8e-5 deg/day off the same elements as a seorbel.txt JDATE body and
   * off the positions' own rate. Now the turning as swi_osc_el_plan()
   * takes it for JDATE (bug 48), a central difference of the same rotation
   * over +-1 day applied to the ecliptic position. Unguarded, as bug
   * 112's: this function is not upstream's, so there is no upstream
   * behaviour to restore. G126. */
  if (of_date) {
    swi_coortrf(xe, xr, -swi_epsiln(ctx, tequ + 1, 0));
    swi_precess(ctx, xr, tequ + 1, 0, J_TO_J2000);
    swi_coortrf(xe, xe, -swi_epsiln(ctx, tequ - 1, 0));
    swi_precess(ctx, xe, tequ - 1, 0, J_TO_J2000);
    for (i = 0; i <= 2; i++)
      xp[3+i] += (xr[i] - xe[i]) / 2;
  }
  return OK;
}

/* The apparent-position pipeline for caller-supplied orbital elements:
 * app_pos_etc_plan_osc()'s for the fictitious bodies, with three
 * differences. The state comes from the caller's elements, not
 * seorbel.txt; the centre is SE_SUN or SE_EARTH by parameter; and the
 * retarded-epoch state of the speed branch is the same elements' two-body
 * motion (M - n * dt), there being no element polynomial to evaluate
 * again -- light time is carried through the same two-body motion.
 * Results land in the scratch body SEI_ANYBODY, as the fictitious
 * bodies' do; nothing here outlives the call.
 * Returns ERR, or NOT_AVAILABLE / BEYOND_EPH_LIMITS from the ephemeris
 * reads, for the caller to redo with Moshier. */
static int app_pos_etc_orbel(swe_ctx *ctx, double tjd, const double *el,
    double mu, double tequ, AS_BOOL of_date, int32 iplctr, int32 iflag, char *serr)
{
  int i, j, niter, retc;
  double xx[6], dx[3], dt, dtsave_for_defl;
#ifndef SWE_UPSTREAM_COMPAT
  double dtNew, acc_e[3];
  int have_acc = FALSE;
  /* bug 109: Moshier's light time in a frame where the Sun moves, as a
   * planet's (bug 102); xv0 the body's velocity at teval in it */
  AS_BOOL mosh_bary = (iflag & SEFLG_MOSEPH) && !(iflag & SEFLG_TRUEPOS);
  double xsb[6] = {0, 0, 0, 0, 0, 0}, asun[3] = {0, 0, 0}, xv0[3];
  const double *vsun = xsb + 3;
#endif
  double xearth[6], xsun[6], xmoon[6];
  double xxsv[6], xxsp[3]={0}, xobs[6], xobs2[6];
  double t, dmot, elret[6];
  struct plan_data *pdp = &ctx->pldat[SEI_ANYBODY];
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  struct epsilon *oe = &ctx->oec2000;
  int32 epheflag = SEFLG_DEFAULTEPH;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 130, this fork's swe_calc_orbel(): bug 20's defect on
   * this copy of the fictitious bodies' path, which was made before bug 20
   * was fixed there and never got the fix. The elements' state is
   * dynamical J2000, the Sun and the Earth added to it ICRS, and neither
   * was turned: a J2000 answer was seen from the ICRS Earth. Every
   * geocentric and topocentric place was off by (I - B)(S - E), 3.2e-8 to
   * 1.1e-7 AU by the Earth's direction (1.4e-5 deg on an orbit of a = 1.2
   * AU, 2.0e-7 deg at 30 AU), a barycentric one by (I - B) S (3.2e-8 deg),
   * an Earth-centred orbit's heliocentric one by the whole 6.1e-6 deg, and
   * with SEFLG_ICRS the body was left unturned from every centre, 4.9e-6
   * to 6.1e-6 deg (the bias's 23 mas) off the same elements read from
   * seorbel.txt. Now bug 20's pair: the body to ICRS before the centre is
   * added, at teval and at every retarded epoch, and the whole back to
   * J2000 where a planet takes it, on the same condition, so the pair
   * cancels on the body and turns only what came from the ephemeris.
   * G126. */
  AS_BOOL bias = swi_get_denum(ctx, SEI_EARTH, pedp->iephe) >= 403;
#endif
  dt = dtsave_for_defl = 0;	/* dummy assign to silence gcc */
  if (iflag & SEFLG_MOSEPH) {
    epheflag = SEFLG_MOSEPH;
  } else if (iflag & SEFLG_SWIEPH) {
    epheflag = SEFLG_SWIEPH;
  } else if (iflag & SEFLG_JPLEPH) {
    epheflag = SEFLG_JPLEPH;
  }
  /* the two-body state at t, relative to the centre, then to the
   * centre's barycentric position: what swi_osc_el_plan() does with
   * xsun/xearth at its end */
  if (swi_orbel_to_state(ctx, tjd, el, mu, tequ, of_date, pdp->x, serr) != OK)
    return ERR;
#ifndef SWE_UPSTREAM_COMPAT
  if (bias)	/* bug 130 */
    swi_bias(ctx, pdp->x, tjd, SEFLG_SPEED, TRUE);
#endif
  if (iplctr == SE_EARTH) {
    for (i = 0; i <= 5; i++)
      pdp->x[i] += pedp->x[i];
  } else {
    for (i = 0; i <= 5; i++)
      pdp->x[i] += psdp->x[i];
  }
  pdp->teval = tjd;
  pdp->iephe = pedp->iephe;
  /* the mean motion of the elements' own Kepler orbit, deg/day */
  dmot = sqrt(mu / (el[1] * el[1] * el[1])) * RADTODEG;
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
#ifdef SWE_UPSTREAM_COMPAT	/* bug 71: no instant is special */
      || ctx->topd.teval == 0
#else				/* bug 92: nor is any ephemeris */
      || ctx->topd.iflgkey != SWI_TOPD_KEY(iflag)
#endif
      ) {
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

#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 109: bug 102's on the fictitious bodies and swe_calc_orbel(). Moshier's
   * light time held the Sun still, v_S tau off: 2.6e-6 to 3.0e-6 deg on
   * Cupido, Isis and Vulcan heliocentric and without aberration (their
   * elements the files' own), the apparent distance by rho^.v_S tau.
   * Solved with the origin at the Sun at teval and barycentric
   * velocities: the observer's and the body's velocity at teval gain v_S,
   * the body read at t (with its focus, the Earth at t, for an
   * Earth-centred set) moves by the Sun's displacement, the heliocentric
   * centre is (0, v_S). */
  for (i = 0; i <= 2; i++)
    xv0[i] = pdp->x[i+3];
  if (mosh_bary) {
    swi_mosh_sun_bary(ctx, pedp->teval, SWI_MOSH_SUN_FULL, xsb, asun);
    for (i = 0; i <= 2; i++) {
      xobs[i+3] += vsun[i];
      xv0[i] += vsun[i];
    }
  }
#endif
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
	  dx[i] = xxsp[i] - (xobs[i] - xobs[i+3]);   /* xobs: the centre, every centre */
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
	dx[i] = xx[i] - xobs[i];
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
#ifndef SWE_UPSTREAM_COMPAT
    if (iflag & SEFLG_SPEED) {
      /* part of daily motion resulting from change of dt */
      for (i = 0; i <= 2; i++)
	xxsp[i] = pdp->x[i] - xx[i] - xxsp[i];
    }
    /* Upstream bug 81 (part 3), this fork's swe_calc_orbel(): the light
     * time was the straight line above, x(t) - dt v(t), and the orbit was
     * read at t - dt only with SEFLG_SPEED, so the position depended on
     * the flag (bug 26's defect, G27, here never fixed): a Vulcan-like
     * orbit (a = 0.137 AU) 1.6e-5 deg off the light-time fixed point
     * without the flag, 4.4e-9 with it, a Mercury-like one 3.0e-6 and
     * 7.2e-10. Solved on the orbit for every flag set as
     * app_pos_etc_plan_osc() does, finished by a Newton step; the orbit
     * moved by that step, the centre by it and the rounding of t. */
    for (j = 0; j < 10; j++) {
      t = pdp->teval - dt;
      retc = main_planet_bary(ctx, t, SEI_EARTH, epheflag, iflag, NO_SAVE, NULL, xearth, xearth, xsun, xmoon, serr);
      /* Upstream bug 96: the Earth at t as the one at teval moved back */
      if (iplctr == SE_EARTH)
        earth_place_at_lt(ctx, pdp->teval, t, &have_acc, acc_e, xearth);
      for (i = 0; i <= 5; i++)
        elret[i] = el[i];
      elret[0] = el[0] - dmot * dt;
      /* Upstream bug 136: an equinox of date at the emission instant, as
       * read_elements_file() gives JDATE (the protocol's 3.5a); it was
       * held at teval, the frame's turn over the light time missing: 3.5e-7
       * deg at a = 1.2 AU, 6.9e-6 deg at 30 AU, 1.9e-2 deg at 80000 AU
       * (462 days) off the same elements as a JDATE body */
      if (swi_orbel_to_state(ctx, t, elret, mu, of_date ? t : tequ, of_date, xx, serr) != OK)
	return ERR;
      if (bias)	/* bug 130 */
        swi_bias(ctx, xx, t, SEFLG_SPEED, TRUE);
      if (iplctr == SE_EARTH) {
        for (i = 0; i <= 5; i++)
          xx[i] += xearth[i];
      } else {
        for (i = 0; i <= 5; i++)
          xx[i] += xsun[i];
      }
      if (retc != OK)
	return retc;
      if (mosh_bary)	/* bug 109: the Sun's displacement (bug 125) */
        mosh_sun_move(ctx, xx, xsb, asun, pdp->teval, t);
      for (i = 0; i <= 2; i++)
        dx[i] = xx[i] - xobs[i];
      dtNew = sqrt(square_sum(dx)) * AUNIT / CLIGHT / 86400.0;
      if (fabs(dtNew - dt) < 1e-12 || j == 9)
        break;
      dt = dtNew;
    }
    {
      /* the orbit was taken at M - n dt exactly, the centre read at t, the
       * rounding of teval - dt: each moved by its own velocity at teval */
      double dtr = dt, eps, *vc = (iplctr == SE_EARTH) ? pedp->x + 3 : psdp->x + 3;
      double vcb[3];	/* bug 109: the centre's velocity in the frame */
      for (i = 0; i <= 2; i++)
        vcb[i] = vc[i] + vsun[i];
      dt = lt_newton_step(dx, xv0, dt, dtNew);
      eps = (pdp->teval - t) - dt;
      for (i = 0; i <= 2; i++)
        xx[i] += (dtr - dt) * (xv0[i] - vcb[i]) + eps * vcb[i];
    }
    dtsave_for_defl = dt;
    if (iflag & SEFLG_SPEED) {
#else
    if (iflag & SEFLG_SPEED) {
      /* part of daily motion resulting from change of dt */
      for (i = 0; i <= 2; i++)
	xxsp[i] = pdp->x[i] - xx[i] - xxsp[i];
      t = pdp->teval - dt;
      /* for accuracy in speed, we will need earth as well */
      retc = main_planet_bary(ctx, t, SEI_EARTH, epheflag, iflag, NO_SAVE, NULL, xearth, xearth, xsun, xmoon, serr);
      /* the same elements' two-body motion at the retarded epoch, where
       * swi_osc_el_plan() re-evaluates the element file's polynomial */
      for (i = 0; i <= 5; i++)
        elret[i] = el[i];
      elret[0] = el[0] - dmot * dt;
      /* bug 136, in both builds: an equinox of date at the emission
       * instant, as upstream's JDATE branch reads it there too */
      if (swi_orbel_to_state(ctx, t, elret, mu, of_date ? t : tequ, of_date, xx, serr) != OK)
	return ERR;
      if (iplctr == SE_EARTH) {
        for (i = 0; i <= 5; i++)
          xx[i] += xearth[i];
      } else {
        for (i = 0; i <= 5; i++)
          xx[i] += xsun[i];
      }
      if (retc != OK)
	return retc;
#endif
      if (iflag & SEFLG_TOPOCTR) {
#ifndef SWE_UPSTREAM_COMPAT
        double xoh[6];	/* bug 109: heliocentric, then the Sun's motion */
        for (i = 0; i <= 5; i++)
          xoh[i] = xobs[i] - (i >= 3 ? vsun[i-3] : 0);
        if (observer_at_lt(ctx, pedp->teval, t, iflag, xoh, pedp->x, xearth, xobs2, serr) != OK)
          return ERR;
#else
        if (swi_get_observer(ctx, t, iflag | SEFLG_NONUT, NO_SAVE, xobs2, serr) != OK)
          return ERR;
        for (i = 0; i <= 5; i++)
          xobs2[i] += xearth[i];
#endif
      } else {
#ifndef SWE_UPSTREAM_COMPAT
        earth_at_lt(ctx, pedp->teval, t, pedp->x, xearth, xobs2);
#else
        for (i = 0; i <= 5; i++)
          xobs2[i] = xearth[i];
#endif
      }
#ifndef SWE_UPSTREAM_COMPAT
      if (mosh_bary)
        mosh_sun_shift(xobs2, vsun, asun, t - pedp->teval);
#endif
    }
  }
  /*******************************
   * conversion to geocenter     *
   *******************************/
#ifndef SWE_UPSTREAM_COMPAT
  if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_TRUEPOS))
    lt_rate_exact(xx, xobs, xxsp);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    if (iflag & SEFLG_SPEED)
      add_aberr_rate(xx, xobs, xobs2);
#else
    if (iflag & SEFLG_SPEED)
      for (i = 3; i <= 5; i++)
	xx[i] += xobs[i] - xobs2[i];
#endif
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* bug 130: ICRS to J2000, as app_pos_etc_plan_osc() does */
  if (!(iflag & SEFLG_ICRS) && bias)
    swi_bias(ctx, xx, pdp->teval, iflag, FALSE);
#endif
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
  return app_pos_rest(ctx, pdp, pdp->teval, iflag, xx, xxsv, oe, serr);
}

/* influence of precession on speed 
 * xx		position and speed of planet in equatorial cartesian
 *		coordinates */
void swi_precess_speed(swe_ctx *ctx, double *xx, double t, int32 iflag, int direction) 
{
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 49: the precession's rate was taken as the general
   * precession in longitude about the ecliptic pole of date, which leaves
   * out the ecliptic's own turning (the planetary precession, chi_A' sin
   * eps = 4.2"/cy about the ecliptic's 90-degree point): every latitude
   * speed of date off by up to 3e-8 deg/day, a polar star's longitude
   * speed by 7e-8. Now dP/dt x0 itself, x0 the position in the frame
   * precessed from, as (P(t + 1) x0 - P(t - 1) x0) / 2, for any model
   * swi_precess() has, the series being smooth over centuries. Central,
   * because P^T P is not I to the last bits: a one-sided difference
   * against x itself picks that up, 5e-12 / h of the rate.
   * For the Vondrak series, the default, M and dM/dt come from the series
   * and their derivatives in one pass (swi_precess_matrix_rate()), the
   * matrix M being swi_precess()'s to the bit: no dearer than upstream's
   * rotation plus its two swi_ldp_peps() calls, where the differences cost
   * three more precessions (+7% on a planet). x(t) = M x0 into the date,
   * x0 = M^T x out of it. */
  int i;
  double x0[3], a[3], m[9], dm[9];
  int back = (direction == J2000_TO_J) ? J_TO_J2000 : J2000_TO_J;
  if (swi_precess_matrix_rate(ctx, t, iflag, m, dm)) {
    if (direction == J2000_TO_J) {
      for (i = 0; i <= 2; i++)	/* x0 = M^T x */
        x0[i] = xx[0] * m[i] + xx[1] * m[i + 3] + xx[2] * m[i + 6];
      for (i = 0; i <= 2; i++)
        a[i] = xx[3] * m[3 * i] + xx[4] * m[3 * i + 1] + xx[5] * m[3 * i + 2]
          + x0[0] * dm[3 * i] + x0[1] * dm[3 * i + 1] + x0[2] * dm[3 * i + 2];
    } else {
      for (i = 0; i <= 2; i++)	/* the date's x = M x */
        x0[i] = xx[0] * m[3 * i] + xx[1] * m[3 * i + 1] + xx[2] * m[3 * i + 2];
      for (i = 0; i <= 2; i++)
        a[i] = xx[3] * m[i] + xx[4] * m[i + 3] + xx[5] * m[i + 6]
          + x0[0] * dm[i] + x0[1] * dm[i + 3] + x0[2] * dm[i + 6];
    }
    for (i = 0; i <= 2; i++)
      xx[i+3] = a[i];
    return;
  }
  for (i = 0; i <= 2; i++)
    x0[i] = xx[i];
  swi_precess(ctx, x0, t, iflag, back);
  for (i = 0; i <= 2; i++)
    a[i] = x0[i];
  swi_precess(ctx, a, t + 1, iflag, direction);
  swi_precess(ctx, x0, t - 1, iflag, direction);
  swi_precess(ctx, xx+3, t, iflag, direction);
  for (i = 0; i <= 2; i++)
    xx[i+3] += (a[i] - x0[i]) / 2;
#else
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
    swi_ldp_peps(ctx, t, &dpre, NULL);
    swi_ldp_peps(ctx, t + 1, &dpre2, NULL);
    xx[3] += (dpre2 - dpre) * fac;
  } else {
    xx[3] += (50.290966 + 0.0222226 * tprec) / 3600 / 365.25 * DEGTORAD * fac;
			/* formula from Montenbruck, German 1994, p. 18 */
  }
}
  swi_polcart_sp(xx, xx);
  swi_coortrf2(xx, xx, -oe->seps, oe->ceps);
  swi_coortrf2(xx+3, xx+3, -oe->seps, oe->ceps);
#endif
}

/* multiplies cartesian equatorial coordinates with previously
 * calculated nutation matrix. also corrects speed. 
 */
#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 34: aberration moves the apparent place by about tau V, V the
 * observer's barycentric velocity, so its rate carries tau dV/dt. The
 * planets', the Moon's and the fictitious bodies' paths took that as
 * V(t) - V(t - tau), the velocity at the instant of observation less the
 * velocity at the instant the light left. For the Earth's annual and
 * monthly motion, which is linear over minutes, that is the same thing; for
 * a topocentric observer turning with the Earth it is not -- the site turns
 * through 2.5-5 deg in a planet's light time, a few per cent of a diurnal
 * aberration rate of 5.6e-4 deg/day (0.32" a turn) -- and the Sun's path
 * had no such term at all (3.2e-4 deg/day topocentric). Which is why
 * swe_calc() computed every topocentric speed with aberration from three
 * positions instead. The observer's own part is now tau a(t), a = the site's
 * acceleration by a central difference of its velocity over +-2^-11 day
 * ((omega h)^2 / 6 = 1.6e-6 of it, a binary step so t +- h is exact; a
 * step of 2^-8 day cost 1e-4, 2e-8 deg/day on topocentric latitude
 * speeds). It is the whole of a: add_aberr_rate() takes what turns
 * the apparent place from it (bug 52).
 * Upstream bug 79: that 1.6e-6 of a is 1.9e-9 AU/day^2 at the equator, and
 * a / c of the diurnal aberration's rate, 6e-10 deg/day on every
 * topocentric apparent speed (Neptune's mean points' longitude 8.3e-10
 * at 47 N, 1.2e-9 on the equator). A central difference of a velocity
 * turning at omega is a sin(omega h) / (omega h) exactly, so it is now
 * divided by that, over +-2^-6 day, where the velocities' rounding (the
 * site's turning read at a rounded UT) is 5e-9 of a, not 2.4e-7: within
 * 7e-9 of a against a fit to the site's own velocity. */
static int observer_accel(swe_ctx *ctx, double tjd, int32 iflag, double *acc, char *serr)
{
  const double h = 1.0 / 64;
  const double wh = EARTH_ROT_SPEED * h;
  double xp[6], xm[6];
  int i;
  if (swi_get_observer(ctx, tjd + h, iflag | SEFLG_NONUT, NO_SAVE, xp, serr) != OK
      || swi_get_observer(ctx, tjd - h, iflag | SEFLG_NONUT, NO_SAVE, xm, serr) != OK)
    return ERR;
  for (i = 0; i <= 2; i++)
    acc[i] = (xp[i+3] - xm[i+3]) / (2 * h) * (wh / sin(wh));
  return OK;
}

/* The Earth at the instant the light left, t, as the aberration rate term
 * reads it. That term wants tau a(teval), the observer's acceleration now
 * times the light time; V(teval) - V(t) is tau a at the midpoint, off by
 * tau^2 a'/2, 1% of the rate at a light time of a day (2e-6 deg/day on
 * Nibiru, 2e-7 on Pluto). a' is taken as the Sun's pull's rate on the
 * Earth at teval, -GM (v/r^3 - 3 r (r.v)/r^5), from the heliocentric state
 * alone: not from the two reads' positions, which can disagree with their
 * velocities by 1e-9 AU across a segment of the Swiss files (at -3001 and
 * 3000), 3e-6 deg/day once divided by tau.
 * Upstream bug 62: both leave out the Moon's pull on the Earth, 0.6% of a
 * and 7.5% of a' (the difference of the reads carries it below 10 days,
 * but not at tau a(teval)): 2.3e-7 deg/day on Nibiru's speed, 6e-7 on VEA's.
 * The Moon's pull, GM_m d/|d|^3 with d the geocentric Moon, comes from the
 * save area (the Moon sweplan() left there at teval, with its velocity; no
 * extra read; Swiss files only, JPL and Moshier have no Moon there and keep
 * the Sun's pull alone). It joins a' (the Moon's rate, 7.5%) and, with it,
 * tau a(teval) is taken whole from the Sun's and Moon's pulls past a light
 * time of 0.3 day rather than 10: the Moon's period is 27 days, and the
 * series tau^2 a'/2 for its part fails by tau omega / 3 = 8% per day of
 * light time (Nibiru, 3.5 days: 7.7e-8 deg/day left with the series to 10
 * days, 7.5e-9 with the whole form past 0.3). Below
 * 1e-3 day (the Moon) the term is under 1e-5 of the rate and is skipped.
 * Upstream bug 66: bug 62 had the Moon on the Swiss files only; on JPL
 * and Moshier a far fictitious body kept the Sun-only a' and the 10-day
 * switch (2e-7 to 6e-7 deg/day), and Harrington and Pluto lacked the
 * Moon's 7.5% of a' (1.4e-8 to 1.7e-8). The Moon is now the one the saved
 * Earth was made from, kept by that computation at no extra read:
 * ctx->earth_moon, the geocentric Moon of the JPL Earth (EMB less the
 * Moon's share, swi_pleph) and of Moshier's (embofs_mosh, whose rate
 * difference holds the Moon's velocity and place). It serves every light
 * time. Without it (an Earth not saved at teval) the Moon is read past
 * 0.3 day. Past 0.3 day tau a is whole and now also holds the pull of
 * Mercury, Venus, Mars, Jupiter and Saturn on the Earth, 5e-5 of a each for
 * Jupiter and Venus at their nearest (4e-9 deg/day on the far bodies,
 * Swiss files too), from Moshier's heliocentric places (1e-2 of the pull is
 * plenty); only bodies past 0.3 day pay for them. Below 0.3 day the series
 * reads the planets' share in the difference of the Earth's velocities
 * (its jerk, 8e-6 of tau a, is left out).
 * Part 3: on Moshier, past 0.3 day tau a is taken from the Earth the series
 * returns, a second difference of its position (EMB plus Moon offset) over
 * +-2^-8 day at exact instants, not from the gravitational sum: the series'
 * small monthly terms move its acceleration by 1e-4 (2e-9 to 6e-9 deg/day
 * on the far bodies). Swiss and JPL reach the same 1e-9 with the sum (JPL
 * measured with the Earth's velocity differenced over the same step: 1.0e-9
 * either way), which needs no further read.
 * Without the Moon in hand, past 10 days the series itself fails (3.7e-4
 * deg/day at the 462 days of a body 80,000 AU out), and tau a(teval) is
 * taken whole from the Sun's pull, -GM r/r^3: that leaves out the Moon's
 * 0.6% of a, which the difference of the reads carries below 10 days.
 * xsun is the barycentric Sun at teval (unused on Moshier, whose Earth is
 * heliocentric); NULL leaves the plain difference. Sets xout to xe with
 * the velocity the term should read. */
/* The inner planets', Jupiter's and Saturn's pull on the Earth at teval,
 * sum GM_p (P - E) / |P - E|^3 in AU/day^2, from Moshier's heliocentric
 * places (arcseconds are plenty: the pull is 1e-4 of the Sun's and wants
 * 1e-2 of itself), E the Earth's heliocentric place (equatorial J2000). */
static void planets_pull(swe_ctx *ctx, double teval, const double *e, double *ap)
{
  static const struct { int iplm; double mrat; } pl[] = {
    {0, 6023600.0}, {1, 408523.72}, {3, 3098703.6}, {4, 1047.3486}, {5, 3497.898},
  };
  const double gm = HELGRAVCONST * 86400.0 * 86400.0 / (AUNIT * AUNIT * AUNIT);
  int k, i;
  for (i = 0; i <= 2; i++)
    ap[i] = 0;
  for (k = 0; k < 5; k++) {
    double p[3], d[3], dd;
    if (swi_moshplan2(ctx, teval, pl[k].iplm, p) < 0)
      continue;
    swi_polcart(p, p);
    swi_coortrf2(p, p, -ctx->oec2000.seps, ctx->oec2000.ceps);
    for (i = 0; i <= 2; i++)
      d[i] = p[i] - e[i];
    dd = sqrt(square_sum(d));
    for (i = 0; i <= 2; i++)
      ap[i] += gm / pl[k].mrat * d[i] / (dd * dd * dd);
  }
}

static void earth_at_lt_sun(swe_ctx *ctx, double teval, double t, const double *xeteval,
                            const double *xsun, const double *xe, double *xout)
{
  const double gm = HELGRAVCONST * 86400.0 * 86400.0 / (AUNIT * AUNIT * AUNIT);
  const struct plan_data *pmdp = &ctx->pldat[SEI_MOON];
  double tau = teval - t, r[6], rr, rv = 0, jerk;
  double am[3] = {0, 0, 0}, jm[3] = {0, 0, 0};
  int i, have_moon = FALSE;
  for (i = 0; i <= 5; i++)
    xout[i] = xe[i];
  if (tau < 1e-3)
    return;
  /* the Moon's pull on the Earth, a_m = GM_m d/|d|^3, and its rate: from
   * the save area on the Swiss files, from the Earth's own read on JPL (the
   * Earth is the EMB less the Moon's share) and Moshier (embofs), else, past
   * the switch, read here */
  {
    const double gmm = GEOGCONST * 86400.0 * 86400.0 / (AUNIT * AUNIT * AUNIT) / EARTH_MOON_MRAT;
    double dm[6];
    const double *d = NULL;
    if (pmdp->teval == teval && pmdp->iephe == SEFLG_SWIEPH
        && ctx->pldat[SEI_EARTH].iephe == SEFLG_SWIEPH) {
      d = pmdp->x;
    } else if (ctx->earth_moon.t == teval && ctx->pldat[SEI_EARTH].teval == teval
               && ctx->earth_moon.iephe == ctx->pldat[SEI_EARTH].iephe) {
      d = ctx->earth_moon.x;
    } else if (tau > 0.3) {
      int ie = ctx->pldat[SEI_EARTH].iephe;
      if ((ie == SEFLG_JPLEPH && swi_pleph(ctx, teval, J_MOON, J_EARTH, dm, NULL) == OK)
          || (ie == SEFLG_MOSEPH && swi_moshmoon(ctx, teval, NO_SAVE, dm, NULL) == OK))
        d = dm;
    }
    if (d != NULL) {
      double dd = sqrt(square_sum(d)), dv = 0;
      for (i = 0; i <= 2; i++)
        dv += d[i] * d[i+3];
      for (i = 0; i <= 2; i++) {
        am[i] = gmm * d[i] / (dd * dd * dd);
        jm[i] = gmm * (d[i+3] - 3 * d[i] * dv / (dd * dd)) / (dd * dd * dd);
      }
      have_moon = TRUE;
    }
  }
  /* Moshier: the Earth is heliocentric already */
  for (i = 0; i <= 5; i++)
    r[i] = xeteval[i];
  if (ctx->pldat[SEI_EARTH].iephe != SEFLG_MOSEPH) {
    if (xsun == NULL)
      return;
    for (i = 0; i <= 5; i++)
      r[i] -= xsun[i];
  }
  rr = sqrt(square_sum(r));
  if (tau > (have_moon ? 0.3 : 10)) {
    double ap[3];
    if (ctx->pldat[SEI_EARTH].iephe == SEFLG_MOSEPH && ctx->pldat[SEI_EARTH].teval == teval) {
      /* Upstream bug 66, part 3: the acceleration of the Earth the series
       * returns, not the gravitational sum (the series' monthly terms move
       * it by 1e-4: 4-6e-9 deg/day on the far bodies) */
      swi_mosh_earth_accel(ctx, teval, ctx->pldat[SEI_EARTH].x, ap);
      for (i = 0; i <= 2; i++)
        xout[i+3] = xeteval[i+3] - tau * ap[i];
      return;
    }
    /* Upstream bug 66: and the planets' pull on the Earth */
    planets_pull(ctx, teval, r, ap);
    for (i = 0; i <= 2; i++)
      am[i] += ap[i];
    for (i = 0; i <= 2; i++)
      xout[i+3] = xeteval[i+3] + tau * (gm * r[i] / (rr * rr * rr) - am[i]);
    return;
  }
  for (i = 0; i <= 2; i++)
    rv += r[i] * r[i+3];
  for (i = 0; i <= 2; i++) {
    jerk = -gm * (r[i+3] - 3 * r[i] * rv / (rr * rr)) / (rr * rr * rr) + jm[i];
    xout[i+3] -= tau * tau / 2 * jerk;
  }
}

/* the same, with the Sun from the save area when it is the one at teval */
static void earth_at_lt(swe_ctx *ctx, double teval, double t, const double *xeteval, const double *xe, double *xout)
{
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  earth_at_lt_sun(ctx, teval, t, xeteval, psdp->teval == teval ? psdp->x : NULL, xe, xout);
}

/* Upstream bug 96: an Earth-centred set (seorbel.txt's "geo", or
 * swe_calc_orbel() about the Earth) is seen from the Earth at teval, its
 * focus the Earth at the emission instant t: the body's geocentric place
 * holds E(t) - E(teval), a few 1e-7 AU, as the difference of two reads of
 * the Earth, 1 AU vectors. Each Moshier read is off a smooth curve by its
 * series' rounding, and the two reads, 4e-5 day apart, do not share it:
 * the place jittered by about 3e-15 AU rms from one instant to the next,
 * 2.7e-11 deg on Waldemath (0.0068 AU), 3e-12 on Selena (0.053 AU); its
 * fourth difference over 2^-10 day reached 9e-14 AU, against 1e-15 with
 * no light time and 7e-15 on the Swiss files (an ulp of 1 AU a read), and
 * a nine-point difference over 2^-10 day read 3.4e-8 deg/day of speed into
 * it. Below a light time of 2^-10 day the Earth at t is now the Earth at
 * teval carried back along its own motion, E - tau V + tau^2 A / 2, A the
 * series' acceleration (swi_mosh_earth_accel(), once a call), so E(teval)
 * cancels exactly against the observer and only the smooth -tau V +
 * tau^2 A / 2 is left. Left out is tau^3 / 6 of the jerk, 6e-6 AU/day^3:
 * 2e-16 AU at the 2^-10 day switch, 3e-17 on Selena, 4e-20 on Waldemath.
 * Only the place is replaced; the velocity is the read's. Moshier only. */
#define LT_EARTH_TAYLOR (1.0 / 1024)
static void earth_place_at_lt(swe_ctx *ctx, double teval, double t, int *have_acc,
                              double *acc, double *xe)
{
  const struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  double tau = teval - t;
  int i;
  if (pedp->iephe != SEFLG_MOSEPH || pedp->teval != teval || !(tau < LT_EARTH_TAYLOR))
    return;
  if (!*have_acc) {
    swi_mosh_earth_accel(ctx, teval, pedp->x, acc);
    *have_acc = TRUE;
  }
  for (i = 0; i <= 2; i++)
    xe[i] = pedp->x[i] - tau * pedp->x[i+3] + tau * tau / 2 * acc[i];
}

/* The observer at the instant the light left, t, as the aberration rate
 * term xobs - xobs2 uses it: the Earth's state at t, xe, and the site's
 * velocity at teval less tau a(teval). xobs is the barycentric observer at
 * teval, xeteval the Earth there, xsun the barycentric Sun there (or
 * NULL). Only xobs2's velocity is read. */
static int observer_at_lt_sun(swe_ctx *ctx, double teval, double t, int32 iflag,
                              const double *xobs, const double *xeteval, const double *xsun,
                              const double *xe, double *xobs2, char *serr)
{
  double acc[3], xel[6];
  int i;
  if (observer_accel(ctx, teval, iflag, acc, serr) != OK)
    return ERR;
  earth_at_lt_sun(ctx, teval, t, xeteval, xsun, xe, xel);
  for (i = 0; i <= 2; i++) {
    xobs2[i] = xel[i] + xobs[i] - xeteval[i];
    xobs2[i+3] = xel[i+3] + xobs[i+3] - xeteval[i+3] - (teval - t) * acc[i];
  }
  return OK;
}

/* the same, the Sun from the save area */
static int observer_at_lt(swe_ctx *ctx, double teval, double t, int32 iflag,
                          const double *xobs, const double *xeteval,
                          const double *xe, double *xobs2, char *serr)
{
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  return observer_at_lt_sun(ctx, teval, t, iflag, xobs, xeteval,
                            psdp->teval == teval ? psdp->x : NULL, xe, xobs2, serr);
}

/* The aberration rate term, xobs - xobs2 (the observer's velocity at the
 * instant of observation less that at the instant the light left, ~tau
 * dV/dt), added to the apparent velocity xx[3..5].
 * Upstream bug 52: bug 34's fix added this d = tau a less its part along
 * the line of sight, the first-order derivative of the aberrated place
 * A(u, v) in v = V/c, with a site's acceleration stripped of its radial
 * part beforehand. The aberrated place moves with v at second order too,
 * by terms in v times dv of relative size v/c = 1e-4, and a radial dv
 * turns it by v_perp |dv| / 2: a site's centripetal 1.7e-3 AU/day^2
 * against the Earth's orbital v, 4.5e-5 of the diurnal aberration rate,
 * 1.1-3.5e-8 deg/day on every topocentric longitude speed (the Sun's
 * 2.5e-8 on the equator, 1.8e-8 at 47N, signed by the hour), against
 * 2e-9 without aberration; geocentric, the Earth's own acceleration gave
 * +4-5e-9 deg/day on every body, the Sun's at every epoch. The term is
 * now the exact derivative of A along w = d / |u| (= a / c, the light
 * time being |u| / c), at the geometric place u, which is the apparent
 * one aberrated back by -v (A(A(u, v), -v) = u, |A| = |u|, to rounding),
 * with the whole of a; its radial part is the distance's own, second
 * order (bug 34's 4e-6 AU/day on Mars was the first-order d's). */
static void add_aberr_rate(double *xx, const double *xobs, const double *xobs2)
{
  double v[3], mv[3], u[3], w[3], dn[3];
  double ru = sqrt(square_sum(xx)), v2, b_1, f1, f2, vw = 0, uw = 0, db, df1, df2;
  int i;
  for (i = 0; i <= 2; i++) {
    v[i] = xobs[i+3] / 24.0 / 3600.0 / CLIGHT * AUNIT;
    mv[i] = -v[i];
  }
  v2 = square_sum(v);
  b_1 = sqrt(1 - v2);
  /* the geometric place: the apparent one, aberrated by -v */
  f1 = dot_prod(xx, mv) / ru;
  f2 = 1.0 + f1 / (1.0 + b_1);
  for (i = 0; i <= 2; i++)
    u[i] = (b_1 * xx[i] + f2 * ru * mv[i]) / (1.0 + f1);
  /* A(u, v) = (b u + f2 |u| v) / (1 + f1), f1 = u.v / |u|,
   * f2 = 1 + f1 / (1 + b), b = sqrt(1 - v^2); its derivative along w,
   * with A = xx */
  f1 = dot_prod(u, v) / ru;
  f2 = 1.0 + f1 / (1.0 + b_1);
  for (i = 0; i <= 2; i++) {
    w[i] = (xobs[i+3] - xobs2[i+3]) / ru;
    vw += v[i] * w[i];
    uw += u[i] * w[i];
  }
  db = -vw / b_1;
  df1 = uw / ru;
  df2 = df1 / (1.0 + b_1) - f1 * db / ((1.0 + b_1) * (1.0 + b_1));
  for (i = 0; i <= 2; i++) {
    dn[i] = db * u[i] + df2 * ru * v[i] + f2 * ru * w[i];
    xx[i+3] += (dn[i] - xx[i] * df1) / (1.0 + f1);
  }
}

/* For swe_nod_aps() (Upstream bug 70): the observer's barycentric
 * acceleration at teval, whole, as bug 34's term tau a reads it -- the
 * Earth's from earth_at_lt_sun() (V(teval) less the velocity it gives, over
 * tau = teval - t), plus the site's for SEFLG_TOPOCTR. xeteval and xsun
 * are the barycentric Earth and Sun at teval, xe the Earth at t. The
 * points there take the aberration's rate from it exactly (second order in
 * v/c, the radial part included), not from tau a's transverse part. */
int swi_observer_accel_lt(swe_ctx *ctx, double teval, double t, int32 iflag,
                          const double *xeteval, const double *xsun, const double *xe,
                          double *acc, char *serr)
{
  double xel[6], as[3], tau = teval - t;
  int i;
  /* Upstream bug 93: on Moshier tau a was V(teval) less the Earth's
   * velocity at t carried back by the gravitational jerk, over tau
   * (earth_at_lt_sun()), which is a less tau/2 times the difference between
   * the series' jerk and the gravitational one. The series' small monthly
   * terms are 1e-4 of a (bug 66, part 3), turning at 0.23 rad/day: 7e-9
   * AU/day^3, so 3e-10 to 1e-9 AU/day^2 off the Earth's acceleration at
   * tau = 0.03..0.17 day (the planets' light times), 6e-12 rad/day through
   * the aberration: 1.7e-10 deg/day on Neptune's points' geocentric
   * latitude speed (1e-12 under SEFLG_NOABERR; 2e-11 on JPL and Swiss,
   * whose Earth is the gravitational one). Now a is the Earth's
   * acceleration at teval, the central difference of its velocities at
   * teval +- 2^-9 day, as bug 76 takes it for a star; the positions'
   * second difference (bug 66, part 3) is rougher by the series' rounding
   * (it left 1.6e-10 deg/day on the points' longitude speed). */
  if (ctx->pldat[SEI_EARTH].iephe == SEFLG_MOSEPH && ctx->pldat[SEI_EARTH].teval == teval) {
    double xa[2][6];
    int k;
    for (k = 0; k < 2; k++) {
      int32 iephe_used = SEFLG_MOSEPH;
      if (main_planet_bary(ctx, k ? teval - 1.0 / 512 : teval + 1.0 / 512, SEI_EARTH,
                           SEFLG_MOSEPH, iflag | SEFLG_SPEED, NO_SAVE, &iephe_used,
                           xa[k], xa[k], NULL, NULL, serr) != OK)
        return ERR;
    }
    for (i = 0; i <= 2; i++)
      acc[i] = (xa[0][i+3] - xa[1][i+3]) * 256;
  } else {
    earth_at_lt_sun(ctx, teval, t, xeteval, xsun, xe, xel);
    for (i = 0; i <= 2; i++)
      acc[i] = tau > 0 ? (xeteval[i+3] - xel[i+3]) / tau : 0;
  }
  if (iflag & SEFLG_TOPOCTR) {
    if (observer_accel(ctx, teval, iflag, as, serr) != OK)
      return ERR;
    for (i = 0; i <= 2; i++)
      acc[i] += as[i];
  }
  return OK;
}

/* Upstream bug 35: the light time's rate was taken as the change, over a
 * day, of the light-time displacement of a body moved linearly along its
 * velocity (xxsp), which is off by the curvature of that day's motion:
 * 3e-6 deg/day on Mercury's speed, 6.9e-5 AU/day on Pluto's distance rate,
 * 5.5e-4 on a fictitious body's. It is v_P dtau/dt, dtau/dt =
 * rho^.(v_P - v_O) / (c + rho^.v_P), exactly, from the solved light time's
 * vectors (as the Moon's, bug 32): xx the body at t - tau and xo the
 * observer at t, barycentric, with velocities. Sets the xxsp that the
 * callers subtract from the speed. */
static void lt_rate_exact(const double *xx, const double *xo, double *xxsp)
{
  double rho[3], r, rv = 0, rvp = 0, taudot;
  const double c = CLIGHT * 86400.0 / AUNIT;   /* AU per day */
  int i;
  for (i = 0; i <= 2; i++)
    rho[i] = xx[i] - xo[i];
  r = sqrt(square_sum(rho));
  for (i = 0; i <= 2; i++) {
    rv += rho[i] * (xx[i+3] - xo[i+3]);
    rvp += rho[i] * xx[i+3];
  }
  taudot = (rv / r) / (c + rvp / r);
  for (i = 0; i <= 2; i++)
    xxsp[i] = taudot * xx[i+3];
}
#endif

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 33: the ecliptic of date turns about the equinox as the
 * obliquity changes, and a velocity rotated into it with the same matrix
 * as the position is not the rate of the rotated position. The obliquity
 * rate, rad/day: the mean obliquity's by a difference over one day (its
 * rate changes over centuries; the value at tjd is usually cached in
 * ctx->oec already), and with_nut adds the nutation in
 * obliquity's, swi_nutation_rate() (up to 1.3e-5 deg/day, the
 * fortnightly and semiannual terms; the mean rate is 3.6e-7). */
double swi_obliquity_rate(swe_ctx *ctx, double tjd, int32 iflag, AS_BOOL with_nut)
{
  double e0 = (ctx->oec.teps == tjd && ctx->oec.epsflag == SWI_EPS_KEY(iflag))
              ? ctx->oec.eps : swi_epsiln(ctx, tjd, iflag);
  double rate = swi_epsiln(ctx, tjd + 1, iflag) - e0;
  if (with_nut) {
    if (ctx->nut.tnut == tjd && ctx->nut.dnut_ok) {
      rate += ctx->nut.dnut[1];
    } else {
      double dn[2];
      swi_nutation_rate(ctx, tjd, iflag, dn);
      rate += dn[1];
    }
  }
  return rate;
}

/* xx: ecliptic cartesian position and velocity in a frame rotated from
 * the equator by an obliquity that changes at edot. The rotation about x,
 * y' = y cos e + z sin e, z' = -y sin e + z cos e, has d/de (y', z') =
 * (z', -y'): entering the frame, call it with +edot after the rotation;
 * leaving it, with -edot before. */
void swi_ecl_frame_speed(double *xx, double edot)
{
  xx[4] += edot * xx[2];
  xx[5] -= edot * xx[1];
}
#endif

void swi_nutate(swe_ctx *ctx, double *xx, int32 iflag, AS_BOOL backward)
{
  int i;
  double x[6];
#ifdef SWE_UPSTREAM_COMPAT
  double xv[6];
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 49: this was (N(t) x - N(t - h) x) / h, h = 1e-4 day,
     * two vectors of the body's distance differenced: their last bits, 2e-9
     * AU for a star at 1.6e7 AU, made 2e-5 AU/day of its distance rate.
     * Now the rotation's rate from the angles' own: N = R1(-eps - deps)
     * R3(-dpsi) R1(eps), the mean obliquity held, turns at w = deps' e1 +
     * dpsi' (0, -sin(eps + deps), cos(eps + deps)) in the true frame, and
     * the velocity gains w x x_true; backward, -(N^T w) x x_mean.
     * Upstream bug 58: the angles' rates were a backward difference over h,
     * the rate at t - h/2, 6.5e-10 deg/day off in every longitude speed
     * of date (2.7e-8 in Polaris's right ascension). Now swi_nutation_rate(),
     * the polynomial's own derivative or a central difference.
     * Upstream bug 69: the mean obliquity was held, but N turns with it too:
     * R1(-eps - deps) and R1(eps) do not cancel when dpsi != 0, and w
     * left out eps' (e1 - R1(-eps - deps) R3(-dpsi) e1) = eps' (1 - cos
     * dpsi, -sin dpsi cos(eps + deps), -sin dpsi sin(eps + deps)), 3e-11
     * deg/day, 2e-9 in the right ascension of a star near the pole
     * (Polaris 1.9e-9, Polaris Australis 2.0e-9). Now in w, eps' the mean
     * obliquity's rate kept beside the angles' (nut.deps0). */
    {
      double w[3], wm[3], et = ctx->oec.eps + ctx->nut.nutlo[1];
      double dpsi = ctx->nut.dnut[0], psi = ctx->nut.nutlo[0];
      double de0 = ctx->nut.deps0;
      double sp = sin(psi), se = sin(et), ce = cos(et);
      w[0] = ctx->nut.dnut[1] + de0 * (1 - cos(psi));
      w[1] = -dpsi * se - de0 * sp * ce;
      w[2] = dpsi * ce - de0 * sp * se;
      if (backward) {
        for (i = 0; i <= 2; i++)
          wm[i] = -(w[0] * ctx->nut.matrix[i][0] + w[1] * ctx->nut.matrix[i][1]
                    + w[2] * ctx->nut.matrix[i][2]);
      } else {
        for (i = 0; i <= 2; i++)
          wm[i] = w[i];
      }
      xx[3] = x[3] + wm[1] * x[2] - wm[2] * x[1];
      xx[4] = x[4] + wm[2] * x[0] - wm[0] * x[2];
      xx[5] = x[5] + wm[0] * x[1] - wm[1] * x[0];
    }
#else
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
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 49: the shift aberr_light() makes, A(x) - x, formed
 * directly: ((b_1 - 1 - f1) x + f2 ru v) / (1 + f1), b_1 - 1 as
 * -v^2 / (1 + b_1). */
static void aberr_shift(const double *xx, const double *xe, double *dx) {
  int i;
  double v[3], u[3], ru, b_1, f1, f2, v2;
  for (i = 0; i <= 2; i++) {
    u[i] = xx[i];
    v[i] = xe[i+3] / 24.0 / 3600.0 / CLIGHT * AUNIT;
  }
  ru = sqrt(square_sum(u));
  v2 = square_sum(v);
  b_1 = sqrt(1 - v2);
  f1 = dot_prod(u, v) / ru;
  f2 = 1.0 + f1 / (1.0 + b_1);
  for (i = 0; i <= 2; i++)
    dx[i] = ((-v2 / (1.0 + b_1) - f1) * xx[i] + f2 * ru * v[i]) / (1.0 + f1);
}
#endif

void swi_aberr_light_ex(double *xx, double *xe, double *xe_dt, double dt, int32 iflag) {
  int i;
  double xxs[6];
  double xx2[6];
#ifndef SWE_UPSTREAM_COMPAT
  double d0[3], d1[3];
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 49: the speed was (A(x(t)) - A(x(t - dt))) / dt, two
     * vectors of the star's distance differenced over 1e-5 day, so their
     * last bits (2e-9 AU for Spica) made 2e-4 AU/day of the distance rate.
     * Now the velocity plus the change of the shift alone. */
    aberr_shift(xxs, xe, d0);
    aberr_shift(xx2, xe_dt, d1);
    for (i = 0; i <= 2; i++)
      xx[i+3] = xxs[i+3] + (d0[i] - d1[i]) / dt;
#else
    aberr_light(xx2, xe_dt);
    for (i = 0; i <= 2; i++) {
      xx[i+3] = (xx[i] - xx2[i]) / dt;
    }
#endif
  }
}

#ifndef SWE_UPSTREAM_COMPAT
/* A star's aberration with its rate as a central difference of the shift,
 * observer velocities xe_m at t - dm and xe_p at t + dp (dm, dp the exact
 * differences of the instants). Upstream bug 49: the backward one over 1e-5
 * day is the rate at t - dt/2, which for the site's diurnal aberration
 * (0.32", turning once a day) is off by dt/2 w^2 A = 1.8e-8 deg/day; and dt
 * was taken as the nominal 1e-5 where tjd - dt is rounded to 4.7e-10 day,
 * 2.3e-5 of the step: 2.2e-9 deg/day of the annual rate, 1.3e-8 of the
 * diurnal. */
static void aberr_light_central(double *xx, const double *xe, const double *xe_m,
                                const double *xe_p, double dm, double dp, int32 iflag)
{
  int i;
  double xxs[6], xm[3], xp[3], d0[3], d1[3];
  for (i = 0; i <= 5; i++)
    xxs[i] = xx[i];
  aberr_light(xx, (double *) xe);
  if (iflag & SEFLG_SPEED) {
    for (i = 0; i <= 2; i++) {
      xm[i] = xxs[i] - dm * xxs[i+3];
      xp[i] = xxs[i] + dp * xxs[i+3];
    }
    aberr_shift(xm, xe_m, d0);
    aberr_shift(xp, xe_p, d1);
    for (i = 0; i <= 2; i++)
      xx[i+3] = xxs[i+3] + (d1[i] - d0[i]) / (dm + dp);
  }
}

/* The observer's velocity at t - dm and t + dp for aberr_light_central():
 * the Earth's at t, xe, carried there with its acceleration at t, and the
 * site's, which turns, read at both instants. Upstream bug 76: the Earth's
 * velocity at t + dp was carried on linearly from its velocities at t - dm
 * and t, the acceleration at t - dm/2, short by dm/2 of its rate of change:
 * 8.6e-8 of the aberration's rate, 8e-12 deg/day on the sky, 1.03e-9 in
 * Polaris's right ascension speed at any step. Now the acceleration at t
 * from reads at t +- h, exact instants: the velocities' central
 * difference, h = 2^-9 (truncation h^2/6 a'', <= 5e-10 of a, the Moon's
 * month), on every ephemeris since the Moshier Earth's velocity is smooth
 * (UPSTREAM-BUGS 85; before, its roughness took 2 B - A over 2^-3 day, B
 * the positions' second difference, A the velocities' central one). */
static int fixstar_obs_rate(swe_ctx *ctx, double tjd, double dm, double dp, int32 epheflag,
                            int32 iflag, const double *xe, double *xo_m, double *xo_p,
                            char *serr)
{
  int i, k;
  int32 iephe_used;
  const double h = 1.0 / 512;
  double xa[2][6], xs[6], ae[3];
  for (k = 0; k < 2; k++) {
    iephe_used = epheflag;
    /* no Sun: these reads are for the Earth's velocity only */
    if (main_planet_bary(ctx, k ? tjd - h : tjd + h, SEI_EARTH, epheflag, iflag, NO_SAVE,
                         &iephe_used, xa[k], xa[k], NULL, NULL, serr) != OK)
      return ERR;
    if (swi_refuse_downgrade(ctx, epheflag, iephe_used, tjd, serr))
      return ERR;
    if (!(iflag & SEFLG_HELCTR) && swi_refuse_moshmoon(ctx, tjd, serr))	/* bug 120 */
      return ERR;
  }
  for (i = 0; i <= 2; i++)
    ae[i] = (xa[0][i+3] - xa[1][i+3]) / (2 * h);
  for (i = 0; i <= 2; i++) {
    xo_m[i] = xe[i] - dm * xe[i+3];
    xo_p[i] = xe[i] + dp * xe[i+3];
    xo_m[i+3] = xe[i+3] - dm * ae[i];
    xo_p[i+3] = xe[i+3] + dp * ae[i];
  }
  if (iflag & SEFLG_TOPOCTR) {
    for (k = 0; k < 2; k++) {
      double *xo = k ? xo_p : xo_m;
      if (swi_get_observer(ctx, k ? tjd + dp : tjd - dm, iflag | SEFLG_NONUT, NO_SAVE, xs, serr) != OK)
        return ERR;
      for (i = 0; i <= 5; i++)
        xo[i] += xs[i];
    }
  }
  return OK;
}
#endif

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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 73: the shift's change along the body's motion was the
     * backward difference over intv, the rate at t - intv/2: the Moon's
     * place turns at 0.23 rad/day, so its 20" shift curves, and that was
     * 1.9e-8 deg/day of its speed (2.8e-8 topocentric). Now the derivative of A(u) = (b u + f2
     * |u| v) / (1 + f1) itself along w = u', b and v held: d|u| = u.w / |u|,
     * df1 = v.w / |u| - f1 d|u| / |u|, df2 = df1 / (1 + b). */
    double w[3], uw = 0, vw = 0, dru, df1, df2;
    (void) dx1; (void) dx2; (void) xx2; (void) intv;
    for (i = 0; i <= 2; i++) {
      w[i] = xxs[i+3];
      uw += xxs[i] * w[i];
      vw += v[i] * w[i];
    }
    dru = uw / ru;
    df1 = (vw - f1 * dru) / ru;
    df2 = df1 / (1.0 + b_1);
    for (i = 0; i <= 2; i++)
      xx[i+3] = (b_1 * w[i] + (df2 * ru + f2 * dru) * v[i] - xx[i] * df1) / (1.0 + f1);
#else
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
#endif
  }
}

/* computes relativistic light deflection by the sun
 * ipli 	sweph internal planet number 
 * xx		planet's position accounted for light-time
 * dt		dt of light-time
 */
/* The deflection seen from xobs, barycentric with its velocity, the Sun
 * at the instant of observation xsb (barycentric, or unused under
 * Moshier, whose xobs is heliocentric already): deflect_light_site()'s
 * for the Earth or a site on it, swe_calc_pctr()'s for its centre body
 * (Upstream bug 77). */
static void deflect_light_from(double *xx, double dt, int32 iflag, const double *xobs,
                               const double *xsb, int32 iephe)
{
  int i;
  double xx2[6];
  double u[6], e[6], q[6], ru, re, rq, uq, ue, g1, g2;
#ifdef SWE_UPSTREAM_COMPAT
  double qe;
  double xx3[6], dx1, dx2;
  double dtsp;
#endif
  double xsun[6], xearth[6];
  double sina, sin_sunr, meff_fact;
  for (i = 0; i <= 5; i++)
    xearth[i] = xobs[i];
  /* U = planetbary(t-tau) - earthbary(t) = planetgeo */
  for (i = 0; i <= 2; i++) 
    u[i] = xx[i]; 
  /* Eh = earthbary(t) - sunbary(t) = earthhel */
  if (iephe == SEFLG_JPLEPH || iephe == SEFLG_SWIEPH) {
    for (i = 0; i <= 2; i++) 
      e[i] = xearth[i] - xsb[i]; 
  } else {
    for (i = 0; i <= 2; i++) 
      e[i] = xearth[i];
  }
  /* Q = planetbary(t-tau) - sunbary(t-tau) = 'planethel' */
  /* first compute sunbary(t-tau) for */
  if (iephe == SEFLG_JPLEPH || iephe == SEFLG_SWIEPH) {
    for (i = 0; i <= 2; i++)
      /* this is sufficient precision */
      xsun[i] = xsb[i] - dt * xsb[i+3];
    for (i = 3; i <= 5; i++)
      xsun[i] = xsb[i];
  } else {
    for (i = 0; i <= 5; i++)
      xsun[i] = xsb[i];
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
#ifdef SWE_UPSTREAM_COMPAT
  qe = dot_prod(q,e);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 39: sqrt(1 - ue^2) cancels where it matters, a planet
   * behind the disc, ue ~ 1: at 0.02 of the Sun's radius sina ~ 1e-4 and
   * carries 1e-12 of rounding, against the 1e-8 it changes over the speed
   * step below; the deflection's rate came out rough by 1.5e-5 deg/day
   * between instants 1e-9 day apart. |u x e| is exact to its last bits. */
  {
    double ux[3];
    swi_cross_prod(u, e, ux);
    sina = sqrt(square_sum(ux));
  }
#else
  sina = sqrt(1 - ue * ue);	/* sin(angle) between sun and planet */
#endif
  sin_sunr = SUN_RADIUS / re; 	/* sine of sun radius (= sun radius) */
  if (sina < sin_sunr) 	{
    meff_fact = meff(sina / sin_sunr);
  } else {
    meff_fact = 1;
  }
  g1 = 2.0 * HELGRAVCONST * meff_fact / CLIGHT / CLIGHT / AUNIT / re; 
#ifndef SWE_UPSTREAM_COMPAT
  /* and 1 + q.e cancels there too, q.e ~ -1, g2 ~ 5e-9: for unit vectors
   * it is |q + e|^2 / 2, formed from the small vector itself */
  {
    double qpe[3];
    for (i = 0; i <= 2; i++)
      qpe[i] = q[i] + e[i];
    g2 = square_sum(qpe) / 2;
  }
#else
  g2 = 1.0 + qe;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 74: the deflection's rate was its change over the 5e-7
     * day before t, one-sided, so off by half that step times its second
     * derivative. Behind the disc the deflection, 1.75" m(r)/r at r solar
     * radii from the centre, turns about the centre at v/b and curves
     * fast: Isis-Transpluto's speed 1.8e-6 deg/day off at 0.005 deg from
     * the centre, 4.3e-7 at 0.026, 2.6e-8 at 0.1. Now the derivative of
     * the displacement ru g1/g2 (uq e - ue q), through u, e and q's rates
     * (the step's own) and meff()'s slope on r's segment of its table:
     * the rate of the deflected position, on r's side of a node.
     * Upstream bug 49: the step took the deflection as xx2 - xx, a
     * difference of two vectors of the star's distance, 1.6e7 AU for
     * Spica, whose last bits (2e-9 AU) it made 4e-3 AU/day of noise; the
     * displacement and its rate are formed directly. */
    {
      double U1[3], E1[3], Q1[3], du[3], de[3], dq[3];
      double ru1, re1, rq1, duq, due, dmeff = 0, dg1, dg2 = 0, f, df;
      double k1 = 2.0 * HELGRAVCONST / CLIGHT / CLIGHT / AUNIT;
      for (i = 0; i <= 2; i++) {
        U1[i] = xx[i+3];
        if (iephe == SEFLG_JPLEPH || iephe == SEFLG_SWIEPH)
          E1[i] = xearth[i+3] - xsb[i+3];
        else
          E1[i] = xearth[i+3];
        Q1[i] = xx[i+3] + xearth[i+3] - xsun[i+3];
      }
      ru1 = dot_prod(u, U1);
      re1 = dot_prod(e, E1);
      rq1 = dot_prod(q, Q1);
      for (i = 0; i <= 2; i++) {
        du[i] = (U1[i] - u[i] * ru1) / ru;
        de[i] = (E1[i] - e[i] * re1) / re;
        dq[i] = (Q1[i] - q[i] * rq1) / rq;
      }
      duq = dot_prod(du, q) + dot_prod(u, dq);
      due = dot_prod(du, e) + dot_prod(u, de);
      if (sina < sin_sunr && sina > 0) {
        /* r = sina / sin_sunr = sina re / SUN_RADIUS */
        double c[3], c1[3], c2[3], dsina;
        swi_cross_prod(u, e, c);
        swi_cross_prod(du, e, c1);
        swi_cross_prod(u, de, c2);
        dsina = (dot_prod(c, c1) + dot_prod(c, c2)) / sina;
        dmeff = meff_slope(sina / sin_sunr) * (dsina * re + sina * re1) / SUN_RADIUS;
      }
      dg1 = k1 * (dmeff - meff_fact * re1 / re) / re;
      for (i = 0; i <= 2; i++)
        dg2 += (q[i] + e[i]) * (dq[i] + de[i]);
      f = ru * g1 / g2;
      df = ru1 * g1 / g2 + ru * (dg1 - g1 * dg2 / g2) / g2;
      for (i = 0; i <= 2; i++)
        xx[i+3] += df * (uq * e[i] - ue * q[i])
                 + f * (duq * e[i] + uq * de[i] - due * q[i] - ue * dq[i]);
    }
#else
    dtsp = -DEFL_SPEED_INTV;
    /* U = planetbary(t-tau) - earthbary(t) = planetgeo */
    for (i = 0; i <= 2; i++) 
      u[i] = xx[i] - dtsp * xx[i+3]; 
    /* Eh = earthbary(t) - sunbary(t) = earthhel */
    if (iephe == SEFLG_JPLEPH || iephe == SEFLG_SWIEPH) {
      for (i = 0; i <= 2; i++) 
	e[i] = xearth[i] - xsb[i] -
	       dtsp * (xearth[i+3] - xsb[i+3]); 
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
#endif
  } /* endif speed */
  /* deflected position */
  for (i = 0; i <= 2; i++) 
    xx[i] = xx2[i];
}

/* The observer's site (J2000, relative to the geocenter, AU and AU/day) is
 * a parameter, read when iflag has SEFLG_TOPOCTR: swi_deflect_light() takes
 * it from the planets' save area, the fixed stars' paths from the site they
 * computed for the call (Upstream bug 61). */
static void deflect_light_site(swe_ctx *ctx, double *xx, double dt, int32 iflag, const double *xsite)
{
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  double xearth[6];
  int i;
  for (i = 0; i <= 5; i++)
    xearth[i] = pedp->x[i];
  if (iflag & SEFLG_TOPOCTR)
    for (i = 0; i <= 5; i++)
      xearth[i] += xsite[i];
  deflect_light_from(xx, dt, iflag, xearth, ctx->pldat[SEI_SUNBARY].x, pedp->iephe);
}

void swi_deflect_light(swe_ctx *ctx, double *xx, double dt, int32 iflag) 
{
  deflect_light_site(ctx, xx, dt, iflag, ctx->topd.xobs);
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
#ifndef SWE_UPSTREAM_COMPAT
  /* bug 102: Moshier's light time in a frame where the Sun moves; bug
   * 109: with aberration too */
  AS_BOOL mosh_bary = pedp->iephe == SEFLG_MOSEPH && !(iflag & SEFLG_TRUEPOS);
  double xsb[6] = {0, 0, 0, 0, 0, 0}, asun[3] = {0, 0, 0};
  const double *vsun = xsb + 3;
#endif
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
#ifdef SWE_UPSTREAM_COMPAT	/* bug 71: no instant is special */
      || ctx->topd.teval == 0
#else				/* bug 92: nor is any ephemeris */
      || ctx->topd.iflgkey != SWI_TOPD_KEY(iflag)
#endif
      ) {
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 102, as for the planets (app_pos_etc_plan()): without
   * aberration Moshier's geocentric Sun had no light time at all, -E(t)
   * where the Sun seen is S(t - tau) - E(t), and its heliocentric Earth
   * was E(t - tau) where the light time ends at the Sun of teval: both
   * v_S tau off, 2.96e-6 deg. The Sun and the Earth are taken in the frame
   * of the Sun at teval with barycentric velocities, the Sun moving; with
   * aberration too (bug 109), where the Sun's distance was 5e-8 AU off. */
  if (mosh_bary) {
    swi_mosh_sun_bary(ctx, pedp->teval, SWI_MOSH_SUN_FULL, xsb, asun);
    for (i = 0; i <= 2; i++)
      xobs[i+3] += vsun[i];
  }
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
      || mosh_bary
#endif
      || (iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR)) {
      for (i = 0; i <= 5; i++) {
        xearth[i] = xobs[i];
	if (pedp->iephe == SEFLG_MOSEPH)
	  xsun[i] = 0;
	else 
	  xsun[i] = psdp->x[i];
      }
#ifndef SWE_UPSTREAM_COMPAT
      if (mosh_bary)		/* the Sun at teval, (0, v_S) */
        for (i = 0; i <= 2; i++)
          xsun[i+3] = vsun[i];
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
	      /* Upstream bug 78: the Sun came back read at t - dt as well and
	       * was subtracted below, where de200 and every planet's
	       * heliocentric light time (bug 26) take the Sun at teval, where
	       * the light arrives: 3.0e-6 deg between the two. xsun keeps the
	       * Sun at teval. */
	      retc = sweplan(ctx, t, SEI_EARTH, SEI_FILE_PLANET, iflag, NO_SAVE, xearth, NULL, NULL, NULL, serr);
#else
	      retc = sweplan(ctx, t, SEI_EARTH, SEI_FILE_PLANET, iflag, NO_SAVE, xearth, NULL, xsun, NULL, serr);
#endif
            } else {
	      retc = sweph(ctx, t, SEI_SUNBARY, SEI_FILE_PLANET, iflag, NULL, NO_SAVE, xsun, serr);
	    }
	    break;
	  case SEFLG_MOSEPH:
	    if ((iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR))
	      retc = swi_moshplan(ctx, t, SEI_EARTH, NO_SAVE, xearth, xearth, serr);
	    /* with moshier there is no barycentric sun */
#ifndef SWE_UPSTREAM_COMPAT
	    /* bug 102: but the Sun's displacement over s = t - teval */
	    if (mosh_bary && retc == OK) {
	      if (!(iflag & SEFLG_HELCTR))
	        for (i = 0; i <= 5; i++)
	          xsun[i] = 0;
	      mosh_sun_shift((iflag & SEFLG_HELCTR) ? xearth : xsun, vsun, asun,
	                     t - pedp->teval);
	    }
#endif
	    break;
          default:
	    retc = ERR;
	    break;
	} 
	if (retc != OK)
	  return(retc);
      } 
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 78: the body read at t (the Earth for a heliocentric
       * or barycentric Earth, the Sun for a geocentric Sun) had no
       * light-time rate, which bug 35 gave the planets: 1.7e-6 deg/day on
       * the heliocentric and barycentric Earth's longitude speed against
       * five-point differences, the same at every step, and 6.8e-10 (9.1e-10
       * on de200) on the geocentric Sun's without aberration (G77). v dtau/dt
       * from the read body's own vectors and the centre's at teval
       * (lt_rate_exact()). And it was read at t
       * rounded to a double, which jumps the Earth's place by up to 2.3e-10
       * deg from one date to the next: moved along its velocity at teval
       * by (teval - t) - dt, as the planets (bug 63). */
      {
        AS_BOOL earth = (iflag & SEFLG_HELCTR) || (iflag & SEFLG_BARYCTR);
        double *xb = earth ? xearth : xsun;
        const double *vb = earth ? pedp->x : psdp->x;
        double xc[6], xxsp[3], eps = (pedp->teval - t) - dt;
        double vbm[6];		/* bug 102: Moshier's, barycentric */
        if (mosh_bary) {
          for (i = 0; i <= 2; i++) {
            vbm[i] = earth ? pedp->x[i] : 0;
            vbm[i+3] = (earth ? pedp->x[i+3] : 0) + vsun[i];
          }
          vb = vbm;
        }
        for (i = 0; i <= 2; i++)
          xb[i] += eps * vb[i+3];
        if (iflag & SEFLG_SPEED) {
          for (i = 0; i <= 5; i++)
            xc[i] = !earth ? xobs[i] : (iflag & SEFLG_BARYCTR) ? 0 : xsun[i];
          lt_rate_exact(xb, xc, xxsp);
          for (i = 0; i <= 2; i++)
            xb[i+3] -= xxsp[i];
        }
      }
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* The aberration rate term the other paths have, which this one did not
     * (bug 34): the observer's velocity now less at the instant the light
     * left. The Earth's own acceleration points at the Sun, but not all of
     * it -- the Moon's pull is 0.6% of the Sun's, 5.6e-7 deg/day of the
     * Sun's speed -- and a site's turning not at all (3.2e-4). */
    if (iflag & SEFLG_SPEED) {
      double tlt = pedp->teval - sqrt(square_sum(xx)) * AUNIT / CLIGHT / 86400.0;
      double xelt[6], xobs2[6];
      switch (pedp->iephe) {
        case SEFLG_JPLEPH:
          retc = swi_pleph(ctx, tlt, J_EARTH, J_SBARY, xelt, serr);
          break;
        case SEFLG_SWIEPH:
          retc = sweplan(ctx, tlt, SEI_EARTH, SEI_FILE_PLANET, iflag, NO_SAVE, xelt, NULL, NULL, NULL, serr);
          break;
        default:
          retc = swi_moshplan(ctx, tlt, SEI_EARTH, NO_SAVE, xelt, xelt, serr);
          break;
      }
      if (retc != OK)
        return retc;
      if (iflag & SEFLG_TOPOCTR) {
        /* bug 109: built heliocentric, then the Sun's motion */
        double xoh[6];
        for (i = 0; i <= 5; i++)
          xoh[i] = xobs[i] - (i >= 3 ? vsun[i-3] : 0);
        if (observer_at_lt(ctx, pedp->teval, tlt, iflag, xoh, pedp->x, xelt, xobs2, serr) != OK)
          return ERR;
      } else {
        earth_at_lt(ctx, pedp->teval, tlt, pedp->x, xelt, xobs2);
      }
      if (mosh_bary)
        mosh_sun_shift(xobs2, vsun, asun, tlt - pedp->teval);
      add_aberr_rate(xx, xobs, xobs2);
    }
#endif
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
  return app_pos_rest(ctx, pedp, pedp->teval, iflag, xx, xxsv, oe, serr);
}


#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 53: the accelerations, AU/day^2, J2000 equator, of the
 * Moshier Moon about the Earth (xm, geocentric) and of the Earth about the
 * Sun (xe, heliocentric), for carrying both over the Moon's light time:
 * the Earth-Moon pair's pull and the Sun's tide on the Moon; the Sun's
 * pull on the Earth-Moon barycentre, less the Earth's share of the
 * Moon's motion about it. Left out: the planets (Venus ~3e-5 of the
 * Earth's), the Earth's figure (~5e-7 of the Moon's); over 1.3 s each is
 * below 1e-8 deg/day of the speed. */
static void mosh_moon_earth_acc(const double *xm, const double *xe,
                                double *am, double *ae)
{
  const double k = 86400.0 * 86400.0 / (AUNIT * AUNIT * AUNIT);
  const double gms = HELGRAVCONST * k;
  const double gmem = GEOGCONST * (1.0 + 1.0 / EARTH_MOON_MRAT) * k;
  double xh[3], xb[3], rm, rh, re, rb;
  int i;
  for (i = 0; i <= 2; i++) {
    xh[i] = xe[i] + xm[i];			/* heliocentric Moon */
    xb[i] = xe[i] + xm[i] / (EARTH_MOON_MRAT + 1.0);	/* barycentre */
  }
  rm = sqrt(square_sum(xm));
  rh = sqrt(square_sum(xh));
  re = sqrt(square_sum(xe));
  rb = sqrt(square_sum(xb));
  rm = rm * rm * rm;
  rh = rh * rh * rh;
  re = re * re * re;
  rb = rb * rb * rb;
  for (i = 0; i <= 2; i++) {
    am[i] = -gmem * xm[i] / rm - gms * (xh[i] / rh - xe[i] / re);
    ae[i] = -(gms + gmem) * xb[i] / rb - am[i] / (EARTH_MOON_MRAT + 1.0);
  }
}
#endif

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
#ifndef SWE_UPSTREAM_COMPAT
  double vb[3], lt_e = 0;
#endif
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psdp = &ctx->pldat[SEI_SUNBARY];
  struct plan_data *pdp = &ctx->pldat[SEI_MOON];
#ifndef SWE_UPSTREAM_COMPAT
  /* bug 102: Moshier's light time in a frame where the Sun moves; bug
   * 109: with aberration too, the Sun's motion from fewer planets there
   * (its direction does not depend on it, its distance to 1e-13 AU).
   * Upstream bug 109, mixed: the Moon is Moshier's but the Earth and the
   * Sun the Swiss files' when the Moon's file is missing (sweplan()'s
   * fallback). That Earth is barycentric, so the frame is the files': no
   * v_S, and the light time read at t as the files' path reads it, where
   * sweplan() falls back to Moshier's Moon again. The Moshier branch took
   * the files' Earth for Moshier's heliocentric one: v_S added to
   * barycentric velocities (bug 102, 3e-6 deg), Moshier's heliocentric
   * Earth read at t against the files' Sun (0.09-0.43 deg heliocentric),
   * and its series' acceleration differenced against the files' Earth
   * (0.2-0.96 deg barycentric, 337 deg/day without aberration). G108 (5). */
  AS_BOOL mosh_mixed = pdp->iephe == SEFLG_MOSEPH && pedp->iephe == SEFLG_SWIEPH;
  AS_BOOL mosh_bary = pdp->iephe == SEFLG_MOSEPH && !mosh_mixed
                      && !(iflag & SEFLG_TRUEPOS);
  int mosh_set = (iflag & SEFLG_NOABERR) ? SWI_MOSH_SUN_FULL : SWI_MOSH_SUN_MOON;
  double xsb[6] = {0, 0, 0, 0, 0, 0}, asun[3] = {0, 0, 0}, xeh[6];
  const double *vsun = xsb + 3;
#endif
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
#ifdef SWE_UPSTREAM_COMPAT	/* bug 71: no instant is special */
      || ctx->topd.teval == 0
#else				/* bug 92: nor is any ephemeris */
      || ctx->topd.iflgkey != SWI_TOPD_KEY(iflag)
#endif
      ) {
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 102, as for the planets (app_pos_etc_plan()): Moshier's
   * Moon and Earth were carried over the light time with the Sun held
   * still, v_S tau off without aberration and heliocentric, 3.0e-6 deg.
   * In the frame of the Sun at teval with barycentric velocities; with
   * aberration too (bug 109: the distance 1.3e-10 AU off), v_S there from
   * Venus, the Earth-Moon barycentre and the giants (SWI_MOSH_SUN_MOON). */
  if (mosh_bary) {
    swi_mosh_sun_bary(ctx, pdp->teval, mosh_set, xsb, asun);
    for (i = 0; i <= 2; i++) {
      xx[i+3] += vsun[i];
      xobs[i+3] += vsun[i];
    }
  }
#endif
  /*******************************
   * light-time                  * 
   *******************************/
  t = pdp->teval;
  if ((iflag & SEFLG_TRUEPOS) == 0) {
    dt = sqrt(square_sum(xxm)) * AUNIT / CLIGHT / 86400.0;     
    t = pdp->teval - dt;
#ifndef SWE_UPSTREAM_COMPAT
    for (i = 0; i <= 2; i++)
      vb[i] = xx[i+3];	/* barycentric velocity at teval, every ephemeris */
    switch (mosh_mixed ? SEFLG_SWIEPH : pdp->iephe) {	/* bug 109, mixed */
#else
    switch(pdp->iephe) {
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
        /* Upstream bug 53: the Moon and the Earth were moved back along
         * their velocities at teval, x - dt v, keeping v: the speed
         * without dt a, 1.1e-4 deg/day without aberration (the Earth's
         * 6e-6 km/s^2 and the Moon's 3e-6 over 1.3 s), 4.6e-5
         * heliocentric. Now both at t, the instant the files are read
         * at, rounding and all, to second order: x + s v + s^2 a / 2,
         * v + s a, s = t - teval exactly; against the series read at t,
         * 1e-18 AU on the Moon, its velocity 1e-13 AU/day (the
         * accelerations' 4e-5, see mosh_moon_earth_acc()). Re-reading the
         * series would cost a fifth of every Moshier Moon (its five-point
         * velocity) and an Earth. Heliocentric, over 500 s, the third
         * order left 4.2e-8 deg/day, its distance rate 7.4e-10 AU/day,
         * constant at every step: upstream bug 109, heliocentric the Moon
         * and the Earth are read at t, the Moon's place and velocity the
         * series' (bug 95), at the cost of a Moon and an Earth there. */
        if (iflag & SEFLG_HELCTR) {
          double xm[6];
          retc = swi_moshplan(ctx, t, SEI_EARTH, NO_SAVE, xe, xe, serr);
          if (retc == OK)
            retc = swi_moshmoon(ctx, t, NO_SAVE, xm, serr);
          if (retc != OK)
            return retc;
          for (i = 0; i <= 5; i++) {
            xx[i] = xm[i] + xe[i];
            xs[i] = 0;
          }
        } else {
          double am[3], ae[3], s = t - pdp->teval;
          mosh_moon_earth_acc(pdp->x, pedp->x, am, ae);
          /* bug 109: without aberration the Earth's acceleration is the
           * series' own (bug 66 part 3), not the gravitational model's,
           * which the series' monthly terms and the planets move by 1e-4:
           * s da was 1.1e-8 deg/day of the geocentric Moon's speed. With
           * aberration it cancels against the aberration rate's Earth,
           * carried the same way. */
          if (mosh_bary && (iflag & SEFLG_NOABERR))
            swi_mosh_earth_accel(ctx, pedp->teval, pedp->x, ae);
          for (i = 0; i <= 2; i++) {
            xe[i] = pedp->x[i] + s * (pedp->x[i+3] + 0.5 * s * ae[i]);
            xe[i+3] = pedp->x[i+3] + s * ae[i];
            xx[i] = pdp->x[i] + s * (pdp->x[i+3] + 0.5 * s * am[i]) + xe[i];
            xx[i+3] = pdp->x[i+3] + s * am[i] + xe[i+3];
            xs[i] = 0;
            xs[i+3] = 0;
          }
        }
        {
          double s = t - pdp->teval;
          for (i = 0; i <= 5; i++)
            xeh[i] = xe[i];	/* heliocentric, for a site's rate below */
          if (mosh_bary) {	/* bug 102: the Sun's displacement */
            mosh_sun_shift(xe, vsun, asun, s);
            mosh_sun_shift(xx, vsun, asun, s);
            mosh_sun_shift(xs, vsun, asun, s);
          }
        }
#else
        /* this method results in an error of a milliarcsec in speed */
        for (i = 0; i <= 2; i++) {
          xx[i] -= dt * xx[i+3];
          xe[i] = pedp->x[i] - dt * pedp->x[i+3];
		  xe[i+3] = pedp->x[i+3];
	  xs[i] = 0;
	  xs[i+3] = 0;
        }
#endif
        break;
    } 
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 23: the light time was taken once, from the distance at
     * teval, |M(teval) - O(teval)| / c, where it is defined by
     * tau = |M(teval - tau) - O(teval)| / c (up to 1.08 mas over 1800-2100),
     * and the Moon was then read at teval - dt rounded to a double (40 us
     * near the present, up to 0.18 mas more). Solve the equation from the
     * one read already made, moving the Moon along its barycentric velocity
     * by the difference between the instant read and teval - tau; each pass
     * gains the factor v/c, so three are exact. The rounding is recovered
     * as (teval - t) - dt, exact since t is near teval; Moshier is carried
     * to the same t (bug 53). */
    {
      double eps0 = (pdp->teval - t) - dt;
      double dtk = dt, e = eps0, xk[3];
      int k;
      for (k = 0; k < 3; k++) {
        for (i = 0; i <= 2; i++)
          xk[i] = xx[i] + e * vb[i] - xobs[i];
        dtk = sqrt(square_sum(xk)) * AUNIT / CLIGHT / 86400.0;
        e = eps0 + dt - dtk;
      }
      for (i = 0; i <= 2; i++)
        xx[i] += e * vb[i];
      /* Upstream bug 73: and the velocity is the one at the instant read,
       * teval - dt, where it is wanted at teval - tau: dt - tau is the
       * barycentric range rate times tau / c, up to 2e-9 day, which the
       * barycentric Moon's acceleration turns into 0.07 m/day, 1.0e-8
       * deg/day of the geocentric Moon's speed. Moved with the position, the
       * acceleration taken between the velocities at the instant read and
       * at teval (Moshier's carried there, bug 53). */
      if (iflag & SEFLG_SPEED) {
        for (i = 0; i <= 2; i++)
          xx[i+3] += e * (vb[i] - xx[i+3]) / (pdp->teval - t);
        lt_e = e;
      }
    }
    /* Upstream bug 32: the Moon is seen at t - tau(t), so the velocity of
     * M(t - tau) - O(t) is v_M (1 - dtau/dt) - v_O, and dtau/dt = rdot / c
     * with rdot = rho^ . (v_M (1 - dtau/dt) - v_O). The rate of the light
     * time was left out: with v_M ~30 km/s barycentric, dtau/dt = 1e-6 is
     * 3e-5 km/s, 4e-4 deg/day at the Moon's distance, which a topocentric
     * observer's diurnal 0.3 km/s along the line of sight reaches (4.3e-4
     * measured) and the geocentric range rate a tenth of (1.7e-5). The
     * planets take it from the light-time displacement a day apart (xxsp in
     * app_pos_etc_plan()); here it is exact, from the vectors at hand. */
    if (iflag & SEFLG_SPEED) {
      double rho[3], r, rv = 0, rvm = 0, taudot;
      const double c = CLIGHT * 86400.0 / AUNIT;   /* AU per day */
      for (i = 0; i <= 2; i++)
        rho[i] = xx[i] - xobs[i];
      r = sqrt(square_sum(rho));
      for (i = 0; i <= 2; i++) {
        rv += rho[i] * (xx[i+3] - xobs[i+3]);
        rvm += rho[i] * xx[i+3];
      }
      taudot = (rv / r) / (c + rvm / r);
      for (i = 3; i <= 5; i++)
        xx[i] -= taudot * xx[i];
    }
#endif
    if (iflag & SEFLG_TOPOCTR) {
#ifndef SWE_UPSTREAM_COMPAT
      if (pdp->iephe != SEFLG_MOSEPH || mosh_mixed) {
        if (observer_at_lt(ctx, pedp->teval, t, iflag, xobs, pedp->x, xe, xobs2, NULL) != OK)
	  return ERR;
      } else {		/* bug 109: built heliocentric, then the Sun's motion */
        double xoh[6];
        for (i = 0; i <= 5; i++)
          xoh[i] = xobs[i] - (i >= 3 ? vsun[i-3] : 0);
        if (observer_at_lt(ctx, pedp->teval, t, iflag, xoh, pedp->x, xeh, xobs2, NULL) != OK)
	  return ERR;
        if (mosh_bary)
          mosh_sun_shift(xobs2, vsun, asun, t - pedp->teval);
      }
#else
      if (swi_get_observer(ctx, t, iflag | SEFLG_NONUT, NO_SAVE, xobs2, NULL) != OK)
	  return ERR;
      for (i = 0; i <= 5; i++)
	xobs2[i] += xe[i];
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 73: the aberration rate term xobs - xobs2 is tau a, and
     * xobs2 was the observer at the instant read, teval - dt, so it was
     * dt a, 1.3e-4 too much or too little where dt - tau is 2e-9 day:
     * 1.0e-8 deg/day of the geocentric Moon's speed, 4.0e-8 topocentric.
     * Scaled to tau, both
     * the Earth's part and the site's being linear in it. */
    if (lt_e != 0)
      for (i = 3; i <= 5; i++)
        xobs2[i] = xobs[i] - (xobs[i] - xobs2[i])
          * ((pdp->teval - t - lt_e) / (pdp->teval - t));
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    if (iflag & SEFLG_SPEED)
      add_aberr_rate(xx, xobs, xobs2);
#else
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
  return app_pos_rest(ctx, pdp, pdp->teval, iflag, xx, xxsv, oe, serr);
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 78: the light time from the barycentre had no rate:
     * 5.8e-9 deg/day on the barycentric Sun's longitude speed against
     * five-point differences, 5.8e-10 without light time. v dtau/dt with
     * the observer at the barycentre (lt_rate_exact()). And the velocity
     * was the one at teval, not at t - dt, off by dt a: 3.1e-9 deg/day
     * with the rate. The Sun read at t - dt for its velocity (the place
     * along the line is off by a dt^2 / 2, 1e-17 AU, and stays, so it is
     * the same with or without SEFLG_SPEED). */
    if (iflag & SEFLG_SPEED) {
      static const double xo[6] = {0, 0, 0, 0, 0, 0};
      double xxsp[3], xs[6], t = psbdp->teval - dt;
      int retc;
      if (iflag & SEFLG_JPLEPH) {
        retc = swi_pleph(ctx, t, J_SUN, J_SBARY, xs, serr);
        if (retc != OK) {
          swi_close_jpl_file(ctx);
          ctx->jpl_file_is_open = FALSE;
          return retc;
        }
      } else {
        retc = sweph(ctx, t, SEI_SUNBARY, SEI_FILE_PLANET, iflag, NULL, NO_SAVE, xs, serr);
        if (retc != OK)
          return retc;
      }
      for (i = 3; i <= 5; i++)
        xx[i] = xs[i];
      lt_rate_exact(xx, xo, xxsp);
      for (i = 0; i <= 2; i++)
        xx[i+3] -= xxsp[i];
    }
#endif
  }
  if (!(iflag & SEFLG_SPEED))
    for (i = 3; i <= 5; i++)
      xx[i] = 0;
  /* ICRS to J2000 */
  if (!(iflag & SEFLG_ICRS) && swi_get_denum(ctx, SEI_SUN, iflag) >= 403) {
#ifndef SWE_UPSTREAM_COMPAT
    swi_bias(ctx, xx, psbdp->teval, iflag, FALSE);	/* bug 86 */
#else
    swi_bias(ctx, xx, psdp->teval, iflag, FALSE);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 86: psdp is the Earth's slot, which holds the result but
   * whose instant is the barycentric Sun's only on the Swiss files: on JPL
   * only the Sun's slot is filled, so the bias, the ayanamsa and (fork) the
   * ecliptic's turning were taken at whatever instant the Earth's slot last
   * held -- JD 0 in a fresh context, NaN since its empty mark is
   * SWI_TJD_NEVER. They are taken at the Sun's instant. */
  return app_pos_rest(ctx, psdp, psbdp->teval, iflag, xx, xxsv, oe, serr);
#else
  return app_pos_rest(ctx, psdp, psdp->teval, iflag, xx, xxsv, oe, serr);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* out of the mean ecliptic of date, which turns at the mean obliquity's
   * rate (bug 33): app_pos_rest() adds it back on the way in */
  if (iflag & SEFLG_SPEED)
    swi_ecl_frame_speed(xx, -swi_obliquity_rate(ctx, pdp->teval, iflag, FALSE));
#endif
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
  return app_pos_rest(ctx, pdp, pdp->teval, iflag, xx, xxsv, oe, serr);
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
	snprintf(serr, AS_MAXCH, "error in ephemeris file: %d coefficients instead of %d. ", nco, pdp->ncoe);
	if (strlen(serr) + strlen(fdp->fnam) < AS_MAXCH - 1) {
	  snprintf(serr, AS_MAXCH, "error in ephemeris file %.180s: %d coefficients instead of %d. ", fdp->fnam, nco, pdp->ncoe);
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 104: free_planets() here emptied the other open files'
   * constants too (see read_const()). */
  file_reject(ctx, ifno, TRUE);
#else
  fclose(fdp->fptr);
  // free(fdp->fptr);  is not from malloc(), must not be freed by us
  fdp->fptr = NULL;
  free_planets(ctx);
#endif
  return ERR;
}

/* Upstream bug 106: the body numbers a file lists index pldat[] (SEI_
 * numbers) or name the numbered asteroid or planetary moon of its slot;
 * a body that is not this slot's -- out of the array, or another file's --
 * wrote over other memory or another open file's constants. Every
 * distributed file lists only its own (sepl 0 and 2-10, semo 1, seas 12-17,
 * the planetary moons their 9nnn). */
static AS_BOOL file_may_hold(int ifno, int ipli)
{
  switch (ifno) {
  case SEI_FILE_PLANET:
    return ipli == SEI_EMB || (ipli >= SEI_MERCURY && ipli <= SEI_SUNBARY);
  case SEI_FILE_MOON:
    return ipli == SEI_MOON;
  case SEI_FILE_MAIN_AST:
    return ipli >= SEI_CHIRON && ipli <= SEI_VESTA;
  default:
    return ipli > SE_PLMOON_OFFSET;
  }
}

/* Column offsets into an astorb.dat element line, counted from the end of
 * the MPC number and name, which is variable-length. */
#define ASTORB_COL_H	35
#define ASTORB_COL_G	42
#define ASTORB_COL_DIAM	51
#define ASTORB_LEN_DIAM	7

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
#ifndef SWE_UPSTREAM_COMPAT
  AS_BOOL touched = FALSE;	/* the bodies' file data written to */
#endif
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
      snprintf(serr, AS_MAXCH, "Ephemeris file name '%.100s' wrong; rename '%.100s' ", s2, s);
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
    /* Upstream bug 106: a number field this long copied past sastnam[] */
    if (lastnam + i >= (int) sizeof(sastnam)) {
      smsg = "d";
      goto file_damage;
    }
    strncpy(sastnam, s, lastnam+i);	// fixed 19-nov-19
    *(sastnam+lastnam+i) = '\0';
    /* save elements, they are required for swe_plan_pheno() */
    strcpy(ctx->astelem, s);
    /* required for magnitude */
    ctx->ast_H = atof(s + ASTORB_COL_H + i);
    ctx->ast_G = atof(s + ASTORB_COL_G + i);
    if (ctx->ast_G == 0) ctx->ast_G = 0.15;
    /* diameter in kilometers, not always given: */
    strncpy(s2, s + ASTORB_COL_DIAM + i, ASTORB_LEN_DIAM);
    *(s2 + ASTORB_LEN_DIAM) = '\0';
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
  if (!(fdp->tfstart < fdp->tfend) || !isfinite(fdp->tfstart) || !isfinite(fdp->tfend)) {
    smsg = "r";	/* upstream bug 106 */
    goto file_damage;
  }
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
  for (kpl = 0; kpl < nplan; kpl++)
    if (!file_may_hold(ifno, fdp->ipl[kpl])) {
      smsg = "o";	/* upstream bug 106 */
      goto file_damage;
    }
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
      fdp->astnam[30] = '\0';	/* strlen() below: upstream bug 106 */
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
  if (swi_crc32((unsigned char *) s, (int) fpos) != ulng) {
    smsg = "n";
    goto file_damage;
  }
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
#ifndef SWE_UPSTREAM_COMPAT
  touched = TRUE;
#endif
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
    /* Upstream bug 106: rot_back() holds MAXORD + 1 coefficients on the
     * stack, do_fread() 1000 bytes of reference ellipse */
    if (pdp->ncoe < 1 || pdp->ncoe > MAXORD + 1) {
      smsg = "p";
      goto file_damage;
    }
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
    /* Upstream bug 106: the segment index, its arithmetic and rot_back()'s
     * angle reductions within what they can hold: a positive segment
     * length, the body's span covering the file's, so that every instant
     * the file answers has a segment, its index inside the file, and the
     * elements finite and small enough for an int number of turns. Every
     * distributed file is well inside (lndx0 + 3 (nndx + 1) <= length;
     * |prot|, |peri| < 1.1e4, |dprot|, |dperi| < 710, |telem| < 8e6). */
    for (i = 0; i < 10; i++)
      if (!isfinite(doubles[i]))
        break;
    if (i < 10 || !(doubles[2] > 0) || doubles[0] > fdp->tfstart || doubles[1] < fdp->tfend
        || (doubles[1] - doubles[0] + 0.1) / doubles[2] > 1e9
        || pdp->lndx0 < 0
        || pdp->lndx0 + 3.0 * ((doubles[1] - doubles[0] + 0.1) / doubles[2] + 1) > (double) flen
        || fabs(doubles[3]) > 1e8 || fabs(doubles[4]) > 1e5 || fabs(doubles[5]) > 1e5
        || fabs(doubles[8]) > 1e5 || fabs(doubles[9]) > 1e5) {
      smsg = "q";
      goto file_damage;
    }
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 104: a refused file -- under another bucket's name,
   * damaged -- ran free_planets(), which empties every body's constants
   * while the other files stay open: the next calls read them with dseg 0
   * and failed in turn, "sepl_06.se1 is damaged (2)", "semo_06.se1 is
   * damaged (2)", three calls after a misnamed seas_06.se1 before one
   * answered, and the spares of bug 89 went with it. Only this file and
   * its bodies' file data are cleared now. */
  file_reject(ctx, ifno, touched);
#else
  fclose(fdp->fptr);
  // free(fdp->fptr);  is not from malloc(), must not be freed by us
  fdp->fptr = NULL;
  free_planets(ctx);
#endif
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
	  snprintf(serr, AS_MAXCH, "Ephemeris file %.220s is damaged (2).", ctx->fidat[ifno].fnam);
	}
      }
      return(ERR);
    } else
      return(OK);
  } else {
    /* upstream bug 106: never more than space[] holds */
    if (totsize < 0 || totsize > (int) sizeof(space)) {
      if (serr != NULL)
	snprintf(serr, AS_MAXCH, "Ephemeris file %.220s is damaged (5).", ctx->fidat[ifno].fnam);
      return(ERR);
    }
    if (fread((void *) &space[0], (size_t) totsize, 1, fp) == 0) {
      if (serr != NULL) {
	strcpy(serr, "Ephemeris file is damaged (3). ");
	if (strlen(serr) + strlen(ctx->fidat[ifno].fnam) < AS_MAXCH - 1) {
	  snprintf(serr, AS_MAXCH, "Ephemeris file %.220s is damaged (4).", ctx->fidat[ifno].fnam);
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

#ifndef SWE_UPSTREAM_COMPAT
/* G (M_earth + M_moon) in AU^3/day^2, for the two-body ellipse
 * lunar_osc_elem() fits to the Moon: the constant the ephemeris in use
 * integrated the Moon with, not a constant from a different almanac.
 *
 * Upstream used GEOGCONST x (1 + 1/EARTH_MOON_MRAT), the Astronomical
 * Almanac's 1996 G M_earth, which is 3.3e-8 above DE440/441's GMB and
 * 3.5e-8 above DE403's. The eccentricity vector of a near-circular orbit
 * amplifies a relative error in GM by about (1 + e)/e = 19, so the
 * osculating apogee was off by up to 0.2" (UPSTREAM-BUGS.md 28, G38).
 *
 * A JPL file carries GMB in its header, and that is used when it is open.
 * The Swiss files and Moshier do not carry one: the file's DE number names
 * the JPL header the value is taken from (ssd.jpl.nasa.gov/ftp/eph/planets/
 * ascii, header.4xx, GROUP 1041, read 2026-10-24); Moshier's lunar theory
 * is fitted to DE404, whose constants are taken to be DE403's
 * (swi_get_denum() answers 403 for it; DE404's own header is not on JPL's
 * server, so that is assumed, and DE403 to DE405 is 3e-9 relative). DE440 and DE441 share one set, as do DE430/431 and DE405/406.
 * The nearest generation is taken for a DE number between them. */
static double gm_earth_moon(swe_ctx *ctx, int32 epheflag)
{
  double gm;
  int32 denum;
  if ((epheflag & SEFLG_JPLEPH) && ctx->jpl_file_is_open
      && swi_get_jpl_const(ctx, "GMB", &gm) == OK && gm > 0)
    return gm;
  denum = swi_get_denum(ctx, SEI_MOON, epheflag);
  if (denum >= 440) return 8.997011392947347e-10;   /* header.440, header.441 */
  if (denum >= 430) return 8.997011390199871e-10;   /* header.430, header.431 */
  if (denum >= 405) return 8.997011346712499e-10;   /* header.405, header.406 */
  if (denum >= 403) return 8.997011374291876e-10;   /* header.403; Moshier */
  return 8.997011658557308e-10;                     /* header.200 */
}

/* Upstream bug 105: swe_nod_aps()'s osculating ellipses, the Moon's about
 * the Earth (G (M_earth + M_moon), above) and the planets' about the Sun:
 * G (M_sun + M_planet) of the ephemeris in use, AU^3/day^2. iel is the
 * planet, 0..8 Mercury..Pluto with 2 the Earth-Moon barycentre, or -1 for
 * the Sun alone (asteroids, fictitious bodies). A JPL file's own GMS and
 * GMi are used when it is open; otherwise those of the header the DE
 * number names, as in gm_earth_moon() (ssd.jpl.nasa.gov/ftp/eph/planets/
 * ascii, GROUP 1041, read 2026-10-24; Moshier's planets are fitted to
 * DE404, taken as DE403). The Sun's is k^2 from DE200 to DE431,
 * 2.9591220828411956e-4 in DE440/441; upstream's HELGRAVCONST / AUNIT^3
 * is 1.80e-10 below the first, 1.76e-10 below the second, and its
 * planets' masses are DE431's but Mercury's and Pluto's. */
double swi_gm_earth_moon(swe_ctx *ctx, int32 epheflag)
{
  return gm_earth_moon(ctx, epheflag);
}

/* Upstream bug 108: the Earth-Moon mass ratio the Earth's place was made
 * from the EMB with, to make the EMB again: a JPL file's own EMRAT
 * (swi_pleph() takes the Earth from the file's EMB with it), else
 * EARTH_MOON_MRAT, the ratio embofs() and Moshier's series take the Earth
 * from the Swiss files' and the series' EMB with. */
double swi_emb_mrat(swe_ctx *ctx, int32 epheflag)
{
  double em;
  if ((epheflag & SEFLG_JPLEPH) && ctx->jpl_file_is_open
      && swi_get_jpl_const(ctx, "EMRAT", &em) == OK && em > 0)
    return em;
  return EARTH_MOON_MRAT;
}

static void gm_sun_planet(swe_ctx *ctx, int32 epheflag, int iel, double *pgms, double *pgmp)
{
  static const char *gmname[9] = {"GM1", "GM2", "GMB", "GM4", "GM5", "GM6",
                                  "GM7", "GM8", "GM9"};
  static const double gmde[5][10] = {
    /* GMS, GM1 .. GM9 (GMB for the Earth-Moon barycentre) */
    {2.9591220828411956e-4, 4.912500194889318e-11, 7.243452332644119e-10,
     8.997011392947347e-10, 9.549548829725812e-11, 2.825345825225792e-07,
     8.45970599337629e-08, 1.29202656496824e-08, 1.524357347885194e-08,
     2.175096464893358e-12},				/* header.440, .441 */
    {2.959122082855911e-4, 4.91248045036476e-11, 7.24345233264412e-10,
     8.997011390199871e-10, 9.54954869555077e-11, 2.82534584083387e-07,
     8.45970607324503e-08, 1.29202482578296e-08, 1.52435734788511e-08,
     2.17844105197418e-12},				/* header.430, .431 */
    {2.959122082855911e-4, 4.912547451450812e-11, 7.243452486162703e-10,
     8.997011346712499e-10, 9.549535105779258e-11, 2.8253459095242264e-07,
     8.459715185680659e-08, 1.2920249167819694e-08, 1.5243589007842763e-08,
     2.1886997654259697e-12},				/* header.405, .406 */
    {2.959122082855911e-4, 4.912547451450812e-11, 7.243452486162703e-10,
     8.997011374291876e-10, 9.549535105779258e-11, 2.825345909524227e-07,
     8.459715185680659e-08, 1.29202491678197e-08, 1.524358900784276e-08,
     2.18869976542597e-12},				/* header.403; Moshier */
    {2.959122082855911e-4, 4.912547451450812e-11, 7.243456209632766e-10,
     8.997011658557308e-10, 9.549528942224058e-11, 2.8253421034459264e-07,
     8.459468504830659e-08, 1.288816238138035e-08, 1.5321124812842762e-08,
     2.2762477518636993e-12},				/* header.200 */
  };
  double gms, gmp = 0;
  int32 denum;
  int k;
  if ((epheflag & SEFLG_JPLEPH) && ctx->jpl_file_is_open
      && swi_get_jpl_const(ctx, "GMS", &gms) == OK && gms > 0
      && (iel < 0 || iel > 8
          || (swi_get_jpl_const(ctx, gmname[iel], &gmp) == OK && gmp > 0))) {
    *pgms = gms;
    *pgmp = gmp;
    return;
  }
  denum = swi_get_denum(ctx, SEI_EARTH, epheflag);
  k = denum >= 440 ? 0 : denum >= 430 ? 1 : denum >= 405 ? 2 : denum >= 403 ? 3 : 4;
  *pgms = gmde[k][0];
  *pgmp = iel >= 0 && iel <= 8 ? gmde[k][1 + iel] : 0;
}

double swi_gm_sun_planet(swe_ctx *ctx, int32 epheflag, int iel)
{
  double gms, gmp;
  gm_sun_planet(ctx, epheflag, iel, &gms, &gmp);
  return gms + gmp;
}

/* bug 108: the planet's GM alone (0 for iel -1), for
 * swe_get_orbital_elements()'s SEFLG_ORBEL_AA sum of the masses inside */
double swi_gm_planet(swe_ctx *ctx, int32 epheflag, int iel)
{
  double gms, gmp;
  gm_sun_planet(ctx, epheflag, iel, &gms, &gmp);
  return gmp;
}
#endif

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 75: the rounding error of s = a + b, exactly (TwoSum): the
 * true sum less s, so an epoch's rounding can be undone to first order. */
static double osc_round_err(double a, double b, double s)
{
  double bb = s - a;
  return (a - (s - bb)) + (b - bb);
}

/* The true node's and osculating apogee's speeds: a five-point difference
 * of their positions from the Moon at epochs OSC_INTV_* day apart (binary,
 * so tjd + k x step is exact), the step of least truncation plus noise.
 * The noise is the Moon's velocity's, which the apogee amplifies about
 * twentyfold (1 / eccentricity): over a three-point 0.0001 day it was
 * 5e-7 deg/day on the apogee. Measured against a nine-point difference
 * over 2^-3 day, 150 dates (UPSTREAM-BUGS 75): five points over 2^-6 day
 * on the Swiss files and JPL (2^-5: truncation in the distance; 2^-4 in
 * all), over 2^-5 on Moshier, whose Moon is noisier (2^-4: truncation in
 * the node). OSC_NPT_* 3 takes the three-point slope instead. */
# ifndef OSC_NPT_SWI
#  define OSC_NPT_SWI   5
#  define OSC_INTV_SWI  (1.0 / 64)
#  define OSC_NPT_JPL   5
#  define OSC_INTV_JPL  (1.0 / 64)
#  define OSC_NPT_MOSH  5
#  define OSC_INTV_MOSH (1.0 / 32)
# endif
#endif

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
  int i, j, k, kstart = 2, npt = 3;
  int ipli = SEI_MOON;
  /* the Moon's epochs, tjd + osc_off[i] x speed_intv: 0 and 1 are the
   * three-point slope's, 2 is tjd, 3 and 4 the five-point's outer ones;
   * computed in the order ord[], tjd last */
  static const double osc_off[5] = {-1, 1, 0, -2, 2};
  static const int ord3[3] = {0, 1, 2}, ord5[5] = {0, 1, 3, 4, 2};
  const int *ord = ord3;
  int32 epheflag = SEFLG_DEFAULTEPH; 
  int retc = ERR; 
  int32 flg1, flg2;
  double daya[2];
  struct plan_data *ndp, *ndnp, *ndap;
  struct epsilon *oe;
  double speed_intv = NODE_CALC_INTV;	/* to silence gcc warning */
  double xpos[5][6], xx[5][6], xxa[5][6], xnorm[6], r[6];
  double *xp;
  double rxy, rxyz, t, dt, fac, sgn;
  double sinnode, cosnode, sinincl, cosincl, sinu, cosu, cosE;
  double uu, ny, sema, ecce, Gmsm, c2, v2;
#ifdef SWE_UPSTREAM_COMPAT
  double sinE, pp;
#endif
  int32 speedf1, speedf2;
#ifndef SWE_UPSTREAM_COMPAT
  double rr[5] = {0, 0, 0, 0, 0};
#endif
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
#ifdef SWE_UPSTREAM_COMPAT	/* bug 71: no instant is special */
	&& tjd != 0 
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  ctx->pldat[SEI_MOON].teval = SWI_TJD_NEVER;	/* Upstream bug 71 */
#else
  ctx->pldat[SEI_MOON].teval = 0;
#endif
  if (serr != NULL)
    *serr = '\0';
  three_positions:
  switch(epheflag) {
    case SEFLG_JPLEPH:
#ifndef SWE_UPSTREAM_COMPAT
      speed_intv = OSC_INTV_JPL;
      npt = OSC_NPT_JPL;
#else
      speed_intv = NODE_CALC_INTV;
#endif
      ord = (npt == 5) ? ord5 : ord3;
      kstart = (iflag & SEFLG_SPEED) ? 0 : npt - 1;
      for (k = kstart; k < npt; k++) {
	i = ord[k];
	t = tjd + osc_off[i] * speed_intv;
#ifndef SWE_UPSTREAM_COMPAT
	rr[i] = osc_round_err(tjd, osc_off[i] * speed_intv, t);	/* bug 75 */
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
	  rr[i] += osc_round_err(t, -dt, t - dt);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
      speed_intv = OSC_INTV_SWI;
      npt = OSC_NPT_SWI;
#else
      speed_intv = NODE_CALC_INTV;
#endif
      ord = (npt == 5) ? ord5 : ord3;
      kstart = (iflag & SEFLG_SPEED) ? 0 : npt - 1;
      for (k = kstart; k < npt; k++) {
	i = ord[k];
	t = tjd + osc_off[i] * speed_intv;
#ifndef SWE_UPSTREAM_COMPAT
	rr[i] = osc_round_err(tjd, osc_off[i] * speed_intv, t);	/* bug 75 */
#endif
	retc = swemoon(ctx, t, iflag | SEFLG_SPEED, NO_SAVE, xpos[i], serr);/**/
	if (retc == ERR)
	  return(ERR);
	/* light-time-corrected moon for apparent node (~ 0.006") */
	if ((iflag & SEFLG_TRUEPOS) == 0 && retc >= OK) { 
	  dt = sqrt(square_sum(xpos[i])) * AUNIT / CLIGHT / 86400.0;     
	  retc = swemoon(ctx, t-dt, iflag | SEFLG_SPEED, NO_SAVE, xpos[i], serr);/**/
#ifndef SWE_UPSTREAM_COMPAT
	  rr[i] += osc_round_err(t, -dt, t - dt);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 51: the oscillation was the Moshier Moon's speed,
       * noisy at 2.4e-6 of itself; the 0.1-day step that averaged it
       * out cost 1.5e-3 deg/day of truncation on the apogee's speed at
       * -3000 (5.8 deg/day, curving with the half-month). With that
       * speed smooth, +-2^-8 day, exact instants: 3.4e-6 deg/day. Bug
       * 75: five points 2^-5 day apart, 4.7e-7 (the node 6.6e-9). */
      speed_intv = OSC_INTV_MOSH;
      npt = OSC_NPT_MOSH;
#else
      speed_intv = NODE_CALC_INTV_MOSH;
#endif
      ord = (npt == 5) ? ord5 : ord3;
      kstart = (iflag & SEFLG_SPEED) ? 0 : npt - 1;
      for (k = kstart; k < npt; k++) {
	i = ord[k];
	t = tjd + osc_off[i] * speed_intv;
#ifndef SWE_UPSTREAM_COMPAT
	rr[i] = osc_round_err(tjd, osc_off[i] * speed_intv, t);	/* bug 75 */
#endif
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
  for (k = kstart; k < npt; k++) {
    i = ord[k];
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
#ifdef SWE_UPSTREAM_COMPAT
  /* save position and speed (overwritten below) */
  for (i = 0; i <= 2; i++) {
    ndnp->x[i] = xx[2][i];
    if (iflag & SEFLG_SPEED) {
      double a, b;
      b = (xx[1][i] - xx[0][i]) / 2;
      a = (xx[1][i] + xx[0][i]) / 2 - xx[2][i];
      ndnp->x[i+3] = (2 * a + b) / speed_intv;
    } else
      ndnp->x[i+3] = 0;
    ndnp->teval = tjd;
    ndnp->iephe = epheflag;
  }
#else
  /* (bug 31's slope here, overwritten below and never returned, is gone) */
  ndnp->teval = tjd;
  ndnp->iephe = epheflag;
#endif
  /************************************************************
   * apogee with speed                                        * 
   * must be computed anyway to get the node's distance       *
   ************************************************************/
  ndap = &ctx->nddat[SEI_OSCU_APOG];
#ifndef SWE_UPSTREAM_COMPAT
  Gmsm = gm_earth_moon(ctx, epheflag);
#else
  Gmsm = GEOGCONST * (1 + 1 / EARTH_MOON_MRAT) /AUNIT/AUNIT/AUNIT*86400.0*86400.0;
#endif
  /* three apogees */
  for (k = kstart; k < npt; k++) {
    i = ord[k];
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 95: the eccentricity was sqrt(1 - p / a), 1 - p / a
     * being e^2 less a few ulps of 1, and the true anomaly from it through
     * cos E = (1 - r / a) / e: with the Moon's e = 0.055 the apogee
     * jittered by up to 3.5e-10 deg between instants 2^-30 day apart
     * (de200; Swiss 1.1e-11), where the Moon's state allows 1e-12, and
     * its five-point speed by 2.4e-8 deg/day (bug 80 fixed the same in
     * swe_nod_aps()).
     * Now the eccentricity vector, ((v^2 - GM / r) r - (r.v) v) / GM: e
     * its length and the true anomaly the angle from it to r about h. */
    {
      double ev[3], evr[3], rv = dot_prod(xpos[i], xpos[i]+3);
      for (j = 0; j <= 2; j++)
        ev[j] = ((v2 - Gmsm / rxyz) * xpos[i][j] - rv * xpos[i][j+3]) / Gmsm;
      ecce = sqrt(square_sum(ev));
      swi_cross_prod(ev, xpos[i], evr);
      ny = atan2(dot_prod(evr, xnorm) / sqrt(c2), dot_prod(ev, xpos[i]));
    }
#else
    /* eccentricity */
    pp = c2 / Gmsm;
    ecce = sqrt(1 - pp / sema);	
    /* eccentric anomaly */
    cosE = 1 / ecce * (1 - rxyz / sema);	
    sinE = 1 / ecce / sqrt(sema * Gmsm) * dot_prod(xpos[i], (xpos[i]+3));
    /* true anomaly */
    ny = 2 * atan(sqrt((1+ecce)/(1-ecce)) * sinE / (1 + cosE));
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 75: the difference was divided by 2 x 0.0001 day, but
   * the Moon's epochs, tjd +- 0.0001 less the light-time, are rounded to
   * the date's grid (2^-31 day from JD 2097152 to 4194304): the interval
   * between them is off by up to 2.3e-6 of itself, steadily (1.7e-6 at
   * binary dates) and at random with the light-time. Every speed was off
   * by that fraction of itself on the Swiss files and JPL: 2.3e-7 deg/day
   * on the node's longitude, 6e-6 on the apogee's. Now each epoch's
   * rounding, rr[i] (exact, from osc_round_err()), is taken back to first
   * order: x(epoch + rr) = x + v rr, so the slope over the unrounded
   * epochs is the slope over the rounded ones times 1 + D[rr], D the same
   * difference. And the step and the formula are those of least total
   * error (truncation plus the Moon's velocity noise): OSC_NPT_* points
   * OSC_INTV_* apart, see there. */
  for (i = 0; i <= 2; i++) {
    ndap->x[i] = xxa[2][i];
    ndnp->x[i] = xx[2][i];
    ndap->x[i+3] = ndnp->x[i+3] = 0;
  }
  ndap->teval = tjd;
  ndap->iephe = epheflag;
  if (iflag & SEFLG_SPEED) {
    double fr;
    if (npt == 5) {
      fr = 1 + (rr[3] - 8 * rr[0] + 8 * rr[1] - rr[4]) / (12 * speed_intv);
      for (i = 0; i <= 2; i++) {
        ndap->x[i+3] = (xxa[3][i] - 8 * xxa[0][i] + 8 * xxa[1][i] - xxa[4][i])
                       / (12 * speed_intv) * fr;
        ndnp->x[i+3] = (xx[3][i] - 8 * xx[0][i] + 8 * xx[1][i] - xx[4][i])
                       / (12 * speed_intv) * fr;
      }
    } else {
      fr = 1 + (rr[1] - rr[0]) / (2 * speed_intv);
      for (i = 0; i <= 2; i++) {
        ndap->x[i+3] = (xxa[1][i] - xxa[0][i]) / (2 * speed_intv) * fr;
        ndnp->x[i+3] = (xx[1][i] - xx[0][i]) / (2 * speed_intv) * fr;
      }
    }
  }
#else
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
#endif
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
    if (iflag & SEFLG_SPEED) {
#ifndef SWE_UPSTREAM_COMPAT
      /* out of the true ecliptic of date, which turns (bug 33). Bug 75:
       * bug 33 left J2000 out, but the node and apogee are found in the
       * mean ecliptic of date and precessed to J2000 below, so their J2000
       * latitude and declination speeds missed the mean obliquity's rate,
       * 3.5e-7 deg/day. Only SID_TNODE_FROM_ECL_T0 finds them in the fixed
       * J2000 ecliptic. */
      double xe[6];
      for (i = 0; i <= 5; i++)
        xe[i] = ndp->xreturn[6+i];
#ifdef SID_TNODE_FROM_ECL_T0
      if (!(iflag & SEFLG_J2000))
#endif
        swi_ecl_frame_speed(xe, -swi_obliquity_rate(ctx, tjd, iflag, !(iflag & SEFLG_NONUT)));
      swi_coortrf2(xe+3, ndp->xreturn+21, -oe->seps, oe->ceps);
#else
      swi_coortrf2(ndp->xreturn+9, ndp->xreturn+21, -oe->seps, oe->ceps);
#endif
    }
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
#ifdef SWE_UPSTREAM_COMPAT
  double speed_intv = 0.1;
  double t, dt;
#endif
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
#ifdef SWE_UPSTREAM_COMPAT	/* bug 71: no instant is special */
	&& tjd != 0 
#endif
	&& flg1 == flg2
	&& (!speedf2 || speedf1)) {
    ndp->xflgs = iflag;
    ndp->iephe = iflag & SEFLG_MOSEPH;
    return OK;
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 44: the light-time step below, x -= tau v, took v from the
   * speed, which is zero without SEFLG_SPEED, and from i = 1, so the x
   * coordinate never had it: the position depended on the speed flag, the
   * perigee by 111 mas (tau v, tau = 1.2 s, v up to 2.3 deg/day), the
   * apogee by 13. The apsis, like the osculating one built from the Moon
   * at t - tau, is seen at t - tau: it is now computed there, for every
   * flag set (TRUEPOS aside). tau is the apsis's distance over c, from the
   * first passes of the iteration at t (swi_intp_apsides_est()): the
   * distance is stationary at the apsis, so two passes put the perigee's
   * within 4.3 km and one the apogee's within 1 km over 20000 dates
   * -10000..+12000, at most 0.0013 mas of position (one pass would leave
   * the perigee 92 km). The estimate is seven (perigee) and four (apogee)
   * lunar evaluations (~42,000 instructions each) besides the iteration's
   * own: with bugs 45, 56 and 72 a call without the speed flag is 1.57
   * (perigee) and 1.29 (apogee) of upstream's instructions, at instants a
   * day apart 1.99 and 1.80 with this build's IAU 2000A nutation node; the
   * three apsides of a velocity for tau V cost 3.2 (UPSTREAM-BUGS 44).
   * Upstream bug 45: the speed was the difference over +-0.1 day, off by
   * the h^2 term, 1.1e-4 deg/day on the perigee. It is now the three
   * positions 2^-8 day apart about t - tau, where that term (1.7e-7) and
   * the positions' noise over 2h (0.0018 mas, 7e-8) balance, times
   * 1 - dtau/dt: the rate of the position returned, within 2.1e-7 deg/day
   * of a five-point difference. */
  {
    const double h = INTP_APS_SPEED_INTV;
    double tlt = tjd, taudot = 0, est[3], r, rdot;
    if (!(iflag & SEFLG_TRUEPOS)) {
      /* Bug 72 (this fork's): the pass count compared ipl, an SEI_ index,
       * with SE_INTP_PERG, so the perigee's estimate took one pass, not
       * two: its distance up to 92 km from the converged one (rms 20, 20000
       * dates -10000..+12000), its position 0.0049 mas and its speed 1.1e-7
       * deg/day off (-3000..+3000). Two passes: 4.3 km (rms 1.1). */
      swi_intp_apsides_est(ctx, tjd, est, ipl, ipl == SEI_INTP_PERG ? 2 : 1);
      tlt = tjd - est[2] * AUNIT / CLIGHT / 86400.0;
    }
    memset((void *) ndp->xreturn, 0, 24 * sizeof(double));
    if (iflag & SEFLG_SPEED) {
      for (i = 0; i < 3; i++) {
        swi_intp_apsides(ctx, tlt + (i - 1) * h, xpos[i], ipl);
        swi_polcart(xpos[i], xpos[i]);
      }
      for (i = 0; i < 3; i++) {
        xx[i] = xpos[1][i];
        xx[i+3] = (xpos[2][i] - xpos[0][i]) / (2 * h);
      }
      if (!(iflag & SEFLG_TRUEPOS)) {
        r = sqrt(square_sum(xx));
        rdot = (xx[0] * xx[3] + xx[1] * xx[4] + xx[2] * xx[5]) / r;
        taudot = rdot * AUNIT / CLIGHT / 86400.0;
        for (i = 3; i < 6; i++)
          xx[i] *= 1 - taudot;
      }
    } else {
      swi_intp_apsides(ctx, tlt, xx, ipl);
      swi_polcart(xx, xx);
      for (i = 3; i < 6; i++)
        xx[i] = 0;
    }
  }
#else
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
#endif
  for (i = 0; i <= 5; i++) 
    ndp->xreturn[i+6] = xx[i];
  /*printf("%.10f, %.10f, %.10f, %.10f\n", xx[0] /DEGTORAD, xx[1] / DEGTORAD, xx [2], xx[3] /DEGTORAD);*/
  /* equatorial cartesian */
  swi_coortrf2(ndp->xreturn+6, ndp->xreturn+18, -oe->seps, oe->ceps);
  if (iflag & SEFLG_SPEED) {
#ifndef SWE_UPSTREAM_COMPAT
    /* out of the mean ecliptic of date, which turns (bug 33) */
    for (i = 0; i <= 5; i++)
      x[i] = ndp->xreturn[6+i];
    swi_ecl_frame_speed(x, -swi_obliquity_rate(ctx, tjd, iflag, FALSE));
    swi_coortrf2(x+3, ndp->xreturn+21, -oe->seps, oe->ceps);
#else
    swi_coortrf2(ndp->xreturn+9, ndp->xreturn+21, -oe->seps, oe->ceps);
#endif
  }
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
#ifndef SWE_UPSTREAM_COMPAT
    /* into the true ecliptic of date, which turns (bug 33) */
    if (iflag & SEFLG_SPEED)
      swi_ecl_frame_speed(ndp->xreturn+6, swi_obliquity_rate(ctx, tjd, iflag, !(iflag & SEFLG_NONUT)));
#endif
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
    /* Upstream bug 67: ctx->nut2000 is never filled and ctx->nutv no longer
     * is (bug 58); their tnut of 0 matched tjd = 0 (nutv) and J2000
     * (nut2000) with a zero matrix. Neither is read now. */
#ifdef SWE_UPSTREAM_COMPAT
    } else if (tjd == J2000) {
      nutp = &ctx->nut2000;
    } else if (tjd == ctx->nutv.tnut) {
      nutp = &ctx->nutv;
#endif
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

#ifndef SWE_UPSTREAM_COMPAT
/* meff()'s slope dm/dr on the segment of its table meff(r) interpolates
 * on (Upstream bug 74) */
static double meff_slope(double r)
{
  int i;
  if (r <= 0 || r >= 1)
    return 0.0;
  for (i = 0; eff_arr[i].r > r; i++)
    ;	/* empty body */
  return (eff_arr[i].m - eff_arr[i-1].m) / (eff_arr[i].r - eff_arr[i-1].r);
}
#endif

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
  double b;
  for (j = 0; j <= 18; j += 6) {
    for (i = 0; i < 3; i++) {
      k = j + i;
      b = (x2[k] - x0[k]) / 2;
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 31: x0, x1, x2 are at t - dt, t, t + dt, and the
       * parabola through them, x1 + b s + a s^2 in steps of dt, has the slope
       * b at t. 2 a + b is its slope at s = 1: the speed 4.32 s (the Moon)
       * or 8.64 s (a planet) late, off by the acceleration times dt --
       * 1.3e-3 deg/day on the topocentric Moon, whose diurnal parallax
       * curves its path at ~30 deg/day^2. */
      x1[k+3] = b / dt;
#else
      {
        double a = (x2[k] + x0[k]) / 2 - x1[k];
        x1[k+3] = (2 * a + b) / dt;
      }
#endif
    }
  }
}

void swi_check_ecliptic(swe_ctx *ctx, double tjd, int32 iflag)
{
  /* The flags are part of the key, not just the epoch: a value cached under
   * SEFLG_JPLHOR_APPROX carries the Horizons obliquity offset, and handing it
   * to a caller that did not ask for it moved a mean node by 0.065 arcsec. */
  if (ctx->oec2000.teps != J2000
      || ctx->oec2000.epsflag != SWI_EPS_KEY(iflag)) {
    calc_epsilon(ctx, J2000, iflag, &ctx->oec2000);
  }
  if (tjd == J2000) {
    ctx->oec.teps = ctx->oec2000.teps;
    ctx->oec.eps = ctx->oec2000.eps;
    ctx->oec.seps = ctx->oec2000.seps;
    ctx->oec.ceps = ctx->oec2000.ceps;
    ctx->oec.epsflag = ctx->oec2000.epsflag;
    return;
  }
  if (ctx->oec.teps != tjd
#ifdef SWE_UPSTREAM_COMPAT	/* bug 71: no instant is special */
      || tjd == 0
#endif
      || ctx->oec.epsflag != SWI_EPS_KEY(iflag)) {
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
#ifdef SWE_UPSTREAM_COMPAT
  double t;
#endif
  speedf1 = ctx->sp.nut.nutflag & SEFLG_SPEED;
  speedf2 = iflag & SEFLG_SPEED;
  /* The JPLHOR bits belong in the key for the same reason they belong in
   * swi_check_ecliptic()'s: calc_nutation() branches on them, so a value
   * cached under SEFLG_JPLHOR_APPROX is not the one a caller without it
   * asked for. calc_nutation() already declines its own memo for these
   * flags -- SWI_NUT_NO_MEMO -- and this is the same argument one level up. */
  if (!(iflag & SEFLG_NONUT)
	&& (tjd != ctx->nut.tnut
#ifdef SWE_UPSTREAM_COMPAT
	|| tjd == 0	/* upstream bug 67: an emptied cache has tnut 0 */
#endif
	|| (!speedf1 && speedf2)
	|| SWI_EPS_KEY(ctx->sp.nut.nutflag) != SWI_EPS_KEY(iflag))) {
    swi_nutation(ctx, tjd, iflag, ctx->nut.nutlo);
    ctx->nut.tnut = tjd;
    ctx->nut.snut = sin(ctx->nut.nutlo[1]);
    ctx->nut.cnut = cos(ctx->nut.nutlo[1]);
    ctx->sp.nut.nutflag = iflag;
    nut_matrix(&ctx->nut, &ctx->oec);
#ifndef SWE_UPSTREAM_COMPAT
    ctx->nut.dnut_ok = FALSE;
    if (iflag & SEFLG_SPEED) {
      swi_nutation_rate(ctx, tjd, iflag, ctx->nut.dnut);
      /* Upstream bug 69: the matrix turns with the mean obliquity too. */
      ctx->nut.deps0 = swi_obliquity_rate(ctx, tjd, iflag, FALSE);
      ctx->nut.dnut_ok = TRUE;
    }
#else
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
#endif
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
  if (i >= 2)	/* a line without a comma left cpos[1] NULL: upstream bug 106 */
    swi_right_trim(cpos[1]);
  if (i < 14) {
    if (serr != NULL) {
      if (i >= 2) {
	snprintf(serr, AS_MAXCH, "data of star '%.40s,%.40s' incomplete", cpos[0], cpos[1]);
      } else {
        if (strlen(s) > 200) s[200] = '\0';
	snprintf(serr, AS_MAXCH, "invalid line in fixed stars file: '%.200s'", s);
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
#ifndef SWE_UPSTREAM_COMPAT
static int32 load_fixed_stars_file(swe_ctx *ctx, char *serr)
#else
static int32 load_all_fixed_stars(swe_ctx *ctx, char *serr) 
#endif
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
  /* Upstream bug 113: a load that failed part way left its records, and
   * the next call's load dropped the pointer to them */
  free(ctx->fixed_stars);
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

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 121: a load that failed left n_fixstars_records 0, so
 * every later swe_fixstar2() and swe_fixstar2_mag() loaded again: a
 * catalogue broken at line 500 was reread and reparsed up to it on every
 * call (842 us a call for 2.8), and a missing one searched for along the
 * path, sefstars.txt and fixstars.cat, two fopen() a call. The failure
 * and its error are now kept until the path changes or swe_close()
 * (swi_fixstar_forget()), and each later call reports the same error
 * without touching the file; the partial table is freed at once. */
static int32 load_all_fixed_stars(swe_ctx *ctx, char *serr)
{
  int32 retc;
  if (ctx->fixstars_load_failed) {
    if (serr != NULL)
      strcpy(serr, ctx->fixstars_load_err);
    return ERR;
  }
  *ctx->fixstars_load_err = '\0';
  retc = load_fixed_stars_file(ctx, ctx->fixstars_load_err);
  if (retc == ERR) {
    ctx->fixstars_load_failed = TRUE;
    free(ctx->fixed_stars);
    ctx->fixed_stars = NULL;
  }
  if (serr != NULL && *ctx->fixstars_load_err != '\0')
    strcpy(serr, ctx->fixstars_load_err);
  return retc;
}
#endif

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
#ifndef SWE_UPSTREAM_COMPAT
  double xpo_p[6] = {0}, xsite[6];	/* read only with SEFLG_SPEED; set for LTO's -Wmaybe-uninitialized */
  /* the exact steps to the instants the observer is read at (bug 49) */
  double dm = tjd - (tjd - dt), dp = (tjd + dt) - tjd;
#endif
  int32 epheflag, iflgsave;
#ifndef SWE_UPSTREAM_COMPAT
  int32 ephe_used = 0;	/* the ephemeris that answered, 0: none consulted */
  AS_BOOL mosh_star = FALSE;	/* bug 109 */
  double xsb_star[6] = {0, 0, 0, 0, 0, 0}, asb_star[3] = {0, 0, 0};
#endif
  struct epsilon *oe = &ctx->oec2000;
  iflgsave = iflag;
  iflag |= SEFLG_SPEED; /* we need this in order to work correctly */
  if (serr != NULL)
    *serr = '\0';
  iflag = plaus_iflag(ctx, iflag, -1, tjd, serr);
  epheflag = iflag & SEFLG_EPHMASK;
#ifndef SWE_UPSTREAM_COMPAT
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL
      && swi_ephe_path_none(ctx)) {	/* bug 110 */
#else
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL) {
#endif
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_fixstar() or swe_fixstar_ut()");
  }
  if (ctx->last_epheflag != epheflag) {
#ifndef SWE_UPSTREAM_COMPAT
    ephe_switch_forget(ctx);	/* bug 103 */
    ctx->last_epheflag = epheflag;
#else
    free_planets(ctx);
    /* close and free ephemeris files */
    if (ctx->jpl_file_is_open) {
      swi_close_jpl_file(ctx);
      ctx->jpl_file_is_open = FALSE;
    }
    swi_close_ephe_files(ctx, KEEP_DENUM);
    ctx->last_epheflag = epheflag;
#endif
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
    /* ⚠️ This is the one place a star's answer depends on an ephemeris, and
     * it does not go through swe_calc(), so the strict guard at swe_calc()'s
     * exit never sees it. main_planet_bary() substitutes on its own when the
     * requested file is missing; the return flag of this function is the
     * caller's iflag, unchanged, so without this check a missing JPL file
     * gave a Swiss-based position labelled SEFLG_JPLEPH. */
    int32 iephe_used = epheflag;
#ifdef SWE_UPSTREAM_COMPAT
    if ((retc =  main_planet_bary(ctx, tjd - dt, SEI_EARTH, epheflag, iflag, NO_SAVE, &iephe_used, ctx->sp.fx_struct.xearth_dt, ctx->sp.fx_struct.xearth_dt, ctx->sp.fx_struct.xsun_dt, NULL, serr)) != OK) {
      return ERR;
    }
    if (swi_refuse_downgrade(ctx, epheflag, iephe_used, tjd, serr))
      return ERR;
#endif
#ifndef SWE_UPSTREAM_COMPAT
    ctx->moshmoon_used = FALSE;	/* bug 120 */
#endif
    if ((retc =  main_planet_bary(ctx, tjd, SEI_EARTH, epheflag, iflag, DO_SAVE, &iephe_used, ctx->sp.fx_struct.xearth, ctx->sp.fx_struct.xearth, ctx->sp.fx_struct.xsun, NULL, serr)) != OK) {
      return ERR;
    }
    if (swi_refuse_downgrade(ctx, epheflag, iephe_used, tjd, serr))
      return ERR;
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 120: and an Earth made from Moshier's Moon, the semo
     * file missing (a heliocentric star reads the Earth but uses only the
     * Sun) */
    if (!(iflag & SEFLG_HELCTR) && swi_refuse_moshmoon(ctx, tjd, serr))
      return ERR;
    ephe_used = iephe_used;
#endif
  }
  /************************************
   * observer: geocenter or topocenter
   ************************************/
  /* if topocentric position is wanted  */
  if (iflag & SEFLG_TOPOCTR) { 
#ifdef SWE_UPSTREAM_COMPAT
    if (swi_get_observer(ctx, tjd - dt, iflag | SEFLG_NONUT, NO_SAVE, xobs_dt, serr) != OK)
      return ERR;
#endif
    if (swi_get_observer(ctx, tjd, iflag | SEFLG_NONUT, NO_SAVE, xobs, serr) != OK)
      return ERR;
#ifndef SWE_UPSTREAM_COMPAT
    memcpy(xsite, xobs, sizeof(xsite));
#endif
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++) {
      xobs[i] = xobs[i] + ctx->sp.fx_struct.xearth[i];	
#ifdef SWE_UPSTREAM_COMPAT
      xobs_dt[i] = xobs_dt[i] + ctx->sp.fx_struct.xearth_dt[i];	
#endif
    }
  } else if (!(iflag & SEFLG_BARYCTR) && (!(iflag & SEFLG_HELCTR) || !(iflag & SEFLG_MOSEPH))) {
    /* barycentric position of geocenter */
    for (i = 0; i <= 5; i++) {
      xobs[i] = ctx->sp.fx_struct.xearth[i];
#ifdef SWE_UPSTREAM_COMPAT
      xobs_dt[i] = ctx->sp.fx_struct.xearth_dt[i];
#endif
    }
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 109: Moshier's Earth is heliocentric, and a star's
   * parallax and aberration want the barycentric observer: the parallax
   * was taken from the heliocentric Earth (9.2e-7 deg on Sirius without
   * aberration; none heliocentric, where the Sun's 0.005-0.01 AU from the
   * barycentre is the parallax), the aberration with its heliocentric
   * velocity, v_S short (3.0e-6 deg on every apparent star). The Sun's
   * barycentric place and velocity from Moshier's planets
   * (swi_mosh_sun_bary()) carry the observer there, and the aberration
   * rate's observer at -dm, dp takes the rate of that velocity
   * (swi_mosh_sun_accel()). */
  if (ephe_used == SEFLG_MOSEPH || ((iflag & SEFLG_HELCTR) && (iflag & SEFLG_MOSEPH))) {
    mosh_star = TRUE;
    swi_mosh_sun_bary(ctx, tjd, SWI_MOSH_SUN_FULL, xsb_star, NULL);
    if (!(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR)))
      for (i = 0; i <= 5; i++)
        xobs[i] += xsb_star[i];
  }
#endif
  /************************************
   * position and speed at tjd        *
   ************************************/
  /* for parallax */ 
  if ((iflag & SEFLG_HELCTR) && (iflag & SEFLG_MOSEPH)) {
#ifndef SWE_UPSTREAM_COMPAT
    xpo = xsb_star;	/* bug 109: the Sun's barycentric place */
#else
    xpo = NULL;		/* no parallax, if moshier and heliocentric */
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 61: the flag was masked to SEFLG_SPEED, so the Earth
     * was the geocenter on a topocentric star: the site's 4.3e-5 AU
     * missing from e, the site-dependent part of the deflection (3e-7 to
     * 9e-6 arcsec for stars 5-40 degrees from the Sun) lost. The site
     * this call computed above goes in; swi_deflect_light()'s ctx->topd
     * is the last planet call's, and this observer is not saved there. */
    deflect_light_site(ctx, x, 0, iflag & (SEFLG_SPEED | SEFLG_TOPOCTR), xsite);
#else
    swi_deflect_light(ctx, x, 0, iflag & SEFLG_SPEED);
#endif
  }
  /**********************************
   * 'annual' aberration of light   *
   * speed is incorrect !!!         *
   **********************************/
#ifndef SWE_UPSTREAM_COMPAT
  /* never heliocentric or barycentric: plaus_iflag() sets SEFLG_NOABERR */
  if ((iflag & SEFLG_TRUEPOS) == 0 && (iflag & SEFLG_NOABERR) == 0) {
    /* the observer at t - dm and t + dp (xpo is xobs here, xpo_dt xobs_dt),
     * read only for a speed the caller asked for */
    int32 spf = iflgsave & SEFLG_SPEED;
    if (spf && mosh_star)	/* bug 109: the Sun's motion over -dm, dp */
      swi_mosh_sun_accel(ctx, tjd, asb_star);	/* before the Earth's reads */
    if (spf && fixstar_obs_rate(ctx, tjd, dm, dp, epheflag, iflag,
                              ctx->sp.fx_struct.xearth, xpo_dt, xpo_p, serr) != OK)
      return ERR;
    if (spf && mosh_star)
      for (i = 0; i <= 2; i++) {
        xpo_dt[i] += xsb_star[i] - dm * xsb_star[i+3];
        xpo_dt[i+3] += xsb_star[i+3] - dm * asb_star[i];
        xpo_p[i] += xsb_star[i] + dp * xsb_star[i+3];
        xpo_p[i+3] += xsb_star[i+3] + dp * asb_star[i];
      }
    aberr_light_central(x, xpo, xpo_dt, xpo_p, dm, dp, spf);
  }
#else
  if ((iflag & SEFLG_TRUEPOS) == 0 && (iflag & SEFLG_NOABERR) == 0)
    swi_aberr_light_ex(x, xpo, xpo_dt, dt, iflag & SEFLG_SPEED);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* into the ecliptic of date, which turns (bug 33) */
    if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_J2000))
      swi_ecl_frame_speed(x, swi_obliquity_rate(ctx, tjd, iflag, !(iflag & SEFLG_NONUT)));
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 67: the flags used, as swe_calc() returns them: a
   * substituted ephemeris (swe_set_ephe_fallback) replaces the ephemeris
   * bits. The public star functions return this value, not the caller's. */
  if (ephe_used != 0 && ephe_used != epheflag)
    iflag = (iflag & ~SEFLG_EPHMASK) | ephe_used;
#endif
  /* if no ephemeris has been specified, do not return chosen ephemeris */
  if ((iflgsave & SEFLG_EPHMASK) == 0)
    iflag = iflag & ~SEFLG_DEFAULTEPH;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 61: SEFLG_SPEED was cleared from the returned flags even
   * when the caller asked for it; swi_fixstar_calc_from_record() and
   * swe_calc() keep it. Only what the caller did not ask for goes. (The
   * public wrappers return their own iflag and use this value for the
   * error test only; bug 67 makes them return it.) */
  if (!(iflgsave & SEFLG_SPEED))
    iflag = iflag & ~SEFLG_SPEED;
#else
  iflag = iflag & ~SEFLG_SPEED;
#endif
  return iflag;
}

#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 113: a star not found after the catalogue failed to load is
 * reported with the load's error first, then the search's */
static void fixstar_load_error(char *serr, int32 retc_load, const char *serr_load)
{
  char s[AS_MAXCH];
  if (serr == NULL || retc_load != ERR)
    return;
  snprintf(s, AS_MAXCH, "%s", serr);
  if (*serr_load == '\0')
    serr_load = "fixed stars file " SE_STARFILE " could not be read";
  snprintf(serr, AS_MAXCH, "%.170s; %.80s", serr_load, s);
}
#endif

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
	snprintf(serr, AS_MAXCH, "error, swe_fixstar(): invalid search string %.40s", sstar);
      return ERR;
    }
    strcpy(searchkey, sstar);
    len = (int) (strlen(sstar) - 1);
    searchkey[len] = '\0';
    /* The list is sorted by skey (strcmp), so every key with this prefix is
     * contiguous and the first of them is the lower bound -- the same
     * element the linear scan this replaces returned first. */
    {
      int lo = 0, hi = ndata;
      while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (strncmp(stardatabegp[mid].skey, sstar, len) < 0)
          lo = mid + 1;
        else
          hi = mid;
      }
      i = lo;
    }
    if (i < ndata && strncmp(stardatabegp[i].skey, sstar, len) == 0) {
      *stardata = stardatabegp[i];
      return OK;
    }
    if (serr != NULL)
      snprintf(serr, AS_MAXCH, "error, swe_fixstar(): star search string %.40s did not match", sstar);
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
    /* Upstream bug 106: a star file that failed to load left no table,
     * and bsearch() was handed its NULL; the answer, not found, is kept */
    stardatap = ndata <= 0 ? NULL : (struct fixed_star *) bsearch((void *) searchkey,
	       (void *) stardatabegp, (size_t) ndata,
	       sizeof (struct fixed_star),
	       fstar_node_compare);
    if (stardatap == NULL) {
      if (serr != NULL)
	snprintf(serr, AS_MAXCH, "error, swe_fixstar(): could not find star name %.40s", sstar);
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
  /* Radial velocity -3.31 km/s (SIMBAD, 2023ApJS..266...11B), where the record
   * carried +1: nothing reads a catalogue file for this name, so this line is
   * the only place it can be corrected. G31. */
  if (strncmp(star, "spica", 5) == 0 || strncmp(star, "Spica", 5) == 0) {
    strcpy(srecord, "Spica,alVir,ICRS,13,25,11.57937,-11,09,40.7501,-42.35,-30.67,-3.31,13.06,0.97,-10,3672");
    strcpy(sstar, "spica");
    return TRUE;
  /* Ayanamsha SE_SIDM_TRUE_REVATI */
  } else if (strstr(star, ",zePsc") != NULL || strncmp(star, "revati", 6) == 0 || strncmp(star, "Revati", 6) == 0) {
    strcpy(srecord, "Revati,zePsc,ICRS,01,13,43.88735,+07,34,31.2745,145,-55.69,15,18.76,5.187,06,174");
    strcpy(sstar, "revati");
    return TRUE;
  /* Ayanamsha SE_SIDM_TRUE_PUSHYA, and SE_SIDM_TRUE_SHEORAN, which uses
   * the same star.
   *
   * SHEORAN had its own branch below this one, guarded by
   * strstr(star, ",deCnc") != NULL -- a strict subset of this branch's
   * condition, so it was unreachable, and its body was byte-identical to
   * this one. It returned the right record only because it never ran.
   * (notes/REVIEW.md finding S1.) Removed; the mapping it documented is
   * recorded here instead. */
  } else if (strstr(star, ",deCnc") != NULL || strncmp(star, "pushya", 6) == 0 || strncmp(star, "Pushya", 6) == 0 ) {
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
static int32 swe_fixstar2_r_body(swe_ctx *ctx, char *star, double tjd, int32 iflag,
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
#ifndef SWE_UPSTREAM_COMPAT
  char serr_load[AS_MAXCH];
  int32 retc_load;
#endif
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 92: the lookup came before the first sync, so a thread
   * that had adopted one path looked its star up in that path's
   * catalogue after another thread had set a new one. Adopt it first;
   * and, bug 98, name the path as a first swe_calc() does. */
  fixstar_first_use(ctx);
#endif
  if (serr != NULL)
    *serr = '\0';
#ifdef TRACE
  swi_open_trace(serr);
  trace_swe_fixstar(1, star, tjd, iflag, xx, serr);
#endif /* TRACE */
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 113: the load's error was dropped, and a star then not
   * found in the table it did not fill was reported only as not found.
   * Kept for that report; a built-in star needs no catalogue. */
  *serr_load = '\0';
  retc_load = load_all_fixed_stars(ctx, serr_load);
#else
  load_all_fixed_stars(ctx, serr); // loads stars unless loaded with an earlier call of function
#endif
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
  if (retc == ERR) {
#ifndef SWE_UPSTREAM_COMPAT
    fixstar_load_error(serr, retc_load, serr_load);
#endif
    goto return_err;
  }
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 67: the flags used, not the caller's iflag (swe_calc()). */
  return retc;
#else
  return iflag;
#endif
  return_err:
  for (i = 0; i <= 5; i++)
    xx[i] = 0;
#ifdef TRACE
  trace_swe_fixstar(2, star, tjd, iflag, xx, serr);
#endif
  return retc;
}

int32 CALL_CONV swe_fixstar2_r(swe_ctx *ctx, char *star, double tjd, int32 iflag,
  double *xx, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_fixstar2_r_body(ctx, star, tjd, iflag, xx, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar2(char *star, double tjd, int32 iflag, 
  double *xx, char *serr)
{
  return swe_fixstar2_r(swi_default_ctx(), star, tjd, iflag, xx, serr);
}

static int32 swe_fixstar2_ut_r_body(swe_ctx *ctx, char *star, double tjd_ut, int32 iflag,
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

int32 CALL_CONV swe_fixstar2_ut_r(swe_ctx *ctx, char *star, double tjd_ut, int32 iflag,
  double *xx, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_fixstar2_ut_r_body(ctx, star, tjd_ut, iflag, xx, serr);
  swi_config_release(ctx);
  return ret;
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
static int32 swe_fixstar2_mag_r_body(swe_ctx *ctx, char *star, double *mag, char *serr)
{
  char sstar[SWI_STAR_LENGTH + 1];
  //static TLS char slast_stardata[AS_MAXCH];
  /* moved to ctx->sp.fs2mag (Phase 3c) */
  /* moved to ctx->sp.fs2mag (Phase 3c) */
  int retc;
  struct fixed_star stardata;
#ifndef SWE_UPSTREAM_COMPAT
  char serr_load[AS_MAXCH];
  int32 retc_load;
#endif
#ifndef SWE_UPSTREAM_COMPAT
  fixstar_first_use(ctx);	/* bugs 92, 98, see swe_fixstar2_r() */
#endif
  if (serr != NULL)
    *serr = '\0';
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 113: the load's error was dropped, and a star then not
   * found in the table it did not fill was reported only as not found.
   * Kept for that report; a built-in star needs no catalogue. */
  *serr_load = '\0';
  retc_load = load_all_fixed_stars(ctx, serr_load);
#else
  load_all_fixed_stars(ctx, serr); // loads stars unless loaded with an earlier call of function
#endif
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
  if (retc == ERR) {
#ifndef SWE_UPSTREAM_COMPAT
    fixstar_load_error(serr, retc_load, serr_load);
#endif
    goto return_err;
  }
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

int32 CALL_CONV swe_fixstar2_mag_r(swe_ctx *ctx, char *star, double *mag, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_fixstar2_mag_r_body(ctx, star, mag, serr);
  swi_config_release(ctx);
  return ret;
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
        /* ctx, not swi_default_ctx(): this reads seorbel.txt along the
         * ephemeris path and through the per-context line cache, so asking
         * the default context returned names from the wrong directory
         * whenever the caller was not the default context. Same shape as
         * init_leapsec()'s two sites. */
        swi_get_fict_name(ctx, ipl - SE_FICT_OFFSET, s);
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
#ifndef SWE_UPSTREAM_COMPAT
  ctx->topd.teval = SWI_TJD_NEVER;	/* bug 71 */
#else
  ctx->topd.teval = 0;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    ctx->savedat[i].tsave = SWI_TJD_NEVER;	/* bug 71 */
#else
    ctx->savedat[i].tsave = 0;
#endif
    ctx->savedat[i].iflgsave = -1;
  }
}

int swi_get_observer(swe_ctx *ctx, double tjd, int32 iflag, 
	AS_BOOL do_save, double *xobs, char *serr)
{
  int i;
  double sidt, delt, tjd_ut, eps, nut;
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
#ifndef SWE_UPSTREAM_COMPAT
  /* swe_deltat_ex_r() empties its serr first, and this runs in the middle
   * of a calculation: a topocentric request with SEFLG_SPEED, whose second
   * observer comes after the files are read, lost the call's notes ("using
   * Moshier eph. for moon", Mars at 1500 without semo_12, fallback allowed:
   * return 0x8102 and an empty serr). Its own message is appended instead. */
  {
    char sdt[AS_MAXCH];
    *sdt = '\0';
    delt = swe_deltat_ex_r(ctx, tjd, iflag, sdt);
    if (serr != NULL && *sdt != '\0' && strstr(serr, sdt) == NULL
        && strlen(serr) + strlen(sdt) < AS_MAXCH)
      strcat(serr, sdt);
  }
#else
  delt = swe_deltat_ex_r(ctx, tjd, iflag, serr);
#endif
  tjd_ut = tjd - delt;
#ifndef SWE_UPSTREAM_COMPAT
  /* The Earth turns about its TRUE pole of date, and the apparent sidereal
   * time is the angle it has turned through. So the observer is built on
   * the true equator and equinox of date, whatever frame the caller wants
   * its answer in: the SEFLG_NONUT bit says how the result is to be
   * expressed, not where the observer is. Upstream read it as the latter,
   * and since every caller passes it (the J2000 vector is what they add to
   * the barycentric Earth), the site was placed relative to the MEAN pole
   * -- 200-270 m from its place, 0.10-0.15" on the Moon (UPSTREAM-BUGS.md
   * 27, G37). The obliquity and nutation are taken at this instant rather
   * than from the caches, whose entry may be another instant's. */
  struct epsilon oectmp;
  struct nut nuttmp;
  double pm[9], pdm[9];
  AS_BOOL have_pm;
  calc_epsilon(ctx, tjd, iflag, &oectmp);
  if (swi_nutation(ctx, tjd, iflag, nuttmp.nutlo) == ERR)
    return ERR;
  nuttmp.tnut = tjd;
  nuttmp.snut = sin(nuttmp.nutlo[1]);
  nuttmp.cnut = cos(nuttmp.nutlo[1]);
  nut_matrix(&nuttmp, &oectmp);
  eps = oectmp.eps + nuttmp.nutlo[1];
  nut = nuttmp.nutlo[0];
  /* apparent sidereal time */
  sidt = swe_sidtime0_r(ctx, tjd_ut, eps * RADTODEG, nut * RADTODEG);
  sidt *= 15;	/* in degrees */
  /* Upstream bug 140: UT1 = TT - delta t is rounded to a double, 2^-31
   * day (40 us) apart at JD 2.4e6, so the site placed by it was a
   * staircase in delta t: steps of 8.5e-14 AU (13 mm at 47 N) every ulp /
   * (d delta t / dt), minutes to hours apart, and between them the
   * rotation's rate without delta t's, which the velocity carries (bug
   * 36). Against five-point differences of the positions the site's
   * velocity was off by d delta t / dt of itself (3.8e-8 in 1900), the
   * topocentric speed of an orbit 0.01 AU from the Earth by 1.8e-8 to
   * 4.7e-8 deg/day (2.8e-7 on de200 at 1700, a step inside the stencil),
   * on swe_calc_orbel() and seorbel.txt alike, and the topocentric Moon's
   * by 2.5e-7 (G72). The rounding is recovered exactly (Knuth's two-sum)
   * and the site turned by it at the rotation's rate. G126. */
  {
    double bb = tjd_ut - tjd, ut_err = (tjd - (tjd_ut - bb)) + (-delt - bb);
    sidt += ut_err * 360.0 * 1.00273781191135448;
  }
#else
  double nutlo[2];
  /* Keyed like every other reader of these caches (swi_check_ecliptic,
   * swi_check_nutation): the instant AND the flag key. On the instant
   * alone, a JPLHOR call could reuse an obliquity computed without the
   * Horizons offsets, or the reverse (2026-09-16 review, F10). */
  if (ctx->oec.teps == tjd && ctx->nut.tnut == tjd
      && ctx->oec.epsflag == SWI_EPS_KEY(iflag)
      && SWI_EPS_KEY(ctx->sp.nut.nutflag) == SWI_EPS_KEY(iflag)) {
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
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 36: the site turns at EARTH_ROT_SPEED per day of UT1, and
   * positions are functions of TT: per TT day it turns by that times
   * dUT1/dTT = 1 - d(delta t)/dTT. Far from now delta t changes by ~1800
   * s a century (-999), 5e-7 of the rotation, 2e-6 deg/day of the
   * topocentric Moon's speed; near now 3e-8. */
  /* Upstream bug 73: the site is placed by sidereal time, its angle from
   * the moving equinox of date, and turned at EARTH_ROT_SPEED, the rate of
   * the Earth's rotation angle (1.0027378119 turns a day, against inertial
   * space); swi_precess_speed() below adds the equinox's own motion, the
   * precession in right ascension, so the site's velocity had it twice.
   * And the true pole's turning was left out: the equation of the
   * equinoxes' rate here, the nutation matrix's rate below. The site's
   * velocity was off by 2-6 m/day, the topocentric Moon's speed by up to
   * 8.7e-7 deg/day (G72). Now sidereal time's rate: the rotation angle's,
   * per day of UT1, the precession in right ascension's from the
   * precession matrix and its rate, -(dM/dt M^T)[0][1] (what the
   * precession's rate takes off again below), and d(dpsi cos eps)/dt. */
  {
    double ddt = (swe_deltat_ex_r(ctx, tjd + 1, iflag, NULL)
                - swe_deltat_ex_r(ctx, tjd - 1, iflag, NULL)) / 2;
    double pra = 0;
    swi_nutation_rate(ctx, tjd, iflag, nuttmp.dnut);
    have_pm = swi_precess_matrix_rate(ctx, tjd, iflag, pm, pdm);
    if (have_pm) {
      for (i = 0; i <= 2; i++)
        pra += pdm[3 + i] * pm[i];
    } else {	/* the date's axes in J2000, rows of M */
      double ex[3] = {1, 0, 0}, eyp[3] = {0, 1, 0}, eym[3] = {0, 1, 0};
      swi_precess(ctx, ex, tjd, iflag, J_TO_J2000);
      swi_precess(ctx, eyp, tjd + 1, iflag, J_TO_J2000);
      swi_precess(ctx, eym, tjd - 1, iflag, J_TO_J2000);
      for (i = 0; i <= 2; i++)
        pra += (eyp[i] - eym[i]) / 2 * ex[i];
    }
    xobs[3] = TWOPI * 1.00273781191135448 * (1 - ddt) + pra
      + nuttmp.dnut[0] * cos(eps) - nut * sin(eps) * nuttmp.dnut[1];
  }
#else
  xobs[3] = EARTH_ROT_SPEED;		
#endif
  xobs[4] = xobs[5] = 0;
  swi_polcart_sp(xobs, xobs);
  /* to AUNIT */
  for (i = 0; i <= 5; i++)
    xobs[i] /= AUNIT;
#ifndef SWE_UPSTREAM_COMPAT
  /* true equator and equinox of date -> mean equator and equinox of date:
   * the transposed nutation matrix, as swi_nutate(..., TRUE) applies it,
   * on the position and on the velocity. Upstream bug 73: and the matrix's
   * rate, which this comment had as micrometres a day: the true pole turns
   * at w = deps' e1 + dpsi' (0, -sin eps, cos eps), metres a day on a
   * 6378 km vector, and the velocity gains -(N^T w) x x_mean, as in
   * swi_nutate(). The mean obliquity's part of w (bug 69) is 1e-6 m/day
   * here, and left out, as in the equation of the equinoxes' rate above. */
  {
    double x[6], w[3], wm[3];
    for (i = 0; i <= 2; i++) {
      x[i]   = xobs[0] * nuttmp.matrix[i][0] + xobs[1] * nuttmp.matrix[i][1] + xobs[2] * nuttmp.matrix[i][2];
      x[i+3] = xobs[3] * nuttmp.matrix[i][0] + xobs[4] * nuttmp.matrix[i][1] + xobs[5] * nuttmp.matrix[i][2];
    }
    w[0] = nuttmp.dnut[1];
    w[1] = -nuttmp.dnut[0] * sin(eps);
    w[2] = nuttmp.dnut[0] * cos(eps);
    for (i = 0; i <= 2; i++)
      wm[i] = -(w[0] * nuttmp.matrix[i][0] + w[1] * nuttmp.matrix[i][1]
                + w[2] * nuttmp.matrix[i][2]);
    x[3] += wm[1] * x[2] - wm[2] * x[1];
    x[4] += wm[2] * x[0] - wm[0] * x[2];
    x[5] += wm[0] * x[1] - wm[1] * x[0];
    for (i = 0; i <= 5; i++)
      xobs[i] = x[i];
  }
#else
  /* subtract nutation, set backward flag */
  if (!(iflag & SEFLG_NONUT)) {
    swi_coortrf2(xobs, xobs, -ctx->nut.snut, ctx->nut.cnut);
    /* speed of xobs is always required, namely for aberration!!! */
    /*if (iflag & SEFLG_SPEED)*/
      swi_coortrf2(xobs+3, xobs+3, -ctx->nut.snut, ctx->nut.cnut);
    swi_nutate(ctx, xobs, iflag | SEFLG_SPEED, TRUE);
  }
#endif
  /* precess to J2000 */
#ifndef SWE_UPSTREAM_COMPAT
  if (have_pm) {
    /* with the matrix and rate in hand: M^T x, swi_precess()'s to the bit,
     * and M^T v + dM^T x, swi_precess_speed()'s */
    double x[6];
    for (i = 0; i <= 2; i++) {
      x[i] = xobs[0] * pm[i] + xobs[1] * pm[i + 3] + xobs[2] * pm[i + 6];
      x[i+3] = xobs[3] * pm[i] + xobs[4] * pm[i + 3] + xobs[5] * pm[i + 6]
        + xobs[0] * pdm[i] + xobs[1] * pdm[i + 3] + xobs[2] * pdm[i + 6];
    }
    for (i = 0; i <= 5; i++)
      xobs[i] = x[i];
  } else {
    swi_precess(ctx, xobs, tjd, iflag, J_TO_J2000);
    swi_precess_speed(ctx, xobs, tjd, iflag, J_TO_J2000);
  }
#else
  swi_precess(ctx, xobs, tjd, iflag, J_TO_J2000);
  /*if (iflag & SEFLG_SPEED)*/
    swi_precess_speed(ctx, xobs, tjd, iflag, J_TO_J2000);
#endif
#ifndef SWE_UPSTREAM_COMPAT
  /* The Earth this is added to is ICRS (DE403 and later), and the callers
   * take the sum back to J2000 with the forward bias, so the site goes in
   * as ICRS too: J2000 -> ICRS, the 45 cm upstream neglected (0.3 mas on
   * the Moon; G37 measures the observer to 0.05 m). */
  if (swi_get_denum(ctx, SEI_EARTH, iflag) >= 403)
    swi_bias(ctx, xobs, tjd, iflag | SEFLG_SPEED, TRUE);
#else
  /* neglect frame bias (displacement of 45cm) */
  /* ... */
#endif
  /* save */
  if (do_save) {
    for (i = 0; i <= 5; i++)
      ctx->topd.xobs[i] = xobs[i];
    ctx->topd.teval = tjd;
    ctx->topd.tjd_ut = tjd_ut;	/* -> save area */
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 92: the observer was kept for the instant alone, but it
     * is placed by the delta t of the request's ephemeris (its tidal
     * acceleration) and, here, framed by its DE number (the bias above):
     * a topocentric request at the instant of one on another ephemeris
     * took that one's site. De200 after Swiss at 2000 Jan 1 12h UT, the
     * Moon 3.7e-8 deg off in right ascension, 3.9e-8 in azimuth, its
     * latitude speed 4.1e-7 deg/day; at TT 1600 Jan 1, where their delta t
     * differ, Swiss after de200 4.4" (0.00122 deg), Moshier after Swiss
     * 0.77", at 1000 5.4" (upstream, without the bias: 4.4" and 31"). It is
     * keyed on those flags too now (SWI_TOPD_KEY()), and
     * emptied by a path change, which can bring other files. */
    ctx->topd.iflgkey = SWI_TOPD_KEY(iflag);
#endif
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
static int32 swe_time_equ_r_body(swe_ctx *ctx, double tjd_ut, double *E, char *serr)
{
  int32 retval;
  double t, dt, x[6];
  double sidt = swe_sidtime_r(ctx, tjd_ut);
  int32 iflag = SEFLG_EQUATORIAL;
  iflag = plaus_iflag(ctx, iflag, -1, tjd_ut, serr);
#ifndef SWE_UPSTREAM_COMPAT
  if (swi_init_swed_if_start(ctx) == 1 && !(iflag & SEFLG_MOSEPH) && serr != NULL
      && swi_ephe_path_none(ctx)) {	/* bug 110 */
#else
  if (swi_init_swed_if_start(ctx) == 1 && !(iflag & SEFLG_MOSEPH) && serr != NULL) {
#endif
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_time_equ(), swe_lmt_to_lat() or swe_lat_to_lmt()");
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 132: the Sun was JPL's when THIS context had the JPL file
   * open, bug 103's old guess: after swe_set_jpl_file("de200.eph") a
   * second thread, a context made after it and a thread that had computed
   * before it answered from the Swiss files, E at J2000 1.19e-8 day (1.03
   * ms) off the setting thread's, 1.17e-7 day in 1800; after swe_close()
   * a JPLEPH calculation, which reopens the file by its kept name, moved
   * it the other way. It follows the configuration now, as swe_deltat()
   * and swe_get_ayanamsa_ut() do: swi_guess_ephe_flag(). */
  if (swi_guess_ephe_flag(ctx) & SEFLG_JPLEPH)
    iflag |= SEFLG_JPLEPH;
#else
  if (ctx->jpl_file_is_open)
    iflag |= SEFLG_JPLEPH;
#endif
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

int32 CALL_CONV swe_time_equ_r(swe_ctx *ctx, double tjd_ut, double *E, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_time_equ_r_body(ctx, tjd_ut, E, serr);
  swi_config_release(ctx);
  return ret;
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

static int32 swe_lat_to_lmt_r_body(swe_ctx *ctx, double tjd_lat, double geolon, double *tjd_lmt, char *serr)
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

int32 CALL_CONV swe_lat_to_lmt_r(swe_ctx *ctx, double tjd_lat, double geolon, double *tjd_lmt, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_lat_to_lmt_r_body(ctx, tjd_lat, geolon, tjd_lmt, serr);
  swi_config_release(ctx);
  return ret;
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
#ifndef SWE_UPSTREAM_COMPAT
  if (fpath == ctx->ephepath)
    swi_ephe_path_name_first_use(ctx);	/* bug 107 */
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 138: the automatic term was set for the file opened,
     * and a calculation opens it lazily, while swe_deltat()'s guess follows
     * the JPL file the configuration sets (bug 103): after the path,
     * swe_set_jpl_file("de200.eph") and the path again by another string,
     * which forgets the JPL DE number (F4), a JPL Sun left -23.8946 in
     * swe_get_tid_acc() and the next swe_deltat() used the moon file's
     * -25.936 (18.902020 s in 1800), the getter -25.936 after it. Resolved
     * for the guess now, as the setters resolve it; where the
     * configuration names this file it is the same term. */
    swi_set_tid_acc(ctx, 0, swi_guess_ephe_flag(ctx), 0, serr);
#else
    swi_set_tid_acc(ctx, 0, 0, ctx->jpldenum, serr);
#endif
  }
  /* The DE number is published by swe_set_jpl_file_r(), not here: this
   * runs for lazy opens on the calculation paths too (F4). */
  return retc;
}

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
	snprintf(serr, AS_MAXCH, "star file %.40s damaged at line %d", SE_STARFILE, fline);
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
    snprintf(serr, AS_MAXCH, "star  not found");
    if (strlen(serr) + strlen(star) < AS_MAXCH) {
      snprintf(serr, AS_MAXCH, "star %.230s not found", star);
    }
    return ERR;
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 113: with serr NULL a star not found fell through to
   * found: and answered as the file's last line */
  return ERR;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  double daya[2], rdist;
#else
  double daya, rdist;
#endif
  double x[6], xxsv[6], xobs[6], xobs_dt[6], *xpo = NULL, *xpo_dt = NULL;
  /* moved to ctx->sp.fx_record (Phase 3c) */
  double dt = PLAN_SPEED_INTV * 0.1;
#ifndef SWE_UPSTREAM_COMPAT
  double xpo_p[6] = {0}, xsite[6];	/* read only with SEFLG_SPEED; set for LTO's -Wmaybe-uninitialized */
  /* the exact steps to the instants the observer is read at (bug 49) */
  double dm = tjd - (tjd - dt), dp = (tjd + dt) - tjd;
#endif
  int32 epheflag, iflgsave;
#ifndef SWE_UPSTREAM_COMPAT
  int32 ephe_used = 0;	/* the ephemeris that answered, 0: none consulted */
  AS_BOOL mosh_star = FALSE;	/* bug 109 */
  double xsb_star[6] = {0, 0, 0, 0, 0, 0}, asb_star[3] = {0, 0, 0};
#endif
  // char s[AS_MAXCH];
  struct epsilon *oe = &ctx->oec2000;
  iflgsave = iflag;
  iflag |= SEFLG_SPEED; /* we need this in order to work correctly */
  if (serr != NULL)
    *serr = '\0';
  iflag = plaus_iflag(ctx, iflag, -1, tjd, serr);
  epheflag = iflag & SEFLG_EPHMASK;
#ifndef SWE_UPSTREAM_COMPAT
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL
      && swi_ephe_path_none(ctx)) {	/* bug 110 */
#else
  if (swi_init_swed_if_start(ctx) == 1 && !(epheflag & SEFLG_MOSEPH) && serr != NULL) {
#endif
    strcpy(serr, "Please call swe_set_ephe_path() or swe_set_jplfile() before calling swe_fixstar() or swe_fixstar_ut()");
  }
  if (ctx->last_epheflag != epheflag) {
#ifndef SWE_UPSTREAM_COMPAT
    ephe_switch_forget(ctx);	/* bug 103 */
    ctx->last_epheflag = epheflag;
#else
    free_planets(ctx);
    /* close and free ephemeris files */
    if (ctx->jpl_file_is_open) {
      swi_close_jpl_file(ctx);
      ctx->jpl_file_is_open = FALSE;
    }
    swi_close_ephe_files(ctx, KEEP_DENUM);
    ctx->last_epheflag = epheflag;
#endif
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
    /* Same check as in fixstar_calc_from_struct(), for the same reason: the
     * strict guard lives at swe_calc()'s exit and this path never reaches it. */
    int32 iephe_used = epheflag;
#ifdef SWE_UPSTREAM_COMPAT
    if ((retc =  main_planet_bary(ctx, tjd - dt, SEI_EARTH, epheflag, iflag, NO_SAVE, &iephe_used, ctx->sp.fx_record.xearth_dt, ctx->sp.fx_record.xearth_dt, ctx->sp.fx_record.xsun_dt, NULL, serr)) != OK) {
      return ERR;
    }
    if (swi_refuse_downgrade(ctx, epheflag, iephe_used, tjd, serr))
      return ERR;
#endif
#ifndef SWE_UPSTREAM_COMPAT
    ctx->moshmoon_used = FALSE;	/* bug 120 */
#endif
    if ((retc =  main_planet_bary(ctx, tjd, SEI_EARTH, epheflag, iflag, DO_SAVE, &iephe_used, ctx->sp.fx_record.xearth, ctx->sp.fx_record.xearth, ctx->sp.fx_record.xsun, NULL, serr)) != OK) {
      return ERR;
    }
    if (swi_refuse_downgrade(ctx, epheflag, iephe_used, tjd, serr))
      return ERR;
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 120: and an Earth made from Moshier's Moon, the semo
     * file missing (a heliocentric star reads the Earth but uses only the
     * Sun) */
    if (!(iflag & SEFLG_HELCTR) && swi_refuse_moshmoon(ctx, tjd, serr))
      return ERR;
    ephe_used = iephe_used;
#endif
  }
  /************************************
   * observer: geocenter or topocenter
   ************************************/
  /* if topocentric position is wanted  */
  if (iflag & SEFLG_TOPOCTR) { 
#ifdef SWE_UPSTREAM_COMPAT
    if (swi_get_observer(ctx, tjd - dt, iflag | SEFLG_NONUT, NO_SAVE, xobs_dt, serr) != OK)
      return ERR;
#endif
    if (swi_get_observer(ctx, tjd, iflag | SEFLG_NONUT, NO_SAVE, xobs, serr) != OK)
      return ERR;
#ifndef SWE_UPSTREAM_COMPAT
    memcpy(xsite, xobs, sizeof(xsite));
#endif
    /* barycentric position of observer */
    for (i = 0; i <= 5; i++) {
      xobs[i] = xobs[i] + ctx->sp.fx_record.xearth[i];	
#ifdef SWE_UPSTREAM_COMPAT
      xobs_dt[i] = xobs_dt[i] + ctx->sp.fx_record.xearth_dt[i];	
#endif
    }
  } else if (!(iflag & SEFLG_BARYCTR) && (!(iflag & SEFLG_HELCTR) || !(iflag & SEFLG_MOSEPH))) {
    /* barycentric position of geocenter */
    for (i = 0; i <= 5; i++) {
      xobs[i] = ctx->sp.fx_record.xearth[i];
#ifdef SWE_UPSTREAM_COMPAT
      xobs_dt[i] = ctx->sp.fx_record.xearth_dt[i];
#endif
    }
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 109: Moshier's Earth is heliocentric, and a star's
   * parallax and aberration want the barycentric observer: the parallax
   * was taken from the heliocentric Earth (9.2e-7 deg on Sirius without
   * aberration; none heliocentric, where the Sun's 0.005-0.01 AU from the
   * barycentre is the parallax), the aberration with its heliocentric
   * velocity, v_S short (3.0e-6 deg on every apparent star). The Sun's
   * barycentric place and velocity from Moshier's planets
   * (swi_mosh_sun_bary()) carry the observer there, and the aberration
   * rate's observer at -dm, dp takes the rate of that velocity
   * (swi_mosh_sun_accel()). */
  if (ephe_used == SEFLG_MOSEPH || ((iflag & SEFLG_HELCTR) && (iflag & SEFLG_MOSEPH))) {
    mosh_star = TRUE;
    swi_mosh_sun_bary(ctx, tjd, SWI_MOSH_SUN_FULL, xsb_star, NULL);
    if (!(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR)))
      for (i = 0; i <= 5; i++)
        xobs[i] += xsb_star[i];
  }
#endif
  /************************************
   * position and speed at tjd        *
   ************************************/
  /* for parallax */ 
  if ((iflag & SEFLG_HELCTR) && (iflag & SEFLG_MOSEPH)) {
#ifndef SWE_UPSTREAM_COMPAT
    xpo = xsb_star;	/* bug 109: the Sun's barycentric place */
#else
    xpo = NULL;		/* no parallax, if moshier and heliocentric */
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 61: the flag was masked to SEFLG_SPEED, so the Earth
     * was the geocenter on a topocentric star: the site's 4.3e-5 AU
     * missing from e, the site-dependent part of the deflection (3e-7 to
     * 9e-6 arcsec for stars 5-40 degrees from the Sun) lost. The site
     * this call computed above goes in; swi_deflect_light()'s ctx->topd
     * is the last planet call's, and this observer is not saved there. */
    deflect_light_site(ctx, x, 0, iflag & (SEFLG_SPEED | SEFLG_TOPOCTR), xsite);
#else
    swi_deflect_light(ctx, x, 0, iflag & SEFLG_SPEED);
#endif
  }
  /**********************************
   * 'annual' aberration of light   *
   * speed is incorrect !!!         *
   **********************************/
#ifndef SWE_UPSTREAM_COMPAT
  /* never heliocentric or barycentric: plaus_iflag() sets SEFLG_NOABERR */
  if ((iflag & SEFLG_TRUEPOS) == 0 && (iflag & SEFLG_NOABERR) == 0) {
    /* the observer at t - dm and t + dp (xpo is xobs here, xpo_dt xobs_dt),
     * read only for a speed the caller asked for */
    int32 spf = iflgsave & SEFLG_SPEED;
    if (spf && mosh_star)	/* bug 109: the Sun's motion over -dm, dp */
      swi_mosh_sun_accel(ctx, tjd, asb_star);	/* before the Earth's reads */
    if (spf && fixstar_obs_rate(ctx, tjd, dm, dp, epheflag, iflag,
                              ctx->sp.fx_record.xearth, xpo_dt, xpo_p, serr) != OK)
      return ERR;
    if (spf && mosh_star)
      for (i = 0; i <= 2; i++) {
        xpo_dt[i] += xsb_star[i] - dm * xsb_star[i+3];
        xpo_dt[i+3] += xsb_star[i+3] - dm * asb_star[i];
        xpo_p[i] += xsb_star[i] + dp * xsb_star[i+3];
        xpo_p[i+3] += xsb_star[i+3] + dp * asb_star[i];
      }
    aberr_light_central(x, xpo, xpo_dt, xpo_p, dm, dp, spf);
  }
#else
  if ((iflag & SEFLG_TRUEPOS) == 0 && (iflag & SEFLG_NOABERR) == 0)
    swi_aberr_light_ex(x, xpo, xpo_dt, dt, iflag & SEFLG_SPEED);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* into the ecliptic of date, which turns (bug 33) */
    if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_J2000))
      swi_ecl_frame_speed(x, swi_obliquity_rate(ctx, tjd, iflag, !(iflag & SEFLG_NONUT)));
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 49: the ayanamsa was taken off the longitude and not
       * its rate off the speed, so swe_fixstar()'s sidereal longitude speed
       * was the tropical one, 3.8e-5 deg/day too fast; swe_fixstar2() and
       * swe_calc() take both. */
      if (swi_get_ayanamsa_with_speed(ctx, tjd, iflag, daya, serr) == ERR)
        return ERR;
      x[0] -= daya[0] * DEGTORAD;
      x[3] -= daya[1] * DEGTORAD;
#else
      if (swi_get_ayanamsa_ex(ctx, tjd, iflag, &daya, serr) == ERR)
        return ERR;
      x[0] -= daya * DEGTORAD;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 67: the flags used, as swe_calc() returns them: a
   * substituted ephemeris (swe_set_ephe_fallback) replaces the ephemeris
   * bits. The public star functions return this value, not the caller's. */
  if (ephe_used != 0 && ephe_used != epheflag)
    iflag = (iflag & ~SEFLG_EPHMASK) | ephe_used;
#endif
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
static int32 swe_fixstar_r_body(swe_ctx *ctx, char *star, double tjd, int32 iflag,
  double *xx, char *serr)
{
  int i;
  char sstar[SWI_STAR_LENGTH + 1];
  /* moved to ctx->sp.fs1 (Phase 3c) */
  /* moved to ctx->sp.fs1 (Phase 3c) */
  char srecord[AS_MAXCH + 20], *sp;	/* 20 byte for SE_STARFILE */
  int retc;
#ifndef SWE_UPSTREAM_COMPAT
  fixstar_first_use(ctx);	/* bugs 92, 98, see swe_fixstar2_r() */
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 67: the flags used, not the caller's iflag (swe_calc()). */
  return retc;
#else
  return iflag;
#endif
  return_err:
  for (i = 0; i <= 5; i++)
    xx[i] = 0;
#ifdef TRACE
  trace_swe_fixstar(2, star, tjd, iflag, xx, serr);
#endif
  return retc;
}

int32 CALL_CONV swe_fixstar_r(swe_ctx *ctx, char *star, double tjd, int32 iflag,
  double *xx, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_fixstar_r_body(ctx, star, tjd, iflag, xx, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar(char *star, double tjd, int32 iflag, 
  double *xx, char *serr)
{
  return swe_fixstar_r(swi_default_ctx(), star, tjd, iflag, xx, serr);
}

static int32 swe_fixstar_ut_r_body(swe_ctx *ctx, char *star, double tjd_ut, int32 iflag,
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

int32 CALL_CONV swe_fixstar_ut_r(swe_ctx *ctx, char *star, double tjd_ut, int32 iflag,
  double *xx, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_fixstar_ut_r_body(ctx, star, tjd_ut, iflag, xx, serr);
  swi_config_release(ctx);
  return ret;
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
static int32 swe_fixstar_mag_r_body(swe_ctx *ctx, char *star, double *mag, char *serr)
{
  char sstar[SWI_STAR_LENGTH + 1];
  /* moved to ctx->sp.fs1mag (Phase 3c) */
  /* moved to ctx->sp.fs1mag (Phase 3c) */
  char srecord[AS_MAXCH + 20], *sp;	/* 20 byte for SE_STARFILE */
  struct fixed_star stardata;
  int retc;
  double dparams[20];
#ifndef SWE_UPSTREAM_COMPAT
  fixstar_first_use(ctx);	/* bugs 92, 98, see swe_fixstar2_r() */
#endif
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

int32 CALL_CONV swe_fixstar_mag_r(swe_ctx *ctx, char *star, double *mag, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_fixstar_mag_r_body(ctx, star, mag, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_fixstar_mag(char *star, double *mag, char *serr)
{
  return swe_fixstar_mag_r(swi_default_ctx(), star, mag, serr);
}


static int32 swe_calc_pctr_r_body(swe_ctx *ctx, double tjd, int32 ipl, int32 iplctr, int32 iflag, double *xxret, char *serr)
{
  double t = 0, dt = 0, daya[2], dtsave_for_defl = 0;
  double xx[6], xxctr[6], xxctr2[6], xx0[6], xxsv[24], xxsp[6], dx[6], xreturn[24];
#ifndef SWE_UPSTREAM_COMPAT
  double xctra[3] = {0, 0, 0}, xsunb[6] = {0, 0, 0, 0, 0, 0};
  AS_BOOL do_defl;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 77: SEFLG_TRUEPOS left t at 0, the date swi_bias() is
   * given below, which skips the bias before 1962 under
   * SEFLG_JPLHOR_APPROX and SEMOD_JPLHORA_3 */
  t = tjd;
  /* the Sun at tjd, barycentric, for the deflection seen from the centre
   * (below); no deflection of the Sun itself, nor seen from it */
  do_defl = !(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOGDEFL)
            && ipl != SE_SUN && iplctr != SE_SUN;
  if (do_defl && swe_calc_r(ctx, tjd, SE_SUN, iflag2, xsunb, serr) == ERR)
    return ERR;
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* the centre's acceleration at tjd for the aberration rate below:
     * from its velocities at t = tjd - d, read above, at tjd and at
     * tjd + h, h = 2^-10 day, the derivative at tjd of the parabola
     * through them, ((v(+h) - v0) d/h + (v0 - v(-d)) h/d) / (d + h),
     * off by d h a'''/6 */
    if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_NOABERR)) {
      double xp[6], tp = tjd + 1.0 / 1024, d = tjd - t, h;
      if (swe_calc_r(ctx, tp, iplctr, iflag2, xp, serr) == ERR)
        return ERR;
      h = tp - tjd;
      for (i = 0; i <= 2; i++)
        xctra[i] = ((xp[i+3] - xxctr[i+3]) * (d / h)
                    + (xxctr[i+3] - xxctr2[i+3]) * (h / d)) / (d + h);
    }
#endif
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 77: both bodies were read at t = tjd - dt rounded to a
     * double, up to half an ulp of the date (2.3e-10 day) from the
     * emission instant, so the position jumped by the body's motion over
     * that from one date to the next: Mercury from Mars 9.8e-8 deg/day in
     * a five-point difference over 2^-9 day, the Moon from Venus 1.8e-7,
     * against 4e-10 for true positions. The body is moved along its
     * velocity at tjd by the rounding, (tjd - t) - dt, as bug 63's; the
     * centre at t gives only its velocity. (Moshier has no barycentric
     * positions; this path refuses it.) */
    {
      double eps = (tjd - t) - dt;
      for (i = 0; i <= 2; i++)
        xx[i] += eps * xx0[i+3];
    }
    /* The light time above is solved along a straight line, xx0 - dt v,
     * and is off the fixed point tau = |X(tjd - tau) - O(tjd)| / c by
     * that line's departure from the orbit: 5e-9 deg on Mercury from
     * Pluto, 3.3e-10 from Mars. One Newton step on it from the body read
     * at tjd - dt, d tau = (|rho| / c - dt) / (1 + rho^.V / c), the body
     * moved along its velocity by -d tau (as bug 78's in
     * app_pos_etc_plan()): 6e-14 deg off after (G76). */
    {
      const double c = CLIGHT * 86400.0 / AUNIT;   /* AU per day */
      double rho[3], r, rv = 0, dtau;
      for (i = 0; i <= 2; i++)
        rho[i] = xx[i] - xxctr[i];
      r = sqrt(square_sum(rho));
      for (i = 0; i <= 2; i++)
        rv += rho[i] * xx[i+3];
      dtau = (r / c - dt) / (1 + rv / r / c);
      for (i = 0; i <= 2; i++)
        xx[i] -= dtau * xx[i+3];
      dt += dtau;
      dtsave_for_defl = dt;
    }
#endif
  }
  /*******************************
   * conversion to planetocenter     * 
   *******************************/
  if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR)) {
#ifndef SWE_UPSTREAM_COMPAT
    if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_TRUEPOS))
      lt_rate_exact(xx, xxctr, xxsp);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 77: this was swi_deflect_light(), which takes the
   * observer and the Sun from the save area: the Earth and the Sun at t
   * that the reads above left there, not the centre. The deflection was
   * the Earth's geometry applied to the centre's vector: 0.28" on Saturn
   * seen from Mercury, 0.135" on the Sun seen from Jupiter, which is not
   * deflected by itself at all. Now seen from the centre at tjd, with
   * the Sun at tjd; none for the Sun, nor seen from it. It reads no
   * saved observer, so the topocentric one the fork computed and saved
   * at the top for it (2026-09-16 review, F7) is gone. */
  if (do_defl)
    deflect_light_from(xx, dtsave_for_defl, iflag & SEFLG_SPEED, xxctr, xsunb, epheflag);
#else
  if (!(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOGDEFL))
    	/* SEFLG_NOGDEFL is on, if SEFLG_HELCTR or SEFLG_BARYCTR */
    swi_deflect_light(ctx, xx, dtsave_for_defl, iflag);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 77: this added the centre's velocity at tjd less that
     * at t whole, bug 34's first-order term with its radial part, which
     * is the distance's own and second order (bug 52): the distance rate
     * off by 1.5e-6 AU/day on Mercury from Mars, 5.5e-6 on the Moon from
     * Venus, 1.3e-4 on Saturn from Mercury. And that difference is tau
     * a(tjd - tau/2), not tau a(tjd): Saturn's longitude speed from
     * Mercury was off by 2.1e-6 deg/day, 2.0e-6 of it this (5.6e-8 on
     * Venus from the Moon). Now bug 52's exact term, add_aberr_rate(),
     * with tau a(tjd), a the centre's acceleration above (over tau either
     * side instead, 5.7e-9 on Saturn from Mercury). Only xobs2's velocity
     * is read. */
    if (iflag & SEFLG_SPEED) {
      double xobs2[6];
      for (i = 0; i <= 2; i++) {
        xobs2[i] = xxctr2[i];
        xobs2[i+3] = xxctr[i+3] - dt * xctra[i];
      }
      add_aberr_rate(xx, xxctr, xobs2);
    }
#else
    if (iflag & SEFLG_SPEED) {
      for (i = 3; i <= 5; i++) 
        xx[i] += xxctr[i] - xxctr2[i];
    }
#endif
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
  /* The obliquity and nutation used below are per-context caches keyed on
   * the epoch they were last computed for. Every other site that reads them
   * -- swe_calc_r, app_pos_etc_plan, swe_fixstar -- calls these two first;
   * this one did not, and relied on a side effect of the SE_ECL_NUT call at
   * the top of the function. Four swe_calc_r() calls run in between and each
   * re-keys the caches to its own epoch, so the transformation ended up
   * using whatever the last of them left. That made the answer depend on
   * what had been computed BEFORE the function was called: asking for Mars
   * from Jupiter at J2000 moved 1.9 arcsec if a position 100 days away had
   * been computed first, and 24.5 arcsec at 10000 days. */
#ifndef SWE_UPSTREAM_COMPAT
  swi_check_ecliptic(ctx, tjd, iflag);
  swi_check_nutation(ctx, tjd, iflag);
#endif
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
#ifndef SWE_UPSTREAM_COMPAT
  /* into the ecliptic of date, which turns (bug 33) */
  if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_J2000))
    swi_ecl_frame_speed(xx, swi_obliquity_rate(ctx, tjd, iflag, !(iflag & SEFLG_NONUT)));
#endif
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

int32 CALL_CONV swe_calc_pctr_r(swe_ctx *ctx, double tjd, int32 ipl, int32 iplctr, int32 iflag, double *xxret, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_calc_pctr_r_body(ctx, tjd, ipl, iplctr, iflag, xxret, serr);
  swi_config_release(ctx);
  return ret;
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
static double swe_solcross_r_body(swe_ctx *ctx, double x2cross, double jd_et, int flag, char *serr)
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

double CALL_CONV swe_solcross_r(swe_ctx *ctx, double x2cross, double jd_et, int flag, char *serr)
{
  double ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_solcross_r_body(ctx, x2cross, jd_et, flag, serr);
  swi_config_release(ctx);
  return ret;
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
static double swe_solcross_ut_r_body(swe_ctx *ctx, double x2cross, double jd_ut, int flag, char *serr)
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

double CALL_CONV swe_solcross_ut_r(swe_ctx *ctx, double x2cross, double jd_ut, int flag, char *serr)
{
  double ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_solcross_ut_r_body(ctx, x2cross, jd_ut, flag, serr);
  swi_config_release(ctx);
  return ret;
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
static double swe_mooncross_r_body(swe_ctx *ctx, double x2cross, double jd_et, int flag, char *serr)
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

double CALL_CONV swe_mooncross_r(swe_ctx *ctx, double x2cross, double jd_et, int flag, char *serr)
{
  double ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_mooncross_r_body(ctx, x2cross, jd_et, flag, serr);
  swi_config_release(ctx);
  return ret;
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
static double swe_mooncross_ut_r_body(swe_ctx *ctx, double x2cross, double jd_ut, int flag, char *serr)
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

double CALL_CONV swe_mooncross_ut_r(swe_ctx *ctx, double x2cross, double jd_ut, int flag, char *serr)
{
  double ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_mooncross_ut_r_body(ctx, x2cross, jd_ut, flag, serr);
  swi_config_release(ctx);
  return ret;
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
static double swe_mooncross_node_r_body(swe_ctx *ctx, double jd_et, int flag, double *xlon, double *xla, char *serr)
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

double CALL_CONV swe_mooncross_node_r(swe_ctx *ctx, double jd_et, int flag, double *xlon, double *xla, char *serr)
{
  double ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_mooncross_node_r_body(ctx, jd_et, flag, xlon, xla, serr);
  swi_config_release(ctx);
  return ret;
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
static double swe_mooncross_node_ut_r_body(swe_ctx *ctx, double jd_ut, int flag, double *xlon, double *xla, char *serr)
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

double CALL_CONV swe_mooncross_node_ut_r(swe_ctx *ctx, double jd_ut, int flag, double *xlon, double *xla, char *serr)
{
  double ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_mooncross_node_ut_r_body(ctx, jd_ut, flag, xlon, xla, serr);
  swi_config_release(ctx);
  return ret;
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
static int32 swe_helio_cross_r_body(swe_ctx *ctx, int ipl, double x2cross, double jd_et, int iflag, int dir, double *jd_cross, char *serr)
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
    if (serr != NULL) snprintf(serr, AS_MAXCH, "swe_helio_cross: not possible for object %d = %.190s", ipl, snam);
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

int32 CALL_CONV swe_helio_cross_r(swe_ctx *ctx, int ipl, double x2cross, double jd_et, int iflag, int dir, double *jd_cross, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_helio_cross_r_body(ctx, ipl, x2cross, jd_et, iflag, dir, jd_cross, serr);
  swi_config_release(ctx);
  return ret;
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
static int32 swe_helio_cross_ut_r_body(swe_ctx *ctx, int ipl, double x2cross, double jd_ut, int iflag, int dir, double *jd_cross, char *serr)
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
    if (serr != NULL) snprintf(serr, AS_MAXCH, "swe_helio_cross: not possible for object %d = %.190s", ipl, snam);
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

int32 CALL_CONV swe_helio_cross_ut_r(swe_ctx *ctx, int ipl, double x2cross, double jd_ut, int iflag, int dir, double *jd_cross, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_helio_cross_ut_r_body(ctx, ipl, x2cross, jd_ut, iflag, dir, jd_cross, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_helio_cross_ut(int ipl, double x2cross, double jd_ut, int iflag, int dir, double *jd_cross, char *serr)
{
  return swe_helio_cross_ut_r(swi_default_ctx(), ipl, x2cross, jd_ut, iflag, dir, jd_cross, serr);
}
