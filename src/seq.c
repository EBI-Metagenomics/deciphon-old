#include "seq.h"
#include "common/safe.h"

void seq_init(struct seq *seq, char const *name, struct imm_str str)
{
    safe_strcpy(seq->name, name, SEQ_NAME_SIZE);
    seq->str = str;
    cco_node_init(&seq->node);
}
