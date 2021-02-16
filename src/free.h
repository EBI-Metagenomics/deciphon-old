#ifndef FREE_H
#define FREE_H

#include <stdlib.h>

static inline void free_c(void const* p) { free((void*)p); }

#endif
