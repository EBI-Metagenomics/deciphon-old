#ifndef CORE_PP_H
#define CORE_PP_H

#define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,  \
                 _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26,   \
                 _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38,   \
                 _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,   \
                 _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62,   \
                 _63, N, ...)                                                  \
    N
#define PP_RSEQ_N()                                                            \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,    \
        45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29,    \
        28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,    \
        11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define STRINGIFY(s) __STRINGIFY(s)
#define __STRINGIFY(s) #s

#define __MEMBER_REF(var, member) ((__typeof__(var) *)0)->member

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define ARRAY_SIZE_OF(var, member) ARRAY_SIZE(__MEMBER_REF((var), member))

/**
 * sizeof_field() - Report the size of a struct field in bytes
 *
 * @TYPE: The structure containing the field of interest
 * @MEMBER: The field to return the size of
 */
#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))

#define PASTE(x, y) PASTE_(x, y)
#define PASTE_(x, y) x##y

#define UNUSED(...) UNUSED_n(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define UNUSED_n(N, ...) PASTE(UNUSED_, N)(__VA_ARGS__)
#define UNUSED_1(a) (void)(a)
#define UNUSED_2(a, b)                                                         \
    (void)(a);                                                                 \
    (void)b
#define UNUSED_3(a, b, c)                                                      \
    (void)(a);                                                                 \
    (void)b;                                                                   \
    (void)c
#define UNUSED_4(a, b, c, d)                                                   \
    (void)(a);                                                                 \
    (void)b;                                                                   \
    (void)c;                                                                   \
    (void)d

#endif
