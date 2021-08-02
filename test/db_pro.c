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
    struct dcp_pro_profile p;
    pro_profile_with_3cores(&p);
    struct imm_abc const *abc = dcp_super(&p)->abc;
    FILE *fd = fopen(TMPDIR "/3core_nodes.dcp", "wb");

    struct dcp_db *db = dcp_db_openw(fd, abc, dcp_db_pro(p.cfg));
    NOTNULL(db);

    EQ(dcp_db_write(db, dcp_super(&p)), IMM_SUCCESS);

    dcp_del(&p);
    EQ(dcp_db_close(db), IMM_SUCCESS);
    fclose(fd);
}
