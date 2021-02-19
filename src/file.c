#include "file.h"
#include "imm/imm.h"

#define BUFFSIZE (8 * 1024)

int file_copy_content(FILE* dst, FILE* src)
{
    char   buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0) {
        if (n < BUFFSIZE && ferror(src)) {
            imm_error("failed to read from file");
            clearerr(src);
            return 1;
        }
        if (fwrite(buffer, sizeof(*buffer), n, dst) < n) {
            imm_error("failed to write to file");
            return 1;
        }
    }
    if (ferror(src)) {
        imm_error("failed to read from file");
        clearerr(src);
        return 1;
    }
    return 0;
}
