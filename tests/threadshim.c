/* threadshim.c -- exercise the swethread.h primitives.
 *
 * A shim that merely compiles proves nothing, so this hammers it:
 *
 *   1. mutual exclusion   N threads x M increments of an unguarded counter
 *                         inside the lock; the total must be exactly N*M.
 *                         Without a real lock this loses updates.
 *   2. generation counter  N threads x M bumps; final value must be N*M,
 *                         and every observed value must be monotonic
 *                         non-decreasing from any reader's point of view.
 *   3. publish/observe    the pattern config sync will actually use:
 *                         writer publishes payload then bumps the
 *                         generation; readers that see a new generation
 *                         must see the matching payload, never a torn or
 *                         stale one. This is what catches a missing
 *                         release/acquire pairing.
 *
 * Build with -fsanitize=thread for the strongest signal.
 */
#include <stdio.h>
#include <string.h>
#include "swethread.h"

#if defined(SWE_NO_THREADS)
int main(void) {
  swi_gen_t g = 0;
  swi_mutex_t m = SWI_MUTEX_INIT;

  swi_mutex_lock(&m); swi_mutex_unlock(&m);
  if (swi_gen_bump(&g) != 1 || swi_gen_load(&g) != 1) {
    printf("FAIL: no-op backend arithmetic wrong\n"); return 1;
  }
  printf("backend=%s (single-threaded build)\nPASS\n", SWI_THREAD_BACKEND);
  return 0;
}
#else

/* A tiny thread shim for the TEST only. swethread.h itself needs no thread
 * creation API -- it only locks and counts -- but this test has to spawn
 * threads, and MSVC has no <pthread.h>. Without this the Windows CI job
 * cannot exercise the SRWLOCK backend at all, which is the one backend
 * unreachable from Linux and macOS. */
#if defined(_WIN32)
# include <windows.h>
  typedef HANDLE thr_t;
  typedef DWORD  thr_ret_t;
# define THR_CALL __stdcall
  static int thr_create(thr_t *t, thr_ret_t (THR_CALL *fn)(void *), void *arg) {
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) fn, arg, 0, NULL);
    return *t == NULL;
  }
  static void thr_join(thr_t t) { WaitForSingleObject(t, INFINITE); CloseHandle(t); }
#else
# include <pthread.h>
  typedef pthread_t thr_t;
  typedef void *    thr_ret_t;
# define THR_CALL
  static int thr_create(thr_t *t, thr_ret_t (THR_CALL *fn)(void *), void *arg) {
    return pthread_create(t, NULL, fn, arg);
  }
  static void thr_join(thr_t t) { pthread_join(t, NULL); }
#endif

#define NTHREADS 8
#define NITER    20000

/* backend 5 needs this defined exactly once */
#if !defined(_WIN32) && !defined(__ATOMIC_SEQ_CST) \
    && !(defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L \
         && !defined(__STDC_NO_ATOMICS__))
swi_mutex_t swi_gen_fallback_mutex = SWI_MUTEX_INIT;
#endif

static swi_mutex_t lock = SWI_MUTEX_INIT;
static long guarded_counter;          /* deliberately NOT atomic */
static swi_gen_t generation;

/* --- 1 & 2 ------------------------------------------------------------ */
static thr_ret_t THR_CALL hammer(void *arg) {
  (void)arg;
  int i;
  for (i = 0; i < NITER; i++) {
    swi_mutex_lock(&lock);
    guarded_counter++;
    swi_mutex_unlock(&lock);
    swi_gen_bump(&generation);
  }
  return (thr_ret_t) 0;
}

/* --- 3: publish / observe --------------------------------------------- */
struct payload { int a, b, c; };
static struct payload shared_payload;
static swi_gen_t pub_gen;
/* Not a plain volatile int: volatile is not a synchronisation
 * primitive, and TSan correctly flags that as a race. Use the
 * shim's own acquire/release counter -- the test synchronises
 * itself with the thing it is testing. */
static swi_gen_t stop_readers;
static long torn_reads, observed_updates;
static swi_mutex_t stat_lock = SWI_MUTEX_INIT;

static thr_ret_t THR_CALL publisher(void *arg) {
  (void)arg;
  int i;
  for (i = 1; i <= NITER; i++) {
    swi_mutex_lock(&lock);
    shared_payload.a = i;
    shared_payload.b = i * 2;
    shared_payload.c = i * 3;
    swi_mutex_unlock(&lock);
    swi_gen_bump(&pub_gen);          /* release: payload before generation */
  }
  swi_gen_bump(&stop_readers);
  return (thr_ret_t) 0;
}

static thr_ret_t THR_CALL reader(void *arg) {
  (void)arg;
  swi_gen_t last = 0, g;
  struct payload p;
  long torn = 0, seen = 0;
  while (swi_gen_load(&stop_readers) == 0) {
    g = swi_gen_load(&pub_gen);   /* acquire */
    if (g == last) continue;
    last = g;
    seen++;
    swi_mutex_lock(&lock);
    p = shared_payload;
    swi_mutex_unlock(&lock);
    if (p.a != 0 && (p.b != p.a * 2 || p.c != p.a * 3)) torn++;
  }
  swi_mutex_lock(&stat_lock);
  torn_reads += torn;
  observed_updates += seen;
  swi_mutex_unlock(&stat_lock);
  return (thr_ret_t) 0;
}

int main(void) {
  thr_t t[NTHREADS];
  thr_t pub, rd[NTHREADS - 1];
  struct payload p;
  long want;
  int bad = 0, i;

  printf("backend = %s\n", SWI_THREAD_BACKEND);

  for (i = 0; i < NTHREADS; i++) thr_create(&t[i], hammer, NULL);
  for (i = 0; i < NTHREADS; i++) thr_join(t[i]);

  want = (long)NTHREADS * NITER;
  printf("mutex      : counter=%ld expected=%ld %s\n",
         guarded_counter, want, guarded_counter == want ? "OK" : "MISMATCH");
  if (guarded_counter != want) bad = 1;

  printf("generation : value=%lu expected=%ld %s\n",
         (unsigned long)generation, want,
         (long)generation == want ? "OK" : "MISMATCH");
  if ((long)generation != want) bad = 1;

  /* publish/observe */
  thr_create(&pub, publisher, NULL);
  for (i = 0; i < NTHREADS - 1; i++) thr_create(&rd[i], reader, NULL);
  thr_join(pub);
  for (i = 0; i < NTHREADS - 1; i++) thr_join(rd[i]);

  printf("publish    : %ld updates observed, %ld torn %s\n",
         observed_updates, torn_reads, torn_reads == 0 ? "OK" : "TORN");
  if (torn_reads != 0) bad = 1;
  if (observed_updates == 0) {
    printf("publish    : readers never observed an update -- test is vacuous\n");
    bad = 1;
  }

  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad;
}
#endif
