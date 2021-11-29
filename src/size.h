#ifndef SIZE_H
#define SIZE_H

#define SIZE_BITS_PER_INT                                                      \
    (sizeof(unsigned) < 8                                                      \
         ? (sizeof(unsigned) < 4 ? (sizeof(unsigned) < 2 ? 8U : 16U) : 32U)    \
         : 64U)

#define SIZE_BITS_PER_LONG                                                     \
    (sizeof(long) < 8                                                          \
         ? (sizeof(long) < 4 ? (sizeof(long) < 2 ? 8U : 16U) : 32U)            \
         : 64U)

#endif
