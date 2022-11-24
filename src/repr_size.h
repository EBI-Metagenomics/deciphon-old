#ifndef REPR_SIZE
#define REPR_SIZE

// type    min                  bytes   digits
// short   -32768               2       6
// int     -2147483648          4       11
// long    -9223372036854775808 8       20
// llong   -9223372036854775808 8       20

#define __repr_size_inttypes(TYPE)                                             \
    sizeof(TYPE) <= 2 ? 6 : sizeof(TYPE) <= 4 ? 11 : sizeof(TYPE) <= 8 ? 20 : 40

#define __repr_size_shrt __repr_size_inttypes(short)
#define __repr_size_int __repr_size_inttypes(int)
#define __repr_size_long __repr_size_inttypes(long)

// 9 for IEEE float and 17 for IEEE double
#define __repr_size_flt 9
#define __repr_size_dbl 17

#define repr_size(TYPE)                                                        \
    _Generic((__typeof__(TYPE)){0}, short                                      \
             : __repr_size_shrt, int                                           \
             : __repr_size_int, long                                           \
             : __repr_size_long, float                                         \
             : __repr_size_flt, double                                         \
             : __repr_size_dbl)

#define repr_size_field(TYPE, MEMBER) repr_size(((TYPE *)0)->MEMBER)

/* Reference: https://stackoverflow.com/a/21162120 */
#define repr_size_flt_fmt ".9g"
#define repr_size_dbl_fmt ".17g"

#endif
