################################################################################
# Swiss Ephemeris Multi-platform Build Makefile
#
# This Makefile builds the Swiss Ephemeris project on both Linux and macOS.
#
# Features:
#  - Automatically detects the operating system using `uname`
#  - Sets appropriate compiler flags, library linking options, and shared
#    library creation flags for Linux and macOS.
#  - Builds dynamically linked executables (swetest, swevents, swemini) on both
#    platforms.
#  - Builds a fully statically linked executable (swetests) on Linux only.
#  - Creates both static (libswe.a) and shared libraries (libswe.so on Linux,
#    libswe.dylib on macOS) from the source object files.
#
# Targets:
#    all         - Build all executables (swetest, swevents, swemini, and swetests on Linux)
#    swetest     - Build the swetest executable using libswe.a (dynamic linking)
#    swetests    - Build a fully statically linked swetest (Linux only)
#    swevents    - Build the swevents executable
#    swemini     - Build the swemini executable using libswe.a (static linking)
#    libswe.a    - Create the static library archive from object files
#    libswe.$(DYLIB_EXT)
#                - Create the shared library (extension depends on OS)
#    test        - Run tests from the setest directory (requires a Makefile in setest)
#    clean       - Remove all generated files and clean the setest directory
#
# To customize, modify the CFLAGS, LIBS, or any other variables as needed.
################################################################################

# Detect OS type via uname
OS := $(shell uname)

ifeq ($(OS), Darwin)
  # macOS settings
  CC               = cc
  CFLAGS           = -std=c17 -Wall -Wextra -Werror -O2 -g -fPIC -D_GNU_SOURCE
  LIBS             = -lm
  DYLIB_FLAG       = -dynamiclib
  DYLIB_EXT        = dylib
  STATIC_SUPPORTED = false
else
  # Assume Linux settings
  CC               = cc
  CFLAGS           = -std=c17 -Wall -Wextra -Werror -O2 -g -fPIC -D_GNU_SOURCE
  LIBS             = -lm -ldl
  DYLIB_FLAG       = -shared
  DYLIB_EXT        = so
  STATIC_SUPPORTED = true
  STATIC_LINK_FLAGS= -Wl,-Bstatic
  DYNAMIC_LINK_FLAGS= -Wl,-Bdynamic
endif

# Link-time optimisation. ON by default; `make LTO=0` turns it off.
#
# Measured on gcc 11.4 -O2 (tests/bench, 3 interleaved runs, median of 5):
#
#   moon         -5.0%     swemmoon.c calling swephlib.c helpers across TUs
#   calc-moseph  -2.6%
#   jpl-interp   -1.3%     within the benchmark's +/-2-3% noise floor
#   calc-swieph  -0.8%     "
#   houses       -1.3%     "
#
# Only the Moshier Moon path clears the noise floor convincingly, which is
# what notes/C17_PERFORMANCE.md section 4.4 predicted: this is a 10-object
# library and cross-TU inlining is the only way -O2 can reach helpers like
# swi_coortrf2 (64 cross-file call sites).
#
# Output is BIT-IDENTICAL to plain -O2 across all 5137 golden rows on gcc
# 13 -- verified, not assumed.
#
# On by default since the CI lto job runs the golden transcript under
# gcc, clang and Apple clang with -flto, all within the 1e-5 cross-toolchain
# tolerance. MSVC builds through the .vcxproj files and is not affected.
# tests/Makefile keeps its own LTO opt-in, so the bit-exact G1 reference
# build stays plain -O0.
#
# A libswe.a of LTO objects needs a plugin-aware ar (GNU binutils and Apple
# ar both are). If yours is not: make LTO=0.
LTO ?= 1
ifeq ($(LTO),1)
  CFLAGS += -flto
  LIBS   += -flto
endif

# Appended to every compile and link. Lets a caller add flags without
# restating the whole list -- CFLAGS is assigned, not ?=, so overriding it
# from the command line drops -std=c17 -Werror and the rest along with it.
#
# The release packaging uses it for macOS universal binaries:
#   make EXTRA_CFLAGS="-arch arm64 -arch x86_64"
EXTRA_CFLAGS ?=
CFLAGS += $(EXTRA_CFLAGS)

# Object files for the Swiss Ephemeris library
SWEOBJ = swedate.o swehouse.o swejpl.o swemmoon.o swemplan.o sweph.o sweconfig.o \
         swephlib.o swecl.o swehel.o

# Define overall targets. On Linux, include the static swetests target.
ifeq ($(STATIC_SUPPORTED),true)
ALL_TARGETS = swetest swetests swevents swemini obama
else
ALL_TARGETS = swetest swevents swemini
endif

all: $(ALL_TARGETS)

# Compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build swetest: link swetest.o with the static library libswe.a
swetest: swetest.o libswe.a
	$(CC) $(CFLAGS) -o swetest swetest.o -L. -lswe $(LIBS)

# Build swetests: fully statically linked version (Linux only)
ifeq ($(STATIC_SUPPORTED),true)
swetests: swetest.o $(SWEOBJ)
	$(CC) $(CFLAGS) $(STATIC_LINK_FLAGS) -o swetests swetest.o $(SWEOBJ) $(DYNAMIC_LINK_FLAGS) $(LIBS)
	@# mkdir -p: bin/ is no longer in the repository at all.
	@#
	@# bin/swetest and bin/swevents used to be tracked -- normal practice
	@# when this codebase was written -- but `make all` REWRITES them, so
	@# every build dirtied the working tree and invited a rebuilt binary
	@# into a commit. They are build output of this Makefile and are now
	@# ignored. The prebuilt Windows and Android binaries are NOT: nothing
	@# here regenerates those, so they stay tracked.
	@#
	@# This also fixed a real CI failure -- the runner's sparse checkout had
	@# no bin/, so the bare cp took `make all` down with it.
	mkdir -p bin
	cp swetests bin/swetest
endif

# Build swevents
swevents: swevents.o $(SWEOBJ)
	$(CC) $(CFLAGS) -o swevents swevents.o $(SWEOBJ) $(LIBS)

# Build sweventss, statically compiled
sweventss: swevents.o $(SWEOBJ)
	$(CC) $(CFLAGS) $(STATIC_LINK_FLAGS) -o sweventss swevents.o $(SWEOBJ) $(DYNAMIC_LINK_FLAGS) $(LIBS)
	mkdir -p bin
	cp sweventss  bin/swevents

# Build swemini
swemini: swemini.o libswe.a
	$(CC) $(CFLAGS) -o swemini swemini.o -L. -lswe $(LIBS)

# Build obama
obama: obama.o libswe.a
	$(CC) $(CFLAGS) -o obama obama.o -L. -lswe $(LIBS)

# Create a static library from the object files
libswe.a: $(SWEOBJ)
	ar r libswe.a $(SWEOBJ)

# Create a shared library
# CFLAGS and LIBS belong on this line as much as on any other. Without
# them the shared library is the one artifact built with different flags
# from everything around it: -flto never reached it under LTO=1, macOS
# universal builds silently produced a host-only dylib from fat objects,
# and it recorded no dependency on libm or libdl.
libswe.$(DYLIB_EXT): $(SWEOBJ)
	$(CC) $(CFLAGS) $(DYLIB_FLAG) -o libswe.$(DYLIB_EXT) $(SWEOBJ) $(LIBS)

# ---------------------------------------------------------------------
# Version. SE_VERSION in sweph.h is the ONLY place the version is written
# down; everything else derives from it (contrib/android/jni/Android.mk
# reads it with the same sed, the release workflow refuses to publish a
# tag that disagrees with it, and tests/Makefile's check-version fails the
# build if any other file grows a copy).
#
#   make version                       -> prints the current version
#   make bump VERSION=X.Y.Z-ts.N
# ---------------------------------------------------------------------
VERSION_SED = s/^\#define SE_VERSION[[:space:]]*"\([^"]*\)".*/\1/p

.PHONY: version bump
version:
	@sed -n '$(VERSION_SED)' sweph.h

bump:
	@test -n "$(VERSION)" || { \
	  echo "usage: make bump VERSION=X.Y.Z-ts.N   (current: $$(sed -n '$(VERSION_SED)' sweph.h))"; \
	  exit 1; }
	@old=$$(sed -n '$(VERSION_SED)' sweph.h); \
	if [ "$$old" = "$(VERSION)" ]; then echo "already $(VERSION)"; exit 0; fi; \
	sed -i 's/^\(#define SE_VERSION[[:space:]]*\)"[^"]*"/\1"$(VERSION)"/' sweph.h; \
	new=$$(sed -n '$(VERSION_SED)' sweph.h); \
	[ "$$new" = "$(VERSION)" ] || { echo "FAIL: sweph.h still reads $$new"; exit 1; }; \
	echo "SE_VERSION $$old -> $$new"; \
	echo "  tag with: git tag v$$new && git push origin v$$new"

# ---------------------------------------------------------------------
# Installation, and the pkg-config file that makes this library findable.
#
#   make install                     -> /usr/local
#   make install PREFIX=$HOME/.local -> anywhere, no root needed
#
# Consumers then build against it with the usual incantation:
#
#   pkg-config --cflags --libs swe
#
# This exists because a consumer that statically compiles our .c files ends
# up with its OWN copy of libswe -- its own `swed`, its own caches, its own
# open file handles. Two such consumers in one process (a Python binding
# and a native extension, say) cannot see each other's configuration at
# all, and no amount of thread-safety work in here fixes that. Linking one
# shared library is what makes them one library.
# ---------------------------------------------------------------------
PREFIX       ?= /usr/local
LIBDIR       ?= $(PREFIX)/lib
INCLUDEDIR   ?= $(PREFIX)/include
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig
INSTALL      ?= install

# PUBLIC headers only, the same set the release packages ship. swephlib.h
# and sweph.h are internal; swephexp.h does not include them and consumers
# do not need them -- verified by compiling against this set alone.
PUBLIC_HEADERS = swephexp.h sweodef.h swedate.h swehouse.h swedll.h

# The pkg-config version is the upstream API level -- SE_VERSION with the
# -ts suffix removed and leading zeros normalised (X.Y.0Z-ts.N -> X.Y.Z).
#
# That is deliberate, and it is NOT the same question as SE_VERSION.
# pkg-config's version answers "which API do you implement", which is what
# a consumer compares against; pyswisseph, for one, tests it for exact
# equality with 2.10.3 and refuses the library otherwise. SE_VERSION
# answers "which build is this", and swe_version() still reports the full
# 2.10.03-ts.N at runtime. The .pc also exposes it as a variable:
#
#   pkg-config --variable=swe_full_version swe
#
# Derived rather than written down, so it cannot drift from SE_VERSION.
SWE_API_VERSION = $(shell sed -n '$(VERSION_SED)' sweph.h | sed 's/-ts\..*//' \
                  | awk -F. '{for(i=1;i<=NF;i++) printf "%s%d", (i>1?".":""), $$i+0}')

.PHONY: install swe.pc
swe.pc:
	@printf '%s\n' \
	  'prefix=$(PREFIX)' \
	  'exec_prefix=$${prefix}' \
	  'libdir=$(LIBDIR)' \
	  'includedir=$(INCLUDEDIR)' \
	  'swe_full_version=$(shell sed -n '$(VERSION_SED)' sweph.h)' \
	  '' \
	  'Name: swe' \
	  'Description: Swiss Ephemeris (thread-safe fork)' \
	  'URL: https://github.com/nrvate/swisseph' \
	  'Version: $(SWE_API_VERSION)' \
	  'Libs: -L$${libdir} -lswe' \
	  'Libs.private: $(LIBS)' \
	  'Cflags: -I$${includedir}' > swe.pc
	@echo "swe.pc: Version $(SWE_API_VERSION), swe_full_version $$(sed -n '$(VERSION_SED)' sweph.h)"

install: libswe.a libswe.$(DYLIB_EXT) swe.pc
	$(INSTALL) -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m 644 libswe.a              $(DESTDIR)$(LIBDIR)/
	$(INSTALL) -m 755 libswe.$(DYLIB_EXT)   $(DESTDIR)$(LIBDIR)/
	$(INSTALL) -m 644 $(PUBLIC_HEADERS)     $(DESTDIR)$(INCLUDEDIR)/
	$(INSTALL) -m 644 swe.pc                $(DESTDIR)$(PKGCONFIGDIR)/
	@echo "installed to $(DESTDIR)$(PREFIX)"
	@echo "  PKG_CONFIG_PATH=$(DESTDIR)$(PKGCONFIGDIR) pkg-config --cflags --libs swe"

# Test targets (requires a "setest" subdirectory with its own Makefile)
test:
	cd setest && make && ./setest t

test.exp:
	cd setest && make && ./setest -g t

# Clean up build artifacts
clean:
	rm -f *.o swetest libswe.* swetests swevents swemini swe.pc
	cd setest && make clean

# Dependency rules
swecl.o: swejpl.h sweodef.h swephexp.h swedll.h sweph.h swephlib.h
sweclips.o: sweodef.h swephexp.h swedll.h
swedate.o: swephexp.h sweodef.h swedll.h
swehel.o: swephexp.h sweodef.h swedll.h
swehouse.o: swephexp.h sweodef.h swedll.h swephlib.h swehouse.h
swejpl.o: swephexp.h sweodef.h swedll.h sweph.h swejpl.h
swemini.o: swephexp.h sweodef.h swedll.h
swemmoon.o: swephexp.h sweodef.h swedll.h sweph.h swephlib.h
swemplan.o: swephexp.h sweodef.h swedll.h sweph.h swephlib.h swemptab.h
sweph.o: swejpl.h sweodef.h swephexp.h swedll.h sweph.h swephlib.h
swephlib.o: swephexp.h sweodef.h swedll.h sweph.h swephlib.h
swetest.o: swephexp.h sweodef.h swedll.h
swevents.o: swephexp.h sweodef.h swedll.h
