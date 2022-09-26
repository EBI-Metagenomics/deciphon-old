#include "dotenv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VARS(X)                                                                \
    X("DCP_NUM_THREADS")                                                       \
    X("DCP_API_HOST")                                                          \
    X("DCP_API_PORT")                                                          \
    X("DCP_API_PREFIX")                                                        \
    X("DCP_API_KEY")

int main(int argc, char *argv[])
{
    if (argc != 3) return 1;
    if (dotenv_load(argv[1], true)) return 1;

#define X(X)                                                                   \
    if (!strcmp(argv[2], X)) return puts(getenv(X)), 0;
    VARS(X)
#undef X

    return 1;
}
