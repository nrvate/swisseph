## Swiss Ephemeris for Windows

The folder 'windows' contains

### The MSVC build system

folder projects/	Visual Studio projects and `sweph.sln`. CI builds every
			one of them on each push; see `projects/README.md`.

folder swewin/	Windows sample program sources (swewin.c, swewin64.c and resources)

folder vb/	declarations files for VB

### Prebuilt binaries are no longer checked in

`programs/` used to hold seven compiled `.exe` samples. They were removed:
they were built from Swiss Ephemeris **2.10.03** and so contain none of the
thread-safety work on this branch, which made them actively misleading in a
fork whose whole point is that the library behaves differently. All of them
are rebuildable from source in this repository.

`ceres.exe` was removed outright rather than rebuilt. Its own readme stated
it "is NOT part of the Swiss Ephemeris, and it is NOT supported", and no
source for it exists anywhere in this tree.

For official upstream Windows binaries, see
https://github.com/aloistr/swisseph — this fork does not redistribute them.

CI-built artifacts are the replacement, and they exist now: every release
carries a `swisseph-windows-*.zip` with both 32- and 64-bit DLLs, import
libraries, static libraries, `swetest`, the sample programs, the diagnostic
DLLs and the public headers, plus a `SHA256SUMS` covering all of it. See
[the latest release](https://github.com/nrvate/swisseph/releases/latest).
