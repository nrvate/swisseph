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

**Not changed: the delta-t coupling itself.** Away from the boundaries the
0.041" per second remains. Against the IERS 2010 expression, the default is
+37" at 1000, +328" at year 0 and +2382" (0.66 deg) at -3000, of the
order of 0.041" times delta t (60", 434", 3082"; the rest is the two
precession treatments), where sidereal time at a given UT1 is set by the
Earth's rotation, not the Sun. Removing it would move every house before
1850 by tens of arcseconds at 1500 and by most of a degree of RAMC in
antiquity, a departure from upstream that every historical chart would
see, so it is left as a decision rather than made here.
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
