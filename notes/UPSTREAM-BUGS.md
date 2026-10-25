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
| 64 | The planets' mean nodes' and apsides' speeds leave out their elements' t^2 and t^3 terms | Correctness | `swecl.c` |
| 67 | The star functions return the caller's flags, and an emptied nutation cache answers for JD 0 | Correctness | `sweph.c`, `sweconfig.c` |

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
where of date they agree to 1e-6 deg. A position defect, not fixed here.

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
dtau/dt, dtau/dt = rdot / c. Cost at -O2, against upstream: 1.3x without
the flag (perigee 62 -> 82 us, apogee 53 -> 69), 1.03-1.13x with it
(182 -> 205, 153 -> 157). A first version took X(t) - tau V(t), which
needs the velocity, three apsides, on every call: 2.7x without the flag.

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
apogee (7e-7 at 12000).

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
5, rounded to 5e-10 degree, which moves each position along the orbit.
Apparent positions are the body at t - tau, not a binary instant, and
their differences carry ~1e-7 deg/day of the instant's rounding on
Vulcan, 3e-7 on Waldemath; ecliptic-of-date output keeps the ~5e-8
deg/day latitude-speed residual every planet has there.

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
Left: the apparent speeds still lack tau times the acceleration, as Moshier
places a body at x - tau v rather than at t - tau (the Moon 4e-7 deg/day
with aberration and 8e-5 without, Mercury heliocentric 5e-6 AU/day in its
distance rate; entry 32 left the Moon's); and `SEFLG_SPEED3` still took
the node and apogee over 0.1 day, for every ephemeris (entry 54).

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
turning inside the nutation matrix, ~3e-11 deg/day times 82, left alone).
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
