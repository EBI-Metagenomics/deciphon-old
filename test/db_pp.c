#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_db_openw_3core_nodes(void);

int main(void)
{
    test_db_openw_3core_nodes();
    return hope_status();
}

void test_db_openw_3core_nodes(void)
{
    imm_float null_lprobs[IMM_AMINO_SIZE] =
        IMM_ARR(-2.540912081508539, -4.1890948987679115, -2.9276587578784476,
                -2.7056061901315998, -3.2262479321978232, -2.6663263733558105,
                -3.7757541131771606, -2.8300619150291904, -2.8227508671445096,
                -2.3395312748559522, -3.739255274772411, -3.1835424653853783,
                -3.0305224958425274, -3.2298381926580353, -2.916961759390943,
                -2.6833127114700477, -2.9174998187846013, -2.6979756205426138,
                -4.472958413679587, -3.4928752662451816);

    imm_float null_lodds[IMM_AMINO_SIZE] =
        IMM_ARR(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    struct dcp_pp_cfg cfg = {3, DCP_PP_ENTRY_DISTR_OCCUPANCY, (imm_float)0.01};
    struct dcp_pp *pp = dcp_pp_create(null_lprobs, null_lodds, cfg);
#if 0
    struct imm_dna const *dna = &imm_dna_default;
    struct imm_abc const *abc = imm_super(imm_super(dna));
    FILE *fd = fopen(TMPDIR "/empty.dcp", "wb");
    struct dcp_db_cfg cfg = {.prof_type = DCP_PROFILE_TYPE_PLAIN};
    struct dcp_db *db = dcp_db_openw(fd, abc, cfg);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
#endif
}
