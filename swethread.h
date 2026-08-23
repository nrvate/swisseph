/************************************************************
  $Header: swethread.h $

  Minimal threading primitives for the Swiss Ephemeris.

  This is the only place in libswe that knows about threads. It provides
  exactly what the configuration-propagation work needs and nothing more:

    - swi_mutex_t          a statically-initialisable mutex
    - swi_mutex_lock/unlock
    - swi_gen_t            a generation counter
    - swi_gen_load         acquire-load of a generation counter
    - swi_gen_bump         release-increment, returns the new value

  Design constraints, from notes/PLAN.md section 14.4:

  1. The library pins no -std= anywhere, so the C dialect floats with
     whatever the local cc defaults to. C11 <stdatomic.h> disappears under
     -std=c89; the GCC/Clang __atomic builtins do not. The builtins are
     therefore preferred over C11, not the other way round.
  2. sweodef.h hand-rolls its integer types and has no uint64, so the
     generation counter is uint32. Only ever compared with != , so wrapping
     after 2^32 configuration changes is harmless.
  3. Everything must compile away to nothing under -DSWE_NO_THREADS, so
     single-threaded and embedded builds take no dependency at all.

  Backend selection, in order of preference:

    SWE_NO_THREADS defined  -> no-ops
    _WIN32                  -> SRWLOCK + Interlocked intrinsics
    __ATOMIC_SEQ_CST        -> pthreads + __atomic builtins   (gcc/clang)
    C11 without the above   -> pthreads + <stdatomic.h>
    otherwise               -> pthreads, counter guarded by its own mutex

  The last fallback is always correct, just slower: it takes a lock to read
  the generation counter instead of doing an atomic load.

************************************************************/

#ifndef _SWETHREAD_INCLUDED
#define _SWETHREAD_INCLUDED

#include "sweodef.h"

/* `inline` is not a keyword in C89, and MSVC before 2015 has no C99 inline
 * for C code either. Since the library pins no -std=, we may well be
 * compiled as C89, so this cannot simply be `static inline`. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
# define SWI_INLINE static inline
#elif defined(__GNUC__)
# define SWI_INLINE static __inline__
#elif defined(_MSC_VER)
# define SWI_INLINE static __inline
#else
/* Last resort: a plain static function. Correct everywhere; may draw an
 * unused-function warning in translation units that don't call all of them. */
# define SWI_INLINE static
#endif

typedef uint32 swi_gen_t;

/*======================================================================
 * 1. No threads at all
 *====================================================================*/
#if defined(SWE_NO_THREADS)

#define SWI_THREAD_BACKEND "none"
typedef int swi_mutex_t;
#define SWI_MUTEX_INIT 0
SWI_INLINE void swi_mutex_lock(swi_mutex_t *m)   { (void)m; }
SWI_INLINE void swi_mutex_unlock(swi_mutex_t *m) { (void)m; }
SWI_INLINE swi_gen_t swi_gen_load(const swi_gen_t *g) { return *g; }
SWI_INLINE swi_gen_t swi_gen_bump(swi_gen_t *g)       { return ++(*g); }

/*======================================================================
 * 2. Windows
 *====================================================================*/
#elif defined(_WIN32)

#include <windows.h>
#define SWI_THREAD_BACKEND "srwlock"
typedef SRWLOCK swi_mutex_t;
#define SWI_MUTEX_INIT SRWLOCK_INIT
SWI_INLINE void swi_mutex_lock(swi_mutex_t *m)   { AcquireSRWLockExclusive(m); }
SWI_INLINE void swi_mutex_unlock(swi_mutex_t *m) { ReleaseSRWLockExclusive(m); }

SWI_INLINE swi_gen_t swi_gen_load(const swi_gen_t *g)
{
  swi_gen_t v = *(volatile const swi_gen_t *)g;
  MemoryBarrier();
  return v;
}
SWI_INLINE swi_gen_t swi_gen_bump(swi_gen_t *g)
{
  return (swi_gen_t) InterlockedIncrement((volatile LONG *) g);
}

/*======================================================================
 * 3. POSIX + __atomic builtins  (preferred: survives -std=c89)
 *
 * Define SWE_PREFER_C11_ATOMICS to skip this and use <stdatomic.h>
 * instead. Only useful for exercising backend 4 in tests, or on a
 * toolchain where the builtins are known bad.
 *====================================================================*/
#elif defined(__ATOMIC_SEQ_CST) && !defined(SWE_PREFER_C11_ATOMICS)

#include <pthread.h>
#define SWI_THREAD_BACKEND "pthread+__atomic"
typedef pthread_mutex_t swi_mutex_t;
#define SWI_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
SWI_INLINE void swi_mutex_lock(swi_mutex_t *m)   { pthread_mutex_lock(m); }
SWI_INLINE void swi_mutex_unlock(swi_mutex_t *m) { pthread_mutex_unlock(m); }

SWI_INLINE swi_gen_t swi_gen_load(const swi_gen_t *g)
{
  return __atomic_load_n((const volatile swi_gen_t *) g, __ATOMIC_ACQUIRE);
}
SWI_INLINE swi_gen_t swi_gen_bump(swi_gen_t *g)
{
  return __atomic_add_fetch(g, 1u, __ATOMIC_RELEASE);
}

/*======================================================================
 * 4. POSIX + C11 <stdatomic.h>
 *====================================================================*/
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L \
      && !defined(__STDC_NO_ATOMICS__)

#include <pthread.h>
#include <stdatomic.h>
#define SWI_THREAD_BACKEND "pthread+c11"
typedef pthread_mutex_t swi_mutex_t;
#define SWI_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
SWI_INLINE void swi_mutex_lock(swi_mutex_t *m)   { pthread_mutex_lock(m); }
SWI_INLINE void swi_mutex_unlock(swi_mutex_t *m) { pthread_mutex_unlock(m); }

SWI_INLINE swi_gen_t swi_gen_load(const swi_gen_t *g)
{
  return atomic_load_explicit((const _Atomic swi_gen_t *) g, memory_order_acquire);
}
SWI_INLINE swi_gen_t swi_gen_bump(swi_gen_t *g)
{
  return atomic_fetch_add_explicit((_Atomic swi_gen_t *) g, 1u,
                                   memory_order_release) + 1u;
}

/*======================================================================
 * 5. POSIX, no atomics: guard the counter with its own mutex
 *====================================================================*/
#else

#include <pthread.h>
#define SWI_THREAD_BACKEND "pthread+mutex"
typedef pthread_mutex_t swi_mutex_t;
#define SWI_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
SWI_INLINE void swi_mutex_lock(swi_mutex_t *m)   { pthread_mutex_lock(m); }
SWI_INLINE void swi_mutex_unlock(swi_mutex_t *m) { pthread_mutex_unlock(m); }

/* One lock shared by every generation counter. There is expected to be
 * exactly one, and this backend is the last resort anyway. */
extern swi_mutex_t swi_gen_fallback_mutex;

SWI_INLINE swi_gen_t swi_gen_load(const swi_gen_t *g)
{
  swi_gen_t v;
  pthread_mutex_lock(&swi_gen_fallback_mutex);
  v = *g;
  pthread_mutex_unlock(&swi_gen_fallback_mutex);
  return v;
}
SWI_INLINE swi_gen_t swi_gen_bump(swi_gen_t *g)
{
  swi_gen_t v;
  pthread_mutex_lock(&swi_gen_fallback_mutex);
  v = ++(*g);
  pthread_mutex_unlock(&swi_gen_fallback_mutex);
  return v;
}

#endif  /* backend selection */

#endif  /* _SWETHREAD_INCLUDED */
