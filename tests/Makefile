# Regression + thread-safety tests for libswe.
SWE      = ../swedate.c ../swehouse.c ../swejpl.c ../swemmoon.c ../swemplan.c \
           ../sweph.c ../swephlib.c ../swecl.c ../swehel.c
CFLAGS  ?= -O0 -g -D_GNU_SOURCE -I..
LIBS     = -lm -ldl -lpthread
EPHE    ?= ../ephe

all: golden sunrace glprace

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

# swe_get_library_path(): buffer bounds + no shared Dl_info static
check-glprace: glprace
	@./glprace

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
# would use (the shipped root Makefile has no -O flag at all). Compared with
# tolerance, not diff: measured gcc -O0 vs -O2 divergence on this tree is
# 224 values / 65 rows, median 2.2e-15, max 7.6e-08 deg = 0.00027 arcsec.
TOL ?= 1e-6

golden_O2: golden.c $(SWE)
	$(CC) -O2 -g -D_GNU_SOURCE -I.. $^ -o $@ $(LIBS)

check-golden-O2: golden_O2 baseline.txt
	@env -u SE_EPHE_PATH ./golden_O2 --ephe $(EPHE) > .out_O2.txt
	@./cmpgolden.py baseline.txt .out_O2.txt --abs=$(TOL) \
	  && echo "G1b PASS: -O2 within $(TOL)" || { echo "G1b FAIL"; exit 1; }

# G2: every thread reproduces the main-thread transcript with NO per-thread setup
check-threads: golden
	@env -u SE_EPHE_PATH ./golden --ephe $(EPHE) --threads 8 > /dev/null \
	  && echo "G2 PASS" || { echo "G2 FAIL (expected until Phase 2)"; exit 1; }

# Sanity: the same test WITH per-thread setup must pass today.
check-threads-workaround: golden
	@env -u SE_EPHE_PATH ./golden --ephe $(EPHE) --threads 8 --per-thread-setup > /dev/null \
	  && echo "workaround PASS" || { echo "workaround FAIL -- something is badly wrong"; exit 1; }

check: check-golden check-golden-O2 check-sunrace check-glprace check-threads-workaround check-threads

clean:
	rm -f golden golden_O2 golden_tsan sunrace sunrace_tsan glprace glprace_tsan .out.txt .out_O2.txt .setest_now.txt

.PHONY: setest-ref check-setest all baseline check check-golden check-golden-O2 check-sunrace check-sunrace-tsan \
        check-glprace check-threads check-threads-workaround clean

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
#     make setest-ref REF=3fd0f95     # capture reference output once
#     make check-setest               # compare current tree against it
SETEST_REF ?= /tmp/setest_ref.txt
REF        ?= 3fd0f95

# setest resolves t.exp relative to its CWD, so it must be run from inside
# setest/. It also aborts partway through, so `|| true` is expected -- but the
# output must still be substantial; a short file means the run never started.
SETEST_MINLINES = 1000

setest-ref:
	@rm -rf /tmp/se_ref_wt && git -C .. worktree add -q /tmp/se_ref_wt $(REF)
	@cp -n ../setest/t.exp ../setest/t.fix /tmp/se_ref_wt/setest/ 2>/dev/null || true
	@$(MAKE) -C /tmp/se_ref_wt libswe.a >/dev/null 2>&1
	@$(MAKE) -C /tmp/se_ref_wt/setest >/dev/null 2>&1
	@cd /tmp/se_ref_wt/setest && SE_EPHE_PATH=$(EPHE) ./setest t > $(SETEST_REF) 2>&1 || true
	@git -C .. worktree remove --force /tmp/se_ref_wt
	@lines=$$(wc -l < $(SETEST_REF)); \
	 if [ $$lines -lt $(SETEST_MINLINES) ]; then \
	   echo "setest-ref FAILED: only $$lines lines (expected >= $(SETEST_MINLINES))"; \
	   head -3 $(SETEST_REF); exit 1; fi; \
	 echo "setest reference captured from $(REF): $$lines lines"

check-setest: $(SETEST_REF)
	@$(MAKE) -C .. libswe.a >/dev/null 2>&1
	@$(MAKE) -C ../setest >/dev/null 2>&1
	@cd ../setest && SE_EPHE_PATH=$(EPHE) ./setest t > ../tests/.setest_now.txt 2>&1 || true
	@lines=$$(wc -l < .setest_now.txt); \
	 if [ $$lines -lt $(SETEST_MINLINES) ]; then \
	   echo "G8 FAIL: setest produced only $$lines lines -- it did not run"; \
	   head -3 .setest_now.txt; exit 1; fi
	@diff -q $(SETEST_REF) .setest_now.txt >/dev/null \
	  && echo "G8 PASS: setest output identical to $(REF) ($$(wc -l < .setest_now.txt) lines)" \
	  || { echo "G8 FAIL: setest output changed"; diff $(SETEST_REF) .setest_now.txt | head -20; exit 1; }
