# Swiss Ephemeris — thread-safe fork

A fork of [Swiss Ephemeris](https://github.com/aloistr/swisseph) that can be
used from more than one thread, with the same numbers and the same ABI.

[**Latest release**](https://github.com/nrvate/swisseph/releases/latest) —
prebuilt for Linux, macOS, Windows and Android. Versions are `2.10.03-ts.N`:
upstream's version, plus this fork's own counter, so a tag here can never be
confused with one of upstream's.

(The version is deliberately not written out on this page. `SE_VERSION` in
`sweph.h` is the only place it lives, and a gate enforces that against every
tracked file — this one included, as an earlier draft of this paragraph
discovered.)

---

## Why this fork exists

Upstream is not "not thread safe" in the usual sense. Since v2.03 it annotates
the global `swed` with `TLS`, which on Linux/GCC genuinely eliminates data
races. The result is **race-free but not thread-usable**: every thread gets its
own private copy of the configuration, so

```c
swe_set_ephe_path("/usr/share/ephe");   /* main thread */
```

is invisible to every worker thread. They silently fall back to the built-in
Moshier ephemeris — lower precision, different answers — and still report
success. Nothing errors and nothing warns; the positions are just quietly worse.

On platforms where `TLS` expanded to nothing, the failure inverted into real
races over 23 KB of shared caches and open `FILE *` handles.

This fork fixes both halves: configuration propagates between threads, and
threads can hold genuinely independent state when they need to.

---

## Getting it

**Prebuilt**, from [the latest release](https://github.com/nrvate/swisseph/releases/latest) —
Linux (`libswe.so`, `libswe.a`, CLI tools), macOS (universal arm64 + x86_64),
Windows (32- and 64-bit DLLs, import libraries, static libraries), and Android
(JNI library for every ABI the NDK builds). Every archive carries a
`SHA256SUMS`; the release carries one for the archives themselves.

Nothing is checked into this repository — the binaries are built by CI from the
tag and only exist in releases.

**From source:**

```sh
make                    # -std=c17 -Wall -Wextra -Werror -O2
make libswe.so          # or libswe.dylib on macOS
make LTO=0              # without link-time optimisation, see THREADING.md
```

Ephemeris data files are **not** included and have their own release cadence —
see [Ephemeris data files](#ephemeris-data-files) below. Without them a request
for the Swiss or JPL ephemeris is an **error**, not a quiet substitution; the
built-in Moshier model is still there for callers who ask for it by name.

---

## Using it

There are two ways, and they compose. Both are described in full in
[THREADING.md](THREADING.md).

**1. The existing API, which now works across threads.** Nothing to change:

```c
swe_set_ephe_path("/usr/share/ephe");   /* main thread */
/* ... worker threads now see it, and use the real ephemeris */
swe_calc_ut(tjd, SE_MOON, iflag, xx, serr);
```

**2. Explicit contexts,** when threads need genuinely separate state:

```c
swe_ctx *ctx = swe_ctx_new();
swe_set_sid_mode_r(ctx, SE_SIDM_LAHIRI, 0, 0);
swe_calc_ut_r(ctx, tjd, SE_MOON, iflag, xx, serr);
swe_ctx_free(ctx);
```

There are **80** `_r` entry points, each taking a context as its first
argument. Two contexts can hold two sidereal modes or two observer positions at
once — something the process-wide API cannot express at all.

`swe_close_r()` is separate from `swe_close()`: the latter also resets the
process-wide configuration, so a worker releasing its own state would otherwise
wipe the settings every other thread was reading.

---

## No silent ephemeris downgrades

⚠️ **The one deliberate behavioural break from upstream.** Ask for an
ephemeris and you get it, or you get an error.

Upstream substitutes on its own — a missing `.se1` file or a date outside it
drops to Moshier, a missing JPL file drops to Swiss. It mentions this in
`serr` and sets the ephemeris bit in the return flag, but the **return code
still says success**, so a caller checking only that cannot tell a data-file
position from an analytic approximation. Every wrapper that discards `serr` —
pyswisseph among them — makes it invisible.

The gap is small enough to pass a spot check and large enough to matter.
Against the DE441-based `.se1` files over 1900–2050, Moshier tracks the Sun to
**0.02″** — so checking the Sun proves nothing — while the Moon reaches
**2.85″** and Neptune passes an arcsecond after 2030.

If you relied on the substitution, restore it:

```c
swe_set_ephe_fallback(1);          /* process-wide  */
swe_set_ephe_fallback_r(ctx, 1);   /* one context   */
```

or set `SE_EPHE_FALLBACK=1` in the environment, which needs no recompile.
Asking for Moshier and getting it was never a downgrade and is unaffected.

Two details worth knowing. Naming no ephemeris at all is treated as asking for
Swiss — it is what `swe_calc_ut()` has always done explicitly, and the two entry
points must agree — so it is refused the same way. And the check covers
`swe_fixstar()`/`swe_fixstar2()`, whose Earth position comes from a path that
bypasses `swe_calc()`; with fallback enabled those still return upstream's
flag, which names the ephemeris requested rather than the one used.

---

## Compatibility

The public ABI is **additive only**: 106 exported symbols before this work,
**190** after, none removed and none changed. Existing binaries keep working
and existing source keeps compiling. Every legacy entry point is now exactly
`swe_X_r(swi_default_ctx(), ...)`.

A CI job compares the exported symbol set against upstream on Linux, macOS and
Windows on every push, and fails if anything upstream exported disappears.

The one thing that is **not** source-compatible is the behaviour above: code
that leaned on a silent fallback now sees `ERR` where it used to get numbers.

---

## What is verified

Every change is gated on a bit-exact transcript — **13000 rows** of C99 `%a`
hex floats compared byte for byte, so no test has to pick a tolerance. The
transcript sweeps 120 pseudo-random dates spanning roughly 1400 years across
three ephemeris flag sets and every major body, recording longitude, latitude,
distance and all three speed components. Where a call fails, the error string
is pinned too, so a message cannot change without a row changing.

Twenty-six gates run on every push (`make -C tests check`), covering
bit-exactness, cross-thread agreement, context independence, configuration
leaks, two specific historical races, malformed-input handling, the threading
backends, the JPL reader and its byte-swapping path, per-context data files,
and whether every source still compiles as conforming C. Two more are opt-in:
a byte-for-byte differential against unmodified upstream's own test suite, and
true JPL Horizons mode, which needs a 2.6 GB ephemeris.

CI additionally builds and checks under gcc, clang, macOS/clang and MSVC, with
ThreadSanitizer, AddressSanitizer and LeakSanitizer, across four C dialects,
plus an LTO build and a differential run of upstream's own `setest` suite.
MSVC's output is compared numerically against the gcc reference over the whole
transcript.

See [THREADING.md](THREADING.md) for the gate-by-gate detail.

---

## Repository layout

| Branch | What it is |
|---|---|
| `main` | default branch; releases are cut from here |
| `work` | fixes in flight, squashed into `main` through a pull request |
| `threadsafe` | the granular history of the transformation — **frozen** |
| `legacy-master` | pristine upstream tree, no CI, kept for merges from upstream |

`notes/` holds the working notes from the transformation: the plan, the
investigation, the configuration map and the API design. They are a historical
record and are dated accordingly, not a description of the current tree. The
exception is `notes/REVIEW.md`, which is kept current: what is still worth
doing in the library code, what was decided against and why, and what closed.

**Versioning.** `SE_VERSION` in `sweph.h` is the only place the version is
written down; everything else derives from it.

```sh
make version                    # print it
make bump VERSION=X.Y.Z-ts.N    # set it
```

A gate fails the build if any other tracked file grows a version literal, and
the release workflow refuses to publish a tag that disagrees with `SE_VERSION`.

---

## License

Swiss Ephemeris is dual-licensed by Astrodienst AG under either the **GNU
Affero General Public License (AGPL)** or a **Swiss Ephemeris Professional
License**, and you must choose one before distributing software that uses it.
This fork changes nothing about that. Read [LICENSE](LICENSE) and
[LICENSE.TXT](LICENSE.TXT), and see
[astro.com/swisseph](https://www.astro.com/swisseph/) for the professional
license.

Swiss Ephemeris was developed by Dieter Koch and Alois Treindl at Astrodienst
AG, Zollikon/Zürich, Switzerland. All the astronomy here is theirs; this fork
only changes how the library handles state.

---

## Upstream

Upstream repository, documentation and support:

- Source: <https://github.com/aloistr/swisseph>
- Documentation: [`doc/`](doc/), and <https://www.astro.com/swisseph>
- Mailing list (the main support channel, and public):
  <https://groups.io/g/swisseph>

Support questions about the ephemeris itself belong upstream. Issues with
*this fork's* threading, contexts, build or packaging belong here.

**For upstream maintainers.** [`notes/UPSTREAM-BUGS.md`](notes/UPSTREAM-BUGS.md)
lists the defects fixed here that are still present upstream — a crash, several
memory errors and four cases where an answer depends on what was computed
before it. Each is verified against upstream's own source, with file, line, a
reproducer and the fix. Nothing in it depends on adopting this fork's context
API.

### Ephemeris data files

Not included in this repository's releases. As of April 2026 all `.se1` files
have been rebuilt with JPL ephemeris DE441 and remain compatible with Swiss
Ephemeris releases back to 1.67 (March 2005).

- Planets and main asteroids:
  [upstream `ephe/`](https://github.com/aloistr/swisseph/tree/master/ephe)
- Asteroid files for all numbered asteroids (over 760,000; ~48 GB), and JPL
  binaries: Alois' public
  [Dropbox area](https://www.dropbox.com/scl/fo/y3naz62gy6f6qfrhquu7u/h?rlkey=ejltdhb262zglm7eo6yfj2940&dl=0),
  or <https://ephe.scryr.io/> (web space provided by Phillip McCabe)
- JPL files direct from JPL:
  <https://www.astro.com/ftp/swisseph/jplfiles/index.htm>, which links the
  files on `ssd.jpl.nasa.gov` and publishes their md5 sums. After download,
  `de441/linux_m13000p17000.441` must be renamed `de441.eph` to be recognised
  (likewise `de431/lnxm13000p17000.431` → `de431.eph`). Both are ~2.6 GB;
  expect around 0.3 MB/s from JPL, so the Dropbox mirror above is usually
  faster.

**Where the library looks.** `swed.ephepath` defaults to `\sweph\ephe` on
Windows and `.:/users/ephe2/:/users/ephe/` on Unix-likes; `;` (Windows) or `;`
and `:` (Unix) separate entries. Call `swe_set_ephe_path()` to change it — in
this fork, from any thread, and it will be seen by the others.

Planetary files (`sepl*.se1`, `sem*.se1`, `seas*.se1`) go directly in a path
element. Asteroid files go in subdirectories `astN`, where N is the asteroid
number divided by 1000. Short and long asteroid files may share a directory.

### Other language bindings

Python ([pysweph](https://github.com/sailorfe/pysweph)),
Java ([Thomas Mack](http://www.th-mack.de/international/download/)),
PHP ([php-sweph](https://github.com/cyjoelchen/php-sweph)),
Perl ([perl-sweph](https://github.com/aloistr/perl-sweph)).
These wrap upstream and do not carry this fork's changes. The AGPL applies to
them as well.
