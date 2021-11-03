#include "pro_match.h"
#include "dcp.h"
#include "dcp/strlcpy.h"
#include "error.h"

void pro_match_init(struct pro_match *m, char const frag[static 1],
                    char const state[static 1], char const codon[static 1],
                    char amino)
{
    dcp_strlcpy(m->frag, frag, MEMBER_SIZE(*m, frag));
    dcp_strlcpy(m->state, state, MEMBER_SIZE(*m, state));
    dcp_strlcpy(m->codon, codon, MEMBER_SIZE(*m, codon));
    m->amino = amino;
    cco_node_init(&m->node);
}

enum dcp_rc pro_match_write(struct pro_match *m, FILE *restrict fd)
{
    if (fprintf(fd, "%s,%s,%s,%c", m->frag, m->state, m->codon, m->amino) < 0)
        return error(DCP_IOERROR, "failed to write match");
    return DCP_DONE;
}

enum dcp_rc pro_match_write_sep(FILE *restrict fd)
{
    if (fputc(';', fd) == EOF) return error(DCP_IOERROR, "failed to write sep");
    return DCP_DONE;
}
