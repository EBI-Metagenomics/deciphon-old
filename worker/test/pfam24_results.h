#ifndef PFAM24_RESULTS_H
#define PFAM24_RESULTS_H

#include "imm/imm.h"
#include "static_tensor.h"

#define logliks_shape(i) SHAPE(i, 24, 2, 2, 2, 2)
#define paths_shape(i) SHAPE(i, 24, 2, 2, 2, 2)
#define codons_shape(i) SHAPE(i, 24, 2, 2, 2, 2)

extern imm_float logliks[];
extern char const* paths[];
extern char const* codons[];

#endif