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

int file_read_string(FILE* stream, char* str, size_t max_size)
{
    size_t count = 0;
    do {
        int c = fgetc(stream);
        if (c == EOF) {
            /* EOF means either an error or end of file but
             * we do not care which. Clear file error flag
             * and return -1 */
            clearerr(stream);
            return 1;
        } else {
            /* Cast c to char */
            *str = (char)c;
            count++;
        }
    } while ((*str++ != '\0') && (count < max_size));

    return count == max_size;
}
