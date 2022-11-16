#include "core/filename.h"
#include "core/limits.h"
#include "core/logy.h"
#include <string.h>

#define INVAL_SIZE "file extension must be 3 characters long"
#define SHORT_FILE "filename is too short"
#define LONG_FILE "filename is too long"
#define INVAL_EXT "invalid file extension"

#define CANON "x.xxx"

static int validate(char const *filename, char const *ext);

int filename_validate(char const *filename, char const *ext)
{
    int rc = validate(filename, ext);
    if (rc) return rc;

    if (strcmp(filename + strlen(filename) - 3, ext)) return einval(INVAL_EXT);
    return RC_OK;
}

int filename_setext(char *filename, char const *ext)
{
    int rc = validate(filename, ext);
    if (rc) return rc;

    memcpy(filename + strlen(filename) - strlen(ext), ext, strlen(ext));
    return RC_OK;
}

static int validate(char const *filename, char const *ext)
{
    if (strlen(ext) != 3) return einval(INVAL_SIZE);
    if (strlen(filename) < strlen(CANON)) return einval(SHORT_FILE);
    if (strlen(filename) + 1 > FILENAME_SIZE) return einval(LONG_FILE);
    if (filename[strlen(filename) - 4] != '.') return einval(INVAL_EXT);
    return RC_OK;
}
