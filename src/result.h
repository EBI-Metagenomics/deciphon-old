#ifndef RESULT_H
#define RESULT_H

#include "imm/imm.h"
#include "lib/list.h"

struct dcp_result
{
    uint32_t                 profid;
    uint32_t                 seqid;
    imm_float                alt_loglik;
    struct imm_result const* alt_result;
    char const*              alt_stream;
    char const*              alt_codon_stream;
    imm_float                null_loglik;
    struct imm_result const* null_result;
    char const*              null_stream;
    char const*              null_codon_stream;
    struct list              link;
};

#endif
