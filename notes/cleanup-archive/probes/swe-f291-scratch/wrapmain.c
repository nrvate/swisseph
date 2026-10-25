#include <unistd.h>
#include <stdio.h>
int real_main(int argc, char **argv);
int main(int argc, char **argv) { setvbuf(stdout, NULL, _IOLBF, 0); alarm(40); return real_main(argc, argv); }
