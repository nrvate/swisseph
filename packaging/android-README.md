# Swiss Ephemeris for Android (thread-safe fork)

Built by CI from source. Nothing here is checked into the repository — see
`SHA256SUMS` for the manifest of this build.

This is a **JNI library**, not a general-purpose `libswe`. It is compiled
with `-fvisibility=hidden`, so it exports the `Java_*` entry points that
`swejni.h` declares and keeps the C `swe_*` API internal. If you want to call
the C API directly, take the Linux package and build for your ABI, or build
from source.

## What is here

```
jni/arm64-v8a/     libswe-*.so
jni/armeabi-v7a/
jni/riscv64/
jni/x86/
jni/x86_64/
swejni.h           the JNI entry points
```

Drop `jni/` into your project's `src/main/jniLibs/` (Gradle) or `libs/`
(ndk-build), and Android will pick the right ABI per device.

## Threading

`THREADING.md` explains what differs from upstream. The part that matters on
Android: configuration set on one thread is now visible to the others.
Upstream gave each thread its own private copy, so a worker silently fell
back to the built-in Moshier ephemeris — lower precision, different answers,
no error — while the main thread thought it had set an ephemeris path.

That is a particularly easy trap on Android, where almost nothing runs on
the main thread.

## Ephemeris data

Not included — it is hundreds of megabytes. Ship the `.se1` files you need as
assets, copy them somewhere readable at runtime, and call
`swe_set_ephe_path()` with that directory. Without them the library uses the
built-in Moshier ephemeris, which needs no files at all and is often enough
for chart work.

## Verifying

```sh
sha256sum -c SHA256SUMS
```
