# Defects still present in upstream Swiss Ephemeris

Bugs found and fixed in a thread-safety fork of Swiss Ephemeris, **each one
verified still present in upstream** at `3fd0f95` ("with -astorb"), the
`legacy-master` this fork branched from. Line numbers below are upstream's.

This is not a diff of the fork. Most of what the fork changed — explicit
contexts, a C17 build, a strict ephemeris policy — is a deliberate
divergence upstream has no reason to want. What follows is only the subset
that is a defect in upstream's own terms: memory errors, crashes, and
answers that depend on what was computed before them.

**Severity** is about what a caller sees, not how hard it was to find:

| | |
|---|---|
| **Memory** | out-of-bounds read or write, or use of uninitialised memory |
| **Crash** | reachable null dereference |
| **Correctness** | a wrong number returned from a public function |
| **Robustness** | a crash or silent truncation on malformed input |

Each entry gives upstream's file and line, what a caller has to do to reach
it, and the shape of the fix. Where the fork's fix is quoted, it has been
reduced to the part that matters — the fork's version also carries a `ctx`
parameter that upstream has no equivalent for, and that is not part of the
bug.

**How these were found.** Almost none came from reading code looking for
defects. They came from asking why a particular function had low or zero
test coverage, and then building a test that reached it. Several of the
worst were in code that had never executed even once.

---

## Contents

| # | Defect | Severity | File |
|---|---|---|---|
| 1 | `find_conjunct_sun()` indexes an 18-element table with an asteroid number | Memory | `swehel.c` |
| 2 | `xaz[2]` passed where `swe_azalt()` writes 3 doubles | Memory | `swehel.c` |
| 3 | `swe_heliacal_ut()` copies uninitialised stack to the caller | Memory | `swehel.c` |
| 4 | `swe_set_jpl_file()` then a Horizons call — **segfault** | Crash | `sweph.c`, `swephlib.c` |
| 5 | `SEFLG_JPLHOR` contaminates later calls at the same instant | Correctness | `sweph.c` |
| 6 | `swe_calc_pctr()` depends on what was computed before it | Correctness | `sweph.c` |
| 7 | `swe_get_astro_models()` writes as much as the caller's argument is long | Memory | `swephlib.c` |
| 8 | JPL reader trusts the file's `ksize`/`ncf` against fixed arrays | Memory | `swejpl.c` |
| 9 | `sprintf(serr, "…%s…")` overflows the caller's buffer, ~37 sites | Memory | several |
| 10 | Changing the precession model does not invalidate the obliquity cache | Correctness | `swephlib.c`, `sweph.c` |
| 11 | Changing the ephemeris path does not re-read what was loaded from it | Correctness | `swedate.c`, `swephlib.c` |
| 12 | `setest/` — five defects in the shipped harness | Memory / Robustness | `setest/` |
| 13 | Lower severity, grouped | — | `swephgen4.c` |
| 14 | The tidal term behind delta-t follows which ephemeris files are open | Correctness | `swephlib.c`, `sweph.c` |
| 15 | `swe_calc(SE_ECL_NUT)` under another ephemeris wipes the open files' constants | Correctness / Memory | `sweph.c` |
| 16 | `swe_pheno(SE_ECL_NUT)` indexes two tables with -1 | Memory | `swecl.c` |
| 17 | `ipl * 100` overflows int32 for a large negative body number | Robustness | `sweph.c` |
| 18 | Changing delta t does not invalidate the cached topocentric observer | Correctness | `swephlib.c`, `sweph.c` |
| 19 | A fictitious body's position depends on SEFLG_SPEED | Correctness | `sweph.c` |
| 20 | A geocentric fictitious body mixes the dynamical and ICRS frames | Correctness | `swemplan.c`, `sweph.c` |
| 21 | A sidereal node or apsis carries the nutation in longitude | Correctness | `swecl.c` |
| 22 | The built-in Spica record's radial velocity is a stale catalogue value | Data | `sweph.c` |
| 23 | The Moon's light time is taken once, and read at a rounded instant | Correctness | `sweph.c` |
| 24 | The ephemeris files are read at TT; their argument is TDB | Correctness | `swejpl.c`, `sweph.c` |
| 25 | The default sidereal time steps at 1850 and 2050 | Correctness | `swephlib.c` |
| 26 | A heliocentric light time is measured from the barycentre on its second pass | Correctness | `sweph.c` |
| 27 | The topocentric observer is placed about the mean pole of date | Correctness | `sweph.c` |
| 28 | The osculating lunar apogee is fitted with a GM from a different almanac | Correctness | `sweph.c`, `swejpl.c` |
| 29 | The long-term sidereal time follows delta t | Correctness | `swephlib.c` |
| 30 | The Earth rotation angle is rounded at the size of the day count | Correctness | `swephlib.c` |
| 31 | The three-point speed is the slope at t + dt, not at t | Correctness | `sweph.c`, `swemmoon.c` |
| 32 | The Moon's speed leaves out the rate of its light time | Correctness | `sweph.c` |
| 33 | Speeds in the ecliptic of date leave out its turning | Correctness | `sweph.c` |
| 34 | The aberration rate is the observer's velocity change over the light time | Correctness | `sweph.c` |
| 35 | A planet's light-time rate is a one-day difference along a straight line | Correctness | `sweph.c` |
| 36 | The topocentric observer turns at the rate per UT day, per TT day | Correctness | `sweph.c` |
| 37 | `swe_nod_aps()` carries the ecliptic's turning about the wrong centre | Correctness | `swecl.c` |
| 38 | `swe_nod_aps()` returns the mean apsides' latitude as its speed | Correctness | `swecl.c` |
| 39 | The light deflection's speed is rough behind the Sun's disc | Correctness | `sweph.c` |
| 40 | Whole-sign, 0-Aries, Porphyry and Krusinski cusp speeds are not the cusps' rate | Correctness | `swehouse.c` |
| 41 | Koch, Placidus and Gauquelin cusp speeds leave out the moving semi-arc; differenced systems use one second | Correctness | `swehouse.c` |
| 42 | House speeds turn the ARMC at a constant rate and hold the obliquity still | Correctness | `swehouse.c` |
| 43 | Sidereal house speeds are the tropical ones | Correctness | `swehouse.c` |
| 44 | The interpolated apsides depend on SEFLG_SPEED, and x has no light time | Correctness | `sweph.c` |
| 45 | The interpolated apsides are rough at the JD's resolution, and so are their speeds | Correctness | `swemmoon.c`, `sweph.c` |
| 46 | `swe_nod_aps()`'s positions depend on SEFLG_SPEED | Correctness | `swecl.c` |
| 47 | `swe_nod_aps()` under SEFLG_J2000 is not the J2000 frame | Correctness | `swecl.c` |
| 48 | A fictitious body's speed is a fixed ellipse's, not its elements' | Correctness | `swemplan.c` |
| 49 | A fixed star's speeds are not the rates of its position | Correctness | `sweph.c` |
| 50 | The Moshier series round their arguments at more time than a JD resolves | Correctness | `swemplan.c`, `swemmoon.c` |
| 51 | The Moshier velocities are differences at rounded instants | Correctness | `swemplan.c`, `swemmoon.c`, `sweph.c` |
| 60 | A negative mean anomaly with no T terms freezes a fictitious body | Correctness | `swemplan.c` |
| 54 | `SEFLG_SPEED3` takes the true node's and osculating apogee's speeds over 0.1 day | Correctness | `sweph.c` |
| 55 | The mean nodes' and apsides' speeds are one-day differences | Correctness | `swecl.c`, `swemmoon.c` |
| 58 | The nutation's rate in every speed of date is a backward difference | Correctness | `sweph.c`, `swephlib.c` |
| 62 | The aberration rate's Earth leaves out the Moon's pull | Correctness | `sweph.c` |
| 61 | A topocentric fixed star is deflected from the geocenter | Correctness | `sweph.c` |
| 59 | Four small house defects: Placidus cusps unconverged, RADIANS speeds in degrees, sidereal Makransky computed as Treindl, Sunshine houses without files | Correctness | `swehouse.c` |
| 68 | Three house residues: Sunshine's Sun from the wrong ephemeris, stale Gauquelin cusps 13..36 after the Porphyry fallback, ARMC's rate for a Placidus cusp on an equinox point | Correctness | `swehouse.c` |
| 63 | A planet's light-time position is read at the rounded instant, and its heliocentric light-time rate leaves out the Sun's velocity | Correctness | `sweph.c` |
| 64 | The planets' mean nodes' and apsides' speeds leave out their elements' t^2 and t^3 terms | Correctness | `swecl.c` |
| 67 | The star functions return the caller's flags, and an emptied nutation cache answers for JD 0 | Correctness | `sweph.c`, `sweconfig.c` |
| 52 | The aberration rate term is first order in v/c | Correctness | `sweph.c` |
| 53 | Moshier moves a body back along its velocity instead of reading it at t - tau | Correctness | `sweph.c` |
| 56 | The interpolated apsides stop short of their iteration's fixed point | Correctness | `swemmoon.c` |
| 69 | The nutation matrix's rate holds the mean obliquity still | Correctness | `sweph.c` |
| 70 | `swe_nod_aps()`'s apparent speeds are not the rate of the aberration it applies | Correctness | `swecl.c`, `sweph.c` |
| 65 | Kronos' built-in semi-axis has two digits swapped, and the Kepler solver's starting guess is wrong | Correctness | `swemplan.c` |
| 66 | The aberration rate's Earth lacks the Moon on JPL and Moshier, and the planets everywhere | Correctness | `sweph.c` |
| 73 | The Moon's speed is not the rate of its position: the site's turning, the files' TDB rate, the light-time velocity, the aberration's shift rate | Correctness | `sweph.c`, `swejpl.c`, `swephlib.c` |
| 76 | A fixed star's aberration rate takes the Earth's acceleration half a step back | Correctness | `sweph.c` |
| 72 | The interpolated perigee's light-time estimate takes one pass, not two (this fork's) | Correctness | `sweph.c` |
| 74 | The light deflection's rate is a one-sided step, off by the deflection's curvature behind the Sun's disc | Correctness | `sweph.c` |
| 75 | The true node's and osculating apogee's speeds divide by the step asked for, not the step taken | Correctness | `sweph.c` |
| 71 | Every other cache emptied by an instant of 0 answers for JD 0, and sweph()'s forced EMB is the Earth there | Correctness | `sweph.c`, `sweconfig.c`, `sweph.h` |
| 78 | The planets' light time is solved along a straight line, and the Earth's and the Sun's light times have no rate | Correctness | `sweph.c` |
| 77 | `swe_calc_pctr()` reads its bodies at the rounded instant, its aberration rate is the centre's whole velocity change over the light time, it deflects as seen from the Earth, and its light time stops short of its fixed point | Correctness | `sweph.c` |
| 85 | The Moshier Earth-Moon offset's rate is a difference of rounded arguments | Correctness | `swemplan.c` |
| 88 | `swe_calc_orbel()` answers from caches of another instant and save areas of another ephemeris, and nothing on a context's first call (this fork's) | Correctness | `sweph.c` |
| 81 | A fictitious body's light-time position is read at the rounded instant, and its elements' large terms are rounded unreduced | Correctness | `sweph.c`, `swemplan.c` |
| 86 | The JPL barycentric Sun takes its instant of date from the Earth's slot | Correctness | `sweph.c` |
| 84 | `jplplan()`'s Earth and Sun answer for any ephemeris, and the JPL barycentric Sun is finished at the Earth slot's instant | Correctness | `sweph.c` |
| 79 | The topocentric site's acceleration is 1.6e-6 low | Correctness | `sweph.c` |
| 80 | `swe_nod_aps()`'s osculating speeds difference rounded points over a few minutes | Correctness | `swecl.c` |
| 82 | The heliacal memos answer for an atmosphere, latitude or star catalogue they do not hold | Correctness | `swehel.c`, `sweph.c`, `sweconfig.c`, `sweph.h` |
| 83 | A setting made before the first calculation is lost, and the first heliacal call after a setter fails (partly this fork's) | Correctness | `sweconfig.c`, `swehel.c`, `swephlib.c`, `swecl.c`, `sweph.c` |
| 90 | `swe_calc(SE_ECL_NUT, SEFLG_SPEED3)` differences the four angles as a position; `jplplan()`'s Earth also takes the other-planet branch; `-Og` false positive on `dt` | Correctness / Memory | `sweph.c` |
| 93 | `swe_nod_aps()`'s aberration rate takes the Moshier Earth's acceleration from the gravitational jerk | Correctness | `sweph.c` |
| 87 | The precession at exactly J2000 is the identity, which the Vondrak series is not | Correctness | `swephlib.c` |
| 91 | The ayanamsa's speed is a backward difference over 0.001 day | Correctness | `sweph.c` |
| 92 | The fixed-star memos and catalogue outlive a new path, a path change resets a manual tidal acceleration, and the topocentric observer outlives an ephemeris switch (partly this fork's) | Correctness | `sweph.c`, `sweconfig.c`, `sweph.h` |
| 96 | An Earth-centred fictitious body's light-time place on Moshier jitters with the second read of the Earth | Correctness | `sweph.c` |
| 89 | At the Swiss files' 600-year joins an open file answers past its nominal end, so the answer depends on the file an earlier call left open | Correctness | `sweph.c`, `sweph.h` |
| 98 | A star function as the first call ignores `SE_EPHE_PATH`, and a path change throws away the astronomical models | Correctness | `sweph.c` |
| 101 | `swe_get_tid_acc()` answers -25.80 after `SE_TIDAL_AUTOMATIC` and 0 before any initialisation, and the shared configuration copies an automatic tidal acceleration (partly this fork's) | Correctness | `swephlib.c`, `sweconfig.c` |
| 97 | The nakshatras of `swe_split_deg()` are shifted for SE_SIDM_USER, whose 255 passes a bit-mask test for Sheoran | Correctness | `swephlib.c`, `sweph.c` |
| 100 | A missing Swiss file whose neighbour does not hold the instant leaves the neighbour open, and the Moon is never read: NaN fresh, the last read's Moon warm (this fork's) | Correctness | `sweph.c` |
| 95 | The Moshier Moon's velocity is a difference of the series, and the osculating apogee's eccentricity is sqrt(1 - p / a): the osculating apogee and perigee jitter | Correctness | `swemmoon.c`, `sweph.c`, `sweph.h` |
| 104 | A refused Swiss file empties the constants of every file left open, so the next calls fail on healthy files; the path setter crashes on a refused moon file (the crash this fork's) | Correctness / Memory | `sweph.c` |
| 103 | Files closed and reopened when nothing changed (the implicit `SE_EPHE_PATH` path after any setter, the same path or JPL file again, an ephemeris switch, another thread's JPL file), and `swe_deltat()`'s ephemeris guessed from the files this thread has open (partly this fork's) | Correctness, performance | `sweph.c`, `swephlib.c`, `sweconfig.c`, `sweph.h` |
| 105 | `swe_nod_aps()`'s osculating ellipses take the Astronomical Almanac's GM, not the ephemeris's, and Pluto Mercury's mass | Correctness | `swecl.c`, `sweph.c`, `sweph.h` |
| 102 | Moshier's light time holds the Sun still: v_S tau left out of every place without aberration and every heliocentric one | Correctness | `sweph.c`, `swemplan.c`, `sweph.h` |
| 107 | With the files named by `SE_EPHE_PATH`, a first call that reads a file before `swe_calc()` names the path reads it along the compiled default path, and some read it twice: delta t, sidereal time, houses, stars, ayanamsa, eclipses, leap seconds, a JPL file | Correctness, performance | `sweph.c`, `swephlib.c`, `swedate.c` |
| 108 | `swe_nod_aps()`'s osculating points turned by the frame bias twice and its EMB built with the AA's Earth-Moon ratio; `swe_get_orbital_elements()` takes the AA's GMs and gives Pluto Mercury's mass | Correctness | `swecl.c`, `sweph.c`, `sweph.h` |
| 110 | A first call warns "Please call swe_set_ephe_path()" whenever it initialises the context, with a path in effect too: `swe_deltat_ex()` and `swe_get_ayanamsa_ex_ut()` answer correctly with the warning in serr, and `swe_calc_orbel()` refuses on it (that refusal this fork's) | Correctness (error reporting) | `sweph.c`, `swephlib.c`, `sweph.h` |
| 106 | Values read from an ephemeris file's header, and from the delta-psi and fixed-star tables, index and size arrays unchecked: a crafted file writes past `pldat[]`, `rot_back()`'s stack array, `sastnam[]` and `dpsi[]`, or reads past the JPL record | Memory | `sweph.c`, `swejpl.c` |
| 112 | `swe_calc_orbel()` (this fork's) refuses on the "Please call swe_set_ephe_path()" warning with no path at all, ignores the strict fallback setting, and never names the path from `SE_EPHE_PATH` itself | Correctness (behaviour) | `sweph.c` |
| 111 | `swe_get_orbital_elements()`: the true anomaly cancels near aphelion, e loses digits, and the periods leave out the body's mass (the tropical one 3.9e-5 long, the synodic against k) | Correctness | `swecl.c` |
| 113 | A seorbel.txt element field with a character no term has hangs `swe_calc()` for that body; a non-numeric epoch or equinox, an empty field, a short line are read as 0 or refuse every later body; a broken sefstars.txt is reported as "star not found", and `swe_fixstar()` with serr NULL answers an unknown star with the catalogue's last | Correctness (a hang; error reporting) | `swemplan.c`, `sweph.c` |
| 114 | A seorbel.txt element term reads a power "T5" to "T9" as nothing, "T0" as T, "T 2" as T2 times 2, a number ".0" as 1 and "1.2.3" as 1.2, all without an error | Correctness | `swemplan.c` |
| 116 | `swe_set_astro_models()` of a version before 2.01 sets the tidal acceleration but publishes only the SID group, so the default context's next sync puts the automatic term back (this fork's) | Correctness | `swephlib.c` |
| 118 | The path setter (this fork's code): a context's first tidal term opens the default path's moon file and the setter overwrites that FILE (106 descriptors after 100 contexts); the setters resolve the automatic tidal term before their new state, so `swe_get_tid_acc()` reports a dropped JPL file's; the same path again never sees a moon file installed since; the first-call warning probe opens the planet file the calculation then opens again | Correctness / Resources | `sweph.c`, `sweph.h` |
| 122 | `swe_set_ephe_fallback()` belongs to no configuration group: another thread on the default context, and a context made after it, keep `SE_EPHE_FALLBACK`'s value, refusing where the setting allows Moshier or the reverse; a first `swe_get_ephe_fallback()` on a thread reads 0 with the variable set (this fork's) | Correctness (behaviour) | `sweph.c`, `sweconfig.c`, `sweconfig.h` |
| 109 | Moshier's fictitious bodies, fixed stars and apparent light time stay heliocentric, and its heliocentric Moon's speed is off its positions | Correctness | `sweph.c`, `swemplan.c`, `sweph.h` |
| 117 | `swe_calc()`'s mean node and mean apogee speeds divide by 0.001 a step that is not 0.001 | Correctness | `sweph.c` |
| 121 | A sefstars.txt that failed to load is reread and reparsed by every later star call, a missing one searched for on every call | Performance / files | `sweph.c`, `sweph.h` |
| 119 | A thread that adopts another thread's `swe_set_jpl_file()` keeps the old file's positions, and one that adopts another's `swe_set_ephe_path()` the old directory's segments, and answers from them (this fork's) | Correctness (behaviour) | `sweconfig.c`, `sweph.c`, `sweph.h` |
| 120 | With the Swiss Moon file missing, the Moon and the Earth come from Moshier under a SEFLG_SWIEPH flag, and the strict setting answers | Correctness (behaviour) | `sweph.c`, `sweph.h` |
| 115 | A calculation on the default context (this fork's) adopts another thread's setter half-way through: its files closed under it (a crash on the JPL file), one answer from two configurations | Correctness (a crash; wrong answers) | `sweconfig.c`, `sweph.c`, `swecl.c` |
| 123 | `swe_close()` zeroes the shared configuration's generation, so a running thread misses the close and the publishes after it, and empties it, so a setting the close keeps -- the path, the JPL file, delta t, the nutation interpolation, the lapse rate, the fallback -- goes back to its default at the next setter and never reaches a thread started after it (this fork's) | Correctness (behaviour) | `sweconfig.c`, `sweconfig.h`, `sweph.c` |
| 124 | The searches, the houses, the sidereal time, `swe_azalt()`, `swe_pheno()`, delta t and the UTC conversions on the default context (this fork's) adopt another thread's setter between two of their calculations, or never: a sidereal Moon crossing 5600 s off, a local eclipse searched at another site from half-way, 3752 s late; another thread's user delta t kept out of the UTC conversions, 22 s | Correctness (behaviour) | `swecl.c`, `swehel.c`, `swehouse.c`, `sweph.c`, `swephlib.c`, `swedate.c` |
| 126 | `swe_set_interpolate_nut()` resets the interpolation's nodes and not the nutation, observer and positions cached at an instant, nor does a thread adopting the change (this fork's adoption): the old setting's nutation answers there, 7.42e-5" in dpsi | Correctness (behaviour) | `swephlib.c`, `sweconfig.c`, `sweconfig.h` |
| 125 | Moshier's light time past a day carries the Sun about the barycentre by bug 109's Taylor step: VEA 2e-7 deg off its light time's fixed point, its speed 5.4e-9 deg/day off its positions, a `swe_calc_orbel()` orbit at 80000 AU the same (this fork's) | Correctness | `sweph.c`, `swemplan.c`, `sweph.h` |
| 127 | `swe_close()` resets the observer and the sidereal mode, which the manual has survive it: after it a topocentric calculation is refused ("geographic position has not been set") and a sidereal one is in Fagan/Bradley's mode, 0.88 deg off Lahiri's -- on the closing thread, every thread on the default context and an explicit context closed by `swe_close_r()` | Correctness (behaviour) | `sweph.c`, `sweconfig.c`, `sweph.h`, `sweconfig.h` |
| 128 | A thread's first call reads the configuration before its context has started (this fork's): the main thread's user delta t answered 63.83 s for 86.40 s by `swe_deltat_ex()` and every function of UT through it (the Moon 3.1e-3 deg), its sidereal-time model missed by `swe_sidtime0()`, its models listed as zeros | Correctness (behaviour) | `sweconfig.c`, `sweph.c`, `sweph.h`, `swephlib.c`, `swedate.c` |
| 129 | A change of nutation model -- `swe_set_astro_models()`, its adoption by another thread, an explicit context, `SE_SIDBIT_PREC_ORIG`, `swe_close()` -- keeps the interpolation's nodes of the old model, and any change of model the topocentric observer: the old model answers at an instant already computed, 8.7e-3" in dpsi, the topocentric Moon 0.17" in AD 1000 after a delta t model | Correctness (behaviour) | `swephlib.c`, `sweconfig.c`, `sweph.h` |
| 130 | `swe_calc_orbel()` (this fork's) never pairs the frame bias, entry 20's defect on its copy of the fictitious path: its geocentric places are seen from the ICRS Earth, 3.2e-8 to 1.1e-7 AU off its heliocentric ones less the Earth (1.4e-5 deg at a = 1.2 AU), and with SEFLG_ICRS the body stays unturned, 23 mas off the same elements in seorbel.txt | Correctness | `sweph.c` |
| 132 | `swe_time_equ()`, `swe_lmt_to_lat()` and `swe_lat_to_lmt()` take the JPL Sun only when this context has the JPL file open: after `swe_set_jpl_file()` a new thread, a thread that computed before it and a context made after it answer from the Swiss files, E 1.19e-8 day (1.03 ms) off at J2000, 1.17e-7 day in 1800, and after `swe_close()` a JPLEPH calculation moves them to the JPL file while `swe_deltat()` stays Swiss | Correctness (behaviour) | `sweph.c` |
| 133 | The Sunshine houses' remembered declination, set by `swe_houses_armc()` before a context's first start, is wiped by the start's memset, which leaves 0 for 99, "none yet" (this fork's): a recall with 99 after the first calculation answers with declination 0, cusp 3 6.71 deg off | Correctness (behaviour) | `sweph.c`, `sweph.h` |
| 131 | After `swe_close()` without the path again, the kept path's first delta t takes the default tidal term (-25.80) until a file opens: 1463.43 s in AD 1000 for 1474.72, the first UT Moon there 1.6e-3 deg off the second, with de200.eph 14.29 s in AD 1700 for 3.01 -- on the closing thread, every thread and context adopting the close and an explicit context closed by `swe_close_r()` | Correctness (behaviour) | `sweph.c`, `sweconfig.c`, `sweph.h`, `sweconfig.h` |
| 135 | `swe_calc_orbel()` (this fork's) answers a barycentric Moshier request, which `swe_calc()` refuses: the heliocentric place under a return flag with SEFLG_BARYCTR set, 5.7e-3 AU off the barycentric one (0.29 deg at a = 1.2 AU) | Correctness (behaviour) | `sweph.c` |
| 136 | `swe_calc_orbel()` (this fork's) with elements of the equinox of date holds that equinox at the observation instant through the light time and leaves its turning out of the speed: 1.9e-2 deg at a = 80000 AU, 6.9e-6 deg at 30 AU, 3.5e-7 deg at 1.2 AU off the same elements as a seorbel.txt JDATE body, the speeds 4.0e-5 to 8.8e-5 deg/day off them and off the positions' rate | Correctness | `sweph.c`, `swephexp.h` |
| 134 | The interpolation's nodes are keyed on neither the JPL Horizons flags of the call nor the approximation's mode (`SE_MODEL_JPLHORA_MODE`), nor go with the IERS corrections `swe_close()` frees, and `swe_get_ayanamsa_ex()` takes the nutation kept at its instant whatever flags computed it: another setting's nutation answers there, 6.2e-2" after the flag per call, 0.104" after a change of mode, 0.131" across the corrections | Correctness (behaviour) | `swephlib.c`, `sweph.c`, `sweconfig.c`, `sweph.h` |
| 137 | A thread that had the JPL file open and adopts another thread's `swe_set_ephe_path()` keeps the closed file's DE number, so its SEFLG_JPLEPH delta t takes that file's tidal term: 3.012003 s in AD 1700 for the 14.286718 of the setting thread and a fresh process, after a path without the file or the same path by another string (this fork's) | Correctness (behaviour) | `sweconfig.c` |
| 138 | A JPL file a calculation opens sets the automatic tidal term for itself, not for `swe_deltat()`'s guess, which follows the configuration: after the path, de200.eph and the path again by another string, which forgets the JPL DE number, a TT JPL Sun left `swe_get_tid_acc()` at -23.8946 while the next `swe_deltat()` used -25.936 (18.902020 s in 1800) -- on every thread and context | Correctness | `sweph.c` |
| 145 | `swe_deltat_ex()` with no ephemeris bit in the flag takes the tidal term of the JPL file the context has open, else the default, not the Swiss files' that a 0 flag means to `swe_calc()` and every UT function: at 1950 28.927550 s on the thread that set de200.eph, 28.931886 on a second thread until it opens the file, for 28.932196; 169.4 s in AD 1000; a flag-0 eclipse search 11.3 s off | Correctness | `swephlib.c` |
| 139 | With the fallback allowed, a Swiss or JPL barycentric request whose files are missing is answered from Moshier, which refuses it asked for itself: Moshier's heliocentric place under SEFLG_BARYCTR and SEFLG_MOSEPH, Mars 0.24 deg and 7.2e-3 AU off the files' barycentric one, the Moon 0.37 deg; `swe_calc_orbel()`, `swe_calc_pctr()` and `swe_nod_aps()`'s barycentric ellipse alike | Correctness (behaviour) | `sweph.c` |
| 146 | The nutation interpolation's nodes lie both sides of `SEMOD_JPLHORA_3`'s step at 1962 Jan 20 under SEFLG_JPLHOR_APPROX (7.17e-2" in dpsi on the default model): dpsi and every place of date within a day of it interpolated across it, 1.95e-2" off the exact nutation at the switch + 0.3 after a call at - 0.5, 6.93e-2" at worst, where the interpolation is otherwise within about 3 mas | Correctness | `swephlib.c` |
| 140 | The topocentric site is placed by UT1 = TT - delta t rounded to a double, 2^-31 day: a staircase in delta t whose rate leaves out delta t's, which the site's velocity carries -- the topocentric speed of an orbit 0.01 AU from the Earth 1.8e-8 to 4.7e-8 deg/day off five-point differences of its positions (2.8e-7 on de200), the topocentric Moon's 2.5e-7, on `swe_calc_orbel()` and seorbel.txt alike | Correctness | `sweph.c` |
| 141 | `swe_calc_orbel()` (this fork's) returns SEFLG_SWIEPH where no ephemeris was named (18178 for `swe_calc()`'s 18176 with SEFLG_BARYCTR and SEFLG_SPEED) and drops SEFLG_RADIANS with SEFLG_XYZ (4354 for 12546): not `swe_calc()`'s return flag for the same flags, the answer the same | Correctness (behaviour) | `sweph.c` |
| 142 | `swe_get_ayanamsa_ex()`, its `_ut` form, the sidereal houses and `swe_gauquelin_sector()`'s methods 0 and 1 with SEFLG_JPLHOR and no IERS corrections loaded -- de200, a thread or context that never loaded them, eop_1962_today.txt missing or broken off -- read the corrections' table, NULL, and crash (SIGSEGV), where `swe_calc()` takes the flag as SEFLG_JPLHOR_APPROX | Crash | `swephlib.c`, `swephlib.h`, `sweph.c`, `swecl.c` |
| 143 | A thread or context that adopts another thread's `swe_set_jpl_file()` (this fork's) -- a thread running across it, one started after it, a context made after it -- never loads the IERS corrections, so its SEFLG_JPLHOR falls to SEFLG_JPLHOR_APPROX where the setter's does not: dpsi, the ayanamsa and the sidereal houses 4.2e-2" off the setter's on G134's fixtures | Correctness (behaviour) | `sweconfig.c`, `sweph.c`, `sweph.h` |
| 144 | `swe_set_ephe_path()` keeps the IERS corrections read from the old directory (`load_dpsi_deps()` loads once), and so does a thread adopting the path: after path A, the JPL file, path B and the JPL file again, SEFLG_JPLHOR answers A's dpsi, 0.131" off B's on G134's fixtures, and after path B alone A's for SEFLG_JPLHOR_APPROX's, 4.2e-2" | Correctness (behaviour) | `sweph.c` |
| 151 | Woolard's nutation (`SEMOD_NUT_WOOLARD`) turns the Moon's mean anomaly at 13255523.59 revolutions a century, 10^4 times its 1325.552359, so its terms in it cycle every 3.97 minutes: dpsi up to 0.236" and deps 0.0226" off the series, the interpolation on it 0.196"; `SE_SIDBIT_PREC_ORIG`'s Newcomb ayanamsas take it | Correctness | `swephlib.c` |
| 152 | The default nutation's grid (this fork's) leaves the IAU 2000A series it interpolates far inside its guard of |J| < 1e12 days, as the arguments' T^3 and T^4 terms speed them up: 2.3e-9 deg at |J| 6e8 (1.6 million years), 1.3e-4 at 1e9, 7 deg at 3e9, answered by `swe_calc(SE_ECL_NUT)` with no error; its rate 5.2e-4 deg/day at 1e9 | Correctness | `swephlib.c` |
| 147 | A thread adopting another thread's `swe_set_ephe_path()` keeps the old directory's delta t table, leap seconds and planet-name memo (this fork's): delta t at 1990 79.987645 s for 56.856191, the TT of 2026 UTC 1 s late, Cupido's name from the old seorbel.txt; and calc_deltat()'s memo outlives a path change and `swe_close()` on every thread, the same 79.987645 s at an instant asked before it | Correctness (behaviour) | `sweconfig.c`, `swephlib.c`, `sweph.c` |
| 148 | After `swe_set_ephe_path()` forgets the DE number of the JPL file whose name it keeps, the first JPLEPH delta t takes the default tidal term until a calculation opens the file: after the path, de200.eph and the path again by another string, the first UT JPL Moon in AD 1700 40.0057507520 deg and the second 40.0039614086 (1.79e-3 deg), `swe_deltat()`'s guess Swiss, where a fresh process given that path and de200.eph answers 40.0039614086 both times -- on every thread and context | Correctness (behaviour) | `sweph.c` |
| 153 | Under SEFLG_JPLHOR_APPROX with `SEMOD_JPLHORA_3`, the precession's, the obliquity's, the nutation's and the topocentric site's rates and SEFLG_SPEED3 difference the frame's models across their step at 1962 Jan 20 (0.113" of precession, 7.17e-2" of dpsi): every speed of date 1.57e-5 deg/day off for a day before it (Polaris's right ascension 1.31e-4), 9.96e-2 within 1e-4 day (1.09), SEFLG_SPEED3 5.75e-2 either side | Correctness | `sweph.c`, `swephlib.c` |
| 149 | `swe_nod_aps()` with barycentric output on Moshier, asked for or fallen to, answers Moshier's heliocentric points, returned OK: Mars's osculating node 0.077 deg off the files' barycentric one, the Moon's mean node 0.37 deg, the Sun's mean perigee 0.40 deg; every method and body class but the SE_NODBIT_OSCU_BAR ellipse beyond 6 AU | Correctness (behaviour) | `swecl.c`, `sweph.c`, `sweph.h` |
| 150 | `swe_calc_pctr()` returns the caller's flags, not the ephemeris that answered: with the fallback allowed and the JPL file alone missing Mars from Jupiter returns 257 (SEFLG_JPLEPH) with the Swiss answer bit for bit, and with the Moon files missing the Moon from Mars 258 (SEFLG_SWIEPH) with Moshier's Moon | Correctness (behaviour) | `sweph.c` |
| 157 | `SE_SIDM_LAHIRI_ICRC`'s mean ayanamsa at its epoch is 23.25 deg less 0.00464207 deg (16.7115"), the broken Woolard series' dpsi there (entry 151), not Woolard's 16.792088": with the series fixed, mode 46 with `SE_SIDBIT_PREC_ORIG` gives a true ayanamsa of 23 deg 15' 0.0806" on 1956 Mar 21 for the Calendar Reform Committee's 23 deg 15' 00" | Correctness | `sweph.h` |
| 154 | `swe_set_jpl_file()` of a file below DE 403, or of one not there, keeps the IERS corrections a DE 403 file loaded from the same directory: SEFLG_JPLHOR answers A's dpsi under its own return flag where a fresh process falls to SEFLG_JPLHOR_APPROX, 4.2e-2" on G134's fixtures, on the setter's thread and an explicit context | Correctness (behaviour) | `sweph.c` |
| 155 | `load_dpsi_deps()` allocates its two tables anew over those a load it gave up on left (a day missing, -2; eop_finals.txt broken off, -3): 589824 bytes leaked a reload | Memory | `sweph.c` |
| 156 | `load_dpsi_deps()` leaves eop_1962_today.txt open when a table's allocation fails, `dpsi[]` allocated when `deps[]`'s does, and eop_finals.txt open when a full table meets more days | Memory | `sweph.c` |
| 158 | `swe_calc_pctr()` overwrites the return of the centre's read at the light-time instant with the body's before testing it: a centre whose files begin between that instant and the date is taken as zero there, and Mars from Ceres in the first 0.024 day of seas_18.se1 returns 258 with a longitude speed 5.09e-3 deg/day off (0.130 upstream) | Correctness | `sweph.c` |
| 171 | An eop_finals.txt that breaks off (a day missing after those it adds) makes `load_dpsi_deps()` set -3, which `plaus_iflag()` takes as no IERS corrections at all: a complete eop_1962_today.txt is thrown away with it, against upstream's own comment ("no error, as we do have data"), and SEFLG_JPLHOR falls to SEFLG_JPLHOR_APPROX, dpsi 4.20e-2" off on G134's fixtures | Correctness (behaviour) | `sweph.c`, `sweph.h` |
| 172 | A failed allocation of the IERS corrections' tables sets ERR, eop_1962_today.txt's "not found", so SEFLG_JPLHOR's warning names a missing file that had just been opened | Robustness (message) | `sweph.c`, `sweph.h` |
| 173 | `swi_approx_jplhor()` interpolates the Horizons offset's yearly table backwards, (t - t0) (v[t0] - v[t1]) + v[t0]: the offset in right ascension jumps by 2 (v[t1] - v[t0]) at every year from 1963 to 2011, up to 0.99 mas (Polaris 2.87e-7 deg at 2006 Jul 1), its slope reversed between | Correctness | `swephlib.c` |
| 161 | Under SEFLG_JPLHOR_APPROX with `SEMOD_JPLHORA_3`, on a JPL file of DE 403 or later, `swi_bias()` puts the frame bias (23 mas) and the Horizons offset in right ascension (-51.26 mas) in from tjd >= 1962 Jan 20, where the precession and nutation change after it, and is given the instant the body was read at, the true Sun 0: over the light time after the step every body 1.1e-5 deg off (the true Sun at every date after it), at the step Polaris 4.95e-5 deg in right ascension, SEFLG_SPEED3 0.162 deg/day | Correctness | `sweph.c`, `swephlib.c` |
| 162 | Under SEFLG_JPLHOR_APPROX (`SEMOD_JPLHORA_1`, and `_3` after 1962 Jan 20), on a JPL file of DE 403 or later, `swi_approx_jplhor()` turns the position about the pole by the Horizons offset in right ascension (-51 to -53 mas) and not the velocity: speeds off their places' rate, the Moon's latitude 1.46e-6 deg/day, its right ascension 5.6e-7, Mars's 1.25e-7, the Sun's latitude 1.02e-7; distance rates 7.2e-9 AU/day on Mars | Correctness | `swephlib.c` |
| 164 | A JPL file whose `swe_set_jpl_file()` failed along the path then set, and a path where it is (or the file installed there and `swe_close()`): no DE number was read, so the path setter and the close read none, and the first UT JPL Moon in AD 1700 is 40.0057507520 deg and the second 40.0039614086, `swe_deltat()` 15.091463 s, where a fresh process given that path and the file answers 40.0039614086 twice and 3.012003 s -- on every thread and context | Correctness (behaviour) | `sweph.c`, `sweconfig.c`, `sweph.h`, `sweconfig.h` |
| 165 | With no JPL file set, a JPLEPH delta t knows no DE number and takes the default tidal term, and the calculation after it opens the default file, de431.eph, by its name (or its fallback de406.eph): with a de431.eph that is de200.eph the first UT JPL Moon in AD 1700 40.0057507520 deg and the second 40.0039614086 in a fresh process, the JPLEPH delta t 14.286718 s before a JPL calculation and 3.012003 after one | Correctness (behaviour) | `sweph.c`, `swephlib.c`, `sweconfig.c`, `sweph.h` |
| 166 | Without the moon files the Swiss files' tidal term is the default, -25.80 (DE431's), not the planet files' DE number's (DE441, -25.936): delta t in AD 1000 1463.430567 s for 1474.717432, 0.037 s at 1900, the heliocentric Mars 2.6e-7 deg off at 1900 and 5.8e-5 in AD 1000, the mean apogee 4.9e-8 at 1900 -- on every thread and context | Correctness | `sweph.c`, `swephlib.c`, `sweconfig.c`, `sweph.h`, `sweconfig.h` |
| 167 | `swe_nod_aps()` on Moshier takes the Earth for the observer of the Sun's geocentric points, not the Sun: the mean perigee at 2000 Jul 1.5 281.4259 deg at 1.9993 AU, the apogee 191.4583 deg at 0.0531 AU, where the files' are 282.9417 deg at 0.9833 AU and 102.9417 deg at 1.0167 AU; every method, SEFLG_TOPOCTR too | Correctness (behaviour) | `swecl.c` |
| 168 | `swe_nod_aps()` takes its frame and site from the caller's ephemeris bits, not from the ephemeris that answered or as `swe_calc()` resolves them: warm, a request fallen to Moshier adds an earlier call's Swiss Sun (Mars's node at 1500 0.13 deg off Moshier's, the Moon's heliocentric 0.40 deg), the site takes the files' delta t (2.9e-4 deg on the Moon's topocentric perigee); SEFLG_SWIEPH with SEFLG_MOSEPH, answered from the files, takes Moshier's frame (0.108 deg), SEFLG_JPLEPH with SEFLG_MOSEPH Moshier's frame bias (1.5e-4 deg) | Correctness (behaviour) | `swecl.c` |
| 169 | `swe_nod_aps()` reads the centre bits otherwise than `swe_calc()`: SEFLG_HELCTR with SEFLG_BARYCTR answers heliocentric (Mars's node 49.5625 deg for `swe_calc()`'s barycentric 49.6398), SEFLG_TOPOCTR with SEFLG_HELCTR heliocentric, SEFLG_TOPOCTR with SEFLG_BARYCTR the barycentric points less the site's offset (1.2e-3 deg off the barycentric node), where `swe_calc()` answers topocentric | Correctness (behaviour) | `swecl.c` |
| 159 | SEFLG_JPLHOR's precession and obliquity step at 1799 Jan 1 and 2202 Jan 1 (IAU 1976 within, Owen 1990 outside; 17.6 and 19.8 mas in the mean obliquity), and the precession's, the obliquity's and the site's rates and SEFLG_SPEED3 difference them across, under SEFLG_JPLHOR and (at 1799) SEFLG_JPLHOR_APPROX: every speed of date 2.8e-7 to 1.0e-6 deg/day off for a day either side, the Sun's latitude 5.0e-6 the day before, Polaris's right ascension 5.6e-6, SEFLG_SPEED3 8.9e-3 | Correctness | `swephlib.c`, `sweph.c` |
| 160 | A short-term precession model the caller chooses (IAU 1976 or 2000 within 2 centuries of J2000, IAU 2006 within 75) steps to the long-term one at its window's edges (0.53" with IAU 1976 under Vondrak, 107" with IAU 2006 at 75 centuries), and the precession's, the obliquity's and the site's rates and SEFLG_SPEED3 difference them across: every speed of date 8.0e-5 deg/day off within a day inside the window, latitudes 9.3e-7 the day before, SEFLG_SPEED3 0.80; with IAU 2006 1.5e-2 | Correctness | `swephlib.c` |

---

## 1. `find_conjunct_sun()` indexes a 18-entry table with an asteroid number

**Severity: Memory (out-of-bounds read, far out of bounds)**
**Where:** `swehel.c:2586`, table at `swehel.c:2566`

```c
static const double tcon[] = {
  0, 0,
  2451550, 2451550,  /* Moon */
  ...
  2451568, 2451753,  /* Neptune */
};                                        /* 18 elements */

static int32 find_conjunct_sun(double tjd_start, int32 ipl, int32 helflag,
                               int32 TypeEvent, double *tjd, char *serr)
{
  ...
  i = (TypeEvent - 1) / 2 + ipl * 2;
  tjd0 = tcon[i];                          /* swehel.c:2587 -- unbounded */
```

`tcon[]` tabulates two conjunction epochs each for the Sun through Neptune
and stops there. `ipl` reaches this function from `DeterObject()`
(`swehel.c:305`), whose final branch is:

```c
  if ((ipl = atoi(s)) > 0) {
    ipl += SE_AST_OFFSET;                  /* SE_AST_OFFSET is 10000 */
    return ipl;
  }
```

So a numeric object name produces `ipl >= 10000`, and the index becomes
`ipl * 2` — around **20000 into an 18-element array**. This is not an
off-by-one; it reads roughly 160 KB past the table into whatever the linker
placed next, and segfaults as soon as that address is not mapped.

**Reproducer**

```c
    double dret[50], dgeo[3] = {16.4, 48.2, 190}, datm[4] = {1013.25, 15, 40, 0};
    double dobs[6] = {36, 1, 1, 1, 1, 1};
    char serr[256];
    swe_set_ephe_path("/path/to/ephe");
    /* "433" is Eros; any numeric name reaches the same index */
    swe_heliacal_ut(2451545.0, dgeo, datm, dobs, "433",
                    SE_HELIACAL_RISING, SEFLG_SWIEPH, dret, serr);
```

The value read becomes `tjd0`, the seed epoch for the conjunction search, so
even when the address happens to be mapped the function iterates from a
number that means nothing.

**Fix.** There is no tabulated conjunction epoch for these bodies, so the
honest answer is a refusal rather than a search seeded from garbage:

```c
  i = (TypeEvent - 1) / 2 + ipl * 2;
  if (ipl < 0 || i < 0 || (size_t) i >= sizeof(tcon) / sizeof(tcon[0])) {
    if (serr != NULL)
      snprintf(serr, AS_MAXCH,
               "heliacal events are not available for object no. %d: "
               "no conjunction epoch is tabulated for it\n", (int) ipl);
    return ERR;
  }
  tjd0 = tcon[i];
```

---

## 2. `heliacal_ut_arc_vis()` gives `swe_azalt()` a 2-element array to write 3 doubles into

**Severity: Memory (stack write, 8 bytes past the end, four call sites)**
**Where:** `swehel.c:2215`, written at `swehel.c:2302, 2318, 2329, 2347`

```c
static int32 heliacal_ut_arc_vis(double JDNDaysUTStart, ...)
{
  ...
  double xaz[2];                           /* swehel.c:2215 */
  ...
    swe_azalt(tret, SE_EQU2HOR, dgeo, Pressure, Temperature, xin, xaz);
```

`swe_azalt()` writes three doubles to its output array — azimuth, true
altitude, and apparent altitude (`swecl.c`, in `swe_azalt`):

```c
  xaz[0] = 360 - x[0];
  xaz[1] = x[1];		/* true height */
  xaz[2] = swe_refrac_extended(x[1], ...);
```

so every one of the four calls writes eight bytes past the end of a stack
array. What that corrupts depends on the compiler's frame layout; with a
stack protector it is a clean abort, without one it is silent.

Two other functions in the same file declare `xaz[3]` for the same call,
which is what makes this look like a typo rather than a design.

**Reproducer.** Any `swe_heliacal_ut()` call that reaches the arcus-visionis
strategy — the default for a planet:

```c
    swe_heliacal_ut(2451545.0, dgeo, datm, dobs, "venus",
                    SE_HELIACAL_RISING, SEFLG_SWIEPH, dret, serr);
```

Build with `-fsanitize=address` to see it reported rather than tolerated.

**Fix.** `double xaz[3];`

---

## 3. `swe_heliacal_ut()` copies uninitialised stack memory to the caller

**Severity: Memory (uninitialised read, then an unbounded copy into the caller's buffer)**
**Where:** `swehel.c`, in `swe_heliacal_ut()` — declaration and the Moon branch

```c
  char ObjectName[AS_MAXCH], serr[AS_MAXCH], s[AS_MAXCH];   /* uninitialised */
  ...
    tjd = tjd0;
    retval = MoonEventJDut(tjd, dgeo, datm, dobs, TypeEvent, helflag, dret, serr);
    while (retval != -2 && *dret < tjd0) {
      tjd += 15;
      *serr = '\0';                       /* only inside the loop */
      retval = MoonEventJDut(tjd, dgeo, datm, dobs, TypeEvent, helflag, dret, serr);
    }
    if (serr_ret != NULL && *serr != '\0')
      strcpy(serr_ret, serr);             /* reads serr, which may never have been written */
    return retval;
```

`serr` is a local with no initialiser. The only unconditional write to it is
`*serr = '\0'` **inside** the `while` loop, which does not run when the first
`MoonEventJDut()` call already returns an acceptable date. And
`MoonEventJDut()` does not initialise it either — it forwards to
`moon_event_arc_vis()` or `moon_event_vis_lim()`, and neither writes `serr`
except on an error path.

So on the ordinary success path `*serr` is read uninitialised. If that byte
happens to be non-zero, `strcpy()` then copies stack garbage into the
caller's buffer and keeps going until it finds a zero — which is not
guaranteed to be within `AS_MAXCH`, so this can also overflow whatever the
caller passed.

The same pattern appears at the end of the function for the non-Moon path,
but there the loop above it always executes at least once, so that read is
safe today. It is safe by accident of control flow rather than by design.

**Reproducer**

```c
    char serr[256];
    double dret[50], dgeo[3] = {16.4, 48.2, 190}, datm[4] = {1013.25, 15, 40, 0};
    double dobs[6] = {36, 1, 1, 1, 1, 1};
    swe_heliacal_ut(2451545.0, dgeo, datm, dobs, "moon",
                    SE_HELIACAL_RISING, SEFLG_SWIEPH, dret, serr);
```

Under MemorySanitizer this reports a use of uninitialised value at the
`*serr` test. Under ASan it is invisible — reading uninitialised stack is not
an ASan finding — which is part of why it survived.

**Fix.** Initialise at the declaration:

```c
  char ObjectName[AS_MAXCH], serr[AS_MAXCH] = "", s[AS_MAXCH];
```

Initialising the buffer is preferable to adding `*serr = '\0'` before the
first call, because the same function has two more sites with the same shape
and one fix covers all of them.

---

## 4. `swe_set_jpl_file()` after a JPL Horizons calculation makes the next one segfault

**Severity: Crash (null dereference, reachable from ordinary setup calls)**
**Where:** free at `sweph.c:1268`, guard at `sweph.c:1387`, dereference at `swephlib.c:2091`

`load_dpsi_deps()` loads the IERS dpsi/deps corrections once and remembers
that it did:

```c
void load_dpsi_deps(void)
{
  ...
  if (swed.eop_dpsi_loaded > 0)
    return;                                   /* sweph.c:1387 -- load once */
  ...
  swed.dpsi = calloc(SWE_DATA_DPSI_DEPS, sizeof(double));
  swed.deps = calloc(SWE_DATA_DPSI_DEPS, sizeof(double));
```

`swi_close_keep_topo_etc()` frees both arrays and nulls the pointers, but
**does not reset `eop_dpsi_loaded`**:

```c
  if (swed.dpsi != NULL) {
    free(swed.dpsi);
    swed.dpsi = NULL;                         /* sweph.c:1270 */
  }
  if (swed.deps != NULL) {
    free(swed.deps);
    swed.deps = NULL;
  }
  /* eop_dpsi_loaded still says "loaded" */
```

So the next Horizons request reloads nothing, and `calc_nutation()` hands the
null pointer straight to `bessel()`:

```c
      dpsi = bessel(swed.dpsi, n + 1, J2 - swed.eop_tjd_beg);   /* swephlib.c:2091 */
```

whose first statement dereferences it:

```c
static double bessel(double *v, int n, double t)
{
  ...
  if (t <= 0) {
    ans = v[0];                               /* swephlib.c:2008 */
```

**What makes this easy to hit.** The function that frees the arrays is not
some teardown path — it is called by exactly two public functions, and both
are ordinary setup:

```
  swe_set_ephe_path()   sweph.c:1323
  swe_set_jpl_file()    sweph.c:1481
```

`swe_set_jpl_file()` is *required* to reach Horizons mode in the first place
— without it, `plaus_iflag()` downgrades `SEFLG_JPLHOR` to
`SEFLG_JPLHOR_APPROX` and says so. So any program that names a JPL file,
computes, then names another one — or simply re-points the ephemeris path —
crashes on its next Horizons calculation.

**Reproducer.** Needs a JPL file with DE ≥ 403 (e.g. `de431.eph`) and
`eop_1962_today.txt` present, which is what Horizons mode requires anyway:

```c
    double xx[6]; char serr[256];
    swe_set_ephe_path("/path/with/de431.eph/and/eop");
    swe_set_jpl_file("de431.eph");
    swe_calc(2451550.5, SE_MARS, SEFLG_JPLEPH | SEFLG_JPLHOR, xx, serr);
    swe_set_jpl_file("de431.eph");                 /* frees dpsi/deps */
    swe_calc(2451550.5, SE_MARS, SEFLG_JPLEPH | SEFLG_JPLHOR, xx, serr);  /* SEGV */
```

**Fix.** Put the flag back with the arrays it describes:

```c
  if (swed.dpsi != NULL) {
    free(swed.dpsi);
    swed.dpsi = NULL;
  }
  if (swed.deps != NULL) {
    free(swed.deps);
    swed.deps = NULL;
  }
  swed.eop_dpsi_loaded = 0;
```

**Why it was never seen.** `bessel()` is only reachable through
`SEFLG_JPLHOR`, which needs a JPL ephemeris of DE ≥ 403 *and* the IERS
corrections. Neither ships with the source, so the function had never
executed at all — it measured 0.00% coverage — and the first time it ran, it
crashed.

---

## 5. A `SEFLG_JPLHOR` request contaminates every later calculation at the same instant

**Severity: Correctness (wrong results from unrelated calls, up to ~0.9 arcsec in obliquity)**
**Where:** `sweph.c`, `swi_check_ecliptic()` and `swi_check_nutation()`

Both the obliquity and the nutation are cached per epoch:

```c
void swi_check_ecliptic(double tjd, int32 iflag)
{
  ...
  if (swed.oec.teps != tjd || tjd == 0) {
    calc_epsilon(tjd, iflag, &swed.oec);      /* recompute only if the EPOCH differs */
  }
}

void swi_check_nutation(double tjd, int32 iflag)
{
  ...
  if (!(iflag & SEFLG_NONUT)
	&& (tjd != swed.nut.tnut || tjd == 0
	|| (!speedf1 && speedf2))) {          /* epoch, plus the SPEED bit only */
```

But both cached values **depend on flags the key omits**. `calc_epsilon()`
forwards `iflag` to `swi_epsiln()`, and both it and `calc_nutation()` branch
on the Horizons flags:

```c
double swi_epsiln(double J, int32 iflag)
  ...
  if (iflag & SEFLG_JPLHOR)
  if ((iflag & SEFLG_JPLHOR_APPROX) && jplhora_model == SEMOD_JPLHORA_3)
```

The `SEFLG_JPLHOR_APPROX` branch adds the JPL Horizons offset to the
obliquity. Measured at −3000 that is **0.88 arcsec**. So once a Horizons
request has computed the obliquity for an instant, every later request about
**the same instant** is served that value — whatever flags it asked for.

Worse, it reaches callers that were explicitly excluded from Horizons mode.
`plaus_iflag()` strips `SEFLG_JPLHOR`/`_APPROX` for the nodes and apogees:

```c
  if (ipl == SE_OSCU_APOG || ipl == SE_TRUE_NODE
      || ipl == SE_MEAN_APOG || ipl == SE_MEAN_NODE
      || ipl == SE_INTP_APOG || ipl == SE_INTP_PERG)
    iflag = iflag & ~(SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX);
```

and they still get the Horizons obliquity, because the cache does not know
the difference.

**Scale.** Over 10 bodies × 4 epochs × 3 flag sets, **104 of 120 answers
changed** when a Horizons request preceded them. The prior request does not
even have to succeed: with no JPL file present it still resolves the flags
and fills the caches before the file open fails.

**Reproducer**

```c
    double a[6], b[6], junk[6]; char serr[256];
    swe_set_ephe_path(path);
    swe_calc(1356173.5, SE_MEAN_NODE, SEFLG_SWIEPH, a, serr);

    swe_close(); swe_set_ephe_path(path);
    swe_calc(1356173.5, SE_MARS, SEFLG_JPLEPH | SEFLG_JPLHOR, junk, serr);
    swe_calc(1356173.5, SE_MEAN_NODE, SEFLG_SWIEPH, b, serr);

    /* a[0] != b[0] -- the node moved because Mars was asked about first */
```

**Fix.** Put the flags the value depends on into the key. Only two bits
matter, since those are the only ones `swi_epsiln()` and `calc_nutation()`
consult:

```c
#define SWI_JPLHOR_FLAGMASK (SEFLG_JPLHOR | SEFLG_JPLHOR_APPROX)

/* struct epsilon gains: int32 epsflag; */

static void calc_epsilon(double tjd, int32 iflag, struct epsilon *e)
{
    e->teps = tjd;
    e->epsflag = iflag & SWI_JPLHOR_FLAGMASK;
    ...
}

/* and in swi_check_ecliptic() */
  if (swed.oec.teps != tjd || tjd == 0
      || swed.oec.epsflag != (iflag & SWI_JPLHOR_FLAGMASK)) {
    calc_epsilon(tjd, iflag, &swed.oec);
  }
/* likewise for oec2000, and copy epsflag through the tjd == J2000 branch */

/* and in swi_check_nutation(), adding to the existing condition */
	|| (nutflag & SWI_JPLHOR_FLAGMASK) != (iflag & SWI_JPLHOR_FLAGMASK)
```

There is precedent for this one level down: `calc_nutation()` already
declines its own memo for exactly these two flags, because its result depends
on them. This is the same argument applied to the enclosing cache.

After the change the same measurement gives **0 of 120**.

---

## 6. `swe_calc_pctr()` returns a different answer depending on what was computed before it

**Severity: Correctness (up to 24.5 arcsec)**
**Where:** `sweph.c:8042`, `swe_calc_pctr()`

The function transforms to the ecliptic of date using the shared obliquity
and nutation caches:

```c
    oe = &swed.oec;                    /* or &swed.oec2000 under SEFLG_J2000 */
    ...
    swi_coortrf2(xx, xx, oe->seps, oe->ceps);
    swi_coortrf2(xx, xx, swed.nut.snut, swed.nut.cnut);
```

but it never calls `swi_check_ecliptic()` or `swi_check_nutation()` for its
own `tjd` — **not once in the whole function**. Every other site that reads
those caches keys them first: `swecalc()`, `app_pos_etc_plan()`,
`swi_fixstar_calc_from_record()` all do.

Instead it relies on a side effect of the call at the top:

```c
  /* this fills in obliquity and nutation values in swed */
  swe_calc(tjd + swe_deltat_ex(tjd, epheflag, serr), SE_ECL_NUT, iflag, xx, serr);
```

That is already the wrong epoch — `tjd` is ET, so adding delta-t overshoots —
but the real problem is what comes next. **Four more `swe_calc()` calls run
between there and the transformation**, and each one re-keys the caches to
its own epoch:

```c
  retc = swe_calc(tjd, iplctr, iflag2, xxctr, serr);
  retc = swe_calc(tjd, ipl,    iflag2, xx,    serr);
  ...
    retc = swe_calc(t, iplctr, iflag2, xxctr2, serr);   /* t = tjd - light time */
    retc = swe_calc(t, ipl,    iflag2, xx,     serr);
```

so the transformation ends up using whatever the last of them happened to
leave — which depends on the caller's history, not on the arguments.

**Measured.** Asking for Mars from Jupiter at J2000, after computing one
other position first:

| the previous position's epoch | error |
|---|---|
| +1 day | 0.02 arcsec |
| +100 days | 1.89 arcsec |
| +1000 days | 2.94 arcsec |
| +10000 days | **24.5 arcsec** |

`swe_calc()` itself is stable under the same sequence — only
`swe_calc_pctr()` moves.

**Reproducer**

```c
    double x[6]; char serr[256];
    swe_set_ephe_path(path);
    swe_calc_pctr(2451545.0, SE_MARS, SE_JUPITER, SEFLG_SWIEPH, x, serr);
    double first = x[0];
    swe_calc(2451545.0 + 10000.0, SE_MARS, SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
    swe_calc_pctr(2451545.0, SE_MARS, SE_JUPITER, SEFLG_SWIEPH, x, serr);
    /* x[0] differs from `first` by ~24.5 arcsec */
```

**Fix.** Key the caches to the epoch being transformed to, immediately before
using them — the two calls every other site makes:

```c
  swi_check_ecliptic(tjd, iflag);
  swi_check_nutation(tjd, iflag);
  if (!(iflag & SEFLG_J2000)) {
    swi_precess(xx, tjd, iflag, J2000_TO_J);
    ...
```

**Independent check that this is the right direction.** Planetocentric
coordinates taken from the Earth must equal ordinary geocentric ones. That
identity holds better after the fix for every body tested:

| body | before | after |
|---|---|---|
| Mars | 3.92e-06″ | 1.10e-06″ |
| Jupiter | 1.93e-06″ | 1.25e-06″ |
| Venus | 4.90e-07″ | 3.45e-07″ |
| Saturn | 4.28e-06″ | 2.19e-06″ |

This is worth checking against an independent reference before adopting: it
is a real improvement in an invariant the library must satisfy, but the
remaining residual shows the identity is not exact either way.

---

## 7. `swe_get_astro_models()` lets a caller's argument decide how much it writes into the caller's buffer

**Severity: Memory (buffer overflow, reachable from a command-line argument)**
**Where:** `swephlib.c:4493`; the exposed caller is `swetest.c:802/1089/1331`

The function echoes `samod` — the **caller's** string — into `sdet`, the
caller's buffer, with a bare `sprintf`:

```c
      sprintf(sdet + strlen(sdet),
              "For list of all available astronomical models, add a '+' to the version string\n"
              "(swetest parameter -amod%s+ or -amod%s+)\n", samod, samod0);
```

Nothing bounds `%s`, and the API documents no size for `sdet`, so no buffer
the caller provides can be safe.

`swetest` supplies both halves of the problem:

```c
static char smod[2000];                       /* swetest.c:802 */
...
    } else if (strncmp(argv[i], "-amod",5) == 0) {
      astro_models = argv[i] + 5;             /* swetest.c:1089 -- straight into argv */
...
    swe_get_astro_models(astro_models, smod, iflag);   /* swetest.c:1331 */
```

**Reproducer**

```sh
    swetest -edir/path/to/ephe -amod$(python3 -c 'print("M"*2000)') -p0 -b1.1.2000
```

Under ASan: `global-buffer-overflow`, `WRITE of size 2133`. Without a
sanitizer it silently corrupts whatever follows `smod` in BSS.

**A second, quieter problem in the same function.** A `'+'` in `samod` makes
it enumerate every model, which writes about **1914 bytes** — into
`swetest`'s 2000. That leaves 86 bytes of headroom with no argument involved
at all, so adding one more model, or lengthening one description, overflows
it on its own.

**Fix.** Two parts, and the first is what makes the second possible:

1. Bound the echo, so the library's output no longer depends on the caller's
   input length:

```c
      sprintf(sdet + strlen(sdet),
              "...(swetest parameter -amod%.60s+ or -amod%.60s+)\n", samod, samod0);
```

   60 is well past anything meaningful — `set_astro_models()` reads a short
   digit list, and the `"SE2.06"` form is truncated to 20 characters before
   it is parsed.

2. Then document the buffer size at the prototype and give `swetest` real
   headroom, since the enumeration alone needs ~1.9 KB:

```c
/* sdet: a '+' in samod lists every model -- ~1.9 KB today, growing with each
 * one added. Give it 4000. */
ext_def(void) swe_get_astro_models(char *samod, char *sdet, int32 iflag);
```

---

## 8. The JPL reader trusts the file's own size fields against fixed arrays

**Severity: Memory (two overflows, both driven by values read from the file)**
**Where:** `swejpl.c:108–109` (the arrays), `swejpl.c:807` and `swejpl.c:~487` (the writes)

Two fixed buffers are filled to sizes the ephemeris file chooses:

```c
  double buf[1500];                             /* swejpl.c:108 */
  double pc[18], vc[18], ac[18], jc[18];        /* swejpl.c:109 */
```

**(a) `ncoeffs` against `buf[1500]`.** `ksize` is computed from `eh_ipt[]`,
which is read from the file header, then:

```c
    ncoeffs = ksize / 2;                        /* swejpl.c:674 */
    ...
    for (k = 1; k <= ncoeffs; ++k) {
      if (fread((void *) &buf[k - 1], sizeof(double), 1, js->jplfptr) != 1) {   /* :807 */
```

Nothing checks `ncoeffs <= 1500`. A file whose header implies more simply
writes past `buf` into the rest of the `jpl_save` structure — `pc`, `vc`,
`ac`, `jc` and `do_km` all follow it.

Note this is not only a malformed-file concern: real ephemerides sit close
to the limit, and the code carries hard-coded `ksize` values up to 2036 for
DE403/405/410/413/414/418/421.

**(b) `ncf` against `pc[18]`.** `ipt[i*3+1]` is the per-body coefficient
count, also straight from the file, and `interp()` uses it to index the
Chebyshev work arrays:

```c
  int ncf = (int) ncfin;
  ...
  if (np < ncf) {
    for (i = np; i < ncf; ++i)
      pc[i] = twot * pc[i - 1] - pc[i - 2];     /* pc is double pc[18] */
```

A file claiming more than 18 overflows `pc`, and then `vc`, `ac`, `jc` in
turn, since they are adjacent members.

**Fix.** Bound both against the arrays, and tie the array size to the bound
so the two cannot drift apart later:

```c
/* fsizer() rejects any ksize outside [1000, 5000]; ncoeffs is ksize/2, so
 * this is that bound restated as a coefficient count. */
#define JPL_NCOEFF_MAX 2500

  double buf[JPL_NCOEFF_MAX];

static_assert(sizeof(((struct jpl_save *) 0)->buf)
            / sizeof(((struct jpl_save *) 0)->buf[0]) == JPL_NCOEFF_MAX,
    "buf[] must hold exactly JPL_NCOEFF_MAX doubles");
```

and reject an implausible per-body count when the header is parsed, rather
than corrupting memory on the first `interp()` call:

```c
    for (i = 0; i < 13; ++i) {
      if (ipt[i * 3 + 1] > (int32) (sizeof(js->pc) / sizeof(js->pc[0]))) {
        if (serr != NULL)
          sprintf(serr, "JPL ephemeris file has an invalid coefficient count (%d) for body %d",
                  (int) ipt[i * 3 + 1], i);
        return NOT_AVAILABLE;
      }
    }
```

The `static_assert` matters more than it looks: the original `buf[1500]`
was too small for a legal `ksize` at the upper end of the range the reader
itself accepts, and nothing connected the two numbers.

---

## 9. `sprintf(serr, "…%s…", <caller string>)` overflows the caller's buffer

**Severity: Memory (buffer overflow into a caller-provided buffer)**
**Where:** ~37 sites across `sweph.c`, `swemplan.c`, `swehel.c`, `swecl.c`, `swejpl.c`, `swephlib.c`

`serr` is documented as `char[AS_MAXCH]`, and `AS_MAXCH` is **256**
(`sweodef.h:261`). Many error paths format a variable-length string into it
with a bare `sprintf` and no precision, so the caller's buffer overflows
whenever that string is long enough.

The cleanest example takes the string straight from the caller:

```c
/* swecl.c:1648, and identically at swecl.c:2456 */
      sprintf(serr, "occultation never occurs: star %s has ecl. lat. %.1f",
              starname, ls[1]);
```

`starname` is the `char *starname` parameter of `swe_lun_occult_when_glob()`
/ `swe_lun_occult_when_loc()`. The fixed text is about 45 characters, so a
star name beyond roughly 205 characters writes past the end of whatever the
caller passed as `serr`.

**Reproducer**

```c
    char serr[256];                      /* AS_MAXCH, as documented */
    char star[400];
    double tret[10], attr[20];
    memset(star, 'A', sizeof star - 1); star[sizeof star - 1] = '\0';
    swe_set_ephe_path(path);
    swe_lun_occult_when_glob(2451545.0, SE_MOON, star, SEFLG_SWIEPH, 0,
                             tret, 0, serr);
```

Others take the string from a file path or a filename rather than the caller
— still unbounded, just less directly controlled:

```c
/* swejpl.c:232 */
	sprintf(serr, "alleged ephemeris file (%s) has invalid format.", js->jplfname);
/* swejpl.c:758 */
	  sprintf(serr, "JPL ephemeris file %s is mutilated; length = %d instead of %d.",
	          js->jplfname, ...);
/* sweph.c:7575 */
      sprintf(serr, "star %s not found", star);
```

**Fix.** `snprintf` with the destination size, and a precision on each `%s`
so the variable part cannot crowd out the fixed text:

```c
      snprintf(serr, AS_MAXCH,
               "occultation never occurs: star %.180s has ecl. lat. %.1f",
               starname, ls[1]);
```

Two things worth knowing if you take this on, both learned the hard way:

- **Size the precisions so the worst case fits.** Converting the sites but
  leaving a precision too generous (`%.230s` in a message with 30 characters
  of fixed text) still overruns — and `-Werror=format-truncation` only
  reports it in some optimisation modes, so a build can look clean and fail
  elsewhere.
- **Watch the escapes when editing in bulk.** A mechanical sweep across these
  sites is easy to get subtly wrong: `"\\n"` written where `"\n"` was meant
  produces a literal backslash-n in the message, which no test notices.

---

## 10. Changing the precession or nutation model does not invalidate the cached obliquity

**Severity: Correctness (the old model's values keep being returned)**
**Where:** `swephlib.c`, `swe_set_astro_models()`; `sweph.c`, `swe_set_sid_mode()`

`swed.oec` (obliquity) and `swed.nut` (nutation) are cached and keyed on the
epoch alone — see entry 5. Their *values* also depend on which model is
selected, through `swed.astro_models[]`:

```c
double swi_epsiln(double J, int32 iflag)
{
  ...
  int prec_model = swed.astro_models[SE_MODEL_PREC_LONGTERM];
```

But `swe_set_astro_models()`, which changes exactly those settings,
invalidates **nothing** — no reference to `swed.oec`, `swed.nut`, `teps`,
`tnut` or `swi_force_app_pos_etc()` appears anywhere in it.

`swe_set_sid_mode()` is close but not sufficient: it calls
`swi_force_app_pos_etc()`, which clears the saved planet positions —

```c
    swed.pldat[i].xflgs = -1;
    swed.nddat[i].xflgs = -1;
```

— and leaves the obliquity and nutation caches untouched.

So: compute a position, change the precession model, ask again about **the
same instant**, and the answer is still built on the old model's obliquity,
because the cache sees a matching epoch and returns.

**Reproducer**

```c
    double x1[6], x2[6]; char serr[256], sam[64];
    swe_set_ephe_path(path);
    swe_calc(1356173.5, SE_MARS, SEFLG_SWIEPH, x1, serr);   /* caches oec at this epoch */
    strcpy(sam, "0,1,1,0,0,0,0,0");                          /* switch precession model */
    swe_set_astro_models(sam, 0);
    swe_calc(1356173.5, SE_MARS, SEFLG_SWIEPH, x2, serr);   /* still the old obliquity */
```

Use a distant epoch — near J2000 most models take the same short-term branch
and agree, which is part of why this is easy to miss.

**Fix.** Give the setters an invalidation step. The minimum is to void the
epoch keys so the next request recomputes:

```c
static void swi_invalidate_models(void)
{
  swed.nut.tnut     = 0;
  swed.nutv.tnut    = 0;
  swed.oec.teps     = 0;
  swed.oec2000.teps = 0;
  swi_force_app_pos_etc();
}
```

called from `swe_set_astro_models()` and from `swe_set_sid_mode()`.

This is the same defect family as entries 5 and 6: a cache whose key omits
something its value depends on. Entry 5 is the flags, this one is the model
settings, entry 6 is a caller that never keys the cache at all. Fixing any
one of them does not address the others.

---

## 11. Changing the ephemeris path does not re-read the files that were loaded from it

**Severity: Correctness (a new ephemeris directory is silently ignored)**
**Where:** `swedate.c:87/319` (`init_leapseconds_done`), `swephlib.c:3098` (`init_dt_done`)

Two tables are loaded once from files found along `swed.ephepath`, each
behind a load-once flag:

```c
/* swedate.c:87 */
static TLS AS_BOOL init_leapseconds_done = FALSE;
...
/* swedate.c:319 -- init_leapsec() */
  if (!init_leapseconds_done) {
    init_leapseconds_done = TRUE;
    ...
    if ((fp = swi_fopen(-1, "seleapsec.txt", swed.ephepath, NULL)) == NULL)
```

```c
/* swephlib.c:3098 -- init_dt() */
if (!swed.init_dt_done) {
  swed.init_dt_done = TRUE;
```

`swe_set_ephe_path()` calls `swi_close_keep_topo_etc()`, which is where
path-derived state is dropped — it closes the ephemeris files, the JPL file
and the star file. **Neither flag is reset there**, and
`init_leapseconds_done` could not be reset from `sweph.c` anyway, being a
file-scope static in `swedate.c`.

So after the first use, the leap-second table and the delta-t table are
frozen to whichever directory happened to be current, and a later
`swe_set_ephe_path()` to a directory with its own `seleapsec.txt` or
`swe_deltat.txt` has no effect on them.

**Reproducer**

```c
    /* dirA/seleapsec.txt lists a leap second at 2026-12-31
       dirB/seleapsec.txt lists one at 2030-12-31 instead */
    double dret[2]; char serr[256];
    swe_set_ephe_path("dirA");
    swe_utc_to_jd(2026,12,31,23,59,60.0, SE_GREG_CAL, dret, serr);   /* accepted */
    swe_set_ephe_path("dirB");
    swe_utc_to_jd(2030,12,31,23,59,60.0, SE_GREG_CAL, dret, serr);   /* REFUSED  */
    swe_utc_to_jd(2026,12,31,23,59,60.0, SE_GREG_CAL, dret, serr);   /* still accepted */
```

A UTC time of `23:59:60` is valid only on a day the table lists, so whether
it is accepted is a direct read-out of which file is in force.

**Fix.** Reset both when the path changes. `init_dt_done` lives in `swed`
and can be cleared directly in `swi_close_keep_topo_etc()`; the leap-second
flag needs a small reset function exported from `swedate.c`, since it is a
file-scope static there:

```c
/* swedate.c */
void swi_reset_leapsec(void) { init_leapseconds_done = FALSE; }

/* sweph.c, in swi_close_keep_topo_etc(), beside the other path-derived state */
  swed.init_dt_done = FALSE;
  swi_reset_leapsec();
```

`init_dt()` re-seeds its table from the built-in values when it runs again,
so the flag is all that has to go back.

**Honest scope.** The leap-second half has the reproducer above. The delta-t
half is the same flag pattern in the same close path, and is fixed by the
same one-line reset, but I did not manage to build a reproducer that shows
it changing an answer — treat that half as fixed by inspection.

---

## 12. `setest/` — five defects in the test harness upstream ships

**Severity: Memory / Robustness** — none of these affect the library, but
they weaken the tool used to validate it. The leak in particular makes the
harness useless for finding leaks anywhere else.

**(a) A leak on every check that PASSES.** `check_s()` copies the actual
value onto the heap *before* comparing it (`checkpoints.c`):

```c
    const typed_value act = { .value.s = copy_string(field),
                              .on_heap = true,
                              .type = S };
    entry exp = get_entry( name, S, ctx );
```

but `clear_failures() -> st_free()` only frees copies that reached a
`failure`. Every passing string comparison loses one — **4153 bytes in 1038
allocations** in a single `setest t` run, which is all of the harness's
leakage. Fix: take the copy inside the branch that stores it, as
`check_equals_s_internal()` already does.

**(b) `is_blank()` returns false for every input** (`globals.c:86`):

```c
bool is_blank(const char *s) {
  for (const char *s1 = s;s1 != '\0';s1++) if (!isspace(*s1)) return false;
```

`s1 != '\0'` compares the **pointer** against the null pointer constant, so
the walk ends only at the terminator — where `isspace()` fails and the
function returns false. `-Wpointer-compare` names it; setest builds with
`-Wall` alone, and that warning needs `-Wextra`. Fix: `*s1 != '\0'`, and cast
to `unsigned char` for `isspace()` since these files are UTF-8.

**(c) `ends_with()` reads before the buffer** (`globals.c:47`):

```c
bool ends_with(const char *string, const char * sub) {
  const int len = strlen(string);
  const char* sub_start = string + len - strlen(sub);
  for (const char *p = string+len-1; p>=sub_start; p--) {
```

With `strlen(string) < strlen(sub)`, `sub_start` points before the buffer.
It is data-dependent — the loop returns at the first mismatch, so it only
runs off the front when the tail matches — but ASan confirms it for
`ends_with("x", ".fix")`. The caller then does
`test_collection[strlen(name)-4] = '\0'`, a write at `[-3]`, if the
comparison spuriously succeeds. Fix: `if (strlen(sub) > len) return false;`

**(d) Unbounded `strcpy` from `argv`** (`setest.c:182`):

```c
           strcpy(ctx->test_collection,argv[optind]);
```

into `char test_collection[SETEST_MAX_SYMBOL_SIZE]` — 50 bytes — in the
middle of `test_context`. A longer name overwrites every member after it and
dies later somewhere unrelated; ASan cannot see it, because the overflow
stays inside one object. Three sites then build `"<name>.exp"` in another
`char[50]`, overflowing that by up to four more. Fix: refuse a name that does
not fit, rather than truncate — a truncated name reads someone else's
expectations.

**(e) SIGFPE on a malformed fixture** (`multivalues.c:126`):

```c
      default:
        fprintf(stderr,"Could not parse multivalue expression '%s'\n",p0);
        return;                      /* the function is void */
```

The caller never learns, the table stays at length 0, and
`multivalues_get_next()` evaluates

```c
    mv->i = ( mv-> i + 1 ) % mv->length;    /* multivalues.c:31 */
```

— integer division by zero, core dumped, with the real cause already
scrolled past. The `double` version of the same parser has the mirror bug:
it exits on the parse failure but *returns* on `!ok`, silently running a
smaller test set. Fix: both unrecoverable paths should exit, as the
surrounding code already does in the other half of each pair.

---

## 13. Lower severity, grouped

**`degstr()` formats into a 20-byte static** — `swephgen4.c:171`

```c
  static char a[20];	/* must survive call */
  ...
  sprintf (a, "%c%3d %2d'%5.2f\"", sign, ideg, imin, sec);
```

`%3d` is a minimum width, not a maximum. For an in-range angle the result is
about 14 characters and fits; for a degree value with more than three digits
it does not. This is in a generator utility rather than the library, and
needs out-of-range input to bite, so it is a hardening item rather than a
live defect. `snprintf(a, sizeof a, ...)` is enough.

**Bare column offsets in the IERS and astorb parsers.** The finals and
astorb readers slice fixed-width records with numeric literals at the call
site. Nothing is out of bounds today, but the widths are stated nowhere and
a format change would be silent. Naming the offsets costs nothing and makes
the next format revision reviewable.

---

## 14. The tidal term behind delta-t follows which ephemeris files are open

`swi_get_tid_acc()` resolves the Moon's tidal acceleration — the term every
pre-1955 delta-t carries — from whichever ephemeris file happens to be open
at the moment it is asked:

```c
/* swephlib.c, swi_get_tid_acc() */
if (denum == 0) {
    if (iflag & SEFLG_JPLEPH) {
      if (swed.jpl_file_is_open)
	denum = swed.jpldenum;
    }
    if (iflag & SEFLG_SWIEPH) {
      if (swed.fidat[SEI_FILE_MOON].fptr != NULL)
	denum = swed.fidat[SEI_FILE_MOON].sweph_denum;
    }
}
```

`calc_deltat()` passes `fidat[SEI_FILE_MOON].sweph_denum` in directly, so
the same open-state reaches it from the other side. The DE number is a
property of the files on the configured path, not of the call sequence —
but the call sequence is what this reads.

Upstream hides the dependence because its one global `swed` is pre-opened by
`swe_set_ephe_path()` itself, which reads the moon file's header for exactly
this DE number; every later delta-t in the process then finds the file open.
It still surfaces upstream as a first-call difference: a delta-t computed
before any file is open takes `SE_TIDAL_DEFAULT` (DE431's −25.8) where the
same instant after a file open takes the file's term — DE441's −25.936 for
current Astrodienst files, an 0.136 ct/sec² difference the pre-1955 tables
amplify.

The fork made it worse, and that is how it was found: `swe_ctx_new()`
children inherit the published configuration but not the asking context's
open files, so every child's first delta-t took the default term and every
calculation after its first file open took the file's — the same body at the
same instant moved with call order.

**Reproducer** (fork API; the difference is invisible at J2000, where the
delta-t polynomial carries no tidal term — it needs a pre-1955 instant):

```c
    swe_set_ephe_path(".../ephem");           /* the moon file is DE441 */
    swe_ctx *c = swe_ctx_new();
    swe_calc_ut_r(c, 2415020.5, SE_MOON,    SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    /* 272.41632607666696  -- delta-t resolved with SE_TIDAL_DEFAULT */
    swe_calc_ut_r(c, 2415020.5, SE_MERCURY, SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    swe_calc_ut_r(c, 2415020.5, SE_MOON,    SEFLG_SWIEPH|SEFLG_SPEED, x, serr);
    /* 272.41633228606418  -- the same instant, ~0.05 arcsec away */
```

**Fixed on this fork** (notes/REVIEW.md, Closed): the setters' header
pre-opens publish the DE numbers as configuration — `sweph_denum_moon` with
`swe_set_ephe_path()`, `jpldenum_cfg` with `swe_set_jpl_file()` — and both
resolution sites read them whenever the files are closed. Property 5 of
`tests/ctxtest.c` holds it. Upstream's fix would be the same published
value, wherever upstream chooses to keep it.

---

## 15. `swe_calc(SE_ECL_NUT)` under another ephemeris wipes the open files' constants

`swe_calc()` clears its state when the ephemeris flag changes, and
deliberately leaves the files open for `SE_ECL_NUT`, whose answer depends on
no ephemeris:

```c
/* sweph.c:386, swe_calc() */
if (swed.last_epheflag != epheflag) {
    free_planets();
    /* close and free ephemeris files */
    if (ipl != SE_ECL_NUT) {  /* because file will not be reopened with this ipl */
      ...
      swi_close_ephe_files();
      swed.last_epheflag = epheflag;
    }
}
```

`free_planets()` sits outside the exemption. It zeroes every `pldat[]`,
which is where `read_const()` put each open file's constants -- `tfstart`,
`dseg`, `lndx0`. So after the `SE_ECL_NUT` call the files are still open,
`last_epheflag` still names the old ephemeris, and the constants are gone.
The next call under the old flag finds its file open, skips `read_const()`,
and `get_new_segment()` (sweph.c:4383) divides by `dseg = 0`: the segment
index is a double-to-int conversion of infinity (undefined behaviour), and
`iseg * 3` at :4389 overflows. On x86-64 the resulting file offset is
garbage and the call fails:

```c
    swe_calc(2440000.0, SE_SUN,     SEFLG_SWIEPH, x, serr);
    swe_calc(2440000.0, SE_ECL_NUT, SEFLG_MOSEPH, x, serr);
    swe_calc(2451545.0, SE_MARS,    SEFLG_SWIEPH | SEFLG_SPEED, x, serr);
    /* returns ERR: "Ephemeris file .../sepl_18.se1 is damaged (2)."
     * The same Mars call on a fresh process returns 327.962729488835. */
```

Reproduced on `3fd0f95` and on `91339e5`, one commit later. Nothing about the
sequence is unusual: a program that computes nutation with the Moshier flag
and planets with Swiss files reaches it.

**Fixed on this fork** by moving `free_planets()` inside the `ipl !=
SE_ECL_NUT` block, so the planets' data is freed exactly when the files are
closed. Nutation reads none of it, and every save area is keyed on the flags
it was computed under, so nothing stale can be served. G24 (`check-compat`)
holds it both ways, and G25 (`check-hostile`, UBSan) spells the sequence out.

## 16. `swe_pheno(SE_ECL_NUT)` indexes two tables with -1

`SE_ECL_NUT` (-1) is a valid `swe_calc()` body number, so the geocentric
`swe_calc()` at the top of `swe_pheno()` (swecl.c:3791) succeeds, and the
function carries on to

```c
/* swecl.c:3879 */  if (ipl < NDIAM) dd = pla_diam[ipl];
/* swecl.c:3891 */  if (ipl > SE_AST_OFFSET || (ipl < NMAG_ELEM && mag_elem[ipl][0] < 99)) {
```

Both read one element before the start of a static table, and the magnitude
branch goes on to read `mag_elem[-1][1..3]`. The call returns OK with a phase,
diameter and magnitude computed from whatever precedes those tables in the
data segment. `swe_pheno_ut()` has the same path. No caller wants phenomena
of the nutation record; the fix is to refuse it -- the fork returns ERR with
"illegal planet number -1." for any `ipl < SE_SUN`, after the Ceres-to-Vesta
renumbering. G24 holds it.

## 17. `ipl * 100` overflows int32 for a large negative body number

```c
/* sweph.c:422, swe_calc() */
if ((iflag & SEFLG_CENTER_BODY) && ipl <= SE_PLUTO && ...) {
    iplmoon = ipl * 100 + 9099; // planetary center of body
```

Any `ipl` below about -21.5 million overflows (signed overflow, undefined
behaviour). The call then fails as an illegal planet number, so on common
compilers nothing visible happens -- UBSan reports it, and an optimiser is
entitled to do anything. It is reached by a program that passes body numbers
from outside unchecked; that is how it was found, by fuzzing an ephemeris
server's wire protocol. The fork bounds the test at `ipl >= SE_ECL_NUT`,
which keeps upstream's arithmetic, flag clearing included, for every body
number that exists. G25 fuzzes every public calculation entry point over
edge ids and flag sets under `-fsanitize=undefined` and holds this and entry
15.

---

## 18. Changing delta t does not invalidate the cached topocentric observer

**Severity: Correctness (the first delta t wins for every later call at the same instant)**
**Where:** `swephlib.c`, `swe_set_delta_t_userdef()` and `swe_set_tid_acc()`; `sweph.c`, `swi_get_observer()`

The topocentric observer's position vector is cached in `swed.topd` and
recomputed only when the *instant* moves:

```c
    if (swed.topd.teval != pedp->teval
      || swed.topd.teval == 0) {
```

(`sweph.c:2526`, and again at `:3397` and `:3925`.) That observer is a
function of sidereal time, which `swi_get_observer()` derives from
UT = TT − Δt:

```c
  delt = swe_deltat_ex(tjd, iflag, serr);
  tjd_ut = tjd - delt;
```

So Δt is an input to the cached value, and the key does not carry it. The
only thing that ever sets `topd.teval = 0` is `swe_set_topo()` — and that
early-returns when the site is unchanged, so re-asserting the same site does
not clear it either. A **site** change therefore invalidates and a **Δt**
change does not, which is why a Greenwich answer and a Sydney answer at one
instant are both correct and this is not otherwise visible.

`swe_set_delta_t_userdef()` invalidates nothing at all:

```c
void CALL_CONV swe_set_delta_t_userdef(double dt)
{
  if (dt == SE_DELTAT_AUTOMATIC) {
    swed.delta_t_userdef_is_set = FALSE;
  } else {
    swed.delta_t_userdef_is_set = TRUE;
    swed.delta_t_userdef = dt;
  }
}
```

`swe_set_tid_acc()` is the same story by a second route: the tidal term
feeds `swe_deltat_ex()`.

**To reach it:** set a topocentric site, ask for a body, set a different
`swe_set_delta_t_userdef()`, ask again **at the same instant**. The second
answer is the first one. Measured on the Moon at J2000 from Zurich
(8.55°E, 47.37°N, 400 m): Δt 0 then 100 answers Δt 0 twice; Δt 100 then 0
answers Δt 100 twice; the two correct values are **6.98″** apart. The wrong
answer is not merely wrong — it depends on what the caller asked earlier,
which is entry 6's shape.

**The fix** is one cache line more than entry 10's: `swi_force_app_pos_etc()`
clears `pldat`/`nddat`/`savedat` and **not** `topd.teval`, so the setters
need

```c
  swed.topd.teval = 0;          /* force swi_get_observer() to recompute */
  swi_force_app_pos_etc();
```

on a real change of value — the "on a real change" matters, since a caller
that sets a Δt per row would otherwise discard every planet cache on each
one. In the fork this is `swi_invalidate_deltat()`, called from both
setters and from the cross-thread adoption path, and held by G26
(`tests/dtobs.c`), which needs no ephemeris files.

---

## 19. A fictitious body's position depends on SEFLG_SPEED

**Severity: Correctness (up to 64″; 0.095″ on an ordinary element set)**
**Where:** `sweph.c`, `app_pos_etc_plan_osc()`

Light time for a body from `seorbel.txt` is a straight line,
`x(t) - dt * v(t)`, iterated on `dt` with the straight-line position. Only
under `SEFLG_SPEED` does the function go on to re-evaluate the orbit at
`t - dt`, so the same question answers two positions depending on a flag
that is only meant to add columns. The straight line ignores the orbit's
curvature over the light time (0.095″ for the intramercurial Vulcan, set 16,
which goes round in 18.5 days), and it also leaves an equinox of date at the
observation instant rather than the emission instant (63.7″ for set 26 at
80,000 AU, 0.063″ for Proserpina).

**Fix, as done here:** after the straight-line first guess, solve the
light-time equation on the orbit itself, for every flag set: re-evaluate the
elements at `t - dt`, recompute `dt` from that position, and repeat until it
converges (1e-12 day). G27 `tests/fictlt.c`.

## 20. A geocentric fictitious body mixes the dynamical and ICRS frames

**Severity: Correctness (0.023″, constant, on every geocentric fictitious body)**
**Where:** `swemplan.c`, `swi_osc_el_plan()`; `sweph.c`, `app_pos_etc_plan_osc()`

The elements are dynamical (a mean ecliptic and equinox), and
`swi_osc_el_plan()` precesses the body to the dynamical J2000 frame, then
adds the Sun's or the Earth's barycentric position, which is ICRS.
`app_pos_etc_plan_osc()` subtracts the ICRS Earth and never applies a frame
bias, where `app_pos_etc_plan()` does for a planet (ICRS to J2000, for DE403
and later). The Earth a fictitious body is seen from therefore differs from
the one `swe_calc(SE_EARTH)` returns by the bias rotation: **15.5 km**, the
same for every body and under Moshier or files alike, or 0.023″ on Vulcan.

**Fix, as done here:** `swi_bias(..., backward)` on the body inside
`swi_osc_el_plan()` before the centre is added, and the forward
`swi_bias()` in `app_pos_etc_plan_osc()` where a planet takes it, both on
`swi_get_denum(SEI_EARTH, ...) >= 403`, so the pair cancels on the body and
rotates only what came from the ephemeris. G27 holds it: every body's
astrometric direction equals B(t − τ) − E(t) built from this library's own
geometric outputs, to 0.000000″.

## 21. A sidereal node or apsis carries the nutation in longitude

**Severity: Correctness (the whole of delta-psi, up to 17″, on every sidereal node and apsis from `swe_nod_aps()`)**
**Where:** `swecl.c`, `swe_nod_aps_r()`

`swe_calc()` runs its flags through `plaus_iflag()`, which says "if sidereal
bit is set, set also no_nutation bit": a sidereal planet, `SE_MEAN_NODE` or
`SE_OSCU_APOG` is the tropical mean ecliptic of date less the ayanamsa.
`swe_nod_aps()` has no such line. Called with `SEFLG_SIDEREAL` it applies
the nutation and then subtracts the ayanamsa, which is the mean one, so the
longitude of a node or apsis is out by delta-psi against the planets beside
it and against the same point from `swe_calc()`: **+12.82″** for the Moon's
mean node on 1990-06-16, +17.43″ on 1900-01-01, for the mean and osculating
methods and for all four points of every planet.

Found by comparing Astrolog's sidereal chart through Ephemeris Prometheia's
server (which puts the node on the mean ecliptic of date and subtracts the
ayanamsa) with the local Swiss answer: every body agreed to 0.02″ except the
nodes, 12.84″ apart. Neither engine's rate or ephemeris was involved.

**Fix, as done here:** `if (iflag & SEFLG_SIDEREAL) iflag |= SEFLG_NONUT;`
at the top of `swe_nod_aps_r()`, the same rule as `plaus_iflag()`. G30 holds
it: 144 longitudes (Moon, Mars, Jupiter; mean and osculating; four points;
three epochs; Fagan-Bradley and Lahiri) equal the NONUT tropical longitude
less `swe_get_ayanamsa_ex()` to under 1e-9 degrees, and fail by delta-psi
on the `SWE_UPSTREAM_COMPAT` build.

## 22. The built-in Spica record's radial velocity is a stale catalogue value

**Severity: Data (a 4.3 km/s error in the distance rate of the star named "Spica"; positions unaffected)**
**Where:** `sweph.c`, `get_builtin_star()`

`swe_fixstar2()` answers "Spica" from a record built into the source (it
serves `SE_SIDM_TRUE_CITRA`) and never reads `sefstars.txt` for that name, so
a corrected catalogue file cannot correct it. The record carries a radial
velocity of +1 km/s; SIMBAD's is now -3.31 (2023ApJS..266...11B, quality A).
A radial velocity moves no direction that matters and is the whole of the
distance rate, so the star's position is untouched and its distance rate was
2.5e-3 AU/day out. "Citra", the same star, is read from the file.

Found by grading Astrolog's star rates against Ephemeris Prometheia's, whose
Spica carries -3.31: every other star's difference vanished after the
catalogue was refreshed, and this one alone did not.

**Fix, as done here:** the record's radial velocity is -3.31. G31 builds a
catalogue whose "Citra" is that line and requires the built-in "Spica" to have
the same differenced distance rate to 1e-9 AU/day; with the old value it
fails by 2.49e-3. The other built-in records (Revati, Pushya, Mula) were
compared with SIMBAD the same day: Pushya's radial velocity and Mula's are
current, and Revati (zeta Psc) has no radial velocity in SIMBAD to compare.

## 23. The Moon's light time is taken once, and read at a rounded instant

**Severity: Correctness (up to 1.18 mas on the Moon over 1800-2100, on the Swiss files and JPL alike)**
**Where:** `sweph.c`, `app_pos_etc_moon()`

The light time of a body seen at t is the solution of
tau = |M(t - tau) - O(t)| / c: the distance from where the Moon was when the
light left it to where the observer is when it arrives. `app_pos_etc_moon()`
takes it once, from the distance at the instant of observation,
tau0 = |M(t) - O(t)| / c, reads the Moon at t - tau0, and stops. The Moon
moves about 30 km/s against the barycentre during its 1.3 s of light time,
so the distance it was read at is out by up to ~40 km, tau0 by up to
0.13 ms, and the position by **up to 1.08 mas**. A planet on the files
takes two passes (`app_pos_etc_plan()`); the Moon took one.

The instant read, t - tau0, is also a Julian date in one double, which near
the present resolves about 40 microseconds. The Moon covers 1.2 m in that,
**up to 0.18 mas more**, because the rounding is never carried back. The two
together reach 1.18 mas (G33, 40 dates in 1800-2100, geocentric), and 0.07
heliocentric, where the light time is 500 s and the observer the Sun.

Found by comparing Swiss's Moon with Ephemeris Prometheia's, which solves the
light time and carries the rounding through the velocity: once both read the
same JPL ephemeris, the recipe above (one pass, rounded instant, file read at
TT, which is entry 24) reproduces Swiss's Moon to 0.00000 mas at eight dates,
and the corrected recipe reproduces Prometheia's to 0.0000 mas.

**Fix, as done here:** keep the one read Swiss already makes at the rounded
t - tau0, and solve the equation from it: move the Moon along its barycentric
velocity at t by the difference between the instant read and t - tau, and
recompute tau from that, three times (each pass gains the factor v/c). The
rounding is (t - (t - tau0)) - tau0, exact since the two are close. No
second read of the ephemeris, so the cost is a few multiplications. Moshier
extrapolates by tau0 itself and rounds nothing; it gets the iteration only.
Its remaining 0.04 mas (0.5 heliocentric) is its own finite-difference
velocities, far inside its 766 mas mean distance from the Swiss files, so it
is not re-read either. G33 `tests/moonlt.c`: the astrometric direction equals
M(t - tau) - O(t) built from this library's own geometric vectors within
0.01 mas on the files and JPL (0.00007 measured) and does not depend on
`SEFLG_SPEED`; check-compat (G24) asserts the compat build keeps the 1.18 mas.

## 24. The ephemeris files are read at TT; their argument is TDB

**Severity: Correctness (0.88 mas on the Moon, 0.06 on the Sun, 45 m on the barycentric Earth)**
**Where:** `swejpl.c`, `state()`; `sweph.c`, `sweph()`

A JPL ephemeris takes TDB as its time argument (Teph, which for DE200 to
DE441 is TDB to the precision that matters). Swiss passes the TT it is
given: `jplplan()` says "we assume Teph ~= TDB ~= TT. The maximum error is
< 0.002 sec, corresponding to an ephemeris error < 0.001 arcsec for the
moon", with the conversion written out and commented off. TDB - TT is an
annual 1.66 ms; the Moon's 1 km/s against the Earth makes that up to 1.7 m,
**0.88 mas**, inside the comment's 1 mas but the largest single term
between Swiss and an ephemeris read correctly, and the barycentric Earth's
30 km/s makes it **45 m** (testpo.200's rows, worst). The commented formula
also divides seconds by 8640.

The Swiss files take the same argument as the JPL file they were fitted to:
over 2000-2020 the along-track offset of the Swiss Moon from DE441 read at
the same number has an annual term of 0.006 ms, where a file fitted on a
converted argument would show 1.657. So both readers carry the defect.
Moshier is left alone: its theory takes TDT as its argument, and its Moon is
766 mas from the Swiss files on average.

Found comparing with Ephemeris Prometheia, which reads at TDB: with entry
23, this accounts for all of Swiss's astrometric Moon difference.

**Fix, as done here:** `swi_tdb_minus_tt()` (`swephlib.c`), the seven terms
of USNO Circular 179 eq. 2.6, within 9 us of the full Fairhead & Bretagnon
series over 1600-2169 and 28 us over 1-3000 (1 ms at -13000 and 17000,
where its linear term is extrapolated), with a one-entry memo per context.
It is added to the offset into the record, `(tjd - tseg0) + dtdb` for the
Swiss files and the fraction of the day in `state()`, with the carry into
the day, never to tjd itself, which would round it to the 40 us a JD
resolves near the present. Cost: +0.02-0.2 us a call at hourly steps, +3%
at scattered instants. G34 `tests/jpltdb.c`: JPL's own testpo.200 rows for
the geocentric Moon and the barycentric Earth (`tests/testpo200.txt`), each
asked at the TT whose TDB is its JED, match to 7 mm (testpo's last digit),
where the compat build misses by 1.5 m and 45 m; a G24 case asserts both.
Against Prometheia on DE440 at twelve dates 1600-2600, the astrometric Moon
went from 1.340 mas to 0.011 and every planet is within 0.002.

## 25. The default sidereal time steps at 1850 and 2050

**Severity: Correctness (a -1.908" step at 2050-01-01 0h UT, -0.097" at 1850-01-01; every house cusp and ascendant follows)**
**Where:** `swephlib.c`, `swe_sidtime0_r()`

`SEMOD_SIDT_LONGTERM`, the default, uses the IERS Conventions 2010 sidereal
time between 1850 and 2050 and `sidtime_long_term()` outside, which builds
it from the mean Earth of Simon et al. 1994 carried through the context's
precession. The two are spliced by constants, `SIDT_LTERM_OFS0` and `OFS1`.
`sidtime_long_term()` evaluates the mean Earth at TT for a UT instant, so it
moves 0.041" (the Sun's mean motion) for every second of delta t, where the
IERS expression moves 0.0001" per 100 s (precession only). The constants
were fitted under the delta t of their day: under Espenak-Meeus 2006, the
default until 2.06, the 1850 splice meets to 0.00003". Under Stephenson et
al. 2016, the default since, sidereal time steps by **-0.097"** across
1850-01-01 0h UT and **-1.908"** across 2050-01-01, in upstream 2.10.03 and
the fork alike; -0.509" at 2050 under Espenak-Meeus. A house cusp at high
latitude moves twice that (G1: 3.7").

**Fix, as done here:** the offset is the difference of the two formulas
at its boundary, under the context's own models (`sidt_lterm_offset()`),
so the splice is continuous for any precession, nutation or delta t.
Memoized per boundary on everything it reads, by value: the whole
`astro_models[]`, delta t at the boundary and at J2000 (struct
`sidt_splice_memo`). Cost: +1.8% on `swe_houses()` before 1850. G35
`tests/sidsplice.c`: the step across each boundary within 0.001" under the
default and four other model sets, in turn on one context and back to the
default (0.00004" measured); the compat build fails 10 of 12. A G24 case.
Sidereal time moves by the old step, constant, outside 1850-2050: +1.908"
after 2050 and -0.097" before 1850.

**The delta-t coupling itself** was then fixed as entry 29; the offsets
remain, now a few milliarcseconds (Vondrak against the IAU 2006
polynomial at the boundary). What follows is as first written. Away from
the boundaries the 0.041" per second remains. Against the IERS 2010 expression, the default is
+37" at 1000, +328" at year 0 and +2382" (0.66 deg) at -3000, of the
order of 0.041" times delta t (60", 434", 3082"; the rest is the two
precession treatments), where sidereal time at a given UT1 is set by the
Earth's rotation, not the Sun. Removing it would move every house before
1850 by tens of arcseconds at 1500 and by most of a degree of RAMC in
antiquity, a departure from upstream that every historical chart would
see, so it was left as a decision rather than made here. (Decided: fix
it. See 29.)
`SEMOD_SIDT_IERS_CONV_2010` already gives the ERA-based value at every
date (its precession polynomial is the IAU 2006 one, for near J2000).

## 26. A heliocentric light time is measured from the barycentre on its second pass

**Severity: Correctness (up to 0.84" on Mercury, 0.28" on Venus, 2.6" on a fictitious body inside Mercury's orbit; every heliocentric position from the files)**
**Where:** `sweph.c`, `app_pos_etc_plan()` and `app_pos_etc_plan_osc()`

The light time of a body seen from the Sun is the solution of
tau = |B(t - tau) - S(t)| / c, the body's barycentric place when the light
left it against the Sun's when it arrives. `app_pos_etc_plan()` iterates it
in two passes for the JPL and Swiss files (`niter = 1`). Each pass measures
the distance of an estimate of the body from `xobs` and reads the body at
t - dt. For a geocentric request `xobs` is the barycentric Earth and both
passes subtract it. For a heliocentric one the subtraction is skipped
(`if (!(iflag & SEFLG_HELCTR) && !(iflag & SEFLG_BARYCTR))` on both loops),
on the reasoning that `xx` is heliocentric already -- and on the first pass
it is, the Sun having been taken out above for the `SEFLG_TRUEPOS` answer.
But every later estimate is formed as `xx0 - dt * v`, and `xx0` is the
BARYCENTRIC position. So the second pass measures the distance from the
barycentre, and the light time the function keeps is the barycentric
distance over c. The Sun sits up to 0.01 AU from the barycentre, so the
instant the body is read at is off by up to 2.3% of its light time: 0.84"
on Mercury (2200-01-01), 0.28" on Venus, 0.09" on Mars, 0.016" on Jupiter,
against JPL Horizons' Sun-centred astrometric places and Ephemeris
Prometheia, which agree with each other to 0.00001" (2026-10-24). The
`SEFLG_SPEED` loop has the same skip, so the light-time term of the speed
carries it too.

`app_pos_etc_plan_osc()`, for a body from orbital elements, sets `xobs` to
the Sun for a heliocentric request and then skips it in both loops the same
way, so there the barycentric distance was measured on every pass: 2.6" on
Vulcan at J2000 (G36), whose light time is 0.00078 d at 19 degrees a day.

Found comparing heliocentric Mercury with Horizons and Prometheia; the
recipe above reproduces Swiss's number from its own barycentric positions
to roundoff, and the corrected recipe reproduces theirs.

**Fix, as done here:** the loops measure from `xobs` for every centre. In
`app_pos_etc_plan()` `xobs` is set for a barycentric request (zero) and a
heliocentric one (the barycentric Sun from the files; zero under Moshier,
whose stored positions are heliocentric), the iteration starts from the
barycentric vector `xx0` so that every pass measures the same quantity, and
the two skips are kept only under `SWE_UPSTREAM_COMPAT`. The same two skips
in `app_pos_etc_plan_osc()`, and the fork's `app_pos_etc_orbel()`, which
was written from it. No new reads of the ephemeris. G36 `tests/hellt.c`:
the heliocentric astrometric direction of Mercury to Pluto under the Swiss
files and de200.eph, and of a fictitious body from an element set written
by the test, equals B(t - tau) - S(t) built from this library's own
geometric barycentric positions, to 0.0001" (0.000000" measured; the
unfixed library misses on all 51 rows, worst 2.608"). check-compat (G24)
asserts the compat build keeps 0.27" on Mercury at J2000.

## 27. The topocentric observer is placed about the mean pole of date

**Severity: Correctness (200-270 m on the observer, 0.10-0.15" on the topocentric Moon; every topocentric position, eclipse and occultation geometry)**
**Where:** `sweph.c`, `swi_get_observer()`

The Earth turns about its true pole of date, and the apparent sidereal time
is the angle it has turned through, so the geocentric vector of a site is
built on the true equator and equinox of date and carried from there to
J2000. `swi_get_observer()` takes `SEFLG_NONUT` to mean something else:
under it the function uses the MEAN sidereal time and the mean obliquity,
skips its own nutation step, and precesses the vector to J2000 as if it
were on the true equator. Every caller in the library passes
`iflag | SEFLG_NONUT` (the J2000 vector is what they add to the barycentric
Earth), so the site was always placed relative to the mean pole, which is
up to 9" (the nutation in obliquity) and 17" x sin(eps) (the nutation in
longitude) from the pole the Earth actually turns about. 6378 km x that
angle is 200-270 m (G37: 15-277 m at five sites and five instants
1800-2100), and 200-270 m seen from the Moon is **0.10-0.15"**: measured
against Ephemeris Prometheia on the same delta t and against JPL Horizons
at six sites and six instants 1800-2100, 10-148 mas, every one of them
explained by the site offset to about 1 mas (2026-10-24). Planets: under
1 mas. The function also neglected the frame bias ("displacement of 45cm"),
though the Earth the site is added to is ICRS for DE403 and later and the
callers take the sum back to J2000 with the forward bias: 0.3 mas on the
Moon.

Found comparing the topocentric Moon with Prometheia and Horizons; the
recipe above reproduces Swiss's observer to the centimetre from its own
sidereal time, obliquity and nutation, and the true-pole recipe reproduces
theirs.

**Fix, as done here:** the observer is built on the true equator and
equinox of date whatever frame the caller wants its answer in: the
apparent sidereal time from the true obliquity and the nutation in
longitude, both computed at this instant rather than taken from the caches
(whose entry may be another instant's), the site vector and its velocity
rotated by the transposed nutation matrix to the mean equator of date,
precessed to J2000, and then, when the ephemeris is ICRS
(`swi_get_denum() >= 403`), taken to ICRS with the backward frame bias.
`SEFLG_NONUT` no longer says where the observer is. Upstream's branch is
kept under `SWE_UPSTREAM_COMPAT`. G37 `tests/topopole.c`: the geocentric
minus the topocentric GEOMETRIC Moon (`SEFLG_TRUEPOS`, so no light time
enters) in the true equator of date is the observer the library used; it
must equal the site built from the geodetic coordinates, the library's own
delta t, true obliquity and nutation, and `swe_sidtime0()`, to 5 cm
(0.0000 m measured; the unfixed library misses on all 25 rows, worst
276.6 m). Moshier, no files. check-compat (G24) asserts the compat build
keeps 197.5 m at Zurich, J2000. The `topd` cache is unchanged: it is keyed
by instant and invalidated with delta t (entry 18), and the new vector goes
through the same `do_save` path.

## 28. The osculating lunar apogee is fitted with a GM from a different almanac

**Severity: Correctness (up to 0.2" on SE_OSCU_APOG, median 0.08"; distance and speed with it)**
**Where:** `sweph.c`, `lunar_osc_elem()`

`lunar_osc_elem()` fits a two-body ellipse to the Moon's geocentric state
and reports its apogee (`SE_OSCU_APOG`). The gravitational parameter of that
ellipse was a compile-time constant, `GEOGCONST * (1 + 1 / EARTH_MOON_MRAT)`
with `GEOGCONST` the Astronomical Almanac's 1996 G M_earth and `EARTH_MOON_MRAT`
1 / 0.0123000383. That is 3.26e-8 above the GMB (G (M_earth + M_moon)) that
DE440/441 integrated the Moon with, 3.47e-8 above DE403's and 3.1e-9 above
DE200's. An osculating ellipse through a given state has its eccentricity
vector e = (v x h) / GM - r / |r|, and for a near-circular orbit a relative
error in GM moves the direction of e by about (1 + e) / e = 19 times that
error: the apogee's longitude was off by **up to 0.2"**, median 0.08" over
36 instants 1600-2600 against Ephemeris Prometheia (2026-10-24). With the
ephemeris's own GMB the two agree to 0.004" geometric, the remainder being
the Moon's state itself.

Found comparing the osculating apogee with Prometheia, whose apsides are
read off the ephemeris's own constants; the recipe above with the compiled
constants reproduces Swiss's apogee to 0.00005 mas, and the recipe with the
header's GMB reproduces Prometheia's.

**Fix, as done here:** `gm_earth_moon()` gives `lunar_osc_elem()` the GM of
the ephemeris in use. A JPL file carries GMB in its header, and that value
is used when the file is open and the Moon came from it (`swi_get_jpl_const()`,
new in `swejpl.c`, reads a named constant from the record `state()` has
already loaded). The Swiss files and Moshier carry no constants: the Moon
file's DE number names the JPL header the value is taken from (DE440/441,
DE430/431, DE405/406, DE403, else DE200; the pairs share one set of
constants), and Moshier, whose lunar theory is fitted to DE404 and for which
`swi_get_denum()` answers 403, takes DE403's. (DE404 has no header on
JPL's server, ssd.jpl.nasa.gov/ftp/eph/planets/ascii, checked 2026-10-24,
so that it shares DE403's constants is assumed; DE403 and DE405 differ by
3e-9 relative, far inside Moshier's own error. The other values were
checked against header.200, .403, .405, .430_572, .440 and .441 the same
day, to the last digit.) EMRAT is not needed: GMB is
the sum. Upstream's constant stays under `SWE_UPSTREAM_COMPAT`. G38
`tests/apogm.c`: the apogee DISTANCE the library reports, a (1 + e),
together with the Moon's geometric geocentric state it was fitted to
(`SEFLG_TRUEPOS` on both), determines the GM of the ellipse uniquely;
solving for it must give the ephemeris's GMB to 1e-10 relative, at four
instants 1900-2100 under Moshier (DE403's), the Swiss files (by their DE
number) and de200.eph (whose header the test parses itself, independently
of the library's reader). Before: all 12 rows fail, +3.47e-8 Moshier,
+3.26e-8 Swiss DE441, +3.07e-9 DE200. After: 1.6e-13. check-compat (G24)
asserts the compat build keeps 3.5e-8 under Moshier.

## 29. The long-term sidereal time follows delta t

**Severity: Correctness (+37" at 1000, +328" at year 0, +796" at -999, +2382" at -3000 against the Earth rotation angle; every house cusp before 1850 and after 2050 follows)**
**Where:** `swephlib.c`, `sidtime_long_term()`, `swe_sidtime0_r()`

Sidereal time at a UT1 instant is the Earth rotation angle, a linear
function of UT1, plus the equinox's accumulated precession in right
ascension, a function of TT: the IERS Conventions 2010, eq. 5.30-5.32.
Delta t enters only through the precession, 0.00015" per 100 s.
`sidtime_long_term()`, the default (`SEMOD_SIDT_LONGTERM`) outside
1850-2050, took it instead as the hour angle of the mean Sun plus the mean
Earth's longitude evaluated at TT for that UT instant (Simon et al. 1994),
so every second of delta t moved it by the Sun's mean motion: **4.1069"
per 100 s**, measured at every epoch from -1000 to 2200 by raising delta t
through `swe_set_delta_t_userdef()`. The IERS 2010 model moved 0.0001".
With delta t at hours in antiquity, the default stood against the IERS
expression at +37" at 1000, +328" at year 0, +2382" (0.66 deg of RAMC) at
-3000 -- of the order of 0.041" times delta t, the rest being the two
precession treatments.

**Fix, as done here:** outside 1850-2050 the default is now ERA(UT1) +
0.014506" + c T + A(T), the IERS 2010 construction with the IAU 2006
polynomial for A replaced by A integrated from the context's own
precession matrices, so it holds wherever the precession model does
(Vondrak 2011 by default, +-200 millennia):
- A(T) is the integral, from J2000, of the rotation rate of the mean
  equinox about the mean pole of date, -(dM/dt M^T)[1][0] for M =
  J2000 -> mean of date, the rate by central difference over 50 days. Its
  integral is the equinox's motion against a non-rotating origin.
- c = -0.003874"/cy: on the IAU 2006 matrices that rate at J2000 is
  zeta_A' + z_A' = 4612.160408"/cy, where the IAU 2006 sidereal time has
  4612.156534. The difference is the secular rate of the CIO locator s,
  which comes from precession-nutation coupling (Capitaine et al. 2003)
  and which no precession matrix carries.
- So corrected, the integral on the IAU 2006 matrices reproduces the IAU
  2006 polynomial to 0.09 mas over 1500-2500, which checks the method and
  the constant together (G39).
- A is tabulated per context, lazily outward from J2000, at nodes one
  century apart with the rate at each, and interpolated by a cubic Hermite
  polynomial (struct `sidt_pra_memo`, keyed by value on the two precession
  models). Eight-point Gauss-Legendre per node; panels of 10 centuries
  against 1 agree to 1e-7". Beyond +-400 centuries it integrates from the
  last node on each call.
- The complementary terms of the IERS expression are kept, and the 1850 and
  2050 offsets of entry 25 now absorb a few milliarcseconds.
- Cost: none measurable on `swe_houses()` before 1850 (52.2 to 51.8 us, it
  no longer computes a precession and a nutation per call); the table is
  built once per context and precession model, 0.2 ms the first time a
  date in 1500 is asked for, 1.6 ms for -3000, 4.8 ms for -13000.

The new default against the IERS 2010 expression: -0.046" at 1000, -0.29"
at year 0, -1.17" at -1000, -10.1" at -3000, +0.030" at 3000, within
0.001" over 1800-2200 -- Vondrak 2011 against the IAU 2006 polynomial,
which is meant for near J2000. In G1 sidereal time moved by -796" at -999,
-328" at year 1, -15.6" at 1300, -0.12" at 2100, -94" at 3000, and every
house, topocentric and Makransky row with it. Upstream's formula stays
under `SWE_UPSTREAM_COMPAT`. `swe_sidtime0()` called with eps = 0 now
adds nut * cos(0) like the IERS branch, where the old long-term formula
computed a nutation of its own in that case.

G39 `tests/sidera.c`: delta t + 100 s moves sidereal time by less than
0.001" at nine epochs from -50000 to +50000 (0.00014-0.00015" measured;
4.1" on the compat build); on the IAU 2006 precession it reproduces the
IERS 2010 expression within 0.2 mas over 1500-2500 (0.09 mas measured; up
to 19" on the compat build); and it is continuous where the table ends,
within what a JD resolves there (4.8 mas). G35 still holds the 1850 and
2050 splices. A G24 case asserts the delta-t coupling both ways.

## 30. The Earth rotation angle is rounded at the size of the day count

**Severity: Correctness (0.3 mas of sidereal time at -999, 1 mas at -13000, different on arm64; 1.9e-5 deg/day on the topocentric Moon's speed)**
**Where:** `swephlib.c`, `swe_sidtime0_r()`'s IERS 2010 branch (in the fork `era_deg()`, which it and entry 29's long-term sidereal time share)

The IERS 2010 sidereal time forms the Earth rotation angle as
`(0.7790572732640 + 1.00273781191135448 * jdrel) * 360`, jdrel = JD(UT1) -
J2000. At year -999 the product is 1.1e6 turns, which a double holds to
2.3e-10 of a turn, 0.3 mas, and the multiplication by 360 rounds again. The
JD itself resolves 20 us there, about the same 0.3 mas of rotation, but
that loss is the input's and is the same on every machine; this one is the
formula's, and where the compiler fuses the multiply-add into one rounding
(arm64 does by default, clang's -ffp-contract=on) the last bit differs. So
sidereal time differed between platforms by up to 0.3 mas in antiquity.

Upstream reaches this only when a caller selects
`SEMOD_SIDT_IERS_CONV_2010` (by default it is used only within 1850-2050,
where jdrel is small and the loss a few uas). Entry 29's fix made the
rotation angle the default at every date, and the fork's macOS and
apple-clang LTO jobs then failed G1b: the topocentric Moon's speed, which
`swe_calc()` differences from three positions 4.32 s apart when aberration
is on, moved 1.86e-5 deg/day (tolerance 1e-5) for a 5 uas shift of the
Moon. Reproduced on x86 with clang -O2 -mfma -ffp-contract=on, to the digit.

A second rounding comes before it: below JD 1225772 (-1357), |jdrel| is
larger than the JD and cannot hold every JD, so consecutive JDs share one
jdrel in pairs (1 mas at -13000).

**Fix, as done here:** the Conventions' own form, eq. 5.15: the fraction of
the day plus 0.7790572732640 + 0.00273781191135448 * jdrel, reduced to one
turn before the multiplication by 360. The fraction of the day is taken from
the JD itself (J2000 is a whole day), which is exact for any double; only
the small product rounds, 4.5e-13 of a turn (0.6 uas) at -999. Upstream's
form stays under `SWE_UPSTREAM_COMPAT`.

Over 64 consecutive representable JDs the sidereal time now lies on a
straight line within 0.0005 mas at -999, 0.003 at -13000, 0.0014 at +12000;
upstream's form scatters by 0.33, 1.03 and 0.78 mas. The FMA build against
the -O0 one: topocentric Moon speeds within 7.6e-7 deg/day (was 1.86e-5).
G1: 1,160 rows, all sidereal-time consumers; sidereal time moved by up to
0.36 mas, house cusps by up to 0.63 mas (system H at year 1), topocentric
Moon speeds by up to 5.2e-6 deg/day, which is the old rounding leaving
them.

G39 `tests/sidera.c` check 4 is the straight line, within 0.01 mas, at
-999, -13000 and +12000. A G24 case asserts it both ways on the IERS 2010
model at -13000 (0.0033 mas shipped, 1.03 mas compat).

## 31. The three-point speed is the slope at t + dt, not at t

**Severity: Correctness (1.3e-3 deg/day on the topocentric Moon's speed, 9.5e-5 with SEFLG_SPEED3 geocentric; the Moshier Moon's velocity 4.32 s late)**
**Where:** `sweph.c`, `calc_speed()`; `swemmoon.c`, `swi_moshmoon()`

`swe_calc()` computes a speed from three positions when the caller asks
for `SEFLG_SPEED3`, and on its own for every topocentric request with
aberration ("topocentric with SEFLG_SPEED is not good if aberration is
included"). `calc_speed()` fits the parabola x1 + b s + a s^2 through the
positions at t - dt, t and t + dt (s in steps of dt) and returned
(2 a + b) / dt: the parabola's slope at s = 1, the speed at t + dt -- 4.32
s late for the Moon, 8.64 s for a planet. The error is the acceleration
times dt; the topocentric Moon's diurnal parallax curves its path at ~30
deg/day^2, so its speed was off by up to 1.3e-3 deg/day (4.7"/day). The
Moshier Moon's velocity in `swi_moshmoon()` used the same formula on the
same three points, with the central difference beside it under `#if 0`.
A third copy, for the osculating node in `lunar_osc_elem()`, is
overwritten by a central difference before it is returned.

**Fix, as done here:** b / dt at all three. Upstream's formula stays under
`SWE_UPSTREAM_COMPAT`. The library's three-point speed now equals a
three-point difference of the positions it returns, to the last digit.
What is left in it is the positions' own roughness over 8.64 s, ~3e-5
deg/day on the topocentric Moon against a five-point difference over
+-0.02 day. The Moshier osculating apogee, which is fitted to the Moon's
velocity and amplifies it ~18 times, moves by up to 90" (closer to the
Swiss files': rms 62.3" to 59.4", 1800-2100).

## 32. The Moon's speed leaves out the rate of its light time

**Severity: Correctness (up to 4.3e-4 deg/day on the topocentric Moon without aberration, 3.6e-5 geocentric)**
**Where:** `sweph.c`, `app_pos_etc_moon()`

The Moon is seen at t - tau(t), so the velocity of M(t - tau) - O(t) is
v_M (1 - dtau/dt) - v_O, with dtau/dt = rdot / c.
`app_pos_etc_moon()` returned v_M - v_O. With v_M ~30 km/s barycentric,
dtau/dt = 1e-6 is 3e-5 km/s, 4e-4 deg/day at the Moon's distance. A
topocentric observer's diurnal 0.3 km/s along the line of sight reaches
that (up to 4.3e-4 measured); the geocentric range rate makes a tenth of it.
The planets have the term, from the light-time displacement a day apart
(`xxsp` in `app_pos_etc_plan()`); the Moon never did.

**Fix, as done here:** dtau/dt = rho^ . (v_M - v_O) / (c + rho^ . v_M), from
the solved light time's vectors, and v_M scaled by 1 - dtau/dt. Swiss
files and JPL: the geocentric Moon's speed within 5e-7 deg/day, the
topocentric one without aberration within 1.7e-6, 1800-2100. Moshier's
Moon is moved back along its velocity at t rather than read at t - tau,
so its speed still lacks tau times the Moon's acceleration (1.3e-4 deg/day
topocentric); left, as G33 left its light time.

G40 `tests/moonspeed.c` checks both against a five-point difference of
the library's own positions, J2000 equator: Moon geocentric, topocentric
through the three-point path, topocentric without aberration, and
`SEFLG_SPEED3` on the Moon and Mars, on the Swiss files, de200.eph and
Moshier (three-point cases only). The upstream build fails 11 of the 12
judged. G24 asserts each both ways. G1: 1,265 rows; speeds by up to 1.3e-3
deg/day, positions only in the Moshier osculating apogee and in iterated
results (0.1 mas); G17's swemini Moon speed 2e-5 deg/day.

## 33. Speeds in the ecliptic of date leave out its turning

**Severity: Correctness (up to 1.6e-5 deg/day on every ecliptic latitude speed of date, and on the equatorial speeds of the nodes and apsides)**
**Where:** `sweph.c`: `app_pos_rest()`, `app_pos_etc_mean()`, `lunar_osc_elem()`'s output, `intp_apsides()`, `fixstar_calc_from_struct()`, `swi_fixstar_calc_from_record()`, `swe_calc_pctr_r()`

The ecliptic of date is the equator rotated about the equinox by the
obliquity, and the obliquity changes: the mean obliquity by -47" a
century (3.6e-7 deg/day) and the nutation in obliquity by up to 1.3e-5
deg/day, from its fortnightly and semiannual terms. Every conversion
rotated the velocity with the position's matrix and nothing else, so the
velocity returned was not the rate of the rotated position: it lacked
de/dt (0, z', -y'). Measured against five-point differences of the
library's own positions: the Sun's, Moon's, every planet's, Chiron's, a
fixed star's and a planetocentric latitude speed off by 1.2-1.6e-5
deg/day; the true node's and the osculating apogee's declination speeds
the same, from the conversion out of the ecliptic.

**Fix, as done here:** `swi_obliquity_rate()` (the mean obliquity's rate
by a central difference over two days, plus the nutation in obliquity's
from the nutation at t and t - NUT_SPEED_INTV that `swi_check_nutation()`
already keeps for speeds) and `swi_ecl_frame_speed()`, which adds
de/dt (0, z', -y') entering an ecliptic of date and subtracts it before
leaving one. The J2000 ecliptic does not turn. Applied to the paths
above; latitude speeds now within 5.5e-8 deg/day, the nodes'
declination speeds within 2.8e-7.

Left alone on purpose: `swi_plan_for_osc_elem()`, which turns the Moon's
or a planet's state into the ecliptic to fit osculating elements; those
need the inertial velocity in the frame of the instant, and the node's
speed is taken from nodes differenced a step apart, which already carry
the turning. Not done here: `swe_nod_aps_r()` with the osculating method
(1.3e-5 deg/day in latitude, ecliptic and equatorial alike), whose
speeds take a different route (entry 37).

G41 `tests/eclspeed.c`: twelve speeds against five-point differences,
within 1e-6 deg/day, and 2.5e-7 where only the mean obliquity's rate is
at stake; the upstream build fails all twelve. G24 asserts it both ways
on the Sun (8.4e-9 shipped, 7.2e-6 compat). G1: 5,810 rows, speeds
only (latitude by up to 2.2e-5, longitude by ~5e-7 through the
latitude), and the Moon's node-crossing results by 0.3 mas, which iterate
on the latitude speed. G17: last digits of six speeds.

## 34. The aberration rate is the observer's velocity change over the light time

**Severity: Correctness (3.2e-4 deg/day on the topocentric Sun's speed, a few per cent of the diurnal aberration rate on every topocentric body, up to 7.4e-5 AU/day on distance rates, 2e-6 deg/day on distant bodies' longitude speeds); upstream answered the topocentric case from three positions instead**
**Where:** `sweph.c`: `app_pos_etc_plan()`, `app_pos_etc_plan_osc()`, `app_pos_etc_orbel()`, `app_pos_etc_moon()`, `app_pos_etc_sun()`, `swe_calc_r()`; `swecl.c`: `swe_nod_aps_r()`

Aberration moves the apparent place by about tau V across the line of
sight, V the observer's barycentric velocity and tau the light time, so
the apparent velocity carries tau dV/dt. The paths took that term as
V(t) - V(t - tau), the observer's velocity at the instant of observation
less at the instant the light left, added whole. Four things were wrong
with it:

1. **A turning observer.** A site on the Earth turns 2.5-5 deg in a
   planet's light time; V(t) - V(t - tau) for its 0.46 km/s is then a
   chord, not tau a(t), and off by a few per cent of a diurnal aberration
   rate of 5.6e-4 deg/day. Upstream knew: `swe_calc_r()` sent every
   topocentric request with aberration to the three-point speed (three
   positions 8.64 s apart, three times the cost, and their 0.1 mas
   roughness, ~3e-5 deg/day on the Moon; with entry 31, the slope at
   t + dt, 1.3e-3).
2. **The Sun had no such term.** `app_pos_etc_sun()` applied aberration
   with the velocity at t and no rate: 3.2e-4 deg/day topocentric, and
   the Moon's 0.6% of the Earth's acceleration, 5.6e-7 deg/day,
   geocentric.
3. **Its radial part.** Aberration keeps the distance to first order in
   v/c, so only the transverse part of tau a turns the apparent place; the
   whole vector moved the distance rate: 7.4e-5 AU/day on Pluto, 5.1e-5 on
   Neptune, 4e-6 on Mars (also found by the scan, `tests/scan`).
4. **Second order.** V(t) - V(t - tau) is tau a at the midpoint, off by
   tau^2 a'/2: 1% of the rate at a light time of a day, 1.9e-6 deg/day on
   Nibiru, 2e-7 on Pluto.

**Fix, as done here:** the observer's part is the Earth's plus the
site's. The site's is tau times its acceleration at t, a central
difference of `swi_get_observer()`'s velocity over +-2^-11 day
(`observer_accel()`, `observer_at_lt()`; truncation (omega h)^2/6 =
1.6e-6 of it. Bug 49's commit had set +-2^-8 day, whose 1e-4 put 2e-8
deg/day on every topocentric latitude speed and doubled the longitude
residual. The step was corrected afterwards: topocentric latitude speeds
of the planets went from 2e-8 to 6-9e-9 deg/day and the Sun's from
1.5e-8 to 6e-10, against five-point differences over 2^-9 day, -2900..
2900); the Earth's keeps the
difference of the two ephemeris reads, plus tau^2 a'/2 with a' the
rate of the Sun's pull at t from the heliocentric state alone
(`earth_at_lt()`). Not from the two reads' positions: across a segment
of the Swiss files those disagree with the velocities by 1e-9 AU (at
-3001 and 3000), 2e-5 deg/day once divided by tau; and on Moshier, whose
Earth velocity is a backward difference over 1e-4 day, the same
expression would triple that velocity's 1.5e-8 AU/day lag. The Moon's
part of a', 7.5%, is left out. Past a light time of 10 days the series
fails (3.7e-4 deg/day at the 462 days of a body 80,000 AU out), and tau
a(teval) is taken whole from the Sun's pull, which leaves out the Moon's
0.6% of a. The sum goes in less its part along the
line of sight (`add_aberr_rate()`). `swe_nod_aps()` had upstream's term for
its points too, 3e-4 deg/day on Pluto's topocentric nodes; it now calls the
same term (`swi_aberr_rate()`), <= 4e-8. The Sun's path reads the Earth at
t - tau for it (one more read, +7% on a geocentric Sun with speed). The
forced three-point path is gone; `SEFLG_SPEED3` still asks for it, and a
topocentric speed costs 24 us instead of 31-43.

G43 `tests/topospeed.c`: the topocentric Sun, Moon, Mercury, Venus, Mars
and Jupiter at Zurich and 70 N, nine dates -999..2100, against
five-point differences on binary-fraction instants, within 1e-6 deg/day
(measured <= 5.6e-7 on the Moon, 2e-7 on the planets); the upstream
build fails all twelve, by up to 1.5e-3 on the Moon. G24:
`topo_sun_speed_off`, 1.6e-9 shipped, 3.4e-6 compat. Geocentric, every
planet's longitude speed is now within 1.1e-7 deg/day on the Swiss
files, the distance rates within 1.4e-8 AU/day.

## 35. A planet's light-time rate is a one-day difference along a straight line

**Severity: Correctness (3e-6 deg/day on Mercury's geocentric speed, 1.5e-4 heliocentric; 6.9e-5 AU/day on Pluto's distance rate, 5.4e-4 on a fictitious body's)**
**Where:** `sweph.c`: `app_pos_etc_plan()`, `app_pos_etc_plan_osc()`, `app_pos_etc_orbel()`, `swe_calc_pctr_r()`

The light time changes as the body and the observer move, and the
apparent velocity is v_P (1 - dtau/dt) - v_O. These paths took the
dtau/dt part as the change, over one day, of the light-time
displacement of the body moved linearly along its velocity (`xxsp`, "true
position - apparent position at time t-1"). That is the rate a day back
along a straight line, off by the curvature of a day's motion: Mercury's
geocentric speed by 3e-6 deg/day, Pluto's distance rate by 6.9e-5
AU/day, a planetocentric speed by 6.4e-7. A heliocentric or barycentric
planet had no such term at all, though its light time from the centre
changes too: 1.6e-4 deg/day on Mercury, 4e-6 on Mars (the scan, B7).

**Fix, as done here:** dtau/dt = rho^.(v_P - v_O) / (c + rho^.v_P),
exactly, from the solved light time's own vectors (`lt_rate_exact()`),
as entry 32 does for the Moon, and for every centre. Mercury 4.8e-8
deg/day geocentric and 2.4e-7 heliocentric, the outer planets'
heliocentric speeds within 1.7e-8, Pluto's distance rate 1.5e-8
AU/day, planetocentric 3.3e-8. G24 `mercury_lt_rate_off`, geocentric
and heliocentric: 1.0e-7 shipped, 1.7e-4 compat.

## 36. The topocentric observer turns at the rate per UT day, per TT day

**Severity: Correctness (3e-6 deg/day on the topocentric Moon's speed at -999; 3e-8 now)**
**Where:** `sweph.c`: `swi_get_observer()`

The site's velocity was EARTH_ROT_SPEED, the Earth's turning per day of
UT1. Positions are functions of TT, and per TT day the Earth turns by
that times dUT1/dTT = 1 - d(delta t)/dTT. Far from now delta t changes by
~1800 s a century, 5e-7 of the rotation.

**Fix, as done here:** the rate times 1 - d(delta t)/dt, the latter by a
central difference of the context's delta t over +-1 day. G24
`topo_moon_ancient_off` (-999, true positions without aberration, so
nothing else is at stake): 4.5e-7 shipped, 3.2e-6 compat; G43's
ancient dates hold it with aberration.

The G1 baseline moved on 7,035 rows for the three together: speeds only
(Pluto's distance rate by 7.3e-5 AU/day, the fictitious bodies' by up to
5.4e-4, each checked against a difference of the returned positions),
and positions by at most 3.9e-11 deg topocentric and 9.3e-10 day on the
occultation and eclipse times that iterate on a topocentric speed. G17:
twelve speed digits.
## 37. `swe_nod_aps()` carries the ecliptic's turning about the wrong centre

**Severity: Correctness (up to 1.6e-5 deg/day on the latitude and declination speeds of every geocentric node and apsis of a planet, 6e-6 on the right ascension speeds; 1.3e-4 on the Sun's perigee near perihelion)**
**Where:** `swecl.c`: `swe_nod_aps_r()`, the conversions shared by the mean and osculating points

The nodes and apsides are built in the ecliptic of date: the osculating
ones by `swi_plan_for_osc_elem()`, which turns the planet's state into
the true ecliptic of each instant, and differenced there a step apart;
the mean ones from elements referred to the mean ecliptic of date. So
their speeds already carry that ecliptic's turning, the obliquity's rate
de/dt (0, z', -y') of entry 33, taken about the HELIOCENTRIC point. The
shared conversions then rotate to the equator with the matrix alone
(leaving the turning in), go to J2000, add the Sun, subtract the
observer, apply deflection and aberration, and rotate back into the
ecliptic of date with the matrix alone (adding no turning for the
GEOCENTRIC point). What survives is de/dt x (the Earth's heliocentric
position), 1 AU long, which is nothing heliocentric or for the Moon,
whose two centres are the same point, and up to 1.6e-5 deg/day on a
planet's geocentric node or apsis -- or more where the point is near
the Earth: the Sun's osculating perigee at J2000 is 0.044 AU away and
its latitude speed was off by 1.3e-4 deg/day. Measured against five-point
differences of the function's own positions (h = 0.01 day): Mars's
osculating ascending node's latitude speed off by 1.3e-5 at 1800, the
error at every date and point matching de/dt x r_Earth to two digits;
with SEFLG_HELCTR 1e-8. With the mean method the nutation in obliquity's
rate survived alone (the points leave the MEAN ecliptic and re-enter the
TRUE one, and the mean obliquity's rate cancels), 2e-6 to 1.5e-5.

**Fix, as done here:** before the rotation to the equator,
`swi_ecl_frame_speed()` with minus the rate of the frame the points were
built in (true with nutation for the osculating points, mean for the mean
ones, mean under SEFLG_NONUT); after the rotation back into the ecliptic,
plus the rate of the output frame. Latitude and declination speeds now
within 9e-8 deg/day. Not under SEFLG_J2000, whose output is left as
upstream has it: that path does not set SEFLG_NONUT as `swe_calc()`'s
`plaus_iflag()` would, so it rotates the ecliptic-of-date points by the
J2000 obliquity and applies the nutation to J2000 coordinates; the Moon's
mean node from `swe_nod_aps()` under SEFLG_J2000 is 7.8" in longitude
and 6.9" in latitude from `swe_calc(SE_MEAN_NODE)` under the same flag,
where of date they agree to 1e-6 deg. A position defect, not fixed here;
fixed as entry 47 (4.6e-9" now).

## 38. `swe_nod_aps()` returns the mean apsides' latitude as its speed

**Severity: Correctness (the mean perihelion's and aphelion's latitude speed is the latitude itself, 1.1-3.9 deg/day against a true 1e-6 to 1e-2; 0.01-0.1 deg/day on their geocentric longitude speeds through it)**
**Where:** `swecl.c`: `swe_nod_aps_r()`, the mean apsides

The mean perihelion's speed is taken from an aux. position a day on:
`xpe[3]` is set to the argument of perihelion plus its daily rate, both
are turned from the orbital plane into the ecliptic by `swe_cotrans()`,
and the longitude speed is their difference. The latitude's difference
is never taken: `xpe[4]`, the aux. position's latitude, goes out as the
latitude speed, and the aphelion gets its negative. Heliocentric, Mars's
mean perihelion had a latitude speed of -1.8 deg/day, its latitude; the
Moon's mean perigee -3.4 deg/day. Through the cartesian state and the
change of centre the geocentric speeds carry it too: Mars's mean
perihelion's longitude speed was off by 0.014 deg/day, Mercury's by 0.047,
and the distance rates by up to 1.4e-3 AU/day.

**Fix, as done here:** `xpe[4] -= xpe[1]` where the longitude speed is
formed, the same one-day difference. The mean apsides' latitude speeds are
now within 9e-8 deg/day of a five-point difference, geocentric and
heliocentric; the Moon's mean perigee 0.0110 deg/day.

G42 `tests/nodspeed.c`, for both: the longitude and latitude (right
ascension and declination) speeds of the four points of Mars, Venus and
Jupiter, osculating and mean, ecliptic and equatorial, against five-point
differences at seven dates -200..+200 years; latitude and declination
within 2.5e-7 deg/day, longitude and right ascension within 1e-6; the
upstream build fails every case. G24 asserts both ways: Mars's osculating
node's latitude speed (3.9e-8 shipped, 1.2e-5 compat) and Mars's mean
perihelion's heliocentric latitude speed (1.4e-9 shipped, 1.77 compat).
G1: 76 rows, every `nod_aps`/`nodaps` row with speeds but the Earth's
ten zeroed nodes, speeds only -- positions unchanged to the bit. The 34 mean rows by bug 38 (latitude
speeds by up to 3.4 deg/day, longitude by 0.047, distance by 1.4e-3) and
the Moon's mean nodes' latitude speed by 4.7e-6 (bug 37); the 42
osculating rows by bug 37 (latitude speeds by up to 1.3e-4 on the Sun's
perigee and 8.2e-6 elsewhere, longitude and distance speeds by at most
3e-8), the geocentric Moon's in the last bits. G17 unchanged.

## 39. The light deflection's speed is rough behind the Sun's disc

**Severity: Correctness (1.5e-5 deg/day of noise on a planet's speed while it is behind the Sun, between instants 1e-9 day apart)**
**Where:** `sweph.c`: `swi_deflect_light()`

The deflection's rate is the change of the deflection over 5e-7 day,
from u (the planet), e (Sun-Earth) and q (Sun-planet), unit vectors.
Two of its quantities cancel exactly where the deflection is largest, a
planet behind the disc: the elongation's sine as sqrt(1 - (u.e)^2), u.e ~
1, and the denominator 1 + q.e, q.e ~ -1. At 0.02 of the Sun's radius
from its centre they are ~1e-4 and ~5e-9 and carry ~1e-12 and ~1e-16 of
rounding; over the speed's step they change by only ~1e-8 and ~1e-13 of
themselves, so the rate came out rough: Venus at its 2016-06-06
conjunction 1.5e-5 deg/day from one instant to the next, Saturn 4e-6,
Jupiter 3e-7. Found when it moved G1's `cov:meff[venus]` row past G1b's
1e-5 tolerance on arm64.

**Fix, as done here:** the sine as |u x e| and the denominator as
|q + e|^2 / 2 (the same for unit vectors), both formed from the small
quantity itself. The spread is now <= 2.9e-8 deg/day and the speed within
2.2e-7 of a central difference. Positions move by the rounding they
carried, <= 3.1e-13 deg.

G44 `tests/deflspeed.c`: the five conjunctions G1 uses for meff(), speeds
at seven instants 1e-9 day apart within 1e-7 of each other and within
1e-6 of a central difference; the upstream build fails on Venus, Saturn,
Jupiter and (light-time rate, entry 35) Mercury. G24
`defl_rough_off`: 6.8e-9 shipped, 1.5e-5 compat. G1: the five meff rows.

## 40. Whole-sign, 0-Aries, Porphyry and Krusinski cusp speeds are not the cusps' rate

**Severity: Correctness (hundreds to 1600 deg/day on a returned cusp speed)**
**Where:** `swehouse.c`: `CalcH()` cases `W`, `N`, `O`, `U`

`swe_houses_ex2()` returns cusp speeds next to the cusps. `CalcH()` seeds
cusps 1 and 10 with the Asc's and MC's speeds and each system overwrites
what differs. Whole signs (`W`) and 0 Aries (`N`) never did: their cusps
stand still within a sign, but 1 and 7 kept the Asc's speed and 4 and 10
the MC's, 326-555 deg/day at 47N and 1270 at 70N where the truth is 0
(sidereal whole signs, computed as `E`, got the Asc's speed on all 12;
entry 43). Porphyry's cusps 11 and 12 are MC + acmc / 3 and MC + 2 acmc /
3, but their speeds were built on the Asc's speed instead of the MC's, off
by the difference of the two at every latitude, 1600 deg/day at 70N; the
Koch, Placidus and Gauquelin fallbacks inside the polar circle share the
code. Krusinski (`U`) never took a speed: cusps 2, 3, 11, 12 and their
opposites came back 0 (1500 deg/day at 70N).

**Fix, as done here:** `W` and `N` speeds 0; Porphyry's 11 and 12 from the
MC's speed; Krusinski differenced in ARMC as the other non-analytic
systems are (entry 41). Positions do not move.

G45 `tests/housespeed.c` (with 41-43). G24 `house_speed_off(0)`: Koch
cusp 11 and whole-sign cusp 1 at 47N against a five-point difference,
2.0e-10 deg/day shipped, 265 compat.

## 41. Koch, Placidus and Gauquelin cusp speeds leave out the moving semi-arc

**Severity: Correctness (200 deg/day on Koch cusps, 3.2-32 on Placidus at 47-60N; 1e-4 to 0.1 on the differenced systems)**
**Where:** `swehouse.c`: `CalcH()` cases `K`, `P` (default), `G`; the interpolation in `swe_houses_armc_ex2_r()`

Each analytic speed is `AscDash()`, the rate of Asc1(x, f) in x with the
pole height f held still. For Koch the cusps are Asc1(th + k 30 + j ad3,
fi), and ad3, a third of the MC's ascensional difference, moves with the
MC; its rate was left out: lib 721.4 against 521.7 deg/day on cusp 11 at
47N, 1375 for 776 at 60N. A Placidus (or Gauquelin) cusp is the fixed
point of C = Asc1(ra, f(C)), its pole height set by the cusp's own
declination, so the pole height moves with the cusp: 3.2 deg/day at 47N,
32 at 60N, 54 for Gauquelin sectors. The systems without an analytic
speed (`B F I i L M Q S X Y`) were differenced over one second of time by
three points: the h^2 term of a fast cusp is 3.7e-7 of its rate at 70N
(1e-4 deg/day), Q at 85N 0.1 deg/day, APC at 70S 0.02, Sunshine 9e-3.

**Fix, as done here:** Koch's speed carries d(ad3)/d(ARMC) by the chain
through sin a = sin MC sin e / cos fi, c = atan(tan fi / cos a) and
sin(3 ad3) = sin c sin a. Placidus and Gauquelin: dC = A_x / (1 - A_f
f'(C)), A_x and A_f the partials of Asc1 in its arguments and f'(C) the
pole height's rate in the cusp (`PlacDer()`). The differenced systems,
and Krusinski, by five points over 2^-8 deg of ARMC (Makransky's
Sunshine, whose cusps are rough by 2e-10 deg, 2^-6), a side that jumps or
falls back to Porphyry dropped for a one-sided difference. In ARMC alone,
against a ladder of differences: Koch 4e-11 of the rate, the differenced
systems <= 2e-9 (Q at 85N 4e-8), Placidus and Gauquelin within 1e-9 of
the converged cusp's rate. The Placidus iteration stops at 1/360000 deg
and leaves the cusps 3.5e-8 deg off the fixed point; differences of the
returned cusps carry that error's rate, 2e-8 of the speed at 60N (1.1e-5
deg/day), which tightening the iteration removes (entry 59). Positions do
not move.

G45; G24 `house_speed_off(0)` (Koch cusp 11).

## 42. House speeds turn the ARMC at a constant rate and hold the obliquity still

**Severity: Correctness (6e-5 deg/day on the ARMC, 1.5e-4 on the Asc and 4.8e-4 on a Regiomontanus cusp at 70N; 2.4 deg/day on Sunshine cusps)**
**Where:** `swehouse.c`: `swe_houses_ex2_r()`, `ARMCS`, `AscDash()`

Every speed was the cusp's rate in ARMC times `ARMCS`, 360.9856473350
deg/day at every epoch, the sidereal days of a fixed solar year. The ARMC
`swe_houses_ex2()` computes the cusps from is the library's apparent
sidereal time: the equation of the equinoxes moves it at -3.9e-5..+6.2e-5
deg/day about the mean rate, and the mean is off `ARMCS` by 3e-8 deg/day
at 2000, -9e-7 at -3000, +1.3e-6 at 12000; a cusp n times as fast as the
ARMC carries n times that. The obliquity (nutation's 1.3e-5 deg/day) was
held still, and so was the Sun's declination of Sunshine houses (up to 0.4
deg/day, 2.4 deg/day on a cusp at 70S).

**Fix, as done here:** the ARMC's and true obliquity's rates are
differences of the library's own values 2^-6 day of UT, the time
argument, either side (three points; nutation's fortnightly term leaves
2e-10 deg/day, the ARMC's rounding 2e-8 at -13000); the cusp speeds in
ARMC are scaled to that rate, and the partials in obliquity and (Sunshine)
declination, differences of the cusps over 2^-10 deg, times their rates
are added. The declination's rate is swe_calc_ut()'s, per TT day, turned
per UT day. Speeds are per day of UT, as differences of the cusps over
the time argument are. `swe_houses_armc_ex2()` has no date and still gives
its speeds at `ARMCS`. A call with speeds evaluates the frame twice more
(deltat, nutation, sidereal time) and the houses two or four more times:
17-31 us against 11-21 without speeds (-O2), upstream's speeds cost ~0.5
us. Positions do not move.

G45; G24 `house_speed_off(1)` (with 43): equal houses' Asc under Lahiri
at 47N, 4.6e-11 deg/day shipped, 3.9e-5 compat.

## 43. Sidereal house speeds are the tropical ones

**Severity: Correctness (3.8-8.5e-5 deg/day on every sidereal cusp; whole signs 326-555 deg/day)**
**Where:** `swehouse.c`: `sidereal_houses_trad()`, `sidereal_houses_ecl_t0()`, `sidereal_houses_ssypl()`

The sidereal cusps are the tropical ones less the ayanamsa (or, on the
ecliptic of t0 or the solar-system plane, houses of an auxiliary ARMC and
obliquity less the auxiliary vernal point's distance), all moving; the
speeds were passed through from the tropical computation without any of
those rates. Sidereal whole signs, computed as equal houses, kept the
Asc's speed on all 12 cusps.

**Fix, as done here:** the ayanamsa's rate, a difference over the frame's
TT either side, is subtracted from the cusp and ascmc speeds; on the
ecliptic of t0 and the solar-system plane the auxiliary obliquity, ARMC
offset and vernal-point distance (steps 1, 2 and 5, now `ecl_t0_aux()` and
`ssypl_aux()`, the same arithmetic) are evaluated either side for their
rates. Whole signs and 0 Aries 0. Positions do not move (checked bitwise,
both builds). Near t0 itself (J2000 for `SE_SIDM_J2000`) the auxiliary
vernal point's distance is an acos near 1 and rough at 1e-8 deg, in the
positions as well.

G45; G24 `house_speed_off(1)`.

## 44. The interpolated apsides depend on SEFLG_SPEED, and x has no light time

**Severity: Correctness (the interpolated perigee 111 mas apart with and without the speed flag, the apogee 13)**
**Where:** `sweph.c`: `intp_apsides()`

The apsis is moved back along its light time as `xx[i] -= dt * xx[i+3]`:
the speed, zero unless SEFLG_SPEED is set, and `i` from 1, so the x
coordinate never had it. Without the flag the position was geometric,
with it light-time corrected in y and z only: tau v, tau ~1.2 s and v up
to 2.3 deg/day, 111 mas on the perigee (scan C, every ephemeris and flag
set) and 13 on the apogee.

The intended position is the light-time corrected one: the code gates the
step on SEFLG_TRUEPOS, and the osculating apogee, which the interpolated
one smooths, is built from the Moon at t - tau.

**Fix, as done here:** the apsis is computed at t - tau, as the osculating
one is, for every flag set but TRUEPOS, with and without SEFLG_SPEED, so
the position is the same either way, to the bit. tau is the apsis's
distance over c, taken from the first passes of the iteration at t
(`swi_intp_apsides_est()`): the distance is stationary at the apsis, so
two passes put the perigee's within 4 km and one the apogee's within 1
km (20000 dates, -10000..+12000), at most 0.0013 mas of position. The
speed is the difference over +-2^-8 day about t - tau (bug 45), times 1 -
dtau/dt, dtau/dt = rdot / c.

Cost, in instructions per `swe_calc()` (`perf stat -e instructions:u`,
gcc -O2 with LTO, 2000 calls less a run of none; timings on the loaded
machine were not reliable). An apsis is lunar-series evaluations,
~42,000 instructions each: upstream takes 18 (perigee) and 15 (apogee),
three times that with SEFLG_SPEED. The estimate of tau adds 7 to the
perigee (two passes) and 4 to the apogee (one); until entry 72 the
perigee's took one pass too, 4. Entry 45 adds 2 to the perigee, entry 56
2 more and 1 to the apogee. Shipped (e8fd2ac, this fix and entry 45;
c8e3710, entry 56; then entry 72), against the compat build, at instants
0.01 day apart:

| | upstream | e8fd2ac | c8e3710 | entry 72 | lunar evaluations |
|---|---|---|---|---|---|
| perigee | 0.78e6 | 1.01e6 (1.30x) | 1.10e6 (1.41x) | 1.22e6 (1.57x) | 18 -> 24 -> 26 -> 29 |
| apogee | 0.66e6 | 0.81e6 (1.23x) | 0.85e6 (1.29x) | 0.85e6 (1.29x) | 15 -> 19 -> 20 -> 20 |
| perigee, SEFLG_SPEED | 2.31e6 | 2.70e6 (1.17x) | 2.95e6 (1.28x) | 3.08e6 (1.33x) | 54 -> 64 -> 70 -> 73 |
| apogee, SEFLG_SPEED | 1.94e6 | 2.07e6 (1.07x) | 2.20e6 (1.13x) | 2.20e6 (1.14x) | 45 -> 49 -> 52 -> 52 |

At instants a day or more apart every `swe_calc()` of this build, of any
body, also evaluates a node of its default nutation (IAU 2000A on a
grid, ~0.33e6 instructions a node; upstream's whole mean-apogee call,
2000B included, is 0.03e6), which is not the apsides' work: there the
shipped perigee is 1.56e6 against 0.78e6 at daily steps (1.99x; 1.43e6,
1.83x, at c8e3710), 1.68e6 at instants 1.37 day apart (2.15x; 1.55e6 at
c8e3710, 1.47e6 at e8fd2ac), the apogee 1.18e6 (1.80x) and 1.31e6
(1.99x). Best of five timings, 0.01 day apart, c8e3710 against compat:
perigee 87 against 65 us, apogee 66 against 55. Nothing in the rise is
work done twice: it is the evaluation count above, each one the price of
a fix. A first version took X(t) - tau V(t), which needs the velocity,
three apsides (60 evaluations), on every call: 3.2x without the flag
(2.52e6).

G46 `tests/intpaps.c` (a), (b): the position with and without the flag,
six flag sets, identical (upstream 140 mas, angle plus distance); the
cartesian position less the TRUEPOS one is -tau V within 3.4e-3 mas (the
frame's turning over tau; upstream 120 mas). G24 `intp_speedflag_off`:
0 shipped, 107 mas compat. Baselines: every interpolated-apsis position
without SEFLG_SPEED moves by tau V, with it by tau V_x.

## 45. The interpolated apsides are rough at the JD's resolution, and so are their speeds

**Severity: Correctness (the perigee jumps 1.3 mas between consecutive JDs, its SEFLG_SPEED3 speed is off by 1.5e-3 deg/day, its default speed by 1.1e-4)**
**Where:** `swemmoon.c`: `swi_intp_apsides()`; `sweph.c`: `intp_apsides()`, `swe_calc_r()`

`swi_intp_apsides()` finds the apsis along a line in the lunar arguments
as the vertex of parabolas through three distances dd apart, dd = 5 deg,
0.5, ... 1.8" (0.18" for the perigee's last, unused, pass). At dd = 18"
and 1.8" the parabola's slope and curvature are ~1e-6 km, read from a
distance of 3.6e5 km held to ~1e-10 km; the step is still 1-3" there,
because the iteration converges only linearly (by ~0.15 a pass on some
dates), and carried ~1e-4 of itself in noise. The perigee's longitude
jumped 1.3 mas between consecutive JDs at 2100 (sescan A, S = 351), 0.14
at -3000; the apogee 0.023 mas. The speed was the difference over +-0.1
day, off by its h^2 term (1.1e-4 deg/day on the perigee, 1.7e-6 the
apogee), and SEFLG_SPEED3's over +-8.64 s of the jitter (1.5e-3, 3.7e-5).

**Fix, as done here:** below dd = 180" the step is the Newton step
-r'/r'' at the iteration's centre from five points 180" apart (the
slope's h^4 term ~1e-14 rad, its noise ~1e-11 rad); the parabolas above
are unchanged, and so is the number of passes, so the result is
upstream's to its noise: within 0.17 mas, smoothly (the 18" parabola's
own h^2 bias). The unused last pass computes only its centre. The
perigee is smooth to 1.8e-3 mas, the apogee to 1.1e-3 (S <= 1.5 on
lon). Both speeds difference over +-2^-8 day, where the h^2 term and the
noise over 2h balance: within 2.1e-7 deg/day of a five-point difference
(SEFLG_SPEED and SEFLG_SPEED3; 9e-13 AU/day on the distance).

Not changed: the iteration stops after five (perigee) or four (apogee)
passes, not at its fixed point, which lies up to 1.08" away for the
perigee (2.4 mas the apogee) over -10000..+12000; converging would move
the positions by that much. SEFLG_J2000's latitude speed is off by up to
4.6e-7 deg/day (-3000), from the J2000 conversion shared with the mean
apogee (7e-7 at 12000). Both since: entry 56.

G46 `tests/intpaps.c` (c), (d): 64 consecutive JDs on a line within 0.01
mas (1.8e-3; upstream 1.3), the speeds within 1e-6 deg/day of a
five-point difference at multiples of 2^-8 day (2.1e-7; upstream 1.8e-3
on the Lahiri SPEED3 perigee). G24 `intp_rough_off`: 1.8e-3 mas shipped,
1.2 compat. Baselines: interpolated-apsis positions move by <= 0.17 mas
besides entry 44's light time, their speeds by up to 1.1e-4 deg/day.

## 46. `swe_nod_aps()`'s positions depend on SEFLG_SPEED

**Severity: Correctness (6-35 mas on Mars's descending node, perihelion and aphelion, the same on every planet's; 10 mas on the Sun's mean aphelion, 300 on its osculating focal point, 660 on Venus's; only with SEFLG_SPEED)**
**Where:** `swecl.c`: `swe_nod_aps_r()`, the apparent speed's aberration term

With SEFLG_SPEED and aberration, each point's speed gets the change of
the observer's velocity over its light time dt: the body is computed at
tjd - dt, which leaves the Sun and the Earth in the save area at tjd -
dt, the observer is read from there, and then the save area is
"restored" by computing the Sun at tjd with iflg0, the flags the
function's internal calls use. Those carry SEFLG_HELCTR for every body
but the Moon, and a heliocentric Sun is zeros, returned before anything
is computed; for the Moon (heliocentric or barycentric points), whose
iflg0 is geocentric, the call finds the Sun of the setup call in
`swe_calc()`'s save area and returns it as it is. Either way the
barycentric Sun and the Earth stayed at tjd - dt of the point before:
the first point (the ascending node; the Sun's perihelion) is right, and
the deflection and change of centre of the three after it used the
wrong Earth. Without SEFLG_SPEED nothing of this runs. Found by sescan's
scan C (`--only nod_aps --scan C`): Mars 1.5-30 mas equivalent with the
Swiss files at -3000 and with JPL at 1800. Measured with and without
SEFLG_SPEED over Sun..Pluto and the Moon, the four methods, geocentric,
heliocentric, barycentric, topocentric, equatorial and J2000: Mars's
mean descending node, perihelion and aphelion 30, 27 and 18 mas apart
(equatorial 32, 35, 20); Mercury's 20, 10, 19; the Sun's mean aphelion
10, its osculating focal point 307; Venus's focal point, heliocentric,
664. Also, osculating, the middle of the three instants was tjd - dt +
dt, tjd rounded twice, against tjd itself without SEFLG_SPEED (1e-8 mas).

**Fix, as done here:** the restore repeats the setup call -- the body
itself with iflg0 (topocentric but for the Earth, see the F6 note
there), or the Sun for the Moon's heliocentric points -- after
`swi_force_app_pos_etc()`, so that the save area is what the first point
saw; the middle instant is tjd. Positions with and without SEFLG_SPEED
now agree to <= 8e-6 mas (the Moon's osculating points; <= 6e-7 for the
rest; 0 under SEFLG_J2000), the rounding of `swi_cartpol_sp()`'s and
`swi_precess_speed()`'s own formulas for the position; sescan's scan C
for `nod_aps` now worst 7e-6 mas.

## 47. `swe_nod_aps()` under SEFLG_J2000 is not the J2000 frame

**Severity: Correctness (7.9" on the Moon's mean node in 2026, 148" in latitude in 2326, against `swe_calc()`; 3.2e-5 deg/day on Mars's osculating points' declination speeds)**
**Where:** `swecl.c`: `swe_nod_aps_r()`, the conversions shared by the mean and osculating points

`swe_calc()`'s `plaus_iflag()` sets SEFLG_NONUT with SEFLG_J2000 ("if
J2000, then no nutation"); this function did not, and it takes one
obliquity, `oe`, for both ends of the conversion: the output's,
J2000's under SEFLG_J2000. So the points, built in the ecliptic of date,
were turned to the equator by the obliquity of J2000, the nutation was
taken out on the way in (osculating) and applied to J2000 coordinates on
the way out. The Moon's mean node from `swe_nod_aps()` was 7.9" in
longitude and 6.9" in latitude from `swe_calc(SE_MEAN_NODE)` under the
same flag on 2026-10-24 (of date they agree to 5e-9"), 15" and 148" in
2326; SEFLG_J2000|SEFLG_NONUT was off by as much, the obliquity alone.
The speeds had none of the frame's turning: entry 37's correction is for
the ecliptic of date at both ends, skipped here, and
`swi_precess_speed()`'s general precession in longitude about the J2000
ecliptic is not the whole of the mean equator of date's turning. Against
five-point differences, Mars's osculating points' declination speeds
were off by 3.2e-5 deg/day, Jupiter's 2.2e-5.

**Fix, as done here:** SEFLG_NONUT with SEFLG_J2000, as `swe_calc()`
does (the sidereal case beside it, G30, already did), so the points are
built in the mean ecliptic of date; then `nodaps_ecl_to_j2000()`: the
mean obliquity of date to the equator, precession to J2000, and on the
velocity the frame's turning as R'(tjd) x, the whole rotation's central
difference over a day either side. The output's obliquity stays J2000's.
The Moon's mean node and mean apogee now agree with `swe_calc()` under
SEFLG_J2000 to 4.6e-9", ecliptic and equatorial, -300..+300 years
(sidereal and SEFLG_NONUT with J2000 too); the angle between the
osculating ones and `swe_calc()`'s, made another way, is the same in
J2000 as of date to 3e-10"; and the planets' J2000 speeds are within
1.4e-7 deg/day (latitude, declination) and 3.6e-7 (longitude, right
ascension) of five-point differences, as of date.

G47 `tests/nodaps2.c`, for both: (A) positions with and without
SEFLG_SPEED, Sun..Pluto, the Moon and the Earth, four methods, six flag
sets, five dates -3000..2100 and Mars with de200.eph at 1800, within
1e-4 mas; (B) the Moon's mean points against `swe_calc()` under J2000
within 1e-7", the osculating ones and the planets' angles between the
points, J2000 against of date, within 1e-6"; (C) J2000 speeds of Mars,
Venus, Jupiter and the Moon's mean nodes against five-point differences
at binary-fraction instants, latitude/declination within 2.5e-7,
longitude/right ascension within 1e-6 deg/day. The upstream build fails
34 of its 35 rows. G24 asserts both ways: Mars's osculating points with
and without SEFLG_SPEED (4.1e-7 mas shipped, 17 mas compat) and the
Moon's mean node under J2000 against `swe_calc()` (2.7e-9" shipped,
7.9" compat). G1: 36 rows, all of them bug 46 -- every `nodaps` row with
SEFLG_SPEED that is not a call's first point and runs the aberration
rate (the Earth's, Mercury's and Pluto's dsc/peri/aphe over the five
methods, Mars's `_ut` aphelion): positions by 0.008-18 mas, each now
within 2e-7 mas of the same call without SEFLG_SPEED (before: up to 18),
and speeds by up to 1.3e-7 deg/day through the corrected Earth. No G1
row is under SEFLG_J2000. G17 unchanged.

Found along the way: `swe_nod_aps()`'s topocentric speeds took the
aberration rate upstream's way, from the call at tjd - dt, 7e-6 deg/day
off on Mercury's points, 3e-4 on Pluto's; fixed under entry 34. Not
fixed: the Moon's mean perigee's latitude speed is a one-day forward
difference (entry 38), 2e-5 deg/day from the rate.

## 48. A fictitious body's speed is a fixed ellipse's, not its elements'

**Severity: Correctness (2.9e-2 deg/day on Vulcan's heliocentric longitude speed, 1.3e-2 on Waldemath's geocentric one, 9.4e-6 on seorbel.txt's Mercury set, 3.8e-5 on every set referred to the equinox of date)**
**Where:** `swemplan.c`: `swi_osc_el_plan()`, `read_elements_file()`, `check_t_terms()`

`swi_osc_el_plan()` took the speed as the two-body motion of a fixed
ellipse, K / sqrt(a) along the orbit with K Gauss's constant, while the
position follows `seorbel.txt`'s polynomials in T. Three things moved
that the speed never saw:

1. **The mean anomaly's own rate.** A mean anomaly with T terms is the
   polynomial itself (the epoch is set to tjd, so the daily motion adds
   nothing); Vulcan's 252.8987988 + 707550.7341 T advances 19.3717
   deg/day, against the 19.3428 that a = 0.13744 AU gives.
2. **The other elements' rates.** Semi-axis, eccentricity, perihelion,
   node and inclination in T: Vulcan's node and perihelion turn at 1670
   deg/cy, the Mercury set's elements of date at planetary rates, and a
   semi-axis in T changes the daily motion that M = M0 + (t - t0) n(a)
   accumulates.
3. **An equinox of date's turning.** JDATE elements are referred to the
   ecliptic and equinox of t, which the position rotates to J2000 with
   the obliquity and precession of t; the speed was rotated with the
   same matrices and nothing for their rate, 3.8e-5 deg/day (50"/yr) on
   Proserpina, Kore, Selena and VEA, which otherwise do not move at all.

Even a fixed ellipse was off for an Earth-centred set: the speed's
KGAUSS_GEO is 7.6e-8 off the 0.9856076686 / sqrt(SUN_EARTH_MRAT) the
position's daily motion uses, 4.8e-7 deg/day on Selena. Heliocentric
J2000 longitude speeds against five-point differences: Vulcan 2.9e-2,
Waldemath 6.9e-5 (1.3e-2 geocentric), Mercury of date 9.4e-6, the JDATE
sets 3.8e-5; the built-in table's sets and the file's fixed ellipses
(Cupido..Pickering, Nibiru, Harrington, Christ Comet, Planet 9, the
Halleys, 2015 RR245) were already right.

**Fix, as done here:** the speed is the derivative of the position as
it is computed. `check_t_terms()` returns the polynomial's derivative
with its value (each term's factor carried with its derivative through
the products; the value is formed exactly as before), and
`read_elements_file()` passes the six rates out, with flags for a mean
anomaly in T and an equinox of date. Then dM/dt is the polynomial's,
or n - 1.5 n (da/dt) / a (t - t0); Kepler's equation differentiated,
dE/dt = (dM/dt + sin E de/dt) / (1 - e cos E); the orbit-plane state's
derivative through a and e; the Gaussian vectors' through the
perihelion (dP/dw = Q, dQ/dw = -P), the node and the inclination; and
for JDATE the frame's turning applied to the ecliptic position, a
central difference of the same rotation (obliquity, then precession to
J2000) over +-1 day, exact to rounding for smooth polynomials and
long-period series. Positions are unchanged.

Every set of `ephe/seorbel.txt` (40..69), heliocentric and geocentric,
geometric, J2000 ecliptic and equator, nine dates -400..+400 years, is
now within 2.2e-8 deg/day of five-point differences over 2^-6 day (all
but Vulcan, Waldemath and Mercury within 4e-10), most of it the
reference's own noise: Vulcan's mean anomaly reaches 3e6 degrees at T =
5, rounded to 5e-10 degree, which moves each position along the orbit
(fixed as entry 81 part 2).
Apparent positions are the body at t - tau, not a binary instant, and
their differences carried the instant's rounding: over 2^-7 day 3.4e-7
deg/day on Vulcan heliocentric and 1e-6 to 5.5e-6 on Waldemath (an
earlier draft said 3e-7; measured with too few dates), fixed as entry 81;
ecliptic-of-date output keeps the ~5e-8 deg/day latitude-speed residual
every planet has there.

G48 `tests/orbelspeed.c`: Vulcan, Waldemath, Selena, Mercury of date,
Proserpina, Harrington and a set with T terms on everything but the
mean anomaly, heliocentric and geocentric, seven dates -300..+300,
within 1e-7 deg/day (measured <= 8.4e-9) and the distance rate within
1e-9 of the distance per day (<= 1.6e-11); the upstream build fails
every set but Harrington, by 2.1e-6 to 0.68 deg/day. G24
`orbel_speed_off`: 3.8e-9 shipped, 2.9e-2 compat. G1: the 51
`cov:fict` rows move in their speed fields only -- 40..54 by <= 8.0e-15
deg/day (the daily-motion constant in place of KGAUSS, 1.4e-12 of
it), Vulcan (55) by up to 2.8e-3 and Selena (56) by up to 4.8e-7, both
now within 2.6e-8 and 7.8e-8 of five-point differences (were 2.8e-3
and 4.9e-7). G17 unchanged.

## 49. A fixed star's speeds are not the rates of its position

**Severity: Correctness (2.3e-3 AU/day on Spica's distance rate; 3e-8 deg/day on every latitude speed of date, every body; 3.8e-5 deg/day on `swe_fixstar()`'s sidereal speed)**
**Where:** `sweph.c`: `swi_deflect_light()`, `swi_aberr_light_ex()`, `swi_nutate()`, `swi_precess_speed()`, `fixstar_calc_from_struct()`, `swi_fixstar_calc_from_record()`

A scan (`tests/scan/sescan ephe --only fixstar2`) found the geocentric
distance rate off a five-point difference of the returned distances by
3.0e-3 AU/day on Spica and 2.8e-4 on Aldebaran, under SEFLG_NOABERR and
J2000|NONUT too, clean heliocentric. Taken apart against five-point
differences over 2^-8 day at binary-fraction instants, nine stars, five
dates, ten flag sets, there were four causes:

1. The deflection's rate was (xx2 - xx) at t less the same at t + 5e-7
   day, over 5e-7: differences of two vectors of the star's distance,
   1.6e7 AU, whose last bits (2e-9 AU) come out as 4e-3 AU/day of noise,
   1e-8 deg/day in angle. The aberration's rate, A(x(t)) - A(x(t - dt))
   over dt = 1e-5, the same at 2e-4 AU/day; the nutation's, (N(t) x -
   N(t - h) x) / h, h = 1e-4, 2e-5 AU/day. In angle these are 1e-12 of
   the distance per day and below 1e-8 deg/day; the distance rate is where
   they show. The deflection's is also on every planet: up to 2.9e-8
   deg/day of roughness (G44's residual spread) and ~1e-7 on a
   topocentric planet's speed (G43's Venus 1.3e-7, now 2e-8).
2. `swi_precess_speed()` took the precession's rate as the general
   precession in longitude about the ecliptic pole of date. The equator of
   date turns about a different axis: what that leaves out is the
   planetary precession, chi_A' sin(eps) = 4.2"/cy about the ecliptic's
   90-degree point, so every latitude speed of date was off by up to 3.2e-8
   deg/day and a polar star's longitude speed by 7e-8. This is the 5.5e-8
   entry 33 left on the planets' latitude speeds; it is in every body's
   path.
3. The aberration rate was a backward difference over 1e-5 day: the rate
   at t - dt/2, which for the site's diurnal aberration (0.32", turning
   once a day) is dt/2 w^2 A = 1.8e-8 deg/day off. And dt was the nominal
   1e-5 where tjd - dt is rounded to 4.7e-10 day, 2.3e-5 of the step:
   2.2e-9 deg/day of the annual rate, 1.3e-8 of the diurnal. Topocentric
   speeds were off by up to 2.9e-8 (delta Dor, Polaris Australis).
4. `swi_fixstar_calc_from_record()` (`swe_fixstar()`, and the stars
   `swe_fixstar2()` does not keep) took the ayanamsa off a sidereal
   longitude and not its rate off the speed: the tropical speed, 3.8e-5
   deg/day too fast. `fixstar_calc_from_struct()` takes both.

**Fix, as done here:** (1) the displacements formed directly: the
deflection as ru g1/g2 (uq e - ue q) at both instants; the aberration's
shift A(x) - x as ((b_1 - 1 - f1) x + f2 ru v) / (1 + f1), b_1 - 1 =
-v^2 / (1 + b_1) (`aberr_shift()`); the nutation's rate from the angles'
own, w = deps' e1 + dpsi' (0, -sin eps_t, cos eps_t) in the true frame and
the velocity gains w x x (backward, -(N^T w) x x). Positions are
unchanged bit for bit. (2) dP/dt x0 itself, x0 the position back in the
frame precessed from. For the Vondrak 2011 series, the default for short
and long terms, the matrix and its rate come from the series and their
term-by-term derivatives in one pass (`swi_precess_matrix_rate()`): the
matrix is `swi_precess()`'s to the bit, the rate within 1.6e-16 rad/day of
a five-point difference over 4 days (-13000..+17000, that difference's
own rounding). That is cheaper than upstream's rotation plus its two
`swi_ldp_peps()` calls: `bench` calc-swieph 0.74 -> 0.70 s, calc-moseph
1.22 -> 1.20. Any other model takes (P(t + 1) x0 - P(t - 1) x0) / 2.
Central, because P^T P is not I to the last bits and a one-sided
difference against x picks that up (5e-12 / h of the rate). A first
version differenced for every model, three precessions more a call: +7%
on calc-swieph. (3) for stars, `aberr_light_central()`: the shift's
change from t - dm to t + dp, dm and dp the exact differences of the
instants, the observer's velocity at t + dp the Earth's carried on
linearly from t - dm, t and the site read there. (4) the ayanamsa's rate
taken with `swi_get_ayanamsa_with_speed()`.

Angular speeds are now within 6.6e-10 deg/day of the positions' rate
(longitude times cos(latitude)), 2e-11 under J2000|NONUT, sidereal and
heliocentric alike, both functions; the distance rate within 1.7e-13 of
the distance per day (the reference's own noise; 5e-7 AU/day on Spica);
XYZ within 1.1e-11 of the distance per day. G41's planets' latitude
speeds go to 1.6e-9 (Mars, from 2.8e-8) and 7.5e-10 (Chiron, from 4e-8).

Not done here (done in entry 58): the 6.5e-10 deg/day left in every
longitude speed of date. Also seen and left
alone: the topocentric deflection reads the site from `ctx->topd`, which
the star's path fills NO_SAVE, so it is a stale or empty site (an Earth
radius in e, 4e-5 of a 4 mas deflection).

G49 `tests/starspeed.c`: nine stars (bright, Barnard's Star, Polaris,
the galactic centre, 42 Dra and delta Dor near the ecliptic poles), five
dates, ten flag sets, both functions: 36 checks, angles within 1e-8
deg/day, distance rate within 1e-12 of the distance per day, XYZ within
1.7e-10; the upstream build fails all 36. G24 `star_dist_rate_off`:
Spica's distance rate 5.1e-7 AU/day shipped, 1.8e-3 compat. G1: 8170
rows, re-baselined with the other follow-ups (the re-baseline commit
classifies them): every speed of date by up to 2e-7 deg/day in latitude
and 1e-7 in longitude, from (2); positions by at most 1.1e-13 deg, from
(1)'s reordered arithmetic. G17 unchanged.

## 50. The Moshier series round their arguments at more time than a JD resolves

**Severity: Correctness (Mercury 3.9e-3 mas, the Moon 0.011 mas between consecutive JDs at -3000; S ~ 3-8)**
**Where:** `swemplan.c`, `swi_moshplan2()`; `swemmoon.c`, `swi_moshmoon2()`, `mean_elements()`

The planets' arguments were `freqs[i] * T` reduced modulo 360 degrees,
T = (J - J2000) / 3652500, and Mercury's mean longitude its polynomial's
`53810162857.6 * T` by Horner: at -3000 numbers of 2.7e10", each of three
roundings (J - J2000, the division, the product) ~2e-6" and ~1.3e-10 day
of the planet's motion, against the 1.2e-10 day a JD resolves there. The
Moon's T, / 36525, rounds at 7e-15 century (2.6e-10 day) at -3000, which
its mean elements, 1.7e9" a century, carried as 0.011 mas; their largest
term, up to `1739232000.0 * fracT`, rounded at 1.2e-7" (2e-4 mas) besides.
Over 64 consecutive JDs the geometric places scattered at S ~ 3-8
(Mercury 3.9e-3 mas, Venus 1.7e-3, the Earth 1.1e-3 heliocentric).

**Fix, as done here:** J - J2000 exactly as a two-sum dh + dl; each planet
argument and Mercury's (every planet's) mean-longitude T term as
rate * (dh + dl) / TIMESCALE with the product's error from `fma()` and the
quotient's from its remainder, reduced before the small parts are added;
the Moon's T with its rounding recovered the same way and the five mean
elements advanced by their rates times it, and their `fracT` products
reduced before their error is added. The values move by the roundings
they carried, <= 1.2e-3 mas (planets, heliocentric). Over 64 JDs at -3000:
Mercury 7e-7 mas, the Moon 7.5e-6 mas.

G50 `tests/moshprec.c` check A (with entry 51). G24 `mosh_rough`: Mercury's
geometric longitude over 64 JDs at -3000, 2.5e-7 mas shipped, 1.6e-3 compat.

## 51. The Moshier velocities are differences at rounded instants

**Severity: Correctness (~3e-6 of the speed at -3000; 0.13 mas between consecutive JDs on Mercury's apparent place, 45" on the osculating apogee, 9e-6 AU on Mars's osculating aphelion)**
**Where:** `swemplan.c`, `swi_moshplan()`; `swemmoon.c`, `swi_moshmoon()`, `ecldat_equ2000()`; `sweph.c`, `lunar_osc_elem()`

Every Moshier velocity was a finite difference: the planets and the Earth
(x(t) - x(t - h)) / h with h = 8.64 s, the speed 4.32 s late (Mercury
~1e-7 AU/day); the Moon central over +-4.32 s (bug 31). The instants
t +- h round at the JD's resolution -- a bias of up to 2e-6 of the speed,
the same for every JD of a binade -- and the series' own rounding (entry
50) was divided by 1e-4 day: ~3e-6 of the speed from one JD to the next at
-3000. The light time, x - tau v for Moshier, carried it into the apparent
places (Mercury 0.13 mas between consecutive JDs, S ~ 250; fixed stars and
True Citra 0.034 mas through the Earth's velocity in the aberration), and
the osculating elements amplified it: the osculating apogee 4.5e4 mas, the
true node's distance 1e-8 AU, `swe_nod_aps()`'s osculating Mars aphelion
9.3e-6 AU, between consecutive JDs. Speeds against five-point differences
of the positions: 1e-7..2.7e-5 deg/day. Two more things
the noise had hidden: `lunar_osc_elem()` took the true node's and apogee's
speeds over +-0.1 day "because here the node and apogee oscillate wildly"
-- the oscillation was this noise, and the step's truncation was 1.5e-3
deg/day on the apogee at -3000; and `ecldat_equ2000()` turned each of the
Moon's samples at other instants (the speed's, the node's at +-speed_intv)
with the cached obliquity of the date asked for, leaving its 6.2e-9 rad/day
out of the Moon's speed (3.5e-7 deg/day) and, through the 5 degree
inclination, 3.8e-6 deg/day out of the true node's.

**Fix, as done here:** the planets' and the EMB's velocities are the series'
derivative, term by term (C' cos A + S' sin A + A' (S cos A - C sin A),
A' the sum of the term's frequencies); the Earth-Moon offset's rate is a
central difference over +-2^-8 day (exact instants). The Moon's velocity
is a five-point difference over +-2^-5 and +-2^-4 day, exact instants,
truncation 9e-11 of the speed; each sample turned with its own date's
obliquity, in the cache's model. The true node's and apogee's speeds use
+-2^-8 day on Moshier. Positions move only where they carried the speed:
<= 0.24 mas on the planets' apparent places (the light time's x - tau v),
<= 0.009 mas the Moon, up to 66 mas on the true node and 34" on the
osculating apogee (their noise). Over 64 JDs at -3000: Mercury apparent
3e-7 mas, the apogee 0.044 mas, the true node's distance 1e-14 AU, Mars's
osculating aphelion 5e-15 AU, fixed stars and True Citra <= 4e-7 mas.
Speeds against five-point differences, J2000 geometric: planets <= 1.3e-12
deg/day, the Moon <= 5e-9; the true node <= 3.1e-8, the apogee <= 3.4e-6.
Left: the apparent speeds lacked tau times the acceleration, as Moshier
placed a body at x - tau v rather than at t - tau (the Moon 4e-7 deg/day
with aberration and 8e-5 without, Mercury heliocentric 5e-6 AU/day in its
distance rate; entry 32 left the Moon's), now entry 53; and `SEFLG_SPEED3`
still took the node and apogee over 0.1 day, for every ephemeris (entry 54).

Against the Swiss files, 241 epochs -3000..3000, Sun..Pluto, Moon, true
node, osculating apogee, geocentric apparent and topocentric equatorial:
the rms position differences are unchanged to 1e-5" for every planet and
the Moon, the apogee's 123.55" becomes 123.25", the true node's 72.7746"
72.7752". Heliocentric J2000 speeds agree as well or better (Venus
3.2e-6 to 1.9e-6 deg/day rms); Mercury's geocentric speed 2.37e-5 to
2.56e-5: the theory runs ~19 s behind the files in position and velocity
alike (a fit of x_S - x_M against v_M: -19 s Mercury, -20 s Venus, -25 s
the Earth), and the old 4.32 s lag leaned the same way by chance.

G50 `tests/moshprec.c`: A, at -3000 and 1300, 64 consecutive JDs, Mercury
geometric and apparent, the Moon, the osculating apogee, the true node's
distance and Mars's osculating aphelion; B, at -2500, 1 and 2000, the
planets', Moon's and Earth's speeds against five-point differences (<= 1e-7
deg/day), the true node's (< 1e-7) and apogee's (< 1e-5). The upstream
build fails all 21. G24 `mosh_speed_off`: the Moon's and Mercury's J2000
geometric speeds at -2500, 8.3e-10 deg/day shipped, 7.2e-6 compat.
G1: 3165 rows, re-baselined with the other follow-ups. Moshier positions
by up to 6e-8 deg and speeds by up to 2.9e-5 deg/day (the Moon's old step
bias); the osculating apogee and true node (~illcond) by up to 21", their
old noise; the mean node and apogee by up to 1.6e-10 deg on every
ephemeris (the lunar mean elements' T rounding, entry 50). G17 unchanged.

## 60. A plain negative mean anomaly freezes a fictitious body

**Severity: Correctness (a body whose mean anomaly is written "-12.5" does not move: 161 deg off its own ellipse 200 days on, its speed 1.2 deg/day against a rate of 0)**
**Where:** `swemplan.c`: `check_t_terms()`, `read_elements_file()`

`check_t_terms()` set its return value to "has T terms" when the field
held a `+` or `-` anywhere, taking the sign as the operator of a further
term. A mean anomaly written as a plain negative number, `-12.5`, is
one term with a sign; `read_elements_file()` read it as a polynomial in
T, and for a polynomial sets epoch = tjd so that no daily motion is
added. The mean anomaly stayed at -12.5 for all time: the body stood
still at its epoch position, while its reported speed (the two-body
motion) was 1.2 deg/day.
The same ellipse written 347.5 moved at its mean motion. A field with
a space and a minus after the first term and no T (`10 - 2`) went the
same way.

**Fix, as done here:** only a T makes the field a polynomial
(`strpbrk(sinp, "tT")`); a leading sign is a sign. Every field of
`ephe/seorbel.txt` that carries a sign in its mean anomaly also has a
T, so every set is unchanged: the 30 sets, at five dates over 1500
years, geocentric, heliocentric and J2000, bit-identical (positions
and speeds) before and after.

G59 `tests/fictneg.c`: a set with mean anomaly `-12.5` and the same
ellipse with `347.5` (a = 1 AU, e = 0.1), heliocentric J2000 at five
dates: the two within 1e-9 deg (measured 0), the negative one advanced
by the daily motion (161 deg at the latest date), its speed within
1e-7 deg/day of a five-point difference over 2^-6 day (2.6e-12) and its
distance rate within 1e-9 (1.3e-14); the upstream build fails all four
(161 deg off the positive set, 0 deg advance, speed off by 1.2
deg/day, distance rate by 5.1e-4). G24 `fict_negative_mano_off`: the
two sets 100 days on, 0 deg shipped, 113 deg compat. G1 and G17
unchanged.

## 54. `SEFLG_SPEED3` takes the true node's and osculating apogee's speeds over 0.1 day

**Severity: Correctness (1.4e-3 deg/day on the apogee, 3.5e-5 on the node, 1e-4 deg/day in the apogee's latitude, 5e-9 AU/day in distance)**
**Where:** `sweph.c`, `swe_calc_r()`; `sweph.h`

With `SEFLG_SPEED3`, `swe_calc_r()` picks the step of its three positions per
body: `MOON_SPEED_INTV` for the Moon, `INTP_APS_SPEED_INTV` for the
interpolated apsides (entry 45), `PLAN_SPEED_INTV` for the rest -- and
`NODE_CALC_INTV_MOSH`, 0.1 day, for `SE_TRUE_NODE` and `SE_OSCU_APOG` on
every ephemeris ("the optimum dt with Moshier"). That step averaged out the
Moshier Moon's noise, which entry 51 removed; what is left is the h^2 term of
a three-point slope over 0.1 day. Against five-point differences over 2^-5
day of the library's own positions at binary-fraction instants, at eight
epochs: the apogee's longitude speed is off by 1.4e-3 deg/day with Swiss
files, Moshier and de200 alike (1.1e-3 de200), its latitude's by 1.0e-4, the
true node's longitude by 3.5e-5, the distances by 4.5e-9 AU/day.

**Fix, as done here:** the step is `NODE_SPEED3_INTV`, 2^-9 day, for both
bodies and every ephemeris. Measured over steps 2^-14..2^-3 at eight epochs
(-2500..2800; 1650..2100 for de200, which spans 1600-2170), the error is
the truncation above 2^-9 and the positions' noise below: Swiss files and
de200 bottom out at 2^-11..2^-9 (apogee 1e-7..4e-7 deg/day), Moshier at
2^-10..2^-8 (1e-6), the node at 1e-8 on all three. The speed stays the
three-point slope at t (entry 31). Now: apogee longitude <= 6.2e-7 (Swiss),
2.4e-7 (de200), 1.3e-6 (Moshier) deg/day, latitude <= 9.5e-8, the true node
<= 2.3e-8, distances 2e-12 AU/day. Cost: none (the same three calls).

G53 `tests/node3speed.c`: both bodies on the three ephemerides, longitude,
latitude and distance speeds against five-point differences, within 3e-6
deg/day and 1e-10 AU/day. The upstream build fails all six rows (apogee
0.15..0.19 deg/day, node 3.2e-3..4.7e-3, with entry 31's late slope on top;
7e-7 AU/day). G24 `node3_speed_off`: the Moshier apogee at 2800, 1.1e-6
deg/day shipped, 0.19 compat. G1: rows with `SEFLG_SPEED3` on these two
bodies, if any, move by the figures above; not re-baselined here.

## 55. The mean nodes' and apsides' speeds are one-day differences

**Severity: Correctness (2e-5 deg/day on the Moon's mean perigee and apogee latitude speed, 2e-6 on their longitude speed, 6e-10 AU/day on the nodes' distance speed)**
**Where:** `swecl.c`, `swe_nod_aps_r()`; `swemmoon.c`, `swi_mean_lunar_elements()`

The mean points' speeds were never rates. `swi_mean_lunar_elements()` gave the
Moon's node and perigee speeds as the position less the position a day back,
a backward difference with the curvature of half a day and without the
correction terms (`corr_mean_node()`, `corr_mean_apog()`) of the position
itself: 2e-6 deg/day on the perigee's longitude. `swe_nod_aps()` took the
aux. position of the mean perigee a day on and the speed as that less the
position (bug 38 had only moved the subtraction to the latitude): the
Moon's perigee turns 0.11 deg/day on an orbit inclined 5.1 degrees, and the
half day of the latitude's curvature is 2e-5 deg/day of the latitude speed.
The nodes' distance speed was the radius at an argument of perihelion a day
on less the radius, 6e-10 AU/day for the Moon (1e-10 for Mars; the planets'
perihelia turn slowly, their longitude and latitude speeds were within 1e-8).

**Fix, as done here:** the Moon's node and perigee speeds are central
differences of the corrected positions over +-2^-5 day, at exact instants;
`swe_nod_aps()` carries the node, the argument of perihelion, the
inclination, the semi-major axis and the eccentricity to +-2^-6 day and
takes the perigee's longitude and latitude speeds and the nodes' distance
speeds as the central differences of those points (the aphelion's from the
perigee's). Positions are unchanged (the Moon's node and perigee at tjd are
the same code). Against five-point differences of the library's own
positions at binary-fraction instants, 1500..2500, mean, ecliptic: Moon
geocentric lon 4.7e-6 to 8.1e-10, lat 2.1e-5 to 3.9e-11 deg/day, dist 6.2e-10
to 3.8e-15 AU/day; Moon heliocentric 2.8e-8, 5.7e-8, 5.6e-10 to 7.9e-10,
4.7e-11, 5.8e-11. Mars is unchanged, 4.2e-8, 5.8e-9, 6.6e-10 at +-500 years:
the planets' element rates are the linear terms only, ep[1]/36525, and the
elements' quadratic and cubic terms are not in the speeds. Left, not
fixed here.

G54 `tests/moonaspeed.c`: the longitude, latitude and distance speed of the
four points, Moon and Mars, geocentric and heliocentric, at seven instants
1500..2500, within 3e-8, 1e-8 and 2e-10; the upstream build fails all four
cases (Moon geocentric 5.2e-6 lon, 5.1 lat, the latitude through bug 38).
G42 and G47 unchanged. G24 `moon_perigee_speed_off`: the Moon's mean
perigee longitude speed at 2461338.137 (binary fraction), 2.6e-10 deg/day
shipped, 1.8e-6 compat. G1 rows of the mean nodes' and apsides' speeds
move (the Moon by 5e-6 deg/day longitude, 2e-5 latitude) and need the
re-baseline.

## 58. The nutation's rate in every speed of date is a backward difference

**Severity: Correctness (6.5e-10 deg/day in every longitude speed of date; 2.7e-8 deg/day in the right ascension of a star near the pole, 3.5e-10 as an angle on the sky)**
**Where:** `sweph.c`: `swi_check_nutation()`, `swi_nutate()`, `swi_obliquity_rate()`; `swephlib.c`: `calc_nutation_grid()`, new `swi_nutation_rate()`

`swi_nutate()` adds the rotation's own turning to the velocity (entry 49),
from the rate of the nutation angles: (nut(t) - nut(t - h)) / h, h = 1e-4
day. That is the rate at t - h/2, short by (h/2) times the second
derivative, which the fortnightly term (18.6-year node, 13.66 days) makes
6.5e-10 deg/day. It is in every body's longitude and right ascension speed
of date, planets, Moon, stars, and in the obliquity rate of entry 33; near
the pole the right ascension multiplies it by 1/cos(declination), 2.7e-8
for Polaris. Measured as the speed of date less the NONUT speed (which
removes the files' and the light time's own 5e-9..3e-8) against the
five-point difference of the same two positions: the Sun, Mars, Jupiter,
Saturn and Pluto 6.2e-10..7.3e-10 deg/day, Polaris's longitude 6.9e-10,
its right ascension 2.65e-8.

**Fix, as done here:** `swi_nutation_rate()` returns the rate at t. For
the default model (the 2000A septic Hermite grid) it is the polynomial's
own derivative from the nodes' cached values: no series evaluation, and
`swi_check_nutation()` no longer evaluates the nutation and builds a matrix
at t - h at all (one nutation and one matrix less per fresh instant). Any
other model (2000B, 1980, Woolard, JPL Horizons, the interpolated nutation)
takes the central difference of nutation at t - h and t + h, one nutation
more than before, uncached. The rate is kept in `struct nut` (`dnut`,
`dnut_ok`) beside the angles; `ctx->nutv` is no longer filled. Nutation
itself is unchanged, bit for bit.

G57 `tests/nutrate.c`: Sun, Mars, Jupiter, Saturn, Pluto and Polaris,
ecliptic and equatorial, seven dates 1900-2100, speed of date less NONUT
against five-point differences over 2^-8 day: the planets <= 1.7e-11
deg/day (<= 7.3e-10 upstream), Polaris's longitude 6.7e-11 (6.9e-10),
its right ascension 1.9e-9 (2.65e-8; the remainder is the mean obliquity's
turning inside the nutation matrix, ~3e-11 deg/day times 82, left alone;
done in entry 69).
The upstream build fails all 12. G24 `nut_rate_off`: Polaris's right
ascension at J2000, 1.0e-9 deg/day shipped, 1.04e-8 compat. G32 (default
against 2000B: daily 1.95, scattered 3.20, limits 5 and 8), G49, G41, G29
unchanged. bench calc-swieph/calc-moseph no slower (min of 7: 1.23/1.06 s
against 1.40/1.22 s for the commit before).

## 62. The aberration rate's Earth leaves out the Moon's pull

**Severity: Correctness (2.3e-7 deg/day on Nibiru's longitude speed, 6e-7 on VEA's, 3e-7 on Planet 9's and the Christ Comet's; 1e-7 to 3e-8 on the farther planets' latitude speeds)**
**Where:** `sweph.c`: `earth_at_lt_sun()`

Entry 34's aberration-rate term needs tau a(teval), the light time times
the observer's acceleration. Its Earth was V(teval) - V(t) from two
ephemeris reads plus tau^2 a'/2 with a' the rate of the Sun's pull alone
(light time under 10 days), and, past 10 days, tau a(teval) whole from
the Sun's pull, -GM r / r^3. Both left out the Moon's pull on the Earth,
GM_m d / |d|^3: 0.6% of a, and 7.5% of a' (the reads carry it below 10
days, but only as a midpoint value; the series for its 27-day period fails
by tau omega / 3 = 8% per day of light time). Measured on ts20 against
five-point differences of the same call's positions over 2^-7 day, seven
dates -300..+300 years, Swiss files: Nibiru 2.0e-7 deg/day geocentric
(1.9e-7 topocentric), the Christ Comet 3.8e-7, Planet 9 3.2e-7, VEA 5.1e-7,
Harrington 6.8e-8; Pluto 2.1e-8, Neptune 1.5e-8, Saturn through Pluto's
latitude speeds 4e-10 to 5e-9 and the Witte planets 2e-8.

**Fix, as done here:** the geocentric Moon at teval is in the save area
(`sweplan()` left it there with its velocity for the Earth), so the Moon's
pull a_m = GM_m d / |d|^3 (GM_m = GEOGCONST / EARTH_MOON_MRAT) and its rate
come with no further ephemeris read. The rate joins a' below the switch;
past it tau (a_Sun + a_m)(teval) is taken whole. The switch is a light time
of 0.3 day when the Moon is in hand (scanned 0.05, 0.3, 0.8, 1.5 days: 0.3
and 0.8 leave every body within 1e-8 deg/day, 0.05 is no better than 0.3 on
planets and 0.3 is a little better than 0.8 on Apollon's latitude), and
stays at 10 days without it. JPL and Moshier have no Moon in the save area
and are unchanged (the JPL Earth is the real one, the difference of its
reads carries the Moon; its a' still lacks 7.5%, as before). Measured after: Nibiru 7.5e-9
geocentric, 1.1e-8 topocentric; Christ Comet 8.4e-9 and 2.1e-8; Planet 9
8.6e-9 and 2.2e-8; VEA 8.7e-9 and 1.8e-8; Harrington 8e-9; Pluto 5.4e-9 and
1.5e-8; no body of the 36 scanned (planets, Chiron, Pholus, Ceres, fictitious
bodies in seorbel.txt) is worse by more than the reference's noise. No extra read: bench
calc-swieph 0.703-0.721 s after (three runs, machine loaded), 0.746 before
(one run); calc-moseph does not take the new branch.

G61 `tests/lunarpull.c`: Nibiru, the Christ Comet, Planet 9, VEA, Saturn and
Pluto, geocentric and topocentric (Zurich), seven dates, longitude and
latitude speeds within 5e-8 deg/day (measured <= 2.3e-8; the build before
this fix fails the eight far-body rows at 1.9e-7 to 5.1e-7; the upstream
build fails all twelve). G24: `far_body_moon_pull_off`, VEA's longitude
speed, 2.7e-9 shipped, 3.4e-5 compat.

## 61. A topocentric fixed star is deflected from the geocenter

**Severity: Correctness (3e-7 to 9e-6 arcsec of the deflection, stars 5-40 degrees from the Sun; the second part is a returned value nothing reads)**
**Where:** `sweph.c`: `fixstar_calc_from_struct()`, `swi_fixstar_calc_from_record()`, `swi_deflect_light()`

Two small things in the star paths, both seen at entry 49.

(1) Both paths called `swi_deflect_light(ctx, x, 0, iflag & SEFLG_SPEED)`:
the topocentric flag never reached the deflection, so the Earth in it was
the geocenter, the site's 4.3e-5 AU missing from the Earth-Sun vector and
the site-dependent part of the deflection (4e-5 of it) lost: 7.4e-6 arcsec
for Regulus 5 degrees from the Sun, 9.2e-6 for Spica, 5.3e-6 Aldebaran,
5.2e-7 Sirius at 40. Passing the flag alone would have been worse than
nothing: the deflection reads the site from `ctx->topd`, which the star's
path never fills (its `swi_get_observer()` calls are NO_SAVE), so it would
have been empty or the last planet call's. Entry 49 had that as a stale
site; measured, the position after topocentric Mars and Moon calls at
another site and instant equals the fresh one to the bit, because the flag
was never passed.

**Fix, as done here:** `swi_deflect_light()` is `deflect_light_site()` with the
site as a parameter, `swi_deflect_light()` passing `ctx->topd.xobs` (planets
and swecl.c, unchanged); the star paths pass the site they computed for
the call, with `SEFLG_TOPOCTR` kept in the flag.

(2) `fixstar_calc_from_struct()` returned its flags with `SEFLG_SPEED`
always cleared, `swi_fixstar_calc_from_record()` and `swe_calc()` clear it
only when the caller did not ask. Measured over twelve flag sets the public
`swe_fixstar2()`, `swe_fixstar()` and their `_ut` forms return the caller's
own `iflag` in every one (`swe_calc()` returns the flags it used: NONUT
added for sidereal, NOABERR and NOGDEFL for heliocentric Moshier), and use
the star functions' value for the error test only. The value is dead, so
this is not observable through the library and has no gate; it now matches
the record function. Left: the public star functions returning the caller's
flags rather than the flags used (what `swe_fixstar_ut()`'s ephemeris
check was written to read) is upstream's, changes every `rf` in G1's star
rows, and is not touched here.

G60 `tests/startopodefl.c`: Regulus, Spica, Aldebaran, Sirius on dates 5-40
degrees from the Sun, two sites, both functions: the site-dependent part
of the deflection against `g1/g2 (u.q e - u.e q)` formed from `swe_calc()`'s
barycentric Earth and Sun and the site (geocentric minus topocentric Moon):
off by <= 4.3e-11 arcsec, effect 3e-7 to 9e-6; and the position after other
topocentric calls equal to the bit. The upstream build fails all 16 of
the first (off by 100%). G24 `star_topo_defl_off`: Regulus's site-dependent
deflection 7.4e-6 arcsec shipped, 5e-11 compat.

## 59. Four small house defects: Placidus cusps unconverged, RADIANS speeds in degrees, sidereal Makransky computed as Treindl, Sunshine houses without files

**Severity: Correctness (1.1e-5 deg/day on a Placidus cusp speed at 60N; 57.3 times on every RADIANS speed; 41 deg on sidereal Makransky cusps; ERR and Porphyry for Sunshine houses)**
**Where:** `swehouse.c`: `CalcH()` (Placidus, Gauquelin; `VERY_SMALL_PLAC_ITER`), `swe_houses_ex2_r()`, `sidereal_houses_trad()`, `swe_houses_r()`

1. The Placidus and Gauquelin cusp is the fixed point of C = Asc1(ra,
f(C)), iterated until two cusps lie within 1/360000 deg. The last step
is the error times the iteration's contraction, so the cusp stayed up to
3.5e-8 deg off the fixed point at 47-60N (4.5e-6 at 65N, where the
contraction nears 1). Differences of the returned cusps carry the rate
of that error, 2.6e-6 deg/day over the G58 cases and 1.1e-5 at 60N,
against `PlacDer()`'s exact rate (entry 41).
2. With `SEFLG_RADIANS`, `swe_houses_ex2()` turned the cusps and ascmc to
radians and left `cusp_speed` and `ascmc_speed` in deg/day.
3. `sidereal_houses_trad()` took `toupper()` of the house system, so
sidereal Makransky Sunshine (`'i'`, the one lower-case system the tropical
path dispatches; the others are upcased with a deprecation warning)
computed Treindl's `'I'`: 41 deg apart at worst at 60S.
4. Sunshine houses need the Sun's declination, from `swe_calc_ut()`. With
no Swiss files for the date, and the ephemeris fallback off (this fork's
default), it refuses; the houses went to Porphyry and returned ERR (16.7
deg from the Sunshine cusps), `ascmc[9]` 0.

**Fix, as done here:** (1) the stopping step is 1e-12 deg, with an
Aitken step once the steps are below 1e-3 deg and shrinking by less than
half (the remaining error is d r / (1 - r)): 3-6 evaluations of the map
per cusp against 2-4. Positions move by up to 1.9e-8 deg at 45-50N,
5.3e-7 at 60-65N and 4.5e-6 at 65-70N (over latitudes 70S-70N, 12 ARMCs,
Placidus and Gauquelin); no call changes between OK and Porphyry.
`swe_houses_ex` Placidus: 4.85 us at 47N and 5.79 at 60N before, 5.54 and
6.67 after (+14%, -O2, best of 8). (2) The speeds are converted with the
positions. (3) `'i'` is kept as is for the tropical call inside. (4) On a
refusal the Sun is asked again with `SEFLG_MOSEPH`: the declination is an
input to the houses, not an ephemeris position the caller asked for, and
Moshier's is 0.1" off (cusps 3.8e-6 deg from the Swiss-file call).

G58 `tests/housefix.c`: Placidus and Gauquelin speeds against five-point
differences, worst 3.1e-9 deg/day (2.6e-6 with the old stopping rule
alone, 24 in the upstream build, which has entry 41 as well); RADIANS speeds
2.3e-18 relative to degrees times pi/180 (0.98); sidereal `'i'` 0 from the
tropical cusps less the ayanamsa (41 deg); Sunshine houses OK without files
(ERR). The upstream build fails all four (251 failures). G24
`house_fix_off`: 0/4 defects shipped, 4/4 compat (Placidus cusp 11 at 60N:
7.4e-10 deg/day shipped, 1.25 compat).

## 68. Three house residues: Sunshine's Sun from the wrong ephemeris, stale Gauquelin cusps after the Porphyry fallback, the ARMC's rate for a cusp on an equinox point

**Severity: Correctness (6e-6 deg on a Sunshine declination; 24 cusps and 24 speeds left as the caller had them; 105 to 427 deg/day on a Placidus cusp speed at one instant)**
**Where:** `swehouse.c`: `swe_houses_ex2_r()`, `CalcH()` (Placidus, Gauquelin)

1. `swe_houses_ex2_r()` asked `swe_calc_ut_r()` for the Sun's declination
(Sunshine houses) with no ephemeris flag, so a `SEFLG_MOSEPH` (or
`SEFLG_JPLEPH`) call got the Swiss ephemeris' Sun whatever the caller
chose: at 2026-07, 47N, the declination 23.026219549 against Moshier's
23.026225559 (6.0e-6 deg; the cusps 3.8e-6 deg). `swe_houses_r()` has no
`iflag`, so there is nothing to pass there.
2. A Gauquelin call (36 cusps) that fell back to Porphyry (within the
polar circle, a negative return) has 12 cusps. Cusps 13..36 and their
speeds stayed as the caller had them, and with `SEFLG_RADIANS` the
conversion multiplied them anyway: at 80N arrays prefilled with 7 came
back 7 in degrees and 0.1222 in radians, 24 cusps and 24 speeds each.
`swe_houses_r()` and `swe_houses_armc_ex2_r()` left them the same way.
3. In the Placidus and Gauquelin branches, when the first-guess cusp is on
the equator (|tan dec| < 1e-10, the cusp at an equinox point; cusps 11, 12,
2, 3 and sectors 2..9, 29..36) the cusp is the right ascension and its
speed was set to ARMCS, 360.99 deg/day. The cusp moves at A_x, the rate of
Asc1 in the right ascension at the pole height m tan(fi) the semi-arc
fraction m gives there (f'(C) = 0 on the equator, as `PlacDer()` takes it):
465.6 deg/day for Placidus cusp 11 at 47N, 524.8 at 60N, 787.9 for cusp 12.

**Fix, as done here:** (1) the caller's `SEFLG_JPLEPH`, `SEFLG_SWIEPH` and
`SEFLG_MOSEPH` bits go with the Sun's call; bug 59's Moshier retry on a
refusal stays. (2) A Gauquelin return below zero zeroes cusps and speeds 13..36 in
`swe_houses_armc_ex2_r()` (so `swe_houses_r()` too) and, after the call and
before the radians conversion, in `swe_houses_ex2_r()` (the sidereal paths). (3) The speed is
A_x * ARMCS at pole height atan(m tan fi), the same limit `PlacDer()`
uses. No cost: none of it is on a path that was not already taken.

G67 `tests/houseresid.c`: the MOSEPH call's `ascmc[9]` is Moshier's Sun to
1e-12 (the Swiss one before); 96 of 96 entries 0 at 80N (24 of 96 stale), and 72 of 72 from `swe_houses_r()` and `swe_houses_armc_ex2_r()`;
the 12 equinox-point cusps (Placidus 11, 12, 2, 3; Gauquelin 9, 29; 47N and
60N) within 6.5e-8 deg/day of five-point differences (up to 427 deg/day off
before). The upstream build fails all three (14 failures). G24
`house_resid_off`: 0/3 defects shipped, 3/3 compat (equinox cusp 3.8e-8
deg/day shipped, 105 compat). G45's Placidus and Gauquelin tolerance,
left at the old 3.5e-8 deg convergence, is now 2e-7 deg/day + 5e-10 of the
rate (worst 9.4e-8, 1.4e-10 of the rate, with bug 59's 1e-12 convergence).

## 63. A planet's light-time position is read at the rounded instant, and its heliocentric light-time rate leaves out the Sun's velocity

**Severity: Correctness (positions ~1e-9 deg on Mercury, a noise that a five-point difference over 2^-9 day turns into 4.7e-7 deg/day heliocentric and 2.2e-7 geocentric; 3e-8 deg/day on Mercury's heliocentric speed)**
**Where:** `sweph.c`: `app_pos_etc_plan()`

Two residues after entry 35, found as Mercury's speed sitting 1-2e-8
deg/day off a five-point difference of its positions with SEFLG_NOABERR or
SEFLG_HELCTR, and 7e-10 with SEFLG_TRUEPOS. The first scales as 1/h with
the step, so it is in the positions: the body was read at t = teval - dt
rounded to a double, up to half an ulp of the date (2.3e-10 day near the
present) from the emission instant, and the apparent place jumped by the
body's motion over that from one date to the next (the Moon had the same,
entry 23). Scanned on ts20, Swiss files, 61 dates -3000..+3000, steps
2^-6..2^-9 day, binary-fraction instants, longitude speed against the
difference: Mercury 1.8e-7 geocentric and 7.7e-7 heliocentric, Venus 3.9e-7
and 2.2e-7, Mars 1.4e-7 and 4.9e-8 (5.3e-8 at thirteen dates without
aberration, the 3e-8 seen in entry 52's scan), Jupiter 1.2e-8 (-9e-9 in that
scan), Saturn 4e-9, the outer planets 1.6e-9; Ceres, Pallas, Juno and
Vesta 3e-8 to 7e-8 (1400-2900), Chiron and Pholus 8e-9. The second is steady with the step and heliocentric
only: entry 35's rate rho^.(v_P - v_O) / (c + rho^.v_P) times v_P was taken
from the body relative to the Sun at teval with the Sun at the origin, so
rho^.v_P and v_P lacked the Sun's barycentric velocity, ~13 m/s: 3e-8
deg/day on Mercury's heliocentric speed (dtau/dt 3.5e-5 times 7.5e-6
AU/day).

**Fix, as done here:** after the read at t, the body is moved along its
velocity at teval (there with or without SEFLG_SPEED) by the rounding,
(teval - t) - dt, exact since t is near teval; JPL and Swiss files only,
as Moshier's positions are extrapolated by dt itself. The heliocentric and
barycentric rate takes the barycentric body and centre (xobs, the Sun or
zero). After, same scan: every planet within its true positions' own
floor, geocentric <= 8.7e-10 (Mercury), without aberration <= 7.9e-10,
heliocentric <= 2.1e-9 (Mercury, whose true heliocentric positions are off
their own differences by the same); the asteroids <= 1.7e-10 without
aberration. Positions
move by <= 1e-9 deg. Two multiply-adds a call: bench calc-swieph
1.237-1.251 s after, 1.242 before (runs within the machine's noise).

G62 `tests/ltround.c`: Mercury to Pluto, Ceres and Vesta, geocentric with
and without aberration and heliocentric, thirteen dates, four steps,
longitude and latitude speeds within 5e-9 deg/day (measured <= 1.6e-9; the
build before this fix fails 18 of the 30 rows, by up to 4.7e-7; the
upstream build fails all 30, by 8.9e-6 to 1.7e-4). G24:
`mercury_lt_round_off`, Mercury's heliocentric longitude speed over 2^-8
day at nine dates: 1.8e-9 shipped (2.1e-7 before the fix), 1.5e-4 compat.

Not fixed here, same rounding: `swe_calc_pctr()` reads both bodies at
tjd - dt rounded (Mercury from Mars, 1.1e-7 deg/day at 2^-9 day, 4.9e-10
true positions). Fixed as entry 77.

## 64. The planets' mean nodes' and apsides' speeds leave out their elements' t^2 and t^3 terms

**Severity: Correctness (4e-6 deg/day on Jupiter's mean aphelion at +-3000 years; Mars 4.2e-8 deg/day in longitude, 5.8e-9 in latitude and 6.6e-10 AU/day in distance at +-500)**
**Where:** `swecl.c`, `swe_nod_aps_r()`, the mean points of Mercury..Neptune

The mean node, perihelion, inclination, semi-major axis and eccentricity of
a planet are cubics in T (`el_node`, `el_peri`, `el_incl`, `el_sema`,
`el_ecce`), and the points' positions carry all four terms. Their rates,
`vincl`, `vsema`, `vecce` and the node's and perihelion's speeds, were
`ep[1] / 36525`: the linear term. Bug 55 made the speeds central differences
of the elements carried +-2^-6 day, so the differences were of a straight
line and the quadratic and cubic terms were never in them (entry 55 left it
open). Measured against five-point differences of the library's own
positions (true positions, no deflection), mean nodes, perihelia and
aphelia of Mercury..Neptune, geocentric and heliocentric, seven instants
-3000..+3000: longitude speeds off by 5.4e-7 (Mercury) to 4.2e-6 (Jupiter)
deg/day, latitude by 1.9e-8 to 1.6e-7, distance by 1.6e-10 (Mercury
heliocentric) to 5.9e-8 (Jupiter geocentric) AU/day.

**Fix, as done here:** the rates are the polynomials' derivatives at t,
(ep[1] + t (2 ep[2] + 3 ep[3] t)) / 36525; the central differences over
+-2^-6 day of the straight lines they give are the rates at t. Positions
are unchanged to the bit. After: longitude <= 5.3e-10 deg/day (geocentric
Mercury; heliocentric <= 8e-12), latitude <= 2.8e-11, distance <= 2.2e-11
AU/day.

G63 `tests/planetaspeed.c`: the same fourteen cases (Mercury..Neptune,
geocentric and heliocentric), the four points, longitude, latitude and
distance speeds within 1.5e-9, 1e-10 deg/day and 1e-10 AU/day; the
upstream build and the build before this fix both fail all 42 checks
(three per case). The
gate uses SEFLG_TRUEPOS|SEFLG_NOGDEFL: with the apparent position the
heliocentric distance rates of Uranus and Neptune and Mars's geocentric
longitude speed (5.6e-9 deg/day in G54) carry 1e-9 AU/day and 5e-9 deg/day
of the deflection's and aberration's own rates, which this entry does not
touch. G54's Mars limits, ten times the Moon's, are the Moon's now except
the longitude, 1e-8 (measured 5.6e-9, 9.8e-11 latitude, 1.2e-10 distance).
G24 `planet_apsis_rate_off`: Jupiter's mean aphelion's heliocentric
longitude speed at -3000, 2.1e-12 deg/day shipped, 3.8e-6 compat. G42, G47,
G54 pass; G1 rows of the planets' mean points' speeds move.

## 67. The star functions return the caller's flags, and an emptied nutation cache answers for JD 0

**Severity: Correctness (a returned value: the flags used are not reported; 4.2e-4 deg in the sidereal ayanamsa at exactly tjd = 0)**
**Where:** `sweph.c`: `swe_fixstar2_r()`, `swe_fixstar_r()`, `fixstar_calc_from_struct()`, `swi_fixstar_calc_from_record()`, `swi_check_nutation()`, `swi_plan_for_osc_elem()`; `sweconfig.c`: `swi_invalidate_models()`; `sweph.h`: `SWI_NUT_NEVER`

Three small things, the first two left open at entries 61 and 58.

(1) `swe_fixstar2()`, `swe_fixstar()` and their `_ut` forms returned the
caller's own `iflag`, and used the inner functions' value for the error
test only. `swe_calc()` returns the flags it used: NONUT added for a
sidereal request (0x40), NOABERR and NOGDEFL for heliocentric Moshier
(0x600), and the ephemeris bits of a substituted ephemeris. Measured over
twelve flag sets and the four functions, 14 of 48 returns differed from
`swe_calc()`'s for Mars with the same flags, and the four of the JPL
request answered by the Swiss ephemeris (fallback allowed) named JPL.
`swe_fixstar_ut()`'s own check of the ephemeris returned could therefore
never fire.

(2) `ctx->nut.tnut = 0` marked the nutation cache empty (a closed library,
a model change, a new context), and 0 is also JD 0 (-4712): a request at
exactly tjd = 0 found the empty cache's zero angles. `ctx->nutv` (no longer
filled since entry 58) and `ctx->nut2000` (never filled) had the same tnut
of 0 and were read by `swi_plan_for_osc_elem()` for tjd = 0 and J2000.
`swi_check_nutation()` guarded its own reader with `tjd == 0`; the others
did not. Measured: `swe_get_ayanamsa_ex()` at tjd = 0, sidereal, default
flags, after `swe_close()`: 291.345410209, the NONUT value, against
291.345832639 at tjd = 1e-9 (4.22e-4 deg, the nutation in longitude).
`swe_calc()` itself at tjd = 0 was right (the guard), as were the
osculating elements (the cache was filled by the call's own planet
position first), so (2) is a latent hazard with one measured case.

(3) `tests/starspeed.c`'s header said the 6.6e-10 deg/day measured there was
the nutation's backward difference; entry 58 removed that. Re-measured: 4.0e-11
deg/day (angles), 1.7e-13 (distance rate), 7e-13 (XYZ).

**Fix, as done here:** (1) the two public functions return the inner
value; `fixstar_calc_from_struct()` and `swi_fixstar_calc_from_record()`
put the ephemeris that answered (`main_planet_bary()`'s `iephe_used`) into
the ephemeris bits, as `swe_calc()` does. (2) An empty nutation cache is
`tnut = SWI_NUT_NEVER` (-1e300): set at context creation, at both clearing
sites and in `swi_invalidate_models()`; `swi_check_nutation()` no longer
needs `tjd == 0`; `swi_plan_for_osc_elem()` no longer reads `nutv` or
`nut2000`. (3) The comment says what is measured, and the gate's limits are
2e-10 deg/day (was 1e-8) and 1.7e-11 (was 1.7e-10) for XYZ.

G66 `tests/starflags.c`: the four star functions against `swe_calc()` and
`swe_calc_ut()` over twelve flag sets, flags equal and ephemeris bits equal,
the JPL row proven to have fallen back; the ayanamsa at tjd 0 against 1e-9
(1.1e-13 deg) and against NONUT (4.22e-4). The upstream build fails 20
(14 flag rows, 4 ephemeris-bit rows, 2 ayanamsa). G24 `star_ret_flags_off`:
0 bits off `swe_calc()`'s shipped, 3 compat; `ayanamsa_jd0_off`: 1.1e-13
deg shipped, 4.2e-4 compat. G1: none of its 90 star rows moves (all
rf=258 both ways: flags used equal flags asked); the golden transcript is
byte-identical before and after. G49, G57, G60 unchanged.

## 52. The aberration rate term is first order in v/c

**Severity: Correctness (2.8-3.2e-8 deg/day on every topocentric longitude speed, up to 2e-8 on latitude speeds; +4-5e-9 deg/day on every geocentric one)**
**Where:** `sweph.c`: `add_aberr_rate()`, `observer_accel()`; through them `app_pos_etc_plan()`, `app_pos_etc_plan_osc()`, `app_pos_etc_orbel()`, `app_pos_etc_moon()`, `app_pos_etc_sun()`, and `swe_nod_aps_r()` by `swi_aberr_rate()`

The aberrated place A(u, v), v = V/c, moves with the observer's velocity,
and the rate term (entry 34) is its derivative along dv/dt = a / c. It was
taken as tau a less its part along the line of sight: the first order in
v/c. And a site's acceleration had its radial part stripped before that,
on the ground that aberration keeps the distance to first order. The
derivative has a second-order part, terms in v times dv of relative size
v/c = 1e-4; a radial dv turns the place by v_perp |dv| / 2. On a site,
the centripetal acceleration (1.7e-3 AU/day^2 at the equator) against the
Earth's orbital velocity: 4.5e-5 of the diurnal aberration rate, scaling
with cos(latitude) and signed by the hour. Topocentric longitude speeds
with aberration were off their positions' rate by 2.5e-8 deg/day on the
Sun at the equator, 1.8e-8 at 47N, 1.1e-8 at 70N; 2-3.5e-8 on the planets;
against 2e-9 with SEFLG_NOABERR. Geocentric, the Earth's own acceleration
gave +4-5e-9 deg/day on the Sun at every epoch and on every body. A
decomposition at Jupiter, topocentric: the change of the place with the
geometric direction (the backward difference in `swi_aberr_light()`)
agrees with its numerical rate to 5e-11 deg/day; the change with V, taken
exactly along the true a, to 6e-12; the first-order formula is off by up
to 2.2e-8, and the stripped a, exactly differentiated, by 1.6e-8. Both
had to go: replacing only the formula moved the topocentric Sun from
2.15e-8 to 1.66e-8. Upstream's term is the chord V(t) - V(t - tau), added
whole, so the second-order part is missing there too (upstream sent
topocentric speeds through three positions instead).

**Fix, as done here:** `add_aberr_rate()` adds the exact derivative of
A(u, v) = (b u + f2 |u| v) / (1 + f1) along w = d / |u|, d the observer's
velocity difference (tau a; |u| / c is the light time), at the geometric
place u, which is the apparent one aberrated back by -v (exact: A(A(u, v),
-v) = u and |A| = |u| to rounding). `observer_accel()` returns the whole
of the site's acceleration. Over 15 bodies, 10 epochs -2900..2900,
against five-point differences: the topocentric Sun 2.15e-8 to 2.9e-9
(<= 3e-9 at 0, 47 and 70 deg latitude, the level without aberration),
Jupiter 2.1e-8 to 8.2e-9 (its ephemeris' own, the same geocentric),
topocentric latitude speeds 6e-9..1e-8 to 6e-10..3e-9; the geocentric Sun
5.5e-9 to 1.5e-9. Positions unchanged; the cost is not measurable (a
topocentric Sun or Jupiter with speed 22.6-23 us/call before and after,
calc-swieph and calc-moseph within the bench's noise). Left: the Moon's
part of the Earth's jerk, left out of `earth_at_lt_sun()`'s light-time
series (entry 34), gives tau j / 2c, a lunar-month wave of 1.2e-8 deg/day
on Neptune, 4.6e-9 on Saturn, the aberration's part of their geocentric
speeds.

G51 `tests/aberrspeed.c`: the Sun, Mercury, Venus, Mars, Jupiter and
Ceres, ten epochs, geocentric of date and J2000 equatorial, topocentric of
date and J2000 equatorial at 0, 47.37, 70 and -33.9 deg; the residual
against a five-point difference with aberration less that without, so the
ephemeris' own speed noise cancels. Angles within 5e-9 deg/day (measured
<= 3.3e-9), the distance rate within 1e-11 of the distance per day
(<= 5.4e-12). Before the fix all ten sets failed (1.4-3.2e-8 topocentric,
6.5-6.8e-9 geocentric); the upstream build fails all 20 (5.3e-7 and
1.7-2.7e-5 deg/day, 3e-7..1.8e-6 of the distance per day). G24
`topo_aberr_second_order_off`: the aberration's part of the topocentric
Sun's longitude speed on the equator in 1990, 5.4e-10 deg/day shipped
(3.1e-8 before), 3.0e-6 compat.

## 53. Moshier moves a body back along its velocity instead of reading it at t - tau

**Severity: Correctness (13.0 mas on Mercury's place; the Moon's speed 1.1e-4 deg/day without aberration, 4.6e-5 heliocentric; Mercury's heliocentric distance rate 5.6e-6 AU/day; the centre of body added twice, 4.7e-7 AU on Jupiter)**
**Where:** `sweph.c`: `app_pos_etc_plan()`, `app_pos_etc_moon()`

A body seen at t is where it was at t - tau. On the files and JPL,
`app_pos_etc_plan()` sets tau by two passes along the velocity and reads
the ephemeris at t - tau; `app_pos_etc_moon()` reads it there and solves
the rest (entry 23). Under `SEFLG_MOSEPH` both kept x(t) - tau v(t)
instead, the body moved back along its velocity at t, and the planets
took one pass, not two:
- the place was off by tau^2 a / 2 and, from the single pass, by
  v tau rdot / c: 13.0 mas on Mercury, 3.4 on Venus, 1.3 on Mars, 0.13 on
  Jupiter, 0.09 on Chiron (an asteroid under Moshier takes this path),
  1.2 mas on Mercury heliocentric;
- the speed kept v(t) where it is v(t - tau). The planets re-read the
  series at t - tau for the speed, but only geocentric: heliocentric,
  tau a was missing, radial, 5.6e-6 AU/day on Mercury's distance rate,
  1.2e-6 on Mars's. The Moon was never re-read: the Earth's 6e-6 km/s^2
  and the Moon's own 3e-6 over its 1.3 s, 1.1e-4 deg/day without
  aberration (topocentric alike; 4e-7 with it, where the Earth's velocity
  at teval in the aberration rate cancels most of it), 4.6e-5
  heliocentric over 500 s;
- the single pass left tau off by 1e-4 of itself, and the aberration rate
  divides the observer's velocity change by the tau the place implies:
  1e-4 of the aberration acceleration, 6.8e-9 deg/day on Mercury
  geocentric, 5.7e-8 topocentric;
- with `SEFLG_CENTER_BODY`, the centre-of-body offset was in x(t) already
  and `calc_center_body()` added it again at t - tau: 4.66e-7 AU on
  Jupiter.

**Fix, as done here:** the planets take the files' two passes and are read
at t - tau, with or without `SEFLG_SPEED`; the series' Earth there only
for the speed. With the speed, this is the call that was already made;
without it, one planet series (~1.9 us). The Moon and the Earth are
carried from teval to the instant the files are read at, t = teval - tau
rounded, by x + s v + s^2 a / 2 and v + s a, s = t - teval exactly, and
the files' rounding correction (entry 23) now applies to Moshier too. The
accelerations are the Earth-Moon pair's pull and the Sun's tide on the
Moon, the Sun's pull on the barycentre less the Earth's share of the Moon's
(`mosh_moon_earth_acc()`): 4e-5 of the series' own second differences
(the planets, the Earth's figure), so 1e-18 AU and 1e-13 AU/day from the
series read at t over 1.3 s. Re-reading the Moon there would cost its five
series evaluations and an Earth, ~10% of calc-moseph. The speed carries
1 - dtau/dt from `lt_rate_exact()` and the Moon's entry-32 term as on the
files. Cost: bench calc-moseph min 1.050 s against 1.034 (+1.5%, the bench
binary's layout: linked against libswe.a the same ten bodies take 170 us a
date against 172, each body alone unchanged within 1%), moon 0.827
against 0.829.

Positions move by up to 13.0 mas (Mercury), 3.4 (Venus), 1.3 (Mars), 0.13
(Jupiter), 0.05 (Saturn), 0.09 (Chiron); the Moon 0.003 mas geocentric,
0.51 heliocentric. Speeds: the Moon's by 1.0e-4 deg/day without
aberration, 4.8e-5 heliocentric, 1e-6 topocentric; Mercury's by 1.7e-7;
heliocentric distance rates by 5.6e-6 AU/day (Mercury). Against the Swiss
files, 241 epochs 1800-2100, J2000: the Moon's longitude speed without
aberration 1.18e-4 rms to 9.72e-5, now the same as with it (the theory's
own); heliocentric 3.15e-5 to 7.3e-7; Mercury's heliocentric distance rate
4.46e-6 to 7.3e-9 AU/day, Mars's 1.12e-6 to 8.1e-9. Left: the heliocentric
Moon's 4.7e-8 deg/day, the second order's truncation over 500 s (the
files 5e-10); Mercury's 0.009 mas heliocentric from the two straight-line
passes and the instant's rounding, the same as the files (0.005).

G52 `tests/moshlt.c`, Moshier, 24 dates 1800-2100: the astrometric
direction equals P(t - tau) - O(t) from the library's own geometric
vectors within 0.05 mas (<= 0.0088) and does not depend on `SEFLG_SPEED`,
Mercury, Venus, Mars, Jupiter, Pluto and Chiron, geocentric and
heliocentric; 13 speeds against five-point differences, the Moon
geocentric with and without aberration, topocentric with and without,
heliocentric, Mercury and Mars geocentric with and without aberration and
heliocentric, their heliocentric distance rates (the Moon <= 4.4e-7
topocentric, 4.7e-8 heliocentric; the planets <= 3.6e-8; distance rates
<= 2.9e-11 AU/day); the aberration's part of Mercury's and Mars's speeds,
geocentric and topocentric, within 3e-9 (<= 1.4e-9; 5.7e-8 before); and
Jupiter's centre-of-body offset equal to the files' (7.8e-15 AU). The
upstream build fails 27 of the 42. G33 and G40 now hold Moshier to the
files' limits (0.00035 mas, 4.8e-7 deg/day geocentric, 1.7e-6
topocentric). G24 `mosh_lt_off`: Mercury's heliocentric distance rate,
1.1e-11 AU/day shipped, 3.6e-6 compat. Since Moshier now reads the planets
at the rounded t = teval - dt the files use, entry 63's correction for that
rounding (the body moved along its velocity by (teval - t) - dt) applies to
Moshier too.

## 56. The interpolated apsides stop short of their iteration's fixed point

**Severity: Correctness (the perigee up to 1.21" from the point its iteration converges to, the apogee 2.5 mas)**
**Where:** `swemmoon.c`: `swi_intp_apsides()`

The iteration (entry 45) steps along a line in the lunar arguments, the
mean anomaly moving by `off` and the others by `off / f`, f their periods
in anomalistic months. But each pass starts its line from a centre where
the Moon's four arguments have all moved by the whole step and the Sun's
and planets' are back at J, so the centre moves along another line than
the one the step was taken on. The fixed point, where the distance's
slope along the line is zero, comes only a fraction 1 - q nearer each
pass: q = -0.04..+0.15 on the perigee, -0.013..+0.024 on the apogee
(100000 dates). Five passes (perigee) and four (apogee) left the
position up to 1.21" and 2.46 mas from it (20000 dates -10000..+12000;
rms 0.12" and 0.43 mas), 1.16" and 2.34 mas over the -3000..+3000 that
`swe_calc()` serves (entry 45's 1.08" was another sample of dates).

**Fix, as done here:** the Newton steps (entry 45's, from five points
180" apart) are scaled by 1 / (1 - q), q a secant on the last two: N_k
the plain step at the k-th centre, D_k the step taken to it, q = 1 -
(N_{k-1} - N_k) / D_k, damped by D_k^2 / (D_k^2 + (1e-4")^2) so that
where the point is already within the step's noise (~1e-6") q is not
noise over noise. The perigee keeps its three parabolas (5 deg, 0.5 deg,
180") and takes three such passes, its curvature from the 180" parabola,
so a pass costs four lunar evaluations; the apogee takes two parabolas
and two passes, the first with a centre point for the curvature (its
180" parabola dropped: the Newton pass from the 0.5 deg one converges as
well). The light-time estimate's passes (entry 44) are the first
parabolas, unchanged. Against a reference with fourteen more plain
Newton passes, max over the same 20000 dates: 0.0036 mas (perigee),
0.0024 (apogee), rms 4.5e-4 mas, the reference's own noise. Positions
are still identical with and without SEFLG_SPEED and as smooth (64
consecutive JDs at 200 epochs: rms 2.4e-4 -> 2.6e-4 mas on the perigee,
2.1e-4 the apogee; worst 2.6e-3..3.2e-3 mas before and after). Lunar
evaluations per iteration: 22 for the perigee (20 before), 16 for the
apogee (15), besides entry 44's 4 for the light time; instructions per
`swe_calc()` at instants 1.37 day apart +5.7% (perigee, 1.47e6 ->
1.55e6), +3.4% (apogee, 1.26e6 -> 1.31e6), +8% and +5% with SEFLG_SPEED
(entry 44 has the cost against upstream). The positions move by up to 1.2"
(perigee) and 2.5 mas (apogee), the perigee's speed by up to 2e-4
deg/day (the rate of what moved).

SEFLG_J2000's latitude speed, the other half of entry 45's note: 4.6e-7
deg/day on the interpolated apogee at -3000 and 7.1e-7 on the mean
apogee at 12000 when measured, from `swi_precess_speed()`'s rate of the
precession. Entry 49's fix of that rate already made it <= 2.7e-9
(interpolated apsides, -3000..+3000) and 2.1e-8 (mean apogee,
-10000..+12000) against five-point differences; nothing changed here.

G55 `tests/intpconv.c`: (a) the perigee and apogee at the eight dates in
-3000..+3000 where the old iteration stopped farthest from its fixed
point, and two modern ones, TRUEPOS and NONUT, against the fixed point
within 0.01 mas (2.0e-3 perigee, 1.2e-3 apogee; before 1163 and 2.3,
upstream 1164 and 2.4); (b) the J2000 latitude speeds of the interpolated
apsides and the mean apogee against five-point differences within 1e-7
deg/day (2.4e-9, 2.7e-9, 2.1e-8; upstream 3.4e-7, 4.5e-7, 7.6e-7).
The upstream build fails 63 of 68. G24 `intp_fixpoint_off`: the perigee's
longitude at JD 687314.573 from its fixed point, 3.4e-4 mas shipped,
1.17e3 compat. Baselines: interpolated-perigee positions move by up to
1.2", the apogee's by 2.5 mas.

## 66. The aberration rate's Earth lacks the Moon on JPL and Moshier, and the planets everywhere

**Severity: Correctness (2e-7 to 6e-7 deg/day on a far fictitious body's longitude speed on JPL and Moshier)**
**Where:** `sweph.c`: `earth_at_lt_sun()`

Entry 62 added the Moon's pull on the Earth to the aberration rate's
tau a(teval) term from the geocentric Moon in the Swiss save area. JPL
(`SEFLG_JPLEPH`) and Moshier have no Moon there and kept the Sun-only a' and
the 10-day switch. Measured on ts20 against five-point differences of the
same call's positions over 2^-7 day, seven dates, de200.eph (1650..2150) and
Moshier (-300..+300 years): longitude speed Nibiru 2.2e-7 deg/day
geocentric (2.3e-7 topocentric) on JPL, 8.2e-8 (8.6e-8) on Moshier; the
Christ Comet 3.9e-7 and 5.9e-8; Planet 9 3.3e-7 and 1.4e-7; VEA 5.6e-7 and
5.9e-7; Planet 9's latitude speed on JPL 1.5e-7. Pluto and Harrington, under
0.3 day of light time, 1.4e-8 to 3.6e-8.

**Fix, as done here:** past a light time of 0.3 day, where the Moon's pull
is taken whole, the geocentric Moon at teval is read where the save area has
none: `swi_pleph(J_MOON, J_EARTH)` on JPL, `swi_moshmoon()` (NO_SAVE) on
Moshier. The same 0.3-day switch as the Swiss path then applies. Nearer
bodies read nothing and keep the Sun's a' (the Moon's 7.5% of it is
tau omega / 2 * 0.6% < 2e-4 of tau a at 0.3 day), so the planets Sun to Pluto
(0.29 day at most) pay one comparison. Measured after: Nibiru 7.4e-9
geocentric JPL, 2.0e-8 topocentric, Moshier 7.6e-9 and 1.1e-8; the Christ
Comet, Planet 9, VEA 2.8e-8 or less everywhere. Harrington (light time under
0.3 day, so unchanged) stays at 1.9e-8 geocentric and 3.5e-8 topocentric on
JPL, a little above the Swiss 8e-9 for the missing 7.5% of a'. Bench
calc-swieph/calc-moseph: the bodies never reach the new branch; the runs
differ by load noise (moseph min 1.055 s after, 1.026 before, load 5-7).

**Part 2 (the residual, and the planets).** (a) Below 0.3 day of light time
JPL and Moshier still lacked the Moon's 7.5% of a': Harrington 1.4e-8
deg/day, Pluto 1.7e-8. The Moon is already in hand there: the JPL Earth is
the EMB less the Moon's share (`swi_pleph` keeps that geocentric Moon,
`jplplan` saves it with the Earth), and Moshier's `embofs_mosh_rate` differences
the Moon-offset series at teval +-2^-8 day, whose mean and quotient are the
Moon's place and velocity. Both are kept in `ctx->earth_moon` for the saved
Earth and serve every light time (no extra read; the 0.3-day read stays as a
fallback for an Earth not saved at teval). (b) On every ephemeris, past 0.3
day tau a took the Sun's and Moon's pull only: the planets' pull on the Earth
(Jupiter 1.6e-8 AU/day^2, Venus as much at its nearest) was 4e-9 deg/day on
the far bodies (Nibiru, Planet 9, VEA, Christ Comet, Kronos; Zeus 3e-9 on
JPL), Swiss files included. `planets_pull()` adds Mercury, Venus, Mars,
Jupiter and Saturn from Moshier's heliocentric places (Uranus and up: under
1e-10), only for bodies past 0.3 day. Measured after, geocentric/topocentric:
Swiss <= 8.3e-10, JPL <= 1.0e-9 (Harrington 4e-10/1.0e-9, Pluto 4e-10); Moshier
Harrington 3e-10 to 1.1e-9 and Pluto 3e-10 to 1.6e-9, but the far bodies stay at
2e-9 to 6e-9: that is the Moshier Earth's own acceleration (the series' small
monthly terms), not the model; a full Moshier Moon in place of the short one
gives the same. Bench calc-swieph/calc-moseph/jpl-interp: within noise (min of
two runs 0.5567/0.5549 s, 1.0298/1.0376 s vs 0.5485/0.5495, 1.0361/1.0526
before). Left out below 0.3 day: the planets' jerk, 8e-6 of tau a.

**Part 3 (the Moshier floor).** The far bodies stayed at 2e-9 to 6e-9 on
Moshier: the Earth the series returns accelerates differently from the
gravitational sum (its small monthly terms: 1e-4 of a; a full Moshier Moon
gave the same numbers). Past 0.3 day `swi_mosh_earth_accel()` now takes a as
the second difference of the Earth's position (EMB series plus the Moon
offset) over +-2^-8 day at exact instants (truncation 7e-8 of the Moon's
part, rounding 7e-12 AU/day^2 against 3e-4), from the saved Earth at teval;
the planets' sum is not needed there. Measured after: Moshier <= 1.06e-9
(Harrington topo 1.06e-9, Pluto 8.6e-10; the far bodies 5e-10 to 6.5e-10 topo,
1.2e-10 geo). Swiss and JPL with the same own-a (JPL: the Earth's velocity
from the file differenced over the same step) reach 1.0e-9 as well, which is
what the gravitational sum gives them, so they keep the sum (no extra reads).

G65 `tests/lunarpulleph.c`: Nibiru, Harrington, the Christ Comet, Planet 9,
VEA, Pluto, Zeus and Kronos, geocentric and topocentric, Swiss files, JPL and
Moshier, seven dates 1650..2150, longitude and latitude speeds within 2e-9
deg/day (measured <= 1.06e-9). The build before part 3 fails Moshier's far
bodies (2e-9 to 6e-9) and Harrington/Pluto before part 2 (1.4e-8 to 1.7e-8),
Swiss at 4.6e-9; the build before part 1 fails the far-body rows at 5.9e-8 to
6.0e-7; the upstream build fails them all. G24: `far_body_moon_pull_eph_off`,
VEA's longitude speed on JPL and Moshier, 2.9e-11 shipped, 3.4e-5 compat.

## 69. The nutation matrix's rate holds the mean obliquity still

**Severity: Correctness (2.0e-9 deg/day in the right ascension speed of a star near the pole; 4e-10 in the ecliptic longitude speed near the ecliptic's pole; 2e-11 in every latitude and declination speed of date)**
**Where:** `sweph.c`: `swi_nutate()`, `swi_check_nutation()`

Entry 49 put the nutation's own turning into the velocity as w x x, w
the angular velocity of N = R1(-eps - deps) R3(-dpsi) R1(eps) from the
angles' rates (entry 58 made those the rates at t): w = deps' e1 + dpsi'
(0, -sin(eps + deps), cos(eps + deps)), the mean obliquity eps held. It
moves, 3.6e-7 deg/day, and R1(-eps - deps) and R1(eps) do not cancel
when dpsi != 0: w left out eps' (e1 - R1(-eps - deps) R3(-dpsi) e1) =
eps' (1 - cos dpsi, -sin dpsi cos(eps + deps), -sin dpsi sin(eps +
deps)), up to eps' x 8e-5 rad = 3e-11 deg/day. Measured as G57 does
(speed of date less the NONUT speed, against the five-point difference of
the same two calls' positions) over 2^-6 day, seven dates 1900-2100:
Polaris's right ascension 1.88e-9, Polaris Australis's 2.04e-9 (times
1/cos(declination)), delta Dor's ecliptic longitude 4.06e-10, 42 Dra's
1.96e-10, the polar stars' other longitudes ~7e-11, latitudes and
declinations of the planets and stars 1.3e-11 to 2.8e-11. The same
1.88e-9 over 2^-8 and 2^-6 day: not the reference's noise. The rate of
N by a central difference of its nine entries (40 digits, eps, deps and
dpsi all moving, dpsi = 8e-5) agrees with the w below to 1e-30 rad/day;
the old w is short by 4.6e-13 rad/day, eps' sin(dpsi) cos(eps + deps).

**Fix, as done here:** w gains the eps' terms above. eps' is the mean
obliquity's rate from `swi_obliquity_rate()` (its one-day difference of
`swi_epsiln()`), taken once per fresh instant in `swi_check_nutation()`
beside the angles' rates and kept as `nut.deps0`. Positions unchanged.
Measured after: Polaris's right ascension 1.1e-11, Polaris Australis's
1.2e-11, delta Dor's ecliptic longitude 1.2e-11, 42 Dra's 8.6e-12, every
latitude and declination <= 1.2e-12 (the difference's own rounding:
Polaris's right ascension 6.2e-11 over 2^-8 day, 1.1e-11 over 2^-6 and
2^-5); the planets' longitudes 1e-12..5e-12, unchanged. bench
calc-swieph/calc-moseph no slower (min of 3x7: 0.553/1.038 s against
0.553/1.029). G57's Polaris
right ascension 1.82e-9 -> 6.2e-11 (its limit 3e-9 kept), G24
`nut_rate_off` 1.0e-9 -> 5.75e-11; G49's angles 3.9e-11 -> 1.9e-11, its
XYZ rates 7.0e-13 -> 1.8e-13 of the distance per day; G41 within
2e-11 either way (Aldebaran's latitude 7.2e-12 -> 1.3e-12).

G68 `tests/nutobliq.c`: Polaris, Polaris Australis, delta Dor, 42 Dra,
the Sun, Mars, Saturn and Pluto, ecliptic and equatorial, longitude and
latitude, the nutation's share over 2^-6 day at seven dates: 32 checks,
longitudes within 3e-11 deg/day, latitudes within 5e-12. Before the fix
20 of 32 failed; the upstream build fails all 32 (bug 58 besides). G24
`nut_obliq_rate_off`: Polaris Australis's right ascension, the worse of
1900 and 2000, 6.6e-12 deg/day shipped (2.04e-9 before the fix), 8.2e-9
compat.

## 70. `swe_nod_aps()`'s apparent speeds are not the rate of the aberration it applies

**Severity: Correctness (1.6e-8 deg/day on Neptune's mean points' geocentric longitude, 5.7e-9 on Mars's; 2.1e-9 AU/day on Neptune's heliocentric distance rate)**
**Where:** `swecl.c`, `swe_nod_aps_r()`, the aberration of the points (every
point and method; measured on the planets' mean ones); `sweph.c`,
`swi_aberr_rate()` (replaced)

The points carry no light time: a point x(t) seen from the observer, then
aberration A(x, V) by the observer's velocity V(t). Its rate is A's
derivative along x' and along V' = a, the observer's acceleration. The
speed was `swi_aberr_light()`'s (the point moved, V held) plus bug 34's
term tau a, a's part across the line of sight -- A to first order in v/c
-- with tau a's series taken while the save area held the instant
t - dt, so without the Moon (bug 62) in the Earth's jerk: tau/2 of the
Moon's 7.5% of it left in a. Seen from the Sun (heliocentric, and the Sun's
own points) it was the raw V(t) - V(t - dt), its radial part too, which
moves the distance rate. Measured with default flags against five-point
differences of the library's own positions, mean nodes, perihelia and
aphelia of Mercury..Neptune, thirteen instants -3000..+3000: geocentric
longitude off by 5.2e-9 (Mercury) to 1.6e-8 (Neptune) deg/day, latitude by
up to 1.3e-9 (Neptune); heliocentric distance by 1.3e-9 AU/day (Uranus) and
2.1e-9 (Neptune). With SEFLG_NOABERR the same cases are within 5.3e-10,
3e-11 and 2.3e-11; the deflection's rate (SEFLG_NOGDEFL) moves nothing.

**Fix, as done here:** the position is `swi_aberr_light()`'s, unchanged to
the bit (with and without SEFLG_SPEED, every point, method and frame
compared). The velocity is the central difference of the aberrated place,
A(x + s x', V + s a) at s = +-2^-6 day, x with its velocity before
aberration. a is the new `swi_observer_accel_lt()`'s (the Earth's, from
`earth_at_lt_sun()`'s tau a over tau, plus the site's for SEFLG_TOPOCTR),
taken after the save area is back at tjd so that the Moon is in it; seen
from the Sun, V(t) - V(t - dt) over dt. After, same instants: geocentric
longitude <= 5.3e-10 deg/day (Mercury at -3000, the floor without
aberration), Neptune 3.5e-10; latitude <= 4.4e-11; distance <= 2.2e-11
AU/day. The speeds of the other points through the same code move by up to
2.6e-8 deg/day, toward their positions' rates: the osculating points' (Mars
geocentric 1.2e-8 to 6.8e-9 deg/day, Pluto 2.1e-8 to 2.0e-9), the Moon's
heliocentric mean points' distance rate (5.8e-11 to 2.7e-14 AU/day), the
topocentric mean points' (Neptune 3.2e-8 to 1.6e-9 deg/day).

G69 `tests/nodapsaberr.c`: the fourteen cases of G63 (Mercury..Neptune,
geocentric and heliocentric) with default flags, seven instants
-3000..+3000, longitude, latitude and distance speeds within 1e-9, 1e-10
deg/day and 5e-11 AU/day (measured 5.3e-10, 4.4e-11, 2.2e-11); the build
before this fix fails 20 of the 42 checks, the upstream build all 42.
G54's Mars longitude limit is 5e-10 now, from 1e-8 (measured 4.1e-11, from
5.6e-9). G24 `planet_aberr_rate_off`: Neptune's mean aphelion's
heliocentric distance rate at J2000 + 100 days, 2.2e-13 AU/day shipped,
3.3e-9 compat. G42, G47, G54, G63 pass.

## 65. Kronos' built-in semi-axis has two digits swapped, and the Kepler solver's starting guess is wrong

**Severity: Correctness (Kronos 0.0086 deg at 1700, 0.0169 deg at 2300 without seorbel.txt; the second part moves no result beyond rounding)**
**Where:** `swemplan.c`: `plan_oscu_elem[]` (SE_NEELY block), `swi_osc_el_plan()`

Without `seorbel.txt` on the path the fictitious bodies come from a built-in
table. Its Neely row for Kronos has the semi-axis 64.81960; `seorbel.txt`
(Neely's, "refined by James Neely") and Witte's original 64.816896 (the
non-Neely rows of the same table) give 64.81690. The repo holds no Neely
paper to check the digits against; the file and the original elements agree
with each other, and the other fourteen built-in rows agree with the file to
the last digit (compared row by row), so the value is the swap. Kronos'
mean motion is off by 0.0043 deg per century from the 1900 epoch:
-0.0086 deg at 1700, +0.0169 deg at 2300, 0.0257 deg and 2.8e-3 AU at 2500.

The solver of `swi_osc_el_plan()` for eccentricities above 0.975 (Nibiru,
the Christ Comet) builds a starting guess near perihelion with
`zeta = pow(x, 1/3)`, where `1/3` is integer division, so `zeta = pow(x, 0)
= 1`, and adds `M_180_or_0 = 180` to an angle in radians where 180 degrees
is pi. `swi_kepler()` converges to the same root from either guess, so the
results move only by rounding. Measured over 20000 dates 1699 to 2301
(Swiss, geocentric, with speed): Nibiru differs by 2.8e-14 deg in longitude,
1.4e-14 in latitude, 6.1e-11 deg/day in longitude speed, 3.1e-10 AU/day in
distance speed; the Christ Comet by 8.9e-15 deg, 1.9e-11 deg/day.

**Fix, as done here:** the semi-axis is 64.81690. The guess is
`pow(x, 1.0 / 3)` and `M_PI`. The solver now needs fewer passes: Nibiru
383199 to 295420 for 59947 calls (6.4 to 4.9 per call), the Christ Comet
512773 to 179943 for 59981 (8.5 to 3.0). Cost is below any bench noise.

G64 `tests/fictbuiltin.c`: the 15 built-in bodies at five dates 1700..2500,
heliocentric Moshier, with the shipped `seorbel.txt` against an empty
directory (the built-in table): 0 deg, 5.7e-14 AU; the upstream build fails
on Kronos alone (0.0086 deg at 1700, 0.0169 deg at 2300, 0.0257 deg at 2500).
G24 `kronos_builtin_off`: Kronos at 1700, built-in against the file, 0 shipped,
8.7e-3 deg compat. The starting-guess part is not gated: no public result
moves beyond rounding, and the pass count has no clean hook.

## 73. The Moon's speed is not the rate of its position: the site's turning, the files' TDB rate, the light-time velocity, the aberration's shift rate

**Severity: Correctness (8.7e-7 deg/day on the topocentric Moon's longitude speed, 3.7e-7 on its latitude speed; 2.4e-8 on the geocentric Moon's; every topocentric body's site velocity 2-6 m/day off)**
**Where:** `sweph.c`: `swi_get_observer()`, `sweph()`, `app_pos_etc_moon()`, `swi_aberr_light()`; `swejpl.c`: `state()`; `swephlib.c`: `swi_tdb_minus_tt_rate()`

On the Swiss files the Moon's topocentric longitude speed was off a
five-point difference of its positions by up to 8.7e-7 deg/day (latitude
3.7e-7), the same with SEFLG_NOABERR, SEFLG_TRUEPOS or SEFLG_NONUT, and
the same at every step from 2^-6 to 2^-11 day; geocentric 2.4e-8 (1.4e-8
without aberration, 5e-9 for true positions). Ten epochs -2900..2900,
binary-fraction instants. Taken apart, four causes:

1. The site (`swi_get_observer()`). Against nine-point differences of the
   observer itself it was 2.2-4.6 m/day off. It is placed by sidereal time,
   its angle from the moving equinox of date, but turned at
   EARTH_ROT_SPEED, 7.2921151467e-5 rad/s, the rate of the Earth rotation
   angle (1.0027378119 turns a day, against inertial space), where
   sidereal time turns 1.0027379093 a day; `swi_precess_speed()` then adds
   the equinox's own motion, so the precession in right ascension counted
   twice (9.7e-8 of the rotation, 2.6 m/day at 47N, up to 6.2e-7 deg/day on
   the Moon). And the true pole's turning was missing: the equation of the
   equinoxes' rate in the rotation, and the nutation matrix's rate on the
   site (the code's comment had it as "micrometres a day"; it is 1-3 m/day,
   each up to 9.8e-7 deg/day on the Moon, partly cancelling). Upstream has
   both (it never nutated the site, every caller passing SEFLG_NONUT).
2. The files' rate (`sweph()`, `state()`). Read at TDB since entry 24, the
   Chebyshev derivative is per day of TDB, the positions are functions of
   TT: short by d(TDB - TT)/dTT, up to 3.3e-10 of every speed, 5.1e-9
   deg/day of the Moon's (the whole true-position residual, on JPL as on the
   Swiss files). Not in upstream, which reads at TT.
3. The light-time velocity (`app_pos_etc_moon()`). Entry 23 moves the Moon
   read at teval - dt to teval - tau along its velocity, but left the
   velocity the one at teval - dt: dt - tau is the barycentric range rate
   times tau / c, up to 2e-9 day, which the barycentric Moon's acceleration
   makes 0.07 m/day, 1.0e-8 deg/day. And the aberration rate term
   xobs - xobs2 was the observer's velocity change over teval - (teval - dt),
   not over tau: 1e-4 of the term, 1.0e-8 geocentric, 2.4-4.0e-8
   topocentric.
4. The aberration's shift along the body's motion (`swi_aberr_light()`)
   was a backward difference over 1e-4 day, the rate at t - 0.5e-4: the
   Moon's place turns at 0.23 rad/day, so its 20" shift curves, 1.9e-8
   deg/day geocentric, 2.8e-8 topocentric. Upstream has it.

The topocentric residual left is the site's own jitter: UT1 = TT - delta t
rounds to 4.7e-10 day, 6 mm on the site, 1e-9 deg on the Moon, which a
five-point difference over 2^-8 day reads as 2.5e-7. With delta t set to
2^-11 day (UT1 exact) and a 17-point difference over 2^-5 day the shipped
topocentric Moon is within 9.5e-10 deg/day, the build before within 6.2e-7.

**Fix, as done here:** (1) the site turns at sidereal time's rate: the
rotation angle's, 2 pi x 1.00273781191135448 per day of UT1, times
dUT1/dTT (entry 36), plus the precession in right ascension's rate,
-(dM/dt M^T)[0][1] from the precession matrix and its rate (what the
precession's rate takes off again; the same matrices then precess the site,
`swi_precess()`'s to the bit, so the matrix is computed once, not twice),
plus d(dpsi cos eps)/dt; the nutation matrix's rate, -(N^T w) x x_mean as
in `swi_nutate()`, is added to the site's velocity. Positions unchanged to
the bit. Site velocity after: 0.21 m/day, the differences' own rounding.
(2) `swi_tdb_minus_tt_rate()`, the analytic derivative of the seven-term
series, scales the Swiss files' and the JPL file's rates (`intv` in
`state()`); TDB - TT and its rate cached for the last two instants, the
value bit-identical. (3) The Moon's velocity moved with its place by
e (v(teval) - v(t_read)) / (teval - t_read), and the rate term's
xobs - xobs2 scaled from teval - t_read to tau, on every ephemeris
(entry 53 carries Moshier's Moon to t_read as well). (4) The shift's rate
is the analytic derivative of A(u) = (b u + f2 |u| v) / (1 + f1) along u'.

Measured after, against 17-point differences (noise-free with delta t
2^-11 day for the site): geocentric 2.2e-10 deg/day with or without
aberration (before 2.4e-8, 1.5e-8), topocentric 4.6e-10 to 9.5e-10 at 0,
47.37 and -65 deg (before 5.1e-7 to 7.3e-7). Moshier: 2.3e-9 geocentric,
2.8e-9 topocentric (before 2.0e-8, 7.4e-7), the series' own roughness
(3.2e-9 for true positions) left. Removing any one part brings back its
share: the precession 4.2-6.2e-7, the equation of the equinoxes'
rate 9.8e-7, the nutation matrix's rate 9.7e-7, the TDB rate 5.1e-9, the
light-time velocity 1.0e-8, the rate term's tau 1.0e-8 (4.0e-8 topo), the
shift's rate 1.9e-8 (2.8e-8 topo). JPL (de200.eph, 1650..2150): geocentric
4.7e-11 (before 2.2e-8), true positions 2.1e-11 (5.1e-9), topocentric with
exact UT1 6e-10 (6.2e-7). Every body's speed moves by the TDB rate and
the shift's rate (Venus 6e-10 deg/day) and every topocentric one by the
site (Venus 4.8e-9); every apparent position by 2e-12 deg (geocentric) to 9e-12 (topocentric), through
the observer's velocity in the aberration. The osculating apogee and the
true node, which `lunar_osc_elem()` builds from the Moon's position and
velocity, move by up to 9.7e-7 deg (0.0035") with the Moon's velocity, in
true and no-aberration requests too (swemini's "osc. Apogee" 252.2333404
-> 252.2333407); barycentric places without speed by 1-2 ulp; on the JPL
reader only the rates, by 6e-11 to 3.3e-10 of themselves (d(TDB - TT)/dTT;
G10's positions bit for bit). Found in the G1/G10/G17 re-baseline. Cost: calc-swieph +0.9%
instructions, +1.1% cycles (median of five, `perf stat`, load 5-6; the
minimum 0.6% lower), calc-moseph unchanged; a topocentric `swe_calc()` 4%
fewer instructions than before, the precession matrix no longer computed
twice for the site.

G72 `tests/moonsitespeed.c`: the Moon's longitude and latitude speeds,
Swiss files, with and without SEFLG_NOABERR, ten epochs -2900..2900,
against five-point differences: geocentric over 2^-6 day within 5e-9
(1.41e-9), topocentric at 0, 30, 47.37 and -65 deg over 2^-8 day within
4e-7 (2.51e-7), the same with delta t 2^-11 day over 2^-9 day within 3e-8
(1.05e-8). The build before fails all three (2.40e-8, 8.69e-7, 7.53e-7),
the upstream build all three (6.86e-5, 1.66e-3, 2.33e-3). G24
`moon_site_speed_off`: the geocentric apparent and the topocentric true
Moon's longitude speed at 500 and 2000, 2.39e-9 deg/day shipped, 9.65e-4
compat.

## 76. A fixed star's aberration rate takes the Earth's acceleration half a step back

**Severity: Correctness (8e-12 deg/day on the sky in every apparent star speed; 1.03e-9 in Polaris's right ascension speed, 7.4e-10 in Polaris Australis's, 2.2e-10 in delta Dor's ecliptic longitude speed; on Moshier 2e-10 on the sky, 1.56e-8 in Polaris's right ascension)**
**Where:** `sweph.c`: `fixstar_obs_rate()` (was `fixstar_obs_ahead()`), `fixstar_calc_from_struct()`, `swi_fixstar_calc_from_record()`, `main_planet_bary()`

After entry 69, Polaris's right ascension speed was still off a
five-point difference of its right ascension by 1.03e-9 deg/day, and
Polaris Australis's by 7.4e-10, the same at every step from 2^-5 to
2^-10 day: not the reference's rounding. Taken apart one correction at a
time (seven dates 1900-2100, 2^-6 day): SEFLG_NONUT, SEFLG_NOGDEFL and
SEFLG_J2000 left it (J2000 still 5.8e-10), SEFLG_NOABERR and
SEFLG_TRUEPOS took it to 2.8e-11, J2000|NOABERR to 7.7e-13: the
aberration. Every star had it, about 8e-12 deg/day on the sky in both
coordinates (delta Dor's and 42 Dra's ecliptic longitudes 2.2e-10 and
1.9e-10, near the ecliptic's poles).

Entry 49's `aberr_light_central()` takes the star's aberration rate as
the change of the shift from t - dm to t + dp over dm + dp, dm and dp the
exact 1e-5 day steps. The observer's velocity at t + dp came from
`fixstar_obs_ahead()`, the Earth's carried on linearly from its
velocities at t - dm and t: (v(t) - v(t - dm)) (dm + dp) / dm for the
change, the acceleration at t - dm/2, short by dm/2 of its rate of
change. For the Earth's orbit that is dm/2 x 2 pi/year = 8.6e-8 of the
acceleration, and so of the aberration's rate (20.5" x 2 pi/year =
9.8e-5 deg/day): 8.4e-12 deg/day, 1/cos(declination) of that in a right
ascension near the pole. The function's own comment had it: "linear to
1e-7 of itself".

A first fix read the Earth at t + dp from the ephemeris: Polaris's right
ascension 1.8e-11 over 2^-5 day (the files' velocities 1e-5 day apart
are good to 4.3e-9 of the acceleration), but a third Earth read on every star
call, `swe_fixstar2()` with SEFLG_SPEED +5.7% in instructions (+9% in
time), and Moshier still off. On Moshier (SEFLG_MOSEPH) Polaris's right
ascension speed was off by up to 1.56e-8 deg/day (2.0e-10 on the sky),
the same at 2^-5 and 2^-7 day, 7e-12 in 1900 and 2.1e-9 to 1.56e-8 at
1970-2100, 8.6e-12 under SEFLG_NOABERR. The Moshier Earth's velocity is
rough: its acceleration as a central difference of its velocities over
+-1e-5 day is off a five-point one over 2^-4 day by up to 2.1e-6 of
itself (1.1e-9 in 1900; 3.5e-10 to 4.3e-9 on the Swiss files), over +-2^-9 still
1.7e-8 -- velocity noise of ~2e-14 AU/day from one instant to the next.
It is the Earth-Moon offset's rate (entry 51), a central difference of
the short lunar series over +-2^-8 day whose arguments, in degrees of
481267.88 T, round at 1e-12 rad once T is near 1: with that rate taken
over +-2^-2 day (an experiment, not kept) the noise goes, and what is
left, 1e-5 day apart, is the EMB series' own (up to 9e-8 of the
acceleration).

**Fix, as done here:** `fixstar_obs_rate()` gives aberr_light_central()
the observer's velocity at t - dm and t + dp as the Earth's at t carried
with its acceleration at t, v(t) -+ d a(t) (and the site read at both
instants), a(t) from two NO_SAVE reads at t +- h, exact instants, which
replace the read at t - dt: on the Swiss and JPL files the velocities'
central difference over h = 2^-9 (truncation h^2/6 a'', <= 5e-10 of a,
the Moon's month); on Moshier (the ephemeris the read at t used, so a
substituted one too) 2B - A over h = 2^-3, B the positions'
second difference and A the velocities' central one, whose h^2 terms
cancel (h^4/360 of the fourth derivative is 1e-11) and which divides the
velocities' roughness by 2h, so the offset's rate needs no change (the
light-time code's Moshier Earth, `earth_at_lt_sun()`, is another
entry's). The results are flat in h: on Moshier 2B - A over 2^-4..2^-2
and a central difference over 2^-7 all within 3.5e-11 in Polaris's right
ascension except 2^-4 at 2470000.7 (8.5e-11); on the files h =
2^-10..2^-8 and 3e-5 within 1.4e-11. Measured after, over 2^-5 day: Swiss
Polaris 5.2e-12, Polaris Australis 1.2e-11, delta Dor 1.05e-11, 42 Dra
4.8e-12; Moshier 1.58e-11, 1.82e-11, 1.07e-11, 7.5e-12; JPL (DE200)
Polaris 7.0e-12. Topocentric on Moshier is now Swiss's (over 2^-8 day,
4.8e-10 against 1.59e-8, the rest being the diurnal term's truncation
there). Positions bit for bit unchanged, both functions, seven stars,
nine flag sets. (Entry 85 made the Moshier Earth's velocity smooth at its
source; the star side then took the files' form on Moshier too.)

Cost: the two reads at t +- h skip the barycentric Sun (they want only
the Earth's velocity; `main_planet_bary()` now takes a NULL Sun on
Moshier as `sweplan()` and `jplplan()` already do), and are made only
when the caller asked for SEFLG_SPEED and the star is aberrated. Three
reads cannot become two: an acceleration at t to 1e-9 needs the Earth at
two instants besides t (one-sided, from t and t - h, it is the
acceleration at t - h/2, or with positions a Hermite fit whose rounding
is 6 ulp/h^2, 7e-8 at h = 2^-7), and there is no other Earth read in a
star call to borrow -- no light time, and the read at t is the saved one
the deflection uses. The read at t - dt is the one moved. Instructions
per `swe_fixstar2()` call, Swiss: with SEFLG_SPEED 170.8k before this
entry, 180.6k with the first fix, 172.0k now (+0.7%); without it 160.6k
(-6%). Moshier with SEFLG_SPEED 246k -> 293k (+19%: the EMB series read
is most of the call), without it 200k (-19%). `swe_calc()` untouched.
G49's angles 4.0e-11 -> 3.5e-11, the apparent ones 1.9e-11 -> 9.5e-12,
its angle limit 2e-10 -> 1e-10; G68 within its rounding either way (it
takes the nutation's share, in which the aberration cancels), limits
kept.

G75 `tests/staraberr.c`: Polaris and Polaris Australis in right
ascension, delta Dor and 42 Dra in ecliptic longitude, apparent
geocentric, `swe_fixstar2()` and `swe_fixstar()`, Swiss and Moshier,
seven dates 1900-2100, against five-point differences over 2^-5 day: 16
checks within 6e-11 deg/day (measured <= 1.82e-11). The build before the
fix fails all 16 (Swiss 1.9e-10 to 1.03e-9, Moshier 1.1e-8 to 5.1e-8);
the first fix fails the 8 Moshier ones (2.4e-9 to 1.57e-8); the upstream
build fails all 16 (2.6e-6 to 5.7e-3, bugs 49 and 58 besides). G24
`star_aberr_accel_off`: Polaris's right ascension speed, Swiss in 2100
4.3e-12 deg/day shipped, 2.99e-6 compat; Moshier in 2000 1.5e-12
shipped (1.56e-8 with the first fix), 4.2e-5 compat.

## 72. The interpolated perigee's light-time estimate takes one pass, not two

**Severity: Correctness (this fork's, from entry 44's fix: the perigee 0.0049 mas off its light-time position, its speed 1.1e-7 deg/day, 1.8e-7 with SEFLG_SPEED3)**
**Where:** `sweph.c`: `intp_apsides()`

Entry 44's fix takes tau from the first passes of the apsis's iteration
at t, two for the perigee and one for the apogee, chosen as `ipl ==
SE_INTP_PERG ? 2 : 1`. But `ipl` there is the SEI_ index (5), never
SE_INTP_PERG (22), so both took one pass: the perigee's distance was up
to 92 km from the converged iteration's (rms 20 km, 20000 dates
-10000..+12000; 86 km over -3000..+3000), tau up to 0.31 ms off. Found
while measuring entry 44's cost under callgrind (four lunar evaluations
for the estimate, not seven). Not in the compat build, which has no
estimate.

**Fix, as done here:** the comparison is with SEI_INTP_PERG: two passes,
4.3 km (rms 1.1 km; three would be 0.13 km). Over 3000 dates
-3000..+3000 and nine flag sets the perigee moves by up to 0.0049 mas
(0.0053 in right ascension), its speeds by up to 1.1e-7 deg/day (1.8e-7
SEFLG_SPEED3), toward the converged light time: against the TRUEPOS
position at t - tau, tau the TRUEPOS distance over c, rms 9.8e-4 ->
2.1e-4 mas over 5000 dates (the apogee's, with its 1 km estimate, 7.4e-5:
the positions' own noise between neighbouring JDs), G46's (b) 3.0e-3 ->
1.8e-3 mas. The apogee is unchanged to the bit; positions are still
identical with and without SEFLG_SPEED and as smooth (G46 (a), (c)
unchanged: 0, 2.2e-3 mas). Cost: three lunar evaluations, perigee
1.10e6 -> 1.22e6 instructions per call without the flag (+11%), 2.95e6
-> 3.08e6 with it (entry 44's table).

G55 `tests/intpconv.c` (c): the perigee at the five dates of those 3000
where the one-pass estimate moved it most, NONUT, against the TRUEPOS
position at t - tau, within 1e-3 mas (3.6e-8: the estimate is tau to
within the JD's resolution there; one pass 3.5e-3..4.9e-3, upstream 110
mas); the upstream build fails 68 of 73. G24 `intp_lighttime_off`: the
same at JD 2486223.6015625, 4.5e-9 mas shipped, 91 compat (4.4e-3 with
one pass). Baselines: interpolated-perigee positions move by up to
0.005 mas, their speeds by 1.8e-7 deg/day.

## 74. The light deflection's rate is a one-sided step, off by the deflection's curvature behind the Sun's disc

**Severity: Correctness (up to 1.9e-6 deg/day on a body's speed behind the Sun's disc: Zeus and Isis-Transpluto 0.005 deg from its centre; 4e-7 at 0.03 deg, 3e-8 at 0.1, nothing outside the disc)**
**Where:** `sweph.c`: `deflect_light_from()` (`swi_deflect_light()`, the fixed stars, `swe_calc_pctr()`)

The deflection's speed is its change over the 5e-7 day before t, one
step back, so it is off by half that step times the deflection's second
derivative. Outside the disc that is nothing (3e-9 deg/day at the limb).
Behind it the deflection, 1.75" m(r)/r at r solar radii from the centre
(m from `meff()`'s table), turns about the centre at v/b, b the separation
and v the body's speed relative to the Sun, so it curves at ~D v^2 / b^2:
Isis-Transpluto (2025-09-21, relative 1.0 deg/day) 1.8e-6 deg/day at 0.005
deg, 1.2e-6 at 0.01, 5.7e-7 at 0.02, 2.5e-8 at 0.1; Zeus (1960) 1.9e-6;
Venus (2016-06-06, relative 0.25 deg/day) 3.8e-8 at 0.01 deg. The bug 81
agent's numbers are this defect: Zeus at year 0 (0.66 R) 5.2e-9 / 7.9e-9
deg/day in longitude / latitude, Zeus in 1960 (0.17 R) 9.4e-8, Isis in
year 170 1.1e-8; after, <= 3e-10 at each (five points over 2^-12..2^-14
day). A star behind the disc
the same: delta Gem (0.18 deg from the centre, 2025) 1.0e-8, after
3e-10.

The errors first reported for this item, 3.8e-4 deg/day for Isis at
0.065 deg, 6.7e-3 at 0.005, 1.4e-4 for Vulcanus at 0.1, 9e-7 for Admetos
and 4e-7 for Uranus near the limb, are not the library's: five-point
references over 2^-12 day whose stencils straddle a node of `meff()`'s
table or the limb. `meff()` interpolates linearly between nodes 0.01
solar radii apart, so the deflected position's slope breaks at each
(4e-6 deg/day for Isis at r = 0.9), and no central difference across a
break is a rate; with the stencil inside one cell, or at 2^-15 day, they
are the 1e-9..2e-6 above. The positions themselves are the model's and
unchanged.

**Fix, as done here:** the rate is the displacement's derivative,
d/dt [ru g1/g2 (uq e - ue q)], through the rates of u, e and q (the
ones the step extrapolated along: the body's geocentric velocity, the
observer's and the Sun's) and, inside the disc, `meff()`'s slope on the
table cell r is in (new `meff_slope()`), with r = |u x e| re / R_sun and
its rate. The speed is the rate of the returned position on r's side of
a node, everywhere. Positions are bit-identical (seven bodies, 49
instants near their conjunctions, with and without SEFLG_SPEED). G44's spreads fall (Venus
5.6e-10 -> 3.2e-10). `bench`: calc-swieph 0.5537 s against 0.5560 before,
calc-moseph 1.0303 against 1.0432 (minima of eight interleaved runs): the
derivative costs no more than the second evaluation it replaces.

G73 `tests/defldisc.c`: the conjunctions with the Sun of Isis-Transpluto,
Vulcanus, Admetos, Zeus, Uranus, Venus and Mercury; the instants before
and after at which each is 0.005..1 deg from the centre and 0.998 / 1.002
of the radius, on the 2^-15-day grid, moved off any table node or the
limb inside the stencil; the deflection's part of the longitude and
latitude speeds (less the SEFLG_NOGDEFL speed) against a five-point
difference of the deflection over 2^-14 day within 5e-9 deg/day, measured
<= 1.1e-9. The build before the fix fails 76 of the 248 speeds (1.9e-6),
the upstream build 164 (1.4e-5, entry 39 as well). G24
`defl_disc_rate_off`: Isis 0.005 deg from the centre, 5.2e-11 deg/day
shipped, 5.95e-6 compat. G44, G49, G51, G76 pass.

## 77. `swe_calc_pctr()` reads its bodies at the rounded instant, its aberration rate is the centre's whole velocity change over the light time, it deflects as seen from the Earth, and its light time stops short of its fixed point

**Severity: Correctness (positions up to 1.25" off, the deflection seen from the Earth; distance rate 1.5e-6 AU/day off on Mercury seen from Mars, 1.3e-4 on Saturn from Mercury; longitude speed 2.1e-6 deg/day off on Saturn from Mercury; position noise ~1e-9 deg, 1.8e-7 deg/day in a five-point difference over 2^-9 day of the Moon's places from Venus)**
**Where:** `sweph.c`: `swe_calc_pctr_r()`

Two residues that entries 52 and 63 fixed in `app_pos_etc_plan()` but not
in the planet-centric path, which has its own copy of the light-time and
aberration code, and two defects of its own, the deflection and the frame
bias's date (below). Scanned on ts20, Swiss files, Mercury from Mars, the
Moon from Venus, Jupiter from Saturn, the Sun from Jupiter, Venus from the
Moon, Mars from the Earth and Saturn from Mercury, thirteen dates
-3000..+3000, steps 2^-6..2^-9 day, binary-fraction instants, against
five-point differences of the positions. The first is entry 63's: both
bodies were read at t = tjd - dt rounded to a double, up to half an ulp of
the date (2.3e-10 day) from the emission instant, and the place jumped by
the body's motion over that from one date to the next. It scales as 1/h:
without aberration, Mercury from Mars 9.8e-8 deg/day, the Moon from Venus
1.6e-7, Venus from the Moon 1.1e-7, Mars from the Earth 5.3e-8, Jupiter
from Saturn 7.6e-9, against 4e-10 for true positions; distance rates <=
2.2e-9 AU/day. The second is steady with the step: the aberration rate
added the centre's velocity at tjd less that at t, upstream's form of
entry 34's term, and entry 52's exact one never reached this path. Its
radial part is the distance's own (second order), and the difference is
tau a(tjd - tau/2) where the rate wants tau a(tjd): the distance rate off
by 1.55e-6 AU/day on Mercury from Mars, 5.5e-6 on the Moon from Venus,
4.2e-6 on Venus from the Moon and Mars from the Earth, 2.9e-7 on Jupiter
from Saturn, 3.5e-7 on the Sun from Jupiter, 1.3e-4 on Saturn from
Mercury, whose longitude speed was off by 2.1e-6 deg/day (2.0e-6 of it the
half light time's shift of a; 5.6e-8 on Venus from the Moon). Bug 35's
light-time rate (`lt_rate_exact()`) was already applied here, with the
body at t and the centre at tjd, barycentric; the aberration used the
centre's velocity at tjd; the frames (bias, precession, nutation, the
ecliptic of date's turning) are `app_pos_rest()`'s and were right.

The third: the deflection was `swi_deflect_light()`'s, which takes the
observer and the Sun from the save area, there the Earth and the Sun at t
that the reads at t left, not the centre: the Earth's geometry applied to
the centre's vector. Against the deflection written out from the centre's
geometry (the Explanatory Supplement's 3.326, in G76), Saturn seen from
Mercury was 0.435" off over 400 dates from 2000, where the deflection
itself is 0.434"; the Sun seen from Jupiter was deflected by 0.33", which
nothing should deflect by itself; the planets seen from the Earth were
0.092" from `swe_calc()`'s geocentric places (Uranus; the Sun 4.7e-5",
deflected by itself; speeds 7.4e-5 deg/day). The fourth: under
SEFLG_TRUEPOS t stayed 0, the date `swi_bias()` is given, which under
SEFLG_JPLHOR_APPROX and SEMOD_JPLHORA_3 skips the bias before 1962: on
DE431, Mars from the Earth with SEFLG_TRUEPOS | SEFLG_JPLHOR_APPROX was
0.042" from `swe_calc()`'s, 1968-2020. The fifth (found after entry 78,
the same in `app_pos_etc_plan()`): the light time was solved along a
straight line, xx0 - dt v, and stopped short of its fixed point tau =
|X(tjd - tau) - O(tjd)| / c by that line's departure from the orbit:
against the fixed point iterated on true positions (each read corrected
for its instant's rounding), 5e-9 deg on Mercury from Pluto, 3.3e-10 from
Mars, 1.1e-10 on Venus from the Moon, thirteen dates -3000..+3000.

**Fix, as done here:** after the read at t, the body is moved along its
velocity at tjd by the rounding, (tjd - t) - dt, as entry 63's (the path
refuses Moshier, which has no barycentric positions). The aberration rate
is entry 52's `add_aberr_rate()`, with the centre's velocity change tau
a(tjd): dt times the centre's acceleration at tjd, the derivative of the
parabola through its velocities at t and tjd, read already, and at tjd +
2^-10 day, one read more, off by d h a'''/6. The two velocities alone give
a(tjd - tau/2) (Saturn from Mercury 2.0e-6 deg/day off), and with the
Sun's jerk added still 5.7e-9 there and 5.1e-8 on Venus from the Moon,
whose acceleration is the Earth's pull more than the Sun's; a central
difference over tau either side, also one read, 5.7e-9: so one read stays,
against two before (`swe_calc_pctr()` with SEFLG_SPEED and aberration 18.4
-> 22.9 us with two, 20.7 us with one, +12% over the call before entry
77). The deflection is seen from the centre: the core of
`deflect_light_site()` is split out as `deflect_light_from()`, taking the
observer (the centre at tjd) and the barycentric Sun at tjd, read once
more (a `swe_calc_r()` the save area answers, at no measurable cost); none for the Sun, nor
seen from it. `deflect_light_site()` calls it with the Earth (and site)
and the save area's Sun, as before: `swe_calc()` and the star functions
are bit-identical (3600 rows, every planet, Spica, six flag sets), bench
calc-swieph/calc-moseph unchanged (min of 3: 0.557/1.049 s against
0.554/1.036, within the runs' spread). The deflection reads no saved
observer now, so the fork's topocentric observer computed and saved for it
at the top (2026-09-16 review, F7) is gone. Under SEFLG_TRUEPOS t is tjd.
After the read, one Newton step on the light time, d tau = (|rho| / c -
dt) / (1 + rho^.V / c), the body moved along its velocity by -d tau (entry
78's approach): 6e-14 deg off the fixed point after.

After, same scan: longitude and latitude speeds <= 5.9e-10 deg/day (true
positions 4.0e-10), distance rates <= 3.4e-11 AU/day. Saturn from Mercury
within 6.4e-7" of the reference deflection; the Sun undeflected; the
planets from the Earth equal to `swe_calc()`'s to 2.8e-6" (speeds 1.1e-9
deg/day, 300 dates 1863-2137, four flag sets), which is `swe_calc()`'s own
distance from the light time's fixed point until entry 78 (4e-10" without
the Newton step here); TRUEPOS | JPLHOR_APPROX equal. Positions move by up
to 1.25" against the build before (the deflection: Venus from Mercury
1.25", Neptune from Mercury 0.87", the Sun from Neptune 0.75"; 120 dates
1900-2100, every pair of Sun..Pluto, the Moon and the Earth), which needs
a re-baseline; the light time's step moves them by up to 5e-9 deg; true
positions not at all except under JPLHOR_APPROX after 1962 (0.042", the
bias); the same with and without SEFLG_SPEED.

G76 `tests/pctrspeed.c`: Mercury from Mars, the Moon from Venus, Jupiter
from Saturn, the Sun from Jupiter, Venus from the Moon, Saturn from
Mercury; default flags, SEFLG_NOABERR, SEFLG_TRUEPOS, SEFLG_J2000,
SEFLG_EQUATORIAL; thirteen dates, four steps; longitude and latitude
speeds within 5e-9 deg/day, distance rates within 5e-10 AU/day (measured
<= 5.9e-10 and <= 3.4e-11). Then the light time: six pairs (Mercury from
Pluto among them), thirteen dates, within 1e-11 deg of the fixed point
iterated in the test (6.0e-14). Then the deflection, 400 dates from 2000:
the Sun from Jupiter not deflected at all, Saturn from Mercury within
1e-5" of the deflection written out in the test from Mercury's and the
Sun's places (6.4e-7"); and Sun..Pluto from the Earth at thirteen dates
within 1e-5" and 5e-9 deg/day of `swe_calc()` (8.7e-7" and 4.2e-10). The
build before the rounding and aberration fixes fails 22 of the 30 speed
rows, by up to 2.1e-6 deg/day and 1.3e-4 AU/day, and the other three; the
build before this part (entry 77's first commit) fails the light time
(5.0e-9 deg) and the two deflection checks (0.33", 0.435" off; 5.2e-3" and
1.2e-6 deg/day from `swe_calc()`); the upstream build fails all 33, by up
to 2.3e-6 deg/day in longitude, 1.9e-5 in declination and 1.3e-4 AU/day.
G24 `pctr_dist_rate_off`: Mercury from Mars, distance rate over 2^-8 day
at nine dates, 8.3e-12 AU/day shipped (1.52e-6 before the fix), 1.50e-6
compat; `pctr_sun_defl_off`: the Sun from Jupiter's deflection at nine
dates, 0 shipped (0.030" before the deflection fix), 0.030" compat.
SEFLG_JPLHOR_APPROX needs a DE403-or-later JPL file, which the gates do
not have; measured by hand on DE431 as above.

## 75. The true node's and osculating apogee's speeds divide by the step asked for, not the step taken

**Severity: Correctness (1.7e-6 to 2.3e-6 of the speed: 2.3e-7 deg/day on the true node's longitude, 6e-6 on the osculating apogee's, 2.8e-7 in the node's declination (G41); 3.5e-7 in both bodies' J2000 latitude and declination speeds)**
**Where:** `sweph.c`: `lunar_osc_elem()`

G41 found the true node's declination speed 2.8e-7 deg/day off, 30 times
its neighbours (the mean node's 4e-9). Measured against five-point
differences of the library's own positions at binary instants, eight to
twelve dates -2500..2800 (de200 1650..2100), steps 2^-4..2^-10 day: on
the Swiss files the node's longitude speed was off by 2.2e-7 to 2.5e-7 at
every step, the apogee's by 5.8e-6 to 6.1e-6, the distances by 1.6e-11
and 1.8e-11 AU/day; de200 4.2e-7, 6e-6, 3.3e-11. Constant over the steps,
so not the reference's noise (which shows only below 2^-7 day, 1e-8 on
the node). Two causes:

1. The speeds are `(x(tjd + h) - x(tjd - h)) / 2h`, h = 0.0001 day, from
   the Moon at those epochs less the light-time. Neither `tjd +- h` nor
   `t - lt` is representable: they round to the date's grid, 2^-31 day
   from JD 2097152 to 4194304, and the interval between the Moon's epochs
   is off by up to 2.3e-6 of itself: 1.7e-6, steadily, at a binary date
   (1e-4 is 214748.36 grid steps) and at random with the light-time.
   With SEFLG_TRUEPOS and the interval fixed alone, the node's error fell
   to 1.6e-9 and the apogee's to 1e-7; with the light-time, still 5.5e-8
   and 6e-6. Moshier's 2^-8 day step is exact and has no light-time here.
2. Bug 33 gave the node's and apogee's equatorial speed of date the
   ecliptic's turning, but not with SEFLG_J2000, though both are found in
   the ecliptic of date and precessed to J2000 afterwards (only the
   `SID_TNODE_FROM_ECL_T0` build finds them in J2000's): their J2000
   latitude and declination speeds left out the mean obliquity's rate,
   3.5e-7 deg/day, on every ephemeris. The sidereal ECL_T0 and SSY_PLANE
   modes, which convert the same equatorial speed of date, were right.

Sidereal (traditional), NONUT and TRUEPOS were otherwise like the default.

**Fix, as done here:** each Moon epoch used and its light-time are kept;
the difference is divided by the interval between the two epochs as
rounded, times dts / (dts - (lt1 - lt0)), dts = (tjd + h) - (tjd - h),
the light-time-corrected epoch's rate from the unrounded light-times
(Moshier: exactly 2^-7 day). The frame's turning is added for J2000 too
(not under `SID_TNODE_FROM_ECL_T0`). Positions unchanged. Measured after,
same grid, steps 2^-5..2^-7: the Swiss node's longitude, latitude and
declination speeds 8.5e-10..2.5e-9, de200 6e-10..4.3e-9, distances
<= 5e-13 AU/day; J2000 latitude 3.5e-7 -> 1.2e-11 (de200 8e-13, Moshier
3e-10); the apogee's longitude 2.5e-7 without nutation, 6e-7 with (de200
1e-7..4e-7), latitude 2e-8..3e-7. What is left on the apogee is the
Moon's velocity noise over the 0.0001-day step, sporadic by date (a
smaller step is worse: 1.6e-6 at 2^-15 day; 1.2e-7 at 2^-11, not changed
here). A Swiss Moon segment boundary inside a reference stencil (JD
1790500.80) shows 4e-4 on the node either way: the file's jump, not the
speed. Moshier is unchanged apart from J2000: node 5.5e-8, apogee 2.7e-6,
bug 51's three-point slope over 2^-8 day. G41's true node declination
2.80e-7 -> 1.65e-9, its limit 1e-6 -> 2e-8 (the build before the fix
fails it). G53 and G40 unchanged.

G74 `tests/oscspeed.c`: the true node and osculating apogee on the Swiss
files and de200, the node on Moshier, ecliptic, equatorial, equatorial
NONUT, J2000 ecliptic and equatorial, eight epochs, longitude, latitude
and distance speeds against a five-point difference over 2^-6 day: 25
rows, the node within 2e-8 deg/day (measured <= 1.5e-9), the apogee 5e-7
(<= 9.6e-8), the Moshier node 1.5e-7 (5.2e-8), distances 5e-12 AU/day
(1.9e-13; Moshier 2e-11, 6.7e-12). Before the fix 22 of 25 fail (node
longitude 9.4e-8 Swiss, 4.5e-7 de200, J2000 latitude 3.5e-7, apogee
1.1-1.3e-5, distances 2.5-3.3e-11); the upstream build fails all 25. G24
`osc_speed_off`: the apogee's longitude speed at JD 2460000.297, 9.97e-9
deg/day shipped, 4.01e-6 compat; the node's J2000 latitude speed 1.0e-13
shipped, 2.32e-7 compat.

**Then the step, as done here (second part):** with the interval right,
what was left was the step. Against a nine-point difference over 2^-3 day
(eighth order; checked against the same over 2^-4, the dates where the
two disagree, a Swiss Moon segment boundary within the stencil, left
out: 5-9 of 150 on the Swiss files and de200), 150 binary dates,
ecliptic and equatorial, worst of longitude and latitude:

| speed of | before (3 points, 1e-4 day; Moshier 2^-8) | 3 points 2^-11 | 5 points 2^-10 | 2^-8 | 2^-7 | 2^-6 | 2^-5 | 2^-4 |
|---|---|---|---|---|---|---|---|---|
| Swiss node | 1.3e-9 | 1.3e-9 | 3.1e-10 | 3.1e-10 | 3.0e-10 | 3.2e-10 | 4.3e-10 | 3.6e-9 |
| Swiss apogee | 5.2e-7 | 9.1e-8 | 1.1e-7 | 4.2e-8 | 2.2e-8 | 2.1e-8 | 1.7e-8 | 2.7e-7 |
| de200 node | 1.2e-9 | 1.4e-9 | 4.8e-10 | 5.8e-10 | 5.9e-10 | 5.9e-10 | 5.1e-10 | 3.3e-9 |
| de200 apogee | 1.5e-6 | 5.5e-8 | 3.3e-8 | 3.8e-8 | 3.8e-8 | 3.8e-8 | 2.9e-8 | 3.2e-7 |
| Moshier node | 6.8e-8 | 1.9e-7 | 1.4e-7 | 4.0e-8 | 2.1e-8 | 1.1e-8 | 6.6e-9 | 1.8e-8 |
| Moshier apogee | 3.2e-6 | 7.6e-6 | 7.3e-6 | 2.7e-6 | 1.6e-6 | 9.0e-7 | 4.7e-7 | 3.0e-7 |

(the columns after the first from 60 dates for 2^-11 and 2^-10, 150 for
the rest). Noise falls as the step grows until truncation takes over: on
the Swiss files and de200 at 2^-5 in the distance speeds (6.4e-13 AU/day
against 4.5e-14 at 2^-6) and at 2^-4 in all; on Moshier, whose Moon is
noisier, at 2^-4 in the node. So five points over 2^-6 day on the Swiss
files and JPL, over 2^-5 on Moshier (`OSC_NPT_*`, `OSC_INTV_*`); each
epoch's rounding, `osc_round_err()`'s exact TwoSum of tjd + k step and of
t - lt, is taken back to first order as a factor 1 + D[rr] on the
difference, which replaces the first part's interval. Bug 31's slope of
the node, overwritten before it was returned, is gone from the default
build. Positions unchanged (bit for bit, with and without SEFLG_SPEED,
three ephemerides, five flag sets, 40 dates); the compat build bit for
bit as before. Cost: the true node with speed takes 5 Moon positions
instead of 3, 22.6 against 16.8 us a call on the Swiss files, 113 against
72 on Moshier (not in `calc-swieph`/`calc-moseph`). Near the end of a
JPL or Swiss file the epochs reach 2 steps (0.031 day) past tjd, not
0.0001, before the fall-back to another ephemeris.

G74 now takes the nine-point difference over 2^-3 day as the reference
(a five-point over 2^-6 would be the library's own stencil) and adds the
Moshier apogee: 30 rows, the node within 1e-9 deg/day (measured <=
1.0e-10), the apogee 5e-8 (8.6e-9), Moshier 5e-9 and 4e-7 (6.5e-10,
7.2e-8), distances 4e-13 AU/day (4.6e-14; Moshier 2e-12, 3.1e-13). The
first part's build fails 20 of 30 (Swiss node 1.6e-9, apogee 1.4e-7,
Moshier node 5.1e-8, apogee 2.0e-6, Moshier distances 6.7e-12 and
2.9e-11); the build before entry 75 and the upstream build fail all 30.
G24 `osc_speed_off` against the same nine-point reference: 8.48e-9
deg/day shipped, 4.00e-6 compat (shipped limit 3e-7 -> 5e-8); the J2000
latitude 3.2e-14 and 2.32e-7. G41's true node 1.71e-9 (limit 2e-8).

## 71. Every other cache emptied by an instant of 0 answers for JD 0, and sweph()'s forced EMB is the Earth there

**Severity: Correctness (returned values at exactly tjd = 0: 0.025 deg on Mars's osculating node, 15 deg on a barycentric Sun in a warm context, 0.3 mag in a limiting magnitude; NaN)**
**Where:** `sweph.c`: `sweph()`, `swe_calc_r()`, `jplplan()`, `free_planets()`, `ctx_init_defaults()`, `swi_init_swed_if_start()`, `swi_close_keep_topo_etc()`, `ctx_release()`, `lunar_osc_elem()`, `intp_apsides()`, the five `app_pos_etc_*()` observer checks, `swi_check_ecliptic()`, `swe_set_topo_r()`, `swi_force_app_pos_etc()`; `sweconfig.c`: `swi_invalidate_deltat()`, `swi_invalidate_models()`, `swi_config_apply()`; `sweph.h`: `SWI_TJD_NEVER`

Entry 67's idiom, everywhere else. The planets' slots (`pldat[].teval`),
`swe_calc()`'s save area (`savedat[].tsave`), the nodes' (`nddat[].teval`),
the obliquity (`oec.teps`, `oec2000.teps`), the observer (`topd.teval`) and
`SunRA()`'s memo in `swehel.c` (`hel.sunra.tjdlast`) were emptied, or a
recomputation forced, by setting the instant to 0, which is also JD 0
(-4712). Some readers guarded it (`tjd == 0` in `swi_check_ecliptic()`,
`jplplan()` and the observer checks, `tjd != 0` in `swe_calc_r()` and the
node caches), so their caches never answered at tjd 0; others did not:

- `sweph()` forces the EMB to be recomputed for the barycentric Sun by
  setting the Earth slot's teval to 0 around the call (the slot is the EMB's
  too). At tjd 0 the slot then claims to hold JD 0, and the Earth it holds,
  of its own instant, comes back as the EMB. `swe_nod_aps()` puts a
  light-time step (a `swe_calc()` at tjd - dt) between its points and
  recomputes the Sun after it, which is the state that reaches this.
  Measured after `swe_close()`, against tjd 1e-9: Mars's heliocentric
  osculating ascending node 356.99952 against 357.02455 (0.025 deg; the
  speed 1.57e-4 against 3.92e-5 deg/day), its geocentric osculating
  perihelion 0.22 deg, its geocentric mean perihelion 5.5e-3 deg, the
  Moon's and every planet's points by every method; the osculating points
  without `SEFLG_SPEED` NaN after a call at J2000. In a context warm from
  other instants, `swe_calc()` itself at tjd 0: the barycentric Sun 157.81
  against 172.98 (15.2 deg), the Sun with `SEFLG_TRUEPOS` 4.95 deg (the
  Earth slot held the Earth five days on), Mercury 2.6e-4 deg.
- `lunar_osc_elem()` forced the Moon's slot the same way ("there may be a
  moon of wrong ephemeris"), leaving its last Moon claiming JD 0.
- `SunRA()`'s memo, empty with tjdlast 0, answered RA 0 for UT 0:
  `swe_vis_limit_mag()` at tjd_ut 0 on a fresh context, Venus -6.4973 mag
  against -6.2006, the Moon -13.93 against -12.46.
- Latent: `swi_plan_for_osc_elem()`, `swi_obliquity_rate()` and the
  Moshier Moon read `oec.teps == tjd` unguarded, and an emptied obliquity
  cache has eps 0; every public path fills it at the call's tjd first.

The default context's first use, `swi_init_swed_if_start()`, clears it
with a memset and set none of the sentinels, entry 67's included (only
`ctx_init_defaults()`, for `swe_ctx_new()`, did). Not measured to reach a
result: the first call's ephemeris-path setup empties the caches again
through `swi_close_keep_topo_etc()` (a first `swe_get_ayanamsa_ex()` or
`swe_calc()` at tjd 0 was right), but `SunRA()`'s memo is not among them.

**Fix, as done here:** one sentinel, `SWI_TJD_NEVER` (-1e300, which no
instant equals; `SWI_NUT_NEVER` is now its alias). `free_planets()` sets it
in every planet, save-area and node slot; the two close paths in the
obliquity and all three nutation caches; context creation, through both
`ctx_init_defaults()` and the default context's memset, in all of them, the
observer's and `SunRA()`'s memo; and every forcing site (`sweph()`'s EMB,
`lunar_osc_elem()`'s Moon, `swi_force_app_pos_etc()`, `swe_set_topo_r()`,
the invalidations in `sweconfig.c`) uses it. The `tjd == 0` and `tjd != 0`
guards are compat-only: tjd 0 is an instant like any other, and a cache
filled there answers there. At every other instant nothing changes: G70's
requests at J2000, 1e-9 and two more instants, in its four states, are
bit-identical before and after, as is the bench checksum; calc-swieph and
calc-moseph unchanged within noise.

G70 `tests/tjdzero.c`: the same requests at tjd 0 and 1e-9 -- `swe_calc()`
for nine bodies and ten flag sets, `swe_nod_aps()` for seven bodies by the
three methods with and without speed and heliocentric, the ayanamsa for
four modes with and without nutation, `swe_vis_limit_mag()` for Venus and
the Moon -- in a fresh context, after `swe_close()`, warm, and after the
invalidating setters; 7676 rows, positions within the motion over 1e-9 day
plus 2e-8 deg (largest residual 7.6e-11), speeds within 2e-7 deg/day
(largest 4.4e-8), magnitudes within 1e-5 (6.4e-7), flags equal, no errors.
The upstream build fails 3342 rows; ts20 before this fix failed 4176. G24
`nodaps_jd0_off`: Mars's heliocentric osculating node at 0 against 1e-9,
0 deg shipped, 2.5e-2 compat. G24, G42, G47, G57 and G66 pass.

## 78. The planets' light time is solved along a straight line, and the Earth's and the Sun's light times have no rate

**Severity: Correctness (7.8e-10 deg on Mercury's position; 1.7e-6 deg/day on the heliocentric and barycentric Earth's longitude speed)**
**Where:** `sweph.c`: `app_pos_etc_plan()`, `app_pos_etc_sun()`, `app_pos_etc_sbar()`

Two residues found by entry 63's scan. The light time of a planet is the
fixed point of tau = |X(t - tau) - O(t)| / c. `app_pos_etc_plan()` solves
it along the straight line X(t) - tau v in two passes and reads the body
at the last estimate's t - tau, but never measures the distance of the
body it read: that body is off the line by its curvature over the light
time, a tau^2 / 2, and the tau it was read at is short by that over c.
Against the fixed point iterated from the library's own true positions (X
from `SEFLG_BARYCTR | SEFLG_TRUEPOS` at t - tau, the rounding of t - tau
carried along the velocity, O the body less its true place from the
observer; Moshier's from `SEFLG_HELCTR`, its frame), astrometric, 601
dates -3000..+3000 (de200 1601..2168): Mercury 7.8e-10 deg geocentric
(topocentric the same), 1.8e-10 heliocentric, Venus 1.1e-10 and 4.6e-11,
Mars 3.1e-11 and 1.8e-11, Jupiter 3e-12, Saturn 1.2e-12, on the Swiss
files, de200 and Moshier alike (Moshier reads the body at t - tau since
entry 53).

`app_pos_etc_sun()` reads the heliocentric or barycentric Earth (and the
geocentric Sun) at t - tau converged, but has no light-time rate, the
term entry 35 gave the planets: the Earth's longitude speed is 1.7e-6
deg/day off a five-point difference of its positions at every step from
2^-6 to 2^-9 day (Swiss files 1.8e-6 heliocentric and 1.6e-6
barycentric, de200 1.7e-6 and 1.8e-6, Moshier 1.8e-6), v_E tau', tau' =
r'/c up to 1.7e-6. The geocentric Sun without aberration is 6.8e-10
(9.1e-10 on de200), v_S tau'. The Earth was also read at t rounded to a
double (entry 63), up to 2.3e-10 deg from one date to the next.
`app_pos_etc_sbar()`, the barycentric Sun, had no rate either: 5.8e-9
deg/day, its true positions 5.8e-10.

**Fix, as done here:** after the read (and entry 63's rounding), one Newton
step on the light-time equation from the body read, f = tau - r/c, f' =
1 + rho^.v/c, the body moved along its velocity by the change; the
correction is ~1e-9 day and the step leaves nothing of it. In
`app_pos_etc_sun()` the read body takes v dtau/dt from `lt_rate_exact()`
against the centre at teval (the Sun, the barycentre, or for the
geocentric Sun the observer), and is moved along its velocity at teval by
the rounding (teval - t) - dt. In `app_pos_etc_sbar()` the same rate with
the observer at the barycentre.

After, same measures: every planet within 7.6e-13 deg of the fixed point
on all three ephemerides (Mercury; Venus 2.6e-13). The Earth's speed
within 3.6e-10 deg/day of its positions on de200 and Moshier and
barycentric on the files, 6.8e-10 heliocentric on the files (that path
subtracts the Sun read at t - tau); the geocentric Sun's without
aberration 3.4e-10 and 3.5e-10, its true positions' floor; the
barycentric Sun's 3.1e-9, the rest its velocity taken at teval. Positions
move by up to 7.2e-10 deg (Mercury geocentric), 1.1e-10 Venus, 3.1e-11
Mars, 1.8e-10 Mercury heliocentric, the heliocentric Earth 2.3e-10, on
all three ephemerides; speeds by <= 1.6e-10 deg/day for the planets,
1.7e-6 for the heliocentric and barycentric Earth, 8.7e-10 (1.4e-9
topocentric) for the geocentric Sun. Positions with and without
SEFLG_SPEED still agree exactly. Cost: one sqrt and ~15 flops a call,
instructions +0.06% on bench calc-swieph and +0.04% on calc-moseph
(perf stat, measured before the rebase onto entry 53); wall-clock within
the machine's noise.

G77 `tests/ltconv.c`: the heliocentric and barycentric Earth (Swiss files,
de200, Moshier heliocentric) and the geocentric Sun without aberration,
thirteen dates, steps 2^-6..2^-9 day, longitude and latitude speeds
within 2e-9 deg/day (the Sun's within 5e-10) of five-point differences;
Mercury, Venus and Mars geocentric, topocentric and heliocentric on all
three ephemerides against the fixed point iterated in the gate, 61
dates, within 1e-11 deg: 34 rows, measured <= 6.8e-10 deg/day and
<= 5.2e-13 deg. The build before this fix fails all 34 (Mars heliocentric
by the least, 1.8e-11 deg); the upstream build fails all 34. G24
`earth_helio_lt_rate_off`, the heliocentric Earth's longitude speed over
2^-8 day at nine dates: 6.0e-10 shipped (1.7e-6 before the fix), 1.7e-6
compat.

**Part 2, the Swiss files' heliocentric Earth.** `app_pos_etc_sun()`
read the heliocentric or barycentric Earth on the Swiss files through
`sweplan()`, which handed back the Sun read at the same t - tau, and that
Sun was subtracted: E(t - tau) - S(t - tau), where de200 and every
planet's heliocentric light time (entry 26) take E(t - tau) - S(t), the
Sun where the light arrives. 3.0e-6 deg between the two (v_Sun / c),
121 dates -3000..+3000; the speed, whose rate was taken against that
Sun, 6.8e-10 deg/day off its positions. Fixed: the Sun is not asked back
from that read, so xsun stays the Sun at teval, in the light time's
distance and in the subtraction. After: 3.4e-14 deg from the fixed point
(de200 3.3e-14), the speed 3.4e-10 deg/day, its true positions' floor.
Swiss heliocentric Earth positions move by 3.0e-6 deg. G77 gains the
heliocentric Earth's position against the fixed point on the Swiss files
and de200, within 1e-11 deg: 3.4e-14 and 3.8e-14 measured, 3.0e-6 on the
Swiss files before (de200 2.3e-10 before part 1); the upstream build
3.0e-6 and 2.3e-10.

**Part 3, the barycentric Sun's velocity.** `app_pos_etc_sbar()` moves
the barycentric Sun back along its velocity by the light time from the
barycentre, and with part 1 takes the light time's rate, but the
velocity it reported was the one at teval, not at t - tau: off by tau a,
the Sun's acceleration by the planets (1e-8 AU/day^2 over 5e-5 day, at
0.005-0.01 AU). In J2000 equatorial coordinates, thirteen dates, steps
2^-6..2^-9: 3.1e-9 deg/day on the Swiss files, 2.1e-9 on de200 with the
rate (6.0e-9 and 3.7e-9 upstream). Fixed: with SEFLG_SPEED the Sun is
read at t - tau for its velocity; the place stays the line's, off by
a tau^2 / 2 = 1e-17 AU, so positions are the same with or without
SEFLG_SPEED and do not move. After: 1.2e-9 and 6.3e-11 deg/day, each the
true positions' own floor. One extra read on SE_SUN | SEFLG_BARYCTR |
SEFLG_SPEED calls only. G77 gains both rows, within 2e-9 (Swiss files)
and 5e-10 (de200): 38 rows.

Moshier solved the light time in its heliocentric frame, which leaves out
the Sun's motion over it, v_Sun tau. In an apparent direction that cancels
against the aberration taken with the heliocentric Earth's velocity (the
direction depends on the relative velocity only): within 6.6e-10 deg of
the barycentric answer. The remainder, 3.0e-6 deg (v_Sun / c) on every
SEFLG_NOABERR and heliocentric Moshier position, was left here for want of
the Sun's barycentric velocity; entry 102 forms it from Moshier's own
planets and removes it (G101). The heliocentric Moshier speeds 2e-9 to
7e-9 deg/day off their positions, seen before entry 53, are gone with
this entry's fix: <= 2.2e-10 now, Mercury to Vesta.

## 81. A fictitious body's light-time position is read at the rounded instant, and its elements' large terms are rounded unreduced

**Severity: Correctness (3.9e-8 deg on Waldemath's apparent position, a jitter that a five-point difference over 2^-7 day turns into 1e-6 to 5.5e-6 deg/day; 3.4e-7 deg/day on Vulcan heliocentric, 6.7e-8 on Selena under Moshier; part 2: 3.0e-8 deg on Vulcan's true position at -3000, 2.4e-6 deg/day in its differences)**
**Where:** `sweph.c`: `app_pos_etc_plan_osc()`; part 2 `swemplan.c`: `check_t_terms()`, `read_elements_file()`; part 3 `sweph.c`: `app_pos_etc_plan_osc()`, `app_pos_etc_orbel()`

Entry 63's defect on the fictitious bodies' path. The light time is solved
on the orbit (entry 26, G27), the elements read at t = teval - dt rounded to
a double, up to half an ulp of the date (4.7e-10 day at the present) from
the emission instant. An Earth-centred set's focus is the Earth at t, which
moves 0.0172 AU/day while the observer stays at teval: one ulp is 8e-12 AU,
7e-8 deg seen at Waldemath's 0.0065 AU. Its apparent place is a sawtooth
whose period is the time its light time takes to change by one ulp (0.0015
day at a radial speed of 5.5e-5 AU/day). Found surveying bodies 40..58 while
chasing Zeus's 3.3e-7 / 1.3e-6 deg/day at year 0 (that one is the
reference: Zeus 0.18 deg from the Sun's centre, behind the disc, where
`meff()`'s table makes the deflection piecewise linear; over 2^-9..2^-15 day
the residual is 5e-9 to 8e-9, entry 74's one-sided deflection rate).
Measured on ts20, 61 dates 1700..2300 and -300..+300, five-point
differences at the best of 2^-6, 2^-7 and 2^-8 day: Waldemath 1.9e-6 deg/day
(Swiss, JPL), 1.3e-6 (Moshier), Selena 1.3e-7 (Moshier), Vulcan heliocentric
3.4e-7, against their true positions' 1.1e-8, 2.6e-10 and 5.2e-8. Selena's
light time hardly changes (a circle about the Earth), so its rounding is
nearly the same at every instant; it shows on Moshier at year 40.

**Fix, as done here:** after the loop, as for a planet, the body is moved
along its barycentric velocity at teval (`pdp->x`, there with or without
SEFLG_SPEED) by the rounding, (teval - t) - dt, heliocentric and
Earth-centred sets alike. Positions move by <= 3.9e-8 deg (Waldemath),
4.3e-9 (Selena), 6.6e-10 (Vulcan), <= 1.3e-12 on the other sets (200
dates 1700..2300); with and without SEFLG_SPEED they stay identical. Three
multiply-adds per fictitious call; the planets never reach this path.
After: Waldemath at 1980, 5.5e-6 -> 1.1e-9 deg/day over 2^-7 day. The rest
was entry 48's floor, the elements' own rounding, which the true positions
shared (part 2 below): Vulcan heliocentric 1.4e-7 deg/day apparent against 5.2e-8 true
(fourth differences 4.0e-9 and 3.1e-9 deg), and before 1 CE more (Vulcan's
mean anomaly at -200 is 1.6e7 deg; its true positions are 3.4e-7 off their
own differences over 2^-9 day). Zeus's behind-disc residual is not this
defect and is unchanged.

G80 `tests/fictltround.c`: Vulcan heliocentric and geocentric, Selena and
Waldemath, apparent, Swiss and Moshier, thirteen dates 1700..2300: speeds
against five-point differences over 2^-7 and 2^-9 day within 5e-7
(Vulcan heliocentric; measured 1.4e-7), 1e-7 (Vulcan geocentric, Waldemath;
2.2e-8) and 5e-9 (Selena; 2.8e-9) deg/day, and fourth differences of the
positions at eight instants 2^-9 day apart within 1e-8 (Vulcan
heliocentric; 4.0e-9) and 2e-9 deg (4.7e-10). The build before the fix
fails 14 checks on all eight rows (Waldemath 1.5e-5 deg/day and 2.3e-7 deg,
Vulcan heliocentric 2.3e-6 and 3.7e-8, Selena 8.5e-9 deg/day Swiss and 6.7e-8 Moshier); the
upstream build fails all eight rows by bug 48's speeds. G24
`waldemath_lt_round_off`: Waldemath's geocentric longitude speed over 2^-8
day at nine dates, 1.65e-8 deg/day shipped (5.3e-6 before the fix), 1.2e-2
compat. G48, G59, G62, G64 pass.

**Part 2 (the elements' own rounding).** `check_t_terms()` formed a term
n T as n times T = (tjd - epoch) / 36525 and the element's sum was reduced
to 0..360 afterwards, so an angle with a large rate carried the rounding of
its unreduced value: Vulcan's mean anomaly 707550.7341 T is 1.6e7 deg at
-200 and 3.5e7 at -3000 (an ulp of 1.9e-9 and 3.7e-9 deg, with the
roundings of T, of the product and of tjd - epoch as much again), and its
node and perihelion turn at 1670 deg/cy; Waldemath's mean anomaly is
109023 T. The true positions, against which part 1 was judged, jittered.
Fourth differences at eight instants 2^-9 day apart (deg) and the speed
against five-point differences over 2^-9 day (deg/day), Moshier, five
dates at each epoch, SEFLG_TRUEPOS:

| body | -3000 | -200 | 2000 | +3000 |
|---|---|---|---|---|
| Vulcan, heliocentric, before | 3.0e-8, 2.4e-6 | 7.7e-9, 4.3e-7 | 3.6e-10, 2.7e-8 | 5.5e-9, 3.0e-7 |
| after | 1.8e-12, 4.3e-11 | 1.3e-12, 2.5e-11 | 1.8e-12, 4.4e-11 | 1.4e-12, 1.8e-11 |
| Waldemath, geocentric, before | 3.5e-9, 3.5e-7 | 3.3e-9, 2.0e-8 | 6.3e-11, 3.1e-9 | 1.1e-9, 1.0e-7 |
| after | 4.5e-12, 2.4e-10 | 3.4e-12, 1.6e-10 | 3.2e-12, 1.8e-10 | 3.9e-12, 8.8e-11 |
| Selena, geocentric, before | 8.8e-11, 8.6e-9 | 4.4e-11, 5.2e-10 | 8.0e-13, 2.9e-11 | 4.4e-11, 2.6e-9 |
| after | 4.8e-13, 3.6e-11 | 4.5e-13, 2.5e-11 | 8.0e-13, 2.9e-11 | 5.4e-13, 3.7e-11 |

The other sets (40..54, 57) were at 1e-12 and are bit-identical.

**Fix, as done here (part 2):** for an angle (mean anomaly, perihelion,
node, inclination; not the semi-axis or eccentricity) a term in T alone is
(n / 36525)(tt + ttlo): the rate as two doubles (n / 36525 and its
remainder by fma), the product by fma with its error kept, tt = tjd - epoch
with its rounding ttlo from a TwoSum, and the product's whole revolutions
taken out before it is added (exact: 360 k is a multiple of the product's
ulp). The angle is then good to a few ulps of 360 deg. Other terms (T^2..)
are as before. The rates (bug 48's drate) are the analytic ones, unchanged
in form. Positions move by the old rounding only: <= 7.4e-9 deg on Vulcan,
5.9e-12 on Waldemath, 2.0e-12 on Selena, 0 elsewhere (300 dates
-3000..+3000); speeds by <= 1.1e-10 deg/day through the corrected angle.
After it, the apparent rows of part 1 are at their ephemeris's floor:
Vulcan heliocentric 9.1e-11 deg/day (1.4e-7 with part 1 alone), Waldemath
1.6e-9 on the Swiss files and 1.8e-8 on Moshier, whose Earth is itself 5e-14
AU off a smooth curve (3.4e-10 deg at 0.0065 AU). G48's Vulcan
heliocentric 8.4e-9 -> 8.6e-11, Waldemath geocentric 3.3e-9 -> 2.4e-11.

G80 gains the true positions of the same four rows at -3000 and -200, three
dates each, speeds within 1e-9 deg/day (measured <= 3.4e-10) and jitter
within 2e-11 deg (<= 4.0e-12), and its part-1 limits are now 1e-9 / 2e-11
(Vulcan heliocentric; measured 9.1e-11 / 2.4e-12), 5e-9 / 5e-11 (Vulcan
geocentric; 1.2e-9 / 1.1e-11), 5e-9 / 2e-9 (Selena) and 5e-8 / 1e-9
(Waldemath, for Moshier's Earth). With part 1 alone 20 of the 24 rows fail
(the four Vulcan rows of part 1 and all sixteen true rows); before part 1
and on the upstream build all 24. G24 `vulcan_true_jitter_off`: Vulcan's
heliocentric true longitude at -3000, Moshier, largest fourth difference,
9.8e-13 deg shipped (3.05e-8 with part 1 alone), 3.05e-8 compat; G24
`waldemath_lt_round_off` is now 2.0e-9 (1.65e-8 with part 1 alone). G48,
G59, G64 pass.

**Part 3 (the light time's fixed point).** `app_pos_etc_plan_osc()`
iterates the light time on the orbit and stops when a pass changes it by
less than 1e-12 day, the body as read at that pass's dt, not the converged
one: Waldemath 2.3e-11 deg and Selena 1.5e-11 deg off the fixed point (their
barycentric motion, 0.017 AU/day, over ~1e-12 day at 0.0065 and 0.053 AU),
Vulcan and the heliocentric sets <= 1e-12. This fork's `swe_calc_orbel()`
(`app_pos_etc_orbel()`) had kept upstream's fictitious-body light time from
before entry 26: a straight line, x(t) - dt v(t), and the orbit read at
t - dt only with SEFLG_SPEED, so the position depended on the flag. Against
the fixed point, astrometric (SEFLG_NOABERR | SEFLG_NOGDEFL), thirteen dates
1700..2300: an orbit with a = 0.137 AU 1.6e-5 deg without the flag and
4.4e-9 with it, a = 0.387 AU 3.0e-6 and 7.2e-10, an Earth-centred one at
0.0068 AU 1.7e-9 and 3.0e-8 (its centre read at the rounded instant).

**Fix, as done here (part 3):** both finish the loop with one Newton step on
f(tau) = tau - |X(teval - tau) - O| / c, f' = 1 + (rho^ . v) / c, v the
body's barycentric velocity at teval (there with or without SEFLG_SPEED;
the Earth read at t - dt has none without it, which made a first try
move fictitious positions by 2.3e-13 deg with the flag), as entry 78 does for the planets, and
part 1's move takes the step with the rounding. `app_pos_etc_orbel()` now
solves the light time on the orbit for every flag set, as the fictitious
bodies do; its orbit is taken at M - n dt exactly and only its centre at the
rounded instant, so the orbit is moved by the Newton step and the centre by
the step and the rounding. After: the fictitious bodies <= 1.5e-12 deg
(Waldemath; Selena 2.7e-13), the orbel orbits 5.4e-15, 1.0e-14 and 3.6e-12,
the same with and without SEFLG_SPEED (positions bit-identical either way,
fictitious and orbel). Speeds unchanged in form; swe_calc_orbel()'s default
geocentric longitude speed against five-point differences over 2^-7 day goes
from 4.9e-6 to 1.2e-7 deg/day (the rest is below).

G80 gains (3): the four bodies, Swiss and Moshier, and the three orbel
orbits with and without SEFLG_SPEED, Swiss, against the fixed point
iterated in the gate (the body's true barycentric place read at t - tau and
moved by that instant's rounding, the observer at t) within 5e-12 and 2e-11
deg. The build before part 3 fails 10 of its 14 rows (the four Earth-centred
ones, all six orbel ones); upstream fails all 14 (Vulcan heliocentric by
1.0e-3, entry 26). G24 `orbel_lt_flag_off`: swe_calc_orbel()'s position for
a = 0.137 AU with against without SEFLG_SPEED, 0 shipped (1.55e-5 deg
before), 1.55e-5 compat.

Not fixed (found measuring part 3, `swe_calc_orbel()` only): as the first
call of a context it returns zeros (ecliptic) or (0.17, -0, 0) (XYZ) with its
flags as the return value and no error, until some `swe_calc()` has run; its
speeds are off five-point differences of its positions by 8e-8 deg/day in
the ecliptic of date with SEFLG_TRUEPOS (1.8e-12 in J2000), by 1.07e-6
heliocentric apparent in J2000 (the heliocentric light-time rate), and by
2.45e-3 deg/day on Moshier heliocentric (SEFLG_TRUEPOS, J2000). Fixed as
entry 88, where the 1.07e-6 turns out to be the probe's, not the function's.

Not fixed here: at exactly tjd = 2451545.0 every body's latitude of date
(planets too; Mars 1.5e-10, the Sun 2.7e-10 deg in a fourth difference) is
off its neighbours'; one instant only, so a J2000 special case somewhere in
the frame of date. Fixed as entry 87.

## 85. The Moshier Earth-Moon offset's rate is a difference of rounded arguments

**Severity: Correctness (2e-14 AU/day of roughness in every Moshier Earth velocity, 2.1e-6 of the Earth's acceleration from velocities 1e-5 day apart at 2000, 3.8e-5 at -3000; ~1e-12 AU/day of bias)**
**Where:** `swemplan.c`: `embofs_mosh_rate()`, `embofs_mosh()`, `moon_short()`; `swephlib.c`: `pre_pmat_d()`

The Moshier Earth is the EMB series less the Moon's share of a short
lunar series, `embofs_mosh()`. Entry 51 gave the EMB the series'
derivative and the offset a central difference over +-2^-8 day. The
series' arguments are degrees of 481267.88 T and the like, which round at
1e-12 rad once T is near 1 (5e-11 at -3000, T = -49): the offset's place
is rough at a few 1e-17 AU, and divided by 2^-7 day that is ~2e-14 AU/day
from one instant to the next. Measured, heliocentric J2000 Earth at
instants that are multiples of 2^-8 day, its acceleration from the
velocities at t +- 1e-5 day against the five-point second difference of
its positions over 2^-3 day: 3.8e-5 of itself at -3000, 4.3e-9 in 1900
(T ~ 0), 2.1e-6 in 2000, 2.1e-7 in 2100, 1.7e-5 in 2970; at +-2^-9 day
still 1.7e-8 in 2000. The difference's truncation, h^2/6 of the offset's
third derivative, left a bias besides: the velocity against the
five-point difference of the positions over 2^-4 day, 7.9e-13 to 1.6e-12
AU/day. Entry 76 met it in a star's aberration rate (1.56e-8 deg/day in
Polaris's right ascension) and worked round it on the star side.

**Fix, as done here:** `moon_short()` evaluates the series once, with
the place's arithmetic unchanged to the bit, and with a velocity pointer
also its derivative term by term: each argument's rate from its
polynomial, the sines' and cosines' by the chain rule, the distance's
from the parallax's, through `swi_polcart_sp()`, the obliquity held as
the place holds ctx->oec, and the precession's turning with
`swi_precess_speed()`. `embofs_mosh()` is its place;
`embofs_mosh_rate()` its velocity, and bug 66's geocentric Moon
(ctx->earth_moon) is now the series' place and velocity at t, not the
mean and difference of two samples (a parabola's 1e-10 AU). A wider span
or a five-point form does not get there, measured as below: the central
difference over 2^-7..2^-3 day leaves 1e-7..2.6e-5 of the acceleration
at -3000 and 2970 (the arguments' rounding is 50 times larger there) and
a bias growing as h^2 to 1e-9 AU/day; the five-point one over 2^-4..2^-2
day 1.5e-7..2.8e-6 at the range's ends, and costs four series reads. The
derivative costs less than the two reads it replaces: calc-moseph 24.94e9 -> 24.90e9 instructions (-0.14%; `bench`
1.063 -> 1.048 s, min of 3x7).

After: the acceleration 2.9e-8 at -3000 (what is left is the EMB series'
own roughness, ~2e-16 AU/day), 2.7e-9, 5.2e-9, 1.3e-9, 9.4e-9; the
velocity's bias <= 1.9e-13 AU/day, the obliquity's turning, which the
places have (each rotated with its own date's ctx->oec) and the rate
does not: taking it with `swi_obliquity_rate()` gives <= 8.6e-14 but
costs 1.8% on calc-moseph (a `swi_epsiln()` a call), not kept. Positions:
the heliocentric Earth bit for bit; geocentric planets up to 2e-6 mas,
the aberration's change, the old bias over c; the Moon 8e-6 mas, one ulp
of its heliocentric intermediate (1e-16 AU); the heliocentric Moon 8e-15
AU, the light time times the old bias. The compat build is bit for bit
unchanged. `swi_mosh_earth_accel()` (bug 66 part 3) differences places,
not this rate, and `mosh_moon_earth_acc()` (bug 53) is the pair's
gravity, taking the Earth's velocity as it is: both unchanged in form.
G65's Moshier speeds move within 5e-11 deg/day either way, all within
its limits; G52's Mars without aberration 1.30e-10 -> 2.8e-11 deg/day,
Mercury 1.37e-10 -> 5.6e-11; G50 within its noise.

Entry 76's star side no longer needs its Moshier form: with the
velocity smooth, the files' central difference over 2^-9 day serves
Moshier too, and G75's Moshier stars go to <= 1.41e-11 deg/day
(1.82e-11 with 2B - A over 2^-3 day). The cost there is the third Earth
read, not the form: a Moshier `swe_fixstar2()` with SEFLG_SPEED is 291.6k
instructions against 292.6k, still +18% on the two reads before entry 76
(why three reads, see there).

**Part 2.** Two things were left. (a) The 2.9e-8 at -3000 was not the
EMB series: the residual falls as 1/spacing (velocity noise), and the
offset's velocity, 7.3e-6 AU/day, still turned through arguments
rounded at ulp(rate T), 6.5e-11 rad at T = -49: 4.7e-16 AU/day.
Bug 50's exact arguments cover the EMB series (`mosh_arg()`, the
polynomial terms' T term included) and bug 51's derivative takes A' as
the sum of exact frequencies; nothing there rounds that needn't. The
short lunar series' arguments now do the same (`moon_arg()`): T as th +
tl, exact, the product rate * th with its fma error, the large part
reduced modulo 360 exactly (p - 360 floor(p/360) is a multiple of ulp(p)
below 360), the small T^2 and T^3 terms added after. This moves the
offset's places by their old rounding: the heliocentric Earth by up to
4.9e-15 AU at -2700, 2.1e-15 at -1800, geocentric places by <= 2.2e-6
mas (planets) and 8e-6 mas (the Moon's one ulp); the compat build is bit
for bit unchanged. (b) The obliquity's turning, which each place has
through its own date's ctx->oec: eps' from the same Vondrak poles the
precession's rate already evaluates, cos eps = P . Q, eps' = -(P' . Q +
P . Q') / sin eps (`swi_precess_matrix_rate_eps()`, the mean obliquity of
`swi_epsiln()`'s pole angle exactly), applied with
`swi_ecl_frame_speed()`; the precession's rate then from the same M and
dM/dt, inline. `swi_obliquity_rate()` remains for the other precession
models. calc-moseph 24.904e9 -> 24.871e9 instructions (-0.13%): the
first try at the arguments, five fma-quotient-remainders and fmod, was
+0.64%; one quotient for T and a floor reduction cost less than the
precession call they save.

After both: the acceleration 8.1e-9, 2.8e-9, 5.0e-9, 1.6e-9, 2.6e-9 at
-3000, 1900, 2000, 2100, 2970 (falling as 1/spacing to ~2e-9 at 1.6e-4
day): velocity noise of 2e-17 to 3.4e-17 AU/day, one or two ulp of a
longitude near 2 pi (8.9e-16 rad) times the Earth's speed, the series'
place in a double -- genuine rounding, nothing to take out. The
velocity's bias 1.9e-14 to 5.7e-14 AU/day (was 2.9e-14 to 1.9e-13), the
five-point reference's own rounding (1.1e-15 AU of position over 2^-4
day). G52, G65, G50 and G40 within their noise; G75 Moshier <= 1.44e-11.

G84 `tests/moshvel.c`: the Moshier heliocentric J2000 Earth at -3000,
1900, 2000, 2100 and 2970 (the range ends in 2973): the acceleration as
above within 2e-8 of itself, the velocity within 1.2e-13 AU/day: 10
checks. The build before bug 85 fails 9 (all but 1900's acceleration),
the build after its first part 5; the upstream build fails all 10 (its
backward difference, entry 51). G24 `mosh_earth_rough_off`: the
acceleration in 2000, 5.0e-9 shipped, 2.6e-3 compat;
`mosh_earth_vbias_off`: the velocity's bias in 1900, 2.7e-14 AU/day
shipped (1.87e-13 after the first part), 3.4e-8 compat.

## 86. The JPL barycentric Sun takes its instant of date from the Earth's slot

**Severity: Correctness (the JPL barycentric Sun's latitude speed of date off by up to 1.9e-5 deg/day, NaN as a context's first request; its sidereal longitude off by the ayanamsa's change since another request's date, 0.14 deg ten years on)**
**Where:** `sweph.c`: `app_pos_etc_sbar()`, `app_pos_rest()`

The barycentric Sun is answered in the Earth's slot (`pldat[SEI_EARTH]`,
whose `xreturn` `swe_calc()` reads), and `app_pos_etc_sbar()` passed that
slot's instant to `swi_bias()` and to `app_pos_rest()`, which takes the
ecliptic of date's turning (entry 33), the ayanamsa and, in the bias, the
JPL Horizons mode from it. On the Swiss files `sweplan()` has just filled
the Earth's slot for the Sun's instant. On JPL only the Sun's slot
(`SEI_SUNBARY`) is read, so the instant was whatever the Earth's slot last
held: JD 0 in a fresh context, then (since entry 71's empty mark
SWI_TJD_NEVER) a NaN speed, or the instant of the last request that read
the Earth. Measured on de200, against five-point differences over 2^-6
day at binary-fraction instants: the latitude speed of date 1.03e-5
deg/day at seven dates 1968-2000 (1.85e-5 at eight dates 1925-2075, each
right after a request for Mars ten years on), the longitude 2.8e-7; with
SEFLG_NONUT the mean obliquity's rate alone, at another date, 5.9e-12. The
sidereal (Lahiri) longitude after that Mars request was the tropical less
the ayanamsa ten years on, 0.140 deg off, and its speed 3.8e-5 deg/day.
J2000 and equatorial places and the Swiss files were unaffected. Found by
entry 78's agent.

**Fix, as done here:** `app_pos_rest()` takes the instant explicitly;
every caller passes its slot's instant as before, and `app_pos_etc_sbar()`
the Sun's (`psbdp->teval`), as it already did for the precession; the bias
takes the Sun's instant too. The compat build passes the Earth slot's, as
upstream. After: the JPL Sun's latitude speed 9.2e-13 deg/day, longitude
2.8e-11 (seven dates); over G85's eight dates and four frames <= 1.6e-12,
sidereal 5.6e-11 (the ayanamsa's rate, the same on every body), its first
request 1.2e-13, its sidereal longitude exact. Swiss results do not move
(the two instants are equal there). Every barycentric body was checked on
JPL and the Swiss files, of date, without nutation, J2000, equatorial and
sidereal: the Earth, Moon, Mercury, Mars, Jupiter, Pluto and Chiron within
4.2e-12 deg/day (5.7e-11 sidereal), the Sun on the files 9.7e-10, their
own (the same with SEFLG_TRUEPOS). Moshier refuses SEFLG_BARYCTR.

The second part of this item, `swi_aberr_light()`'s rate of the shift
along the body's motion as a backward difference over 1e-4 day, was
already done by entry 73 (the derivative of A(u) along the velocity, v
held). Measured here before and after that entry, the geocentric
aberration's part of a speed (the residual against a five-point
difference with aberration less that without, Sun, Mercury, Venus, Mars,
Jupiter, Ceres, ten dates -2900..2900): angles 1.3-2.7e-10 -> 0.9-2.5e-11
deg/day, the distance rate 1.8-4.3e-12 -> 0.4-2.1e-13 of the distance per
day; the geocentric Moon 1.77e-8 -> 2.7e-11 deg/day with entry 73's other
causes. Nothing of it is left: an independent analytic version written
here gave the same numbers. The topocentric residual, 3-9.6e-10 deg/day
before and after, is not the aberration's directional rate. G51's
geocentric angle limit goes from 5e-9 to 1e-10 and its distance limit
from 1e-11 to 5e-13; the build before entry 73 fails 12 of its 20 checks
there.

G85 `tests/baryspeed.c`: the barycentric Sun, Earth, Moon, Mercury, Mars,
Jupiter, Pluto and Chiron on JPL and the Swiss files, five frames, eight
dates 1925-2075, each speed taken after a request ten years on: angles
within 2e-11 deg/day, 2e-10 sidereal, 2e-9 for the Sun on the files,
distance rates within 5e-11 of the distance per day; the JPL Sun's first
request within 2e-9 and its sidereal longitude within 1e-9 deg: 162
checks.
The build before the fix fails 4 (the first request, of date, sidereal
speed and longitude); the upstream build fails 162. G24
`bary_sun_sid_off`: the sidereal longitude, 0 shipped, 0.140 deg compat.

## 84. `jplplan()`'s Earth and Sun answer for any ephemeris, and the JPL barycentric Sun is finished at the Earth slot's instant

**Severity: Correctness (the JPL Moon 160 deg off, Mercury 0.30 deg, after a JPL call that fell to Moshier; a heliocentric asteroid 0.24 deg off after the JPL barycentric Sun; that Sun's sidereal place taken at another instant, 0.11 deg, or at JD 0)**
**Where:** `sweph.c`: `jplplan()` (upstream 2034, 2056), `swecalc()`'s barycentric Sun (757); latent, `app_pos_etc_plan()` (2701-2707)

Three hygiene defects around the Earth's and the Sun's save-area slots.

- `jplplan()` reuses the Earth and the barycentric Sun in their slots
  whenever the slot's instant is tjd, whatever ephemeris filled it, where
  its planet branch, `sweplan()` and `swi_moshplan()` all check the slot's
  ephemeris too. It leans on `free_planets()`, which `swe_calc()` runs when
  the ephemeris ASKED for changes; nothing runs it when a call answers from
  another. A JPL call whose light time leaves the file is redone with
  Moshier (`main_planet()`'s BEYOND_EPH_LIMITS branch) -- answered with
  `swe_set_ephe_fallback(1)`, refused without, the slots filled either way
  -- and leaves the series' heliocentric Earth in the Earth slot at tjd.
  The next JPL call at that instant took it for the barycentric Earth and
  returned the result flagged `SEFLG_JPLEPH`. Measured 0.125 day into
  DE200, after Pluto (light time 0.28 day): the Moon 321.37543 against
  161.30743 (160.1 deg), Mercury 0.30 deg, Mars 0.090, Jupiter 0.010,
  with the fallback refused and allowed alike. The other paths that change
  the answering ephemeris without `free_planets()` cannot reach it: a
  missing JPL file falls to the Swiss files, whose `sweplan()` checks the
  slots' ephemeris, as `swi_moshplan()` does; `swe_set_jpl_file()` and
  `swe_set_ephe_path()` run `free_planets()`; a per-call flag change runs
  it in `swe_calc()` and the fixed stars' two calculations.
- `swecalc()`'s barycentric Sun on JPL read the Sun into its slot with
  `swi_pleph()`, setting the slot's instant but not its ephemeris, and
  left the Earth slot alone; `app_pos_etc_sbar()` then finishes the Sun
  through the Earth slot (`SEI_SUN` is `SEI_EARTH`), at that slot's
  instant. On a fresh context that is the emptied instant: upstream JD 0
  (the sidereal place 270.004 for 177.493), this fork's `SWI_TJD_NEVER`
  since entry 71 (every speed NaN, every sidereal coordinate). After a call
  3000 days on, the ayanamsa and the ecliptic's turning were that day's:
  177.378, 0.11 deg off, and the longitude speed 6.62412e-2 for 6.62415e-2
  deg/day. And `sweph()` gives an asteroid read with `DO_SAVE` the Sun
  slot's ephemeris, which was none, so `app_pos_etc_plan()` neither took
  the Sun out of a heliocentric asteroid nor measured its light time from
  the Sun: after the JPL barycentric Sun, heliocentric Pallas 117.18588
  against 116.95039 (0.24 deg), Juno 0.14, Ceres 0.11, Vesta 0.10, Chiron
  0.033, Pholus 0.0059. The Swiss files' branch reads both through
  `sweplan(SEI_EARTH, DO_SAVE)` and has neither problem.
- Latent: `app_pos_etc_plan()` builds the observer at t - tau for the
  aberration rate (`xobs2`) from `xearth`, which the JPL and Moshier
  branches read only for a geocentric or topocentric request; for a
  heliocentric or barycentric one with `SEFLG_SPEED` it copied
  uninitialised stack. Only the aberration rate reads `xobs2`, and
  `plaus_iflag()` turns aberration off for both centres before any path
  reaches the function, so nothing reached a result: with every working
  local poisoned with NaN, 5100 requests (25 bodies, 17 flag sets, three
  ephemerides, fresh, warm and planetocentric) answered bit-identically,
  and valgrind saw no use (a copy is not a use). This fork's bug 53
  rewrite of the Moshier branch kept the gap.

**Fix, as done here:** `jplplan()`'s Earth and Sun caches answer only for a
slot filled from JPL (`iephe == SEFLG_JPLEPH`), as its planet branch does;
the JPL barycentric Sun is read through `jplplan(SEI_EARTH, DO_SAVE)`, the
JPL twin of the Swiss branch's `sweplan()` call, so the Earth and the Sun
are both in their slots at tjd, marked JPL (one Earth read more, on that
path only); `app_pos_etc_plan()` builds `xobs2` only where there is an
Earth, geocentric and topocentric. Upstream's code is under
`SWE_UPSTREAM_COMPAT`. Searched for and not found: a first-call Moshier
Chiron that moves with `SEFLG_SPEED` (6e-5 mas reported) -- Chiron at
300 instants 1600-2170 and every body at 60, Swiss, Moshier and DE200, 20
flag sets, fresh, speed-first and speed-last in one context, and the
default context's first call in a new process: none differ, on ts20 and,
for Chiron, just before and after bug 53. Not this entry's,
and left: the Swiss files run past their nominal 600-year ends (`sepl_12`
to JD 2378575.7, `seas_12` to 2378857.5, past 1800.0) and an open file is
kept for any instant it covers, so a context warm from just before a join
answers just after it from the older file, a fresh one from the newer (JD
2378496.6, after a call 1.5 days before: the barycentric Sun 47.6 mas,
Juno 8.7, Chiron 0.67 geocentric; speeds up to 4.7e-3 mas/day where the
light time crosses the join). calc-swieph and calc-moseph unchanged within
noise.

G83 `tests/callorder.c`, bit for bit: (1) fresh contexts, with and without
`SEFLG_SPEED`, 25 bodies x 11 flag sets x Swiss, Moshier and DE200 x four
instants 1700-2050 away from the joins; (2) fresh against warm from eleven
earlier calls (the Sun, its barycentric place plain and sidereal, the Moon,
Mercury, the same body with the speed flag the other way, 1.5 and 3000
days off, heliocentric Chiron, Jupiter's centre of body, the topocentric
Moon), with and without speed; (3) the JPL Sun, Moon, Mercury, Mars,
Jupiter, Chiron and heliocentric Earth 0.125 day into DE200 after a JPL
Pluto, fallback refused and allowed, against fresh; (4) one instant
through a sequence of per-call ephemeris switches, barycentric Suns,
heliocentric asteroids, the fallback toggled and a missing JPL file set and
replaced, against fresh; every coordinate finite. 20001 rows, none differ.
The upstream build fails 2027 (1903 of them check 1's, upstream's
SEFLG_SPEED dependences that earlier entries fixed); ts20 before this fix
42 (the four fresh NaN speeds, 23 warm rows, 12 after Pluto, 3 in the
sequence). Each half of the fix is needed: without the ephemeris key 12
rows fail (check 3), without the barycentric Sun's Earth 18 (its NaN).
G24 `jpl_stale_earth_off`: the JPL Moon after Pluto against alone, 0 deg
shipped, 160 compat. G24, G52, G62, G66 and G70 pass.

## 79. The topocentric site's acceleration is 1.6e-6 low

**Severity: Correctness (6e-10 deg/day on every topocentric apparent speed through the aberration's rate: 1.2e-9 on Neptune's mean points' longitude on the equator, 3.9e-10 on their latitude)**
**Where:** `sweph.c`: `observer_accel()`

Found as `swe_nod_aps()`'s topocentric mean points being worse than the
geocentric ones (SEFLG_TOPOCTR, default flags): 5.5e-10 AU/day on every
distance rate and 5.0e-8 deg/day on Mercury's longitude against a
five-point difference at h = 2^-6 day. That much was the reference: the
site turns at 6.3 rad/day, and the five-point's truncation, (omega h)^4 / 30
of the diurnal parallax's rate, is exactly that at 2^-6 (16 times less per
halving of h, measured), while below 2^-8 the site's place read at a
rounded UT (4.7e-10 day at a JD of 2.4e6, 6e-14 AU) gives 1e-9 of noise.
Against the slope of a least-squares polynomial (degree 8, 257 places
2^-11 day apart; degree 10 at 2^-12 agrees to 3e-11) two defects of the
observer were left, both shared with `swe_calc()`:

- The site's velocity lacked the sidereal time's precession in right
  ascension, the equation of the equinoxes' rate and the nutation matrix's
  rate, 1.2-2.4e-11 AU/day (2.9e-9 deg/day on Mercury's points, 3.8e-7 on
  the topocentric Moon). Found here and independently under entry 73,
  whose fix (G72) is the one in the tree; nothing of this entry's is in
  `swi_get_observer()`.
- `observer_accel()`, the site's acceleration that the aberration's rate
  takes (bugs 34, 52, 70), was a central difference of its velocity over
  +-2^-11 day, (omega h)^2 / 6 = 1.6e-6 of it low: 1.9e-9 AU/day^2 on the
  equator, a / c of the diurnal aberration's rate, 6e-10 deg/day on every
  topocentric apparent speed. With entry 73's site velocity in place,
  Mercury..Neptune's mean points at 0, 47 and 70 N, seven instants
  -3000..3000: longitude 9.6e-10 (Mercury) to 1.2e-9 (Neptune) deg/day on
  the equator, 8.3e-10 at 47 N, latitude 3.7-4.0e-10 for every planet on
  the equator, scaling with the cosine of the site's latitude; geocentric
  the same points are within 3.5e-10 and 2.2e-11.

**Fix, as done here:** `observer_accel()` divides its difference by
sin(omega h) / (omega h), exact for a velocity turning at omega, and takes
it over +-2^-6 day, where the velocities' rounding (the site's turning read
at a rounded UT) is 5e-9 of a, not 2.4e-7 as at 2^-11: within 7e-9 of a
fit to the site's own velocity. Positions are unchanged to the bit; cost
none (two observer reads, as before). After, the same checks: longitude
<= 3.5e-10 deg/day (Neptune, its geocentric floor), latitude <= 1.1e-10
(Mercury at J2000 on the equator, the rounded UT's phase), distance <=
6.4e-11 AU/day (Neptune's node at -1, geocentric the same); Saturn's
latitude speeds 3.5e-10 to 7e-12.

G78 `tests/nodapstopo.c`: the longitude, latitude and distance speeds of
the four mean points of Mercury..Neptune, topocentric, default flags, at
0, 47.37 and 70 N, seven instants -3000..3000, against the fit; within
6e-10, 2e-10 deg/day and 1e-10 AU/day. The tree before this fix (with
entry 73's) fails 27 of its 63 checks, the upstream build all 63. G24
`topo_accel_off`: Saturn's mean ascending node seen from the equator on
2026-10-24, its latitude speed against the fit, 7.6e-13 deg/day shipped
(3.5e-10 before), 8.0e-6 compat. G42, G43, G47, G54, G63, G69, G72 pass.
G1: topocentric apparent speed rows move by up to 1e-9 deg/day, positions
do not.

## 80. `swe_nod_aps()`'s osculating speeds difference rounded points over a few minutes

**Severity: Correctness (5.6e-7 deg/day on Venus's osculating perihelion and aphelion, 6.0e-8 on the Sun's perigee and apogee, 2.0e-6 on the Moon's osculating perigee; 3.0e-5 on Venus's with DE200; positions 3e-9 deg)**
**Where:** `swecl.c`: `swe_nod_aps_r()`, the osculating points (SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR)

The osculating points' speeds are a central difference of the points at
tjd -+ dt, dt = 1e-3 day per AU of the body's distance (7.2e-4 day for
Venus, 1e-3 for the Earth's orbit) and 1e-4 for the Moon. Two things made
that difference something other than the points' rate:

- The eccentricity was sqrt(1 - p / a), and 1 - p / a is e^2 less a few
  ulps of 1, so e carried 1e-16 / e^2 of itself and the true anomaly
  through it, cos E = (1 - r / a) / e, about 1e-11 rad on Venus's orbit
  (e = 0.0068). Venus's perihelion jittered by 5.1e-10 deg rms from one
  instant to the next (201 instants 2^-20 day apart; the same formula in
  long double moves it by up to 7.6e-10), Neptune's heliocentric one by
  5.8e-11. Over 2 dt = 1.4e-3 day that is 6e-7 deg/day of speed.
- The outer instants were tjd - dt and tjd - dt + 2 dt, each rounded, and
  divided by 2 dt: off their spacing by up to a JD's ulp, 2.3e-6 of the
  Moon's points' speeds (1.9e-6 deg/day on its osculating perigee), less
  for the planets' longer steps.

Each sample is in the ecliptic of its own instant, with the same bias,
precession, nutation and (for the Earth) EMB; that part was right.
Measured against five-point differences of the library's own positions at
binary-fraction instants (h = 2^-6 day, which agrees with 2^-4 and 2^-5),
the Sun and Mercury..Neptune, osculating and barycentric-osculating,
geocentric and heliocentric, seven instants -3000..3000: Venus 2.0e-7
deg/day geocentric and 5.6e-7 heliocentric in longitude, 2.8e-8 in
latitude, 6.1e-9 AU/day in distance; the Sun's perigee 6.0e-8; Mars 3.6e-9,
Jupiter 1.9e-8; Neptune 1.1e-7 to 1.7e-7 (barycentric), its jitter in the
reference too. JPL (DE200, 1700-2150), heliocentric: Venus 3.0e-5, the
Sun's perigee 3.8e-8. (Measured on the tree with entries 66-78 in.)

**Fix, as done here:** the eccentricity is the length of the
eccentricity vector, ((v^2 - GM / r) r - (r.v) v) / GM, and the true
anomaly the angle from it to r about the orbit's normal, each good to
1e-16 / e: Venus's perihelion jitter 2.7e-12 deg rms, Neptune's 5.6e-12
(1.8e-10 at -3000, which is the Swiss files' own state there: 2e-10 deg in
its eccentricity vector computed directly). The outer instants are tjd -+
a binary step, exact but for a JD within the step of a power of two, and
divided by their own spacing: 2^-6
day for the planets, where the frame's nutation bends a three-point
difference by 2e-10 deg/day and the points' rounding adds about as much,
2^-11 for the Moon, whose perigee turns too fast for a longer one (4e-8
deg/day, against 5e-7 at 2^-9 and 1.7e-7 at 2^-13). Positions move by the
old formula's rounding: <= 2.7e-9 deg (Neptune), 1.0e-9 on Uranus's
points, 5.1e-10 on Venus's, 1.0e-10 on the Sun's, the rest below 5e-11 (13
instants -3000..3000, both methods, four flag sets); with and
without SEFLG_SPEED they are still one position (G47). After, the same
checks: longitude <= 6.5e-10 deg/day (Neptune geocentric at -3000),
latitude <= 2.1e-11, distance <= 2.6e-11 AU/day. JPL heliocentric <= 4.6e-10
deg/day, geocentric <= 6.5e-10 (Neptune), as on the Swiss files. The Moon's
osculating points: 2.0e-6 to 3.9e-8 deg/day. G42's osculating rows are now
its mean rows' floor (Venus 2.7e-7 to 2.4e-8, Mars 1.8e-8 to 1.4e-8).

At exactly tjd = 0 the points with and without SEFLG_SPEED agree (Mars's
osculating ascending node 320.6782751218 both, entry 71's caches), before
and after this fix.

G79 `tests/nodaposcspeed.c`: the longitude, latitude and distance speeds of
the four points (the Sun's two), osculating and barycentric-osculating, of
the Sun and Mercury..Neptune, geocentric and heliocentric, default flags,
seven instants -3000..3000, against five-point differences over 2^-6 day;
within 1.5e-9, 1e-10 deg/day and 1e-10 AU/day. The tree before fails 51 of
its 96 checks, the upstream build all 96. G24 `oscu_apsis_speed_off`:
Venus's osculating perihelion, heliocentric, at J2000 + 100 days, 3.5e-10
deg/day shipped (3.7e-7 before), 4.7e-7 compat. G42, G43, G47, G54, G63,
G69, G72, G78 pass. G1: the osculating rows' speeds move (Venus by up to
6e-7 deg/day, the Moon's perigee 2e-6), their positions by <= 3e-9 deg.

## 82. The heliacal memos answer for an atmosphere, latitude or star catalogue they do not hold

**Severity: Correctness (returned values: 0.67 mag in a limiting magnitude, 0.46 mag in a star's, a day in a heliacal event)**
**Where:** `swehel.c`: `call_swe_fixstar_mag()`, `kOZ()`, `ka()`, `Deltam()`, `swi_hel_forget()`; `sweph.c`: `ctx_forget_instants()`, `swi_close_keep_topo_etc()`, `ctx_release()`; `sweconfig.c`: `swi_config_apply()`; `sweph.h`: `struct hel_state`

`swehel.c` memoises five results per context (upstream: five pairs of TLS
statics). Three were keyed on less than their result depends on:

- `kOZ()`, the ozone extinction, on the Sun's altitude and `SunRA()`; it
  depends on the latitude too.
- `ka()`, the aerosol extinction, on the same two; it depends on the
  latitude, eye height, temperature, humidity and visual range (or
  extinction coefficient).
- `Deltam()`, the extinction along the line of sight, on those two and the
  object's altitude; it depends on all of the above, the pressure, and
  `SE_HELFLAG_HIGH_PRECISION` (through `AppAltfromTopoAlt()`).

So the same instant and place under another atmosphere was answered with
the previous request's extinction, and so was `SE_HELFLAG_VISLIM_DARK`
(which fixes the Sun's altitude at -90) at another latitude or eye height.
Measured with `swe_vis_limit_mag()`, Venus at 2000 Jan 1 12h UT from
8.55 E 47.37 N, each request right after the one before it against the
same request in a fresh process: humidity 40 -> 80 % -6.447 for -7.121
(0.67 mag), temperature 15 -> 30 C 0.62 mag, a visual range of 40 km 0.43,
an extinction coefficient of 0.25 0.57, 900 mbar 0.53; in the dark sky,
47 N -> 33 S 0.10, 400 -> 1500 m 0.048, humidity 80 % 0.26; Sirius at
humidity 80 % 0.25. None was emptied by anything, so the dark-sky answer
after `swe_close()` was still the one before it (0.26 mag).

`call_swe_fixstar_mag()`'s memo, keyed on the star's name, was not emptied
by `swe_close()` or a new ephemeris path, so a star kept the magnitude of
the catalogue it was first read from: Sirius after `swe_close()` and a
path whose `sefstars.txt` gives it -1.00 was -1.46 in
`swe_vis_limit_mag()`'s `dret[7]` and `swe_heliacal_pheno_ut()`'s
`darr[20]`, and `swe_heliacal_ut()`'s end of visibility of its heliacal
rising came a day early (2451778.67 for 2451779.67). It took the name
before the lookup, so a failed lookup left the name keyed to the previous
star's magnitude, and an empty name was a hit on the empty memo; and a hit
left `serr` as it was where the lookup clears it, so a warning from `ka()`
reached `swe_vis_limit_mag()`'s `serr` on the second call only.

`kOZ()`, `ka()` and `Deltam()` took a key of 0 as empty, and a hit dropped
the warning `ka()` writes for an over-long visual range or an extinction
coefficient too small for the other terms. The key of 0 is not reachable
through the public functions: `SunRA()` is never exactly 0.

`SunRA()`'s memo is keyed on UT and nothing else, which is complete:
`SIMULATE_VICTORVB` (always defined) makes it the monthly approximation, a
function of UT alone, so neither the ephemeris flag nor delta T reaches it.
G81's sequence alternates both at one UT and finds nothing. Entry 71 gave
it `SWI_TJD_NEVER`.

**Fix, as done here:** each memo is keyed on every input of its result,
has a `valid` flag for empty, and keeps what its computation wrote to
`serr` -- `ka()`'s warning (by number, also in `Deltam()`'s memo, which
reaches `ka()` on both its branches), the star lookup's `serr` -- to write
it again on a hit. The star's memo is filled on success only.
`swi_hel_forget()` empties all five; context creation, `swe_close()`,
every path change (`swi_close_keep_topo_etc()`) and an adopted path
(`swi_config_apply()`) call it. No setter feeds the extinction memos, now
that their keys are complete. Under `SWE_UPSTREAM_COMPAT` the keys, the
empty 0, the name before the lookup and the dropped warnings are
upstream's, and nothing empties them.

Not fixed here: `swe_fixstar_mag()`'s own one-star memo in `sweph.c`
(`sp.fs1mag`, and `sp.fs1` in `swe_fixstar()`) is not emptied by
`swe_close()` or a path change either: Sirius -1.46 after `swe_close()`
and the -1.00 catalogue, and -1.00 after a path back to the -1.46 one,
until another star is asked for. Fixed as entry 92.

G81 `tests/helmemo.c`: one sequence of 37 `swe_vis_limit_mag()`,
`swe_heliacal_pheno_ut()` and `swe_heliacal_ut()` requests alternating the
atmosphere, eye height, latitude in the dark sky, the high-precision flag,
the ephemeris (Swiss, Moshier, de200), a user delta T, `swe_close()` and
two star catalogues, each answer -- return code, every value, `serr` --
against the same request in a fresh process; all agree within 1e-6 (an
order dependence outside the memos moves a de200 answer after a Swiss one
by 3.9e-8 deg; G81 does not judge it). The build before the fix fails 17, the upstream build the same 17. G24
`hel_memo_off`: Venus at humidity 80 % right after 40 %, 0 mag shipped,
0.67 compat. G24, G70 and G20 pass.

## 83. A setting made before the first calculation is lost, and the first heliacal call after a setter fails

**Severity: Correctness (a failed call; a lost setting: 22.6 s of delta t, 158 s of delta t in AD 1000, 0.9 deg of refraction)**
**Where:** `sweconfig.c`: `swi_config_publish()`, `swi_config_reset()`, `cfg_defaults()`; `swehel.c`: `set_topo_for_this_call()` (`Magnitude()`, `swe_vis_limit_mag_r()`, `swe_heliacal_pheno_ut_r()`, `swe_heliacal_ut_r()`); `swephlib.c`: `swe_set_tid_acc_r()`, `swe_set_delta_t_userdef_r()`, `swe_set_interpolate_nut_r()`; `swecl.c`: `swe_set_lapse_rate_r()`; `sweph.c`: `swi_init_swed_if_start()`

On the process-wide default context,

    swe_set_ephe_path(path);
    swe_vis_limit_mag(tjd_ut, dgeo, datm, dobs, "venus", 0, dret, serr);

returned ERR, "geographic position has not been set", at any instant; the
same call again worked, and after any other setter -- or `swe_close()` and
`swe_set_ephe_path()` -- the next one failed again. So did
`swe_heliacal_pheno_ut()`. Three causes, two of them this fork's:

- The configuration master (`sweconfig.c`) started as zeros, and
  `swi_config_reset()` (from `swe_close()`) re-armed it as zeros, though its
  header says "from the compile-time defaults". A setter merges in its own
  group only, so the others held zeros, and the first `swi_config_sync()`
  after it adopted every group the context had not claimed: an observer
  not set, a lapse rate of 0 for 0.0065, a tidal acceleration of 0. After
  `swe_set_ephe_path()` alone the first calculation set the lapse rate to
  0, so `swe_azalt()`'s Sun from 2000 m, 15.75 min before its true rise,
  was refracted to -1.243 deg where it is -2.215, below the dip (-1.235):
  0.97 deg; `swe_ctx_new()` after it inherited a lapse rate of 0 and a
  tidal acceleration of 0.
- `swehel.c` set the observer of the call under `SWI_CFG_LOCAL` (which
  claims nothing) without syncing first, the order G20 fixed in `swecl.c`,
  so that sync -- at the first `swe_calc()` inside the call -- replaced it
  with the master's "not set".
- The lapse rate, a static in upstream, is a context field here, and
  `swe_set_lapse_rate()` did not initialise the context, so set before any
  calculation it was wiped by the default context's first-use memset
  (`swi_init_swed_if_start()`): 0.05 K/m read as 0.0065, 0.90 deg in the
  same refraction. Upstream shares this one for `swe_set_tid_acc()`,
  `swe_set_delta_t_userdef()` and `swe_set_interpolate_nut()`: as the
  first library call, `swe_set_tid_acc(SE_TIDAL_DE200)` left -25.80 and
  `swe_deltat()` in AD 1000 1463.43 s for 1305.30; a user delta t of
  86.4 s was 63.83 (the automatic one) after the first `swe_calc_ut()`;
  `swe_set_interpolate_nut(TRUE)` was lost (8.6e-8 deg of nutation). The
  manual's own advice for `swe_deltat()` -- "it is safe if you first call
  swe_set_tid_acc()" -- is the order that lost it.

The other setters (`swe_set_topo()`, `swe_set_sid_mode()`,
`swe_set_jpl_file()`, `swe_set_astro_models()`, `swe_set_ephe_fallback()`)
initialise first and were right in every order; so were
`swe_heliacal_ut()`, which calculates before it sets its observer,
`swe_topo_arcus_visionis()` and `swe_heliacal_angle()`. Upstream loses
`swe_set_astro_models()` set as the first call too (1.2e-5 deg in
`SE_ECL_NUT`), which this fork already keeps.

**Fix, as done here:** the first publish after a start or a
`swi_config_reset()` fills the master with the defaults a fresh context
has (`cfg_defaults()`, under the mutex), so an adopted group nobody set is
the library's default; `swehel.c` syncs before it sets its observer
(`set_topo_for_this_call()`, as `swecl.c`'s); the four setters initialise
the context first (`swi_init_swed_if_start()` now marks it initialised
before its own `swe_set_tid_acc_r()`, which would otherwise come back to
it). The first two and the lapse rate are this fork's own defects and are
fixed in both builds; under `SWE_UPSTREAM_COMPAT` the other three setters
do not initialise, as upstream.

Not fixed here, upstream's too: a manual tidal acceleration is reset to
automatic by every path change, the implicit one at the first calculation
when the files are named by `SE_EPHE_PATH` included --
`swe_set_tid_acc(SE_TIDAL_DE200); swe_calc_ut(...)` leaves -25.936 -- where
the manual says the automatic setting is blocked once it has been used.
Fixed as entry 92.

G82 `tests/firstset.c`: every public setter and every heliacal entry point
on the default context, each in a forked, pristine process, set or called
after `swe_set_ephe_path()`, as the very first call (files by
`SE_EPHE_PATH`), and after `swe_close()` and re-open in both ways, against
the same in a context warm from a calculation, and the unset library after
`swe_set_ephe_path()` against `SE_EPHE_PATH`; each setter must move its
observation by more than 1e-7, and every observation agree within 1e-9:
54 scenarios pass. Before the fix 9 fail, the compat build the same 9 (3
after it: upstream's), the upstream build 4 of 49. G24
`dt_userdef_first_off`: a user delta t set as the first call, 0 s
shipped, 22.6 s compat. G24, G81, G70 and G20 pass.

## 90. `swe_calc(SE_ECL_NUT, SEFLG_SPEED3)` differences the four angles as a position; `jplplan()`'s Earth also takes the other-planet branch; `-Og` false positive on `dt`

**Severity: Correctness / Memory (x[3] the true obliquity's rate in place of the nutation in obliquity, x[4], x[5] nonzero; 576 valgrind errors)**
**Where:** `sweph.c`: `swe_calc_r()`, `jplplan()`, `app_pos_etc_plan()`

Three small defects, found while fixing 84.

- `SE_ECL_NUT` with `SEFLG_SPEED3` ran `calc_speed()` over its answer,
  (true obliquity, mean obliquity, nutation in longitude, nutation in
  obliquity), as if they were a longitude, latitude and distance. x[3],
  the nutation in obliquity (-1.6026e-3 deg), came back as the true
  obliquity's rate (-6.06e-6), x[4] and x[5] as the rates of the mean
  obliquity and the nutation in longitude (-3.6e-7, 4.6e-5), and
  `denormalize_positions()` read `x0[12]` and `x2[12]`, which that layout
  never writes (valgrind: 576 errors). `swe_calc_pctr()`'s internal
  `SE_ECL_NUT` call reaches it with SPEED3 set. With `SEFLG_SPEED` the
  save area it shares with the asteroids (slot `SE_NPLANETS`) was copied
  into x[3..5], so x[4] and x[5] were whatever the last asteroid left
  there. The other special bodies, the nodes, apsides, interpolated apsides
  and fictitious bodies, have a position's layout, and give the same speed
  with SPEED and SPEED3 (checked on Swiss and Moshier; valgrind clean).
- `jplplan()` had `} if (ipli == SEI_SUNBARY)` where an `else if` belongs:
  the Earth, copied into the result, went through the "other planet"
  branch too, which re-read the same Earth (xp = pdp->x, or one more
  `swi_pleph()` for the same state without `do_save`). Bit for bit the
  same answers, JPL Sun, Moon, Mars, Pluto, Earth, four centres, and
  `swe_calc_pctr()`.
- `-Og` could not see that the light-time loop in `app_pos_etc_plan()` runs
  at least once (niter is 1, or 0 from j = 0) and warned that `dt` may be
  used uninitialised at the bug 63 and 78 lines.

- Part 2: `SE_ECL_NUT` with `SEFLG_EQUATORIAL` or `SEFLG_XYZ` read slots 12.. or
  6.. of that same asteroid save area, where it writes only the first four:
  after an equatorial Cupido call, 2460000.5 gave x[0] = 44.7721 (Cupido's
  right ascension) for the true obliquity 23.4384, x[3] = 0.0209 for
  0.00215, and with XYZ other bodies' coordinates; on a fresh context
  zeros. The manual documents nothing for them. Now the four angles, x[4] =
  x[5] = 0, whatever coordinate flags are given.

**Fix, as done here:** `SE_ECL_NUT` takes no SPEED3 pass and copies no speed
slots: with any speed flag it answers as with none, x[0..3] the true and
mean obliquity, the nutation in longitude and in obliquity, x[4] = x[5] = 0
(upstream documents nothing for the speed slots). The `else`, and `dt = 0`
at the top of the function; the second changes no result.

G89 `tests/eclnutspeed.c`: part 2, the same four angles with EQUATORIAL, XYZ and both, after a Cupido call, 3 epochs, Swiss and Moshier, with and without speed flags: 324 slots, 216 fail upstream and before. Part 1: `SE_ECL_NUT` with no flag, SPEED and SPEED3, six
epochs -2000..2800, Swiss and Moshier, degrees and radians, every slot bit
for bit equal, x[4] = x[5] = 0, x[3] = x[0] - x[1]: 360 slots, none differ;
valgrind 0 errors. The upstream build and ts20 before this fix fail 118 of
360 (96 valgrind errors in the gate). G24 `ecl_nut_speed3_off`: the largest
slot difference with SPEED3 or EQUATORIAL after Cupido, 0 shipped, 78.5 deg compat. G53, G76, G83
and G24 pass; `-Og -Wall -Wextra -Werror` builds every file, default and
compat.

## 88. `swe_calc_orbel()` answers from caches of another instant and save areas of another ephemeris, and nothing on a context's first call

**Severity: Correctness (this fork's function: zeros or garbage with no error as a context's first call; 1.8e-5 deg/day on a latitude speed of date and the frame of date of another instant; 0.009 AU and 2.4e-3 deg/day on Moshier after a Swiss or JPL call)**
**Where:** `sweph.c`: `swe_calc_orbel_r()`

Found as entry 81's part 3 left it. `swe_calc_orbel_r()` set up what
swecalc() sets up for any body except two things.

1. **The caches of date.** swecalc() calls `swi_check_ecliptic()` and
   `swi_check_nutation()` for tjd (and sets the default ayanamsa for
   SEFLG_SIDEREAL); this function did not, and `app_pos_rest()` rotated with
   `ctx->oec`, `ctx->oec2000` and `ctx->nut` as the last `swe_calc()` left
   them. As a context's first call those were empty, sin eps = cos eps = 0:
   the ecliptic answer was zeros, XYZ (0.17, -0, 0), the return value the
   flags and no error (283 deg off a warm call, 4.6 AU in heliocentric
   J2000 XYZ, 82.5 deg sidereal). After a `swe_calc()` at another instant,
   the frame of date was that instant's: an orbit with a = 0.137 AU off its
   positions' rate by 8.7e-7 deg/day in longitude and 1.8e-5 in latitude,
   when each call follows a `swe_calc()` 1000 days away (the 8e-8 of entry
   81's note, after one call at J2000).
2. **The save areas across a change of ephemeris.** `swe_calc_r()` frees
   them when the ephemeris differs from the last call's; this function did
   not. Moshier has no barycentric Sun, the origin is the Sun, and
   SEI_SUNBARY is never written under it, so a Swiss or JPL Sun left there
   was added to the orbit: 0.009 AU on a SEFLG_TRUEPOS position, its
   velocity 2.4e-3 deg/day on the heliocentric speed and 5.6e-4 on the
   geocentric one (a = 0.137 AU).

Entry 81's 1.07e-6 deg/day on the heliocentric apparent J2000 speed is not
this function's: it was one date, 1800.0 (2378496.5, where sepl_12.se1
ends and sepl_18.se1 begins), with the probe's stencil across it; the speed
there is right against a stencil on either side (1.8e-11 over 2^-6..2^-8
day). (The fictitious bodies' own heliocentric apparent speed at exactly
that instant is 8.2e-7 deg/day off, at any step, Swiss only, and right
0.25 day either side: not fixed here. Fixed as entry 89: at exactly 1800.0
the speed is now the newer file's, as are the positions there, within
1.7e-9 deg/day of a one-sided difference over 2^-12 day on that side.)

**Fix, as done here:** after `plaus_iflag()`, the default ayanamsa for a
sidereal request, `swi_check_ecliptic()` and `swi_check_nutation()` for
tjd, and on a change of ephemeris the same `free_planets()` and file
closing `swe_calc_r()` does. A context's first call now answers as a warm
one, bit for bit (Swiss, Moshier, JPL, ecliptic, equator, XYZ, heliocentric
J2000, sidereal, topocentric with speed). Speeds against five-point
differences over 2^-7 day, nine dates 1800..2150, four orbits (a = 0.137,
0.387 and 5.2 AU about the Sun, 0.0068 about the Earth), each call after a
`swe_calc()` 1000 days away: geocentric <= 1.6e-9 deg/day on the files
(Moshier 5.1e-9, its Earth's own noise), heliocentric and barycentric <=
3.8e-11, topocentric 2.7e-9; the distance rates <= 4.6e-11 of the distance
per day. The orbit 0.0068 AU from the Earth first looked 2.9e-7 deg/day
off topocentrically (Waldemath through `swe_calc()` the same, the Moon
6.8e-7), as if the site's velocity were 1.3e-7 of itself off. That was the
reference, as under entry 79: the site turns at 6.3 rad/day, and the
five-point's truncation, (omega h)^4 / 30 of the diurnal parallax's rate,
is that much at 2^-7 day (7.2e-7, 4.3e-8 and 8.6e-9 deg/day on the Moon at
2^-7, 2^-8 and 2^-9), and below that the site read at a UT rounded to the
JD's ulp (4.7e-10 day) is the noise. Against a nine-point difference over
2^-8 day with delta t set to 2^-11 day (UT exact): the Moon, Selena and
Waldemath through `swe_calc()` and the Earth-centred orbit here, Swiss,
JPL and Moshier, ecliptic and equator of date, J2000, true positions:
<= 2.3e-9 deg/day on the files (Moshier 6.9e-9, 1.1e-8 on G87's orbit),
the distance rates <= 3.7e-11 of the distance per day (Moshier 1.4e-10);
and the site's velocity from `swi_get_observer()` is within 4.5e-11 of
itself of the slope of a degree-8 least-squares fit through 257 of its
places 2^-11 day apart, four latitudes, nine dates 1800..2150 (1.2e-8
with the default delta t: the rounded UT's noise in the fit, which does
not follow the delta t's rate). Nothing to fix; G87's topocentric rows
take that reference and include the orbit.

G87 `tests/orbelcalc.c`: (1) eight first calls against the same call
warm, after a `swe_calc()` at another instant, equal to the bit; (2) the
speeds of the four orbits against five-point differences over 2^-7 day
(topocentric, nine-point over 2^-8 day with delta t 2^-11 day), Swiss,
Moshier and de200, geocentric, true, J2000, equator of date,
heliocentric, barycentric and topocentric (29 rows), within 5e-9 deg/day
(Moshier 2e-8). The build before the fix fails 20 of the 37 checks (all
eight first calls; twelve speed rows of date, 8.7e-7 / 1.8e-5 deg/day); the
SWE_UPSTREAM_COMPAT build fails 33. G24 `orbel_first_call_off`: the first
call's longitude against a warm one, 0 shipped, 76.8 deg before the fix
and compat. G24 `orbel_lt_flag_off` (entry 81 part 3) no longer warms the
context: it compares equatorial J2000 XYZ, which needs no obliquity, 0 AU
shipped, 3.1e-7 AU compat. G48, G62, G76, G77, G80 pass.

## 93. `swe_nod_aps()`'s aberration rate takes the Moshier Earth's acceleration from the gravitational jerk

**Severity: Correctness (1.7e-10 deg/day on Neptune's points' geocentric latitude speed, 3.1e-10 on its mean points' longitude speed; Moshier only)**
**Where:** `sweph.c`: `swi_observer_accel_lt()`

Entry 70 gave the planets' apparent points the rate of their aberration,
the central difference of A(x + s x', V + s a), a the observer's
acceleration from `swi_observer_accel_lt()`: V(teval) less the Earth's
velocity at t = teval - tau carried back by `earth_at_lt_sun()` with the
Sun's and the Moon's gravitational jerk, over tau. On Moshier the Earth
the series returns is not the gravitational one: its small monthly terms
are 1e-4 of a (entry 66, part 3), turning at 0.23 rad/day, so its jerk
differs from the gravitational one by 7e-9 AU/day^3 and tau a came out
3e-10 to 1e-9 AU/day^2 off the acceleration of the Earth's velocity, at the
points' light times of 0.03 to 0.17 day. Through the aberration (a/c)
that is 6e-12 rad/day. Measured on Moshier, Jupiter..Neptune, mean and
osculating points, geocentric, default flags, seven instants -3000..3000,
against five-point differences of the points' own positions over 2^-6 day:
latitude speed 3.1e-11 (Jupiter), 5.3e-11, 1.1e-10, 1.7e-10 deg/day
(Neptune), against 1.5e-12 or less under SEFLG_NOABERR; mean longitude
speed 3.3e-11 (Saturn) to 3.1e-10 (Neptune), against 5.8e-12. On JPL and
Swiss files, whose Earth is the gravitational one, the same case is
2e-11.

**Fix, as done here:** on Moshier `swi_observer_accel_lt()` takes a as the
Earth's acceleration at teval itself, the central difference of its
velocities at teval +- 2^-9 day (`main_planet_bary()`, no save), as entry 76
takes it for a star; the Moshier velocity is smooth since entry 85. The
positions' second difference that entry 66 part 3 uses for tau a past 0.3
day is not used: it carries the series' rounding and left 1.6e-10 deg/day
on the points' longitude speed. Measured after: mean longitude speed
<= 7.4e-12, latitude <= 9.1e-13 deg/day, osculating latitude <= 1.6e-12,
osculating longitude <= 3.9e-10 (the reference's own floor, 3.8e-10 under
SEFLG_NOABERR). JPL, Swiss, the Sun's and heliocentric points and the
positions are unchanged; the topocentric site's acceleration is added as
before. Two velocity reads (the Earth only) per point set with SEFLG_SPEED
on Moshier, none otherwise.

G92 `tests/nodapsmosh.c`: Jupiter..Neptune, mean and osculating, the four
points, longitude and latitude speeds against five-point differences, seven
instants; limits 3e-11 (mean longitude), 5e-10 (osculating longitude),
1e-11 deg/day (latitude). 13 of the 16 checks fail before the fix, none
after. G24 `nodaps_mosh_lat_off`: Neptune's mean ascending node's latitude
speed at 2000 BC, 6.6e-15 deg/day shipped, 1.7e-10 before the fix, 4.8e-6
compat (upstream's whole aberration rate). G69, G79, G63, G54, G65 and G84
pass.

## 87. The precession at exactly J2000 is the identity, which the Vondrak series is not

**Severity: Correctness (2.8e-10 deg on every place of date at exactly tjd = J2000, in latitude, declination and right ascension; 2.5e-9 deg on the true node's longitude; 1.7e-8 deg on Polaris's right ascension)**
**Where:** `swephlib.c`: `precess_3()`, `swi_precess_matrix_rate_eps()`

`precess_3()`, which takes the Vondrak 2011 series (the default, short-
and long-term) and Owen 1990, returned at once for `J == J2000`, as
`precess_1()` and `precess_2()` do: from J2000 to J2000 is no rotation.
For the IAU and Laskar-type polynomials that is exact (their matrices at
J2000 +- 2^-30 are the identity within 6e-16 rad). The Vondrak series is
not: its coefficients are published to 1e-6", and their sum at T = 0
leaves the equator pole of date at y = -4.848e-12 rad, the frame turned
1 uas about x (Owen 1990: 3.1e-7 rad, 0.063"). Every instant but J2000
itself carried that turn. So at exactly tjd = 2451545.0 every position
of date jumped against its neighbours; SEFLG_J2000 output, which is not
precessed, did not. This fork's `swi_precess_matrix_rate_eps()` (entry
49) copied the exception, sending J2000's speeds to the differencing
path.

Measured on ts20 before the fix, the residual of the value at J2000
against the polynomial through its six neighbours 2^-10 day apart, the
same at J2000 + 1/4 in brackets: the Sun's latitude 2.73e-10 deg
(2.1e-15), Mars's 1.47e-10, Pluto's 2.63e-10 (1.1e-14); equatorial,
Mercury's right ascension 5.3e-11 and declination 1.8e-10; the true
node's longitude 2.5e-9 (1.3e-13), the mean node's and mean apogee's
latitudes with SEFLG_J2000 2.3e-10 and 2.8e-10 (precessed from the
ecliptic of date); Mars's latitude speed 4.2e-11 deg/day (1.1e-13); the
same on de200 and Moshier, heliocentric, topocentric, sidereal and in
XYZ (the Sun's z 4.7e-12 AU); a star's right ascension 2.9e-11 deg,
Polaris's 1.7e-8; and in `swe_nod_aps()` and the ayanamsa. The nutation,
the obliquity (`oec2000` copied into `oec` at J2000 is the value
`calc_epsilon()` gives there) and the frame bias are smooth through
J2000. The defect is upstream's.

**Fix, as done here:** `precess_3()` takes J2000 like any other instant,
the matrix the series gives there; `swi_precess_matrix_rate_eps()` no
longer excludes it. Those that want the J2000 frame (`sip->t0 != J2000`,
`tequ != J2000`, SEFLG_J2000) skip the call. One caller did pass J2000
meaning "no rotation": `swi_get_ayanamsa_ex()` precesses with
`swi_precess(ctx, x, t0, 0, J2000_TO_J)`, and t0 is J2000 for
SE_SIDM_J2000 (mode 18), so that ayanamsa now carries the series' 1-uas
matrix at every date, up to 9.1e-13 deg at -999..3000 (found in the G1
re-baseline). Otherwise only J2000 itself changes; the bench checksum is identical, calc-swieph and calc-moseph unchanged
within noise. After: the residuals above 2.9e-15 (Sun), 6.7e-16 (Mars),
5.3e-15 (Pluto), 7.7e-12 (true node, its neighbourhood's 1.3e-11).

G86 `tests/j2000inst.c`: the Swiss files, de200 and Moshier; fifteen
bodies by fourteen flag sets, five stars by five, `swe_nod_aps()` for the
Moon, Mars and Jupiter by two methods, the ayanamsa for three modes: 744
rows, every position and speed at J2000 against the mean of J2000 +-
2^-30 and against the fit through its neighbours, each within three
times its largest value at twelve instants J2000 + c/16 plus a floor:
largest 0.61 of the tolerance (Mercury's topocentric latitude speed fit,
rounding). The upstream build fails 549 rows, ts20 before the fix 526,
this build with SWE_UPSTREAM_COMPAT 549. G24 `j2000_lat_off`: the Sun's
latitude at J2000 against the mean of J2000 +- 2^-30, 6.5e-15 deg
shipped, 2.73e-10 compat. G70, G57, G68, G41 pass.

## 91. The ayanamsa's speed is a backward difference over 0.001 day

**Severity: Correctness (every sidereal speed off the rate by 1.3e-11 to 9e-11 deg/day in the precession modes, 1e-9 in the star modes, 2.1e-6 for True Revati near the Sun; 3.6e5 deg/day for 0.001 day where the ayanamsa crosses 0)**
**Where:** `sweph.c`: `swi_get_ayanamsa_with_speed()` (now through `get_ayanamsa()`, `swi_get_ayanamsa_ex()`'s body with the rate), and through it every sidereal speed of the traditional algorithm (`app_pos_rest()`, the mean and osculating nodes and apsides, the fixed stars, `swe_nod_aps()`)

`swi_get_ayanamsa_with_speed()` took the ayanamsa's rate as
(a(t) - a(t - 0.001)) / 0.001. Measured against five-point differences
of the ayanamsa's own values at binary-fraction instants, all modes,
1900-2000, the Swiss files:

- **The precession modes** (all but the twelve taken from a star): a(t)
  carries its own rounding, 1e-14 to 7e-14 deg (the precession's matrices
  and the polar angle; steps of 1e-14 on a 1e-4 day grid), which a
  thousandth of a day turns into 1.3e-11 to 8.9e-11 deg/day (Lahiri
  6.3e-11, Fagan/Bradley 7.9e-11). The reference converges to the same
  values over 2^-6 to 1 day: it is the speed's, not the reference's. It is
  the 5.3-5.7e-11 on every sidereal row of G85.
- **The star modes** (True Citra, Revati, Pushya, Sheoran, Mula, the
  Galactic Centre modes, the galactic-equator ones): a(t) is a star's
  longitude with aberration and deflection, and the backward difference is
  off by half its curvature times the step: 9.0e-10 to 1.1e-9 deg/day, and
  2.1e-6 for True Revati at JD 2444704.3086 (1981-04-08), zeta Psc 0.32 deg
  from the Sun. The galactic-equator modes (the pole's place without
  aberration) 2.8-3.7e-11.
- **Across 0 deg** (Lahiri, JD 1825399.07, 285 AD) the difference took the
  360 deg jump: -3.6e5 deg/day for 0.001 day after the crossing.

SEFLG_NONUT makes no difference: the ayanamsa is always the mean one, and
SEFLG_SIDEREAL implies SEFLG_NONUT. J2000, J1900, B1950 and Mardyks
(forced onto the ecliptic of t0) and SE_SIDBIT_ECL_T0 / SSY_PLANE never
take this speed.

**Fix, as done here:** the star modes take the star's own speed
(`swe_fixstar_r()` with SEFLG_SPEED; its positions are bit-identical with
or without it, 400 dates, four flag sets; its speed within 5.8e-12 of
its rate), the Wilhelm mode the derivative of its projection, tan mc =
tan ra / cos eps, with the mean obliquity's rate; the precession modes
the derivative itself, the point's velocity carried through the same
rotations: the vernal point of date, at rest there, moves in J2000 at
dP^T/dt (`swi_precess_speed()`, the Vondrak series' M and dM/dt in one
pass), turns with it into t0's fixed ecliptic, and its longitude's rate is
(x vy - y vx) / (x^2 + y^2); with SE_SIDBIT_ECL_DATE the t0 point, at rest
in J2000, moves at dP/dt into the date's equator and with the mean
obliquity's turning (`swi_ecl_frame_speed()`) into its ecliptic. The
values are computed as before, bit for bit; the rate needs no difference,
so there is nothing to wrap at 0 deg. A precession mode reads two
precessions fewer than upstream's two ayanamsas, a star mode one star
instead of two: a Lahiri sidereal planet 9.2 -> 8.3 us, a True Citra one
14.9 -> 10.5 (calc-swieph and calc-moseph are tropical, untouched).
Upstream's code is under `SWE_UPSTREAM_COMPAT`. After, at the same dates:
the precession modes within 5.6e-15 deg/day of a five-point difference
over 8 days (6.3e-15 with the SE1.64, SE1.70 and SE2.00 precession
models, whose rates `swi_precess_speed()` takes over +-1 day), the star
modes <= 3.1e-12 over 2^-6 day (9.3e-13 over 2^-4), True Revati at the
conjunction 3.7e-11 against a reference over 2^-8 day (whose own
truncation is 8.9e-9 over 2^-6), the crossing 2.1e-14. Sidereal positions
do not move: 2736 places, with and without SEFLG_SPEED (19 modes and SIDBITs, 12 bodies, six dates,
Swiss and Moshier) are bit-identical but Neptune's and Pluto's at JD
2378496.637 in the star modes, 6.7e-10 deg, the Swiss files' join at
1800.0, where which file answers depends on the calls before (entry 84's
note); speeds move by the fix, up to 8.4e-10 deg/day. G85's sidereal rows
go from 5.3-5.7e-11 to 2.1-5.0e-12, as its other frames, and its sidereal
limit from 2e-10 to 2e-11; the build before the fix fails those 15 rows.

G90 `tests/ayaspeed.c`: a = the Sun's tropical longitude without nutation
less its sidereal one, its speed the difference of the speeds, against
five-point differences: 43 predefined modes, SE_SIDM_USER (t0 TT and UT),
Lahiri and USER with SE_SIDBIT_ECL_DATE, with and without SEFLG_NONUT,
Swiss files and Moshier, nine dates (-2000..2900, 1800..2200 for the
star modes); the precession modes over 1 day within 2e-12 deg/day
(measured <= 1.1e-13, the reference's noise), the star modes over 2^-6 day within 2e-11
(<= 6.3e-12), True Revati at the conjunction within 2e-10 (3.7e-11),
Lahiri 2^-12 day past 0 deg within 2e-12 (2.1e-14): 1694 checks. The
build before the fix fails 1506 (precession modes 2.9e-11 to 7.9e-11,
star modes 3.9e-11 to 9.8e-10, the conjunction 2.1e-6, the crossing 3.6e5;
only the modern galactic pole's two modes pass, 1.6e-11); the upstream
build fails 1546 (its Moshier stars 5.1e-7, their own). G24
`aya_star_rate_off`: True Revati at the conjunction, 3.7e-11 deg/day
shipped, 2.1e-6 compat.

Measured with it and not this entry's: the topocentric aberration
residual of G51, 3.1-9.6e-10 deg/day against 2.5e-11 geocentric, is bug
79's site acceleration (`observer_accel()` over +-2^-6 day divided by
sin(omega h) / (omega h)): applied in scratch it falls to 1.6-3.7e-11 at
all four latitudes.
The Swiss-file Mercury and Venus position jitter of 1e-11 to 2e-11 deg on
1e-4 day scales is not there: at binary-fraction instants 2^-13 day apart
their geocentric longitudes' noise (fourth differences / sqrt(70)) is
2.5e-14 to 6e-14 deg rms, 1.7e-13 at the most (2200), a few ulps: default
flags, SEFLG_TRUEPOS, NOGDEFL, NOABERR, heliocentric, barycentric and
on Moshier at J2000, the default flags at six more epochs -1000..2900,
and the same at bf04301's parent; five-point speed residuals over ten
epochs -2900..2900 go as 1/h from that (1.8e-8 deg/day over 2^-16).
Over 1e-4 day steps that are not binary the noise is 1.9e-10 to 2.4e-10
deg rms: the instants' own rounding (ulp 4.7e-10 day at JD 2.45e6 times
1.2-1.4 deg/day), which five-point differences over 1e-4 day turn into a
few 1e-6 deg/day; the measurement's, not the library's.

## 92. The fixed-star memos and catalogue outlive a new path, a path change resets a manual tidal acceleration, and the topocentric observer outlives an ephemeris switch

**Severity: Correctness (returned values: a star's magnitude and distance from another catalogue, 9.13 s of delta t in AD 1000, 4.4" on the topocentric Moon)**
**Where:** `sweph.c`: `swi_fixstar_forget()`, `swi_close_keep_topo_etc()`, `ctx_release()`, `swe_fixstar_r()`, `swe_fixstar2_r()`, `swe_fixstar_mag_r()`, `swe_fixstar2_mag_r()`, `swi_get_observer()` and the five places that reuse `topd`; `sweconfig.c`: `swi_config_apply()`; `sweph.h`: `struct topo_data`, `SWI_TOPD_KEY()`

Three answers that depended on what was asked before them, each measured
against the same request in a fresh process:

- **The fixed-star memos.** `swe_fixstar()` and `swe_fixstar_mag()` keep
  their last star's record (`sp.fs1`, `sp.fs1mag`), `swe_fixstar2()` and
  `swe_fixstar2_mag()` their last star (`sp.fs2`, `sp.fs2mag`), and
  `swe_fixstar2()` reads the whole catalogue into memory once
  (`fixed_stars`). None was emptied by a path change, and the memos not by
  `swe_close()` either (it frees the catalogue): with a second
  `sefstars.txt` giving Sirius -1.00 mag and a parallax of 300 mas, Sirius
  after `swe_close()` and the path to it stayed at -1.46 and 2.64 pc
  (543932 AU) for -1.00 and 3.33 pc (687549 AU) in all four functions, and
  at the other catalogue's values after the path back; `swe_fixstar2()`
  answered from the old catalogue after a path change whatever star was
  asked, until `swe_close()`. And on the shared configuration a thread
  adopting another thread's path kept its star file open on the old
  directory and its catalogue in memory, and looked a star up before
  adopting at all (the four functions synced only inside the calculation,
  after the lookup, and the magnitudes never): every answer on it after
  each path B was catalogue A's (Aldebaran 0.86 mag and 4214646 AU for
  0.50 and 5156619). Entry 82 found the first of these.
- **A manual tidal acceleration** was reset to automatic by every path
  change -- `swe_set_ephe_path()`, `swe_set_jpl_file()`, and the implicit
  path the first calculation sets when the files are named by
  `SE_EPHE_PATH` -- in `swi_close_keep_topo_etc()`. So
  `swe_set_tid_acc(SE_TIDAL_DE406)` followed by `swe_calc_ut()` (files by
  `SE_EPHE_PATH`), or by another `swe_set_ephe_path()`, left -25.936, the
  file's, and `swe_deltat()` in AD 1000 1474.79 s for 1465.66; followed by
  `swe_set_jpl_file("de200.eph")`, -23.8946 and 1305.37 s. The manual
  (10.3): "Once the function swe_set_tid_acc() has been used, the
  automatic setting of tidal acceleration is blocked. In order to unblock
  it again, call swe_set_tid_acc(SE_TIDAL_AUTOMATIC)." `swe_close()`
  releases everything, and after it "no Swiss Ephemeris functions should
  be used unless you call swe_set_ephe_path() again" (2.2); it returns the
  setting to automatic, as upstream's does, and that is kept. Upstream's
  too.
- **The topocentric observer** (`topd`) is reused when its instant equals
  the Earth's, but it is placed by the delta t of the request's ephemeris
  (its tidal acceleration) and, in this fork, framed by its DE number
  (entry 27's J2000 -> ICRS bias, for DE403 and later): a topocentric
  request at the instant of one on another ephemeris took that one's site,
  with no setter between them. De200 after Swiss (or Moshier) at 2000 Jan
  1 12h UT: the Moon 3.7e-8 deg in right ascension, 3.9e-8 in azimuth and
  1e-8 in altitude (`swe_azalt()`), 4.1e-7 deg/day in its latitude speed;
  this is the 3.9e-8 G81 saw. At TT 1600 Jan 1, where the ephemerides'
  delta t differ, `swe_calc()` Swiss after de200 4.4" (0.00122 deg),
  Moshier after Swiss 0.77", and at 1000 5.4" (upstream, without the
  bias: 4.4" and 31"). `swe_calc_ut()` at those dates misses the reuse
  (the TT differ with the delta t). The delta t part is upstream's.

Swept for the rest, each against a fresh process, on Swiss, Moshier and
de200 after each other at 2000, 1600 and 1000: `swe_calc()` and
`swe_calc_ut()` (geocentric, topocentric, with speeds), `swe_azalt()` and
`swe_houses_ex()`; nothing else moves. The nutation and obliquity caches
are keyed on the instant and the JPL Horizons flags, which is complete
across ephemerides; the save areas carry the ephemeris; delta t is
recomputed per request with the ephemeris's tidal acceleration.

**Fix, as done here:** `swi_fixstar_forget()` empties the four memos,
frees the catalogue and closes the star file; `swe_close()`, every path
change (`swi_close_keep_topo_etc()`) and an adopted path
(`swi_config_apply()`) call it, where `swi_hel_forget()` runs. The four
star functions sync first (`swi_init_swed_if_start()`), so a thread adopts
a new path before its lookup. `swi_close_keep_topo_etc()` returns the tidal
acceleration to automatic only when it is not manual. The observer is
keyed on its instant and `SWI_TOPD_KEY()` of the flags it was built for
(the ephemeris and the Horizons flags), and emptied by a path change, which
can bring files of another DE number. Under `SWE_UPSTREAM_COMPAT` all
three are upstream's.

The star functions as the very first library call, with the files named
by `SE_EPHE_PATH`, still failed after this fix (the sync gave the compiled
default path, not the variable's); entry 98 fixes that.

G91 `tests/ephswitch.c`: the four star functions on Sirius, Aldebaran and
Sirius again through path A, `swe_close()` and B, A, B, `swe_close()` and
A (two catalogues), and on a second thread while the first sets the paths;
seven tidal-acceleration scenarios against the setting alone or none; and
140 topocentric Moon, `swe_azalt()` and `swe_houses_ex()` requests at
2000, 1600 and 1000 on each ephemeris right after a topocentric Moon on
each other one at the same instant; within 1e-10 of a fresh process: 156
pass. Before the fix 34 fail (stars 6 of 9, tidal 4 of 7, ephem 24 of
140); the compat build 18 (ephem 8: the delta t cases); the upstream build
(legacy-master) 18 (stars 8, its second thread finding no path at all,
tidal 4, ephem 6). G24 `tid_acc_path_off`: `swe_deltat()` in AD 1000
after a manual DE406 and a second path, 0 s shipped, 9.13 s compat. G24,
G81, G82, G83, G20, G70, G2 and the context gate pass.

## 96. An Earth-centred fictitious body's light-time place on Moshier jitters with the second read of the Earth

**Severity: Correctness (3e-15 AU rms of jitter in the place: 2.7e-11 deg on Waldemath, 3e-12 on Selena, the same on a `swe_calc_orbel()` orbit about the Earth; 3.4e-8 deg/day in a nine-point difference over 2^-10 day; Moshier only)**
**Where:** `sweph.c`: `app_pos_etc_plan_osc()`, `app_pos_etc_orbel()`

An Earth-centred set (seorbel.txt's "geo": Selena, Waldemath; or
`swe_calc_orbel()` about the Earth) is seen from the Earth at teval, with
its focus at the Earth at the emission instant t = teval - tau. Its
geocentric place therefore holds E(t) - E(teval), a few 1e-7 AU, formed
as the difference of two reads of the Earth, 1 AU vectors 4e-5 day
apart. A Moshier read is off a smooth curve by its series' rounding, and
the two reads do not share it. Found as the Moshier rows of a nine-point
speed check (entry 88's note) whose residual grew as the step shrank,
3.2e-9, 2.6e-8 and 3.4e-8 deg/day on Waldemath at 2^-8, 2^-9 and 2^-10
day, where the Moon and the Sun on Moshier and the same bodies on the
files stay flat: a jitter in the place, not a speed error. Measured as
the largest fourth difference over 2^-10 day of the geocentric
equatorial J2000 XYZ place, 64 instants at each of nine dates 1800..2150:

- Moshier, apparent or without aberration: Waldemath 8.9e-14 AU (1.5e-11
  of its distance), the Earth-centred orbit (a = 0.00684 AU) 1.0e-13,
  Selena 8.0e-14 (1.5e-12), about 3e-15 AU rms in the place;
- Moshier, SEFLG_TRUEPOS (no light time, E(teval) cancels): 6.7e-16,
  1.3e-15 and 0;
- the Swiss files 7.2e-15, 8.8e-15 and 7.8e-15 (their reads' rounding,
  an ulp of 1 AU), de200 3.6e-15 (5.3e-13 of Waldemath's distance).

The defect is upstream's (the compat build: 7.4e-13 AU on Waldemath, its
light time otherwise unsolved too).

**Fix, as done here:** below a light time of 2^-10 day (a body within
0.17 AU) on Moshier, the Earth at t is the Earth at teval carried back
along its own motion, E - tau V + tau^2 A / 2, A the series' acceleration
(`swi_mosh_earth_accel()`, bug 66 part 3's second difference over
+-2^-8 day, taken once a call). E(teval) then cancels exactly against the
observer and only the smooth -tau V + tau^2 A / 2 is left. Left out is
tau^3 / 6 of the jerk, 6e-6 AU/day^3: 2e-16 AU at the switch, 3e-17 on
Selena, 4e-20 on Waldemath. Only the place is replaced; the velocity is
the read's. Cost: two Moshier Earth evaluations a call, for such bodies
only. After, the same measure: Waldemath 3.0e-15 AU, the orbit 1.2e-15,
Selena 1.7e-15, less than the files' own. The nine-point speed residual
at 2^-10 day: 1.4e-9 deg/day on Waldemath (3.4e-8 before), 3.5e-10 on
Selena (6.3e-9), 1.7e-9 on the orbit (7.2e-8); the Swiss files 3.3e-9,
1.1e-9 and 4.4e-9. G87's Moshier geocentric rows fall from 5.1e-9 to
2.5e-10 deg/day. Swiss and JPL are unchanged to the bit.

G95 `tests/geoltearth.c`: Moshier, Waldemath and Selena through
`swe_calc()` and the Earth-centred orbit through `swe_calc_orbel()`, nine
dates 1800..2150: (a) the fourth difference above, apparent and without
aberration, within 2e-14 AU; (b) the longitude, latitude and distance
speeds (per unit distance) in the ecliptic of date against a nine-point
difference of the positions over 2^-10 day, within 5e-9 deg/day.
Topocentric places are left out of (a): the site's turning puts 4e-14 AU
of real curvature into the fourth difference. The build before the fix
fails all nine checks, the SWE_UPSTREAM_COMPAT build too. G80's
fixed-point rows (3) on Moshier compared against a reference that read
the Earth twice; they now take its difference E(t - tau) - E(t) from a
degree-8 least-squares fit through 257 reads 2^-11 day apart (1e-19 AU of
the reads' noise over a light time): Waldemath 2.4e-12 deg, Selena 1.0e-12
off the fixed point (before the fix 6.3e-11 and 9.7e-12, which G80 now
fails). G24 `mosh_geo_lt_jitter`: Waldemath's fourth difference at
2026-10-24, 2.5e-15 AU shipped, 1.3e-14 before the fix, 1.2e-13 compat.
G59, G64, G80, G87 pass.

**Not fixed, below the precision floor (1e-9 deg, 1e-8 deg/day):** the
same two-read difference on the Swiss files and de200. Each file read is
off by about an ulp of 1 AU, and the largest fourth difference over 2^-10
day (geocentric J2000 XYZ, nine dates 1800..2150) is 7.2e-15 AU on
Waldemath, 8.8e-15 on Selena and 7.8e-15 on the Earth-centred orbit on the
files, 2.1-3.5e-15 on de200. The Moon's place on the files (the geocentric
Moon read at t, plus the Earth read at t, less the Earth at teval) shows
6.7e-15 AU (2.5e-12 of its distance; 2.5e-13 true), de200 2.8e-15. The
nine-point speed residuals over 2^-10 day are 3.3e-9 deg/day on Waldemath
and 4.4e-9 on the orbit. The same Taylor step, with the Earth's
acceleration from the Sun's and the Moon's pull, takes all of these down
to 1.3-3.1e-15 AU and moves no position by more than 1.6e-15 AU (and no
far body at all). It was measured, then left out under the floor.

## 89. At the Swiss files' 600-year joins an open file answers past its nominal end, so the answer depends on the file an earlier call left open

**Severity: Correctness (call-order dependence: the barycentric Sun 47.6 mas at 1800.0 + 0.1 day after a call 1.5 days before, 1.03 arcsec at -7800; the Moon 220 mas across a join's light time; Juno 37 km; Vulcan's heliocentric speed at 1800.0 8.2e-7 deg/day)**
**Where:** `sweph.c`: `sweph()`, the test that keeps the open file

Found by entry 84's sweep and left there. Every Swiss file's data runs past
its nominal 600 years: sepl 2-89 days (sepl_12 to JD 2378575.7, 79 days
past 1800.0), every seas file 350-361 days (seas_12 to 2378857.5), semo
1-30 days, and the semo files also start 1-27 days before their nominal
start (the sepl and seas files start exactly on it). `sweph()` kept an open
file for any instant inside its data, and a fresh context opens the file
`swi_gen_filename()` names for the instant. So just across a join a context
warm from the other side answered from that side's file and a fresh one
from the instant's own: at JD 2378496.6 after a call 1.5 days before, the
barycentric Sun 47.6 mas, Juno 8.7 mas, Chiron 0.67 mas geocentric, speeds
up to 4.7e-3 mas/day where the light time crosses the join. Over the 49
joins -12600..16800: the barycentric Sun up to 1.03 arcsec (-7800), the
geocentric Moon 220 mas (J + 2^-17 day, its light time across the join),
Mercury 2.7 mas, Juno 37 km. At 1800.0 itself a fictitious body's
heliocentric speed is the older file's in a context that holds it and the
newer's in a fresh one, Vulcan 8.2e-7 deg/day apart -- the "8.2e-7 at any
step" of entry 88's note; on Moshier and JPL there is no join. Within one
calculation the light time's read at t - tau, before the join, switched
the open file to the older one, and every later read at t answered from
it, so even a fresh context's answer mixed files by its own read order;
those move a little too (the Moon 0.26 mas and 1.5e-8 deg/day at 1800.0 +
2^-17 day, the planets' and Juno's speeds up to 4.8e-13 deg/day).
Entry F11's neighbour-file fix (REVIEW) covers only a missing file.

**Fix, as done here:** an open sepl, semo or seas file answers only inside
the nominal span it was opened for -- the 600-year bucket of the instant
that opened it, `[tnom0, tnom1)` on `struct file_data`, found from the
calendar and moved to the double where `swi_gen_filename()`'s name changes,
so the two cannot disagree at a join by a rounding. Every read, the light
time's t - tau included, comes from the file of its own instant, which is a
fresh context's choice: across a join's light time the read at t is the
newer file's and the read at t - tau the older's. One file for both is not
possible for the planets and asteroids, whose newer files start exactly at
the join; the position therefore steps by the files' difference at J and
again at J + tau (Vulcan 0.4e-6 deg, Mars 0.1 mas, Juno and the Moon some 4
mas), as a fresh context's did. A missing bucket file is still answered by
its neighbour (F11), for that bucket's span only. The file replaced is
kept open as a spare per file kind, with its bodies' file data (the part of
`plan_data` before `teval`: constants, reference ellipse, loaded segment),
and an instant it answers swaps it back: inside a join's light time a call
costs 4.1 us against 4.0 elsewhere, where reopening cost 33 (ten planets
with SEFLG_SPEED, 1800.0 + 2^-7); `free_planets()` and
`swi_close_ephe_files()` drop the spares. calc-swieph unchanged within
noise (best of 9: 0.568 against 0.571 s); calc-moseph does not reach the
code. The files' general constants are the same in all 150 files, and each
body's coefficient count the same in every file, so the last file read
setting `gcdat` and a reused segment buffer depend on no order. Upstream's
code is under `SWE_UPSTREAM_COMPAT`.

Not this entry's, and left: with `semo_12.se1` missing, at JD 2159377.5
(1200.0 + 20 days, past `semo_06`'s data) F11 opens `semo_06`, its range
check refuses the instant and the file stays open, so `swemoon()`'s
"no moon file, use Moshier" test does not fire: a fresh context returns
the Moon as NaN with flags 0x100 and no error, a warm one 26.44179 deg,
where upstream answers 349.29034 from Moshier; the Sun there 6.03 arcsec
apart fresh and warm. The same before and after this fix; fixed as entry
100.

G88 `tests/filejoin.c`, bit for bit, at each of the 49 joins: (1) at J - 4,
J - 2^-7, J, J + 2^-17, J + 2^-7, J + 2^-4, J + 2, J + 20 and J + 200 the
Sun, Moon, Mercury, Mars, Jupiter, Neptune, Pluto, Chiron, Ceres, Juno,
Cupido and Vulcan, geocentric, barycentric, heliocentric and J2000 XYZ
true, with speed, in a context warmed 1.5 days across the join by the same
call or by Mercury, the Moon and Ceres, against fresh; (2) one context
stepping 2^-5 day across every fourth join, both ways, the barycentric Sun,
Moon, Jupiter, Pluto, Juno and heliocentric Vulcan, against fresh; every
coordinate finite. 42684 rows, none differ. ts20 before the fix fails
30538 (29346 and 1192), the upstream build 30958 (29766 and 1192). G24
`file_join_off`: the barycentric Sun at JD 2378496.6 after a call 1.5 days
before against alone, 0 shipped, 47.4 mas compat. G24, G62, G70 and G83
pass; G88 also under ASan and UBSan.

Also gone with it: at exactly JD 2378496.5 a context's first apparent
Swiss Moon (J2000 ICRS XYZ, no aberration or deflection) was 1.58e-9 AU
(0.12") from the same call made warm, the emission-instant Earth read from
the other file; the two now agree bit for bit (found measuring entry 96).

## 98. A star function as the first call ignores `SE_EPHE_PATH`, and a path change throws away the astronomical models

**Severity: Correctness (a failed call; 18.27 s of delta t in AD 1000, 2.39e-6 deg of nutation, 0.00056 deg of ayanamsa)**
**Where:** `sweph.c`: `fixstar_first_use()` (`swe_fixstar_r()`, `swe_fixstar2_r()`, `swe_fixstar_mag_r()`, `swe_fixstar2_mag_r()`), `swi_close_keep_topo_etc()`

- **The star functions as the first call.** `swe_fixstar()`,
  `swe_fixstar2()`, `swe_fixstar_mag()` and `swe_fixstar2_mag()` read
  `sefstars.txt` along the ephemeris path but never name one, where
  `swe_calc()` on its first use calls `swe_set_ephe_path(NULL)`, which
  reads `SE_EPHE_PATH`. So as the very first library call, with the files
  named by the variable alone, each failed for Sirius: "SwissEph file
  'sefstars.txt' not found in PATH ''" (upstream; after entry 92's sync,
  the compiled default `.:/users/ephe2/:/users/ephe/`), and
  `swe_fixstar2()` "could not find star name sirius". The same request
  after `swe_set_ephe_path()` works. Upstream's too.
- **A path change zeroed the models.** `swi_close_keep_topo_etc()` --
  run by every `swe_set_ephe_path()` and `swe_set_jpl_file()`, and by the
  implicit path of a first calculation with the files named by
  `SE_EPHE_PATH` -- carried `swe_close()`'s
  `memset(astro_models, 0)`, so a path change put the defaults back over
  what `swe_set_astro_models()` had set, and over the precession models
  `swe_set_sid_mode(..., SE_SIDBIT_PREC_ORIG)` sets. With
  `swe_set_astro_models("1,0,0,1")` (delta t Stephenson & Morrison 1984,
  nutation IAU 1980), a second path, `swe_set_jpl_file()`, or the setting
  made first and the files by `SE_EPHE_PATH`, left the nutation in
  longitude at J2000 at -0.0038700008 for -0.0038676070 deg (2.39e-6) and
  delta t in AD 1000 at 1474.79 s for 1456.52 (18.27); Lahiri with
  `SE_SIDBIT_PREC_ORIG` after a second path gave 9.91733 for 9.91677 deg
  in AD 1000 (0.00056). Upstream's too, the same numbers (its
  `swe_set_jpl_file()` case 6.98 s, entry 92's tidal reset mixed in). The
  models are the caller's settings, not anything read from the files, and
  the angle caches they key are emptied by the path change anyway; nothing
  in the manual ties them to the path. Entry 83 recorded upstream losing
  `swe_set_astro_models()` set as the first call, and this fork keeping
  it: it kept it after `swe_set_ephe_path()`, not through the implicit
  path a first calculation sets from `SE_EPHE_PATH`.
  On the shared configuration the master kept the models while the
  context that changed its path lost them, so threads disagreed.

**Fix, as done here:** the four star functions call `fixstar_first_use()`:
initialise and sync (entry 92), then `swe_set_ephe_path(NULL)` on
`swe_calc()`'s condition (no path set, no JPL file open), under
`SWI_CFG_LOCAL`. `swi_close_keep_topo_etc()` no longer zeroes the models;
`swe_close()`, which releases everything, still does. Under
`SWE_UPSTREAM_COMPAT` both are upstream's.

G97 `tests/pathkeep.c`: each star function on Sirius as the first call
with the files by `SE_EPHE_PATH` against the same after a path (return
code, values, serr); `swe_set_astro_models("1,0,0,1")` then a second path,
`swe_set_jpl_file()`, or set first with the files by `SE_EPHE_PATH`,
against the setting after one path; then `swe_close()` and a path against
no setting; Lahiri with `SE_SIDBIT_PREC_ORIG` and a second path against
one path; within 1e-12, each setting seen against none: 9 pass. Before the
fix 8 fail, the compat build the same 8, the upstream build (legacy-master)
the same 8. G24 `models_path_off`: delta t in AD 1000 after the models and
a second path, 0 s shipped, 18.27 s compat. G81 (`tests/helmemo.c`) now
holds every answer to 1e-9, bit-identical after entry 92; at 1e-9 the
library before entry 92 fails its "Swiss again" (3.86e-8 deg). G24, G81,
G91, G2 and the context gate pass.

## 101. `swe_get_tid_acc()` answers -25.80 after `SE_TIDAL_AUTOMATIC` and 0 before any initialisation, and the shared configuration copies an automatic tidal acceleration

**Severity: Correctness (returned value: a tidal acceleration of 0, or 0.136 arcsec/cty^2 off; and a gate that could not see entry 98)**
**Where:** `swephlib.c`: `swe_set_tid_acc_r()`, `swe_get_tid_acc_r()`; `sweconfig.c`: `swi_config_apply()`; `tests/firstset.c` (G82)

The tidal acceleration a context holds is, when automatic, the one the
last delta t resolved: `swe_deltat()` and `swe_deltat_ex()` resolve it
afresh on every call (`calc_deltat()` -> `swi_set_tid_acc()`), so they
were right everywhere measured -- straight after `swe_set_ephe_path()`,
after `swe_set_jpl_file()`, after `SE_TIDAL_AUTOMATIC`, with no
calculation between (AD 1000: 1474.79 s Swiss, 1305.37 s de200,
1445.25 s Moshier, as after a calculation). `swe_get_tid_acc()` only
reads the field, and the field was wrong in three ways:

- `swe_set_tid_acc(SE_TIDAL_AUTOMATIC)` stored `SE_TIDAL_DEFAULT`
  (-25.80), which only the next delta t replaced. Straight after it,
  following a manual DE406, `swe_get_tid_acc()` answered -25.80 where a
  fresh `swe_set_ephe_path()` answers the moon file's -25.936; with the
  de200 file open -25.80 for -23.8946; after a Moshier calculation -25.80
  for the -25.936 the next delta t takes. Upstream's too (and upstream's
  `swe_deltat()` after the Moshier case is 1463.50 s for 1474.79).
- `swe_get_tid_acc()` did not initialise or sync: as the very first
  library call it answered 0.0, and on a second thread 0.0 for the value
  the first thread's path (-25.936) or `swe_set_tid_acc(SE_TIDAL_DE406)`
  (-25.826) had set. Upstream's too (its threads share nothing).
- This fork's: `swi_config_apply()` compared and copied the tidal
  acceleration whether manual or not, though `sweconfig.h` says an
  automatic one is each context's own, from its files. So the first sync
  after `swe_set_ephe_path()` put the master's -25.80 over the -25.936 the
  path had just derived (hidden while only a delta t, which re-resolves,
  read it), a context from `swe_ctx_new()` answered -25.80, and an
  adopted path left the old path's value.

And G82 (`tests/firstset.c`) compared its `SE_EPHE_PATH` orders (bare,
renv) with a reference warmed by `SE_EPHE_PATH` too, which lost a setting
the same way they did: entry 98's lost `swe_set_astro_models()` (1.17e-5
deg in the obliquity) passed. Its header said this fork "already keeps"
the models set first; it did only after `swe_set_ephe_path()`.

**Fix, as done here:** `SE_TIDAL_AUTOMATIC` resolves the value at once as
`swe_deltat()` would, for the ephemeris `swi_guess_ephe_flag()` names,
from the files open or the DE numbers published. `swe_get_tid_acc()`
initialises and syncs first. `swi_config_apply()` adopts only a manual
value or a change to or from one (back to automatic it resolves the
context's own), and resolves an automatic value from an adopted path's DE
numbers. These behind `#ifndef SWE_UPSTREAM_COMPAT`. G82's every order is
compared with the path-warmed reference; on the build before entry 98 it
now fails bare and renv for `swe_set_astro_models()`.

Noticed, not fixed: `swe_deltat()` takes its ephemeris from
`swi_guess_ephe_flag()` -- JPL when this context has the JPL file open --
so after `swe_set_jpl_file("de200.eph")` the main thread answers 1305.37 s
in AD 1000 and a second thread, which has not opened the file, 1474.79.
Fixed as entry 103.

G100 `tests/tidget.c`: `swe_get_tid_acc()` read first, then `swe_deltat()`
in AD 1000, then `swe_get_tid_acc()` again, in nine forked scenarios
(after a path, a path and the de200 file, automatic after DE406 with and
without it, automatic after Moshier, the first call, a second thread after
a path and after DE406, `swe_ctx_new()`); the reads agree within 1e-12 and
with the named reference: 9 pass. Before the fix 7 fail, the compat build
the same 7, the upstream build (legacy-master, without `swe_ctx_new()`) 6
of 8. G24 `tid_auto_get_off`: 0 shipped, 0.136 compat. G82 54 pass (2
fail on the build before entry 98). G24, G81, G91, G97, G2 and the context
gate pass.

## 97. The nakshatras of `swe_split_deg()` are shifted for SE_SIDM_USER, whose 255 passes a bit-mask test for Sheoran

**Severity: Correctness (every nakshatra split under a user-defined ayanamsa 3 deg 20' early: 10 deg read as Bharani 0 deg, not Ashvini 10)**
**Where:** `swephlib.c`: `split_deg_nakshatra()` (through `swe_split_deg()` with SE_SPLIT_DEG_NAKSHATRA); `sweph.c`: `get_ayanamsa()`'s missing-path warning

`split_deg_nakshatra()` moves the place by 3 deg 20' for the Sheoran
"Vedic" ayanamsa (its 0 Aries is 3 deg 20' of Ashvini), and tested the
mode as `(sid_mode & SE_SIDM_TRUE_SHEORAN) == SE_SIDM_TRUE_SHEORAN`, a bit
mask on a mode number. 39 is binary 100111; SE_SIDM_USER, 255, has every
one of those bits, with or without SIDBITs above it. Of the stored modes
(0..46, USER, either with SIDBITs) only Sheoran and USER pass. Measured
with `swe_split_deg(10, SE_SPLIT_DEG_NAKSHATRA)`: Fagan/Bradley and
Lahiri, with or without a SIDBIT, Ashvini 10 deg 00'; SE_SIDM_USER (255,
1279, 2303) Bharani 0 deg 00', as Sheoran.

The second comparison named in the report, the "call swe_set_ephe_path()"
warning in `get_ayanamsa()` testing `sip->sid_mode == SE_SIDM_TRUE_SHEORAN`
where the rest of its list tests the masked `sid_mode`, changes nothing:
`swe_set_sid_mode()` stores a star mode without its SIDBITs, so the two are
equal for every mode that can be set. Made the masked one for the list's
sake. Every other comparison of a mode number in the library takes
`sid_mode % SE_SIDBITS` first (`swe_set_sid_mode()`,
`get_aya_correction()`, `get_ayanamsa()`, `swe_get_ayanamsa_name()`), and
the SIDBIT tests (`swehouse.c`, `swecl.c`, `sweph.c`) test bits that are
bits. Not the library's, and left: `swetest.c` decides whether to pass
`-sidudef`'s t0 and value with `if (sid_mode & SE_SIDM_USER)`, true for
every mode but 0; harmless, since `swe_set_sid_mode()` takes t0 and the
value from its table for a predefined mode.

**Fix, as done here:** the mode itself, `sid_mode % SE_SIDBITS ==
SE_SIDM_TRUE_SHEORAN`; the warning's comparison masked too. Upstream's
code is under `SWE_UPSTREAM_COMPAT`. The Sheoran shift's constant,
3.33333333333333, leaves 5 deg at 8 deg 19' 59.99999" unrounded: below the
precision floor, left.

G96 `tests/nakshatra.c`: 5 deg and 10 deg split into nakshatras, rounded
to the second, under the 47 predefined modes, USER alone and with
USER_UT, ECL_DATE, ECL_T0 and SSY_PLANE, Sheoran with ECL_DATE and Lahiri
with four SIDBITs: Ashvini 5 deg 00' and 10 deg 00' but in Sheoran,
Ashvini 8 deg 20' and Bharani 0 deg 00' there: 114 checks. The build
before the fix and the upstream build fail the 10 USER ones. G24
`user_nakshatra_off`: USER's 10 deg read back, 0 shipped, 3.33 deg compat.

Measured with it, below the precision floor (1e-9 deg, 1e-8 deg/day) and
not fixed:

- **A planet's light-time velocity at the instant read.** With default
  flags Mercury's geocentric speed is 4-6e-11 deg/day off five-point
  differences at some dates (1990, 2200), the same at every step from
  2^-5 to 2^-9 day, and 1.7e-11 with SEFLG_TRUEPOS. It is the light time,
  not the aberration or the deflection: the same with SEFLG_NOABERR |
  SEFLG_NOGDEFL, 9.0e-11 at the worst of 400 dates 1900-2100, 1.25e-11
  rms (Venus 2.1e-11, 3.5e-12 rms; Mars at its floor; de200 8.0e-11;
  Moshier 8.1e-11; `swe_calc_pctr()` Mercury from Mars 8.3e-11). After the
  read at t = teval - dt, `app_pos_etc_plan()` moves the body to the
  emission instant along a velocity, by t's rounding (bug 63) and by the
  Newton step on the light time (bug 78), about 1e-9 day together, and
  keeps the velocity read at t: off by the acceleration times that shift,
  each part about half. Moving the velocity too, by the shift times
  (v(teval) - v(t)) / (teval - t) as the Moon's path does (bug 73), gave
  1.6e-11 worst and 3.3e-12 rms on Mercury, the true positions' floor
  (pctr 1.8e-11, JPL 1.8e-11, Venus 1.6e-11); written and reverted under
  the floor. The fictitious bodies' light-time positions (bug 81) and
  `swe_calc_pctr()` (77) have the same shape; Waldemath's and the White
  Moon's residuals are their geocentric cancellation's noise (4e-12 deg
  per position) all the same.
- **G51's geocentric 2.5e-11 deg/day** is not the above: the light time is
  the same with and without aberration and cancels in G51's difference
  (unchanged by the fix above). Over 2^-8 day it is the reference's noise
  (4.8e-11 over 2^-9); over 2^-4 to 2^-7 day it stays at 1.5e-11, Jupiter
  at 500 AD alone (every other body and date <= 9e-12), a 28-day
  sawtooth of +-1.5e-11 with jumps every 28 days, 5e-6 of the aberration
  rate term, +-7e-12 at 2000; Moshier noisier. Not traced; the
  shape points at the Swiss files' Earth (its monthly terms against the
  gravitational jerk `earth_at_lt_sun()` adds), not at the code's terms.

## 100. A missing Swiss file whose neighbour does not hold the instant leaves the neighbour open, and the Moon is never read: NaN fresh, the last read's Moon warm (this fork's)

**Severity: Correctness (with semo_12.se1 missing, the Moon NaN with no error on a fresh context, 37 deg off warm; the Sun and every geocentric body 6 arcsec apart fresh and warm)**
**Where:** `sweph.c`: `sweph()`, F11's neighbour-file lookup (this fork's); the reader it misled is `sweplan()`'s Moon

Found by entry 89 and left there. F11 (REVIEW, the 2026-09-16 review)
made `sweph()`, when the file named for the instant is missing, open the
neighbouring 600-year bucket's file, whose data runs a little into this
bucket; the range check after it refuses an instant it does not hold.
That refusal returned NOT_AVAILABLE and left the neighbour open. For
the planets and the main asteroids the return value is what the callers
look at -- `main_planet()` falls to Moshier with "using Moshier eph.", an
asteroid is an error -- but `sweplan()` reads the Moon to make the Earth
from the EMB and takes "the moon file is not open" as its sign that there
is no moon file and the Moshier Moon is to be used. With the neighbour open
it went on with a Moon it never read: the save area's, emptied (NaN) on a
fresh context and the last read's on a warm one. Measured with semo_12.se1
missing, at JD 2159377.5 (1200.0 + 20 days; semo_06's data ends at +16):
a fresh Moon NaN with flags 0x100 and no error, a warm one (after the
Sun 1.5 days before 1200.0) 26.44179 deg where the Moshier Moon, upstream's
answer, is 349.29034; the Sun 6.03 arcsec apart fresh and warm, Mercury
and every body whose Earth comes from that Moon likewise, and J2000 XYZ
the Moon zeros. The same at every instant of the bucket beyond the
neighbour's overrun, after it or before it (semo_18 starts 9 days early,
so 1800.0 - 20 days with semo_12 missing). Upstream has no neighbour
lookup: a missing moon file is closed, and the Moon is Moshier's. The
planets' and asteroids' paths were right before; only their message
named the neighbour's limit for the missing file.

**Fix, as done here:** a neighbour that does not hold the instant is
closed again, its bodies' file data with it, and the instant's own file
reported as not found -- the state and message of a missing file with no
neighbour at all, so `sweplan()` takes the Moshier Moon with "using Moshier
eph. for moon" and a warm context answers as a fresh one. A neighbour that
holds the instant still answers it. A file present under its own name
that does not hold its own bucket's instants cannot occur with the
distributed files (the Moon's files outlast the planets' at both ends of
the ephemeris, and `read_const()` refuses a file under another bucket's
name), so `sweplan()`'s test is unchanged. The open path only; no
calculation that finds its file is touched.

Not this entry's, and left (upstream's too): a `read_const()` error -- a
file under another bucket's name, "Ephemeris file name 'seas_06.se1'
wrong; rename 'seas_00.se1'" -- runs `free_planets()`, which empties the
constants of the files that stay open, so the next calls fail on them:
after Ceres in 600.0 + 375 days with seas_06.se1 a copy of seas_00.se1,
the Sun 25 days later "sepl_06.se1 is damaged (2)", Mars there "semo_06.se1
is damaged (2)", Mars 1000 days on "sepl_06.se1 is damaged (2)", and only
the fourth call answers; the same in the compat build. Fixed as entry
104.

G99 `tests/missfile.c`, in two directories of links in /tmp, (a) without
semo_12, sepl_18 and seas_06, (b) without semo_18, sepl_12 and seas_12, at
15 instants about 600.0, 1200.0 and 1800.0 (inside and past each
neighbour's overrun, both sides), nine bodies, four flag sets with speed:
(1) every answer an error or finite; (2) warm from five calls equal to
fresh, bit for bit; (3) within 60 arcsec of the complete directory's, and
"Moshier" in serr when more than 0.5 arcsec from it. 1080 rows, none
fail; ts20 before the fix fails 215 (21 not finite, 84 warm, 110 off or
unannounced, all through the Moon), the upstream build, which has no
neighbour lookup, 320 (all check 2). G24 `moon_neighbour_off`, asserted the same both
ways because the lookup is this fork's: the Moon at JD 2159377.5 with
semo_12 missing, fresh against warm from 1200.0 + 10, 0 shipped and compat,
1e9 (NaN) before the fix. G24, G62, G83 and G88 pass; G99 also under ASan
and UBSan.

Also (2026-10-25 review, D6): with both the instant's own file and the
neighbour missing (semo_12 and semo_06), serr named the neighbour last
tried, "semo_06.se1 not found", for the Moon at 1200; it names the
instant's own file now, as with no neighbour at all. G99 (e) checks it.

## 95. The Moshier Moon's velocity is a difference of the series, and the osculating apogee's eccentricity is sqrt(1 - p / a): the osculating apogee and perigee jitter

**Severity: Correctness (Moshier: `swe_nod_aps()`'s osculating perigee and apogee speeds noisy by up to 5.5e-6 deg/day, SE_OSCU_APOG's by 1.4e-7, positions by 3.4e-9 deg; de200: SE_OSCU_APOG's speed by 2.4e-8 deg/day; the Moshier Moon's speed 1.2e-10 of itself off)**
**Where:** `swemmoon.c`: `swi_moshmoon()`, `swi_moshmoon2()`, `mean_elements()`, `mean_elements_pl()`, `moon1()`..`moon4()`, `chewm()`, `ecldat_equ2000()`; `sweph.c`: `lunar_osc_elem()`; `sweph.h`: `struct moon_state`

Found while fixing entry 87: on Moshier the osculating apogee's
longitude (SE_OSCU_APOG, and `swe_nod_aps()` SE_NODBIT_OSCU for the
Moon) jittered from one instant to the next 2^-30 day later, at every
date. Measured as the rms of fourth differences over sqrt(70), 32
instants 2^-30 (and 2^-20) day apart, eight dates -2500..2100 (de200
1700..2100):

- The Moon's state. Moshier's velocity was bug 51's five-point
  difference of the series over +-2^-5 and +-2^-4 day. The series'
  samples are good to their last bits (longitude 1e-16..1.6e-15 rad, the
  J2000 vector 1e-16..1e-15 of its length), but the difference divides
  that by 0.03 day: the velocity jittered by 1e-14..1.2e-13 of itself,
  against 1e-15..2e-15 on the Swiss files and JPL (whose velocity is the
  Chebyshev series' derivative). Its truncation, (w h)^4 / 30, was
  1.2e-10 of it (rms, against a nine-point difference). The apogee of a
  near-circular orbit turns by about 2 dv / (v e), e = 0.055 (36-fold):
  SE_OSCU_APOG and `swe_nod_aps()`'s points jittered by 3e-10 deg at
  present dates, 3.4e-9 at -2500, their speeds by 1.4e-7 (five points
  over 2^-5 day) and 5.5e-6 deg/day (three over 2^-11; 1.8e-5 off the
  rate of the positions at -1125). With a 9-point velocity over 2^-3 day
  the jitter fell threefold, as the noise ratio says. Moshier only.
- `lunar_osc_elem()` took the eccentricity as sqrt(1 - p / a), 1 - p / a
  being e^2 less a few ulps of 1, and the true anomaly from it through
  cos E = (1 - r / a) / e: the formula entry 80 replaced in
  `swe_nod_aps()`. From the same Moshier states, the eccentricity vector
  in long double put the apogee's jitter at 0.9e-12..1.3e-12 deg, this
  formula at 1.2e-11..1.5e-11. On every ephemeris: SE_OSCU_APOG 1.1e-11
  deg and 5.4e-10 deg/day on the Swiss files, 3.5e-10 deg and 2.4e-8
  deg/day on de200 (the last above the residual floor, which is why the
  formula is changed; the Swiss figures are below it and fixed by the
  same change).

**Fix, as done here:** the Moshier Moon's velocity is the series' own
derivative. Each statement of `mean_elements()`, `mean_elements_pl()`,
`moon1()`..`moon4()` gets an `MD()` line before it computing the d/dT
of what it assigns (forward-mode, generated mechanically from the
statements and checked by hand; a cos/sin pair's rates after the pair),
`chewm()` accumulates each table line's rate from its angle's, sum j x
dang[m]; the values are untouched (positions bit for bit), and the
rates are only computed while `ctx->moon.want_d` is set
(`moshmoon2_d()`, from `swi_moshmoon()`; the interpolated apsides and
the mean elements do not pay for them). The polar rates are made
cartesian with the position, turned by the same frame, and the frame's
own turning added as R'(t) x, the same frame at tjd +- 1 day applied to
x (it moves at 6.6e-7 rad/day: curvature and rounding below 1e-15 of
the Moon's speed). `lunar_osc_elem()` takes the eccentricity vector,
((v^2 - GM / r) r - (r.v) v) / GM, as `swe_nod_aps()` does: e its length,
the true anomaly the angle from it to r about h.

After: the Moshier velocity agrees with a degree-12 least-squares fit
of 2049 positions over +-1 day to 4e-14 of itself (rms 1.7e-14, the
fit's noise), and jitters by 1e-16..1.4e-15. SE_OSCU_APOG's jitter
<= 5.7e-13 deg (Swiss), 4.2e-13 (de200), 1.5e-12 (Moshier), its speed's
<= 4.1e-11 deg/day; `swe_nod_aps()`'s osculating points <= 2.4e-11 deg
and 3.6e-8 deg/day on all three, against 5.8e-8 off the rate of the
positions on Moshier at -1125 (the step's truncation). The floors: for
SE_OSCU_APOG the Moon's state inside `lunar_osc_elem()`, about 1e-12 deg
(the long-double figure above); for `swe_nod_aps()`, which reads the Moon
through `swe_calc()`, that state's 2e-14 of its distance on every
ephemeris times 2 / e, <= 5.0e-11 deg; its speed is a three-point
difference over 2^-11 day of the points, 1448 times their noise.
Positions move by what was removed: the Moshier osculating apogee and
`swe_nod_aps()` perigee by up to 3.7e-7 deg (the old velocity's
truncation through 2 / e), the Moshier true node by 4.0e-10 deg, the
Swiss and de200 apogee by 2.7e-10 deg (the formula's rounding); the
Moshier Moon's apparent place by 1.2e-12 deg (rounding at the
barycentric scale through the light time), its speed by 3.9e-9
deg/day; nothing else moves (planets, Sun, interpolated apsides, Swiss
and de200 Moon bit for bit, their true node within 6e-14 deg and
4e-13 deg/day). The Swiss true node's distance and speeds also move, and
its longitude by 1-2 ulp: `lunar_osc_elem()` scales the node's distance to
the osculating ellipse with `ecce` and `ny`, which now come from the
eccentricity vector (G1: 112 distance, 12 longitude, 120 speed rows).
Cost: one series evaluation with its rates and three frames where the
difference took five of each: the `moon` bench 0.90 -> 0.565 s,
calc-moseph 1.045 -> 0.957 s, calc-swieph unchanged.

G94 `tests/oscsmooth.c`: the Swiss files, de200 and Moshier, eight dates,
32 instants 2^-30 and 2^-20 day apart, the jitter of SE_OSCU_APOG's and
`swe_nod_aps()`'s osculating perigee's and apogee's longitude and
longitude speed: 18 rows, SE_OSCU_APOG within 3e-12 deg and 1e-10 deg/day
(measured 1.5e-12, 4.1e-11), the `swe_nod_aps()` points 6e-11 and 8e-8
(2.4e-11, 3.6e-8); it prints the Moon's-state floor per ephemeris. The
build before the fix fails 10 rows, the upstream build all 18. G24
`moon_oscu_peri_noise`: the Moshier osculating perigee's speed jitter at
JD 2460000.297, 1.28e-8 deg/day shipped (1.05e-7 before the fix), 0.456
compat. G74 (Moshier apogee 7.2e-8 -> 8.2e-9 deg/day against its
reference; its 4e-7 limit not tightened), G79, G41, G86, G84, G52, G40,
G46, G38, G54, G92 pass.

## 104. A refused Swiss file empties the constants of every file left open, so the next calls fail on healthy files; the path setter crashes on a refused moon file (the crash this fork's)

**Severity: Correctness (calls on other bodies fail "file is damaged" until each open file has been reopened; the healthy open files and entry 89's spares reopened) / Memory (fclose(NULL): the path setter segfaults, both builds)**
**Where:** `sweph.c`: `read_const()`'s and `get_new_segment()`'s error exits (upstream); the path setter's moon header read (this fork's, ts.5)

Found by entry 100's gate and left there. `read_const()` refuses a file
that is not what its name says ("Ephemeris file name 'seas_06.se1' wrong;
rename 'seas_00.se1'"), is damaged or short; `get_new_segment()` refuses
one whose segment cannot be read. Both closed the file and ran
`free_planets()`, which zeroes every body's `plan_data` -- the constants
(`tfstart`, `dseg`, `lndx0`) of the files that stay open as well. The next
call on one of those found its file open, skipped `read_const()`, and
looked up a segment with `dseg` 0: an error, which closes that file and
runs `free_planets()` again. Measured with seas_06.se1 a link to
seas_00.se1, after Ceres at 600.0 + 375 days: the Sun at 600.0 + 400
"sepl_06.se1 is damaged (2)", Mars there "semo_06.se1 is damaged (2)", Mars
1000 days on "sepl_06.se1 is damaged (2)", and only the fourth call
answered; a fresh context answers all three. On this fork `free_planets()`
also drops entry 89's spares, so going back to the previous bucket
reopened sepl, semo and seas (three opens, traced). The same in the compat
build. A related crash: the path setter reads semo_18's header (this fork's
ts.5 replacement of upstream's Moon calculation) and on a refusal closed
the file `read_const()` had already closed, `fclose(NULL)`: with semo_18.se1
holding semo_12's data, `swe_set_ephe_path()` segfaults, in both builds.

**Fix, as done here:** a refusal closes and clears only the refused file:
its `fidat` slot and its bodies' file data (the part of `plan_data` before
`teval`), and the slot numbered asteroids and planetary moons share only
if this file had begun writing it. The other open files, their constants
and the spares are left as they were, so nothing healthy is reopened (the
same trace: no opens after the refusal), and the calls after it answer as
a fresh context with the same directory does. The general constants a
refused file may have written (`gcdat`) are read by nothing. Upstream's
exits are under `SWE_UPSTREAM_COMPAT`. The path setter no longer closes
the file twice; that code is this fork's and is fixed in both builds.

Not this entry's, and left: `read_const()` indexes `pldat[]` with the
body numbers the file lists, and checks them only against
`SE_PLMOON_OFFSET`, not `SEI_NPLANETS`. They are covered by the header's
CRC, so a damaged file cannot reach it; only a crafted file whose CRC was
recomputed could write past the array. Entry 106 now refuses such a file.

G99 `tests/missfile.c`, extended: (c) a directory with copies of semo_06,
sepl_12 and seas_00 as semo_12, sepl_18 and seas_06, and a copy of seas_00
with Pallas's segment index overwritten (get_new_segment() refuses Pallas,
and only Pallas). Every row of checks 1-3 there, plus two warm-ups: the
call, then the four calls that meet a refused file (Ceres 600.0 + 375, the
Moon 1200.0 + 20, Mercury 1800.0 + 100, Pallas 600.0 - 50000), then the
call; and those four, then the call. (d) a copy of semo_12 as semo_18:
setting the path does not crash, J2000 is refused for the name, the Sun at
1800.0 - 100 answers as the complete directory's. 1728 rows, none fail;
before the fix (c) fails 77 (warm against fresh) and (d) crashes; the
upstream build 521 ((c) 201; (d) passes there, its fix in both builds).
G24 `refused_file_off`: the Sun 25 days after the Ceres that met the
misnamed seas_06, against alone, 0 shipped, 1e9 (an error) compat and
before the fix. G24, G62, G83 and G88 pass; G99 also under ASan and UBSan.

## 103. Files closed and reopened when nothing changed, and `swe_deltat()`'s ephemeris guessed from the files this thread has open

**Severity: Correctness (returned values: 169.4 s of delta t in AD 1000, 7.5e-8 deg of ayanamsa) and performance (up to 13 times slower)**
**Where:** `swephlib.c`: `swi_guess_ephe_flag()`, `swe_deltat_r()`; `sweph.c`: `swe_get_ayanamsa_r()`, `swe_get_ayanamsa_ut_r()`, `swe_set_ephe_path_r()`, `ephe_path_resolve()`, `swi_set_ephe_path_implicit()`, `swe_set_jpl_file_r()`, `ephe_switch_forget()` (the ephemeris switch in `swecalc()`, `swe_calc_orbel` and the two fixed-star calculations), `ctx_release()`; `sweconfig.c`: `swi_config_apply()`; `sweph.h`: `ephe_path_implicit`, `ephe_path_live`

**The guess.** `swe_deltat()`, `swe_get_ayanamsa()` and
`swe_get_ayanamsa_ut()` take no ephemeris flag and guess one:
`swi_guess_ephe_flag()` said JPL when this context had the JPL file open.
So after `swe_set_jpl_file("de200.eph")`, `swe_deltat()` in AD 1000 was
1305.37 s on the setting thread and 1474.79 s on a second one, which had
not opened the file (the ayanamsa 7.5e-8 deg apart in AD 1000; upstream's
second thread, sharing no configuration, 1463.50 s); and after a path
change and a JPLEPH calculation, which reopened the file by its kept name,
1305.37 for the 1474.79 just before it. The guess also ran before the
context had synced, so a second thread's first call took the stale
configuration. Upstream's in part.

**The churn.** Measured in `fopen()` calls over 100 rounds after a warm-up
(G102 interposes `fopen()` and counts per thread):

| Round | Before | After |
|---|---|---|
| any setter + a calculation, files named by `SE_EPHE_PATH` | 794 | 0 |
| `swe_set_ephe_path(same)` + a calculation | 398 | 0 |
| `swe_set_jpl_file(same)` + a JPL calculation | 299 | 0 |
| `swe_set_ephe_path(same)` + `swe_fixstar2()` | 300 | 0 |
| Swiss / de200 / Moshier alternating | 101 | 0 |
| a second thread computing Swiss while the first sets the de200 file | 2 | 0 |
| a real path change, alternating two directories (must reopen) | 398 | 396 |

- With the files named by `SE_EPHE_PATH`, the first calculation sets the
  path itself, locally (it publishes nothing), and the shared master
  names no path. Any setter's publish then made the next sync adopt "no
  path": `path_changed`, every file closed, every cache emptied, and the
  next calculation set the implicit path again. A setter and a Moon cost
  52.8 us a round for 4.0 with an explicit path. This fork's; it was how
  entries 92, 98 and 101 lost their settings.
- `swe_set_ephe_path()` with the path the context already holds closed
  every file and cache and reopened the moon file for its header;
  `swe_set_jpl_file()` with the name already open closed every Swiss file
  too; a fixed star after either reread the whole catalogue.
- Every switch of ephemeris between calculations (`last_epheflag`) closed
  every ephemeris file and the JPL file and freed every planet's file data:
  alternating Swiss and de200 on Mars, 3000 opens in 2000 requests, 35.6
  us each for 6.7.
- A thread adopting another thread's `swe_set_jpl_file()` closed its
  Swiss files and its star catalogue, though only the JPL name moved.

**Fix, as done here** (behind `#ifndef SWE_UPSTREAM_COMPAT`):

- The guess follows the configuration: JPL when a JPL file has been set
  and read (`jpldenum_cfg`, published with the path); the three functions
  initialise and sync before guessing.
- The implicit path is set through `swi_set_ephe_path_implicit()`, which
  marks it; a sync against a master that names no path keeps a marked
  path (that is what "no path" means for it).
- `swe_set_ephe_path()` with the path the context holds, live (set or
  adopted, not since `swe_close()`), keeps its files and caches and only
  resolves an automatic tidal acceleration again; `swe_set_jpl_file()`
  with the name open and read does nothing but publish. Chosen: a caller
  who rewrote a file on disk and wants it read again calls `swe_close()`
  first, which releases everything, as the manual has it before a new
  `swe_set_ephe_path()`. A same-path call no longer rereads
  `seleapsec.txt`, `swe_deltat.txt`, `seorbel.txt` or `sefstars.txt`.
- An ephemeris switch forgets every position (each body's last result,
  the save areas, the nodes, the Earth-Moon memo -- cleared, not only
  their instants: a Moshier request reads the barycentric Sun's slot,
  which it never fills, as zero, and left at a Swiss Sun it moved Mars
  2.9e-9 deg, which G83 caught) and keeps the files and their constants.
- A sync whose only path change is the JPL name or DE number closes the
  JPL file only.

A real change still closes and re-derives everything entries 92, 98 and
101 need; G91, G97, G100, G82 and G83 pass, and the golden transcript is
unchanged.

Noticed, not fixed: with the files named by `SE_EPHE_PATH`, the first
`swe_calc_ut()` takes its delta t before `swe_calc()` sets the implicit
path, so it reads `swe_deltat.txt` along the compiled default path: with
a `swe_deltat.txt` giving 70.0 s for 2000 in the `SE_EPHE_PATH`
directory, the first Moon at 2000 Jan 1 12h was 223.323751 deg and the
second 223.324610 (8.6e-4 deg); and the table is read twice. Fixed in
entry 107.

G102 `tests/keepopen.c`: the seven counted rounds, each answer against a
fresh process within 1e-12, and the three guess cases: 10 pass. Before the
fix 9 fail, the compat build 9, the upstream build (legacy-master) 8 (its
setter round opens nothing, having no shared configuration; its second
thread finds no files at all, 600 opens). G24 `dt_guess_thread_off`: 0 s
shipped, 169.4 s compat. G24, G81, G82, G83, G91, G97, G100, G2 and the
context gate pass; bench within noise.

## 105. `swe_nod_aps()`'s osculating ellipses take the Astronomical Almanac's GM, not the ephemeris's, and Pluto Mercury's mass

**Severity: Correctness (the Moon's osculating apsides 5e-5 deg, 7.8e-6 deg/day and 2e-10 AU off SE_OSCU_APOG's; Venus's perihelion 3e-6 deg; on de200 Neptune's 2.9e-3 deg and 3.3e-5 AU, Uranus's 1.4e-4 deg; Pluto's 3.9e-5 deg and 2e-5 AU on every ephemeris)**
**Where:** `swecl.c`: `swe_nod_aps_r()`, the osculating points (SE_NODBIT_OSCU, SE_NODBIT_OSCU_BAR); `sweph.c`: `swi_gm_earth_moon()`, `swi_gm_sun_planet()`

`swe_nod_aps()` fits each osculating ellipse to the body's state with a
GM: the Moon's G M_earth (1 + 1 / EARTH_MOON_MRAT) from the Astronomical
Almanac's 1996 G M_earth, the planets' HELGRAVCONST (1 + 1 / plmass) /
AUNIT^3. Neither is the ephemeris's own constant. The Moon's is 3.3e-8
above DE441's GMB (bug 28 gave `lunar_osc_elem()`, SE_OSCU_APOG, the
ephemeris's GMB, and left this copy); the Sun's is 1.80e-10 below k^2,
the GMS of every JPL header from DE200 to DE431, and 1.76e-10 below
DE440/441's; plmass is DE431's planets (but Mercury, 1.4e-5, and Pluto,
5e-3), and de200's differ (Uranus 2.5e-3, Neptune 5e-3, Pluto 4e-2).
And `ipl_to_elem[]` sends Pluto (9) to 0, Mercury's element: Pluto's
ellipse took Mercury's mass, 1.6e-7 of the Sun's, where its own is
7.4e-9. The eccentricity vector turns by about dGM / (GM e) and the
semi-axis moves by about dGM / GM.

Measured with SEFLG_TRUEPOS, 64 dates 1700..2100: the Moon's osculating
apogee from `swe_nod_aps()` against SE_OSCU_APOG, distance 1.96e-10 AU
(Swiss files), 1.85e-11 (de200, whose file GMB SE_OSCU_APOG takes),
2.08e-10 (Moshier), longitude 5.3e-5 deg, 4.9e-6, 5.7e-5. The planets'
perihelia against the eccentricity vector of `swe_calc()`'s own
heliocentric J2000 state with the header's GMS + GMi (G104): distance
up to 2.5e-10 AU (Venus), 1.8e-9 (Jupiter), 1.0e-8 (Neptune), Pluto
6.9e-6; on de200 direction Venus 1.5e-6 deg, Jupiter 1.2e-6, Saturn
8.1e-6, Uranus 1.3e-4, Neptune 2.5e-3, Pluto 3.7e-5.

**Fix, as done here:** the ellipses take the ephemeris's constants: the
Moon's `gm_earth_moon()` (bug 28's, exported as `swi_gm_earth_moon()`),
the planets' `swi_gm_sun_planet()`: GMS + GMi (GMB for the Earth's orbit)
from the JPL file's header when one is open, otherwise from the header the
DE number names (DE440/441, DE430/431, DE405/406, DE403 for Moshier, DE200;
ssd.jpl.nasa.gov/ftp/eph/planets/ascii, GROUP 1041, read 2026-10-24), the
Sun's alone for asteroids and fictitious bodies; the ephemeris is the one
`swe_calc()` answered with. Pluto takes its own element. After: the Moon's
apogee distance against SE_OSCU_APOG <= 1.4e-16 AU on all three, longitude
4.1e-11 deg on de200; the perihelia <= 6e-14 AU, direction <= 4e-12 deg
on de200. Positions move by what was removed: geocentric default flags,
900..2960 (de200 1959..2027), the Moon's points by 5.1e-5 deg (speeds
7.8e-6 deg/day, distances 2.1e-10 AU), Venus's 3.0e-6 deg, the Sun's
(the Earth's orbit) 6.1e-7, Jupiter's 3.0e-7, Mars's 1.1e-7, Chiron's
2.5e-8, Pluto's 3.4e-5 deg and 1.9e-5 AU; on de200 Neptune's 2.9e-3 deg
and 3.3e-5 AU, Uranus's 1.4e-4, Saturn's 8.6e-6, Jupiter's 1.5e-6. Not a
hot path (once per `swe_nod_aps()` call).

Left, recorded, not this defect:
- The direction of the Moon's osculating apogee from `swe_nod_aps()`
  still differs from SE_OSCU_APOG's on the Swiss files and Moshier, by
  6.2e-6 deg in latitude and 2.4e-6 in longitude (SEFLG_TRUEPOS; twice
  that with SEFLG_ICRS), and by 5.5e-6 deg from the eccentricity vector of
  `swe_calc()`'s J2000 state; on de200, an ephemeris without a frame bias,
  they agree to 4e-11. `swe_nod_aps()` reads `swe_calc()`'s J2000 state,
  frame bias applied, and `swi_plan_for_osc_elem()` applies the bias again
  (for DE >= 403), so every osculating point of every body on those
  ephemerides is turned by the frame bias once more. Above the floor;
  a separate item, fixed as 108.
- The Earth's (the Sun's) osculating ellipse is fitted to an EMB built
  with EARTH_MOON_MRAT, the AA's 81.30055985, not the ephemeris's EMRAT
  (DE200 81.300587): on de200 3.6e-7 deg on its perihelion and 1e-11 AU,
  1.2e-7 deg with DE431's 81.30056907 (G104 builds the EMB the same way). Above the floor; a
  separate item, fixed as 108.
- With the default (apparent) flags `swe_nod_aps()`'s and SE_OSCU_APOG's
  apogees still differ by 9e-5 deg, as they always have: SE_OSCU_APOG
  fits the light-time-corrected Moon, `swe_nod_aps()` the geometric one
  and applies its corrections to the points. Two definitions, not a
  rounding.
- `swe_get_orbital_elements()` (`get_gmsm()`) takes the same AA
  constants, with deliberate Astronomical Almanac factors on top, and the
  same `ipl_to_elem[]` for Pluto; not changed here. (Fixed as 108, which
  found the factors compiled only under `TEST_ORBEL_AA`, a test switch
  nothing sets.)

G104 `tests/nodapsgm.c`: the Swiss files (their DE number's header,
DE441 here), de200 and Moshier, eight dates each: (a) the Moon's apogee
against SE_OSCU_APOG, distance within 1e-14 AU, direction within 1e-9 deg
on de200; (b) Mercury..Pluto and the Earth (EMB) against their own
eccentricity vectors, perihelion distance within 1e-12 AU, direction
within 1e-9 deg on de200 (directions not compared on the others, above).
30 rows, measured <= 1.4e-16 and 6e-14 AU, 4.1e-11 deg. The build before
the fix fails all 30; the upstream build 27 (its two GMs agree for the
Moon). G24 `nodaps_gm_off`: Venus's osculating perihelion distance on
Moshier at JD 2460000.297 against DE403's GMS + GM2, 1.11e-16 AU shipped,
1.15e-10 compat. G74's Moshier apogee limit tightened from 4e-7 to 3e-8
deg/day (measured 8.2e-9; 1.8e-8 over 150 dates; the build before entry
95 fails its five rows). G94, G74, G79, G86, G69 pass.

## 102. Moshier's light time holds the Sun still

**Severity: Correctness (3.0e-6 deg, v_Sun / c, on every Moshier position without aberration and every heliocentric one; their speeds by up to 7.8e-7 deg/day)**
**Where:** `sweph.c`: `app_pos_etc_plan()`, `app_pos_etc_sun()`, `app_pos_etc_moon()`; `swemplan.c`: `swi_mosh_sun_bary()` (new); `sweph.h`: its memo

Entry 78's remainder. Moshier's positions are heliocentric and its light
time was solved in that frame: X(t - tau) - O(t) with the Sun held still,
where the Swiss files and JPL take X(t - tau) - O(t) about the barycentre,
the Sun moving 13 m/s. The light time leaves out S(t - tau) - S(t) =
-v_Sun tau. Without aberration the geocentric Sun had no light time at all
(-E(t) for S(t - tau) - E(t)), the heliocentric Earth was E(t - tau) - S(t
- tau), the planets, asteroids and the Moon (geocentric, topocentric and
heliocentric) each v_Sun tau off. With aberration it cancels: the
aberration was taken with the heliocentric Earth's velocity, v_Sun short,
and the apparent direction depends on the relative velocity only.

Not a convention. The library defines a heliocentric light time as
X(t - tau) - S(t), the Sun where the light arrives, on the files and on
de200 (entries 26 and 78 part 2), and an astrometric place about the
barycentre; Moshier's differed from its own other two ephemerides by
v_Sun tau, and from its own apparent place by more than the aberration.

Measured, Moshier against the Swiss files, 121 dates 1800..2200, J2000
equatorial, deg. Moshier's own theory error (the apparent place's) is
larger for the major bodies: Sun 2.2e-5, Moon 7.4e-4, Mercury 3.1e-5,
Venus 8.5e-5, Mars 1.7e-4, Jupiter 1.9e-4, Saturn 1.2e-4, Uranus 3.2e-4,
Neptune 7.2e-4, Pluto 1.6e-3 (to 0.16 deg on Mars and the Moon at
-2900..+2900). It cancels from the place without aberration less the
apparent place against the files' same difference, which leaves v_Sun tau:
2.96e-6 to 3.04e-6 on every body, the predicted v_Sun / c across the line
of sight to <= 1e-8 (further out the theory error couples in: 1.4e-5 on
Mars and the Moon near -2900). An asteroid under Moshier is read from the same file as on the Swiss
path (its true heliocentric places agree to 1e-14): Chiron and Ceres
heliocentric were 2.97e-6 and 2.92e-6 off the files, the whole of their
difference, and Chiron's geocentric place without aberration 3.5e-6
against 1.4e-6 apparent.

**Fix, as done here:** `swi_mosh_sun_bary()` forms the Sun's barycentric
velocity and acceleration from Moshier's eight planets, the barycentre of
point masses: v_Sun = -sum m_i v_i / (M_Sun + sum m_i), the planets'
heliocentric series velocities (the Earth-Moon barycentre's series),
DE431's mass ratios; a_Sun = k^2 sum (m_i / M_Sun) x_i / r_i^3. Left out:
Pluto (3e-5 m/s), the asteroids (8e-6 m/s for Ceres). Against the files'
barycentric Sun it is good to <= 6.5 mm/s 1800..2200 (4.1 mm/s
-2900..+2900), which is what the files' own planets give the same way: the
files' Sun is not the eight-planet barycentre closer than that, and jumps
by up to 1.2e-8 AU at its yearly segment joins (below). 6.5 mm/s is
1.2e-9 deg of v_Sun / c. Memoised per instant in the context, emptied with
the other instants. Without aberration (and not SEFLG_TRUEPOS), each of the
three light-time paths now works with the origin at the Sun at teval and
barycentric velocities, as on the files: every velocity at teval gains
v_Sun, the body read at t moves by the Sun's displacement s v_Sun + s^2
a_Sun / 2, s = t - teval, and the heliocentric centre is the Sun at teval,
(0, v_Sun); the light-time loops, the Newton step, the rounding move and
the light-time rate are then the files' own. The apparent path is not
touched (bit-identical, apparent and topocentric, positions and speeds),
nor SEFLG_TRUEPOS.

After: the place without aberration less the apparent one against the
files', 1800..2200: 1.2e-8 to 6.8e-8 (Pluto 1.6e-7), the theory error's
coupling; Chiron and Ceres heliocentric 7.7e-10 and 7.1e-10 off the files
(2.5e-10 away from the joins), Chiron's geocentric place without
aberration 1.44e-6, as apparent. Moshier positions without aberration and
heliocentric move by 2.7e-6 to 3.0e-6 deg (-2900..+2900), their distances
by up to 2.4e-6 AU (Pluto), speeds by up to 6.6e-7 deg/day (the Moon
without aberration; 7.8e-7 topocentric, 2.8e-7 Mercury heliocentric), each
the rate of the moved positions: against five-point differences the same
before and after (Mercury 4.5e-11 deg/day, the Moon 9.5e-9). Cost:
calc-moseph, the apparent bench, +0.01% instructions (perf stat). The
eight series per new instant land on the other paths: a ten-body chart
without aberration +12%, heliocentric +17%, a single body without
aberration per instant +30%.

Not fixed here, fixed since in entry 109 (G108): the apparent distance.
The apparent direction needs no v_Sun, but the light-time distance does,
rho^.v_Sun tau, along the line of sight: Mercury 7.1e-8 AU, Mars 1.3e-7,
Saturn 4.4e-7, Pluto 2.4e-6, Chiron 8.3e-7 (its apparent distance 9.2e-7
off the files against 3.1e-7 without aberration), the Sun 5e-8, the Moon
1.3e-10. Nor the fictitious bodies (`app_pos_etc_plan_osc()`, the same
2.6e-6 to 3.0e-6 deg heliocentric on Moshier, Cupido, Isis, Vulcan), nor
the fixed stars, whose Moshier aberration was taken with the heliocentric
Earth's velocity and has no light time to cancel it (3.0e-6 deg on Spica,
Aldebaran, Polaris and Sirius apparent; Sirius 9.2e-7 without aberration,
its parallax from the heliocentric Earth). Entry 109 also adds Pluto to
v_Sun.

Found on the way, the reference's: the Swiss files' barycentric Sun is
discontinuous at its yearly segment joins, by up to 1.2e-8 AU (302 joins
over 1e-10 AU, 2000..2300). A light time that straddles one carries it:
the Swiss geocentric apparent Sun 2.1e-7 deg off its neighbours at
2451547.51 for one light time, Mercury 1.5e-7 geocentric and 4.9e-7
heliocentric, Ceres heliocentric 5.6e-8 on 7 Jan 2200. Not fixed here.

G101 `tests/moshsunlt.c`: (1) Chiron and Ceres heliocentric under Moshier
against the Swiss files, 13 dates 1800..2200 mid-way between the joins,
within 2e-9 deg: 2.5e-10 measured; (2) the astrometric place of the Sun,
the Moon, Mercury, Jupiter and Chiron geocentric and topocentric, of the
Moon, Mercury, Jupiter and Chiron heliocentric and of the heliocentric
Earth, against the light time's fixed point about the barycentre, iterated
in the gate from the library's heliocentric true places and v_Sun, a_Sun
formed there from its planets, 31 dates -2900..+2900, within 1e-11 deg
(<= 3.6e-13; the Moon within 5e-10, 1.1e-10, its series' own floor, the
same about the heliocentric frame before the fix); (3) the speeds of
Mercury and the Sun without aberration, Mercury and the Earth
heliocentric, within 2e-9 deg/day of five-point differences over 2^-7 day
(<= 5.2e-11), the Moon without aberration within 2e-8 (9.5e-9). 22 rows.
The build before the fix fails 17 (all of (1) and (2), by 2.6e-6 to
3.0e-6 deg); the upstream build all 22. G77, G52 and G33 judged Moshier
against the light time about its heliocentric frame and now take the Sun's
motion over tau into their references the same way (the build before the
fix fails G77's nine Moshier rows by 3.0e-6 deg, G52's twelve by 8.2 to
10.7 mas, G33's two by 10.3 mas). G24 `mosh_sun_lt_off`: Ceres
heliocentric on Moshier against the files, five dates 1900..2100, 1.9e-10
deg shipped (2.8e-6 before the fix), 1.25e-5 compat. G24, G52, G63, G84,
G77 and G33 pass.

## 107. With the files named by `SE_EPHE_PATH`, a first call that reads a file before `swe_calc()` names the path reads it along the compiled default path, and some read it twice

**Severity: Correctness (returned values: 6.17 s of delta t, 8.6e-4 deg on the Moon, 7.6e-5 deg of altitude, 3.9e-9 deg on the house cusps) and performance (files opened twice)**
**Where:** `sweph.c`: `swi_fopen()`, `open_jpl_file()`, `swe_set_ephe_path_r()` (its moon-header pre-open, now `ephe_path_read_moon()`), `swi_ephe_path_name_first_use()`; `swephlib.c`: `calc_deltat()`, `swi_get_tid_acc()`; `swedate.c`: `init_leapsec()`

With no `swe_set_ephe_path()` call, the path is the one `SE_EPHE_PATH`
names, set by the first `swe_calc()` (and, since entry 98, the first
star function). Everything that reads a file before that ran along
whatever the context held: a context not yet initialised has no path, so
`swe_deltat.txt` and `seleapsec.txt` were looked up along the compiled
default (`.:/users/ephe2/:/users/ephe/`), and the initialisation that
came after wiped what had been read, so it was read again. G106 puts the
files in a directory of links with its own `swe_deltat.txt` (70.0 s for
2000-2002, against the built-in 63.83 s at J2000), names it by
`SE_EPHE_PATH`, and calls each entry point first, twice, in a fresh
process, against `swe_set_ephe_path()` of the same directory:

| First call | Opens outside the path | Off the explicit path |
|---|---|---|
| `swe_calc_ut()` Moon | 6 | 8.6e-4 deg (the second call 0) |
| `swe_deltat()`, `swe_deltat_ex()` | 6 | 6.17 s |
| `swe_sidtime()` | 8 | 1.76e-10 h |
| `swe_houses()`, `swe_houses_ex2()` | 8, 6 | 3.9e-9 deg |
| `swe_azalt()` after the Sun | 6 | 7.6e-5 deg |
| `swe_fixstar_ut()`, `swe_fixstar2_ut()` | 6 | 2.9e-6, 1.4e-6 deg (the second call 0) |
| `swe_get_ayanamsa_ut()`, `_ex_ut()` | 6 | 2.7e-9, 2.9e-9 deg |
| `swe_lun_eclipse_how()` | 6 | 5.6e-4 (magnitude) |
| `swe_rise_trans()` Sun | 6 | (under 1e-12) |
| `swe_pheno_ut()` Moon | 6 | 7.8e-4 deg |
| `swe_utc_to_jd()` (`seleapsec.txt`) | 3 | 6.7e-5 day of UT1 |
| `swe_set_jpl_file("de200.eph")` + JPLEPH Mars | 9, the file opened twice | 5.5e-5 deg |

`swe_calc()`, the eclipse searches, `swe_heliacal_ut()` and
`swe_fixstar2_mag()` named the path before reading and pass. Upstream's
own (legacy-master: 17 of the 22 fail, the star functions without the
catalogue as in entry 98).

**Fix, as done here** (behind `#ifndef SWE_UPSTREAM_COMPAT`):

- The path is named at the first file a context opens along its path
  (`swi_fopen()` with the context's path, `open_jpl_file()` with it) or
  the first tidal term it takes from the moon file (`swi_get_tid_acc()`
  for SWIEPH), so before anything is read: `SE_EPHE_PATH` resolved, the
  moon file's header read for its DE number as `swe_set_ephe_path()`
  does, local and marked implicit as `swi_set_ephe_path_implicit()`
  marks it. Nothing is closed, as nothing is open; the tidal term of the
  request under way is left as it was resolved. The later implicit-path
  sites then find a path and do nothing.
- `calc_deltat()` initialises a context first, and `init_leapsec()` too,
  so a table is read once, into an initialised context, along its path.
  The "Please call swe_set_ephe_path()" note on a first delta t is kept.

G106 `tests/firstpath.c`: the 22 first calls, no open outside the path,
no file opened twice, the first answer equal to the second and to the
explicit path's within 1e-12: 22 pass. Before the fix 16 fail, the
compat build 17 (its star functions also lose the catalogue, entry 98),
the upstream build 17. G24 `calc_ut_first_path_off`: 0 deg shipped,
8.59e-4 deg compat. G24, G81, G82, G91, G97, G100, G102 (its open counts
0), G2 and the context gate pass. Instructions (perf, the bench
workloads): `houses` 19566.2M for 19569.6M (+0.017 %), the others +6k of
12-23G.

## 108. `swe_nod_aps()`'s osculating points turned by the frame bias twice and its EMB built with the AA's Earth-Moon ratio; `swe_get_orbital_elements()` takes the AA's GMs and gives Pluto Mercury's mass

**Severity: Correctness (`swe_nod_aps()` on the Swiss files, Moshier and DE4xx: every osculating apsis 2e-6..1.2e-5 deg off, the Moon's node 7.2e-5 deg and 1.5e-10 AU, Mars's 4.4e-4 deg, twice that with SEFLG_ICRS; on de200 the Earth's perihelion 4.2e-7 deg and 1.1e-11 AU; `swe_get_orbital_elements()`: Pluto's a 2.6e-7 relative and its perihelion 3.7e-5 deg, the Moon's 3.8e-8 and 5e-5 deg, de200's Neptune's 2.5e-3 deg, every planet's a about 2e-10)**
**Where:** `swecl.c`: `swe_nod_aps_r()`, the osculating points; `get_gmsm()`, `swe_get_orbital_elements_r()`; `sweph.c`: `swi_emb_mrat()`, `swi_gm_planet()`

Three defects, the first two left over from 105.

(a) `swe_nod_aps()` reads the body's J2000 state from `swe_calc()`
(`iflJ2000`), which already carries the frame bias on an ephemeris that
has one (DE >= 403: the Swiss files, Moshier, a DE4xx JPL file), and then
`swi_plan_for_osc_elem()` applies `swi_bias()` again for DE >= 403 unless
SEFLG_ICRS is set. Every osculating point of every body on those
ephemerides was turned by the bias a second time; with SEFLG_ICRS, whose
answer should carry no bias at all, `iflJ2000` never took the flag, so it
carried two. de200 has no bias, so G104's de200 rows agreed and hid it. Measured (G104, eight dates
-2500..2800, SEFLG_TRUEPOS): the Moon's apogee against SE_OSCU_APOG 4.3e-6
deg (DE441 JPL 6.1e-6; ICRS 8.6e-6 and 1.2e-5), its ascending node
against SE_TRUE_NODE 6.5e-5 deg and 1.5e-10 AU (an orbit inclined 5 deg
turns its node by the bias over sin i), the planets' perihelia against
their own eccentricity vectors 4e-6..1.2e-5 deg.

(b) The Earth's osculating orbit is the Earth-Moon barycentre's, built
as Earth + Moon / (EARTH_MOON_MRAT + 1), the Astronomical Almanac's ratio
1 / 0.0123000383. That is the ratio the Swiss files (`embofs`) and
Moshier (`swemplan`) make their Earth from the EMB with, so it is right
there; but a JPL file's Earth comes from its EMB with the file's own
EMRAT (`swi_pleph()`, `eh_emrat`), DE200's 81.300587, DE441's
81.30056822. On de200 the EMB was 1.1e-11 AU off and its perihelion 4.2e-7
deg; on DE441 3.3e-12 AU.

(c) `swe_get_orbital_elements()`'s `get_gmsm()` takes the AA's constants
as `swe_nod_aps()` did before 105: the Moon's G M_earth (1 + 1 /
EARTH_MOON_MRAT), 3.3e-8 above DE441's GMB, the Sun's HELGRAVCONST /
AUNIT^3, 1.8e-10 below every header's GMS, plmass's planets (de200's
Uranus and Neptune 2.5e-3 and 5e-3 off its own); and `ipl_to_elem[]`
sends Pluto to Mercury's element, so Pluto's two-body GM was the Sun's
plus Mercury's, and SEFLG_ORBEL_AA's sum of the masses inside the orbit
counted Mercury twice and Pluto never. Its EMB was built as in (b).

Whether the AA choice is deliberate: SEFLG_ORBEL_AA itself is documented
(swephprg 7.2, "sum all masses inside the orbit to be computed (method of
Astronomical Almanac)"; `swephexp.h`: "used for Astronomical Almanac mode
in calculation of Kepler elipses"), so the sum is kept. The constants are
not documented anywhere as a choice; they are the library's legacy
HELGRAVCONST and plmass. The correction factors that 105 called
deliberate, `Gmsm_factor_AA[]`, are compiled only under `TEST_ORBEL_AA`,
which nothing defines ("This is only a test feature. Orbital elements in
agreement with AA should be calculated using iflag | SEFLG_ORBEL_AA"):
they never ran, and are left as they were.

Measured (G107, eight dates, against the eccentricity vector of
`swe_calc()`'s heliocentric J2000 state with the header's GMs), upstream:
Pluto a 2.6e-7 relative, e 2.0e-7, q 2.3e-7, longitude of perihelion
3.7e-5 deg on every ephemeris (SEFLG_ORBEL_AA the same, but de200's 2.2e-8
and 3.2e-6 deg, where de200's other masses offset it); the Moon 3.8e-8,
2.9e-8 and 5e-5 deg (Swiss, Moshier), 3.5e-9 and 2.6e-6 deg (de200); on
de200 Neptune 2.7e-7, 2.6e-7 and 2.5e-3 deg, Uranus 1.2e-7 and 1.3e-4
deg, Jupiter 1.2e-9 and 1.2e-6 deg; otherwise a about 2e-10 relative, e
1.4e-10..2.4e-10, perihelion 4e-8 (Mercury) to 2.7e-6 deg (Neptune AA).
The sidereal period (dret[10]) is a^1.5 and moves 1.5 times a (Pluto
3.9e-7, 51 minutes); the tropical and synodic periods with it.

**Fix, as done here:** (a) `swe_nod_aps()` asks `swe_calc()` for the J2000
state with SEFLG_ICRS, unbiased, and passes the caller's SEFLG_ICRS to
`swi_plan_for_osc_elem()`, so the bias is applied once, there, unless the
caller asked for ICRS. (b) the EMB takes `swi_emb_mrat()`: the open JPL
file's EMRAT on SEFLG_JPLEPH, EARTH_MOON_MRAT otherwise, the ephemeris the
one the first `swe_calc()` answered with; in both functions. (c)
`get_gmsm()` takes the ephemeris's GMs as 105 did: the Moon's
`swi_gm_earth_moon()`, the Sun's GMS and each planet's GMi
(`swi_gm_sun_planet()` with no planet, new `swi_gm_planet()`), from the
JPL header when one is open, otherwise the header the DE number names; the
ephemeris `swe_calc()` answered with; Pluto its own element, in the
two-body GM and in the SEFLG_ORBEL_AA sum. After: G104's Moon apogee and
node within 1.4e-16 AU and 4.1e-11 deg of SE_OSCU_APOG and SE_TRUE_NODE on
all four ephemerides and both frames, the perihelia within 5.3e-14 AU and
5.9e-12 deg; G107's a and q within 3.7e-14 relative (a the same
expression, 0), e 3.6e-14, the longitude of perihelion 4.8e-9 deg.
Positions move by what was removed: `swe_nod_aps()` with default flags,
900..2960 (de200 1959..2027), the apsides 2e-6..9.3e-6 deg (Mars) and
3.5e-8 AU geocentric, the nodes up to 4.4e-4 deg (Mars), 3.1e-4 (Uranus),
6.9e-5 (the Moon), 5.2e-5 (Chiron); on de200 only the Earth's orbit (the
Sun's points 9.4e-7 deg). The upstream build is unchanged (both functions'
outputs byte-identical to the build before, SWE_UPSTREAM_COMPAT). Not a
hot path.

Left, recorded, not this defect:
- With the default (apparent) flags `swe_nod_aps()`'s and SE_OSCU_APOG's
  apogees still differ by 9e-5 deg: SE_OSCU_APOG fits the
  light-time-corrected Moon, `swe_nod_aps()` the geometric one and applies
  its corrections to the points. Two definitions, not a defect (105).
- The Swiss files' and Moshier's Earth is made from the EMB with the AA's
  ratio though the files were fitted with DE441's EMRAT (81.30056822, vs
  81.30055985): the Earth 3.2e-12 AU off DE441's, geocentric places about
  2e-10 deg. Below the floor.
- `swe_get_orbital_elements()`'s longitude of perihelion loses digits near
  aphelion on a near-circular orbit: the true anomaly is 2 atan(k sin E /
  (1 + cos E)), and 1 + cos E cancels: Venus 4.4e-9 deg at 1700 (E 178
  deg), Neptune 1.6e-9. Its e from sqrt(1 - p / a) loses half the digits
  of a small e (3.6e-14; Neptune's q 1.1e-12 AU). The first above the
  floor; a separate item.
- `swe_get_orbital_elements()`'s periods take Kepler's third law without
  the planet's mass, a^1.5 sidereal years: against 2 pi sqrt(a^3 / GM),
  Jupiter's 4.8e-4 (2.1 days) long, Saturn's 1.4e-4 (1.5 days). A
  modelling choice upstream; not changed.

G104 `tests/nodapsgm.c`, extended: the Swiss files, de200, Moshier and,
with SE_TEST_DE4, a DE >= 403 JPL file (the DE number and EMRAT read from
its first record, the GMs from the table by DE number); (a) the Moon's
apogee against SE_OSCU_APOG and its node against SE_TRUE_NODE, (b) the
perihelia against the eccentricity vectors, now in both SEFLG_TRUEPOS and
SEFLG_ICRS and with direction compared on every ephemeris, the EMB built
with the file's EMRAT on a JPL file. 33 rows (44 with DE441); the build
before the fix fails 23 (34), the upstream build 31 (42). G107
`tests/orbelgm.c`: Mercury..Pluto, the EMB and the Moon, two-body and
SEFLG_ORBEL_AA, on the same ephemerides; a and q within 1e-12 relative, e
within 1e-12, the longitude of perihelion within 1e-8 deg (above its
4.8e-9 residual). 60 rows (80); the build before and the upstream build
fail all 60 (80). G24 `nodaps_bias_off`: the Moon's apogee on Moshier,
SEFLG_TRUEPOS, against SE_OSCU_APOG, 1.99e-12 deg shipped, 1.65e-6 compat;
`orbel_pluto_off`: Pluto's a on Moshier against DE403's GMS + GM9, 0
shipped, 2.03e-7 compat. G94, G74, G79, G86, G69, G38 pass.

## 110. A first call warns "Please call swe_set_ephe_path()" whenever it initialises the context, with a path in effect too

**Severity: Correctness (error reporting: a warning on a correct answer; `swe_calc_orbel()`, this fork's, refused)**
**Where:** `sweph.c`: `swi_ephe_path_none()` (new), `swe_calc_r()`, `swe_calc_orbel_r()`, `get_ayanamsa()`, `fixstar_calc_from_struct()`, `swi_fixstar_calc_from_record()`, `swe_time_equ_r()`; `swephlib.c`: `calc_deltat()`; `sweph.h`

Seven sites write "Please call swe_set_ephe_path() or swe_set_jplfile()
before calling ..." into serr, each when `swi_init_swed_if_start()`
reports that this call initialised the context and the ephemeris is not
Moshier -- whether or not a path is in effect. G109 makes 21 first calls,
each in a fresh process, in three settings with a path: `SE_EPHE_PATH`
naming the files; no `SE_EPHE_PATH` with the files in the current
directory, where the default path starts (`.:/users/ephe2/:/users/ephe/`);
and the path set by another thread, which the context adopts as it
initialises. In all three:

- `swe_deltat_ex(J2000, SEFLG_SWIEPH)` answered the built-in 63.83 s, correctly, with
  the warning in serr;
- `swe_get_ayanamsa_ex_ut()` answered correctly with that delta-t warning
  in serr (return 2);
- `swe_calc_orbel()` returned -1 on it: its warning is a `goto
  return_error` (this fork's function).

The other sites did not fire in this fork, the context being initialised
before them: `swe_calc()`'s by the warning being overwritten on success,
the star functions' by entry 98's first-use step, `swe_time_equ()`'s by
its sidereal time's delta t (entry 107), and the star-mode ayanamsa's by
`swe_set_sid_mode()`, a setter. Upstream (legacy-master) fails 24 of the
84 G109 cases, `swe_calc()` too carrying the warning on its answer (return
2), with `SE_EPHE_PATH` and with the files along the default path.

**Fix, as done here** (behind `#ifndef SWE_UPSTREAM_COMPAT`): each of the
seven sites also asks `swi_ephe_path_none()`, which says a path is in
effect when the JPL file is open, a path was set or adopted (not implicit),
`SE_EPHE_PATH` names one (as `ephe_path_resolve()` takes it), or the
default path holds the moon file (read as the path is named, entry 107),
the J2000 planet file or the JPL file. It runs on a context's first call
only, after the initialisation test, so it costs nothing later. With no
path at all the warning is as before: `swe_deltat_ex()`,
`swe_get_ayanamsa_ex_ut()` and `swe_calc_orbel()` still carry it, and the
rest answer with the missing file's own message, as before. (Since entry
112 `swe_calc_orbel()` answers as `swe_calc()` there, and G24's second
probe is `swe_get_ayanamsa_ex_ut()`.)

G109 `tests/pathwarn.c`: with a path, every call answers with serr empty;
with none, the three still warn: 84 pass. Before the fix 9 fail, the compat
build 15 (entries 98 and 107 restored), the upstream build 24. G24
`path_warn_first_count`: 0 of 2 shipped, 2 of 2 compat. G106, G102, G82,
G90 and G24 pass.

## 106. Values read from an ephemeris file's header, and from the delta-psi and fixed-star tables, index and size arrays unchecked: a crafted file writes past `pldat[]`, `rot_back()`'s stack array, `sastnam[]` and `dpsi[]`, or reads past the JPL record

**Severity: Memory (stack and heap writes past arrays, NULL dereferences, out-of-bounds reads; a damaged file is mostly stopped by the header CRC, a crafted one is not)**
**Where:** `sweph.c`: `read_const()`, `do_fread()`, `sweph()`'s segment load, `load_dpsi_deps()`, `fixstar_cut_string()`, `search_star_in_list()`; `swejpl.c`: `fsizer()`, `state()`

Entry 104's noticed item, and an audit of every reader that takes a size
or an index from a file. Each was measured with a crafted copy of a real
file (CRC recomputed where the reader checks one), the copy in a /tmp
directory of links to the others, under ASan and UBSan:

- `read_const()` used the body numbers a Swiss file lists as `pldat[]`
  indexes, checked only against `SE_PLMOON_OFFSET`. 500 in sepl_12's list
  wrote `pldat[500]` of 18 (ASan: SEGV in `read_const()`); 1 wrote the
  Moon's constants while semo_18 was open, and the Moon's next segment
  failed "semo_18.se1 is damaged (2)"; 65000 wrote the slot a planetary
  moon's open file uses.
- The coefficient count `ncoe` was unbounded: 60 in semo_12 overflowed
  `rot_back()`'s `x[MAXORD+1][3]` on the stack (ASan: stack-buffer-
  overflow in `rot_back()`), and `get_new_segment()`'s `longs[]` and
  `do_fread()`'s 1000-byte `space[]` have the same size behind them; in
  sepm9501 the call answered (r 1048832, no error).
- The segment length, index offset and span were used unchecked:
  Mercury's `dseg` 0 gave a segment number of inf converted to int
  (UBSan), `lndx0` 0x7fffff00 overflowed `lndx0 + 3 iseg` (UBSan: signed
  overflow), and a span starting 1000 days late answered from the wrong
  segment without an error.
- A body the file does not list (Mercury's entry replaced by Venus) kept
  the constants of no file, `dseg` 0 (UBSan: inf to int), or those of the
  file read before.
- A planetary moon's or asteroid's element line opening with 30 digits:
  `strncpy()` copied 19 + 31 bytes into `sastnam[41]` (crash); the 30-byte
  `astnam` was `strlen()`'d without a terminator.
- `swejpl.c`'s `state()` bounded each body's coefficient count by 18 but
  not its offset or sub-interval count, and the file-length identity
  checks only the body with the highest offset: DE200 with the Earth-Moon
  barycentre's sub-intervals 2 -> 402 and the Moon's 8 -> -492 (15 and 12
  coefficients, the sum kept) read far past `buf[2500]` (ASan: SEGV in
  `interp()`). `fsizer()` with no positive offset read `eh_ipt[-3]`.
- `load_dpsi_deps()`: a line of fewer than ten fields left `cpos[3]`,
  `[8]` and `[9]` unset (SEGV in `atoi()`), and a table of more than
  `SWE_DATA_DPSI_DEPS` (36525) days wrote past `dpsi[]` and `deps[]`
  (ASan: heap-buffer-overflow).
- `fixstar_cut_string()` trimmed `cpos[1]` for a sefstars.txt line with
  no comma, NULL (SEGV); the catalogue that then failed to load left
  `search_star_in_list()` calling `bsearch()` on its NULL table (UBSan).

seorbel.txt's reader writes no fixed array from the file (its name is
copied bounded).

**Fix, as done here:** every such value is checked against what it indexes
or sizes before it is used, and a file that fails is refused as entry 104
refuses one: closed, its own slot cleared, the other files kept. In
`read_const()`: the file's span finite and ordered ("damaged (0r)"); every
listed body one that kind of file holds -- the planet file the
barycentre, Mercury to the solar-system barycentre; the Moon file the
Moon; the main-asteroid file Chiron to Vesta; a single-body file a number
above `SE_PLMOON_OFFSET` ("0o"); `ncoe` in 1..MAXORD + 1 ("0p"); per body
the ten constants finite, `dseg` positive, the body's span covering the
file's, at most 1e9 segments, `lndx0` non-negative and its index inside
the file, the elements within 1e8 days and 1e5 deg ("0q"; every
distributed file is far inside: `|prot|`, `|peri|` < 1.1e4, `|dprot|`,
`|dperi|` < 710, `|telem|` < 8e6); the element line's number field within
`sastnam[]` ("0d"); `astnam` terminated. `do_fread()` refuses a reordered
read larger than its buffer. `sweph()` refuses a body its open file does
not list ("does not contain body"), checked at a segment load, fresh or
warm alike, the file left open for the bodies it does list. In
`swejpl.c` every body's offset, coefficient count and sub-interval count
must lie inside the record (NOT_AVAILABLE, and the Swiss fallback with its
warning), and a file with no offsets is refused. `load_dpsi_deps()` skips
a short line and stops at 36525 days; `fixstar_cut_string()` trims only a
field that is there, and the star search does not call `bsearch()` on an
empty table (the answer, not found, as before).

All of it is outside `SWE_UPSTREAM_COMPAT`: it refuses only files no valid
ephemeris is, and changes nothing else. Every distributed file -- the 152
sepl/semo/seas files, the 32 planetary-moon files and de200.eph -- opens
and answers; 3672 fresh calls (18 bodies, four instants in each 600-year
bucket from -13200 to 17250) are bit for bit as before. So there is no
G24 case. A segment load costs a scan of the file's body list (at most
ten); `calc-swieph` in `bench` within its noise (minimum 0.598-0.626 us
before, 0.598-0.619 after, three runs each).

G103 `tests/hdrbound.c`: the 14 crafted files above, each in a child
process, the other kinds of file opened first so that a write into their
slots shows; the call that meets the crafted file must fail with a
message, and the open bodies' next segments, Mars, and after the JPL and
table cases the Swiss Mars and the Sun, must answer bit for bit as a fresh
context on the complete directory. Built with ASan and with UBSan (G25's
flags). 14 pass in both; before the fix ASan fails 10 and UBSan 13, every
case in one of them. In the compat build the memory checks hold too; its
calls after a refusal fail by entry 104's `free_planets()`. G24, G83, G88
and G99 pass.

## 112. `swe_calc_orbel()` refuses on the "Please call swe_set_ephe_path()" warning, ignores the strict fallback setting, and does not name the path itself (this fork's)

**Severity: Correctness (behaviour: a refusal where `swe_calc()` answers, an answer where it refuses)**
**Where:** `sweph.c`: `swe_calc_orbel_r()` (and through it `swe_calc_orbel_ut_r()`)

`swe_calc_orbel()` was written to do "the ephemeris setup, as swe_calc()
does it", and differed from it in three ways, each measured in G109's part
2 against `swe_calc()` of the Sun, which reads the same Earth, first call
in a fresh process:

- With no path at all, its warning was a `goto return_error`: -1, "Please
  call swe_set_ephe_path() or swe_set_jplfile() before calling swe_calc()
  or swe_calc_orbel()", where `swe_calc()` tries the default path and
  refuses for the missing file under the strict default ("Swiss (.se1)
  ephemeris is not available for jd 2451545.0000; falling back to Moshier
  would answer with lower precision. ...") or answers from Moshier under
  `swe_set_ephe_fallback(1)` (260, "SwissEph file 'sepl_18.se1' not found
  ... using Moshier eph.; "). A setter before it, such as
  `swe_set_ephe_fallback(1)` itself, or the delta t of
  `swe_calc_orbel_ut()`, initialised the context first and hid the
  refusal.
- It never applied the strict setting (`swi_refuse_downgrade()`, which
  `swe_calc_r()` applies to every answer): where the files are missing it
  answered from Moshier (260) whatever `swe_set_ephe_fallback()` said --
  with no path through `swe_calc_orbel_ut()`, and with the links' path at
  AD 1000, which they do not cover, for Swiss, JPL and no ephemeris flag.
  12 of the 24 cases fail, every strict one.
- It never named the path on first use, as `swe_calc_r()` does: in a build
  without entry 107's naming at the first file (the compat build) it never
  read `SE_EPHE_PATH`: with the strict check alone in place, the compat
  build refused its first call for the missing file with the files in
  that directory (-1, where its `swe_calc()` answers), and with the
  fallback allowed answered from Moshier, naming the compiled default
  path in its note.

**Fix, as done here**, unguarded: the warning is a warning, `main_planet()`
then deciding as for `swe_calc()`; the requested ephemeris is kept and
`swi_refuse_downgrade()` applied before the output; and the path is named
on first use with `swe_calc_r()`'s own statement, including its compat
branch. Chosen unguarded: the function is not upstream's, so there is no
upstream behaviour for `SWE_UPSTREAM_COMPAT` to restore, and the compat
build's `swe_calc()` warns and carries on too -- the compat build's
`swe_calc_orbel()` now answers as its `swe_calc()` does, part 2 passing in
both builds. No G24 case for that reason, and no `sweph.h` bullet; G24's
`path_warn_first_count` (entry 110) took `swe_get_ayanamsa_ex_ut()` for
its second probe, `swe_calc_orbel()` no longer warning in the compat
build either.

G109 `tests/pathwarn.c`, part 2: the 24 first calls, the same return and
serr as `swe_calc()`/`swe_calc_ut()`: 24 pass, 108 in all. Before the fix
12 fail; the compat build passes part 2 (its 12 failures are part 1's,
entries 98, 107 and 110 restored); the upstream build, which has no
`swe_calc_orbel()`, 24, all part 1's. G87, G106 and G24 pass.

## 111. `swe_get_orbital_elements()`: the true anomaly cancels near aphelion, e loses digits, and the periods leave out the body's mass

**Severity: Correctness (the periods: Jupiter's sidereal 2.1 days long, Saturn's 1.5, Neptune's 1.5, SEFLG_ORBEL_AA's Pluto 61 days; the time of perihelion passage 1.6 days off on Jupiter, 34 on SEFLG_ORBEL_AA's Neptune; the tropical period 3.9e-5 long everywhere; the Moon's 0.14 day. Precision: the true and mean anomaly and the perihelion longitude 4.8e-9 deg off near aphelion; e 3.6e-14)**
**Where:** `swecl.c`: `swe_get_orbital_elements_r()`

Three defects in what the function makes of the ellipse once it has a and
GM (108 fixed the GM).

(a) The true anomaly was 2 atan(sqrt((1 + e) / (1 - e)) sin E / (1 +
cos E)), with cos E = (1 - r / a) / e and sin E = r.v / (e sqrt(a GM)).
Near aphelion, E near 180 deg, 1 + cos E cancels, and on a near-circular
orbit cos E itself comes from 1 - r / a divided by a small e: the true
anomaly (dret[7]), the mean anomaly from E (dret[6]), and through the
argument of perihelion (u - v) the longitude of perihelion (dret[5]) were
off by up to 4.8e-9 deg (Venus at 1700, E 178 deg; Neptune 1.6e-9),
against the angle from the eccentricity vector to r. Above the floor.

(b) e was sqrt(1 - p / a), which keeps only half the digits of a small e:
3.6e-14 off the length of the eccentricity vector, Neptune's q 1.1e-12 AU
(3.7e-14 relative). At the floor; fixed because it sits in the same
lines.

(c) The periods. The function's documentation (swephprg 7.2) says the
elements "define the orbital ellipse under the premise that it is a
two-body system" and lists dret[10] as the "sidereal orbital period in
tropical years", dret[11] "mean daily motion", dret[12] "tropical period
in years", dret[13] "synodic period in days"; neither it nor
`doc/swisseph.htm` defines the period as Kepler's third law without mass
or as a^1.5. The code took the sidereal period as a^1.5 sidereal years
(the Moon's scaled to 27.32166 days at a = 383397772.5 m), and the daily
motion as 0.9856076686 / a^1.5 deg/day: k in degrees, a massless body's,
whose year is 365.25690 days, while the sidereal period's year was
365.25636 (the two disagreed by 1.5e-6). Neither is the two-body
ellipse's mean motion sqrt(GM / a^3) with the GM its elements were
fitted with: Jupiter's sidereal period was 4.8e-4 (2.1 days) long,
Saturn's 1.4e-4 (1.5 days), Neptune's 1.5 days, Uranus's 0.6; with
SEFLG_ORBEL_AA, whose GM sums the inner planets, Saturn's 6.7 days,
Neptune's 40, Pluto's 61. The time of perihelion passage, t - M / n
(dret[14]), moved with n: 1.6 days on Jupiter, 1.4 on Saturn, 34 on
SEFLG_ORBEL_AA's Neptune. And dret[12], 360 / (n + p) days in tropical
years of 365.242189 days, was multiplied by ysid / ytrop as well, 3.9e-5
long: a body on the Earth's mean motion came out 1.0000388 tropical years
where it is 1, Pluto's period 3.5 days long. dret[13] took the Earth's
mean motion as k, 1.5e-6 slow.

**Fix, as done here:** (a) the true anomaly is the angle from the
eccentricity vector to r about h, atan2((ev x r).h / |h|, ev.r), and E =
atan2(sqrt(1 - e^2) sin v, e + cos v): each well conditioned at every
anomaly, and in the same half-turn, so upstream's quadrant fix-ups go.
(b) e is the length of the eccentricity vector, ((v^2 - GM / r) r -
(r.v) v) / GM, as in `swe_nod_aps()`. (c) the daily motion is the
ellipse's, sqrt(GM / a^3), with the GM `get_gmsm()` gave (SEFLG_ORBEL_AA's
sum included, documented and kept by 108); the sidereal period 360 / n
days in tropical years of 365.242189 days; the tropical period 360 / (n +
p) days in the same years, without ysid / ytrop; the synodic period
against the sidereal year, 360 / (360 / 365.25636 - n); dret[14] follows
from n as before. After (G107): a, e and q the same expressions as the
eccentricity vector's (0), the perihelion longitude 1e-13 deg, the
anomalies 0, the periods 2e-16 relative, dret[14] 0. The upstream build's
output is byte-identical to the build before (SWE_UPSTREAM_COMPAT).

The Moon's period changes most and by design: the a^1.5 scaling to
27.32166 days at its mean distance made the osculating Moon's period
look like the real sidereal month, but it was not the ellipse's, whose
elements the documentation calls "particularly bad for the Moon". At
J2000 the Moon's sidereal period goes from 27.159 to 27.016 days, its
synodic from 29.341 to 29.174 (the real months, perturbed by the Sun,
are 27.322 and 29.531), dret[14] by 0.058 day. Keeping the old scaling
for the Moon alone would be a one-line exception; it is not taken, so
that dret[10], [11] and [14] describe the same ellipse for every body.

Left, recorded, not this defect:
- The Swiss files' and Moshier's Earth is made from the EMB with the AA's
  ratio though the files were fitted with DE441's EMRAT: the Earth 3.2e-12
  AU off DE441's, geocentric places about 2e-10 deg. Below the floor
  (108's notes).
- The node's distance (`rn`, the ellipse's radius at the node) still uses
  2 atan(tan(v / 2) / k), which is ill conditioned near v = 180 deg, but
  only its direction, the node's longitude, reaches dret[3]; no output
  carries the distance.

G107 extended: the true anomaly against the angle from the eccentricity
vector to r, the mean anomaly from it, within 1e-9 deg; the sidereal,
tropical and synodic periods and the daily motion against n = sqrt(mu /
a^3) within 1e-12 relative; the time of perihelion passage within 1e-6
day; the perihelion longitude's limit from 1e-8 to 1e-11 deg; e's from
1e-12 to 1e-14. 60 rows (80 with DE441); the build before this fix fails
all 60 on the periods, the upstream build all 60. G24
`orbel_period_off`: Jupiter's sidereal period on Moshier against 2 pi
sqrt(a^3 / (GMS + GM5)), 0 shipped, 4.76e-4 compat;
`orbel_anom_off`: Venus's true anomaly on Moshier at 1700 against the
eccentricity vector's, 0 shipped, 5.6e-8 deg compat (4.8e-9 with the
ephemeris's GMs). G104, G38 pass.

## 113. A seorbel.txt element field with a character no term has hangs `swe_calc()` for that body; a non-numeric epoch or equinox, an empty field, a short line are read as 0 or refuse every later body; a broken sefstars.txt is reported as "star not found", and `swe_fixstar()` with serr NULL answers an unknown star with the catalogue's last

**Severity: Correctness (a hang: `swe_calc()` never returns; wrong elements read without an error; the error that explains a failure lost; a wrong star answered OK)**
**Where:** `swemplan.c`: `check_t_terms()`, `read_elements_file()`; `sweph.c`: `swe_fixstar2_r()`, `swe_fixstar2_mag_r()`, `load_all_fixed_stars()`, `swi_fixstar_load_record()`

Entry 106's noticed items. Measured with crafted copies in /tmp:

- `check_t_terms()` reads an element field as terms separated by signs,
  each a product of numbers and powers of T. A character that is none of
  those -- "12x", "40.99837e0" (an exponent), "0.0046q", "171.4333 * x",
  "nan", "1.0833k" -- was consumed by no branch, and the loop came back to
  it for ever: `swe_calc()` for Cupido did not return (killed after 5 s;
  each of the six T-term fields). The same in upstream.
- The epoch and equinox, when not J2000/B1950/J1900/JDATE, were `atof()`:
  "2451545.0abc" read as 2451545 and an empty field as JD 0, both
  answered without an error. An empty element field was the value 0
  (Cupido answered with mean anomaly 0).
- A line with fewer than nine fields stopped the scan: with Cupido's line
  cut to five, Hades, Zeus and every later body were refused "nine
  elements required" too, and `swe_get_planet_name()` gave "name not
  found" for Hades.
- `swe_fixstar2_r()` and `swe_fixstar2_mag_r()` ignored
  `load_all_fixed_stars()`'s return: with a sefstars.txt line without a
  comma, or no sefstars.txt, Aldebaran failed "error, swe_fixstar():
  could not find star name aldebaran", the load's error ("invalid line in
  fixed stars file", "SwissEph file 'sefstars.txt' not found") overwritten.
  The built-in stars (Spica, the nakshatra stars) do not need the
  catalogue and answered, as they still do. A load that failed part way
  kept its partial table, and the next call's load dropped the pointer:
  31200 bytes leaked over three calls (LSan).
- `swe_fixstar()` and `swe_fixstar_mag()` read the file record by record
  and report a missing or damaged file themselves; but with serr NULL a
  star not found fell through to `found:` and answered OK with the file's
  last line (Gliese 710,HD168442, magnitude 9.66).

The rest of the seorbel.txt reader was audited for the same patterns: the
fields are cut with a bound (20), a line is a fixed buffer, the name is
copied into a buffer of the line's size, the geocentric flag is a
substring test; the five-character lower-casing of the epoch and equinox
ran into the next field when one was shorter (harmless: only the
equinox's and mean anomaly's first letters, which are read without case)
and now stops at the field's end.

**Fix, as done here:** `check_t_terms()` refuses a field when a pass
consumes nothing, which is exactly when the loop would not end: every
field that ended is read as before, the bad one is refused with the
caller's message naming the file, the line and the field ("error in file
seorbel.txt, line      28: mean anomaly value invalid"). Fixed in both builds:
a hang is nothing to reproduce, and no input that returned changes. Under
`SWE_UPSTREAM_COMPAT`'s `#ifndef`: the epoch and equinox must be a whole
finite number (strtod, blanks allowed) or one of the names, "invalid
epoch" / "invalid equinox"; an empty element field is refused; a short
line is one body's line, numbered as the others, and only that body is
refused; a star not found after a failed load is reported with the
load's error first, then the search's ("invalid line in fixed stars file:
'a line without a comma'; error, swe_fixstar(): could not find star name
aldebaran"); `swi_fixstar_load_record()` returns the error with serr NULL
too. The leak is fixed in both builds (the reload frees the partial
table). Every body of the distributed seorbel.txt at six instants
(-190..+3460), its names, and the catalogue's stars 1-1199 through
`swe_fixstar2()` and `swe_fixstar2_mag()` are bit for bit as before.

G111 `tests/fictstar.c`: 13 cases in forked children with a 5 s alarm --
the six T-term fields, the epoch, the equinox empty, the mean anomaly
empty, the short line; a star line without a comma, no catalogue (the
message must carry the load's error; Spica as with the complete
catalogue), and an unknown star with serr NULL through `swe_fixstar()`
and `swe_fixstar_mag()`. Cupido must fail naming file, line and field;
Hades, Zeus and Kronos answer bit for bit as from the complete file. 13
pass; before the fix 13 fail, six of them by the alarm; the compat build
passes the six and fails the other seven. G24 `fict_star_parse`: the
short line, the epoch, the empty field, the load error and the serr-NULL
answer as five bits, 0 shipped, 31 compat and before the fix. G24, G59,
G64, G91, G99 and G103 pass.

## 114. A seorbel.txt element term reads a power "T5" to "T9" as nothing, "T0" as T, "T 2" as T2 times 2, a number ".0" as 1 and "1.2.3" as 1.2, all without an error

**Severity: Correctness (a fictitious body's elements silently wrong: Cupido 1.0 deg off at J2000 for ".0 * T", 1 deg for "1 * T5")**
**Where:** `swemplan.c`: `check_t_terms()`, the T and number branches

Entry 113's noticed items. The program manual (swephprg, the seorbel.txt
section) says the elements other than epoch and equinox "may have T
terms, where T = (tjd - epoch) / 36525 ... "T * T", "T2", "T3" are also
allowed"; the code has had T4 as well. Two branches read a term's factors:

- After a T, the power was `atoi()` of the rest, taken if 0 to 4 and
  otherwise ignored. So "T5" to "T9", "T12" left the term as its other
  factors ("163.7409 + 1 * T5" read as 164.7409); "T0" was T, since
  `atoi()` of nothing is 0 and `tt[0]` is T, which bare "T" needs; and
  `atoi()` skips blanks, so "T 2" was T2 and then a factor 2.
- A number was taken only when `atof()` was not 0 or it began with '0',
  a test meant to tell a number from other text: ".0" (and "." or ".00")
  was skipped, the term read as if its factor were 1 -- "163.7409 + .0 *
  T" as "163.7409 + T", Cupido 1.0 deg off at J2000; and the run of digits
  and points was skipped whole, so "1.2.3" read as 1.2.

**Fix, as done here:** a T is followed by nothing -- the power 1, as bare
"T", "-T", "+T" always were -- or by the digits of a power from 1 to 4,
and anything else ("T0", "T5", "T12", "T2.5") refuses the field; blanks
after a T end it, so "T 2" is T times 2. A number is its whole run of
digits and points, read by `strtod()` and refused unless all of it is
one number: ".0" is 0 in every position, "1.2.3" and "." refuse the
field. The message is entry 113's, naming the file, line and field. The
shipped seorbel.txt -- every body at six instants, its speeds and names --
is bit for bit as before, so is "T" against "1 * T", "+T2" against
"1 * T2", "-T" against "-1 * T". Behind `SWE_UPSTREAM_COMPAT`'s `#ifndef`,
upstream's branches under `#else`, since these change what input that
returned reads as.

G111 extended: O11-O13 "T5", "T0", "1.2.3" must be refused naming the
field; O14-O16 Cupido's and Hades' lines replaced by two spellings of the
same elements (".0 * T" and "0.0 * T"; "T", "+T2", "-T" and "1 * T",
"1 * T2", "-1 * T"; "T 2" and "2 * T") must answer bit for bit alike, and
Zeus and Kronos as from the complete file. 19 pass; before the fix
O11-O14 and O16 fail (O15 passes, as it should); the compat build fails
12. G24 `fict_star_parse` bit 6, ".0 * T" against "0.0 * T": 0 shipped, 63
of 63 compat. G24, G59, G64 and G80 pass.

## 109. Moshier's fictitious bodies, fixed stars and apparent light time stay heliocentric, and its heliocentric Moon's speed is off its positions

**Severity: Correctness (3.0e-6 deg on every Moshier apparent fixed star and every heliocentric fictitious body, 9.2e-7 deg on Sirius without aberration; Moshier apparent distances up to 2.4e-6 AU; the heliocentric Moon's speed 4.2e-8 deg/day)**
**Where:** `sweph.c`: `app_pos_etc_plan()`, `app_pos_etc_sun()`, `app_pos_etc_moon()`, `app_pos_etc_plan_osc()`, `app_pos_etc_orbel()`, `fixstar_calc_from_struct()`, `swi_fixstar_calc_from_record()`, `mosh_sun_shift()` (new); `swemplan.c`: `swi_mosh_sun_bary()`, `mosh_series_cart()` (new), `swi_mosh_sun_accel()` (new), `swi_moshplan()`; `sweph.h`: the memo

Entry 102's remainder: Moshier's places are heliocentric, and bug 102 took
the light time about the barycentre only for the planets, the Sun and the
Moon, and only where aberration was off. Four things were left.

(a) The fictitious bodies. `app_pos_etc_plan_osc()`, and
`app_pos_etc_orbel()` for `swe_calc_orbel()`, solved the light time with
the Sun held still, v_Sun tau left out. Measured, Moshier against the
Swiss files, heliocentric, 13 dates 1800..2200: Cupido 2.88e-6 deg, Isis
2.79e-6, Vulcan 2.98e-6, and an orbit with a = 0.387 AU 2.86e-6. The
elements are heliocentric on both paths, so this is the whole difference.

(b) The fixed stars. A star's parallax was taken from the heliocentric
Earth, and its aberration from the heliocentric Earth's velocity, v_Sun
short. Against the files, J2000:
- apparent: 2.8e-6 to 2.9e-6 deg on Sirius, Spica and Polaris;
- without aberration and with SEFLG_TRUEPOS: Sirius 9.2e-7 (the Sun's
  0.005-0.01 AU from the barycentre, against a parallax of 0.38"), Spica
  2.4e-8, Polaris 1.8e-8;
- heliocentric: no parallax at all, the same values.

(c) The heliocentric Moon. Its speed was a constant 4.2e-8 deg/day off
its positions, and its distance rate 7.4e-10 AU/day off. The Moon and the
Earth were carried over the light time (about 500 s heliocentric) by a
Taylor series with the gravity model's acceleration. That model differs
from the series' own acceleration by 1e-4, the series' monthly terms and
the planets' pull. The geocentric Moon without aberration carried the
Earth the same way, which put 1.1e-8 deg/day on its speed
(-3000..+3000).

(d) The apparent light time. With aberration the light time stayed
heliocentric. The direction cancels against the aberration, but the
distance does not: it is off by rho^.v_Sun tau. Moshier's apparent
distance less its distance without aberration, against the files' same
difference, 81 dates 1800..2200, AU:

| Body | Before | After |
|---|---|---|
| Sun | 4.95e-8 | 8.9e-16 |
| Moon | 1.25e-10 | 1.46e-13 |
| Mercury | 7.13e-8 | 1.1e-15 |
| Venus | 7.34e-8 | 1.3e-15 |
| Mars | 1.25e-7 | 1.1e-15 |
| Jupiter | 1.00e-7 | 4.4e-15 |
| Saturn | 4.39e-7 | 8.9e-15 |
| Uranus | 9.99e-7 | 1.8e-14 |
| Neptune | 1.44e-6 | 2.1e-14 |
| Pluto | 2.40e-6 | 3.6e-14 |
| Chiron | 8.26e-7 | 1.4e-14 |
| Ceres | 1.80e-7 | 2.2e-15 |

**Fix, as done here:**

- **The Sun's motion.** Every Moshier path now works in entry 102's frame:
  the origin at the Sun at teval, with barycentric velocities. With
  aberration as well, not SEFLG_TRUEPOS. `mosh_sun_shift()` moves a body
  read at t by s v_Sun + s^2 a_Sun / 2.
  - The aberration-rate observer is built heliocentric, then shifted.
  - In the planets' set (`SWI_MOSH_SUN_FULL`), Pluto joins the eight
    planets: 3.4e-5 m/s, 5.5e-12 AU over Pluto's light time.
  - The fictitious bodies take the same frame: the observer's and the
    body's velocities at teval gain v_Sun, the body read at t (with an
    Earth-centred set's focus) is shifted, and swe_calc_orbel()'s centre
    is moved too.

- **The stars.** On Moshier the observer is the barycentric Earth: the
  heliocentric Earth plus the Sun's barycentric place and velocity from
  `swi_mosh_sun_bary()`. Heliocentric, the parallax is the Sun's
  barycentric place.
  - The aberration rate's observer at -dm and +dp also takes the Sun's
    motion. Its acceleration comes from `swi_mosh_sun_accel()`: the rate
    of the series' v_Sun, since a star's speed is the rate of its
    positions.
  - The point masses' Newtonian pull is 2e-12 AU/day^2 off that rate,
    because the giants' series are 1.2e-4 to 7e-3 of their acceleration
    off Newton. Used alone it gave 1.05e-10 deg/day in Polaris's right
    ascension, against G75's 6e-11 and 7.3e-12 before.
  - (1 + sum m) a_Sun = -sum m_i a_i, with the four giants' a_i taken
    one-sided from their series over 2^-9 day. The others' a_i are
    Newtonian, with the planets' pull and its indirect part.
  - This lands within 1.2e-12 AU/day^2 of the central difference of v_Sun
    across the theory's range, and gives the full difference's answers on
    G75: Polaris 2.27e-11.

- **The Moon.** Heliocentric, the Moon and the Earth are read at t, so
  the place and velocity are the series' own (entry 95). Geocentric
  without aberration, the Earth's acceleration for the carry is the
  series' (`swi_mosh_earth_accel()`, entry 66 part 3).

- **The mixed case.** With the Swiss files and the Moon's file missing,
  `sweplan()` falls back to Moshier's Moon (its `pldat` marked Moshier)
  while the Earth and the Sun stay the files'. That Earth is barycentric,
  so the frame is the files'. `app_pos_etc_moon()` keys the Moshier frame
  on the Moon's ephemeris alone, so this case took the files' Earth for
  Moshier's heliocentric one:
  - The first version of this fix read Moshier's heliocentric Earth at t
    against the files' Sun, 0.09 to 0.43 deg off heliocentric.
  - It also differenced Moshier's Earth series against the files' Earth
    for the Earth's acceleration, 0.2 to 0.96 deg off barycentric and
    2.5e-3 deg geocentric without aberration, the speed 337 deg/day.
  - G99 caught 17 heliocentric and barycentric Moon rows of these (0.15 to
    0.5 deg off the complete directory's answer).
  - Bug 102's frame already did the same with v_Sun alone: v_Sun added to
    barycentric velocities, 2.2e-6 deg, speeds 6.7e-7 deg/day.
  - Bug 53's carry took the Sun's pull on the barycentric Earth as if it
    were heliocentric, 6.9e-9 deg/day of the apparent Moon's speed.

  Now a Moshier Moon with the files' Earth (`mosh_mixed`) takes no Moshier
  frame. Its light time reads at t as the files' path does: `sweplan()`,
  which falls back to Moshier's Moon again, with the files' Earth and Sun
  there. The topocentric observer is built as on the files. It is behind
  the same guard. The mixed answers are now as far from the complete
  directory's as the Moshier Moon's own theory: 2e-6 to 4e-6 deg
  heliocentric and barycentric, 1.9e-3 geocentric. Every other answer is
  bit-identical to before (dumped), and the compat build is unchanged.

- **The cost.** The series are memoised per planet and instant
  (`mosh_series_cart()`, the same bits as `swi_moshplan()`'s own
  evaluation), so the Sun's motion and a chart's bodies share them.

Cheapest form, examined:

- **a_Sun.** It is needed on the apparent path. s^2 a_Sun / 2 is 4.3e-10
  AU over Pluto's light time.
- **The inner planets.** They cannot be dropped from v_Sun for the far
  bodies. Mercury's share alone (8 mm/s) is 1.1e-9 AU over Pluto's light
  time, and even Pluto's own share is 5.5e-12 AU. A cheaper source for
  them, mean elements, would need Mercury to 1e-3 and Venus to 1e-4 of
  their velocity. It would also add a table from outside the code. And in
  a chart their series are computed for the bodies anyway, so it would
  cost a chart more, not less. Not done.
- **The Moon's light time (1.3 s).** It needs v_Sun only to 0.11 m/s.
  `SWI_MOSH_SUN_MOON` takes Venus, the Earth-Moon barycentre (read for the
  Earth anyway) and the four giants: Mercury, Mars and Pluto left out are
  0.016 m/s, 1.5e-13 AU.
- **A fixed-grid interpolated v_Sun.** It is a pure function of t, and
  would make a scan's new instants cheap. But a chart would then pay grid
  nodes on top of the series it computes anyway, and switching between
  exact and interpolated by what is cached would make answers depend on
  call order (G-callorder). Not done.

After, against the Swiss files, 13 dates 1800..2200:

- **(a)** Cupido 6.6e-10 deg, Isis 3.0e-10, Vulcan 9.6e-10, the 0.387 AU
  orbit 8.5e-10. This is the files' Sun against Moshier's planets'
  barycentre, 6.5 mm/s, 1.2e-9 deg of v_Sun / c (2.8e-10 mid-way between
  the files' yearly Sun joins). Without aberration, the geocentric
  fictitious bodies are now as close as their apparent places, the Moshier
  Earth's own error.
- **(b)** Without aberration, TRUEPOS and heliocentric: Sirius 1.06e-10,
  Spica 1.1e-10 (3e-12 TRUEPOS), Polaris 1.9e-12. Apparent: 8.9e-9 to
  1.17e-8. The rest is the Moshier Earth's velocity theory error, 5.4 cm/s
  or 1.04e-8 deg of aberration: RESIDUAL, below the floor, not fixed.
- **(c)** The heliocentric Moon's speed is 1.9e-11 deg/day off its
  five-point rate (3.7e-11 at -3000..+3000) and its distance rate 9e-14
  AU/day. The geocentric Moon without aberration is at 2.2e-9 deg/day
  (G101's dates 1.07e-8 -> 1.48e-9; the Moon's own series). The Moon's
  heliocentric fixed point (G101) is 1.03e-10 -> 1.25e-12 deg.
- **(d)** The table above. Each apparent distance is now as far off the
  files as the one without aberration, Moshier's own theory error, from
  1.8e-7 AU (the Sun) to 2.9e-4 (Pluto). Chiron's error is 9.17e-7 ->
  3.10e-7 and Ceres's 3.63e-7 -> 3.06e-7, the whole of the asteroids'
  difference.
- **The apparent direction** moves by at most 6.6e-10 deg (-2900..+2900):
  Pluto 6.6e-10, Mercury 6.1e-10, Saturn 5.9e-10, the Sun 1.6e-10, the
  Moon 1.5e-10, topocentric the same. Speeds move by at most 9.5e-10
  deg/day (Saturn), each still the rate of its positions (Saturn 1.38e-10
  -> 1.37e-10 against five-point differences).

Cost, perf stat instructions (the machine was too loaded for wall time):

- calc-moseph +1.85% (2.2913e10 -> 2.3337e10). Every body needs v_Sun
  now, and a chart shares the series.
- calc-swieph +0.06%.
- moon +55% (1.3456e10 -> 2.0827e10). That is +153k instructions a call:
  a Moon-only scan at 0.37 day pays the five series of Venus and the
  giants at every new instant.

Per call at new instants, Moshier:

| Call | Before | After |
|---|---|---|
| Mars apparent | 667k | 884k (+32%) |
| Moon | 616k | 770k (+25%) |
| Cupido apparent | 815k | 1077k (+32%) |
| Vulcan | 1180k | 1438k (+22%) |
| A star apparent, with speed | 627k | 1024k (+63%: the nine series and the four giants' at t + 2^-9) |
| A star without speed, or without aberration | 533k | 794k (+49%) |
| Cupido heliocentric | 1112k | 1119k (bug 102 already paid) |

The 2% rule was waived for (d).

The Swiss files' barycentric Sun jumps at its yearly segment joins (up to
1.2e-8 AU). That is data, recorded in entry 102.

G108 `tests/moshbary.c`, 40 rows:
- (1) Cupido, Isis, Vulcan and a swe_calc_orbel() orbit heliocentric
  against the files, within 2e-9 deg.
- (2) Sirius, Spica and Polaris against the files: within 1e-9 deg without
  aberration, true and heliocentric, and within 3e-8 apparent.
- (3) The Moon's heliocentric speeds within 1e-9 deg/day and 1e-11 AU/day,
  and without aberration within 5e-9.
- (4) The apparent less the astrometric distance against the files'
  within 1e-12 AU, for the Sun, the Moon, Mercury to Pluto and Chiron, and
  the apparent distance rate of the Sun, the Moon, Mars and Pluto within
  1e-11 AU/day.
- (5) The mixed case, the Swiss files without semo_12.se1, 13 dates
  1300..1700:
  - the Moon heliocentric, barycentric and geocentric without aberration,
    against the light time's fixed point iterated from the same
    directory's true barycentric places: within 1e-11 deg (measured
    <= 9.8e-13), geocentric 1e-10 (1.0e-11);
  - its speeds against five-point differences: heliocentric within 1e-9
    deg/day (6.0e-12), without aberration and apparent within 2e-8 (1.0e-9,
    the Moshier Moon's series).

The build before the fix fails 30 of the first 34 rows. The upstream build
fails 23 of them: its 11 distance rows pass, both its light times held
heliocentric alike. Of (5)'s six rows:
- the build before bug 109 fails 5 (2.2e-6 deg, 6.7e-7 deg/day);
- the first version of its fix fails 5 (0.43 deg heliocentric, 0.96
  barycentric, 337 deg/day);
- the upstream build fails all 6 (bug 53's straight line, 1.5e-7 to
  3.0e-7 deg, speeds up to 1.4e-4 deg/day).

G80's Moshier fixed-point reference now moves the body by the Sun's motion
over the light time. Without that, its four Moshier rows fail by 2.5e-6 to
3.0e-6. With it they are 3.3e-14 to 2.25e-12.

The Sun's motion in the G101, G77, G52 and G33 references now takes Pluto,
as the library does.

G24 `mosh_bary_off`: Sirius without aberration and Cupido heliocentric
against the files, five dates 1900..2100. 1.2e-10 deg shipped, 3.05e-6
compat.

G108, G101, G95, G80, G87, G75, G60, G52, G84, G77, G33 and G24 (both
ways) pass, and G99 with the mixed-case fix. The compat build's output is bit-identical to before
(planets, stars, fictitious and orbel bodies, every flag set dumped).

Entry 125: the Taylor step s v_Sun + s^2 a_Sun / 2 falls short past a day
of light time, VEA's 462 days 2e-7 deg; the Sun's own place at t there
since.

## 116. `swe_set_astro_models()` of a version before 2.01 sets the tidal acceleration but publishes only the SID group (this fork's)

**Severity: Correctness (returned values: -25.936 for -25.826, 9.13 s of delta t in AD 1000)**
**Where:** `swephlib.c`: `swe_set_astro_models_r()` (and `swe_get_astro_models_r()`, which calls it)

For a version string before 2.01 the setter also sets the tidal
acceleration: SE_TIDAL_DE406 for "SE1.77" to "SE1.80" (and "SE2.00" when
the files' Sun is DE431), -25.7376 before 1.77. It does so with
`swe_set_tid_acc_r()` inside its own configuration guard, where the inner
publish is skipped, and then published only SWI_CFG_SID. The tidal group
stayed unclaimed, so on the default context the next sync -- which the
SID publish itself makes due, and which `swe_get_tid_acc()`, the next
calculation or another thread runs -- copied the master's automatic term
over it. G113, the default context against an explicit one (which does
not sync) given the same calls:

| Version | Explicit context | Default, before |
|---|---|---|
| SE1.80, 1.78, 1.77, 2.00 | -25.826, 1559.759 s | -25.936, 1568.888 s |
| SE1.72 | -25.7376, 1548.309 s | -25.936, 1564.775 s |
| SE1.70, 1.64 | -25.7376, 1578.121 s | -25.936, 1594.586 s |
| SE1.00 | -25.7376, 1456.524 s | -25.936, 1456.524 s |

(tidal acceleration, then delta t in AD 1000), right after the setting,
after a calculation, whether the path came before or after the models,
through `swe_get_astro_models()`, and on another thread.

**Audit of the other setters.** Every other publishing setter changes the
fields of its own group only: `swe_set_ephe_path()` and
`swe_set_jpl_file()` (PATH; an automatic tidal term they re-derive is not
copied by a sync, and a manual one is kept, entry 92),
`swe_set_sid_mode()` (SID, the astronomical models of
SE_SIDBIT_PREC_ORIG included), `swe_set_topo()` (TOPO),
`swe_set_tid_acc()` (TIDACC), `swe_set_delta_t_userdef()` (DELTAT),
`swe_set_lapse_rate()` (LAPSE), `swe_set_interpolate_nut()` (NUT). The
compat build's path change zeroes the models without publishing SID; that
is entry 98's restored defect.

**Fix, as done here** (behind `#ifndef SWE_UPSTREAM_COMPAT`): the setter
notes the tidal term and whether it was manual before the version branch,
and publishes SWI_CFG_TIDACC with SID when the branch changed either.

G113 `tests/modelstid.c`: nine versions in four orders: 36 pass. Before the
fix 32 fail (all but SE2.06's), the compat build 32. G24 `models_tid_off`:
0 shipped, 0.110 compat. G100, G97, G82, G2 and G24 pass.

Noticed, not fixed: `swe_set_ephe_fallback()` belongs to no configuration
group, so a second thread sharing the default context does not see it
(main thread 1, second thread 0, `SE_EPHE_FALLBACK` unset), unlike every
other setter.

## 117. `swe_calc()`'s mean node and mean apogee speeds divide by 0.001 a step that is not 0.001

**Severity: Correctness (the mean node's longitude speed up to 1.2e-8 deg/day off the rate of its positions, the mean apogee's longitude 2.5e-8 and latitude 2.1e-8, on every ephemeris and frame)**
**Where:** `sweph.c`: `swecalc()`, the SE_MEAN_NODE and SE_MEAN_APOG branches

The speed was `(x(t) - x(t - MEAN_NODE_SPEED_INTV)) / MEAN_NODE_SPEED_INTV`,
0.001 day. `t - 0.001` is rounded to a double, so the interval actually
taken is 0.001 to half an ulp of the date: 2.3e-10 day near the present,
2.3e-7 of the step. On the node's 0.053 deg/day that is up to 1.2e-8
deg/day; at binary-fraction instants the rounding is nearly the same for
every date of a binade, -7.4e-9 to -9.8e-9 at every one of 18 dates
1271-3000. The apogee
(0.111 deg/day) is off by 2.5e-8 in longitude, and its latitude, the
projection of the inclined mean orbit, by 2.1e-8 with half the
projection's curvature times the step added. Measured against five-point
differences of the positions at binary-fraction instants, the same over
2^-2 to 2^-6 day, so the speed's and not the reference's; the same on
the Swiss files, Moshier and de200, with and without SEFLG_NONUT, J2000
and equatorial (1.09e-8 / 4.6e-9 latitude equatorial for the node, 2.9e-8
/ 2.6e-8 for the apogee): the mean elements are Moshier's on every
ephemeris, and the frame rates are not where it is lost. Not the series'
time-derivative terms, the frame of date's turning or the nutation and
precession of the ecliptic: those would differ between the frames.

**Fix, as done here:** the central difference over the interval actually
taken around tjd +- 2^-5 day, as `swe_nod_aps()`'s mean points since entry
55 (truncation 6e-12 deg/day on the apogee's latitude); the position read
last, at tjd. Upstream's code is under `SWE_UPSTREAM_COMPAT`. After, at 60
dates -3000..3000: the node 1.4e-10 deg/day, the apogee 2.4e-10 (both at
-2595, the ancient positions' own noise: 5.2e-10 against a reference over
2^-6 day, 1.4e-10 over 2^-4), 1.7e-11 and 4.6e-11 1600-2160. Positions do
not move (1280 node and apogee places, eight flag sets with and without
SEFLG_SPEED, sidereal and topocentric included, bit-identical). One mean-element evaluation more per call.

G114 `tests/meanspeed.c`: SE_MEAN_NODE and SE_MEAN_APOG, of date, without
nutation, J2000 and equatorial, the Swiss files and Moshier at 24 dates
-3000..3000 and de200 at 12 dates 1600..2160, longitude and latitude speed
against five-point differences over 2^-4 day within 2e-9 deg/day
(measured <= 1.3e-10; de200 <= 3.1e-11): 480 checks. The build before the
fix fails all 480 (8.9e-9 to 2.5e-8), the upstream build all 480 too (up
to 1.4e-5 deg/day, earlier entries' frame rates not there either). G24
`mean_apog_speed_off`: the apogee's longitude speed at JD 2460000.3,
2.6e-12 deg/day shipped, 3.9e-7 compat (upstream's other frame terms with
it). G53, G54, G63 and G24 pass.

Measured with it, not fixed:
- `corr_mean_node()` and `corr_mean_apog()` interpolate their correction
  tables linearly per century, so the positions' rate jumps at each
  century boundary: within 2^-5 day of one the central difference takes
  the mean of the two slopes, as any difference must (a five-point
  reference over +-1 day there is off by up to 9.9e-9 deg/day). A kink in
  the positions themselves, upstream's model; left.

## 121. A sefstars.txt that failed to load is reread and reparsed by every later star call, a missing one searched for on every call

**Severity: Performance / files (a catalogue broken at line 500: 842 us a `swe_fixstar2()` call for 2.8, 718 us a `swe_fixstar2_mag()` call for 0.1, 1500 lines read a round of three; a missing catalogue: two fopen() a call)**
**Where:** `sweph.c`: `load_all_fixed_stars()`, `swi_fixstar_forget()`; `sweph.h`: `swe_ctx`

`load_all_fixed_stars()` marks the catalogue loaded by
`n_fixstars_records > 0`, and only a load that reaches the end of the file
sets it. A load that failed part way (a line `fixstar_cut_string()`
refuses) left it 0, so the next `swe_fixstar2()` or `swe_fixstar2_mag()`
loaded again: rewound the file, still open, and read and parsed it up to
the bad line, every call. A missing catalogue left `fixfp` NULL, so every
call searched the path for sefstars.txt and then fixstars.cat. Bug 113
gave these calls the load's error and freed the partial table; it did
not stop the reload.

Measured, `swe_set_ephe_path()` to a directory of links to `ephe/` with
sefstars.txt broken at line 500, 1000 calls after the first: Aldebaran
and Sirius alternating in `swe_fixstar2()`, 842.2 us a call (2.8 with the
complete catalogue), 0 fopen(); Spica in `swe_fixstar2_mag()` 717.6 us
(0.1); 40 rounds of three calls read 60000 lines. With no sefstars.txt:
2000 fopen() in 1000 calls of each, 4.1 and 3.3 us a call.

**Fix, as done here:** the loader is wrapped: a load that fails sets
`fixstars_load_failed` and keeps its error in `fixstars_load_err`, and
frees the partial table at once; while it is set, each later load
returns ERR with that error without touching the file, so
`swe_fixstar2()` and `swe_fixstar2_mag()` report it as bug 113 made them.
`swi_fixstar_forget()`, which runs wherever the path can change (a new
path, a thread adopting another's) and in `swe_close()`, clears it. After:
0 fopen() and 0 lines read in 40 rounds, broken or missing, each refusal
headed by the first call's load error; 0.3 us a call broken, 0.5
missing. A catalogue repaired in place is read again only after a new
path or `swe_close()`: the same contract as the complete catalogue's
table, which is not reread when the file changes either. `swe_fixstar()`
and `swe_fixstar_mag()`, which search the open file themselves and never
load the table, are unchanged.

G118 `tests/starfail.c` (fopen() and fgets() interposed, each case in a
child, crafted catalogues in /tmp): C1 broken at line 500 and C2 missing,
40 rounds after the first failure open and read nothing (before: 0 opens
and 60000 lines; 240 opens), the same error each time, built-in Spica
answering; C3 a new path and C4 `swe_close()` load the complete catalogue
again, Aldebaran bit for bit as before; C5 a repair in place is not seen
without either. The build before the fix fails C1, C2 and C5; the
upstream build all five, for bug 92 (it keeps the first path's catalogue,
so the broken one is never read). G24 `star_load_kept`: after
`swe_close()`, a failed load, then a repair in place: 0 (still refused)
shipped, 1 (reread, answered) compat. G111, G91, G66 pass.

## 119. A thread that adopts another thread's `swe_set_jpl_file()` keeps the old file's positions, and one that adopts another's `swe_set_ephe_path()` the old directory's segments (this fork's)

**Severity: Correctness (a wrong answer with a success code, on the thread that follows the shared configuration: Mercury 5.8e-5 deg, Jupiter 3.4e-6 deg with the crafted files below; in general whatever the two files differ by)**
**Where:** `sweconfig.c`: `swi_config_apply()`, its two branches for an adopted path group; `sweph.c`: `swi_adopted_files_forget()` (new)

Found in the review of PR #33's file handling. A thread on the default
context that has set no path of its own follows the master: when another
thread sets a JPL file or a path, its next calculation adopts it in
`swi_config_apply()`. The setting thread drops, in
`swi_close_keep_topo_etc()`, every position and every file's data; the
adopting thread did not.

- A new JPL file on the same path (the `jpl_only` branch, bug 103) closed
  the old JPL file and kept every position it had filled, and did not
  reset `last_epheflag`, so nothing forgot them later either.
  `jplplan()`'s Earth is cached on the instant and the ephemeris alone
  (bug 84), so the adopting thread's next JPL call at that instant took
  the old file's Earth. With de200 and de200x, a copy whose EMB
  coefficients are scaled by 1 + 1e-5: Mercury at J2000 on the adopting
  thread 271.8881245810 (de200's) for 271.8881823960, 5.78e-5 deg; the
  Sun 1.01e-5 deg; at J2000 + 0.5, computed before as well, Mercury
  5.24e-5.
- A new path (the other branch) closed the Swiss files and set
  `last_epheflag` to 0, and kept the bodies' constants and segments. The
  next calculation's switch of ephemeris keeps those too since bug 103
  (`ephe_switch_forget()`, the files being open there), and `read_const()`
  frees a body's segment only when it has a reference ellipse. So Jupiter
  to Pluto, Chiron and the main asteroids were read from the old
  directory's segment, up to 4000 days of it, while the files reopened
  from the new one: with a copy of sepl_18.se1 whose Jupiter segment at
  J2000 has one packed coefficient moved, Jupiter at J2000 25.253056935371
  for 25.253053515612 and a day later 25.295503129714 for 25.295499716142,
  3.4e-6 deg; Mars, which has an ellipse, was right.

Both are this fork's: the shared configuration is, and before bug 103 the
next calculation's switch ran `free_planets()`. The compat build answers
from the adopted files.

**Fix, as done here:** an adopted file change drops what the setter drops
on its own thread, through `swi_adopted_files_forget()`: with only the JPL
file closed, the positions (`ephe_switch_forget()`: the Swiss files stay
open, and their constants with them); with the Swiss files closed,
`free_planets()`: the positions and the bodies' constants and segments.
Neither opens or closes a file, so bug 103's no-change paths keep their 0
opens (G102). Behind `SWE_UPSTREAM_COMPAT`'s `#ifndef`; there is no
upstream code to restore under `#else`.

G116 `tests/adoptfiles.c`: in forked processes, with de200x.eph and the
crafted sepl_18.se1 built in /tmp directories of links, the main thread
sets the path and file, a second thread computes, the main thread sets
the crafted file or directory and computes, and the second thread
computes again; its return, longitude, latitude, distance and speed must
equal the main thread's within 1e-12, and the crafted file must move the
answer by more than 1e-7 deg. Mercury and the Sun on JPL at J2000 and
J2000 + 0.5, Jupiter on the Swiss files at J2000 and J2000 + 1, Saturn,
whose segment was not moved, at both as controls: 8 pass; before the fix
6 fail (the controls pass); the compat build passes 8. G24
`adopt_jpl_off`, Mercury on the adopting thread against the setting
thread's after de200 to de200x: 0 both ways, 5.78e-5 deg shipped before
the fix. G102, G91, G83, G88, G2 and check-ctx pass.

## 118. The path setter (this fork's code): a context's first tidal term opens the default path's moon file and the setter overwrites that FILE; the setters resolve the automatic tidal term before their new state; the same path again never sees a moon file installed since; the first-call probe opens the planet file twice

**Severity: Correctness (`swe_get_tid_acc()` and delta t for a JPL file the configuration no longer has, 11.3 s of delta t in AD 1000; a file opened outside the path set) / Resources (a FILE leaked per context and per first path)**
**Where:** `sweph.c`: `swi_init_swed_if_start()`, `swi_close_keep_topo_etc()`, `ephe_path_read_moon()`, `swe_set_ephe_path_r()`, `swe_set_jpl_file_r()`, `swi_ephe_path_none()`

Found by an independent review of the path setter, all in code this fork
added (bugs 101, 103, 107, 110), so the compat build has none of them.
Measured with the files linked into the current directory, which the
compiled default path's "." finds, and the path set to the ephemeris
directory:

- D1, a FILE leak. A context's initialisation resolves the automatic
  tidal acceleration (bug 101), which names the default path on first use
  (bug 107) and opens its moon file; inside `swe_set_ephe_path()`
  `swi_close_keep_topo_etc()` initialised the context that way, and
  `ephe_path_read_moon()` then opened the moon file of the path being set
  into the same slot without closing the first: 2 ephemeris descriptors
  after the first `swe_set_ephe_path()`, 1 still open after
  `swe_close()`; 100 x (`swe_ctx_new()`, `swe_set_ephe_path_r()`,
  `swe_ctx_free()`) left 100 (compat 0). A thread's first calculation
  opened ./semo_18.se1 the same way before it adopted the main thread's
  path (2 opens outside the path set).
- D4. `swi_close_keep_topo_etc()` resolved the automatic term (bug 92 kept
  it from resetting a manual one), but before the setter calling it had
  put its new state in place -- the path, the JPL DE number it publishes:
  the path, `swe_set_jpl_file("de200.eph")`, then a path without
  semo_18.se1: `swe_get_tid_acc()` -23.8946 (de200's) where a fresh
  process with that path answers -25.80; the path, de200, then a JPL file
  that is not there: -23.8946 for the path's -25.936. Delta t followed at
  its next resolution.
- D5. Bug 103's same-path fast path kept the context's state even when no
  moon file had been found: a path without semo_18.se1, the file then
  installed and the same path set again, delta t in AD 1000 was 1463.44 s
  (the default term) until a Moon was computed, then 1474.72, and
  `swe_get_tid_acc()` -25.80 for -25.936.
- D7. `swi_ephe_path_none()` (bug 110) opened the planet file at J2000, or
  the JPL file, and closed it, to learn whether a path is in effect; the
  calculation then opened it again: sepl_18.se1 opened twice for a first
  Mars with no path set and no moon file (compat once).

**Fix, as done here:** a context's initialisation resolves the automatic
term without naming the path, and names it after adopting the master's
configuration, only if there is still no path and no setter is naming one
(`path_naming_off`, set by the path setter); `ephe_path_read_moon()` also
closes a moon file it finds open before opening the path's. The setters no
longer resolve the term in `swi_close_keep_topo_etc()` but when they are
done (`setter_tid_acc_auto()`), for the path, moon file and JPL file now in
place. The same path again looks for the moon file when none was found,
which opens nothing while there is still none; with one open, nothing is
touched (G102's counts stay 0). `swi_ephe_path_none()` looks the files up
along the path with `stat()`, opening nothing. All in this fork's code,
under `#ifndef SWE_UPSTREAM_COMPAT`.

G115 `tests/pathset.c`, nine cases in children, `fopen()` interposed and
descriptors counted in /proc/self/fd: the first path (1 descriptor, 0 after
`swe_close()`), 100 contexts (0 left), a thread's first Moon (nothing
opened outside the path), the two tidal sequences and the late moon file
against a fresh process's `swe_get_tid_acc()` and delta t in AD 1000 (on a
second thread too), the same path with nothing changed (no `fopen()`) and
still without a moon file (nothing opened), a first Mars (sepl_18 opened
once). 9 pass; before the fix 7 fail. The compat build fails 3 of them
(P4, P5, P7) by the upstream behaviour it restores (threads sharing no
configuration, bug 101's -25.80 until the next delta t, bug 103's reopening
path), none by these. G24 `path_setter_118`: the 100 contexts' descriptors
and the late moon file's delta t, 0 in both builds, 2 before the fix.
G2, G24, G91, G97, G100, G102, G106 and G109 pass.

## 122. `swe_set_ephe_fallback()` belongs to no configuration group, so another thread never sees it (this fork's)

**Severity: Correctness (behaviour: a Swiss request without its file refused where the setting allows Moshier, or answered from Moshier where it refuses)**
**Where:** `sweph.c`: `swe_set_ephe_fallback_r()`, `swe_get_ephe_fallback_r()`, `swe_close()`; `sweconfig.c`, `sweconfig.h`

Every other setter publishes its group to the shared configuration, which
the other threads on the default context adopt at their next sync and a
new context inherits. `swe_set_ephe_fallback_r()` wrote the context's
field and published nothing, so the setting stayed on the thread that made
it. With `SE_EPHE_FALLBACK` unset, the main thread read 1 after
`swe_set_ephe_fallback(1)` and a second thread 0, and the second thread's
Swiss Mars at J2000 without `sepl_18.se1` returned -1 where the main
thread's returned 4 (Moshier). With the variable set, `swe_set_ephe_fallback(0)`
left the second thread answering from Moshier. A context made by
`swe_ctx_new()` after the setting took the variable's value as well. And
`swe_get_ephe_fallback()` as a thread's first call read the context before
anything had initialised it: 0 with the variable set (bug 101's shape, for
`swe_get_tid_acc()`).

**Fix, as done here** (not behind `SWE_UPSTREAM_COMPAT`: the setting, the
strict refusal it switches off and the configuration layer that shares it
are all this fork's, active in both builds, and upstream has no such call
-- the `#else` would restore this fork's defect, not upstream's behaviour,
as with entry 83): the setting is the configuration group
SWI_CFG_FALLBACK, captured, merged, adopted and inherited like the others;
the master's default for it, armed at the first publish, is the
variable's, as a fresh context's is, so a setter of another group does not
turn a thread strict. Adopting it invalidates nothing, as the setter
itself does not: the policy is read at the end of each calculation. The
getter initialises and syncs first. `swe_close()` drops the master and
the groups a context owns, but keeps the setting's value in the context
(`ctx_release()` does not reset it), so it keeps owning that group: without
this, a 1 set before `swe_close()` was read back 0 after the next
`swe_set_ephe_path()` or `swe_set_topo()`, a setting lost that the
unpublished field never lost.

G119 `tests/fbshare.c`: 28 forked scenarios with a second thread: the
variable unset and set, the setting 0 and 1, set before and after the
thread starts and before `swe_close()`; explicit contexts made before and
after the setting and set to their own value. Each reads the setting back
and asks for a Swiss Mars in a directory of links without `sepl_18.se1`:
refused or answered from Moshier by it. 28 pass. Before the fix 9 fail, the
compat build 9; with the publish but without `swe_close()` keeping the
group, 2. No compat difference, so no G24 case. G109, G113, G102, G2,
check-ctx and G24 pass.

Noticed, not fixed: `swi_config_reset()` (`swe_close()`) zeroes the master's
generation, so a thread that had synced at generation N misses the Nth
publish after the close: `swe_set_delta_t_userdef(0.001)`, a second thread
computes, `swe_close()`, `swe_set_delta_t_userdef(0.002)`, and the second
thread still answers 0.001. And a setting `swe_close()` keeps in the
context is reverted to the default by the next sync after any other
setter: `swe_set_delta_t_userdef(0.001)`, `swe_close()`, delta t at J2000
0.001 days, then after a `swe_set_topo()` 0.000739 (computed). The lapse
rate and nutation interpolation, which `ctx_release()` keeps too, take the
same path through `swi_config_apply()` (read, not measured). Fixed as entry
123, which also has the master keep this setting across `swe_close()`, so
G119's close case now expects it on the new thread too.

## 123. `swe_close()` zeroes the shared configuration's generation and empties it: a thread misses the close and the publishes after it, and a setting the close keeps is lost at the next setter (this fork's)

**Severity: Correctness (behaviour: a setting lost or never seen -- delta t, the path, the JPL file, the nutation interpolation, the lapse rate, the fallback -- on the closing thread after its next setter, on every thread started after the close, and on a running thread after the close and the publishes that follow it)**
**Where:** `sweconfig.c`: `swi_config_reset()`; `sweph.c`: `swe_close()`; `sweconfig.h`

Entry 122's noticed item. `swe_close()` releases the default context
(`ctx_release()`), then resets the shared configuration in
`swi_config_reset()`, which zeroed the master, generation and all, and
dropped every group the context owned; the next publish refilled the master
from the defaults (bug 83).

- The generation. A thread on the default context that had synced at
  generation N compares the master's with N, and after the close the master
  counted from 0 again: `swe_set_delta_t_userdef(0.001)`, a second thread
  computes, `swe_close()`, the path and `swe_set_delta_t_userdef(0.002)`,
  and the second thread still answered 0.001. The path, the lapse rate, the
  fallback, the observer, the sidereal mode and the tidal acceleration did
  the same, each set to a second value with as many publishes as before the
  close. The close itself, a generation of 0 ("nothing published"), reached
  no running thread: it kept the closed session's observer (a topocentric
  Moon at 222.988 deg where the main thread is refused "geographic position
  has not been set"), sidereal mode (Lahiri's 23.857 deg for the default
  24.740) and tidal acceleration (-23.8946 for -25.80: delta t in AD 1000
  1305.30 s for 1474.72 s).
- The kept settings. `ctx_release()` keeps the path, the JPL file's name,
  delta t, the nutation interpolation, the lapse rate and the fallback
  setting, as upstream's `swe_close()` keeps the first five, and resets the
  observer, the sidereal mode and models, the tidal acceleration and the
  path's DE numbers. The emptied master put the kept ones back to their
  defaults at the closing context's next sync after a setter of any other
  group -- `swe_set_topo()`, `swe_set_sid_mode()`, `swe_set_tid_acc()` or
  the path again: delta t 0.001, a calculation and `swe_close()` gave delta
  t at J2000 0.001 days, and after the setter 0.000739 (computed); the path
  went back to the compiled default, so a Swiss Mars at J2000 was refused
  ("not available"), and every calculation on the files with it; the JPL
  file to the default name, its Mars refused; the nutation at J2000 + 0.37
  was exact (-13.927054") where interpolated (-13.926980"); the Sun from
  2000 m 14 min before its true rise was refracted to -1.066 deg, at the
  default lapse rate, where 0.05 K/m leaves it below the dip at its true
  -1.965. Bug 122 had the closing context keep owning the fallback group, so
  the context kept that one, but the master lost it too. A thread started
  after the close found the generation 0 and adopted nothing: no path, delta
  t computed, the interpolation off, the default lapse rate, the variable's
  fallback setting, in every case.

Both are this fork's: upstream has no shared configuration, and its
`swe_close()` keeps those settings on its thread. The compat build had both,
the master being active in both builds.

**Fix, as done here** (not behind `SWE_UPSTREAM_COMPAT`: the master and its
reset are this fork's and active in both builds, so the `#else` would
restore this fork's defect, not upstream's behaviour, as with entries 83 and
122): `swi_config_reset()` resets in the master exactly what `ctx_release()`
resets in the context -- the observer, sidereal and tidal groups to the
defaults, the path's two DE numbers to 0 -- and keeps the rest: the path and
JPL file names, delta t, the nutation interpolation, the lapse rate and the
fallback setting. It bumps the generation instead of zeroing it (a master
nothing was ever published to stays at 0, untouched), so every thread on the
default context takes the close at its next calculation and sees every
publish after it. The closing context gives up its ownership of the three
groups the close reset and keeps the rest, which replaces bug 122's special
case in `swe_close()`. Of the two readings -- the master keeping what the
close keeps, or the master reset and only the closing context keeping it,
as bug 122 had it -- this is the first, because the manual has
`swe_close()` "release all resources (open files and allocated memory)"
and has the parameters of `swe_set_topo()` and `swe_set_sid_mode()` survive
it: it describes no setting being reset, so a setting the close keeps stays
the library's, for the other threads and the contexts made after it as for
the closing one. The second would turn a kept setting into the closing
thread's private copy that no thread or context started after the close
sees; G119's close case asserted that for the fallback setting and now
expects the setting on the new thread. A running thread that had adopted a
group the close resets adopts the reset too, as the closing thread has it.
`swi_config_sync()`, the hot path, is unchanged.

G120 `tests/closekeep.c`, each scenario in a forked process. A: each kept
setting -- the path, de200.eph, delta t 0.001, the interpolation on, the
lapse rate 0.05, the fallback 1 -- after the path, then a Swiss Sun,
`swe_close()`, and the path again, `swe_set_topo()`, `swe_set_sid_mode()`,
`swe_set_tid_acc()` or nothing; the main thread and a thread started then
observe it (the Mars and its file, the JPL Mars, delta t, the nutation at
J2000 + 0.37, the Sun below the dip, the setting read back) and must answer
as a fresh process given the same calls without `swe_close()`, within 1e-9:
28 scenarios. B: each of the eight groups set, a second thread computes,
then `swe_close()` and nothing, or the path and the group's other value; the
second thread must answer as the main thread: 16 scenarios. 44 pass in both
builds. Before the fix 38 fail: all 28 of A (the main thread after a setter
for every kept setting but the fallback, the new thread in every one) and
10 of B (7 with the other value, 3 with nothing). The compat build 39: the
interpolation set off again also failed there, where in the default build
the interpolation setter's own first sync (bug 83) had already put the main
thread's setting back to off, so setting it published nothing and no
generation collided. G119 with its close case changed passes 28 in both
builds (2 fail before the fix, the new thread taking the variable's value).
No compat difference, so no G24 case. G113, G100, G97, G82, G2, check-ctx
and G24 pass.

Noticed: `swe_set_interpolate_nut()` resets the interpolation's nodes and
not the nutation cached on the instant alone, nor does a thread adopting
the change: interpolation on, the nutation at J2000 + 0.37, then off, and
the same instant again answers the interpolated -13.926980" (7.42e-5" from
the exact value; 8.38e-5" in the compat build, where the setter is
upstream's). G120's second thread therefore syncs at another instant.
Fixed as entry 126. And the manual says the parameters of
`swe_set_topo()` and `swe_set_sid_mode()` survive `swe_close()`, where
upstream's code and this fork's reset both: after the close a topocentric
Moon is refused "geographic position has not been set" and the ayanamsa is
the default's 24.740 deg for Lahiri's 23.857. Fixed by entry 127: both
survive, in the context and the master.

## 126. `swe_set_interpolate_nut()` resets the interpolation's nodes and not the nutation, observer and positions cached at an instant, nor does a thread adopting the change: the old setting answers there

**Severity: Correctness (behaviour: a setting without effect at an instant already computed; dpsi 7.42e-5", deps 3.13e-4", every place of date there 7.4e-5", the topocentric Moon 7.5e-5")**
**Where:** `swephlib.c`: `swe_set_interpolate_nut_r()`; `sweconfig.c`: `swi_config_apply()`, new `swi_invalidate_nut_interp()`; `sweconfig.h`

Entry 123's noticed item. With the interpolation on, `swi_nutation()`
answers from a quadratic through three nodes a day apart, built at the
first instant outside them (and exact there). `swe_set_interpolate_nut()`
reset those nodes and nothing else, but three caches hold the nutation of
an instant under whichever setting computed it:
- `swi_check_nutation()` keeps it in `ctx->nut` -- the angles, their matrix
  and (this fork's) their rate -- keyed on the instant, the speed flag and
  the JPLHOR bits; every place of date reads it, and
  `swe_get_ayanamsa_ex()` takes it when it holds the instant;
- the topocentric observer (`topd`) is kept for the instant, its apparent
  sidereal time carrying the nutation;
- the positions in `pldat`, `nddat` and `savedat`, kept for the instant
  and the flags.

Interpolation on, `SE_ECL_NUT` at J2000 and at J2000 + 0.37 (interpolated
from J2000's nodes), off, and J2000 + 0.37 again answered the interpolated
dpsi -13.926980" for the exact -13.927054" (7.42e-5") and deps -5.776802"
for -5.777114" (3.13e-4"); the longitudes of Mars, the true node and Spica
and the ayanamsa 7.42e-5" off, the topocentric Moon's 7.53e-5". Off and
then on, the exact values stayed where a fresh process interpolates
(7.39e-5", the Moon 7.51e-5"). A thread on the default context that adopts
the change through the shared configuration (`swi_config_apply()`,
`SWI_CFG_NUT`) reset the same nodes and no more, with the same numbers, and
an explicit context through `swe_set_interpolate_nut_r()` the same.
`swe_sidtime()`, the houses and a sidereal place were right: the first two
take the nutation afresh, so the setting reaches them, and the third has
none. The compat build, where the setter is upstream's code, is 8.38e-5"
off in dpsi.

**Fix, as done here:** `swi_invalidate_nut_interp()` empties what depends
on the setting: the interpolation's nodes, `ctx->nut` and `nutv`, the
observer (`topd.teval`) and the positions (`swi_force_app_pos_etc()`). The
setter calls it in place of its node resets, behind `#ifndef
SWE_UPSTREAM_COMPAT` with upstream's resets under `#else`.
`swi_config_apply()` calls it when it adopts a changed setting, in both
builds: the adoption is this fork's, so an `#else` there would restore this
fork's defect, not upstream's behaviour (as with entries 83, 122 and 123).
Kept: the mean obliquity (`oec`, `oec2000`), `calc_nutation()`'s memo and
grid and the sidereal-time memos, whose values are exact either way. Each of
the three emptied is needed: with the observer left the topocentric Moon
stays 1.49e-5" off; with the positions left Mars, the node and the Moon
7.4e-5"; with `ctx->nut` left the ayanamsa and Spica too. No hot path is
touched (the setter, and the adoption only on a change); the golden
transcript is bit for bit the tip's (13000 rows).

G122 `tests/nutintp.c`, each run in a forked process: the path, an
observer and True Citra set, the interpolation A; the probe at J2000 and
J2000 + 0.37 and a topocentric Moon at J2000 + 0.37; the interpolation B;
`swe_sidtime()` at J2000, the probe at J2000 + 0.37. The answer must equal,
bit for bit with its return flag, a fresh process's that set B first and
made the same calls after it. Nine probes (`SE_ECL_NUT`, Mars and the true
node with speeds, the topocentric Moon, the ayanamsa, Spica,
`swe_sidtime()`, Placidus cusps, the sidereal Mars), both directions, on
the setter's thread, on a second thread on the default context that adopts
the change, and on an explicit context: 54 pass in the default build.
Before the fix 36 fail (all but sidereal time, the houses and the sidereal
place, in every mode and direction). The compat build fails 22, the
setter's thread and the explicit context; the second thread passes there,
its adoption fixed in both builds. G24 `nut_intp_off`: dpsi at J2000 +
0.37 after on, J2000, J2000 + 0.37, off, against the exact value: 0
shipped, 8.38e-5" compat. G120, G29, G32, G57, G2 and check-ctx pass; G1's
transcript is identical to the tip's.

Noticed, not fixed: the nodes are kept across a change of nutation model.
`swi_invalidate_models()` empties `ctx->nut` and not the nodes, and
`swe_close()` resets the models and keeps the interpolation and its nodes:
interpolation on, the nutation at J2000, `swe_set_astro_models("0,0,0,1")`
(IAU 1980), and dpsi at J2000 + 0.37 is the default model's -13.926980"
where a process that set IAU 1980 first answers -13.918307" (8.7e-3");
after IAU 1980, J2000 and `swe_close()`, the default model's nutation at
J2000 + 0.1 and J2000 + 0.37 answers IAU 1980's -13.918277" for
-13.927012". Both builds. Fixed by entry 129, with the topocentric
observer, which a change of model left too.

## 120. With the Swiss Moon file missing, the Moon and the Earth come from Moshier under a SEFLG_SWIEPH flag, and the strict setting answers

**Severity: Correctness (behaviour: an answer where the strict setting refuses, and a flag that names the wrong ephemeris; the Moon 0.6-5.0" off the files' at 1338-1776, a planet seen from that Earth 3.4e-8 deg)**
**Where:** `sweph.c`: `sweplan()`, `swe_calc_r()`, `swe_calc_orbel_r()`, `fixstar_calc_from_struct()`, `swi_fixstar_calc_from_record()`, `fixstar_obs_rate()`

Found by an independent review. With a semo file missing (semo_12.se1,
1200-1800) and its planet file present, `sweplan()` reads the Moon, finds
the file not open, writes "using Moshier eph. for moon" into serr, takes
`swi_moshmoon()`'s Moon and makes the Earth from the EMB less that Moon.
Nothing else is told. `swe_calc()` of the Moon at 1500 returned 258
(SEFLG_SWIEPH | SEFLG_SPEED), its place within 1.1e-10 deg of the
SEFLG_MOSEPH Moon's. Mars, the Sun, a fixed star and an orbit about the
Earth were seen from that Earth and returned SEFLG_SWIEPH too. Since the
flags said Swiss, `swi_refuse_downgrade()` saw no substitution, and this
fork's strict default answered all of them. A planet file missing is
refused there (the whole body goes to Moshier and the flag says so). Both
builds behave this way, and so did the tree before this PR.

**Fix, as done here:**
- `sweplan()` marks the context when a read takes Moshier's Moon
  (`ctx->moshmoon_used`, cleared at the start of each `swe_calc_r()`,
  each star read and each `swe_calc_orbel_r()`). It also keeps the
  instant of a saved Earth made so (`ctx->moshmoon_earth_t`), so an Earth
  reused from the save area is marked again.
- Under the strict setting the answer is refused ("Swiss (.se1) Moon file
  is not available for jd ...; the Moon, and the Earth made from it,
  would come from Moshier ...") when it depends on that Moon:
  - the Moon and the Earth themselves;
  - any body seen from the Earth;
  - a star unless heliocentric;
  - an orbit about the Earth, or one seen from it.
- What only reads the Earth on the way still answers, bit for bit as with
  the complete directory: a heliocentric or barycentric planet, the
  barycentric Sun, a heliocentric star, the mean node. The refused answer
  is not left in `swe_calc_r()`'s save area.
- With the fallback allowed, the Moon's flag is SEFLG_MOSEPH, the
  ephemeris that answered, as `swephexp.h` promises ("The ephemeris
  actually used is reported in the return flag either way") and as a
  planet's full fallback does. `swe_calc_ut()` then takes Moshier's delta
  t for it: before, the files' delta t with Moshier's Moon, 1.9e-3 deg off
  the SEFLG_MOSEPH Moon at 1338.
- A planet, the Sun, a star or an orbit keeps SEFLG_SWIEPH with serr's
  note. Its body and the EMB are the files', and only the Earth's Moon
  offset, 1/82 of the Moon's 0.6-5.0", is Moshier's: 3.4e-8 deg at most from
  the complete directory's answer. A single ephemeris bit cannot say
  "files, with an Earth 1/82 Moshier", and Swiss is the true answer for
  the body asked. The true node already goes to Moshier itself and says
  so.
- No cost: calc-swieph 0.578 s against 0.584-0.591 s before, minimum the
  same, checksum identical; calc-moseph untouched.

G117 `tests/moonfile.c`: a directory of links in /tmp without
semo_12.se1, four instants 1338..1776, 372 checks against the complete
directory:
- (a) strict refuses the Moon (Swiss, with no ephemeris flag, through
  `swe_calc_ut()`, heliocentric), the Sun, Mars geocentric and
  topocentric, the heliocentric Earth, the true node, both star functions
  and an orbit about the Earth. It does so fresh and in a warm context
  after a heliocentric Mars, each asked twice.
- (b) strict answers the independent calls bit for bit, fresh and warm.
- (c) with the fallback allowed, the Moon says SEFLG_MOSEPH and is that
  Moon within 1e-9 deg; the others say Swiss, note Moshier, and are within
  1e-6 deg.
- (d) the complete directory answers each with the flag asked.
- None fail. The build before the fix fails 152 (44 of (a), its 88 warm,
  20 of (c)'s Moon); the SWE_UPSTREAM_COMPAT build 156.

G24 `moshmoon_strict_ret`: the Swiss Moon at 1500 Jan 5 without
semo_12.se1, strict, its return: -1 shipped, 258 compat. G24
`moon_neighbour_off` (entry 100) now allows the fallback for its two
calls, since the strict setting refuses the Moshier Moon it compares; 0
both ways as before. G99, G109, G88 pass.

Two defects of the fallback path were seen here and left (not this
entry's):
- **The heliocentric Moon is wrong.** A Swiss heliocentric Moon whose
  Moon is Moshier's is 0.29 deg off the files' and the SEFLG_MOSEPH one
  (1776: 4.679 against 4.392 deg). The Moshier branch of
  `app_pos_etc_moon()` forms it heliocentric while the observer is the
  files' barycentric Sun. Fixed by bug 109's mixed-case commit, which this
  entry's branch predated: G108 (5) holds it to 1e-11 deg.
- **The note is lost.** A topocentric call with SEFLG_SPEED loses serr's
  "using Moshier eph. for moon" note. Fixed since: `swi_get_observer()`
  computed its delta t with the call's serr, and `swe_deltat_ex_r()` empties
  its serr first; the second observer, after the files are read, wiped the
  note (Mars at 1500 without semo_12, fallback allowed: 0x8102, empty serr).
  Its message is appended instead; G117's topocentric Mars row now requires
  the note.

## 115. A calculation on the default context (this fork's) adopts another thread's setter half-way through: its files closed under it (a crash on the JPL file), one answer from two configurations

**Severity: Correctness (a crash: SIGSEGV in `swejpl.c`'s `state()`; wrong answers, no error)**
**Where:** `sweconfig.c`: `swi_config_sync()`, new `swi_config_hold()` / `swi_config_release()`; `sweph.c`: `swi_init_swed_if_start()`, the calculation entry points; `swecl.c`: `swe_nod_aps_r()` and its neighbours

Found by a stress program (scratch, not in the tree): one thread drives the
process-wide setters through a cycle -- three paths (the files; the files
with another sefstars.txt; an empty directory), `swe_set_jpl_file()`
between de200.eph and a missing file, a manual and the automatic tidal
acceleration, `swe_set_astro_models()` -- while three threads calculate on
their default contexts and every answer is compared with a fresh explicit
context replaying the same calls. In 0.3 s a follower died in `state()`
(`js` NULL), always in a topocentric JPL Jupiter by `swe_calc_ut()`; with
that call left out, 412 answers of 5166 matched no state of the cycle, up
to 2.6e-5 deg (a sidereal Moshier Venus), 5.4e-6 deg on a topocentric
Moon.

The default context adopts the shared master at `swi_init_swed_if_start()`
-- the hook Phase 2 hangs off -- and a calculation passes it several
times, not once: `swe_calc_ut()` at its delta t and again at `swe_calc()`;
a topocentric `swe_calc()` from inside itself, at the observer's
`swe_deltat_ex()` and `swe_sidtime0()`; a star at the Earth's and the
Sun's `swe_calc()`. A setter another thread called in between was adopted
there: `swi_config_apply()` closed the JPL file (another thread's
`swe_set_jpl_file()`), the Swiss files and the star catalogue the
calculation was reading, and emptied the caches it had filled, and the
rest of the call ran on the new configuration. Where it crashed, the next
`swi_pleph()` read the file just closed.

Measured deterministically by G112, whose calculations wait on a FIFO for
the file they read between their entry and their first nested sync while
the main thread calls the setter: a topocentric Moshier fictitious body,
`swe_set_astro_models()` mid-call, 4.3e-6 deg off in latitude; the same on
de200, `swe_set_jpl_file()` to a missing file, 4.3e-6 (the file closed and
reopened half-way); through `swe_calc_ut()` in AD 1000,
`swe_set_tid_acc(-20)`, 4.8e-6 deg/day; Sirius by `swe_fixstar2_ut()`,
`swe_set_astro_models()`, 1.1e-5 deg.

**Fix, as done here:** the calculation entry points -- `swe_calc_r()`,
`swe_calc_ut_r()`, `swe_calc_orbel_r()` and `_ut_r()`, the six star
functions, `swe_calc_pctr_r()`, `swe_get_ayanamsa_ex_r()` and `_ut_r()`,
`swe_nod_aps_r()` and `_ut_r()`, `swe_get_orbital_elements_r()`,
`swe_orbit_max_min_true_distance_r()` -- are wrappers around their old
bodies that call `swi_config_hold()` first and `swi_config_release()`
last. The hold syncs once, at the outermost entry, and counts its depth in
`ctx->cfg_hold`; `swi_config_sync()` does nothing while the depth is
above 0. A context not yet initialised was not synced by the hold: its
first `swi_init_swed_if_start()` started it and synced regardless of the
depth (nothing of it is in use), and the depth survives that start's
memset as `cfg_seen` does; what the call read before that start it read
unsynced, and the hold starts the context itself now (entry 128). The
other public functions -- the eclipse, rise and heliacal searches among
them -- were left syncing at each calculation they make, so another
thread's setter was still adopted between two of them: entry 124, where
they hold the same way. Under
`SWE_UPSTREAM_COMPAT`'s `#ifndef`: the
hold and release are no-ops and every nested sync runs, as before. The
golden transcript is bit for bit as before (13000 rows); `bench` medians
within the machine's noise (calc-swieph -4.6..+2.7%, calc-moseph
+2.0..+3.0%, minima within 1.2%). With the fix the stress program's
followers, its owner and its private context gave no wrong answer over
46200 changer calls (300 cycles) and no crash.

G112 `tests/cfghold.c`: the four FIFO cases, each in a forked child; each
answer must equal bit for bit a fresh context's with the starting
configuration. 0 in all four; before the fix 4.3e-6, 4.3e-6, 4.8e-6 and
1.1e-5; the compat build fails all four (0.41 deg/day in the longitude
speed, the de200 call refused "JPL ephemeris is not available", 1.1e-2
deg/day, 2.5e-4 in the distance speed). G24 `cfg_hold_off`: the
models case, 0 shipped, 0.413 compat. G2, G4, G12, G20, G23, G24, G81,
G82, G83, G91, G97, G99, G100, G102, G106, G109, G111 and cfgleak pass.

Not fixed here (entry 116): `swe_set_astro_models()` with a version
string before 2.01 sets the tidal acceleration and publishes only the
SID group, so the default context's next sync puts the master's automatic
one back (the stress program's inherited contexts saw it).

## 124. The searches, the houses and the other public functions that make several calculations (this fork's) adopt another thread's setter between two of them, or never

**Severity: Correctness (behaviour: wrong answers, no error)**
**Where:** every public `_r` function that makes several calculations:
`swecl.c` (17), `swehel.c` (5), `swehouse.c` (2), `sweph.c` (12),
`swephlib.c` (4), `swedate.c` (3); `sweconfig.c`: `swi_config_hold()`'s
comment

Entry 115 made the calculation entry points sync the shared configuration
once, at their outermost entry, and hold every nested sync until they
return. It wrapped only those: `swe_calc_r()`, `swe_calc_ut_r()`, the
orbital-element calculations, the six star functions, `swe_calc_pctr_r()`,
the two `swe_get_ayanamsa_ex` functions, `swe_nod_aps_r()` and `_ut_r()`,
`swe_get_orbital_elements_r()`, `swe_orbit_max_min_true_distance_r()`.
Every other public function that makes more than one calculation synced
at each of them, at each `swe_calc()`, `swe_deltat_ex()` with an ephemeris
flag, `swe_sidtime0()` it reached: the eclipse and occultation searches and
their `_how` and `_where`, rise, set and transit, the heliacal functions,
the crossings, the houses, `swe_sidtime()`, `swe_azalt()` and
`swe_azalt_rev()`, `swe_pheno()`, `swe_time_equ()` and
`swe_lat_to_lmt()`, `swe_get_ayanamsa()` and `_ut()`, the Gauquelin
sector. A setter another thread called while one of them ran on its
default context was adopted between two iterations, and the rest of the
call ran on the new configuration. Where the call had set something for
itself -- the observer an eclipse search puts in the context without
publishing -- the adopted master overwrote it too, and the search went on
at the master's site.

Measured deterministically by G121, whose calls wait on a FIFO for the
context's swe_deltat.txt, read at their first delta t, while the main
thread calls the setter (G112's technique): `swe_sol_eclipse_when_loc()` at
Zurich from 2003 May 29, `swe_set_topo()` to New York, found the eclipse of
May 31 with its maximum 3752 s late and its first contact and maximum
visible (retflag 5010 for 4242), contacts up to 4990 s off, the Sun's
azimuth 11.2 deg -- neither configuration's answer, the call naming its
site; a sidereal `swe_mooncross_ut()`, `swe_set_sid_mode()` from Lahiri to
Fagan-Bradley, 5600 s off (Fagan-Bradley's crossing); sidereal
`swe_houses_ex2()`, the same setter, 0.883 deg in every cusp;
`swe_rise_trans()` of the Moon, `swe_set_delta_t_userdef(0.001)`, 1.03 s;
`swe_sol_eclipse_when_glob()`, `swe_set_astro_models()`, 1.49e-3 s.

Some of these synced nowhere on a context already started, or only after
reading part of the configuration: `swe_deltat_ex()` with iflag -1 has no
sync, and so neither had `swe_utc_to_jd()`, `swe_jdet_to_utc()` and
`swe_jdut1_to_utc()`, which make only those; `swe_deltat_ex()` with a flag
read the user delta t before the sync in `calc_deltat()`, `swe_sidtime0()`
its models before its own, `swe_houses()` its delta t, obliquity and
nutation before the one in `swe_sidtime0()`. On a thread that had
calculated before, another thread's `swe_set_delta_t_userdef(0.001)`
stayed out of the UTC conversions for good: TT from `swe_utc_to_jd()`
22.2 s off, `swe_jdet_to_utc()`'s UTC 22.2 s, `swe_deltat_ex()` 22.6 s
with -1 and, as the first call to sync, with SEFLG_MOSEPH.

**Fix, as done here:** the same wrapper as entry 115's for each:
`swe_sol_eclipse_when_glob_r()`, `_when_loc_r()`, `_where_r()`,
`_how_r()`, `swe_lun_occult_when_glob_r()`, `_when_loc_r()`,
`_where_r()`, `swe_lun_eclipse_when_r()`, `_when_loc_r()`, `_how_r()`,
`swe_rise_trans_r()`, `swe_rise_trans_true_hor_r()`, `swe_azalt_r()`,
`swe_azalt_rev_r()`, `swe_pheno_r()`, `_ut_r()`,
`swe_gauquelin_sector_r()`; `swe_heliacal_ut_r()`,
`swe_heliacal_pheno_ut_r()`, `swe_vis_limit_mag_r()`,
`swe_topo_arcus_visionis_r()`, `swe_heliacal_angle_r()`;
`swe_houses_r()`, `swe_houses_ex2_r()`; the eight crossings
(`swe_solcross_r()`, `swe_mooncross_r()`, `swe_mooncross_node_r()`,
`swe_helio_cross_r()` and their `_ut_r()`), `swe_time_equ_r()`,
`swe_lat_to_lmt_r()`, `swe_get_ayanamsa_r()`, `_ut_r()`;
`swe_deltat_ex_r()`, `swe_deltat_r()`, `swe_sidtime0_r()`,
`swe_sidtime_r()`; `swe_utc_to_jd_r()`, `swe_jdet_to_utc_r()`,
`swe_jdut1_to_utc_r()`. Each old body is a static `<name>_body()`, and the
public function calls `swi_config_hold()` first and `swi_config_release()`
last, so no early return can leave a hold behind. The legacy entry points
already go through the `_r` functions. Left as they are:
`swe_houses_ex_r()` and `swe_lmt_to_lat_r()`, whose whole body is one call
to a held function (`swe_houses_ex2_r()`, `swe_time_equ_r()`);
`swe_houses_armc_r()`, `swe_houses_armc_ex2_r()` and `swe_house_pos_r()`,
arithmetic on the caller's ARMC and obliquity that never syncs;
`swe_refrac()` and `swe_refrac_extended()`, which take no context;
`swe_split_deg_r()`; the getters (`swe_get_tid_acc_r()`,
`swe_get_planet_name_r()`, `swe_get_ephe_fallback_r()`,
`swe_get_astro_models_r()`, `swe_get_current_file_data_r()`), one sync at
their start or none, then a read. Under `SWE_UPSTREAM_COMPAT` the hold and
release are entry 115's no-ops, and every nested sync runs as before. The
golden transcript is bit for bit as before (13000 rows). Instructions
(`perf stat`, one count each): `swe_calc()` over the planets on the Swiss
files, calc-swieph's loop, 6141191593 before and 6141191523 after;
`swe_calc_ut()` over the same, +0.016%; `swe_houses_ex2()` with speeds and
`swe_rise_trans()` of the Sun, +0.048%.

G121 `tests/searchhold.c`: the five FIFO cases above, each in a forked
child, each answer bit for bit a fresh context's with the starting
configuration, and a sixth (`entry`) for the setter made before the calls,
each of the six answers bit for bit a fresh context's with that user delta
t. 0 in all six; before the fix 4990 s (and the retflag), 1.49e-3 s,
5600 s, 1.03 s, 0.883 deg, 22.6 s; the compat build fails all six the same
way (4990 s, 1.53e-3 s, 5600 s, 1.03 s, 0.883 deg, 22.6 s). G24
`search_hold_off`: the mooncross case, 0 shipped, 5600 s compat. G2, G4,
G102, G112, G115 and G24 pass.

Not fixed here (entry 128): the hold still left a context not yet
initialised to the first `swi_init_swed_if_start()` inside the call, so
what the call read before it -- the user delta t `swe_deltat_ex()` checks
first, the model `swe_sidtime0()` reads first -- a thread's first call
read from the unstarted context.

## 125. Moshier's light time past a day carries the Sun about the barycentre by a Taylor step: VEA 2e-7 deg off its fixed point (this fork's)

**Severity: Correctness (VEA, seorbel.txt's set at 80000 AU, 2.1e-7 deg off its light time's fixed point on Moshier, its speed 5.4e-9 deg/day off its positions; a `swe_calc_orbel()` orbit at 80000 AU 1.8e-7 deg and 5.3e-9 deg/day)**
**Where:** `sweph.c`: the body read at t in `app_pos_etc_plan()`, `app_pos_etc_plan_osc()` and `app_pos_etc_orbel()`, `mosh_sun_move()` (new); `swemplan.c`: `swi_mosh_sun_bary_aside()` (new); `sweph.h`

Entry 109 carries a Moshier body read at the emission instant t = teval
+ s into the frame of the Sun at teval by the Sun's displacement s v_Sun +
s^2 a_Sun / 2 and its velocity v_Sun + s a_Sun, a Taylor step about
teval. Over a planet's light time (under 0.3 day) the third order is
5.4e-13 AU. VEA's light time is 462 days, 0.67 rad of the Sun's loop about
the barycentre (Jupiter's 12 years, 0.005 AU), and the step leaves out up
to 5.7e-4 AU of the Sun's place and 3.0e-6 AU/day of its velocity (400
dates -3000..+3000, against the same planets' sum at t).

Measured, the build before against after, 13 dates 1650..2150:
- VEA moves 2.06e-7 deg in longitude, 1.2e-8 in latitude and 4.6e-4 AU
  in distance, apparent, astrometric, heliocentric and topocentric alike.
  Against the Swiss files, 9 dates 1900..2100: heliocentric 2.7e-7 ->
  9.4e-12 deg, geocentric without aberration 2.7e-7 -> 1.4e-10, apparent
  2.7e-7 -> 8.0e-9 (the Moshier Earth's velocity, entry 109's residual).
- Its longitude speed against five-point differences of its positions:
  5.4e-9 deg/day before (G65 failed on it), 1.6e-10 after.
- `swe_calc_orbel()` takes the same step: a circular orbit at 80000 AU
  moves 1.75e-7 deg, its speed 5.3e-9 deg/day off its positions before,
  8e-11 after.

The step's error grows as s^3: 2.0e-11 AU at one day, 1.3e-9 at four,
7.9e-8 at sixteen. Seen at the distance c s that light covers in that
time, at most 6.5e-12 deg at one day, 1.1e-10 at four, 1.6e-9 at sixteen
(speeds 2e-11, 8e-11, 3e-10 deg/day). seorbel.txt's other far sets stay
under the floor: Nibiru (to 466 AU) moves 3.2e-11 deg, the Christ Comet
(862 AU) 6.7e-11, Planet 9 (644 AU) 3.9e-11, their speeds by at most
4.7e-11 deg/day. Upstream holds the Sun still over every Moshier light time
(entries 102, 109), 2.5e-6 deg on VEA: this is the residual of the fork's
fix.

**Fix, as done here:** `mosh_sun_move()` carries the body read at t. Past
one day of light time (|s| > 1, a body beyond 173 AU) the Sun's place and
velocity at t are Moshier's planets' own there, from
`swi_mosh_sun_bary_aside()` (entry 102's sum, evaluated aside of the
memos, which stay at the chart's instant): the displacement x_Sun(t) -
x_Sun(teval) and the velocity v_Sun(t). Within a day, the Taylor step as
before. The switch's jump is the step's error at a day, 6.5e-12 deg, up to
5.6e-10 deg/day in a five-point difference across it.
- The aberration rate's observer at t (xobs2) keeps the step. Its
  velocity is V(teval) - tau a(teval) by design: only tau times the
  observer's acceleration at teval enters the rate.
- The Sun's and the Moon's own light times (8 minutes, 1.3 s) are not
  touched.
- Under `SWE_UPSTREAM_COMPAT` the Sun is held still, as upstream's.

Every answer within a day of light time is bit-identical (G1's golden
output is identical to the build before's). Cost, perf stat instructions:
the calc-moseph loop of the ten planets +0.03% (1.16990e10 -> 1.17027e10).
A body past a day pays the nine series once per light-time read: VEA
1.88M -> 2.53M instructions a call with speed (+35%), Planet 9 1.66M ->
2.24M, Cupido unchanged.

G27 `tests/fictlt.c`, rebuilt and extended. Its reconstruction was out of
date: it took B and E as Moshier's heliocentric places, the Sun held
still, where since entries 102 and 109 the library solves the light time
about the barycentre (Moshier still refuses SEFLG_BARYCTR at the API), so
it failed Proserpina (0.0043") and Vulcan (0.0043") on a correct answer.
- B(t - tau) and E(t) gain the Sun's barycentric place, built from the
  public heliocentric places of the nine planets with DE431's masses (the
  Earth-Moon barycentre from the Earth and the geocentric Moon), within
  2e-13 AU of the library's own.
- B is moved from the rounded read instant jd - tau to the emission
  instant along its velocity (entry 81's rounding, 8.6e-7" on Vulcan).
- c is the library's: the old 173.1446326846693 AU/day is the AA 2006
  AU's, 6e-11 of every light time (VEA's 2.8e-8 day).
- Five dates 1725..2125, and a fourth body: a circular `swe_calc_orbel()`
  orbit at 80000 AU, seen from the Sun.
- The limit is 1e-6" (it was 1e-3"). Measured: Proserpina 1.2e-9",
  Vulcan 4.0e-9", VEA 2.9e-12", the orbel orbit 1.1e-9". The build before
  the fix fails VEA at 1.12e-3" and the orbel orbit at 1.13e-3"; the build
  before entry 109 fails the four at 7.3e-3" to 9.6e-3"; the upstream
  build fails seven of the eight checks.

G65 `tests/lunarpulleph.c`, extended with the same `swe_calc_orbel()`
orbit on the three ephemerides. Moshier VEA 5.36e-9 / 5.43e-9 -> 8.5e-11 /
8.2e-11 deg/day geocentric / topocentric, the orbel orbit 5.33e-9 / 5.41e-9
-> 8.1e-11 / 7.4e-11 (Swiss and JPL 2.5e-11 to 1.1e-10). Nibiru's, the
Christ Comet's and Planet 9's Moshier rows move in their second digit
(all <= 1.4e-10 either way); the Swiss and JPL rows do not move. Every row
<= 5.0e-10, within 2e-9. The upstream build fails all 54.

G24 `mosh_far_lt_off`: VEA heliocentric, Moshier against the Swiss files,
five dates 1900..2100: 7.95e-12 deg shipped, 2.51e-6 compat (2.71e-7 the
build before).

G27, G65, G108, G80, G52, G101, G77, G33 and G24 (both ways) pass; G108's,
G80's, G52's, G101's, G77's and G33's outputs are unchanged.

## 127. `swe_close()` resets the observer and the sidereal mode, which the manual has survive it

**Severity: Correctness (behaviour: a setting lost -- a topocentric calculation refused, "geographic position has not been set", and a sidereal one in Fagan/Bradley's mode, 0.88 deg off Lahiri's -- after `swe_close()`, on every thread and context)**
**Where:** `sweph.c`: `ctx_release()` (`swe_close()`, `swe_close_r()`), `swe_set_sid_mode_r()`, new `swi_sid_mode_models()`; `sweconfig.c`: `swi_config_reset()`; `sweph.h`, `sweconfig.h`

Entry 123's noticed item. The programmer's manual says it twice: "The
parameters set by swe_set_topo() survive swe_close()." (with
`swe_set_topo()`) and "NOTE: the parameters set by swe_set_sid_mode()
survive calls of the function swe_close()." (12.1). Upstream's
`swe_close()` resets both -- `geopos_is_set` and `ayana_is_set` FALSE,
`topd` and `sidd` zeroed -- and so did this fork's `ctx_release()`, and,
since bug 123, the shared configuration's reset after it. After the path,
`swe_set_topo(8.55, 47.37, 400)`, a Swiss Sun and `swe_close()`, the
topocentric Moon at J2000 was refused where a process that never closed
answers 222.988 deg. After `swe_set_sid_mode(SE_SIDM_LAHIRI, 0, 0)` the
ayanamsa at J2000 was Fagan/Bradley's 24.740 deg (`swe_get_ayanamsa_ut()`)
for 23.857, the sidereal Sun 0.883 deg off; a user t0 and ayan_t0 (23.857
for 24.740), `SE_SIDM_J2000` with its `SE_SIDBIT_ECL_T0` (the ayanamsa 0.000
for 24.740, the sidereal Moon in AD 1000 at 186.576 deg for 211.317) and
Lahiri with `SE_SIDBIT_PREC_ORIG` went the same way. So on the closing
thread, on a thread started after the close, on a thread that had computed
before it, and on an explicit context closed by `swe_close_r()`; with the
path set again after the close or another group's setter alike. The compat
build the same; the upstream build (legacy-master), a main-thread program:
the Moon refused, the ayanamsa 24.736 (true) for 23.853.

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`, upstream's
reset under `#else`, in `ctx_release()` and the master alike):
- `ctx_release()` keeps `geopos_is_set` with the site (`topd`'s
  geolon, geolat, geoalt) and `ayana_is_set` with `sidd` (the mode, t0,
  ayan_t0, t0_is_UT). It empties the rest of `topd`, the observer's
  computed place (`teval` to `SWI_TJD_NEVER`), which the files and the
  tidal term the close resets frame. Nothing else caches what the two set:
  there is no ayanamsa memo, and the nutation, obliquity and planets' save
  areas the close empties already. `swe_set_ephe_path()`'s and
  `swe_set_jpl_file()`'s close (`swi_close_keep_topo_etc()`) already kept
  both and empties the observer's place, so the two closes now agree.
- The models still go: the manual says nothing of `swe_set_astro_models()`
  surviving. But the models `swe_set_sid_mode()` sets for
  `SE_SIDBIT_PREC_ORIG` -- the ayanamsa's own precession and nutation --
  are a parameter it set: the setter's code moved to
  `swi_sid_mode_models()` (the setter unchanged), which the close calls on
  the kept mode after zeroing the models. Without it Lahiri with the bit
  kept its mode and lost its IAU 1976 precession: the ayanamsa in AD 1000
  5.6e-4 deg off.
- `swi_config_reset()` resets in the master the tidal group, the models
  (the `SE_SIDBIT_PREC_ORIG` ones given back from the master's kept mode)
  and the DE numbers, and keeps the observer and the sidereal mode; the
  closing context keeps owning those two groups (`SWI_CFG_CLOSED` is the
  tidal group alone). With `ctx_release()` changed alone, 54 of G123's 84
  fail: every reader on the default context, the closing thread included,
  takes the master's reset at its next sync.
- The tidal acceleration stays reset, as entry 92 has it.
- No hot path is touched: `swe_close()` and the sid-mode setter only.

G123 `tests/closesurvive.c`, each scenario in a forked process: the path,
the setting, a Swiss Sun, `swe_close()`, then nothing, the path again or
another group's setter; the main thread, a thread started then, a thread
that had computed before the close, and an explicit context given the same
calls with `swe_close_r()` must answer bit for bit as a process given the
same calls without the close. Seven settings: the observer (the
topocentric Moon at J2000 with its speed), Lahiri, a user t0 and ayan_t0,
Lahiri with `SE_SIDBIT_PREC_ORIG`, `SE_SIDM_J2000` (the ayanamsa at J2000
and in AD 1000, the sidereal Sun and Moon, the nutation, in TT), and two the
close still resets: `swe_set_astro_models("0,0,0,1")` against a process
that never set it, and Lahiri with the bit, then `"0,0,0,4"`, against
Lahiri with the bit alone. 84 checks; all pass. Before the fix 72 fail
(the five kept settings and the bit with the models after it, everywhere;
the models alone pass); the compat build 72; with the mode's models not
given back 24 (5.6e-4 deg). G24 `close_keeps_off`, the topocentric Moon and
the ayanamsa at J2000 after `swe_close()` against a fresh context given the
two: 0 shipped, 360 compat (the Moon refused). G120's comment notes the two
kept (its scenarios compare threads with each other and pass, 44); cfgleak's
test 2 used the observer as the thing `swe_close()` clears from the master
and asserted the new thread refused -- it now asserts the observer kept and
a manual tidal acceleration (DE200's) cleared. G119, G112, G82, G2,
check-ctx and G24 pass; the golden transcript is the same as before the
fix, row for row.

The manual's other `swe_close()` sentences, measured:
- "After a call of swe_close() the data will be lost. A new call of
  swe_fixstar2() will reload all stars from sefstars.txt." Holds: with
  Aldebaran's right ascension edited in the catalogue after a first call,
  `swe_fixstar2()` answers the old 69.790 deg until `swe_close()` and the
  edited 84.190 after it.
- "After swe_close(), no Swiss Ephemeris functions should be used unless
  you call swe_set_ephe_path() again and, if required, swe_set_jpl_file()."
  Advice, and the code keeps the path and the JPL file's name, as
  upstream's does (its reset of the path is commented out): a Swiss Mars
  after `swe_close()` alone is read from the files. Not changed.

Noticed, not fixed:
- After `swe_close()` without the path again, the path is kept and its DE
  numbers are not (they go with the files, the 2026-09-16 review's F2), so a
  first UT calculation before any file opens takes the default tidal term,
  -25.80 for the moon file's -25.936: delta t in AD 1000 1463.43 s for
  1474.72, and `swe_calc_ut()`'s Moon there 197.37472 deg for 197.37633
  (1.6e-3 deg); the call itself opens the file, and the same call again
  answers right. Upstream the same; the manual says to set the path again,
  which answers right at once. G123 observes in TT for this reason (a UT
  ayanamsa in AD 1000 moves by 5e-9 deg). Fixed as entry 131, which adds
  a UT ayanamsa to G123 (3.3e-9 deg off before it).
- The manual has "Once the function swe_set_tid_acc() has been used, the
  automatic setting of tidal acceleration is blocked. In order to unblock
  it again, call swe_set_tid_acc(SE_TIDAL_AUTOMATIC)", and
  `swe_close()` unblocks it too (as upstream's); `swe_set_delta_t_userdef()`,
  whose return to automatic the manual words the same way, survives the
  close. Neither sentence names `swe_close()`; left as entry 92 has it.

## 128. A thread's first call reads the configuration before its context has started (this fork's): another thread's user delta t and models missed, 22.6 s

**Severity: Correctness (behaviour: wrong answers, no error)**
**Where:** `sweconfig.c`: `swi_config_hold()`; `sweph.c`:
`swi_init_swed_if_start()`; `sweph.h`: `struct swe_ctx`; `swephlib.c`:
`calc_deltat()`, `swe_get_astro_models_r()`; `swedate.c`: `init_leapsec()`

Entries 115 and 124 made every public function that calculates sync the
shared configuration once, in `swi_config_hold()` at its entry. The hold
did not sync a context not yet initialised -- a thread's default context
before its first call -- and left it to the first
`swi_init_swed_if_start()` inside the call to start it from scratch and
sync. What the call read before that it read from the unstarted context,
all zeros: `swe_deltat_ex()` checks the user delta t before
`calc_deltat()` starts the context, `swe_sidtime0()` reads its model
first. `swe_get_astro_models()`, a getter no hold covers, listed the
models without starting the context at all, and on a context already
started read them before the sync its tidal getter makes, so its
"-amod" line kept the old models under the new ones' descriptions. The
second call was right.

Measured by a scratch program (not in the tree): 72 calls of 69 public
functions, each as a new thread's first call after the main thread set
nothing, the path alone, or the path and one of nine settings (the user
delta t, the tidal acceleration, the models, the sidereal mode, the
observer, the nutation interpolation, the lapse rate, the fallback with
an empty path, the JPL file), against a fresh process that set the same
first and made the call on its main thread, each in its own process.
After `swe_set_delta_t_userdef(0.001)`: `swe_deltat_ex()` with an
ephemeris flag and with -1 63.83 s for 86.40 s, `swe_jdut1_to_utc()`
22.6 s, and every function of UT through its delta t --
`swe_calc_ut()`'s Moon 3.1e-3 deg, `swe_sol_eclipse_where()`'s central
line at the 2003 May 31 maximum 0.238 deg in latitude,
`swe_sol_eclipse_how()` 1.9e-3 deg, `swe_lun_eclipse_how()`'s umbral
magnitude 9.9e-4, `swe_nod_aps_ut()` 1.0e-4 deg, `swe_pheno_ut()` 6.4e-5
deg, `swe_gauquelin_sector()` 3.0e-5 of a sector, `swe_calc_orbel_ut()`
2.4e-5 deg, the stars 3.7e-8 deg, `swe_get_ayanamsa_ex_ut()` 1.1e-8 deg,
the houses 2.8e-9 deg (1.3e-8 deg/day in `swe_houses_ex2()`'s speeds),
`swe_azalt()` 7.1e-10 deg, `swe_sidtime()` 6.9e-11 h, `swe_time_equ()`
2.9e-12 day. After `swe_set_astro_models("1,3,1,1,1,0,0,1")`:
`swe_sidtime0()` 3.1e-7 h (4.6e-6 deg), `swe_get_astro_models()`
"-amod0,0,0,0,0,0,0,0" for "-amod1,3,1,1,1,0,0,1". No other first call
moved: the searches, rise and set, the crossings, the heliacal
functions, `swe_utc_to_jd()`, `swe_jdet_to_utc()` and the getters that
start the context (bugs 101, 122) start it before they read, and the
other seven settings moved no first call at all. The eclipse functions'
opening `swi_set_tid_acc()` did read the unstarted context, but the
start that followed resolved the term again.

**Fix, as done here:** `swi_config_hold()`, at depth 0, starts a context
not yet initialised (`swi_init_swed_if_start()`, which syncs it) before
the call reads anything, and syncs one already started as before. The
start is the same start in the same call: the bug-118 naming of the
default path and its tidal term, the files it opens (G106 and G115 count
them) are unchanged. What the start answers is not lost: the context's
first-call warning, "Please call swe_set_ephe_path() or
swe_set_jplfile() before ...", keys on `swi_init_swed_if_start()`
answering 1 in the call, and with the hold starting first every one
inside it answered 0 -- a new thread's or a fresh process's first
`swe_deltat_ex()` with no path in effect lost its warning. The hold
marks the start (`ctx->start_unreported`), and the first
`swi_init_swed_if_start()` after it answers 1 and clears the mark, as
when that one started the context; `calc_deltat()` and `init_leapsec()`,
which call it only on a context not yet initialised, call it on a marked
one too, so the mark is taken where the start was. The first-call
warnings of 15 first calls, each followed by four others, on the main
thread and a new one, with no path, strict and with the fallback, are
word for word as before. `swe_get_astro_models_r()` starts and syncs
first, as `swe_get_tid_acc_r()` does. Explicit contexts are started by
`swe_ctx_new()` and never reach the new branch (a context made after the
setters answers the same before and after). Under `SWE_UPSTREAM_COMPAT`
the hold does nothing and the getter is upstream's, as before. With the
fix the scratch program finds no first call off. Instructions (`perf
stat`, one count each), `swe_calc()` over the planets on the Swiss files,
calc-swieph's loop: 6219228370 before, 6217798530 after.

G124 `tests/holdstart.c`: seven cases, each in its own process, a new
thread's first call after the main thread's setter bit for bit a fresh
process's: `swe_deltat_ex()`, `swe_calc_ut()`'s Moon,
`swe_jdut1_to_utc()`, `swe_sol_eclipse_where()` and `swe_houses()` after
the user delta t, `swe_sidtime0()` and `swe_get_astro_models()` after the
models; and an eighth, `warn`, that the first `swe_deltat_ex()` with no
path still warns, on the main thread and a new one. 0 in all seven and
the warning kept; before the fix 22.6 s, 3.1e-3 deg, 22.6 s, 0.238 deg,
2.8e-9 deg, 3.1e-7 h and the zeros; the compat build fails the seven
(22.6 s and the warning bug 110 restores, 3.1e-3 deg, 0.355 s, 0.238
deg, 2.8e-9 deg, 3.1e-7 h, a tidal acceleration of 0 in the models'
text, bug 101) and passes `warn`. G24 `hold_start_off`: the deltat case,
0 shipped, 22.6 s compat. G2, G20, G82, G106, G112, G115, G119, G120,
G121, cfgleak and the context test pass.

## 129. A change of nutation model keeps the interpolation's nodes of the old one, and any change of model the topocentric observer: the old model answers at an instant already computed

**Severity: Correctness (behaviour: a setting without effect at an instant already computed; dpsi 8.6e-3", every place of date there 8.7e-3", the houses 2.0e-2"; the topocentric Moon 5.4e-5" after a nutation model, 1.7e-4" after a sidereal-time model, 0.17" in AD 1000 after a delta t model)**
**Where:** `swephlib.c`: `swi_nutation()`; `sweph.h`: `struct interpol`; `sweconfig.c`: `swi_invalidate_models()`, `swi_config_apply()`

Entry 126's noticed item. With the interpolation on, `swi_nutation()`
answers from a quadratic through three nodes a day apart, built at the
first instant outside them, and kept for their instants alone. Every
route that changes the nutation model left them:
- `swe_set_astro_models()`, on the setter's thread and through
  `swe_set_astro_models_r()` on an explicit context:
  `swi_invalidate_models()` empties `ctx->nut`, the obliquity and the
  positions, not the nodes;
- a thread on the default context adopting it (`swi_config_apply()`,
  the SID group), which calls the same;
- `swe_set_sid_mode()` with `SE_SIDBIT_PREC_ORIG`, which sets the
  nutation that goes with the ayanamsa's precession (IAU 1980 for
  Lahiri's IAU 1976, Woolard for Newcomb), and the same function;
- `swe_close()`: `ctx_release()` zeroes the models and keeps the
  interpolation (entry 123) and its nodes, and the closing thread's next
  sync adopts a master whose models are zeroed the same way, so no
  invalidation runs at all; and a second thread adopting the close.

Interpolation on, the nutation at J2000, `swe_set_astro_models("0,0,0,1")`
(IAU 1980), and dpsi at J2000 + 0.37 was the default model's interpolated
-13.926980" where a process that set IAU 1980 first answers -13.918339"
(8.6e-3"); IAU 1980, the nutation at J2000, `swe_close()`, then J2000 +
0.1 and J2000 + 0.37 answered IAU 1980's -13.918277" for the default's
-13.927012" (8.7e-3"). The second thread, the explicit context and
`SE_SIDBIT_PREC_ORIG` the same, and with them every place of date there,
the ayanamsa, Spica, `swe_sidtime()` (5.3e-4 s) and the houses (2.0e-2").
Both builds.

What else a change of model reaches, measured:
- **The topocentric observer** (`topd`), kept for its instant, is placed
  by the models: its apparent sidereal time carries the nutation and the
  sidereal-time model, its UT delta t's. `swi_invalidate_models()` left
  it, and the adoption reset it for a new site or user delta t only.
  Interpolation off, the topocentric Moon at J2000 + 0.37 asked again
  after IAU 1980 nutation was 5.4e-5" off a fresh process's, after the
  IAU 1976 sidereal time 1.7e-4", at JD 2086308 (AD 1000) after Stephenson
  1997 delta t 0.17", after the IAU 1976 precession 1.9e-8". The compat
  build builds that Moon's observer afresh and is right there.
- `calc_nutation()`'s memo is keyed on the model and its grid holds the
  series' own values; both are exact after any change. The memo's key is
  the defaulted model, though, so the default (the grid) and an explicit
  IAU 2000A (the direct series) share it, and at one instant either
  answers for the other: 2.4e-12" (below the residual floor, not fixed).
- The obliquity (`oec`, `oec2000`) and `ctx->nut` are emptied by
  `swi_invalidate_models()` and zeroed by `swe_close()`; delta t's memo
  is keyed on its model, the sidereal-time memos by value.

**Fix, as done here:**
- `struct interpol` keeps the nutation model its nodes are of
  (`nut_model`, `astro_models[SE_MODEL_NUT]` as set, so the default's grid
  and an explicit IAU 2000A's series are told apart), and `swi_nutation()`
  uses them only under that model; another builds its own, whatever route
  set it. That covers `swe_close()` without touching `ctx_release()`.
  Behind `#ifndef SWE_UPSTREAM_COMPAT`, upstream's test under `#else`.
- `swi_invalidate_models()` empties the observer too, behind `#ifndef`
  (upstream's setter invalidates nothing).
- `swi_config_apply()`, in both builds, as the adoption is this fork's
  (entries 83, 122, 123, 126): a changed SID group resets the observer, as
  a new site or delta t does, and an adopted change of nutation model
  calls `swi_invalidate_nut_interp()` (the nodes, with `ctx->nut`, the
  observer and the positions). This fork's build would refuse those nodes
  by their key anyway.
- Kept: a model changed and changed back with no nutation between keeps
  the nodes, which are that model's own (they answer as in a process that
  never changed it), and a sidereal mode alone keeps them.
- No hot path is touched: the key is read only with the interpolation
  on, the rest at a setter or an adoption. The golden transcript is bit
  for bit the tip's (13000 rows).

G122 `tests/nutintp.c`, extended with 108 checks, each in a forked
process, each answer bit for bit with its return flag a fresh process's
that set the final state first: the interpolation on throughout, the path,
an observer and True Citra set, the probe at J2000 and J2000 + 0.37 and a
topocentric Moon at J2000 + 0.37, the change, `swe_sidtime()` at J2000
and the probe at J2000 + 0.37, for the nutation model from the default to
IAU 1980 and back by `swe_set_astro_models()` on the setter's thread, by
a second thread adopting it and by `swe_set_astro_models_r()` on an
explicit context; to IAU 1980 by `swe_set_sid_mode(SE_SIDM_LAHIRI |
SE_SIDBIT_PREC_ORIG)` from True Citra; from IAU 1980 by `swe_close()`, on
the setter's thread and adopted by a second thread, against a process
that set what the close keeps (the path, the interpolation); and the
interpolation off, the sidereal-time model to IAU 1976 by the first three
routes. The nine probes of entry 126 each: 162 checks in all, none fail.
Before the fix 73 of the 108 fail: every probe but the sidereal Mars in
every nutation change (and the topocentric Moon after `swe_close()`,
which has dropped the site, so both answer an error), dpsi, Mars, the
true node, the ayanamsa and Spica 8.70e-3", the topocentric Moon
8.65e-3", `swe_sidtime()` 5.32e-4 s, the houses 1.95e-2"; the
topocentric Moon after the sidereal-time model 3.12e-4". The compat build
fails 74 (92 before the fix): entry 126's 22 and 52 of these, every
nutation change but the two a second thread adopts, and after
`swe_close()` the topocentric Moon, the ayanamsa and the sidereal Mars
on the second thread too, where upstream's close drops the observer and
the sidereal mode that entry 127 keeps (69 before 127 was merged); its
sidereal-time cases pass, as above. G24 `nut_model_intp_off`: dpsi at J2000 + 0.37
after on, J2000 and IAU 1980, and at J2000 + 10.37 after J2000 + 10 and
`swe_close()`, against a fresh context's with the final model, the
larger: 0 shipped (8.68e-3" before the fix), 8.45e-3" compat. G113,
G120, G29, G32, G57, G2 and G4 pass; G1's transcript is identical to the
tip's.

Noticed, not fixed: the nodes are keyed on neither the JPLHOR flags nor
the Horizons approximation's model (`SE_MODEL_JPLHORA_MODE`), which
`calc_nutation()` reads with `SEFLG_JPLHOR_APPROX`. On de200, the
interpolation on, `SE_ECL_NUT` with `SEFLG_JPLHOR_APPROX` at JD 2433282.5
(1950) and then without it at + 0.37 answered dpsi -3.199992" for
-3.261906" (6.2e-2"); with it at both, `swe_set_astro_models()` of
`SEMOD_JPLHORA_2` between, -3.199992" for -3.303286" (0.103"). Both
builds. Fixed by entry 134, with the nodes of `SEFLG_JPLHOR`'s IERS
corrections and `swe_get_ayanamsa_ex()`'s read of the nutation kept at
its instant.

## 134. The interpolation's nodes are keyed on neither the JPL Horizons flags nor the approximation's mode, nor go with the IERS corrections, and the ayanamsa takes the nutation kept at its instant whatever flags computed it: another setting's nutation answers there

**Severity: Correctness (behaviour: a flag or setting without effect at an instant already computed; dpsi and every place of date there 6.2e-2" after the flag per call, 0.104" after a change of mode, 0.131" across the corrections; the ayanamsa 6.19e-2" with the interpolation off)**
**Where:** `swephlib.c`: `swi_nutation()`; `sweph.h`: `struct interpol`; `sweph.c`: `ctx_release()`, `swe_get_ayanamsa_ex_r_body()`; `sweconfig.c`: `swi_config_apply()`

Entry 129's noticed item. With the interpolation on, `swi_nutation()`
answers from a quadratic through three nodes a day apart, which entry 129
keyed on the nutation model. `calc_nutation()`, which builds them, reads
more: the flags' `SEFLG_JPLHOR` (IAU 1980 with the IERS corrections of
eop_1962_today.txt) and `SEFLG_JPLHOR_APPROX`, and with the latter
`SE_MODEL_JPLHORA_MODE` (`SEMOD_JPLHORA_3`, the default: IAU 1980 with a
fixed offset before 1962; `SEMOD_JPLHORA_2`: IAU 2000 with another). The
nodes answered for any flags and mode, and those of `SEFLG_JPLHOR` for
any corrections: `swe_close()` frees them and keeps the interpolation and
its nodes (entry 123), and the next `swe_set_jpl_file()` loads another
directory's. Measured on de200, the interpolation on, at JD 2433282.5
(1950):
- the flag per call: `SE_ECL_NUT` with `SEFLG_JPLHOR_APPROX` there, then
  without it at + 0.37: dpsi -3.199992" for -3.261906" (6.19e-2"), and
  the reverse; Mars, Spica and the ayanamsa the same, the topocentric
  Moon 6.22e-2";
- the mode: with the flag both times, `swe_set_astro_models()` from mode
  3 to 2 between: -3.199992" for -3.303681" (0.104"), and back; adopted
  by a second thread, by `swe_set_astro_models_r()` on an explicit
  context, and from 2 to the default by `swe_close()` (which resets the
  models), the same;
- `SEFLG_JPLHOR`, which needs a DE >= 403 and the corrections, on G19's
  synthetic DE431 header (`SE_ECL_NUT` reads none of its coefficients)
  and two fixture files: `SEFLG_JPLHOR` and then `SEFLG_JPLHOR_APPROX`,
  4.20e-2", and the reverse; with it throughout, across `swe_close()` from
  one directory's corrections to another's, the old ones' -13.531351" for
  -13.400650" (0.131", the fixtures' own difference).
Both builds.

The other caches of an instant, checked for the same flags:
- `ctx->nut` is keyed on the JPLHOR bits (`SWI_EPS_KEY`); every route
  that changes the mode empties it (`swi_invalidate_models()`), and
  `swe_close()` with the corrections. Its readers inside a calculation
  follow `swi_check_nutation()` with the same flags, but
  `swe_get_ayanamsa_ex()` took it on the instant alone: the interpolation
  off, on de200 at 1950 + 0.37, Mars with `SEFLG_JPLHOR_APPROX` and then
  the ayanamsa without it answered the flag's nutation, 83309.173544" for
  83309.111630" (6.19e-2", True Citra; Lahiri the same), and the
  reverse. Both builds.
- The obliquity (`oec`, `oec2000`) is keyed on the same bits and emptied
  the same way; the corrections do not enter it.
- The observer (`topd`) is keyed on them too (`SWI_TOPD_KEY`, entry 92)
  and emptied by a change of model (entry 129); its nutation comes
  through `swi_nutation()`, so through the nodes.
- `calc_nutation()`'s memo declines both flags (`SWI_NUT_NO_MEMO`).
- The corrections' availability: `plaus_iflag()` turns `SEFLG_JPLHOR`
  into `SEFLG_JPLHOR_APPROX` when none are loaded, so the flag a
  calculation's nodes are keyed on says it. Their contents change only by
  `swe_close()` and a later `swe_set_jpl_file()`.

**Fix, as done here:**
- `struct interpol` keeps, with the model, the flags' JPLHOR bits
  (`nut_jplhor`) and, with `SEFLG_JPLHOR_APPROX`, the approximation's
  mode, defaulted (`nut_jplhora`; 0 without the flag, which does not read
  it), and `swi_nutation()` uses the nodes only under all three. Behind
  `#ifndef SWE_UPSTREAM_COMPAT`, upstream's test under `#else`.
- `ctx_release()` (`swe_close()`, `swe_ctx_free()`) drops nodes built
  under `SEFLG_JPLHOR` with the corrections it frees. Behind `#ifndef`.
- `swe_get_ayanamsa_ex()` takes `ctx->nut` only under its own JPLHOR
  bits, as `swi_check_nutation()` keys it. Behind `#ifndef`.
- `swi_config_apply()`, in both builds, as the adoption is this fork's
  (entries 83, 122, 123, 126, 129): an adopted change of the mode empties
  the nodes as one of the nutation model does. This fork's build would
  refuse them by their key anyway.
- No hot path: with the interpolation off `swi_nutation()` is the code it
  was, and the key is read only with it on. `perf stat` instructions,
  `swe_calc()` of the ten bodies with speed at 20000 instants on the Swiss
  files: 11400206950 before, 11401497681 after (+0.011%). The golden
  transcript is bit for bit the tip's (13000 rows).

G122 `tests/nutintp.c`, extended with 84 checks, each in a forked
process, each answer bit for bit with its return flag a fresh process's
that set the final state first and made the calls after the change: the
interpolation on, de200, the path, an observer and True Citra set; the
probe at 1950 and 1950 + 0.37 and a topocentric Moon at 1950 + 0.37 under
the first flags; the change; the probe at 1950 and 1950 + 0.37 under the
final ones. `SEFLG_JPLHOR_APPROX` before and not after, and the reverse;
with it throughout, the mode from 3 to 2 and back by
`swe_set_astro_models()`, by a second thread adopting it and by
`swe_set_astro_models_r()` on an explicit context, and from 2 to the
default by `swe_close()` with the files set again: the nine probes of
entry 126 each, the flags theirs. And `SE_ECL_NUT` on the fixtures, made
in $TMPDIR: `SEFLG_JPLHOR` to `SEFLG_JPLHOR_APPROX` and back, and the
corrections A to B across `swe_close()`. 246 checks in all, none fail.
Before the fix 50 of the 84 fail: the flag per call 6.19e-2" (the
topocentric Moon 6.22e-2"), each change of mode 1.04e-1", and the houses
on the second thread 2.0e-10" (their nodes built at its delta t before it
opened the JPL file, noticed below); `SEFLG_JPLHOR` against
`SEFLG_JPLHOR_APPROX` 4.20e-2", the corrections 0.131". With the nodes
keyed and the ayanamsa not, its two flag cases fail; without the drop at
`swe_close()`, the corrections' case. The compat build fails 113 of 246:
entry 129's 74 and 39 of these, all but the second thread's ten, which
this fork's adoption fixes in both builds, and after `swe_close()` the
topocentric Moon, the ayanamsa and the sidereal Mars as in entry 129 (125
before the fix). G24 `nut_jplhor_intp_off`: dpsi at 1950 + 0.37 after
`SEFLG_JPLHOR_APPROX` at 1950, and at 1950 + 10.37 after mode 2 under it,
against a fresh context's, the larger: 0 shipped (1.03e-1" before the
fix), 1.03e-1" compat. G19, G29, G32, G57, G113, G2 and check-ctx pass;
G25 skips (no DE >= 403 here); G1's transcript is identical to the tip's.

Noticed, not fixed:
- `swe_get_ayanamsa_ex()`, `swe_get_ayanamsa_ex_ut()` and sidereal
  `swe_houses_ex2()` with `SEFLG_JPLHOR` and no corrections loaded (de200,
  or a thread that never called `swe_set_jpl_file()`) crash: their flags
  skip `plaus_iflag()`, and `calc_nutation()` calls `bessel()` on
  `ctx->dpsi`, NULL (SIGSEGV at J2000). Both builds. Entry 142.
- A second thread on the default context never loads the corrections
  (`load_dpsi_deps()` runs in `swe_set_jpl_file()` alone), so its
  `SEFLG_JPLHOR` becomes `SEFLG_JPLHOR_APPROX` ("you did not call
  swe_set_jpl_file()") where the setter's thread keeps it: dpsi on the
  fixtures -13.522732" for -13.564630" (4.2e-2"). Entry 143.
- `swe_set_ephe_path()` keeps the corrections of the old directory
  (only `swe_close()` frees them, and the loader loads once): after A,
  B and `swe_set_jpl_file()`, `SEFLG_JPLHOR` answered A's -13.479309"
  for B's -13.348539" (0.131"). Entry 144.
- `swe_deltat_ex()` with no ephemeris flag (0, which the houses pass)
  answers 28.927550 s at 1950 on the thread that set de200 and 28.931886
  s on a second thread until it opens the JPL file (4.3e-3 s). Fixed as
  entry 145: no ephemeris bit is the Swiss files' term on every thread.
- Across `SEMOD_JPLHORA_3`'s switch at 1962 Jan 20, the nodes lie on both
  sides of a 7.2e-2" step: dpsi at the switch + 0.3 after a call at - 0.5,
  -10.105320" interpolated for -10.124814" (1.95e-2"), where the
  interpolation is otherwise within about 3 mas. Both builds. Entry 146,
  fixed there.

## 130. `swe_calc_orbel()` never pairs the frame bias: its geocentric places are seen from the ICRS Earth, and SEFLG_ICRS leaves the body unturned (this fork's)

**Severity: Correctness (3.2e-8 to 1.1e-7 AU on every geocentric and topocentric place, 1.4e-5 deg on an orbit with a = 1.2 AU; 4.9e-6 to 6.1e-6 deg from every centre with SEFLG_ICRS)**
**Where:** `sweph.c`: `app_pos_etc_orbel()`

`app_pos_etc_orbel()` is the fork's copy of the fictitious bodies' path,
`app_pos_etc_plan_osc()`, made for `swe_calc_orbel()` (f45e0b1,
2026-09-17) before entry 20 paired the frame bias on that path (9ed27ec,
2026-09-29). The pair was never carried to the copy. The elements' state
(`swi_orbel_to_state()`) is dynamical J2000; the Sun or the Earth added to
it is ICRS on every ephemeris with a bias (DE403 and later, the Swiss
files, Moshier); and nothing turned either. So a J2000 answer was seen
from the ICRS Earth, entry 20's defect:

- Every geocentric and topocentric place was off by (I - B)(S - E):
  3.2e-8 to 1.1e-7 AU by the Earth's direction (2025, 48 dates), whatever
  the orbit and every flag set, SEFLG_TRUEPOS included. Against the same
  elements read from a seorbel.txt (G126, five dates 1700..2150):
  1.1e-5 to 1.45e-5 deg at a = 1.2 AU (1.5e-7 deg/day), 2.0e-7 deg at
  30 AU, 6.2e-10 deg at 80000 AU.
- A barycentric place was off by (I - B) S: 3.2e-8 deg at 1.2 AU.
- An orbit about the Earth was off heliocentrically and barycentrically
  by the whole Earth's turn, 6.1e-6 deg, and topocentrically by the
  site's, 2.1e-8 deg at a = 0.01 AU.
- With SEFLG_ICRS the body itself stayed dynamical from every centre:
  4.9e-6 to 6.1e-6 deg, the bias's 23 mas.

The geocentric place less the heliocentric one is not the Earth that
`swe_calc(SE_EARTH, SEFLG_HELCTR)` returns: up to 1.1e-7 AU. de200, an
ephemeris with no frame bias, agreed to 1.6e-11 deg.

**Fix, as done here:** entry 20's pair, on the same condition
(`swi_get_denum(SEI_EARTH) >= 403`). The body goes to ICRS (the backward
`swi_bias()`) before its centre is added, at teval and at every retarded
epoch of the light-time loop. The whole goes back to J2000 (the forward
`swi_bias()`) where `app_pos_etc_plan_osc()` takes it, unless SEFLG_ICRS.
So the pair cancels on the body and turns only what came from the
ephemeris. Behind `#ifndef SWE_UPSTREAM_COMPAT`, as entry 20's own pair
is, although the function is the fork's: its compat build follows
upstream's fictitious path, as for entries 81 (part 3), 88 and 109, and
that path has no pair. Only `swe_calc_orbel()` pays: two rotations a
call and one per light-time iteration.

Left as they are:
- On the Earth-centred orbit (a = 0.01 AU) the two paths' speeds differ
  by up to 1.0e-11 deg/day, 5.8e-12 on a = 1.2 AU: their light-time
  rates, the same before the fix and on de200. Below the floor, not
  fixed.
- Moshier with SEFLG_BARYCTR: `swe_calc()` refuses it, and
  `swe_calc_orbel()` answered it with the heliocentric place, 5.7e-3 AU
  off the Swiss barycentric one, returning SEFLG_BARYCTR. A matter apart,
  not fixed here: entry 135, which made G126 check that centre on Moshier
  as a refusal.
- Elements of the equinox of date (SE_ORBEL_EQ_DATE): the light time held
  that equinox at the observation instant and the speed had no frame
  turning, where seorbel.txt's JDATE path has both. Suspected here, not
  measured: entry 136, measured and fixed there.

G126 `tests/orbelbias.c`. Check 1: `swe_calc_orbel()` against `swe_calc()`
of the same elements, written to a seorbel.txt in a temp dir (each with
the date as its epoch and M0 + n T, n of KGAUSS or KGAUSS_EARTH). Four
orbits (1.2 AU; 30 AU on the B1950 equinox; 80000 AU circular; 0.01 AU
about the Earth), five dates spread over the year, four centres, twelve
flag sets (of date, J2000, ICRS, equatorial, XYZ, true, no aberration, no
nutation, sidereal), on the Swiss files, de200, Moshier and an optional
DE4xx file (SE_TEST_DE4, as G104): 240 rows an orbit, 180 on Moshier.
Within 6.8e-13 deg and 1.0e-11 deg/day (limits 1e-11, 1e-10). Check 2:
the true J2000 equatorial geocentric place and velocity are the
heliocentric ones less `swe_calc()`'s heliocentric Earth, the barycentric
ones the heliocentric plus its barycentric Sun, and about the Earth the
heliocentric ones the geocentric plus the Earth: within 3.6e-16 and
6.7e-18 of the distance (limit 1e-14). Before the fix 16 failures, 24
with DE431; the compat build fails 20, 28 with DE431 (check 2 by 7.1e-8
of the distance; check 1 by its other upstream defects too).

G27 `tests/fictlt.c` gains a fifth row, the same orbel orbit seen from the
Earth: 1.83e-10" from B(t - tau) - E(t), 2.70e-7" before the fix (within
its 1e-6" limit, the orbit being at 80000 AU). The row from the Sun moves
1.05e-9" -> 1.83e-10". The compat build fails eight of the ten checks.

G24 `orbel_bias_off`: the geocentric true J2000 equatorial XYZ place of
the 1.2 AU orbit less its heliocentric one plus the Earth, Swiss files,
four dates in 2025: 2.22e-16 AU shipped, 8.52e-8 AU compat.

G126, G27, G65, G87, G107, G48, G108 and G24 (both ways) pass. G1's
output is bit-identical to the build before's.

## 132. `swe_time_equ()`, `swe_lmt_to_lat()` and `swe_lat_to_lmt()` take the JPL Sun only when this context has the JPL file open: another thread's `swe_set_jpl_file()` missed, 1.19e-8 day

**Severity: Correctness (behaviour: the answer follows whether this context opened the JPL file, not the setting; E 1.19e-8 day (1.03 ms) off at J2000, 1.17e-7 day in 1800, no error)**
**Where:** `sweph.c`: `swe_time_equ_r_body()` (`swe_time_equ()`, `swe_lmt_to_lat()`, `swe_lat_to_lmt()` and their `_r()`)

Found by entry 128's probe. The three take no ephemeris flag:
`swe_time_equ()` reads its Sun with SEFLG_JPLEPH added when the context
has the JPL file open -- upstream's way of saying the caller chose JPL,
the same test upstream's `swi_guess_ephe_flag()` makes for
`swe_deltat()`, `swe_get_ayanamsa()` and `swe_get_ayanamsa_ut()`, and
the two others call it. Entry 103 moved the guess onto the configuration
(JPL when a JPL file has been set and read, `jpldenum_cfg`) and left this
copy of the test, so the three answered for whether this context had
happened to open the file. Measured by a scratch program, each scenario
in its own process, against a fresh process that set the path and
`swe_set_jpl_file("de200.eph")` and called on its main thread, with each
scenario's own `swe_deltat()` and `swe_get_ayanamsa_ut()` beside them:
- A new thread after the main thread's setter, a thread that had computed
  on the path alone before it, a context made by `swe_ctx_new()` after it,
  and a new thread after `swe_close()` and the two setters again answered
  from the Swiss files: E at J2000 + 6h -2.363773431805e-3 day for
  -2.363761535354e-3, 1.19e-8 day (1.03 ms), 1.17e-7 day at 1800 Jan 1,
  LAT and LMT with it, while their own `swe_deltat()` and
  `swe_get_ayanamsa_ut()` answered for de200 (in 1800 4.46 s of delta t
  from the Swiss files').
- After `swe_close()` the guess is Swiss -- the close forgets the DE
  numbers with the files (the 2026-09-16 review's F2), and the manual has
  the caller set the JPL file again "if required" -- and so were the
  three, until a JPLEPH calculation, which reopens the file by the name
  the close keeps, made them de200's, 1.19e-8 day the other way, with
  `swe_deltat()` still Swiss.
- The setting thread, a context given its own `swe_set_jpl_file_r()` and
  the closing thread after the setters again opened the file and were
  right.
The compat build answers the three the same. Upstream, which shares no
configuration, is
consistent within itself: its guess and the three both follow the open
file, and a JPLEPH calculation moves all of them alike (entry 103's
call-order part).

The reference. The three have no flag argument to read, and the last
ephemeris used is what entry 103 took out of the guess; the JPL file the
configuration sets is what `swi_guess_ephe_flag()` already reads for the
other functions without a flag. (`swe_sidtime()` and `swe_deltat_ex()`
with -1 take the default tidal term and no ephemeris at all; the eclipse
and heliacal functions take the caller's flag.) So the three follow the
guess, after `swe_close()` too, where it is Swiss: answering JPL there
from the kept name while `swe_deltat()` answers Swiss would split them
again, and the close's treatment of the DE numbers is F2's, not this
entry's.

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`, upstream's
test under `#else`): `swe_time_equ_r_body()` adds SEFLG_JPLEPH when
`swi_guess_ephe_flag()` names JPL. The context is started and synced by
then (the hold, bug 128, and the call's own `swi_init_swed_if_start()`).
A context that has not opened the file opens it by the configuration's
name in the calculation (bug 119). No hot path: the three functions
alone. With the fix the scratch program finds every scenario's E, LAT
and LMT bit for bit the fresh process's -- de200's where the guess is
JPL, the Swiss files' after `swe_close()` with or without the JPLEPH
calculation -- and on the ephemeris of its own `swe_deltat()` and
`swe_get_ayanamsa_ut()`.

G128 `tests/timeequ.c`, each case in a forked process, the three
functions at J2000 + 6h and 1800 Jan 1 with their return codes, bit for
bit against a fresh process given the same settings that opened the file
itself and called on its main thread: a new thread after the setter
(`thread`), a thread that computed before it (`early`), a context made
after it (`ctx`), a new thread after `swe_close()` and the setters again
(`reset`), and after `swe_close()` a JPLEPH Sun against the same calls
without it (`closecalc`). 0 in all five; before the fix the five fail,
E 1.19e-8 day at J2000 and 1.17e-7 day in 1800 (`closecalc` the other
way); the compat build fails the same five by the same amounts. G24
`time_equ_thread_off`, `swe_time_equ()` on a second thread after
`swe_set_jpl_file("de200.eph")` against the first's: 0 shipped, 1.03e-3
s compat. G2, G116, G121, G123, G124, the context test and G24 pass.

Noticed, not fixed: after `swe_set_jpl_file("de200.eph")`, `swe_close()`
and a JPLEPH Sun, `swe_get_tid_acc()` answers de200's -23.8946 while the
next `swe_deltat()` (the guess Swiss, the moon file not open) uses
-25.80, 18.604686 s in 1800, and the getter then answers -25.80; bug 101
has the getter answer with the term the next delta t uses. The compat
build's getter and delta t agree (-23.8946, 14.438958 s), both following
the open file. The first delta t's -25.80 itself is entry 127's noticed
item.

## 133. The Sunshine houses' remembered declination is wiped by a context's first start (this fork's): a recall with 99 after a first calculation answers with declination 0, cusp 3 6.71 deg off

**Severity: Correctness (behaviour: wrong cusps, no error)**
**Where:** `sweph.c`: `swi_init_swed_if_start()`; `sweph.h`: `struct swe_ctx`
(`saved_sundec`); the memo is `swehouse.c`'s `swe_houses_armc_ex2_r()`

`swe_houses_armc()` and `swe_houses_armc_ex2()` remember the Sun's
declination a Sunshine ('I') call is given in `ascmc[9]`, and a later
call with 99 there recalls it, as `swe_house_pos()` always does. Upstream
keeps the memo in a function-scope static that nothing resets; Phase 3c
moved it into the context as `saved_sundec`, 99 in `swed`'s initialiser
and in `ctx_init_defaults()`. The two functions take no ephemeris and do
not start the context, so on a thread's first calls the memo can come
before the start, and the start's memset in `swi_init_swed_if_start()`
wiped it, leaving 0 where 99 means "none yet" (the comment at the field
said the start set 99; it did not). The recall then answered with
declination 0: the cusps depended on whether a calculation had come
between. At ARMC 123.4, 47.37 N, obliquity 23.44 with 20 deg remembered,
a first `swe_calc_ut()` or `swe_set_ephe_path()` and a recall gave cusps
2 and 3 at 228.717 and 260.801 deg for 223.821 and 254.087, cusp 3's
speed 9.53 deg/day off by `swe_houses_armc_ex2()`, and `swe_house_pos()`
of a body at 200 deg 1 deg N 12.7645 for 12.8080; both builds, on the
main thread and a new one. Upstream 2.10.03, built from its tag, answers
223.821 after the calculation too. The wrong marker alone is not seen
through the API: with no memo, 99 and 0 both recall declination 0.

Measured by a scratch program (not in the tree): 66 public functions,
each as a fresh process's first call, with the context's bytes before and
after it compared field by field where it left the context unstarted, and
again after a start; and a started default context against a
`swe_ctx_new()` one. In the default build `saved_sundec` is the one field
a public function sets before the start, and the one whose empty value
the start leaves wrong (the others that differ are the path's first use
and the tables a new context seeds and the default one seeds on first
use). The setters, the getters and the functions that calculate start
the context first (bugs 83, 101, 122, 128); the rest -- the date and
angle arithmetic, `swe_house_pos()` and `swe_houses_armc()` with other
systems, the names, `swe_version()`, `swe_get_library_path()`,
`swe_get_current_file_data()`, `swe_refrac()`, `swe_ctx_new()` -- write
nothing to the default context. Under `SWE_UPSTREAM_COMPAT` more write
before the start and lose it there, as upstream's do: the setters of the
tidal acceleration, delta t and the nutation interpolation (upstream's
half of bug 83, which the switch restores), the caches of the UTC
conversions, the heliacal angle functions and `swe_close()`, and the star
magnitudes' open sefstars.txt, whose FILE the memset forgets -- one
descriptor left open past `swe_close()`, upstream 2.10.03 the same, not
fixed here.

**Fix, as done here:** `swi_init_swed_if_start()` keeps `saved_sundec`
across its memset, as it keeps the configuration fields, so a memo made
before the start is recalled after it and a context with none keeps 99.
In both builds: upstream's static was never reset, so `SWE_UPSTREAM_COMPAT`
has nothing to restore. Only a context's first start runs the memset, off
the hot path; G1's 13000 rows are bit-identical to the tip's.

G129 `tests/sunmemo.c`: six cases, each in its own process, the memo,
the first start and the recall bit for bit a fresh process's recall
straight after the memo: `calc` (`swe_calc_ut()` starts the context,
`swe_houses_armc()` recalls), `path` (`swe_set_ephe_path()` starts it),
`pos` (`swe_house_pos()` recalls), `speed` (`swe_houses_armc_ex2()` with
speeds, both ways), `thread` (calc on a new thread), and `marker` (no
memo: 99 after the start, as before it). 0 in all six; before the fix
both builds fail all six, 6.71 deg, 6.71 deg, 0.0435 of a house, 9.53
deg/day, 6.71 deg and 0 for 99. No G24 case: the two builds agree before
the fix and after it. G20, G24, G45, G58, G67, G82, G124 and the context
test pass.

## 131. After `swe_close()` the kept path's first delta t takes the default tidal term until a file opens: the same call answers differently first and second

**Severity: Correctness (behaviour: call order -- the first delta t after `swe_close()` 11.29 s off in AD 1000, the first UT Moon there 1.6e-3 deg off the second; with a JPL file 11.27 s in AD 1700 -- on every thread and context)**
**Where:** upstream `sweph.c:1233` (`swe_close()`), `swephlib.c:3198` (`swi_get_tid_acc()`, the moon file's DE number from the open file only); here `sweph.c`: `ctx_release()` (`swe_close()`, `swe_close_r()`), `swi_init_swed_if_start()`, `swe_set_ephe_path_r()`, `swe_set_jpl_file_r()`, new `swi_denum_reread()`; `sweconfig.c`: `swi_config_capture()`, `cfg_merge()`, `swi_config_apply()`, `swi_config_reset()`; `sweph.h`, `sweconfig.h`

Entry 127's noticed item. `swe_close()` keeps the path and the JPL file's
name, as upstream's does (its reset of the path is commented out), and
forgets what the setters read from them: upstream closes the moon file
whose header `swi_get_tid_acc()` takes the DE number from, and this fork's
`ctx_release()`, and since bug 123 the shared configuration's reset after
it, zero the two published DE numbers too (the 2026-09-16 review's F2). So
the first delta t after the close, before any file is open, took the
default tidal term: after the path, a Swiss Moon and `swe_close()`,
`swe_deltat_ex()` in AD 1000 answered 1463.43 s for 1474.72,
`swe_get_tid_acc()` -25.80 for the moon file's -25.936, and
`swe_calc_ut()`'s Moon there 197.3747177 deg on the first call and
197.3763253 on the second, which the first had opened the file for. With
`swe_set_jpl_file("de200.eph")` set too, delta t in AD 1700 14.29 s for
3.01, the JPL Moon 40.0057508 deg for 40.0039614, `swe_get_tid_acc()`
-25.80 for -23.8946. Where a TT calculation came first the getter and
the next delta t disagreed, which bug 101 has agree (found in bug 132's
work): after a JPL Sun `swe_get_tid_acc()` answered de200's -23.8946 and
the next `swe_deltat()`, whose guess was Swiss with the JPL DE number
forgotten, used -25.80, 18.604686 s in AD 1800 for 14.438958, the getter
-25.80 after it; after a Swiss Sun, which opens the moon file for the
Earth, the getter -25.80 and the delta t -25.936. So on the closing
thread, on a thread that had computed before the close and adopted it,
on a thread started and a context made after it, and on an explicit
context closed by `swe_close_r()`; with the files named by `SE_EPHE_PATH` alone on the
closing thread and the explicit context. A UT search converges past its
first delta t and is not moved, and with no path at all the default term
is the right one either way. The upstream build (legacy-master), a
main-thread program: 1463.43 s, -25.80, the Moon 197.3747175 then
197.3763252 deg; with de200.eph 14.29 s, 40.0057511 then 40.0039618 deg.
Setting the path again after the close, as the manual advises, answers
right at once.

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`; the compat
build keeps upstream's default term after the close, on the threads too):
- `ctx_release()` notes what it forgets that the kept settings give
  (`denum_unread`: the moon file's DE number when a path is in effect, the
  JPL file's when one was read), and opens nothing.
- The first entry after it that is not a setter (`swi_init_swed_if_start()`
  outside a setter's apply) reads them again along the kept path as the
  setters read them (`swi_denum_reread()`: the moon file's header at J2000
  by `ephe_path_read_moon()`, the JPL file opened, with the IERS
  corrections of a DE 403 or later) and resolves the automatic tidal term
  as a first use does (bug 118's block, which now runs for this too). No
  new probe: the setters' own reads. `swe_set_ephe_path()` clears both,
  `swe_set_jpl_file()` the JPL one.
- The shared configuration (this fork's): `swi_config_reset()` marks the
  same two in the master with the close's generation (`path_closed`),
  published and merged with the path group, so a thread adopting the
  close and a context made after it read them again themselves. Until a
  setter publishes them, the master's zeros are not compared with what a
  context that adopted that close has read: compared, every sync after its
  reading took it back to the close, every file closed and reopened (3
  opens for the next Moon after an unrelated `swe_set_topo()`, 2 with
  de200.eph). A later close has another generation and is adopted again.
- The cost: the first entry after a close opens `semo_18.se1` for its
  header, as the path setter did; nothing else. `swi_init_swed_if_start()`
  is on the hot path: one load and test, calc-swieph 12262522596
  instructions before and 12262762642 after, calc-moseph 23329084923 and
  23329324913 (0.002%).

G127 `tests/closetid.c`, each scenario in a forked process: the setting, a
Moon by `swe_calc_ut()`, `swe_close()`, then the observation twice, on the
main thread, a thread that had computed before the close, a thread started
and a context made by `swe_ctx_new()` after it, and an explicit context
given the same calls with `swe_close_r()`, each bit for bit (values, flag,
serr) as in a process given the same calls without the close. Settings: the
path; the path and de200.eph (SEFLG_JPLEPH, AD 1700); `SE_EPHE_PATH` alone;
no path at all. Observations: `swe_deltat_ex()`, `swe_deltat()`,
`swe_get_tid_acc()`, the Moon and its speed, `swe_mooncross_ut()`, and a
TT Sun followed by the getter, `swe_deltat()` in AD 1800 and the getter.
120 checks, and 2 more: the thread that adopted the close and read the
files again opens nothing at its next sync after another thread's
`swe_set_topo()`. All 122 pass. Before the fix 59 fail (25 with the path,
23 with de200.eph, 10 with `SE_EPHE_PATH`, and the second part's running
thread on its first Moon); the compat build 44 (its JPL Sun's getter and
delta t agree, its guess following the open JPL file); the fix without
the master's generation fails 2 (3 and 2 fopen() calls). G24
`close_tid_off`, the first delta t in AD 1000 after the close against a fresh context given the
path: 0 shipped, 11.29 s compat. G123 (`tests/closesurvive.c`), which
observed in TT for this, now takes the sidereal readers' ayanamsa in AD
1000 by `swe_get_ayanamsa_ex_ut()` first, before any file opens: 84 pass;
48 fail before this fix (3.3e-9 deg), 78 compat. G120, G115, G106, G82,
G100, the delta-t memo, G119, cfgleak, G2 and check-ctx pass.

Noticed, not fixed: a thread that had the JPL file open and adopts another
thread's `swe_set_ephe_path()` to a directory without it kept the file's DE
number, so its SEFLG_JPLEPH delta t in AD 1700 was 3.012003 s for the
14.286718 a fresh process with that path gives (before this entry too).
Fixed as entry 137. And with no close: the path, de200.eph, the path again
by another string (which forgets the JPL DE number) and a JPL Sun left
`swe_get_tid_acc()` at -23.8946 while the next `swe_deltat()` guessed the
Swiss files and used -25.936 (18.902020 s in 1800), the getter -25.936
after it; the compat build agrees with itself (14.438958 s). Fixed as
entry 138.

## 135. `swe_calc_orbel()` answers a barycentric Moshier request, which `swe_calc()` refuses (this fork's)

**Severity: Correctness (behaviour: an answer where `swe_calc()` refuses, the heliocentric place labelled barycentric, 5.7e-3 AU off the barycentric one)**
**Where:** `sweph.c`: `swe_calc_orbel_r()` (and through it `swe_calc_orbel_ut_r()`)

Found by entry 130's work. `swecalc()` refuses every barycentric Moshier
request: "barycentric Moshier positions are not supported.", -1. Moshier
serves heliocentric places only, the Sun its origin. `swe_calc_orbel()`
does its own ephemeris setup and never passes through `swecalc()`, so it
answered. Its observer for SEFLG_BARYCTR is the origin, which on Moshier
is the Sun, so the place was the heliocentric one, under a return flag
with SEFLG_BARYCTR set (18180 with SEFLG_SPEED):

- an orbit with a = 1.2 AU at 2000 Jul 1.5: 167.8283 deg, Moshier's
  heliocentric longitude to the last digit, where the Swiss files'
  barycentric one is 168.1189 deg, 0.29 deg and 5.7e-3 AU away;
- the same for an orbit about the Earth (279.418 against 279.049 deg).

With the files missing the two functions already agreed, as entries 112
and 120 have them follow each other: under the strict setting both refuse
a Swiss or JPL request for the missing files, and with the fallback
allowed both answer it from Moshier (below). Only a request that named
Moshier itself differed.

**Fix, as done here:** `swecalc()`'s test and message in
`swe_calc_orbel_r()`, on the flags as `plaus_iflag()` leaves them (the
ephemeris bits' precedence, so SEFLG_SWIEPH | SEFLG_MOSEPH is a Swiss
request; SEFLG_TOPOCTR clears SEFLG_BARYCTR). Unguarded, in both builds,
on entry 112's precedent: the function is the fork's, so it follows the
fork's `swe_calc()` there, and the compat build's `swe_calc()` refuses
the request too -- there is no upstream behaviour for
`SWE_UPSTREAM_COMPAT` to restore. It was first made behind `#ifndef
SWE_UPSTREAM_COMPAT`, as entries 88 and 130 are, which kept in the compat
build an answer upstream never gave. Its `sweph.h` bullet now says it is
in both builds, and there is no G24 case.

Left as it is, not this entry's: with the fallback allowed, a Swiss or JPL
barycentric request that falls to Moshier is answered by `swe_calc()`
itself with Moshier's heliocentric place under SEFLG_BARYCTR | SEFLG_MOSEPH
(Mars at 2000 Jul 1.5: 7.2e-3 AU, 0.24 deg off the files' barycentric Mars,
0 from Moshier's heliocentric one), and `swe_calc_orbel()` answers as it
does, bit for bit in flag and message. A defect of `swe_calc()`'s
fallback, measured here and not fixed. Fixed as entry 139, both now refusing it.

G126 `tests/orbelbias.c` (entry 130's gate) now checks that centre:
- check 1, Moshier's barycentre, 60 rows an orbit, is refused by both with
  `swe_calc()`'s message;
- a new check 3 takes the files missing (the element file's temp dir
  alone), strict and with the fallback: each orbit at each date, Moshier,
  Swiss and JPL, geocentric, heliocentric and barycentric, 360 calls. Each
  has `swe_calc()`'s return flag and message for the same elements from
  seorbel.txt, and where both answer the same place, within 1.7e-13 deg
  and 1.0e-11 deg/day; 160 are refused by both.

Before the fix G126 fails 5 (each orbit's Moshier barycentre, answered
18180, and check 3 by its 40 such calls); after, it passes. The compat
build failed 26 with the fix guarded, its 20 of entry 130 and these 6;
unguarded it passes these, Moshier's barycentre refused there too (53
failures with entry 136's eight orbits, 62 with both guarded).

No G24 case: the guarded fix's `orbel_mosh_bary_ret` (`swe_calc_orbel()`
of the a = 1.2 AU orbit, Moshier, barycentric, at 2000 Jul 1.5: -1
shipped, 18180 compat) went with the guard; the compat build returns -1
too.

## 136. `swe_calc_orbel()` holds an equinox of date at the observation instant through the light time, and its speed leaves out that frame's turning (this fork's)

**Severity: Correctness (1.9e-2 deg at a = 80000 AU, 6.9e-6 deg at 30 AU, 3.5e-7 deg at 1.2 AU; speeds 4.0e-5 to 8.8e-5 deg/day off)**
**Where:** `sweph.c`: `swi_orbel_to_state()`, `app_pos_etc_orbel()`; `swephexp.h` (the contract)

Suspected by entry 130's work and measured here. With SE_ORBEL_EQ_DATE,
`swe_calc_orbel_r()` takes the elements' equinox as tjd. Two things then
departed from seorbel.txt's JDATE elements, the path `app_pos_etc_orbel()`
was copied from:

- **The light time.** The loop (bug 81's, on every flag set) evaluated the
  elements at the retarded epoch, M - n dt, but still in the equinox of
  tjd. `read_elements_file()` gives JDATE the instant it is read at: under
  light time the emission instant, for the elements and for an equinox of
  date alike (the ephemeris protocol's 3.5a, G27). So the frame's turn
  over the light time was missing, about 3.8e-5 deg a day of light time.
- **The speed.** `swi_orbel_to_state()` gave the two-body motion in the
  frame held at tequ. A frame of date turns with t, and since entry 48
  `swi_osc_el_plan()` adds that turning (the precession's and the
  obliquity's rates) to a JDATE body's speed. Here it was missing.

Measured against the same elements written as a JDATE body (G126's check
1 arrangement: epoch the date, M0 + n T), five dates 1700..2150, every
centre and twelve flag sets, Swiss, de200 and Moshier alike:

| orbit | position | speed |
|---|---|---|
| a = 1.2 AU | 3.5e-7 deg (3.2e-7 geocentric) | 8.8e-5 deg/day geocentric, 4.2e-5 heliocentric |
| a = 30 AU | 6.9e-6 deg | 4.0e-5 deg/day |
| a = 80000 AU (462 days of light time) | 1.9e-2 deg | 4.2e-5 deg/day |
| a = 0.01 AU about the Earth | 2.9e-9 deg | 4.6e-5 deg/day geocentric, 4.3e-7 heliocentric |

The speeds were off the positions' own five-point rate by the same
amounts, while the JDATE body's agree with theirs within 4.6e-9 deg/day
(the Earth-centred orbit's topocentric speed apart, below). All are above
the floor (1e-9 deg, 1e-8 deg/day). The position did not depend on
SEFLG_SPEED, before or after.

**Fix, as done here:**
- `swi_orbel_to_state()` takes an `of_date` flag. With it the speed gains
  the frame's turning exactly as `swi_osc_el_plan()` computes it for
  JDATE: the same rotation over +-1 day, applied to the ecliptic position
  and differenced centrally.
- The light-time loop of `app_pos_etc_orbel()` evaluates the equinox at
  the retarded epoch t.
- The cost falls only on SE_ORBEL_EQ_DATE calls: two obliquities and two
  precessions per state evaluated.
- `swephexp.h` now says so: "SE_ORBEL_EQ_DATE is the equinox of the
  instant evaluated, as seorbel.txt's JDATE: under light time the
  emission instant's, and the speed carries that frame's turning."
- Unguarded, in both builds, on entry 112's precedent, as entry 135: the
  function is the fork's and follows the fork's `swe_calc()` there. The
  compat build's speed branch (the straight light time, the orbit read at
  t - dt for the speed only) takes the equinox at t as well, as upstream's
  own JDATE branch reads it there; the guard had kept the equinox at tjd,
  a defect upstream never had. (Upstream's fictitious speed has no
  turning, entry 48's defect, which the compat build's fictitious path
  keeps.) Its `sweph.h` bullet now says it is in both builds, and there
  is no G24 case.

After: within 7.1e-13 deg and 8.2e-12 deg/day of the JDATE body. The
speeds are within 4.6e-9 deg/day of five-point differences (the 1.2 AU
orbit topocentric; the JDATE body's own 4.6e-9 there too), 2.4e-9 about
the Earth.

Left as it is, not this entry's: about the Earth, the topocentric speed
of the 0.01 AU orbit is 1.8e-8 to 4.6e-8 deg/day off five-point
differences of its positions. It holds flat over steps 2^-8..2^-12 day,
on every equinox (J2000 too) and on the seorbel.txt path alike, at the tip
before this change. A residual above the floor, measured and recorded
here, not fixed. Fixed as entry 140: the observer's site placed by a
rounded UT1, a staircase in delta t; G126's check 4 now takes that
centre too.

G126 `tests/orbelbias.c` (entries 130 and 135) gains four orbits, the
first four with their elements of the equinox of date (JDATE in the
file), in checks 1 to 3. Check 3 grows to 720 calls, 320 refused by both.
A new check 4 takes their speeds against five-point differences of the
positions:
- step 2^-7 day (2^-10 about the Earth), at a binary-fraction instant
  near each date, the mean anomaly moved along the mean motion;
- six flag sets, every centre but Moshier's barycentre and the
  Earth-centred orbit's topocentric one (above);
- within 1e-8 deg/day.

Before the fix 25 failures: checks 1 and 4 on each orbit of date on each
ephemeris, and check 3's places, 1.8e-2 deg. After, it passes. The
compat build failed 62 before and after the guarded fix, its output the
same. Unguarded, with entry 135's, it fails 53: check 1 on every orbit,
check 2 on every orbit on the ephemerides with a bias, and check 4 on the
orbits of date (9.3e-5 to 4.0e-4 deg/day, its copy's straight light
time, G27's defect) as before, the a = 80000 AU orbit of date now 1.9e-2
deg off the JDATE body in check 1 for 2.3e-2, and check 3's places by
entry 88's infinity.

No G24 case: the guarded fix's `orbel_date_rate_off` (the 1.2 AU orbit of
date, heliocentric, geometric, J2000 equatorial XYZ, Swiss files, its
velocity against a five-point difference of its positions over 2^-7 day,
four dates in 2025: 1.73e-14 AU/day shipped, 6.11e-7 AU/day compat, the
frame's turning, omega x r; 6.11e-7 shipped before the fix) went with the
guard; the compat build gives 3.48e-14 AU/day.

## 137. A thread that had the JPL file open and adopts another thread's `swe_set_ephe_path()` keeps the file's DE number: its JPLEPH delta t takes the closed file's tidal term (this fork's)

**Severity: Correctness (behaviour: delta t for a JPL file the thread no longer has, 11.27 s in AD 1700, on the adopting thread)**
**Where:** `sweconfig.c`: `swi_config_apply()`, its branch for an adopted path

Entry 131's noticed item. A thread on the default context that follows
the shared configuration adopts another thread's `swe_set_ephe_path()` in
`swi_config_apply()`. Its branch for a new path closed the JPL file and
kept the file's DE number (`ctx->jpldenum`), which the branch for a new
JPL file alone zeroes (bug 103), and which the setting thread's
`swi_close_keep_topo_etc()` zeroes; `calc_deltat()` takes it, upstream's
line, for every SEFLG_JPLEPH delta t. So after the path,
`swe_set_jpl_file("de200.eph")` and a JPL Sun on a second thread, which
opened the file, the main thread's path to a directory without de200.eph
left the second thread's `swe_deltat_ex()` in AD 1700 with SEFLG_JPLEPH at
3.012003 s, de200's term (-23.8946), where the setting thread, a thread
started after it, an explicit context given the same calls and a fresh
process given the same setters answer 14.286718 (the default term: the
file is nowhere along the path); and the same after the path again by
another string (EPHEDIR/.), where the file is still there and the path
setter forgets its DE number by design (the 2026-09-16 review's F4). The
stale number stayed until the thread opened a JPL file again;
`swe_get_tid_acc()` after it followed it (-23.8946). `swe_deltat()` was
not moved, its guess being Swiss; `swi_get_denum()` reads the number only
for a JPLEPH calculation, which opens the file first.

The two branches differ otherwise, each measured on a thread adopting
the change against the setting thread:
- `last_epheflag`, zeroed by the path branch alone: the JPL-file branch
  forgets the positions itself (`swi_adopted_files_forget()`, bug 119), so
  the next calculation's switch has nothing more to forget; G116's JPL
  cases answer bit for bit as the setting thread.
- The Swiss files, seorbel.txt's lines and the star catalogue, closed and
  emptied by the path branch alone: the JPL-file branch keeps them as the
  same path's, by design (bug 103). A JPL Sirius by `swe_fixstar2()` on
  a thread adopting de200.eph to G116's de200x.eph equals the setting
  thread's bit for bit (the crafted file moves it 8.8e-11 deg).
- The tidal term, the heliacal memos and the observer: in both.

**Fix, as done here:** the path branch zeroes the DE number with the
file, as the JPL-file branch does. In both builds: the adoption is this
fork's, and upstream's threads share no configuration.

G132 `tests/jpldenum.c`, each case in a forked process: the path and
de200.eph, a JPL Sun by the reader, then the last setter (none, the path
again as EPHEDIR/., a path without de200.eph, a JPL file that is not
there), read by the setting thread, a thread that computed before the
last setter, a thread started after it and an explicit context:
`swe_deltat_ex()` in AD 1700 with SEFLG_JPLEPH and its serr,
`swe_get_tid_acc()`, `swe_deltat()` and the getter again, bit for bit as
a fresh process given the same setters alone. 16 pass; before the fix the
running thread fails after the two paths, 2. The compat build fails 7,
with the fix and without, by the upstream behaviour it restores: there
the running thread's first delta t reads the number before the call
syncs (bugs 124, 128), and a new thread warns on its first call (bug 110)
and guesses from the file it has open (bug 103); with a
`swe_get_tid_acc()` first, which syncs, its running thread answers
14.286718 with the fix and 3.012003 without. No G24 case: the fix is in
both builds.

Noticed, not fixed: a thread adopting another thread's path keeps the old
directory's delta t table, which the setter rereads (its
`swi_close_keep_topo_etc()` clears `init_dt_done`, the adoption does not):
with a swe_deltat.txt giving 80.0 s for 1990 in the first directory, the
adopting thread's `swe_deltat()` at 1990 Jan 1 stayed 79.987645 s after
the other thread's path to EPHEDIR, where that thread and a fresh process
answer 56.856191 (23.13 s). `leapsec_done`, cleared beside it, likewise
(not measured). Fixed as entry 147, with the leap seconds (1 s in 2026),
the planet-name memo and the delta-t memo, which kept the old table on
the setting thread too. And F4's forgetting of the JPL file's DE number,
by which the path again by another string answered above as a path
without the file, is reversed by entry 148: the number is read again
along the new path.

## 138. A JPL file a calculation opens sets the automatic tidal term for itself, not for the guess: after a path setter forgets the JPL DE number, `swe_get_tid_acc()` answers a term the next `swe_deltat()` does not use

**Severity: Correctness (returned value: the tidal acceleration, 2.04 arcsec/cty^2 off the term the next delta t takes, 4.46 s of delta t in 1800 between them; on every thread and context)**
**Where:** upstream `sweph.c` `open_jpl_file()` (the term set for the file opened); here the same, with `swi_guess_ephe_flag()` (`swephlib.c`, bug 103)

Entry 131's noticed item, bug 101's promise broken by another door.
`open_jpl_file()` sets the automatic tidal term for the file it opened,
upstream's line, and a calculation opens the file lazily. Upstream's
guess, which `swe_deltat()` takes for its ephemeris, followed the open
file, so the two agreed there; entry 103 moved the guess onto the
configuration (JPL when a JPL file has been set and read, `jpldenum_cfg`),
and the path setter forgets that DE number by design (the 2026-09-16
review's F4). So after `swe_set_ephe_path()`, `swe_set_jpl_file(
"de200.eph")`, the path again by another string (EPHEDIR/.) and a TT JPL
Sun, which opened de200.eph again by its kept name, `swe_get_tid_acc()`
answered de200's -23.8946, the next `swe_deltat()` guessed the Swiss files
and used the moon file's -25.936 (18.902020 s in AD 1800), and the getter
answered -25.936 after it. The same on the setting thread, a thread that
computed before the path, a thread started after it and an explicit
context. A fresh process given the same setters answers -25.936 and
18.902020 throughout. The compat build agrees with itself, its guess
following the open file: -23.8946 and 14.438958 s.

The reference. The configuration names no JPL DE number after the path
setter, so the guess is Swiss, as entry 132 has it for every function
without a flag: a calculation does not choose for it (G128's closecalc).
The getter reads the term the context holds, which every delta t resolves
afresh for its flag; the guess's is the one the next `swe_deltat()` takes.

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`, upstream's
line under `#else`): `open_jpl_file()` resolves the automatic term for
the ephemeris `swi_guess_ephe_flag()` names, as the setters do (bug 118).
Where the configuration names the file opened, the guess is JPL and the
term the file's, as before. The setters' own opens resolve the term
again when they are done (`setter_tid_acc_auto()`), and `swi_denum_reread()` keeps the term
of the call under way, so neither changes. No hot path: a file's open.

G132 (`tests/jpldenum.c`, entry 137) gains the observation: a TT JPL Sun at
J2000, then the getter, `swe_deltat()` in AD 1800 and the getter, for the
same 4 settings and 4 readers, against a fresh process given the setters
alone, without the Sun. 32 pass; before this fix the 4 readers after the
path again fail, the getter -23.8946 for -25.936 (before 137's fix too, 6
in all). The compat build fails 14 of 32, with the fixes and without, by
the upstream behaviour it restores: entry 137's 7, and here after the path
again its getter and delta t following the open file (4), after the path
without the file the refused Sun's sync copying the master's -25.80 over
the moon file's -25.936 (bug 101's, 3). G24 `jpl_open_tid_off`, the getter
after that sequence against a fresh context given the second path: 0
shipped, 2.04 compat (2.04 shipped before the fix).

Since entry 148 the path setter reads the kept JPL file's DE number again
along the new path, so "path, de200.eph, path again" is the configuration
"path and de200.eph": G132 compares each case with a fresh process given
the final settings, and after the path again the getter after the JPL
Sun, the next `swe_deltat()` in 1800 and the getter again are de200's,
-23.8946, 14.438958 s and -23.8946, as that process gives -- no longer the
-25.936 and 18.902020 s above, which a replay of the calls gave. This
entry's state, a calculation opening a JPL file the configuration does not
name, is still reached by the default name: G24 `jpl_open_tid_off` now
takes a path whose de431.eph (`SE_FNAME_DFT`) is de200.eph, no
`swe_set_jpl_file()`, a TT JPL Sun and the getter, against a context then
made and given that path, in a forked child called first: 0 shipped, 2.04
compat.

Noticed, not fixed: in the same state (the JPL file's name kept, its DE
number forgotten by the path setter) the first JPLEPH delta t before a
calculation has opened the file takes the default term, and the next the
file's: `swe_calc_ut()`'s JPL Moon in AD 1700 was 40.0057507520 deg on the
first call and 40.0039614086 on the second (1.79e-3 deg), where a fresh
process given the path and then de200.eph answers 40.0039614086 both
times; the compat build the same (40.0057510966, then 40.0039617599).
Reading the kept JPL file again along the new path in the path setter, as
entry 131 does after `swe_close()`, would remove it and make this entry's
guess JPL (de200's -23.8946, 14.438958 s in 1800) -- F4's choice, to be
made there. Fixed as entry 148, which made it. And the getter after a
delta t asked with a flag still
answers that delta t's term (upstream's, and bug 101's "the one the last
delta t resolved"): after a second UT JPL Moon in this state it is
-23.8946, and the next `swe_deltat()` takes -25.936.

## 145. `swe_deltat_ex()` with no ephemeris bit takes the tidal term of the JPL file the context has open, else the default, never the Swiss files' that a 0 flag means everywhere else

**Severity: Correctness (returned value: delta t 4.6 ms off in 1950 and 169.4 s in AD 1000 with de200.eph open, 0.31 ms and 11.29 s without; different on two threads, and moved by opening a file; the eclipse and house functions with a flag of 0 the same)**
**Where:** upstream `swephlib.c` `calc_deltat()` (the flag's ephemeris bits taken as given, `denum = swed.jpldenum`)

Found by entry 134's work. `calc_deltat()` takes the tidal term "consistent
with epheflag", and with no ephemeris bit in the flag there was none to be
consistent with: the DE number was the JPL file's this context had open
(`ctx->jpldenum`, upstream's line), else none, and `swi_get_tid_acc()`
then answered the default -25.80. So `swe_deltat_ex(tjd, 0)` at 1950 was
28.927550 s (de200's -23.8946) on the thread that set de200.eph, which
had opened it, and 28.931886 (-25.80) on a second thread until it opened
the file (4.3 ms); in AD 1000 1305.30 s and 1463.43. Neither is the term
of the ephemeris a 0 flag means everywhere else: `swe_calc()` reads it as
the Swiss files (SEFLG_DEFAULTEPH is SEFLG_SWIEPH), and `swe_calc_ut()`,
the star and ayanamsa UT functions and `swe_calc_orbel_ut()` set
SEFLG_SWIEPH before their own delta t, so a caller converting with
`swe_deltat_ex(tjd, 0)` and calling `swe_calc(tt, ipl, 0)` disagreed with
`swe_calc_ut(tjd, ipl, 0)`. The functions that pass the caller's flag on
unchanged took it the same way: `swe_sol_eclipse_when_glob()` from AD
1000 with a flag of 0 put the maximum 11.281 s off its SEFLG_SWIEPH
answer, in a program with the path alone; `swe_houses_ex()` there the
ascendant 3.2e-9 deg. The compat build answers as upstream.

The choice. Not the guess (`swi_guess_ephe_flag()`, bug 132's for the
functions without a flag): this function takes a flag, and its 0 is the
default ephemeris's, as in every function the flag goes on to. With
de200.eph set, a flag of 0 means the Swiss files to `swe_calc()`, and
answering de200's term would split the two again.

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`):
`calc_deltat()` reads a flag with no ephemeris bit as SEFLG_SWIEPH, before
it picks the DE number. `iflag == -1` is the default term as before. On
the UT path (one compare): 20000 x 10 `swe_calc_ut()` 20792848981
instructions before, 20733635298 after; calc-swieph and calc-moseph call
`swe_calc()` and reach no delta t.

G132 (`tests/jpldenum.c`, entry 137) gains the observation:
`swe_deltat_ex()` in 1950 with a flag of 0 and its serr, with
SEFLG_SWIEPH, the getter, and a flag of 0 in AD 1000, the first two equal
and all bit for bit as the fresh process, for the 4 settings and 4
readers. 48 pass; before this fix the 16 new ones fail (28.927550 s where
the reader has de200.eph open, 28.931886 elsewhere, for 28.932196); the
compat build the same 16, 30 of 48 in all. G24 `dt_flag0_off`, a flag of
0 in AD 1000 against SEFLG_SWIEPH with de200.eph open: 0 shipped, 169.4 s
compat (and shipped before the fix). The golden transcript is unchanged.

## 139. With the fallback allowed, a Swiss or JPL barycentric request whose files are missing is answered from Moshier, which serves no barycentric place

**Severity: Correctness (behaviour: a wrong answer labelled as the centre asked for, Moshier's heliocentric place under SEFLG_BARYCTR, Mars 0.24 deg and 7.2e-3 AU off)**
**Where:** `sweph.c`: `swe_calc_r()`, `swe_calc_orbel_r()`; through them `swe_calc_ut()`, `swe_calc_orbel_ut()`, `swe_calc_pctr()`, `swe_nod_aps()`

Found by entry 135's work. `swecalc()` refuses a barycentric Moshier
request before it computes anything: -1, "barycentric Moshier positions
are not supported.". A Swiss or JPL request passes that test, and when its
files are missing falls to Moshier further down (`main_planet()`, the
Moon's branch, the fictitious bodies' Earth). Moshier's origin is the Sun,
so the barycentric place came out heliocentric, under a return flag with
SEFLG_BARYCTR and SEFLG_MOSEPH set (18180 with SEFLG_SPEED). Upstream's
substitution is unconditional, so upstream answers so wherever the files
are missing; here `swe_set_ephe_fallback(1)` (or `SE_EPHE_FALLBACK=1`)
restores it, and the strict default refuses the request for its files.

Measured at 2000 Jul 1.5 in a directory without the files, the fallback
allowed, Swiss and JPL (de200.eph named and missing) alike, each in a
fresh process:

| request | answered | off the files' barycentric place |
|---|---|---|
| Mars (also `swe_calc_ut()`, SEFLG_SPEED3, J2000 XYZ) | 18180, Moshier's heliocentric Mars bit for bit | 0.24 deg, 7.2e-3 AU |
| the Earth, the Moon | 18180 | 0.37 deg |
| Jupiter, Pluto | 18180 | 0.027, 8.5e-3 deg |
| Cupido (seorbel.txt) | 18180 | 5.1e-3 deg |
| `swe_calc_orbel()`, a = 1.2 AU about the Sun | 18180 | 0.18 deg |
| `swe_calc_pctr()` Mars from Jupiter, SEFLG_BARYCTR | 1794 (SEFLG_SWIEPH), from the two heliocentric places | 1.6e-4 deg |
| `swe_nod_aps()` Uranus, SE_NODBIT_OSCU_BAR | 0, the heliocentric ellipse | 0.025 deg (node, geocentric) |

The Moshier request of each is refused, by `swecalc()` or by the inner
barycentric `swe_calc()` of `swe_calc_pctr()` and of the barycentric
ellipse. `swe_calc_pctr()` without SEFLG_BARYCTR was refused already, by
its deflection's barycentric Sun, for that file.

**The right answer.** A refusal, with the Moshier request's message. The
manual (swephprg, SEFLG_BARYCTR): "It works only with the JPL and the
Swiss Ephemeris, not with the Moshier ephemeris"; swisseph.htm: "Moshier
provides heliocentric positions". Nothing in the library says Moshier
answers a barycentric request. The fallback exists so that a caller gets
Moshier's answer where the files are missing, and `swephexp.h` has the
return flag say which ephemeris answered. A fallback cannot then answer
what the ephemeris it falls to refuses, under a flag the library never
returns for that ephemeris. Serving the place from `swi_mosh_sun_bary()`
(the barycentric Sun Moshier's light time has carried since entries 102,
109 and 125) would leave a direct Moshier request refused, the same
disagreement the other way, and would make that internal model a public
barycentric ephemeris. Not done.

Checked and unchanged, both settings:
- the barycentric Sun: the Swiss branch has no fallback; refused for its
  file;
- the asteroids (Ceres, Chiron; a numbered one) and the planetary centres:
  no Moshier; refused for their files;
- the nodes and apsides, barycentric: zero vectors under the flag asked,
  as with the files; there is no barycentric node (the manual: "only with
  physical bodies"), and no file is read;
- SE_ECL_NUT, and Aldebaran barycentric (`swe_fixstar2()`, `swe_fixstar()`):
  no ephemeris read, the complete directory's answer bit for bit. A
  Moshier barycentric star is answered too (18180), the same place: not a
  wrong answer;
- with only the Moon files missing, the barycentric Moon is Moshier's
  geocentric Moon on the files' Earth, a barycentric place, 2.0e-7 deg
  off the files' (5.2e-7 deg in 1900), flagged SEFLG_MOSEPH by entry 120.
  It stays answered;
- strict: every request above refused with the downgrade's message, as
  before.

**Fix, as done here:** `swi_refuse_mosh_bary()`: an answer whose flag has
SEFLG_BARYCTR and SEFLG_MOSEPH is refused. Under the strict setting it is
refused as any substitution is, with `swi_refuse_downgrade()`'s message
(the same text as before); with the fallback allowed, with `swecalc()`'s.
- `swe_calc_r()` tests `swecalc()`'s own return flag, right after it,
  before entry 120 relabels the Moon of a missing Moon file SEFLG_MOSEPH:
  tested after that, the barycentric place above would be refused too. A
  refused answer stays in the save area as before; it cannot answer
  later, its flag being no request's (a request naming Moshier changes the
  ephemeris, which forgets the save areas).
- `swe_calc_orbel_r()` tests its answer's flag after its strict check: a
  two-line call, as `swe_calc_r()` refuses it.
- `swe_calc_ut()`, `swe_calc_orbel_ut()`, `swe_calc_pctr()` and the
  barycentric ellipse of `swe_nod_aps()` inherit it through those calls.
- Behind `#ifndef SWE_UPSTREAM_COMPAT`: upstream answers so. The compat
  build's `swe_calc_orbel()` (this fork's) keeps answering as its
  `swe_calc()` does, as for entry 135.
- Cost: instructions +0.0003% on calc-swieph, +0.0004% on calc-moseph.

Left as it is, not this entry's:
- `swe_nod_aps()` with barycentric output on Moshier, asked for or fallen
  to, answers the heliocentric points: Mars's osculating ascending node
  49.5626 deg at 2000 Jul 1.5, Moshier's heliocentric node to the last
  digit, where the files' barycentric one is 49.6398 (0.077 deg); the
  Moon's mean node 0.37 deg. Nothing in it refuses SEFLG_MOSEPH with
  SEFLG_BARYCTR; only its SE_NODBIT_OSCU_BAR ellipse goes through
  `swe_calc()`'s refusal. The fallback answers as the Moshier request
  does. A defect of its own, measured, not fixed. Fixed as entry 149,
  which refuses it as `swe_calc()` does.
- `swe_calc_pctr()` returns the caller's flags: with the fallback allowed
  and only the JPL file missing, Mars from Jupiter returns 257
  (SEFLG_JPLEPH) with the Swiss answer bit for bit. Measured, not fixed.
  Fixed as entry 150: the ephemeris that answered.

G133 `tests/fbbary.c`: links to sefstars.txt and seorbel.txt alone, each
case in a forked process, 2000 Jul 1.5 and 1900 Jan 0.75, Swiss and JPL:
the calls of the table, with `swe_calc_orbel_ut()`, `swe_calc_pctr()`
without SEFLG_BARYCTR and the ellipse's barycentric output, refused with
the fallback with the Moshier request's return and message, and strict
with the downgrade's; every one answered
from the complete directory; the unchanged classes above as they were; the
Moon files alone missing, the barycentric Moon answered within 1e-6 deg;
and in one warm process the refusals repeated, Moshier asked with the
substituted answer's flags (18180) refused, and heliocentric Mars around
them answered alike. 368 checks pass. Before the fix 72 fail: the 60
fallback calls and 12 warm. The compat build fails 82: those 72, its
`swe_calc_orbel()` answering the 8 Moshier requests (entry 135 restored),
and the Moon files' Moon twice (SEFLG_SWIEPH, entry 120 restored).

G126 check 3 now has 400 of its 720 calls refused by both (320 before:
the 80 fallback barycentric calls, refused by both now). It passes before
and after. G99, G117, G119, G109, G87, G2, check-ctx and G24 pass.

G24 `fallback_mosh_bary_ret`: a Swiss barycentric Mars at 2000 Jul 1.5,
an empty directory, the fallback allowed: -1 shipped, 18180 compat.

## 146. The nutation interpolation's nodes lie both sides of `SEMOD_JPLHORA_3`'s step at 1962 Jan 20: dpsi there 1.95e-2" off the exact nutation, 6.93e-2" at worst

**Severity: Correctness (dpsi and every place of date within a day of 1962 Jan 20 up to 6.93e-2" (1.9e-5 deg) off, with the interpolation on and SEFLG_JPLHOR_APPROX under its default mode; the interpolation is otherwise within about 3 mas)**
**Where:** `swephlib.c`: `swi_nutation()`

Entry 134's noticed item. With the interpolation on, `swi_nutation()`
answers from a quadratic through the nutation at three nodes a day apart,
t - 1, t and t + 1 of the call that built them, for any instant strictly
between the outer two. What it interpolates, `calc_nutation()`, has a
step: with `SEFLG_JPLHOR_APPROX` and `SEMOD_JPLHORA_3` (the default
mode) it takes IAU 1980 plus a fixed offset (64.284 and 6.151 mas) up to
`HORIZONS_TJD0_DPSI_DEPS_IAU1980`, JD 2437684.5 = 1962 Jan 20
(inclusive), and the nutation model after it. Measured on de200 at the
switch, exactly (the instant and the next double):
- the default model (IAU 2000A): dpsi -10.074409" before, -10.146158"
  after, 7.17e-2"; deps 2.96e-3";
- IAU 1980 and Herring's 1987: the offset itself, 6.43e-2" and 6.15e-3";
  IAU 2000B 7.20e-2"; Woolard 0.267".
Nodes built on both sides interpolate across it. After a call at the
switch - 0.5, SE_ECL_NUT at + 0.3 answered dpsi -10.105320" for
-10.124814" (1.95e-2"), Mars's longitude the same. Over every node centre
within 1.5 day of it by 1/64 day and every query between its nodes by
1/16: 6.93e-2" at worst on the default model, before the switch, after it
and at it alike, 6.20e-2" on IAU 1980. Nodes all on one side answer
within 1.03e-3" before it and 4.27e-4" after. Both builds.

The rest of what `calc_nutation()` returns, searched for steps:
- `SEFLG_JPLHOR` (IAU 1980 plus the IERS corrections of
  eop_1962_today.txt, by `bessel()`): the table's interpolation is
  continuous at its days, and before 1962 Jan 20
  (`eop_tjd_beg_horizons`), before the table's first day and past its
  last day it holds the value there: a kink, not a step. On G19's
  synthetic DE431 header with a 40-day table whose corrections move 0.3
  mas a day with 0.3 mas of noise, the value across the first and the last
  day moves 8.6e-12" and 5.1e-11" (the series over one ulp), and the
  interpolation within 1.5 day of them is within 8.8e-4" and 1.41e-3", as
  inside the table (2.35e-3").
- `SEMOD_JPLHORA_2` adds a constant offset to IAU 2000, `SEMOD_JPLHORA_1`
  nothing: neither has a date, and across the switch's date both answer as
  without the flag (3.5e-11", one ulp of the series).
- The series (IAU 1980, Herring 1987, IAU 2000A and B, Woolard) have no
  date in them but T.
- The default's grid gives way to the direct series at |J| = 1e12, a
  guard for absurd dates (G25). The grid has left the series long before
  that (noticed below), so its step there is the grid's own defect, not one
  to keep the nodes off; with the grid where it holds the step would be
  below the floor.
So the one step is the switch, under `SEFLG_JPLHOR_APPROX` without
`SEFLG_JPLHOR` (which wins in `calc_nutation()`) and under mode 3.

**Fix, as done here:** where the nodes a call would build, t - 1.0, t and
t + 1.0, lie both sides of the switch (t - 1.0 <= it < t + 1.0) under
those flags and that mode, `swi_nutation()` returns `calc_nutation()` at
t and leaves the nodes as they were. No set of nodes spans the step, so
none interpolates across it: nodes all at or before it answer only before
it, nodes after it only after. Nodes of another key (entry 134) are
rebuilt through the same test. Within a day of the switch the answer is
the exact nutation, one series a call where the interpolation took a
quadratic; elsewhere nothing changes. Behind `#ifndef
SWE_UPSTREAM_COMPAT`, upstream's interpolation under `#else`.
- After: 0 within a day of the switch, before it, after it and at it, on
  every model; nodes on one side keep their 1.03e-3" and 4.27e-4".
- Away from the switch the interpolation is bit for bit the tip's: a
  probe's 192000 queries 1800-2100 on four settings (the default, mode 3,
  IAU 1980 under mode 3, mode 2; 3.06 mas worst, the interpolation's own
  "about 3 mas"), and 24366 under `SEFLG_JPLHOR` on the fixture above,
  hashed the same before and after.
- No hot path: the interpolation is off by default, and the test runs
  only where nodes are built. `perf stat` instructions, `swe_calc()` of the
  ten bodies with speed at 20000 instants on the Swiss files:
  15892588394 before and 15887959108 after with it off, 15901468958 and
  15897329213 with it on (-0.03%, layout). The golden transcript is bit
  for bit the tip's (13000 rows).

G122 `tests/nutintp.c`, extended with 104 checks on explicit contexts:
SE_ECL_NUT and Mars at the switch + 0.3 after a call at - 0.5, bit for bit
with the return against a context with the interpolation off; and scans:
each node centre c within 2 days of the switch by 1/4 day, after a call
at c + 500 and at c, the queries c + j/4 (|j| <= 3) against it off, bit
for bit where c's nodes straddle the switch and within 3.5 mas elsewhere,
on the default model and on IAU 1980 (17 centres each); and where nothing
steps, within 3.5 mas throughout, `SEMOD_JPLHORA_2` and no flag at the
switch's date and `SEFLG_JPLHOR` about the first and the last day of entry
134's fixture A. 350 checks in all, none fail. Before the fix 18 of the
104 fail: the two at + 0.3 (1.95e-2") and the eight straddling centres on
each model, 2.74e-2" to 6.01e-2" on the default and 2.46e-2" to 5.38e-2"
on IAU 1980. The compat build fails 131 of 350, entry 134's 113 and these
18. G24 `nut_step_intp_off`: dpsi at the switch + 0.3 after a call at
- 0.5, and at - 0.3 after one at + 0.5, against the interpolation off, the
larger: 0 shipped (1.96e-2" before the fix), 1.97e-2" compat. G19, G29,
G32 and G57 pass; G1's transcript is identical to the tip's.

Noticed, not fixed:
- Woolard's nutation (`SEMOD_NUT_WOOLARD`, `calc_nutation_woolard()`)
  turns the Moon's mean anomaly at 13255523.59 revolutions a century, 10^4
  times its 1325.552359 (1325r 198 deg 50' 56".79): its five terms in md
  have a period of 3.97 minutes. dpsi up to 0.238" and deps 0.0226" off
  the same series with the right rate (20000 instants 1900-1975), and the
  interpolation on it 0.151" off away from any step. Both builds. Fixed
  as entry 151.
- The default's grid (`calc_nutation_grid()`) leaves the direct IAU 2000A
  series far inside its guard of |J| < 1e12: worst over instants between
  its nodes 4.5e-10 deg at |J| 5.6e8 (1.5 million years), 8.0e-7 deg at
  1e9, 2.8e-3 deg at 1.8e9, 8.7 deg at 3.2e9 and 5.4e25 deg near 1e12,
  where `swe_calc(SE_ECL_NUT)` on Moshier answers without an error. No
  model holds there; the direct series stays bounded. Fixed as entry
  152.
- The speeds across the same switch, with the interpolation off: for t in
  (switch - 1, switch] Mars's longitude speed of date under
  `SEFLG_JPLHOR_APPROX` is 1.571e-5 deg/day off a five-point difference of
  its positions over 2^-8 day on that side (RA 1.71e-5), 1e-11 outside
  that day. The precession's speed for a model other than Vondrak's,
  (P(t + 1) - P(t - 1)) / 2 (entry 49's fallback), and the obliquity's
  rate, eps(t + 1) - eps(t), reach past the switch, where the precession
  and the obliquity change model with the nutation. Within
  `NUT_SPEED_INTV` (1e-4 day) of it the nutation's rate, a central
  difference there, takes its 7.17e-2" step: the speed 9.96e-2 deg/day
  off at the switch - 2^-16 and - 2^-14 day. Entry 153, fixed there.

## 153. Under `SEMOD_JPLHORA_3` the frame's rates difference its models across their step at 1962 Jan 20: every speed of date 1.57e-5 deg/day off for a day before it, 9.96e-2 within 1e-4 day

**Severity: Correctness (with SEFLG_JPLHOR_APPROX under its default mode, on a JPL file: every speed of date 1.57e-5 deg/day off in longitude for a day before 1962 Jan 20 0h TT, Polaris's right ascension 1.31e-4; 9.96e-2 within 1e-4 day of it, Polaris's right ascension 1.09; SEFLG_SPEED3 5.75e-2 within 1e-4 day either side)**
**Where:** `sweph.c`: `swi_precess_speed()`, `swi_obliquity_rate()`, `swi_get_observer()`, `swe_calc()`'s SEFLG_SPEED3; `swephlib.c`: `swi_nutation_rate()`, new `swi_frame_step_side()`

Entry 146's noticed item. With `SEFLG_JPLHOR_APPROX` and
`SEMOD_JPLHORA_3` (the default mode), `swi_precess()`, `swi_epsiln()` and
`calc_nutation()` take SEFLG_JPLHOR's models up to
`HORIZONS_TJD0_DPSI_DEPS_IAU1980`, JD 2437684.5 = 1962 Jan 20 0h TT
(inclusive) -- the IAU 1976 precession and obliquity, IAU 1980 nutation
plus a fixed offset -- and the chosen models after it. Measured on de200
at the step, between the instant and 2^-20 day after it less the motion:
the precession (SEFLG_NONUT) 113.2 mas in Mars's longitude, 6.4 mas in
its latitude, 123 mas in its right ascension, 942 mas in Polaris's; the
mean obliquity 3.19 mas; dpsi 71.75 mas, deps 2.96 mas (entry 146). Speed
is the rate of the position (entry 31), so on each side of a step it is
the rate of that side's model. The rates that difference the models
reached across it:
- the precession's for a model other than Vondrak's, entry 49's fallback
  (P(t + 1) x0 - P(t - 1) x0) / 2: the model is IAU 1976 before the step,
  so for t in (switch - 1, switch] P(t + 1) was the other model, half the
  step a day: every longitude speed of date 1.57e-5 deg/day off (the Sun,
  the Moon, Mars, Polaris, Spica), right ascension 1.67e-5 to 1.71e-5,
  Polaris's 1.31e-4. After the step too when the model chosen is not
  Vondrak's: with IAU 2006, 1.57e-5 within a day after it;
- the topocentric site's precession in right ascension (entry 73), the
  same difference of the date's axes: 1.58e-7 deg/day more on the
  topocentric Moon's speed (Zurich);
- the mean obliquity's rate, eps(t + 1) - eps(t) (entry 33): for t in
  (switch - 1, switch] the whole step a day, 8.9e-7 deg/day (3.19 mas;
  the ecliptic latitude speeds up to 8.85e-7 by it alone, 4.9e-7 net of
  the precession's); with IAU 2006 after the step 42 mas, 9.35e-6;
- the nutation's rate, a central difference over `NUT_SPEED_INTV` = 1e-4
  day (entry 58) wherever the default model's grid gives no derivative --
  before the step -- and for any other model: within 1e-4 day before it
  every longitude speed 9.96e-2 deg/day off, right ascension up to 0.108,
  Polaris's 1.09, declination up to 3.85e-2 (Spica); within 1e-4
  day after it for a chosen model, IAU 2000B 0.100, IAU 1980 8.93e-2;
- `SEFLG_SPEED3`'s three places over 1e-4 day (entry 31): within 1e-4 day
  either side, Mars's longitude speed 5.75e-2 off, its right ascension
  6.32e-2.

Measured against five-point differences of the same call's positions over
2^-8 day on the instant's side of the step (centred where that fits, else
one-sided), at every 1/64 day across switch -+ 2 and at switch -+ 2^-k,
k = 7..26; with the interpolation on the same (entry 146 computes the
nutation at the instant there). Upstream has the step and its rate the
other way: a backward difference of the nutation, 0.200 deg/day off
within 1e-4 day after the step.

Not affected: sidereal places (`plaus_iflag()` drops the Horizons flags
under SEFLG_SIDEREAL: sidereal Mars within 4.1e-10 throughout), the nodes,
apsides and fictitious bodies (dropped likewise), the Swiss files and
Moshier (the flags need a JPL file), the ayanamsa (its flags are masked to
the ephemeris bits), `SEMOD_JPLHORA_1` and `_2` (no date). The frame bias
and the Horizons offset in right ascension (`swi_bias()`,
`swi_approx_jplhor()`) are fixed rotations on each side, and no rate
differences them (but see below).

**Fix, as done here:** `swi_frame_step_side(ctx, t, h, iflag)` says which
side a difference of half-width h about t must keep to: -1 when t is at
or before the step and t + h after it, +1 when t is after it and t - h at
or before it, 0 otherwise, and under any other flags or mode
(`SEFLG_JPLHOR`, which wins in all three models, has no step). Where it
is not 0:
- `swi_precess_speed()`'s fallback and the site's precession in right
  ascension take the three-point derivative on t's side, from P(t),
  P(t + s) and P(t + 2s), s = -+1 day: s (4 P(t + s) - 3 P(t) - P(t +
  2s)) / 2;
- `swi_obliquity_rate()` the difference back, eps(t) - eps(t - 1) (the
  forward difference after the step never reaches across it);
- `swi_nutation_rate()` the three-point derivative over t, t + s h and t +
  2 s h, with the instants' exact offsets;
- `SEFLG_SPEED3` a third place at tjd + 2 s dt, taken before tjd's (which
  the save areas keep), and the three-point slope on tjd's side
  (`calc_speed_side()`), over the exact offsets.

The one-sided differences are of the central ones' order; their
truncation, h^2 f''' / 3, is 1e-19 rad/day on the precession and
1e-10"/day on the nutation. Elsewhere nothing changes: the test is a few
comparisons, true only with SEFLG_JPLHOR_APPROX under mode 3 within h of
the step. Behind `#ifndef SWE_UPSTREAM_COMPAT`; the fallback, the
obliquity's, the nutation's and the site's rates are this fork's,
upstream's SPEED3 is under `#else`.
- After: the day before the step, the day after and the rest alike, within
  the reference's own noise: the Sun, Mars and Spica within 4e-10 deg/day,
  Polaris 6e-11 in longitude and 1.9e-9 in right ascension (one-sided over
  2^-8, the reference's rounding times 1/cos(declination) = 82; 7.3e-10
  over 2^-6), the Moon 1.0e-9 (over 2^-6), the topocentric Moon 5.4e-10
  (delta t set to 2^-11 so that its UT1 is exact, a nine-point difference
  over 2^-7 where it fits), with the default models, IAU 2006 with IAU
  2000B, and IAU 1980 nutation; SPEED3 1.41e-6, its own everywhere.
- The site's acceleration in the aberration rate (`observer_accel()`, a
  central difference of the site's velocity over -+2^-6 day, entry 79)
  still reaches across the step within 1/64 day of it: the topocentric Sun
  7.3e-10 deg/day there against 1.9e-10 elsewhere (delta t exact). Below
  the floor, left.
- No hot path: the test runs in the fallback, the SPEED3 branch and the
  nutation's differenced rate, and once per obliquity rate. `perf stat`
  instructions, `swe_calc()` of the ten bodies with speed at 20000 instants
  on the Swiss files: 20152347160 before, 20151689151 after. G1's
  transcript is bit for bit the tip's (13000 rows).

G139 `tests/framestep.c`: de200, SEFLG_JPLHOR_APPROX, the default models
and IAU 2006 precession with IAU 2000B nutation ("0,8,8,4", which
difference after the step too); the Sun, the Moon, Mars, Polaris, sidereal
Mars, the topocentric Moon and Mars under SEFLG_SPEED3, ecliptic and
equatorial, at 105 instants across (switch - 2, switch + 2], against
five-point differences over 2^-6 day on the instant's side (the
topocentric Moon's as above); the worst of the two speeds over the day
before the step, the day after it and the rest: 84 checks within 1e-8
deg/day (SPEED3 within 1e-5). Measured within 1.45e-9 (Polaris's right
ascension under the second setting), SPEED3 1.41e-6. Before the fix 38 of
84 fail: every body but sidereal Mars on the day before, under both
settings, and on the day after under the second, SPEED3 on both days under
both. The upstream build fails 80 of 84: its backward nutation rate across
the step after it (0.200 deg/day, Polaris's right ascension 2.14) and,
elsewhere, its own speed defects that earlier entries fixed (the
topocentric Moon 4e-3, sidereal Mars 5e-7). G24 `jplhora_step_speed_off`:
Mars's ecliptic longitude speed at the step - 0.5 day and -+ 2^-14 day
against the five-point difference on its side, the largest: 4.19e-11 deg/day
shipped (9.96e-2 before the fix), 0.200 compat. G32, G57, G122, G90, G75,
G89 and G40 pass.

Noticed, not fixed:
- The same differences reach across the other steps of `swi_precess()`'s
  and `swi_epsiln()`'s models. SEFLG_JPLHOR's IAU 1976 holds strictly
  between 1799 Jan 1 and 2202 Jan 1, Owen 1990 outside, and mode 3 takes it
  before 1962 too: at 1799 Jan 1 under mode 3 (the mean obliquity 17.57
  mas, the precession 3.6 mas in Mars's longitude, 4.4 mas in its
  latitude), within a day either side the Sun's latitude speed is 5.0e-6
  deg/day off, Polaris's right ascension 5.6e-6, Mars's 8.9e-7, SPEED3
  8.9e-3. A short-term model the caller chooses
  (`SEMOD_PREC_IAU_1976` or `_IAU_2000` within 2 centuries of J2000,
  `_IAU_2006` within 75) steps to the long-term one: with IAU 1976 short
  and Vondrak long ("0,9,1"), at J2000 - 2 cy 8.0e-5 deg/day within a day
  of it, SPEED3 0.80. `swi_frame_step_side()` would take them as more
  steps, each with its own inclusive side.
- In mode 3 `swi_bias()` leaves the frame bias and the Horizons offset in
  right ascension out for tjd < the step, where the three models change at
  tjd <= it: at exactly 1962 Jan 20 0h TT a place is the left side's
  precession and nutation with the right side's bias (23 mas) and offset
  (-51.26 mas in right ascension). Only with a JPL file of DE >= 403,
  none here to measure. Measured and fixed as entry 161 (with the
  light-time instant the callers gave it).
- `swi_approx_jplhor()` turns the position by that offset and not the
  velocity, and leaves the table's rate out: the right ascension speed off
  by the offset times rho'/rho, estimated 1.4e-7 deg/day on Mars near
  quadrature (2.5e-7 rad times 0.01/day), 6e-8 on the Moon; after 1962
  under mode 3, and always under `SEMOD_JPLHORA_1`, with DE >= 403. Not
  measured here. Measured and fixed as entry 162 (the table's rate, 3.8e-10
  deg/day at most, left out).

## 140. The topocentric site is placed by UT1 = TT - delta t rounded to a double: a staircase in delta t, whose rate leaves out delta t's that the velocity carries, 1.8e-8 to 4.7e-8 deg/day on an orbit 0.01 AU from the Earth

**Severity: Correctness (speeds against their positions' rate: 1.8e-8 to 4.7e-8 deg/day on the Earth-centred orbit, 2.8e-7 on de200; the topocentric Moon 2.5e-7 to 4.3e-7; places 6 mm at the site, up to 9e-10 deg on the Moon)**
**Where:** `sweph.c`: `swi_get_observer()`

Recorded by entry 136 as a residual of the 0.01 AU orbit about the Earth:
its topocentric speed 1.8e-8 to 4.6e-8 deg/day off five-point differences
of its positions, flat over steps 2^-8..2^-12 day, on every equinox and on
the seorbel.txt path alike. Not the light time, the focus or the diurnal
aberration's rate: with SEFLG_TRUEPOS, where the Earth cancels and only
the two-body orbit less the site is left, it was the same 4.5e-8; with a
user delta t of 2^-11 day, exact and constant, 2.3e-9. The site itself:

- `swi_get_observer()` takes UT1 as tjd - delta t, a double: 2^-31 day
  (40 us) apart at JD 2.4e6. Delta t changes by 3e-9 to 4e-8 of a day a
  day, so over a five-point stencil (2^-8 day at h = 2^-10) by less than
  an ulp: the rounded UT1 follows TT with delta t frozen, and steps by an
  ulp every ulp / (d delta t / dt), 18 minutes to 3.5 hours. The site was a
  staircase in delta t, steps of 8.5e-14 AU (13 mm at 47 N).
- Its velocity carries the rotation per TT day, (1 - d delta t / dt)
  (entry 36). So against five-point differences of the positions it was
  off by -d delta t / dt of itself: 1.13e-8, -3.76e-8, -8.2e-9, 3.3e-9
  relative (1700, 1900, 2000, 2025) for d delta t / dt of -1.12e-8,
  3.77e-8, 8.25e-9, -3.23e-9; at 2150 -6.1e-8 for 2.2e-8, and on de200 at
  1700 -2.7e-7 for -8.2e-9: a step inside the stencil.
- On the 0.01 AU orbit (the site's motion 1.0 deg/day of its topocentric
  one): 4.4e-8 deg/day Swiss, 4.5e-8 Moshier, 2.6e-7 de200 over eleven
  flag sets, J2000 and of date, `swe_calc_orbel()` and seorbel.txt alike.
- Every topocentric place has the site: the topocentric Moon's speed
  against five-point differences at Zurich, ten epochs -2900..2900, 2.5e-7,
  2.7e-7 and 4.3e-7 deg/day at steps 2^-8, 2^-9 and 2^-10 day; G72's (b)
  2.51e-7, G43's Moon 1.28e-7.
- The place: up to half an ulp of UT1 off, 6 mm at 47 N, 2.4e-10 deg on
  the 0.01 AU orbit and up to 9e-10 on the Moon, below the position
  floor; the speeds' are above it.

**Fix, as done here:** the rounding of tjd - delta t is recovered exactly
(Knuth's two-sum: bb = tjd_ut - tjd, err = (tjd - (tjd_ut - bb)) + (-delt
- bb)) and the site's sidereal time moved by err at the rotation's rate,
360 x 1.00273781191135448 deg a day (the precession in right ascension's
1e-7 of it is 1e-14 deg over the 2.3e-10 day). Behind `#ifndef
SWE_UPSTREAM_COMPAT`, inside the true-pole block (entry 27) where the
site's sidereal time is taken; the compat build places upstream's site.
Six operations a topocentric observer; the benches make no topocentric
call.

After: the site's velocity agrees with five-point differences of its
positions within 4.9e-11 of itself (8.9e-15 AU/day). The 0.01 AU orbit's
topocentric speed within 2.3e-9 deg/day on the Swiss files, 9.2e-10 on
de200, 1.3e-9 on Moshier, at 2^-10 day, both equinoxes, `swe_calc_orbel()`
and seorbel.txt alike (2^-8: 1.2e-8, the difference's truncation; 2^-9
1.8e-9; 2^-12 1.5e-8, the positions' rounding over the step). The
topocentric Moon: 4.5e-8, 6.3e-9 and 8.5e-9 deg/day at 2^-8, 2^-9 and
2^-10 day; G72's (b) 7.46e-8 (its 2^-8 day's truncation), (c) 1.06e-8 as
before; G43's Moon 4.59e-8 at Zurich, 1.98e-8 at 70 N.

G1 moves 236 of its 13000 rows against the tip before this change, all
through the observer: topo (180) by up to 8.5e-10 deg in place and
4.6e-9 deg/day in speed; the local eclipse and occultation times by an ulp
of the JD (4.66e-10 day); helpheno and vislim by up to 1.5e-9;
cov:lun_occult_where's place by 5.2e-8 deg; solecl_where[5] and
solecl_how_max[5] attr[0] by 5.0e-7, a near-central eclipse whose Moon-Sun
separation at the maximum found is the search's own residual, 2.56e-6 deg
before, 2.83e-6 after.

G126 `tests/orbelbias.c` (entries 130, 135, 136, 141): check 4 takes the
topocentric centre about the Earth back, 120 rows on the Swiss files and
de200, 90 on Moshier. Before the fix 3 failures, 4.7e-8 deg/day Swiss,
2.8e-7 de200, 4.6e-8 Moshier; after it passes, check 4 within 4.0e-9 (the
1.2 AU orbit). The compat build fails 53 as before, that orbit's check 4
now 3.4e-4 deg/day with its topocentric rows.

G24 `topo_ut_round_off`: the site (the Sun's geocentric less topocentric
true J2000 equatorial ICRS XYZ place) at TT 2000 Jan 1.5, Zurich, Swiss
files, with a user delta t of 2^-11 day and 2^-11 + 2^-33 (a quarter ulp
of UT1): its displacement against -v 2^-33 day, relative. 1.73e-3
shipped, 1.00 compat and before the fix (the quarter ulp rounded away).

G126, G87, G107, G48, G27, G65, G80, G95, G43, G99, G72 and G24 (both
ways) pass; G1 moves as above.

## 141. `swe_calc_orbel()` returns SEFLG_SWIEPH where no ephemeris was named, and drops SEFLG_RADIANS with SEFLG_XYZ: not `swe_calc()`'s return flag (this fork's)

**Severity: Correctness (behaviour: a return flag that differs from `swe_calc()`'s for the same flags; the answer itself the same)**
**Where:** `sweph.c`: `swe_calc_orbel_r()` (and through it `swe_calc_orbel_ut_r()`)

`swe_calc_orbel()` returned the flags as the Earth's computation left them
(`pldat[SEI_EARTH].xflgs`, the light time's Moshier redo applied). The
answer was right; the flag was not `swe_calc()`'s, which returns its
answer's flags with the caller's coordinate bits (SEFLG_EQUATORIAL,
SEFLG_XYZ, SEFLG_RADIANS) and, where the caller named no ephemeris,
without the default's bit ("if no ephemeris has been specified, do not
return chosen ephemeris"). Measured against `swe_calc()` of the same
elements from a seorbel.txt, files present, 2000 Jul 1.5:

- no ephemeris bit: SEFLG_BARYCTR | SEFLG_SPEED returned 18178 for
  18176, 0 returned 2 for 0, every flag set and centre the default's bit
  more (SEFLG_SWIEPH, 2); a fallback to Moshier says SEFLG_MOSEPH in both;
- SEFLG_XYZ | SEFLG_RADIANS: the radians bit, which `swe_calc()` drops for
  the computation as this function does, came back cleared, 4354 for
  12546 with SEFLG_SWIEPH | SEFLG_SPEED, on every ephemeris and centre and
  through `swe_calc_orbel_ut()` as well.

`swe_calc_orbel_ut()` names the ephemeris for its delta t as
`swe_calc_ut()` does, so its no-bit returns already agreed. Both builds
behaved the same way.

**Fix, as done here:** `swe_calc()`'s own statement at the end of
`swe_calc_orbel_r()`: the answer's flags with the caller's SEFLG_COORDSYS
bits, and SEFLG_DEFAULTEPH cleared when the caller's flags had no
ephemeris bit. Unguarded, on entry 112's precedent: the function is the
fork's and follows the fork's `swe_calc()` in both builds; no G24 case.
Not a hot path: two masks a call.

Left as it is: SEFLG_SPEED3 is refused by contract ("SEFLG_SPEED3 is not
supported for orbital elements"), where `swe_calc()` answers a fictitious
body with three positions.

G126 `tests/orbelbias.c` gains a check 5, files present: check 1's twelve
flag sets and SEFLG_RADIANS with and without SEFLG_XYZ, every centre, the
Swiss files, de200, Moshier and no ephemeris bit, each orbit at one date,
through `swe_calc_orbel()` and `swe_calc_orbel_ut()` against `swe_calc()`
and `swe_calc_ut()`: 3584 calls, the same return and message, Moshier's
barycentre (224) refused by both. Check 3 (files missing, strict and with
the fallback) takes no ephemeris bit too: 960 calls, 440 refused by both,
the same before and after. Before the fix check 5 fails in both builds,
656 calls: the 448 with no ephemeris bit through `swe_calc_orbel()`, and
240 with SEFLG_XYZ | SEFLG_RADIANS (32 among those). After, G126 passes;
the compat build fails 53 (54 before), the others its copy of upstream's
fictitious path's (entry 130's guard, G27's light time, entry 88's first
call).

## 142. `swe_get_ayanamsa_ex()`, its `_ut` form, the sidereal houses and `swe_gauquelin_sector()` with SEFLG_JPLHOR and no IERS corrections loaded read the corrections' table, NULL, and crash

**Severity: Crash (a NULL dereference; with an eop_1962_today.txt that breaks off, a read 2.45e6 doubles before the table), both builds**
**Where:** `swephlib.c`: `calc_nutation()`; `sweph.c`: `swe_get_ayanamsa_ex_r_body()`; `swecl.c`: `swe_gauquelin_sector_r_body()`

Entry 134's noticed item. `plaus_iflag()` turns SEFLG_JPLHOR into
SEFLG_JPLHOR_APPROX when no corrections are loaded (`eop_dpsi_loaded` <=
0: no `swe_set_jpl_file()` of a DE >= 403 on this context, no
eop_1962_today.txt, or one the loader gave up on), so `swe_calc()` never
asks `calc_nutation()` for them without them. Three public functions take
their flags past it to the nutation: `swe_get_ayanamsa_ex()` adds the
nutation of the caller's flags to `swi_get_ayanamsa_ex()`'s value (which
does take them through `plaus_iflag()`, and drops the Horizons bits);
`swe_get_ayanamsa_ex_ut()` calls it, and so do the sidereal houses in the
default sidereal frame (`sidereal_houses_trad()`); `swe_gauquelin_sector()`'s
methods 0 and 1 take the obliquity and the nutation with them before the
body's own `swe_calc()`. With SEFLG_JPLHOR and no corrections
`calc_nutation()` called `bessel()` on `ctx->dpsi`: NULL (SIGSEGV), or,
after a file with a day missing (`eop_dpsi_loaded` -2: the table
allocated, its end never set), at index n - 1 = -2451545. Measured with
Lahiri on de200 at J2000: each of the eight probes below crashes, with
SEFLG_JPLEPH | SEFLG_JPLHOR and with SEFLG_SWIEPH | SEFLG_JPLHOR, the
interpolation on or off, on the setter's thread, on a second thread on
the default context and on an explicit context given the path alone; and
on G19's synthetic DE431 header at 2000 Jan 7 in a directory without the
file (-1) and in one whose file has a day missing (-2). Both builds. The
other public functions that take flags were called the same way and
answer: `swe_calc()` and its `_ut` and `_pctr` forms, the four star
functions, `swe_nod_aps()` (which drops the Horizons bits), the eclipse,
occultation, rise and set, crossing and heliacal functions, `swe_pheno()`,
`swe_orbit_max_min_true_distance()`, `swe_get_orbital_elements()`,
`swe_deltat_ex()`, Gauquelin's method 2 (rise and set); `swe_sidtime()` and the
tropical houses take their nutation with no flags.

**Fix, as done here:** `swi_jplhor_plaus()` (`swephlib.c`) takes
SEFLG_JPLHOR as `plaus_iflag()` does -- SEFLG_JPLHOR_APPROX while
`eop_dpsi_loaded` <= 0 -- and the nutation `swe_get_ayanamsa_ex()` adds
and the obliquity and nutation of `swe_gauquelin_sector()`'s methods 0 and
1 take their flags through it, so each answers as the same call with
SEFLG_JPLHOR_APPROX, the frame `swe_calc()` gives the flag there. The
warning in serr is the one their own `plaus_iflag()` or `swe_calc()`
writes ("you did not call swe_set_jpl_file(); default to
SEFLG_JPLHOR_APPROX"). `calc_nutation()` takes its flags through it too,
where the table is read, so that no other route reaches the table
empty. In both builds, as entry 104's crash fix: nothing upstream
answered changes. One flag test in `calc_nutation()`; `perf stat`
instructions, `swe_calc()` of the ten bodies with speed at 5000 instants
on the Swiss files: 2902077905 before, 2902101848 after (+0.0008%), the
answers the same.

G134 `tests/eopfollow.c`: eight probes -- `swe_get_ayanamsa_ex()` and
`swe_get_ayanamsa_ex_ut()` with SEFLG_JPLEPH, `swe_get_ayanamsa_ex()` with
SEFLG_SWIEPH, `swe_houses_ex2()` sidereal Placidus with the speeds,
`swe_houses_ex()` sidereal Koch, `swe_gauquelin_sector()` methods 0 and 1
for Mars and 0 for Spica -- in six settings without corrections: de200 at
J2000, the same interpolated, a second thread making the call after the
main thread set de200, an explicit context given the path alone, and the
fixture header in the directory without the file and in the one with a
day missing, made in $TMPDIR. Each call in a forked process must return
with no signal, bit for bit with its return flag as the same call with
SEFLG_JPLHOR_APPROX in a fresh one. 48 checks; before the fix all 48
crash. The upstream build crashed in 46, the interpolated houses passing
there: their ayanamsa's nutation is interpolated from the nodes the
houses' own nutation of no flags builds just before (entry 134's unkeyed
nodes); after the fix it passes all 48. No G24 case: the fix is in both
builds.

## 143. A thread or context that adopts another thread's `swe_set_jpl_file()` never loads the IERS corrections, so its SEFLG_JPLHOR falls to SEFLG_JPLHOR_APPROX where the setter's does not (this fork's)

**Severity: Correctness (behaviour: a setting without effect on every context but the setter's; dpsi, the ayanamsa and every place of date under SEFLG_JPLHOR 4.2e-2" off the setter's on G134's fixtures)**
**Where:** `sweconfig.c`: `swi_config_apply()`; `sweph.c`: `load_dpsi_deps()`'s callers

Entry 134's noticed item. `load_dpsi_deps()` runs in `swe_set_jpl_file()`
(and, since entry 131, at the reread after `swe_close()`) on the context
that calls it, when the file opens with a DE number of 403 or more. A
context that takes the setting from the shared configuration
(`swi_config_apply()`, entry 119's route) closes its JPL file and adopts
the name and the DE number, and loaded no corrections: `eop_dpsi_loaded`
stayed 0, and `plaus_iflag()` turned its SEFLG_JPLHOR into
SEFLG_JPLHOR_APPROX with "you did not call swe_set_jpl_file(); default to
SEFLG_JPLHOR_APPROX". Three routes: a thread on the default context running
across the main thread's `swe_set_jpl_file()` (at its next sync), a thread
started after it (its first start's sync), and a context made after it
(`swe_ctx_new()`'s inheritance). Measured on G19's synthetic DE431 header
with G122's corrections A, Lahiri, at 2000 Jan 7.37 after the same call at
Jan 7: `SE_ECL_NUT` with SEFLG_JPLEPH | SEFLG_JPLHOR | SEFLG_SPEED
answered dpsi -13.488712" for the setter's -13.530688" (4.20e-2"), under
SEFLG_JPLHOR_APPROX's return flag (524545 for 393473, SEFLG_JPLHOR with
SEFLG_ICRS); `swe_get_ayanamsa_ex()` and the sidereal cusps of
`swe_houses_ex2()` 4.20e-2" (1.166e-5 deg) with it, on each route; with
the interpolation on, the thread across the setter the same, -13.489375"
for -13.531351". Both builds: the adoption is this fork's code (upstream's
threads share no setting).

**Fix, as done here:** `swi_config_apply()` calls `swi_dpsi_deps_adopt()`
(`sweph.c`) for an adopted path or JPL file: with the adopted DE number 403
or more and no corrections loaded, it loads them as the setter does, along
the adopted path, and drops what was computed without them -- the nutation
kept at an instant, the topocentric observer and the interpolation's nodes
built under SEFLG_JPLHOR (in the compat build, which keys neither the
nodes nor the nutation on the flags, all the nodes). In both builds, as the
adoption is this fork's (entries 119, 126, 129, 134). No hot path: it runs
only when a context adopts a path or a JPL file.

G134 `tests/eopfollow.c`, extended with 12 checks, each in a forked
process, bit for bit with its return flag against a fresh process whose
main thread set the path and the fixture and made the same calls:
`SE_ECL_NUT` with its speed, `swe_get_ayanamsa_ex()` and `swe_houses_ex2()`
sidereal Placidus with the speeds, all with SEFLG_JPLEPH | SEFLG_JPLHOR, at
2000 Jan 7 and 7.37, by a thread running across `swe_set_jpl_file()`
(started by the same call before it), the same interpolated, a thread
started after it and a context made after it. 60 checks in all; before the
fix the 12 fail (4.20e-2"), the compat build 11 of them (the interpolated
houses passing there on entry 134's unkeyed nodes); after it both builds
pass 60.

## 144. `swe_set_ephe_path()` keeps the IERS corrections read from the old directory, and so does its adoption: SEFLG_JPLHOR answers with them after another path and `swe_set_jpl_file()`, or after the path alone

**Severity: Correctness (behaviour: a setting without effect; dpsi, the ayanamsa and every place of date under SEFLG_JPLHOR 0.131" off the new directory's on G134's fixtures, 4.2e-2" off SEFLG_JPLHOR_APPROX's after the path alone)**
**Where:** `sweph.c`: `swe_set_ephe_path_r()`, `load_dpsi_deps()`'s load-once guard; `swi_dpsi_deps_adopt()` (entry 143) for the adoption

Entry 134's noticed item. `load_dpsi_deps()` opens with "if
(eop_dpsi_loaded > 0) return", and only `swe_close()` (`ctx_release()`)
frees the corrections and resets the flag. `swe_set_ephe_path()` closes
the files and zeroes the published JPL DE number, as the JPL file is found
along the path (2026-09-16 review, F4), and kept them: the next
`swe_set_jpl_file()` of a DE 403 or later loaded nothing, and without one
`plaus_iflag()` still saw them loaded. Measured on G19's synthetic DE431
header in two directories with G122's corrections A and B, Lahiri: path A,
the fixture and `SE_ECL_NUT` with SEFLG_JPLEPH | SEFLG_JPLHOR at 2000 Jan
7, then path B and the fixture, and dpsi at Jan 7.37 was A's -13.530688"
for B's -13.399987" (0.131"; interpolated -13.531351" for -13.400650");
after path B alone, A's under SEFLG_JPLHOR for SEFLG_JPLHOR_APPROX's
-13.488712" (4.20e-2"), the return SEFLG_JPLHOR's; `swe_get_ayanamsa_ex()`
the same, and an explicit context's `swe_set_ephe_path_r()` the same. And
since entry 143, a thread on the default context adopting the path did
too, the same numbers, across `swe_close()` with B and the fixture as
well. `swe_close()` on the setter's thread already passes: it frees them
and, since entry 134, the nodes built from them. Both builds; the
adoption's is this fork's code.

**Fix, as done here:** `swe_set_ephe_path_r()` drops the corrections where
it zeroes the DE number (`dpsi_deps_free()`): the tables freed, the flag
back to 0 so that the next `swe_set_jpl_file()` reads the new directory's,
and with them what was computed from them -- the nutation kept at an
instant, the topocentric observer and the interpolation's nodes built
under SEFLG_JPLHOR. The same path again (entry 103's early return) keeps
them: it is the same directory. Behind `#ifndef SWE_UPSTREAM_COMPAT`.
`swi_dpsi_deps_adopt()` drops them first for every adopted path or JPL
file and then loads the adopted setting's, in both builds, as the
adoption is this fork's (in the compat build the nodes all go, being
keyed on the instant alone). No hot path.

G134 `tests/eopfollow.c`, extended with 18 checks, each in a forked
process, bit for bit with its return flag against a fresh process that
set the final state (B and the fixture; for B alone, until entry 148, the
fixture and then B, which loads nothing): `SE_ECL_NUT` and `swe_get_ayanamsa_ex()` with
SEFLG_JPLHOR at 2000 Jan 7 and 7.37 after path B and the fixture, path B
alone, or `swe_close()`, B and the fixture, on the setter's thread (B and
the fixture interpolated too), an explicit context and a second thread
adopting each (interpolated too). 78 checks in all; before the fix 16 of
the 18 fail, all but `swe_close()` on the setter's thread; the compat
build fails the 8 of the setter's thread and the explicit context (16
before), and since entry 148's reference for B alone the 2 of the second
thread after it, its path setter reading no JPL file. G24 `eop_path_kept_off`: dpsi at Jan 7.37 after A, the fixture,
B and the fixture against a fresh context's: 0 shipped (1.31e-1" before
the fix), 1.31e-1" compat. With the three fixes (entries 142-144) G19,
G103, G116, G122, G127, G24, G2 (check-threads) and check-ctx pass; G25
(check-jplhor) skips, no DE >= 403 file here.

Noticed, not fixed:
- `swe_set_jpl_file()` to a file of DE below 403 keeps the corrections a
  DE 403 file loaded from the same directory: after path A (with
  de200.eph linked in), the fixture and de200.eph, `SE_ECL_NUT` with
  SEFLG_JPLEPH | SEFLG_JPLHOR at 2000 Jan 7.37 keeps SEFLG_JPLHOR with A's
  -13.530688" (return 393217) where a fresh process with A and de200.eph
  falls to SEFLG_JPLHOR_APPROX's -13.488712" (524289), 4.20e-2". A thread
  adopting it drops them, as a fresh process has none. Both builds.
  Fixed as entry 154.
- `load_dpsi_deps()` loads again over tables it left allocated: after a
  file it gave up on (`eop_dpsi_loaded` -2, a day missing; by reading, -3
  for eop_finals.txt) the next load, by `swe_set_jpl_file()` of another
  file and the fixture again, allocates both anew: 589824 bytes more
  mapped each time (two 292200-byte tables). It does not close the file
  when an allocation fails (by reading). Both builds. Fixed as entries 155
  and 156.

## 151. Woolard's nutation turns the Moon's mean anomaly at 10^4 times its rate: its five terms in it cycle every 3.97 minutes, dpsi up to 0.236" off the series

**Severity: Correctness (dpsi up to 0.236" and deps 0.0226" off Woolard's series under `SEMOD_NUT_WOOLARD`, which `SE_SIDBIT_PREC_ORIG` sets for the ayanamsas on Newcomb's precession; the interpolation on it 0.196" off)**
**Where:** `swephlib.c`: `calc_nutation_woolard()`

Entry 146's noticed item. `calc_nutation_woolard()` turns each mean
element through its revolutions a Julian century since 1900 Jan 0.5 and
keeps the fraction: the Sun's longitude 100.0021358, the Moon's
1336.855231, the Sun's anomaly 99.99736, the node 5.372617, and the Moon's
anomaly 13255523.59, 10^4 times its 1325.552359. The Explanatory
Supplement (1961) gives the Moon's mean anomaly from Brown's theory, the
elements Woolard (1953) used, as 296 deg 06' 16".59 + 1325r 198 deg 50'
56".79 T + 33".09 T^2 + 0".0518 T^3 (Meeus's 296.104608 + 477198.849108 T
+ 0.009192 T^2 in his Astronomical Formulae for Calculators): 1325.552359
revolutions, and the code's own 296.105 and .009192 T^2 are its other two
terms. Cited from memory with high confidence, and held three ways here:
the anomalistic month, 27.554550 days, is 1325.5524 revolutions a
century; IAU 1980's rate in this file, 1717915922.633"/cty at J2000
(1325r 198 deg 52' 02".633), is Brown's carried a century on by its T^2
and T^3 terms (198 deg 52' 03".13) to 0.5"/cty; and with 1325.552359 the
series is IAU 1980 less their coefficients' difference to 0.9 mas (below).
The rate is the decimal point moved four places.

So the terms in the anomaly l -- 0".0675 sin l, -0".0261 sin(2L' + l),
-0".0149 sin(2L - 2L' + l) and 0".0114 sin(2L' - l) in dpsi, 0".1199
together, and 0".0113 cos(2L' + l) in deps -- turned with a period of
36525 / 13255523.59 day, 3.97 minutes. Measured on Moshier over 20000
instants 1900-2100, against the same series with the right rate: dpsi
0.236" (twice 0.1199" at most) and deps 0.0226". Against IAU 1980, 0.323"
and 0.071", where with the right rate the two differ by 0.159" and
0.050": Woolard's coefficients (the rigid Earth of 1953) are not Wahr's
(the elastic Earth of IAU 1980), by 50.5 mas over 1900-2100 in the
18.6-year term (-17.2327 - 0.01737 T from 1900 against -17.1996 -
0.01742 T from 2000), 45.7 in the semiannual, 23.7 in the fortnightly,
16.8 in the annual, and IAU 1980's 93 terms this series' 13 leave out
88.0 mas together: 0.2444" in dpsi and 0.0864" in deps summed. Less that
difference itself, evaluated on IAU 1980's arguments, Woolard is within
0.90 mas of IAU 1980 in dpsi and 0.47 mas in deps (0.236" and 0.0228"
before), the two sets of arguments' difference; so the series' own
difference from IAU 1980 is tenths of an arcsecond, and what agrees to
the mas is the series. Over minutes, the second difference of dpsi over
2^-11 day was up to 0.133" where the series' curvature allows 1.6e-8".
With the interpolation on, dpsi between its nodes was 0.196" off the
series (4000 instants; IAU 1980's interpolation 2.5 mas). Both builds;
upstream's code.

`SE_SIDBIT_PREC_ORIG` sets this model for the ayanamsas on Newcomb's
precession (Fagan/Bradley, Raman, Krishnamurti, Lahiri 1940, Lahiri
ICRC), whose true ayanamsa takes its dpsi: all five move with it, by
+0.0806" at 1956 Mar 21 (below).

**Fix, as done here:** 1325.552359 revolutions a century, behind `#ifndef
SWE_UPSTREAM_COMPAT`, upstream's 13255523.59 under `#else`.

G138 `tests/woolard.c`, Moshier, 4000 instants 1900-2100 at multiples of
2^-10 day: (a) Woolard against IAU 1980 within 0.25" in dpsi and 0.09" in
deps, the coefficients' summed difference above with 1 mas for the
arguments: 0.160" and 0.049"; (b) the second difference of dpsi and deps
over 2^-11 day within 1e-6", the series' curvature giving at most 1.6e-8"
and 6.9e-9": 1.5e-8" and 6.7e-9"; (c) the interpolation, dpsi and deps at
t + 0.37 after a call at t, against the direct value within 3.5 mas,
G122's: 1.68 mas and 0.73 mas. The compat build fails 5 of 6: (a) dpsi
0.306" (deps 0.066" passes), (b) 0.133" and 0.0126", (c) 0.197" and
0.0192". G24 `woolard_md_cycle`: the largest second difference of
Woolard's dpsi over 2^-11 day at eight instants 1900-2100: 1.29e-8"
shipped, 9.70e-2" compat. Woolard is not the default: against the tip's
own G1 transcript one row moves, `cov:nutmodel[5]`, `swe_sidtime()` at
J2000 under Woolard, by 7.10e-3 s (dpsi 0.1161" there, times cos eps).

Noticed, not fixed:
- `SE_SIDM_LAHIRI_ICRC`'s t0 value, 23.25 - 0.00464207 at JD 2435553.5
  (1956 Mar 21), subtracts "nutation according to Woolard 1953" (sweph.h),
  and 0.00464207 deg = 16.7115" is the defective series' dpsi at that
  instant to its last digit; the series gives 16.7921" there. Mode 46 |
  `SE_SIDBIT_PREC_ORIG`'s true ayanamsa at t0 was 23 deg 15' 00".0000, the
  Calendar Reform Committee's value, and is 23 deg 15' 00".0806 after the
  fix. 23.25 - 0.00466447 would make it 23 deg 15' again, and move mode
  46's mean ayanamsa by -0.0806" at every date, with the bit or without.
  Fixed as entry 157.

## 152. The default nutation's grid leaves the IAU 2000A series far inside its guard of 1e12 days: 2.3e-9 deg at |J| 6e8, 1.3e-4 deg at 1e9, 7 deg at 3e9, answered by `swe_calc(SE_ECL_NUT)`

**Severity: Correctness (this fork's grid; the default nutation off the series it interpolates beyond |J| 5e8 days, 1.4 million years: 2.3e-9 deg at 6e8, 1.3e-4 at 1e9, 7 deg at 3e9 and up to 6.5e28 deg near 1e12, from `swe_calc(SE_ECL_NUT)` on Moshier with no error; its rate 2.1e-7 deg/day off at 6.3e8, 5.2e-4 at 1e9)**
**Where:** `swephlib.c`: `calc_nutation()`, `calc_nutation_grid()`, `swi_nutation_rate()` (this fork's; upstream's default is 2000B, computed directly)

Entry 146's noticed item. The default nutation since ts.17 (G29) is full
IAU 2000A interpolated: the series and its first three derivatives at
nodes a day apart, and the septic Hermite polynomial on the two nodes
around J. `calc_nutation()` and `swi_nutation_rate()` took it for any |J|
< 1e12, the guard G25's hostile walk needed (the nodes are indexed as
integers), and the direct series beyond.

Why it leaves the series: the fundamental arguments (Simon et al. 1994)
carry T^3 and T^4 terms, whose rates grow as T^3. At |J| = 1e9 days (T =
27300 centuries) the Moon's mean anomaly turns at 2.4 rad/day, ten times
its 0.228 now, and the node at 0.64 rad/day for 9.2e-4: the 18.6-year
term has a 9.8-day period there, and with its -0.0174"/cty some 490" of
amplitude. The polynomial's error goes as (omega h)^8, h one day, so as
T^24: it reaches the floor at about 6e8 days and is degrees by 2e9.
Below that, what the grid and the series differ by is not the
polynomial: at the nodes they agree to 1e-17 deg, and between them both
are as far from the series evaluated in long double, which is the
arguments' rounding at that size (the luni-solar arguments are some
1.5e13" at 3.2e8 days, their last bit 2e-3"): the grid and the series
9.4e-12 and 9.3e-12 deg off it at 1e8, 9.3e-11 and 9.7e-11 at 3.2e8,
1.1e-9 and 1.75e-9 at 5e8; then 5.0e-8 and 3.8e-9 at 6.3e8, 1.3e-4 and
3.0e-8 at 1e9.

Measured, Moshier, the default against `SEMOD_NUT_IAU_2000A` set
explicitly (the direct series), worst over instants in each band of |J|,
either sign (4000 a band of 0.05 decade, 400 of 0.25), dpsi and deps:
- 1e8 to 1.1e8: 1.1e-11 and 1.1e-11 deg; 3.2e8 to 3.5e8: 1.2e-10 and
  7.3e-11; 4.0e8 to 4.5e8: 3.1e-10 and 1.1e-10;
- 5.0e8 to 5.6e8: 6.7e-10 and 2.4e-10; 5.6e8 to 6.3e8: 2.3e-9 and 1.1e-9;
  7.1e8 to 7.9e8: 1.4e-8 and 5.5e-8; 8.9e8 to 1e9: 1.3e-6 and 5.2e-6;
  1e9 to 1.1e9: 1.4e-5 and 3.5e-5;
- 1.8e9 to 3.2e9: 7.1 and 1.2 deg; 1e10 to 1.8e10: 2.4e8 deg; 5.6e11 to
  1e12: 5.5e25 deg (G29's instants below: 6.5e28).
The rate, `swi_nutation_rate()`'s derivative of the polynomial against
the series' own derivative at the instant: 6.1e-10 deg/day at 4e8 to
5e8, 4.0e-9 at 5e8 to 6.3e8, 2.1e-7 at 6.3e8 to 7.9e8, 5.2e-4 at 1e9.
Nothing but the nutation reaches such dates: Moshier's planets and Moon
and the mean node and apogee refuse them; `SE_ECL_NUT`, `swe_sidtime()`
and the houses (whose speeds difference their positions) take the
nutation, and the rate reaches only speeds of date.

**Fix, as done here:** the grid within |J| < `SWI_NUT_GRID_JMAX`, 4e8 days
(1.1 million years), and the direct series beyond, in `calc_nutation()`;
in `swi_nutation_rate()`, where the polynomial's derivative was taken,
beyond the same bound the series' own derivative at the instant
(`calc_nutation_2000a_derivs()`, the grid's node function, at tjd), not
the central difference other models take, which is the series' rounding
over 2e-4 day there (1.2e-6 deg/day at 4e8). Within 4e8 days the grid is
within the series' rounding, 2.1e-10 deg in dpsi and 1.2e-10 in deps
(8000 instants a band of 0.05 decade), and that is the step where the
two meet; the rate's is 3.5e-10 deg/day. The grid is this fork's, so the
fix is in place: the compat build has no grid and nothing to restore.
- After: the default is the direct series bit for bit beyond 4e8 days.
  The rate is within 2.2e-10 deg/day of a five-point difference over
  2^-6 day of the series evaluated in long double up to 4e8, 1.1e-10 at
  4e8 to 6.3e8 and 2.6e-9 at 6.3e8 to 1e9 (beyond, the reference's own
  truncation), for 2.3e-9 and 1.6e-5 before.
- In -13000..+17000 nothing changes: G29's 1800-2100 and -13000..15000
  numbers are the tip's (0.0011 and 0.0006 uas, 0.0014 and 0.0005), G1's
  transcript is too, and `perf stat` instructions for `SE_ECL_NUT` at
  20000 instants over -13000..+17000 with the ten Moshier bodies with
  speed at 20000 instants 1800-2100: 52629580884 before, 52621865158
  after (-0.015%).

G29 `tests/nutgrid.c`, extended: 4000 instants with |J| log-uniform over
1e7..1e13, either sign, the default against the direct series within 1e-9
deg (3.6 uas), the residual floor: 0.57 uas in dpsi and 0.21 in deps;
before the fix 2.4e35 uas, near 1e12. The compat build, whose default is
2000B, fails it as it does G29's other spans (1.4e10 uas). No G24 case:
the compat build has nothing to restore. G32, G57, G50 and G122 pass.

Noticed, not fixed:
- RESIDUAL. `swi_nutation_rate()`'s central difference over
  `NUT_SPEED_INTV` (1e-4 day), the rate of every model but the default's
  grid, is the series' rounding over 2e-4 day far from J2000: the
  2000A series set explicitly, against its own derivative, 1.0e-9 deg/day
  at |J| 1e6, 3.4e-9 at 7.9e6 (the end of -13000..+17000), 1.1e-8 at
  2.5e7 (68000 years), 5.4e-8 at 1e8, 1.2e-6 at 4e8. Above the 1e-8
  deg/day floor only beyond 68000 years, where no body has a speed.

## 147. A thread adopting another thread's `swe_set_ephe_path()` keeps the old directory's delta t table, leap seconds and planet-name memo, and the delta-t memo outlives a path change on every thread (this fork's)

**Severity: Correctness (behaviour: delta t 23.13 s off at 1990 after a path change, on the setting thread at an instant already asked and on every adopting thread; UTC to TT 1 s off in 2026, and a fictitious body's name, on the adopting thread)**
**Where:** `sweconfig.c`: `swi_config_apply()`, its branch for an adopted path; `swephlib.c`: `calc_deltat()`'s memo, new `swi_dt_table_forget()`; `sweph.c`: `swi_close_keep_topo_etc()`, `ctx_release()`

Entry 137's noticed item, found by its work. A path change on the setting
thread (`swe_set_ephe_path()`, `swi_close_keep_topo_etc()`) sends what it
read along the old path back to be read along the new one: the leap
seconds (`seleapsec.txt`, `leapsec_done`), the delta t table
(`swe_deltat.txt`, `init_dt_done`), the star file and catalogue
(`sefstars.txt`, bug 92's `swi_fixstar_forget()`, bug 121's failed load),
`seorbel.txt`'s lines, the planet-name memo (`saved_planet_name`, which
holds a name read from `seorbel.txt`, an asteroid file or `seasnam.txt`),
and the IERS files (bugs 142-144's). A thread on the default context
adopting the change in `swi_config_apply()` emptied the star and
`seorbel.txt` caches and kept the rest. Measured with a first directory of
links to EPHEDIR's files with its own `swe_deltat.txt` (80.0 s for 1990),
`seleapsec.txt` (a leap second at 2025 Jul 1), `sefstars.txt` (Sirius and
Aldebaran 10 s of RA on) and `seorbel.txt` (Cupido 1 deg on, named
Cupido147), the path then set to EPHEDIR by another thread, against a
fresh process with EPHEDIR:

- the adopting thread's delta t at 1990 Jan 1 79.987645 s for 56.856191,
  by `swe_deltat()` and by `swe_deltat_ex()` with SEFLG_SWIEPH;
- its `swe_utc_to_jd()` for 2026 Jan 1 0h UTC TT 2461041.50081231 for
  2461041.50080074, the old file's leap second (1.0 s);
- its `swe_get_planet_name(SE_CUPIDO)` "Cupido147" for "Cupido", the memo
  of the last name asked; Cupido's place was right (the lines were
  emptied), and so were both star functions (bug 92);
- under SWE_UPSTREAM_COMPAT, where the star caches are upstream's and
  stay, its `swe_fixstar()` went on reading the old `sefstars.txt` through
  the file left open: Sirius 104.136831 deg for the setting thread's
  104.085304.

And the delta t on the setting thread itself, and on an explicit context
given the same calls: at an instant asked before the path change, under
the same tidal term (`swe_deltat_ex()` with SEFLG_SWIEPH, both paths
holding semo_18.se1), 79.987645 s for 56.856191. `calc_deltat()`'s memo
(this fork's, G18) is consulted before `init_dt()`, which empties it when
it reads the table again; clearing `init_dt_done` did not reach it. With
de200.eph set, `swe_deltat()` dodged it only because the path setter
changed its guess (F4) and so the memo's key; `swe_close()` the same: a
`swe_deltat.txt` rewritten to 70.0 s and `swe_close()` left 79.987645 s
(the manual's way to have a rewritten file read again, bug 103). A
thread started and a context made after the path change answered right;
upstream has no memo, and its threads share no configuration.

**Fix, as done here:** in both builds (the adoption and the memo are this
fork's). The adoption's branch for a new path drops what the setter
drops: `leapsec_done`, the delta t table, the planet-name memo, and the
star file and `is_old_starfile` (in the shipped build
`swi_fixstar_forget()` did already). The delta t table goes with its memo
everywhere it is sent back to be read (`swi_dt_table_forget()`: the path
setter, the JPL setter, `swe_close()` and the adoption). Nothing is opened
by any of it; each file is read again when first asked for, as on the
setting thread. Not on a hot path: a path change.

`seasnam.txt` is read at each lookup that needs it, through the same name
memo; not measured on its own (no asteroid file with a provisional
designation in ephe/). The IERS files are entries 142-144's.

G132 (`tests/jpldenum.c`, entry 137) gains the setting (the first path a
directory with its own four tables, the last setter the path to EPHEDIR),
the observation (delta t at 1990 Jan 1 by `swe_deltat_ex()` with
SEFLG_SWIEPH and by `swe_deltat()`, the TT of 2026 Jan 1 UTC, Sirius by
`swe_fixstar2()` and `swe_fixstar()`, Aldebaran by `swe_fixstar()`, Cupido
and its name, read first by the readers that compute before the last
setter, delta t at 1990 last), and a reader (a context made by
`swe_ctx_new()` after the last setter): 5 settings x 5 readers x 4
observations, and a check that the first directory's tables move every
value: 101 pass. Before the fix 3 fail: the setting thread and the
explicit context (delta t, the memo) and the running thread (delta t, the
TT, the name). The compat build fails 52 of 101 with the fix and
without, by upstream behaviour it restores, of which these 3 by
`swe_fixstar2()`'s catalogue, which upstream's path setter keeps (bug
92), the running thread now answering as the setting thread. No G24 case:
the fix is in both builds. G18 (`check-dtmemo`) and G116 pass.

## 148. After the path setter forgets the JPL DE number of the file whose name it keeps, the first JPLEPH delta t takes the default tidal term until a calculation opens the file: the same call answers differently first and second

**Severity: Correctness (behaviour: call order -- the first UT JPL Moon in AD 1700 1.79e-3 deg off the second, the first JPLEPH delta t 11.27 s off, and `swe_deltat()`'s guess Swiss for a configuration naming de200.eph -- on every thread and context)**
**Where:** upstream `sweph.c` `swe_set_ephe_path()` (its `swi_close_keep_topo_etc()` closes the JPL file and keeps `jplfnam`; `calc_deltat()` takes the open file's DE number); here `sweph.c`: `swe_set_ephe_path_r()` (F4's `jpldenum_cfg = 0`), `swi_denum_reread()`

Entry 138's noticed item; the coordinator's decision. `swe_set_ephe_path()`
closes the JPL file and keeps its name, and since the 2026-09-16 review's
F4 forgets its published DE number (`jpldenum_cfg`), so that a JPL delta
t after a path to a directory without the file takes the default term, as
upstream's does. But the name is kept, and a JPLEPH calculation opens the
file by it along the new path: after `swe_set_ephe_path()`,
`swe_set_jpl_file("de200.eph")` and the path again by another string
(EPHEDIR/.), `swe_calc_ut()`'s JPL Moon in AD 1700 was 40.0057507520 deg
on the first call, whose delta t took the default term (14.286718 s for
de200's 3.012003), and 40.0039614086 on the second, the file open; a TT
JPL Sun then left the getter and `swe_deltat()` on the Swiss files' term
(entry 138: -25.936, 18.902020 s in 1800); and a fresh process given the
path and de200.eph answers 40.0039614086 both times, 3.012003 s and the
JPL guess (-23.8946, 14.438958 s). So on the setting thread, a thread
adopting the path (the master's number zeroed with it), a thread started
and a context made after it, and an explicit context given the same calls.
The compat build the same: 40.0057510966, then 40.0039617599. Upstream's,
its JPL delta t following the open file alone.

The decision. Every gate here takes "the answer equals a fresh process
with the same final settings". After "path, file, path" the configuration
is the last path and the file, whose name the setter keeps: the same as
"path, file". F4's case, a new path without the file, is a fresh process
whose `swe_set_jpl_file()` fails there: no DE number, the default term.

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`; the compat
build keeps F4's zero and upstream's first and second answers): when a
JPL file had been set and read (`jpldenum_cfg`), or is owed since
`swe_close()` (`denum_unread`, bug 131), `swe_set_ephe_path()` reads it
again along the new path once the moon file's header is read, as a fresh
process's `swe_set_jpl_file()` reads it there and as bug 131 reads it
after the close (`swi_denum_reread()`: the file opened, the IERS
corrections of a DE 403 or later), and publishes its DE number with the
path; where it is not found the number stays 0. A thread adopting the
path takes that number with it, as it takes the moon file's; a context
made after inherits it. The same path again keeps the number, as before
(bug 103). Entry 138's G132 expectation after the path again follows (its
entry says how).

The open. Reading the number opens the JPL file in the setter, and the
file stays open for the next JPL calculation, which opens nothing then.
Counted with `fopen()` interposed over 100 rounds after a warm-up, before
and after: G102's seven rounds 0, 0, 0, 0, 0, 0 and 396 both; G115's and
G106's counted cases pass; with de200.eph set, a path alternating
between two directories and a JPL Moon each round 396 both, the same path
again and a JPL Moon 0 both, the path and then `swe_set_jpl_file()` of
the same file each round 399 both (the setter finds it open); with a
Swiss Moon each round 396 before and 495 after, one open of de200.eph per
real path change, as a fresh process given those settings opens it. Made
lazy as bug 131's (read at the next entry that is not a setter), it would
open the same file at that Swiss Moon's entry, so it would keep no count; the setter's own read
is bug 131's "the setters' own reads", and an adopting thread reads
nothing. The one sequence it costs more: a path change followed at once
by `swe_set_jpl_file()` of another file, one open. No hot path.

G134 (`tests/eopfollow.c`, entry 144): after path A and the DE431 fixture,
path B alone now reads the kept fixture along B and with it B's IERS
corrections, so its reference is a fresh process given path B and the
fixture, as for B and the fixture; it was the fixture and then B, which
loads nothing (A's corrections had been the defect, none the answer
expected). SEFLG_JPLHOR answers B's dpsi -13.399987", return 393473, on
the setter's thread and a thread adopting the path; the 4 checks failed
against the old reference.

G132 (`tests/jpldenum.c`, entry 137): the reference is now a fresh process
given the final settings (the last path and the JPL file's name), not a
replay of the calls, and the observation gains a UT JPL Moon in AD 1700
twice, its return and longitude, which must agree, then the getter and
`swe_deltat()`: 5 settings x 5 readers x 5 observations and one check, 126
pass. Before the fix 30 fail: after the path again and after the tables'
path (entry 147), on all 5 readers alike, the JPLEPH delta t 14.286718 s
for 3.012003, the getter after the JPL Sun -25.936 for -23.8946 and
`swe_deltat()` in 1800 18.902020 s for 14.438958, the first Moon
40.0057507520 deg for the second 40.0039614086. The compat build fails 55
of 126 with the fix and without, by upstream behaviour it restores, 18 of
them this one's (the first delta t and the first Moon on the setting
thread, a new thread, the explicit context and a context made after,
after the path again and the tables' path, and the running thread's
guess). G115 (`tests/pathset.c`) P4, the path, de200.eph and a path
without semo_18.se1, now takes that path without de200.eph too, where
there is nothing to read again, so that it still sees bug 118's term
resolved for the state replaced (-23.8946 for -25.80), and its fresh
process is given the path and de200.eph. G24 `jpl_path_moon_off`, the
first UT JPL Moon in AD 1700 after the path, de200.eph and the path again
against the second: 0 shipped, 1.79e-3 deg compat (and shipped before
the fix). G24 `jpl_open_tid_off` (entry 138) moved to the default file
name, as its entry says.

Noticed, not fixed: a JPL file whose `swe_set_jpl_file()` failed along
the path then set, and a path where it is: no number was ever read, so
the path setter reads none (the condition is a number set and read, as
bug 131's), and the first UT JPL Moon in AD 1700 is again 40.0057507520
deg and the second 40.0039614086, `swe_deltat()` there 15.091463 s, where
a fresh process given the second path and de200.eph answers 40.0039614086
twice and 3.012003 s; the configuration does not record that a name was
set (only the name, which a calculation's fallback to de406.eph rewrites
too). And with no JPL file set at all, a JPLEPH calculation opens the
default file by its name and the first delta t before it takes the
default term: in a fresh process given a path whose de431.eph is
de200.eph, the same two Moons; with the real files only where de431.eph
is missing and the fallback finds de406.eph, whose term (-25.826) is not
the default's (upstream's, not measured here).

## 149. `swe_nod_aps()` with barycentric output on Moshier, asked for or fallen to, answers Moshier's heliocentric points

**Severity: Correctness (behaviour: a wrong answer under the centre asked for, returned OK; Mars's osculating node 0.077 deg off the files' barycentric one, the Moon's mean node 0.37 deg, the Sun's mean perigee 0.40 deg)**
**Where:** `swecl.c`: `swe_nod_aps_r_body()`, and through it `swe_nod_aps_ut()`; `sweph.c`, `sweph.h`: `swi_refuse_mosh_bary()`, shared

Found by entry 139's work. `swecalc()` refuses every barycentric Moshier
request ("barycentric Moshier positions are not supported."); Moshier
serves heliocentric places only, the Sun its origin. `swe_nod_aps()` builds
its points heliocentric (the Moon's geocentric) and moves them to the
centre asked at the end: for the barycentric output it adds the
barycentric Sun (to the Moon's points the barycentric Earth) unless the
request's flag has SEFLG_MOSEPH, and subtracts nothing. Nothing in it
refused SEFLG_MOSEPH with SEFLG_BARYCTR, so on Moshier the barycentric
points were the heliocentric ones, returned OK, which says nothing of the
ephemeris. At 2000 Jul 1.5:

| point, barycentric | Moshier | the files' | off |
|---|---|---|---|
| Mars, osculating ascending node | 49.5626 deg, its heliocentric node to the last digit | 49.6398, latitude 7.2e-3 | 0.077 deg |
| Mars, mean node | 49.5577 | 49.6350 | 0.077 |
| the Moon, mean node | 279.9123 | 279.5396 | 0.37 |
| the Sun, mean perigee | 102.9417 | 103.3367 | 0.40 |

Every body class alike: Mercury to Neptune mean and osculating, Pluto, the
Sun's and the Earth's apsides, an asteroid (Chiron, its file present),
SE_NODBIT_FOPOINT's focal point and SEFLG_TOPOCTR | SEFLG_BARYCTR, which
this function answers barycentric. Only SE_NODBIT_OSCU_BAR beyond 6 AU
(Saturn on), whose ellipse is read barycentric through `swe_calc()`, was
refused, by that call. With the fallback allowed, a Swiss or JPL request
whose files are missing fell to Moshier inside its calls and answered the
same: in a fresh process Moshier's heliocentric points bit for bit; warm,
those plus the Swiss Sun an earlier call left in the save area (the files
of 1800-2400 alone, Mars's node at 1500 after a call at 2000: 45.8451 deg
for Moshier's heliocentric 45.6908); Chiron with its file and no planet
file, 209.3985 deg, the heliocentric node. Strict, those calls refuse the
substitution: unchanged. Both builds answered so.

**The right answer.** A refusal, as `swe_calc()` refuses: the barycentric
place of a point is its heliocentric one plus the barycentric Sun (the
Moon's: the geocentric one plus the barycentric Earth), and Moshier serves
neither (entry 139, which also declined to make `swi_mosh_sun_bary()` a
public barycentric ephemeris). That holds for the mean points too: their
elements read no file, but their change of centre does, so a barycentric
mean node on Moshier is the heliocentric one under the barycentric request;
`swe_calc()` refuses SE_MEAN_NODE with SEFLG_MOSEPH | SEFLG_BARYCTR as well.
A Moon from Moshier on the files' Earth (the Moon files alone missing) is a
barycentric place, as entry 139 keeps `swe_calc()`'s barycentric Moon.

**Fix, as done here:**
- The barycentric output read as this function reads the flags:
  SEFLG_BARYCTR without SEFLG_HELCTR, SEFLG_TOPOCTR or not. The ephemeris
  by the bits' precedence, as `plaus_iflag()` reads it (SEFLG_SWIEPH |
  SEFLG_MOSEPH is a Swiss request).
- Moshier asked: refused before anything is computed, -1 and `swecalc()`'s
  message, the four points zeroed as `swe_calc()` zeroes a refused place
  (and this function an unimplemented body's).
- A Swiss or JPL request: the return flag of the call that sets the save
  area's Sun and Earth (the body heliocentric; for the Moon the Sun
  geocentric) goes through entry 139's `swi_refuse_mosh_bary()`, now shared
  with swecl.c: fallen to Moshier, refused with `swecalc()`'s message;
  strict, the calls have refused it already with the downgrade's.
- `swe_nod_aps_ut()` inherits it. Heliocentric and geocentric output,
  SEFLG_HELCTR | SEFLG_BARYCTR (heliocentric here) and the complete
  directory answer as before; the Moon files alone missing, the Moon's
  barycentric points answer, 6.4e-9 deg off the complete directory's on
  the mean node, 2.1e-5 deg on the osculating points (the ellipse of
  Moshier's Moon), the others within 1e-12 deg.
- Behind `#ifndef SWE_UPSTREAM_COMPAT`: upstream answers so.
- Cost: instructions +0.011% on calc-swieph, +0.006% on calc-moseph
  (`swi_refuse_mosh_bary()` no longer static; `swe_nod_aps()` is not in
  the bench).

Noticed, not fixed (not this entry's):
- **Moshier's geocentric Sun apsides are seen from the Earth.** For
  SE_SUN without SEFLG_HELCTR or SEFLG_BARYCTR the observer is the Sun
  unless the flag has SEFLG_MOSEPH, then the Earth: Moshier's mean perigee
  at 2000 Jul 1.5 281.4259 deg at 1.9993 AU, its apogee 191.4583 deg at
  0.0531 AU, where the files' are 282.9417 deg at 0.9833 AU and 102.9417
  deg at 1.0167 AU (1.5 and 88.5 deg off). Both builds.
- **The fallback's change of centre reads the caller's flag.** A Swiss or
  JPL request fallen to Moshier adds and subtracts the save area's Sun as
  the files' answer does: fresh it is zero and the answer Moshier's bit for
  bit, warm it is an earlier call's Swiss Sun. The files of 1800-2400
  alone, Mars's geocentric osculating node at 1500 Jan 1 after a call at
  2000: 4.3907 deg, latitude 0.0101, for Moshier's 4.4536 and 1.4e-4
  (0.063 deg); heliocentric 3e-6 deg. (The stale Sun is whichever the
  call before left: entry 168 measured 4.3236 deg after Mars's points at
  2000 Jul 1.5.) Fixed as entry 168. The same raw-bit test makes
  SEFLG_SWIEPH | SEFLG_MOSEPH, answered from the files, take Moshier's
  centre: Mars's geocentric node 69.7303 deg for 69.8382, its barycentric
  one the heliocentric.
- **The flag precedence is not `swe_calc()`'s.** SEFLG_HELCTR |
  SEFLG_BARYCTR is answered heliocentric (`swe_calc()`: barycentric; Mars's
  node 49.5625 deg for 49.6398), SEFLG_TOPOCTR with SEFLG_BARYCTR
  barycentric less the site's offset (49.6387, 1.2e-3 deg from the
  barycentric node) and with SEFLG_HELCTR heliocentric, where `swe_calc()`
  answers topocentric. With the files too.
- **`swe_nod_aps_ut()`'s delta t is the asked ephemeris's.** Fallen to
  Moshier it keeps the files' (1900 Jan 0.75: -1.9915 s, Moshier's
  -2.0521 s): Mars's points 2.9e-7 deg off the Moshier request's.
  `swe_calc_ut()` and `swe_pheno_ut()` redo it with the ephemeris that
  answered; this function returns no flag to say which.
- **The Moon files alone missing, the Swiss tidal term is the default.**
  -25.80 for the planet file's DE431 -25.936: delta t at 1900 -1.9915 s
  for -1.9541; the Moon's mean node through `swe_nod_aps_ut()` 5.0e-8 deg
  off the complete directory's.

G133 `tests/fbbary.c` (entry 139's gate) now also checks, each case in a
forked process at its two instants, Swiss and JPL:
- (g) the Sun, the Moon, Mercury, Mars, Saturn, Pluto and the Earth,
  mean, osculating, SE_NODBIT_OSCU_BAR, SE_NODBIT_FOPOINT and method 0,
  SEFLG_TOPOCTR | SEFLG_BARYCTR, `swe_nod_aps_ut()`, and Chiron in a
  directory with the asteroid files alone (Moshier asked in the complete
  directory too), barycentric: the Moshier request and the fallback
  refused with `swecalc()`'s message, strict the downgrade's, the complete
  directory answering;
- (h) the same calls heliocentric, geocentric and SEFLG_HELCTR |
  SEFLG_BARYCTR: the fallback the Moshier request's answer bit for bit
  (not the UT calls, whose delta t differs, nor the Sun's geocentric
  points, above), SEFLG_HELCTR | SEFLG_BARYCTR the heliocentric answer;
- (e) the Moon files alone missing: those barycentric points answered, the
  Moon's within 1e-7 deg (mean) and 1e-4 deg (osculating), the others'
  within 1e-12 deg;
- (f) warm: Mars barycentric refused twice with heliocentric calls around
  it answering alike, and with the files of 1800-2400 alone answered at
  the instant, refused at 1500 and the first answer again bit for bit.

1002 checks pass. Before the fix 162 fail: 152 of (g), every Moshier and
fallback call but the Saturn ellipse's, and 10 of (f). The compat build
fails 262: those 162, entry 139's 72, the 8 Moshier requests of
`swe_calc_orbel()` (entry 135), the Saturn ellipse fallen to Moshier and
answered in (g) (4) and (h) (12, entry 139), the Moon files' Moon twice
(entry 120), and the Sun's and the Earth's osculating apsides with the
Moon files missing, 3.9e-10 deg off through upstream's Earth-Moon mass
ratio (entry 108). G99, G117, G47, G92, G104, G76, G126, G2, check-ctx and
G24 pass.

G24 `nodaps_mosh_bary_ret`: `swe_nod_aps()` of Mars, Moshier,
barycentric, osculating, at 2000 Jul 1.5: -1 shipped, 0 compat.

## 150. `swe_calc_pctr()` returns the caller's flags, not the ephemeris that answered

**Severity: Correctness (behaviour: a return flag naming an ephemeris that did not answer; with the fallback allowed, SEFLG_JPLEPH on the Swiss answer bit for bit, SEFLG_SWIEPH on a Moon from Moshier)**
**Where:** `sweph.c`: `swe_calc_pctr_r_body()`

Found by entry 139's work. `swe_calc_pctr()` reads the centre and the body
with `swe_calc()`, barycentric, and returned its own flags as
`plaus_iflag()` left them, whatever those calls had answered with.
`swephexp.h`: "The ephemeris actually used is reported in the return flag
either way"; the manual has `swe_calc()` say so in its return value. With
the fallback allowed, at 2000 Jul 1.5:
- the JPL file alone missing, JPL asked: Mars from Jupiter returned 257
  (SEFLG_JPLEPH | SEFLG_SPEED), with SEFLG_TRUEPOS 1553, with SEFLG_BARYCTR
  1793, each the Swiss request's answer bit for bit; the Moon from Mars
  and Mars from the Moon alike;
- the Moon files alone missing, Swiss: the Moon from Mars 258
  (SEFLG_SWIEPH) with Moshier's Moon on the files' Earth, 8.2e-8 deg in
  longitude off the complete directory's (2.1e-7 deg in longitude and
  latitude at 1900), where `swe_calc()` of that Moon says SEFLG_MOSEPH
  (entry 120); Mars from the Moon alike.
Strict, every one is refused for its files, as before. A barycentric read
cannot fall to Moshier at all since entry 139, so no other substitution
reaches it. Both builds returned so.

**The right answer.** The ephemeris the reads answered with. The centre
and the body can differ: the Moon from Mars above, the Moon from Moshier
and Mars from the files. Not refused: the caller allowed the substitution
and each read is an answer `swe_calc()` gives. The lower one, Moshier
below Swiss below JPL: the answer is the body less the centre, each whole,
so it is no better than the worse of them. Entry 120 keeps SEFLG_SWIEPH on
a planet whose Earth has a Moshier Moon because only 1/82 of that Moon's
error reaches it (3.4e-8 deg); here the Moon enters whole, as in its own
flag.

**Fix, as done here:** the centre's and the body's reads at tjd and, with
light time, at the emission instant fold their returned ephemeris, the
less precise kept (`pctr_ephe_lower()`), and the return carries it in
place of the caller's. The Sun read for the deflection and the centre's
read for its acceleration are of the same files and not counted (entry
120's rule: a small part does not relabel the answer). Under the strict
setting nothing changes: no read substitutes. Behind `#ifndef
SWE_UPSTREAM_COMPAT`: upstream returns the caller's flags. No cost on the
bench (`swe_calc_pctr()` is not in it).

The other functions that return flags after `swe_calc()`, checked with
the JPL file alone missing (JPL asked) and the Moon files alone missing
(Swiss), the fallback allowed:
- `swe_pheno()`, `swe_pheno_ut()`: the first call's ephemeris, which the
  later calls then use: SEFLG_SWIEPH for the JPL request, SEFLG_MOSEPH for
  the Moon without its files. Right.
- `swe_fixstar()`, `swe_fixstar2()` and their UT forms: the Earth's
  ephemeris (entry 67's `ephe_used`): SEFLG_SWIEPH. Right.
- `swe_get_ayanamsa_ex()` (and `_ut`): the star modes the star's
  ephemeris (SEFLG_SWIEPH); the precession modes read no ephemeris and
  return the one asked, as `swe_calc()` does for the mean node. Right.
- `swe_nod_aps()` (and `_ut`), `swe_get_orbital_elements()`,
  `swe_orbit_max_min_true_distance()`: OK or ERR, no ephemeris bits, as the
  manual has them. Nothing claimed.
- the eclipse and occultation functions: their retflag is the eclipse
  type (SE_ECL_*), never ephemeris bits. Nothing claimed.
- `swe_calc_orbel()` (this fork's): not checked here (entry 141's).

Noticed, not fixed (not this entry's): the light-time read of the centre
overwrites `retc` before it is tested, so its failure is not seen (only
the body's read after it is): the aberration rate then takes a zeroed
centre. Not reached in any measured case; both reads use the same files.
Fixed as entry 158, which reaches it with a centre whose files begin
later than the body's.

G133 `tests/fbbary.c` (i), at its two instants: the complete directory's
the ephemeris asked; JPL asked, the JPL file alone missing, the fallback
allowed: Mars from Jupiter (SEFLG_SPEED, SEFLG_TRUEPOS, SEFLG_BARYCTR), the
Moon from Mars and Mars from the Moon SEFLG_SWIEPH, the Swiss request's
return and answer bit for bit, strict the downgrade's refusal; Swiss, the
Moon files alone missing, the fallback allowed: the two Moon cases
SEFLG_MOSEPH within 1e-6 deg (2.1e-7) of the complete directory, Mars
from Jupiter its answer bit for bit, strict entry 120's refusal of the two
and the complete directory's answer for the rest. 1072 checks pass.
Before the fix 24 of (i) fail (20 JPL, 4 Moon). The compat build fails
296: entry 149's 262 (G133 there), these 24, entry 120's 4 strict answers
of the Moon cases, and 6 of Mars from Jupiter at 1900 with the Moon files
missing, 2.1e-11 deg off (the compat build keys its obliquity to the
SE_ECL_NUT call at tjd + delta t, whose tidal term the missing Moon file
changes; check-compat's "swe_calc_pctr drift"). G99, G117, G76, G47, G92,
G104, G126, G2, check-ctx and G24 pass.

G24 `pctr_ephe_ret`: Mars from Jupiter at 2000 Jul 1.5, JPL asked,
de999.eph named and missing, the fallback allowed: 258 shipped, 257
compat.

## 157. `SE_SIDM_LAHIRI_ICRC`'s epoch value subtracts the broken Woolard series' dpsi: with entry 151 fixed its true ayanamsa on 1956 Mar 21 is 23 deg 15' 0.0806", not the committee's 23 deg 15'

**Severity: Correctness (mode 46's ayanamsa 0.0806" large at every date, with `SE_SIDBIT_PREC_ORIG` or without)**
**Where:** `sweph.h`: `ayanamsa[]`, mode 46

Entry 151's noticed item. Mode 46 is the Indian Calendar Reform
Committee's ayanamsa, defined as a true ayanamsa of 23 deg 15' 00" on
1956 Mar 21 (JD 2435553.5) with Newcomb's precession and Woolard's (1953)
nutation. The table holds the mean ayanamsa, so its t0 value is 23.25 deg
less the nutation in longitude at the epoch; sweph.h's comment says so
("nutation according to Woolard 1953"). The value subtracted, 0.00464207
deg = 16.7115", is what `calc_nutation_woolard()` gave there with the
Moon's mean anomaly turning 10^4 times too fast, to its last digit; the
series with the right rate gives 16.792088" (0.0046644689480 deg). Before
entry 151 the two errors cancelled: mode 46 | `SE_SIDBIT_PREC_ORIG`'s true
ayanamsa at t0 was 23 deg 15' 00".0000. After it, 23 deg 15' 0.0806".

Fix (behind `#ifndef SWE_UPSTREAM_COMPAT`; the compat build keeps both the
series and the constant): t0's value is 23.25 - 0.0046644689480, Woolard's
dpsi at the epoch as `calc_nutation_woolard()` now computes it. The true
ayanamsa at t0 is 23.25 again to 4.3e-14 deg, and every date's mean
ayanamsa of mode 46 is 0.0806" (2.24e-5 deg) smaller, with the bit or
without. The definition, not the old number, is the reference: the old
number held only because the series was wrong.

G138 `tests/woolard.c` (d): a context with `SE_SIDM_LAHIRI_ICRC |
SE_SIDBIT_PREC_ORIG`, `swe_get_ayanamsa_ex_r()` at 2435553.5 on Moshier,
within 1e-9 deg of 23.25: -4.263e-14 deg. Before the fix 2.24e-5 deg. The
upstream build passes (d): its constant and its series agree with each
other.

G24 `lahiri_icrc_t0_off`: mode 46 | `SE_SIDBIT_PREC_ORIG`, the mean
ayanamsa at t0 (SEFLG_NONUT) less 23.25 - 0.00464207, in arcseconds:
-0.0806 shipped, 0.0000 compat.

## 154. `swe_set_jpl_file()` of a file below DE 403, or of one not there, keeps the IERS corrections a DE 403 file loaded: SEFLG_JPLHOR answers with them where a fresh process falls to SEFLG_JPLHOR_APPROX

**Severity: Correctness (behaviour: a setting without effect; dpsi, the ayanamsa and every place of date under SEFLG_JPLHOR 4.20e-2" off a fresh process's on G134's fixtures, under SEFLG_JPLHOR's return flag), both builds**
**Where:** `sweph.c`: `swe_set_jpl_file_r()`

Entry 144's noticed item. `swe_set_jpl_file()` loads the corrections
(`load_dpsi_deps()`) for a file of DE 403 or later, and for any other file
did nothing about them: those a DE 403 file had loaded from the same
directory stayed, `eop_dpsi_loaded` 1, and `plaus_iflag()` kept
SEFLG_JPLHOR. Measured on G19's synthetic DE431 header in G134's
directory A (G122's corrections A, de200.eph linked in), Lahiri: path A,
the fixture and `SE_ECL_NUT` with SEFLG_JPLEPH | SEFLG_JPLHOR |
SEFLG_SPEED at 2000 Jan 7, then de200.eph, and dpsi at Jan 7.37 was A's
-13.530688" under SEFLG_JPLHOR (return 393473) where a fresh process
with path A and de200.eph answers SEFLG_JPLHOR_APPROX's -13.488712"
(524545, "you did not call swe_set_jpl_file()"), 4.20e-2" (1.166e-5
deg); interpolated -13.531351" for -13.489375"; `swe_get_ayanamsa_ex()`
85872.809871" for 85872.851847"; the same after `swe_set_jpl_file()` of a
file that is not there, and on an explicit context's
`swe_set_jpl_file_r()`. Both builds (the compat build -13.530688" for its
-13.488785"). A thread adopting de200.eph already drops them (entry 143's
`swi_dpsi_deps_adopt()`), and de200.eph then the fixture loads A's, as a
fresh process with the fixture does.

**Fix, as done here:** `swe_set_jpl_file_r()` drops the corrections when
the file it opened loads none -- the open failed, or its DE number is
below 403 -- with `dpsi_deps_free()`, as the adoption does: the tables
freed, `eop_dpsi_loaded` back to 0, and what was computed from them, the
nutation kept at an instant, the topocentric observer and the
interpolation's nodes built under SEFLG_JPLHOR. The same file again
(entry 103's early return) keeps them; a file of DE 403 or later loads
them as before (the same directory's, kept once loaded). Behind `#ifndef
SWE_UPSTREAM_COMPAT`. No hot path: it runs only in the setter.

G134 `tests/eopfollow.c`, extended with 16 checks, each in a forked
process, bit for bit with its return flag against a fresh process that
set path A and the last file: `SE_ECL_NUT` and `swe_get_ayanamsa_ex()`
with SEFLG_JPLHOR at Jan 7 and 7.37 after path A, the fixture and
de200.eph (also interpolated), or a file not there, or de200.eph and then
the fixture, on the setter's thread and an explicit context, and a second
thread adopting de200.eph. 94 checks; before the fix 10 of the 16 fail,
all but the other way round and the adopting thread; the compat build
fails the same 10 (and entry 144's 8). G24 `eop_jpl_kept_off`: dpsi at
Jan 7.37 after A, the fixture and de200.eph against a fresh context's: 0
shipped (4.20e-2" before the fix), 4.19e-2" compat.

## 155. `load_dpsi_deps()` loads again over the tables a load it gave up on left allocated: two tables, 589824 bytes, leaked a reload

**Severity: Memory (a leak: 589824 bytes a reload after an IERS file the loader gave up on), both builds**
**Where:** `sweph.c`: `load_dpsi_deps()`

Entry 144's noticed item. `load_dpsi_deps()` allocates `dpsi[]` and
`deps[]` (36525 doubles, 292200 bytes each) before it reads
eop_1962_today.txt, and a file it gives up on -- a day missing
(`eop_dpsi_loaded` -2), or an eop_finals.txt that breaks off (-3) --
leaves them allocated with the flag at or below 0. Its load-once guard
only skips a load above 0, so the next `swe_set_jpl_file()` of a DE 403
file (in the shipped build another name: the same one again is entry
103's early return) allocated both anew over them, and only `swe_close()`
(since entry 144 a path change too) freed the last pair. Measured with mallinfo2() (`uordblks` + `hblkhd`) over ten
`swe_set_jpl_file()`s in turn of G19's DE431 fixture and another name of
it: 4718592 bytes more after the last eight, 589824 a load (two tables
of 294912 mapped), with a day missing and with eop_finals.txt broken off
alike; the fixture and de200.eph in turn 2359296 (four loads), which
entry 154's fix closes in the shipped build, de200.eph's setter freeing
them. Both builds.

**Fix, as done here:** `load_dpsi_deps()` frees what a previous load
left before it allocates: nothing reads the tables while
`eop_dpsi_loaded` <= 0 (`plaus_iflag()`, and since entry 142
`swi_jplhor_plaus()` for every other reader, take SEFLG_JPLHOR as
SEFLG_JPLHOR_APPROX there). In both builds, as entry 104's crash fix: a
leak's fix, it changes no answer, and the compat build reproduces
upstream's answers, not its leaks. No hot path: the loader runs in the
setters and the adoption.

G134 `tests/eopfollow.c`, extended with 3 checks, each in a forked
process: the ten setters with a day missing, with eop_finals.txt broken
off, and of the fixture and de200.eph with a day missing; the bytes
allocated after the last eight may not grow by one table, and no IERS
file may be left open. calloc() is the gate's own, counting the tables
(20, 20 and 10 asked for). 97 checks; before the fix the first two fail
(4718592 bytes), the third in the compat build (2359296), all three on
the tip before entry 154; after it both builds pass the three. No G24
case: the fix is in both builds.

## 156. `load_dpsi_deps()` leaves the IERS file open when a table's allocation fails, and when a full table meets more days in eop_finals.txt; `dpsi[]` stays allocated when `deps[]`'s allocation fails

**Severity: Memory (a file descriptor leaked a call, and 294912 bytes with it when the second allocation fails), both builds**
**Where:** `sweph.c`: `load_dpsi_deps()`

By reading, measured here. Of the loader's early returns three skipped
`fclose()`: `dpsi[]`'s allocation failing and `deps[]`'s left
eop_1962_today.txt open, `deps[]`'s with `dpsi[]` allocated; and a table
full at 36525 days returned from the eop_finals.txt loop with that file
open (upstream's own return; a real eop_1962_today.txt reaches 36525 days
in 2062, a crafted one at once). The others close the file, and the
missing file's allocates nothing. Measured with the gate's calloc()
failing the first, then the second table-sized allocation, and a
36525-day eop_1962_today.txt with an eop_finals.txt of one day more:
after `swe_set_jpl_file()` of G19's DE431 fixture one descriptor open on
eop_1962_today.txt in the first two, on eop_finals.txt in the third
(/proc/self/fd, and strace's open and close calls); 326768 bytes left
allocated after `deps[]`'s failure for the JPL file's 31856. Both builds.

**Fix, as done here:** each of the three returns closes the file, and
`deps[]`'s failure frees `dpsi[]` too. In both builds, as entry 155's: a
leak's fix, the flag and every answer as before. No hot path.

G134 `tests/eopfollow.c`, extended with 3 checks, each in a forked
process: after `swe_set_jpl_file()` of the fixture with `dpsi[]`'s
allocation failing, with `deps[]`'s, and with the full table, no
descriptor open on an IERS file and, after a failed allocation, the bytes
allocated under one table more than before the call. 100 checks in all;
before the fix the 3 fail in both builds, after it both pass them (the
compat build failing only entries 144's and 154's 18, behind the switch).
No G24 case: the fix is in both builds. With the three fixes (entries
154-156) G19, G103, G116, G122, G127, G132, G24, G2 (check-threads) and
check-ctx pass.

## 158. `swe_calc_pctr()` overwrites the centre's light-time read's return with the body's before testing it: a centre whose files begin after the light-time instant is taken as zero there, the speed 5.09e-3 deg/day off

**Severity: Correctness (the speed off, returned OK: 5.09e-3 deg/day on Mars from Ceres, 0.130 in the compat build; the position right)**
**Where:** `sweph.c`: `swe_calc_pctr()`

Entry 150's noticed item. After solving the light time, `swe_calc_pctr()`
reads the centre and then the body again at t = tjd - tau, both into
`retc`, and tests `retc` only at the end, so a failure of the centre's
read is never seen: `swe_calc()` zeroes its output on ERR, and the
centre's vector at t enters the aberration rate as zero. Upstream adds the
centre's velocity at tjd less that at t to the body's velocity (bug 34's
first-order term); the shipped build, since entry 77, takes the centre's
acceleration at tjd from its velocities at t, tjd and tjd + 2^-10. The
position does not read it.

Both reads use the same files when the centre and the body are planets.
They do not when the centre's files begin later than the body's, so the
centre's read at tjd succeeds and the one at t, a light time earlier,
does not. Mars from Ceres in a directory of seas_18.se1 (Ceres from 1800
Jan 1, JD 2378496.5) and sepl_12, sepl_18, semo_12 and semo_18, at t0 +
k/256 + 2^-10 day, k = 0..5 (tau is 0.024 day there): return 258
(SEFLG_SWIEPH | SEFLG_SPEED), the longitude speed 0.276774 deg/day where
five-point differences give 0.281859, 5.09e-3 off; the compat build
0.151762, 0.130 off. The answer with SEFLG_SPEED is wrong exactly in that
window and right after it (3.5e-12 deg/day). The positions in the window
step by -2.6e-8 deg from those after it, with SEFLG_SPEED or without:
that is the files' own junction at 1800 Jan 1, Mars at t being read from
sepl_12.se1 there and from sepl_18.se1 after, not this.

**Fix, as done here:** the shipped build reads the centre at t only where
the speed's aberration rate needs it (SEFLG_SPEED without SEFLG_NOABERR),
tests that read, and refuses the call when it fails, with its error
message, as the read at tjd + 2^-10 for the same rate already does. The
position without the speed, and SEFLG_SPEED | SEFLG_NOABERR, need no
centre at t and are answered. Behind `#ifndef SWE_UPSTREAM_COMPAT`. One
read fewer without the speed or the aberration, no hot path otherwise.

G144 `tests/pctredge.c`, in that directory: (a) at the six instants
SEFLG_SPEED is refused, naming seas_12.se1; (b) the same instants answer
OK without the speed and with SEFLG_NOABERR; (c) at four instants beyond
the window, every stencil point in sepl_18.se1, the speeds within 1e-8
deg/day and 1e-11 AU/day of five-point differences over 2^-7 day: 3.5e-12
deg/day. 30 checks pass. Before the fix (a) fails its 6 (258); the compat
build fails 18, these 6 and (c)'s 12 by its own speeds, 1.7e-6 deg/day
(entry 77). G117, G76, G47, G92, G104, G126, G133, G77, G62, G99 and G24
pass.

G24 `pctr_ctr_edge_ret`: Mars from Ceres with SEFLG_SPEED at t0 + 2^-10
day in that directory, the return: -1 shipped, 258 compat.

## 171. An eop_finals.txt that breaks off throws away a complete eop_1962_today.txt: SEFLG_JPLHOR falls to SEFLG_JPLHOR_APPROX, dpsi 4.20e-2" off

**Severity: Correctness (behaviour: dpsi and the ayanamsa 4.20e-2" off on G134's fixtures, under SEFLG_JPLHOR, wherever eop_finals.txt has a gap after the days it adds)**
**Where:** `sweph.c`: `load_dpsi_deps()`

Found by entry 154's agent. `load_dpsi_deps()` reads eop_1962_today.txt
(EOPC04) into `dpsi[]` and `deps[]`, sets `eop_dpsi_loaded` 1, and then
appends the days of eop_finals.txt (finals.all) that follow it, one-day
steps, Bulletin B or A. A day missing there set -3, under upstream's
comment "no error, as we do have data; however, if this file is usefull,
then swed.eop_dpsi_loaded will be set to 2". But `plaus_iflag()` takes
every value <= 0 as no corrections ("file eop_finals.txt corrupt; default
to SEFLG_JPLHOR_APPROX"), so the complete EOPC04 table, and the finals
days before the gap, went with the broken file. In G134's directory F
(fixture A's eop_1962_today.txt, MJD 51544-51583, and an eop_finals.txt of
51584 and 51586), `SE_ECL_NUT` with SEFLG_JPLEPH | SEFLG_JPLHOR |
SEFLG_SPEED at 2000 Jan 7.37 answered SEFLG_JPLHOR_APPROX's -13.488712"
(return 524545) where the same directory with an eop_finals.txt of 51584
alone answers -13.530688" (393473): 4.20e-2" (1.166e-5 deg). At MJD
51583.5, where the finals day enters the interpolation, 1.184e-5 deg; at
51590, past both, 1.150e-5; `swe_get_ayanamsa_ex()` the same.

Fix, as done here (behind `#ifndef SWE_UPSTREAM_COMPAT`): the gap sets
`SWI_EOP_FINALS_CUT` (3), a loaded state; `eop_tjd_end` is already the
last day before the gap, so the table holds what a finals file ending
there gives, and dates past it take its last value, as past any table's
end. `plaus_iflag()` keeps SEFLG_JPLHOR and writes "file eop_finals.txt
corrupt after JD n; its later days not used" in serr, so the broken file
is still reported. A gap in eop_1962_today.txt itself (-2) is left as
upstream has it: the main file corrupt, nothing taken from it.

G134 `tests/eopfollow.c`, extended with 8 checks: in F, `SE_ECL_NUT` and
`swe_get_ayanamsa_ex()` with SEFLG_JPLHOR at Jan 7.37, MJD 51583.5 and
51590, bit for bit with the return against a fresh process in P (A's
eop_1962_today.txt and an eop_finals.txt of 51584 alone); SEFLG_JPLHOR
kept in F with serr naming eop_finals.txt; and entry 172's warning. 108
checks in all. Before the fix the 8 fail; the compat build fails 27: the
6 and the first warning, entry 144's 8, entry 154's 10, and since entry
148's reference the 2 of the second thread after path B alone. G24
`eop_finals_cut_off`: dpsi at Jan 7.37 in a directory whose eop_finals.txt
has a gap at its first day, against the same after the file is removed:
0 shipped, 4.19e-2" compat.

## 172. A failed allocation of the IERS corrections' tables reports eop_1962_today.txt "not found"

**Severity: Robustness (message only: the warning names a missing file that had just been opened; no answer changes)**
**Where:** `sweph.c`: `load_dpsi_deps()`, `plaus_iflag()`

Found by entry 154's agent, by reading. When `calloc()` of `dpsi[]` or
`deps[]` fails, `load_dpsi_deps()` sets `eop_dpsi_loaded` to ERR, the
value of eop_1962_today.txt not found, after having opened it; SEFLG_JPLHOR
then falls to SEFLG_JPLHOR_APPROX with "file eop_1962_today.txt not found;
default to SEFLG_JPLHOR_APPROX". Measured with G134's calloc() failing
`dpsi[]`'s table: return 524289, that text.

Fix, as done here: the failure sets `SWI_EOP_NOMEM` (-4), and
`plaus_iflag()` says "no memory for the corrections of eop_1962_today.txt;
default to SEFLG_JPLHOR_APPROX". In both builds, as entry 156's fix of the
same branch: the text of a failure, which nothing computes from. G134's
warning check: the memory named, SEFLG_JPLHOR dropped; before the fix it
fails. No G24 case: the fix is in both builds.

## 161. Under `SEMOD_JPLHORA_3` the frame bias and the Horizons offset take the other side of the step at 1962 Jan 20 than the precession and nutation, and another instant: every body 1.1e-5 deg off over its light time after it, Polaris 4.95e-5 in right ascension at it

**Severity: Correctness (with SEFLG_JPLHOR_APPROX under its default mode, on a JPL file of DE 403 or later: every body 1.11e-5 to 1.24e-5 deg off over its light time after 1962 Jan 20 0h TT, the true Sun at every date after it; at the instant Polaris 1.15e-5 deg in longitude and 4.95e-5 in right ascension, true Mars 1.12e-5, the topocentric Moon 1.7e-7; SEFLG_SPEED3 0.162 deg/day there)**
**Where:** `swephlib.c`: `swi_bias()`; `sweph.c`: `app_pos_etc_plan()`, `app_pos_etc_sun()`, `app_pos_etc_moon()`, `swe_calc_pctr()`

Entry 153's noticed item. With `SEFLG_JPLHOR_APPROX` and
`SEMOD_JPLHORA_3`, `swi_precess()`, `swi_epsiln()` and `calc_nutation()`
take SEFLG_JPLHOR's models up to and including
`HORIZONS_TJD0_DPSI_DEPS_IAU1980`, JD 2437684.5 = 1962 Jan 20 0h TT, and
the chosen models after it (entry 153). The frame bias (23 mas) and the
Horizons offset in right ascension (`swi_approx_jplhor()`, -51.26 mas
then) go with the chosen models, and `swi_bias()` left them out for tjd <
the step only. Nor was it given the models' instant:
`app_pos_etc_plan()`, `app_pos_etc_sun()`, `app_pos_etc_moon()` and
`swe_calc_pctr()` pass t, the instant the body was read at, teval - tau,
and precess and nutate at teval; `app_pos_etc_sun()` leaves t at 0 under
SEFLG_TRUEPOS (entry 77's fault, fixed there for `swe_calc_pctr()`). So:
- at the step instant a fixed star (given tjd), a true place (t = teval)
  and the topocentric site (`swi_get_observer()` at tjd) took the left
  side's precession and nutation with the right side's bias and offset;
- over the light time after it, teval in (step, step + tau], the Sun, the
  Moon and the planets took the chosen models without them;
- the true Sun, given 0, at every date after it; under `SEMOD_JPLHORA_1`
  the true Sun took the table's first offset, 1962's, at every date (in
  2024 1.0 mas in right ascension off).

Measured on de200 read as DE 431 (the context's DE number set after the
file opens: no file of DE 403 or later is here, and the bias and the
offset are rotations whatever the coefficients), the default models.
After the step, against `SEMOD_JPLHORA_1`, which has the chosen models
with the bias and the offset throughout: every body with a light time
1.11e-5 to 1.24e-5 deg (ecliptic and equatorial) over it -- the Sun's 8.2
minutes, the Moon's 1.3 s, Mars's 20 minutes, Saturn's 91, heliocentric
Mars, the topocentric Moon -- and the true Sun throughout. At the step,
against the place's five-point extrapolation from the left: Polaris
1.15e-5 deg in longitude, 4.95e-5 in right ascension, true Mars 1.12e-5,
the topocentric Moon 1.7e-7 (its site's frame). SEFLG_SPEED3 at
the step, whose places at t - dt and t - 2 dt were the left side's (entry
153) and t's had the right side's bias: true Mars 0.162 deg/day. The
speeds are otherwise not affected: the bias and the offset are fixed
rotations on each side and no rate differences them.

Reached with a JPL file of DE 403 or later (`swi_get_denum()` >= 403 for
the planets, the Sun, the Moon and the site; a barycentric fixed star
takes the bias whatever the file), and through the fallback: with the
JPL file missing and the fallback allowed, SEFLG_JPLEPH falls to the
Swiss files keeping SEFLG_JPLHOR_APPROX, and their DE 431 takes the bias.
Not affected: `SEMOD_JPLHORA_2` (no bias, ICRS), `SEMOD_JPLHORA_1` but for
the true Sun's date, SEFLG_JPLHOR (the bias throughout), SEFLG_ICRS (none),
and what `plaus_iflag()` drops the Horizons flags from: sidereal places,
the nodes, apsides and fictitious bodies.

**Fix, as done here:** `swi_bias()` leaves the bias and the offset out
for tjd <= the step, the instant on its models' side, and
`app_pos_etc_plan()`, `app_pos_etc_sun()` and `app_pos_etc_moon()` give
it teval, `swe_calc_pctr()` tjd: the instants they precess and nutate at.
The offset's table is read at that instant too, a light time later; its
slope is at most 3.8e-10 deg/day (entry 162), 6e-11 deg over Neptune's
four hours. Without SEFLG_JPLHOR_APPROX `swi_bias()` does not read its
date, so every other place is as before, bit for bit. Behind `#ifndef
SWE_UPSTREAM_COMPAT`, upstream's under `#else`.
- After: `SEMOD_JPLHORA_3` is `SEMOD_JPLHORA_1` bit for bit after the
  step, within the light time too; at the step every place within 3.0e-11
  deg of its extrapolation from the left (the Moon; the others 6e-13);
  SPEED3 there 1.01e-9 deg/day.
- No hot path: the test is inside `swi_bias()`'s SEFLG_JPLHOR_APPROX
  branch, and the callers pass another double. `perf stat` instructions,
  `swe_calc()` of the ten bodies with speed at 20000 instants on the Swiss
  files: 15887299279 before, 15888470747 after.

G141 `tests/jplhorbias.c`: de200 read as DE 431, SEFLG_JPLHOR_APPROX, the
default models; the Sun, the Moon, Mars, Saturn, Polaris, the true Sun
and Mars, the topocentric Moon and heliocentric Mars, ecliptic and
equatorial. After the step, at 148 instants in (step, step + 1] (every
1/64 day, every 1/1024 day over the first 1/16, step + 2^-k for k =
7..26), the place under `SEMOD_JPLHORA_3` against `SEMOD_JPLHORA_1`'s; at
the step against the five-point extrapolation from the left over 2^-12
day; both within 1e-9 deg; and SPEED3 at the step within 1e-5 deg/day: 37
checks. Measured as above. Before the fix 21 of 37 fail: after the step
every body with a light time and the true Sun, at the step Polaris, true
Mars and the topocentric Moon, and SPEED3. The upstream build fails 25:
those 21, and at the step Mars and heliocentric Mars by 1.3e-9 and the
topocentric Moon by 1.1e-6, its own defects that earlier entries fixed.
G24 `jplhora_bias_side_off`: Mars 2^-8 day after the step on the Swiss
files the fallback reaches, against `SEMOD_JPLHORA_1`, 0 deg shipped
(1.23e-5 before the fix), 1.23e-5 compat; barycentric Polaris on de200
at the step against 2^-20 day before it less its motion, 8.2e-14 shipped
(4.92e-5 before), 4.92e-5 compat.

## 162. `swi_approx_jplhor()` turns the position by the Horizons offset in right ascension and not the velocity: the Moon's latitude speed 1.46e-6 deg/day off its places' rate, Mars's right ascension 1.25e-7

**Severity: Correctness (with SEFLG_JPLHOR_APPROX under `SEMOD_JPLHORA_1` at every date and under `SEMOD_JPLHORA_3`, the default, after 1962 Jan 20, on a JPL file of DE 403 or later: speeds off their places' rate by the offset times the transverse velocity, the Moon's latitude 1.46e-6 deg/day and its right ascension 5.6e-7, Mars's right ascension 1.25e-7, the Sun's latitude 1.02e-7; distance rates 7.2e-9 AU/day on Mars, 4.1e-9 on the Sun)**
**Where:** `swephlib.c`: `swi_approx_jplhor()`

Entry 153's second noticed item (estimated there at 1.4e-7 deg/day on
Mars, 6e-8 on the Moon). `swi_bias()` turns the position and the velocity
by the frame bias's matrix, and `swi_approx_jplhor()` then (before it,
backward) turns the position about the pole by the Horizons offset in
right ascension -- -51.257 mas before 1962 Jul 1 (`DCOR_RA_JPL_TJD0`), a
yearly table to 2012, -52.255 mas after it -- by `swi_cartpol()`, the
longitude plus the offset, and `swi_polcart()`, which convert the
position alone. The velocity stayed unturned, off by the offset (2.5e-7
rad) times z x v: in right ascension the offset times rho'/rho (rho the
distance from the pole's axis), in declination and the ecliptic latitude
about the offset times the right ascension's motion, in the distance rate
the offset times rho^2 alpha' / r. Reached as entry 161: on a JPL file of
DE 403 or later, or the Swiss files through the fallback, by the Sun, the
Moon, the planets, the fixed stars (too slow to show: Polaris within
1.84e-9) and the topocentric site's velocity (`swi_get_observer()`,
backward); not under `SEMOD_JPLHORA_2`, which has no offset.

Measured on de200 read as DE 431 (as entry 161) and on the Swiss files
through the fallback, alike, the default models, against five-point
differences of the places over 2^-6 day centred at binary-fraction
instants on seven dates 1950-2024 (the topocentric Moon nine-point over
2^-7, delta t 2^-11 day), under `SEMOD_JPLHORA_1` and after the step
`_3`: the Moon's ecliptic latitude speed 1.46e-6 deg/day off, its right
ascension 5.6e-7 (the topocentric Moon's alike), Mars's longitude 1.17e-7
and right ascension 1.25e-7, the Sun's latitude 1.02e-7, Saturn's right
ascension 2.6e-8, heliocentric Mars's latitude 5.8e-8; the distance rates
7.2e-9 AU/day on Mars, 5.1e-9 on Saturn, 4.1e-9 on the Sun, 1.5e-10 on
the Moon. With the bias alone (SEFLG_JPLHOR_APPROX off) the same within
1.1e-9 deg/day (the Moon on the Swiss files) and 4e-13 AU/day.

**Fix, as done here:** with SEFLG_SPEED the velocity is turned about the
pole by the position's angle, (cos a v_x - sin a v_y, sin a v_x + cos a
v_y), a the offset (negative backward); the position's path is
upstream's. The table's own rate is left out: its slope is at most 0.496
mas a year, 3.8e-10 deg/day, under the residual floor. Behind `#ifndef
SWE_UPSTREAM_COMPAT`, an addition (upstream's code is unchanged).
- After: within 3.6e-10 deg/day (the topocentric Moon 8.5e-10, its
  reference's own), the distance rates within 1.7e-13 AU/day. What is
  left, 2.6e-10 to 3.0e-10 deg/day in longitude and right ascension on
  every body, is the table's rate.
- No hot path: a sine, a cosine and four products under
  SEFLG_JPLHOR_APPROX with SEFLG_SPEED. `perf stat` instructions, as
  entry 161's: 15887299279 on the tip, 15888745518 with both fixes.

G141 `tests/jplhorbias.c`, extended with 36 checks: the nine bodies'
speeds, ecliptic and equatorial, against five-point differences over
2^-6 day (the topocentric Moon nine-point over 2^-7) centred at
binary-fraction instants on the seven dates under `SEMOD_JPLHORA_1` and
the six after the step under `_3`, none within a day of a year of the
offset's table; the worst of the two angles within 1e-8 deg/day, of the
distance within 1e-11 AU/day (not Polaris's). 73 checks. Measured as
above. Before the fix 32 of the 36 fail, Polaris passing; the upstream
build fails all 36, by these and its own speed defects that earlier
entries fixed (ecliptic speeds 1.5e-5 to 5.3e-5 deg/day, the topocentric
Moon 4.4e-3, Saturn's distance rate 1.9e-5 AU/day), 61 of 73 in all.
G24 `jplhora_vel_off`: true Mars, J2000 equatorial cartesian (no frame
rate in it), on the Swiss files the fallback reaches, at 2000 Jan 1.797,
the velocity against the five-point difference over 2^-6 day: 1.08e-12
AU/day shipped (4.57e-9 before the fix), 4.57e-9 compat. With entries
161 and 162 G141 (73), G24 (both builds), G139 (check-framestep), G122
(check-nutintp) 350, G134 (check-eopfollow) 100, G19 (check-eopload), G75
(check-staraberr) and G40 (check-moonspeed) pass.

Noticed, not fixed:
- The offset table is interpolated backwards: (t - t0) (dcor[t0] -
  dcor[t1]) + dcor[t0] runs away from the next year's value, and the
  offset jumps by 2 (dcor[t1] - dcor[t0]) at every year of the table
  (`DCOR_RA_JPL_TJD0` + 365.25 k, 1963-2012): up to 0.99 mas, 2.8e-7 deg
  in right ascension (Polaris's measured 1.03 mas at JD 2453917.5), its
  slope between the wrong way. Both builds. Fixed as entry 173.
- Its rate, the slope as interpolated, is left out of the speeds: at most
  3.8e-10 deg/day, under the floor (above).
- A JPL request that falls to the Swiss files keeps SEFLG_JPLHOR_APPROX,
  and their DE 431 takes the bias and the offset, where `plaus_iflag()`
  drops the flag from a Swiss request: the same Swiss place 52.9 mas
  apart in right ascension (Mars at J2000) as asked of SEFLG_JPLEPH with
  the fallback or of SEFLG_SWIEPH, the return flag naming the
  approximation in the first. Left as it is, not a defect: the request
  asked for the Horizons approximation of a JPL file of DE 403 or later,
  the Swiss files reproduce DE 431 to about 1 mas, so the approximation
  applied to them is the nearest answer to the one asked, and the return
  names both the files and the approximation; a Swiss request cannot ask
  for it.

## 173. The Horizons approximation's offset table is interpolated backwards: the offset jumps up to 0.99 mas at every year from 1963 to 2011

**Severity: Correctness (positions under SEFLG_JPLHOR_APPROX, SEMOD_JPLHORA_1 at every date and SEMOD_JPLHORA_3 after 1962, up to 0.99 mas off and discontinuous at each year of the table)**
**Where:** `swephlib.c`: `swi_approx_jplhor()`

Entry 162's noticed item. The Horizons approximation turns a place about
the pole by an offset in right ascension, -51.257 mas before 1962 Jul 1
(`DCOR_RA_JPL_TJD0`), then a table of yearly values, `dcor_ra_jpl[]`, to
2012, and -52.255 mas after. Between two years the code took
(t - t0) (v[t0] - v[t1]) + v[t0], the difference the wrong way round: the
offset ran from v[t0] away from v[t1], ended the year at 2 v[t0] - v[t1],
and jumped by 2 (v[t1] - v[t0]) to the next year's value. The largest
step in the table, -52.292 to -51.796 mas into 2006 Jul 1, made a jump of
0.992 mas: Polaris's barycentric right ascension under SEMOD_JPLHORA_1
moved 2.87e-7 deg in 2^-9 day across JD 2453917.5 beyond its motion,
Mars's 2.76e-7. Between the years the offset was off by up to that much
too. Both builds.

**Fix, as done here:** linear between the years' values, (t - t0)
(v[t1] - v[t0]) + v[t0], behind `#ifndef SWE_UPSTREAM_COMPAT`. The places
are continuous across the table's years to the stencil's own error (Mars
2.0e-12 deg, Polaris 2.9e-10 through its speed). The offset's rate, now
the table's slope, stays out of the speeds, at most 3.8e-10 deg/day
(entry 162). No hot path: the approximation's own branch.

G141 `tests/jplhorbias.c`, extended with 2 checks: Polaris's and Mars's
right ascension under SEMOD_JPLHORA_1 at the start of each of the table's
49 inner years, the place 2^-10 day after less that 2^-10 day before
less 2^-9 day of their mean speed, the worst within 1e-9 deg. 75 checks.
Before the fix both fail (2.87e-7 and 2.76e-7 at 2006 Jul 1), and so in
the compat build. G24 `jplhora_table_jump`: barycentric Polaris on
de200, across 2006 Jul 1: 1.5e-11 deg shipped, 2.90e-7 compat.

## 164. A JPL file whose `swe_set_jpl_file()` failed along the path then set, and a path where it is: no DE number was read, so the path setter reads none, and the first JPLEPH delta t takes the default tidal term until a calculation opens the file

**Severity: Correctness (behaviour: call order -- the first UT JPL Moon in AD 1700 1.79e-3 deg off the second, the first JPLEPH delta t 11.27 s off, `swe_deltat()` 12.08 s off and its guess Swiss for a configuration naming de200.eph -- on every thread and context)**
**Where:** `sweph.c`: `swe_set_jpl_file_r()`, `swe_set_ephe_path_r()` (bug 148's condition), `ctx_release()` (bug 131's); `sweconfig.c`: `swi_config_capture()`, `swi_config_apply()`, `cfg_merge()`, `swi_config_reset()`; `sweph.h`, `sweconfig.h`

Entry 148's noticed item. Bug 148 has `swe_set_ephe_path()` read the kept
JPL file again along the new path, and bug 131 has the first entry after
`swe_close()` read it, when a DE number had been read for it
(`jpldenum_cfg`, or owed since a close). A `swe_set_jpl_file()` that
failed along the path in effect read none, so neither fired. After a path
without de200.eph (a /tmp directory of links to EPHEDIR's other files),
`swe_set_jpl_file("de200.eph")` and the path to EPHEDIR: `swe_calc_ut()`'s
JPL Moon in AD 1700 40.0057507520 deg on the first call and 40.0039614086
on the second, which the first had opened the file for; `swe_deltat_ex()`
there with SEFLG_JPLEPH 14.286718 s (the default term) and `swe_deltat()`
15.091463 s (its guess Swiss, the moon file's -25.936); after a TT JPL Sun
the getter -25.936 and `swe_deltat()` in AD 1800 18.902020 s. A fresh
process given EPHEDIR and de200.eph answers 40.0039614086 twice, 3.012003
s for both delta t, -23.8946 and 14.438958 s. The same on the setting
thread, a thread running across the path, a thread started and a context
made after it, and an explicit context. And with `swe_close()` for the
path: the directory without de200.eph, the failed setter, de200.eph then
linked into it and `swe_close()`: the same values, where a fresh process
given that directory and de200.eph answers as above. The configuration did
not record that a file had been named: the name alone cannot say so -- it
starts as `SE_FNAME_DFT`, and a calculation's fallback to de406.eph
rewrites it. The compat build the same (40.0057510966, then
40.0039617599), as entry 148's.

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`; the compat
build keeps upstream's first and second answers):
- `swe_set_jpl_file()` marks the file named (`jpl_named`), found or not.
  The flag is in the configuration's path group with the name: captured,
  merged, compared and adopted as the name is, so a thread adopting the
  setter or a later path and a context made after carry it; the compat
  build never sets it.
- Bug 148's condition in the path setter, and bug 131's in `swe_close()`
  (`ctx_release()`, and `swi_config_reset()` for the master), is that
  flag: the named file is read again along the new path, or owed to the
  next entry after the close, as a fresh process's `swe_set_jpl_file()`
  reads it whether or not an earlier one found it.
- The opens, `fopen()` interposed, 100 rounds after the setters and a
  warm-up, before and after: a path alternating between EPHEDIR and
  EPHEDIR/. and a JPL Moon each round, 200 calls and 100 opens of
  de200.eph both (the setter's open replaces the calculation's); with a
  Swiss Moon each round 300 calls and no open before, 400 calls and 100
  opens after, as a process given EPHEDIR and de200.eph from the start
  makes (400 and 100); with the named file nowhere along the path, 300
  calls before and 400 after, one failed attempt per path change, which a
  fresh process's setter makes there too. No hot path.

G132 (`tests/jpldenum.c`, entry 137) gains two settings: the first path a
directory without de200.eph and the last setter the path to EPHEDIR, and a
second such directory, de200.eph linked into it and the last setter
`swe_close()`, each against a fresh process given the final path and
de200.eph: 7 settings x 5 readers x 5 observations and one check, 176
pass. Before the fix 30 fail, 15 in each setting, on all 5 readers alike:
the JPLEPH delta t, the getter after the JPL Sun with `swe_deltat()` in
1800, and the first Moon, as above. The compat build fails 85 of 176 with
the fix and without: entry 148's 55 of 126, and in the two settings 30 by
the upstream behaviour it restores, the first JPLEPH delta t and the first
Moon (bug 148's, 10 each) and the flag-0 delta t (bug 145's, 10). G24
`jpl_named_moon_off`, the first UT JPL Moon in AD 1700 after the
directory without the file, de200.eph and EPHEDIR against the second: 0
shipped, 1.79e-3 deg compat (and shipped before the fix).

## 165. With no JPL file set, the first JPLEPH delta t takes the default tidal term and the calculation after it opens the default file by its name: the first answer differs from the second, in a fresh process too

**Severity: Correctness (behaviour: call order -- the first UT JPL Moon in AD 1700 1.79e-3 deg off the second where de431.eph is not DE431, the JPLEPH delta t 11.27 s apart before and after a JPL calculation; with de431.eph missing and the fallback's de406.eph, by -25.826 for -25.80, 0.154 s in AD 1700 by the same proportion -- on every thread and context)**
**Where:** upstream `swephlib.c` `swi_get_tid_acc()` (the JPL file's DE number only when it is open), `sweph.c` `open_jpl_file()` (the fallback to de406.eph); here `swephlib.c`: `swi_get_tid_acc()`; `sweph.c`: new `swi_jpl_open_first_use()` and `jpl_open_note_say()`, `open_jpl_file()`, `swecalc()` and `jplplan()` (their opens), `swi_close_keep_topo_etc()`, `ctx_release()`; `sweconfig.c`: `swi_config_apply()`; `sweph.h`

Entry 148's noticed item. With no `swe_set_jpl_file()` the JPL file's
name is the default, `SE_FNAME_DFT` (de431.eph), and nothing has read its
DE number: `jpldenum_cfg` is published by the setter alone (the
2026-09-16 review's F4), and the file is open only once a calculation has
opened it. So a JPLEPH delta t took the default term, -25.80, and the
calculation after it opened the file by its name, its term the file's
from then on. With a de431.eph that is de200.eph (a /tmp directory of
links), in a fresh process given that path alone: `swe_calc_ut()`'s JPL
Moon in AD 1700 40.0057507520 deg on the first call and 40.0039614086 on
the second; `swe_deltat_ex()` in AD 1700 with SEFLG_JPLEPH 14.286718 s
before any JPL calculation and 3.012003 after a JPL Sun; the same on a
thread running across the path, a thread started and a context made
after it, and an explicit context. Where de431.eph is missing and
de406.eph is there, the calculation's fallback opens de406.eph ("Error
with JPL ephemeris file de431.eph: ... Defaulting to de406.eph", only when
there is a serr to write it in) and rewrites the name: the same two
Moons with that warning on the first, where de406.eph is de200.eph; with
a real DE406 its -25.826 for -25.80, 0.154 s of delta t in AD 1700 and
2.4e-5 deg of the Moon by the same proportion (no DE406 file here). With
a real DE431 file the term is the default's, and no answer moves. The
compat build the same (40.0057510966, then 40.0039617599).

The decision. The default name does not count as named for bug 164's
flag: the path setter would then open de431.eph at every path change for
every caller, or try it where none is installed (a failed `fopen()` per
path element), and the guess (`swe_deltat()`, and the functions without a
flag, bug 132) would turn JPL wherever a de431.eph is found, DE431's
-25.80 for the Swiss files' -25.936 (11.29 s of delta t in AD 1000), for a
configuration that names no JPL file (entry 138: a calculation does not
choose for the guess). The file a JPLEPH request uses is the default
file, or its fallback, along the path: its DE number is read where it is
first needed, as bug 107 reads the moon file's at the first tidal term.

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`; the compat
build keeps upstream's first and second answers):
- `swi_get_tid_acc()`, for SEFLG_JPLEPH with no DE number -- no file
  open, none published -- opens the file first
  (`swi_jpl_open_first_use()`), as the calculation would: its name along
  the path, and the fallback to de406.eph only when the delta t has a
  serr, as the calculation's open falls back only with one (in
  `swe_calc_ut()` and the other UT functions the two are the same
  buffer). The file stays open for the calculation, which opens nothing
  then. The guess stays Swiss; nothing is published (F4).
- The fallback's warning, which the calculation's own open wrote, is
  written into the delta t's serr and kept for the calculation that finds
  the file open (`jpl_open_note`, emptied by every open), which writes it
  where its open would have (`jpl_open_note_say()` in `swecalc()` and
  `jplplan()`): `swe_calc()` clears serr at its start, so the first
  `swe_calc_ut()` would otherwise have lost it.
- Where the file is not found the delta t answers as before, the default
  term, without the open's error in its serr, and does not try again
  until a file opens or a setter, `swe_close()` or an adopted path or
  file changes what would be found (`jpl_first_missed`); the calculation
  retries its own open at every call, as upstream's does. A default name
  tried with no serr has not tried de406.eph, so that miss is not kept.
- The opens, `fopen()` interposed, before and after: the path whose
  de431.eph is de200.eph and two UT JPL Moons, 1 call and 1 open both;
  100 more, none both; 100 rounds of a path change and a JPL Moon, 200
  calls and 100 opens both; with a Swiss Moon, 300 calls and no open both;
  the de406.eph fallback and two Moons, 2 calls and 1 open both; no JPL
  file anywhere, the fallback allowed, two UT JPL Moons 6 .eph calls
  before and 8 after (the first delta t's try of de431.eph and de406.eph)
  and 100 more 300 both. The resolver is on the hot path, the new branch
  only for SEFLG_JPLEPH: calc-swieph 12266746003 instructions before and
  12266746129 after, calc-moseph 23333308548 and 23333308874.

G132 (`tests/jpldenum.c`, entry 137) gains three settings with no JPL file
set: the path a directory whose de431.eph is de200.eph and no last
setter, or the path again by another string, or the first path a
directory with no JPL file (its readers asking a JPLEPH delta t too) and
the last setter the de431.eph directory, each against a fresh process
given the final path alone: 10 settings x 5 readers x 5 observations and
one check, 251 pass. Before the fix 18 fail: the first Moon on all 5
readers in each setting (15), and with no last setter the JPLEPH delta t
3.012003 s for the fresh process's 14.286718 on the 3 readers that
computed a JPL Sun first. The compat build fails 137 of 251 with the fix
and without: entry 164's 85 of 176, and in the three settings 52 by the
upstream behaviour it restores (the first Moon, 15; the JPLEPH delta t,
7; the getter and `swe_deltat()` after a JPL Sun following the open file,
bugs 103 and 138, 15; the flag-0 delta t, bug 145's, 15). The fallback's
warning does not fit G132's bit-for-bit serr (a reader that opened the
file before reports nothing), so G24 takes it: `jpl_dft_moon_off`, in a
forked child called first, the first UT JPL Moon in AD 1700 against the
second along the de431.eph-is-de200 path, and after `swe_close()` along a
path without de431.eph whose de406.eph is, the larger, 1 if the first
call's serr lacks the fallback's warning: 0 shipped, 1.79e-3 deg compat
(and shipped before the fix); with the warning not carried to the
calculation, 1.

## 166. Without the moon files the Swiss files' tidal term is the default, not the planet files' DE number's: delta t 11.29 s off in AD 1000, and every answer that keeps SEFLG_SWIEPH with it

**Severity: Correctness (residual above the floor: delta t 0.037 s off at 1900 and 11.29 s in AD 1000; the heliocentric Mars 2.6e-7 deg off at 1900 and 5.8e-5 in AD 1000, the mean apogee 4.9e-8 at 1900, with the fallback the Sun 4.4e-7 and 1.3e-4 -- on every thread and context)**
**Where:** upstream `swephlib.c` `swi_get_tid_acc()` (SEFLG_SWIEPH's DE number from the moon file alone); here `sweph.c`: `ephe_path_read_moon()`, new `ephe_path_read_planet()`, `ctx_release()`; `swephlib.c`: `swi_get_tid_acc()`; `sweconfig.c`: `swi_config_capture()`, `swi_config_apply()`, `cfg_merge()`, `swi_config_reset()`; `sweph.h`, `sweconfig.h`

From PR #33's list of defects found and not fixed. The Swiss files' term
is the moon file's DE number: the open moon file's, else the one the path
setter read from semo_18.se1's header and published (bug 101's
`sweph_denum_moon`). With no moon file along the path there is neither,
and the term was the default, `SE_TIDAL_DEFAULT`, -25.80, DE431's. The
planet files there are DE441's: sepl_18.se1's header, like semo_18.se1's,
names DE441, `SE_TIDAL_DE441` -25.936 (the finder's figure; -25.80 is
DE431's, the default's). Measured with a /tmp directory of links to
EPHEDIR's files but semo*.se1, against EPHEDIR: `swe_get_tid_acc()`
-25.80 for -25.936; `swe_deltat_ex()` with SEFLG_SWIEPH in AD 1000
1463.430567 s for 1474.717432, at 1900 -1.990764 for -1.953326 (0.037 s);
under the strict setting, which refuses what depends on Moshier's Moon
(bug 120), `swe_calc_ut()`'s heliocentric Mars at 1900 286.446337609530
deg for 286.446337869546 (2.6e-7) and in AD 1000 110.447311102471 for
110.447369542638 (5.8e-5); with the fallback allowed `swe_nod_aps_ut()`'s
mean node and apogee at 1900 259.161307172549 and 154.332767121104 for
259.161307149613 and 154.332767169674 (2.3e-8, 4.9e-8: the finder's 5.0e-8
is the apogee's), the Sun 280.153361019726 for 280.153361460308 (4.4e-7)
and in AD 1000 1.3e-4 deg off. The same on a thread, a context and a first
use by `SE_EPHE_PATH`, and in the compat build, upstream's.

The term, on its science. The tidal term in delta t is the Moon's secular
acceleration, n-dot, of the lunar ephemeris: delta t before the
telescopic era is reconstructed from eclipse and occultation timings
reduced with a lunar theory of one n-dot, and `adjust_for_tidacc()`
rescales it to the n-dot of the ephemeris in use (by -0.000091 (n-dot -
n-dot0) (Y - 1955)^2 s), so that this ephemeris reproduces the observed
events. The n-dot belongs to the whole DE integration the files were made
from -- the Moon's orbit was integrated with it, and the planets' with
that Moon -- and the planet files carry its DE number in their header as
the moon files do. Without the moon files:
- the Moon is Moshier's, a theory fitted to DE404 (swemmoon.c), whose term
  is `SE_TIDAL_DE404`, -25.58 (`SE_TIDAL_MOSEPH`). Since bug 120 it answers
  SEFLG_MOSEPH, and `swe_calc_ut()` takes Moshier's delta t for it: its
  term is its own theory's already;
- what keeps SEFLG_SWIEPH comes from the planet files -- the planets, the
  Sun, the Earth but for its 1/82 lunar offset (bug 120's 3.4e-8 deg) --
  or from no file at all (the mean elements): the delta t the complete
  directory gives them is the planet files' DE441 term, and nothing in
  them is fitted to any other;
- -25.80 is the term of no file present, the default for want of one.
So the term is the planet files' DE number's. It is a defect, above the
residual floor (5.0e-8 deg at 1900, 5.8e-5 in AD 1000).

**Fix, as done here** (under `SWE_UPSTREAM_COMPAT`'s `#ifndef`; the compat
build keeps upstream's default term):
- `ephe_path_read_moon()`, when the moon file's header is not found at
  J2000, reads the planet file's there (`ephe_path_read_planet()`:
  sepl_18.se1 opened and its header read as the moon file's is, kept open
  for the calculations with the span it answers for, bug 89's). Its DE
  number is published with the path (`sweph_denum_planet`), captured,
  merged, compared and adopted with the moon file's and owed with it after
  `swe_close()`, so the setters, bug 107's first use, bug 131's reading
  again and a thread or context taking the path all carry it; 0 when the
  moon file's was read. The same path again with its planet file open
  keeps the number read with it.
- `swi_get_tid_acc()` takes it for SEFLG_SWIEPH after the open moon file
  and the moon file's published number.
- After the fix the directory without the moon files answers delta t, the
  getter, the heliocentric Mars and the mean points bit for bit as
  EPHEDIR; the Sun with the fallback 1.55e-9 deg off it at 1900 and
  8.7e-9 in AD 1000 (the Earth's Moshier lunar offset, bug 120's).
- The opens, `fopen()` interposed, 100 rounds after a warm-up, before and
  after: a path change and the heliocentric Mars at 1900 each round, 400
  calls and 100 opens of sepl_18.se1 both (the setter's open replaces the
  calculation's); a path change and a Moshier Mars, 100 calls and no open
  before, 200 and 100 after, as EPHEDIR's setter opens semo_18.se1 for its
  header (100 and 100); the same path again and the heliocentric Mars, 100
  calls (bug 118's look for a moon file installed since) and no open both.
  EPHEDIR's counts unchanged. Instructions: calc-swieph 12266746129 before
  and 12266751448 after, calc-moseph 23333308874 and 23333313678.

G142 `tests/moonlesstid.c`, each case in a forked process: the directory
without the moon files read by the setting thread, a thread started after
the path, a thread running across it, a context made after it, an
explicit context, a first use by `SE_EPHE_PATH` and the setting thread
after `swe_close()`; under the strict setting the getter, delta t in AD
1000 and 1900, `swe_deltat()`, the heliocentric Mars at 1900 and in AD 1000
and the getter again, and with the fallback allowed the mean node and
apogee at 1900 and in AD 1000 and the Moon's return flag (SEFLG_MOSEPH),
each bit for bit as the same reader given EPHEDIR: 14 pass. Before the fix
the 14 fail; the compat build fails the 14 with the fix and without, the
strict 7 by upstream's default term and the fallback's 7 by bug 120's
SEFLG_SWIEPH on Moshier's Moon, which it restores. G24 `moonless_tid_off`,
delta t in AD 1000 along that directory against EPHEDIR, each on a context
made and given its path: 0 shipped, 11.29 s compat (and shipped before the
fix).

Noticed, not fixed: `swe_nod_aps_ut()` of the Moon with SE_NODBIT_MEAN and
SEFLG_SWIEPH, geocentric, along a directory without the moon files is
refused under the strict setting ("Swiss (.se1) Moon file is not
available"), though with the fallback allowed its four points and their
speeds at 1900, AD 1000 and J2000 equal EPHEDIR's bit for bit -- an answer
that does not depend on Moshier's Moon refused (bug 120's rule). And with
both J2000 files missing, semo_18.se1 and sepl_18.se1, and another
epoch's there, the path setter reads no DE number and the first delta t
takes the default term: in AD 1500 291.693033 s, then 294.255183 after a
Swiss Moon there opened semo_12.se1, the first Moon 158.4001317882 deg for
EPHEDIR's 158.4005084277 (3.8e-4 deg); before this entry too.

## 167. `swe_nod_aps()` on Moshier sees the Sun's geocentric points from the Earth

**Severity: Correctness (behaviour: a wrong answer, returned OK; the Sun's mean perigee 1.5 deg and 1.02 AU off, its apogee 88.5 deg and 0.96 AU), both builds**
**Where:** `swecl.c`: `swe_nod_aps_r_body()`, and through it `swe_nod_aps_ut()`

Found by entry 149's work. The Sun's geocentric nodes and apsides are the
Earth's orbit (`ipli` the Earth) seen from the Sun and negated: the
points, built heliocentric, are moved to the barycentre by adding the
barycentric Sun, the observer is the barycentric Sun, and the difference,
negated, is the Sun's place at its perigee seen from the Earth. On
Moshier, which serves no barycentric Sun, the points stay heliocentric
(`!(iflag & SEFLG_MOSEPH)` skips the addition), and the same test sent the
Sun to the planets' branch: the observer was the Earth, the point the
vector from the Earth to the Earth's own perihelion, negated. At 2000 Jul
1.5:

| the Sun, geocentric | Moshier | the files' |
|---|---|---|
| mean perigee | 281.4259 deg, 1.9993 AU | 282.9417 deg, 0.9833 AU |
| mean apogee | 191.4583 deg, 0.0531 AU | 102.9417 deg, 1.0167 AU |
| osculating perigee | 281.4572 deg, 1.9993 AU | 283.0042 deg, 0.9833 AU |

Every method alike (0, mean, osculating, SE_NODBIT_OSCU_BAR, the focal
point), SEFLG_TOPOCTR with the site added (the speeds then 1.3e-2 deg/day
off their positions' rate). Heliocentric and barycentric output, and the
Earth's points, are not the planets' branch and were right. Both builds.

**Fix, as done here:**
- On Moshier the observer of the Sun's geocentric points is the Sun, its
  origin (zero), as it is the barycentric Sun on the files; with
  SEFLG_SPEED the same for the observer at t - dt the aberration's rate
  takes.
- Now the Sun's points are the Earth's heliocentric ones of the same
  ephemeris and method at their own distance (4.4e-16 AU) in the opposite
  direction (7.6e-7 deg, the light deflection, which takes the distance),
  on Moshier as on the files; Moshier's mean points are the files' within
  2.9e-6 deg and 2.2e-16 AU (the same elements; the files' observer moves
  with the Sun's barycentric velocity, an aberration Moshier's lacks), its
  osculating points as close as the Earth's (1.2e-3 deg at 2000, 4.8e-2 at
  -3000).
- Behind `#ifndef SWE_UPSTREAM_COMPAT`: upstream answers so. No hot path
  (`swe_nod_aps()` is not in the bench).

G143 `tests/nodcentre.c` (new): (a) the Swiss files and Moshier, seven
instants -3000..3000, the six methods, geocentric and topocentric: the
Sun's perigee and apogee against the Earth's heliocentric points, distance
within 1e-12 AU, direction within 1e-5 deg; Moshier's mean points against
the files' within 1e-5 deg and 1e-12 AU; their speeds against five-point
differences over 2^-6 day at binary instants within 1e-10 deg/day and
1e-12 AU/day (measured 2.9e-12, 2.9e-14). 357 rows pass; before the fix
189 fail (the 168 Moshier rows of the first check, 143 deg and 0.97 AU,
the second's 14 and the topocentric 7 of the third), the upstream build
273. G133 (h) no longer skips the Sun's geocentric points: the fallback
answers them as the Moshier request does, bit for bit (8 rows; before the
fix they fail, the Moshier request from the Earth and the fallback, its
flag Swiss, from the Sun). 1080 rows pass; the compat build fails 296.

G24 `nodaps_mosh_sun_off`: the Sun's mean perigee on Moshier at 2000 Jul
1.5, its distance less the Earth's heliocentric perihelion distance: 0
shipped, 1.02 AU compat.

## 168. `swe_nod_aps()` takes its frame and site from the caller's ephemeris bits, not from the ephemeris that answered

**Severity: Correctness (behaviour: a wrong answer, returned OK, that depends on call order or on a redundant flag bit; Mars's node 0.13 deg off on a warm fallback, 0.108 deg under SEFLG_SWIEPH | SEFLG_MOSEPH), both builds**
**Where:** `swecl.c`: `swe_nod_aps_r_body()`, and through it `swe_nod_aps_ut()`

Found by entry 149's work. `swe_nod_aps()` builds its points
heliocentric and moves them to the centre asked with the save area's Sun
and Earth: on the files, it adds the barycentric Sun and takes the
barycentric Sun or Earth for the observer; on Moshier, whose origin is the
Sun, it adds nothing and the heliocentric observer is the origin. It chose
between the two by the caller's SEFLG_MOSEPH, and placed a topocentric
site by the delta t of the caller's ephemeris bits, where `swe_calc()`
resolves the bits (JPL, then Swiss, then Moshier) and, after a fallback,
goes on with the ephemeris that answered:
- A Swiss or JPL request fallen to Moshier, the fallback allowed, added
  the save area's Sun to Moshier's heliocentric points and took it for
  the heliocentric observer: zero in a fresh process (the answer was
  Moshier's), another call's Swiss Sun in a warm one. The files of
  1800-2400 alone, after Mars's points at 2000 Jul 1.5, Mars's geocentric
  osculating node at 1500 Jan 1 came out 4.3236 deg, latitude 9.1e-3, for
  Moshier's 4.4536 and 1.4e-4 (0.13 deg); the Moon's heliocentric mean
  node 99.7972 deg for 100.1993 (0.40). The site took the files' delta t
  for Moshier's places, fresh or warm: the Moon's topocentric mean perigee
  there 2.9e-4 deg off Moshier's, Mars's 3.3e-7.
- SEFLG_SWIEPH | SEFLG_MOSEPH, answered from the files, took Moshier's
  frame: Mars's geocentric osculating node at 2000 Jul 1.5 69.7303 deg for
  the SEFLG_SWIEPH request's 69.8382, the barycentric node the
  heliocentric 49.5625 for 49.6398, the heliocentric points without the
  aberration the files' carry (3e-6 deg). And the bits went on, unresolved,
  to `swi_plan_for_osc_elem()`, whose `swi_get_denum()` reads SEFLG_MOSEPH
  first: SEFLG_JPLEPH | SEFLG_MOSEPH on de200, answered from DE200, turned
  the osculating ellipse by the frame bias of DE >= 403, Mars's node 1.5e-4
  deg off the SEFLG_JPLEPH request's (1.6e-4 heliocentric).
Both builds.

**Fix, as done here:**
- One ephemeris bit at the top, by `plaus_iflag()`'s precedence, as
  `swe_calc()` resolves them; no bit stays no bit.
- After the call that sets the save area's Sun and Earth, the frame (the
  barycentric Sun added, the heliocentric observer, the Sun's observer of
  entry 167) and the topocentric site's delta t (and the site's
  acceleration in the aberration's rate) are those of the ephemeris of the
  save area's Earth, the one that answered. A Moon from Moshier on the
  files' Earth, the Moon files alone missing (SEFLG_MOSEPH in its flag,
  entry 120), keeps the files' frame and site, as `swe_calc()` computes it.
- Now the mixed bits answer as the ephemeris they resolve to, return and
  points bit for bit; the warm fallback as the Moshier request in a fresh
  process, bit for bit under SEFLG_NOGDEFL, geocentric, topocentric and
  heliocentric. With the deflection Mars's points stay 3.0e-8 deg off:
  `deflect_light_from()` (sweph.c) takes the save area's barycentric Sun on
  Moshier too, the stale one here, as `swe_calc()`'s own Mars on the same
  warm fallback (1.7e-9 deg); not this entry's.
- Behind `#ifndef SWE_UPSTREAM_COMPAT`: upstream answers so. No hot path.

G143 `tests/nodcentre.c` (b): SEFLG_SWIEPH | SEFLG_MOSEPH against
SEFLG_SWIEPH, and SEFLG_JPLEPH with SEFLG_MOSEPH, SEFLG_SWIEPH or both
against SEFLG_JPLEPH (de200), five bodies, three methods, four centres, two
instants, the return and the points bit for bit: 480 rows pass; before the
fix 340 fail (12 deg at most), the upstream build all 340 too (613 of
G143's 837). G133 (f), the files of 1800-2400, warm after the call at the
instant: four calls at 1500 against the Moshier request in a fresh
process, Mars geocentric and topocentric under SEFLG_NOGDEFL and the
Moon's heliocentric mean points bit for bit, Mars geocentric within 1e-7
deg; 8 rows, failing before the fix and in the upstream build. 1088 pass
(the compat build fails 304).

G24 `nodaps_mixed_eph_off`: Mars's geocentric osculating node at 2000 Jul
1.5, SEFLG_SWIEPH | SEFLG_MOSEPH against SEFLG_SWIEPH: 0 shipped, 0.108
deg compat.

## 169. `swe_nod_aps()` reads the centre bits otherwise than `swe_calc()`

**Severity: Correctness (behaviour: a centre other than `swe_calc()`'s for the same flags, or none, returned OK; Mars's node 0.077 deg off the barycentric one under SEFLG_HELCTR | SEFLG_BARYCTR), both builds**
**Where:** `swecl.c`: `swe_nod_aps_r_body()`, and through it `swe_nod_aps_ut()`

Found by entry 149's work. `plaus_iflag()` gives `swe_calc()` one centre
for any combination of the bits: SEFLG_TOPOCTR turns SEFLG_HELCTR and
SEFLG_BARYCTR off, SEFLG_BARYCTR SEFLG_HELCTR. `swe_nod_aps()` documents
the centres "like the planetary function swe_calc()" but tested the raw
bits: the observer was the barycentric Sun whenever SEFLG_HELCTR was set,
nothing for SEFLG_BARYCTR without it, the site (its geocentric offset)
whenever SEFLG_TOPOCTR was set, and the Earth only without SEFLG_HELCTR and
SEFLG_BARYCTR. At 2000 Jul 1.5, Mars's osculating ascending node on the
files:

| flags | `swe_nod_aps()` | `swe_calc()`'s centre |
|---|---|---|
| SEFLG_HELCTR \| SEFLG_BARYCTR | 49.5625 deg, heliocentric | barycentric: 49.6398 |
| SEFLG_TOPOCTR \| SEFLG_HELCTR | 49.5625, heliocentric | topocentric: 69.8377 |
| SEFLG_TOPOCTR \| SEFLG_BARYCTR | 49.6387, the barycentric points less the site's offset, no centre's | topocentric: 69.8377 |

On Moshier the first answered its heliocentric points under a barycentric
request (`swe_calc()` refuses it, entry 149), and SEFLG_TOPOCTR |
SEFLG_BARYCTR was refused as barycentric (entry 149) where `swe_calc()`
answers it topocentric. For the Moon, which takes no aberration
geocentric, SEFLG_TOPOCTR | SEFLG_HELCTR took it as heliocentric. Every
body and method alike; the UT form too. Both builds.

**Fix, as done here:**
- The centre bits as `plaus_iflag()` leaves them, at the top, so every
  test below (the observer, the change of centre, the Moon's aberration,
  entry 149's barycentric refusal) reads one centre: SEFLG_HELCTR |
  SEFLG_BARYCTR is barycentric (on Moshier refused, as SEFLG_BARYCTR is),
  SEFLG_TOPOCTR with either or both topocentric.
- Now each combination answers as the centre `swe_calc()` takes, return,
  message and points bit for bit, on the files, de200 and Moshier.
- Behind `#ifndef SWE_UPSTREAM_COMPAT`: upstream answers so. No hot path.

G143 `tests/nodcentre.c` (c): SEFLG_HELCTR | SEFLG_BARYCTR against
SEFLG_BARYCTR, SEFLG_TOPOCTR with SEFLG_BARYCTR, SEFLG_HELCTR or both
against SEFLG_TOPOCTR, the Swiss files, Moshier and de200, five bodies,
three methods, two instants, `swe_nod_aps()` and (the Moon and Mars)
`swe_nod_aps_ut()`: 456 rows pass; before the fix 448 fail (the 8 that
pass are Moshier's Saturn ellipse, refused either way), and the upstream
build. G143 1293 rows pass; the upstream build fails 1025. G133 had
SEFLG_TOPOCTR | SEFLG_BARYCTR among its barycentric requests in (g) and
SEFLG_HELCTR | SEFLG_BARYCTR among its heliocentric ones in (h): the first
moves to (h), answered by the fallback as by the Moshier request bit for
bit, and Moshier now refuses the second as barycentric (60 rows that fail
before the fix). Its `ask()` now marks a call as answered, which the warm
checks read unset from the stack (they passed at -O1 and failed 8 at -O0
once the file changed). 1120 rows pass; the upstream build fails 390.

G24 `nodaps_helbary_off`: Mars's osculating node at 2000 Jul 1.5,
SEFLG_HELCTR | SEFLG_BARYCTR against SEFLG_BARYCTR: 0 shipped, 0.077 deg
compat.

## 159. SEFLG_JPLHOR's precession and obliquity step at 1799 Jan 1 and 2202 Jan 1, IAU 1976 within and Owen 1990 outside, and the frame's rates difference them across: for a day either side every speed of date 2.8e-7 to 1.0e-6 deg/day off, latitudes 5.0e-6 the day before, SEFLG_SPEED3 8.9e-3

**Severity: Correctness (under SEFLG_JPLHOR within a day of 1799 Jan 1 or 2202 Jan 1, and with SEFLG_JPLHOR_APPROX under `SEMOD_JPLHORA_3` within a day of 1799 Jan 1: every speed of date off by the precession's difference, 2.8e-7 deg/day at 1799 and 1.0e-6 at 2202 in its own rate, by the obliquity's on the day before, the Sun's latitude speed 5.0e-6, Polaris's right ascension 5.6e-6; SEFLG_SPEED3 8.9e-3 within 1e-4 day)**
**Where:** `swephlib.c`: `swi_frame_step_side()`, which `sweph.c`'s `swi_precess_speed()`, `swi_obliquity_rate()`, `swi_get_observer()` and `swe_calc()`'s SEFLG_SPEED3 consult

Entry 153's noticed item. Under SEFLG_JPLHOR, and with
SEFLG_JPLHOR_APPROX under `SEMOD_JPLHORA_3` up to 1962 Jan 20,
`swi_precess()` and `swi_epsiln()` take IAU 1976 for 2378131.5 < J <
2525323.5 (strictly between 1799 Jan 1 and 2202 Jan 1) and Owen 1990
outside, so each boundary's own instant is Owen's: at 1799 the left
side's model, at 2202 the right side's. Measured between the instant and 2^-20
day either side, less the motion: the mean obliquity 17.57 mas at 1799 and
19.75 mas at 2202, the precession matrix 6.45 and 7.35 mas. The rates
that difference the two models reached across them as they did across
1962 (entry 153), but `swi_frame_step_side()` told them of the 1962 step
alone, and of none under SEFLG_JPLHOR:
- the precession's, entry 49's (P(t + 1) - P(t - 1)) / 2, which both
  models take (neither is Vondrak's): within a day either side its rate
  2.82e-7 deg/day off at 1799 and 1.02e-6 at 2202 (`swi_precess_speed()`
  of a unit vector of date against five-point differences of
  `swi_precess()`); the site's precession in right ascension the same;
- the mean obliquity's, eps(t + 1) - eps(t), for t in the day before:
  4.88e-6 and 5.49e-6 deg/day;
- SEFLG_SPEED3's three places within 1e-4 day either side.
The nutation has no step there (IAU 1980 throughout). On de200 with
SEFLG_JPLHOR_APPROX at 1799 Jan 1 (2202 is beyond de200, and SEFLG_JPLHOR
needs a DE 403 file and the IERS corrections, neither here), against
five-point differences of the same call's positions over 2^-6 day on the
instant's side (G139's), the worst over the day before and the day after:
the Sun's ecliptic speeds 5.02e-6 deg/day (its latitude, from the
obliquity) and 5.78e-7 equatorial, the Moon 1.96e-6, Mars 1.05e-6,
Polaris 4.81e-6 in the ecliptic and 5.60e-6 in right ascension, the
topocentric Moon 1.95e-6; SEFLG_SPEED3 on Mars 6.04e-3 in the ecliptic and
8.85e-3 in right ascension; sidereal Mars, which has no Horizons flags,
1.6e-11.

**Fix, as done here:** `swi_frame_step_side(ctx, t, h, iflag, which)`
compares the model each function of `which` (`SWI_FRAME_PREC`, `_EPS`,
`_NUT`) takes at t with the one it takes at t + h and at t - h, chosen as
the function itself chooses it (`frame_prec_model()`, `frame_eps_model()`,
`frame_nut_model()`, SEFLG_JPLHOR's models by `frame_is_jplhor()`):
-1 when t + h's differs (difference backward), +1 when t - h's does
(forward), 0 when neither. So every step of the three, with its own
inclusive side, is found where the model changes, and each rate asks for
its own model: the precession's fallback and the site's for the
precession, the obliquity's for the obliquity, the nutation's for the
nutation (no step at 1799 and 2202), SEFLG_SPEED3 for all three. Entry
153's one-sided differences then take each side's rate as at 1962. Behind
`#ifndef SWE_UPSTREAM_COMPAT`, as entry 153's. The test returns 0 at once
unless the flags take SEFLG_JPLHOR's models somewhere (SEFLG_JPLHOR, or
SEFLG_JPLHOR_APPROX under mode 3); the default models and flags read one
model number and two flag bits, as before.
- After, on de200 at 1799: the Sun within 3.6e-11 deg/day, Mars 3.1e-11,
  Polaris 3.1e-11 in the ecliptic and 7.5e-10 in right ascension (as
  elsewhere), the Moon 2.0e-9 (the one-sided reference's truncation over
  2^-6; 2.8e-10 away from the step), the topocentric Moon 6.8e-10;
  SEFLG_SPEED3 1.0e-6, its own everywhere. Under SEFLG_JPLHOR the
  precession's rate within 3.7e-12 deg/day of the five-point difference
  at 1799 and 2202, the obliquity's within 2.4e-12.

G139 `tests/framestep.c`, extended: check (a) takes de200 with
SEFLG_JPLHOR_APPROX and the default models at 1799 Jan 1 too, the same
seven bodies and two frames over the day before, the day after and the
rest, 42 checks; check (b) takes the rates themselves under SEFLG_JPLHOR
(`swi_precess_speed()`'s and `swi_obliquity_rate()`'s against five-point
differences of `swi_precess()` and `swi_epsiln()` on the instant's side)
at 1799 and 2202, 12 checks, in the fork's build only. 138 checks; before
the fix 30 fail: at 1799 every body but sidereal Mars on both days, 24 of
42, and in (b) the precession's on both days and the obliquity's the day
before at both steps, 6 of 12. The upstream build fails 120 of (a)'s 126,
its speeds of date off throughout by the defects earlier entries fixed
(the Sun's 1e-5 deg/day, the ecliptic's turning, entry 33); its SPEED3
2.65e-2 the day before 1799 Jan 1. G24 `jplhor_1799_speed_off`: the Sun's
ecliptic speeds at 1799 Jan 1 - 0.5 day and -+ 2^-14 day on de200 with
SEFLG_JPLHOR_APPROX against the five-point difference on its side, the
largest: 1.67e-11 deg/day shipped (5.02e-6 before the fix), 9.98e-6
compat.

Noticed, not fixed:
- RESIDUAL. Owen 1990's own fits are pieces 80 centuries long, joined at
  the midpoints between their centres (`get_owen_t0_icof()`: JD
  -1931455.5, 990544.5, 3912544.5, 6834544.5, about -10000, -2000, +6000
  and +14000), where the model steps: the mean obliquity by 0.095 mas at
  -10000, the precession by 0.085 mas at +6000 and 0.065 mas at +14000.
  The same rates reach across them, under SEFLG_JPLHOR (Owen outside
  1799-2202) and with Owen 1990 chosen: the obliquity's 2.6e-8 deg/day the
  day before -10000, the precession's 9.8e-9 and 1.1e-8 either side of
  +6000 and +14000; SEFLG_SPEED3 some 1.2e-4 within 1e-4 day.

## 160. A short-term precession model the caller chooses steps to the long-term one at its window's edges, and the frame's rates difference the two across: with IAU 1976 at J2000 -+ 2 centuries every speed of date 8.0e-5 deg/day off within a day inside the window, SEFLG_SPEED3 0.80; with IAU 2006 at -+ 75 centuries 1.5e-2

**Severity: Correctness (with `SE_MODEL_PREC_SHORTTERM` set to IAU 1976, 2000 or 2006 by `swe_set_astro_models()` and a different long-term model: every speed of date off within a day of the window's edges, by the precession's difference inside the window, 6.1e-5 deg/day in its own rate with IAU 1976 and Vondrak, 1.5e-2 with IAU 2006 at 75 centuries; latitudes by the obliquity's the day before each edge, 9.3e-7 to 1.9e-2; SEFLG_SPEED3 0.80 to 0.86 within 1e-4 day)**
**Where:** `swephlib.c`: `swi_frame_step_side()`'s models (`frame_chosen_model()`), which `sweph.c`'s `swi_precess_speed()`, `swi_obliquity_rate()`, `swi_get_observer()` and `swe_calc()`'s SEFLG_SPEED3 consult

Entry 153's noticed item. `swi_precess()` and `swi_epsiln()` take a
short-term model within its window -- IAU 1976 and IAU 2000 for |T| <=
`PREC_IAU_1976_CTIES`, `PREC_IAU_2000_CTIES` = 2 centuries of J2000, IAU
2006 within `PREC_IAU_2006_CTIES` = 75, the edge's own instant inside --
and the long-term model outside (a long-term IAU 1976 or IAU 2000 wins
over a later short-term model, which then never holds). Measured between
the instant and 2^-20 day either side, less the motion: with IAU 1976
short and the default Vondrak long ("0,9,1") the precession matrix steps
by 529 mas at J2000 - 2 centuries (JD 2378495) and 572 mas at + 2
(2524595), the mean obliquity by 3.36 and 81.57 mas; with IAU 2006 short
("0,9,8") at -+ 75 centuries (JD -287830, 5190920) the precession by
42.3" and 107.4", the obliquity by 68.5" and 53.4". The rates entry 153
made keep to one side were told of no such step:
- the precession's, entry 49's (P(t + 1) - P(t - 1)) / 2, taken where
  the model is not Vondrak's: inside the window, and outside too when the
  long-term model is another (IAU 2006 long, IAU 1976 short, "0,8,1",
  both sides); within a day of the edge its rate (`swi_precess_speed()`
  of a unit vector of date against five-point differences of
  `swi_precess()` on the instant's side) 6.1e-5 deg/day off at - 2 and
  6.9e-5 at + 2 with "0,9,1", 2.6e-6 with IAU 2000 ("0,9,6"), 3.9e-3 and
  1.5e-2 at -+ 75 with "0,9,8"; the site's precession in right ascension
  the same;
- the obliquity's, eps(t + 1) - eps(t), the day before each edge: 9.3e-7
  and 2.3e-5 deg/day with "0,9,1", 1.4e-6 and 3.0e-6 with "0,9,6", 1.9e-2
  and 1.5e-2 with "0,9,8";
- SEFLG_SPEED3's three places within 1e-4 day either side.
The nutation does not step there. With SEFLG_JPLHOR_APPROX under
`SEMOD_JPLHORA_3` the window holds after 1962 (+ 2 centuries) and the
same rates reached across it; before 1962 SEFLG_JPLHOR's models hold
(entry 159), so - 2 centuries is not a step there. On de200 with "0,9,1"
at J2000 - 2 centuries (sidereal time the IERS 2010 formula's, see
below), against five-point differences of the positions over 2^-6 day on
the instant's side: within the day after the edge (inside) the Sun's
speeds 8.73e-5 deg/day off, the Moon's 8.04e-5, Mars's 8.27e-5,
Polaris's 7.80e-5 in the ecliptic and 7.11e-5 in right ascension, the
topocentric Moon's 8.04e-5, sidereal Mars's 8.27e-5 (its ayanamsa's rate,
which precesses with no flags, too); within the day before, the ecliptic
latitudes 6.5e-7 to 9.3e-7; SEFLG_SPEED3 0.799 and 0.826 either side. On
the Swiss files at + 2 centuries the same, the planets' and the Moon's
8.4e-5 to 9.4e-5 within the day before (inside), Polaris's right
ascension 2.24e-3, SEFLG_SPEED3 0.864.

**Fix, as done here:** `swi_frame_step_side()` (entry 159) takes the
chosen precession and obliquity as `swi_precess()` and `swi_epsiln()`
choose them (`frame_chosen_model()`, in their order of tests: the short-
term model within its window, a long-term IAU 1976 or 2000 first,
SEMOD_JPLHORA_2's obliquity IAU 1976 throughout), so each window's edges
are steps with the window's inclusive side, and nothing where the two
models are one (IAU 1976 within IAU 1976, or IAU 2000 short under IAU
1976 long). The test reaches the models only when the short-term model is
IAU 1976, 2000 or 2006 (or the flags take SEFLG_JPLHOR's models); the
default models and flags return 0 after reading two model numbers and two
flag bits. Behind `#ifndef SWE_UPSTREAM_COMPAT`, as entry 153's.
- After, on de200 at J2000 - 2 centuries: the Sun within 1.8e-11 deg/day,
  Mars 1.6e-11, Polaris 6.9e-11 in right ascension, the Moon 2.4e-9 (the
  one-sided reference's truncation; 3.7e-10 away from the edge), the
  topocentric Moon 6.1e-10, sidereal Mars 2.6e-11; SEFLG_SPEED3 1.24e-6,
  its own; on the Swiss files at + 2 centuries likewise (the Moon 9.0e-9,
  the one-sided reference on the files' Moon, 1.7e-9 centred). The rates
  themselves within 8.5e-12 deg/day of the five-point differences at
  every edge and setting above.

G139 `tests/framestep.c`, extended: check (a) takes de200 with "0,9,1" at
J2000 - 2 centuries, no Horizons flag and the IERS 2010 sidereal time
(the long-term sidereal time reaches across the edge on its own, below),
the same seven bodies and two frames over the three spans, 42 checks;
check (b) the precession's and obliquity's rates at "0,9,1" -+ 2
centuries, "0,9,6" + 2, "0,9,8" -+ 75, "0,8,1" - 2 and "0,9,1" + 2 with
SEFLG_JPLHOR_APPROX, 42 checks. 222 checks; before the fix 37 fail: at
J2000 - 2 centuries 22 of 42 (every body the day after, the ecliptic
the day before, SPEED3 both), in (b) 15 of 42 (the precession's 2.6e-6
to 1.5e-2 deg/day, the obliquity's 4.5e-7 to 1.9e-2). The upstream build
fails 160 of (a)'s 168, at the window by its own speed defects and its
backward ayanamsa rate across the edge (sidereal Mars 0.160 deg/day the
day after), its SPEED3 2.48. G24 `prec_window_speed_off`: Mars's
ecliptic speeds on de200 with "0,9,1" at J2000 - 2 centuries -+ 0.5 day
and -+ 2^-14 day against the five-point difference on its side, the
largest: 1.16e-11 deg/day shipped (7.99e-5 before the fix), 1.57e-5
compat.

Noticed, not fixed (each found checking which rates see these steps; the
first five reach across the windows' edges by differences that do not
consult `swi_frame_step_side()`):
- The Moshier Moon's frame turning (entry 95's R(t -+ 1) x, of the
  ecliptic of date to the J2000 equator): with "0,9,1" at J2000 + 2
  centuries on Moshier the Moon's speed 8.7e-5 deg/day off within a day
  either side, the topocentric Moon's 8.8e-5.
- The JDATE fictitious bodies' frame turning (entry 48, `swi_osc_el_plan()`)
  and `swe_calc_orbel()`'s of SE_ORBEL_EQ_DATE (entry 136), the same
  difference over -+ 1 day: with "0,9,1" at J2000 - 2 centuries Selena's
  speed 8.0e-5 deg/day off the day before (inside, the light time moves the
  step by 3e-4 day), Vulcan's 1.3e-5; `swe_calc_orbel()` of an Earth orbit
  0.053 AU, its speed 8.0e-5 deg/day off the default models' within a day
  either side, 1.7e-14 beyond.
- The true node's and osculating apogee's speeds, five places of date over
  the Moon's epochs (entry 75): on de200 with "0,9,1" within 2^-7 day of
  J2000 - 2 centuries 2.9e-3 and 6.0e-3 deg/day off.
- The houses' speeds, from the ARMC and true obliquity HOUSE_SPEED_DT =
  1/64 day of UT either side (entry 42; the sidereal houses' frames and
  ayanamsa the same, entry 43): on de200 with "0,9,1" within 1/64 day of J2000
  - 2 centuries (in UT), against five-point differences of the cusps over
  2^-12 day, the MC's ecliptic speed 9.1e-4 deg/day off, the second cusp's
  8.8e-3, the Ascendant's 1.1e-2.
- BEHAVIOUR. The long-term sidereal time (entry 29, outside 1850-2050)
  integrates the precession in right ascension from the matrices' rate,
  a central difference over -+ 50 days (`sidt_pra_rate()`), at nodes 1
  century apart: the window's edges fall on nodes (-+ 2, -+ 75
  centuries), whose rate takes the step over 100 days, some 176"/cy, and
  the cubic Hermite carries it a century either side, and through the
  1850 splice's offset (`sidt_lterm_offset()`, at - 1.5 centuries) to
  every date before. With "0,9,1" the sidereal time against "0,9,9" is
  +24.0" before 1700, +50.5" at 1776, +16.7" at 1804 and -4.5" at 1831,
  -22.0" at 2215 (IAU 2000, "0,9,6": -1.9" at 1776), where the two
  models' rates in right ascension, some 0.3"/cy apart, account for under
  1" over the window. The topocentric Moon's speed takes it, 1.7e-8
  deg/day off a day or two from the edge (6e-10 with the IERS 2010
  sidereal time). G139's window takes the IERS 2010 sidereal time.
- BEHAVIOUR. SEFLG_JPLHOR_APPROX's offsets in the mean obliquity
  (`swi_epsiln()`, `dcor_eps_jpl[]`) and in right ascension
  (`swi_approx_jplhor()`, `dcor_ra_jpl[]`) interpolate their yearly table
  with the sign reversed, (t - t0) (d[t0] - d[t1]) + d[t0]: each year
  starts at d[t0] and ends at 2 d[t0] - d[t1], so they step by 2 (d[t1] -
  d[t0]) at every node, the obliquity by up to 0.44 mas (1972, JD
  2441499.0), and the obliquity's rate takes the step within a day before
  each (1.2e-7 deg/day). The obliquity's under the Vondrak model with
  SEMOD_JPLHORA_1, and with SEMOD_JPLHORA_3 after 1962; the right
  ascension's wherever `swi_approx_jplhor()` applies. Upstream's.

## How these were found

Almost none of this came from reading code looking for defects. The method
was:

1. **Measure coverage across every test binary, not just one.** Several
   files look far worse from a single run than they are, and — more
   importantly — the functions at genuine zero are where the defects were.
2. **Ask why a specific number is low, then build a test that reaches it.**
   Entries 1, 2, 4 and 5 were all in code that had never executed. Entry 4
   crashed on its first ever execution.
3. **Run the result under sanitizers.** ASan found entries 1, 2, 7 and the
   setest ones. Entry 3 is invisible to ASan — reading uninitialised stack is
   not an ASan finding — and needs MSan or an explicit initialiser.
4. **Test the invariants, not the expectations.** Entries 5, 6, 10 and 11 are
   all "the same question, asked twice, answered differently". No reference
   ephemeris can adjudicate that; asking the same question in two orders can.

Point 4 matters most for anyone reproducing this. Upstream's own `t.exp`
fixture cannot settle these, because it is a *recording of upstream's
output* — its header names the machine, the user and the date it was
generated. Agreeing with it means behaving like the version that produced
it, bugs included.

## Suggested order

If only some of this is worth taking:

| Priority | Entries | Why |
|---|---|---|
| First | 4 | a crash, reachable from ordinary setup calls |
| | 1, 2, 3, 7, 8, 9 | memory errors, several caller-triggerable |
| | 15, 16 | a failed or garbage answer from an ordinary call sequence |
| Then | 5, 6, 10, 11, 14 | wrong answers that depend on call order |
| | 21 | a sidereal node or apsis off by delta-psi |
| | 22 | a stale built-in star record (distance rate of Spica) |
| Last | 12, 13, 17 | test harness, utilities, undefined behaviour on hostile input |

Entries 5, 6 and 10 are one family — a cache whose key omits something its
value depends on. Fixing one does not address the others, and it is worth
auditing the remaining caches on the same question: *what does this value
depend on, and is all of it in the key?*

## Caveats

- Line numbers are upstream `3fd0f95`. The mechanisms are stable but the
  numbers will drift.
- Every entry was verified present in that tree by reading it, and every
  reproducer was run against a build of this fork with the fix reverted.
- The fixes shown are the shape that worked here, reduced to what upstream
  needs. They are not upstream patches — this fork carries an explicit
  context parameter that upstream has no equivalent for.
- Entries 5, 6, 10 and 11 change numerical output. That is the point of
  them, but it means they need a deliberate decision about regression
  fixtures rather than a silent adoption.
