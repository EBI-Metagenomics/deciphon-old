#ifndef CORE_C23
#define CORE_C23

#ifdef NULL
#undef NULL
#endif

#define NULL ((void *)0)

#ifndef nullptr
#define nullptr NULL
#endif

#endif
