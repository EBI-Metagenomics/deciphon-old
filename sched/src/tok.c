#include "tok.h"
#include "common/compiler.h"
#include "common/logger.h"
#include "common/rc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DELIM " \t\r"

static void add_space_before_newline(char *line);
static enum rc next_line(FILE *restrict fd, unsigned size, char *line);

enum tok_id tok_id(struct tok const *tok) { return tok->id; }

char const *tok_value(struct tok const *tok) { return tok->value; }

unsigned tok_size(struct tok const *tok)
{
    return (unsigned)strlen(tok->value);
}

enum rc tok_next(struct tok *tok, FILE *restrict fd)
{
    if (tok->line.consumed)
    {
        enum rc rc =
            next_line(fd, MEMBER_SIZE(tok->line, data), tok->line.data);
        if (rc && rc == NOTFOUND)
        {
            tok->value = NULL;
            tok->id = TOK_EOF;
            tok->line.data[0] = '\0';
            return DONE;
        }
        if (rc && rc != NOTFOUND) return efail("get line");
        tok->value = strtok_r(tok->line.data, DELIM, &tok->line.ctx);
        tok->line.number++;
    }
    else
        tok->value = strtok_r(NULL, DELIM, &tok->line.ctx);

    if (!tok->value) return efail("get value");

    if (!strcmp(tok->value, "\n"))
        tok->id = TOK_NL;
    else
        tok->id = TOK_WORD;

    tok->line.consumed = tok->id == TOK_NL;

    return DONE;
}

static enum rc next_line(FILE *restrict fd, unsigned size, char *line)
{
    /* -1 to append space before newline if required */
    assert(size > 0);
    if (!fgets(line, (int)(size - 1), fd))
    {
        if (feof(fd)) return NOTFOUND;

        return eio("fgets");
    }

    add_space_before_newline(line);
    return DONE;
}

static void add_space_before_newline(char *line)
{
    unsigned n = (unsigned)strlen(line);
    if (n > 0)
    {
        if (line[n - 1] == '\n')
        {
            line[n - 1] = ' ';
            line[n] = '\n';
            line[n + 1] = '\0';
        }
        else
        {
            line[n - 1] = '\n';
            line[n] = '\0';
        }
    }
}
