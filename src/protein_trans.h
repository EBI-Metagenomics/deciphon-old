#ifndef DCP_PROTEIN_TRANS_H
#define DCP_PROTEIN_TRANS_H

#include "imm/imm.h"

#define DCP_PROTEIN_TRANS_SIZE 7

struct dcp_protein_trans
{
    union
    {
        struct
        {
            imm_float MM;
            imm_float MI;
            imm_float MD;
            imm_float IM;
            imm_float II;
            imm_float DM;
            imm_float DD;
        } __attribute__((packed));
        struct
        {
            imm_float data[DCP_PROTEIN_TRANS_SIZE];
        };
    };
};

#endif
