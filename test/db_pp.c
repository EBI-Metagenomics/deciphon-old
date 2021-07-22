#include "data.h"
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
    struct pp_3core_nodes data = pp_3core_nodes_data();
    struct dcp_pp *pp = dcp_pp_new(data.null_lprobs, data.null_lodds, data.cfg);

    EQ(dcp_pp_add_node(pp, data.match_lprobs1), IMM_SUCCESS);
    EQ(dcp_pp_add_node(pp, data.match_lprobs2), IMM_SUCCESS);
    EQ(dcp_pp_add_node(pp, data.match_lprobs3), IMM_SUCCESS);

    EQ(dcp_pp_add_trans(pp, data.trans0), IMM_SUCCESS);
    EQ(dcp_pp_add_trans(pp, data.trans1), IMM_SUCCESS);
    EQ(dcp_pp_add_trans(pp, data.trans2), IMM_SUCCESS);
    EQ(dcp_pp_add_trans(pp, data.trans3), IMM_SUCCESS);

    FILE *fd = fopen(TMPDIR "/3core_nodes.dcp", "wb");
    struct dcp_db_cfg cfg = {.prof_type = DCP_PROFILE_TYPE_PROTEIN,
                             .float_bytes = IMM_FLOAT_BYTES,
                             .pp = data.cfg};
    struct dcp_db *db = dcp_db_openw(fd, dcp_pp_abc(pp), cfg);
    NOTNULL(db);
    struct dcp_profile prof = {0};
    dcp_profile_init(dcp_pp_abc(pp), &prof);
    prof.prof_type = DCP_PROFILE_TYPE_PROTEIN;
    prof.idx = 0;
    prof.mt = dcp_metadata("NAME0", "ACC0");
    dcp_pp_set_profile(pp, &prof);
    dcp_db_write(db, &prof);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
}
