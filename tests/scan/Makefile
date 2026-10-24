# sescan: the by-hand numerics audit (see the header of sescan.c). Not part
# of `make check`: it is a survey that ranks findings, not a gate.
#   make            build ./sescan from the library sources
#   make run        run the full matrix (a few minutes), report to sescan.txt
#   make quick      three epochs only
SWE    = $(addprefix ../../,sweconfig.c swedate.c swehouse.c swejpl.c swemmoon.c \
         swemplan.c sweph.c swephlib.c swecl.c swehel.c)
CC     ?= cc
CFLAGS ?= -std=c17 -O2 -g -ffp-contract=off -D_GNU_SOURCE -Wall -Wextra -Werror
LIBS   = -lm -ldl -lpthread
EPHE  ?= ../../ephe

sescan: sescan.c $(SWE) $(wildcard ../../*.h)
	$(CC) $(CFLAGS) -I../.. sescan.c $(SWE) $(LIBS) -o $@

run: sescan
	./sescan $(EPHE) > sescan.txt
	@tail -n 20 sescan.txt

quick: sescan
	./sescan $(EPHE) --quick

clean:
	rm -f sescan sescan.txt

.PHONY: run quick clean
