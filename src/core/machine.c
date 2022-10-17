#include <unistd.h>

int machine_ncpus(void) { return (int)sysconf(_SC_NPROCESSORS_ONLN); }
