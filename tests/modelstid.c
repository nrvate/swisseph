/* modelstid.c -- G113: swe_set_astro_models() of a version before 2.01 sets
 * the tidal acceleration on the default context, and the context keeps it.
 *
 * The defect (UPSTREAM-BUGS 116, this fork's configuration layer): those
 * versions set the term with swe_set_tid_acc() inside the setter's guard,
 * which publishes nothing, and the setter published only the SID group. The
 * next sync on the default context -- swe_get_tid_acc() itself, or the next
 * calculation -- took the master's automatic term back.
 *
 * The check: nine versions (SE1.80 .. SE1.00, SE2.00 and SE2.06), each in
 * four orders on the default context -- path, models, calculation; models,
 * path, calculation; swe_get_astro_models() (which calls the setter); the
 * models set and the calculation on another thread -- each in a fresh
 * process, against an explicit context given the same calls: the term read
 * right after the setting (on the setting thread), after a Moon at J2000,
 * and delta t in AD 1000, within 1e-12 and 1e-9 s.
 *
 * Measured before the fix, 32 of 36 fail (every version but SE2.06): SE1.80
 * -25.936 for -25.826, delta t in AD 1000 1568.888 s for 1559.759; SE1.72
 * -25.936 for -25.7376, 1564.775 s for 1548.309; SE1.70 1594.586 s for
 * 1578.121; SE2.00 (the files' Sun is DE431) as SE1.80. The compat build
 * fails the same 32 (with entries 92 and 98 restored, the path after the
 * models also zeroes them: 1474.792 s). The upstream build has no explicit
 * contexts and no shared configuration to lose it in. Run as
 *   modelstid EPHEDIR
 */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T0 2451545.0
#define T_1000 2086302.5
#define TOL_TID 1e-12
#define TOL_DT 1e-9	/* s */

static const char *EPHE;

/* the default context, then the version, a calculation, the readings */
enum { V_180, V_178, V_177, V_172, V_170, V_164, V_100, V_200, V_206, NV };
static const char *VS[NV] = { "SE1.80", "SE1.78", "SE1.77", "SE1.72", "SE1.70",
                              "SE1.64", "SE1.00", "SE2.00", "SE2.06" };
enum { O_SET, O_BEFORE_PATH, O_GET_MODELS, O_THREAD, NO };
static const char *ON[NO] = { "path, models, calc", "models, path, calc",
                              "swe_get_astro_models(), calc",
                              "models; another thread's calc" };

struct ans { double tid_set, tid_calc, dt; int ran; };

static void *worker(void *arg)
{
  struct ans *a = arg;
  double x[6];
  char serr[AS_MAXCH];
  swe_calc_ut(T0, SE_MOON, SEFLG_SWIEPH, x, serr);
  a->tid_calc = swe_get_tid_acc();
  a->dt = swe_deltat(T_1000) * 86400;
  return NULL;
}

/* o < 0: the reference, an explicit context */
static void scenario(int v, int o, struct ans *a)
{
  char m[AS_MAXCH], d[4096];	/* swe_get_astro_models() writes ~480 bytes of detail */
  double x[6];
  char serr[AS_MAXCH];
  strcpy(m, VS[v]);
  if (o < 0) {
    swe_ctx *c = swe_ctx_new();
    swe_set_ephe_path_r(c, (char *) EPHE);
    swe_set_astro_models_r(c, m, 0);
    a->tid_set = swe_get_tid_acc_r(c);
    swe_calc_ut_r(c, T0, SE_MOON, SEFLG_SWIEPH, x, serr);
    a->tid_calc = swe_get_tid_acc_r(c);
    a->dt = swe_deltat_r(c, T_1000) * 86400;
    swe_ctx_free(c);
    a->ran = 1;
    return;
  }
  if (o != O_BEFORE_PATH)
    swe_set_ephe_path((char *) EPHE);
  if (o == O_GET_MODELS)
    swe_get_astro_models(m, d, 0);
  else
    swe_set_astro_models(m, 0);
  if (o == O_BEFORE_PATH)
    swe_set_ephe_path((char *) EPHE);
  a->tid_set = swe_get_tid_acc();
  if (o == O_THREAD) {
    pthread_t th;
    if (pthread_create(&th, NULL, worker, a) != 0)
      return;
    pthread_join(th, NULL);
  } else
    worker(a);
  a->ran = 1;
}

static int in_child(int v, int o, struct ans *out)
{
  int fd[2];
  ssize_t got = 0, r;
  pid_t pid;
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    struct ans a;
    close(fd[0]);
    memset(&a, 0, sizeof a);
    scenario(v, o, &a);
    _exit(write(fd[1], &a, sizeof a) == (ssize_t) sizeof a ? 0 : 3);
  }
  close(fd[1]);
  memset(out, 0, sizeof *out);
  while (got < (ssize_t) sizeof *out
         && (r = read(fd[0], (char *) out + got, sizeof *out - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return (got == (ssize_t) sizeof *out && out->ran) ? 0 : -1;
}

int main(int argc, char **argv)
{
  int v, o, n = 0, nfail = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  for (v = 0; v < NV; v++) {
    struct ans ref;
    if (in_child(v, -1, &ref) != 0) {
      printf("  %s reference did not run\n", VS[v]);
      nfail++;
      continue;
    }
    printf("  %s: explicit context %.4f, delta t AD 1000 %.3f s\n", VS[v], ref.tid_calc, ref.dt);
    for (o = 0; o < NO; o++, n++) {
      struct ans a;
      int bad;
      if (in_child(v, o, &a) != 0) {
        printf("    %-32s did not run\n", ON[o]);
        nfail++;
        continue;
      }
      /* the setting thread reads the term before its calculation as well */
      bad = (o != O_THREAD && fabs(a.tid_set - ref.tid_set) > TOL_TID)
            || fabs(a.tid_calc - ref.tid_calc) > TOL_TID
            || fabs(a.dt - ref.dt) > TOL_DT;
      if (bad) {
        printf("    %-32s FAIL: %.4f set, %.4f after the calculation, %.3f s\n",
               ON[o], a.tid_set, a.tid_calc, a.dt);
        nfail++;
      }
    }
  }
  if (nfail)
    printf("G113 FAIL: %d of %d\n", nfail, n);
  else
    printf("G113 PASS (%d)\n", n);
  return nfail != 0;
}
