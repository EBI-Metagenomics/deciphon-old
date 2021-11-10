#ifndef MACROS_H
#define MACROS_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MEMBER_REF(var, member) ((__typeof__(var) *)0)->member
#define MEMBER_SIZE(var, member) sizeof(MEMBER_REF((var), member))

#endif
