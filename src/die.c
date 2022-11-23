#include "die.h"
#include <stdio.h>
#include <stdlib.h>

void __die(char const *file, int line, char const *func)
{
    fprintf(stderr, "DIE: failure at %s:%d %s()!\n", file, line, func);
    exit(EXIT_FAILURE);
}
