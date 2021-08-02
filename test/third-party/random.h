/*
 * Usage example
 * -------------
 *
 * struct random rnd = random(1);
 * double dbl_value = random_dbl(&rnd);
 * uint64_t u64_value = random_u64(&rnd);
 *
 * Acknowledgement
 * ---------------
 *
 * I'm using xoshiro256+ 1.0 (David Blackman and Sebastiano Vigna) and
 * splitmix64 (Sebastiano Vigna) methods for random number generation.
 *
 * - https://prng.di.unimi.it
 * - https://github.com/svaarala/duktape/blob/master/misc/splitmix64.c
 */
#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

struct random
{
    uint64_t data[4];
};

/* A standard double (64-bit) floating-point number in IEEE floating point
 * format has 52 bits of significand, plus an implicit bit at the left of the
 * significand. Thus, the representation can actually store numbers with 53
 * significant binary digits. */
static inline double __random_u64_to_dbl(uint64_t x)
{
    return (x >> 11) * 0x1.0p-53;
}

static inline uint64_t __random_rotl(const uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

static uint64_t random_u64(struct random *rnd)
{
    const uint64_t result = rnd->data[0] + rnd->data[3];

    const uint64_t t = rnd->data[1] << 17;

    rnd->data[2] ^= rnd->data[0];
    rnd->data[3] ^= rnd->data[1];
    rnd->data[1] ^= rnd->data[2];
    rnd->data[0] ^= rnd->data[3];

    rnd->data[2] ^= t;

    rnd->data[3] = __random_rotl(rnd->data[3], 45);

    return result;
}

static uint64_t __random_splitmix64_next(uint64_t *x)
{
    uint64_t z = (*x += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

static struct random random(uint64_t seed)
{
    struct random r;
    r.data[0] = __random_splitmix64_next(&seed);
    r.data[1] = __random_splitmix64_next(&seed);
    r.data[2] = __random_splitmix64_next(&seed);
    r.data[3] = __random_splitmix64_next(&seed);
    return r;
}

static inline double random_dbl(struct random *rnd)
{
    return __random_u64_to_dbl(random_u64(rnd));
}

#endif
