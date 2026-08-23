# Binaries in the source tree — findings and plan

**Status:** in progress. Step 1 partially done; `contrib/` deliberately deferred.

The tree carries compiled artifacts, prebuilt archives, and in one case a
second copy of its own source. This note records what is actually there, what
has been removed, and what is deliberately left alone for now.

---

## Why this matters more than tidiness

Every prebuilt binary in the tree is stamped **2.10.03** (upstream's release)
or older — `swedll64.dll` inside `contrib/` is 2.02.01, from 2015. None of
them contain any of the thread-safety work on this branch.

That is worse than clutter. Someone who clones a *thread-safe fork* and runs
its `swetest.exe`, or links its `libswe-2.10.03.so`, gets the **unfixed**
library, silently — the exact failure mode this branch exists to eliminate.

---

## Inventory

### Removed

| Path | What | Why |
|---|---|---|
| `bin/swetest`, `bin/swevents` | output of the `swetests`/`sweventss` targets | `make all` **rewrote them on every build**, so the tree was dirty after every gate run. Also broke CI: the runner's sparse checkout had no `bin/`, so the bare `cp` failed. Both targets now `mkdir -p bin` first. |
| `windows/programs/*.exe` (7) | 2.10.03 prebuilts | predate every fix on this branch; all rebuildable from source here |
| `windows/programs/ceres.readme` | doc for `ceres.exe` | its subject is gone |

`ceres.exe` is the only one that was **not** rebuildable — no source for it
exists anywhere in this tree. Its own readme said it "is NOT part of the
Swiss Ephemeris, and it is NOT supported", so it was dropped rather than
replaced.

### Still present, decision pending

**`windows/sweph.zip` (10.5 MB)** — the worst offender, and *not* a simple
delete. It is a snapshot of the 2021/22 upstream tree containing:

- **35 of its 73 files duplicate something already tracked** — including
  full copies of `sweph.c` (290 KB vs the current 314 KB), `swephlib.c`,
  `swephexp.h`, `seasnam.txt`, `sefstars.txt`. So the repo currently holds
  two different versions of its own source.
- **Unique source found nowhere else**: 15 `.vcxproj`, `sweph.sln`, 2 `.rc`,
  `module1.bas`, `sweph_vb7_64.bas` — the MSVC build system.
- **Unique binary samples**: `orbit.xls`, `orbit2.xls` (to be kept, extracted).
- Binaries to drop: `swedll32/64.dll` and `.lib`, `vb/swedll32.lib`.
- Docs to drop: `doc/*.htm`, `*.gif`, `*.cdr` — `doc/*.pdf` covers these.
- **A nested copy of `contrib/Sweph32_For_Excel_VBA_and_VB.zip`.**

Plan: extract the unique source into `windows/projects/` and `windows/vb/`,
**repair it** (see below), drop the rest, delete the zip. Both
`windows/readme.md` and `contrib/readme.md` reference the zip by name and
need updating with it.

**The extracted projects are broken as-is.** All 15 `.vcxproj` list nine
library sources and **none mentions `sweconfig.c`**, which this branch added
and the library now requires. Opening the solution today gives unresolved
externals for `swi_config_publish`, `swi_default_ctx` and the rest. Checking
in a build system that cannot build is just relocating the smell, so the
extraction must fix them. Their `Include="..\foo.c"` paths are also relative
to the zip's `src/projects/`, and need rewriting for wherever they land.

### Deliberately deferred: `contrib/`

Left entirely alone for now — it is a large enough question to derail the
work in progress, and may end up refactored or dropped wholesale. Recorded
so the findings are not lost:

- `contrib/android/libs/*.so` — 4 ABIs, all stamped 2.10.03
- `contrib/swedll64_2.02.01.zip` — 2 files, both binary, from 2015
- `contrib/android/jni/` — **`sweph.c`, `swephlib.c`, `swephexp.h` and
  others here are symlinks to the root sources**, so the Android build
  tracks this branch automatically. Worth knowing before anyone "tidies"
  them into copies.
- The other five contrib archives are genuine third-party contributions with
  essentially no overlap with the tree (0 duplicate filenames each, except
  the Java one's 4: `LICENSE`, `sefstars.txt`, `seleapsec.txt`,
  `seorbel.txt`).

---

## Not the same question: repository size

Removing binaries is right on principle but will **not** shrink a clone.
Every historical version stays in the pack (876 MB), so this only stops
future churn.

And binaries are not where the size is:

```
ephe/      378.3 MB     <- ephemeris data
windows/    16.0 MB
setest/     12.5 MB     <- test expectations
doc/         8.5 MB
contrib/     4.5 MB
source       2.3 MB     <- all .c and .h
```

`ephe/` is upstream **data** on its own release cadence, not build output.
Whether it belongs in git at all is a separate decision from this one, and
shrinking history would mean a rewrite — not worth it on a branch already
pushed.

---

## Where this is heading

1. **Binaries out of `HEAD`** — in progress.
2. **CI builds per-platform artifacts on every push**, uploaded as workflow
   artifacts. Valuable on its own and a prerequisite for step 3: today the
   Windows job runs `cl /c` on the library sources and links **nothing**, and
   there is no Android job at all — so we would otherwise be promising
   binaries we have never proven we can build.
3. **Tag-triggered release** attaching those same artifacts, per platform,
   plus `SHA256SUMS`. The tag *is* the source; GitHub generates source
   tarballs from it, so no hand-built source archive can drift — which is
   precisely how `windows/sweph.zip` came to exist.

Open decision before any tag: `SE_VERSION` is still `"2.10.03"` and the repo
carries 24 upstream tags with 0 releases. A fork that ships releases needs
its own version identity, or its tags become indistinguishable from
upstream's while the behaviour differs.
