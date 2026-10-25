#define main probe_main
#include "probe.c"
#undef main
int main(void) { double A; for (A = 0; A <= 360; A += 90) printf("%g %.5f\n", A, vert_vec(289.0071, 0, 23.4377, A)); return 0; }
