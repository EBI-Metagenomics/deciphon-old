#include "tok.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

struct tok
{
    unsigned id;
    char const *value;
    struct
    {
        unsigned number;
        bool consumed;
        char *ctx;
        unsigned size;
        char data[];
    } line;
};

#define DELIM " \t\r"

static void add_space_before_newline(char *line);
static enum rc next_line(FILE *restrict fd, unsigned size, char *line);

struct tok *tok_new(unsigned size)
{
    struct tok *tok = malloc(sizeof(*tok) + sizeof(char) * size);
    if (!tok) return tok;
    tok->id = TOK_NL;
    tok->value = tok->line.data;
    tok->line.number = 0;
    tok->line.consumed = true;
    tok->line.ctx = 0;
    tok->line.size = size;
    memset(tok->line.data, 0, size);
    return tok;
}

enum tok_id tok_id(struct tok const *tok) { return tok->id; }

char const *tok_value(struct tok const *tok) { return tok->value; }

unsigned tok_size(struct tok const *tok)
{
    return (unsigned)strlen(tok->value);
}

void tok_del(struct tok const *tok) { free((void *)tok); }

enum rc tok_next(struct tok *tok, FILE *restrict fd)
{
    enum rc rc = RC_DONE;

    if (tok->line.consumed)
    {
        if ((rc = next_line(fd, tok->line.size, tok->line.data)))
        {
            if (rc == RC_END)
            {
                tok->value = NULL;
                tok->id = TOK_EOF;
                tok->line.data[0] = '\0';
                return RC_DONE;
            }
            return rc;
        }
        tok->value = strtok_r(tok->line.data, DELIM, &tok->line.ctx);
        tok->line.number++;
    }
    else
        tok->value = strtok_r(NULL, DELIM, &tok->line.ctx);

    if (!tok->value) return RC_PARSEERROR;

    if (!strcmp(tok->value, "\n"))
        tok->id = TOK_NL;
    else
        tok->id = TOK_WORD;

    tok->line.consumed = tok->id == TOK_NL;

    return RC_DONE;
}

static enum rc next_line(FILE *restrict fd, unsigned size, char *line)
{
    /* -1 to append space before newline if required */
    if (!fgets(line, (int)(size - 1), fd))
    {
        if (feof(fd)) return RC_END;

        return error(RC_IOERROR, "failed to read line");
    }

    add_space_before_newline(line);
    return RC_DONE;
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
