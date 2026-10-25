# Defects found during ts.20, not fixed there

Each line: what a caller sees, its size, who found it. Triaged with the
rest of ts.21 (`notes/TS21-PLAN.md`).

| Found by | Defect |
|---|---|
| P3 (fix 7) | A lone sign in a seorbel.txt element field is read as a value: "-" is -1, "+" is 1, "+ + 2" is 3; a dangling `*` is skipped ("T * + 2" is 3); implicit products ("1 2" = 2) are accepted. Old behaviour, ts20 left it too. |
| P3 (fix 7) | `save_star_in_struct()`: `ctx->fixed_stars = realloc(ctx->fixed_stars, ...)` loses the old block when realloc fails; a small leak on out-of-memory. |
| P3 (fix 7) | A star line without a comma crashes `fixstar_cut_string()` (`swi_right_trim(NULL)`); bug 106's guard, fix 6 (P5). Add a test case for it once P5 is merged. |
| P2 (fix 12) | `swe_houses_ex2()` with SEFLG_RADIANS leaves `ascmc[9]` (the Sun's declination for Sunshine) in degrees; `ascmc_speed[8..9]` are 0. |
| P2 (fix 12) | `CalcH()` Placidus/Gauquelin: the in-loop equator branch's `cusp_speed = armc_speed` is a dead store, overwritten after the loop. |
| P2 (fix 12) | Placidus and Gauquelin cusp speeds away from the equator are the partial rate at the converged pole height (ts20 bug 41, not in ts.20). |
| P1 (fix 1) | `swe_get_tid_acc()` reads an unstarted context (ts20 bug 101); an adopted tidal group shows the default -25.80 where DE441's automatic term is -25.936. |
| P1 (fix 1) | `swe_calc_pctr()` overwrites, and never checks, the return of its apparent-time `swe_calc_r()` for the centre. |
| P1 (fix 1) | ts20 bug 123: `swe_close()` zeroes the master's generation; a setting it keeps is reverted at the next sync after another setter. |
| P1 (fix 2) | Entry 83's tidal-reset item ("a path change resets a manual tidal acceleration") stays open: its fix is ts20 bug 92. |
| coordinator | ts20 bug 90's other parts: `jplplan()`'s Earth also takes the other-planet branch (answers identical); `app_pos_etc_plan()`'s `dt` maybe-uninitialised under -Og (a false positive). |
| P5 (fix 4) | With both buckets' files present, a context that had the previous bucket's file open keeps it for the first days of the next: up to 3e-7 AU from a fresh context (ts20 bug 89's class). |
| P5 (fix 4) | With semo, sepl and seas all missing, a heliocentric or barycentric request fallen to Moshier differs warm against fresh by 0.004-0.007 AU, and barycentric is answered heliocentric, up to 0.4 deg (ts20 bugs 109, 139). |
| P5 (fix 6) | `load_dpsi_deps()` leaves its FILE open on the finals loop's early return and on allocation failure (ts20 bugs 155, 156). |
| P5 (fix 6) | `get_new_segment()`'s "N coefficients instead of M" exit does not close the file, unlike the other exits. |
| P4 (fix 15) | Whole-sign and 0-Aries cusp speeds are still the Ascendant's and MC's (ts20 bug 40), tropical and sidereal. |
| P4 (fix 15) | `sidereal_houses_ssypl()` still folds by acos() before about 5650 BC (ts20 bug 296). |
| P4 (fix 13) | `swe_house_pos()` takes 'i' as 'I'; the Makransky cusps are off at 60N/60S (ts20 bug 290). |
| P4 (fix 14) | Gauquelin methods 0/1 with TOPOCTR see the place from `swe_set_topo()`'s observer, not geopos (ts20 273); SE_EARTH answers a meaningless sector (274); swetest's -sid house positions (253); the frame takes Horizons bits the place drops (223). |
| P6 (fix 17) | Fictitious-body speeds are not the rate of their position (ts20 bug 48): Nibiru 7.7e-6, Christ Comet 1.8e-4 deg/day. |
| P6 (fix 16) | swetest tests `-sidudef`'s t0 and value with `sid_mode & SE_SIDM_USER`, which is true for every mode but 0. Harmless. |
| P8 (fix 11) | The default nutation past 4e8 days is the direct series, whose arguments lose digits past about 1e9 days (ts20 bug 188). |
| P10 (fix 20) | `swe_sol_eclipse_when_loc()` leaves `tret[5..9]` unwritten when the Sun neither rises nor sets during the eclipse (upstream too); zero `tret[0..9]` at `next_try`. |
| P10 (fix 20) | swephprg does not say that the lunar `when_loc` replaces `tret[0]` with the moonrise or moonset; a doc note would help. |
| P9b (fix 19) | `swi_get_observer()` empties the caller's `serr` (through `swe_deltat_ex_r()`): a topocentric SPEED request loses the "using Moshier eph." note, and returns SEFLG_SWIEPH for a Moon from Moshier with no message (ts20 `085152e`). |
| P9b (fix 19) | A one-instant outlier in the topocentric Moon on the Swiss files, about 9e-8 deg, which the three-point topocentric speed turns into 1e-3 to 6e-2 deg/day; cause not found. Plus the topocentric speed items of ts20 bugs 34, 36, 73 and 79. |
| P7 (fix 10) | `swe_set_astro_models()` accepts any nutation model number; 99 reaches `calc_nutation()`, which writes nothing, and its callers in swehouse.c, sweph.c, swecl.c and swephlib.c keep an uninitialised `nutlo[2]`. Validate the model, or zero the output. |
| P7 (fix 9) | `swi_refuse_downgrade()`'s message is cut before "falling back to ..." past about 1e188 days (bounded); `sweph()`'s "jd %f < lower limit" still uses `sprintf` into a 256-byte local, safe only while the range tests come first (ts20 entry 216). |

| coordinator (fix 18) | The houses, `swe_azalt()` and the Gauquelin sectors take any date: past \|J\| 1e12 and at inf/NaN the sidereal time is NaN and `Asc1()` casts it to int (UBSan, swehouse.c). The range refusal of ts20 bugs 209 (houses part), 213 and 214. |
| P9a (fix 18) | Within 400-2000 centuries of TT every sidereal-time call integrates up to 160 panels from the table's end: bounded, but slow for scans. |
| P9a (fix 18) | G1b's margin on FMA platforms is about 2x (4.7e-6 against 1e-5). |
| upstream sync (bug 46) | swevents' node passages (`-donode`, the latitude's sign change) are the root of one parabola through the step's three latitudes, unrefined: at a 1-day step the Moon's in March 2000 come out 147, 74 and 2 s late (2000 Mar 2 19:42:46 UT for 19:40:19.2 by bisection). The same Newton step bug 46 gives the zero declination would fix it. Upstream's, older than `3fd0f95`. |
| upstream sync (re-review) | CI runs none of the gates added since ts.19, G21-G55 apart from those wired before (32 targets in `make check`, none in `.github/workflows/ci.yml`), so a green CI run says less than a local `make check`, which the workflow's own comment calls a hole. In the CI image all but G40 pass as they are; G40 needs `ephe/sat/sepm9501.se1`, which CI's checkout leaves out. |
| upstream sync (re-review) | `swedll.h`, the legacy DLL header, declares neither `swe_get_denum()` (upstream's omission) nor `swe_get_denum_r()`; the DLL exports both. |
