#ifndef ITOA_H
#define ITOA_H

// 2^1: 256
// 2^2: 65536
// 2^4: 4294967296
// 2^8: 18446744073709551616

#define ITOA_SIZE                                                              \
    sizeof(int) <= 1 ? 5 : sizeof(int) <= 2 ? 7 : sizeof(int) <= 4 ? 12 : 22

int itoa(char *buf, int i);

#endif
