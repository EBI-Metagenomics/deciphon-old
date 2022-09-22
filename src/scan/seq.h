#ifndef SCAN_SEQ_H
#define SCAN_SEQ_H

#include "imm/seq.h"
#include <stdint.h>

struct seq
{
    int64_t id;
    char const *name;
    struct imm_seq iseq;
};

void seq_init(struct seq *, struct imm_abc const *);
void seq_set(struct seq *, int64_t id, char const *name, char const *data);
int64_t seq_id(struct seq const *);
char const *seq_name(struct seq const *);
struct imm_seq const *seq_iseq(struct seq const *);

#endif
