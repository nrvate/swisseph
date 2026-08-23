/* glprace.c -- swe_get_library_path() thread safety and buffer bounds.
 *
 * sweph.c kept the Dl_info that dladdr() fills as a file-scope static, so
 * concurrent callers race on it.  The function also wrote s[AS_MAXCH],
 * one past the end of a caller's char[AS_MAXCH] buffer.
 *
 * Each thread passes an over-allocated buffer with a canary immediately
 * after the AS_MAXCH-sized region the API is allowed to touch.
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "swephexp.h"

#define NTHREADS 4
#define NITER    500
#define CANARY   0x5A

static volatile int canary_hits[NTHREADS];
static volatile int empty_hits[NTHREADS];
static volatile int unterminated[NTHREADS];

static void *worker(void *arg) {
  long id = (long)arg;
  for (int i = 0; i < NITER; i++) {
    char buf[AS_MAXCH + 16];
    memset(buf, CANARY, sizeof buf);
    buf[0] = '\0';
    swe_get_library_path(buf);
    for (size_t k = AS_MAXCH; k < sizeof buf; k++)
      if ((unsigned char)buf[k] != CANARY) { canary_hits[id]++; break; }
    if (memchr(buf, '\0', AS_MAXCH) == NULL) unterminated[id]++;
    if (buf[0] == '\0') empty_hits[id]++;
  }
  return NULL;
}

int main(void) {
  pthread_t t[NTHREADS];
  char one[AS_MAXCH];
  swe_get_library_path(one);
  printf("library path: %s\n", one[0] ? "(non-empty)" : "(EMPTY -- regression!)");
  for (long i = 0; i < NTHREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
  for (int i = 0; i < NTHREADS; i++) pthread_join(t[i], NULL);
  int c = 0, e = 0, u = 0;
  for (int i = 0; i < NTHREADS; i++) { c += canary_hits[i]; e += empty_hits[i]; u += unterminated[i]; }
  printf("canary overwritten : %d/%d\n", c, NTHREADS * NITER);
  printf("empty result       : %d/%d\n", e, NTHREADS * NITER);
  printf("unterminated       : %d/%d\n", u, NTHREADS * NITER);
  int bad = (c || u || e || !one[0]);
  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
