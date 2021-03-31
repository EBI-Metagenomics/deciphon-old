#include "util.h"
#include <stdarg.h>

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h> // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

#define BUFFSIZE (8 * 1024)

int fcopy_content(FILE* dst, FILE* src)
{
    char   buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0) {
        if (n < BUFFSIZE && ferror(src)) {
            error("failed to read from file");
            clearerr(src);
            return 1;
        }
        if (fwrite(buffer, sizeof(*buffer), n, dst) < n) {
            error("failed to write to file");
            return 1;
        }
    }
    if (ferror(src)) {
        error("failed to read from file");
        clearerr(src);
        return 1;
    }
    return 0;
}

int fread_string(FILE* stream, char* str, size_t max_size)
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

void die(char const* err, ...)
{
    va_list params;
    va_start(params, err);
    error(err, params);
    va_end(params);
    exit(1);
}

void error(char const* err, ...)
{
    va_list params;
    va_start(params, err);
    vfprintf(stderr, err, params);
    fputc('\n', stderr);
    va_end(params);
}

void warn(char const* err, ...)
{
    va_list params;
    va_start(params, err);
    vfprintf(stderr, err, params);
    fputc('\n', stderr);
    va_end(params);
}
