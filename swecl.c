/* SWISSEPH 

    Ephemeris computations
    Author: Dieter Koch

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

#include "swejpl.h"
#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"
#include <time.h>

#define SEFLG_EPHMASK	(SEFLG_JPLEPH|SEFLG_SWIEPH|SEFLG_MOSEPH)
static int find_maximum(double y00, double y11, double y2, double dx, 
			double *dxret, double *yret);
static int find_zero(double y00, double y11, double y2, double dx, 
			double *dxret, double *dxret2);
static double calc_dip(double geoalt, double atpress, double attemp, double lapse_rate);
static double calc_astronomical_refr(double geoalt,double atpress, double attemp);

#if 0
#define DSUN 	(1391978489.9 / AUNIT)	/* this value is consistent with
					 * 959.63 arcsec at AU distance (Astr. Alm.) */
#else
#define DSUN 	(1392000000.0 / AUNIT)
#endif
#define DMOON 	(3476300.0 / AUNIT) 
#define DEARTH  (6378140.0 * 2 / AUNIT)
#define RSUN	(DSUN / 2)
#define RMOON	(DMOON / 2)
#define REARTH	(DEARTH / 2)
/*#define SEI_OCC_FAST    (16 * 1024L)*/
static int32 eclipse_where( swe_ctx *ctx, double tjd_ut, int32 ipl, char *starname, int32 ifl, double *geopos, 
	double *dcore, char *serr);
static int32 eclipse_how( swe_ctx *ctx, double tjd_ut, int32 ipl, char *starname, int32 ifl,
        double geolon, double geolat, double geohgt, 
	double *attr, char *serr);
/* backward is int32, not AS_BOOL: swe_lun_occult_when_loc() callers OR
 * SE_ECL_ONE_TRY into it (see the definitions below), so it is a bitmask,
 * not a strict boolean. These forward declarations said AS_BOOL until
 * C17_MIGRATION.md Phase 2 -- harmless while AS_BOOL was plain `int`,
 * a compile error once it became a real bool, because the definitions
 * below (the actual truth) never agreed with it. */
static int32 eclipse_when_loc(swe_ctx *ctx, double tjd_start, int32 ifl, double *geopos,
	double *tret, double *attr, int32 backward, char *serr);
static int32 occult_when_loc(swe_ctx *ctx, double tjd_start, int32 ipl, char *starname, int32 ifl,
        double *geopos, double *tret, double *attr, int32 backward, char *serr);
static int32 lun_eclipse_how(swe_ctx *ctx, double tjd_ut, int32 ifl, double *attr, 
        double *dcore, char *serr);
static int32 calc_mer_trans(swe_ctx *ctx, 
               double tjd_ut, int32 ipl, int32 epheflag, int32 rsmi,
               double *geopos,
               char *starname, 
               double *tret,
               char *serr); 
static int32 calc_planet_star(swe_ctx *ctx, double tjd_et, int32 ipl, char *starname, int32 iflag, double *x, char *serr);

struct saros_data {int series_no; double tstart;};

// Saros cycle numbers of solar eclipses with date of initial 
// eclipse. Table was derived from the table of the Nasa Eclipse Web Site:
// https://eclipse.gsfc.nasa.gov/SEsaros/SEsaros0-180.html
// Note, for eclipse dates =< 15 Feb -1604 and eclipse dates
// >= 2 Sep 2666, Saros cycle numbers cannot always be given.
#define SAROS_CYCLE  6585.3213
#define NSAROS_SOLAR 181
const struct saros_data saros_data_solar[NSAROS_SOLAR] = {
{0, 641886.5}, /* 23 May -2955 */
{1, 672214.5}, /* 04 Jun -2872 */
{2, 676200.5}, /* 04 May -2861 */
{3, 693357.5}, /* 24 Apr -2814 */
{4, 723685.5}, /* 06 May -2731 */
{5, 727671.5}, /* 04 Apr -2720 */
{6, 744829.5}, /* 27 Mar -2673 */
{7, 775157.5}, /* 08 Apr -2590 */
{8, 779143.5}, /* 07 Mar -2579 */
{9, 783131.5}, /* 06 Feb -2568 */
{10, 820044.5}, /* 28 Feb -2467 */
{11, 810859.5}, /* 06 Jan -2492 */
{12, 748993.5}, /* 20 Aug -2662 */
{13, 792492.5}, /* 23 Sep -2543 */
{14, 789892.5}, /* 11 Aug -2550 */
{15, 787294.5}, /* 01 Jul -2557 */
{16, 824207.5}, /* 23 Jul -2456 */
{17, 834779.5}, /* 03 Jul -2427 */
{18, 838766.5}, /* 02 Jun -2416 */
{19, 869094.5}, /* 15 Jun -2333 */
{20, 886251.5}, /* 05 Jun -2286 */
{21, 890238.5}, /* 05 May -2275 */
{22, 927151.5}, /* 28 May -2174 */
{23, 937722.5}, /* 07 May -2145 */
{24, 941709.5}, /* 06 Apr -2134 */
{25, 978623.5}, /* 30 Apr -2033 */
{26, 989194.5}, /* 08 Apr -2004 */
{27, 993181.5}, /* 09 Mar -1993 */
{28, 1023510.5}, /* 22 Mar -1910 */
{29, 1034081.5}, /* 01 Mar -1881 */
{30, 972214.5}, /* 12 Oct -2051 */
{31, 1061811.5}, /* 31 Jan -1805 */
{32, 1006529.5}, /* 24 Sep -1957 */
{33, 997345.5}, /* 02 Aug -1982 */
{34, 1021088.5}, /* 04 Aug -1917 */
{35, 1038245.5}, /* 25 Jul -1870 */
{36, 1042231.5}, /* 23 Jun -1859 */
{37, 1065974.5}, /* 25 Jun -1794 */
{38, 1089716.5}, /* 26 Jun -1729 */
{39, 1093703.5}, /* 26 May -1718 */
{40, 1117446.5}, /* 28 May -1653 */
{41, 1141188.5}, /* 28 May -1588 */
{42, 1145175.5}, /* 28 Apr -1577 */
{43, 1168918.5}, /* 29 Apr -1512 */
{44, 1192660.5}, /* 30 Apr -1447 */
{45, 1196647.5}, /* 30 Mar -1436 */
{46, 1220390.5}, /* 01 Apr -1371 */
{47, 1244132.5}, /* 02 Apr -1306 */
{48, 1234948.5}, /* 08 Feb -1331 */
{49, 1265277.5}, /* 22 Feb -1248 */
{50, 1282433.5}, /* 11 Feb -1201 */
{51, 1207395.5}, /* 02 Sep -1407 */
{52, 1217968.5}, /* 14 Aug -1378 */
{53, 1254881.5}, /* 06 Sep -1277 */
{54, 1252282.5}, /* 25 Jul -1284 */
{55, 1262855.5}, /* 06 Jul -1255 */
{56, 1293182.5}, /* 17 Jul -1172 */
{57, 1297169.5}, /* 17 Jun -1161 */
{58, 1314326.5}, /* 07 Jun -1114 */
{59, 1344654.5}, /* 19 Jun -1031 */
{60, 1348640.5}, /* 18 May -1020 */
{61, 1365798.5}, /* 10 May -0973 */
{62, 1396126.5}, /* 22 May -0890 */
{63, 1400112.5}, /* 20 Apr -0879 */
{64, 1417270.5}, /* 11 Apr -0832 */
{65, 1447598.5}, /* 24 Apr -0749 */
{66, 1444999.5}, /* 12 Mar -0756 */
{67, 1462157.5}, /* 04 Mar -0709 */
{68, 1492485.5}, /* 16 Mar -0626 */
{69, 1456959.5}, /* 09 Dec -0724 */
{70, 1421434.5}, /* 05 Sep -0821 */
{71, 1471518.5}, /* 19 Oct -0684 */
{72, 1455748.5}, /* 16 Aug -0727 */
{73, 1466320.5}, /* 27 Jul -0698 */
{74, 1496648.5}, /* 08 Aug -0615 */
{75, 1500634.5}, /* 07 Jul -0604 */
{76, 1511207.5}, /* 18 Jun -0575 */
{77, 1548120.5}, /* 11 Jul -0474 */
{78, 1552106.5}, /* 09 Jun -0463 */
{79, 1562679.5}, /* 21 May -0434 */
{80, 1599592.5}, /* 13 Jun -0333 */
{81, 1603578.5}, /* 12 May -0322 */
{82, 1614150.5}, /* 22 Apr -0293 */
{83, 1644479.5}, /* 05 May -0210 */
{84, 1655050.5}, /* 14 Apr -0181 */
{85, 1659037.5}, /* 14 Mar -0170 */
{86, 1695950.5}, /* 06 Apr -0069 */
{87, 1693351.5}, /* 23 Feb -0076 */
{88, 1631484.5}, /* 06 Oct -0246 */
{89, 1727666.5}, /* 04 Feb 0018 */
{90, 1672384.5}, /* 28 Sep -0134 */
{91, 1663200.5}, /* 06 Aug -0159 */
{92, 1693529.5}, /* 19 Aug -0076 */
{93, 1710685.5}, /* 09 Aug -0029 */
{94, 1714672.5}, /* 09 Jul -0018 */
{95, 1738415.5}, /* 11 Jul 0047 */
{96, 1755572.5}, /* 01 Jul 0094 */
{97, 1766144.5}, /* 11 Jun 0123 */
{98, 1789887.5}, /* 12 Jun 0188 */
{99, 1807044.5}, /* 03 Jun 0235 */
{100, 1817616.5}, /* 13 May 0264 */
{101, 1841359.5}, /* 15 May 0329 */
{102, 1858516.5}, /* 05 May 0376 */
{103, 1862502.5}, /* 04 Apr 0387 */
{104, 1892831.5}, /* 17 Apr 0470 */
{105, 1903402.5}, /* 27 Mar 0499 */
{106, 1887633.5}, /* 23 Jan 0456 */
{107, 1924547.5}, /* 15 Feb 0557 */
{108, 1921948.5}, /* 04 Jan 0550 */
{109, 1873251.5}, /* 07 Sep 0416 */
{110, 1890409.5}, /* 30 Aug 0463 */
{111, 1914151.5}, /* 30 Aug 0528 */
{112, 1918138.5}, /* 31 Jul 0539 */
{113, 1935296.5}, /* 22 Jul 0586 */
{114, 1959038.5}, /* 23 Jul 0651 */
{115, 1963024.5}, /* 21 Jun 0662 */
{116, 1986767.5}, /* 23 Jun 0727 */
{117, 2010510.5}, /* 24 Jun 0792 */
{118, 2014496.5}, /* 24 May 0803 */
{119, 2031654.5}, /* 15 May 0850 */
{120, 2061982.5}, /* 27 May 0933 */
{121, 2065968.5}, /* 25 Apr 0944 */
{122, 2083126.5}, /* 17 Apr 0991 */
{123, 2113454.5}, /* 29 Apr 1074 */
{124, 2104269.5}, /* 06 Mar 1049 */
{125, 2108256.5}, /* 04 Feb 1060 */
{126, 2151755.5}, /* 10 Mar 1179 */
{127, 2083302.5}, /* 10 Oct 0991 */
{128, 2080704.5}, /* 29 Aug 0984 */
{129, 2124203.5}, /* 03 Oct 1103 */
{130, 2121603.5}, /* 20 Aug 1096 */
{131, 2132176.5}, /* 01 Aug 1125 */
{132, 2162504.5}, /* 13 Aug 1208 */
{133, 2166490.5}, /* 13 Jul 1219 */
{134, 2177062.5}, /* 22 Jun 1248 */
{135, 2207390.5}, /* 05 Jul 1331 */
{136, 2217962.5}, /* 14 Jun 1360 */
{137, 2228534.5}, /* 25 May 1389 */
{138, 2258862.5}, /* 06 Jun 1472 */
{139, 2269434.5}, /* 17 May 1501 */
{140, 2273421.5}, /* 16 Apr 1512 */
{141, 2310334.5}, /* 19 May 1613 */
{142, 2314320.5}, /* 17 Apr 1624 */
{143, 2311722.5}, /* 07 Mar 1617 */
{144, 2355221.5}, /* 11 Apr 1736 */
{145, 2319695.5}, /* 04 Jan 1639 */
{146, 2284169.5}, /* 19 Sep 1541 */
{147, 2314498.5}, /* 12 Oct 1624 */
{148, 2325069.5}, /* 21 Sep 1653 */
{149, 2329056.5}, /* 21 Aug 1664 */
{150, 2352799.5}, /* 24 Aug 1729 */
{151, 2369956.5}, /* 14 Aug 1776 */
{152, 2380528.5}, /* 26 Jul 1805 */
{153, 2404271.5}, /* 28 Jul 1870 */
{154, 2421428.5}, /* 19 Jul 1917 */
{155, 2425414.5}, /* 17 Jun 1928 */
{156, 2455743.5}, /* 01 Jul 2011 */
{157, 2472900.5}, /* 21 Jun 2058 */
{158, 2476886.5}, /* 20 May 2069 */
{159, 2500629.5}, /* 23 May 2134 */
{160, 2517786.5}, /* 13 May 2181 */
{161, 2515187.5}, /* 01 Apr 2174 */
{162, 2545516.5}, /* 15 Apr 2257 */
{163, 2556087.5}, /* 25 Mar 2286 */
{164, 2487635.5}, /* 24 Oct 2098 */
{165, 2504793.5}, /* 16 Oct 2145 */
{166, 2535121.5}, /* 29 Oct 2228 */
{167, 2525936.5}, /* 06 Sep 2203 */
{168, 2543094.5}, /* 28 Aug 2250 */
{169, 2573422.5}, /* 10 Sep 2333 */
{170, 2577408.5}, /* 09 Aug 2344 */
{171, 2594566.5}, /* 01 Aug 2391 */
{172, 2624894.5}, /* 13 Aug 2474 */
{173, 2628880.5}, /* 12 Jul 2485 */
{174, 2646038.5}, /* 04 Jul 2532 */
{175, 2669780.5}, /* 05 Jul 2597 */
{176, 2673766.5}, /* 04 Jun 2608 */
{177, 2690924.5}, /* 27 May 2655 */
{178, 2721252.5}, /* 09 Jun 2738 */
{179, 2718653.5}, /* 28 Apr 2731 */
{180, 2729226.5}, /* 08 Apr 2760 */
};

// Saros cycle numbers of lunar eclipses with date of initial 
// eclipse. Table was derived from the table of the Nasa Eclipse Web Site:
// https://eclipse.gsfc.nasa.gov/LEsaros/LEsaroscat.html
// Note, for eclipse dates =< 29 April -1337 and eclipse dates
// >= 10 Aug 2892, Saros cycle numbers cannot always be given.
#define NSAROS_LUNAR 180
const struct saros_data saros_data_lunar[NSAROS_LUNAR] = {
{1, 782437.5}, /* 14 Mar -2570 */
{2, 799593.5}, /* 03 Mar -2523 */
{3, 783824.5}, /* 30 Dec -2567 */
{4, 754884.5}, /* 06 Oct -2646 */
{5, 824724.5}, /* 22 Dec -2455 */
{6, 762857.5}, /* 04 Aug -2624 */
{7, 773430.5}, /* 16 Jul -2595 */
{8, 810343.5}, /* 08 Aug -2494 */
{9, 807743.5}, /* 26 Jun -2501 */
{10, 824901.5}, /* 17 Jun -2454 */
{11, 855229.5}, /* 29 Jun -2371 */
{12, 859215.5}, /* 28 May -2360 */
{13, 876373.5}, /* 20 May -2313 */
{14, 906701.5}, /* 01 Jun -2230 */
{15, 910687.5}, /* 30 Apr -2219 */
{16, 927845.5}, /* 21 Apr -2172 */
{17, 958173.5}, /* 04 May -2089 */
{18, 962159.5}, /* 02 Apr -2078 */
{19, 979317.5}, /* 24 Mar -2031 */
{20, 1009645.5}, /* 05 Apr -1948 */
{21, 1007046.5}, /* 22 Feb -1955 */
{22, 1017618.5}, /* 02 Feb -1926 */
{23, 1054531.5}, /* 25 Feb -1825 */
{24, 979493.5}, /* 16 Sep -2031 */
{25, 976895.5}, /* 06 Aug -2038 */
{26, 1020394.5}, /* 09 Sep -1919 */
{27, 1017794.5}, /* 28 Jul -1926 */
{28, 1028367.5}, /* 09 Jul -1897 */
{29, 1058695.5}, /* 21 Jul -1814 */
{30, 1062681.5}, /* 19 Jun -1803 */
{31, 1073253.5}, /* 30 May -1774 */
{32, 1110167.5}, /* 23 Jun -1673 */
{33, 1114153.5}, /* 22 May -1662 */
{34, 1131311.5}, /* 13 May -1615 */
{35, 1161639.5}, /* 25 May -1532 */
{36, 1165625.5}, /* 24 Apr -1521 */
{37, 1176197.5}, /* 03 Apr -1492 */
{38, 1213111.5}, /* 27 Apr -1391 */
{39, 1217097.5}, /* 26 Mar -1380 */
{40, 1221084.5}, /* 24 Feb -1369 */
{41, 1257997.5}, /* 18 Mar -1268 */
{42, 1255398.5}, /* 04 Feb -1275 */
{43, 1186946.5}, /* 07 Sep -1463 */
{44, 1283128.5}, /* 06 Jan -1199 */
{45, 1227845.5}, /* 29 Aug -1351 */
{46, 1225247.5}, /* 19 Jul -1358 */
{47, 1255575.5}, /* 31 Jul -1275 */
{48, 1272732.5}, /* 21 Jul -1228 */
{49, 1276719.5}, /* 21 Jun -1217 */
{50, 1307047.5}, /* 03 Jul -1134 */
{51, 1317619.5}, /* 13 Jun -1105 */
{52, 1328191.5}, /* 23 May -1076 */
{53, 1358519.5}, /* 05 Jun -0993 */
{54, 1375676.5}, /* 26 May -0946 */
{55, 1379663.5}, /* 25 Apr -0935 */
{56, 1409991.5}, /* 07 May -0852 */
{57, 1420562.5}, /* 16 Apr -0823 */
{58, 1424549.5}, /* 16 Mar -0812 */
{59, 1461463.5}, /* 09 Apr -0711 */
{60, 1465449.5}, /* 08 Mar -0700 */
{61, 1436509.5}, /* 13 Dec -0780 */
{62, 1493179.5}, /* 08 Feb -0624 */
{63, 1457653.5}, /* 03 Nov -0722 */
{64, 1435298.5}, /* 20 Aug -0783 */
{65, 1452456.5}, /* 11 Aug -0736 */
{66, 1476198.5}, /* 12 Aug -0671 */
{67, 1480184.5}, /* 11 Jul -0660 */
{68, 1503928.5}, /* 14 Jul -0595 */
{69, 1527670.5}, /* 15 Jul -0530 */
{70, 1531656.5}, /* 13 Jun -0519 */
{71, 1548814.5}, /* 04 Jun -0472 */
{72, 1579142.5}, /* 17 Jun -0389 */
{73, 1583128.5}, /* 16 May -0378 */
{74, 1600286.5}, /* 07 May -0331 */
{75, 1624028.5}, /* 08 May -0266 */
{76, 1628015.5}, /* 07 Apr -0255 */
{77, 1651758.5}, /* 09 Apr -0190 */
{78, 1675500.5}, /* 10 Apr -0125 */
{79, 1672901.5}, /* 27 Feb -0132 */
{80, 1683474.5}, /* 07 Feb -0103 */
{81, 1713801.5}, /* 19 Feb -0020 */
{82, 1645349.5}, /* 21 Sep -0208 */
{83, 1649336.5}, /* 22 Aug -0197 */
{84, 1686249.5}, /* 13 Sep -0096 */
{85, 1683650.5}, /* 02 Aug -0103 */
{86, 1694222.5}, /* 13 Jul -0074 */
{87, 1731136.5}, /* 06 Aug 0027 */
{88, 1735122.5}, /* 05 Jul 0038 */
{89, 1745694.5}, /* 15 Jun 0067 */
{90, 1776022.5}, /* 27 Jun 0150 */
{91, 1786594.5}, /* 07 Jun 0179 */
{92, 1797166.5}, /* 17 May 0208 */
{93, 1827494.5}, /* 30 May 0291 */
{94, 1838066.5}, /* 09 May 0320 */
{95, 1848638.5}, /* 19 Apr 0349 */
{96, 1878966.5}, /* 01 May 0432 */
{97, 1882952.5}, /* 31 Mar 0443 */
{98, 1880354.5}, /* 18 Feb 0436 */
{99, 1923853.5}, /* 24 Mar 0555 */
{100, 1881741.5}, /* 06 Dec 0439 */
{101, 1852801.5}, /* 11 Sep 0360 */
{102, 1889715.5}, /* 05 Oct 0461 */
{103, 1893701.5}, /* 03 Sep 0472 */
{104, 1897688.5}, /* 04 Aug 0483 */
{105, 1928016.5}, /* 16 Aug 0566 */
{106, 1938588.5}, /* 27 Jul 0595 */
{107, 1942575.5}, /* 26 Jun 0606 */
{108, 1972903.5}, /* 08 Jul 0689 */
{109, 1990059.5}, /* 27 Jun 0736 */
{110, 1994046.5}, /* 28 May 0747 */
{111, 2024375.5}, /* 10 Jun 0830 */
{112, 2034946.5}, /* 20 May 0859 */
{113, 2045518.5}, /* 29 Apr 0888 */
{114, 2075847.5}, /* 13 May 0971 */
{115, 2086418.5}, /* 21 Apr 1000 */
{116, 2083820.5}, /* 11 Mar 0993 */
{117, 2120733.5}, /* 03 Apr 1094 */
{118, 2124719.5}, /* 02 Mar 1105 */
{119, 2062852.5}, /* 14 Oct 0935 */
{120, 2086596.5}, /* 16 Oct 1000 */
{121, 2103752.5}, /* 06 Oct 1047 */
{122, 2094568.5}, /* 14 Aug 1022 */
{123, 2118311.5}, /* 16 Aug 1087 */
{124, 2142054.5}, /* 17 Aug 1152 */
{125, 2146040.5}, /* 17 Jul 1163 */
{126, 2169783.5}, /* 18 Jul 1228 */
{127, 2186940.5}, /* 09 Jul 1275 */
{128, 2197512.5}, /* 18 Jun 1304 */
{129, 2214670.5}, /* 10 Jun 1351 */
{130, 2238412.5}, /* 10 Jun 1416 */
{131, 2242398.5}, /* 10 May 1427 */
{132, 2266142.5}, /* 12 May 1492 */
{133, 2289884.5}, /* 13 May 1557 */
{134, 2287285.5}, /* 01 Apr 1550 */
{135, 2311028.5}, /* 13 Apr 1615 */
{136, 2334770.5}, /* 13 Apr 1680 */
{137, 2292659.5}, /* 17 Dec 1564 */
{138, 2276890.5}, /* 15 Oct 1521 */
{139, 2326974.5}, /* 09 Dec 1658 */
{140, 2304619.5}, /* 25 Sep 1597 */
{141, 2308606.5}, /* 25 Aug 1608 */
{142, 2345520.5}, /* 19 Sep 1709 */
{143, 2349506.5}, /* 18 Aug 1720 */
{144, 2360078.5}, /* 29 Jul 1749 */
{145, 2390406.5}, /* 11 Aug 1832 */
{146, 2394392.5}, /* 11 Jul 1843 */
{147, 2411550.5}, /* 02 Jul 1890 */
{148, 2441878.5}, /* 15 Jul 1973 */
{149, 2445864.5}, /* 13 Jun 1984 */
{150, 2456437.5}, /* 25 May 2013 */
{151, 2486765.5}, /* 06 Jun 2096 */
{152, 2490751.5}, /* 07 May 2107 */
{153, 2501323.5}, /* 16 Apr 2136 */
{154, 2538236.5}, /* 10 May 2237 */
{155, 2529052.5}, /* 18 Mar 2212 */
{156, 2473771.5}, /* 08 Nov 2060 */
{157, 2563367.5}, /* 01 Mar 2306 */
{158, 2508085.5}, /* 21 Oct 2154 */
{159, 2505486.5}, /* 09 Sep 2147 */
{160, 2542400.5}, /* 03 Oct 2248 */
{161, 2546386.5}, /* 02 Sep 2259 */
{162, 2556958.5}, /* 12 Aug 2288 */
{163, 2587287.5}, /* 27 Aug 2371 */
{164, 2597858.5}, /* 05 Aug 2400 */
{165, 2601845.5}, /* 06 Jul 2411 */
{166, 2632173.5}, /* 18 Jul 2494 */
{167, 2649330.5}, /* 09 Jul 2541 */
{168, 2653317.5}, /* 08 Jun 2552 */
{169, 2683645.5}, /* 22 Jun 2635 */
{170, 2694217.5}, /* 01 Jun 2664 */
{171, 2698203.5}, /* 01 May 2675 */
{172, 2728532.5}, /* 15 May 2758 */
{173, 2739103.5}, /* 24 Apr 2787 */
{174, 2683822.5}, /* 16 Dec 2635 */
{175, 2740492.5}, /* 11 Feb 2791 */
{176, 2724722.5}, /* 09 Dec 2747 */
{177, 2708952.5}, /* 05 Oct 2704 */
{178, 2732695.5}, /* 07 Oct 2769 */
{179, 2749852.5}, /* 27 Sep 2816 */
{180, 2753839.5}, /* 28 Aug 2827 */
};

/* Computes geographic location and type of solar eclipse 
 * for a given tjd 
 * iflag:	to indicate ephemeris to be used
 *			(SEFLG_JPLEPH, SEFLG_SWIEPH, SEFLG_MOSEPH)
 *
 * Algorithms for the central line is taken from Montenbruck, pp. 179ff.,
 * with the exception, that we consider refraction for the maxima of
 * partial and noncentral eclipses.
 * Geographical positions are referred to sea level / the mean ellipsoid.
 *
 * Errors:
 * - from uncertainty of JPL-ephemerides (0.01 arcsec): 
 *	about 40 meters
 * - from displacement of shadow points by atmospheric refraction:
 *      a few meters 
 * - from deviation of the geoid from the ellipsoid 
 *      a few meters
 * - from polar motion
 *      a few meters
 * For geographical locations that are interesting for observation,
 * the error is always < 100 m.
 * However, if the sun is close to the horizon,
 * all of these errors can grow up to a km or more. 
 * 
 * Function returns:
 * -1 (ERR)	on error (e.g. if swe_calc() for sun or moon fails)
 * 0		if there is no solar eclipse at tjd
 * SE_ECL_TOTAL
 * SE_ECL_ANNULAR
 * SE_ECL_TOTAL | SE_ECL_CENTRAL
 * SE_ECL_TOTAL | SE_ECL_NONCENTRAL
 * SE_ECL_ANNULAR | SE_ECL_CENTRAL
 * SE_ECL_ANNULAR | SE_ECL_NONCENTRAL
 * SE_ECL_PARTIAL
 * 
 * geopos[0]:	geographic longitude of central line
 * geopos[1]:	geographic latitude of central line
 *
 * not implemented so far:
 *
 * geopos[2]:	geographic longitude of northern limit of umbra
 * geopos[3]:	geographic latitude of northern limit of umbra
 * geopos[4]:	geographic longitude of southern limit of umbra
 * geopos[5]:	geographic latitude of southern limit of umbra
 * geopos[6]:	geographic longitude of northern limit of penumbra
 * geopos[7]:	geographic latitude of northern limit of penumbra
 * geopos[8]:	geographic longitude of southern limit of penumbra
 * geopos[9]:	geographic latitude of southern limit of penumbra
 *
 * Attention: "northern" and "southern" limits of umbra do not 
 * necessarily correspond to the northernmost or southernmost 
 * geographic position, where the total, annular, or partial
 * phase is visible at a given time. 
 * Imagine a situation in northern summer, when the sun illuminates 
 * the northern polar circle. The southernmost point of the core 
 * shadow may then touch the north pole, and therefore the 
 * northernmost point will be more in the south.
 * Note also that with annular eclipses, the northern edge is 
 * usually geographically the southern one. With annular-total 
 * ones, the two lines cross, usually twice. The maximum is always
 * total in such cases. 
 *
 * attr[0]	fraction of solar diameter covered by moon (magnitude)
 * attr[1]	ratio of lunar diameter to solar one
 * attr[2]	fraction of solar disc covered by moon (obscuration)
 * attr[3]      diameter of core shadow in km
 * attr[4]	azimuth of sun at tjd
 * attr[5]	true altitude of sun above horizon at tjd
 * attr[6]	apparent altitude of sun above horizon at tjd
 * attr[7]	angular distance of moon from sun in degrees
 * attr[8]	magnitude acc. to NASA;
 *              = attr[0] for partial and attr[1] for annular and total eclipses
 * attr[9]	saros series number
 * attr[10]	saros series member number
 *         declare as attr[20] at least !
 */
static int32 swe_sol_eclipse_where_r_body(swe_ctx *ctx, double tjd_ut, 
                int32 ifl,
		double *geopos, 
		double *attr, 
		char *serr)
{
  int32 retflag, retflag2;
  double dcore[10];
  ifl &= SEFLG_EPHMASK; 
  swi_set_tid_acc(ctx, tjd_ut, ifl, 0, serr);
  if ((retflag = eclipse_where(ctx, tjd_ut, SE_SUN, NULL, ifl, geopos, dcore, serr)) < 0)
    return retflag;
  if ((retflag2 = eclipse_how(ctx, tjd_ut, SE_SUN, NULL, ifl, geopos[0], geopos[1], 0, attr, serr)) == ERR)
    return retflag2;
  attr[3] = dcore[0];
  return retflag;
}

int32 CALL_CONV swe_sol_eclipse_where_r(swe_ctx *ctx, double tjd_ut, 
                int32 ifl,
		double *geopos, 
		double *attr, 
		char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_sol_eclipse_where_r_body(ctx, tjd_ut, ifl, geopos, attr, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_sol_eclipse_where(
		double tjd_ut, 
                int32 ifl,
		double *geopos, 
		double *attr, 
		char *serr)
{
  return swe_sol_eclipse_where_r(swi_default_ctx(), tjd_ut, ifl, geopos, attr, serr);
}

/*
  double tjd_ut,       time, Jul. day UT 
  int32 ipl,           planet number 
  char* starname,      star name, must be NULL or ”” if not a star 
  int32 ifl,           ephemeris flag 
  double *geopos,      return array, 2 doubles, geo. long. and lat.
		       eastern longitude is positive,
		       western longitude is negative,
		       northern latitude is positive,
  double *attr,        return array, 20 doubles, see below 
  char *serr           return error string 

  array attr:
  attr[0]              fraction of object's diameter covered by moon (magnitude)
  attr[1]              ratio of lunar diameter to object's diameter
  attr[2]              fraction of object's disc covered by moon (obscuration)
  attr[3]              diameter of core shadow in km
  attr[4]              azimuth of object at tjd
  attr[5]              true altitude of object above horizon at tjd
  attr[6]              apparent altitude of object above horizon at tjd
  attr[7]              angular distance of moon from object in degrees
 */
static int32 swe_lun_occult_where_r_body(swe_ctx *ctx, double tjd_ut, 
                int32 ipl,
                char *starname,
                int32 ifl,
		double *geopos, 
		double *attr, 
		char *serr)
{
  int32 retflag, retflag2;
  double dcore[10];
  if (ipl < 0) ipl = 0;
  ifl &= SEFLG_EPHMASK; 
  swi_set_tid_acc(ctx, tjd_ut, ifl, 0, serr);
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO */
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  if ((retflag = eclipse_where(ctx, tjd_ut, ipl, starname, ifl, geopos, dcore, serr)) < 0)
    return retflag;
  if ((retflag2 = eclipse_how(ctx, tjd_ut, ipl, starname, ifl, geopos[0], geopos[1], 0, attr, serr)) == ERR)
    return retflag2;
  attr[3] = dcore[0];
  return retflag;
}

int32 CALL_CONV swe_lun_occult_where_r(swe_ctx *ctx, double tjd_ut, 
                int32 ipl,
                char *starname,
                int32 ifl,
		double *geopos, 
		double *attr, 
		char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_lun_occult_where_r_body(ctx, tjd_ut, ipl, starname, ifl, geopos, attr, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_lun_occult_where(
		double tjd_ut, 
                int32 ipl,
                char *starname,
                int32 ifl,
		double *geopos, 
		double *attr, 
		char *serr)
{
  return swe_lun_occult_where_r(swi_default_ctx(), tjd_ut, ipl, starname, ifl, geopos, attr, serr);
}

/* Set the observer position for THIS calculation only.
 *
 * Every eclipse and occultation search here takes a geopos argument and has
 * to put it into the context before computing topocentrically. It must not
 * publish -- an eclipse calculation announcing its observer as the
 * process-wide default would silently move every other thread's
 * topocentric results, which is what SWI_CFG_LOCAL exists to prevent.
 *
 * The init call is the part that is easy to leave out, and doing so was a
 * real bug: swe_sol_eclipse_when_loc() returned "geographic position has
 * not been set" when it was the FIRST library call in a process, and
 * succeeded if anything at all had run before it.
 *
 * The order is the whole story. swi_init_swed_if_start() carries
 * swi_config_sync(), which adopts whatever another thread has published.
 * swe_set_topo_r() does call it -- but under swi_config_begin_apply(), and
 * sync is a documented no-op while a setter is mid-apply; SWI_CFG_LOCAL
 * wraps it the same way again. So the adopt never happened here. It
 * happened later instead, at the first swe_calc() inside the search, by
 * which point it re-applied the master configuration and cleared the
 * observer this had just set. Nothing in swecl.c called
 * swi_init_swed_if_start() -- sweph.c does at 14 sites and swephlib.c at 5,
 * which is why every other entry point was already immune.
 *
 * Sync first, then override. Both halves matter and neither is optional.
 */
static void set_topo_for_this_call(swe_ctx *ctx, double lon, double lat, double alt)
{
  swi_init_swed_if_start(ctx);
  SWI_CFG_LOCAL(ctx, swe_set_topo_r(ctx, lon, lat, alt));
}

/* Used by several swe_sol_eclipse_ functions.
 * Like swe_sol_eclipse_where(), but instead of attr[0], it returns:
 *
 * dcore[0]:	core shadow width in km
 * dcore[2]:	distance of shadow axis from geocenter r0
 * dcore[3]:	diameter of core shadow on fundamental plane d0
 * dcore[4]:	diameter of half-shadow on fundamental plane D0
 */
static int32 eclipse_where( swe_ctx *ctx, double tjd_ut, int32 ipl, char *starname, int32 ifl, double *geopos, double *dcore, 
		char *serr)
{
  int i;
  int32 retc = 0, niter = 0;
  double e[6], et[6], rm[6], rs[6], rmt[6], rst[6], xs[6], xst[6];
  double x[6];
  double lm[6], ls[6], lx[6];
  double dsm, dsmt, d0, D0, s0, r0, d, s, dm;
  double de = 6378140.0 / AUNIT;
  double earthobl = 1 - EARTH_OBLATENESS;
  double deltat, tjd, sidt;
  double drad;
  double sinf1, sinf2, cosf1, cosf2;
  double rmoon = RMOON;
  double dmoon = 2 * rmoon;
  int32 iflag, iflag2;
  /* double ecce = sqrt(2 * EARTH_OBLATENESS - EARTH_OBLATENESS * EARTH_OBLATENESS); */
  AS_BOOL no_eclipse = FALSE;
  struct epsilon *oe = &ctx->oec;
  for (i = 0; i < 10; i++) 
    dcore[i] = 0;
  /* nutation need not be in lunar and solar positions,
   * if mean sidereal time will be used */
  iflag = SEFLG_SPEED | SEFLG_EQUATORIAL | ifl;
  iflag2 = iflag | SEFLG_RADIANS;
  iflag  = iflag | SEFLG_XYZ;
  deltat = swe_deltat_ex_r(ctx, tjd_ut, ifl, serr);
  tjd = tjd_ut + deltat;
  /* moon in cartesian coordinates */
  if ((retc = swe_calc_r(ctx, tjd, SE_MOON, iflag, rm, serr)) == ERR)
    return retc;
  /* moon in polar coordinates */
  if ((retc = swe_calc_r(ctx, tjd, SE_MOON, iflag2, lm, serr)) == ERR)
    return retc;
  /* sun in cartesian coordinates */
  if ((retc = calc_planet_star(ctx, tjd, ipl, starname, iflag, rs, serr)) == ERR)
    return retc;
  /* sun in polar coordinates */
  if ((retc = calc_planet_star(ctx, tjd, ipl, starname, iflag2, ls, serr)) == ERR)
    return retc;
  /* save sun position */
  for (i = 0; i <= 2; i++)
    rst[i] = rs[i];
  /* save moon position */
  for (i = 0; i <= 2; i++)
    rmt[i] = rm[i];
  if (iflag & SEFLG_NONUT)
    sidt = swe_sidtime0_r(ctx, tjd_ut, oe->eps * RADTODEG, 0) * 15 * DEGTORAD;
  else
    sidt = swe_sidtime_r(ctx, tjd_ut) * 15 * DEGTORAD;
  /*
   * radius of planet disk in AU
   */
  if (starname != NULL && *starname != '\0')
    drad = 0;
  else if (ipl < NDIAM)
    drad = pla_diam[ipl] / 2 / AUNIT;
  else if (ipl > SE_AST_OFFSET)
    drad = ctx->ast_diam / 2 * 1000 / AUNIT; /* km -> m -> AU */
  else
    drad = 0;
iter_where:
  for (i = 0; i <= 2; i++) {
    rs[i] = rst[i];
    rm[i] = rmt[i];
  }
  /* Account for oblateness of earth:
   * Instead of flattening the earth, we apply the 
   * correction to the z coordinate of the moon and
   * the sun. This makes the calculation easier.
   */
  for (i = 0; i <= 2; i++)
    lx[i] = lm[i];
  swi_polcart(lx, rm);
  rm[2] /= earthobl;
  /* distance of moon from geocenter */
  dm = sqrt(square_sum(rm));
  /* Account for oblateness of earth */
  for (i = 0; i <= 2; i++)
    lx[i] = ls[i];
  swi_polcart(lx, rs);
  rs[2] /= earthobl;
  /* sun - moon vector */
  for (i = 0; i <= 2; i++) {
    e[i] = (rm[i] - rs[i]);
    et[i] = (rmt[i] - rst[i]);
  }
  /* distance sun - moon */
  dsm = sqrt(square_sum(e));
  dsmt = sqrt(square_sum(et));
  /* sun - moon unit vector */
  for (i = 0; i <= 2; i++) {
    e[i] /= dsm;
    et[i] /= dsmt;
  }
  sinf1 = ((drad - rmoon) / dsm);
  cosf1 = sqrt(1 - sinf1 * sinf1);
  sinf2 = ((drad + rmoon) / dsm);
  cosf2 = sqrt(1 - sinf2 * sinf2);
  /* distance of moon from fundamental plane */
  s0 = -dot_prod(rm, e);
  /* distance of shadow axis from geocenter */
  r0 = sqrt(dm * dm - s0 * s0);
  /* diameter of core shadow on fundamental plane */
  d0 = (s0 / dsm * (drad * 2 - dmoon) - dmoon) / cosf1;
  /* diameter of half-shadow on fundamental plane */
  D0 = (s0 / dsm * (drad * 2 + dmoon) + dmoon) / cosf2;
  dcore[2] = r0;
  dcore[3] = d0;
  dcore[4] = D0;
  dcore[5] = cosf1;
  dcore[6] = cosf2;
  for (i = 2; i < 5; i++) 
    dcore[i] *= AUNIT / 1000.0;
  /**************************
   * central (total or annular) phase 
   **************************/
  retc = 0;
  if (de * cosf1 >= r0) {
    retc |= SE_ECL_CENTRAL;
  } else if (r0 <= de * cosf1 + fabs(d0) / 2) {
    retc |= SE_ECL_NONCENTRAL;
  } else if (r0 <= de * cosf2 + D0 / 2) {
    retc |= (SE_ECL_PARTIAL | SE_ECL_NONCENTRAL);
  } else {
    if (serr != NULL)
      sprintf(serr, "no solar eclipse at tjd = %f", tjd);
    for (i = 0; i < 2; i++)
      geopos[i] = 0;
    *dcore = 0;
    retc = 0;
    d = 0;
    no_eclipse = TRUE;
    /*return retc;*/
  } 
  /* distance of shadow point from fundamental plane */
  d = s0 * s0 + de * de - dm * dm;
  if (d > 0)
    d = sqrt(d);
  else 
    d = 0;
  /* distance of moon from shadow point on earth */
  s = s0 - d;
  /* next: geographic position of eclipse center.
   * if shadow axis does not touch the earth, 
   * place on earth with maximum occultation is computed.
   */
#if 0	/* the following stuff is meaningless for observations */
  /* 
   * account for refraction at horizon
   */
  if (d == 0) {
    double ds, a, b;
    /* distance of sun from geocenter */
    ds = sqrt(square_sum(rs));
    a = PI - acos(swi_dot_prod_unit(e, erm));
       /* refraction at horizon + sun radius = about 0.83 degrees */
    b = 34.4556 / 60.0 * DEGTORAD + asin(drad / ds);
# if 0
     /* at edge of umbra and penumbra 
      * light rays are not parallel to shadow axis.
      * for a short time close to contact of umbra and
      * penumbra, an angle < 0.27 degrees would have
      * to be subtracted from b;
      */ 
    if (retc & SE_ECL_PARTIAL) {
      d = d0;
      sinf = sinf1;
    } else {
      d = D0;
      sinf = sinf2;
    }
    c = (r0 - de) / d * 2 * sinf;
    if (c > sinf1) {
      b -= .....;
    }
      printf("%f %f %f", a * RADTODEG, b * RADTODEG, s);
      printf(" %f\n", s);
# else
    if (retc & SE_ECL_PARTIAL) 
      b -= asin(sinf2);	/* maximum! */
    else
      b -= asin(sinf1);
# endif
    s += tan(b) * cos(PI / 2 - a) * dm;
  }
#endif
  /* geographic position of eclipse center (maximum) */
  for (i = 0; i <= 2; i++)
    xs[i] = rm[i] + s * e[i];
  /* we need geographic position with correct z, as well */
  for (i = 0; i <= 2; i++)
    xst[i] = xs[i];
  xst[2] *= earthobl;
  swi_cartpol(xst, xst);
  if (niter <= 0) {
    double cosfi = cos(xst[1]);
    double sinfi = sin(xst[1]);
    double eobl = EARTH_OBLATENESS;
    double cc= 1 / sqrt(cosfi * cosfi + (1-eobl) * (1-eobl) * sinfi * sinfi); 
    double ss= (1-eobl) * (1-eobl) * cc; 
    earthobl =  ss;
    niter++;
    goto iter_where;
  }
  swi_polcart(xst, xst);
  /* to longitude and latitude */
  swi_cartpol(xs, xs);
  /* measure from sidereal time at greenwich */
  xs[0] -= sidt;
  xs[0] *= RADTODEG;
  xs[1] *= RADTODEG;
  xs[0] = swe_degnorm(xs[0]);
  /* west is negative */
  if (xs[0] > 180)
    xs[0] -= 360;
  geopos[0] = xs[0];
  geopos[1] = xs[1];
  /* diameter of core shadow: 
   * first, distance moon - place of eclipse on earth */
  for (i = 0; i <= 2; i++) 
    x[i] = rmt[i] - xst[i];
  s = sqrt(square_sum(x));
  /* diameter of core shadow at place of maximum eclipse */
  *dcore = (s / dsmt * ( drad * 2 - dmoon) - dmoon) * cosf1;
  *dcore *= AUNIT / 1000.0;
  /* diameter of penumbra at place of maximum eclipse */
  dcore[1] = (s / dsmt * ( drad * 2 + dmoon) + dmoon) * cosf2;
  dcore[1] *= AUNIT / 1000.0;
  if (!(retc & SE_ECL_PARTIAL) && !no_eclipse) {
    if (*dcore > 0) {
      /*printf("annular\n");*/
      retc |= SE_ECL_ANNULAR;
    } else {
      /*printf("total\n");*/
      retc |= SE_ECL_TOTAL;
    }
  }
  return retc;
}

static int32 calc_planet_star(swe_ctx *ctx, double tjd_et, int32 ipl, char *starname, int32 iflag, double *x, char *serr)
{
  int retc = OK;
  if (starname == NULL || *starname == '\0') {
    retc = swe_calc_r(ctx, tjd_et, ipl, iflag, x, serr);
  } else {
    retc = swe_fixstar_r(ctx, starname, tjd_et, iflag, x, serr);
  }
  return retc;
}

/* Computes attributes of a solar eclipse for given tjd, geo. longitude, 
 * geo. latitude, and geo. height.
 * 
 * retflag	SE_ECL_TOTAL or SE_ECL_ANNULAR or SE_ECL_PARTIAL
 *              SE_ECL_NONCENTRAL
 *              if 0, no eclipse is visible at geogr. position.
 * 
 * attr[0]	fraction of solar diameter covered by moon;
 *              with total/annular eclipses, it results in magnitude acc. to IMCCE.
 * attr[1]	ratio of lunar diameter to solar one
 * attr[2]	fraction of solar disc covered by moon (obscuration)
 * attr[3]      diameter of core shadow in km
 * attr[4]	azimuth of sun at tjd
 * attr[5]	true altitude of sun above horizon at tjd
 * attr[6]	apparent altitude of sun above horizon at tjd
 * attr[7]	elongation of moon in degrees
 * attr[8]	magnitude acc. to NASA;
 *              = attr[0] for partial and attr[1] for annular and total eclipses
 * attr[9]	saros series number
 * attr[10]	saros series member number
 *         declare as attr[20] at least !
 * 
 */
static int32 swe_sol_eclipse_how_r_body(swe_ctx *ctx, double tjd_ut, 
          int32 ifl,
          double *geopos,
          double *attr, 
          char *serr)
{
  int32 retflag, retflag2, i;
  double dcore[10], ls[6], xaz[6];
  double geopos2[20];
  for (i = 0; i <= 10; i++)
    attr[i] = 0;
  if (geopos[2] < SEI_ECL_GEOALT_MIN || geopos[2] > SEI_ECL_GEOALT_MAX) {
    if (serr != NULL)
      sprintf(serr, "location for eclipses must be between %.0f and %.0f m above sea", SEI_ECL_GEOALT_MIN, SEI_ECL_GEOALT_MAX);
    return ERR;
  }
  ifl &= SEFLG_EPHMASK; 
  swi_set_tid_acc(ctx, tjd_ut, ifl, 0, serr);
  if ((retflag = eclipse_how(ctx, tjd_ut, SE_SUN, NULL, ifl, geopos[0], geopos[1], geopos[2], attr, serr)) == ERR)
    return retflag;
  if ((retflag2 = eclipse_where(ctx, tjd_ut, SE_SUN, NULL, ifl, geopos2, dcore, serr)) == ERR)
    return retflag2;
  if (retflag)
    retflag |= (retflag2 & (SE_ECL_CENTRAL | SE_ECL_NONCENTRAL));
  attr[3] = dcore[0];
  set_topo_for_this_call(ctx, geopos[0], geopos[1], geopos[2]);
  if (swe_calc_ut_r(ctx, tjd_ut, SE_SUN, ifl | SEFLG_TOPOCTR | SEFLG_EQUATORIAL, ls, serr) == ERR)
    return ERR;
  swe_azalt_r(ctx, tjd_ut, SE_EQU2HOR, geopos, 0, 10, ls, xaz);
  attr[4] = xaz[0];
  attr[5] = xaz[1];
  attr[6] = xaz[2];
  if (xaz[2] <= 0)
    retflag = 0;
  if (retflag == 0) {
    for (i = 0; i <= 3; i++)
      attr[i] = 0;
    for (i = 8; i <= 10; i++)
      attr[i] = 0;
  }
  return retflag;
}

int32 CALL_CONV swe_sol_eclipse_how_r(swe_ctx *ctx, double tjd_ut, 
          int32 ifl,
          double *geopos,
          double *attr, 
          char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_sol_eclipse_how_r_body(ctx, tjd_ut, ifl, geopos, attr, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_sol_eclipse_how(
          double tjd_ut, 
          int32 ifl,
          double *geopos,
          double *attr, 
          char *serr)
{
  return swe_sol_eclipse_how_r(swi_default_ctx(), tjd_ut, ifl, geopos, attr, serr);
}

#define USE_AZ_NAV 0
static int32 eclipse_how( swe_ctx *ctx, double tjd_ut, int32 ipl, char *starname, int32 ifl,
          double geolon, double geolat, double geohgt,
          double *attr, char *serr)
{
  int i, j, k;
  int32 retc = 0;
  double te, d;
  double xs[6], xm[6], ls[6], lm[6], x1[6], x2[6];
  double rmoon, rsun, rsplusrm, rsminusrm;
  double dctr;
  double drad;
  int32 iflag = SEFLG_EQUATORIAL | SEFLG_TOPOCTR | ifl;
  int32 iflagcart = iflag | SEFLG_XYZ;
#if USE_AZ_NAV
  double mdd, eps, sidt, armc;
#endif
  double xh[6], hmin_appr;
  double lsun, lmoon, lctr, lsunleft, a, b, sc1, sc2;
  double geopos[3];
  for (i = 0; i < 10; i++)
    attr[i] = 0;
  geopos[0] = geolon;
  geopos[1] = geolat;
  geopos[2] = geohgt;
  te = tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, ifl, serr);
  set_topo_for_this_call(ctx, geolon, geolat, geohgt);
  if (calc_planet_star(ctx, te, ipl, starname, iflag, ls, serr) == ERR)
    return ERR;
  if (swe_calc_r(ctx, te, SE_MOON, iflag, lm, serr) == ERR)
    return ERR;
  if (calc_planet_star(ctx, te, ipl, starname, iflagcart, xs, serr) == ERR)
    return ERR;
  if (swe_calc_r(ctx, te, SE_MOON, iflagcart, xm, serr) == ERR)
    return ERR;
  /*
   * radius of planet disk in AU
   */
  if (starname != NULL && *starname != '\0')
    drad = 0;
  else if (ipl < NDIAM)
    drad = pla_diam[ipl] / 2 / AUNIT;
  else if (ipl > SE_AST_OFFSET)
    drad = ctx->ast_diam / 2 * 1000 / AUNIT; /* km -> m -> AU */
  else
    drad = 0;
  /* 
   * azimuth and altitude of sun or planet
   */
#if USE_AZ_NAV   /* old */
  eps = swi_epsiln(ctx, te, iflag);
  if (iflag & SEFLG_NONUT)
    sidt = swe_sidtime0_r(ctx, tjd_ut, eps * RADTODEG, 0) * 15;
  else
    sidt = swe_sidtime_r(ctx, tjd_ut) * 15;
  armc = sidt + geolon;
  mdd = swe_degnorm(ls[0] - armc);
  xh[0] = swe_degnorm(mdd - 90);
  xh[1] = ls[1];
  xh[2] = ls[2];
  swe_cotrans(xh, xh, 90 - geolat); /* azimuth from east, counterclock, via north */
#else
  swe_azalt_r(ctx, tjd_ut, SE_EQU2HOR, geopos, 0, 10, ls, xh); /* azimuth from south, clockwise, via west */
#endif
  /* eclipse description */
  rmoon = asin(RMOON / lm[2]) * RADTODEG;
  rsun = asin(drad / ls[2]) * RADTODEG;
  rsplusrm = rsun + rmoon;
  rsminusrm = rsun - rmoon;
  for (i = 0; i < 3; i++) {
    x1[i] = xs[i] / ls[2];
    x2[i] = xm[i] / lm[2];
  }
  dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
  /* 
   * phase 
   */
  if (dctr < rsminusrm)
    retc = SE_ECL_ANNULAR;
  else if (dctr < fabs(rsminusrm))
    retc = SE_ECL_TOTAL;
  else if (dctr < rsplusrm)
    retc = SE_ECL_PARTIAL;
  else {
    retc = 0;
    if (serr != NULL)
      sprintf(serr, "no solar eclipse at tjd = %f", tjd_ut);
  }
  /*
   * ratio of diameter of moon to that of sun
   */
  if (rsun > 0)
    attr[1] = rmoon / rsun;
  else
    attr[1] = 0;
  /* 
   * eclipse magnitude:
   * fraction of solar diameter covered by moon
   */
  lsun = asin(rsun / 2 * DEGTORAD) * 2;
  lsunleft = (-dctr + rsun + rmoon);
  if (lsun > 0) {
    attr[0] = lsunleft / rsun / 2;
  } else {
    //attr[0] = 100;
    attr[0] = 1;
  }
  /*if (retc == SE_ECL_ANNULAR || retc == SE_ECL_TOTAL)
      attr[0] = attr[1];*/
  /*
   * obscuration:
   * fraction of solar disc obscured by moon
   */
  lsun = rsun;
  lmoon = rmoon;
  lctr = dctr;
  if (retc == 0 || lsun == 0) {
    //attr[2] = 100;
    attr[2] = 1;
  } else if (retc == SE_ECL_TOTAL || retc == SE_ECL_ANNULAR) {
    attr[2] = lmoon * lmoon / lsun / lsun;
  } else {
    a = 2 * lctr * lmoon;
    b = 2 * lctr * lsun;
    if (a < 1e-9) {
      attr[2] = lmoon * lmoon / lsun / lsun;
    } else {
      a = (lctr * lctr + lmoon * lmoon - lsun * lsun) / a;
      if (a > 1) a = 1;
      if (a < -1) a = -1;
      b = (lctr * lctr + lsun * lsun - lmoon * lmoon) / b;
      if (b > 1) b = 1;
      if (b < -1) b = -1;
      a = acos(a);
      b = acos(b);
      sc1 = a * lmoon * lmoon / 2;
      sc2 = b * lsun * lsun / 2;
      sc1 -= (cos(a) * sin(a)) * lmoon * lmoon / 2;
      sc2 -= (cos(b) * sin(b)) * lsun * lsun / 2;
      attr[2] = (sc1 + sc2) * 2 / PI / lsun / lsun;
    }
  }
  attr[7] = dctr;
  /* approximate minimum height for visibility, considering
   * refraction and dip
   * 34.4556': refraction at horizon, from Bennets formulae 
   * 1.75' / sqrt(geohgt): dip of horizon
   * 0.37' / sqrt(geohgt): refraction between horizon and observer */
  hmin_appr = -(34.4556 + (1.75 + 0.37) * sqrt(geohgt)) / 60;	
  if (xh[1] + rsun + fabs(hmin_appr) >= 0 && retc) 
    retc |= SE_ECL_VISIBLE;	/* eclipse visible */
#if USE_AZ_NAV   /* old */
  attr[4] = swe_degnorm(90 - xh[0]); /* azimuth, from north, clockwise, via east */
#else
  attr[4] = xh[0];	/* azimuth, from south, clockwise, via west */
#endif
  attr[5] = xh[1]; /* height */
  attr[6] = xh[2]; /* height */
  if (ipl == SE_SUN && (starname == NULL || *starname == '\0')) {
    /* magnitude of solar eclipse according to NASA */
    attr[8] = attr[0]; /* fraction of diameter occulted */
    if (retc & (SE_ECL_TOTAL | SE_ECL_ANNULAR))
      attr[8] = attr[1]; /* ratio between diameters of sun and moon */
    /* saros series and member */
    for (i = 0; i < NSAROS_SOLAR; i++) {
      d = (tjd_ut - saros_data_solar[i].tstart) / SAROS_CYCLE;
      if (d < 0 && d * SAROS_CYCLE > -2) d = 0.0000001;
      if (d < 0) continue;
      j = (int) d;
      if ((d - j) * SAROS_CYCLE < 2) {
	attr[9] = (double) saros_data_solar[i].series_no;
	attr[10] = (double) j + 1;
	break;
      }
      k = j + 1;
      if ((k - d) * SAROS_CYCLE < 2) {
	attr[9] = (double) saros_data_solar[i].series_no;
	attr[10] = (double) k + 1;
	break;
      }
    }
    if (i == NSAROS_SOLAR) {
      attr[9] = attr[10] = -99999999;
    }
  }
  return retc;
}

/* When is the next solar eclipse anywhere on earth?
 *
 * input parameters:
 *
 * tjd_start    start time for search (UT)
 * ifl		ephemeris to be used (SEFLG_SWIEPH, etc.)
 * ifltype	eclipse type to be searched (SE_ECL_TOTAL, etc.)
 *              0, if any type of eclipse is wanted
 *
 * return values: 
 *
 * retflag	SE_ECL_TOTAL or SE_ECL_ANNULAR or SE_ECL_PARTIAL
 *              or SE_ECL_ANNULAR_TOTAL
 *              SE_ECL_CENTRAL
 *              SE_ECL_NONCENTRAL
 *
 * tret[0]	time of maximum eclipse
 * tret[1]	time, when eclipse takes place at local apparent noon
 * tret[2]	time of eclipse begin
 * tret[3]	time of eclipse end
 * tret[4]	time of totality begin
 * tret[5]	time of totality end
 * tret[6]	time of center line begin
 * tret[7]	time of center line end
 * tret[8]	time when annular-total eclipse becomes total
 *      	   not implemented so far
 * tret[9]	time when annular-total eclipse becomes annular again
 *      	   not implemented so far
 *         declare as tret[10] at least!
 *
 */
static int32 swe_sol_eclipse_when_glob_r_body(swe_ctx *ctx, double tjd_start, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  int i, j, k, m, n, o, i1 = 0, i2 = 0;
  int32 retflag = 0, retflag2 = 0;
  double de = 6378.140, a;
  double t, tt, tjd, tjds, dt, dtint, dta, dtb;
  double T, T2, T3, T4, K, M, Mm;
  double E, Ff;
  double xs[6], xm[6], ls[6], lm[6];
  double rmoon, rsun, dcore[10];
  double dc[3], dctr;
  double twohr = 2.0 / 24.0;
  double tenmin = 10.0 / 24.0 / 60.0;
  double dt1 = 0, dt2 = 0;
  double geopos[20], attr[20];
  double dtstart, dtdiv;
  double xa[6], xb[6];
  int direction = 1;
  AS_BOOL dont_times = FALSE;
  int32 iflag, iflagcart;
  ifl &= SEFLG_EPHMASK; 
  swi_set_tid_acc(ctx, tjd_start, ifl, 0, serr);
  iflag = SEFLG_EQUATORIAL | ifl;
  iflagcart = iflag | SEFLG_XYZ;
  if (ifltype == (SE_ECL_PARTIAL | SE_ECL_CENTRAL)) {
    if (serr != NULL)
      strcpy(serr, "central partial eclipses do not exist");
    return ERR;
  }
  if (ifltype == (SE_ECL_ANNULAR_TOTAL | SE_ECL_NONCENTRAL)) {
    if (serr != NULL)
      strcpy(serr, "non-central hybrid (annular-total) eclipses do not exist");
    return ERR;
  }
  if (ifltype == 0)
    ifltype = SE_ECL_TOTAL | SE_ECL_ANNULAR | SE_ECL_PARTIAL
           | SE_ECL_ANNULAR_TOTAL | SE_ECL_NONCENTRAL | SE_ECL_CENTRAL;
  if (ifltype == SE_ECL_TOTAL || ifltype == SE_ECL_ANNULAR || ifltype == SE_ECL_ANNULAR_TOTAL)
    ifltype |= (SE_ECL_NONCENTRAL | SE_ECL_CENTRAL);
  if (ifltype == SE_ECL_PARTIAL)
    ifltype |= SE_ECL_NONCENTRAL;
  if (backward)
    direction = -1;
  K = (int) ((tjd_start - J2000) / 365.2425 * 12.3685);
  K -= direction;
next_try:
  retflag = 0;
  dont_times = FALSE;
  for (i = 0; i <= 9; i++)
    tret[i] = 0;
  T = K / 1236.85;
  T2 = T * T; T3 = T2 * T; T4 = T3 * T;
  Ff = swe_degnorm(160.7108 + 390.67050274 * K
               - 0.0016341 * T2
               - 0.00000227 * T3
               + 0.000000011 * T4);
  if (Ff > 180)
    Ff -= 180;
  if (Ff > 21 && Ff < 159) { 	/* no eclipse possible */
    K += direction;
    goto next_try;
  }
  /* approximate time of geocentric maximum eclipse
   * formula from Meeus, German, p. 381 */
  tjd = 2451550.09765 + 29.530588853 * K
                      + 0.0001337 * T2
                      - 0.000000150 * T3
                      + 0.00000000073 * T4;
  M = swe_degnorm(2.5534 + 29.10535669 * K
                      - 0.0000218 * T2
                      - 0.00000011 * T3);
  Mm = swe_degnorm(201.5643 + 385.81693528 * K
                      + 0.1017438 * T2
                      + 0.00001239 * T3
                      + 0.000000058 * T4);
  E = 1 - 0.002516 * T - 0.0000074 * T2;
  M *= DEGTORAD;
  Mm *= DEGTORAD;
  tjd = tjd - 0.4075 * sin(Mm)
            + 0.1721 * E * sin(M);
  /* 
   * time of maximum eclipse (if eclipse) = 
   * minimum geocentric angle between sun and moon edges.
   * After this time has been determined, check
   * whether or not an eclipse is taking place with
   * the functions eclipse_where(ctx, ) and _how().
   */
  dtstart = 1;
  if (tjd < 2000000 || tjd > 2500000)
    dtstart = 5;
  dtdiv = 4;
  for (dt = dtstart; 
       dt > 0.0001; 
       dt /= dtdiv) {
    for (i = 0, t = tjd - dt; i <= 2; i++, t += dt) {
      if (swe_calc_r(ctx, t, SE_SUN, iflag, ls, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflag, lm, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_SUN, iflagcart, xs, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
      for (m = 0; m < 3; m++) {
        xa[m] = xs[m] / ls[2];
        xb[m] = xm[m] / lm[2];
      }
      dc[i] = acos(swi_dot_prod_unit(xa, xb)) * RADTODEG;
      rmoon = asin(RMOON / lm[2]) * RADTODEG;
      rsun = asin(RSUN / ls[2]) * RADTODEG;
      dc[i] -= (rmoon + rsun);
    }
    find_maximum(dc[0], dc[1], dc[2], dt, &dtint, &dctr);
    tjd += dtint + dt;
  }
  tjds = tjd - swe_deltat_ex_r(ctx, tjd, ifl, serr);
  tjds = tjd - swe_deltat_ex_r(ctx, tjds, ifl, serr);
  tjds = tjd = tjd - swe_deltat_ex_r(ctx, tjds, ifl, serr);
  if ((retflag = eclipse_where(ctx, tjd, SE_SUN, NULL, ifl, geopos, dcore, serr)) == ERR)
    return retflag;
  retflag2 = retflag;
    /* in extreme cases _where() returns no eclipse, where there is
     * actually a very small one, therefore call _how() with the
     * coordinates returned by _where(): */
  if ((retflag2 = eclipse_how(ctx, tjd, SE_SUN, NULL, ifl, geopos[0], geopos[1], 0, attr, serr)) == ERR)
    return retflag2;
  if (retflag2 == 0) {
    K += direction;
    goto next_try;
  }
  tret[0] = tjd;
  if ((backward && tret[0] >= tjd_start - 0.0001) 
    || (!backward && tret[0] <= tjd_start + 0.0001)) {
    K += direction;
    goto next_try;
  }
  /*
   * eclipse type, SE_ECL_TOTAL, _ANNULAR, etc.
   * SE_ECL_ANNULAR_TOTAL will be discovered later
   */
  if ((retflag = eclipse_where(ctx, tjd, SE_SUN, NULL, ifl, geopos, dcore, serr)) == ERR)
    return retflag;
  if (retflag == 0) {	/* can happen with extremely small percentage */
    retflag = SE_ECL_PARTIAL | SE_ECL_NONCENTRAL;
    tret[4] = tret[5] = tjd;	/* fix this ???? */
    dont_times = TRUE;
  } 
  /* 
   * check whether or not eclipse type found is wanted
   */
  /* non central eclipse is wanted: */
  if (!(ifltype & SE_ECL_NONCENTRAL) && (retflag & SE_ECL_NONCENTRAL)) {
    K += direction;
    goto next_try;
  }
  /* central eclipse is wanted: */
  if (!(ifltype & SE_ECL_CENTRAL) && (retflag & SE_ECL_CENTRAL)) {
    K += direction;
    goto next_try;
  }
  /* non annular eclipse is wanted: */
  if (!(ifltype & SE_ECL_ANNULAR) && (retflag & SE_ECL_ANNULAR)) {
    K += direction;
    goto next_try;
  }
  /* non partial eclipse is wanted: */
  if (!(ifltype & SE_ECL_PARTIAL) && (retflag & SE_ECL_PARTIAL)) {
    K += direction;
    goto next_try;
  }
  /* annular-total eclipse will be discovered later */
  if (!(ifltype & (SE_ECL_TOTAL | SE_ECL_ANNULAR_TOTAL)) && (retflag & SE_ECL_TOTAL)) {
    K += direction;
    goto next_try;
  }
  if (dont_times)
    goto end_search_global;
  /* 
   * n = 0: times of eclipse begin and end 
   * n = 1: times of totality begin and end 
   * n = 2: times of center line begin and end 
   */
  if (retflag & SE_ECL_PARTIAL)
    o = 0;
  else if (retflag & SE_ECL_NONCENTRAL)
    o = 1;
  else 
    o = 2;
  dta = twohr;
  dtb = tenmin / 3.0;
  for (n = 0; n <= o; n++) {
    if (n == 0) {
      /*dc[1] = dcore[3] / 2 + de - dcore[1];*/
      i1 = 2; i2 = 3;
    } else if (n == 1) {
      if (retflag & SE_ECL_PARTIAL)
        continue;
      i1 = 4; i2 = 5;
    } else if (n == 2) {
      if (retflag & SE_ECL_NONCENTRAL)
        continue;
      i1 = 6; i2 = 7;
    }
    for (i = 0, t = tjd - dta; i <= 2; i += 1, t += dta) {
      if ((retflag2 = eclipse_where(ctx, t, SE_SUN, NULL, ifl, geopos, dcore, serr)) == ERR)
        return retflag2;
      if (n == 0)
        dc[i] = dcore[4] / 2 + de / dcore[5] - dcore[2];
      else if (n == 1)
        dc[i] = fabs(dcore[3]) / 2 + de / dcore[6] - dcore[2];
      else if (n == 2)
        dc[i] = de / dcore[6] - dcore[2];
    }
    find_zero(dc[0], dc[1], dc[2], dta, &dt1, &dt2);
    tret[i1] = tjd + dt1 + dta;
    tret[i2] = tjd + dt2 + dta;
    for (m = 0, dt = dtb; m < 3; m++, dt /= 3) {
      for (j = i1; j <= i2; j += (i2 - i1)) {
        for (i = 0, t = tret[j] - dt; i < 2; i++, t += dt) {
          if ((retflag2 = eclipse_where(ctx, t, SE_SUN, NULL, ifl, geopos, dcore, serr)) == ERR)
            return retflag2;
          if (n == 0)
            dc[i] = dcore[4] / 2 + de / dcore[5] - dcore[2];
          else if (n == 1)
            dc[i] = fabs(dcore[3]) / 2 + de / dcore[6] - dcore[2];
          else if (n == 2)
            dc[i] = de / dcore[6] - dcore[2];
        }
        dt1 = dc[1] / ((dc[1] - dc[0]) / dt);
        tret[j] -= dt1;
      }
    }
  }
  /*
   * annular-total eclipses
   */
  if (retflag & SE_ECL_TOTAL) {
    if ((retflag2 = eclipse_where(ctx, tret[0], SE_SUN, NULL, ifl, geopos, dcore, serr)) == ERR)
      return retflag2;
    dc[0] = *dcore;
    if ((retflag2 = eclipse_where(ctx, tret[4], SE_SUN, NULL, ifl, geopos, dcore, serr)) == ERR)
      return retflag2;
    dc[1] = *dcore;
    if ((retflag2 = eclipse_where(ctx, tret[5], SE_SUN, NULL, ifl, geopos, dcore, serr)) == ERR)
      return retflag2;
    dc[2] = *dcore;
    /* the maximum is always total, and there is either one or
     * to times before and after, when the core shadow becomes
     * zero and totality changes into annularity or vice versa.
     */
    if (dc[0] * dc[1] < 0 || dc[0] * dc[2] < 0) {
      retflag |= SE_ECL_ANNULAR_TOTAL;
      retflag &= ~SE_ECL_TOTAL;
    } 
  } 
  /* if eclipse is given but not wanted: */
  if (!(ifltype & SE_ECL_TOTAL) && (retflag & SE_ECL_TOTAL)) {
    K += direction;
    goto next_try;
  }
  /* if annular_total eclipse is given but not wanted: */
  if (!(ifltype & SE_ECL_ANNULAR_TOTAL) && (retflag & SE_ECL_ANNULAR_TOTAL)) {
    K += direction;
    goto next_try;
  }
  /*
   * time of maximum eclipse at local apparent noon
   */
  /* first, find out, if there is a solar transit 
   * between begin and end of eclipse */
  k = 2;
  for (i = 0; i < 2; i++) {
    j = i + k;
    tt = tret[j] + swe_deltat_ex_r(ctx, tret[j], ifl, serr);
    if (swe_calc_r(ctx, tt, SE_SUN, iflag, ls, serr) == ERR)
        return ERR;
    if (swe_calc_r(ctx, tt, SE_MOON, iflag, lm, serr) == ERR)
        return ERR;
    dc[i] = swe_degnorm(ls[0] - lm[0]);
    if (dc[i] > 180)
      dc[i] -= 360;
  }
  if (dc[0] * dc[1] >= 0)	/* no transit */
    tret[1] = 0;
  else {
    tjd = tjds;
    dt = 0.1;
    dt1 = (tret[3] - tret[2]) / 2.0;
    if (dt1 < dt)
      dt = dt1 / 2.0;
    for (j = 0; 
        dt > 0.01;
        j++, dt /= 3) {
      for (i = 0, t = tjd; i <= 1; i++, t -= dt) {
        tt = t + swe_deltat_ex_r(ctx, t, ifl, serr);
        if (swe_calc_r(ctx, tt, SE_SUN, iflag, ls, serr) == ERR)
          return ERR;
        if (swe_calc_r(ctx, tt, SE_MOON, iflag, lm, serr) == ERR)
          return ERR;
        dc[i] = swe_degnorm(ls[0] - lm[0]);
        if (dc[i] > 180)
          dc[i] -= 360;
        if (dc[i] > 180)
          dc[i] -= 360;
      }
      a = (dc[1] - dc[0]) / dt;
      if (a < 1e-10)
        break;
      dt1 = dc[0] / a;
      tjd += dt1;
    }
    tret[1] = tjd;
  }
end_search_global:
  return retflag;
  /*
   * the time of maximum occultation is practically identical
   * with the time of maximum core shadow diameter.
   *
   * the time, when duration of totality is maximal,
   * is not an interesting computation either. Near the maximum
   * occulation, the time of totality can be the same by 
   * a second for hundreds of kilometers (for 10 minutes 
   * or more).
   *  
   * for annular eclipses the maximum duration is close to the
   * beginning and the end of the center lines, where is also
   * the minimum of core shadow diameter.
   */
}

int32 CALL_CONV swe_sol_eclipse_when_glob_r(swe_ctx *ctx, double tjd_start, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_sol_eclipse_when_glob_r_body(ctx, tjd_start, ifl, ifltype, tret, backward, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_sol_eclipse_when_glob(double tjd_start, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  return swe_sol_eclipse_when_glob_r(swi_default_ctx(), tjd_start, ifl, ifltype, tret, backward, serr);
}

/* When is the next lunar occultation anywhere on earth?
 * This function also finds solar eclipses, but is less efficient
 * than swe_sol_eclipse_when_glob().
 * 
 * input parameters:
 *
 * tjd_start          start time for search (UT)
 * ipl                planet number of occulted body
 * starname           name of occulted star. Must be NULL or "", if a planetary
 *                    occultation is to be calculated. For the use of this
 *                    field, also see swe_fixstar().
 * ifl		      ephemeris to be used (SEFLG_SWIEPH, etc.)
 *          	      ephemeris flag. 
 *
 * ifltype	      eclipse type to be searched (SE_ECL_TOTAL, etc.)
 *                    0, if any type of eclipse is wanted
 *                    this functionality also works with occultations
 *
 * backward	      if 1, causes search backward in time
 *
 *                    If you want to have only one conjunction 
 *                    of the moon with the body tested, add the following flag:
 *                    backward |= SE_ECL_ONE_TRY. If this flag is not set, 
 *                    the function will search for an occultation until it
 *                    finds one. For bodies with ecliptical latitudes > 5,
 *                    the function may search successlessly until it reaches
 *                    the end of the ephemeris.
 *                    (Note: we do not add SE_ECL_ONE_TRY to ifl, because 
 *                    ifl may contain SEFLG_TOPOCTR (=SE_ECL_ONE_TRY) from
 *		      the parameter iflag of swe_calc() etc. Although the
 *                    topocentric flag is irrelevant here, it might cause 
 * 		      confusion.)
 *
 * return values: 
 *
 * retflag	SE_ECL_TOTAL or SE_ECL_ANNULAR or SE_ECL_PARTIAL
 *              or SE_ECL_ANNULAR_TOTAL
 *              SE_ECL_CENTRAL
 *              SE_ECL_NONCENTRAL
 *
 * tret[0]	time of maximum eclipse
 * tret[1]	time, when eclipse takes place at local apparent noon
 * tret[2]	time of eclipse begin
 * tret[3]	time of eclipse end
 * tret[4]	time of totality begin
 * tret[5]	time of totality end
 * tret[6]	time of center line begin
 * tret[7]	time of center line end
 * tret[8]	time when annular-total eclipse becomes total
 *      	   not implemented so far
 * tret[9]	time when annular-total eclipse becomes annular again
 *      	   not implemented so far
 *         declare as tret[10] at least!
 *
 */
static int32 swe_lun_occult_when_glob_r_body(swe_ctx *ctx, double tjd_start, int32 ipl, char *starname, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  int i, j, k, m, n, o, i1, i2;
  int32 retflag = 0, retflag2 = 0;
  double de = 6378.140, a;
  double t, tt, tjd = 0, tjds, dt, dtint, dta, dtb;
  double drad, dl;
  double xs[6], xm[6], ls[6], lm[6];
  double rmoon, rsun, dcore[10];
  double dc[20], dctr;
  double twohr = 2.0 / 24.0;
  double tenmin = 10.0 / 24.0 / 60.0;
  double dt1 = 0, dt2 = 0, dadd2 = 1;
  double geopos[20];
  double dtstart, dtdiv;
  int direction = 1;
  int32 ifltype2;
  int32 iflag, iflagcart;
  AS_BOOL dont_times = FALSE;
  int32 one_try = backward & SE_ECL_ONE_TRY;
  if (ipl < 0) ipl = 0;
/*if (backward & SEI_OCC_FAST)
    dont_times = TRUE; */
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO */
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  ifl &= SEFLG_EPHMASK; 
  swi_set_tid_acc(ctx, tjd_start, ifl, 0, serr);
  iflag = SEFLG_EQUATORIAL | ifl;
  iflagcart = iflag | SEFLG_XYZ;
  backward &= 1L;
  /* 
   * initializations 
   */
  if (ifltype == (SE_ECL_PARTIAL | SE_ECL_CENTRAL)) {
    if (serr != NULL)
      strcpy(serr, "central partial eclipses do not exist");
    return ERR;
  }
  if (ipl != SE_SUN) {
    ifltype2 = (ifltype & ~(SE_ECL_NONCENTRAL | SE_ECL_CENTRAL));
    if (ifltype2 == SE_ECL_ANNULAR || ifltype == SE_ECL_ANNULAR_TOTAL) {
      if (serr != NULL)
	snprintf(serr, AS_MAXCH, "annular occulation do not exist for object %d %.190s\n", ipl, starname);
      return ERR;
    }
  }
  if (ipl != SE_SUN && (ifltype & (SE_ECL_ANNULAR | SE_ECL_ANNULAR_TOTAL)))
    ifltype &= ~(SE_ECL_ANNULAR|SE_ECL_ANNULAR_TOTAL);
  if (ifltype == 0) {
    ifltype = SE_ECL_TOTAL | SE_ECL_PARTIAL | SE_ECL_NONCENTRAL | SE_ECL_CENTRAL;
    if (ipl == SE_SUN) 
      ifltype |= (SE_ECL_ANNULAR | SE_ECL_ANNULAR_TOTAL);
  }
  if (ifltype & (SE_ECL_TOTAL | SE_ECL_ANNULAR | SE_ECL_ANNULAR_TOTAL))
    ifltype |= (SE_ECL_NONCENTRAL | SE_ECL_CENTRAL);
  if (ifltype & SE_ECL_PARTIAL)
    ifltype |= SE_ECL_NONCENTRAL;
  retflag = 0;
  for (i = 0; i <= 9; i++)
    tret[i] = 0;
  if (backward)
    direction = -1;
  t = tjd_start;
  tjd = t;
next_try:
  if (calc_planet_star(ctx, t, ipl, starname, ifl, ls, serr) == ERR)
      return ERR;
  /* fixed stars with an ecliptic latitude > 7  or < -7 cannot have
   * an occultation. Even lunar parallax andd proper motion of star
   * will never allow it. */
  if (fabs(ls[1]) > 7 && starname != NULL && *starname != '\0') {
    if (serr != NULL)
      snprintf(serr, AS_MAXCH, "occultation never occurs: star %.150s has ecl. lat. %.1f", starname, ls[1]);
    return ERR;
  }
  if (swe_calc_r(ctx, t, SE_MOON, ifl, lm, serr) == ERR)
      return ERR;
  dl = swe_degnorm(ls[0] - lm[0]);
  if (direction < 0)
    dl -= 360;
  /* get rough conjunction in ecliptic longitude */
  while (fabs(dl) > 0.1) {
    t += dl / 13;
    if (calc_planet_star(ctx, t, ipl, starname, ifl, ls, serr) == ERR)
	return ERR;
    if (swe_calc_r(ctx, t, SE_MOON, ifl, lm, serr) == ERR)
	return ERR;
    dl = swe_degnorm(ls[0] - lm[0]);
    if (dl > 180) dl -= 360;
  }
  tjd = t;
  /* difference in latitude too big for an occultation */
  drad = fabs(ls[1] - lm[1]);
  if (drad > 2) {
    if (one_try) {
      tret[0] = t + direction; /* return a date suitable for next try */
      return 0;
    }
    t += direction * 20;
    tjd = t;
    goto next_try;
  }
  /*
   * radius of planet disk in AU
   */
  if (starname != NULL && *starname != '\0')
    drad = 0;
  else if (ipl < NDIAM)
    drad = pla_diam[ipl] / 2 / AUNIT;
  else if (ipl > SE_AST_OFFSET)
    drad = ctx->ast_diam / 2 * 1000 / AUNIT; /* km -> m -> AU */
  else
    drad = 0;
  /* 
   * time of maximum eclipse (if eclipse) = 
   * minimum geocentric angle between sun and moon edges.
   * After this time has been determined, check
   * whether or not an eclipse is taking place with
   * the functions eclipse_where(ctx, ) and _how().
   */
  dtstart = dadd2; /* originally 1 */
  dtdiv = 3;
  for (dt = dtstart; 
       dt > 0.0001; 
       dt /= dtdiv) {
    for (i = 0, t = tjd - dt; i <= 2; i++, t += dt) {
      if (calc_planet_star(ctx, t, ipl, starname, iflag, ls, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflag, lm, serr) == ERR)
        return ERR;
      if (calc_planet_star(ctx, t, ipl, starname, iflagcart, xs, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
      dc[i] = acos(swi_dot_prod_unit(xs, xm)) * RADTODEG;
      rmoon = asin(RMOON / lm[2]) * RADTODEG;
      rsun = asin(drad / ls[2]) * RADTODEG;
      dc[i] -= (rmoon + rsun);
    }
    find_maximum(dc[0], dc[1], dc[2], dt, &dtint, &dctr);
    tjd += dtint + dt;
  }
  tjd -= swe_deltat_ex_r(ctx, tjd, ifl, serr);
  tjds = tjd;
  if ((retflag = eclipse_where(ctx, tjd, ipl, starname, ifl, geopos, dcore, serr)) == ERR)
    return retflag;
  retflag2 = retflag;
    /* in extreme cases _where() returns no eclipse, where there is
     * actually a very small one, therefore call _how() with the
     * coordinates returned by _where(): */
  /* if ((retflag2 = eclipse_how(ctx, tjd, ipl, starname, ifl, geopos[0], geopos[1], 0, attr, serr)) == ERR)
    return retflag2; */
  if (retflag2 == 0) {
    /* only one try! */
    /* if (one_try && ((direction == 1 && tjd > tjd_start) || (direction == -1 && tjd < tjd_start))) {*/
    if (one_try) {
      tret[0] = tjd;
      return 0;
    }
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    tjd = t;
    goto next_try;
  }
  tret[0] = tjd;
  /* should not happen anymore Version 2.01 */
  if ((backward && tret[0] >= tjd_start - 0.0001) 
    || (!backward && tret[0] <= tjd_start + 0.0001)) {
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    tjd = t;
    goto next_try;
  }
  /*
   * eclipse type, SE_ECL_TOTAL, _ANNULAR, etc.
   * SE_ECL_ANNULAR_TOTAL will be discovered later
   */
  if ((retflag = eclipse_where(ctx, tjd, ipl, starname, ifl, geopos, dcore, serr)) == ERR)
    return retflag;
  if (retflag == 0) {	/* can happen with extremely small percentage */
    retflag = SE_ECL_PARTIAL | SE_ECL_NONCENTRAL;
    tret[4] = tret[5] = tjd;	/* fix this ???? */
    dont_times = TRUE;
  } 
  /* 
   * check whether or not eclipse type found is wanted
   */
  /* non central eclipse is wanted: */
  if (!(ifltype & SE_ECL_NONCENTRAL) && (retflag & SE_ECL_NONCENTRAL)) {
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    if (one_try) {
      tret[0] = tjd;
      return 0;
    }
    tjd = t;
    goto next_try;
  }
  /* central eclipse is wanted: */
  if (!(ifltype & SE_ECL_CENTRAL) && (retflag & SE_ECL_CENTRAL)) {
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    if (one_try) {
      tret[0] = tjd;
      return 0;
    }
    tjd = t;
    goto next_try;
  }
  /* non annular eclipse is wanted: */
  if (!(ifltype & SE_ECL_ANNULAR) && (retflag & SE_ECL_ANNULAR)) {
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    if (one_try) {
      tret[0] = tjd;
      return 0;
    }
    tjd = t;
    goto next_try;
  }
  /* non partial eclipse is wanted: */
  if (!(ifltype & SE_ECL_PARTIAL) && (retflag & SE_ECL_PARTIAL)) {
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    if (one_try) {
      tret[0] = tjd;
      return 0;
    }
    tjd = t;
    goto next_try;
  }
  /* annular-total eclipse will be discovered later */
  if (!(ifltype & (SE_ECL_TOTAL | SE_ECL_ANNULAR_TOTAL)) && (retflag & SE_ECL_TOTAL)) {
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    if (one_try) {
      tret[0] = tjd;
      return 0;
    }
    tjd = t;
    goto next_try;
  }
  if (dont_times)
    goto end_search_global;
  /* 
   * n = 0: times of eclipse begin and end 
   * n = 1: times of totality begin and end 
   * n = 2: times of center line begin and end 
   */
  if (retflag & SE_ECL_PARTIAL)
    o = 0;
  else if (retflag & SE_ECL_NONCENTRAL)
    o = 1;
  else 
    o = 2;
  dta = twohr;
  dtb = tenmin;
  for (n = 0; n <= o; n++) {
    if (n == 0) {
      /*dc[1] = dcore[3] / 2 + de - dcore[1];*/
      i1 = 2; i2 = 3;
    } else if (n == 1) {
      if (retflag & SE_ECL_PARTIAL)
        continue;
      i1 = 4; i2 = 5;
    } else if (n == 2) {
      if (retflag & SE_ECL_NONCENTRAL)
        continue;
      i1 = 6; i2 = 7;
    }
    for (i = 0, t = tjd - dta; i <= 2; i += 1, t += dta) {
      if ((retflag2 = eclipse_where(ctx, t, ipl, starname, ifl, geopos, dcore, serr)) == ERR)
        return retflag2;
      if (n == 0)
        dc[i] = dcore[4] / 2 + de / dcore[5] - dcore[2];
      else if (n == 1)
        dc[i] = fabs(dcore[3]) / 2 + de / dcore[6] - dcore[2];
      else if (n == 2)
        dc[i] = de / dcore[6] - dcore[2];
    }
    find_zero(dc[0], dc[1], dc[2], dta, &dt1, &dt2);
    tret[i1] = tjd + dt1 + dta;
    tret[i2] = tjd + dt2 + dta;
    for (m = 0, dt = dtb; m < 3; m++, dt /= 3) {
      for (j = i1; j <= i2; j += (i2 - i1)) {
        for (i = 0, t = tret[j] - dt; i < 2; i++, t += dt) {
          if ((retflag2 = eclipse_where(ctx, t, ipl, starname, ifl, geopos, dcore, serr)) == ERR)
            return retflag2;
          if (n == 0)
            dc[i] = dcore[4] / 2 + de / dcore[5] - dcore[2];
          else if (n == 1)
            dc[i] = fabs(dcore[3]) / 2 + de / dcore[6] - dcore[2];
          else if (n == 2)
            dc[i] = de / dcore[6] - dcore[2];
        }
        dt1 = dc[1] / ((dc[1] - dc[0]) / dt);
        tret[j] -= dt1;
      }
    }
  }
  /*
   * annular-total eclipses
   */
  if (retflag & SE_ECL_TOTAL) {
    if ((retflag2 = eclipse_where(ctx, tret[0], ipl, starname, ifl, geopos, dcore, serr)) == ERR)
      return retflag2;
    dc[0] = *dcore;
    if ((retflag2 = eclipse_where(ctx, tret[4], ipl, starname, ifl, geopos, dcore, serr)) == ERR)
      return retflag2;
    dc[1] = *dcore;
    if ((retflag2 = eclipse_where(ctx, tret[5], ipl, starname, ifl, geopos, dcore, serr)) == ERR)
      return retflag2;
    dc[2] = *dcore;
    /* the maximum is always total, and there is either one or
     * to times before and after, when the core shadow becomes
     * zero and totality changes into annularity or vice versa.
     */
    if (dc[0] * dc[1] < 0 || dc[0] * dc[2] < 0) {
      retflag |= SE_ECL_ANNULAR_TOTAL;
      retflag &= ~SE_ECL_TOTAL;
    } 
  } 
  /* if eclipse is given but not wanted: */
  if (!(ifltype & SE_ECL_TOTAL) && (retflag & SE_ECL_TOTAL)) {
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    if (one_try) {
      tret[0] = tjd;
      return 0;
    }
    tjd = t;
    goto next_try;
  }
  /* if annular_total eclipse is given but not wanted: */
  if (!(ifltype & SE_ECL_ANNULAR_TOTAL) && (retflag & SE_ECL_ANNULAR_TOTAL)) {
    /*t= tjd + direction * dadd;*/
    t = tjd + direction * 20;
    if (one_try) {
      tret[0] = tjd;
      return 0;
    }
    tjd = t;
    goto next_try;
  }
  /*
   * time of maximum eclipse at local apparent noon
   */
  /* first, find out, if there is a solar transit 
   * between begin and end of eclipse */
  k = 2;
  for (i = 0; i < 2; i++) {
    j = i + k;
    tt = tret[j] + swe_deltat_ex_r(ctx, tret[j], ifl, serr);
    if (calc_planet_star(ctx, tt, ipl, starname, iflag, ls, serr) == ERR)
        return ERR;
    if (swe_calc_r(ctx, tt, SE_MOON, iflag, lm, serr) == ERR)
        return ERR;
    dc[i] = swe_degnorm(ls[0] - lm[0]);
    if (dc[i] > 180)
      dc[i] -= 360;
  }
  if (dc[0] * dc[1] >= 0)	/* no transit */
    tret[1] = 0;
  else {
    tjd = tjds;
    dt = 0.1;
    dt1 = (tret[3] - tret[2]) / 2.0;
    if (dt1 < dt)
      dt = dt1 / 2.0;
    for (j = 0; 
        dt > 0.01;
        j++, dt /= 3) {
      for (i = 0, t = tjd; i <= 1; i++, t -= dt) {
        tt = t + swe_deltat_ex_r(ctx, t, ifl, serr);
        if (calc_planet_star(ctx, tt, ipl, starname, iflag, ls, serr) == ERR)
          return ERR;
        if (swe_calc_r(ctx, tt, SE_MOON, iflag, lm, serr) == ERR)
          return ERR;
        dc[i] = swe_degnorm(ls[0] - lm[0]);
        if (dc[i] > 180)
          dc[i] -= 360;
        if (dc[i] > 180)
          dc[i] -= 360;
      }
      a = (dc[1] - dc[0]) / dt;
      if (a < 1e-10)
        break;
      dt1 = dc[0] / a;
      tjd += dt1;
    }
    tret[1] = tjd;
  }
end_search_global:
  return retflag;
  /*
   * the time of maximum occultation is practically identical
   * with the time of maximum core shadow diameter.
   *
   * the time, when duration of totality is maximal,
   * is not an interesting computation either. Near the maximum
   * occulation, the time of totality can be the same by 
   * a second for hundreds of kilometers (for 10 minutes 
   * or more).
   *  
   * for annular eclipses the maximum duration is close to the
   * beginning and the end of the center lines, where is also
   * the minimum of core shadow diameter.
   */
}

int32 CALL_CONV swe_lun_occult_when_glob_r(swe_ctx *ctx, double tjd_start, int32 ipl, char *starname, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_lun_occult_when_glob_r_body(ctx, tjd_start, ipl, starname, ifl, ifltype, tret, backward, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_lun_occult_when_glob(
     double tjd_start, int32 ipl, char *starname, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  return swe_lun_occult_when_glob_r(swi_default_ctx(), tjd_start, ipl, starname, ifl, ifltype, tret, backward, serr);
}

/* When is the next solar eclipse at a given geographical position?
 * Note the uncertainty of Delta T for the remote past and for
 * the future.
 *
 * retflag	SE_ECL_TOTAL or SE_ECL_ANNULAR or SE_ECL_PARTIAL
 *              SE_ECL_VISIBLE, 
 *              SE_ECL_MAX_VISIBLE, 
 *              SE_ECL_1ST_VISIBLE, SE_ECL_2ND_VISIBLE
 *              SE_ECL_3RD_VISIBLE, SE_ECL_4TH_VISIBLE
 *
 * tret[0]	time of maximum eclipse
 * tret[1]	time of first contact
 * tret[2]	time of second contact
 * tret[3]	time of third contact
 * tret[4]	time of forth contact
 * tret[5]	time of sun rise between first and forth contact
 * tret[6]	time of sun set beween first and forth contact
 *
 * attr[0]	fraction of solar diameter covered by moon;
 *              with total/annular eclipses, it results in magnitude acc. to IMCCE.
 * attr[1]	ratio of lunar diameter to solar one
 * attr[2]	fraction of solar disc covered by moon (obscuration)
 * attr[3]      diameter of core shadow in km
 * attr[4]	azimuth of sun at tjd
 * attr[5]	true altitude of sun above horizon at tjd
 * attr[6]	apparent altitude of sun above horizon at tjd
 * attr[7]	elongation of moon in degrees
 * attr[8]	magnitude acc. to NASA;
 *              = attr[0] for partial and attr[1] for annular and total eclipses
 * attr[9]	saros series number
 * attr[10]	saros series member number
 *         declare as attr[20] at least !
 */
static int32 swe_sol_eclipse_when_loc_r_body(swe_ctx *ctx, double tjd_start, int32 ifl,
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  int32 retflag = 0, retflag2 = 0;
  double geopos2[20], dcore[10];
  if (geopos[2] < SEI_ECL_GEOALT_MIN || geopos[2] > SEI_ECL_GEOALT_MAX) {
    if (serr != NULL)
      sprintf(serr, "location for eclipses must be between %.0f and %.0f m above sea", SEI_ECL_GEOALT_MIN, SEI_ECL_GEOALT_MAX);
    return ERR;
  }
  ifl &= SEFLG_EPHMASK; 
  swi_set_tid_acc(ctx, tjd_start, ifl, 0, serr);
  if ((retflag = eclipse_when_loc(ctx, tjd_start, ifl, geopos, tret, attr, backward, serr)) <= 0)
    return retflag;
  /* 
   * diameter of core shadow
   */
  if ((retflag2 = eclipse_where(ctx, tret[0], SE_SUN, NULL, ifl, geopos2, dcore, serr)) == ERR)
    return retflag2;
  retflag |= (retflag2 & SE_ECL_NONCENTRAL);
  attr[3] = dcore[0];
  return retflag; 
}

int32 CALL_CONV swe_sol_eclipse_when_loc_r(swe_ctx *ctx, double tjd_start, int32 ifl,
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_sol_eclipse_when_loc_r_body(ctx, tjd_start, ifl, geopos, tret, attr, backward, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_sol_eclipse_when_loc(double tjd_start, int32 ifl,
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  return swe_sol_eclipse_when_loc_r(swi_default_ctx(), tjd_start, ifl, geopos, tret, attr, backward, serr);
}

/* When is the next solar eclipse at a given geographical position?
 * Note the uncertainty of Delta T for the remote past and for
 * the future.
 *
 * retflag	SE_ECL_TOTAL or SE_ECL_ANNULAR or SE_ECL_PARTIAL
 *              SE_ECL_VISIBLE, 
 *              SE_ECL_MAX_VISIBLE, 
 *              SE_ECL_1ST_VISIBLE, SE_ECL_2ND_VISIBLE
 *              SE_ECL_3RD_VISIBLE, SE_ECL_4TH_VISIBLE
 *              SE_ECL_OCC_BEG_DAYLIGHT, SE_ECL_OCC_END_DAYLIGHT
 * The latter two indicate that the beginning or end of the occultation takes
 * place during the day. If Venus is occulted, it may be observable with the
 * naked eye; if other objects, it may be observable with telescopes.
 *
 * int32 ipl          planet number of occulted body
 * char* starname     name of occulted star. Must be NULL or "", if a planetary
 *                    occultation is to be calculated. For the use of this
 *                    field, also see swe_fixstar().
 * int32 ifl	      ephemeris flag. If you want to have only one conjunction 
 *                    of the moon with the body tested, add the following flag:
 *                    backward |= SE_ECL_ONE_TRY. If this flag is not set, 
 *                    the function will search for an occultation until it
 *                    finds one. For bodies with ecliptical latitudes > 5,
 *                    the function may search unsuccessfully until it reaches
 *                    the end of the ephemeris.
 *
 * for all other parameters, see function swe_sol_eclipse_when_loc().
 */
static int32 swe_lun_occult_when_loc_r_body(swe_ctx *ctx, double tjd_start, int32 ipl, char *starname, int32 ifl,
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  int32 retflag = 0, retflag2 = 0;
  double geopos2[20], dcore[10];
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO */
  if (geopos[2] < SEI_ECL_GEOALT_MIN || geopos[2] > SEI_ECL_GEOALT_MAX) {
    if (serr != NULL)
      sprintf(serr, "location for occultations must be between %.0f and %.0f m above sea", SEI_ECL_GEOALT_MIN, SEI_ECL_GEOALT_MAX);
    return ERR;
  }
  if (ipl < 0) ipl = 0;
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  ifl &= SEFLG_EPHMASK; 
  swi_set_tid_acc(ctx, tjd_start, ifl, 0, serr);
  if ((retflag = occult_when_loc(ctx, tjd_start, ipl, starname, ifl, geopos, tret, attr, backward, serr)) <= 0)
    return retflag;
  /* 
   * diameter of core shadow
   */
  if ((retflag2 = eclipse_where(ctx, tret[0], ipl, starname, ifl, geopos2, dcore, serr)) == ERR)
    return retflag2;
  retflag |= (retflag2 & SE_ECL_NONCENTRAL);
  attr[3] = dcore[0];
  return retflag; 
}

int32 CALL_CONV swe_lun_occult_when_loc_r(swe_ctx *ctx, double tjd_start, int32 ipl, char *starname, int32 ifl,
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_lun_occult_when_loc_r_body(ctx, tjd_start, ipl, starname, ifl, geopos, tret, attr, backward, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_lun_occult_when_loc(double tjd_start, int32 ipl, char *starname, int32 ifl,
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  return swe_lun_occult_when_loc_r(swi_default_ctx(), tjd_start, ipl, starname, ifl, geopos, tret, attr, backward, serr);
}

static int32 eclipse_when_loc(swe_ctx *ctx, double tjd_start, int32 ifl, double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  int i, j, k, m;
  int32 retflag = 0, retc;
  double t, tjd, dt, dtint, K, T, T2, T3, T4, F, M, Mm;
  double tjdr, tjds;
  double E, Ff; // A1, Om;
  double xs[6], xm[6], ls[6], lm[6], x1[6], x2[6], dm, ds;
  double rmoon, rsun, rsplusrm, rsminusrm;
  double dc[3], dctr, dctrmin;
  double twomin = 2.0 / 24.0 / 60.0;
  double tensec = 10.0 / 24.0 / 60.0 / 60.0;
  double twohr = 2.0 / 24.0;
  double tenmin = 10.0 / 24.0 / 60.0;
  double dt1 = 0, dt2 = 0, dtdiv, dtstart;
  int32 iflag = SEFLG_EQUATORIAL | SEFLG_TOPOCTR | ifl;
  int32 iflagcart = iflag | SEFLG_XYZ;
  set_topo_for_this_call(ctx, geopos[0], geopos[1], geopos[2]);
  K = (int) ((tjd_start - J2000) / 365.2425 * 12.3685);
  if (backward)
    K++;
  else
    K--;
next_try:
  T = K / 1236.85;
  T2 = T * T; T3 = T2 * T; T4 = T3 * T;
  Ff = F = swe_degnorm(160.7108 + 390.67050274 * K
               - 0.0016341 * T2
               - 0.00000227 * T3
               + 0.000000011 * T4);
  if (Ff > 180)
    Ff -= 180;
  if (Ff > 21 && Ff < 159) { 	/* no eclipse possible */
    if (backward)
      K--;
    else 
      K++;
    goto next_try;
  }
  /* approximate time of geocentric maximum eclipse.
   * formula from Meeus, German, p. 381 */
  tjd = 2451550.09765 + 29.530588853 * K
                      + 0.0001337 * T2
                      - 0.000000150 * T3
                      + 0.00000000073 * T4;
  M = swe_degnorm(2.5534 + 29.10535669 * K
                      - 0.0000218 * T2
                      - 0.00000011 * T3);
  Mm = swe_degnorm(201.5643 + 385.81693528 * K
                      + 0.1017438 * T2
                      + 0.00001239 * T3
                      + 0.000000058 * T4);
  // Om = swe_degnorm(124.7746 - 1.56375580 * K + 0.0020691 * T2 + 0.00000215 * T3);
  E = 1 - 0.002516 * T - 0.0000074 * T2;
  // A1 = swe_degnorm(299.77 + 0.107408 * K - 0.009173 * T2);
  M *= DEGTORAD;
  Mm *= DEGTORAD;
  F *= DEGTORAD;
  // Om *= DEGTORAD;
  // A1 *= DEGTORAD;
  tjd = tjd - 0.4075 * sin(Mm)
            + 0.1721 * E * sin(M);
  set_topo_for_this_call(ctx, geopos[0], geopos[1], geopos[2]);
  dtdiv = 2;
  dtstart = 0.5;
  if (tjd < 1900000 || tjd > 2500000)	/* because above formula is not good (delta t?) */
    dtstart = 2;
  for (dt = dtstart; 
       dt > 0.00001; 
       dt /= dtdiv) {
    if (dt < 0.1) 
      dtdiv = 3;
    for (i = 0, t = tjd - dt; i <= 2; i++, t += dt) {
      /* this takes some time, but is necessary to avoid
       * missing an eclipse */
      if (swe_calc_r(ctx, t, SE_SUN, iflagcart, xs, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_SUN, iflag, ls, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflag, lm, serr) == ERR)
        return ERR;
      dm = sqrt(square_sum(xm));
      ds = sqrt(square_sum(xs));
      for (k = 0; k < 3; k++) {
        x1[k] = xs[k] / ds /*ls[2]*/;
        x2[k] = xm[k] / dm /*lm[2]*/;
      }
      dc[i] = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
    }
    find_maximum(dc[0], dc[1], dc[2], dt, &dtint, &dctr);
    tjd += dtint + dt;
  }
  if (swe_calc_r(ctx, tjd, SE_SUN, iflagcart, xs, serr) == ERR)
        return ERR;
  if (swe_calc_r(ctx, tjd, SE_SUN, iflag, ls, serr) == ERR)
        return ERR;
  if (swe_calc_r(ctx, tjd, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
  if (swe_calc_r(ctx, tjd, SE_MOON, iflag, lm, serr) == ERR)
        return ERR;
  dctr = acos(swi_dot_prod_unit(xs, xm)) * RADTODEG;
  rmoon = asin(RMOON / lm[2]) * RADTODEG;
  rsun = asin(RSUN / ls[2]) * RADTODEG;
  rsplusrm = rsun + rmoon;
  rsminusrm = rsun - rmoon;
  if (dctr > rsplusrm) {
    if (backward)
      K--;
    else
      K++;
    goto next_try;
  }
  tret[0] = tjd - swe_deltat_ex_r(ctx, tjd, ifl, serr);
  tret[0] = tjd - swe_deltat_ex_r(ctx, tret[0], ifl, serr); /* these two lines are an iteration! */
  if ((backward && tret[0] >= tjd_start - 0.0001) 
    || (!backward && tret[0] <= tjd_start + 0.0001)) {
    if (backward)
      K--;
    else
      K++;
    goto next_try;
  }
  if (dctr < rsminusrm)
    retflag = SE_ECL_ANNULAR;
  else if (dctr < fabs(rsminusrm))
    retflag = SE_ECL_TOTAL;
  else if (dctr <= rsplusrm)
    retflag = SE_ECL_PARTIAL;
  dctrmin = dctr;
  /* contacts 2 and 3 */
  if (dctr > fabs(rsminusrm))  /* partial, no 2nd and 3rd contact */
    tret[2] = tret[3] = 0;
  else {
    dc[1] = fabs(rsminusrm) - dctrmin;
    for (i = 0, t = tjd - twomin; i <= 2; i += 2, t = tjd + twomin) {
      if (swe_calc_r(ctx, t, SE_SUN, iflagcart, xs, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
      dm = sqrt(square_sum(xm));
      ds = sqrt(square_sum(xs));
      rmoon = asin(RMOON / dm) * RADTODEG;
      rmoon *= 0.99916; /* gives better accuracy for 2nd/3rd contacts */
      rsun = asin(RSUN / ds) * RADTODEG;
      rsminusrm = rsun - rmoon;
      for (k = 0; k < 3; k++) {
        x1[k] = xs[k] / ds /*ls[2]*/;
        x2[k] = xm[k] / dm /*lm[2]*/;
      }
      dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
      dc[i] = fabs(rsminusrm) - dctr;
    }
    find_zero(dc[0], dc[1], dc[2], twomin, &dt1, &dt2);
    tret[2] = tjd + dt1 + twomin;
    tret[3] = tjd + dt2 + twomin;
    for (m = 0, dt = tensec; m < 2; m++, dt /= 10) {
      for (j = 2; j <= 3; j++) {
        if (swe_calc_r(ctx, tret[j], SE_SUN, iflagcart | SEFLG_SPEED, xs, serr) == ERR)
          return ERR;
        if (swe_calc_r(ctx, tret[j], SE_MOON, iflagcart | SEFLG_SPEED, xm, serr) == ERR)
          return ERR;
        for (i = 0; i < 2; i++) {
          if (i == 1) {
            for(k = 0; k < 3; k++) {
              xs[k] -= xs[k+3] * dt;
              xm[k] -= xm[k+3] * dt;
            }
          }
          dm = sqrt(square_sum(xm));
          ds = sqrt(square_sum(xs));
          rmoon = asin(RMOON / dm) * RADTODEG;
	  rmoon *= 0.99916; /* gives better accuracy for 2nd/3rd contacts */
          rsun = asin(RSUN / ds) * RADTODEG;
          rsminusrm = rsun - rmoon;
          for (k = 0; k < 3; k++) {
            x1[k] = xs[k] / ds /*ls[2]*/;
            x2[k] = xm[k] / dm /*lm[2]*/;
          }
          dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
          dc[i] = fabs(rsminusrm) - dctr;
        }
        dt1 = -dc[0] / ((dc[0] - dc[1]) / dt);
        tret[j] += dt1;
      }
    }
    tret[2] -= swe_deltat_ex_r(ctx, tret[2], ifl, serr);
    tret[3] -= swe_deltat_ex_r(ctx, tret[3], ifl, serr);
  }
  /* contacts 1 and 4 */
  dc[1] = rsplusrm - dctrmin;
  for (i = 0, t = tjd - twohr; i <= 2; i += 2, t = tjd + twohr) {
    if (swe_calc_r(ctx, t, SE_SUN, iflagcart, xs, serr) == ERR)
      return ERR;
    if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
      return ERR;
    dm = sqrt(square_sum(xm));
    ds = sqrt(square_sum(xs));
    rmoon = asin(RMOON / dm) * RADTODEG;
    rsun = asin(RSUN / ds) * RADTODEG;
    rsplusrm = rsun + rmoon;
    for (k = 0; k < 3; k++) {
      x1[k] = xs[k] / ds /*ls[2]*/;
      x2[k] = xm[k] / dm /*lm[2]*/;
    }
    dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
    dc[i] = rsplusrm - dctr;
  }
  find_zero(dc[0], dc[1], dc[2], twohr, &dt1, &dt2);
  tret[1] = tjd + dt1 + twohr;
  tret[4] = tjd + dt2 + twohr;
  for (m = 0, dt = tenmin; m < 3; m++, dt /= 10) {
    for (j = 1; j <= 4; j += 3) {
      if (swe_calc_r(ctx, tret[j], SE_SUN, iflagcart | SEFLG_SPEED, xs, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, tret[j], SE_MOON, iflagcart | SEFLG_SPEED, xm, serr) == ERR)
        return ERR;
      for (i = 0; i < 2; i++) {
        if (i == 1) {
          for(k = 0; k < 3; k++) {
            xs[k] -= xs[k+3] * dt;
            xm[k] -= xm[k+3] * dt;
          }
        }
        dm = sqrt(square_sum(xm));
        ds = sqrt(square_sum(xs));
        rmoon = asin(RMOON / dm) * RADTODEG;
        rsun = asin(RSUN / ds) * RADTODEG;
        rsplusrm = rsun + rmoon;
        for (k = 0; k < 3; k++) {
          x1[k] = xs[k] / ds /*ls[2]*/;
          x2[k] = xm[k] / dm /*lm[2]*/;
        }
        dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
        dc[i] = fabs(rsplusrm) - dctr;
      }
      dt1 = -dc[0] / ((dc[0] - dc[1]) / dt);
      tret[j] += dt1;
    }
  }
  tret[1] -= swe_deltat_ex_r(ctx, tret[1], ifl, serr);
  tret[4] -= swe_deltat_ex_r(ctx, tret[4], ifl, serr);
  /*  
   * visibility of eclipse phases 
   */
  for (i = 4; i >= 0; i--) {	/* attr for i = 0 must be kept !!! */
    if (tret[i] == 0)
      continue;
    if (eclipse_how(ctx, tret[i], SE_SUN, NULL, ifl, geopos[0], geopos[1], geopos[2], 
		attr, serr) == ERR)
      return ERR;
    /*if (retflag2 & SE_ECL_VISIBLE) { could be wrong for 1st/4th contact */
    if (attr[6] > 0) {	/* this is safe, sun above horizon, using app. alt. */
      retflag |= SE_ECL_VISIBLE;
      switch(i) {
      case 0: retflag |= SE_ECL_MAX_VISIBLE; break;
      case 1: retflag |= SE_ECL_1ST_VISIBLE; break;
      case 2: retflag |= SE_ECL_2ND_VISIBLE; break;
      case 3: retflag |= SE_ECL_3RD_VISIBLE; break;
      case 4: retflag |= SE_ECL_4TH_VISIBLE; break;
      default:  break;
      }
    }
  }
#if 1
  if (!(retflag & SE_ECL_VISIBLE)) {
    if (backward)
      K--;
    else
      K++;
    goto next_try;
  }
#endif
  if ((retc = swe_rise_trans_r(ctx, tret[1] - 0.001, SE_SUN, NULL, iflag, SE_CALC_RISE|SE_BIT_DISC_BOTTOM, geopos, 0, 0, &tjdr, serr)) == ERR)
    return ERR;
  if (retc == -2) /* circumpolar sun */
    return retflag;
  if ((retc = swe_rise_trans_r(ctx, tret[1] - 0.001, SE_SUN, NULL, iflag, SE_CALC_SET|SE_BIT_DISC_BOTTOM, geopos, 0, 0, &tjds, serr)) == ERR)
    return ERR;
  if (retc == -2) /* circumpolar sun */
    return retflag;
  if (tjds < tret[1] || (tjds > tjdr && tjdr > tret[4])) {
    if (backward)
      K--;
    else
      K++;
    goto next_try;
  }
  if (tjdr > tret[1] && tjdr < tret[4]) {
    tret[5] = tjdr;
    if (!(retflag & SE_ECL_MAX_VISIBLE)) {
      tret[0] = tjdr;
      if ((retc = eclipse_how(ctx, tret[5], SE_SUN, NULL, ifl, geopos[0], geopos[1], geopos[2], attr, serr)) == ERR)
	return ERR;
      retflag &= ~(SE_ECL_TOTAL|SE_ECL_ANNULAR|SE_ECL_PARTIAL);
      retflag |= (retc & (SE_ECL_TOTAL|SE_ECL_ANNULAR|SE_ECL_PARTIAL));
    }
  }
  if (tjds > tret[1] && tjds < tret[4]) {
    tret[6] = tjds;
    if (!(retflag & SE_ECL_MAX_VISIBLE)) {
      tret[0] = tjds;
      if ((retc = eclipse_how(ctx, tret[6], SE_SUN, NULL, ifl, geopos[0], geopos[1], geopos[2], attr, serr)) == ERR)
	return ERR;
      retflag &= ~(SE_ECL_TOTAL|SE_ECL_ANNULAR|SE_ECL_PARTIAL);
      retflag |= (retc & (SE_ECL_TOTAL|SE_ECL_ANNULAR|SE_ECL_PARTIAL));
    }
  }
  return retflag;
}

static int32 occult_when_loc(
     swe_ctx *ctx, double tjd_start, int32 ipl, char *starname, 
     int32 ifl, double *geopos, double *tret, double *attr, 
     int32 backward, char *serr)
{
  int i, j, k, m;
  int32 retflag = 0, retc;
  double t, tjd, dt, dtint;
  double tjdr, tjds;
  double xs[6], xm[6], ls[6], lm[6], x1[6], x2[6], dm, ds;
  double rmoon, rsun, rsplusrm, rsminusrm;
  double dc[20], dctr, dctrmin;
  double twomin = 2.0 / 24.0 / 60.0;
  double tensec = 10.0 / 24.0 / 60.0 / 60.0;
  double twohr = 2.0 / 24.0;
  double tenmin = 10.0 / 24.0 / 60.0;
  double dt1 = 0, dt2 = 0, dtdiv, dtstart;
  double dadd2 = 1;
  double drad, dl;
  //AS_BOOL is_partial = FALSE;
  int32 iflag = SEFLG_TOPOCTR | ifl;
  int32 iflaggeo = iflag & ~SEFLG_TOPOCTR;
  int32 iflagcart = iflag | SEFLG_XYZ;
  int direction = 1;
  int32 one_try = backward & SE_ECL_ONE_TRY;
  AS_BOOL stop_after_this = FALSE;
  backward &= 1L;
  retflag = 0;
  set_topo_for_this_call(ctx, geopos[0], geopos[1], geopos[2]);
  for (i = 0; i <= 9; i++)
    tret[i] = 0;
  if (backward)
    direction = -1;
  t = tjd_start;
  tjd = tjd_start;
next_try:
  //is_partial = FALSE;
  if (calc_planet_star(ctx, t, ipl, starname, iflaggeo, ls, serr) == ERR)
      return ERR;
  /* fixed stars with an ecliptic latitude > 7  or < -7 cannot have
   * an occultation. Even lunar parallax andd proper motion of star
   * will never allow it. */
  if (fabs(ls[1]) > 7 && starname != NULL && *starname != '\0') {
    if (serr != NULL)
      snprintf(serr, AS_MAXCH, "occultation never occurs: star %.150s has ecl. lat. %.1f", starname, ls[1]);
    return ERR;
  }
  if (swe_calc_r(ctx, t, SE_MOON, iflaggeo, lm, serr) == ERR)
      return ERR;
  dl = swe_degnorm(ls[0] - lm[0]);
  if (direction < 0)
    dl -= 360;
  /* get rough conjunction in ecliptic longitude */
  while (fabs(dl) > 0.1) {
    t += dl / 13;
    if (calc_planet_star(ctx, t, ipl, starname, iflaggeo, ls, serr) == ERR)
	return ERR;
    if (swe_calc_r(ctx, t, SE_MOON, iflaggeo, lm, serr) == ERR)
	return ERR;
    dl = swe_degnorm(ls[0] - lm[0]);
    if (dl > 180) dl -= 360;
  }
  tjd = t;
  /* difference in latitude too big for an occultation */
  drad = fabs(ls[1] - lm[1]);
  if (drad > 2) {
    if (one_try) {
      tret[0] = t + direction; /* return a date suitable for next try */
      return 0;
    }
    t += direction * 20;
    tjd = t;
    goto next_try;
  }
  /*
   * radius of planet disk in AU
   */
  if (starname != NULL && *starname != '\0')
    drad = 0;
  else if (ipl < NDIAM)
    drad = pla_diam[ipl] / 2 / AUNIT;
  else if (ipl > SE_AST_OFFSET)
    drad = ctx->ast_diam / 2 * 1000 / AUNIT; /* km -> m -> AU */
  else
    drad = 0;
  /* now find out, if there is an occultation at our geogr. location */
  dtdiv = 2;
  dtstart = dadd2; /* formerly 0.2 */
  for (dt = dtstart; 
       dt > 0.00001; 
       dt /= dtdiv) {
    if (dt < 0.01) 
      dtdiv = 2;
    for (i = 0, t = tjd - dt; i <= 2; i++, t += dt) {
      /* this takes some time, but is necessary to avoid
       * missing an eclipse */
      if (calc_planet_star(ctx, t, ipl, starname, iflagcart, xs, serr) == ERR)
        return ERR;
      if (calc_planet_star(ctx, t, ipl, starname, iflag, ls, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflag, lm, serr) == ERR)
        return ERR;
      if (dt < 0.1 && fabs(ls[1] - lm[1]) > 2) {
        if (one_try || stop_after_this) {
          stop_after_this = TRUE;
        } else {
          /*t = tjd + direction * 2;*/
          t = tjd + direction * 20;
          tjd = t;
          goto next_try;
        }
      }
      dc[i] = acos(swi_dot_prod_unit(xs, xm)) * RADTODEG;
      rmoon = asin(RMOON / lm[2]) * RADTODEG;
      rsun = asin(drad / ls[2]) * RADTODEG;
      dc[i] -= (rmoon + rsun);
    }
    find_maximum(dc[0], dc[1], dc[2], dt, &dtint, &dctr);
    tjd += dtint + dt;
  }
  if (stop_after_this) { /* has one_try = TRUE */
    tret[0] = tjd + direction;  /* return a date suitable for next try */
    return 0;
  }
  if (calc_planet_star(ctx, tjd, ipl, starname, iflagcart, xs, serr) == ERR)
        return ERR;
  if (calc_planet_star(ctx, tjd, ipl, starname, iflag, ls, serr) == ERR)
        return ERR;
  if (swe_calc_r(ctx, tjd, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
  if (swe_calc_r(ctx, tjd, SE_MOON, iflag, lm, serr) == ERR)
        return ERR;
  dctr = acos(swi_dot_prod_unit(xs, xm)) * RADTODEG;
  rmoon = asin(RMOON / lm[2]) * RADTODEG;
  rsun = asin(drad / ls[2]) * RADTODEG;
  rsplusrm = rsun + rmoon;
  rsminusrm = rsun - rmoon;
  if (dctr > rsplusrm) {
    if (one_try) {
      tret[0] = tjd + direction;  /* return a date suitable for next try */
      return 0;
    }
    /*t = tjd + direction;*/
    t = tjd + direction * 20;
    tjd = t;
    goto next_try;
  }
  tret[0] = tjd - swe_deltat_ex_r(ctx, tjd, ifl, serr);
  tret[0] = tjd - swe_deltat_ex_r(ctx, tret[0], ifl, serr);
  if ((backward && tret[0] >= tjd_start - 0.0001) 
    || (!backward && tret[0] <= tjd_start + 0.0001)) {
    /* t = tjd + direction;*/
    if (one_try) {
      tret[0] = tjd + direction;  /* return a date suitable for next try */
      return 0;
    }
    t = tjd + direction * 20;
    tjd = t;
    goto next_try;
  }
  if (dctr < rsminusrm)
    retflag = SE_ECL_ANNULAR;
  else if (dctr < fabs(rsminusrm))
    retflag = SE_ECL_TOTAL;
  else if (dctr <= rsplusrm)
    retflag = SE_ECL_PARTIAL;
  dctrmin = dctr;
  /* contacts 2 and 3 */
  if (dctr > fabs(rsminusrm)) {  /* partial, no 2nd and 3rd contact */
    tret[2] = tret[3] = 0;
    //is_partial = TRUE;
  } else {
    dc[1] = fabs(rsminusrm) - dctrmin;
    for (i = 0, t = tjd - twomin; i <= 2; i += 2, t = tjd + twomin) {
      if (calc_planet_star(ctx, t, ipl, starname, iflagcart, xs, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
      dm = sqrt(square_sum(xm));
      ds = sqrt(square_sum(xs));
      rmoon = asin(RMOON / dm) * RADTODEG;
      rmoon *= 0.99916; /* gives better accuracy for 2nd/3rd contacts */
      rsun = asin(drad / ds) * RADTODEG;
      rsminusrm = rsun - rmoon;
      for (k = 0; k < 3; k++) {
        x1[k] = xs[k] / ds /*ls[2]*/;
        x2[k] = xm[k] / dm /*lm[2]*/;
      }
      dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
      dc[i] = fabs(rsminusrm) - dctr;
    }
    find_zero(dc[0], dc[1], dc[2], twomin, &dt1, &dt2);
    tret[2] = tjd + dt1 + twomin;
    tret[3] = tjd + dt2 + twomin;
    for (m = 0, dt = tensec; m < 2; m++, dt /= 10) {
      for (j = 2; j <= 3; j++) {
        if (calc_planet_star(ctx, tret[j], ipl, starname, iflagcart | SEFLG_SPEED, xs, serr) == ERR)
          return ERR;
        if (swe_calc_r(ctx, tret[j], SE_MOON, iflagcart | SEFLG_SPEED, xm, serr) == ERR)
          return ERR;
        for (i = 0; i < 2; i++) {
          if (i == 1) {
            for(k = 0; k < 3; k++) {
              xs[k] -= xs[k+3] * dt;
              xm[k] -= xm[k+3] * dt;
            }
          }
          dm = sqrt(square_sum(xm));
          ds = sqrt(square_sum(xs));
          rmoon = asin(RMOON / dm) * RADTODEG;
	  rmoon *= 0.99916; /* gives better accuracy for 2nd/3rd contacts */
          rsun = asin(drad / ds) * RADTODEG;
          rsminusrm = rsun - rmoon;
          for (k = 0; k < 3; k++) {
            x1[k] = xs[k] / ds /*ls[2]*/;
            x2[k] = xm[k] / dm /*lm[2]*/;
          }
          dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
          dc[i] = fabs(rsminusrm) - dctr;
        }
        dt1 = -dc[0] / ((dc[0] - dc[1]) / dt);
        tret[j] += dt1;
      }
    }
    tret[2] -= swe_deltat_ex_r(ctx, tret[2], ifl, serr);
    tret[3] -= swe_deltat_ex_r(ctx, tret[3], ifl, serr);
    //is_partial = FALSE;
  }
  /* contacts 1 and 4 */
  dc[1] = rsplusrm - dctrmin;
if (starname == NULL || *starname == '\0') {
  for (i = 0, t = tjd - twohr; i <= 2; i += 2, t = tjd + twohr) {
    if (calc_planet_star(ctx, t, ipl, starname, iflagcart, xs, serr) == ERR)
      return ERR;
    if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
      return ERR;
    dm = sqrt(square_sum(xm));
    ds = sqrt(square_sum(xs));
    rmoon = asin(RMOON / dm) * RADTODEG;
    rsun = asin(drad / ds) * RADTODEG;
    rsplusrm = rsun + rmoon;
    for (k = 0; k < 3; k++) {
      x1[k] = xs[k] / ds /*ls[2]*/;
      x2[k] = xm[k] / dm /*lm[2]*/;
    }
    dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
    dc[i] = rsplusrm - dctr;
  }
  find_zero(dc[0], dc[1], dc[2], twohr, &dt1, &dt2);
  tret[1] = tjd + dt1 + twohr;
  tret[4] = tjd + dt2 + twohr;
  for (m = 0, dt = tenmin; m < 3; m++, dt /= 10) {
    for (j = 1; j <= 4; j += 3) {
      if (calc_planet_star(ctx, tret[j], ipl, starname, iflagcart | SEFLG_SPEED, xs, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, tret[j], SE_MOON, iflagcart | SEFLG_SPEED, xm, serr) == ERR)
        return ERR;
      for (i = 0; i < 2; i++) {
        if (i == 1) {
          for(k = 0; k < 3; k++) {
            xs[k] -= xs[k+3] * dt;
            xm[k] -= xm[k+3] * dt;
          }
        }
        dm = sqrt(square_sum(xm));
        ds = sqrt(square_sum(xs));
        rmoon = asin(RMOON / dm) * RADTODEG;
        rsun = asin(drad / ds) * RADTODEG;
        rsplusrm = rsun + rmoon;
        for (k = 0; k < 3; k++) {
          x1[k] = xs[k] / ds /*ls[2]*/;
          x2[k] = xm[k] / dm /*lm[2]*/;
        }
        dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
        dc[i] = fabs(rsplusrm) - dctr;
      }
      dt1 = -dc[0] / ((dc[0] - dc[1]) / dt);
      tret[j] += dt1;
    }
  }
  tret[1] -= swe_deltat_ex_r(ctx, tret[1], ifl, serr);
  tret[4] -= swe_deltat_ex_r(ctx, tret[4], ifl, serr);
} else { /* fixed stars are point sources, contacts 1 and 4 = contacts 2 and 3 */
  tret[1] = tret[2];
  tret[4] = tret[3];
}
  /*  
   * visibility of eclipse phases 
   */
  for (i = 4; i >= 0; i--) {	/* attr for i = 0 must be kept !!! */
    if (tret[i] == 0)
      continue;
    if (eclipse_how(ctx, tret[i], ipl, starname, ifl, geopos[0], geopos[1], geopos[2], 
		attr, serr) == ERR)
      return ERR;
    /*if (retflag2 & SE_ECL_VISIBLE) { could be wrong for 1st/4th contact */
    if (attr[6] > 0) {	/* this is save, sun above horizon (using app. alt.) */
      retflag |= SE_ECL_VISIBLE;
      switch(i) {
      case 0: retflag |= SE_ECL_MAX_VISIBLE; break;
      case 1: retflag |= SE_ECL_1ST_VISIBLE; break;
      case 2: retflag |= SE_ECL_2ND_VISIBLE; break;
      case 3: retflag |= SE_ECL_3RD_VISIBLE; break;
      case 4: retflag |= SE_ECL_4TH_VISIBLE; break;
      default:  break;
      }
    }
  }
#if 1
  if (!(retflag & SE_ECL_VISIBLE)) {
    /* t = tjd + direction;*/
    if (one_try) {
      tret[0] = tjd + direction;  /* return a date suitable for next try */
      return 0;
    }
    t = tjd + direction * 20;
    tjd = t;
    goto next_try;
  }
#endif
  if ((retc = swe_rise_trans_r(ctx, tret[1] - 0.1, ipl, starname, iflag, SE_CALC_RISE|SE_BIT_DISC_BOTTOM, geopos, 0, 0, &tjdr, serr)) == ERR)
    return ERR;
  if (retc >= 0 && (retc = swe_rise_trans_r(ctx, tret[1] - 0.1, ipl, starname, iflag, SE_CALC_SET|SE_BIT_DISC_BOTTOM, geopos, 0, 0, &tjds, serr)) == ERR)
    return ERR;
  if (retc >= 0) {
    if (tjdr > tret[1] && tjdr < tret[4])
      tret[5] = tjdr;
    if (tjds > tret[1] && tjds < tret[4])
      tret[6] = tjds;
  }
  /* note, circumpolar sun above horizon is not tested */ 
  //if (!is_partial) {
    if ((retc = swe_rise_trans_r(ctx, tret[1], SE_SUN, NULL, iflag, SE_CALC_RISE, geopos, 0, 0, &tjdr, serr)) == ERR)
      return ERR;
    if (retc >= 0 && (retc = swe_rise_trans_r(ctx, tret[1], SE_SUN, NULL, iflag, SE_CALC_SET, geopos, 0, 0, &tjds, serr)) == ERR)
      return ERR;
    if (retc >= 0) {
      if (tjds < tjdr)
	retflag |= SE_ECL_OCC_BEG_DAYLIGHT;
    } 
    if ((retc = swe_rise_trans_r(ctx, tret[4], SE_SUN, NULL, iflag, SE_CALC_RISE, geopos, 0, 0, &tjdr, serr)) == ERR)
      return ERR;
    if (retc >= 0 && (retc = swe_rise_trans_r(ctx, tret[4], SE_SUN, NULL, iflag, SE_CALC_SET, geopos, 0, 0, &tjds, serr)) == ERR)
      return ERR;
    if (retc >= 0) {
      if (tjds < tjdr)
	retflag |= SE_ECL_OCC_END_DAYLIGHT;
    }
  //}
  return retflag;
}

  /* 
   * swe_azalt()
   * Computes azimut and height, from either ecliptic or 
   * equatorial coordinates
   *
   * input:
   *   tjd_ut	  
 *   iflag        either SE_ECL2HOR or SE_EQU2HOR
 *   geopos[3]    geograph. longitude, latitude, height above sea
 *   atpress	  atmospheric pressure at geopos in millibars (hPa)
 *   attemp	  atmospheric temperature in degrees C
 *   xin[2]       input coordinates polar, in degrees
 * 
 * Horizontal coordinates are returned in
 *   xaz[3]       xaz[0] = azimuth
 *                xaz[1] = true altitude
 *                xaz[2] = apparent altitude
 *
 * If atpress is not given (= 0), the programm assumes 1013.25 mbar;
 * if a non-zero height above sea is given, atpress is estimated.
 *   geohgt	  height of observer above sea (optional)
 */
static void swe_azalt_r_body(swe_ctx *ctx, double tjd_ut,
      int32  calc_flag,
      double *geopos,
      double atpress,
      double attemp,
      double *xin, 
      double *xaz)
{
  int i;
  double x[6], xra[3];
  double armc = swe_degnorm(swe_sidtime_r(ctx, tjd_ut) * 15 + geopos[0]);
  double mdd, eps_true;
  for (i = 0; i < 2; i++)
    xra[i] = xin[i];
  xra[2] = 1;
  if (calc_flag == SE_ECL2HOR) {
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 214: SE_ECL_NUT's return was not read, and it refuses
     * past |TT| 1e12 days (bug 196), from UT 8.13e11, inside the date
     * bound: there the ecliptic was turned by 0, the ecliptic place (100,
     * 1) at UT 9e11 answered as the equatorial place (100, 1), azimuth
     * 95.831841562 and altitude -3.572531312 deg (sidereal time IERS
     * 2010). With no obliquity there is no answer: NaN, as past the bound. */
    if (swe_calc_r(ctx, tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, -1, NULL), SE_ECL_NUT, 0, x, NULL) < 0) {
      xaz[0] = xaz[1] = xaz[2] = NAN;
      return;
    }
#else
    swe_calc_r(ctx, tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, -1, NULL), SE_ECL_NUT, 0, x, NULL);
#endif
    eps_true = x[0];
    swe_cotrans(xra, xra, -eps_true);
  }
  mdd = swe_degnorm(xra[0] - armc);
  x[0] = swe_degnorm(mdd - 90);
  x[1] = xra[1];
  x[2] = 1;
  /* azimuth from east, counterclock */
  swe_cotrans(x, x, 90 - geopos[1]);
  /* azimuth from south to west */
  x[0] = swe_degnorm(x[0] + 90);
  xaz[0] = 360 - x[0];
  xaz[1] = x[1];		/* true height */
  if (atpress == 0) {
    /* estimate atmospheric pressure */
    atpress = 1013.25 * pow(1 - 0.0065 * geopos[2] / 288, 5.255);
  } 
  xaz[2] = swe_refrac_extended(x[1], geopos[2], atpress, attemp, ctx->const_lapse_rate, SE_TRUE_TO_APP, NULL);
  /* xaz[2] = swe_refrac_extended(xaz[2], geopos[2], atpress, attemp, ctx->const_lapse_rate, SE_APP_TO_TRUE, NULL);*/
}

void CALL_CONV swe_azalt_r(swe_ctx *ctx, double tjd_ut,
      int32  calc_flag,
      double *geopos,
      double atpress,
      double attemp,
      double *xin, 
      double *xaz)
{
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 214: any date was taken, and with no error to return the
   * answer was NaN at 1e84, +1e300, +inf and NaN, and past 1e12 days
   * azimuth and altitude from a sidereal time and an obliquity past the
   * residual floor. NaN now past the bound (SWI_ECL_NUT_JMAX) and at no
   * date. The refraction takes no date (the altitude, the site's height,
   * pressure and temperature). */
  if (!(tjd_ut >= -SWI_ECL_NUT_JMAX && tjd_ut <= SWI_ECL_NUT_JMAX)) {
    xaz[0] = xaz[1] = xaz[2] = NAN;
    return;
  }
#endif
  swi_config_hold(ctx);	/* bug 124 */
  swe_azalt_r_body(ctx, tjd_ut, calc_flag, geopos, atpress, attemp, xin, xaz);
  swi_config_release(ctx);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
void CALL_CONV swe_azalt(
      double tjd_ut,
      int32  calc_flag,
      double *geopos,
      double atpress,
      double attemp,
      double *xin, 
      double *xaz)
{
  swe_azalt_r(swi_default_ctx(), tjd_ut, calc_flag, geopos, atpress, attemp, xin, xaz);
}

/* 
 * swe_azalt_rev()
 * computes either ecliptical or equatorial coordinates from
 * azimuth and true altitude in degrees.
 * For conversion between true and apparent altitude, there is
 * the function swe_refrac().
 *
 * input:
 *   tjd_ut	  
 *   iflag        either SE_HOR2ECL or SE_HOR2EQU
 *   xin[2]       azimut and true altitude, in degrees 
 */
static void swe_azalt_rev_r_body(swe_ctx *ctx, double tjd_ut,
      int32  calc_flag,
      double *geopos,
      double *xin, 
      double *xout)
{
  int i;
  double x[6], xaz[3];
  double geolon = geopos[0];
  double geolat = geopos[1];
  double armc = swe_degnorm(swe_sidtime_r(ctx, tjd_ut) * 15 + geolon);
  double eps_true, dang;
  for (i = 0; i < 2; i++)
    xaz[i] = xin[i];
  xaz[2] = 1;
  /* azimuth is from south, clockwise. 
   * we need it from east, counterclock */
  xaz[0] = 360 - xaz[0];
  xaz[0] = swe_degnorm(xaz[0] - 90);
  /* equatorial positions */
  dang = geolat - 90;
  swe_cotrans(xaz, xaz, dang);
  xaz[0] = swe_degnorm(xaz[0] + armc + 90);
  xout[0] = xaz[0];
  xout[1] = xaz[1];
  /* ecliptic positions */
  if (calc_flag == SE_HOR2ECL) {
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 214, as in swe_azalt_r_body() */
    if (swe_calc_r(ctx, tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, -1, NULL), SE_ECL_NUT, 0, x, NULL) < 0) {
      xout[0] = xout[1] = NAN;
      return;
    }
#else
    swe_calc_r(ctx, tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, -1, NULL), SE_ECL_NUT, 0, x, NULL);
#endif
    eps_true = x[0];
	swe_cotrans(xaz, x, eps_true);
    xout[0] = x[0];
    xout[1] = x[1];
  }
}

void CALL_CONV swe_azalt_rev_r(swe_ctx *ctx, double tjd_ut,
      int32  calc_flag,
      double *geopos,
      double *xin, 
      double *xout)
{
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 214, as in swe_azalt_r(); the declination too, which
   * HOR2EQU answered at every date */
  if (!(tjd_ut >= -SWI_ECL_NUT_JMAX && tjd_ut <= SWI_ECL_NUT_JMAX)) {
    xout[0] = xout[1] = NAN;
    return;
  }
#endif
  swi_config_hold(ctx);	/* bug 124 */
  swe_azalt_rev_r_body(ctx, tjd_ut, calc_flag, geopos, xin, xout);
  swi_config_release(ctx);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
void CALL_CONV swe_azalt_rev(
      double tjd_ut,
      int32  calc_flag,
      double *geopos,
      double *xin, 
      double *xout)
{
  swe_azalt_rev_r(swi_default_ctx(), tjd_ut, calc_flag, geopos, xin, xout);
}

/* swe_refrac()
 * Transforms apparent to true altitude and vice-versa.
 * These formulae do not handle the case when the
 * sun is visible below the geometrical horizon 
 * (from a mountain top or an air plane)
 * input:
 * double inalt;        * altitude of object in degrees *
 * double atpress;      * millibars (hectopascal) *
 * double attemp;       * degrees C *
 * int32  calc_flag;	* either SE_APP_TO_TRUE or 
 *                      *        SE_TRUE_TO_APP
 */
double CALL_CONV swe_refrac(double inalt, double atpress, double attemp, int32 calc_flag)
{
  double a, refr;
  double pt_factor = atpress / 1010.0 * 283.0 / (273.0 + attemp);
  double trualt, appalt;
#if 0
 /*
  * -- S. L. Moshier */
  double y, yy0, D0, N, D, P, Q;
  int i;
  if (calc_flag == SE_TRUE_TO_APP) {
    trualt = inalt;
    if( (trualt < -2.0) || (trualt >= 90.0) )
      return(trualt);
    /* For high altitude angle, AA page B61
     * Accuracy "usually about 0.1' ".
     */
    if( trualt > 15.0 ) {
	  D = 0.00452*atpress/((273.0+attemp)*tan( DEGTORAD*trualt ));
	  return(trualt + D);
	}
    /* Formula for low altitude is from the Almanac for Computers.
     * It gives the correction for observed altitude, so has
     * to be inverted numerically to get the observed from the true.
     * Accuracy about 0.2' for -20C < T < +40C and 970mb < P < 1050mb.
     */
    /* Start iteration assuming correction = 0
     */
    y = trualt;
    D = 0.0;
    /* Invert Almanac for Computers formula numerically
     */
    P = (atpress - 80.0)/930.0;
    Q = 4.8e-3 * (attemp - 10.0);
    yy0 = y;
    D0 = D;
    for( i=0; i<4; i++ ) {
  	  N = y + (7.31/(y+4.4));
	  N = 1.0/tan(DEGTORAD*N);
	  D = N*P/(60.0 + Q * (N + 39.0));
	  N = y - yy0;
	  yy0 = D - D0 - N; /* denominator of derivative */
	  if( (N != 0.0) && (yy0 != 0.0) )
      /* Newton iteration with numerically estimated derivative */
	    N = y - N*(trualt + D - y)/yy0;
	  else
      /* Can't do it on first pass */
	    N = trualt + D;
	  yy0 = y;
	  D0 = D;
	  y = N;
	}
    return( trualt + D );
  } else {
#else
  /* another algorithm, from Meeus, German, p. 114ff.
   */
  if (calc_flag == SE_TRUE_TO_APP) {
    trualt = inalt;
    if (trualt > 15) {
      a = tan((90 - trualt) * DEGTORAD);
      refr = (58.276 * a - 0.0824 * a * a * a);
      refr *=  pt_factor / 3600.0;
    } else if (trualt > -5) {
      /* the following tan is not defined for a value
       * of trualt near -5.00158 and 89.89158 */
      a = trualt + 10.3 / (trualt + 5.11);
      if (a + 1e-10 >= 90)
    	refr = 0;
      else
        refr = 1.02 / tan(a * DEGTORAD);
      refr *= pt_factor / 60.0;
    } else
      refr = 0;
    appalt = trualt;
    if (appalt + refr > 0)
      appalt += refr;
    return appalt;
  } else {
#endif
  /* apparent to true */
    appalt = inalt;
    /* the following tan is not defined for a value
     * of inalt near -4.3285 and 89.9225 */
    a = appalt + 7.31 / (appalt + 4.4);
    if (a + 1e-10 >= 90)
      refr = 0;
    else {
      refr = 1.00 / tan(a * DEGTORAD);
      refr -= 0.06 * sin(14.7 * refr + 13);
    }
    refr *= pt_factor / 60.0;
    trualt = appalt;
    if (appalt - refr > 0)
      trualt = appalt - refr;
    return trualt;
  }
}

/* Upstream bug 83 -- this fork's: the lapse rate was a static in upstream,
 * which no memset reaches; as a context field it was wiped by the default
 * context's first-use memset when set before any calculation (0.05 K/m
 * read as 0.0065). Initialise first, as the other setters do. */
void CALL_CONV swe_set_lapse_rate_r(swe_ctx *ctx, double lapse_rate)
{
  swi_init_swed_if_start(ctx);
  ctx->const_lapse_rate = lapse_rate;
  swi_config_publish(ctx, SWI_CFG_LAPSE);
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
void CALL_CONV swe_set_lapse_rate(double lapse_rate)
{
  swe_set_lapse_rate_r(swi_default_ctx(), lapse_rate);
}

/* swe_refrac_extended()
 *
 * This function was created thanks to and with the help of the
 * archaeoastronomer Victor Reijs.
 * It is more correct and more skilled than the old function swe_refrac():
 * - it allows correct calculation of refraction for altitudes above sea > 0,
 *   where the ideal horizon and planets that are visible may have a 
 *   negative height. (for swe_refrac(), negative apparent heights do not 
 *   exist!)
 * - it allows to manipulate the refraction constant
 *
 * Transforms apparent to true altitude and vice-versa.
 * input:
 * double inalt;        * altitude of object above geometric horizon in degrees*
 *                      * geometric horizon = plane perpendicular to gravity *
 * double geoalt;       * altitude of observer above sea level in meters *
 * double atpress;      * millibars (hectopascal) *
 * double lapse_rate;    * (dT/dh) [deg K/m]
 * double attemp;       * degrees C *
 * int32  calc_flag;    * either SE_APP_TO_TRUE or
 *                      *        SE_TRUE_TO_APP
 *
 * function returns:
 * double *dret;        * array of 4 doubles; declare 20 doubles !
 * - dret[0] true altitude, if possible; otherwise input value
 * - dret[1] apparent altitude, if possible; otherwise input value
 * - dret[2] refraction
 * - dret[3] dip of the horizon
 * 
 * The body is above the horizon if the dret[0] != dret[1]
 *
 * case 1, conversion from true altitude to apparent altitude
 * - apparent altitude, if body is observable above ideal horizon
 * - true altitude (the input value), otherwise
 *   "ideal horizon" is the horizon as seen above an ideal sphere (as seen
 *   from a plane over the ocean with a clear sky)
 * case 2, conversion from apparent altitude to true altitude
 * - the true altitude resulting from the input apparent altitude, if this value
 *   is a plausible apparent altitude, i.e. if it is a position above the ideal
 *   horizon, taking into account the dip of the horizon
 * - the input altitude otherwise
 *
 * The body is above the horizon if the dret[0] != dret[1]
 */
double CALL_CONV swe_refrac_extended(double inalt, double geoalt, double atpress, double attemp, double lapse_rate, int32 calc_flag, double *dret)
{
  double refr;
  double trualt;
  double dip = calc_dip(geoalt, atpress, attemp, lapse_rate);
  double D, D0, N, y, yy0;
  int i;
  /* make sure that inalt <=90 */
  if( (inalt>90) )
    inalt=180-inalt;
  if (calc_flag == SE_TRUE_TO_APP) {
    if (inalt < -10) {
      if (dret != NULL) {
	dret[0]=inalt;
	dret[1]=inalt;
	dret[2]=0;
	dret[3]=dip;
      }
      return inalt;
    }
    /* by iteration */
    y = inalt;
    D = 0.0;
    yy0 = 0;
    D0 = D;
    for(i=0; i<5; i++) {
      D = calc_astronomical_refr(y,atpress,attemp);
      N = y - yy0;
      yy0 = D - D0 - N; /* denominator of derivative */
      if (N != 0.0 && yy0 != 0.0) /* sic !!! code by Moshier */
        N = y - N*(inalt + D - y)/yy0; /* Newton iteration with numerically estimated derivative */
      else /* Can't do it on first pass */
        N = inalt + D;
      yy0 = y;
      D0 = D;
      y = N;
    }
    refr = D;
    if (inalt + refr < dip) {
      if (dret != NULL) {
	dret[0]=inalt;
	dret[1]=inalt;
	dret[2]=0;
	dret[3]=dip;
      }
      return inalt;
    }
    if (dret != NULL) {
      dret[0]=inalt;
      dret[1]=inalt+refr;
      dret[2]=refr;
      dret[3]=dip;
    }
    return inalt+refr;
  } else {
    refr = calc_astronomical_refr(inalt,atpress,attemp);
    trualt=inalt-refr;
    //printf("inalt=%f, dip=%f\n", inalt, dip);
    if (dret != NULL) {
      if (inalt > dip) {
	dret[0]=trualt;
	dret[1]=inalt;
	dret[2]=refr;
	dret[3]=dip;
      } else {
	dret[0]=inalt;
	dret[1]=inalt;
	dret[2]=0;
	dret[3]=dip;
      }
    }
    // Apparent altitude cannot be below dip.
    // True altitude is only returned if apparent altitude is hgher than dip.
    // Othwise the apparent altitude is returned.
    //if (trualt > dip)
    if (inalt >= dip)  // bug fix dieter, 4 feb 20
      return trualt;
    else
      return inalt;
  }
}

/* calculate the astronomical refraction
 * input parameters:
 * double inalt        * apparent altitude of object
 * double atpress      * atmospheric pressure millibars (hectopascal) *
 * double attemp       * atmospheric temperature degrees C *
 * returns double r in degrees
*/
static double calc_astronomical_refr(double inalt,double atpress, double attemp)
{
#if 0
  /* formula based on G.G. Bennett, The calculation of astronomical refraction in marine navigation,
  * Journal of Inst. Navigation, No. 35, page 255-259, 1982,
  * page 257 for refraction formula: formula H
  * and page 259 for atmospheric compensation
  */
  double refractaccent = 1/tan(DEGTORAD*(inalt + 7.31/(inalt+4.4)));
  double r = (refractaccent - 0.06 * sin(DEGTORAD*(14.7*refractaccent +13)));
  r = ((atpress - 80) / 930 / (1 + 0.00008 * (r + 39) * (attemp - 10)) * r)/60;
 return r;
#else
  /* Formula by Sinclair, see article mentioned above, p. 256. Better for 
   * apparent altitudes < 0;  */
  double r;
  if (inalt > 17.904104638432) { /* for continuous function, instead of '>15' */
    r = 0.97 / tan(inalt * DEGTORAD);
  } else {
    r = (34.46 + 4.23 * inalt + 0.004 * inalt * inalt) / (1 + 0.505 * inalt + 0.0845 * inalt * inalt);
  }
  r = ((atpress - 80) / 930 / (1 + 0.00008 * (r + 39) * (attemp - 10)) * r) / 60.0;
  return r;
#endif
}

/* calculate dip of the horizon
 * input parameters:
 * double geoalt       * altitude of observer above sea level in meters *
 * double atpress      * atmospheric pressure millibars (hectopascal) *
 * double attemp       * atmospheric temperature degrees C *
 * double lapse_rate   * (dT/dh) [deg K/m]
 * returns dip in degrees
 */
static double calc_dip(double geoalt, double atpress, double attemp, double lapse_rate)
{
  /* below formula is based on A. Thom, Megalithic lunar observations, 1973 (page 32).
  * conversion to metric has been done by
  * V. Reijs, 2000, http://www.archaeocosmology.org/eng/refract.htm#Sea
  */
  double krefr = (0.0342 + lapse_rate) / (0.154 * 0.0238);
  double d = 1-1.8480*krefr*atpress/(273.15+attemp)/(273.15+attemp);
  /* return -0.03203*sqrt(geoalt)*sqrt(d); */
  /* double a = acos(1/(1+geoalt/EARTH_RADIUS));*/
  return -180.0/PI * acos(1 / (1 + geoalt / EARTH_RADIUS)) * sqrt(d);
}


/* Computes attributes of a lunar eclipse for given tjd and geopos
 * 
 * retflag	SE_ECL_TOTAL or SE_ECL_PARTIAL
 *              SE_ECL_PENUMBRAL
 *              if 0, there is no eclipse
 * 
 * attr[0]	umbral magnitude at tjd
 * attr[1]      penumbral magnitude
 * attr[4]	azimuth of moon at tjd
 * attr[5]	true altitude of moon above horizon at tjd
 * attr[6]	apparent altitude of moon above horizon at tjd
 * attr[7]	distance of moon from opposition in degrees
 * attr[8]	umbral magnitude at tjd (= attr[0])
 * attr[9]	saros series number
 * attr[10]	saros series member number
 *         declare as attr[20] at least !
 * 
 */
static int32 swe_lun_eclipse_how_r_body(swe_ctx *ctx, double tjd_ut, 
          int32 ifl,
          double *geopos, 
          double *attr, 
          char *serr)
{
  double dcore[10];
  double lm[6], xaz[6];
  int32 retc;
  /* attention: geopos[] is not used so far; may be NULL */
  if (geopos != NULL) 
    geopos[0] = geopos[0]; /* to shut up mint */
  if (geopos != NULL && (geopos[2] < SEI_ECL_GEOALT_MIN || geopos[2] > SEI_ECL_GEOALT_MAX)) {
    if (serr != NULL)
      sprintf(serr, "location for eclipses must be between %.0f and %.0f m above sea", SEI_ECL_GEOALT_MIN, SEI_ECL_GEOALT_MAX);
    return ERR;
  }
  ifl = ifl & ~SEFLG_TOPOCTR;
  ifl &= ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
  swi_set_tid_acc(ctx, tjd_ut, ifl, 0, serr);
  retc = lun_eclipse_how(ctx, tjd_ut, ifl, attr, dcore, serr);
  if (geopos == NULL) {
    return retc;
  }
  /* 
   * azimuth and altitude of moon
   */
  set_topo_for_this_call(ctx, geopos[0], geopos[1], geopos[2]);
  if (swe_calc_ut_r(ctx, tjd_ut, SE_MOON, ifl | SEFLG_TOPOCTR | SEFLG_EQUATORIAL, lm, serr) == ERR)
    return ERR;
  swe_azalt_r(ctx, tjd_ut, SE_EQU2HOR, geopos, 0, 10, lm, xaz);
  attr[4] = xaz[0];
  attr[5] = xaz[1];
  attr[6] = xaz[2];
  if (xaz[2] <= 0)
    retc = 0;
  return retc;
}

int32 CALL_CONV swe_lun_eclipse_how_r(swe_ctx *ctx, double tjd_ut, 
          int32 ifl,
          double *geopos, 
          double *attr, 
          char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_lun_eclipse_how_r_body(ctx, tjd_ut, ifl, geopos, attr, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_lun_eclipse_how(
          double tjd_ut, 
          int32 ifl,
          double *geopos, 
          double *attr, 
          char *serr)
{
  return swe_lun_eclipse_how_r(swi_default_ctx(), tjd_ut, ifl, geopos, attr, serr);
}

/*
 * attr[]: 	see swe_lun_eclipse_how()
 *
 * dcore[0]:	distance of shadow axis from geocenter r0
 * dcore[1]:	diameter of core shadow on fundamental plane d0
 * dcore[2]:	diameter of half-shadow on fundamental plane D0
 */
static int32 lun_eclipse_how(swe_ctx *ctx, 
          double tjd_ut, 
          int32 ifl,
          double *attr, 
          double *dcore, 
          char *serr)
{
  int i, j, k;
  int32 retc = 0;
  double e[6], rm[6], rs[6];
  double dsm, d0, D0, s0, r0, ds, dm;
  double dctr, x1[6], x2[6];
  double f1, f2;
  double deltat, tjd, d;
  double cosf1, cosf2;
  double rmoon = RMOON;
  double dmoon = 2 * rmoon;
  int32 iflag;
  for (i = 0; i < 10; i++) 
    dcore[i] = 0;
  for (i = 0; i < 20; i++) 
    attr[i] = 0;
  /* nutation need not be in lunar and solar positions,
   * if mean sidereal time will be used */
  iflag = SEFLG_SPEED | SEFLG_EQUATORIAL | ifl;
  iflag  = iflag | SEFLG_XYZ;
  deltat = swe_deltat_ex_r(ctx, tjd_ut, ifl, serr);
  tjd = tjd_ut + deltat;
  /* moon in cartesian coordinates */
  if (swe_calc_r(ctx, tjd, SE_MOON, iflag, rm, serr) == ERR)
    return ERR;
  /* distance of moon from geocenter */
  dm = sqrt(square_sum(rm));
  /* sun in cartesian coordinates */
  if (swe_calc_r(ctx, tjd, SE_SUN, iflag, rs, serr) == ERR)
    return ERR;
  /* distance of sun from geocenter */
  ds = sqrt(square_sum(rs));
  for (i = 0; i < 3; i++) {
    x1[i] = rs[i] / ds;
    x2[i] = rm[i] / dm;
  }
  dctr = acos(swi_dot_prod_unit(x1, x2)) * RADTODEG;
  /* selenocentric sun */
  for (i = 0; i <= 2; i++)
    rs[i] -= rm[i];
  /* selenocentric earth */
  for (i = 0; i <= 2; i++)
    rm[i] = -rm[i];
  /* sun - earth vector */
  for (i = 0; i <= 2; i++)
    e[i] = (rm[i] - rs[i]);
  /* distance sun - earth */
  dsm = sqrt(square_sum(e));
  /* sun - earth unit vector */
  for (i = 0; i <= 2; i++)
    e[i] /= dsm;
  f1 = ((RSUN - REARTH) / dsm);
  cosf1 = sqrt(1 - f1 * f1);
  f2 = ((RSUN + REARTH) / dsm);
  cosf2 = sqrt(1 - f2 * f2);
  /* distance of earth from fundamental plane */
  s0 = -dot_prod(rm, e);
  /* distance of shadow axis from selenocenter */
  r0 = sqrt(dm * dm - s0 * s0);
  /* diameter of core shadow on fundamental plane */
         /* one 50th is added for effect of atmosphere, AA98, L4 */
  d0 = fabs(s0 / dsm * (DSUN - DEARTH) - DEARTH) * (1 + 1.0 / 50.0) / cosf1;
  /* diameter of half-shadow on fundamental plane */
  D0 = (s0 / dsm * (DSUN + DEARTH) + DEARTH) * (1 + 1.0 / 50.0) / cosf2;
  d0 /= cosf1;
  D0 /= cosf2;
  /* for better agreement with NASA: */
  d0 *= 0.99405;
  D0 *= 0.98813;
  dcore[0] = r0; 
  dcore[1] = d0;
  dcore[2] = D0;
  dcore[3] = cosf1;
  dcore[4] = cosf2;
  /**************************
   * phase and umbral magnitude
   **************************/
  retc = 0;
  if (d0 / 2 >= r0 + rmoon / cosf1) {
    retc = SE_ECL_TOTAL;
    attr[0] = (d0 / 2 - r0 + rmoon) / dmoon;
  } else if (d0 / 2 >= r0 - rmoon / cosf1) {
    retc = SE_ECL_PARTIAL;
    attr[0] = (d0 / 2 - r0 + rmoon) / dmoon;
  } else if (D0 / 2 >= r0 - rmoon / cosf2) {
    retc = SE_ECL_PENUMBRAL;
    attr[0] = 0;
  } else {
    if (serr != NULL)
      sprintf(serr, "no lunar eclipse at tjd = %f", tjd);
  }
  attr[8] = attr[0];
  /**************************
   * penumbral magnitude
   **************************/
  attr[1] = (D0 / 2 - r0 + rmoon) / dmoon;
  if (retc != 0)
    attr[7] = 180 - fabs(dctr);
  /* saros series and member */
  for (i = 0; i < NSAROS_LUNAR; i++) {
    d = (tjd_ut - saros_data_lunar[i].tstart) / SAROS_CYCLE;
    if (d < 0 && d * SAROS_CYCLE > -2) d = 0.0000001;
    if (d < 0) continue;
    j = (int) d;
    if ((d - j) * SAROS_CYCLE < 2) {
      attr[9] = (double) saros_data_lunar[i].series_no;
      attr[10] = (double) j + 1;
      break;
    }
    k = j + 1;
    if ((k - d) * SAROS_CYCLE < 2) {
      attr[9] = (double) saros_data_lunar[i].series_no;
      attr[10] = (double) k + 1;
      break;
    }
  }
  if (i == NSAROS_LUNAR) {
    attr[9] = attr[10] = -99999999;
  }
  return retc;
}

/* When is the next lunar eclipse?
 *
 * retflag	SE_ECL_TOTAL or SE_ECL_PENUMBRAL or SE_ECL_PARTIAL
 *
 * tret[0]	time of maximum eclipse
 * tret[1]	
 * tret[2]	time of partial phase begin (indices consistent with solar eclipses)
 * tret[3]	time of partial phase end
 * tret[4]	time of totality begin
 * tret[5]	time of totality end
 * tret[6]	time of penumbral phase begin
 * tret[7]	time of penumbral phase end
 */
static int32 swe_lun_eclipse_when_r_body(swe_ctx *ctx, double tjd_start, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  int i, j, m, n, o, i1 = 0, i2 = 0;
  int32 retflag = 0, retflag2 = 0;
  double t, tjd, tjd2, dt, dtint, dta, dtb;
  double T, T2, T3, T4, K, F, M, Mm;
  double E, Ff, F1, A1, Om;
  double xs[6], xm[6], dm, ds;
  double rsun, rearth, dcore[10];
  double dc[3], dctr;
  double twohr = 2.0 / 24.0;
  double tenmin = 10.0 / 24.0 / 60.0;
  double dt1 = 0, dt2 = 0;
  double kk;
  double attr[20];
  double dtstart, dtdiv;
  double xa[6], xb[6];
  int direction = 1;
  int32 iflag;
  int32 iflagcart;
  ifl &= SEFLG_EPHMASK; 
  swi_set_tid_acc(ctx, tjd_start, ifl, 0, serr);
  iflag = SEFLG_EQUATORIAL | ifl;
  iflagcart = iflag | SEFLG_XYZ;
  ifltype &= ~(SE_ECL_CENTRAL|SE_ECL_NONCENTRAL);
  if (ifltype & (SE_ECL_ANNULAR|SE_ECL_ANNULAR_TOTAL)) {
    ifltype &= ~(SE_ECL_ANNULAR|SE_ECL_ANNULAR_TOTAL);
    if (ifltype == 0) {
      if (serr != NULL) {
        strcpy(serr, "annular lunar eclipses don't exist");
      }
      return ERR; /* avoids infinite loop */
    }
  }
  if (ifltype == 0)
    ifltype = SE_ECL_TOTAL | SE_ECL_PENUMBRAL | SE_ECL_PARTIAL;
  if (backward)
    direction = -1;
  K = (int) ((tjd_start - J2000) / 365.2425 * 12.3685);
  K -= direction;
next_try:
  retflag = 0;
  for (i = 0; i <= 9; i++)
    tret[i] = 0;
  kk = K + 0.5;
  T = kk / 1236.85;
  T2 = T * T; T3 = T2 * T; T4 = T3 * T;
  Ff = F = swe_degnorm(160.7108 + 390.67050274 * kk
               - 0.0016341 * T2
               - 0.00000227 * T3
               + 0.000000011 * T4);
  if (Ff > 180)
    Ff -= 180;
  if (Ff > 21 && Ff < 159) { 	/* no eclipse possible */
    K += direction;
    goto next_try;
  }
  /* approximate time of geocentric maximum eclipse
   * formula from Meeus, German, p. 381 */
  tjd = 2451550.09765 + 29.530588853 * kk
                      + 0.0001337 * T2
                      - 0.000000150 * T3
                      + 0.00000000073 * T4;
  M = swe_degnorm(2.5534 + 29.10535669 * kk
                      - 0.0000218 * T2
                      - 0.00000011 * T3);
  Mm = swe_degnorm(201.5643 + 385.81693528 * kk
                      + 0.1017438 * T2
                      + 0.00001239 * T3
                      + 0.000000058 * T4);
  Om = swe_degnorm(124.7746 - 1.56375580 * kk
                      + 0.0020691 * T2
                      + 0.00000215 * T3);
  E = 1 - 0.002516 * T - 0.0000074 * T2;
  A1 = swe_degnorm(299.77 + 0.107408 * kk - 0.009173 * T2);
  M *= DEGTORAD;
  Mm *= DEGTORAD;
  F *= DEGTORAD;
  Om *= DEGTORAD;
  F1 = F - 0.02665 * sin(Om) * DEGTORAD;
  A1 *= DEGTORAD;
  tjd = tjd - 0.4075 * sin(Mm)
            + 0.1721 * E * sin(M)
            + 0.0161 * sin(2 * Mm)
            - 0.0097 * sin(2 * F1)
            + 0.0073 * E * sin(Mm - M)
            - 0.0050 * E * sin(Mm + M)
            - 0.0023 * sin(Mm - 2 * F1)
            + 0.0021 * E * sin(2 * M)
            + 0.0012 * sin(Mm + 2 * F1)
            + 0.0006 * E * sin(2 * Mm + M)
            - 0.0004 * sin(3 * Mm)
            - 0.0003 * E * sin(M + 2 * F1)
            + 0.0003 * sin(A1)
            - 0.0002 * E * sin(M - 2 * F1)
            - 0.0002 * E * sin(2 * Mm - M)
            - 0.0002 * sin(Om);
  /* 
   * precise computation:
   * time of maximum eclipse (if eclipse) = 
   * minimum selenocentric angle between sun and earth edges.
   * After this time has been determined, check
   * whether or not an eclipse is taking place with
   * the function lun_eclipse_how().
   */
  dtstart = 0.1;
  if (tjd < 2100000 || tjd > 2500000)	// was tjd < 2000000 until 26-aug-22
    dtstart = 5;
  dtdiv = 4;
  for (j = 0, dt = dtstart;
       dt > 0.001; 
       j++, dt /= dtdiv) {
    for (i = 0, t = tjd - dt; i <= 2; i++, t += dt) {
      if (swe_calc_r(ctx, t, SE_SUN, iflagcart, xs, serr) == ERR)
        return ERR;
      if (swe_calc_r(ctx, t, SE_MOON, iflagcart, xm, serr) == ERR)
        return ERR;
      for (m = 0; m < 3; m++) {
        xs[m] -= xm[m];	/* selenocentric sun */
        xm[m] = -xm[m];	/* selenocentric earth */
      }
      ds = sqrt(square_sum(xs));
      dm = sqrt(square_sum(xm));
      for (m = 0; m < 3; m++) {
        xa[m] = xs[m] / ds;
        xb[m] = xm[m] / dm;
      }
      dc[i] = acos(swi_dot_prod_unit(xa, xb)) * RADTODEG;
      rearth = asin(REARTH / dm) * RADTODEG;
      rsun = asin(RSUN / ds) * RADTODEG;
      dc[i] -= (rearth + rsun);
    }
    find_maximum(dc[0], dc[1], dc[2], dt, &dtint, &dctr);
    tjd += dtint + dt;
  }
  tjd2 = tjd - swe_deltat_ex_r(ctx, tjd, ifl, serr);
  tjd2 = tjd - swe_deltat_ex_r(ctx, tjd2, ifl, serr);
  tjd = tjd - swe_deltat_ex_r(ctx, tjd2, ifl, serr);
  if ((retflag = swe_lun_eclipse_how_r(ctx, tjd, ifl, NULL, attr, serr)) == ERR)
    return retflag;
  if (retflag == 0) {
    K += direction;
    goto next_try;
  }
  tret[0] = tjd;
  if ((backward && tret[0] >= tjd_start - 0.0001) 
    || (!backward && tret[0] <= tjd_start + 0.0001)) {
    K += direction;
    goto next_try;
  }
  /* 
   * check whether or not eclipse type found is wanted
   */
  /* non penumbral eclipse is wanted: */
  if (!(ifltype & SE_ECL_PENUMBRAL) && (retflag & SE_ECL_PENUMBRAL)) {
    K += direction;
    goto next_try;
  }
  /* non partial eclipse is wanted: */
  if (!(ifltype & SE_ECL_PARTIAL) && (retflag & SE_ECL_PARTIAL)) {
    K += direction;
    goto next_try;
  }
  /* annular-total eclipse will be discovered later */
  if (!(ifltype & (SE_ECL_TOTAL)) && (retflag & SE_ECL_TOTAL)) {
    K += direction;
    goto next_try;
  }
  /* 
   * n = 0: times of eclipse begin and end 
   * n = 1: times of totality begin and end 
   * n = 2: times of center line begin and end 
   */
  if (retflag & SE_ECL_PENUMBRAL)
    o = 0;
  else if (retflag & SE_ECL_PARTIAL)
    o = 1;
  else 
    o = 2;
  dta = twohr;
  dtb = tenmin;
  for (n = 0; n <= o; n++) {
    if (n == 0) {
      i1 = 6; i2 = 7;
    } else if (n == 1) {
      i1 = 2; i2 = 3;
    } else if (n == 2) {
      i1 = 4; i2 = 5;
    }
#if 1
    for (i = 0, t = tjd - dta; i <= 2; i += 1, t += dta) {
      if ((retflag2 = lun_eclipse_how(ctx, t, ifl, attr, dcore, serr)) == ERR)
        return retflag2;
      if (n == 0)
        dc[i] = dcore[2] / 2 + RMOON / dcore[4] - dcore[0];
      else if (n == 1)
        dc[i] = dcore[1] / 2 + RMOON / dcore[3] - dcore[0];
      else if (n == 2)
        dc[i] = dcore[1] / 2 - RMOON / dcore[3] - dcore[0];
    }
    find_zero(dc[0], dc[1], dc[2], dta, &dt1, &dt2);
    dtb = (dt1 + dta) / 2;
    tret[i1] = tjd + dt1 + dta;
    tret[i2] = tjd + dt2 + dta;
#else
    tret[i1] = tjd - dtb;
    tret[i2] = tjd + dtb;
#endif
    for (m = 0, dt = dtb / 2; m < 3; m++, dt /= 2) {
      for (j = i1; j <= i2; j += (i2 - i1)) {
        for (i = 0, t = tret[j] - dt; i < 2; i++, t += dt) {
          if ((retflag2 = lun_eclipse_how(ctx, t, ifl, attr, dcore, serr)) == ERR)
            return retflag2;
          if (n == 0)
            dc[i] = dcore[2] / 2 + RMOON / dcore[4] - dcore[0];
          else if (n == 1)
            dc[i] = dcore[1] / 2 + RMOON / dcore[3] - dcore[0];
          else if (n == 2)
            dc[i] = dcore[1] / 2 - RMOON / dcore[3] - dcore[0];
        }
        dt1 = dc[1] / ((dc[1] - dc[0]) / dt);
        tret[j] -= dt1;
      }
    }
  }
  return retflag;
}

int32 CALL_CONV swe_lun_eclipse_when_r(swe_ctx *ctx, double tjd_start, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_lun_eclipse_when_r_body(ctx, tjd_start, ifl, ifltype, tret, backward, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_lun_eclipse_when(double tjd_start, int32 ifl, int32 ifltype,
     double *tret, int32 backward, char *serr)
{
  return swe_lun_eclipse_when_r(swi_default_ctx(), tjd_start, ifl, ifltype, tret, backward, serr);
}

/* When is the next lunar eclipse, observable at a geographic position?
 *
 * retflag	SE_ECL_TOTAL or SE_ECL_PENUMBRAL or SE_ECL_PARTIAL
 *
 * tret[0]	time of maximum eclipse
 * tret[1]	
 * tret[2]	time of partial phase begin (indices consistent with solar eclipses)
 * tret[3]	time of partial phase end
 * tret[4]	time of totality begin
 * tret[5]	time of totality end
 * tret[6]	time of penumbral phase begin
 * tret[7]	time of penumbral phase end
 * tret[8]	time of moonrise, if it occurs during the eclipse
 * tret[9]	time of moonset, if it occurs during the eclipse
 *
 * attr[0]      umbral magnitude at tjd
 * attr[1]      penumbral magnitude
 * attr[4]      azimuth of moon at tjd
 * attr[5]      true altitude of moon above horizon at tjd
 * attr[6]      apparent altitude of moon above horizon at tjd
 * attr[7]      distance of moon from opposition in degrees
 * attr[8]      umbral magnitude at tjd (= attr[0])
 * attr[9]      saros series number
 * attr[10]     saros series member number
 *         declare as attr[20] at least !
 */
static int32 swe_lun_eclipse_when_loc_r_body(swe_ctx *ctx, double tjd_start, int32 ifl, 
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  int32 retflag = 0, retflag2 = 0, retc;
  double tjdr, tjds, tjd_max = 0;
  int i;
  if (geopos != NULL && (geopos[2] < SEI_ECL_GEOALT_MIN || geopos[2] > SEI_ECL_GEOALT_MAX)) {
    if (serr != NULL)
      sprintf(serr, "location for eclipses must be between %.0f and %.0f m above sea", SEI_ECL_GEOALT_MIN, SEI_ECL_GEOALT_MAX);
    return ERR;
  }
  ifl &= ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
next_lun_ecl:
  if ((retflag = swe_lun_eclipse_when_r(ctx, tjd_start, ifl, 0, tret, backward, serr)) == ERR) {
    return ERR;
  }
  /*  
   * visibility of eclipse phases 
   */
  retflag = 0;
  for (i = 7; i >= 0; i--) {
    if (i == 1) continue;
    if (tret[i] == 0) continue;
    if ((retflag2 = swe_lun_eclipse_how_r(ctx, tret[i], ifl, geopos, attr, serr)) == ERR)
      return ERR;
    if (attr[6] > 0) {  /* moon above horizon, using app. alt. */
      retflag |= SE_ECL_VISIBLE;
      switch(i) {
      case 0: retflag |= SE_ECL_MAX_VISIBLE; break;
      case 2: retflag |= SE_ECL_PARTBEG_VISIBLE; break;
      case 3: retflag |= SE_ECL_PARTEND_VISIBLE; break;
      case 4: retflag |= SE_ECL_TOTBEG_VISIBLE; break;
      case 5: retflag |= SE_ECL_TOTEND_VISIBLE; break;
      case 6: retflag |= SE_ECL_PENUMBBEG_VISIBLE; break;
      case 7: retflag |= SE_ECL_PENUMBEND_VISIBLE; break;
      default:  break;
      }
    }
  }
  if (!(retflag & SE_ECL_VISIBLE)) {
    if (backward)
      tjd_start = tret[0] - 25;
    else
      tjd_start = tret[0] + 25;
    goto next_lun_ecl;
  }
  /* moon rise and moon set */
  tjd_max = tret[0];
  if ((retc = swe_rise_trans_r(ctx, tret[6] - 0.001, SE_MOON, NULL, ifl, SE_CALC_RISE|SE_BIT_DISC_BOTTOM, geopos, 0, 0, &tjdr, serr)) == ERR)
    return ERR;
  if (retc >= 0 && (retc = swe_rise_trans_r(ctx, tret[6] - 0.001, SE_MOON, NULL, ifl, SE_CALC_SET|SE_BIT_DISC_BOTTOM, geopos, 0, 0, &tjds, serr)) == ERR)
    return ERR;
  if (retc >= 0) {
    if (tjds < tret[6] || (tjds > tjdr && tjdr > tret[7])) {
      if (backward)
	tjd_start = tret[0] - 25;
      else
	tjd_start = tret[0] + 25;
      goto next_lun_ecl;
    }
    if (tjdr > tret[6] && tjdr < tret[7]) {
      tret[6] = 0;
      for (i = 2; i <= 5; i++) {
	if (tjdr > tret[i]) 
	  tret[i] = 0;
      }
      tret[8] = tjdr;
      if (tjdr > tret[0]) {
	tjd_max = tjdr;
      }
    }
    if (tjds > tret[6] && tjds < tret[7]) {
      tret[7] = 0;
      for (i = 2; i <= 5; i++) {
	if (tjds < tret[i]) 
	  tret[i] = 0;
      }
      tret[9] = tjds;
      if (tjds < tret[0]) {
	tjd_max = tjds;
      }
    }
  }
  tret[0] = tjd_max;
  if ((retflag2 = swe_lun_eclipse_how_r(ctx, tjd_max, ifl, geopos, attr, serr)) == ERR)
    return ERR;
  if (retflag2 == 0) {
    if (backward)
      tjd_start = tret[0] - 25;
    else
      tjd_start = tret[0] + 25;
    goto next_lun_ecl;
  }
  retflag |= (retflag2 & SE_ECL_ALLTYPES_LUNAR);
  return retflag;
}

int32 CALL_CONV swe_lun_eclipse_when_loc_r(swe_ctx *ctx, double tjd_start, int32 ifl, 
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_lun_eclipse_when_loc_r_body(ctx, tjd_start, ifl, geopos, tret, attr, backward, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_lun_eclipse_when_loc(double tjd_start, int32 ifl, 
     double *geopos, double *tret, double *attr, int32 backward, char *serr)
{
  return swe_lun_eclipse_when_loc_r(swi_default_ctx(), tjd_start, ifl, geopos, tret, attr, backward, serr);
}

/* 
 * function calculates planetary phenomena
 * 
 * attr[0] = phase angle (earth-planet-sun)
 * attr[1] = phase (illumined fraction of disc)
 * attr[2] = elongation of planet
 * attr[3] = apparent diameter of disc
 * attr[4] = apparent magnitude
 * attr[5] = geocentric horizontal parallax (Moon)
 *         declare as attr[20] at least !
 *
 * Note: the lunar magnitude is quite a complicated thing,
 * but our algorithm is very simple. 
 * The phase of the moon, its distance from the earth and
 * the sun is considered, but no other factors.
 * 
 */
#define EULER 2.718281828459
#define NMAG_ELEM  (SE_VESTA + 1)
#define MAG_MALLAMA_2018  1
#define MAG_MOON_VREIJS   1
/* Magnitudes according to:
 * - "Explanatory Supplement to the Astronomical Almanac" 1986.
 * Magnitudes for Mercury and Venus:
 * - James L. Hilton, "Improving the Visual Magnitudes of the Planets in
 *   the Astronomical Almanac, I. Mercury and Venus.", The Astronomical
 *   Journal, 129:2902-2906, 2005 June 
 * - James L. Hilton, "Erratum...", The Astronomical Journal,
 *   130:2928, 2005 December
 * For the ring system of Saturn:
 * - Jean Meeus, Astronomical Algorithms, p. 301ff. (German edition 1999(2), p. 329ff.)
 *   */
static const double mag_elem[NMAG_ELEM][4] = {
                /* DTV-Atlas Astronomie, p. 32 */
                {-26.86, 0, 0, 0}, /* Sun */
                {-12.55, 0, 0, 0}, /* Moon */
                /* IAU 1986 */
		/* Venus and Mercury values are obsolete, are only kept
		 * as place holders. Correct handling according to Hilton
		 * 2005 is seen further below in the code. */
                {-0.42, 3.80, -2.73, 2.00}, /* Mercury (obsolete, but don't delete this line!) */
                {-4.40, 0.09, 2.39, -0.65}, /* Venus (obsolete, but don't delete this line!) */
                {- 1.52, 1.60, 0, 0},   /* Mars */
                {- 9.40, 0.5, 0, 0},    /* Jupiter */
                {- 8.88, -2.60, 1.25, 0.044},   /* Saturn */
                {- 7.19, 0.0, 0, 0},    /* Uranus */
                {- 6.87, 0.0, 0, 0},    /* Neptune */
                {- 1.00, 0.0, 0, 0},    /* Pluto */
                {99, 0, 0, 0},          /* nodes and apogees */
                {99, 0, 0, 0},
                {99, 0, 0, 0},
                {99, 0, 0, 0},
                {99, 0, 0, 0},          /* Earth */
                /* from Bowell data base */
                {6.5, 0.15, 0, 0},      /* Chiron */
                {7.0, 0.15, 0, 0},      /* Pholus */
                {3.34, 0.12, 0, 0},     /* Ceres */
                {4.13, 0.11, 0, 0},     /* Pallas */
                {5.33, 0.32, 0, 0},     /* Juno */
                {3.20, 0.32, 0, 0},     /* Vesta */
                };
static int32 swe_pheno_r_body(swe_ctx *ctx, double tjd, int32 ipl, int32 iflag, double *attr, char *serr)
{
  int i;
  double xx[6], xx2[6], xxs[6], lbr[6], lbr2[6], dt = 0, dd;
  double fac;
  double T, in, om, sinB;
  double ph1, ph2, me[2];
  int32 iflagp, epheflag, retflag, epheflag2;
  char serr2[AS_MAXCH];
  *serr2 = '\0';
  iflag &= ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO */
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  for (i = 0; i < 20; i++)
    attr[i] = 0;
  /* Ceres - Vesta must be SE_CERES etc., not 10001 etc. */
  if (ipl > SE_AST_OFFSET && ipl <= SE_AST_OFFSET + 4)
        ipl = ipl - SE_AST_OFFSET - 1 + SE_CERES;
#ifndef SWE_UPSTREAM_COMPAT
  /* ⚠️ SE_ECL_NUT is a valid swe_calc() number but not a body: upstream
   * computed it and then indexed pla_diam[] and mag_elem[] with -1, out of
   * bounds, and returned whatever it read as a phase and a magnitude. */
  if (ipl < SE_SUN) {
    if (serr != NULL)
      sprintf(serr, "illegal planet number %d.", ipl);
    return ERR;
  }
#endif
  iflag = iflag & (SEFLG_EPHMASK | 
                   SEFLG_TRUEPOS | 
                   SEFLG_J2000 | 
                   SEFLG_NONUT |
                   SEFLG_NOGDEFL |
                   SEFLG_NOABERR |
                   SEFLG_TOPOCTR);
  iflagp = iflag & (SEFLG_EPHMASK | 
                   SEFLG_TRUEPOS | 
                   SEFLG_J2000 | 
                   SEFLG_NONUT |
                   SEFLG_NOABERR);
  iflagp |= SEFLG_HELCTR;                
  epheflag = iflag & SEFLG_EPHMASK;
  /*  
   * geocentric planet
   */
  if ((retflag = swe_calc_r(ctx, tjd, (int) ipl, iflag | SEFLG_XYZ, xx, serr)) == ERR)
    /* int cast can be removed when swe_calc() gets int32 ipl definition */
    return ERR;
  // check epheflag and adjust iflag
  epheflag2 = retflag & SEFLG_EPHMASK;
  if (epheflag != epheflag2) {
    iflag &= ~epheflag;
    iflagp &= ~epheflag;
    iflag |= epheflag2;
    iflagp |= epheflag2;
    epheflag = epheflag2;
  }
  if (swe_calc_r(ctx, tjd, (int) ipl, iflag, lbr, serr) == ERR)
    /* int cast can be removed when swe_calc() gets int32 ipl definition */
    return ERR;
  /* if moon, we need sun as well, for magnitude */
  if (ipl == SE_MOON) {
    if (swe_calc_r(ctx, tjd, SE_SUN, iflag | SEFLG_XYZ, xxs, serr) == ERR)
      return ERR;
  }
  if (ipl != SE_SUN && ipl != SE_EARTH &&
    ipl != SE_MEAN_NODE && ipl != SE_TRUE_NODE &&
    ipl != SE_MEAN_APOG && ipl != SE_OSCU_APOG) {
    /*
     * light time planet - earth
     */
    dt = lbr[2] * AUNIT / CLIGHT / 86400.0;     
        if (iflag & SEFLG_TRUEPOS)
      dt = 0;
    /* 
     * heliocentric planet at tjd - dt
     */
    if (swe_calc_r(ctx, tjd - dt, (int) ipl, iflagp | SEFLG_XYZ, xx2, serr) == ERR)
    /* int cast can be removed when swe_calc() gets int32 ipl definition */
      return ERR;
    if (swe_calc_r(ctx, tjd - dt, (int) ipl, iflagp, lbr2, serr) == ERR)
    /* int cast can be removed when swe_calc() gets int32 ipl definition */
      return ERR;
    /*
     * phase angle
     */
    attr[0] = acos(swi_dot_prod_unit(xx, xx2)) * RADTODEG;
    /*
     * phase
     */
    attr[1] = (1 + cos(attr[0] * DEGTORAD)) / 2;
  }
  /*
   * apparent diameter of disk
   */
  if (ipl < NDIAM)
    dd = pla_diam[ipl];
  else if (ipl > SE_AST_OFFSET)
    dd = ctx->ast_diam * 1000;	/* km -> m */
  else
    dd = 0;
  if (lbr[2] < dd / 2 / AUNIT)
    attr[3] = 180;  /* assume position on surface of earth */
  else
    attr[3] = asin(dd / 2 / AUNIT / lbr[2]) * 2 * RADTODEG;
  /* 
   * apparent magnitude 
   */
  if (ipl > SE_AST_OFFSET || (ipl < NMAG_ELEM && mag_elem[ipl][0] < 99)) {
    if (ipl == SE_SUN) {
      /* ratio apparent diameter : average diameter */
      fac = attr[3] / (asin(pla_diam[SE_SUN] / 2.0 / AUNIT) * 2 * RADTODEG);
      fac *= fac;
      attr[4] = mag_elem[ipl][0] - 2.5 * log10(fac);
    } else if (ipl == SE_MOON) {
#if MAG_MOON_VREIJS
     // double a=fabs(attr[0]);
     double a = attr[0];
     if (a<=147.1385465) {
       /* formula according to Allen, C.W., 1976, Astrophysical Quantities */
       attr[4] = -21.62 + 0.026 * fabs(a) + 0.000000004 * pow(a, 4);
       attr[4]+=5 * log10(lbr[2] * lbr2[2] * AUNIT / EARTH_RADIUS);
     } else {
       /* using the cube phase angle proposed by Samaha (Samaha, A.E.; Asaad,
	A. S. and Mikhail, J. S. (1969).
	Visibility of the New Moon, Bulletin of Observatory Helwan, 84), and VR
	adjusted the stitch phase (align Allen's and Samaha's magnitude) 
	of 147.14degrees.
       */
       attr[4] = -4.5444 - (2.5 * log10(pow(180 - a, 3)));
       attr[4]+=5 * log10(lbr[2] * lbr2[2] * AUNIT / EARTH_RADIUS);
     } 
#else
      /* formula according to Allen, C.W., 1976, Astrophysical Quantities */
      attr[4] = -21.62 + 5 * log10(lbr[2] * lbr2[2] * AUNIT / EARTH_RADIUS) + 0.026 * fabs(attr[0]) + 0.000000004 * pow(attr[0], 4);
#endif      
#if MAG_MALLAMA_2018
    // see: A. Mallama, J.Hilton,
    // "ComputingApparentPlanetaryMagnitudesforTheAstronomicalAlmanac" (2018)
    // https://arxiv.org/ftp/arxiv/papers/1808/1808.01973.pdf
    } else if (ipl == SE_MERCURY) {
      double a = attr[0];
      double a2 = a * a; double a3 = a2 * a; double a4 = a3 * a; double a5 = a4 * a; double a6 = a5 * a; 
      attr[4] = -0.613 + a * 6.3280E-02 - a2 * 1.6336E-03 + a3 * 3.3644E-05 - a4 * 3.4265E-07 + a5 * 1.6893E-09 - a6 * 3.0334E-12;
      attr[4] += 5 * log10(lbr2[2] * lbr[2]);
    } else if (ipl == SE_VENUS) {
      double a = attr[0];
      double a2 = a * a; double a3 = a2 * a; double a4 = a3 * a; 
      if (a <= 163.7)
	attr[4] = -4.384 - a * 1.044E-03 + a2 * 3.687E-04 - a3 * 2.814E-06 + a4 * 8.938E-09;
      else 
	attr[4] = 236.05828 - a * 2.81914E+00 + a2 * 8.39034E-03;
      attr[4] += 5 * log10(lbr2[2] * lbr[2]);
      if (attr[0] > 179.0)
        sprintf(serr2, "magnitude value for Venus at phase angle i=%.1f is bad; formula is valid only for i < 179.0", attr[0]);
    } else if (ipl == SE_MARS) {
      double a = attr[0];
      double a2 = a * a; 
      /* With the following formulae, the terms +L(λe)+L(LS) have been omitted.
       * They are "the magnitude corrections for the longitude of the sub-Earth 
       * meridian of the illuminated disk and the longitude of the vernal 
       * equinox,respectively".
       * The apparent magnitude of Mars changes considerably within hours, 
       * depending on the surface that is seen.
       * Note that the maximum phase angle of mars as seen from earth is
       * about 45°.
       * The deviation of this simplified solution from Horizons is
       * smaller than 0.1m.
       */
      if (a <= 50.0)
	attr[4] = -1.601 + a * 0.02267 - a2 * 0.0001302; 
      else  // irrelevant to earth-centered observation
	attr[4] = -0.367 - a * 0.02573 + a2 * 0.0003445;
      attr[4] += 5 * log10(lbr2[2] * lbr[2]);
    } else if (ipl == SE_JUPITER) {
      /* the phase angle of Jupiter never exceeds 12°. */
      double a = attr[0];
      double a2 = a * a; 
      attr[4] = -9.395 - a * 3.7E-04 + a2 * 6.16E-04;
      attr[4] += 5 * log10(lbr2[2] * lbr[2]);
    } else if (ipl == SE_SATURN) {
      double a = attr[0];
      double sinB2;
      T = (tjd - dt - J2000) / 36525.0;
      in = (28.075216 - 0.012998 * T + 0.000004 * T * T) * DEGTORAD;
      om = (169.508470 + 1.394681 * T + 0.000412 * T * T) * DEGTORAD;
      // B is "mean tilt of the ring plane to the Earth and Sun (If the Earth
      // and Sun are on opposite sides of the ring plane B = 0)" according to Hilton:
      // https://syrte.obspm.fr/astro/journees2019/journees_pdf/SessionV_1/HiltonStewart_final.pdf
      // Mallama does not provide B. We derive it from 
      // Meeus, p. 301ff. (German version 329ff.)
      // There are small differences from Horizons < 0.02m.
      sinB = (sin(in) * cos(lbr[1] * DEGTORAD) 
                    * sin(lbr[0] * DEGTORAD - om)
                    - cos(in) * sin(lbr[1] * DEGTORAD));
      sinB2 = (sin(in) * cos(lbr2[1] * DEGTORAD) 
                    * sin(lbr2[0] * DEGTORAD - om)
                    - cos(in) * sin(lbr2[1] * DEGTORAD));
      sinB = fabs(sin((asin(sinB) + asin(sinB2)) / 2.0));/**/
      attr[4] = -8.914 - 1.825 * sinB + 0.026 * a - 0.378 * sinB * pow(2.7182818,-2.25 * a);
      attr[4] += 5 * log10(lbr2[2] * lbr[2]);
    } else if (ipl == SE_URANUS) { 
      // This is a simplified solution ignoring the term depending on
      // sub-Earth latitude. The difference from Horizons is +-0.03m.
      double a = attr[0];
      double a2 = a * a; 
      double fi_ = 0; // sub-Earth latitude in deg; ignored here
      attr[4] = -7.110 - 8.4E-04 * fi_ + a * 6.587E-3 + a2 * 1.045E-4;
      attr[4] += 5 * log10(lbr2[2] * lbr[2]);
      // instead of the term with fi_, we do subtract the 0.05m.
      // the remaining error is +-0.03m
      attr[4] -= 0.05; 
    } else if (ipl == SE_NEPTUNE) { 
      if (tjd < 2444239.5) {
	attr[4] = -6.89;
      } else if (tjd <= 2451544.5) {
	attr[4] = -6.89 - 0.0055 * (tjd - 2444239.5) / 365.25;
	// Mallama has 0.0054, but that would make the curve discontinuos
	// Nevertheless, JPL Horizons has 0.0054 and the discontinuity
      } else {
	attr[4] = -7.00;
      }
      attr[4] += 5 * log10(lbr2[2] * lbr[2]);
#else
    } else if (ipl == SE_SATURN) {
      double u1, u2, du;
      /* rings are considered according to Meeus, p. 301ff. (German version 329ff.) */
      T = (tjd - dt - J2000) / 36525.0;
      in = (28.075216 - 0.012998 * T + 0.000004 * T * T) * DEGTORAD;
      om = (169.508470 + 1.394681 * T + 0.000412 * T * T) * DEGTORAD;
      sinB = fabs(sin(in) * cos(lbr[1] * DEGTORAD) 
                    * sin(lbr[0] * DEGTORAD - om)
                    - cos(in) * sin(lbr[1] * DEGTORAD));
      u1 = atan2(sin(in) * tan(lbr2[1] * DEGTORAD) 
                             + cos(in) * sin(lbr2[0] * DEGTORAD - om), 
                        cos(lbr2[0] * DEGTORAD - om)) * RADTODEG;
      u2 = atan2(sin(in) * tan(lbr[1] * DEGTORAD) 
                             + cos(in) * sin(lbr[0] * DEGTORAD - om), 
                        cos(lbr[0] * DEGTORAD - om)) * RADTODEG;
      du = swe_degnorm(u1 - u2);
      if (du > 10) 
        du = 360 - du;
      attr[4] = 5 * log10(lbr2[2] * lbr[2])
                  + mag_elem[ipl][1] * sinB
                  + mag_elem[ipl][2] * sinB * sinB
                  + mag_elem[ipl][3] * du
                  + mag_elem[ipl][0];
#endif
    } else if (ipl < SE_CHIRON) {
      attr[4] = 5 * log10(lbr2[2] * lbr[2])
                  + mag_elem[ipl][1] * attr[0] /100.0
                  + mag_elem[ipl][2] * attr[0] * attr[0] / 10000.0
                  + mag_elem[ipl][3] * attr[0] * attr[0] * attr[0] / 1000000.0
                  + mag_elem[ipl][0];
    } else if (ipl < NMAG_ELEM || ipl > SE_AST_OFFSET) { /* other planets, asteroids */
      ph1 = pow(EULER, -3.33 * pow(tan(attr[0] * DEGTORAD / 2), 0.63));
      ph2 = pow(EULER, -1.87 * pow(tan(attr[0] * DEGTORAD / 2), 1.22));
      if (ipl < NMAG_ELEM) {    /* other planets, main asteroids */
        me[0] = mag_elem[ipl][0];
        me[1] = mag_elem[ipl][1];
      } else if (ipl == SE_AST_OFFSET + 1566) { 
                  /* Icarus has elements from JPL database */
                me[0] = 16.9;
                me[1] = 0.15;
      } else {      /* other asteroids */
        me[0] = ctx->ast_H;
        me[1] = ctx->ast_G;
      }
      attr[4] = 5 * log10(lbr2[2] * lbr[2])
          + me[0]
          - 2.5 * log10((1 - me[1]) * ph1 + me[1] * ph2);
    } else { /* ficticious bodies */
      attr[4] = 0;
    }
  }
  if (ipl != SE_SUN && ipl != SE_EARTH) {
    /* 
     * elongation of planet
     */
    if (swe_calc_r(ctx, tjd, SE_SUN, iflag | SEFLG_XYZ, xx2, serr) == ERR)
      return ERR;
    if (swe_calc_r(ctx, tjd, SE_SUN, iflag, lbr2, serr) == ERR)
      return ERR;
    attr[2] = acos(swi_dot_prod_unit(xx, xx2)) * RADTODEG;
  }
  /* horizontal parallax */
  if (ipl == SE_MOON) {
    double sinhp, xm[6];
    /* geocentric horizontal parallax */
    /* Expl.Suppl. to the AA 1984, p.400 */
    if (swe_calc_r(ctx, tjd, (int) ipl, epheflag|SEFLG_TRUEPOS|SEFLG_EQUATORIAL|SEFLG_RADIANS, xm, serr) == ERR)
      /* int cast can be removed when swe_calc() gets int32 ipl definition */
      return ERR;
    sinhp = EARTH_RADIUS / xm[2] / AUNIT;
    attr[5] = asin(sinhp) / DEGTORAD;
    /* topocentric horizontal parallax */
    if (iflag & SEFLG_TOPOCTR) {
      if (swe_calc_r(ctx, tjd, (int) ipl, epheflag|SEFLG_XYZ|SEFLG_TOPOCTR, xm, serr) == ERR)
	return ERR;
      if (swe_calc_r(ctx, tjd, (int) ipl, epheflag|SEFLG_XYZ, xx, serr) == ERR)
	return ERR;
      attr[5] = acos(swi_dot_prod_unit(xm, xx)) / DEGTORAD;
    }
  }
  if (*serr2 != '\0' && serr != NULL)
    strcpy(serr, serr2);
  return iflag;
}

int32 CALL_CONV swe_pheno_r(swe_ctx *ctx, double tjd, int32 ipl, int32 iflag, double *attr, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_pheno_r_body(ctx, tjd, ipl, iflag, attr, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_pheno(double tjd, int32 ipl, int32 iflag, double *attr, char *serr)
{
  return swe_pheno_r(swi_default_ctx(), tjd, ipl, iflag, attr, serr);
}

static int32 swe_pheno_ut_r_body(swe_ctx *ctx, double tjd_ut, int32 ipl, int32 iflag, double *attr, char *serr)
{
  double deltat;
  int32 retflag = OK;
  int32 epheflag = iflag & SEFLG_EPHMASK;
  if (epheflag == 0) {
    epheflag = SEFLG_SWIEPH;
    iflag |= SEFLG_SWIEPH;
  }
  deltat = swe_deltat_ex_r(ctx, tjd_ut, iflag, serr);
  retflag = swe_pheno_r(ctx, tjd_ut + deltat, ipl, iflag, attr, serr);
  /* if ephe required is not ephe returned, adjust delta t: */
  if ((retflag & SEFLG_EPHMASK) != epheflag) {
    deltat = swe_deltat_ex_r(ctx, tjd_ut, retflag, serr);
    retflag = swe_pheno_r(ctx, tjd_ut + deltat, ipl, iflag, attr, serr);
  }
  return retflag;
}

int32 CALL_CONV swe_pheno_ut_r(swe_ctx *ctx, double tjd_ut, int32 ipl, int32 iflag, double *attr, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_pheno_ut_r_body(ctx, tjd_ut, ipl, iflag, attr, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_pheno_ut(double tjd_ut, int32 ipl, int32 iflag, double *attr, char *serr)
{
  return swe_pheno_ut_r(swi_default_ctx(), tjd_ut, ipl, iflag, attr, serr);
}

static int find_maximum(double y00, double y11, double y2, double dx, 
                        double *dxret, double *yret)
{
  double a, b, c, x, y;
  c = y11;
  b = (y2 - y00) / 2.0;
  a = (y2 + y00) / 2.0 - c;
  x = -b / 2 / a;
  y = (4 * a * c - b * b) / 4 / a;
  *dxret = (x - 1) * dx;
  if (yret != NULL)
    *yret = y;
  return OK;
}

static int find_zero(double y00, double y11, double y2, double dx, 
                        double *dxret, double *dxret2)
{
  double a, b, c, x1, x2;
  c = y11;
  b = (y2 - y00) / 2.0;
  a = (y2 + y00) / 2.0 - c;
  if (b * b - 4 * a * c < 0) 
    return ERR;
  x1 = (-b + sqrt(b * b - 4 * a * c)) / 2 / a;
  x2 = (-b - sqrt(b * b - 4 * a * c)) / 2 / a;
  *dxret = (x1 - 1) * dx;
  *dxret2 = (x2 - 1) * dx;
  return OK;
}

double rdi_twilight(int32 rsmi)
{
  double rdi = 0;
  if (rsmi & SE_BIT_CIVIL_TWILIGHT) 
    rdi = 6; 
  if (rsmi & SE_BIT_NAUTIC_TWILIGHT)
    rdi = 12; 
  if (rsmi & SE_BIT_ASTRO_TWILIGHT)
    rdi = 18; 
  return rdi;
}

static double get_sun_rad_plus_refr(int32 ipl, double dd, int32 rsmi, double refr) 
{
  double rdi = 0;
  if (rsmi & SE_BIT_FIXED_DISC_SIZE) {
    if (ipl == SE_SUN)
      dd = 1.0;
    else if (ipl == SE_MOON)
      dd = 0.00257;
  }
  /* apparent radius of disc */
  if (!(rsmi & SE_BIT_DISC_CENTER)) 
    rdi = asin( pla_diam[ipl] / 2.0 / AUNIT / dd) * RADTODEG;
  if (rsmi & SE_BIT_DISC_BOTTOM) 
    rdi = -rdi;
  if (!(rsmi & SE_BIT_NO_REFRACTION)) {
    rdi += refr; // (34.5 / 60.0);
  }
  return rdi;
}

/* Simple fast algorithm for risings and settings of 
 * - planets Sun, Moon, Mercury - Pluto + Lunar Nodes and Fixed stars
 * Does not work well for geographic latitudes
 * > 65 N/S for the Sun
 * > 60 N/S for the Moon and the planets
 * and is called only for latitudes smaller than this.
 */
static int32 rise_set_fast(
               swe_ctx *ctx, double tjd_ut, int32 ipl,
	       int32 epheflag, int32 rsmi,
               double *dgeo, 
	       double atpress, double attemp,
               double *tret,
               char *serr)
{
  int i;
  double xx[6], xaz[6], xaz2[6];
  double dd, dt, refr;
  int32 iflag = epheflag & (SEFLG_JPLEPH|SEFLG_SWIEPH|SEFLG_MOSEPH); 
  int32 iflagtopo = iflag | SEFLG_EQUATORIAL;
  double sda, armc, md, dmd, mdrise, rdi, tr, dalt;
  double decl;
  double tjd_ut0 = tjd_ut;
  int32 facrise = 1;
  int32 tohor_flag = SE_EQU2HOR;
  AS_BOOL is_second_run = FALSE;
  int nloop = 2;
  *tret = 0;
  if (ipl == SE_MOON)
    nloop = 4; 
  if (rsmi & SE_CALC_SET)
    facrise = -1;
  if (!(rsmi & SE_BIT_GEOCTR_NO_ECL_LAT)) {
    iflagtopo |= SEFLG_TOPOCTR;
    set_topo_for_this_call(ctx, dgeo[0], dgeo[1], dgeo[2]);
  }
run_rise_again:
  if (swe_calc_ut_r(ctx, tjd_ut, ipl, iflagtopo, xx, serr) == ERR) 
    return ERR;
  /* the diurnal arc is a bit fuzzy, 
   * - because the object changes declination during the day
   * - because there is refraction of light
   * nevertheless this works well as soon as the object is not
   * circumpolar or near-circumpolar
   */
  decl = xx[1];
  // semi-diurnal arcs
  sda = -tan(dgeo[1] * DEGTORAD) * tan(decl * DEGTORAD);
  if (sda >= 1) {
    sda = 10;  // actually sda = 0°, but we give it a value of 10° 
               // to account for refraction. value 0 would cause
               // problems
  } else if (sda <= -1) {
    sda = 180;
  } else {
    sda = acos(sda) * RADTODEG;
  }
  // sidereal time at tjd_start
  armc = swe_degnorm(swe_sidtime_r(ctx, tjd_ut) * 15 + dgeo[0]); 
  // meridian distance of object
  md = swe_degnorm(xx[0] - armc);
  mdrise = swe_degnorm(sda * facrise);
  //dmd = swe_degnorm(md - mdrise - 1);
  dmd = swe_degnorm(md - mdrise);
  // Avoid the risk of getting the event of next day:
#if 0
  if (dmd > 358) {
    tjd_ut -= 0.1;
    goto run_rise_again;
  }
#else
  if (dmd > 358) {
    dmd -= 360;
  }
#endif
  // rough subsequent rising/setting time
  tr = tjd_ut + dmd / 360;
  /* if object is sun or moon and rising of upper limb is required,
   * calculate apparent radius of disk (ignoring refraction);
   * with other objects disk diameter is ignored. */
  rdi = 0;
  /* true altitude of sun, when it appears at the horizon; 
   * refraction for a body visible at the horizon at 0m above sea,
   */
  if (atpress == 0) {
    /* estimate atmospheric pressure */
    atpress = 1013.25 * pow(1 - 0.0065 * dgeo[2] / 288, 5.255);
  } 
  swe_refrac_extended(0.000001, 0, atpress, attemp, ctx->const_lapse_rate, SE_APP_TO_TRUE, xx);
  refr = xx[1] - xx[0];
//fprintf(stderr, "refr=%f, %f, %f\n", refr, xx[0], xx[1]);
  if (rsmi & SE_BIT_GEOCTR_NO_ECL_LAT) {
    tohor_flag = SE_ECL2HOR;
    iflagtopo = iflag;
  } else {
    tohor_flag = SE_EQU2HOR; // this is more efficient
    iflagtopo = iflag | SEFLG_EQUATORIAL;
    iflagtopo |= SEFLG_TOPOCTR;
    set_topo_for_this_call(ctx, dgeo[0], dgeo[1], dgeo[2]);
  }
  for (i = 0; i < nloop; i++) {
    if (swe_calc_ut_r(ctx, tr, ipl, iflagtopo, xx, serr) == ERR)
      return ERR;
    if (rsmi & SE_BIT_GEOCTR_NO_ECL_LAT)
      xx[1] = 0;
    rdi = get_sun_rad_plus_refr(ipl, xx[2], rsmi, refr); 
    swe_azalt_r(ctx, tr, tohor_flag, dgeo, atpress, attemp, xx, xaz);
    swe_azalt_r(ctx, tr + 0.001, tohor_flag, dgeo, atpress, attemp, xx, xaz2);
    dd = (xaz2[1] - xaz[1]);
    dalt = xaz[1] + rdi;
    dt = dalt / dd / 1000.0;
    if (dt > 0.1) {
      dt = 0.1;
    } else if (dt < -0.1) {
      dt = -0.1;
    }
    if ((0) && fabs(dt) > 5.0 / 86400.0 && nloop < 20)
      nloop++;
    tr -= dt;
  }
//fprintf(stderr, "tr-tjd=%f tin=%f tout=%f\n", tr - tjd_ut0, tjd_ut0, tr);
  // if the event found is before input time, we search next event.
  if (tr < tjd_ut0 && !is_second_run) {
    tjd_ut += 0.5;
    is_second_run = TRUE;
    goto run_rise_again;
  }
  *tret = tr;
  return OK;
}

/* rise, set, and meridian transits of sun, moon, planets, and stars
 *
 * tjd_ut	universal time from when on search ought to start
 * ipl		planet number, neglected, if starname is given
 * starname	pointer to string. if a planet, not a star, is
 *              wanted, starname must be NULL or ""
 * epheflag	used for ephemeris only
 * rsmi		SE_CALC_RISE, SE_CALC_SET, SE_CALC_MTRANSIT, SE_CALC_ITRANSIT
 *              | SE_BIT_DISC_CENTER       for rises of disc center of body
 *              | SE_BIT_DISC_BOTTOM       for rises of disc bottom of body
 *              | SE_BIT_GEOCTR_NO_ECL_LAT use geocentric position of object
 *                                         and ignore ecliptic latitude
 *              | SE_BIT_NO_REFRACTION     to neglect refraction
 *              | SE_BIT_CIVIL_TWILIGHT    calculate civil twilight
 *              | SE_BIT_NAUTIC_TWILIGHT   calculate nautical twilight
 *              | SE_BIT_ASTRO_TWILIGHT    calculate astronomical twilight 
 *              | SE_BIT_FIXED_DISC_SIZE   neglect the effect of distance on disc size
 *              | SE_BIT_HINDU_RISING      risings according to Hindu astrology 
 *                                         = (SE_BIT_DISC_CENTER|SE_BIT_NO_REFRACTION|SE_BIT_GEOCTR_NO_ECL_LAT)
 * geopos	array of doubles for geogr. long., lat. and height above sea
 * atpress	atmospheric pressure
 * attemp	atmospheric temperature
 *
 * return variables:
 * tret         time of rise, set, meridian transits
 * serr[256]	error string
 * function return value -2 means that the body does not rise or set */
#define SEFLG_EPHMASK	(SEFLG_JPLEPH|SEFLG_SWIEPH|SEFLG_MOSEPH)
static int32 swe_rise_trans_r_body(swe_ctx *ctx, double tjd_ut, int32 ipl, char *starname,
	       int32 epheflag, int32 rsmi,
               double *geopos, 
	       double atpress, double attemp,
               double *tret,
               char *serr)
{
  int32 retval = 0;
  /* Simple fast algorithm for risings and settings of 
   * - planets Sun, Moon, Mercury - Pluto + Lunar Nodes
   * Does not work well for geographic latitudes
   * > 65 N/S for the Sun
   * > 60 N/S for the Moon and the planets
   * Beyond these limits, some risings or settings may be missed.
   */
  AS_BOOL do_fixstar = (starname != NULL && *starname != '\0');
  if (!do_fixstar
    && (rsmi & (SE_CALC_RISE|SE_CALC_SET)) 
    && !(rsmi & SE_BIT_FORCE_SLOW_METHOD)
    && !(rsmi & (SE_BIT_CIVIL_TWILIGHT|SE_BIT_NAUTIC_TWILIGHT|SE_BIT_ASTRO_TWILIGHT))
    && (ipl >= SE_SUN && ipl <= SE_TRUE_NODE)
    && (fabs(geopos[1]) <= 60 || (ipl == SE_SUN && fabs(geopos[1]) <= 65))
    ) {
      retval = rise_set_fast(ctx, tjd_ut, ipl, epheflag, rsmi, geopos, atpress, attemp, tret, serr);
      return retval;
  }
  return swe_rise_trans_true_hor_r(ctx, tjd_ut, ipl, starname, epheflag, rsmi, geopos, atpress, attemp, 0, tret, serr);
}

int32 CALL_CONV swe_rise_trans_r(swe_ctx *ctx, double tjd_ut, int32 ipl, char *starname,
	       int32 epheflag, int32 rsmi,
               double *geopos, 
	       double atpress, double attemp,
               double *tret,
               char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_rise_trans_r_body(ctx, tjd_ut, ipl, starname, epheflag, rsmi, geopos, atpress, attemp, tret, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_rise_trans(
               double tjd_ut, int32 ipl, char *starname,
	       int32 epheflag, int32 rsmi,
               double *geopos, 
	       double atpress, double attemp,
               double *tret,
               char *serr)
{
  return swe_rise_trans_r(swi_default_ctx(), tjd_ut, ipl, starname, epheflag, rsmi, geopos, atpress, attemp, tret, serr);
}

/* same as swe_rise_trans(), but allows to define the height of the horizon
 * at the point of the rising or setting (horhgt) */
static int32 swe_rise_trans_true_hor_r_body(swe_ctx *ctx, double tjd_ut, int32 ipl, char *starname,
	       int32 epheflag, int32 rsmi,
               double *geopos, 
	       double atpress, double attemp,
	       double horhgt,
               double *tret,
               char *serr)
{
  int i, j, k, ii, calc_culm, nculm = -1;
  double tjd_et = tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, epheflag, serr);
  double xc[6], xh[20][6], ah[6], aha;
  double tculm[4], tcu, tc[20], h[20], t2[6], dc[6], dtint, dx, rdi, dd = 0;
  int32 iflag = epheflag;
  int jmax = 14;
  double t, te, tt, dt, twohrs = 1.0 / 12.0;
  double curdist;
  int32 tohor_flag = SE_EQU2HOR;
  int nazalt = 0;
  int ncalc = 0;
  AS_BOOL do_fixstar = (starname != NULL && *starname != '\0');
  if (geopos[2] < SEI_ECL_GEOALT_MIN || geopos[2] > SEI_ECL_GEOALT_MAX) {
    if (serr != NULL)
      sprintf(serr, "location for swe_rise_trans() must be between %.0f and %.0f m above sea", SEI_ECL_GEOALT_MIN, SEI_ECL_GEOALT_MAX);
    return ERR;
  }
  // if horhgt == -100, set horhgt = dip of horizon, i.e. refracted height
  // of ocean if visible at horizon.
  if (horhgt == -100) {
    horhgt = 0.0001 + calc_dip(geopos[2], atpress, attemp, ctx->const_lapse_rate);
  }
  /*swi_set_tid_acc(ctx, tjd_ut, epheflag, 0, serr);*/
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO */
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  xh[0][0] = 0; /* to shut up mint */
  /* allowing SEFLG_NONUT and SEFLG_TRUEPOS speeds it up */
  iflag &= (SEFLG_EPHMASK | SEFLG_NONUT | SEFLG_TRUEPOS);
  *tret = 0;
  if (rsmi & SE_BIT_GEOCTR_NO_ECL_LAT) {
    tohor_flag = SE_ECL2HOR;
  } else {
    tohor_flag = SE_EQU2HOR;
    iflag |= SEFLG_EQUATORIAL;
    iflag |= SEFLG_TOPOCTR;
    set_topo_for_this_call(ctx, geopos[0], geopos[1], geopos[2]);
  }
  if (rsmi & (SE_CALC_MTRANSIT | SE_CALC_ITRANSIT))
    return calc_mer_trans(ctx, tjd_ut, ipl, epheflag, rsmi, 
		geopos, starname, tret, serr);
  if (!(rsmi & (SE_CALC_RISE | SE_CALC_SET)))
    rsmi |= SE_CALC_RISE;
  /* twilight calculation */
  if (ipl == SE_SUN && (rsmi & (SE_BIT_CIVIL_TWILIGHT|SE_BIT_NAUTIC_TWILIGHT|SE_BIT_ASTRO_TWILIGHT))) {
    rsmi |= (SE_BIT_NO_REFRACTION | SE_BIT_DISC_CENTER);
    horhgt = -rdi_twilight(rsmi); 
      /* note: twilight is not dependent on height of horizon, so we can
       * use this parameter and define a fictitious height of horizon */
  }
  /* find culmination points within 28 hours from t0 - twohrs.
   * culminations are required in case there are maxima or minima
   * in height slightly above or below the horizon.
   * we do not use meridian transits, because in polar regions
   * the culmination points may considerably deviate from
   * transits. also, there are cases where the moon rises in the
   * western half of the sky for a short time. 
   */
  if (do_fixstar) {
    if (swe_fixstar_r(ctx, starname, tjd_et, iflag, xc, serr) == ERR)
      return ERR;
  } 
  for (ii = 0, t = tjd_ut - twohrs; ii <= jmax; ii++, t += twohrs) {
    tc[ii] = t;
    if (!do_fixstar) {
      te = t + swe_deltat_ex_r(ctx, t, epheflag, serr);
      if (swe_calc_r(ctx, te, ipl, iflag, xc, serr) == ERR)
        return ERR;
ncalc++;
    }
    if (rsmi & SE_BIT_GEOCTR_NO_ECL_LAT)
      xc[1] = 0;
    /* diameter of object in km */
    if (ii == 0) {
      if (do_fixstar) 
        dd = 0;
      else if (rsmi & SE_BIT_DISC_CENTER)
        dd = 0;
      else if (ipl < NDIAM)
        dd = pla_diam[ipl];
      else if (ipl > SE_AST_OFFSET)
        dd = ctx->ast_diam * 1000;	/* km -> m */
      else
        dd = 0;
    }
    curdist = xc[2];
    if (rsmi & SE_BIT_FIXED_DISC_SIZE) {
      if (ipl == SE_SUN) {
        curdist = 1.0;
      } else if (ipl == SE_MOON) {
        curdist = 0.00257;
      }
    }
    /* apparent radius of disc */
    rdi = asin( dd / 2 / AUNIT / curdist ) * RADTODEG;
    /* true height of center of body */
    swe_azalt_r(ctx, t, tohor_flag, geopos, atpress, attemp, xc, xh[ii]);
nazalt++;
    if (rsmi & SE_BIT_DISC_BOTTOM) {
      /* true height of bottom point of body */
      xh[ii][1] -= rdi;
    } else {
      /* true height of uppermost point of body */
      xh[ii][1] += rdi;
    }
    /* apparent height of uppermost point of body */
    if (rsmi & SE_BIT_NO_REFRACTION) {
      xh[ii][1] -= horhgt;
      h[ii] = xh[ii][1];
    } else {
      swe_azalt_rev_r(ctx, t, SE_HOR2EQU, geopos, xh[ii], xc);
nazalt++;
      swe_azalt_r(ctx, t, SE_EQU2HOR, geopos, atpress, attemp, xc, xh[ii]);
nazalt++;
      xh[ii][1] -= horhgt;
      xh[ii][2] -= horhgt;
      h[ii] = xh[ii][2];
    }
    calc_culm = 0;
    if (ii > 1) {
      dc[0] = xh[ii-2][1];
      dc[1] = xh[ii-1][1];
      dc[2] = xh[ii][1];
      if (dc[1] > dc[0] && dc[1] > dc[2])
        calc_culm = 1;
      if (dc[1] < dc[0] && dc[1] < dc[2])
        calc_culm = 2;
    }
    if (calc_culm) {
      dt = twohrs;
      tcu = t - dt;
      find_maximum(dc[0], dc[1], dc[2], dt, &dtint, &dx);
      tcu += dtint + dt;
      dt /= 3;
      for (; dt > 0.0001; dt /= 3) {
        for (i = 0, tt = tcu - dt; i < 3; tt += dt, i++) {
          te = tt + swe_deltat_ex_r(ctx, tt, epheflag, serr);
          if (!do_fixstar) {
            if (swe_calc_r(ctx, te, ipl, iflag, xc, serr) == ERR)
              return ERR;
	  }
	  if (rsmi & SE_BIT_GEOCTR_NO_ECL_LAT)
	    xc[1] = 0;
	  ncalc++;
          swe_azalt_r(ctx, tt, tohor_flag, geopos, atpress, attemp, xc, ah);
	  nazalt++;
	  ah[1] -= horhgt;
          dc[i] = ah[1];
        }
        find_maximum(dc[0], dc[1], dc[2], dt, &dtint, &dx);
        tcu += dtint + dt;
      }
      nculm++;
      tculm[nculm] = tcu;
    }
  }
  /* note: there can be a rise or set on the poles, even if 
   * there is no culmination. So, we must not leave here
   * in any case. */
  /* insert culminations into array of heights */
  for (i = 0; i <= nculm; i++) {
    for (j = 1; j <= jmax; j++) {
      if (tculm[i] < tc[j]) {
        for (k = jmax; k >= j; k--) {
          tc[k+1] = tc[k];
          h[k+1] = h[k];
        }
        tc[j] = tculm[i];
        if (!do_fixstar) {
          te = tc[j] + swe_deltat_ex_r(ctx, tc[j], epheflag, serr);
          if (swe_calc_r(ctx, te, ipl, iflag, xc, serr) == ERR)
            return ERR;
	  if (rsmi & SE_BIT_GEOCTR_NO_ECL_LAT)
	    xc[1] = 0;
ncalc++;
        }
        curdist = xc[2];
        if (rsmi & SE_BIT_FIXED_DISC_SIZE) {
          if ( ipl == SE_SUN ) {
            curdist = 1.0;
          } else if (ipl == SE_MOON) {
            curdist = 0.00257;
          }
        }
        /* apparent radius of disc */
        rdi = asin( dd / 2 / AUNIT / curdist ) * RADTODEG;
        /* true height of center of body */
        swe_azalt_r(ctx, tc[j], tohor_flag, geopos, atpress, attemp, xc, ah);
nazalt++;
        if (rsmi & SE_BIT_DISC_BOTTOM) {
          /* true height of bottom point of body */
          ah[1] -= rdi;
        } else {
	  /* true height of uppermost point of body */
	  ah[1] += rdi;
        }
        /* apparent height of uppermost point of body */
	if (rsmi & SE_BIT_NO_REFRACTION) {
	  ah[1] -= horhgt;
	  h[j] = ah[1];
	} else {
	  swe_azalt_rev_r(ctx, tc[j], SE_HOR2EQU, geopos, ah, xc);
nazalt++;
	  swe_azalt_r(ctx, tc[j], SE_EQU2HOR, geopos, atpress, attemp, xc, ah);
nazalt++;
	  ah[1] -= horhgt;
	  ah[2] -= horhgt;
	  h[j] = ah[2];
	}
        jmax++;
        break;
      }
    }
  }
  *tret = 0;
  /* find points with zero height. 
   * binary search */
  for (ii = 1; ii <= jmax; ii++) {
    if (h[ii-1] * h[ii] >= 0)
      continue;
    if (h[ii-1] < h[ii] && !(rsmi & SE_CALC_RISE))
      continue;
    if (h[ii-1] > h[ii] && !(rsmi & SE_CALC_SET))
      continue;
    dc[0] = h[ii-1];
    dc[1] = h[ii];
    t2[0] = tc[ii-1];
    t2[1] = tc[ii];
    for (i = 0; i < 20; i++) {
      t = (t2[0] + t2[1]) / 2;
      if (!do_fixstar) {
        te = t + swe_deltat_ex_r(ctx, t, epheflag, serr);
        if (swe_calc_r(ctx, te, ipl, iflag, xc, serr) == ERR)
          return ERR;
	if (rsmi & SE_BIT_GEOCTR_NO_ECL_LAT)
	  xc[1] = 0;
ncalc++;
      }
      curdist = xc[2];
      if (rsmi & SE_BIT_FIXED_DISC_SIZE) {
        if (ipl == SE_SUN) {
          curdist = 1.0;
        } else if (ipl == SE_MOON) {
          curdist = 0.00257;
        }
      }
      /* apparent radius of disc */
      rdi = asin( dd / 2 / AUNIT / curdist ) * RADTODEG;
      /* true height of center of body */
      swe_azalt_r(ctx, t, tohor_flag, geopos, atpress, attemp, xc, ah);
nazalt++;
      if (rsmi & SE_BIT_DISC_BOTTOM) {
        /* true height of bottom point of body */
        ah[1] -= rdi;
      } else {
	/* true height of uppermost point of body */
	ah[1] += rdi;
      }
      /* apparent height of uppermost point of body */
      if (rsmi & SE_BIT_NO_REFRACTION) {
	ah[1] -= horhgt;
	aha = ah[1];
      } else {
	swe_azalt_rev_r(ctx, t, SE_HOR2EQU, geopos, ah, xc);
	nazalt++;
	swe_azalt_r(ctx, t, SE_EQU2HOR, geopos, atpress, attemp, xc, ah);
	nazalt++;
	ah[1] -= horhgt;
	ah[2] -= horhgt;
	aha = ah[2];
      }
      if (aha * dc[0] <= 0) {
        dc[1] = aha;
        t2[1] = t;
      } else {
        dc[0] = aha;
        t2[0] = t;
      }
    }
    if (t > tjd_ut) {
     *tret = t;
    if (0)  fprintf(stderr, "nazalt=%d\n", nazalt);
    if (0)  fprintf(stderr, "ncalc=%d\n", ncalc);
     return OK;
    }
  }
  if (serr)
    sprintf(serr, "rise or set not found for planet %d", ipl);
  return -2; /* no t of rise or set found */
}

int32 CALL_CONV swe_rise_trans_true_hor_r(swe_ctx *ctx, double tjd_ut, int32 ipl, char *starname,
	       int32 epheflag, int32 rsmi,
               double *geopos, 
	       double atpress, double attemp,
	       double horhgt,
               double *tret,
               char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_rise_trans_true_hor_r_body(ctx, tjd_ut, ipl, starname, epheflag, rsmi, geopos, atpress, attemp, horhgt, tret, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_rise_trans_true_hor(
               double tjd_ut, int32 ipl, char *starname,
	       int32 epheflag, int32 rsmi,
               double *geopos, 
	       double atpress, double attemp,
	       double horhgt,
               double *tret,
               char *serr)
{
  return swe_rise_trans_true_hor_r(swi_default_ctx(), tjd_ut, ipl, starname, epheflag, rsmi, geopos, atpress, attemp, horhgt, tret, serr);
}

static int32 calc_mer_trans(swe_ctx *ctx, 
               double tjd_ut, int32 ipl, int32 epheflag, int32 rsmi,
               double *geopos,
               char *starname, 
               double *tret,
               char *serr)
{
  int i;
  double tjd_et = tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, epheflag, serr);
  double armc, armc0, arxc, x0[6], x[6], t, te;
  double mdd;
  int32 iflag = epheflag;
  AS_BOOL do_fixstar = (starname != NULL && *starname != '\0');
  iflag &= SEFLG_EPHMASK;
  *tret = 0;
  iflag |= (SEFLG_EQUATORIAL | SEFLG_TOPOCTR);
  armc0 = swe_sidtime_r(ctx, tjd_ut) + geopos[0] / 15;
  if (armc0 >= 24)
    armc0 -= 24;
  if (armc0 < 0)
    armc0 += 24;
  armc0 *= 15;
  if (do_fixstar) {
    if (swe_fixstar_r(ctx, starname, tjd_et, iflag, x0, serr) == ERR)
      return ERR;
  } else {
    if (swe_calc_r(ctx, tjd_et, ipl, iflag, x0, serr) == ERR)
      return ERR;
  }
  /*
   * meridian transits
   */
  x[0] = x0[0];
  x[1] = x0[1];
  t = tjd_ut;
  arxc = armc0;
  if (rsmi & SE_CALC_ITRANSIT)
    arxc = swe_degnorm(arxc + 180);
  for (i = 0; i < 4; i++) {
    mdd = swe_degnorm(x[0] - arxc);
    if (i > 0 && mdd > 180)
      mdd -= 360;
    t += mdd / 361;
    armc = swe_sidtime_r(ctx, t) + geopos[0] / 15;
    if (armc >= 24)
      armc -= 24;
    if (armc < 0)
      armc += 24;
    armc *= 15;
      arxc = armc;
    if (rsmi & SE_CALC_ITRANSIT)
      arxc = swe_degnorm(arxc + 180);
    if (!do_fixstar) {
      te = t + swe_deltat_ex_r(ctx, t, epheflag, serr);
      if (swe_calc_r(ctx, te, ipl, iflag, x, serr) == ERR)
	return ERR;
    }
  }
  *tret = t;
  return OK;
}

/*
Nodes and apsides of planets and moon

Planetary nodes can be defined in three different ways:
a) They can be understood as a direction or as an axis 
  defined by the intersection line of two orbital planes. 
  E.g., the nodes of Mars are defined by the intersection 
  line of Mars' orbital plane with the ecliptic (= the 
  Earths orbit heliocentrically or the solar orbit 
  geocentrically). However, as Michael Erlewine points 
  out in his elaborate web page on this topic 
  (http://thenewage.com/resources/articles/interface.html),
  planetary nodes can be defined for any couple of 
  planets. E.g. there is also an intersection line for the 
  two orbital planes of Mars and Saturn.
  Because such lines are, in principle, infinite, the 
  heliocentric and the geocentric positions of the 
  planetary nodes will be the same. There are astrologers 
  that use such heliocentric planetary nodes in geocentric 
  charts.
  The ascending and the descending node will, in this 
  case, be in precise opposition.

b) The planetary nodes can also be understood in a 
  different way, not as an axis, but as the two points on a 
  planetary orbit that are located precisely on the 
  intersection line of the two planes.
  This second definition makes no difference for the moon or 
  for heliocentric positions of planets, but it does so for 
  geocentric positions. There are two possibilities for 
  geocentric planetary nodes based on this definition. 
  1) The common solution is that the points on the 
    planets orbit are transformed to the geocenter. The 
    two points will not be in opposition anymore, or 
    they will only roughly be so with the outer planets. The 
    advantage of these nodes is that when a planet is in 
    conjunction with its node, then its ecliptic latitude 
    will be zero. This is not true when a planet is in 
    geocentric conjunction with its heliocentric node. 
    (And neither is it always true for the inner planets, 
    i.e. Mercury and Venus.)
  2) The second possibility that nobody seems to have 
    thought of so far: One may compute the points of 
    the earth's orbit that lie exactly on another planet's 
    orbital plane and transform it to the geocenter. The two 
    points will always be in an approximate square.

c) Third, the planetary nodes could be defined as the 
  intersection points of the plane defined by their 
  momentary geocentric position and motion with the 
  plane of the ecliptic. Such points would move very fast 
  around the planetary stations. Here again, as in b)1), 
  the planet would cross the ecliptic and its ecliptic 
  latitude would be 0 exactly when it were in 
  conjunction with one of its nodes.

The Swiss Ephemeris supports the solutions a) and b) 1).

Possible definitions for apsides

a) The planetary apsides can be defined as the perihelion and
  aphelion points on a planetary orbit. For a
  geocentric chart, these points could be transformed 
  from the heliocenter to the geocenter.
b) However, one might consider these points as 
  astrologically relevant axes rather than as points on a 
  planetary orbit. Again, this would allow heliocentric 
  positions in a geocentric chart.

Note: For the "Dark Moon" or "Lilith", which I usually 
define as the lunar apogee, some astrologers give a 
different definition. They understand it as the second focal 
point of the moon's orbital ellipse. This definition does not 
make a difference for geocentric positions, because the 
apogee and the second focus are in exactly the same geocentric 
direction. However, it makes a difference with topocentric 
positions, because the two points do not have same distance. 
Analogous "black planets" have been proposed: they would be the 
second focal points of the planets' orbital ellipses. The 
heliocentric positions of these "black planets" are identical 
with the heliocentric positions of the aphelia, but geocentric 
positions are not identical, because the focal points are 
much closer to the sun than the aphelia.

The Swiss Ephemeris allows to compute the "black planets" as well.

Mean positions

Mean nodes and apsides can be computed for the Moon, the 
Earth and the planets Mercury - Neptune. They are taken 
from the planetary theory VSOP87. Mean points can not be 
calculated for Pluto and the asteroids, because there is no 
planetary theory for them.
 
Osculating nodes and apsides

Nodes and apsides can also be derived from the osculating 
orbital elements of a body, the paramaters that define an  
ideal unperturbed elliptic (two-body) orbit. 
For astrology, note that this is a simplification and 
idealization. 
Problem with Neptune: Neptune's orbit around the sun does not 
have much in common with an ellipse. There are often two 
perihelia and two aphelia within one revolution. As a result, 
there is a wild oscillation of the osculating perihelion (and 
aphelion). 
In actuality, Neptune's orbit is not heliocentric orbit at all. 
The twofold perihelia and aphelia are an effect of the motion of 
the sun about the solar system barycenter. This motion is 
much faster than the motion of Neptune, and Neptune 
cannot react on such fast displacements of the Sun. As a 
result, Neptune seems to move around the barycenter (or a 
mean sun) rather than around the true sun. In fact, 
Neptune's orbit around the barycenter is therefore closer to 
an ellipse than the his orbit around the sun. The same 
statement is also true for Saturn, Uranus and Pluto, but not 
for Jupiter and the inner planets.

This fundamental problem about osculating ellipses of 
planetary orbits does of course not only affect the apsides 
but also the nodes.

Two solutions can be thought of for this problem: 
1) The one would be to interpolate between actual 
  passages of the planets through their nodes and 
  apsides. However, this works only well with Mercury. 
  With all other planets, the supporting points are too far 
  apart as to make an accurate interpolation possible. 
  This solution is not implemented, here.
2) The other solution is to compute the apsides of the 
  orbit around the barycenter rather than around the sun. 
  This procedure makes sense for planets beyond Jupiter, 
  it comes closer to the mean apsides and nodes for 
  planets that have such points defined. For all other 
  transsaturnian planets and asteroids, this solution yields 
  a kind of "mean" nodes and apsides. On the other hand, 
  the barycentric ellipse does not make any sense for 
  inner planets and Jupiter.

The Swiss Ephemeris supports solution 2) for planets and 
asteroids beyond Jupiter.

Anyway, neither the heliocentric nor the barycentric ellipse 
is a perfect representation of the nature of a planetary orbit, 
and it will not yield the degree of precision that today's 
astrology is used to.
The best choice of method will probably be:
- For Mercury - Neptune: mean nodes and apsides
- For asteroids that belong to the inner asteroid belt: 
osculating nodes/apsides from a heliocentric ellipse
- For Pluto and outer asteroids: osculating nodes/apsides 
from a barycentric ellipse

The Moon is a special case: A "lunar true node" makes 
more sense, because it can be defined without the idea of an 
ellipse, e.g. as the intersection axis of the momentary lunar 
orbital plane with the ecliptic. Or it can be said that the 
momentary motion of the moon points to one of the two 
ecliptic points that are called the "true nodes".  So, these 
points make sense. With planetary nodes, the situation is 
somewhat different, at least if we make a difference 
between heliocentric and geocentric positions. If so, the 
planetary nodes are points on a heliocentric orbital ellipse, 
which are transformed to the geocenter. An ellipse is 
required here, because a solar distance is required. In 
contrast to the planetary nodes, the lunar node does not 
require a distance, therefore manages without the idea of an 
ellipse and does not share its weaknesses. 
On the other hand, the lunar apsides DO require the idea of 
an ellipse. And because the lunar ellipse is actually 
extremely distorted, even more than any other celestial 
ellipse, the "true Lilith" (apogee), for which printed 
ephemerides are available, does not make any sense at all. 
(See the chapter on the lunar node and apogee.)

Special case: the Earth

The Earth is another special case. Instead of the motion of 
the Earth herself, the heliocentric motion of the Earth-
Moon-Barycenter (EMB) is used to determine the 
osculating perihelion. 
There is no node of the earth orbit itself. However, there is 
an axis around which the earth's orbital plane slowly rotates 
due to planetary precession. The position points of this axis 
are not calculated by the Swiss Ephemeris.

Special case: the Sun

In addition to the Earth (EMB) apsides, the function 
computes so-to-say "apsides" of the sun, i.e. points on the 
orbit of the Sun where it is closest to and where it is farthest 
from the Earth. These points form an opposition and are 
used by some astrologers, e.g. by the Dutch astrologer 
George Bode or the Swiss astrologer Liduina Schmed. The 
perigee, located at about 13 Capricorn, is called the 
"Black Sun", the other one, in Cancer, the "Diamond".
So, for a complete set of apsides, one ought to calculate 
them for the Sun and the Earth and all other planets. 

The modes of the Swiss Ephemeris function 
swe_nod_aps()

The  function swe_nod_aps() can be run in the following 
modes:
1) Mean positions are given for nodes and apsides of Sun, 
  Moon, Earth, and the up to Neptune. Osculating 
  positions are given with Pluto and all asteroids. This is 
  the default mode.
2) Osculating positions are returned for nodes and apsides 
  of all planets.
3) Same as 2), but for planets and asteroids beyond 
  Jupiter, a barycentric ellipse is used.
4) Same as 1), but for Pluto and asteroids beyond Jupiter, 
  a barycentric ellipse is used.

In all of these modes, the second focal point of the ellipse 
can be computed instead of the aphelion.
Like the planetary function swe_calc(), swe_nod_aps() is 
able to return geocentric, topocentric, heliocentric, or 
barycentric position.
 *
 * tjd_ut	 julian day, ephemeris time
 * ipl		 planet number
 * iflag	 as usual, SEFLG_HELCTR, etc.
 * xnasc         an array of 6 doubles: ascending node
 * xndsc         an array of 6 doubles: ascending node
 * xperi         an array of 6 doubles: perihelion
 * xaphe         an array of 6 doubles: aphelion
 * method        see below
 * serr          error message
 *
 * method        can have the following values:
 *               - 0 or SE_NODBIT_MEAN. MEAN positions are given for
 *                 nodes and apsides of Sun, Moon, Earth, and the 
 *                 planets up to Neptune. Osculating positions are
 *                 given with Pluto and all asteroids.
 *               - SE_NODBIT_OSCU. Osculating positions are given
 *                 for all nodes and apsides.
 *               - SE_NODBIT_OSCU_BAR. Osculating nodes and apsides
 *                 are computed from barycentric ellipses, for planets
 *                 beyond Jupiter, but from heliocentric ones for
 *                 ones for Jupiter and inner planets.
 *               - SE_NODBIT_MEAN and SE_NODBIT_OSCU_BAR can be combined.
 *                 The program behaves the same way as with simple 
 *                 SE_NODBIT_MEAN, but uses barycentric ellipses for
 *                 planets beyond Neptune and asteroids beyond Jupiter.
 *               - SE_NODBIT_FOCAL can be combined with any of the other
 *                 bits. The second focal points of the ellipses will 
 *                 be returned instead of the aphelia.
 */
/* mean elements for Mercury - Neptune from VSOP87 (mean equinox of date) */
static const double el_node[8][4] = 
  {{ 48.330893,  1.1861890,  0.00017587,  0.000000211,}, /* Mercury */
  { 76.679920,  0.9011190,  0.00040665, -0.000000080,}, /* Venus   */
  {  0       ,  0        ,  0         ,  0          ,}, /* Earth   */
  { 49.558093,  0.7720923,  0.00001605,  0.000002325,}, /* Mars    */
  {100.464441,  1.0209550,  0.00040117,  0.000000569,}, /* Jupiter */
  {113.665524,  0.8770970, -0.00012067, -0.000002380,}, /* Saturn  */
  { 74.005947,  0.5211258,  0.00133982,  0.000018516,}, /* Uranus  */
  {131.784057,  1.1022057,  0.00026006, -0.000000636,}, /* Neptune */
  };
static const double el_peri[8][4] = 
  {{ 77.456119,  1.5564775,  0.00029589,  0.000000056,}, /* Mercury */
  {131.563707,  1.4022188, -0.00107337, -0.000005315,}, /* Venus   */
  {102.937348,  1.7195269,  0.00045962,  0.000000499,}, /* Earth   */
  {336.060234,  1.8410331,  0.00013515,  0.000000318,}, /* Mars    */
  { 14.331309,  1.6126668,  0.00103127, -0.000004569,}, /* Jupiter */
  { 93.056787,  1.9637694,  0.00083757,  0.000004899,}, /* Saturn  */
  {173.005159,  1.4863784,  0.00021450,  0.000000433,}, /* Uranus  */
  { 48.123691,  1.4262677,  0.00037918, -0.000000003,}, /* Neptune */
  };
static const double el_incl[8][4] = 
  {{  7.004986,  0.0018215, -0.00001809,  0.000000053,}, /* Mercury */
  {  3.394662,  0.0010037, -0.00000088, -0.000000007,}, /* Venus   */
  {  0,         0,          0,           0          ,}, /* Earth   */
  {  1.849726, -0.0006010,  0.00001276, -0.000000006,}, /* Mars    */
  {  1.303270, -0.0054966,  0.00000465, -0.000000004,}, /* Jupiter */
  {  2.488878, -0.0037363, -0.00001516,  0.000000089,}, /* Saturn  */
  {  0.773196,  0.0007744,  0.00003749, -0.000000092,}, /* Uranus  */
  {  1.769952, -0.0093082, -0.00000708,  0.000000028,}, /* Neptune */
  };
static const double el_ecce[8][4] = 
  {{  0.20563175,  0.000020406, -0.0000000284, -0.00000000017,}, /* Mercury */
  {  0.00677188, -0.000047766,  0.0000000975,  0.00000000044,}, /* Venus   */
  {  0.01670862, -0.000042037, -0.0000001236,  0.00000000004,}, /* Earth   */
  {  0.09340062,  0.000090483, -0.0000000806, -0.00000000035,}, /* Mars    */
  {  0.04849485,  0.000163244, -0.0000004719, -0.00000000197,}, /* Jupiter */
  {  0.05550862, -0.000346818, -0.0000006456,  0.00000000338,}, /* Saturn  */
  {  0.04629590, -0.000027337,  0.0000000790,  0.00000000025,}, /* Uranus  */
  {  0.00898809,  0.000006408, -0.0000000008, -0.00000000005,}, /* Neptune */
  };
static const double el_sema[8][4] = 
  {{  0.387098310,  0.0,  0.0,  0.0,}, /* Mercury */
  {  0.723329820,  0.0,  0.0,  0.0,}, /* Venus   */
  {  1.000001018,  0.0,  0.0,  0.0,}, /* Earth   */
  {  1.523679342,  0.0,  0.0,  0.0,}, /* Mars    */
  {  5.202603191,  0.0000001913,  0.0,  0.0,}, /* Jupiter */
  {  9.554909596,  0.0000021389,  0.0,  0.0,}, /* Saturn  */
  { 19.218446062, -0.0000000372,  0.00000000098,  0.0,}, /* Uranus  */
  { 30.110386869, -0.0000001663,  0.00000000069,  0.0,}, /* Neptune */
  };
#ifdef SWE_UPSTREAM_COMPAT
/* Ratios of mass of Sun to masses of the planets (bugs 105, 108: the
 * default build takes the ephemeris's, swi_gm_sun_planet()) */
static const double plmass[9] = {
    6023600,        /* Mercury */
     408523.719,    /* Venus */
     328900.5,      /* Earth and Moon */
    3098703.59,     /* Mars */
       1047.348644, /* Jupiter */
       3497.9018,   /* Saturn */
      22902.98,     /* Uranus */
      19412.26,     /* Neptune */
  136566000,        /* Pluto */
};
#endif
static const int ipl_to_elem[15] = {2, 0, 0, 1, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 2,};
#ifndef SWE_UPSTREAM_COMPAT
/* A position in the mean ecliptic of t to the J2000 equator. */
static void nodaps_ecl_rot_j2000(swe_ctx *ctx, double *x, double t, int32 iflag)
{
  swi_coortrf(x, x, -swi_epsiln(ctx, t, iflag));
  swi_precess(ctx, x, t, iflag, J_TO_J2000);
}

/* swe_nod_aps()'s points under SEFLG_J2000 (UPSTREAM-BUGS 47): x, in the
 * mean ecliptic of tjd, to the J2000 equator; with SEFLG_SPEED the
 * velocity is rotated and given the frame's turning, R'(tjd) x, the
 * rotation's central difference over NODAPS_FRAME_DT a side (smooth over
 * centuries; truncation ~1e-15 of the rate). */
#define NODAPS_FRAME_DT 1.0
static void nodaps_ecl_to_j2000(swe_ctx *ctx, double *x, double tjd, int32 iflag)
{
  double xm[3], xp[3];
  int i;
  if (iflag & SEFLG_SPEED) {
    for (i = 0; i <= 2; i++)
      xm[i] = xp[i] = x[i];
    nodaps_ecl_rot_j2000(ctx, xm, tjd - NODAPS_FRAME_DT, iflag);
    nodaps_ecl_rot_j2000(ctx, xp, tjd + NODAPS_FRAME_DT, iflag);
    nodaps_ecl_rot_j2000(ctx, x + 3, tjd, iflag);
    for (i = 0; i <= 2; i++)
      x[i + 3] += (xp[i] - xm[i]) / (2 * NODAPS_FRAME_DT);
  }
  nodaps_ecl_rot_j2000(ctx, x, tjd, iflag);
}

#ifndef SWE_UPSTREAM_COMPAT
/* the end of swe_nod_aps()'s reads that take Moshier's Moon unrefused
 * (UPSTREAM-BUGS 192): the save areas they leave dropped, as a refused
 * read leaves none */
static void nodaps_moshmoon_end(swe_ctx *ctx)
{
  if (ctx->moshmoon_cancels) {
    ctx->moshmoon_cancels = FALSE;
    swi_force_app_pos_etc(ctx);
  }
}
#endif

/* swe_nod_aps()'s refusal of a barycentric Moshier answer (UPSTREAM-BUGS
 * 149), and since bug 179 of any: its points zeroed, as swe_calc() zeroes
 * a refused position and this function an unimplemented body's */
static int32 nodaps_refuse(double *xnasc, double *xndsc, double *xperi, double *xaphe)
{
  int i;
  for (i = 0; i <= 5; i++) {
    if (xnasc != NULL)
      xnasc[i] = 0;
    if (xndsc != NULL)
      xndsc[i] = 0;
    if (xperi != NULL)
      xperi[i] = 0;
    if (xaphe != NULL)
      xaphe[i] = 0;
  }
  return ERR;
}
#endif

/* ephret, if not NULL, gets the ephemeris that answered (bug 170) */
static int32 swe_nod_aps_r_body(swe_ctx *ctx, double tjd_et, int32 ipl, int32 iflag,
                      int32  method,
                      double *xnasc, double *xndsc, 
                      double *xperi, double *xaphe, 
                      char *serr, int32 *ephret)
{
  int ij, i, j;
  int32 iplx;
  int32 ipli;
  int istart, iend;
  int32 iflJ2000;
  double daya;
#ifdef SWE_UPSTREAM_COMPAT
  double plm;
#endif
  double t = (tjd_et - J2000) / 36525, dt;
  double x[6], xx[24], *xp, xobs[6], x2000[6]; 
#ifndef SWE_UPSTREAM_COMPAT
# define NODAPS_NOSC 5	/* the osculating points' epochs (bug 201) */
#else
# define NODAPS_NOSC 3
#endif
  double xpos[NODAPS_NOSC][6], xnorm[6];
  double xposm[6];
  double xn[NODAPS_NOSC][6], xs[NODAPS_NOSC][6];
  double xq[NODAPS_NOSC][6], xa[NODAPS_NOSC][6];
  double xobs2[6], x2[6];
  double *xna, *xnd, *xpe, *xap;
  double incl, sema, ecce, parg, ea, vincl, vsema, vecce;
#ifdef SWE_UPSTREAM_COMPAT
  double pargx, eax, pp, sinE;
#else
  double dtosc = 0, rosc[NODAPS_NOSC] = {0, 0, 0, 0, 0};
  AS_BOOL hold = FALSE, holdi;	/* bug 199 */
  int32 held[2];
  double teps = 0, tnut = 0;
#endif
  struct plan_data *pedp = &ctx->pldat[SEI_EARTH];
  struct plan_data *psbdp = &ctx->pldat[SEI_SUNBARY];
  struct plan_data pldat;
  double *xsun = psbdp->x;
  double *xear = pedp->x;
  const double *ep;
  double Gmsm, dzmin;
#ifndef SWE_UPSTREAM_COMPAT
  int32 retc0 = 0, retcs = 0, retx = 0, eph_asked, iflag_ctr = iflag;
  AS_BOOL out_bary, mosh_ctr = FALSE, moon_geo_mean = FALSE, ctr_bias = FALSE;
  double xsite[6] = {0, 0, 0, 0, 0, 0};	/* the Sun's points' site, bug 190 */
#endif
  double rxy, rxyz, fac, sgn;
  double sinnode, cosnode, sinincl, cosincl, sinu, cosu, cosE, cosE2;
  double uu, ny, ny2, c2, v2, ro, ro2, rn, rn2;
  struct epsilon *oe;
  AS_BOOL is_true_nodaps = FALSE;
  AS_BOOL do_aberr = !(iflag & (SEFLG_TRUEPOS | SEFLG_NOABERR));
  AS_BOOL do_defl = !(iflag & SEFLG_TRUEPOS) && !(iflag & SEFLG_NOGDEFL);
  AS_BOOL do_focal_point = method & SE_NODBIT_FOPOINT;
  AS_BOOL ellipse_is_bary = FALSE;
  int32 iflg0;
  iflag &= ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
#ifndef SWE_UPSTREAM_COMPAT
  /* A sidereal position is the mean ecliptic of date less the ayanamsa:
   * swe_calc() sets SEFLG_NONUT for it (plaus_iflag(): "if sidereal bit is
   * set, set also no_nutation bit") and this function did not, so a sidereal
   * node or apsis carried the nutation in longitude that the sidereal
   * planets beside it do not: +12.8" for the mean node on 1990-06-16, the
   * whole of delta-psi (G30). */
  if (iflag & SEFLG_SIDEREAL)
    iflag |= SEFLG_NONUT;
  /* and SEFLG_J2000 for the same reason (plaus_iflag(): "if J2000, then
   * no nutation"), UPSTREAM-BUGS 47 */
  if (iflag & SEFLG_J2000)
    iflag |= SEFLG_NONUT;
  /* Upstream bug 168: and one ephemeris bit, by plaus_iflag()'s
   * precedence (JPL, then Swiss, then Moshier). Its calls resolved the
   * bits so, but the tests here (SEFLG_MOSEPH, below) and those it passes
   * them to did not: swi_get_denum() reads Moshier first, so JPLEPH |
   * MOSEPH on de200, answered from DE200, turned the osculating ellipse
   * by the frame bias of DE >= 403 in swi_plan_for_osc_elem(): Mars's node
   * at 2000 Jul 1.5 1.5e-4 deg off the JPLEPH request's. */
  if (iflag & SEFLG_EPHMASK)
    iflag = (iflag & ~SEFLG_EPHMASK) | ((iflag & SEFLG_JPLEPH) ? SEFLG_JPLEPH
            : (iflag & SEFLG_SWIEPH) ? SEFLG_SWIEPH : SEFLG_MOSEPH);
  /* Upstream bug 169: and the centre bits as swe_calc() reads them
   * (plaus_iflag(): SEFLG_TOPOCTR turns SEFLG_HELCTR and SEFLG_BARYCTR
   * off, SEFLG_BARYCTR SEFLG_HELCTR). The tests below let SEFLG_HELCTR
   * win: with SEFLG_BARYCTR the points were heliocentric (Mars's
   * osculating node at 2000 Jul 1.5 49.5625 deg, where swe_calc() answers
   * barycentric, the files' 49.6398), with SEFLG_TOPOCTR too; and
   * SEFLG_TOPOCTR | SEFLG_BARYCTR gave the barycentric points less the
   * site's geocentric offset, nothing's place (49.6387, 1.2e-3 deg off the
   * barycentric node), where swe_calc() answers topocentric. */
  if (iflag & SEFLG_TOPOCTR)
    iflag &= ~(SEFLG_HELCTR | SEFLG_BARYCTR);
  if (iflag & SEFLG_BARYCTR)
    iflag &= ~SEFLG_HELCTR;
  /* Upstream bug 178: and no aberration or deflection for the heliocentric
   * and barycentric points, as swe_calc() has none for those places
   * (plaus_iflag() sets SEFLG_NOABERR | SEFLG_NOGDEFL). The heliocentric
   * points took the aberration of the Sun's barycentric velocity (Mars's
   * 3.0e-6 deg), and both took swi_deflect_light()'s deflection, whose
   * observer is the Earth whatever the centre: a vector from the Sun or
   * the barycentre deflected as if the Earth saw along it, Jupiter's mean
   * descending node at J2000, in line with the Sun from the Earth, 9.6e-4
   * deg, its barycentric speed 3.4e-2 deg/day. The flag itself is kept,
   * so that a star mode's ayanamsa stays swe_calc()'s for those places. */
  if (iflag & (SEFLG_HELCTR | SEFLG_BARYCTR))
    do_aberr = do_defl = FALSE;
#endif
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO */
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  xna = xx; 
  xnd = xx+6; 
  xpe = xx+12; 
  xap = xx+18;
  xpos[0][0] = 0; /* to shut up mint */
  /* to get control over the save area: */
  swi_force_app_pos_etc(ctx);
  method %= SE_NODBIT_FOPOINT;
  ipli = ipl;
  if (ipl == SE_SUN) 
    ipli = SE_EARTH;
  if (ipl == SE_MOON) {
    do_defl = FALSE;
    if (!(iflag & SEFLG_HELCTR))
      do_aberr = FALSE;
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 192: the Sun's points (the Earth's orbit, seen from the
   * Sun since bug 167) and the Earth's heliocentric and barycentric ones
   * read the Earth, and the osculating ones the Moon too, to make the
   * Earth-Moon barycentre again, so that without the moon files the strict
   * setting refused them (bug 120: "Swiss (.se1) Moon file is not
   * available"), where the Moon reaches none of them: the mean points are
   * the elements with the Sun, the ellipse is fitted to the barycentre,
   * the Earth less Moshier's Moon's share and that share again, and only
   * the deflection of the Sun's geocentric points looks from the Earth;
   * with the fallback they answered within 6.5e-15 deg of the complete
   * directory's (the speeds 1.5e-11 deg/day, the barycentres' rounding
   * over the ellipse's 2^-6 day). Now their reads take Moshier's Moon
   * unrefused (ctx->moshmoon_cancels), the ellipse's barycentre from the
   * Moon the Earth was made with, and the wrappers below drop the save
   * areas the reads leave, so that a strict request does not find there
   * an Earth made from Moshier's Moon. The Earth's geocentric and
   * topocentric points, seen from that Earth, stay refused. */
  if (ipli == SE_EARTH && (ipl == SE_SUN || (iflag & (SEFLG_HELCTR | SEFLG_BARYCTR))))
    ctx->moshmoon_cancels = TRUE;
#endif
  iflg0 = (iflag & (SEFLG_EPHMASK|SEFLG_NONUT)) | SEFLG_SPEED | SEFLG_TRUEPOS;
  if (ipli != SE_MOON)
    iflg0 |= SEFLG_HELCTR;
  if (ipl == SE_MEAN_NODE || ipl == SE_TRUE_NODE || 
	  ipl == SE_MEAN_APOG || ipl == SE_OSCU_APOG || 
	  ipl < 0 || 
	  (ipl >= SE_NPLANETS && ipl <= SE_AST_OFFSET)) 
	  // (ipl >= SE_FICT_OFFSET && ipl - SE_FICT_OFFSET < SE_NFICT_ELEM)) 
	  {
    if (serr != NULL)
      sprintf(serr, "nodes/apsides for planet %5.0f are not implemented", (double) ipl);
    if (xnasc != NULL)
      for (i = 0; i <= 5; i++)
	xnasc[i] = 0;
    if (xndsc != NULL)
      for (i = 0; i <= 5; i++)
	xndsc[i] = 0;
    if (xaphe != NULL)
      for (i = 0; i <= 5; i++)
	xaphe[i] = 0;
    if (xperi != NULL)
      for (i = 0; i <= 5; i++)
	xperi[i] = 0;
    return ERR;
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 149: barycentric points on Moshier, asked for or fallen
   * to, were its heliocentric ones. The change of centre below adds the
   * barycentric Sun (to the Moon's points the barycentric Earth) except on
   * Moshier, whose origin is the Sun and which serves no barycentric place,
   * and the barycentric output (SEFLG_BARYCTR, with SEFLG_HELCTR too since
   * bug 169 above) subtracts nothing: Mars's osculating ascending node at 2000
   * Jul 1.5 49.5626 deg, Moshier's heliocentric node to the last digit,
   * where the files' barycentric one is 49.6398 (0.077 deg, the latitude
   * 7.2e-3 deg); its mean node 0.077 deg off, the Moon's 0.37, the Sun's
   * mean perigee 0.40. Only SE_NODBIT_OSCU_BAR's ellipse beyond 6 AU, read
   * barycentric, went through swe_calc()'s refusal. The mean points read no
   * file, but their barycentric place does: the heliocentric one plus the
   * Sun's (the geocentric one plus the Earth's), which Moshier does not
   * serve; swe_calc() refuses SE_MEAN_NODE | SEFLG_MOSEPH | SEFLG_BARYCTR
   * too. Refused now as swecalc() refuses a barycentric Moshier request, on
   * the ephemeris bits' precedence (SEFLG_SWIEPH | SEFLG_MOSEPH is a Swiss
   * request): asked for, here, before anything is computed; a Swiss or JPL
   * request that falls to Moshier, after the call below that sets the Sun
   * and the Earth, by swe_calc_r()'s test (bug 139), with its message
   * (strict, the calls refuse the substitution with the downgrade's). */
  eph_asked = (iflag & SEFLG_JPLEPH) ? SEFLG_JPLEPH : (iflag & SEFLG_SWIEPH) ? SEFLG_SWIEPH
              : (iflag & SEFLG_MOSEPH) ? SEFLG_MOSEPH : SEFLG_DEFAULTEPH;
  out_bary = (iflag & SEFLG_BARYCTR) && !(iflag & SEFLG_HELCTR);
  if (out_bary && eph_asked == SEFLG_MOSEPH) {
    if (serr != NULL)
      strcpy(serr, "barycentric Moshier positions are not supported.");
    return nodaps_refuse(xnasc, xndsc, xperi, xaphe);
  }
#endif
  for (i = 0; i < 24; i++)
    xx[i] = 0;
  /***************************************
   * mean nodes and apsides 
   ***************************************/
  /* mean points only for Sun - Neptune */
  if ((method == 0 || (method & SE_NODBIT_MEAN)) &&
        ((ipl >= SE_SUN && ipl <= SE_NEPTUNE) || ipl == SE_EARTH)) {
    if (ipl == SE_MOON) {
      swi_mean_lunar_elements(ctx, tjd_et, &xna[0], &xna[3], &xpe[0], &xpe[3]);
      incl = MOON_MEAN_INCL;
      vincl = 0;
      ecce = MOON_MEAN_ECC;
      vecce = 0;
      sema = MOON_MEAN_DIST / AUNIT;
      vsema = 0;
    } else {
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 64: the planets' mean elements' rates were the linear
       * terms only, ep[1] / 36525; the t^2 and t^3 terms of the polynomials
       * were left out of every speed -- Mars 4.2e-8 deg/day in longitude,
       * 5.8e-9 in latitude, 6.6e-10 AU/day in distance at +-500 years, more
       * further out. The rates are now the polynomials' derivatives at t. */
      iplx = ipl_to_elem[ipl];
      ep = el_incl[iplx];
      incl = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      vincl = (ep[1] + t * (2 * ep[2] + 3 * ep[3] * t)) / 36525;
      ep = el_sema[iplx];
      sema = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      vsema = (ep[1] + t * (2 * ep[2] + 3 * ep[3] * t)) / 36525;
      ep = el_ecce[iplx];
      ecce = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      vecce = (ep[1] + t * (2 * ep[2] + 3 * ep[3] * t)) / 36525;
      ep = el_node[iplx];
      /* ascending node */
      xna[0] = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      xna[3] = (ep[1] + t * (2 * ep[2] + 3 * ep[3] * t)) / 36525;
      /* perihelion */
      ep = el_peri[iplx];
      xpe[0] = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      xpe[3] = (ep[1] + t * (2 * ep[2] + 3 * ep[3] * t)) / 36525;
#else
      iplx = ipl_to_elem[ipl];
      ep = el_incl[iplx];
      incl = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      vincl = ep[1] / 36525;
      ep = el_sema[iplx];
      sema = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      vsema = ep[1] / 36525;
      ep = el_ecce[iplx];
      ecce = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      vecce = ep[1] / 36525;
      ep = el_node[iplx];
      /* ascending node */
      xna[0] = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      xna[3] = ep[1] / 36525;
      /* perihelion */
      ep = el_peri[iplx];
      xpe[0] = ep[0] + ep[1] * t + ep[2] * t * t + ep[3] * t * t * t;
      xpe[3] = ep[1] / 36525;
#endif
    }
    /* descending node */
    xnd[0] = swe_degnorm(xna[0] + 180);
    xnd[3] = xna[3];
    /* angular distance of perihelion from node */
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 55: the speeds were differences over one day -- the
     * perigee's latitude 2e-5 deg/day off the rate for the Moon (curvature
     * of a 5.1 deg inclined orbit turning 0.11 deg/day), its longitude
     * 2e-6, the nodes' distance 6e-10 AU/day -- the aux. position a day on
     * less the position. They are now central differences of the elements
     * carried +-2^-6 day, the planets' mean apsides' too (bug 38's latitude
     * speed, xpe[4] -= xpe[1], is this with a one-sided step). */
    {
      const double hh = 1.0 / 64;
      double xa[2][3], rn[2], rd[2];
      int sg;
      parg = xpe[0] = swe_degnorm(xpe[0] - xna[0]);
      for (sg = 0; sg < 2; sg++) {
	double s1 = sg ? hh : -hh;
	double pg = swe_degnorm(parg + s1 * (xpe[3] - xna[3]));
	double ec = ecce + s1 * vecce, se = sema + s1 * vsema;
	xa[sg][0] = pg; xa[sg][1] = 0; xa[sg][2] = 0;
	/* transform from orbital plane to mean ecliptic of date */
	swe_cotrans(xa[sg], xa[sg], -(incl + s1 * vincl));
	/* add node again */
	xa[sg][0] = swe_degnorm(xa[sg][0] + xna[0] + s1 * xna[3]);
	ea = atan(tan(-pg * DEGTORAD / 2) * sqrt((1-ec)/(1+ec))) * 2;
	rn[sg] = se * (cos(ea) - ec) / cos(pg * DEGTORAD);
	ea = atan(tan((180 - pg) * DEGTORAD / 2) * sqrt((1-ec)/(1+ec))) * 2;
	rd[sg] = se * (cos(ea) - ec) / cos((180 - pg) * DEGTORAD);
      }
      /* transform from orbital plane to mean ecliptic of date */
      swe_cotrans(xpe, xpe, -incl);
      /* add node again */
      xpe[0] = swe_degnorm(xpe[0] + xna[0]);
      xpe[3] = swe_difdeg2n(xa[1][0], xa[0][0]) / (2 * hh);
      xpe[4] = (xa[1][1] - xa[0][1]) / (2 * hh);
      /* heliocentric distance of perihelion and aphelion */
      xpe[2] = sema * (1 - ecce);
      xpe[5] = (sema + vsema) * (1 - ecce - vecce) - xpe[2];
      /* aphelion */
      xap[0] = swe_degnorm(xpe[0] + 180);
      xap[1] = -xpe[1];
      xap[3] = xpe[3];
      xap[4] = -xpe[4];
      if (do_focal_point) {
	xap[2] = sema * ecce * 2;
	xap[5] = (sema + vsema) * (ecce + vecce) * 2 - xap[2];
      } else {
	xap[2] = sema * (1 + ecce);
	xap[5] = (sema + vsema) * (1 + ecce + vecce) - xap[2];
      }
      /* heliocentric distance of nodes */
      ea = atan(tan(-parg * DEGTORAD / 2) * sqrt((1-ecce)/(1+ecce))) * 2;
      xna[2] = sema * (cos(ea) - ecce) / cos(parg * DEGTORAD);
      xna[5] = (rn[1] - rn[0]) / (2 * hh);
      ea = atan(tan((180 - parg) * DEGTORAD / 2) * sqrt((1-ecce)/(1+ecce))) * 2;
      xnd[2] = sema * (cos(ea) - ecce) / cos((180 - parg) * DEGTORAD);
      xnd[5] = (rd[1] - rd[0]) / (2 * hh);
    }
#else
    parg = xpe[0] = swe_degnorm(xpe[0] - xna[0]);
    pargx = xpe[3] = swe_degnorm(xpe[0] + xpe[3]  - xna[3]);
    /* transform from orbital plane to mean ecliptic of date */
    swe_cotrans(xpe, xpe, -incl);
    /* xpe+3 is aux. position, not speed!!! */ 
    swe_cotrans(xpe+3, xpe+3, -incl-vincl);	
    /* add node again */
    xpe[0] = swe_degnorm(xpe[0] + xna[0]);
    /* xpe+3 is aux. position, not speed!!! */ 
    xpe[3] = swe_degnorm(xpe[3] + xna[0] + xna[3]);
    /* speed */
    xpe[3] = swe_degnorm(xpe[3] - xpe[0]);
    /* heliocentric distance of perihelion and aphelion */
    xpe[2] = sema * (1 - ecce);
    xpe[5] = (sema + vsema) * (1 - ecce - vecce) - xpe[2];
    /* aphelion */
    xap[0] = swe_degnorm(xpe[0] + 180);
    xap[1] = -xpe[1];
    xap[3] = xpe[3];
    xap[4] = -xpe[4];
    if (do_focal_point) {
      xap[2] = sema * ecce * 2;
      xap[5] = (sema + vsema) * (ecce + vecce) * 2 - xap[2];
    } else {
      xap[2] = sema * (1 + ecce);
      xap[5] = (sema + vsema) * (1 + ecce + vecce) - xap[2];
    }
    /* heliocentric distance of nodes */
    ea = atan(tan(-parg * DEGTORAD / 2) * sqrt((1-ecce)/(1+ecce))) * 2;
    eax = atan(tan(-pargx * DEGTORAD / 2) * sqrt((1-ecce-vecce)/(1+ecce+vecce))) * 2;
    xna[2] = sema * (cos(ea) - ecce) / cos(parg * DEGTORAD);
    xna[5] = (sema+vsema) * (cos(eax) - ecce - vecce) / cos(pargx * DEGTORAD);
    xna[5] -= xna[2];
    ea = atan(tan((180 - parg) * DEGTORAD / 2) * sqrt((1-ecce)/(1+ecce))) * 2;
    eax = atan(tan((180 - pargx) * DEGTORAD / 2) * sqrt((1-ecce-vecce)/(1+ecce+vecce))) * 2;
    xnd[2] = sema * (cos(ea) - ecce) / cos((180 - parg) * DEGTORAD);
    xnd[5] = (sema+vsema) * (cos(eax) - ecce - vecce) / cos((180 - pargx) * DEGTORAD);
    xnd[5] -= xnd[2];
#endif
    /* no light-time correction because speed is extremely small */
    for (i = 0, xp = xx; i < 4; i++, xp += 6) {
      /* to cartesian coordinates */
      xp[0] *= DEGTORAD;
      xp[1] *= DEGTORAD;
      xp[3] *= DEGTORAD;
      xp[4] *= DEGTORAD;
      swi_polcart_sp(xp, xp);
    }
  /***************************************
   * "true" or osculating nodes and apsides 
   ***************************************/
  } else {
    /* first, we need a heliocentric distance of the planet */
#ifndef SWE_UPSTREAM_COMPAT
    if ((retc0 = swe_calc_r(ctx, tjd_et, ipli, iflg0, x, serr)) == ERR)
      return ERR;
#else
    if (swe_calc_r(ctx, tjd_et, ipli, iflg0, x, serr) == ERR)
      return ERR;
#endif
    iflJ2000 = (iflag & SEFLG_EPHMASK)|SEFLG_J2000|SEFLG_EQUATORIAL|SEFLG_XYZ|SEFLG_TRUEPOS|SEFLG_NONUT|SEFLG_SPEED;
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 108: swe_calc()'s J2000 state has the frame bias
     * (DE >= 403: Swiss files, Moshier, DE4xx), and swi_plan_for_osc_elem()
     * below applies it again: every osculating point was turned by it
     * twice, 5.5e-6 deg on the Moon's apogee against SE_OSCU_APOG's
     * (6.2e-6 in latitude), twice that with SEFLG_ICRS, where it was
     * applied twice and SE_OSCU_APOG's not at all. Now the ICRS state,
     * biased once there unless SEFLG_ICRS is asked for, as
     * lunar_osc_elem() does with the ephemeris's own. */
    iflJ2000 |= SEFLG_ICRS;
#endif
    ellipse_is_bary = FALSE;
    if (ipli != SE_MOON) {
      if ((method & SE_NODBIT_OSCU_BAR) && x[2] > 6) {
        iflJ2000 |= SEFLG_BARYCTR; /* only planets beyond Jupiter */
        ellipse_is_bary = TRUE;
      } else {
        iflJ2000 |= SEFLG_HELCTR;
      }
    }
    /* we need three positions and three speeds
     * for three nodes/apsides. from the three node positions,
     * the speed of the node will be computed. */
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 105: the ellipses' GM was the Astronomical Almanac's,
     * G M_earth (1 + 1 / EARTH_MOON_MRAT) for the Moon, 3.3e-8 above the
     * ephemeris's GMB (DE431/441; bug 28 fixed lunar_osc_elem() alone), and
     * HELGRAVCONST / AUNIT^3 for the Sun, 1.8e-10 below every JPL
     * header's, with plmass's planets (DE200's Uranus is 2.5e-3 heavier)
     * and Mercury's for Pluto.
     * The eccentricity vector turns by about dGM / (GM e): the Moon's
     * osculating apsides were 5.3e-5 deg off SE_OSCU_APOG's. Now the
     * ephemeris's own constants (swi_gm_earth_moon(), swi_gm_sun_planet()). */
    if (ipli == SE_MOON) {
      dt = NODE_CALC_INTV;
      dzmin = 1e-15;
      Gmsm = swi_gm_earth_moon(ctx, retc0 & SEFLG_EPHMASK);
    } else {
      /* ipl_to_elem[] gives Pluto Mercury's element, 0: Pluto's ellipse
       * took Mercury's mass, 1.6e-7 of the Sun's (its perihelion 1e-5 AU
       * off, 3.7e-5 deg) */
      int iel = ipli == SE_PLUTO ? 8
                : ((ipli >= SE_MERCURY && ipli < SE_PLUTO) || ipli == SE_EARTH)
                ? ipl_to_elem[ipl] : -1;
      dt = NODE_CALC_INTV * 10 * x[2];
      dzmin = 1e-15 * dt / NODE_CALC_INTV;
      Gmsm = swi_gm_sun_planet(ctx, retc0 & SEFLG_EPHMASK, iel);
    }
#else
    if (ipli == SE_MOON) {
      dt = NODE_CALC_INTV;
      dzmin = 1e-15;
      Gmsm = GEOGCONST * (1 + 1 / EARTH_MOON_MRAT) /AUNIT/AUNIT/AUNIT*86400.0*86400.0;
    } else {
      if ((ipli >= SE_MERCURY && ipli <= SE_PLUTO) || ipli == SE_EARTH)
        plm = 1 / plmass[ipl_to_elem[ipl]];
      else
        plm = 0;
      dt = NODE_CALC_INTV * 10 * x[2];
      dzmin = 1e-15 * dt / NODE_CALC_INTV;
      Gmsm = HELGRAVCONST * (1 + plm) /AUNIT/AUNIT/AUNIT*86400.0*86400.0;
    }
#endif
    if (iflag & SEFLG_SPEED) {
      istart = 0;
#ifndef SWE_UPSTREAM_COMPAT
      iend = 4;		/* bug 201: five points */
#else
      iend = 2;
#endif
    } else {
      istart = iend = 0;
      dt = 0;
    }
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 80: the speeds were differences over tjd -+ dt, dt =
     * 1e-3 day per AU of distance (7e-4 for Venus) and 1e-4 for the Moon,
     * at instants rounded twice, so 2 dt was off their spacing by up to a
     * JD's ulp (2.3e-6 of the Moon's points' speeds, 2.0e-6 deg/day on its
     * perigee); and over so short a step the points' own rounding (the
     * eccentricity's, below) made 5.6e-7 deg/day on Venus's perihelion, 6e-8
     * on the Sun's perigee. Now tjd -+ a binary step, exact, divided by the
     * instants' own spacing: 2^-6 day for the planets, where the frame's
     * nutation (the points are in the ecliptic of each instant) bends the
     * difference by 2e-10 deg/day and the points' rounding adds about as
     * much (Neptune's 7e-10 at -3000), and 2^-11 for the Moon, whose
     * perigee turns too fast for more (4e-8 deg/day there, against 5e-7 at
     * 2^-9 and 1.7e-7 at 2^-13).
     * Upstream bug 201: and a three-point difference is off by h^2 / 6 of
     * the point's third derivative, which a near-circular orbit's apsides
     * make large, and over the Moon's 2^-11 day by its points' noise over
     * 2 h: against nine-point differences of the positions over 2^-4 day
     * (the Moon 2^-5), at 1125 dates 1800-2100 on the Swiss files,
     * Neptune's heliocentric perihelion 5.3e-7 deg/day off in 1815 (it
     * turns at 0.1 deg/day there, its third derivative 1.3e-2 deg/day^3),
     * Uranus's 4.2e-8, the Earth's 3.5e-8, Jupiter's and Saturn's 2.2e-8
     * and 2.0e-8, Venus's and Mercury's 1.3e-8 and 1.2e-8, the Moon's
     * geocentric apsides 7.9e-8 and its nodes 1.6e-8 (7.8e-8 and 6.8e-8 on
     * de200 and Moshier). Now the five-point difference over tjd + k h, k = -2..2, exact
     * but for a binade's rounding, which is taken back to first order as in
     * lunar_osc_elem() (bug 75): h = 2^-5 day for the planets (1.0e-10
     * deg/day heliocentric; 2.8e-9 at 2^-6, the points' noise) and 2^-6 for
     * the Moon (3.1e-9 geocentric; 1.8e-8 at 2^-5, its truncation). */
    dtosc = ipli == SE_MOON ? 1.0 / 64 : 1.0 / 32;
    /* Upstream bug 199: and each epoch's place is put into the frame of
     * date at that epoch (swi_plan_for_osc_elem()), so where the stencil
     * spans a short-term precession model's window edge (entry 160: 0.53"
     * of precession and 3.4 mas of mean obliquity at J2000 - 2 centuries
     * with IAU 1976 under Vondrak) the difference took the frame's step as
     * the orbit's motion, as the true node's did (bug 186): under "0,9,1"
     * on the Swiss files within 1/8 day of J2000 - 2 centuries Mars's
     * ascending node's speed 5.2e-3 deg/day off, the Moon's nodes 8.0e-2,
     * its apsides 0.16; with the five-point difference above the Moon's
     * points 6.5e-3 and Uranus's 1.7e-2 within its stencil, and under
     * "0,9,8" at -+ 75 centuries the Moon's 7.9, Mars's 8.1, Jupiter's 41.
     * Now within the stencil's reach of an edge every epoch is put into the
     * frame of tjd's side's model (swi_frame_model_hold()), so the speeds
     * are that side's own rate (within 4.6e-9 deg/day); tjd's own place is
     * in the frame it had, and everything farther from an edge as it was.
     * Whether to hold: */
    hold = (iflag & SEFLG_SPEED)
           && swi_frame_step_side(ctx, tjd_et, 2 * dtosc, iflag, SWI_FRAME_PREC | SWI_FRAME_EPS) != 0;
# ifdef SID_TNODE_FROM_ECL_T0
    if (iflag & SEFLG_SIDEREAL)	/* the frame is t0's at every epoch */
      hold = FALSE;
# endif
#endif
    for (i = istart, t = tjd_et - dt; i <= iend; i++, t += dt) {
      if (istart == iend)
        t = tjd_et;
#ifndef SWE_UPSTREAM_COMPAT
      /* bug 46 as well: the middle instant is tjd itself, as are the points
       * without SEFLG_SPEED; bugs 80 and 201: the others tjd + k h, and
       * rosc[i] what rounding took from them, exact (two-sum) */
      if (istart != iend) {
        double bb, kh = (i - 2) * dtosc;
        t = tjd_et + kh;
        bb = t - tjd_et;
        rosc[i] = (tjd_et - (t - bb)) + (kh - bb);
      }
      if ((retx = swe_calc_r(ctx, t, ipli, iflJ2000, xpos[i], serr)) == ERR)
        return ERR;
#else
      if (swe_calc_r(ctx, t, ipli, iflJ2000, xpos[i], serr) == ERR)
        return ERR;
#endif
      /* the EMB is used instead of the earth */
      if (ipli == SE_EARTH) {
#ifndef SWE_UPSTREAM_COMPAT
        /* bug 192: an Earth made from Moshier's Moon, the moon files
         * missing, gets its barycentre back from the Moon it was made
         * with, which its read saved; a read of the Moon itself is refused
         * under the strict setting */
        if (ctx->moshmoon_cancels && ctx->moshmoon_earth_t == t && pedp->teval == t
            && ctx->pldat[SEI_MOON].teval == t && ctx->pldat[SEI_MOON].iephe == SEFLG_MOSEPH) {
          for (j = 0; j <= 5; j++)
            xposm[j] = ctx->pldat[SEI_MOON].x[j];
        } else
#endif
        if (swe_calc_r(ctx, t, SE_MOON, iflJ2000 & ~(SEFLG_BARYCTR|SEFLG_HELCTR), xposm, serr) == ERR)
          return ERR;
#ifndef SWE_UPSTREAM_COMPAT
        /* Upstream bug 180: on Moshier the Earth is its series' EMB less
         * the short lunar series' Moon (embofs_mosh()), 0.24 deg from
         * Moshier's own Moon at 2000, 1.55 deg at -3000; adding that one's
         * share back made the EMB 1.3e-7 AU and 3.2e-8 AU/day off the
         * series' (7.9e-7 and 2.0e-7 at -3000), in the Moon's inequalities
         * of 9 to 35 days. The ellipse turned that into a wobble of the
         * perihelion, 8.0e-3 deg at 2000, 4.6e-2 at -2700, whose third
         * derivative, 1.6e-3 deg/day^3, put the speed over +-2^-6 day
         * 4.7e-8 deg/day off the positions' rate (the distance's 2.5e-11
         * AU/day), at every epoch: the Earth's heliocentric points, the
         * Sun's geocentric ones. Now the Moon the Earth was made with, the
         * one its computation keeps (ctx->earth_moon), so the EMB is the
         * series' own. */
        if ((retx & SEFLG_EPHMASK) == SEFLG_MOSEPH && ctx->earth_moon.t == t
            && ctx->earth_moon.iephe == SEFLG_MOSEPH
            && ctx->pldat[SEI_EARTH].teval == t && ctx->pldat[SEI_EARTH].iephe == SEFLG_MOSEPH)
          for (j = 0; j <= 5; j++)
            xposm[j] = ctx->earth_moon.x[j];
        /* Upstream bug 108: with the EMB's mass ratio the Earth was made
         * from (swi_emb_mrat(): a JPL file's EMRAT, not the AA's) */
        for (j = 0; j <= 5; j++)
          xpos[i][j] += xposm[j] / (swi_emb_mrat(ctx, retc0 & SEFLG_EPHMASK) + 1.0);
#else
        for (j = 0; j <= 5; j++)
          xpos[i][j] += xposm[j] / (EARTH_MOON_MRAT + 1.0);
#endif
      }
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 176: the frame bias here is the ephemeris's, by
       * swi_get_denum() of iflg0's bit, the one asked for: SEFLG_JPLEPH on
       * de200, fallen to Moshier outside its years, gave the Moshier state
       * DE200's frame, no bias, where swe_calc() biases a fallen request by
       * the ephemeris that answered: every osculating point off the
       * Moshier request's, Mars's node at 1500 Jan 1 1.58e-4 deg
       * heliocentric, 1.56e-4 geocentric, the Moon's 2.9e-5. Now the bit
       * of the call that gave this state. */
      /* bug 199: not Moshier's Moon, which is of date already, taken to
       * J2000 by the frame at its epoch (ecldat_equ2000()), which
       * swi_plan_for_osc_elem() undoes (lunar_osc_elem() holds none) */
      holdi = hold && !(ipli == SE_MOON && (retx & SEFLG_EPHMASK) == SEFLG_MOSEPH);
      if (holdi) {
        /* the obliquity and nutation memos swe_calc() left at another
         * epoch are its own side's model's: computed afresh under the held
         * model, and the memos kept */
        teps = ctx->oec.teps;
        tnut = ctx->nut.tnut;
        if (t != tjd_et)
          ctx->oec.teps = ctx->nut.tnut = SWI_TJD_NEVER;
        swi_frame_model_hold(ctx, tjd_et, held);
      }
      swi_plan_for_osc_elem(ctx, (iflg0 & ~SEFLG_EPHMASK) | (retx & SEFLG_EPHMASK)
                            | (iflag & SEFLG_ICRS), t, xpos[i]);	/* bug 108 */
      if (holdi) {
        swi_frame_model_release(ctx, held);
        ctx->oec.teps = teps;
        ctx->nut.tnut = tnut;
      }
#else
      swi_plan_for_osc_elem(ctx, iflg0, t, xpos[i]);
#endif
    }
    for (i = istart; i <= iend; i++) {
      if (fabs(xpos[i][5]) < dzmin)
        xpos[i][5] = dzmin;
      fac = xpos[i][2] / xpos[i][5];
      sgn = xpos[i][5] / fabs(xpos[i][5]);
      for (j = 0; j <= 2; j++) {
        xn[i][j] = (xpos[i][j] - fac * xpos[i][j+3]) * sgn;
        xs[i][j] = -xn[i][j];
      }
    }
    for (i = istart; i <= iend; i++) {
      /* node */
      rxy =  sqrt(xn[i][0] * xn[i][0] + xn[i][1] * xn[i][1]);
      cosnode = xn[i][0] / rxy;	
      sinnode = xn[i][1] / rxy;
      /* inclination */
      swi_cross_prod(xpos[i], xpos[i]+3, xnorm);
      rxy =  xnorm[0] * xnorm[0] + xnorm[1] * xnorm[1];
      c2 = (rxy + xnorm[2] * xnorm[2]);
      rxyz = sqrt(c2);
      rxy = sqrt(rxy);
      sinincl = rxy / rxyz;
      cosincl = sqrt(1 - sinincl * sinincl);
      if (xnorm[2] < 0) cosincl = -cosincl; /* retrograde asteroid, e.g. 20461 Dioretsa */
      /* argument of latitude */
      cosu = xpos[i][0] * cosnode + xpos[i][1] * sinnode;
      sinu = xpos[i][2] / sinincl;	
      uu = atan2(sinu, cosu);	
      /* semi-axis */
      rxyz = sqrt(square_sum(xpos[i]));
      v2 = square_sum((xpos[i]+3));
      sema = 1 / (2 / rxyz - v2 / Gmsm);	
#ifndef SWE_UPSTREAM_COMPAT
      /* Upstream bug 80: the eccentricity was sqrt(1 - p / a), where 1 - p / a
       * is e^2 less a few ulps, so e carried 1e-16 / e^2 of itself, and the
       * true anomaly from it (cos E = (1 - r / a) / e) 1e-11 rad on Venus's
       * orbit (e = 0.0068): its perihelion 5e-10 deg rms from instant to
       * instant, the speed, differenced over 2 x 7.2e-4 day, 6e-7 deg/day off.
       * Now the eccentricity vector, ((v^2 - GM / r) r - (r.v) v) / GM: e
       * its length and the true anomaly the angle from it to r about h,
       * each 1e-16 / e at worst. The same quantities. */
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
      /* distance of perihelion from ascending node */
      xq[i][0] = swi_mod2PI(uu - ny);
      xq[i][1] = 0;			/* latitude */
      xq[i][2] = sema * (1 - ecce);	/* distance of perihelion */
      /* transformation to ecliptic coordinates */
      swi_polcart(xq[i], xq[i]);
      swi_coortrf2(xq[i], xq[i], -sinincl, cosincl);
      swi_cartpol(xq[i], xq[i]);
      /* adding node, we get perihelion in ecl. coord. */
      xq[i][0] += atan2(sinnode, cosnode);
      xa[i][0] = swi_mod2PI(xq[i][0] + PI);
      xa[i][1] = -xq[i][1];
      if (do_focal_point) {
        xa[i][2] = sema * ecce * 2;	/* distance of aphelion */
      } else {
        xa[i][2] = sema * (1 + ecce);	/* distance of aphelion */
      }
      swi_polcart(xq[i], xq[i]);
      swi_polcart(xa[i], xa[i]);
      /* new distance of node from orbital ellipse:
       * true anomaly of node: */
      ny = swi_mod2PI(ny - uu);
      ny2 = swi_mod2PI(ny + PI);
      /* eccentric anomaly */
      cosE = cos(2 * atan(tan(ny / 2) / sqrt((1+ecce) / (1-ecce))));
      cosE2 = cos(2 * atan(tan(ny2 / 2) / sqrt((1+ecce) / (1-ecce))));
      /* new distance */
      rn = sema * (1 - ecce * cosE);
      rn2 = sema * (1 - ecce * cosE2);
      /* old node distance */
      ro = sqrt(square_sum(xn[i]));
      ro2 = sqrt(square_sum(xs[i]));
      /* correct length of position vector */
      for (j = 0; j <= 2; j++) {
        xn[i][j] *= rn / ro;
        xs[i][j] *= rn2 / ro2;
      }
    }
    for (i = 0; i <= 2; i++) {
      if (iflag & SEFLG_SPEED) {
#ifndef SWE_UPSTREAM_COMPAT
        /* bug 201: the five-point difference, times 1 + D[rosc] for the
         * instants' rounding: x(tk - r) = x(tk) - v r (bug 75) */
        double f = (1 + (8 * (rosc[3] - rosc[1]) - (rosc[4] - rosc[0])) / (12 * dtosc))
                   / (12 * dtosc);
        xpe[i] = xq[2][i];
        xpe[i+3] = (8 * (xq[3][i] - xq[1][i]) - (xq[4][i] - xq[0][i])) * f;
        xap[i] = xa[2][i];
        xap[i+3] = (8 * (xa[3][i] - xa[1][i]) - (xa[4][i] - xa[0][i])) * f;
        xna[i] = xn[2][i];
        xna[i+3] = (8 * (xn[3][i] - xn[1][i]) - (xn[4][i] - xn[0][i])) * f;
        xnd[i] = xs[2][i];
        xnd[i+3] = (8 * (xs[3][i] - xs[1][i]) - (xs[4][i] - xs[0][i])) * f;
#else
        xpe[i] = xq[1][i];
        xpe[i+3] = (xq[2][i] - xq[0][i]) / dt / 2;
        xap[i] = xa[1][i];
        xap[i+3] = (xa[2][i] - xa[0][i]) / dt / 2;
        xna[i] = xn[1][i];
        xna[i+3] = (xn[2][i] - xn[0][i]) / dt / 2;
        xnd[i] = xs[1][i];
        xnd[i+3] = (xs[2][i] - xs[0][i]) / dt / 2;
#endif
      } else {
        xpe[i] = xq[0][i];
        xpe[i+3] = 0;
        xap[i] = xa[0][i];
        xap[i+3] = 0;
        xna[i] = xn[0][i];
        xna[i+3] = 0;
        xnd[i] = xs[0][i];
        xnd[i+3] = 0;
      }
    }
    is_true_nodaps = TRUE;
  }
  /* to set the variables required in the save area,
   * i.e. ecliptic, nutation, barycentric sun, earth
   * we compute the planet */
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 174: the Moon's mean points (every method but the
   * osculating ones) geocentric or topocentric read the Moon here, and
   * nothing of it reaches them: the elements read no file, and the
   * barycentric Earth added to the points and to the observer below
   * cancels. They need the ecliptic and the nutation of date, and the site
   * its delta t. Yet the read made them depend on the Moon files: without
   * them the strict setting refused them ("Swiss (.se1) Moon file is not
   * available"), and in an empty directory every ephemeris but Moshier, as
   * JPL without its file, where swe_calc() answers SE_MEAN_NODE under the
   * ephemeris asked; with the fallback the read fell to Moshier and said
   * so, so that a delta t taken again for the ephemeris that answered (bug
   * 170) moved them, the mean node by swe_nod_aps_ut() 6.0e-8 deg off at
   * 1900 and 1.8e-5 in AD 1000, the apogee 1.3e-7 and 3.8e-5, and in an
   * empty directory the site took Moshier's delta t (the topocentric node
   * 1.4e-6 deg off at 1900, 1.5e-4 in AD 1000). Now the ecliptic and
   * nutation alone (SE_ECL_NUT, which returns the ephemeris asked), no
   * Earth added or taken away (its rounding, 1.6e-11 deg on the focal
   * point, gone), the site placed by the delta t of the ephemeris asked,
   * and swe_calc()'s range of the mean node. */
  moon_geo_mean = ipli == SE_MOON && !is_true_nodaps && !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR));
#endif
  if (ipli == SE_MOON && (iflag & (SEFLG_HELCTR | SEFLG_BARYCTR))) {
    swi_force_app_pos_etc(ctx);
#ifndef SWE_UPSTREAM_COMPAT
    if ((retcs = swe_calc_r(ctx, tjd_et, SE_SUN, iflg0, x, serr)) == ERR)
#else
    if (swe_calc_r(ctx, tjd_et, SE_SUN, iflg0, x, serr) == ERR)
#endif
      return ERR;
#ifndef SWE_UPSTREAM_COMPAT
  } else if (moon_geo_mean) {
    /* swe_calc()'s range of the mean node, its speed's instants in it */
    double tm = tjd_et - 1.0 / 32, tp = tjd_et + 1.0 / 32;
    if (tm < MOSHNDEPH_START || tp > MOSHNDEPH_END) {
      if (serr != NULL)
        sprintf(serr, "jd %f outside mean node range %.2f .. %.2f ",
                tm < MOSHNDEPH_START ? tm : tp, MOSHNDEPH_START, MOSHNDEPH_END);
      return ERR;
    }
    if ((retcs = swe_calc_r(ctx, tjd_et, SE_ECL_NUT, iflg0, x, serr)) == ERR)
      return ERR;
#endif
  } else {
#ifndef SWE_UPSTREAM_COMPAT
    /* Not TOPOCTR for the Earth (the Sun's orbit): a topocentric Earth
     * returns before its barycentric position reaches the save area, and
     * swi_deflect_light() below reads that slot -- NaN on a fresh context,
     * and on a used one whatever Earth an earlier call left, so the Sun's
     * mean perihelion moved with call order (2026-09-16 review, F6).
     * The observer is computed separately below; this call is only for
     * the save area. Upstream hides it: its swe_set_ephe_path() computes
     * the Moon at J2000, leaving an Earth there. */
    if ((retcs = swe_calc_r(ctx, tjd_et, ipli, iflg0 | (ipli == SE_EARTH ? 0 :
        (iflag & SEFLG_TOPOCTR)), x, serr)) == ERR)
      return ERR;
#else
    if (swe_calc_r(ctx, tjd_et, ipli, iflg0 | (iflag & SEFLG_TOPOCTR), x, serr) == ERR)
      return ERR;
#endif
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 149: and a request fallen to Moshier, whose Sun and Earth
   * this call has just set (a Moon from Moshier on the files' Earth, the
   * Moon file alone missing, is a barycentric place and answers, as
   * swe_calc()'s barycentric Moon does) */
  if (out_bary && swi_refuse_mosh_bary(ctx, eph_asked,
                                       (retcs & SEFLG_EPHMASK) | SEFLG_BARYCTR, tjd_et, serr))
    return nodaps_refuse(xnasc, xndsc, xperi, xaphe);
  /* Upstream bug 193: a planet's points geocentric or topocentric are
   * seen from the Earth the read above saved, and the read, heliocentric,
   * passes bug 120's test: without the moon files the strict setting
   * answered them from an Earth made from Moshier's Moon (Mars's points at
   * 1900 3.7e-9 deg off the complete directory's in latitude, Mercury's at
   * 2000 8.7e-9 in longitude), where swe_calc() of the planet refuses ("the
   * Moon, and the Earth made from it, would come from Moshier"). Refused
   * now as swe_calc() refuses it, with its message, the points zeroed;
   * with the fallback answered as before. The heliocentric and barycentric
   * points do not use that Earth. */
  if (ipli != SE_MOON && ipli != SE_EARTH && !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR))
      && swi_refuse_moshmoon_earth(ctx, tjd_et, serr))
    return nodaps_refuse(xnasc, xndsc, xperi, xaphe);
  /* Upstream bug 168: the change of centre below read the caller's
   * ephemeris, not the one that answered: Moshier's frame (the Sun its
   * origin, no barycentric Sun added) from SEFLG_MOSEPH, the topocentric
   * site from the asked ephemeris's delta t. A Swiss or JPL request fallen
   * to Moshier added the save area's Sun and took it for the heliocentric
   * observer, zero in a fresh process but another call's Swiss Sun in a
   * warm one (the files of 1800-2400 alone, after a call at 2000: Mars's
   * geocentric osculating node at 1500 Jan 1 4.3236 deg for Moshier's
   * 4.4536, the Moon's heliocentric mean node 99.7972 for 100.1993), and
   * placed the site by the files' delta t (the Moon's topocentric mean
   * perigee there 2.9e-4 deg off Moshier's, fresh or warm); and SEFLG_SWIEPH
   * | SEFLG_MOSEPH, answered from the files, took Moshier's frame: Mars's
   * geocentric node at 2000 Jul 1.5 69.7303 deg for 69.8382, the
   * barycentric one the heliocentric 49.5625 for 49.6398 (its bits are one
   * now, above). Now both are those of the Earth and the Sun the call
   * above has set, the ephemeris of the save area's Earth: a Moon from
   * Moshier on the files' Earth, the Moon file alone missing, keeps the
   * files' frame and site, as swe_calc() computes it. */
  if (moon_geo_mean) {	/* bug 174: no Earth read; the ephemeris asked */
    int32 e = (iflag & SEFLG_EPHMASK) ? (iflag & SEFLG_EPHMASK) : SEFLG_SWIEPH;
    mosh_ctr = e == SEFLG_MOSEPH;
    iflag_ctr = (iflag & ~SEFLG_EPHMASK) | e;
  } else {
    mosh_ctr = pedp->iephe == SEFLG_MOSEPH;
    iflag_ctr = (iflag & ~SEFLG_EPHMASK) | (pedp->iephe & SEFLG_EPHMASK);
  }
  /* Upstream bug 221: the points are J2000 (the mean ones from elements of
   * date, the osculating ones from a state swi_plan_for_osc_elem() biased),
   * and the change of centre below adds and takes away the barycentric Sun
   * and Earth, which are in the ephemeris's frame, ICRS for DE 403 and
   * later (here the site too, which swi_get_observer() turns into it):
   * every point off by the frame bias, 23 mas, times the vectors that do
   * not cancel. On the files a planet's geocentric points were 3.5e-8 AU
   * off its heliocentric ones less the Earth at J2000, 9.6e-8 AU and 1.9e-9
   * AU/day at G163's instants -3000..3000 (2.6e-6 deg on Mercury's apparent
   * perihelion), the barycentric ones, and the heliocentric ones of a
   * barycentric ellipse, 9.7e-10 AU, the Moon's heliocentric and
   * barycentric points 9.6e-8 AU off its geocentric ones plus the Earth
   * (1.9e-6 deg), its and the Sun's topocentric ones 4.8e-12 AU off the
   * geocentric less the site; Moshier alike; with SEFLG_ICRS and on de200
   * (no bias) within 7e-15. Now the points go into the ephemeris's frame
   * for the change of centre, the deflection and the aberration, and back
   * to J2000 after, as swe_calc() takes a place through app_pos_rest();
   * with SEFLG_ICRS and on de200, where the bias is the identity, bit for
   * bit as before. */
  ctr_bias = !(iflag & SEFLG_ICRS) && swi_get_denum(ctx, SEI_EARTH, iflag_ctr) >= 403;
#endif
  /***********************
   * position of observer
   ***********************/
  if (iflag & SEFLG_TOPOCTR) {
    /* geocentric position of observer */
#ifndef SWE_UPSTREAM_COMPAT
    if (swi_get_observer(ctx, tjd_et, iflag_ctr, FALSE, xobs, serr) != OK)	/* bug 168 */
#else
    if (swi_get_observer(ctx, tjd_et, iflag, FALSE, xobs, serr) != OK)
#endif
      return ERR;
    /*for (i = 0; i <= 5; i++)
      xobs[i] = ctx->topd.xobs[i];*/
  } else {
    for (i = 0; i <= 5; i++)
      xobs[i] = 0;
  }
  if (iflag & (SEFLG_HELCTR | SEFLG_BARYCTR)) {
#ifndef SWE_UPSTREAM_COMPAT
    if ((iflag & SEFLG_HELCTR) && !mosh_ctr)	/* bug 168 */
#else
    if ((iflag & SEFLG_HELCTR) && !(iflag & SEFLG_MOSEPH))
#endif
      for (i = 0; i <= 5; i++)
        xobs[i] = xsun[i];
#ifndef SWE_UPSTREAM_COMPAT
  } else if (ipl == SE_SUN) {
    /* Upstream bug 167: the Sun's geocentric points are the Earth's
     * orbit seen from the Sun, negated below, so the observer is the Sun:
     * the barycentric Sun for the files' points, made barycentric above,
     * but on Moshier, whose points stay heliocentric, it was the Earth,
     * as for a planet: the mean perigee at 2000 Jul 1.5 281.4259 deg at
     * 1.9993 AU, the apogee 191.4583 deg at 0.0531 AU, where the files'
     * are 282.9417 deg at 0.9833 AU and 102.9417 deg at 1.0167 AU; every
     * method alike, SEFLG_TOPOCTR the site added. On Moshier it is now the
     * Sun, the origin: the files' points to Moshier's precision, 1.8e-6
     * deg for the mean perigee (the files' observer moving with the Sun's
     * barycentric velocity, aberration that Moshier's lacks). The site,
     * if any, is the Earth's below (bug 190). */
    for (i = 0; i <= 5; i++) {
      xsite[i] = xobs[i];
      xobs[i] = mosh_ctr ? 0 : xsun[i];
    }
#else
  } else if (ipl == SE_SUN && !(iflag & SEFLG_MOSEPH)) {
    for (i = 0; i <= 5; i++)
      xobs[i] = xsun[i];
#endif
  } else {
    /* barycentric position of observer */
#ifndef SWE_UPSTREAM_COMPAT
    if (!moon_geo_mean)	/* bug 174: the points stay geocentric */
#endif
    for (i = 0; i <= 5; i++)
      xobs[i] += xear[i];
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* bug 221: that frame only where a vector of it is added or taken away,
   * not where nothing is and the trip would only round: the Moon's mean
   * geocentric points, which read no file (bug 174) and so must not
   * depend on a DE number, Moshier's heliocentric points, a barycentric
   * ellipse's barycentric ones */
  if (ctr_bias) {
    AS_BOOL adds = ipli == SE_MOON ? !moon_geo_mean : (!mosh_ctr && !ellipse_is_bary);
    for (i = 0; i <= 5 && !adds; i++)
      adds = xobs[i] != 0 || xsite[i] != 0;
    ctr_bias = adds;
  }
#endif
  /* ecliptic obliqity */
  if (iflag & SEFLG_J2000)
    oe = &ctx->oec2000;
  else
    oe = &ctx->oec;
  /*************************************************
   * conversions shared by mean and osculating points
   *************************************************/
  for (ij = 0, xp = xx; ij < 4; ij++, xp += 6) {
    /* no nodes for earth */
    if (ipli == SE_EARTH && ij <= 1) {
      for (i = 0; i <= 5; i++)
    	xp[i] = 0;
      continue;
    }
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 37, the sibling of 33: the points were built in the
     * ecliptic of date -- the osculating ones in the true one by
     * swi_plan_for_osc_elem() and differenced there, the mean ones from
     * elements of the mean one -- so their speeds carry its turning, the
     * obliquity's rate, about the HELIOCENTRIC point. Nothing took it out
     * on the way to the equator, and nothing put it back below on the way
     * into the ecliptic again, on the GEOCENTRIC point; what survived was
     * de/dt x (the Earth's heliocentric position), up to 1.6e-5 deg/day on
     * the latitude and declination speeds, and nothing heliocentric or for
     * the Moon. Taken out here, added back below. Under SEFLG_J2000 the
     * whole frame's turning is taken in nodaps_ecl_to_j2000() (bug 47). */
    if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_J2000))
      swi_ecl_frame_speed(xp, -swi_obliquity_rate(ctx, tjd_et, iflag,
                              is_true_nodaps && !(iflag & SEFLG_NONUT)));
    /* Upstream bug 47: under SEFLG_J2000 the points, in the ecliptic of
     * date, were turned to the equator by the obliquity of J2000 (oe is
     * the output's), and without SEFLG_NONUT, which swe_calc() sets for
     * J2000, the nutation was taken out on the way in and applied to the
     * J2000 coordinates on the way out: the Moon's mean node was 7.8" in
     * longitude and 6.9" in latitude from swe_calc(SE_MEAN_NODE, J2000)
     * on 2026-10-24, 148" in latitude in 2326. Now the mean ecliptic of
     * date, its own obliquity, and the frame's turning on the velocity,
     * which bug 37's correction left out here and swi_precess_speed()'s
     * precession in longitude about the J2000 ecliptic does not give:
     * the derivative of the whole rotation. */
    if (iflag & SEFLG_J2000) {
      nodaps_ecl_to_j2000(ctx, xp, tjd_et, iflag);
    } else {
#endif
    /*********************
     * to equator 
     *********************/
    if (is_true_nodaps && !(iflag & SEFLG_NONUT)) {
      swi_coortrf2(xp, xp, -ctx->nut.snut, ctx->nut.cnut);
      if (iflag & SEFLG_SPEED)
        swi_coortrf2(xp+3, xp+3, -ctx->nut.snut, ctx->nut.cnut);
    }
    swi_coortrf2(xp, xp, -oe->seps, oe->ceps);
    swi_coortrf2(xp+3, xp+3, -oe->seps, oe->ceps);
    if (is_true_nodaps) {
      /****************************
       * to mean ecliptic of date 
       ****************************/
      if (!(iflag & SEFLG_NONUT))
	swi_nutate(ctx, xp, iflag, TRUE);
    }
    /*********************
     * to J2000 
     *********************/
    swi_precess(ctx, xp, tjd_et, iflag, J_TO_J2000);
    if (iflag & SEFLG_SPEED)
      swi_precess_speed(ctx, xp, tjd_et, iflag, J_TO_J2000);
#ifndef SWE_UPSTREAM_COMPAT
    }
    /* bug 221: into the frame of the Sun, the Earth and the site */
    if (ctr_bias)
      swi_bias(ctx, xp, tjd_et, iflag, TRUE);
#endif
//      fprintf(stderr, "%.17f, %.17f, %.17f, %.17f, %.17f, %.17f\n", xp[0], xp[1], xp[2], xp[3], xp[4], xp[5]);
    /*********************
     * to barycenter 
     *********************/
    if (ipli == SE_MOON) {
#ifndef SWE_UPSTREAM_COMPAT
      if (!moon_geo_mean)	/* bug 174 */
#endif
      for (i = 0; i <= 5; i++)
        xp[i] += xear[i];
    } else {
#ifndef SWE_UPSTREAM_COMPAT
      if (!mosh_ctr && !ellipse_is_bary)	/* bug 168 */
#else
      if (!(iflag & SEFLG_MOSEPH) && !ellipse_is_bary)
#endif
        for (j = 0; j <= 5; j++)
          xp[j] += xsun[j];
    }
    /*********************
     * to correct center 
     *********************/
    for (j = 0; j <= 5; j++)
      xp[j] -= xobs[j];
        /* geocentric perigee/apogee of sun */
    if (ipl == SE_SUN && !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR)))
      for (j = 0; j <= 5; j++)
        xp[j] = -xp[j];
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 190: the Sun's topocentric points were its geocentric
     * ones. The site was set as the observer and then overwritten by the
     * Sun, the observer of the Earth's orbit (bug 167), so they carried no
     * parallax, where swe_calc()'s topocentric Sun at J2000 is 2.3e-3 deg
     * from its geocentric one in latitude. The site is the Earth's: the
     * Sun seen from it with the Earth at the point is the point negated
     * less the site, as a planet's topocentric points are the geocentric
     * ones less it, its velocity with it. The aberration's observer stays
     * the Sun, as for the geocentric points: they carry no annual
     * aberration, and so no diurnal one. */
    if (ipl == SE_SUN && (iflag & SEFLG_TOPOCTR))
      for (j = 0; j <= 5; j++)
        xp[j] -= xsite[j];
#endif
    /*********************
     * light deflection 
     *********************/
    dt = sqrt(square_sum(xp)) * AUNIT / CLIGHT / 86400.0;     
    if (do_defl)
      swi_deflect_light(ctx, xp, dt, iflag);
    /*********************
     * aberration 
     *********************/
    if (do_aberr) {
#ifndef SWE_UPSTREAM_COMPAT
      /* the point before aberration, for its rate (bug 70); the position
       * is swi_aberr_light()'s as before */
      double xab0[6];
      for (i = 0; i <= 5; i++)
        xab0[i] = xp[i];
      swi_aberr_light(xp, xobs, iflag & ~SEFLG_SPEED);
#else
      swi_aberr_light(xp, xobs, iflag);
#endif
      /* 
       * Apparent speed is also influenced by
       * the difference of speed of the earth between t and t-dt. 
       * Neglecting this would result in an error of several 0.1"
       */
      if (iflag & SEFLG_SPEED) {
#ifndef SWE_UPSTREAM_COMPAT
        /* bug 34 here too: for a point seen from the Earth or a site, the
         * observer's velocity now less at t - dt, whole, was a chord for a
         * turning site, radial too and first order only (7e-6 deg/day on
         * Mercury's points topocentric, 3e-4 on Pluto's); bug 70 takes
         * the observer's acceleration from swi_observer_accel_lt(), with
         * the Earth at t - dt from the call below and the save area at tjd
         * again after it. */
        double xe1[6];
        AS_BOOL rate34 = !(iflag & (SEFLG_HELCTR | SEFLG_BARYCTR)) && ipl != SE_SUN;
#endif
        /* get barycentric sun and earth for t-dt into save area */
        if (swe_calc_r(ctx, tjd_et - dt, ipli, iflg0 | (iflag & SEFLG_TOPOCTR), x2, serr) == ERR)
          return ERR;
        if (iflag & SEFLG_TOPOCTR) {
          /* geocentric position of observer */
          /* if (swi_get_observer(ctx, tjd_et - dt, iflag, FALSE, xobs, serr) != OK)
            return ERR;*/
          for (i = 0; i <= 5; i++)
            xobs2[i] = ctx->topd.xobs[i];
        } else {
          for (i = 0; i <= 5; i++)
            xobs2[i] = 0;
        }
        if (iflag & (SEFLG_HELCTR | SEFLG_BARYCTR)) {
#ifndef SWE_UPSTREAM_COMPAT
          if ((iflag & SEFLG_HELCTR) && !mosh_ctr)	/* bug 168 */
#else
          if ((iflag & SEFLG_HELCTR) && !(iflag & SEFLG_MOSEPH))
#endif
            for (i = 0; i <= 5; i++)
              xobs2[i] = xsun[i];
#ifndef SWE_UPSTREAM_COMPAT
        } else if (ipl == SE_SUN) {
          /* bug 167: the Sun, Moshier's origin, as above */
          for (i = 0; i <= 5; i++)
            xobs2[i] = mosh_ctr ? 0 : xsun[i];
#else
        } else if (ipl == SE_SUN && !(iflag & SEFLG_MOSEPH)) {
          for (i = 0; i <= 5; i++)
            xobs2[i] = xsun[i];
#endif
        } else {
          /* barycentric position of observer */
          for (i = 0; i <= 5; i++)
            xobs2[i] += xear[i];
        }
#ifndef SWE_UPSTREAM_COMPAT
        /* the Earth at t - dt, for bug 70's acceleration below */
        for (i = 0; i <= 5; i++)
          xe1[i] = xear[i];
#else
        for (i = 3; i <= 5; i++) 
          xp[i] += xobs[i] - xobs2[i];
#endif
        /* The above call of swe_calc() has destroyed the
         * parts of the save area 
         * (i.e. bary sun, earth nutation matrix!). 
         * to restore it:
         */
#ifndef SWE_UPSTREAM_COMPAT
        /* Upstream bug 46: it did not. The call was for the Sun with
         * iflg0, which carries SEFLG_HELCTR for every body but the Moon,
         * and a heliocentric Sun is zeros, returned before anything is
         * computed; for the Moon, whose iflg0 is geocentric, it found the
         * Sun of the setup call above in swe_calc()'s save area and
         * returned that. Either way the barycentric Sun and the Earth stayed
         * at tjd - dt for the deflection and the change of centre of every
         * later point: with SEFLG_SPEED, Mars's descending node, perihelion
         * and aphelion were 6-35 mas from where they are without it, the
         * ascending node, first, was right. Now the setup call is repeated,
         * forced, so that the save area is what the first point saw. */
        swi_force_app_pos_etc(ctx);
        if (ipli == SE_MOON && (iflag & (SEFLG_HELCTR | SEFLG_BARYCTR))) {
          if (swe_calc_r(ctx, tjd_et, SE_SUN, iflg0, x2, serr) == ERR)
            return ERR;
        } else if (swe_calc_r(ctx, tjd_et, ipli, iflg0 | (ipli == SE_EARTH ? 0 :
                   (iflag & SEFLG_TOPOCTR)), x2, serr) == ERR) {
          return ERR;
        }
#else
        if (swe_calc_r(ctx, tjd_et, SE_SUN, iflg0 | (iflag & SEFLG_TOPOCTR), x2, serr) == ERR)
          return ERR;
#endif
#ifndef SWE_UPSTREAM_COMPAT
        /* Upstream bug 70: the aberration's rate was swi_aberr_light()'s
         * (the point moved, the observer's velocity held) plus bug 34's
         * tau a, a's part across the line of sight -- first order in v/c,
         * its series for a missing the Moon's jerk (taken before the save
         * area was back at tjd), and, seen from the Sun (heliocentric, the
         * Sun's points), the raw xobs - xobs2, radial part and all.
         * Against the rate of the positions: 5.7e-9 deg/day on Mars's
         * points' geocentric longitude, 1.6e-8 on Neptune's, 1.3e-9 on its
         * latitude; heliocentric distance 1.3e-9 AU/day for Uranus, 2.1e-9
         * for Neptune (2e-11 with SEFLG_NOABERR). Now the central
         * difference of the aberrated place, A(x + s x', V + s a) at
         * s = +-2^-6 day: x the point with its velocity, V the observer's
         * velocity, a its acceleration -- swi_observer_accel_lt()'s, with
         * the Moon in the save area again, or the Sun's V(t) - V(t - dt)
         * over dt. The points carry no light time, so that is the
         * derivative of what they return; the positions are unchanged. */
        {
          const double hab = 1.0 / 64;
          double acc[3], xa[2][6], oa[6], d[2][3];
          int sg;
          if (rate34) {
            if (swi_observer_accel_lt(ctx, tjd_et, tjd_et - dt, iflag_ctr, xear, xsun, xe1, acc, serr) != OK)
              return ERR;
          } else {
            for (i = 0; i <= 2; i++)
              acc[i] = dt > 0 ? (xobs[i+3] - xobs2[i+3]) / dt : 0;
          }
          for (sg = 0; sg < 2; sg++) {
            double s1 = sg ? hab : -hab;
            for (i = 0; i <= 2; i++) {
              xa[sg][i] = xab0[i] + s1 * xab0[i+3];
              xa[sg][i+3] = 0;
              oa[i] = xobs[i];
              oa[i+3] = xobs[i+3] + s1 * acc[i];
              d[sg][i] = xa[sg][i];
            }
            swi_aberr_light(xa[sg], oa, iflag & ~SEFLG_SPEED);
            for (i = 0; i <= 2; i++)
              d[sg][i] = xa[sg][i] - d[sg][i];
          }
          for (i = 0; i <= 2; i++)
            xp[i+3] = xab0[i+3] + (d[1][i] - d[0][i]) / (2 * hab);
        }
#endif
      }
    }
#ifndef SWE_UPSTREAM_COMPAT
    /* bug 221: and back to J2000 */
    if (ctr_bias)
      swi_bias(ctx, xp, tjd_et, iflag, FALSE);
#endif
    /*********************
     * precession
     *********************/
    /* save J2000 coordinates; required for sidereal positions */
    for (j = 0; j <= 5; j++)
      x2000[j] = xp[j];
    if (!(iflag & SEFLG_J2000)) {
      swi_precess(ctx, xp, tjd_et, iflag, J2000_TO_J);
      if (iflag & SEFLG_SPEED)
        swi_precess_speed(ctx, xp, tjd_et, iflag, J2000_TO_J);
    }
    /*********************
     * nutation           
     *********************/
    if (!(iflag & SEFLG_NONUT))
      swi_nutate(ctx, xp, iflag, FALSE);
    /* now we have equatorial cartesian coordinates; keep them */
    for (j = 0; j <= 5; j++)
      pldat.xreturn[18+j] = xp[j];
    /************************************************
     * transformation to ecliptic.                  *
     * with sidereal calc. this will be overwritten *
     * afterwards.                                  *
     ************************************************/
    swi_coortrf2(xp, xp, oe->seps, oe->ceps);
    if (iflag & SEFLG_SPEED)
      swi_coortrf2(xp+3, xp+3, oe->seps, oe->ceps);
    if (!(iflag & SEFLG_NONUT)) {
      swi_coortrf2(xp, xp, ctx->nut.snut, ctx->nut.cnut);
      if (iflag & SEFLG_SPEED)
        swi_coortrf2(xp+3, xp+3, ctx->nut.snut, ctx->nut.cnut);
    }
#ifndef SWE_UPSTREAM_COMPAT
    /* into the ecliptic of date, which turns (bugs 33, 37); J2000's does
     * not (bug 47) */
    if ((iflag & SEFLG_SPEED) && !(iflag & SEFLG_J2000))
      swi_ecl_frame_speed(xp, swi_obliquity_rate(ctx, tjd_et, iflag,
                                                 !(iflag & SEFLG_NONUT)));
#endif
    /* now we have ecliptic cartesian coordinates */
    for (j = 0; j <= 5; j++)
      pldat.xreturn[6+j] = xp[j];
    /************************************
     * sidereal positions               *
     ************************************/
    if (iflag & SEFLG_SIDEREAL) {
      /* project onto ecliptic t0 */
      if (ctx->sidd.sid_mode & SE_SIDBIT_ECL_T0) {
        if (swi_trop_ra2sid_lon(ctx, x2000, pldat.xreturn+6, pldat.xreturn+18, iflag) != OK)
          return ERR;
      /* project onto solar system equator */
      } else if (ctx->sidd.sid_mode & SE_SIDBIT_SSY_PLANE) {
        if (swi_trop_ra2sid_lon_sosy(ctx, x2000, pldat.xreturn+6, iflag) != OK)
          return ERR;
      } else {
      /* traditional algorithm */
        swi_cartpol_sp(pldat.xreturn+6, pldat.xreturn); 
	if (swi_get_ayanamsa_ex(ctx, tjd_et, iflag, &daya, serr) == ERR)
	  return ERR;
        pldat.xreturn[0] -= daya * DEGTORAD;
        swi_polcart_sp(pldat.xreturn, pldat.xreturn+6); 
      }
    } 
    if ((iflag & SEFLG_XYZ) && (iflag & SEFLG_EQUATORIAL)) {
      for (j = 0; j <= 5; j++)
        xp[j] = pldat.xreturn[18+j];
      continue;
    }
    if (iflag & SEFLG_XYZ) {
      for (j = 0; j <= 5; j++)
        xp[j] = pldat.xreturn[6+j];
      continue;
    }
    /************************************************
     * transformation to polar coordinates          *
     ************************************************/
    swi_cartpol_sp(pldat.xreturn+18, pldat.xreturn+12); 
    swi_cartpol_sp(pldat.xreturn+6, pldat.xreturn); 
    /********************** 
     * radians to degrees *
     **********************/
    if (!(iflag & SEFLG_RADIANS)) {
      for (j = 0; j < 2; j++) {
	pldat.xreturn[j] *= RADTODEG;		/* ecliptic */
	pldat.xreturn[j+3] *= RADTODEG;
	pldat.xreturn[j+12] *= RADTODEG;	/* equator */
	pldat.xreturn[j+15] *= RADTODEG;
      }
    }
    if (iflag & SEFLG_EQUATORIAL) {
      for (j = 0; j <= 5; j++)
        xp[j] = pldat.xreturn[12+j];
      continue;
    } else {
      for (j = 0; j <= 5; j++)
        xp[j] = pldat.xreturn[j];
      continue;
    }
  }
  for (i = 0; i <= 5; i++) {
    if (i > 2 && !(iflag & SEFLG_SPEED))
      xna[i] = xnd[i] = xpe[i] = xap[i] = 0;
    if (xnasc != NULL)
      xnasc[i] = xna[i];
    if (xndsc != NULL)
      xndsc[i] = xnd[i];
    if (xperi != NULL)
      xperi[i] = xpe[i];
    if (xaphe != NULL)
      xaphe[i] = xap[i];
  }
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 170: the ephemeris that answered, the less precise where
   * the reads differ (the osculating ellipse's, the save area's), as
   * swe_calc_pctr() returns it (bug 150): the Moon's, from Moshier with
   * the Moon files alone missing, SEFLG_MOSEPH, as swe_calc() flags it */
  if (ephret != NULL) {
    int32 e = (retc0 | retcs) & SEFLG_EPHMASK;
    *ephret = (e & SEFLG_MOSEPH) ? SEFLG_MOSEPH : (e & SEFLG_SWIEPH) ? SEFLG_SWIEPH : e;
  }
#else
  (void) ephret;
#endif
  return OK;
}

int32 CALL_CONV swe_nod_aps_r(swe_ctx *ctx, double tjd_et, int32 ipl, int32 iflag,
                      int32  method,
                      double *xnasc, double *xndsc,
                      double *xperi, double *xaphe,
                      char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_nod_aps_r_body(ctx, tjd_et, ipl, iflag, method, xnasc, xndsc, xperi, xaphe, serr, NULL);
#ifndef SWE_UPSTREAM_COMPAT
  nodaps_moshmoon_end(ctx);	/* bug 192 */
  /* Upstream bug 179: a refusal by a calculation inside (the strict
   * setting's, a file's, a range's) returned -1 and left the four points as
   * the caller had them, where swe_calc() zeroes a refused position (the
   * manual: "the position and speed variables are set to zero") and this
   * function zeroed an unimplemented body's and, since bug 149, a
   * barycentric Moshier request's. Every refusal zeroes them now. */
  if (ret == ERR)
    nodaps_refuse(xnasc, xndsc, xperi, xaphe);
#endif
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_nod_aps(double tjd_et, int32 ipl, int32 iflag, 
                      int32  method,
                      double *xnasc, double *xndsc, 
                      double *xperi, double *xaphe, 
                      char *serr)
{
  return swe_nod_aps_r(swi_default_ctx(), tjd_et, ipl, iflag, method, xnasc, xndsc, xperi, xaphe, serr);
}

static int32 swe_nod_aps_ut_r_body(swe_ctx *ctx, double tjd_ut, int32 ipl, int32 iflag,
                      int32  method,
                      double *xnasc, double *xndsc, 
                      double *xperi, double *xaphe, 
                      char *serr)
{
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 170: delta t was the asked ephemeris's, taken from the
   * bits as given. Fallen to Moshier (the fallback allowed), the points
   * kept the files' delta t, as swe_calc_ut() does not: at 1900 Jan 0.75
   * -1.9915 s for Moshier's -2.0521 s, Mars's osculating perihelion 2.8e-7
   * deg off the Moshier request's, the Moon's 1.7e-6. And
   * swe_deltat_ex() takes SEFLG_SWIEPH with SEFLG_JPLEPH for the Swiss
   * files' tidal term: JPLEPH | SWIEPH on de200 took DE431's for DE200's,
   * the Moon's topocentric osculating points 3.6e-5 deg off the JPLEPH
   * request's. Now as swe_calc_ut(): delta t of the ephemeris the bits
   * resolve to (bug 168; Swiss without one), and, if another answered, of
   * that one, the points computed again (its message kept, as there). */
  double deltat;
  int32 retval, epheflag, ephused = 0;
  epheflag = (iflag & SEFLG_JPLEPH) ? SEFLG_JPLEPH : (iflag & SEFLG_SWIEPH) ? SEFLG_SWIEPH
             : (iflag & SEFLG_MOSEPH) ? SEFLG_MOSEPH : SEFLG_SWIEPH;
  iflag = (iflag & ~SEFLG_EPHMASK) | epheflag;
  deltat = swe_deltat_ex_r(ctx, tjd_ut, iflag, serr);
  retval = swe_nod_aps_r_body(ctx, tjd_ut + deltat, ipl, iflag, method, xnasc, xndsc,
                              xperi, xaphe, serr, &ephused);
  if (retval != ERR && ephused != 0 && ephused != epheflag) {
    deltat = swe_deltat_ex_r(ctx, tjd_ut, (iflag & ~SEFLG_EPHMASK) | ephused, NULL);
    retval = swe_nod_aps_r_body(ctx, tjd_ut + deltat, ipl, iflag, method, xnasc, xndsc,
                                xperi, xaphe, NULL, &ephused);
  }
  return retval;
#else
  /*swi_set_tid_acc(ctx, tjd_ut, iflag, 0, serr);*/
  return swe_nod_aps_r(ctx, tjd_ut + swe_deltat_ex_r(ctx, tjd_ut, iflag, serr),
                      ipl, iflag, method, xnasc, xndsc, xperi, xaphe,
                      serr);
#endif
}

int32 CALL_CONV swe_nod_aps_ut_r(swe_ctx *ctx, double tjd_ut, int32 ipl, int32 iflag,
                      int32  method,
                      double *xnasc, double *xndsc,
                      double *xperi, double *xaphe,
                      char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_nod_aps_ut_r_body(ctx, tjd_ut, ipl, iflag, method, xnasc, xndsc, xperi, xaphe, serr);
#ifndef SWE_UPSTREAM_COMPAT
  nodaps_moshmoon_end(ctx);	/* bug 192 */
  if (ret == ERR)	/* bug 179, as swe_nod_aps_r() */
    nodaps_refuse(xnasc, xndsc, xperi, xaphe);
#endif
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_nod_aps_ut(double tjd_ut, int32 ipl, int32 iflag, 
                      int32  method,
                      double *xnasc, double *xndsc, 
                      double *xperi, double *xaphe, 
                      char *serr)
{
  return swe_nod_aps_ut_r(swi_default_ctx(), tjd_ut, ipl, iflag, method, xnasc, xndsc, xperi, xaphe, serr);
}

#ifdef TEST_ORBEL_AA
/* Corrections to Gmsm to make orbital elements agree with AA 2011-2013
 * using DE406, or AA 2016 using DE431. Example:
 * https://books.google.ch/books?id=Mc_VuQN_gVsC&pg=PR5&dq=%22astronomical+almanac%22+%22osculating+%22+planets&hl=en&sa=X&ved=0ahUKEwib1-rR_rjLAhUEWSwKHZRQAjYQ6AEIOzAG#v=onepage&q=%22astronomical%20almanac%22%20%22osculating%20%22%20planets&f=false
 * swetest -bj2455600.5 -p8 -orbel -hel -ejplde406.eph -j2000
 * This is only a test feature. Orbital elements in agreement with AA should
 * be calculated using iflag | SEFLG_ORBEL_AA.
 */
static const double Gmsm_factor_AA[] = {
1,		/* Mercury */
0.9999999,	/* Venus */
0.9999941,	/* EMB */
0.9999941,	/* Mars */
0.9999941,	/* Jupiter */
0.9990404,      /* Saturn */
0.9987549, 	/* Uranus */
0.998711, 	/* Neptune */
0.99866025, 	/* Pluto */
};
#endif
#ifndef SWE_UPSTREAM_COMPAT
/* Upstream bug 108: swe_get_orbital_elements()'s GM was the Astronomical
 * Almanac's, as swe_nod_aps()'s was (bug 105): the Moon's G M_earth (1 + 1 /
 * EARTH_MOON_MRAT), 3.3e-8 above the ephemeris's GMB, the Sun's
 * HELGRAVCONST / AUNIT^3, 1.8e-10 below every JPL header's, plmass's
 * planets; and ipl_to_elem[] sent Pluto to Mercury's mass, and in
 * SEFLG_ORBEL_AA's sum Mercury's twice and Pluto's never. SEFLG_ORBEL_AA,
 * the Almanac's sum of the masses inside the orbit, is documented
 * (swephprg 7.2) and kept; the constants are the ephemeris's
 * (swi_gm_sun_planet(), swi_gm_planet(), as bug 105), the ephemeris the
 * one swe_calc() answered with. TEST_ORBEL_AA's factors, a test feature
 * never compiled, are left as they were. */
static int pl_elem(int j)
{
  return j == SE_PLUTO ? 8 : ipl_to_elem[j];
}

static int32 get_gmsm(swe_ctx *ctx, double tjd_et, int32 ipl, int32 iflag, double r, double *gmsm, char *serr)
{
  int j;
  double Gmsm, x[6];
  int32 eph = iflag & SEFLG_EPHMASK;
  int32 iflJ2000p = (iflag & (SEFLG_EPHMASK |SEFLG_HELCTR|SEFLG_BARYCTR))|SEFLG_J2000|SEFLG_TRUEPOS|SEFLG_NONUT;
  if (!(iflJ2000p & (SEFLG_HELCTR|SEFLG_BARYCTR)))
    iflJ2000p |= SEFLG_HELCTR;
  if (ipl == SE_MOON) {
    *gmsm = swi_gm_earth_moon(ctx, eph);
    return OK;
  }
  Gmsm = swi_gm_sun_planet(ctx, eph, -1);
  if ((ipl >= SE_MERCURY && ipl <= SE_PLUTO) || ipl == SE_EARTH) {
    if (iflag & SEFLG_ORBEL_AA) {
      /* the masses of all planets inside the orbit */
      if (ipl == SE_EARTH) {
        Gmsm += swi_gm_planet(ctx, eph, pl_elem(SE_EARTH));
        Gmsm += swi_gm_planet(ctx, eph, pl_elem(SE_VENUS));
        Gmsm += swi_gm_planet(ctx, eph, pl_elem(SE_MERCURY));
      } else {
        for (j = ipl; j >= SE_MERCURY; j--)
          Gmsm += swi_gm_planet(ctx, eph, pl_elem(j));
        if (ipl >= SE_MARS)
          Gmsm += swi_gm_planet(ctx, eph, pl_elem(SE_EARTH));
      }
    } else {
      Gmsm += swi_gm_planet(ctx, eph, pl_elem(ipl));
    }
#ifdef TEST_ORBEL_AA
    if (!(iflag & SEFLG_ORBEL_AA))
      Gmsm /= Gmsm_factor_AA[ipl_to_elem[ipl]];
#endif
  } else if (iflag & SEFLG_ORBEL_AA) {	/* asteroid or fictitious object */
    for (j = SE_MERCURY; j <= SE_PLUTO; j++) {
      if (swe_calc_r(ctx, tjd_et, j, iflJ2000p, x, serr) == ERR)
        return ERR;
      if (r > x[2])
        Gmsm += swi_gm_planet(ctx, eph, pl_elem(j));
    }
    if (swe_calc_r(ctx, tjd_et, SE_EARTH, iflJ2000p, x, serr) == ERR)
      return ERR;
    if (r > x[2])
      Gmsm += swi_gm_planet(ctx, eph, pl_elem(SE_EARTH));
  }
  *gmsm = Gmsm;
  return OK;
}
#else
static int32 get_gmsm(swe_ctx *ctx, double tjd_et, int32 ipl, int32 iflag, double r, double *gmsm, char *serr)
{
  int j;
  double Gmsm = 0, plm = 0, x[6];
  int32 iflJ2000p = (iflag & (SEFLG_EPHMASK |SEFLG_HELCTR|SEFLG_BARYCTR))|SEFLG_J2000|SEFLG_TRUEPOS|SEFLG_NONUT;
  if (!(iflJ2000p & (SEFLG_HELCTR|SEFLG_BARYCTR)))
    iflJ2000p |= SEFLG_HELCTR;
  if (ipl == SE_MOON) {
    Gmsm = GEOGCONST * (1 + 1 / EARTH_MOON_MRAT) /AUNIT/AUNIT/AUNIT*86400.0*86400.0;
  } else {
    if ((ipl >= SE_MERCURY && ipl <= SE_PLUTO) || ipl == SE_EARTH) {
      plm = 0;
      /* to reproduce AA orbital elements, sum up masses of all planets
       * that are inside the orbit to be computed */
      if (iflag & SEFLG_ORBEL_AA) {
	if (ipl == SE_EARTH) {
	  plm = 1.0 / plmass[ipl_to_elem[ipl]];
	  plm += 1.0 / plmass[ipl_to_elem[SE_VENUS]];
	  plm += 1.0 / plmass[ipl_to_elem[SE_MERCURY]];
	} else {
	  for (j = ipl; j >= SE_MERCURY; j--) {
	    plm += 1.0 / plmass[ipl_to_elem[j]];
	  }
	  if (ipl >= SE_MARS)
	    plm += 1.0 / plmass[ipl_to_elem[SE_EARTH]];
	}
      /* ... treat it as a pure two-body problem */
      } else {
	plm = 1.0 / plmass[ipl_to_elem[ipl]];
      }
      Gmsm = HELGRAVCONST * (1 + plm) /AUNIT/AUNIT/AUNIT*86400.0*86400.0;
#ifdef TEST_ORBEL_AA
      if (!(iflag & SEFLG_ORBEL_AA))
	Gmsm /= Gmsm_factor_AA[ipl_to_elem[ipl]];
#endif
    // asteroid or fictitious object
    } else {
      plm = 0;
      if (iflag & SEFLG_ORBEL_AA) {
	for (j = SE_MERCURY; j <= SE_PLUTO; j++) {
	  if (swe_calc_r(ctx, tjd_et, j, iflJ2000p, x, serr) == ERR)
	    return ERR;
	  if (r > x[2])
	    plm += 1.0 / plmass[ipl_to_elem[j]];
	}
	if (swe_calc_r(ctx, tjd_et, SE_EARTH, iflJ2000p, x, serr) == ERR)
	  return ERR;
	if (r > x[2])
	  plm += 1.0 / plmass[ipl_to_elem[SE_EARTH]];
      }
      Gmsm = HELGRAVCONST * (1 + plm) /AUNIT/AUNIT/AUNIT*86400.0*86400.0;
    }
  }
  *gmsm = Gmsm;
  return OK;
}
#endif

/* Function calculates osculating orbital elements (Kepler elements) of a planet 
 * or asteroid or the Earth-Moon barycentre. 
 * The function returns error if called for the Sun, the lunar nodes, or the apsides.
 * Input parameters:
 * tjd_et	Julian day number, in TT (ET)
 * ipl		object number
 * iflag	can contain 
 *              - ephemeris flag: SEFLG_JPLEPH, SEFLG_SWIEPH, SEFLG_MOSEPH
 * 		- center: 
 * 		  Sun:            SEFLG_HELCTR (assumed as default) or
 * 		  SS Barycentre:  SEFLG_BARYCTR (rel. to solar system barycentre)
 * 		                  (only possible for planets beyond Jupiter)
 *                For elements of the Moon, the calculation is geocentric.
 *              - sum all masses inside the orbit to be computed (method
 *                of Astronomical Almanac):
 *                                SEFLG_ORBEL_AA
 *              - reference ecliptic: SEFLG_J2000;
 * 		  if missing, mean ecliptic of date is chosen (still not implemented)
 * output parameters:
 * dret[]       array of return values, declare as dret[50]
 * dret[0]      semimajor axis (a)
 * dret[1]      eccentricity (e)
 * dret[2]      inclination (in)
 * dret[3]      longitude of ascending node (upper case omega OM)
 * dret[4]      argument of periapsis (lower case omega om)
 * dret[5]      longitude of periapsis (peri) 
 * dret[6]      mean anomaly at epoch (M0) 
 * dret[7]      true anomaly at epoch (N0) 
 * dret[8]      eccentric anomaly at epoch (E0) 
 * dret[9]      mean longitude at epoch (LM) 
 * dret[10]     sidereal orbital period in tropical years
 * dret[11]     mean daily motion
 * dret[12]     tropical period in years
 * dret[13]     synodic period in days,
 *              negative, if inner planet (Venus, Mercury, Aten asteroids) or Moon
 * dret[14]     time of perihelion passage
 * dret[15]     perihelion distance
 * dret[16]     aphelion distance
*/
static int32 swe_get_orbital_elements_r_body(swe_ctx *ctx, double tjd_et,
  int32 ipl, int32 iflag, 
  double *dret,
  char *serr)
{
  int j;
  double x[6], xpos[6], xposm[6], xn[6], xs[6], xnorm[6], xq[6], xa[6] ;
  //int32 iflJ2000 = (iflag & SEFLG_EPHMASK)|SEFLG_J2000|SEFLG_EQUATORIAL|SEFLG_XYZ|SEFLG_TRUEPOS|SEFLG_NONUT|SEFLG_SPEED;
  int32 iflJ2000 = (iflag & SEFLG_EPHMASK)|SEFLG_J2000|SEFLG_XYZ|SEFLG_TRUEPOS|SEFLG_NONUT|SEFLG_SPEED;
  int32 iflJ2000p = (iflag & SEFLG_EPHMASK)|SEFLG_J2000|SEFLG_TRUEPOS|SEFLG_NONUT|SEFLG_SPEED;
  double Gmsm;
  // int32 iflg0 = 0;
  double fac, sgn, rxy, rxyz, c2, cosnode, sinnode;
  double incl, node, parg, peri, mlon;
  double csid, ctro, csyn, dmot, pa;
  double T, T2, T3, T4, T5;
  double sinincl, cosincl, cosu, sinu, uu, eanom, tanom, manom;
  double v2, sema, ecce, cosE, ny, ny2, rn, rn2, ro, ro2, cosE2;
  double r;
#ifndef SWE_UPSTREAM_COMPAT
  int32 retc0;
  double ev[3], evr[3], rv;
#else
  double pp, sinE, ecce2, ytrop, ysid;
#endif
  if (ipl <= 0 || ipl == SE_MEAN_NODE || ipl == SE_TRUE_NODE || ipl == SE_MEAN_APOG || ipl == SE_OSCU_APOG || ipl == SE_INTP_APOG || ipl == SE_INTP_PERG) {
    if (serr != NULL)
      sprintf(serr, "error in swe_get_orbital_elements(): object %d not valid\n", ipl);
    return ERR;
  }
  // if (ipl != SE_MOON) iflg0 |= SEFLG_HELCTR;
  /* first, we need a heliocentric distance of the planet */
#ifndef SWE_UPSTREAM_COMPAT
  if ((retc0 = swe_calc_r(ctx, tjd_et, ipl, iflJ2000p, x, serr)) == ERR)
    return ERR;
  /* bug 108: the GM of the ephemeris swe_calc() answered with */
  iflag = (iflag & ~SEFLG_EPHMASK) | (retc0 & SEFLG_EPHMASK);
#else
  if (swe_calc_r(ctx, tjd_et, ipl, iflJ2000p, x, serr) == ERR)
    return ERR;
#endif
  r =  x[2];
  if (ipl != SE_MOON) {
    if ((iflag & SEFLG_BARYCTR) && r > 6) {
      iflJ2000 |= SEFLG_BARYCTR; /* only planets beyond Jupiter */
    } else {
      iflJ2000 |= SEFLG_HELCTR;
    }
  }
  if (get_gmsm(ctx, tjd_et, ipl, iflag, r, &Gmsm, serr))
    return ERR;
  if (swe_calc_r(ctx, tjd_et, ipl, iflJ2000, xpos, serr) == ERR)
    return ERR;
  /* the EMB is used instead of the earth */
  if (ipl == SE_EARTH) {
    if (swe_calc_r(ctx, tjd_et, SE_MOON, iflJ2000 & ~(SEFLG_BARYCTR|SEFLG_HELCTR), xposm, serr) == ERR)
      return ERR;
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 189: on Moshier the Earth was its series' EMB less the
     * short lunar series' Moon (embofs_mosh(); bug 191 made it the full
     * series', the Moon's own), and the EMB here was that
     * Earth plus Moshier's own Moon, 0.24 deg away at 2000, 1.55 deg at
     * -3000, as in swe_nod_aps() (bug 180): the longitude of perihelion
     * wobbled 8.0e-3 deg about the series' EMB's at 2000, 4.1e-2 at -3000,
     * the perihelion distance 3.8e-6 and 3.9e-5 AU, and
     * swe_orbit_max_min_true_distance() with it. Now the Moon the Earth was
     * made with (ctx->earth_moon, ICRS equatorial), given the frame bias
     * swe_calc() gives Moshier (DE403's) and turned into the J2000
     * ecliptic of the places here, so the EMB is the series' own. */
    if ((iflag & SEFLG_EPHMASK) == SEFLG_MOSEPH && ctx->earth_moon.t == tjd_et
        && ctx->earth_moon.iephe == SEFLG_MOSEPH
        && ctx->pldat[SEI_EARTH].teval == tjd_et && ctx->pldat[SEI_EARTH].iephe == SEFLG_MOSEPH) {
      for (j = 0; j <= 5; j++)
        xposm[j] = ctx->earth_moon.x[j];
      swi_bias(ctx, xposm, tjd_et, iflJ2000, FALSE);
      swi_coortrf2(xposm, xposm, ctx->oec2000.seps, ctx->oec2000.ceps);
      swi_coortrf2(xposm + 3, xposm + 3, ctx->oec2000.seps, ctx->oec2000.ceps);
    }
    /* bug 108: with the ratio the Earth was made from the EMB with */
    for (j = 0; j <= 5; j++)
      xpos[j] += xposm[j] / (swi_emb_mrat(ctx, iflag & SEFLG_EPHMASK) + 1.0);
#else
    for (j = 0; j <= 5; j++)
      xpos[j] += xposm[j] / (EARTH_MOON_MRAT + 1.0);
#endif
  }
  fac = xpos[2] / xpos[5];
  sgn = xpos[5] / fabs(xpos[5]);
  for (j = 0; j <= 2; j++) {
    xn[j] = (xpos[j] - fac * xpos[j+3]) * sgn;
    xs[j] = -xn[j];
  }
  /* node */
  rxy =  sqrt(xn[0] * xn[0] + xn[1] * xn[1]);
  cosnode = xn[0] / rxy;	
  sinnode = xn[1] / rxy;
  /* inclination */
  swi_cross_prod(xpos, xpos+3, xnorm);
  rxy =  xnorm[0] * xnorm[0] + xnorm[1] * xnorm[1];
  c2 = (rxy + xnorm[2] * xnorm[2]);
  rxyz = sqrt(c2);
  rxy = sqrt(rxy);
  sinincl = rxy / rxyz;
  cosincl = sqrt(1 - sinincl * sinincl);
  if (xnorm[2] < 0) cosincl = -cosincl; /* retrograde asteroid, e.g. 20461 Dioretsa */
  incl = acos(cosincl) * RADTODEG; // inclination
  /* argument of latitude */
  cosu = xpos[0] * cosnode + xpos[1] * sinnode;
  sinu = xpos[2] / sinincl;	
  uu = atan2(sinu, cosu);	
  /* semi-axis */
  rxyz = sqrt(square_sum(xpos));
  v2 = square_sum((xpos+3));
  sema = 1.0 / (2.0 / rxyz - v2 / Gmsm);	
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 111: e was sqrt(1 - p / a), which loses half the digits
   * of a small e (Neptune's 3.6e-14, its q 1.1e-12 AU), and the true
   * anomaly 2 atan(k sin E / (1 + cos E)), where 1 + cos E cancels near
   * aphelion (Venus's perihelion 4.4e-9 deg at E 178 deg). Now e is the
   * length of the eccentricity vector, ((v^2 - GM / r) r - (r.v) v) / GM,
   * as in swe_nod_aps(), the true anomaly the angle from it to r about h
   * (atan2 of (ev x r).h / |h| and ev.r), and E = atan2(sqrt(1 - e^2)
   * sin v, e + cos v): each well conditioned at every anomaly, and the
   * same half-turn without the quadrant fix-ups. */
  rv = dot_prod(xpos, (xpos+3));
  for (j = 0; j <= 2; j++)
    ev[j] = ((v2 - Gmsm / rxyz) * xpos[j] - rv * xpos[j+3]) / Gmsm;
  ecce = sqrt(square_sum(ev));
  swi_cross_prod(ev, xpos, evr);
  ny = atan2(dot_prod(evr, xnorm) / sqrt(c2), dot_prod(ev, xpos));
  eanom = atan2(sqrt(ecce < 1 ? 1 - ecce * ecce : 0) * sin(ny), ecce + cos(ny));
  eanom = swe_degnorm(eanom * RADTODEG);
  tanom = swe_degnorm(ny * RADTODEG);
#else
  /* eccentricity */
  pp = c2 / Gmsm;
  ecce = pp / sema;
  if (ecce > 1)
    ecce = 1;
  ecce = sqrt(1 - ecce);	
  /* eccentric anomaly */
  ecce2 = ecce;
  if (ecce2 == 0)
    ecce2 = 0.0000000001;
  cosE = 1 / ecce2 * (1 - rxyz / sema);	
  sinE = 1 / ecce2 / sqrt(sema * Gmsm) * dot_prod(xpos, (xpos+3));
  eanom = swe_degnorm(atan2(sinE, cosE) * RADTODEG); 
//  eanom = acos((1 - rxyz / sema) / ecce2);
  /* true anomaly */
  ny = 2 * atan(sqrt((1+ecce)/(1-ecce)) * sinE / (1 + cosE));
  tanom = swe_degnorm(ny * RADTODEG);
  if (eanom > 180 && tanom < 180) 
    tanom += 180;
  if (eanom < 180 && tanom > 180) 
    tanom -= 180;
#endif
//  tanom = acos((sema * (1 - ecce * ecce) / rxyz - 1.0) / ecce2);
  /* mean anomaly */
  manom = swe_degnorm(eanom - ecce * RADTODEG * sin(eanom * DEGTORAD)); // mean anomaly
  /* distance of perihelion from ascending node */
  xq[0] = swi_mod2PI(uu - ny);
  parg = xq[0] * RADTODEG;
  xq[1] = 0;			/* latitude */
  xq[2] = sema * (1 - ecce);	/* distance of perihelion */
  /* transformation to ecliptic coordinates */
  swi_polcart(xq, xq);
  swi_coortrf2(xq, xq, -sinincl, cosincl);
  swi_cartpol(xq, xq);
  /* adding node, we get perihelion in ecl. coord. */
  xq[0] += atan2(sinnode, cosnode);
  xa[0] = swi_mod2PI(xq[0] + PI);
  xa[1] = -xq[1];
//  if (do_focal_point) {
//    xa[2] = sema * ecce * 2;	/* distance of aphelion */
//  } else {
    xa[2] = sema * (1 + ecce);	/* distance of aphelion */
//  }
  swi_polcart(xq, xq);
  swi_polcart(xa, xa);
  /* new distance of node from orbital ellipse:
   * true anomaly of node: */
  ny = swi_mod2PI(ny - uu);
  ny2 = swi_mod2PI(ny + PI);
  /* eccentric anomaly */
  cosE = cos(2 * atan(tan(ny / 2) / sqrt((1+ecce) / (1-ecce))));
  cosE2 = cos(2 * atan(tan(ny2 / 2) / sqrt((1+ecce) / (1-ecce))));
  /* new distance */
  rn = sema * (1 - ecce * cosE);
  rn2 = sema * (1 - ecce * cosE2);
  /* old node distance */
  ro = sqrt(square_sum(xn));
  ro2 = sqrt(square_sum(xs));
  /* correct length of position vector */
  for (j = 0; j <= 2; j++) {
    xn[j] *= rn / ro;
    xs[j] *= rn2 / ro2;
  }
  swi_cartpol(xn, xn);
  swi_cartpol(xq, xq);
  node = xn[0] * RADTODEG;
  peri = swe_degnorm(node + parg);
  mlon = swe_degnorm(manom + peri);
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 111: the period was a^1.5 sidereal years (the Moon's
   * scaled to 27.32166 days at a = 383397772.5 m), Kepler's third law
   * without the body's mass, though the elements are the two-body
   * ellipse's with GM = Gmsm (swephprg 7.2): Jupiter's 2.1 days long,
   * Saturn's 1.5, and the time of perihelion passage with it; and the
   * sidereal period (365.25636 days a year) and the daily motion (k,
   * 365.25690 days) disagreed by 1.5e-6. Now the
   * ellipse's own mean motion, sqrt(GM / a^3), with the GM the elements
   * were fitted with (SEFLG_ORBEL_AA's sum included); the tropical and
   * synodic periods and dret[14] follow from it as before. */
  dmot = sqrt(Gmsm / (sema * sema * sema)) * RADTODEG; // daily motion
  csid = 360.0 / dmot / 365.242189; // sidereal period in tropical years J2000
#else
  csid = sema * sqrt(sema);   // sidereal period in sidereal years
  if (ipl == SE_MOON) {
    double semam = sema * AUNIT / 383397772.5;
    csid = semam * sqrt(semam); // sidereal period in sidereal months
    csid *= 27.32166 / 365.25636300;
  }
  dmot = 0.9856076686 / csid; // daily motion
  csid *= 365.25636 / 365.242189; // sidereal period in tropical years J2000
#endif
  // daily motion due to precession (Simon et alii 1994)
  T = (tjd_et - J2000) / 365250.0;
  T2 = T * T; T3 = T2 * T; T4 = T3 * T; T5 = T4 * T;
  pa = (50288.200 + 222.4045 * T + 0.2095 * T2 - 0.9408 * T3 - 0.0090 * T4 + 0.0010 * T5) / 3600.0 / 365250.0;
#ifndef SWE_UPSTREAM_COMPAT
  /* Upstream bug 111: the tropical period, 360 / (n + p) days, in tropical
   * years J2000 was multiplied by ysid / ytrop as well, 3.9e-5 long (a body
   * on the Earth's mean motion 1.0000388 years where it is 1, Pluto's 3.5
   * days); and the
   * synodic period took the Earth's mean motion as 0.9856076686 deg/day,
   * k in degrees, the motion of a massless body at 1 AU, not the
   * sidereal year 365.25636 days the sidereal period is measured in. */
  ctro = 360.0 / (dmot + pa) / 365.242189; // tropical period in tropical years J2000
  if (ipl == SE_EARTH)
    csyn = 0;
  else
    csyn = 360.0 / (360.0 / 365.25636 - dmot); // synodic period in days
#else
  // sidereal and tropical year length (Simon et alii 1994)
  ysid = (1295977422.83429 - 2 * 2.0441 * T  - 3 * 0.00523 * T * T) / 3600.0 / 365250.0;
  ysid = 360.0 / ysid;
  ytrop = (1296027711.03429 + 2 * 109.15809 * T  + 3 * 0.07207 * T2 - 4 * 0.23530 * T3 - 5 * 0.00180 * T4 + 6 * 0.00020 * T5) / 3600.0 / 365250.0;
  ytrop = 360.0 / ytrop;
  ctro = 360.0 / (dmot + pa) / 365.242189; // tropical period in years
  ctro *= ysid / ytrop; // tropical period in tropical years J2000
  if (ipl == SE_EARTH)
    csyn = 0;
  else 
    csyn = 360.0 / (0.9856076686 - dmot); // synodic period in days 
#endif
  dret[0] = sema;  // semimajor axis
  dret[1] = ecce;  // eccentricity
  dret[2] = incl;  // inclination
  dret[3] = node;  // node 
  dret[4] = parg;  // argument of perihelion
  dret[5] = peri;  // longitude of perihelion
  dret[6] = manom; // mean anomaly
  dret[7] = tanom; // true anomaly
  dret[8] = eanom; // eccentric anomaly
  dret[9] = mlon;  // mean longitude
  dret[10] = csid; // sidereal orbital period in sidereal years 
  dret[11] = dmot; // daily motion 
  dret[12] = ctro; // tropical period in years 
  dret[13] = csyn; /* synodic period in days */
  dret[14] = tjd_et - dret[6] / dmot; /* tjd_et of perihelion passage */
  dret[15] = sema * (1 - ecce); /* perihelion distance */
  dret[16] = sema * (1 + ecce); /* aphelion distance */
//  printf("%d: a=%f, e=%f, in=%f, node=%f, parg=%f, peri=%f\n", ipl, sema, ecce, dret[2], dret[3], dret[4], dret[5]);
//  printf("tan=%f, ean=%f, man=%f, mlon=%f\n", dret[7], dret[8], dret[6], dret[9]);
//  printf("cyc=%f, dmot=%f, cyct=%f, cycs=%f\n", dret[10], dret[11], dret[12], dret[13]);
//  printf("tperi=%f, rperi=%f, raph=%f\n", dret[14], dret[15], dret[16]);
  return OK;
}

int32 CALL_CONV swe_get_orbital_elements_r(swe_ctx *ctx, double tjd_et,
  int32 ipl, int32 iflag,
  double *dret,
  char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_get_orbital_elements_r_body(ctx, tjd_et, ipl, iflag, dret, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_get_orbital_elements(
  double tjd_et, 
  int32 ipl, int32 iflag, 
  double *dret,
  char *serr)
{
  return swe_get_orbital_elements_r(swi_default_ctx(), tjd_et, ipl, iflag, dret, serr);
}

static void osc_get_orbit_constants(double *dp, double *pqr)
{
  double sema = dp[0];
  double ecce = dp[1];
  double incl = dp[2];
  double node = dp[3];
  double parg = dp[4];
  double cosnode = cos(node * DEGTORAD);
  double sinnode = sin(node * DEGTORAD);
  double cosincl = cos(incl * DEGTORAD);
  double sinincl = sin(incl * DEGTORAD);
  double cosparg = cos(parg * DEGTORAD);
  double sinparg = sin(parg * DEGTORAD);  
  double fac = sqrt((1 - ecce) * (1 + ecce));
  pqr[0] = cosparg * cosnode - sinparg * cosincl * sinnode;
  pqr[1] = -sinparg * cosnode - cosparg * cosincl * sinnode;
  pqr[2] = sinincl * sinnode;
  pqr[3] = cosparg * sinnode + sinparg * cosincl * cosnode;
  pqr[4] = -sinparg * sinnode + cosparg * cosincl * cosnode;
  pqr[5] = -sinincl * cosnode;
  pqr[6] = sinparg * sinincl;
  pqr[7] = cosparg * sinincl;
  pqr[8] = cosincl;
  pqr[9] = sema;
  pqr[10] = ecce;
  pqr[11] = fac;
}

static void osc_get_ecl_pos(double ean, double *pqr, double *xp)
{
  double x[2];
  double cose = cos(ean * DEGTORAD);
  double sine = sin(ean * DEGTORAD);
  double sema = pqr[9];
  double ecce = pqr[10];
  double fac = pqr[11];
  x[0] = sema * (cose - ecce);
  x[1] = sema * fac * sine;
  /* transformation to ecliptic */
  xp[0] = pqr[0] * x[0] + pqr[1] * x[1];
  xp[1] = pqr[3] * x[0] + pqr[4] * x[1];
  xp[2] = pqr[6] * x[0] + pqr[7] * x[1];
}

static double get_dist_from_2_vectors(double *x1, double *x2)
{
  double r0, r1, r2;
  r0 = x1[0] - x2[0];
  r1 = x1[1] - x2[1];
  r2 = x1[2] - x2[2];
  return sqrt(r0 * r0 + r1 * r1 + r2 * r2);
}

static void osc_iterate_max_dist(double ean, double *pqr, double *xa, double *xb, double *deanopt, double *drmax, AS_BOOL high_prec)
{
  int i;
  double r, rmax, eansv = 0, dstep, dstep_min = 1;
  if (high_prec)
    dstep_min = 0.000001;
  ean = 0;
  osc_get_ecl_pos(ean, pqr, xa);
  r = get_dist_from_2_vectors(xb, xa);
  rmax = r;
  dstep = 1;
  while (dstep >= dstep_min) {
    //printf("r=%.17f, dstep=%f\n", r, dstep);
    for (i = 0; i < 2; i++) {
      while(r >= rmax) {
	eansv = ean;
	if (i == 0) 
	  ean += dstep;
	else
	  ean -= dstep;
	osc_get_ecl_pos(ean, pqr, xa);
	r = get_dist_from_2_vectors(xb, xa);
	if (r > rmax)
	  rmax = r;
      }
      ean = eansv;
      r = rmax;
    }
    ean = eansv;
    r = rmax;
    dstep /= 10;
  }
  *drmax = rmax;
  *deanopt = eansv;
}

static void osc_iterate_min_dist(double ean, double *pqr, double *xa, double *xb, double *deanopt, double *drmin, AS_BOOL high_prec)
{
  int i;
  double r, rmin, eansv = 0, dstep, dstep_min = 1;
  if (high_prec)
    dstep_min = 0.000001;
  ean = 0;
  osc_get_ecl_pos(ean, pqr, xa);
  r = get_dist_from_2_vectors(xb, xa);
  rmin = r;
  dstep = 1;
  while (dstep >= dstep_min) {
    //printf("r=%.17f, dstep=%f\n", r, dstep);
    for (i = 0; i < 2; i++) {
      while(r <= rmin) {
	eansv = ean;
	if (i == 0) 
	  ean += dstep;
	else
	  ean -= dstep;
	osc_get_ecl_pos(ean, pqr, xa);
	r = get_dist_from_2_vectors(xb, xa);
	if (r < rmin)
	  rmin = r;
      }
      ean = eansv;
      r = rmin;
    }
    ean = eansv;
    r = rmin;
    dstep /= 10;
  }
  *drmin = rmin;
  *deanopt = eansv;
}

/* function calculates maximum distance, minimum distance and true distance between
 * the Sun and the Earth-Moon barycentre. Maximum and minimum distance are derived
 * from Kepler elements. */
static int32 orbit_max_min_true_distance_helio(swe_ctx *ctx, double tjd_et, int ipl, int32 iflag, double *dmax, double *dmin, double *dtrue, char *serr)
{
  double xinner[3], pqri[20];
  double eani;
  double de[50];
  int32 retval;
  int32 ipli = ipl;
  int32 iflagi = (iflag & (SEFLG_EPHMASK | SEFLG_HELCTR | SEFLG_BARYCTR));
  if (ipl == SE_SUN) {
    ipli = SE_EARTH;
  }
  /* Kepler elements */
  if ((retval = swe_get_orbital_elements_r(ctx, tjd_et, ipli, iflagi, de, serr)) == ERR)
    return ERR;
  *dmax = de[16];
  *dmin = de[15];
  osc_get_orbit_constants(de, pqri);
  /* true distance */
  eani = de[8];
  /* heliocentric ecliptic position of EMB, cartesian coordinates J2000 */
  osc_get_ecl_pos(eani, pqri, xinner);
  /* its distance */
  *dtrue = sqrt(xinner[0] * xinner[0] + xinner[1] * xinner[1] + xinner[2] * xinner[2]);
#ifdef DEBUG_REL_DIST
  printf("rtrue=%.17f (%.17f, %.17f\n", *dtrue, *dmin, *dmax);
#endif
  return retval;
}

/* This function calculates calculates the maximum possible distance, the
 * minimum possible distance, and the current true distance of planet, the EMB,
 * or an asteroid. The calculation can be done either heliocentrically or
 * geocentrically. With heliocentric calculations, it is based on the momentary
 * Kepler ellipse of the planet. With geocentric calculations, it is based on
 * the Kepler ellipses of the planet and the EMB. The geocentric calculation is
 * rather expensive. 
 *
 * The problem is a bit tricky. The maximum and minimum possible distance of
 * an object from the earth can only be calculated over a limited time range,
 * not over the whole time the solar system exists. Since a scan of the whole
 * available time range is very costly, we should not do that. Alternatively, 
 * one could create a database that provides the minimal and maximal distances
 * for each object. However, the creation and maintenance of such a database 
 * would be expensive, too. In addition, since planetary orbits change over 
 * time, a limited period won't provide a meaningful value.
 *
 * Instead, we determine the maximal and minimal distance from the osculating
 * ellipses of the planet and the Earth-Moon barycentre, assuming that both
 * the planet and the EMB could have any position on its respective ellipse.
 * 
 * Note that instead of the position of the Earth, the position of the EMB
 * is used. Using the true position of the Earth would make the problem 
 * considerably more complicated. Even if this were done, the Swiss Ephemeris
 * still is not able provide the true planets, but ony their barycentres. E.g. 
 * it cannot provide the true position of Jupiter, but only the position of 
 * the barycentre of the Jupiter system. The geocentric difference between 
 * the two is below 0.2 arcsec for all planets.
 *
 * Input:
 * tjd_et       epoch
 * ipl		planet number
 * iflag 	ephemeris flag and optional heliocentrif flag (SEFLG_HELCTR)
 *
 * output:
 * dmax		maximum distance (pointer to double)
 * dmin		minimum distance (pointer to double)
 * dtrue	true distance (pointer to double)
 * serr	        error string
 */
static int32 swe_orbit_max_min_true_distance_r_body(swe_ctx *ctx, double tjd_et, int32 ipl, int32 iflag, double *dmax, double *dmin, double *dtrue, char *serr)
{
  int i, j, k, retval;
  int32 iflagi = (iflag & (SEFLG_EPHMASK | SEFLG_HELCTR | SEFLG_BARYCTR));
  double dp[50], de[50];
  double xouter[3], xinner[3], max_xouter[3], max_xinner[3], min_xouter[3], min_xinner[3], pqro[20], pqri[20];
  double eano, eani;
  double *douter, *dinner;
  double r, rtrue, rmax = 0, rmin = 100000000, rminsv = 0, rmaxsv = 0;
  double min_eanisv = 0, min_eanosv = 0, max_eanisv = 0, max_eanosv = 0;
  int ncnt;
  double dstep;
  double nitermax = 300;
  /* separate handling for the Sun, Moon and heliocentric calculation */
  if (ipl == SE_SUN || ipl == SE_MOON || (iflagi & (SEFLG_HELCTR | SEFLG_BARYCTR))) {
    retval = orbit_max_min_true_distance_helio(ctx, tjd_et, ipl, iflagi, dmax, dmin, dtrue, serr);
    return retval;
  }
  if ((retval = swe_get_orbital_elements_r(ctx, tjd_et, ipl, iflagi, dp, serr)) == ERR)
    return ERR;
  if ((retval = swe_get_orbital_elements_r(ctx, tjd_et, SE_EARTH, iflagi, de, serr)) == ERR)
    return ERR;
  if (de[0] > dp[0]) {
    douter = de;
    dinner = dp;
  } else {
    douter = dp;
    dinner = de;
  }
  osc_get_orbit_constants(douter, pqro);
  osc_get_orbit_constants(dinner, pqri);
  eano = douter[8]; // ecc. anomaly outer planet
  eani = dinner[8]; // ecc. anomaly inner planet
  osc_get_ecl_pos(eano, pqro, xouter); // coordinates outer planet J2000
  osc_get_ecl_pos(eani, pqri, xinner); // coordinates inner planet J2000
  rtrue = get_dist_from_2_vectors(xouter, xinner); // true distance between them
//  printf("rtrue=%.17f\n", rtrue);
  /* search rough maximum and minimum distance for objects on the two ellipses.
   * Attention, there may be two minima or maxima, and we need the smaller 
   * minimum and the greate maximum. 
   * To find minima and maxima, we start with a rough calculation: We move the
   * one planet through the whole orbit in two-degree steps. For each step, 
   * we move the other planet through its own whole orbit at two-degree steps.
   * In vary rare cases we may get the wrong minimum. To avoid that, we would
   * have to make smaller steps, but that would considerably reduce 
   * performance. A faster algorithm without this problem would require 
   * considerably higher sophistication.
   * */
  ncnt = 182;
  dstep = 2;
  for (i = 0; i < 3; i++) { /* initialisation */
    max_xouter[i] = 0;
    max_xinner[i] = 0;
    min_xouter[i] = 0;
    min_xinner[i] = 0;
  }
  for (j = 0; j < ncnt; j++) {
    eano = (double) j * dstep;
    osc_get_ecl_pos(eano, pqro, xouter);
    for (i = 0; i < ncnt; i++) {
      eani = (double) i;
      osc_get_ecl_pos(eani, pqri, xinner);
      r = get_dist_from_2_vectors(xouter, xinner);
      /* maximum/minimum found; save positions and ecc. anomalies */
      if (r > rmax) {
        rmax = r;
        max_eanisv = eani;
        max_eanosv = eano;
	for (k = 0; k < 3; k++) {
	  max_xouter[k] = xouter[k];
	  max_xinner[k] = xinner[k];
	}
      }
      if (r < rmin) {
        rmin = r;
        min_eanisv = eani;
        min_eanosv = eano;
	for (k = 0; k < 3; k++) {
	  min_xouter[k] = xouter[k];
	  min_xinner[k] = xinner[k];
	}
      }
    }
  }
  /* find accurate values, starting iterations from above-calculated rough values; 
   * maximum distance: */
  eani = max_eanisv;
  eano = max_eanosv;
  for (k = 0; k < 3; k++) {
    xouter[k] = max_xouter[k];
    xinner[k] = max_xinner[k];
  }
  for (k = 0; k <= nitermax; k++) {
    osc_iterate_max_dist(eani, pqri, xinner, xouter, &eani, &rmax, TRUE);
    osc_iterate_max_dist(eano, pqro, xouter, xinner, &eano, &rmax, TRUE);
    if (k > 0 && fabs(rmax - rmaxsv) < 0.00000001)
      break;
    rmaxsv = rmax;
  }
  /* minimum distance: */
  eani = min_eanisv;
  eano = min_eanosv;
  for (k = 0; k < 3; k++) {
    xouter[k] = min_xouter[k];
    xinner[k] = min_xinner[k];
  }
  for (k = 0; k <= nitermax; k++) {
    osc_iterate_min_dist(eani, pqri, xinner, xouter, &eani, &rmin, TRUE);
    osc_iterate_min_dist(eano, pqro, xouter, xinner, &eano, &rmin, TRUE);
    if (k > 0 && fabs(rmin - rminsv) < 0.00000001)
      break;
    rminsv = rmin;
  }
  *dmax = rmax;
  *dmin = rmin;
  *dtrue = rtrue;
  return retval;
}

int32 CALL_CONV swe_orbit_max_min_true_distance_r(swe_ctx *ctx, double tjd_et, int32 ipl, int32 iflag, double *dmax, double *dmin, double *dtrue, char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 115 */
  ret = swe_orbit_max_min_true_distance_r_body(ctx, tjd_et, ipl, iflag, dmax, dmin, dtrue, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_orbit_max_min_true_distance(double tjd_et, int32 ipl, int32 iflag, double *dmax, double *dmin, double *dtrue, char *serr)
{
  return swe_orbit_max_min_true_distance_r(swi_default_ctx(), tjd_et, ipl, iflag, dmax, dmin, dtrue, serr);
}

/* function finds the gauquelin sector position of a planet or fixed star
 * 
 * if starname != NULL then a star is computed.
 * iflag: use the flags SE_SWIEPH, SE_JPLEPH, SE_MOSEPH, SEFLG_TOPOCTR.
 *
 * imeth defines method:
 *    imeth = 0    use Placidus house position
 *    imeth = 1    use Placidus house posiition (with planetary lat = 0)
 *    imeth = 2    use rise and set of body's disc center
 *    imeth = 3    use rise and set of body's disc center with refraction
 * rise and set are defined as appearance and disappearance of disc center
 *
 * geopos is an array of 3 doubles for geo. longitude, geo. latitude, elevation.
 * atpress and attemp are only needed for imeth = 3. If imeth = 3,
 * If imeth=3 and atpress not given (= 0), the programm assumes 1013.25 mbar;
 * if a non-zero height above sea is given in geopos, atpress is estimated.
 * dgsect is return area (pointer to a double)
 * serr is pointer to error string, may be NULL
 *
 */
static int32 swe_gauquelin_sector_r_body(swe_ctx *ctx, double t_ut,  /* input time (UT) */
  int32 ipl,    /* planet number, if planet, or moon;
                 * ipl is ignored if the following parameter (starname) is set*/
  char *starname, /* star name, if star; otherwise NULL or empty */
  int32 iflag,  /* flag for ephemeris and SEFLG_TOPOCTR */
  int32 imeth,  /* method: 0 = with lat., 1 = without lat., 
		 *         2 = from rise/set, 3 = from rise/set with refraction */
  double *geopos, /* array of three doubles containing
		   * geograph. long., lat., height of observer */
  double atpress, /* atmospheric pressure, only useful with imeth=3; 
		   * if 0, default = 1013.25 mbar is used */
  double attemp,  /* atmospheric temperature in degrees Celsius, 
                   *only useful with imeth=3 */
  double *dgsect, /* return address for gauquelin sector position */
  char *serr)
{
  AS_BOOL rise_found = TRUE;
  AS_BOOL set_found = TRUE;
  int32 retval;
  double tret[3];
  double t_et, t;
  double x0[6];
  double eps, nutlo[2], armc;
  int32 epheflag = iflag & SEFLG_EPHMASK;
  AS_BOOL do_fixstar = (starname != NULL && *starname != '\0');
  int32 risemeth = 0;
  AS_BOOL above_horizon = FALSE;
  if (imeth < 0 || imeth > 5) {
    if (serr)
          sprintf(serr, "invalid method: %d", imeth);
    return ERR;
  }
  /* function calls for Pluto with asteroid number 134340
   * are treated as calls for Pluto as main body SE_PLUTO */
  if (ipl == SE_AST_OFFSET + 134340)
    ipl = SE_PLUTO;
  /* 
   * geometrically from ecl. longitude and latitude 
   */
  if (imeth == 0 || imeth == 1) {
    int32 iflag_nut;
#ifndef SWE_UPSTREAM_COMPAT
    int32 retflag;
#endif
    t_et = t_ut + swe_deltat_ex_r(ctx, t_ut, iflag, serr);
#ifndef SWE_UPSTREAM_COMPAT
    /* Upstream bug 223: the house frame's obliquity and nutation took the
     * JPL Horizons flags as given, where the body's own call drops them
     * (plaus_iflag(): every request on the Swiss files or Moshier, every
     * sidereal one, the nodes, apsides and fictitious bodies): at 1950,
     * Zurich, with SEFLG_JPLHOR_APPROX, method 0 moved Mars on the Swiss
     * files by 1.49e-6 sector, the mean node on de200 by 1.57e-6 and
     * sidereal Mars by 1.41e-6, their places not at all. The body comes
     * first now, and the frame takes the Horizons bits its call answered
     * with: bug 142's SEFLG_JPLHOR_APPROX for SEFLG_JPLHOR without the
     * IERS corrections, and none where the place takes none. */
    if (do_fixstar)
      retflag = swe_fixstar_r(ctx, starname, t_et, iflag, x0, serr);
    else
      retflag = swe_calc_r(ctx, t_et, ipl, iflag, x0, serr);
    if (retflag == ERR)
      return ERR;
    iflag_nut = (iflag & ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX))
                | (retflag & (SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX));
#else
    /* Upstream bug 142: the obliquity and the nutation took SEFLG_JPLHOR
     * without the IERS corrections, which the body's swe_calc() turns into
     * SEFLG_JPLHOR_APPROX, and the nutation read their table, NULL: on
     * de200 methods 0 and 1 with SEFLG_JPLEPH | SEFLG_JPLHOR crashed
     * (SIGSEGV at J2000), for a planet or a star. They take the flag as
     * that swe_calc() does now. In this build; the shipped one takes the
     * Horizons bits the body's call returned (bug 223), which have it so. */
    iflag_nut = swi_jplhor_plaus(ctx, iflag);
#endif
    eps = swi_epsiln(ctx, t_et, iflag_nut) * RADTODEG;
    swi_nutation(ctx, t_et, iflag_nut, nutlo);
    nutlo[0] *= RADTODEG;
    nutlo[1] *= RADTODEG;
    armc = swe_degnorm(swe_sidtime0_r(ctx, t_ut, eps + nutlo[1], nutlo[0]) * 15 + geopos[0]);
#ifdef SWE_UPSTREAM_COMPAT
    if (do_fixstar) {
      if (swe_fixstar_r(ctx, starname, t_et, iflag, x0, serr) == ERR)
	return ERR;
    } else {
      if (swe_calc_r(ctx, t_et, ipl, iflag, x0, serr) == ERR)
	return ERR;
    }
#endif
    if (imeth == 1) 
      x0[1] = 0;
    *dgsect = swe_house_pos_r(ctx, armc, geopos[1], eps + nutlo[1], 'G', x0, NULL);
    return OK;
  }
  /* 
   * from rise and set times
   */
  if (imeth == 2 || imeth == 4) 
    risemeth |= SE_BIT_NO_REFRACTION;
  if (imeth == 2 || imeth == 3)
    risemeth |= SE_BIT_DISC_CENTER;
  /* find the next rising time of the planet or star */
  retval = swe_rise_trans_r(ctx, t_ut, ipl, starname, epheflag, SE_CALC_RISE|risemeth, geopos, atpress, attemp, &(tret[0]), serr);
  if (retval == ERR) {
    return ERR; 
  } else if (retval == -2) {
    /* actually, we could return ERR here. However, we
     * keep this variable, in case we implement an algorithm
     * for Gauquelin sector positions of circumpolar bodies.
     * As with the Ludwig Otto procedure with Placidus, one 
     * could replace missing rises or sets by meridian transits,
     * although there are cases where even this is not possible.
     * Sometimes a body both appears and disappears on the western 
     * part of the horizon. Using true culminations rather than meridan
     * transits would not help in any case either, because there are
     * cases where a body does not have a culmination within days,
     * e.g. the sun near the poles.
     */
    rise_found = FALSE;    
  }
  /* find the next setting time of the planet or star */
  retval = swe_rise_trans_r(ctx, t_ut, ipl, starname, epheflag, SE_CALC_SET|risemeth, geopos, atpress, attemp, &(tret[1]), serr);
  if (retval == ERR) {
    return ERR; 
  } else if (retval == -2) {
    set_found = FALSE;
  }
  if (tret[0] < tret[1] && rise_found == TRUE) {
    above_horizon = FALSE;
    /* find last set */
    t = t_ut - 1.2;
    if (set_found) t = tret[1] - 1.2;
    set_found = TRUE;
    retval = swe_rise_trans_r(ctx, t, ipl, starname, epheflag, SE_CALC_SET|risemeth, geopos, atpress, attemp, &(tret[1]), serr);
    if (retval == ERR) {
      return ERR; 
    } else if (retval == -2) {
      set_found = FALSE;
    }
  } else if (tret[0] >= tret[1] && set_found == TRUE) {
    above_horizon = TRUE;
    /* find last rise */
    t = t_ut - 1.2;
    if (rise_found) t = tret[0] - 1.2;
    rise_found = TRUE;
    retval = swe_rise_trans_r(ctx, t, ipl, starname, epheflag, SE_CALC_RISE|risemeth, geopos, atpress, attemp, &(tret[0]), serr);
    if (retval == ERR) {
      return ERR; 
    } else if (retval == -2) {
      rise_found = FALSE;
    }
  }
  if (rise_found && set_found) {
    if (above_horizon) {
      *dgsect = (t_ut - tret[0]) / (tret[1] - tret[0]) * 18 + 1;
    } else {
      *dgsect = (t_ut - tret[1]) / (tret[0] - tret[1]) * 18 + 19;
    }
    return OK;
  } else {
    *dgsect = 0;
    if (serr)
      sprintf(serr, "rise or set not found for planet %d", ipl);
    return ERR;
  }
}

int32 CALL_CONV swe_gauquelin_sector_r(swe_ctx *ctx, double t_ut,  /* input time (UT) */
  int32 ipl,    /* planet number, if planet, or moon;
                 * ipl is ignored if the following parameter (starname) is set*/
  char *starname, /* star name, if star; otherwise NULL or empty */
  int32 iflag,  /* flag for ephemeris and SEFLG_TOPOCTR */
  int32 imeth,  /* method: 0 = with lat., 1 = without lat., 
		 *         2 = from rise/set, 3 = from rise/set with refraction */
  double *geopos, /* array of three doubles containing
		   * geograph. long., lat., height of observer */
  double atpress, /* atmospheric pressure, only useful with imeth=3; 
		   * if 0, default = 1013.25 mbar is used */
  double attemp,  /* atmospheric temperature in degrees Celsius, 
                   *only useful with imeth=3 */
  double *dgsect, /* return address for gauquelin sector position */
  char *serr)
{
  int32 ret;
  swi_config_hold(ctx);	/* bug 124 */
  ret = swe_gauquelin_sector_r_body(ctx, t_ut, ipl, starname, iflag, imeth, geopos, atpress, attemp, dgsect, serr);
  swi_config_release(ctx);
  return ret;
}

/* Legacy entry point: the process-wide default context.
 * Kept byte-compatible -- this is the ABI. */
int32 CALL_CONV swe_gauquelin_sector(
  double t_ut,  /* input time (UT) */
  int32 ipl,    /* planet number, if planet, or moon;
                 * ipl is ignored if the following parameter (starname) is set*/
  char *starname, /* star name, if star; otherwise NULL or empty */
  int32 iflag,  /* flag for ephemeris and SEFLG_TOPOCTR */
  int32 imeth,  /* method: 0 = with lat., 1 = without lat., 
		 *         2 = from rise/set, 3 = from rise/set with refraction */
  double *geopos, /* array of three doubles containing
		   * geograph. long., lat., height of observer */
  double atpress, /* atmospheric pressure, only useful with imeth=3; 
		   * if 0, default = 1013.25 mbar is used */
  double attemp,  /* atmospheric temperature in degrees Celsius, 
                   *only useful with imeth=3 */
  double *dgsect, /* return address for gauquelin sector position */
  char *serr)
{
  return swe_gauquelin_sector_r(swi_default_ctx(), t_ut, ipl, starname, iflag, imeth, geopos, atpress, attemp, dgsect, serr);
}
