#include "dotenv.h"
#define _POSIX_C_SOURCE 200112L
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

enum
{
    ENDFILE = -1,
    LINE_SIZE = 2 * FILENAME_MAX,
};

static struct
{
    char path[FILENAME_MAX];
    char line[LINE_SIZE];
    bool override;
    char *name;
    char *value;
} self = {0};

/*
 * Copy string src to buffer dst of size dsize. At most dsize-1
 * chars will be copied.  Always NUL terminates (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 *
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 */
static size_t dotenv_strlcpy(char *dst, const char *src, size_t dsize)
{
    const char *osrc = src;
    size_t nleft = dsize;

    /* Copy as many bytes as will fit. */
    if (nleft != 0)
    {
        while (--nleft != 0)
        {
            if ((*dst++ = *src++) == '\0') break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src. */
    if (nleft == 0)
    {
        if (dsize != 0) *dst = '\0'; /* NUL-terminate dst */
        while (*src++)
            ;
    }

    return (size_t)(src - osrc - 1); /* count does not include NUL */
}

/*
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left).  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occurred.
 *
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 */
static size_t dotenv_strlcat(char *dst, const char *src, size_t dsize)
{
    const char *odst = dst;
    const char *osrc = src;
    size_t n = dsize;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end. */
    while (n-- != 0 && *dst != '\0')
        dst++;
    dlen = dst - odst;
    n = dsize - dlen;

    if (n-- == 0) return (dlen + strlen(src));
    while (*src != '\0')
    {
        if (n != 0)
        {
            *dst++ = *src;
            n--;
        }
        src++;
    }
    *dst = '\0';

    return (dlen + (src - osrc)); /* count does not include NUL */
}

static bool is_directory(char const *path)
{
    struct stat sb = {0};
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

static int next_line(FILE *fp)
{
    if (!fgets(self.line, LINE_SIZE - 1, fp))
    {
        if (feof(fp)) return ENDFILE;

        return DOTENV_EIO;
    }

    size_t ln = strlen(self.line) - 1;
    if (self.line[ln] == '\n') self.line[ln] = 0;
    return DOTENV_OK;
}

static int expect_spaces_or_comment(char const *p)
{
    while (*p)
    {
        if (*p == '#') return DOTENV_OK;
        if (!isspace(*p)) return DOTENV_EPARSE;
        ++p;
    }
    return DOTENV_OK;
}

static int parse_name(char *p, char **end)
{
    while (isspace(*p))
        ++p;

    if (isalpha(*p) || *p == '_')
    {
        self.name = p;

        ++p;
        while (isalnum(*p) || *p == '_')
            ++p;

        if (*p != '=') return DOTENV_EPARSE;

        *p = 0;
        *end = p;
        return DOTENV_OK;
    }

    if (!*p || *p == '#')
    {
        *end = 0;
        return DOTENV_OK;
    }

    return DOTENV_EPARSE;
}

static int parse_value(char *p)
{
    if (*p == '"')
    {
        ++p;
        self.value = p;

        int escape = *p == '\\';
        while (!(*p == '"' && !escape))
        {
            escape = *p == '\\';
            ++p;
        }

        if (!*p) return DOTENV_OK;

        *p = 0;
        return expect_spaces_or_comment(p + 1);
    }

    self.value = p;
    while (*p && !isspace(*p))
        ++p;

    if (!*p) return DOTENV_OK;

    *p = 0;
    return expect_spaces_or_comment(p + 1);
}

static int parse_file(FILE *fp)
{
    int rc = DOTENV_OK;
    while (!(rc = next_line(fp)))
    {
        char *p = 0;
        rc = parse_name(self.line, &p);
        if (rc) return rc;
        if (!p) continue;

        rc = parse_value(p + 1);
        if (rc) return rc;

        if (setenv(self.name, self.value, self.override)) return DOTENV_ESETENV;
    }

    return rc == ENDFILE ? DOTENV_OK : rc;
}

static int setup_path(char const *path)
{
    if (dotenv_strlcpy(self.path, path, FILENAME_MAX) >= FILENAME_MAX)
        return DOTENV_ELONGPATH;

    if (is_directory(self.path))
    {
        if (dotenv_strlcat(self.path, "/.env", FILENAME_MAX) >= FILENAME_MAX)
            return DOTENV_ELONGPATH;
    }
    return DOTENV_OK;
}

int dotenv_load(char const *path, bool override)
{
    int rc = setup_path(path);
    if (rc) return rc;

    self.override = override;

    FILE *fp = fopen(self.path, "r");
    if (!fp) return DOTENV_EIO;

    rc = parse_file(fp);

    fclose(fp);
    return rc;
}
