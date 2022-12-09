#ifndef SCAN_PRODMAN_H
#define SCAN_PRODMAN_H

#include <stdio.h>

void prodman_init(void);
int prodman_setup(int nthreads);
FILE *prodman_file(int idx);
int prodman_finishup(void);
void prodman_cleanup(void);

#endif
