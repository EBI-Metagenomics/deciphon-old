#include "data.h"

struct pro_profile_3core_nodes pro_profile_with_3cores_data(void)
{
    struct pro_profile_3core_nodes data = {
        {
            .amino = &imm_amino_iupac,
            .nuclt = imm_super(imm_gc_dna()),
            .edist = DCP_ENTRY_DIST_OCCUPANCY,
            .epsilon = 0.01f,
        },
        .core_size = 3,
        .null_lprobs = {-2.54091f, -4.18909f, -2.92766f, -2.70561f, -3.22625f,
                        -2.66633f, -3.77575f, -2.83006f, -2.82275f, -2.33953f,
                        -3.73926f, -3.18354f, -3.03052f, -3.22984f, -2.91696f,
                        -2.68331f, -2.91750f, -2.69798f, -4.47296f, -3.49288f},
        .null_lodds = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .match_lprobs1 = {-4.25112f, -5.66746f, -5.45821f, -5.25250f,
                          -4.25670f, -4.83375f, -5.75580f, -3.58523f,
                          -4.98633f, -2.78386f, -0.25191f, -5.37670f,
                          -5.38471f, -5.32431f, -5.04119f, -4.64540f,
                          -4.62574f, -3.69840f, -6.12261f, -4.97173f},
        .match_lprobs2 = {-3.57128f, -5.79713f, -3.93569f, -3.24178f,
                          -5.33367f, -4.17277f, -4.07137f, -4.66233f,
                          -1.31669f, -4.02389f, -4.86889f, -1.67040f,
                          -4.52334f, -3.18856f, -1.37659f, -3.53154f,
                          -3.72192f, -4.30796f, -6.01160f, -4.84790f},
        .match_lprobs3 = {-2.73370f, -5.25734f, -2.75559f, -2.32811f,
                          -4.59103f, -1.94455f, -3.63027f, -4.07087f,
                          -2.20106f, -3.55750f, -4.30274f, -2.64987f,
                          -3.04144f, -2.68671f, -2.89772f, -2.68931f,
                          -2.71242f, -3.64220f, -5.69560f, -4.28822f},
        .trans0 = {.MM = -0.01083f,
                   .MI = -4.92694f,
                   .MD = -5.64929f,
                   .IM = -0.61958f,
                   .II = -0.77255f,
                   .DM = -0.00000f,
                   .DD = IMM_LPROB_ZERO},
        .trans1 = {.MM = -0.01083f,
                   .MI = -4.92694f,
                   .MD = -5.64929f,
                   .IM = -0.61958f,
                   .II = -0.77255f,
                   .DM = -0.48576f,
                   .DD = -0.95510f},
        .trans2 = {.MM = -0.01083f,
                   .MI = -4.92694f,
                   .MD = -5.64929f,
                   .IM = -0.61958f,
                   .II = -0.77255f,
                   .DM = -0.48576f,
                   .DD = -0.95510f},
        .trans3 = {.MM = -0.00968f,
                   .MI = -4.64203f,
                   .MD = IMM_LPROB_ZERO,
                   .IM = -0.61958f,
                   .II = -0.77255f,
                   .DM = -0.00000f,
                   .DD = IMM_LPROB_ZERO},
    };
    return data;
}

struct dcp_pro_profile *pro_profile_with_3cores(void)
{
    struct pro_profile_3core_nodes data = pro_profile_with_3cores_data();
    struct dcp_pro_model *model =
        dcp_pro_model_new(data.cfg, data.null_lprobs, data.null_lodds);

    int rc = dcp_pro_model_setup(model, data.core_size);

    rc += dcp_pro_model_add_node(model, data.match_lprobs1);
    rc += dcp_pro_model_add_node(model, data.match_lprobs2);
    rc += dcp_pro_model_add_node(model, data.match_lprobs3);

    rc += dcp_pro_model_add_trans(model, data.trans0);
    rc += dcp_pro_model_add_trans(model, data.trans1);
    rc += dcp_pro_model_add_trans(model, data.trans2);
    rc += dcp_pro_model_add_trans(model, data.trans3);

    IMM_BUG(rc);

    struct dcp_pro_profile *p =
        dcp_pro_profile_new(data.cfg, dcp_meta("NAME0", "ACC0"));

    dcp_pro_profile_init(p, model);
    dcp_del(model);
    return p;
}
