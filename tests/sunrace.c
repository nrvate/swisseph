/* sunrace.c -- proves the saved_sundec data race in swehouse.c.
 *
 * swe_houses_armc_ex2() with hsys 'I' (Sunshine) keeps the last supplied
 * solar declination in a file-scope static so that a later caller can pass
 * the sentinel 99 and have it recalled.  That static is not thread-local,
 * so concurrent callers both race on it AND can silently recall a
 * declination belonging to a completely different date.
 *
 * Build (race detection):
 *   cc -g -O1 -fsanitize=thread -I.. sunrace.c ../sw*.c -lm -ldl -lpthread -o sunrace
 *   (spelled sw*.c so the glob does not open a nested comment; clang warns)
 *   setarch -R ./sunrace
 *
 * Exit 0 = no cross-thread contamination observed.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "swephexp.h"

#define NTHREADS 4
#define NITER    400

/* Each thread owns a distinct declination and must never observe another's. */
static const double MINE[NTHREADS] = { 23.44, -23.44, 11.0, -5.5 };
static volatile int contaminated[NTHREADS];

static void *worker(void *arg) {
  long id = (long)arg;
  double cusp[37], ascmc[10];
  char serr[AS_MAXCH];
  for (int i = 0; i < NITER; i++) {
    /* store our own declination ... */
    memset(cusp, 0, sizeof cusp); memset(ascmc, 0, sizeof ascmc); serr[0] = 0;
    ascmc[9] = MINE[id];
    swe_houses_armc_ex2(37.5 * id + 3.0, 47.37, 23.4392911, 'I',
                        cusp, ascmc, NULL, NULL, serr);
    /* ... then recall it via the sentinel. It must still be ours. */
    memset(cusp, 0, sizeof cusp); memset(ascmc, 0, sizeof ascmc); serr[0] = 0;
    ascmc[9] = 99;
    swe_houses_armc_ex2(37.5 * id + 3.0, 47.37, 23.4392911, 'I',
                        cusp, ascmc, NULL, NULL, serr);
    if (fabs(ascmc[9] - MINE[id]) > 1e-12) contaminated[id]++;
  }
  return NULL;
}

int main(void) {
  pthread_t t[NTHREADS];
  swe_set_ephe_path(getenv("SE_TEST_EPHE") ? getenv("SE_TEST_EPHE") : "../ephe");
  for (long i = 0; i < NTHREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
  for (int i = 0; i < NTHREADS; i++) pthread_join(t[i], NULL);

  int total = 0;
  for (int i = 0; i < NTHREADS; i++) {
    printf("thread %d (dec %+7.2f): %4d/%d recalls contaminated\n",
           i, MINE[i], contaminated[i], NITER);
    total += contaminated[i];
  }
  printf("%s: %d/%d recalls returned another thread's declination\n",
         total ? "FAIL" : "PASS", total, NTHREADS * NITER);
  swe_close();
  return total ? 1 : 0;
}
