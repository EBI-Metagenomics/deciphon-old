#include "uv.h"

int machine_ncpus(void) { return (int)uv_available_parallelism(); }
