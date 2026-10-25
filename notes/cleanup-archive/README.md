# ts20 archive: work not merged into ts20

This branch is `ts20` (tip fd605d3) plus this directory. It keeps what existed
only on scratch disk when the overnight run on PR #33 was stopped
(2026-10-25). The cleanup plan is `notes/CLEANUP-PLAN.md` on branch `ts20-clean`.

| File | What it is |
|---|---|
| `swe-f291.b9c5efe.patch` | Bugs 291-293 (rise_trans/eclipse functions leave geopos as observer; SE_EARTH in rise_trans; SE_ECL_NUT pla_diam[-1] read), committed in worktree swe-f291, never merged. `git am` applies it onto fd605d3. |
| `swe-f296.uncommitted.diff` | Partial work on 296 (ssypl_aux acos fold before 5650 BC), 299 (Koch house_pos at MC/IC), 300 ('U' on the equator); uncommitted, unfinished. Base 1fc11a3. |
| `swe-f301.uncommitted.diff` | Partial work on 301-303 (strict setting at semo_18's start; Sun SPEED there; de406 warning lost); uncommitted, unfinished. Base 9d96e4c. |
| `coordinator-state-log.md` | The coordinator's running log: every dispatch, merge and decision, with the bug descriptions of 290-316 that never reached the notes. |
| `PR-ts20-body-draft.md` | The PR body drafted for ts20, including the "Not changed" decisions (e.g. 233). |

The agents on 290, 297, 298, 304-306, 308, 310-316 had nothing saved; their
bugs are described in the state log only.
