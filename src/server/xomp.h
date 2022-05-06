#ifndef XOMP_H
#define XOMP_H

#ifdef DECIPHON_OPENM

#include <omp.h>

static inline int xomp_max_num_threads(void) { return omp_get_max_threads(); }

static inline int xomp_num_threads(void) { return omp_get_num_threads(); }

static inline int xomp_thread_num(void) { return omp_get_thread_num(); }

#else

static inline int xomp_max_num_threads(void) { return 1; }
static inline int xomp_num_threads(void) { return 1; }
static inline int xomp_thread_num(void) { return 0; }

#endif

#endif
