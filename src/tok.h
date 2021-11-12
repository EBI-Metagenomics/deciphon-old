#ifndef TOK_H
#define TOK_H

#include "dcp/limits.h"
#include "fasta/rc.h"
#include <stdbool.h>
#include <stdio.h>

#define TOK_LINE_SIZE (DCP_SEQ_SIZE * 8)

enum tok_id
{
    TOK_NL,
    TOK_WORD,
    TOK_EOF,
};

struct tok
{
    unsigned id;
    char const *value;
    struct
    {
        char data[TOK_LINE_SIZE];
        unsigned number;
        bool consumed;
        char *ctx;
    } line;
};

void tok_init(struct tok *tok);
enum dcp_rc tok_next(struct tok *tok, FILE *restrict fd);

#endif
