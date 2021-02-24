#ifndef MYTHREADS_H
#define MYTHREADS_H

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L) && !defined(__STDC_NO_THREADS__)
#include <threads.h>
#else
#include "lib/c11threads.h"
#endif

#endif
