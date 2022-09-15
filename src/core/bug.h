#ifndef CORE_BUG_H
#define CORE_BUG_H

/*
 * Force a compilation error if condition is true, but also produce a
 * result (of value 0 and type int), so the expression can be used
 * e.g. in a structure initializer (or where-ever else comma expressions
 * aren't permitted).
 *
 * Acknowledgement: Linux kernel developers.
 */
#define BUILD_BUG_ON_ZERO(e) ((int)(sizeof(struct { int : (-!!(e)); })))

/*
 * Acknowledgement: Linux kernel developers.
 */
#define BUG()                                                                  \
    do                                                                         \
    {                                                                          \
        printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        exit(EXIT_FAILURE);                                                    \
        __builtin_unreachable();                                               \
    } while (1)

#endif
