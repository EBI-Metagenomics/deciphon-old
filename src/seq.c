#include "dcp/seq.h"
#include "xstrlcpy.h"

void dcp_seq_init(struct dcp_seq *seq, char const id[static 1],
                  char const data[static 1])
{
    xstrlcpy(seq->id, id, DCP_SEQ_ID_SIZE);
    xstrlcpy(seq->data, data, DCP_SEQ_SIZE);
    cco_node_init(&seq->node);
}
