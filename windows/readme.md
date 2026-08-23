## Swiss Ephemeris for Windows

The folder 'windows'
https://github.com/aloistr/swisseph/windows
contains

file sweph.zip
which packages Swiss Ephemeris for Windows

- sweph/bin	compiled 32-bit and 64-bit DLLs, compiled binary sample programs
- sweph/src	the C source code used for compilation of DLLs and binaries. This is usually an older version of the source code than the one in the mail git repository.
- sweph/src/projects the project files for Visual Studio to build the binaries
- sweph/doc 	documentation files, often older than the ones in the main /doc folder
- sweph/vb	sample and support files for VB.  Note: Visual Basic files may not have been updated and
  tested, as we currently do not possess a working copy of VB at Astrodienst.

file swephzip.txt 	containing an overview of content of the sweph.zip package

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
CI-built artifacts are the intended replacement; see the release plan in
`notes/`.
