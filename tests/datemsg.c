/* datemsg.c -- G51: the refusal of a date far outside a range names the date
 * in a message that fits serr, and the range with it.
 *
 * The refusals of a date outside a range printed it by "%f" with
 * sprintf(), the date's digits in full, into 256 bytes: the Moshier
 * planets', Moon's, mean node's and mean apogee's into a local buffer,
 * the JPL file's straight into serr, and swe_utc_to_jd()'s "invalid time"
 * with the caller's seconds. From |J| about 1e188 days (Moshier) to 1e196
 * (JPL), and seconds from 1e232, the message outgrew the buffer: glibc's
 * fortify aborted ("buffer overflow detected"), at -O0 the stack protector
 * ("stack smashing detected"), and where it reached serr the bytes past
 * the 256 the API allows were overwritten.
 *
 * Check (a): at |J| 1e200, 1e250, 1e300, 1e308 and DBL_MAX, either sign,
 * Mars, the Moon, the mean node, the mean apogee and Cupido on Moshier,
 * swe_nod_aps() of the Moon's and Mars's mean points, Spica by
 * swe_fixstar2() on Moshier and Mars on de200.eph; and swe_utc_to_jd()
 * with those seconds. Each call in a child process: ERR, a message from
 * "jd " (or "invalid time") of under 100 characters, that says "outside"
 * and the range it was refused for (or ends in the seconds), the date read
 * as "e+", and the bytes of the caller's buffer past AS_MAXCH as they were.
 * 100 checks.
 *
 * Check (b): a date a little outside a range reads as it always did, the
 * date by "%f" (the seconds by "%.2f"): 4 checks.
 *
 * Run as
 *   datemsg EPHEDIR
 */
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "swephexp.h"

static const char *EPHE;
static double T;	/* the date of the call in the child */

/* the caller's buffer: AS_MAXCH the API allows, and a canary after it */
static char SERR[4 * AS_MAXCH];
static void serr_arm(void) { memset(SERR, 0x5a, sizeof SERR); SERR[0] = '\0'; }
static int serr_kept(void)
{
  int i;
  for (i = AS_MAXCH; i < (int) sizeof SERR; i++)
    if (SERR[i] != 0x5a)
      return 0;
  return 1;
}

static swe_ctx *fresh(void)
{
  swe_ctx *c = swe_ctx_new();
  if (c == NULL)
    _exit(3);
  swe_set_ephe_path_r(c, (char *) EPHE);
  swe_set_jpl_file_r(c, "de200.eph");
  return c;
}

#define NA 10
/* call k at T: its return, the message in SERR */
static int32 call(int k)
{
  swe_ctx *c = fresh();
  double x[6], b[6], p[6], q[6], d[2];
  char star[AS_MAXCH];
  serr_arm();
  switch (k) {
  case 0: return swe_calc_r(c, T, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR);
  case 1: return swe_calc_r(c, T, SE_MOON, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR);
  case 2: return swe_calc_r(c, T, SE_MEAN_NODE, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR);
  case 3: return swe_calc_r(c, T, SE_MEAN_APOG, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR);
  case 4: return swe_calc_r(c, T, SE_CUPIDO, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR);
  case 5: return swe_nod_aps_r(c, T, SE_MOON, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR);
  case 6: return swe_nod_aps_r(c, T, SE_MARS, SEFLG_MOSEPH | SEFLG_SPEED, SE_NODBIT_MEAN, x, b, p, q, SERR);
  case 7: strcpy(star, "Spica"); return swe_fixstar2_r(c, star, T, SEFLG_MOSEPH | SEFLG_SPEED, x, SERR);
  case 8: return swe_calc_r(c, T, SE_MARS, SEFLG_JPLEPH | SEFLG_SPEED, x, SERR);
  default: return swe_utc_to_jd_r(c, 2000, 1, 1, 0, 0, T, SE_GREG_CAL, d, SERR);
  }
}

/* call k at t in a child process: whether it returned ERR (0 if not, or if
 * it died), and the message it left */
static int in_child(int k, double t, char *msg, int *kept)
{
  int fd[2], st;
  pid_t pid;
  ssize_t got;
  char buf[AS_MAXCH + 2];
  msg[0] = '\0';
  *kept = 0;
  fflush(stdout);
  if (pipe(fd) != 0)
    return 0;
  pid = fork();
  if (pid < 0)
    return 0;
  if (pid == 0) {
    int32 r;
    close(fd[0]);
    T = t;
    r = call(k);
    memset(buf, 0, sizeof buf);
    buf[0] = (r == ERR) ? 'E' : 'o';
    buf[1] = serr_kept() ? 'k' : 'x';
    memcpy(buf + 2, SERR, strnlen(SERR, AS_MAXCH - 1));
    _exit(write(fd[1], buf, sizeof buf) == (ssize_t) sizeof buf ? 0 : 4);
  }
  close(fd[1]);
  got = read(fd[0], buf, sizeof buf);
  close(fd[0]);
  if (waitpid(pid, &st, 0) != pid || !WIFEXITED(st) || WEXITSTATUS(st) != 0
      || got != (ssize_t) sizeof buf)
    return 0;
  strcpy(msg, buf + 2);
  *kept = buf[1] == 'k';
  return buf[0] == 'E';
}

static int n, bad;
static void check(int ok, const char *what, double t, const char *msg)
{
  n++;
  if (!ok) {
    bad++;
    printf("    %s at %g: %s FAIL\n", what, t, msg);
  }
}

static const char *NAME[NA] = { "Mars", "Moon", "mean node", "mean apogee", "Cupido",
  "nod_aps Moon", "nod_aps Mars", "Spica", "Mars, de200.eph", "swe_utc_to_jd" };

int main(int argc, char **argv)
{
  static const double MAG[5] = { 1e200, 1e250, 1e300, 1e308, DBL_MAX };
  int i, k, sgn;
  if (argc < 2) {
    fprintf(stderr, "usage: datemsg EPHEDIR\n");
    return 2;
  }
  EPHE = argv[1];
  for (i = 0; i < 5; i++)
    for (sgn = -1; sgn <= 1; sgn += 2)
      for (k = 0; k < NA; k++) {
        double t = sgn * MAG[i];
        char msg[AS_MAXCH + 2];
        int kept = 0, err = in_child(k, t, msg, &kept);
        int fits = strlen(msg) < 100 && kept;
        int says = k == NA - 1 ? strncmp(msg, "invalid time", 12) == 0
                               : strncmp(msg, "jd ", 3) == 0
                                 && strstr(msg, "outside") != NULL && strstr(msg, " .. ") != NULL;
        check(err && fits && says && strstr(msg, "e+") != NULL, NAME[k], t, msg);
      }
  /* a date a little outside a range: as it always read */
  {
    char msg[AS_MAXCH + 2];
    int kept = 0;
    check(in_child(0, 3000000.5, msg, &kept) && kept
          && strncmp(msg, "jd 3000000.500000 outside Moshier planet range ", 47) == 0,
          "Mars", 3000000.5, msg);
    check(in_child(1, 3000000.5, msg, &kept) && kept
          && strncmp(msg, "jd 3000000.500000 outside Moshier's Moon range ", 47) == 0,
          "Moon", 3000000.5, msg);
    check(in_child(2, 9000000.5, msg, &kept) && kept
          && strstr(msg, "jd 9000000.500000 outside mean node range ") != NULL,
          "mean node", 9000000.5, msg);
    check(in_child(NA - 1, 61.0, msg, &kept) && kept
          && strcmp(msg, "invalid time: 0:0:61.00") == 0,
          "swe_utc_to_jd", 61.0, msg);
  }
  if (bad)
    printf("G51 FAIL: %d of %d\n", bad, n);
  else
    printf("G51 PASS (%d)\n", n);
  return bad ? 1 : 0;
}
