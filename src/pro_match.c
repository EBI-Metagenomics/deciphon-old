#include "pro_match.h"
#include "error.h"
#include "macros.h"
#include "xstrlcpy.h"

void pro_match_setup(struct pro_match *m, struct imm_seq frag,
                     char const state[static 1])
{
    xstrlcpy(m->frag, frag.str, frag.size + 1);
    xstrlcpy(m->state, state, MEMBER_SIZE(*m, state));
    m->codon[0] = 0;
    m->amino[0] = 0;
}

void pro_match_set_codon(struct pro_match *m, struct imm_codon codon)
{
    m->codon[0] = imm_codon_asym(&codon);
    m->codon[1] = imm_codon_bsym(&codon);
    m->codon[2] = imm_codon_csym(&codon);
}

void pro_match_set_amino(struct pro_match *m, char amino)
{
    m->amino[0] = amino;
}

enum dcp_rc pro_match_write(struct pro_match *m, FILE *restrict fd)
{
    if (fprintf(fd, "%s,%s,%s,%s", m->frag, m->state, m->codon, m->amino) < 0)
        return error(DCP_IOERROR, "failed to write match");
    return DCP_DONE;
}

enum dcp_rc pro_match_write_sep(FILE *restrict fd)
{
    if (fputc(';', fd) == EOF) return error(DCP_IOERROR, "failed to write sep");
    return DCP_DONE;
}
