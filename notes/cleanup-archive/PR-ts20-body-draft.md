# ts.20

**Status (2026-10-25):** the first round below (UPSTREAM-BUGS 23-51) is what this branch
shows on GitHub now. A second round, 52-122, is committed locally and is pushed after the
final pass; more (147-156) are in progress. The final pass (full `make check`, the
warning matrix, the G1/G10/G17 re-baselines with every moved row attributed, the bench)
runs once they land, and the Verified section is redone then.

Eighty-nine defects (UPSTREAM-BUGS 23-51 and 52-114), almost all upstream's, each fixed behind
`SWE_UPSTREAM_COMPAT` with a gate (G33-G111), a check-compat case asserted both ways and a
REVIEW row, plus defects of the fork's own: the ts.17 default nutation's cost (G32), and
those marked *fork* in the second round. Found by comparing this library with Ephemeris
Prometheia and JPL Horizons at the milliarcsecond level, by astromcp's test suite, and,
in the second round, by checking every speed against its own positions and every answer
against a fresh context's. `SE_VERSION` is `2.10.03-ts.20`.

First round:

| | before (ts.19) | after |
|---|---|---|
| nutation cost at daily / scattered instants, vs 2000B | 26.9x / 35.7x | 2.7x / 4.3x |
| astrometric Moon vs Prometheia, 1600-2600 | 1.340 mas | 0.011 mas |
| every planet, astrometric, vs Prometheia | <= 0.040 mas | <= 0.002 mas |
| heliocentric planets vs Horizons' Sun-centred places | up to 839 mas | <= 0.010 mas |
| topocentric observer | 15-277 m off | 0.0000 m (G37) |
| osculating apogee vs Prometheia, 1900-2100 | 21-163 mas | 0.06-1.0 mas |
| default sidereal time at -3000 / year 0, vs Earth rotation | +2382" / +328" | -10.1" / -0.29" (precession model) |
| topocentric Moon speed vs the rate of its own position | 1.3e-3 deg/day | 5.6e-7 |
| topocentric Sun and planet speeds | up to 3.1e-5 deg/day | <= 2e-7 |
| geocentric Moon speed | 3.6e-5 deg/day | 4.8e-7 |
| ecliptic latitude speeds of date, every body | 1.2-1.6e-5 deg/day | <= 5e-9 (Swiss files) |
| geocentric planet longitude speeds (Swiss files) | up to 3.3e-6 deg/day | <= 1.1e-7 |
| heliocentric Mercury speed | 1.6e-4 deg/day | 2.4e-7 |
| distance rates (Pluto, Neptune) | 7.4e-5, 5.1e-5 AU/day | 1.4e-8, 6e-9 |
| `swe_nod_aps()` speeds: osculating latitude, mean apsis latitude | 1.3e-5, 1.8 deg/day | <= 1.6e-7 |
| a planet's speed behind the Sun's disc, between instants 1e-9 day apart | 1.5e-5 deg/day | 2.9e-8 |
| house cusp speeds (whole sign, Koch, Placidus, Porphyry; 47-70 N) | up to 1600 deg/day | <= 5.8e-6 (G45) |
| interpolated perigee: position with vs without the speed flag / roughness | 111 mas / 1.3 mas | 0 / 0.0018 mas |
| `swe_nod_aps()` Mars points with vs without the speed flag | 6-35 mas | <= 2e-7 mas |
| `swe_nod_aps()` Moon's mean node under J2000, vs `swe_calc()` | 7.9" | 4.6e-9" |
| orbital-element bodies' speeds (Vulcan) | 2.9e-2 deg/day | 2.2e-8 |
| fixed star distance rate (Spica) | 3e-3 AU/day | 5e-7 |
| Moshier speeds vs the rate of its positions | 1e-6 to 2.7e-5 deg/day | 1e-8 to 4e-7 |
| Moshier osculating apogee between consecutive JDs at -3000 | 45" | 0.044 mas |

**Behaviour that users will see:** house cusps and the ascendant before 1850 move, by
-15.6" of sidereal time at 1300, -328" at year 1, -796" at -999 (UPSTREAM-BUGS 29). The
old default followed delta t at the Sun's mean motion; the new one is the Earth rotation
angle plus precession, as in the IERS Conventions. Also visible: `swe_houses_ex2()` cusp
speeds change, by hundreds of deg/day on whole-sign, Koch and Porphyry cusps (40-43; cusps
themselves do not move); `swe_nod_aps()` under `SEFLG_J2000` moves by up to 7.9" (148" in
latitude by 2326) to the J2000 frame (47); the Moshier osculating apogee and true node
move by up to 34" and 66 mas, their old noise (51). Everything else moves by
milliarcseconds, or by tenths of an arcsecond on heliocentric, topocentric and apogee
requests.

Second round, visible without looking past the third decimal: nakshatras under a
user-defined ayanamsa (97), sidereal Makransky houses (59), answers that depended on an
earlier call, a path change, a missing file or the thread (82-84, 88, 89, 92, 98, 100,
101), and Kronos without seorbel.txt (65). See "Second round" below.

## The default nutation is fast again at any spacing (G32)

ts.17 made full IAU 2000A the default and paid for it with a quarter-day grid: the
series only at nodes, cached, with a quintic Lagrange polynomial through the six
nodes around the instant. Its cost was measured on hourly Moon rows, where
consecutive instants share nodes. An instant a day or more from the previous one
shares none, so it evaluated the full series six times:

G32 at -O2, Moshier Sun with speed, cost relative to SEMOD_NUT_IAU_2000B in the
same process (ts.14's default):

| spacing | ts.19 | ts.20 | ts.20 limit |
|---|---:|---:|---:|
| hourly | 1.97x | 0.65x | 1.5 |
| daily | 26.9x | 2.68x | 5.0 |
| scattered | 35.7x | 4.34x | 8.0 |

astromcp (the downstream that found it), on ts.14 / ts.19 / ts.20:
- unit suite: 15.3 s / 30.5 s with 3 timeouts / 14.7 s, all passing
- `test_every_reported_return_really_is_at_the_natal_longitude`: 0.67 s / 7.19 s / 1.15 s

What changed (`swephlib.c`, behind `SWE_UPSTREAM_COMPAT` as before):
- `calc_nutation_2000a_derivs()`: the full series with its first three time
  derivatives. Each term's sine and cosine are products of e^(ik x) powers of its
  arguments, one factor per nonzero multiplier, instead of fmod + sin + cos per
  term. The nonzero multipliers come from `swenut2000a_sparse.h`, generated once by
  `tools/gen_nut_sparse.py` (the 2000A tables are a fixed standard); the dense row
  walk cost more than the products. 23 us per node at -O2, against ~60 us for one
  evaluation of the direct series.
- `calc_nutation_grid()`: nodes one day apart, septic Hermite on the two nodes around
  the instant (value and three derivatives at each). One node per call at daily
  steps, two for a fresh scattered instant, where ts.19 took six. G29 worst
  0.0014 uas over 1800-2100 and -13000..15000 (ts.19: 0.018 uas).
- The value depends on the instant alone, never on what the cache held.
- Explicit SEMOD_NUT_IAU_2000A still runs the direct series, so G29 compares the grid
  against an independent evaluation.

Considered and rejected: evaluating the fast series directly at every instant (no
grid). Exact, and faster on the return test (1.00 s), but SEFLG_SPEED evaluates
nutation at t and t - NUT_SPEED_INTV, and `nutv` is reused as a primary nutation
when a later call asks for that instant (sweph.c, swi_check_nutation and the
lookup beside nut_matrix), so it cannot be a Taylor shortcut; dense scans then pay
two full evaluations an instant and astromcp's suite took 27 s with 2 timeouts.

What is left: a fresh scattered instant costs two nodes of the full series, which
2000B's 77 terms never did, so scatter-heavy searches remain ~1.7x ts.14. That is
the price of full 2000A at a fresh instant with history-independent values.

New gate G32 `tests/nutspeed.c` (-O2): part 1 checks the generated sparse header
against nls[]/npl[]; part 2 the ratios above. Fails on ts.19 at all three spacings.

## The Moon's light time and the ephemeris time argument

Found comparing against Ephemeris Prometheia and JPL Horizons (DE441 on both sides;
DE440 and DE441 Moons themselves differ by 1.3 mas now, 21 mas at 1800, 5.8 mas at 2100).
SE's geocentric astrometric Moon was rebuilt exactly (0.00000 mas at 8 dates) from its
own geometric vectors, and a corrected recipe matches Prometheia to 0.0000 mas. SE's
error (0.02-1.71 mas) is the combination of three things in `app_pos_etc_moon()` and
the ephemeris reads:

1. Time argument: the JPL file is read at TT; its argument is TDB. The Moon's 1 km/s
   geocentric speed times TDB-TT: up to 0.88 mas (Sun ~0.06 mas, planets 0.03-0.04 mas).
2. One-pass light time: tau = |geocentric Moon(t)|/c, never solving
   |B(t - tau) - E(t)| = c tau: up to 1.08 mas.
3. The retarded epoch t - dt formed as one double (~40 us resolution near the
   present; the barycentric Moon moves 30 km/s): up to 0.18 mas.

Both upstream 2.10.03 and the fork have all three.

### Light time and the rounded instant: fixed (items 2 and 3; UPSTREAM-BUGS 23, G33)

`app_pos_etc_moon()` keeps the one read it already makes at the rounded t - tau0 and
solves the equation from it: the Moon moves along its barycentric velocity at t by the
difference between the instant read and t - tau, and tau is recomputed from that,
three passes (each gains v/c). The rounding is recovered as (t - (t - tau0)) - tau0,
exact. No second ephemeris read. The velocity is the one saved with the Moon and Earth
at t, filled on every ephemeris with or without SEFLG_SPEED, so the flag moves nothing.

Moshier rounds nothing (it extrapolates by tau0 itself) and gets the iteration only;
its remaining 0.04 mas geocentric / 0.5 mas heliocentric is its finite-difference
velocities, against a Moon 766 mas from the Swiss files on average (worst 2.3"), so
re-reading its series at t - tau (doubling every Moshier Moon's cost) was rejected.

| G33, worst over 40 dates 1800-2100 | before | after |
|---|---|---|
| Swiss files, geocentric | 1.18 mas | 0.00007 mas |
| JPL de200, geocentric | 1.18 mas | 0.00003 mas |
| Swiss / JPL, heliocentric | 0.07 mas | 0.000002 mas |
| Moshier, geocentric | 1.12 mas | 0.04 mas |

G33 `tests/moonlt.c` builds M(t - tau) - O(t) from the library's own geometric vectors
(tau solved, rounding put back via the velocity) and checks the astrometric direction
to 0.01 mas (Moshier 0.1 / 1), plus SEFLG_SPEED independence. G24 has a Moon case in
both directions (0.0001 mas shipped, 1.18 mas compat). G1: 950 of 13,000 rows moved,
all Moon or Moon-derived, worst 1.02 mas; G17's swemini Moon moved 0.0000002 deg.

### The ephemeris argument is TDB: fixed (item 1; UPSTREAM-BUGS 24, G34)

The Swiss files take the same argument as the JPL file they were fitted to (the annual
term in the Swiss-vs-DE441 Moon's along-track offset is 0.006 ms, against 1.657 for a
converted argument), so both readers had it. `swi_tdb_minus_tt()` (USNO Circular 179
eq. 2.6: within 9 us of the full Fairhead & Bretagnon series over 1600-2169, 28 us over
1-3000, 1 ms at -13000/17000) is added to the offset into the record (`sweph()`'s
Chebyshev argument; `state()`'s day fraction, with the carry), never to the JD, which
would round it to 40 us. One-entry memo per context: +0.3-3.5% at hourly steps, +3%
scattered. Moshier unchanged (TDT argument; 766 mas from the files anyway). Swiss's
commented-out formula in `jplplan()` also divided seconds by 8640.

G34 `tests/jpltdb.c`: JPL's own testpo.200 rows (geocentric Moon, barycentric Earth;
47 rows in `tests/testpo200.txt` with URL and sha256) from de200.eph at the TT whose TDB
is JED: 7 mm (testpo's last digit); compat 1.5 m and 45 m. G24 case both ways.

### End to end, against Ephemeris Prometheia on DE440, 12 dates 1600-2600

| astrometric ICRF, worst | ts.19 | ts.20 |
|---|---|---|
| Moon | 1.340 mas | 0.011 mas |
| Sun | 0.063 mas | 0.001 mas |
| Venus / Mars | 0.040 / 0.034 mas | 0.001 / 0.001 mas |
| every planet | <= 0.040 mas | <= 0.002 mas |

Baselines: G1 7,804 of 13,000 rows (every file read; Moshier bodies bit-identical),
G10 78 of 96, G17 last digit on Mars and the osculating apogee.

## The default sidereal time is continuous at 1850 and 2050 (UPSTREAM-BUGS 25, G35)

`SEMOD_SIDT_LONGTERM` splices `sidtime_long_term()` onto the IERS 2010 expression at
1850-01-01 and 2050-01-01 0h UT by two constants. `sidtime_long_term()` takes the mean
Earth at TT for a UT instant, so it moves 0.041" per second of delta t (IERS: 0.0001" per
100 s), and the constants were fitted under the pre-2.06 delta t (Espenak-Meeus: the 1850
splice meets to 0.00003"). Under the default since 2.06 it steps **-0.097"** at 1850 and
**-1.908"** at 2050 (upstream and fork alike); cusps follow (3.7" at high latitude).

The offset is now the two formulas' difference at the boundary under the context's own
models (`sidt_lterm_offset()`), memoized by value on `astro_models[]` and delta t at the
boundary and at J2000. G35: continuous within 0.001" (0.00004" measured) under five model
sets in turn on one context; compat fails 10 of 12. +1.8% on `swe_houses()` before 1850.
G1: 993 rows, all sidereal-time consumers; values shift by -0.097" before 1850 and
+1.908" after 2050.

## The long-term sidereal time is Earth rotation plus precession (UPSTREAM-BUGS 29, G39)

Outside 1850-2050 the default came from `sidtime_long_term()`: the mean Sun's hour angle
plus the mean Earth's longitude at TT for the UT instant. So it followed delta t at the
Sun's mean motion, **4.1069" per 100 s** (the IERS 2010 model: 0.0001"), and with delta t
at hours in antiquity it stood +37" at 1000, +328" at year 0, +2382" at -3000 from the
IERS expression.

It is now ERA(UT1) + 0.014506" + c T + A(T), the IERS 2010 construction with A, the
accumulated precession in right ascension, integrated from the context's own precession
matrices instead of the IAU 2006 polynomial (meant for near J2000):
- A is the integral of the mean equinox's rotation rate about the mean pole of date,
  -(dM/dt M^T)[1][0], from J2000.
- c = -0.003874"/cy is the secular rate of the CIO locator s (precession-nutation
  coupling, Capitaine et al. 2003), which no precession matrix carries: on the IAU 2006
  matrices the rate at J2000 is zeta_A' + z_A' = 4612.160408"/cy, where the IAU 2006
  sidereal time has 4612.156534.
- So corrected, integrating the IAU 2006 matrices reproduces the IAU 2006 polynomial to
  0.09 mas over 1500-2500, which G39 checks on every run.
- Tabulated per context, lazily from J2000 outward, nodes one century apart, cubic
  Hermite; built once per precession model (0.2 ms for a date in 1500, 4.8 ms at -13000).
  `swe_houses()` before 1850 costs the same as before.

New default against the IERS expression: within 0.001" over 1800-2200, -0.046" at 1000,
-1.17" at -1000, -10.1" at -3000 (Vondrak 2011 against the IAU 2006 polynomial). G39:
delta t + 100 s moves it 0.00014-0.00015" at nine epochs from -50000 to +50000 (compat
4.1"); continuity where the table ends. The topocentric Moon at 1800 and 2100 now agrees
with Prometheia to 0.11-0.25 mas (was 0.31-3.7, the residual G37 left there). G1: 842
rows, all sidereal-time consumers.

## The rotation angle is not rounded at the size of the day count (UPSTREAM-BUGS 30, G39)

Found by this PR's macOS and apple-clang LTO jobs. G1b failed on the topocentric Moon's
speed at -999 (1.86e-5 deg/day against a 1e-5 tolerance). The IERS 2010 expression formed
the rotation angle as `(0.779... + 1.0027... * (JD - J2000)) * 360`. At -999 that product
is 1.1e6 turns, which a double holds to 0.3 mas, and where the multiply-add is fused (arm64
by default) the last bit is different. The three-point topocentric speed, 4.32 s apart,
turns 0.3 mas of sidereal time into that speed difference. Upstream reaches it only with
`SEMOD_SIDT_IERS_CONV_2010`; bug 29 made it the default everywhere. Reproduced on x86 with
`clang -O2 -mfma -ffp-contract=on`, to the digit.

`era_deg()` now uses the Conventions' own form (eq. 5.15). The fraction of the day is
taken from the JD, which is exact for any double, and only 0.0027... * (JD - J2000) is
rounded, at 0.6 uas. That also removes a second rounding: before -1357 the difference
JD - J2000 cannot hold every JD. Over 64 consecutive JDs, sidereal time is now on a
straight line within 0.0005 / 0.003 / 0.0014 mas at -999 / -13000 / +12000 (upstream's
form: 0.33 / 1.03 / 0.78). The FMA build against -O0 is within 7.6e-7 deg/day on the
topocentric Moon. G39 check 4 and a G24 case cover it. G1: 1,160 rows, sidereal time by
<= 0.36 mas, cusps by <= 0.63 mas.

## Speeds are the rate of the position (UPSTREAM-BUGS 31-33, G40, G41)

Found checking the topocentric Moon's speed (a follow-up in the first draft of this PR)
against a five-point difference of the library's own positions.

- **31, the three-point speed was the slope at t + dt.** `calc_speed()` fits a parabola
  through t - dt, t, t + dt and returned its slope at t + dt: 4.32 s late on the Moon. It
  serves `SEFLG_SPEED3` and (until 34) every topocentric request with aberration; the topocentric
  Moon's diurnal parallax made that 1.3e-3 deg/day. The Moshier Moon's velocity had the
  same formula. Now the slope at t; the library's three-point speed equals a three-point
  difference of its returned positions to the last digit.
- **32, the Moon's speed left out its light time's rate.** v_M (1 - dtau/dt) - v_O, not
  v_M - v_O: up to 4.3e-4 deg/day topocentric without aberration, 3.6e-5 geocentric. The
  planets had the term; the Moon never did.
- **33, speeds in the ecliptic of date left out its turning.** The obliquity changes (mean
  -47"/cy, nutation in obliquity up to 1.3e-5 deg/day), and velocities were rotated with
  the position's matrix only: every ecliptic latitude speed of date off by 1.2-1.6e-5
  deg/day, and the true node's and osculating apogee's declination speeds the same. Fixed
  in `app_pos_rest()`, the mean, true-node, interpolated-apsis, fixed-star and planetocentric
  paths; now <= 5.5e-8. The cost is inside run-to-run noise (it reuses the cached
  obliquity).

G40 `tests/moonspeed.c` (Moon on Swiss/JPL/Moshier, Mars `SEFLG_SPEED3`; upstream fails 11
of 12) and G41 `tests/eclspeed.c` (twelve latitude/declination speeds; upstream fails all
twelve), plus three G24 cases. G1: 1,265 rows for 31/32 (speeds, the Moshier osculating
apogee by up to 90" -- it is fitted to the Moon's velocity, ~18x -- and iterated results by
0.1 mas), 5,810 for 33 (speeds, and the Moon's node crossings by 0.3 mas), then 5,140 more
in the last bits (<= 4.1e-14 deg/day) when the obliquity rate went to one new evaluation.

## Topocentric speeds are analytic, and every light-time rate is exact (UPSTREAM-BUGS 34-36, G43)

`swe_calc_r()` sent every topocentric request with aberration to the three-point speed
(three positions 8.64 s apart, three times the cost), because the analytic speed's
aberration-rate term was wrong for an observer turning with the Earth. That term is now
right everywhere and the forced three-point path is gone; `SEFLG_SPEED3` still asks for
it. A topocentric speed costs 24 us instead of 31-43.

- **34, the aberration rate.** Aberration moves the apparent place by about tau V, so the
  apparent velocity carries tau dV/dt. It was taken as V(t) - V(t - tau), added whole:
  - a chord for a turning site, a few per cent of a 5.6e-4 deg/day diurnal aberration
    rate; now tau times the site's acceleration at t (`observer_accel()`);
  - nothing at all on the Sun (3.2e-4 deg/day topocentric); it now reads the Earth at
    t - tau (+7% on a geocentric Sun with speed);
  - radial too: aberration keeps the distance to first order, and the radial part moved
    distance rates by 7.4e-5 AU/day on Pluto; only the transverse part goes in now;
  - first order only, tau a at the midpoint: tau^2 a'/2 is added, a' from the Sun's
    pull at t (1.9e-6 deg/day on Nibiru before, 2e-7 on Pluto). Not from the two
    ephemeris reads' positions: across a Swiss-file segment they disagree with the
    velocities by 1e-9 AU, which made Mars 2e-5 off at -3001 in a first attempt.
- **35, the light-time rate.** Planets took it as a one-day straight-line difference;
  heliocentric and barycentric planets did not take it at all. Now dtau/dt =
  rho^.(v_P - v_O)/(c + rho^.v_P) from the solved vectors, for every centre, in the
  planet, fictitious, orbital-element and planetocentric paths. Mercury 3.1e-6 -> 4.8e-8
  geocentric, 1.6e-4 -> 2.4e-7 heliocentric; Pluto's distance rate 6.9e-5 -> 1.5e-8.
- **36, the site turned per UT day.** `swi_get_observer()` gave it EARTH_ROT_SPEED per
  TT day; that is the rate per UT day. Now times 1 - d(delta t)/dt: 3e-6 deg/day on the
  topocentric Moon at -999.

G43 `tests/topospeed.c`: the topocentric Sun, Moon, Mercury, Venus, Mars and Jupiter at
Zurich and 70 N, nine dates -999..2100, against five-point differences on
binary-fraction instants (at rounded instants the reference itself reads ~1.5e-6 on the
topocentric Moon), within 1e-6 deg/day; measured <= 5.6e-7 on the Moon, 2e-7 on the
rest. Upstream fails all twelve, by up to 1.5e-3 on the Moon. Three G24 cases. G1:
7,035 rows, speeds only, every class checked against a difference of the returned
positions; positions by <= 3.9e-11 deg topocentric and <= 9.3e-10 day on eclipse and
occultation times that iterate on a topocentric speed. G17: twelve speed digits.

## `swe_nod_aps()` speeds (UPSTREAM-BUGS 37, 38, G42)

- **37.** The nodes and apsides are built in the ecliptic of date, so their speeds carry
  its turning about the heliocentric point; the conversions took nothing out on the way
  to the equator and put nothing back about the geocentric point. What survived was
  de/dt x (the Earth's heliocentric position): 1.3e-5 deg/day on Mars's osculating node
  latitude speed, 1.3e-4 on the Sun's osculating perigee at J2000 (0.044 AU away). A
  probe predicting the error from that formula matched it to two digits at every date.
- **38.** The mean perihelion's and aphelion's latitude speed was never taken: the
  auxiliary position's latitude a day on went out as the speed (1.8 deg/day for Mars,
  3.4 for the Moon's mean perigee, and through the cartesian state 0.047 deg/day on
  Mercury's geocentric longitude speed).

G42 `tests/nodspeed.c`: Mars, Venus and Jupiter, osculating and mean, ecliptic and
equatorial, all four points at seven dates, within 2.5e-7 (latitude, declination) and
1e-6 (longitude, RA); measured <= 1.6e-7. Upstream fails all twelve. Two G24 cases. G1:
72 rows, nod_aps speeds only, positions bit-identical. `SEFLG_J2000` is left as upstream
has it: that path rotates points of date by the J2000 obliquity (see below).

## The light deflection's speed behind the Sun's disc (UPSTREAM-BUGS 39, G44)

`swi_deflect_light()` differences the deflection over 5e-7 day, and two of its quantities
cancel exactly where the deflection is largest: the elongation's sine as
sqrt(1 - (u.e)^2), and 1 + q.e, with u.e ~ 1 and q.e ~ -1. At 0.02 of the Sun's radius
they are ~1e-4 and ~5e-9 and carry ~1e-12 and ~1e-16 of rounding, which the step's tiny
change turned into a speed rough by 1.5e-5 deg/day from one instant to the next (Venus,
2016-06-06). It showed when bug 34 moved G1's `cov:meff[venus]` row differently on x86-64
and arm64, 1.12e-5 apart, over G1b's tolerance. Now |u x e| and |q + e|^2 / 2: spread
<= 2.9e-8, within 2.2e-7 of a central difference. G44 `tests/deflspeed.c` (the five
meff conjunctions; upstream fails on four), a G24 case; G1: the five meff rows.

## House cusp speeds are the cusps' rate, every system (UPSTREAM-BUGS 40-43, G45)

`swe_houses_ex2()` returns cusp speeds. sescan flagged 3632 of its 3840 house rates, worst
555 deg/day; now 408, worst 4.8e-7 (the Placidus residue below).
- **40.** Whole signs and 0 Aries stand still within a sign, but cusps 1/7 and 4/10 kept
  the Asc's and MC's speeds: 326-555 deg/day at 47 N, 1270 at 70 N. Porphyry's cusps 11
  and 12 were built on the Asc's speed instead of the MC's (1600 deg/day at 70 N; the
  K/P/G polar fallback shares the code). Krusinski never took a speed. Now 0, from the
  MC's speed, and differenced in ARMC.
- **41.** Koch left out the rate of the MC's ascensional difference (721 against 522
  deg/day at 47 N): now an analytic chain rule. Placidus and Gauquelin left out the pole
  height's motion with the cusp (3.2 deg/day at 47 N, 32 at 60 N, 54 Gauquelin): now
  the fixed point differentiated implicitly, dC = A_x / (1 - A_f f'(C)) (`PlacDer()`).
  The ten differenced systems (`B F I i L M Q S X Y`) took three points over one second
  of time (1e-4 deg/day at 70 N, Q 0.1 at 85 N): now five points over 2^-8 deg of ARMC.
- **42.** The ARMC turned at the constant `ARMCS`; the equation of the equinoxes moves it
  by -3.9e-5..+6.2e-5 deg/day (Regiomontanus 4.8e-4 at 70 N), and the obliquity's rate
  and Sunshine's solar declination rate (2.4 deg/day at 70 S) were left out. Now the
  library's own ARMC and true obliquity differenced 2^-6 day of UT either side, and the
  obliquity and declination partials added. Speeds are per UT day, like differences of
  the cusps over the time argument.
- **43.** Sidereal speeds lacked the ayanamsa's rate (3.8-8.5e-5 deg/day) and, on the
  ecliptic of t0 and the solar-system plane, the auxiliary frame's rates; sidereal whole
  signs got the Asc's speed on all twelve.

Cusp positions are bit-identical in both builds (26 systems x 4 sidereal modes x 40
cases). A call with speeds now costs 17-31 us against 11-21 without (upstream's speeds
cost ~0.5 us, wrong). G45 `tests/housespeed.c`: all 26 systems at 0/47N/47S/70N, three
dates, tropical/Lahiri/ECL_T0, 56,104 speeds within 1e-6 + 3e-8 x rate; worst 5.8e-6 (APC
at 70 N, 5.7e-9 of its rate). Upstream fails all 78 rows. G24: Koch plus whole signs 2e-10
shipped, 265 compat. G1 records cusps only and does not move.

## The interpolated apogee and perigee (UPSTREAM-BUGS 44, 45, G46)

- **44.** The light-time step was `xx[i] -= dt * xx[i+3]` with the speed, zero without
  `SEFLG_SPEED`, and from `i = 1`: the position depended on the flag (111 mas perigee, 13
  apogee) and x never had the correction. Now the apsis is computed at t - tau, as the
  osculating apogee is, for every flag set; tau from the first passes of its iteration
  (`swi_intp_apsides_est()`; the distance is stationary at the apsis, so two passes put
  the perigee within 4 km, one the apogee within 1 km: <= 0.0013 mas). Identical with
  and without the flag, to the bit. Cost, -O2, vs upstream: 1.3x without the flag (a
  first version that differenced the velocity was 2.7x), 1.03-1.13x with it.
- **45.** The last parabola steps (18" and 1.8" wide) read slope and curvature from the
  last digits of a 3.6e5 km distance: the perigee jumped 1.3 mas between consecutive JDs.
  The speed was a difference over +-0.1 day (1.1e-4 deg/day off), `SEFLG_SPEED3`'s over
  +-8.64 s of the jitter (1.5e-3). Now Newton steps from five points 180" apart below dd
  = 180", and both speeds over +-2^-8 day. Smooth to 1.8e-3 mas; speeds within 2.1e-7
  deg/day of a five-point difference.

G46 `tests/intpaps.c`; G24 0 vs 107 mas (speed flag) and 1.5e-3 vs 1.2 mas (roughness).

## `swe_nod_aps()`: the speed flag and J2000 (UPSTREAM-BUGS 46, 47, G47)

- **46.** With `SEFLG_SPEED`, the "restore" call after each point's aberration-rate call
  was a heliocentric Sun (zeros, returned early), so the Sun and Earth in the save area
  stayed at tjd - dt and every point after the first used the wrong Earth: Mars's points
  6-35 mas apart with and without the flag, Venus's focal point 664 mas. Now the restore
  repeats the setup call; positions agree with and without the flag to <= 8e-6 mas.
- **47.** Under `SEFLG_J2000` the points of date were turned to the equator by J2000's
  obliquity and nutated as J2000 coordinates: the Moon's mean node 7.9" from
  `swe_calc()`'s in 2026, 148" in latitude in 2326. Now `SEFLG_NONUT` with J2000, as
  `swe_calc()` does, and `nodaps_ecl_to_j2000()`; agreement to 4.6e-9", J2000 speeds
  within 1.4e-7 deg/day.
- Also: the topocentric points' speeds now take bug 34's aberration rate (Mercury 6.7e-6
  -> 4e-8 deg/day, Pluto 3.1e-4 -> 3e-8). Past 10 days of light time, tau a comes from
  the Sun's pull whole, where the series fails (3.7e-4 deg/day at 462 days).

G47 `tests/nodaps2.c` (upstream fails 34 of 35 rows); two G24 cases.

## Orbital-element bodies' speeds (UPSTREAM-BUGS 48, G48)

`swi_osc_el_plan()` gave a fixed ellipse's two-body speed while the position follows
`seorbel.txt`'s polynomials in T: the mean anomaly's own rate, the other elements' rates
and an equinox of date's turning were missing (Vulcan 2.9e-2 deg/day, Waldemath 1.3e-2
geocentric, every JDATE set 3.8e-5). Now the exact derivative of the position as it is
computed (`check_t_terms()` returns the polynomial's derivative). Every set in
`seorbel.txt` is within 2.2e-8 deg/day of five-point differences. G48
`tests/orbelspeed.c` (upstream fails every set but Harrington); G24 3.8e-9 vs 2.9e-2.

## Speeds of date: precession, aberration, deflection and nutation rates (UPSTREAM-BUGS 49, G49)

Found from a fixed star's distance rate (3e-3 AU/day on Spica), which differenced
star-sized vectors. Four causes, two of which are on every body:
- `swi_precess_speed()` took precession's rate as the general precession in longitude,
  leaving out the planetary precession (chi_A' sin eps = 4.2"/cy): every latitude speed
  of date off by up to 3.2e-8 deg/day (the 5.5e-8 that bug 33 left), a polar star's
  longitude speed by 7e-8. Now dP/dt x0 exactly. For the default Vondrak 2011 series
  the matrix and its rate come from the series and their term-by-term derivatives in one
  pass (`swi_precess_matrix_rate()`, the matrix `swi_precess()`'s to the bit): cheaper
  than upstream's code, bench calc-swieph 0.74 -> 0.70 s. A first version, three
  precessions per call, was +7%.
- The deflection's, aberration's and nutation's rates differenced vectors of the body's
  distance (for planets, up to 2.9e-8 deg/day of roughness): now the displacements are
  formed directly. Stars' aberration rate is central over exact instants, and
  `swe_fixstar()`'s sidereal speed takes the ayanamsa's rate (3.8e-5 deg/day).

Stars' angular speeds are within 6.6e-10 deg/day of their positions' rate, distance
rates within 1.7e-13 of the distance per day. G49 `tests/starspeed.c` (upstream fails all
36); G24 5e-7 vs 1.8e-3 AU/day.

That commit also changed the site-acceleration step of bug 34's term from 0.001 day to
2^-8, whose (omega h)^2/6 is 1e-4 of the diurnal aberration rate: topocentric longitude
speeds doubled their residual (Jupiter 2.0e-8 -> 4.5e-8 deg/day), and every topocentric
latitude speed carried ~2e-8. Corrected here to 2^-11 day: topocentric latitude speeds 6-9e-9
deg/day (Sun 6e-10), longitudes back to 2-3.5e-8.

## Moshier far from J2000 (UPSTREAM-BUGS 50, 51, G50)

- **50.** Series arguments formed as freq x T and reduced afterwards, at -3000 numbers
  of 2.7e10", rounded at more time than a JD resolves (Mercury 3.9e-3 mas between
  consecutive JDs, the Moon 0.011). Now J - J2000 as an exact two-sum and each argument
  formed with its rounding recovered (fma and the division remainder): Mercury 7e-7 mas,
  the Moon 7.5e-6.
- **51.** Every Moshier velocity was a finite difference at rounded instants (planets
  backward over 8.64 s, the Moon central over +-4.32 s), noisy at 3e-6 of the speed at
  -3000. The light time and the osculating elements carried that into positions:
  Mercury's apparent place 0.13 mas between consecutive JDs, the osculating apogee 45",
  Mars's osculating aphelion 9e-6 AU. Now the planets' and the EMB's velocities are the
  series' derivative term by term, the Moon's is a five-point difference over exact
  instants, and `lunar_osc_elem()` takes +-2^-8 day instead of the +-0.1 day that had
  averaged this noise. Each of `ecldat_equ2000()`'s samples now uses its own obliquity
  (3.8e-6 deg/day on the true node).
  - Planet speeds are now within 1.3e-12 deg/day of their positions' rate (were 1e-6 to
    1e-5). The osculating apogee over 64 JDs at -3000 is smooth to 0.044 mas.
  - Against the Swiss files, rms positions are unchanged to 1e-5". Mercury's geocentric
    speed rms goes 2.37e-5 -> 2.56e-5: the theory runs ~19 s behind the files, and the
    old 4.32 s lag leaned the same way by chance.

G50 `tests/moshprec.c` (upstream fails all 21); G24 2.5e-7 vs 1.6e-3 mas and 8.3e-10 vs
7.2e-6 deg/day.

## `tests/scan/sescan`: the audit that found most of these

The generalisation of G39's consecutive-JD check and of the five-point speed checks, run
across the API: smoothness over 64 consecutive JDs (precision loss beyond what the time
argument resolves), every returned rate against a five-point difference on a ladder of
power-of-two steps, and the position with and without `SEFLG_SPEED`. 9,892 cases (14
flag sets, three ephemerides, twelve epochs -13000..12000, houses, stars, ayanamsas,
nod_aps), about 4 s on 12 cores. Not a gate: `make -C tests/scan run`.

## Heliocentric light time is measured from the Sun (UPSTREAM-BUGS 26, G36)

`app_pos_etc_plan()` and `app_pos_etc_plan_osc()` took the first light-time pass from
the Sun and the second from the barycentre, and kept the second: the light time of a
heliocentric request was the barycentric distance over c. 0.84" on Mercury, 0.28" on
Venus, 2.6" on Vulcan against B(t - tau) - S(t). Now every pass measures from the
centre. G36: 51 rows (Mercury..Pluto on the Swiss files and de200.eph, Vulcan) to
0.000000"; was up to 2.608". Every planet within 0.010 mas of Horizons' Sun-centred
astrometric place 1600-2600.

## The topocentric observer stands about the true pole (UPSTREAM-BUGS 27, G37)

`swi_get_observer()` read `SEFLG_NONUT`, which every caller passes, as where the site is
rather than how the answer is expressed, and built it about the mean pole with mean
sidereal time: 15-277 m from its place, up to 0.15" on the Moon. It now builds the site
on the true equator of date with apparent sidereal time, rotates to the mean equator,
precesses, and applies the backward frame bias the old code noted and skipped (45 cm,
0.3 mas). G37: 25 site/instant pairs to 0.0000 m; was up to 276.6 m. Topocentric Moon
against Prometheia (same delta t): 0.04-0.26 mas 1975-2026; was 10-148 mas.

## The osculating apogee uses the ephemeris's GM (UPSTREAM-BUGS 28, G38)

`lunar_osc_elem()` fitted its ellipse with the 1996 Astronomical Almanac G M_earth, 3.3e-8
above DE440's GMB; a near-circular orbit's eccentricity amplifies that ~19 times, 0.2" on
the apogee. Now GMB from the open JPL file's header, else by the Moon file's DE number
(values checked against JPL's header.200/.403/.405/.430/.440/.441); Moshier takes
DE403's (assumed for DE404, whose header JPL does not publish). G38: the GM implied by the
reported apogee distance matches to 1.6e-13; was 3.47e-8. Against Prometheia's geometric
apogee: 0.06-1.0 mas 1900-2100; was 21-163 mas.

Baselines for the three together: G1 1,348 rows, all heliocentric, topocentric, local
eclipse/occultation, apogee or true-node (last bits) rows; G17's swemini apogee 0.059".

## Second round: UPSTREAM-BUGS 52-114 (G51-G111)

Started from the "Found, not fixed" list of the first round, then whatever each fix turned
up in turn. 60 entries so far; 57 and 94 turned out not to be defects (the reference's error, not
the library's) and 99 is below the floor below, so those numbers carry notes only. Entries
marked *fork* are defects of this fork's own code, not upstream's; *partly fork* means
upstream has the defect and the fork's shared configuration added to it.

### Wrong answers that depend on settings, call order or files

These are the ones a caller can hit without looking at the eighth decimal. Most need a
change in mid-session (a new path, a JPL file, a second thread) or a missing or broken file.

| | before | after | gate |
|---|---|---|---|
| 82 heliacal memos reused for another atmosphere, latitude or star catalogue | 0.67 mag, a day in an event | fresh answer | G81 |
| 83 a setting made before the first calculation is lost (*partly fork*) | 158 s of delta t in AD 1000, 0.9 deg refraction | kept | G82 |
| 84 `jplplan()`'s Earth and Sun answer for any ephemeris after a fall to Moshier | the JPL Moon 160 deg off, Mercury 0.30 deg | right file | G83 |
| 88 `swe_calc_orbel()` on a context's first call, and after another ephemeris (*fork*) | zeros with no error; 0.009 AU on Moshier | answers | G87 |
| 89 at the Swiss files' 600-year joins, the answer depends on the file an earlier call left open | barycentric Sun 47.6 mas at 1800.0, 1.03" at -7800; Moon 220 mas | identical in any order | G88 |
| 92 star memos and catalogue survive a new path; a path change resets a manual tidal acceleration; the observer survives an ephemeris switch (*partly fork*) | Sirius from the old catalogue; 9.13 s of delta t; 4.4" on the topocentric Moon | fresh | G91 |
| 97 `swe_split_deg()` nakshatras under a user-defined ayanamsa | 3 deg 20' early (10 deg read as Bharani 0) | Ashvini 10 | G96 |
| 98 star functions as the first call ignore `SE_EPHE_PATH`; a path change discards `swe_set_astro_models()` | a failed call; 18.27 s of delta t, 0.00056 deg ayanamsa | found, kept | G97 |
| 100 a missing Moon file whose neighbour does not hold the instant (*fork*) | the Moon NaN with no error, 37 deg off warm | Moshier Moon with a warning | G99 |
| 101 `swe_get_tid_acc()` after `SE_TIDAL_AUTOMATIC` or before initialisation; automatic values shared like manual ones (*partly fork*) | -25.80 or 0 | the value delta t uses | G100 |
| 59, 68 houses: sidereal Makransky computed as Treindl, Sunshine without files, RADIANS cusp speeds in degrees, stale Gauquelin cusps after the Porphyry fallback | 41 deg on Makransky cusps; ERR; 57.3x | right system, Moshier fallback | G58, G67 |
| 60 a fictitious body with a mean anomaly written "-12.5" does not move | 161 deg off its ellipse 200 days on | moves | G59 |
| 65 Kronos' built-in semi-axis has two digits swapped (no seorbel.txt) | 0.0169 deg at 2300 | right | G64 |
| 67, 71 star functions return the caller's flags; caches emptied by an instant of 0 answer for JD 0 | 15 deg on a barycentric Sun at tjd 0; NaN | right flags; no false hit | G66, G70 |
| 86 the JPL barycentric Sun takes its date from the Earth's slot | NaN speed on a first call; 0.14 deg sidereal ten years on | right | G85 |
| 90 `SE_ECL_NUT` with `SEFLG_SPEED3` | wrong angles in x[3..5]; 576 valgrind errors | right; clean | G89 |
| 103 files closed and reopened when nothing changed; `swe_deltat()` depends on the calling thread (*partly fork*) | 794 file opens per 100 setter rounds, 52.8 us a round; 1305 s vs 1475 s of delta t | 0 opens, 3.8 us; same on every thread | G102 |
| 104 a refused Swiss file empties every open file's constants; the path setter crashes on a refused moon file (crash *fork*) | the next three calls fail "damaged"; segfault | only that file refused | G99 |
| 106 sizes and indexes read from file headers, the delta-psi table and the star catalogue unchecked | a crafted file writes past arrays (10 of 14 cases crash under ASan) | refused | G103 |
| 107 with files named by `SE_EPHE_PATH`, first calls read along the compiled default path | Moon 8.6e-4 deg, delta t 6.2 s, UT1 6.7e-5 day | read along the path, once | G106 |
| 110 "Please call swe_set_ephe_path()" on a first call with a path in effect | warning on a right answer; `swe_calc_orbel()` refused | only with no path | G109 |
| 112 `swe_calc_orbel()` with no path ignores the strict fallback setting (*fork*) | answers from Moshier where `swe_calc()` refuses | as `swe_calc()` | G109 |
| 113, 114 seorbel.txt and sefstars.txt readers: a bad character in an element field loops forever; epochs read with `atof()`; powers T5-T9 ignored; ".0" read as 1; a broken catalogue's load error replaced by "could not find star"; a failed load leaks its table | `swe_calc()` hangs; Cupido 1.0 deg off; 10 kB a reload | refused with file, line and field | G111 |
| 115 a default-context calculation adopts another thread's setter half-way through (*fork*) | a crash in the JPL reader; 4.3e-6 to 1.1e-5 deg from two configurations in one call | one configuration a call | G112 |
| 116 `swe_set_astro_models()` of a version before 2.01 sets the tidal acceleration without publishing it (*fork*) | the automatic term back at the next sync: 9.1 s of delta t in AD 1000 | kept | G113 |
| 118 the path setter: a context's first tidal term opens and leaks the default path's moon file; `swe_get_tid_acc()` reports a dropped JPL file's term; a moon file installed later never read; the planet file opened twice on a first call (*fork*) | 106 descriptors after 100 contexts | none left; read; once | G115 |
| 119 a thread adopting another's `swe_set_jpl_file()` or `swe_set_ephe_path()` keeps the old files' positions and segments (*fork*) | Mercury 5.78e-5 deg, Jupiter 3.4e-6 deg from the old files | the new files' | G116 |
| 120 the Swiss Moon file missing: the Moon, and the Earth made from it, come from Moshier under `SEFLG_SWIEPH`, and the strict setting answers | the Moon 0.6-5.0" off the files, flagged Swiss; every geocentric body too | refused when strict; flagged `SEFLG_MOSEPH` with the fallback | G117 |
| 121 a sefstars.txt that failed to load is reparsed by every star call, a missing one searched for on every call | 842 us a call, 500 lines read each time | 0.3 us, nothing read | G118 |
| 122 `swe_set_ephe_fallback()` belongs to no configuration group: other threads and new contexts keep `SE_EPHE_FALLBACK`'s value (*fork*) | refused where the setting allows Moshier, or the reverse | the setting | G119 |

### Speeds and positions

The rest are precision: speeds that were not the rate of the position the same call
returns, and positions read at a rounded instant or along a straight line.

| | before | after | gate |
|---|---|---|---|
| 52 aberration rate first order in v/c | 3e-8 deg/day, every topocentric speed | second order | G51 |
| 53 Moshier moves a body back along its velocity instead of reading it at t - tau | 13 mas on Mercury; Moon speed 1.1e-4 deg/day without aberration | read at t - tau | G52 |
| 54 `SEFLG_SPEED3` true node and osculating apogee over 0.1 day | 1.4e-3 deg/day on the apogee | rate | G53 |
| 55 Moon's mean apsides' speeds one-day differences | 2e-5 deg/day | analytic | G54 |
| 56, 72 interpolated apsides stop short of their fixed point; the perigee's light time takes one pass (*fork*) | 1.21" on the perigee | converged | G55 |
| 58, 69 nutation rate a backward difference; nutation matrix rate holds the obliquity still | 6.5e-10 deg/day, 2.7e-8 in a polar star's RA | analytic | G57, G68 |
| 61 topocentric star deflected from the geocentre | 9e-6" | from the site | G60 |
| 62, 66, 93 the aberration rate's Earth lacks the Moon's pull (all ephemerides), and on Moshier takes its jerk | 6e-7 deg/day on far bodies | full acceleration | G61, G65, G92 |
| 63, 81 light-time position read at the rounded instant (planets, fictitious bodies) | 1e-9 deg noise, 4.7e-7 deg/day in differences; Waldemath 3.9e-8 deg | exact instant | G62, G80 |
| 64 planets' mean nodes and apsides leave out t^2, t^3 in their speeds | 4e-6 deg/day at +-3000 | full | G63 |
| 70, 80 `swe_nod_aps()` apparent and osculating speeds | 2e-6 deg/day on the Moon's osculating perigee, 3e-5 on Venus with DE200 | rate | G69, G79 |
| 73 the Moon's speed: site turning, TDB rate, light-time velocity, aberration shift | 8.7e-7 deg/day topocentric | rate | G72 |
| 74 deflection rate a one-sided step behind the Sun's disc | 1.9e-6 deg/day | central | G73 |
| 75 true node and osculating apogee divide by the step asked, not taken | 6e-6 deg/day on the apogee | rate | G74 |
| 76 star aberration rate takes the Earth's acceleration half a step back | 1e-9 deg/day in Polaris's RA | centred | G75 |
| 77 `swe_calc_pctr()`: rounded instant, whole-velocity aberration rate, deflection seen from the Earth, light time short | 1.25" in position | right | G76 |
| 78 light time solved along a straight line; Earth's and Sun's light time without rate | 7.8e-10 deg on Mercury; 1.7e-6 deg/day heliocentric Earth | on the orbit | G77 |
| 79 topocentric site acceleration 1.6e-6 low | 6e-10 deg/day | right | G78 |
| 85 Moshier Earth-Moon offset rate a difference of rounded arguments | 2e-14 AU/day roughness | analytic | G84 |
| 87 precession at exactly J2000 taken as the identity | 2.8e-10 deg at that instant only | Vondrak | G86 |
| 91 ayanamsa speed a backward difference over 0.001 day | 2.1e-6 deg/day for True Revati near the Sun; 3.6e5 at a 0 deg crossing | the rate | G90 |
| 95 Moshier Moon velocity a difference of the series; osculating apogee eccentricity from sqrt(1 - p/a) | Moshier perigee speed noise 5.5e-6 deg/day at -2500; de200 apogee 2.4e-8 | 3.6e-8, 2.8e-11 | G94 |
| 96 Earth-centred fictitious bodies on Moshier read the Earth twice | 2.7e-11 deg jitter, 3.4e-8 deg/day | 1.4e-9 deg/day | G95 |
| 102 Moshier's light time holds the Sun still | 3.0e-6 deg on every place without aberration and every heliocentric one | 1e-8 | G101 |
| 109 the same on Moshier's fictitious bodies, fixed stars (aberration and parallax from the heliocentric Earth) and apparent light time; the heliocentric Moon's speed | Cupido 2.9e-6 deg; Sirius 9.2e-7 deg; apparent distance 2.4e-6 AU (Pluto); Moon 4.1e-8 deg/day | 6.6e-10 deg; 1.1e-10; Moshier's own theory error; 1.9e-11 | G108 |
| 105, 108 `swe_nod_aps()` osculating ellipses with the AA's GMs (Pluto with Mercury's mass), the frame bias applied twice, the EMB from the AA's Earth-Moon ratio | Moon apogee 5.3e-5 deg off `SE_OSCU_APOG`; nodes up to 4.4e-4 deg (Mars); Neptune's perihelion on de200 2.5e-3 deg | 4e-11 deg | G104 |
| 108, 111 `swe_get_orbital_elements()`: AA GMs, Pluto with Mercury's mass, true anomaly cancelling near aphelion, periods without the body's mass | Pluto's period 51 min; Jupiter's 2.1 days; tropical periods 3.9e-5 long; anomalies 4.8e-9 deg | the ellipse's own | G107 |
| 117 `swe_calc()`'s mean node and mean apogee speeds divide by 0.001 a step that is not 0.001 | up to 1.4e-5 deg/day (2.5e-8 on this branch before) | 1.4e-10 / 2.4e-10 | G114 |

### Cost

| | before | after |
|---|---|---|
| bench calc-swieph | 0.70 s | final figure with the final pass |
| bench calc-moseph | 1.045 s | 0.957 s (the Moshier Moon's analytic velocity, 95) |
| calc-moseph, instructions (109: the Sun's barycentric motion on every Moshier path) | 2.291e10 | 2.334e10 (+1.85%) |
| Moshier Moon alone at new instants (109) | 616k instructions a call | 770k (+25%); the moon bench +55% |
| Moshier fixed star / fictitious body at a new instant (109) | 627k / 815k | 1024k / 1077k |
| Moshier Moon alone | 0.90 s | 0.565 s |
| sidereal planet, Lahiri / True Citra (91) | 9.2 / 14.9 us | 8.3 / 10.5 us |
| interpolated perigee (56, 72), vs upstream | 1x | 1.57x without the speed flag, 1.33x with it |
| true node with speed (75) | 16.8 / 72 us | 22.6 us Swiss (+35%), 113 us Moshier (+57%, measured before 95) |
| Moshier fixed-star speed (76) | 1x | +18% |
| `swe_calc_pctr()` (77) | 1x | +12% |
| a call inside a Swiss file join's light-time window (89) | 4.0 us | 4.1 us |

### Where the floor is

Agents kept finding smaller residuals, so this round stops at **1e-9 deg in position and
1e-8 deg/day in speed** (3.6 microarcseconds, about 7 mm at the Moon; the Swiss files are
1 mas from JPL, 280 times coarser). Behaviour defects are fixed whatever their size; a
residual below the floor is measured, written into its entry, and left:

- the same Earth two-read jitter as 96 on the Swiss files and de200, and the Moon's
  (3.3-4.4e-9 deg/day, 7e-15 AU; entry 96);
- Mercury's light-time velocity not moved with its position (9e-11 deg/day; entry 97);
- a +-1.5e-11 deg/day sawtooth on Jupiter at AD 500 in G51 (entry 97);
- the Moshier Moon's speed about 5e-9 deg/day off its positions' rate, flat with step;
- the geocentric Moon from `swe_calc()` at 2e-14 of its distance (rounding through
  barycentric coordinates).

### Still in progress

- 123 (merged): `swe_close()` zeroes the configuration generation, and the next sync reverts the
  settings `swe_close()` is documented to keep (this fork's).
- 124 (merged): the searches and multi-call functions (eclipses, rise and set, crossings, houses,
  heliacal events) still sync at every calculation they make, so a setter another thread
  calls mid-search gives an answer from two configurations (this fork's; 115's remainder).
- 125 (merged): Moshier moved the Sun over a light time with a second-order step, not
  enough over VEA's 462 days (2.1e-7 deg, 5.4e-9 deg/day; 109's remainder).
- 126 (merged): `swe_set_interpolate_nut()` leaves the nutation cached for the instant, so the
  switch has no effect there (7.4e-5").
- 127 (merged): `swe_close()` resets the observer and the sidereal mode, which the manual says it
  keeps.
- 128 (merged): a thread's first call can read the configuration before its context has adopted
  the shared one (delta t 63.83 s for the 86.4 s another thread set; this fork's).
- 129 (merged): with interpolation on, a change of nutation model keeps the old model's nodes
  (8.7e-3").
- 130 (merged): `swe_calc_orbel()`'s geocentric places carry an unpaired frame bias on the Earth
  (1.7e-6 deg at 1.2 AU).
- 131 (merged): after `swe_close()` the first UT calculation takes the default tidal term, the
  second the path's files' (the Moon 1.6e-3 deg apart in AD 1000).
- 132 (merged): `swe_time_equ()` and the local-mean-time conversions use JPL only if their own
  context opened the file (1.03 ms off on a thread that adopted it).
- 133 (merged): a Sunshine-house declination remembered before a context's first start is wiped by
  the start (cusp 2 off by 2.04 deg).
- 134 (merged): the nutation interpolation is not keyed on the JPL Horizons flags or mode (0.10").
- 135, 136 (merged): `swe_calc_orbel()` answered a Moshier barycentric request with the
  heliocentric place; its equator-of-date elements held the equinox at the observation
  instant (1.9e-2 deg at 80000 AU, 8.8e-5 deg/day).
- 137, 138 (merged): a thread adopting another's path change kept the old JPL file's DE
  number (11 s of JPL delta t in 1700); a JPL file a calculation opened set a tidal term
  the next delta t did not use.
- 139 (merged): with the fallback allowed and the files missing, a barycentric request is answered
  with Moshier's heliocentric place (0.24 deg on Mars).
- 140, 141 (merged): every topocentric site was placed at UT1 rounded to 40 us, so it
  stood still and jumped 13 mm (topocentric speeds 4.4e-8 deg/day off their positions,
  2.3e-9 now); `swe_calc_orbel()`'s return flag now matches `swe_calc()`'s.
- 142-144 (merged): the ayanamsa, sidereal houses and Gauquelin sectors with
  `SEFLG_JPLHOR` and no IERS corrections crashed (both builds; now answered as
  `SEFLG_JPLHOR_APPROX`, as `swe_calc()` does); a second thread never loaded the
  corrections, and a new path kept the old directory's (4.2e-2", 0.131").
- 145 (merged): `swe_deltat_ex(t, 0)` differs by 4.3 ms between threads until the JPL file opens.
- 146 (merged): the nutation interpolation straddles the Horizons approximation's 1962 step
  (1.95e-2").
- 147 (merged): a thread adopting another's path keeps the old directory's swe_deltat.txt (23 s at
  1990); 148 (merged): after the path is set again the first UT JPL Moon differs from the second
  (1.8e-3 deg in 1700).
- 149 (merged): `swe_nod_aps()` answers a Moshier barycentric request with the heliocentric points
  (0.077 deg); 150 (merged): `swe_calc_pctr()` reports the ephemeris asked for, not the one used.
- 151 (merged): the Woolard nutation's lunar anomaly turns 10^4 times too fast (0.24"); 152 (merged): the
  default nutation grid leaves the series beyond 1.5 million years.
- 153 (merged): speeds across the 1962 Horizons-approximation step difference across it (1.6e-5
  deg/day within a day, 0.1 deg/day at it).
- 154-156 (merged): a JPL file below DE 403 keeps the corrections a later one loaded (4.2e-2"); the
  corrections loader leaks its tables on a reload after a broken file, and its file on a
  failed allocation.
- 157 (merged): the Lahiri ICRC ayanamsa's epoch value subtracted the broken Woolard
  series' nutation, so with that fixed its defining 23 deg 15' came out 0.08" off.
- 158 (merged): `swe_calc_pctr()` ignored a failed read of the centre a light time back, so
  where the centre's files begin later than the body's it returned a speed 5e-3 deg/day off.
- 171, 172 (merged): an `eop_finals.txt` with a gap threw away a complete
  `eop_1962_today.txt` (`SEFLG_JPLHOR` fell to the approximation, 4.2e-2"); a failed
  allocation was reported as a missing file.
- 161, 162, 173 (merged): the Horizons approximation's frame bias and offset took the other
  side of the 1962 step from its models, and the light-time instant (1.2e-5 deg after the
  step); its offset turned the position but not the velocity (the Moon's latitude speed
  1.5e-6 deg/day); its yearly offset table was interpolated backwards (jumps to 0.99 mas).
- 164-166 (merged): a JPL file named before the path that holds it, or not named at all,
  gave a first JPL Moon unlike the second (1.8e-3 deg in 1700); without the Moon files the
  Swiss tidal term fell to the default instead of the planet files' (11 s of delta t in AD 1000).
- 167-169 (merged): `swe_nod_aps()` on Moshier gave the Sun's apsides as seen from the Earth
  (perigee at 2 AU); it took its centre and frame from the ephemeris asked, not the one that
  answered (0.13 deg after a fallback), and read the centre flags in another order than
  `swe_calc()` (0.077 deg).
- 159, 160, 163, 182 (merged): speeds of date differenced across SEFLG_JPLHOR's 1799 and
  2202 steps (5e-6 deg/day) and across a chosen short-term precession model's window edges
  (8.7e-5); the nutation rate taken as a difference that the series' rounding swamped far
  from J2000 (now each series' own derivative); the Horizons obliquity offset table
  interpolated backwards (0.44 mas).
- 176, 178, 180 (merged): `swe_nod_aps()` gave a JPL request fallen to Moshier no frame
  bias (1.6e-4 deg); its heliocentric and barycentric points took aberration and the Earth's
  light deflection (9.6e-4 deg for a node behind the Sun); on Moshier it rebuilt the Earth's
  orbit from a different Moon than the Earth was made with (8e-3 deg in the perihelion).
- 175, 177 (merged): without the J2000 files the first delta t took the default tidal term
  until a Moon opened another span's file (2.6 s in 1500); Moshier never wrote the
  barycentric Sun's slot, so after a Swiss call its heliocentric Moon took that Swiss Sun
  (0.41 deg), asteroids 0.14 deg, and its deflection the wrong Sun.
- 183-185, 188 (merged): speeds across Owen 1990's piece joins (2.6e-8 deg/day); the
  Moshier Moon's and the equinox-of-date orbits' frames across the precession windows'
  edges (9e-5 deg/day); the IAU 2000A/B arguments past 4e8 days evaluated exactly (9.5e-9
  deg of rounding jitter).
- 194, 195 (merged): the strict setting let a topocentric request with the heliocentric or
  barycentric flag through on an Earth made from Moshier's Moon, and the save area then
  answered later topocentric requests; a topocentric star fallen to Moshier placed its site
  with the files' delta t (2e-8 deg).
- 170, 174, 179, 192, 193 (merged): `swe_nod_aps_ut()` kept the files' delta t after a
  fallback (2.8e-7 deg); the Moon's mean points read the Moon they never use, so they were
  refused or fell back without the moon files (1.4e-6 deg topocentric); a refusal left the
  points unwritten; the strict setting refused the Sun's points the Moon cancels from, and
  answered planets' geocentric points from an Earth made with Moshier's Moon (8.7e-9 deg).
- 181, 186, 187 (merged): the long-term sidereal time across a short-term precession window
  (50.8" in 1776 under IAU 1976 short-term, 1143" at -5550 under IAU 2006); the true node's
  and osculating apogee's speeds there (2.9e-3 to 7.9 deg/day) and the house speeds
  (2.1e-3 to 8.1e-3 deg/day).
- 197, 198 (merged): a save-area answer skipped the strict test after
  `swe_set_ephe_fallback(0)` (258 where a fresh strict call refuses); a JPL star outside
  de200's span was refused where `swe_calc()` falls to Moshier.
- 196 (merged): `SE_ECL_NUT` answered any date with OK flags, NaN from 4e82 and a cache's
  zeros at -1e300; it now refuses past 1e12 days, where IAU 2000B leaves the 1e-9 deg floor.
- 189-191 (merged): `swe_get_orbital_elements()` on Moshier built the Earth-Moon
  barycentre from two different Moons (8e-3 deg on the Earth's perihelion); the Sun's
  topocentric `swe_nod_aps()` points were its geocentric ones (2.3e-3 deg); Moshier's Earth
  was made with the short lunar series' Moon (1.0e-7 AU at 2000, now 9e-9). Cost: Moshier
  `swe_fixstar2()` with speeds +18.6% instructions a call, calc-moseph +0.26%.
- 203 (merged): sidereal house speeds under `SEFLG_JPLHOR_APPROX` took the ayanamsa's
  rate across the 1962 nutation step (6.38e-4 deg/day). The Sunshine cusp's 1.2e-8 deg
  step at 1800 Jan 1 is the files' own join (the Earth 236 m, the Sun 383 m apart): data.
- 200, 202 (merged): the true node's and osculating apogee's speeds across Owen 1990's joins
  (1.1e-5 deg/day) and on Moshier across a precession window's edge (5.0e-6); the sidereal
  time's table at Owen's joins (2.8e-9 deg).
- 206, 208 (merged): a star with speeds refused at a file span's edge where it answers
  without them; a fictitious body whose light time crosses the Swiss files' start read
  Moshier's Earth silently (4.3e-3 deg), now falling back or refusing as a planet does.
- 199, 201 (merged): `swe_nod_aps()`'s osculating speeds were a three-point difference
  (Neptune's perihelion 5.3e-7 deg/day off) and read the frame's step at a precession
  window's edge (the Moon 0.16 deg/day, 217 at -+75 centuries). Cost: osculating
  `swe_nod_aps()` +7.5% instructions.
- 209, 214 (merged): the long-term sidereal time re-integrated from its table's end on
  every call past 40000 years (over 10 s at 1e12 days) and crashed at -1e300; now a bounded
  cost, and NaN past 1e12 days from the functions that have no error return.
- 218-220 (merged): `SE_ECL_NUT` with `SEFLG_NONUT` returned the last date's nutation;
  IAU 1980's arguments in double past 4e8 days (8.2 deg at 1e12); IAU 2000B without its
  published fixed offsets (3.75e-8 deg).
- 221 (merged): `swe_nod_aps()` changed centre with ICRS vectors on J2000 points (3.5e-8
  AU). Moshier's Mars 0.21 deg off at -3000 is the series' own fit (valid from -1350): data.
- 223, 224 (merged): the ayanamsa, sidereal houses and Gauquelin sectors took the JPL
  Horizons flags that sidereal places drop (1.72e-5 deg between a chart's cusps and its
  planets); Makransky's Sunshine cusps lost their digits near w = 90, 270 (0.72 deg).
- 210, 211, 216 (merged): refusal messages overran their 256-byte buffers past 1e188 days;
  a NaN date passed every range test (crashes in the mean node, hangs in the crossings,
  NaN with OK flags elsewhere); the Swiss file name came from an overflowed year past
  7.8e11 days.
- 205, 207, 227 (merged): a cached answer lost the "using Moshier eph. for moon" note, and
  the Moon's slot, relabelled Swiss, let later calls skip the note and the strict test; a
  JPL request with the Horizons flags kept them after falling back (Mars 3.5e-4 deg).
- 228-230 (merged): `swe_nod_aps()`'s J2000 speeds across a window's edge (8e-5 deg/day),
  Moshier's Moon's osculating points there (1e-5), its sidereal speeds without the
  ayanamsa's rate (3.8e-5).
- 212, 213, 215, 217 (merged): the ayanamsa at any date (Lahiri 121.4 deg at 1e12, NaN from
  1e84) and on Vondrak 2011 past its fit; the houses at a TT that SE_ECL_NUT refuses; delta t
  at NaN and +-inf (0 with no message, crashes in the UT functions); the polynomial precession
  models at any date (the mean obliquity 1.6e51 deg at 1e12 under Laskar 1986).
- 225, 226 (merged): near a JPL file's or the Swiss Moon files' edge the true node, the
  osculating apogee and `swe_nod_aps()` with SEFLG_SPEED answered from Moshier or were refused,
  where the call without the flag answers (de200 2^-10 day inside its end: 260 for 257, the
  node 2.2e-3 deg off; Pluto's node speed 1.2e-5 deg/day off, nothing noted).
- 231, 232 (merged): the nutation interpolation's nodes placed by whichever call found none
  (an instant's nutation 7.9e-7 deg apart by call order; now day nodes and a cubic through
  the nutation and its rate, 0.08 mas of the series); Woolard's arguments past 4e8 days
  (3.2e-4 deg off its formula at 1e12).
- 236, 247 (merged): `swe_gauquelin_sector()` methods 0 and 1 with SEFLG_SIDEREAL set the
  sidereal place in the tropical frame (Mars 1950 2.69 sectors off); a star-mode ayanamsa
  whose star is refused returned -65 with the ayanamsa dpsi, and its sidereal houses OK.
- 234, 235 (merged): the sidereal times evaluated in double and then reduced (IAU 2006
  left 0..24 h from 2e10 days; 180 deg off at 1e12; 24 h for a sum just under 0); the
  long-term sidereal time's rate a central difference (its integral never converged, 5.9e-8
  deg).
- 245, 246 (merged): Moshier's held Moon for the true node and apogee depended on call order
  (1.66e-3 deg/day); Moshier's aberration rate read across a window's edge (3.3e-5 deg/day).
- 237, 238, 243, 255 (merged): the LMT/LAT conversions answered the date plus 0 at absurd
  dates and longitudes; `swe_utc_to_jd()` OK at NaN seconds, the UTC converters' int casts of
  NaN and year INT_MIN dates inside the bound; refusal messages cut before the range from
  1e188; Moshier's messages naming a range 0.3 day narrower than the one tested.
- 249 (merged): SEFLG_SPEED3 at a span's edge differenced two ephemerides (the Moon 3.37 deg/day
  for 14.05), flagged JPL with a Moshier note, and strict never refused.
- 254 (documented): `swe_house_pos()` cannot answer sidereal whole-sign and 0-Aries houses (no
  date or ayanamsa reaches it; 0.30 and 0.84 house off); the header and comments give the
  caller's formula, and G168 pins the behaviour.
- 252, 253 (merged): every frame or format flag but sidereal moved a Gauquelin sector of
  methods 0 and 1 (radians 17.8 sectors, equatorial 0.148, J2000 0.0875); swetest's house
  positions took the printed place (-sid1 2.62 sectors off) and its diff modes the
  difference's place (Mars-Sun -15.3 deg for -100.9).
- 250 (merged): every stencil across a join of the Swiss files (the 1800 join: Chiron's node
  speed 1.3e-2 deg/day off, Pluto's perihelion 6.4e-3, the true node 4.5e-6, Aldebaran
  1.7e-7) took the two files' step for motion; now the rate on the instant's side.
- 239-242, 244, 248 (merged): 52 of 64 refusals left the caller's outputs as they were; far
  eclipse searches named a date from the lunation polynomial; the heliacal functions at any
  date; planet files named "asteroid eph. file"; the "trying Swiss Eph" note lost; a NULL JPL
  read (crash) for the barycentric Sun with SEFLG_SPEED when the JPL file is missing.
- 256, 257 (merged): the long-term sidereal time integrated a polynomial precession model past
  its range (up to 8.4 deg off a line under Owen); its far delta t rounding (5.9e-9 deg).
- 258, 259 (merged): Moshier's fallback Moon at its light-time instant in the other window's
  frame (1.7e-4 deg, 2.3e-2 deg/day); a topocentric star's site read across a window's edge
  (3.4e-4 deg/day).
- 268-272 (merged): the ecliptic-of-t0 and solar-system-plane houses gave the auxiliary ARMC
  (3.9 deg off on the solar system plane) and whole signs off the sidereal signs (20.3 deg),
  and missed the places' precession offset (2.3e-4 deg); Sripati's house 12 answered 1;
  Polich/Page's position line stopped at 1e-6 deg.
- 273-275 (merged): a topocentric Gauquelin sector took the observer from `swe_set_topo()`, not
  geopos (0.126 sector off; ERR with none set); the Earth's sector was a zero vector's;
  swetest's meridian and zenith distances moved with the frame options (-sid1: the
  Ascendant's zenith distance 69.09 deg for 90).
- 287-289 (merged): the ecliptic-of-t0 houses took their nutation offset's sign from the date
  (a 9.6e-3 deg step at t0, cusp speeds 0.31 deg/day off near it); ascmc[8] held an angle on
  the sidereal planes; azimuthal house positions counted backwards south of the equator.
- 264, 265, 267 (merged): within its light time after de200's start the Moon came out in the
  Sun's direction (256.1 deg for 159.8, flag 1, strict answering), asteroids from a zeroed Sun
  (a crash with SEFLG_SPEED); SEFLG_SPEED refused where the plain call answers; the JPL file's
  open warning lost with a dropped read.
- 260-263, 280, 282, 286 (merged): the leap-second date overflowed an int past year 214748
  (-ftrapv aborted); `swe_utc_time_zone()` moved the date by one day at most (12:00 at a
  -100 h zone gave hour 88); the UT functions named the TT ("jd 1281959700055.5513" for UT
  1e12); "%f" printed the double next to a range's end as the end; `swe_heliacal_ut()` lost
  the fallback note. `swe_date_conversion()` of an illegal date keeps setting *tjd (callers
  rely on it; now documented).
- 283-285 (merged): the nutation is refused outside -13200 .. +17191 (DE431's span) under every
  model: its series' amplitude rates and T^4 arguments turn it into noise beyond (|dpsi| 13
  deg at 1e11 days, the main term already 10-13% off at the range's ends); the formula
  sidereal times took an out-of-range obliquity; a topocentric site chose its precession
  model at the default delta t's TT, not the call's (2.9e-7 deg).
- 294, 295 (merged): swetest's cusp horizon at 0 m, and the cusps' RA/dec speeds the last
  body's.
- 266, 276 (merged): every speed stencil across a segment boundary inside a Swiss file (the
  osculating apogee 9.7e-3 deg/day off, Mars's perihelion 3.1e-3) took the fits' step for
  motion; SEFLG_SPEED3 divided by its nominal step, not its instants' spacing (1.7e-6 of
  every speed).
- 290-316 (in progress): the Moon's place for its light time after an Earth step, a
  geocentric point's deflection near the Sun; swetest's TT dates as UT for the houses, -fq/-fW on cusp rows; the mean
  node's speed at its range's ends, the searches' start message, `swe_jdut1_to_utc()` far
  out; a NONUT star past Vondrak's range, the site's acceleration across a model step; the light-time Earth at a Moon file's start, the JPL open warning
  in the node, asteroid and `swe_nod_aps()` paths; Makransky's cusps where the meridian distance passes 90, the
  horizon houses with the MC past the zenith, Sunshine house positions in the polar circles,
  the solar system plane before 5650 BC, Koch's MC and IC, Krusinski at the equator; rise-set and eclipse functions leaving geopos as the observer, the
  Earth's rise and transit, SE_ECL_NUT reading out of bounds in rise-set; swetest's cusp
  azimuths at 0 m and leftover speeds; heliacal and rise-set altitude bounds, the Sun's orbital distance
  extremes, heliacal_pheno's last slots, backward eclipse searches at a file's end; the
  nutation's own validity range and the formula sidereal times' obliquity past their range; the same at the files' segment boundaries (the osculating apogee
  9.2e-3 deg/day) and SEFLG_SPEED3's nominal step; the topocentric Gauquelin observer, the geocentric Earth's sector,
  swetest's meridian and zenith distances; the ecliptic-of-t0 and solar-system-plane houses' ARMC and whole signs,
  Sripati's and Polich/Page's house positions; the searches at absurd dates, outputs left on
  refusals, wrong or lost messages, a barycentric-Sun crash without the JPL file; the
  long-term sidereal time past its precession model's range; Moshier's Moon on the fallback
  and a topocentric star's rate at the windows; the UTC functions' int overflows; SEFLG_SPEED3 mixing ephemerides at a span's edge.

`swe_get_orbital_elements()`'s periods are now the osculating ellipse's for every body,
the Moon included: 27.02 days at J2000 where upstream scaled it to 27.16 (the real
sidereal month, perturbed by the Sun, is 27.32). Bug 106's checks are not behind
`SWE_UPSTREAM_COMPAT`: they refuse only files no valid ephemeris is, and 3672 calls over
every shipped file are bit-identical with them. The fork's default fallback setting is
strict: with no files, `swe_calc()` and now `swe_calc_orbel()` refuse rather than answer
from Moshier.

## Also

- Android: `swenut2000a_sparse.h` (G32's generated table) is linked into
  `contrib/android/jni/`, where ndk-build looks; the Android job could not find it.
- G31 `tests/spicarv.c` removes its scratch directory with `remove()` rather than an
  unchecked `system("rm -rf")`, which the gcc reference job's `-Werror` has rejected
  since ts.19.
- Build: `-ffp-contract=off` everywhere (root Makefile, tests, Android). clang fuses
  a * b + c by default and arm64 always can, which changed last bits wherever an operand
  is inexact: G10 (bit-exact JPL reader) differed in 78 of 96 rows on Apple clang once
  reading at TDB moved its day fractions off .5. gcc in ISO mode never fused, so its output
  does not move; clang -O2 -mfma now reproduces gcc -O0's G1 bit for bit.
- `swe_sidtime_r`: zeroes the nutation that `calc_nutation()` leaves unwritten for a model
  it does not know (a gcc -O3 maybe-uninitialized warning, also on main).

## Not changed

- Lahiri differs from Prometheia by a common ~1.3 mas on every body: the ayanamsa's
  anchor definition, not a defect.
- SE's `.se1` files for Ceres/Pallas/Juno/Vesta/Chiron are 32-774 mas from Horizons
  at 2026-10-24 (old orbit solutions): data, not code.
- IAU 2000B keeps the full Simon et al. polynomial arguments, where the published model
  (McCarthy & Luzum 2003, SOFA `iauNut00b`) takes their linear terms: 7.4e-9 deg apart in
  2023, 1.4e-5 at AD 1000. Within 2.0-3.7 mas of IAU 2000A from -3000 to 3000, as
  `swephexp.h` promises ("precision of milli-arcsec"); the published model is 1.44" off
  at -3000 and 62 mas at 1000-1800, and agrees only over 1995-2050, where both are 2-3.5
  mas off. The fork's default is IAU 2000A. A change to the published terms exists
  (b064fce on ts20-f231) and was left out as the less accurate nutation.
- SEFLG_SPEED3's own noise (Mercury 1.1e-8 deg/day one-sided, the Moon 2.5e-7): the 1e-4-day
  three-point difference the flag names; the manual calls SEFLG_SPEED the precise speed.
- The Sun's mean perigee speed behind the Sun's disc (reported 2.9e-7 deg/day off) is its
  places' rate to 2.6e-11: the reference's 2^-5-day stencil spanned the light deflection's
  0.0024-day variation there.

## Found, not fixed (follow-ups)

Every item the first round listed here is fixed in the second: the topocentric residual
(52), Moshier's x - tau v (53), `SEFLG_SPEED3` (54), the mean perigee latitude speed (55),
the interpolated apsides' iteration (56), the nutation rate (58), the four house defects
(59), the negative mean anomaly (60), the star deflection and flags (61, 67) and the
Earth's lunar pull (62). The ecliptic-of-date speeds at -3000 were already right (57).
What is open now is under "Still in progress" and "Where the floor is" above.

## Verified

For the second round so far: each of G51-G109 passes on this build and fails on the build
before its fix; an independent sweep built every gate G51-G89 against the first round's
tip (2e39062) and against the `SWE_UPSTREAM_COMPAT` build, and every one fails on both
(G70, for one, 4176 and 3342 checks). The full suite, warning matrix, re-baselines and
bench below are the first round's and are redone in the final pass.

- `make -k check` passes (G1-G50); the FMA-contracted build (clang -mfma
  -ffp-contract=on, which reproduces the arm64 results) passes G1b; the warning matrix (gcc and clang-22, -O0 -O1 -O2
  -O3 -Os, default and `SWE_UPSTREAM_COMPAT`, `-std=c17 -Wall -Wextra -Werror`) is clean.
- Each new gate was run against the tree before its fix and fails as its commit says.
- Baselines (G1, G10, G17) were re-baselined in the commits that moved them, and every
  moved row was classified against the fix that moved it; the commit messages give the
  counts. For 40-51, built in parallel, G1 was regenerated once at the end, with each
  move attributed by regenerating it at every commit. Moved speeds were checked against
  five-point differences of the positions, ce4da90 build against this one.
- Measured cost: bench calc-swieph 0.70 s (0.71 before 40-51); interpolated apsides 1.3x
  upstream without the speed flag; house speeds 17-31 us per call.
