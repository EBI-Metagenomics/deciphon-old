#ifndef DCP_H
#define DCP_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MEMBER_SIZE(var, member) sizeof(((__typeof__(var) *)0)->member)

#endif
