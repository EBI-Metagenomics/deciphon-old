#ifndef PPARR_H
#define PPARR_H

#define SWITCH(a, b, v) (a * (b == v))

#define SHAPE1(i, i0) SWITCH(i0, i, 0)
#define SHAPE2(i, i0, i1) (SHAPE1(i, i0) + SWITCH(i1, i, 1))
#define SHAPE3(i, i0, i1, i2) (SHAPE2(i, i0, i1) + SWITCH(i2, i, 2))
#define SHAPE4(i, i0, i1, i2, i3) (SHAPE3(i, i0, i1, i2) + SWITCH(i3, i, 3))
#define SHAPE5(i, i0, i1, i2, i3, i4) (SHAPE4(i, i0, i1, i2, i3) + SWITCH(i4, i, 4))
#define SHAPE6(i, i0, i1, i2, i3, i4, i5) (SHAPE5(i, i0, i1, i2, i3, i4) + SWITCH(i5, i, 5))
#define SHAPE7(i, i0, i1, i2, i3, i4, i5, i6) (SHAPE6(i, i0, i1, i2, i3, i4, i5) + SWITCH(i6, i, 6))

#define GET_MACRO1(N, SHAPE1, SHAPE2, SHAPE3, SHAPE4, SHAPE5, SHAPE6, SHAPE7, NAME, ...) NAME
#define SHAPE(...) GET_MACRO1(__VA_ARGS__, SHAPE7, SHAPE6, SHAPE5, SHAPE4, SHAPE3, SHAPE2, SHAPE1)(__VA_ARGS__)

#define _1(N, i0) (1 * i0)
#define _2(N, i0, i1) (_1(N, i0) * N(1) + 1 * i1)
#define _3(N, i0, i1, i2) (_2(N, i0, i1) * N(2) + 1 * i2)
#define _4(N, i0, i1, i2, i3) (_3(N, i0, i1, i2) * N(3) + 1 * i3)
#define _5(N, i0, i1, i2, i3, i4) (_4(N, i0, i1, i2, i3) * N(4) + 1 * i4)

#define GET_MACRO2(_1, _2, _3, _4, _5, NAME, ...) NAME
#define _(N, ...) GET_MACRO2(__VA_ARGS__, _5, _4, _3, _2, _1)(N##_shape, __VA_ARGS__)

#define GET1(N, i0) N[_(N, i0)]
#define GET2(N, i0, i1) N[_(N, i0, i1)]
#define GET3(N, i0, i1, i2) N[_(N, i0, i1, i2)]
#define GET4(N, i0, i1, i2, i3) N[_(N, i0, i1, i2, i3)]
#define GET5(N, i0, i1, i2, i3, i4) N[_(N, i0, i1, i2, i3, i4)]

#define GET_MACRO3(GET1, GET2, GET3, GET4, GET5, NAME, ...) NAME
#define GET(N, ...) GET_MACRO3(__VA_ARGS__, GET5, GET4, GET3, GET2, GET1)(N, __VA_ARGS__)

#endif
