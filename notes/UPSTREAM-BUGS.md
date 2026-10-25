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
| 23 | A bad `seorbel.txt` field hangs `swe_calc()`, terms such as "T5" or ".0" are misread, a broken `sefstars.txt` is reported as "star not found" | Correctness / Robustness | `swemplan.c`, `sweph.c` |
| 24 | Seven house defects: Placidus and Gauquelin cusps not converged, RADIANS speeds in degrees, sidereal Makransky computed as Treindl, Sunshine houses without files, Sunshine's Sun from the wrong ephemeris, stale Gauquelin cusps after the Porphyry fallback, the speed of a cusp on an equinox point | Correctness | `swehouse.c` |
| 25 | A setting made before the first calculation is lost; the first heliacal call after a setter fails | Correctness | `sweconfig.c`, `swehel.c`, `swephlib.c`, `swecl.c` |
| 26 | `swe_set_ephe_fallback()` is never seen by another thread or a new context | Correctness | `sweconfig.c`, `sweph.c` |
| 27 | A call on the default context adopts another thread's setter half-way through: its files closed under it, two configurations in one answer | Crash / Correctness | `sweconfig.c`, every public calculation |
| 28 | `swe_calc(SE_ECL_NUT)` with a speed flag differences its angles as a position, reads two doubles never written, and copies stale speed slots | Memory / Correctness | `sweph.c` |
| 29 | A missing Swiss file whose neighbour does not hold the instant leaves the neighbour open, and the Moon is never read (this fork's) | Correctness | `sweph.c` |
| 30 | A refused Swiss file empties the constants of every file left open; the path setter crashes on a refused moon file | Correctness / Memory | `sweph.c` |
| 31 | Values read from an ephemeris file's header and from the delta-psi and fixed-star tables index and size arrays unchecked | Memory | `sweph.c`, `swejpl.c` |
| 32 | `swe_house_pos()` answers a place in the wrong house: Sripati 1.0 for all of house 12 and half of house 1, the horizon system counted backwards south of the equator | Correctness | `swehouse.c` |
| 33 | `swe_gauquelin_sector()` methods 0 and 1 moved by SEFLG_SIDEREAL (2.6 sectors) and by the frame, form and centre flags (up to 18.4 sectors) | Correctness | `swecl.c` |
| 34 | Sidereal houses on the ecliptic of t0 and the solar system plane (J2000, J1900, B1950 always): ascmc[2] the auxiliary ARMC, whole signs off the sidereal signs, a step of 2 dpsi at t0, ascmc[8] not 0 | Correctness | `swehouse.c` |
| 35 | Kronos' built-in semi-axis has two digits swapped, the Kepler solver's starting guess is wrong, and the nakshatras of `swe_split_deg()` are shifted for `SE_SIDM_USER` | Correctness | `swemplan.c`, `swephlib.c`, `sweph.c` |
| 36 | The heliacal memos answer for an atmosphere, latitude or star catalogue they do not hold | Correctness | `swehel.c`, `sweph.c`, `sweconfig.c`, `sweph.h` |
| 37 | The default nutation (full IAU 2000A) costs 12 to 100 times IAU 2000B's when the calls are a day or more apart: transit, station and return scans, lists of unrelated charts (this fork's) | Performance | `swephlib.c` |
| 38 | `calc_dip()` is NaN below sea level: `swe_rise_trans_true_hor()` with horhgt -100 answers two hours after the start, `swe_refrac_extended()` never converts an apparent altitude, `swe_azalt()` refracts a body below the horizon | Correctness | `swecl.c` |
| 39 | The topocentric observer is placed about the mean pole of date, not the true pole: 200-270 m on the observer, 0.10-0.15" on the topocentric Moon | Correctness | `sweph.c` |
| 40 | A heliocentric light time is measured from the barycentre on its second pass, not from the Sun: up to 0.84" on Mercury, 2.6" on a fictitious body inside its orbit | Correctness | `sweph.c` |
| 41 | `swe_get_ayanamsa_ex()`, the sidereal houses and `swe_gauquelin_sector()` with SEFLG_JPLHOR and no IERS corrections loaded read the corrections' table, NULL, and crash | Crash | `swephlib.c`, `swecl.c` |
| 42 | The refusals of a date outside a range write its digits in full into 256 bytes: from \|J\| about 1e188 days the message overruns its buffer or the caller's serr | Memory | `swemplan.c`, `swemmoon.c`, `swejpl.c`, `swedate.c`, `swecl.c`, `swephlib.c` |
| 43 | `swe_sidtime()` reads a nutation `calc_nutation()` left unwritten; SE_ECL_NUT is taken for a body by the rise and set, the Gauquelin sectors, the occultations, `swe_calc_pctr()` and `swe_helio_cross()`, and reads `pla_diam[-1]` | Memory / Correctness | `swephlib.c`, `swecl.c`, `sweph.c` |

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

## 23. A bad `seorbel.txt` field hangs `swe_calc()`, terms such as "T5" or ".0" are misread, and a broken `sefstars.txt` is reported as "star not found"

**Severity: Correctness (a hang; fictitious-body elements read wrongly without an error, Cupido 1.0° off at J2000; the error that explains a failure lost; a wrong star answered OK)**
**Where:** `swemplan.c`: `check_t_terms()`, `read_elements_file()`; `sweph.c`: `swe_fixstar2_r()`, `swe_fixstar2_mag_r()`, `load_all_fixed_stars()`, `swi_fixstar_load_record()`

An element field of `seorbel.txt` is a sum of terms, each a product of
numbers and powers of T. `check_t_terms()` reads it, and:

- a character that is none of sign, blank, `*`, T, digit or point ("12x",
  "40.99837e0", "0.0046q", "171.4333 * x", "nan") was consumed by no branch
  and the loop came back to it for ever: `swe_calc()` for that body never
  returned, in any of the six fields;
- the power after T was `atoi()`: "T5" to "T9" (and "T12") were ignored, so
  "163.7409 + 1 * T5" read as 164.7409, "T0" read as T, and "T 2" (`atoi()`
  skips blanks) as T2 times 2;
- a number was taken only if `atof()` was not 0 or it began with '0', so
  ".0" was skipped and the term read with factor 1 ("163.7409 + .0 * T" as
  "+ T", Cupido 1.0° off at J2000), and "1.2.3" read as 1.2;
- an empty field was the value 0.

`read_elements_file()` read the epoch and equinox with `atof()` ("2451545.0abc"
as 2451545, an empty field as JD 0), and a line with fewer than nine fields
stopped the scan, so every body after it was refused too ("name not found"
for Hades when Cupido's line was cut).

In `sefstars.txt`, `swe_fixstar2()` and `swe_fixstar2_mag()` dropped
`load_all_fixed_stars()`'s return: a catalogue line that does not parse, or no
catalogue, was reported only as "could not find star name aldebaran". A load
that failed part way kept its table, and the next call's load dropped the
pointer to it. `swe_fixstar()` and `swe_fixstar_mag()` with `serr` NULL
answered a star that is not there with the file's last (Gliese 710).

**Fix, as done here:** `check_t_terms()` refuses a field when a pass consumes
nothing (exactly the case that looped), when it is empty, when a power after T
is not 1 to 4 ("T0", "T5", "T2.5"; T alone is T1), and when a run of digits
and points is not one number (".", "1.2.3"); a number is its value by
`strtod()`. The epoch and equinox are a whole finite number or a name. A short
line refuses its own body only. All refusals name the file, the line and the
field ("error in file seorbel.txt, line      28: mean anomaly value
invalid"). A star not found after the catalogue failed to load is reported
with the load's error first ("data of star 'Atest,alTes' incomplete; error,
swe_fixstar(): could not find star name aldebaran"); the reload frees the
partial table; the record reader returns ERR with `serr` NULL. Every body of
the distributed `seorbel.txt` (six instants and its names), catalogue stars
1 to 1141 through `swe_fixstar2()` and `swe_fixstar2_mag()`, and lines 1 to
1210 through `swe_fixstar()` and `swe_fixstar_mag()` are bit for bit as
before. G32 `tests/fictstar.c`: 20 cases in child processes under a 5 s alarm.

## 24. Seven house defects: Placidus and Gauquelin cusps not converged, RADIANS speeds in degrees, sidereal Makransky computed as Treindl, Sunshine houses without files, Sunshine's Sun from the wrong ephemeris, stale Gauquelin cusps after the Porphyry fallback, the speed of a cusp on an equinox point

**Severity: Correctness (up to 1.1e-5 deg on a Placidus cusp; 57.3 times on every RADIANS speed; 41 deg on sidereal Makransky cusps; ERR and Porphyry, 16.7 deg off, for Sunshine houses without files; 6e-6 deg on a Sunshine declination; 24 cusps and 24 speeds left as the caller had them; up to 427 deg/day on a cusp speed at one instant)**
**Where:** `swehouse.c`: `CalcH()` (Placidus, Gauquelin), `swe_houses_ex2_r()`, `swe_houses_r()`, `swe_houses_armc_ex2_r()`, `sidereal_houses_trad()`

1. The Placidus and Gauquelin cusp is the fixed point of C = Asc1(ra,
f(C)), iterated until two cusps lie within 1/360000 deg. The last step
is the error times the iteration's contraction, so the cusp stayed off
the fixed point by 1.4e-9 deg at 30-40N, 1.8e-8 at 40-50N, 1.3e-7 at
55-60N, 6.2e-7 at 63-65N, 1.3e-6 at 65-66N and 1.1e-5 within 0.01 deg of
the polar circle (Placidus and Gauquelin, scanned over ARMCs 7.3 deg apart). The
cusps of consecutive instants differ by the rate of that error, so a
speed taken from differences of the returned cusps is off by it.
2. With `SEFLG_RADIANS`, `swe_houses_ex2()` turned the cusps and ascmc to
radians and left `cusp_speed` and `ascmc_speed` in deg/day: 57.3 times
the radian rate.
3. `sidereal_houses_trad()` took `toupper()` of the house system, so
sidereal Makransky Sunshine (`'i'`, the one lower-case system the tropical
path dispatches; the others are upcased with a deprecation warning)
computed Treindl's `'I'`: 41 deg apart at worst at 60S.
4. Sunshine houses need the Sun's declination, from `swe_calc_ut()`. With
no Swiss files for the date, and the ephemeris fallback off (this fork's
default), it refuses; the houses went to Porphyry and returned ERR (16.7
deg from the Sunshine cusps), `ascmc[9]` 0.
5. `swe_houses_ex2_r()` asked the Sun's declination with no ephemeris
flag, so a `SEFLG_MOSEPH` (or `SEFLG_JPLEPH`) call got the Swiss
ephemeris' Sun whatever the caller chose: at 2026-07, 47N, the declination
23.026219549 against Moshier's 23.026225558 (6.0e-6 deg; the cusps 3.8e-6
deg). `swe_houses_r()` has no `iflag`, so there is nothing to pass there.
6. A Gauquelin call (36 cusps) that fell back to Porphyry (within the
polar circle, a negative return) has 12 cusps. Cusps 13..36 and their
speeds stayed as the caller had them, and with `SEFLG_RADIANS` the
conversion multiplied them anyway: at 80N arrays prefilled with 7 came
back 7 in degrees and 0.1222 in radians, 24 cusps and 24 speeds each. In
the sidereal calls the ayanamsa was subtracted from them as well.
`swe_houses_r()` and `swe_houses_armc_ex2_r()` left them the same way.
7. In the Placidus and Gauquelin branches, when the first-guess cusp is on
the equator (|tan dec| < 1e-10, the cusp at an equinox point; cusps 11, 12,
2, 3 and sectors 2..9, 29..36) the cusp is the right ascension and its
speed was set to the ARMC's, 360.99 deg/day. The cusp moves at A_x, the
rate of Asc1 in the right ascension at the pole height m tan(fi) that the
semi-arc fraction m gives there (the pole height has a zero derivative on
the equator): 415 to 788 deg/day at 47N and 60N, up to 427 deg/day from
what the ARMC's rate gave.

**Fix, as done here:** (1) The stopping step is 1e-12 deg, with an Aitken
step once the steps are below 1e-3 deg and shrinking by less than 0.9 (the
remaining error is d r / (1 - r)): about one evaluation of the map more
per cusp. Plain steps down to 1e-12 would exhaust the 100 iterations a few
hundred metres short of the polar circle, where the old stop still
answered; the Aitken step keeps the answers: of 242190 Placidus and
Gauquelin calls over the last 0.02 deg of latitude before the circle, 9
(within 65 m of it) go to Porphyry that did not, against 303 with Aitken
limited to ratios below 0.5. Elsewhere no call changes between OK and
Porphyry.
(2) The speeds are converted with the positions. (3) `'i'` is kept as is
for the call inside, as the tropical path does. (4) and (5) One
function asks the Sun with the caller's `SEFLG_JPLEPH`, `SEFLG_SWIEPH` and
`SEFLG_MOSEPH` bits and, on a refusal, again from Moshier: the declination
is an input to the houses, not an ephemeris position the caller asked for,
and Moshier's is 0.1" off (cusps 3.8e-6 deg from the Swiss-file call).
(6) A Gauquelin return below zero zeroes cusps and speeds 13..36 in
`swe_houses_armc_ex2_r()` (so `swe_houses_r()` and the tropical
`swe_houses_ex2_r()` too), and the three sidereal paths then keep the 12
Porphyry cusps only, as that function does, so that they do not subtract
the ayanamsa from the zeros. (7) The speed is A_x * ARMCS at pole height
atan(m tan fi).

G33 `tests/housecusps.c`: the cusps of Placidus 11, 12, 2, 3 and
Gauquelin sectors 2..9, 29..36 satisfy their defining condition (the right
ascension at the fraction of the cusp's own semi-arc) to 1.1e-13 deg at 47N,
60N and 65N over 24 ARMCs (3.0e-7 before); the 12 equinox-point cusps are
within 6.5e-8 deg/day of five-point differences (427 deg/day before);
RADIANS speeds are the degree speeds times pi/180 to 3.2e-18 (0.98
before); sidereal `'i'` is the tropical cusps less the ayanamsa (41 deg off
before); Sunshine houses are OK without files and agree with the
Swiss-file call to 3.8e-6 deg (ERR, 16.7 deg off before); the MOSEPH call's
`ascmc[9]` is Moshier's Sun to 1e-12 (the Swiss one before); 0 of 96
entries 13..36 stale in each of four kinds of call and 0 of 72 from
`swe_houses_r()` and `swe_houses_armc_ex2_r()` (all stale before).

## 25. A setting made before the first calculation is lost, and the first heliacal call after a setter fails

**Severity: Correctness (a failed call; a lost setting: 22.6 s of delta t, 158 s of delta t in AD 1000, 0.9 deg of refraction)**
**Where:** `sweconfig.c`, `swi_config_publish()` and `swi_config_reset()`; `swehel.c`, the observer set by `swe_vis_limit_mag()`, `swe_heliacal_pheno_ut()`, `swe_heliacal_ut()` and `Magnitude()`; `swephlib.c`, `swe_set_tid_acc()`, `swe_set_delta_t_userdef()`, `swe_set_interpolate_nut()`; `swecl.c`, `swe_set_lapse_rate()`

On the process-wide default context, `swe_set_ephe_path()` and then
`swe_vis_limit_mag()` returned ERR, "geographic position has not been
set", at any instant; the same call again worked, and after any other
setter, or `swe_close()` and re-open, the next one failed again. So did
`swe_heliacal_pheno_ut()`. Three causes:

- The fork's shared configuration master started, and `swe_close()`
  re-armed it, as zeros. A setter merges in its own group only, so the
  first sync after it adopted every group the context had not claimed as
  zeros: an observer not set, a lapse rate of 0 for 0.0065, a tidal
  acceleration of 0. After `swe_set_ephe_path()` alone the Sun from
  2000 m, 15.75 min before its true rise, was refracted to -1.243 deg
  where it is -2.215, 0.97 deg; `swe_ctx_new()` inherited the zeros too.
- `swehel.c` set its observer for the call without syncing first, so the
  first `swe_calc()` inside the call put the master's "not set" back.
- `swe_set_tid_acc()`, `swe_set_delta_t_userdef()` and
  `swe_set_interpolate_nut()` (upstream's too) and the lapse rate (a
  context field in the fork) did not initialise the context, so set before
  any calculation they were wiped by its first-use memset:
  `swe_set_tid_acc(SE_TIDAL_DE200)` as the first call left -25.80, and
  `swe_deltat()` in AD 1000 was 1463.43 s for 1305.30; a user delta t of
  86.4 s read back 63.83; the interpolation was lost (8.6e-8 deg of
  nutation); 0.05 K/m read as 0.0065 (0.90 deg).

**Fix, as done here:** the first publish after a start or a reset fills
the master with the defaults a fresh context has; `swehel.c` syncs before
it sets its observer, as `swecl.c` does; the four setters initialise the
context first. G34 `tests/firstset.c`: every setter and heliacal entry
point, each in a forked pristine process, after `swe_set_ephe_path()`, as
the very first call, and after `swe_close()` and re-open, against a warm
context: 54 scenarios pass, 9 failed before.

Not fixed here, upstream's too: a manual tidal acceleration is reset to
automatic by every path change, including the implicit one at the first
calculation when the files are named by `SE_EPHE_PATH`.

## 26. `swe_set_ephe_fallback()` belongs to no configuration group, so another thread never sees it

**Severity: Correctness (a Swiss request without its file refused where the setting allows Moshier, or answered from Moshier where it refuses)**
**Where:** `sweph.c`, `swe_set_ephe_fallback()`, `swe_get_ephe_fallback()`, `swe_close()`; `sweconfig.c`

The fork's own setting. Every other setter publishes its group to the
shared configuration, which the other threads on the default context adopt
at their next sync and a new context inherits; this one wrote the
context's field and published nothing. With `SE_EPHE_FALLBACK` unset the
main thread read 1 after `swe_set_ephe_fallback(1)` and a second thread 0,
and the second thread's Swiss Mars at J2000 without `sepl_18.se1` returned
-1 where the main thread's returned 4 (Moshier); with the variable set,
`swe_set_ephe_fallback(0)` left the second thread on Moshier. A context made
by `swe_ctx_new()` after the setting took the variable's value, and
`swe_get_ephe_fallback()` as a thread's first call read the context before
anything had started it: 0 with the variable set.

**Fix, as done here:** the setting is the configuration group
`SWI_CFG_FALLBACK`, captured, merged, adopted and inherited like the
others, with no invalidation (the setter invalidates nothing). The master's
default for it is the variable's, as a fresh context's is, so a setter of
another group does not turn a thread strict. The getter initialises and
syncs first. `swe_close()` keeps the context's ownership of the group, as
it keeps the value. G35 `tests/fbshare.c`: 28 forked scenarios with a
second thread and explicit contexts, the variable unset and set; 9 failed
before.

## 27. A call on the default context adopts another thread's setter half-way through

**Severity: Crash (SIGSEGV in `swejpl.c`'s `state()`) and Correctness (wrong answers, no error)**
**Where:** `sweconfig.c`, `swi_config_sync()`, new `swi_config_hold()`; the public functions that calculate, in `sweph.c`, `swecl.c`, `swehel.c`, `swehouse.c`, `swephlib.c` and `swedate.c`; `swephlib.c`, `swe_get_astro_models()`

The fork's own. The default context adopts the shared configuration at
`swi_init_swed_if_start()`, and one call passes it many times:
`swe_calc_ut()` at its delta t and again at `swe_calc()`, a topocentric
`swe_calc()` at the observer's delta t and sidereal time, a star at the
Earth's `swe_calc()`, a search at every calculation it makes. A setter
another thread called in between was adopted there: the files the call
was reading were closed and its caches emptied, and the rest of the call
ran on the new configuration. A topocentric JPL Jupiter by `swe_calc_ut()`
while another thread switched `swe_set_jpl_file()` crashed on the closed
file. Elsewhere one answer mixed two configurations: a sidereal
`swe_mooncross_ut()` 5600 s off after `swe_set_sid_mode()`, sidereal houses
0.883 deg, `swe_sol_eclipse_when_loc()` searched at the master's site from
half-way (contacts up to 4990 s off, a contact reported visible that is
not), a fictitious body 0.42 deg/day in its speed after
`swe_set_astro_models()`. Some calls synced only after reading part of the
configuration, or never: `swe_deltat_ex()` read the user delta t before
its sync and the UTC conversions had none, so another thread's
`swe_set_delta_t_userdef()` stayed out of them (22.6 s); and a thread's
first call read what came before its first sync from a context nothing
had started: zeros for the user delta t (22.6 s, and 3.1e-3 deg in
`swe_calc_ut()`'s Moon), the models in `swe_sidtime0()` (3.1e-7 h) and
`swe_get_astro_models()`.

**Fix, as done here:** every public function that calculates holds the
configuration: its old body is a static `<name>_body()`, and the public
function calls `swi_config_hold()` first and `swi_config_release()` last.
The outermost hold syncs once, at the entry, and every sync nested in it
does nothing until it returns; a context not yet initialised it starts,
which syncs it, and the first `swi_init_swed_if_start()` after that still
answers 1, so the first-call warnings are unchanged.
`swe_get_astro_models()` starts and syncs first. 60 functions are held;
not held are those whose body is one call to a held function
(`swe_houses_ex()`, `swe_lmt_to_lat()`), the arithmetic on the caller's
ARMC (`swe_houses_armc()`, `swe_house_pos()`), the setters and the getters.
G36 `tests/cfghold.c`: 18 cases, each in its own process, every answer
bit for bit a fresh context's or a fresh process's: four calculations and
five searches whose file (seorbel.txt, sefstars.txt, swe_deltat.txt) is a
FIFO the main thread fills only after its setter, so no timing; the UTC
conversions and delta t after a setter made before them; seven first calls
on a new thread, and the first-call warning kept. 17 failed before.

`swe_split_deg()` with `SE_SPLIT_DEG_NAKSHATRA` reads the sidereal mode and
is held too: on a new thread after the main thread's True Sheoran, 10 deg
came back nakshatra 0, 10 deg 0' where the main thread gives nakshatra 1,
0 deg 0'. G36's split case.

## 28. `swe_calc(SE_ECL_NUT)` with a speed flag differences its angles as a position, reads two doubles never written, and copies stale speed slots

**Severity: Memory (an uninitialised read) / Correctness (x[3..5] wrong by up to 1.6e-3 deg)**
**Where:** `sweph.c`: `swe_calc_r()`

SE_ECL_NUT answers four angles: the true and mean obliquity and the
nutation in longitude and in obliquity. With SEFLG_SPEED3, or with
SEFLG_SPEED and SEFLG_TOPOCTR (which takes the three-position speed),
`calc_speed()` differenced them as if they were a position. x[3], the
nutation in obliquity (-1.6026e-3 deg), came back as the true obliquity's
rate (-6.06e-6), and x[4] and x[5] as rates (up to 5e-5).
`denormalize_positions()` read `x0[12]` and `x2[12]`, which nothing writes:
valgrind counts 1948 errors over a 30-call hostile run, and on a stack that
leaves NaN there, UBSan stops in `swe_revjul()`. With SEFLG_SPEED alone,
x[4] and x[5] were copied from the save area SE_ECL_NUT shares with the
asteroids, so they were stale.

**Fix, as done here:** SE_ECL_NUT never takes the three-position speed, and
no speed slots are copied for it. Any speed flag answers as none: x[3] is
the nutation in obliquity, and x[4] = x[5] = 0. G37 `tests/eclnutspeed.c`
compares the six slots under no speed flag, SPEED and SPEED3 at six epochs,
on the Swiss files and Moshier, in degrees and radians, and finds 0
differences. Before the fix it failed by 1.6e-3 deg.

## 29. A missing Swiss file whose neighbour does not hold the instant leaves the neighbour open, and the Moon is never read: NaN fresh, the last read's Moon warm (this fork's)

**Severity: Correctness (with `semo_12.se1` missing, the Moon NaN with no error on a fresh context, 37 deg off warm; the Sun and every geocentric body 6 arcsec apart fresh and warm)**
**Where:** `sweph.c`: `sweph()`, the neighbour-file lookup (this fork's); the reader it misled is `sweplan()`'s Moon

Upstream has no neighbour lookup: a missing moon file is a closed one, and
the Moon is Moshier's. This fork's `sweph()`, when the file named for the
instant is missing, opens the neighbouring 600-year bucket's file, whose data
runs a little into this bucket, so that a context that had just computed on
the neighbour and a fresh one answer alike. The range check after it refused
an instant the neighbour does not hold, and left the file open. For the
planets and the main asteroids the return value is what the callers look at:
`main_planet()` falls to Moshier with "using Moshier eph.", an asteroid is an
error. `sweplan()` reads the Moon to make the Earth from the EMB and takes
"the moon file is not open" as its sign that there is no moon file and the
Moshier Moon is to be used. With the neighbour open it went on with a Moon it
never read: the save area's, empty (NaN) on a fresh context and the last
read's on a warm one.

Measured with `semo_12.se1` missing, at JD 2159377.5 (1200.0 + 20 days;
`semo_06`'s data ends at +16): the Moon NaN with flags 0x100 and no error
fresh, 26.44 deg warm (after the Sun 1.5 days before 1200.0) where Moshier's
is 349.29; the Sun 307.55655 fresh and 307.55823 warm, 6.0 arcsec apart, and
Mercury and every body whose Earth comes from that Moon likewise; J2000 XYZ
the Moon zeros. The same at every instant of the bucket beyond the
neighbour's overrun, after it or before it (`semo_18` starts 9 days early, so
1800.0 - 20 days with `semo_12` missing). With both the file and its neighbour missing, serr
named the neighbour last tried ("semo_06.se1 not found") for a Moon at 1200.

**Fix, as done here:** a neighbour that does not hold the instant is closed
again, with its bodies' file data, and the instant's own file is reported as
not found: the state and message of a missing file with no neighbour at all,
so `sweplan()` takes the Moshier Moon with "using Moshier eph. for moon" and a
warm context answers as a fresh one. A neighbour that holds the instant still
answers it. Only the open path is touched; no calculation that finds its file
is. G38 `tests/missfile.c`, in directories of links in /tmp without
`semo_12` and without `semo_18`: 16 instants about 600.0, 1200.0 and 1800.0
(inside and past each neighbour's overrun, both sides), nine bodies, four
flag sets with speed, 1152 rows. Every answer is an error or finite; a warm
context equals a fresh one in flags and to 1e-6 AU; an answer is within 60
arcsec of the complete directory's and says "Moshier" in serr when it is more
than 0.5 arcsec from it. With `sepl_12` missing the Sun falls to Moshier, and
with a file and its neighbour missing serr names the file for the instant.
Before the fix 191 rows fail (7 not finite, 72 warm differ, 112 off or
unannounced), after it none.

## 30. A refused Swiss file empties the constants of every file left open, so the next calls fail on healthy files; the path setter crashes on a refused moon file

**Severity: Correctness (calls on other bodies fail "file is damaged" until each open file has been reopened) / Memory (`fclose(NULL)`: the path setter segfaults)**
**Where:** `sweph.c`: `read_const()`'s and `get_new_segment()`'s error exits (upstream); the path setter's moon header read (this fork's)

`read_const()` refuses a file that is not what its name says ("Ephemeris file
name 'seas_06.se1' wrong; rename 'seas_00.se1'"), is damaged or short;
`get_new_segment()` refuses one whose segment cannot be read. Both closed the
file and ran `free_planets()`, which zeroes every body's `plan_data` -- the
constants (`tfstart`, `dseg`, `lndx0`) of the files that stay open as well.
The next call on one of those found its file open, skipped `read_const()`,
and looked up a segment with `dseg` 0: an error, which closes that file and
runs `free_planets()` again.

Measured with `seas_06.se1` a link to `seas_00.se1`, after Ceres at 600.0 +
375 days: the Sun at 600.0 + 400 "sepl_06.se1 is damaged (2)", the Moon there
"semo_06.se1 is damaged (2)", Mars there "sepl_06.se1 is damaged (2)", and
only Mars 1000 days on answered; a fresh context answers all four. With
`sepl_18.se1` a copy of `sepl_12.se1`, after the Sun at 1800.0 - 100 and the
Mercury that met it, the Moon 25 days on failed "22 coefficients instead of
0" and Mars "semo_12.se1 is damaged (2)".

A related crash: the path setter reads `semo_18.se1`'s header (this fork's
replacement of upstream's calculation of the Moon at J2000) and on a refusal
closed the file `read_const()` had already closed, `fclose(NULL)`:
`swe_set_ephe_path()` segfaults with `semo_18.se1` holding `semo_12`'s data.

**Fix, as done here:** a refusal closes and clears only the refused file: its
`fidat` slot and its bodies' file data (the part of `plan_data` before
`teval`), and the slot the numbered asteroids and planetary moons share only
if this file had begun writing it. The other open files and their constants
are left as they were, so nothing healthy is reopened and the calls after it
answer as a fresh context with the same directory does. The general constants
a refused file may have written (`gcdat`) are read by nothing. The path
setter no longer closes the file twice.

G39 `tests/refusedfile.c`, in directories in /tmp of links to the complete
`ephe/` with one fixture each: (a) `seas_06` a copy of `seas_00`, (b)
`seas_00` with Pallas's segment index overwritten, (c) `sepl_18` a copy of
`sepl_12`, (d) `semo_12` a copy of `semo_06`. Each meets its refusal, after
healthy calls that open the other files where the file needs them, and then
answers the later calls (the Sun, Moon, Mars, Ceres) bit for bit as a fresh
context does; (e) `semo_18` a copy of `semo_12`: the path setter survives, in
a child process. Before the fix (a) to (d) fail, (e) crashes.

## 31. Values read from an ephemeris file's header, and from the delta-psi and fixed-star tables, index and size arrays unchecked: a crafted file writes past `pldat[]`, `rot_back()`'s stack array, `sastnam[]` and `dpsi[]`, or reads past the JPL record

**Severity: Memory (stack and heap writes past arrays, NULL dereferences, out-of-bounds reads; a damaged file is mostly stopped by the header CRC, a crafted one is not)**
**Where:** `sweph.c`: `read_const()`, `do_fread()`, `sweph()`'s segment load, `load_dpsi_deps()`, `fixstar_cut_string()`, `search_star_in_list()`; `swejpl.c`: `fsizer()`, `state()`

An audit of every reader that takes a size or an index from a file. Each was
measured with a crafted copy of a real file (CRC recomputed where the reader
checks one), the copy in a /tmp directory of links to the others:

- `read_const()` used the body numbers a Swiss file lists as `pldat[]`
  indexes, checked only against `SE_PLMOON_OFFSET`. 500 in `sepl_12`'s list
  wrote `pldat[500]` of 18; 1 wrote the Moon's constants while `semo_18` was
  open, and the Moon's next segment failed "semo_18.se1 is damaged (2)"; 65000
  wrote the slot a planetary moon's open file uses.
- The coefficient count `ncoe` was unbounded: 60 in `semo_12` overflowed
  `rot_back()`'s `x[MAXORD+1][3]` on the stack (the process died on "stack
  smashing detected"), and `get_new_segment()`'s `longs[]` and `do_fread()`'s
  1000-byte `space[]` have the same size behind them; in `sepm9501` the call
  answered (r 1048832, no error).
- The segment length, index offset and span were used unchecked: Mercury's
  `dseg` 0 gave a segment number of inf converted to int, `lndx0` 0x7fffff00
  overflowed `lndx0 + 3 iseg`, and a span starting 1000 days late answered
  from the wrong segment without an error.
- A body the file does not list (Mercury's entry replaced by Venus) kept the
  constants of no file, `dseg` 0, or those of the file read before.
- A planetary moon's or asteroid's element line opening with 30 digits:
  `strncpy()` copied 19 + 31 bytes into `sastnam[41]`; the 30-byte `astnam`
  was `strlen()`'d without a terminator.
- `swejpl.c`'s `state()` bounded each body's coefficient count by 18 but not
  its offset or sub-interval count, and the file-length identity checks only
  the body with the highest offset: DE200 with the Earth-Moon barycentre's
  sub-intervals 2 -> 402 and the Moon's 8 -> -492 (15 and 12 coefficients, the
  sum kept) read far past `buf[2500]` in `interp()`. `fsizer()` with no
  positive offset read `eh_ipt[-3]`.
- `load_dpsi_deps()`: a line of fewer than ten fields left `cpos[3]`, `[8]` and
  `[9]` unset (a crash in `atoi()`), and an `eop_1962_today.txt` of more than
  `SWE_DATA_DPSI_DEPS` (36525) days wrote past `dpsi[]` and `deps[]`.
- `fixstar_cut_string()` trimmed `cpos[1]` for a `sefstars.txt` line with no
  comma, NULL (a crash in `swi_right_trim()`); the catalogue that then failed
  to load left `search_star_in_list()` calling `bsearch()` on its NULL table.

`seorbel.txt`'s reader writes no fixed array from the file (its name is copied
bounded).

**Fix, as done here:** every such value is checked against what it indexes or
sizes before it is used, and a file that fails is refused alone: closed, its own
slot cleared, the other files kept. In `read_const()`: the
file's span finite and ordered ("damaged (0r)"); every listed body one that kind
of file holds -- the planet file the barycentre, Mercury to the solar-system
barycentre; the Moon file the Moon; the main-asteroid file Chiron to Vesta; a
single-body file a number above `SE_PLMOON_OFFSET` ("0o"); `ncoe` in 1..MAXORD
+ 1 ("0p"); per body the ten constants finite, `dseg` positive, the body's
span covering the file's, at most 1e9 segments, `lndx0` non-negative and its
index inside the file, the elements within 1e8 days and 1e5 deg ("0q"; every
distributed file is far inside: `|prot|`, `|peri|` < 1.1e4, `|dprot|`,
`|dperi|` < 710, `|telem|` < 8e6); the element line's number field within
`sastnam[]` ("0d"); `astnam` terminated. `do_fread()` refuses a reordered read
larger than its buffer. `sweph()` refuses a body its open file does not list
("does not contain body"), checked at a segment load, fresh or warm alike. In
`swejpl.c` every body's offset, coefficient count and sub-interval count must
lie inside the record (NOT_AVAILABLE, and the fallback with its message), and
a file with no offsets is refused. `load_dpsi_deps()` skips a short line and
stops at 36525 days; `fixstar_cut_string()` trims only a field that is there,
and the star search does not call `bsearch()` on an empty table (the answer,
not found, as before).

It refuses only files no valid ephemeris is. Every distributed file -- the 153
sepl/semo/seas files, the 32 planetary-moon files and `de200.eph` --
opens and answers; 2958 fresh calls (the Sun to Pluto, Chiron to Vesta and
the 32 moons' bodies, at four instants in each 600-year bucket from -13200 to
17250, those the files cover) are bit for bit as before. A segment load costs a scan of the file's body list (at most ten).

G40 `tests/hdrbound.c`: the 13 crafted files above (the star line is case F4
of G32), each in a child process, the other kinds of file opened first so
that a write into their slots shows; the call that meets the crafted file
must be refused as the header is read, and the open bodies' next segments,
Mars, and after the JPL and table cases the Swiss Mars and the Sun, must
answer bit for bit as a fresh context on the complete directory. All 13 fail
before the fix. `make hdrbound_asan hdrbound_ubsan` build it under the
sanitizers, where a removed bound is a diagnosed overflow.

## 32. `swe_house_pos()` answers a place in the wrong house: Sripati ('S') 1.0 for all of house 12 and the first half of house 1, the horizon system ('H') counted the other way from the cusps south of the equator and on it

**Severity: Correctness (a house position up to 11.5 houses off in Sripati, at every date and place; the horizon system's mirrored, house k answered as 13 - k, everywhere south of the equator and on it)**
**Where:** `swehouse.c`: `swe_house_pos_r()`, cases 'S' and 'H'

- Sripati's cusps are the middles of Porphyry's houses, so a place at
  Porphyry's position p, 1 .. 13, is at Sripati's p + 0.5, 1.5 .. 13.5, of
  which 13 .. 13.5 is house 1. The function took `hpos += 0.5; if (hpos >
  12) hpos = 1;`: all of house 12 and the first half of house 1 answered
  1.0. At Zurich, UT 2433282.5, a place 1e-4 of the way into house 12
  answered 1.000000 for 12.000003, house 12's middle (Porphyry's cusp 12)
  1.0 for 12.5. The exact-cusp test hid it at the cusps.
- `CalcH()` computes the horizon houses as Campanus's at the co-latitude
  with the ARMC + 180, the co-latitude 90 - lat north of the equator and
  -90 - lat south of it and on it. `swe_house_pos()` turned the place by
  90 - lat everywhere, so south of the equator and on it the houses ran
  the other way from the cusps: at 33.87S the middles of houses 1 .. 12
  answered 12.50, 11.52, 10.52 .. 1.48, on the equator 12.56 .. 1.37, at
  60S alike.

**Fix, as done here:** 'S' wraps by `if (hpos >= 13) hpos -= 12`; 'H' turns
the place by `CalcH()`'s co-latitude, `geolat > 0 ? 90 - geolat : -90 -
geolat`. G41 `tests/hposinhouse.c`: at 47.37N, 33.87S, the equator, 60N and
60S and four dates 1800..2100, in every letter both functions take (A .. Y,
a .. y), each cusp at its number within 1e-7 house (2.8e-8) and the places
1e-6, 0.5 and 1 - 1e-6 of the way through each house in it (248 checks);
Porphyry's cusps at Sripati's middles k + 0.5 within 1e-7 (1.8e-8; 5
checks). Before the fix 21 of the 253 fail: 'S' and 's' at every latitude,
'H' and 'h' at 33.87S, the equator and 60S, and all five middles checks
(0.5 house off). Every other letter passed before.

## 33. `swe_gauquelin_sector()` methods 0 and 1 move with SEFLG_SIDEREAL and with every flag that names the place's frame, form or centre: Mars at 1950, Zurich, Lahiri 2.62 sectors, SEFLG_XYZ 18.3

**Severity: Correctness (a sector, a place on the observer's diurnal circle, moved by flags that do not move the body there: every sidereal request 1.0 to 4.8 sectors off at Zurich, 1800-2100; SEFLG_NONUT, ICRS, J2000, EQUATORIAL, RADIANS, XYZ, HELCTR and BARYCTR up to 18.4 sectors)**
**Where:** `swecl.c`: `swe_gauquelin_sector_r()`, methods 0 and 1

A Gauquelin sector is the share of its semi-arc a body has run since its
rising or setting, its place on its diurnal circle, which no zodiac or
coordinate frame moves. Methods 0 and 1 are `swe_house_pos()` of the body's
ecliptic longitude and latitude in 'G' (swephprg 16.1), which "requires
TROPICAL positions"; iflag is documented as "flag for ephemeris and
SEFLG_TOPOCTR", and methods 2 to 5 pass `swe_rise_trans()` the ephemeris
bits alone. But methods 0 and 1 computed the body with iflag as given and set
the place it answered, as longitude and latitude in degrees, in the frame of
the true ecliptic of date (the ARMC and the true obliquity). Mars at UT
2433282.5, Zurich (8.55E, 47.37N, 400 m), Swiss files, method 0, 2.6855985
plain:
- SEFLG_SIDEREAL (Lahiri) 5.3075799, +2.62 sectors (method 1 +2.69): the
  sidereal longitude set in the tropical frame, the body moved back by the
  ayanamsa; up to 3.42 sectors over Mars, the Moon, Spica and the mean node
  at six dates 1800..2100, in every sidereal setting and on every ephemeris;
- SEFLG_NONUT -1.14e-4 (the mean place in the true frame), SEFLG_ICRS
  -1.5e-8, SEFLG_J2000 -0.0875, SEFLG_EQUATORIAL -0.148 (right ascension
  and declination taken for longitude and latitude), SEFLG_RADIANS +17.8,
  SEFLG_XYZ +18.3 (method 1 +18.4);
- SEFLG_HELCTR and SEFLG_BARYCTR +3.88: a direction from the Sun or the
  barycentre set on the observer's sky.

**Fix, as done here:** methods 0 and 1 compute the place with the flags that
choose the body's direction from the observer and no other: the ephemeris
and its Horizons corrections, SEFLG_TOPOCTR, TRUEPOS, NOABERR, NOGDEFL and
CENTER_BODY. The others are ignored, as methods 2 to 5 ignore them, not
refused. A sidereal place takes no Horizons corrections, so with
SEFLG_SIDEREAL neither the place nor the frame takes them, and every method
answers the sector of the call without SEFLG_SIDEREAL. G42
`tests/gauqflags.c`, at Zurich, six dates 1800..2100, on de200, the Swiss
files and Moshier: (a) with SEFLG_SIDEREAL (five sidereal settings, the
sidereal call also with SEFLG_JPLHOR_APPROX and SEFLG_JPLHOR) the call
without it, bit for bit, methods 0 to 5 (42 checks); (b) the sidereal method
1 sector k lies between the sidereal Gauquelin cusps k and k + 1 (9); (c)
methods 0 and 1 with each frame, form or centre flag on six bases of
direction flags as the base alone, bit for bit (66); (d) on each base the
sector is `swe_house_pos()` of the place it gives in the true frame of date
within 1e-11 (7.1e-15; 15); (e) methods 2 to 5 with each flag as without (4).
Before the fix the sidereal call with SEFLG_JPLHOR crashed (the frame's
nutation reads the IERS table, absent); without that case 99 of 136 fail:
30 of (a) (up to 3.42 sectors), all 9 of (b) (up to 23.0 deg outside), 60 of
(c) (up to 18.0 sectors; SPEED, SPEED3 and ICRS on de200 pass). The golden
transcript's sector rows do not move.

## 34. Sidereal houses on the ecliptic of t0 and the solar system plane: ascmc[2] the auxiliary ARMC, whole signs 8.94 deg off the sidereal signs, a step of 2 dpsi at t0, ascmc[8] moved by up to 359 deg

**Severity: Correctness (every house call with SE_SIDBIT_ECL_T0 or SE_SIDBIT_SSY_PLANE, and with the J2000, J1900 and B1950 modes, which always take the first: ascmc[2] 0.0043 to 3.90 deg off; whole signs up to 20.3 deg off; every cusp and ascmc 2 |dpsi| off for some 130 days on one side of t0; ascmc[8] nonzero)**
**Where:** `swehouse.c`: `sidereal_houses_ecl_t0()`, `sidereal_houses_ssypl()`

These houses are computed by `swe_houses_armc_ex2_r()` in an auxiliary
frame, the true equator of date and the sidereal plane cut at the
auxiliary vernal point, at the auxiliary ARMC (armc - dvpx), and moved to
the sidereal plane by that point's sidereal longitude (dvpxe + ayan_t0, less
the plane's zero point on the solar system plane). Measured at Zurich
(47.37N, 8.55E), UT 2378496.5, 2433282.5, 2451545.0 and 2488069.5:

- ascmc[2], documented as the ARMC (swephprg 15.4), was the auxiliary ARMC
  the loop skipped: 4.27e-3 deg off the tropical call's on the user's
  ecliptic of 1900 (t0 2415020.0) and J1900, 6.55e-3 on Lahiri's ecliptic of
  t0, 8.51e-3 J2000, 3.904 on the solar system plane. Given it,
  `swe_house_pos()` put the sidereal cusps' places (carried to the ecliptic
  of date) off their numbers in the systems the manual says work there: on
  the ecliptic of 1900 X 1.42e-4 house to H 3.10e-4, on the solar system
  plane P 0.184 to C 0.339.
- Whole signs ('W', 'w') were the auxiliary frame's signs moved by the
  frames' offset: 30 deg apart but 8.94 deg off a sidereal sign start on the
  ecliptic of 1900 and Lahiri's, 2.80 J1900 and J2000, 20.3 on the solar
  system plane, and their speeds the Ascendant's and MC's, 621 to 650
  deg/day. 0 Aries was set for `hsys == 'N'` only, so 'n' was moved off it,
  by up to 25.3 deg.
- dvpxe was `acos()`, 0 .. 180, negative before t0. It is about p (t - t0)
  + dpsi, so for some 130 days on one side of t0 (where p (t - t0) and dpsi
  differ in sign) its sign was wrong and every cusp and ascmc 2 |dvpxe|
  off, a step of 2 dpsi at t0: the cusps against a construction by vectors
  9.64e-3 deg off just before t0 on the ecliptic of 1900 and J1900, 7.74e-3
  just after on J2000's, 9.32e-3 before on Lahiri's.
- The loops that move ascmc ran `i <= SE_NASCMC` and moved ascmc[8], which
  the manual gives as 0, by the frames' offset: at UT 2433282.5 336.84 deg
  on the ecliptic of 1900 and Lahiri's, 359.30 J1900, 0.70 J2000, 340.40 on
  the solar system plane.

**Fix, as done here:** ascmc[2] is the ARMC the function was given (its speed
was the ARMC's already). Whole signs are computed as equal houses in the
auxiliary frame and moved; cusp 1 is then floored to its sign, cusp k is
cusp 1 + 30 (k - 1), their speeds 0. 0 Aries is set for `toupper(hsys) ==
'N'`. dvpxe is the signed angle from the auxiliary vernal point to t0's
about the pole of the ecliptic of t0, by `atan2()`, continuous through t0.
The two loops run `i < SE_NASCMC`. G43 `tests/sidplanehouses.c`, at Zurich:
(a) six settings (the user's ecliptic of 1900 and solar system plane,
Lahiri's both, J1900, J2000), four dates, ascmc[2] and its speed the
tropical call's bit for bit, and for the user settings the carried
sidereal cusps given ascmc[2] at their numbers in P R C H X within 1e-7
house (9.26e-9; 16 checks); (b) 'W' 'w' 'N' 'n' 'E' 'A' 'D' 'V' there at
their definitions within 1e-9 deg (1.1e-13), the whole signs' speeds 0
(48); (c) the ecliptic of 1900, J1900, J2000 and Lahiri's at TT t0 -+ 2^-20
.. 200 days, Regiomontanus and Campanus cusps against the vector
construction within 1e-11 deg (2.3e-13; 4); (d) ascmc[8] and its speed 0,
ascmc[9] the tropical call's, in 'P' 'I' 'G' 'W' (6). Before the fix 44 of
the 74 fail: all 16 of (a), 18 of (b) ('W', 'w' and 'n' in each setting),
all of (c) and (d). The golden transcript's cov:hsys_sid[eclt0] row (Lahiri,
J2000) moves by 5.0e-13 deg, the `acos()`'s rounding.

## 35. Kronos' built-in semi-axis has two digits swapped, the Kepler solver's starting guess is wrong, and the nakshatras of `swe_split_deg()` are shifted for `SE_SIDM_USER`

**Severity: Correctness (Kronos 0.0086 deg at 1700, 0.0169 deg at 2300 without `seorbel.txt`; every nakshatra split under a user-defined ayanamsa 3 deg 20' early; the starting guess moves no result beyond rounding)**
**Where:** `swemplan.c`: `plan_oscu_elem[]` (the Neely block), `swi_osc_el_plan()`; `swephlib.c`: `split_deg_nakshatra()`; `sweph.c`: the "call `swe_set_ephe_path()`" warning of `swi_get_ayanamsa_ex()`

Three small defects in three functions, one entry.

Without `seorbel.txt` on the path the fictitious bodies come from a
built-in table. Its Neely row for Kronos has the semi-axis 64.81960;
`seorbel.txt` and Witte's original 64.816896 (the non-Neely rows of the same
table) give 64.81690, and the other fourteen built-in rows agree with the
file to the last digit, so the value is a swap of two digits. Kronos'
mean motion was off by 0.0043 deg per century from the 1900 epoch:
-0.0086 deg at 1700, +0.0169 deg at 2300, 0.0257 deg and 2.8e-3 AU at 2500.

The solver of `swi_osc_el_plan()` for eccentricities above 0.975 (Nibiru,
the Christ Comet) builds a starting guess near perihelion with
`zeta = pow(x, 1/3)`, where `1/3` is integer division, so `zeta = pow(x, 0)
= 1`, and adds 180, not pi, to an angle in radians. `swi_kepler()` converges
to the same root from either guess, so a result moves only by rounding:
over 20000 dates 1700 to 2500 (Moshier, geocentric, with speed) Nibiru's
position by 4.3e-14 deg and 3.4e-13 AU, the Christ Comet's by 1.4e-14 deg
and 1.1e-12 AU; the speeds by 4e-8 deg/day, which is that rounding read
through the 5e-7 day step of the deflection's differenced speed (6e-11
deg/day with `SEFLG_NOGDEFL`).

`split_deg_nakshatra()` moves the place by 3 deg 20' for the Sheoran
"Vedic" ayanamsa (its 0 Aries is 3 deg 20' of Ashvini), and tested the mode
as `(sid_mode & SE_SIDM_TRUE_SHEORAN) == SE_SIDM_TRUE_SHEORAN`, a bit mask
on a mode number. 39 is binary 100111; `SE_SIDM_USER`, 255, has every one
of those bits, with or without SIDBITs above it. Of the stored modes only
Sheoran and USER pass. `swe_split_deg(10, SE_SPLIT_DEG_NAKSHATRA)` gave
Ashvini 10 deg 00' under Fagan/Bradley and Lahiri and Bharani 0 deg 00'
under 255, 1279 and 2303. The warning in `swi_get_ayanamsa_ex()` tested
`sip->sid_mode == SE_SIDM_TRUE_SHEORAN` where the rest of its list tests the
masked `sid_mode`; that changes nothing, since `swe_set_sid_mode()` stores a
star mode without its SIDBITs, and is made the masked one for the list's
sake. Every other comparison of a mode number takes `sid_mode %
SE_SIDBITS` first.

**Fix, as done here:** the semi-axis is 64.81690. The guess is
`pow(x, 1.0 / 3)` and pi. The Sheoran shift tests the mode itself,
`sid_mode % SE_SIDBITS == SE_SIDM_TRUE_SHEORAN`. G44 `tests/fictnakshatra.c`:
the 15 built-in bodies at five dates 1700..2500, heliocentric Moshier, with
the shipped `seorbel.txt` against an empty directory: 3.6e-15 deg, 5.7e-14
AU (Kronos 0.0257 deg at 2500 before); and 5 deg and 10 deg split into
nakshatras under the 47 predefined modes, USER alone and with four SIDBITs,
Sheoran with a SIDBIT and Lahiri with four: 114 checks pass, the ten USER
ones failed before. The starting guess is not gated: no public result moves
beyond rounding.

## 36. The heliacal memos answer for an atmosphere, latitude or star catalogue they do not hold

**Severity: Correctness (returned values: 0.67 mag in a limiting magnitude, 0.46 mag in a star's, a day in a heliacal event)**
**Where:** `swehel.c`: `call_swe_fixstar_mag()`, `kOZ()`, `ka()`, `Deltam()`; `sweph.c`: `swi_close_keep_topo_etc()`, `ctx_release()`, the star functions' memos and catalogue; `sweconfig.c`: `swi_config_apply()`; `sweph.h`: `struct hel_state`

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
humidity 80 % 0.25. Nothing emptied them, so the dark-sky answer after
`swe_close()` was still the one before it (0.26 mag).

`call_swe_fixstar_mag()`'s memo, keyed on the star's name, was not emptied
by `swe_close()` or a new ephemeris path, so a star kept the magnitude of
the catalogue it was first read from: Sirius after `swe_close()` and a path
whose `sefstars.txt` gives it -1.00 was -1.46 in `swe_vis_limit_mag()`'s
`dret[7]` and `swe_heliacal_pheno_ut()`'s `darr[20]`, and
`swe_heliacal_ut()`'s end of visibility of its heliacal rising came a day
early (2451778.67 for 2451779.67). It took the name before the lookup, so
a failed lookup left the name keyed to the previous star's magnitude, and
an empty name was a hit on the empty memo; and a hit left `serr` as it was
where the lookup clears it, so a warning from `ka()` reached
`swe_vis_limit_mag()`'s `serr` on the second call only.

The star functions of `sweph.c` under it kept memos of their own:
`swe_fixstar()`'s and `swe_fixstar_mag()`'s last record (`sp.fs1`,
`sp.fs1mag`), `swe_fixstar2()`'s and `swe_fixstar2_mag()`'s last star
(`sp.fs2`, `sp.fs2mag`), and the catalogue `swe_fixstar2()` reads into
memory, which `swe_close()` freed only when it was complete and a path
change not at all. None of them was emptied by `swe_close()` or a path to
another `sefstars.txt`, nor by a thread adopting another thread's path. With
a second catalogue giving Sirius -1.00 mag and 300 mas, all four functions
still gave -1.46 mag and 2.64 pc (543932 AU) for -1.00 and 3.33 pc (687549
AU) after `swe_close()` and the path to it, and after a path change alone.
That also refilled `swehel.c`'s memo with the old magnitude.

`kOZ()`, `ka()` and `Deltam()` took a key of 0 as empty, and a hit dropped
the warning `ka()` writes for an over-long visual range or an extinction
coefficient too small for the other terms. The key of 0 is not reachable
through the public functions: `SunRA()` is never exactly 0. `SunRA()`'s own
memo, keyed on UT, is complete: `SIMULATE_VICTORVB` (always defined) makes
it the monthly approximation, a function of UT alone.

**Fix, as done here:** each memo is keyed on every input of its result, has
a `valid` flag for empty, and keeps what its computation wrote to `serr`
-- `ka()`'s warning (by number, also in `Deltam()`'s memo, which reaches
`ka()` on both its branches), the star lookup's `serr` -- to write it again
on a hit. The star's memo is filled on success only. `swi_hel_forget()`
empties the four; `swe_close()` and `swe_ctx_free()` (`ctx_release()`),
every path change (`swi_close_keep_topo_etc()`) and an adopted path
(`swi_config_apply()`, which also sees a changed JPL file) call it. No
setter feeds the extinction memos, now that their keys are complete.
`swi_fixstar_forget()`, called from the same three places, empties the four
star memos, frees the catalogue and closes the star file, so the next call
reads the one on the new path.

G45 `tests/helmemo.c`: one sequence of 37 `swe_vis_limit_mag()`,
`swe_heliacal_pheno_ut()` and `swe_heliacal_ut()` requests alternating the
atmosphere, eye height, latitude in the dark sky, the high-precision flag,
the ephemeris (Swiss, Moshier, de200), a user delta T, `swe_close()` and
two star catalogues, each answer -- return code, every value, `serr` --
against the same request in a fresh process; all agree within 1e-9. 17 of
the 37 failed before. The same test asks the four star functions for Sirius
under the two catalogues, switched by `swe_close()`, by a path alone, and in
a thread that adopts another thread's path: 24 checks of magnitude and
distance pass, 12 failed without the emptying.

## 37. The default nutation, full IAU 2000A, costs 12 to 100 times IAU 2000B's when the calls are a day or more apart (this fork's)

**Severity: Performance (a Sun or Moon position with speed 270 to 770 us at daily steps and 470 to 650 us at 30-day steps, against 11 to 24 us under IAU 2000B; `swe_calc(SE_ECL_NUT)` 250 to 390 us against 4 us)**
**Where:** `swephlib.c`: `calc_nutation_grid()`

Full IAU 2000A costs about 14 times IAU 2000B's per instant (64 us against
4 us, Moshier, -O2), so the default evaluated it only at nodes a quarter day
apart, cached, and took a quintic Lagrange polynomial through the six nodes
around the instant. Calls within a few hours share their nodes and were
measured that way. An instant a day or more from the last one shares none and
evaluated the series six times, so a search stepping by days (returns,
ingresses, transits, stations) or a list of unrelated charts paid for it on
every call; reported downstream as an astromcp return search that took 0.6 s
on ts.14 and 6.7 s on ts.19. The Sun with speed as a multiple of the same
call under IAU 2000B, in one process: hourly 1.6, daily 36, 30-day steps 34,
scattered over 1800-2100 33; the Moon 1.3, 12, 20, 17.

**Fix, as done here:** the series is evaluated with each term's sine and
cosine built as products of e^(ik x) powers over its nonzero argument
multipliers (`swenut2000a_sparse.h`, generated from `swenut2000a.h` by
`tools/gen_nut_sparse.py`), together with its first three time derivatives:
23 us an evaluation against 64. The nodes are a day apart, each holding the
value and the three derivatives, and the septic Hermite polynomial on the two
nodes around the instant gives the value: no new node while calls stay
between the same two, one at daily steps, two at a fresh instant. The value
still depends on the instant alone, never on what the cache held, and is
nearer the direct series than before: worst 0.0014 uas in longitude and
0.0006 in obliquity over 1800-2100 and -13000..15000, against 0.018 and
0.008. Positions move by up to 5e-12 deg and speeds by up to 2.4e-10 deg/day;
the osculating apogee, derived from differenced positions, by 2.9e-11 deg
and 9.5e-8 deg/day.
Now the Sun with speed is 0.65 of IAU 2000B's cost hourly, 2.6 daily, 6.5 at
30-day steps and 4.4 scattered (the Moon 0.82, 1.8, 3.7, 2.7); a fresh
instant still costs two nodes of the full series, which 2000B's 77 terms
never did. G46 `tests/nutspeed.c` checks the generated table against
`nls[]` and `npl[]`, and the Sun with speed against IAU 2000B at hourly,
daily and scattered spacings (rounds interleaved, best of fifteen, limits
1.5, 5 and 8): 0.66, 2.6 and 4.4 now, 1.7, 24 and 33 before the fix.

Past 4e8 days (1.1 million years) the day grid's fit leaves the series fast
(3.4 deg at 3e9 days, where the old quarter-day grid was 0.75), so the series
itself is taken there, as for any date the grid does not cover.

## 38. `calc_dip()` is NaN below sea level: `swe_rise_trans_true_hor()` with horhgt -100 answers the time two hours after the start, `swe_refrac_extended()` converts no apparent altitude to a true one, `swe_azalt()` refracts a body below the horizon

**Severity: Correctness (a wrong time returned as a success; the refraction missing from an apparent altitude, 0.58 deg at the horizon; any height from -500 m to just below 0, the range the rise, azalt and eclipse functions accept)**
**Where:** `swecl.c:3158` (upstream's), `calc_dip()`; reached from `swe_refrac_extended()` and `swe_rise_trans_true_hor()`

The dip of the horizon is `-acos(1 / (1 + h / R)) * sqrt(d)`. For h < 0
the argument is above 1 and the dip NaN, though `SEI_ECL_GEOALT_MIN` is
-500 m and the Dead Sea shore is at -430. swephprg documents nothing for it:
it describes `swe_refrac_extended()` "for altitudes above sea > 0", and
horhgt -100, the horizon at the dip, is in no document. At -10 m (the
Netherlands), -430 m (the Dead Sea) and -86 m (Death Valley), 1013.25 hPa,
10 C, 2026-10-25:
- `swe_rise_trans_true_hor()` with horhgt -100, the Sun and the Moon, rising
  and setting, 12 of 12: success, with the time 0.0833 d (two hours) after
  the start, 0.8 to 22.5 hours from the rising or setting;
- `swe_refrac_extended()`: dret[3] NaN; an apparent altitude is never
  converted (`inalt >= NaN` is false), 0 deg answering 0 where sea level
  answers -0.57634, and 10 deg 10 where it answers 9.91100; a true altitude
  below the horizon is refracted (-3 answering -1.77881, sea level -3);
- `swe_azalt()`: the same refracted apparent altitude for a body below the
  horizon;
- `swe_rise_trans()`, with its default flags and with every SE_BIT_ option,
  does not change: its horizon is 0, and the apparent altitude crosses 0 at
  the same instant with or without the cut below the dip.

**Fix, as done here:** below sea level there is no sea horizon; the terrain
around is the horizon, which only `swe_rise_trans_true_hor()` can be told.
`calc_dip()` takes a negative height as 0, the geometric horizon, as at sea
level. The dip is continuous there (it grows as the square root of the
height) and unchanged above it. G47 `tests/belowsea.c`, with the five heights
-500, -430, -86, -10 and -1e-6 m: (a) `swe_refrac_extended()` in both
directions at 13 altitudes, the answer and dret[0..3] those of 0 m bit for
bit, the dip 0, and the dips at 1, 10 and 100 m as before (278 checks);
(b) horhgt -100, the Sun and the Moon, rising and setting, at four sites
(-10, -430, -86 and -500 m): found, within 2 days after the start, the time
of horhgt 0.0001 to 1e-9 day, and within 1 s of sea level's (0.054 s worst;
64); (c) `swe_azalt_r()` over a day, +60 to -60 deg, as at 0 m bit for bit, and
finite with the pressure estimated (241). Before the fix 232 of the 583 fail
(190 of (a), 32 of (b), 10 of (c)).

## 39. The topocentric observer is placed about the mean pole of date

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
angle is 200-270 m (G48: 15-277 m at five sites and five instants
1800-2100), and 200-270 m seen from the Moon is **0.10-0.15"**: measured
against Ephemeris Prometheia on the same delta t and against JPL Horizons
at six sites and six instants 1800-2100, 10-148 mas, every one of them
explained by the site offset to about 1 mas (2026-10-24). Planets: under
1 mas. The function also neglected the frame bias ("displacement of
45cm"), though the Earth the site is added to is ICRS for DE403 and later
and the callers take the sum back to J2000 with the forward bias: 0.3 mas
on the Moon.

Found comparing the topocentric Moon with Prometheia and Horizons; the
recipe above reproduces Swiss's observer to the centimetre from its own
sidereal time, obliquity and nutation, and the true-pole recipe reproduces
theirs.

**Fix, as done here:** the observer is built on the true equator and
equinox of date whatever frame the caller wants its answer in: the
apparent sidereal time from the true obliquity and the nutation in
longitude, both computed at this instant rather than taken from the caches,
the site vector and its velocity rotated by the transposed nutation matrix
to the mean equator of date, precessed to J2000, and then, when the
ephemeris is ICRS (`swi_get_denum() >= 403`), taken to ICRS with the
backward frame bias. `SEFLG_NONUT` no longer says where the observer is.
G48 `tests/topopole.c`: the geocentric minus the topocentric GEOMETRIC Moon
(`SEFLG_TRUEPOS`, so no light time enters) in the true equator of date is
the observer the library used; it must equal the site built from the
geodetic coordinates, the library's own delta t, true obliquity and
nutation, and `swe_sidtime0()`, to 5 cm (0.0000 m measured; the unfixed
library misses on all 25 rows, worst 276.6 m). Moshier, no files. The
`topd` cache is unchanged: it is keyed by instant and invalidated with delta
t (entry 18), and the new vector goes through the same `do_save` path.

## 40. A heliocentric light time is measured from the barycentre on its second pass

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
Vulcan at J2000 (G49), whose light time is 0.00078 d at 19 degrees a day.
`app_pos_etc_orbel()`, which `swe_calc_orbel()` runs, was written from it
and has the same two skips.

Found comparing heliocentric Mercury with Horizons and Prometheia; the
recipe above reproduces Swiss's number from its own barycentric positions
to roundoff, and the corrected recipe reproduces theirs.

**Fix, as done here:** the loops measure from `xobs` for every centre. In
`app_pos_etc_plan()` `xobs` is set for a barycentric request (zero) and a
heliocentric one (the barycentric Sun from the files; zero under Moshier,
whose stored positions are heliocentric), and the iteration starts from the
barycentric vector `xx0` so that every pass measures the same quantity. The
same two skips are removed from `app_pos_etc_plan_osc()` and
`app_pos_etc_orbel()`, whose `xobs` already is the centre. No new reads of
the ephemeris. G49 `tests/hellt.c`: the heliocentric astrometric direction
of Mercury to Pluto under the Swiss files and de200.eph, and of a fictitious
body from an element set written by the test, equals B(t - tau) - S(t)
built from this library's own geometric barycentric positions, to 0.0001"
(0.000000" measured; the unfixed library misses on all 51 rows, worst
2.608").

## 41. `swe_get_ayanamsa_ex()`, its `_ut` form, the sidereal houses and `swe_gauquelin_sector()` with SEFLG_JPLHOR and no IERS corrections loaded read the corrections' table, NULL, and crash

**Severity: Crash (a NULL dereference; with an eop_1962_today.txt that breaks off, a read 2.45e6 doubles before the table)**
**Where:** `swephlib.c`: `calc_nutation()`; `swecl.c`: `swe_gauquelin_sector_r_body()`

`plaus_iflag()` turns SEFLG_JPLHOR into SEFLG_JPLHOR_APPROX when no
corrections are loaded (`eop_dpsi_loaded` <= 0: no `swe_set_jpl_file()` of a
DE >= 403 on the context, no eop_1962_today.txt, or one the loader gave up
on), so `swe_calc()` never asks the nutation for them without them. Three
public functions take their flags past it. `swe_get_ayanamsa_ex()` adds the
nutation of the caller's flags to the ayanamsa; `swe_get_ayanamsa_ex_ut()`
calls it, and so do the sidereal houses. `swe_gauquelin_sector()`'s tropical
methods 0 and 1 set the obliquity and the nutation from the caller's flags
before the body's own `swe_calc()`. With SEFLG_JPLHOR and no corrections
`calc_nutation()` called `bessel()` on `ctx->dpsi`: NULL (SIGSEGV), or, after
a file with a day missing (`eop_dpsi_loaded` -2: the table allocated, its end
never set), at index n - 1 = -2451545.

Measured with Lahiri on de200 at J2000: eight probes crash, with SEFLG_JPLEPH
| SEFLG_JPLHOR and with SEFLG_SWIEPH | SEFLG_JPLHOR, on the setter's
thread, on a second thread on the default context and on an explicit context
given the path alone; and on a synthetic DE431 header at 2000 Jan 7 in a
directory without the file (-1) and in one whose file has a day missing (-2).
Every other public function that takes flags was called the same way and
answers.

**Fix, as done here:** `swi_jplhor_plaus()` takes SEFLG_JPLHOR as
`plaus_iflag()` does, SEFLG_JPLHOR_APPROX while `eop_dpsi_loaded` <= 0.
`calc_nutation()`, the only reader of the table, takes its flags through it,
so every route is covered; the Gauquelin sectors' obliquity takes the flags
the body's place is computed with through it as well, so the frame and the
place agree. Each call answers as the same call with SEFLG_JPLHOR_APPROX, the
warning in serr the one `swe_calc()` writes. The Horizons bits the sidereal
sectors drop (entry 33) stay dropped. G50 `tests/eopfollow.c`: the eight
probes (the ayanamsa and its `_ut` form, the ayanamsa on SEFLG_SWIEPH,
sidereal Placidus and Koch houses, Gauquelin methods 0 and 1 for Mars and
method 0 for Spica) in six settings without corrections (de200, the same with
the nutation interpolated, a second thread, an explicit context, and the two
fixture directories), each call in a forked process returning with no signal,
bit for bit with its return flag as the same call with SEFLG_JPLHOR_APPROX in
a fresh one: 48 checks. Before the fix 46 crash (the interpolated houses pass,
their ayanamsa's nutation interpolated from nodes the houses' own nutation
without flags had built), after it none fail.

## 42. The refusals of a date outside a range write its digits in full into 256 bytes: from |J| about 1e188 days the message overruns its buffer or the caller's serr

**Severity: Memory (stack buffer and serr overrun; glibc's fortify aborts at -O2, the stack protector at -O0)**
**Where:** `swemplan.c`: `swi_moshplan()`; `swemmoon.c`: `swi_moshmoon()`, `swi_mean_node()`, `swi_mean_apog()`; `swejpl.c`: `state()`; `swedate.c`: `swe_utc_to_jd()`; `swecl.c`: the eclipse functions' "no ... eclipse at tjd"

Every refusal of a date outside a body's range printed the date by `%f`
with `sprintf()`: "jd %f outside Moshier planet range %.2f .. %.2f " into a
local `char s[AS_MAXCH]` (256 bytes) in `swi_moshplan()`, `swi_moshmoon()`,
`swi_mean_node()` and `swi_mean_apog()`, copied into serr after, and "jd %f
outside JPL eph. range" straight into the caller's serr. `%f` writes every
integer digit, so the message is 256 characters or more (with its NUL past
256 bytes) from |J| 1e188 (the Moshier planets and Moon), 1e191 (the mean
apogee), 1e193 (the mean node) and 1e196 (JPL; 360 characters at 1e300).
Measured, each call in a child process, |J| 1e200, 1e250, 1e300, 1e308 and
DBL_MAX either sign, Mars, the Moon, the mean node and apogee and Cupido on
Moshier, `swe_nod_aps()` of the Moon's and Mars's mean points, Spica by
`swe_fixstar2()` on Moshier and Mars on de200.eph: the 80 calls on Moshier
died ("stack smashing detected"), the 10 on de200.eph wrote past the 256
bytes of serr.
`swe_utc_to_jd()`'s "invalid time: %d:%d:%.2f" took the caller's seconds the
same way, past serr from 1e232. The eclipse functions' "no solar eclipse at
tjd = %f" and the JPL reader's read and header errors would overrun likewise,
behind range tests that refuse such a date first.

**Fix, as done here:** `swi_jd_str()` prints the date: `%f` with the
message's decimals below 1e15 days, byte for byte what the messages had, and
`%.10g` from there up and for NaN and infinity ("jd 1e+300 outside Moshier
planet range 625000.50 .. 2818000.50 "), at most 32 characters, so the range
stays in the message. The Moshier, mean node, mean apogee and JPL range
refusals and `swe_utc_to_jd()`'s two "invalid time" messages print the date
(the seconds) by it, into `snprintf()` with the buffer's size; the messages
that sit behind range tests are `snprintf()` with AS_MAXCH, the format
unchanged. G51 `tests/datemsg.c`: the 90 calls above and `swe_utc_to_jd()` at
those seconds, each in a child process: ERR, a message from "jd " (or
"invalid time") of under 100 characters that names the range and the date
as "e+", the bytes past AS_MAXCH untouched; and four dates a little outside a
range, printed as before: 104 checks. Before the fix 100 fail, after it none.

## 43. `swe_sidtime()` reads a nutation `calc_nutation()` left unwritten, and SE_ECL_NUT is taken for a body: the rise and set, the Gauquelin sectors, the occultations, `swe_calc_pctr()` and `swe_helio_cross()` answer it, two of them from `pla_diam[-1]`, and the crossing does not end

**Severity: Memory (an uninitialised read; an out-of-bounds read, `pla_diam[-1]`); Correctness (an answer for a number that is no body, a hang)**
**Where:** `swephlib.c`: `swe_sidtime_r_body()`; `swecl.c`: `swe_rise_trans_true_hor_r_body()`, `swe_gauquelin_sector_r_body()`, `swe_lun_occult_where_r_body()`, `swe_lun_occult_when_glob_r_body()`, `swe_lun_occult_when_loc_r_body()`; `sweph.c`: `swe_calc_pctr_r_body()`, `swe_helio_cross_r_body()`, `swe_helio_cross_ut_r_body()`

`calc_nutation()` writes nothing for a nutation model it does not know (the
caller keeps what it passed in), and `swe_sidtime()` passed it an
uninitialised local pair and used both elements: its sidereal time was
whatever the stack held. gcc -O3 with -Wall -Wextra -Werror rejects the read
as maybe-uninitialized; at -O0 the slot held the bits of a pointer, a
denormal, and the answer was right by luck.

SE_ECL_NUT (-1) is a `swe_calc()` number, the obliquities and the nutation,
not a body; `swe_pheno()`, `swe_nod_aps()`, `swe_get_orbital_elements()`
and `swe_orbit_max_min_true_distance()` refuse it, and `swe_calc()` refuses
every other negative number. The functions that take a body to a horizon or
a centre did not. At UT 2433282.5, Zurich, Swiss files: `swe_rise_trans()`
and `_true_hor()` answered a rise, a set and transits of the angles taken
for a right ascension and declination, and a rise of the disc's edge read
`pla_diam[-1]`; `swe_gauquelin_sector()` 16.6929 (method 0) and 17.8208 (1),
methods 2 to 5 through the same rise; `swe_calc_pctr()` answered it as the
body or the centre; the three occultation functions set every negative
number without a star to the Sun and answered its eclipses
(`swe_lun_occult_when_glob()` 10 at 2433359.147); `swe_helio_cross()` and its
`_ut` form took the obliquity and its 0.0023 deg/day for a longitude and its
speed, and the search, with no bound on its steps, did not end in 5 s.

**Fix, as done here:** a negative number with no star is refused before any
calculation by the rise and set, the Gauquelin sectors (every method), the
three occultation functions (a star's ipl is not read, and still maps to 0),
`swe_calc_pctr()` (as the body or the centre) and the two crossings: ERR,
"illegal planet number -1." (the crossings: "not possible for object -1").
The sidereal time's pair starts at zero, no nutation, for a model without
one. The `pla_diam[]` reads of the eclipse and rise code are reached by
non-negative numbers only. G52 `tests/eclnutbody.c`: -1, -2 and -100 in the
twelve calls above (the crossings and `swe_calc_pctr()` in child processes,
the crossings with a 5 s alarm): 36 checks; a star with ipl -1 the star as
with ipl 0, and the Sun in the occultations still the eclipse of 1999 Aug
11: 3; the sidereal time under an unknown nutation model the same after two
dirtyings of the stack and that of no nutation: 2; and `make` compiles
`swephlib.c` at -O3 with -Werror. Before the fix 18 of the 41 fail and the
-O3 compile, after it none.

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
