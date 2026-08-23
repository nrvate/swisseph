# Regression + thread-safety tests for libswe.
SWE      = ../swedate.c ../swehouse.c ../swejpl.c ../swemmoon.c ../swemplan.c \
           ../sweph.c ../swephlib.c ../swecl.c ../swehel.c
CFLAGS  ?= -O0 -g -D_GNU_SOURCE -I..
LIBS     = -lm -ldl -lpthread
EPHE    ?= ../ephe

all: golden sunrace

golden: golden.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

sunrace: sunrace.c $(SWE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

sunrace_tsan: sunrace.c $(SWE)
	$(CC) $(CFLAGS) -fsanitize=thread $^ -o $@ $(LIBS)

# Sunshine-houses cross-thread contamination (swehouse.c saved_sundec)
check-sunrace: sunrace
	@SE_TEST_EPHE=$(EPHE) ./sunrace

check-sunrace-tsan: sunrace_tsan
	@SE_TEST_EPHE=$(EPHE) setarch -R env TSAN_OPTIONS=halt_on_error=0 ./sunrace_tsan

golden_tsan: golden.c $(SWE)
	$(CC) $(CFLAGS) -fsanitize=thread $^ -o $@ $(LIBS)

# Regenerate the baseline. Only ever run this on a PRISTINE tree.
baseline: golden
	env -u SE_EPHE_PATH ./golden --ephe $(EPHE) > baseline.txt
	@echo "baseline.txt: $$(wc -l < baseline.txt) rows"

# G1: numerical no-op
check-golden: golden baseline.txt
	@env -u SE_EPHE_PATH ./golden --ephe $(EPHE) > .out.txt
	@diff -q baseline.txt .out.txt && echo "G1 PASS: output bit-identical" \
	  || { echo "G1 FAIL: numerical regression"; diff baseline.txt .out.txt | head -20; exit 1; }

# G2: every thread reproduces the main-thread transcript with NO per-thread setup
check-threads: golden
	@env -u SE_EPHE_PATH ./golden --ephe $(EPHE) --threads 8 > /dev/null \
	  && echo "G2 PASS" || { echo "G2 FAIL (expected until Phase 2)"; exit 1; }

# Sanity: the same test WITH per-thread setup must pass today.
check-threads-workaround: golden
	@env -u SE_EPHE_PATH ./golden --ephe $(EPHE) --threads 8 --per-thread-setup > /dev/null \
	  && echo "workaround PASS" || { echo "workaround FAIL -- something is badly wrong"; exit 1; }

check: check-golden check-sunrace check-threads-workaround check-threads

clean:
	rm -f golden golden_tsan sunrace sunrace_tsan .out.txt

.PHONY: all baseline check check-golden check-sunrace check-sunrace-tsan \
        check-threads check-threads-workaround clean
