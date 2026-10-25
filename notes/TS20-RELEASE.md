# ts.20 release: the critical fixes

ts.20 is a short release of the fixes that make ts.19 unstable or visibly
wrong. Everything else from PR #33 is left for ts.21 (`notes/TS21-PLAN.md`).

This file is the working document for the release. A fresh session reads
it first, checks the status column, and carries on from there.

| | |
|---|---|
| Branch | `ts20-release`, off `main` (`14462bf`, ts.19); worktree `/nvm/work/swe-rel` |
| Replaces | PR #33 (branch `ts20`, kept as is). A new PR is opened; #33 is closed with a link to it. |
| Source of each fix | Its commits on `ts20` (worktree `/nvm/work/swe-ts20`, tip `fd605d3`). Bug 293 comes from `notes/cleanup-archive/swe-f291.b9c5efe.patch` on `ts20-archive`. The bug entries are in `ts20`'s `notes/UPSTREAM-BUGS.md` (`git -C /nvm/work/swe-ts20 show fd605d3:notes/UPSTREAM-BUGS.md`). |
| Commit map | `notes/ts21/commits.tsv`: every `ts20` commit with its bugs |

## Decisions (owner, 2026-10-25)

1. **Scope:** Tier 1 and Tier 2 below, released today if possible.
2. **No compat switch** (`SWE_UPSTREAM_COMPAT`) for any new fix. The fix
   replaces the code. `main`'s 21 existing switches stay.
3. **One commit per fix.** A fix's layers on `ts20` become one change.
4. **Agents:** Sonnet, except where Opus is marked. At most three run at a
   time, each with at most 4 processes.
5. **The PR:** a new PR from `ts20-release`; #33 is closed with a link to it.

## The fixes

Status is one of `todo`, `agent`, `merged` or `dropped (why)`.

### Tier 1: crashes, memory errors, lost settings, performance regression

| # | What a user sees in ts.19 | ts20 bugs | ts20 commits | Package | Status |
|---|---|---|---|---|---|
| 1 | A setting changed in one thread can crash another thread's calculation (files closed under it) or mix two configurations into one answer. A new thread's first call misses settings made elsewhere (delta t 22.6 s off). | 115, 124, 128 | `4f30ce67c` `63f62f83d` `958bd9c64` | P1 | todo |
| 2 | A setting made before the first calculation is lost; the first heliacal call after a setter fails | 83 | `02ec81291` `fef95437c` (settings part) | P1 | todo |
| 3 | `swe_set_ephe_fallback()` is never seen by other threads | 122 | `e1158f3a0` | P1 | todo |
| 4 | A missing Swiss file whose neighbour does not hold the date gives a NaN Moon, or the last read's Moon | 100 | `30960edf8` `9d97c0a8d` | P5 | todo |
| 5 | A refused Swiss file empties the constants of every open file, so later calls fail on healthy files; the path setter crashes on a refused moon file | 104 | `19995e7ac` | P5 | todo |
| 6 | Values read from an ephemeris file's header, the delta-psi tables and the star catalogue index arrays unchecked: a corrupted file writes past them | 106 | `93d682540` `f48cd8b03` | P5 | todo |
| 7 | A bad seorbel.txt field hangs `swe_calc()`; terms such as "T5" or ".0" are misread without an error | 113, 114 | `0dc29607b` `b962846c2` | P3 | todo |
| 8 | SEFLG_JPLHOR with no IERS corrections loaded crashes the ayanamsa, sidereal houses and Gauquelin sectors | 142 | `5a9ff4333` | P7 | todo |
| 9 | The range refusal of a huge date overruns the 256-byte error buffer | 210 | `5b1965023` (the overrun part only; the rest of 5b19650's absurd-date work is ts.21) | P7 | todo |
| 10 | `swe_sidtime()` reads nutation left unwritten; SE_ECL_NUT reads `pla_diam[-1]` in the rise and eclipse functions | — , 293 | `897045c`, archive patch `b9c5efe` (the 293 part) | P7 | todo |
| 11 | The default nutation (full IAU 2000A since ts.17) is slow for dates a day or more apart: transit and station scans pay for it | G32 | `571481b` `427f30c` | P8 | todo |

### Tier 2: big wrong answers in normal use

| # | What a user sees in ts.19 | ts20 bugs | ts20 commits | Package | Status |
|---|---|---|---|---|---|
| 12 | Placidus cusps not converged, RADIANS house speeds in degrees, sidereal Makransky computed as Treindl, Sunshine houses without files; Sunshine's Sun from the wrong ephemeris, stale Gauquelin cusps 13..36 after the Porphyry fallback | 59, 68 | `ede4afa38` `0871d47dd` `4a8865244` | P2 | todo |
| 13 | `swe_house_pos()`: Sripati answers 1.0 for all of house 12 and half of house 1 (up to 11.5 houses off); the horizon system counts backwards south of the equator | 270, 289 | `ede2cce9d` (270 part), `1fc11a3b7` (289 part) | P4 | todo |
| 14 | `swe_gauquelin_sector()`: sidereal places set in the tropical frame (2.6 sectors); other flags move the sector by up to 18.4 sectors | 236, 252 | `1df58684d` `6e0c792c3` | P4 | todo |
| 15 | Sidereal houses on the ecliptic of t0 and the solar system plane (the J2000, J1900 and B1950 modes always): ascmc[2] the auxiliary ARMC, whole signs 8.9 deg off, ascmc[8] moved by up to 359 deg, the vernal point's offset folded by acos | 268, 269, 287, 288 | `ede2cce9d` (268/269 part), `1fc11a3b7` (287/288 part) | P4 (Opus) | todo |
| 16 | Kronos's semi-axis has two digits swapped; the nakshatras of `swe_split_deg()` are shifted for SE_SIDM_USER | 65, 97 | `6b514b758` `cfb91442c` `14cecf55a` | P6 | todo |
| 17 | The heliacal functions answer from memos made for another atmosphere, latitude or star catalogue | 82 | `2f9d1dec1` `0b6f0c5dc` `fef95437c` (heliacal part) | P6 | todo |
| 18 | The default sidereal time steps 1.9" at 1850 and 2050; the long-term sidereal time follows delta t, up to 2400" off for distant dates. It must come with 209's bound: past its table the long-term integration took 140 s or crashed. | 25, 29, 209 | `bd6056215` `1e5683532` `8215e170c` (the bound) | P9 (Opus) | todo |
| 19 | The topocentric observer is placed about the mean pole (up to 17"); the heliocentric light time is measured from the barycentre on its second pass (up to 2.6") | 27, 26 | `9c98d8b5a` `a11c59e4f` | P9 (Opus) | todo |
| 20 | Reported, never verified: `calc_dip()` NaN for a site below sea level (rising and setting there); `swe_lun_eclipse_when_loc()` started at an eclipse's maximum returns that same eclipse | — | — | P10 | todo |

Fix 20 is verified first. It is fixed only if the bug is real and the fix
is small; otherwise it goes to ts.21 with the measurement.

## Packages and order

| Batch | Packages, run in parallel |
|---|---|
| A | P1 (Opus: fixes 2, 3, then 1), P2 (fix 12), P3 (fix 7) |
| B | P4 (Opus: fixes 13, 14, 15), P5 (fixes 4, 5, 6), P6 (fixes 16, 17) |
| C | P7 (fixes 8, 9, 10), P8 (fix 11), P9 (Opus: fixes 18, 19) |
| D | P10 (fix 20) |

The merge order onto `ts20-release` is P1 first, because fix 1 touches the
entry of every public function, then the other packages in fix order.

**Overlaps:**
- `fef95437c` carries parts of both 82 and 83. P1 takes the settings part
  and P6 the heliacal part.
- `ede2cce9d` and `1fc11a3b7` each carry several bugs. P4 takes only the
  parts of fixes 13 and 15.

## Brief (the same for every package)

Fill in the package and its fixes from the tables above.

> You are carrying fixes from the `ts20` branch of the swisseph fork onto a
> clean release branch. First read `notes/TS20-RELEASE.md` in
> `/nvm/work/swe-rel`. Your package is PN: fixes N, M.
>
> **Setup.** Make your own worktree:
> `git -C /nvmraid/shares/swisseph worktree add /nvm/work/swe-rel-PN -b ts20-rel-PN ts20-release`.
> Work only there.
>
> **For each fix:**
> 1. Read its bug entries (`## N.` in
>    `git -C /nvm/work/swe-ts20 show fd605d3:notes/UPSTREAM-BUGS.md`) and
>    its listed commits (`git -C /nvm/work/swe-ts20 show <commit>`).
> 2. Check whether later `ts20` commits corrected the same code
>    (`git log -L` or `git blame fd605d3` on those lines). Take a
>    correction that belongs to this fix. Leave refinements that belong to
>    other bugs.
> 3. Write the fix against your branch's code:
>    - no `SWE_UPSTREAM_COMPAT`, and no `#else` copy of upstream;
>    - the fixed behaviour only, in straight code: no layered retries and
>      no pasted checks;
>    - comments explain the code and never say "bug NNN";
>    - where `ts20`'s fix stands on code from fixes that are not in this
>      release, carry the minimum it needs, or write the fix directly on
>      the code as it is here.
> 4. Add one test per fix in `tests/`, taken from `ts20`'s gate test for it
>    and trimmed: no both-builds part, and no `compat.c` or G24 case. Name
>    it after the behaviour and write `GNN` where the gate number goes; the
>    coordinator numbers it at merge. Wire it into `tests/Makefile` the way
>    `main`'s tests are, and add it to `check`.
> 5. Add one entry per fix to `notes/UPSTREAM-BUGS.md` as `## NN.`, plus
>    its Contents row. Keep it short: what a caller sees, where, how big,
>    and the shape of the fix. Do not add a `sweph.h` compat bullet or a
>    REVIEW clause.
> 6. Check: `make -j4 LTO= libswe.a` at the root, then in `tests/`
>    `make NPROC=4 check` and your new test.
> 7. Commit, one commit per fix:
>    - the subject says what changes for a caller;
>    - the body gives the cause, the size, the test and "ts20 bugs: N, M";
>    - no attribution lines and no session ids.
>
> **Rules:**
> - At most 4 processes. Check `uptime` first, and wait if the load is
>   above about 12.
> - No sanitizers, stress runs or benchmarks.
> - Never `git stash`. Do not push.
> - Do not fix anything outside your package. List defects you notice in
>   your report.
>
> **Report:**
> - your commits;
> - the test results, with numbers;
> - what you left out of each `ts20` fix, and why;
> - anything the coordinator must know to merge;
> - defects noticed, for `notes/ts21/found-later.md`.

## Merge (coordinator)

1. Cherry-pick each package's commits onto `ts20-release` in order, and
   resolve any conflicts.
2. Number the bug entries 23, 24, … in commit order, and the gates G32, G33,
   … in commit order.
3. Build and run `make NPROC=4 check`.
4. Mark the fixes `merged` in the tables above and commit.
5. Remove the package's worktree and branch.

## Final pass

All steps run sequentially, with at most 4 processes:

1. `make NPROC=4 check` in full.
2. The thread gates under TSan (`check-sunrace-tsan`, `check-threadshim-tsan`
   and `glprace_tsan`), because of fix 1.
3. The new tests under ASan and UBSan, one at a time.
4. Re-baseline G1, G10, G17 and golden. Each change must be explained by a
   fix in this release.
5. The version: ts.20, as in `ts20`'s `7390781`.
6. The REVIEW row for ts.20.
7. Write the PR body; parts of `ts20-archive`'s
   `notes/cleanup-archive/PR-ts20-body-draft.md` can be reused.
8. In the last commit, remove this file. `notes/TS21-PLAN.md` and
   `notes/ts21/` stay.
9. With the owner's go-ahead:
   - push `ts20-release` and open the PR;
   - close #33 with a link to it;
   - wait for CI;
   - push `ts20-archive` for safekeeping.
10. After the merge:
    - reinstall the system libswe as the README describes;
    - tell the astromcp and Astrolog sessions;
    - remove the old agent worktrees and branches, as in
      `notes/TS21-PLAN.md`, chunk 6.

## Progress log

| Date | Step | Notes |
|---|---|---|
| 2026-10-25 | Plan | Scope confirmed by the owner (Tier 1 and Tier 2, new PR). Branch made off `main`. `ts20-archive` (`122f995`) holds the unmerged agent work. |
