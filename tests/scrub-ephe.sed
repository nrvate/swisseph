# Replace any ephemeris path with a placeholder.
#
# setest embeds the ephemeris path in its "file not found" messages, so its
# output otherwise depends on HOW make was invoked (EPHE=../ephe vs an
# absolute path) rather than on the library under test. That made the
# differential gate fail for a reason having nothing to do with any code
# change. tests/golden.c's sanitize() does the same job for the golden
# transcript; this is the equivalent for setest.
s@'[^']*ephe/*'@'$EPHE/'@g
s@PATH '[^']*'@PATH '$EPHE/'@g
