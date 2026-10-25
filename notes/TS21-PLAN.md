# ts.21 plan: the rest of PR #33

PR #33 (branch `ts20`) could not be merged as it stood. Its critical fixes
went out as ts.20. The fix list was `notes/TS20-RELEASE.md`, which stays in
that release's history. This plan carries whatever else from `ts20` is
worth keeping into ts.21, on a clean branch, `ts21`, made off `main` after
ts.20 is merged.

The work is split into chunks. Each chunk can run on its own in a fresh
session, at whatever pace the token allowance permits. Each one ends by
updating the progress log at the bottom of this file and committing.

A session picking up a chunk reads this file first. It reads nothing else
from earlier conversations: everything it needs is here or in the repo.

**Excluded everywhere below:** the bugs ts.20 already carries. They are 25,
26, 27, 29, 59, 65, 68, 82, 83, 97, 100, 104, 106, 113, 114, 115, 122, 124,
128, 142, 209 (its bound), 210, 236, 252, 268, 269, 270, 287, 288, 289, 293,
the sidereal-time nutation zeroing (`897045c`) and the fast default
nutation (G32). The release's progress log records any that were dropped
or cut, and those come back into scope here.

## Why

`ts20` is 255 commits on top of `main` (`14462bf`, ts.19), made on
2026-10-24/25. It changes:

- 20k lines of library code, 66k lines of tests (128 new test files) and
  19.5k lines of notes;
- about 262 entries in `notes/UPSTREAM-BUGS.md` (#23–295).

Its first 113 commits were made with the owner, from 16:01 to 00:31. They
are correctness and performance fixes that ts.19 needs. The other 142 came
from an unattended loop that started at 00:39 on Oct 25. That loop fixed
every defect it found, at any size, and each fix agent found two or three
more.

The result reads as a pile of workarounds:

- every fix sits behind `#ifndef SWE_UPSTREAM_COMPAT` with upstream's code
  kept under `#else`, and the count of these switches went from 167 to 902
  overnight;
- fixes are stacked in layers inside the same functions (`swe_nod_aps` is
  1,581 lines with 48 switches, `lunar_osc_elem` is 725 lines where
  upstream has 426);
- the same inline checks are pasted many times over;
- comments name bug numbers instead of explaining the code;
- many fixes are for rounding noise or for dates billions of years out.

## Decisions (owner, 2026-10-25)

1. **A fresh branch.** `ts21` starts from `main` once ts.20 is merged.
   `ts20` and `ts20-archive` are the sources, left untouched.
2. **No compat switches** for anything new. Each fix replaces the code it
   fixes. The original upstream (`3fd0f95`, the `legacy-master` the fork
   branched from) is the reference build for comparisons. The 21 switches
   already on `main` from earlier rounds stay as they are.
3. **Pre-loop fixes lean towards keep.** These are the 113 commits from
   `571481b` to `b962846`, made with the owner: bugs 23–114 and the
   performance and build work. They are sorted with the same rubric, but
   the default is KEEP. Only clear noise, or a fix superseded by another,
   is dropped. They are still consolidated (point 7) and carried over
   without switches.
4. **Loop-era fixes are kept only if they matter to users or to accuracy.**
   Loop-era means #109, #115–295 and the never-merged 290–316 plus seven
   unnumbered bugs. The accuracy threshold is Swiss Ephemeris's own
   precision, **0.001 arcsec**. The full rubric is below.
5. **Real effects below 0.001" go to `notes/accuracy.md`.** That covers
   systematic effects, not rounding noise. They are reviewed later for a
   possible next PR. Pure noise is dropped.
6. **Rare setups** keep only crashes and errors of 0.001" or more. Rare
   setups are: JPL files, the Moshier fallback, missing files, alternative
   precession or nutation models, and changing files or settings mid-run.
7. **One commit per fix.** A fix's layers, spread over several commits and
   bug numbers on `ts20`, become one unified change.
8. **swetest fixes are kept.**
9. **Agents:** use Sonnet for mechanical work. Use Opus only where a step
   needs judgment or a delicate rewrite. The triage uses four agents plus a
   review by the coordinator.
10. **No new defect hunting.** A defect noticed while doing this work goes
    into `notes/ts21/found-later.md` with one line on its size. It is
    not fixed in this PR.

## Where things are

| What | Where |
|---|---|
| Repository | `/nvmraid/shares/swisseph`, origin `git@github.com:nrvate/swisseph.git` |
| `main` | `14462bf`; 22 bug entries, 21 compat switches, 25 tests |
| `ts20` (PR #33) | worktree `/nvm/work/swe-ts20`, tip `fd605d3`; 221 commits not pushed (`origin/ts20` is `2e39062`) |
| `ts20-archive` | `ts20` plus `notes/cleanup-archive/`: the unmerged agent work (291–293 commit, 296/299/300 and 301–303 partial diffs), the coordinator's state log with the descriptions of 290–316, and the PR body draft. See its README. |
| `ts20-release` | ts.20, the critical fixes; it carries this file and `notes/ts21/` onto `main` |
| `ts21` | made off `main` after ts.20 is merged. Chunk outputs go in `notes/ts21/`. `notes/ts21/commits.tsv` (every `ts20` commit with its bugs, from `notes/ts21/commit-bugs.sh`) is chunk 1's first step, already done. |
| Upstream reference | commit `3fd0f95`; build it in a scratch worktree when a comparison is needed |
| Ephemeris files | `tests/Makefile`'s `EPHE ?= ../ephe` |
| Pre-loop commits | `git log --reverse --format=%H origin/main..fd605d3` with committer time before 2026-10-25T05:39:19Z (113 commits, `571481b` .. `b962846`) |
| Old agent worktrees and branches | `/nvm/work/swe-f*`, about 90 local `ts20-f*` branches, plus `ts20-review`, `ts20-tsan` and `ts20-warn`. Their work is merged or archived; they are removed in chunk 6. |

## Rules for every chunk

- **Machine:** the owner works on the same 12 cores. Each agent runs at
  most 4 processes. Use `make NPROC=4` in `tests/`, because its default is
  `nproc`. Use `make -j4 LTO= libswe.a` at the root. Check `uptime` first,
  and wait if the load is above about 12. No sanitizers, stress runs,
  warning matrix or benchmark before chunk 6.
- **Git:** never use `git stash`. Commits go on `ts21` only. Commit
  messages are meaningful and multi-line, with no attribution lines and no
  session ids. Nothing is pushed before chunk 6, except pushing
  `ts21` and `ts20-archive` for safekeeping if the owner agrees.
- **Budget:** a chunk stops at its end. It does not run on into the next
  one. If a chunk cannot finish, it records where it stopped in the
  progress log and commits.
- **Asking the owner:** use plain language. Say what a choice changes for a
  caller, not how it is implemented.

## The rubric (loop-era fixes)

Sizes are converted to an angle on the sky:

| Kind of number | Converts as |
|---|---|
| Angle | 0.001" = 2.78e-7 deg = 4.85e-9 rad |
| Speed | the same per day |
| Distance | relative, dr/r ≥ 4.85e-9 |
| Times | through the fastest motion they move: the Moon, 1 s ≈ 0.55"; the Earth's rotation (sidereal time, houses, rising and setting), 1 s ≈ 15", so 0.001" ≈ 6.7e-5 s. Delta t errors move the Moon. |

When an entry quotes several sizes, use the largest one reachable in the
setup class it belongs to.

| Decision | When |
|---|---|
| **KEEP** always | A memory error, a crash, a hang of 1 s or more, or a defect in the fork's own thread-safety or configuration code ("this fork's" in the entry) that gives a wrong answer. |
| **KEEP** | In a standard setup (Swiss files, dates inside their range of 13200 BC .. AD 17191, documented flags and options): a wrong number of 0.001" or more, or a wrong non-number. Wrong non-numbers include a wrong flag or ephemeris reported, a wrong house number, a valid request refused, and success reported for wrong data. |
| **KEEP** | In a standard setup: an answer that depends on earlier calls or settings, by 0.001" or more or in a non-number. |
| **KEEP** (rare setup) | Only a memory error, crash, hang, or an error of 0.001" or more. |
| **ACCURACY** | A real systematic effect below 0.001": a missing term, a wrong formula or constant, a biased method. It goes to `notes/accuracy.md` and is not applied. |
| **DROP** | Rounding or step-size noise. |
| **DROP** | Dates outside every supported ephemeris (before 13200 BC, after AD 17191, NaN, ±inf), unless the fix prevents a memory error, crash or hang. |
| **DROP** | Error-message wording, unless the message misleads in a standard setup. |
| **DROP** | Fork-internal bookkeeping with no effect on callers. |
| **ASK** | A genuinely unclear case. The coordinator decides in chunk 4, with the owner where needed. |

## Chunks

| # | Chunk | Model | Size | Needs | Output |
|---|---|---|---|---|---|
| 0 | This plan, the archive branch | — | done | — | this file, `ts20-archive` |
| 1 | Fix map | Sonnet | S | 0 | `notes/ts21/map.tsv`, `hunks.tsv`, `commits.tsv` |
| 2p | Triage of the pre-loop entries not in ts.20, default KEEP | Sonnet | M | 1 | `notes/ts21/triage-p.tsv` |
| 2a–2d | Triage of the loop-era entries, one share each | Sonnet | M each | 1 | `notes/ts21/triage-{a,b,c,d}.tsv` |
| 3 | Triage of the never-merged bugs | Sonnet | S | 1 | `notes/ts21/triage-e.tsv` |
| 4 | Consolidation and review sheet | Opus | M–L | 2, 3 | `REVIEW-SHEET.md`, `notes/accuracy.md` draft, cluster list. **Owner sign-off.** |
| 5.x | Apply, one chunk per cluster | Sonnet; Opus for the layered rewrites | M–L each | 4 signed off | commits on `ts21` |
| 6 | Final pass, new PR | coordinator; Sonnet for mechanical steps | M | all of 5 | the PR |

The 2x chunks and chunk 3 are independent of each other. They can run in
parallel or on different days.

### Chunk 1: the fix map

**Goal:** for every changed line, know which fix wrote it. This is
mechanical. Write it as a script and commit the script with its output.

1. **Commit to bugs.** For each commit in `origin/main..fd605d3`:
   - first, take the `## N.` headings it adds to `notes/UPSTREAM-BUGS.md`;
   - else, the entries whose text it changes. Map each changed line
     number, from the `-U0` hunk headers, to the nearest preceding `## N.`
     heading in that commit's version of the file;
   - else, the bug numbers in its message, marked `weak`;
   - else, `none`, with a one-line description from the subject.

   Mark each commit `pre-loop` or `loop` by its committer time (see the
   table above). Output: `commits.tsv` (commit, time, phase, bugs,
   source of the bugs, subject).
2. **Lines to commits.** For each library file changed in
   `origin/main..fd605d3` (root `*.c` and `*.h`, `swetest.c` included), run
   `git blame --line-porcelain origin/main..fd605d3 -- FILE`. A boundary
   commit means the line is `main`'s own. Collapse runs of lines from the
   same commit into hunks:
   - mark each hunk `fixed` (inside an `#ifndef SWE_UPSTREAM_COMPAT`
     branch), `guard` (the `#ifndef`/`#else`/`#endif` lines themselves) or
     `plain`;
   - give the enclosing function;
   - list deletion-only hunks of `git diff -U0 origin/main fd605d3`
     separately, with the commit from `git log -L` where it resolves.

   Output: `hunks.tsv` (file, first line, last line, function, kind,
   commit, bugs).
3. **Per bug.** List the commits, the hunks, the lines added in each file,
   the tests added (`tests/*.c` files created by its commits, and their
   Makefile targets), and where its entry sits in `notes/UPSTREAM-BUGS.md`
   at `fd605d3` (line range, title and severity from the Contents table).
   Output: `map.tsv`.
4. **Acceptance.** Every added library line is attributed to a commit, and
   at least 95% of commits resolve to a bug or to a named non-bug item.
   List the rest in the progress log.

### Chunk 2p: triage of the pre-loop entries

The pre-loop entries (23–114) sit in lines 1596–7151 of
`notes/UPSTREAM-BUGS.md` at `fd605d3`. Skip those ts.20 carries. The brief
and schema are the same as for 2a–2d below, with one difference: the
default decision is KEEP, and DROP needs a reason (noise, or superseded by
another fix). Also list the pre-loop commits with no bug (see
`commits.tsv`), each with a keep or drop and its reason.

### Chunks 2a–2d: triage of the loop-era entries

Skip the entries ts.20 carries. The 174 loop-era entries sit in lines 7152–20934 of `notes/UPSTREAM-BUGS.md`
at `fd605d3`. They are split into four even shares by file position:

| Share | File lines | Entries | Bugs |
|---|---|---|---|
| a | 7152–10660 | 39 | 109 116 117 121 119 118 122 123 126 120 115 124 125 127 128 129 134 130 132 133 131 135 136 137 138 145 139 146 153 140 141 142 143 144 151 152 147 148 149 |
| b | 10661–14108 | 46 | 150 157 154 155 156 158 171 172 161 162 173 164 165 166 167 168 169 159 160 163 181 182 176 178 180 175 177 183 184 185 188 194 195 170 174 179 192 193 186 187 203 197 198 196 189 190 |
| c | 14109–17586 | 42 | 191 200 202 205 206 207 208 201 199 218 219 220 209 214 221 223 224 210 211 216 227 228 229 230 212 213 215 217 225 226 231 232 236 247 237 238 243 255 234 235 245 246 |
| d | 17587–20934 | 47 | 249 254 268 269 270 271 272 252 253 273 274 275 250 239 240 241 242 244 248 256 257 258 259 260 261 262 263 280 282 286 287 288 289 264 265 267 294 295 283 284 285 266 276 277 278 279 281 |

Brief for each agent (Sonnet), filling in the share letter:

> Read-only. Do not build anything and do not edit any code. Read
> `notes/TS21-PLAN.md` in the `ts21` worktree, then the rubric. Read
> your share of entries with
> `git -C /nvm/work/swe-ts20 show fd605d3:notes/UPSTREAM-BUGS.md | sed -n 'FIRST,LASTp'`.
> For each entry, decide using the rubric. Quote the size the entry gives
> and convert it. For "same fix as" and "needs", use
> `notes/ts21/map.tsv`; look at the code at `fd605d3` only where the
> entry does not settle it. Write one row per entry to
> `notes/ts21/triage-X.tsv`. Do not commit. End with a count per
> decision and the rows you marked ASK.

Row schema (tab-separated):

| Column | Contents |
|---|---|
| `bug` | the entry's number |
| `title` | 80 characters at most |
| `category` | one of memory, crash, hang, thread, wrong-answer, order, precision, absurd-date, message, swetest, perf, docs |
| `setup` | `standard`, or `rare:<which>` |
| `size_quoted` | the size as the entry gives it, with units |
| `size_arcsec` | the converted size, or `n/a` for a non-number |
| `decision` | KEEP, ACCURACY, DROP or ASK |
| `reason` | 120 characters at most |
| `same_fix_as` | other bugs that are layers of the same problem |
| `needs` | bugs whose code this one builds on |

### Chunk 3: triage of the never-merged bugs

Same rubric and same schema, written to `triage-e.tsv`. The sources are
in `notes/cleanup-archive/` on `ts20-archive`: the state log, the
`swe-f291` patch (291–293, with notes entries) and the partial diffs.

| Bugs | What was found (from the state log) |
|---|---|
| 290 | Makransky cusps: the formula misses "90 +" for md > 90. G58 and `house_fix_off` need re-pointing. |
| 291–293 | Rise and eclipse functions leave geopos as the observer; SE_EARTH in rise_trans; SE_ECL_NUT reads `pla_diam[-1]` and is not refused as a body |
| 296 | `ssypl_aux` acos fold before 5650 BC |
| 297 | 'H' with the MC beyond the zenith |
| 298 | Sunshine `house_pos` in the polar circles |
| 299 | Koch `house_pos` at MC/IC fails |
| 300 | 'U' on the equator, 3.6e-6 deg |
| 301 | The strict setting refuses Mars and Pluto at semo_18's start + 0.001 without semo_12 |
| 302 | Sun SPEED refused there, or an extra note |
| 303 | The de406 warning lost in the true node's SPEED3; asteroid SPEED3 says "sun: sun:"; the same in `swe_nod_aps` |
| 304 | swetest uses TT as UT for houses and azalt |
| 305 | swetest -fq on cusps |
| 306 | swetest -fW and -fw on cusps |
| 308 | Mean node and apogee SPEED refused 1/32 day inside the ends |
| 310 | The search-start test names the wrong file at UT 7e11 |
| 311 | `jdut1_to_utc` far inversion |
| 312 | NONUT star past Vondrak's range |
| 313 | `observer_accel` across a model step, 1.5e-7 |
| 314 | Polaris topocentric, the Moshier Sun's light time across a step, 3e-9 |
| 315 | The Moon's apparent place steps 4.9e-5 deg for its light time after an Earth step |
| 316 | `nod_aps` geocentric Mars deflection jitter, 6.7e-5 deg at JD 7090348.28 |

Seven more were found but never numbered:

- `swe_azalt` with atpress 0 gives NaN above 44307.7 m;
- `calc_dip` is NaN at negative heights;
- `get_orbital_elements(SE_EARTH)` depends on call order, by 1e-11 AU;
- `orbit_max_min(SE_EARTH)` geocentric answers 1.999993/0/0;
- `vis_limit_mag` leaves `dret[7]` unzeroed on refusal;
- `lun_eclipse_when_loc` started at an eclipse's maximum returns that same
  eclipse;
- `heliacal_ut`'s height message ends in a stray "\n".

Numbers 307 and 309 were never assigned.

### Chunk 4: consolidation and review sheet (Opus)

1. **Merge** the five triage files with the pre-loop bugs, which are all
   KEEP.
2. **Re-check** every ASK row, every rare-setup KEEP and every ACCURACY
   row against DROP. Spot-check 10% of the rest.
3. **Group into fixes** (`F01` …). A fix is one problem as a caller sees
   it: all its layers, both pre-loop and loop-era. Where a kept fix's final
   code includes a layer that was DROPped, decide per case whether to
   apply the fix without that layer, or to keep the layer because it
   cannot be separated.
4. **Dependencies.** Find helpers introduced by dropped bugs that kept
   fixes use. Each such helper travels with the first kept fix that needs
   it. Order the fixes so that each one builds on `main` plus the fixes
   before it.
5. **Non-bug commits.** List the pre-loop ones and keep them as their own
   fixes: fast default nutation (G32) with `swenut2000a_sparse.h` and the
   android link; `-ffp-contract=off`; the obliquity-rate, Vondrak-rate and
   interpolated-apsides performance work; the sidtime zeroing; sescan;
   the test-infrastructure fixes. Re-baselines are not fixes: they are
   redone in chunk 6.
6. **Clusters.** Group the fixes by the code they touch, so that each
   chunk 5.x touches one area. Mark each fix `transplant` (Sonnet: the
   fixed side, carried over as is) or `rewrite` (Opus: a function whose
   layers must be written as one). Expect rewrites in `swe_nod_aps`,
   `lunar_osc_elem`, `app_pos_etc_plan`, `app_pos_etc_moon`,
   `app_pos_etc_sun`, the fixed-star paths and `swecalc`.
7. **Write `notes/ts21/REVIEW-SHEET.md`:** one line per fix (id, bugs,
   decision, category, setup, size, reason, files, estimated lines),
   grouped by cluster, with totals. Add a table of the dropped bugs with
   their reasons, and the old-to-new bug numbering. Draft
   `notes/accuracy.md`: for each item its size, setups, the `ts20` commits
   and why it is real rather than noise.
8. **Stop.** The owner signs off on the sheet, or changes lines in it.
   Nothing is applied before that.

### Chunks 5.x: apply, one cluster each

For each fix in the cluster, in the order the sheet gives:

1. **Read** the fix's hunks at `fd605d3` (the fixed side only; see
   `hunks.tsv`) and its entries.
2. **Write the change on `ts21`** against the code as it stands
   there:
   - no `SWE_UPSTREAM_COMPAT` and no `#else` copy;
   - all of the fix's kept layers as one coherent change, and none of the
     code that belongs to dropped or ACCURACY items;
   - where `ts20`'s code is layered (retries, `goto`s, pasted checks,
     per-fix step sizes), write it straight: one helper for a repeated
     check, and one step-size policy per kind of speed;
   - comments explain the code, and never say "bug NNN".
3. **Test.** One test file per fix, taken from `ts20`'s gate test or tests
   and trimmed. Drop the both-builds part and the G24 case. Keep the
   comparison with an independent reference where there was one. Wire it
   into `tests/Makefile`.
4. **Notes.**
   - One entry per fix in `notes/UPSTREAM-BUGS.md`, numbered from 23 in
     commit order, using the numbering table in the review sheet. Keep it
     short: what a caller sees, where, the size, the shape of the fix.
     Add its Contents row.
   - No `sweph.h` compat bullet, and no per-fix REVIEW clause.
5. **Check:** `make -j4 LTO= libswe.a`, then in `tests/` this fix's test
   and `main`'s existing tests, with `NPROC=4` and a 9 s timeout per file.
6. **Commit.** One commit per fix. The subject says what changes for a
   caller. The body gives the cause, the size, the test and the old `ts20`
   bug numbers.

Close each chunk by updating the progress log here, ticking the fixes in
the review sheet, and committing.

### Chunk 6: final pass and new PR

1. **Full checks**, one at a time and 4 processes at most:
   - `make check`;
   - the warning matrix, one cell at a time;
   - the sanitizer builds, one at a time;
   - the thread-safety gates.
2. **Re-baseline** G1, G10, G17 and golden. Each change has to be
   explained by a kept fix.
3. **Compare with upstream** `3fd0f95`: a sweep over bodies, flags, dates
   and house systems. Every difference larger than 0.001" must trace back
   to a kept fix.
4. **Benchmark**, sequentially, against `main` and upstream.
5. **Bookkeeping:**
   - check that the bug numbering is contiguous and matches the Contents
     table;
   - the REVIEW row for this round;
   - the version;
   - the PR body.

   Remove `notes/ts21/` and this file in the last commit. They stay in
   the branch history. `notes/accuracy.md` stays.
6. **Push and open the PR.** With the owner's go-ahead, push `ts21`
   and open the new PR. Close #33 with a link to it and wait for CI.
7. **After the merge:**
   - reinstall the system libswe as the README describes;
   - tell the astromcp and Astrolog sessions;
   - remove the old `ts20-f*` branches and `/nvm/work/swe-f*` worktrees,
     after checking that `ts20-archive` is pushed.

## Progress log

| Date | Chunk | State | Notes |
|---|---|---|---|
| 2026-10-25 | 0 | done | Plan written. `ts20-archive` made (`122f995`), and the stopped agents' work saved into it. Nine agents stopped; the loop cron cancelled. |
| 2026-10-25 | — | replanned | The owner chose a short ts.20 of the critical fixes (`notes/TS20-RELEASE.md` on `ts20-release`), so this plan became ts.21's. Chunk 1 step 1 is done (`notes/ts21/commits.tsv`). |
