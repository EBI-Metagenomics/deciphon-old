#include "rand.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "abcdefghijklmnopqrstuvwxyz"
                         "0123456789";

static unsigned mix(unsigned a, unsigned b, unsigned c);

#define UINT(x) ((unsigned)(((unsigned long)x) % UINT_MAX))

void rand_string(unsigned size, char dst[size])
{
    /* https://stackoverflow.com/a/323302 */
    unsigned seed = mix(UINT(clock()), UINT(time(NULL)), UINT(getpid()));
    srand(seed);
    for (unsigned i = 0; i < size; ++i)
        dst[i] = alphanum[rand() % (int)strlen(alphanum)];
}

static unsigned mix(unsigned a, unsigned b, unsigned c)
{
    static_assert(sizeof(unsigned) >= 4, "minimum unsigned size");
    /* http://www.concentric.net/~Ttwang/tech/inthash.htm */
    /* http://burtleburtle.net/bob/hash/doobs.html */
    a = a - b;
    a = a - c;
    a = a ^ (c >> 13);
    b = b - c;
    b = b - a;
    b = b ^ (a << 8);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 13);
    a = a - b;
    a = a - c;
    a = a ^ (c >> 12);
    b = b - c;
    b = b - a;
    b = b ^ (a << 16);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 5);
    a = a - b;
    a = a - c;
    a = a ^ (c >> 3);
    b = b - c;
    b = b - a;
    b = b ^ (a << 10);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 15);
    return c;
}
