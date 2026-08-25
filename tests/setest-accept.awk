# Drops the blocks of setest output belonging to testcases this fork has
# deliberately changed, so G8 can stay a strict differential everywhere else.
# Applied to BOTH sides of the comparison -- see check-setest in the Makefile
# for what is accepted and why.
#
# A block runs from a "# setest -s S.C.I t / ..." header to the next header;
# the trailing "Testmode ..." summary belongs to no block and is always kept,
# or a skipped final block would swallow it.
#
# ACCEPT is a regex anchored at the start of the selector, passed with -v.
/^# setest -s / { skip = ($4 ~ "^" ACCEPT) }
/^Testmode/     { skip = 0 }
!skip
