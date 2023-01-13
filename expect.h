#ifndef EXPECT_H
#define EXPECT_H

#include "compiler.h"
#include "imm/float.h"
#include "lite_pack/lite_pack.h"
#include <stdint.h>

int expect_map_size(struct lip_file *file, unsigned size);
int expect_map_key(struct lip_file *file, char const key[]);

#endif
