#ifndef DCP_PRO_MODEL_H
#define DCP_PRO_MODEL_H

#include "dcp/entry_distr.h"
#include "dcp/export.h"
#include "dcp/profile_types.h"

#define DCP_PRO_MODEL_MATCH_ID (0U << (DCP_PROFILE_BITS_ID - 2))
#define DCP_PRO_MODEL_INSERT_ID (1U << (DCP_PROFILE_BITS_ID - 2))
#define DCP_PRO_MODEL_DELETE_ID (2U << (DCP_PROFILE_BITS_ID - 2))
#define DCP_PRO_MODEL_SPECIAL_ID (3U << (DCP_PROFILE_BITS_ID - 2))
#define DCP_PRO_MODEL_R_ID (DCP_PRO_MODEL_SPECIAL_ID | 0U)
#define DCP_PRO_MODEL_S_ID (DCP_PRO_MODEL_SPECIAL_ID | 1U)
#define DCP_PRO_MODEL_N_ID (DCP_PRO_MODEL_SPECIAL_ID | 2U)
#define DCP_PRO_MODEL_B_ID (DCP_PRO_MODEL_SPECIAL_ID | 3U)
#define DCP_PRO_MODEL_E_ID (DCP_PRO_MODEL_SPECIAL_ID | 4U)
#define DCP_PRO_MODEL_J_ID (DCP_PRO_MODEL_SPECIAL_ID | 5U)
#define DCP_PRO_MODEL_C_ID (DCP_PRO_MODEL_SPECIAL_ID | 6U)
#define DCP_PRO_MODEL_T_ID (DCP_PRO_MODEL_SPECIAL_ID | 7U)

struct dcp_pro_model_trans
{
    imm_float MM;
    imm_float MI;
    imm_float MD;
    imm_float IM;
    imm_float II;
    imm_float DM;
    imm_float DD;
};

struct dcp_pro_model;
struct dcp_pro_model_node;
struct dcp_pro_profile;

DCP_API struct dcp_pro_model *
dcp_pro_model_new(struct imm_amino const *amino, struct imm_nuclt const *nuclt,
                  imm_float const null_lprobs[IMM_AMINO_SIZE],
                  imm_float const null_lodds[IMM_AMINO_SIZE], imm_float epsilon,
                  unsigned core_size, enum dcp_entry_distr entry_distr);

DCP_API int dcp_pro_model_add_node(struct dcp_pro_model *m,
                                   imm_float const lprobs[IMM_AMINO_SIZE]);

DCP_API int dcp_pro_model_add_trans(struct dcp_pro_model *m,
                                    struct dcp_pro_model_trans trans);

DCP_API int dcp_pro_model_set(struct dcp_pro_model *m,
                              struct dcp_pro_profile *p);

#endif
