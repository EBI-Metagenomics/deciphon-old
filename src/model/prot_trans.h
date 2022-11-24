#ifndef MODEL_PROT_TRANS_H
#define MODEL_PROT_TRANS_H

#include "imm/imm.h"

#define PROTEIN_TRANS_SIZE 7

struct prot_trans
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
            imm_float data[PROTEIN_TRANS_SIZE];
        };
    };
};

#endif
