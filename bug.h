#ifndef BUG_H
#define BUG_H

/*
 * Force a compilation error if condition is true, but also produce a
 * result (of value 0 and type int), so the expression can be used
 * e.g. in a structure initializer (or where-ever else comma expressions
 * aren't permitted).
 *
 * Acknowledgement: Linux kernel developers.
 */
#define BUILD_BUG_ON_ZERO(e) ((int)(sizeof(struct { int : (-!!(e)); })))

#endif
