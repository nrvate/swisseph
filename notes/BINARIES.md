# Binaries in the source tree — findings and plan

**Status: done.** No build products are tracked. CI builds Linux, macOS,
Windows and Android packages on every push, and pushing a `v*` tag publishes
them as a GitHub release — first done for `2.10.03-ts.1`, and again for
`2.10.03-ts.2`. The `contrib/` third-party archives are still there by
decision, not oversight; see
[Still deferred: the rest of `contrib/`](#still-deferred-the-rest-of-contrib)
below.

*Everything after this line is the note as written while the work was in
progress, kept as the record of how it was reasoned about.*

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
| `bin/swetest`, `bin/swevents` | output of the `swetests`/`sweventss` targets | `make all` **rewrote them on every build**, so the tree was dirty after every gate run. Also broke CI: the runner's sparse checkout had no `bin/`, so the bare `cp` failed. `swetests` now `mkdir -p bin` first; `sweventss` was removed in `8dcd53d`, having never been in `ALL_TARGETS`. |
| `windows/programs/*.exe` (7) | 2.10.03 prebuilts | predate every fix on this branch; all rebuildable from source here |
| `windows/sweph.zip` | source snapshot + 9 binaries | see below |
| `windows/swephzip.txt` | listing of that archive | its subject is gone |
| `windows/programs/ceres.readme` | doc for `ceres.exe` | its subject is gone |

`ceres.exe` is the only one that was **not** rebuildable — no source for it
exists anywhere in this tree. Its own readme said it "is NOT part of the
Swiss Ephemeris, and it is NOT supported", so it was dropped rather than
replaced.

### Removed: `windows/sweph.zip` (10.5 MB)

A snapshot of the 2021/22 upstream tree. **35 of its 73 files duplicated
something already tracked** — including full copies of `sweph.c` (290 KB
against the current 314 KB), `swephlib.c`, `swephexp.h`, `seasnam.txt` — so
the repository held two different versions of its own source. It even
contained a nested copy of `contrib/Sweph32_For_Excel_VBA_and_VB.zip`.

It was not simply deleted, because it also held the **only** copy of the
MSVC build system. Extracted first:

| to | what |
|---|---|
| `windows/projects/` | 15 `.vcxproj`, `sweph.sln` |
| `windows/vb/` | `module1.bas`, `sweph_vb7_64.bas`, `orbit.xls`, `orbit2.xls` |
| `doc/ast_list.txt` | the last unique text in the archive |

Dropped: the duplicated sources, `swedll32/64.dll` and `.lib`,
`vb/swedll32.lib`, the empty `.rc` stubs, and `doc/*.htm`/`*.gif`/`*.cdr`
(older HTML copies of what `doc/*.pdf` already carries).

**The projects could not build this tree**, and CI now builds all 15 on
every push so they cannot rot again. What had to be fixed is in
`windows/projects/README.md`; the one that mattered most was every project
writing its object files to a single shared directory, so the DLL was
linked from objects compiled without `MAKE_DLL` and exported **nothing** —
while still building, running, and passing every smoke test.

### `contrib/android` — fixed and building

`contrib/android/libs/*.so` — four `libswe-2.10.03.so`, stamped with
upstream's version — were the last binaries in the tree that nothing could
rebuild. Untracked; CI now builds every ABI the NDK offers (five as of
NDK 27, which adds `riscv64`).

**The build was broken in exactly the way the MSVC projects were:**
`Android.mk` did not list `sweconfig.c`, and `jni/` had no symlink for
`sweconfig.h` or `swethread.h` — all three arrived with Phase 2, and the
library needs them. The module could not compile, let alone link.

Worth preserving deliberately: every library source in `jni/` is a
**symlink** to the repository root, so Android tracks this branch
automatically. Replacing them with copies would silently freeze it — which
is precisely what `windows/sweph.zip` did.

### Still deferred: the rest of `contrib/`

- `contrib/swedll64_2.02.01.zip` — 2 files, both binary, from 2015
- The other five archives are genuine third-party contributions with
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

*All three landed. Kept as written, with the outcome recorded after each.*

1. **Binaries out of `HEAD`** — done for `bin/` and `windows/`; `contrib/`
   deferred.
2. **CI builds per-platform artifacts on every push.** Done for Windows: the
   `windows / package` job builds all 15 MSVC projects, smoke-tests both
   CLIs, verifies the DLL actually exports the context API, and uploads a
   package with `SHA256SUMS`. Still to do: Linux, macOS, and Android — there
   is no Android job at all, so that platform's prebuilts in `contrib/`
   remain something we cannot yet reproduce.
3. **Tag-triggered release** attaching those same artifacts, per platform,
   plus `SHA256SUMS`. The tag *is* the source; GitHub generates source
   tarballs from it, so no hand-built source archive can drift — which is
   precisely how `windows/sweph.zip` came to exist.

Open decision before any tag: `SE_VERSION` is still `"2.10.03"` and the repo
carries 24 upstream tags with 0 releases. A fork that ships releases needs
its own version identity, or its tags become indistinguishable from
upstream's while the behaviour differs.

---

## Outcome

**2** and **3** landed. All four platforms package on every push — Linux
(pinned to `ubuntu-22.04` so the binaries need only `GLIBC_2.34`), macOS
(universal `arm64` + `x86_64`), Windows (all 15 MSVC projects), and Android
(five ABIs, JNI export count checked against `swejni.h` rather than a guessed
threshold). A `v*` tag publishes them.

The version question was settled with a `-ts.N` suffix: `SE_VERSION` was
`2.10.03-ts.2` when this was written, it is the only place the version is
written down, and
`make bump` moves it. Tags are `v2.10.03-ts.N`, which cannot collide with
upstream's, and the release workflow refuses to publish a tag that disagrees
with `SE_VERSION`.

Two things only showed up once a release actually existed, and neither was
visible from a green CI run:

- `actions/upload-artifact` does not carry unix file modes, so `ts.1` shipped
  tarballs whose binaries were `0644` — extract, run `./bin/swetest`, get
  `Permission denied`. Found by downloading the published release and running
  it. The gate now reads the mode back out of the finished tarball.
- The Windows `SHA256SUMS` was written with CRLF, which makes `sha256sum -c`
  fail on every line. It survived because the Windows job only checked the
  file *existed*, where Linux and macOS ran a real `-c`.

Both are why the packaging jobs now verify their own output the way a
recipient would, rather than trusting the step that produced it.
