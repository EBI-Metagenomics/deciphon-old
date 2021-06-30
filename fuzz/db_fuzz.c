#include "dcp/dcp.h"
#include <stdio.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    imm_log_setup(imm_log_default_callback, IMM_FATAL);

    FILE *file = fmemopen((void *)data, size, "r");
    if (!file)
        return 0;
    struct imm_abc abc = imm_abc_empty;
    imm_abc_read(&abc, file);
    fclose(file);
    return 0;
}
