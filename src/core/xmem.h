#ifndef CORE_XMEM_H
#define CORE_XMEM_H

#include <stddef.h>

void *xmalloc(size_t size);
void *xmemdup(void const *mem, size_t size);
void *xrealloc(void *mem, size_t size);

#endif
