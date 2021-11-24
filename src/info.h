#ifndef INFO_H
#define INFO_H

#include "macros.h"

#define __INFO_FMT(msg) __LOCAL(__LINE__) ":" msg

void __dcp_info(char const *msg);

#define info(msg) __dcp_info(__INFO_FMT(msg))

#endif
