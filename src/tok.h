#ifndef TOK_H
#define TOK_H

#include <stdbool.h>
#include <stdio.h>

enum tok_id
{
    TOK_NL,
    TOK_WORD,
    TOK_EOF,
};

struct tok;

struct tok *tok_new(unsigned size);
void tok_del(struct tok const *tok);
enum dcp_rc tok_next(struct tok *tok, FILE *restrict fd);

#endif
