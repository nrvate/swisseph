/* helmemo.c -- G81: the heliacal memos answer only for what they hold.
 *
 * The defects (UPSTREAM-BUGS 82): swehel.c memoises five results per
 * context. kOZ() (ozone extinction), ka() (aerosol extinction) and
 * Deltam() (the extinction along the line of sight) were keyed on the
 * Sun's altitude and SunRA() alone (Deltam() on the object's altitude
 * too), but kOZ() depends on the latitude, ka() on the latitude, eye
 * height, temperature, humidity and visual range, and Deltam() on all of
 * those, the pressure and the high-precision flag: the same instant and
 * place under another atmosphere, or SE_HELFLAG_VISLIM_DARK (Sun altitude
 * -90) at another latitude or eye height, was answered with the previous
 * request's extinction. call_swe_fixstar_mag() kept the last star's
 * magnitude across swe_close() and a path to another catalogue, took the
 * name before the lookup (a failed lookup left it keyed to the previous
 * star's magnitude), and on a hit left serr as it was where the lookup
 * clears it. Nothing emptied them in swe_close(); kOZ(), ka() and
 * Deltam() took a key of 0 as empty (unreachable: SunRA() is never
 * exactly 0), and a hit dropped ka()'s warning. SunRA()'s memo is keyed
 * on UT, which is complete: SIMULATE_VICTORVB makes it the monthly
 * approximation, so the ephemeris and delta T cannot reach it (bug 71
 * gave it SWI_TJD_NEVER).
 *
 * The check: one sequence of swe_vis_limit_mag(), swe_heliacal_pheno_ut()
 * and swe_heliacal_ut() requests at a few instants and one longitude,
 * alternating the atmosphere (pressure, temperature, humidity, visual
 * range, extinction coefficient), eye height, latitude in the dark-sky
 * mode, the high-precision flag, the ephemeris (Swiss, Moshier, de200), a
 * user delta T, swe_close(), and for Sirius two star catalogues (the
 * second, written to a temporary directory, gives it -1.00 for -1.46);
 * each answer -- return code, every returned value, serr -- against the
 * same request in a fresh process (fork). Values within 1e-6 (see TOL),
 * serr equal. After the fix all 37 agree within that.
 *
 * Before it, 17 fail. Venus's limiting magnitude at noon, the previous
 * request's for humidity 40 -> 80 % (-6.447 for -7.121, 0.67 mag),
 * temperature 15 -> 30 C (0.62), a visual range of 40 km (0.43), an
 * extinction coefficient of 0.25 or 0.05 (0.57), pressure 900 mbar
 * (0.53); in the dark at 47 N -> 33 S (0.10), at 1500 m (0.048), at
 * humidity 80 (0.26), and after swe_close() (0.26); Sirius's at humidity
 * 80 (0.25) and ktot 0.05 (0.12); Sirius's magnitude after swe_close() and
 * the second catalogue -1.46 for -1.00 in swe_vis_limit_mag()'s dret[7]
 * and swe_heliacal_pheno_ut()'s darr[20], and swe_heliacal_ut()'s end of
 * visibility of its heliacal rising a day early (2451778.67 for
 * 2451779.67). The upstream build fails the same 17. Run as
 *   helmemo EPHEDIR
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "swephexp.h"

#define T_DAY   2451545.0     /* 2000 Jan 1 12h UT: Venus up, in daylight */
#define T_NIGHT 2451545.45    /* 22:48 UT: Sirius up, the Sun well down */
#define NV 40
/* 1e-6, not tighter: apart from the memos, an answer after a request on
 * another ephemeris moves by up to 4e-8 deg in the Moon's altitude and
 * azimuth (de200 after Swiss, against de200 fresh: 3.9e-8 in dret[6]),
 * an order dependence outside swehel.c's memos, and not this gate's. The
 * memos' errors are 0.048 mag and more. */
#define TOL 1e-6

static const char *EPHE;
static char PATH_B[2 * AS_MAXCH];   /* a catalogue in which Sirius is -1.00, then EPHE */

enum { F_VIS, F_PHENO, F_HELUT };
struct req {
  int fn;
  const char *obj;
  double tjd;
  double lat, alt;        /* longitude fixed at 8.55 */
  double datm[4];
  int32 helflag;
  double dt;              /* user delta T in seconds, or NAN */
  int close_before;       /* swe_close() and re-open before this request */
  int cat_b;              /* the second star catalogue */
  const char *what;
};

#define ATM0 { 1013.25, 15, 40, 0 }
static const struct req R[] = {
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0,                     0, NAN, 0, 0, "base" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, { 1013.25, 15, 80, 0 },   0, NAN, 0, 0, "humidity 80" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, { 1013.25, 30, 80, 0 },   0, NAN, 0, 0, "temperature 30" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, { 1013.25, 30, 80, 40 },  0, NAN, 0, 0, "visual range 40 km" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, { 1013.25, 30, 80, 0.25 },0, NAN, 0, 0, "ktot 0.25" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, { 1013.25, 30, 80, 0.05 },0, NAN, 0, 0, "ktot 0.05 (warns)" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, { 1013.25, 30, 80, 0.05 },0, NAN, 0, 0, "ktot 0.05 again" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, { 900, 30, 80, 0.05 },    0, NAN, 0, 0, "pressure 900" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0,                     0, NAN, 0, 0, "base again" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0, SE_HELFLAG_HIGH_PRECISION, NAN, 0, 0, "high precision" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0,        SEFLG_MOSEPH, NAN, 0, 0, "Moshier" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0,        SEFLG_JPLEPH, NAN, 0, 0, "de200" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0,                     0, NAN, 0, 0, "Swiss again" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0,                     0, 100, 0, 0, "delta T 100 s" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0,                     0, NAN, 0, 0, "delta T automatic" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0,                     0, NAN, 1, 0, "after swe_close" },
  { F_VIS, "venus",  T_DAY, 47.37, 400, ATM0, SE_HELFLAG_VISLIM_DARK, NAN, 0, 0, "dark, 47 N" },
  { F_VIS, "venus",  T_DAY, -33.0, 400, ATM0, SE_HELFLAG_VISLIM_DARK, NAN, 0, 0, "dark, 33 S" },
  { F_VIS, "venus",  T_DAY, -33.0, 1500, ATM0, SE_HELFLAG_VISLIM_DARK, NAN, 0, 0, "dark, 33 S, 1500 m" },
  { F_VIS, "venus",  T_DAY, -33.0, 1500, { 1013.25, 15, 80, 0 }, SE_HELFLAG_VISLIM_DARK, NAN, 0, 0, "dark, humidity 80" },
  { F_VIS, "venus",  T_DAY, -33.0, 1500, { 1013.25, 15, 80, 0 }, SE_HELFLAG_VISLIM_DARK, NAN, 1, 0, "dark, after swe_close" },
  { F_VIS, "sirius", T_NIGHT, 47.37, 400, ATM0,                   0, NAN, 0, 0, "Sirius" },
  { F_VIS, "sirius", T_NIGHT, 47.37, 400, ATM0,                   0, NAN, 1, 1, "Sirius, catalogue B" },
  { F_VIS, "sirius", T_NIGHT, 47.37, 400, ATM0,                   0, NAN, 1, 0, "Sirius, catalogue A" },
  { F_VIS, "sirius", T_NIGHT, 47.37, 400, { 1013.25, 15, 80, 0 }, 0, NAN, 0, 0, "Sirius, humidity 80" },
  { F_VIS, "sirius", T_NIGHT, 47.37, 400, { 1013.25, 15, 80, 0.05 }, 0, NAN, 0, 0, "Sirius, ktot 0.05" },
  { F_VIS, "sirius", T_NIGHT, 47.37, 400, { 1013.25, 15, 80, 0.05 }, 0, NAN, 0, 0, "Sirius, ktot 0.05 again" },
  { F_VIS, "moon",   T_NIGHT - 0.2, 47.37, 400, ATM0,             0, NAN, 0, 0, "Moon" },
  { F_VIS, "moon",   T_NIGHT - 0.2, 47.37, 400, { 1013.25, 15, 80, 0 }, 0, NAN, 0, 0, "Moon, humidity 80" },
  { F_PHENO, "venus", 2451545.0 - 7 * 30, 47.37, 400, ATM0,      0, NAN, 0, 0, "pheno" },
  { F_PHENO, "venus", 2451545.0 - 7 * 30, 47.37, 400, { 1013.25, 15, 80, 0 }, 0, NAN, 0, 0, "pheno, humidity 80" },
  { F_PHENO, "venus", 2451545.0 - 7 * 30, 47.37, 400, { 1013.25, 30, 80, 0 }, 0, NAN, 0, 0, "pheno, temperature 30" },
  { F_PHENO, "venus", 2451545.0 - 7 * 30, 47.37, 400, { 1013.25, 30, 80, 0 }, 0, NAN, 1, 0, "pheno, after swe_close" },
  { F_PHENO, "sirius", 2451545.0 + 7 * 30, 47.37, 400, ATM0,     0, NAN, 0, 0, "pheno Sirius" },
  { F_PHENO, "sirius", 2451545.0 + 7 * 30, 47.37, 400, ATM0,     0, NAN, 1, 1, "pheno Sirius, catalogue B" },
  { F_HELUT, "sirius", 2451545.0, 47.37, 400, ATM0,              0, NAN, 1, 0, "heliacal_ut Sirius" },
  { F_HELUT, "sirius", 2451545.0, 47.37, 400, ATM0,              0, NAN, 1, 1, "heliacal_ut Sirius, catalogue B" },
};
#define NREQ ((int) (sizeof R / sizeof R[0]))

struct ans { int n; double v[NV]; char serr[AS_MAXCH]; };

static void open_lib(int cat_b)
{
  swe_set_ephe_path(cat_b ? PATH_B : (char *) EPHE);
  swe_set_jpl_file("de200.eph");
}

static void ask(const struct req *q, struct ans *a)
{
  double geo[3], datm[4], dobs[6] = { 36, 1, 0, 0, 0, 0 }, d[50];
  char obj[AS_MAXCH];
  int32 rf;
  int i, n;
  geo[0] = 8.55; geo[1] = q->lat; geo[2] = q->alt;
  memcpy(datm, q->datm, sizeof datm);
  memset(d, 0, sizeof d);
  memset(a, 0, sizeof *a);
  strcpy(obj, q->obj);
  swe_set_delta_t_userdef(isnan(q->dt) ? SE_DELTAT_AUTOMATIC : q->dt / 86400);
  /* the site the call sets for itself, set by the caller too: before bug
   * 83's fix (G82) a heliacal call after a setter lost its own observer */
  swe_set_topo(geo[0], geo[1], geo[2]);
  if (q->fn == F_VIS) {
    rf = swe_vis_limit_mag(q->tjd, geo, datm, dobs, obj, q->helflag, d, a->serr);
    n = 8;
  } else if (q->fn == F_PHENO) {
    rf = swe_heliacal_pheno_ut(q->tjd, geo, datm, dobs, obj, SE_HELIACAL_RISING,
                               q->helflag, d, a->serr);
    n = 30;
  } else {
    rf = swe_heliacal_ut(q->tjd, geo, datm, dobs, obj, SE_HELIACAL_RISING,
                         q->helflag | SEFLG_SWIEPH, d, a->serr);
    n = 3;
  }
  a->v[a->n++] = rf;
  for (i = 0; i < n; i++)
    a->v[a->n++] = d[i];
}

/* run `fn` in a forked child, which starts from this process's state */
static int in_child(void (*fn)(int, struct ans *), int k, struct ans *out, int nout)
{
  int fd[2];
  ssize_t want = (ssize_t) (nout * sizeof *out), got = 0, r;
  pid_t pid;
  if (pipe(fd) != 0)
    return -1;
  fflush(stdout);
  if ((pid = fork()) < 0)
    return -1;
  if (pid == 0) {
    close(fd[0]);
    fn(k, out);
    got = 0;
    while (got < want && (r = write(fd[1], (char *) out + got, want - got)) > 0)
      got += r;
    _exit(got == want ? 0 : 3);
  }
  close(fd[1]);
  while (got < want && (r = read(fd[0], (char *) out + got, want - got)) > 0)
    got += r;
  close(fd[0]);
  waitpid(pid, NULL, 0);
  return got == want ? 0 : -1;
}

static void fresh(int k, struct ans *a)
{
  open_lib(R[k].cat_b);
  ask(&R[k], a);
}

static void sequence(int k, struct ans *a)
{
  int i, cat = 0;
  (void) k;
  open_lib(0);
  for (i = 0; i < NREQ; i++) {
    if (R[i].close_before || R[i].cat_b != cat) {
      swe_close();
      open_lib(R[i].cat_b);
      if (R[i].cat_b != cat) {
        /* swe_fixstar_mag()'s own one-star memo in sweph.c (sp.fs1mag)
         * outlives swe_close() too; that is not this gate's, so ask it
         * for another star and leave only swehel.c's memo to answer */
        char star[AS_MAXCH], serr[AS_MAXCH];
        double mag;
        strcpy(star, "aldebaran");
        swe_fixstar_mag(star, &mag, serr);
      }
      cat = R[i].cat_b;
    }
    ask(&R[i], &a[i]);
  }
}

static char CATDIR[] = "/tmp/helmemo-XXXXXX";
static char CATFILE[sizeof CATDIR + 16];

static int make_catalogue_b(void)
{
  FILE *fp;
  if (mkdtemp(CATDIR) == NULL)
    return -1;
  snprintf(CATFILE, sizeof CATFILE, "%s/sefstars.txt", CATDIR);
  if ((fp = fopen(CATFILE, "w")) == NULL)
    return -1;
  fputs("Sirius     ,alCMa,ICRS,06,45,08.91728,-16,42,58.0171,-546.01,-1223.07,-5.5,379.21,-1.00,-16, 1591\n"
        "Aldebaran  ,alTau,ICRS,04,35,55.23907,+16,30,33.4885,63.45,-188.94,54.26,48.94,0.86, 16,  629\n", fp);
  fclose(fp);
  snprintf(PATH_B, sizeof PATH_B, "%s:%s", CATDIR, EPHE);
  return 0;
}

static void remove_catalogue_b(void)
{
  remove(CATFILE);
  rmdir(CATDIR);
}

int main(int argc, char **argv)
{
  static struct ans seq[NREQ], one;
  int i, j, nfail = 0;
  EPHE = argc > 1 ? argv[1] : "../ephe";
  if (make_catalogue_b() != 0) {
    printf("G81 FAIL: cannot write the second catalogue\n");
    return 1;
  }
  if (in_child(sequence, 0, seq, NREQ) != 0) {
    printf("G81 FAIL: the sequence did not run\n");
    remove_catalogue_b();
    return 1;
  }
  for (i = 0; i < NREQ; i++) {
    double d = 0;
    int at = -1;
    if (in_child(fresh, i, &one, 1) != 0) {
      printf("  %-26s the fresh request did not run\n", R[i].what);
      nfail++;
      continue;
    }
    if (one.v[0] < 0 && one.v[0] != -2)
      printf("  %-26s note: rf %g %s\n", R[i].what, one.v[0], one.serr);
    for (j = 0; j < one.n; j++) {
      double e = fabs(seq[i].v[j] - one.v[j]);
      if (!(e <= d)) { d = e; at = j; }
    }
    if (seq[i].n != one.n || d > TOL || strcmp(seq[i].serr, one.serr) != 0) {
      printf("  %-26s FAIL: [%d] %.10g, fresh %.10g (off by %.3g)%s%s\n", R[i].what,
             at - 1, at >= 0 ? seq[i].v[at] : 0, at >= 0 ? one.v[at] : 0, d,
             strcmp(seq[i].serr, one.serr) ? "; serr: " : "",
             strcmp(seq[i].serr, one.serr) ? seq[i].serr : "");
      nfail++;
    }
  }
  remove_catalogue_b();
  if (nfail)
    printf("G81 FAIL: %d of %d requests\n", nfail, NREQ);
  else
    printf("G81 PASS (%d requests)\n", NREQ);
  return nfail != 0;
}
