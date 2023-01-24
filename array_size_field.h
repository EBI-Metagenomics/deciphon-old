#ifndef ARRAY_SIZE_FIELD_H
#define ARRAY_SIZE_FIELD_H

#include "sizeof_field.h"

#define array_size_field(T, M) (sizeof_field(T, M) / sizeof(((((T *)0)->M))[0]))

#endif
