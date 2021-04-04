#ifndef PPARR_H
#define PPARR_H

#define STRIDES1_0 1
#define SIZE1 (GET_SHAPE(0))
#define _1(i0) (STRIDES1_0 * i0)

#define STRIDES2_0 (GET_SHAPE(1) * STRIDES2_1)
#define STRIDES2_1 1
#define SIZE2 (GET_SHAPE(0) * GET_SHAPE(1))
#define _2(i0, i1) (STRIDES2_0 * i0 + STRIDES2_1 * i1)

#define STRIDES3_0 (GET_SHAPE(1) * STRIDES3_1)
#define STRIDES3_1 (GET_SHAPE(2) * STRIDES3_2)
#define STRIDES3_2 1
#define SIZE3 (GET_SHAPE(0) * GET_SHAPE(1) * GET_SHAPE(2))
#define _3(i0, i1, i2) (STRIDES3_0 * i0 + STRIDES3_1 * i1 + STRIDES3_2 * i2)

#define STRIDES4_0 (GET_SHAPE(1) * STRIDES4_1)
#define STRIDES4_1 (GET_SHAPE(2) * STRIDES4_2)
#define STRIDES4_2 (GET_SHAPE(3) * STRIDES4_3)
#define STRIDES4_3 1
#define SIZE4 (GET_SHAPE(0) * GET_SHAPE(1) * GET_SHAPE(2) * GET_SHAPE(3))
#define _4(i0, i1, i2, i3) (STRIDES4_0 * i0 + STRIDES4_1 * i1 + STRIDES4_2 * i2 + STRIDES4_3 * i3)

#define STRIDES5_0 (GET_SHAPE(1) * STRIDES5_1)
#define STRIDES5_1 (GET_SHAPE(2) * STRIDES5_2)
#define STRIDES5_2 (GET_SHAPE(3) * STRIDES5_3)
#define STRIDES5_3 (GET_SHAPE(4) * STRIDES5_4)
#define STRIDES5_4 1
#define SIZE5 (GET_SHAPE(0) * GET_SHAPE(1) * GET_SHAPE(2) * GET_SHAPE(3) * GET_SHAPE(4))
#define _5(i0, i1, i2, i3, i4) (STRIDES5_0 * i0 + STRIDES5_1 * i1 + STRIDES5_2 * i2 + STRIDES5_3 * i3 + STRIDES5_4 * i4)

#define STRIDES6_0 (GET_SHAPE(1) * STRIDES6_1)
#define STRIDES6_1 (GET_SHAPE(2) * STRIDES6_2)
#define STRIDES6_2 (GET_SHAPE(3) * STRIDES6_3)
#define STRIDES6_3 (GET_SHAPE(4) * STRIDES6_4)
#define STRIDES6_4 (GET_SHAPE(5) * STRIDES6_5)
#define STRIDES6_5 1
#define SIZE5 (GET_SHAPE(0) * GET_SHAPE(1) * GET_SHAPE(2) * GET_SHAPE(3) * GET_SHAPE(4) * GET_SHAPE(5))
#define _6(i0, i1, i2, i3, i4, i5) (STRIDES6_0 * i0 + STRIDES6_1 * i1 + STRIDES6_2 * i2 + STRIDES6_3 * i3 + STRIDES6_4 * i4 + STRIDES6_5 * i5)

#define STRIDES7_0 (GET_SHAPE(1) * STRIDES7_1)
#define STRIDES7_1 (GET_SHAPE(2) * STRIDES7_2)
#define STRIDES7_2 (GET_SHAPE(3) * STRIDES7_3)
#define STRIDES7_3 (GET_SHAPE(4) * STRIDES7_4)
#define STRIDES7_4 (GET_SHAPE(5) * STRIDES7_5)
#define STRIDES7_5 (GET_SHAPE(6) * STRIDES7_6)
#define STRIDES7_6 1
#define SIZE5 (GET_SHAPE(0) * GET_SHAPE(1) * GET_SHAPE(2) * GET_SHAPE(3) * GET_SHAPE(4) * GET_SHAPE(5) * GET_SHAPE(6))
#define _7(i0, i1, i2, i3, i4, i5, i6) (STRIDES7_0 * i0 + STRIDES7_1 * i1 + STRIDES7_2 * i2 + STRIDES7_3 * i3 + STRIDES7_4 * i4 + STRIDES7_5 * i5 + STRIDES7_6 * i6)

#define GET_MACRO1(_1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define _(...) GET_MACRO1(__VA_ARGS__, _7, _6, _5, _4, _3, _2, _1)(__VA_ARGS__)

#define SWITCH(a, b, v) (a * (b == v))
#define SHAPE1(i0, i) SWITCH(i0, i, 0)
#define SHAPE2(i0, i1, i) (SHAPE1(i0, i) + SWITCH(i1, i, 1))
#define SHAPE3(i0, i1, i2, i) (SHAPE2(i0, i1, i) + SWITCH(i2, i, 2))
#define SHAPE4(i0, i1, i2, i3, i) (SHAPE3(i0, i1, i2, i) + SWITCH(i3, i, 3))
#define SHAPE5(i0, i1, i2, i3, i4, i) (SHAPE4(i0, i1, i2, i3, i) + SWITCH(i4, i, 4))
#define SHAPE6(i0, i1, i2, i3, i4, i5, i) (SHAPE4(i0, i1, i2, i3, i4, i) + SWITCH(i5, i, 5))
#define SHAPE7(i0, i1, i2, i3, i4, i5, i6, i) (SHAPE4(i0, i1, i2, i3, i4, i5, i) + SWITCH(i6, i, 6))

#define GET_MACRO2(SKIP, SHAPE1, SHAPE2, SHAPE3, SHAPE4, SHAPE5, NAME, ...) NAME
#define SHAPE(...) GET_MACRO2(__VA_ARGS__, SHAPE5, SHAPE4, SHAPE3, SHAPE2, SHAPE1)(__VA_ARGS__)

#endif
