#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_pro_model(void);

int main(void) { return hope_status(); }

void test_pro_model(void)
{
    unsigned core_size = 3;
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    imm_float null_lprobs[IMM_AMINO_SIZE] = {
        -2.540f, -4.189f, -2.927f, -2.705f, -3.226f, -2.666f, -3.775f,
        -2.830f, -2.822f, -2.339f, -3.739f, -3.183f, -3.030f, -3.229f,
        -2.916f, -2.683f, -2.917f, -2.697f, -4.472f, -3.492f};
    imm_float null_lodds[IMM_AMINO_SIZE] = {
        -1.107f, 0.0893f, -2.34f, 0.548f,  0.684f,  -0.409f, 0.195f,
        -1.272f, -0.112f, -1.57f, 0.0891f, -0.746f, 0.475f,  1.016f,
        0.940f,  -0.405f, 0.849f, -0.673f, -2.088f, -0.02f};
    imm_float match_lprobs1[IMM_AMINO_SIZE] = {
        0.819f,  -1.057f, 1.625f, -0.496f, -1.208f, 0.133f, -0.504f,
        -0.967f, 0.146f,  0.051f, -0.302f, 0.424f,  1.178f, -2.117f,
        0.352f,  1.456f,  0.254f, -0.377f, -0.110f, 0.469f};
    imm_float match_lprobs2[IMM_AMINO_SIZE] = {
        1.063f,  -1.583f, -1.171f, 1.664f,  0.212f, 0.749f,  -0.073f,
        -1.100f, -1.248f, 0.135f,  -0.909f, 0.864f, -0.061f, 1.365f,
        0.744f,  1.002f,  0.433f,  1.770f,  0.992f, 1.849f};
    imm_float match_lprobs3[IMM_AMINO_SIZE] = {
        0.958f,  -1.258f, 0.879f, 0.157f,  -0.134f, -1.645f, 1.183f,
        0.231f,  1.669f,  0.551f, -0.709f, 1.164f,  -0.247f, -1.196f,
        -0.435f, 0.881f,  0.464f, 0.186f,  -0.144f, -1.181f};
    struct dcp_pro_model_trans trans0 = {0.897f, 0.747f,  0.558f, 1.050f,
                                         0.635f, -0.535f, 0.877f};
    struct dcp_pro_model_trans trans1 = {1.558f, 0.654f,  -0.922f, -1.525f,
                                         0.724f, -0.153f, -2.197f};
    struct dcp_pro_model_trans trans2 = {-0.981f, 0.785f,  0.186f, 0.167f,
                                         0.621f,  -0.875f, 0.969f};
    struct dcp_pro_model_trans trans3 = {-0.657f, -0.064f, -0.529f, -0.450f,
                                         -0.097f, -0.044f, -2.194f};

    struct dcp_pro_model *model =
        dcp_pro_model_new(cfg, null_lprobs, null_lodds);

    EQ(dcp_pro_model_setup(model, core_size), IMM_SUCCESS);

    EQ(dcp_pro_model_add_node(model, match_lprobs1), IMM_SUCCESS);
    EQ(dcp_pro_model_add_node(model, match_lprobs2), IMM_SUCCESS);
    EQ(dcp_pro_model_add_node(model, match_lprobs3), IMM_SUCCESS);

    EQ(dcp_pro_model_add_trans(model, trans0), IMM_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, trans1), IMM_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, trans2), IMM_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, trans3), IMM_SUCCESS);

    struct dcp_pro_profile *p =
        dcp_pro_profile_new(cfg, dcp_meta("NAME0", "ACC0"));

    dcp_pro_profile_absorb(p, model);

    dcp_del(model);
}
