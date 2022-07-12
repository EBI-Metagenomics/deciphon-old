#ifndef DECIPHON_CORE_EXPECT_H
#define DECIPHON_CORE_EXPECT_H

#include "deciphon/core/compiler.h"
#include "deciphon/core/lite_pack.h"
#include "imm/float.h"
#include <stdbool.h>
#include <stdint.h>

#include <stdbool.h>
#include <stdint.h>

bool expect_map_size(struct lip_file *file, unsigned size);
bool expect_map_key(struct lip_file *file, char const key[]);

#endif
