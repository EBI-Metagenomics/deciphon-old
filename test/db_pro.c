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
    struct dcp_pro_profile *p = pro_profile_with_3cores();
    struct imm_abc const *abc = dcp_super(p)->abc;
    FILE *fd = fopen(TMPDIR "/3core_nodes.dcp", "wb");

    enum dcp_entry_distr edistr = DCP_ENTRY_DISTR_OCCUPANCY;
    struct imm_nuclt const *nuclt = dcp_pro_profile_nuclt(p);
    struct imm_amino const *amino = dcp_pro_profile_amino(p);

    struct dcp_db *db =
        dcp_db_openw(fd, abc, dcp_db_pro(0.01f, edistr, nuclt, amino));
    NOTNULL(db);

    EQ(dcp_db_write(db, dcp_super(p)), IMM_SUCCESS);

    dcp_del(p);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
}
