# ts20 merge state (coordinator notes)

ts20 head: (see git) (117 G114 mean node/apog central diff; 9d97c0a D6 mine; 116 d95f12a G113; 556c073 notes 73/87/95 from attribution; 17368b4 REVIEW range, e729a15 Makefile tidy + G53 leak; 109 e5c5042 G108 Moshier bary everywhere: calc-moseph +1.85% instr, moon bench +55%, stars +49-63%, fict +22-32%; 114 b962846 G111ext; 113 0dc2960 G111 seorbel hang/star load error, + star table leak, fixstar NULL serr OK; 112 649122f G109 part 2 orbel = calc fallback/strict, unguarded (fork fn); 106 93d6825 G103 header bounds, outside compat guard, no G24 -- note in PR; 110 c8b4633 G109 path warnings only when no path; 108 afa5f55 G104ext+G107: nod_aps bias once, EMRAT, orbital elements GMs+Pluto; 107 198832a G106 first calls read files along SE_EPHE_PATH; 102 f8aac55 G101 Moshier light time with v_Sun; 105 6dfd8f7 G104: nod_aps GMs from ephemeris, Pluto had Mercury's mass; G74 tightened; 103 f1682ea G102: files kept open on non-change, same-path no longer rereads disk (swe_close first) -- mention in PR; 104 19995e7 G99ext + fclose(NULL) crash fix in path setter (both builds), 95 a2131d1 G94, 100 30960ed G99, 101 fd5d529 G100, 97 14cecf5 G96) (94 notes cddb081, 91 76f2237, 92 54d5799, 96 fd68b08 G95, 89 e359ec8 G88, 98 189de19 G97) (/nvm/work/swe-ts20), local, NOT pushed. Original PR base 2e39062 (pushed, CI green).

## Rule (user, 2026-10-24): behaviour bugs always; residuals only > 1e-9 deg / 1e-8 deg/day (in brief2.md)

## Running agents (bug -> agent id, worktree)
- agent aaaab17a6461be92d idle (files/seorbel context), nothing new after 114
- (done) 97 merged 14cecf5
- agent a5c9595aa0ded7ff2 idle (nothing new after 112)
- NOTE for PR: this fork's default fallback setting is strict
- 111 swe_get_orbital_elements true anomaly near aphelion 4.4e-9, e from vector, periods without planet mass -> aef43f533b99a4aea, swe-f95 (opus), extend G107; 95 merged a2131d1 (G94; Moshier Moon velocity analytic, calc-moseph 1.045->0.957s)
- 99 DROPPED (below floor): merged notes as 65a0d3d (entry 96 para + entry 89 note on 1800.0 first-call Moon, already fixed by 89); agent a78d2ad7b8c40160a idle; 96 merged fd68b08 (G95)
- gates reserved: G96 bug 97, G97 bug 98, G98 bug 99, G99 bug 100, G100 bug 101, G101 bug 102, G102 bug 103, G103 bug 104, G104 bug 105

## Merge procedure
cherry-pick; for conflicted files: git checkout --conflict=diff3 F; python3 $CLAUDE_JOB_DIR/tmp/union.py F;
then python3 tmp/fixdup.py . (compat.c decl lines, REVIEW ts.20 row dup; check "Moon's pull on it (UPSTREAM-BUGS 62" appears once);
python3 tmp/phonyfix.py tests/Makefile (after any tests/Makefile merge); python3 tmp/entcheck.py <hash> (UPSTREAM-BUGS entries not mixed; once misplaced 81 into 85 -> fixed by fixup commit 9e9065f);
check Contents row duplicates when an agent retitles an entry; build; run gates; commit.

## Final pass TODO
- autosquash fixup 9e9065f (GIT_SEQUENCE_EDITOR=true git rebase -i --autosquash 2e39062)
- amend bug 53 commit (9cc76fa) message: mention extension of bug 63 rounding correction to Moshier (done by coordinator during merge)
- full make -k -C tests check; warning matrix gcc+clang-22 (clang may be missing here) -O0..-Os default+compat; -Og too
- thread gates (sweconfig.c changed by bug 83)
- re-baseline G1 (make -C tests baseline), G17, G10; classify moved rows
- bench calc-swieph / calc-moseph
- renumber gaps: bug 57/G56 unused, G71 unused (72 used G55(c)); sort UPSTREAM-BUGS entries + Contents rows numerically; fix REVIEW "passes with" range (has junk like G95)
- tests/Makefile list consistency check (script in history: names vs rm lists vs check lists)
- PR body: round-2 section applied to GitHub 2026-10-24 (status line, tables for 52-101, cost, floor, in-progress 102-105; backup PR-ts20-final.before-round2.md). At the end: add 102-105 rows, drop Status + "Still in progress", redo Verified, final bench numbers. Old note: cost lines (interp apsides 1.57x/1.33x perigee; true node speed +35%/+58%; fixstar Moshier speed +18%; pctr +12%), Verified section (independent gate sweep: all new gates fail on 2e39062 and upstream)
- push ts20, CI, gh pr edit 33 -R nrvate/swisseph --body-file ...
- reinstall system libswe (backup ts.19 at /nvm/work/libswe-ts19-backup), notify astromcp + Astrolog sessions
- remove $CLAUDE_JOB_DIR/tmp/old worktree (2e39062) and scratch dirs
- restored entries 65/69/70 (cfb9144); answered Astrolog delta-T question

- agent aa8c4567de04a09d5 idle (Moshier context) after 109
- USER DECIDED fix everywhere: Moshier apparent distance residual (v_Sun on apparent path, +~12% calc-moseph accepted) -> folded into 109 item (d)
- data, record: Swiss barycentric Sun segment-join jumps 1.2e-8 AU -> light-time positions jump up to 4.9e-7 deg for ~1 light time
- 97 merged 14cecf5 (nakshatra split under SE_SIDM_USER shifted 3d20m; Mercury 9e-11 below floor recorded)
- no-effect, left: swetest.c:1341 sid_mode & SE_SIDM_USER (t0 ignored for predefined modes); invalid mode+SIDBITs -> Fagan (upstream handling); G51 Jupiter 500AD 1.5e-11 sawtooth (below floor)
- cleanup: /nvm/work/swe-f91-scratch/up2 up3 (agent rm blocked)
- (done fef9543) stale 'Not fixed here' notes 81/82/83
- below-floor record: Moshier Moon speed ~5e-9 deg/day systematic vs position rate (flat with step) -- mention in PR body / an entry note
- user: avoid closing/opening files for no reason -> added to 103(b) as audit of all non-change close paths
- below floor: swe_calc geocentric Moon jitter 2e-14 of distance (all ephemerides)
- cost line for PR: Moshier Moon faster (moon bench 0.90->0.565 s, calc-moseph 1.045->0.957 s)
- check-golden baseline already fails since before fd5d529 -> re-baseline in final pass with G1/G10/G17
- below floor: Swiss/Moshier Earth from EMB with AA EMRAT not DE441's (2e-10 deg geocentric)
- baseline attribution agent a2e0904bd641bf26f in /nvm/work/swe-base (detached f00be0c), report /nvm/work/swe-base-scratch/attribution.md, script attrib.sh <from> <to> for 109/113 later
- 111 merged f00be0c (Moon period = osculating ellipse's, user: 'most scientifically accurate')
- Makefile lists: all G51+ gates consistent. Pre-existing gaps to tidy in final pass: .PHONY list (line ~1322) lacks dtobs fictlt moonlt jpltdb sidsplice sidera obliq nutgrid nutspeed sidnod spicarv; check list lacks jplhor; second rm list (~1514) lacks nutspeed (and others by design?)
- FINAL: REVIEW 'passes with' list is unsorted and missing gates (G65 G68-G70 G74-G76 G101 G103 G105 G108...) -> rewrite as a range minus unused numbers
- FINAL: baseline attribution must also cover fef9543 (notes), 0dc2960 (113), 109, 114
- (2026-10-25) user: fan out one agent per item, keep agents busy (memory fan-out-agents.md)
- warning matrix agent a9060dc5a6f382fdb RESUMED throttled at e5c5042 (-j4, -flto=1, sequential)
- thread/sanitizer agent afadc509a88f71c95, /nvm/work/swe-tsan (ts20-tsan from 0dc2960)
- independent review (no fixes, independent items) ae98ff7d8eb431289, /nvm/work/swe-review (ts20-review from 0dc2960)
- idle with context: a5c9595aa0ded7ff2 (config/path), aef43f533b99a4aea (nod_aps/orbel), ae7c7dbf50a2f177e (ayanamsa), a78d2ad7b8c40160a (fict bodies)
- /loop cron 75dc8fa4 every 1m: 'continue wrapping up pr #33; if ready, kill this cron' (CronDelete 75dc8fa4 when done)
- rerun warning matrix throttled only when an agent slot frees (keep total <= ~4 agents x 4 procs)
- FINAL: REVIEW gate list also missing G101 (and others) -> rewrite as a range
- user (2026-10-25): 'dont stop until we're good to go on #33' -- loop 75dc8fa4 keeps going
- 115 crash: nested config sync mid-swe_calc closes JPL file (fork's) -> afadc509a88f71c95 (tsan agent), swe-tsan, G112
- 116 swe_set_astro_models old version sets tid_acc but publishes only SID group -> a5c9595aa0ded7ff2, swe-f92, G113
- user: small stuff fix myself (memory fix-small-yourself.md)
- DONE: REVIEW passes-with range; Makefile .PHONY/rm tidy
- attribution DONE for 2e39062..f00be0c: all moves explained; report /nvm/work/swe-base-scratch/attribution.md; baselines at f00be0c in scratch; rerun attrib.sh f00be0c <tip> with MAXPROC=2 at the end
- below floor: Pluto speed ~2e-10 deg/day vs its positions (golden)
- 117 SE_MEAN_NODE/SE_MEAN_APOG speed 8.7e-9/2e-8 deg/day off rate -> ae7c7dbf50a2f177e, swe-f91, G114
- golden coverage gaps (64, 102, 65 unseen by G1): fine, gates cover
- review DONE: D1 D4 D5 D7 -> 118 aaaab17 swe-f89 G115; D2 D3 -> 119 ae98ff7 swe-review G116; Moshier Moon under strict -> 120 a78d2ad swe-f81 G117; sefstars partial-load reparse -> 121 aef43f5 swe-f95 G118
- mine (small): D6 neighbour 'not found' names wrong file (sweph.c ~3550); moong_t 0 sentinel (swejpl.c 131/1017); swe_get_astro_models sdet ~480 bytes > AS_MAXCH
- 122 swe_set_ephe_fallback not in any config group (other threads don't see it) -> fresh agent
- union.py duplicates Makefile .PHONY continuation lines -> check .PHONY after each Makefile merge (make -n)
- user: prefer fresh agents over resuming (memory fresh-vs-resume-agents.md)
- 109 REGRESSION: G99 fails 17 rows (helio/bary Moon with Swiss planets + Moshier Moon fallback, 0.15-0.5 deg) since e5c5042 -> sent back to aa8c456 (swe-f102)
- below floor: mean node/apogee century-boundary kink up to 1e-8 deg/day (upstream's model)
- RETIRED agents (big context): ae7c7db, a5c9595
- tests/Makefile forces -j$(NPROC)=12: final full suite must run with NPROC=4 (or MAKEFLAGS override)
- cleanup: /nvm/work/swe-f91-scratch/b117/up
- merged 121 (star catalogue failed load kept); mine: 0a1febc moong_t sentinel (no reachable path); astro_models sdet: NOT a defect (swephexp.h documents 4000 bytes)
- RETIRED: aef43f5 (big context)
- user: NO tsan/stress/matrix until fixes done ('get the fixes in then you can fuck with your gadgets'); warning agent STOPPED again; brief updated
- merged 119 (4379322 adopted file settings forget); mine 0b6f0c5 hel memos in jpl_only branch; RETIRED ae98ff7 (reviewer)
- merged 118 (314ff15 G115), 122 (e1158f3 G119), 109 regression (ca6c910, G99 passes again)
- 123 swe_close config reset: generation zeroed + kept settings reverted by next sync -> fresh agent, swe-f122, G120
- RETIRED: aaaab17, aa8c456, a6d5e6f
- running: 115 (afadc509 tsan agent, told no tsan), 120 (a78d2ad), 123
- merged 120 (4b2d8b2), 115 (4f30ce6 G112); mine 085152e swi_get_observer serr; mine 1811f46 G115 P9 fallback (bug 120 strict broke it)
- running: 123 (afd3a7d8 fresh, swe-f122, G120), 124 (adc066f7 fresh, swe-f124, G121: hold/release on searches/houses/rise/eclipse)
- full check at 1811f46 running -> tmp/fullcheck-1811f46.log
- full check @1811f46: baseline-only fails G1 G1b G10 G17 G11(threadtiers vs baseline.txt) ephe-ci; real: G19 eopload (bug 106 fixture offset<3 -> fixed f48cd8b mine), G27 fictlt + G65 lunarpulleph (bug 109 mosh_sun_shift Taylor over VEA 462 d) -> bug 125 fresh agent a328e35, swe-f125
- /nvm/work/swe-bisect worktree (detached) for bisects: remove in cleanup
- PR body on GitHub updated 01:2x with rows 115-122 and in-progress 123-125 (backup tmp/PR-ts20-final.before-115rows.md)
- merged 123 (7a3ce7a G120). New from its report: 126 nutation cache on swe_set_interpolate_nut (fresh a5108121, swe-f126, G122); 127 swe_close keeps topo + sid mode per manual (fresh a84eeb9d, swe-f127, G123)
- running: 124 (adc066f7, G121), 125 (a328e35, extend G27/G65), 126, 127
- final pass: root make needs LTO= (default -flto=auto); include check-cfgleak
- merged 124 (63f62f8 G121, 43 fns held). New: 128 first call reads unstarted ctx (fresh a8da55c4, swe-f128, G124)
- running: 125 (a328e35), 126 (a5108121), 127 (a84eeb9d), 128 (a8da55c4)
- merged 126 (2fe022d G122). New: 129 nut interp nodes survive model change (fresh a067f5b9, swe-f129, extend G122 or G125)
- running: 125 (a328e35), 127 (a84eeb9d), 128 (a8da55c4), 129 (a067f5b9)
- merged 125 (083dcc6, G27 rebuilt 1e-6", G65 + orbel row). New: 130 swe_calc_orbel geocentric frame bias unpaired 1.7e-6 deg (fresh abd409b6, swe-f130, G126)
- running: 127 (a84eeb9d), 128 (a8da55c4), 129 (a067f5b9), 130 (abd409b6)
- merged 127 (a7ac89a G123; fixed entry 123 note union misplacement by hand). New: 131 tidal term after swe_close first UT call (fresh add5ec18, swe-f131, G127)
- final pass: run check-setest (G8) too
- running: 128 (a8da55c4), 129 (a067f5b9), 130 (abd409b6), 131 (add5ec18)
- merged 128 (958bd9c G124; moved 124's pointer paragraph back by hand). New: 132 time_equ JPL flag on adopted file (fresh a9c9eef3, swe-f132, G128); 133 Sunshine saved_sundec wiped by start memset (fresh aa4ba23a, swe-f133, G129)
- running: 129 (a067f5b9), 130 (abd409b6), 131 (add5ec18), 132 (a9c9eef3), 133 (aa4ba23a)
- merge note: union.py places a theirs-side paragraph appended to an entry that preceded "How these were found" at the end of whatever entry now precedes it; entcheck catches it ("mixed N")
- merged 129 (76a2789; G122 close cases adapted to 127 by me, compat 74), 130 (010da74 G126). New: 134 nut nodes JPLHOR key (fresh a739680b, swe-f134); 135 orbel Moshier bary answers + 136 orbel EQ_DATE suspected (fresh af82f8f8, swe-f135)
- running: 131 (add5ec18), 132 (a9c9eef3), 133 (aa4ba23a), 134 (a739680b), 135/136 (af82f8f8)
- merged 132 (414742a G128). Its noticed tid_acc getter vs delta t after close+JPLEPH -> sent to 131 agent (add5ec18) as an extra case
- running: 131, 133 (aa4ba23a), 134 (a739680b), 135/136 (af82f8f8)
- merged 133 (a8e7e99 G129, fork-only fix, unguarded). Its noticed fixstar_mag handle leak is compat/upstream only: no action
- running: 131 (add5ec18), 134 (a739680b), 135/136 (af82f8f8)
- merged 131 (fda5627 G127, covers 132's getter case). New: 137 stale jpldenum on adopted path change + 138 tid_acc getter vs guess after path re-set (fresh ace8f12e, swe-f137, G132)
- running: 134 (a739680b), 135/136 (af82f8f8), 137/138 (ace8f12e)
- merged 135 (17f8a5f), 136 (4bed4ca) G126. Decision (mine, per entry 112 precedent): unguard 135/136 (fork fn). Fresh ad727c28 swe-f140: unguard + 141 return flag + 140 topo speed Earth-centred orbit 1.8-4.6e-8 deg/day. Fresh a261595d swe-f139: 139 fallback bary answered with Moshier helio (G133)
- running: 134 (a739680b), 137/138 (ace8f12e), 139 (a261595d), 140/141 (ad727c28)
- merged 134 (410a4c8 G122 246). New: 142 CRASH ayanamsa_ex/houses_ex2 JPLHOR no EOP + 143 second thread no EOP + 144 path keeps old EOP (fresh a6c6610c, swe-f142, G134); 146 interp across JPLHORA_3 1962 step 1.95e-2" (fresh a4cd8a0f, swe-f146); 145 deltat_ex(t,0) thread-dependent 4.3 ms -> sent to 137/138 agent ace8f12e
- running: 137/138/145 (ace8f12e), 139 (a261595d), 140/141+unguard (ad727c28), 142-144 (a6c6610c), 146 (a4cd8a0f)
- merged 137 (3efe9b9), 138 (3f06f24), 145 (f8837a6) G132; 4fc8669 notes fix (131 para back, 134->145); 139 (65f90ca G133)
- DECISION (mine): 148 reverses F4 for JPL DE: path setter re-reads kept JPL file's DE along new path (fresh-process equivalence); 138's expectation changes accordingly
- New: 147 adopted path keeps swe_deltat.txt/leapsec + 148 (fresh a8a718b0, swe-f147); 149 nod_aps Moshier bary + 150 calc_pctr return flag (fresh a7a14a91, swe-f149)
- running: 140/141+unguard (ad727c28), 142-144 (a6c6610c), 146 (a4cd8a0f), 147/148 (a8a718b0), 149/150 (a7a14a91)
- merged 146 (30694b6 G122 350). New: 151 Woolard Moon anomaly 13255523.59 typo + 152 grid at |J|>5.6e8 (fresh a435aa18, swe-f151, G138); 153 speeds across 1962 JPLHORA step (fresh a63953b6, swe-f153, G139)
- running: 140/141+unguard (ad727c28), 142-144 (a6c6610c), 147/148 (a8a718b0), 149/150 (a7a14a91), 151/152 (a435aa18), 153 (a63953b6)
- merged 135/136 unguard (6a80924 + fixup d05f2b8: union kept removed G24 cases/bullets -> autosquash in final pass), 141 (26ae9d8), 140 (1b56b3b, swi_get_observer UT rounding; G1 moves 236 rows: topo <=8.5e-10 deg, ecl times 1 ulp, solecl attr 5e-7), 280e81f G72 header (mine)
- union.py hazard: a commit that DELETES lines in a conflicted block -> union keeps them. Check deletion-only hunks by hand.
- running: 142-144 (a6c6610c), 147/148 (a8a718b0), 149/150 (a7a14a91), 151/152 (a435aa18), 153 (a63953b6)
- merged 142 (5a9ff43 crash, unguarded), 143 (8ece6b0), 144 (1c01d3d) G134 78. New: 154 jpl file <DE403 keeps EOP, 155 load_dpsi_deps leak, 156 file open on alloc fail (fresh a1cccd27, swe-f154)
- running: 147/148 (a8a718b0), 149/150 (a7a14a91), 151/152 (a435aa18), 153 (a63953b6), 154-156 (a1cccd27)
- mine: 3d8cd07 157 Lahiri ICRC t0 (G138 d, G24 lahiri_icrc_t0_off)
- merged 154 (7e1f45e; G134 CH_B fresh ref adapted to 148 by me), 155 (ff05a4e), 156 (166fd97) G134 100. RETIRED a1cccd27
- New from 154 agent: 171 broken eop_finals.txt (-3) discards complete eop_1962_today.txt (JPLHOR->APPROX 4.2e-2"); 172 alloc fail msg "not found" (message only) -> mine or next agent
- dispatched (brief2 NPROC=2): ae4ad4a9 swe-f159 (159 Owen/IAU1976 1799/2202 boundary rates, 160 short-term prec window edges, 163 nutation_rate rounding; G139/G140); a210f84e swe-f161 (161 mode3 bias side at step, 162 approx_jplhor velocity; G141); aa06e252 swe-f164 (164 JPL named-not-found then path, 165 default de431 first delta t, 166 Moon-files-missing tidal; G132/G142); a6d2e261 swe-f167 (167 Moshier Sun apsides, 168 nod_aps centre from answering eph, 169 centre flag precedence, 170 nod_aps_ut delta t after fallback; G143)
- 158 (pctr retc overwrite): mine, next
- mine: 62684e6 158 pctr centre read at t (G144 pctredge, G24 pctr_ctr_edge_ret); b2456f5 171 finals cut keeps prefix (SWI_EOP_FINALS_CUT=3, warning) + 172 NOMEM msg (unguarded) G134 108, G24 eop_finals_cut_off
- G134 compat now fails 27 (incl. 2 from my 148 ref change in 7e1f45e)
- swe-bisect left at 62684e6 (detached), cleanup in final pass
- numbering: next bug 173, next gate G145 (G140-G143 reserved for running agents)
- merged 161 (fe7c56c), 162 (10526f6) G141 jplhorbias. RETIRED a210f84e. mine: 173 offset table backwards (G141 75, G24 jplhora_table_jump); 162's fallback-keeps-APPROX recorded not a defect
- next bug 174, next gate G145
- merged 164 (21d78a2), 165 (18eb6d4), 166 (dfeedc7) G132 251, G142; 167 (250e8d2), 168 (01bc640), 169 (5dc826e) G143, G133. RETIRED aa06e252, a6d2e261
- HELD 170 (76ef3cc on ts20-f167): breaks G142 7/14 (Moon mean points read the Moon -> MOSEPH -> delta t redo). Needs 174 first.
- dispatched from 5dc826e: a639b7b3 swe-f174 (174 Moon mean save-area read/refusal, re-land 170, 179 outputs on ERR, 149 comment; G143/G142/G145); a6c6f4dd swe-f176 (176 osc frame bias after fallback, 178 helio/bary aberr/defl, 180 Moshier Earth osc apsides speed; G146); af96ae5d swe-f175 (175 first delta t with J2000 files absent, 177 Moshier deflection SUNBARY slot; G142/G147)
- still running: ae4ad4a9 swe-f159 (159/160/163)
- next bug 181, next gate G148
- merged 159 (1818dd5), 160 (19cb5bc), 163 (ecbfb9f G140 nutderiv). RETIRED ae4ad4a9. mine 182 (b68f3c6 eps offset table backwards, G141 76, G24 jplhora_eps_table_jump)
- dispatching from b68f3c6: swe-f181 (181 sidereal time node rate across windows 50", 186 true node/osc apogee speeds near edge, 187 house speeds near edge; G148), swe-f183 (183 Owen segment rate steps, 184 Moshier Moon frame across edges, 185 JDATE fict/orbel frame across edges, 188 2000A/B argument rounding 4e8-1e9 days 6e-9 deg; G149)
- next bug 189, next gate G150
- merged 176 (be75118), 178 (b4b82b5 G146 nodapshel; 9.6e-4 deg real size), 180 (c80d9ae Moshier EMB) . RETIRED a6c6f4dd
- dispatching from c80d9ae: swe-f189 (189 swe_get_orbital_elements Moshier EMB wrong Moon, 190 Sun topocentric nodes/apsides no parallax, 191 Moshier Earth made with short-series Moon 1.3e-7..7.9e-7 AU; G150)
- next bug 192, next gate G151
- 174/170/179 (ts20-f174 b9aa9b2 e4aaa82 98476cc) conflict with 176/178/180 in swecl.c (union duplicated decls) -> aborted, RESUMED a639b7b3 to rebase onto c80d9ae (its context ~85k)
- new from 174 agent: 192 strict over-refusal: Sun's mean/osc points and Earth helio/bary mean points refused w/o Moon files though Moon cancels; 193 strict under-refusal: nod_aps planets geo/topo answered from Moshier-Moon Earth where swe_calc refuses (verified: swe_calc Mars -1, nod_aps 0, 1.1e-9 deg) -> give to a639b7b3 after rebase; 194 sweph.c swe_calc TOPOCTR|HELCTR/BARYCTR passes bug 120 strict test (32770, 9e-10) and later TOPOCTR answered from save area -> pending (fresh or 175 agent follow-up)
- next bug 195
- merged 175 (fbf8c18->), 177 G133 1136, G142 28, G109 ok. RETIRED af96ae5d
- new: 195 topocentric swe_fixstar2 fallen to Moshier places observer with files' delta t (2e-8 deg, 1.35e-7 deg/day); cost note: 175 adds ~100 stat()/path element per path-setter call in a dir with no Swiss files (150 us), same path included
- dispatching swe-f194 (194, 195; G151)
- merged 183, 184, 185, 188 (G139 330, G140 48). RETIRED aec42541
- noticed: Owen 1990 pieces step in position 2.2e-8 (published model; not a defect, record); 9.9e-10 deg/day below floor; NEW 196: swe_calc(SE_ECL_NUT) accepts any date, OK flags, dpsi 1.2e72 deg at J=1e82, NaN from ~1e84
- dispatched a7566bb4 swe-f196 (196 SE_ECL_NUT absurd dates + survey; G152). Gates: G150 189-agent, G151 194-agent, G152 196-agent; next gate G153, next bug 197
- merged 194 (7e45af0 G117 472), 195 (cfa8ed3 G133 1165). RETIRED aef2f80d
- dispatching swe-f197 (197 save-area answer skips bug-120 strict test after swe_set_ephe_fallback(0); 198 fixstar JPLEPH on de200 outside span refuses where swe_calc falls to Moshier; G153). next bug 199, next gate G154
- merged 174 (7e139e7), 170 (f8290ed; G133 1205, upstream 469), 179 (93ea546), 192 (1581067), 193 (aad0ebc). RETIRED a639b7b3. Accepted its change of G133 (h): Moon mean points compare with the strict request (consistent with swe_calc mean node). 194 already covers its TOPOCTR|HELCTR note.
- merged 181 (8057c1a), 187 (55d9f04), 186 (tip). RETIRED ac30148d. Merge fixes (mine): G139 letters now (c) 184/185 bodies, (d) 181 sidereal time, (e) 187 houses, (f) 186 node/apogee; G139 454, upstream 339/358. frame_chosen_model() returns Owen PIECE numbers since 183 -> mapped to SEMOD_PREC_OWEN_1990 in swe_sidtime0_r (181) and swi_frame_model_hold (186): Owen joints not held (new item).
- dispatched from 09adb9b: a35eea4d swe-f199 (199 nod_aps OSCU speeds across window edges 0.16 deg/day -> hold; 201 Mars osc asc node speed 2.2e-5 off away from edges, unexplained; G154); acac80c6 swe-f200 (200 Owen joins: sidt table node rates 3.6e-9 bound + true node hold keeps piece; 202 Moshier node/apogee near window edges re-measure after 184; G155); a0652385 swe-f203 (203 sidereal houses JPLHOR_APPROX 1962 step 6.4e-4 deg/day; 204 Sunshine cusp 9 jump 7e-9 near 2378496.5-2^-11; G156)
- running: a7c2c1e5 swe-f189 (189-191, G150), a7566bb4 swe-f196 (196, G152), a3af4473 swe-f197 (197/198, G153), + the three above
- next bug 205, next gate G157
- merged 197 (27f84db G117), 198 (58f8f11 G133 1383, upstream 645). RETIRED a3af4473. 198 also fixed helio star parallax Sun=0 after fallback (same line; accepted)
- new from 197 agent: 205 save-area hit with fallback returns empty serr (computed answer had "using Moshier eph. for moon"); 206 star with SPEED at de200 edges refused (aberr-rate reads +-2^-9 outside span) where it answers without speed / Mars answers or falls; 207 JPL request with JPLHOR(_APPROX) fallen keeps Horizons bit (524548), Mars 1500 3.5e-4 deg off Moshier request; 208 fictitious body near Swiss files start answers wrong even strict (Cupido 2378496.6 in 1800-2400 dir, 4.33e-3 deg), app_pos_etc_plan_osc light-time reads main_planet_bary w/o eph check
- dispatched from 58f8f11: adfa7b19 swe-f205 (205 save-area serr, 207 Horizons bit after fallback; G117/G133 or G157); a1fed21f swe-f206 (206 star SPEED at JPL/Swiss edges, 208 fict light-time unchecked reads; G158)
- running: f189 (a7c2c1e5), f196 (a7566bb4), f199 (a35eea4d), f200 (acac80c6), f203 (a0652385), f205 (adfa7b19), f206 (a1fed21f). next bug 209, next gate G159
- merged 196 (a67593c G152 eclnutrange, G29, G24 ecl_nut_far_answered). RETIRED a7566bb4. Survey (probes in /nvm/work/swe-f196-scratch/probe, py): new items
  209 swe_sidtime long-term integrates from table end each call (>10 s at 1e12), (int)ceil overflow UB past 7.8e14, SIGSEGV at -1e300/-inf (NaN index INT_MIN)
  210 sprintf "jd %f outside ... range" overflow s[256] from |J|1e195 (swemmoon.c 776/1442/1515, swemplan.c 525, swecl.c 6218 into serr, sweph.c 4515)
  211 NaN date passes range checks: mean node/apogee crash corr_mean_node, Cupido Moshier 260 NaN, nod_aps Mars mean 0 NaN, fixstar2 Moshier 4 NaN, gauquelin 0 NaN
  212 swe_get_ayanamsa_ex(_ut) OK with absurd values (1e12 121.4 deg, NaN at 1e84, inf, NaN)
  213 swe_houses_ex2 from 1e20 ERR "polar circle" (obliquity 51 deg), NaN cusps from 1e84
  214 sidtime/sidtime0/azalt/azalt_rev NaN without error; after 196 azalt rotates by 0 past 1e12 (eps refused)
  215 swe_deltat_ex(NaN) 0 no serr; +-1e300 inf/NaN
  216 Swiss file name from revjul year int overflow past 7.8e11 ("seplm21474840.se1")
  217 non-default precession models' mean obliquity outside 0..180 past |J| 0.8e7..3.2e7, SE_ECL_NUT OK (Laskar 1.6e51 deg at 1e12)
  218 SE_ECL_NUT with SEFLG_NONUT returns stale cached nutation (both builds)
  219 RESIDUAL IAU 1980/1987 arguments in plain double, est 6e-9 deg at 1e9 (as 188)
  220 RESIDUAL IAU 2000B lacks SOFA nut00b fixed offsets -0.135/+0.388 mas (3.75e-8/1.08e-7 deg)
- POLICY (mine) for absurd dates: 196's bound |J|<=1e12 & finite for every date-taking fn; error return -> ERR+zeroed+"jd %.10g outside ..." msg; no error return -> NaN; bit for bit inside
- dispatched from a67593c: a4325ac0 swe-f209 (209, 214; G159); ab1583c9 swe-f211 (210, 211, 216; G160 + hostile.c dates, not run); aa3ad5a3 swe-f212 (212, 213, 215, 217; G161)
- HELD for next free slot: 218 NONUT stale nutation, 219 IAU1980/1987 double-double args, 220 IAU 2000B fixed offsets (one agent, G162)
- running (9): f189 a7c2c1e5, f199 a35eea4d, f200 acac80c6, f203 a0652385, f205 adfa7b19, f206 a1fed21f, f209 a4325ac0, f211 ab1583c9, f212 aa3ad5a3. next bug 221, next gate G163
- merged 189 (49d5d39 G107), 190 (158fcc8 G143 2558/1905; merge fixes: duplicated decl, header), 191 (tip, G150 moshemb). RETIRED a7c2c1e5. Cost note for PR: Moshier swe_fixstar2 with SPEED +18.6% instructions (694k -> 823k/call); calc-moseph +0.26%, moon-bench -1.3%. G75 Moshier rows 1.41e-11 -> 2.16e-11 deg/day (limit 6e-11). Moshier golden/baseline rows move with 191 (final-pass re-baseline).
- FINAL PASS TODO: sweph.c stale comments (earth_at_lt_sun, observer_accel bug-66 text) say Moshier's Moon from embofs -> fix after agents finish
- new from 189 agent: 221 nod_aps change of centre adds ICRS Sun/Earth/site to J2000-frame points (planets geo 3.5e-8 AU at J2000, 1.1e-7 AU -3000..3000; Moon topo 4.8e-12); 222 Moshier Mars at -3000 0.21 deg helio off files (0.59 geo), Jupiter 2.4e-3 deg ~constant -> investigate. Below floor recorded: Sun topo nod_aps depends on prior swe_calc topo Sun (6.2e-12 deg); mosh_earth_accel 5e-10..9e-10 AU/day^2
- dispatched from 10ad9cd: a932a2d6 swe-f218 (218 NONUT stale, 219 IAU1980 dd args, 220 2000B offsets; G162 or G140/G152); a7a57083 swe-f221 (221 nod_aps ICRS/J2000 centre mix, 222 Moshier Mars -3000 0.21 deg investigate; G163)
- running (10): f199 a35eea4d, f200 acac80c6, f203 a0652385, f205 adfa7b19, f206 a1fed21f, f209 a4325ac0, f211 ab1583c9, f212 aa3ad5a3, f218 a932a2d6, f221 a7a57083. next bug 223, next gate G164
- merged 203 (5646d28 G156 housejplhora). 204 NOT A DEFECT (file joint sepl_12/sepl_18 Earth 236 m, Sun 383 m; recorded under entry 187; number 204 unused). RETIRED a0652385
- new: 223 ayanamsa_ex and sidereal houses honour JPLHOR_APPROX nutation while plaus_iflag drops Horizons flags for sidereal places (1.72e-5 deg at 1950, cusps move, sidereal Mars not); 224 RESIDUAL Makransky 'i' cusp 9 speed 1.6e-7 deg/day off (Moshier 4.2e-7), entry 41 ARMC difference of rough cusps
- dispatched from 5646d28: aa98d289 swe-f223 (223 Horizons flags ayanamsa/houses vs sidereal places, 224 Makransky 'i' cusp 9 speed; G164 or G156). next bug 225, next gate G165
- running (10): f199, f200, f205, f206, f209, f211, f212, f218, f221, f223
- merged 200+202 (41c90a6 G155 framehold; owen_hold ctx fields), 206+208 (25f499c G158 spanedge). RETIRED acac80c6, a1fed21f
- new from 206 agent: 225 true node/osc apogee SPEED reads Moon at tjd+2^-6 -> at de200 end fall to Moshier 260 (node 2.2e-3, apogee 1.67e-2 deg off) where w/o speed JPL; strict refused; 226 nod_aps SPEED reads Earth at tjd-lt, silently Moshier near start (Pluto osc 1800-2400 files 2378496.625 speed 1.2e-5 off no note; strict refused); 227 Moshier-Moon mark not carried across calls (strict: fixstar after Moon+Sun answers 2; fallback Sun after Moon no note) -> SENT to f205 agent adfa7b19 as follow-up
- told f209 agent a4325ac0 to rebase onto 25f499c (200 changed sidt_pra)
- dispatched from 25f499c: aaf168e5 swe-f225 (225 true node/apogee SPEED at span edge, 226 nod_aps SPEED Earth read near start; extend G158). next bug 228, next gate G165
- running (9): f199 a35eea4d, f205 adfa7b19 (+227), f209 a4325ac0, f211 ab1583c9, f212 aa3ad5a3, f218 a932a2d6, f221 a7a57083, f223 aa98d289, f225 aaf168e5
- merged 201 (973fc07), 199 (82d54b1) G154 nodaposcrate. RETIRED a35eea4d. Cost: nod_aps osculating +7.5% instructions (two more epochs; not swe_calc path)
- new from 199 agent: 228 RESIDUAL 8.0e-5 deg/day nod_aps SEFLG_J2000 frame turning over +-1 day (nodaps_ecl_to_j2000) across edges; 229 RESIDUAL 1.0e-5 Moshier Moon osc points in nod_aps within 2^-7 day of edge (186 noticed); 231 BEHAVIOUR nod_aps sidereal speeds lack ayanamsa rate (3.82e-5 deg/day). Below floor: 230 Pluto geo osc aphelion aberr rate 7.7e-9 deg/day at 1812 (record only, number 230 = record)
- RENUMBERED: 228 J2000 turning, 229 Moshier Moon osc nod_aps, 230 nod_aps sidereal speeds (BEHAVIOUR); Pluto 7.7e-9 below floor, recorded in entry 199 only (no number)
- dispatched from 82d54b1: a151b6ba swe-f228 (228-230; extend G154 or G165). next bug 231, next gate G165
- running (9): f205 adfa7b19 (+227), f209 a4325ac0, f211 ab1583c9, f212 aa3ad5a3, f218 a932a2d6, f221 a7a57083, f223 aa98d289, f225 aaf168e5, f228 a151b6ba
- merged 218-220 (aba2e92 G162 nutseries; G1 cov:nutmodel[4] moves 18.697138162535065 -> 18.697138160241526 h), 209+214 (8215e17 G159 sidtrange; SIDT_PRA_TFAR 2000 cy linear beyond; SWI_ECL_NUT_JMAX in sweph.h), 221 (887873d G163 nodbias). 222 NOT A DEFECT (Moshier Mars series fit outside -1350..3000; Jupiter DE404 vs DE431). RETIRED a932a2d6, a4325ac0, a7a57083
- told f225, f228 to rebase onto 887873d; told f212 per-quantity bound (ayanamsa by precession model validity incl. Vondrak)
- new: 231 interpolate_nut nodes depend on earlier calls (6.7e-7 deg, both builds) -> fixed grid; 232 Woolard args plain double (2.1e-7 deg at 1e10-1e11); 233 IAU 2000B Delaunay args linear per SOFA (7.4e-9 deg at 2023); 234 IAU 2006 sidereal time leaves 0..24h from |J| 2e10 (86400*floor of polynomial seconds); 235 RESIDUAL sidt integral noise 5e-8 deg 1500-2000 cy Vondrak (rate rounding 5e-7"/cy)
- dispatched from 887873d: a4bed93f swe-f231 (231-233 nutation; G162/G122), a4d54ad3 swe-f234 (234 IAU2006 sidtime reduction, 235 sidt rate noise; G159/G39). next bug 236, next gate G165
- running (8): f205 adfa7b19 (+227), f211 ab1583c9, f212 aa3ad5a3, f223 aa98d289, f225 aaf168e5, f228 a151b6ba, f231 a4bed93f, f234 a4d54ad3
- merged 223+224 (0eccce8 G164 sidhorflag, G156 extended; 203's nut side test removed by 223's rule: Horizons flags reach no sidereal output). RETIRED aa98d289
- new: 236 swe_gauquelin_sector SIDEREAL puts sidereal place in tropical frame (2.62 sectors, Swiss Mars 1950 Lahiri). Below floor: MC/Asc VERY_SMALL snap 1e-10 deg
- dispatched from 0eccce8: a9868641 swe-f236 (236 gauquelin sidereal; extend G164). told f212 to rebase onto 0eccce8. next bug 237, next gate G165
- running (8): f205 adfa7b19 (+227), f211 ab1583c9, f212 aa3ad5a3, f225 aaf168e5, f228 a151b6ba, f231 a4bed93f, f234 a4d54ad3, f236 a9868641
- merged 210+211+216 (5b19650 G160 absdate; swi_date_invalid(); SWI_FILENAME_JMAX 7.8e11). RETIRED ab1583c9. FINAL PASS: reword its subject "fix: absurd dates are refused..." to house style
- new: 237 time_equ/lmt_to_lat/lat_to_lmt at absurd/NaN dates (ERR with NaN/inf outputs, not 0; reach sidtime); 238 swe_utc_to_jd NaN seconds OK with NaN; 239 outputs untouched on refusal: calc_pctr, get_orbital_elements, orbit_max_min_true_distance, sol_eclipse_where, lun_occult_where, helio_cross (finite out-of-range); 240 eclipse searches from +-6e10 refuse naming a date nobody gave; 241 heliacal_ut at +-1e200.. "no heliacal date found" not out of range; 242 sweph range msg for Mars/Moon at 8e6 calls it "asteroid eph. file (sepl_168.se1)"; 243 huge-date messages cut at 255 digits only (use %.10g past 1e15)
- dispatched from 5b19650: a194b778 swe-f237 (237, 238, 243; extend G160), a02267bb swe-f239 (239-242; G160 or G165). next bug 244, next gate G165
- running (9): f205 adfa7b19 (+227), f212 aa3ad5a3, f225 aaf168e5, f228 a151b6ba, f231 a4bed93f, f234 a4d54ad3, f236 a9868641, f237 a194b778, f239 a02267bb
- merged 205+227 (db233f4 G117 1916), 207 (b4f9be9 G157 horfall; G24 jplhora_bias_side/vel now de200_as_431), 228-230 (fd58190 G154 624). RETIRED adfa7b19, a151b6ba. Cost: nod_aps sidereal osc +6.5% (ayanamsa rate)
- 244 (JPL fallen to Swiss with file missing: empty serr, sweph() wipes "trying Swiss Eph") SENT to f239 agent a02267bb. FINAL PASS: plaus_iflag comment "SEFLG_JPLHOR only with JPL and Swiss Ephemeeris" wrong (code drops for Swiss)
- told f237 a194b778 to rebase onto b4f9be9
- new: 245 lunar_osc_elem Moshier hold (202) call-order dependence: swi_moshmoon under hold returns unheld Moon saved by prior swe_calc(SE_MOON) (true node speed 1.66e-3 deg/day off fresh); 246 RESIDUAL swe_calc apparent Sun/planets on Moshier just after "0,9,1" edges latitude speed 3.28e-5 deg/day (aberration rate)
- dispatched from fd58190: a8d88a58 swe-f245 (245 Moshier hold call order, 246 Moshier aberr rate at edges; G155/G139/G165). next bug 247, next gate G165
- running (8): f212 aa3ad5a3, f225 aaf168e5, f231 a4bed93f, f234 a4d54ad3, f236 a9868641, f237 a194b778, f239 a02267bb (+244), f245 a8d88a58
- 225/226 merged as 188d997 (hand-merged swecl.c holdi line: (kk >= 5 ? holdx : hold) && (!moshm || t != tjd_et)); gates G158 G155 G163 G154 G143 G47 G69 G146 G79 G92 G42 G38 G74 G46 G78 G133 G117 G24 G99 pass
- dispatched aaa326bd (swe-f249: 249 SPEED3 span edge, G165) and a016f88d (swe-f250: 250 file-join stencil + 251 mean perigee deflection rate, G166), both from 188d997. Next bug 252, next gate G167.
- 69c41a0: final-pass comment TODOs done (earth_at_lt_sun embofs refs, embofs line numbers, plaus_iflag JPLHOR comment). Remaining final-pass rewording: 5b19650 subject, 9cc76fa message, autosquash 9e9065f d05f2b8.
- merged 231 (a6957e4, G122 371) and 232 (36fea03, G162). 233 (b064fce, published 2000B linear args) LEFT OUT: hybrid is 2-3.7 mas of 2000A everywhere vs published 1.44" at -3000; recorded in PR "Not changed". Agent a4bed93f done. Final-pass baseline moves: G1 cov:interpolate_nut 3.8e-9, cov:interp_nut[on] 1.7e-8, [mid] 1.5e-8.
- merged 236 (1df5868) and 247 (432851c), G164. Agent a9868641 done.
- new: 252 Gauquelin m0/1 other flags + 253 swetest house pos with -sid etc -> ad3c23dc (swe-f252, G164 or G167); 254 sidereal W/N house_pos -> a9c5c94f (swe-f254, G168); 255 Moshier range message 0.3-day margin -> sent to a194b778 (237/238/243 agent). Next bug 256, next gate G169.
- merged 234/235 (e9f32ac; hand-merged swephlib.c: exact paths use sidt_deltat; G159 (e) expects houses refused at UT next inside +1e12 per 213; upstream G159 755), 245/246 (78984d3, kept the || mm Swiss-from-Moshier-Moon stretch), 237/238/243/255 (3e41806). Agents a4d54ad3, a8d88a58, a194b778 done.
- new: 256 long-term sidt past precession model range (212 rule) + 257 delta t rounding 5.8e-9 at 1e12 -> a7b9e24d (swe-f256, G159/G169); 258 Moshier Moon fallback light-time across window + 259 topo star site read across edge -> aac5309c (swe-f258, G139/G155/G170).
- HELD (load ~11): 260 ndat=iyear*10000 int overflow (swe_jdet_to_utc, swe_utc_to_jd, years>214748); 261 swe_utc_time_zone unchecked int cast, one-day-only date correction (12:00 at -100h -> hour 88); 262 UT functions' file refusal names TT not caller's UT; 263 fixed-decimal messages print a double next to range end as the end (strict JPL "not available for jd 2305424.5000" for 2305424.4999999995). Worktree swe-f260 ready (from 3e41806). Next bug 264, next gate G171.
- merged 249 (6355ebe, G165 speed3edge). Agent aaa326bd done.
- new: 264 JPL Moon in light time after de200 start = Sun's direction (flag 1, 256.12 deg) + helio/bary; 265 SEFLG_SPEED refuses where plain answers (bary Sun after de200/sepl_18 start; Moshier Sun at planets' start-0.3; Moshier tnode/oscapog in Moon's 0.2 margins) -> a6f43c88 (swe-f264, G158/G165/G171).
- HELD (load ~11, 7 agents): 260-263 (swedate UB/messages, worktree swe-f260 from 3e41806); 266 speed stencils divide by nominal step though tjd+-h rounded: SPEED3 calc_speed b/dt (1.7e-6 relative, Moon 2.15e-5 deg/day), survey every stencil with non-binary h or large |tjd| (sweph.c 7544, 8192, 8322, 11573; swecl.c 5997-6020, 7032, 1626, 2112; swehouse.c 383, 1423; swemmoon.c 900, 1831; swemplan.c 688), fix via exact spacing (calc_speed_side form) where above floor; 267 dropped reads' notes lost (225 epochs, 249 SPEED3: de406 fallback warning on first JPL open) - check with 244's fix (a02267bb).
- merged 254 (2ce2cd4, G168 sidhpos; documented limitation, comments only). Agent a9c5c94f done.
- dispatched a98fc017 (swe-f268: 268 eclt0/ssy ascmc[2] auxiliary ARMC; 269 eclt0/ssy W cusps not sidereal sign starts; 270 Sripati house_pos wrap hpos>12 -> 1; 271 'T' house_pos 1e-6 deg search; 272 Lahiri eclt0/ssy get_aya_correction planets vs houses 3.6e-5; G168/G172) and a53b30fb (swe-f260: 260-263; G160/G173). Still HELD: 266 (nominal step division survey), 267 (dropped reads' notes). Next bug 273, next gate G174.
- gate numbers: G165 249 (merged), G166 f250 joinspeed, G167 f252 hposflag, G168 254 (merged), G169-G173 promised to 256/258/264/268/260, G174 a02267bb refusal.c (239-248, was G165). Next gate G175.
- merged 252/253 (6e0c792, G167 hposflag; also fixed swetest diff-mode house positions). Agent ad3c23dc done. G1 baseline mismatch on tip is expected (final-pass re-baseline).
- dispatched ad4b3997 (swe-f273: 273 Gauquelin TOPOCTR uses set_topo not geopos; 274 SE_EARTH geocentric sector of zero vector; 275 swetest -fm/-fz/RA with frame flags, cusps' ZD with -sid; G167/G175). Next bug 276, next gate G176.
- merged 250 (bc1a592, G166 joinspeed; auto-merged sweph.c with 245/246's fixstar_obs_rate/lunar_osc_elem, reviewed). 251 NOT A DEFECT (reference's own stencil spanned the deflection's 0.0024-day variation; 2.6e-11 vs fine differences). Agent a016f88d done.
- dispatched a1773a39 (swe-f276: 276 Chebyshev segment-boundary stencils incl SPEED3 across joins, up to 9.2e-3 deg/day; 266 nominal-step division survey; G166/G176). Still HELD: 267 (dropped reads' notes; check vs 244). Next bug 277, next gate G177.
- merged 239-248 (140eec4, G174 refusal), 256/257 (752d14a, G159), 258/259 (cdf372b, G170 framefall). Agents a02267bb, a7b9e24d, aac5309c done. Below floor recorded: observer_accel crossing 4.2e-9 deg/day.
- dispatched a75a54d4 (swe-f277: 277 altitude checks rise_trans fast path/vis_limit_mag/topo_arcus_visionis; 278 orbit_max_min(SE_SUN) vs orbital elements; 279 heliacal_pheno_ut darr[28..30]; 281 backward eclipse searches near sepl_168 end; G174/G177), ab03fd3c (swe-f283: 283 formula sidtime eqeq with out-of-range obliquity; 284 nutation validity range per model (A'T, T^4 noise past 1e9); 285 site sidt model chosen at default-delta-t TT not the call's; G159/G161/G178).
- sent to a53b30fb: 280 heliacal_pheno_ut/vis_limit_mag name TT; tnode SPEED refusal names stencil date; heliacal_ut fallen JPL empty serr; 282 date_conversion sets *tjd on ERR. Sent to a6f43c88: 267 dropped reads' notes. Next bug 286, next gate G179.
- merged 268-272 (ede2cce, G172 sidhplane). Agent a98fc017 done. golden: cov:hsys_sid[eclt0]/[ssypl] -3.62e-5, cov:house_pos[T,*] 3.4e-9..2.9e-8 house (final-pass rebaseline).
- dispatched a984d273 (swe-f287: 287 ecl_t0_aux dvpxe sign by date; 288 ascmc[8] nonzero on planes; 289 house_pos 'H' reversed south; 290 house_pos toupper 'i' -> 'I'; G172/G179). 286 reserved for a53b30fb. Next bug 291, next gate G180.
- merged 273-275 (e8a24ab, G175 localsky). Accepted the agent's choice: swetest cusp RA/dec always on the true equator of date (with -nonut the Asc RA moves ~1e-3 deg) so they match the MD/ZD beside them. Agent ad4b3997 done.
- dispatched ae4892c5 (swe-f291: 291 rise_trans/eclipse_how/when_loc leave geopos as observer; 292 rise_trans SE_EARTH; 293 SE_ECL_NUT pla_diam[-1] OOB + non-body refusal; G175/G180) and a3ced28f (swe-f294: 294 swetest cusp az/alt at 0 m; 295 swetest cusp RA/dec speeds leftover; G175). Next bug 296, next gate G181.
- merged 287-289 (1fc11a3, G179 housecusp). 290's cause is the Makransky cusp formula (md>90 missing +90), not toupper. Agent a984d273 done. G1 cov:hsys_sid[eclt0] moves 5e-13.
- dispatched a6558530 (swe-f290: 290 Makransky cusps; 297 'H' MC beyond zenith; 298 Sunshine house_pos in polar circles; G179/G181) and a582bdbd (swe-f296: 296 ssypl_aux acos fold before 5650 BC; 299 Koch house_pos MC/IC failed; 300 'U' equator 3.6e-6 deg; G179/G172/G182). Next bug 301, next gate G183.
- merged 264/265 (0415017 -> picked) and 267 (9d96e4c), G171 ltstart. Agent a6f43c88 done. Below floor: Moshier Sun aberration rate near 625000.3 short/full lunar series ~1.6e-9 deg/day.
- dispatched a2d3ddc1 (swe-f301: 301 strict refuses Mars/Pluto at semo_18 start+0.001 w/o semo_12; 302 Sun SPEED refused/extra note there; 303 de406 warning lost in tnode SPEED3, asteroid SPEED3 "sun: sun:", swe_nod_aps; G171/G183). Next bug 304, next gate G184.
- merged 294/295 (96547f3; + localsky.c bounded memcpy, 275 note -fPaSS), 260-263/280/282/286 (e852290; hand-merged lunar_osc_elem: 263's kstart before 265's span loop; G171 uses swi_jd_str), 283-285 (d5143d5; nutation range -3100015.5..8000016.5 all models; G140 (c) adapted), 3a130bb G161/G29 header numbers. Agents a3ced28f, a53b30fb, ab03fd3c done. 282 decided: keep *tjd on ERR, documented.
- verified fixed by 264/265: asteroid JPL SPEED crash at de200 start; Moshier tnode SPEED at 2818000.69.
- dispatched a848883c (swe-f304: 304 swetest TT as UT for houses/azalt; 305 -fq on cusps; 306 -fW/-fw on cusps; G175), abe8f0b6 (swe-f308: 308 mean node/apogee SPEED refused 1/32 day inside ends; 310 search start test names wrong file at UT 7e11; 311 jdut1_to_utc far inversion; G160/G184), a5cd5198 (swe-f312: 312 NONUT star past Vondrak range; 313 observer_accel across step 1.5e-7; 314 Polaris topo Moshier Sun lt across step 3e-9; G178/G170/G185). Next bug 315, next gate G186.
- merged 276/266 (01d0170, G176 segspeed; auto-merged sweph.c/swecl.c, reviewed SPEED3 path). Agent a1773a39 done.
- recorded not fixed: SPEED3's own 1e-4-day step noise (Mercury one-sided 1.1e-8, Moon 2.5e-7/1.6e-6 deg/day): the method SEFLG_SPEED3 names; the manual says SEFLG_SPEED is the precise one. No double-sided-step case found for SPEED3/pctr/stars (nearest boundary to a join 0.088 day).
- dispatched a1f0c38e (swe-f315: 315 Moon apparent place steps 4.9e-5 deg for its light time after an Earth step; G176/G186), aca87fb6 (swe-f316: 316 nod_aps geocentric Mars deflection jitter 6.7e-5 deg at JD 7090348.28; G187). Next bug 317, next gate G188.
