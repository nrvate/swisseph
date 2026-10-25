#define main probe_main
#include "probe.c"
#undef main
/* classify CalcH 'H' cusps against the azimuth division */
int main(void) {
  double eps = 23.4377, cu[13], am[10];
  char serr[256];
  int il, ia, k;
  long nnear = 0, nfar = 0, nother = 0, nbeyond_far = 0, nbeyond = 0, nbeyond_mixed = 0, nnot_far = 0;
  for (il = -90; il <= 90; il++) {
    double lat = il == 0 ? 0 : il * 0.9 + 0.013;
    if (lat > 89.9 || lat < -89.9) continue;
    for (ia = 0; ia < 720; ia++) {
      double armc = ia * 0.5 + 0.0371;
      double dmc;
      int nf = 0, nn = 0, beyond;
      swe_houses_armc(armc, lat, eps, 'H', cu, am);
      dmc = atan(sin(armc * D2R) * tan(eps * D2R)) / D2R;
      beyond = lat > 0 ? dmc > lat : dmc < lat;
      for (k = 1; k <= 12; k++) {
        double A = lat > 0 ? 90 - 30 * (k - 1) : 90 + 30 * (k - 1);
        double v = vert_vec(armc, lat, eps, A);
        double d = fabs(dd(cu[k], v));
        if (d < 1e-6) { nnear++; nn++; }
        else if (fabs(d - 180) < 1e-6) { nfar++; nf++; }
        else { nother++; if (nother < 10) printf("other lat %.3f armc %.4f k %d cu %.6f vec %.6f\n", lat, armc, k, cu[k], v); }
      }
      if (beyond) { nbeyond++; if (nf == 12) nbeyond_far++; else nbeyond_mixed++; }
      else if (nf) { nnot_far++; if (nnot_far < 10) printf("far but not beyond: lat %.3f armc %.4f nf %d\n", lat, armc, nf); }
    }
  }
  printf("near %ld far %ld other %ld; beyond %ld (all 12 far %ld, else %ld); far when not beyond %ld\n",
         nnear, nfar, nother, nbeyond, nbeyond_far, nbeyond_mixed, nnot_far);
  return 0;
}
