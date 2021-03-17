#ifndef RESULT_H
#define RESULT_H

#include "deciphon/deciphon.h"
#include "imm/imm.h"

struct stream
{
    char* path;
    char* codons;
};

struct result
{
    imm_float                loglik;
    struct imm_result const* result;
    struct stream            stream;
};

struct dcp_result
{
    uint32_t      profid;
    uint32_t      seqid;
    struct result models[DCP_NMODELS];
};

struct dcp_result* result_create(void);

#endif
