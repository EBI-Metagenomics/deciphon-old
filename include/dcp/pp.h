/*
 * Protein profile.
 */
#ifndef DCP_PP_H
#define DCP_PP_H

#include "dcp/export.h"
#include "imm/imm.h"

#define DCP_PP_BITS_ID 16

#define DCP_PP_MATCH_ID (0U << (DCP_PP_BITS_ID - 2))
#define DCP_PP_INSERT_ID (1U << (DCP_PP_BITS_ID - 2))
#define DCP_PP_DELETE_ID (2U << (DCP_PP_BITS_ID - 2))
#define DCP_PP_SPECIAL_ID (3U << (DCP_PP_BITS_ID - 2))
#define DCP_PP_R_ID (DCP_PP_SPECIAL_ID | 0U)
#define DCP_PP_S_ID (DCP_PP_SPECIAL_ID | 1U)
#define DCP_PP_N_ID (DCP_PP_SPECIAL_ID | 2U)
#define DCP_PP_B_ID (DCP_PP_SPECIAL_ID | 3U)
#define DCP_PP_E_ID (DCP_PP_SPECIAL_ID | 4U)
#define DCP_PP_J_ID (DCP_PP_SPECIAL_ID | 5U)
#define DCP_PP_C_ID (DCP_PP_SPECIAL_ID | 6U)
#define DCP_PP_T_ID (DCP_PP_SPECIAL_ID | 7U)

enum dcp_entry_distr
{
    UNIFORM = 0,
    OCCUPANCY = 1,
};

struct dcp_trans
{
    imm_float MM;
    imm_float MI;
    imm_float MD;
    imm_float IM;
    imm_float II;
    imm_float DM;
    imm_float DD;
};

struct dcp_special_trans
{
    imm_float NN;
    imm_float NB;
    imm_float EC;
    imm_float CC;
    imm_float CT;
    imm_float EJ;
    imm_float JJ;
    imm_float JB;
    imm_float RR;
    imm_float BM;
    imm_float ME;
};

struct dcp_pp;

DCP_API void dcp_pp_state_name(unsigned id, char name[8]);

DCP_API struct dcp_pp *
dcp_pp_create(imm_float const null_lprobs[IMM_AMINO_SIZE],
              imm_float const null_lodds[IMM_AMINO_SIZE], imm_float epsilon,
              unsigned core_size, enum dcp_entry_distr entry_distr);

DCP_API int dcp_pp_add_node(struct dcp_pp *pp,
                            imm_float const lprobs[IMM_AMINO_SIZE]);

DCP_API int dcp_pp_add_trans(struct dcp_pp *pp, struct dcp_trans trans);

DCP_API struct imm_hmm *dcp_pp_null_hmm(struct dcp_pp *pp);

DCP_API struct imm_hmm *dcp_pp_alt_hmm(struct dcp_pp *pp);

DCP_API struct imm_dp *dcp_pp_null_new_dp(struct dcp_pp *pp);

DCP_API struct imm_dp *dcp_pp_alt_new_dp(struct dcp_pp *pp);

DCP_API void dcp_pp_destroy(struct dcp_pp *pp);

DCP_API void dcp_pp_set_target_length(struct dcp_pp *pp, unsigned target_length,
                                      bool multihits, bool hmmer3_compat);

#endif
