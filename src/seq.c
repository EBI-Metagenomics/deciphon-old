#include "seq.h"
#include "safe.h"

void seq_init(struct seq *seq, char const id[static 1],
                  struct imm_str str)
{
    safe_strcpy(seq->name, id, DCP_SEQ_NAME_SIZE);
    seq->str = str;
    cco_node_init(&seq->node);
}
