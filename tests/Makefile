# Regression + thread-safety tests for libswe.
SWE      = ../sweconfig.c ../swedate.c ../swehouse.c ../swejpl.c ../swemmoon.c ../swemplan.c \
           ../sweph.c ../swephlib.c ../swecl.c ../swehel.c
CFLAGS  ?= -O0 -g -D_GNU_SOURCE -I..
# `make LTO=1` mirrors the root Makefile's opt-in; see the note there for
# the measured numbers and why it is not on by default.
ifeq ($(LTO),1)
  CFLAGS += -flto
endif
LIBS     = -lm -ldl -lpthread
EPHE    ?= ../ephe
REP     ?= 7

all: golden sunrace glprace threadshim cfgleak jplguard jplreal

golden: golden.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

sunrace: sunrace.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

sunrace_tsan: sunrace.c $(SWE)
	$(CC) $(CFLAGS) -fsanitize=thread $^ -o $@ $(LIBS)

glprace: glprace.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

glprace_tsan: glprace.c $(SWE)
	$(CC) $(CFLAGS) -fsanitize=thread $^ -o $@ $(LIBS)

# Sunshine-houses cross-thread contamination (swehouse.c saved_sundec)
check-sunrace: sunrace
	@SE_TEST_EPHE=$(EPHE) ./sunrace

check-sunrace-tsan: sunrace_tsan
	@SE_TEST_EPHE=$(EPHE) setarch -R env TSAN_OPTIONS=halt_on_error=0 ./sunrace_tsan

cfgleak: cfgleak.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# REVIEW.md J1: swejpl.c's header-validation guards, exercised against real
# malformed .eph fixtures. The ksize half is backed by a static_assert and so
# cannot regress silently; the ncf half is a runtime value read from a file
# and cannot be, which is why it needs this. Built with ASan so a removed
# guard shows up as a diagnosed overflow rather than silent corruption.
jplguard: jplguard.c $(SWE)
	$(CC) $(CFLAGS) -fsanitize=address $^ -o $@ $(LIBS)

# G15: the JPL path, against a genuine JPL binary rather than a fixture.
#
# Runs unconditionally: ephe/de200.eph is tracked beside the .se1 files, so
# there is nothing to configure and nothing to skip. jplguard.c and jplcalc.c
# only ever fed the reader fabricated headers; this opens a real one.
#
# SE_JPL_FILE and SE_TEST_EPHE override the defaults to point at a different
# ephemeris. ⚠️ SE_TEST_EPHE must name a directory holding BOTH the JPL file
# and the .se1 files, since the gate compares one against the other:
#
#   make check-jplreal SE_JPL_FILE=de441.eph SE_TEST_EPHE=$$HOME/se1
jplreal: jplreal.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

check-jplreal: jplreal
	@SE_TEST_EPHE="$${SE_TEST_EPHE:-$(EPHE)}" ./jplreal

check-jplguard: jplguard
	@./jplguard

# Phase 2 stragglers: internal setter calls must not publish, and
# swe_close() must clear the shared master.
check-cfgleak: cfgleak
	@SE_TEST_EPHE=$(EPHE) ./cfgleak

# swe_get_library_path(): buffer bounds + no shared Dl_info static
check-glprace: glprace
	@./glprace

# swethread.h primitives. No libswe dependency -- header-only.
threadshim: threadshim.c ../swethread.h
	$(CC) $(CFLAGS) threadshim.c -o $@ $(LIBS)

threadshim_tsan: threadshim.c ../swethread.h
	$(CC) $(CFLAGS) -fsanitize=thread threadshim.c -o $@ $(LIBS)

check-threadshim: threadshim
	@./threadshim

# Every backend swethread.h can select, plus the C89 dialect floor.
# Backend 2 (Windows SRWLOCK) is unreachable here and needs MSVC/mingw CI.
check-threadshim-all: threadshim.c ../swethread.h
	@set -e; for cfg in "-DSWE_NO_THREADS" "" "-std=gnu11 -DSWE_PREFER_C11_ATOMICS" \
	                    "-std=c99 -U__ATOMIC_SEQ_CST" "-std=c89"; do \
	  printf "  %-34s " "cc $$cfg"; \
	  $(CC) $$cfg -O2 -D_GNU_SOURCE -I.. threadshim.c -o .ts_cfg $(LIBS) 2>/dev/null \
	    && ./.ts_cfg | grep -E '^PASS|^FAIL' | tr -d '\n' && echo \
	    || { echo "BUILD FAIL"; exit 1; }; \
	done; rm -f .ts_cfg

check-threadshim-tsan: threadshim_tsan
	@setarch -R env TSAN_OPTIONS=halt_on_error=0 ./threadshim_tsan

golden_tsan: golden.c $(SWE)
	$(CC) $(CFLAGS) -fsanitize=thread $^ -o $@ $(LIBS)

# Regenerate the baseline. Only ever run this on a PRISTINE tree.
baseline: golden
	env -u SE_EPHE_PATH ./golden --ephe $(EPHE) > baseline.txt
	@echo "baseline.txt: $$(wc -l < baseline.txt) rows"

# G1: numerical no-op.
#
# Bit-exact, and therefore only meaningful at FIXED build flags. baseline.txt
# is pinned to the CFLAGS above (gcc -O0). Changing optimisation level or
# compiler legitimately perturbs the last bits -- speed components are
# finite-differenced, which amplifies rounding -- so use check-golden-O2 /
# cmpgolden.py for cross-configuration comparison instead of this target.
check-golden: golden baseline.txt
	@env -u SE_EPHE_PATH ./golden --ephe $(EPHE) > .out.txt
	@diff -q baseline.txt .out.txt && echo "G1 PASS: output bit-identical" \
	  || { echo "G1 FAIL: numerical regression"; diff baseline.txt .out.txt | head -20; exit 1; }

# G1b: the library must also behave at -O2, which is what a release build
# uses. Compared with tolerance, not diff: measured gcc -O0 vs -O2 divergence
# on this tree is 224 values / 65 rows, median 2.2e-15, max 7.6e-08 deg
# (0.00027 arcsec).
#
# TOL is 1e-5, and the number is measured rather than picked. It has to clear
# the INTRINSIC sensitivity of the values themselves: perturb the input tjd by
# just +2 ULP and the worst non-ill-conditioned row moves by
#
#   Moon,    Moshier, longitude speed, year +3000   8.7e-06 deg/day
#   Mercury, Moshier, longitude speed, year -3000   1.5e-06 deg/day
#
# i.e. a difference below ~1e-5 is indistinguishable from the input noise
# floor and says nothing about the code. Those are finite-differenced analytic
# series at the extreme ends of the date range, where they are weakest; the
# same bodies mid-range sit at 1.1e-09 and 7.3e-07.
#
# 1e-5 deg is 0.036 arcsec, still ~30x tighter than the library's own quoted
# positional accuracy. This gate is a cross-toolchain SANITY check; the thing
# that actually catches regressions is check-golden, which is bit-exact.
TOL ?= 1e-5

golden_O2: golden.c $(SWE)
	$(CC) -O2 -g -D_GNU_SOURCE -I.. $^ -o $@ $(LIBS)

check-golden-O2: golden_O2 baseline.txt
	@env -u SE_EPHE_PATH ./golden_O2 --ephe $(EPHE) > .out_O2.txt
	@./cmpgolden.py baseline.txt .out_O2.txt --abs=$(TOL) \
	  && echo "G1b PASS: -O2 within $(TOL)" || { echo "G1b FAIL"; exit 1; }

# G2: every thread reproduces the main-thread transcript with NO per-thread setup
check-threads: golden
	@env -u SE_EPHE_PATH ./golden --ephe $(EPHE) --threads 8 > /dev/null \
	  && echo "G2 PASS" || { echo "G2 FAIL"; exit 1; }

# Sanity: the same test WITH per-thread setup must pass today.
check-threads-workaround: golden
	@env -u SE_EPHE_PATH ./golden --ephe $(EPHE) --threads 8 --per-thread-setup > /dev/null \
	  && echo "workaround PASS" || { echo "workaround FAIL -- something is badly wrong"; exit 1; }

check: check-version check-threadshim check-jplreal check-golden check-golden-O2 check-sunrace check-glprace check-cfgleak check-jplguard check-threads-workaround check-threads

clean:
	rm -f jplcalc jplreal .build.log .jplcalc.txt .jplcalc.err bench ctxtest jplguard cfgleak threadshim threadshim_tsan .ts_cfg golden golden_O2 golden_tsan sunrace sunrace_tsan glprace glprace_tsan .out.txt .out_O2.txt .setest_now.txt

.PHONY: setest-ref check-setest check-threadshim check-threadshim-all \
        check-threadshim-tsan all baseline check check-version check-golden check-golden-O2 check-sunrace check-sunrace-tsan \
        check-glprace check-cfgleak check-jplguard check-threads check-threads-workaround \
        check-build check-ctx check-jplcalc check-threadtiers check-bridge \
        check-winmacros check-jplreal \
        ci-image check-ci clean

# Benchmark for notes/C17_PERFORMANCE.md.
#
# NOT a gate -- it is a measuring instrument, and wall-clock on a shared
# machine is not a pass/fail signal. Built at -O2 regardless of CFLAGS,
# since a claim about codegen measured at -O0 is meaningless.
BENCH_CFLAGS = -O2 -g -D_GNU_SOURCE -I..

bench: bench.c $(wildcard ../*.c)
	$(CC) $(BENCH_CFLAGS) bench.c $(SWE) -o $@ $(LIBS)

bench-run: bench
	@./bench --ephe $(EPHE) --rep $(REP)

# G13: identifiers must not collide with the Windows annotation macros.
#
# windows.h defines IN, OUT, OPTIONAL and friends as EMPTY macros -- they are
# SAL parameter annotations in winnt.h. A variable called OUT therefore
# vanishes on MSVC: `FILE *OUT;` becomes `FILE *;` and every `fprintf(OUT,
# ...)` becomes `fprintf(, ...)`.
#
# That is exactly what happened to tests/golden.c, and the resulting ~30
# cascading errors all pointed at fprintf call sites and looked like a
# broken thread-local. It cost four push-and-wait CI cycles to find, because
# nothing here could reproduce an MSVC-only failure.
#
# This does reproduce it, in about a second and with no Windows: define the
# macros the way winnt.h does and see whether the sources still parse.
# Verified against the broken revision -- 20 errors, at the same two lines
# MSVC reported.
WINMACROS = -DIN= -DOUT= -DOPTIONAL= -DFAR= -DNEAR=

# winstub/ is a handful of stand-in Windows headers -- just enough for a
# Linux compiler to PARSE the `#if defined(_WIN32)` branches. Nothing links
# against them. See tests/winstub/windows.h for the rationale.
WINPATH = -D_WIN32 -D__stdcall= '-D__declspec(x)=' -Iwinstub

check-winmacros:
	@echo "G13: Windows code paths, checked without Windows"
	@rc=0; \
	for f in $(wildcard *.c) $(SWE) ../swetest.c ../swevents.c ../swemini.c; do \
	  $(CC) -fsyntax-only -std=c17 -D_GNU_SOURCE $(WINMACROS) -I.. -I. $$f 2>/tmp/wm.$$$$ \
	    || { echo "  FAIL (macro collision): $$f"; head -4 /tmp/wm.$$$$ | sed 's/^/      /'; rc=1; }; \
	done; \
	echo "  annotation macros (IN/OUT/OPTIONAL/FAR/NEAR)  OK"; \
	for f in golden.c ctxtest.c threadshim.c; do \
	  $(CC) -fsyntax-only -std=c17 -D_GNU_SOURCE $(WINPATH) -I.. -I. $$f 2>/tmp/wm.$$$$ \
	    || { echo "  FAIL (_WIN32 path): $$f"; head -4 /tmp/wm.$$$$ | sed 's/^/      /'; rc=1; }; \
	done; \
	echo "  _WIN32 branches parse                         OK"; \
	rm -f /tmp/wm.$$$$; \
	[ $$rc -eq 0 ] && echo "PASS" || { echo "G13 FAIL"; exit 1; }

# G14: SE_VERSION in sweph.h is the only place the version is written.
#
# It was in four files. Bumping meant remembering all of them, and the one
# that mattered most was the least obvious: Android.mk's LOCAL_MODULE is
# the string a consumer passes to System.loadLibrary(), so a missed copy
# there ships a library nobody can load by the documented name.
#
# The check is a grep, deliberately: anything cleverer would itself need
# to know where versions are allowed to live.
check-version:
	@echo "G14: one source of truth for the version"
	@v=$$(sed -n 's/^\#define SE_VERSION[[:space:]]*"\([^"]*\)".*/\1/p' ../sweph.h); \
	[ -n "$$v" ] || { echo "  FAIL: cannot read SE_VERSION from sweph.h"; exit 1; }; \
	echo "  SE_VERSION = $$v"; \
	stray=$$(cd .. && git grep -l -E '[0-9]+\.[0-9]+\.[0-9]+-ts\.[0-9]+' -- . ':!ephe' ':!notes' \
	         | grep -v '^sweph\.h$$' || true); \
	if [ -n "$$stray" ]; then \
	  echo "  FAIL: version literal outside sweph.h:"; \
	  echo "$$stray" | sed 's/^/    /'; \
	  echo "  derive it instead -- see Android.mk, or 'make version' in the root"; \
	  exit 1; \
	fi; \
	echo "  no other tracked file carries a version literal   OK"; \
	m=$$(cd ../contrib/android/jni && sed -n 's/^\#define SE_VERSION[[:space:]]*"\([^"]*\)".*/\1/p' sweph.h); \
	[ "$$m" = "$$v" ] || { echo "  FAIL: Android.mk would derive $$m, not $$v"; exit 1; }; \
	echo "  Android.mk derives swe-$$v                        OK"; \
	echo "PASS"

# check-ci: run the gate set under the CI runner's exact toolchain.
#
# NOT one of the numbered gates -- it is the others, re-run somewhere that
# matches CI. This box is Ubuntu 22.04 with gcc 11.4 and no clang; the
# runner is ubuntu-24.04 with gcc 13.3.0-6ubuntu2~24.04.1 and clang 18.
# Two gcc releases apart, and several diagnostics the CI build treats as
# errors simply do not exist here.
#
# That cost real time: an uninitialised read in swevents.c passed all 14
# gates locally and failed CI, and a gcc-only __attribute__((aligned(64)))
# broke the MSVC job while gcc accepted it in silence. Both were a
# push/wait/read cycle to find.
#
# The clang half mirrors the CI clang job exactly -- same targets, same
# CC=clang, same flags (tests/CFLAGS, which carries no -Werror). An earlier
# version swept every source with -Wall -Wextra -Werror and failed on things
# CI never compiles or never treats as errors, which makes this gate a worse
# proxy, not a better one. Findings from that stricter sweep are recorded in
# notes/REVIEW.md rather than enforced here.
#
# Requires docker. Builds the image on first use (~30 s), reuses it after.
CI_IMAGE = swisseph-ci

ci-image:
	@docker build -q -t $(CI_IMAGE) ../tools/ci-image > /dev/null
	@echo "  $(CI_IMAGE): $$(docker run --rm $(CI_IMAGE) gcc --version | head -1)"

check-ci: ci-image
	@echo "=== gate set under the CI toolchain ==="
	@docker run --rm -v $(abspath ..):/src -w /src/tests $(CI_IMAGE) sh -c '\
	  set -e; \
	  make -s -C .. clean > /dev/null 2>&1 || true; \
	  make -s clean       > /dev/null 2>&1 || true; \
	  for t in check-golden check-golden-O2 check-threads check-cfgleak \
	           check-sunrace check-glprace check-jplguard check-jplcalc \
	           check-threadshim check-threadtiers check-ctx check-bridge \
	           check-build; do \
	    printf "  %-22s " $$t; \
	    if make -s $$t > /tmp/o 2>&1; then echo OK; \
	    else echo FAIL; tail -20 /tmp/o | sed "s/^/      /"; exit 1; fi; \
	  done; \
	  echo "  --- clang (as the CI clang job runs it) ---"; \
	  make -s clean > /dev/null 2>&1 || true; \
	  for t in check-threads check-threads-workaround check-sunrace \
	           check-glprace check-cfgleak check-threadshim; do \
	    printf "  %-22s " $$t; \
	    if make -s $$t CC=clang > /tmp/o 2>&1; then echo OK; \
	    else echo FAIL; tail -20 /tmp/o | sed "s/^/      /"; exit 1; fi; \
	  done'
	@# Clean both trees afterwards too: binaries built inside the container
	@# link against the image's libasan/libc and will not run on the host.
	@# Inheriting them in either direction is what made this gate fail the
	@# first time it ran.
	@$(MAKE) -s -C .. clean > /dev/null 2>&1 || true
	@$(MAKE) -s clean       > /dev/null 2>&1 || true
	@echo "PASS (CI toolchain)"

# G12: no library-internal code may call a legacy (shim) entry point.
#
# A bare swe_X() from inside the library computes against the DEFAULT
# context regardless of the caller's -- so an explicit swe_ctx quietly gives
# the wrong answer, with no error and no race for a sanitizer to catch.
#
# A gate rather than a review item because the hand audit under-reported:
# the first pass said "0 violations" using a name list that was stale, and
# the second found 12 real ones.
check-bridge:
	@python3 ../tools/bridgecheck.py

# G11: every swethread.h backend must build, link and compute.
#
# gcc and clang select tier 3 (__atomic builtins) at EVERY -std=, c89
# included, so tiers 4 and 5 were never built by anything. Tier 5 had
# rotted: swethread.h declared swi_gen_fallback_mutex extern and called it,
# and no translation unit in the library defined it, so the library could
# not link at all on the exotic toolchains that fallback exists to serve.
# tests/threadshim.c had a hand-copied selector that had drifted too.
#
# Nothing detects that except building the tiers, which is what this does.
check-threadtiers: baseline.txt
	@echo "G11: swethread.h backends"
	@rc=0; \
	for spec in "c17:" "c17:-DSWE_PREFER_C11_ATOMICS" \
	            "c99:-DSWE_PREFER_C11_ATOMICS" "c17:-DSWE_NO_THREADS"; do \
	  std=$${spec%%:*}; extra=$${spec##*:}; \
	  if $(CC) -std=$$std $$extra -O0 -g -D_GNU_SOURCE -I.. golden.c $(SWE) \
	       -o .tiergolden $(LIBS) 2>.tier.err; then \
	    ./.tiergolden --ephe $(EPHE) > .tier.txt 2>/dev/null; \
	    if diff -q baseline.txt .tier.txt >/dev/null; then \
	      printf "  %-34s OK\n" "-std=$$std $$extra"; \
	    else printf "  %-34s OUTPUT DIFFERS\n" "-std=$$std $$extra"; rc=1; fi; \
	  else printf "  %-34s BUILD FAIL\n" "-std=$$std $$extra"; \
	       head -3 .tier.err; rc=1; fi; \
	done; \
	rm -f .tiergolden .tier.txt .tier.err; \
	[ $$rc -eq 0 ] && echo "PASS" || { echo "G11 FAIL"; exit 1; }

# G10: numerical regression coverage for the JPL reader.
#
# swejpl.c's state() and interp() are executed ZERO times by every other
# gate -- they need a .eph file, and this repo ships only .se1. Measured
# with an fprintf: 0 hits across the whole golden suite. jplcalc generates
# a synthetic .eph and drives swi_pleph() through it.
jplcalc: jplcalc.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

check-jplcalc: jplcalc
	@./jplcalc --dir . > .jplcalc.txt 2>.jplcalc.err || \
	  { echo "G10 FAIL: jplcalc errored"; cat .jplcalc.err; exit 1; }
	@if [ ! -f jplcalc.baseline ]; then \
	   echo "G10: no baseline, recording one"; cp .jplcalc.txt jplcalc.baseline; fi
	@if diff -q jplcalc.baseline .jplcalc.txt >/dev/null; then \
	   echo "G10 PASS: JPL reader output bit-identical ($$(wc -l < .jplcalc.txt) rows)"; \
	 else echo "G10 FAIL: JPL reader output changed"; \
	   diff jplcalc.baseline .jplcalc.txt | head -20; exit 1; fi

# G4: two swe_ctx handles are genuinely independent.
#
# The gate every other one is blind to. G1 is bit-exactness of the legacy
# path and G2 is agreement between threads sharing the default context --
# a swe_X_r() that ignored its context argument entirely would pass both.
ctxtest: ctxtest.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

check-ctx: ctxtest
	@./ctxtest --ephe $(EPHE)

# G9: the shipped sample programs and the TRACE build must still compile.
#
# Not a behaviour gate -- a compile gate. swetest.c and swevents.c include
# swephlib.h BEFORE sweph.h, so every swe_ctx parameter added during Phase 3
# broke `make all` while every behaviour gate stayed green: nothing in
# tests/ compiles them. It went unnoticed across three phases. swetest is
# the library's shipped CLI, so that is not an acceptable blind spot.
#
# TRACE is likewise compiled by no other target. It is off by default and
# has no runtime gate, but it must at least not bit-rot.
check-build:
	@echo "G9: sample programs + TRACE build"
	@# -B, not plain `all`: the Makefile carries no header dependencies, so a
	@# stale swetest.o hides exactly the header-ordering break this gate exists
	@# to catch. Verified by removing the forward decl and watching `make all`
	@# pass anyway.
	@# Errors are SHOWN, not swallowed. The first version sent both stdout
	@# and stderr to /dev/null, so when this gate failed in CI it reported
	@# only "FAIL: make all" -- the diagnosis had to be reconstructed from
	@# scratch. A gate that hides why it failed is barely a gate.
	@$(MAKE) -B -C .. all > .build.log 2>&1 || { \
	  echo "  FAIL: make all"; tail -25 .build.log; rm -f .build.log; exit 1; }
	@rm -f .build.log
	@echo "  make all              OK  (swetest swevents swemini)"
	@# TRACE=0 means "no -DTRACE at all" -- the default build, checked here
	@# because it is the one the sample programs actually ship as.
	@for t in 0 1 2; do \
	  [ $$t = 0 ] && d="" || d="-DTRACE=$$t"; \
	  rc=0; \
	  for f in $(wildcard ../*.c); do \
	    $(CC) -fsyntax-only -D_GNU_SOURCE $$d -I.. $$f 2>&1 | grep -q "error:" && rc=1; \
	  done; \
	  [ $$rc -eq 0 ] && echo "  $${d:-(no TRACE)}         OK" \
	                 || { echo "  FAIL: $${d:-default build} does not compile"; exit 1; }; \
	done
	@echo "PASS"

# G8: upstream's own suite (setest/, 13 MB of expectations).
#
# It CANNOT be used as pass/fail here: t.exp was generated against the JPL
# ephemeris (de431.eph), which this checkout does not ship, so ~2730
# assertions fail on pristine upstream too -- retflag comes back SWIEPH(2)
# where the fixture expects JPLEPH(1), with ~1e-8 positional differences.
# It also aborts with "stack smashing detected" in suite_08_eclipses on
# pristine 3fd0f95.
#
# It IS a strong DIFFERENTIAL gate: whatever it prints must be byte-identical
# before and after our changes. Usage:
#     make setest-ref REF=origin/legacy-master   # capture reference once
#     make check-setest               # compare current tree against it
SETEST_REF ?= /tmp/setest_ref.txt
REF        ?= origin/legacy-master

# EPHE is relative to tests/ by default. Both setest runs below cd elsewhere
# -- one into a throwaway worktree under /tmp -- so a relative EPHE would
# resolve to two DIFFERENT ephemeris directories and the "differential" gate
# would be comparing runs over different data. Resolve it once, absolutely.
EPHE_ABS := $(abspath $(EPHE))

# setest resolves t.exp relative to its CWD, so it must be run from inside
# setest/. It also aborts partway through, so `|| true` is expected -- but the
# output must still be substantial; a short file means the run never started.
SETEST_MINLINES = 1000

# See scrub-ephe.sed: normalises the ephemeris path out of setest's output so
# the gate compares the library, not how make happened to be invoked.
SCRUB = sed -f $(CURDIR)/scrub-ephe.sed

# Both sides of the differential MUST be built with the same flags, and they
# cannot be left to the root Makefile: the reference is built from an old
# commit's checkout, which carries that commit's CFLAGS. When the C17
# migration flipped the root CFLAGS to "-std=c17 -Wall -Wextra -O2 -g -fPIC",
# this gate started comparing an -O2 build of HEAD against an -O0 build of
# 3fd0f95 and reported a "change" that was pure optimisation noise
# (0.00001923540096290257 vs 0.00001923540095976480).
#
# Pinned to -O0 for the same reason tests/baseline.txt is: a differential is
# only meaningful when the only variable is the code.
SETEST_CFLAGS = -O0 -g -fPIC -D_GNU_SOURCE

setest-ref:
	@rm -rf /tmp/se_ref_wt
	@git -C .. worktree prune          # a killed run leaves a stale registration
	@git -C .. worktree add -q /tmp/se_ref_wt $(REF)
	@cp -n ../setest/t.exp ../setest/t.fix /tmp/se_ref_wt/setest/ 2>/dev/null || true
	@$(MAKE) -B -C /tmp/se_ref_wt libswe.a CFLAGS="$(SETEST_CFLAGS)" >/dev/null 2>&1
	@$(MAKE) -B -C /tmp/se_ref_wt/setest >/dev/null 2>&1
	@cd /tmp/se_ref_wt/setest && SE_EPHE_PATH=$(EPHE_ABS) ./setest t 2>&1 | $(SCRUB) > $(SETEST_REF) || true
	@git -C .. worktree remove --force /tmp/se_ref_wt 2>/dev/null || true
	@git -C .. worktree prune
	@lines=$$(wc -l < $(SETEST_REF)); \
	 if [ $$lines -lt $(SETEST_MINLINES) ]; then \
	   echo "setest-ref FAILED: only $$lines lines (expected >= $(SETEST_MINLINES))"; \
	   head -3 $(SETEST_REF); exit 1; fi; \
	 echo "setest reference captured from $(REF): $$lines lines"

check-setest: $(SETEST_REF)
	# -B: the tree may already hold .o files built with the root CFLAGS
	# (the C17 migration sets -O2 there); without a forced rebuild make
	# would consider them up to date and the pin above would do nothing.
	@$(MAKE) -B -C .. libswe.a CFLAGS="$(SETEST_CFLAGS)" >/dev/null 2>&1
	@$(MAKE) -B -C ../setest >/dev/null 2>&1
	# ⚠️ SE_EPHE_FALLBACK=1: this gate asks whether our changes altered
	# UPSTREAM's numerical behaviour, so it has to run upstream's suite under
	# upstream's rules. setest requests ephemerides the machine may not have
	# and expects numbers back; under this fork's strict default it reads
	# zeros and every comparison fails -- which says nothing about the
	# arithmetic this gate exists to protect. The strict default is tested by
	# check-golden and G15 instead.
	@cd ../setest && SE_EPHE_PATH=$(EPHE_ABS) SE_EPHE_FALLBACK=1 ./setest t 2>&1 | $(SCRUB) > ../tests/.setest_now.txt || true
	@lines=$$(wc -l < .setest_now.txt); \
	 if [ $$lines -lt $(SETEST_MINLINES) ]; then \
	   echo "G8 FAIL: setest produced only $$lines lines -- it did not run"; \
	   head -3 .setest_now.txt; exit 1; fi
	@diff -q $(SETEST_REF) .setest_now.txt >/dev/null \
	  && echo "G8 PASS: setest output identical to $(REF) ($$(wc -l < .setest_now.txt) lines)" \
	  || { echo "G8 FAIL: setest output changed"; diff $(SETEST_REF) .setest_now.txt | head -20; exit 1; }
