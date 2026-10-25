/************************************************************
   definitions and constants SWISSEPH

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

/*
 * move over from swephexp.h
 */

/* Fork version. Upstream is 2.10.03; the "-ts.N" suffix marks the
 * thread-safe fork, so swe_version() cannot report a number that
 * implies upstream behaviour this library no longer has. */
#define SE_VERSION      "2.10.03-ts.20" 

#define J2000           2451545.0  	/* 2000 January 1.5 */
#define B1950           2433282.42345905  	/* 1950 January 0.923 */
#define J1900           2415020.0  	/* 1900 January 0.5 */
#define B1850           2396758.2035810  	/* 1850 January 16:53 */

#define MPC_CERES       1
#define MPC_PALLAS      2
#define MPC_JUNO        3
#define MPC_VESTA       4
#define MPC_CHIRON      2060
#define MPC_PHOLUS      5145

#define SE_NAME_SUN             "Sun"
#define SE_NAME_MOON            "Moon"
#define SE_NAME_MERCURY         "Mercury"
#define SE_NAME_VENUS           "Venus"
#define SE_NAME_MARS            "Mars"
#define SE_NAME_JUPITER         "Jupiter"
#define SE_NAME_SATURN          "Saturn"
#define SE_NAME_URANUS          "Uranus"
#define SE_NAME_NEPTUNE         "Neptune"
#define SE_NAME_PLUTO           "Pluto"
#define SE_NAME_MEAN_NODE       "mean Node"
#define SE_NAME_TRUE_NODE       "true Node"
#define SE_NAME_MEAN_APOG       "mean Apogee"
#define SE_NAME_OSCU_APOG       "osc. Apogee"
#define SE_NAME_INTP_APOG       "intp. Apogee"
#define SE_NAME_INTP_PERG       "intp. Perigee"
#define SE_NAME_EARTH           "Earth"
#define SE_NAME_CERES           "Ceres"
#define SE_NAME_PALLAS          "Pallas"
#define SE_NAME_JUNO            "Juno"
#define SE_NAME_VESTA           "Vesta"
#define SE_NAME_CHIRON          "Chiron"
#define SE_NAME_PHOLUS          "Pholus"

 
#define SE_NAME_CUPIDO          "Cupido"
#define SE_NAME_HADES           "Hades"
#define SE_NAME_ZEUS            "Zeus"
#define SE_NAME_KRONOS          "Kronos"
#define SE_NAME_APOLLON         "Apollon"
#define SE_NAME_ADMETOS         "Admetos"
#define SE_NAME_VULKANUS        "Vulkanus"
#define SE_NAME_POSEIDON        "Poseidon"
#define SE_NAME_ISIS            "Isis"
#define SE_NAME_NIBIRU          "Nibiru"
#define SE_NAME_HARRINGTON      "Harrington"
#define SE_NAME_NEPTUNE_LEVERRIER       "Leverrier"
#define SE_NAME_NEPTUNE_ADAMS   "Adams"
#define SE_NAME_PLUTO_LOWELL    "Lowell"
#define SE_NAME_PLUTO_PICKERING "Pickering"
#define SE_NAME_VULCAN          "Vulcan"
#define SE_NAME_WHITE_MOON      "White Moon"

/*
 * earlier content
 */

#define PI              M_PI	/* 3.14159265358979323846, math.h */
#define TWOPI           (2.0 * PI)

#define ENDMARK         -99

#define SEI_EPSILON     -2
#define SEI_NUTATION    -1
#define SEI_EMB		0	
#define SEI_EARTH	0	
#define SEI_SUN  	0	
#define	SEI_MOON	1	
#define	SEI_MERCURY	2	
#define	SEI_VENUS	3	
#define	SEI_MARS	4	
#define	SEI_JUPITER	5	
#define	SEI_SATURN	6	
#define	SEI_URANUS	7	
#define	SEI_NEPTUNE	8	
#define	SEI_PLUTO	9	
#define	SEI_SUNBARY	10	/* barycentric sun */
#define	SEI_ANYBODY	11	/* any asteroid */
#define	SEI_CHIRON	12	
#define	SEI_PHOLUS	13	
#define	SEI_CERES	14	
#define	SEI_PALLAS	15	
#define	SEI_JUNO	16	
#define	SEI_VESTA	17	

#define SEI_NPLANETS    18

#define SEI_MEAN_NODE   0
#define SEI_TRUE_NODE   1
#define SEI_MEAN_APOG   2
#define SEI_OSCU_APOG   3
#define SEI_INTP_APOG   4
#define SEI_INTP_PERG   5

#define SEI_NNODE_ETC    6

#define SEI_FLG_HELIO   1
#define SEI_FLG_ROTATE  2
#define SEI_FLG_ELLIPSE 4
#define SEI_FLG_EMBHEL  8   	/* TRUE, if heliocentric earth is given
				 * instead of barycentric sun 
				 * i.e. bary sun is computed from 
				 * barycentric and heliocentric earth */

#define SEI_FILE_PLANET	  0
#define SEI_FILE_MOON	  1 
#define SEI_FILE_MAIN_AST 2
#define SEI_FILE_ANY_AST  3
#define SEI_FILE_FIXSTAR  4
#define SEI_FILE_PLMOON   5

#if 0
#define SEI_FILE_TEST_ENDIAN     (97L * 65536L + 98L * 256L + 99L) /*abc*/
#endif
#define SEI_FILE_TEST_ENDIAN     (0x616263L) 	/* abc*/ 
#define SEI_FILE_BIGENDIAN	0
#define SEI_FILE_NOREORD	0
#define SEI_FILE_LITENDIAN	1
#define SEI_FILE_REORD  	2

#define SEI_FILE_NMAXPLAN	50
/* Largest planet count a .se1 file header may declare. Distinct from
 * SEI_FILE_NMAXPLAN, which is the capacity of fdp->ipl[]; the static_assert
 * at the check site in sweph.c ties the two together. */
#define SEI_FILE_MAXPLAN	20
#define SEI_FILE_EFPOSBEGIN      500

#define SE_FILE_SUFFIX	"se1"

#define SEI_NEPHFILES   7
#define SEI_CURR_FPOS   -1
#define SEI_NMODELS 8

/* Forward declaration of the ephemeris context.
 *
 * The full definition is ~700 lines below, but the swi_* prototypes between
 * here and there already need to name the type, so declare it up front. An
 * incomplete type is all a prototype requires. See notes/PHASE3-API.md. */
struct swe_ctx;
typedef struct swe_ctx swe_ctx;

#define SEI_ECL_GEOALT_MAX   25000.0
#define SEI_ECL_GEOALT_MIN   (-500.0)

/* Chiron's orbit becomes chaotic 
 * before 720 AD and after 4606 AD, because of close encounters
 * with Saturn. Accepting a maximum error of 5 degrees, 
 * the ephemeris is good between the following dates:
 */
/*#define CHIRON_START    1958470.5  	* 1.1.650 old limit until v. 2.00 */
#define CHIRON_START    1967601.5  	/* 1.1.675 */
#define CHIRON_END      3419437.5  	/* 1.1.4650 */

/* Pholus's orbit is unstable as well, because he sometimes
 * approaches Saturn.
 * Accepting a maximum error of 5 degrees,
 * the ephemeris is good after the following date:
 */
/* #define PHOLUS_START    314845.5  	* 1.1.-3850  old limit until v. 2.00 */
#define PHOLUS_START    640648.5	/* 1.1.-2958 jul */
#define PHOLUS_END      4390617.5  	/* 1.1.7309 */

#define MOSHPLEPH_START	 625000.5
#define MOSHPLEPH_END  	2818000.5
#define MOSHLUEPH_START	 625000.5
#define MOSHLUEPH_END  	2818000.5
/*#define MOSHNDEPH_START	-254900.5 */	/* 14 Feb -5410 00:00 ET jul.cal.*/
/*#define MOSHNDEPH_END  	3697000.5 */	/* 11 Dec 5409 00:00 ET, greg. cal */
#define MOSHNDEPH_START	-3100015.5	/* 15 Aug -13200 00:00 ET jul.cal.*/
#define MOSHNDEPH_END  	8000016.5       /* 15 Mar 17191 00:00 ET, greg. cal */
/*
#define MOSHPLEPH_START	 -225000.5
#define MOSHPLEPH_END  	3600000.5
#define MOSHLUEPH_START	 -225000.5
#define MOSHLUEPH_END  	3600000.5
*/
#define JPL_DE431_START -3027215.5 
#define JPL_DE431_END    7930192.5

#if FALSE	/*	Alois commented out, not used anywhere  */
#define JPLEPH_START	 625307.5	/* about -3000 (DE406) */
#define JPLEPH_END	2816848.5	/* about  3000 (DE406) */
#define SWIEPH_START	 625614.927151
#define SWIEPH_END	2813641.5
#define ALLEPH_START	MOSHPLEPH_START
#define ALLEPH_END	MOSHPLEPH_END
#define BEG_YEAR       (-3000)
#define END_YEAR       3000
#endif

#define MAXORD          40

#define NCTIES         6.0     /* number of centuries per eph. file */

#define OK (0)
#define ERR (-1)
#define NOT_AVAILABLE (-2)
#define BEYOND_EPH_LIMITS (-3)

#define J_TO_J2000   	1
#define J2000_TO_J   	-1

/* we always use Astronomical Almanac constants, if available */
#define MOON_MEAN_DIST  384400000.0		/* in m, AA 1996, F2 */
#define MOON_MEAN_INCL  5.1453964		/* AA 1996, D2 */
#define MOON_MEAN_ECC   0.054900489		/* AA 1996, F2 */
/* #define SUN_EARTH_MRAT  328900.561400           Su/(Ea+Mo) AA 2006 K7 */
#define SUN_EARTH_MRAT  332946.050895           /* Su / (Ea only) AA 2006 K7 */   
#define EARTH_MOON_MRAT (1 / 0.0123000383)	/* AA 2006, K7 */
#if 0
#define EARTH_MOON_MRAT 81.30056907419062	/* de431 */
#endif
#if 0
#define EARTH_MOON_MRAT 81.30056		/* de406 */
#endif
//#define AUNIT       	1.49597870691e+11  	/* au in meters, AA 2006 K6 */
#define AUNIT       	1.49597870700e+11  	/* au in meters, DE431 */
#define CLIGHT       	2.99792458e+8   	/* m/s, AA 1996 K6 / DE431 */
#if 0
#define HELGRAVCONST    1.32712438e+20		/* G * M(sun), m^3/sec^2, AA 1996 K6 */
#endif
#define HELGRAVCONST    1.32712440017987e+20	/* G * M(sun), m^3/sec^2, AA 2006 K6 */
#define GEOGCONST       3.98600448e+14 		/* G * M(earth) m^3/sec^2, AA 1996 K6 */
#define KGAUSS		0.01720209895		/* Gaussian gravitational constant K6 */
/* Earth's own GM, Earth only (no Moon), in the same units as KGAUSS:
 * sqrt(GM_earth) in AU^(3/2)/day. Same value as KGAUSS_GEO in swemplan.c,
 * which keeps its own define for the fictitious bodies; this copy exists
 * for the caller-supplied-elements entry point, whose centre-Earth mass
 * must not depend on that file. */
#define KGAUSS_EARTH	0.0000298122353216	/* Earth only */
#define SUN_RADIUS      (959.63 / 3600 * DEGTORAD)  /*  Meeus germ. p 391 */
#define EARTH_RADIUS	6378136.6		/* AA 2006 K6 */
//#define EARTH_OBLATENESS (1.0/ 298.257223563)	/* AA 1998 K13 */
#define EARTH_OBLATENESS (1.0/ 298.25642)	/* AA 2006 K6 */
#define EARTH_ROT_SPEED (7.2921151467e-5 * 86400) /* in rad/day, expl. suppl., p 162 */

#define LIGHTTIME_AUNIT  (499.0047838362/3600.0/24.0) 	/* 8.3167 minutes (days) */
#define PARSEC_TO_AUNIT  206264.8062471         /* 648000/PI, according to IAU Resolution B2, 2016 */

/* node of ecliptic measured on ecliptic 2000 */
#define SSY_PLANE_NODE_E2000    (107.582569 * DEGTORAD)
/* node of ecliptic measured on solar system rotation plane */
#define SSY_PLANE_NODE          (107.58883388 * DEGTORAD)
/* inclination of ecliptic against solar system rotation plane */
#define SSY_PLANE_INCL          (1.578701 * DEGTORAD)

#define KM_S_TO_AU_CTY	 21.095			/* km/s to AU/century */
#define MOON_SPEED_INTV  0.00005 		/* 4.32 seconds (in days) */
#define PLAN_SPEED_INTV  0.0001 	        /* 8.64 seconds (in days) */
#define MEAN_NODE_SPEED_INTV  0.001		
#define NODE_CALC_INTV  0.0001		
#define NODE_CALC_INTV_MOSH   0.1		
#define NUT_SPEED_INTV   0.0001
#define DEFL_SPEED_INTV  0.0000005
#ifndef SWE_UPSTREAM_COMPAT
#define INTP_APS_SPEED_INTV  0.00390625	/* 2^-8 day, interpolated apsides (bug 45) */
#define NODE_SPEED3_INTV  0.001953125	/* 2^-9 day, SPEED3 of the true node and osculating apogee (bug 54) */
#endif

#define SE_LAPSE_RATE        0.0065  /* deg K / m, for refraction */

/* swi_gen_t for the config-tracking fields in swe_ctx, and SWI_INLINE for
 * the two helpers immediately below. */
#include "swethread.h"

/* Functions, not macros (notes/C17_PERFORMANCE.md A3/A4).
 *
 * The macro forms were `(x[0]*x[0]+...)` with no parentheses around x, so
 * the argument had to be a bare identifier: square_sum(a+1) would have
 * parsed as a + 1[0]. Call sites were already working around it --
 * swecl.c:5489 and sweph.c:5673 write square_sum((xpos[i]+3)) with an
 * extra pair of parens for exactly this reason.
 *
 * They were also independently redefined, byte-identical, in swevents.c
 * and swetest.c, both of which already include this header.
 *
 * The argument order is unchanged, so the arithmetic is bit-identical;
 * -O2 inlines these to the same code the macro produced. */
SWI_INLINE double square_sum(const double *x)
{
  return x[0]*x[0] + x[1]*x[1] + x[2]*x[2];
}

SWI_INLINE double dot_prod(const double *x, const double *y)
{
  return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];
}

#define PNOINT2JPL {J_EARTH, J_MOON, J_MERCURY, J_VENUS, J_MARS, J_JUPITER, J_SATURN, J_URANUS, J_NEPTUNE, J_PLUTO, J_SUN, }

/* planetary radii in meters */
#define NDIAM  (SE_VESTA + 1)
static const double pla_diam[NDIAM] = {1392000000.0, /* Sun */
                           3475000.0, /* Moon */
                           2439400.0 * 2, /* Mercury */
                           6051800.0 * 2, /* Venus */
                           3389500.0 * 2, /* Mars */
                          69911000.0 * 2, /* Jupiter */
                          58232000.0 * 2, /* Saturn */
                          25362000.0 * 2, /* Uranus */
                          24622000.0 * 2, /* Neptune */
                           1188300.0 * 2, /* Pluto */
                           0, 0, 0, 0,    /* nodes and apogees */
                           6371008.4 * 2, /* Earth */
                            271370.0, /* Chiron */
                            290000.0, /* Pholus */
                            939400.0, /* Ceres */
                            545000.0, /* Pallas */
                            246596.0, /* Juno */
                            525400.0, /* Vesta */
                        };


/* Ayanamsas 
 * For each ayanamsa, there are the following values:
 * t0       epoch of ayanamsa, TDT (can be ET or UT)
 * ayan_t0  ayanamsa value at epoch
 * t0_is_UT true, if t0 is UT
 * prec_offset is the precession model for which the ayanamsha
 *          has to be corrected by adding/subtracting a constant offset. 
 *          0, if no correction is needed
 *          -1, if correction is unclear or has not been investigated
 *              and therefore is not applied
 */
struct aya_init {double t0;
                 double ayan_t0; 
		 AS_BOOL t0_is_UT;
		 int prec_offset;};
static const struct aya_init ayanamsa[SE_NSIDM_PREDEF] = {
/* 0: Fagan/Bradley (Default) 
     "The American Sidereal Ephemeris, 1976-2000" (Astro Computing Services, 1981)
     states on S.V.P. ("Synetic Vernal Point"):
     "The S.V.P. is the Sidereal longitude of the Vernal Equinox (the
     Tropical zero-point) in the Fagan-Bradley school of Western Sidereal
     astrology. It was determined empirically, its mean value being defined
     as 335°57'28".64 for the epoch 1950.0."
     Fagan/Firebrace, "Primer of Sidereal Astrology", p. 13:
     "It was during 1957 that Garth Allen .... experimenting ... But when
     progressed for the dates of the calamities, all were found by him to be 
     slightly out, the mean error being equivalent to an increase of 0°06'05"
     in the then-adopted sidereal longitude of the vernal point, determined
     from Spica in 29 Virgo (i.e. 29°06'05" Virgo; D.K.), and the proper motion
     having been allowed for. In short, for the epoch 1950.0 he proposed as the 
     mean longitude of the vernal point 335°57'28.64", proper motion being 
     disregarded."
     If "1950.0" means the standard epoch B1950 = JD 2433282.423, and based 
     on the then-used precession model of Newcomb, this ayanamsha leads to 
     a true position of 29°06'05.965" Virgo, based on Hipparcos position of 
     the star. */
{2433282.42346, 24.042044444, FALSE, SEMOD_PREC_NEWCOMB}, // 0: Fagan/Bradley
/*************************/
/* 1: Standard Lahiri 
     according to program NOVA by Robert Hand: 
     {J1900, 360 - 337.53953},   
     This corresponds to an ayanamsha 22°27'37.69 as given in
     Indian Ephemeris and Nautical Almanac" 1965, p. 459.
     Note, however, this value should only with a precession formula 
     where T is measured in tropical centuries. Swiss Ephemeris always
     uses Julian centuries.
     The following definition is according to:
     Calendar Reform Committee 1956; the subtracted value is nutation:
     {2435553.5, 23.25 - 0.00464207, FALSE}, 
     Lahiri (derived from: Indian Astronomical Ephemeris 1989, p. 556;
     the subtracted value is nutation, according to Wahr 1980) */
{2435553.5, 23.250182778 - 0.004658035, FALSE, SEMOD_PREC_IAU_1976}, // 1: Lahiri
/*************************/
/* 2: Robert DeLuce (Constellational Astrology ... p. 5; 
     birth of Jesus assumed on 1 Jan 1 BC (= 0) jul.,
     based on Newcomb precession. 
     {J1900, 360 - 333.58695, FALSE, 0}, 
     Ayanamsha was corrected with SE 2.09 as follows:
     Started at zero ayanamsha epoch with value 0 and 
     run with standard precession.
     This makes a difference of 22" compared with previous version: */
{1721057.5, 0, TRUE, 0}, // 2: DeLuce
/*************************/
/* 3: B.V. Raman (Robert Hand) 
     See B.V. Raman, "Hindu Predictive Astrology" (1938, Introduction), 
     pp. 279, 287.
     This ayanamsha is apparently not based on a valid precession theory (e.g.
     Newcomb). We cannot reproduce precisely the ayanamsha values on p. 287. */
{J1900, 360 - 338.98556, FALSE, SEMOD_PREC_NEWCOMB}, // 3: Raman
/*************************/
/* 4: Usha/Shashi (Robert Hand) 
     Usha and Shashi, "Hindu Astrological Calculations" (1978, 
     Sagar Publications, New Delhi).
     We do not have this book. */
{J1900, 360 - 341.33904, FALSE, -1}, // 4: Usha/Shashi
/*************************/
/* 5: Krishnamurti (Robert Hand)
     K.S. Krishnamurti, "Reader 1", pp. 55-59.
     Autor does not give precise information. Zero ayanamsha year is said to 
     be 291 CE, and there is an ayanamsha table with arc min precision for
     1840 to 2000 on p. 58.
     This ayanamsha reproduces the table quite well, if 1 Jan of each year
     is taken. (Turn off Newcomb precession in order to verify.)
     However, D. Senthilathiban believes the values are given for the date
     of sidereal Aries ingress of each year. ("Study of KP Ayanamsa with
     Modern Precession Theories", pp. 126f. */
{J1900, 360 - 337.636111, FALSE, SEMOD_PREC_NEWCOMB}, // 5: Krishnamurti
/*************************/
/* 6: Djwhal Khool (Graham Dawson), 
     "Channeled" information: Aquarius ingress of VP on 1 July 2117
     See Philipp Lindsay, “The Beginning of the Age of Aquarius: 2,117 A.D.”,
     http://esotericastrologer.org/newsletters/the-age-of-aquarius-ray-and-zodiac-cycles/ */
/*************************/
{J1900, 360 - 333.0369024, FALSE, 0}, // 6: Djwhal Khool
/* 7: Shri Yukteshwar; (David Cochrane) 
     This ayanamsha seems to be wrong.
     Swami Sri Yukteswar, "The Holy Science", 1920 (1949, 1957 and 1977, 
     partly revised), Yogoda Satsanga Society of India.
     Ayanamsha on the spring equinox 1893 was 20°54'36" (1894 according to 
     the revised edition of 1977) At the same time he believed that this was the
     distance of the spring equinox from the star Revati, which he put at the
     initial point of Aries.  Unfortunately, this is wrong, because on that date
     Revati was actually 18°23' away from the vernal point. The error is
     explained from the fact that Yukteshwar used the zero ayanamsha year 499 CE
     and an inaccurate Suryasiddhantic precession rate of 360°/24'000 years = 54
     arcsec/year. It is obvious that Yukteshwar actually intended an ayanamsha
     that starts at the star Revati.  */
{J1900, 360 - 338.917778, FALSE, -1},  // 7: Shri Yukteshwar
//{2412543.5, 20.91, TRUE, -1},        // 7: Shri Yukteshwar; (Holy Science, p. xx) 
/*************************/
/* 8: J.N. Bhasin; (David Cochrane) 
     We don't have any sources or detailed information about this ayanamsha. */
{J1900, 360 - 338.634444, FALSE, -1}, // 8: Bhasin
/*************************/
/* 14 Sept. 2018: the following three ayanamshas have been wrong for
   many years */
/* 9 - 11: Babylonian, Kugler */
{1684532.5, -5.66667, TRUE, -1},     //  9: Babylonian, Kugler 1
{1684532.5, -4.26667, TRUE, -1},     // 10: Babylonian, Kugler 2
{1684532.5, -3.41667, TRUE, -1},     // 11: Babylonian, Kugler 3 
/*************************/
/* 12: Babylonian, Huber 
      P. Huber, "Über den Nullpunkt der babylonischen Ekliptik", in: Centaurus
      1958, 5, p. 192-208. 
      This ayanamsha had a wrong initial value until 14 Sept. 2018. */
{1684532.5, -4.46667, TRUE, -1},     // 12: Baylonian, Huber 
/*************************/
/* 13: Babylonian, Mercier; eta Piscium culminates with zero point */
{1673941, -5.079167, TRUE, -1},      // 13: Babylonian, Mercier
/*************************/
/* 14: t0 is defined by Aldebaran at 15 Taurus in year -100 */
{1684532.5, -4.44138598, TRUE, 0},  // 14: Babylonian/Aldebaran = 15 Tau
/*************************/
/* 15: Hipparchos */
{1674484.0, -9.33333, TRUE, -1},       // 15: Hipparchos
/*************************/
/* 16: Sassanian */
{1927135.8747793, 0, TRUE, -1},      // 16: Sassanian 
/*************************/
/* 17: Galactic Center at 0 Sagittarius */
{0, 0, FALSE, 0},                    // 17: Galactic Center at 0 Sagittarius 
/*************************/
/* 18: J2000 */
{J2000, 0, FALSE, 0},	             // 18: J2000
/*************************/
/* 19: J1900 */
{J1900, 0, FALSE, 0},	             // 19: J1900
/*************************/
/* 20: B1950 */
{B1950, 0, FALSE, 0},	             // 20: B1950
/*************************/
/* 21: Suryasiddhanta, assuming ingress of mean Sun into Aries at point of mean
      equinox of date on 21.3.499, near noon, Ujjain (75.7684565 E) 
      = 7:30:31.57 UT = 12:33:36 LMT*/
{1903396.8128654, 0, TRUE, 0},	     // 21: Suryasiddhanta
/*************************/
/* 22: Suryasiddhanta, assuming ingress of mean Sun into Aries at true position
      of mean Sun at same epoch */
{1903396.8128654,-0.21463395, TRUE, 0}, // 22: Suryasiddhanta, mean Sun
/*************************/
/* 23: Aryabhata, same date, but UT 6:56:55.57 analogous to 21 */
{1903396.7895321, 0, TRUE, 0},	     // 23: Aryabhata
/*************************/
/* 24: Aryabhata, analogous 22 */
{1903396.7895321,-0.23763238, TRUE, 0}, // 24: Aryabhata, mean Sun
/*************************/
/* 25: Suryasiddhanta, Revati/zePsc at polar long. 359°50'*/
{1903396.8128654,-0.79167046, TRUE, 0}, // 25: SS Revati
/*************************/
/* 26: Suryasiddhanta, Citra/Spica at polar long. 180° */
{1903396.8128654, 2.11070444, TRUE, 0}, // 26: SS Citra
/*************************/
/* 27: True Citra (Spica exactly at 0 Libra) */
{0, 0, FALSE, 0},	             // 27: True Citra 
/*************************/
/* 28: True Revati (zeta Psc exactly at 29°50' Pisces) */
{0, 0, FALSE, 0},	             // 28: True Revati 
/*************************/
/* 29: True Pushya (delta Cnc exactly a 16 Cancer */
{0, 0, FALSE, 0},		     // 29: True Pushya 
/*************************/
/* 30: R. Gil Brand; Galactic Center at golden section between 0 Sco and 0 Aqu;
      note: 0° Aqu/Leo is the symmetric axis of rulerships */
{0, 0, FALSE, 0},                    // 30: Gil Brand
/*************************/
/* 31: Galactic Equator IAU 1958, i.e. galactic/ecliptic intersection point
      based on galactic coordinate system */
{0, 0, FALSE, 0},	             // 31: GE IAU 1958
/*************************/
/* 32: Galactic Equator True, i.e. galactic/ecliptic intersection point based
     on the galactic pole as given in: Liu/Zhu/Zhang, „Reconsidering the
     galactic coordinate system“, A & A No. AA2010, Oct. 2010 */
{0, 0, FALSE, 0},	             // 32: GE true
/*************************/
/* 33: Galactic Equator Mula, i.e. galactic/ecliptic intersection point in the
      middle of lunar mansion Mula */
{0, 0, FALSE, 0},	             // 33: GE Mula
/*************************/
/* 34: Skydram/Galactic Alignment (R. Mardyks); autumn equinox aligned with
      Galactic Equator/Pole */
{2451079.734892000, 30, FALSE, 0},   // 34: Skydram/Mardyks
/*************************/
/* 35: Chandra Hari */
{0, 0, FALSE, 0},	             // 35: Chandra Hari 
/*************************/
/* 36: Dhruva Galactic Centre Middle of Mula (Ernst Wilhelm) */
{0, 0, FALSE, 0},	             // 36: Ernst Wilhelm
/*************************/
/* 37: Kali 3623 = 522 CE, Ujjain (75.7684565), based on Kali midnight and 
      year length of Suryasiddhanta */
{1911797.740782065, 0, TRUE, 0},     // 36: 0 ayanamsha in year 522
/*************************/
/* 38: Babylonian (Britton 2010) 
      John P. Britton, "Studies in Babylonian lunar theory: part III. The
      introduction of the uniform zodiac", in Arch. Hist. Exact. Sci.
      (2010)64:617-663, p. 630. */
{1721057.5, -3.2, TRUE, -1},         // 38: Babylonian (Britton 2010)
/*************************/
/* 39: Sunil Sheoran ("Vedic") 
      S. Sheoran, "The Science of Time and Timeline of World History", 2017. */
{0, 0, FALSE, 0},                    // 39: Sunil Sheoran ("Vedic") 
/*************************/
/* 40: Galactic Center at 0 Capricon (Cochrane) */
{0, 0, FALSE, 0},                    // 40: Cochrane 
/*************************/
/* 41: "Galactic Equatorial" (N.A. Fiorenza) */
{2451544.5, 25.0, TRUE, 0},          // 41: N.A. Fiorenza */
/*************************/
/* 42: Vettius Valens (Moon; derived from Holden 1995 p. 12 for epoch of Valens
      1 Jan. 150 CE julian) */
{1775845.5, -2.9422, TRUE, -1},      // 42: Vettius Valens
/*************************/
/* 43: Lahiri (1940), book "Panchanga darpan": 
      22°26'45".50 + 50".25748T + 0".00011115T^2 */
{J1900, 22.44597222, FALSE, SEMOD_PREC_NEWCOMB}, // 43: Lahiri (1940)
/*************************/
/* 44: Lahiri (VP285), mean sun at 360° in 285CE; epoch for mean sun at 0 acc.
      to Simon 1994, corrected for Vondrak precession 
      (Preface to Lahiri's "Indian Ephemeris" 1980) */
{1825235.2458513028, 0.0, FALSE, 0}, // 44: Lahiri VP285 (1980)
/*************************/
/* 45: Krishnamurti from mean equinox 291, based on Newcomb precession,
      according to D. Senthilathiban, "Study of KP Ayanamsa with Modern
      Precession Theories" (2019), but using precession Vondrak 2011 and
      correction base on Newcomb precession. */
//{1827424.752255678, 0.0, FALSE, SEMOD_PREC_NEWCOMB}, // 45: Krishnamurti VP291
{1827424.752255678, 0.0, FALSE, 0}, // 45: Krishnamurti VP291
/*************************/
/* 46: Lahiri original: Calendar Reform Committee 1956, 
      before the correction by 0.658" in IAE 1985.
      The subtracted value is nutation according to Woolard 1953.
      However, nutation Woolard was used by IENA/IAE only from 1960 on,
      so this value is not correct. In order to reproduce mean ayanamshas
      of IENA >=1960, we could choose 23.25 - 0.00464207 + 0.07 / 3600.0
      as initial value in 1956. However this will not help to reproduce 
      true ayanamshas. A deviation of around 0.1" remains,
      for unknown reasons. The difference between Lahiri (1) and
      Lahiri ICRC (45) amounts to 1.1". */
#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 157: 0.00464207 deg (16.7115") is what calc_nutation_woolard()
 * gave at the epoch with the Moon's mean anomaly 10^4 times too fast (bug
 * 151), not Woolard's nutation: the true ayanamsa there, with
 * SE_SIDBIT_PREC_ORIG (Newcomb, Woolard), came out 23 deg 15' 0.0806" once
 * the series was fixed. Woolard's dpsi at JD 2435553.5 is 16.792088"; the
 * committee's 23 deg 15' 00" holds again, and every date's ayanamsa of this
 * mode is 0.0806" smaller. */
{2435553.5, 23.25 - 0.0046644689480, FALSE, SEMOD_PREC_NEWCOMB}, // 46: SE_SIDM_LAHIRI_ICRC
#else
{2435553.5, 23.25 - 0.00464207, FALSE, SEMOD_PREC_NEWCOMB}, // 46: SE_SIDM_LAHIRI_ICRC
#endif
/*************************/
    };

#define PLAN_DATA struct plan_data

/* obliquity of ecliptic */
#define SWI_EOP_NOMEM		(-4)	/* eop_dpsi_loaded: a table not allocated */
#define SWI_EOP_FINALS_CUT	3	/* eop_dpsi_loaded: finals kept up to a gap */

struct epsilon {
  double teps, eps, seps, ceps; 	/* jd, eps, sin(eps), cos(eps) */
  /* The flags eps was computed under. swi_epsiln() consults SEFLG_JPLHOR and
   * SEFLG_JPLHOR_APPROX -- the Horizons offset moves the obliquity by 0.88
   * arcsec at -3000 -- so an epoch alone does not identify a cached value.
   * swi_check_nutation() has always carried its flags for the same reason. */
  int32 epsflag;
};
/* The bits swi_epsiln() and calc_nutation() both branch on, and so the
 * bits their caches have to key on as well as the epoch. */
#define SWI_JPLHOR_FLAGMASK (SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX)

/* SWE_UPSTREAM_COMPAT -- reproduce defects this fork has FIXED.
 *
 * Never define it in a shipped build. It exists for one caller: G8, the
 * differential that asks whether our changes altered what upstream's own
 * suite prints. A deliberate fix does alter it, and the choice used to be
 * between filtering the affected testcases -- which quietly hollows the gate
 * out -- and re-baselining against ourselves, which loses the upstream
 * comparison for good. This is the third option: build the library the way
 * upstream behaves, compare strictly, and ship the fix.
 *
 * G8 already had the shape of this idea. It runs setest with
 * SE_EPHE_FALLBACK=1 so the suite meets upstream's rules on ephemeris
 * substitution; this extends the same reasoning to numerical defects.
 *
 * What it restores, each proven by check-compat:
 *   - swi_check_ecliptic()/swi_check_nutation() keyed on epoch alone, so a
 *     value computed under SEFLG_JPLHOR was served to callers that did not
 *     ask for it (104 of 120 answers contaminated)
 *   - swe_calc_pctr() never keyed those caches to its own tjd (24.5 arcsec)
 *   - swe_calc(SE_ECL_NUT) under a different ephemeris flag wiped the open
 *     files' constants, so the next call under the old flag failed ("file
 *     is damaged") after a segment lookup divided by zero
 *   - swe_pheno(SE_ECL_NUT) read pla_diam[-1] and mag_elem[-1] and returned
 *     OK; the fork refuses it
 *   - ipl * 100 in swe_calc()'s center-of-body setup overflowed int32 for a
 *     large negative id (UBSan only, so asserted by check-hostile, G25)
 *   - the Moon's light time was taken once, from the distance at the instant
 *     of observation, and read at that instant rounded to a double
 *     (1.18 mas; UPSTREAM-BUGS 23, G33)
 *   - the JPL file and the Swiss files read at TT, where their argument is
 *     TDB (45 m on the barycentric Earth, 0.88 mas on the Moon;
 *     UPSTREAM-BUGS 24, G34)
 *   - the long-term sidereal time spliced at 1850 and 2050 by constant
 *     offsets fitted under an older delta t (-0.097" and -1.908" steps;
 *     UPSTREAM-BUGS 25, G35)
 *   - the long-term sidereal time taken from the mean Sun at TT, so it
 *     followed delta t at 0.041" a second (+2382" at -3000; UPSTREAM-BUGS
 *     29, G39)
 *   - the Earth rotation angle formed as 1.0027 * (JD - J2000), rounded at
 *     the size of the day count (0.3 mas at -999, 1 mas at -13000, and
 *     differently where the multiply-add fuses; UPSTREAM-BUGS 30, G39)
 *   - calc_speed() and the Moshier Moon took a three-point speed as the
 *     slope at t + dt (1.3e-3 deg/day on the topocentric Moon;
 *     UPSTREAM-BUGS 31, G40)
 *   - app_pos_etc_moon() left the light time's rate out of the Moon's
 *     speed (4.3e-4 deg/day topocentric; UPSTREAM-BUGS 32, G40)
 *   - speeds rotated into or out of the ecliptic of date without its
 *     turning, the obliquity's rate (1.6e-5 deg/day; UPSTREAM-BUGS 33, G41)
 *   - the aberration rate taken as the observer's velocity change over the
 *     light time, whole: a chord for a turning site, none on the Sun,
 *     radial too, and first-order only (3.2e-4 deg/day topocentric,
 *     7.4e-5 AU/day on Pluto's distance rate); and with it the forced
 *     three-point topocentric speed (UPSTREAM-BUGS 34, G43)
 *   - a planet's light-time rate taken as a one-day straight-line
 *     difference (3e-6 deg/day on Mercury; UPSTREAM-BUGS 35)
 *   - the topocentric observer turning EARTH_ROT_SPEED per TT day, not
 *     per UT day (3e-6 deg/day on the Moon at -999; UPSTREAM-BUGS 36)
 *   - swe_nod_aps() left that turning in about the heliocentric point and
 *     put none back about the geocentric one (1.6e-5 deg/day on a planet's
 *     node or apsis; UPSTREAM-BUGS 37, G42)
 *   - swe_nod_aps() returned the mean perihelion's and aphelion's latitude
 *     as its latitude speed (1.8 deg/day for Mars; UPSTREAM-BUGS 38, G42)
 *   - swi_deflect_light() formed the elongation's sine and 1 + q.e by
 *     cancelling subtractions, so a planet's speed behind the Sun's disc
 *     was rough by 1.5e-5 deg/day (UPSTREAM-BUGS 39, G44)
 *   - swe_nod_aps() "restored" the save area after its aberration-rate
 *     call with a heliocentric Sun, which computes nothing, so with
 *     SEFLG_SPEED the points after the first used the Earth at tjd - dt
 *     (6-35 mas on Mars's; UPSTREAM-BUGS 46, G47)
 *   - swe_nod_aps() under SEFLG_J2000 turned the points of date to the
 *     equator by J2000's obliquity, without SEFLG_NONUT, and its speeds left
 *     out the frame's turning (7.9" on the Moon's mean node in 2026, 148"
 *     in 2326; 3.2e-5 deg/day; UPSTREAM-BUGS 47, G47)
 *   - swi_osc_el_plan() took a fictitious body's speed as a fixed
 *     ellipse's two-body motion, without its elements' rates in T or an
 *     equinox of date's turning (2.9e-2 deg/day on Vulcan; UPSTREAM-BUGS
 *     48, G48)
 *   - check_t_terms() took any '+' or '-' for a T term, so a plain negative
 *     mean anomaly ("-12.5") set epoch = tjd and froze the fictitious body
 *     (113 deg off the same ellipse written 347.5 after 100 days;
 *     UPSTREAM-BUGS 60, G59)
 *   - intp_apsides() moved the interpolated apsides back along their light
 *     time by the speed, zero without SEFLG_SPEED, and not in x (111 mas
 *     on the perigee; UPSTREAM-BUGS 44, G46)
 *   - swi_intp_apsides() took its last steps from parabolas 18" and 1.8"
 *     wide, rough by 1.3 mas between consecutive JDs, and the speeds
 *     differenced over +-0.1 day and +-8.64 s (1.5e-3 deg/day; UPSTREAM-BUGS
 *     45, G46)
 *   - a fixed star's speeds were not the rates of its position: the
 *     deflection's, aberration's and nutation's rates differenced vectors of
 *     the star's distance (2.3e-3 AU/day on Spica's distance rate), the
 *     precession's rate left out the ecliptic's turning (3e-8 deg/day on
 *     every latitude speed of date, all bodies), the aberration rate was a
 *     backward difference over a rounded step (1.5e-8 deg/day topocentric),
 *     and swe_fixstar() kept the tropical speed for a sidereal longitude
 *     (3.8e-5 deg/day) (UPSTREAM-BUGS 49, G49)
 *   - swe_houses_ex2() returned house cusp speeds that were not the cusps'
 *     rate: whole signs and 0 Aries with the Asc's and MC's speeds,
 *     Porphyry's 11 and 12 built on the Asc's, Krusinski's none (up to
 *     1600 deg/day); Koch, Placidus and Gauquelin without the moving
 *     semi-arc (200 deg/day), the rest differenced over one second; the
 *     ARMC at a constant rate, the obliquity held still (6e-5 deg/day on
 *     the ARMC); sidereal speeds without the ayanamsa's rate
 *     (UPSTREAM-BUGS 40-43, G45)
 *   - swe_houses_ex2(): Placidus and Gauquelin cusps iterated to 1/360000
 *     deg only (3.5e-8 deg off, 1.1e-5 deg/day in their differences at
 *     60N); SEFLG_RADIANS left the cusp and ascmc speeds in deg/day;
 *     sidereal Makransky 'i' computed Treindl's 'I'; Sunshine houses
 *     returned ERR and Porphyry without the Swiss files (UPSTREAM-BUGS
 *     59, G58)
 *   - swe_houses_ex2(): the Sun of Sunshine houses asked with no ephemeris
 *     flag (Swiss for a SEFLG_MOSEPH call); Gauquelin cusps and speeds
 *     13..36 left stale after the Porphyry fallback (also in swe_houses()
 *     and swe_houses_armc_ex2()); a Placidus or
 *     Gauquelin cusp on an equinox point moving at the ARMC's rate
 *     (465.6 deg/day against 360.99) (UPSTREAM-BUGS 68, G67)
 *   - the Moshier series formed their arguments as freq * T and reduced
 *     them after, rounding at more time than the JD resolves (Mercury
 *     3.9e-3 mas, the Moon 0.011 mas between consecutive JDs at -3000);
 *     and took every Moshier velocity as a difference over 8.64 s (the
 *     planets, backward) or +-4.32 s (the Moon) at rounded instants, the
 *     Moon's samples turned with one date's obliquity and the true node's
 *     and apogee's speeds over 0.1 day: ~3e-6 of the speed, 0.13 mas on
 *     Mercury's apparent place, 45" jumps on the osculating apogee
 *     (UPSTREAM-BUGS 50, 51, G50)
 *   - swe_calc_r() took the SEFLG_SPEED3 speed of the true node and the
 *     osculating apogee over +-0.1 day on every ephemeris: 1.4e-3 deg/day
 *     on the apogee, 3.5e-5 on the node (UPSTREAM-BUGS 54, G53)
 *   - the mean nodes' and apsides' speeds as one-day differences: the Moon's
 *     node and perigee backward, without the position's correction terms
 *     (2e-6 deg/day), the aux. position of the mean perigee a day on (2e-5
 *     deg/day on the Moon's latitude speed), the nodes' distance speed over
 *     a day (UPSTREAM-BUGS 55, G54)
 *   - swi_nutate() took the nutation angles' rate as a backward difference
 *     over 1e-4 day, the rate at t - h/2: 6.5e-10 deg/day in every
 *     longitude speed of date, 2.7e-8 in a polar star's right ascension
 *     (UPSTREAM-BUGS 58, G57)
 *   - the aberration rate's Earth, for a far body, left out the Moon's pull
 *     on the Earth: 0.6% of the acceleration, 7.5% of its rate (2.3e-7
 *     deg/day on Nibiru's speed, 6e-7 on VEA's; UPSTREAM-BUGS 62, G61)
 *   - the same Moon's pull was left out on JPL and Moshier (2e-7 to 6e-7
 *     deg/day on a far fictitious body's speed; UPSTREAM-BUGS 66, G65), and so
 *     was the planets' pull on it past 0.3 day of light time (4e-9 on the
 *     far bodies, Swiss files too)
 *   - a topocentric fixed star's light deflection was made from the
 *     geocenter, the topocentric flag not passed to it (3e-7 to 9e-6 arcsec
 *     5-40 degrees from the Sun; UPSTREAM-BUGS 61, G60)
 *   - the planets' mean nodes' and apsides' speeds used the linear terms of
 *     their elements' polynomials only (4e-6 deg/day on Jupiter's aphelion
 *     at +-3000 years, 4.2e-8 on Mars at +-500; UPSTREAM-BUGS 64, G63)
 *   - the star functions returned the caller's flags, not the flags used,
 *     and an emptied nutation cache (tnut 0) answered for JD 0 (4.2e-4 deg
 *     in the ayanamsa; UPSTREAM-BUGS 67, G66)
 *   - add_aberr_rate() took the aberration rate term as tau a less its
 *     radial part, first order in v/c, a site's acceleration stripped of
 *     its radial part (3e-8 deg/day on topocentric speeds, +5e-9
 *     geocentric; UPSTREAM-BUGS 52, G51)
 *   - Moshier moved a body back along its velocity, x(t) - tau v(t), with
 *     one light-time pass for the planets, instead of reading it at t - tau
 *     (13 mas on Mercury; the Moon's speed 1.1e-4 deg/day without
 *     aberration, Mercury's heliocentric distance rate 5.6e-6 AU/day; the
 *     centre-of-body offset added twice; UPSTREAM-BUGS 53, G52)
 *   - swi_intp_apsides() stopped after five passes (perigee) or four
 *     (apogee) of an iteration that converges only linearly, up to 1.21"
 *     (perigee) and 2.5 mas (apogee) short of its fixed point
 *     (UPSTREAM-BUGS 56, G55)
 *   - swi_nutate()'s rate of the nutation matrix held the mean obliquity
 *     still, which N = R1(-eps - deps) R3(-dpsi) R1(eps) turns with when
 *     dpsi != 0: 3e-11 deg/day, 2.0e-9 in a polar star's right ascension
 *     (UPSTREAM-BUGS 69, G68)
 *   - app_pos_etc_plan() read a planet at teval - dt rounded to a double,
 *     and took the heliocentric light-time rate with the Sun's velocity
 *     out of the body's (4.7e-7 deg/day on a five-point difference of
 *     Mercury's heliocentric places, 3e-8 on its heliocentric speed;
 *     UPSTREAM-BUGS 63, G62)
 *   - swe_nod_aps()'s apparent points took the aberration's rate to first
 *     order in v/c, the Earth's jerk without the Moon, and from the Sun the
 *     raw change of its velocity, radial part too (5.7e-9 deg/day on Mars's
 *     mean points, 1.6e-8 on Neptune's, 2.1e-9 AU/day on Neptune's
 *     heliocentric distance rate; UPSTREAM-BUGS 70, G69)
 *   - a fixed star's aberration rate took the Earth's velocity at t + dt
 *     carried on linearly from t - dt and t, its acceleration half a step
 *     back: 8e-12 deg/day on the sky, 1.03e-9 in Polaris's right ascension;
 *     on Moshier the velocities 1e-5 day apart are rough, 1.56e-8
 *     (UPSTREAM-BUGS 76, G75)
 *   - embofs_mosh_rate() took the Moshier Earth-Moon offset's rate as a
 *     difference of the short lunar series over +-2^-8 day, whose rounded
 *     arguments made every Moshier Earth velocity rough at 2e-14 AU/day
 *     (2.1e-6 of the acceleration from velocities 1e-5 day apart;
 *     UPSTREAM-BUGS 85, G84); and the series' arguments were formed as
 *     rate * T, rounded at 6.5e-11 rad at -3000, the offset's rate
 *     without the obliquity's turning (1.5e-13 AU/day; part 2)
 *   - swe_calc_pctr() read both bodies at tjd - dt rounded to a double,
 *     and added the centre's velocity change over the light time as its
 *     aberration rate, radial part and all (9.8e-8 deg/day on a five-point
 *     difference of Mercury's places from Mars, 1.5e-6 AU/day on its
 *     distance rate, 2.1e-6 deg/day on Saturn's speed from Mercury;
 *     UPSTREAM-BUGS 77, G76)
 *   - lunar_osc_elem() divided the true node's and osculating apogee's
 *     differences by 2 x 0.0001 day, not by the interval between the
 *     Moon's rounded epochs, off by up to 2.3e-6 of itself (2.3e-7 deg/day
 *     on the node, 6e-6 on the apogee), and their J2000 latitude and
 *     declination speeds left out the mean obliquity's rate (3.5e-7),
 *     the 0.0001-day step leaving 1e-6 of the Moon's velocity noise on
 *     the apogee and Moshier's 2^-8-day three points 3.2e-6
 *     (UPSTREAM-BUGS 75, G74)
 *   - the other instant-keyed caches were emptied, or a recomputation forced,
 *     by setting their instant to 0, which is also JD 0: at tjd 0 the
 *     barycentric Sun took the Earth slot's Earth for the EMB (0.025 deg on
 *     Mars's osculating node, 5.5e-3 on its mean perihelion, 15 deg on a
 *     barycentric Sun in a warm context, NaN after a call at J2000),
 *     SunRA()'s empty memo answered RA 0 for UT 0 (0.3 mag), and `tjd == 0`
 *     guards kept the save areas from answering at tjd 0 (UPSTREAM-BUGS 71,
 *     G70)
 *   - swe_calc_pctr() deflected as seen from the save area's Earth, not the
 *     centre (0.435" on Saturn from Mercury, 0.33" on the Sun from Jupiter,
 *     deflected by itself), stopped its light time short of the fixed
 *     point (5e-9 deg on Mercury from Pluto), and gave swi_bias() the date
 *     0 under SEFLG_TRUEPOS (0.042" under SEFLG_JPLHOR_APPROX after 1962);
 *     UPSTREAM-BUGS 77, G76)
 *   - the Moon's speed was not the rate of its position: the site turned at
 *     the Earth rotation angle's rate with the precession added again by
 *     swi_precess_speed(), without the equation of the equinoxes' or the
 *     nutation matrix's rate (8.7e-7 deg/day topocentric); the files' rates
 *     were per TDB day (5e-9); the Moon's velocity stayed at the instant
 *     read, not teval - tau, and the aberration rate term spanned dt, not
 *     tau (1e-8 each); swi_aberr_light()'s shift rate was a backward
 *     difference (1.9e-8; UPSTREAM-BUGS 73, G72)
 *   - jplplan()'s Earth and Sun answered for whichever ephemeris had filled
 *     their slots at tjd, so a JPL call after one that fell to Moshier took
 *     the series' heliocentric Earth (the Moon 160 deg off), and the JPL
 *     barycentric Sun was read without its slot's ephemeris and finished at
 *     the Earth slot's instant (JD 0 fresh, another instant warm: 0.11 deg
 *     sidereal; a heliocentric asteroid after it 0.24 deg; UPSTREAM-BUGS
 *     84, G83)
 *   - the heliacal memos kOZ(), ka() and Deltam() were keyed on the Sun's
 *     and object's altitudes alone, not the latitude, eye height or
 *     atmosphere (0.67 mag in a limiting magnitude), the star magnitude's
 *     outlived swe_close() and a new catalogue (0.46 mag, a day in a
 *     heliacal event), and none was emptied (UPSTREAM-BUGS 82, G81)
 *   - swe_set_tid_acc(), swe_set_delta_t_userdef() and
 *     swe_set_interpolate_nut() did not initialise the default context, so
 *     called before any calculation they were wiped by its first-use memset
 *     (22.6 s of delta t; UPSTREAM-BUGS 83, G82)
 *   - the fixed-star memos (sp.fs1, fs1mag, fs2, fs2mag) and the catalogue
 *     swe_fixstar2() reads outlived swe_close() and a path to another
 *     sefstars.txt (Sirius 2.64 pc for 3.33, -1.46 mag for -1.00), a path
 *     change reset a manual tidal acceleration to automatic (9.13 s of
 *     delta t in AD 1000), and the topocentric observer was kept for the
 *     instant alone across ephemerides (4.4" on the Moon at 1600 TT;
 *     UPSTREAM-BUGS 92, G91)
 *   - the star functions as the first call did not name the path from
 *     SE_EPHE_PATH as swe_calc() does (sefstars.txt not found), and a
 *     path change zeroed the astronomical models (18.27 s of delta t in
 *     AD 1000; UPSTREAM-BUGS 98, G97)
 *   - SE_TIDAL_AUTOMATIC stored -25.80 until the next delta t, and
 *     swe_get_tid_acc() did not initialise (0 as the first call), so it
 *     read 0.136 off the value the next delta t uses (UPSTREAM-BUGS 101,
 *     G100)
 *   - swe_deltat() and swe_get_ayanamsa() guessed their ephemeris from the
 *     files this thread had open (169.4 s of delta t in AD 1000 on a second
 *     thread), and a same path, a same JPL file and an ephemeris switch
 *     closed and reopened every file (UPSTREAM-BUGS 103, G102)
 *   - app_pos_etc_plan(), app_pos_etc_sun() and app_pos_etc_moon() solved
 *     Moshier's light time with the Sun held still, v_S tau left out (3.0e-6
 *     deg on every Moshier place without aberration and every heliocentric
 *     one; UPSTREAM-BUGS 102, G101)
 *   - with the files named by SE_EPHE_PATH, a first call that reads a file
 *     before swe_calc() names the path -- swe_calc_ut()'s delta t,
 *     swe_deltat(), swe_sidtime(), swe_houses(), the stars, the ayanamsa,
 *     eclipses, swe_utc_to_jd()'s leap seconds, swe_set_jpl_file() --
 *     read it along the compiled default path, and some read it again
 *     (8.6e-4 deg on the first Moon; UPSTREAM-BUGS 107, G106)
 *   - a first call wrote "Please call swe_set_ephe_path() or
 *     swe_set_jplfile() before ..." into serr whenever it initialised the
 *     context, with SE_EPHE_PATH, files along the default path or another
 *     thread's path in effect (swe_deltat_ex(), swe_get_ayanamsa_ex_ut();
 *     swe_calc_orbel() refused on it; UPSTREAM-BUGS 110, G109)
 *   - on Moshier the fictitious bodies' and swe_calc_orbel()'s light time
 *     held the Sun still (3.0e-6 deg heliocentric), a fixed star took its
 *     parallax and aberration from the heliocentric Earth (3.0e-6 deg
 *     apparent, 9.2e-7 without aberration), the apparent light time stayed
 *     heliocentric (the distance 2.4e-6 AU off on Pluto), and the
 *     heliocentric Moon's speed was 4.2e-8 deg/day off its positions
 *     (UPSTREAM-BUGS 109, G108)
 *   - app_pos_etc_plan() and app_pos_etc_plan_osc() measured a heliocentric
 *     light time from the barycentre on the second pass (0.84" on Mercury;
 *     UPSTREAM-BUGS 26, G36)
 *   - swi_get_observer() built the topocentric observer on the mean equator
 *     of date, 200-270 m from its true-pole place (0.15" on the Moon;
 *     UPSTREAM-BUGS 27, G37)
 *   - lunar_osc_elem() fitted the osculating apogee's ellipse with a 1996
 *     almanac GM, 3.3e-8 above the ephemeris's GMB (0.2" on the apogee;
 *     UPSTREAM-BUGS 28, G38)
 *   - the built-in Kronos had the semi-axis 64.81960 for seorbel.txt's
 *     64.81690 (0.0086 deg at 1700, 0.0169 deg at 2300 without the file),
 *     and swi_osc_el_plan()'s Kepler guess for e > 0.975 used pow(x, 1/3)
 *     = 1 and 180 for pi (rounding only; UPSTREAM-BUGS 65, G64)
 *   - app_pos_etc_plan_osc() read a fictitious body's elements at teval -
 *     dt rounded to a double, so an Earth-centred set's focus jittered by
 *     the Earth's motion over an ulp of the date (3.9e-8 deg on Waldemath,
 *     5.5e-6 deg/day on a five-point difference of its places, 3.4e-7 on
 *     Vulcan heliocentric; UPSTREAM-BUGS 81, G80)
 *   - app_pos_etc_plan() solved the light time along a straight line and
 *     never against the body read (7.8e-10 deg on Mercury on the files),
 *     and app_pos_etc_sun() and app_pos_etc_sbar() gave the heliocentric
 *     and barycentric Earth, the geocentric Sun and the barycentric Sun no
 *     light-time rate (1.7e-6 deg/day on the Earth; UPSTREAM-BUGS 78, G77)
 *   - app_pos_etc_sun() subtracted the Sun read at t - tau from the Swiss
 *     files' heliocentric Earth, not the Sun at t as de200 and the planets
 *     do (3.0e-6 deg; UPSTREAM-BUGS 78 part 2, G77)
 *   - app_pos_etc_sbar() gave the barycentric Sun its velocity at teval, not
 *     at t - tau (3.1e-9 deg/day; UPSTREAM-BUGS 78 part 3, G77)
 *   - check_t_terms() rounded an element's n T as the unreduced angle
 *     (Vulcan's mean anomaly 3.5e7 deg at -3000: its true places 3.0e-8
 *     deg off a smooth curve; UPSTREAM-BUGS 81 part 2, G80)
 *   - app_pos_etc_plan_osc() stopped its light time 1e-12 day short of the
 *     fixed point (2.3e-11 deg on Waldemath), and swe_calc_orbel() took it
 *     along a straight line, reading the orbit at t - tau only with
 *     SEFLG_SPEED (1.6e-5 deg for a = 0.137 AU; UPSTREAM-BUGS 81 part 3,
 *     G80)
 *   - app_pos_etc_sbar() took the barycentric Sun's instant of date from
 *     the Earth's slot, which holds its result but on JPL another
 *     request's instant: its latitude speed of date off by up to 1.9e-5
 *     deg/day (NaN as a context's first request) and its sidereal
 *     longitude by the ayanamsa's change since then (0.14 deg ten years
 *     on; UPSTREAM-BUGS 86, G85)
 *   - deflect_light_from() took the deflection's rate as its change over
 *     the 5e-7 day before t, one-sided, off by half that step times its
 *     curvature behind the Sun's disc (1.9e-6 deg/day on Zeus and 1.8e-6
 *     on Isis-Transpluto 0.005 deg from the centre; UPSTREAM-BUGS 74, G73)
 *   - observer_accel() took the site's acceleration as a difference over
 *     +-2^-11 day, (omega h)^2 / 6 = 1.6e-6 low (6e-10 deg/day on every
 *     topocentric apparent speed; UPSTREAM-BUGS 79, G78)
 *   - swe_nod_aps()'s osculating points took e as sqrt(1 - p / a), 1e-16 / e^2
 *     of itself, and their speeds as differences over 1e-3 day per AU at
 *     rounded instants (5.6e-7 deg/day on Venus's perihelion, 6.0e-8 on
 *     the Sun's perigee, 2.0e-6 on the Moon's; UPSTREAM-BUGS 80, G79)
 *   - SEFLG_SPEED3 differenced SE_ECL_NUT's four angles as a position: the
 *     nutation in obliquity came back as the true obliquity's rate (-6.06e-6
 *     for -1.6026e-3 deg), x[4], x[5] nonzero, uninitialised reads
 *     (UPSTREAM-BUGS 90, G89); with SEFLG_EQUATORIAL or SEFLG_XYZ it read the
 *     asteroids' save area, the last such body's coordinates
 *   - swe_calc_orbel() never brought the obliquity and nutation caches to
 *     tjd nor cleared the save areas on a change of ephemeris: zeros as a
 *     context's first call, another instant's frame of date (1.8e-5
 *     deg/day), a Swiss Sun added on Moshier (2.4e-3 deg/day; UPSTREAM-BUGS
 *     88, G87)
 *   - swi_observer_accel_lt() took the Moshier Earth's acceleration in the
 *     aberration rate of swe_nod_aps()'s points from its velocity at the
 *     light time and the gravitational jerk, off by the series' monthly
 *     terms (1.7e-10 deg/day on Neptune's points' latitude speed;
 *     UPSTREAM-BUGS 93, G92)
 *   - precess_3() answered J2000 itself with the identity, which the
 *     Vondrak series is not there (turned 4.848e-12 rad about x): every
 *     place of date at exactly J2000 2.7e-10 deg off its neighbours in
 *     latitude (UPSTREAM-BUGS 87, G86)
 *   - swi_get_ayanamsa_with_speed() took the ayanamsa's rate as a backward
 *     difference over 0.001 day: the precession modes' rounding over that
 *     step (1.3e-11 to 9e-11 deg/day on every sidereal speed), the star
 *     modes' curvature (1e-9, 2.1e-6 for True Revati at zeta Psc's
 *     conjunction), and 3.6e5 deg/day where the ayanamsa crosses 0
 *     (UPSTREAM-BUGS 91, G90)
 *   - an Earth-centred fictitious body's light-time place on Moshier took
 *     its focus from a second read of the Earth at t - tau, whose rounding
 *     the Earth at teval does not share: 3e-15 AU rms of jitter, 2.7e-11
 *     deg on Waldemath (UPSTREAM-BUGS 96, G95)
 *   - sweph() kept an open Swiss file for any instant inside its data,
 *     which runs past the file's nominal 600 years (and, for semo, starts
 *     before them): a context warm from one side of a join answered just
 *     across it from that side's file (the barycentric Sun 47.6 mas at JD
 *     2378496.6 after a call 1.5 days before; UPSTREAM-BUGS 89, G88)
 *   - split_deg_nakshatra() tested for the Sheoran ayanamsa as a bit mask,
 *     (sid_mode & 39) == 39, which SE_SIDM_USER (255) passes: a
 *     user-defined ayanamsa's nakshatras 3 deg 20' early (UPSTREAM-BUGS
 *     97, G96)
 *   - (nothing for UPSTREAM-BUGS 100: the neighbour-file lookup it closes
 *     again when the neighbour does not hold the instant is this fork's
 *     and absent under the switch; G24 asserts both builds take the
 *     Moshier Moon there, G99)
 *   - swi_moshmoon()'s velocity a difference of the series' positions,
 *     not their derivative (bug 51's five points left 1.2e-13 of it as
 *     jitter, upstream's two far more), and lunar_osc_elem()'s
 *     eccentricity sqrt(1 - p / a): the osculating apogee's and perigee's
 *     speeds noisy by 1.4e-7 (SE_OSCU_APOG) and 5.5e-6 deg/day
 *     (swe_nod_aps()) on Moshier, 2.4e-8 on de200 (UPSTREAM-BUGS 95, G94)
 *   - read_const() and get_new_segment() ran free_planets() on a file they
 *     refused (misnamed, damaged), emptying the constants of every file
 *     left open: the next calls failed "sepl_06.se1 is damaged (2)" until
 *     each had been reopened (UPSTREAM-BUGS 104, G99)
 *   - swe_nod_aps()'s osculating ellipses fitted with the Astronomical
 *     Almanac's GM, not the ephemeris's: the Moon's 3.3e-8 high (its
 *     apsides 5e-5 deg off SE_OSCU_APOG's), the Sun's 1.8e-10 low,
 *     plmass's planets for de200's and Mercury's mass for Pluto (Venus's
 *     perihelion 3e-6 deg, de200's Neptune's 2.9e-3; UPSTREAM-BUGS 105,
 *     G104)
 *   - swe_nod_aps()'s osculating points on an ephemeris with a frame bias
 *     (DE >= 403) turned by it twice, swe_calc()'s J2000 state already
 *     having it (the Moon's node 7e-5 deg, its apogee 6e-6, twice that
 *     with SEFLG_ICRS), its EMB on a JPL file built with EARTH_MOON_MRAT
 *     not the file's EMRAT; swe_get_orbital_elements() with the AA's GMs
 *     and Pluto Mercury's mass (Pluto's a 2.5e-7 off, relative;
 *     UPSTREAM-BUGS 108, G104, G107)
 *   - (nothing for UPSTREAM-BUGS 106: the bounds on what a file's header,
 *     the delta-psi table and the star catalogue give as sizes and indexes
 *     hold in both builds. They refuse only files no valid ephemeris is;
 *     every distributed file reads bit for bit as before, so no G24 case;
 *     G103)
 *   - swe_get_orbital_elements()'s e as sqrt(1 - p / a) and true anomaly
 *     as 2 atan(k sin E / (1 + cos E)) (4.8e-9 deg near aphelion), its
 *     periods a^1.5 sidereal years without the body's mass (Jupiter's 2.1
 *     days long), the tropical period times ysid / ytrop, the synodic
 *     against k (UPSTREAM-BUGS 111, G107)
 *   - a seorbel.txt epoch or equinox read by atof() (any text, or none, a
 *     JD), an empty element field read as 0, and a short line refusing
 *     every body after it; swe_fixstar2() and swe_fixstar2_mag() dropping
 *     the catalogue's load error ("could not find star name" only), and
 *     swe_fixstar() with serr NULL answering an unknown star with the
 *     file's last (UPSTREAM-BUGS 113, G111; the element-term hang and the
 *     partial table's leak are fixed in both builds)
 *   - a seorbel.txt term's power after T taken by atoi() ("T5" to "T9"
 *     ignored, "T0" as T, "T 2" as T2 times 2) and a number taken only
 *     when atof() was not 0 or began with '0' (".0" read as a factor 1,
 *     "1.2.3" as 1.2) (UPSTREAM-BUGS 114, G111)
 *   - swe_set_astro_models() of a version before 2.01 published only the
 *     SID group, so the tidal acceleration it sets was lost at the default
 *     context's next sync (-25.936 for -25.826 after "SE1.80", 9.13 s of
 *     delta t in AD 1000; UPSTREAM-BUGS 116, G113)
 *   - swe_calc()'s mean node and mean apogee speeds as (x(t) - x(t -
 *     0.001)) / 0.001 with t - 0.001 rounded: the step off by half an ulp
 *     of the date, 1.2e-8 deg/day on the node, 2.5e-8 on the apogee
 *     (UPSTREAM-BUGS 117, G114)
 *   - a sefstars.txt that failed to load reread and reparsed by every
 *     later swe_fixstar2() and swe_fixstar2_mag() (842 us a call at a bad
 *     line 500), a missing one searched for on every call (two fopen());
 *     upstream also rereads a catalogue repaired in place
 *     (UPSTREAM-BUGS 121, G118)
 *   - (nothing for UPSTREAM-BUGS 119: the thread that adopted another's
 *     JPL file kept the old file's positions, and one that adopted another's
 *     path the old directory's segments, through this fork's shortcuts of
 *     bug 103; under the switch the next calculation frees every body's
 *     data, as upstream's does, and answers from the adopted files: G24's
 *     adopted-JPL case is 0 both ways; G116)
 *   - (nothing for UPSTREAM-BUGS 118: the moon file a context's first
 *     tidal term opened and the path setter overwrote, the setters'
 *     automatic tidal term resolved before their new state, the same path
 *     blind to a moon file installed since, and the first-call probe's
 *     second open are all in this fork's code (bugs 101, 103, 107, 110) and
 *     absent under the switch; G24 asserts both builds clean, G115)
 *   - (nothing for UPSTREAM-BUGS 122: swe_set_ephe_fallback() published as
 *     a configuration group, so another thread on the default context and
 *     a new context see it, is in both builds; the setting is this fork's
 *     and upstream has no such call; G119)
 *   - with the Swiss Moon file missing, the Moon and the Earth made from
 *     Moshier's Moon under a SEFLG_SWIEPH flag, answered under the strict
 *     setting: the Moon 258 at 1500 (UPSTREAM-BUGS 120, G117)
 *   - a calculation on the default context syncing with the shared
 *     configuration from inside itself (the observer's delta t and
 *     sidereal time, swe_calc_ut()'s second step, a star's Earth), so
 *     another thread's setter closed its files and emptied its caches
 *     half-way: a crash on the JPL file, 4.3e-6..1.1e-5 deg mixed answers
 *     (UPSTREAM-BUGS 115, G112)
 *   - (nothing for UPSTREAM-BUGS 123: swe_close() bumping the shared
 *     configuration's generation instead of zeroing it, and resetting in
 *     it only what it resets in the context, so the path, JPL file, delta
 *     t, interpolation, lapse rate and fallback it keeps survive the next
 *     setter, is in both builds; the master is this fork's, and upstream's
 *     swe_close() keeps those settings on its thread; G120)
 *   - the searches, the houses, the sidereal time, swe_azalt(),
 *     swe_pheno(), delta t and the UTC conversions on the default context
 *     syncing at each calculation they make, or never: another thread's
 *     setter adopted between two iterations, a sidereal Moon crossing
 *     5600 s off, a local eclipse searched at the master's site from
 *     half-way, a user delta t kept out of the UTC conversions, 22 s
 *     (UPSTREAM-BUGS 124, G121)
 *   - swe_set_interpolate_nut() resetting the interpolation's nodes alone,
 *     so the nutation, observer and positions cached at an instant
 *     answered for the old setting (8.38e-5" in dpsi; UPSTREAM-BUGS 126,
 *     G122). A thread adopting the change through the shared
 *     configuration empties the same in both builds: that adoption is
 *     this fork's code
 *   - on Moshier a body read past a day of light time (beyond 173 AU)
 *     took the Sun's motion about the barycentre by bug 109's Taylor step,
 *     5.7e-4 AU short over VEA's 462 days: VEA and a swe_calc_orbel()
 *     orbit at 80000 AU 2e-7 deg off the light time's fixed point, their
 *     speeds 5.4e-9 deg/day off their positions; under the switch the Sun
 *     is held still, as upstream's (UPSTREAM-BUGS 125, G27, G65)
 *   - swe_close() resetting the observer and the sidereal mode, which the
 *     manual has survive it, in the context and in the shared
 *     configuration: after it the topocentric Moon refused ("geographic
 *     position has not been set") and the ayanamsa Fagan/Bradley's 24.740
 *     deg for Lahiri's 23.857 (UPSTREAM-BUGS 127, G123)
 *   - a thread's first call reading the configuration from its context
 *     before anything has started it, zeros for the main thread's setter:
 *     a user delta t of 86.40 s answered 63.83 s by swe_deltat_ex() and
 *     every function of UT through it, the sidereal-time model in
 *     swe_sidtime0() 3.1e-7 h, swe_get_astro_models() listing zeros
 *     (UPSTREAM-BUGS 128, G124)
 *   - the interpolation's nodes kept for their instants alone across a
 *     change of nutation model (swe_set_astro_models(),
 *     SE_SIDBIT_PREC_ORIG, swe_close()), so the old model answered there
 *     (8.45e-3" in dpsi), and the topocentric observer across any change
 *     of model (UPSTREAM-BUGS 129, G122). A thread adopting the change
 *     empties both in both builds: that adoption is this fork's code
 *   - swe_calc_orbel() (this fork's) without bug 20's frame-bias pair, as
 *     upstream's fictitious path it follows there: its J2000 answer seen
 *     from the ICRS Earth, 3.2e-8 to 1.1e-7 AU on every geocentric place,
 *     and with SEFLG_ICRS the body unturned, 23 mas (UPSTREAM-BUGS 130,
 *     G126)
 *   - swe_time_equ(), swe_lmt_to_lat() and swe_lat_to_lmt() reading the
 *     Sun from the JPL file only when this context has it open, not when
 *     the configuration sets it (bug 103's old guess): another thread's,
 *     a later context's swe_set_jpl_file() missed, E 1.19e-8 day (1.03
 *     ms) off at J2000, and after swe_close() a JPLEPH calculation moving
 *     them to the file (UPSTREAM-BUGS 132, G128)
 *   - (nothing for UPSTREAM-BUGS 133: the Sunshine houses' remembered
 *     declination, which swe_houses_armc() can set before a context's
 *     first start, kept across that start's memset, and 99 left there
 *     when none was set, is in both builds; Phase 3c moved upstream's
 *     static, which nothing resets, into the context; G129)
 *   - after swe_close() without the path again, the first delta t taking
 *     the default tidal term until a file opened, the path and the JPL
 *     file kept and their DE numbers forgotten: 1463.43 s in AD 1000 for
 *     1474.72, the first UT Moon there 1.6e-3 deg off the second, with
 *     de200.eph 14.29 s in AD 1700 for 3.01 (UPSTREAM-BUGS 131, G127).
 *     The threads and contexts adopting the close read them again in the
 *     shipped build only, as the closing one does
 *   - the nodes kept across the JPL Horizons flags of a call, the
 *     approximation's mode (1.03e-1" in dpsi) and, under SEFLG_JPLHOR,
 *     the IERS corrections swe_close() frees, and swe_get_ayanamsa_ex()
 *     taking the nutation kept at its instant whatever flags computed it
 *     (6.2e-2") (UPSTREAM-BUGS 134, G122). A thread adopting a change of
 *     mode empties the nodes in both builds, as above
 *   - (nothing for UPSTREAM-BUGS 137: a thread adopting another's path
 *     zeroing the closed JPL file's DE number, as the branch for a new JPL
 *     file does, so its JPLEPH delta t takes the term of the files it has
 *     now, is in both builds; the adoption is this fork's, and upstream's
 *     threads share no configuration; G132)
 *   - a JPL file a calculation opens setting the automatic tidal term for
 *     itself, not for swe_deltat()'s guess: after the path setter forgets
 *     the JPL DE number, swe_get_tid_acc() de200's -23.8946 for the
 *     -25.936 the next swe_deltat() takes, 2.04 arcsec/cty^2
 *     (UPSTREAM-BUGS 138, G132)
 *   - swe_deltat_ex() with no ephemeris bit taking the tidal term of the
 *     JPL file the context has open, else the default, not the Swiss
 *     files' a 0 flag means everywhere else: with de200.eph open 169.4 s
 *     off in AD 1000, a second thread 4.3 ms from the first in 1950
 *     (UPSTREAM-BUGS 145, G132)
 *   - with the fallback allowed, a Swiss or JPL barycentric request whose
 *     files are missing answered from Moshier, which swecalc() refuses
 *     asked for itself: its heliocentric place under SEFLG_BARYCTR |
 *     SEFLG_MOSEPH, Mars 0.24 deg, 7.2e-3 AU off the files' barycentric
 *     one; swe_calc_orbel(), swe_calc_pctr() and swe_nod_aps()'s
 *     barycentric ellipse alike (UPSTREAM-BUGS 139, G133)
 *   - the interpolation's nodes lying both sides of SEMOD_JPLHORA_3's
 *     step at 1962 Jan 20 under SEFLG_JPLHOR_APPROX (7.17e-2" in dpsi):
 *     dpsi there 1.95e-2" off the exact, 6.93e-2" at worst
 *     (UPSTREAM-BUGS 146, G122)
 *   - (nothing for UPSTREAM-BUGS 135: swe_calc_orbel() refusing a
 *     barycentric Moshier request with swe_calc()'s message, as the compat
 *     build's swe_calc() refuses it too, is this fork's function, active
 *     in both builds; G126)
 *   - (nothing for UPSTREAM-BUGS 136: swe_calc_orbel()'s elements of the
 *     equinox of date at the emission instant under light time, as
 *     upstream's JDATE branch reads them, and that frame's turning in the
 *     speed, are this fork's function, active in both builds; G126)
 *   - the topocentric site placed by UT1 = TT - delta t rounded to a
 *     double, 2^-31 day: a staircase in delta t, whose rate leaves out
 *     delta t's that the site's velocity carries, the topocentric speed of
 *     an orbit 0.01 AU from the Earth 1.8e-8 to 4.7e-8 deg/day off its
 *     positions' five-point rate, the topocentric Moon's 2.5e-7
 *     (UPSTREAM-BUGS 140, G126)
 *   - (nothing for UPSTREAM-BUGS 141: swe_calc_orbel()'s return flag as
 *     swe_calc() makes it, the caller's coordinate bits and no default
 *     ephemeris bit where none was named, is this fork's function, active
 *     in both builds; G126)
 *   - (nothing for UPSTREAM-BUGS 142: SEFLG_JPLHOR without the IERS
 *     corrections taken as SEFLG_JPLHOR_APPROX by swe_get_ayanamsa_ex(),
 *     so the sidereal houses, swe_gauquelin_sector() and calc_nutation()
 *     itself, which crashed on the missing table, is in both builds, a
 *     crash's fix; G134)
 *   - (nothing for UPSTREAM-BUGS 143: a thread or context adopting another
 *     thread's swe_set_jpl_file() loading the IERS corrections its setter
 *     loaded, where its SEFLG_JPLHOR fell to SEFLG_JPLHOR_APPROX (4.2e-2"
 *     in dpsi), is in both builds: that adoption is this fork's code; G134)
 *   - swe_set_ephe_path() keeping the IERS corrections read from the old
 *     directory, which load_dpsi_deps() loads once: after path A, the JPL
 *     file, path B and the JPL file again, SEFLG_JPLHOR answered A's dpsi
 *     for B's (0.131"), after path B alone A's for SEFLG_JPLHOR_APPROX's
 *     (UPSTREAM-BUGS 144, G134). A thread adopting the path drops them in
 *     both builds, as above
 *   - Woolard's nutation turning the Moon's mean anomaly at 13255523.59
 *     revolutions a century, 10^4 times its 1325.552359: its terms in it
 *     cycling every 3.97 minutes, dpsi up to 0.236" and deps 0.0226" off
 *     the series (UPSTREAM-BUGS 151, G138)
 *   - SE_SIDM_LAHIRI_ICRC's mean ayanamsa at its epoch, 23.25 - 0.00464207
 *     deg, the subtracted 16.7115" being that broken Woolard's dpsi there:
 *     with Woolard fixed the true ayanamsa at the epoch is 23 deg 15'
 *     0.0806" for the committee's 23 deg 15' (UPSTREAM-BUGS 157, G138)
 *   - swe_calc_pctr()'s centre read at the light-time instant overwritten
 *     by the body's before it was tested: a centre whose files begin
 *     after that instant taken as zero there, the speed 0.130 deg/day off
 *     upstream, returned OK (UPSTREAM-BUGS 158, G144)
 *   - an eop_finals.txt that breaks off throwing away a complete
 *     eop_1962_today.txt (-3): SEFLG_JPLHOR fallen to SEFLG_JPLHOR_APPROX,
 *     dpsi 4.2e-2" off on G134's fixtures (UPSTREAM-BUGS 171, G134)
 *   - (nothing for UPSTREAM-BUGS 172: a failed allocation of the IERS
 *     tables now reports the memory, not eop_1962_today.txt "not found",
 *     in both builds, as entry 156's fix of the same branch)
 *   - (nothing for UPSTREAM-BUGS 152: the default nutation's grid, which
 *     left the IAU 2000A series past 5e8 days and now gives way to it at
 *     4e8, is this fork's; the compat build's default, 2000B, is computed
 *     directly and has no grid; G29)
 *   - (nothing for UPSTREAM-BUGS 147: a thread adopting another's path
 *     dropping the old directory's delta t table, leap seconds, star file
 *     and planet-name memo as the setter does, and calc_deltat()'s memo
 *     emptied with the table it was made from, are in both builds; the
 *     adoption and the memo are this fork's; G132)
 *   - swe_set_ephe_path() forgetting the DE number of the JPL file whose
 *     name it keeps (F4), so the first JPLEPH delta t takes the default
 *     tidal term until a calculation opens the file: after the path,
 *     de200.eph and the path again, the first UT JPL Moon in AD 1700
 *     1.79e-3 deg off the second, swe_deltat()'s guess Swiss
 *     (UPSTREAM-BUGS 148, G132)
 *   - the speeds of date across SEMOD_JPLHORA_3's step at 1962 Jan 20
 *     under SEFLG_JPLHOR_APPROX taking rates that difference the models
 *     across it: upstream's backward nutation rate 0.200 deg/day off
 *     within 1e-4 day after it, SEFLG_SPEED3's three places 5.75e-2 either
 *     side (this fork's precession, obliquity and site rates 1.57e-5 for a
 *     day before it, its central nutation rate 9.96e-2 within 1e-4 day)
 *     (UPSTREAM-BUGS 153, G139)
 *   - swe_nod_aps() with barycentric output on Moshier, asked for or fallen
 *     to, answering Moshier's heliocentric points, returned OK: Mars's
 *     osculating node 0.077 deg off the files' barycentric one, the Moon's
 *     mean node 0.37 deg, every method and body class (UPSTREAM-BUGS 149,
 *     G133)
 *   - swe_calc_pctr() returning the caller's flags whatever answered: with
 *     the fallback allowed and the JPL file missing 257 (SEFLG_JPLEPH) on
 *     the Swiss answer, with the Moon files missing SEFLG_SWIEPH on a Moon
 *     from Moshier (UPSTREAM-BUGS 150, G133)
 *   - swe_set_jpl_file() of a file below DE 403, or of one not there,
 *     keeping the IERS corrections a DE 403 file loaded: after path A, the
 *     DE431 fixture and de200.eph, SEFLG_JPLHOR answered A's dpsi where a
 *     fresh process falls to SEFLG_JPLHOR_APPROX (4.2e-2"; UPSTREAM-BUGS
 *     154, G134). A thread adopting the file drops them in both builds
 *   - (nothing for UPSTREAM-BUGS 155 and 156: load_dpsi_deps() freeing the
 *     tables a load it gave up on left, closing its file on every early
 *     return and freeing dpsi[] when deps[]'s allocation fails, is in both
 *     builds, a leak's fix that changes no answer; G134)
 *   - under SEFLG_JPLHOR_APPROX and SEMOD_JPLHORA_3, the frame bias and the
 *     Horizons offset in right ascension in from the step at 1962 Jan 20
 *     itself, where the precession and nutation change after it (Polaris
 *     4.95e-5 deg in right ascension there), and given the light-time
 *     instant, so a place over its light time after the step without
 *     them (Mars 1.23e-5 deg), the true Sun, given 0, at every date after
 *     it (UPSTREAM-BUGS 161, G141)
 *   - SEFLG_JPLHOR_APPROX's offset in right ascension turning the position
 *     about the pole and not the velocity, so the speeds were off their
 *     places' rate by the offset times the transverse velocity: the Moon's
 *     latitude 1.46e-6 deg/day, Mars's right ascension 1.25e-7, Mars's
 *     distance 7.2e-9 AU/day (UPSTREAM-BUGS 162, G141)
 *   - SEFLG_JPLHOR_APPROX's yearly offset table interpolated backwards:
 *     the offset jumping up to 0.99 mas at every year from 1963 to 2011
 *     (UPSTREAM-BUGS 173, G141)
 *   - the path setter and swe_close() reading the kept JPL file again only
 *     when its DE number had been read, not when swe_set_jpl_file() had
 *     named it and failed along the path then set: after a path without
 *     de200.eph, de200.eph and a path with it, the first UT JPL Moon in AD
 *     1700 1.79e-3 deg off the second (UPSTREAM-BUGS 164, G132)
 *   - a JPLEPH delta t with no JPL file set taking the default tidal term
 *     until a calculation opens the default file by its name: with a
 *     de431.eph that is de200.eph, the first UT JPL Moon in AD 1700
 *     1.79e-3 deg off the second, in a fresh process (UPSTREAM-BUGS 165,
 *     G132, G24)
 *   - the Swiss files' tidal term without the moon files the default,
 *     -25.80 (DE431's), not the planet files' DE number's (DE441, -25.936):
 *     delta t in AD 1000 11.29 s off, the heliocentric Mars 5.8e-5 deg
 *     there (UPSTREAM-BUGS 166, G142, G24)
 *   - swe_nod_aps() on Moshier seeing the Sun's geocentric points from the
 *     Earth, not the Sun: the mean perigee at 2000 Jul 1.5 281.4259 deg at
 *     1.9993 AU for the files' 282.9417 deg at 0.9833 AU, every method
 *     (UPSTREAM-BUGS 167, G143)
 *
 * Anything added here needs a check-compat case, or the switch rots into a
 * macro that silently does nothing.
 */
#ifdef SWE_UPSTREAM_COMPAT
# define SWI_EPS_KEY(iflag)  ((int32) 0)
#else
# define SWI_EPS_KEY(iflag)  ((iflag) & SWI_JPLHOR_FLAGMASK)
#endif

/*
extern struct epsilon oec2000;
extern struct epsilon oec;
*/

struct plan_data {
  /* the following data are read from file only once, immediately after 
   * file has been opened */
  int ibdy;		/* internal body number */
  int32 iflg;		/* contains several bit flags describing the data:	
			 * SEI_FLG_HELIO: true if helio, false if bary
			 * SEI_FLG_ROTATE: TRUE if coefficients are referred 
			 *      to coordinate system of orbital plane 
			 * SEI_FLG_ELLIPSE: TRUE if reference ellipse */
  int ncoe;		/* # of coefficients of ephemeris polynomial,
			   is polynomial order + 1  */
  /* where is the segment index on the file */
  int32 lndx0;   	/* file position of begin of planet's index */
  int32 nndx;		/* number of index entries on file: computed */
  double tfstart;	/* file contains ephemeris for tfstart thru tfend */
  double tfend;         /*      for this particular planet !!!            */
  double dseg;		/* segment size (days covered by a polynomial)  */
  /* orbital elements: */
  double telem;		/* epoch of elements */
  double prot;		
  double qrot; 
  double dprot;
  double dqrot; 
  double rmax;		/* normalisation factor of cheby coefficients */
  /* in addition, if reference ellipse is used: */
  double peri;		
  double dperi; 
  double *refep;	/* pointer to cheby coeffs of reference ellipse,
			 * size of data is 2 x ncoe */
  /* unpacked segment information, only updated when a segment is read: */
  double tseg0, tseg1;	/* start and end jd of current segment */
  double *segp;         /* pointer to unpacked cheby coeffs of segment;
			 * the size is 3 x ncoe */
  int neval;		/* how many coefficients to evaluate. this may
			 * be less than ncoe */
  /* result of most recent data evaluation for this body: */
  double teval;		/* time for which previous computation was made */
  int32 iephe;            /* which ephemeris was used */
  double x[6];		/* position and speed vectors equatorial J2000 */
  int32 xflgs;		/* hel., light-time, aberr., prec. flags etc. */
  double xreturn[24];   /* return positions:
			 * xreturn+0	ecliptic polar coordinates
			 * xreturn+6	ecliptic cartesian coordinates
			 * xreturn+12	equatorial polar coordinates
			 * xreturn+18	equatorial cartesian coordinates
			 */
};

/*
 * stuff exported from swemplan.c and swemmoon.c 
 * and constants used inside these functions.
************************************************************/

#define STR             4.8481368110953599359e-6 /* radians per arc second */

/* moon, s. moshmoon.c */
extern int swi_mean_node(swe_ctx *ctx, double jd, double *x, char *serr);
extern int swi_mean_apog(swe_ctx *ctx, double jd, double *x, char *serr);
extern int swi_moshmoon(swe_ctx *ctx, double tjd, AS_BOOL do_save, double *xpm, char *serr) ;
extern int swi_moshmoon2(swe_ctx *ctx, double jd, double *x);
extern int swi_intp_apsides(swe_ctx *ctx, double J, double *pol, int ipli);
#ifndef SWE_UPSTREAM_COMPAT
extern int swi_intp_apsides_est(swe_ctx *ctx, double J, double *pol, int ipli, int npass);
#endif

/* planets, s. moshplan.c */
extern int swi_moshplan(swe_ctx *ctx, double tjd, int ipli, AS_BOOL do_save, double *xpret, double *xeret, char *serr);
extern int swi_moshplan2(swe_ctx *ctx, double J, int iplm, double *pobj);
#ifndef SWE_UPSTREAM_COMPAT
extern void swi_mosh_earth_accel(swe_ctx *ctx, double tjd, const double *x0, double *acc);
#define SWI_MOSH_SUN_FULL 0	/* every planet (bug 102) */
#define SWI_MOSH_SUN_MOON 1	/* Venus, EMB and the giants (bug 109) */
extern void swi_mosh_sun_bary(swe_ctx *ctx, double tjd, int set, double *xs, double *as);
extern void swi_mosh_sun_bary_aside(swe_ctx *ctx, double tjd, int set, double *xs);
extern void swi_mosh_sun_accel(swe_ctx *ctx, double tjd, double *as);
#endif
extern int swi_osc_el_plan(swe_ctx *ctx, double tjd, double * SWI_RESTRICT xp,
                           int ipl, int ipli, const double * SWI_RESTRICT xearth,
                           const double * SWI_RESTRICT xsun, char *serr);
extern FILE *swi_fopen(swe_ctx *ctx, int ifno, char *fname, char *ephepath, char *serr);
extern int32 swi_init_swed_if_start(swe_ctx *ctx);
/* "JPL" / "Swiss (.se1)" / "Moshier", for error messages. */
extern const char *swi_ephe_name(int32 epheflag);
/* SE_EPHE_FALLBACK, read fresh -- see swe_set_ephe_fallback(). */
extern AS_BOOL swi_env_ephe_fallback(void);
#ifndef SWE_UPSTREAM_COMPAT
/* An answer whose flag got_iflag has SEFLG_BARYCTR and SEFLG_MOSEPH,
 * refused: Moshier serves no barycentric place (UPSTREAM-BUGS 139; and
 * swe_nod_aps(), 149). Strict: the downgrade's message; fallback allowed or
 * Moshier asked: swecalc()'s. */
extern AS_BOOL swi_refuse_mosh_bary(swe_ctx *ctx, int32 wanted, int32 got_iflag, double tjd, char *serr);
#endif
/* Close the .se1 files. FORGET_DENUM when the next open may find different
 * files, KEEP_DENUM when the same ones are coming back -- delta-t reads that
 * number before anything reopens them. */
#define FORGET_DENUM TRUE
#define KEEP_DENUM   FALSE
extern void swi_close_ephe_files(swe_ctx *ctx, AS_BOOL forget_denum);
/* swemplan.c: one non-comment line of seorbel.txt, kept verbatim in the
 * context (see swe_ctx.fict_lines). */
struct fict_line { int32 iline; char s[AS_MAXCH]; };
extern void swi_free_fict_lines(swe_ctx *ctx);
extern int32 swi_set_tid_acc(swe_ctx *ctx, double tjd_ut, int32 iflag, int32 denum, char *serr);
extern int32 swi_get_tid_acc(swe_ctx *ctx, double tjd_ut, int32 iflag, int32 denum, int32 *denumret, double *tid_acc, char *serr);

extern int32 swi_get_ayanamsa_ex(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr);
extern int32 swi_get_ayanamsa_ex_ut(double tjd_ut, int32 iflag, double *daya, char *serr);
extern int32 swi_get_ayanamsa_with_speed(swe_ctx *ctx, double tjd_et, int32 iflag, double *daya, char *serr);

extern double swi_armc_to_mc(double armc, double eps);

extern int32 swi_get_denum(swe_ctx *ctx, int32 ipli, int32 iflag);
#ifndef SWE_UPSTREAM_COMPAT
extern double swi_gm_earth_moon(swe_ctx *ctx, int32 epheflag);
extern double swi_gm_sun_planet(swe_ctx *ctx, int32 epheflag, int iel);
extern double swi_emb_mrat(swe_ctx *ctx, int32 epheflag);
extern double swi_gm_planet(swe_ctx *ctx, int32 epheflag, int iel);
#endif


/* Upstream bug 71: the same idiom in every other cache keyed on an instant
 * -- the planets' and nodes' save areas (teval, tsave), the obliquity
 * (teps), the observer (topd.teval), SunRA()'s memo -- and in sweph()'s
 * "force a new computation" of the EMB by teval = 0, which at tjd 0 handed
 * the Earth slot's Earth to the barycentric Sun as the EMB (0.025 deg on
 * Mars's osculating node, 15 deg on a barycentric Sun). An empty or forced
 * entry is now SWI_TJD_NEVER, which no instant equals, and the `tjd == 0`
 * / `tjd != 0` guards are gone. */
#define SWI_TJD_NEVER (-1.0e300)

/* nutation */
/* Upstream bug 67: tnut == 0 is also JD 0 (-4712), so an emptied or
 * never-filled cache, or ctx->nutv/nut2000 (no longer filled since bug 58),
 * matched a request at exactly tjd = 0 (or J2000, for nut2000) and gave a
 * zero nutation matrix or angles: swe_get_ayanamsa_ex() at tjd 0 lacked the
 * nutation, 4.2e-4 deg. An empty cache is now tnut = SWI_NUT_NEVER. */
#define SWI_NUT_NEVER SWI_TJD_NEVER
struct nut {
  double tnut;
  double nutlo[2];	/* nutation in longitude and obliquity */
  double snut, cnut;	/* sine and cosine of nutation in obliquity */
  double matrix[3][3];	
  double dnut[2];	/* rate of nutlo, rad/day, when dnut_ok */
  double deps0;		/* rate of the mean obliquity the matrix holds, rad/day, when dnut_ok */
  int dnut_ok;
};

struct plantbl {
  char max_harmonic[9];
  char max_power_of_t;
  /* These point at the static perturbation tables in swemptab.h, which are
   * read-only. const-qualified so they land in .rodata rather than .data:
   * shared between processes, and provably never written. */
  const signed char *arg_tbl;
  const double *lon_tbl;
  const double *lat_tbl;
  const double *rad_tbl;
  double distance;
};

struct file_data {
  char fnam[AS_MAXCH];	/* ephemeris file name */
  int fversion;		/* version number of file */
  char astnam[50];	/* asteroid name, if asteroid file */ 
  int32 sweph_denum;     /* DE number of JPL ephemeris, which this file
			 * is derived from. */
  FILE *fptr;		/* ephemeris file pointer */
  double tfstart;       /* file may be used from this date */
  double tfend;         /*      through this date          */
  int32 iflg; 		/* byte reorder flag and little/bigendian flag */
  short npl;		/* how many planets in file */
  int ipl[SEI_FILE_NMAXPLAN];	/* planet numbers */
#ifndef SWE_UPSTREAM_COMPAT
  double tnom0, tnom1;	/* the nominal span [tnom0, tnom1) it answers for,
			 * its instant's 600-year bucket (UPSTREAM-BUGS 89) */
#endif
};
 
struct gen_const {
 double clight, 
	aunit, 
	helgravconst, 
	ratme, 
	sunradius;
};

struct save_positions {
  int ipl;
  double tsave;
  int32 iflgsave;
  /* position at t = tsave,
   * in ecliptic polar (offset 0),
   *    ecliptic cartesian (offset 6), 
   *    equatorial polar (offset 12),
   *    and equatorial cartesian coordinates (offset 18).
   * 6 doubles each for position and speed coordinates.
   */
  double xsaves[24];    
};

struct node_data {
  /* result of most recent data evaluation for this body: */
  double teval;		/* time for which last computation was made */
  int32 iephe;            /* which ephemeris was used */
  double x[6];		/* position and speed vectors equatorial J2000 */
  int32 xflgs;		/* hel., light-time, aberr., prec. flags etc. */
  double xreturn[24];   /* return positions: 
			 * xreturn+0	ecliptic polar coordinates
			 * xreturn+6	ecliptic cartesian coordinates
			 * xreturn+12	equatorial polar coordinates
			 * xreturn+18	equatorial cartesian coordinates
			 */
};

struct topo_data {
  double geolon, geolat, geoalt;
  double teval;
  double tjd_ut;
  double xobs[6];
  int32 iflgkey;	/* SWI_TOPD_KEY() of the flags xobs was built for (bug 92) */
};
/* What of a request's flags the observer depends on besides the instant:
 * the ephemeris, whose delta t (its tidal acceleration) and DE number (the
 * frame bias) place the site, and the JPL Horizons offsets in its
 * nutation (UPSTREAM-BUGS 92). */
#define SWI_TOPD_KEY(iflag)  ((iflag) & (SEFLG_EPHMASK | SWI_JPLHOR_FLAGMASK))

struct sid_data {
  int32 sid_mode;
  double ayan_t0;
  double t0;
  AS_BOOL t0_is_UT;
};
/* The models swe_set_sid_mode() sets for a mode with SE_SIDBIT_PREC_ORIG
 * (its ayanamsa's precession and nutation), into models[]; nothing for any
 * other. The setter's, and swe_close()'s, which keeps the mode and resets
 * the models (UPSTREAM-BUGS 127). */
extern void swi_sid_mode_models(const struct sid_data *sip, int32 *models);

#define SWI_STAR_LENGTH 40
struct fixed_star {
  char skey[SWI_STAR_LENGTH + 2]; // may be prefixed with comma, one char more
  char starname[SWI_STAR_LENGTH + 1];
  char starbayer[SWI_STAR_LENGTH + 1];
  char starno[10];
  double epoch, ra, de, ramot, demot, radvel, parall, mag;
};

/* dpsi and deps loaded for 100 years after 1962 */
#define SWE_DATA_DPSI_DEPS  36525   

struct interpol {
  double tjd_nut0, tjd_nut2;
  double nut_dpsi0, nut_dpsi1, nut_dpsi2;
  double nut_deps0, nut_deps1, nut_deps2;
#ifndef SWE_UPSTREAM_COMPAT
  int32 nut_model;	/* astro_models[SE_MODEL_NUT] the nodes are of (bug 129) */
  int32 nut_jplhor;	/* their flags' SWI_JPLHOR_FLAGMASK bits (bug 134) */
  int32 nut_jplhora;	/* with SEFLG_JPLHOR_APPROX its mode, defaulted; else 0 (bug 134) */
#endif
};

/* The 33-term periodic series in sidtime_non_polynomial_part() (swephlib.c).
 *
 * 462 multiply-adds and 66 sin/cos, and swe_sidtime0() is the hottest
 * function in a heliacal search: swe_azalt() calls it once per coordinate
 * conversion, and one instant gets converted several times over -- for the
 * sun, the moon and the object, in altitude and again in azimuth.
 *
 * The series is a pure function of tt, so keying on tt is the whole
 * correctness argument: nothing else can make the answer differ, and the
 * memo therefore needs no invalidation anywhere. Deriving tt from tjd
 * involves delta-t, but that happens BEFORE the key is formed, so changing
 * the delta-t model changes tt and misses rather than returning a stale
 * value. One slot suffices because the repeats are back to back. What is
 * stored is the computed value, never an interpolation, so a hit is
 * bit-identical to recomputing. */
struct sidt_memo {
  double tt, dadd;
  AS_BOOL valid;
};

/* The offsets that splice the long-term sidereal time onto the IERS 2010
 * one at 1850 and 2050 (swephlib.c, sidt_lterm_offset(); UPSTREAM-BUGS
 * 25), one slot per boundary. Each is the difference of the two formulas
 * at its boundary, so it depends on everything they read: the models, and
 * delta t at the boundary (and at J2000, which upstream's
 * sidtime_long_term() read; kept in the key, harmless). All of that is the
 * key, by value, so a changed model or delta t misses; a configuration
 * generation would not do, since get_aya_correction() overrides
 * astro_models[] mid-calculation without bumping it (see struct
 * nut_memo). */
struct sidt_splice_memo {
  struct {
    double dt_b, dt_j2000, ofs;
    int32 models[SEI_NMODELS];
    AS_BOOL valid;
  } slot[2];
};

/* The precession in right ascension accumulated since J2000, the part of
 * the long-term sidereal time that is not Earth rotation (swephlib.c,
 * sidt_pra(); UPSTREAM-BUGS 29): the integral of the mean-of-date frame's
 * rotation about its pole, from the context's own precession model, at
 * nodes SWI_SIDT_PRA_STEP centuries apart on each side of J2000, with the
 * rate at each node, for a cubic Hermite polynomial between two. Built
 * outward from J2000 as far as asked, once per precession model. Keyed by
 * value on the two precession models swi_precess() reads (the only inputs
 * the integral has), two slots, for the reason given at struct nut_memo. */
#define SWI_SIDT_PRA_STEP   1.0	/* centuries */
#define SWI_SIDT_PRA_NODES  400	/* per side: +-400 centuries */
struct sidt_pra_table {
  int32 prec_long, prec_short;
  int nnode[2];		/* nodes computed, [0] before J2000, [1] after */
  double a[2][SWI_SIDT_PRA_NODES + 1];	/* arcsec */
  double w[2][SWI_SIDT_PRA_NODES + 1];	/* arcsec per century */
  AS_BOOL valid;
};
struct sidt_pra_memo {
  struct sidt_pra_table slot[2];
  int next;
};

/* Vondrak's ten periodic terms in swi_ldp_peps() (swephlib.c).
 *
 * SEMOD_PREC_DEFAULT is SEMOD_PREC_VONDRAK_2011, so swi_epsiln() lands on
 * this series for every obliquity it is asked for -- 219,000 evaluations in
 * a benchmark that made 34,000 heliacal position calls.
 *
 * Pure function of tjd, exactly like struct sidt_memo, so the key is the
 * whole correctness argument and no invalidation is needed. Four slots
 * rather than one because the callers alternate epochs and a single slot
 * would only thrash: precess_2() asks about J and J2000 in turn, and
 * app_pos_etc_plan() differences t against t + 1. */
#define SWI_LDP_PEPS_SLOTS 4
struct ldp_peps_memo {
  struct { double tjd, dpre, deps; AS_BOOL used; } slot[SWI_LDP_PEPS_SLOTS];
  int next;
};

/* calc_nutation()'s result (swephlib.c).
 *
 * The top of the heliacal profile once the two series above are memoised:
 * 39,376 calls in a benchmark making 34,000 position calls, 73% of them
 * repeats of the call immediately before. swe_sidtime() is why -- it calls
 * swi_nutation() directly rather than through swi_check_nutation(), so it
 * misses that cache entirely, and swe_azalt() calls swe_sidtime() once per
 * coordinate conversion.
 *
 * Unlike struct sidt_memo and struct ldp_peps_memo this is NOT a pure
 * function of its arguments, so the key has to carry the rest of what the
 * body reads. It is kept down to two fields by refusing the calls it cannot
 * describe rather than describing them:
 *
 *   J           the epoch.
 *   nut_model   astro_models[SE_MODEL_NUT], read by calc_nutation() and by
 *               each of the three model functions it dispatches to.
 *
 * Neither JPLHOR flag is cached, on lookup or on store, and that is what
 * lets the key stop there. SEFLG_JPLHOR reads the loaded dpsi/deps arrays
 * and eop_tjd_*, which an ephemeris-path change reloads;
 * SEFLG_JPLHOR_APPROX brings astro_models[SE_MODEL_JPLHORA_MODE] into it.
 * With both excluded, is_jplhor is necessarily FALSE and what remains reads
 * J and nut_model and nothing else. The rest of iflag drops out of the key
 * with them -- the three model functions take no iflag at all -- so
 * swe_sidtime()'s iflag of 0 shares a slot with a caller passing
 * SEFLG_SPEED, which the arithmetic ignores.
 *
 * nut_model is in the key rather than resolved by a hook on
 * swe_set_astro_models() because a COMPUTE path mutates it:
 * get_aya_correction() saves astro_models[], overrides it for the duration
 * of a calculation and restores it, deliberately without bumping the
 * configuration generation (see sweconfig.h). A generation-keyed memo would
 * hand out pre-override values inside that window.
 *
 * One slot. Measured: 73.1% hit rate, against 73.2% for four slots and
 * 78.1% for sixteen -- the reuse is all back to back, so more slots buy
 * almost nothing. */
struct nut_memo {
  double J;
  double nutlo[2];
  int32 nut_model;
  AS_BOOL valid;
};

/* The default nutation's grid (swephlib.c, calc_nutation): full IAU 2000A
 * and its first three derivatives at nodes SWI_NUT_GRID_STEP days apart,
 * cached direct-mapped by node index. The values depend on the series
 * alone, so nothing invalidates them. */
#ifndef SWI_NUT_GRID_STEP
# define SWI_NUT_GRID_STEP 1.0
#endif
#define SWI_NUT_GRID_SLOTS 512
struct nut_grid_slot {
  double k;		/* node index: the node is at k * SWI_NUT_GRID_STEP days */
  double v[8];		/* dpsi and its first three derivatives; the same for deps */
  AS_BOOL valid;
};

/* calc_deltat()'s result (swephlib.c).
 *
 * What the heliacal profile leads with once the nutation memo lands: 30% of
 * samples and 135,204 calls in a benchmark making 34,000 position calls,
 * because every swe_sidtime(), every ObjectLoc() and every ET<->UT
 * conversion asks for delta-t at an instant something else just asked about.
 *
 * The memo sits INSIDE calc_deltat(), after the tidal-acceleration
 * resolution and before the model evaluation, and that split is the point.
 * The resolution has side effects -- swi_set_tid_acc() writes ctx->tid_acc,
 * and the missing-ephemeris-path note goes to serr -- so it has to run every
 * time. What follows it is a pure function of:
 *
 *   tjd           the instant.
 *   deltat_model  astro_models[SE_MODEL_DELTAT], read here and again inside
 *                 deltat_aa().
 *   tid_acc       the RESOLVED value, not the inputs that produced it.
 *                 Keying on the answer rather than on epheflag and the DE
 *                 number means a change in how it is derived cannot slip
 *                 past the key.
 *
 * ctx->dt[], the tabulated delta-t, is the fourth input and is not in the
 * key: it has exactly two writers, swi_seed_dt_table() at context creation
 * and init_dt() on first use, and both call dt_memo_clear() instead. That
 * keeps a 220-entry table out of a comparison made 135,204 times. G18
 * (tests/dtmemo.c) is what holds them to it.
 *
 * Four slots. Measured hit rate 36% at one slot, 76.8% at two, 80.5% at
 * three and 80.6% from four on -- the callers work two or three nearby
 * epochs at a time (tjd_ut against tjd_et, t against t + 1), so one slot
 * thrashes where the earlier memos did not. */
#define SWI_DT_MEMO_SLOTS 4
struct dt_memo {
  struct { double tjd, tid_acc, deltat; int32 deltat_model; AS_BOOL used; }
    slot[SWI_DT_MEMO_SLOTS];
  int next;
};

/* Scratch state for the Moshier lunar theory (swemmoon.c).
 *
 * These 27 values were file-scope TLS statics used as IMPLICIT PARAMETERS:
 * moon1(ctx) sets them, moon2(ctx)/moon3(ctx)/moon4(ctx) read and refine them, and
 * mean_elements(ctx) fills them in first. Fourteen functions share them without
 * any of it appearing in a signature.
 *
 * Bundling them here is what lets a context own its lunar scratch, so two
 * contexts can be mid-lunar-calculation at once. The names are kept exactly
 * as they were -- l, B, T, T2, SWELP -- because they match the published
 * theory's notation, and renaming them would make the code harder to check
 * against the source it implements.
 */
struct moon_state {
  double ss[5][8], cc[5][8];
  double l;             /* Moon's ecliptic longitude */
  double B;             /* ecliptic latitude         */
  double moonpol[3];
  double SWELP, M, MP, D, NF, T, T2, T3, T4;
  double f, g, Ve, Ea, Ma, Ju, Sa, cg, sg, l1, l2, l3, l4;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 95: the same quantities' rates, d/dT (T in Julian
   * centuries), kept alongside while want_d is set (swi_moshmoon()'s
   * velocity); dang[] the rates of sscc()'s angles, rad per century */
  int want_d;
  double dl, dB, dmoonpol[3], dSWELP, dM, dMP, dD, dNF, dang[4];
  double df, dg, dVe, dEa, dMa, dJu, dSa, dcg, dsg, dl1, dl2, dl3, dl4;
#endif
};

/* Memo caches for the heliacal code (swehel.c).
 *
 * Function-local TLS statics, grouped by OWNING FUNCTION rather than
 * flattened, because the names collide: alts_last and sunra_last each
 * existed three times -- in kOZ(), ka() and Deltam(). Each was a distinct
 * variable that merely shared a name with the others, so flattening them
 * into one struct would have silently merged three separate caches into
 * one and produced wrong answers with nothing failing to compile.
 *
 * Two further groups, `calc` and `fastmag`, are gone: they belonged to the
 * #if 0 functions 3b9d090 deleted, and nothing has referenced them since.
 * Both interpolated over a five-minute window rather than matching the
 * instant, so neither could have been switched back on without moving
 * results.
 */
/* Since UPSTREAM-BUGS 82 each memo is keyed on every input of its result
 * (the fields after each first line), is empty by its `valid` flag rather
 * than by a key of 0, and each keeps what its computation wrote to serr
 * (ka()'s warning; swe_fixstar_mag()'s clearing), to write it again on a
 * hit. swi_hel_forget() empties them all. Under
 * SWE_UPSTREAM_COMPAT only each first line is used, as upstream did. */
struct hel_state {
  struct { double dmag; char star_save[AS_MAXCH];
           char serr_save[AS_MAXCH]; int valid; }          starmag;
  struct { double tjdlast, ralast; }                       sunra;
  struct { double koz_last, alts_last, sunra_last;
           double lat_last; int valid; }                   koz;
  struct { double alts_last, sunra_last, ka_last;
           double lat_last, heye_last, temps_last, rh_last, vr_last;
           int warn, valid; }                              ka;
  struct { double alts_last, alto_last, sunra_last, deltam_last;
           double lat_last, heye_last, datm_last[4];
           int32 helflag_last; int warn, valid; }          deltam;
};

/* Memo caches for sweph.c.
 *
 * Grouped per owning function for the same reason as struct hel_state:
 * the names are reused across functions and each occurrence was a distinct
 * variable. slast_starname existed FOUR times -- in swe_fixstar2,
 * swe_fixstar2_mag, swe_fixstar and swe_fixstar_mag -- last_stardata and
 * slast_stardata twice each, and the xearth/xsun light-time scratch twice.
 * Merging any of those pairs would make one lookup return another's cached
 * star, silently.
 */
struct sweph_state {
  struct { int force_flag, force_flag_checked; int32 iflag_forced; } calcflag;
  struct { int32 nutflag; }                                          nut;
  struct { double xearth[6], xearth_dt[6], xsun[6], xsun_dt[6]; }     fx_struct;
  struct { double xearth[6], xearth_dt[6], xsun[6], xsun_dt[6]; }     fx_record;
  struct { char slast_starname[AS_MAXCH]; struct fixed_star last_stardata; } fs2;
  struct { char slast_starname[AS_MAXCH]; struct fixed_star last_stardata; } fs2mag;
  struct { char slast_stardata[AS_MAXCH], slast_starname[AS_MAXCH]; } fs1;
  struct { char slast_stardata[AS_MAXCH], slast_starname[AS_MAXCH]; } fs1mag;
};

/* THE EPHEMERIS CONTEXT.
 *
 * Everything the library remembers between calls: configuration, caches,
 * open files, scratch. Historically this was `struct swe_data`, existing
 * exactly once as the global `swed`. Phase 3 (notes/PHASE3-API.md) turns it
 * into a handle callers can hold several of, so that N threads can compute
 * with N independent configurations -- which is the one thing Phase 2's
 * shared-config design deliberately cannot provide.
 *
 * `swe_data` is retained as an alias below: it appears in no public header,
 * but the rename is gratuitous churn for anyone tracking upstream.
 *
 * Renaming the type is all that happens here. `swed` is still the same
 * single TLS instance, and the legacy API still resolves to it. What
 * changes later is only how it is REACHED -- see swi_default_ctx().
 *
 * if this is changed, then also update initialisation in sweph.c */
struct swe_ctx {
  /* Phase 3d: this context's relationship to the shared config master.
   * These were TLS in sweconfig.c, which tied the tracking to the thread
   * rather than to the context -- so two contexts on one thread shared one
   * cfg_local and could not hold independent configurations, which is the
   * entire point of Phase 3. */
  swi_gen_t cfg_seen;      /* master generation this context last adopted  */
  int32     cfg_local;     /* groups this context set itself and now owns  */
  AS_BOOL   cfg_applying;  /* re-entrancy guard while a setter is applying */
  int32     cfg_hold;      /* depth of calculations synced at entry (bug 115) */
  /* bug 128: swi_config_hold() started this context, and no
   * swi_init_swed_if_start() has answered 1 for that start yet */
  AS_BOOL   start_unreported;
  AS_BOOL ephe_path_is_set;
  /* bug 103: the path was set by first use, not by a caller (implicit), and
   * the files and caches it fed are still those of that path (live: not
   * since swe_close()). */
  AS_BOOL ephe_path_implicit;
  AS_BOOL ephe_path_live;
  /* bug 118: the path setter is naming the path itself, or the context is
   * initialising before it adopts the master's: a tidal term resolved
   * meanwhile must not name the default path (and open its moon file) */
  AS_BOOL path_naming_off;
  /* May a calculation quietly finish on a weaker ephemeris than the caller
   * asked for? FALSE by default in this fork -- see swe_set_ephe_fallback().
   *
   * Upstream substitutes on its own: a missing .se1 file or a date outside
   * it drops to Moshier, a missing JPL file drops to Swiss. It notes this in
   * serr and sets the ephemeris bit in the return flag, but the return code
   * still says success, so a caller that checks only the return value gets
   * arcsecond-level analytic positions believing they came from the data
   * files. Every wrapper that discards serr -- pyswisseph among them --
   * makes the substitution completely invisible. */
  AS_BOOL ephe_fallback;
  struct jpl_save *jpl;   /* Phase 3c: the open JPL file and every scrap of
                           * state derived from its header. Opaque here;
                           * defined in swejpl.c. NULL when closed. */
  AS_BOOL jpl_file_is_open;
  FILE *fixfp;		/* fixed stars file pointer */
  char ephepath[AS_MAXCH];
  char jplfnam[AS_MAXCH];
  int32 jpldenum;
  /* The DE numbers the CONFIGURATION knows, learned at the setters' header
   * pre-opens and published with SWI_CFG_PATH, so that a context's first
   * delta-t uses the same tidal term as every later one. swi_get_tid_acc()
   * used to resolve the term from whichever files happened to be open in
   * the asking context: a context that had opened none -- every
   * swe_ctx_new() child, since the pre-open at swe_set_ephe_path_r runs on
   * the asking context only -- took SE_TIDAL_DEFAULT, then switched to the
   * file's term on its first file open, moving pre-1955 positions by
   * ~0.05 arcsec depending on call order. Measured: the Moon at JD
   * 2415020.5 answered 272.41632607 fresh and 272.41633228 after a
   * Mercury calculation on the same context. Catalogued as an upstream
   * defect in notes/UPSTREAM-BUGS.md section 14; the fix is the Closed
   * row of the same name in notes/REVIEW.md.
   * Zero means unknown: no swe_set_ephe_path()/swe_set_jpl_file() has
   * succeeded in this configuration, and the default term is then the
   * honest answer for every call, first or fiftieth. */
  int32 sweph_denum_moon;
  /* bug 166: the planet file's DE number, read at J2000 when the moon
   * file's header was not found, for the Swiss files' tidal term; 0 when
   * the moon file's was, or neither. Published with SWI_CFG_PATH, owed with
   * the moon file's after swe_close(); never set under SWE_UPSTREAM_COMPAT. */
  int32 sweph_denum_planet;
  int32 jpldenum_cfg;
  /* bug 131: the DE numbers swe_close() forgot that the kept path and JPL
   * file give (SWI_DENUM_MOON, SWI_DENUM_JPL), read again at the next
   * entry as a fresh process's setters read them; and the close they are
   * owed to, the master's generation, so that a context adopting it is
   * not taken back to it at every sync after its own reading */
  int32 denum_unread;
  swi_gen_t path_closed;
  /* bug 164: swe_set_jpl_file() has named jplfnam, found or not -- the
   * name alone does not say so: it starts as the default, and a
   * calculation's fallback to de406.eph rewrites it. Published with
   * SWI_CFG_PATH; never set under SWE_UPSTREAM_COMPAT. */
  AS_BOOL jpl_named;
  /* bug 165: the de406.eph fallback's warning when the first JPLEPH delta t
   * opened the file, for the calculation that would have opened it; ""
   * when none is owed. Under SWE_UPSTREAM_COMPAT always "". */
  char jpl_open_note[AS_MAXCH];
  /* bug 165: that first-use open failed, and nothing has changed since --
   * no open, setter, close or adopted path or file -- so a delta t does not
   * try it again; the calculation still does, at each call, as upstream */
  AS_BOOL jpl_first_missed;
  int32 last_epheflag;
  AS_BOOL geopos_is_set;
  AS_BOOL ayana_is_set;
  AS_BOOL is_old_starfile;
  double eop_tjd_beg;
  double eop_tjd_beg_horizons;
  double eop_tjd_end;
  double eop_tjd_end_add;
  /* the IERS corrections: 0 not loaded, 1 eop_1962_today.txt, 2 and
   * eop_finals.txt; ERR not found, -2 eop_1962_today.txt corrupt, -3
   * eop_finals.txt corrupt (the compat build), SWI_EOP_NOMEM (bug 172);
   * SWI_EOP_FINALS_CUT (bug 171) eop_finals.txt broken off, the days
   * before the gap kept */
  int eop_dpsi_loaded;
  double tid_acc;
  AS_BOOL is_tid_acc_manual;
  AS_BOOL init_dt_done;
  AS_BOOL swed_is_initialised;
  AS_BOOL delta_t_userdef_is_set;
  double delta_t_userdef;
  double ast_G;
  double ast_H;
  double ast_diam;
  char astelem[AS_MAXCH * 10];
  int i_saved_planet_name;
  char saved_planet_name[80];
  //double dpsi[36525];  /* works for 100 years after 1962 */
  //double deps[36525];
  double *dpsi;
  double *deps;
  int32 timeout;
  int32 astro_models[SEI_NMODELS];
  /* Atmospheric lapse rate for refraction, set by swe_set_lapse_rate().
   * Was a file-scope static in swecl.c; moved here because it is
   * configuration and Phase 2 needs every config field in one place.
   * (Historical note: swed's initialiser in sweph.c used to be positional
   * and required this field to sit immediately after astro_models, or its
   * SE_LAPSE_RATE default would have been silently zeroed. C17_MIGRATION.md
   * Phase 2 replaced it with a designated initializer, which removed that
   * ordering constraint -- this field's position is no longer load-bearing.) */
  double const_lapse_rate;
  AS_BOOL do_interpolate_nut;
  struct interpol interpol;
  struct sidt_memo sidt_np;	/* see struct sidt_memo */
  struct sidt_splice_memo sidt_splice;	/* see struct sidt_splice_memo */
  struct sidt_pra_memo sidt_pra;	/* see struct sidt_pra_memo */
  struct ldp_peps_memo ldp_peps;	/* see struct ldp_peps_memo */
  struct nut_memo nut_np;	/* see struct nut_memo */
  struct nut_grid_slot nut_grid[SWI_NUT_GRID_SLOTS];
  /* the last TDB - TT, swi_tdb_minus_tt(): a function of the instant
   * alone, so it can never go stale; the reads of one calculation come in
   * groups at one instant (Sun, Moon and EMB at t, then at t - tau); with
   * its rate, swi_tdb_minus_tt_rate(), and the instant before in tdb_np2
   * (UPSTREAM-BUGS 73) */
  struct swi_tdb_memo { double tjd, val, rate; AS_BOOL valid, rvalid; } tdb_np, tdb_np2;
  struct dt_memo dt_np;		/* see struct dt_memo */
  struct file_data fidat[SEI_NEPHFILES];
  struct gen_const gcdat;
  struct plan_data pldat[SEI_NPLANETS];
#ifndef SWE_UPSTREAM_COMPAT
  /* sepl's, semo's and seas's file before the open one, still open, and
   * its bodies' file data, the part of pldat before teval (UPSTREAM-BUGS
   * 89) */
  struct file_data fidat_spare[SEI_FILE_MAIN_AST + 1];
  struct plan_data pldat_spare[SEI_NPLANETS];
  /* the geocentric Moon (equatorial J2000, AU, AU/day) that the saved Earth
   * was made with, on JPL and Moshier; t is its instant, 0 for none
   * (UPSTREAM-BUGS 66) */
  struct { double t, x[6]; int32 iephe; } earth_moon;
  /* a Swiss read in this call took Moshier's Moon for want of the semo
   * file (the Moon itself, or the Earth made from it); and the instant of
   * a saved Earth made so (UPSTREAM-BUGS 120) */
  AS_BOOL moshmoon_used;
  double moshmoon_earth_t;
  /* the Sun's barycentric place, velocity and acceleration from Moshier's
   * planets at instant t, for each planet set, and the planets' series at
   * their own instants tp (UPSTREAM-BUGS 102, 109), swi_mosh_sun_bary()'s
   * memo */
  struct {
    struct swi_mosh_sun { double t, x[6], a[3]; } s[2];
    double tp[9], xp[9][6];
  } mosh_sun_bary;
#endif
  /* nddat is deliberately the heavier plan_data, not node_data.
   *
   * Every access does touch only node_data's five members, so the leaner
   * struct would fit -- but app_pos_rest() takes a struct plan_data * and
   * is called with both pldat[] and nddat[] entries, so narrowing this
   * means refactoring a hot shared helper to save 912 B out of a 34,800 B
   * context. Kept as plan_data on purpose; see notes/C17_PERFORMANCE.md
   * section 4.2. */
  struct plan_data nddat[SEI_NNODE_ETC];
  struct save_positions savedat[SE_NPLANETS+1];
  struct epsilon oec;
  struct epsilon oec2000;
  struct nut nut;
  struct nut nut2000;
  struct nut nutv;
  struct topo_data topd;
  struct sid_data sidd;
  /* Were AS_BOOL until C17_MIGRATION.md Phase 2 -- a pre-existing mistype,
   * harmless for decades because AS_BOOL and int32 were both plain `int`.
   * Made AS_BOOL a real bool (Phase 2, stdbool.h) and every fixed-star
   * lookup broke: load_all_fixed_stars(ctx) correctly counted e.g. 1141 stars,
   * but storing 1141 into a _Bool truncates to 1, so every subsequent
   * bsearch() ran over 1 record instead of the real array. Caught by
   * tests/golden's fixed-star rows, not by inspection. */
  int32 n_fixstars_real;     // real number of fixed stars in sefstars.txt
  int32 n_fixstars_named;    // number of fixed stars with tradtional name
  int32 n_fixstars_records;  // number of fixed stars records in fixed_stars
  struct fixed_star *fixed_stars;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 121: a load of sefstars.txt that failed, and its error,
   * kept until the path changes or swe_close() (swi_fixstar_forget()) */
  int32 fixstars_load_failed;
  char fixstars_load_err[AS_MAXCH];
#endif
  /* seorbel.txt, read once per context. The text, not parsed numbers: the
   * elements carry T terms evaluated against tjd on every call. Dropped
   * whenever the ephemeris path can change. NULL until first use, and left
   * NULL when the file is absent so the built-in table is used as before. */
  struct fict_line *fict_lines;
  int32 n_fict_lines;
  /* Moshier lunar theory scratch -- see struct moon_state above. */
  struct moon_state moon;

  /* --- Phase 3c: former file-scope TLS statics ---------------------- */

  /* swemplan.c: sin/cos of multiple angles, rebuilt per call by sscc().
   * Same role as moon.ss/cc but a different theory and a different shape,
   * so deliberately not shared with it. */
  double plan_ss[9][24], plan_cc[9][24];

  /* swehouse.c: last solar declination handed to swe_houses_armc_ex2(),
   * recalled when a caller passes the sentinel 99. 99 means "none yet", so
   * zero would be a real declination and wrong. Set in swed's designated
   * initialiser (sweph.c) and in ctx_init_defaults(), and kept across
   * swi_init_swed_if_start()'s memset, as upstream's static is (bug 133). */
  double saved_sundec;

  /* swehel.c memo caches -- see struct hel_state above. */
  struct hel_state hel;

  /* sweph.c memo caches -- see struct sweph_state above. */
  struct sweph_state sp;

  /* swedate.c: leap-second table. The built-in values live in a shared
   * const (leap_seconds_builtin); this is the working copy, because
   * init_leapsec() extends it from seleapsec.txt. NLEAP_SECONDS_SPACE is
   * 100 -- kept literal here because the macro is private to swedate.c. */
  /* swephlib.c: Delta-T working copy, seeded from dt_builtin and extended
   * from swe_deltat.txt. TABSIZ_SPACE is 509; literal here because the
   * macro is private to swephlib.c. */
  double  dt[509];

  int     leap_seconds[100];
  AS_BOOL leapsec_done;
};

/* swe_ctx is forward-declared near the top of this header, next to
 * SEI_NMODELS, because the swi_* prototypes above need to name it.
 *
 * Historical name. Nothing public uses it, but keeping it spares anyone
 * tracking upstream a rename they did not ask for. */
#define swe_data swe_ctx

/* The default context: the one the legacy, contextless API resolves to.
 *
 * Still the same TLS instance it has always been. Phase 3d makes the legacy
 * shims call swi_default_ctx() instead of naming `swed` directly, at which
 * point this becomes a process-wide context kept in sync by sweconfig.c --
 * preserving Phase 2's semantics for every caller who never migrates.
 * See notes/PHASE3-API.md section 4. */
extern TLS struct swe_ctx swed;

/* Returns the default context. Exists from the start so call sites can be
 * converted incrementally: a function that has been given a `ctx` parameter
 * is called with swi_default_ctx() until its callers have one of their own. */
extern swe_ctx *swi_default_ctx(void);

#ifndef SWE_UPSTREAM_COMPAT
/* Empty the heliacal memos (struct hel_state; swehel.c). swe_close(), a
 * new ephemeris path and a new context call it: the star magnitude's comes
 * from the catalogue on that path (UPSTREAM-BUGS 82). */
extern void swi_hel_forget(swe_ctx *ctx);
/* Empty the fixed-star memos and the star catalogue read into memory
 * (sweph.c), and close the star file: the catalogue is the path's
 * (UPSTREAM-BUGS 92). Called where swi_hel_forget() is. */
extern void swi_fixstar_forget(swe_ctx *ctx);
/* Forget what files another thread's setter closed on adoption: the
 * positions, and with swiss_closed the Swiss files' bodies' data too
 * (sweph.c; UPSTREAM-BUGS 119). */
extern void swi_adopted_files_forget(swe_ctx *ctx, AS_BOOL swiss_closed);
/* The path first use sets when none has been, local and marked implicit
 * (UPSTREAM-BUGS 103). */
extern void swi_set_ephe_path_implicit(swe_ctx *ctx);
/* Name that path, without closing anything, at the first file or moon
 * tidal term a context takes from it (UPSTREAM-BUGS 107). */
extern void swi_ephe_path_name_first_use(swe_ctx *ctx);
/* No path in effect: none set or adopted, no SE_EPHE_PATH, no file along
 * the default path -- when a first call's "Please call
 * swe_set_ephe_path()" warning is due (UPSTREAM-BUGS 110). */
extern AS_BOOL swi_ephe_path_none(swe_ctx *ctx);
/* The DE numbers swe_close() forgot, read again along the kept path, as
 * the setters read them (UPSTREAM-BUGS 131). */
extern void swi_denum_reread(swe_ctx *ctx);
/* The JPL file a JPLEPH calculation opens, opened by the first JPLEPH
 * tidal term that knows no DE number for it (UPSTREAM-BUGS 165). */
extern void swi_jpl_open_first_use(swe_ctx *ctx, char *serr);
#endif
/* The IERS corrections of an adopted JPL file, loaded as its setter loads
 * them (UPSTREAM-BUGS 143; both builds, the adoption being this fork's) */
extern void swi_dpsi_deps_adopt(swe_ctx *ctx);
/* What denum_unread names (bug 131; never set under SWE_UPSTREAM_COMPAT) */
#define SWI_DENUM_MOON 1	/* the moon file's header, along the path */
#define SWI_DENUM_JPL  2	/* the JPL file swe_set_jpl_file() read */

/* Shared configuration layer. Included here, after struct sid_data and
 * SEI_NMODELS, because struct swe_config embeds both. */
#include "sweconfig.h"

extern AS_BOOL swi_config_begin_apply(swe_ctx *ctx);
extern void    swi_config_end_apply(swe_ctx *ctx, AS_BOOL was);
