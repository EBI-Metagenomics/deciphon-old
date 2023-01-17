#ifndef EXPECT_H
#define EXPECT_H

#include "lip/lip.h"

int expect_map_size(struct lip_file *file, unsigned size);
int expect_map_key(struct lip_file *file, char const key[]);

#endif
