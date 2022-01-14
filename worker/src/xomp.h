#ifndef XOMP_H
#define XOMP_H

#ifdef OPENMP

#include <omp.h>

static inline unsigned xomp_max_num_threads(void)
{
    return (unsigned)omp_get_max_threads();
}
static inline unsigned xomp_num_threads(void)
{
    return (unsigned)omp_get_num_threads();
}
static inline unsigned xomp_thread_num(void)
{
    return (unsigned)omp_get_thread_num();
}

#else

static inline unsigned xomp_max_num_threads(void) { return 1; }
static inline unsigned xomp_num_threads(void) { return 1; }
static inline unsigned xomp_thread_num(void) { return 0; }

#endif

#endif
