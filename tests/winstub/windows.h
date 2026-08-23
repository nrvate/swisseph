/* Minimal windows.h stand-in, for SYNTAX-CHECKING the _WIN32 code paths on
 * a machine that has no Windows SDK.
 *
 * This is not an implementation and nothing links against it. Its whole job
 * is to let a Linux compiler parse the `#if defined(_WIN32)` branches of
 * tests/golden.c, tests/ctxtest.c and tests/threadshim.c, so that a
 * Windows-only break is caught in a second here rather than after a
 * push-and-wait cycle against the MSVC job.
 *
 * It exists because that cycle cost four round trips over a single bug: the
 * OUT macro below is precisely what broke tests/golden.c, and defining it
 * here reproduces the failure exactly.
 *
 * Keep it minimal. Every declaration should be one some _WIN32 branch in
 * this tree actually needs; it is a test fixture, not a compatibility layer.
 */
#ifndef SWE_TESTS_WINSTUB_WINDOWS_H
#define SWE_TESTS_WINSTUB_WINDOWS_H

#include <stddef.h>

/* The SAL parameter annotations. winnt.h defines these as EMPTY, which is
 * the entire point of the stub: any identifier in this tree named IN, OUT,
 * OPTIONAL, FAR or NEAR silently disappears on Windows. */
#define IN
#define OUT
#define OPTIONAL
#define FAR
#define NEAR

typedef void          *HANDLE;
typedef unsigned long  DWORD;
typedef int            BOOL;
typedef void          *LPVOID;
typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);

#define INFINITE            0xFFFFFFFFUL
#define WAIT_OBJECT_0       0UL

HANDLE CreateThread(void *, size_t, LPTHREAD_START_ROUTINE, LPVOID,
                    DWORD, DWORD *);
DWORD  WaitForSingleObject(HANDLE, DWORD);
BOOL   CloseHandle(HANDLE);

/* SRWLOCK, for swethread.h's Windows backend. */
typedef struct { void *Ptr; } SRWLOCK;
#define SRWLOCK_INIT { 0 }
void AcquireSRWLockExclusive(SRWLOCK *);
void ReleaseSRWLockExclusive(SRWLOCK *);

long InterlockedIncrement(long volatile *);
long InterlockedCompareExchange(long volatile *, long, long);

#endif /* SWE_TESTS_WINSTUB_WINDOWS_H */
