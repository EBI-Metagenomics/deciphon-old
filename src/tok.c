#include "tok.h"
#include "error.h"
#include <string.h>

#define DELIM " \t\r"

static void add_space_before_newline(char line[TOK_LINE_SIZE]);
static enum dcp_rc next_line(FILE *restrict fd, char line[TOK_LINE_SIZE]);

void tok_init(struct tok *tok)
{
    tok->id = TOK_NL;
    tok->value = tok->line.data;
    memset(tok->line.data, '\0', TOK_LINE_SIZE);
    tok->line.number = 0;
    tok->line.consumed = true;
    tok->line.ctx = NULL;
}

enum dcp_rc tok_next(struct tok *tok, FILE *restrict fd)
{
    enum dcp_rc rc = DCP_DONE;

    if (tok->line.consumed)
    {
        if ((rc = next_line(fd, tok->line.data)))
        {
            if (rc == DCP_END)
            {
                tok->value = NULL;
                tok->id = TOK_EOF;
                tok->line.data[0] = '\0';
                return DCP_DONE;
            }
            return rc;
        }
        tok->value = strtok_r(tok->line.data, DELIM, &tok->line.ctx);
        tok->line.number++;
    }
    else
        tok->value = strtok_r(NULL, DELIM, &tok->line.ctx);

    if (!tok->value) return DCP_PARSEERROR;

    if (!strcmp(tok->value, "\n"))
        tok->id = TOK_NL;
    else
        tok->id = TOK_WORD;

    tok->line.consumed = tok->id == TOK_NL;

    return DCP_DONE;
}

static enum dcp_rc next_line(FILE *restrict fd, char line[TOK_LINE_SIZE])
{
    if (!fgets(line, TOK_LINE_SIZE - 1, fd))
    {
        if (feof(fd)) return DCP_END;

        return error(DCP_IOERROR, "failed to read line");
    }

    add_space_before_newline(line);
    return DCP_DONE;
}

static void add_space_before_newline(char line[TOK_LINE_SIZE])
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
