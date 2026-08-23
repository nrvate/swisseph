# MSVC project files

Visual Studio projects for building Swiss Ephemeris on Windows. Open
`sweph.sln`, or build a single project from the command line:

```
msbuild swedll64.vcxproj /p:Configuration=Release /p:Platform=x64
```

Output goes to `out\<Platform>\<Configuration>\`; intermediates to `obj\`.
Both are gitignored.

**Build each project at the platform its name implies.** Building the whole
solution at one platform works, but happily produces a 64-bit file called
`swedll32.dll`. The CI job (`windows / package`) maps them explicitly.

| Project | Platform | Produces |
|---|---|---|
| `swedll32` / `swedll64` | Win32 / x64 | DLL + import lib |
| `swelib32` / `swelib64` | Win32 / x64 | static library |
| `swetest` / `swetest64` | Win32 / x64 | CLI, statically linked |
| `swete32` / `swete64` | Win32 / x64 | CLI, linked against the DLL (`USE_DLL`) |
| `swewin32` / `swewin64` | Win32 / x64 | GUI sample from `../swewin/` |
| `swedlltrm*` / `swedlltrs*` | Win32 / x64 | DLL variants |

## Where these came from, and what had to be fixed

They were extracted from `windows/sweph.zip`, an archive that also carried a
stale duplicate of the entire source tree. As shipped they could not build
this repository at all:

| Problem | Fix |
|---|---|
| `PlatformToolset v140_xp` — **removed in VS2019+** | `v143` |
| `WindowsTargetPlatformVersion 8.1` — SDK not on modern runners | `10.0` |
| `OutDir` hardcoded to `s:\devlop\sweph\bin\` — the original author's drive | `$(SolutionDir)out\...` |
| **None of the 15 listed `sweconfig.c`** — added by this branch, and the library now requires it | added to all 8 library projects, cloning `sweph.c`'s element so it inherits the same per-configuration optimisation settings |
| Source paths relative to the archive's `src/projects/` | rewritten for this location |
| `.rc` files referenced a `resource.h` **not present in the archive**, and MFC's `afxres.h` | dropped — the scripts contained no actual resources, only Visual Studio `TEXTINCLUDE` stubs |

CI builds every project here on each push, so they cannot quietly rot again.
That is not a hypothetical concern on this branch: `interp()`,
`swethread.h`'s tier 5 and `sweephe4.c` were all in the tree, all unbuilt,
and all broken by the time anyone looked.

## `swetrace.vcxproj` is deliberately excluded from the solution

It is the one project CI cannot build, and that is correct rather than
broken.

`swetrace.c` is not source — it is **generated output**. `swephlib.h` sets
`fname_trace_c = "swetrace.c"`, and a TRACE-enabled build writes a
replayable C program under that name as it runs. `swetrace.vcxproj` compiles
*your own* trace replay.

A copy was committed upstream in 2009 and deleted again in the 2.10
prerelease — correctly, since it was build output. Verified rather than
assumed: a TRACE build made here produces byte-identical structure to that
2009 file, down to the `/*SWE_CALC*/` markers and the
`"unknown to swetrace"` comment.

So the project is kept as a developer convenience, out of the solution
because it cannot build from a clean checkout.
