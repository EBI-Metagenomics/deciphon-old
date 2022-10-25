#ifndef SCAN_PRODMAN_H
#define SCAN_PRODMAN_H

#include <stdbool.h>

void prodman_init(void);
bool prodman_setup(int nthreads);
char const *prodman_finishup(void);
void prodman_cleanup(void);

#endif
